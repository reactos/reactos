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

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ps.h>
#include <internal/pool.h>
#include <limits.h>
#include <internal/safe.h>

#include <internal/debug.h>

/* TYPES ********************************************************************/

typedef struct _KPROCESS_PROFILE
/*
 * List of the profile data structures associated with a process.
 */
{
  LIST_ENTRY ProfileListHead;
  LIST_ENTRY ListEntry;
  HANDLE Pid;
} KPROCESS_PROFILE, *PKPROCESS_PROFILE;

typedef struct _KPROFILE
/*
 * Describes a contiguous region of process memory that is being profiled.
 */
{
  CSHORT Type;
  CSHORT Name;

  /* Entry in the list of profile data structures for this process. */
  LIST_ENTRY ListEntry; 

  /* Base of the region being profiled. */
  PVOID Base;

  /* Size of the region being profiled. */
  ULONG Size;

  /* Shift of offsets from the region to buckets in the profiling buffer. */
  ULONG BucketShift;

  /* MDL which described the buffer that receives profiling data. */
  PMDL BufferMdl;

  /* System alias for the profiling buffer. */
  PULONG Buffer;

  /* Size of the buffer for profiling data. */
  ULONG BufferSize;

  /* 
   * Mask of processors for which profiling data should be collected. 
   * Currently unused.
   */
  ULONG ProcessorMask;

  /* TRUE if profiling has been started for this region. */
  BOOLEAN Started;

  /* Pointer (and reference) to the process which is being profiled. */
  PEPROCESS Process;
} KPROFILE, *PKPROFILE;

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
static LIST_ENTRY ProcessProfileListHashTable[PROFILE_HASH_TABLE_SIZE];

/*
 * Head of the list of profile data structures for the kernel.
 */
static LIST_ENTRY SystemProfileList;

/*
 * Lock that protects the profiling data structures.
 */
static KSPIN_LOCK ProfileListLock;

/*
 * Timer interrupts happen before we have initialized the profiling
 * data structures so just ignore them before that.
 */
static BOOLEAN ProfileInitDone = FALSE;

/* FUNCTIONS *****************************************************************/

VOID STATIC
KiAddProfileEventToProcess(PLIST_ENTRY ListHead, PVOID Eip)
     /*
      * Add a profile event to the profile objects for a particular process
      * or the system
      */
{
  PKPROFILE current;
  PLIST_ENTRY current_entry;

  current_entry = ListHead->Flink;
  while (current_entry != ListHead)
    {
      current = CONTAINING_RECORD(current_entry, KPROFILE, ListEntry);

      if (current->Base > Eip)
	{
	  return;
	}

      if (current->Base <= Eip && (current->Base + current->Size) > Eip &&
	  current->Started)
	{
	  ULONG Bucket;

	  Bucket = ((ULONG)(Eip - current->Base)) >> current->BucketShift;

	  if ((Bucket*4) < current->BufferSize)
	    {
	      current->Buffer[Bucket]++;
	    }
	}

      current_entry = current_entry->Flink;
    }
}

VOID
KiAddProfileEvent(KPROFILE_SOURCE Source, ULONG Eip)
     /*
      * Add a profile event 
      */
{
  HANDLE Pid;
  PKPROCESS_PROFILE current;
  PLIST_ENTRY current_entry;
  PLIST_ENTRY ListHead;

  if (!ProfileInitDone)
    {
      return;
    }

  Pid = PsGetCurrentProcessId();
  ListHead = 
    ProcessProfileListHashTable[(ULONG)Pid % PROFILE_HASH_TABLE_SIZE].Flink;

  KeAcquireSpinLockAtDpcLevel(&ProfileListLock);

  current_entry = ListHead;
  while (current_entry != ListHead)
    {
      current = CONTAINING_RECORD(current_entry, KPROCESS_PROFILE, ListEntry);

      if (current->Pid == Pid)
	{
	  KiAddProfileEventToProcess(&current->ProfileListHead, (PVOID)Eip);
	  break;
	}

      current_entry = current_entry->Flink;
    }

  KiAddProfileEventToProcess(&SystemProfileList, (PVOID)Eip);

  KeReleaseSpinLockFromDpcLevel(&ProfileListLock);
}

VOID
KiInsertProfileIntoProcess(PLIST_ENTRY ListHead, PKPROFILE Profile)
     /*
      * Insert a profile object into the list for a process or the system
      */
{
  PKPROFILE current;
  PLIST_ENTRY current_entry;

  current_entry = ListHead;
  while (current_entry != ListHead)
    {
      current = CONTAINING_RECORD(current_entry, KPROFILE, ListEntry);

      if (current->Base > Profile->Base)
	{
	  Profile->ListEntry.Flink = current_entry;
	  Profile->ListEntry.Blink = current_entry->Blink;
	  current_entry->Blink->Flink = &Profile->ListEntry;
	  current_entry->Blink = &Profile->ListEntry;
	  return;
	}

      current_entry = current_entry->Flink;
    }
  InsertTailList(ListHead, &Profile->ListEntry);
}

VOID
KiInsertProfile(PKPROFILE Profile)
     /*
      * Insert a profile into the relevant data structures
      */
{
  KIRQL oldIrql;

  KeAcquireSpinLock(&ProfileListLock, &oldIrql);

  if (Profile->Process == NULL)
    {
      KiInsertProfileIntoProcess(&SystemProfileList, Profile);
    }
  else
    {
      ULONG Pid;
      PKPROCESS_PROFILE current;
      PLIST_ENTRY current_entry;
      PLIST_ENTRY ListHead;

      Pid = Profile->Process->UniqueProcessId;
      ListHead = &ProcessProfileListHashTable[Pid % PROFILE_HASH_TABLE_SIZE];

      current_entry = ListHead;
      while(current_entry != ListHead)
	{
	  current = CONTAINING_RECORD(current_entry, KPROCESS_PROFILE, 
				      ListEntry);

	  if (current->Pid == (HANDLE)Pid)
	    {
	      KiInsertProfileIntoProcess(&current->ProfileListHead, Profile);
	      KeReleaseSpinLock(&ProfileListLock, oldIrql);
	      return;
	    }

	  current_entry = current_entry->Flink;
	}

      current = ExAllocatePool(NonPagedPool, sizeof(KPROCESS_PROFILE));

      current->Pid = (HANDLE)Pid;
      InitializeListHead(&current->ProfileListHead);
      InsertTailList(ListHead, &current->ListEntry);

      KiInsertProfileIntoProcess(&current->ProfileListHead, Profile);
    }

  KeReleaseSpinLock(&ProfileListLock, oldIrql);
}

VOID KiRemoveProfile(PKPROFILE Profile)
{
  KIRQL oldIrql;

  KeAcquireSpinLock(&ProfileListLock, &oldIrql);

  if (Profile->Process == NULL)
    {
      RemoveEntryList(&Profile->ListEntry);
    }
  else
    {
      ULONG Pid;
      PLIST_ENTRY ListHead;
      PKPROCESS_PROFILE current;
      PLIST_ENTRY current_entry;
      
      RemoveEntryList(&Profile->ListEntry);

      Pid = Profile->Process->UniqueProcessId;
      ListHead = &ProcessProfileListHashTable[Pid % PROFILE_HASH_TABLE_SIZE];

      current_entry = ListHead;
      while(current_entry != ListHead)
	{
	  current = CONTAINING_RECORD(current_entry, KPROCESS_PROFILE, 
				      ListEntry);

	  if (current->Pid == (HANDLE)Pid)
	    {
	      if (IsListEmpty(&current->ProfileListHead))
		{
		  RemoveEntryList(&current->ListEntry);
		  ExFreePool(current);
		}
	      KeReleaseSpinLock(&ProfileListLock, oldIrql);
	      return;
	    }

	  current_entry = current_entry->Flink;
	}
      KeBugCheck(0);
    }

  KeReleaseSpinLock(&ProfileListLock, oldIrql);
}

VOID STDCALL
KiDeleteProfile(PVOID ObjectBody)
{
  PKPROFILE Profile;

  Profile = (PKPROFILE)ObjectBody;

  KiRemoveProfile(Profile);
  if (Profile->Process != NULL)
    {
      ObDereferenceObject(Profile->Process);
      Profile->Process = NULL;
    }

  if (Profile->BufferMdl->MappedSystemVa != NULL)
    {	     
      MmUnmapLockedPages(Profile->BufferMdl->MappedSystemVa, 
			 Profile->BufferMdl);
    }
  MmUnlockPages(Profile->BufferMdl);
  ExFreePool(Profile->BufferMdl);
  Profile->BufferMdl = NULL;
}

VOID
NtInitializeProfileImplementation(VOID)
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
  ExProfileObjectType->MaxObjects = ULONG_MAX;
  ExProfileObjectType->MaxHandles = ULONG_MAX;
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
}

NTSTATUS STDCALL 
NtCreateProfile(OUT PHANDLE UnsafeProfileHandle, 
		IN HANDLE ProcessHandle,
		IN PVOID ImageBase, 
		IN ULONG ImageSize, 
		IN ULONG Granularity,
		OUT PULONG Buffer, 
		IN ULONG BufferSize,
		IN KPROFILE_SOURCE Source,
		IN ULONG ProcessorMask)
{
  HANDLE ProfileHandle;
  NTSTATUS Status;
  PKPROFILE Profile;
  PEPROCESS Process;

  /*
   * Reference the associated process
   */
  if (ProcessHandle != NULL)
    {
      Status = ObReferenceObjectByHandle(ProcessHandle,
					 PROCESS_QUERY_INFORMATION,
					 PsProcessType,
					 UserMode,
					 (PVOID*)&Process,
					 NULL);
      if (!NT_SUCCESS(Status))
	{
	  return(Status);
	}
    }
  else
    {
      Process = NULL;
      /* FIXME: Check privilege. */
    }

  /*
   * Check the parameters
   */
  if ((Process == NULL && ImageBase < (PVOID)KERNEL_BASE) ||
      (Process != NULL && ImageBase >= (PVOID)KERNEL_BASE))
    {
      return(STATUS_INVALID_PARAMETER_3);
    }
  if (((ImageSize >> Granularity) * 4) >= BufferSize)
    {
      return(STATUS_BUFFER_TOO_SMALL);
    }
  if (Source != ProfileTime)
    {
      return(STATUS_INVALID_PARAMETER_9);
    }
  if (ProcessorMask != 0)
    {
      return(STATUS_INVALID_PARAMETER_10);
    }

  /*
   * Create the object
   */
  Status = ObCreateObject(&ProfileHandle,
			  STANDARD_RIGHTS_ALL,
			  NULL,
			  ExProfileObjectType,
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
  Profile->BucketShift = Granularity;
  Profile->BufferMdl = MmCreateMdl(NULL, Buffer, BufferSize);
  MmProbeAndLockPages(Profile->BufferMdl, UserMode, IoWriteAccess);
  Profile->Buffer = MmGetSystemAddressForMdl(Profile->BufferMdl);
  Profile->BufferSize = BufferSize;
  Profile->ProcessorMask = ProcessorMask;
  Profile->Started = FALSE;
  Profile->Process = Process;

  /*
   * Insert the profile into the profile list data structures
   */
  KiInsertProfile(Profile);

  /*
   * Copy the created handle back to the caller
   */
  Status = MmCopyToCaller(UnsafeProfileHandle, &ProfileHandle, sizeof(HANDLE));
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
NtQueryIntervalProfile(OUT PULONG UnsafeInterval,
		       OUT KPROFILE_SOURCE Source)
{
  NTSTATUS Status;

  if (Source == ProfileTime)
    {
      ULONG Interval;

      /* FIXME: What units does this use, for now nanoseconds */
      Interval = 100;
      Status = MmCopyToCaller(UnsafeInterval, &Interval, sizeof(ULONG));
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


