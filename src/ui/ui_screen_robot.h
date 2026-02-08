/**
 * @file ui_screen_robot.h
 * @brief Robot Selection Screen Header
 * 
 * Allows user to select robot profile:
 * - Generic Robot
 * - Hexapod (with DYNAMIXEL MX-28AR servos)
 */

#ifndef UI_SCREEN_ROBOT_H
#define UI_SCREEN_ROBOT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_common.h"

//=============================================================================
// Robot Types
//=============================================================================

typedef enum {
    ROBOT_TYPE_GENERIC = 0,
    ROBOT_TYPE_HEXAPOD = 1,
    ROBOT_TYPE_COUNT
} RobotType_t;

//=============================================================================
// Callback Types
//=============================================================================

/**
 * @brief Callback for robot selection change
 * @param robot_type Selected robot type
 */
typedef void (*robot_select_cb_t)(RobotType_t robot_type);

//=============================================================================
// Screen Objects
//=============================================================================

extern lv_obj_t *ui_RobotSelectScreen;

//=============================================================================
// Functions
//=============================================================================

/**
 * @brief Create the robot selection screen
 * @param parent Parent object
 */
void ui_robot_screen_create(lv_obj_t *parent);

/**
 * @brief Show or hide robot selection screen
 */
void ui_robot_screen_show(bool show);

/**
 * @brief Set the currently selected robot type
 * @param robot_type Robot type to select
 */
void ui_robot_set_selected(RobotType_t robot_type);

/**
 * @brief Get the currently selected robot type
 */
RobotType_t ui_robot_get_selected(void);

/**
 * @brief Set callback for robot selection
 */
void ui_robot_set_callback(robot_select_cb_t cb);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_ROBOT_H
