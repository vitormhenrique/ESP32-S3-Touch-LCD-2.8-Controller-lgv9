/**
 * @file ui_screen_input.h
 * @brief Input Screen - Displays gimbals, nav switches, encoders, buttons
 */

#ifndef UI_SCREEN_INPUT_H
#define UI_SCREEN_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_common.h"

//=============================================================================
// Input Screen Objects
//=============================================================================

extern lv_obj_t *ui_InputScreen;

// Gimbal displays
extern lv_obj_t *ui_GimbalLeft;
extern lv_obj_t *ui_GimbalLeftDot;
extern lv_obj_t *ui_GimbalRight;
extern lv_obj_t *ui_GimbalRightDot;

// Navigation switch displays (5 buttons each: up, down, left, right, center)
extern lv_obj_t *ui_NavSwitch1Btns[5];
extern lv_obj_t *ui_NavSwitch2Btns[5];

// Encoder displays
extern lv_obj_t *ui_Encoder1Value;
extern lv_obj_t *ui_Encoder2Value;

// Button displays
extern lv_obj_t *ui_Button1;
extern lv_obj_t *ui_Button2;

// Potentiometer displays
extern lv_obj_t *ui_Pot1Bar;
extern lv_obj_t *ui_Pot1Value;
extern lv_obj_t *ui_Pot2Bar;
extern lv_obj_t *ui_Pot2Value;

//=============================================================================
// Functions
//=============================================================================

/**
 * Create the input screen
 */
void ui_input_screen_create(lv_obj_t *parent);

/**
 * Show/hide the input screen
 */
void ui_input_screen_show(bool show);

/**
 * Update gimbal positions
 */
void ui_input_set_gimbal(uint8_t index, int16_t x, int16_t y);

/**
 * Update navigation switch state
 */
void ui_input_set_nav_switch(uint8_t index, bool up, bool down, bool left, bool right, bool center);

/**
 * Update encoder display
 */
void ui_input_set_encoder(uint8_t index, int32_t value);

/**
 * Update button state
 */
void ui_input_set_button(uint8_t index, bool pressed);

/**
 * Update potentiometer value
 */
void ui_input_set_pot(uint8_t index, int16_t value);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_INPUT_H
