/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/bug.c
 * PURPOSE:         Graceful system shutdown if a bug is detected
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
NtCreateMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN InitialOwner
	)
{
}

NTSTATUS
STDCALL
ZwCreateMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN OBJECT_ATTRIBUTES ObjectAttributes,
	IN BOOLEAN InitialOwner
	)
{
}


NTSTATUS
STDCALL
NtOpenMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL
ZwOpenMutant(
	OUT PHANDLE MutantHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL
NtQueryMutant(
	IN HANDLE MutantHandle,
	IN CINT MutantInformationClass,
	OUT PVOID MutantInformation,
	IN ULONG Length,
	OUT PULONG ResultLength 
	)
{
}

NTSTATUS
STDCALL
ZwQueryMutant(
	IN HANDLE MutantHandle,
	IN CINT MutantInformationClass,
	OUT PVOID MutantInformation,
	IN ULONG Length,
	OUT PULONG ResultLength 
	)
{
}


NTSTATUS
STDCALL	
NtReleaseMutant(
	IN HANDLE MutantHandle,
	IN PULONG ReleaseCount OPTIONAL 
	)
{
}

NTSTATUS
STDCALL	
ZwReleaseMutant(
	IN HANDLE MutantHandle,
	IN PULONG ReleaseCount OPTIONAL 
	)
{
}
