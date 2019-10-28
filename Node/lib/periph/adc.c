#include <periph/adc.h>

static uint32_t adc_get_channel_configuration(uint8_t channels);

void adc_setup() {
    adc_power_off(ADC);

    ADC_CFGR2(ADC) &= ~ADC_CFGR2_CKMODE_PCLK;
    ADC_CFGR2(ADC) |= ADC_CFGR2_CKMODE_PCLK_DIV4;

    adc_set_single_conversion_mode(ADC);
    adc_set_right_aligned(ADC);
    adc_set_resolution(ADC, ADC_CFGR1_RES_12_BIT);

    //101: 39.5 ADC clock cycle
    ADC_SMPR1(ADC) |= (1 << 2) | (1 << 0);

    adc_calibrate(ADC);
    adc_power_on(ADC);

    ADC_CFGR1(ADC) |= ADC_CFGR1_WAIT | ADC_CFGR1_AUTOFF;
}

void adc_convert(uint8_t channels, uint16_t *result, uint8_t len) {
    ADC_CHSELR(ADC) = adc_get_channel_configuration(channels);
    adc_start_conversion_regular(ADC);

    for (uint8_t i = 0; i < len; i++) {
        if (channels & (1 << i)) {
            while (!(ADC_ISR(ADC) & ADC_ISR_EOC));
            result[i] = ADC_DR(ADC);
        } else {
            result[i] = 0;
            continue;
        }
    }
}

static uint32_t adc_get_channel_configuration(uint8_t channels) {
    uint32_t chan_select = 0;

    if (channels & A0_CHANNEL) {
        chan_select |= A0_INTER_CHANNEL;
    }

    if (channels & A1_CHANNEL) {
        chan_select |= A1_INTER_CHANNEL;
    }

    if (channels & A2_CHANNEL) {
        chan_select |= A2_INTER_CHANNEL;
    }

    if (channels & BAT_CHANNEL) {
        chan_select |= BAT_INTER_CHANNEL;
    }

    return chan_select;
}
