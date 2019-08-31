#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

#ifndef WEATHER_STATION_I2C_H
#define WEATHER_STATION_I2C_H

#define I2C_CLOCK RCC_GPIOA
#define I2C_PORT GPIOA
#define I2C_SCL GPIO9
#define I2C_SDA GPIO10
#define I2C_CONSOLE I2C2
//#define I2C_NVIC_IRQ NVIC_I2C2_IRQ
//#define I2C_EXTI_LINE EXTI26

#endif //WEATHER_STATION_I2C_H
