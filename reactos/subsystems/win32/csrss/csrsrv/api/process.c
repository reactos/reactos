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
    
extern NTSTATUS CallProcessCreated(PCSR_PROCESS, PCSR_PROCESS);

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

    /* Dereference all process threads */
    NextEntry = pProcessData->ThreadList.Flink;
    while (NextEntry != &pProcessData->ThreadList)
    {
        Thread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);
        NextEntry = NextEntry->Flink;

        ASSERT(ProcessStructureListLocked());
        CsrThreadRefcountZero(Thread);
        LOCK;
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

   NewProcessData = CsrCreateProcessData(Request->Data.CreateProcessRequest.NewProcessId);
   if (NewProcessData == NULL)
     {
	return(STATUS_NO_MEMORY);
     }

   if (!(Request->Data.CreateProcessRequest.Flags & (CREATE_NEW_CONSOLE|DETACHED_PROCESS)))
     {
       NewProcessData->ParentConsole = ProcessData->Console;
       NewProcessData->bInheritHandles = Request->Data.CreateProcessRequest.bInheritHandles;
     }
     
     CallProcessCreated(ProcessData, NewProcessData);

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
    
    /* Get the current CSR thread */
    CurrentThread = NtCurrentTeb()->CsrClientThread;
    if (!CurrentThread) return STATUS_SUCCESS; // server-to-server
    
    /* Get the CSR Process for this request */
    CsrProcess = CurrentThread->Process;
    if (CsrProcess->ClientId.UniqueProcess !=
        Request->Data.CreateThreadRequest.ClientId.UniqueProcess)
    {
        /* This is a remote thread request -- is it within the server itself? */
        if (Request->Data.CreateThreadRequest.ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess)
        {
            /* Accept this without any further work */
            return STATUS_SUCCESS;
        }
        
        /* Get the real CSR Process for the remote thread's process */
        Status = CsrLockProcessByClientId(Request->Data.CreateThreadRequest.ClientId.UniqueProcess,
                                          &CsrProcess);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Duplicate the thread handle so we can own it */
    Status = NtDuplicateObject(CurrentThread->Process->ProcessHandle,
                               Request->Data.CreateThreadRequest.ThreadHandle,
                               NtCurrentProcess(),
                               &ThreadHandle,
                               0,
                               0,
                               DUPLICATE_SAME_ACCESS);
    if (NT_SUCCESS(Status))
    {
        /* Call CSRSRV to tell it about the new thread */
        Status = CsrCreateThread(CsrProcess,
                                 ThreadHandle,
                                 &Request->Data.CreateThreadRequest.ClientId);
    }

    /* Unlock the process and return */
    if (CsrProcess != CurrentThread->Process) CsrUnlockProcess(CsrProcess);
    return Status;
}

CSR_API(CsrTerminateProcess)
{
   PLIST_ENTRY NextEntry;
   PCSR_THREAD Thread;
   
   LOCK;
   
   NextEntry = ProcessData->ThreadList.Flink;
   while (NextEntry != &ProcessData->ThreadList)
   {
        Thread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);
        NextEntry = NextEntry->Flink;
        
        ASSERT(ProcessStructureListLocked());
        CsrThreadRefcountZero(Thread);
        LOCK;
        
   }
   
   UNLOCK;
   ProcessData->Flags |= CsrProcessTerminated;
   return STATUS_SUCCESS;
}

CSR_API(CsrConnectProcess)
{

   return(STATUS_SUCCESS);
}

CSR_API(CsrGetShutdownParameters)
{

  Request->Data.GetShutdownParametersRequest.Level = ProcessData->ShutdownLevel;
  Request->Data.GetShutdownParametersRequest.Flags = ProcessData->ShutdownFlags;

  return(STATUS_SUCCESS);
}

CSR_API(CsrSetShutdownParameters)
{

  ProcessData->ShutdownLevel = Request->Data.SetShutdownParametersRequest.Level;
  ProcessData->ShutdownFlags = Request->Data.SetShutdownParametersRequest.Flags;

  return(STATUS_SUCCESS);
}

/* EOF */
