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
KdpBochsDebugPrint(IN PCH Message,
                   IN ULONG Length)
{
    if (!KdpDebugMode.Bochs) return;

    while (*Message != 0)
    {
        if (*Message == '\n')
        {
#if defined(_M_IX86) && defined(__GNUC__) 
           /* Don't use WRITE_PORT_UCHAR because hal isn't initialized yet in the very early boot phase. */
           __asm__("outb %b0, %w1\n\t" :: "a" ('\r'), "d" (BOCHS_LOGGER_PORT));
#else
           WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, '\r');
#endif
        }
#if defined(_M_IX86) && defined(__GNUC__) 
        /* Don't use WRITE_PORT_UCHAR because hal isn't initialized yet in the very early boot phase. */
        __asm__("outb %b0, %w1\n\t" :: "a" (*Message), "d" (BOCHS_LOGGER_PORT));
#else
        WRITE_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT, *Message);
#endif
        Message++;
    }
}

VOID
STDCALL
KdpBochsInit(PKD_DISPATCH_TABLE DispatchTable,
             ULONG BootPhase)
{
    UCHAR Value;
    if (!KdpDebugMode.Bochs) return;

    if (BootPhase == 0)
    {
#if defined(_M_IX86) && defined(__GNUC__) 
        __asm__("inb %w1, %b0\n\t" : "=a" (Value) : "d" (BOCHS_LOGGER_PORT));
#else
        Value = READ_PORT_UCHAR((PUCHAR)BOCHS_LOGGER_PORT);
#endif
        if (Value != BOCHS_LOGGER_PORT)
        {
           KdpDebugMode.Bochs = FALSE;
           return;
        }

        /* Write out the functions that we support for now */
        DispatchTable->KdpInitRoutine = KdpBochsInit;
        DispatchTable->KdpPrintRoutine = KdpBochsDebugPrint;

        /* Register as a Provider */
        InsertTailList(&KdProviders, &DispatchTable->KdProvidersList);
    }
    else if (BootPhase == 2)
    {
        HalDisplayString("\n   Bochs debugging enabled\n\n");
    }
}

/* EOF */
