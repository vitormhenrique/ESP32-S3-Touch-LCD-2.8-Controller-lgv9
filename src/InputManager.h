#pragma once
#include <Arduino.h>
#include "InputConfig.h"
#include "MCP23017_Driver.h"
#include "ADS1X15_Driver.h"

/******************************************************************************
 * RC Input Manager
 * 
 * Unified interface for all RC remote inputs:
 * - Switches/Buttons (via MCP23017)
 * - Navigation switches (via MCP23017)
 * - Encoders (via MCP23017)
 * - Gimbals (via ADS1X15)
 * - Potentiometers (via ADS1X15)
 ******************************************************************************/

// Gimbal axis indices for convenience
enum GimbalAxis {
    GIMBAL_1_X = 0,
    GIMBAL_1_Y = 1,
    GIMBAL_2_X = 2,
    GIMBAL_2_Y = 3
};

// Potentiometer indices
enum PotIndex {
    POT_1 = 0,
    POT_2 = 1
};

// Navigation switch indices
enum NavSwitchIndex {
    NAV_SWITCH_1 = 0,
    NAV_SWITCH_2 = 1
};

// Encoder indices
enum EncoderIndex {
    ENCODER_1 = 0,
    ENCODER_2 = 1
};

// Button indices
enum ButtonIndex {
    BUTTON_1 = 0,
    BUTTON_2 = 1
};

class InputManager {
public:
    InputManager();
    
    /**
     * Initialize all input hardware
     * @return true if all hardware initialized successfully
     */
    bool begin();
    
    /**
     * Update all inputs - call this every loop iteration
     * Handles reading switches, nav switches, encoders, gimbals, and pots
     */
    void update();
    
    //=========================================================================
    // Button/Switch Access
    //=========================================================================
    
    /**
     * Get switch state by index
     */
    SwitchState_t getSwitch(uint8_t index) { return SwitchInput.getSwitchState(index); }
    
    /**
     * Check if switch/button is ON
     */
    bool isSwitchOn(uint8_t index) { return SwitchInput.getSwitchState(index) == SWITCH_ON; }
    
    /**
     * Check if switch/button was just pressed
     */
    bool switchPressed(uint8_t index) { return SwitchInput.switchPressed(index); }
    
    /**
     * Check if switch/button was just released
     */
    bool switchReleased(uint8_t index) { return SwitchInput.switchReleased(index); }
    
    /**
     * Get button state (alias for switch)
     */
    bool isButtonPressed(uint8_t index) { return isSwitchOn(index); }
    
    /**
     * Check if button was just clicked (rising edge)
     */
    bool buttonClicked(uint8_t index) { return switchPressed(index); }
    
    //=========================================================================
    // Navigation Switch Access
    //=========================================================================
    
    /**
     * Check if a navigation switch direction is active
     * @param navIndex NAV_SWITCH_1 or NAV_SWITCH_2
     * @param direction NAV_UP, NAV_DOWN, NAV_LEFT, NAV_RIGHT, or NAV_CENTER
     */
    bool getNavDirection(uint8_t navIndex, NavDirection_t direction) {
        return SwitchInput.getNavDirection(navIndex, direction);
    }
    
    /**
     * Check if navigation direction was just pressed
     */
    bool navPressed(uint8_t navIndex, NavDirection_t direction) {
        return SwitchInput.navPressed(navIndex, direction);
    }
    
    /**
     * Check if navigation direction was just released
     */
    bool navReleased(uint8_t navIndex, NavDirection_t direction) {
        return SwitchInput.navReleased(navIndex, direction);
    }
    
    // Convenience methods for navigation switches
    bool isNavUp(uint8_t navIndex) { return getNavDirection(navIndex, NAV_UP); }
    bool isNavDown(uint8_t navIndex) { return getNavDirection(navIndex, NAV_DOWN); }
    bool isNavLeft(uint8_t navIndex) { return getNavDirection(navIndex, NAV_LEFT); }
    bool isNavRight(uint8_t navIndex) { return getNavDirection(navIndex, NAV_RIGHT); }
    bool isNavCenter(uint8_t navIndex) { return getNavDirection(navIndex, NAV_CENTER); }
    
    bool navUpPressed(uint8_t navIndex) { return navPressed(navIndex, NAV_UP); }
    bool navDownPressed(uint8_t navIndex) { return navPressed(navIndex, NAV_DOWN); }
    bool navLeftPressed(uint8_t navIndex) { return navPressed(navIndex, NAV_LEFT); }
    bool navRightPressed(uint8_t navIndex) { return navPressed(navIndex, NAV_RIGHT); }
    bool navCenterPressed(uint8_t navIndex) { return navPressed(navIndex, NAV_CENTER); }
    
    //=========================================================================
    // Encoder Access
    //=========================================================================
    
    /**
     * Get encoder position
     */
    int32_t getEncoderPosition(uint8_t encIndex) {
        return SwitchInput.getEncoderPosition(encIndex);
    }
    
    /**
     * Get encoder delta (change since last update)
     */
    int32_t getEncoderDelta(uint8_t encIndex) {
        return SwitchInput.getEncoderDelta(encIndex);
    }
    
    /**
     * Check if encoder changed
     */
    bool encoderChanged(uint8_t encIndex) {
        return SwitchInput.encoderChanged(encIndex);
    }
    
    /**
     * Reset encoder position to zero
     */
    void resetEncoder(uint8_t encIndex) {
        SwitchInput.resetEncoder(encIndex);
    }
    
    /**
     * Set encoder position
     */
    void setEncoderPosition(uint8_t encIndex, int32_t position) {
        SwitchInput.setEncoderPosition(encIndex, position);
    }
    
    //=========================================================================
    // Gimbal Access
    //=========================================================================
    
    /**
     * Get gimbal axis value (-1000 to +1000)
     */
    int16_t getGimbal(uint8_t axis) { return AnalogInput.getGimbalAxis(axis); }
    
    /**
     * Get gimbal 1 X axis
     */
    int16_t getGimbal1X() { return AnalogInput.getGimbalAxis(GIMBAL_1_X); }
    
    /**
     * Get gimbal 1 Y axis
     */
    int16_t getGimbal1Y() { return AnalogInput.getGimbalAxis(GIMBAL_1_Y); }
    
    /**
     * Get gimbal 2 X axis
     */
    int16_t getGimbal2X() { return AnalogInput.getGimbalAxis(GIMBAL_2_X); }
    
    /**
     * Get gimbal 2 Y axis
     */
    int16_t getGimbal2Y() { return AnalogInput.getGimbalAxis(GIMBAL_2_Y); }
    
    // Legacy aliases
    int16_t getLeftX() { return getGimbal1X(); }
    int16_t getLeftY() { return getGimbal1Y(); }
    int16_t getRightX() { return getGimbal2X(); }
    int16_t getRightY() { return getGimbal2Y(); }
    
    /**
     * Get raw gimbal value (for calibration)
     */
    int16_t getGimbalRaw(uint8_t axis) { return AnalogInput.getGimbalAxisRaw(axis); }
    
    //=========================================================================
    // Potentiometer Access
    //=========================================================================
    
    /**
     * Get potentiometer value (0 to 1000)
     */
    int16_t getPot(uint8_t index) { return AnalogInput.getPotentiometer(index); }
    
    /**
     * Get raw pot value (for calibration)
     */
    int16_t getPotRaw(uint8_t index) { return AnalogInput.getPotentiometerRaw(index); }
    
    //=========================================================================
    // Calibration
    //=========================================================================
    
    /**
     * Calibrate a gimbal axis
     */
    void calibrateGimbal(uint8_t axis, int16_t min_val, int16_t center_val, int16_t max_val) {
        AnalogInput.calibrateGimbalAxis(axis, min_val, center_val, max_val);
    }
    
    /**
     * Calibrate a potentiometer
     */
    void calibratePot(uint8_t index, int16_t min_val, int16_t max_val) {
        AnalogInput.calibratePotentiometer(index, min_val, max_val);
    }
    
    //=========================================================================
    // 3-Position Toggle Access (legacy compatibility)
    //=========================================================================
    
    /**
     * Get 3-position toggle state (legacy - returns CENTER always)
     */
    Toggle3PosState_t getToggle3Pos(uint8_t index) { return SwitchInput.getToggle3PosState(index); }
    
    bool isToggleUp(uint8_t index) { return SwitchInput.getToggle3PosState(index) == TOGGLE_POS_UP; }
    bool isToggleCenter(uint8_t index) { return SwitchInput.getToggle3PosState(index) == TOGGLE_POS_CENTER; }
    bool isToggleDown(uint8_t index) { return SwitchInput.getToggle3PosState(index) == TOGGLE_POS_DOWN; }
    bool toggleChanged(uint8_t index) { return SwitchInput.toggle3PosChanged(index); }
    
    //=========================================================================
    // Status
    //=========================================================================
    
    /**
     * Check if all input hardware is ready
     */
    bool isReady();
    
    /**
     * Print all input values to serial (for debugging)
     */
    void printDebug();
    
    /**
     * Get update rate (calls per second)
     */
    uint32_t getUpdateRate() { return _updateRate; }

private:
    bool _initialized;
    uint32_t _lastUpdateMs;
    uint32_t _updateIntervalMs;
    
    // Performance tracking
    uint32_t _updateCount;
    uint32_t _lastRateCalcMs;
    uint32_t _updateRate;
};

// Global instance
extern InputManager RCInput;

//=============================================================================
// Convenience Functions
//=============================================================================

/**
 * Initialize all RC inputs
 */
void Input_Init();

/**
 * Update all RC inputs
 */
void Input_Update();
