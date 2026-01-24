/**
 * @file ui_custom_integration.h
 * @brief Integration helpers for custom UI with InputManager
 * 
 * This file provides the glue code to connect the custom UI
 * with the InputManager for real-time input visualization.
 */

#ifndef UI_CUSTOM_INTEGRATION_H
#define UI_CUSTOM_INTEGRATION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ui_custom.h"

/**
 * @brief Update all UI elements from InputManager data
 * 
 * Call this function periodically in your main loop after Input_Update()
 * to keep the UI synchronized with the actual input states.
 * 
 * Example usage in main.cpp:
 * @code
 * void loop() {
 *     Lvgl_Loop();
 *     ui_update_from_inputs();  // Add this line
 *     vTaskDelay(pdMS_TO_TICKS(5));
 * }
 * @endcode
 */
void ui_update_from_inputs(void);

/**
 * @brief Initialize custom UI and switch to it
 * 
 * Call this instead of ui_init() in setup() to use the custom UI.
 * 
 * Example usage in main.cpp:
 * @code
 * void setup() {
 *     // ... other init code ...
 *     Lvgl_Init();
 *     ui_custom_init();  // Use this instead of ui_init()
 *     // ...
 * }
 * @endcode
 */
// Note: ui_custom_init() is already defined in ui_custom.h

#ifdef __cplusplus
}
#endif

#endif // UI_CUSTOM_INTEGRATION_H
