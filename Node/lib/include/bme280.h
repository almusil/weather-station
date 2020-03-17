#include <periph/i2c.h>
#include <stdint.h>

#ifndef WEATHER_STATION_BME280_H
#define WEATHER_STATION_BME280_H

#define BME_ADDR 0x76

#define REG_COMP1 0x88
#define REG_COMP1_LEN 24
#define REG_COMP2 0xA1
#define REG_COMP2_LEN 1
#define REG_COMP3 0xE1
#define REG_COMP3_LEN 7
#define REG_CTRL_HUM 0xF2
#define REG_STATUS 0xF3
#define REG_CTRL_MEASURE 0xF4
#define REG_CONFIG 0xF5
#define REG_PRESS 0xF7
#define REG_PRESS_LEN 3
#define REG_TEMP 0xFA
#define REG_TEMP_LEN 3
#define REG_HUMIDITY 0xFD
#define REG_HUMIDITY_LEN 2

#define STATUS_MEASURING 0x08

#define OVERSAMPLING_1X (1<<0)

#define FORCED_MODE (1<<0)


void bme_setup(void);

void bme_start_measurement(void);

bool bme_measure_done(void);

int16_t bme_get_temp(void);

uint32_t bme_get_pressure(void);

uint16_t bme_get_humidity(void);

#endif //WEATHER_STATION_BME280_H
