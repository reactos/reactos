/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/ntsem.c
 * PURPOSE:         Synchronization primitives
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
			   IN ULONG InitialCount,
			   IN ULONG MaximumCount)
{
   return(ZwCreateSemaphore(SemaphoreHandle,
			    DesiredAccess,
			    ObjectAttributes,
			    InitialCount,
			    MaximumCount));
}

NTSTATUS STDCALL ZwCreateSemaphore(OUT PHANDLE SemaphoreHandle,
			      IN ACCESS_MASK DesiredAccess,
			      IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
			      IN ULONG InitialCount,
			      IN ULONG MaximumCount)
{
   UNIMPLEMENTED;
}


NTSTATUS STDCALL NtOpenSemaphore(IN HANDLE SemaphoreHandle,
				 IN ACCESS_MASK DesiredAccess,
				 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenSemaphore(SemaphoreHandle,
			  DesiredAccess,
			  ObjectAttributes));
}

NTSTATUS STDCALL ZwOpenSemaphore(IN HANDLE SemaphoreHandle,
				 IN ACCESS_MASK DesiredAccess,
				 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQuerySemaphore(HANDLE SemaphoreHandle,
				  CINT SemaphoreInformationClass,
				  OUT PVOID SemaphoreInformation,
				  ULONG Length,
				  PULONG ReturnLength)
{
   return(ZwQuerySemaphore(SemaphoreHandle,
			   SemaphoreInformationClass,
			   SemaphoreInformation,
			   Length,
			   ReturnLength));
}

NTSTATUS STDCALL ZwQuerySemaphore(HANDLE SemaphoreHandle,
				  CINT SemaphoreInformationClass,
				  OUT PVOID SemaphoreInformation,
				  ULONG Length,
				  PULONG ReturnLength)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
				    IN ULONG ReleaseCount,
				    IN PULONG PreviousCount)
{
   return(ZwReleaseSemaphore(SemaphoreHandle,
			     ReleaseCount,
			     PreviousCount));
}

NTSTATUS STDCALL ZwReleaseSemaphore(IN HANDLE SemaphoreHandle,
				    IN ULONG ReleaseCount,
				    IN PULONG PreviousCount)
{
   UNIMPLEMENTED;
}
