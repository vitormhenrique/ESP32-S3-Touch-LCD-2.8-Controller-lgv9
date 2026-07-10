#pragma once
/******************************************************************************
 * UI Input Simulation (compile-time option)
 *
 * When built with -DUI_INPUT_SIM=1, the touch UI becomes the input source
 * instead of the physical gimbals/switches/buttons:
 *   - Tap & drag inside a gimbal widget to move the stick (springs back to
 *     center on release).
 *   - Tap a button widget for a momentary press.
 *   - Tap a switch widget to toggle it on/off.
 *   - Tap a 3-position toggle widget to cycle UP -> CENTER -> DOWN.
 *
 * Channels not exposed in the UI (pots, encoders, nav switches) send their
 * default/neutral values.
 ******************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#ifndef UI_INPUT_SIM
#define UI_INPUT_SIM 0
#endif

#if UI_INPUT_SIM

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int16_t gimbal[4];   // -1000..+1000, order LX, LY, RX, RY (default 0)
    bool    sw[6];       // SW_A..SW_H two-position switches (default off)
    bool    btn[4];      // BTN_1..BTN_4 momentary buttons (default released)
    uint8_t toggle3[2];  // SW_E/SW_F, 0=UP 1=CENTER 2=DOWN (default center)
} input_sim_state_t;

// Read-only access to the current simulated state
const input_sim_state_t* input_sim_get(void);

// Mutators used by the UI event handlers
void input_sim_set_gimbal(uint8_t axis, int16_t value);  // clamped -1000..1000
void input_sim_set_button(uint8_t index, bool pressed);
void input_sim_toggle_switch(uint8_t index);
void input_sim_cycle_toggle3(uint8_t index);

#ifdef __cplusplus
}
#endif

#endif // UI_INPUT_SIM
