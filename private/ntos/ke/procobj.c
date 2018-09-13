/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    procobj.c

Abstract:

    This module implements the machine independent functions to manipulate
    the kernel process object. Functions are provided to initialize, attach,
    detach, exclude, include, and set the base priority of process objects.

Author:

    David N. Cutler (davec) 7-Mar-1989

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KeInitializeProcess)
#endif


//
// Define forward referenced function prototypes.
//

VOID
KiAttachProcess (
    IN PRKTHREAD Thread,
    IN PRKPROCESS Process,
    IN KIRQL OldIrql,
    OUT PRKAPC_STATE SavedApcState
    );

VOID
KiMoveApcState (
    IN PKAPC_STATE Source,
    OUT PKAPC_STATE Destination
    );

//
// The following assert macro is used to check that an input process is
// really a kprocess and not something else, like deallocated pool.
//

#define ASSERT_PROCESS(E) {             \
    ASSERT((E)->Header.Type == ProcessObject); \
}


VOID
KeInitializeProcess (
    IN PRKPROCESS Process,
    IN KPRIORITY BasePriority,
    IN KAFFINITY Affinity,
    IN ULONG_PTR DirectoryTableBase[2],
    IN BOOLEAN Enable
    )

/*++

Routine Description:

    This function initializes a kernel process object. The base priority,
    affinity, and page frame numbers for the process page table directory
    and hyper space are stored in the process object.

    N.B. It is assumed that the process object is zeroed.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

    BasePriority - Supplies the base priority of the process.

    Affinity - Supplies the set of processors on which children threads
        of the process can execute.

    DirectoryTableBase - Supplies a pointer to an array whose fist element
        is the value that is to be loaded into the Directory Table Base
        register when a child thread is dispatched for execution and whose
        second element contains the page table entry that maps hyper space.

    Enable - Supplies a boolean value that determines the default
        handling of data alignment exceptions for child threads. A value
        of TRUE causes all data alignment exceptions to be automatically
        handled by the kernel. A value of FALSE causes all data alignment
        exceptions to be actually raised as exceptions.

Return Value:

    None.

--*/

{

    //
    // Initialize the standard dispatcher object header and set the initial
    // signal state of the process object.
    //

    Process->Header.Type = ProcessObject;
    Process->Header.Size = sizeof(KPROCESS) / sizeof(LONG);
    InitializeListHead(&Process->Header.WaitListHead);

    //
    // Initialize the base priority, affinity, directory table base values,
    // autoalignment, and stack count.
    //
    // N.B. The distinguished value MAXSHORT is used to signify that no
    //      threads have been created for the process.
    //

    Process->BasePriority = (SCHAR)BasePriority;
    Process->Affinity = Affinity;
    Process->AutoAlignment = Enable;
    Process->DirectoryTableBase[0] = DirectoryTableBase[0];
    Process->DirectoryTableBase[1] = DirectoryTableBase[1];
    Process->StackCount = MAXSHORT;

    //
    // Initialize the stack count, profile listhead, ready queue list head,
    // accumulated runtime, process quantum, thread quantum, and thread list
    // head.
    //

    InitializeListHead(&Process->ProfileListHead);
    InitializeListHead(&Process->ReadyListHead);
    InitializeListHead(&Process->ThreadListHead);
    Process->ThreadQuantum = THREAD_QUANTUM;

    //
    // Initialize the process state and set the thread processor selection
    // seed.
    //

    Process->State = ProcessInMemory;
    Process->ThreadSeed = (UCHAR)KiQueryLowTickCount();

    //
    // Initialize IopmBase and Iopl flag for this process (i386 only)
    //

#ifdef i386

    Process->IopmOffset = KiComputeIopmOffset(IO_ACCESS_MAP_NONE);

#endif

    return;
}

VOID
KeAttachProcess (
    IN PRKPROCESS Process
    )

/*++

Routine Description:

    This function attaches a thread to a target process' address space
    if, and only if, there is not already a process attached.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the target process is the current process, then return immediately.
    // Otherwise, check whether there is a process address space attached or
    // the thread is executing a DPC. If either condition is true, then call
    // bug check. Otherwise, attach the target process.
    //

    if (Thread->ApcState.Process == Process) {
        KiUnlockDispatcherDatabase(OldIrql);

    } else if ((Thread->ApcStateIndex != 0) ||
               (KeIsExecutingDpc() != FALSE)) {
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     (ULONG)Thread->ApcStateIndex,
                     (ULONG)KeIsExecutingDpc());

    } else {
        KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
    }

    return;
}

LOGICAL
KeForceAttachProcess (
    IN PRKPROCESS Process
    )

/*++

Routine Description:

    This function forces an attach of a thread to a target process' address
    space if the process is not current being swapped into or out of memory.

    N.B. This function is for use by memory management ONLY.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // Check whether there is already a process address space attached or
    // the thread is executing a DPC. If either condition is true, then call
    // bug check.
    //

    if ((Thread->ApcStateIndex != 0) ||
        (KeIsExecutingDpc() != FALSE)) {
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     (ULONG)Thread->ApcStateIndex,
                     (ULONG)KeIsExecutingDpc());
    }

    //
    // If the target process is not the current process, then attach the target
    // process if the process is not currently being swapped in or out of memory.
    //

    if (Thread->ApcState.Process == Process) {
        KiUnlockDispatcherDatabase(OldIrql);

    } else {

        //
        // If the target process is currently being swapped into or out of memory,
        // then return a value of FALSE. Otherwise, force the process to be inswapped.
        //

        if ((Process->State == ProcessInSwap) || Process->State == ProcessOutSwap) {
            KiUnlockDispatcherDatabase(OldIrql);
            return FALSE;

        } else {

            //
            // If the target process is in transition, then remove it from its
            // transition list.
            //

            if (Process->State == ProcessInTransition) {
                RemoveEntryList(&Process->SwapListEntry);
            }

            //
            // Force the process state to in memory and attach the target process.
            //

            Process->State = ProcessInMemory;
            KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
        }
    }

    return TRUE;
}

VOID
KeStackAttachProcess (
    IN PRKPROCESS Process,
    OUT PRKAPC_STATE ApcState
    )

/*++

Routine Description:

    This function attaches a thread to a target process' address space
    and returns information about a previous attached process.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the current thread is executing a DPC, then bug check.
    //

    if (KeIsExecutingDpc() != FALSE) {
        KeBugCheckEx(INVALID_PROCESS_ATTACH_ATTEMPT,
                     (ULONG_PTR)Process,
                     (ULONG_PTR)Thread->ApcState.Process,
                     (ULONG)Thread->ApcStateIndex,
                     (ULONG)KeIsExecutingDpc());
    }

    //
    // If the target process is not the current process, then attach the target
    // process. Otherwise, return a distinguished process value to indicate that
    // an attach was not performed.
    //

    if (Thread->ApcState.Process == Process) {
        KiUnlockDispatcherDatabase(OldIrql);
        ApcState->Process = (PRKPROCESS)1;

    } else {

        //
        // If the current thread is attached to a process, then save the current
        // APC state in the callers APC state structure. Otherwise, save the
        // current APC state in the saved APC state structure, and return a NULL
        // process pointer.
        //

        if (Thread->ApcStateIndex != 0) {
            KiAttachProcess(Thread, Process, OldIrql, ApcState);

        } else {
            KiAttachProcess(Thread, Process, OldIrql, &Thread->SavedApcState);
            ApcState->Process = NULL;
        }
    }

    return;
}

VOID
KeDetachProcess (
    VOID
    )

/*++

Routine Description:

    This function detaches a thread from another process' address space.

Arguments:

    None.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the current thread is attached to another process, then detach
    // it.
    //

    if (Thread->ApcStateIndex != 0) {

        //
        // Check if a kernel APC is in progress, the kernel APC queue is
        // not empty, or the user APC queue is not empty. If any of these
        // conditions are true, then call bug check.
        //

#if DBG

        if ((Thread->ApcState.KernelApcInProgress) ||
            (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) ||
            (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]) == FALSE)) {
            KeBugCheck(INVALID_PROCESS_DETACH_ATTEMPT);
        }

#endif

        //
        // Unbias current process stack count and check if the process should
        // be swapped out of memory.
        //

        Process = Thread->ApcState.Process;
        Process->StackCount -= 1;
        if ((Process->StackCount == 0) &&
            (IsListEmpty(&Process->ThreadListHead) == FALSE)) {
            Process->State = ProcessInTransition;
            InsertTailList(&KiProcessOutSwapListHead, &Process->SwapListEntry);
            KiSwapEvent.Header.SignalState = 1;
            if (IsListEmpty(&KiSwapEvent.Header.WaitListHead) == FALSE) {
                KiWaitTest(&KiSwapEvent, BALANCE_INCREMENT);
            }
        }

        //
        // Restore APC state and check whether the kernel APC queue contains
        // an entry. If the kernel APC queue contains an entry then set kernel
        // APC pending and request a software interrupt at APC_LEVEL.
        //

        KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
        Thread->SavedApcState.Process = (PKPROCESS)NULL;
        Thread->ApcStatePointer[0] = &Thread->ApcState;
        Thread->ApcStatePointer[1] = &Thread->SavedApcState;
        Thread->ApcStateIndex = 0;
        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) {
            Thread->ApcState.KernelApcPending = TRUE;
            KiRequestSoftwareInterrupt(APC_LEVEL);
        }

        //
        // Swap the address space back to the parent process.
        //

        KiSwapProcess(Thread->ApcState.Process, Process);
    }

    //
    // Lower IRQL to its previous value and return.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

VOID
KeUnstackDetachProcess (
    IN PRKAPC_STATE ApcState
    )

/*++

Routine Description:

    This function detaches a thread from another process' address space
    and restores previous attach state.

Arguments:

    ApcState - Supplies a pointer to an APC state structure that was returned
        from a previous call to stack attach process.

Return Value:

    None.

--*/

{

    KIRQL OldIrql;
    PKPROCESS Process;
    PKTHREAD Thread;

    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    Thread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the APC state has a distinguished process pointer value, then no
    // attach was performed on the paired call to stack attach process.
    //

    if (ApcState->Process != (PRKPROCESS)1) {

        //
        // If the current thread is not attached to another process, a kernel
        // APC is in progress, or either the kernel or user mode APC queues
        // are not empty, then call bug check.
        //

        if ((Thread->ApcStateIndex == 0) ||
             (Thread->ApcState.KernelApcInProgress) ||
             (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) ||
             (IsListEmpty(&Thread->ApcState.ApcListHead[UserMode]) == FALSE)) {
            KeBugCheck(INVALID_PROCESS_DETACH_ATTEMPT);
        }

        //
        // Unbias current process stack count and check if the process should
        // be swapped out of memory.
        //

        Process = Thread->ApcState.Process;
        Process->StackCount -= 1;
        if ((Process->StackCount == 0) &&
            (IsListEmpty(&Process->ThreadListHead) == FALSE)) {
            Process->State = ProcessInTransition;
            InsertTailList(&KiProcessOutSwapListHead, &Process->SwapListEntry);
            KiSwapEvent.Header.SignalState = 1;
            if (IsListEmpty(&KiSwapEvent.Header.WaitListHead) == FALSE) {
                KiWaitTest(&KiSwapEvent, BALANCE_INCREMENT);
            }
        }

        //
        // Restore APC state and check whether the kernel APC queue contains
        // an entry. If the kernel APC queue contains an entry then set kernel
        // APC pending and request a software interrupt at APC_LEVEL.
        //

        if (ApcState->Process != NULL) {
            KiMoveApcState(ApcState, &Thread->ApcState);

        } else {
            KiMoveApcState(&Thread->SavedApcState, &Thread->ApcState);
            Thread->SavedApcState.Process = (PKPROCESS)NULL;
            Thread->ApcStatePointer[0] = &Thread->ApcState;
            Thread->ApcStatePointer[1] = &Thread->SavedApcState;
            Thread->ApcStateIndex = 0;
        }

        if (IsListEmpty(&Thread->ApcState.ApcListHead[KernelMode]) == FALSE) {
            Thread->ApcState.KernelApcPending = TRUE;
            KiRequestSoftwareInterrupt(APC_LEVEL);
        }

        //
        // Swap the address space back to the parent process.
        //

        KiSwapProcess(Thread->ApcState.Process, Process);
    }

    //
    // Lower IRQL to its previous value and return.
    //

    KiUnlockDispatcherDatabase(OldIrql);
    return;
}

LONG
KeReadStateProcess (
    IN PRKPROCESS Process
    )

/*++

Routine Description:

    This function reads the current signal state of a process object.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

Return Value:

    The current signal state of the process object.

--*/

{

    ASSERT_PROCESS(Process);

    //
    // Return current signal state of process object.
    //

    return Process->Header.SignalState;
}

LONG
KeSetProcess (
    IN PRKPROCESS Process,
    IN KPRIORITY Increment,
    IN BOOLEAN Wait
    )

/*++

Routine Description:

    This function sets the signal state of a proces object to Signaled
    and attempts to satisfy as many Waits as possible. The previous
    signal state of the process object is returned as the function value.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

    Increment - Supplies the priority increment that is to be applied
       if setting the process causes a Wait to be satisfied.

    Wait - Supplies a boolean value that signifies whether the call to
       KeSetProcess will be immediately followed by a call to one of the
       kernel Wait functions.

Return Value:

    The previous signal state of the process object.

--*/

{

    KIRQL OldIrql;
    LONG OldState;
    PRKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // If the previous state of the process object is Not-Signaled and
    // the wait queue is not empty, then satisfy as many Waits as
    // possible.
    //

    OldState = Process->Header.SignalState;
    Process->Header.SignalState = 1;
    if ((OldState == 0) && (!IsListEmpty(&Process->Header.WaitListHead))) {
        KiWaitTest(Process, Increment);
    }

    //
    // If the value of the Wait argument is TRUE, then return to the
    // caller with IRQL raised and the dispatcher database locked. Else
    // release the dispatcher database lock and lower IRQL to its
    // previous value.
    //

    if (Wait) {
        Thread = KeGetCurrentThread();
        Thread->WaitNext = Wait;
        Thread->WaitIrql = OldIrql;

    } else {
        KiUnlockDispatcherDatabase(OldIrql);
    }

    //
    // Return previous signal state of process object.
    //

    return OldState;
}

KPRIORITY
KeSetPriorityProcess (
    IN PKPROCESS Process,
    IN KPRIORITY NewBase
    )

/*++

Routine Description:

    This function set the base priority of a process to a new value
    and adjusts the priority and base priority of all child threads
    as appropriate.

Arguments:

    Process - Supplies a pointer to a dispatcher object of type process.

    NewBase - Supplies the new base priority of the process.

Return Value:

    The previous base priority of the process.

--*/

{

    KPRIORITY Adjustment;
    PLIST_ENTRY NextEntry;
    KPRIORITY NewPriority;
    KIRQL OldIrql;
    KPRIORITY OldBase;
    PKTHREAD Thread;

    ASSERT_PROCESS(Process);
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    //
    // Raise IRQL to dispatcher level and lock dispatcher database.
    //

    KiLockDispatcherDatabase(&OldIrql);

    //
    // Save the current process base priority, set the new process base
    // priority, compute the adjustment value, and adjust the priority
    // and base priority of all child threads as appropriate.
    //

    OldBase = Process->BasePriority;
    Process->BasePriority = (SCHAR)NewBase;
    Adjustment = NewBase - OldBase;
    NextEntry = Process->ThreadListHead.Flink;
    if (NewBase >= LOW_REALTIME_PRIORITY) {
        while (NextEntry != &Process->ThreadListHead) {
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            //
            // Compute the new base priority of the thread.
            //

            NewPriority = Thread->BasePriority + Adjustment;

            //
            // If the new base priority is outside the realtime class,
            // then limit the change to the realtime class.
            //

            if (NewPriority < LOW_REALTIME_PRIORITY) {
                NewPriority = LOW_REALTIME_PRIORITY;

            } else if (NewPriority > HIGH_PRIORITY) {
                NewPriority = HIGH_PRIORITY;
            }

            //
            // Set the base priority and the current priority of the
            // thread to the appropriate value.
            //
            // N.B. If priority saturation occured the last time the thread
            //      base priority was set and the new process base priority
            //      is not crossing from variable to realtime, then it is not
            //      necessary to change the thread priority.
            //

            if ((Thread->Saturation == 0) || (OldBase < LOW_REALTIME_PRIORITY)) {
                if (Thread->Saturation > 0) {
                    NewPriority = HIGH_PRIORITY;

                } else if (Thread->Saturation < 0) {
                    NewPriority = LOW_REALTIME_PRIORITY;
                }

                Thread->BasePriority = (SCHAR)NewPriority;
                Thread->Quantum = Process->ThreadQuantum;
                Thread->DecrementCount = 0;
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority);
            }

            NextEntry = NextEntry->Flink;
        }

    } else {
        while (NextEntry != &Process->ThreadListHead) {
            Thread = CONTAINING_RECORD(NextEntry, KTHREAD, ThreadListEntry);

            //
            // Compute the new base priority of the thread.
            //

            NewPriority = Thread->BasePriority + Adjustment;

            //
            // If the new base priority is outside the variable class,
            // then limit the change to the variable class.
            //

            if (NewPriority >= LOW_REALTIME_PRIORITY) {
                NewPriority = LOW_REALTIME_PRIORITY - 1;

            } else if (NewPriority <= LOW_PRIORITY) {
                NewPriority = 1;
            }

            //
            // Set the base priority and the current priority of the
            // thread to the computed value and reset the thread quantum.
            //
            // N.B. If priority saturation occured the last time the thread
            //      base priority was set and the new process base priority
            //      is not crossing from realtime to variable, then it is not
            //      necessary to change the thread priority.
            //

            if ((Thread->Saturation == 0) || (OldBase >= LOW_REALTIME_PRIORITY)) {
                if (Thread->Saturation > 0) {
                    NewPriority = LOW_REALTIME_PRIORITY - 1;

                } else if (Thread->Saturation < 0) {
                    NewPriority = 1;
                }

                Thread->BasePriority = (SCHAR)NewPriority;
                Thread->Quantum = Process->ThreadQuantum;
                Thread->DecrementCount = 0;
                Thread->PriorityDecrement = 0;
                KiSetPriorityThread(Thread, NewPriority);
            }

            NextEntry = NextEntry->Flink;
        }
    }

    //
    // Unlock dispatcher database and lower IRQL to its previous
    // value.
    //

    KiUnlockDispatcherDatabase(OldIrql);

    //
    // Return previous process base priority
    //

    return OldBase;
}

LOGICAL
KeSetDisableQuantumProcess (
    IN PKPROCESS Process,
    IN LOGICAL Disable
    )

/*++

Routine Description:

    This function disables quantum runout for realtime threads in the
    specified process.

Arguments:

    Process  - Supplies a pointer to a dispatcher object of type process.

    Disable - Supplies a logical value that determines whether quantum
        runout for realtime threads in the specified process are disabled
        or enabled.

Return Value:

    The previous value of the disable quantum state variable.

--*/

{

    LOGICAL DisableQuantum;

    ASSERT_PROCESS(Process);

    //
    // Capture the current state of the disable boost variable and set its
    // state to TRUE.
    //

    DisableQuantum = Process->DisableQuantum;
    Process->DisableQuantum = (BOOLEAN)Disable;

    //
    // Return the previous disable quantum state.
    //

    return DisableQuantum;
}

VOID
KiAttachProcess (
    IN PRKTHREAD Thread,
    IN PKPROCESS Process,
    IN KIRQL OldIrql,
    OUT PRKAPC_STATE SavedApcState
    )

/*++

Routine Description:

    This function attaches a thread to a target process' address space.

    N.B. The dispatcher database lock must be held when this routine is
        called.

Arguments:

    Thread - Supplies a pointer to a dispatcher object of type thread.

    Process - Supplies a pointer to a dispatcher object of type process.

    OldIrql - Supplies the previous IRQL.

    SavedApcState - Supplies a pointer to the APC state structure that receives
        the saved APC state.

Return Value:

    None.

--*/

{

    PRKTHREAD OutThread;
    KAFFINITY Processor;
    PLIST_ENTRY NextEntry;
    KIRQL HighIrql;

    ASSERT(Process != Thread->ApcState.Process);

    //
    // Bias the stack count of the target process to signify that a
    // thread exists in that process with a stack that is resident.
    //

    Process->StackCount += 1;

    //
    // Save current APC state and initialize a new APC state.
    //

    KiMoveApcState(&Thread->ApcState, SavedApcState);
    InitializeListHead(&Thread->ApcState.ApcListHead[KernelMode]);
    InitializeListHead(&Thread->ApcState.ApcListHead[UserMode]);
    Thread->ApcState.Process = Process;
    Thread->ApcState.KernelApcInProgress = FALSE;
    Thread->ApcState.KernelApcPending = FALSE;
    Thread->ApcState.UserApcPending = FALSE;
    if (SavedApcState == &Thread->SavedApcState) {
        Thread->ApcStatePointer[0] = &Thread->SavedApcState;
        Thread->ApcStatePointer[1] = &Thread->ApcState;
        Thread->ApcStateIndex = 1;
    }

    //
    // If the target process is in memory, then immediately enter the
    // new address space by loading a new Directory Table Base. Otherwise,
    // insert the current thread in the target process ready list, inswap
    // the target process if necessary, select a new thread to run on the
    // the current processor and context switch to the new thread.
    //

    if (Process->State == ProcessInMemory) {

        //
        // It is possible that the process is in memory, but there exist
        // threads in the process ready list. This can happen when memory
        // management forces a process attach.
        //

        NextEntry = Process->ReadyListHead.Flink;
        while (NextEntry != &Process->ReadyListHead) {
            OutThread = CONTAINING_RECORD(NextEntry, KTHREAD, WaitListEntry);
            RemoveEntryList(NextEntry);
            OutThread->ProcessReadyQueue = FALSE;
            KiReadyThread(OutThread);
            NextEntry = Process->ReadyListHead.Flink;
        }

        KiSwapProcess(Process, SavedApcState->Process);
        KiUnlockDispatcherDatabase(OldIrql);

    } else {
        Thread->State = Ready;
        Thread->ProcessReadyQueue = TRUE;
        InsertTailList(&Process->ReadyListHead, &Thread->WaitListEntry);
        if (Process->State == ProcessOutOfMemory) {
            Process->State = ProcessInTransition;
            InsertTailList(&KiProcessInSwapListHead, &Process->SwapListEntry);
            KiSwapEvent.Header.SignalState = 1;
            if (IsListEmpty(&KiSwapEvent.Header.WaitListHead) == FALSE) {
                KiWaitTest(&KiSwapEvent, BALANCE_INCREMENT);
            }
        }

        //
        // Clear the active processor bit in the previous process and
        // set active processor bit in the process being attached to.
        //

#if !defined(NT_UP)

        KiLockContextSwap(&HighIrql);
        Processor = KeGetCurrentPrcb()->SetMember;
        SavedApcState->Process->ActiveProcessors &= ~Processor;
        Process->ActiveProcessors |= Processor;

#if defined(_ALPHA_)

        Process->RunOnProcessors |= Processor;

#endif

        KiUnlockContextSwap(HighIrql);

#endif

        Thread->WaitIrql = OldIrql;
        KiSwapThread();
    }

    return;
}

VOID
KiMoveApcState (
    IN PKAPC_STATE Source,
    OUT PKAPC_STATE Destination
    )

/*++

Routine Description:

    This function moves the APC state from the source structure to the
    destination structure and reinitializes list headers as appropriate.

Arguments:

    Source - Supplies a pointer to the source APC state structure.

    Destination - Supplies a pointer to the destination APC state structure.


Return Value:

    None.

--*/

{

    PLIST_ENTRY First;
    PLIST_ENTRY Last;

    //
    // Copy the APC state from the source to the destination.
    //

    *Destination = *Source;
    if (IsListEmpty(&Source->ApcListHead[KernelMode]) != FALSE) {
        InitializeListHead(&Destination->ApcListHead[KernelMode]);

    } else {
        First = Source->ApcListHead[KernelMode].Flink;
        Last = Source->ApcListHead[KernelMode].Blink;
        Destination->ApcListHead[KernelMode].Flink = First;
        Destination->ApcListHead[KernelMode].Blink = Last;
        First->Blink = &Destination->ApcListHead[KernelMode];
        Last->Flink = &Destination->ApcListHead[KernelMode];
    }

    if (IsListEmpty(&Source->ApcListHead[UserMode]) != FALSE) {
        InitializeListHead(&Destination->ApcListHead[UserMode]);

    } else {
        First = Source->ApcListHead[UserMode].Flink;
        Last = Source->ApcListHead[UserMode].Blink;
        Destination->ApcListHead[UserMode].Flink = First;
        Destination->ApcListHead[UserMode].Blink = Last;
        First->Blink = &Destination->ApcListHead[UserMode];
        Last->Flink = &Destination->ApcListHead[UserMode];
    }

    return;
}
