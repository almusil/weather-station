#include "include/rfm69.h"

static uint8_t read_reg(uint8_t addr);

static void write_reg(uint8_t addr, uint8_t val);

static void spi_chip_select(void);

static void spi_chip_unselect(void);

static void spi_chip_select() {
    gpio_clear(RFM69_PORT, RFM69_NSS);
}

static void spi_chip_unselect() {
    gpio_set(RFM69_PORT, RFM69_NSS);
}

void rfm69_setup() {
    spi_setup();
}

static uint8_t read_reg(uint8_t addr) {
    spi_chip_select();
    uint8_t val = spi_transfer(addr & 0x7f);
    spi_chip_unselect();

    return val;
}

static void write_reg(uint8_t addr, uint8_t val) {
    if (addr > 0x7f) {
        return;
    }

    spi_chip_select();
    spi_transfer(addr | 0x80);
    spi_transfer(val);
    spi_chip_unselect();
}

void rfm69_read_all_regs() {
    printf("RFM69 read all regs!\n");
    for (uint8_t addr = 1; addr <= 0x4f; addr++) {
        printf("Reg: %02x - val: %02x \n", addr, read_reg(addr));
    }
}
