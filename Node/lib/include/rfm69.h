#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "periph/spi.h"
#include "periph/sys_tick.h"
#include "rfm69_defs.h"

#ifndef WEATHER_STATION_RFM69_H
#define WEATHER_STATION_RFM69_H

#define RFM69_PORT GPIOA
#define RFM69_NSS GPIO4
#define RFM69_INT GPIO11
#define RFM69_EXTI_LINE EXTI11
#define RFM69_NVIC_IRQ NVIC_EXTI4_15_IRQ
#define NETWORK_ID 100
#define NODE_ADDR 10

struct rfm69_packet {
    uint8_t sender_id;
    uint8_t target_id;
    uint8_t data_len;
    uint8_t data_buffer[RF69_MAX_DATA_LEN + 1];
};

void rfm69_setup(void);

void rfm69_read_all_regs(void);

void rfm69_sleep(void);

void rfm69_wakeup(void);

void rfm69_encryption_key(const char *key);

void rfm69_send(uint8_t to_addr, const void* buffer, uint8_t len, bool request_ack);

bool rfm69_receive_done(void);

bool rfm69_ack_requested(void);

void rfm69_send_ack(void);

bool rfm69_ack_received(uint8_t from_addr);

void rfm69_get_data(struct rfm69_packet *packet);

#endif //WEATHER_STATION_RFM69_H
