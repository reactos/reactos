/*
 *  ReactOS kernel
 *  Copyright (C) 1998-2003 ReactOS Team
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
/* $Id: profile.c,v 1.1 2003/01/15 19:58:07 chorns Exp $
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/dbg/profile.c
 * PURPOSE:         Kernel profiling
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *                  Created 12/01/2003
 */

/* INCLUDES *****************************************************************/

#define NTOS_MODE_KERNEL
#include <ntos.h>
#include <internal/ldr.h>
#include "kdb.h"

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

#define PROFILE_SESSION_LENGTH 30 /* Session length in seconds */

typedef struct _PROFILE_DATABASE_ENTRY
{
  ULONG_PTR Address;
} PROFILE_DATABASE_ENTRY, *PPROFILE_DATABASE_ENTRY;

#define PDE_BLOCK_ENTRIES ((PAGE_SIZE - (sizeof(LIST_ENTRY) + sizeof(ULONG))) / sizeof(PROFILE_DATABASE_ENTRY))

typedef struct _PROFILE_DATABASE_BLOCK
{
   LIST_ENTRY ListEntry;
   ULONG UsedEntries;
   PROFILE_DATABASE_ENTRY Entries[PDE_BLOCK_ENTRIES];
} PROFILE_DATABASE_BLOCK, *PPROFILE_DATABASE_BLOCK;

typedef struct _PROFILE_DATABASE
{
  LIST_ENTRY ListHead;
} PROFILE_DATABASE, *PPROFILE_DATABASE;

typedef struct _SAMPLE_GROUP_INFO
{
  ULONG_PTR Address;
  ULONG Count;
  CHAR Description[128];
  LIST_ENTRY ListEntry;
} SAMPLE_GROUP_INFO, *PSAMPLE_GROUP_INFO;

static volatile BOOLEAN KdbProfilingInitialized = FALSE;
static volatile BOOLEAN KdbProfilingEnabled = FALSE;
static volatile BOOLEAN KdbProfilingSuspended = FALSE;
static PPROFILE_DATABASE KdbProfileDatabase = NULL;
static KDPC KdbProfilerCollectorDpc;
static HANDLE KdbProfilerThreadHandle;
static CLIENT_ID KdbProfilerThreadCid;
static HANDLE KdbProfilerLogFile;
static KTIMER KdbProfilerTimer;
static KMUTEX KdbProfilerLock;
static BOOLEAN KdbEnableProfiler = FALSE;

VOID
KdbDeleteProfileDatabase(PPROFILE_DATABASE ProfileDatabase)
{
  PLIST_ENTRY current = NULL;

  current = RemoveHeadList(&ProfileDatabase->ListHead);
  while (current != &ProfileDatabase->ListHead)
    {
      PPROFILE_DATABASE_BLOCK block = CONTAINING_RECORD(
		current, PROFILE_DATABASE_BLOCK, ListEntry);
	  ExFreePool(block);
	  current = RemoveHeadList(&ProfileDatabase->ListHead);
    }
}

VOID
KdbAddEntryToProfileDatabase(PPROFILE_DATABASE ProfileDatabase, ULONG_PTR Address)
{
  PPROFILE_DATABASE_BLOCK block;

  if (IsListEmpty(&ProfileDatabase->ListHead))
    {
      block = ExAllocatePool(NonPagedPool, sizeof(PROFILE_DATABASE_BLOCK));
      assert(block);
      block->UsedEntries = 0;
      InsertTailList(&ProfileDatabase->ListHead, &block->ListEntry);
      block->Entries[block->UsedEntries++].Address = Address;
      return;
    }

  block = CONTAINING_RECORD(ProfileDatabase->ListHead.Blink, PROFILE_DATABASE_BLOCK, ListEntry);
  if (block->UsedEntries >= PDE_BLOCK_ENTRIES)
    {
      block = ExAllocatePool(NonPagedPool, sizeof(PROFILE_DATABASE_BLOCK));
      assert(block);
      block->UsedEntries = 0;
      InsertTailList(&ProfileDatabase->ListHead, &block->ListEntry);
    }
  block->Entries[block->UsedEntries++].Address = Address;
}

VOID
KdbInitProfiling()
{
  KdbEnableProfiler = TRUE;
}

VOID
KdbInitProfiling2()
{
  if (KdbEnableProfiler)
    {
      KdbEnableProfiling();
      KdbProfilingInitialized = TRUE;
    }
}

VOID
KdbSuspendProfiling()
{
  KdbProfilingSuspended = TRUE;
}

VOID
KdbResumeProfiling()
{
  KdbProfilingSuspended = FALSE;
}

BOOLEAN
KdbProfilerGetSymbolInfo(PVOID address, OUT PCH NameBuffer)
{
   PLIST_ENTRY current_entry;
   MODULE_TEXT_SECTION* current;
   extern LIST_ENTRY ModuleTextListHead;
   ULONG_PTR RelativeAddress;
   NTSTATUS Status;
   ULONG LineNumber;
   CHAR FileName[256];
   CHAR FunctionName[256];

   current_entry = ModuleTextListHead.Flink;
   
   while (current_entry != &ModuleTextListHead &&
	  current_entry != NULL)
     {
	current = 
	  CONTAINING_RECORD(current_entry, MODULE_TEXT_SECTION, ListEntry);

	if (address >= (PVOID)current->Base &&
	    address < (PVOID)(current->Base + current->Length))
	  {
            RelativeAddress = (ULONG_PTR) address - current->Base;
            Status = LdrGetAddressInformation(&current->SymbolInfo,
              RelativeAddress,
              &LineNumber,
              FileName,
              FunctionName);
            if (NT_SUCCESS(Status))
              {
                sprintf(NameBuffer, "%s (%s)", FileName, FunctionName);
                return(TRUE);
              }
	     return(TRUE);
	  }
	current_entry = current_entry->Flink;
     }
   return(FALSE);
}

PLIST_ENTRY
KdbProfilerLargestSampleGroup(PLIST_ENTRY SamplesListHead)
{
  PLIST_ENTRY current;
  PLIST_ENTRY largest;
  ULONG count;

  count = 0;
  largest = SamplesListHead->Flink;
  current = SamplesListHead->Flink;
  while (current != SamplesListHead)
    {
      PSAMPLE_GROUP_INFO sgi = CONTAINING_RECORD(
		current, SAMPLE_GROUP_INFO, ListEntry);

      if (sgi->Count > count)
        {
          largest = current;
          count = sgi->Count;
        }

	  current = current->Flink;
    }
  if (count == 0)
    {
      return NULL;
    }
  return largest;
}

VOID
KdbProfilerWriteString(PCH String)
{
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  ULONG Length;

  Length = strlen(String);
  Status = NtWriteFile(KdbProfilerLogFile,
    NULL,
    NULL,
    NULL,
    &Iosb,
    String,
    Length,
    NULL,
    NULL);

 if (!NT_SUCCESS(Status))
   {
     DPRINT1("NtWriteFile() failed with status 0x%.08x\n", Status);
   }
}

NTSTATUS
KdbProfilerWriteSampleGroups(PLIST_ENTRY SamplesListHead)
{
  CHAR Buffer[256];
  PLIST_ENTRY current = NULL;
  PLIST_ENTRY Largest;

  KdbProfilerWriteString("\r\n\r\n");
  KdbProfilerWriteString("Count     Symbol\n");
  KdbProfilerWriteString("--------------------------------------------------\r\n");

  current = SamplesListHead->Flink;
  while (current != SamplesListHead)
    {
      Largest = KdbProfilerLargestSampleGroup(SamplesListHead);
      if (Largest != NULL)
        {
          PSAMPLE_GROUP_INFO sgi = CONTAINING_RECORD(
		    Largest, SAMPLE_GROUP_INFO, ListEntry);

		  //DbgPrint("%.08d  %s\n", sgi->Count, sgi->Description);

		  sprintf(Buffer, "%.08d  %s\r\n", sgi->Count, sgi->Description);
          KdbProfilerWriteString(Buffer);

          RemoveEntryList(Largest);
          ExFreePool(sgi);
        }
      else
        {
          break;
        }

	  current = SamplesListHead->Flink;
    }

  return STATUS_SUCCESS;
}

LONG STDCALL
KdbProfilerKeyCompare(IN PVOID  Key1,
  IN PVOID  Key2)
{
  int value = strcmp(Key1, Key2);

  if (value == 0)
    return 0;

  return (value < 0) ? -1 : 1;
}


NTSTATUS
KdbProfilerAnalyzeSamples()
{
  CHAR NameBuffer[512];
  ULONG KeyLength;
  PLIST_ENTRY current = NULL;
  HASH_TABLE Hashtable;
  LIST_ENTRY SamplesListHead;
  ULONG Index;
  ULONG_PTR Address;

  if (!ExInitializeHashTable(&Hashtable, 17, KdbProfilerKeyCompare, TRUE))
    {
      DPRINT1("ExInitializeHashTable() failed.");
      KeBugCheck(0);
    }

  InitializeListHead(&SamplesListHead);

  current = RemoveHeadList(&KdbProfileDatabase->ListHead);
  while (current != &KdbProfileDatabase->ListHead)
    {
      PPROFILE_DATABASE_BLOCK block;

      block = CONTAINING_RECORD(current, PROFILE_DATABASE_BLOCK, ListEntry);

      for (Index = 0; Index < block->UsedEntries; Index++)
        {
          PSAMPLE_GROUP_INFO sgi;
          Address = block->Entries[Index].Address;
	      if (KdbProfilerGetSymbolInfo((PVOID) Address, (PCH) &NameBuffer))
	        {
	        }
	      else
		    {
	          sprintf(NameBuffer, "(0x%.08x)", (ULONG) Address);
		    }

	      KeyLength = strlen(NameBuffer);
	      if (!ExSearchHashTable(&Hashtable, (PVOID) NameBuffer, KeyLength, (PVOID *) &sgi))
	        {
	          sgi = ExAllocatePool(NonPagedPool, sizeof(SAMPLE_GROUP_INFO));
	          assert(sgi);
              sgi->Address = Address;
	          sgi->Count = 1;
	          strcpy(sgi->Description, NameBuffer);
	          InsertTailList(&SamplesListHead, &sgi->ListEntry);
	          ExInsertHashTable(&Hashtable, sgi->Description, KeyLength, (PVOID) sgi);
	        }
	      else
	        {
	          sgi->Count++;
	        }
        }

      ExFreePool(block);

      current = RemoveHeadList(&KdbProfileDatabase->ListHead);
    }

  KdbProfilerWriteSampleGroups(&SamplesListHead);

  ExDeleteHashTable(&Hashtable);

  KdbDeleteProfileDatabase(KdbProfileDatabase);

  return STATUS_SUCCESS;
}

NTSTATUS
KdbProfilerThreadMain(PVOID Context)
{
  for (;;)
    {
      KeWaitForSingleObject(&KdbProfilerTimer, Executive, KernelMode, TRUE, NULL);

      KeWaitForSingleObject(&KdbProfilerLock, Executive, KernelMode, FALSE, NULL);

	  KdbSuspendProfiling();

      KdbProfilerAnalyzeSamples();

	  KdbResumeProfiling();

	  KeReleaseMutex(&KdbProfilerLock, FALSE);
	}
}

VOID
KdbDisableProfiling()
{
  if (KdbProfilingEnabled == TRUE)
    {
      /* FIXME: Implement */
#if 0
      KdbProfilingEnabled = FALSE;
      /* Stop timer */
      /* Close file */
      if (KdbProfileDatabase != NULL)
        {
          KdbDeleteProfileDatabase(KdbProfileDatabase);
          ExFreePool(KdbProfileDatabase);
          KdbProfileDatabase = NULL;
        }
#endif
    }
}

/*
 * SystemArgument1 = EIP
 */
static VOID STDCALL
KdbProfilerCollectorDpcRoutine(PKDPC Dpc, PVOID DeferredContext,
  PVOID SystemArgument1, PVOID SystemArgument2)
{
  ULONG_PTR address = (ULONG_PTR) SystemArgument1;

  KdbAddEntryToProfileDatabase(KdbProfileDatabase, address);
}

VOID
KdbEnableProfiling()
{
  if (KdbProfilingEnabled == FALSE)
    {
	  NTSTATUS Status;
	  OBJECT_ATTRIBUTES ObjectAttributes;
	  UNICODE_STRING FileName;
	  IO_STATUS_BLOCK Iosb;
      LARGE_INTEGER DueTime;

	  RtlInitUnicodeString(&FileName, L"\\SystemRoot\\profiler.log");
	  InitializeObjectAttributes(&ObjectAttributes,
		&FileName,
		0,
		NULL,
		NULL);
	
	  Status = NtCreateFile(&KdbProfilerLogFile,
		FILE_ALL_ACCESS,
		&ObjectAttributes,
		&Iosb,
		NULL,
		FILE_ATTRIBUTE_NORMAL,
		0,
		FILE_SUPERSEDE,
		FILE_WRITE_THROUGH | FILE_SYNCHRONOUS_IO_NONALERT,
		NULL,
		0);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("Failed to create profiler log file\n");
	      return;
	    }

	  Status = PsCreateSystemThread(&KdbProfilerThreadHandle,
		THREAD_ALL_ACCESS,
		NULL,
		NULL,
		&KdbProfilerThreadCid,
		KdbProfilerThreadMain,
		NULL);
	  if (!NT_SUCCESS(Status))
	    {
	      DPRINT1("Failed to create profiler thread\n");
	      return;
	    }

      KeInitializeMutex(&KdbProfilerLock, 0);

      KdbProfileDatabase = ExAllocatePool(NonPagedPool, sizeof(PROFILE_DATABASE));
      assert(KdbProfileDatabase);
      InitializeListHead(&KdbProfileDatabase->ListHead);
      KeInitializeDpc(&KdbProfilerCollectorDpc, KdbProfilerCollectorDpcRoutine, NULL);

	  /* Initialize our periodic timer and its associated DPC object. When the timer
	     expires, the KdbProfilerSessionEndDpc deferred procedure call (DPC) is queued */
	  KeInitializeTimerEx(&KdbProfilerTimer, SynchronizationTimer);

	  /* Start the periodic timer with an initial and periodic
	     relative expiration time of PROFILE_SESSION_LENGTH seconds */
	  DueTime.QuadPart = -(LONGLONG) PROFILE_SESSION_LENGTH * 1000 * 10000;
	  KeSetTimerEx(&KdbProfilerTimer, DueTime, PROFILE_SESSION_LENGTH * 1000, NULL);

      KdbProfilingEnabled = TRUE;
    }
}

VOID
KdbProfileInterrupt(ULONG_PTR Address)
{
  assert(KeGetCurrentIrql() == PROFILE_LEVEL);

  if (KdbProfilingInitialized != TRUE)
    {
      return;
    }

  if ((KdbProfilingEnabled) && (!KdbProfilingSuspended))
    {
      (BOOLEAN) KeInsertQueueDpc(&KdbProfilerCollectorDpc, (PVOID) Address, NULL);
    }
}
