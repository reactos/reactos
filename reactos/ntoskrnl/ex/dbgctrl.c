/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/dbgctrl.c
 * PURPOSE:         System debug control
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtSystemDebugControl(DEBUG_CONTROL_CODE ControlCode,
                     PVOID InputBuffer,
                     ULONG InputBufferLength,
                     PVOID OutputBuffer,
                     ULONG OutputBufferLength,
                     PULONG ReturnLength)
{
    switch (ControlCode)
    {
        case DebugGetTraceInformation:
        case DebugSetInternalBreakpoint:
        case DebugSetSpecialCall:
        case DebugClearSpecialCalls:
        case DebugQuerySpecialCalls:
        case DebugDbgBreakPoint:
            break;

        case DebugDbgLoadSymbols:
            KDB_LOADUSERMODULE_HOOK((PLDR_MODULE) InputBuffer);
            break;

        default:
            break;
    }

    return STATUS_SUCCESS;
}
