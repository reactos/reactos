/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS CSR Sub System
 * FILE:            subsys/csr/csrsrv/procsup.c
 * PURPOSE:         CSR Process Management
 * PROGRAMMERS:     ReactOS Portable Systems Group
 *                  Alex Ionescu
 */
 
/* INCLUDES *******************************************************************/

#include <srv.h>

#define NDEBUG
#include <debug.h>

#define LOCK   RtlEnterCriticalSection(&ProcessDataLock)
#define UNLOCK RtlLeaveCriticalSection(&ProcessDataLock)
#define CsrHeap RtlGetProcessHeap()

#define CsrAcquireProcessLock() LOCK
#define CsrReleaseProcessLock() UNLOCK

/* GLOBALS ********************************************************************/

extern RTL_CRITICAL_SECTION ProcessDataLock;
extern PCSRSS_PROCESS_DATA ProcessData[256];
PCSRSS_PROCESS_DATA CsrRootProcess;
LIST_ENTRY CsrThreadHashTable[256];
SECURITY_QUALITY_OF_SERVICE CsrSecurityQos =
{
    sizeof(SECURITY_QUALITY_OF_SERVICE),
    SecurityImpersonation,
    SECURITY_STATIC_TRACKING,
    FALSE
};

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

PCSRSS_PROCESS_DATA
NTAPI
FindProcessForShutdown(IN PLUID CallerLuid)
{
    ULONG Hash;
    PCSRSS_PROCESS_DATA CsrProcess, ReturnCsrProcess = NULL;
    NTSTATUS Status;
    ULONG Level = 0;
    LUID ProcessLuid;
    LUID SystemLuid = SYSTEM_LUID;
    BOOLEAN IsSystemLuid = FALSE, IsOurLuid = FALSE;
    
    for (Hash = 0; Hash < (sizeof(ProcessData) / sizeof(*ProcessData)); Hash++)
    {
        /* Get this process hash bucket */
        CsrProcess = ProcessData[Hash];
        while (CsrProcess)
        {
            /* Skip this process if it's already been processed*/
            if (CsrProcess->Flags & CsrProcessSkipShutdown) goto Next;
        
            /* Get the LUID of this Process */
            Status = CsrGetProcessLuid(CsrProcess->Process, &ProcessLuid);

            /* Check if we didn't get access to the LUID */
            if (Status == STATUS_ACCESS_DENIED)
            {
                /* FIXME:Check if we have any threads */
            }
            
            if (!NT_SUCCESS(Status))
            {
                /* We didn't have access, so skip it */
                CsrProcess->Flags |= CsrProcessSkipShutdown;
                goto Next;
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
Next:
            /* Next process */
            CsrProcess = CsrProcess->next;
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
    PCSRSS_PROCESS_DATA CsrProcess = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    BOOLEAN FirstTry;
    ULONG Result = 0;
    ULONG Hash;

    /* Acquire process lock */
    CsrAcquireProcessLock();

    /* Start the loop */
    for (Hash = 0; Hash < (sizeof(ProcessData) / sizeof(*ProcessData)); Hash++)
    {
        /* Get the Process */
        CsrProcess = ProcessData[Hash];
        while (CsrProcess)
        {
           /* Remove the skip flag, set shutdown flags to 0*/
            CsrProcess->Flags &= ~CsrProcessSkipShutdown;
            CsrProcess->ShutdownFlags = 0;

            /* Move to the next */
            CsrProcess = CsrProcess->next;
        }
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
        //DPRINT1("Found process: %lx\n", CsrProcess->ProcessId);
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
CsrLockProcessByClientId(IN HANDLE Pid,
                         OUT PCSRSS_PROCESS_DATA *CsrProcess OPTIONAL)
{
    ULONG Hash;
    PCSRSS_PROCESS_DATA CurrentProcess = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;

    /* Acquire the lock */
    CsrAcquireProcessLock();

    /* Start the loop */
    for (Hash = 0; Hash < (sizeof(ProcessData) / sizeof(*ProcessData)); Hash++)
    {
        /* Get the Process */
        CurrentProcess = ProcessData[Hash];
        while (CurrentProcess)
        {
            /* Check for PID match */
            if (CurrentProcess->ProcessId == Pid)
            {
                /* Get out of here with success */
//                DPRINT1("Found %p for PID %lx\n", CurrentProcess, Pid);
                Status = STATUS_SUCCESS;
                goto Found;
            }
            
            /* Move to the next */
            CurrentProcess = CurrentProcess->next;
        }
    }
    
    /* Nothing found, release the lock */
Found:
    if (!CurrentProcess) CsrReleaseProcessLock();

    /* Return the status and process */
    if (CsrProcess) *CsrProcess = CurrentProcess;
    return Status;
}

NTSTATUS
NTAPI
CsrUnlockProcess(IN PCSRSS_PROCESS_DATA CsrProcess)
{
    /* Dereference the process */
    //CsrLockedDereferenceProcess(CsrProcess);

    /* Release the lock and return */
    CsrReleaseProcessLock();
    return STATUS_SUCCESS;
}

/* EOF */
