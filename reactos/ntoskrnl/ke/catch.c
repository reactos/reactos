/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/catch.c
 * PURPOSE:              Exception handling
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <windows.h>
#include <ddk/ntddk.h>

/* FUNCTIONS ****************************************************************/

VOID ExRaiseStatus(NTSTATUS Status)
{
   DbgPrint("ExRaiseStatus(%d)\n",Status);
   for(;;);
}


NTSTATUS
STDCALL
NtRaiseException(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PCONTEXT Context,
	IN BOOL IsDebugger OPTIONAL
	)
{
}

NTSTATUS
STDCALL
ZwRaiseException(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN PCONTEXT Context,
	IN BOOL IsDebugger OPTIONAL
	)
{
}
