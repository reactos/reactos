/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsystems/win32/csrss/csrsrv/procsup.c
 * PURPOSE:         CSR Process Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Alex Ionescu
 */
 
/* INCLUDES *******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

RTL_CRITICAL_SECTION ProcessDataLock, CsrWaitListsLock;
PCSR_PROCESS CsrRootProcess;
SECURITY_QUALITY_OF_SERVICE CsrSecurityQos =
{
    sizeof(SECURITY_QUALITY_OF_SERVICE),
    SecurityImpersonation,
    SECURITY_STATIC_TRACKING,
    FALSE
};
LONG CsrProcessSequenceCount = 5;
extern ULONG CsrTotalPerProcessDataLength;

/* FUNCTIONS ******************************************************************/

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
                                     TokenStats,
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

BOOLEAN
NTAPI
CsrImpersonateClient(IN PCSR_THREAD CsrThread)
{
    NTSTATUS Status;
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;

    /* Use the current thread if none given */
    if (!CsrThread) CsrThread = CurrentThread;

    /* Still no thread, something is wrong */
    if (!CsrThread)
    {
        /* Failure */
        return FALSE;
    }

    /* Make the call */
    Status = NtImpersonateThread(NtCurrentThread(),
                                 CsrThread->ThreadHandle,
                                 &CsrSecurityQos);

    if (!NT_SUCCESS(Status))
    {
        /* Failure */
        return FALSE;
    }

    /* Increase the impersonation count for the current thread */
    if (CurrentThread) ++CurrentThread->ImpersonationCount;

    /* Return Success */
    return TRUE;
}

BOOLEAN
NTAPI
CsrRevertToSelf(VOID)
{
    NTSTATUS Status;
    PCSR_THREAD CurrentThread = NtCurrentTeb()->CsrClientThread;
    HANDLE ImpersonationToken = NULL;

    /* Check if we have a Current Thread */
    if (CurrentThread)
    {
        /* Make sure impersonation is on */
        if (!CurrentThread->ImpersonationCount)
        {
            return FALSE;
        }
        else if (--CurrentThread->ImpersonationCount > 0)
        {
            /* Success; impersonation count decreased but still not zero */
            return TRUE;
        }
    }

    /* Impersonation has been totally removed, revert to ourselves */
    Status = NtSetInformationThread(NtCurrentThread(),
                                    ThreadImpersonationToken,
                                    &ImpersonationToken,
                                    sizeof(HANDLE));

    /* Return TRUE or FALSE */
    return NT_SUCCESS(Status);
}

PCSR_PROCESS
NTAPI
FindProcessForShutdown(IN PLUID CallerLuid)
{
    PCSR_PROCESS CsrProcess, ReturnCsrProcess = NULL;
    NTSTATUS Status;
    ULONG Level = 0;
    LUID ProcessLuid;
    LUID SystemLuid = SYSTEM_LUID;
    BOOLEAN IsSystemLuid = FALSE, IsOurLuid = FALSE;
    PLIST_ENTRY NextEntry;
    
    /* Set the List Pointers */
    NextEntry = CsrRootProcess->ListLink.Flink;
    while (NextEntry != &CsrRootProcess->ListLink)
    {
        /* Get the process */
        CsrProcess = CONTAINING_RECORD(NextEntry, CSR_PROCESS, ListLink);

        /* Move to the next entry */
        NextEntry = NextEntry->Flink;
        
        /* Skip this process if it's already been processed */
        if (CsrProcess->Flags & CsrProcessSkipShutdown) continue;
    
        /* Get the LUID of this Process */
        Status = CsrGetProcessLuid(CsrProcess->ProcessHandle, &ProcessLuid);

        /* Check if we didn't get access to the LUID */
        if (Status == STATUS_ACCESS_DENIED)
        {
            /* FIXME:Check if we have any threads */
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

/* This is really "CsrShutdownProcess", mostly */
NTSTATUS
WINAPI
CsrEnumProcesses(IN CSRSS_ENUM_PROCESS_PROC EnumProc,
                 IN PVOID Context)
{
    PVOID* RealContext = (PVOID*)Context;
    PLUID CallerLuid = RealContext[0];
    PCSR_PROCESS CsrProcess = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN FirstTry;
    PLIST_ENTRY NextEntry;
    ULONG Result = 0;

    /* Acquire process lock */
    CsrAcquireProcessLock();
    
    /* Get the list pointers */
    NextEntry = CsrRootProcess->ListLink.Flink;
    while (NextEntry != &CsrRootProcess->ListLink)
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

    /* Loop all processes */
    //DPRINT1("Enumerating for LUID: %lx %lx\n", CallerLuid->HighPart, CallerLuid->LowPart);
    
    /* Start looping */
    while (TRUE)
    {
        /* Find the next process to shutdown */
        FirstTry = TRUE;
        if (!(CsrProcess = FindProcessForShutdown(CallerLuid)))
        {
            /* Done, quit */
            CsrReleaseProcessLock();
            Status = STATUS_SUCCESS;
            goto Quickie;
        }

LoopAgain:
        /* Release the lock, make the callback, and acquire it back */
        //DPRINT1("Found process: %lx\n", CsrProcess->ClientId.UniqueProcess);
        CsrReleaseProcessLock();
        Result = (ULONG)EnumProc(CsrProcess, (PVOID)((ULONG_PTR)Context | FirstTry));
        CsrAcquireProcessLock();

        /* Check the result */
        //DPRINT1("Result: %d\n", Result);
        if (Result == CsrShutdownCsrProcess)
        {
            /* The callback unlocked the process */
            break;
        }
        else if (Result == CsrShutdownNonCsrProcess)
        {
            /* A non-CSR process, the callback didn't touch it */
            //continue;
        }
        else if (Result == CsrShutdownCancelled)
        {
            /* Shutdown was cancelled, unlock and exit */
            CsrReleaseProcessLock();
            Status = STATUS_CANCELLED;
            goto Quickie;
        }

        /* No matches during the first try, so loop again */
        if (FirstTry && Result == CsrShutdownNonCsrProcess)
        {
            FirstTry = FALSE;
            goto LoopAgain;
        }
    }

Quickie:
    /* Return to normal priority */
    CsrSetToNormalPriority();
    return Status;
}

NTSTATUS
NTAPI
CsrUnlockProcess(IN PCSR_PROCESS CsrProcess)
{
    /* Dereference the process */
    //CsrLockedDereferenceProcess(CsrProcess);

    /* Release the lock and return */
    CsrReleaseProcessLock();
    return STATUS_SUCCESS;
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

/* EOF */
