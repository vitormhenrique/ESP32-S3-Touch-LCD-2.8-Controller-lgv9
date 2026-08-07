#include <Arduino.h>
#include "Display_ST7789.h"
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
#include "CRSF_Manager.h"
#include "RadioCli.h"

void DriverTask(void *parameter) {
  // Start CRSF before input polling, but CRSF_Init deliberately waits before
  // configuring Serial1 so the ELRS module can finish cold boot/autobaud.
  // Real RC frames are internally gated on RCInput.isReady().
  CRSF_Init();

  // Wireless_Test2();  // Disabled - WiFi/BLE scan not needed for RC
  Input_Init();  // Initialize InputManager

  // Load saved gimbal calibrations from Settings
  ui_load_gimbal_calibrations();

  // Load saved pot calibrations from Settings
  ui_load_pot_calibrations();

  // Until the ELRS link is up, inputs/telemetry are useless - poll slowly to
  // keep the I2C bus and core 0 quiet while the CRSF task establishes the
  // link. Once linked, switch to the normal 50Hz polling permanently.
  bool everLinked = false;

  while(1){
    if (!everLinked && CRSFLink.isLinkUp()) {
      everLinked = true;
      printf("[Driver] Link up - switching input polling to 50Hz\n");
    }

    Perf_StartSection(PERF_COUNTER_DRIVER_LOOP);
    
    Perf_StartSection(PERF_COUNTER_INPUT_UPDATE);
    Input_Update();  // Update all inputs
    Perf_EndSection(PERF_COUNTER_INPUT_UPDATE);
    
    PWR_Loop();
    BAT_Get_Volts();
    PCF85063_Loop();
    QMI8658_Loop(); 
    
    Perf_EndSection(PERF_COUNTER_DRIVER_LOOP);
    // 50Hz once linked; 10Hz while waiting for link (keeps PWR button responsive)
    vTaskDelay(pdMS_TO_TICKS(everLinked ? 20 : 100));
  }
}

void Driver_Loop() {
  xTaskCreatePinnedToCore(
    DriverTask,           
    "DriverTask",         
    8192,                 // Increased stack for I2C + mutex overhead
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
  Serial.begin(115200);

  // Tri-state the CRSF half-duplex buffer IMMEDIATELY so the bus stays quiet
  // while the display/LVGL initialize. A floating OE during boot can spray
  // garbage at the ELRS module, sending its UART watchdog baud-cycling and
  // delaying (sometimes preventing) the bind until a reboot.
  pinMode(CRSF_OE_PIN, OUTPUT);
  digitalWrite(CRSF_OE_PIN, LOW);  // Active-high OE: LOW = hi-Z

  // Flash_test();
  PWR_Init();
  BAT_Init();
  I2C_Init();
  PCF85063_Init();
  QMI8658_Init();
  Backlight_Init();

  // SD_Init();
  LCD_Init();
  Lvgl_Init();
  
  Settings_Init();  // Initialize settings (memory-only)
  // Note: Perf_Init moved to loop() to ensure CDC is ready

  // ELRS config client must be initialized before the UI is created
  // (the Radio screen registers callbacks that elrs_client_init would reset).
  CRSF_ElrsClientInit();
  RadioCli_Init();

  ui_custom_init();  // Use custom UI

  Driver_Loop();
}

//=============================================================================
// Main Loop
//=============================================================================

void loop()
{
  static bool perf_initialized = false;
  
  // Initialize perf monitor after startup (reduced delay since wireless scan is disabled)
  if (!perf_initialized && millis() > 2000) {
    Perf_Init();
    perf_initialized = true;
  }
  
  Perf_StartSection(PERF_COUNTER_MAIN_LOOP);

  RadioCli_Poll();
  
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