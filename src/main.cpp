#include <Arduino.h>
#include "Display_ST7789.h"
#include "Audio_PCM5101.h"
#include "RTC_PCF85063.h"
#include "Gyro_QMI8658.h"
#include "LVGL_Driver.h"
#include "PWR_Key.h"
#include "SD_Card.h"
#include "LVGL_Example.h"
#include "BAT_Driver.h"
#include "Wireless.h"
#include "InputManager.h"
#include "ui_custom.h"
#include "ui_custom_integration.h"

void DriverTask(void *parameter) {
  Wireless_Test2();
  Input_Init();  // Initialize InputManager
  
  while(1){
    Input_Update();  // Update all inputs
    PWR_Loop();
    BAT_Get_Volts();
    PCF85063_Loop();
    QMI8658_Loop(); 
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
void Driver_Loop() {
  xTaskCreatePinnedToCore(
    DriverTask,           
    "DriverTask",         
    4096,                 
    NULL,                 
    3,                    
    NULL,                 
    0                     
  );  
}
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

  ui_custom_init();  // Use custom UI

  Driver_Loop();
}

void loop()
{
  Lvgl_Loop();
  ui_update_from_inputs();  // Update UI with input states
  vTaskDelay(pdMS_TO_TICKS(5));
}