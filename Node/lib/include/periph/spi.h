#include <libopencm3/stm32/spi.h>

#ifndef WEATHER_STATION_SPI_H
#define WEATHER_STATION_SPI_H

#define SPI_CLOCK RCC_SPI1
#define SPI SPI1
#define SPI_PORT GPIOA
#define SPI_NSS GPIO4
#define SPI_SCK GPIO5
#define SPI_MISO GPIO6
#define SPI_MOSI GPIO7

void spi_setup(void);

uint8_t spi_transfer(uint8_t data);

#endif //WEATHER_STATION_SPI_H
