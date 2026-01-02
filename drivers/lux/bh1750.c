#include "bh1750.h"

/* ---------------- BH1750 ---------------- */
void bh1750_cmd_init() {
    uint8_t power_on_cmd = 0x01;
    uint8_t cont_high_res_mode = 0x10;
    i2c_write_blocking(I2C_COM_PORT, BH1750_ADDR, &power_on_cmd, 1, false);
    vTaskDelay(pdMS_TO_TICKS(10));
    i2c_write_blocking(I2C_COM_PORT, BH1750_ADDR, &cont_high_res_mode, 1, false);
}

float bh1750_read_lux() {
    uint8_t buffer[2];
    if (i2c_read_blocking(I2C_COM_PORT, BH1750_ADDR, buffer, 2, false) != 2)
        return -1.0;
    uint16_t raw = (buffer[0] << 8) | buffer[1];
    return raw / 1.2;
}

/* ---------------- MUX ------------------- */
void mux_select_chanel(uint8_t channel) {
    if (channel > 7) return;
    uint8_t data = 1 << channel;
    i2c_write_blocking(I2C_COM_PORT, PCA9548A_ADDR, &data, 1, false);
}

void mux_sweep(float *arrayBH1750) {
    for (int i = 5; i < 8; i++) {
        if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {

            mux_select_chanel(i);
            vTaskDelay(pdMS_TO_TICKS(15));

            arrayBH1750[i - 5] = bh1750_read_lux();

            xSemaphoreGive(i2c_mutex);
        }
    }
}

void bh1750_initialize(){
    for (int i = 5; i < 8; i++) {
        if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(50)) == pdTRUE) {

            mux_select_chanel(i);
            vTaskDelay(pdMS_TO_TICKS(15));

            bh1750_cmd_init();
            vTaskDelay(pdMS_TO_TICKS(180));

            xSemaphoreGive(i2c_mutex);
        }
    }
}