/*
 * PROJECT:     ReactOS Hardware Abstraction Layer
 * LICENSE:     GPL-2.0-or-later
 * PURPOSE:     AMD64-specific HAL initialization
 * COPYRIGHT:   Copyright 2024 ReactOS Team
 */

/* INCLUDES *****************************************************************/

#include <hal.h>
#include <debug.h>

#ifdef _M_AMD64

/* GLOBALS ******************************************************************/

extern VOID FASTCALL HalpClockInterrupt(VOID);
extern VOID FASTCALL HalpProfileInterrupt(VOID);

/* FUNCTIONS ****************************************************************/

VOID
HalpInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Debug output */
    #define COM1_PORT 0x3F8
    {
        const char msg[] = "*** HAL: HalpInitPhase0 for AMD64 ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Nothing special needed in Phase 0 for AMD64 */
}

VOID
HalpInitPhase1(VOID)
{
    /* Debug output */
    {
        const char msg[] = "*** HAL: HalpInitPhase1 for AMD64 - Setting up timer interrupt ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Register the clock interrupt handler at vector 0x30 (IRQ0) */
    /* PRIMARY_VECTOR_BASE is 0x30, PIC_TIMER_IRQ is 0 */
    {
        const char msg[] = "*** HAL: Registering clock interrupt handler at vector 0x30 ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Use simplified registration for AMD64 */
    extern VOID KeRegisterInterruptHandler(IN ULONG Vector, IN PVOID Handler);
    KeRegisterInterruptHandler(0x30, HalpClockInterrupt);
    
    {
        const char msg[] = "*** HAL: Clock interrupt handler registered ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Enable the timer interrupt on the PIC */
    {
        const char msg[] = "*** HAL: Enabling timer interrupt on PIC ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
    
    /* Unmask IRQ0 on the master PIC */
    UCHAR Mask = __inbyte(0x21);  /* Read current mask from master PIC */
    Mask &= ~0x01;  /* Clear bit 0 to unmask IRQ0 */
    __outbyte(0x21, Mask);  /* Write back the mask */
    
    {
        const char msg[] = "*** HAL: Timer interrupt enabled on PIC ***\n";
        const char *p = msg;
        while (*p) { while ((__inbyte(COM1_PORT + 5) & 0x20) == 0); __outbyte(COM1_PORT, *p++); }
    }
}

#endif /* _M_AMD64 */

/* EOF */