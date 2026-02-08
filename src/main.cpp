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
#include "SettingsManager.h"

// Include new modular UI
#include "ui/ui_manager.h"
#include "ui/ui_screen_input.h"
#include "ui/ui_screen_telemetry.h"
#include "ui/ui_screen_config.h"
#include "ui/ui_screen_calibration.h"
#include "ui/ui_screen_robot.h"

// Debug metrics (controlled by DEBUG_METRICS build flag)
#if DEBUG_METRICS
#include "DebugMetrics.h"
#endif

//=============================================================================
// Globals
//=============================================================================

extern SettingsManager Settings;

//=============================================================================
// UI Update Function
//=============================================================================

void updateUIFromInputs() {
    // Update gimbal displays
    for (int i = 0; i < 2; i++) {
        int16_t x = RCInput.getGimbal(i * 2);      // X axis
        int16_t y = RCInput.getGimbal(i * 2 + 1);  // Y axis
        
        // Values are already -1000 to 1000, map to -100 to 100 for display
        int16_t displayX = x / 10;
        int16_t displayY = y / 10;
        
        ui_input_set_gimbal(i, displayX, displayY);
    }
    
    // Update navigation switches
    for (int i = 0; i < 2; i++) {
        bool up = RCInput.isNavUp(i);
        bool down = RCInput.isNavDown(i);
        bool left = RCInput.isNavLeft(i);
        bool right = RCInput.isNavRight(i);
        bool center = RCInput.isNavCenter(i);
        ui_input_set_nav_switch(i, up, down, left, right, center);
    }
    
    // Update encoders
    for (int i = 0; i < 2; i++) {
        int32_t position = RCInput.getEncoderPosition(i);
        ui_input_set_encoder(i, position);
    }
    
    // Update buttons
    for (int i = 0; i < 8; i++) {
        ui_input_set_button(i, RCInput.isButtonPressed(i));
    }
    
    // Update potentiometers
    for (int i = 0; i < 2; i++) {
        int16_t value = RCInput.getPot(i);
        // Map from 0-1000 to 0-100 percent
        uint8_t percent = value / 10;
        ui_input_set_pot(i, percent);
    }
    
#if DEBUG_METRICS
    Debug.inputUpdated();
#endif
}

//=============================================================================
// Callback Overrides
//=============================================================================

// Override the weak function for robot selection
void ui_on_robot_selected(RobotType_t robot_type) {
    // Update telemetry screen
    ui_telemetry_set_robot_type((uint8_t)robot_type);
    
    // Save to settings
    Settings.setRobotProfile((RobotProfile_t)robot_type);
    Settings.save();
    
    Serial.printf("Robot type changed to: %d\n", robot_type);
}

// Override the weak function for gimbal calibration
void ui_on_gimbal_calibrated(uint8_t gimbal_id, int16_t min_val, int16_t center_val, int16_t max_val) {
    if (gimbal_id < 4) {
        Settings.setGimbalCalibration(gimbal_id, min_val, center_val, max_val, false);
        Settings.save();
        
        Serial.printf("Gimbal %d calibrated: min=%d, center=%d, max=%d\n", 
                     gimbal_id, min_val, center_val, max_val);
    }
}

// Override the weak function for touch calibration
void ui_on_touch_calibrated(int16_t x1, int16_t y1, int16_t x2, int16_t y2, 
                            int16_t x3, int16_t y3, int16_t x4, int16_t y4) {
    Settings.setTouchCalibration(x1, y1, x2, y2, x3, y3, x4, y4);
    Settings.save();
    
    Serial.println("Touch screen calibrated");
}

//=============================================================================
// Driver Task (runs on Core 0)
//=============================================================================

void DriverTask(void *parameter) {
    Wireless_Test2();
    Input_Init();  // Initialize InputManager
    
    while(1) {
        Input_Update();  // Update all inputs
        PWR_Loop();
        BAT_Get_Volts();
        PCF85063_Loop();
        QMI8658_Loop(); 
        vTaskDelay(pdMS_TO_TICKS(50));  // 20Hz update rate
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
    Serial.begin(115200);
    Serial.println("RC Controller Starting...");
    
    // Initialize hardware
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
    
    // Initialize settings
    Settings.begin();
    
    // Initialize new modular UI
    ui_init();
    ui_setup_callbacks();
    
    // Apply loaded settings
    ui_telemetry_set_robot_type((uint8_t)Settings.getRobotProfile());
    ui_robot_set_selected((RobotType_t)Settings.getRobotProfile());
    
    // Set version
    ui_config_set_version("v1.0.0");
    
#if DEBUG_METRICS
    Debug.begin();
    Serial.println("Debug metrics enabled");
#endif
    
    // Start driver task
    Driver_Loop();
    
    Serial.println("Setup complete");
}

//=============================================================================
// Main Loop
//=============================================================================

void loop()
{
#if DEBUG_METRICS
    Debug.loopStart();
#endif
    
    // Update LVGL
    Lvgl_Loop();
    
#if DEBUG_METRICS
    Debug.frameRendered();
#endif
    
    // Update UI from inputs
    updateUIFromInputs();
    
#if DEBUG_METRICS
    Debug.loopEnd();
    Debug.printMetrics();  // Prints every 5 seconds if enabled
#endif
    
    vTaskDelay(pdMS_TO_TICKS(5));  // ~200Hz main loop
}