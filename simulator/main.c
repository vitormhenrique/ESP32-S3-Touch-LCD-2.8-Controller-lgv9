/**
 * @file main.c
 * LVGL Desktop Simulator - SDL2 based
 * Runs the same UI as the ESP32-S3 microcontroller
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "lvgl.h"
#include "ui/ui.h"
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
    ui_init();
    printf("UI initialized successfully!\n\n");
    
    /* Main loop */
    printf("Running... Press ESC or close window to exit.\n");
    while (!lv_sdl_should_quit()) {
        /* Poll SDL events */
        lv_sdl_poll_events();
        
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
