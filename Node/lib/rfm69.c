#include "include/rfm69.h"
#include "include/rfm69_defs.h"

static uint8_t read_reg(uint8_t addr);

static void write_reg(uint8_t addr, uint8_t val);

static void spi_chip_select(void);

static void spi_chip_unselect(void);

static void rfm69_interrupt_setup(void);

static void rfm69_interrupt_disable(void);

static void rfm69_interrupt_setup() {

    exti_select_source(RFM69_INT, RFM69_PORT);
    exti_set_trigger(RFM69_INT, EXTI_TRIGGER_RISING);
    exti_reset_request(RFM69_INT);

    nvic_enable_irq(NVIC_EXTI0_1_IRQ);
    exti_enable_request(EXTI1);
}

static void rfm69_interrupt_disable() {
    nvic_disable_irq(NVIC_EXTI0_1_IRQ);
    exti_disable_request(EXTI1);
}

static void spi_chip_select() {
    gpio_clear(RFM69_PORT, RFM69_NSS);
}

static void spi_chip_unselect() {
    while (SPI_SR(SPI) & SPI_SR_BSY);
    gpio_set(RFM69_PORT, RFM69_NSS);
}

void rfm69_setup() {
    spi_setup();
    rfm69_interrupt_setup();
}

static uint8_t read_reg(uint8_t addr) {
    spi_chip_select();
    spi_xfer(SPI, addr & 0x7f);
    uint8_t val = spi_xfer(SPI, 0);
    spi_chip_unselect();

    return val;
}

static void write_reg(uint8_t addr, uint8_t val) {
    if (addr > 0x7f) {
        return;
    }

    spi_chip_select();
    spi_xfer(SPI, addr | 0x80);
    spi_xfer(SPI, val);
    spi_chip_unselect();
}

void rfm69_read_all_regs() {
    printf("RFM69 read all regs!\n");
    for (uint8_t addr = 1; addr <= 0x4f; addr++) {
        printf("Reg: %02x - val: %02x \n", addr, read_reg(addr));
    }
}

void rfm69_sleep() {
    rfm69_interrupt_disable();
    //TODO Set rfm69 to sleep
}

void rfm69_wakeup() {
    rfm69_interrupt_setup();
    //TODO Wakeup rfm69
}

void exti0_1_isr() {
    exti_reset_request(EXTI1);
    // Handle interrupt
    printf("Interrupt incoming!\n");
}
