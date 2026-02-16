/*****************************************************************************
  | File        :   LVGL_Driver.c
  
  | help        : 
    The provided LVGL library file must be installed first
    
  | Rotation    :
    This driver uses HARDWARE ROTATION via the ST7789 MADCTL register.
    - LCD_SetRotation() sets the hardware rotation
    - LVGL display is created with rotated dimensions
    - Touch coordinates are transformed to match display rotation
******************************************************************************/
#include "LVGL_Driver.h"
#include "Encoder_Driver.h"
#include "MCP23017_Driver.h"

static uint8_t buf1[LVGL_BUF_LEN * sizeof(lv_color_t)];
static uint8_t buf2[LVGL_BUF_LEN * sizeof(lv_color_t)];

/* Store display pointer for rotation updates */
static lv_display_t *lvgl_display = NULL;
static lv_indev_t *lvgl_indev = NULL;
static lv_indev_t *lvgl_encoder_indev = NULL;
static lv_group_t *lvgl_default_group = NULL;

/* Encoder button pin - using NAV2_C (center) as encoder button */
#define ENCODER_BUTTON_INDEX  20  // NAV2_C in SWITCH_CONFIGS

/* Serial debugging */
void Lvgl_print(const char * buf)
{
    // Serial.printf(buf);
    // Serial.flush();
}

/*  Display flushing 
    Displays LVGL content on the LCD
    This function implements associating LVGL data to the LCD screen
*/
void Lvgl_Display_LCD(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t *)px_map);
  lv_display_flush_ready(disp);
}

/*Read the touchpad*/
void Lvgl_Touchpad_Read(lv_indev_t * indev, lv_indev_data_t * data)
{
  uint16_t touchpad_x[5] = {0};
  uint16_t touchpad_y[5] = {0};
  uint16_t strength[5]   = {0};
  uint8_t touchpad_cnt = 0;
  Touch_Read_Data();
  uint8_t touchpad_pressed = Touch_Get_XY(touchpad_x, touchpad_y, strength, &touchpad_cnt, CST328_LCD_TOUCH_MAX_POINTS);
  if (touchpad_pressed && touchpad_cnt > 0) {
    uint16_t x = touchpad_x[0];
    uint16_t y = touchpad_y[0];
    
    /* Transform touch coordinates to match hardware rotation */
    Touch_TransformCoordinates(&x, &y);
    
    data->point.x = x;
    data->point.y = y;
    data->state = LV_INDEV_STATE_PRESSED;
    // printf("LVGL  : X=%u Y=%u (raw: %u,%u) num=%d\r\n", x, y, touchpad_x[0], touchpad_y[0], touchpad_cnt);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

/*Read the encoder*/
void Lvgl_Encoder_Read(lv_indev_t * indev, lv_indev_data_t * data)
{
  // Get accelerated encoder delta (velocity-based) for natural feel
  int32_t delta = EncoderInput.getAcceleratedDelta(ENCODER_1);
  data->enc_diff = delta;
  
  // Check encoder button (using NAV2 center button)
  if (SwitchInput.getSwitchState(ENCODER_BUTTON_INDEX) == SWITCH_ON) {
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void example_increase_lvgl_tick(void *arg)
{
    /* Tell LVGL how many milliseconds has elapsed */
    lv_tick_inc(EXAMPLE_LVGL_TICK_PERIOD_MS);
}

void Lvgl_Init(void)
{
  Lvgl_InitWithRotation(LCD_ROTATION_90);
}

/******************************************************************************
function: Initialize LVGL with a specific rotation
parameter:
    rotation: LCD_ROTATION_0, LCD_ROTATION_90, LCD_ROTATION_180, LCD_ROTATION_270
    
Note: This uses HARDWARE ROTATION via the ST7789 MADCTL register.
      LVGL display is created with rotated dimensions.
      Touch coordinates are transformed to match the rotation.
******************************************************************************/
void Lvgl_InitWithRotation(lcd_rotation_t rotation)
{
  lv_init();

  /* Set hardware rotation on the display */
  LCD_SetRotation(rotation);
  
  /* Get the rotated dimensions */
  uint16_t display_width = LCD_GetWidth();
  uint16_t display_height = LCD_GetHeight();
  
  /* Initialize the display with rotated dimensions */
  lvgl_display = lv_display_create(display_width, display_height);
  lv_display_set_flush_cb(lvgl_display, Lvgl_Display_LCD);
  lv_display_set_buffers(lvgl_display, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);
  
  /* NOTE: Do NOT call lv_display_set_rotation() - we use hardware rotation instead */

  /*Initialize the input device driver*/
  lvgl_indev = lv_indev_create();
  lv_indev_set_type(lvgl_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(lvgl_indev, Lvgl_Touchpad_Read);

  /*Initialize the encoder input device*/
  lvgl_encoder_indev = lv_indev_create();
  lv_indev_set_type(lvgl_encoder_indev, LV_INDEV_TYPE_ENCODER);
  lv_indev_set_read_cb(lvgl_encoder_indev, Lvgl_Encoder_Read);
  
  /* Create default group for encoder navigation */
  lvgl_default_group = lv_group_create();
  lv_group_set_default(lvgl_default_group);
  lv_indev_set_group(lvgl_encoder_indev, lvgl_default_group);

  const esp_timer_create_args_t lvgl_tick_timer_args = {
    .callback = &example_increase_lvgl_tick,
    .name = "lvgl_tick"
  };
  esp_timer_handle_t lvgl_tick_timer = NULL;
  esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer);
  esp_timer_start_periodic(lvgl_tick_timer, EXAMPLE_LVGL_TICK_PERIOD_MS * 1000);
}

lv_indev_t* Lvgl_GetEncoderIndev(void)
{
  return lvgl_encoder_indev;
}

lv_group_t* Lvgl_GetDefaultGroup(void)
{
  return lvgl_default_group;
}

uint32_t Lvgl_Loop(void)
{
  return lv_timer_handler(); /* let the GUI do its work, returns ms until next call */
}

/******************************************************************************
function: Set display rotation (runtime change)
parameter:
    rotation: LCD_ROTATION_0, LCD_ROTATION_90, LCD_ROTATION_180, LCD_ROTATION_270
    
Note: This changes the hardware rotation and recreates the LVGL display.
      For best results, call this before loading a new screen.
******************************************************************************/
void Lvgl_SetRotation(lcd_rotation_t rotation)
{
  if (lvgl_display == NULL) return;
  
  /* Set hardware rotation */
  LCD_SetRotation(rotation);
  
  /* Update LVGL display resolution to match rotated dimensions */
  uint16_t display_width = LCD_GetWidth();
  uint16_t display_height = LCD_GetHeight();
  lv_display_set_resolution(lvgl_display, display_width, display_height);
  
  /* Force refresh */
  lv_obj_invalidate(lv_scr_act());
}
