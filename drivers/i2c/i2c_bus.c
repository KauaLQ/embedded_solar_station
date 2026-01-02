#include "i2c_bus.h"

SemaphoreHandle_t i2c_mutex;

void i2c_bus_init(void) {
    // Como esse é o barramento i2c onde estão os sensores, inicializamos o mutex aqui
    i2c_mutex = xSemaphoreCreateMutex();
    configASSERT(i2c_mutex != NULL);

    i2c_init(I2C_COM_PORT, I2C_BAUDRATE);
    gpio_set_function(SDA_COM_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_COM_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_COM_PIN);
    gpio_pull_up(SCL_COM_PIN);
}

void i2c_oled_init(void) {
    i2c_init(i2c_default, 400 * 1000);
    gpio_set_function(PICO_DEFAULT_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(PICO_DEFAULT_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(PICO_DEFAULT_I2C_SDA_PIN);
    gpio_pull_up(PICO_DEFAULT_I2C_SCL_PIN);
}