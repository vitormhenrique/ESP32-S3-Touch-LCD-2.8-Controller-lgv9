#pragma once
#include <Wire.h> 
#include <freertos/semphr.h>

#define I2C_SCL_PIN       10
#define I2C_SDA_PIN       11

void I2C_Init(void);

/**
 * @brief Get the global I2C mutex for thread-safe bus access.
 * All drivers sharing Wire must take this mutex before any I2C transaction.
 */
SemaphoreHandle_t I2C_GetMutex(void);

/**
 * @brief Convenience: take I2C mutex with timeout.
 * @return true if mutex acquired
 */
bool I2C_MutexTake(uint32_t timeout_ms = 50);

/**
 * @brief Convenience: release I2C mutex.
 */
void I2C_MutexGive(void);

bool I2C_Read(uint8_t Driver_addr, uint8_t Reg_addr, uint8_t *Reg_data, uint32_t Length);
bool I2C_Write(uint8_t Driver_addr, uint8_t Reg_addr, const uint8_t *Reg_data, uint32_t Length);