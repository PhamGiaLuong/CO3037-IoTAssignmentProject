#include <Arduino.h>
#include <unity.h>

#include "global.h"

// TC: IS-01
void test_error_triggers_led_semaphore(void) {
    // Drain semaphore
    xSemaphoreTake(sensor_led_sync_semaphore, 0);

    setSensorErrorFlag(SENSOR_FLAG_TEMP_WARN);

    // Check if task was awakened
    BaseType_t res = xSemaphoreTake(sensor_led_sync_semaphore, 0);
    TEST_ASSERT_EQUAL(pdTRUE, res);
}

// TC: IS-02
void test_error_triggers_lcd_semaphore(void) {
    xSemaphoreTake(lcd_sync_semaphore, 0);

    setSensorErrorFlag(SENSOR_FLAG_LCD_ERR);

    BaseType_t res = xSemaphoreTake(lcd_sync_semaphore, 0);
    TEST_ASSERT_EQUAL(pdTRUE, res);
}

// TC: IS-03
void test_save_load_config_flash_nvs(void) {
    SensorConfig writeConfig = {1500, 35.0, 15.0, 85.0, 45.0};
    setSensorConfig(writeConfig);
    saveConfigToFlash();  // Save to Flash

    // Corrupt RAM deliberately
    SensorConfig corruptConfig = {0, 0, 0, 0, 0};
    setSensorConfig(corruptConfig);

    loadConfigFromFlash();  // Restore from Flash

    SensorConfig retrieved = getSensorConfig();
    TEST_ASSERT_EQUAL_INT16(1500, retrieved.read_interval_ms);
    TEST_ASSERT_EQUAL_FLOAT(35.0, retrieved.max_temp_threshold);
}