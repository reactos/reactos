#ifndef _RDBSSPROCS_
#define _RDBSSPROCS_

#include "backpack.h"
#include "rxlog.h"
#include "rxtimer.h"
#include "rxstruc.h"

extern PVOID RxNull;

#define RxLogFailure(DO, Originator, Event, Status) \
    RxLogEventDirect(DO, Originator, Event, Status, __LINE__)

VOID
NTAPI
RxLogEventDirect(
    _In_ PRDBSS_DEVICE_OBJECT DeviceObject,
    _In_ PUNICODE_STRING OriginatorId,
    _In_ ULONG EventId,
    _In_ NTSTATUS Status,
    _In_ ULONG Line);

VOID
NTAPI
RxLogEventWithAnnotation(
    _In_ PRDBSS_DEVICE_OBJECT DeviceObject,
    _In_ ULONG EventId,
    _In_ NTSTATUS Status,
    _In_ PVOID DataBuffer,
    _In_ USHORT DataBufferLength,
    _In_ PUNICODE_STRING Annotation,
    _In_ ULONG AnnotationCount);

NTSTATUS
RxPrefixClaim(
    _In_ PRX_CONTEXT RxContext);

VOID
RxpPrepareCreateContextForReuse(
    _In_ PRX_CONTEXT RxContext);

NTSTATUS
RxLowIoCompletionTail(
    _In_ PRX_CONTEXT RxContext);

LUID
RxGetUid(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext);

ULONG
RxGetSessionId(
    _In_ PIO_STACK_LOCATION IrpSp);

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
RxFindOrCreateConnections(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ NET_ROOT_TYPE NetRootType,
    _In_ BOOLEAN TreeConnect,
    _Out_ PUNICODE_STRING LocalNetRootName,
    _Out_ PUNICODE_STRING FilePathName,
    _Inout_ PLOCK_HOLDING_STATE LockState,
    _In_ PRX_CONNECTION_ID RxConnectionId);
#else
NTSTATUS
RxFindOrCreateConnections(
    _In_ PRX_CONTEXT RxContext,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ NET_ROOT_TYPE NetRootType,
    _Out_ PUNICODE_STRING LocalNetRootName,
    _Out_ PUNICODE_STRING FilePathName,
    _Inout_ PLOCK_HOLDING_STATE LockState,
    _In_ PRX_CONNECTION_ID RxConnectionId);
#endif

typedef enum _RX_NAME_CONJURING_METHODS
{
    VNetRoot_As_Prefix,
    VNetRoot_As_UNC_Name,
    VNetRoot_As_DriveLetter
} RX_NAME_CONJURING_METHODS;

VOID
RxConjureOriginalName(
    _Inout_ PFCB Fcb,
    _Inout_ PFOBX Fobx,
    _Out_ PULONG ActualNameLength,
    _Out_writes_bytes_( *LengthRemaining) PWCHAR OriginalName,
    _Inout_ PLONG LengthRemaining,
    _In_ RX_NAME_CONJURING_METHODS NameConjuringMethod);

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
RxCompleteMdl(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp);
#else
NTSTATUS
NTAPI
RxCompleteMdl(
    _In_ PRX_CONTEXT RxContext);
#endif

#if (_WIN32_WINNT >= 0x0600)
VOID
RxLockUserBuffer(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ LOCK_OPERATION Operation,
    _In_ ULONG BufferLength);

PVOID
RxMapSystemBuffer(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp);
#else
VOID
RxLockUserBuffer(
    _In_ PRX_CONTEXT RxContext,
    _In_ LOCK_OPERATION Operation,
    _In_ ULONG BufferLength);

PVOID
RxMapSystemBuffer(
    _In_ PRX_CONTEXT RxContext);
#endif

#define FCB_MODE_EXCLUSIVE 1
#define FCB_MODE_SHARED 2
#define FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE 3
#define FCB_MODE_SHARED_STARVE_EXCLUSIVE 4

#define CHANGE_BUFFERING_STATE_CONTEXT ((PRX_CONTEXT)IntToPtr(0xffffffff))
#define CHANGE_BUFFERING_STATE_CONTEXT_WAIT ((PRX_CONTEXT)IntToPtr(0xfffffffe))

NTSTATUS
__RxAcquireFcb(
    _Inout_ PFCB Fcb,
    _Inout_opt_ PRX_CONTEXT RxContext,
    _In_ ULONG Mode
#ifdef RDBSS_TRACKER
    ,
    _In_ ULONG LineNumber,
    _In_ PCSTR FileName,
    _In_ ULONG SerialNumber
#endif
    );

#ifdef  RDBSS_TRACKER
#define RxAcquireExclusiveFcb(R, F) __RxAcquireFcb((F), (R), FCB_MODE_EXCLUSIVE, __LINE__, __FILE__, 0)
#else
#define RxAcquireExclusiveFcb(R, F) __RxAcquireFcb((F), (R), FCB_MODE_EXCLUSIVE)
#endif

#define RX_GET_MRX_FCB(F) ((PMRX_FCB)((F)))

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcb(R, F) __RxAcquireFcb((F), (R), FCB_MODE_SHARED, __LINE__, __FILE__, 0)
#else
#define RxAcquireSharedFcb(R, F) __RxAcquireFcb((F), (R), FCB_MODE_SHARED)
#endif

#ifdef  RDBSS_TRACKER
#define RxAcquireSharedFcbWaitForEx(R, F) __RxAcquireFcb((F),(R), FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE, __LINE__, __FILE__,0)
#else
#define RxAcquireSharedFcbWaitForEx(R, F) __RxAcquireFcb((F), (R), FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE)
#endif

VOID
__RxReleaseFcb(
    _Inout_opt_ PRX_CONTEXT RxContext,
    _Inout_ PMRX_FCB MrxFcb
#ifdef RDBSS_TRACKER
    ,
    _In_ ULONG LineNumber,
    _In_ PCSTR FileName,
    _In_ ULONG SerialNumber
#endif
    );

#ifdef  RDBSS_TRACKER
#define RxReleaseFcb(R, F) __RxReleaseFcb((R), RX_GET_MRX_FCB(F), __LINE__, __FILE__, 0)
#else
#define RxReleaseFcb(R, F) __RxReleaseFcb((R), RX_GET_MRX_FCB(F))
#endif

VOID
__RxReleaseFcbForThread(
    _Inout_opt_ PRX_CONTEXT RxContext,
    _Inout_ PMRX_FCB MrxFcb,
    _In_ ERESOURCE_THREAD ResourceThreadId
#ifdef RDBSS_TRACKER
    ,
    _In_ ULONG LineNumber,
    _In_ PCSTR FileName,
    _In_ ULONG SerialNumber
#endif
    );

#ifdef  RDBSS_TRACKER
#define RxReleaseFcbForThread(R, F, T) __RxReleaseFcbForThread((R), RX_GET_MRX_FCB(F), (T), __LINE__, __FILE__, 0)
#else
#define RxReleaseFcbForThread(R, F, T) __RxReleaseFcbForThread((R), RX_GET_MRX_FCB(F), (T))
#endif

#ifdef RDBSS_TRACKER
VOID
RxTrackerUpdateHistory(
    _Inout_opt_ PRX_CONTEXT RxContext,
    _Inout_ PMRX_FCB MrxFcb,
    _In_ ULONG Operation,
    _In_ ULONG LineNumber,
    _In_ PCSTR FileName,
    _In_ ULONG SerialNumber);
#else
#define RxTrackerUpdateHistory(R, F, O, L, F, S) { NOTHING; }
#endif

VOID
RxTrackPagingIoResource(
    _Inout_ PVOID Instance,
    _In_ ULONG Type,
    _In_ ULONG Line,
    _In_ PCSTR File);

#define RxIsFcbAcquiredShared(Fcb) ExIsResourceAcquiredSharedLite((Fcb)->Header.Resource)
#define RxIsFcbAcquiredExclusive(Fcb) ExIsResourceAcquiredExclusiveLite((Fcb)->Header.Resource)
#define RxIsFcbAcquired(Fcb) (ExIsResourceAcquiredSharedLite((Fcb)->Header.Resource) ||  \
			      ExIsResourceAcquiredExclusiveLite((Fcb)->Header.Resource))

#define RxAcquirePagingIoResource(RxContext, Fcb)                         \
    ExAcquireResourceExclusiveLite((Fcb)->Header.PagingIoResource, TRUE); \
    if (RxContext != NULL)                                                \
    {                                                                     \
        (RxContext)->FcbPagingIoResourceAcquired = TRUE;                  \
    }                                                                     \
    RxTrackPagingIoResource(Fcb, 1, __LINE__, __FILE__)

#ifndef __REACTOS__
#define RxAcquirePagingIoResourceShared(RxContext, Fcb, Flag)             \
    ExAcquireResourceSharedLite((Fcb)->Header.PagingIoResource, Flag);    \
    if (AcquiredFile)                                                     \
    {                                                                     \
        if (RxContext != NULL)                                            \
	{                                                                 \
            ((PRX_CONTEXT)RxContext)->FcbPagingIoResourceAcquired = TRUE; \
        }                                                                 \
        RxTrackPagingIoResource(Fcb, 2, __LINE__, __FILE__);              \
    }
#else
#define RxAcquirePagingIoResourceShared(RxContext, Fcb, Flag)                             \
    {                                                                                     \
        BOOLEAN AcquiredFile;                                                             \
        AcquiredFile = ExAcquireResourceSharedLite((Fcb)->Header.PagingIoResource, Flag); \
        if (AcquiredFile)                                                                 \
        {                                                                                 \
            if (RxContext != NULL)                                                        \
	    {                                                                             \
                ((PRX_CONTEXT)RxContext)->FcbPagingIoResourceAcquired = TRUE;             \
            }                                                                             \
            RxTrackPagingIoResource(Fcb, 2, __LINE__, __FILE__);                          \
        }                                                                                 \
    }
#endif

#define RxReleasePagingIoResource(RxContext, Fcb)         \
    RxTrackPagingIoResource(Fcb, 3, __LINE__, __FILE__);  \
    if (RxContext != NULL)                                \
    {                                                     \
        (RxContext)->FcbPagingIoResourceAcquired = FALSE; \
    }                                                     \
    ExReleaseResourceLite((Fcb)->Header.PagingIoResource)

#define RxReleasePagingIoResourceForThread(RxContext, Fcb, Thread) \
    RxTrackPagingIoResource(Fcb, 3, __LINE__, __FILE__);           \
    if (RxContext != NULL)                                         \
    {                                                              \
        (RxContext)->FcbPagingIoResourceAcquired = FALSE;          \
    }                                                              \
    ExReleaseResourceForThreadLite((Fcb)->Header.PagingIoResource, (Thread))

#ifdef __REACTOS__
VOID
__RxWriteReleaseResources(
    PRX_CONTEXT RxContext,
    BOOLEAN ResourceOwnerSet
#ifdef RDBSS_TRACKER
    ,
    ULONG LineNumber,
    PCSTR FileName,
    ULONG SerialNumber
#endif
    );

#ifdef  RDBSS_TRACKER
#define RxWriteReleaseResources(R, B) __RxWriteReleaseResources((R), (B), __LINE__, __FILE__, 0)
#else
#define RxWriteReleaseResources(R, B) __RxWriteReleaseResources((R), (B))
#endif
#endif

BOOLEAN
NTAPI
RxAcquireFcbForLazyWrite(
    _In_ PVOID Null,
    _In_ BOOLEAN Wait);

VOID
NTAPI
RxReleaseFcbFromLazyWrite(
    _In_ PVOID Null);

BOOLEAN
NTAPI
RxAcquireFcbForReadAhead(
    _In_ PVOID Null,
    _In_ BOOLEAN Wait);

VOID
NTAPI
RxReleaseFcbFromReadAhead(
    _In_ PVOID Null);

BOOLEAN
NTAPI
RxNoOpAcquire(
    _In_ PVOID Fcb,
    _In_ BOOLEAN Wait);

VOID
NTAPI
RxNoOpRelease(
    _In_ PVOID Fcb);

#define RxConvertToSharedFcb(R, F) ExConvertExclusiveToSharedLite(RX_GET_MRX_FCB(F)->Header.Resource)

VOID
RxVerifyOperationIsLegal(
    _In_ PRX_CONTEXT RxContext);

VOID
RxPrePostIrp(
    _In_ PVOID Context,
    _In_ PIRP Irp);

VOID
NTAPI
RxAddToWorkque(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp);

NTSTATUS
RxFsdPostRequest(
    _In_ PRX_CONTEXT RxContext);

#define QuadAlign(V) (ALIGN_UP(V, ULONGLONG))

VOID
RxCompleteRequest_Real(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ NTSTATUS Status);

NTSTATUS
RxCompleteRequest(
    _In_ PRX_CONTEXT pContext,
    _In_ NTSTATUS Status);

#if (_WIN32_WINNT >= 0x600)
NTSTATUS
RxConstructSrvCall(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ PSRV_CALL SrvCall,
    _Out_ PLOCK_HOLDING_STATE LockHoldingState);
#else
NTSTATUS
RxConstructSrvCall(
    _In_ PRX_CONTEXT RxContext,
    _In_ PSRV_CALL SrvCall,
    _Out_ PLOCK_HOLDING_STATE LockHoldingState);
#endif

#define RxCompleteAsynchronousRequest(C, S) RxCompleteRequest(C, S)

NTSTATUS
RxConstructNetRoot(
    _In_ PRX_CONTEXT RxContext,
    _In_ PSRV_CALL SrvCall,
    _In_ PNET_ROOT NetRoot,
    _In_ PV_NET_ROOT VirtualNetRoot,
    _Out_ PLOCK_HOLDING_STATE LockHoldingState);

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
RxConstructVirtualNetRoot(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ NET_ROOT_TYPE NetRootType,
    _In_ BOOLEAN TreeConnect,
    _Out_ PV_NET_ROOT *VirtualNetRootPointer,
    _Out_ PLOCK_HOLDING_STATE LockHoldingState,
    _Out_ PRX_CONNECTION_ID  RxConnectionId);

NTSTATUS
RxFindOrConstructVirtualNetRoot(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ NET_ROOT_TYPE NetRootType,
    _In_ PUNICODE_STRING RemainingName);
#else
NTSTATUS
RxConstructVirtualNetRoot(
    _In_ PRX_CONTEXT RxContext,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ NET_ROOT_TYPE NetRootType,
    _Out_ PV_NET_ROOT *VirtualNetRootPointer,
    _Out_ PLOCK_HOLDING_STATE LockHoldingState,
    _Out_ PRX_CONNECTION_ID  RxConnectionId);

NTSTATUS
RxFindOrConstructVirtualNetRoot(
    _In_ PRX_CONTEXT RxContext,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ NET_ROOT_TYPE NetRootType,
    _In_ PUNICODE_STRING RemainingName);
#endif

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
RxLowIoLockControlShell(
    _In_ PRX_CONTEXT RxContext,
    _In_ PIRP Irp,
    _In_ PFCB Fcb);
#else
NTSTATUS
RxLowIoLockControlShell(
    _In_ PRX_CONTEXT RxContext);
#endif

NTSTATUS
NTAPI
RxChangeBufferingState(
    PSRV_OPEN SrvOpen,
    PVOID Context,
    BOOLEAN ComputeNewState);

VOID
NTAPI
RxIndicateChangeOfBufferingStateForSrvOpen(
    PMRX_SRV_CALL SrvCall,
    PMRX_SRV_OPEN SrvOpen,
    PVOID SrvOpenKey,
    PVOID Context);

NTSTATUS
NTAPI
RxPrepareToReparseSymbolicLink(
    PRX_CONTEXT RxContext,
    BOOLEAN SymbolicLinkEmbeddedInOldPath,
    PUNICODE_STRING NewPath,
    BOOLEAN NewPathIsAbsolute,
    PBOOLEAN ReparseRequired);

VOID
RxReference(
    _Inout_ PVOID Instance);

VOID
RxDereference(
    _Inout_ PVOID Instance,
    _In_ LOCK_HOLDING_STATE LockHoldingState);

VOID
RxWaitForStableCondition(
    _In_ PRX_BLOCK_CONDITION Condition,
    _Inout_ PLIST_ENTRY TransitionWaitList,
    _Inout_ PRX_CONTEXT RxContext,
    _Out_opt_ NTSTATUS *AsyncStatus);

VOID
RxUpdateCondition(
    _In_ RX_BLOCK_CONDITION NewConditionValue,
    _Out_ PRX_BLOCK_CONDITION Condition,
    _In_ OUT PLIST_ENTRY TransitionWaitList);

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
RxCloseAssociatedSrvOpen(
    _In_opt_ PRX_CONTEXT RxContext,
    _In_ PFOBX Fobx);
#else
NTSTATUS
RxCloseAssociatedSrvOpen(
    _In_ PFOBX Fobx,
    _In_opt_ PRX_CONTEXT RxContext);
#endif

NTSTATUS
NTAPI
RxFinalizeConnection(
    _Inout_ PNET_ROOT NetRoot,
    _Inout_opt_ PV_NET_ROOT VNetRoot,
    _In_ LOGICAL ForceFilesClosed);

#if DBG
VOID
RxDumpWantedAccess(
    _In_ PSZ where1,
    _In_ PSZ where2,
    _In_ PSZ wherelogtag,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG DesiredShareAccess);

VOID
RxDumpCurrentAccess(
    _In_ PSZ where1,
    _In_ PSZ where2,
    _In_ PSZ wherelogtag,
    _In_ PSHARE_ACCESS ShareAccess);
#else
#define RxDumpWantedAccess(w1,w2,wlt,DA,DSA) {NOTHING;}
#define RxDumpCurrentAccess(w1,w2,wlt,SA)  {NOTHING;}
#endif

NTSTATUS
RxCheckShareAccessPerSrvOpens(
    _In_ PFCB Fcb,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG DesiredShareAccess);

VOID
RxUpdateShareAccessPerSrvOpens(
    _In_ PSRV_OPEN SrvOpen);

VOID
RxRemoveShareAccessPerSrvOpens(
    _Inout_ PSRV_OPEN SrvOpen);

#if DBG
NTSTATUS
RxCheckShareAccess(
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG DesiredShareAccess,
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PSHARE_ACCESS ShareAccess,
    _In_ BOOLEAN Update,
    _In_ PSZ where,
    _In_ PSZ wherelogtag);

VOID
RxRemoveShareAccess(
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PSHARE_ACCESS ShareAccess,
    _In_ PSZ where,
    _In_ PSZ wherelogtag);

VOID
RxSetShareAccess(
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ULONG DesiredShareAccess,
    _Inout_ PFILE_OBJECT FileObject,
    _Out_ PSHARE_ACCESS ShareAccess,
    _In_ PSZ where,
    _In_ PSZ wherelogtag);

VOID
RxUpdateShareAccess(
    _Inout_ PFILE_OBJECT FileObject,
    _Inout_ PSHARE_ACCESS ShareAccess,
    _In_ PSZ where,
    _In_ PSZ wherelogtag);
#else
#define RxCheckShareAccess(a1, a2, a3, a4, a5, a6, a7) IoCheckShareAccess(a1, a2, a3, a4, a5)
#define RxRemoveShareAccess(a1, a2, a3, a4) IoRemoveShareAccess(a1, a2)
#define RxSetShareAccess(a1, a2, a3, a4, a5, a6) IoSetShareAccess(a1, a2, a3, a4)
#define RxUpdateShareAccess(a1, a2, a3, a4) IoUpdateShareAccess(a1, a2)
#endif

NTSTATUS
NTAPI
RxDriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath);

VOID
NTAPI
RxUnload(
    _In_ PDRIVER_OBJECT DriverObject);

VOID
RxInitializeMinirdrDispatchTable(
    _In_ PDRIVER_OBJECT DriverObject);

ULONG
RxGetNetworkProviderPriority(
    _In_ PUNICODE_STRING DeviceName);

VOID
RxPrepareRequestForReuse(
    PCHANGE_BUFFERING_STATE_REQUEST Request);

VOID
RxpDiscardChangeBufferingStateRequests(
    _Inout_ PLIST_ENTRY DiscardedRequests);

VOID
RxGatherRequestsForSrvOpen(
    _Inout_ PSRV_CALL SrvCall,
    _In_ PSRV_OPEN SrvOpen,
    _Inout_ PLIST_ENTRY RequestsListHead);

NTSTATUS
RxpLookupSrvOpenForRequestLite(
    _In_ PSRV_CALL SrvCall,
    _Inout_ PCHANGE_BUFFERING_STATE_REQUEST Request);

VOID
RxProcessChangeBufferingStateRequestsForSrvOpen(
    PSRV_OPEN SrvOpen);

NTSTATUS
RxPurgeFobxFromCache(
    PFOBX FobxToBePurged);

BOOLEAN
RxPurgeFobx(
    PFOBX pFobx);

VOID
RxUndoScavengerFinalizationMarking(
    PVOID Instance);

ULONG
RxTableComputePathHashValue(
    _In_ PUNICODE_STRING Name);

VOID
RxExtractServerName(
    _In_ PUNICODE_STRING FilePathName,
    _Out_ PUNICODE_STRING SrvCallName,
    _Out_ PUNICODE_STRING RestOfName);

VOID
NTAPI
RxCreateNetRootCallBack(
    _In_ PMRX_CREATENETROOT_CONTEXT CreateNetRootContext);

PVOID
RxAllocateObject(
    _In_ NODE_TYPE_CODE NodeType,
    _In_opt_ PMINIRDR_DISPATCH MRxDispatch,
    _In_ ULONG NameLength);

VOID
RxFreeObject(
    _In_ PVOID pObject);

NTSTATUS
RxInitializeSrvCallParameters(
    _In_ PRX_CONTEXT RxContext,
    _Inout_ PSRV_CALL SrvCall);

VOID
RxAddVirtualNetRootToNetRoot(
    _In_ PNET_ROOT NetRoot,
    _In_ PV_NET_ROOT VNetRoot);

VOID
RxRemoveVirtualNetRootFromNetRoot(
    _In_ PNET_ROOT NetRoot,
    _In_ PV_NET_ROOT VNetRoot);

PVOID
RxAllocateFcbObject(
    _In_ PRDBSS_DEVICE_OBJECT RxDeviceObject,
    _In_ NODE_TYPE_CODE NodeType,
    _In_ POOL_TYPE PoolType,
    _In_ ULONG NameSize,
    _In_opt_ PVOID AlreadyAllocatedObject);

VOID
RxFreeFcbObject(
    _In_ PVOID Object);

VOID
RxPurgeFcb(
    _In_ PFCB Fcb);

BOOLEAN
RxFinalizeNetFcb(
    _Out_ PFCB ThisFcb,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize,
    _In_ LONG ReferenceCount);

BOOLEAN
RxIsThisACscAgentOpen(
    _In_ PRX_CONTEXT RxContext);

VOID
NTAPI
RxCheckFcbStructuresForAlignment(
    VOID);

NTSTATUS
RxInitializeWorkQueueDispatcher(
   _In_ PRX_WORK_QUEUE_DISPATCHER Dispatcher);

VOID
RxInitializeWorkQueue(
   _In_ PRX_WORK_QUEUE WorkQueue,
   _In_ WORK_QUEUE_TYPE WorkQueueType,
   _In_ ULONG MaximumNumberOfWorkerThreads,
   _In_ ULONG MinimumNumberOfWorkerThreads);

NTSTATUS
RxSpinUpWorkerThread(
   _In_ PRX_WORK_QUEUE WorkQueue,
   _In_ PRX_WORKERTHREAD_ROUTINE Routine,
   _In_ PVOID Parameter);

VOID
RxSpinUpWorkerThreads(
   _In_ PRX_WORK_QUEUE WorkQueue);

VOID
NTAPI
RxSpinUpRequestsDispatcher(
    _In_ PVOID Dispatcher);

VOID
RxpWorkerThreadDispatcher(
   _In_ PRX_WORK_QUEUE WorkQueue,
   _In_ PLARGE_INTEGER WaitInterval);

VOID
NTAPI
RxBootstrapWorkerThreadDispatcher(
   _In_ PVOID WorkQueue);

PRX_PREFIX_ENTRY
RxTableLookupName_ExactLengthMatch(
    _In_ PRX_PREFIX_TABLE ThisTable,
    _In_ PUNICODE_STRING  Name,
    _In_ ULONG HashValue,
    _In_opt_ PRX_CONNECTION_ID RxConnectionId);

PVOID
RxTableLookupName(
    _In_ PRX_PREFIX_TABLE ThisTable,
    _In_ PUNICODE_STRING Name,
    _Out_ PUNICODE_STRING RemainingName,
    _In_opt_ PRX_CONNECTION_ID RxConnectionId);

VOID
RxOrphanSrvOpens(
    _In_ PV_NET_ROOT ThisVNetRoot);

VOID
RxOrphanThisFcb(
    _In_ PFCB Fcb);

VOID
RxOrphanSrvOpensForThisFcb(
    _In_ PFCB Fcb,
    _In_ PV_NET_ROOT ThisVNetRoot,
    _In_ BOOLEAN OrphanAll);

#define RxEqualConnectionId(C1, C2) RtlEqualMemory(C1, C2, sizeof(RX_CONNECTION_ID))

NTSTATUS
NTAPI
RxLockOperationCompletion(
    _In_ PVOID Context,
    _In_ PIRP Irp);

VOID
NTAPI
RxUnlockOperation(
    _In_ PVOID Context,
    _In_ PFILE_LOCK_INFO LockInfo);

#if (_WIN32_WINNT >= 0x0600)
NTSTATUS
RxPostStackOverflowRead(
    _In_ PRX_CONTEXT RxContext,
    _In_ PFCB Fcb);
#else
NTSTATUS
RxPostStackOverflowRead(
    _In_ PRX_CONTEXT RxContext);
#endif

VOID
NTAPI
RxCancelRoutine(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

#ifdef __REACTOS__
#define RxWriteCacheingAllowed(F, S) (                              \
    BooleanFlagOn((F)->FcbState, FCB_STATE_WRITECACHING_ENABLED) && \
    !BooleanFlagOn((S)->Flags, SRVOPEN_FLAG_DONTUSE_WRITE_CACHING))
#endif

#endif
