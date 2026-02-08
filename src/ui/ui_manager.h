/**
 * @file ui_manager.h
 * @brief Main UI Manager - Controls screens and navigation
 */

#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_common.h"
#include "ui_screen_robot.h"

//=============================================================================
// Screen Types
//=============================================================================

typedef enum {
    UI_SCREEN_INPUT = 0,
    UI_SCREEN_TELEMETRY,
    UI_SCREEN_CONFIG,
    UI_SCREEN_COUNT
} UIScreen_t;

//=============================================================================
// Callback Types
//=============================================================================

typedef void (*ui_screen_change_cb_t)(UIScreen_t screen);

//=============================================================================
// Main Screen Objects
//=============================================================================

extern lv_obj_t *ui_MainScreen;

//=============================================================================
// Initialization Functions
//=============================================================================

/**
 * Initialize the complete UI system
 */
void ui_init(void);

/**
 * Destroy all UI elements
 */
void ui_destroy(void);

/**
 * Set up default callbacks
 */
void ui_setup_callbacks(void);

//=============================================================================
// Navigation Functions
//=============================================================================

/**
 * Show a specific screen
 * @param screen Screen to show
 */
void ui_show_screen(UIScreen_t screen);

/**
 * Get current active screen
 */
UIScreen_t ui_get_current_screen(void);

/**
 * Set callback for screen changes
 */
void ui_set_screen_change_callback(ui_screen_change_cb_t cb);

//=============================================================================
// Style Access
//=============================================================================

/**
 * Get global UI styles
 */
UI_Styles_t *ui_get_styles(void);

//=============================================================================
// Update Function
//=============================================================================

/**
 * Update all UI elements (call in main loop)
 */
void ui_update(void);

//=============================================================================
// Weak Callbacks (Override in main application)
//=============================================================================

/**
 * Called when robot type is selected
 * @param robot_type Selected robot type
 */
void ui_on_robot_selected(RobotType_t robot_type);

/**
 * Called when gimbal calibration completes
 */
void ui_on_gimbal_calibrated(uint8_t gimbal_id, int16_t min_val, int16_t center_val, int16_t max_val);

/**
 * Called when touch calibration completes
 */
void ui_on_touch_calibrated(int16_t x1, int16_t y1, int16_t x2, int16_t y2, 
                            int16_t x3, int16_t y3, int16_t x4, int16_t y4);

#ifdef __cplusplus
}
#endif

#endif // UI_MANAGER_H
