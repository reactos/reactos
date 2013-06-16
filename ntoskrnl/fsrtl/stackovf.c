/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/fsrtl/stackovf.c
 * PURPOSE:         Provides Stack Overflow support for File System Drivers
 * PROGRAMMERS:     Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

/* We have one queue for paging files, one queue for normal files
 * Queue 0 is for non-paging files
 * Queue 1 is for paging files
 * Don't add new/change current queues unless you know what you do
 * Most of the code relies on the fact that we have two queues in that order
 */
#define FSRTLP_MAX_QUEUES 2

typedef struct _STACK_OVERFLOW_WORK_ITEM
{   
   
    WORK_QUEUE_ITEM WorkItem;
    PFSRTL_STACK_OVERFLOW_ROUTINE Routine;   
    PVOID Context;
    PKEVENT Event;
} STACK_OVERFLOW_WORK_ITEM, *PSTACK_OVERFLOW_WORK_ITEM;

KEVENT StackOverflowFallbackSerialEvent;
STACK_OVERFLOW_WORK_ITEM StackOverflowFallback;
KQUEUE FsRtlWorkerQueues[FSRTLP_MAX_QUEUES];

/* PRIVATE FUNCTIONS *********************************************************/

/*
 * @implemented
 */
VOID
NTAPI
FsRtlStackOverflowRead(IN PVOID Context)
{
    PSTACK_OVERFLOW_WORK_ITEM WorkItem;

    WorkItem = (PSTACK_OVERFLOW_WORK_ITEM)Context;

    /* Put us as top IRP for current thread */
    IoSetTopLevelIrp((PIRP)FSRTL_FSP_TOP_LEVEL_IRP);
    /* And call FsRtlSORoutine */
    WorkItem->Routine(WorkItem->Context, WorkItem->Event);

    /* If we were using fallback workitem, don't free it, just reset event */
    if (WorkItem == &StackOverflowFallback)
    {
        KeSetEvent(&StackOverflowFallbackSerialEvent, 0, FALSE);
    }
    /* Otherwise, free the work item */
    else
    {
        ExFreePoolWithTag(WorkItem, 'Fsrs');
    }

    /* Reset top level */
    IoSetTopLevelIrp(NULL);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlpPostStackOverflow(IN PVOID Context,
                        IN PKEVENT Event,
                        IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine,
                        IN BOOLEAN IsPaging)
{
    PSTACK_OVERFLOW_WORK_ITEM WorkItem;

    /* Try to allocate a work item */
    WorkItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(STACK_OVERFLOW_WORK_ITEM), 'FSrs');
    if (WorkItem == NULL)
    {
        /* If we failed, and we are not a paging file, just raise an error */
        if (!IsPaging)
        {
            RtlRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
        }

        /* Otherwise, wait for fallback workitem to be available and use it */
        KeWaitForSingleObject(&StackOverflowFallbackSerialEvent, Executive, KernelMode, FALSE, NULL);
        WorkItem = &StackOverflowFallback;
    }

    /* Initialize work item */
    WorkItem->Context = Context;
    WorkItem->Event = Event;
    WorkItem->Routine = StackOverflowRoutine;
    ExInitializeWorkItem(&WorkItem->WorkItem, FsRtlStackOverflowRead, WorkItem);

    /* And queue it in the appropriate queue (paging or not?) */
    KeInsertQueue(&FsRtlWorkerQueues[IsPaging], &WorkItem->WorkItem.List);
}

/*
 * @implemented
 */
VOID
NTAPI
FsRtlWorkerThread(IN PVOID StartContext)
{
    KIRQL Irql;
    PLIST_ENTRY Entry;
    PWORK_QUEUE_ITEM WorkItem;
    ULONG QueueId = (ULONG)StartContext;

    /* Set our priority according to the queue we're dealing with */
    KeSetPriorityThread(&PsGetCurrentThread()->Tcb, LOW_REALTIME_PRIORITY + QueueId);

    /* Loop for events */
    for (;;)
    {
        /* Look for next event */
        Entry = KeRemoveQueue(&FsRtlWorkerQueues[QueueId], KernelMode, NULL);
        WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);

        /* Call its routine (here: FsRtlStackOverflowRead) */
        WorkItem->WorkerRoutine(WorkItem->Parameter);

        /* Check we're still at passive level or bugcheck */
        Irql = KeGetCurrentIrql();
        if (Irql != PASSIVE_LEVEL)
        {
            KeBugCheckEx(IRQL_NOT_LESS_OR_EQUAL, (ULONG_PTR)WorkItem->WorkerRoutine,   
                         (ULONG_PTR)Irql, (ULONG_PTR)WorkItem->WorkerRoutine,
                         (ULONG_PTR)WorkItem);
        }
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
INIT_FUNCTION
FsRtlInitializeWorkerThread(VOID)
{
    ULONG i;
    NTSTATUS Status;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Initialize each queue we have */
    for (i = 0; i < FSRTLP_MAX_QUEUES; ++i)
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);

        /* Initialize the queue and its associated thread and pass it the queue ID */
        KeInitializeQueue(&FsRtlWorkerQueues[i], 0);
        Status = PsCreateSystemThread(&ThreadHandle, THREAD_ALL_ACCESS, &ObjectAttributes,
                                      0, 0, FsRtlWorkerThread, (PVOID)i);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Don't leak handle */
        ZwClose(ThreadHandle);
    }

    /* Also initialize our fallback event, set it to ensure it's already usable */
    KeInitializeEvent(&StackOverflowFallbackSerialEvent, SynchronizationEvent, TRUE);

    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*++
 * @name FsRtlPostPagingFileStackOverflow
 * @implemented NT 5.2
 *
 *     The FsRtlPostPagingFileStackOverflow routine
 *
 * @param Context
 *
 * @param Event
 *
 * @param StackOverflowRoutine
 *
 * @return
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
FsRtlPostPagingFileStackOverflow(IN PVOID Context,
                                 IN PKEVENT Event,
                                 IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine)
{
    FsRtlpPostStackOverflow(Context, Event, StackOverflowRoutine, TRUE);
}

/*++
 * @name FsRtlPostStackOverflow
 * @implemented NT 5.2
 *
 *     The FsRtlPostStackOverflow routine
 *
 * @param Context
 *
 * @param Event
 *
 * @param StackOverflowRoutine
 *
 * @return
 *
 * @remarks None.
 *
 *--*/
VOID
NTAPI
FsRtlPostStackOverflow(IN PVOID Context,
                       IN PKEVENT Event,
                       IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine)
{
    FsRtlpPostStackOverflow(Context, Event, StackOverflowRoutine, FALSE);
}

/* EOF */
