#include "../include/periph/spi.h"

void spi_setup() {
    spi_reset(SPI);
    spi_disable(SPI);
    spi_init_master(SPI,
                    SPI_CR1_BAUDRATE_FPCLK_DIV_256,
                    SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                    SPI_CR1_CPHA_CLK_TRANSITION_1,
                    SPI_CR1_DFF_8BIT,
                    SPI_CR1_MSBFIRST);

    spi_disable_crc(SPI);
    spi_set_nss_high(SPI);
    spi_enable_software_slave_management(SPI);
    spi_set_full_duplex_mode(SPI);
    spi_enable(SPI);
}

uint8_t spi_transfer(uint8_t data) {
    spi_send(SPI, data);
    while (SPI_SR(SPI) & SPI_SR_BSY);
    return (uint8_t) spi_read(SPI);
}
