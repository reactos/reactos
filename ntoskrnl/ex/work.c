/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS Kernel
 * FILE:               ntoskrnl/ex/work.c
 * PURPOSE:            Manage system work queues and worker threads
 * PROGRAMMER:         Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* DATA **********************************************************************/

/* Number of worker threads for each Queue */
#define EX_HYPERCRITICAL_WORK_THREADS               1
#define EX_DELAYED_WORK_THREADS                     3
#define EX_CRITICAL_WORK_THREADS                    5

/* Magic flag for dynamic worker threads */
#define EX_DYNAMIC_WORK_THREAD                      0x80000000

/* Worker thread priority increments (added to base priority) */
#define EX_HYPERCRITICAL_QUEUE_PRIORITY_INCREMENT   7
#define EX_CRITICAL_QUEUE_PRIORITY_INCREMENT        5
#define EX_DELAYED_QUEUE_PRIORITY_INCREMENT         4

/* The actual worker queue array */
EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];

/* Accounting of the total threads and registry hacked threads */
ULONG ExCriticalWorkerThreads;
ULONG ExDelayedWorkerThreads;
ULONG ExpAdditionalCriticalWorkerThreads;
ULONG ExpAdditionalDelayedWorkerThreads;

/* Future support for stack swapping worker threads */
BOOLEAN ExpWorkersCanSwap;
LIST_ENTRY ExpWorkerListHead;
FAST_MUTEX ExpWorkerSwapinMutex;

/* The worker balance set manager events */
KEVENT ExpThreadSetManagerEvent;
KEVENT ExpThreadSetManagerShutdownEvent;

/* Thread pointers for future worker thread shutdown support */
PETHREAD ExpWorkerThreadBalanceManagerPtr;
PETHREAD ExpLastWorkerThread;

/* PRIVATE FUNCTIONS *********************************************************/

/*++
 * @name ExpWorkerThreadEntryPoint
 *
 *     The ExpWorkerThreadEntryPoint routine is the entrypoint for any new
 *     worker thread created by teh system.
 *
 * @param Context
 *        Contains the work queue type masked with a flag specifing whether the
 *        thread is dynamic or not.
 *
 * @return None.
 *
 * @remarks A dynamic thread can timeout after 10 minutes of waiting on a queue
 *          while a static thread will never timeout.
 *
 *          Worker threads must return at IRQL == PASSIVE_LEVEL, must not have
 *          active impersonation info, and must not have disabled APCs.
 *
 *          NB: We will re-enable APCs for broken threads but all other cases
 *              will generate a bugcheck.
 *
 *--*/
VOID
NTAPI
ExpWorkerThreadEntryPoint(IN PVOID Context)
{
    PWORK_QUEUE_ITEM WorkItem;
    PLIST_ENTRY QueueEntry;
    WORK_QUEUE_TYPE WorkQueueType;
    PEX_WORK_QUEUE WorkQueue;
    LARGE_INTEGER Timeout;
    PLARGE_INTEGER TimeoutPointer = NULL;
    PETHREAD Thread = PsGetCurrentThread();
    KPROCESSOR_MODE WaitMode;
    EX_QUEUE_WORKER_INFO OldValue, NewValue;

    /* Check if this is a dyamic thread */
    if ((ULONG_PTR)Context & EX_DYNAMIC_WORK_THREAD)
    {
        /* It is, which means we will eventually time out after 10 minutes */
        Timeout.QuadPart = Int32x32To64(10, -10000000 * 60);
        TimeoutPointer = &Timeout;
    }

    /* Get Queue Type and Worker Queue */
    WorkQueueType = (WORK_QUEUE_TYPE)((ULONG_PTR)Context &
                                      ~EX_DYNAMIC_WORK_THREAD);
    WorkQueue = &ExWorkerQueue[WorkQueueType];

    /* Select the wait mode */
    WaitMode = (UCHAR)WorkQueue->Info.WaitMode;

    /* Nobody should have initialized this yet, do it now */
    ASSERT(Thread->ExWorkerCanWaitUser == 0);
    if (WaitMode == UserMode) Thread->ExWorkerCanWaitUser = TRUE;

    /* If we shouldn't swap, disable that feature */
    if (!ExpWorkersCanSwap) KeSetKernelStackSwapEnable(FALSE);

    /* Set the worker flags */
    do
    {
        /* Check if the queue is being disabled */
        if (WorkQueue->Info.QueueDisabled)
        {
            /* Re-enable stack swapping and kill us */
            KeSetKernelStackSwapEnable(TRUE);
            PsTerminateSystemThread(STATUS_SYSTEM_SHUTDOWN);
        }

        /* Increase the worker count */
        OldValue = WorkQueue->Info;
        NewValue = OldValue;
        NewValue.WorkerCount++;
    }
    while (InterlockedCompareExchange((PLONG)&WorkQueue->Info,
                                      *(PLONG)&NewValue,
                                      *(PLONG)&OldValue) != *(PLONG)&OldValue);

    /* Success, you are now officially a worker thread! */
    Thread->ActiveExWorker = TRUE;

    /* Loop forever */
ProcessLoop:
    for (;;)
    {
        /* Wait for something to happen on the queue */
        QueueEntry = KeRemoveQueue(&WorkQueue->WorkerQueue,
                                   WaitMode,
                                   TimeoutPointer);

        /* Check if we timed out and quit this loop in that case */
        if ((NTSTATUS)(ULONG_PTR)QueueEntry == STATUS_TIMEOUT) break;

        /* Increment Processed Work Items */
        InterlockedIncrement((PLONG)&WorkQueue->WorkItemsProcessed);

        /* Get the Work Item */
        WorkItem = CONTAINING_RECORD(QueueEntry, WORK_QUEUE_ITEM, List);

        /* Make sure nobody is trying to play smart with us */
        ASSERT((ULONG_PTR)WorkItem->WorkerRoutine > MmUserProbeAddress);

        /* Call the Worker Routine */
        WorkItem->WorkerRoutine(WorkItem->Parameter);

        /* Make sure APCs are not disabled */
        if (Thread->Tcb.CombinedApcDisable != 0)
        {
            /* We're nice and do it behind your back */
            DPRINT1("Warning: Broken Worker Thread: %p %p %p came back "
                    "with APCs disabled!\n",
                    WorkItem->WorkerRoutine,
                    WorkItem->Parameter,
                    WorkItem);
            ASSERT(Thread->Tcb.CombinedApcDisable == 0);
            Thread->Tcb.CombinedApcDisable = 0;
        }

        /* Make sure it returned at right IRQL */
        if (KeGetCurrentIrql() != PASSIVE_LEVEL)
        {
            /* It didn't, bugcheck! */
            KeBugCheckEx(WORKER_THREAD_RETURNED_AT_BAD_IRQL,
                         (ULONG_PTR)WorkItem->WorkerRoutine,
                         KeGetCurrentIrql(),
                         (ULONG_PTR)WorkItem->Parameter,
                         (ULONG_PTR)WorkItem);
        }

        /* Make sure it returned with Impersionation Disabled */
        if (Thread->ActiveImpersonationInfo)
        {
            /* It didn't, bugcheck! */
            KeBugCheckEx(IMPERSONATING_WORKER_THREAD,
                         (ULONG_PTR)WorkItem->WorkerRoutine,
                         (ULONG_PTR)WorkItem->Parameter,
                         (ULONG_PTR)WorkItem,
                         0);
        }
    }

    /* This is a dynamic thread. Terminate it unless IRPs are pending */
    if (!IsListEmpty(&Thread->IrpList)) goto ProcessLoop;

    /* Don't terminate it if the queue is disabled either */
    if (WorkQueue->Info.QueueDisabled) goto ProcessLoop;

    /* Set the worker flags */
    do
    {
        /* Decrease the worker count */
        OldValue = WorkQueue->Info;
        NewValue = OldValue;
        NewValue.WorkerCount--;
    }
    while (InterlockedCompareExchange((PLONG)&WorkQueue->Info,
                                      *(PLONG)&NewValue,
                                      *(PLONG)&OldValue) != *(PLONG)&OldValue);

    /* Decrement dynamic thread count */
    InterlockedDecrement(&WorkQueue->DynamicThreadCount);

    /* We're not a worker thread anymore */
    Thread->ActiveExWorker = FALSE;

    /* Re-enable the stack swap */
    KeSetKernelStackSwapEnable(TRUE);
    return;
}

/*++
 * @name ExpCreateWorkerThread
 *
 *     The ExpCreateWorkerThread routine creates a new worker thread for the
 *     specified queue.
 *
 * @param QueueType
 *        Type of the queue to use for this thread. Valid values are:
 *          - DelayedWorkQueue
 *          - CriticalWorkQueue
 *          - HyperCriticalWorkQueue
 *
 * @param Dynamic
 *        Specifies whether or not this thread is a dynamic thread.
 *
 * @return None.
 *
 * @remarks HyperCritical work threads run at priority 7; Critical work threads
 *          run at priority 5, and delayed work threads run at priority 4.
 *
 *          This, worker threads cannot pre-empty a normal user-mode thread.
 *
 *--*/
VOID
NTAPI
ExpCreateWorkerThread(WORK_QUEUE_TYPE WorkQueueType,
                      IN BOOLEAN Dynamic)
{
    PETHREAD Thread;
    HANDLE hThread;
    ULONG Context;
    KPRIORITY Priority;

    /* Check if this is going to be a dynamic thread */
    Context = WorkQueueType;

    /* Add the dynamic mask */
    if (Dynamic) Context |= EX_DYNAMIC_WORK_THREAD;

    /* Create the System Thread */
    PsCreateSystemThread(&hThread,
                         THREAD_ALL_ACCESS,
                         NULL,
                         NULL,
                         NULL,
                         ExpWorkerThreadEntryPoint,
                         UlongToPtr(Context));

    /* If the thread is dynamic */
    if (Dynamic)
    {
        /* Increase the count */
        InterlockedIncrement(&ExWorkerQueue[WorkQueueType].DynamicThreadCount);
    }

    /* Set the priority */
    if (WorkQueueType == DelayedWorkQueue)
    {
        /* Priority == 4 */
        Priority = EX_DELAYED_QUEUE_PRIORITY_INCREMENT;
    }
    else if (WorkQueueType == CriticalWorkQueue)
    {
        /* Priority == 5 */
        Priority = EX_CRITICAL_QUEUE_PRIORITY_INCREMENT;
    }
    else
    {
        /* Priority == 7 */
        Priority = EX_HYPERCRITICAL_QUEUE_PRIORITY_INCREMENT;
    }

    /* Get the Thread */
    ObReferenceObjectByHandle(hThread,
                              THREAD_SET_INFORMATION,
                              PsThreadType,
                              KernelMode,
                              (PVOID*)&Thread,
                              NULL);

    /* Set the Priority */
    KeSetBasePriorityThread(&Thread->Tcb, Priority);

    /* Dereference and close handle */
    ObDereferenceObject(Thread);
    ObCloseHandle(hThread, KernelMode);
}

/*++
 * @name ExpDetectWorkerThreadDeadlock
 *
 *     The ExpDetectWorkerThreadDeadlock routine checks every queue and creates
 *     a dynamic thread if the queue seems to be deadlocked.
 *
 * @param None
 *
 * @return None.
 *
 * @remarks The algorithm for deciding if a new thread must be created is based
 *          on whether the queue has processed no new items in the last second,
 *          and new items are still enqueued.
 *
 *--*/
VOID
NTAPI
ExpDetectWorkerThreadDeadlock(VOID)
{
    ULONG i;
    PEX_WORK_QUEUE Queue;

    /* Loop the 3 queues */
    for (i = 0; i < MaximumWorkQueue; i++)
    {
        /* Get the queue */
        Queue = &ExWorkerQueue[i];
        ASSERT(Queue->DynamicThreadCount <= 16);

        /* Check if stuff is on the queue that still is unprocessed */
        if ((Queue->QueueDepthLastPass) &&
            (Queue->WorkItemsProcessed == Queue->WorkItemsProcessedLastPass) &&
            (Queue->DynamicThreadCount < 16))
        {
            /* Stuff is still on the queue and nobody did anything about it */
            DPRINT1("EX: Work Queue Deadlock detected: %lu\n", i);
            ExpCreateWorkerThread(i, TRUE);
            DPRINT1("Dynamic threads queued %d\n", Queue->DynamicThreadCount);
        }

        /* Update our data */
        Queue->WorkItemsProcessedLastPass = Queue->WorkItemsProcessed;
        Queue->QueueDepthLastPass = KeReadStateQueue(&Queue->WorkerQueue);
    }
}

/*++
 * @name ExpCheckDynamicThreadCount
 *
 *     The ExpCheckDynamicThreadCount routine checks every queue and creates
 *     a dynamic thread if the queue requires one.
 *
 * @param None
 *
 * @return None.
 *
 * @remarks The algorithm for deciding if a new thread must be created is
 *          documented in the ExQueueWorkItem routine.
 *
 *--*/
VOID
NTAPI
ExpCheckDynamicThreadCount(VOID)
{
    ULONG i;
    PEX_WORK_QUEUE Queue;

    /* Loop the 3 queues */
    for (i = 0; i < MaximumWorkQueue; i++)
    {
        /* Get the queue */
        Queue = &ExWorkerQueue[i];

        /* Check if still need a new thread. See ExQueueWorkItem */
        if ((Queue->Info.MakeThreadsAsNecessary) &&
            (!IsListEmpty(&Queue->WorkerQueue.EntryListHead)) &&
            (Queue->WorkerQueue.CurrentCount <
             Queue->WorkerQueue.MaximumCount) &&
            (Queue->DynamicThreadCount < 16))
        {
            /* Create a new thread */
            DPRINT1("EX: Creating new dynamic thread as requested\n");
            ExpCreateWorkerThread(i, TRUE);
        }
    }
}

/*++
 * @name ExpWorkerThreadBalanceManager
 *
 *     The ExpWorkerThreadBalanceManager routine is the entrypoint for the
 *     worker thread balance set manager.
 *
 * @param Context
 *        Unused.
 *
 * @return None.
 *
 * @remarks The worker thread balance set manager listens every second, but can
 *          also be woken up by an event when a new thread is needed, or by the
 *          special shutdown event. This thread runs at priority 7.
 *
 *          This routine must run at IRQL == PASSIVE_LEVEL.
 *
 *--*/
VOID
NTAPI
ExpWorkerThreadBalanceManager(IN PVOID Context)
{
    KTIMER Timer;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    PVOID WaitEvents[3];
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Context);

    /* Raise our priority above all other worker threads */
    KeSetBasePriorityThread(KeGetCurrentThread(),
                            EX_CRITICAL_QUEUE_PRIORITY_INCREMENT + 1);

    /* Setup the timer */
    KeInitializeTimer(&Timer);
    Timeout.QuadPart = Int32x32To64(-1, 10000000);

    /* We'll wait on the periodic timer and also the emergency event */
    WaitEvents[0] = &Timer;
    WaitEvents[1] = &ExpThreadSetManagerEvent;
    WaitEvents[2] = &ExpThreadSetManagerShutdownEvent;

    /* Start wait loop */
    for (;;)
    {
        /* Wait for the timer */
        KeSetTimer(&Timer, Timeout, NULL);
        Status = KeWaitForMultipleObjects(3,
                                          WaitEvents,
                                          WaitAny,
                                          Executive,
                                          KernelMode,
                                          FALSE,
                                          NULL,
                                          NULL);
        if (Status == 0)
        {
            /* Our timer expired. Check for deadlocks */
            ExpDetectWorkerThreadDeadlock();
        }
        else if (Status == 1)
        {
            /* Someone notified us, verify if we should create a new thread */
            ExpCheckDynamicThreadCount();
        }
        else if (Status == 2)
        {
            /* We are shutting down. Cancel the timer */
            DPRINT1("System shutdown\n");
            KeCancelTimer(&Timer);

            /* Make sure we have a final thread */
            ASSERT(ExpLastWorkerThread);

            /* Wait for it */
            KeWaitForSingleObject(ExpLastWorkerThread,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);

            /* Dereference it and kill us */
            ObDereferenceObject(ExpLastWorkerThread);
            PsTerminateSystemThread(STATUS_SYSTEM_SHUTDOWN);
        }

        /*
         * If WinDBG wants to attach or kill a user-mode process, and/or
         * page-in an address region, queue a debugger worker thread.
         */
        if (ExpDebuggerWork == WinKdWorkerStart)
        {
             ExInitializeWorkItem(&ExpDebuggerWorkItem, ExpDebuggerWorker, NULL);
             ExpDebuggerWork = WinKdWorkerInitialized;
             ExQueueWorkItem(&ExpDebuggerWorkItem, DelayedWorkQueue);
        }
    }
}

/*++
 * @name ExpInitializeWorkerThreads
 *
 *     The ExpInitializeWorkerThreads routine initializes worker thread and
 *     work queue support.
 *
 * @param None.
 *
 * @return None.
 *
 * @remarks This routine is only called once during system initialization.
 *
 *--*/
CODE_SEG("INIT")
VOID
NTAPI
ExpInitializeWorkerThreads(VOID)
{
    ULONG WorkQueueType;
    ULONG CriticalThreads, DelayedThreads;
    HANDLE ThreadHandle;
    PETHREAD Thread;
    ULONG i;

    /* Setup the stack swap support */
    ExInitializeFastMutex(&ExpWorkerSwapinMutex);
    InitializeListHead(&ExpWorkerListHead);
    ExpWorkersCanSwap = TRUE;

    /* Set the number of critical and delayed threads. We shouldn't hardcode */
    DelayedThreads = EX_DELAYED_WORK_THREADS;
    CriticalThreads = EX_CRITICAL_WORK_THREADS;

    /* Protect against greedy registry modifications */
    ExpAdditionalDelayedWorkerThreads =
        min(ExpAdditionalDelayedWorkerThreads, 16);
    ExpAdditionalCriticalWorkerThreads =
        min(ExpAdditionalCriticalWorkerThreads, 16);

    /* Calculate final count */
    DelayedThreads += ExpAdditionalDelayedWorkerThreads;
    CriticalThreads += ExpAdditionalCriticalWorkerThreads;

    /* Initialize the Array */
    for (WorkQueueType = 0; WorkQueueType < MaximumWorkQueue; WorkQueueType++)
    {
        /* Clear the structure and initialize the queue */
        RtlZeroMemory(&ExWorkerQueue[WorkQueueType], sizeof(EX_WORK_QUEUE));
        KeInitializeQueue(&ExWorkerQueue[WorkQueueType].WorkerQueue, 0);
    }

    /* Dynamic threads are only used for the critical queue */
    ExWorkerQueue[CriticalWorkQueue].Info.MakeThreadsAsNecessary = TRUE;

    /* Initialize the balance set manager events */
    KeInitializeEvent(&ExpThreadSetManagerEvent, SynchronizationEvent, FALSE);
    KeInitializeEvent(&ExpThreadSetManagerShutdownEvent,
                      NotificationEvent,
                      FALSE);

    /* Create the built-in worker threads for the critical queue */
    for (i = 0; i < CriticalThreads; i++)
    {
        /* Create the thread */
        ExpCreateWorkerThread(CriticalWorkQueue, FALSE);
        ExCriticalWorkerThreads++;
    }

    /* Create the built-in worker threads for the delayed queue */
    for (i = 0; i < DelayedThreads; i++)
    {
        /* Create the thread */
        ExpCreateWorkerThread(DelayedWorkQueue, FALSE);
        ExDelayedWorkerThreads++;
    }

    /* Create the built-in worker thread for the hypercritical queue */
    ExpCreateWorkerThread(HyperCriticalWorkQueue, FALSE);

    /* Create the balance set manager thread */
    PsCreateSystemThread(&ThreadHandle,
                         THREAD_ALL_ACCESS,
                         NULL,
                         0,
                         NULL,
                         ExpWorkerThreadBalanceManager,
                         NULL);

    /* Get a pointer to it for the shutdown process */
    ObReferenceObjectByHandle(ThreadHandle,
                              THREAD_ALL_ACCESS,
                              NULL,
                              KernelMode,
                              (PVOID*)&Thread,
                              NULL);
    ExpWorkerThreadBalanceManagerPtr = Thread;

    /* Close the handle and return */
    ObCloseHandle(ThreadHandle, KernelMode);
}

VOID
NTAPI
ExpSetSwappingKernelApc(IN PKAPC Apc,
                        OUT PKNORMAL_ROUTINE *NormalRoutine,
                        IN OUT PVOID *NormalContext,
                        IN OUT PVOID *SystemArgument1,
                        IN OUT PVOID *SystemArgument2)
{
    PBOOLEAN AllowSwap;
    PKEVENT Event = (PKEVENT)*SystemArgument1;

    /* Make sure it's an active worker */
    if (PsGetCurrentThread()->ActiveExWorker) 
    {
        /* Read the setting from the context flag */
        AllowSwap = (PBOOLEAN)NormalContext;
        KeSetKernelStackSwapEnable(*AllowSwap);
    }

    /* Let caller know that we're done */
    KeSetEvent(Event, 0, FALSE);
}

VOID
NTAPI
ExSwapinWorkerThreads(IN BOOLEAN AllowSwap)
{
    KEVENT Event;
    PETHREAD CurrentThread = PsGetCurrentThread(), Thread;
    PEPROCESS Process = PsInitialSystemProcess;
    KAPC Apc;
    PAGED_CODE();

    /* Initialize an event so we know when we're done */
    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    /* Lock this routine */
    ExAcquireFastMutex(&ExpWorkerSwapinMutex);

    /* New threads cannot swap anymore */
    ExpWorkersCanSwap = AllowSwap;

    /* Loop all threads in the system process */
    Thread = PsGetNextProcessThread(Process, NULL);
    while (Thread)
    {
        /* Skip threads with explicit permission to do this */
        if (Thread->ExWorkerCanWaitUser) goto Next;

        /* Check if we reached ourselves */
        if (Thread == CurrentThread)
        {
            /* Do it inline */
            KeSetKernelStackSwapEnable(AllowSwap);
        }
        else
        {
            /* Queue an APC */
            KeInitializeApc(&Apc,
                            &Thread->Tcb,
                            InsertApcEnvironment,
                            ExpSetSwappingKernelApc,
                            NULL,
                            NULL,
                            KernelMode,
                            &AllowSwap);
            if (KeInsertQueueApc(&Apc, &Event, NULL, 3))
            {
                /* Wait for the APC to run */
                KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
                KeClearEvent(&Event);
            }
        }
        
        /* Next thread */
Next:
        Thread = PsGetNextProcessThread(Process, Thread);
    }

    /* Release the lock */
    ExReleaseFastMutex(&ExpWorkerSwapinMutex);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name ExQueueWorkItem
 * @implemented NT4
 *
 *     The ExQueueWorkItem routine acquires rundown protection for
 *     the specified descriptor.
 *
 * @param WorkItem
 *        Pointer to an initialized Work Queue Item structure. This structure
 *        must be located in nonpaged pool memory.
 *
 * @param QueueType
 *        Type of the queue to use for this item. Can be one of the following:
 *          - DelayedWorkQueue
 *          - CriticalWorkQueue
 *          - HyperCriticalWorkQueue
 *
 * @return None.
 *
 * @remarks This routine is obsolete. Use IoQueueWorkItem instead.
 *
 *          Callers of this routine must be running at IRQL <= DISPATCH_LEVEL.
 *
 *--*/
VOID
NTAPI
ExQueueWorkItem(IN PWORK_QUEUE_ITEM WorkItem,
                IN WORK_QUEUE_TYPE QueueType)
{
    PEX_WORK_QUEUE WorkQueue = &ExWorkerQueue[QueueType];
    ASSERT(QueueType < MaximumWorkQueue);
    ASSERT(WorkItem->List.Flink == NULL);

    /* Don't try to trick us */
    if ((ULONG_PTR)WorkItem->WorkerRoutine < MmUserProbeAddress)
    {
        /* Bugcheck the system */
        KeBugCheckEx(WORKER_INVALID,
                     1,
                     (ULONG_PTR)WorkItem,
                     (ULONG_PTR)WorkItem->WorkerRoutine,
                     0);
    }

    /* Insert the Queue */
    KeInsertQueue(&WorkQueue->WorkerQueue, &WorkItem->List);
    ASSERT(!WorkQueue->Info.QueueDisabled);

    /*
     * Check if we need a new thread. Our decision is as follows:
     *  - This queue type must support Dynamic Threads (duh!)
     *  - It actually has to have unprocessed items
     *  - We have CPUs which could be handling another thread
     *  - We haven't abused our usage of dynamic threads.
     */
    if ((WorkQueue->Info.MakeThreadsAsNecessary) &&
        (!IsListEmpty(&WorkQueue->WorkerQueue.EntryListHead)) &&
        (WorkQueue->WorkerQueue.CurrentCount <
         WorkQueue->WorkerQueue.MaximumCount) &&
        (WorkQueue->DynamicThreadCount < 16))
    {
        /* Let the balance manager know about it */
        DPRINT1("Requesting a new thread. CurrentCount: %lu. MaxCount: %lu\n",
                WorkQueue->WorkerQueue.CurrentCount,
                WorkQueue->WorkerQueue.MaximumCount);
        KeSetEvent(&ExpThreadSetManagerEvent, 0, FALSE);
    }
}

/* EOF */
