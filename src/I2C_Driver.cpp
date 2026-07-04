#include "I2C_Driver.h"

// Global I2C mutex — shared by ALL drivers on Wire bus
static SemaphoreHandle_t _i2cGlobalMutex = NULL;

void I2C_Init(void) {
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Wire.setClock(400000);  // Fast-mode I2C: all devices on this bus support 400kHz.
                          // At the default 100kHz the DriverTask held the bus
                          // (and its mutex) for most of every 20ms cycle.
  
  // Create the global I2C mutex (once)
  if (_i2cGlobalMutex == NULL) {
    _i2cGlobalMutex = xSemaphoreCreateMutex();
    if (_i2cGlobalMutex == NULL) {
      printf("I2C: FATAL - Failed to create I2C mutex\r\n");
    }
  }
}

SemaphoreHandle_t I2C_GetMutex(void) {
  return _i2cGlobalMutex;
}

bool I2C_MutexTake(uint32_t timeout_ms) {
  if (_i2cGlobalMutex == NULL) return false;
  return xSemaphoreTake(_i2cGlobalMutex, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void I2C_MutexGive(void) {
  if (_i2cGlobalMutex != NULL) {
    xSemaphoreGive(_i2cGlobalMutex);
  }
}

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length)
{
  if (!I2C_MutexTake(50)) {
    printf("I2C Read: mutex timeout for addr 0x%02X\r\n", Driver_addr);
    return -1;
  }
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr); 
  if ( Wire.endTransmission(true)){
    printf("The I2C transmission fails. - I2C Read\r\n");
    I2C_MutexGive();
    return -1;
  }
  Wire.requestFrom(Driver_addr, Length);
  for (int i = 0; i < Length; i++) {
    *Reg_data++ = Wire.read();
  }
  I2C_MutexGive();
  return 0;
}
bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length)
{
  if (!I2C_MutexTake(50)) {
    printf("I2C Write: mutex timeout for addr 0x%02X\r\n", Driver_addr);
    return -1;
  }
  Wire.beginTransmission(Driver_addr);
  Wire.write(Reg_addr);       
  for (int i = 0; i < Length; i++) {
    Wire.write(*Reg_data++);
  }
  if ( Wire.endTransmission(true))
  {
    printf("The I2C transmission fails. - I2C Write\r\n");
    I2C_MutexGive();
    return -1;
  }
  I2C_MutexGive();
  return 0;
}