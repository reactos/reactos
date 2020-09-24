//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "..\ioprivshared.hpp"


extern "C" {
#if defined(EVENT_TRACING)
#include "FxIoQueueKm.tmh"
#endif
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::QueueForwardProgressIrpLocked(
    __in PIRP Irp
    )
{
    InsertTailList(&m_FwdProgContext->m_PendedIrpList,
                   &Irp->Tail.Overlay.ListEntry);

    Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] =
            m_FwdProgContext;

    IoSetCancelRoutine(Irp, _WdmCancelRoutineForReservedIrp);

    if (Irp->Cancel) {
        if (IoSetCancelRoutine(Irp, NULL) != NULL){

            Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] = NULL;

            RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
            InitializeListHead(&Irp->Tail.Overlay.ListEntry);

            return STATUS_CANCELLED;
        }
        else {
            //
            // CancelRoutine will complete the IRP
            //
            DO_NOTHING();
        }
    }

    IoMarkIrpPending(Irp);

    return STATUS_PENDING;
}


_Must_inspect_result_
PIRP
FxIoQueue::GetForwardProgressIrpLocked(
    __in_opt PFILE_OBJECT FileObject
    )
/*++

    Routine Description:
        Remove an IRP from the pending irp list if it matches with the input
        fileobject. If the fileobject value is NULL, return the first one from
        the pending list.

--*/
{
    PIRP pIrp;
    PLIST_ENTRY thisEntry, nextEntry, listHead;
    PIO_STACK_LOCATION  pIrpStack;

    pIrp = NULL;
    listHead = &m_FwdProgContext->m_PendedIrpList;

    for(thisEntry = listHead->Flink;
        thisEntry != listHead;
        thisEntry = nextEntry) {
        nextEntry = thisEntry->Flink;

        pIrp = CONTAINING_RECORD(thisEntry, IRP, Tail.Overlay.ListEntry);
        pIrpStack = IoGetCurrentIrpStackLocation(pIrp);

        if (FileObject == NULL || FileObject == pIrpStack->FileObject) {

            RemoveEntryList(thisEntry);
            InitializeListHead(thisEntry);

            if (NULL != IoSetCancelRoutine (pIrp, NULL)) {
                pIrp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] = NULL;
                break;
            }
            else {
                //
                // Irp is canceled and the cancel routines is waiting to run.
                // Continue to get the next the irp in the list.
                //
                DO_NOTHING();
            }
        }

        pIrp = NULL;
    }

    return pIrp;
}

VOID
FxIoQueue::FreeAllReservedRequests(
    __in BOOLEAN Verify
    )
/*++

Routine Description:
    Called from dispose to Free all the reserved requests.

    Verify -
        TRUE - Make sure the number of request freed matches with the count of
               request created.
        FALSE - Called when we fail to allocate all the reserved requests
                during config at init time. So we don't verify because the
                count of request freed wouldn't match with the configured value.
--*/
{
    ULONG count;
    KIRQL oldIrql;

    count = 0;

    //
    // Since forward progress request are allocated only for top level
    // queues which cant be deleted so we dont need to add a reference on the
    // queue for each reserved request since the Queue is guaranteed to be
    // around when the request is being freed even if the Request was forwarded
    // to another Queue.
    //
    m_FwdProgContext->m_PendedReserveLock.Acquire(&oldIrql);

    while (!IsListEmpty(&m_FwdProgContext->m_ReservedRequestList)) {
        PLIST_ENTRY pEntry;
        FxRequest * pRequest;

        pEntry = RemoveHeadList(&m_FwdProgContext->m_ReservedRequestList);
        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryForwardProgress,
                                                  pEntry);
        pRequest->FreeRequest();

        count++;
    }

    if (Verify) {
        ASSERT(count == m_FwdProgContext->m_NumberOfReservedRequests);
    }

    ASSERT(IsListEmpty(&m_FwdProgContext->m_ReservedRequestInUseList));
    ASSERT(IsListEmpty(&m_FwdProgContext->m_PendedIrpList));

    m_FwdProgContext->m_PendedReserveLock.Release(oldIrql);

    return;
}

VOID
FxIoQueue::ReturnReservedRequest(
    __in FxRequest *ReservedRequest
    )
/*++

Routine Description:
    Reuse the ReservedRequest if there are pended IRPs otherwise
    add it back to the reserve list.

--*/
{
    KIRQL   oldIrql;
    PIRP    pIrp;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();
    pIrp= NULL;

    ASSERT(ReservedRequest->GetRefCnt() == 1);

    if (pFxDriverGlobals->FxVerifierOn) {
        ReservedRequest->ClearVerifierFlags(
            FXREQUEST_FLAG_RESERVED_REQUEST_ASSOCIATED_WITH_IRP);
    }

    //
    // Reuse the reserved request for dispatching the next pending IRP.
    //
    m_FwdProgContext->m_PendedReserveLock.Acquire(&oldIrql);

    pIrp = GetForwardProgressIrpLocked(NULL);

    m_FwdProgContext->m_PendedReserveLock.Release(oldIrql);

    ReservedRequest->ClearFieldsForReuse();

    if (pIrp != NULL) {
        //
        // Associate the reserved request with the Pended IRP
        //
        ReservedRequest->m_Irp.SetIrp(pIrp);
        ReservedRequest->AssignMemoryBuffers(m_Device->GetIoType());

        if (pFxDriverGlobals->FxVerifierOn) {
            ReservedRequest->SetVerifierFlags(
                FXREQUEST_FLAG_RESERVED_REQUEST_ASSOCIATED_WITH_IRP);
        }

        //
        // Ignore the return status because QueueRequest will complete the
        // request on it own if it fails to queue request.
        //
        (VOID) QueueRequest(ReservedRequest);
    }
    else {
        PutBackReservedRequest(ReservedRequest);
    }
}

VOID
FxIoQueue::GetForwardProgressIrps(
    __in     PLIST_ENTRY    IrpListHead,
    __in_opt MdFileObject   FileObject
    )
/*++

Routine Description:

    This function is called to retrieve the list of reserved queued IRPs.
    The IRP's Tail.Overlay.ListEntry field is used to link these structs together.

--*/
{
    PIRP    irp;
    KIRQL   irql;

    m_FwdProgContext->m_PendedReserveLock.Acquire(&irql);

    do {
        irp = GetForwardProgressIrpLocked(FileObject);
        if (irp != NULL) {
            //
            // Add it to the cleanupList. We will complete the IRP after
            // releasing the lock.
            //
            InsertTailList(IrpListHead, &irp->Tail.Overlay.ListEntry);
        }
    } while (irp != NULL);

    m_FwdProgContext->m_PendedReserveLock.Release(irql);
}

VOID
FxIoQueue::_WdmCancelRoutineForReservedIrp(
    __inout struct _DEVICE_OBJECT * DeviceObject,
    __in __drv_useCancelIRQL PIRP   Irp
    )
/*++

Routine Description:

    Cancel routine for forward progress irp.

--*/
{
    KIRQL irql;
    PFXIO_FORWARD_PROGRESS_CONTEXT m_FwdPrgContext;

    UNREFERENCED_PARAMETER (DeviceObject);

    IoReleaseCancelSpinLock(Irp->CancelIrql);

    m_FwdPrgContext = (PFXIO_FORWARD_PROGRESS_CONTEXT)
        Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY];

    Irp->Tail.Overlay.DriverContext[FX_IRP_QUEUE_CSQ_CONTEXT_ENTRY] = NULL;

    m_FwdPrgContext->m_PendedReserveLock.Acquire(&irql);

    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    InitializeListHead(&Irp->Tail.Overlay.ListEntry);

    m_FwdPrgContext->m_PendedReserveLock.Release(irql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return;
}

VOID
FxIoQueue::FlushQueuedDpcs(
    VOID
    )
/*++

Routine Description:

    This is the kernel mode routine to flush queued DPCs.

Arguments:

Return Value:

--*/
{
    KeFlushQueuedDpcs();
}


VOID
FxIoQueue::InsertQueueDpc(
    VOID
    )
/*++

Routine Description:

    This is the kernel mode routine to insert a dpc.

Arguments:

Return Value:

--*/
{
    KeInsertQueueDpc(&m_Dpc, NULL, NULL);
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::GetReservedRequest(
    __in MdIrp Irp,
    __deref_out_opt FxRequest **ReservedRequest
    )
/*++

Routine Description:
    Use the policy configured on the queue to decide whether to allocate a
    reserved request.

--*/
{
    WDF_IO_FORWARD_PROGRESS_ACTION action;
    FxRequest *pRequest;
    KIRQL      oldIrql;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    *ReservedRequest = NULL;
    action = WdfIoForwardProgressActionInvalid;

    switch (m_FwdProgContext->m_Policy) {

    case WdfIoForwardProgressReservedPolicyPagingIO:
        if (IsPagingIo(Irp)) {
            action = WdfIoForwardProgressActionUseReservedRequest;
        }
        else {
            action =  WdfIoForwardProgressActionFailRequest;
        }

        break;
    case WdfIoForwardProgressReservedPolicyAlwaysUseReservedRequest:

        action = WdfIoForwardProgressActionUseReservedRequest;
        break;

    case WdfIoForwardProgressReservedPolicyUseExamine:

        //
        // Call the driver to figure out what action to take.
        //
        if (m_FwdProgContext->m_IoExamineIrp.Method != NULL) {

            action = m_FwdProgContext->m_IoExamineIrp.Invoke(GetHandle(), Irp);

            //
            // Make sure the use has returned a valid action
            //
            if((action < WdfIoForwardProgressActionFailRequest ||
                action > WdfIoForwardProgressActionUseReservedRequest)) {

                status = STATUS_UNSUCCESSFUL;
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                    "EvtIoExamineIrp callback on WDFQUEUE %p returned an "
                    "invalid action %d, %!STATUS!",
                    GetHandle(), action, status);

                FxVerifierDbgBreakPoint(pFxDriverGlobals);

                return status;
             }

        }
        break;

    default:
            ASSERTMSG("Invalid forward progress setting ", FALSE);
            break;
    }

    if (action == WdfIoForwardProgressActionFailRequest) {
        status = STATUS_UNSUCCESSFUL;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Forward action on WDFQUEUE %p says that framework should fail "
            "the Irp %p, %!STATUS!",
            GetHandle(), Irp, status);

        return status;
    }

    pRequest = NULL;

    m_FwdProgContext->m_PendedReserveLock.Acquire(&oldIrql);

    if (!IsListEmpty(&m_FwdProgContext->m_ReservedRequestList)) {
        PLIST_ENTRY pEntry;

        pEntry = RemoveHeadList(&m_FwdProgContext->m_ReservedRequestList);

        pRequest = FxRequest::_FromOwnerListEntry(FxListEntryForwardProgress,
                                                  pEntry);
        ASSERT(pRequest != NULL);
        ASSERT(pRequest->GetRefCnt() == 1);

        InsertTailList(&m_FwdProgContext->m_ReservedRequestInUseList,
                    pRequest->GetListEntry(FxListEntryForwardProgress));

        pRequest->m_Irp.SetIrp(Irp);
        pRequest->AssignMemoryBuffers(m_Device->GetIoType());

        if (pFxDriverGlobals->FxVerifierOn) {
            pRequest->SetVerifierFlags(FXREQUEST_FLAG_RESERVED_REQUEST_ASSOCIATED_WITH_IRP);
        }

        //
        // if *ReservedRequest is non-null the caller needs to free the
        // previous request it allocated.
        //
        *ReservedRequest = pRequest;

        status = STATUS_SUCCESS;
    }
    else {

        status = QueueForwardProgressIrpLocked(Irp);
        ASSERT(*ReservedRequest == NULL);
    }

    m_FwdProgContext->m_PendedReserveLock.Release(oldIrql);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoQueue::AssignForwardProgressPolicy(
    __in PWDF_IO_QUEUE_FORWARD_PROGRESS_POLICY Policy
    )
/*++

Routine Description:
    Configure the queue for forward Progress.

--*/
{
    NTSTATUS status;
    FxRequest *pRequest;
    ULONG index;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    //
    // If the queue is not a top level queue then return an error
    //
    status = STATUS_SUCCESS;

    //
    // From v1.11 any queue can be top level (see WdfDeviceWdmDispatchIrpToIoQueue API).
    //
    if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1, 11) == FALSE) {
        if (m_PkgIo->IsTopLevelQueue(this) == FALSE) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                "Setting Forward progress policy on non-top level queue %!STATUS!",
                status);

            return status;
        }
    }

    m_FwdProgContext = (PFXIO_FORWARD_PROGRESS_CONTEXT)
                                FxPoolAllocate(GetDriverGlobals(),
                                    NonPagedPool,
                                    sizeof(FXIO_FORWARD_PROGRESS_CONTEXT)
                                    );
    if (m_FwdProgContext == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Could not allocate memory for forward progress context %!STATUS!",
            status);

        return status;
    }

    RtlZeroMemory(m_FwdProgContext, sizeof(FXIO_FORWARD_PROGRESS_CONTEXT));

    //
    // Initialize the things which will not fail first
    //
    m_FwdProgContext->m_Policy = Policy->ForwardProgressReservedPolicy;

    m_FwdProgContext->m_NumberOfReservedRequests =
            Policy->TotalForwardProgressRequests;

    m_FwdProgContext->m_IoReservedResourcesAllocate.Method  =
            Policy->EvtIoAllocateResourcesForReservedRequest;

    m_FwdProgContext->m_IoResourcesAllocate.Method  =
            Policy->EvtIoAllocateRequestResources;

    m_FwdProgContext->m_IoExamineIrp.Method  =
            Policy->ForwardProgressReservePolicySettings.Policy.\
                ExaminePolicy.EvtIoWdmIrpForForwardProgress;

    InitializeListHead(&m_FwdProgContext->m_ReservedRequestList);
    InitializeListHead(&m_FwdProgContext->m_ReservedRequestInUseList);
    InitializeListHead(&m_FwdProgContext->m_PendedIrpList);

    m_FwdProgContext->m_PendedReserveLock.Initialize();

    for (index = 0;
         index < m_FwdProgContext->m_NumberOfReservedRequests;
         index++)
    {
        status = AllocateReservedRequest(&pRequest);
        if (!NT_SUCCESS(status)) {
            break;
        }

        InsertTailList(&m_FwdProgContext->m_ReservedRequestList,
                       pRequest->GetListEntry(FxListEntryForwardProgress));
    }

    //
    // Since we allow forward progress policy to be set even after AddDevice
    // when the queue has already started dispatching, we need to make
    // sure all checks which determine whether the queue is configured for
    // forward progress succeed. Setting Forward progress on the queue as the
    // last operation helps with that.
    //
    if (NT_SUCCESS(status)) {
        m_SupportForwardProgress = TRUE;
    }
    else {
        FreeAllReservedRequests(FALSE);

        m_FwdProgContext->m_PendedReserveLock.Uninitialize();

        FxPoolFree(m_FwdProgContext);
        m_FwdProgContext = NULL;
    }

    return status;
}
