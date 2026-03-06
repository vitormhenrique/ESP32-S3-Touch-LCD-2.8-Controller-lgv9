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

//=============================================================================
// MCP23017 Interrupt Configuration
// Both MCP23017 INTA+INTB outputs are wire-ORed to a single ESP32 GPIO.
// Set MCP_USE_INTERRUPT to 1 for interrupt-driven updates (lower latency,
// less I2C traffic) or 0 for polling-only mode.
//=============================================================================
#define MCP_USE_INTERRUPT   1       // 1 = interrupt-driven, 0 = polling only
#define MCP_INT_PIN         15      // ESP32 GPIO connected to MCP23017 INTA+INTB
#define MCP_INT_FALLBACK_MS 100     // Periodic fallback read interval (ms)

// ADS1X15 addresses (ADDR pin: GND=0x48, VDD=0x49, SDA=0x4A, SCL=0x4B)
#define ADS1X15_ADDR_1      0x48    // Gimbal 1 (left stick X/Y) + Gimbal 2 X
#define ADS1X15_ADDR_2      0x49    // Gimbal 2 Y + Potentiometers
#define ADS1X15_ADDR_3      0x4A    // Additional analog inputs (spare)

//=============================================================================
// Device Index Constants (for use in pin config tables)
//=============================================================================

// MCP23017 expander indices (by I2C address)
#define MCP_23017_20        0       // MCP23017 at 0x20 (first expander)
#define MCP_23017_21        1       // MCP23017 at 0x21 (second expander)

// ADS1115 ADC indices (by I2C address)
#define ADS_115_48          0       // ADS1115 at 0x48 (gimbals)
#define ADS_115_49          1       // ADS1115 at 0x49 (gimbals + pots)
#define ADS_115_4A          2       // ADS1115 at 0x4A (spare)

//=============================================================================
// MCP23017 Pin Name Constants
// Port A: GPA0-GPA7 = pins 0-7   (named A0-A7)
// Port B: GPB0-GPB7 = pins 8-15  (named B0-B7)
//=============================================================================

#define MCP_A0              0
#define MCP_A1              1
#define MCP_A2              2
#define MCP_A3              3
#define MCP_A4              4
#define MCP_A5              5
#define MCP_A6              6
#define MCP_A7              7
#define MCP_B0              8
#define MCP_B1              9
#define MCP_B2              10
#define MCP_B3              11
#define MCP_B4              12
#define MCP_B5              13
#define MCP_B6              14
#define MCP_B7              15

//=============================================================================
// ADS1115 Channel Name Constants (named A0-A3)
//=============================================================================

#define ADS_A0              0
#define ADS_A1              1
#define ADS_A2              2
#define ADS_A3              3

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
#define NUM_SWITCHES        24      // 6 2-pos SW + 4 BTN + 10 Nav switches (2x5) + 4 Encoder pins
#define NUM_3POS_TOGGLES    2       // Two 3-position toggle switches (SW_E, SW_F)
#define NUM_GIMBAL_AXES     4       // 2 gimbals x 2 axes each
#define NUM_POTENTIOMETERS  2       // 2 potentiometers

// Total analog axes
#define NUM_ANALOG_AXES     (NUM_GIMBAL_AXES + NUM_POTENTIOMETERS)

//=============================================================================
// Switch Pin Assignments (MCP23017)
// Use MCP_23017_20 / MCP_23017_21 for expander and MCP_A0..MCP_B7 for pins
//=============================================================================

// Standard 2-position switches + buttons + nav + encoders
// Format: { "NAME", expander, pin, type, inverted }
#define SWITCH_CONFIGS { \
    /* 6 two-position toggle switches (indices 0-5) */ \
    { "SW_A",    MCP_23017_21, MCP_A5, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_B",    MCP_23017_20, MCP_B2, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_C",    MCP_23017_21, MCP_A0, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_D",    MCP_23017_21, MCP_A3, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_G",    MCP_23017_21, MCP_A4, SWITCH_TYPE_TOGGLE_2POS, true }, \
    { "SW_H",    MCP_23017_20, MCP_B3, SWITCH_TYPE_TOGGLE_2POS, true }, \
    /* 4 Buttons (indices 6-9) */ \
    { "BTN_1",   MCP_23017_21, MCP_B7, SWITCH_TYPE_MOMENTARY,   true }, \
    { "BTN_2",   MCP_23017_20, MCP_A7, SWITCH_TYPE_MOMENTARY,   true }, \
    { "BTN_3",   MCP_23017_21, MCP_A1, SWITCH_TYPE_MOMENTARY,   true }, \
    { "BTN_4",   MCP_23017_21, MCP_A2, SWITCH_TYPE_MOMENTARY,   true }, \
    /* Nav Switch 1 (Left) - Expander 1 Port B (indices 10-14) */ \
    { "NAV1_U",  MCP_23017_21, MCP_B4, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV1_D",  MCP_23017_21, MCP_B6, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV1_L",  MCP_23017_21, MCP_B5, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV1_R",  MCP_23017_21, MCP_B3, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV1_C",  MCP_23017_21, MCP_B0, SWITCH_TYPE_MOMENTARY,   true }, \
    /* Nav Switch 2 (Right) - Expander 0 Port A (indices 15-19) */ \
    { "NAV2_U",  MCP_23017_20, MCP_A4, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV2_D",  MCP_23017_20, MCP_A6, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV2_L",  MCP_23017_20, MCP_A5, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV2_R",  MCP_23017_20, MCP_A3, SWITCH_TYPE_MOMENTARY,   true }, \
    { "NAV2_C",  MCP_23017_20, MCP_A0, SWITCH_TYPE_MOMENTARY,   true }, \
    /* Encoders (indices 20-23) */ \
    { "ENC1_A",  MCP_23017_21, MCP_B1, SWITCH_TYPE_MOMENTARY,   true }, \
    { "ENC1_B",  MCP_23017_21, MCP_B2, SWITCH_TYPE_MOMENTARY,   true }, \
    { "ENC2_A",  MCP_23017_20, MCP_A1, SWITCH_TYPE_MOMENTARY,   true }, \
    { "ENC2_B",  MCP_23017_20, MCP_A2, SWITCH_TYPE_MOMENTARY,   true }, \
}

// 3-position toggle switches configuration (SW_E and SW_F)
// Format: { "NAME", expander, pin_up, pin_down, inverted }
// CENTER is detected when neither UP nor DOWN pin is active
#define TOGGLE_3POS_CONFIGS { \
    { "SW_E", MCP_23017_21, MCP_A6, MCP_A7, true }, \
    { "SW_F", MCP_23017_20, MCP_B1, MCP_B0, true }, \
}

//=============================================================================
// Analog Input Assignments (ADS1X15)
// Use ADS_115_48 / ADS_115_49 for device and ADS_A0..ADS_A3 for channel
//=============================================================================

// Gimbal axes configuration
// Format: { "NAME", adc, channel, min_raw, max_raw, center_raw, deadzone, inverted }
#define GIMBAL_CONFIGS { \
    { "LEFT_X",   ADS_115_48, ADS_A1,  0, 32767, 16383, 200, false }, \
    { "LEFT_Y",   ADS_115_48, ADS_A0,  0, 32767, 16383, 200, false }, \
    { "RIGHT_X",  ADS_115_49, ADS_A1,  0, 32767, 16383, 200, false }, \
    { "RIGHT_Y",  ADS_115_49, ADS_A0,  0, 32767, 16383, 200, false }, \
}

// Potentiometer configurations  
// Format: { "NAME", adc, channel, min_raw, max_raw, center_raw, deadzone, inverted }
// Note: center_raw and deadzone are ignored for pots (no center position)
#define POT_CONFIGS { \
    { "POT_1",    ADS_115_49, ADS_A3,  0, 32767, 0, 0, false }, \
    { "POT_2",    ADS_115_49, ADS_A2,  0, 32767, 0, 0, false }, \
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

