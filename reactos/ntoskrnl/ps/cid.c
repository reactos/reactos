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

/* FUNCTIONS *****************************************************************/

VOID
INIT_FUNCTION
PsInitClientIDManagment(VOID)
{
  PspCidTable = ExCreateHandleTable(NULL);
  ASSERT(PspCidTable);
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
  NTSTATUS Status = STATUS_INVALID_CID;

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
    Status = STATUS_SUCCESS;
  }

  return Status;
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
  NTSTATUS Status = STATUS_INVALID_CID;

  PAGED_CODE();

  ASSERT(Thread);

  CidEntry = PsLookupCidHandle(ThreadId, PsThreadType, (PVOID*)&FoundThread);
  if(CidEntry != NULL)
  {
    ObReferenceObject(FoundThread);

    PsUnlockCidHandle(CidEntry);

    *Thread = FoundThread;
    Status = STATUS_SUCCESS;
  }

  return Status;
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
