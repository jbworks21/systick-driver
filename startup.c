/*
 * startup.c — Vector table and Reset_Handler for STM32F407.
 *
 * THE VECTOR TABLE
 *
 * The ARM Cortex-M4 vector table is a fixed-layout array of function
 * pointers. On reset, the CPU reads:
 *   - word 0 (offset 0x00): the initial stack pointer
 *   - word 1 (offset 0x04): the address of Reset_Handler
 * and jumps there. Every subsequent exception or interrupt reads its
 * own slot in the same table:
 *
 *   offset  index  name
 *   0x00    0      initial SP         (not a function pointer!)
 *   0x04    1      Reset
 *   0x08    2      NMI
 *   0x0C    3      HardFault
 *   0x10    4      MemManage
 *   0x14    5      BusFault
 *   0x18    6      UsageFault
 *   0x1C-0x28  7-10  reserved
 *   0x2C    11     SVCall
 *   0x30    12     DebugMon
 *   0x34    13     reserved
 *   0x38    14     PendSV
 *   0x3C    15     SysTick         ← the one this project uses
 *   0x40    16     IRQ0            ← peripheral interrupts start here
 *   ...
 *
 * Entries 0-15 are "system exceptions" — always present, always at
 * these offsets. Entries 16+ are vendor IRQs (USART2, TIM4, etc.),
 * whose number depends on the specific chip. This project enables no
 * peripheral IRQs, so 16 entries are enough.
 *
 * WEAK ALIASES
 *
 * Every handler except Reset is declared as a "weak alias" to
 * Default_Handler. The pattern means:
 *   - The symbol exists and points at Default_Handler by default
 *   - If any other translation unit defines a strong version
 *     (e.g. systick.c's SysTick_Handler), the linker uses that
 *     instead, and the vector table automatically picks it up.
 *
 * This is exactly how CMSIS works, and how every STM32 project —
 * from the smallest blinky to FreeRTOS — handles the vector table.
 * You can write ISRs in any .c file without touching startup.c.
 */
#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;
extern int main(void);

void Reset_Handler(void);

/*
 * Default_Handler — anything without a real handler lands here.
 *
 * A production system would log a fault and reset. For a learning
 * project, we loop forever. If you ever see the LED stop responding
 * and the debugger shows the CPU in a tight loop, you're in
 * Default_Handler — some exception fired with no real handler.
 */
void Default_Handler(void) {
    while (1) { }
}

/*
 * Weak aliases for the standard Cortex-M exceptions.
 * Defining a strong version of any of these in another file — e.g.
 *   void SysTick_Handler(void) { ... }
 * in systick.c — overrides the alias. Same for NMI_Handler, etc.
 */
void NMI_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void MemManage_Handler(void)  __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void)        __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void)   __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void)     __attribute__((weak, alias("Default_Handler")));
void SysTick_Handler(void)    __attribute__((weak, alias("Default_Handler")));

/*
 * The vector table itself. __attribute__((section(".isr_vector")))
 * places this array in the section the linker script reserves for it,
 * which is mapped to the very start of Flash (0x08000000).
 *
 * If the section is missing from the array, the CPU will read
 * whatever happens to be at that address and jump into space.
 */
__attribute__((section(".isr_vector")))
void (*const g_pfnVectors[])(void) = {
    /* 0 */ (void (*)(void))(&_estack),
    /* 1 */ Reset_Handler,
    /* 2 */ NMI_Handler,
    /* 3 */ HardFault_Handler,
    /* 4 */ MemManage_Handler,
    /* 5 */ BusFault_Handler,
    /* 6 */ UsageFault_Handler,
    /* 7 */ 0, 0, 0, 0,
    /* 11 */ SVC_Handler,
    /* 12 */ DebugMon_Handler,
    /* 13 */ 0,
    /* 14 */ PendSV_Handler,
    /* 15 */ SysTick_Handler,
};

/*
 * Reset_Handler — called by the CPU on reset.
 *
 *  1. Copy .data from Flash into RAM (initialized globals)
 *  2. Zero .bss (uninitialized globals)
 *  3. Call main()
 *  4. If main returns, loop forever
 *
 * Steps 1 and 2 use symbols from the linker script. The linker places
 * their addresses at the boundary of each section — see linker.ld.
 */
void Reset_Handler(void) {
    uint32_t *pSrc  = &_sidata;
    uint32_t *pDest = &_sdata;
    while (pDest < &_edata) {
        *pDest++ = *pSrc++;
    }

    pDest = &_sbss;
    while (pDest < &_ebss) {
        *pDest++ = 0;
    }

    main();
    while (1) { }
}
