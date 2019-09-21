#include "../include/periph/sys_tick.h"

static volatile uint64_t millis;

void sys_tick_setup(void) {
    millis = 0;

    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_clear();

    systick_set_reload(rcc_ahb_frequency / 1000 - 1);

    systick_interrupt_enable();
    systick_counter_enable();
}

void delay(uint32_t ms) {
    const uint64_t until = get_millis() + ms;
    while (get_millis() < until);
}

uint64_t get_millis() {
    return millis;
}

void sys_tick_disable() {
    systick_interrupt_disable();
    systick_counter_disable();
}

void sys_tick_enable() {
    systick_interrupt_enable();
    systick_counter_enable();
}

void sys_tick_handler(void) {
    millis++;
}
