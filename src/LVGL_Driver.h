#pragma once

#include <lvgl.h>
#include "lv_conf.h"
#include <demos/lv_demos.h>
#include <esp_heap_caps.h>
#include "Display_ST7789.h"
#include "Touch_CST328.h"

/* LVGL dimensions - use dynamic values from LCD driver */
#define LVGL_BUF_LEN  (LCD_WIDTH * LCD_HEIGHT / 10)

#define EXAMPLE_LVGL_TICK_PERIOD_MS  2


void Lvgl_print(const char * buf);
void Lvgl_Display_LCD(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map); // Displays LVGL content on the LCD
void Lvgl_Touchpad_Read(lv_indev_t * indev, lv_indev_data_t * data);                // Read the touchpad
void Lvgl_Encoder_Read(lv_indev_t * indev, lv_indev_data_t * data);                 // Read encoder
void example_increase_lvgl_tick(void *arg);

void Lvgl_Init(void);                                   // Initialize with default rotation (0°)
void Lvgl_InitWithRotation(lcd_rotation_t rotation);    // Initialize with specific hardware rotation
void Lvgl_SetRotation(lcd_rotation_t rotation);         // Change rotation at runtime
uint32_t Lvgl_Loop(void);                               // Returns time until next handler call (0 = rendered)

// Encoder input device
lv_indev_t* Lvgl_GetEncoderIndev(void);                 // Get encoder input device
lv_group_t* Lvgl_GetDefaultGroup(void);                 // Get default group for encoder navigation
