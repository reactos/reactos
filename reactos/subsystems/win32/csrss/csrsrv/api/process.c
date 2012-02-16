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

#define CSR_SERVER_DLL_MAX 4
#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)
#define CsrAcquireProcessLock() LOCK
#define CsrReleaseProcessLock() UNLOCK
#define ProcessStructureListLocked() \
    (ProcessDataLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread)
    
extern NTSTATUS CallProcessInherit(PCSR_PROCESS, PCSR_PROCESS);
extern NTSTATUS CallProcessDeleted(PCSR_PROCESS);

/* GLOBALS *******************************************************************/

RTL_CRITICAL_SECTION ProcessDataLock, CsrWaitListsLock;
extern PCSR_PROCESS CsrRootProcess;
extern LIST_ENTRY CsrThreadHashTable[256];
extern ULONG CsrTotalPerProcessDataLength;
LONG CsrProcessSequenceCount = 5;

/* FUNCTIONS *****************************************************************/

/*++
 * @name CsrAllocateProcess
 * @implemented NT4
 *
 * The CsrAllocateProcess routine allocates a new CSR Process object.
 *
 * @return Pointer to the newly allocated CSR Process.
 *
 * @remarks None.
 *
 *--*/
PCSR_PROCESS
NTAPI
CsrAllocateProcess(VOID)
{
    PCSR_PROCESS CsrProcess;
    ULONG TotalSize;

    /* Calculate the amount of memory this should take */
    TotalSize = sizeof(CSR_PROCESS) +
                (CSR_SERVER_DLL_MAX * sizeof(PVOID)) +
                CsrTotalPerProcessDataLength;

    /* Allocate a Process */
    CsrProcess = RtlAllocateHeap(CsrHeap, HEAP_ZERO_MEMORY, TotalSize);
    if (!CsrProcess) return NULL;

    /* Handle the Sequence Number and protect against overflow */
    CsrProcess->SequenceNumber = CsrProcessSequenceCount++;
    if (CsrProcessSequenceCount < 5) CsrProcessSequenceCount = 5;

    /* Increase the reference count */
    CsrProcess->ReferenceCount++;

    /* Initialize the Thread List */
    InitializeListHead(&CsrProcess->ThreadList);

    /* Return the Process */
    return CsrProcess;
}

/*++
 * @name CsrLockedReferenceProcess
 *
 * The CsrLockedReferenceProcess refences a CSR Process while the
 * Process Lock is already being held.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process to be referenced.
 *
 * @return None.
 *
 * @remarks This routine will return with the Process Lock held.
 *
 *--*/
VOID
NTAPI
CsrLockedReferenceProcess(IN PCSR_PROCESS CsrProcess)
{
    /* Increment the reference count */
    ++CsrProcess->ReferenceCount;
}

/*++
 * @name CsrServerInitialization
 * @implemented NT4
 *
 * The CsrInitializeProcessStructure routine sets up support for CSR Processes
 * and CSR Threads.
 *
 * @param None.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrInitializeProcessStructure(VOID)
{
    NTSTATUS Status;
    ULONG i;

    /* Initialize the Lock */
    Status = RtlInitializeCriticalSection(&ProcessDataLock);
    if (!NT_SUCCESS(Status)) return Status;

    /* Set up the Root Process */
    CsrRootProcess = CsrAllocateProcess();
    if (!CsrRootProcess) return STATUS_NO_MEMORY;

    /* Set up the minimal information for it */
    InitializeListHead(&CsrRootProcess->ListLink);
    CsrRootProcess->ProcessHandle = (HANDLE)-1;
    CsrRootProcess->ClientId = NtCurrentTeb()->ClientId;

    /* Initialize the Thread Hash List */
    for (i = 0; i < 256; i++) InitializeListHead(&CsrThreadHashTable[i]);

    /* Initialize the Wait Lock */
    return RtlInitializeCriticalSection(&CsrWaitListsLock);
}

/*++
 * @name CsrDeallocateProcess
 *
 * The CsrDeallocateProcess frees the memory associated with a CSR Process.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process to be freed.
 *
 * @return None.
 *
 * @remarks Do not call this routine. It is reserved for the internal
 *          thread management routines when a CSR Process has been cleanly
 *          dereferenced and killed.
 *
 *--*/
VOID
NTAPI
CsrDeallocateProcess(IN PCSR_PROCESS CsrProcess)
{
    /* Free the process object from the heap */
    RtlFreeHeap(CsrHeap, 0, CsrProcess);
}

/*++
 * @name CsrRemoveProcess
 *
 * The CsrRemoveProcess function undoes a CsrInsertProcess operation and
 * removes the CSR Process from the Process List and notifies Server DLLs
 * of this removal.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process to remove.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrRemoveProcess(IN PCSR_PROCESS CsrProcess)
{
#if 0
    PCSR_SERVER_DLL ServerDll;
    ULONG i;
#endif
    ASSERT(ProcessStructureListLocked());

    /* Remove us from the Process List */
    RemoveEntryList(&CsrProcess->ListLink);

    /* Release the lock */
    CsrReleaseProcessLock();
#if 0
    /* Loop every Server DLL */
    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Get the Server DLL */
        ServerDll = CsrLoadedServerDll[i];

        /* Check if it's valid and if it has a Disconnect Callback */
        if (ServerDll && ServerDll->DisconnectCallback)
        {
            /* Call it */
            (ServerDll->DisconnectCallback)(CsrProcess);
        }
    }
#endif
}

/*++
 * @name CsrInsertProcess
 *
 * The CsrInsertProcess routine inserts a CSR Process into the Process List
 * and notifies Server DLLs of the creation of a new CSR Process.
 *
 * @param Parent
 *        Optional pointer to the CSR Process creating this CSR Process.
 *
 * @param CurrentProcess
 *        Optional pointer to the current CSR Process.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process which is to be inserted.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrInsertProcess(IN PCSR_PROCESS Parent OPTIONAL,
                 IN PCSR_PROCESS CurrentProcess OPTIONAL,
                 IN PCSR_PROCESS CsrProcess)
{
#if 0
    PCSR_SERVER_DLL ServerDll;
    ULONG i;
#endif
    ASSERT(ProcessStructureListLocked());

    /* Set the parent */
    CsrProcess->Parent = Parent;

    /* Insert it into the Root List */
    InsertTailList(&CsrRootProcess->ListLink, &CsrProcess->ListLink);
#if 0
    /* Notify the Server DLLs */
    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Get the current Server DLL */
        ServerDll = CsrLoadedServerDll[i];

        /* Make sure it's valid and that it has callback */
        if ((ServerDll) && (ServerDll->NewProcessCallback))
        {
            ServerDll->NewProcessCallback(CurrentProcess, CsrProcess);
        }
    }
#endif
}

/*++
 * @name CsrLockProcessByClientId
 * @implemented NT4
 *
 * The CsrLockProcessByClientId routine locks the CSR Process corresponding
 * to the given Process ID and optionally returns it.
 *
 * @param Pid
 *        Process ID corresponding to the CSR Process which will be locked.
 *
 * @param CsrProcess
 *        Optional pointer to a CSR Process pointer which will hold the
 *        CSR Process corresponding to the given Process ID.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks Locking a CSR Process is defined as acquiring an extra
 *          reference to it and returning with the Process Lock held.
 *
 *--*/
NTSTATUS
NTAPI
CsrLockProcessByClientId(IN HANDLE Pid,
                         OUT PCSR_PROCESS *CsrProcess)
{
    PLIST_ENTRY NextEntry;
    PCSR_PROCESS CurrentProcess = NULL;

    /* Acquire the lock */
    CsrAcquireProcessLock();

    /* Assume failure */
    ASSERT(CsrProcess != NULL);
    *CsrProcess = NULL;

    /* Setup the List Pointers */
    NextEntry = CsrRootProcess->ListLink.Flink;
    while (NextEntry != &CsrRootProcess->ListLink)
    {
        /* Get the Process */
        CurrentProcess = CONTAINING_RECORD(NextEntry, CSR_PROCESS, ListLink);

        /* Check for PID Match */
        if (CurrentProcess->ClientId.UniqueProcess == Pid) break;

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Check if we didn't find it in the list */
    if (NextEntry == &CsrRootProcess->ListLink)
    {
        /* Nothing found, release the lock */
        CsrReleaseProcessLock();
        return STATUS_UNSUCCESSFUL;
    }

    /* Lock the found process and return it */
    CsrLockedReferenceProcess(CurrentProcess);
    *CsrProcess = CurrentProcess;
    return STATUS_SUCCESS;
}

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
