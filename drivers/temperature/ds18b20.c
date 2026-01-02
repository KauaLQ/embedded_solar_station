#include "ds18b20.h" // RTOS version

// Todas as chamadas ao driver DS18B20 são thread-safe.
// Não chamar funções ow_* diretamente fora deste driver.
static bool ds18b20_rtos_search(ds18b20_t *dev) {
    int found = ow_romsearch(&dev->ow, &dev->rom, 1, OW_SEARCH_ROM);
    dev->initialized = (found > 0);
    return dev->initialized;
}

bool ds18b20_rtos_init(ds18b20_t *dev, PIO pio, uint gpio) {
    uint offset;

    dev->pio = pio;
    dev->gpio = gpio;
    dev->initialized = false;

    if (!pio_can_add_program(pio, &onewire_program)) {
        return false;
    }

    offset = pio_add_program(pio, &onewire_program);

    if (!ow_init(&dev->ow, pio, offset, gpio)) {
        return false;
    }

    dev->mutex = xSemaphoreCreateMutex();
    if (dev->mutex == NULL) {
        return false;
    }

    return ds18b20_rtos_search(dev);
}

float ds18b20_rtos_read_temperature(ds18b20_t *dev) {
    if (xSemaphoreTake(dev->mutex, portMAX_DELAY) != pdTRUE) {
        return DS18B20_ERROR_TEMP;
    }

    float temp = DS18B20_ERROR_TEMP;

    do {
        if (!dev->initialized) {
            if (!ds18b20_rtos_search(dev)) break;
        }

        OW *ow = &dev->ow;

        if (!ow_reset(ow)) {
            dev->initialized = false;
            break;
        }

        ow_send(ow, OW_MATCH_ROM);
        for (int i = 0; i < 64; i += 8)
            ow_send(ow, dev->rom >> i);
        ow_send(ow, DS18B20_CONVERT_T);

        TickType_t start = xTaskGetTickCount();
        while (ow_read(ow) == 0) {
            if ((xTaskGetTickCount() - start) >
                pdMS_TO_TICKS(DS18B20_CONV_TIMEOUT_MS)) {
                dev->initialized = false;
                break;
            }
            vTaskDelay(pdMS_TO_TICKS(5));
        }

        if (!ow_reset(ow)) {
            dev->initialized = false;
            break;
        }

        ow_send(ow, OW_MATCH_ROM);
        for (int i = 0; i < 64; i += 8)
            ow_send(ow, dev->rom >> i);
        ow_send(ow, DS18B20_READ_SCRATCHPAD);

        int16_t raw = ow_read(ow) | (ow_read(ow) << 8);
        temp = raw / 16.0f;

    } while (0);

    xSemaphoreGive(dev->mutex);
    return temp;
}