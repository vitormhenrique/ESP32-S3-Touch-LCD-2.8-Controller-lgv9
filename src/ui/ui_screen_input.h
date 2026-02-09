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

// Button displays (B1-B4)
extern lv_obj_t *ui_Button1;
extern lv_obj_t *ui_Button2;
extern lv_obj_t *ui_Button3;
extern lv_obj_t *ui_Button4;

// Switch displays (S1-S6)
extern lv_obj_t *ui_Switch1;
extern lv_obj_t *ui_Switch2;
extern lv_obj_t *ui_Switch3;
extern lv_obj_t *ui_Switch4;
extern lv_obj_t *ui_Switch5;
extern lv_obj_t *ui_Switch6;

// Toggle displays (T1-T2) 3-position
extern lv_obj_t *ui_Toggle1;
extern lv_obj_t *ui_Toggle2;

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
 * @param index 0=Left, 1=Right
 * @param x -100 to 100
 * @param y -100 to 100
 */
void ui_input_set_gimbal(uint8_t index, int16_t x, int16_t y);

/**
 * Update navigation switch state
 * @param index 0=Left, 1=Right
 * @param up, down, left, right, center state
 */
void ui_input_set_nav_switch(uint8_t index, bool up, bool down, bool left, bool right, bool center);

/**
 * Update encoder display
 * @param index 0=E1, 1=E2
 * @param value Current value
 */
void ui_input_set_encoder(uint8_t index, int32_t value);

/**
 * Update button state
 * @param index 0=B1, 1=B2, 2=B3, 3=B4
 * @param pressed true/false
 */
void ui_input_set_button(uint8_t index, bool pressed);

/**
 * Update switch state
 * @param index 0=S1...5=S6
 * @param active true/false (or On/Off)
 */
void ui_input_set_switch(uint8_t index, bool active);

/**
 * Update toggle state
 * @param index 0=T1, 1=T2
 * @param state 0=Top, 1=Middle, 2=Bottom
 */
void ui_input_set_toggle(uint8_t index, uint8_t state);

/**
 * Update potentiometer value
 * @param index 0=P1, 1=P2
 * @param value 0-100 (or raw value)
 */
void ui_input_set_pot(uint8_t index, int16_t value);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_INPUT_H
