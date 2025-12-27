#include "mpu6050.h"

// Função para escrever no registrador
void mpu6050_write(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    i2c_write_blocking(I2C_COM_PORT, MPU6050_ADDR, buf, 2, false);
}

// Função para ler blocos do MPU6050
void mpu6050_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    i2c_write_blocking(I2C_COM_PORT, MPU6050_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_COM_PORT, MPU6050_ADDR, buf, len, false);
}

// Inicializa o MPU6050
void mpu6050_init() {
    mpu6050_write(MPU6050_REG_PWR_MGMT_1, 0x00); // Desliga sleep
    sleep_ms(100);
}

// Converte 2 bytes em inteiro de 16 bits
int16_t combine_bytes(uint8_t msb, uint8_t lsb) {
    return (int16_t)((msb << 8) | lsb);
}

void mpu6050_get_values(float *arrayMPU6050){
    uint8_t buf[14];
    mpu6050_read(MPU6050_REG_ACCEL_XOUT_H, buf, 14);

    int16_t accel_x = combine_bytes(buf[0], buf[1]);
    int16_t accel_y = combine_bytes(buf[2], buf[3]);
    int16_t accel_z = combine_bytes(buf[4], buf[5]);

    // Conversão para g (assumindo sensibilidade padrão ±2g -> 16384 LSB/g)
    float ax = accel_x / 16384.0f;
    float ay = accel_y / 16384.0f;
    float az = accel_z / 16384.0f;

    // Cálculo simples de inclinação (em graus)
    float pitch = atan2(ax, sqrt(ay * ay + az * az)) * 180.0 / M_PI;
    float roll  = atan2(ay, sqrt(ax * ax + az * az)) * 180.0 / M_PI;

    arrayMPU6050[0] = pitch;
    arrayMPU6050[1] = roll;
}