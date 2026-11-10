/*
 * app/main.c — Blink + heartbeat demo using SysTick as the time base.
 *
 * THE DEMO
 *
 *   - LED on PD12 toggles every 500 ms (fixed wall-clock rate)
 *   - Serial prints a heartbeat every 2000 ms with the ms counter
 *   - main does "work" in between (a measurable busy loop)
 *
 * The point: the LED keeps toggling at exactly 500 ms even while main
 * is stuck doing heavy work, because the toggling is driven by the ISR
 * + a comparison against systick_ms(). Compare this to the blinky
 * project, where delay(500000) *blocked* the CPU for ~500 ms and did
 * nothing else during that time.
 *
 * This is the fundamental shift from polling to interrupt-driven
 * design: timing accuracy is now provided by hardware, and the CPU is
 * free to spend its cycles on other things.
 */
#include "stm32f407_regs.h"
#include "systick.h"

/* ========== Demo console (same as previous projects) ========== */

static void console_init(void) {
    RCC->AHB1ENR |= (1U << 0);               /* GPIOAEN */
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    volatile uint32_t *gpioa_moder = (volatile uint32_t *)0x40020000UL;
    volatile uint32_t *gpioa_afr0  = (volatile uint32_t *)0x40020020UL;
    *gpioa_moder &= ~((3U << 4) | (3U << 6));
    *gpioa_moder |=  ((2U << 4) | (2U << 6));
    *gpioa_afr0 &= ~((0xFU << 8) | (0xFU << 12));
    *gpioa_afr0 |=  ((0x7U << 8) | (0x7U << 12));

    USART2->BRR = (16000000UL + 115200U / 2) / 115200U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_UE;
}

static void putc_(char c) {
    while (!(USART2->SR & USART_SR_TXE)) { }
    USART2->DR = (uint8_t)c;
}
static void puts_(const char *s) { while (*s) putc_(*s++); }

static void print_u32(uint32_t v) {
    char buf[10];
    int i = 0;
    if (v == 0) { putc_('0'); return; }
    while (v > 0) { buf[i++] = (char)('0' + (v % 10U)); v /= 10U; }
    while (i--) putc_(buf[i]);
}

/* ========== LED setup (bare metal, same as blinky) ========== */

static void led_init(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER &= ~(3U << (12 * 2));
    GPIOD->MODER |=  (1U << (12 * 2));
}

static void led_toggle(void) {
    GPIOD->ODR ^= (1U << 12);
}

/* ========== "Busy work" ==========
 *
 * A loop long enough to be measurable, with a volatile counter so the
 * optimizer can't delete it. This simulates any real application work
 * that would have starved a delay()-based blinky. */
static volatile uint32_t s_work_counter = 0;

static void busy_work(void) {
    for (uint32_t i = 0; i < 200000U; i++) {
        s_work_counter++;
    }
}

/* ============================ Main ============================ */

int main(void) {
    console_init();
    led_init();

    if (systick_init(1000) != 0) {              /* 1 kHz = 1 ms tick */
        puts_("systick_init failed\r\n");
        while (1) { }
    }

    puts_("\r\n=== SysTick demo — 1 ms tick, LED every 500 ms ===\r\n");

    uint32_t next_led_toggle = 500;             /* ms */
    uint32_t next_heartbeat  = 2000;            /* ms */

    while (1) {
        uint32_t now = systick_ms();

        /* Non-blocking timing: "has enough time elapsed?" rather than
         * "sleep for a while." Uses unsigned subtraction so wrap at
         * ~49 days is handled correctly (a known trick worth knowing).
         *
         * The `>=` comparison (with subtraction) is correct even when
         * now has just wrapped past 2^32 — `now - next_led_toggle`
         * wraps consistently because unsigned math is modular. */
        if ((now - next_led_toggle) < 0x80000000U && now >= next_led_toggle) {
            led_toggle();
            next_led_toggle = now + 500;
        }

        if ((now - next_heartbeat) < 0x80000000U && now >= next_heartbeat) {
            puts_("heartbeat at ");
            print_u32(now);
            puts_(" ms, work_counter = ");
            print_u32(s_work_counter);
            puts_("\r\n");
            next_heartbeat = now + 2000;
        }

        /* Occupy the CPU. The LED keeps toggling at exactly 500 ms
         * despite this — that is the whole point. A delay()-based
         * blinky would have its timing destroyed by this loop. */
        busy_work();
    }
}
