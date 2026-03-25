#include "lcdDisplayTask.h"

// Error modes for display state
enum LcdMode { MODE_NORMAL, MODE_WARNING, MODE_CRITICAL };

// Scan I2C address to optimize
uint8_t scanLCDAddress() {
    for (uint8_t addr = 0; addr < 127; addr++) {
        LOG_INFO("LCD", "Scanning I2C address: 0x%02X", addr);
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            if (addr == 0x38) {
                continue;
            }

            LOG_INFO("LCD", "Found LCD at address: 0x%02X", addr);
            return addr;
        }
    }
    return 0x00;
}

void lcdDisplayTask(void *pvParameters) {
    LOG_INFO("LCD", "LCD display task started");

    // Call func to scan address
    uint8_t default_addr = 0x00;
    while (default_addr == 0x00) {
        default_addr = scanLCDAddress();
        if (default_addr == 0x00) {
            LOG_ERR("LCD", "LCD not found");
            setSystemErrorFlag(EVENT_LCD_ERROR);
            vTaskDelay(pdMS_TO_TICKS(2000));
        }
    }

    clearSystemErrorFlag(EVENT_LCD_ERROR);
    LOG_INFO("LCD", "SUCCESS! LCD initialized at address: 0x%02X",
             default_addr);

    // Cấp phát địa chỉ mới vừa đọc được
    LiquidCrystal_I2C *lcd = new LiquidCrystal_I2C(default_addr, 16, 2);
    lcd->begin();
    lcd->backlight();

    TickType_t wait_time = portMAX_DELAY;
    bool blink_state = false;
    LcdMode current_mode = MODE_NORMAL;

    static char error_msg[8];
    while (1) {
        Wire.beginTransmission(default_addr);
        if (Wire.endTransmission() != 0) {
            if (!checkSystemErrorFlag(EVENT_LCD_ERROR)) {
                LOG_WARN("LCD", "LCD I2C error: %d", Wire.endTransmission());
                setSystemErrorFlag(EVENT_LCD_ERROR);
                SensorData data = getSensorData();
                data.is_lcd_ok = false;
            }

            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        if (checkSystemErrorFlag(EVENT_LCD_ERROR)) {
            LOG_INFO("LCD", "LCD I2C restored");
            clearSystemErrorFlag(EVENT_LCD_ERROR);

            lcd->begin();
            lcd->backlight();
        }

        // Checking semaphore when temp - previous_temp = 0.5, hum -
        // previous_hum = 1.0 or context changing
        bool is_new_data =
            (xSemaphoreTake(lcd_sync_semaphore, wait_time) == pdTRUE);

        SensorData data = getSensorData();

        SystemConfig config = getSystemConfig();
        uint32_t current_flag = getActiveErrorFlags();
        SystemState current_state = getSystemState();
        if (data.is_lcd_ok == false) {
            data.is_lcd_ok = true;
            setSensorData(data);
        }

        // 1. TÍNH TOÁN DẢI ĐO (RANGE) VÀ BIÊN ĐỘ (MARGIN)
        // 0.20 = 20% dải đo sẽ được dùng làm biên độ để xác định ngưỡng cảnh
        // báo động (dynamic warning threshold)
        float temp_range =
            config.max_temp_threshold - config.min_temp_threshold;
        float temp_margin = temp_range * 0.20;

        float hum_range =
            config.max_humidity_threshold - config.min_humidity_threshold;
        float hum_margin = hum_range * 0.20;

        // 2. TÍNH TOÁN 4 NGƯỠNG WARNING ĐỘNG
        float warn_temp_high = config.max_temp_threshold - temp_margin;
        float warn_temp_low = config.min_temp_threshold + temp_margin;
        float warn_hum_high = config.max_humidity_threshold - hum_margin;
        float warn_hum_low = config.min_humidity_threshold + hum_margin;

        // 3. XÉT DUYỆT TRẠNG THÁI (State Machine)
        if ((current_flag & EVENT_TEMP_HIGH) ||
            (current_flag & EVENT_TEMP_LOW) ||
            (current_flag & EVENT_HUM_HIGH) || (current_flag & EVENT_HUM_LOW) ||
            (current_flag & EVENT_SENSOR_ERROR)) {
            current_mode = MODE_CRITICAL;
        } else if ((data.current_temperature >= warn_temp_high) ||
                   (data.current_temperature <= warn_temp_low) ||
                   (data.current_humidity >= warn_hum_high) ||
                   (data.current_humidity <= warn_hum_low)) {
            current_mode = MODE_WARNING;
        } else {
            current_mode = MODE_NORMAL;
        }

        if (is_new_data) {
            if (current_flag & EVENT_LCD_ERROR)
                strlcpy(error_msg, "LCD ERR", sizeof(error_msg));
            else if (current_flag & EVENT_SENSOR_ERROR)
                strlcpy(error_msg, "SENSOR ", sizeof(error_msg));
            else if (current_flag & EVENT_TEMP_HIGH)
                strlcpy(error_msg, "TEMP HI", sizeof(error_msg));
            else if (current_flag & EVENT_TEMP_LOW)
                strlcpy(error_msg, "TEMP LO", sizeof(error_msg));
            else if (current_flag & EVENT_HUM_HIGH)
                strlcpy(error_msg, "HUM HI ", sizeof(error_msg));
            else if (current_flag & EVENT_HUM_LOW)
                strlcpy(error_msg, "HUM LO ", sizeof(error_msg));
            // else if (current_flag & EVENT_WIFI_DISCONN)
            //     strlcpy(error_msg, "WIFI   ", sizeof(error_msg));
            // else if (current_flag & EVENT_COREIOT_DISCONN)
            //     strlcpy(error_msg, "COREIOT", sizeof(error_msg));
            else
                strlcpy(error_msg, "       ", sizeof(error_msg));

            lcd->setCursor(0, 0);
            switch (current_mode) {
                case MODE_CRITICAL:
                    lcd->print("CRITICAL ");
                    wait_time = pdMS_TO_TICKS(500);
                    break;
                case MODE_WARNING:
                    lcd->print("WARNING  ");
                    wait_time = pdMS_TO_TICKS(1000);
                    lcd->backlight();
                    break;
                default:
                    lcd->print("NORMAL   ");
                    wait_time = portMAX_DELAY;
                    lcd->backlight();
                    break;
            }
            lcd->printf(error_msg);

            lcd->setCursor(0, 1);
            lcd->printf("T:%4.1fC  H:%4.1f%%", data.current_temperature,
                        data.current_humidity);

            // lcd->setCursor(14, 1);
            // if (current_state.is_ap_mode) {
            //     lcd->print("AP");
            // } else if (current_state.is_wifi_connected) {
            //     lcd->print("WF");
            // } else {
            //     lcd->print("  ");
            // }

            blink_state = true;
        } else {
            blink_state = !blink_state;
            if (current_mode == MODE_WARNING) {
                lcd->setCursor(0, 0);
                if (blink_state) {
                    lcd->printf("WARNING ");
                } else {
                    lcd->printf("        ");
                }
            } else if (current_mode == MODE_CRITICAL) {
                if (blink_state) {
                    lcd->backlight();
                } else {
                    lcd->noBacklight();
                }
            }
        }
    }
}