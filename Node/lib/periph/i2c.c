#include <periph/i2c.h>

void i2c_setup() {
    i2c_reset(I2C);
    i2c_disable();

    i2c_enable_analog_filter(I2C);
    i2c_set_digital_filter(I2C, 0);
    i2c_enable_stretching(I2C);
    i2c_set_speed(I2C, i2c_speed_sm_100k, rcc_apb1_frequency);
    i2c_set_7bit_addr_mode(I2C);

    i2c_enable();
}

void i2c_disable() {
    i2c_peripheral_disable(I2C);
}

void i2c_enable() {
    i2c_peripheral_enable(I2C);
}

void i2c_transfer(uint8_t addr, uint8_t *w, size_t wn, uint8_t *r, size_t rn) {
    /*  waiting for busy is unnecessary. read the RM */
    if (wn) {
        i2c_set_7bit_address(I2C, addr);
        i2c_set_write_transfer_dir(I2C);
        i2c_set_bytes_to_transfer(I2C, wn);
        if (rn) {
            i2c_disable_autoend(I2C);
        } else {
            i2c_enable_autoend(I2C);
        }
        i2c_send_start(I2C);

        while (wn--) {
            bool wait = true;
            while (wait) {
                if (i2c_transmit_int_status(I2C)) {
                    wait = false;
                }
                uint64_t now = get_millis();
                bool nack = i2c_nack(I2C);
                while (nack && get_millis() - now < I2C_TIMEOUT) {
                    nack = i2c_nack(I2C);
                }
                if (nack) {
                    printf("Error getting I2C ACK!\n");
                    return;
                }
            }
            i2c_send_data(I2C, *w++);
        }
        /* not entirely sure this is really necessary.
         * RM implies it will stall until it can write out the later bits
         */
        if (rn) {
            while (!i2c_transfer_complete(I2C));
        }
    }

    if (rn) {
        /* Setting transfer properties */
        i2c_set_7bit_address(I2C, addr);
        i2c_set_read_transfer_dir(I2C);
        i2c_set_bytes_to_transfer(I2C, rn);
        /* start transfer */
        i2c_send_start(I2C);
        /* important to do it afterwards to do a proper repeated start! */
        i2c_enable_autoend(I2C);

        for (size_t i = 0; i < rn; i++) {
            while (i2c_received_data(I2C) == 0);
            r[i] = i2c_get_data(I2C);
        }
    }
}
