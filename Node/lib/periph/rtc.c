#include "../include/periph/rtc.h"

static void rtc_interrupt_setup(void);

void rtc_setup() {
    // Disable rtc register write protection
    pwr_disable_backup_domain_write_protect();

    // Reset status of all RTC registers
    RCC_CSR |= RCC_CSR_RTCRST;
    RCC_CSR &= ~RCC_CSR_RTCRST;

    // Enable internal Low speed oscillator
    rcc_osc_on(RCC_LSI);
    rcc_wait_for_osc_ready(RCC_LSI);

    // Set LSI as RTC clock source
    RCC_CSR &= ~(RCC_CSR_RTCSEL_MASK << RCC_CSR_RTCSEL_SHIFT);
    RCC_CSR |= (RCC_CSR_RTCSEL_LSI << RCC_CSR_RTCSEL_SHIFT);

    // Enable RTC clock
    RCC_CSR |= RCC_CSR_RTCEN;
}

void rtc_wakeup_setup(uint16_t seconds) {
    rtc_unlock();
    rtc_set_wakeup_time(seconds - 1, RTC_CR_WUCLKSEL_SPRE);
    rtc_interrupt_setup();
    rtc_lock();
}

void rtc_disable() {
    rtc_unlock();
    RTC_CR |= RTC_CR_WUTIE;
    RTC_CR &= ~RTC_CR_WUTE;
    nvic_disable_irq(NVIC_RTC_IRQ);
    exti_disable_request(EXTI20);
}

static void rtc_interrupt_setup() {
    // Enable interrupt in controller
    nvic_enable_irq(NVIC_RTC_IRQ);

    RTC_CR |= RTC_CR_WUTIE;

    // Enable external interrupt 20 (RTC wakeup)
    exti_set_trigger(EXTI20, EXTI_TRIGGER_RISING);
    exti_enable_request(EXTI20);
}

void rtc_isr() {
    rtc_clear_wakeup_flag();
    exti_reset_request(EXTI20);
}
