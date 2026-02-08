#pragma once
#include <Arduino.h>
#include <Preferences.h>

/******************************************************************************
 * Settings Manager
 * 
 * Persistent storage for RC Controller settings using ESP32 Preferences
 * Survives reboots and power cycles
 ******************************************************************************/

// Robot profiles
typedef enum {
    ROBOT_GENERIC = 0,
    ROBOT_HEXAPOD,
    ROBOT_PROFILE_COUNT
} RobotProfile_t;

// Gimbal calibration data
typedef struct {
    int16_t min_raw;
    int16_t center_raw;
    int16_t max_raw;
    bool inverted;
} GimbalCalibration_t;

// Touch screen calibration data
typedef struct {
    int16_t x1, y1;  // Top-left calibration point
    int16_t x2, y2;  // Top-right calibration point
    int16_t x3, y3;  // Bottom-left calibration point
    int16_t x4, y4;  // Bottom-right calibration point
    bool calibrated;
} TouchCalibration_t;

// Display settings
typedef struct {
    uint8_t brightness;     // 0-255
    uint8_t screenTimeout;  // Minutes, 0 = never
    bool invertDisplay;
} DisplaySettings_t;

// Radio settings
typedef struct {
    char ssid[32];
    char password[64];
    uint8_t channel;
    uint8_t txPower;        // dBm
    bool autoConnect;
} RadioSettings_t;

// All settings
typedef struct {
    uint32_t magic;                         // Magic number for validation
    uint8_t version;                        // Settings version
    RobotProfile_t robotProfile;            // Selected robot profile
    GimbalCalibration_t gimbalCal[4];       // Calibration for 4 gimbal axes
    TouchCalibration_t touchCal;            // Touch screen calibration
    DisplaySettings_t display;              // Display settings
    RadioSettings_t radio;                  // Radio settings
    int32_t encoderPresets[2];              // Encoder reset values
} Settings_t;

#define SETTINGS_MAGIC 0x52435331  // "RCS1" in hex
#define SETTINGS_VERSION 1

class SettingsManager {
public:
    SettingsManager();
    
    /**
     * Initialize settings manager, load from NVS
     * @return true if settings loaded successfully
     */
    bool begin();
    
    /**
     * Save all settings to NVS
     * @return true if saved successfully
     */
    bool save();
    
    /**
     * Reset all settings to defaults
     */
    void resetToDefaults();
    
    /**
     * Get current settings (read-only)
     */
    const Settings_t& getSettings() const { return _settings; }
    
    /**
     * Get settings for modification
     */
    Settings_t& settings() { return _settings; }
    
    //=========================================================================
    // Robot Profile
    //=========================================================================
    
    RobotProfile_t getRobotProfile() { return _settings.robotProfile; }
    void setRobotProfile(RobotProfile_t profile);
    const char* getRobotProfileName(RobotProfile_t profile);
    
    //=========================================================================
    // Gimbal Calibration
    //=========================================================================
    
    GimbalCalibration_t& getGimbalCalibration(uint8_t axis);
    void setGimbalCalibration(uint8_t axis, int16_t min_val, int16_t center_val, int16_t max_val, bool inverted);
    bool isGimbalCalibrated(uint8_t axis);
    
    //=========================================================================
    // Touch Screen Calibration
    //=========================================================================
    
    TouchCalibration_t& getTouchCalibration() { return _settings.touchCal; }
    void setTouchCalibration(int16_t x1, int16_t y1, int16_t x2, int16_t y2,
                             int16_t x3, int16_t y3, int16_t x4, int16_t y4);
    bool isTouchCalibrated() { return _settings.touchCal.calibrated; }
    
    //=========================================================================
    // Display Settings
    //=========================================================================
    
    uint8_t getBrightness() { return _settings.display.brightness; }
    void setBrightness(uint8_t brightness);
    
    uint8_t getScreenTimeout() { return _settings.display.screenTimeout; }
    void setScreenTimeout(uint8_t minutes);
    
    //=========================================================================
    // Radio Settings
    //=========================================================================
    
    const char* getWiFiSSID() { return _settings.radio.ssid; }
    void setWiFiSSID(const char* ssid);
    
    const char* getWiFiPassword() { return _settings.radio.password; }
    void setWiFiPassword(const char* password);
    
    uint8_t getTxPower() { return _settings.radio.txPower; }
    void setTxPower(uint8_t dbm);
    
    bool getAutoConnect() { return _settings.radio.autoConnect; }
    void setAutoConnect(bool enable);

private:
    Preferences _prefs;
    Settings_t _settings;
    bool _dirty;  // Settings modified since last save
    
    void loadDefaults();
    bool loadFromNVS();
    bool saveToNVS();
};

// Global instance
extern SettingsManager Settings;
