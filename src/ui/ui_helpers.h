#ifndef UI_HELPERS_H
#define UI_HELPERS_H

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
// Icon Symbols
//=============================================================================

#define ICON_WIFI_ON      LV_SYMBOL_WIFI
#define ICON_WIFI_OFF     LV_SYMBOL_WARNING
#define ICON_BATTERY_FULL LV_SYMBOL_BATTERY_FULL
#define ICON_BATTERY_3    LV_SYMBOL_BATTERY_3
#define ICON_BATTERY_2    LV_SYMBOL_BATTERY_2
#define ICON_BATTERY_1    LV_SYMBOL_BATTERY_1
#define ICON_BATTERY_EMPTY LV_SYMBOL_BATTERY_EMPTY
#define ICON_CHARGE       LV_SYMBOL_CHARGE
#define ICON_INPUT        LV_SYMBOL_EDIT
#define ICON_TELEMETRY    LV_SYMBOL_DOWNLOAD
#define ICON_SETTINGS     LV_SYMBOL_SETTINGS

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

// Screen dimensions
#define UI_SCREEN_WIDTH         320
#define UI_SCREEN_HEIGHT        240

// Layout heights
#define UI_HEADER_HEIGHT        24
#define UI_NAV_HEIGHT           30
#define UI_CONTENT_HEIGHT       (UI_SCREEN_HEIGHT - UI_HEADER_HEIGHT - UI_NAV_HEIGHT)
#define INPUT_PANEL_HEIGHT      60
#define GIMBAL_PANEL_HEIGHT     (UI_CONTENT_HEIGHT - INPUT_PANEL_HEIGHT)

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

#ifdef __cplusplus
}
#endif

#endif // UI_HELPERS_H
