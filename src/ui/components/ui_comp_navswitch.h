#ifndef UI_COMP_NAVSWITCH_H
#define UI_COMP_NAVSWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

// Creates a navigation switch widget
// populates indicators array with the 5 button objects
lv_obj_t* ui_create_nav_switch_widget(lv_obj_t *parent, lv_obj_t **indicators);

// Updates the state of the nav switch
void ui_update_nav_switch(lv_obj_t **indicators, bool up, bool down, bool left, bool right, bool center);

#ifdef __cplusplus
}
#endif

#endif // UI_COMP_NAVSWITCH_H
