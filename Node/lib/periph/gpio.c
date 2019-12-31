#include <periph/gpio.h>

static int pin_mapping[] = {D0, D1, D2, D3, D4, D5, D6, D7};
static unsigned int port_mapping[] = {GPIO_PORT1, GPIO_PORT1, GPIO_PORT1, GPIO_PORT2, GPIO_PORT2, GPIO_PORT2,
                                      GPIO_PORT2, GPIO_PORT2};

void gpio_dio_setup(uint8_t direction, uint8_t value) {
    for (uint8_t i = 0; i < GPIO_COUNT; i++) {
        if (direction & (1 << i)) {
            gpio_mode_setup(port_mapping[i], GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, pin_mapping[i]);
            if (value & (1 << i)) {
                gpio_set(port_mapping[i], pin_mapping[i]);
            } else {
                gpio_clear(port_mapping[i], pin_mapping[i]);
            }
        } else {
            gpio_mode_setup(port_mapping[i], GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, pin_mapping[i]);
        }

    }
}

uint8_t read_gpio_value() {
    uint8_t value = 0;
    for (uint8_t i = 0; i < GPIO_COUNT; i++) {
        if (gpio_get(port_mapping[i], pin_mapping[i])) {
            value |= (1 << i);
        }
    }
    return value;
}

