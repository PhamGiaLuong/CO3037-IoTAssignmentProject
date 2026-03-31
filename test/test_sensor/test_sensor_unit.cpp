#include <Arduino.h>
#include <unity.h>

#include "global.h"

// TC: US-01
void test_set_and_clear_sensor_error_flag(void) {
    // Clear all flags first to ensure clean state
    clearSensorErrorFlag(0xFFFF);

    setSensorErrorFlag(SENSOR_FLAG_DHT_ERR);
    TEST_ASSERT_TRUE(checkSensorErrorFlag(SENSOR_FLAG_DHT_ERR));

    clearSensorErrorFlag(SENSOR_FLAG_DHT_ERR);
    TEST_ASSERT_FALSE(checkSensorErrorFlag(SENSOR_FLAG_DHT_ERR));
}

// TC: US-02
void test_sensor_config_read_write(void) {
    SensorConfig mockConfig = {1000, 30.0, 10.0, 80.0, 40.0};
    setSensorConfig(mockConfig);

    SensorConfig retrieved = getSensorConfig();
    TEST_ASSERT_EQUAL_INT16(1000, retrieved.read_interval_ms);
    TEST_ASSERT_EQUAL_FLOAT(30.0, retrieved.max_temp_threshold);
    TEST_ASSERT_EQUAL_FLOAT(10.0, retrieved.min_temp_threshold);
    TEST_ASSERT_EQUAL_FLOAT(80.0, retrieved.max_humidity_threshold);
    TEST_ASSERT_EQUAL_FLOAT(40.0, retrieved.min_humidity_threshold);
}

// TC: US-03
void test_ml_state_encapsulation(void) {
    MlState mockState = {"Door Open", 0.95};
    setMlState(mockState);

    MlState retrieved = getMlState();
    TEST_ASSERT_EQUAL_STRING("Door Open", retrieved.prediction);
    TEST_ASSERT_EQUAL_FLOAT(0.95, retrieved.confidence);
}