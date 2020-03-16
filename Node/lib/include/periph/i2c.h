#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/i2c.h>

#include <periph/sys_tick.h>
#include <stdio.h>

#ifndef WEATHER_STATION_I2C_H
#define WEATHER_STATION_I2C_H

#define I2C I2C1
#define I2C_CLOCK RCC_I2C1
#define I2C_PORT GPIOA
#define I2C_SDA GPIO10
#define I2C_SCL GPIO9
#define I2C_TIMEOUT 50

void i2c_setup(void);

void i2c_disable(void);

void i2c_enable(void);

void i2c_transfer(uint8_t addr, uint8_t *w, size_t wn, uint8_t *r, size_t rn);

#endif //WEATHER_STATION_I2C_H
