/*
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               ntoskrnl/ex/work.c
 * PURPOSE:            Manage system work queues
 *
 * PROGRAMMERS:        Alex Ionescu - Used correct work queue array and added some fixes and checks.
 *                     Gunnar Dalsnes - Implemented
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DEFINES *******************************************************************/

#define NUMBER_OF_WORKER_THREADS   (5)

/* TYPES *********************************************************************/

/* GLOBALS *******************************************************************/

/*
 * PURPOSE: Queue of items waiting to be processed at normal priority
 */
EX_WORK_QUEUE ExWorkerQueue[MaximumWorkQueue];

/* FUNCTIONS ****************************************************************/

/*
 * FUNCTION: Entry point for a worker thread
 * ARGUMENTS:
 *         context = Parameters
 * RETURNS: Status
 * NOTE: To kill a worker thread you must queue an item whose callback
 * calls PsTerminateSystemThread
 */
static
VOID
STDCALL
ExpWorkerThreadEntryPoint(IN PVOID Context)
{
    PWORK_QUEUE_ITEM WorkItem;
    PLIST_ENTRY QueueEntry;
    WORK_QUEUE_TYPE WorkQueueType;
    PEX_WORK_QUEUE WorkQueue;

    /* Get Queue Type and Worker Queue */
    WorkQueueType = (WORK_QUEUE_TYPE)Context;
    WorkQueue = &ExWorkerQueue[WorkQueueType];

    /* Loop forever */
    while (TRUE) {

        /* Wait for Something to Happen on the Queue */
        QueueEntry = KeRemoveQueue(&WorkQueue->WorkerQueue, KernelMode, NULL);

        /* Can't happen since we do a KernelMode wait (and we're a system thread) */
        ASSERT((NTSTATUS)QueueEntry != STATUS_USER_APC);

        /* this should never happen either, since we wait with NULL timeout,
         * but there's a slight possibility that STATUS_TIMEOUT is returned
         * at queue rundown in NT (unlikely) -Gunnar
         */
        ASSERT((NTSTATUS)QueueEntry != STATUS_TIMEOUT);

        /* Increment Processed Work Items */
        InterlockedIncrement((PLONG)&WorkQueue->WorkItemsProcessed);

        /* Get the Work Item */
        WorkItem = CONTAINING_RECORD(QueueEntry, WORK_QUEUE_ITEM, List);

        /* Call the Worker Routine */
        WorkItem->WorkerRoutine(WorkItem->Parameter);

        /* Make sure it returned at right IRQL */
        if (KeGetCurrentIrql() != PASSIVE_LEVEL) {

            /* FIXME: Make this an Ex */
            KEBUGCHECK(WORKER_THREAD_RETURNED_AT_BAD_IRQL);
        }

        /* Make sure it returned with Impersionation Disabled */
        if (PsGetCurrentThread()->ActiveImpersonationInfo) {

            /* FIXME: Make this an Ex */
            KEBUGCHECK(IMPERSONATING_WORKER_THREAD);
        }
    }
}

static
VOID
STDCALL
ExpInitializeWorkQueue(WORK_QUEUE_TYPE WorkQueueType,
                       KPRIORITY Priority)
{
    ULONG i;
    PETHREAD Thread;
    HANDLE hThread;

    /* Loop through how many threads we need to create */
    for (i = 0; i < NUMBER_OF_WORKER_THREADS; i++) {

        /* Create the System Thread */
        PsCreateSystemThread(&hThread,
                             THREAD_ALL_ACCESS,
                             NULL,
                             NULL,
                             NULL,
                             ExpWorkerThreadEntryPoint,
                             (PVOID)WorkQueueType);

        /* Get the Thread */
        ObReferenceObjectByHandle(hThread,
                                  THREAD_SET_INFORMATION,
                                  PsThreadType,
                                  KernelMode,
                                  (PVOID*)&Thread,
                                  NULL);

        /* Set the Priority */
        KeSetPriorityThread(&Thread->Tcb, Priority);

        /* Dereference and close handle */
        ObDereferenceObject(Thread);
        ZwClose(hThread);
    }
}

VOID
INIT_FUNCTION
ExpInitializeWorkerThreads(VOID)
{
    ULONG WorkQueueType;

    /* Initialize the Array */
    for (WorkQueueType = 0; WorkQueueType < MaximumWorkQueue; WorkQueueType++) {

        RtlZeroMemory(&ExWorkerQueue[WorkQueueType], sizeof(EX_WORK_QUEUE));
        KeInitializeQueue(&ExWorkerQueue[WorkQueueType].WorkerQueue, 0);
    }

    /* Create the built-in worker threads for each work queue */
    ExpInitializeWorkQueue(CriticalWorkQueue, LOW_REALTIME_PRIORITY);
    ExpInitializeWorkQueue(DelayedWorkQueue, LOW_PRIORITY);
    ExpInitializeWorkQueue(HyperCriticalWorkQueue, HIGH_PRIORITY);
}

/*
 * @implemented
 *
 * FUNCTION: Inserts a work item in a queue for one of the system worker
 * threads to process
 * ARGUMENTS:
 *        WorkItem = Item to insert
 *        QueueType = Queue to insert it in
 */
VOID
STDCALL
ExQueueWorkItem(PWORK_QUEUE_ITEM WorkItem,
                WORK_QUEUE_TYPE QueueType)
{
    ASSERT(WorkItem!=NULL);
    ASSERT_IRQL(DISPATCH_LEVEL);
    ASSERT(WorkItem->List.Flink == NULL);

    /* Insert the Queue */
    KeInsertQueue(&ExWorkerQueue[QueueType].WorkerQueue, &WorkItem->List);
}

/* EOF */
