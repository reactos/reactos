#ifndef _RXMINIRDR_
#define _RXMINIRDR_

#define RxSetIoStatusStatus(R, S) (R)->CurrentIrp->IoStatus.Status = (S)
#define RxSetIoStatusInfo(R, I) (R)->CurrentIrp->IoStatus.Information = (I)

#define RxShouldPostCompletion() ((KeGetCurrentIrql() >= DISPATCH_LEVEL))

#define RX_REGISTERMINI_FLAG_DONT_PROVIDE_UNCS 0x00000001
#define RX_REGISTERMINI_FLAG_DONT_PROVIDE_MAILSLOTS 0x00000002
#define RX_REGISTERMINI_FLAG_DONT_INIT_DRIVER_DISPATCH 0x00000004
#define	RX_REGISTERMINI_FLAG_DONT_INIT_PREFIX_N_SCAVENGER 0x00000008

NTSTATUS
NTAPI
RxRegisterMinirdr(
    _Out_ PRDBSS_DEVICE_OBJECT *DeviceObject,
    _Inout_ PDRIVER_OBJECT DriverObject,
    _In_ PMINIRDR_DISPATCH MrdrDispatch,
    _In_ ULONG Controls,
    _In_ PUNICODE_STRING DeviceName,
    _In_ ULONG DeviceExtensionSize,
    _In_ DEVICE_TYPE DeviceType,
    _In_ ULONG DeviceCharacteristics);

VOID
NTAPI
RxpUnregisterMinirdr(
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject);

NTSTATUS
NTAPI
RxStartMinirdr(
    _In_ PRX_CONTEXT RxContext,
    _Out_ PBOOLEAN PostToFsp);

NTSTATUS
NTAPI
RxStopMinirdr(
    _In_ PRX_CONTEXT RxContext,
    _Out_ PBOOLEAN PostToFsp);

NTSTATUS
NTAPI
RxFsdDispatch(
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject,
    _In_ PIRP Irp);

typedef
NTSTATUS
(NTAPI *PMRX_CALLDOWN) (
    _Inout_ PRX_CONTEXT RxContext);

typedef
NTSTATUS
(NTAPI *PMRX_CALLDOWN_CTX) (
    _Inout_ PRX_CONTEXT RxContext,
    _Inout_ PRDBSS_DEVICE_OBJECT RxDeviceObject);

typedef
NTSTATUS
(NTAPI *PMRX_CHKDIR_CALLDOWN) (
    _Inout_ PRX_CONTEXT RxContext,
    _In_ PUNICODE_STRING DirectoryName);

typedef
NTSTATUS
(NTAPI *PMRX_CHKFCB_CALLDOWN) (
    _In_ PFCB Fcb1,
    _In_ PFCB Fcb2);

typedef enum _RX_BLOCK_CONDITION {
    Condition_Uninitialized = 0,
    Condition_InTransition,
    Condition_Closing,
    Condition_Good,
    Condition_Bad,
    Condition_Closed
} RX_BLOCK_CONDITION, *PRX_BLOCK_CONDITION;

#define StableCondition(X) ((X) >= Condition_Good)

typedef
VOID
(NTAPI *PMRX_NETROOT_CALLBACK) (
    _Inout_ PMRX_CREATENETROOT_CONTEXT CreateContext);

typedef
VOID
(NTAPI *PMRX_EXTRACT_NETROOT_NAME) (
    _In_ PUNICODE_STRING FilePathName,
    _In_ PMRX_SRV_CALL SrvCall,
    _Out_ PUNICODE_STRING NetRootName,
    _Out_opt_ PUNICODE_STRING RestOfName);

typedef struct _MRX_CREATENETROOT_CONTEXT
{
    PRX_CONTEXT RxContext;
    PV_NET_ROOT pVNetRoot;
    KEVENT FinishEvent;
    NTSTATUS VirtualNetRootStatus;
    NTSTATUS NetRootStatus;
    RX_WORK_QUEUE_ITEM WorkQueueItem;
    PMRX_NETROOT_CALLBACK Callback;
} MRX_CREATENETROOT_CONTEXT, *PMRX_CREATENETROOT_CONTEXT;

typedef
NTSTATUS
(NTAPI *PMRX_CREATE_V_NET_ROOT) (
    _Inout_ PMRX_CREATENETROOT_CONTEXT Context);

typedef
NTSTATUS
(NTAPI *PMRX_UPDATE_NETROOT_STATE) (
    _Inout_ PMRX_NET_ROOT NetRoot);

typedef struct _MRX_SRVCALL_CALLBACK_CONTEXT
{
    PMRX_SRVCALLDOWN_STRUCTURE SrvCalldownStructure;
    ULONG CallbackContextOrdinal;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    NTSTATUS Status;
    PVOID RecommunicateContext;
} MRX_SRVCALL_CALLBACK_CONTEXT, *PMRX_SRVCALL_CALLBACK_CONTEXT;

typedef
VOID
(NTAPI *PMRX_SRVCALL_CALLBACK) (
    _Inout_ PMRX_SRVCALL_CALLBACK_CONTEXT Context);

typedef struct _MRX_SRVCALLDOWN_STRUCTURE
{
    KEVENT FinishEvent;
    LIST_ENTRY SrvCalldownList;
    PRX_CONTEXT RxContext;
    PMRX_SRV_CALL SrvCall;
    PMRX_SRVCALL_CALLBACK CallBack;
    BOOLEAN CalldownCancelled;
    ULONG NumberRemaining;
    ULONG NumberToWait;
    ULONG BestFinisherOrdinal;
    PRDBSS_DEVICE_OBJECT BestFinisher;
    MRX_SRVCALL_CALLBACK_CONTEXT CallbackContexts[1];
} MRX_SRVCALLDOWN_STRUCTURE, *PMRX_SRVCALLDOWN_STRUCTURE;

typedef
NTSTATUS
(NTAPI *PMRX_CREATE_SRVCALL) (
    _Inout_ PMRX_SRV_CALL SrvCall,
    _Inout_ PMRX_SRVCALL_CALLBACK_CONTEXT SrvCallCallBackContext);

typedef
NTSTATUS
(NTAPI *PMRX_SRVCALL_WINNER_NOTIFY)(
    _Inout_ PMRX_SRV_CALL SrvCall,
    _In_ BOOLEAN ThisMinirdrIsTheWinner,
    _Inout_ PVOID RecommunicateContext);

typedef
NTSTATUS
(NTAPI *PMRX_DEALLOCATE_FOR_FCB) (
    _Inout_ PMRX_FCB Fcb);

typedef
NTSTATUS
(NTAPI *PMRX_DEALLOCATE_FOR_FOBX) (
    _Inout_ PMRX_FOBX Fobx);

typedef
NTSTATUS
(NTAPI *PMRX_IS_LOCK_REALIZABLE) (
    _Inout_ PMRX_FCB Fcb,
    _In_ PLARGE_INTEGER ByteOffset,
    _In_ PLARGE_INTEGER Length,
    _In_ ULONG LowIoLockFlags);

typedef
NTSTATUS
(NTAPI *PMRX_FORCECLOSED_CALLDOWN) (
    _Inout_ PMRX_SRV_OPEN SrvOpen);

typedef
NTSTATUS
(NTAPI *PMRX_FINALIZE_SRVCALL_CALLDOWN) (
    _Inout_ PMRX_SRV_CALL SrvCall,
    _In_ BOOLEAN Force);

typedef
NTSTATUS
(NTAPI *PMRX_FINALIZE_V_NET_ROOT_CALLDOWN) (
    _Inout_ PMRX_V_NET_ROOT VirtualNetRoot,
    _In_ PBOOLEAN Force);

typedef
NTSTATUS
(NTAPI *PMRX_FINALIZE_NET_ROOT_CALLDOWN) (
    _Inout_ PMRX_NET_ROOT NetRoot,
    _In_ PBOOLEAN Force);

typedef
ULONG
(NTAPI *PMRX_EXTENDFILE_CALLDOWN) (
    _Inout_ PRX_CONTEXT RxContext,
    _Inout_ PLARGE_INTEGER NewFileSize,
    _Out_ PLARGE_INTEGER NewAllocationSize);

typedef
NTSTATUS
(NTAPI *PMRX_CHANGE_BUFFERING_STATE_CALLDOWN) (
    _Inout_ PRX_CONTEXT RxContext,
    _Inout_ PMRX_SRV_OPEN SrvOpen,
    _In_ PVOID MRxContext);

typedef
NTSTATUS
(NTAPI *PMRX_PREPARSE_NAME) (
    _Inout_ PRX_CONTEXT RxContext,
    _In_ PUNICODE_STRING Name);

typedef
NTSTATUS
(NTAPI *PMRX_GET_CONNECTION_ID) (
    _Inout_ PRX_CONTEXT RxContext,
    _Inout_ PRX_CONNECTION_ID UniqueId);

typedef
NTSTATUS
(NTAPI *PMRX_COMPUTE_NEW_BUFFERING_STATE) (
    _Inout_ PMRX_SRV_OPEN SrvOpen,
    _In_ PVOID MRxContext,
    _Out_ PULONG NewBufferingState);

typedef enum _LOWIO_OPS {
    LOWIO_OP_READ = 0,
    LOWIO_OP_WRITE,
    LOWIO_OP_SHAREDLOCK,
    LOWIO_OP_EXCLUSIVELOCK,
    LOWIO_OP_UNLOCK,
    LOWIO_OP_UNLOCK_MULTIPLE,
    LOWIO_OP_FSCTL,
    LOWIO_OP_IOCTL,
    LOWIO_OP_NOTIFY_CHANGE_DIRECTORY,
    LOWIO_OP_CLEAROUT,
    LOWIO_OP_MAXIMUM
} LOWIO_OPS;

typedef
NTSTATUS
(NTAPI *PLOWIO_COMPLETION_ROUTINE) (
    _In_ PRX_CONTEXT RxContext);

typedef LONGLONG RXVBO;

typedef struct _LOWIO_LOCK_LIST
{
    struct _LOWIO_LOCK_LIST * Next;
    ULONG LockNumber;
    RXVBO ByteOffset;
    LONGLONG Length;
    BOOLEAN ExclusiveLock;
    ULONG Key;
} LOWIO_LOCK_LIST, *PLOWIO_LOCK_LIST;

typedef struct _XXCTL_LOWIO_COMPONENT
{
    ULONG Flags;
    union
    {
        ULONG FsControlCode;
        ULONG IoControlCode;
    };
    ULONG InputBufferLength;
    PVOID pInputBuffer;
    ULONG OutputBufferLength;
    PVOID pOutputBuffer;
    UCHAR MinorFunction;
} XXCTL_LOWIO_COMPONENT;

typedef struct _LOWIO_CONTEXT
{
    USHORT Operation;
    USHORT Flags;
    PLOWIO_COMPLETION_ROUTINE CompletionRoutine;
    PERESOURCE Resource;
    ERESOURCE_THREAD ResourceThreadId;
    union
    {
        struct
        {
            ULONG Flags;
            PMDL Buffer;
            RXVBO ByteOffset;
            ULONG ByteCount;
            ULONG Key;
            PNON_PAGED_FCB NonPagedFcb;
        } ReadWrite;
        struct
        {
            union
            {
                PLOWIO_LOCK_LIST LockList;
                LONGLONG Length;
            };
            ULONG Flags;
            RXVBO ByteOffset;
            ULONG Key;
        } Locks;
        XXCTL_LOWIO_COMPONENT FsCtl;
        XXCTL_LOWIO_COMPONENT IoCtl;
        struct
        {
            BOOLEAN WatchTree;
            ULONG CompletionFilter;
            ULONG NotificationBufferLength;
            PVOID pNotificationBuffer;
        } NotifyChangeDirectory;
    } ParamsFor;
} LOWIO_CONTEXT;

#define LOWIO_CONTEXT_FLAG_SYNCCALL 0x01
#define LOWIO_CONTEXT_FLAG_SAVEUNLOCKS 0x2
#define LOWIO_CONTEXT_FLAG_LOUDOPS 0x04
#define LOWIO_CONTEXT_FLAG_CAN_COMPLETE_AT_DPC_LEVEL 0x08

#define LOWIO_READWRITEFLAG_PAGING_IO 0x01
#define LOWIO_READWRITEFLAG_EXTENDING_FILESIZE 0x02
#define LOWIO_READWRITEFLAG_EXTENDING_VDL 0x04

#define RDBSS_MANAGE_SRV_CALL_EXTENSION 0x01
#define RDBSS_MANAGE_NET_ROOT_EXTENSION 0x02
#define RDBSS_MANAGE_V_NET_ROOT_EXTENSION 0x04
#define RDBSS_MANAGE_FCB_EXTENSION 0x08
#define RDBSS_MANAGE_SRV_OPEN_EXTENSION 0x10
#define RDBSS_MANAGE_FOBX_EXTENSION 0x20
#define RDBSS_NO_DEFERRED_CACHE_READAHEAD 0x1000

typedef struct _MINIRDR_DISPATCH
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    ULONG MRxFlags;
    ULONG MRxSrvCallSize;
    ULONG MRxNetRootSize;
    ULONG MRxVNetRootSize;
    ULONG MRxFcbSize;
    ULONG MRxSrvOpenSize;
    ULONG MRxFobxSize;
    PMRX_CALLDOWN_CTX MRxStart;
    PMRX_CALLDOWN_CTX MRxStop;
    PMRX_CALLDOWN MRxCancel;
    PMRX_CALLDOWN MRxCreate;
    PMRX_CALLDOWN MRxCollapseOpen;
    PMRX_CALLDOWN MRxShouldTryToCollapseThisOpen;
    PMRX_CALLDOWN MRxFlush;
    PMRX_CALLDOWN MRxZeroExtend;
    PMRX_CALLDOWN MRxTruncate;
    PMRX_CALLDOWN MRxCleanupFobx;
    PMRX_CALLDOWN MRxCloseSrvOpen;
    PMRX_DEALLOCATE_FOR_FCB MRxDeallocateForFcb;
    PMRX_DEALLOCATE_FOR_FOBX MRxDeallocateForFobx;
    PMRX_IS_LOCK_REALIZABLE MRxIsLockRealizable;
    PMRX_FORCECLOSED_CALLDOWN MRxForceClosed;
    PMRX_CHKFCB_CALLDOWN MRxAreFilesAliased;
    PMRX_CALLDOWN MRxOpenPrintFile;
    PMRX_CALLDOWN MRxClosePrintFile;
    PMRX_CALLDOWN MRxWritePrintFile;
    PMRX_CALLDOWN MRxEnumeratePrintQueue;
    PMRX_CALLDOWN MRxClosedSrvOpenTimeOut;
    PMRX_CALLDOWN MRxClosedFcbTimeOut;
    PMRX_CALLDOWN MRxQueryDirectory;
    PMRX_CALLDOWN MRxQueryFileInfo;
    PMRX_CALLDOWN MRxSetFileInfo;
    PMRX_CALLDOWN MRxSetFileInfoAtCleanup;
    PMRX_CALLDOWN MRxQueryEaInfo;
    PMRX_CALLDOWN MRxSetEaInfo;
    PMRX_CALLDOWN MRxQuerySdInfo;
    PMRX_CALLDOWN MRxSetSdInfo;
    PMRX_CALLDOWN MRxQueryQuotaInfo;
    PMRX_CALLDOWN MRxSetQuotaInfo;
    PMRX_CALLDOWN MRxQueryVolumeInfo;
    PMRX_CALLDOWN MRxSetVolumeInfo;
    PMRX_CHKDIR_CALLDOWN MRxIsValidDirectory;
    PMRX_COMPUTE_NEW_BUFFERING_STATE MRxComputeNewBufferingState;
    PMRX_CALLDOWN MRxLowIOSubmit[LOWIO_OP_MAXIMUM+1];
    PMRX_EXTENDFILE_CALLDOWN MRxExtendForCache;
    PMRX_EXTENDFILE_CALLDOWN MRxExtendForNonCache;
    PMRX_CHANGE_BUFFERING_STATE_CALLDOWN MRxCompleteBufferingStateChangeRequest;
    PMRX_CREATE_V_NET_ROOT MRxCreateVNetRoot;
    PMRX_FINALIZE_V_NET_ROOT_CALLDOWN MRxFinalizeVNetRoot;
    PMRX_FINALIZE_NET_ROOT_CALLDOWN MRxFinalizeNetRoot;
    PMRX_UPDATE_NETROOT_STATE MRxUpdateNetRootState;
    PMRX_EXTRACT_NETROOT_NAME MRxExtractNetRootName;
    PMRX_CREATE_SRVCALL MRxCreateSrvCall;
    PMRX_CREATE_SRVCALL MRxCancelCreateSrvCall;
    PMRX_SRVCALL_WINNER_NOTIFY MRxSrvCallWinnerNotify;
    PMRX_FINALIZE_SRVCALL_CALLDOWN MRxFinalizeSrvCall;
    PMRX_CALLDOWN MRxDevFcbXXXControlFile;
    PMRX_PREPARSE_NAME MRxPreparseName;
    PMRX_GET_CONNECTION_ID MRxGetConnectionId;
    ULONG ScavengerTimeout;
} MINIRDR_DISPATCH, *PMINIRDR_DISPATCH;

#endif
