/* $Id: env.c,v 1.1 1999/12/01 15:14:59 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/env.c
 * PURPOSE:         Environment functions
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  Created 30/09/98
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <internal/teb.h>
#include <string.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
RtlCreateEnvironment (
	BOOLEAN	Inherit,
	PVOID	*Environment
	)
{
	MEMORY_BASIC_INFORMATION MemInfo;
	PVOID EnvPtr = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG RegionSize = 1;

	if (Inherit == TRUE)
	{
		RtlAcquirePebLock ();

		if (NtCurrentPeb()->ProcessInfo->Environment != NULL)
		{
			Status = NtQueryVirtualMemory (NtCurrentProcess (),
			                               NtCurrentPeb ()->ProcessInfo->Environment,
			                               MemoryBasicInformation,
			                               &MemInfo,
			                               sizeof(MEMORY_BASIC_INFORMATION),
			                               NULL);
			if (!NT_SUCCESS(Status))
			{
				RtlReleasePebLock ();
				*Environment = NULL;
				return Status;
			}

			RegionSize = MemInfo.RegionSize;
			Status = NtAllocateVirtualMemory (NtCurrentProcess (),
			                                  &EnvPtr,
			                                  0,
			                                  &RegionSize,
			                                  MEM_COMMIT,
			                                  PAGE_READWRITE);
			if (!NT_SUCCESS(Status))
			{
				RtlReleasePebLock ();
				*Environment = NULL;
				return Status;
			}

			memmove (EnvPtr,
			         NtCurrentPeb ()->ProcessInfo->Environment,
			         MemInfo.RegionSize);

			*Environment = EnvPtr;
		}
		RtlReleasePebLock ();
	}
	else
	{
		RegionSize = 1;
		Status = NtAllocateVirtualMemory (NtCurrentProcess (),
		                                  &EnvPtr,
		                                  0,
		                                  &RegionSize,
		                                  MEM_COMMIT,
		                                  PAGE_READWRITE);
		if (NT_SUCCESS(Status))
			*Environment = EnvPtr;
	}

	return Status;
}


VOID
STDCALL
RtlDestroyEnvironment (
	PVOID	Environment
	)
{
	ULONG Size = 0;

	NtFreeVirtualMemory (NtCurrentProcess (),
	                     &Environment,
	                     &Size,
	                     MEM_RELEASE);
}


VOID
STDCALL
RtlSetCurrentEnvironment (
	PVOID	NewEnvironment,
	PVOID	*OldEnvironment
	)
{
	PVOID EnvPtr;

	RtlAcquirePebLock ();

	EnvPtr = NtCurrentPeb()->ProcessInfo->Environment;
	NtCurrentPeb()->ProcessInfo->Environment = NewEnvironment;

	if (OldEnvironment != NULL)
		*OldEnvironment = EnvPtr;

	RtlReleasePebLock ();
}


NTSTATUS
STDCALL
RtlSetEnvironmentVariable (
	PVOID		*Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	)
{
	NTSTATUS Status;

	Status = STATUS_VARIABLE_NOT_FOUND;

	/* FIXME: add missing stuff */


	return Status;
}


NTSTATUS
STDCALL
RtlQueryEnvironmentVariable_U (
	PVOID		Environment,
	PUNICODE_STRING	Name,
	PUNICODE_STRING	Value
	)
{
	NTSTATUS Status;

	Status = STATUS_VARIABLE_NOT_FOUND;

	/* FIXME: add missing stuff */


	return Status;
}



/* EOF */
