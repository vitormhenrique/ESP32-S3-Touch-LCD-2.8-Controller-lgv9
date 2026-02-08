/**
 * @file ui_common.h
 * @brief Common UI definitions, styles, and utilities
 * 
 * Shared components for all UI screens
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

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

#define UI_GIMBAL_SIZE          70       // Gimbal display size
#define UI_GIMBAL_DOT_SIZE      14       // Gimbal position dot size
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
#define ICON_UP           LV_SYMBOL_UP
#define ICON_DOWN         LV_SYMBOL_DOWN
#define ICON_LEFT         LV_SYMBOL_LEFT
#define ICON_RIGHT        LV_SYMBOL_RIGHT
#define ICON_OK           LV_SYMBOL_OK
#define ICON_CLOSE        LV_SYMBOL_CLOSE
#define ICON_REFRESH      LV_SYMBOL_REFRESH
#define ICON_HOME         LV_SYMBOL_HOME
#define ICON_LIST         LV_SYMBOL_LIST
#define ICON_DRIVE        LV_SYMBOL_DRIVE

//=============================================================================
// Screen Types
//=============================================================================

typedef enum {
    SCREEN_INPUT = 0,
    SCREEN_TELEMETRY,
    SCREEN_CONFIG,
    SCREEN_ROBOT_SELECT,
    SCREEN_GIMBAL_CALIBRATION,
    SCREEN_TOUCH_CALIBRATION,
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
// Style Structures
//=============================================================================

typedef struct {
    lv_style_t panel;
    lv_style_t card;
    lv_style_t gimbal_bg;
    lv_style_t gimbal_dot;
    lv_style_t switch_off;
    lv_style_t switch_on;
    lv_style_t btn_default;
    lv_style_t btn_pressed;
    lv_style_t btn_highlight;
    lv_style_t toggle3_bg;
    lv_style_t toggle3_indicator;
    lv_style_t bar_bg;
    lv_style_t bar_indicator;
    lv_style_t text_primary;
    lv_style_t text_secondary;
    lv_style_t text_small;
    lv_style_t nav_active;
    lv_style_t nav_inactive;
    lv_style_t nav_btn_up;
    lv_style_t nav_btn_down;
    lv_style_t nav_btn_left;
    lv_style_t nav_btn_right;
    lv_style_t nav_btn_center;
} UI_Styles_t;

//=============================================================================
// Global Style Access
//=============================================================================

extern UI_Styles_t ui_styles;

//=============================================================================
// Style Functions
//=============================================================================

/**
 * Initialize all UI styles
 */
void ui_styles_init(void);

/**
 * Check if styles are initialized
 */
bool ui_styles_ready(void);

//=============================================================================
// Common Widget Creation Functions
//=============================================================================

/**
 * Create a standard panel
 */
lv_obj_t* ui_create_panel(lv_obj_t *parent, int32_t width, int32_t height);

/**
 * Create a card (elevated panel)
 */
lv_obj_t* ui_create_card(lv_obj_t *parent, int32_t width, int32_t height);

/**
 * Create a label with primary text style
 */
lv_obj_t* ui_create_label(lv_obj_t *parent, const char *text);

/**
 * Create a label with secondary text style
 */
lv_obj_t* ui_create_label_secondary(lv_obj_t *parent, const char *text);

/**
 * Create a small label
 */
lv_obj_t* ui_create_label_small(lv_obj_t *parent, const char *text);

/**
 * Create a standard button
 */
lv_obj_t* ui_create_button(lv_obj_t *parent, const char *text, int32_t width, int32_t height);

/**
 * Create a gimbal widget
 * @param parent Parent object
 * @param dot_out Pointer to store the dot object
 * @param label Label text for the gimbal
 * @return The gimbal container object
 */
lv_obj_t* ui_create_gimbal(lv_obj_t *parent, lv_obj_t **dot_out, const char *label);

/**
 * Create a navigation switch widget (5-way display)
 * @param parent Parent object
 * @param buttons_out Array of 5 button objects (up, down, left, right, center)
 * @param label Label text
 * @return The nav switch container
 */
lv_obj_t* ui_create_nav_switch_widget(lv_obj_t *parent, lv_obj_t **buttons_out, const char *label);

/**
 * Create a horizontal scrollable container with page snap
 */
lv_obj_t* ui_create_scroll_container(lv_obj_t *parent, int32_t width, int32_t height, uint8_t num_pages);

/**
 * Create a title bar for a screen
 */
lv_obj_t* ui_create_title_bar(lv_obj_t *parent, const char *title);

#ifdef __cplusplus
}
#endif

#endif // UI_COMMON_H
