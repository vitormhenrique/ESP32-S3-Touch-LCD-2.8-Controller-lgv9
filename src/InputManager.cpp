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
    
    _initialized = switchOk && analogOk;
    
    if (_initialized) {
        printf("RC Input Manager: All systems OK\r\n");
    } else {
        printf("RC Input Manager: Initialization FAILED\r\n");
        if (!switchOk) printf("  - Switch input (MCP23017) failed\r\n");
        if (!analogOk) printf("  - Analog input (ADS1X15) failed\r\n");
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
    
    // Track update rate
    _updateCount++;
    if ((now - _lastRateCalcMs) >= 1000) {
        _updateRate = _updateCount;
        _updateCount = 0;
        _lastRateCalcMs = now;
    }
}

bool InputManager::isReady() {
    return _initialized && SwitchInput.isReady() && AnalogInput.isReady();
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
    
    // Print navigation switches
    printf("Navigation Switches:\r\n");
    for (uint8_t i = 0; i < NUM_NAV_SWITCHES; i++) {
        printf("  NAV_%d: U:%d D:%d L:%d R:%d C:%d\r\n", 
            i + 1,
            isNavUp(i) ? 1 : 0,
            isNavDown(i) ? 1 : 0,
            isNavLeft(i) ? 1 : 0,
            isNavRight(i) ? 1 : 0,
            isNavCenter(i) ? 1 : 0);
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
