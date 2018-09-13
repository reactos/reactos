
/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    worker.c

Abstract:

    This module implements a worker thread and a set of functions for
    passing work to it.

Author:

    Steve Wood (stevewo) 25-Jul-1991


Revision History:

--*/

#include "exp.h"

//
// Define balance set wait object types.
//

typedef enum _BALANCE_OBJECT {
    TimerExpiration,
    ThreadSetManagerEvent,
    MaximumBalanceObject
} BALANCE_OBJECT;

//
// If this assertion fails then we must supply our own array of wait blocks.
//

C_ASSERT(MaximumBalanceObject < THREAD_WAIT_OBJECTS);

//
// Define priorities for delayed and critical worker threads. Note that these do not run
// at realtime. They run at csrss and below csrss to avoid pre-empting the
// user interface under heavy load.
//

#define DELAYED_WORK_QUEUE_PRIORITY         (12 - NORMAL_BASE_PRIORITY)
#define CRITICAL_WORK_QUEUE_PRIORITY        (13 - NORMAL_BASE_PRIORITY)
#define HYPER_CRITICAL_WORK_QUEUE_PRIORITY  (15 - NORMAL_BASE_PRIORITY)

//
// Number of worker threads to create for each type of system.
//

#define MAX_ADDITIONAL_THREADS 16
#define MAX_ADDITIONAL_DYNAMIC_THREADS 16

#define SMALL_NUMBER_OF_THREADS 2
#define MEDIUM_NUMBER_OF_THREADS 3
#define LARGE_NUMBER_OF_THREADS 5

//
// 10-minute timeout used for terminating dynamic work item worker threads.
//

#define DYNAMIC_THREAD_TIMEOUT ((LONGLONG)10 * 60 * 1000 * 1000 * 10)

//
// 1-second timeout used for waking up the worker thread set manager.
//

#define THREAD_SET_INTERVAL (1 * 1000 * 1000 * 10)

//
// Flag to pass in to the worker thread, indicating whether it is dynamic
// or not. 
//

#define DYNAMIC_WORKER_THREAD 0x80000000

//
// Per-queue dynamic thread state.
//

EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];

//
// Additional worker threads... Controlled using registry settings
//

ULONG ExpAdditionalCriticalWorkerThreads;
ULONG ExpAdditionalDelayedWorkerThreads;

ULONG ExCriticalWorkerThreads;
ULONG ExDelayedWorkerThreads;

//
// Global event to wake up the thread set manager.
//

KEVENT ExThreadSetManagerEvent;

VOID
ExpCheckDynamicThreadCount( VOID );

NTSTATUS
ExpCreateWorkerThread(
    WORK_QUEUE_TYPE QueueType,
    BOOLEAN Dynamic
    );
    
VOID
ExpDetectWorkerThreadDeadlock( VOID );

VOID
ExpWorkerThreadBalanceManager(
    IN PVOID StartContext
    );
    
//
// Procedure prototype for the worker thread.
//

VOID
ExpWorkerThread(
    IN PVOID StartContext
    );

#if DBG

EXCEPTION_DISPOSITION
ExpWorkerThreadFilter(
    IN PWORKER_THREAD_ROUTINE WorkerRoutine,
    IN PVOID Parameter,
    IN PEXCEPTION_POINTERS ExceptionInfo
    );

#endif

PVOID
ExpCheckForWorker(
    IN PVOID p,
    IN ULONG Size
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpWorkerInitialization)
#pragma alloc_text(PAGE, ExpCheckDynamicThreadCount)
#pragma alloc_text(PAGE, ExpCreateWorkerThread)
#pragma alloc_text(PAGE, ExpDetectWorkerThreadDeadlock)
#pragma alloc_text(PAGE, ExpWorkerThreadBalanceManager)
#endif


BOOLEAN
__inline
ExpNewThreadNecessary(
    IN WORK_QUEUE_TYPE QueueType
    )

/*++

Routine Description:

    This function checks the supplied worker queue and determines whether
    it is appropriate to spin up a dynamic worker thread for that queue.

Arguments:

    QueueType - Supplies the type of the queue that should be examined.

Return Value:

    TRUE if the given work queue would benefit from the creation of an
    additional thread, FALSE if not.

--*/
{
    PEX_WORK_QUEUE Queue;

    Queue = &ExWorkerQueue[QueueType];

    if (Queue->MakeThreadsAsNecessary != FALSE &&
        IsListEmpty( &Queue->WorkerQueue.EntryListHead ) == FALSE &&
        Queue->WorkerQueue.CurrentCount < Queue->WorkerQueue.MaximumCount &&
        Queue->DynamicThreadCount < MAX_ADDITIONAL_DYNAMIC_THREADS) {

        //
        // We know these things:
        //
        // - This queue is eligible for dynamic creation of threads to try
        //   to keep the CPUs busy,
        //
        // - There are work items waiting in the queue,
        //
        // - The number of runable worker threads for this queue is less than
        //   the number of processors on this system, and
        //
        // - We haven't reached the maximum dynamic thread count.
        //
        // An additional worker thread at this point will help clear the
        // backlog.
        //

        return TRUE;

    } else {

        //
        // One of the above conditions is false.
        //

        return FALSE;
    }
}

BOOLEAN
ExpWorkerInitialization(
    VOID
    )

{

    ULONG Index;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG NumberOfDelayedThreads;
    ULONG NumberOfCriticalThreads;
    NTSTATUS Status;
    HANDLE Thread;
    BOOLEAN NtAs;
    WORK_QUEUE_TYPE WorkQueueType;

    //
    // Set the number of worker threads based on the system size.
    //

    NtAs = MmIsThisAnNtAsSystem();
    switch (MmQuerySystemSize()) {
    case MmSmallSystem:
        NumberOfDelayedThreads = MEDIUM_NUMBER_OF_THREADS;
        if (MmNumberOfPhysicalPages > ((12*1024*1024)/PAGE_SIZE) ) {
            NumberOfCriticalThreads = MEDIUM_NUMBER_OF_THREADS;
        } else {
            NumberOfCriticalThreads = SMALL_NUMBER_OF_THREADS;
        }
        break;

    case MmMediumSystem:
        NumberOfDelayedThreads = MEDIUM_NUMBER_OF_THREADS;
        NumberOfCriticalThreads = MEDIUM_NUMBER_OF_THREADS;
        if ( NtAs ) {
            NumberOfCriticalThreads += MEDIUM_NUMBER_OF_THREADS;
            }
        break;

    case MmLargeSystem:
        NumberOfDelayedThreads = MEDIUM_NUMBER_OF_THREADS;
        NumberOfCriticalThreads = LARGE_NUMBER_OF_THREADS;
        if ( NtAs ) {
            NumberOfCriticalThreads += LARGE_NUMBER_OF_THREADS;
            }
        break;

    default:
        NumberOfDelayedThreads = SMALL_NUMBER_OF_THREADS;
        NumberOfCriticalThreads = SMALL_NUMBER_OF_THREADS;
    }


    //
    // Initialize the work Queue objects.
    //

    if ( ExpAdditionalCriticalWorkerThreads > MAX_ADDITIONAL_THREADS ) {
        ExpAdditionalCriticalWorkerThreads = MAX_ADDITIONAL_THREADS;
        }

    if ( ExpAdditionalDelayedWorkerThreads > MAX_ADDITIONAL_THREADS ) {
        ExpAdditionalDelayedWorkerThreads = MAX_ADDITIONAL_THREADS;
        }

    //
    // Initialize the ExWorkerQueue[] array.
    // 

    for (WorkQueueType = 0; WorkQueueType < MaximumWorkQueue; WorkQueueType++) {

        RtlZeroMemory(&ExWorkerQueue[WorkQueueType],
                      sizeof(EX_WORK_QUEUE));

        KeInitializeQueue(&ExWorkerQueue[WorkQueueType].WorkerQueue, 0);
    }

    //
    // We only create dynamic threads for the critical work queue (note
    // this doesn't apply to dynamic threads created to break deadlocks.)
    //
    // The rationale is this: folks who use the delayed work queue are
    // not time critical, and the hypercritical queue is used rarely
    // by folks who are non-blocking.
    //

    ExWorkerQueue[CriticalWorkQueue].MakeThreadsAsNecessary = TRUE;

    //
    // Initialize the global thread set manager event.
    //

    KeInitializeEvent(&ExThreadSetManagerEvent,
                      SynchronizationEvent,
                      FALSE);

    //
    // Create the desired number of executive worker threads for each
    // of the work queues.
    //

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    //
    // Create any builtin critical and delayed worker threads.
    //

    for (Index = 0; Index < (NumberOfCriticalThreads + ExpAdditionalCriticalWorkerThreads); Index += 1) {

        //
        // Create a worker thread to service the critical work queue.
        //

        Status = ExpCreateWorkerThread( CriticalWorkQueue, FALSE );
        if (!NT_SUCCESS(Status)) {
            break;
        }
        ExCriticalWorkerThreads++;
    }


    for (Index = 0; Index < (NumberOfDelayedThreads + ExpAdditionalDelayedWorkerThreads); Index += 1) {

        //
        // Create a worker thread to service the delayed work queue.
        //

        Status = ExpCreateWorkerThread( DelayedWorkQueue, FALSE );
        if (!NT_SUCCESS(Status)) {
            break;
        }

        ExDelayedWorkerThreads++;
    }

    Status = ExpCreateWorkerThread( HyperCriticalWorkQueue, FALSE );

    //
    // Create the worker thread set manager thread.
    //

    Status = PsCreateSystemThread(&Thread,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  0L,
                                  NULL,
                                  ExpWorkerThreadBalanceManager,
                                  NULL);
    if (NT_SUCCESS(Status)) {
        ZwClose( Thread );
    }

    return (BOOLEAN)NT_SUCCESS(Status);
}

VOID
ExQueueWorkItem(
    IN PWORK_QUEUE_ITEM WorkItem,
    IN WORK_QUEUE_TYPE QueueType
    )

/*++

Routine Description:

    This function inserts a work item into a work queue that is processed
    by a worker thread of the corresponding type.

Arguments:

    WorkItem - Supplies a pointer to the work item to add the the queue.
        This structure must be located in NonPagedPool. The work item
        structure contains a doubly linked list entry, the address of a
        routine to call and a parameter to pass to that routine.

    QueueType - Specifies the type of work queue that the work item
        should be placed in.

Return Value:

    None

--*/

{

    ASSERT(QueueType < MaximumWorkQueue);
    ASSERT(WorkItem->List.Flink == NULL);

    //
    // Insert the work item in the appropriate queue object.
    //

    KeInsertQueue(&ExWorkerQueue[QueueType].WorkerQueue, &WorkItem->List);

    //
    // Determine whether another thread should be created, and signal the
    // thread set balance manager if so.
    //

    if (ExpNewThreadNecessary(QueueType) != FALSE) {
        
        KeSetEvent( &ExThreadSetManagerEvent,
                    0,
                    FALSE );
    }

    return;
}


VOID
ExpWorkerThreadBalanceManager(
    IN PVOID StartContext
    )

/*++

Routine Description:

    This function is the startup code for the worker thread manager thread.
    The worker thread manager thread is created during system initialization
    and begins execution in this function.

    This thread is responsible for detecting and breaking circular deadlocks
    in the system worker thread queues.  It will also create and destroy
    additional worker threads as needed based on loading.

Arguments:

    Context - Supplies a pointer to an arbitrary data structure (NULL).

Return Value:

    None.

--*/
{
    KTIMER PeriodTimer;
    LARGE_INTEGER DueTime;
    PVOID WaitObjects[MaximumBalanceObject];
    NTSTATUS Status;

    PAGED_CODE();

    //
    // Raise the thread priority to just higher than the priority of the
    // critical work queue.
    //

    KeSetBasePriorityThread(KeGetCurrentThread(),
                            CRITICAL_WORK_QUEUE_PRIORITY+1);

    //
    // Initialize the periodic timer and set the manager period.
    //

    KeInitializeTimer(&PeriodTimer);
    DueTime.QuadPart = - THREAD_SET_INTERVAL;

    //
    // Initialize the wait object array.
    //

    WaitObjects[TimerExpiration] = (PVOID)&PeriodTimer;
    WaitObjects[ThreadSetManagerEvent] = (PVOID)&ExThreadSetManagerEvent;

    //
    // Loop forever processing events.
    //

    while( TRUE ) {

        //
        // Set the timer to expire at the next periodic interval.
        //

        KeSetTimer(&PeriodTimer, DueTime, NULL);

        //
        // Wake up when the timer expires or the set manager event is
        // signalled.
        //

        Status = KeWaitForMultipleObjects(MaximumBalanceObject,
                                          WaitObjects,
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          NULL);

        switch (Status) {

            case TimerExpiration:

                //
                // Periodic timer expiration - go see if any work queues
                // are deadlocked.
                //
        
                ExpDetectWorkerThreadDeadlock();
                break;

            case ThreadSetManagerEvent:

                //
                // Someone has asked us to check some metrics to determine
                // whether we should create another worker thread.
                //

                ExpCheckDynamicThreadCount();
                break;
        }
    }
}

VOID
ExpWorkerThread(
    IN PVOID StartContext
    )

{

    PLIST_ENTRY Entry;
    WORK_QUEUE_TYPE QueueType;
    PWORK_QUEUE_ITEM WorkItem;
    KPROCESSOR_MODE WaitMode;
    LARGE_INTEGER TimeoutValue;
    PLARGE_INTEGER Timeout;
    PETHREAD Thread;
    BOOLEAN DynamicThread;
    PEX_WORK_QUEUE WorkerQueue;
    PVOID WorkerRoutine;
    PVOID Parameter;

    WaitMode = UserMode;

    //
    // Set timeout value etc according to whether we are static or dynamic.
    //

    if (((ULONG_PTR)StartContext & DYNAMIC_WORKER_THREAD) == 0) {

        //
        // We are being created as a static thread.  As such it will not
        // terminate, so there is no point in timing out waiting for a work
        // item.
        //

        Timeout = NULL;

    } else {

        //
        // This is a dynamic worker thread.  It has a non-infinite timeout
        // so that it can eventually terminate.
        //

        TimeoutValue.QuadPart = -DYNAMIC_THREAD_TIMEOUT;
        Timeout = &TimeoutValue;
    }

    Thread = PsGetCurrentThread();

    //
    // If the thread is a critical worker thread, then set the thread
    // priority to the lowest realtime level. Otherwise, set the base
    // thread priority to time critical.
    //

    QueueType = (WORK_QUEUE_TYPE)
                ((ULONG_PTR)StartContext & ~DYNAMIC_WORKER_THREAD);

    WorkerQueue = &ExWorkerQueue[QueueType];

    switch ( QueueType ) {

        case HyperCriticalWorkQueue:

            //
            // Always make stack for this thread resident
            // so that worker pool deadlock magic can run
            // even when what we are trying to do is inpage
            // the hyper critical worker thread's stack.
            // Without this fix, we hold the process lock
            // but this thread's stack can't come in, and
            // the deadlock detection cannot create new threads
            // to break the system deadlock.
            //

            WaitMode = KernelMode;
            break;

        case CriticalWorkQueue:
            if ( MmIsThisAnNtAsSystem() ) {
                WaitMode = KernelMode;
                }

            break;
    }

#if defined(REMOTE_BOOT)
    //
    // In diskless NT scenarios ensure that the kernel stack of the worker
    // threads will not be swapped out.
    //

    if (IoRemoteBootClient) {
        KeSetKernelStackSwapEnable(FALSE);
    }
#endif // defined(REMOTE_BOOT)

    //
    // Loop forever waiting for a work queue item, calling the processing
    // routine, and then waiting for another work queue item.
    //

    do {

        while (TRUE) {

            //
            // Wait until something is put in the queue or until we time out.
            //
            // By specifying a wait mode of UserMode, the thread's kernel
            // stack is swappable.
            //

            Entry = KeRemoveQueue(&WorkerQueue->WorkerQueue,
                                  WaitMode,
                                  Timeout);
            if ((ULONG_PTR)Entry != STATUS_TIMEOUT) {

                //
                // This is a real work item, break out of the timeout loop
                // and go process.
                //

                break;
            }

            // 
            // These things are known:
            //
            // - Static worker threads do not time out, so this is a dynamic
            //   worker thread.
            //
            // - This thread has been waiting for a long time with nothing
            //   to do.
            //

            if (IsListEmpty( &Thread->IrpList ) == FALSE) {

                //
                // There is still I/O pending, can't terminate yet.
                //

                continue;
            }

            //
            // This dynamic thread can be terminated.
            //

#if DBG
            DbgPrint("EXWORKER: Dynamic type %d thread no longer needed,"
                     " terminating.\n",
                     QueueType );
#endif

            InterlockedDecrement(
                &WorkerQueue->DynamicThreadCount );

#if defined(REMOTE_BOOT)
            //
            // We will bugcheck if we terminate a thread with stack swapping
            // disabled.
            //

            if (IoRemoteBootClient) {
                KeSetKernelStackSwapEnable(TRUE);
            }
#endif // defined(REMOTE_BOOT)

            return;
        }

        //
        // Update the total number of work items processed.
        // 

        InterlockedIncrement( &WorkerQueue->WorkItemsProcessed );

        WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);
        WorkerRoutine = WorkItem->WorkerRoutine;
        Parameter = WorkItem->Parameter;

        //
        // Execute the specified routine.
        //

#if DBG

        try {

            ((PWORKER_THREAD_ROUTINE)WorkerRoutine)(Parameter);

            if (KeGetCurrentIrql() != 0) {
                DbgPrint("EXWORKER: worker exit at IRQL %d, worker routine %x, "
                        "parameter %x, item %x\n",
                        KeGetCurrentIrql(), WorkerRoutine, Parameter, WorkItem);

                DbgBreakPoint();
            }

            //
            // Catch worker routines that forget to do KeLeaveCriticalRegion.
            //
            if (Thread->Tcb.KernelApcDisable != 0) {
                DbgPrint("EXWORKER: worker exit with APCs disabled, worker routine %x, "
                        "parameter %x, item %x\n",
                        WorkerRoutine, Parameter, WorkItem);

                DbgBreakPoint();
                Thread->Tcb.KernelApcDisable = 0;
            }

            if (Thread->ActiveImpersonationInfo) {
                KeBugCheckEx(
                    IMPERSONATING_WORKER_THREAD,
                    (ULONG_PTR)WorkerRoutine,
                    (ULONG_PTR)Parameter,
                    (ULONG_PTR)WorkItem,
                    0);
            }

        } except( ExpWorkerThreadFilter(WorkerRoutine,
                                        Parameter,
                                        GetExceptionInformation() )) {
        }

#else

        ((PWORKER_THREAD_ROUTINE)WorkerRoutine)(Parameter);

        //
        // Catch worker routines that forget to do KeLeaveCriticalRegion.
        // It has to be zero at this point. In the debug case we enter a 
        // breakpoint. In the non-debug case just zero the flag so that
        // APCs can continue to fire to this thread.
        //

        if (Thread->Tcb.KernelApcDisable != 0) {
            DbgPrint("EXWORKER: worker exit with APCs disabled, worker routine %x, "
                    "parameter %x, item %x\n",
                    WorkerRoutine, Parameter, WorkItem);

            Thread->Tcb.KernelApcDisable = 0;
        }

        if (KeGetCurrentIrql() != 0) {
            KeBugCheckEx(
                WORKER_THREAD_RETURNED_AT_BAD_IRQL,
                (ULONG_PTR)WorkerRoutine,
                (ULONG_PTR)KeGetCurrentIrql(),
                (ULONG_PTR)Parameter,
                (ULONG_PTR)WorkItem
                );
            }

        if (Thread->ActiveImpersonationInfo) {
            KeBugCheckEx(
                IMPERSONATING_WORKER_THREAD,
                (ULONG_PTR)WorkerRoutine,
                (ULONG_PTR)Parameter,
                (ULONG_PTR)WorkItem,
                0
                );
        }
#endif

    } while(TRUE);
}

VOID
ExpCheckDynamicThreadCount( VOID )

/*++

Routine Description:

    This routine is called when there is reason to believe that a work queue
    might benefit from the creation of an additional worker thread.

    This routine checks each queue to determine whether it would benefit from
    an additional worker thread (see ExpNewThreadNecessary()), and creates
    one if so.

Arguments:

    None.

Return Value:

    None.

--*/
{
    WORK_QUEUE_TYPE QueueType;

    PAGED_CODE();

    //
    // Check each worker queue.
    //

    for (QueueType = 0; QueueType < MaximumWorkQueue; QueueType++) {

        if (ExpNewThreadNecessary(QueueType)) {

            //
            // Create a new thread for this queue.  We explicitly ignore
            // an error from ExpCreateDynamicThread(): there's nothing
            // we can or should do in the event of a failure.
            //

            ExpCreateWorkerThread(QueueType, TRUE);
        }
    }
}

VOID
ExpDetectWorkerThreadDeadlock( VOID )

/*++

Routine Description:

    This function creates new work item threads if a possible deadlock is
    detected.

Arguments:

    None.

Return Value:

    None

--*/

{
    LONG QueueDepth;
    ULONG Index;
    LONG ThreadCount;
    LONG NewThreadCount;
    NTSTATUS Status;
    HANDLE Thread;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PEX_WORK_QUEUE Queue;

    PAGED_CODE();

    //
    // Process each queue type.
    // 

    for (Index = 0; Index < MaximumWorkQueue; Index += 1) {

        Queue = &ExWorkerQueue[Index];

        ASSERT( Queue->DynamicThreadCount <=
                MAX_ADDITIONAL_DYNAMIC_THREADS );

        if (Queue->QueueDepthLastPass > 0 &&

            Queue->WorkItemsProcessed ==
                Queue->WorkItemsProcessedLastPass &&

            Queue->DynamicThreadCount <
                MAX_ADDITIONAL_DYNAMIC_THREADS) {

            //
            // These things are known:
            //
            // - There were work items waiting in the queue at the last pass.
            // - No work items have been processed since the last pass.
            // - We haven't yet created the maximum number of dynamic threads.
            //
            // Things look like they're stuck, create a new thread.
            //

#if DBG
            DbgPrint("EXWORKER: Work item deadlock detected, creating "
                     "type %d worker thread\n",
                     Index );
#endif

            //
            // Create a new thread for this queue.  We explicitly ignore
            // an error from ExpCreateDynamicThread(): we'll try again in
            // another detection period if the queue looks like it's still
            // stuck.
            //

            ExpCreateWorkerThread(Index, TRUE);
        }

        //
        // Update some bookkeeping.
        //
        // Note that WorkItemsProcessed and the queue depth must be recorded
        // in that order to avoid getting a false deadlock indication.
        //

        Queue->WorkItemsProcessedLastPass = Queue->WorkItemsProcessed;
        Queue->QueueDepthLastPass = KeReadStateQueue( &Queue->WorkerQueue );
    }
}

#if DBG

EXCEPTION_DISPOSITION
ExpWorkerThreadFilter(
    IN PWORKER_THREAD_ROUTINE WorkerRoutine,
    IN PVOID Parameter,
    IN PEXCEPTION_POINTERS ExceptionInfo
    )
{
    KdPrint(("EXWORKER: exception in worker routine %p(%p)\n", WorkerRoutine, Parameter));
    KdPrint(("  exception record at %p\n", ExceptionInfo->ExceptionRecord));
    KdPrint(("  context record at %p\n",ExceptionInfo->ContextRecord));

    try {
        DbgBreakPoint();

    } except (EXCEPTION_EXECUTE_HANDLER) {
        //
        // No kernel debugger attached, so let the system thread
        // exception handler call KeBugCheckEx.
        //
        return(EXCEPTION_CONTINUE_SEARCH);
    }

    return(EXCEPTION_EXECUTE_HANDLER);
}

#endif

NTSTATUS
ExpCreateWorkerThread(
    WORK_QUEUE_TYPE QueueType,
    BOOLEAN Dynamic
    )

/*++

Routine Description:

    This function creates a single new static or dynamic worker thread for
    the given queue type.

Arguments:

    QueueType - Supplies the type of the queue for which the worker thread
                should be created.

    Dynamic - If TRUE, the worker thread is created as a dynamic thread that
              will terminate after a sufficient period of inactivity.  If FALSE,
              the worker thread will never terminate.


Return Value:

    The final status of the operation.

Notes:

    This routine is only called from the worker thread set balance thread,
    therefore it will not be reentered.

--*/

{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    ULONG Context;
    ULONG BasePriority;
    PETHREAD Thread;

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    Context = QueueType;
    if (Dynamic != FALSE) {
        Context |= DYNAMIC_WORKER_THREAD;
    }

    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  0L,
                                  NULL,
                                  ExpWorkerThread,
                                  (PVOID)Context);
    if (!NT_SUCCESS(Status)) {
#if DBG
        DbgPrint("EXWORKER: Worker thread creation failed, status %08x\n",
                 Status);
#endif
        return Status;
    }

    if (Dynamic != FALSE) {

#if DBG
        DbgPrint("EXWORKER: Created dynamic thread type %d, %d total\n",
                 QueueType,
                 ExWorkerQueue[QueueType].DynamicThreadCount);
#endif

        InterlockedIncrement( &ExWorkerQueue[QueueType].DynamicThreadCount );
    }

    //
    // Set the priority according to the type of worker thread.
    //

    switch (QueueType) {
   
        case HyperCriticalWorkQueue:

            BasePriority = HYPER_CRITICAL_WORK_QUEUE_PRIORITY;
            break;

        case CriticalWorkQueue:

            BasePriority = CRITICAL_WORK_QUEUE_PRIORITY;
            break;

        case DelayedWorkQueue:

            BasePriority = DELAYED_WORK_QUEUE_PRIORITY;
            break;
    }

    //
    // Set the base priority of the just-created thread.
    // 

    Status = ObReferenceObjectByHandle( ThreadHandle,
                                        THREAD_SET_INFORMATION,
                                        PsThreadType,
                                        KernelMode,
                                        (PVOID *)&Thread,
                                        NULL );
    if (NT_SUCCESS(Status)) {

        KeSetBasePriorityThread( &Thread->Tcb, BasePriority );
        ObDereferenceObject( Thread );

    } else {

        //
        // The thread was created but we were unable to reference it.  This is
        // very odd... just leave it at the default priority, better than no
        // thread at all.
        //

    }

    ZwClose( ThreadHandle );
    return Status;
}

PVOID
ExpCheckForWorker(
    IN PVOID p,
    IN ULONG Size
    )

{
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    PCHAR BeginBlock;
    PCHAR EndBlock;
    WORK_QUEUE_TYPE wqt;

    BeginBlock = (PCHAR)p;
    EndBlock = (PCHAR)p + Size;

    KiLockDispatcherDatabase (&OldIrql);
    for (wqt = CriticalWorkQueue; wqt < MaximumWorkQueue; wqt++) {
        for (Entry = (PLIST_ENTRY) ExWorkerQueue[wqt].WorkerQueue.EntryListHead.Flink;
             Entry && (Entry != (PLIST_ENTRY) &ExWorkerQueue[wqt].WorkerQueue.EntryListHead);
             Entry = Entry->Flink) {
           if (((PCHAR) Entry >= BeginBlock) && ((PCHAR) Entry < EndBlock)) {
              KeBugCheckEx(WORKER_INVALID,
                           0x0,
                           (ULONG_PTR)Entry,
                           (ULONG_PTR)BeginBlock,
                           (ULONG_PTR)EndBlock);
 
           }
        }
    }
    KiUnlockDispatcherDatabase (OldIrql);

    return NULL;
}