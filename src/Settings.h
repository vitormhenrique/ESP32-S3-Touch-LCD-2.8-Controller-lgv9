#pragma once
#include <stdint.h>
#include <stdbool.h>

/******************************************************************************
 * Settings Module - Configuration with NVS Persistence
 * 
 * Stores runtime configuration for radio, robot, calibration settings.
 * Gimbal calibration is automatically saved to NVS flash and restored on boot.
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Robot Profile Types
//=============================================================================

typedef enum {
    ROBOT_PROFILE_GENERIC = 0,
    ROBOT_PROFILE_HEXAPOD,
    ROBOT_PROFILE_COUNT
} RobotProfile_t;

//=============================================================================
// Touch Calibration Data
//=============================================================================

typedef struct {
    int16_t x_min;          // Raw touch X at screen left
    int16_t x_max;          // Raw touch X at screen right
    int16_t y_min;          // Raw touch Y at screen top
    int16_t y_max;          // Raw touch Y at screen bottom
    bool calibrated;        // True if calibration has been performed
} TouchCalibration_t;

//=============================================================================
// Gimbal Calibration Data (per axis)
//=============================================================================

typedef struct {
    int16_t min_raw;        // Raw ADC value at minimum position
    int16_t max_raw;        // Raw ADC value at maximum position
    int16_t center_raw;     // Raw ADC value at center
    int16_t deadzone;       // Deadzone around center
    bool inverted;          // True to invert axis direction
    bool calibrated;        // True if calibration has been performed
} GimbalCalibration_t;

//=============================================================================
// Potentiometer Calibration Data (per pot)
//=============================================================================

typedef struct {
    int16_t min_raw;        // Raw ADC value at minimum position
    int16_t max_raw;        // Raw ADC value at maximum position
    bool inverted;          // True to invert direction (min recorded > max)
    bool calibrated;        // True if calibration has been performed
} PotCalibration_t;

//=============================================================================
// Radio Settings (ExpressLRS)
//=============================================================================

typedef struct {
    uint8_t packet_rate;    // 0=50Hz, 1=150Hz, 2=250Hz, 3=500Hz
    uint8_t tx_power;       // 0=10mW, 1=25mW, 2=50mW, 3=100mW, 4=250mW
    uint8_t telemetry_ratio;// 0=Off, 1=1:128, 2=1:64, 3=1:32, 4=1:16, 5=1:8, 6=1:4, 7=1:2
    char bind_phrase[32];   // Bind phrase for ELRS
} RadioSettings_t;

//=============================================================================
// Display Settings
//=============================================================================

typedef struct {
    uint8_t brightness;     // 0-255
    uint8_t screen_timeout; // Screen timeout in seconds (0=disabled)
} DisplaySettings_t;

//=============================================================================
// All Settings Structure
//=============================================================================

typedef struct {
    // Robot configuration
    RobotProfile_t robot_profile;
    
    // Touch calibration
    TouchCalibration_t touch_cal;
    
    // Gimbal calibrations (4 axes)
    GimbalCalibration_t gimbal_cal[4];
    
    // Potentiometer calibrations (2 pots)
    PotCalibration_t pot_cal[2];
    
    // Radio settings
    RadioSettings_t radio;
    
    // Display settings
    DisplaySettings_t display;
} Settings_t;

//=============================================================================
// Settings API
//=============================================================================

// Initialize settings with defaults
void Settings_Init(void);

// Get pointer to settings (for reading)
const Settings_t* Settings_Get(void);

// Get mutable pointer (for editing)
Settings_t* Settings_GetMutable(void);

// Reset to factory defaults
void Settings_ResetToDefaults(void);

// Individual setters
void Settings_SetRobotProfile(RobotProfile_t profile);
void Settings_SetTouchCalibration(const TouchCalibration_t* cal);
void Settings_SetGimbalCalibration(uint8_t axis, const GimbalCalibration_t* cal);
void Settings_SetPotCalibration(uint8_t pot, const PotCalibration_t* cal);
void Settings_SetRadio(const RadioSettings_t* radio);
void Settings_SetDisplayBrightness(uint8_t brightness);

// Name getters for UI
const char* Settings_GetRobotProfileName(RobotProfile_t profile);
const char* Settings_GetPacketRateName(uint8_t rate);
const char* Settings_GetTxPowerName(uint8_t power);
const char* Settings_GetTelemetryRatioName(uint8_t ratio);

#ifdef __cplusplus
}
#endif
