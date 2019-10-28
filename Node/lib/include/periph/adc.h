#include <libopencm3/stm32/adc.h>

#ifndef WEATHER_STATION_ADC_H
#define WEATHER_STATION_ADC_H

#define ADC ADC1
#define ADC_CLOCK RCC_ADC1
#define ADC_PORT1 GPIOA
#define ADC_PORT2 GPIOB
#define A0 GPIO0
#define A1 GPIO1
#define A2 GPIO1
#define BAT_ADC GPIO0
#define ADC_ISR_EOCAL (1 << 11)

#define A0_INTER_CHANNEL (1 << 0)
#define A1_INTER_CHANNEL (1 << 1)
#define A2_INTER_CHANNEL (1 << 9)
#define BAT_INTER_CHANNEL (1 << 8)

#define A0_CHANNEL (1 << 0)
#define A1_CHANNEL (1 << 1)
#define A2_CHANNEL (1 << 2)
#define BAT_CHANNEL (1 << 3)

void adc_setup(void);

void adc_convert(uint8_t channels, uint16_t *result, uint8_t len);


#endif //WEATHER_STATION_ADC_H
