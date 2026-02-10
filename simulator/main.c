/**
 * @file main.c
 * LVGL Desktop Simulator - SDL2 based
 * Runs the same UI as the ESP32-S3 microcontroller
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "ui/ui.h"
#include "ui/screens/ui_screen_settings.h"
#include "Settings.h"

/* Simulator display/input driver declarations */
void lv_sdl_display_init(void);
void lv_sdl_mouse_init(void);
void lv_sdl_poll_events(void);
bool lv_sdl_should_quit(void);

/* Screen resolution */
#define DISP_HOR_RES 320
#define DISP_VER_RES 240

/* Tick period in ms */
#define TICK_PERIOD_MS 5

static uint32_t get_tick_ms(void)
{
    return SDL_GetTicks();
}

/* Simulate gimbal values for testing */
static void update_simulated_gimbals(void)
{
    /* Generate simulated gimbal values using keyboard control */
    static int16_t gimbal_base[4] = {2048, 2048, 2048, 2048};
    static uint32_t last_auto_update = 0;
    
    /* Check keyboard state for manual control */
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    
    /* If calibration is active, arrow keys control the current axis */
    /* Otherwise, use 1-4 keys to select axis and arrows to adjust */
    uint8_t axis = ui_gimbal_cal_is_active() ? ui_gimbal_cal_get_axis() : 0;
    
    /* Number keys 1-4 select axis when not in calibration */
    if (!ui_gimbal_cal_is_active()) {
        if (keys[SDL_SCANCODE_1]) axis = 0;
        if (keys[SDL_SCANCODE_2]) axis = 1;
        if (keys[SDL_SCANCODE_3]) axis = 2;
        if (keys[SDL_SCANCODE_4]) axis = 3;
    }
    
    if (axis < 4) {
        if (keys[SDL_SCANCODE_UP]) {
            gimbal_base[axis] = (gimbal_base[axis] < 4000) ? gimbal_base[axis] + 50 : 4095;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            gimbal_base[axis] = (gimbal_base[axis] > 95) ? gimbal_base[axis] - 50 : 0;
        }
        /* Quick presets: HOME for center, END for min, PAGEUP for max */
        if (keys[SDL_SCANCODE_HOME]) {
            gimbal_base[axis] = 2048;
        }
        if (keys[SDL_SCANCODE_END]) {
            gimbal_base[axis] = 100;
        }
        if (keys[SDL_SCANCODE_PAGEUP]) {
            gimbal_base[axis] = 3995;
        }
    }
    
    /* Auto-oscillate values slowly when not being controlled */
    uint32_t now = SDL_GetTicks();
    if (now - last_auto_update > 100) {
        last_auto_update = now;
        /* Add slight oscillation to make values look alive */
        for (int i = 0; i < 4; i++) {
            int16_t noise = (rand() % 5) - 2;  /* -2 to +2 */
            gimbal_base[i] = LV_CLAMP(0, gimbal_base[i] + noise, 4095);
        }
    }
    
    int16_t gimbal_values[4];
    for (int i = 0; i < 4; i++) {
        gimbal_values[i] = gimbal_base[i];
    }
    
    ui_gimbal_cal_update(gimbal_values);
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    
    printf("╔════════════════════════════════════════════╗\n");
    printf("║   LVGL Simulator - RC Remote Controller    ║\n");
    printf("║   Resolution: %dx%d                      ║\n", DISP_HOR_RES, DISP_VER_RES);
    printf("║   Mouse = Touch Input, ESC = Quit          ║\n");
    printf("╚════════════════════════════════════════════╝\n\n");
    
    /* Initialize SDL */
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        fprintf(stderr, "Error: Failed to initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    
    /* Initialize LVGL */
    lv_init();
    
    /* Set up tick function */
    lv_tick_set_cb(get_tick_ms);
    
    /* Initialize SDL display driver */
    lv_sdl_display_init();
    
    /* Initialize SDL mouse (touch emulation) */
    lv_sdl_mouse_init();
    
    /* Initialize settings (stub) */
    Settings_Init();
    
    /* Initialize UI */
    printf("Initializing UI...\n");
    fflush(stdout);
    ui_init();
    printf("UI initialized successfully!\n\n");
    fflush(stdout);
    
    /* Force initial screen refresh */
    lv_obj_invalidate(lv_screen_active());
    lv_refr_now(lv_display_get_default());
    
    /* Main loop */
    printf("Running... Press ESC or close window to exit.\n");
    printf("  During gimbal calibration:\n");
    printf("    Arrow UP/DOWN = Adjust gimbal value\n");
    printf("    HOME = Center (2048), END = Min (100), PAGEUP = Max (3995)\n");
    while (!lv_sdl_should_quit()) {
        /* Poll SDL events */
        lv_sdl_poll_events();
        
        /* Update simulated gimbals for calibration */
        update_simulated_gimbals();
        
        /* Handle LVGL tasks */
        uint32_t time_till_next = lv_timer_handler();
        
        /* Sleep to reduce CPU usage */
        if (time_till_next > 0) {
            SDL_Delay(time_till_next < TICK_PERIOD_MS ? time_till_next : TICK_PERIOD_MS);
        }
    }
    
    /* Cleanup */
    printf("\nShutting down...\n");
    ui_destroy();
    SDL_Quit();
    
    printf("Goodbye!\n");
    return 0;
}
