#include <Arduino.h>
#include <unity.h>

#include "esp_now_manager.h"
#include "global.h"
extern void testHook_setNodeLastSeenMillis(const char* mac_str,
                                           uint32_t simulated_millis);
extern void testHook_macStrToBytes(const char* mac_str, uint8_t* mac_bytes);
extern void testHook_bytesToMacStr(const uint8_t* mac_bytes, char* mac_str);
extern uint8_t testHook_mapPredictionToId(const char* pred);
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

// TC: US-04
void test_sensor_data_read_write(void) {
    SensorData mock_data = {24.7f, 62.3f, true, false};
    setSensorData(mock_data);

    SensorData read_data = getSensorData();
    TEST_ASSERT_EQUAL_FLOAT(24.7f, read_data.current_temperature);
    TEST_ASSERT_EQUAL_FLOAT(62.3f, read_data.current_humidity);
    TEST_ASSERT_TRUE(read_data.is_dht20_ok);
    TEST_ASSERT_FALSE(read_data.is_lcd_ok);
}

// TC: US-05
void test_multiple_error_flags_logic(void) {
    clearSensorErrorFlag(0xFFFF);  // Clean state

    setSensorErrorFlag(SENSOR_FLAG_DHT_ERR);
    setSensorErrorFlag(SENSOR_FLAG_LCD_ERR);

    uint32_t combined_flags = getSensorActiveErrorFlags();
    TEST_ASSERT_EQUAL_UINT32((SENSOR_FLAG_DHT_ERR | SENSOR_FLAG_LCD_ERR),
                             combined_flags);
}

// TC: US-06
void test_multiple_error_flags_logic_2(void) {
    clearSensorErrorFlag(0xFFFF);  // Clean state

    setSensorErrorFlag(SENSOR_FLAG_DHT_ERR);
    setSensorErrorFlag(SENSOR_FLAG_LCD_ERR);

    TEST_ASSERT_TRUE(checkSensorErrorFlag(SENSOR_FLAG_DHT_ERR));
    TEST_ASSERT_TRUE(checkSensorErrorFlag(SENSOR_FLAG_LCD_ERR));
    TEST_ASSERT_FALSE(checkSensorErrorFlag(SENSOR_FLAG_TEMP_HIGH));
}

// TC: US-07
void test_clear_non_existent_flag(void) {
    clearSensorErrorFlag(0xFFFF);
    setSensorErrorFlag(SENSOR_FLAG_TEMP_WARN);

    clearSensorErrorFlag(SENSOR_FLAG_HUM_WARN);

    uint32_t current_flags = getSensorActiveErrorFlags();
    TEST_ASSERT_EQUAL_UINT32(SENSOR_FLAG_TEMP_WARN, current_flags);
}

// TC: US-08
void test_sensor_config_boundary_values(void) {
    SensorConfig extreme_cfg = {0, 100.0f, -25.5f, 100.0f, 0.0f};
    setSensorConfig(extreme_cfg);

    SensorConfig read_cfg = getSensorConfig();
    TEST_ASSERT_EQUAL_INT16(0, read_cfg.read_interval_ms);
    TEST_ASSERT_EQUAL_FLOAT(-25.5f, read_cfg.min_temp_threshold);
    TEST_ASSERT_EQUAL_FLOAT(100.0f, read_cfg.max_humidity_threshold);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, read_cfg.min_humidity_threshold);
}

// TC: US-09
void test_lcd_i2c_connection(void) {
    uint8_t error, address;
    bool lcd_found = false;

    // Quét các địa chỉ I2C phổ biến của LCD
    for (address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();

        if (error == 0) {
            if (address == 0x27 || address == 0x3F) {
                lcd_found = true;
                break;
            }
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(
        lcd_found,
        "CRITICAL: Không tìm thấy LCD trên bus I2C! Hãy kiểm tra lại dây cắm.");
}

// TC: US-10
void test_mac_str_to_bytes_conversion(void) {
    uint8_t mac[6];
    testHook_macStrToBytes("AA:BB:CC:11:22:33", mac);
    uint8_t exp[6] = {0xAA, 0xBB, 0xCC, 0x11, 0x22, 0x33};
    TEST_ASSERT_EQUAL_UINT8_ARRAY(exp, mac, 6);
}

// TC: UG-11
void test_bytes_to_mac_str_conversion(void) {
    uint8_t mac[6] = {0xFF, 0xEE, 0xDD, 0x11, 0x22, 0x33};
    char str[18];
    testHook_bytesToMacStr(mac, str);
    TEST_ASSERT_EQUAL_STRING("FF:EE:DD:11:22:33", str);
}

// TC: UG-12
void test_map_prediction_to_id(void) {
    TEST_ASSERT_EQUAL_UINT8(0, testHook_mapPredictionToId("Normal"));
    TEST_ASSERT_EQUAL_UINT8(1, testHook_mapPredictionToId("Door Open"));
    TEST_ASSERT_EQUAL_UINT8(2, testHook_mapPredictionToId("Fault"));
    TEST_ASSERT_EQUAL_UINT8(0, testHook_mapPredictionToId("Unknown State"));
}

// TC: UG-13
void test_add_node_exceeds_max_limit(void) {
    for (int i = 0; i < 10; i++) {  // Max limit 10
        char mac[20];
        sprintf(mac, "AA:BB:CC:11:22:%02X", i);
        addPairedNode(mac, "R");
    }
    TEST_ASSERT_FALSE(addPairedNode("AA:BB:CC:11:22:FF", "RX"));
    TEST_ASSERT_EQUAL(10, getActiveNodeCount());
}

// TC: UG-14
void test_remove_node_not_found(void) {
    TEST_ASSERT_FALSE(removePairedNode("00:11:22:33:44:55"));
}

// TC: UG-15
void test_update_node_config_not_found(void) {
    SensorConfig cfg;
    TEST_ASSERT_FALSE(updateNodeConfig("00:11:22:33:44:55", cfg));
}

// TC: UG-16
void test_update_node_data_ml_not_found(void) {
    SensorData d;
    MlState m;
    TEST_ASSERT_FALSE(updateNodeDataAndMl("00:11:22:33", d, m));
}

// TC: UG-17
