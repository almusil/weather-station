#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <stdint.h>

#ifndef WEATHER_STATION_RTC_H
#define WEATHER_STATION_RTC_H

void rtc_setup(void);

void rtc_wakeup_setup(uint16_t seconds);

void rtc_disable(void);

#endif //WEATHER_STATION_RTC_H
