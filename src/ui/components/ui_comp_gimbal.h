#ifndef UI_COMP_GIMBAL_H
#define UI_COMP_GIMBAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Creates a gimbal widget and returns the container
// Outputs the dot object via dot_out parameter
lv_obj_t* ui_create_gimbal_widget(lv_obj_t *parent, lv_obj_t **dot_out);

// Updates the position of the gimbal dot
void ui_update_gimbal_dot(lv_obj_t *dot, int16_t x, int16_t y);

#ifdef __cplusplus
}
#endif

#endif // UI_COMP_GIMBAL_H
