/*
 * stm32f407_regs.h — Register map for the STM32F407.
 *
 * Includes SysTick, which is special: it lives in the ARM-defined
 * System Control Space at 0xE000E000, *not* in the STM32 peripheral
 * region at 0x40000000. Every Cortex-M chip has SysTick at exactly
 * this address, so this struct works on any ARM board.
 */
#ifndef STM32F407_REGS_H
#define STM32F407_REGS_H

#include <stdint.h>

/* ---- STM32 peripheral base addresses ----------------------------- */
#define PERIPH_BASE         0x40000000UL
#define APB1PERIPH_BASE     (PERIPH_BASE + 0x00000000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800UL)
#define GPIOD_BASE          (AHB1PERIPH_BASE + 0x0C00UL)
#define USART2_BASE         (APB1PERIPH_BASE + 0x4400UL)

/* ---- ARM core peripheral space (System Control Space) ------------
 * Every Cortex-M has these at fixed addresses, independent of vendor. */
#define SCS_BASE            0xE000E000UL
#define SYSTICK_BASE        (SCS_BASE + 0x0010UL)   /* 0xE000E010 */

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
    volatile uint32_t CR3;
    volatile uint32_t GTPR;
} USART_TypeDef;

/*
 * SysTick — a 24-bit down-counter clocked by HCLK (or HCLK/8).
 *
 *   CSR   — control and status
 *   RVR   — reload value: the counter loads this on wrap
 *   CVR   — current value: the live counter (write any value to clear)
 *   CALIB — factory calibration for a 10 ms tick (rarely used)
 *
 * The counter decrements every tick, wraps from 0 to RVR, and if
 * TICKINT is set, fires the SysTick exception on every wrap. Period
 * is (RVR + 1) ticks — the same "value minus one" convention as the
 * PSC/ARR registers in the PWM project.
 */
typedef struct {
    volatile uint32_t CSR;     /* 0x00 control and status */
    volatile uint32_t RVR;     /* 0x04 reload value (24-bit) */
    volatile uint32_t CVR;     /* 0x08 current value (write to clear) */
    volatile uint32_t CALIB;   /* 0x0C calibration */
} SysTick_TypeDef;

typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t          RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t          RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t          RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

#define RCC      ((RCC_TypeDef *)RCC_BASE)
#define GPIOD    ((GPIO_TypeDef *)GPIOD_BASE)
#define USART2   ((USART_TypeDef *)USART2_BASE)
#define SYSTICK  ((SysTick_TypeDef *)SYSTICK_BASE)

/* RCC enables */
#define RCC_AHB1ENR_GPIODEN   (1U << 3)
#define RCC_APB1ENR_USART2EN  (1U << 17)

/* SysTick CSR bits */
#define SYSTICK_CSR_ENABLE    (1U << 0)   /* counter runs */
#define SYSTICK_CSR_TICKINT   (1U << 1)   /* fire the exception on wrap */
#define SYSTICK_CSR_CLKSOURCE (1U << 2)   /* 1 = processor clock, 0 = clk/8 */

/* USART */
#define USART_SR_TXE          (1U << 7)
#define USART_CR1_UE          (1U << 13)
#define USART_CR1_TE          (1U << 3)

#endif /* STM32F407_REGS_H */
