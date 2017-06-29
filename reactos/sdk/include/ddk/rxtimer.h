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
RxInitializeRxTimer(
    VOID);

#endif
