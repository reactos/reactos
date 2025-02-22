/*
 *  ReactOS kernel
 *  Copyright (C) 2017 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             sdk/lib/drivers/rxce/rxce.c
 * PURPOSE:          RXCE library
 * PROGRAMMER:       Pierre Schweitzer (pierre@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <rx.h>
#include <pseh/pseh2.h>
#include <dfs.h>

#define NDEBUG
#include <debug.h>

VOID
RxAssert(
    PVOID Assert,
    PVOID File,
    ULONG Line,
    PVOID Message);

VOID
NTAPI
RxCreateSrvCallCallBack(
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT Context);

NTSTATUS
RxFinishSrvCallConstruction(
    PMRX_SRVCALLDOWN_STRUCTURE Calldown);

VOID
NTAPI
RxFinishSrvCallConstructionDispatcher(
    IN PVOID Context);

NTSTATUS
RxInsertWorkQueueItem(
    PRDBSS_DEVICE_OBJECT pMRxDeviceObject,
    WORK_QUEUE_TYPE WorkQueueType,
    PRX_WORK_QUEUE_ITEM WorkQueueItem);

PVOID
RxNewMapUserBuffer(
    PRX_CONTEXT RxContext);

VOID
NTAPI
RxpDestroySrvCall(
    IN PVOID Context);

VOID
RxpDispatchChangeBufferingStateRequests(
    PSRV_CALL SrvCall,
    PSRV_OPEN SrvOpen,
    PLIST_ENTRY DiscardedRequests);

VOID
NTAPI
RxScavengerTimerRoutine(
    PVOID Context);

VOID
NTAPI
RxTimerDispatch(
    _In_ struct _KDPC *Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2);

VOID
NTAPI
RxWorkItemDispatcher(
    PVOID Context);

PVOID
NTAPI
_RxAllocatePoolWithTag(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag);

VOID
NTAPI
_RxFreePool(
    _In_ PVOID Buffer);

VOID
NTAPI
_RxFreePoolWithTag(
    _In_ PVOID Buffer,
    _In_ ULONG Tag);

extern ULONG ReadAheadGranularity;

volatile LONG RxNumberOfActiveFcbs = 0;
ULONG SerialNumber = 1;
PVOID RxNull =  NULL;
volatile ULONG RxContextSerialNumberCounter;
BOOLEAN RxStopOnLoudCompletion = TRUE;
BOOLEAN RxSrvCallConstructionDispatcherActive = FALSE;
LIST_ENTRY RxSrvCalldownList;
RX_SPIN_LOCK RxStrucSupSpinLock;
#if 0
ULONG RdbssReferenceTracingValue = (RDBSS_REF_TRACK_SRVCALL | RDBSS_REF_TRACK_NETROOT |
                                    RDBSS_REF_TRACK_VNETROOT | RDBSS_REF_TRACK_NETFOBX |
                                    RDBSS_REF_TRACK_NETFCB | RDBSS_REF_TRACK_SRVOPEN |
                                    RX_PRINT_REF_TRACKING);
#else
ULONG RdbssReferenceTracingValue = 0;
#endif
LARGE_INTEGER RxWorkQueueWaitInterval[RxMaximumWorkQueue];
LARGE_INTEGER RxSpinUpDispatcherWaitInterval;
RX_DISPATCHER RxDispatcher;
RX_WORK_QUEUE_DISPATCHER RxDispatcherWorkQueues;
FAST_MUTEX RxLowIoPagingIoSyncMutex;
BOOLEAN RxContinueFromAssert = TRUE;
ULONG RxExplodePoolTags = 1;
LARGE_INTEGER RxTimerInterval;
RX_SPIN_LOCK RxTimerLock;
LIST_ENTRY RxTimerQueueHead;
LIST_ENTRY RxRecurrentWorkItemsList;
KDPC RxTimerDpc;
KTIMER RxTimer;
ULONG RxTimerTickCount;
FAST_MUTEX RxContextPerFileSerializationMutex;
#if DBG
BOOLEAN DumpDispatchRoutine = TRUE;
#else
BOOLEAN DumpDispatchRoutine = FALSE;
#endif

#if RDBSS_ASSERTS
#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT(exp)                               \
    if (!(exp))                                   \
    {                                             \
        RxAssert(#exp, __FILE__, __LINE__, NULL); \
    }
#endif

#if RX_POOL_WRAPPER
#undef RxAllocatePool
#undef RxAllocatePoolWithTag
#undef RxFreePool

#define RxAllocatePool(P, S) _RxAllocatePoolWithTag(P, S, 0)
#define RxAllocatePoolWithTag _RxAllocatePoolWithTag
#define RxFreePool _RxFreePool
#define RxFreePoolWithTag _RxFreePoolWithTag
#endif

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxAcquireExclusiveFcbResourceInMRx(
    _Inout_ PMRX_FCB Fcb)
{
    return RxAcquireExclusiveFcb(NULL, (PFCB)Fcb);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RxAcquireFcbForLazyWrite(
    PVOID Context,
    BOOLEAN Wait)
{
    PFCB Fcb;
    BOOLEAN Ret;

    PAGED_CODE();

    Fcb = Context;
    /* The received context is a FCB */
    ASSERT(NodeType(Fcb) == RDBSS_NTC_FCB);
    ASSERT_CORRECT_FCB_STRUCTURE(Fcb);
    ASSERT(Fcb->Specific.Fcb.LazyWriteThread == NULL);

    /* Acquire the paging resource (shared) */
    Ret = ExAcquireResourceSharedLite(Fcb->Header.PagingIoResource, Wait);
    if (Ret)
    {
        /* Update tracker information */
        Fcb->PagingIoResourceFile = __FILE__;
        Fcb->PagingIoResourceLine = __LINE__;
        /* Lazy writer thread is the current one */
        Fcb->Specific.Fcb.LazyWriteThread = PsGetCurrentThread();

        /* There is no top level IRP */
        ASSERT(RxIsThisTheTopLevelIrp(NULL));
        /* Now, there will be! */
        Ret = RxTryToBecomeTheTopLevelIrp(NULL, (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP,
                                          Fcb->RxDeviceObject, TRUE);
        /* In case of failure, release the lock and reset everything */
        if (!Ret)
        {
            Fcb->PagingIoResourceFile = NULL;
            Fcb->PagingIoResourceLine = 0;
            ExReleaseResourceLite(Fcb->Header.PagingIoResource);
            Fcb->Specific.Fcb.LazyWriteThread = NULL;
        }
    }

    return Ret;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
RxAcquireFcbForReadAhead(
    PVOID Context,
    BOOLEAN Wait)
{
    PFCB Fcb;
    BOOLEAN Ret;

    PAGED_CODE();

    Fcb = Context;
    /* The received context is a FCB */
    ASSERT(NodeType(Fcb) == RDBSS_NTC_FCB);
    ASSERT_CORRECT_FCB_STRUCTURE(Fcb);

    Ret = ExAcquireResourceSharedLite(Fcb->Header.Resource, Wait);
    if (Ret)
    {
        /* There is no top level IRP */
        ASSERT(RxIsThisTheTopLevelIrp(NULL));
        /* Now, there will be! */
        Ret = RxTryToBecomeTheTopLevelIrp(NULL, (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP,
                                          Fcb->RxDeviceObject, TRUE);
        /* In case of failure, release the lock and reset everything */
        if (!Ret)
        {
            ExReleaseResourceLite(Fcb->Header.Resource);
        }
    }

    return Ret;
}

VOID
NTAPI
RxAcquireFileForNtCreateSection(
    PFILE_OBJECT FileObject)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
RxAcquireForCcFlush(
    PFILE_OBJECT FileObject,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxAddVirtualNetRootToNetRoot(
    PNET_ROOT NetRoot,
    PV_NET_ROOT VNetRoot)
{
    PAGED_CODE();

    DPRINT("RxAddVirtualNetRootToNetRoot(%p, %p)\n", NetRoot, VNetRoot);

    /* Insert in the VNetRoot list - make sure lock is held */
    ASSERT(RxIsPrefixTableLockExclusive(NetRoot->SrvCall->RxDeviceObject->pRxNetNameTable));

    VNetRoot->pNetRoot = (PMRX_NET_ROOT)NetRoot;
    ++NetRoot->NumberOfVirtualNetRoots;
    InsertTailList(&NetRoot->VirtualNetRoots, &VNetRoot->NetRootListEntry);
}

/*
 * @implemented
 */
PVOID
RxAllocateFcbObject(
    PRDBSS_DEVICE_OBJECT RxDeviceObject,
    NODE_TYPE_CODE NodeType,
    POOL_TYPE PoolType,
    ULONG NameSize,
    PVOID AlreadyAllocatedObject)
{
    PFCB Fcb;
    PFOBX Fobx;
    PSRV_OPEN SrvOpen;
    PVOID Buffer, PAPNBuffer;
    PNON_PAGED_FCB NonPagedFcb;
    PMINIRDR_DISPATCH Dispatch;
    ULONG NonPagedSize, FobxSize, SrvOpenSize, FcbSize;

    PAGED_CODE();

    Dispatch = RxDeviceObject->Dispatch;

    NonPagedSize = 0;
    FobxSize = 0;
    SrvOpenSize = 0;
    FcbSize = 0;

    Fcb = NULL;
    Fobx = NULL;
    SrvOpen = NULL;
    NonPagedFcb = NULL;
    PAPNBuffer = NULL;

    /* If we ask for FOBX, just allocate FOBX and its extension if asked */
    if (NodeType == RDBSS_NTC_FOBX)
    {
        FobxSize = sizeof(FOBX);
        if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_FOBX_EXTENSION))
        {
            FobxSize += QuadAlign(Dispatch->MRxFobxSize);
        }
    }
    /* If we ask for SRV_OPEN, also allocate the "internal" FOBX and the extensions if asked */
    else if (NodeType == RDBSS_NTC_SRVOPEN || NodeType == RDBSS_NTC_INTERNAL_SRVOPEN)
    {
        SrvOpenSize = sizeof(SRV_OPEN);
        if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_SRV_OPEN_EXTENSION))
        {
            SrvOpenSize += QuadAlign(Dispatch->MRxSrvOpenSize);
        }

        FobxSize = sizeof(FOBX);
        if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_FOBX_EXTENSION))
        {
            FobxSize += QuadAlign(Dispatch->MRxFobxSize);
        }
    }
    /* Otherwise, we're asked to allocate a FCB */
    else
    {
        /* So, allocate the FCB and its extension if asked */
        FcbSize = sizeof(FCB);
        if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_FCB_EXTENSION))
        {
            FcbSize += QuadAlign(Dispatch->MRxFcbSize);
        }

        /* If we're asked to allocate from nonpaged, also allocate the NON_PAGED_FCB
         * Otherwise, it will be allocated later on, specifically
         */
        if (PoolType == NonPagedPool)
        {
            NonPagedSize = sizeof(NON_PAGED_FCB);
        }

        /* And if it's not for a rename operation also allcoate the internal SRV_OPEN and FOBX and their extensions */
        if (NodeType != RDBSS_NTC_OPENTARGETDIR_FCB)
        {
            SrvOpenSize = sizeof(SRV_OPEN);
            if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_SRV_OPEN_EXTENSION))
            {
                SrvOpenSize += QuadAlign(Dispatch->MRxSrvOpenSize);
            }

            FobxSize = sizeof(FOBX);
            if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_FOBX_EXTENSION))
            {
                FobxSize += QuadAlign(Dispatch->MRxFobxSize);
            }
        }
    }

    /* If we already have a buffer, go ahead */
    if (AlreadyAllocatedObject != NULL)
    {
        Buffer = AlreadyAllocatedObject;
    }
    /* Otherwise, allocate it */
    else
    {
        Buffer = RxAllocatePoolWithTag(PoolType, NameSize + FcbSize + SrvOpenSize + FobxSize + NonPagedSize, RX_FCB_POOLTAG);
        if (Buffer == NULL)
        {
            return NULL;
        }
    }

    /* Now, get the pointers - FOBX is easy */
    if (NodeType == RDBSS_NTC_FOBX)
    {
        Fobx = Buffer;
    }
    /* SRV_OPEN first, FOBX next */
    else if (NodeType == RDBSS_NTC_SRVOPEN)
    {
        SrvOpen = Buffer;
        Fobx = Add2Ptr(Buffer, SrvOpenSize);
    }
    else if (NodeType == RDBSS_NTC_INTERNAL_SRVOPEN)
    {
        SrvOpen = Buffer;
    }
    else
    {
        /* FCB first, and if needed, SRV_OPEN next, FOBX last */
        Fcb = Buffer;
        if (NodeType != RDBSS_NTC_OPENTARGETDIR_FCB)
        {
            SrvOpen = Add2Ptr(Buffer, FcbSize);
            Fobx = Add2Ptr(Buffer, FcbSize + SrvOpenSize);
        }

        /* If we were not allocated from non paged, allocate the NON_PAGED_FCB now */
        if (PoolType != NonPagedPool)
        {
            NonPagedFcb = RxAllocatePoolWithTag(NonPagedPool, sizeof(NON_PAGED_FCB), RX_NONPAGEDFCB_POOLTAG);
            if (NonPagedFcb == NULL)
            {
                RxFreePoolWithTag(Buffer, RX_FCB_POOLTAG);
                return NULL;
            }

            PAPNBuffer = Add2Ptr(Buffer, FcbSize + SrvOpenSize + FobxSize);
        }
        /* Otherwise, just point at the right place in what has been allocated previously */
        else
        {
            NonPagedFcb = Add2Ptr(Fobx, FobxSize);
            PAPNBuffer = Add2Ptr(Fobx, FobxSize + NonPagedSize);
        }
    }

    /* If we have allocated a SRV_OPEN, initialize it */
    if (SrvOpen != NULL)
    {
        ZeroAndInitializeNodeType(SrvOpen, RDBSS_NTC_SRVOPEN, SrvOpenSize);

        if (NodeType == RDBSS_NTC_SRVOPEN)
        {
            SrvOpen->InternalFobx = Fobx;
        }
        else
        {
            SrvOpen->InternalFobx = NULL;
            SrvOpen->Flags |= SRVOPEN_FLAG_FOBX_USED;
        }

        if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_SRV_OPEN_EXTENSION))
        {
            SrvOpen->Context = Add2Ptr(SrvOpen, sizeof(SRV_OPEN));
        }

        InitializeListHead(&SrvOpen->SrvOpenQLinks);
    }

    /* If we have allocated a FOBX, initialize it */
    if (Fobx != NULL)
    {
        ZeroAndInitializeNodeType(Fobx, RDBSS_NTC_FOBX, FobxSize);

        if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_FOBX_EXTENSION))
        {
            Fobx->Context = Add2Ptr(Fobx, sizeof(FOBX));
        }
    }

    /* If we have allocated a FCB, initialize it */
    if (Fcb != NULL)
    {
        ZeroAndInitializeNodeType(Fcb, RDBSS_STORAGE_NTC(FileTypeNotYetKnown), FcbSize);

        Fcb->NonPaged = NonPagedFcb;
        ZeroAndInitializeNodeType(Fcb->NonPaged, RDBSS_NTC_NONPAGED_FCB, sizeof(NON_PAGED_FCB));
#if DBG
        Fcb->CopyOfNonPaged = NonPagedFcb;
        NonPagedFcb->FcbBackPointer = Fcb;
#endif

        Fcb->InternalSrvOpen = SrvOpen;
        Fcb->InternalFobx = Fobx;

        Fcb->PrivateAlreadyPrefixedName.Length = NameSize;
        Fcb->PrivateAlreadyPrefixedName.MaximumLength = NameSize;
        Fcb->PrivateAlreadyPrefixedName.Buffer = PAPNBuffer;

        if (BooleanFlagOn(Dispatch->MRxFlags, RDBSS_MANAGE_FCB_EXTENSION))
        {
            Fcb->Context = Add2Ptr(Fcb, sizeof(FCB));
        }

        ZeroAndInitializeNodeType(&Fcb->FcbTableEntry, RDBSS_NTC_FCB_TABLE_ENTRY, sizeof(RX_FCB_TABLE_ENTRY));

        InterlockedIncrement(&RxNumberOfActiveFcbs);
        InterlockedIncrement((volatile long *)&RxDeviceObject->NumberOfActiveFcbs);

        ExInitializeFastMutex(&NonPagedFcb->AdvancedFcbHeaderMutex);
        FsRtlSetupAdvancedHeader(Fcb, &NonPagedFcb->AdvancedFcbHeaderMutex);
    }

    DPRINT("Allocated %p\n", Buffer);

    return Buffer;
}

/*
 * @implemented
 */
PVOID
RxAllocateObject(
    NODE_TYPE_CODE NodeType,
    PMINIRDR_DISPATCH MRxDispatch,
    ULONG NameLength)
{
    ULONG Tag, ObjectSize;
    PVOID Object, *Extension;
    PRX_PREFIX_ENTRY PrefixEntry;
    USHORT StructSize, ExtensionSize;

    PAGED_CODE();

    /* Select the node to allocate and always deal with the fact we may have to manage its extension */
    ExtensionSize = 0;
    switch (NodeType)
    {
        case RDBSS_NTC_SRVCALL:
            Tag = RX_SRVCALL_POOLTAG;
            StructSize = sizeof(SRV_CALL);
            if (MRxDispatch != NULL && BooleanFlagOn(MRxDispatch->MRxFlags, RDBSS_MANAGE_SRV_CALL_EXTENSION))
            {
                ExtensionSize = QuadAlign(MRxDispatch->MRxSrvCallSize);
            }
            break;

        case RDBSS_NTC_NETROOT:
            Tag = RX_NETROOT_POOLTAG;
            StructSize = sizeof(NET_ROOT);
            if (BooleanFlagOn(MRxDispatch->MRxFlags, RDBSS_MANAGE_NET_ROOT_EXTENSION))
            {
                ExtensionSize = QuadAlign(MRxDispatch->MRxNetRootSize);
            }
            break;

        case RDBSS_NTC_V_NETROOT:
            Tag = RX_V_NETROOT_POOLTAG;
            StructSize = sizeof(V_NET_ROOT);
            if (BooleanFlagOn(MRxDispatch->MRxFlags, RDBSS_MANAGE_V_NET_ROOT_EXTENSION))
            {
                ExtensionSize = QuadAlign(MRxDispatch->MRxVNetRootSize);
            }
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    /* Now, allocate the object */
    ObjectSize = ExtensionSize + StructSize + NameLength;
    Object = RxAllocatePoolWithTag(NonPagedPool, ObjectSize, Tag);
    if (Object == NULL)
    {
        return NULL;
    }
    /* Initialize it */
    ZeroAndInitializeNodeType(Object, NodeType, ObjectSize);

    /* For SRV_CALL and NETROOT, the name points to the prefix table name */
    switch (NodeType)
    {
        case RDBSS_NTC_SRVCALL:
            PrefixEntry = &((PSRV_CALL)Object)->PrefixEntry;
            Extension = &((PSRV_CALL)Object)->Context;
            ((PSRV_CALL)Object)->pSrvCallName = &PrefixEntry->Prefix;
            break;

        case RDBSS_NTC_NETROOT:
            PrefixEntry = &((PNET_ROOT)Object)->PrefixEntry;
            Extension = &((PNET_ROOT)Object)->Context;
            ((PNET_ROOT)Object)->pNetRootName = &PrefixEntry->Prefix;
            break;

        case RDBSS_NTC_V_NETROOT:
            PrefixEntry = &((PV_NET_ROOT)Object)->PrefixEntry;
            Extension = &((PV_NET_ROOT)Object)->Context;
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    /* Set the prefix table unicode string */
    RtlZeroMemory(PrefixEntry, sizeof(RX_PREFIX_ENTRY));
    PrefixEntry->NodeTypeCode = RDBSS_NTC_PREFIX_ENTRY;
    PrefixEntry->NodeByteSize = sizeof(RX_PREFIX_ENTRY);
    PrefixEntry->Prefix.Length = NameLength;
    PrefixEntry->Prefix.MaximumLength = NameLength;
    PrefixEntry->Prefix.Buffer = Add2Ptr(Object, ExtensionSize + StructSize);

    /* Return the extension if we are asked to manage it */
    if (ExtensionSize != 0)
    {
        *Extension = Add2Ptr(Object, StructSize);
    }

    return Object;
}

/*
 * @implemented
 */
VOID
RxAssert(
    PVOID Assert,
    PVOID File,
    ULONG Line,
    PVOID Message)
{
    CHAR Response[2];
    CONTEXT Context;

    /* If we're not asked to continue, just stop the system */
    if (!RxContinueFromAssert)
    {
        KeBugCheckEx(RDBSS_FILE_SYSTEM, RDBSS_BUG_CHECK_ASSERT | Line, 0, 0, 0);
    }

    /* Otherwise, capture context to offer the user to dump it */
    RtlCaptureContext(&Context);

    /* Loop until the user hits 'i' */
    while (TRUE)
    {
        /* If no file provided, use empty name */
        if (File == NULL)
        {
            File = "";
        }

        /* If no message provided, use empty one */
        if (Message == NULL)
        {
            Message = "";
        }

        /* Display the message */
        DbgPrint("\n*** Assertion failed: %s%s\n***   Source File: %s, line %ld\n\n", Message, Assert, File, Line);
        /* And ask the user */
        DbgPrompt("Break, Ignore (bi)? ", Response, sizeof(Response));
        /* If he asks for ignore, quit
         * In case of invalid input, ask again
         */
        if (Response[0] != 'B' && Response[0] != 'b')
        {
            if (Response[0] == 'I' || Response[0] == 'i')
            {
                return;
            }

            continue;
        }

        /* Break: offer the user to dump the context and break */
        DbgPrint("Execute '!cxr %lx' to dump context\n", &Context);
        DbgBreakPoint();

        /* Continue looping, so that after dump, execution can continue (with ignore) */
    }
}

/*
 * @implemented
 */
VOID
NTAPI
RxBootstrapWorkerThreadDispatcher(
   IN PVOID WorkQueue)
{
    PRX_WORK_QUEUE RxWorkQueue;

    PAGED_CODE();

    RxWorkQueue = WorkQueue;
    RxpWorkerThreadDispatcher(RxWorkQueue, NULL);
}

/*
 * @implemented
 */
VOID
RxCancelBlockingOperation(
    IN OUT PRX_CONTEXT RxContext)
{
    PFOBX Fobx;
    BOOLEAN PostRequest;

    PAGED_CODE();

    Fobx = (PFOBX)RxContext->pFobx;
    PostRequest = FALSE;

    /* Acquire the pipe mutex */
    ExAcquireFastMutex(&RxContextPerFileSerializationMutex);

    /* If that's a blocking pipe operation which is not the CCB one, then handle it */
    if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION) &&
        RxContext->RxContextSerializationQLinks.Flink != NULL &&
        RxContext != CONTAINING_RECORD(&Fobx->Specific.NamedPipe.ReadSerializationQueue, RX_CONTEXT, RxContextSerializationQLinks) &&
        RxContext != CONTAINING_RECORD(&Fobx->Specific.NamedPipe.WriteSerializationQueue, RX_CONTEXT, RxContextSerializationQLinks))
    {
        /* Clear it! */
        ClearFlag(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

        /* Drop it off the list */
        RemoveEntryList(&RxContext->RxContextSerializationQLinks);
        RxContext->RxContextSerializationQLinks.Flink = NULL;
        RxContext->RxContextSerializationQLinks.Blink = NULL;

        /* Set we've been cancelled */
        RxContext->IoStatusBlock.Status = STATUS_CANCELLED;

        /*
         * If it's async, we'll post completion, otherwise, we signal to waiters
         * it's being cancelled
         */
        if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION))
        {
            PostRequest = TRUE;
        }
        else
        {
            RxSignalSynchronousWaiter(RxContext);
        }
    }

    /* Done */
    ExReleaseFastMutex(&RxContextPerFileSerializationMutex);

    /* Post if async */
    if (PostRequest)
    {
        RxFsdPostRequest(RxContext);
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxChangeBufferingState(
    PSRV_OPEN SrvOpen,
    PVOID Context,
    BOOLEAN ComputeNewState)
{
    PFCB Fcb;
    NTSTATUS Status, MiniStatus;
    ULONG NewBufferingState, OldBufferingState;

    PAGED_CODE();

    DPRINT("RxChangeBufferingState(%p, %p, %d)\n", SrvOpen, Context, ComputeNewState);

    Fcb = (PFCB)SrvOpen->pFcb;
    ASSERT(NodeTypeIsFcb(Fcb));
    /* First of all, mark that buffering state is changing */
    SetFlag(Fcb->FcbState, FCB_STATE_BUFFERSTATE_CHANGING);

    /* Assume success */
    Status = STATUS_SUCCESS;
    _SEH2_TRY
    {
        /* If we're asked to compute a new state, ask the mini-rdr for it */
        if (ComputeNewState)
        {
            MINIRDR_CALL_THROUGH(MiniStatus, Fcb->MRxDispatch, MRxComputeNewBufferingState,
                                 ((PMRX_SRV_OPEN)SrvOpen, Context, &NewBufferingState));
            if (MiniStatus != STATUS_SUCCESS)
            {
                NewBufferingState = 0;
            }
        }
        else
        {
            /* If not, use SRV_OPEN state */
            NewBufferingState = SrvOpen->BufferingFlags;
        }

        /* If no shared access, and if we're not asked to compute a new state, use maximum flags set */
        if ((Fcb->ShareAccess.SharedRead + Fcb->ShareAccess.SharedWrite + Fcb->ShareAccess.SharedDelete) == 0 && !ComputeNewState)
        {
            SetFlag(NewBufferingState, FCB_STATE_BUFFERING_STATE_WITH_NO_SHARES);
        }

        /* If there's a lock operation to complete, clear that flag */
        if (Fcb->OutstandingLockOperationsCount != 0)
        {
            ClearFlag(NewBufferingState, FCB_STATE_LOCK_BUFFERING_ENABLED);
        }

        /* Get the old state */
        OldBufferingState = Fcb->FcbState & FCB_STATE_BUFFERING_STATE_MASK;
        DPRINT("ChangeBufferingState %x -> %x (%x)\n", OldBufferingState, NewBufferingState, SrvOpen->BufferingFlags);

        /* If we're dropping write cache, then flush the FCB */
        if (BooleanFlagOn(OldBufferingState, FCB_STATE_WRITECACHING_ENABLED) &&
            !BooleanFlagOn(NewBufferingState, FCB_STATE_WRITECACHING_ENABLED))
        {
            DPRINT("Flushing\n");

            Status = RxFlushFcbInSystemCache(Fcb, TRUE);
        }

        /* If we're dropping read cache, then purge */
        if (Fcb->UncleanCount == 0 ||
            (BooleanFlagOn(OldBufferingState, FCB_STATE_READCACHING_ENABLED) &&
             !BooleanFlagOn(NewBufferingState, FCB_STATE_READCACHING_ENABLED)) ||
            BooleanFlagOn(NewBufferingState, FCB_STATE_DELETE_ON_CLOSE))
        {
            DPRINT("Purging\n");

            if (!NT_SUCCESS(Status))
            {
                 DPRINT("Previous flush failed with status: %lx\n", Status);
            }

            CcPurgeCacheSection(&Fcb->NonPaged->SectionObjectPointers, NULL, 0, TRUE);
        }

        /* If there's already a change pending in SRV_OPEN */
        if (ComputeNewState && BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING))
        {
            /* If there's a FOBX at least */
            if (!IsListEmpty(&SrvOpen->FobxList))
            {
                PRX_CONTEXT RxContext;

                /* Create a fake context to pass to the mini-rdr */
                RxContext = RxCreateRxContext(NULL, Fcb->RxDeviceObject, RX_CONTEXT_FLAG_MUST_SUCCEED_NONBLOCKING | RX_CONTEXT_FLAG_WAIT);
                if (RxContext != NULL)
                {
                    PFOBX Fobx;

                    RxContext->pFcb = RX_GET_MRX_FCB(Fcb);

                    /* Give the first FOBX */
                    Fobx = CONTAINING_RECORD(SrvOpen->FobxList.Flink, FOBX, FobxQLinks);
                    RxContext->pFobx = (PMRX_FOBX)Fobx;
                    RxContext->pRelevantSrvOpen = Fobx->pSrvOpen;

                    /* If there was a delayed close, perform it */
                    if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_CLOSE_DELAYED))
                    {
                        DPRINT("Oplock break close for %p\n", SrvOpen);

                        RxCloseAssociatedSrvOpen(Fobx, RxContext);
                    }
                    /* Otherwise, inform the mini-rdr about completion */
                    else
                    {
                        MINIRDR_CALL_THROUGH(MiniStatus, Fcb->MRxDispatch, MRxCompleteBufferingStateChangeRequest,
                                             (RxContext, (PMRX_SRV_OPEN)SrvOpen, Context));
                        (void)MiniStatus;
                    }

                    RxDereferenceAndDeleteRxContext(RxContext);
                }
            }
        }

        /* Set the new state */
        Fcb->FcbState ^= (NewBufferingState ^ Fcb->FcbState) & FCB_STATE_BUFFERING_STATE_MASK;
    }
    _SEH2_FINALLY
    {
        /* Job done, clear the flag */
        ClearFlag(Fcb->FcbState, FCB_STATE_BUFFERSTATE_CHANGING);

        if (!BooleanFlagOn(NewBufferingState, FCB_STATE_FILETIMECACHEING_ENABLED))
        {
            ClearFlag(Fcb->FcbState, FCB_STATE_TIME_AND_SIZE_ALREADY_SET);
        }
    }
    _SEH2_END;

    return Status;
}

NTSTATUS
RxCheckVNetRootCredentials(
    PRX_CONTEXT RxContext,
    PV_NET_ROOT VNetRoot,
    PLUID LogonId,
    PUNICODE_STRING UserName,
    PUNICODE_STRING UserDomain,
    PUNICODE_STRING Password,
    ULONG Flags)
{
    PAGED_CODE();

    /* If that's a UNC name, there's nothing to process */
    if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_CREATE_FLAG_UNC_NAME) &&
        (BooleanFlagOn(VNetRoot->Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE) ||
         Flags != 0))
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Compare the logon ID in the VNetRoot with the one provided */
    if (RtlCompareMemory(&VNetRoot->LogonId, LogonId, sizeof(LUID)) != sizeof(LUID))
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* No credential provided? That's OK */
    if (UserName == NULL && UserDomain == NULL && Password == NULL)
    {
        return STATUS_SUCCESS;
    }

    /* Left to do! */
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
RxCompleteRequest(
      PRX_CONTEXT Context,
      NTSTATUS Status)
{
    PIRP Irp;

    PAGED_CODE();

    DPRINT("RxCompleteRequest(%p, %lx)\n", Context, Status);

    ASSERT(Context != NULL);
    ASSERT(Context->CurrentIrp != NULL);
    Irp = Context->CurrentIrp;

    /* Debug what the caller asks for */
    if (Context->LoudCompletionString != NULL)
    {
        DPRINT("LoudCompletion: %lx/%lx with %wZ\n", Status, Irp->IoStatus.Information, Context->LoudCompletionString);
        /* Does the user asks to stop on failed completion */
        if (!NT_SUCCESS(Status) && RxStopOnLoudCompletion)
        {
            DPRINT1("LoudFailure: %lx/%lx with %wZ\n", Status, Irp->IoStatus.Information, Context->LoudCompletionString);
        }
    }

    /* Complete for real */
    Context->CurrentIrp = NULL;
    RxCompleteRequest_Real(Context, Irp, Status);

    DPRINT("Status: %lx\n", Status);
    return Status;
}

/*
 * @implemented
 */
VOID
RxCompleteRequest_Real(
    IN PRX_CONTEXT RxContext,
    IN PIRP Irp,
    IN NTSTATUS Status)
{
    CCHAR Boost;
    KIRQL OldIrql;
    PIO_STACK_LOCATION Stack;

    DPRINT("RxCompleteRequest_Real(%p, %p, %lx)\n", RxContext, Irp, Status);

    /* Nothing to complete, just free context */
    if (Irp == NULL)
    {
        DPRINT("NULL IRP for %p\n", RxContext);
        if (RxContext != NULL)
        {
            RxDereferenceAndDeleteRxContext_Real(RxContext);
        }

        return;
    }

    /* Remove cancel routine */
    IoAcquireCancelSpinLock(&OldIrql);
    IoSetCancelRoutine(Irp, NULL);
    IoReleaseCancelSpinLock(OldIrql);

    /* Select the boost, given the success/paging operation */
    if (NT_SUCCESS(Status) || !BooleanFlagOn(Irp->Flags, IRP_SYNCHRONOUS_PAGING_IO))
    {
        Boost = IO_DISK_INCREMENT;
    }
    else
    {
        Irp->IoStatus.Information = 0;
        Boost = IO_NO_INCREMENT;
    }
    Irp->IoStatus.Status = Status;

    if (RxContext != NULL)
    {
        ASSERT(RxContext->MajorFunction <= IRP_MJ_MAXIMUM_FUNCTION);
        if (RxContext->MajorFunction != IRP_MJ_DEVICE_CONTROL)
        {
            DPRINT("Completing: MN: %d, Context: %p, IRP: %p, Status: %lx, Info: %lx, #%lx\n",
                   RxContext->MinorFunction, RxContext, Irp,
                   Status, Irp->IoStatus.Information, RxContext->SerialNumber);
        }
    }

    /* If that's an opening, there might be a canonical name allocated,
     * if completion isn't pending, release it
     */
    Stack = IoGetCurrentIrpStackLocation(Irp);
    if (Stack->MajorFunction == IRP_MJ_CREATE && Status != STATUS_PENDING &&
        RxContext != NULL)
    {
        if (BooleanFlagOn(RxContext->Create.Flags, 2))
        {
            Stack->FileObject->FileName.Length += sizeof(WCHAR);
        }

        RxpPrepareCreateContextForReuse(RxContext);
        ASSERT(RxContext->Create.CanonicalNameBuffer == NULL);
    }

    /* If it's a write, validate the correct behavior of the operation */
    if (Stack->MajorFunction == IRP_MJ_WRITE)
    {
        if (NT_SUCCESS(Irp->IoStatus.Status))
        {
            ASSERT(Irp->IoStatus.Information <= Stack->Parameters.Write.Length);
        }
    }

    /* If it's pending, make sure IRP is marked as such */
    if (RxContext != NULL)
    {
        if (RxContext->PendingReturned)
        {
            ASSERT(BooleanFlagOn(Stack->Control, SL_PENDING_RETURNED));
        }
    }

    /* Complete now */
    DPRINT("Completing IRP with %x/%x\n", Irp->IoStatus.Status, Irp->IoStatus.Information);
    IoCompleteRequest(Irp, Boost);

    /* If there's a context, dereference it */
    if (RxContext != NULL)
    {
        RxDereferenceAndDeleteRxContext_Real(RxContext);
    }
}

/*
 * @implemented
 */
VOID
RxCompleteSrvOpenKeyAssociation(
    IN OUT PSRV_OPEN SrvOpen)
{
    PSRV_CALL SrvCall;

    SrvCall = (PSRV_CALL)((PFCB)SrvOpen->pFcb)->VNetRoot->pNetRoot->pSrvCall;
    /* Only handle requests if opening was a success */
    if (SrvOpen->Condition == Condition_Good)
    {
        KIRQL OldIrql;
        BOOLEAN ProcessChange;
        LIST_ENTRY DiscardedRequests;

        /* Initialize our discarded requests list */
        InitializeListHead(&DiscardedRequests);

        RxAcquireBufferingManagerMutex(&SrvCall->BufferingManager);

        /* Transfer our requests in the SRV_CALL */
        RxTransferList(&SrvCall->BufferingManager.SrvOpenLists[0], &SrvOpen->SrvOpenKeyList);

        /* Was increased in RxInitiateSrvOpenKeyAssociation(), opening is done */
        InterlockedDecrement(&SrvCall->BufferingManager.NumberOfOutstandingOpens);

        /* Dispatch requests and get the discarded ones */
        RxpDispatchChangeBufferingStateRequests(SrvCall, SrvOpen, &DiscardedRequests);

        RxReleaseBufferingManagerMutex(&SrvCall->BufferingManager);

        /* Is there still anything to process? */
        KeAcquireSpinLock(&SrvCall->BufferingManager.SpinLock, &OldIrql);
        if (IsListEmpty(&SrvCall->BufferingManager.HandlerList))
        {
            ProcessChange = FALSE;
        }
        else
        {
            ProcessChange = (SrvCall->BufferingManager.HandlerInactive == FALSE);
            if (ProcessChange)
            {
                SrvCall->BufferingManager.HandlerInactive = TRUE;
            }
        }
        KeReleaseSpinLock(&SrvCall->BufferingManager.SpinLock, OldIrql);

        /* Yes? Go ahead! */
        if (ProcessChange)
        {
            RxReferenceSrvCall(SrvCall);
            RxPostToWorkerThread(RxFileSystemDeviceObject, HyperCriticalWorkQueue,
                                 &SrvCall->BufferingManager.HandlerWorkItem,
                                 RxProcessChangeBufferingStateRequests, SrvCall);
        }

        /* And discard left requests */
        RxpDiscardChangeBufferingStateRequests(&DiscardedRequests);
    }
    else
    {
        InterlockedDecrement(&SrvCall->BufferingManager.NumberOfOutstandingOpens);
    }
}

/*
 * @implemented
 */
NTSTATUS
RxConstructNetRoot(
    IN PRX_CONTEXT RxContext,
    IN PSRV_CALL SrvCall,
    IN PNET_ROOT NetRoot,
    IN PV_NET_ROOT VirtualNetRoot,
    OUT PLOCK_HOLDING_STATE LockHoldingState)
{
    NTSTATUS Status;
    PRX_PREFIX_TABLE PrefixTable;
    PMRX_CREATENETROOT_CONTEXT Context;
    RX_BLOCK_CONDITION RootCondition, VRootCondition;

    PAGED_CODE();

    DPRINT("RxConstructNetRoot(%p, %p, %p, %p, %p)\n", RxContext, SrvCall, NetRoot,
           VirtualNetRoot, LockHoldingState);

    /* Validate the lock is exclusively held */
    PrefixTable = RxContext->RxDeviceObject->pRxNetNameTable;
    ASSERT(*LockHoldingState == LHS_ExclusiveLockHeld);

    /* Allocate the context */
    Context = RxAllocatePoolWithTag(PagedPool, sizeof(MRX_CREATENETROOT_CONTEXT), RX_SRVCALL_POOLTAG);
    if (Context == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* We can release lock now */
    RxReleasePrefixTableLock(PrefixTable);
    *LockHoldingState = LHS_LockNotHeld;

    RootCondition = Condition_Bad;
    VRootCondition = Condition_Bad;

    /* Initialize the context */
    RtlZeroMemory(Context, sizeof(MRX_CREATENETROOT_CONTEXT));
    KeInitializeEvent(&Context->FinishEvent, SynchronizationEvent, FALSE);
    Context->RxContext = RxContext;
    Context->pVNetRoot = VirtualNetRoot;
    Context->Callback = RxCreateNetRootCallBack;

    /* And call the mini-rdr */
    MINIRDR_CALL_THROUGH(Status, SrvCall->RxDeviceObject->Dispatch, MRxCreateVNetRoot, (Context));
    if (Status == STATUS_PENDING)
    {
        /* Wait for the mini-rdr to be done */
        KeWaitForSingleObject(&Context->FinishEvent, Executive, KernelMode, FALSE, NULL);
        /* Update the structures condition according to mini-rdr return */
        if (NT_SUCCESS(Context->NetRootStatus))
        {
            if (NT_SUCCESS(Context->VirtualNetRootStatus))
            {
                RootCondition = Condition_Good;
                VRootCondition = Condition_Good;
                Status = STATUS_SUCCESS;
            }
            else
            {
                RootCondition = Condition_Good;
                Status = Context->VirtualNetRootStatus;
            }
        }
        else
        {
            Status = Context->VirtualNetRootStatus;
            if (NT_SUCCESS(Status))
            {
                Status = Context->NetRootStatus;
            }
        }
    }
    else
    {
        /* It has to return STATUS_PENDING! */
        ASSERT(FALSE);
    }

    /* Acquire lock again - for caller lock status will remain unchanged */
    ASSERT(*LockHoldingState == LHS_LockNotHeld);
    RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
    *LockHoldingState = LHS_ExclusiveLockHeld;

    /* Do the transition to the condition got from mini-rdr */
    RxTransitionNetRoot(NetRoot, RootCondition);
    RxTransitionVNetRoot(VirtualNetRoot, VRootCondition);

    /* Context is not longer needed */
    RxFreePoolWithTag(Context, RX_SRVCALL_POOLTAG);

    DPRINT("Status: %x\n", Status);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxConstructSrvCall(
    IN PRX_CONTEXT RxContext,
    IN PSRV_CALL SrvCall,
    OUT PLOCK_HOLDING_STATE LockHoldingState)
{
    NTSTATUS Status;
    PRX_PREFIX_TABLE PrefixTable;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PMRX_SRVCALLDOWN_STRUCTURE Calldown;
    PMRX_SRVCALL_CALLBACK_CONTEXT CallbackContext;

    PAGED_CODE();

    DPRINT("RxConstructSrvCall(%p, %p, %p)\n", RxContext, SrvCall, LockHoldingState);

    /* Validate the lock is exclusively held */
    RxDeviceObject = RxContext->RxDeviceObject;
    PrefixTable = RxDeviceObject->pRxNetNameTable;
    ASSERT(*LockHoldingState == LHS_ExclusiveLockHeld);

    /* Allocate the context for mini-rdr */
    Calldown = RxAllocatePoolWithTag(NonPagedPool, sizeof(MRX_SRVCALLDOWN_STRUCTURE), RX_SRVCALL_POOLTAG);
    if (Calldown == NULL)
    {
        SrvCall->Context = NULL;
        SrvCall->Condition = Condition_Bad;
        RxReleasePrefixTableLock(PrefixTable);
        *LockHoldingState = LHS_LockNotHeld;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Initialize it */
    RtlZeroMemory(Calldown, sizeof(MRX_SRVCALLDOWN_STRUCTURE));

    SrvCall->Context = NULL;
    SrvCall->Condition = Condition_InTransition;

    RxReleasePrefixTableLock(PrefixTable);
    *LockHoldingState = LHS_LockNotHeld;

    CallbackContext = &Calldown->CallbackContexts[0];
    DPRINT("CalldownContext %p for %wZ\n", CallbackContext, &RxDeviceObject->DeviceName);
    DPRINT("With calldown %p and SrvCall %p\n", Calldown, SrvCall);
    CallbackContext->SrvCalldownStructure = Calldown;
    CallbackContext->CallbackContextOrdinal = 0;
    CallbackContext->RxDeviceObject = RxDeviceObject;

    RxReferenceSrvCall(SrvCall);

    /* If we're async, we'll post, otherwise, we'll have to wait for completion */
    if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION))
    {
        RxPrePostIrp(RxContext, RxContext->CurrentIrp);
    }
    else
    {
        KeInitializeEvent(&Calldown->FinishEvent, SynchronizationEvent, FALSE);
    }

    Calldown->NumberToWait = 1;
    Calldown->NumberRemaining = 1;
    Calldown->RxContext = RxContext;
    Calldown->SrvCall = (PMRX_SRV_CALL)SrvCall;
    Calldown->CallBack = RxCreateSrvCallCallBack;
    Calldown->BestFinisher = NULL;
    CallbackContext->Status = STATUS_BAD_NETWORK_PATH;
    InitializeListHead(&Calldown->SrvCalldownList);

    /* Call the mini-rdr */
    ASSERT(RxDeviceObject->Dispatch != NULL);
    ASSERT(NodeType(RxDeviceObject->Dispatch) == RDBSS_NTC_MINIRDR_DISPATCH);
    ASSERT(RxDeviceObject->Dispatch->MRxCreateSrvCall != NULL);
    Status = RxDeviceObject->Dispatch->MRxCreateSrvCall((PMRX_SRV_CALL)SrvCall, CallbackContext);
    /* It has to return STATUS_PENDING! */
    ASSERT(Status == STATUS_PENDING);

    /* No async, start completion */
    if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION))
    {
        KeWaitForSingleObject(&Calldown->FinishEvent, Executive, KernelMode, FALSE, NULL);

        /* Finish construction - we'll notify mini-rdr it's the winner */
        Status = RxFinishSrvCallConstruction(Calldown);
        if (!NT_SUCCESS(Status))
        {
            RxReleasePrefixTableLock(PrefixTable);
            *LockHoldingState = LHS_LockNotHeld;
        }
        else
        {
            ASSERT(RxIsPrefixTableLockAcquired(PrefixTable));
            *LockHoldingState = LHS_ExclusiveLockHeld;
        }
    }

    DPRINT("RxConstructSrvCall() = Status: %x\n", Status);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxConstructVirtualNetRoot(
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING CanonicalName,
    IN NET_ROOT_TYPE NetRootType,
    OUT PV_NET_ROOT *VirtualNetRootPointer,
    OUT PLOCK_HOLDING_STATE LockHoldingState,
    OUT PRX_CONNECTION_ID RxConnectionId)
{
    NTSTATUS Status;
    PV_NET_ROOT VNetRoot;
    RX_BLOCK_CONDITION Condition;
    UNICODE_STRING LocalNetRootName, FilePathName;

    PAGED_CODE();

    ASSERT(*LockHoldingState != LHS_LockNotHeld);

    VNetRoot = NULL;
    Condition = Condition_Bad;
    /* Before creating the VNetRoot, try to find the appropriate connection */
    Status = RxFindOrCreateConnections(RxContext, CanonicalName, NetRootType,
                                       &LocalNetRootName, &FilePathName,
                                       LockHoldingState, RxConnectionId);
    /* Found and active */
    if (Status == STATUS_CONNECTION_ACTIVE)
    {
        /* We need a new VNetRoot */
        VNetRoot = RxCreateVNetRoot(RxContext, (PNET_ROOT)RxContext->Create.pVNetRoot->pNetRoot,
                                    CanonicalName, &LocalNetRootName, &FilePathName, RxConnectionId);
        if (VNetRoot != NULL)
        {
            RxReferenceVNetRoot(VNetRoot);
        }

        /* Dereference previous VNetRoot */
        RxDereferenceVNetRoot(RxContext->Create.pVNetRoot->pNetRoot, *LockHoldingState);
        /* Reset and start construct (new structures will replace old ones) */
        RxContext->Create.pSrvCall = NULL;
        RxContext->Create.pNetRoot = NULL;
        RxContext->Create.pVNetRoot = NULL;

        /* Construct new NetRoot */
        if (VNetRoot != NULL)
        {
            Status = RxConstructNetRoot(RxContext, (PSRV_CALL)VNetRoot->pNetRoot->pSrvCall,
                                        (PNET_ROOT)VNetRoot->pNetRoot, VNetRoot, LockHoldingState);
            if (NT_SUCCESS(Status))
            {
                Condition = Condition_Good;
            }
        }
        else
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        /* If it failed creating the connection, leave */
        if (Status != STATUS_SUCCESS)
        {
            if (*LockHoldingState != LHS_LockNotHeld)
            {
                RxReleasePrefixTableLock(RxContext->RxDeviceObject->pRxNetNameTable);
                *LockHoldingState = LHS_LockNotHeld;
            }

            *VirtualNetRootPointer = VNetRoot;
            DPRINT("RxConstructVirtualNetRoot() = Status: %x\n", Status);
            return Status;
        }

        *LockHoldingState = LHS_ExclusiveLockHeld;

        VNetRoot = (PV_NET_ROOT)RxContext->Create.pVNetRoot;
        Condition = Condition_Good;
    }

    /* We have a non stable VNetRoot - transition it */
    if (VNetRoot != NULL && !StableCondition(VNetRoot->Condition))
    {
        RxTransitionVNetRoot(VNetRoot, Condition);
    }

    /* If recreation failed */
    if (Status != STATUS_SUCCESS)
    {
        /* Dereference potential VNetRoot */
        if (VNetRoot != NULL)
        {
            ASSERT(*LockHoldingState != LHS_LockNotHeld);
            RxDereferenceVNetRoot(VNetRoot, *LockHoldingState);
            VNetRoot = NULL;
        }

        /* Release lock */
        if (*LockHoldingState != LHS_LockNotHeld)
        {
            RxReleasePrefixTableLock(RxContext->RxDeviceObject->pRxNetNameTable);
            *LockHoldingState = LHS_LockNotHeld;
        }

        /* Set NULL ptr */
        *VirtualNetRootPointer = VNetRoot;
        return Status;
    }

    /* Return the allocated VNetRoot */
    *VirtualNetRootPointer = VNetRoot;
    return Status;
}

/*
 * @implemented
 */
PFCB
RxCreateNetFcb(
    IN PRX_CONTEXT RxContext,
    IN PV_NET_ROOT VNetRoot,
    IN PUNICODE_STRING Name)
{
    PFCB Fcb;
    BOOLEAN FakeFcb;
    PNET_ROOT NetRoot;
    POOL_TYPE PoolType;
    NODE_TYPE_CODE NodeType;
    PIO_STACK_LOCATION Stack;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;

    PAGED_CODE();

    /* We need a decent VNetRoot */
    ASSERT(VNetRoot != NULL && NodeType(VNetRoot) == RDBSS_NTC_V_NETROOT);

    NetRoot = (PNET_ROOT)VNetRoot->pNetRoot;
    ASSERT(NodeType(NetRoot) == RDBSS_NTC_NETROOT);
    ASSERT((PMRX_NET_ROOT)NetRoot == RxContext->Create.pNetRoot);

    RxDeviceObject = NetRoot->pSrvCall->RxDeviceObject;
    ASSERT(RxDeviceObject == RxContext->RxDeviceObject);

    Stack = RxContext->CurrentIrpSp;

    /* Do we need to create a fake FCB? Like for renaming */
    FakeFcb = BooleanFlagOn(Stack->Flags, SL_OPEN_TARGET_DIRECTORY) &&
              !BooleanFlagOn(NetRoot->Flags, NETROOT_FLAG_SUPPORTS_SYMBOLIC_LINKS);
    ASSERT(FakeFcb || RxIsFcbTableLockExclusive(&NetRoot->FcbTable));

    PoolType = (BooleanFlagOn(Stack->Flags, SL_OPEN_PAGING_FILE) ? NonPagedPool : PagedPool);
    NodeType = (FakeFcb) ? RDBSS_NTC_OPENTARGETDIR_FCB : RDBSS_STORAGE_NTC(FileTypeNotYetKnown);

    /* Allocate the FCB */
    Fcb = RxAllocateFcbObject(RxDeviceObject, NodeType, PoolType,
                              NetRoot->InnerNamePrefix.Length + Name->Length, NULL);
    if (Fcb == NULL)
    {
        return NULL;
    }

    /* Initialize the FCB */
    Fcb->CachedNetRootType = NetRoot->Type;
    Fcb->RxDeviceObject = RxDeviceObject;
    Fcb->MRxDispatch = RxDeviceObject->Dispatch;
    Fcb->VNetRoot = VNetRoot;
    Fcb->pNetRoot = VNetRoot->pNetRoot;

    InitializeListHead(&Fcb->SrvOpenList);
    Fcb->SrvOpenListVersion = 0;

    Fcb->FcbTableEntry.Path.Length = Name->Length;
    Fcb->FcbTableEntry.Path.MaximumLength = Name->Length;
    Fcb->FcbTableEntry.Path.Buffer = Add2Ptr(Fcb->PrivateAlreadyPrefixedName.Buffer, NetRoot->InnerNamePrefix.Length);
    RtlMoveMemory(Fcb->PrivateAlreadyPrefixedName.Buffer, NetRoot->InnerNamePrefix.Buffer,
                  NetRoot->InnerNamePrefix.Length);
    RtlMoveMemory(Fcb->FcbTableEntry.Path.Buffer, Name->Buffer, Name->Length);

    /* Copy back parameters from RxContext */
    if (BooleanFlagOn(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_ADDEDBACKSLASH))
    {
        Fcb->FcbState |= FCB_STATE_ADDEDBACKSLASH;
    }

    InitializeListHead(&Fcb->NonPaged->TransitionWaitList);

    if (BooleanFlagOn(Stack->Flags, SL_OPEN_PAGING_FILE))
    {
        Fcb->FcbState |= FCB_STATE_PAGING_FILE;
    }

    if (RxContext->MajorFunction == IRP_MJ_CREATE && BooleanFlagOn(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_SPECIAL_PATH))
    {
        Fcb->FcbState |= FCB_STATE_SPECIAL_PATH;
    }

    Fcb->Header.Resource = &Fcb->NonPaged->HeaderResource;
    ExInitializeResourceLite(Fcb->Header.Resource);

    Fcb->Header.PagingIoResource = &Fcb->NonPaged->PagingIoResource;
    ExInitializeResourceLite(Fcb->Header.PagingIoResource);

    Fcb->BufferedLocks.Resource = &Fcb->NonPaged->BufferedLocksResource;
    ExInitializeResourceLite(Fcb->BufferedLocks.Resource);

    /* Fake FCB doesn't go in prefix table */
    if (FakeFcb)
    {
        Fcb->FcbState |= (FCB_STATE_FAKEFCB | FCB_STATE_NAME_ALREADY_REMOVED);
        InitializeListHead(&Fcb->FcbTableEntry.HashLinks);
        DPRINT("Fake FCB: %p\n", Fcb);
    }
    else
    {
        RxFcbTableInsertFcb(&NetRoot->FcbTable, Fcb);
    }

    RxReferenceVNetRoot(VNetRoot);
    InterlockedIncrement((volatile long *)&Fcb->pNetRoot->NumberOfFcbs);

    Fcb->ulFileSizeVersion = 0;

    DPRINT("FCB %p for %wZ\n", Fcb, &Fcb->FcbTableEntry.Path);
    RxReferenceNetFcb(Fcb);

    return Fcb;
}

/*
 * @implemented
 */
PMRX_FOBX
NTAPI
RxCreateNetFobx(
    OUT PRX_CONTEXT RxContext,
    IN PMRX_SRV_OPEN MrxSrvOpen)
{
    PFCB Fcb;
    PFOBX Fobx;
    ULONG Flags;
    PNET_ROOT NetRoot;
    PSRV_OPEN SrvOpen;
    POOL_TYPE PoolType;

    PAGED_CODE();

    SrvOpen = (PSRV_OPEN)MrxSrvOpen;
    ASSERT(NodeType(SrvOpen) == RDBSS_NTC_SRVOPEN);
    ASSERT(NodeTypeIsFcb(SrvOpen->Fcb));
    ASSERT(RxIsFcbAcquiredExclusive(SrvOpen->Fcb));

    Fcb = SrvOpen->Fcb;
    PoolType = (BooleanFlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) ? NonPagedPool : PagedPool);
    /* Can we use pre-allocated FOBX? */
    if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_FOBX_USED) && Fcb->InternalSrvOpen == (PSRV_OPEN)MrxSrvOpen)
    {
        Fobx = Fcb->InternalFobx;
        /* Call allocate to initialize the FOBX */
        RxAllocateFcbObject(Fcb->RxDeviceObject, RDBSS_NTC_FOBX, PoolType, 0, Fobx);
        /* Mark it used now */
        Fcb->FcbState |= FCB_STATE_FOBX_USED;
        Flags = FOBX_FLAG_ENCLOSED_ALLOCATED;
    }
    else if (!BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_FOBX_USED))
    {
        Fobx = SrvOpen->InternalFobx;
        /* Call allocate to initialize the FOBX */
        RxAllocateFcbObject(Fcb->RxDeviceObject, RDBSS_NTC_FOBX, PoolType, 0, Fobx);
        /* Mark it used now */
        SrvOpen->Flags |= SRVOPEN_FLAG_FOBX_USED;
        Flags = FOBX_FLAG_ENCLOSED_ALLOCATED;
    }
    else
    {
        /* Last case, we cannot, allocate a FOBX */
        Fobx = RxAllocateFcbObject(Fcb->RxDeviceObject, RDBSS_NTC_FOBX, PoolType, 0, NULL);
        Flags = 0;
    }

    /* Allocation failed! */
    if (Fobx == NULL)
    {
        return NULL;
    }

    /* Set flags */
    Fobx->Flags = Flags;

    /* Initialize throttling */
    NetRoot = (PNET_ROOT)RxContext->Create.pNetRoot;
    if (NetRoot != NULL)
    {
        if (NetRoot->DeviceType == FILE_DEVICE_DISK)
        {
            RxInitializeThrottlingState(&Fobx->Specific.DiskFile.LockThrottlingState,
                                        NetRoot->DiskParameters.LockThrottlingParameters.Increment,
                                        NetRoot->DiskParameters.LockThrottlingParameters.MaximumDelay);
        }
        else if (NetRoot->DeviceType == FILE_DEVICE_NAMED_PIPE)
        {
            RxInitializeThrottlingState(&Fobx->Specific.NamedPipe.ThrottlingState,
                                        NetRoot->NamedPipeParameters.PipeReadThrottlingParameters.Increment,
                                        NetRoot->NamedPipeParameters.PipeReadThrottlingParameters.MaximumDelay);
        }
    }

    /* Propagate flags fron RxContext */
    if (BooleanFlagOn(RxContext->Create.Flags, RX_CONTEXT_CREATE_FLAG_UNC_NAME))
    {
        Fobx->Flags |= FOBX_FLAG_UNC_NAME;
    }

    if (BooleanFlagOn(RxContext->Create.NtCreateParameters.CreateOptions, FILE_OPEN_FOR_BACKUP_INTENT))
    {
        Fobx->Flags |= FOBX_FLAG_BACKUP_INTENT;
    }

    /* Continue init */
    Fobx->FobxSerialNumber = 0;
    Fobx->SrvOpen = (PSRV_OPEN)MrxSrvOpen;
    Fobx->NodeReferenceCount = 1;
    Fobx->RxDeviceObject = Fcb->RxDeviceObject;

    RxReferenceSrvOpen(SrvOpen);
    InterlockedIncrement((volatile long *)&SrvOpen->pVNetRoot->NumberOfFobxs);

    InsertTailList(&SrvOpen->FobxList, &Fobx->FobxQLinks);
    InitializeListHead(&Fobx->ScavengerFinalizationList);
    InitializeListHead(&Fobx->ClosePendingList);

    Fobx->CloseTime.QuadPart = 0;
    Fobx->fOpenCountDecremented = FALSE;

    DPRINT("FOBX %p for SRV_OPEN %p FCB %p\n", Fobx, Fobx->SrvOpen, Fobx->SrvOpen->pFcb);

    return (PMRX_FOBX)Fobx;
}

/*
 * @implemented
 */
PNET_ROOT
RxCreateNetRoot(
    IN PSRV_CALL SrvCall,
    IN PUNICODE_STRING Name,
    IN ULONG NetRootFlags,
    IN PRX_CONNECTION_ID OPTIONAL RxConnectionId)
{
    PNET_ROOT NetRoot;
    USHORT CaseInsensitiveLength;
    PRX_PREFIX_TABLE PrefixTable;

    DPRINT("RxCreateNetRoot(%p, %wZ, %x, %p)\n", SrvCall, Name, NetRootFlags, RxConnectionId);

    PAGED_CODE();

    /* We need a SRV_CALL */
    ASSERT(SrvCall != NULL);

    PrefixTable = SrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT(RxIsPrefixTableLockExclusive(PrefixTable));

    /* Get name length */
    CaseInsensitiveLength = SrvCall->PrefixEntry.Prefix.Length + Name->Length;
    if (CaseInsensitiveLength > MAXUSHORT)
    {
        return NULL;
    }

    /* Allocate the NetRoot */
    NetRoot = RxAllocateObject(RDBSS_NTC_NETROOT, SrvCall->RxDeviceObject->Dispatch,
                               CaseInsensitiveLength);
    if (NetRoot == NULL)
    {
        return NULL;
    }

    /* Construct name */
    RtlMoveMemory(Add2Ptr(NetRoot->PrefixEntry.Prefix.Buffer, SrvCall->PrefixEntry.Prefix.Length),
                  Name->Buffer, Name->Length);
    if (SrvCall->PrefixEntry.Prefix.Length != 0)
    {
        RtlMoveMemory(NetRoot->PrefixEntry.Prefix.Buffer, SrvCall->PrefixEntry.Prefix.Buffer,
                      SrvCall->PrefixEntry.Prefix.Length);
    }

    if (!BooleanFlagOn(SrvCall->Flags, SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS))
    {
        CaseInsensitiveLength = SrvCall->PrefixEntry.CaseInsensitiveLength;
    }
    /* Inisert in prefix table */
    RxPrefixTableInsertName(PrefixTable, &NetRoot->PrefixEntry, NetRoot,
                            (PULONG)&NetRoot->NodeReferenceCount, CaseInsensitiveLength,
                            RxConnectionId);

    /* Prepare the FCB table */
    RxInitializeFcbTable(&NetRoot->FcbTable, TRUE);

    InitializeListHead(&NetRoot->TransitionWaitList);
    InitializeListHead(&NetRoot->ScavengerFinalizationList);
    InitializeListHead(&NetRoot->VirtualNetRoots);

    RxInitializePurgeSyncronizationContext(&NetRoot->PurgeSyncronizationContext);

    NetRoot->SerialNumberForEnum = SerialNumber++;
    NetRoot->Flags |= NetRootFlags;
    NetRoot->DiskParameters.ClusterSize = 1;
    NetRoot->DiskParameters.ReadAheadGranularity = ReadAheadGranularity;
    NetRoot->SrvCall = SrvCall;

    RxReferenceSrvCall(SrvCall);

    DPRINT("NetRootName: %wZ (%p)\n", NetRoot->pNetRootName, NetRoot);
    return NetRoot;
}

/*
 * @implemented
 */
VOID
NTAPI
RxCreateNetRootCallBack(
    IN PMRX_CREATENETROOT_CONTEXT CreateNetRootContext)
{
    PAGED_CODE();

    KeSetEvent(&CreateNetRootContext->FinishEvent, IO_NETWORK_INCREMENT, FALSE);
}

/*
 * @implemented
 */
PRX_CONTEXT
NTAPI
RxCreateRxContext(
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG InitialContextFlags)
{
    KIRQL OldIrql;
    PRX_CONTEXT Context;

    ASSERT(RxDeviceObject != NULL);

    DPRINT("RxCreateRxContext(%p, %p, %u)\n", Irp, RxDeviceObject, InitialContextFlags);

#if DBG
    InterlockedIncrement((volatile LONG *)&RxFsdEntryCount);
#endif
    InterlockedIncrement((volatile LONG *)&RxDeviceObject->NumberOfActiveContexts);

    /* Allocate the context from our lookaside list */
    Context = ExAllocateFromNPagedLookasideList(&RxContextLookasideList);
    if (Context == NULL)
    {
        return NULL;
    }

    /* Zero it */
    RtlZeroMemory(Context, sizeof(RX_CONTEXT));

    /* It was allocated on NP pool, keep track of it! */
    SetFlag(Context->Flags, RX_CONTEXT_FLAG_FROM_POOL);
    /* And initialize it */
    RxInitializeContext(Irp, RxDeviceObject, InitialContextFlags, Context);
    ASSERT((Context->MajorFunction != IRP_MJ_CREATE) || !BooleanFlagOn(Context->Flags, RX_CONTEXT_FLAG_MUST_SUCCEED_ALLOCATED));

    /* Add it to our global list */
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);
    InsertTailList(&RxActiveContexts, &Context->ContextListEntry);
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

    DPRINT("Context: %p\n", Context);
    return Context;
}

/*
 * @implemented
 */
PSRV_CALL
RxCreateSrvCall(
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING Name,
    IN PUNICODE_STRING InnerNamePrefix OPTIONAL,
    IN PRX_CONNECTION_ID RxConnectionId)
{
    ULONG NameLength;
    PSRV_CALL SrvCall;

    PAGED_CODE();

    DPRINT("RxCreateSrvCall(%p, %wZ, %wZ, %p)\n", RxContext, Name, InnerNamePrefix, RxConnectionId);

    ASSERT(RxIsPrefixTableLockExclusive(RxContext->RxDeviceObject->pRxNetNameTable));

    /* Get the name length */
    NameLength = Name->Length + 2 * sizeof(WCHAR);
    if (InnerNamePrefix != NULL)
    {
        NameLength += InnerNamePrefix->Length;
    }

    /* Allocate the object */
    SrvCall = RxAllocateObject(RDBSS_NTC_SRVCALL, NULL, NameLength);
    if (SrvCall == NULL)
    {
        return NULL;
    }

    /* Initialize it */
    SrvCall->SerialNumberForEnum = SerialNumber++;
    SrvCall->RxDeviceObject = RxContext->RxDeviceObject;
    RxInitializeBufferingManager(SrvCall);
    InitializeListHead(&SrvCall->TransitionWaitList);
    InitializeListHead(&SrvCall->ScavengerFinalizationList);
    RxInitializePurgeSyncronizationContext(&SrvCall->PurgeSyncronizationContext);
    RxInitializeSrvCallParameters(RxContext, SrvCall);
    RtlMoveMemory(SrvCall->PrefixEntry.Prefix.Buffer, Name->Buffer, Name->Length);
    SrvCall->PrefixEntry.Prefix.MaximumLength = Name->Length + 2 * sizeof(WCHAR);
    SrvCall->PrefixEntry.Prefix.Length = Name->Length;
    RxPrefixTableInsertName(RxContext->RxDeviceObject->pRxNetNameTable, &SrvCall->PrefixEntry,
                            SrvCall, (PULONG)&SrvCall->NodeReferenceCount, Name->Length, RxConnectionId);

    DPRINT("SrvCallName: %wZ (%p)\n", SrvCall->pSrvCallName, SrvCall);
    return SrvCall;
}

/*
 * @implemented
 */
VOID
NTAPI
RxCreateSrvCallCallBack(
    IN OUT PMRX_SRVCALL_CALLBACK_CONTEXT Context)
{
    KIRQL OldIrql;
    PSRV_CALL SrvCall;
    PRX_CONTEXT RxContext;
    ULONG NumberRemaining;
    BOOLEAN StartDispatcher;
    PMRX_SRVCALLDOWN_STRUCTURE Calldown;

    DPRINT("RxCreateSrvCallCallBack(%p)\n", Context);

    /* Get our context structures */
    Calldown = Context->SrvCalldownStructure;
    SrvCall = (PSRV_CALL)Calldown->SrvCall;

    /* If it is a success, that's the winner */
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);
    if (Context->Status == STATUS_SUCCESS)
    {
        Calldown->BestFinisherOrdinal = Context->CallbackContextOrdinal;
        Calldown->BestFinisher = Context->RxDeviceObject;
    }
    NumberRemaining = --Calldown->NumberRemaining;
    SrvCall->Status = Context->Status;
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

    /* Still some to ask, keep going */
    if (NumberRemaining != 0)
    {
        return;
    }

    /* If that's not async, signal we're done */
    RxContext = Calldown->RxContext;
    if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION))
    {
        KeSetEvent(&Calldown->FinishEvent, IO_NETWORK_INCREMENT, FALSE);
        return;
    }
    /* If that's a mailslot, finish construction, no more to do */
    else if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_CREATE_MAILSLOT))
    {
        RxFinishSrvCallConstruction(Calldown);
        return;
    }

    /* Queue our finish call for delayed completion */
    DPRINT("Queuing RxFinishSrvCallConstruction() call\n");
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);
    InsertTailList(&RxSrvCalldownList, &Calldown->SrvCalldownList);
    StartDispatcher = !RxSrvCallConstructionDispatcherActive;
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

    /* If we have to start dispatcher, go ahead */
    if (StartDispatcher)
    {
        NTSTATUS Status;

        Status = RxDispatchToWorkerThread(RxFileSystemDeviceObject, CriticalWorkQueue,
                                          RxFinishSrvCallConstructionDispatcher, &RxSrvCalldownList);
        if (!NT_SUCCESS(Status))
        {
            /* It failed - run it manually.... */
            RxFinishSrvCallConstructionDispatcher(NULL);
        }
    }
}

/*
 * @implemented
 */
PSRV_OPEN
RxCreateSrvOpen(
    IN PV_NET_ROOT VNetRoot,
    IN OUT PFCB Fcb)
{
    ULONG Flags;
    PSRV_OPEN SrvOpen;
    POOL_TYPE PoolType;

    PAGED_CODE();

    ASSERT(NodeTypeIsFcb(Fcb));
    ASSERT(RxIsFcbAcquiredExclusive(Fcb));

    PoolType = (BooleanFlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE) ? NonPagedPool : PagedPool);

    _SEH2_TRY
    {
        SrvOpen = Fcb->InternalSrvOpen;
        /* Check whethet we have to allocate a new SRV_OPEN */
        if (Fcb->InternalSrvOpen == NULL || BooleanFlagOn(Fcb->FcbState, FCB_STATE_SRVOPEN_USED) ||
            BooleanFlagOn(Fcb->InternalSrvOpen->Flags, SRVOPEN_FLAG_ENCLOSED_ALLOCATED) ||
            !IsListEmpty(&Fcb->InternalSrvOpen->SrvOpenQLinks))
        {
            /* Proceed */
            SrvOpen = RxAllocateFcbObject(Fcb->VNetRoot->NetRoot->pSrvCall->RxDeviceObject,
                                          RDBSS_NTC_SRVOPEN, PoolType, 0, NULL);
            Flags = 0;
        }
        else
        {
            /* Otherwise, just use internal one and initialize it */
            RxAllocateFcbObject(Fcb->VNetRoot->NetRoot->pSrvCall->RxDeviceObject,
                                RDBSS_NTC_INTERNAL_SRVOPEN, PoolType, 0,
                                Fcb->InternalSrvOpen);
            Fcb->FcbState |= FCB_STATE_SRVOPEN_USED;
            Flags = SRVOPEN_FLAG_ENCLOSED_ALLOCATED | SRVOPEN_FLAG_FOBX_USED;
        }

        /* If SrvOpen was properly allocated, initialize it */
        if (SrvOpen != NULL)
        {
            SrvOpen->Flags = Flags;
            SrvOpen->pFcb = RX_GET_MRX_FCB(Fcb);
            SrvOpen->pAlreadyPrefixedName = &Fcb->PrivateAlreadyPrefixedName;
            SrvOpen->pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
            SrvOpen->ulFileSizeVersion = Fcb->ulFileSizeVersion;
            SrvOpen->NodeReferenceCount = 1;

            RxReferenceVNetRoot(VNetRoot);
            RxReferenceNetFcb(Fcb);

            InsertTailList(&Fcb->SrvOpenList, &SrvOpen->SrvOpenQLinks);
            ++Fcb->SrvOpenListVersion;

            InitializeListHead(&SrvOpen->ScavengerFinalizationList);
            InitializeListHead(&SrvOpen->TransitionWaitList);
            InitializeListHead(&SrvOpen->FobxList);
            InitializeListHead(&SrvOpen->SrvOpenKeyList);
        }
    }
    _SEH2_FINALLY
    {
        if (_SEH2_AbnormalTermination())
        {
            if (SrvOpen != NULL)
            {
                RxFinalizeSrvOpen(SrvOpen, TRUE, TRUE);
                SrvOpen = NULL;
            }
        }
        else
        {
            DPRINT("SrvOpen %p for FCB %p\n", SrvOpen, SrvOpen->pFcb);
        }
    }
    _SEH2_END;

    return SrvOpen;
}

/*
 * @implemented
 */
PV_NET_ROOT
RxCreateVNetRoot(
    IN PRX_CONTEXT RxContext,
    IN PNET_ROOT NetRoot,
    IN PUNICODE_STRING CanonicalName,
    IN PUNICODE_STRING LocalNetRootName,
    IN PUNICODE_STRING FilePath,
    IN PRX_CONNECTION_ID RxConnectionId)
{
    NTSTATUS Status;
    PV_NET_ROOT VNetRoot;
    USHORT CaseInsensitiveLength;

    PAGED_CODE();

    DPRINT("RxCreateVNetRoot(%p, %p, %wZ, %wZ, %wZ, %p)\n", RxContext, NetRoot, CanonicalName,
           LocalNetRootName, FilePath, RxConnectionId);

    /* Lock must be held exclusively */
    ASSERT(RxIsPrefixTableLockExclusive(RxContext->RxDeviceObject->pRxNetNameTable));

    /* Check for overflow */
    if (LocalNetRootName->Length + NetRoot->PrefixEntry.Prefix.Length > MAXUSHORT)
    {
        return NULL;
    }

    /* Get name length and allocate VNetRoot */
    CaseInsensitiveLength = LocalNetRootName->Length + NetRoot->PrefixEntry.Prefix.Length;
    VNetRoot = RxAllocateObject(RDBSS_NTC_V_NETROOT, NetRoot->SrvCall->RxDeviceObject->Dispatch,
                                CaseInsensitiveLength);
    if (VNetRoot == NULL)
    {
        return NULL;
    }

    /* Initialize its connection parameters */
    Status = RxInitializeVNetRootParameters(RxContext, &VNetRoot->LogonId, &VNetRoot->SessionId,
                                            &VNetRoot->pUserName, &VNetRoot->pUserDomainName,
                                            &VNetRoot->pPassword, &VNetRoot->Flags);
    if (!NT_SUCCESS(Status))
    {
        RxUninitializeVNetRootParameters(VNetRoot->pUserName, VNetRoot->pUserDomainName,
                                         VNetRoot->pPassword, &VNetRoot->Flags);
        RxFreeObject(VNetRoot);

        return NULL;
    }

    /* Set name */
    RtlMoveMemory(VNetRoot->PrefixEntry.Prefix.Buffer, CanonicalName->Buffer, VNetRoot->PrefixEntry.Prefix.Length);

    VNetRoot->PrefixOffsetInBytes = LocalNetRootName->Length + NetRoot->PrefixEntry.Prefix.Length;
    VNetRoot->NamePrefix.Buffer = Add2Ptr(VNetRoot->PrefixEntry.Prefix.Buffer, VNetRoot->PrefixOffsetInBytes);
    VNetRoot->NamePrefix.Length = VNetRoot->PrefixEntry.Prefix.Length - VNetRoot->PrefixOffsetInBytes;
    VNetRoot->NamePrefix.MaximumLength = VNetRoot->PrefixEntry.Prefix.Length - VNetRoot->PrefixOffsetInBytes;

    InitializeListHead(&VNetRoot->TransitionWaitList);
    InitializeListHead(&VNetRoot->ScavengerFinalizationList);

    if (!BooleanFlagOn(NetRoot->SrvCall->Flags, SRVCALL_FLAG_CASE_INSENSITIVE_FILENAMES))
    {
        USHORT i;

        if (BooleanFlagOn(NetRoot->SrvCall->Flags, SRVCALL_FLAG_CASE_INSENSITIVE_NETROOTS))
        {
            CaseInsensitiveLength = NetRoot->PrefixEntry.CaseInsensitiveLength;
        }
        else
        {
            CaseInsensitiveLength = NetRoot->SrvCall->PrefixEntry.CaseInsensitiveLength;
        }

        for (i = 1; i < CanonicalName->Length / sizeof(WCHAR); ++i)
        {
            if (CanonicalName->Buffer[i] != OBJ_NAME_PATH_SEPARATOR)
            {
                break;
            }
        }

        CaseInsensitiveLength += (i * sizeof(WCHAR));
    }

    /* Insert in prefix table */
    RxPrefixTableInsertName(RxContext->RxDeviceObject->pRxNetNameTable, &VNetRoot->PrefixEntry,
                            VNetRoot, (PULONG)&VNetRoot->NodeReferenceCount, CaseInsensitiveLength,
                            RxConnectionId);

    RxReferenceNetRoot(NetRoot);
    RxAddVirtualNetRootToNetRoot(NetRoot, VNetRoot);

    /* Finish init */
    VNetRoot->SerialNumberForEnum = SerialNumber++;
    VNetRoot->UpperFinalizationDone = FALSE;
    VNetRoot->ConnectionFinalizationDone = FALSE;
    VNetRoot->AdditionalReferenceForDeleteFsctlTaken = 0;

    DPRINT("NamePrefix: %wZ\n", &VNetRoot->NamePrefix);
    DPRINT("PrefixEntry: %wZ\n", &VNetRoot->PrefixEntry.Prefix);

    return VNetRoot;
}

/*
 * @implemented
 */
VOID
RxDereference(
    IN OUT PVOID Instance,
    IN LOCK_HOLDING_STATE LockHoldingState)
{
    LONG RefCount;
    NODE_TYPE_CODE NodeType;
    PNODE_TYPE_AND_SIZE Node;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    /* Check we have a node we can handle */
    NodeType = NodeType(Instance);
    ASSERT((NodeType == RDBSS_NTC_SRVCALL) || (NodeType == RDBSS_NTC_NETROOT) ||
           (NodeType == RDBSS_NTC_V_NETROOT) || (NodeType == RDBSS_NTC_SRVOPEN) ||
           (NodeType == RDBSS_NTC_FOBX));

    Node = (PNODE_TYPE_AND_SIZE)Instance;
    RefCount = InterlockedDecrement((volatile long *)&Node->NodeReferenceCount);
    ASSERT(RefCount >= 0);

    /* Trace refcount */
    switch (NodeType)
    {
        case RDBSS_NTC_SRVCALL:
            PRINT_REF_COUNT(SRVCALL, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_NETROOT:
            PRINT_REF_COUNT(NETROOT, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_V_NETROOT:
            PRINT_REF_COUNT(VNETROOT, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_SRVOPEN:
            PRINT_REF_COUNT(SRVOPEN, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_FOBX:
            PRINT_REF_COUNT(NETFOBX, Node->NodeReferenceCount);
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    /* No need to free - still in use */
    if (RefCount > 1)
    {
        RxReleaseScavengerMutex();
        return;
    }

    /* We have to be locked exclusively */
    if (LockHoldingState != LHS_ExclusiveLockHeld)
    {
        if ((NodeType == RDBSS_NTC_FOBX && RefCount == 0) ||
             (NodeType >= RDBSS_NTC_SRVCALL && NodeType <= RDBSS_NTC_V_NETROOT))
        {
            RxpMarkInstanceForScavengedFinalization(Instance);
        }

        RxReleaseScavengerMutex();
        return;
    }
    else
    {
        if (BooleanFlagOn(NodeType, RX_SCAVENGER_MASK))
        {
            RxpUndoScavengerFinalizationMarking(Instance);
        }
    }

    RxReleaseScavengerMutex();

    /* Now, deallocate the memory */
    switch (NodeType)
    {
        case RDBSS_NTC_SRVCALL:
        {
            PSRV_CALL SrvCall;

            SrvCall = (PSRV_CALL)Instance;

            ASSERT(SrvCall->RxDeviceObject != NULL);
            ASSERT(RxIsPrefixTableLockAcquired(SrvCall->RxDeviceObject->pRxNetNameTable));
            RxFinalizeSrvCall(SrvCall, TRUE, TRUE);
            break;
        }

        case RDBSS_NTC_NETROOT:
        {
            PNET_ROOT NetRoot;

            NetRoot = (PNET_ROOT)Instance;

            ASSERT(NetRoot->pSrvCall->RxDeviceObject != NULL);
            ASSERT(RxIsPrefixTableLockAcquired(NetRoot->pSrvCall->RxDeviceObject->pRxNetNameTable));
            RxFinalizeNetRoot(NetRoot, TRUE, TRUE);
            break;
        }

        case RDBSS_NTC_V_NETROOT:
        {
            PV_NET_ROOT VNetRoot;

            VNetRoot = (PV_NET_ROOT)Instance;

            ASSERT(VNetRoot->pNetRoot->pSrvCall->RxDeviceObject != NULL);
            ASSERT(RxIsPrefixTableLockAcquired(VNetRoot->pNetRoot->pSrvCall->RxDeviceObject->pRxNetNameTable));
            RxFinalizeVNetRoot(VNetRoot, TRUE, TRUE);
            break;
        }

        case RDBSS_NTC_SRVOPEN:
        {
            PSRV_OPEN SrvOpen;

            SrvOpen = (PSRV_OPEN)Instance;

            ASSERT(RxIsFcbAcquired(SrvOpen->Fcb));
            if (SrvOpen->OpenCount == 0)
            {
                RxFinalizeSrvOpen(SrvOpen, FALSE, FALSE);
            }
            break;
        }

        case RDBSS_NTC_FOBX:
        {
            PFOBX Fobx;

            Fobx = (PFOBX)Instance;

            ASSERT(RxIsFcbAcquired(Fobx->SrvOpen->Fcb));
            RxFinalizeNetFobx(Fobx, TRUE, FALSE);
            break;
        }
    }
}

/*
 * @implemented
 */
VOID
NTAPI
RxDereferenceAndDeleteRxContext_Real(
    IN PRX_CONTEXT RxContext)
{
    KIRQL OldIrql;
    ULONG RefCount;
    BOOLEAN Allocated;
    PRX_CONTEXT StopContext = NULL;

    /* Make sure we really have a context */
    KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);
    ASSERT(RxContext->NodeTypeCode == RDBSS_NTC_RX_CONTEXT);
    RefCount = InterlockedDecrement((volatile LONG *)&RxContext->ReferenceCount);
    /* If refcount is 0, start releasing stuff that needs spinlock held */
    if (RefCount == 0)
    {
        PRDBSS_DEVICE_OBJECT RxDeviceObject;

        Allocated = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_FROM_POOL);

        /* If that's stop context from DO, remove it */
        RxDeviceObject = RxContext->RxDeviceObject;
        if (RxDeviceObject->StartStopContext.pStopContext == RxContext)
        {
            RxDeviceObject->StartStopContext.pStopContext = NULL;
        }
        else
        {
           /* Remove it from the list */
            ASSERT((RxContext->ContextListEntry.Flink->Blink == &RxContext->ContextListEntry) &&
                   (RxContext->ContextListEntry.Blink->Flink == &RxContext->ContextListEntry));
            RemoveEntryList(&RxContext->ContextListEntry);

            /* If that was the last active context, save the stop context */
            if (InterlockedExchangeAdd((volatile LONG *)&RxDeviceObject->NumberOfActiveContexts, -1) == 0)
            {
                if (RxDeviceObject->StartStopContext.pStopContext != NULL)
                {
                    StopContext = RxDeviceObject->StartStopContext.pStopContext;
                }
            }
        }
    }
    KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

    /* Now, deal with what can be done without spinlock held */
    if (RefCount == 0)
    {
        /* Refcount shouldn't have changed */
        ASSERT(RxContext->ReferenceCount == 0);
        /* Reset everything that can be */
        RxPrepareContextForReuse(RxContext);

#ifdef RDBSS_TRACKER
        ASSERT(RxContext->AcquireReleaseFcbTrackerX == 0);
#endif
        /* If that was the last active, set the event */
        if (StopContext != NULL)
        {
            StopContext->Flags &= ~RX_CONTEXT_FLAG_RECURSIVE_CALL;
            KeSetEvent(&StopContext->SyncEvent, IO_NO_INCREMENT, FALSE);
        }

#if DBG
        /* Is ShadowCrit still owned? Shouldn't happen! */
        if (RxContext->ShadowCritOwner != 0)
        {
            DPRINT1("ShadowCritOwner not null! %lx\n", RxContext->ShadowCritOwner);
            ASSERT(FALSE);
        }
#endif

        /* If it was allocated, free it */
        if (Allocated)
        {
            ExFreeToNPagedLookasideList(&RxContextLookasideList, RxContext);
        }
    }
}

VOID
NTAPI
RxDispatchChangeBufferingStateRequests(
    PVOID Context)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxDispatchToWorkerThread(
    IN  PRDBSS_DEVICE_OBJECT pMRxDeviceObject,
    IN  WORK_QUEUE_TYPE WorkQueueType,
    IN  PRX_WORKERTHREAD_ROUTINE Routine,
    IN PVOID pContext)
{
    NTSTATUS Status;
    PRX_WORK_DISPATCH_ITEM DispatchItem;

    /* Allocate a bit of context */
    DispatchItem = RxAllocatePoolWithTag(PagedPool, sizeof(RX_WORK_DISPATCH_ITEM), RX_WORKQ_POOLTAG);
    if (DispatchItem == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Set all the routines, the one our dispatcher will call, the one ntoskrnl will call */
    DispatchItem->DispatchRoutine = Routine;
    DispatchItem->DispatchRoutineParameter = pContext;
    DispatchItem->WorkQueueItem.WorkerRoutine = RxWorkItemDispatcher;
    DispatchItem->WorkQueueItem.Parameter = DispatchItem;

    /* Insert item */
    Status = RxInsertWorkQueueItem(pMRxDeviceObject, WorkQueueType, &DispatchItem->WorkQueueItem);
    if (!NT_SUCCESS(Status))
    {
        RxFreePoolWithTag(DispatchItem, RX_WORKQ_POOLTAG);
        DPRINT1("RxInsertWorkQueueItem failed! Queue: %ld, Routine: %p, Context: %p, Status: %lx\n", WorkQueueType, Routine, pContext, Status);
    }

    DPRINT("Dispatching: %p, %p\n", Routine, pContext);

    return Status;
}

/*
 * @implemented
 */
VOID
RxExclusivePrefixTableLockToShared(
    PRX_PREFIX_TABLE Table)
{
    PAGED_CODE();

    ExConvertExclusiveToSharedLite(&Table->TableLock);
}

/*
 * @implemented
 */
VOID
RxExtractServerName(
    IN PUNICODE_STRING FilePathName,
    OUT PUNICODE_STRING SrvCallName,
    OUT PUNICODE_STRING RestOfName)
{
    USHORT i, Length;

    PAGED_CODE();

    ASSERT(SrvCallName != NULL);

    /* SrvCall name will start from the begin up to the first separator */
    SrvCallName->Buffer = FilePathName->Buffer;
    for (i = 1; i < FilePathName->Length / sizeof(WCHAR); ++i)
    {
        if (FilePathName->Buffer[i] == OBJ_NAME_PATH_SEPARATOR)
        {
            break;
        }
    }

    /* Compute length */
    Length = (USHORT)((ULONG_PTR)&FilePathName->Buffer[i] - (ULONG_PTR)FilePathName->Buffer);
    SrvCallName->MaximumLength = Length;
    SrvCallName->Length = Length;

    /* Return the rest if asked */
    if (RestOfName != NULL)
    {
        Length = (USHORT)((ULONG_PTR)&FilePathName->Buffer[FilePathName->Length / sizeof(WCHAR)] - (ULONG_PTR)FilePathName->Buffer[i]);
        RestOfName->Buffer = &FilePathName->Buffer[i];
        RestOfName->MaximumLength = Length;
        RestOfName->Length = Length;
    }
}

/*
 * @implemented
 */
NTSTATUS
RxFcbTableInsertFcb(
    IN OUT PRX_FCB_TABLE FcbTable,
    IN OUT PFCB Fcb)
{
    PAGED_CODE();

    /* We deal with the table, make sure it's locked */
    ASSERT(RxIsFcbTableLockExclusive(FcbTable));

    /* Compute the hash */
    Fcb->FcbTableEntry.HashValue = RxTableComputePathHashValue(&Fcb->FcbTableEntry.Path);

    RxReferenceNetFcb(Fcb);

    /* If no length, it will be our null entry */
    if (Fcb->FcbTableEntry.Path.Length == 0)
    {
        FcbTable->TableEntryForNull = &Fcb->FcbTableEntry;
    }
    /* Otherwise, insert in the appropriate bucket */
    else
    {
        InsertTailList(FCB_HASH_BUCKET(FcbTable, Fcb->FcbTableEntry.HashValue),
                       &Fcb->FcbTableEntry.HashLinks);
    }

    /* Propagate the change by incrementing the version number */
    InterlockedIncrement((volatile long *)&FcbTable->Version);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
PFCB
RxFcbTableLookupFcb(
    IN  PRX_FCB_TABLE FcbTable,
    IN  PUNICODE_STRING Path)
{
    PFCB Fcb;
    PRX_FCB_TABLE_ENTRY TableEntry;

    PAGED_CODE();

    /* No path - easy, that's null entry */
    if (Path == NULL)
    {
        TableEntry = FcbTable->TableEntryForNull;
    }
    else
    {
        ULONG Hash;
        PLIST_ENTRY HashBucket, ListEntry;

        /* Otherwise, compute the hash value and find the associated bucket */
        Hash = RxTableComputePathHashValue(Path);
        HashBucket = FCB_HASH_BUCKET(FcbTable, Hash);
        /* If the bucket is empty, it means there's no entry yet */
        if (IsListEmpty(HashBucket))
        {
            TableEntry = NULL;
        }
        else
        {
            /* Otherwise, browse all the entry */
            for (ListEntry = HashBucket->Flink;
                 ListEntry != HashBucket;
                 ListEntry = ListEntry->Flink)
            {
                TableEntry = CONTAINING_RECORD(ListEntry, RX_FCB_TABLE_ENTRY, HashLinks);
                InterlockedIncrement(&FcbTable->Compares);

                /* If entry hash and string are equal, thatt's the one! */
                if (TableEntry->HashValue == Hash &&
                    TableEntry->Path.Length == Path->Length &&
                    RtlEqualUnicodeString(Path, &TableEntry->Path, FcbTable->CaseInsensitiveMatch))
                {
                    break;
                }
            }

            /* We reached the end? Not found */
            if (ListEntry == HashBucket)
            {
                TableEntry = NULL;
            }
        }
    }

    InterlockedIncrement(&FcbTable->Lookups);

    /* If table entry isn't null, return the FCB */
    if (TableEntry != NULL)
    {
        Fcb = CONTAINING_RECORD(TableEntry, FCB, FcbTableEntry);
        RxReferenceNetFcb(Fcb);
    }
    else
    {
        Fcb = NULL;
        InterlockedIncrement(&FcbTable->FailedLookups);
    }

    return Fcb;
}

/*
 * @implemented
 */
NTSTATUS
RxFcbTableRemoveFcb(
    IN OUT PRX_FCB_TABLE FcbTable,
    IN OUT PFCB Fcb)
{
    PAGED_CODE();

    ASSERT(RxIsPrefixTableLockExclusive(FcbTable));

    /* If no path, then remove entry for null */
    if (Fcb->FcbTableEntry.Path.Length == 0)
    {
        FcbTable->TableEntryForNull = NULL;
    }
    /* Otherwise, remove from the bucket */
    else
    {
        RemoveEntryList(&Fcb->FcbTableEntry.HashLinks);
    }

    /* Reset its list entry */
    InitializeListHead(&Fcb->FcbTableEntry.HashLinks);

    /* Propagate the change by incrementing the version number */
    InterlockedIncrement((volatile long *)&FcbTable->Version);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxFinalizeConnection(
    IN OUT PNET_ROOT NetRoot,
    IN OUT PV_NET_ROOT VNetRoot OPTIONAL,
    IN LOGICAL ForceFilesClosed)
{
    NTSTATUS Status;
    PRX_PREFIX_TABLE PrefixTable;
    ULONG UncleanAny, UncleanDir;
    LONG FilesOpen, AdditionalRef;
    BOOLEAN PrefixLocked, FcbTableLocked, ForceClose;

    PAGED_CODE();

    ASSERT(NodeType(NetRoot) == RDBSS_NTC_NETROOT);

    /* Get a BOOLEAN out of LOGICAL
     * -1 is like FALSE but also drops extra V_NET_ROOT reference in case of failure
     */
    ForceClose = (ForceFilesClosed == TRUE ? TRUE : FALSE);

    /* First, delete any notification change */
    Status = RxCancelNotifyChangeDirectoryRequestsForVNetRoot(VNetRoot, ForceClose);
    /* If it failed, continue if forced */
    if (Status != STATUS_SUCCESS && !ForceFilesClosed)
    {
        return Status;
    }
    /* Reset status, in case notification deletion failed */
    Status = STATUS_SUCCESS;

    PrefixTable = NetRoot->pSrvCall->RxDeviceObject->pRxNetNameTable;

    PrefixLocked = FALSE;
    FcbTableLocked = FALSE;
    FilesOpen = 0;
    AdditionalRef = 0;
    UncleanAny = 0;
    UncleanDir = 0;
    _SEH2_TRY
    {
        RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
        PrefixLocked = TRUE;

        RxReferenceNetRoot(NetRoot);

        RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);
        FcbTableLocked = TRUE;

        /* If our V_NET_ROOT wasn't finalized yet, proceed! */
        if (!VNetRoot->ConnectionFinalizationDone)
        {
            USHORT Bucket;
            PRX_FCB_TABLE FcbTable;

            DPRINT("Finalizing connection %p: %wZ\n", NetRoot, &NetRoot->PrefixEntry.Prefix);

            /* We'll browse all its associated FCB to check whether they're open/orphaned */
            FcbTable = &NetRoot->FcbTable;
            for (Bucket = 0; Bucket < FcbTable->NumberOfBuckets; ++Bucket)
            {
                PLIST_ENTRY BucketList, Entry;

                BucketList = &FcbTable->HashBuckets[Bucket];
                Entry = BucketList->Flink;
                while (Entry != BucketList)
                {
                    PFCB Fcb;

                    Fcb = CONTAINING_RECORD(Entry, FCB, FcbTableEntry.HashLinks);
                    Entry = Entry->Flink;

                    /* FCB for this connection, go ahead */
                    if (Fcb->VNetRoot == VNetRoot)
                    {
                        /* It's still open, and no force? Fail and keep track */
                        if (Fcb->UncleanCount > 0 && !ForceClose)
                        {
                            Status = STATUS_CONNECTION_IN_USE;
                            if (NodeType(Fcb) == RDBSS_NTC_STORAGE_TYPE_DIRECTORY)
                            {
                                ++UncleanDir;
                            }
                            else
                            {
                                ++UncleanAny;
                            }
                        }
                        else
                        {
                            /* Else, force purge */
                            ASSERT(NodeTypeIsFcb(Fcb));

                            Status = RxAcquireExclusiveFcb(NULL, Fcb);
                            ASSERT(Status == STATUS_SUCCESS);

                            ClearFlag(Fcb->FcbState, FCB_STATE_COLLAPSING_ENABLED);

                            RxScavengeRelatedFobxs(Fcb);
                            RxPurgeFcb(Fcb);

                            /* We don't need to release FCB lock, FCB finalize will take care of it */
                        }
                    }
                }
            }

            /* No files left, our V_NET_ROOT is finalized */
            if (VNetRoot->NumberOfFobxs == 0)
            {
                VNetRoot->ConnectionFinalizationDone = TRUE;
            }
        }

        /* Keep Number of open files and track of the extra reference */
        FilesOpen = VNetRoot->NumberOfFobxs;
        AdditionalRef = VNetRoot->AdditionalReferenceForDeleteFsctlTaken;
        /* If force close, caller doesn't want to keep connection alive
         * and wants it totally close, so drop the V_NET_ROOT too
         */
        if (ForceClose)
        {
            RxFinalizeVNetRoot(VNetRoot, FALSE, TRUE);
        }
    }
    _SEH2_FINALLY
    {
        /* Release what was acquired */
        if (FcbTableLocked)
        {
            RxReleaseFcbTableLock(&NetRoot->FcbTable);
        }

        /* If close is forced, only fix status if there are open files */
        if (ForceClose)
        {
            if (Status != STATUS_SUCCESS && UncleanAny != 0)
            {
                Status = STATUS_FILES_OPEN;
            }
        }
        /* Else, fix status and fail closing if there are open files */
        else
        {
            if ((Status != STATUS_SUCCESS && UncleanAny != 0) || FilesOpen > 0)
            {
                Status = STATUS_FILES_OPEN;
            }
        }

        DPRINT("UncleanAny: %ld, UncleanDir: %ld, FilesOpen: %ld\n", UncleanAny, UncleanDir, FilesOpen);

        /* If we're are asked to remove the extra ref, or if closing was a success, do it;
         * only if it was still referenced!
         */
        if ((ForceFilesClosed == 0xFF || Status == STATUS_SUCCESS) && AdditionalRef != 0)
        {
            VNetRoot->AdditionalReferenceForDeleteFsctlTaken = 0;
            RxDereferenceVNetRoot(VNetRoot, LHS_ExclusiveLockHeld);
        }

        if (PrefixLocked)
        {
            RxDereferenceNetRoot(NetRoot, LHS_ExclusiveLockHeld);
            RxReleasePrefixTableLock(PrefixTable);
        }
    }
    _SEH2_END;

    return Status;
}

/*
 * @implemented
 */
VOID
RxFinalizeFcbTable(
    IN OUT PRX_FCB_TABLE FcbTable)
{
    USHORT Bucket;

    PAGED_CODE();

    /* Just delete the lock */
    ExDeleteResourceLite(&FcbTable->TableLock);

    /* And make sure (checked) that the table is really empty... */
    for (Bucket = 0; Bucket < FcbTable->NumberOfBuckets; ++Bucket)
    {
        ASSERT(IsListEmpty(&FcbTable->HashBuckets[Bucket]));
    }
}

/*
 * @implemented
 */
BOOLEAN
RxFinalizeNetFcb(
    OUT PFCB ThisFcb,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize,
    IN LONG ReferenceCount)
{
    PAGED_CODE();

    DPRINT("RxFinalizeNetFcb(%p, %d, %d, %d)\n", ThisFcb, RecursiveFinalize, ForceFinalize, ReferenceCount);
    DPRINT("Finalize: %wZ\n", &ThisFcb->FcbTableEntry.Path);

    /* Make sure we have an exclusively acquired FCB */
    ASSERT_CORRECT_FCB_STRUCTURE(ThisFcb);
    ASSERT(RxIsFcbAcquiredExclusive(ThisFcb));

    /* We shouldn't force finalization... */
    ASSERT(!ForceFinalize);

    /* If recurisve, finalize all the associated SRV_OPEN */
    if (RecursiveFinalize)
    {
        PLIST_ENTRY ListEntry;

        for (ListEntry = ThisFcb->SrvOpenList.Flink;
             ListEntry != &ThisFcb->SrvOpenList;
             ListEntry = ListEntry->Flink)
        {
            PSRV_OPEN SrvOpen;

            SrvOpen = CONTAINING_RECORD(ListEntry, SRV_OPEN, SrvOpenQLinks);
            RxFinalizeSrvOpen(SrvOpen, TRUE, ForceFinalize);
        }
    }
    /* If FCB is still in use, that's over */
    else
    {
        if (ThisFcb->OpenCount != 0 || ThisFcb->UncleanCount != 0)
        {
            ASSERT(ReferenceCount > 0);

            return FALSE;
        }
    }

    ASSERT(ReferenceCount >= 1);

    /* If FCB is still referenced, that's over - unless you force it and want to BSOD somewhere */
    if (ReferenceCount != 1 && !ForceFinalize)
    {
        return FALSE;
    }

    ASSERT(ForceFinalize || ((ThisFcb->OpenCount == 0) && (ThisFcb->UncleanCount == 0)));

    DPRINT("Finalizing FCB open: %d (%d)\n", ThisFcb->OpenCount, ForceFinalize);

    /* If finalization was not already initiated, go ahead */
    if (!ThisFcb->UpperFinalizationDone)
    {
        /* Free any FCB_LOCK */
        if (NodeType(ThisFcb) == RDBSS_NTC_STORAGE_TYPE_FILE)
        {
            FsRtlUninitializeFileLock(&ThisFcb->Specific.Fcb.FileLock);

            while (ThisFcb->BufferedLocks.List != NULL)
            {
                PFCB_LOCK Entry;

                Entry = ThisFcb->BufferedLocks.List;
                ThisFcb->BufferedLocks.List = Entry->Next;

                RxFreePool(Entry);
            }
        }

        /* If not orphaned, it still has a NET_ROOT and potentially is still in a table */
        if (!BooleanFlagOn(ThisFcb->FcbState, FCB_STATE_ORPHANED))
        {
            PNET_ROOT NetRoot;

            NetRoot = (PNET_ROOT)ThisFcb->pNetRoot;

            ASSERT(RxIsFcbTableLockExclusive(&NetRoot->FcbTable));
            /* So, remove it */
            if (!BooleanFlagOn(ThisFcb->FcbState, FCB_STATE_NAME_ALREADY_REMOVED))
            {
                RxFcbTableRemoveFcb(&NetRoot->FcbTable, ThisFcb);
            }
        }

        ThisFcb->UpperFinalizationDone = TRUE;
    }

    ASSERT(ReferenceCount >= 1);

    /* Even if forced, don't allow broken free */
    if (ReferenceCount != 1)
    {
        return FALSE;
    }

    /* Now, release everything */
    if (ThisFcb->pBufferingStateChangeCompletedEvent != NULL)
    {
        RxFreePool(ThisFcb->pBufferingStateChangeCompletedEvent);
    }

    if (ThisFcb->MRxDispatch != NULL)
    {
        ThisFcb->MRxDispatch->MRxDeallocateForFcb(RX_GET_MRX_FCB(ThisFcb));
    }

    ExDeleteResourceLite(ThisFcb->BufferedLocks.Resource);
    ExDeleteResourceLite(ThisFcb->Header.Resource);
    ExDeleteResourceLite(ThisFcb->Header.PagingIoResource);

    InterlockedDecrement((volatile long *)&ThisFcb->pNetRoot->NumberOfFcbs);
    RxDereferenceVNetRoot(ThisFcb->VNetRoot, LHS_LockNotHeld);

    ASSERT(IsListEmpty(&ThisFcb->FcbTableEntry.HashLinks));
    ASSERT(!ThisFcb->fMiniInited);

    /* And free the object */
    RxFreeFcbObject(ThisFcb);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
RxFinalizeNetFobx(
    _Out_ PFOBX ThisFobx,
    _In_ BOOLEAN RecursiveFinalize,
    _In_ BOOLEAN ForceFinalize)
{
    PFCB Fcb;
    PSRV_OPEN SrvOpen;

    PAGED_CODE();

    ASSERT(NodeType(ThisFobx) == RDBSS_NTC_FOBX);

    /* Only finalize if forced or if there's no ref left */
    if (ThisFobx->NodeReferenceCount != 0 &&
        !ForceFinalize)
    {
        return FALSE;
    }

    DPRINT("Finalize Fobx: %p (with %d ref), forced: %d\n", ThisFobx, ThisFobx->NodeReferenceCount, ForceFinalize);

    SrvOpen = ThisFobx->SrvOpen;
    Fcb = SrvOpen->Fcb;
    /* If it wasn't finalized yet, do it */
    if (!ThisFobx->UpperFinalizationDone)
    {
        ASSERT(NodeType(SrvOpen->Fcb) != RDBSS_NTC_OPENTARGETDIR_FCB);
        ASSERT(RxIsFcbAcquiredExclusive(SrvOpen->Fcb));

        /* Remove it from the SRV_OPEN */
        RemoveEntryList(&ThisFobx->FobxQLinks);

        /* If we were used to browse a directory, free the query buffer */
        if (BooleanFlagOn(ThisFobx->Flags, FOBX_FLAG_FREE_UNICODE))
        {
            RxFreePoolWithTag(ThisFobx->UnicodeQueryTemplate.Buffer, RX_DIRCTL_POOLTAG);
        }

        /* Notify the mini-rdr */
        if (Fcb->MRxDispatch != NULL && Fcb->MRxDispatch->MRxDeallocateForFobx != NULL)
        {
            Fcb->MRxDispatch->MRxDeallocateForFobx((PMRX_FOBX)ThisFobx);
        }

        /* If the SRV_OPEN wasn't closed yet, do it */
        if (!BooleanFlagOn(ThisFobx->Flags, FOBX_FLAG_SRVOPEN_CLOSED))
        {
            NTSTATUS Status;

            Status = RxCloseAssociatedSrvOpen(ThisFobx, FALSE);
            DPRINT("Closing SRV_OPEN %p for %p: %x\n", SrvOpen, ThisFobx, Status);
        }

        /* Finalization done */
        ThisFobx->UpperFinalizationDone = TRUE;
    }

    /* If we're still referenced, don't go any further! */
    if (ThisFobx->NodeReferenceCount != 0)
    {
        return FALSE;
    }

    /* At that point, everything should be closed */
    ASSERT(IsListEmpty(&ThisFobx->ClosePendingList));

    /* Was the FOBX allocated with another object?
     * If so, mark the buffer free in said object
     */
    if (ThisFobx == Fcb->InternalFobx)
    {
        ClearFlag(Fcb->FcbState, FCB_STATE_FOBX_USED);
    }
    else if (ThisFobx == SrvOpen->InternalFobx)
    {
        ClearFlag(SrvOpen->Flags, SRVOPEN_FLAG_FOBX_USED);
    }

    ThisFobx->pSrvOpen = NULL;

    /* A FOBX less */
    InterlockedDecrement((volatile long *)&SrvOpen->pVNetRoot->NumberOfFobxs);

    RxDereferenceSrvOpen(SrvOpen, LHS_ExclusiveLockHeld);

    /* If it wasn't allocated with another object, free the FOBX */
    if (!BooleanFlagOn(ThisFobx->Flags, FOBX_FLAG_ENCLOSED_ALLOCATED))
    {
        RxFreeFcbObject(ThisFobx);
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
RxFinalizeNetRoot(
    OUT PNET_ROOT ThisNetRoot,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize)
{
    PSRV_CALL SrvCall;
    PRX_FCB_TABLE FcbTable;
    PRX_PREFIX_TABLE PrefixTable;

    PAGED_CODE();

    ASSERT(NodeType(ThisNetRoot) == RDBSS_NTC_NETROOT);

    PrefixTable = ThisNetRoot->pSrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT(RxIsPrefixTableLockAcquired(PrefixTable));

    /* If sme finalization is already ongoing, leave */
    if (BooleanFlagOn(ThisNetRoot->Flags, NETROOT_FLAG_FINALIZATION_IN_PROGRESS))
    {
        return FALSE;
    }

    /* Mark we're finalizing */
    SetFlag(ThisNetRoot->Flags, NETROOT_FLAG_FINALIZATION_IN_PROGRESS);

    FcbTable = &ThisNetRoot->FcbTable;
    /* Did caller asked us to finalize any associated FCB? */
    if (RecursiveFinalize)
    {
        USHORT Bucket;

        /* Browse all the FCBs in our FCB table */
        RxAcquireFcbTableLockExclusive(FcbTable, TRUE);
        for (Bucket = 0; Bucket < FcbTable->NumberOfBuckets; ++Bucket)
        {
            PLIST_ENTRY HashBucket, ListEntry;

            HashBucket = &FcbTable->HashBuckets[Bucket];
            ListEntry = HashBucket->Flink;
            while (ListEntry != HashBucket)
            {
                PFCB Fcb;

                Fcb = CONTAINING_RECORD(ListEntry, FCB, FcbTableEntry.HashLinks);
                ASSERT(NodeTypeIsFcb(Fcb));

                ListEntry = ListEntry->Flink;

                /* If the FCB isn't orphaned, then, it's time to purge it */
                if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_ORPHANED))
                {
                    NTSTATUS Status;

                    Status = RxAcquireExclusiveFcb(NULL, Fcb);
                    ASSERT(Status == STATUS_SUCCESS);
                    RxPurgeFcb(Fcb);
                }
            }
        }
        RxReleaseFcbTableLock(FcbTable);
    }

    /* Only finalize if forced or if there's a single ref left */
    if (ThisNetRoot->NodeReferenceCount != 1 && !ForceFinalize)
    {
        return FALSE;
    }

    DPRINT("Finalizing NetRoot %p for %wZ\n", ThisNetRoot, &ThisNetRoot->PrefixEntry.Prefix);

    /* If we're still referenced, don't go any further! */
    if (ThisNetRoot->NodeReferenceCount != 1)
    {
        return FALSE;
    }

    /* Finalize the FCB table (and make sure it's empty!) */
    RxFinalizeFcbTable(FcbTable);

    /* If name wasn't remove already, do it now */
    if (!BooleanFlagOn(ThisNetRoot->Flags, NETROOT_FLAG_NAME_ALREADY_REMOVED))
    {
        RxRemovePrefixTableEntry(PrefixTable, &ThisNetRoot->PrefixEntry);
    }

    /* Delete the object */
    SrvCall = (PSRV_CALL)ThisNetRoot->pSrvCall;
    RxFreeObject(ThisNetRoot);

    /* And dereference the associated SRV_CALL */
    if (SrvCall != NULL)
    {
        RxDereferenceSrvCall(SrvCall, LHS_ExclusiveLockHeld);
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
RxFinalizeSrvCall(
    OUT PSRV_CALL ThisSrvCall,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize)
{
    PRX_PREFIX_TABLE PrefixTable;

    PAGED_CODE();

    ASSERT(NodeType(ThisSrvCall) == RDBSS_NTC_SRVCALL);

    PrefixTable = ThisSrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT(RxIsPrefixTableLockAcquired(PrefixTable));

    /* Only finalize if forced or if there's a single ref left */
    if (ThisSrvCall->NodeReferenceCount != 1 &&
        !ForceFinalize)
    {
        return FALSE;
    }

    DPRINT("Finalizing SrvCall %p for %wZ\n", ThisSrvCall, &ThisSrvCall->PrefixEntry.Prefix);

    /* If it wasn't finalized yet, do it */
    if (!ThisSrvCall->UpperFinalizationDone)
    {
        BOOLEAN WillFree;

        /* Remove ourselves from prefix table */
        RxRemovePrefixTableEntry(PrefixTable, &ThisSrvCall->PrefixEntry);

        /* Remember our third arg, in case we get queued for later execution */
        if (ForceFinalize)
        {
            SetFlag(ThisSrvCall->Flags, SRVCALL_FLAG_FORCE_FINALIZED);
        }

        /* And done */
        ThisSrvCall->UpperFinalizationDone = TRUE;

        /* Would defered execution free the object? */
        WillFree = (ThisSrvCall->NodeReferenceCount == 1);

        /* If we have a device object */
        if (ThisSrvCall->RxDeviceObject != NULL)
        {
            NTSTATUS Status;

            /* If we're not executing in the RDBSS thread, queue for execution within the thread */
            if (RxGetRDBSSProcess() != IoGetCurrentProcess())
            {
                /* Extra ref, as usual */
                InterlockedIncrement((volatile long *)&ThisSrvCall->NodeReferenceCount);
                /* And dispatch */
                RxDispatchToWorkerThread(ThisSrvCall->RxDeviceObject, DelayedWorkQueue, RxpDestroySrvCall, ThisSrvCall);

                /* Return to the caller, in advance, whether we're freeing the object or not */
                return WillFree;
            }

            /* If in the right thread already, call the mini-rdr */
            MINIRDR_CALL_THROUGH(Status, ThisSrvCall->RxDeviceObject->Dispatch,
                                 MRxFinalizeSrvCall, ((PMRX_SRV_CALL)ThisSrvCall, ForceFinalize));
            (void)Status;
        }
    }

    /* If we're still referenced, don't go any further! */
    if (ThisSrvCall->NodeReferenceCount != 1)
    {
        return FALSE;
    }

    /* Don't leak */
    if (ThisSrvCall->pDomainName != NULL)
    {
        RxFreePool(ThisSrvCall->pDomainName);
    }

    /* And free! */
    RxTearDownBufferingManager(ThisSrvCall);
    RxFreeObject(ThisSrvCall);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
RxFinalizeSrvOpen(
    OUT PSRV_OPEN ThisSrvOpen,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize)
{
    PFCB Fcb;

    PAGED_CODE();

    /* We have to have a SRV_OPEN */
    ASSERT(NodeType(ThisSrvOpen) == RDBSS_NTC_SRVOPEN);

    /* If that's a recursive finalization, finalize any related FOBX */
    if (RecursiveFinalize)
    {
        PLIST_ENTRY ListEntry;

        ListEntry = ThisSrvOpen->FobxList.Flink;
        while (ListEntry != &ThisSrvOpen->FobxList)
        {
            PFOBX Fobx;

            Fobx = CONTAINING_RECORD(ListEntry, FOBX, FobxQLinks);
            ListEntry = ListEntry->Flink;
            RxFinalizeNetFobx(Fobx, TRUE, ForceFinalize);
        }
    }

    /* If we have still references, don't finalize unless forced */
    if (ThisSrvOpen->NodeReferenceCount != 0 &&
        !ForceFinalize)
    {
        return FALSE;
    }

    DPRINT("Finalize SRV_OPEN: %p (with %d ref), forced: %d\n", ThisSrvOpen, ThisSrvOpen->NodeReferenceCount, ForceFinalize);

    /* Only finalize if closed, or if it wasn't already done and SRV_OPEN is in a bad shape */
    Fcb = (PFCB)ThisSrvOpen->pFcb;
    if ((!ThisSrvOpen->UpperFinalizationDone && ThisSrvOpen->Condition != Condition_Good) ||
        BooleanFlagOn(ThisSrvOpen->Flags, SRVOPEN_FLAG_CLOSED))
    {
        PV_NET_ROOT VNetRoot;

        /* Associated FCB can't be fake one */
        ASSERT(NodeType(Fcb) != RDBSS_NTC_OPENTARGETDIR_FCB);
        ASSERT(RxIsFcbAcquiredExclusive (Fcb));

        /* Purge any pending operation */
        RxPurgeChangeBufferingStateRequestsForSrvOpen(ThisSrvOpen);

        /* If the FCB wasn't orphaned, inform the mini-rdr about close */
        if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_ORPHANED))
        {
            NTSTATUS Status;

            MINIRDR_CALL_THROUGH(Status, Fcb->MRxDispatch, MRxForceClosed, ((PMRX_SRV_OPEN)ThisSrvOpen));
            (void)Status;
        }

        /* Remove ourselves from the FCB */
        RemoveEntryList(&ThisSrvOpen->SrvOpenQLinks);
        InitializeListHead(&ThisSrvOpen->SrvOpenQLinks);
        ++Fcb->SrvOpenListVersion;

        /* If we have a V_NET_ROOT, dereference it */
        VNetRoot = (PV_NET_ROOT)ThisSrvOpen->pVNetRoot;
        if (VNetRoot != NULL)
        {
            InterlockedDecrement((volatile long *)&VNetRoot->pNetRoot->NumberOfSrvOpens);
            RxDereferenceVNetRoot(VNetRoot, LHS_LockNotHeld);
            ThisSrvOpen->pVNetRoot = NULL;
        }

        /* Finalization done */
        ThisSrvOpen->UpperFinalizationDone = TRUE;
    }

    /* Don't free memory if still referenced */
    if (ThisSrvOpen->NodeReferenceCount != 0)
    {
        return FALSE;
    }

    /* No key association left */
    ASSERT(IsListEmpty(&ThisSrvOpen->SrvOpenKeyList));

    /* If we're still in some FCB, remove us */
    if (!IsListEmpty(&ThisSrvOpen->SrvOpenQLinks))
    {
        RemoveEntryList(&ThisSrvOpen->SrvOpenQLinks);
    }

    /* If enclosed allocation, mark the memory zone free */
    if (BooleanFlagOn(ThisSrvOpen->Flags, SRVOPEN_FLAG_ENCLOSED_ALLOCATED))
    {
        ClearFlag(Fcb->FcbState, FCB_STATE_SRVOPEN_USED);
    }
    /* Otherwise, free the memory */
    else
    {
        RxFreeFcbObject(ThisSrvOpen);
    }

    RxDereferenceNetFcb(Fcb);

    return TRUE;
}

/*
 * @implemented
 */
BOOLEAN
RxFinalizeVNetRoot(
    OUT PV_NET_ROOT ThisVNetRoot,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize)
{
    PNET_ROOT NetRoot;
    PRX_PREFIX_TABLE PrefixTable;

    PAGED_CODE();

    ASSERT(NodeType(ThisVNetRoot) == RDBSS_NTC_V_NETROOT);

    PrefixTable = ThisVNetRoot->pNetRoot->pSrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT(RxIsPrefixTableLockAcquired(PrefixTable));

    /* Only finalize if forced or if there's a single ref left */
    if (ThisVNetRoot->NodeReferenceCount != 1 &&
        !ForceFinalize)
    {
        return FALSE;
    }

    DPRINT("Finalizing VNetRoot %p for %wZ\n", ThisVNetRoot, &ThisVNetRoot->PrefixEntry.Prefix);

    NetRoot = (PNET_ROOT)ThisVNetRoot->pNetRoot;
    /* If it wasn't finalized yet, do it */
    if (!ThisVNetRoot->UpperFinalizationDone)
    {
        ASSERT(NodeType(NetRoot) == RDBSS_NTC_NETROOT);

        /* Reference the NetRoot so that it doesn't disappear */
        RxReferenceNetRoot(NetRoot);
        RxOrphanSrvOpens(ThisVNetRoot);
        /* Remove us from the available VNetRoot for NetRoot */
        RxRemoveVirtualNetRootFromNetRoot(NetRoot, ThisVNetRoot);
        /* Remove extra ref */
        RxDereferenceNetRoot(NetRoot, LHS_ExclusiveLockHeld);

        /* Remove ourselves from prefix table */
        RxRemovePrefixTableEntry(PrefixTable, &ThisVNetRoot->PrefixEntry);

        /* Finalization done */
        ThisVNetRoot->UpperFinalizationDone = TRUE;
    }

    /* If we're still referenced, don't go any further! */
    if (ThisVNetRoot->NodeReferenceCount != 1)
    {
        return FALSE;
    }

    /* If there's an associated device, notify mini-rdr */
    if (NetRoot->pSrvCall->RxDeviceObject != NULL)
    {
        NTSTATUS Status;

        MINIRDR_CALL_THROUGH(Status, NetRoot->pSrvCall->RxDeviceObject->Dispatch,
                             MRxFinalizeVNetRoot, ((PMRX_V_NET_ROOT)ThisVNetRoot, FALSE));
        (void)Status;
    }

    /* Free parameters */
    RxUninitializeVNetRootParameters(ThisVNetRoot->pUserName, ThisVNetRoot->pUserDomainName,
                                     ThisVNetRoot->pPassword, &ThisVNetRoot->Flags);
    /* Dereference our NetRoot, we won't reference it anymore */
    RxDereferenceNetRoot(NetRoot, LHS_ExclusiveLockHeld);

    /* And free the object! */
    RxFreePoolWithTag(ThisVNetRoot, RX_V_NETROOT_POOLTAG);

    return TRUE;
}

NTSTATUS
RxFindOrConstructVirtualNetRoot(
    IN PRX_CONTEXT RxContext,
    IN PUNICODE_STRING CanonicalName,
    IN NET_ROOT_TYPE NetRootType,
    IN PUNICODE_STRING RemainingName)
{
    ULONG Flags;
    NTSTATUS Status;
    PVOID Container;
    BOOLEAN Construct;
    PV_NET_ROOT VNetRoot;
    RX_CONNECTION_ID ConnectionID;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    LOCK_HOLDING_STATE LockHoldingState;

    PAGED_CODE();

    RxDeviceObject = RxContext->RxDeviceObject;
    ASSERT(RxDeviceObject->Dispatch != NULL);
    ASSERT(NodeType(RxDeviceObject->Dispatch) == RDBSS_NTC_MINIRDR_DISPATCH);

    /* Ask the mini-rdr for connection ID */
    ConnectionID.SessionID = 0;
    if (RxDeviceObject->Dispatch->MRxGetConnectionId != NULL)
    {
        Status = RxDeviceObject->Dispatch->MRxGetConnectionId(RxContext, &ConnectionID);
        if (!NT_SUCCESS(Status) && Status != STATUS_NOT_IMPLEMENTED)
        {
            /* mini-rdr is expected not to fail - unless it's not implemented */
            DPRINT1("Failed to initialize connection ID\n");
            ASSERT(FALSE);
        }
    }

    RxContext->Create.NetNamePrefixEntry = NULL;

    Status = STATUS_MORE_PROCESSING_REQUIRED;
    RxAcquirePrefixTableLockShared(RxDeviceObject->pRxNetNameTable, TRUE);
    LockHoldingState = LHS_SharedLockHeld;
    Construct = TRUE;
    Flags = 0;

    /* We will try twice to find a matching VNetRoot: shared locked and then exlusively locked */
    while (TRUE)
    {
        PNET_ROOT NetRoot;
        PV_NET_ROOT SavedVNetRoot;

        /* Look in prefix table */
        Container = RxPrefixTableLookupName(RxDeviceObject->pRxNetNameTable, CanonicalName, RemainingName, &ConnectionID);
        if (Container != NULL)
        {
            /* If that's not a VNetRoot, that's a SrvCall, not interesting, loop again */
            if (NodeType(Container) != RDBSS_NTC_V_NETROOT)
            {
                ASSERT(NodeType(Container) == RDBSS_NTC_SRVCALL);
                RxDereferenceSrvCall(Container, LockHoldingState);
            }
            else
            {
                VNetRoot = Container;
                NetRoot = VNetRoot->NetRoot;

                /* If the matching VNetRoot isn't in a good shape, there's something wrong - fail */
                if ((NetRoot->Condition != Condition_InTransition && NetRoot->Condition != Condition_Good) ||
                    NetRoot->SrvCall->RxDeviceObject != RxContext->RxDeviceObject)
                {
                    Status = STATUS_BAD_NETWORK_PATH;
                    SavedVNetRoot = NULL;
                }
                else
                {
                    LUID LogonId;
                    ULONG SessionId;
                    PUNICODE_STRING UserName, UserDomain, Password;

                    /* We can reuse if we use same credentials */
                    Status = RxInitializeVNetRootParameters(RxContext, &LogonId,
                                                            &SessionId, &UserName,
                                                            &UserDomain, &Password,
                                                            &Flags);
                    if (NT_SUCCESS(Status))
                    {
                        SavedVNetRoot = VNetRoot;
                        Status = RxCheckVNetRootCredentials(RxContext, VNetRoot,
                                                            &LogonId, UserName,
                                                            UserDomain, Password,
                                                            Flags);
                        if (Status == STATUS_MORE_PROCESSING_REQUIRED)
                        {
                            PLIST_ENTRY ListEntry;

                            for (ListEntry = NetRoot->VirtualNetRoots.Flink;
                                 ListEntry != &NetRoot->VirtualNetRoots;
                                 ListEntry = ListEntry->Flink)
                            {
                                SavedVNetRoot = CONTAINING_RECORD(ListEntry, V_NET_ROOT, NetRootListEntry);
                                Status = RxCheckVNetRootCredentials(RxContext, SavedVNetRoot,
                                                                    &LogonId, UserName,
                                                                    UserDomain, Password,
                                                                    Flags);
                                if (Status != STATUS_MORE_PROCESSING_REQUIRED)
                                {
                                    break;
                                }
                            }

                            if (ListEntry == &NetRoot->VirtualNetRoots)
                            {
                                SavedVNetRoot = NULL;
                            }
                        }

                        if (!NT_SUCCESS(Status))
                        {
                            SavedVNetRoot = NULL;
                        }

                        RxUninitializeVNetRootParameters(UserName, UserDomain, Password, &Flags);
                    }
                }

                /* We'll fail, if we had referenced a VNetRoot, dereference it */
                if (Status != STATUS_MORE_PROCESSING_REQUIRED && !NT_SUCCESS(Status))
                {
                    if (SavedVNetRoot == NULL)
                    {
                        RxDereferenceVNetRoot(VNetRoot, LockHoldingState);
                    }
                }
                /* Reference VNetRoot we'll keep, and dereference current */
                else if (SavedVNetRoot != VNetRoot)
                {
                    RxDereferenceVNetRoot(VNetRoot, LockHoldingState);
                    if (SavedVNetRoot != NULL)
                    {
                        RxReferenceVNetRoot(SavedVNetRoot);
                    }
                }
            }

            /* We may have found something, or we fail hard, so don't attempt to create a VNetRoot */
            if (Status != STATUS_MORE_PROCESSING_REQUIRED)
            {
                Construct = FALSE;
                break;
            }
        }

        /* If we're locked exclusive, we won't loop again, it was the second pass */
        if (LockHoldingState != LHS_SharedLockHeld)
        {
            break;
        }

        /* Otherwise, prepare for second pass, exclusive, making sure we can acquire without delay */
        if (RxAcquirePrefixTableLockExclusive(RxDeviceObject->pRxNetNameTable, FALSE))
        {
            RxReleasePrefixTableLock(RxDeviceObject->pRxNetNameTable);
            LockHoldingState = LHS_ExclusiveLockHeld;
            break;
        }

        RxReleasePrefixTableLock(RxDeviceObject->pRxNetNameTable);
        RxAcquirePrefixTableLockExclusive(RxDeviceObject->pRxNetNameTable, TRUE);
        LockHoldingState = LHS_ExclusiveLockHeld;
    }

    /* We didn't fail, and didn't find any VNetRoot, construct one */
    if (Construct)
    {
        ASSERT(LockHoldingState == LHS_ExclusiveLockHeld);

        Status = RxConstructVirtualNetRoot(RxContext, CanonicalName, NetRootType, &VNetRoot, &LockHoldingState, &ConnectionID);
        ASSERT(Status != STATUS_SUCCESS || LockHoldingState != LHS_LockNotHeld);

        if (Status == STATUS_SUCCESS)
        {
            DPRINT("CanonicalName: %wZ (%d)\n", CanonicalName, CanonicalName->Length);
            DPRINT("VNetRoot: %wZ (%d)\n", &VNetRoot->PrefixEntry.Prefix, VNetRoot->PrefixEntry.Prefix.Length);
            ASSERT(CanonicalName->Length >= VNetRoot->PrefixEntry.Prefix.Length);

            RemainingName->Buffer = Add2Ptr(CanonicalName->Buffer, VNetRoot->PrefixEntry.Prefix.Length);
            RemainingName->Length = CanonicalName->Length - VNetRoot->PrefixEntry.Prefix.Length;
            RemainingName->MaximumLength = RemainingName->Length;

            if (BooleanFlagOn(Flags, VNETROOT_FLAG_CSCAGENT_INSTANCE))
            {
                DPRINT("CSC instance, VNetRoot: %p\n", VNetRoot);
            }
            VNetRoot->Flags |= Flags;
        }
    }

    /* Release the prefix table - caller expects it to be released */
    if (LockHoldingState != LHS_LockNotHeld)
    {
        RxReleasePrefixTableLock(RxDeviceObject->pRxNetNameTable);
    }

    /* If we failed creating, quit */
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("RxFindOrConstructVirtualNetRoot() = Status: %x\n", Status);
        return Status;
    }

    /* Otherwise, wait until the VNetRoot is stable */
    DPRINT("Waiting for stable condition for: %p\n", VNetRoot);
    RxWaitForStableVNetRoot(VNetRoot, RxContext);
    /* It's all good, update the RX_CONTEXT with all our structs */
    if (VNetRoot->Condition == Condition_Good)
    {
        PNET_ROOT NetRoot;

        NetRoot = VNetRoot->NetRoot;
        RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
        RxContext->Create.pNetRoot = (PMRX_NET_ROOT)NetRoot;
        RxContext->Create.pSrvCall = (PMRX_SRV_CALL)NetRoot->SrvCall;
    }
    else
    {
        RxDereferenceVNetRoot(VNetRoot, LHS_LockNotHeld);
        RxContext->Create.pVNetRoot = NULL;
        Status = STATUS_BAD_NETWORK_PATH;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxFindOrCreateConnections(
    _In_ PRX_CONTEXT RxContext,
    _In_ PUNICODE_STRING CanonicalName,
    _In_ NET_ROOT_TYPE NetRootType,
    _Out_ PUNICODE_STRING LocalNetRootName,
    _Out_ PUNICODE_STRING FilePathName,
    _Inout_ PLOCK_HOLDING_STATE LockState,
    _In_ PRX_CONNECTION_ID RxConnectionId)
{
    PVOID Container;
    PSRV_CALL SrvCall;
    PNET_ROOT NetRoot;
    PV_NET_ROOT VNetRoot;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PRX_PREFIX_TABLE PrefixTable;
    UNICODE_STRING RemainingName, NetRootName;

    PAGED_CODE();

    DPRINT("RxFindOrCreateConnections(%p, %wZ, %x, %p, %p, %p, %p)\n",
           RxContext, CanonicalName, NetRootType, LocalNetRootName,
           FilePathName, LockState, RxConnectionId);

    *FilePathName = *CanonicalName;
    LocalNetRootName->Length = 0;
    LocalNetRootName->MaximumLength = 0;
    LocalNetRootName->Buffer = CanonicalName->Buffer;

    /* UNC path, split it */
    if (FilePathName->Buffer[1] == ';')
    {
        BOOLEAN Slash;
        USHORT i, Length;

        Slash = FALSE;
        for (i = 2; i < FilePathName->Length / sizeof(WCHAR); ++i)
        {
            if (FilePathName->Buffer[i] == OBJ_NAME_PATH_SEPARATOR)
            {
                Slash = TRUE;
                break;
            }
        }

        if (!Slash)
        {
            return STATUS_OBJECT_NAME_INVALID;
        }

        FilePathName->Buffer = &FilePathName->Buffer[i];
        Length = (USHORT)((ULONG_PTR)FilePathName->Buffer - (ULONG_PTR)LocalNetRootName->Buffer);
        LocalNetRootName->Length = Length;
        LocalNetRootName->MaximumLength = Length;
        FilePathName->Length -= Length;

        DPRINT("CanonicalName: %wZ\n", CanonicalName);
        DPRINT(" -> FilePathName: %wZ\n", FilePathName);
        DPRINT(" -> LocalNetRootName: %wZ\n", LocalNetRootName);
    }

    Container = NULL;
    PrefixTable = RxContext->RxDeviceObject->pRxNetNameTable;

    _SEH2_TRY
    {
RetryLookup:
        ASSERT(*LockState != LHS_LockNotHeld);

        /* If previous lookup left something, dereference it */
        if (Container != NULL)
        {
            switch (NodeType(Container))
            {
                case RDBSS_NTC_SRVCALL:
                    RxDereferenceSrvCall(Container, *LockState);
                    break;

                case RDBSS_NTC_NETROOT:
                    RxDereferenceNetRoot(Container, *LockState);
                    break;

                case RDBSS_NTC_V_NETROOT:
                    RxDereferenceVNetRoot(Container, *LockState);
                    break;

                default:
                    /* Should never happen */
                    ASSERT(FALSE);
                    break;
            }
        }

        /* Look for our NetRoot in prefix table */
        Container = RxPrefixTableLookupName(PrefixTable, FilePathName, &RemainingName, RxConnectionId);
        DPRINT("Container %p for path %wZ\n", Container, FilePathName);

        while (TRUE)
        {
            UNICODE_STRING SrvCallName;

            SrvCall = NULL;
            NetRoot = NULL;
            VNetRoot = NULL;

            /* Assume we didn't succeed */
            RxContext->Create.pVNetRoot = NULL;
            RxContext->Create.pNetRoot = NULL;
            RxContext->Create.pSrvCall = NULL;
            RxContext->Create.Type = NetRootType;

            /* If we found something */
            if (Container != NULL)
            {
                /* A VNetRoot */
                if (NodeType(Container) == RDBSS_NTC_V_NETROOT)
                {
                    VNetRoot = Container;
                    /* Use its NetRoot */
                    NetRoot = VNetRoot->NetRoot;

                    /* If it's not stable, wait for it to be stable */
                    if (NetRoot->Condition == Condition_InTransition)
                    {
                        RxReleasePrefixTableLock(PrefixTable);
                        DPRINT("Waiting for stable condition for: %p\n", NetRoot);
                        RxWaitForStableNetRoot(NetRoot, RxContext);
                        RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
                        *LockState = LHS_ExclusiveLockHeld;

                        /* Now that's it's ok, retry lookup to find what we want */
                        if (NetRoot->Condition == Condition_Good)
                        {
                            goto RetryLookup;
                        }
                    }

                    /* Is the associated netroot good? */
                    if (NetRoot->Condition == Condition_Good)
                    {
                        SrvCall = (PSRV_CALL)NetRoot->pSrvCall;

                        /* If it is, and SrvCall as well, then, we have our active connection */
                        if (SrvCall->Condition == Condition_Good &&
                            SrvCall->RxDeviceObject == RxContext->RxDeviceObject)
                        {
                            RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
                            RxContext->Create.pNetRoot = (PMRX_NET_ROOT)NetRoot;
                            RxContext->Create.pSrvCall = (PMRX_SRV_CALL)SrvCall;

                            Status = STATUS_CONNECTION_ACTIVE;
                            _SEH2_LEAVE;
                        }
                    }

                    /* If VNetRoot was well constructed, it means the connection is active */
                    if (VNetRoot->ConstructionStatus == STATUS_SUCCESS)
                    {
                        Status = STATUS_CONNECTION_ACTIVE;
                    }
                    else
                    {
                        Status = VNetRoot->ConstructionStatus;
                    }

                    RxDereferenceVNetRoot(VNetRoot, *LockState);
                    _SEH2_LEAVE;
                }
                /* Can only be a SrvCall */
                else
                {
                    ASSERT(NodeType(Container) == RDBSS_NTC_SRVCALL);
                    SrvCall = Container;

                    /* Wait for the SRV_CALL to be stable */
                    if (SrvCall->Condition == Condition_InTransition)
                    {
                        RxReleasePrefixTableLock(PrefixTable);
                        DPRINT("Waiting for stable condition for: %p\n", SrvCall);
                        RxWaitForStableSrvCall(SrvCall, RxContext);
                        RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
                        *LockState = LHS_ExclusiveLockHeld;

                        /* It went good, loop again to find what we look for */
                        if (SrvCall->Condition == Condition_Good)
                        {
                            goto RetryLookup;
                        }
                    }

                    /* If it's not good... */
                    if (SrvCall->Condition != Condition_Good)
                    {
                        /* But SRV_CALL was well constructed, assume a connection was active */
                        if (SrvCall->Status == STATUS_SUCCESS)
                        {
                            Status = STATUS_CONNECTION_ACTIVE;
                        }
                        else
                        {
                            Status = SrvCall->Status;
                        }

                        RxDereferenceSrvCall(SrvCall, *LockState);
                        _SEH2_LEAVE;
                    }
                }
            }

            /* If we found a SRV_CALL not matching our DO, quit */
            if (SrvCall != NULL && SrvCall->Condition == Condition_Good &&
                SrvCall->RxDeviceObject != RxContext->RxDeviceObject)
            {
                RxDereferenceSrvCall(SrvCall, *LockState);
                Status = STATUS_BAD_NETWORK_NAME;
                _SEH2_LEAVE;
            }

            /* Now, we want exclusive lock */
            if (*LockState == LHS_SharedLockHeld)
            {
                if (!RxAcquirePrefixTableLockExclusive(PrefixTable, FALSE))
                {
                    RxReleasePrefixTableLock(PrefixTable);
                    RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
                    *LockState = LHS_ExclusiveLockHeld;
                    goto RetryLookup;
                }

                RxReleasePrefixTableLock(PrefixTable);
                *LockState = LHS_ExclusiveLockHeld;
            }

            ASSERT(*LockState == LHS_ExclusiveLockHeld);

            /* If we reach that point, we found something, no need to create something */
            if (Container != NULL)
            {
                break;
            }

            /* Get the name for the SRV_CALL */
            RxExtractServerName(FilePathName, &SrvCallName, NULL);
            DPRINT(" -> SrvCallName: %wZ\n", &SrvCallName);
            /* And create the SRV_CALL */
            SrvCall = RxCreateSrvCall(RxContext, &SrvCallName, NULL, RxConnectionId);
            if (SrvCall == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                _SEH2_LEAVE;
            }

            /* Reset RX_CONTEXT, so far, connection creation isn't a success */
            RxReferenceSrvCall(SrvCall);
            RxContext->Create.pVNetRoot = NULL;
            RxContext->Create.pNetRoot = NULL;
            RxContext->Create.pSrvCall = NULL;
            RxContext->Create.Type = NetRootType;
            Container = SrvCall;

            /* Construct SRV_CALL, ie, use mini-rdr */
            Status = RxConstructSrvCall(RxContext, SrvCall, LockState);
            ASSERT(Status != STATUS_SUCCESS || RxIsPrefixTableLockAcquired(PrefixTable));
            if (Status != STATUS_SUCCESS)
            {
                DPRINT1("RxConstructSrvCall() = Status: %x\n", Status);
                RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
                RxDereferenceSrvCall(SrvCall, *LockState);
                RxReleasePrefixTableLock(PrefixTable);
                _SEH2_LEAVE;
            }

            /* Loop again to make use of SRV_CALL stable condition wait */
        }

        /* At that point, we have a stable SRV_CALL (either found or constructed) */
        ASSERT((NodeType(SrvCall) == RDBSS_NTC_SRVCALL) && (SrvCall->Condition == Condition_Good));
        ASSERT(NetRoot == NULL && VNetRoot == NULL);
        ASSERT(SrvCall->RxDeviceObject == RxContext->RxDeviceObject);

        /* Call mini-rdr to get NetRoot name */
        SrvCall->RxDeviceObject->Dispatch->MRxExtractNetRootName(FilePathName, (PMRX_SRV_CALL)SrvCall, &NetRootName, NULL);
        /* And create the NetRoot with that name */
        NetRoot = RxCreateNetRoot(SrvCall, &NetRootName, 0, RxConnectionId);
        if (NetRoot == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }
        NetRoot->Type = NetRootType;

        RxDereferenceSrvCall(SrvCall, *LockState);

        /* Finally, create the associated VNetRoot */
        VNetRoot = RxCreateVNetRoot(RxContext, NetRoot, CanonicalName, LocalNetRootName, FilePathName, RxConnectionId);
        if (VNetRoot == NULL)
        {
            RxFinalizeNetRoot(NetRoot, TRUE, TRUE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            _SEH2_LEAVE;
        }
        RxReferenceVNetRoot(VNetRoot);

        /* We're get closer! */
        NetRoot->Condition = Condition_InTransition;
        RxContext->Create.pSrvCall = (PMRX_SRV_CALL)SrvCall;
        RxContext->Create.pNetRoot = (PMRX_NET_ROOT)NetRoot;
        RxContext->Create.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;

        /* Construct the NetRoot, involving the mini-rdr now that we have our three control structs */
        Status = RxConstructNetRoot(RxContext, SrvCall, NetRoot, VNetRoot, LockState);
        if (!NT_SUCCESS(Status))
        {
            RxTransitionVNetRoot(VNetRoot, Condition_Bad);
            DPRINT1("RxConstructNetRoot failed Ctxt: %p, VNet: %p, Status: %lx, Condition: %d\n", RxContext, VNetRoot, Status, VNetRoot->Condition);
            RxDereferenceVNetRoot(VNetRoot, *LockState);

            RxContext->Create.pNetRoot = NULL;
            RxContext->Create.pVNetRoot = NULL;
        }
        else
        {
            PIO_STACK_LOCATION Stack;

            ASSERT(*LockState == LHS_ExclusiveLockHeld);

            Stack = RxContext->CurrentIrpSp;
            if (BooleanFlagOn(Stack->Parameters.Create.Options, FILE_CREATE_TREE_CONNECTION))
            {
                RxExclusivePrefixTableLockToShared(PrefixTable);
                *LockState = LHS_SharedLockHeld;
            }
        }
    }
    _SEH2_FINALLY
    {
        if (Status != STATUS_SUCCESS && Status != STATUS_CONNECTION_ACTIVE)
        {
            if (*LockState != LHS_LockNotHeld)
            {
                RxReleasePrefixTableLock(PrefixTable);
                *LockState = LHS_LockNotHeld;
            }
        }
    }
    _SEH2_END;

    DPRINT("RxFindOrCreateConnections() = Status: %x\n", Status);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RxFinishFcbInitialization(
    IN OUT PMRX_FCB Fcb,
    IN RX_FILE_TYPE FileType,
    IN PFCB_INIT_PACKET InitPacket OPTIONAL)
{
    RX_FILE_TYPE OldType;

    PAGED_CODE();

    DPRINT("RxFinishFcbInitialization(%p, %x, %p)\n", Fcb, FileType, InitPacket);

    OldType = NodeType(Fcb);
    NodeType(Fcb) = FileType;
    /* If mini-rdr already did the job for mailslot attributes, 0 the rest */
    if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_TIME_AND_SIZE_ALREADY_SET) && FileType == RDBSS_NTC_MAILSLOT)
    {
        FILL_IN_FCB((PFCB)Fcb, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }
    /* Otherwise, if mini-rdr provided us with an init packet, copy its data */
    else if (InitPacket != NULL)
    {
        FILL_IN_FCB((PFCB)Fcb, *InitPacket->pAttributes, *InitPacket->pNumLinks,
                    InitPacket->pCreationTime->QuadPart, InitPacket->pLastAccessTime->QuadPart,
                    InitPacket->pLastWriteTime->QuadPart, InitPacket->pLastChangeTime->QuadPart,
                    InitPacket->pAllocationSize->QuadPart, InitPacket->pFileSize->QuadPart,
                    InitPacket->pValidDataLength->QuadPart);
    }

    if (FileType != RDBSS_NTC_STORAGE_TYPE_UNKNOWN &&
        FileType != RDBSS_NTC_STORAGE_TYPE_DIRECTORY)
    {
        /* If our FCB newly points to a file, initiliaze everything related */
        if (FileType == RDBSS_NTC_STORAGE_TYPE_FILE)

        {
            if (OldType != RDBSS_NTC_STORAGE_TYPE_FILE)
            {
                RxInitializeLowIoPerFcbInfo(&((PFCB)Fcb)->Specific.Fcb.LowIoPerFcbInfo);
                FsRtlInitializeFileLock(&((PFCB)Fcb)->Specific.Fcb.FileLock, RxLockOperationCompletion,
                                        RxUnlockOperation);

                ((PFCB)Fcb)->BufferedLocks.List = NULL;
                ((PFCB)Fcb)->BufferedLocks.PendingLockOps = 0;

                Fcb->Header.IsFastIoPossible = FastIoIsQuestionable;
            }
        }
        /* If not a file, validate type */
        else
        {
            ASSERT(FileType >= RDBSS_NTC_SPOOLFILE && FileType <= RDBSS_NTC_MAILSLOT);
        }
    }
}

/*
 * @implemented
 */
NTSTATUS
RxFinishSrvCallConstruction(
    PMRX_SRVCALLDOWN_STRUCTURE Calldown)
{
    NTSTATUS Status;
    PSRV_CALL SrvCall;
    PRX_CONTEXT Context;
    RX_BLOCK_CONDITION Condition;
    PRX_PREFIX_TABLE PrefixTable;

    DPRINT("RxFinishSrvCallConstruction(%p)\n", Calldown);

    SrvCall = (PSRV_CALL)Calldown->SrvCall;
    Context = Calldown->RxContext;
    PrefixTable = Context->RxDeviceObject->pRxNetNameTable;

    /* We have a winner, notify him */
    if (Calldown->BestFinisher != NULL)
    {
        DPRINT("Notify the winner: %p (%wZ)\n", Calldown->BestFinisher, &Calldown->BestFinisher->DeviceName);

        ASSERT(SrvCall->RxDeviceObject == Calldown->BestFinisher);

        MINIRDR_CALL_THROUGH(Status, Calldown->BestFinisher->Dispatch,
                             MRxSrvCallWinnerNotify,
                             ((PMRX_SRV_CALL)SrvCall, TRUE,
                              Calldown->CallbackContexts[Calldown->BestFinisherOrdinal].RecommunicateContext));
        if (Status != STATUS_SUCCESS)
        {
            Condition = Condition_Bad;
        }
        else
        {
            Condition = Condition_Good;
        }
    }
    /* Otherwise, just fail our SRV_CALL */
    else
    {
        Status = Calldown->CallbackContexts[0].Status;
        Condition = Condition_Bad;
    }

    RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
    RxTransitionSrvCall(SrvCall, Condition);
    RxFreePoolWithTag(Calldown, RX_SRVCALL_POOLTAG);

    /* If async, finish it here, otherwise, caller has already finished the stuff */
    if (BooleanFlagOn(Context->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION))
    {
        DPRINT("Finishing async call\n");

        RxReleasePrefixTableLock(PrefixTable);

        /* Make sure we weren't cancelled in-between */
        if (BooleanFlagOn(Context->Flags, RX_CONTEXT_FLAG_CANCELLED))
        {
            Status = STATUS_CANCELLED;
        }

        /* In case that was a create, context can be reused */
        if (Context->MajorFunction == IRP_MJ_CREATE)
        {
            RxpPrepareCreateContextForReuse(Context);
        }

        /* If that's a failure, reset everything and return failure */
        if (Status != STATUS_SUCCESS)
        {
            Context->MajorFunction = Context->CurrentIrpSp->MajorFunction;
            if (Context->MajorFunction == IRP_MJ_DEVICE_CONTROL)
            {
                if (Context->Info.Buffer != NULL)
                {
                    RxFreePool(Context->Info.Buffer);
                    Context->Info.Buffer = NULL;
                }
            }
            Context->CurrentIrp->IoStatus.Information = 0;
            Context->CurrentIrp->IoStatus.Status = Status;
            RxCompleteRequest(Context, Status);
        }
        /* Otherwise, call resume routine and done! */
        else
        {
            Status = Context->ResumeRoutine(Context);
            if (Status != STATUS_PENDING)
            {
                RxCompleteRequest(Context, Status);
            }

            DPRINT("Not completing, pending\n");
        }
    }

    RxDereferenceSrvCall(SrvCall, LHS_LockNotHeld);
    return Status;
}

/*
 * @implemented
 */
VOID
NTAPI
RxFinishSrvCallConstructionDispatcher(
    IN PVOID Context)
{
    KIRQL OldIrql;
    BOOLEAN Direct, KeepLoop;

    DPRINT("RxFinishSrvCallConstructionDispatcher(%p)\n", Context);

    /* In case of failure of starting dispatcher, context is not set
     * We keep track of it to fail associated SRV_CALL
     */
    Direct = (Context == NULL);

    /* Separated thread, loop forever */
    while (TRUE)
    {
        PLIST_ENTRY ListEntry;
        PMRX_SRVCALLDOWN_STRUCTURE Calldown;

        /* If there are no SRV_CALL to finalize left, just finish thread */
        KeAcquireSpinLock(&RxStrucSupSpinLock, &OldIrql);
        if (IsListEmpty(&RxSrvCalldownList))
        {
            KeepLoop =  FALSE;
            RxSrvCallConstructionDispatcherActive = FALSE;
        }
        /* Otherwise, get the SRV_CALL to finish construction */
        else
        {
            ListEntry = RemoveHeadList(&RxSrvCalldownList);
            KeepLoop =  TRUE;
        }
        KeReleaseSpinLock(&RxStrucSupSpinLock, OldIrql);

        /* Nothing to do */
        if (!KeepLoop)
        {
            break;
        }

        /* If direct is set, reset the finisher to avoid electing a winner
         * and fail SRV_CALL (see upper comment)
         */
        Calldown = CONTAINING_RECORD(ListEntry, MRX_SRVCALLDOWN_STRUCTURE, SrvCalldownList);
        if (Direct)
        {
            Calldown->BestFinisher = NULL;
        }
        /* Finish SRV_CALL construction */
        RxFinishSrvCallConstruction(Calldown);
    }
}

/*
 * @implemented
 */
NTSTATUS
RxFlushFcbInSystemCache(
    IN PFCB Fcb,
    IN BOOLEAN SynchronizeWithLazyWriter)
{
    IO_STATUS_BLOCK IoStatus;

    PAGED_CODE();

    /* Deal with Cc */
    CcFlushCache(&Fcb->NonPaged->SectionObjectPointers, NULL, 0, &IoStatus);
    /* If we're asked to sync with LW, do it in case of success */
    if (SynchronizeWithLazyWriter && NT_SUCCESS(IoStatus.Status))
    {
        RxAcquirePagingIoResource((PRX_CONTEXT)NULL, Fcb);
        RxReleasePagingIoResource((PRX_CONTEXT)NULL, Fcb);
    }

    DPRINT("Flushing for FCB %p returns %lx\n", Fcb, IoStatus.Status);
    return IoStatus.Status;
}

/*
 * @implemented
 */
VOID
RxFreeFcbObject(
    PVOID Object)
{
    PAGED_CODE();

    DPRINT("Freeing %p\n", Object);

    /* If that's a FOBX/SRV_OPEN, nothing to do, just free it */
    if (NodeType(Object) == RDBSS_NTC_FOBX || NodeType(Object) == RDBSS_NTC_SRVOPEN)
    {
        RxFreePoolWithTag(Object, RX_FCB_POOLTAG);
    }
    /* If that's a FCB... */
    else if (NodeTypeIsFcb(Object))
    {
        PFCB Fcb;
        PRDBSS_DEVICE_OBJECT DeviceObject;

        Fcb = (PFCB)Object;
        DeviceObject = Fcb->RxDeviceObject;

        /* Delete per stream contexts */
        FsRtlTeardownPerStreamContexts(&Fcb->Header);

        SetFlag(Fcb->Header.Flags, FSRTL_FLAG_ACQUIRE_MAIN_RSRC_SH);

        /* If there was a non-paged FCB allocated, free it */
        if (!BooleanFlagOn(Fcb->FcbState, FCB_STATE_PAGING_FILE))
        {
            RxFreePoolWithTag(Fcb->NonPaged, RX_NONPAGEDFCB_POOLTAG);
        }

        /* Free the FCB */
        RxFreePool(Fcb);

        /* Update statistics */
        InterlockedDecrement(&RxNumberOfActiveFcbs);
        InterlockedDecrement((volatile long *)&DeviceObject->NumberOfActiveFcbs);
    }
}

/*
 * @implemented
 */
VOID
RxFreeObject(
    PVOID pObject)
{
    PAGED_CODE();

    /* First, perform a few sanity checks if we're dealing with a SRV_CALL or a NET_ROOT */
    if (NodeType(pObject) == RDBSS_NTC_SRVCALL)
    {
        PSRV_CALL SrvCall;
        PRDBSS_DEVICE_OBJECT DeviceObject;

        SrvCall = (PSRV_CALL)pObject;
        DeviceObject = SrvCall->RxDeviceObject;
        if (DeviceObject != NULL)
        {
            if (!BooleanFlagOn(DeviceObject->Dispatch->MRxFlags, RDBSS_MANAGE_SRV_CALL_EXTENSION))
            {
                ASSERT(SrvCall->Context == NULL);
            }

            ASSERT(SrvCall->Context2 == NULL);

            SrvCall->RxDeviceObject = NULL;
        }
    }
    else if (NodeType(pObject) == RDBSS_NTC_NETROOT)
    {
        PNET_ROOT NetRoot;

        NetRoot = (PNET_ROOT)pObject;
        NetRoot->pSrvCall = NULL;
        NetRoot->NodeTypeCode = NodeType(pObject) | 0xF000;
    }

    /* And just free the object */
    RxFreePool(pObject);
}

/*
 * @implemented
 */
VOID
RxGatherRequestsForSrvOpen(
    IN OUT PSRV_CALL SrvCall,
    IN PSRV_OPEN SrvOpen,
    IN OUT PLIST_ENTRY RequestsListHead)
{
    KIRQL OldIrql;
    LIST_ENTRY Discarded, *Entry;
    PCHANGE_BUFFERING_STATE_REQUEST Request;

    /* Dispatch any pending operation first */
    RxpDispatchChangeBufferingStateRequests(SrvCall, SrvOpen, &Discarded);

    /* Then, get any entry related to our key and SRV_OPEN */
    KeAcquireSpinLock(&SrvCall->BufferingManager.SpinLock, &OldIrql);
    Entry = SrvCall->BufferingManager.HandlerList.Flink;
    while (Entry != &SrvCall->BufferingManager.HandlerList)
    {
        Request = CONTAINING_RECORD(Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry);
        Entry = Entry->Flink;
        if (Request->SrvOpenKey == SrvOpen->Key && Request->SrvOpen == SrvOpen)
        {
            RemoveEntryList(&Request->ListEntry);
            InsertTailList(RequestsListHead, &Request->ListEntry);
        }
    }
    KeReleaseSpinLock(&SrvCall->BufferingManager.SpinLock, OldIrql);

    /* Perform the same search in the last change list */
    Entry = SrvCall->BufferingManager.LastChanceHandlerList.Flink;
    while (Entry != &SrvCall->BufferingManager.LastChanceHandlerList)
    {
        Request = CONTAINING_RECORD(Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry);
        Entry = Entry->Flink;
        if (Request->SrvOpenKey == SrvOpen->Key && Request->SrvOpen == SrvOpen)
        {
            RemoveEntryList(&Request->ListEntry);
            InsertTailList(RequestsListHead, &Request->ListEntry);
        }
    }

    /* Discard the discarded requests */
    RxpDiscardChangeBufferingStateRequests(&Discarded);
}

/*
 * @implemented
 */
PRDBSS_DEVICE_OBJECT
RxGetDeviceObjectOfInstance(
    PVOID Instance)
{
    NODE_TYPE_CODE NodeType;
    PRDBSS_DEVICE_OBJECT DeviceObject;

    PAGED_CODE();

    /* We only handle a few object types */
    NodeType = NodeType(Instance);
    ASSERT((NodeType == RDBSS_NTC_SRVCALL) || (NodeType == RDBSS_NTC_NETROOT) ||
           (NodeType == RDBSS_NTC_V_NETROOT) || (NodeType == RDBSS_NTC_SRVOPEN) || (NodeType == RDBSS_NTC_FOBX));

    /* Get the device object depending on the object */
    switch (NodeType)
    {
        case RDBSS_NTC_FOBX:
        {
            PFOBX Fobx;

            Fobx = (PFOBX)Instance;
            DeviceObject = Fobx->RxDeviceObject;
            break;
        }

        case RDBSS_NTC_SRVCALL:
        {
            PSRV_CALL SrvCall;

            SrvCall = (PSRV_CALL)Instance;
            DeviceObject = SrvCall->RxDeviceObject;
            break;
        }

        case RDBSS_NTC_NETROOT:
        {
            PNET_ROOT NetRoot;

            NetRoot = (PNET_ROOT)Instance;
            DeviceObject = NetRoot->pSrvCall->RxDeviceObject;
            break;
        }

        case RDBSS_NTC_V_NETROOT:
        {
            PV_NET_ROOT VNetRoot;

            VNetRoot = (PV_NET_ROOT)Instance;
            DeviceObject = VNetRoot->pNetRoot->pSrvCall->RxDeviceObject;
            break;
        }

        case RDBSS_NTC_SRVOPEN:
        {
            PSRV_OPEN SrvOpen;

            SrvOpen = (PSRV_OPEN)Instance;
            DeviceObject = ((PFCB)SrvOpen->pFcb)->RxDeviceObject;
            break;
        }

        default:
            DeviceObject = NULL;
            break;
    }

    /* Job done */
    return DeviceObject;
}

/*
 * @implemented
 */
VOID
RxGetFileSizeWithLock(
    IN PFCB Fcb,
    OUT PLONGLONG FileSize)
{
    PAGED_CODE();

    *FileSize = Fcb->Header.FileSize.QuadPart;
}

/*
 * @implemented
 */
PEPROCESS
NTAPI
RxGetRDBSSProcess(
    VOID)
{
    return RxData.OurProcess;
}

/*
 * @implemented
 */
NTSTATUS
RxInitializeBufferingManager(
   PSRV_CALL SrvCall)
{
    KeInitializeSpinLock(&SrvCall->BufferingManager.SpinLock);
    InitializeListHead(&SrvCall->BufferingManager.DispatcherList);
    InitializeListHead(&SrvCall->BufferingManager.HandlerList);
    InitializeListHead(&SrvCall->BufferingManager.LastChanceHandlerList);
    SrvCall->BufferingManager.DispatcherActive = FALSE;
    SrvCall->BufferingManager.HandlerInactive = FALSE;
    SrvCall->BufferingManager.LastChanceHandlerActive = FALSE;
    SrvCall->BufferingManager.NumberOfOutstandingOpens = 0;
    InitializeListHead(&SrvCall->BufferingManager.SrvOpenLists[0]);
    ExInitializeFastMutex(&SrvCall->BufferingManager.Mutex);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
RxInitializeContext(
    IN PIRP Irp,
    IN PRDBSS_DEVICE_OBJECT RxDeviceObject,
    IN ULONG InitialContextFlags,
    IN OUT PRX_CONTEXT RxContext)
{
    PIO_STACK_LOCATION Stack;

    /* Initialize our various fields */
    RxContext->NodeTypeCode = RDBSS_NTC_RX_CONTEXT;
    RxContext->NodeByteSize = sizeof(RX_CONTEXT);
    RxContext->ReferenceCount = 1;
    RxContext->SerialNumber = InterlockedExchangeAdd((volatile LONG *)&RxContextSerialNumberCounter, 1);
    RxContext->RxDeviceObject = RxDeviceObject;
    KeInitializeEvent(&RxContext->SyncEvent, SynchronizationEvent, FALSE);
    RxInitializeScavengerEntry(&RxContext->ScavengerEntry);
    InitializeListHead(&RxContext->BlockedOperations);
    RxContext->MRxCancelRoutine = NULL;
    RxContext->ResumeRoutine = NULL;
    RxContext->Flags |= InitialContextFlags;
    RxContext->CurrentIrp = Irp;
    RxContext->LastExecutionThread = PsGetCurrentThread();
    RxContext->OriginalThread = RxContext->LastExecutionThread;

    /* If've got no IRP, mark RX_CONTEXT */
    if (Irp == NULL)
    {
        RxContext->CurrentIrpSp = NULL;
        RxContext->MajorFunction = IRP_MJ_MAXIMUM_FUNCTION + 1;
        RxContext->MinorFunction = 0;
    }
    else
    {
        /* Otherwise, first determine whether we are performing async operation */
        Stack = IoGetCurrentIrpStackLocation(Irp);
        if (Stack->FileObject != NULL)
        {
            PFCB Fcb;

            Fcb = Stack->FileObject->FsContext;
            if (!IoIsOperationSynchronous(Irp) ||
                ((Fcb != NULL && NodeTypeIsFcb(Fcb)) &&
                 (Stack->MajorFunction == IRP_MJ_READ || Stack->MajorFunction == IRP_MJ_WRITE || Stack->MajorFunction == IRP_MJ_FILE_SYSTEM_CONTROL) &&
                 (Fcb->pNetRoot != NULL && (Fcb->pNetRoot->Type == NET_ROOT_PIPE))))
            {
                RxContext->Flags |= RX_CONTEXT_FLAG_ASYNC_OPERATION;
            }
        }

        if (Stack->MajorFunction == IRP_MJ_DIRECTORY_CONTROL && Stack->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY)
        {
            RxContext->Flags |= RX_CONTEXT_FLAG_ASYNC_OPERATION;
        }
        if (Stack->MajorFunction == IRP_MJ_DEVICE_CONTROL)
        {
            RxContext->Flags |= RX_CONTEXT_FLAG_ASYNC_OPERATION;
        }

        /* Set proper flags if TopLevl IRP/Device */
        if (!RxIsThisTheTopLevelIrp(Irp))
        {
            RxContext->Flags |= RX_CONTEXT_FLAG_RECURSIVE_CALL;
        }
        if (RxGetTopDeviceObjectIfRdbssIrp() == RxDeviceObject)
        {
            RxContext->Flags |= RX_CONTEXT_FLAG_THIS_DEVICE_TOP_LEVEL;
        }

        /* Copy stack information */
        RxContext->MajorFunction = Stack->MajorFunction;
        RxContext->MinorFunction = Stack->MinorFunction;
        ASSERT(RxContext->MajorFunction <= IRP_MJ_MAXIMUM_FUNCTION);
        RxContext->CurrentIrpSp = Stack;

        /* If we have a FO associated, learn for more */
        if (Stack->FileObject != NULL)
        {
            PFCB Fcb;
            PFOBX Fobx;

            /* Get the FCB and CCB (FOBX) */
            Fcb = Stack->FileObject->FsContext;
            Fobx = Stack->FileObject->FsContext2;
            RxContext->pFcb = (PMRX_FCB)Fcb;
            if (Fcb != NULL && NodeTypeIsFcb(Fcb))
            {
                RxContext->NonPagedFcb = Fcb->NonPaged;
            }

            /* We have a FOBX, this not a DFS opening, keep track of it */
            if (Fobx != NULL && Fobx != UIntToPtr(DFS_OPEN_CONTEXT) && Fobx != UIntToPtr(DFS_DOWNLEVEL_OPEN_CONTEXT))
            {
                RxContext->pFobx = (PMRX_FOBX)Fobx;
                RxContext->pRelevantSrvOpen = Fobx->pSrvOpen;
                if (Fobx->NodeTypeCode == RDBSS_NTC_FOBX)
                {
                    RxContext->FobxSerialNumber = InterlockedIncrement((volatile LONG *)&Fobx->FobxSerialNumber);
                }
            }
            else
            {
                RxContext->pFobx = NULL;
            }

            /* In case of directory change notification, Fobx may be a VNetRoot, take note of that */
            if (RxContext->MajorFunction == IRP_MJ_DIRECTORY_CONTROL && RxContext->MinorFunction == IRP_MN_NOTIFY_CHANGE_DIRECTORY &&
                Fobx != NULL)
            {
                PV_NET_ROOT VNetRoot = NULL;

                if (Fobx->NodeTypeCode == RDBSS_NTC_FOBX)
                {
                    VNetRoot = Fcb->VNetRoot;
                }
                else if (Fobx->NodeTypeCode == RDBSS_NTC_V_NETROOT)
                {
                    VNetRoot = (PV_NET_ROOT)Fobx;
                }

                if (VNetRoot != NULL)
                {
                    RxContext->NotifyChangeDirectory.pVNetRoot = (PMRX_V_NET_ROOT)VNetRoot;
                }
            }

            /* Remember if that's a write through file */
            RxContext->RealDevice = Stack->FileObject->DeviceObject;
            if (BooleanFlagOn(Stack->FileObject->Flags, FO_WRITE_THROUGH))
            {
                RxContext->Flags |= RX_CONTEXT_FLAG_WRITE_THROUGH;
            }
        }
    }

    if (RxContext->MajorFunction != IRP_MJ_DEVICE_CONTROL)
    {
        DPRINT("New Ctxt: %p for MN: %d, IRP: %p, THRD: %p, FCB: %p, FOBX:%p #%lx\n",
               RxContext, RxContext->MinorFunction, Irp,
               PsGetCurrentThread(), RxContext->pFcb, RxContext->pFobx,
               RxContext->SerialNumber);
    }
}

/*
 * @implemented
 */
VOID
NTAPI
RxInitializeDebugSupport(
    VOID)
{
    /* Nothing to do */
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxInitializeDispatcher(
    VOID)
{
    NTSTATUS Status;
    HANDLE ThreadHandle;

    PAGED_CODE();

    RxFileSystemDeviceObject->DispatcherContext.NumberOfWorkerThreads = 0;
    RxFileSystemDeviceObject->DispatcherContext.pTearDownEvent = NULL;

    /* Set appropriate timeouts: 10s & 60s */
    RxWorkQueueWaitInterval[CriticalWorkQueue].QuadPart = -10 * 1000 * 1000 * 10;
    RxWorkQueueWaitInterval[DelayedWorkQueue].QuadPart = -10 * 1000 * 1000 * 10;
    RxWorkQueueWaitInterval[HyperCriticalWorkQueue].QuadPart = -10 * 1000 * 1000 * 10;
    RxSpinUpDispatcherWaitInterval.QuadPart = -60 * 1000 * 1000 * 10;

    RxDispatcher.NumberOfProcessors = 1;
    RxDispatcher.OwnerProcess = IoGetCurrentProcess();
    RxDispatcher.pWorkQueueDispatcher = &RxDispatcherWorkQueues;

    /* Initialize our dispatchers */
    Status = RxInitializeWorkQueueDispatcher(RxDispatcher.pWorkQueueDispatcher);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RxInitializeMRxDispatcher(RxFileSystemDeviceObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* And start them */
    RxDispatcher.State = RxDispatcherActive;
    InitializeListHead(&RxDispatcher.SpinUpRequests);
    KeInitializeSpinLock(&RxDispatcher.SpinUpRequestsLock);
    KeInitializeEvent(&RxDispatcher.SpinUpRequestsEvent, 0, 0);
    KeInitializeEvent(&RxDispatcher.SpinUpRequestsTearDownEvent, 0, 0);
    Status = PsCreateSystemThread(&ThreadHandle, PROCESS_ALL_ACCESS, NULL,
                                  NULL, NULL, RxSpinUpRequestsDispatcher, &RxDispatcher);
    if (NT_SUCCESS(Status))
    {
        ZwClose(ThreadHandle);
    }

    return Status;
}

/*
 * @implemented
 */
VOID
RxInitializeFcbTable(
    IN OUT PRX_FCB_TABLE FcbTable,
    IN BOOLEAN CaseInsensitiveMatch)
{
    USHORT i;

    PAGED_CODE();

    FcbTable->NodeTypeCode = RDBSS_NTC_FCB_TABLE;
    FcbTable->NodeByteSize = sizeof(RX_FCB_TABLE);

    ExInitializeResourceLite(&FcbTable->TableLock);
    FcbTable->CaseInsensitiveMatch = CaseInsensitiveMatch;
    FcbTable->Version = 0;
    FcbTable->TableEntryForNull = NULL;

    FcbTable->NumberOfBuckets = RX_FCB_TABLE_NUMBER_OF_HASH_BUCKETS;
    for (i = 0; i < FcbTable->NumberOfBuckets; ++i)
    {
        InitializeListHead(&FcbTable->HashBuckets[i]);
    }

    FcbTable->Lookups = 0;
    FcbTable->FailedLookups = 0;
    FcbTable->Compares = 0;
}

/*
 * @implemented
 */
VOID
NTAPI
RxInitializeLowIoContext(
    OUT PLOWIO_CONTEXT LowIoContext,
    IN ULONG Operation)
{
    PRX_CONTEXT RxContext;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    RxContext = CONTAINING_RECORD(LowIoContext, RX_CONTEXT, LowIoContext);
    ASSERT(LowIoContext == &RxContext->LowIoContext);

    Stack = RxContext->CurrentIrpSp;

    KeInitializeEvent(&RxContext->SyncEvent, NotificationEvent, FALSE);
    RxContext->LowIoContext.ResourceThreadId = (ERESOURCE_THREAD)PsGetCurrentThread();
    RxContext->LowIoContext.Operation = Operation;

    switch (Operation)
    {
        case LOWIO_OP_READ:
        case LOWIO_OP_WRITE:
            /* In case of RW, set a canary, to make sure these fields are properly set
             * they will be asserted when lowio request will be submit to mini-rdr
             * See LowIoSubmit()
             */
            RxContext->LowIoContext.ParamsFor.ReadWrite.ByteOffset = 0xFFFFFFEE;
            RxContext->LowIoContext.ParamsFor.ReadWrite.ByteCount = 0xEEEEEEEE;
            RxContext->LowIoContext.ParamsFor.ReadWrite.Key = Stack->Parameters.Read.Key;

            /* Keep track of paging IOs */
            if (BooleanFlagOn(RxContext->CurrentIrp->Flags, IRP_PAGING_IO))
            {
                RxContext->LowIoContext.ParamsFor.ReadWrite.Flags = LOWIO_READWRITEFLAG_PAGING_IO;
            }
            else
            {
                RxContext->LowIoContext.ParamsFor.ReadWrite.Flags = 0;
            }

            break;

        case LOWIO_OP_FSCTL:
        case LOWIO_OP_IOCTL:
            /* This will be initialized later on with a call to RxLowIoPopulateFsctlInfo() */
            RxContext->LowIoContext.ParamsFor.FsCtl.Flags = 0;
            RxContext->LowIoContext.ParamsFor.FsCtl.InputBufferLength = 0;
            RxContext->LowIoContext.ParamsFor.FsCtl.pInputBuffer = NULL;
            RxContext->LowIoContext.ParamsFor.FsCtl.OutputBufferLength = 0;
            RxContext->LowIoContext.ParamsFor.FsCtl.pOutputBuffer = NULL;
            RxContext->LowIoContext.ParamsFor.FsCtl.MinorFunction = 0;
            break;

        /* Nothing to do for these */
        case LOWIO_OP_SHAREDLOCK:
        case LOWIO_OP_EXCLUSIVELOCK:
        case LOWIO_OP_UNLOCK:
        case LOWIO_OP_UNLOCK_MULTIPLE:
        case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
        case LOWIO_OP_CLEAROUT:
            break;

        default:
            /* Should never happen */
            ASSERT(FALSE);
            break;
    }
}

/*
 * @implemented
 */
VOID
RxInitializeLowIoPerFcbInfo(
    PLOWIO_PER_FCB_INFO LowIoPerFcbInfo)
{
    PAGED_CODE();

    InitializeListHead(&LowIoPerFcbInfo->PagingIoReadsOutstanding);
    InitializeListHead(&LowIoPerFcbInfo->PagingIoWritesOutstanding);
}

/*
 * @implemented
 */
NTSTATUS
RxInitializeMRxDispatcher(
     IN OUT PRDBSS_DEVICE_OBJECT pMRxDeviceObject)
{
    PAGED_CODE();

    pMRxDeviceObject->DispatcherContext.NumberOfWorkerThreads = 0;
    pMRxDeviceObject->DispatcherContext.pTearDownEvent = NULL;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
RxInitializePrefixTable(
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN ULONG TableSize OPTIONAL,
    IN BOOLEAN CaseInsensitiveMatch)
{
    PAGED_CODE();

    if (TableSize == 0)
    {
        TableSize = RX_PREFIX_TABLE_DEFAULT_LENGTH;
    }

    ThisTable->NodeTypeCode = RDBSS_NTC_PREFIX_TABLE;
    ThisTable->NodeByteSize = sizeof(RX_PREFIX_TABLE);
    InitializeListHead(&ThisTable->MemberQueue);
    ThisTable->Version = 0;
    ThisTable->TableEntryForNull = NULL;
    ThisTable->IsNetNameTable = FALSE;
    ThisTable->CaseInsensitiveMatch = CaseInsensitiveMatch;
    ThisTable->TableSize = TableSize;

    if (TableSize > 0)
    {
        USHORT i;

        for (i = 0; i < RX_PREFIX_TABLE_DEFAULT_LENGTH; ++i)
        {
            InitializeListHead(&ThisTable->HashBuckets[i]);
        }
    }
}

/*
 * @implemented
 */
VOID
RxInitializePurgeSyncronizationContext(
    PPURGE_SYNCHRONIZATION_CONTEXT PurgeSyncronizationContext)
{
    PAGED_CODE();

    InitializeListHead(&PurgeSyncronizationContext->ContextsAwaitingPurgeCompletion);
    PurgeSyncronizationContext->PurgeInProgress = FALSE;
}

NTSTATUS
RxInitializeSrvCallParameters(
    IN PRX_CONTEXT RxContext,
    IN OUT PSRV_CALL SrvCall)
{
    PAGED_CODE();

    SrvCall->pPrincipalName = NULL;

    /* We only have stuff to initialize for file opening from DFS */
    if (RxContext->MajorFunction != IRP_MJ_CREATE || RxContext->Create.EaLength == 0)
    {
        return STATUS_SUCCESS;
    }

    ASSERT(RxContext->Create.EaBuffer != NULL);

    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxInitializeRxTimer(
    VOID)
{
    PAGED_CODE();

    RxTimerInterval.QuadPart = -550000;
    KeInitializeSpinLock(&RxTimerLock);
    InitializeListHead(&RxTimerQueueHead);
    InitializeListHead(&RxRecurrentWorkItemsList);
    KeInitializeDpc(&RxTimerDpc, RxTimerDispatch, NULL);
    KeInitializeTimer(&RxTimer);
    RxTimerTickCount = 0;

    return STATUS_SUCCESS;
}

NTSTATUS
RxInitializeVNetRootParameters(
   PRX_CONTEXT RxContext,
   OUT LUID *LogonId,
   OUT PULONG SessionId,
   OUT PUNICODE_STRING *UserNamePtr,
   OUT PUNICODE_STRING *UserDomainNamePtr,
   OUT PUNICODE_STRING *PasswordPtr,
   OUT PULONG Flags)
{
    NTSTATUS Status;
    PACCESS_TOKEN Token;

    PAGED_CODE();

    DPRINT("RxInitializeVNetRootParameters(%p, %p, %p, %p, %p, %p, %p)\n", RxContext,
           LogonId, SessionId, UserNamePtr, UserDomainNamePtr, PasswordPtr, Flags);

    *UserNamePtr = NULL;
    *UserDomainNamePtr = NULL;
    *PasswordPtr = NULL;
    /* By default, that's not CSC instance */
    *Flags &= ~VNETROOT_FLAG_CSCAGENT_INSTANCE;

    Token = SeQuerySubjectContextToken(&RxContext->Create.NtCreateParameters.SecurityContext->AccessState->SubjectSecurityContext);
    if (SeTokenIsRestricted(Token))
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Get LogonId */
    Status = SeQueryAuthenticationIdToken(Token, LogonId);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* And SessionId */
    Status = SeQuerySessionIdToken(Token, SessionId);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (RxContext->Create.UserName.Buffer != NULL)
    {
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
        goto Leave;
    }

    /* Deal with connection credentials */
    if (RxContext->Create.UserDomainName.Buffer != NULL)
    {
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
        goto Leave;
    }

    if (RxContext->Create.Password.Buffer != NULL)
    {
        UNIMPLEMENTED;
        Status = STATUS_NOT_IMPLEMENTED;
        goto Leave;
    }

Leave:
    if (NT_SUCCESS(Status))
    {
        /* If that's a CSC instance, mark it as such */
        if (RxIsThisACscAgentOpen(RxContext))
        {
            *Flags |= VNETROOT_FLAG_CSCAGENT_INSTANCE;
        }
        return Status;
    }

    return Status;
}

/*
 * @implemented
 */
VOID
RxInitializeWorkQueue(
   PRX_WORK_QUEUE WorkQueue,
   WORK_QUEUE_TYPE WorkQueueType,
   ULONG MaximumNumberOfWorkerThreads,
   ULONG MinimumNumberOfWorkerThreads)
{
    PAGED_CODE();

    WorkQueue->Type = WorkQueueType;
    WorkQueue->MaximumNumberOfWorkerThreads = MaximumNumberOfWorkerThreads;
    WorkQueue->MinimumNumberOfWorkerThreads = MinimumNumberOfWorkerThreads;

    WorkQueue->State = RxWorkQueueActive;
    WorkQueue->SpinUpRequestPending = FALSE;
    WorkQueue->pRundownContext = NULL;
    WorkQueue->NumberOfWorkItemsDispatched = 0;
    WorkQueue->NumberOfWorkItemsToBeDispatched = 0;
    WorkQueue->CumulativeQueueLength = 0;
    WorkQueue->NumberOfSpinUpRequests = 0;
    WorkQueue->NumberOfActiveWorkerThreads = 0;
    WorkQueue->NumberOfIdleWorkerThreads = 0;
    WorkQueue->NumberOfFailedSpinUpRequests = 0;
    WorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse = 0;
    WorkQueue->WorkQueueItemForTearDownWorkQueue.List.Flink = NULL;
    WorkQueue->WorkQueueItemForTearDownWorkQueue.WorkerRoutine = NULL;
    WorkQueue->WorkQueueItemForTearDownWorkQueue.Parameter = NULL;
    WorkQueue->WorkQueueItemForTearDownWorkQueue.pDeviceObject = NULL;
    WorkQueue->WorkQueueItemForSpinUpWorkerThread.List.Flink = NULL;
    WorkQueue->WorkQueueItemForSpinUpWorkerThread.WorkerRoutine = NULL;
    WorkQueue->WorkQueueItemForSpinUpWorkerThread.Parameter = NULL;
    WorkQueue->WorkQueueItemForSpinUpWorkerThread.pDeviceObject = NULL;
    WorkQueue->WorkQueueItemForSpinDownWorkerThread.List.Flink = NULL;
    WorkQueue->WorkQueueItemForSpinDownWorkerThread.WorkerRoutine = NULL;
    WorkQueue->WorkQueueItemForSpinDownWorkerThread.Parameter = NULL;
    WorkQueue->WorkQueueItemForSpinDownWorkerThread.pDeviceObject = NULL;

    KeInitializeQueue(&WorkQueue->Queue, MaximumNumberOfWorkerThreads);
    KeInitializeSpinLock(&WorkQueue->SpinLock);
}

/*
 * @implemented
 */
NTSTATUS
RxInitializeWorkQueueDispatcher(
   PRX_WORK_QUEUE_DISPATCHER Dispatcher)
{
    NTSTATUS Status;
    ULONG MaximumNumberOfWorkerThreads;

    PAGED_CODE();

    /* Number of threads will depend on system capacity */
    if (MmQuerySystemSize() != MmLargeSystem)
    {
        MaximumNumberOfWorkerThreads = 5;
    }
    else
    {
        MaximumNumberOfWorkerThreads = 10;
    }

    /* Initialize the work queues */
    RxInitializeWorkQueue(&Dispatcher->WorkQueue[CriticalWorkQueue], CriticalWorkQueue,
                          MaximumNumberOfWorkerThreads, 1);
    RxInitializeWorkQueue(&Dispatcher->WorkQueue[DelayedWorkQueue], DelayedWorkQueue, 2, 1);
    RxInitializeWorkQueue(&Dispatcher->WorkQueue[HyperCriticalWorkQueue], HyperCriticalWorkQueue, 5, 1);

    /* And start the worker threads */
    Status = RxSpinUpWorkerThread(&Dispatcher->WorkQueue[HyperCriticalWorkQueue],
                                  RxBootstrapWorkerThreadDispatcher,
                                  &Dispatcher->WorkQueue[HyperCriticalWorkQueue]);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RxSpinUpWorkerThread(&Dispatcher->WorkQueue[CriticalWorkQueue],
                                  RxBootstrapWorkerThreadDispatcher,
                                  &Dispatcher->WorkQueue[CriticalWorkQueue]);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RxSpinUpWorkerThread(&Dispatcher->WorkQueue[DelayedWorkQueue],
                                  RxBootstrapWorkerThreadDispatcher,
                                  &Dispatcher->WorkQueue[DelayedWorkQueue]);
    return Status;
}

/*
 * @implemented
 */
VOID
RxInitiateSrvOpenKeyAssociation(
   IN OUT PSRV_OPEN SrvOpen)
{
    PRX_BUFFERING_MANAGER BufferingManager;

    PAGED_CODE();

    SrvOpen->Key = NULL;

    /* Just keep track of the opening request */
    BufferingManager = &((PSRV_CALL)((PFCB)SrvOpen->pFcb)->VNetRoot->pNetRoot->pSrvCall)->BufferingManager;
    InterlockedIncrement(&BufferingManager->NumberOfOutstandingOpens);

    InitializeListHead(&SrvOpen->SrvOpenKeyList);
}

/*
 * @implemented
 */
NTSTATUS
RxInsertWorkQueueItem(
    PRDBSS_DEVICE_OBJECT pMRxDeviceObject,
    WORK_QUEUE_TYPE WorkQueueType,
    PRX_WORK_QUEUE_ITEM WorkQueueItem)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    BOOLEAN SpinUpThreads;
    PRX_WORK_QUEUE WorkQueue;

    /* No dispatcher, nothing to insert */
    if (RxDispatcher.State != RxDispatcherActive)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Get the work queue */
    WorkQueue = &RxDispatcher.pWorkQueueDispatcher->WorkQueue[WorkQueueType];

    KeAcquireSpinLock(&WorkQueue->SpinLock, &OldIrql);
    /* Only insert if the work queue is in decent state */
    if (WorkQueue->State != RxWorkQueueActive || pMRxDeviceObject->DispatcherContext.pTearDownEvent != NULL)
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        SpinUpThreads = FALSE;
        WorkQueueItem->pDeviceObject = pMRxDeviceObject;
        InterlockedIncrement(&pMRxDeviceObject->DispatcherContext.NumberOfWorkerThreads);
        WorkQueue->CumulativeQueueLength += WorkQueue->NumberOfWorkItemsToBeDispatched;
        InterlockedIncrement(&WorkQueue->NumberOfWorkItemsToBeDispatched);

        /* If required (and possible!), spin up a new worker thread */
        if (WorkQueue->NumberOfIdleWorkerThreads < WorkQueue->NumberOfWorkItemsToBeDispatched &&
            WorkQueue->NumberOfActiveWorkerThreads < WorkQueue->MaximumNumberOfWorkerThreads &&
            !WorkQueue->SpinUpRequestPending)
        {
            WorkQueue->SpinUpRequestPending = TRUE;
            SpinUpThreads = TRUE;
        }

        Status = STATUS_SUCCESS;
    }
    KeReleaseSpinLock(&WorkQueue->SpinLock, OldIrql);

    /* If we failed, return and still not insert item */
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* All fine, insert the item */
    KeInsertQueue(&WorkQueue->Queue, &WorkQueueItem->List);

    /* And start a new worker thread if needed */
    if (SpinUpThreads)
    {
        RxSpinUpWorkerThreads(WorkQueue);
    }

    return Status;
}

BOOLEAN
RxIsThisACscAgentOpen(
    IN PRX_CONTEXT RxContext)
{
    BOOLEAN CscAgent;

    CscAgent = FALSE;

    /* Client Side Caching is DFS stuff - we don't support it */
    if (RxContext->Create.EaLength != 0)
    {
        UNIMPLEMENTED;
    }

    if (RxContext->Create.NtCreateParameters.DfsNameContext != NULL &&
        ((PDFS_NAME_CONTEXT)RxContext->Create.NtCreateParameters.DfsNameContext)->NameContextType == 0xAAAAAAAA)
    {
        CscAgent = TRUE;
    }

    return CscAgent;
}

VOID
RxLockUserBuffer(
    IN PRX_CONTEXT RxContext,
    IN LOCK_OPERATION Operation,
    IN ULONG BufferLength)
{
    PIRP Irp;
    PMDL Mdl = NULL;

    PAGED_CODE();

    _SEH2_TRY
    {
        Irp = RxContext->CurrentIrp;
        /* If we already have a MDL, make sure it's locked */
        if (Irp->MdlAddress != NULL)
        {
            ASSERT(RxLowIoIsMdlLocked(Irp->MdlAddress));
        }
        else
        {
            /* That likely means the driver asks for buffered IOs - we don't support it! */
            ASSERT(!BooleanFlagOn(Irp->Flags, IRP_INPUT_OPERATION));

            /* If we have a real length */
            if (BufferLength > 0)
            {
                /* Allocate a MDL and lock it */
                Mdl = IoAllocateMdl(Irp->UserBuffer, BufferLength, FALSE, FALSE, Irp);
                if (Mdl == NULL)
                {
                    RxContext->StoredStatus = STATUS_INSUFFICIENT_RESOURCES;
                    ExRaiseStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                MmProbeAndLockPages(Mdl, Irp->RequestorMode, Operation);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        NTSTATUS Status;

        Status = _SEH2_GetExceptionCode();

        /* Free the possible MDL we have allocated */
        IoFreeMdl(Mdl);
        Irp->MdlAddress = NULL;

        RxContext->Flags |= RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT;

        /* Fix status */
        if (!FsRtlIsNtstatusExpected(Status))
        {
            Status = STATUS_INVALID_USER_BUFFER;
        }

        RxContext->IoStatusBlock.Status = Status;
        ExRaiseStatus(Status);
    }
    _SEH2_END;
}

/*
 * @implemented
 */
NTSTATUS
RxLowIoCompletionTail(
    IN PRX_CONTEXT RxContext)
{
    NTSTATUS Status;
    USHORT Operation;

    PAGED_CODE();

    DPRINT("RxLowIoCompletionTail(%p)\n", RxContext);

    /* Only continue if we're at APC_LEVEL or lower */
    if (RxShouldPostCompletion() &&
        !BooleanFlagOn(RxContext->LowIoContext.Flags, LOWIO_CONTEXT_FLAG_CAN_COMPLETE_AT_DPC_LEVEL))
    {
        return STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Call the completion routine */
    DPRINT("Calling completion routine: %p\n", RxContext->LowIoContext.CompletionRoutine);
    Status = RxContext->LowIoContext.CompletionRoutine(RxContext);
    if (Status == STATUS_MORE_PROCESSING_REQUIRED || Status == STATUS_RETRY)
    {
        return Status;
    }

    /* If it was a RW operation, for a paging file ... */
    Operation = RxContext->LowIoContext.Operation;
    if (Operation == LOWIO_OP_READ || Operation == LOWIO_OP_WRITE)
    {
        /* Remove ourselves from the list and resume operations */
        if (BooleanFlagOn(RxContext->LowIoContext.ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_PAGING_IO))
        {
            ExAcquireFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);
            RemoveEntryList(&RxContext->RxContextSerializationQLinks);
            RxContext->RxContextSerializationQLinks.Flink = NULL;
            RxContext->RxContextSerializationQLinks.Blink = NULL;
            ExReleaseFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);
            RxResumeBlockedOperations_ALL(RxContext);
        }
    }
    else
    {
        /* Sanity check: we had known operation */
        ASSERT(Operation < LOWIO_OP_MAXIMUM);
    }

    /* If not sync operation, complete now. Otherwise, caller has already completed */
    if (!BooleanFlagOn(RxContext->LowIoContext.Flags, LOWIO_CONTEXT_FLAG_SYNCCALL))
    {
        RxCompleteRequest(RxContext, Status);
    }

    DPRINT("Status: %x\n", Status);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxLowIoPopulateFsctlInfo(
    IN PRX_CONTEXT RxContext)
{
    PMDL Mdl;
    PIRP Irp;
    UCHAR Method;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    DPRINT("RxLowIoPopulateFsctlInfo(%p)\n", RxContext);

    Irp = RxContext->CurrentIrp;
    Stack = RxContext->CurrentIrpSp;

    /* Copy stack parameters */
    RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode = Stack->Parameters.FileSystemControl.FsControlCode;
    RxContext->LowIoContext.ParamsFor.FsCtl.InputBufferLength = Stack->Parameters.FileSystemControl.InputBufferLength;
    RxContext->LowIoContext.ParamsFor.FsCtl.OutputBufferLength = Stack->Parameters.FileSystemControl.OutputBufferLength;
    RxContext->LowIoContext.ParamsFor.FsCtl.MinorFunction = Stack->MinorFunction;
    Method = METHOD_FROM_CTL_CODE(RxContext->LowIoContext.ParamsFor.FsCtl.FsControlCode);

    /* Same buffer in case of buffered */
    if (Method == METHOD_BUFFERED)
    {
        RxContext->LowIoContext.ParamsFor.FsCtl.pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
        RxContext->LowIoContext.ParamsFor.FsCtl.pOutputBuffer = Irp->AssociatedIrp.SystemBuffer;

        return STATUS_SUCCESS;
    }

    /* Two buffers for neither */
    if (Method == METHOD_NEITHER)
    {
        RxContext->LowIoContext.ParamsFor.FsCtl.pInputBuffer = Stack->Parameters.FileSystemControl.Type3InputBuffer;
        RxContext->LowIoContext.ParamsFor.FsCtl.pOutputBuffer = Irp->UserBuffer;

        return STATUS_SUCCESS;
    }

    /* Only IN/OUT remain */
    ASSERT(Method == METHOD_IN_DIRECT || Method == METHOD_OUT_DIRECT);

    /* Use system buffer for input */
    RxContext->LowIoContext.ParamsFor.FsCtl.pInputBuffer = Irp->AssociatedIrp.SystemBuffer;
    /* And MDL for output */
    Mdl = Irp->MdlAddress;
    if (Mdl != NULL)
    {
        RxContext->LowIoContext.ParamsFor.FsCtl.pOutputBuffer = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);
        if (RxContext->LowIoContext.ParamsFor.FsCtl.pOutputBuffer == NULL)
        {
            return STATUS_INSUFFICIENT_RESOURCES;
        }
    }
    else
    {
        RxContext->LowIoContext.ParamsFor.FsCtl.pOutputBuffer = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RxLowIoSubmit(
    IN PRX_CONTEXT RxContext,
    IN PLOWIO_COMPLETION_ROUTINE CompletionRoutine)
{
    NTSTATUS Status;
    USHORT Operation;
    BOOLEAN Synchronous;
    PLOWIO_CONTEXT LowIoContext;

    DPRINT("RxLowIoSubmit(%p, %p)\n", RxContext, CompletionRoutine);

    PAGED_CODE();

    LowIoContext = &RxContext->LowIoContext;
    Synchronous = !BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_ASYNC_OPERATION);

    LowIoContext->CompletionRoutine = CompletionRoutine;

    Status = STATUS_SUCCESS;
    Operation = LowIoContext->Operation;
    switch (Operation)
    {
        case LOWIO_OP_READ:
        case LOWIO_OP_WRITE:
            /* Check that the parameters were properly set by caller
             * See comment in RxInitializeLowIoContext()
             */
            ASSERT(LowIoContext->ParamsFor.ReadWrite.ByteOffset != 0xFFFFFFEE);
            ASSERT(LowIoContext->ParamsFor.ReadWrite.ByteCount != 0xEEEEEEEE);

            /* Lock the buffer */
            RxLockUserBuffer(RxContext,
                             (Operation == LOWIO_OP_READ ? IoWriteAccess : IoReadAccess),
                             LowIoContext->ParamsFor.ReadWrite.ByteCount);
            if (RxNewMapUserBuffer(RxContext) == NULL)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }
            LowIoContext->ParamsFor.ReadWrite.Buffer = RxContext->CurrentIrp->MdlAddress;

            /* If that's a paging IO, initialize serial operation */
            if (BooleanFlagOn(LowIoContext->ParamsFor.ReadWrite.Flags, LOWIO_READWRITEFLAG_PAGING_IO))
            {
                PFCB Fcb;

                Fcb = (PFCB)RxContext->pFcb;

                ExAcquireFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);
                RxContext->BlockedOpsMutex = &RxLowIoPagingIoSyncMutex;
                if (Operation == LOWIO_OP_READ)
                {
                    InsertTailList(&Fcb->Specific.Fcb.PagingIoReadsOutstanding, &RxContext->RxContextSerializationQLinks);
                }
                else
                {
                    InsertTailList(&Fcb->Specific.Fcb.PagingIoWritesOutstanding, &RxContext->RxContextSerializationQLinks);
                }

                ExReleaseFastMutexUnsafe(&RxLowIoPagingIoSyncMutex);
            }

            break;

        case LOWIO_OP_FSCTL:
        case LOWIO_OP_IOCTL:
            /* Set FSCTL/IOCTL parameters */
            Status = RxLowIoPopulateFsctlInfo(RxContext);
            /* Check whether we're consistent: a length means a buffer */
            if (NT_SUCCESS(Status))
            {
                if ((LowIoContext->ParamsFor.FsCtl.InputBufferLength > 0 &&
                     LowIoContext->ParamsFor.FsCtl.pInputBuffer == NULL) ||
                    (LowIoContext->ParamsFor.FsCtl.OutputBufferLength > 0 &&
                     LowIoContext->ParamsFor.FsCtl.pOutputBuffer == NULL))
                {
                    Status = STATUS_INVALID_PARAMETER;
                }
            }
            break;

        /* Nothing to do */
        case LOWIO_OP_SHAREDLOCK:
        case LOWIO_OP_EXCLUSIVELOCK:
        case LOWIO_OP_UNLOCK:
        case LOWIO_OP_UNLOCK_MULTIPLE:
        case LOWIO_OP_NOTIFY_CHANGE_DIRECTORY:
        case LOWIO_OP_CLEAROUT:
            break;

        default:
            ASSERT(FALSE);
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    /* No need to perform extra init in case of posting */
    RxContext->Flags |= RX_CONTEXT_FLAG_NO_PREPOSTING_NEEDED;

    /* Preflight checks were OK, time to submit */
    if (NT_SUCCESS(Status))
    {
        PMINIRDR_DISPATCH Dispatch;

        if (!Synchronous)
        {
            InterlockedIncrement((volatile long *)&RxContext->ReferenceCount);
            /* If not synchronous, we're likely to return before the operation is finished */
            if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP))
            {
                IoMarkIrpPending(RxContext->CurrentIrp);
            }
        }

        Dispatch = RxContext->RxDeviceObject->Dispatch;
        if (Dispatch != NULL)
        {
            /* We'll try to execute until the mini-rdr doesn't return pending */
            do
            {
                RxContext->IoStatusBlock.Information = 0;

                MINIRDR_CALL(Status, RxContext, Dispatch, MRxLowIOSubmit[Operation], (RxContext));
                if (Status == STATUS_PENDING)
                {
                    /* Unless it's not synchronous, caller will be happy with pending op */
                    if (!Synchronous)
                    {
                        return Status;
                    }

                    RxWaitSync(RxContext);
                    Status = RxContext->IoStatusBlock.Status;
                }
                else
                {
                    if (!Synchronous)
                    {
                        /* We had marked the IRP pending, whereas the operation finished, drop that */
                        if (Status != STATUS_RETRY)
                        {
                            if (!BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_IN_FSP))
                            {
                                RxContext->CurrentIrpSp->Flags &= ~SL_PENDING_RETURNED;
                            }

                            InterlockedDecrement((volatile long *)&RxContext->ReferenceCount);
                        }
                    }
                }
            } while (Status == STATUS_PENDING);
        }
        else
        {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    /* Call completion and return */
    RxContext->IoStatusBlock.Status = Status;
    LowIoContext->Flags |= LOWIO_CONTEXT_FLAG_SYNCCALL;
    return RxLowIoCompletionTail(RxContext);
}

/*
 * @implemented
 */
PVOID
RxMapSystemBuffer(
    IN PRX_CONTEXT RxContext)
{
    PIRP Irp;

    PAGED_CODE();

    Irp = RxContext->CurrentIrp;
    /* We should have a MDL (buffered IOs are not supported!) */
    if (Irp->MdlAddress != NULL)
    {
        ASSERT(FALSE);
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    }

    /* Just return system buffer */
    return Irp->AssociatedIrp.SystemBuffer;
}

/*
 * @implemented
 */
VOID
RxMarkFobxOnCleanup(
    PFOBX pFobx,
    PBOOLEAN NeedPurge)
{
    PFCB Fcb;
    PFOBX ScavengerFobx;
    LARGE_INTEGER TickCount;
    PRDBSS_SCAVENGER Scavenger;

    PAGED_CODE();

    /* No FOBX, nothing to mark */
    if (pFobx == NULL)
    {
        return;
    }

    /* Query time for close */
    KeQueryTickCount(&TickCount);

    Fcb = (PFCB)pFobx->pSrvOpen->pFcb;
    ASSERT(NodeTypeIsFcb(Fcb));

    Scavenger = Fcb->RxDeviceObject->pRdbssScavenger;
    RxAcquireScavengerMutex();

    ScavengerFobx = NULL;
    /* If that's not a file, or even not a disk resource, just mark as dormant */
    if (NodeType(Fcb) != RDBSS_NTC_STORAGE_TYPE_FILE || Fcb->VNetRoot->pNetRoot->DeviceType != FILE_DEVICE_DISK)
    {
        SetFlag(pFobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT);
        InitializeListHead(&pFobx->ClosePendingList);
        ++Scavenger->NumberOfDormantFiles;
    }
    else
    {
        ASSERT(Scavenger->NumberOfDormantFiles >= 0);
        /* If we're about to reach the maximum dormant of FOBX */
        if (Scavenger->NumberOfDormantFiles >= Scavenger->MaximumNumberOfDormantFiles)
        {
            /* This should never be wrong... */
            if (!IsListEmpty(&Scavenger->ClosePendingFobxsList))
            {
                /* Then, take the first from the list (oldest) and save it for later purge */
                ScavengerFobx = CONTAINING_RECORD(Scavenger->ClosePendingFobxsList.Flink, FOBX, ClosePendingList);
                if (ScavengerFobx->pSrvOpen != NULL && ScavengerFobx->pSrvOpen->pFcb == RX_GET_MRX_FCB(Fcb))
                {
                    *NeedPurge = TRUE;
                    ScavengerFobx = NULL;
                }
                else
                {
                    RxReferenceNetFobx(ScavengerFobx);
                }
            }
        }

        /* Mark ourselves as dormant */
        SetFlag(pFobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT);
        pFobx->CloseTime.QuadPart = TickCount.QuadPart;

        /* And insert us in the list of dormant files */
        InsertTailList(&Scavenger->ClosePendingFobxsList, &pFobx->ClosePendingList);
        /* If scavenger was inactive, start it */
        if (Scavenger->NumberOfDormantFiles++ == 0 && Scavenger->State == RDBSS_SCAVENGER_INACTIVE)
        {
            Scavenger->State = RDBSS_SCAVENGER_DORMANT;
            RxPostOneShotTimerRequest(RxFileSystemDeviceObject, &Scavenger->WorkItem, RxScavengerTimerRoutine,
                                      Fcb->RxDeviceObject, Scavenger->TimeLimit);
        }
    }

    RxReleaseScavengerMutex();

    /* If we had reached max */
    if (ScavengerFobx != NULL)
    {
        NTSTATUS Status;

        /* Purge the oldest FOBX */
        Status = RxPurgeFobxFromCache(ScavengerFobx);
        if (Status != STATUS_SUCCESS)
        {
            *NeedPurge = TRUE;
        }
    }
}

/*
 * @implemented
 */
VOID
RxMarkFobxOnClose(
    PFOBX Fobx)
{
    PFCB Fcb;
    PRDBSS_SCAVENGER Scavenger;

    PAGED_CODE();

    /* No FOBX, nothing to mark */
    if (Fobx == NULL)
    {
        return;
    }

    Fcb = (PFCB)Fobx->pSrvOpen->pFcb;
    ASSERT(NodeTypeIsFcb(Fcb));

    Scavenger = Fcb->RxDeviceObject->pRdbssScavenger;

    RxAcquireScavengerMutex();
    /* Only mark it if it was already marked as dormant */
    if (BooleanFlagOn(Fobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT))
    {
        /* If FCB wasn't already decrement, do it now */
        if (!Fobx->fOpenCountDecremented)
        {
            Fcb = (PFCB)Fobx->pSrvOpen->pFcb;
            ASSERT(NodeTypeIsFcb(Fcb));
            InterlockedDecrement((volatile long *)&Fcb->OpenCount);

            Fobx->fOpenCountDecremented = TRUE;
        }

        /* We're no longer dormant */
        InterlockedDecrement(&Scavenger->NumberOfDormantFiles);
        ClearFlag(Fobx->Flags, FOBX_FLAG_MARKED_AS_DORMANT);
    }

    /* If we were inserted in the scavenger, drop ourselves out */
    if (!IsListEmpty(&Fobx->ClosePendingList))
    {
        RemoveEntryList(&Fobx->ClosePendingList);
        InitializeListHead(&Fobx->ClosePendingList);
    }

    RxReleaseScavengerMutex();
}

/*
 * @implemented
 */
PVOID
RxNewMapUserBuffer(
    PRX_CONTEXT RxContext)
{
    PIRP Irp;

    PAGED_CODE();

    Irp = RxContext->CurrentIrp;
    if (Irp->MdlAddress != NULL)
    {
        return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
    }

    return Irp->UserBuffer;
}

BOOLEAN
NTAPI
RxNoOpAcquire(
    IN PVOID Fcb,
    IN BOOLEAN Wait)
{
    UNIMPLEMENTED;
    return FALSE;
}

VOID
NTAPI
RxNoOpRelease(
    IN PVOID Fcb)
{
    UNIMPLEMENTED;
}

VOID
RxOrphanThisFcb(
    PFCB Fcb)
{
    UNIMPLEMENTED;
}

VOID
RxOrphanSrvOpens(
    IN PV_NET_ROOT ThisVNetRoot)
{
    PFCB Fcb;
    USHORT Bucket;
    PNET_ROOT NetRoot;
    PRX_FCB_TABLE FcbTable;
    PRX_PREFIX_TABLE PrefixTable;

    PAGED_CODE();

    /* Mailslot won't have any SRV_OPEN (to orphan) */
    NetRoot = (PNET_ROOT)ThisVNetRoot->pNetRoot;
    if (NetRoot->Type == NET_ROOT_MAILSLOT)
    {
        return;
    }

    PrefixTable = NetRoot->pSrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT(RxIsPrefixTableLockExclusive(PrefixTable));

    FcbTable = &NetRoot->FcbTable;
    RxAcquireFcbTableLockExclusive(FcbTable, TRUE);

    _SEH2_TRY
    {
        /* Now, we'll browse all the FCBs attached, and orphan related SRV_OPENs */
        for (Bucket = 0; Bucket < FcbTable->NumberOfBuckets; ++Bucket)
        {
            PLIST_ENTRY BucketList, Entry;

            BucketList = &FcbTable->HashBuckets[Bucket];
            Entry = BucketList->Flink;
            while (Entry != BucketList)
            {
                Fcb = CONTAINING_RECORD(Entry, FCB, FcbTableEntry.HashLinks);
                Entry = Entry->Flink;

                ASSERT(NodeTypeIsFcb(Fcb));
                RxOrphanSrvOpensForThisFcb(Fcb, ThisVNetRoot, FALSE);
            }
        }

        /* Of course, don't forget about NULL-entry */
        if (FcbTable->TableEntryForNull != NULL)
        {
            Fcb = CONTAINING_RECORD(FcbTable->TableEntryForNull, FCB, FcbTableEntry.HashLinks);
            ASSERT(NodeTypeIsFcb(Fcb));
            RxOrphanSrvOpensForThisFcb(Fcb, ThisVNetRoot, FALSE);
        }
    }
    _SEH2_FINALLY
    {
        RxReleaseFcbTableLock(FcbTable);
    }
    _SEH2_END;
}

VOID
RxOrphanSrvOpensForThisFcb(
    IN PFCB Fcb,
    IN PV_NET_ROOT ThisVNetRoot,
    IN BOOLEAN OrphanAll)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
BOOLEAN
RxpAcquirePrefixTableLockShared(
   PRX_PREFIX_TABLE pTable,
   BOOLEAN Wait,
   BOOLEAN ProcessBufferingStateChangeRequests)
{
    PAGED_CODE();

    DPRINT("RxpAcquirePrefixTableLockShared(%p, %d, %d) -> %d\n", pTable, Wait, ProcessBufferingStateChangeRequests,
           pTable->TableLock.ActiveEntries);

    return ExAcquireResourceSharedLite(&pTable->TableLock, Wait);
}

/*
 * @implemented
 */
BOOLEAN
RxpAcquirePrefixTableLockExclusive(
   PRX_PREFIX_TABLE pTable,
   BOOLEAN Wait,
   BOOLEAN ProcessBufferingStateChangeRequests)
{
    PAGED_CODE();

    DPRINT("RxpAcquirePrefixTableLockExclusive(%p, %d, %d) -> %d\n", pTable, Wait, ProcessBufferingStateChangeRequests,
           pTable->TableLock.ActiveEntries);

    return ExAcquireResourceExclusiveLite(&pTable->TableLock, Wait);
}

/*
 * @implemented
 */
BOOLEAN
RxpDereferenceAndFinalizeNetFcb(
    OUT PFCB ThisFcb,
    IN PRX_CONTEXT RxContext,
    IN BOOLEAN RecursiveFinalize,
    IN BOOLEAN ForceFinalize)
{
    NTSTATUS Status;
    ULONG References;
    PNET_ROOT NetRoot;
    BOOLEAN ResourceAcquired, NetRootReferenced, Freed;

    PAGED_CODE();

    ASSERT(!ForceFinalize);
    ASSERT(NodeTypeIsFcb(ThisFcb));
    ASSERT(RxIsFcbAcquiredExclusive(ThisFcb));

    /* Unless we're recursively finalizing, or forcing, if FCB is still in use, quit */
    References = InterlockedDecrement((volatile long *)&ThisFcb->NodeReferenceCount);
    if (!ForceFinalize && !RecursiveFinalize && (ThisFcb->OpenCount != 0 || ThisFcb->UncleanCount != 0 || References > 1))
    {
        return FALSE;
    }

    Freed = FALSE;
    Status = STATUS_SUCCESS;
    NetRoot = (PNET_ROOT)ThisFcb->VNetRoot->pNetRoot;
    ResourceAcquired = FALSE;
    NetRootReferenced = FALSE;
    /* If FCB isn't orphaned, it still have context attached */
    if (!BooleanFlagOn(ThisFcb->FcbState, FCB_STATE_ORPHANED))
    {
        /* Don't let NetRoot go away before we're done */
        RxReferenceNetRoot(NetRoot);
        NetRootReferenced = TRUE;

        /* Try to acquire the table lock exclusively */
        if (!RxIsFcbTableLockExclusive(&NetRoot->FcbTable))
        {
            RxReferenceNetFcb(ThisFcb);

            if (!RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, FALSE))
            {
                if (RxContext != NULL && RxContext != CHANGE_BUFFERING_STATE_CONTEXT &&
                    RxContext != CHANGE_BUFFERING_STATE_CONTEXT_WAIT)
                {
                    RxContext->Flags |= RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK;
                }

                RxReleaseFcb(RxContext, ThisFcb);

                RxAcquireFcbTableLockExclusive(&NetRoot->FcbTable, TRUE);

                Status = RxAcquireExclusiveFcb(RxContext, ThisFcb);
            }

            References = RxDereferenceNetFcb(ThisFcb);

            ResourceAcquired = TRUE;
        }
    }

    /* If locking was OK (or not needed!), attempt finalization */
    if (Status == STATUS_SUCCESS)
    {
        Freed = RxFinalizeNetFcb(ThisFcb, RecursiveFinalize, ForceFinalize, References);
    }

    /* Release table lock if acquired */
    if (ResourceAcquired)
    {
        RxReleaseFcbTableLock(&NetRoot->FcbTable);
    }

    /* We don't need the NetRoot anylonger */
    if (NetRootReferenced)
    {
        RxDereferenceNetRoot(NetRoot, LHS_LockNotHeld);
    }

    return Freed;
}

/*
 * @implemented
 */
LONG
RxpDereferenceNetFcb(
   PFCB Fcb)
{
    LONG NewCount;

    PAGED_CODE();

    ASSERT(NodeTypeIsFcb(Fcb));

    NewCount = InterlockedDecrement((volatile long *)&Fcb->NodeReferenceCount);
    ASSERT(NewCount >= 0);

    PRINT_REF_COUNT(NETFCB, NewCount);

    return NewCount;
}

/*
 * @implemented
 */
VOID
NTAPI
RxpDestroySrvCall(
    IN PVOID Context)
{
    NTSTATUS Status;
    PSRV_CALL SrvCall;
    BOOLEAN ForceFinalize;
    PRX_PREFIX_TABLE PrefixTable;

    SrvCall = (PSRV_CALL)Context;
    /* At this step, RxFinalizeSrvCall already cleaned some fields */
    ASSERT(SrvCall->UpperFinalizationDone);

    PrefixTable = SrvCall->RxDeviceObject->pRxNetNameTable;
    /* Were we called with ForceFinalize? */
    ForceFinalize = BooleanFlagOn(SrvCall->Flags, SRVCALL_FLAG_FORCE_FINALIZED);

    /* Notify mini-rdr */
    MINIRDR_CALL_THROUGH(Status, SrvCall->RxDeviceObject->Dispatch,
                         MRxFinalizeSrvCall, ((PMRX_SRV_CALL)SrvCall,
                         ForceFinalize));
    (void)Status;

    /* Dereference our extra reference (set before queueing) */
    RxAcquirePrefixTableLockExclusive(PrefixTable, TRUE);
    InterlockedDecrement((volatile long *)&SrvCall->NodeReferenceCount);
    /* And finalize for real, with the right context */
    RxFinalizeSrvCall(SrvCall, FALSE, ForceFinalize);
    RxReleasePrefixTableLock(PrefixTable);
}

/*
 * @implemented
 */
VOID
RxpDiscardChangeBufferingStateRequests(
    _Inout_ PLIST_ENTRY DiscardedRequests)
{
    PLIST_ENTRY Entry;

    PAGED_CODE();

    /* No requests to discard */
    if (IsListEmpty(DiscardedRequests))
    {
        return;
    }

    /* Free all the discarded requests */
    Entry = DiscardedRequests->Flink;
    while (Entry != DiscardedRequests)
    {
        PCHANGE_BUFFERING_STATE_REQUEST Request;

        Request = CONTAINING_RECORD(Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry);
        Entry = Entry->Flink;

        DPRINT("Req %p for %p (%p) discarded\n", Request, Request->SrvOpenKey, Request->SrvOpen);

        RxPrepareRequestForReuse(Request);
        RxFreePool(Request);
    }
}

/*
 * @implemented
 */
VOID
RxpDispatchChangeBufferingStateRequests(
    PSRV_CALL SrvCall,
    PSRV_OPEN SrvOpen,
    PLIST_ENTRY DiscardedRequests)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    BOOLEAN StartDispatcher;
    LIST_ENTRY AcceptedReqs;
    LIST_ENTRY DispatcherList;
    PRX_BUFFERING_MANAGER BufferingManager;

    /* Initialize our lists */
    InitializeListHead(&AcceptedReqs);
    InitializeListHead(DiscardedRequests);

    /* Transfer the requests to dispatch locally */
    BufferingManager = &SrvCall->BufferingManager;
    KeAcquireSpinLock(&BufferingManager->SpinLock, &OldIrql);
    RxTransferList(&DispatcherList, &BufferingManager->DispatcherList);
    KeReleaseSpinLock(&BufferingManager->SpinLock, OldIrql);

    /* If there were requests */
    if (!IsListEmpty(&DispatcherList))
    {
        PLIST_ENTRY Entry;

        /* For each of the entries... */
        Entry = DispatcherList.Flink;
        while (Entry != &DispatcherList)
        {
            PCHANGE_BUFFERING_STATE_REQUEST Request;

            Request = CONTAINING_RECORD(Entry, CHANGE_BUFFERING_STATE_REQUEST, ListEntry);
            Entry = Entry->Flink;

            /* If we have been provided a SRV_OPEN, see whether it matches */
            if (SrvOpen != NULL)
            {
                /* Match, the request is accepted */
                if (Request->SrvOpenKey == SrvOpen->Key)
                {
                    Request->SrvOpen = SrvOpen;
                    RxReferenceSrvOpen(SrvOpen);

                    RemoveEntryList(&Request->ListEntry);
                    InsertTailList(&AcceptedReqs, &Request->ListEntry);

                    /* Move to the next entry */
                    continue;
                }
                else
                {
                    Status = STATUS_PENDING;
                }
            }
            else
            {
                /* No SRV_OPEN provided, try to find one */
                Status = RxpLookupSrvOpenForRequestLite(SrvCall, Request);
            }

            /* We found a matching SRV_OPEN, accept the request */
            if (Status == STATUS_SUCCESS)
            {
                RemoveEntryList(&Request->ListEntry);
                InsertTailList(&AcceptedReqs, &Request->ListEntry);
            }
            /* Another run might help handling it, don't discard it */
            else if (Status == STATUS_PENDING)
            {
                continue;
            }
            /* Otherwise, discard the request */
            else
            {
                ASSERT(Status == STATUS_NOT_FOUND);

                RemoveEntryList(&Request->ListEntry);
                InsertTailList(DiscardedRequests, &Request->ListEntry);
            }
        }
    }

    KeAcquireSpinLock(&BufferingManager->SpinLock, &OldIrql);
    /* Nothing to dispatch, no need to start dispatcher */
    if (IsListEmpty(&DispatcherList))
    {
        StartDispatcher = FALSE;
    }
    else
    {
        /* Transfer back the list of the not treated entries to the buffering manager */
        RxTransferList(&BufferingManager->DispatcherList, &DispatcherList);
        StartDispatcher = (BufferingManager->DispatcherActive == FALSE);
        /* If the dispatcher isn't active, start it */
        if (StartDispatcher)
        {
            BufferingManager->DispatcherActive = TRUE;
        }
    }

    /* If there were accepted requests, move them to the buffering manager */
    if (!IsListEmpty(&AcceptedReqs))
    {
        RxTransferList(&BufferingManager->HandlerList, &AcceptedReqs);
    }
    KeReleaseSpinLock(&BufferingManager->SpinLock, OldIrql);

    /* If we're to start the dispatcher, do it */
    if (StartDispatcher)
    {
        RxReferenceSrvCall(SrvCall);
        DPRINT("Starting dispatcher\n");
        RxPostToWorkerThread(RxFileSystemDeviceObject, HyperCriticalWorkQueue,
                             &BufferingManager->DispatcherWorkItem,
                             RxDispatchChangeBufferingStateRequests, SrvCall);
    }
}

/*
 * @implemented
 */
NTSTATUS
RxpLookupSrvOpenForRequestLite(
    IN PSRV_CALL SrvCall,
    IN OUT PCHANGE_BUFFERING_STATE_REQUEST Request)
{
    NTSTATUS Status;
    PLIST_ENTRY Entry;
    PSRV_OPEN SrvOpen;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    /* Browse all our associated SRV_OPENs to find the one! */
    for (Entry = SrvCall->BufferingManager.SrvOpenLists[0].Flink;
         Entry != &SrvCall->BufferingManager.SrvOpenLists[0];
         Entry = Entry->Flink)
    {
        /* Same key, not orphaned, this is ours */
        SrvOpen = CONTAINING_RECORD(Entry, SRV_OPEN, SrvOpenKeyList);
        if (SrvOpen->Key == Request->SrvOpenKey)
        {
            if (!BooleanFlagOn(SrvOpen->pFcb->FcbState, FCB_STATE_ORPHANED))
            {
                RxReferenceSrvOpen(SrvOpen);
                break;
            }
        }
    }

    /* We didn't manage to find a SRV_OPEN */
    if (Entry == &SrvCall->BufferingManager.SrvOpenLists[0])
    {
        SrvOpen = NULL;

        /* The coming open might help, mark as pending for later retry */
        if (SrvCall->BufferingManager.NumberOfOutstandingOpens != 0)
        {
            Status = STATUS_PENDING;
        }
        /* Else, it's a complete failure */
        else
        {
            Status = STATUS_NOT_FOUND;
        }
    }

    /* Return the (not) found SRV_OPEN */
    Request->SrvOpen = SrvOpen;

    return Status;
}

/*
 * @implemented
 */
VOID
RxpMarkInstanceForScavengedFinalization(
   PVOID Instance)
{
    NODE_TYPE_CODE NodeType;
    PNODE_TYPE_AND_SIZE Node;
    PRDBSS_SCAVENGER Scavenger;
    PRDBSS_DEVICE_OBJECT DeviceObject;
    PLIST_ENTRY ScavengerHead, InstEntry;

    PAGED_CODE();

    /* If still referenced, don't mark it (broken caller) */
    Node = (PNODE_TYPE_AND_SIZE)Instance;
    if (Node->NodeReferenceCount > 1)
    {
        return;
    }

    DeviceObject = RxGetDeviceObjectOfInstance(Instance);
    Scavenger = DeviceObject->pRdbssScavenger;

    /* Mark the node */
    NodeType = NodeType(Instance);
    SetFlag(NodeType(Node), RX_SCAVENGER_MASK);
    DPRINT("Node %p has now the scavenger mark!\n", Instance);

    /* Increase the count in the scavenger, and queue it */
    ScavengerHead = NULL;
    switch (NodeType)
    {
        case RDBSS_NTC_FOBX:
            ++Scavenger->FobxsToBeFinalized;
            ScavengerHead = &Scavenger->FobxFinalizationList;
            InstEntry = &((PFOBX)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_SRVCALL:
            ++Scavenger->SrvCallsToBeFinalized;
            ScavengerHead = &Scavenger->SrvCallFinalizationList;
            InstEntry = &((PSRV_CALL)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_NETROOT:
            ++Scavenger->NetRootsToBeFinalized;
            ScavengerHead = &Scavenger->NetRootFinalizationList;
            InstEntry = &((PNET_ROOT)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_V_NETROOT:
            ++Scavenger->VNetRootsToBeFinalized;
            ScavengerHead = &Scavenger->VNetRootFinalizationList;
            InstEntry = &((PV_NET_ROOT)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_SRVOPEN:
            ++Scavenger->SrvOpensToBeFinalized;
            ScavengerHead = &Scavenger->SrvOpenFinalizationList;
            InstEntry = &((PSRV_OPEN)Instance)->ScavengerFinalizationList;
            break;
    }

    /* Extra ref for scavenger */
    InterlockedIncrement((volatile long *)&Node->NodeReferenceCount);

    /* If matching type */
    if (ScavengerHead != NULL)
    {
        /* Insert in the scavenger list */
        InsertTailList(ScavengerHead, InstEntry);

        /* And if it wasn't started, start it */
        if (Scavenger->State == RDBSS_SCAVENGER_INACTIVE)
        {
            Scavenger->State = RDBSS_SCAVENGER_DORMANT;
            RxPostOneShotTimerRequest(RxFileSystemDeviceObject, &Scavenger->WorkItem,
                                      RxScavengerTimerRoutine, DeviceObject, Scavenger->TimeLimit);
        }
    }
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxPostOneShotTimerRequest(
    IN PRDBSS_DEVICE_OBJECT pDeviceObject,
    IN PRX_WORK_ITEM pWorkItem,
    IN PRX_WORKERTHREAD_ROUTINE Routine,
    IN PVOID pContext,
    IN LARGE_INTEGER TimeInterval)
{
    KIRQL OldIrql;

    ASSERT(pWorkItem != NULL);

    /* Prepare the work item */
    ExInitializeWorkItem(&pWorkItem->WorkQueueItem, Routine, pContext);
    pWorkItem->WorkQueueItem.pDeviceObject = pDeviceObject;

    /* Last tick can be computed with the number of times it was caller (timertickcount)
     * and the interval between calls
     */
    KeAcquireSpinLock(&RxTimerLock, &OldIrql);
    pWorkItem->LastTick = (TimeInterval.QuadPart / 550000) + RxTimerTickCount + 1;
    /* Insert in work queue */
    InsertTailList(&RxTimerQueueHead, &pWorkItem->WorkQueueItem.List);
    KeReleaseSpinLock(&RxTimerLock, OldIrql);

    /* If there are queued events, queue an execution */
    if (IsListEmpty(&RxTimerQueueHead))
    {
        KeSetTimer(&RxTimer, RxTimerInterval, &RxTimerDpc);
    }

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RxPostToWorkerThread(
    _In_ PRDBSS_DEVICE_OBJECT pMRxDeviceObject,
    _In_ WORK_QUEUE_TYPE WorkQueueType,
    _In_ PRX_WORK_QUEUE_ITEM pWorkQueueItem,
    _In_ PRX_WORKERTHREAD_ROUTINE Routine,
    _In_ PVOID pContext)
{
    /* Initialize work queue item */
    pWorkQueueItem->List.Flink = NULL;
    pWorkQueueItem->WorkerRoutine = Routine;
    pWorkQueueItem->Parameter = pContext;

    /* And insert it in the work queue */
    return RxInsertWorkQueueItem(pMRxDeviceObject, WorkQueueType, pWorkQueueItem);
}

VOID
RxpProcessChangeBufferingStateRequests(
    PSRV_CALL SrvCall,
    BOOLEAN UpdateHandlerState)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
PRX_PREFIX_ENTRY
RxPrefixTableInsertName(
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry,
    IN PVOID Container,
    IN PULONG ContainerRefCount,
    IN USHORT CaseInsensitiveLength,
    IN PRX_CONNECTION_ID ConnectionId
    )
{
    PAGED_CODE();

    DPRINT("Insert: %wZ\n", &ThisEntry->Prefix);

    ASSERT(RxIsPrefixTableLockExclusive(ThisTable));
    ASSERT(CaseInsensitiveLength <= ThisEntry->Prefix.Length);

    /* Copy parameters and compute hash */
    ThisEntry->CaseInsensitiveLength = CaseInsensitiveLength;
    ThisEntry->ContainingRecord = Container;
    ThisEntry->ContainerRefCount = ContainerRefCount;
    InterlockedIncrement((volatile long *)ContainerRefCount);
    ThisEntry->SavedHashValue = RxTableComputeHashValue(&ThisEntry->Prefix);
    DPRINT("Associated hash: %x\n", ThisEntry->SavedHashValue);

    /* If no path length: this is entry for null path */
    if (ThisEntry->Prefix.Length == 0)
    {
        ThisTable->TableEntryForNull = ThisEntry;
    }
    /* Otherwise, insert in the appropriate bucket */
    else
    {
        InsertTailList(HASH_BUCKET(ThisTable, ThisEntry->SavedHashValue), &ThisEntry->HashLinks);
    }

    /* If we had a connection ID, keep track of it */
    if (ConnectionId != NULL)
    {
        ThisEntry->ConnectionId.Luid = ConnectionId->Luid;
    }
    else
    {
        ThisEntry->ConnectionId.Luid.LowPart = 0;
        ThisEntry->ConnectionId.Luid.HighPart = 0;
    }

    InsertTailList(&ThisTable->MemberQueue, &ThisEntry->MemberQLinks);
    /* Reflect the changes */
    ++ThisTable->Version;

    DPRINT("Inserted in bucket: %p\n", HASH_BUCKET(ThisTable, ThisEntry->SavedHashValue));

    return ThisEntry;
}

/*
 * @implemented
 */
PVOID
RxPrefixTableLookupName(
    IN PRX_PREFIX_TABLE ThisTable,
    IN PUNICODE_STRING CanonicalName,
    OUT PUNICODE_STRING RemainingName,
    IN PRX_CONNECTION_ID ConnectionId)
{
    PVOID Container;

    PAGED_CODE();

    ASSERT(RxIsPrefixTableLockAcquired(ThisTable));
    ASSERT(CanonicalName->Length > 0);

    /* Call the internal helper */
    Container = RxTableLookupName(ThisTable, CanonicalName, RemainingName, ConnectionId);
    if (Container == NULL)
    {
        return NULL;
    }

    /* Reference our container before returning it */
    if (RdbssReferenceTracingValue != 0)
    {
        NODE_TYPE_CODE Type;

        Type = (NodeType(Container) & ~RX_SCAVENGER_MASK);
        switch (Type)
        {
            case RDBSS_NTC_SRVCALL:
                RxReferenceSrvCall(Container);
                break;

            case RDBSS_NTC_NETROOT:
                RxReferenceNetRoot(Container);
                break;

            case RDBSS_NTC_V_NETROOT:
                RxReferenceVNetRoot(Container);
                break;

            default:
                DPRINT1("Invalid node type: %x\n", Type);
                ASSERT(FALSE);
                RxReference(Container);
                break;
        }
    }
    else
    {
        RxReference(Container);
    }

    return Container;
}

/*
 * @implemented
 */
LONG
RxpReferenceNetFcb(
   PFCB Fcb)
{
    LONG NewCount;

    PAGED_CODE();

    ASSERT(NodeTypeIsFcb(Fcb));

    NewCount = InterlockedIncrement((volatile long *)&Fcb->NodeReferenceCount);

    PRINT_REF_COUNT(NETFCB, Fcb->NodeReferenceCount);

    return NewCount;
}

/*
 * @implemented
 */
VOID
RxpReleasePrefixTableLock(
   PRX_PREFIX_TABLE pTable,
   BOOLEAN ProcessBufferingStateChangeRequests)
{
    PAGED_CODE();

    DPRINT("RxpReleasePrefixTableLock(%p, %d) -> %d\n", pTable, ProcessBufferingStateChangeRequests,
           pTable->TableLock.ActiveEntries);

    ExReleaseResourceLite(&pTable->TableLock);
}

/*
 * @implemented
 */
VOID
NTAPI
RxPrepareContextForReuse(
   IN OUT PRX_CONTEXT RxContext)
{
    PAGED_CODE();

    /* When we reach that point, make sure mandatory parts are null-ed */
    if (RxContext->MajorFunction == IRP_MJ_CREATE)
    {
        ASSERT(RxContext->Create.CanonicalNameBuffer == NULL);
        RxContext->Create.RdrFlags = 0;
    }
    else if (RxContext->MajorFunction == IRP_MJ_READ || RxContext->MajorFunction == IRP_MJ_WRITE)
    {
        ASSERT(RxContext->RxContextSerializationQLinks.Flink == NULL);
        ASSERT(RxContext->RxContextSerializationQLinks.Blink == NULL);
    }

    RxContext->ReferenceCount = 0;
}

/*
 * @implemented
 */
VOID
RxPrepareRequestForReuse(
    PCHANGE_BUFFERING_STATE_REQUEST Request)
{
    PSRV_OPEN SrvOpen;

    PAGED_CODE();

    SrvOpen = Request->SrvOpen;

    /* If the request was already prepared for service */
    if (BooleanFlagOn(Request->Flags, RX_REQUEST_PREPARED_FOR_HANDLING))
    {
        /* We have to dereference the associated SRV_OPEN depending on the lock */
        if (RxIsFcbAcquiredExclusive(SrvOpen->pFcb))
        {
            RxDereferenceSrvOpen(SrvOpen, LHS_ExclusiveLockHeld);
        }
        else
        {
            RxDereferenceSrvOpen(SrvOpen, LHS_LockNotHeld);
        }
    }
    /* Otherwise, just dereference */
    else if (SrvOpen != NULL)
    {
        RxDereferenceSrvOpen(SrvOpen, LHS_LockNotHeld);
    }

    Request->SrvOpen = NULL;
}

/*
 * @implemented
 */
VOID
NTAPI
RxProcessChangeBufferingStateRequests(
    _In_ PVOID SrvCall)
{
    /* Call internal routine */
    RxUndoScavengerFinalizationMarking(SrvCall);
    RxpProcessChangeBufferingStateRequests(SrvCall, TRUE);
}

/*
 * @implemented
 */
VOID
RxProcessChangeBufferingStateRequestsForSrvOpen(
    PSRV_OPEN SrvOpen)
{
    LONG NumberOfBufferingChangeRequests, LockedOldBufferingToken, OldBufferingToken;

    /* Get the current number of change requests */
    NumberOfBufferingChangeRequests = ((PSRV_CALL)SrvOpen->pVNetRoot->pNetRoot->pSrvCall)->BufferingManager.CumulativeNumberOfBufferingChangeRequests;
    /* Get our old token */
    OldBufferingToken = SrvOpen->BufferingToken;
    LockedOldBufferingToken = InterlockedCompareExchange(&SrvOpen->BufferingToken,
                                                         NumberOfBufferingChangeRequests,
                                                         NumberOfBufferingChangeRequests);
    /* If buffering state changed in between, process changes */
    if (OldBufferingToken != LockedOldBufferingToken)
    {
        PFCB Fcb;
        NTSTATUS Status;

        /* Acquire the FCB and start processing */
        Fcb = (PFCB)SrvOpen->pFcb;
        Status = RxAcquireExclusiveFcb(NULL, Fcb);
        if (Status == STATUS_SUCCESS)
        {
            RxProcessFcbChangeBufferingStateRequest(Fcb);
            RxReleaseFcb(NULL, Fcb);
        }
    }
}

VOID
RxProcessFcbChangeBufferingStateRequest(
    PFCB Fcb)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxpScavengeFobxs(
    PRDBSS_SCAVENGER Scavenger,
    PLIST_ENTRY FobxToScavenge)
{
    /* Explore the whole list of FOBX to scavenge */
    while (!IsListEmpty(FobxToScavenge))
    {
        PFCB Fcb;
        PFOBX Fobx;
        PLIST_ENTRY Entry;

        Entry = RemoveHeadList(FobxToScavenge);
        Fobx = CONTAINING_RECORD(Entry, FOBX, ScavengerFinalizationList);
        Fcb = (PFCB)Fobx->SrvOpen->pFcb;

        /* Try to acquire the lock exclusively to perform finalization */
        if (RxAcquireExclusiveFcb(NULL, Fcb) != STATUS_SUCCESS)
        {
            RxDereferenceNetRoot(Fobx, LHS_LockNotHeld);
        }
        else
        {
            RxReferenceNetFcb(Fcb);
            RxDereferenceNetRoot(Fobx, LHS_ExclusiveLockHeld);

            if (!RxDereferenceAndFinalizeNetFcb(Fcb, NULL, FALSE, FALSE))
            {
                RxReleaseFcb(NULL, Fcb);
            }
        }
    }
}

BOOLEAN
RxpTrackDereference(
    _In_ ULONG TraceType,
    _In_ PCSTR FileName,
    _In_ ULONG Line,
    _In_ PVOID Instance)
{
    PCSTR InstanceType;
    ULONG ReferenceCount;

    PAGED_CODE();

    if (!BooleanFlagOn(RdbssReferenceTracingValue, TraceType))
    {
        return TRUE;
    }

    switch (TraceType)
    {
        case RDBSS_REF_TRACK_SRVCALL:
            InstanceType = "SrvCall";
            ReferenceCount = ((PSRV_CALL)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_NETROOT:
            InstanceType = "NetRoot";
            ReferenceCount = ((PNET_ROOT)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_VNETROOT:
            InstanceType = "VNetRoot";
            ReferenceCount = ((PV_NET_ROOT)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_NETFOBX:
            InstanceType = "NetFobx";
            ReferenceCount = ((PFOBX)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_NETFCB:
            InstanceType = "NetFcb";
            ReferenceCount = ((PFCB)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_SRVOPEN:
            InstanceType = "SrvOpen";
            ReferenceCount = ((PSRV_OPEN)Instance)->NodeReferenceCount;
            break;

        default:
            DPRINT1("Invalid node type!\n");
            return TRUE;
    }

    if (BooleanFlagOn(RdbssReferenceTracingValue, RX_LOG_REF_TRACKING))
    {
        UNIMPLEMENTED;
    }

    if (BooleanFlagOn(RdbssReferenceTracingValue, RX_PRINT_REF_TRACKING))
    {
        DbgPrint("(%s:%d) %p (%s) dereferenced from %d\n", FileName, Line, Instance, InstanceType, ReferenceCount);
    }

    return TRUE;
}

VOID
RxpTrackReference(
    _In_ ULONG TraceType,
    _In_ PCSTR FileName,
    _In_ ULONG Line,
    _In_ PVOID Instance)
{
    PCSTR InstanceType;
    ULONG ReferenceCount;

    if (!BooleanFlagOn(RdbssReferenceTracingValue, TraceType))
    {
        return;
    }

    switch (TraceType)
    {
        case RDBSS_REF_TRACK_SRVCALL:
            InstanceType = "SrvCall";
            ReferenceCount = ((PSRV_CALL)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_NETROOT:
            InstanceType = "NetRoot";
            ReferenceCount = ((PNET_ROOT)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_VNETROOT:
            InstanceType = "VNetRoot";
            ReferenceCount = ((PV_NET_ROOT)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_NETFOBX:
            InstanceType = "NetFobx";
            ReferenceCount = ((PFOBX)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_NETFCB:
            InstanceType = "NetFcb";
            ReferenceCount = ((PFCB)Instance)->NodeReferenceCount;
            break;

        case RDBSS_REF_TRACK_SRVOPEN:
            InstanceType = "SrvOpen";
            ReferenceCount = ((PSRV_OPEN)Instance)->NodeReferenceCount;
            break;

        default:
            DPRINT1("Invalid node type!\n");
            return;
    }

    if (BooleanFlagOn(RdbssReferenceTracingValue, RX_LOG_REF_TRACKING))
    {
        UNIMPLEMENTED;
    }

    if (BooleanFlagOn(RdbssReferenceTracingValue, RX_PRINT_REF_TRACKING))
    {
        DbgPrint("(%s:%d) %p (%s) referenced from %d\n", FileName, Line, Instance, InstanceType, ReferenceCount);
    }
}

/*
 * @implemented
 */
VOID
RxpUndoScavengerFinalizationMarking(
   PVOID Instance)
{
    PLIST_ENTRY ListEntry;
    PNODE_TYPE_AND_SIZE Node;
    PRDBSS_SCAVENGER Scavenger;

    PAGED_CODE();

    Node = (PNODE_TYPE_AND_SIZE)Instance;
    /* There's no marking - nothing to do */
    if (!BooleanFlagOn(NodeType(Node), RX_SCAVENGER_MASK))
    {
        return;
    }

    /* First of all, remove the mark */
    ClearFlag(NodeType(Node), RX_SCAVENGER_MASK);
    DPRINT("Node %p no longer has the scavenger mark\n");

    /* And now, remove from the scavenger */
    Scavenger = RxGetDeviceObjectOfInstance(Instance)->pRdbssScavenger;
    switch (NodeType(Node))
    {
        case RDBSS_NTC_FOBX:
            --Scavenger->FobxsToBeFinalized;
            ListEntry = &((PFOBX)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_SRVCALL:
            --Scavenger->SrvCallsToBeFinalized;
            ListEntry = &((PSRV_CALL)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_NETROOT:
            --Scavenger->NetRootsToBeFinalized;
            ListEntry = &((PNET_ROOT)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_V_NETROOT:
            --Scavenger->VNetRootsToBeFinalized;
            ListEntry = &((PV_NET_ROOT)Instance)->ScavengerFinalizationList;
            break;

        case RDBSS_NTC_SRVOPEN:
            --Scavenger->SrvOpensToBeFinalized;
            ListEntry = &((PSRV_OPEN)Instance)->ScavengerFinalizationList;
            break;

        default:
            return;
    }

    /* Also, remove the extra ref from the scavenger */
    RemoveEntryList(ListEntry);
    InterlockedDecrement((volatile long *)&Node->NodeReferenceCount);
}

/*
 * @implemented
 */
VOID
RxPurgeChangeBufferingStateRequestsForSrvOpen(
    PSRV_OPEN SrvOpen)
{
    PSRV_CALL SrvCall;
    LIST_ENTRY Discarded;

    PAGED_CODE();

    ASSERT(RxIsFcbAcquiredExclusive(SrvOpen->Fcb));

    /* Initialize our discarded list */
    InitializeListHead(&Discarded);

    SrvCall = (PSRV_CALL)SrvOpen->Fcb->VNetRoot->pNetRoot->pSrvCall;
    RxAcquireBufferingManagerMutex(&SrvCall->BufferingManager);

    /* Set the flag, and get the requests */
    InitializeListHead(&SrvOpen->SrvOpenKeyList);
    SetFlag(SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_REQUESTS_PURGED);
    RxGatherRequestsForSrvOpen(SrvCall, SrvOpen, &Discarded);

    RxReleaseBufferingManagerMutex(&SrvCall->BufferingManager);

    /* If there were discarded requests */
    if (!IsListEmpty(&Discarded))
    {
        /* And a pending buffering state change */
        if (BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_BUFFERING_STATE_CHANGE_PENDING))
        {
            /* Clear the flag, and set the associated event - job done */
            RxAcquireSerializationMutex();
            ClearFlag(SrvOpen->Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
            if (SrvOpen->Fcb->pBufferingStateChangeCompletedEvent != NULL)
            {
                KeSetEvent(SrvOpen->Fcb->pBufferingStateChangeCompletedEvent, IO_NETWORK_INCREMENT, FALSE);
            }
            RxReleaseSerializationMutex();
        }

        /* Drop the discarded requests */
        RxpDiscardChangeBufferingStateRequests(&Discarded);
    }
}

/*
 * @implemented
 */
VOID
RxPurgeFcb(
    IN  PFCB Fcb)
{
    PAGED_CODE();

    ASSERT(RxIsFcbAcquiredExclusive(Fcb));

    /* Reference our FCB so that it doesn't disappear */
    RxReferenceNetFcb(Fcb);
    /* Purge Cc if required */
    if (Fcb->OpenCount != 0)
    {
        RxPurgeFcbInSystemCache(Fcb, NULL, 0, TRUE, TRUE);
    }

    /* If it wasn't freed, release the lock */
    if (!RxDereferenceAndFinalizeNetFcb(Fcb, NULL, FALSE, FALSE))
    {
        RxReleaseFcb(NULL, Fcb);
    }
}

/*
 * @implemented
 */
NTSTATUS
RxPurgeFcbInSystemCache(
    IN PFCB Fcb,
    IN PLARGE_INTEGER FileOffset OPTIONAL,
    IN ULONG Length,
    IN BOOLEAN UninitializeCacheMaps,
    IN BOOLEAN FlushFile)
{
    BOOLEAN Purged;
    NTSTATUS Status;

    PAGED_CODE();

    ASSERT(RxIsFcbAcquiredExclusive(Fcb));

    /* Try to flush first, if asked */
    if (FlushFile)
    {
        /* If flushing failed, just make some noise */
        Status = RxFlushFcbInSystemCache(Fcb, TRUE);
        if (!NT_SUCCESS(Status))
        {
            PVOID CallersAddress, CallersCaller;

            RtlGetCallersAddress(&CallersAddress, &CallersCaller);
            DPRINT1("Flush failed with status %lx for FCB %p\n", Status, Fcb);
            DPRINT1("Caller was %p %p\n", CallersAddress, CallersCaller);
        }
    }

    /* Deal with Cc for purge */
    Purged = CcPurgeCacheSection(&Fcb->NonPaged->SectionObjectPointers, FileOffset,
                                 Length, UninitializeCacheMaps);
    /* If purge failed, force section closing */
    if (!Purged)
    {
        MmFlushImageSection(&Fcb->NonPaged->SectionObjectPointers, MmFlushForWrite);

        RxReleaseFcb(NULL, Fcb);
        Purged = MmForceSectionClosed(&Fcb->NonPaged->SectionObjectPointers, TRUE);
        RxAcquireExclusiveFcb(NULL, Fcb);
    }

    /* Return appropriate status */
    Status = (Purged ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    DPRINT("Purge for FCB %p returns %lx\n", Fcb, Status);

    return Status;
}

/*
 * @implemented
 */
BOOLEAN
RxPurgeFobx(
    PFOBX pFobx)
{
    NTSTATUS Status;
    PFCB FcbToBePurged;

    PAGED_CODE();

    /* Get the associated FCB */
    FcbToBePurged = (PFCB)pFobx->pSrvOpen->pFcb;
    Status = RxAcquireExclusiveFcb(NULL, FcbToBePurged);
    ASSERT(Status == STATUS_SUCCESS);

    /* Purge it */
    Status = RxPurgeFcbInSystemCache(FcbToBePurged, NULL, 0, FALSE, TRUE);
    if (Status != STATUS_SUCCESS)
    {
        DPRINT1("Purge failed for %p (%p)\n", FcbToBePurged, pFobx);
        return FALSE;
    }

    /* And flush */
    if (!MmFlushImageSection(&FcbToBePurged->NonPaged->SectionObjectPointers, MmFlushForWrite))
    {
        DPRINT1("Image section flush failed for %p (%p)\n", FcbToBePurged, pFobx);
        return FALSE;
    }

    DPRINT("Purge OK for %p (%p)\n", FcbToBePurged, pFobx);
    return TRUE;
}

/*
 * @implemented
 */
NTSTATUS
RxPurgeFobxFromCache(
    PFOBX FobxToBePurged)
{
    NTSTATUS Status;
    PFCB FcbToBePurged;

    PAGED_CODE();

    FcbToBePurged = (PFCB)FobxToBePurged->pSrvOpen->pFcb;
    ASSERT(FcbToBePurged != NULL);

    /* If we cannot have our FCB exclusively, give up */
    Status = RxAcquireExclusiveFcb(NULL, FcbToBePurged);
    if (Status != STATUS_SUCCESS)
    {
        RxDereferenceNetFobx(FobxToBePurged, LHS_LockNotHeld);
        return Status;
    }

    /* Don't let the FCB disappear */
    RxReferenceNetFcb(FcbToBePurged);

    /* If the SRV_OPEN was already closed, or if there are unclean FOBX, give up */
    if (BooleanFlagOn(FobxToBePurged->Flags, FOBX_FLAG_SRVOPEN_CLOSED) || FobxToBePurged->pSrvOpen->UncleanFobxCount != 0)
    {
        DPRINT("FCB purge skipped\n");
    }
    else
    {
        Status = RxPurgeFcbInSystemCache(FcbToBePurged, NULL, 0, FALSE, TRUE);
    }

    RxDereferenceNetFobx(FobxToBePurged, LHS_ExclusiveLockHeld);
    /* Drop our extra reference */
    if (!RxDereferenceAndFinalizeNetFcb(FcbToBePurged, NULL, FALSE, FALSE))
    {
        RxReleaseFcb(NULL, FcbToBePurged);
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
RxPurgeRelatedFobxs(
    PNET_ROOT NetRoot,
    PRX_CONTEXT RxContext,
    BOOLEAN AttemptFinalization,
    PFCB PurgingFcb)
{
    PLIST_ENTRY Entry;
    ULONG SuccessfullPurge;
    PRDBSS_SCAVENGER Scavenger;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    PPURGE_SYNCHRONIZATION_CONTEXT PurgeSyncCtx;

    PAGED_CODE();

    RxDeviceObject = RxContext->RxDeviceObject;
    Scavenger = RxDeviceObject->pRdbssScavenger;
    PurgeSyncCtx = &NetRoot->PurgeSyncronizationContext;

    RxAcquireScavengerMutex();

    /* If there's already a purge in progress */
    if (PurgeSyncCtx->PurgeInProgress)
    {
        /* Add our RX_CONTEXT to the current run */
        InsertTailList(&PurgeSyncCtx->ContextsAwaitingPurgeCompletion,
                       &RxContext->RxContextSerializationQLinks);

        /* And wait until it's done */
        RxReleaseScavengerMutex();
        RxWaitSync(RxContext);
        RxAcquireScavengerMutex();
    }

    /* Start the purge */
    PurgeSyncCtx->PurgeInProgress = TRUE;

    /* While the purge is still handling our NET_ROOT, do nothing but wait */
    while (Scavenger->CurrentNetRootForClosePendingProcessing == NetRoot)
    {
        RxReleaseScavengerMutex();
        KeWaitForSingleObject(&Scavenger->ClosePendingProcessingSyncEvent, Executive,
                              KernelMode, TRUE, NULL);
        RxAcquireScavengerMutex();
    }

    /* Now, for all the entries */
    SuccessfullPurge = 0;
    Entry = Scavenger->ClosePendingFobxsList.Flink;
    while (Entry != &Scavenger->ClosePendingFobxsList)
    {
        PFCB Fcb;
        PFOBX Fobx;
        BOOLEAN Success;

        Fobx = CONTAINING_RECORD(Entry, FOBX, ClosePendingList);
        DPRINT("Dealing with FOBX: %p\n", Fobx);

        Entry = Entry->Flink;

        /* If it's not matching our NET_ROOT, ignore */
        if (Fobx->pSrvOpen == NULL ||
            Fobx->pSrvOpen->pFcb == NULL ||
            ((PFCB)Fobx->pSrvOpen->pFcb)->VNetRoot == NULL ||
            (PNET_ROOT)((PFCB)Fobx->pSrvOpen->pFcb)->VNetRoot->pNetRoot != NetRoot)
        {
            continue;
        }

        /* Determine if it matches our FCB */
        Fcb = (PFCB)Fobx->pSrvOpen->pFcb;
        if (PurgingFcb != NULL && NodeType(PurgingFcb) != RDBSS_NTC_STORAGE_TYPE_DIRECTORY &&
            PurgingFcb != Fcb)
        {
            NTSTATUS Status;

            MINIRDR_CALL_THROUGH(Status, RxDeviceObject->Dispatch, MRxAreFilesAliased, (Fcb, PurgingFcb));
            if (Status == STATUS_SUCCESS)
            {
                continue;
            }
        }

        /* Matching, we'll purge it */
        RemoveEntryList(&Fobx->ClosePendingList);

        /* Reference it so that it doesn't disappear */
        RxReferenceNetFobx(Fobx);

        RxReleaseScavengerMutex();

        /* And purge */
        Success = RxPurgeFobx(Fobx);
        if (Success)
        {
            ++SuccessfullPurge;
        }

        /* If we don't have to finalize it (or if we cannot acquire lock exclusively
         * Just normally dereference
         */
        if ((AttemptFinalization == DONT_ATTEMPT_FINALIZE_ON_PURGE) ||
            RxAcquireExclusiveFcb(NULL, Fcb) != STATUS_SUCCESS)
        {
            RxDereferenceNetFobx(Fobx, LHS_LockNotHeld);
        }
        /* Otherwise, finalize */
        else
        {
            RxReferenceNetFcb(Fcb);
            RxDereferenceNetFobx(Fobx, LHS_ExclusiveLockHeld);
            if (!RxDereferenceAndFinalizeNetFcb(Fcb, NULL, FALSE, FALSE))
            {
                RxReleaseFcb(NULL, Fcb);
            }
        }

        if (!Success)
        {
            DPRINT1("Failed purging %p (%p)\n", Fcb, Fobx);
        }

        RxAcquireScavengerMutex();
    }

    /* If no contexts left, purge is not running */
    if (IsListEmpty(&PurgeSyncCtx->ContextsAwaitingPurgeCompletion))
    {
        PurgeSyncCtx->PurgeInProgress = FALSE;
    }
    /* Otherwise, notify a waiter it can start */
    else
    {
        PRX_CONTEXT Context;

        Entry = RemoveHeadList(&PurgeSyncCtx->ContextsAwaitingPurgeCompletion);
        Context = CONTAINING_RECORD(Entry, RX_CONTEXT, RxContextSerializationQLinks);

        RxSignalSynchronousWaiter(Context);
    }

    RxReleaseScavengerMutex();

    return (SuccessfullPurge > 0 ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}

/*
 * @implemented
 */
VOID
RxpWorkerThreadDispatcher(
   IN PRX_WORK_QUEUE WorkQueue,
   IN PLARGE_INTEGER WaitInterval)
{
    NTSTATUS Status;
    PVOID Parameter;
    PETHREAD CurrentThread;
    BOOLEAN KillThread, Dereference;
    PRX_WORK_QUEUE_ITEM WorkQueueItem;
    PWORKER_THREAD_ROUTINE WorkerRoutine;

    InterlockedIncrement(&WorkQueue->NumberOfIdleWorkerThreads);

    /* Reference ourselves */
    CurrentThread = PsGetCurrentThread();
    Status = ObReferenceObjectByPointer(CurrentThread, THREAD_ALL_ACCESS, *PsThreadType, KernelMode);
    ASSERT(NT_SUCCESS(Status));

    /* Infinite loop for worker */
    KillThread = FALSE;
    Dereference = FALSE;
    do
    {
        KIRQL OldIrql;
        PLIST_ENTRY ListEntry;

        /* Remove an entry from the work queue */
        ListEntry = KeRemoveQueue(&WorkQueue->Queue, KernelMode, WaitInterval);
        if ((ULONG_PTR)ListEntry != STATUS_TIMEOUT)
        {
            PRDBSS_DEVICE_OBJECT DeviceObject;

            WorkQueueItem = CONTAINING_RECORD(ListEntry, RX_WORK_QUEUE_ITEM, List);

            InterlockedIncrement(&WorkQueue->NumberOfWorkItemsDispatched);
            InterlockedDecrement(&WorkQueue->NumberOfWorkItemsToBeDispatched);
            InterlockedDecrement(&WorkQueue->NumberOfIdleWorkerThreads);

            /* Get the parameters, and null-them in the struct */
            WorkerRoutine = WorkQueueItem->WorkerRoutine;
            Parameter = WorkQueueItem->Parameter;
            DeviceObject = WorkQueueItem->pDeviceObject;

            WorkQueueItem->List.Flink = NULL;
            WorkQueueItem->WorkerRoutine = NULL;
            WorkQueueItem->Parameter = NULL;
            WorkQueueItem->pDeviceObject = NULL;

            /* Call the routine */
            DPRINT("Calling: %p(%p)\n", WorkerRoutine, Parameter);
            WorkerRoutine(Parameter);

            /* Are we going down now? */
            if (InterlockedDecrement(&DeviceObject->DispatcherContext.NumberOfWorkerThreads) == 0)
            {
                PKEVENT TearDownEvent;

                TearDownEvent = InterlockedExchangePointer((void * volatile*)&DeviceObject->DispatcherContext.pTearDownEvent, NULL);
                if (TearDownEvent != NULL)
                {
                    KeSetEvent(TearDownEvent, IO_NO_INCREMENT, FALSE);
                }
            }

            InterlockedIncrement(&WorkQueue->NumberOfIdleWorkerThreads);
        }

        /* Shall we shutdown... */
        KeAcquireSpinLock(&WorkQueue->SpinLock, &OldIrql);
        switch (WorkQueue->State)
        {
            /* Our queue is active, kill it if we have no more items to dispatch
             * and more threads than the required minimum
             */
            case RxWorkQueueActive:
                if (WorkQueue->NumberOfWorkItemsToBeDispatched <= 0)
                {
                    ASSERT(WorkQueue->NumberOfActiveWorkerThreads > 0);
                    if (WorkQueue->NumberOfActiveWorkerThreads > WorkQueue->MinimumNumberOfWorkerThreads)
                    {
                        KillThread = TRUE;
                        Dereference = TRUE;
                        InterlockedDecrement(&WorkQueue->NumberOfActiveWorkerThreads);
                    }

                    if (KillThread)
                    {
                        InterlockedDecrement(&WorkQueue->NumberOfIdleWorkerThreads);
                    }
                }
                break;

            /* The queue is inactive: kill it we have more threads than the required minimum */
            case RxWorkQueueInactive:
                ASSERT(WorkQueue->NumberOfActiveWorkerThreads > 0);
                if (WorkQueue->NumberOfActiveWorkerThreads > WorkQueue->MinimumNumberOfWorkerThreads)
                {
                    KillThread = TRUE;
                    Dereference = TRUE;
                    InterlockedDecrement(&WorkQueue->NumberOfActiveWorkerThreads);
                }

                if (KillThread)
                {
                    InterlockedDecrement(&WorkQueue->NumberOfIdleWorkerThreads);
                }
                break;

            /* Rundown in progress..., kill it for sure! */
            case RxWorkQueueRundownInProgress:
                {
                    PRX_WORK_QUEUE_RUNDOWN_CONTEXT RundownContext;

                    ASSERT(WorkQueue->pRundownContext != NULL);

                    RundownContext = WorkQueue->pRundownContext;
                    RundownContext->ThreadPointers[RundownContext->NumberOfThreadsSpunDown++] = CurrentThread;

                    InterlockedDecrement(&WorkQueue->NumberOfActiveWorkerThreads);
                    KillThread = TRUE;
                    Dereference = FALSE;

                    if (WorkQueue->NumberOfActiveWorkerThreads == 0)
                    {
                        KeSetEvent(&RundownContext->RundownCompletionEvent, IO_NO_INCREMENT, FALSE);
                    }

                    InterlockedDecrement(&WorkQueue->NumberOfIdleWorkerThreads);
                }
                break;

            default:
                break;
        }
        KeReleaseSpinLock(&WorkQueue->SpinLock, OldIrql);
    } while (!KillThread);

    DPRINT("Killed worker thread\n");

    /* Do we have to dereference ourselves? */
    if (Dereference)
    {
        ObDereferenceObject(CurrentThread);
    }

    /* Dump last executed routine */
    if (DumpDispatchRoutine)
    {
        DPRINT("Dispatch routine %p(%p) taken from %p\n", WorkerRoutine, Parameter, WorkQueueItem);
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID
RxReference(
    IN OUT PVOID Instance)
{
    NODE_TYPE_CODE NodeType;
    PNODE_TYPE_AND_SIZE Node;

    PAGED_CODE();

    RxAcquireScavengerMutex();

    /* We can only reference a few structs */
    NodeType = NodeType(Instance) & ~RX_SCAVENGER_MASK;
    ASSERT((NodeType == RDBSS_NTC_SRVCALL) || (NodeType == RDBSS_NTC_NETROOT) ||
           (NodeType == RDBSS_NTC_V_NETROOT) || (NodeType == RDBSS_NTC_SRVOPEN) ||
           (NodeType == RDBSS_NTC_FOBX));

    Node = (PNODE_TYPE_AND_SIZE)Instance;
    InterlockedIncrement((volatile long *)&Node->NodeReferenceCount);

    /* Trace refcount if asked */
    switch (NodeType)
    {
        case RDBSS_NTC_SRVCALL:
            PRINT_REF_COUNT(SRVCALL, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_NETROOT:
            PRINT_REF_COUNT(NETROOT, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_V_NETROOT:
            PRINT_REF_COUNT(VNETROOT, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_SRVOPEN:
            PRINT_REF_COUNT(SRVOPEN, Node->NodeReferenceCount);
            break;

        case RDBSS_NTC_FOBX:
            PRINT_REF_COUNT(NETFOBX, Node->NodeReferenceCount);
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    RxpUndoScavengerFinalizationMarking(Instance);
    RxReleaseScavengerMutex();
}

/*
 * @implemented
 */
VOID
NTAPI
RxReinitializeContext(
   IN OUT PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;
    ULONG InitialContextFlags, SavedFlags;

    PAGED_CODE();

    /* Backup a few flags */
    Irp = RxContext->CurrentIrp;
    RxDeviceObject = RxContext->RxDeviceObject;
    SavedFlags = RxContext->Flags & RX_CONTEXT_PRESERVED_FLAGS;
    InitialContextFlags = RxContext->Flags & RX_CONTEXT_INITIALIZATION_FLAGS;

    /* Reset our context */
    RxPrepareContextForReuse(RxContext);

    /* Zero everything */
    RtlZeroMemory(&RxContext->MajorFunction, sizeof(RX_CONTEXT) - FIELD_OFFSET(RX_CONTEXT, MajorFunction));

    /* Restore saved flags */
    RxContext->Flags = SavedFlags;
    /* And reinit the context */
    RxInitializeContext(Irp, RxDeviceObject, InitialContextFlags, RxContext);
}

/*
 * @implemented
 */
VOID
NTAPI
RxReleaseFcbFromLazyWrite(
    PVOID Context)
{
    PFCB Fcb;

    PAGED_CODE();

    Fcb = Context;
    /* The received context is a FCB */
    ASSERT(NodeType(Fcb) == RDBSS_NTC_FCB);
    ASSERT_CORRECT_FCB_STRUCTURE(Fcb);

    /* Lazy writer is releasing lock, so forget about it */
    Fcb->Specific.Fcb.LazyWriteThread = NULL;

    /* If we were top level IRP, unwind */
    if (RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP)
    {
        RxUnwindTopLevelIrp(NULL);
    }

    /* And finally, release the lock */
    Fcb->PagingIoResourceFile = NULL;
    Fcb->PagingIoResourceLine = 0;
    ExReleaseResourceLite(Fcb->Header.PagingIoResource);
}

/*
 * @implemented
 */
VOID
NTAPI
RxReleaseFcbFromReadAhead(
    PVOID Context)
{
    PFCB Fcb;

    PAGED_CODE();

    Fcb = Context;
    /* The received context is a FCB */
    ASSERT(NodeType(Fcb) == RDBSS_NTC_FCB);
    ASSERT_CORRECT_FCB_STRUCTURE(Fcb);

    /* Top Level IRP is CC */
    ASSERT(RxGetTopIrpIfRdbssIrp() == (PIRP)FSRTL_CACHE_TOP_LEVEL_IRP);
    RxUnwindTopLevelIrp(NULL);

    ExReleaseResourceLite(Fcb->Header.Resource);
}

VOID
NTAPI
RxReleaseFileForNtCreateSection(
    PFILE_OBJECT FileObject)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
RxReleaseForCcFlush(
    PFILE_OBJECT FileObject,
    PDEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxRemoveNameNetFcb(
    OUT PFCB ThisFcb)
{
    PNET_ROOT NetRoot;

    PAGED_CODE();

    ASSERT(NodeTypeIsFcb(ThisFcb));

    /* Just remove the entry from the FCB_TABLE */
    NetRoot = (PNET_ROOT)ThisFcb->VNetRoot->pNetRoot;
    ASSERT(RxIsFcbTableLockExclusive(&NetRoot->FcbTable));
    ASSERT(RxIsFcbAcquiredExclusive(ThisFcb));

#ifdef __REACTOS__
    if (!BooleanFlagOn(ThisFcb->FcbState, FCB_STATE_NAME_ALREADY_REMOVED))
    {
#endif
        RxFcbTableRemoveFcb(&NetRoot->FcbTable, ThisFcb);
        DPRINT("FCB (%p) %wZ removed\n", ThisFcb, &ThisFcb->FcbTableEntry.Path);
        /* Mark, so that we don't try to do it twice */
        SetFlag(ThisFcb->FcbState, FCB_STATE_NAME_ALREADY_REMOVED);
#ifdef __REACTOS__
    }
#endif
}

/*
 * @implemented
 */
VOID
RxRemoveOperationFromBlockingQueue(
    IN OUT PRX_CONTEXT RxContext)
{
    /* Acquire the pipe mutex */
    ExAcquireFastMutex(&RxContextPerFileSerializationMutex);

    /* Is that a blocking serial operation? */
    if (BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION))
    {
        /* Clear it! */
        ClearFlag(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION);

        /* Drop it off the list */
        RemoveEntryList(&RxContext->RxContextSerializationQLinks);
        RxContext->RxContextSerializationQLinks.Flink = NULL;
        RxContext->RxContextSerializationQLinks.Blink = NULL;
    }

    /* Done */
    ExReleaseFastMutex(&RxContextPerFileSerializationMutex);
}

/*
 * @implemented
 */
VOID
RxRemovePrefixTableEntry(
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY Entry)
{
    PAGED_CODE();

    ASSERT(NodeType(Entry) == RDBSS_NTC_PREFIX_ENTRY);
    ASSERT(RxIsPrefixTableLockExclusive(ThisTable));

    /* Check whether we're asked to remove null entry */
    if (Entry->Prefix.Length == 0)
    {
        ThisTable->TableEntryForNull = NULL;
    }
    else
    {
        RemoveEntryList(&Entry->HashLinks);
    }

    Entry->ContainingRecord = NULL;

    /* Also remove it from global list */
    RemoveEntryList(&Entry->MemberQLinks);

    ++ThisTable->Version;
}

/*
 * @implemented
 */
VOID
RxRemoveVirtualNetRootFromNetRoot(
    PNET_ROOT NetRoot,
    PV_NET_ROOT VNetRoot)
{
    PRX_PREFIX_TABLE PrefixTable;

    PAGED_CODE();

    PrefixTable = NetRoot->pSrvCall->RxDeviceObject->pRxNetNameTable;
    ASSERT(RxIsPrefixTableLockAcquired(PrefixTable));

    /* Remove the VNetRoot from the list in the NetRoot */
    --NetRoot->NumberOfVirtualNetRoots;
    RemoveEntryList(&VNetRoot->NetRootListEntry);

    /* Fix the NetRoot if we were the default VNetRoot */
    if (NetRoot->DefaultVNetRoot == VNetRoot)
    {
        /* Put the first one available */
        if (!IsListEmpty(&NetRoot->VirtualNetRoots))
        {
            NetRoot->DefaultVNetRoot = CONTAINING_RECORD(NetRoot->VirtualNetRoots.Flink, V_NET_ROOT, NetRootListEntry);
        }
        /* Otherwise, none */
        else
        {
            NetRoot->DefaultVNetRoot = NULL;
        }
    }

    /* If there are still other VNetRoot available, we're done */
    if (!IsListEmpty(&NetRoot->VirtualNetRoots))
    {
        return;
    }

    /* Otherwise, initiate NetRoot finalization */
    if (!BooleanFlagOn(NetRoot->Flags, NETROOT_FLAG_NAME_ALREADY_REMOVED))
    {
        RxRemovePrefixTableEntry(PrefixTable, &NetRoot->PrefixEntry);
        SetFlag(NetRoot->Flags, NETROOT_FLAG_NAME_ALREADY_REMOVED);
    }

    /* Notify mini-rdr */
    if (NetRoot->pSrvCall != NULL && NetRoot->pSrvCall->RxDeviceObject != NULL)
    {
        NTSTATUS Status;

        MINIRDR_CALL_THROUGH(Status, NetRoot->pSrvCall->RxDeviceObject->Dispatch,
                             MRxFinalizeNetRoot, ((PMRX_NET_ROOT)NetRoot, FALSE));
        (void)Status;
    }
}

VOID
RxResumeBlockedOperations_ALL(
    IN OUT PRX_CONTEXT RxContext)
{
    LIST_ENTRY BlockedOps;

    PAGED_CODE();

    /* Get the blocked operations */
    RxTransferListWithMutex(&BlockedOps, &RxContext->BlockedOperations, RxContext->BlockedOpsMutex);

    if (!IsListEmpty(&BlockedOps))
    {
        UNIMPLEMENTED;
    }
}

VOID
NTAPI
RxResumeBlockedOperations_Serially(
    IN OUT PRX_CONTEXT RxContext,
    IN OUT PLIST_ENTRY BlockingIoQ)
{
    PAGED_CODE();

    RxAcquireSerializationMutex();

    /* This can only happen on pipes */
    if (!BooleanFlagOn(RxContext->FlagsForLowIo, RXCONTEXT_FLAG4LOWIO_PIPE_SYNC_OPERATION))
    {
        RxReleaseSerializationMutex();
        return;
    }

    UNIMPLEMENTED;

    RxReleaseSerializationMutex();
}

/*
 * @implemented
 */
VOID
RxSetFileSizeWithLock(
    IN OUT PFCB Fcb,
    IN PLONGLONG FileSize)
{
    PAGED_CODE();

    /* Set attribute and increase version */
    Fcb->Header.FileSize.QuadPart = *FileSize;
    ++Fcb->ulFileSizeVersion;
}

/*
 * @implemented
 */
VOID
RxScavengeFobxsForNetRoot(
    PNET_ROOT NetRoot,
    PFCB PurgingFcb,
    BOOLEAN SynchronizeWithScavenger)
{
    PRDBSS_SCAVENGER Scavenger;
    PRDBSS_DEVICE_OBJECT RxDeviceObject;

    PAGED_CODE();

    RxDeviceObject = NetRoot->pSrvCall->RxDeviceObject;
    Scavenger = RxDeviceObject->pRdbssScavenger;

    /* Wait for the scavenger, if asked to */
    if (SynchronizeWithScavenger)
    {
        KeWaitForSingleObject(&Scavenger->ScavengeEvent, Executive, KernelMode, FALSE, NULL);
    }

    RxAcquireScavengerMutex();

    /* If there's nothing left to do... */
    if (Scavenger->FobxsToBeFinalized <= 0)
    {
        RxReleaseScavengerMutex();
    }
    else
    {
        PLIST_ENTRY Entry;
        LIST_ENTRY FobxToScavenge;

        InitializeListHead(&FobxToScavenge);

        /* Browse all the FOBXs to finalize */
        Entry = Scavenger->FobxFinalizationList.Flink;
        while (Entry != &Scavenger->FobxFinalizationList)
        {
            PFOBX Fobx;

            Fobx = CONTAINING_RECORD(Entry, FOBX, ScavengerFinalizationList);
            Entry = Entry->Flink;

            if (Fobx->SrvOpen != NULL)
            {
                PFCB Fcb;

                Fcb = (PFCB)Fobx->SrvOpen->pFcb;

                /* If it matches our NET_ROOT */
                if ((PNET_ROOT)Fcb->pNetRoot == NetRoot)
                {
                    NTSTATUS Status;

                    /* Check whether it matches our FCB */
                    Status = STATUS_MORE_PROCESSING_REQUIRED;
                    if (PurgingFcb != NULL && PurgingFcb != Fcb)
                    {
                        MINIRDR_CALL_THROUGH(Status, RxDeviceObject->Dispatch, MRxAreFilesAliased, (Fcb, PurgingFcb));
                    }

                    /* If so, add it to the list of the FOBXs to scavenge */
                    if (Status != STATUS_SUCCESS)
                    {
                        RxReferenceNetFobx(Fobx);
                        ASSERT(NodeType(Fobx) == RDBSS_NTC_FOBX);

                        RemoveEntryList(&Fobx->ScavengerFinalizationList);
                        InsertTailList(&FobxToScavenge, &Fobx->ScavengerFinalizationList);
                    }
                }
            }
        }

        RxReleaseScavengerMutex();

        /* Now, scavenge all the extracted FOBX */
        RxpScavengeFobxs(Scavenger, &FobxToScavenge);
    }

    if (SynchronizeWithScavenger)
    {
        KeSetEvent(&Scavenger->ScavengeEvent, IO_NO_INCREMENT, FALSE);
    }
}

/*
 * @implemented
 */
BOOLEAN
RxScavengeRelatedFobxs(
    PFCB Fcb)
{
    PFOBX Fobx;
    LIST_ENTRY LocalList;
    PLIST_ENTRY NextEntry;
    PRDBSS_SCAVENGER Scavenger;

    PAGED_CODE();

    /* First of all, check whether there are FOBX to scavenge */
    Scavenger = Fcb->RxDeviceObject->pRdbssScavenger;
    RxAcquireScavengerMutex();
    if (Scavenger->FobxsToBeFinalized <= 0)
    {
        RxReleaseScavengerMutex();
        return FALSE;
    }

    /* Initialize our local list which will hold all the FOBX to scavenge so
     * that we don't acquire the scavenger mutex too long
     */
    InitializeListHead(&LocalList);

    /* Technically, that condition should all be true... */
    if (!IsListEmpty(&Scavenger->FobxFinalizationList))
    {
        PLIST_ENTRY NextEntry, LastEntry;

        /* Browse all the FCBs to find the matching ones */
        NextEntry = Scavenger->FobxFinalizationList.Flink;
        LastEntry = &Scavenger->FobxFinalizationList;
        while (NextEntry != LastEntry)
        {
            Fobx = CONTAINING_RECORD(NextEntry, FOBX, ScavengerFinalizationList);
            NextEntry = NextEntry->Flink;
            /* Matching our FCB? Let's finalize it */
            if (Fobx->pSrvOpen != NULL && Fobx->pSrvOpen->pFcb == RX_GET_MRX_FCB(Fcb))
            {
                RxpUndoScavengerFinalizationMarking(Fobx);
                ASSERT(NodeType(Fobx) == RDBSS_NTC_FOBX);
                InsertTailList(&LocalList, &Fobx->ScavengerFinalizationList);
            }
        }
    }

    RxReleaseScavengerMutex();

    /* Nothing to scavenge? Quit */
    if (IsListEmpty(&LocalList))
    {
        return FALSE;
    }

    /* Now, finalize all the extracted FOBX */
    while (!IsListEmpty(&LocalList))
    {
        NextEntry = RemoveHeadList(&LocalList);
        Fobx = CONTAINING_RECORD(NextEntry, FOBX, ScavengerFinalizationList);
        RxFinalizeNetFobx(Fobx, TRUE, TRUE);
    }

    return TRUE;
}

VOID
RxScavengerFinalizeEntries(
    PRDBSS_DEVICE_OBJECT DeviceObject)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
RxScavengerTimerRoutine(
    PVOID Context)
{
    BOOLEAN Requeue;
    PRDBSS_DEVICE_OBJECT DeviceObject;
    PRDBSS_SCAVENGER Scavenger;

    PAGED_CODE();

    DeviceObject = Context;
    Scavenger = DeviceObject->pRdbssScavenger;

    Requeue = FALSE;
    RxAcquireScavengerMutex();
    /* If the scavenger was dormant, wake it up! */
    if (Scavenger->State == RDBSS_SCAVENGER_DORMANT)
    {
        /* Done */
        Scavenger->State = RDBSS_SCAVENGER_ACTIVE;
        KeClearEvent(&Scavenger->ScavengeEvent);

        /* Scavenger the entries */
        RxReleaseScavengerMutex();
        RxScavengerFinalizeEntries(DeviceObject);
        RxAcquireScavengerMutex();

        /* If we're still active (race) */
        if (Scavenger->State == RDBSS_SCAVENGER_ACTIVE)
        {
            /* If there are new entries to scavenge, stay dormant and requeue a run */
            if (Scavenger->NumberOfDormantFiles + Scavenger->SrvCallsToBeFinalized +
                Scavenger->NetRootsToBeFinalized + Scavenger->VNetRootsToBeFinalized +
                Scavenger->FcbsToBeFinalized + Scavenger->SrvOpensToBeFinalized +
                Scavenger->FobxsToBeFinalized != 0)
            {
                Requeue = TRUE;
                Scavenger->State = RDBSS_SCAVENGER_DORMANT;
            }
            /* Otherwise, we're inactive again */
            else
            {
                Scavenger->State = RDBSS_SCAVENGER_INACTIVE;
            }
        }

        RxReleaseScavengerMutex();

        /* Requeue an execution */
        if (Requeue)
        {
            RxPostOneShotTimerRequest(RxFileSystemDeviceObject, &Scavenger->WorkItem,
                                      RxScavengerTimerRoutine, DeviceObject, Scavenger->TimeLimit);
        }
    }
    else
    {
        RxReleaseScavengerMutex();
    }

    KeSetEvent(&Scavenger->ScavengeEvent, IO_NO_INCREMENT, FALSE);
}

BOOLEAN
RxScavengeVNetRoots(
    PRDBSS_DEVICE_OBJECT RxDeviceObject)
{
    UNIMPLEMENTED;
    return FALSE;
}

/*
 * @implemented
 */
VOID
NTAPI
RxSpinUpRequestsDispatcher(
    PVOID Dispatcher)
{
    NTSTATUS Status;
    PRX_DISPATCHER RxDispatcher;

    Status = ObReferenceObjectByPointer(PsGetCurrentThread(), THREAD_ALL_ACCESS, *PsThreadType, KernelMode);
    if (!NT_SUCCESS(Status))
    {
        PsTerminateSystemThread(STATUS_SUCCESS);
    }

    RxDispatcher = Dispatcher;

    do
    {
        KIRQL OldIrql;
        PLIST_ENTRY ListEntry;

        Status = KeWaitForSingleObject(&RxDispatcher->SpinUpRequestsEvent, Executive,
                                       KernelMode, FALSE, &RxSpinUpDispatcherWaitInterval);
        ASSERT((Status == STATUS_SUCCESS) || (Status == STATUS_TIMEOUT));

        KeAcquireSpinLock(&RxDispatcher->SpinUpRequestsLock, &OldIrql);
        if (!IsListEmpty(&RxDispatcher->SpinUpRequests))
        {
            ListEntry = RemoveHeadList(&RxDispatcher->SpinUpRequests);
        }
        else
        {
            ListEntry = &RxDispatcher->SpinUpRequests;
        }
        KeClearEvent(&RxDispatcher->SpinUpRequestsEvent);
        KeReleaseSpinLock(&RxDispatcher->SpinUpRequestsLock, OldIrql);

        while (ListEntry != &RxDispatcher->SpinUpRequests)
        {
            PWORK_QUEUE_ITEM WorkItem;
            PRX_WORK_QUEUE WorkQueue;

            WorkItem = CONTAINING_RECORD(ListEntry, WORK_QUEUE_ITEM, List);
            WorkQueue = WorkItem->Parameter;

            InterlockedDecrement(&WorkQueue->WorkQueueItemForSpinUpWorkerThreadInUse);

            DPRINT("Workqueue: calling %p(%p)\n", WorkItem->WorkerRoutine, WorkItem->Parameter);
            WorkItem->WorkerRoutine(WorkItem->Parameter);
        }
    } while (RxDispatcher->State == RxDispatcherActive);

    KeSetEvent(&RxDispatcher->SpinUpRequestsTearDownEvent, IO_NO_INCREMENT, FALSE);
    PsTerminateSystemThread(STATUS_SUCCESS);
}

/*
 * @implemented
 */
NTSTATUS
RxSpinUpWorkerThread(
   PRX_WORK_QUEUE WorkQueue,
   PRX_WORKERTHREAD_ROUTINE Routine,
   PVOID Parameter)
{
    KIRQL OldIrql;
    NTSTATUS Status;
    HANDLE ThreadHandle;

    PAGED_CODE();

    /* If work queue is inactive, that cannot work */
    KeAcquireSpinLock(&WorkQueue->SpinLock, &OldIrql);
    if (WorkQueue->State != RxWorkQueueActive)
    {
        Status = STATUS_UNSUCCESSFUL;
        DPRINT("Workqueue not active! WorkQ: %p, State: %d, Active: %d\n", WorkQueue, WorkQueue->State, WorkQueue->NumberOfActiveWorkerThreads);
    }
    else
    {
        ++WorkQueue->NumberOfActiveWorkerThreads;
        Status = STATUS_SUCCESS;
    }
    KeReleaseSpinLock(&WorkQueue->SpinLock, OldIrql);

    /* Quit on failure */
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Spin up the worker thread */
    Status = PsCreateSystemThread(&ThreadHandle, PROCESS_ALL_ACCESS, NULL, NULL, NULL, Routine, Parameter);
    if (NT_SUCCESS(Status))
    {
        ZwClose(ThreadHandle);
        return Status;
    }
    /* Read well: we reached that point because it failed! */
    DPRINT("WorkQ: %p, Status: %lx\n", WorkQueue, Status);

    KeAcquireSpinLock(&WorkQueue->SpinLock, &OldIrql);
    --WorkQueue->NumberOfActiveWorkerThreads;
    ++WorkQueue->NumberOfFailedSpinUpRequests;

    /* Rundown, no more active threads, set the event! */
    if (WorkQueue->NumberOfActiveWorkerThreads == 0 &&
        WorkQueue->State == RxWorkQueueRundownInProgress)
    {
        KeSetEvent(&WorkQueue->pRundownContext->RundownCompletionEvent, IO_NO_INCREMENT, FALSE);
    }

    DPRINT("Workqueue not active! WorkQ: %p, State: %d, Active: %d\n", WorkQueue, WorkQueue->State, WorkQueue->NumberOfActiveWorkerThreads);

    KeReleaseSpinLock(&WorkQueue->SpinLock, OldIrql);

    return Status;
}

VOID
RxSpinUpWorkerThreads(
   PRX_WORK_QUEUE WorkQueue)
{
    UNIMPLEMENTED;
}

VOID
RxSynchronizeWithScavenger(
    IN PRX_CONTEXT RxContext)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
ULONG
RxTableComputeHashValue(
    IN PUNICODE_STRING Name)
{
    ULONG Hash;
    SHORT Loops[8];
    USHORT MaxChar, i;

    PAGED_CODE();

    MaxChar = Name->Length / sizeof(WCHAR);

    Loops[0] = 1;
    Loops[1] = MaxChar - 1;
    Loops[2] = MaxChar - 2;
    Loops[3] = MaxChar - 3;
    Loops[4] = MaxChar - 4;
    Loops[5] = MaxChar / 4;
    Loops[6] = 2 * MaxChar / 4;
    Loops[7] = 3 * MaxChar / 4;

    Hash = 0;
    for (i = 0; i < 8; ++i)
    {
        SHORT Idx;

        Idx = Loops[i];
        if (Idx >= 0 && Idx < MaxChar)
        {
            Hash = RtlUpcaseUnicodeChar(Name->Buffer[Idx]) + 8 * Hash;
        }
    }

    return Hash;
}

/*
 * @implemented
 */
ULONG
RxTableComputePathHashValue(
    IN PUNICODE_STRING Name)
{
    ULONG Hash;
    SHORT Loops[8];
    USHORT MaxChar, i;

    PAGED_CODE();

    MaxChar = Name->Length / sizeof(WCHAR);

    Loops[0] = 1;
    Loops[1] = MaxChar - 1;
    Loops[2] = MaxChar - 2;
    Loops[3] = MaxChar - 3;
    Loops[4] = MaxChar - 4;
    Loops[5] = MaxChar / 4;
    Loops[6] = 2 * MaxChar / 4;
    Loops[7] = 3 * MaxChar / 4;

    Hash = 0;
    for (i = 0; i < 8; ++i)
    {
        SHORT Idx;

        Idx = Loops[i];
        if (Idx >= 0 && Idx < MaxChar)
        {
            Hash = RtlUpcaseUnicodeChar(Name->Buffer[Idx]) + 8 * Hash;
        }
    }

    return Hash;
}

/*
 * @implemented
 */
PVOID
RxTableLookupName(
    IN PRX_PREFIX_TABLE ThisTable,
    IN PUNICODE_STRING Name,
    OUT PUNICODE_STRING RemainingName,
    IN PRX_CONNECTION_ID OPTIONAL RxConnectionId)
{
    PVOID Container;
    USHORT i, MaxChar;
    PRX_PREFIX_ENTRY Entry;
    RX_CONNECTION_ID NullId;
    UNICODE_STRING LookupString;

    PAGED_CODE();

    /* If caller didn't provide a connection ID, setup one */
    if (ThisTable->IsNetNameTable && RxConnectionId == NULL)
    {
        NullId.Luid.LowPart = 0;
        NullId.Luid.HighPart = 0;
        RxConnectionId = &NullId;
    }

    /* Validate name */
    ASSERT(Name->Buffer[0] == OBJ_NAME_PATH_SEPARATOR);

    Entry = NULL;
    Container = NULL;
    LookupString.Buffer = Name->Buffer;
    MaxChar = Name->Length / sizeof(WCHAR);
    /* We'll perform the lookup, path component after another */
    for (i = 1; i < MaxChar; ++i)
    {
        ULONG Hash;
        PRX_PREFIX_ENTRY CurEntry;

        /* Don't cut in the middle of a path element */
        if (Name->Buffer[i] != OBJ_NAME_PATH_SEPARATOR && Name->Buffer[i] != ':')
        {
            continue;
        }

        /* Perform lookup in the table */
        LookupString.Length = i * sizeof(WCHAR);
        Hash = RxTableComputeHashValue(&LookupString);
        CurEntry = RxTableLookupName_ExactLengthMatch(ThisTable, &LookupString, Hash, RxConnectionId);
#if DBG
        ++ThisTable->Lookups;
#endif
        /* Entry not found, move to the next component */
        if (CurEntry == NULL)
        {
#if DBG
            ++ThisTable->FailedLookups;
#endif
            continue;
        }

        Entry = CurEntry;
        ASSERT(Entry->ContainingRecord != NULL);
        Container = Entry->ContainingRecord;

        /* If we have a NET_ROOT, let's return a V_NET_ROOT */
        if ((NodeType(Entry->ContainingRecord) & ~RX_SCAVENGER_MASK) == RDBSS_NTC_NETROOT)
        {
            PNET_ROOT NetRoot;

            NetRoot = (PNET_ROOT)Entry->ContainingRecord;
            /* If there's a default one, perfect, that's a match */
            if (NetRoot->DefaultVNetRoot != NULL)
            {
                Container = NetRoot->DefaultVNetRoot;
            }
            /* If none (that shouldn't happen!), try to find one */
            else
            {
                /* Use the first one in the list */
                if (!IsListEmpty(&NetRoot->VirtualNetRoots))
                {
                    Container = CONTAINING_RECORD(NetRoot->VirtualNetRoots.Flink, V_NET_ROOT, NetRootListEntry);
                }
                /* Really, really, shouldn't happen */
                else
                {
                    ASSERT(FALSE);
                    Entry = NULL;
                    Container = NULL;
                }
            }

            break;
        }
        else if ((NodeType(Entry->ContainingRecord) & ~RX_SCAVENGER_MASK) == RDBSS_NTC_V_NETROOT)
        {
            break;
        }
        else
        {
            ASSERT((NodeType(Entry->ContainingRecord) & ~RX_SCAVENGER_MASK) == RDBSS_NTC_SRVCALL);
        }
    }

    /* Entry was found */
    if (Entry != NULL)
    {
        DPRINT("Found\n");

        ASSERT(Name->Length >= Entry->Prefix.Length);

        /* Setup remaining name */
        RemainingName->Buffer = Add2Ptr(Name->Buffer, Entry->Prefix.Length);
        RemainingName->Length = Name->Length - Entry->Prefix.Length;
        RemainingName->MaximumLength = Name->Length - Entry->Prefix.Length;
    }
    else
    {
        /* Otherwise, that's the whole name */
        RemainingName = Name;
    }

    return Container;
}

/*
 * @implemented
 */
PRX_PREFIX_ENTRY
RxTableLookupName_ExactLengthMatch(
    IN PRX_PREFIX_TABLE ThisTable,
    IN PUNICODE_STRING  Name,
    IN ULONG HashValue,
    IN PRX_CONNECTION_ID OPTIONAL RxConnectionId)
{
    PLIST_ENTRY ListEntry, HashBucket;

    PAGED_CODE();

    ASSERT(RxConnectionId != NULL);

    /* Select the right bucket */
    HashBucket = HASH_BUCKET(ThisTable, HashValue);
    DPRINT("Looking in bucket: %p for %x\n", HashBucket, HashValue);
    /* If bucket is empty, no match */
    if (IsListEmpty(HashBucket))
    {
        return NULL;
    }

    /* Browse all the entries in the bucket */
    for (ListEntry = HashBucket->Flink;
         ListEntry != HashBucket;
         ListEntry = ListEntry->Flink)
    {
        PVOID Container;
        PRX_PREFIX_ENTRY Entry;
        BOOLEAN CaseInsensitive;
        PUNICODE_STRING CmpName, CmpPrefix;
        UNICODE_STRING InsensitiveName, InsensitivePrefix;

        Entry = CONTAINING_RECORD(ListEntry, RX_PREFIX_ENTRY, HashLinks);
#if DBG
        ++ThisTable->Considers;
#endif
        ASSERT(HashBucket == HASH_BUCKET(ThisTable, Entry->SavedHashValue));

        Container = Entry->ContainingRecord;
        ASSERT(Container != NULL);

        /* Not the same hash, not the same length, move on */
        if (Entry->SavedHashValue != HashValue || Entry->Prefix.Length != Name->Length)
        {
            continue;
        }

#if DBG
        ++ThisTable->Compares;
#endif
        /* If we have to perform a case insensitive compare on a portion... */
        if (Entry->CaseInsensitiveLength != 0)
        {
            ASSERT(Entry->CaseInsensitiveLength <= Name->Length);

            /* Perform the case insensitive check on the asked length */
            InsensitiveName.Buffer = Name->Buffer;
            InsensitivePrefix.Buffer = Entry->Prefix.Buffer;
            InsensitiveName.Length = Entry->CaseInsensitiveLength;
            InsensitivePrefix.Length = Entry->CaseInsensitiveLength;
            /* No match, move to the next entry */
            if (!RtlEqualUnicodeString(&InsensitiveName, &InsensitivePrefix, TRUE))
            {
                continue;
            }

            /* Was the case insensitive covering the whole name? */
            if (Name->Length == Entry->CaseInsensitiveLength)
            {
                /* If connection ID also matches, that a complete match! */
                if (!ThisTable->IsNetNameTable || RxEqualConnectionId(RxConnectionId, &Entry->ConnectionId))
                {
                    return Entry;
                }
            }

            /* Otherwise, we have to continue with the sensitive match.... */
            InsensitiveName.Buffer = Add2Ptr(InsensitiveName.Buffer, Entry->CaseInsensitiveLength);
            InsensitivePrefix.Buffer = Add2Ptr(InsensitivePrefix.Buffer, Entry->CaseInsensitiveLength);
            InsensitiveName.Length = Name->Length - Entry->CaseInsensitiveLength;
            InsensitivePrefix.Length = Entry->Prefix.Length - Entry->CaseInsensitiveLength;

            CmpName = &InsensitiveName;
            CmpPrefix = &InsensitivePrefix;
            CaseInsensitive = FALSE;
        }
        else
        {
            CmpName = Name;
            CmpPrefix = &Entry->Prefix;
            CaseInsensitive = ThisTable->CaseInsensitiveMatch;
        }

        /* Perform the compare, if there's a match, also check for connection ID */
        if (RtlEqualUnicodeString(CmpName, CmpPrefix, CaseInsensitive))
        {
            if (!ThisTable->IsNetNameTable || RxEqualConnectionId(RxConnectionId, &Entry->ConnectionId))
            {
                return Entry;
            }
        }
    }

    return NULL;
}

/*
 * @implemented
 */
NTSTATUS
RxTearDownBufferingManager(
   PSRV_CALL SrvCall)
{
    PAGED_CODE();

    /* Nothing to do */
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
VOID
NTAPI
RxTimerDispatch(
    _In_ struct _KDPC *Dpc,
    _In_opt_ PVOID DeferredContext,
    _In_opt_ PVOID SystemArgument1,
    _In_opt_ PVOID SystemArgument2)
{
    BOOLEAN Set;
    LIST_ENTRY LocalList;
    PLIST_ENTRY ListEntry;
    PRX_WORK_ITEM WorkItem;

    InitializeListHead(&LocalList);

    KeAcquireSpinLockAtDpcLevel(&RxTimerLock);
    ++RxTimerTickCount;

    /* Find any entry matching */
    if (!IsListEmpty(&RxTimerQueueHead))
    {
        ListEntry = RxTimerQueueHead.Flink;
        do
        {
            WorkItem = CONTAINING_RECORD(ListEntry, RX_WORK_ITEM, WorkQueueItem.List);
            if (WorkItem->LastTick == RxTimerTickCount)
            {
                ListEntry = ListEntry->Flink;

                RemoveEntryList(&WorkItem->WorkQueueItem.List);
                InsertTailList(&LocalList, &WorkItem->WorkQueueItem.List);
            }
            else
            {
                ListEntry = ListEntry->Flink;
            }
        } while (ListEntry != &RxTimerQueueHead);
    }
    /* Do we have to requeue a later execution? */
    Set = !IsListEmpty(&RxTimerQueueHead);

    KeReleaseSpinLockFromDpcLevel(&RxTimerLock);

    /* Requeue if list wasn't empty */
    if (Set)
    {
        KeSetTimer(&RxTimer, RxTimerInterval, &RxTimerDpc);
    }

    /* If we had matching entries */
    if (!IsListEmpty(&LocalList))
    {
        /* Post them, one after another */
        ListEntry = LocalList.Flink;
        do
        {
            WorkItem = CONTAINING_RECORD(ListEntry, RX_WORK_ITEM, WorkQueueItem.List);
            ListEntry = ListEntry->Flink;

            WorkItem->WorkQueueItem.List.Flink = NULL;
            WorkItem->WorkQueueItem.List.Blink = NULL;
            RxPostToWorkerThread(WorkItem->WorkQueueItem.pDeviceObject, CriticalWorkQueue,
                                 &WorkItem->WorkQueueItem, WorkItem->WorkQueueItem.WorkerRoutine,
                                 WorkItem->WorkQueueItem.Parameter);
        }
        while (ListEntry != &LocalList);
    }
}

#ifdef RDBSS_TRACKER
/*
 * @implemented
 */
VOID
RxTrackerUpdateHistory(
    _Inout_opt_ PRX_CONTEXT RxContext,
    _Inout_ PMRX_FCB MrxFcb,
    _In_ ULONG Operation,
    _In_ ULONG LineNumber,
    _In_ PCSTR FileName,
    _In_ ULONG SerialNumber)
{
    PFCB Fcb;
    RX_FCBTRACKER_CASES Case;

    /* Check for null or special context */
    if (RxContext == NULL)
    {
        Case = RX_FCBTRACKER_CASE_NULLCONTEXT;
    }
    else if (RxContext == CHANGE_BUFFERING_STATE_CONTEXT)
    {
        Case = RX_FCBTRACKER_CASE_CBS_CONTEXT;
    }
    else if (RxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT)
    {
        Case = RX_FCBTRACKER_CASE_CBS_WAIT_CONTEXT;
    }
    else
    {
        ASSERT(NodeType(RxContext) == RDBSS_NTC_RX_CONTEXT);
        Case = RX_FCBTRACKER_CASE_NORMAL;
    }

    /* If caller provided a FCB, update its history */
    if (MrxFcb != NULL)
    {
        Fcb = (PFCB)MrxFcb;
        ASSERT(NodeTypeIsFcb(Fcb));

        /* Only one acquire operation, so many release operations... */
        if (Operation == TRACKER_ACQUIRE_FCB)
        {
            ++Fcb->FcbAcquires[Case];
        }
        else
        {
            ++Fcb->FcbReleases[Case];
        }
    }

    /* If we have a normal context, update its history about this function calls */
    if (Case == RX_FCBTRACKER_CASE_NORMAL)
    {
        ULONG TrackerHistoryPointer;

        /* Only one acquire operation, so many release operations... */
        if (Operation == TRACKER_ACQUIRE_FCB)
        {
            InterlockedIncrement(&RxContext->AcquireReleaseFcbTrackerX);
        }
        else
        {
            InterlockedDecrement(&RxContext->AcquireReleaseFcbTrackerX);
        }

        /* We only keep track of the 32 first calls */
        TrackerHistoryPointer = InterlockedExchangeAdd((volatile long *)&RxContext->TrackerHistoryPointer, 1);
        if (TrackerHistoryPointer < RDBSS_TRACKER_HISTORY_SIZE)
        {
            RxContext->TrackerHistory[TrackerHistoryPointer].AcquireRelease = Operation;
            RxContext->TrackerHistory[TrackerHistoryPointer].LineNumber = LineNumber;
            RxContext->TrackerHistory[TrackerHistoryPointer].FileName = (PSZ)FileName;
            RxContext->TrackerHistory[TrackerHistoryPointer].SavedTrackerValue = RxContext->AcquireReleaseFcbTrackerX;
            RxContext->TrackerHistory[TrackerHistoryPointer].Flags = RxContext->Flags;
        }

        /* If it's negative, then we released once more than we acquired it?! */
        ASSERT(RxContext->AcquireReleaseFcbTrackerX >= 0);
    }
}
#endif

VOID
RxTrackPagingIoResource(
    _Inout_ PVOID Instance,
    _In_ ULONG Type,
    _In_ ULONG Line,
    _In_ PCSTR File)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID
RxUndoScavengerFinalizationMarking(
    PVOID Instance)
{
    /* Just call internal routine with mutex held */
    RxAcquireScavengerMutex();
    RxpUndoScavengerFinalizationMarking(Instance);
    RxReleaseScavengerMutex();
}

/*
 * @implemented
 */
VOID
RxUninitializeVNetRootParameters(
   IN PUNICODE_STRING UserName,
   IN PUNICODE_STRING UserDomainName,
   IN PUNICODE_STRING Password,
   OUT PULONG Flags)
{
    PAGED_CODE();

    /* Only free what could have been allocated */
    if (UserName != NULL)
    {
        RxFreePool(UserName);
    }

    if (UserDomainName != NULL)
    {
        RxFreePool(UserDomainName);
    }

    if (Password != NULL)
    {
        RxFreePool(Password);
    }

    /* And remove the possibly set CSC agent flag */
    if (Flags != NULL)
    {
        (*Flags) &= ~VNETROOT_FLAG_CSCAGENT_INSTANCE;
    }
}

/*
 * @implemented
 */
VOID
RxUpdateCondition(
    IN RX_BLOCK_CONDITION NewConditionValue,
    OUT PRX_BLOCK_CONDITION Condition,
    IN OUT PLIST_ENTRY TransitionWaitList)
{
    PRX_CONTEXT Context;
    LIST_ENTRY SerializationQueue;

    PAGED_CODE();

    DPRINT("RxUpdateCondition(%d, %p, %p)\n", NewConditionValue, Condition, TransitionWaitList);

    /* Set the new condition */
    RxAcquireSerializationMutex();
    ASSERT(NewConditionValue != Condition_InTransition);
    *Condition = NewConditionValue;
    /* And get the serialization queue for treatment */
    RxTransferList(&SerializationQueue, TransitionWaitList);
    RxReleaseSerializationMutex();

    /* Handle the serialization queue */
    Context = RxRemoveFirstContextFromSerializationQueue(&SerializationQueue);
    while (Context != NULL)
    {
        /* If the caller asked for post, post the request */
        if (BooleanFlagOn(Context->Flags, RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION))
        {
            Context->Flags &= ~RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION;
            RxFsdPostRequest(Context);
        }
        /* Otherwise, wake up sleeping waiters */
        else
        {
            RxSignalSynchronousWaiter(Context);
        }

        Context = RxRemoveFirstContextFromSerializationQueue(&SerializationQueue);
    }
}

/*
 * @implemented
 */
VOID
RxVerifyOperationIsLegal(
    IN PRX_CONTEXT RxContext)
{
    PIRP Irp;
    PMRX_FOBX Fobx;
    BOOLEAN FlagSet;
    PFILE_OBJECT FileObject;
    PIO_STACK_LOCATION Stack;

    PAGED_CODE();

    Irp = RxContext->CurrentIrp;
    Stack = RxContext->CurrentIrpSp;
    FileObject = Stack->FileObject;

    /* We'll only check stuff on opened files, this requires an IRP and a FO */
    if (Irp == NULL || FileObject == NULL)
    {
        return;
    }

    /* Set no exception for breakpoint - remember whether is was already set */
    FlagSet = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT);
    SetFlag(RxContext->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT);

    /* If we have a CCB, perform a few checks on opened file */
    Fobx = RxContext->pFobx;
    if (Fobx != NULL)
    {
        PMRX_SRV_OPEN SrvOpen;

        SrvOpen = Fobx->pSrvOpen;
        if (SrvOpen != NULL)
        {
            UCHAR MajorFunction;

            MajorFunction = RxContext->MajorFunction;
            /* Only allow closing/cleanup operations on renamed files */
            if (MajorFunction != IRP_MJ_CLEANUP && MajorFunction != IRP_MJ_CLOSE &&
                BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_FILE_RENAMED))
            {
                RxContext->IoStatusBlock.Status = STATUS_FILE_RENAMED;
                ExRaiseStatus(STATUS_FILE_RENAMED);
            }

            /* Only allow closing/cleanup operations on deleted files */
            if (MajorFunction != IRP_MJ_CLEANUP && MajorFunction != IRP_MJ_CLOSE &&
                BooleanFlagOn(SrvOpen->Flags, SRVOPEN_FLAG_FILE_DELETED))
            {
                RxContext->IoStatusBlock.Status = STATUS_FILE_DELETED;
                ExRaiseStatus(STATUS_FILE_DELETED);
            }
        }
    }

    /* If that's an open operation */
    if (RxContext->MajorFunction == IRP_MJ_CREATE)
    {
        PFILE_OBJECT RelatedFileObject;

        /* We won't allow an open operation relative to a file to be deleted */
        RelatedFileObject = FileObject->RelatedFileObject;
        if (RelatedFileObject != NULL)
        {
            PMRX_FCB Fcb;

            Fcb = RelatedFileObject->FsContext;
            if (BooleanFlagOn(Fcb->FcbState, FCB_STATE_DELETE_ON_CLOSE))
            {
                RxContext->IoStatusBlock.Status = STATUS_DELETE_PENDING;
                ExRaiseStatus(STATUS_DELETE_PENDING);
            }
        }
    }

    /* If cleanup was completed */
    if (BooleanFlagOn(FileObject->Flags, FO_CLEANUP_COMPLETE))
    {
        if (!BooleanFlagOn(Irp->Flags, IRP_PAGING_IO))
        {
            UCHAR MajorFunction;

            /* We only allow a subset of operations (see FatVerifyOperationIsLegal for instance) */
            MajorFunction = Stack->MajorFunction;
            if (MajorFunction != IRP_MJ_CLOSE && MajorFunction != IRP_MJ_QUERY_INFORMATION &&
                MajorFunction != IRP_MJ_SET_INFORMATION)
            {
                if ((MajorFunction != IRP_MJ_READ && MajorFunction != IRP_MJ_WRITE) ||
                    !BooleanFlagOn(Stack->MinorFunction, IRP_MN_COMPLETE))
                {
                    RxContext->IoStatusBlock.Status = STATUS_FILE_CLOSED;
                    ExRaiseStatus(STATUS_FILE_CLOSED);
                }
            }
        }
    }

    /* If flag was already set, don't clear it */
    if (!FlagSet)
    {
        ClearFlag(RxContext->Flags, RX_CONTEXT_FLAG_NO_EXCEPTION_BREAKPOINT);
    }
}

/*
 * @implemented
 */
VOID
RxWaitForStableCondition(
    IN PRX_BLOCK_CONDITION Condition,
    IN OUT PLIST_ENTRY TransitionWaitList,
    IN OUT PRX_CONTEXT RxContext,
    OUT NTSTATUS *AsyncStatus OPTIONAL)
{
    BOOLEAN Wait;
    NTSTATUS LocalStatus;

    PAGED_CODE();

    /* Make sure to always get status */
    if (AsyncStatus == NULL)
    {
        AsyncStatus = &LocalStatus;
    }

    /* By default, it's a success */
    *AsyncStatus = STATUS_SUCCESS;

    Wait = FALSE;
    /* If it's not stable, we've to wait */
    if (!StableCondition(*Condition))
    {
        /* Lock the mutex */
        RxAcquireSerializationMutex();
        /* Still not stable? */
        if (!StableCondition(*Condition))
        {
            /* Insert us in the wait list for processing on stable condition */
            RxInsertContextInSerializationQueue(TransitionWaitList, RxContext);

            /* If we're asked to post on stable, don't wait, and just return pending */
            if (BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_POST_ON_STABLE_CONDITION))
            {
                *AsyncStatus = STATUS_PENDING;
            }
            else
            {
                Wait = TRUE;
            }
        }
        RxReleaseSerializationMutex();

        /* We don't post on stable, so, just wait... */
        if (Wait)
        {
            RxWaitSync(RxContext);
        }
    }
}

/*
 * @implemented
 */
VOID
NTAPI
RxWorkItemDispatcher(
    PVOID Context)
{
    PRX_WORK_DISPATCH_ITEM DispatchItem = Context;

    DPRINT("Calling: %p, %p\n", DispatchItem->DispatchRoutine, DispatchItem->DispatchRoutineParameter);

    DispatchItem->DispatchRoutine(DispatchItem->DispatchRoutineParameter);

    RxFreePoolWithTag(DispatchItem, RX_WORKQ_POOLTAG);
}

/*
 * @implemented
 */
PVOID
NTAPI
_RxAllocatePoolWithTag(
    _In_ POOL_TYPE PoolType,
    _In_ SIZE_T NumberOfBytes,
    _In_ ULONG Tag)
{
    return ExAllocatePoolWithTagPriority(PoolType, NumberOfBytes, Tag, LowPoolPriority);
}

/*
 * @implemented
 */
VOID
NTAPI
_RxFreePool(
    _In_ PVOID Buffer)
{
    ExFreePoolWithTag(Buffer, 0);
}

/*
 * @implemented
 */
VOID
NTAPI
_RxFreePoolWithTag(
    _In_ PVOID Buffer,
    _In_ ULONG Tag)
{
    ExFreePoolWithTag(Buffer, Tag);
}

NTSTATUS
__RxAcquireFcb(
    _Inout_ PFCB Fcb,
    _Inout_opt_ PRX_CONTEXT RxContext OPTIONAL,
    _In_ ULONG Mode
#ifdef RDBSS_TRACKER
    ,
    _In_ ULONG LineNumber,
    _In_ PCSTR FileName,
    _In_ ULONG SerialNumber
#endif
    )
{
    NTSTATUS Status;
    BOOLEAN SpecialContext, CanWait, Acquired, ContextIsPresent;

    PAGED_CODE();

    DPRINT("__RxAcquireFcb(%p, %p, %d, %d, %s, %d)\n", Fcb, RxContext, Mode, LineNumber, FileName, SerialNumber);

    SpecialContext = FALSE;
    ContextIsPresent = FALSE;
    /* Check for special context */
    if (RxContext == CHANGE_BUFFERING_STATE_CONTEXT || RxContext == CHANGE_BUFFERING_STATE_CONTEXT_WAIT)
    {
        SpecialContext = TRUE;
    }

    /* We don't handle buffering state change yet... */
    if (!RxIsFcbAcquired(Fcb) && !SpecialContext &&
        BooleanFlagOn(Fcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING))
    {
        UNIMPLEMENTED;
    }

    /* Nor special contexts */
    if (SpecialContext)
    {
        UNIMPLEMENTED;
    }

    /* If we don't have a context, assume we can wait! */
    if (RxContext == NULL)
    {
        CanWait = TRUE;
    }
    else
    {
        /* That said: we have a real context! */
        ContextIsPresent = TRUE;

        /* If we've been cancelled in between, give up */
        Status = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_CANCELLED) ? STATUS_CANCELLED : STATUS_SUCCESS;
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Can we wait? */
        CanWait = BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_WAIT);
    }

    while (TRUE)
    {
        /* Assume we cannot lock */
        Status = STATUS_LOCK_NOT_GRANTED;

        /* Lock according to what the caller asked */
        switch (Mode)
        {
            case FCB_MODE_EXCLUSIVE:
                Acquired = ExAcquireResourceExclusiveLite(Fcb->Header.Resource, CanWait);
                break;

            case FCB_MODE_SHARED:
                Acquired = ExAcquireResourceSharedLite(Fcb->Header.Resource, CanWait);
                break;

            case FCB_MODE_SHARED_WAIT_FOR_EXCLUSIVE:
                Acquired = ExAcquireSharedWaitForExclusive(Fcb->Header.Resource, CanWait);
                break;

            default:
                ASSERT(Mode == FCB_MODE_SHARED_STARVE_EXCLUSIVE);
                Acquired = ExAcquireSharedStarveExclusive(Fcb->Header.Resource, CanWait);
                break;
        }

        /* Lock granted! */
        if (Acquired)
        {
            Status = STATUS_SUCCESS;
            ASSERT_CORRECT_FCB_STRUCTURE(Fcb);

            /* Handle paging write - not implemented */
            if (Fcb->NonPaged->OutstandingAsyncWrites != 0)
            {
                UNIMPLEMENTED;
            }
        }

        /* And break, that cool! */
        if (Acquired)
        {
            break;
        }

        /* If it failed, return immediately */
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    /* If we don't have to check for valid operation, job done, nothing more to do */
    if (!ContextIsPresent || BooleanFlagOn(RxContext->Flags, RX_CONTEXT_FLAG_BYPASS_VALIDOP_CHECK))
    {
        if (NT_SUCCESS(Status))
        {
            RxTrackerUpdateHistory(RxContext, RX_GET_MRX_FCB(Fcb), TRACKER_ACQUIRE_FCB, LineNumber, FileName, SerialNumber);
        }

        return Status;
    }

    /* Verify operation */
    _SEH2_TRY
    {
        RxVerifyOperationIsLegal(RxContext);
    }
    _SEH2_FINALLY
    {
        /* If it failed, release lock and fail */
        if (_SEH2_AbnormalTermination())
        {
            ExReleaseResourceLite(Fcb->Header.Resource);
            Status = STATUS_LOCK_NOT_GRANTED;
        }
    }
    _SEH2_END;

    if (NT_SUCCESS(Status))
    {
        RxTrackerUpdateHistory(RxContext, RX_GET_MRX_FCB(Fcb), TRACKER_ACQUIRE_FCB, LineNumber, FileName, SerialNumber);
    }

    DPRINT("Status: %x\n", Status);
    return Status;
}

/*
 * @implemented
 */
VOID
__RxItsTheSameContext(
    _In_ PRX_CONTEXT RxContext,
    _In_ ULONG CapturedRxContextSerialNumber,
    _In_ ULONG Line,
    _In_ PCSTR File)
{
    /* Check we have a context with the same serial number */
    if (NodeType(RxContext) != RDBSS_NTC_RX_CONTEXT ||
        RxContext->SerialNumber != CapturedRxContextSerialNumber)
    {
        /* Just be noisy */
        DPRINT1("Context %p has changed at line %d in file %s\n", RxContext, Line, File);
    }
}

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
    )
{
    BOOLEAN IsExclusive, BufferingPending;

    RxAcquireSerializationMutex();

    BufferingPending = BooleanFlagOn(MrxFcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
    IsExclusive = !!RxIsResourceOwnershipStateExclusive(MrxFcb->Header.Resource);

    /* If no buffering pending, or no exclusive lock (we can only handle with an exclusive lock),
     * then just release the FCB
     */
    if (!BufferingPending || !IsExclusive)
    {
        RxTrackerUpdateHistory(RxContext, MrxFcb, (!BufferingPending ? TRACKER_RELEASE_FCB_NO_BUFF_PENDING : TRACKER_RELEASE_NON_EXCL_FCB_BUFF_PENDING),
                               LineNumber, FileName, SerialNumber);
        ExReleaseResourceLite(MrxFcb->Header.Resource);
    }

    RxReleaseSerializationMutex();

    /* And finally leave */
    if (!BufferingPending || !IsExclusive)
    {
        return;
    }

    ASSERT(RxIsFcbAcquiredExclusive(MrxFcb));

    /* Otherwise, handle buffering state and release */
    RxProcessFcbChangeBufferingStateRequest((PFCB)MrxFcb);

    RxTrackerUpdateHistory(RxContext, MrxFcb, TRACKER_RELEASE_EXCL_FCB_BUFF_PENDING, LineNumber, FileName, SerialNumber);
    ExReleaseResourceLite(MrxFcb->Header.Resource);
}

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
    )
{
    BOOLEAN IsExclusive, BufferingPending;

    RxAcquireSerializationMutex();

    BufferingPending = BooleanFlagOn(MrxFcb->FcbState, FCB_STATE_BUFFERING_STATE_CHANGE_PENDING);
    IsExclusive = !!RxIsResourceOwnershipStateExclusive(MrxFcb->Header.Resource);

    /* If no buffering pending, or no exclusive lock (we can only handle with an exclusive lock),
     * then just release the FCB
     */
    if (!BufferingPending || !IsExclusive)
    {
        RxTrackerUpdateHistory(RxContext, MrxFcb,
                               (!BufferingPending ? TRACKER_RELEASE_FCB_FOR_THRD_NO_BUFF_PENDING : TRACKER_RELEASE_NON_EXCL_FCB_FOR_THRD_BUFF_PENDING),
                               LineNumber, FileName, SerialNumber);
        ExReleaseResourceForThreadLite(MrxFcb->Header.Resource, ResourceThreadId);
    }

    RxReleaseSerializationMutex();

    /* And finally leave */
    if (!BufferingPending || !IsExclusive)
    {
        return;
    }

    /* Otherwise, handle buffering state and release */
    RxTrackerUpdateHistory(RxContext, MrxFcb, TRACKER_RELEASE_EXCL_FCB_FOR_THRD_BUFF_PENDING, LineNumber, FileName, SerialNumber);
    RxProcessFcbChangeBufferingStateRequest((PFCB)MrxFcb);
    ExReleaseResourceForThreadLite(MrxFcb->Header.Resource, ResourceThreadId);
}
