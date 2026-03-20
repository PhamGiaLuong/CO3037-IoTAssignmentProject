#include "hardware_manager.h"

void buzzerTask(void* pvParameters) {
    TickType_t lastWakeTime = xTaskGetTickCount();
    TickType_t period = pdMS_TO_TICKS(100);
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    while (1) {
        ControlState state = getControlState();
        digitalWrite(BUZZER_PIN, state.is_device1_on ? HIGH : LOW);
        vTaskDelayUntil(&lastWakeTime, period);
    }
}