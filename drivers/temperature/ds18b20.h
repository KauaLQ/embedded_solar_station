// Para mais informações sobre o DS18B20 consulte: 
// https://www.analog.com/en/products/ds18b20.html

#ifndef DS18B20_DRIVER_H
#define DS18B20_DRIVER_H

#include <stdbool.h>
#include "hardware/pio.h"
#include "pico/time.h"
#include "drivers/onewire_library/onewire_library.h"
#include "drivers/onewire_library/ow_rom.h"

#define DS18B20_CONVERT_T           0x44
#define DS18B20_WRITE_SCRATCHPAD    0x4e
#define DS18B20_READ_SCRATCHPAD     0xbe
#define DS18B20_COPY_SCRATCHPAD     0x48
#define DS18B20_RECALL_EE           0xb8
#define DS18B20_READ_POWER_SUPPLY   0xb4

#define DS18B20_ERROR_TEMP   (-999.0f)
#define DS18B20_CONV_TIMEOUT_MS  800   // máx. para 12 bits

typedef struct {
    OW ow;
    uint64_t rom;
    bool initialized;
    PIO pio;
    uint gpio;
} ds18b20_t;

bool ds18b20_init(ds18b20_t *dev, PIO pio, uint gpio);
float ds18b20_read_temperature(ds18b20_t *dev);

#endif
