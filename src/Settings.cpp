#include "Settings.h"
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Settings Module Implementation - Memory Only (No Persistence)
 ******************************************************************************/

static Settings_t settings;
static bool initialized = false;

//=============================================================================
// Default Values
//=============================================================================

static void apply_defaults(void) {
    memset(&settings, 0, sizeof(Settings_t));
    
    settings.robot_profile = ROBOT_PROFILE_GENERIC;
    
    // Touch calibration defaults (needs calibration)
    settings.touch_cal.x_min = 0;
    settings.touch_cal.x_max = 320;
    settings.touch_cal.y_min = 0;
    settings.touch_cal.y_max = 240;
    settings.touch_cal.calibrated = false;
    
    // Gimbal calibration defaults
    for (int i = 0; i < 4; i++) {
        settings.gimbal_cal[i].min_raw = 0;
        settings.gimbal_cal[i].max_raw = 32767;
        settings.gimbal_cal[i].center_raw = 16383;
        settings.gimbal_cal[i].deadzone = 200;
        settings.gimbal_cal[i].inverted = false;
        settings.gimbal_cal[i].calibrated = false;
    }
    
    // Radio defaults (ELRS)
    settings.radio.packet_rate = 2;      // 250Hz
    settings.radio.tx_power = 2;         // 50mW
    settings.radio.telemetry_ratio = 4;  // 1:16
    strncpy(settings.radio.bind_phrase, "default", sizeof(settings.radio.bind_phrase) - 1);
    
    // Display defaults
    settings.display.brightness = 200;
    settings.display.screen_timeout = 0; // Disabled
}

//=============================================================================
// Public API Implementation
//=============================================================================

void Settings_Init(void) {
    if (initialized) return;
    
    printf("Settings: Initializing (memory-only, no persistence)\r\n");
    apply_defaults();
    initialized = true;
    printf("Settings: Ready with defaults\r\n");
}

const Settings_t* Settings_Get(void) {
    if (!initialized) Settings_Init();
    return &settings;
}

Settings_t* Settings_GetMutable(void) {
    if (!initialized) Settings_Init();
    return &settings;
}

void Settings_ResetToDefaults(void) {
    printf("Settings: Resetting to defaults\r\n");
    apply_defaults();
}

void Settings_SetRobotProfile(RobotProfile_t profile) {
    settings.robot_profile = profile;
}

void Settings_SetTouchCalibration(const TouchCalibration_t* cal) {
    if (cal) {
        memcpy(&settings.touch_cal, cal, sizeof(TouchCalibration_t));
    }
}

void Settings_SetGimbalCalibration(uint8_t axis, const GimbalCalibration_t* cal) {
    if (axis < 4 && cal) {
        memcpy(&settings.gimbal_cal[axis], cal, sizeof(GimbalCalibration_t));
    }
}

void Settings_SetRadio(const RadioSettings_t* radio) {
    if (radio) {
        memcpy(&settings.radio, radio, sizeof(RadioSettings_t));
    }
}

void Settings_SetDisplayBrightness(uint8_t brightness) {
    settings.display.brightness = brightness;
}

//=============================================================================
// Name Getters for UI
//=============================================================================

const char* Settings_GetRobotProfileName(RobotProfile_t profile) {
    switch (profile) {
        case ROBOT_PROFILE_GENERIC: return "Generic";
        case ROBOT_PROFILE_HEXAPOD: return "Hexapod";
        default: return "Unknown";
    }
}

const char* Settings_GetPacketRateName(uint8_t rate) {
    static const char* names[] = {"50Hz", "150Hz", "250Hz", "500Hz"};
    if (rate < 4) return names[rate];
    return "Unknown";
}

const char* Settings_GetTxPowerName(uint8_t power) {
    static const char* names[] = {"10mW", "25mW", "50mW", "100mW", "250mW"};
    if (power < 5) return names[power];
    return "Unknown";
}

const char* Settings_GetTelemetryRatioName(uint8_t ratio) {
    static const char* names[] = {"Off", "1:128", "1:64", "1:32", "1:16", "1:8", "1:4", "1:2"};
    if (ratio < 8) return names[ratio];
    return "Unknown";
}
