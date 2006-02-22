/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ke/process.c
 * PURPOSE:         Kernel Process Management and System Call Tables
 * PROGRAMMERS:     Alex Ionescu
 *                  Gregor Anich
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/napi.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS   *****************************************************************/

KSERVICE_TABLE_DESCRIPTOR
__declspec(dllexport)
KeServiceDescriptorTable[SSDT_MAX_ENTRIES] =
{
    { MainSSDT, NULL, NUMBER_OF_SYSCALLS, MainSSPT },
    { NULL,     NULL,   0,   NULL   },
    { NULL,     NULL,   0,   NULL   },
    { NULL,     NULL,   0,   NULL   }
};

KSERVICE_TABLE_DESCRIPTOR
KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES] =
{
    { MainSSDT, NULL, NUMBER_OF_SYSCALLS, MainSSPT },
    { NULL,     NULL,   0,   NULL   },
    { NULL,     NULL,   0,   NULL   },
    { NULL,     NULL,   0,   NULL   }
};

/* FUNCTIONS *****************************************************************/

PKPROCESS
STDCALL
KeGetCurrentProcess(VOID)
{
    return(&(PsGetCurrentProcess()->Pcb));
}

static __inline
VOID
NTAPI
UpdatePageDirs(IN PKTHREAD Thread,
               IN PKPROCESS Process)
{
    /*
     * The stack and the thread structure of the current process may be
     * located in a page which is not present in the page directory of
     * the process we're attaching to. That would lead to a page fault
     * when this function returns. However, since the processor can't
     * call the page fault handler 'cause it can't push EIP on the stack,
     * this will show up as a stack fault which will crash the entire system.
     * To prevent this, make sure the page directory of the process we're
     * attaching to is up-to-date.
     */
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread->StackLimit, KERNEL_STACK_SIZE);
    MmUpdatePageDir((PEPROCESS)Process, (PVOID)Thread, sizeof(ETHREAD));
}

VOID
NTAPI
KiAttachProcess(PKTHREAD Thread,
                PKPROCESS Process,
                KIRQL OldIrql,
                PRKAPC_STATE SavedApcState)
{
    ASSERT(Process != Thread->ApcState.Process);
    DPRINT("KiAttachProcess(Thread: %x, Process: %x, SavedApcState: %x\n",
            Thread, Process, SavedApcState);

    /* Increase Stack Count */
    Process->StackCount++;

    /* Swap the APC Environment */
    KiMoveApcState(&Thread->ApcState, SavedApcState);

    /* Reinitialize Apc State */
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = Process;
    Thread->ApcState.KernelApcInProgress = FALSE;
    Thread->ApcState.KernelApcPending = FALSE;
    Thread->ApcState.UserApcPending = FALSE;

    /* Update Environment Pointers if needed*/
    if (SavedApcState == &Thread->SavedApcState)
    {
        Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->SavedApcState;
        Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->ApcState;
        Thread->ApcStateIndex = AttachedApcEnvironment;
    }

    /* Check if the process is paged in */
    if (Process->State == ProcessInMemory)
    {
        /* FIXME: Scan the Ready Thread List once new scheduler is in */

        /* Swap the Processes */
        KiSwapProcess(Process, SavedApcState->Process);

        /* Return to old IRQL*/
        KeReleaseDispatcherDatabaseLock(OldIrql);
    }
    else
    {
        DPRINT1("Errr. ReactOS doesn't support paging out processes yet...\n");
        DbgBreakPoint();
    }
}

VOID
NTAPI
KeInitializeProcess(PKPROCESS Process,
                    KPRIORITY Priority,
                    KAFFINITY Affinity,
                    LARGE_INTEGER DirectoryTableBase)
{
    DPRINT("KeInitializeProcess. Process: %x, DirectoryTableBase: %x\n",
            Process, DirectoryTableBase);

    /* Initialize the Dispatcher Header */
    KeInitializeDispatcherHeader(&Process->Header,
                                 ProcessObject,
                                 sizeof(KPROCESS),
                                 FALSE);

    /* Initialize Scheduler Data, Disable Alignment Faults and Set the PDE */
    Process->Affinity = Affinity;
    Process->BasePriority = Priority;
    Process->QuantumReset = 6;
    Process->DirectoryTableBase = DirectoryTableBase;
    Process->AutoAlignment = TRUE;
    Process->IopmOffset = 0xFFFF;
    Process->State = ProcessInMemory;

    /* Initialize the Thread List */
    InitializeListHead(&Process->ThreadListHead);
    KeInitializeSpinLock(&Process->ProcessLock);
    DPRINT("The Process has now been initalized with the Kernel\n");
}

ULONG
NTAPI
KeSetProcess(PKPROCESS Process,
             KPRIORITY Increment)
{
    KIRQL OldIrql;
    ULONG OldState;

    /* Lock Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Get Old State */
    OldState = Process->Header.SignalState;

    /* Signal the Process */
    Process->Header.SignalState = TRUE;
    if ((OldState == 0) && IsListEmpty(&Process->Header.WaitListHead) != TRUE)
    {
        /* Satisfy waits */
        KiWaitTest((PVOID)Process, Increment);
    }

    /* Release Dispatcher Database */
    KeReleaseDispatcherDatabaseLock(OldIrql);

    /* Return the previous State */
    return OldState;
}

VOID
NTAPI
KiSwapProcess(PKPROCESS NewProcess,
              PKPROCESS OldProcess)
{
    DPRINT("Switching CR3 to: %x\n", NewProcess->DirectoryTableBase.u.LowPart);
    Ke386SetPageTableDirectory(NewProcess->DirectoryTableBase.u.LowPart);
}

/*
 * @implemented
 */
VOID
NTAPI
KeAttachProcess(PKPROCESS Process)
{
    KIRQL OldIrql;
    PKTHREAD Thread;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    DPRINT("KeAttachProcess: %x\n", Process);

    /* Make sure that we are in the right page directory */
    Thread = KeGetCurrentThread();
    UpdatePageDirs(Thread, Process);

    /* Lock Dispatcher */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Check if we're already in that process */
    if (Thread->ApcState.Process == Process)
    {
        /* Unlock the dispatcher, nothing to do */
        KeReleaseDispatcherDatabaseLock(OldIrql);
    }
    else if ((Thread->ApcStateIndex != OriginalApcEnvironment) ||
             (KeIsExecutingDpc()))
    {
        /* Executing a DPC or already attached, crash! */
        KEBUGCHECKEX(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     Thread->ApcStateIndex,
                     KeIsExecutingDpc());
    }
    else
    {
        /* Legit attach attempt: do it! */
        KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeDetachProcess (VOID)
{
    PKTHREAD Thread;
    KIRQL OldIrql;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);
    DPRINT("KeDetachProcess()\n");

    /* Get Current Thread and lock the dispatcher */
    Thread = KeGetCurrentThread();
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Check if it's attached */
    if (Thread->ApcStateIndex != OriginalApcEnvironment)
    {
        /* It is, decrease Stack Count */
        if(!(--Thread->ApcState.Process->StackCount))
        {
            /* FIXME: Swap the process out */
        }

        /* Restore the APC State */
        KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
        Thread->SavedApcState.Process = NULL;
        Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
        Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
        Thread->ApcStateIndex = OriginalApcEnvironment;

        /* Check if we have pending APCs */
        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
        {
            /* What do you know, we do! Request them to be delivered */
            Thread->ApcState.KernelApcPending = TRUE;
            HalRequestSoftwareInterrupt(APC_LEVEL);
        }

        /* Swap Processes */
        KiSwapProcess(Thread->ApcState.Process, Thread->ApcState.Process);
    }

    /* Unlock Dispatcher */
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeIsAttachedProcess(VOID)
{
    /* Return the APC State */
    return KeGetCurrentThread()->ApcStateIndex;
}

/*
 * @implemented
 */
VOID
NTAPI
KeStackAttachProcess(IN PKPROCESS Process,
                     OUT PRKAPC_STATE ApcState)
{
    KIRQL OldIrql;
    PKTHREAD Thread;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Make sure that we are in the right page directory */
    Thread = KeGetCurrentThread();
    UpdatePageDirs(Thread, Process);

    /* Acquire the dispatcher lock */
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Crash system if DPC is being executed! */
    if (KeIsExecutingDpc())
    {
        /* Executing a DPC, crash! */
        KEBUGCHECKEX(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     Thread->ApcStateIndex,
                     KeIsExecutingDpc());
    }

    /* Check if we are already in the target process */
    if (Thread->ApcState.Process == Process)
    {
        /* Unlock the dispatcher database */
        KeReleaseDispatcherDatabaseLock(OldIrql);

        /* Set magic value so we don't crash later when detaching */
        ApcState->Process = (PKPROCESS)1;
    }
    else
    {
        /* Check if the Current Thread is already attached */
        if (Thread->ApcStateIndex != OriginalApcEnvironment)
        {
            /* We're already attached, so save the APC State into what we got */
            KiAttachProcess(Thread, Process, OldIrql, ApcState);
        }
        else
        {
            /* We're not attached, so save the APC State into SavedApcState */
            KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
            ApcState->Process = NULL;
        }
    }
}

/*
 * @implemented
 */
VOID
NTAPI
KeUnstackDetachProcess(IN PRKAPC_STATE ApcState)
{
    KIRQL OldIrql;
    PKTHREAD Thread;
    ASSERT_IRQL_LESS_OR_EQUAL(DISPATCH_LEVEL);

    /* Get the current thread and acquire the dispatcher lock */
    Thread = KeGetCurrentThread();
    OldIrql = KeAcquireDispatcherDatabaseLock();

    /* Check for magic value meaning we were already in the same process */
    if (ApcState->Process != (PKPROCESS)1)
    {
        /*
         * Check if the process isn't attacked, or has a Kernel APC in progress
         * or has pending APC of any kind.
         */
        if ((Thread->ApcStateIndex == OriginalApcEnvironment) ||
            (Thread->ApcState.KernelApcInProgress) ||
            (!IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode])) ||
            (!IsListEmpty(&Thread->ApcState.ApcListHead[UserMode])))
        {
            KEBUGCHECK(INVALID_PROCESS_DETACH_ATTEMPT);
        }

        /* Decrease Stack Count */
        if(!(--Thread->ApcState.Process->StackCount))
        {
            /* FIXME: Swap the process out */
        }

        if (ApcState->Process != NULL)
        {
            /* Restore the APC State */
            KiMoveApcState(ApcState, &Thread->ApcState);
        }
        else
        {
            /* The ApcState parameter is useless, so use the saved data and reset it */
            KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
            Thread->SavedApcState.Process = NULL;
            Thread->ApcStateIndex = OriginalApcEnvironment;
            Thread->ApcStatePointer[OriginalApcEnvironment] = &Thread->ApcState;
            Thread->ApcStatePointer[AttachedApcEnvironment] = &Thread->SavedApcState;
        }

        /* Check if we have pending APCs */
        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]))
        {
            /* What do you know, we do! Request them to be delivered */
            Thread->ApcState.KernelApcPending = TRUE;
            HalRequestSoftwareInterrupt(APC_LEVEL);
        }

        /* Swap Processes */
        KiSwapProcess(Thread->ApcState.Process, Thread->ApcState.Process);
    }

    /* Return to old IRQL*/
    KeReleaseDispatcherDatabaseLock(OldIrql);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeAddSystemServiceTable(PULONG_PTR Base,
                        PULONG Count OPTIONAL,
                        ULONG Limit,
                        PUCHAR Number,
                        ULONG Index)
{
    /* Check if descriptor table entry is free */
    if ((Index > SSDT_MAX_ENTRIES - 1) ||
        (KeServiceDescriptorTable[Index].Base) ||
        (KeServiceDescriptorTableShadow[Index].Base))
    {
        return FALSE;
    }

    /* Initialize the shadow service descriptor table */
    KeServiceDescriptorTableShadow[Index].Base = Base;
    KeServiceDescriptorTableShadow[Index].Limit = Limit;
    KeServiceDescriptorTableShadow[Index].Number = Number;
    KeServiceDescriptorTableShadow[Index].Count = Count;

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
KeRemoveSystemServiceTable(IN ULONG Index)
{
    /* Make sure the Index is valid */
    if (Index > SSDT_MAX_ENTRIES - 1) return FALSE;

    /* Is there a Normal Descriptor Table? */
    if (!KeServiceDescriptorTable[Index].Base)
    {
        /* Not with the index, is there a shadow at least? */
        if (!KeServiceDescriptorTableShadow[Index].Base) return FALSE;
    }

    /* Now clear from the Shadow Table. */
    KeServiceDescriptorTableShadow[Index].Base = NULL;
    KeServiceDescriptorTableShadow[Index].Number = NULL;
    KeServiceDescriptorTableShadow[Index].Limit = 0;
    KeServiceDescriptorTableShadow[Index].Count = NULL;

    /* Check if we should clean from the Master one too */
    if (Index == 1)
    {
        KeServiceDescriptorTable[Index].Base = NULL;
        KeServiceDescriptorTable[Index].Number = NULL;
        KeServiceDescriptorTable[Index].Limit = 0;
        KeServiceDescriptorTable[Index].Count = NULL;
    }

    return TRUE;
}
/* EOF */
