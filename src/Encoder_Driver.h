#pragma once
#include <Arduino.h>
#include "InputConfig.h"

/******************************************************************************
 * Rotary Encoder Driver using MCP23017 GPIO Expander
 *
 * Features:
 * - High-frequency polling for accurate pulse counting
 * - Support for 2 encoders (with optional push buttons)
 * - Gray code state machine for reliable direction detection
 * - LVGL encoder input device integration
 *
 * Hardware Setup:
 * - Encoder A/B pins connected to MCP23017 with internal pull-ups
 * - Dedicated FreeRTOS task polls at 1ms for fast response
 ******************************************************************************/

// Encoder configuration from InputConfig.h
// ENC1: Expander 1, pins B1 (9) and B2 (10)
// ENC2: Expander 0, pins A1 (1) and A2 (2)
#define ENCODER_COUNT           2

// Encoder indices
enum EncoderIndex {
    ENCODER_1 = 0,
    ENCODER_2 = 1
};

// Encoder configuration struct
typedef struct {
    uint8_t expander;       // MCP23017 index (0 or 1)
    uint8_t pin_a;          // Pin for encoder channel A
    uint8_t pin_b;          // Pin for encoder channel B
    bool inverted;          // Invert direction
} EncoderConfig_t;

// Encoder runtime state
typedef struct {
    volatile int32_t position;      // Current position count
    volatile int32_t last_position; // Last reported position
    volatile uint8_t last_state;    // Last AB state for gray code
    volatile int8_t direction;      // Last direction: 1=CW, -1=CCW, 0=none

    // Velocity tracking
    volatile uint32_t last_change_time_ms;  // Millis of last state change
    volatile int32_t velocity;              // Current velocity (pulses per second)
    volatile int32_t accumulated_delta;     // Accumulated delta for acceleration
} EncoderState_t;

// Default encoder configurations
#define ENCODER_CONFIGS { \
    { 1,  9, 10, false },  /* ENC1: Exp 1, pins B1/B2 */ \
    { 0,  1,  2, false },  /* ENC2: Exp 0, pins A1/A2 */ \
}

class Encoder_Driver {
public:
    Encoder_Driver();

    /**
     * Initialize encoder driver
     * Must be called after MCP23017 is initialized
     * @return true if initialization successful
     */
    bool begin();

    /**
     * Poll encoders - called by dedicated task at high frequency
     * Thread-safe with mutex protection
     */
    void pollEncoders();

    /**
     * Get current encoder position
     * @param index Encoder index (0 or 1)
     * @return Current position count
     */
    int32_t getPosition(uint8_t index);

    /**
     * Get position delta since last call (for LVGL)
     * @param index Encoder index (0 or 1)
     * @return Position change since last call
     */
    int32_t getDelta(uint8_t index);

    /**
     * Reset encoder position to zero
     * @param index Encoder index (0 or 1)
     */
    void resetPosition(uint8_t index);

    /**
     * Set encoder position
     * @param index Encoder index
     * @param position New position value
     */
    void setPosition(uint8_t index, int32_t position);

    /**
     * Get last direction of rotation
     * @param index Encoder index
     * @return 1=CW, -1=CCW, 0=no movement
     */
    int8_t getDirection(uint8_t index);

    /**
     * Check if encoder has moved since last check
     * @param index Encoder index
     * @return true if position changed
     */
    bool hasMoved(uint8_t index);

    /**
     * Get velocity-adjusted delta (accelerated value based on rotation speed)
     * @param index Encoder index (0 or 1)
     * @return Accelerated delta value
     */
    int32_t getAcceleratedDelta(uint8_t index);

    /**
     * Get current velocity (pulses per second)
     * @param index Encoder index
     * @return Velocity in pulses/sec
     */
    int32_t getVelocity(uint8_t index);

    /**
     * Check if driver is ready
     */
    bool isReady() { return _initialized; }

    /**
     * Debug print encoder states
     */
    void printDebug();

private:
    bool _initialized;
    EncoderConfig_t _configs[ENCODER_COUNT];
    EncoderState_t _states[ENCODER_COUNT];

    /**
     * Read current encoder pins state
     */
    uint8_t readEncoderPins(uint8_t index);

    /**
     * Process gray code state machine for one encoder
     */
    void processEncoder(uint8_t index, uint8_t new_state);
};

// Global instance
extern Encoder_Driver EncoderInput;
