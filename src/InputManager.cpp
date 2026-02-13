#include "InputManager.h"

// Global instance
InputManager RCInput;

InputManager::InputManager() {
    _initialized = false;
    _lastUpdateMs = 0;
    _updateIntervalMs = 10;  // Update every 10ms (100Hz)
}

bool InputManager::begin() {
    printf("=== RC Input Manager Initialization ===\r\n");
    
    bool switchOk = SwitchInput.begin();
    bool analogOk = AnalogInput.begin();
    bool encoderOk = EncoderInput.begin();
    
    _initialized = switchOk && analogOk && encoderOk;
    
    if (_initialized) {
        printf("RC Input Manager: All systems OK\r\n");
    } else {
        printf("RC Input Manager: Initialization FAILED\r\n");
        if (!switchOk) printf("  - Switch input (MCP23017) failed\r\n");
        if (!analogOk) printf("  - Analog input (ADS1X15) failed\r\n");
        if (!encoderOk) printf("  - Encoder input failed\r\n");
    }
    
    printf("=======================================\r\n");
    return _initialized;
}

void InputManager::update() {
    uint32_t now = millis();
    
    // Rate limit updates
    if ((now - _lastUpdateMs) < _updateIntervalMs) {
        return;
    }
    _lastUpdateMs = now;
    
    // Update switch inputs
    SwitchInput.update();
    
    // Update analog inputs
    AnalogInput.update();
    
    // Process encoder interrupts (also done in LVGL loop, but good to do here too)
    EncoderInput.processInterrupt();
}

bool InputManager::isReady() {
    return _initialized && SwitchInput.isReady() && AnalogInput.isReady() && EncoderInput.isReady();
}

void InputManager::printDebug() {
    printf("\r\n=== RC Inputs Debug ===\r\n");
    
    // Print gimbal values
    printf("Gimbals:\r\n");
    printf("  Left  X: %5d (raw: %5d)\r\n", getLeftX(), getGimbalRaw(GIMBAL_LEFT_X));
    printf("  Left  Y: %5d (raw: %5d)\r\n", getLeftY(), getGimbalRaw(GIMBAL_LEFT_Y));
    printf("  Right X: %5d (raw: %5d)\r\n", getRightX(), getGimbalRaw(GIMBAL_RIGHT_X));
    printf("  Right Y: %5d (raw: %5d)\r\n", getRightY(), getGimbalRaw(GIMBAL_RIGHT_Y));
    
    // Print potentiometer values
    printf("Potentiometers:\r\n");
    printf("  POT 1: %4d (raw: %5d)\r\n", getPot(POT_1), getPotRaw(POT_1));
    printf("  POT 2: %4d (raw: %5d)\r\n", getPot(POT_2), getPotRaw(POT_2));
    
    // Print encoder values
    printf("Encoders:\r\n");
    printf("  ENC 1: pos=%ld, dir=%d\r\n", getEncoderPosition(ENCODER_1), getEncoderDirection(ENCODER_1));
    printf("  ENC 2: pos=%ld, dir=%d\r\n", getEncoderPosition(ENCODER_2), getEncoderDirection(ENCODER_2));
    
    // Print 3-position toggles
    printf("3-Position Toggles:\r\n");
    for (uint8_t i = 0; i < NUM_3POS_TOGGLES; i++) {
        const char* posName;
        switch (getToggle3Pos(i)) {
            case TOGGLE_POS_UP: posName = "UP"; break;
            case TOGGLE_POS_CENTER: posName = "CENTER"; break;
            case TOGGLE_POS_DOWN: posName = "DOWN"; break;
            default: posName = "?"; break;
        }
        printf("  %s: %s\r\n", SwitchInput.getToggle3PosName(i), posName);
    }
    
    // Print switch states
    printf("Switches:\r\n");
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        printf("  %s: %s\r\n", 
            SwitchInput.getSwitchName(i), 
            isSwitchOn(i) ? "ON" : "OFF");
    }
    
    printf("=======================\r\n");
}

//=============================================================================
// Convenience Functions
//=============================================================================

void Input_Init() {
    RCInput.begin();
}

void Input_Update() {
    RCInput.update();
}
