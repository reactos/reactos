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
NtSystemDebugControl(SYSDBG_COMMAND ControlCode,
                     PVOID InputBuffer,
                     ULONG InputBufferLength,
                     PVOID OutputBuffer,
                     ULONG OutputBufferLength,
                     PULONG ReturnLength)
{
    switch (ControlCode)
    {
        case SysDbgQueryTraceInformation:
        case SysDbgSetTracepoint:
        case SysDbgSetSpecialCall:
        case SysDbgClearSpecialCalls:
        case SysDbgQuerySpecialCalls:
        case SysDbgBreakPoint:
            break;

        case SysDbgQueryVersion:
            KDB_LOADUSERMODULE_HOOK((PLDR_DATA_TABLE_ENTRY) InputBuffer);
            break;

        default:
            break;
    }

    return STATUS_SUCCESS;
}
