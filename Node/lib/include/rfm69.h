#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "periph/spi.h"
#include "periph/sys_tick.h"

#ifndef WEATHER_STATION_RFM69_H
#define WEATHER_STATION_RFM69_H

#define RFM69_PORT GPIOA
#define RFM69_NSS GPIO4
#define RFM69_INT GPIO1
#define NETWORK_ID 100


void rfm69_setup(void);

void rfm69_read_all_regs(void);

void rfm69_sleep(void);

void rfm69_wakeup(void);

void rfm69_encryption_key(const char *key);

#endif //WEATHER_STATION_RFM69_H
