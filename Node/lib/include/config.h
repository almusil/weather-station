#ifndef WEATHER_STATION_CONFIG_H
#define WEATHER_STATION_CONFIG_H

#define PACKET_CONFIG 0x02
#define PACKET_DATA 0x08
#define GATEWAY_ADDR 1
#define CONFIG_TIMEOUT 1000
#define CONFIG_LENGTH 5
#define ACK_TIMEOUT 100
#define ACK_RETRY_COUNT 5

#define SW_PORT GPIOA
#define SW_PIN GPIO8

struct config {
    uint16_t sleep_time;
    uint8_t dio_direction;
    uint8_t dio_value;
    uint8_t analog;
};


struct __attribute__((__packed__)) data_t {
    uint8_t packet_type;
    uint8_t gpio_value;
    uint16_t adc_value[4];
    int16_t temperature;
    uint32_t pressure;
    uint16_t humidity;
};

union packet_t {
    struct data_t data;
    uint8_t bytes[sizeof(struct data_t)];
};

#endif //WEATHER_STATION_CONFIG_H
