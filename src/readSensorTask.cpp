#include "readSensorTask.h"
#define FILTER_SIZE 5
#define DHT20_ADDRESS 0x38

static float temp_buffer[FILTER_SIZE] = {0};
static float hum_buffer[FILTER_SIZE] = {0};
static int buffer_index = 0;
static bool is_buffer_full = false;
static float last_lcd_temp = 0.0;
static float last_lcd_hum = 0.0;
static uint16_t last_lcd_mode = 0;

bool isDeltaChanged(float new_temp, float new_hum) {
    return abs(new_temp - last_lcd_temp) >= 0.5 ||
           abs(new_hum - last_lcd_hum) >= 1.0;
}

// DHT20 CRC calculation based on polynomial 0x31
uint8_t calDht20CRC(uint8_t *data, uint8_t length) {
    uint8_t crc = 0xFF;
    for (uint8_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc = (crc << 1);
            }
        }
    }
    return crc;
}

bool readDht20(float *out_temp, float *out_hum) {
    // 1. Init sensor
    Wire.beginTransmission(DHT20_ADDRESS);
    Wire.write(0xAC);
    Wire.write(0x33);
    Wire.write(0x00);
    if (Wire.endTransmission() != 0) {
        LOG_ERR("SENSOR", "I2C Failed at init! Error Code: %d",
                Wire.endTransmission());
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
    // 2. Read 7 bit data
    Wire.requestFrom(DHT20_ADDRESS, 7);
    if (Wire.available() < 7) {
        return false;
    }

    uint8_t data[7];
    for (int i = 0; i < 7; i++) {
        data[i] = Wire.read();
    }

    // 3. Check BUSY
    if ((data[0] & 0x80) != 0) {
        LOG_ERR("SENSOR", "DHT20 is busy");
        return false;
    }
    // 4. Check CRC
    uint8_t crc = calDht20CRC(data, 6);
    if (crc != data[6]) {
        LOG_ERR("SENSOR", "CRC Checksum failed! Expected: 0x%02X, Got: 0x%02X",
                data[6], crc);
        return false;
    }

    // 5. Convert raw data
    uint32_t raw_hum =
        ((uint32_t)data[1] << 12) | ((uint32_t)data[2] << 4) | (data[3] >> 4);
    uint32_t raw_temp =
        ((uint32_t)(data[3] & 0x0F) << 16) | ((uint32_t)data[4] << 8) | data[5];

    // 6. Convert to real values
    *out_hum = (raw_hum / 1048576.0) * 100.0;
    *out_temp = (raw_temp / 1048576.0) * 200.0 - 50.0;

    return true;
}

bool processSensorData(float temp, float hum, SensorData *out_data) {
    // 1. Validate data
    if (isnan(temp) || isnan(hum) || temp < 0 || hum < 0 || temp > 100 ||
        hum > 100.0) {
        LOG_ERR("SENSOR",
                " Sensor data out of range Temp: %.2f C, Hum: %.2f %%", temp,
                hum);
        return false;
    }

    // 2. Moving Average Filter
    temp_buffer[buffer_index] = temp;
    hum_buffer[buffer_index] = hum;
    buffer_index = (buffer_index + 1) % FILTER_SIZE;
    if (buffer_index == 0) {
        is_buffer_full = true;
    }

    int count = is_buffer_full ? FILTER_SIZE : buffer_index;
    float sum_temp = 0;
    float sum_hum = 0;

    for (int i = 0; i < count; i++) {
        sum_temp += temp_buffer[i];
        sum_hum += hum_buffer[i];
    }

    float avg_temp = sum_temp / count;
    float avg_hum = sum_hum / count;

    out_data->current_temperature = avg_temp;
    out_data->current_humidity = avg_hum;

    return true;
}

SensorData readFromDHT20() {
    SensorData data;
    data.is_dht20_ok = false;

    float raw_temp = 0.0;
    float raw_hum = 0.0;

    if (readDht20(&raw_temp, &raw_hum)) {
        if (processSensorData(raw_temp, raw_hum, &data)) {
            data.is_dht20_ok = true;
            return data;
        }
    } else {
        LOG_ERR("SENSOR", "Failed to read from DHT20 sensor");
    }
    return data;
}
void readSensorTask(void *pvParameters) {
    LOG_INFO("SENSOR", "Sensor reading task started");
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (1) {
        SystemConfig config = getSystemConfig();
        SensorData raw_data = readFromDHT20();
        float raw_temp = raw_data.current_temperature;
        float raw_hum = raw_data.current_humidity;
        bool sensor_ok = raw_data.is_dht20_ok;
        bool lcd_ok = true;

        if (sensor_ok == false) {
            if (!checkSystemErrorFlag(EVENT_SENSOR_ERROR)) {
                setSystemErrorFlag(EVENT_SENSOR_ERROR);
                xSemaphoreGive(sensor_error_semaphore);
                LOG_WARN("SENSOR", "Error reading from sensor");
            }
        } else {
            LOG_INFO("SENSOR",
                     "Done read! Temperature: %.1f C | Humidity: %.1f %%",
                     raw_temp, raw_hum);
            clearSystemErrorFlag(EVENT_SENSOR_ERROR);
            SensorData data = {raw_temp, raw_hum, sensor_ok, lcd_ok};

            uint16_t current_mode = getActiveErrorFlags();
            bool is_state_changed = (current_mode != last_lcd_mode);
            if (isDeltaChanged(raw_temp, raw_hum) || is_state_changed) {
                last_lcd_temp = raw_temp;
                last_lcd_hum = raw_hum;
                last_lcd_mode = current_mode;

                xSemaphoreGive(lcd_sync_semaphore);
            }

            setSensorData(data);

            if (raw_temp > config.max_temp_threshold) {
                if (!checkSystemErrorFlag(EVENT_TEMP_WARNING)) {
                    setSystemErrorFlag(EVENT_TEMP_WARNING);
                    xSemaphoreGive(temp_warning_semaphore);
                    LOG_WARN("SENSOR", "High Temp warning: %.2f C", raw_temp);
                }
            } else {
                if (checkSystemErrorFlag(EVENT_TEMP_WARNING)) {
                    clearSystemErrorFlag(EVENT_TEMP_WARNING);
                    xSemaphoreGive(temp_warning_semaphore);
                    LOG_INFO("SENSOR", "Temperature back to normal: %.2f C",
                             raw_temp);
                }
            }

            if (raw_hum > config.max_humidity_threshold) {
                if (!checkSystemErrorFlag(EVENT_HUM_HIGH)) {
                    setSystemErrorFlag(EVENT_HUM_HIGH);
                    clearSystemErrorFlag(EVENT_HUM_LOW);
                    xSemaphoreGive(hum_warning_semaphore);
                    LOG_WARN("SENSOR", "High Hum warning: %.2f %%", raw_hum);
                }
            } else if (raw_hum < config.min_humidity_threshold) {
                if (!checkSystemErrorFlag(EVENT_HUM_LOW)) {
                    setSystemErrorFlag(EVENT_HUM_LOW);
                    clearSystemErrorFlag(EVENT_HUM_HIGH);
                    xSemaphoreGive(hum_warning_semaphore);
                    LOG_WARN("SENSOR", "Low Hum warning: %.2f %%", raw_hum);
                }
            } else {
                if (checkSystemErrorFlag(EVENT_HUM_HIGH | EVENT_HUM_LOW)) {
                    clearSystemErrorFlag(EVENT_HUM_HIGH | EVENT_HUM_LOW);
                    xSemaphoreGive(hum_warning_semaphore);
                    LOG_INFO("SENSOR", "Humidity back to normal: %.2f %%",
                             raw_hum);
                }
            }
        }
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(config.read_interval_ms));
    }
}