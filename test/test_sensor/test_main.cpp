#include <Arduino.h>
#include <unity.h>

#include "global.h"

// Unit Tests declarations
extern void test_set_and_clear_sensor_error_flag(void);
extern void test_sensor_config_read_write(void);
extern void test_ml_state_encapsulation(void);

// Integration Tests declarations
extern void test_error_triggers_led_semaphore(void);
extern void test_error_triggers_lcd_semaphore(void);
extern void test_save_load_config_flash_nvs(void);

void setUp(void) {
    initGlobal();  // Initialize mutexes and semaphores
}

void tearDown(void) {}

void setup() {
    delay(2000);  // Wait for Serial to stabilize
    UNITY_BEGIN();

    // Unit Test
    RUN_TEST(test_set_and_clear_sensor_error_flag);
    RUN_TEST(test_sensor_config_read_write);
    RUN_TEST(test_ml_state_encapsulation);

    // Integration Test
    RUN_TEST(test_error_triggers_led_semaphore);
    RUN_TEST(test_error_triggers_lcd_semaphore);
    RUN_TEST(test_save_load_config_flash_nvs);

    UNITY_END();
}

void loop() {}