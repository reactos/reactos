/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/profile.c
 * PURPOSE:         Support for profiling
 * PROGRAMMER:      Nobody
 * UPDATE HISTORY:
 *                  
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* TYPES ********************************************************************/

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExProfileObjectType = NULL;

static GENERIC_MAPPING ExpProfileMapping = {
	STANDARD_RIGHTS_READ,
	STANDARD_RIGHTS_WRITE,
	STANDARD_RIGHTS_EXECUTE,
	STANDARD_RIGHTS_ALL};

/*
 * Size of the profile hash table.
 */
#define PROFILE_HASH_TABLE_SIZE      (32)

/*
 * Table of lists of per-process profiling data structures hashed by PID.
 */
LIST_ENTRY ProcessProfileListHashTable[PROFILE_HASH_TABLE_SIZE];

/*
 * Head of the list of profile data structures for the kernel.
 */
LIST_ENTRY SystemProfileList;

/*
 * Lock that protects the profiling data structures.
 */
KSPIN_LOCK ProfileListLock;

/*
 * Timer interrupts happen before we have initialized the profiling
 * data structures so just ignore them before that.
 */
BOOLEAN ProfileInitDone = FALSE;

VOID INIT_FUNCTION
ExpInitializeProfileImplementation(VOID)
{
  ULONG i;

  InitializeListHead(&SystemProfileList);
  
  for (i = 0; i < PROFILE_HASH_TABLE_SIZE; i++)
    {
      InitializeListHead(&ProcessProfileListHashTable[i]);
    }

  KeInitializeSpinLock(&ProfileListLock);
  ProfileInitDone = TRUE;

  ExProfileObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
  
  RtlCreateUnicodeString(&ExProfileObjectType->TypeName, L"Profile");
  
  ExProfileObjectType->Tag = TAG('P', 'R', 'O', 'F');
  ExProfileObjectType->PeakObjects = 0;
  ExProfileObjectType->PeakHandles = 0;
  ExProfileObjectType->TotalObjects = 0;
  ExProfileObjectType->TotalHandles = 0;
  ExProfileObjectType->PagedPoolCharge = 0;
  ExProfileObjectType->NonpagedPoolCharge = sizeof(KPROFILE);
  ExProfileObjectType->Mapping = &ExpProfileMapping;
  ExProfileObjectType->Dump = NULL;
  ExProfileObjectType->Open = NULL;
  ExProfileObjectType->Close = NULL;
  ExProfileObjectType->Delete = KiDeleteProfile;
  ExProfileObjectType->Parse = NULL;
  ExProfileObjectType->Security = NULL;
  ExProfileObjectType->QueryName = NULL;
  ExProfileObjectType->OkayToClose = NULL;
  ExProfileObjectType->Create = NULL;

  ObpCreateTypeObject(ExProfileObjectType);
}

NTSTATUS STDCALL 
NtCreateProfile(OUT PHANDLE ProfileHandle,
		IN HANDLE Process  OPTIONAL,
		IN PVOID ImageBase, 
		IN ULONG ImageSize, 
		IN ULONG BucketSize,
		IN PVOID Buffer,
		IN ULONG BufferSize,
		IN KPROFILE_SOURCE ProfileSource,
		IN KAFFINITY Affinity)
{
  HANDLE SafeProfileHandle;
  NTSTATUS Status;
  PKPROFILE Profile;
  PEPROCESS pProcess;

  /*
   * Reference the associated process
   */
  if (Process != NULL)
    {
      Status = ObReferenceObjectByHandle(Process,
					 PROCESS_QUERY_INFORMATION,
					 PsProcessType,
					 UserMode,
					 (PVOID*)&pProcess,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  else
    {
      pProcess = NULL;
      /* FIXME: Check privilege. */
    }

  /*
   * Check the parameters
   */
  if ((pProcess == NULL && ImageBase < (PVOID)KERNEL_BASE) ||
      (pProcess != NULL && ImageBase >= (PVOID)KERNEL_BASE))
    {
      return(STATUS_INVALID_PARAMETER_3);
    }
  if (((ImageSize >> BucketSize) * 4) >= BufferSize)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }
  if (ProfileSource != ProfileTime)
    {
      return(STATUS_INVALID_PARAMETER_9);
    }
  if (Affinity != 0)
    {
      return(STATUS_INVALID_PARAMETER_10);
    }

  /*
   * Create the object
   */
  Status = ObCreateObject(ExGetPreviousMode(),
			  ExProfileObjectType,
			  NULL,
			  ExGetPreviousMode(),
			  NULL,
			  sizeof(KPROFILE),
			  0,
			  0,
			  (PVOID*)&Profile);
  if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

  /*
   * Initialize it
   */
  Profile->Base = ImageBase;
  Profile->Size = ImageSize;
  Profile->BucketShift = BucketSize;
  Profile->BufferMdl = MmCreateMdl(NULL, Buffer, BufferSize);
  if(Profile->BufferMdl == NULL) {
	DPRINT("MmCreateMdl: Out of memory!");
	return(STATUS_NO_MEMORY);
  }  
  MmProbeAndLockPages(Profile->BufferMdl, UserMode, IoWriteAccess);
  Profile->Buffer = MmGetSystemAddressForMdl(Profile->BufferMdl);
  Profile->BufferSize = BufferSize;
  Profile->ProcessorMask = Affinity;
  Profile->Started = FALSE;
  Profile->Process = pProcess;

  /*
   * Insert the profile into the profile list data structures
   */
  KiInsertProfile(Profile);

  Status = ObInsertObject ((PVOID)Profile,
			   NULL,
			   STANDARD_RIGHTS_ALL,
			   0,
			   NULL,
			   &SafeProfileHandle);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Profile);
      return Status;
    }

  /*
   * Copy the created handle back to the caller
   */
  Status = MmCopyToCaller(ProfileHandle, &SafeProfileHandle, sizeof(HANDLE));
  if (!NT_SUCCESS(Status))
     {
       ObDereferenceObject(Profile);
       ZwClose(ProfileHandle);
       return(Status);
     }

  ObDereferenceObject(Profile);

  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtQueryIntervalProfile(IN  KPROFILE_SOURCE ProfileSource,
		       OUT PULONG Interval)
{
  NTSTATUS Status;

  if (ProfileSource == ProfileTime)
    {
      ULONG SafeInterval;

      /* FIXME: What units does this use, for now nanoseconds */
      SafeInterval = 100;
      Status = MmCopyToCaller(Interval, &SafeInterval, sizeof(ULONG));
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
      return(STATUS_SUCCESS);
    }
  return(STATUS_INVALID_PARAMETER_2);
}

NTSTATUS STDCALL 
NtSetIntervalProfile(IN ULONG Interval,
		     IN KPROFILE_SOURCE Source)
{
  return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS STDCALL 
NtStartProfile(IN HANDLE ProfileHandle)
{
  NTSTATUS Status;
  PKPROFILE Profile;

  Status = ObReferenceObjectByHandle(ProfileHandle,
				     STANDARD_RIGHTS_ALL,
				     ExProfileObjectType,
				     UserMode,
				     (PVOID*)&Profile,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  Profile->Started = TRUE;
  ObDereferenceObject(Profile);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL 
NtStopProfile(IN HANDLE ProfileHandle)
{
  NTSTATUS Status;
  PKPROFILE Profile;

  Status = ObReferenceObjectByHandle(ProfileHandle,
				     STANDARD_RIGHTS_ALL,
				     ExProfileObjectType,
				     UserMode,
				     (PVOID*)&Profile,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  Profile->Started = FALSE;
  ObDereferenceObject(Profile);
  return(STATUS_SUCCESS);
}


