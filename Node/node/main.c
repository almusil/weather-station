#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/pwr.h>

#include <periph/sys_tick.h>
#include <periph/rtc.h>
#include <periph/low_power.h>
#include <periph/spi.h>
#include <periph/usart.h>
#include <periph/adc.h>
#include <periph/gpio.h>
#include <periph/i2c.h>
#include <config.h>
#include <rfm69.h>
#include <bme280.h>

static struct config conf = {
        .sleep_time = 10,
        .dio_direction = 0x00,
        .dio_value = 0x00,
        .analog = BAT_CHANNEL
};

static void clock_setup(void);

static void gpio_setup(void);

static void setup(void);

static void deep_sleep(void);

static void before_sleep(void);

static void after_wakeup(void);

static bool packet_received(void);

static void update_config(void);

static void send_measured_data(void);

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
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(SPI_CLOCK);
    rcc_periph_clock_enable(RCC_PWR);
    rcc_periph_clock_enable(USART_CLOCK);
    rcc_periph_clock_enable(ADC_CLOCK);
    rcc_periph_clock_enable(I2C_CLOCK);
}


static void gpio_setup() {
    /*  SPI GPIO setup */
    gpio_mode_setup(SPI_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, SPI_SCK | SPI_MISO | SPI_MOSI);
    gpio_set_output_options(SPI_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_VERYHIGH, SPI_SCK | SPI_MISO | SPI_MOSI);
    gpio_set_af(SPI_PORT, GPIO_AF0, SPI_SCK | SPI_MISO | SPI_MOSI);

    gpio_clear(RFM69_PORT, RFM69_NSS);
    gpio_mode_setup(RFM69_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, RFM69_NSS);
    gpio_set(RFM69_PORT, RFM69_NSS);

    /* I2C GPIO setup */
    gpio_mode_setup(I2C_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, I2C_SDA | I2C_SCL);
    /* I2C pins have to open-drain */
    gpio_set_output_options(I2C_PORT, GPIO_OTYPE_OD, GPIO_OSPEED_LOW, I2C_SDA | I2C_SCL);
    gpio_set_af(I2C_PORT, GPIO_AF1, I2C_SDA | I2C_SCL);

    /* RFM69 interrupt setup */
    gpio_mode_setup(RFM69_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, RFM69_INT);

    /* USART GPIO setup */
    gpio_mode_setup(USART_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, USART_TX | USART_RX);
    gpio_set_af(USART_PORT, GPIO_AF4, USART_TX | USART_RX);

    /* ADC GPIO setup */
    gpio_mode_setup(ADC_PORT1, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, A0 | A1);
    gpio_mode_setup(ADC_PORT2, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, BAT_ADC | A2);

    /* Battery switch */
    gpio_mode_setup(SW_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, SW_PIN);
    gpio_clear(SW_PORT, SW_PIN);
}

static void setup() {
    clock_setup();
    gpio_setup();
    sys_tick_setup();
    spi_setup();
    rtc_setup();
    usart_setup(USART_SPEED);
    rfm69_setup();
    adc_setup();
    i2c_setup();
    bme_setup();
    gpio_dio_setup(conf.dio_direction, conf.dio_value);
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
    rtc_wakeup_setup(conf.sleep_time);
    rfm69_sleep();
    spi_lib_disable();
    i2c_disable();
}

static void after_wakeup() {
    rtc_disable();
    sys_tick_enable();
    usart_interrupt_enable();
    spi_lib_enable();
    rfm69_wakeup();
    i2c_enable();
    printf("Back from sleep!\n");
}

static bool packet_received() {
    uint64_t now = get_millis();
    bool done = rfm69_receive_done();
    while (!done && get_millis() - now < CONFIG_TIMEOUT) {
        done = rfm69_receive_done();
    }
    printf("Received %d!\n", done);
    return done;
}

static void update_config() {
    struct rfm69_packet packet = {0};
    rfm69_get_data(&packet);
    if (packet.sender_id == GATEWAY_ADDR &&
        packet.target_id == NODE_ADDR &&
        packet.data_len == CONFIG_LENGTH + 1 &&
        packet.data_buffer[0] == PACKET_CONFIG) {
        // Update conf
        conf.sleep_time = (packet.data_buffer[1] << 8) | packet.data_buffer[2];
        conf.dio_direction = packet.data_buffer[3];
        conf.dio_value = packet.data_buffer[4];
        conf.analog = (packet.data_buffer[5] & 0x07) | BAT_CHANNEL;

        // Call gpio_setup
        gpio_dio_setup(conf.dio_direction, conf.dio_value);
        printf("Got new config!\n");
    }
}

static void send_measured_data() {
    printf("Sending measured data!\n");
    uint8_t data[17] = {0};
    uint16_t adc[4] = {0};

    bme_start_measurement();
    gpio_set(SW_PORT, SW_PIN);
    adc_convert(conf.analog, adc, 4);
    gpio_clear(SW_PORT, SW_PIN);

    if (conf.analog & A2_CHANNEL) {
        // Swap A2 and BAT adc data
        uint16_t tmp = adc[2];
        adc[2] = adc[3];
        adc[3] = tmp;
    }

    data[0] = PACKET_DATA;
    data[1] = read_gpio_value();

    for (uint8_t i = 0; i < 4; i++) {
        data[2 + i * 2] = (adc[i] >> 8);
        data[3 + i * 2] = (adc[i] & 0xff);
    }

    while (!bme_measure_done());

    int16_t temp = bme_get_temp();
    data[10] = temp >> 8;
    data[11] = temp & 0xff;

    uint32_t press = bme_get_pressure();
    data[12] = press >> 16;
    data[13] = press >> 8;
    data[14] = press & 0xff;

    uint16_t humidity = bme_get_humidity();
    data[15] = humidity >> 8;
    data[16] = humidity & 0xff;

    for (uint8_t i = 0; i < ACK_RETRY_COUNT; i++) {
        rfm69_send(GATEWAY_ADDR, &data, 17, true);
        uint64_t now = get_millis();
        bool done = rfm69_ack_received(GATEWAY_ADDR);
        while (!done && get_millis() - now < ACK_TIMEOUT) {
            done = rfm69_ack_received(GATEWAY_ADDR);
        }
        if (done) {
            printf("Got ACK!\n");
            break;
        }
    }
}

int main(void) {
    setup();
    delay(1000);
    while (1) {
        uint8_t buffer = {PACKET_CONFIG};
        rfm69_send(GATEWAY_ADDR, &buffer, 1, false);
        bool new_packet = packet_received();
        if (new_packet) {
            update_config();
        }
        send_measured_data();
        deep_sleep();
    }
}
