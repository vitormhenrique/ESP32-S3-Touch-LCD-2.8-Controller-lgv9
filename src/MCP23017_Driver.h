#pragma once
#include <Arduino.h>
#include <Adafruit_MCP23X17.h>
#include "InputConfig.h"

/******************************************************************************
 * MCP23017 GPIO Expander Driver
 * 
 * Handles two MCP23017 expanders for reading switch inputs including
 * standard 2-position switches, momentary buttons, and 3-position toggles.
 ******************************************************************************/

class MCP23017_Driver {
public:
    MCP23017_Driver();
    
    /**
     * Initialize both MCP23017 expanders
     * @return true if both initialized successfully
     */
    bool begin();
    
    /**
     * Read all switch inputs and update internal state
     * Call this periodically (e.g., every 10-20ms)
     */
    void update();
    
    /**
     * Get current state of a standard switch
     * @param index Switch index (0 to NUM_SWITCHES-1)
     * @return Current switch state
     */
    SwitchState_t getSwitchState(uint8_t index);
    
    /**
     * Check if switch was just pressed (rising edge)
     * @param index Switch index
     * @return true if switch was just activated this update cycle
     */
    bool switchPressed(uint8_t index);
    
    /**
     * Check if switch was just released (falling edge)
     * @param index Switch index
     * @return true if switch was just deactivated this update cycle
     */
    bool switchReleased(uint8_t index);
    
    /**
     * Get current state of a 3-position toggle
     * @param index Toggle index (0 to NUM_3POS_TOGGLES-1)
     * @return Current toggle position
     */
    Toggle3PosState_t getToggle3PosState(uint8_t index);
    
    /**
     * Check if 3-position toggle changed position
     * @param index Toggle index
     * @return true if position changed this update cycle
     */
    bool toggle3PosChanged(uint8_t index);
    
    /**
     * Get switch name by index
     * @param index Switch index
     * @return Switch name string
     */
    const char* getSwitchName(uint8_t index);
    
    /**
     * Get 3-position toggle name by index
     * @param index Toggle index
     * @return Toggle name string
     */
    const char* getToggle3PosName(uint8_t index);
    
    /**
     * Read raw pin state from expander (for debugging)
     * @param expander Expander index (0 or 1)
     * @param pin Pin number (0-15)
     * @return Pin state (HIGH or LOW)
     */
    bool readPin(uint8_t expander, uint8_t pin);
    
    /**
     * Check if expanders are initialized and responding
     * @return true if both expanders are working
     */
    bool isReady();

private:
    Adafruit_MCP23X17 _mcp[2];          // Two MCP23017 expanders
    bool _initialized[2];                // Initialization status
    
    // Switch configurations and states
    SwitchConfig_t _switchConfigs[NUM_SWITCHES];
    SwitchState_Runtime_t _switchStates[NUM_SWITCHES];
    
    // 3-position toggle configurations and states
    Toggle3PosConfig_t _toggle3PosConfigs[NUM_3POS_TOGGLES];
    Toggle3PosState_Runtime_t _toggle3PosStates[NUM_3POS_TOGGLES];
    
    uint32_t _lastUpdateMs;
    
    /**
     * Initialize switch configurations from defines
     */
    void initConfigs();
    
    /**
     * Read and debounce a single switch
     */
    void updateSwitch(uint8_t index);
    
    /**
     * Read and debounce a 3-position toggle
     */
    void updateToggle3Pos(uint8_t index);
};

// Global instance
extern MCP23017_Driver SwitchInput;
