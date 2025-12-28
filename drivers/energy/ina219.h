#ifndef INA219_H
#define INA219_H

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "drivers/i2c/i2c_bus.h"

// Macros do INA219
#define INA219_ADDR 0x40
#define INA219_RSHUNT 0.136f
#define INA219_CURRENT_LSB 0.0001f  // 100uA
#define INA219_POWER_LSB   (20 * INA219_CURRENT_LSB)
#define INA219_CALIBRATION \
    (uint16_t)(0.04096f / (INA219_CURRENT_LSB * INA219_RSHUNT))

// Registradores INA219
#define REG_CONFIG        0x00
#define REG_SHUNT_VOLTAGE 0x01
#define REG_BUS_VOLTAGE   0x02
#define REG_POWER         0x03
#define REG_CURRENT       0x04
#define REG_CALIBRATION   0x05

void ina219_init();
void ina219_get_values(float *arrayINA219);

#endif