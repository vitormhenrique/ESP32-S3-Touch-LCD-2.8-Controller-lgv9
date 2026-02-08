/**
 * @file ui_screen_config.h
 * @brief Config Screen Header
 * 
 * Previously "Settings" - renamed to "Config"
 * Contains buttons for WiFi, Calibration, Display, and About
 */

#ifndef UI_SCREEN_CONFIG_H
#define UI_SCREEN_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_common.h"

//=============================================================================
// Screen Objects
//=============================================================================

extern lv_obj_t *ui_ConfigScreen;

//=============================================================================
// Functions
//=============================================================================

/**
 * @brief Create the Config screen
 * @param parent Parent object (usually screen container)
 */
void ui_config_screen_create(lv_obj_t *parent);

/**
 * @brief Show or hide the Config screen
 */
void ui_config_screen_show(bool show);

/**
 * @brief Set WiFi connection status display
 * @param connected True if connected
 * @param ssid Network name
 * @param ip IP address string
 */
void ui_config_set_wifi_status(bool connected, const char *ssid, const char *ip);

/**
 * @brief Set display brightness level
 * @param brightness Brightness 0-100
 */
void ui_config_set_brightness(uint8_t brightness);

/**
 * @brief Set firmware version display
 * @param version Version string
 */
void ui_config_set_version(const char *version);

//=============================================================================
// Callback Types
//=============================================================================

typedef void (*config_callback_t)(void);

/**
 * @brief Set callback for WiFi button press
 */
void ui_config_set_wifi_callback(config_callback_t cb);

/**
 * @brief Set callback for Calibration button press
 */
void ui_config_set_calibration_callback(config_callback_t cb);

/**
 * @brief Set callback for Display button press
 */
void ui_config_set_display_callback(config_callback_t cb);

/**
 * @brief Set callback for About button press
 */
void ui_config_set_about_callback(config_callback_t cb);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_CONFIG_H
