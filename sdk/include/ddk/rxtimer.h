#ifndef _RXTIMER_H_
#define _RXTIMER_H_

typedef struct _RX_WORK_ITEM_
{
    RX_WORK_QUEUE_ITEM WorkQueueItem;
    ULONG LastTick;
    ULONG Options;
} RX_WORK_ITEM, *PRX_WORK_ITEM;

NTSTATUS
NTAPI
RxPostOneShotTimerRequest(
    _In_ PRDBSS_DEVICE_OBJECT pDeviceObject,
    _In_ PRX_WORK_ITEM pWorkItem,
    _In_ PRX_WORKERTHREAD_ROUTINE Routine,
    _In_ PVOID pContext,
    _In_ LARGE_INTEGER TimeInterval);

NTSTATUS
NTAPI
RxInitializeRxTimer(
    VOID);

#endif
