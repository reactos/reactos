/* $Id: catch.c,v 1.7 2000/03/03 00:47:06 ekohl Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 ntoskrnl/ke/catch.c
 * PURPOSE:              Exception handling
 * PROGRAMMER:           David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/

VOID
STDCALL
ExRaiseAccessViolation (
	VOID
	)
{
	ExRaiseStatus (STATUS_ACCESS_VIOLATION);
}

VOID
STDCALL
ExRaiseDatatypeMisalignment (
	VOID
	)
{
	ExRaiseStatus (STATUS_DATATYPE_MISALIGNMENT);
}

VOID
STDCALL
ExRaiseStatus (
	IN	NTSTATUS	Status
	)
{
	DbgPrint("ExRaiseStatus(%x)\n",Status);
	for(;;);
}


NTSTATUS
STDCALL
NtRaiseException (
	IN	PEXCEPTION_RECORD	ExceptionRecord,
	IN	PCONTEXT		Context,
	IN	BOOL			IsDebugger	OPTIONAL
	)
{
	UNIMPLEMENTED;
}

/* EOF */
