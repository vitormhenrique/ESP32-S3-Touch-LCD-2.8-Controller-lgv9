/**
 * @file lv_drv_sdl.c
 * SDL2 Display and Input Driver for LVGL 9
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include "lvgl.h"

/* Screen resolution */
#ifndef DISP_HOR_RES
#define DISP_HOR_RES 320
#endif
#ifndef DISP_VER_RES
#define DISP_VER_RES 240
#endif

/* Window scale factor for better visibility on desktop */
#define WINDOW_SCALE 2

/* Static variables */
static SDL_Window *window = NULL;
static SDL_Renderer *renderer = NULL;
static SDL_Texture *texture = NULL;
static lv_display_t *display = NULL;
static lv_indev_t *mouse_indev = NULL;

static bool quit_requested = false;
static int32_t mouse_x = 0;
static int32_t mouse_y = 0;
static bool mouse_pressed = false;

/* Display flush callback */
static void sdl_display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    int32_t w = lv_area_get_width(area);
    int32_t h = lv_area_get_height(area);
    
    /* Update texture with new pixel data */
    SDL_Rect rect;
    rect.x = area->x1;
    rect.y = area->y1;
    rect.w = w;
    rect.h = h;
    
    /* LVGL uses RGB565 format */
    SDL_UpdateTexture(texture, &rect, px_map, w * 2);
    
    /* Copy texture to renderer and present */
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
    
    lv_display_flush_ready(disp);
}

/* Mouse read callback */
static void sdl_mouse_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    
    data->point.x = mouse_x;
    data->point.y = mouse_y;
    data->state = mouse_pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

/* Initialize SDL display */
void lv_sdl_display_init(void)
{
    /* Create window */
    window = SDL_CreateWindow(
        "LVGL Simulator - RC Remote",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        DISP_HOR_RES * WINDOW_SCALE,
        DISP_VER_RES * WINDOW_SCALE,
        SDL_WINDOW_SHOWN
    );
    
    if (!window) {
        fprintf(stderr, "Error: Failed to create SDL window: %s\n", SDL_GetError());
        exit(1);
    }
    
    /* Create renderer */
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        fprintf(stderr, "Error: Failed to create SDL renderer: %s\n", SDL_GetError());
        exit(1);
    }
    
    /* Set render scale */
    SDL_RenderSetScale(renderer, WINDOW_SCALE, WINDOW_SCALE);
    
    /* Create texture for LVGL rendering (RGB565 format) */
    texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_RGB565,
        SDL_TEXTUREACCESS_STREAMING,
        DISP_HOR_RES,
        DISP_VER_RES
    );
    
    if (!texture) {
        fprintf(stderr, "Error: Failed to create SDL texture: %s\n", SDL_GetError());
        exit(1);
    }
    
    /* Create LVGL display */
    display = lv_display_create(DISP_HOR_RES, DISP_VER_RES);
    if (!display) {
        fprintf(stderr, "Error: Failed to create LVGL display\n");
        exit(1);
    }
    
    /* Allocate draw buffers */
    static uint8_t buf1[DISP_HOR_RES * DISP_VER_RES * 2];
    lv_display_set_buffers(display, buf1, NULL, sizeof(buf1), LV_DISPLAY_RENDER_MODE_FULL);
    
    /* Set flush callback */
    lv_display_set_flush_cb(display, sdl_display_flush);
    
    /* Set color format */
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    
    printf("SDL display initialized: %dx%d (window: %dx%d)\n", 
           DISP_HOR_RES, DISP_VER_RES, 
           DISP_HOR_RES * WINDOW_SCALE, DISP_VER_RES * WINDOW_SCALE);
}

/* Initialize SDL mouse as touch input */
void lv_sdl_mouse_init(void)
{
    mouse_indev = lv_indev_create();
    lv_indev_set_type(mouse_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(mouse_indev, sdl_mouse_read);
    
    printf("SDL mouse input initialized (simulates touch)\n");
}

/* Poll SDL events */
void lv_sdl_poll_events(void)
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                quit_requested = true;
                break;
                
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    quit_requested = true;
                }
                break;
                
            case SDL_MOUSEMOTION:
                /* Scale mouse coordinates back to LVGL resolution */
                mouse_x = event.motion.x / WINDOW_SCALE;
                mouse_y = event.motion.y / WINDOW_SCALE;
                
                /* Clamp to display bounds */
                if (mouse_x < 0) mouse_x = 0;
                if (mouse_x >= DISP_HOR_RES) mouse_x = DISP_HOR_RES - 1;
                if (mouse_y < 0) mouse_y = 0;
                if (mouse_y >= DISP_VER_RES) mouse_y = DISP_VER_RES - 1;
                break;
                
            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouse_pressed = true;
                }
                break;
                
            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT) {
                    mouse_pressed = false;
                }
                break;
                
            default:
                break;
        }
    }
}

/* Check if quit was requested */
bool lv_sdl_should_quit(void)
{
    return quit_requested;
}
