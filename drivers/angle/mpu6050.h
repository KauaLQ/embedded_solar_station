#ifndef MPU6050_H
#define MPU6050_H

#include <stdio.h>
#include <string.h>
#include <math.h>

#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "drivers/i2c/i2c_bus.h"

#define MPU6050_ADDR 0x68
#define MPU6050_REG_PWR_MGMT_1 0x6B
#define MPU6050_REG_ACCEL_XOUT_H 0x3B

void mpu6050_init();

void mpu6050_get_values(float *arrayMPU6050);

#endif