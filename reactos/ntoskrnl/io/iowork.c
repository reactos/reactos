/* $Id: iowork.c,v 1.1 2002/10/03 19:17:26 robd Exp $
 *
 * COPYRIGHT:          See COPYING in the top level directory
 * PROJECT:            ReactOS kernel
 * FILE:               reactos/ntoskrnk/io/iowork.c
 * PURPOSE:            Manage IO system work queues
 * PROGRAMMER:         David Welch (welch@mcmail.com)
 *                     Robert Dickenson (odin@pnc.com.au)
 * REVISION HISTORY:
 *       28/09/2002:   (RDD) Created from copy of ex/work.c
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

#define NDEBUG
#include <internal/debug.h>


/* DEFINES *******************************************************************/

#define NUMBER_OF_WORKER_THREADS   (5)


/* TYPES *********************************************************************/

typedef struct _WORK_QUEUE {
   LIST_ENTRY Head;  // Head of the list of waiting work items
   KSPIN_LOCK Lock;  // Sychronize access to the work queue
   KSEMAPHORE Sem;   // Worker threads with nothing to do wait on this event
   HANDLE Thread[NUMBER_OF_WORKER_THREADS]; // Thread associated with work queue
} WORK_QUEUE, *PWORK_QUEUE;


struct _IO_WORKITEM {
    LIST_ENTRY IoWorkItemsList;
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;
    int size;
    PIO_WORKITEM_ROUTINE WorkerRoutine;
    WORK_QUEUE_TYPE QueueType;
    PVOID Context;
    int reserved[50];
};

//typedef VOID (*PIO_WORKITEM_ROUTINE)(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context);


/* GLOBALS *******************************************************************/

#define TAG_IOWI    TAG('I', 'O', 'W', 'I')

// Queue of items waiting to be processed at normal priority
WORK_QUEUE IoNormalWorkQueue;
WORK_QUEUE IoCriticalWorkQueue;
WORK_QUEUE IoHyperCriticalWorkQueue;


/* LOCALS *******************************************************************/

static BOOL IoWorkQueueItemsListInitialised = FALSE;
static LIST_ENTRY IoWorkQueueItemsListHead;
static KSPIN_LOCK IoWorkQueueItemsListLock;


/* FUNCTIONS ****************************************************************/

static
NTSTATUS
STDCALL
IoWorkerThreadEntryPoint(PVOID context)
/*
 * FUNCTION: Entry point for a worker thread
 * ARGUMENTS:
 *         context = Parameters
 * RETURNS: Status
 * NOTE: To kill a worker thread you must queue an item whose callback
 * calls PsTerminateSystemThread
 */
{
    PWORK_QUEUE queue = (PWORK_QUEUE)context;
    PIO_WORKITEM pWorkItem;
    PLIST_ENTRY pListEntry;
   
    for (;;) {
        pListEntry = ExInterlockedRemoveHeadList(&queue->Head, &queue->Lock);
        if (pListEntry != NULL) {
            pWorkItem = CONTAINING_RECORD(pListEntry, struct _IO_WORKITEM, IoWorkItemsList);
            (pWorkItem->WorkerRoutine)(pWorkItem->DeviceObject, pWorkItem->Context);
        } else {
            KeWaitForSingleObject((PVOID)&queue->Sem,
                   Executive,
                   KernelMode,
                   FALSE,
                   NULL);
            DPRINT("Woke from wait\n");
        }
    }
}
/*

VOID
Scan_IoQueueWorkItems(PDEVICE_OBJECT DeviceObject, WORK_QUEUE_TYPE QueueType)
{
    PIO_WORKITEM pWorkItem;
    PLIST_ENTRY pListEntry;
    KIRQL oldlvl;

    pListEntry = IoWorkQueueItemsListHead.Flink;
    while (pListEntry != &IoWorkQueueItemsListHead) {
        pWorkItem = CONTAINING_RECORD(pListEntry, struct _IO_WORKITEM, IoWorkItemsList);
        if (pWorkItem->QueueType == QueueType) {
            KeAcquireSpinLock(&IoWorkQueueItemsListLock,&oldlvl);
            RemoveEntryList(pListEntry);
            KeReleaseSpinLock(&IoWorkQueueItemsListLock,oldlvl);
            pListEntry = pListEntry->Flink;
            (pWorkItem->WorkerRoutine)(pWorkItem->DeviceObject, pWorkItem->Context);
        } else {
            pListEntry = pListEntry->Flink;
        }
    }
}
 */

static
VOID
IoInitializeWorkQueue(PWORK_QUEUE WorkQueue, KPRIORITY Priority)
{
    ULONG i;
    PETHREAD Thread;
   
    InitializeListHead(&WorkQueue->Head);
    KeInitializeSpinLock(&WorkQueue->Lock);
    KeInitializeSemaphore(&WorkQueue->Sem, 0, 256);
    
    for (i = 0; i < NUMBER_OF_WORKER_THREADS; i++) {
        PsCreateSystemThread(&WorkQueue->Thread[i],
                 THREAD_ALL_ACCESS,
                 NULL,
                 NULL,
                 NULL,
                 IoWorkerThreadEntryPoint,
                 WorkQueue);
        ObReferenceObjectByHandle(WorkQueue->Thread[i],
                  THREAD_ALL_ACCESS,
                  PsThreadType,
                  KernelMode,
                  (PVOID*)&Thread,
                  NULL);
        KeSetPriorityThread(&Thread->Tcb, Priority);
        ObDereferenceObject(Thread);
    }
}

VOID
IoInitializeWorkerThreads(VOID)
{
    IoInitializeWorkQueue(&IoNormalWorkQueue, LOW_PRIORITY);
    IoInitializeWorkQueue(&IoCriticalWorkQueue, LOW_REALTIME_PRIORITY);
    IoInitializeWorkQueue(&IoHyperCriticalWorkQueue, HIGH_PRIORITY);
}

/* NOTES:
IoQueueWorkItem inserts the specified work item into a queue from which a 
  system worker thread removes the item and gives control to the specified
  callback routine.
Highest-level drivers can call IoQueueWorkItem.
Callers of IoQueueWorkItem must be running at IRQL <= DISPATCH_LEVEL.
The callback is run within a system thread context at IRQL PASSIVE_LEVEL.
This caller-supplied routine is responsible for calling IoFreeWorkItem to 
  reclaim the storage allocated for the work item.
IoQueueWorkItem should be used instead of ExQueueWorkItem because 
  IoQueueWorkItem will ensure that the device object associated with the 
  specified work item is available for the processing of the work item.
 */
VOID
STDCALL
IoQueueWorkItem(IN PIO_WORKITEM pIoWorkItem, IN PIO_WORKITEM_ROUTINE pWorkerRoutine,
    IN WORK_QUEUE_TYPE QueueType, IN PVOID pContext)
/*
 * FUNCTION: Inserts a work item in a queue for one of the system worker
 * threads to process
 * ARGUMENTS:
 *        pIoWorkItem = Item to insert
 *        QueueType = Queue to insert it in
 */
{
    DPRINT("IoQueueWorkItem(%p, %p, %p, %p)\n", pIoWorkItem, pWorkerRoutine, QueueType, pContext);

    assert(pIoWorkItem!=NULL);

    if (!IoWorkQueueItemsListInitialised) {
        IoWorkQueueItemsListInitialised = TRUE;
        InitializeListHead(&IoWorkQueueItemsListHead);
        KeInitializeSpinLock(&IoWorkQueueItemsListLock);
    }
    pIoWorkItem->WorkerRoutine = pWorkerRoutine;
    pIoWorkItem->QueueType = QueueType;
    pIoWorkItem->Context = pContext;
    //ExInterlockedInsertHeadList(&IoWorkQueueItemsListHead, &pIoWorkItem->IoWorkItemsList, &IoWorkQueueItemsListLock);

    switch(QueueType) {
    case DelayedWorkQueue:
        ExInterlockedInsertTailList(&IoNormalWorkQueue.Head,
                                    &pIoWorkItem->IoWorkItemsList,
                                    &IoNormalWorkQueue.Lock);
        KeReleaseSemaphore(&IoNormalWorkQueue.Sem,
                           IO_NO_INCREMENT, 1, FALSE);
        break;
    case CriticalWorkQueue:
        ExInterlockedInsertTailList(&IoCriticalWorkQueue.Head,
                                    &pIoWorkItem->IoWorkItemsList,
                                    &IoCriticalWorkQueue.Lock);
        KeReleaseSemaphore(&IoCriticalWorkQueue.Sem,
                           IO_NO_INCREMENT, 1, FALSE);
        break;
    case HyperCriticalWorkQueue:
        ExInterlockedInsertTailList(&IoHyperCriticalWorkQueue.Head,
                                    &pIoWorkItem->IoWorkItemsList,
                                    &IoHyperCriticalWorkQueue.Lock);
        KeReleaseSemaphore(&IoHyperCriticalWorkQueue.Sem,
                           IO_NO_INCREMENT, 1, FALSE);
        break;
    }
}

/*
 */
VOID STDCALL
IoFreeWorkItem(PIO_WORKITEM pIoWorkItem)
{
    if (pIoWorkItem != NULL) {
        ExFreePool(pIoWorkItem);
    } else {
        DPRINT("IoFreeWorkItem() passed NULL pointer ???\n");
    }
}

/* NOTES:
Callers of IoAllocateWorkItem must be running at IRQL <= DISPATCH_LEVEL
 */
PIO_WORKITEM STDCALL
IoAllocateWorkItem(PDEVICE_OBJECT DeviceObject)
{
    PIO_WORKITEM pIoWorkItem = NULL;

    assert(DeviceObject!=NULL);
    //ASSERT_IRQL(DISPATCH_LEVEL);

    pIoWorkItem = ExAllocatePoolWithTag(NonPagedPool, sizeof(struct _IO_WORKITEM), TAG_IOWI);
    if (pIoWorkItem != NULL) {
        RtlZeroMemory(pIoWorkItem, sizeof(struct _IO_WORKITEM));
        pIoWorkItem->size = sizeof(struct _IO_WORKITEM);
        pIoWorkItem->DeviceObject = DeviceObject;
    } else {
        DPRINT("IoAllocateWorkItem() FAILED to allocated %d bytes memory\n", sizeof(struct _IO_WORKITEM));
    }
    return pIoWorkItem;
}

/* EOF */
