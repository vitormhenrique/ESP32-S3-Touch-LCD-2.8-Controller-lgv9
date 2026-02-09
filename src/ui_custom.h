/**
 * @file ui_custom.h
 * @brief Custom RC Controller UI Components for LVGL 9
 * 
 * Multi-screen UI with navigation:
 * - Input screen: Gimbals, buttons, switches, toggles, pots, encoders
 * - Telemetry screen: Robot data with scrollable panels
 * - Settings screen: Configuration options
 */

#ifndef UI_CUSTOM_H
#define UI_CUSTOM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

//=============================================================================
// Theme Colors - Modern Dark Theme with Accent Colors
//=============================================================================

// Base colors
#define UI_COLOR_BG_DARK       0x0D1117  // Very dark background
#define UI_COLOR_BG_PANEL      0x161B22  // Panel background
#define UI_COLOR_BG_CARD       0x21262D  // Card/elevated background
#define UI_COLOR_BORDER        0x30363D  // Border color
#define UI_COLOR_BORDER_ACCENT 0x3D444D  // Lighter border

// Text colors
#define UI_COLOR_TEXT_PRIMARY   0xE6EDF3  // Primary text
#define UI_COLOR_TEXT_SECONDARY 0x8B949E  // Secondary/muted text
#define UI_COLOR_TEXT_TERTIARY  0x6E7681  // Tertiary text

// Accent colors
#define UI_COLOR_ACCENT_BLUE    0x58A6FF  // Primary accent (blue)
#define UI_COLOR_ACCENT_GREEN   0x3FB950  // Success/active (green)
#define UI_COLOR_ACCENT_ORANGE  0xD29922  // Warning (orange)
#define UI_COLOR_ACCENT_RED     0xF85149  // Error/danger (red)
#define UI_COLOR_ACCENT_PURPLE  0xA371F7  // Purple accent
#define UI_COLOR_ACCENT_CYAN    0x39C5CF  // Cyan accent

// State colors
#define UI_COLOR_SWITCH_ON      0x3FB950  // Green for ON
#define UI_COLOR_SWITCH_OFF     0x484F58  // Gray for OFF
#define UI_COLOR_BTN_PRESSED    0x58A6FF  // Blue for pressed
#define UI_COLOR_BTN_RELEASED   0x21262D  // Dark for released
#define UI_COLOR_NAV_ACTIVE     0x58A6FF  // Active nav button
#define UI_COLOR_NAV_INACTIVE   0x21262D  // Inactive nav button

//=============================================================================
// UI Element Sizes
//=============================================================================

#define UI_GIMBAL_SIZE          55       // Gimbal display size
#define UI_GIMBAL_DOT_SIZE      12       // Gimbal position dot size
#define UI_SWITCH_WIDTH         28       // 2-pos switch width
#define UI_SWITCH_HEIGHT        14       // 2-pos switch height
#define UI_TOGGLE3_WIDTH        16       // 3-pos toggle width
#define UI_TOGGLE3_HEIGHT       18       // 3-pos toggle height
#define UI_POT_BAR_WIDTH        70       // Potentiometer bar width
#define UI_POT_BAR_HEIGHT       6        // Potentiometer bar height
#define UI_BTN_WIDTH            32       // Button width
#define UI_BTN_HEIGHT           16       // Button height

// Screen dimensions (320x240 in landscape)
#define UI_SCREEN_WIDTH         320
#define UI_SCREEN_HEIGHT        240

// Layout heights
#define UI_HEADER_HEIGHT        24
#define UI_NAV_HEIGHT           30
#define UI_CONTENT_HEIGHT       (UI_SCREEN_HEIGHT - UI_HEADER_HEIGHT - UI_NAV_HEIGHT)

//=============================================================================
// Screen Types
//=============================================================================

typedef enum {
    SCREEN_INPUT = 0,
    SCREEN_TELEMETRY,
    SCREEN_SETTINGS,
    SCREEN_COUNT
} ScreenType_t;

//=============================================================================
// Battery State
//=============================================================================

typedef enum {
    BATTERY_STATE_UNKNOWN = 0,
    BATTERY_STATE_DISCHARGING,
    BATTERY_STATE_CHARGING,
    BATTERY_STATE_FULL
} BatteryState_t;

//=============================================================================
// UI Objects
//=============================================================================

// Main screen
extern lv_obj_t *ui_MainScreen;

// Header panel
extern lv_obj_t *ui_HeaderPanel;
extern lv_obj_t *ui_TitleLabel;
extern lv_obj_t *ui_StatusIcon;
extern lv_obj_t *ui_BatteryIcon;
extern lv_obj_t *ui_BatteryLabel;

// Navigation bar
extern lv_obj_t *ui_NavPanel;
extern lv_obj_t *ui_NavBtnInput;
extern lv_obj_t *ui_NavBtnTelemetry;
extern lv_obj_t *ui_NavBtnSettings;

// Content area (container for all screens)
extern lv_obj_t *ui_ContentArea;

// Input screen objects
extern lv_obj_t *ui_InputScreen;
extern lv_obj_t *ui_GimbalPanel;
extern lv_obj_t *ui_GimbalLeft;
extern lv_obj_t *ui_GimbalLeftDot;
extern lv_obj_t *ui_GimbalRight;
extern lv_obj_t *ui_GimbalRightDot;
extern lv_obj_t *ui_NavPage;
extern lv_obj_t *ui_NavSwitchIndicators[2][5]; // [NavIndex][Direction] (0=U, 1=D, 2=L, 3=R, 4=C)
extern lv_obj_t *ui_InputControlsPanel;
extern lv_obj_t *ui_Buttons[4];
extern lv_obj_t *ui_BtnLabels[4];
extern lv_obj_t *ui_Switches[6];
extern lv_obj_t *ui_SwitchLabels[6];
extern lv_obj_t *ui_Toggle3Panels[2];
extern lv_obj_t *ui_Toggle3Indicators[2];
extern lv_obj_t *ui_Toggle3Labels[2];
extern lv_obj_t *ui_PotBars[2];
extern lv_obj_t *ui_PotLabels[2];
extern lv_obj_t *ui_PotValues[2];
extern lv_obj_t *ui_EncPanels[2];
extern lv_obj_t *ui_EncLabels[2];
extern lv_obj_t *ui_EncValues[2];

// Telemetry screen objects
extern lv_obj_t *ui_TelemetryScreen;
extern lv_obj_t *ui_TelemetryPanel1;  // Robot data
extern lv_obj_t *ui_TelemetryPanel2;  // Additional data

// Settings screen objects
extern lv_obj_t *ui_SettingsScreen;

//=============================================================================
// Initialization Functions
//=============================================================================

void ui_custom_init(void);
void ui_custom_destroy(void);

//=============================================================================
// Navigation Functions
//=============================================================================

void ui_show_screen(ScreenType_t screen);
ScreenType_t ui_get_current_screen(void);

//=============================================================================
// Input Screen Functions
//=============================================================================

void ui_set_gimbal_left(int16_t x, int16_t y);
void ui_set_gimbal_right(int16_t x, int16_t y);
void ui_set_nav_switch(uint8_t index, bool up, bool down, bool left, bool right, bool center);
void ui_set_button(uint8_t index, bool pressed);
void ui_set_switch(uint8_t index, bool on);
void ui_set_toggle3(uint8_t index, uint8_t position);
void ui_set_pot(uint8_t index, int16_t value);
void ui_set_encoder(uint8_t index, int32_t value);

//=============================================================================
// Telemetry Functions
//=============================================================================

void ui_set_telemetry_value(uint8_t index, const char *label, const char *value);

//=============================================================================
// Header Functions
//=============================================================================

void ui_set_battery(uint8_t percent, BatteryState_t state);
void ui_set_status(bool connected);

//=============================================================================
// Update Function
//=============================================================================

void ui_custom_update(void);

#ifdef __cplusplus
}
#endif

#endif // UI_CUSTOM_H
