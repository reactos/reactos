/* $Id: env.c,v 1.3 1999/12/06 00:22:43 ekohl Exp $
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

//#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
STDCALL
RtlCreateEnvironment (
	BOOLEAN	Initialize,
	PVOID	*Environment
	)
{
	MEMORY_BASIC_INFORMATION MemInfo;
	PVOID EnvPtr = NULL;
	NTSTATUS Status = STATUS_SUCCESS;
	ULONG RegionSize = 1;

	if (Initialize == FALSE)
	{
		RtlAcquirePebLock ();
#if 0
		if (NtCurrentPeb()->ProcessParameters->Environment != NULL)
		{
			Status = NtQueryVirtualMemory (NtCurrentProcess (),
			                               NtCurrentPeb ()->ProcessParameters->Environment,
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
			         NtCurrentPeb ()->ProcessParameters->Environment,
			         MemInfo.RegionSize);

			*Environment = EnvPtr;
		}
#endif
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

	DPRINT ("NewEnvironment %x OldEnvironment %x\n",
	        NewEnvironment, OldEnvironment);

	RtlAcquirePebLock ();

	EnvPtr = NtCurrentPeb()->Ppb->Environment;
	NtCurrentPeb()->Ppb->Environment = NewEnvironment;

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
