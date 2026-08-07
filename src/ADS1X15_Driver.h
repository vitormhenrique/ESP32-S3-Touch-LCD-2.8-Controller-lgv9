#pragma once
#include <Arduino.h>
#include <Adafruit_ADS1X15.h>
#include "InputConfig.h"

/******************************************************************************
 * ADS1X15 ADC Driver
 * 
 * Handles three ADS1X15 ADCs for reading analog inputs:
 * - 2 Gimbals (4 axes total)
 * - 2 Potentiometers
 ******************************************************************************/

class ADS1X15_Driver {
public:
    ADS1X15_Driver();
    
    /**
     * Initialize all ADS1X15 ADCs
     * @return true if all initialized successfully
     */
    bool begin();
    
    /**
     * Read all analog inputs and update internal state
     * Call this periodically (e.g., every 10-20ms)
     */
    void update();
    
    /**
     * Get calibrated gimbal axis value
     * @param index Axis index (0-3: LEFT_X, LEFT_Y, RIGHT_X, RIGHT_Y)
     * @return Calibrated value (-1000 to +1000), 0 at center
     */
    int16_t getGimbalAxis(uint8_t index);
    
    /**
     * Get raw gimbal axis value
     * @param index Axis index (0-3)
     * @return Raw ADC value (0-32767 for 16-bit mode)
     */
    int16_t getGimbalAxisRaw(uint8_t index);
    
    /**
     * Get calibrated potentiometer value
     * @param index Pot index (0-1)
     * @return Calibrated value (0 to 1000)
     */
    int16_t getPotentiometer(uint8_t index);
    
    /**
     * Get raw potentiometer value
     * @param index Pot index (0-1)
     * @return Raw ADC value
     */
    int16_t getPotentiometerRaw(uint8_t index);
    
    /**
     * Get gimbal axis name by index
     * @param index Axis index
     * @return Axis name string
     */
    const char* getGimbalAxisName(uint8_t index);
    
    /**
     * Get potentiometer name by index
     * @param index Pot index
     * @return Pot name string
     */
    const char* getPotentiometerName(uint8_t index);
    
    /**
     * Set calibration for a gimbal axis
     * @param index Axis index
     * @param min_val Raw value at minimum position
     * @param center_val Raw value at center position
     * @param max_val Raw value at maximum position
     * @param deadzone Deadzone around center
     * @param inverted True to invert axis direction
     */
    void calibrateGimbalAxis(uint8_t index, int16_t min_val, int16_t center_val, int16_t max_val, int16_t deadzone = 200, bool inverted = false);
    
    /**
     * Set calibration for a potentiometer
     * @param index Pot index
     * @param min_val Raw value at minimum position
     * @param max_val Raw value at maximum position
     * @param inverted True to invert direction
     */
    void calibratePotentiometer(uint8_t index, int16_t min_val, int16_t max_val, bool inverted = false);
    
    /**
     * Check if ADCs are initialized and responding
     * @return true if all ADCs are working
     */
    bool isReady();
    
    /**
     * Read raw value from specific ADC channel (for debugging)
     * @param adc ADC index (0-2)
     * @param channel Channel (0-3)
     * @return Raw ADC value
     */
    int16_t readRaw(uint8_t adc, uint8_t channel);

private:
    Adafruit_ADS1115 _ads[3];           // Three ADS1115 ADCs
    bool _initialized[3];                // Initialization status
    
    // Gimbal configurations and states
    AnalogAxisConfig_t _gimbalConfigs[NUM_GIMBAL_AXES];
    AnalogAxisState_t _gimbalStates[NUM_GIMBAL_AXES];
    
    // Potentiometer configurations and states
    AnalogAxisConfig_t _potConfigs[NUM_POTENTIOMETERS];
    AnalogAxisState_t _potStates[NUM_POTENTIOMETERS];
    
    // Filter buffers for moving average
    int16_t _gimbalFilterBuf[NUM_GIMBAL_AXES][ANALOG_FILTER_SAMPLES];
    int16_t _potFilterBuf[NUM_POTENTIOMETERS][ANALOG_FILTER_SAMPLES];
    uint8_t _filterIndex;
    
    /**
     * Initialize configurations from defines
     */
    void initConfigs();
    
    /**
     * Read and process a gimbal axis
     */
    void updateGimbalAxis(uint8_t index);
    
    /**
     * Read and process a potentiometer
     */
    void updatePotentiometer(uint8_t index);
    
    /**
     * Apply calibration and scaling to a gimbal value
     * @param raw Raw ADC value
     * @param cfg Axis configuration
     * @return Calibrated value (-1000 to +1000)
     */
    int16_t calibrateGimbal(int16_t raw, const AnalogAxisConfig_t* cfg);
    
    /**
     * Apply calibration and scaling to a pot value
     * @param raw Raw ADC value
     * @param cfg Axis configuration
     * @return Calibrated value (0 to 1000)
     */
    int16_t calibratePot(int16_t raw, const AnalogAxisConfig_t* cfg);
    
    /**
     * Apply moving average filter
     * @param buffer Filter buffer
     * @param newValue New sample value
     * @return Filtered value
     */
    int16_t applyFilter(int16_t* buffer, int16_t newValue);
};

// Global instance
extern ADS1X15_Driver AnalogInput;
