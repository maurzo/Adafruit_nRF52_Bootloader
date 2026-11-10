// Bounded nRF52840 RF bring-up test. No GPIO or nonvolatile configuration writes.
#include <stdint.h>
#include "nrf.h"

#define START_DELAY_US 5000000UL
#define TRANSMIT_US   10000000UL
#define OFF_TIME_US   5000000UL
#define BURST_COUNT   5UL
#define FREQUENCY_MHZ 2440UL
#define UF2_RESET_MAGIC 0x57UL

extern uint32_t __StackTop;
extern uint32_t __data_load__, __data_start__, __data_end__;
extern uint32_t __bss_start__, __bss_end__;

static void return_to_bootloader(void) __attribute__((noreturn));
void Reset_Handler(void) __attribute__((noreturn));
static void Default_Handler(void) __attribute__((noreturn));

typedef void (*handler_t)(void);
__attribute__((used, section(".isr_vector"), aligned(256)))
const handler_t vectors[64] = {
    [0] = (handler_t)&__StackTop,
    [1] = Reset_Handler,
    [2 ... 63] = Default_Handler,
};

// Supply a valid RAM packet pointer even though only the idle carrier is measured.
static uint8_t empty_packet[4] __attribute__((aligned(4)));

static void return_to_bootloader(void)
{
    __disable_irq();
    NRF_PPI->CHENCLR = 1UL;
    NRF_RADIO->SHORTS = 0;
    NRF_RADIO->TASKS_DISABLE = 1;
    NRF_RADIO->POWER = 0; // Turn the radio off even on an error path.
    NRF_POWER->GPREGRET = UF2_RESET_MAGIC;
    __DSB();
    NVIC_SystemReset();
    while (1) {}
}

static void Default_Handler(void)
{
    return_to_bootloader();
}

static void timer_prepare(NRF_TIMER_Type *timer, uint32_t microseconds)
{
    timer->TASKS_STOP = 1;
    timer->MODE = TIMER_MODE_MODE_Timer;
    timer->BITMODE = TIMER_BITMODE_BITMODE_32Bit;
    timer->PRESCALER = 4; // 16 MHz / 16 = 1 MHz.
    timer->INTENCLR = 0xFFFFFFFFUL;
    timer->SHORTS = TIMER_SHORTS_COMPARE0_STOP_Msk;
    timer->CC[0] = microseconds;
    timer->EVENTS_COMPARE[0] = 0;
    timer->TASKS_CLEAR = 1;
}

static void run_test(void)
{
    __disable_irq();
    SCB->VTOR = (uint32_t)vectors;
    __DSB();
    __ISB();

    // A watchdog reset must not restart transmission in a loop.
    uint32_t reason = NRF_POWER->RESETREAS;
    NRF_POWER->RESETREAS = reason;
    if (reason & POWER_RESETREAS_DOG_Msk) {
        return_to_bootloader();
    }

    // Independent watchdog catches stalled clock startup or an unexpected hang.
    NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_RC;
    NRF_CLOCK->TASKS_LFCLKSTART = 1;
    NRF_WDT->CRV = 20UL * 32768UL;
    NRF_WDT->RREN = WDT_RREN_RR0_Enabled;
    NRF_WDT->CONFIG = (WDT_CONFIG_SLEEP_Run << WDT_CONFIG_SLEEP_Pos) |
                      (WDT_CONFIG_HALT_Run << WDT_CONFIG_HALT_Pos);
    NRF_WDT->TASKS_START = 1;

    // The radio requires the accurate high-frequency crystal oscillator.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART = 1;
    while ((NRF_CLOCK->HFCLKSTAT & (CLOCK_HFCLKSTAT_STATE_Msk | CLOCK_HFCLKSTAT_SRC_Msk)) !=
           ((CLOCK_HFCLKSTAT_STATE_Running << CLOCK_HFCLKSTAT_STATE_Pos) |
            (CLOCK_HFCLKSTAT_SRC_Xtal << CLOCK_HFCLKSTAT_SRC_Pos))) {}

    timer_prepare(NRF_TIMER1, START_DELAY_US);
    NRF_TIMER1->TASKS_START = 1;
    while (!NRF_TIMER1->EVENTS_COMPARE[0]) {}

    NRF_RADIO->POWER = 1;
    NRF_RADIO->INTENCLR = 0xFFFFFFFFUL;
    NRF_RADIO->MODE = RADIO_MODE_MODE_Ble_1Mbit;
    NRF_RADIO->FREQUENCY = FREQUENCY_MHZ - 2400UL;
    NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg20dBm;
    NRF_RADIO->MODECNF0 = RADIO_MODECNF0_DTX_Center << RADIO_MODECNF0_DTX_Pos;
    NRF_RADIO->PCNF0 = 0;
    NRF_RADIO->PCNF1 = 0;
    NRF_RADIO->CRCCNF = RADIO_CRCCNF_LEN_Disabled;
    NRF_RADIO->BASE0 = 0;
    NRF_RADIO->PREFIX0 = 0;
    NRF_RADIO->TXADDRESS = 0;
    NRF_RADIO->PACKETPTR = (uint32_t)empty_packet;

    // Match Nordic's radio_test unmodulated-carrier sequence. After the empty
    // packet, DTX=Center maintains an unmodulated center-frequency carrier.
    NRF_RADIO->SHORTS = RADIO_SHORTS_READY_START_Msk;

    // Hardware cutoff: PPI disables RADIO after 10 s, even if the CPU stalls.
    NRF_PPI->CHENCLR = 1UL;
    NRF_PPI->CH[0].EEP = (uint32_t)&NRF_TIMER2->EVENTS_COMPARE[0];
    NRF_PPI->CH[0].TEP = (uint32_t)&NRF_RADIO->TASKS_DISABLE;
    NRF_PPI->FORK[0].TEP = 0;
    NRF_PPI->CHENSET = 1UL;

    for (uint32_t burst = 0; burst < BURST_COUNT; ++burst) {
        NRF_WDT->RR[0] = WDT_RR_RR_Reload;
        NRF_RADIO->EVENTS_READY = 0;
        NRF_RADIO->EVENTS_DISABLED = 0;
        timer_prepare(NRF_TIMER2, TRANSMIT_US);
        NRF_TIMER2->TASKS_START = 1;
        NRF_RADIO->TASKS_TXEN = 1;

        while (!NRF_TIMER2->EVENTS_COMPARE[0]) {}
        while (!NRF_RADIO->EVENTS_DISABLED) {}

        if (burst + 1UL < BURST_COUNT) {
            timer_prepare(NRF_TIMER1, OFF_TIME_US);
            NRF_TIMER1->TASKS_START = 1;
            while (!NRF_TIMER1->EVENTS_COMPARE[0]) {}
        }
    }
    return_to_bootloader();
}

void Reset_Handler(void)
{
    uint32_t *source = &__data_load__;
    for (uint32_t *destination = &__data_start__; destination < &__data_end__;) {
        *destination++ = *source++;
    }
    for (uint32_t *destination = &__bss_start__; destination < &__bss_end__;) {
        *destination++ = 0;
    }
    run_test();
    return_to_bootloader();
}
