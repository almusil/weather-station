#include "include/rfm69.h"
#include "include/rfm69_defs.h"

enum rfm69_mode {
    STANDBY = 1,
    TX,
    RX,
    SYNTH,
    SLEEP
};

struct state {
    enum rfm69_mode mode;
};

static struct state current_state;

static uint8_t read_reg(uint8_t addr);

static void write_reg(uint8_t addr, uint8_t val);

static void spi_chip_select(void);

static void spi_chip_unselect(void);

static void rfm69_interrupt_setup(void);

static void rfm69_interrupt_disable(void);

static void rfm69_set_mode(enum rfm69_mode mode);

static const uint8_t CONFIG[][2] =
        {
                /* 0x01 */ {REG_OPMODE,        RF_OPMODE_SEQUENCER_ON | RF_OPMODE_LISTEN_OFF | RF_OPMODE_STANDBY},
                /* 0x02 */
                           {REG_DATAMODUL,     RF_DATAMODUL_DATAMODE_PACKET | RF_DATAMODUL_MODULATIONTYPE_FSK |
                                               RF_DATAMODUL_MODULATIONSHAPING_00},
                /* 0x03 */
                           {REG_BITRATEMSB,    RF_BITRATEMSB_55555},
                /* 0x04 */
                           {REG_BITRATELSB,    RF_BITRATELSB_55555},
                /* 0x05 */
                           {REG_FDEVMSB,       RF_FDEVMSB_50000},
                /* 0x06 */
                           {REG_FDEVLSB,       RF_FDEVLSB_50000},

                /* 0x07 */
                           {REG_FRFMSB,        RF_FRFMSB_433},
                /* 0x08 */
                           {REG_FRFMID,        RF_FRFMID_433},
                /* 0x09 */
                           {REG_FRFLSB,        RF_FRFLSB_433},

                /* 0x19 */
                           {REG_RXBW,          RF_RXBW_DCCFREQ_010 | RF_RXBW_MANT_16 | RF_RXBW_EXP_2},
                /* 0x25 */
                           {REG_DIOMAPPING1,   RF_DIOMAPPING1_DIO0_01},
                /* 0x26 */
                           {REG_DIOMAPPING2,   RF_DIOMAPPING2_CLKOUT_OFF},
                /* 0x28 */
                           {REG_IRQFLAGS2,     RF_IRQFLAGS2_FIFOOVERRUN},
                /* 0x29 */
                           {REG_RSSITHRESH,    220},
                /* 0x2E */
                           {REG_SYNCCONFIG,    RF_SYNC_ON | RF_SYNC_FIFOFILL_AUTO | RF_SYNC_SIZE_2 | RF_SYNC_TOL_0},
                /* 0x2F */
                           {REG_SYNCVALUE1,    0x2D},
                /* 0x30 */
                           {REG_SYNCVALUE2,    NETWORK_ID},
                /* 0x37 */
                           {REG_PACKETCONFIG1, RF_PACKET1_FORMAT_VARIABLE | RF_PACKET1_DCFREE_OFF | RF_PACKET1_CRC_ON |
                                               RF_PACKET1_CRCAUTOCLEAR_ON | RF_PACKET1_ADRSFILTERING_OFF},
                /* 0x38 */
                           {REG_PAYLOADLENGTH, 66},
                /* 0x3C */
                           {REG_FIFOTHRESH,    RF_FIFOTHRESH_TXSTART_FIFONOTEMPTY | RF_FIFOTHRESH_VALUE},
                /* 0x3D */
                           {REG_PACKETCONFIG2, RF_PACKET2_RXRESTARTDELAY_2BITS | RF_PACKET2_AUTORXRESTART_ON |
                                               RF_PACKET2_AES_OFF},
                /* 0x6F */
                           {REG_TESTDAGC,      RF_DAGC_IMPROVED_LOWBETA0},
                           {255,               0}
        };

void rfm69_setup() {
    memset(&current_state, 0, sizeof(struct state));
    rfm69_interrupt_setup();

    //Wait for sync
    uint64_t start = get_millis();
    uint8_t timeout = 50;

    do {
        write_reg(REG_SYNCVALUE1, 0xAA);
    } while (read_reg(REG_SYNCVALUE1) != 0xaa && get_millis() - start < timeout);
    start = get_millis();
    do {
        write_reg(REG_SYNCVALUE1, 0x55);
    } while (read_reg(REG_SYNCVALUE1) != 0x55 && get_millis() - start < timeout);

    for (uint8_t i = 0; CONFIG[i][0] != 255; i++) {
        write_reg(CONFIG[i][0], CONFIG[i][1]);
    }

    //Clear encryption just to be sure because the key is stored between reboots
    rfm69_encryption_key(0);

    rfm69_set_mode(STANDBY);
    // Wait for mode ready
    start = get_millis();
    while (((read_reg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00) && get_millis() - start < timeout);
}

void rfm69_read_all_regs() {
    printf("RFM69 read all regs!\n");
    for (uint8_t addr = 1; addr <= 0x4f; addr++) {
        printf("Reg: %02x - val: %02x \n", addr, read_reg(addr));
    }
}

void rfm69_encryption_key(const char *key) {
    rfm69_set_mode(STANDBY);
    if (key != 0) {
        spi_chip_select();
        spi_xfer(SPI, REG_AESKEY1 | 0x80);
        for (uint8_t i = 0; i < 16; i++)
            spi_xfer(SPI, key[i]);
        spi_chip_unselect();
    }
    write_reg(REG_PACKETCONFIG2, (read_reg(REG_PACKETCONFIG2) & 0xFE) | (key ? 1 : 0));
}

static void rfm69_set_mode(enum rfm69_mode mode) {
    if(current_state.mode == mode) {
        return;
    }

    switch (mode) {
        case TX:
            write_reg(REG_OPMODE, (read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_TRANSMITTER);
            break;
        case RX:
            write_reg(REG_OPMODE, (read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_RECEIVER);
            break;
        case SYNTH:
            write_reg(REG_OPMODE, (read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_SYNTHESIZER);
            break;
        case STANDBY:
            write_reg(REG_OPMODE, (read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_STANDBY);
            break;
        case SLEEP:
            write_reg(REG_OPMODE, (read_reg(REG_OPMODE) & 0xE3) | RF_OPMODE_SLEEP);
            break;
    }

    // Wait for proper wakeup from sleep
    while (current_state.mode == SLEEP && (read_reg(REG_IRQFLAGS1) & RF_IRQFLAGS1_MODEREADY) == 0x00);

    current_state.mode = mode;
}

void rfm69_sleep() {
    rfm69_interrupt_disable();
    rfm69_set_mode(SLEEP);
}

void rfm69_wakeup() {
    rfm69_interrupt_setup();
    rfm69_set_mode(STANDBY);
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

void exti0_1_isr() {
    exti_reset_request(EXTI1);
    // Handle interrupt
    printf("Interrupt incoming!\n");
}
