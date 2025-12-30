#include "ds18b20.h"

static bool ds18b20_search(ds18b20_t *dev) {
    int found = ow_romsearch(&dev->ow, &dev->rom, 1, OW_SEARCH_ROM);
    dev->initialized = (found > 0);
    return dev->initialized;
}

bool ds18b20_init(ds18b20_t *dev, PIO pio, uint gpio) {
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

    return ds18b20_search(dev);
}

float ds18b20_read_temperature(ds18b20_t *dev) {

    // Se não inicializado, tenta reconectar
    if (!dev->initialized) {
        if (!ds18b20_search(dev)) {
            return DS18B20_ERROR_TEMP;
        }
    }

    OW *ow = &dev->ow;

    // Sensor sumiu?
    if (!ow_reset(ow)) {
        dev->initialized = false;
        return DS18B20_ERROR_TEMP;
    }

    // Start conversion
    ow_send(ow, OW_MATCH_ROM);
    for (int i = 0; i < 64; i += 8)
        ow_send(ow, dev->rom >> i);
    ow_send(ow, DS18B20_CONVERT_T);

    // Timeout de conversão
    absolute_time_t start = get_absolute_time();
    while (ow_read(ow) == 0) {
        if (absolute_time_diff_us(start, get_absolute_time()) >
            DS18B20_CONV_TIMEOUT_MS * 1000) {
            dev->initialized = false;
            return DS18B20_ERROR_TEMP;
        }
    }

    // Read scratchpad
    if (!ow_reset(ow)) {
        dev->initialized = false;
        return DS18B20_ERROR_TEMP;
    }

    ow_send(ow, OW_MATCH_ROM);
    for (int i = 0; i < 64; i += 8)
        ow_send(ow, dev->rom >> i);
    ow_send(ow, DS18B20_READ_SCRATCHPAD);

    int16_t raw = ow_read(ow) | (ow_read(ow) << 8);
    return raw / 16.0f;
}