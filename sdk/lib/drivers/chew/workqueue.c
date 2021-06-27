/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/drivers/chew/workqueue.c
 * PURPOSE:         Common Highlevel Executive Worker
 *
 * PROGRAMMERS:     arty (ayerkes@speakeasy.net)
 */

#include <ntddk.h>
#include <chew/chew.h>

#define NDEBUG
//#include <debug.h>

#define FOURCC(w,x,y,z) (((w) << 24) | ((x) << 16) | ((y) << 8) | (z))
#define CHEW_TAG FOURCC('C','H','E','W')

PDEVICE_OBJECT WorkQueueDevice;
LIST_ENTRY     WorkQueue;
KSPIN_LOCK     WorkQueueLock;
KEVENT         WorkQueueClear;

typedef struct _WORK_ITEM
{
    LIST_ENTRY Entry;
    PIO_WORKITEM WorkItem;
    VOID (*Worker)(PVOID WorkerContext);
    PVOID WorkerContext;
} WORK_ITEM, *PWORK_ITEM;

VOID ChewInit(PDEVICE_OBJECT DeviceObject)
{
    WorkQueueDevice = DeviceObject;
    InitializeListHead(&WorkQueue);
    KeInitializeSpinLock(&WorkQueueLock);
    KeInitializeEvent(&WorkQueueClear, NotificationEvent, TRUE);
}

VOID ChewShutdown(VOID)
{
    KeWaitForSingleObject(&WorkQueueClear, Executive, KernelMode, FALSE, NULL);
}

VOID NTAPI ChewWorkItem(PDEVICE_OBJECT DeviceObject, PVOID ChewItem)
{
    PWORK_ITEM WorkItem = ChewItem;
    KIRQL OldIrql;

    WorkItem->Worker(WorkItem->WorkerContext);

    IoFreeWorkItem(WorkItem->WorkItem);

    KeAcquireSpinLock(&WorkQueueLock, &OldIrql);
    RemoveEntryList(&WorkItem->Entry);

    if (IsListEmpty(&WorkQueue))
        KeSetEvent(&WorkQueueClear, 0, FALSE);

    KeReleaseSpinLock(&WorkQueueLock, OldIrql);

    ExFreePoolWithTag(WorkItem, CHEW_TAG);
}

BOOLEAN ChewCreate(VOID (*Worker)(PVOID), PVOID WorkerContext)
{
    PWORK_ITEM Item;
    Item = ExAllocatePoolWithTag(NonPagedPool,
                                 sizeof(WORK_ITEM),
                                 CHEW_TAG);

    if (Item)
    {
        Item->WorkItem = IoAllocateWorkItem(WorkQueueDevice);
        if (!Item->WorkItem)
        {
            ExFreePool(Item);
            return FALSE;
        }

        Item->Worker = Worker;
        Item->WorkerContext = WorkerContext;
        ExInterlockedInsertTailList(&WorkQueue, &Item->Entry, &WorkQueueLock);
        KeClearEvent(&WorkQueueClear);
        IoQueueWorkItem(Item->WorkItem, ChewWorkItem, DelayedWorkQueue, Item);

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
