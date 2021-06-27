#ifndef __BUFFRING_H__
#define __BUFFRING_H__

#define RX_REQUEST_PREPARED_FOR_HANDLING 0x10000000

typedef struct _CHANGE_BUFFERING_STATE_REQUEST_
{
    LIST_ENTRY ListEntry;
    ULONG Flags;
#if (_WIN32_WINNT < 0x0600)
    PSRV_CALL pSrvCall;
#endif
    PSRV_OPEN SrvOpen;
    PVOID SrvOpenKey;
    PVOID MRxContext;
} CHANGE_BUFFERING_STATE_REQUEST, *PCHANGE_BUFFERING_STATE_REQUEST;

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

#if (_WIN32_WINNT >= 0x0600)
#define RxAcquireBufferingManagerMutex(BufMan) ExAcquireFastMutex(&(BufMan)->Mutex)
#else
#define RxAcquireBufferingManagerMutex(BufMan)          \
    {                                                   \
        if (!ExTryToAcquireFastMutex(&(BufMan)->Mutex)) \
        {                                               \
            ExAcquireFastMutex(&(BufMan)->Mutex);       \
        }                                               \
    }
#endif
#define RxReleaseBufferingManagerMutex(BufMan) ExReleaseFastMutex(&(BufMan)->Mutex)

VOID
RxpProcessChangeBufferingStateRequests(
    PSRV_CALL SrvCall,
    BOOLEAN UpdateHandlerState);

VOID
NTAPI
RxProcessChangeBufferingStateRequests(
    _In_ PVOID SrvCall);

VOID
RxProcessFcbChangeBufferingStateRequest(
    _In_ PFCB Fcb);

VOID
RxPurgeChangeBufferingStateRequestsForSrvOpen(
    _In_ PSRV_OPEN SrvOpen);

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
RxTearDownBufferingManager(
   _In_ PSRV_CALL SrvCall);

NTSTATUS
RxFlushFcbInSystemCache(
    _In_ PFCB Fcb,
    _In_ BOOLEAN SynchronizeWithLazyWriter);

NTSTATUS
RxPurgeFcbInSystemCache(
    _In_ PFCB Fcb,
    _In_ PLARGE_INTEGER FileOffset OPTIONAL,
    _In_ ULONG Length,
    _In_ BOOLEAN UninitializeCacheMaps,
    _In_ BOOLEAN FlushFile);

#endif
