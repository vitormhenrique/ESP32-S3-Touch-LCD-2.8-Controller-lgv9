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

// Volatile flag for ISR
static volatile bool _interruptPending = false;

// ISR handler - keep it minimal
void IRAM_ATTR encoder_isr_handler() {
    _interruptPending = true;
}

Encoder_Driver::Encoder_Driver() {
    _initialized = false;
    
    // Copy configurations
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        _configs[i] = _defaultEncoderConfigs[i];
        _states[i].position = 0;
        _states[i].last_position = 0;
        _states[i].last_state = 0;
        _states[i].direction = 0;
    }
}

bool Encoder_Driver::begin() {
    printf("Encoder_Driver: Initializing...\r\n");
    
    // Verify SwitchInput is available (MCP23017 driver)
    if (!SwitchInput.isReady()) {
        printf("Encoder_Driver: WARNING - SwitchInput not ready, waiting...\r\n");
        delay(100);
    }
    
    // Configure MCP23017 for interrupt operation
    configureMCP23017Interrupts();
    
    // Configure ESP32 interrupt pin
    pinMode(MCP23017_INT_PIN, INPUT_PULLUP);
    
    // Read initial encoder states
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        _states[i].last_state = readEncoderPins(i);
        printf("Encoder %d: Initial state = 0x%02X (A=%d, B=%d)\r\n", 
               i, _states[i].last_state,
               (_states[i].last_state >> 1) & 1,
               _states[i].last_state & 1);
    }
    
    // Attach interrupt - falling edge since MCP23017 INT is active low
    attachInterrupt(digitalPinToInterrupt(MCP23017_INT_PIN), encoder_isr_handler, FALLING);
    
    _initialized = true;
    printf("Encoder_Driver: Initialized on GPIO%d (polling + interrupt)\r\n", MCP23017_INT_PIN);
    printf("Encoder_Driver: Ready - rotate encoders to test\r\n");
    
    return true;
}

void Encoder_Driver::configureMCP23017Interrupts() {
    printf("Encoder_Driver: Configuring MCP23017 interrupts...\r\n");
    
    // Access the MCP23017 instances via I2C
    // We need to configure the interrupt registers directly
    
    // For Adafruit_MCP23X17, we need to set:
    // - GPINTEN: Interrupt-on-change enable
    // - INTCON: Compare against previous value (not DEFVAL)
    // - IOCON: Configure INT pins as open-drain, active-low, mirror A/B
    
    // MCP23017 register addresses
    #define MCP23017_IODIRA   0x00
    #define MCP23017_IODIRB   0x01
    #define MCP23017_IPOLA    0x02
    #define MCP23017_IPOLB    0x03
    #define MCP23017_GPINTENA 0x04
    #define MCP23017_GPINTENB 0x05
    #define MCP23017_DEFVALA  0x06
    #define MCP23017_DEFVALB  0x07
    #define MCP23017_INTCONA  0x08
    #define MCP23017_INTCONB  0x09
    #define MCP23017_IOCON    0x0A
    #define MCP23017_GPPUA    0x0C
    #define MCP23017_GPPUB    0x0D
    #define MCP23017_INTFA    0x0E
    #define MCP23017_INTFB    0x0F
    #define MCP23017_INTCAPA  0x10
    #define MCP23017_INTCAPB  0x11
    #define MCP23017_GPIOA    0x12
    #define MCP23017_GPIOB    0x13
    
    // Configure each encoder's MCP23017
    for (uint8_t enc = 0; enc < ENCODER_COUNT; enc++) {
        uint8_t addr = (_configs[enc].expander == 0) ? MCP23017_ADDR_1 : MCP23017_ADDR_2;
        uint8_t pin_a = _configs[enc].pin_a;
        uint8_t pin_b = _configs[enc].pin_b;
        
        // Determine if pins are on port A (0-7) or port B (8-15)
        bool port_a = (pin_a < 8);
        uint8_t mask_a = 1 << (pin_a % 8);
        uint8_t mask_b = 1 << (pin_b % 8);
        uint8_t mask = mask_a | mask_b;
        
        // Set IOCON: Mirror interrupts (INTA = INTB), active-low, open-drain
        // Bit 6 (MIRROR) = 1, Bit 2 (ODR) = 1, Bit 1 (INTPOL) = 0
        Wire.beginTransmission(addr);
        Wire.write(MCP23017_IOCON);
        Wire.write(0x44);  // MIRROR=1, ODR=1 (open-drain), INTPOL=0 (active-low)
        Wire.endTransmission();
        
        // Enable internal pull-ups for encoder pins
        uint8_t gppu_reg = port_a ? MCP23017_GPPUA : MCP23017_GPPUB;
        
        // Read current pull-up settings
        Wire.beginTransmission(addr);
        Wire.write(gppu_reg);
        Wire.endTransmission(false);
        Wire.requestFrom(addr, (uint8_t)1);
        uint8_t gppu = Wire.read();
        
        // Enable pull-ups for encoder pins
        gppu |= mask;
        Wire.beginTransmission(addr);
        Wire.write(gppu_reg);
        Wire.write(gppu);
        Wire.endTransmission();
        printf("Encoder %d: Pull-ups enabled (GPPU%c = 0x%02X)\r\n", enc, port_a ? 'A' : 'B', gppu);
        
        // Enable interrupt-on-change for encoder pins
        uint8_t gpinten_reg = port_a ? MCP23017_GPINTENA : MCP23017_GPINTENB;
        
        // Read current interrupt enable settings
        Wire.beginTransmission(addr);
        Wire.write(gpinten_reg);
        Wire.endTransmission(false);
        Wire.requestFrom(addr, (uint8_t)1);
        uint8_t gpinten = Wire.read();
        
        // Enable interrupts for encoder pins
        gpinten |= mask;
        Wire.beginTransmission(addr);
        Wire.write(gpinten_reg);
        Wire.write(gpinten);
        Wire.endTransmission();
        printf("Encoder %d: Interrupts enabled (GPINTEN%c = 0x%02X)\r\n", enc, port_a ? 'A' : 'B', gpinten);
        
        // Set INTCON to 0 (compare against previous value, not DEFVAL)
        uint8_t intcon_reg = port_a ? MCP23017_INTCONA : MCP23017_INTCONB;
        Wire.beginTransmission(addr);
        Wire.write(intcon_reg);
        Wire.write(0x00);  // Compare against previous pin value
        Wire.endTransmission();
        
        // Read INTCAP to clear any pending interrupts
        uint8_t intcap_reg = port_a ? MCP23017_INTCAPA : MCP23017_INTCAPB;
        Wire.beginTransmission(addr);
        Wire.write(intcap_reg);
        Wire.endTransmission(false);
        Wire.requestFrom(addr, (uint8_t)1);
        Wire.read();  // Discard to clear interrupt
        
        printf("Encoder %d: Configured on Exp %d, pins %d/%d\r\n", 
               enc, _configs[enc].expander, pin_a, pin_b);
    }
}

uint8_t Encoder_Driver::readEncoderPins(uint8_t index) {
    if (index >= ENCODER_COUNT) return 0;
    
    EncoderConfig_t* cfg = &_configs[index];
    
    // Read pins from MCP23017 via the SwitchInput driver
    bool pin_a = SwitchInput.readPin(cfg->expander, cfg->pin_a);
    bool pin_b = SwitchInput.readPin(cfg->expander, cfg->pin_b);
    
    // Combine into 2-bit state (AB)
    uint8_t state = (pin_a ? 0x02 : 0x00) | (pin_b ? 0x01 : 0x00);
    
    return state;
}

void Encoder_Driver::processEncoder(uint8_t index, uint8_t new_state) {
    if (index >= ENCODER_COUNT) return;
    
    EncoderState_t* state = &_states[index];
    EncoderConfig_t* cfg = &_configs[index];
    
    // Look up direction from gray code table
    uint8_t lookup_index = (state->last_state << 2) | new_state;
    int8_t dir = ENCODER_LOOKUP[lookup_index];
    
    if (dir != 0) {
        // Apply inversion if configured
        if (cfg->inverted) {
            dir = -dir;
        }
        
        state->position += dir;
        state->direction = dir;
    }
    
    state->last_state = new_state;
}

void Encoder_Driver::processInterrupt() {
    if (!_initialized) return;
    
    // Always poll the encoders (don't rely solely on interrupt)
    // This ensures we catch all transitions even if interrupt is missed
    
    // Clear interrupt flag if set
    if (_interruptPending) {
        _interruptPending = false;
    }
    
    // Read all encoder pins and process state changes
    for (uint8_t i = 0; i < ENCODER_COUNT; i++) {
        uint8_t new_state = readEncoderPins(i);
        if (new_state != _states[i].last_state) {
            uint8_t old_state = _states[i].last_state;  // Save before processing
            processEncoder(i, new_state);
            // Debug output when encoder changes
            printf("ENC%d: state 0x%02X->0x%02X, pos=%ld, dir=%d\r\n",
                   i, old_state, new_state, 
                   _states[i].position, _states[i].direction);
        }
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
