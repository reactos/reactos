/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsys/csr/csrsrv/process.c
 * PURPOSE:         CSR Server DLL Process Implementation
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include "srv.h"

#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

PCSR_PROCESS CsrRootProcess = NULL;
RTL_CRITICAL_SECTION CsrProcessLock;
ULONG CsrProcessSequenceCount = 5;
ULONG CsrTotalPerProcessDataLength;

/* PRIVATE FUNCTIONS *********************************************************/

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

    /* Handle the Sequence Number */
    CsrProcess->SequenceNumber = CsrProcessSequenceCount++;

    /* Increase the reference count */
    CsrProcess->ReferenceCount++;

    /* Initialize the Thread List */
    InitializeListHead(&CsrProcess->ThreadList);

    /* Return the Process */
    return CsrProcess;
}

/*++
 * @name CsrServerInitialization
 * @implemented NT4
 *
 * The CsrInitializeProcesses routine sets up support for CSR Processes
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
CsrInitializeProcesses(VOID)
{
    NTSTATUS Status;
    ULONG i;

    /* Initialize the Lock */
    Status = RtlInitializeCriticalSection(&CsrProcessLock);

    /* Set up the Root Process */
    CsrRootProcess = CsrAllocateProcess();
    if (!CsrRootProcess) return STATUS_NO_MEMORY;

    /* Set up the minimal information for it */
    InitializeListHead(&CsrRootProcess->ListLink);
    CsrRootProcess->ProcessHandle = (HANDLE)-1;
    CsrRootProcess->ClientId = NtCurrentTeb()->Cid;

    /* Initialize the Thread Hash List */
    for (i = 0; i < 256; i++) InitializeListHead(&CsrThreadHashTable[i]);

    /* Initialize the Wait Lock */
    Status = RtlInitializeCriticalSection(&CsrWaitListsLock);
    return Status;
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
    PCSR_SERVER_DLL ServerDll;
    ULONG i;

    /* Set the parent */
    CsrProcess->Parent = Parent;

    /* Insert it into the Root List */
    InsertTailList(&CsrRootProcess->ListLink, &CsrProcess->ListLink);

    /* Notify the Server DLLs */
    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Get the current Server DLL */
        ServerDll = CsrLoadedServerDll[i];

        /* Make sure it's valid and that it has callback */
        if (ServerDll && ServerDll->NewProcessCallback)
        {
            (*ServerDll->NewProcessCallback)(CurrentProcess, CsrProcess);
        }
    }
}

/*++
 * @name CsrLockedDereferenceProcess
 *
 * The CsrLockedDereferenceProcess dereferences a CSR Process while the
 * Process Lock is already being held.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process to be dereferenced.
 *
 * @return None.
 *
 * @remarks This routine will return with the Process Lock held.
 *
 *--*/
VOID
NTAPI
CsrLockedDereferenceProcess(PCSR_PROCESS CsrProcess)
{
    /* Decrease reference count */
    if (!(--CsrProcess->ReferenceCount))
    {
        /* Call the generic cleanup code */
        CsrAcquireProcessLock();
        CsrProcessRefcountZero(CsrProcess);
    }
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
    PCSR_SERVER_DLL ServerDll;
    ULONG i;

    /* Remove us from the Process List */
    RemoveEntryList(&CsrProcess->ListLink);

    /* Release the lock */
    CsrReleaseProcessLock();

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
}

/*++
 * @name CsrProcessRefcountZero
 *
 * The CsrProcessRefcountZero routine is executed when a CSR Process has lost
 * all its active references. It removes and de-allocates the CSR Process.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process that is to be deleted.
 *
 * @return None.
 *
 * @remarks Do not call this routine. It is reserved for the internal
 *          thread management routines when a CSR Process has lost all
 *          its references.
 *
 *          This routine is called with the Process Lock held.
 *
 *--*/
VOID
NTAPI
CsrProcessRefcountZero(IN PCSR_PROCESS CsrProcess)
{
    /* Remove the Process from the list */
    CsrRemoveProcess(CsrProcess);

    /* Check if there's a session */
    if (CsrProcess->NtSession)
    {
        /* Dereference the Session */
        CsrDereferenceNtSession(CsrProcess->NtSession, 0);
    }

    /* Close the Client Port if there is one */
    if (CsrProcess->ClientPort ) NtClose(CsrProcess->ClientPort);

    /* Close the process handle */
    NtClose(CsrProcess->ProcessHandle);

    /* Free the Proces Object */
    CsrDeallocateProcess(CsrProcess);
}

/*++
 * @name CsrSetToNormalPriority
 *
 * The CsrSetToNormalPriority routine sets the current NT Process'
 * priority to the normal priority for CSR Processes.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks The "Normal" Priority corresponds to the Normal Forground
 *          Priority (9) plus a boost of 4.
 *
 *--*/
VOID
NTAPI
CsrSetToNormalPriority(VOID)
{
    KPRIORITY BasePriority = (8 + 1) + 4;

    /* Set the Priority */
    NtSetInformationProcess(NtCurrentProcess(),
                            ProcessBasePriority,
                            &BasePriority,
                            sizeof(KPRIORITY));
}

/*++
 * @name CsrSetToShutdownPriority
 *
 * The CsrSetToShutdownPriority routine sets the current NT Process'
 * priority to the boosted priority for CSR Processes doing shutdown.
 * Additonally, it acquires the Shutdown Privilege required for shutdown.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks The "Shutdown" Priority corresponds to the Normal Forground
 *          Priority (9) plus a boost of 6.
 *
 *--*/
VOID
NTAPI
CsrSetToShutdownPriority(VOID)
{
    KPRIORITY SetBasePriority = (8 + 1) + 6;
    BOOLEAN Old;

    /* Get the shutdown privilege */
    if (NT_SUCCESS(RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                      TRUE,
                                      FALSE,
                                      &Old)))
    {
        /* Set the Priority */
        NtSetInformationProcess(NtCurrentProcess(),
                                ProcessBasePriority,
                                &SetBasePriority,
                                sizeof(KPRIORITY));
    }
}

/*++
 * @name FindProcessForShutdown
 *
 * The FindProcessForShutdown routine returns a CSR Process which is ready
 * to be shutdown, and sets the appropriate shutdown flags for it.
 *
 * @param CallerLuid
 *        Pointer to the LUID of the CSR Process calling this routine.
 *
 * @return Pointer to a CSR Process which is ready to be shutdown.
 *
 * @remarks None.
 *
 *--*/
PCSR_PROCESS
NTAPI
FindProcessForShutdown(PLUID CallerLuid)
{
    PLIST_ENTRY ListHead, NextEntry;
    LUID ProcessLuid;
    NTSTATUS Status;
    LUID SystemLuid = SYSTEM_LUID;
    PCSR_PROCESS CsrProcess;
    PCSR_THREAD CsrThread;
    BOOLEAN IsSystemLuid = FALSE, IsOurLuid = FALSE;
    PCSR_PROCESS ReturnCsrProcess = NULL;
    ULONG Level = 0;

    /* Set the List Pointers */
    ListHead = &CsrRootProcess->ListLink;
    NextEntry = ListHead->Flink;

    /* Start looping */
    while (NextEntry != ListHead)
    {
        /* Get the process */
        CsrProcess = CONTAINING_RECORD(NextEntry, CSR_PROCESS, ListLink);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;

        /* Skip this process if it's already been processed*/
        if (CsrProcess->Flags & CsrProcessSkipShutdown) continue;

        /* Get the LUID of this Process */
        Status = CsrGetProcessLuid(CsrProcess->ProcessHandle, &ProcessLuid);

        /* Check if we didn't get access to the LUID */
        if (Status == STATUS_ACCESS_DENIED)
        {
            /* Check if we have any threads */
            if (CsrProcess->ThreadCount)
            {
                /* Impersonate one of the threads and retry */
                CsrThread = CONTAINING_RECORD(CsrProcess->ThreadList.Flink,
                                              CSR_THREAD,
                                              Link);
                CsrImpersonateClient(CsrThread);
                Status = CsrGetProcessLuid(NULL, &ProcessLuid);
                CsrRevertToSelf();
            }
        }

        if (!NT_SUCCESS(Status))
        {
            /* We didn't have access, so skip it */
            CsrProcess->Flags |= CsrProcessSkipShutdown;
            continue;
        }

        /* Check if this is the System LUID */
        if ((IsSystemLuid = RtlEqualLuid(&ProcessLuid, &SystemLuid)))
        {
            /* Mark this process */
            CsrProcess->ShutdownFlags |= CsrShutdownSystem;
        }
        else if (!(IsOurLuid = RtlEqualLuid(&ProcessLuid, CallerLuid)))
        {
            /* Our LUID doesn't match with the caller's */
            CsrProcess->ShutdownFlags |= CsrShutdownOther;
        }

        /* Check if we're past the previous level */
        if (CsrProcess->ShutdownLevel > Level)
        {
            /* Update the level */
            Level = CsrProcess->ShutdownLevel;

            /* Set the final process */
            ReturnCsrProcess = CsrProcess;
        }
    }

    /* Check if we found a process */
    if (ReturnCsrProcess)
    {
        /* Skip this one next time */
        ReturnCsrProcess->Flags |= CsrProcessSkipShutdown;
    }

    return ReturnCsrProcess;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*++
 * @name CsrCreateProcess
 * @implemented NT4
 *
 * Do nothing for 500ms.
 *
 * @param ArgumentCount
 *        Description of the parameter. Wrapped to more lines on ~70th
 *        column.
 *
 * @param Arguments
 *        Description of the parameter. Wrapped to more lines on ~70th
 *        column.     
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrCreateProcess(IN HANDLE hProcess,
                 IN HANDLE hThread,
                 IN PCLIENT_ID ClientId,
                 IN PCSR_NT_SESSION NtSession,
                 IN ULONG Flags,
                 IN PCLIENT_ID DebugCid)
{
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;
    CLIENT_ID CurrentCid;
    PCSR_PROCESS CurrentProcess;
    PVOID ProcessData;
    ULONG i;
    PCSR_PROCESS CsrProcess;
    NTSTATUS Status;
    PCSR_THREAD CsrThread;
    KERNEL_USER_TIMES KernelTimes;

    /* Get the current CID and lock Processes */
    CurrentCid = CurrentThread->ClientId;
    CsrAcquireProcessLock();

    /* Get the current CSR Thread */
    CurrentThread = CsrLocateThreadByClientId(&CurrentProcess, &CurrentCid);
    if (!CurrentThread)
    {
        /* We've failed to locate the thread */
        CsrReleaseProcessLock();
        return STATUS_THREAD_IS_TERMINATING;
    }

    /* Allocate a new Process Object */
    if (!(CsrProcess = CsrAllocateProcess()))
    {
        /* Couldn't allocate Process */
        CsrReleaseProcessLock();
        return STATUS_NO_MEMORY;
    }

    /* Setup Process Data */
    CsrProcess->ClientId = *ClientId;
    CsrProcess->ProcessHandle = hProcess;
    CsrProcess->ShutdownLevel = 0x280;

    /* Inherit the Process Data */
    ProcessData = &CurrentProcess->ServerData[CSR_SERVER_DLL_MAX];
    for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
    {
        /* Check if the DLL is Loaded and has Per Process Data */
        if (CsrLoadedServerDll[i] && CsrLoadedServerDll[i]->SizeOfProcessData)
        {
            /* Set the pointer */
            CsrProcess->ServerData[i] = ProcessData;

            /* Copy the Data */
            RtlMoveMemory(ProcessData,
                          CurrentProcess->ServerData[i],
                          CsrLoadedServerDll[i]->SizeOfProcessData);

            /* Update next data pointer */
            ProcessData = (PVOID)((ULONG_PTR)ProcessData +
                                  CsrLoadedServerDll[i]->SizeOfProcessData);
        }
        else 
        {
            /* No data for this Server */
            CsrProcess->ServerData[i] = NULL;
        }
    }

    /* Set the Exception port to us */
    Status = NtSetInformationProcess(hProcess,
                                     ProcessExceptionPort,
                                     &CsrApiPort,
                                     sizeof(HANDLE));
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();
        return STATUS_NO_MEMORY;
    }

    /* If Check if CreateProcess got CREATE_NEW_PROCESS_GROUP */
    if (!(Flags & CsrProcessCreateNewGroup))
    {
        /* Create new data */
        CsrProcess->ProcessGroupId = HandleToUlong(ClientId->UniqueProcess);
        CsrProcess->ProcessGroupSequence = CsrProcess->SequenceNumber;
    }
    else
    {
        /* Copy it from the current process */
        CsrProcess->ProcessGroupId = CurrentProcess->ProcessGroupId;
        CsrProcess->ProcessGroupSequence = CurrentProcess->ProcessGroupSequence;
    }

    /* Check if this is a console process */
    if(Flags & CsrProcessIsConsoleApp) CsrProcess->Flags |= CsrProcessIsConsoleApp;

    /* Mask out non-debug flags */
    Flags &= ~(CsrProcessIsConsoleApp | CsrProcessCreateNewGroup);

    /* Check if every process will be debugged */
    if (!Flags && CurrentProcess->DebugFlags & CsrDebugProcessChildren)
    {
        /* Pass it on to the current process */
        CsrProcess->DebugFlags = CsrDebugProcessChildren;
        CsrProcess->DebugCid = CurrentProcess->DebugCid;
    }

    /* Check if Debugging was used on this process */
    if (Flags & (CsrDebugOnlyThisProcess | CsrDebugProcessChildren))
    {
        /* Save the debug flag used */
        CsrProcess->DebugFlags = Flags;

        /* Save the CID */
        if (DebugCid) CsrProcess->DebugCid = *DebugCid;
    }

    /* Check if we debugging is enabled */
    if (CsrProcess->DebugFlags)
    {
        /* Set the Debug Port to us */
        Status = NtSetInformationProcess(hProcess,
                                         ProcessDebugPort,
                                         &CsrApiPort,
                                         sizeof(HANDLE));
        if (!NT_SUCCESS(Status))
        {
            /* Failed */
            CsrDeallocateProcess(CsrProcess);
            CsrReleaseProcessLock();
            return STATUS_NO_MEMORY;
        }
    }

    /* Get the Thread Create Time */
    Status = NtQueryInformationThread(hThread,
                                      ThreadTimes,
                                      (PVOID)&KernelTimes,
                                      sizeof(KernelTimes),
                                      NULL);

    /* Allocate a CSR Thread Structure */
    CsrThread = CsrAllocateThread(CsrProcess);
    if (CsrThread == NULL)
    {
        /* Failed */
        CsrDeallocateProcess(CsrProcess);
        CsrReleaseProcessLock();
        return STATUS_NO_MEMORY;
    }

    /* Save the data we have */
    CsrThread->CreateTime = KernelTimes.CreateTime;
    CsrThread->ClientId = *ClientId;
    CsrThread->ThreadHandle = hThread;
    CsrThread->Flags = 0;

    /* Insert the Thread into the Process */
    CsrInsertThread(CsrProcess, CsrThread);

    /* Reference the session */
    CsrReferenceNtSession(NtSession);
    CsrProcess->NtSession = NtSession;

    /* Set the Priority to Background */
    CsrSetBackgroundPriority(CsrProcess);

    /* Insert the Process */
    CsrInsertProcess(NULL, CurrentProcess, CsrProcess);

    /* Release lock and return */
    CsrReleaseProcessLock();
    return Status;
}

/*++
 * @name CsrDebugProcess
 * @implemented NT4
 *
 * The CsrDebugProcess routine is deprecated in NT 5.1 and higher. It is
 * exported only for compatibility with older CSR Server DLLs.
 *
 * @param CsrProcess
 *        Deprecated.  
 *
 * @return Deprecated
 *
 * @remarks Deprecated.
 *
 *--*/
NTSTATUS
NTAPI
CsrDebugProcess(PCSR_PROCESS CsrProcess)
{
    /* CSR does not handle debugging anymore */
    DPRINT("CSRSRV: %s(%08lx) called\n", __FUNCTION__, CsrProcess);
    return STATUS_UNSUCCESSFUL;
}

/*++
 * @name CsrServerInitialization
 * @implemented NT4
 *
 * The CsrDebugProcessStop routine is deprecated in NT 5.1 and higher. It is
 * exported only for compatibility with older CSR Server DLLs.
 *
 * @param CsrProcess
 *        Deprecated.  
 *
 * @return Deprecated
 *
 * @remarks Deprecated.
 *
 *--*/
NTSTATUS
NTAPI
CsrDebugProcessStop(PCSR_PROCESS CsrProcess)
{
    /* CSR does not handle debugging anymore */
    DPRINT("CSRSRV: %s(%08lx) called\n", __FUNCTION__, CsrProcess);
    return STATUS_UNSUCCESSFUL;
}

/*++
 * @name CsrDereferenceProcess
 * @implemented NT4
 *
 * The CsrDereferenceProcess routine removes a reference from a CSR Process.
 *
 * @param CsrThread
 *        Pointer to the CSR Process to dereference.
 *
 * @return None.
 *
 * @remarks If the reference count has reached zero (ie: the CSR Process has
 *          no more active references), it will be deleted.
 *
 *--*/
VOID
NTAPI
CsrDereferenceProcess(PCSR_PROCESS CsrProcess)
{
    /* Acquire process lock */
    CsrAcquireProcessLock();

    /* Decrease reference count */
    if (!(--CsrProcess->ReferenceCount))
    {
        /* Call the generic cleanup code */
        CsrProcessRefcountZero(CsrProcess);
    }
    else
    {
        /* Just release the lock */
        CsrReleaseProcessLock();
    }
}

/*++
 * @name CsrDestroyProcess
 * @implemented NT4
 *
 * The CsrDestroyProcess routine destroys the CSR Process corresponding to 
 * a given Client ID.
 *
 * @param Cid
 *        Pointer to the Client ID Structure corresponding to the CSR
 *        Process which is about to be destroyed. 
 *
 * @param ExitStatus
 *        Unused.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_THREAD_IS_TERMINATING
 *         if the CSR Process is already terminating.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrDestroyProcess(IN PCLIENT_ID Cid,
                  IN NTSTATUS ExitStatus)
{
    PCSR_THREAD CsrThread;
    PCSR_PROCESS CsrProcess;
    CLIENT_ID ClientId = *Cid;
    PLIST_ENTRY ListHead, NextEntry;

    /* Acquire lock */
    CsrAcquireProcessLock();

    /* Find the thread */
    CsrThread = CsrLocateThreadByClientId(&CsrProcess,
                                          &ClientId);

    /* Make sure we got one back, and that it's not already gone */
    if (!CsrThread || CsrProcess->Flags & CsrProcessTerminating)
    {
        /* Release the lock and return failure */
        CsrReleaseProcessLock();
        return STATUS_THREAD_IS_TERMINATING;
    }

    /* Set the terminated flag */
    CsrProcess->Flags |= CsrProcessTerminating;

    /* Get the List Pointers */
    ListHead = &CsrProcess->ThreadList;
    NextEntry = ListHead->Flink;

    /* Loop the list */
    while (NextEntry != ListHead)
    {
        /* Get the current thread entry */
        CsrThread = CONTAINING_RECORD(NextEntry, CSR_THREAD, Link);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;

        /* Make sure the thread isn't already dead */
        if (CsrThread->Flags & CsrThreadTerminated) continue;

        /* Set the Terminated flag */
        CsrThread->Flags |= CsrThreadTerminated;

        /* Acquire the Wait Lock */
        CsrAcquireWaitLock();

        /* Do we have an active wait block? */
        if (CsrThread->WaitBlock)
        {
            /* Notify waiters of termination */
            CsrNotifyWaitBlock(CsrThread->WaitBlock,
                               NULL,
                               NULL,
                               NULL,
                               CsrProcessTerminating,
                               TRUE);
        }

        /* Release the Wait Lock */
        CsrReleaseWaitLock();

        /* Dereference the thread */
        CsrLockedDereferenceThread(CsrThread);
    }

    /* Release the Process Lock and return success */
    CsrReleaseProcessLock();
    return STATUS_SUCCESS;
}

/*++
 * @name CsrGetProcessLuid
 * @implemented NT4
 *
 * Do nothing for 500ms.
 *
 * @param hProcess
 *        Optional handle to the process whose LUID should be returned.
 *
 * @param Luid
 *        Pointer to a LUID Pointer which will receive the CSR Process' LUID    
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks If hProcess is not supplied, then the current thread's token will
 *          be used. If that too is missing, then the current process' token
 *          will be used.
 *
 *--*/
NTSTATUS
NTAPI
CsrGetProcessLuid(HANDLE hProcess OPTIONAL,
                  PLUID Luid)
{
    HANDLE hToken = NULL;
    NTSTATUS Status;
    ULONG Length;
    PTOKEN_STATISTICS TokenStats;

    /* Check if we have a handle to a CSR Process */
    if (!hProcess)
    {
        /* We don't, so try opening the Thread's Token */
        Status = NtOpenThreadToken(NtCurrentThread(),
                                   TOKEN_QUERY,
                                   FALSE,
                                   &hToken);

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* If we got some other failure, then return and quit */
            if (Status != STATUS_NO_TOKEN) return Status;

            /* We don't have a Thread Token, use a Process Token */
            hProcess = NtCurrentProcess();
            hToken = NULL;
        }
    }

    /* Check if we have a token by now */
    if (!hToken)
    {
        /* No token yet, so open the Process Token */
        Status = NtOpenProcessToken(hProcess,
                                    TOKEN_QUERY,
                                    &hToken);
        if (!NT_SUCCESS(Status))
        {
            /* Still no token, return the error */
            return Status;
        }
    }

    /* Now get the size we'll need for the Token Information */
    Status = NtQueryInformationToken(hToken,
                                     TokenStatistics,
                                     NULL,
                                     0,
                                     &Length);

    /* Allocate memory for the Token Info */
    if (!(TokenStats = RtlAllocateHeap(CsrHeap, 0, Length)))
    {
        /* Fail and close the token */
        NtClose(hToken);
        return STATUS_NO_MEMORY;
    }

    /* Now query the information */
    Status = NtQueryInformationToken(hToken,
                                     TokenStatistics,
                                     &TokenStats,
                                     Length,
                                     &Length);

    /* Close the handle */
    NtClose(hToken);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Return the LUID */
        *Luid = TokenStats->AuthenticationId;
    }

    /* Free the query information */
    RtlFreeHeap(CsrHeap, 0, TokenStats);

    /* Return the Status */
    return Status;
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
                         OUT PCSR_PROCESS *CsrProcess OPTIONAL)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_PROCESS CurrentProcess = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Acquire the lock */
    CsrAcquireProcessLock();

    /* Setup the List Pointers */
    ListHead = &CsrRootProcess->ListLink;
    NextEntry = ListHead;

    /* Start Loop */
    while (NextEntry != ListHead)
    {
        /* Get the Process */
        CurrentProcess = CONTAINING_RECORD(NextEntry, CSR_PROCESS, ListLink);

        /* Check for PID Match */
        if (CurrentProcess->ClientId.UniqueProcess == Pid)
        {
            /* Get out of here with success */
            Status = STATUS_SUCCESS;
            break;
        }

        /* Next entry */
        NextEntry = NextEntry->Flink;
    }

    /* Did the loop find something? */
    if (NT_SUCCESS(Status))
    {
        /* Lock the found process */
        CurrentProcess->ReferenceCount++;
    }
    else
    {
        /* Nothing found, release the lock */
        CsrReleaseProcessLock();
    }

    /* Return the status and process */
    if (CsrProcess) *CsrProcess = CurrentProcess;
    return Status;
}

/*++
 * @name CsrSetForegroundPriority
 * @implemented NT4
 *
 * The CsrSetForegroundPriority routine sets the priority for the given CSR
 * Process as a Foreground priority.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process whose priority will be modified.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrSetForegroundPriority(IN PCSR_PROCESS CsrProcess)
{
    PROCESS_PRIORITY_CLASS PriorityClass;

    /* Set the Foreground bit on */
    PriorityClass.Foreground = TRUE;

    /* Set the new Priority */
    NtSetInformationProcess(CsrProcess->ProcessHandle,
                            ProcessPriorityClass,
                            &PriorityClass,
                            sizeof(PriorityClass));
}

/*++
 * @name CsrSetBackgroundPriority
 * @implemented NT4
 *
 * The CsrSetBackgroundPriority routine sets the priority for the given CSR
 * Process as a Background priority.
 *
 * @param CsrProcess
 *        Pointer to the CSR Process whose priority will be modified.
 *
 * @return None.
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
CsrSetBackgroundPriority(IN PCSR_PROCESS CsrProcess)
{
    PROCESS_PRIORITY_CLASS PriorityClass;

    /* Set the Foreground bit off */
    PriorityClass.Foreground = FALSE;

    /* Set the new Priority */
    NtSetInformationProcess(CsrProcess->ProcessHandle,
                            ProcessPriorityClass,
                            &PriorityClass,
                            sizeof(PriorityClass));
}

/*++
 * @name CsrShutdownProcesses
 * @implemented NT4
 *
 * The CsrShutdownProcesses routine shuts down every CSR Process possible
 * and calls each Server DLL's shutdown notification.
 *
 * @param CallerLuid
 *        Pointer to the LUID of the CSR Process that is ordering the
 *        shutdown.
 *
 * @param Flags
 *        Flags to send to the shutdown notification routine.
 *
 * @return STATUS_SUCCESS in case of success, STATUS_UNSUCCESSFUL
 *         othwerwise.
 *
 * @remarks None.
 *
 *--*/
NTSTATUS
NTAPI
CsrShutdownProcesses(PLUID CallerLuid,
                     ULONG Flags)
{
    PLIST_ENTRY ListHead, NextEntry;
    PCSR_PROCESS CsrProcess = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN FirstTry = TRUE;
    ULONG i = 0;
    PCSR_SERVER_DLL ServerDll = NULL;
    ULONG Result = 0;

    /* Acquire process lock */
    CsrAcquireProcessLock();

    /* Add shutdown flag */
    CsrRootProcess->ShutdownFlags |= CsrShutdownSystem;

    /* Get the list pointers */
    ListHead = &CsrRootProcess->ListLink;
    NextEntry = ListHead->Flink;

    /* Start the loop */
    while (NextEntry != ListHead)
    {
        /* Get the Process */
        CsrProcess = CONTAINING_RECORD(NextEntry, CSR_PROCESS, ListLink);

        /* Remove the skip flag, set shutdown flags to 0*/
        CsrProcess->Flags &= ~CsrProcessSkipShutdown;
        CsrProcess->ShutdownFlags = 0;

        /* Move to the next */
        NextEntry = NextEntry->Flink;
    }

    /* Set shudown Priority */
    CsrSetToShutdownPriority();

    /* Start looping */
    while (TRUE)
    {
        /* Find the next process to shutdown */
        if (!(CsrProcess = FindProcessForShutdown(CallerLuid)))
        {
            /* Done, quit */
            CsrReleaseProcessLock();
            Status = STATUS_SUCCESS;
            goto Quickie;
         }

        /* Increase reference to process */
        CsrProcess->ReferenceCount++;

LoopAgain:
        /* Loop all the servers */
        for (i = 0; i < CSR_SERVER_DLL_MAX; i++)
        {
            /* Get the current server */
            ServerDll = CsrLoadedServerDll[i];
            if (ServerDll && ServerDll->ShutdownProcessCallback)
            {
                /* Release the lock, make the callback, and acquire it back */
                CsrReleaseProcessLock();
                Result = (*ServerDll->ShutdownProcessCallback)(CsrProcess,
                                                               Flags,
                                                               FirstTry);
                CsrAcquireProcessLock();

                /* Check the result */
                if (Result == CsrShutdownCsrProcess)
                {
                    /* The callback unlocked the process */
                    break;
                }
                else if (Result == CsrShutdownNonCsrProcess)
                {
                    /* A non-CSR process, the callback didn't touch it */
                    continue;
                }
                else if (Result == CsrShutdownCancelled)
                {
                    /* Shutdown was cancelled, unlock and exit */
                    CsrReleaseProcessLock();
                    Status = STATUS_CANCELLED;
                    goto Quickie;
                }
            }
        }

        /* No matches during the first try, so loop again */
        if (FirstTry && Result == CsrShutdownNonCsrProcess)
        {
            FirstTry = FALSE;
            goto LoopAgain;
        }

        /* We've reached the final loop here, so dereference */
        if (i == CSR_SERVER_DLL_MAX) CsrLockedDereferenceProcess(CsrProcess);
    }

Quickie:
    /* Return to normal priority */
    CsrSetToNormalPriority();
    return Status;
}

/*++
 * @name CsrUnlockProcess
 * @implemented NT4
 *
 * The CsrUnlockProcess undoes a previous CsrLockProcessByClientId operation.
 *
 * @param CsrProcess
 *        Pointer to a previously locked CSR Process. 
 *
 * @return STATUS_SUCCESS.
 *
 * @remarks This routine must be called with the Process Lock held.
 *
 *--*/
NTSTATUS
NTAPI
CsrUnlockProcess(PCSR_PROCESS CsrProcess)
{
    /* Dereference the process */
    CsrLockedDereferenceProcess(CsrProcess);

    /* Release the lock and return */
    CsrReleaseProcessLock();
    return STATUS_SUCCESS;
}

/* EOF */
