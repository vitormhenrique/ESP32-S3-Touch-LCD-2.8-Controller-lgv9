/**
 * @file Settings_stub.c
 * Settings API stub for LVGL Simulator
 * Provides the same API as the ESP32 version but in-memory only
 */

#include "Settings.h"
#include <string.h>
#include <stdio.h>

static Settings_t g_settings;

//=============================================================================
// Default Values
//=============================================================================

static void apply_defaults(Settings_t* s)
{
    memset(s, 0, sizeof(Settings_t));
    
    s->robot_profile = ROBOT_PROFILE_GENERIC;
    
    // Touch calibration defaults (uncalibrated)
    s->touch_cal.x_min = 0;
    s->touch_cal.x_max = 320;
    s->touch_cal.y_min = 0;
    s->touch_cal.y_max = 240;
    s->touch_cal.calibrated = false;
    
    // Gimbal defaults (4 axes)
    for (int i = 0; i < 4; i++) {
        s->gimbal_cal[i].min_raw = 0;
        s->gimbal_cal[i].max_raw = 4095;
        s->gimbal_cal[i].center_raw = 2048;
        s->gimbal_cal[i].deadzone = 50;
        s->gimbal_cal[i].inverted = false;
        s->gimbal_cal[i].calibrated = false;
    }
    
    // Radio defaults
    s->radio.packet_rate = 1;     // 150Hz
    s->radio.tx_power = 2;        // 50mW
    s->radio.telemetry_ratio = 4; // 1:16
    strncpy(s->radio.bind_phrase, "simulator", sizeof(s->radio.bind_phrase) - 1);
    
    // Display defaults
    s->display.brightness = 200;
    s->display.screen_timeout = 60;
}

//=============================================================================
// API Implementation
//=============================================================================

void Settings_Init(void)
{
    printf("Settings: Initializing (simulator stub)...\n");
    apply_defaults(&g_settings);
    printf("Settings: Initialized with defaults\n");
}

const Settings_t* Settings_Get(void)
{
    return &g_settings;
}

Settings_t* Settings_GetMutable(void)
{
    return &g_settings;
}

void Settings_ResetToDefaults(void)
{
    printf("Settings: Reset to defaults\n");
    apply_defaults(&g_settings);
}

void Settings_SetRobotProfile(RobotProfile_t profile)
{
    g_settings.robot_profile = profile;
}

void Settings_SetTouchCalibration(const TouchCalibration_t* cal)
{
    if (cal) {
        memcpy(&g_settings.touch_cal, cal, sizeof(TouchCalibration_t));
    }
}

void Settings_SetGimbalCalibration(uint8_t axis, const GimbalCalibration_t* cal)
{
    if (axis < 4 && cal) {
        memcpy(&g_settings.gimbal_cal[axis], cal, sizeof(GimbalCalibration_t));
    }
}

void Settings_SetRadio(const RadioSettings_t* radio)
{
    if (radio) {
        memcpy(&g_settings.radio, radio, sizeof(RadioSettings_t));
    }
}

void Settings_SetDisplayBrightness(uint8_t brightness)
{
    g_settings.display.brightness = brightness;
}

//=============================================================================
// Name Getters
//=============================================================================

const char* Settings_GetRobotProfileName(RobotProfile_t profile)
{
    switch (profile) {
        case ROBOT_PROFILE_GENERIC: return "Generic";
        case ROBOT_PROFILE_HEXAPOD: return "Hexapod";
        default: return "Unknown";
    }
}

const char* Settings_GetPacketRateName(uint8_t rate)
{
    static const char* names[] = {"50 Hz", "150 Hz", "250 Hz", "500 Hz"};
    return rate < 4 ? names[rate] : "Unknown";
}

const char* Settings_GetTxPowerName(uint8_t power)
{
    static const char* names[] = {"10 mW", "25 mW", "50 mW", "100 mW", "250 mW"};
    return power < 5 ? names[power] : "Unknown";
}

const char* Settings_GetTelemetryRatioName(uint8_t ratio)
{
    static const char* names[] = {"Off", "1:128", "1:64", "1:32", "1:16", "1:8", "1:4", "1:2"};
    return ratio < 8 ? names[ratio] : "Unknown";
}
