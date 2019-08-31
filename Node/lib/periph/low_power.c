#include "../include/periph/low_power.h"

static inline __attribute__((always_inline)) void __WFI(void) {
    __asm volatile ("wfi");
}

void enter_stop_mode() {
    pwr_clear_wakeup_flag();
    pwr_set_stop_mode();
    PWR_CR |= PWR_CR_ULP | PWR_CR_FWU | PWR_CR_LPSDSR;
    pwr_voltage_regulator_low_power_in_stop();
    RCC_CFGR |= RCC_CFGR_STOPWUCK_HSI16;
    SCB_SCR |= SCB_SCR_SLEEPDEEP;
    __WFI();
}
