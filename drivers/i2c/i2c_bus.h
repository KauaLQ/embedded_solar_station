#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "FreeRTOS.h"
#include "semphr.h"

#define I2C_COM_PORT i2c0
#define SDA_COM_PIN  0
#define SCL_COM_PIN  1

#define I2C_BAUDRATE 100000  // 100 kHz

extern SemaphoreHandle_t i2c_mutex; // Mutex global para proteger o barramento i2c (dos sensores, não o OLED)

void i2c_bus_init(void);
void i2c_oled_init(void);

#endif