#ifndef _FCB_STRUCTS_DEFINED_
#define _FCB_STRUCTS_DEFINED_

#include "buffring.h"

struct _FCB_INIT_PACKET;
typedef struct _FCB_INIT_PACKET *PFCB_INIT_PACKET;

typedef struct _SRV_CALL
{
    union
    {
        MRX_SRV_CALL;
        struct
        {
            MRX_NORMAL_NODE_HEADER spacer;
        };
    };
    BOOLEAN UpperFinalizationDone;
    RX_PREFIX_ENTRY PrefixEntry;
    RX_BLOCK_CONDITION Condition;
    ULONG SerialNumberForEnum;
    volatile LONG NumberOfCloseDelayedFiles;
    LIST_ENTRY TransitionWaitList;
    LIST_ENTRY ScavengerFinalizationList;
    PURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext;
    RX_BUFFERING_MANAGER BufferingManager;
} SRV_CALL, *PSRV_CALL;

#define NETROOT_FLAG_FINALIZATION_IN_PROGRESS 0x00040000
#define NETROOT_FLAG_NAME_ALREADY_REMOVED 0x00080000

typedef struct _NET_ROOT
{
    union
    {
        MRX_NET_ROOT;
        struct
        {
            MRX_NORMAL_NODE_HEADER spacer;
            PSRV_CALL SrvCall;
        };
    };
    BOOLEAN UpperFinalizationDone;
    RX_BLOCK_CONDITION Condition;
    LIST_ENTRY TransitionWaitList;
    LIST_ENTRY ScavengerFinalizationList;
    PURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext;
    PV_NET_ROOT DefaultVNetRoot;
    LIST_ENTRY VirtualNetRoots;
    ULONG NumberOfVirtualNetRoots;
    ULONG SerialNumberForEnum;
    RX_PREFIX_ENTRY PrefixEntry;
    RX_FCB_TABLE FcbTable;
} NET_ROOT, *PNET_ROOT;

typedef struct _V_NET_ROOT
{
    union
    {
        MRX_V_NET_ROOT;
        struct
        {
            MRX_NORMAL_NODE_HEADER spacer;
            PNET_ROOT NetRoot;
        };
    };
    BOOLEAN UpperFinalizationDone;
    BOOLEAN ConnectionFinalizationDone;
    RX_BLOCK_CONDITION Condition;
    volatile LONG AdditionalReferenceForDeleteFsctlTaken;
    RX_PREFIX_ENTRY PrefixEntry;
    UNICODE_STRING NamePrefix;
    ULONG PrefixOffsetInBytes;
    LIST_ENTRY NetRootListEntry;
    ULONG SerialNumberForEnum;
    LIST_ENTRY TransitionWaitList;
    LIST_ENTRY ScavengerFinalizationList;
} V_NET_ROOT, *PV_NET_ROOT;

typedef struct _NON_PAGED_FCB
{
    NODE_TYPE_CODE NodeTypeCode;
    NODE_BYTE_SIZE NodeByteSize;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    ERESOURCE HeaderResource;
    ERESOURCE PagingIoResource;
#ifdef USE_FILESIZE_LOCK
    FAST_MUTEX FileSizeLock;
#endif
    LIST_ENTRY TransitionWaitList;
    ULONG OutstandingAsyncWrites;
    PKEVENT OutstandingAsyncEvent;
    KEVENT TheActualEvent;
    PVOID MiniRdrContext[2];
    FAST_MUTEX AdvancedFcbHeaderMutex;
    ERESOURCE BufferedLocksResource;
#if DBG
    PFCB FcbBackPointer;
#endif
} NON_PAGED_FCB, *PNON_PAGED_FCB;

typedef enum _RX_FCBTRACKER_CASES
{
    RX_FCBTRACKER_CASE_NORMAL,
    RX_FCBTRACKER_CASE_NULLCONTEXT,
    RX_FCBTRACKER_CASE_CBS_CONTEXT,
    RX_FCBTRACKER_CASE_CBS_WAIT_CONTEXT,
    RX_FCBTRACKER_CASE_MAXIMUM
} RX_FCBTRACKER_CASES;

typedef struct _FCB_LOCK
{
    struct _FCB_LOCK * Next;
    LARGE_INTEGER Length;
    LARGE_INTEGER BytesOffset;
    ULONG Key;
    BOOLEAN ExclusiveLock;
} FCB_LOCK, *PFCB_LOCK;

typedef struct _FCB_BUFFERED_LOCKS
{
    struct _FCB_LOCK * List;
    volatile ULONG PendingLockOps;
    PERESOURCE Resource;
} FCB_BUFFERED_LOCKS, *PFCB_BUFFERED_LOCKS;

typedef struct _FCB
{
    union
    {
        MRX_FCB;
        struct
        {
            FSRTL_ADVANCED_FCB_HEADER spacer;
            PNET_ROOT NetRoot;
        };
    };
    PV_NET_ROOT VNetRoot;
    PNON_PAGED_FCB NonPaged;
    LIST_ENTRY ScavengerFinalizationList;
    PKEVENT pBufferingStateChangeCompletedEvent;
    LONG NumberOfBufferingStateChangeWaiters;
    RX_FCB_TABLE_ENTRY FcbTableEntry;
    UNICODE_STRING PrivateAlreadyPrefixedName;
    BOOLEAN UpperFinalizationDone;
    RX_BLOCK_CONDITION Condition;
    PRX_FSD_DISPATCH_VECTOR PrivateDispatchVector;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PMINIRDR_DISPATCH MRxDispatch;
    PFAST_IO_DISPATCH MRxFastIoDispatch;
    PSRV_OPEN InternalSrvOpen;
    PFOBX InternalFobx;
    SHARE_ACCESS ShareAccess;
    SHARE_ACCESS ShareAccessPerSrvOpens;
    ULONG NumberOfLinks;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER LastChangeTime;
#if (_WIN32_WINNT < 0x0600)
    PETHREAD CreateSectionThread;
#endif
    ULONG ulFileSizeVersion;
#if (_WIN32_WINNT < 0x0600)
    union
    {
        struct
        {
#endif
        FILE_LOCK FileLock;
#if (_WIN32_WINNT < 0x0600)
        PVOID LazyWriteThread;
#endif
        union
        {
            LOWIO_PER_FCB_INFO;
            LOWIO_PER_FCB_INFO LowIoPerFcbInfo;
        };
#ifdef USE_FILESIZE_LOCK
        PFAST_MUTEX FileSizeLock;
#endif
#if (_WIN32_WINNT < 0x0600)
        } Fcb;
    } Specific;
#endif
    ULONG EaModificationCount;
    FCB_BUFFERED_LOCKS BufferedLocks;
#if DBG
    PNON_PAGED_FCB CopyOfNonPaged;
#endif
#ifdef RDBSS_TRACKER
    ULONG FcbAcquires[RX_FCBTRACKER_CASE_MAXIMUM];
    ULONG FcbReleases[RX_FCBTRACKER_CASE_MAXIMUM];
#else
#error tracker must be defined
#endif
    PCHAR PagingIoResourceFile;
    ULONG PagingIoResourceLine;
} FCB, *PFCB;

#define FCB_STATE_DELETE_ON_CLOSE 0x00000001
#define FCB_STATE_TRUNCATE_ON_CLOSE 0x00000002
#define FCB_STATE_PAGING_FILE 0x00000004
#define FCB_STATE_DISABLE_LOCAL_BUFFERING 0x00000010
#define FCB_STATE_TEMPORARY 0x00000020
#define FCB_STATE_BUFFERING_STATE_CHANGE_PENDING 0x00000040
#define FCB_STATE_ORPHANED 0x00000080
#define FCB_STATE_READAHEAD_DEFERRED 0x00000100
#define FCB_STATE_DELAY_CLOSE 0x00000800
#define FCB_STATE_FAKEFCB 0x00001000
#define FCB_STATE_FILE_IS_BUF_COMPRESSED 0x00004000
#define FCB_STATE_FILE_IS_DISK_COMPRESSED 0x00008000
#define FCB_STATE_FILE_IS_SHADOWED 0x00010000
#define FCB_STATE_BUFFERSTATE_CHANGING 0x00002000
#define FCB_STATE_SPECIAL_PATH 0x00020000
#define FCB_STATE_TIME_AND_SIZE_ALREADY_SET 0x00040000
#define FCB_STATE_FILETIMECACHEING_ENABLED 0x00080000
#define FCB_STATE_FILESIZECACHEING_ENABLED 0x00100000
#define FCB_STATE_LOCK_BUFFERING_ENABLED 0x00200000
#define FCB_STATE_COLLAPSING_ENABLED 0x00400000
#define FCB_STATE_OPENSHARING_ENABLED 0x00800000
#define FCB_STATE_READBUFFERING_ENABLED 0x01000000
#define FCB_STATE_READCACHING_ENABLED 0x02000000
#define FCB_STATE_WRITEBUFFERING_ENABLED 0x04000000
#define FCB_STATE_WRITECACHING_ENABLED 0x08000000
#define FCB_STATE_NAME_ALREADY_REMOVED 0x10000000
#define FCB_STATE_ADDEDBACKSLASH 0x20000000
#define FCB_STATE_FOBX_USED 0x40000000
#define FCB_STATE_SRVOPEN_USED 0x80000000

#define FCB_STATE_BUFFERING_STATE_MASK     \
    ((FCB_STATE_WRITECACHING_ENABLED       \
      | FCB_STATE_WRITEBUFFERING_ENABLED   \
      | FCB_STATE_READCACHING_ENABLED      \
      | FCB_STATE_READBUFFERING_ENABLED    \
      | FCB_STATE_OPENSHARING_ENABLED      \
      | FCB_STATE_COLLAPSING_ENABLED       \
      | FCB_STATE_LOCK_BUFFERING_ENABLED   \
      | FCB_STATE_FILESIZECACHEING_ENABLED \
      | FCB_STATE_FILETIMECACHEING_ENABLED))

typedef struct _FCB_INIT_PACKET
{
    PULONG pAttributes;
    PULONG pNumLinks;
    PLARGE_INTEGER pCreationTime;
    PLARGE_INTEGER pLastAccessTime;
    PLARGE_INTEGER pLastWriteTime;
    PLARGE_INTEGER pLastChangeTime;
    PLARGE_INTEGER pAllocationSize;
    PLARGE_INTEGER pFileSize;
    PLARGE_INTEGER pValidDataLength;
} FCB_INIT_PACKET;

#define SRVOPEN_FLAG_ENCLOSED_ALLOCATED 0x10000
#define SRVOPEN_FLAG_FOBX_USED 0x20000
#define SRVOPEN_FLAG_SHAREACCESS_UPDATED 0x40000

typedef struct _SRV_OPEN
{
    union
    {
        MRX_SRV_OPEN;
        struct
        {
            MRX_NORMAL_NODE_HEADER spacer;
            PFCB Fcb;
#if (_WIN32_WINNT >= 0x600)
            PV_NET_ROOT VNetRoot;
#endif
        };
    };
    BOOLEAN UpperFinalizationDone;
    RX_BLOCK_CONDITION Condition;
    volatile LONG BufferingToken;
    LIST_ENTRY ScavengerFinalizationList;
    LIST_ENTRY TransitionWaitList;
    LIST_ENTRY FobxList;
    PFOBX InternalFobx;
    union
    {
        LIST_ENTRY SrvOpenKeyList;
        ULONG SequenceNumber;
    };
    NTSTATUS OpenStatus;
} SRV_OPEN, *PSRV_OPEN;

#define FOBX_FLAG_MATCH_ALL 0x10000
#define FOBX_FLAG_FREE_UNICODE 0x20000
#define FOBX_FLAG_USER_SET_LAST_WRITE 0x40000
#define FOBX_FLAG_USER_SET_LAST_ACCESS 0x80000
#define FOBX_FLAG_USER_SET_CREATION 0x100000
#define FOBX_FLAG_USER_SET_LAST_CHANGE 0x200000
#define FOBX_FLAG_DELETE_ON_CLOSE 0x800000
#define FOBX_FLAG_SRVOPEN_CLOSED 0x1000000
#define FOBX_FLAG_UNC_NAME 0x2000000
#define FOBX_FLAG_ENCLOSED_ALLOCATED 0x4000000
#define FOBX_FLAG_MARKED_AS_DORMANT 0x8000000
#ifdef __REACTOS__
#define FOBX_FLAG_DISABLE_COLLAPSING 0x20000000
#endif

typedef struct _FOBX
{
    union
    {
        MRX_FOBX;
        struct
        {
            MRX_NORMAL_NODE_HEADER spacer;
            PSRV_OPEN SrvOpen;
        };
    };
    volatile ULONG FobxSerialNumber;
    LIST_ENTRY FobxQLinks;
    LIST_ENTRY ScavengerFinalizationList;
    LIST_ENTRY ClosePendingList;
    LARGE_INTEGER CloseTime;
    BOOLEAN UpperFinalizationDone;
    BOOLEAN ContainsWildCards;
    BOOLEAN fOpenCountDecremented;
    union
    {
        struct
        {
            union
            {
                MRX_PIPE_HANDLE_INFORMATION;
                MRX_PIPE_HANDLE_INFORMATION PipeHandleInformation;
            };
            LARGE_INTEGER CollectDataTime;
            ULONG CollectDataSize;
            THROTTLING_STATE ThrottlingState;
            LIST_ENTRY ReadSerializationQueue;
            LIST_ENTRY WriteSerializationQueue;
        } NamedPipe;
        struct {
            RXVBO PredictedReadOffset;
            RXVBO PredictedWriteOffset;
            THROTTLING_STATE LockThrottlingState;
            LARGE_INTEGER LastLockOffset;
            LARGE_INTEGER LastLockRange;
        } DiskFile;
    } Specific;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
} FOBX, *PFOBX;

#define RDBSS_REF_TRACK_SRVCALL  0x00000001
#define RDBSS_REF_TRACK_NETROOT  0x00000002
#define RDBSS_REF_TRACK_VNETROOT 0x00000004
#define RDBSS_REF_TRACK_NETFOBX  0x00000008
#define RDBSS_REF_TRACK_NETFCB   0x00000010
#define RDBSS_REF_TRACK_SRVOPEN  0x00000020
#define RX_PRINT_REF_TRACKING    0x40000000
#define RX_LOG_REF_TRACKING      0x80000000

extern ULONG RdbssReferenceTracingValue;

VOID
RxpTrackReference(
    _In_ ULONG TraceType,
    _In_ PCSTR FileName,
    _In_ ULONG Line,
    _In_ PVOID Instance);

BOOLEAN
RxpTrackDereference(
    _In_ ULONG TraceType,
    _In_ PCSTR FileName,
    _In_ ULONG Line,
    _In_ PVOID Instance);

#define REF_TRACING_ON(TraceMask) (TraceMask & RdbssReferenceTracingValue)
#ifndef __REACTOS__
#define PRINT_REF_COUNT(TYPE, Count)                          \
    if (REF_TRACING_ON( RDBSS_REF_TRACK_ ## TYPE) &&          \
        (RdbssReferenceTracingValue & RX_PRINT_REF_TRACKING)) \
    {                                                         \
        DbgPrint("%ld\n", Count);                             \
    }
#else
#define PRINT_REF_COUNT(TYPE, Count)                                     \
    if (REF_TRACING_ON( RDBSS_REF_TRACK_ ## TYPE) &&                     \
        (RdbssReferenceTracingValue & RX_PRINT_REF_TRACKING))            \
    {                                                                    \
        DbgPrint("(%s:%d) %s: %ld\n", __FILE__, __LINE__, #TYPE, Count); \
    }
#endif

#define RxReferenceSrvCall(SrvCall)                                          \
    RxpTrackReference(RDBSS_REF_TRACK_SRVCALL, __FILE__, __LINE__, SrvCall); \
    RxReference(SrvCall)

#define RxDereferenceSrvCall(SrvCall, LockHoldingState)                        \
    RxpTrackDereference(RDBSS_REF_TRACK_SRVCALL, __FILE__, __LINE__, SrvCall); \
    RxDereference(SrvCall, LockHoldingState)

#define RxReferenceNetRoot(NetRoot)                                          \
    RxpTrackReference(RDBSS_REF_TRACK_NETROOT, __FILE__, __LINE__, NetRoot); \
    RxReference(NetRoot)

#define RxDereferenceNetRoot(NetRoot, LockHoldingState)                        \
    RxpTrackDereference(RDBSS_REF_TRACK_NETROOT, __FILE__, __LINE__, NetRoot); \
    RxDereference(NetRoot, LockHoldingState)

#define RxReferenceVNetRoot(VNetRoot)                                          \
    RxpTrackReference(RDBSS_REF_TRACK_VNETROOT, __FILE__, __LINE__, VNetRoot); \
    RxReference(VNetRoot)

#define RxDereferenceVNetRoot(VNetRoot, LockHoldingState)                        \
    RxpTrackDereference(RDBSS_REF_TRACK_VNETROOT, __FILE__, __LINE__, VNetRoot); \
    RxDereference(VNetRoot, LockHoldingState)

#define RxReferenceNetFobx(Fobx)                                          \
    RxpTrackReference(RDBSS_REF_TRACK_NETFOBX, __FILE__, __LINE__, Fobx); \
    RxReference(Fobx)

#define RxDereferenceNetFobx(Fobx, LockHoldingState)                        \
    RxpTrackDereference(RDBSS_REF_TRACK_NETFOBX, __FILE__, __LINE__, Fobx); \
    RxDereference(Fobx, LockHoldingState)

#define RxReferenceSrvOpen(SrvOpen)                                          \
    RxpTrackReference(RDBSS_REF_TRACK_SRVOPEN, __FILE__, __LINE__, SrvOpen); \
    RxReference(SrvOpen)

#define RxDereferenceSrvOpen(SrvOpen, LockHoldingState)                        \
    RxpTrackDereference(RDBSS_REF_TRACK_SRVOPEN, __FILE__, __LINE__, SrvOpen); \
    RxDereference(SrvOpen, LockHoldingState)

#define RxReferenceNetFcb(Fcb)                                           \
    (RxpTrackReference(RDBSS_REF_TRACK_NETFCB, __FILE__, __LINE__, Fcb), \
     RxpReferenceNetFcb(Fcb))

#define RxDereferenceNetFcb(Fcb)                                                 \
    ((LONG)RxpTrackDereference(RDBSS_REF_TRACK_NETFCB, __FILE__, __LINE__, Fcb), \
     RxpDereferenceNetFcb(Fcb))

#define RxDereferenceAndFinalizeNetFcb(Fcb, RxContext, RecursiveFinalize, ForceFinalize)  \
    (RxpTrackDereference(RDBSS_REF_TRACK_NETFCB, __FILE__, __LINE__, Fcb),                \
     RxpDereferenceAndFinalizeNetFcb(Fcb, RxContext, RecursiveFinalize, ForceFinalize))

PSRV_CALL
RxCreateSrvCall(
    _In_ PRX_CONTEXT RxContext,
    _In_ PUNICODE_STRING Name,
    _In_opt_ PUNICODE_STRING InnerNamePrefix,
    _In_ PRX_CONNECTION_ID RxConnectionId);

#define RxWaitForStableSrvCall(S, R) RxWaitForStableCondition(&(S)->Condition, &(S)->TransitionWaitList, (R), NULL)
#define RxTransitionSrvCall(S, C) RxUpdateCondition((C), &(S)->Condition, &(S)->TransitionWaitList)

#if (_WIN32_WINNT >= 0x0600)
BOOLEAN
RxFinalizeSrvCall(
    _Out_ PSRV_CALL ThisSrvCall,
    _In_ BOOLEAN ForceFinalize);
#else
BOOLEAN
RxFinalizeSrvCall(
    _Out_ PSRV_CALL ThisSrvCall,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize);
#endif

PNET_ROOT
RxCreateNetRoot(
    _In_ PSRV_CALL SrvCall,
    _In_ PUNICODE_STRING Name,
    _In_ ULONG NetRootFlags,
    _In_opt_ PRX_CONNECTION_ID RxConnectionId);

#define RxWaitForStableNetRoot(N, R) RxWaitForStableCondition(&(N)->Condition, &(N)->TransitionWaitList, (R), NULL)
#define RxTransitionNetRoot(N, C) RxUpdateCondition((C), &(N)->Condition, &(N)->TransitionWaitList)

BOOLEAN
RxFinalizeNetRoot(
    _Out_ PNET_ROOT ThisNetRoot,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize);

NTSTATUS
RxInitializeVNetRootParameters(
   _In_ PRX_CONTEXT RxContext,
   _Out_ LUID *LogonId,
   _Out_ PULONG SessionId,
   _Out_ PUNICODE_STRING *UserNamePtr,
   _Out_ PUNICODE_STRING *UserDomainNamePtr,
   _Out_ PUNICODE_STRING *PasswordPtr,
   _Out_ PULONG Flags);

VOID
RxUninitializeVNetRootParameters(
   _In_ PUNICODE_STRING UserName,
   _In_ PUNICODE_STRING UserDomainName,
   _In_ PUNICODE_STRING Password,
   _Out_ PULONG Flags);

PV_NET_ROOT
RxCreateVNetRoot(
    _In_ PRX_CONTEXT RxContext,
    _In_ PNET_ROOT NetRoot,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ PUNICODE_STRING LocalNetRootName,
    _In_ PUNICODE_STRING FilePath,
    _In_ PRX_CONNECTION_ID RxConnectionId);

BOOLEAN
RxFinalizeVNetRoot(
    _Out_ PV_NET_ROOT ThisVNetRoot,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize);

#define RxWaitForStableVNetRoot(V, R) RxWaitForStableCondition(&(V)->Condition, &(V)->TransitionWaitList, (R), NULL)
#define RxTransitionVNetRoot(V, C) RxUpdateCondition((C), &(V)->Condition, &(V)->TransitionWaitList)

VOID
RxSetFileSizeWithLock(
    _Inout_ PFCB Fcb,
    _In_ PLONGLONG FileSize);

VOID
RxGetFileSizeWithLock(
    _In_ PFCB Fcb,
    _Out_ PLONGLONG FileSize);

#if (_WIN32_WINNT >= 0x0600)
PFCB
RxCreateNetFcb(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ PV_NET_ROOT VNetRoot,
    _In_ PUNICODE_STRING Name);
#else
PFCB
RxCreateNetFcb(
    _In_ PRX_CONTEXT RxContext,
    _In_ PV_NET_ROOT VNetRoot,
    _In_ PUNICODE_STRING Name);
#endif

#define RxWaitForStableNetFcb(F, R) RxWaitForStableCondition(&(F)->Condition, &(F)->NonPaged->TransitionWaitList, (R), NULL )
#define RxTransitionNetFcb(F, C) RxUpdateCondition((C), &(F)->Condition, &(F)->NonPaged->TransitionWaitList)

#define RxFormInitPacket(IP, I1, I1a, I2, I3, I4a, I4b, I5, I6, I7) ( \
    IP.pAttributes = I1, IP.pNumLinks = I1a,                          \
    IP.pCreationTime = I2, IP.pLastAccessTime = I3,                   \
    IP.pLastWriteTime = I4a, IP.pLastChangeTime = I4b,                \
    IP.pAllocationSize = I5, IP.pFileSize = I6,                       \
    IP.pValidDataLength = I7, &IP)

#if DBG
#define ASSERT_CORRECT_FCB_STRUCTURE_DBG_ONLY(Fcb) \
{                                                  \
    ASSERT(Fcb->NonPaged == Fcb->CopyOfNonPaged);  \
    ASSERT(Fcb->NonPaged->FcbBackPointer == Fcb);  \
}
#else
#define ASSERT_CORRECT_FCB_STRUCTURE_DBG_ONLY(Fcb)
#endif

#define ASSERT_CORRECT_FCB_STRUCTURE(Fcb)                      \
{                                                              \
    ASSERT(NodeTypeIsFcb(Fcb));                                \
    ASSERT(Fcb->NonPaged != NULL );                            \
    ASSERT(NodeType(Fcb->NonPaged) == RDBSS_NTC_NONPAGED_FCB); \
    ASSERT_CORRECT_FCB_STRUCTURE_DBG_ONLY(Fcb);                \
}

VOID
NTAPI
RxFinishFcbInitialization(
    _In_ OUT PMRX_FCB Fcb,
    _In_ RX_FILE_TYPE FileType,
    _In_opt_ PFCB_INIT_PACKET InitPacket);

#define RxWaitForStableSrvOpen(S, R) RxWaitForStableCondition(&(S)->Condition, &(S)->TransitionWaitList, (R), NULL)
#define RxTransitionSrvOpen(S, C) RxUpdateCondition((C), &(S)->Condition, &(S)->TransitionWaitList)

VOID
RxRemoveNameNetFcb(
    _Out_ PFCB ThisFcb);

LONG
RxpReferenceNetFcb(
   _In_ PFCB Fcb);

LONG
RxpDereferenceNetFcb(
   _In_ PFCB Fcb);

BOOLEAN
RxpDereferenceAndFinalizeNetFcb(
    _Out_ PFCB ThisFcb,
    _In_ PRX_CONTEXT RxContext,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize);

PSRV_OPEN
RxCreateSrvOpen(
    _In_ PV_NET_ROOT VNetRoot,
    _In_ OUT PFCB Fcb);

BOOLEAN
RxFinalizeSrvOpen(
    _Out_ PSRV_OPEN ThisSrvOpen,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize);

FORCEINLINE
PUNICODE_STRING
GET_ALREADY_PREFIXED_NAME(
    PMRX_SRV_OPEN SrvOpen,
    PMRX_FCB Fcb)
{
    PFCB ThisFcb = (PFCB)Fcb;

#if DBG
    if (SrvOpen != NULL)
    {
        ASSERT(NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN);
        ASSERT(ThisFcb != NULL);
        ASSERT(NodeTypeIsFcb(Fcb));
        ASSERT(SrvOpen->pFcb == Fcb);
        ASSERT(SrvOpen->pAlreadyPrefixedName == &ThisFcb->PrivateAlreadyPrefixedName);
    }
#endif

    return &ThisFcb->PrivateAlreadyPrefixedName;
}
#define GET_ALREADY_PREFIXED_NAME_FROM_CONTEXT(Rxcontext) GET_ALREADY_PREFIXED_NAME((Rxcontext)->pRelevantSrvOpen, (Rxcontext)->pFcb)

PMRX_FOBX
NTAPI
RxCreateNetFobx(
    _Out_ PRX_CONTEXT RxContext,
    _In_ PMRX_SRV_OPEN MrxSrvOpen);

BOOLEAN
RxFinalizeNetFobx(
    _Out_ PFOBX ThisFobx,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize);

#ifdef __REACTOS__
#define FILL_IN_FCB(Fcb, a, nl, ct, lat, lwt, lct, as, fs, vdl)  \
    (Fcb)->Attributes = a;                                       \
    (Fcb)->NumberOfLinks = nl;                                   \
    (Fcb)->CreationTime.QuadPart = ct;                           \
    (Fcb)->LastAccessTime.QuadPart = lat;                        \
    (Fcb)->LastWriteTime.QuadPart = lwt;                         \
    (Fcb)->LastChangeTime.QuadPart = lct;                        \
    (Fcb)->ActualAllocationLength = as;                          \
    (Fcb)->Header.AllocationSize.QuadPart = as;                  \
    (Fcb)->Header.FileSize.QuadPart = fs;                        \
    (Fcb)->Header.ValidDataLength.QuadPart = vdl;                \
    SetFlag((Fcb)->FcbState, FCB_STATE_TIME_AND_SIZE_ALREADY_SET)

#define TRACKER_ACQUIRE_FCB 0x61616161
#define TRACKER_RELEASE_FCB_NO_BUFF_PENDING 0x72727272
#define TRACKER_RELEASE_NON_EXCL_FCB_BUFF_PENDING 0x72727230
#define TRACKER_RELEASE_EXCL_FCB_BUFF_PENDING 0x72727231
#define TRACKER_RELEASE_FCB_FOR_THRD_NO_BUFF_PENDING 0x72727474
#define TRACKER_RELEASE_NON_EXCL_FCB_FOR_THRD_BUFF_PENDING 0x72727430
#define TRACKER_RELEASE_EXCL_FCB_FOR_THRD_BUFF_PENDING 0x72727431
#define TRACKER_FCB_FREE 0x72724372

#define FCB_STATE_BUFFERING_STATE_WITH_NO_SHARES \
    (( FCB_STATE_WRITECACHING_ENABLED            \
    | FCB_STATE_WRITEBUFFERING_ENABLED           \
    | FCB_STATE_READCACHING_ENABLED              \
    | FCB_STATE_READBUFFERING_ENABLED            \
    | FCB_STATE_LOCK_BUFFERING_ENABLED           \
    | FCB_STATE_FILESIZECACHEING_ENABLED         \
    | FCB_STATE_FILETIMECACHEING_ENABLED))

#endif

#endif
