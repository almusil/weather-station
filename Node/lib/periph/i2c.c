#include "../include/periph/i2c.h"

static void i2c_setup_gpio(void);

void i2c_setup() {
    rcc_periph_clock_enable(RCC_I2C1);
    i2c_peripheral_disable(I2C1);

    i2c_enable_analog_filter(I2C1);
    i2c_set_digital_filter(I2C1, 0);

    i2c_set_speed(I2C1, i2c_speed_sm_100k, 16);
    //addressing mode
    i2c_set_7bit_addr_mode(I2C1);
    i2c_peripheral_enable(I2C1);
}

void i2c_setup_gpio() {
    // Enable clock
    rcc_periph_clock_enable(I2C_CLOCK);

    // Set alternate functions
    gpio_mode_setup(I2C_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, I2C_SDA | I2C_SCL);

    gpio_set_af(I2C_PORT, GPIO_AF1, I2C_SDA | I2C_SCL);
}
