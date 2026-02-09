#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_helpers.h"
#include "ui_styles.h"
#include "components/ui_chrome.h"
#include "screens/ui_screen_input.h"
#include "screens/ui_screen_telemetry.h"
#include "screens/ui_screen_settings.h"

extern lv_obj_t *ui_MainScreen;
extern lv_obj_t *ui_ContentArea;

void ui_init(void);
void ui_destroy(void);
void ui_show_screen(int screen_index);
int ui_get_current_screen(void);
void ui_update(void);

#ifdef __cplusplus
}
#endif

#endif // UI_H
