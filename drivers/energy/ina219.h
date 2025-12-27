#ifndef INA219_H
#define INA219_H

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "drivers/i2c/i2c_bus.h"

#define INA219_ADDR 0x40

// Registradores INA219
#define REG_CONFIG        0x00
#define REG_SHUNT_VOLTAGE 0x01
#define REG_BUS_VOLTAGE   0x02
#define REG_POWER         0x03
#define REG_CURRENT       0x04
#define REG_CALIBRATION   0x05

void ina219_init();
float ina219_get_values(float *arrayINA219);

#endif