#pragma once
#include <Arduino.h>

/******************************************************************************
 * RC Remote Input Configuration
 * 
 * Hardware:
 * - 2x MCP23017 GPIO Expanders (16 pins each = 32 digital inputs)
 * - 3x ADS1X15 ADC (4 channels each = 12 analog inputs)
 * 
 * Inputs:
 * - 2x Gimbals (4 axes total: 2 per gimbal)
 * - 2x Potentiometers
 * - Multiple switches (including 2x 3-position toggles)
 ******************************************************************************/

//=============================================================================
// I2C Addresses
//=============================================================================

// MCP23017 addresses (A0, A1, A2 pins determine address 0x20-0x27)
#define MCP23017_ADDR_1     0x20    // First expander
#define MCP23017_ADDR_2     0x21    // Second expander

// ADS1X15 addresses (ADDR pin: GND=0x48, VDD=0x49, SDA=0x4A, SCL=0x4B)
#define ADS1X15_ADDR_1      0x48    // Gimbal 1 (left stick X/Y) + Gimbal 2 X
#define ADS1X15_ADDR_2      0x49    // Gimbal 2 Y + Potentiometers
#define ADS1X15_ADDR_3      0x4A    // Additional analog inputs (spare)

//=============================================================================
// Enums
//=============================================================================

/* Switch state for standard 2-position switches */
typedef enum {
    SWITCH_OFF = 0,
    SWITCH_ON = 1
} SwitchState_t;

/* State for 3-position toggle switches */
typedef enum {
    TOGGLE_POS_UP = 0,      // Position 1 (up)
    TOGGLE_POS_CENTER = 1,  // Position 2 (center/neutral)
    TOGGLE_POS_DOWN = 2     // Position 3 (down)
} Toggle3PosState_t;

/* Switch types */
typedef enum {
    SWITCH_TYPE_MOMENTARY,      // Push button (returns when released)
    SWITCH_TYPE_TOGGLE_2POS,    // 2-position toggle (latching)
    SWITCH_TYPE_TOGGLE_3POS     // 3-position toggle (uses 2 pins)
} SwitchType_t;

//=============================================================================
// Structs
//=============================================================================

/* Configuration for a single switch input */
typedef struct {
    const char* name;           // Human-readable name (e.g., "ARM", "MODE")
    uint8_t expander;           // MCP23017 index (0 or 1)
    uint8_t pin;                // Pin on expander (0-15)
    SwitchType_t type;          // Switch type
    bool inverted;              // True if logic is inverted (pull-up)
} SwitchConfig_t;

/* Configuration for a 3-position toggle (uses 2 pins) */
typedef struct {
    const char* name;           // Human-readable name
    uint8_t expander;           // MCP23017 index (0 or 1)
    uint8_t pin_up;             // Pin for UP position
    uint8_t pin_down;           // Pin for DOWN position
    bool inverted;              // True if logic is inverted (pull-up)
} Toggle3PosConfig_t;

/* Configuration for an analog axis (gimbal or pot) */
typedef struct {
    const char* name;           // Human-readable name (e.g., "THROTTLE", "ROLL")
    uint8_t adc;                // ADS1X15 index (0, 1, or 2)
    uint8_t channel;            // ADC channel (0-3)
    int16_t min_raw;            // Raw ADC value at minimum position
    int16_t max_raw;            // Raw ADC value at maximum position
    int16_t center_raw;         // Raw ADC value at center (for gimbals)
    int16_t deadzone;           // Deadzone around center (for gimbals)
    bool inverted;              // True to invert axis direction
} AnalogAxisConfig_t;

/* Runtime state for a switch */
typedef struct {
    SwitchState_t state;        // Current state
    SwitchState_t prev_state;   // Previous state (for edge detection)
    uint32_t last_change_ms;    // Timestamp of last state change
} SwitchState_Runtime_t;

/* Runtime state for a 3-position toggle */
typedef struct {
    Toggle3PosState_t state;        // Current position
    Toggle3PosState_t prev_state;   // Previous position
    uint32_t last_change_ms;        // Timestamp of last position change
} Toggle3PosState_Runtime_t;

/* Runtime state for an analog axis */
typedef struct {
    int16_t raw;                // Raw ADC reading
    int16_t calibrated;         // Calibrated value (-1000 to +1000 for gimbals, 0-1000 for pots)
    int16_t filtered;           // Filtered/smoothed value
} AnalogAxisState_t;

//=============================================================================
// Input Definitions - CONFIGURE YOUR HARDWARE HERE
//=============================================================================

// Number of each input type
#define NUM_SWITCHES        25      // 11 original + 10 Nav switches (2x5) + 4 Encoder pins
#define NUM_3POS_TOGGLES    2       // Two 3-position toggle switches
#define NUM_GIMBAL_AXES     4       // 2 gimbals x 2 axes each
#define NUM_POTENTIOMETERS  2       // 2 potentiometers

// Total analog axes
#define NUM_ANALOG_AXES     (NUM_GIMBAL_AXES + NUM_POTENTIOMETERS)

//=============================================================================
// Switch Pin Assignments (MCP23017)
// Expander 0: pins 0-15 (GPA0-7 = 0-7, GPB0-7 = 8-15)
// Expander 1: pins 0-15 (GPA0-7 = 0-7, GPB0-7 = 8-15)
//=============================================================================

// Standard 2-position switches configuration
// Format: { "NAME", expander, pin, type, inverted }
#define SWITCH_CONFIGS { \
    /* Switches assigned to Expander 0 Port B (Pins 8-15) */ \
    { "SW_A",    0,  8, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_B",    0,  9, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_C",    0, 10, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_D",    0, 11, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_E",    0, 12, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_F",    0, 13, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_G",    0, 14, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_H",    0, 15, SWITCH_TYPE_TOGGLE_2POS, true }, \
    /* Buttons */ \
    { "BTN_1",   1, 15, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 B7 */ \
    { "BTN_3",   1,  4, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 A4 */ \
    { "BTN_4",   1,  5, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 A5 */ \
    /* Nav Switch 1 (Left) - Expander 1 Port B */ \
    { "NAV1_U",  1, 12, SWITCH_TYPE_MOMENTARY,   true }, /* B4 */ \
    { "NAV1_D",  1, 14, SWITCH_TYPE_MOMENTARY,   true }, /* B6 */ \
    { "NAV1_L",  1, 13, SWITCH_TYPE_MOMENTARY,   true }, /* B5 */ \
    { "NAV1_R",  1, 11, SWITCH_TYPE_MOMENTARY,   true }, /* B3 */ \
    { "NAV1_C",  1,  8, SWITCH_TYPE_MOMENTARY,   true }, /* B0 */ \
    /* Nav Switch 2 (Right) - Expander 0 Port A */ \
    { "NAV2_U",  0,  4, SWITCH_TYPE_MOMENTARY,   true }, /* A4 */ \
    { "NAV2_D",  0,  6, SWITCH_TYPE_MOMENTARY,   true }, /* A6 */ \
    { "NAV2_L",  0,  5, SWITCH_TYPE_MOMENTARY,   true }, /* A5 */ \
    { "NAV2_R",  0,  3, SWITCH_TYPE_MOMENTARY,   true }, /* A3 */ \
    { "NAV2_C",  0,  0, SWITCH_TYPE_MOMENTARY,   true }, /* A0 */ \
    /* Encoders (Treated as switches for raw input) */ \
    { "ENC1_A",  1,  9, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 B1 */ \
    { "ENC1_B",  1, 10, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 1 B2 */ \
    { "ENC2_A",  0,  1, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 0 A1 */ \
    { "ENC2_B",  0,  2, SWITCH_TYPE_MOMENTARY,   true }, /* Exp 0 A2 */ \
}

// 3-position toggle switches configuration
// Format: { "NAME", expander, pin_up, pin_down, inverted }
// CENTER is detected when neither UP nor DOWN pin is active
// Assigned to Expander 1 Port A 0-3
#define TOGGLE_3POS_CONFIGS { \
    { "SW_3POS_1", 1, 0, 1, true }, \
    { "SW_3POS_2", 1, 2, 3, true }, \
}

//=============================================================================
// Analog Input Assignments (ADS1X15)
// ADC 0: channels 0-3
// ADC 1: channels 0-3
// ADC 2: channels 0-3
//=============================================================================

// Gimbal axes configuration
// Format: { "NAME", adc, channel, min_raw, max_raw, center_raw, deadzone, inverted }
#define GIMBAL_CONFIGS { \
    { "LEFT_X",   0, 1,  0, 32767, 16383, 200, false }, /* ADC 0 (0x48) Ch 1 */ \
    { "LEFT_Y",   0, 0,  0, 32767, 16383, 200, false }, /* ADC 0 (0x48) Ch 0 */ \
    { "RIGHT_X",  1, 1,  0, 32767, 16383, 200, false }, /* ADC 1 (0x49) Ch 1 */ \
    { "RIGHT_Y",  1, 0,  0, 32767, 16383, 200, false }, /* ADC 1 (0x49) Ch 0 */ \
}

// Potentiometer configurations  
// Format: { "NAME", adc, channel, min_raw, max_raw, center_raw, deadzone, inverted }
// Note: center_raw and deadzone are ignored for pots (no center position)
#define POT_CONFIGS { \
    { "POT_1",    1, 2,  0, 32767, 0, 0, false }, /* ADC 1 (0x49) Ch 2 */ \
    { "POT_2",    1, 3,  0, 32767, 0, 0, false }, /* ADC 1 (0x49) Ch 3 */ \
}

//=============================================================================
// Calibration Constants
//=============================================================================

#define ANALOG_OUTPUT_MIN       -1000   // Output range minimum (for gimbals)
#define ANALOG_OUTPUT_MAX        1000   // Output range maximum
#define ANALOG_OUTPUT_CENTER        0   // Output center value

#define POT_OUTPUT_MIN              0   // Potentiometer output minimum
#define POT_OUTPUT_MAX           1000   // Potentiometer output maximum

#define DEBOUNCE_MS                20   // Switch debounce time in milliseconds
#define ANALOG_FILTER_SAMPLES       4   // Number of samples for moving average

//=============================================================================
// CRSF UART Pin Assignments
//=============================================================================
#define CRSF_UART_TX_PIN    43      // ESP32-S3 GPIO for UART TX to ELRS module
#define CRSF_UART_RX_PIN    44      // ESP32-S3 GPIO for UART RX from ELRS module
#define CRSF_OE_PIN         18      // SN74LVC1G125 OE control (active-low, pull-up)

