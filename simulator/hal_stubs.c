/**
 * @file hal_stubs.c
 * Hardware Abstraction Layer stubs for LVGL Simulator
 * Provides dummy implementations of hardware-specific functions
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * This file provides stubs for any ESP32-specific functions
 * that might be called from the UI code.
 * 
 * If you add new hardware-dependent features to the UI,
 * add corresponding stubs here for the simulator.
 */

/* InputManager stubs (if referenced from UI) */
#ifdef INPUT_MANAGER_H
int InputManager_GetGimbalValue(int axis) {
    /* Return centered position for simulator */
    return 2048;
}

bool InputManager_GetButtonState(int button) {
    return false;
}

bool InputManager_GetSwitchState(int sw) {
    return false;
}

int InputManager_GetToggle3State(int toggle) {
    return 1; /* middle position */
}

int InputManager_GetPotValue(int pot) {
    return 2048;
}

int InputManager_GetEncoderValue(int encoder) {
    return 0;
}
#endif

/* Any Arduino-specific functions that might leak through */
#ifdef ARDUINO
unsigned long millis(void) {
    /* SDL provides timing - this is a fallback */
    return 0;
}

void delay(unsigned long ms) {
    /* Do nothing in simulator */
    (void)ms;
}
#endif

/* Printf redirect for debugging (already supported on desktop) */

/* UI Integration stubs - these are in ui_custom_integration.cpp on ESP32 */
void ui_apply_gimbal_calibration(uint8_t axis, int16_t min_val, int16_t center_val, 
                                  int16_t max_val, int16_t deadzone, bool inverted)
{
    printf("Simulator: Apply gimbal calibration axis=%d min=%d center=%d max=%d dz=%d inv=%d\n",
           axis, min_val, center_val, max_val, deadzone, inverted);
    /* In simulator, we don't have a real driver to apply calibration to */
}

void ui_load_gimbal_calibrations(void)
{
    printf("Simulator: Load gimbal calibrations (stub)\n");
    /* In simulator, calibrations don't persist anyway */
}