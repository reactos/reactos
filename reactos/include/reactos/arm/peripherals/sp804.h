/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            include/reactos/arm/targets/pl011.h
 * PURPOSE:         SB804 Registers and Constants
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

/* GLOBALS ********************************************************************/

//
// Timer Registers
//
#define TIMER_BASE(x)            (PVOID)(0xE00E2000 + (x * 0x1000))  /* HACK: freeldr mapped it here */
#define TIMER0_LOAD              TIMER_BASE(0) + 0x00
#define TIMER0_VALUE             TIMER_BASE(0) + 0x04
#define TIMER0_CONTROL           TIMER_BASE(0) + 0x08
#define TIMER0_INT_CLEAR         TIMER_BASE(0) + 0x0C
#define TIMER0_INT_STATUS        TIMER_BASE(0) + 0x10
#define TIMER0_INT_MASK          TIMER_BASE(0) + 0x14
#define TIMER0_BACKGROUND_LOAD   TIMER_BASE(1) + 0x18
#define TIMER1_LOAD              TIMER_BASE(1) + 0x00
#define TIMER1_VALUE             TIMER_BASE(1) + 0x04
#define TIMER1_CONTROL           TIMER_BASE(1) + 0x08
#define TIMER1_INT_CLEAR         TIMER_BASE(1) + 0x0C
#define TIMER1_INT_STATUS        TIMER_BASE(1) + 0x10
#define TIMER1_INT_MASK          TIMER_BASE(1) + 0x14
#define TIMER1_BACKGROUND_LOAD   TIMER_BASE(1) + 0x18

//
// Control Register
//
typedef union _SP804_CONTROL_REGISTER
{
    struct
    {
        ULONG OneShot:1;
        ULONG Wide:1;
        ULONG Prescale:2;
        ULONG Reserved:1;
        ULONG Interrupt:1;
        ULONG Periodic:1;
        ULONG Enabled:1;
        ULONG Unused:24;
    };
    ULONG AsUlong;
} SP804_CONTROL_REGISTER, *PSP804_CONTROL_REGISTER;
