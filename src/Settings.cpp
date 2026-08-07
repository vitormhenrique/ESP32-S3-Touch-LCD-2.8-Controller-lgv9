#include "Settings.h"
#include <string.h>
#include <stdio.h>
#include <Preferences.h>

/******************************************************************************
 * Settings Module Implementation - With NVS Persistence
 ******************************************************************************/

static Settings_t settings;
static bool initialized = false;
static Preferences prefs;

// NVS namespace and keys
static const char* NVS_NAMESPACE = "rc_settings";
static const char* KEY_GIMBAL_CAL = "gimbal_cal";
static const char* KEY_POT_CAL = "pot_cal";
static const char* KEY_ROBOT_PROFILE = "robot_prof";

//=============================================================================
// NVS Persistence Functions
//=============================================================================

static bool load_from_nvs(void) {
    if (!prefs.begin(NVS_NAMESPACE, true)) {  // true = read-only
        printf("Settings: NVS namespace not found, using defaults\r\n");
        return false;
    }
    
    // Load robot profile
    if (prefs.isKey(KEY_ROBOT_PROFILE)) {
        settings.robot_profile = (RobotProfile_t)prefs.getUChar(KEY_ROBOT_PROFILE, ROBOT_PROFILE_GENERIC);
        printf("Settings: Loaded robot profile: %s\r\n", Settings_GetRobotProfileName(settings.robot_profile));
    }
    
    // Load gimbal calibrations
    size_t len = prefs.getBytesLength(KEY_GIMBAL_CAL);
    if (len == sizeof(settings.gimbal_cal)) {
        prefs.getBytes(KEY_GIMBAL_CAL, settings.gimbal_cal, len);
        printf("Settings: Loaded gimbal calibration from NVS\r\n");
        
        // Print loaded calibration info
        for (int i = 0; i < 4; i++) {
            if (settings.gimbal_cal[i].calibrated) {
                printf("  Axis %d: min=%d, center=%d, max=%d, inv=%d\r\n",
                       i, settings.gimbal_cal[i].min_raw, 
                       settings.gimbal_cal[i].center_raw,
                       settings.gimbal_cal[i].max_raw,
                       settings.gimbal_cal[i].inverted);
            }
        }
    } else {
        printf("Settings: No valid gimbal calibration in NVS\r\n");
    }
    
    // Load pot calibrations
    len = prefs.getBytesLength(KEY_POT_CAL);
    if (len == sizeof(settings.pot_cal)) {
        prefs.getBytes(KEY_POT_CAL, settings.pot_cal, len);
        printf("Settings: Loaded pot calibration from NVS\r\n");
        for (int i = 0; i < 2; i++) {
            if (settings.pot_cal[i].calibrated) {
                printf("  Pot %d: min=%d, max=%d, inv=%d\r\n",
                       i, settings.pot_cal[i].min_raw,
                       settings.pot_cal[i].max_raw,
                       settings.pot_cal[i].inverted);
            }
        }
    } else {
        printf("Settings: No valid pot calibration in NVS\r\n");
    }
    
    prefs.end();
    return true;
}

static bool save_pot_cal_to_nvs(void) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {  // false = read-write
        printf("Settings: Failed to open NVS for writing\r\n");
        return false;
    }
    
    size_t written = prefs.putBytes(KEY_POT_CAL, settings.pot_cal, sizeof(settings.pot_cal));
    prefs.end();
    
    if (written == sizeof(settings.pot_cal)) {
        printf("Settings: Saved pot calibration to NVS (%d bytes)\r\n", written);
        return true;
    } else {
        printf("Settings: Failed to save pot calibration\r\n");
        return false;
    }
}

static bool save_gimbal_cal_to_nvs(void) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {  // false = read-write
        printf("Settings: Failed to open NVS for writing\r\n");
        return false;
    }
    
    size_t written = prefs.putBytes(KEY_GIMBAL_CAL, settings.gimbal_cal, sizeof(settings.gimbal_cal));
    prefs.end();
    
    if (written == sizeof(settings.gimbal_cal)) {
        printf("Settings: Saved gimbal calibration to NVS (%d bytes)\r\n", written);
        return true;
    } else {
        printf("Settings: Failed to save gimbal calibration\r\n");
        return false;
    }
}

static bool save_robot_profile_to_nvs(void) {
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        printf("Settings: Failed to open NVS for writing\r\n");
        return false;
    }
    
    size_t written = prefs.putUChar(KEY_ROBOT_PROFILE, (uint8_t)settings.robot_profile);
    prefs.end();
    
    if (written == 1) {
        printf("Settings: Saved robot profile to NVS: %s\r\n", 
               Settings_GetRobotProfileName(settings.robot_profile));
        return true;
    } else {
        printf("Settings: Failed to save robot profile\r\n");
        return false;
    }
}

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
    
    // Pot calibration defaults
    for (int i = 0; i < 2; i++) {
        settings.pot_cal[i].min_raw = 0;
        settings.pot_cal[i].max_raw = 32767;
        settings.pot_cal[i].inverted = false;
        settings.pot_cal[i].calibrated = false;
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
    
    printf("Settings: Initializing with NVS persistence\r\n");
    apply_defaults();
    
    // Try to load saved settings from NVS
    load_from_nvs();

    // DEBUG: Force Generic profile to debug crash
    // settings.robot_profile = ROBOT_PROFILE_GENERIC;
    printf("Settings: DEBUG - Forcing Generic profile\r\n");
    
    initialized = true;
    printf("Settings: Ready\r\n");
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
    // Auto-save to NVS when profile is changed
    save_robot_profile_to_nvs();
}

void Settings_SetTouchCalibration(const TouchCalibration_t* cal) {
    if (cal) {
        memcpy(&settings.touch_cal, cal, sizeof(TouchCalibration_t));
    }
}

void Settings_SetGimbalCalibration(uint8_t axis, const GimbalCalibration_t* cal) {
    if (axis < 4 && cal) {
        memcpy(&settings.gimbal_cal[axis], cal, sizeof(GimbalCalibration_t));
        // Auto-save to NVS when calibration is updated
        save_gimbal_cal_to_nvs();
    }
}

void Settings_SetPotCalibration(uint8_t pot, const PotCalibration_t* cal) {
    if (pot < 2 && cal) {
        memcpy(&settings.pot_cal[pot], cal, sizeof(PotCalibration_t));
        // Auto-save to NVS when calibration is updated
        save_pot_cal_to_nvs();
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
