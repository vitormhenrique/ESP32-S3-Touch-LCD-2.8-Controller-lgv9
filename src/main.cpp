#include <Arduino.h>
#include "Display_ST7789.h"
#include "Audio_PCM5101.h"
#include "RTC_PCF85063.h"
#include "Gyro_QMI8658.h"
#include "LVGL_Driver.h"
#include "PWR_Key.h"
#include "SD_Card.h"
#include "BAT_Driver.h"
#include "Wireless.h"
#include "InputManager.h"
#include "Settings.h"
#include "ui_custom.h"
#include "ui_custom_integration.h"
#include "PerfMonitor.h"

void DriverTask(void *parameter) {
  Wireless_Test2();
  Input_Init();  // Initialize InputManager
  
  // Load saved gimbal calibrations from Settings
  ui_load_gimbal_calibrations();
  
  while(1){
    Perf_StartSection(PERF_COUNTER_DRIVER_LOOP);
    
    Perf_StartSection(PERF_COUNTER_INPUT_UPDATE);
    Input_Update();  // Update all inputs
    Perf_EndSection(PERF_COUNTER_INPUT_UPDATE);
    
    PWR_Loop();
    BAT_Get_Volts();
    PCF85063_Loop();
    QMI8658_Loop(); 
    
    Perf_EndSection(PERF_COUNTER_DRIVER_LOOP);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void Driver_Loop() {
    xTaskCreatePinnedToCore(
        DriverTask,           
        "DriverTask",         
        8192,                 // Increased stack size
        NULL,                 
        3,                    
        NULL,                 
        0                     
    );  
}

//=============================================================================
// Setup
//=============================================================================

void setup()
{
  Flash_test();
  PWR_Init();
  BAT_Init();
  I2C_Init();
  PCF85063_Init();
  QMI8658_Init();
  Backlight_Init();

  SD_Init();
  Audio_Init();
  LCD_Init();
  Lvgl_Init();
  
  Settings_Init();  // Initialize settings (memory-only)
  Perf_Init();      // Initialize performance monitor (if DEBUG_PERF enabled)

  ui_custom_init();  // Use custom UI

  Driver_Loop();
}

//=============================================================================
// Main Loop
//=============================================================================

void loop()
{
  Perf_StartSection(PERF_COUNTER_MAIN_LOOP);
  
  Perf_StartSection(PERF_COUNTER_LVGL_LOOP);
  uint32_t time_till_next = Lvgl_Loop();  // Returns ms until next handler should be called
  Perf_EndSection(PERF_COUNTER_LVGL_LOOP);
  
  // Track actual renders (when LVGL did work - time_till_next is small when busy)
  if (time_till_next < 10) {
    Perf_Increment(PERF_COUNTER_LVGL_RENDER);
  }
  
  Perf_StartSection(PERF_COUNTER_UI_UPDATE);
  ui_update_from_inputs();  // Update UI with input states
  Perf_EndSection(PERF_COUNTER_UI_UPDATE);
  
  Perf_EndSection(PERF_COUNTER_MAIN_LOOP);
  
  Perf_CheckReport();  // Print report every 5 seconds (if DEBUG_PERF enabled)
  
  vTaskDelay(pdMS_TO_TICKS(5));
}