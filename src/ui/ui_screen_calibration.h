/**
 * @file ui_screen_calibration.h
 * @brief Calibration Screens Header
 * 
 * Contains:
 * - Touch calibration (4-point)
 * - Gimbal calibration
 */

#ifndef UI_SCREEN_CALIBRATION_H
#define UI_SCREEN_CALIBRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_common.h"

//=============================================================================
// Calibration Types
//=============================================================================

typedef enum {
    CALIB_TYPE_TOUCH,
    CALIB_TYPE_GIMBAL
} CalibType_t;

typedef enum {
    TOUCH_CALIB_POINT_TOP_LEFT,
    TOUCH_CALIB_POINT_TOP_RIGHT,
    TOUCH_CALIB_POINT_BOTTOM_RIGHT,
    TOUCH_CALIB_POINT_BOTTOM_LEFT,
    TOUCH_CALIB_COMPLETE
} TouchCalibPoint_t;

typedef enum {
    GIMBAL_CALIB_IDLE,
    GIMBAL_CALIB_CENTER,
    GIMBAL_CALIB_MIN_MAX,
    GIMBAL_CALIB_COMPLETE
} GimbalCalibState_t;

//=============================================================================
// Callback Types
//=============================================================================

/**
 * @brief Callback for touch point captured
 * @param point Which calibration point was touched
 * @param x Raw X coordinate
 * @param y Raw Y coordinate
 */
typedef void (*touch_calib_point_cb_t)(TouchCalibPoint_t point, int16_t x, int16_t y);

/**
 * @brief Callback for gimbal calibration values
 * @param gimbal_id Gimbal ID (0-3)
 * @param min_val Minimum raw value
 * @param center_val Center raw value
 * @param max_val Maximum raw value
 */
typedef void (*gimbal_calib_cb_t)(uint8_t gimbal_id, int16_t min_val, int16_t center_val, int16_t max_val);

/**
 * @brief Callback for calibration complete
 * @param type Which calibration was completed
 * @param success True if calibration was successful
 */
typedef void (*calib_complete_cb_t)(CalibType_t type, bool success);

//=============================================================================
// Screen Objects
//=============================================================================

extern lv_obj_t *ui_TouchCalibScreen;
extern lv_obj_t *ui_GimbalCalibScreen;

//=============================================================================
// Touch Calibration Functions
//=============================================================================

/**
 * @brief Create the touch calibration screen
 * @param parent Parent object
 */
void ui_touch_calib_screen_create(lv_obj_t *parent);

/**
 * @brief Show or hide touch calibration screen
 */
void ui_touch_calib_screen_show(bool show);

/**
 * @brief Start touch calibration process
 */
void ui_touch_calib_start(void);

/**
 * @brief Report a touch point for calibration
 * @param x Raw touch X
 * @param y Raw touch Y
 */
void ui_touch_calib_report_touch(int16_t x, int16_t y);

/**
 * @brief Get current calibration point being requested
 */
TouchCalibPoint_t ui_touch_calib_get_current_point(void);

/**
 * @brief Set callback for calibration point captured
 */
void ui_touch_calib_set_point_callback(touch_calib_point_cb_t cb);

/**
 * @brief Set callback for calibration complete
 */
void ui_touch_calib_set_complete_callback(calib_complete_cb_t cb);

//=============================================================================
// Gimbal Calibration Functions
//=============================================================================

/**
 * @brief Create the gimbal calibration screen
 * @param parent Parent object
 */
void ui_gimbal_calib_screen_create(lv_obj_t *parent);

/**
 * @brief Show or hide gimbal calibration screen
 */
void ui_gimbal_calib_screen_show(bool show);

/**
 * @brief Start gimbal calibration for specific gimbal
 * @param gimbal_id Which gimbal to calibrate (0-3, or 0xFF for all)
 */
void ui_gimbal_calib_start(uint8_t gimbal_id);

/**
 * @brief Update gimbal calibration with current raw values
 * @param gimbal_id Gimbal being calibrated
 * @param raw_x Current raw X axis value
 * @param raw_y Current raw Y axis value
 */
void ui_gimbal_calib_update(uint8_t gimbal_id, int16_t raw_x, int16_t raw_y);

/**
 * @brief Confirm current calibration step
 */
void ui_gimbal_calib_confirm_step(void);

/**
 * @brief Cancel gimbal calibration
 */
void ui_gimbal_calib_cancel(void);

/**
 * @brief Get current calibration state
 */
GimbalCalibState_t ui_gimbal_calib_get_state(void);

/**
 * @brief Set callback for gimbal calibration values
 */
void ui_gimbal_calib_set_callback(gimbal_calib_cb_t cb);

/**
 * @brief Set callback for calibration complete
 */
void ui_gimbal_calib_set_complete_callback(calib_complete_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_CALIBRATION_H
