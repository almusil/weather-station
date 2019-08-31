#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <stdint.h>
#include <stdio.h>

#include "periph/spi.h"

#ifndef WEATHER_STATION_RFM69_H
#define WEATHER_STATION_RFM69_H

#define RFM69_PORT GPIOA
#define RFM69_NSS GPIO4

void rfm69_setup(void);

void rfm69_read_all_regs(void);

#endif //WEATHER_STATION_RFM69_H
