#include "hardware_manager.h"

void initHardware() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
}

void buzzerControl(bool turn_on) {
    ControlState state = getControlState();
    state.is_device1_on = turn_on;
    setControlState(state);
    digitalWrite(BUZZER_PIN, turn_on ? HIGH : LOW);
}