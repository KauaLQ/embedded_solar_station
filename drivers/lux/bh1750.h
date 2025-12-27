#ifndef BH1750_H
#define BH1750_H

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "drivers/i2c/i2c_bus.h"

#define BH1750_ADDR 0x23
#define PCA9548A_ADDR 0x70

void mux_sweep(float *arrayBH1750);

void bh1750_initialize();

#endif