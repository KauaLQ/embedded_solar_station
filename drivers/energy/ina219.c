#include "ina219.h"

// funções 12c
void ina219_write_register(uint8_t reg, uint16_t value) {
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = value & 0xFF;
    i2c_write_blocking(I2C_COM_PORT, INA219_ADDR, buf, 3, false);
}

uint16_t ina219_read_register(uint8_t reg) {
    uint8_t buf[2];
    i2c_write_blocking(I2C_COM_PORT, INA219_ADDR, &reg, 1, true);
    i2c_read_blocking(I2C_COM_PORT, INA219_ADDR, buf, 2, false);
    return (buf[0] << 8) | buf[1];
}

// init INA219
void ina219_init() {
    // Configuração padrão:
    // Bus voltage range: 32V
    // Gain: ±320mV
    // ADC: 12 bits
    uint16_t config = 0x019F;
    ina219_write_register(REG_CONFIG, config);

    // Calibração (exemplo para shunt 0.1Ω e até ~3.2A)
    // Current LSB ≈ 100uA
    // uint16_t calibration = 40960;
    ina219_write_register(REG_CALIBRATION, INA219_CALIBRATION);
}

// leituras
float ina219_get_bus_voltage() {
    uint16_t raw = ina219_read_register(REG_BUS_VOLTAGE);
    raw >>= 3;                 // bits úteis
    return raw * 0.004f;       // 4mV por bit
}

float ina219_get_shunt_voltage() {
    int16_t raw = ina219_read_register(REG_SHUNT_VOLTAGE);
    return raw * 0.00001f;     // 10uV por bit
}

float ina219_get_current() {
    // garante que calibração está ativa
    ina219_write_register(REG_CALIBRATION, INA219_CALIBRATION);

    int16_t raw = (int16_t)ina219_read_register(REG_CURRENT);
    return raw * INA219_CURRENT_LSB;
}

float ina219_get_power() {
    ina219_write_register(REG_CALIBRATION, INA219_CALIBRATION);

    uint16_t raw = ina219_read_register(REG_POWER);
    return raw * INA219_POWER_LSB;
}

void ina219_get_values(float *arrayINA219){
    arrayINA219[0] = ina219_get_bus_voltage();
    arrayINA219[1] = ina219_get_shunt_voltage();
    arrayINA219[2] = ina219_get_current();
    arrayINA219[3] = ina219_get_power();
}