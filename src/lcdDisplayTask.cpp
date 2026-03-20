#include "lcdDisplayTask.h"

#define LCD_I2C_ADDR 0x27
#define LCD_I2C_ADDR2 0x26

enum LcdMode { MODE_NORMAL, MODE_WARNING, MODE_CRITICAL };

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 16, 2);

void lcdDisplayTask(void *pvParameters) {
    LOG_INFO("LCD", "LCD display task started");
    lcd.begin();
    lcd.backlight();

    TickType_t wait_time = portMAX_DELAY;
    bool blink_state = false;
    LcdMode current_mode = MODE_NORMAL;
    while (1) {
        Wire.beginTransmission(LCD_I2C_ADDR);
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

            lcd.begin();
            lcd.backlight();
        }

        SensorData data = getSensorData();
        SystemConfig config = getSystemConfig();
        if (data.is_lcd_ok == false) {
            data.is_lcd_ok = true;
            setSensorData(data);
        }

        bool is_new_data =
            (xSemaphoreTake(lcd_sync_semaphore, wait_time) == pdTRUE);

        if (data.current_temperature > config.max_temp_threshold ||
            data.current_temperature < 15.0 ||
            data.current_humidity > config.max_humidity_threshold) {
            current_mode = MODE_CRITICAL;
        } else if (data.current_temperature > 28.0 ||
                   data.current_humidity < 50.0) {
            current_mode = MODE_WARNING;
        } else {
            current_mode = MODE_NORMAL;
        }

        if (is_new_data) {
            lcd.setCursor(0, 0);
            if (current_mode == MODE_CRITICAL) {
                lcd.print("!!! CRITICAL !!!");
            } else {
                lcd.printf("T: %4.1f C       ", data.current_temperature);
            }

            lcd.setCursor(0, 1);
            if (current_mode == MODE_CRITICAL) {
                lcd.printf("T:%2.0fC H:%2.0f%%       ",
                           data.current_temperature, data.current_humidity);

            } else {
                lcd.printf("H: %4.1f %%       ", data.current_humidity);
            }

            if (current_mode == MODE_NORMAL) {
                wait_time = portMAX_DELAY;
                lcd.backlight();
            } else if (current_mode == MODE_WARNING) {
                wait_time = pdMS_TO_TICKS(1000);
                lcd.backlight();
            } else if (current_mode == MODE_CRITICAL) {
                wait_time = pdMS_TO_TICKS(500);
            }
            blink_state = true;
        } else {
            blink_state = !blink_state;
            if (current_mode == MODE_WARNING) {
                lcd.setCursor(0, 0);
                if (blink_state) {
                    lcd.printf("*WARN* T:%2.0f C       ",
                               data.current_temperature);
                } else {
                    lcd.printf("T: %2.0f C       ", data.current_temperature);
                }
            } else if (current_mode == MODE_CRITICAL) {
                if (blink_state) {
                    lcd.backlight();
                } else {
                    lcd.noBacklight();
                }
            }
        }
    }
}