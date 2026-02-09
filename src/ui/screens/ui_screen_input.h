#ifndef UI_SCREEN_INPUT_H
#define UI_SCREEN_INPUT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

extern lv_obj_t *ui_InputScreen;
extern lv_obj_t *ui_GimbalPanel;
extern lv_obj_t *ui_GimbalLeft;
extern lv_obj_t *ui_GimbalLeftDot;
extern lv_obj_t *ui_GimbalRight;
extern lv_obj_t *ui_GimbalRightDot;
extern lv_obj_t *ui_NavPage;
extern lv_obj_t *ui_NavSwitchIndicators[2][5]; 
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

void ui_create_input_screen(lv_obj_t *parent);

// Configures the input screen elements
void ui_set_gimbal_left(int16_t x, int16_t y);
void ui_set_gimbal_right(int16_t x, int16_t y);
void ui_set_nav_switch(uint8_t index, bool up, bool down, bool left, bool right, bool center);
void ui_set_button(uint8_t index, bool pressed);
void ui_set_switch(uint8_t index, bool on);
void ui_set_toggle3(uint8_t index, uint8_t position);
void ui_set_pot(uint8_t index, int16_t value);
void ui_set_encoder(uint8_t index, int32_t value);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_INPUT_H
