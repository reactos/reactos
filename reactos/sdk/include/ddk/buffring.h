#ifndef __BUFFRING_H__
#define __BUFFRING_H__

typedef struct _RX_BUFFERING_MANAGER_
{
    BOOLEAN DispatcherActive;
    BOOLEAN HandlerInactive;
    BOOLEAN LastChanceHandlerActive;
    UCHAR Pad;
    KSPIN_LOCK SpinLock;
    volatile LONG CumulativeNumberOfBufferingChangeRequests;
    LONG NumberOfUnhandledRequests;
    LONG NumberOfUndispatchedRequests;
    volatile LONG NumberOfOutstandingOpens;
    LIST_ENTRY DispatcherList;
    LIST_ENTRY HandlerList;
    LIST_ENTRY LastChanceHandlerList;
    RX_WORK_QUEUE_ITEM DispatcherWorkItem;
    RX_WORK_QUEUE_ITEM HandlerWorkItem;
    RX_WORK_QUEUE_ITEM LastChanceHandlerWorkItem;
    FAST_MUTEX Mutex;
    LIST_ENTRY SrvOpenLists[1];
} RX_BUFFERING_MANAGER, *PRX_BUFFERING_MANAGER;

VOID
RxProcessFcbChangeBufferingStateRequest(
    _In_ PFCB Fcb);

VOID
RxCompleteSrvOpenKeyAssociation(
    _Inout_ PSRV_OPEN SrvOpen);

VOID
RxInitiateSrvOpenKeyAssociation(
   _Inout_ PSRV_OPEN SrvOpen);

NTSTATUS
RxInitializeBufferingManager(
   _In_ PSRV_CALL SrvCall);

NTSTATUS
RxPurgeFcbInSystemCache(
    _In_ PFCB Fcb,
    _In_ PLARGE_INTEGER FileOffset OPTIONAL,
    _In_ ULONG Length,
    _In_ BOOLEAN UninitializeCacheMaps,
    _In_ BOOLEAN FlushFile);

#endif
