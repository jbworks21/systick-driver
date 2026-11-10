/*
 * systick.c — SysTick driver for STM32F407 (1 ms tick by default).
 *
 * The three-step configuration is the same as any ARM Cortex-M:
 *   1. Program RVR (the reload value, i.e. the period minus one)
 *   2. Clear CVR (loads RVR into the counter immediately)
 *   3. Set CSR = ENABLE | TICKINT | CLKSOURCE
 *
 * Note that the SysTick exception is enabled purely by CSR.TICKINT —
 * it does NOT go through the NVIC's ISER registers. This is a common
 * source of confusion when moving to peripheral interrupts (which DO
 * need NVIC enable). SysTick is special because it's part of the core.
 */
#include "stm32f407_regs.h"
#include "systick.h"

/* HCLK at reset. If you enable the PLL, update this. */
#define SYSTICK_HCLK_HZ   16000000UL

/* The shared tick counter. `volatile` is mandatory:
 *
 *   - The ISR increments it, main reads it.
 *   - Without `volatile`, the compiler may cache the value in a
 *     register and never reload it from memory in the main loop —
 *     so main sees only the initial value, forever.
 *   - This is THE classic embedded bug. If your counter "never
 *     changes" in the main loop but the LED blinks correctly (because
 *     the ISR writes it), `volatile` was forgotten.
 *
 * On a 32-bit processor, reads and writes of uint32_t are atomic —
 * no tearing, no need for critical sections just to read it. For
 * 64-bit values or compound reads, you'd need to disable interrupts
 * around the read. */
static volatile uint32_t s_ticks = 0;
static volatile uint32_t s_ms = 0;
static uint32_t s_ticks_per_ms = 1;   /* set at init if freq is multiple of 1000 */

int systick_init(uint32_t freq_hz) {
    if (freq_hz == 0) return -1;

    /* Period in HCLK counts. SysTick is a 24-bit counter, so the
     * reload value must fit in 24 bits. Anything larger and we can't
     * express the period — return an error instead of silently
     * truncating, which would give the wrong frequency. */
    uint32_t ticks = SYSTICK_HCLK_HZ / freq_hz;
    if (ticks == 0 || ticks > 0x01000000UL) return -1;

    /* Configure the reload register: value = period - 1, same "minus
     * one" convention as PSC and ARR in the PWM project. The counter
     * counts from RVR down to 0, then wraps — RVR + 1 counts total. */
    SYSTICK->RVR = ticks - 1;

    /* Clear the current value. Writing any value here forces a load
     * from RVR at the next tick, so the first period is full-length
     * (not a partial period from wherever the counter happened to be). */
    SYSTICK->CVR = 0;

    /* Remember the relationship between ticks and ms, if freq is a
     * multiple of 1000 (e.g. 1000 Hz = 1 tick/ms, 8000 Hz = 8 ticks/ms). */
    s_ticks_per_ms = (freq_hz >= 1000) ? (freq_hz / 1000) : 1;
    s_ticks = 0;
    s_ms = 0;

    /* Enable: counter runs, exception fires on wrap, use processor
     * clock (HCLK) rather than HCLK/8. */
    SYSTICK->CSR = SYSTICK_CSR_ENABLE | SYSTICK_CSR_TICKINT | SYSTICK_CSR_CLKSOURCE;
    return 0;
}

void systick_start(void) {
    SYSTICK->CSR |= SYSTICK_CSR_ENABLE;
}

void systick_stop(void) {
    SYSTICK->CSR &= ~SYSTICK_CSR_ENABLE;
}

uint32_t systick_ticks(void) {
    /* One atomic 32-bit load. No interrupt disable needed on Cortex-M4. */
    return s_ticks;
}

uint32_t systick_ms(void) {
    /* Reading s_ticks and dividing gives the same answer as keeping a
     * separate ms counter, but is more robust — no risk of the two
     * getting out of sync if one increment is missed. */
    if (s_ticks_per_ms == 0) return 0;
    return s_ticks / s_ticks_per_ms;
}

/*
 * SysTick_Handler — the ISR that SysTick fires on every wrap.
 *
 * Where this function is declared is important:
 *   - startup.c declares it as a weak alias to Default_Handler.
 *   - The vector table's entry 15 points at SysTick_Handler.
 *   - This strong definition here overrides the weak alias at link
 *     time, so the vector actually points at this function.
 *
 * The ISR should be short. Long ISRs block everything else in the
 * system, including other interrupts. Here we only increment two
 * counters — the minimum viable work. In production code, this is
 * where you'd also toggle a "software timer expired" flag.
 *
 * __attribute__((interrupt)) is not needed on ARM: the hardware
 * saves and restores state automatically. On ARM, ISRs are ordinary
 * C functions that happen to be called from the vector table.
 */
void SysTick_Handler(void) {
    s_ticks++;
    /* Advance the ms counter only every N ticks (for freq > 1000 Hz). */
    if ((s_ticks % s_ticks_per_ms) == 0) {
        s_ms++;
    }
}
