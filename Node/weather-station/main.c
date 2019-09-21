#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>

#include <periph/sys_tick.h>
#include <periph/rtc.h>
#include <periph/low_power.h>
#include <periph/spi.h>
#include <periph/usart.h>
#include <rfm69.h>

#define SLEEP_TIME 30

static void clock_setup(void);

static void gpio_setup(void);

static void setup(void);

static void deep_sleep(void);

static void before_sleep(void);

static void after_wakeup(void);

static void clock_setup() {
    //Setup clock to internal 16MHz
    rcc_osc_on(RCC_HSI16);
    rcc_wait_for_osc_ready(RCC_HSI16);

    //Enable SysClock to 16Mhz
    rcc_set_sysclk_source(RCC_HSI16);

    rcc_ahb_frequency = 16e6;
    rcc_apb1_frequency = 16e6;
    rcc_apb2_frequency = 16e6;

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(SPI_CLOCK);
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(USART_CLOCK);
}


static void gpio_setup() {
    /*  SPI GPIO setup */
    gpio_mode_setup(SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK | SPI_MISO | SPI_MOSI);
    gpio_set_output_options(SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_VERYHIGH, SPI_SCK | SPI_MISO | SPI_MOSI);
    gpio_set_af(SPI_PORT, GPIO_AF0, SPI_SCK | SPI_MISO | SPI_MOSI);

    gpio_clear(RFM69_PORT, RFM69_NSS);
    gpio_mode_setup(RFM69_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RFM69_NSS);
    gpio_set(RFM69_PORT, RFM69_NSS);

    /* RFM69 interrupt setup */
    gpio_mode_setup(RFM69_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, RFM69_INT);

    /* USART GPIO setup */
    gpio_mode_setup(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_TX | USART_RX);
    gpio_set_af(USART_PORT, GPIO_AF4, USART_TX | USART_RX);
}

static void setup() {
    clock_setup();
    gpio_setup();
    sys_tick_setup();
    spi_setup();
    rtc_setup();
    usart_setup(USART_SPEED);
    rfm69_setup();
}

static void deep_sleep() {
    before_sleep();
    enter_stop_mode();
    after_wakeup();
}

static void before_sleep() {
    printf("Going to sleep!\n");
    sys_tick_disable();
    usart_interrupt_disable();
    rtc_wakeup_setup(SLEEP_TIME);
    rfm69_sleep();
    spi_lib_disable();
}

static void after_wakeup() {
    rtc_disable();
    sys_tick_enable();
    usart_interrupt_enable();
    spi_lib_enable();
    rfm69_wakeup();
    printf("Back from sleep!\n");
}

int main(void) {
    setup();
    rfm69_read_all_regs();
    while (1) {
        delay(2000);
        deep_sleep();
    }
}
