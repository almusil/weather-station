#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/cm3/systick.h>

#ifndef WEATHER_STATION_SYS_TICK_H
#define WEATHER_STATION_SYS_TICK_H

void sys_tick_setup(void);

void delay(uint32_t ms);

uint64_t get_millis(void);

void sys_tick_enable(void);

void sys_tick_disable(void);

#endif //WEATHER_STATION_SYS_TICK_H
