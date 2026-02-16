/**
 * @file ui_custom_integration.cpp
 * @brief Integration of custom UI with InputManager
 */

#include "ui_custom_integration.h"
#include "InputManager.h"
#include "Encoder_Driver.h"
#include "Settings.h"
#include "BAT_Driver.h"
#include "ui/screens/ui_screen_settings.h"
#include "CRSF_Manager.h"

void ui_update_from_inputs(void)
{
    // Only update if InputManager is ready
    if (!RCInput.isReady()) {
        ui_set_status(false);  // Show disconnected icon
        return;
    }
    
    // Update status - connected
    ui_set_status(true);  // Show connected icon
    
    //=========================================================================
    // Update Gimbals
    //=========================================================================
    ui_set_gimbal_left(RCInput.getLeftX(), RCInput.getLeftY());
    ui_set_gimbal_right(RCInput.getRightX(), RCInput.getRightY());
    
    //=========================================================================
    // Update Potentiometers
    //=========================================================================
    ui_set_pot(0, RCInput.getPot(POT_1));
    ui_set_pot(1, RCInput.getPot(POT_2));
    
    //=========================================================================
    // Update 3-Position Toggles
    //=========================================================================
    // The toggle state enum matches: UP=0, CENTER=1, DOWN=2
    ui_set_toggle3(0, (uint8_t)RCInput.getToggle3Pos(TOGGLE_3POS_1));
    ui_set_toggle3(1, (uint8_t)RCInput.getToggle3Pos(TOGGLE_3POS_2));
    
    //=========================================================================
    // Update 2-Position Switches
    // Adjust the indices based on your InputConfig.h SWITCH_CONFIGS
    //=========================================================================
    // Assuming switches 0-5 are your 2-position switches
    for (uint8_t i = 0; i < 6; i++) {
        ui_set_switch(i, RCInput.isSwitchOn(i));
    }
    
    //=========================================================================
    // Update Buttons (momentary switches)
    // BTN_1=idx 8, BTN_3=idx 9, BTN_4=idx 10 (BTN_2 removed)
    //=========================================================================
    for (uint8_t i = 0; i < 3; i++) {
        ui_set_button(i, RCInput.isSwitchOn(8 + i));
    }
    
    //=========================================================================
    // Update Nav Switches
    // Nav 1: Indices 11-15 (U, D, L, R, C)
    // Nav 2: Indices 16-20 (U, D, L, R, C)
    //=========================================================================
    ui_set_nav_switch(0,
        RCInput.isSwitchOn(11), // Up
        RCInput.isSwitchOn(12), // Down
        RCInput.isSwitchOn(13), // Left
        RCInput.isSwitchOn(14), // Right
        RCInput.isSwitchOn(15)  // Center
    );

    ui_set_nav_switch(1,
        RCInput.isSwitchOn(16), // Up
        RCInput.isSwitchOn(17), // Down
        RCInput.isSwitchOn(18), // Left
        RCInput.isSwitchOn(19), // Right
        RCInput.isSwitchOn(20)  // Center
    );
    
    //=========================================================================
    // Update Encoders
    // Now using actual encoder values from the Encoder_Driver
    //=========================================================================
    ui_set_encoder(0, RCInput.getEncoderPosition(ENCODER_1));
    ui_set_encoder(1, RCInput.getEncoderPosition(ENCODER_2));
    
    //=========================================================================
    // Update Gimbal Calibration Display
    // Always update the raw values display when on settings screen
    //=========================================================================
    int16_t gimbal_raw[4];
    gimbal_raw[0] = RCInput.getGimbalRaw(GIMBAL_LEFT_X);
    gimbal_raw[1] = RCInput.getGimbalRaw(GIMBAL_LEFT_Y);
    gimbal_raw[2] = RCInput.getGimbalRaw(GIMBAL_RIGHT_X);
    gimbal_raw[3] = RCInput.getGimbalRaw(GIMBAL_RIGHT_Y);
    ui_gimbal_cal_update(gimbal_raw);

    //=========================================================================
    // Update battery voltage on top bar
    //=========================================================================
    ui_set_battery_voltage(BAT_analogVolts);

    //=========================================================================
    // Update Telemetry from CRSF link
    //=========================================================================
    if (CRSFLink.isReady()) {
        CRSFLink.updateUI();
    }
}

// Call these from your encoder ISR or polling routine
void ui_encoder_increment(uint8_t index, int32_t delta)
{
    // No longer needed - encoder is handled by Encoder_Driver and LVGL
    (void)index;
    (void)delta;
}

void ui_encoder_set(uint8_t index, int32_t value)
{
    if (index < ENCODER_COUNT) {
        EncoderInput.setPosition(index, value);
    }
}

void ui_apply_gimbal_calibration(uint8_t axis, int16_t min_val, int16_t center_val, int16_t max_val, int16_t deadzone, bool inverted)
{
    if (!RCInput.isReady()) {
        printf("Warning: Cannot apply calibration - InputManager not ready\n");
        return;
    }
    RCInput.calibrateGimbal(axis, min_val, center_val, max_val, deadzone, inverted);
}

void ui_load_gimbal_calibrations(void)
{
    if (!RCInput.isReady()) {
        printf("Warning: Cannot load calibration - InputManager not ready\n");
        return;
    }
    
    const Settings_t* settings = Settings_Get();
    if (!settings) return;
    
    for (uint8_t i = 0; i < 4; i++) {
        const GimbalCalibration_t* cal = &settings->gimbal_cal[i];
        if (cal->calibrated) {
            RCInput.calibrateGimbal(i, cal->min_raw, cal->center_raw, cal->max_raw, 
                                    cal->deadzone, cal->inverted);
            printf("Loaded calibration for gimbal %d\n", i);
        }
    }
}
