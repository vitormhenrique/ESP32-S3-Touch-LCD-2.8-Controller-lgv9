#include "InputManager.h"

// Global instance
InputManager RCInput;

InputManager::InputManager() {
    _initialized = false;
    _lastUpdateMs = 0;
    _updateIntervalMs = 10;  // Update every 10ms (100Hz)
    _updateCount = 0;
    _lastRateCalcMs = 0;
    _updateRate = 0;
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
    
    // Update switch inputs (including nav switches and encoders)
    SwitchInput.update();
    
    // Update analog inputs
    AnalogInput.update();
    
    // Encoder polling handled by dedicated EncoderTask
}

bool InputManager::isReady() {
    return _initialized && SwitchInput.isReady() && AnalogInput.isReady() && EncoderInput.isReady();
}

void InputManager::printDebug() {
    printf("\r\n=== RC Inputs Debug ===\r\n");
    
    // Print gimbal values
    printf("Gimbals:\r\n");
    printf("  Gimbal 1 X: %5d (raw: %5d)\r\n", getGimbal1X(), getGimbalRaw(GIMBAL_1_X));
    printf("  Gimbal 1 Y: %5d (raw: %5d)\r\n", getGimbal1Y(), getGimbalRaw(GIMBAL_1_Y));
    printf("  Gimbal 2 X: %5d (raw: %5d)\r\n", getGimbal2X(), getGimbalRaw(GIMBAL_2_X));
    printf("  Gimbal 2 Y: %5d (raw: %5d)\r\n", getGimbal2Y(), getGimbalRaw(GIMBAL_2_Y));
    
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
    
    // Print encoders
    printf("Encoders:\r\n");
    for (uint8_t i = 0; i < NUM_ENCODERS; i++) {
        printf("  ENC_%d: %ld\r\n", i + 1, (long)getEncoderPosition(i));
    }
    
    // Print buttons
    printf("Buttons:\r\n");
    for (uint8_t i = 0; i < NUM_SWITCHES; i++) {
        printf("  %s: %s\r\n", 
            SwitchInput.getSwitchName(i), 
            isSwitchOn(i) ? "ON" : "OFF");
    }
    
    printf("Update Rate: %lu Hz\r\n", _updateRate);
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
