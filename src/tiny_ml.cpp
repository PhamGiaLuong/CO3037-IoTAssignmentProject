#include "tiny_ml.h"

// MACROS & CONSTANTS
#define TENSOR_ARENA_SIZE (8 * 1024)       // 8KB arena size
#define TINYML_INFERENCE_INTERVAL_MS 2000  // Run inference every 2 seconds
#define DOOR_OPEN_DEBOUNCE_LIMIT 10  // 10 cycles * 2s = 20 seconds tolerance

// ML Output Classes
#define CLASS_NORMAL 0
#define CLASS_DOOR_OPEN 1
#define CLASS_FAULT 2

// TFLITE GLOBALS
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* tflite_model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* ml_input = nullptr;
TfLiteTensor* ml_output = nullptr;

uint8_t tensor_arena[TENSOR_ARENA_SIZE];
}  // namespace

// INTERNAL FUNCTIONS
static bool setupTinyMl() {
    LOG_INFO("TINYML", "Initializing TensorFlow Lite Micro...");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    tflite_model = tflite::GetModel(model);
    if (tflite_model->version() != TFLITE_SCHEMA_VERSION) {
        LOG_ERR("TINYML",
                "Model schema version %d not equal to supported version %d.",
                tflite_model->version(), TFLITE_SCHEMA_VERSION);
        return false;
    }

    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(
        tflite_model, resolver, tensor_arena, TENSOR_ARENA_SIZE,
        error_reporter);
    interpreter = &static_interpreter;

    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        LOG_ERR("TINYML", "AllocateTensors() failed");
        return false;
    }

    ml_input = interpreter->input(0);
    ml_output = interpreter->output(0);

    LOG_INFO("TINYML",
             "Initialization successful. Input shape: %d, Output shape: %d",
             ml_input->dims->data[1], ml_output->dims->data[1]);

    return true;
}

void evaluateModelAccuracy() {
    if (interpreter == nullptr || ml_input == nullptr || ml_output == nullptr) {
        LOG_ERR("TINYML_TEST",
                "Interpreter not initialized. Cannot run tests.");
        return;
    }

    LOG_INFO("TINYML_TEST", "--- STARTING HARDWARE ACCURACY EVALUATION ---");

    // Test: {t_norm, h_norm, t_speed, h_speed, Expected_Class}
    float test_set[5][5] = {
        {0.5, 0.5, 0.0, 0.0, CLASS_NORMAL},  // Test 1: Normal
        {0.9, 0.9, 0.1, 0.1, CLASS_NORMAL},  // Test 2: Normal near threshold
        {1.5, 1.2, 2.5, 15.0, CLASS_DOOR_OPEN},  // Test 3: Open door
        {1.3, 0.8, 0.1, 0.0, CLASS_FAULT},       // Test 4: Faulty sensor
        {0.2, 1.5, 0.0, 0.5, CLASS_FAULT}        // Test 5: Faulty sensor
    };

    int correct_predictions = 0;
    const int num_tests = 5;

    for (int i = 0; i < num_tests; i++) {
        ml_input->data.f[0] = test_set[i][0];
        ml_input->data.f[1] = test_set[i][1];
        ml_input->data.f[2] = test_set[i][2];
        ml_input->data.f[3] = test_set[i][3];

        uint32_t start_time = micros();
        TfLiteStatus invoke_status = interpreter->Invoke();
        uint32_t end_time = micros();

        if (invoke_status != kTfLiteOk) {
            LOG_ERR("TINYML_TEST", "Test %d failed to invoke!", i + 1);
            continue;
        }

        float p0 = ml_output->data.f[CLASS_NORMAL];
        float p1 = ml_output->data.f[CLASS_DOOR_OPEN];
        float p2 = ml_output->data.f[CLASS_FAULT];

        int predicted_class = CLASS_NORMAL;
        float max_p = p0;
        if (p1 > max_p) {
            predicted_class = CLASS_DOOR_OPEN;
            max_p = p1;
        }
        if (p2 > max_p) {
            predicted_class = CLASS_FAULT;
            max_p = p2;
        }

        int expected_class = (int)test_set[i][4];
        if (predicted_class == expected_class) {
            correct_predictions++;
        }

        LOG_INFO("TINYML_TEST",
                 "Test %d | Exp: %d | Pred: %d | Conf: %.2f | Time: %lu us",
                 i + 1, expected_class, predicted_class, max_p,
                 (unsigned long)(end_time - start_time));
    }

    float accuracy = (float)correct_predictions / num_tests * 100.0f;
    size_t used_ram = interpreter->arena_used_bytes();

    LOG_INFO("TINYML_TEST", "--- EVALUATION SUMMARY ---");
    LOG_INFO("TINYML_TEST", "Hardware Accuracy: %d/%d (%.1f%%)",
             correct_predictions, num_tests, accuracy);
    LOG_INFO("TINYML_TEST", "RAM Used (Arena): %u bytes / %d bytes", used_ram,
             TENSOR_ARENA_SIZE);
    LOG_INFO("TINYML_TEST", "---------------------------------------------");
}

void tinyMlTask(void* pvParameters) {
    if (!setupTinyMl()) {
        LOG_ERR("TINYML", "Failed to start TinyML task due to init error.");
        vTaskDelete(NULL);
    }

    float prev_temp = 0.0f;
    float prev_hum = 0.0f;
    uint8_t door_open_counter = 0;

    SensorData init_data = getSensorData();
    prev_temp = init_data.current_temperature;
    prev_hum = init_data.current_humidity;

    while (1) {
        SensorData current_data = getSensorData();
        SensorConfig current_config = getSensorConfig();

        float temp_range = current_config.max_temp_threshold -
                           current_config.min_temp_threshold;
        float hum_range = current_config.max_humidity_threshold -
                          current_config.min_humidity_threshold;

        if (temp_range == 0.0f) temp_range = 1.0f;
        if (hum_range == 0.0f) hum_range = 1.0f;

        // Feature Engineering
        // Feature 1 & 2: Normalized Values (0.0 to 1.0 means inside thresholds)
        float t_norm = (current_data.current_temperature -
                        current_config.min_temp_threshold) /
                       temp_range;
        float h_norm = (current_data.current_humidity -
                        current_config.min_humidity_threshold) /
                       hum_range;

        // Feature 3 & 4: Rate of Change (Speed)
        float t_speed = current_data.current_temperature - prev_temp;
        float h_speed = current_data.current_humidity - prev_hum;

        prev_temp = current_data.current_temperature;
        prev_hum = current_data.current_humidity;

        // Feed data to input tensor
        ml_input->data.f[0] = t_norm;
        ml_input->data.f[1] = h_norm;
        ml_input->data.f[2] = t_speed;
        ml_input->data.f[3] = h_speed;

        // Run Inference
        uint32_t start_time = micros();
        TfLiteStatus invoke_status = interpreter->Invoke();
        uint32_t end_time = micros();
        if (invoke_status != kTfLiteOk) {
            LOG_WARN("TINYML", "Invoke failed");
            vTaskDelay(pdMS_TO_TICKS(TINYML_INFERENCE_INTERVAL_MS));
            continue;
        }
        // Calculate inference time and memory usage for monitoring
        uint32_t inference_time_us = end_time - start_time;
        size_t used_ram_bytes = interpreter->arena_used_bytes();
        // LOG_INFO("TINYML_METRICS", "Inference: %u us | RAM used: %d bytes",
        //          inference_time_us, used_ram_bytes);

        // Process Output
        float prob_normal = ml_output->data.f[CLASS_NORMAL];
        float prob_door = ml_output->data.f[CLASS_DOOR_OPEN];
        float prob_fault = ml_output->data.f[CLASS_FAULT];

        // Find the class with the highest probability (Argmax)
        uint8_t predicted_class = CLASS_NORMAL;
        float max_confidence = prob_normal;

        if (prob_door > max_confidence) {
            predicted_class = CLASS_DOOR_OPEN;
            max_confidence = prob_door;
        }
        if (prob_fault > max_confidence) {
            predicted_class = CLASS_FAULT;
            max_confidence = prob_fault;
        }

        // Action Logic & Debounce Management
        MlState new_ml_state;
        new_ml_state.confidence = max_confidence;

        switch (predicted_class) {
            case CLASS_NORMAL:
                strlcpy(new_ml_state.prediction, "Normal",
                        sizeof(new_ml_state.prediction));
                door_open_counter = 0;                        // Reset counter
                clearSensorErrorFlag(SENSOR_FLAG_TEMP_WARN);  // System is fine
                break;

            case CLASS_DOOR_OPEN:
                door_open_counter++;
                // Debounce logic: Only trigger alarm if door is open too long
                if (door_open_counter > DOOR_OPEN_DEBOUNCE_LIMIT) {
                    strlcpy(new_ml_state.prediction, "Door Left Open!",
                            sizeof(new_ml_state.prediction));
                    setSensorErrorFlag(
                        SENSOR_FLAG_TEMP_WARN);  // Trigger LED/Buzzer via
                                                 // Semaphore
                    LOG_WARN("TINYML", "Door open timeout! Alert triggered.");
                } else {
                    strlcpy(new_ml_state.prediction, "Door Open",
                            sizeof(new_ml_state.prediction));
                    // Do not set error flag yet (Debouncing in progress)
                }
                break;

            case CLASS_FAULT:
                strlcpy(new_ml_state.prediction, "System Fault",
                        sizeof(new_ml_state.prediction));
                door_open_counter = 0;
                setSensorErrorFlag(SENSOR_FLAG_TEMP_WARN);  // Immediate alarm
                LOG_ERR("TINYML", "System fault detected by AI!");
                break;
        }

        setMlState(new_ml_state);

        LOG_INFO("TINYML",
                 "Pred: %s | Conf: %.2f%% | Norms: (%.2f, %.2f) | Speeds : (% "
                 ".2f, % .2f) ",
                 new_ml_state.prediction, max_confidence * 100.0f, t_norm,
                 h_norm, t_speed, h_speed);

        vTaskDelay(pdMS_TO_TICKS(TINYML_INFERENCE_INTERVAL_MS));
    }
}