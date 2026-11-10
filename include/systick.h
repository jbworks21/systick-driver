/*
 * systick.h — Public API of the SysTick driver.
 *
 * SysTick is the ARM core's built-in countdown timer. This driver uses
 * it in its most common role: a periodic "tick" interrupt, typically at
 * 1 kHz, that applications use as a time base.
 *
 * Why 1 kHz? Because 1 tick = 1 ms, so "how many ticks?" becomes
 * "how many milliseconds?" and timeouts, delays, and debouncing all
 * become plain integer arithmetic. This is the standard time base on
 * almost every embedded project.
 *
 * The ISR runs at the requested frequency forever, in the background.
 * Applications read systick_ms() and compute elapsed time as a
 * subtraction — no blocking, no busy-waiting.
 */
#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

/*
 * Configure SysTick to fire at the requested frequency.
 * Typical value: 1000 (1 kHz, 1 ms tick).
 *
 * Returns 0 on success, -1 if the frequency is unreachable
 * (the 24-bit reload register can't represent that many counts).
 *
 * Assumes HCLK = 16 MHz (the reset default, HSI). If you later enable
 * the PLL, call this *after* the PLL is configured, and update
 * SYSTICK_HCLK_HZ in systick.c.
 */
int systick_init(uint32_t freq_hz);

/* Returns the number of milliseconds since boot. Wraps at 2^32 ms (~49 days). */
uint32_t systick_ms(void);

/* Returns the current tick count (the raw counter, not necessarily ms). */
uint32_t systick_ticks(void);

/* Start the counter without enabling the interrupt. Used internally. */
void systick_start(void);

/* Stop the counter. The tick count stops advancing. */
void systick_stop(void);

#endif /* SYSTICK_H */
