#ifndef _RDBSSSTRUC_
#define _RDBSSSTRUC_

#include "prefix.h"
#include "lowio.h"
#include "scavengr.h"
#include "rxcontx.h"
#include "fcb.h"

extern RX_SPIN_LOCK RxStrucSupSpinLock;

typedef struct _RDBSS_EXPORTS
{
    PRX_SPIN_LOCK pRxStrucSupSpinLock;
    PLONG pRxDebugTraceIndent;
} RDBSS_EXPORTS, *PRDBSS_EXPORTS;

typedef enum _LOCK_HOLDING_STATE
{
    LHS_LockNotHeld,
    LHS_SharedLockHeld,
    LHS_ExclusiveLockHeld
} LOCK_HOLDING_STATE, *PLOCK_HOLDING_STATE;

typedef struct _RDBSS_DATA
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    PDRIVER_OBJECT DriverObject;
    volatile LONG NumberOfMinirdrsStarted;
    FAST_MUTEX MinirdrRegistrationMutex;
    LIST_ENTRY RegisteredMiniRdrs;
    LONG NumberOfMinirdrsRegistered;
    PEPROCESS OurProcess;
    CACHE_MANAGER_CALLBACKS CacheManagerCallbacks;
#if (_WIN32_WINNT < 0x0600)
    CACHE_MANAGER_CALLBACKS CacheManagerNoOpCallbacks;
#endif
    ERESOURCE Resource;
} RDBSS_DATA;
typedef RDBSS_DATA *PRDBSS_DATA;

extern RDBSS_DATA RxData;

PEPROCESS
NTAPI
RxGetRDBSSProcess(
    VOID);

typedef enum _RX_RDBSS_STATE_
{
    RDBSS_STARTABLE = 0,
    RDBSS_STARTED,
    RDBSS_STOP_IN_PROGRESS
} RX_RDBSS_STATE, *PRX_RDBSS_STATE;

typedef struct _RDBSS_STARTSTOP_CONTEXT_
{
    RX_RDBSS_STATE State;
    ULONG Version;
    PRX_CONTEXT pStopContext;
} RDBSS_STARTSTOP_CONTEXT, *PRDBSS_STARTSTOP_CONTEXT;

typedef struct _RX_DISPATCHER_CONTEXT_
{
    volatile LONG NumberOfWorkerThreads;
    volatile PKEVENT pTearDownEvent;
} RX_DISPATCHER_CONTEXT, *PRX_DISPATCHER_CONTEXT;

#define RxSetRdbssState(RxDeviceObject, NewState)        \
{                                                        \
    KIRQL OldIrql;                                       \
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);    \
    RxDeviceObject->StartStopContext.State = (NewState); \
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);     \
}

#define RxGetRdbssState(RxDeviceObject) RxDeviceObject->StartStopContext.State

typedef struct _RDBSS_DEVICE_OBJECT {
    union
    {
        DEVICE_OBJECT DeviceObject;
        DEVICE_OBJECT;
    };
    ULONG RegistrationControls;
    PRDBSS_EXPORTS RdbssExports;
    PDEVICE_OBJECT RDBSSDeviceObject;
    PMINIRDR_DISPATCH Dispatch;
    UNICODE_STRING DeviceName;
    ULONG NetworkProviderPriority;
    HANDLE MupHandle;
    BOOLEAN RegisterUncProvider;
    BOOLEAN RegisterMailSlotProvider;
    BOOLEAN RegisteredAsFileSystem;
    BOOLEAN Unused;
    LIST_ENTRY MiniRdrListLinks;
    volatile ULONG NumberOfActiveFcbs;
    volatile ULONG NumberOfActiveContexts;
    struct
    {
        LARGE_INTEGER PagingReadBytesRequested;
        LARGE_INTEGER NonPagingReadBytesRequested;
        LARGE_INTEGER CacheReadBytesRequested;
        LARGE_INTEGER FastReadBytesRequested;
        LARGE_INTEGER NetworkReadBytesRequested;
        volatile ULONG ReadOperations;
        ULONG FastReadOperations;
        volatile ULONG RandomReadOperations;
        LARGE_INTEGER PagingWriteBytesRequested;
        LARGE_INTEGER NonPagingWriteBytesRequested;
        LARGE_INTEGER CacheWriteBytesRequested;
        LARGE_INTEGER FastWriteBytesRequested;
        LARGE_INTEGER NetworkWriteBytesRequested;
        volatile ULONG WriteOperations;
        ULONG FastWriteOperations;
        volatile ULONG RandomWriteOperations;
    };
    volatile LONG PostedRequestCount[RxMaximumWorkQueue];
    LONG OverflowQueueCount[RxMaximumWorkQueue];
    LIST_ENTRY OverflowQueue[RxMaximumWorkQueue];
    RX_SPIN_LOCK OverflowQueueSpinLock;
    LONG AsynchronousRequestsPending;
    PKEVENT pAsynchronousRequestsCompletionEvent;
    RDBSS_STARTSTOP_CONTEXT StartStopContext;
    RX_DISPATCHER_CONTEXT DispatcherContext;
    PRX_PREFIX_TABLE pRxNetNameTable;
    RX_PREFIX_TABLE RxNetNameTableInDeviceObject;
    PRDBSS_SCAVENGER pRdbssScavenger;
    RDBSS_SCAVENGER RdbssScavengerInDeviceObject;
} RDBSS_DEVICE_OBJECT, *PRDBSS_DEVICE_OBJECT;

FORCEINLINE
VOID
NTAPI
RxUnregisterMinirdr(
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject)
{
    PDEVICE_OBJECT RDBSSDeviceObject;

    RDBSSDeviceObject = RxDeviceObject->RDBSSDeviceObject;

    RxpUnregisterMinirdr(RxDeviceObject);

    if (RDBSSDeviceObject != NULL)
    {
        ObDereferenceObject(RDBSSDeviceObject);
    }
}

#endif
