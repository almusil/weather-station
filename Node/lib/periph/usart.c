#include "../include/periph/usart.h"


static int print(char *str, int len);

static void print_char(char data);

int _write(int file, char *ptr, int len);

void usart_setup(uint32_t baud) {
    usart_set_baudrate(USART_CONSOLE, baud);
    usart_set_databits(USART_CONSOLE, 8);
    usart_set_stopbits(USART_CONSOLE, USART_STOPBITS_1);
    usart_set_mode(USART_CONSOLE, USART_MODE_TX_RX);
    usart_set_parity(USART_CONSOLE, USART_PARITY_NONE);
    usart_set_flow_control(USART_CONSOLE, USART_FLOWCONTROL_NONE);

    usart_interrupt_enable();

    usart_enable(USART_CONSOLE);
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);
    setbuf(stdin, NULL);
}

void usart_interrupt_enable() {
    // Enable interrupt in controller
    nvic_enable_irq(USART_NVIC_IRQ);

    usart_enable_rx_interrupt(USART_CONSOLE);

    // Enable external interrupt 26 (USART)
    exti_set_trigger(USART_EXTI_LINE, EXTI_TRIGGER_RISING);
    exti_enable_request(USART_EXTI_LINE);
}

void usart_interrupt_disable() {
    usart_disable_rx_interrupt(USART_CONSOLE);
    nvic_disable_irq(USART_NVIC_IRQ);
    exti_disable_request(USART_EXTI_LINE);
}

void usart2_isr() {
    exti_reset_request(USART_EXTI_LINE);
    uint16_t temp = usart_recv_blocking(USART_CONSOLE);
    print_char(temp);
}

static void print_char(char data) {
    if (data == '\n') {
        usart_send_blocking(USART_CONSOLE, '\r');
    }
    usart_send_blocking(USART_CONSOLE, data);
    while (!(USART_ISR(USART_CONSOLE) & USART_ISR_TC));
}

void println(char *str) {
    print(str, strlen(str));
    print_char('\n');
}

static int print(char *str, int len) {
    int i;
    for (i = 0; i < len; i++) {
        print_char(str[i]);
    }
    return i;
}

int _write(int file, char *ptr, int len) {
    int i;
    if (file == STDOUT_FILENO || file == STDERR_FILENO) {
        return print(ptr, len);
    }
    errno = EIO;
    return -1;
}
