/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/profile.c
 * PURPOSE:         Kernel Profiling
 * 
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern LIST_ENTRY ProcessProfileListHashTable[PROFILE_HASH_TABLE_SIZE];
extern LIST_ENTRY SystemProfileList;
extern KSPIN_LOCK ProfileListLock;
extern BOOLEAN ProfileInitDone;

/* FUNCTIONS *****************************************************************/

VOID
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

      if (current->Base <= Eip && ((char*)current->Base + current->Size) > (char*)Eip &&
	  current->Started)
	{
	  ULONG Bucket;

	  Bucket = ((ULONG)((char*)Eip - (char*)current->Base)) >> current->BucketShift;

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
      HANDLE Pid;
      PKPROCESS_PROFILE current;
      PLIST_ENTRY current_entry;
      PLIST_ENTRY ListHead;

      Pid = Profile->Process->UniqueProcessId;
      ListHead = &ProcessProfileListHashTable[(ULONG_PTR)Pid % PROFILE_HASH_TABLE_SIZE];

      current_entry = ListHead;
      while(current_entry != ListHead)
	{
	  current = CONTAINING_RECORD(current_entry, KPROCESS_PROFILE, 
				      ListEntry);

	  if (current->Pid == Pid)
	    {
	      KiInsertProfileIntoProcess(&current->ProfileListHead, Profile);
	      KeReleaseSpinLock(&ProfileListLock, oldIrql);
	      return;
	    }

	  current_entry = current_entry->Flink;
	}

      current = ExAllocatePool(NonPagedPool, sizeof(KPROCESS_PROFILE));

      current->Pid = Pid;
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
      HANDLE Pid;
      PLIST_ENTRY ListHead;
      PKPROCESS_PROFILE current;
      PLIST_ENTRY current_entry;
      
      RemoveEntryList(&Profile->ListEntry);

      Pid = Profile->Process->UniqueProcessId;
      ListHead = &ProcessProfileListHashTable[(ULONG_PTR)Pid % PROFILE_HASH_TABLE_SIZE];

      current_entry = ListHead;
      while(current_entry != ListHead)
	{
	  current = CONTAINING_RECORD(current_entry, KPROCESS_PROFILE, 
				      ListEntry);

	  if (current->Pid == Pid)
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
      KEBUGCHECK(0);
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

/*
 * @unimplemented
 */
STDCALL
VOID
KeProfileInterrupt(
    PKTRAP_FRAME TrapFrame
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeProfileInterruptWithSource(
	IN PKTRAP_FRAME   		TrapFrame,
	IN KPROFILE_SOURCE		Source
)
{
	UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
STDCALL
VOID
KeSetProfileIrql(
    IN KIRQL ProfileIrql
)
{
	UNIMPLEMENTED;
}
