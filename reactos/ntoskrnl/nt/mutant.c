/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/mutant.c
 * PURPOSE:         Synchronization primitives
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtCreateMutant (
	OUT	PHANDLE			MutantHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	BOOLEAN			InitialOwner
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtOpenMutant (
	OUT	PHANDLE			MutantHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtQueryMutant (
	IN	HANDLE	MutantHandle,
	IN	CINT	MutantInformationClass,
	OUT	PVOID	MutantInformation,
	IN	ULONG	Length,
	OUT	PULONG	ResultLength
	)
{
	UNIMPLEMENTED;
}


NTSTATUS
STDCALL
NtReleaseMutant (
	IN	HANDLE	MutantHandle,
	IN	PULONG	ReleaseCount	OPTIONAL
	)
{
	UNIMPLEMENTED;
}

/* EOF */
