#include "SettingsManager.h"

// Global instance
SettingsManager Settings;

// Robot profile names
static const char* ROBOT_PROFILE_NAMES[] = {
    "Generic Robot",
    "Hexapod"
};

SettingsManager::SettingsManager() {
    _dirty = false;
    loadDefaults();
}

void SettingsManager::loadDefaults() {
    _settings.magic = SETTINGS_MAGIC;
    _settings.version = SETTINGS_VERSION;
    _settings.robotProfile = ROBOT_GENERIC;
    
    // Default gimbal calibration
    for (int i = 0; i < 4; i++) {
        _settings.gimbalCal[i].min_raw = 0;
        _settings.gimbalCal[i].center_raw = 13000;
        _settings.gimbalCal[i].max_raw = 26000;
        _settings.gimbalCal[i].inverted = false;
    }
    
    // Touch calibration - not calibrated by default
    _settings.touchCal.x1 = 0; _settings.touchCal.y1 = 0;
    _settings.touchCal.x2 = 0; _settings.touchCal.y2 = 0;
    _settings.touchCal.x3 = 0; _settings.touchCal.y3 = 0;
    _settings.touchCal.x4 = 0; _settings.touchCal.y4 = 0;
    _settings.touchCal.calibrated = false;
    
    // Display settings
    _settings.display.brightness = 200;
    _settings.display.screenTimeout = 0;  // Never
    _settings.display.invertDisplay = false;
    
    // Radio settings
    strcpy(_settings.radio.ssid, "RCController");
    strcpy(_settings.radio.password, "");
    _settings.radio.channel = 1;
    _settings.radio.txPower = 20;
    _settings.radio.autoConnect = true;
    
    // Encoder presets
    _settings.encoderPresets[0] = 0;
    _settings.encoderPresets[1] = 0;
}

bool SettingsManager::begin() {
    printf("=== Settings Manager Initialization ===\r\n");
    
    if (!_prefs.begin("rc_settings", false)) {
        printf("Settings: Failed to open preferences\r\n");
        return false;
    }
    
    if (loadFromNVS()) {
        printf("Settings: Loaded from NVS\r\n");
        printf("  Robot Profile: %s\r\n", getRobotProfileName(_settings.robotProfile));
        printf("  Brightness: %d\r\n", _settings.display.brightness);
        printf("  Touch Calibrated: %s\r\n", _settings.touchCal.calibrated ? "Yes" : "No");
    } else {
        printf("Settings: Using defaults (first boot or corrupted)\r\n");
        loadDefaults();
        save();
    }
    
    printf("=======================================\r\n");
    return true;
}

bool SettingsManager::loadFromNVS() {
    size_t size = _prefs.getBytes("settings", &_settings, sizeof(Settings_t));
    
    if (size != sizeof(Settings_t)) {
        printf("Settings: Size mismatch (%d vs %d)\r\n", size, sizeof(Settings_t));
        return false;
    }
    
    if (_settings.magic != SETTINGS_MAGIC) {
        printf("Settings: Magic mismatch\r\n");
        return false;
    }
    
    if (_settings.version != SETTINGS_VERSION) {
        printf("Settings: Version mismatch (stored=%d, current=%d)\r\n", 
               _settings.version, SETTINGS_VERSION);
        // Could add migration logic here
        return false;
    }
    
    return true;
}

bool SettingsManager::saveToNVS() {
    size_t written = _prefs.putBytes("settings", &_settings, sizeof(Settings_t));
    return written == sizeof(Settings_t);
}

bool SettingsManager::save() {
    if (saveToNVS()) {
        printf("Settings: Saved to NVS\r\n");
        _dirty = false;
        return true;
    } else {
        printf("Settings: Failed to save!\r\n");
        return false;
    }
}

void SettingsManager::resetToDefaults() {
    loadDefaults();
    save();
    printf("Settings: Reset to defaults\r\n");
}

//=============================================================================
// Robot Profile
//=============================================================================

void SettingsManager::setRobotProfile(RobotProfile_t profile) {
    if (profile < ROBOT_PROFILE_COUNT) {
        _settings.robotProfile = profile;
        _dirty = true;
    }
}

const char* SettingsManager::getRobotProfileName(RobotProfile_t profile) {
    if (profile < ROBOT_PROFILE_COUNT) {
        return ROBOT_PROFILE_NAMES[profile];
    }
    return "Unknown";
}

//=============================================================================
// Gimbal Calibration
//=============================================================================

GimbalCalibration_t& SettingsManager::getGimbalCalibration(uint8_t axis) {
    static GimbalCalibration_t dummy = {0, 13000, 26000, false};
    if (axis >= 4) return dummy;
    return _settings.gimbalCal[axis];
}

void SettingsManager::setGimbalCalibration(uint8_t axis, int16_t min_val, int16_t center_val, int16_t max_val, bool inverted) {
    if (axis >= 4) return;
    _settings.gimbalCal[axis].min_raw = min_val;
    _settings.gimbalCal[axis].center_raw = center_val;
    _settings.gimbalCal[axis].max_raw = max_val;
    _settings.gimbalCal[axis].inverted = inverted;
    _dirty = true;
}

bool SettingsManager::isGimbalCalibrated(uint8_t axis) {
    if (axis >= 4) return false;
    // Check if values are non-default
    GimbalCalibration_t& cal = _settings.gimbalCal[axis];
    return (cal.min_raw != 0 || cal.center_raw != 13000 || cal.max_raw != 26000);
}

//=============================================================================
// Touch Screen Calibration
//=============================================================================

void SettingsManager::setTouchCalibration(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                                          int16_t x3, int16_t y3, int16_t x4, int16_t y4) {
    _settings.touchCal.x1 = x1; _settings.touchCal.y1 = y1;
    _settings.touchCal.x2 = x2; _settings.touchCal.y2 = y2;
    _settings.touchCal.x3 = x3; _settings.touchCal.y3 = y3;
    _settings.touchCal.x4 = x4; _settings.touchCal.y4 = y4;
    _settings.touchCal.calibrated = true;
    _dirty = true;
}

//=============================================================================
// Display Settings
//=============================================================================

void SettingsManager::setBrightness(uint8_t brightness) {
    _settings.display.brightness = brightness;
    _dirty = true;
}

void SettingsManager::setScreenTimeout(uint8_t minutes) {
    _settings.display.screenTimeout = minutes;
    _dirty = true;
}

//=============================================================================
// Radio Settings
//=============================================================================

void SettingsManager::setWiFiSSID(const char* ssid) {
    strncpy(_settings.radio.ssid, ssid, sizeof(_settings.radio.ssid) - 1);
    _settings.radio.ssid[sizeof(_settings.radio.ssid) - 1] = '\0';
    _dirty = true;
}

void SettingsManager::setWiFiPassword(const char* password) {
    strncpy(_settings.radio.password, password, sizeof(_settings.radio.password) - 1);
    _settings.radio.password[sizeof(_settings.radio.password) - 1] = '\0';
    _dirty = true;
}

void SettingsManager::setTxPower(uint8_t dbm) {
    _settings.radio.txPower = dbm;
    _dirty = true;
}

void SettingsManager::setAutoConnect(bool enable) {
    _settings.radio.autoConnect = enable;
    _dirty = true;
}
