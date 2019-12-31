#include <libopencm3/stm32/gpio.h>

#ifndef WEATHER_STATION_GPIO_H
#define WEATHER_STATION_GPIO_H

#define GPIO_COUNT 8

#define GPIO_PORT1 GPIOA
#define D0 GPIO10
#define D1 GPIO9
#define D2 GPIO15

#define GPIO_PORT2 GPIOB
#define D3 GPIO3
#define D4 GPIO4
#define D5 GPIO5
#define D6 GPIO6
#define D7 GPIO7

void gpio_dio_setup(uint8_t direction, uint8_t value);

uint8_t read_gpio_value(void);

#endif //WEATHER_STATION_GPIO_H
