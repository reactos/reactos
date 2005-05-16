/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/cid.c
 * PURPOSE:         Client ID (CID) management
 *
 * PROGRAMMERS:     Thomas Weidenmueller <w3seek@reactos.com>
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PHANDLE_TABLE PspCidTable = NULL;

#define TAG_CIDOBJECT TAG('C', 'I', 'D', 'O')

#define CID_FLAG_PROCESS 0x1
#define CID_FLAG_THREAD 0x2
#define CID_FLAGS_MASK (CID_FLAG_PROCESS | CID_FLAG_THREAD)

/* FUNCTIONS *****************************************************************/

VOID INIT_FUNCTION
PsInitClientIDManagment(VOID)
{
  PspCidTable = ExCreateHandleTable(NULL);
  ASSERT(PspCidTable);
}

NTSTATUS
PsCreateCidHandle(PVOID Object, POBJECT_TYPE ObjectType, PHANDLE Handle)
{
  HANDLE_TABLE_ENTRY NewEntry;
  LONG ExHandle;
  
  PAGED_CODE();
  
  NewEntry.u1.Object = Object;
  if(ObjectType == PsThreadType)
    NewEntry.u2.GrantedAccess = CID_FLAG_THREAD;
  else if(ObjectType == PsProcessType)
    NewEntry.u2.GrantedAccess = CID_FLAG_PROCESS;
  else
  {
    DPRINT1("Can't create CID handles for %wZ objects\n", &ObjectType->TypeName);
    KEBUGCHECK(0);
  }
  
  ExHandle = ExCreateHandle(PspCidTable,
                            &NewEntry);
  if(ExHandle != EX_INVALID_HANDLE)
  {
    *Handle = EX_HANDLE_TO_HANDLE(ExHandle);
    return STATUS_SUCCESS;
  }
  
  return STATUS_UNSUCCESSFUL;
}

NTSTATUS
PsDeleteCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType)
{
  PHANDLE_TABLE_ENTRY Entry;
  LONG ExHandle = HANDLE_TO_EX_HANDLE(CidHandle);
  
  PAGED_CODE();

  Entry = ExMapHandleToPointer(PspCidTable,
                               ExHandle);
  if(Entry != NULL)
  {
    if((ObjectType == PsThreadType && ((Entry->u2.GrantedAccess & CID_FLAGS_MASK) == CID_FLAG_THREAD)) ||
       (ObjectType == PsProcessType && ((Entry->u2.GrantedAccess & CID_FLAGS_MASK) == CID_FLAG_PROCESS)))
    {
      ExDestroyHandleByEntry(PspCidTable,
                             Entry,
                             ExHandle);
      return STATUS_SUCCESS;
    }
    else
    {
      ExUnlockHandleTableEntry(PspCidTable,
                               Entry);
      return STATUS_OBJECT_TYPE_MISMATCH;
    }
  }

  return STATUS_INVALID_HANDLE;
}

PHANDLE_TABLE_ENTRY
PsLookupCidHandle(HANDLE CidHandle, POBJECT_TYPE ObjectType, PVOID *Object)
{
  PHANDLE_TABLE_ENTRY Entry;
  
  PAGED_CODE();

  KeEnterCriticalRegion();
  
  Entry = ExMapHandleToPointer(PspCidTable,
                               HANDLE_TO_EX_HANDLE(CidHandle));
  if(Entry != NULL)
  {
    if((ObjectType == PsProcessType && ((Entry->u2.GrantedAccess & CID_FLAGS_MASK) == CID_FLAG_PROCESS)) ||
       (ObjectType == PsThreadType && ((Entry->u2.GrantedAccess & CID_FLAGS_MASK) == CID_FLAG_THREAD)))
    {
      *Object = Entry->u1.Object;
      return Entry;
    }
    else
    {
      DPRINT1("CID Obj type mismatch handle 0x%x %wZ vs 0x%x\n", CidHandle,
              &ObjectType->TypeName, Entry->u2.GrantedAccess);
      ExUnlockHandleTableEntry(PspCidTable,
                               Entry);
    }
  }
  
  KeLeaveCriticalRegion();
  
  return NULL;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
PsLookupProcessThreadByCid(IN PCLIENT_ID Cid,
			   OUT PEPROCESS *Process OPTIONAL,
			   OUT PETHREAD *Thread)
{
  PHANDLE_TABLE_ENTRY CidEntry;
  PETHREAD FoundThread;

  PAGED_CODE();

  ASSERT(Thread);
  ASSERT(Cid);

  CidEntry = PsLookupCidHandle(Cid->UniqueThread, PsThreadType, (PVOID*)&FoundThread);
  if(CidEntry != NULL)
  {
    ObReferenceObject(FoundThread);

    PsUnlockCidHandle(CidEntry);

    if(Process != NULL)
    {
      *Process = FoundThread->ThreadsProcess;
    }
    *Thread = FoundThread;
    return STATUS_SUCCESS;
  }

  return STATUS_INVALID_PARAMETER;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsLookupThreadByThreadId(IN HANDLE ThreadId,
			 OUT PETHREAD *Thread)
{
  PHANDLE_TABLE_ENTRY CidEntry;
  PETHREAD FoundThread;

  PAGED_CODE();

  ASSERT(Thread);

  CidEntry = PsLookupCidHandle(ThreadId, PsThreadType, (PVOID*)&FoundThread);
  if(CidEntry != NULL)
  {
    ObReferenceObject(FoundThread);

    PsUnlockCidHandle(CidEntry);

    *Thread = FoundThread;
    return STATUS_SUCCESS;
  }

  return STATUS_INVALID_PARAMETER;
}

VOID
PsUnlockCidHandle(PHANDLE_TABLE_ENTRY CidEntry)
{
  PAGED_CODE();

  ExUnlockHandleTableEntry(PspCidTable,
                           CidEntry);
  KeLeaveCriticalRegion();
}

/* EOF */
