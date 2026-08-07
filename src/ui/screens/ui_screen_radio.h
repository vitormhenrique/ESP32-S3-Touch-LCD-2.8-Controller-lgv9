#ifndef UI_SCREEN_RADIO_H
#define UI_SCREEN_RADIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/** Build the dynamic ELRS radio configuration UI inside `parent`
 *  (the Settings -> Radio submenu container). */
void ui_radio_menu_create(lv_obj_t *parent);

/** Call when the Radio submenu becomes visible: starts device discovery
 *  and parameter loading if needed. */
void ui_radio_on_show(void);

#ifdef __cplusplus
}
#endif

#endif // UI_SCREEN_RADIO_H
