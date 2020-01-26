#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#ifndef WEATHER_STATION_USART_H
#define WEATHER_STATION_USART_H

#define USART_SPEED 115200
#define USART_CLOCK RCC_USART2
#define USART_PORT GPIOA
#define USART_RX GPIO3
#define USART_TX GPIO2
#define USART_CONSOLE USART2
#define USART_NVIC_IRQ NVIC_USART2_IRQ
#define USART_EXTI_LINE EXTI26

void usart_setup(uint32_t baud);

void usart_interrupt_enable(void);

void usart_interrupt_disable(void);

void println(char *str);

#endif //WEATHER_STATION_USART_H
