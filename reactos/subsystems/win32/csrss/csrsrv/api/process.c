/*
 * reactos/subsys/csrss/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>

#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)
#define CsrAcquireProcessLock() LOCK
#define CsrReleaseProcessLock() UNLOCK

extern NTSTATUS CallProcessInherit(PCSRSS_PROCESS_DATA, PCSRSS_PROCESS_DATA);
extern NTSTATUS CallProcessDeleted(PCSRSS_PROCESS_DATA);

/* GLOBALS *******************************************************************/

static ULONG NrProcess;
PCSRSS_PROCESS_DATA ProcessData[256];
RTL_CRITICAL_SECTION ProcessDataLock;
extern PCSRSS_PROCESS_DATA CsrRootProcess;
extern LIST_ENTRY CsrThreadHashTable[256];

/* FUNCTIONS *****************************************************************/

VOID WINAPI CsrInitProcessData(VOID)
{
    ULONG i;
   RtlZeroMemory (ProcessData, sizeof ProcessData);
   NrProcess = sizeof ProcessData / sizeof ProcessData[0];
   RtlInitializeCriticalSection( &ProcessDataLock );
   
   CsrRootProcess = CsrCreateProcessData(NtCurrentTeb()->ClientId.UniqueProcess);
   
   /* Initialize the Thread Hash List */
   for (i = 0; i < 256; i++) InitializeListHead(&CsrThreadHashTable[i]);
}

PCSRSS_PROCESS_DATA WINAPI CsrGetProcessData(HANDLE ProcessId)
{
   ULONG hash;
   PCSRSS_PROCESS_DATA pProcessData;

   hash = ((ULONG_PTR)ProcessId >> 2) % (sizeof(ProcessData) / sizeof(*ProcessData));

   LOCK;

   pProcessData = ProcessData[hash];

   while (pProcessData && pProcessData->ProcessId != ProcessId)
   {
      pProcessData = pProcessData->next;
   }
   UNLOCK;
   return pProcessData;
}

PCSRSS_PROCESS_DATA WINAPI CsrCreateProcessData(HANDLE ProcessId)
{
   ULONG hash;
   PCSRSS_PROCESS_DATA pProcessData;
   OBJECT_ATTRIBUTES ObjectAttributes;
   CLIENT_ID ClientId;
   NTSTATUS Status;

   hash = ((ULONG_PTR)ProcessId >> 2) % (sizeof(ProcessData) / sizeof(*ProcessData));

   LOCK;

   pProcessData = ProcessData[hash];

   while (pProcessData && pProcessData->ProcessId != ProcessId)
   {
      pProcessData = pProcessData->next;
   }
   if (pProcessData == NULL)
   {
      pProcessData = RtlAllocateHeap(CsrssApiHeap,
	                             HEAP_ZERO_MEMORY,
				     sizeof(CSRSS_PROCESS_DATA));
      if (pProcessData)
      {
	 pProcessData->ProcessId = ProcessId;
	 pProcessData->next = ProcessData[hash];
	 ProcessData[hash] = pProcessData;

         ClientId.UniqueThread = NULL;
         ClientId.UniqueProcess = pProcessData->ProcessId;
         InitializeObjectAttributes(&ObjectAttributes,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL);

         /* using OpenProcess is not optimal due to HANDLE vs. DWORD PIDs... */
         Status = NtOpenProcess(&pProcessData->Process,
                                PROCESS_ALL_ACCESS,
                                &ObjectAttributes,
                                &ClientId);
         if (!NT_SUCCESS(Status))
         {
            ProcessData[hash] = pProcessData->next;
	    RtlFreeHeap(CsrssApiHeap, 0, pProcessData);
	    pProcessData = NULL;
         }
         else
         {
            RtlInitializeCriticalSection(&pProcessData->HandleTableLock);
         }
      }
   }
   else
   {
      DPRINT1("Process data for pid %d already exist\n", ProcessId);
   }
   UNLOCK;
   if (pProcessData == NULL)
   {
      DPRINT1("CsrCreateProcessData() failed\n");
   }
   else
   {
      pProcessData->Terminated = FALSE;

      /* Set default shutdown parameters */
      pProcessData->ShutdownLevel = 0x280;
      pProcessData->ShutdownFlags = 0;
   }
   
   pProcessData->ThreadCount = 0;
   InitializeListHead(&pProcessData->ThreadList);
   return pProcessData;
}

NTSTATUS WINAPI CsrFreeProcessData(HANDLE Pid)
{
  ULONG hash;
  PCSRSS_PROCESS_DATA pProcessData, *pPrevLink;
  HANDLE Process;

  hash = ((ULONG_PTR)Pid >> 2) % (sizeof(ProcessData) / sizeof(*ProcessData));
  pPrevLink = &ProcessData[hash];

  LOCK;

  while ((pProcessData = *pPrevLink) && pProcessData->ProcessId != Pid)
    {
      pPrevLink = &pProcessData->next;
    }

  if (pProcessData)
    {
      DPRINT("CsrFreeProcessData pid: %d\n", Pid);
      Process = pProcessData->Process;
      CallProcessDeleted(pProcessData);
      if (pProcessData->CsrSectionViewBase)
        {
          NtUnmapViewOfSection(NtCurrentProcess(), pProcessData->CsrSectionViewBase);
        }
      if (pProcessData->ServerCommunicationPort)
        {
          NtClose(pProcessData->ServerCommunicationPort);
        }
      *pPrevLink = pProcessData->next;

      RtlFreeHeap(CsrssApiHeap, 0, pProcessData);
      UNLOCK;
      if (Process)
        {
          NtClose(Process);
        }
      return STATUS_SUCCESS;
   }

   UNLOCK;
   return STATUS_INVALID_PARAMETER;
}

/**********************************************************************
 *	CSRSS API
 *********************************************************************/

CSR_API(CsrCreateProcess)
{
   PCSRSS_PROCESS_DATA NewProcessData;
   NTSTATUS Status;

   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);

   NewProcessData = CsrCreateProcessData(Request->Data.CreateProcessRequest.NewProcessId);
   if (NewProcessData == NULL)
     {
	return(STATUS_NO_MEMORY);
     }

   if (!(Request->Data.CreateProcessRequest.Flags & (CREATE_NEW_CONSOLE|DETACHED_PROCESS)))
     {
       NewProcessData->ParentConsole = ProcessData->Console;
       NewProcessData->bInheritHandles = Request->Data.CreateProcessRequest.bInheritHandles;
       if (Request->Data.CreateProcessRequest.bInheritHandles)
         {
           Status = CallProcessInherit(ProcessData, NewProcessData);
         }
     }

   if (Request->Data.CreateProcessRequest.Flags & CREATE_NEW_PROCESS_GROUP)
     {
       NewProcessData->ProcessGroup = (DWORD)(ULONG_PTR)NewProcessData->ProcessId;
     }
   else
     {
       NewProcessData->ProcessGroup = ProcessData->ProcessGroup;
     }

   return(STATUS_SUCCESS);
}

CSR_API(CsrSrvCreateThread)
{
    PCSR_THREAD CurrentThread;
    HANDLE ThreadHandle;
    NTSTATUS Status;
    PCSRSS_PROCESS_DATA CsrProcess;
    
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    CurrentThread = NtCurrentTeb()->CsrClientThread;
    CsrProcess = CurrentThread->Process;
//    DPRINT1("Current thread: %p %p\n", CurrentThread, CsrProcess);
//    DPRINT1("Request CID: %lx %lx %lx\n", 
//            CsrProcess->ProcessId, 
//            NtCurrentTeb()->ClientId.UniqueProcess,
 //           Request->Data.CreateThreadRequest.ClientId.UniqueProcess);
    
    if (CsrProcess->ProcessId != Request->Data.CreateThreadRequest.ClientId.UniqueProcess)
    {
        if (Request->Data.CreateThreadRequest.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            return STATUS_SUCCESS;
        }
        
        Status = CsrLockProcessByClientId(Request->Data.CreateThreadRequest.ClientId.UniqueProcess,
                                          &CsrProcess);
  //      DPRINT1("Found matching process: %p\n", CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }
    
//    DPRINT1("PIDs: %lx %lx\n", CurrentThread->Process->ProcessId, CsrProcess->ProcessId);
//    DPRINT1("Thread handle is: %lx Process Handle is: %lx %lx\n",
       //     Request->Data.CreateThreadRequest.ThreadHandle,
     //       CurrentThread->Process->Process,
   //         CsrProcess->Process);
    Status = NtDuplicateObject(CsrProcess->Process,
                               Request->Data.CreateThreadRequest.ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    //DPRINT1("Duplicate status: %lx\n", Status);
    if (!NT_SUCCESS(Status))
    {
        Status = NtDuplicateObject(CurrentThread->Process->Process,
                                   Request->Data.CreateThreadRequest.ThreadHandle,
                                   NtCurrentProcess(),
                                   &ThreadHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS);
       // DPRINT1("Duplicate status: %lx\n", Status);
    }

    Status = STATUS_SUCCESS; // hack
    if (NT_SUCCESS(Status))
    {
        Status = CsrCreateThread(CsrProcess,
                                     ThreadHandle,
                                     &Request->Data.CreateThreadRequest.ClientId);
       // DPRINT1("Create status: %lx\n", Status);
    }

    if (CsrProcess != CurrentThread->Process) CsrReleaseProcessLock();
    
    return Status;
}

CSR_API(CsrTerminateProcess)
{
   PLIST_ENTRY NextEntry;
   PCSR_THREAD Thread;
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);
   
   NextEntry = ProcessData->ThreadList.Flink;
   while (NextEntry != &ProcessData->ThreadList)
   {
        Thread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);
        NextEntry = NextEntry->Flink;
        
        CsrThreadRefcountZero(Thread);
        
   }
   

   ProcessData->Terminated = TRUE;
   return STATUS_SUCCESS;
}

CSR_API(CsrConnectProcess)
{
   Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
   Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

   return(STATUS_SUCCESS);
}

CSR_API(CsrGetShutdownParameters)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  Request->Data.GetShutdownParametersRequest.Level = ProcessData->ShutdownLevel;
  Request->Data.GetShutdownParametersRequest.Flags = ProcessData->ShutdownFlags;

  return(STATUS_SUCCESS);
}

CSR_API(CsrSetShutdownParameters)
{
  Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
  Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

  ProcessData->ShutdownLevel = Request->Data.SetShutdownParametersRequest.Level;
  ProcessData->ShutdownFlags = Request->Data.SetShutdownParametersRequest.Flags;

  return(STATUS_SUCCESS);
}

/* EOF */
