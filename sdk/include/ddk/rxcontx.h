#ifndef _RX_CONTEXT_STRUCT_DEFINED_
#define _RX_CONTEXT_STRUCT_DEFINED_

#define RX_TOPLEVELIRP_CONTEXT_SIGNATURE 'LTxR'

typedef struct _RX_TOPLEVELIRP_CONTEXT
{
    union
    {
#ifndef __cplusplus
        LIST_ENTRY;
#endif
        LIST_ENTRY ListEntry;
    };
    ULONG Signature;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PRX_CONTEXT RxContext;
    PIRP Irp;
    ULONG Flags;
    PVOID Previous;
    PETHREAD Thread;
} RX_TOPLEVELIRP_CONTEXT, *PRX_TOPLEVELIRP_CONTEXT;

BOOLEAN
RxTryToBecomeTheTopLevelIrp(
    _Inout_ PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    _In_ PIRP Irp,
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject,
    _In_ BOOLEAN ForceTopLevel);

VOID
__RxInitializeTopLevelIrpContext(
    _Inout_ PRX_TOPLEVELIRP_CONTEXT TopLevelContext,
    _In_ PIRP Irp,
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject,
    _In_ ULONG Flags);

#define RxInitializeTopLevelIrpContext(a,b,c) __RxInitializeTopLevelIrpContext(a,b,c,0)

PIRP
RxGetTopIrpIfRdbssIrp(
    VOID);

PRDBSS_DEVICE_OBJECT
RxGetTopDeviceObjectIfRdbssIrp(
    VOID);

VOID
RxUnwindTopLevelIrp(
    _Inout_ PRX_TOPLEVELIRP_CONTEXT TopLevelContext);

BOOLEAN
RxIsThisTheTopLevelIrp(
    _In_ PIRP Irp);

#ifdef RDBSS_TRACKER
typedef struct _RX_FCBTRACKER_CALLINFO
{
    ULONG AcquireRelease;
    USHORT SavedTrackerValue;
    USHORT LineNumber;
    PSZ FileName;
    ULONG Flags;
} RX_FCBTRACKER_CALLINFO, *PRX_FCBTRACKER_CALLINFO;
#define RDBSS_TRACKER_HISTORY_SIZE 32
#endif

#define MRX_CONTEXT_FIELD_COUNT 4

#if (_WIN32_WINNT >= 0x0600)
typedef
NTSTATUS
(NTAPI *PRX_DISPATCH) (
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp);
#else
typedef
NTSTATUS
(NTAPI *PRX_DISPATCH) (
    _In_ PRX_CONTEXT RxContext);
#endif

typedef struct _DFS_NAME_CONTEXT_ *PDFS_NAME_CONTEXT;

typedef struct _NT_CREATE_PARAMETERS
{
    ACCESS_MASK DesiredAccess;
    LARGE_INTEGER AllocationSize;
    ULONG FileAttributes;
    ULONG ShareAccess;
    ULONG Disposition;
    ULONG CreateOptions;
    PIO_SECURITY_CONTEXT SecurityContext;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PVOID DfsContext;
    PDFS_NAME_CONTEXT DfsNameContext;
} NT_CREATE_PARAMETERS, *PNT_CREATE_PARAMETERS;

typedef struct _RX_CONTEXT
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    volatile ULONG ReferenceCount;
    LIST_ENTRY ContextListEntry;
    UCHAR MajorFunction;
    UCHAR MinorFunction;
    BOOLEAN PendingReturned;
    BOOLEAN PostRequest;
    PDEVICE_OBJECT RealDevice;
    PIRP CurrentIrp;
    PIO_STACK_LOCATION CurrentIrpSp;
    PMRX_FCB pFcb;
    PMRX_FOBX pFobx;
    PMRX_SRV_OPEN pRelevantSrvOpen;
    PNON_PAGED_FCB NonPagedFcb;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PETHREAD OriginalThread;
    PETHREAD LastExecutionThread;
    volatile PVOID LockManagerContext;
    PVOID RdbssDbgExtension;
    RX_SCAVENGER_ENTRY ScavengerEntry;
    ULONG SerialNumber;
    ULONG FobxSerialNumber;
    ULONG Flags;
    BOOLEAN FcbResourceAcquired;
    BOOLEAN FcbPagingIoResourceAcquired;
    UCHAR MustSucceedDescriptorNumber;
    union
    {
        struct
        {
            union
            {
                NTSTATUS StoredStatus;
                PVOID StoredStatusAlignment;
            };
            ULONG_PTR InformationToReturn;
        };
        IO_STATUS_BLOCK IoStatusBlock;
    };
    union
    {
        ULONGLONG ForceLonglongAligmentDummyField;
        PVOID MRxContext[MRX_CONTEXT_FIELD_COUNT];
    };
    PVOID WriteOnlyOpenRetryContext;
    PMRX_CALLDOWN MRxCancelRoutine;
    PRX_DISPATCH ResumeRoutine;
    RX_WORK_QUEUE_ITEM WorkQueueItem;
    LIST_ENTRY OverflowListEntry;
    KEVENT SyncEvent;
    LIST_ENTRY BlockedOperations;
    PFAST_MUTEX BlockedOpsMutex;
    LIST_ENTRY RxContextSerializationQLinks;
    union
    {
        struct
        {
            union
            {
                FS_INFORMATION_CLASS FsInformationClass;
                FILE_INFORMATION_CLASS FileInformationClass;
            };
            PVOID Buffer;
            union
            {
                LONG Length;
                LONG LengthRemaining;
            };
            BOOLEAN ReplaceIfExists;
            BOOLEAN AdvanceOnly;
        } Info;
        struct
        {
            UNICODE_STRING SuppliedPathName;
            NET_ROOT_TYPE NetRootType;
            PIO_SECURITY_CONTEXT pSecurityContext;
        } PrefixClaim;
    };
    union
    {
        struct
        {
            NT_CREATE_PARAMETERS NtCreateParameters;
            ULONG ReturnedCreateInformation;
            PWCH CanonicalNameBuffer;
            PRX_PREFIX_ENTRY NetNamePrefixEntry;
            PMRX_SRV_CALL pSrvCall;
            PMRX_NET_ROOT pNetRoot;
            PMRX_V_NET_ROOT pVNetRoot;
            PVOID EaBuffer;
            ULONG EaLength;
            ULONG SdLength;
            ULONG PipeType;
            ULONG PipeReadMode;
            ULONG PipeCompletionMode;
            USHORT Flags;
            NET_ROOT_TYPE Type;
            UCHAR RdrFlags;
            BOOLEAN FcbAcquired;
            BOOLEAN TryForScavengingOnSharingViolation;
            BOOLEAN ScavengingAlreadyTried;
            BOOLEAN ThisIsATreeConnectOpen;
            BOOLEAN TreeConnectOpenDeferred;
            UNICODE_STRING TransportName;
            UNICODE_STRING UserName;
            UNICODE_STRING Password;
            UNICODE_STRING UserDomainName;
        } Create;
        struct
        {
            ULONG FileIndex;
            BOOLEAN RestartScan;
            BOOLEAN ReturnSingleEntry;
            BOOLEAN IndexSpecified;
            BOOLEAN InitialQuery;
        } QueryDirectory;
        struct
        {
            PMRX_V_NET_ROOT pVNetRoot;
        } NotifyChangeDirectory;
        struct
        {
            PUCHAR UserEaList;
            ULONG UserEaListLength;
            ULONG UserEaIndex;
            BOOLEAN RestartScan;
            BOOLEAN ReturnSingleEntry;
            BOOLEAN IndexSpecified;
        } QueryEa;
        struct
        {
            SECURITY_INFORMATION SecurityInformation;
            ULONG Length;
        } QuerySecurity;
        struct
        {
            SECURITY_INFORMATION SecurityInformation;
            PSECURITY_DESCRIPTOR SecurityDescriptor;
        } SetSecurity;
        struct
        {
            ULONG Length;
            PSID StartSid;
            PFILE_GET_QUOTA_INFORMATION SidList;
            ULONG SidListLength;
            BOOLEAN RestartScan;
            BOOLEAN ReturnSingleEntry;
            BOOLEAN IndexSpecified;
        } QueryQuota;
        struct
        {
            ULONG Length;
        } SetQuota;
        struct
        {
            PV_NET_ROOT VNetRoot;
            PSRV_CALL SrvCall;
            PNET_ROOT NetRoot;
        } DosVolumeFunction;
        struct {
            ULONG FlagsForLowIo;
            LOWIO_CONTEXT LowIoContext;
        };
        LUID FsdUid;
    };
    PWCH AlsoCanonicalNameBuffer;
    PUNICODE_STRING LoudCompletionString;
#ifdef RDBSS_TRACKER
    volatile LONG AcquireReleaseFcbTrackerX;
    volatile ULONG TrackerHistoryPointer;
    RX_FCBTRACKER_CALLINFO TrackerHistory[RDBSS_TRACKER_HISTORY_SIZE];
#endif
#if DBG
    ULONG ShadowCritOwner;
#endif
} RX_CONTEXT, *PRX_CONTEXT;

typedef enum
{
    RX_CONTEXT_FLAG_FROM_POOL = 0x00000001,
    RX_CONTEXT_FLAG_WAIT = 0x00000002,
    RX_CONTEXT_FLAG_WRITE_THROUGH = 0x00000004,
    RX_CONTEXT_FLAG_FLOPPY = 0x00000008,
    RX_CONTEXT_FLAG_RECURSIVE_CALL = 0x00000010,
    RX_CONTEXT_FLAG_THIS_DEVICE_TOP_LEVEL = 0x00000020,
    RX_CONTEXT_FLAG_DEFERRED_WRITE = 0x00000040,
    RX_CONTEXT_FLAG_VERIFY_READ = 0x00000080,
    RX_CONTEXT_FLAG_STACK_IO_CONTEZT = 0x00000100,
    RX_CONTEXT_FLAG_IN_FSP = 0x00000200,
    RX_CONTEXT_FLAG_CREATE_MAILSLOT = 0x00000400,
    RX_CONTEXT_FLAG_MAILSLOT_REPARSE = 0x00000800,
    RX_CONTEXT_FLAG_ASYNC_OPERATION = 0x00001000,
    RX_CONTEXT_FLAG_NO_COMPLETE_FROM_FSP = 0x00002000,
    RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION = 0x00004000,
    RX_CONTEXT_FLAG_FSP_DELAYED_OVERFLOW_QUEUE = 0x00008000,
    RX_CONTEXT_FLAG_FSP_CRITICAL_OVERFLOW_QUEUE = 0x00010000,
    RX_CONTEXT_FLAG_MINIRDR_INVOKED = 0x00020000,
    RX_CONTEXT_FLAG_WAITING_FOR_RESOURCE = 0x00040000,
    RX_CONTEXT_FLAG_CANCELLED = 0x00080000,
    RX_CONTEXT_FLAG_SYNC_EVENT_WAITERS = 0x00100000,
    RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED = 0x00200000,
    RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK = 0x00400000,
    RX_CONTEXT_FLAG_BLOCKED_PIPE_RESUME = 0x00800000,
    RX_CONTEXT_FLAG_IN_SERIALIZATION_QUEUE = 0x01000000,
    RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT = 0x02000000,
    RX_CONTEXT_FLAG_NEEDRECONNECT = 0x04000000,
    RX_CONTEXT_FLAG_MUST_SUCCEED = 0x08000000,
    RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING = 0x10000000,
    RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED = 0x20000000,
    RX_CONTEXT_FLAG_MINIRDR_INITIATED = 0x80000000,
} RX_CONTEXT_FLAGS;

#define RX_CONTEXT_PRESERVED_FLAGS (RX_CONTEXT_FLAG_FROM_POOL |              \
                                    RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED | \
                                    RX_CONTEXT_FLAG_IN_FSP)

#define RX_CONTEXT_INITIALIZATION_FLAGS (RX_CONTEXT_FLAG_WAIT |         \
                                         RX_CONTEXT_FLAG_MUST_SUCCEED | \
                                         RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING)

typedef enum
{
    RX_CONTEXT_CREATE_FLAG_UNC_NAME = 0x1,
    RX_CONTEXT_CREATE_FLAG_STRIPPED_TRAILING_BACKSLASH = 0x2,
    RX_CONTEXT_CREATE_FLAG_ADDEDBACKSLASH = 0x4,
    RX_CONTEXT_CREATE_FLAG_REPARSE = 0x8,
    RX_CONTEXT_CREATE_FLAG_SPECIAL_PATH = 0x10,
} RX_CONTEXT_CREATE_FLAGS;

typedef enum {
    RXCONTEXT_FLAG4LOWIO_PIPE_OPERATION = 0x1,
    RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION = 0x2,
    RXCONTEXT_FLAG4LOWIO_READAHEAD = 0x4,
    RXCONTEXT_FLAG4LOWIO_THIS_READ_ENLARGED = 0x8,
    RXCONTEXT_FLAG4LOWIO_THIS_IO_BUFFERED = 0x10,
    RXCONTEXT_FLAG4LOWIO_LOCK_FCB_RESOURCE_HELD = 0x20,
    RXCONTEXT_FLAG4LOWIO_LOCK_WAS_QUEUED_IN_LOCKMANAGER = 0x40,
    RXCONTEXT_FLAG4LOWIO_THIS_IO_FAST = 0x80,
    RXCONTEXT_FLAG4LOWIO_LOCK_OPERATION_COMPLETED = 0x100,
    RXCONTEXT_FLAG4LOWIO_LOCK_BUFFERED_ON_ENTRY = 0x200
} RX_CONTEXT_LOWIO_FLAGS;

#if DBG
#define RxSaveAndSetExceptionNoBreakpointFlag(R, F)                \
{                                                                  \
    F = FlagOn(R->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT); \
    SetFlag(R->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT);    \
}

#define RxRestoreExceptionNoBreakpointFlag(R, F)                  \
{                                                                 \
    ClearFlag(R->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT); \
    SetFlag(R->Flags, F);                                         \
}
#else
#define RxSaveAndSetExceptionNoBreakpointFlag(R, F)
#define RxRestoreExceptionNoBreakpointFlag(R, F)
#endif

#if DBG
VOID
__RxItsTheSameContext(
    _In_ PRX_CONTEXT RxContext,
    _In_ ULONG CapturedRxContextSerialNumber,
    _In_ ULONG Line,
    _In_ PCSTR File);
#define RxItsTheSameContext() { __RxItsTheSameContext(RxContext, CapturedRxContextSerialNumber, __LINE__, __FILE__); }
#else
#define RxItsTheSameContext() { NOTHING; }
#endif

extern NPAGED_LOOKASIDE_LIST RxContextLookasideList;

#define MINIRDR_CALL_THROUGH(STATUS, DISPATCH, FUNC, ARGLIST) \
{                                                             \
    ASSERT(DISPATCH);                                         \
    ASSERT(NodeType(DISPATCH) == RDBSS_NTC_MINIRDR_DISPATCH); \
    if (DISPATCH->FUNC == NULL)                               \
    {                                                         \
        STATUS = STATUS_NOT_IMPLEMENTED;                      \
    }                                                         \
    else                                                      \
    {                                                         \
        STATUS = DISPATCH->FUNC ARGLIST;                      \
    }                                                         \
}

#define MINIRDR_CALL(STATUS, CONTEXT, DISPATCH, FUNC, ARGLIST)           \
{                                                                        \
    ASSERT(DISPATCH);                                                    \
    ASSERT(NodeType(DISPATCH) == RDBSS_NTC_MINIRDR_DISPATCH);            \
    if (DISPATCH->FUNC == NULL)                                          \
    {                                                                    \
        STATUS = STATUS_NOT_IMPLEMENTED;                                 \
    }                                                                    \
    else                                                                 \
    {                                                                    \
        if (!BooleanFlagOn((CONTEXT)->Flags, RX_CONTEXT_FLAG_CANCELLED)) \
	{                                                                \
            RtlZeroMemory(&((CONTEXT)->MRxContext[0]),                   \
                          sizeof((CONTEXT)->MRxContext));                \
            STATUS = DISPATCH->FUNC ARGLIST;                             \
        }                                                                \
        else                                                             \
        {                                                                \
            STATUS = STATUS_CANCELLED;                                   \
        }                                                                \
    }                                                                    \
}

#define RxWaitSync(RxContext)                                 \
    (RxContext)->Flags |= RX_CONTEXT_FLAG_SYNC_EVENT_WAITERS; \
    KeWaitForSingleObject(&(RxContext)->SyncEvent,            \
                          Executive, KernelMode, FALSE, NULL)

#define RxSignalSynchronousWaiter(RxContext)                   \
    (RxContext)->Flags &= ~RX_CONTEXT_FLAG_SYNC_EVENT_WAITERS; \
    KeSetEvent(&(RxContext)->SyncEvent, 0, FALSE)

#define RxInsertContextInSerializationQueue(SerializationQueue, RxContext) \
    (RxContext)->Flags |= RX_CONTEXT_FLAG_IN_SERIALIZATION_QUEUE;          \
    InsertTailList(SerializationQueue, &((RxContext)->RxContextSerializationQLinks))

FORCEINLINE
PRX_CONTEXT
RxRemoveFirstContextFromSerializationQueue(
    PLIST_ENTRY SerializationQueue)
{
    if (IsListEmpty(SerializationQueue))
    {
        return NULL;
    }
    else
    {
        PRX_CONTEXT Context = CONTAINING_RECORD(SerializationQueue->Flink,
                                                RX_CONTEXT,
                                                RxContextSerializationQLinks);

        RemoveEntryList(SerializationQueue->Flink);

        Context->RxContextSerializationQLinks.Flink = NULL;
        Context->RxContextSerializationQLinks.Blink = NULL;

        return Context;
    }
}

#define RxTransferList(Destination, Source)         \
    if (IsListEmpty((Source)))                      \
        InitializeListHead((Destination));          \
    else                                            \
    {                                               \
       *(Destination) = *(Source);                  \
       (Destination)->Flink->Blink = (Destination); \
       (Destination)->Blink->Flink = (Destination); \
       InitializeListHead((Source));                \
    }

#define RxTransferListWithMutex(Destination, Source, Mutex) \
    {                                                       \
        ExAcquireFastMutex(Mutex);                          \
        RxTransferList(Destination, Source);                \
        ExReleaseFastMutex(Mutex);                          \
    }

NTSTATUS
RxCancelNotifyChangeDirectoryRequestsForVNetRoot(
   PV_NET_ROOT VNetRoot,
   BOOLEAN ForceFilesClosed);

VOID
RxCancelNotifyChangeDirectoryRequestsForFobx(
   PFOBX Fobx
   );

VOID
NTAPI
RxInitializeContext(
    _In_ PIRP Irp,
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject,
    _In_ ULONG InitialContextFlags,
    _Inout_ PRX_CONTEXT RxContext);

PRX_CONTEXT
NTAPI
RxCreateRxContext(
    _In_ PIRP Irp,
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject,
    _In_ ULONG InitialContextFlags);

VOID
NTAPI
RxPrepareContextForReuse(
   _Inout_ PRX_CONTEXT RxContext);

VOID
NTAPI
RxDereferenceAndDeleteRxContext_Real(
    _In_ PRX_CONTEXT RxContext);

VOID
NTAPI
RxReinitializeContext(
   _Inout_ PRX_CONTEXT RxContext);

#if DBG
#define RxDereferenceAndDeleteRxContext(RXCONTEXT) \
{ \
    RxDereferenceAndDeleteRxContext_Real((RXCONTEXT)); \
    (RXCONTEXT) = NULL; \
}
#else
#define RxDereferenceAndDeleteRxContext(RXCONTEXT) \
{ \
    RxDereferenceAndDeleteRxContext_Real((RXCONTEXT)); \
}
#endif

extern FAST_MUTEX RxContextPerFileSerializationMutex;

VOID
NTAPI
RxResumeBlockedOperations_Serially(
    _Inout_ PRX_CONTEXT RxContext,
    _Inout_ PLIST_ENTRY BlockingIoQ);

VOID
RxResumeBlockedOperations_ALL(
    _Inout_ PRX_CONTEXT RxContext);

#if (_WIN32_WINNT >= 0x0600)
VOID
RxCancelBlockingOperation(
    _Inout_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp);
#else
VOID
RxCancelBlockingOperation(
    _Inout_ PRX_CONTEXT RxContext);
#endif

VOID
RxRemoveOperationFromBlockingQueue(
    _Inout_ PRX_CONTEXT RxContext);

#endif
