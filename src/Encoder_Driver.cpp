#include "Encoder_Driver.h"
#include "MCP23017_Driver.h"
#include "I2C_Driver.h"
#include <Adafruit_MCP23X17.h>

// Global instance
Encoder_Driver EncoderInput;

// External reference to MCP23017 instances from MCP23017_Driver
extern Adafruit_MCP23X17* getMCP23017(uint8_t index);

// Static configuration
static const EncoderConfig_t _defaultEncoderConfigs[ENCODER_COUNT] = ENCODER_CONFIGS;

// Gray code lookup table for direction detection
// Index = (prev_state << 2) | curr_state
// Value = direction: 1=CW, -1=CCW, 0=invalid/no change
static const int8_t ENCODER_LOOKUP[16] = {
     0,  // 00 -> 00: no change
    -1,  // 00 -> 01: CCW
     1,  // 00 -> 10: CW
     0,  // 00 -> 11: invalid (skip)
     1,  // 01 -> 00: CW
     0,  // 01 -> 01: no change
     0,  // 01 -> 10: invalid (skip)
    -1,  // 01 -> 11: CCW
    -1,  // 10 -> 00: CCW
     0,  // 10 -> 01: invalid (skip)
     0,  // 10 -> 10: no change
     1,  // 10 -> 11: CW
     0,  // 11 -> 00: invalid (skip)
     1,  // 11 -> 01: CW
    -1,  // 11 -> 10: CCW
     0   // 11 -> 11: no change
};

// Encoder task configuration
#define ENCODER_TASK_STACK_SIZE     2048
#define ENCODER_TASK_PRIORITY       5       // High priority for responsive input
#define ENCODER_POLL_INTERVAL_MS    1       // Poll every 1ms for fast response

// Velocity and acceleration configuration
#define ENCODER_ACCEL_SLOW_THRESH   5       // Below this: 1x multiplier
#define ENCODER_ACCEL_MED_THRESH    15      // Below this: 2x multiplier
#define ENCODER_ACCEL_FAST_THRESH   30      // Below this: 4x multiplier
#define ENCODER_ACCEL_MAX_MULT      8       // Above fast: 8x multiplier

// Debug logging control
#define ENCODER_DEBUG_LOG           0       // Set to 1 for verbose logging, 0 to disable

// Task handle for encoder polling
static TaskHandle_t _encoderTaskHandle = NULL;
static SemaphoreHandle_t _encoderMutex = NULL;

// Forward declaration of task function
static void encoderPollingTask(void* param);

Encoder_Driver::Encoder_Driver() {
    _initialized = false;

    // Copy configurations
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        _configs[i] = _defaultEncoderConfigs[i];
        _states[i].position = 0;
        _states[i].last_position = 0;
        _states[i].last_state = 0;
        _states[i].direction = 0;
        _states[i].last_change_time_ms = 0;
        _states[i].velocity = 0;
        _states[i].accumulated_delta = 0;
    }
}

bool Encoder_Driver::begin() {
    printf("Encoder_Driver: Initializing (polling mode)...\r\n");

    // Verify SwitchInput is available (MCP23017 driver)
    if (!SwitchInput.isReady()) {
        printf("Encoder_Driver: WARNING - SwitchInput not ready, waiting...\r\n");
        delay(100);
    }

    // Read initial encoder states
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        _states[i].last_state = readEncoderPins(i);
        printf("Encoder %d: Initial state = 0x%02X (A=%d, B=%d)\r\n",
               i, _states[i].last_state,
               (_states[i].last_state >> 1) & 1,
               _states[i].last_state & 1);
    }

    // Create mutex for thread-safe access
    _encoderMutex = xSemaphoreCreateMutex();
    if (_encoderMutex == NULL) {
        printf("Encoder_Driver: ERROR - Failed to create mutex\r\n");
        return false;
    }

    _initialized = true;

    // Create dedicated encoder polling task
    BaseType_t result = xTaskCreatePinnedToCore(
        encoderPollingTask,
        "EncoderTask",
        ENCODER_TASK_STACK_SIZE,
        this,
        ENCODER_TASK_PRIORITY,
        &_encoderTaskHandle,
        0  // Run on Core 0 (same core as I2C drivers)
    );

    if (result != pdPASS) {
        printf("Encoder_Driver: ERROR - Failed to create encoder task\r\n");
        return false;
    }

    printf("Encoder_Driver: Initialized with polling task (every %dms)\r\n", ENCODER_POLL_INTERVAL_MS);
    return true;
}

// Dedicated encoder polling task - runs continuously at high frequency
static void encoderPollingTask(void* param) {
    Encoder_Driver* driver = (Encoder_Driver*)param;

    printf("[ENCODER_TASK] Started on core %d\r\n", xPortGetCoreID());

    while (true) {
        driver->pollEncoders();
        vTaskDelay(pdMS_TO_TICKS(ENCODER_POLL_INTERVAL_MS));
    }
}

uint8_t Encoder_Driver::readEncoderPins(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;

    EncoderConfig_t* cfg = &_configs[index];

    // Read pins from cached GPIO (updated by MCP23017 bulk read - no I2C here)
    bool pin_a = SwitchInput.getCachedPin(cfg->expander, cfg->pin_a);
    bool pin_b = SwitchInput.getCachedPin(cfg->expander, cfg->pin_b);

    // Combine into 2-bit state (AB)
    uint8_t state = (pin_a ? 0x02 : 0x00) | (pin_b ? 0x01 : 0x00);

    return state;
}

// Simple and fast encoder processing using gray code lookup
void Encoder_Driver::processEncoder(uint8_t index, uint8_t new_state) {
    if (index >= ENCODER_COUNT) return;

    EncoderState_t* state = &_states[index];
    EncoderConfig_t* cfg = &_configs[index];

    // If state hasn't changed, nothing to do
    if (new_state == state->last_state) {
        return;
    }

    uint32_t now_ms = millis();

    // Look up direction from gray code table
    uint8_t lookup_index = (state->last_state << 2) | new_state;
    int8_t dir = ENCODER_LOOKUP[lookup_index];

#if ENCODER_DEBUG_LOG
    printf("[ENC%d] %X->%X dir=%d pos=%ld\r\n",
           index, state->last_state, new_state, dir, state->position);
#endif

    if (dir != 0) {
        // Apply inversion if configured
        if (cfg->inverted) {
            dir = -dir;
        }

        // Update position
        state->position += dir;
        state->direction = dir;

        // Track pulses for velocity calculation
        state->accumulated_delta += dir;

        // Update velocity (pulses in the last window)
        uint32_t elapsed_ms = now_ms - state->last_change_time_ms;
        if (elapsed_ms > 0 && elapsed_ms < 1000) {
            state->velocity = 1000 / elapsed_ms;
        }
    }

    state->last_state = new_state;
    state->last_change_time_ms = now_ms;
}

// Called by the dedicated polling task
void Encoder_Driver::pollEncoders() {
    if (!_initialized) return;

#if MCP_USE_INTERRUPT
    // In interrupt mode, refresh MCP cache immediately if an interrupt fired.
    // This gives sub-millisecond encoder response to pin changes.
    SwitchInput.checkAndUpdateInterrupt();
#endif

    // Take mutex for thread safety
    if (_encoderMutex != NULL && xSemaphoreTake(_encoderMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
        for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
            uint8_t new_state = readEncoderPins(i);
            processEncoder(i, new_state);
        }
        xSemaphoreGive(_encoderMutex);
    }
}

int32_t Encoder_Driver::getPosition(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;
    return _states[index].position;
}

int32_t Encoder_Driver::getDelta(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;

    int32_t delta = _states[index].position - _states[index].last_position;
    _states[index].last_position = _states[index].position;
    return delta;
}

void Encoder_Driver::resetPosition(uint8_t index) {
    if (index >= ENCODER_COUNT) return;
    _states[index].position = 0;
    _states[index].last_position = 0;
}

void Encoder_Driver::setPosition(uint8_t index, int32_t position) {
    if (index >= ENCODER_COUNT) return;
    _states[index].position = position;
    _states[index].last_position = position;
}

int8_t Encoder_Driver::getDirection(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;
    return _states[index].direction;
}

bool Encoder_Driver::hasMoved(uint8_t index) {
    if (index >= ENCODER_COUNT) return false;
    return _states[index].position != _states[index].last_position;
}

int32_t Encoder_Driver::getVelocity(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;
    return _states[index].velocity;
}

int32_t Encoder_Driver::getAcceleratedDelta(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;

    // Take mutex for thread safety
    if (_encoderMutex != NULL && xSemaphoreTake(_encoderMutex, pdMS_TO_TICKS(5)) != pdTRUE) {
        return 0;
    }

    // Get raw delta
    int32_t delta = _states[index].position - _states[index].last_position;
    _states[index].last_position = _states[index].position;
    int32_t velocity = _states[index].velocity;

    if (_encoderMutex != NULL) {
        xSemaphoreGive(_encoderMutex);
    }

    if (delta == 0) return 0;

    // Calculate acceleration multiplier based on velocity (pulses per second)
    int32_t multiplier = 1;

    if (velocity >= ENCODER_ACCEL_FAST_THRESH) {
        multiplier = ENCODER_ACCEL_MAX_MULT;  // 8x for very fast rotation
    } else if (velocity >= ENCODER_ACCEL_MED_THRESH) {
        multiplier = 4;  // 4x for medium-fast rotation
    } else if (velocity >= ENCODER_ACCEL_SLOW_THRESH) {
        multiplier = 2;  // 2x for moderate rotation
    }

    // Apply acceleration, preserving direction
    int32_t accelerated = delta * multiplier;

#if ENCODER_DEBUG_LOG
    if (accelerated != 0) {
        printf("[ENC%d] ACCEL: delta=%ld vel=%ld mult=%ld result=%ld\r\n",
               index, delta, velocity, multiplier, accelerated);
    }
#endif

    return accelerated;
}

void Encoder_Driver::printDebug() {
    printf("\r\n=== Encoder Debug ===\r\n");
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        uint8_t state = readEncoderPins(i);
        printf("Encoder %d: pos=%ld, state=0x%02X (A=%d B=%d), dir=%d\r\n",
               i, _states[i].position, state,
               (state >> 1) & 1, state & 1,
               _states[i].direction);
    }
    printf("=====================\r\n");
}
