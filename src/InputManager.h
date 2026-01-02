#pragma once
#include <Arduino.h>
#include "InputConfig.h"
#include "MCP23017_Driver.h"
#include "ADS1X15_Driver.h"

/******************************************************************************
 * RC Input Manager
 * 
 * Unified interface for all RC remote inputs:
 * - Switches (via MCP23017)
 * - 3-position toggles (via MCP23017)
 * - Gimbals (via ADS1X15)
 * - Potentiometers (via ADS1X15)
 ******************************************************************************/

// Gimbal axis indices for convenience
enum GimbalAxis {
    GIMBAL_LEFT_X = 0,
    GIMBAL_LEFT_Y = 1,
    GIMBAL_RIGHT_X = 2,
    GIMBAL_RIGHT_Y = 3
};

// Potentiometer indices
enum PotIndex {
    POT_1 = 0,
    POT_2 = 1
};

// 3-position toggle indices
enum Toggle3PosIndex {
    TOGGLE_3POS_1 = 0,
    TOGGLE_3POS_2 = 1
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
     * Handles reading switches, toggles, gimbals, and pots
     */
    void update();
    
    //=========================================================================
    // Switch Access
    //=========================================================================
    
    /**
     * Get switch state by index
     */
    SwitchState_t getSwitch(uint8_t index) { return SwitchInput.getSwitchState(index); }
    
    /**
     * Check if switch is ON
     */
    bool isSwitchOn(uint8_t index) { return SwitchInput.getSwitchState(index) == SWITCH_ON; }
    
    /**
     * Check if switch was just pressed
     */
    bool switchPressed(uint8_t index) { return SwitchInput.switchPressed(index); }
    
    /**
     * Check if switch was just released
     */
    bool switchReleased(uint8_t index) { return SwitchInput.switchReleased(index); }
    
    //=========================================================================
    // 3-Position Toggle Access
    //=========================================================================
    
    /**
     * Get 3-position toggle state
     */
    Toggle3PosState_t getToggle3Pos(uint8_t index) { return SwitchInput.getToggle3PosState(index); }
    
    /**
     * Check if toggle is in UP position
     */
    bool isToggleUp(uint8_t index) { return SwitchInput.getToggle3PosState(index) == TOGGLE_POS_UP; }
    
    /**
     * Check if toggle is in CENTER position
     */
    bool isToggleCenter(uint8_t index) { return SwitchInput.getToggle3PosState(index) == TOGGLE_POS_CENTER; }
    
    /**
     * Check if toggle is in DOWN position
     */
    bool isToggleDown(uint8_t index) { return SwitchInput.getToggle3PosState(index) == TOGGLE_POS_DOWN; }
    
    /**
     * Check if toggle position changed
     */
    bool toggleChanged(uint8_t index) { return SwitchInput.toggle3PosChanged(index); }
    
    //=========================================================================
    // Gimbal Access
    //=========================================================================
    
    /**
     * Get gimbal axis value (-1000 to +1000)
     */
    int16_t getGimbal(uint8_t axis) { return AnalogInput.getGimbalAxis(axis); }
    
    /**
     * Get left stick X axis
     */
    int16_t getLeftX() { return AnalogInput.getGimbalAxis(GIMBAL_LEFT_X); }
    
    /**
     * Get left stick Y axis
     */
    int16_t getLeftY() { return AnalogInput.getGimbalAxis(GIMBAL_LEFT_Y); }
    
    /**
     * Get right stick X axis
     */
    int16_t getRightX() { return AnalogInput.getGimbalAxis(GIMBAL_RIGHT_X); }
    
    /**
     * Get right stick Y axis
     */
    int16_t getRightY() { return AnalogInput.getGimbalAxis(GIMBAL_RIGHT_Y); }
    
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

private:
    bool _initialized;
    uint32_t _lastUpdateMs;
    uint32_t _updateIntervalMs;
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
 * Update all RC inputs (call in loop)
 */
void Input_Update();
