/* $Id:$
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/dbgctrl.c
 * PURPOSE:         System debug control
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
DbgLoadImageSymbols(
    IN PUNICODE_STRING Name,
    IN ULONG Base, 
    IN ULONG Unknown3
    )
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


NTSTATUS STDCALL 
NtSystemDebugControl(DEBUG_CONTROL_CODE ControlCode,
		     PVOID InputBuffer,
		     ULONG InputBufferLength,
		     PVOID OutputBuffer,
		     ULONG OutputBufferLength,
		     PULONG ReturnLength)
{
  switch (ControlCode) {
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
