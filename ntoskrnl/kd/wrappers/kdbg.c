/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/kd/wrappers/bochs.c
 * PURPOSE:         KDBG Wrapper for Kd
 *
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

VOID NTAPI
KdbInitialize(PKD_DISPATCH_TABLE DispatchTable, ULONG BootPhase);

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
KdpKdbgInit(PKD_DISPATCH_TABLE DispatchTable,
            ULONG BootPhase)
{
#ifdef KDBG
    /* Forward the call */
    KdbInitialize(DispatchTable, BootPhase);
#else
    /* When KDBG is disabled, it is not used/initialized at all */
#endif
}
