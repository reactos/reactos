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
NtCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN ULONG InitialCount,
	IN ULONG MaximumCount
	)
{
}

NTSTATUS
STDCALL
ZwCreateSemaphore(
	OUT PHANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
	IN ULONG InitialCount,
	IN ULONG MaximumCount
	)
{
}


NTSTATUS
STDCALL
NtOpenSemaphore(
	IN HANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAcces,
	IN POBJECT_ATTRIBUTES ObjectAttributes
		)
{
}

NTSTATUS
STDCALL
ZwOpenSemaphore(
	IN HANDLE SemaphoreHandle,
	IN ACCESS_MASK DesiredAcces,
	IN POBJECT_ATTRIBUTES ObjectAttributes
	)
{
}

NTSTATUS
STDCALL
NtQuerySemaphore(
	HANDLE SemaphoreHandle,
	CINT SemaphoreInformationClass,
	OUT PVOID SemaphoreInformation,
	ULONG Length,
	PULONG ReturnLength
	)
{
}

NTSTATUS
STDCALL
ZwQuerySemaphore(
	HANDLE SemaphoreHandle,
	CINT SemaphoreInformationClass,
	OUT PVOID SemaphoreInformation,
	ULONG Length,
	PULONG ReturnLength
	)
{
}

NTSTATUS
STDCALL
NtReleaseSemaphore( 
	IN HANDLE SemaphoreHandle,
	IN ULONG ReleaseCount,
	IN PULONG PreviousCount
	)
{
}

NTSTATUS
STDCALL
ZwReleaseSemaphore( 
	IN HANDLE SemaphoreHandle,
	IN ULONG ReleaseCount,
	IN PULONG PreviousCount
	)
{
}
