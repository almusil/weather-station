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

#endif //WEATHER_STATION_CONFIG_H
