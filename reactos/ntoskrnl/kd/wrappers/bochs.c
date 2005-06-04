/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/wrappers/bochs.c
 * PURPOSE:         BOCHS Wrapper for Kd
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* bochs debug output */
#define BOCHS_LOGGER_PORT (0xe9)

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
KdpBochsDebugPrint(IN PCH Message)
{
    while (*Message != 0)
    {
        if (*Message == '\n')
        {
#if defined(_M_IX86) && defined(__GNUC__) 
           /* Don't use WRITE_PORT_UCHAR because hal isn't initialized yet in the very early boot phase. */
           __asm__("outb %0, %w1\n\t" :: "a" ('\r'), "d" (BOCHS_LOGGER_PORT));
#else
           WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, '\r');
#endif
        }
#if defined(_M_IX86) && defined(__GNUC__) 
        /* Don't use WRITE_PORT_UCHAR because hal isn't initialized yet in the very early boot phase. */
        __asm__("outb %0, %w1\n\t" :: "a" (*Message), "d" (BOCHS_LOGGER_PORT));
#else
        WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, *Message);
#endif
        Message++;
    }
}



VOID
STDCALL
KdpBochsInit(PKD_DISPATCH_TABLE WrapperTable,
             ULONG BootPhase)
{
    if (!KdpDebugMode.Bochs) return;

    if (BootPhase == 0)
    {
        /* Write out the functions that we support for now */
        WrapperTable->KdpInitRoutine = KdpBochsInit;
        WrapperTable->KdpPrintRoutine = KdpBochsDebugPrint;
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\n   Bochs debugging enabled\n\n");
    }
}

/* EOF */
