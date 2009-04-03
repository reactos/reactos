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
#include <debug.h>

/* bochs debug output */
#define BOCHS_LOGGER_PORT ((PVOID)0xe9)

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KdpBochsDebugPrint(IN PCH Message,
                   IN ULONG Length)
{
    if (!KdpDebugMode.Bochs) return;

    while (*Message != 0)
    {
        if (*Message == '\n')
        {
           WRITE_PORT_UCHAR(BOCHS_LOGGER_PORT, '\r');
        }
        WRITE_PORT_UCHAR(BOCHS_LOGGER_PORT, *Message);
        Message++;
    }
}

VOID
NTAPI
KdpBochsInit(PKD_DISPATCH_TABLE DispatchTable,
             ULONG BootPhase)
{
    UCHAR Value;
    if (!KdpDebugMode.Bochs) return;

    if (BootPhase == 0)
    {
        Value = READ_PORT_UCHAR(BOCHS_LOGGER_PORT);
        if (Value != (ULONG)BOCHS_LOGGER_PORT)
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
