/*
 * subsystems/win32/csrss/csrsrv/api/process.c
 *
 * "\windows\ApiPort" port process management functions
 *
 * ReactOS Operating System
 */

/* INCLUDES ******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>
    
extern NTSTATUS CallProcessInherit(PCSR_PROCESS, PCSR_PROCESS);
extern NTSTATUS CallProcessDeleted(PCSR_PROCESS);

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

PCSR_PROCESS WINAPI CsrGetProcessData(HANDLE ProcessId)
{
    PCSR_PROCESS CsrProcess;
    NTSTATUS Status;
    
    Status = CsrLockProcessByClientId(ProcessId, &CsrProcess);
    if (!NT_SUCCESS(Status)) return NULL;
    
    UNLOCK;
    return CsrProcess;
}

PCSR_PROCESS WINAPI CsrCreateProcessData(HANDLE ProcessId)
{
    PCSR_PROCESS pProcessData;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CLIENT_ID ClientId;
    NTSTATUS Status;

    LOCK;
   
    pProcessData = CsrAllocateProcess();
    ASSERT(pProcessData != NULL);

    pProcessData->ClientId.UniqueProcess = ProcessId;

    ClientId.UniqueThread = NULL;
    ClientId.UniqueProcess = pProcessData->ClientId.UniqueProcess;
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL); 

    /* using OpenProcess is not optimal due to HANDLE vs. DWORD PIDs... */
    Status = NtOpenProcess(&pProcessData->ProcessHandle,
                           PROCESS_ALL_ACCESS,
                           &ObjectAttributes,
                           &ClientId);
    DPRINT("CSR Process: %p Handle: %p\n", pProcessData, pProcessData->ProcessHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed\n");
        CsrDeallocateProcess(pProcessData);
        CsrReleaseProcessLock();
        return NULL;
    }
    else
    {
        RtlInitializeCriticalSection(&pProcessData->HandleTableLock);
    }

    /* Set default shutdown parameters */
    pProcessData->ShutdownLevel = 0x280;
    pProcessData->ShutdownFlags = 0;
    
    /* Insert the Process */
    CsrInsertProcess(NULL, NULL, pProcessData);

    /* Release lock and return */
    CsrReleaseProcessLock();
    return pProcessData;
}

NTSTATUS WINAPI CsrFreeProcessData(HANDLE Pid)
{
    PCSR_PROCESS pProcessData;
    HANDLE Process;
    PLIST_ENTRY NextEntry;
    PCSR_THREAD Thread;

    pProcessData = CsrGetProcessData(Pid);
    if (!pProcessData) return STATUS_INVALID_PARAMETER;

    LOCK;

    Process = pProcessData->ProcessHandle;
    CallProcessDeleted(pProcessData);

    /* Dereference all process threads */
    NextEntry = pProcessData->ThreadList.Flink;
    while (NextEntry != &pProcessData->ThreadList)
    {
        Thread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);
        NextEntry = NextEntry->Flink;

        CsrThreadRefcountZero(Thread);
    }

    if (pProcessData->ClientViewBase)
    {
        NtUnmapViewOfSection(NtCurrentProcess(), (PVOID)pProcessData->ClientViewBase);
    }

    if (pProcessData->ClientPort)
    {
        NtClose(pProcessData->ClientPort);
    }

    CsrRemoveProcess(pProcessData);

    CsrDeallocateProcess(pProcessData);
    
    if (Process)
    {
      NtClose(Process);
    }
    
    return STATUS_SUCCESS;
}

/**********************************************************************
 *	CSRSS API
 *********************************************************************/

CSR_API(CsrCreateProcess)
{
   PCSR_PROCESS NewProcessData;

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
           CallProcessInherit(ProcessData, NewProcessData);
         }
     }

   if (Request->Data.CreateProcessRequest.Flags & CREATE_NEW_PROCESS_GROUP)
     {
       NewProcessData->ProcessGroupId = (DWORD)(ULONG_PTR)NewProcessData->ClientId.UniqueProcess;
     }
   else
     {
       NewProcessData->ProcessGroupId = ProcessData->ProcessGroupId;
     }

   return(STATUS_SUCCESS);
}

CSR_API(CsrSrvCreateThread)
{
    PCSR_THREAD CurrentThread;
    HANDLE ThreadHandle;
    NTSTATUS Status;
    PCSR_PROCESS CsrProcess;
    
    Request->Header.u1.s1.TotalLength = sizeof(CSR_API_MESSAGE);
    Request->Header.u1.s1.DataLength = sizeof(CSR_API_MESSAGE) - sizeof(PORT_MESSAGE);

    CurrentThread = NtCurrentTeb()->CsrClientThread;
    CsrProcess = CurrentThread->Process;

    if (CsrProcess->ClientId.UniqueProcess != Request->Data.CreateThreadRequest.ClientId.UniqueProcess)
    {
        if (Request->Data.CreateThreadRequest.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            return STATUS_SUCCESS;
        }
        
        Status = CsrLockProcessByClientId(Request->Data.CreateThreadRequest.ClientId.UniqueProcess,
                                          &CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }
    
    Status = NtDuplicateObject(CsrProcess->ProcessHandle,
                               Request->Data.CreateThreadRequest.ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (!NT_SUCCESS(Status))
    {
        Status = NtDuplicateObject(CurrentThread->Process->ProcessHandle,
                                   Request->Data.CreateThreadRequest.ThreadHandle,
                                   NtCurrentProcess(),
                                   &ThreadHandle,
                                   0,
                                   0,
                                   DUPLICATE_SAME_ACCESS);
    }

    Status = STATUS_SUCCESS; // hack
    if (NT_SUCCESS(Status))
    {
        Status = CsrCreateThread(CsrProcess,
                                     ThreadHandle,
                                     &Request->Data.CreateThreadRequest.ClientId);
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
   

   ProcessData->Flags |= CsrProcessTerminated;
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
