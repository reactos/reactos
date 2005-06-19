/* $Id$
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           User-mode callback support
 * FILE:              lib/ntdll/rtl/callback.c
 * PROGRAMER:         David Welch <welch@cwcom.net>
 */

/* INCLUDES *****************************************************************/

#include <ntdll.h>
#define NDEBUG
#include <debug.h>
/* TYPES *********************************************************************/

typedef NTSTATUS (STDCALL *KERNEL_CALLBACK_FUNCTION)(PVOID Argument,
                                                    ULONG ArgumentLength);

/* FUNCTIONS *****************************************************************/

VOID STDCALL
KiUserCallbackDispatcher(ULONG RoutineIndex,
			 PVOID Argument,
			 ULONG ArgumentLength)
{
   PPEB Peb;
   NTSTATUS Status;
   KERNEL_CALLBACK_FUNCTION Callback;

   Peb = NtCurrentPeb();
   Callback = (KERNEL_CALLBACK_FUNCTION)Peb->KernelCallbackTable[RoutineIndex];
   Status = Callback(Argument, ArgumentLength);
   ZwCallbackReturn(NULL, 0, Status);
}
