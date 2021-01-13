/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTarget.cpp

Abstract:

    This module implements the IO Target APIs

Author:

Environment:

    Both kernel and user mode

Revision History:

--*/


#include "../fxtargetsshared.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "FxIoTarget.tmh"
#endif
}

const PVOID FxIoTarget::m_SentRequestTag = (PVOID) 'lcnC';

FxIoTarget::FxIoTarget(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize
    ) :
    FxNonPagedObject(FX_TYPE_IO_TARGET, ObjectSize, FxDriverGlobals)
{
    Construct();
}

FxIoTarget::FxIoTarget(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in USHORT WdfType
    ) :
    FxNonPagedObject(WdfType, ObjectSize, FxDriverGlobals)
{
    Construct();
}

VOID
FxIoTarget::Construct(
    VOID
    )
{
    InitializeListHead(&m_SentIoListHead);
    InitializeListHead(&m_IgnoredIoListHead);

    m_InStack = TRUE;

    m_State = WdfIoTargetStarted;

    m_WaitingForSentIo = FALSE;
    m_Removing = FALSE;
    m_AddedToDeviceList = FALSE;

    m_Driver = NULL;
    m_InStackDevice = NULL;
    m_TargetDevice = NULL;
    m_TargetPdo = NULL;
    m_TargetFileObject = NULL;
    m_TargetStackSize = 0;
    m_TargetIoType = WdfDeviceIoUndefined;
    m_IoCount = 1;
    m_DisposeEvent = NULL;
    m_TransactionedEntry.SetTransactionedObject(this);

    m_PendedQueue.Initialize(this, _RequestCancelled);

    //
    // We want to guarantee that the cleanup callback is called at passive level
    // so the driver writer can override the automatic behavior the target uses
    // when shutting down with i/o in progress.
    //
    MarkPassiveDispose(ObjectDoNotLock);
    MarkDisposeOverride(ObjectDoNotLock);
}

FxIoTarget::~FxIoTarget()
{
    ASSERT(IsListEmpty(&m_SentIoListHead));
    ASSERT(IsListEmpty(&m_IgnoredIoListHead));
    ASSERT(m_IoCount == 0);
}

VOID
FxIoTarget::PrintDisposeMessage(
    VOID
    )
/*++

Routine Description:
    To prevent WPP from reporting incorrect module or line number if the
    caller is an inline function we use this function to print the message
    to the IFR.

Arguments:
    None

Return Value:
    None

  --*/

{
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                        "WDFIOTARGET %p, setting Dispose event %p",
                        GetObjectHandle(), m_DisposeEvent->GetEvent());
}

VOID
FxIoTarget::WaitForDisposeEvent(
    VOID
    )
{
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
        FxCREvent * event = m_DisposeEventUm.GetSelfPointer();
#else
        FxCREvent eventOnStack;
        eventOnStack.Initialize();
        FxCREvent * event = eventOnStack.GetSelfPointer();
#endif

    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    m_DisposeEvent = event;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                        "WDFIOTARGET %p, Waiting on Dispose event %p",
                        GetObjectHandle(),m_DisposeEvent->GetEvent());

    if (InterlockedDecrement(&m_IoCount) > 0) {
        event->EnterCRAndWaitAndLeave();
    }

    m_DisposeEvent = NULL;
    ASSERT(m_IoCount == 0);
}


BOOLEAN
FxIoTarget::Dispose(
    VOID
    )
/*++

Routine Description:
    Dispose is overridden so that the driver can have a chance to override the
    default remove behavior for the target.  By default, remove will cancel
    all sent I/O and then wait for it to complete.  For instance, if the driver
    wants to wait for all i/o to complete rather then be canceled, a cleanup
    callback should be registered on the target and the wait should be performed
    there.

Arguments:
    None

Return Value:
    FALSE, indicating to the FxObject state machine *NOT* to call

  --*/

{
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    if (m_AddedToDeviceList) {
        //
        // Remove the target from the list of targets that the device keeps track
        // of.
        //
        ASSERT(m_DeviceBase != NULL);
        m_DeviceBase->RemoveIoTarget(this);
    }

    //
    //
    // Call all of the cleanup callbacks first
    //
    CallCleanup();

    //
    // Now cancel all sent i/o and shut the target down
    //
    Remove();

    //
    // By returning FALSE, the object state machine will not attempt to call
    // cleanup again.
    //
    return FALSE;
}

VOID
FxIoTarget::SubmitPendedRequest(
    __in FxRequestBase* Request
    )
{
    ULONG action;
    FxIrp* irp = NULL;

    irp = Request->GetSubmitFxIrp();
    action = Submit(Request, NULL, 0);
    if (action & SubmitSend) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Sending Pended WDFREQUEST %p, Irp %p",
            Request->GetTraceObjectHandle(),
            irp->GetIrp());

        Send(irp->GetIrp());
    }

    //
    // If the request was not sent or pended (queued), complete it.
    //
    if ((action & (SubmitSend | SubmitQueued)) == 0) {
        ASSERT(0 == action);
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Completing Pended WDFREQUEST %p, Irp %p, %!STATUS!",
            Request->GetTraceObjectHandle(), irp->GetIrp(), irp->GetStatus());

        //
        // Fail the request.
        //
        ASSERT(!NT_SUCCESS(irp->GetStatus()));
        irp->SetInformation(0);

        //
        // This function updates the outstanding 'io count' value and
        // decrement the ref count on Request.
        //
        HandleFailedResubmit(Request);
    }
    else {
        //
        // Submit() updated the 'io count' counter for request that got sent or
        // queued. Note that the input request was already tracked via this
        // counter, thus decrement its value.
        //
        DecrementIoCount();

        //
        // SubmitLocked will add another ref if it needs it.  Release the
        // reference taken when the request was pended.
        //
        // Release the reference after submitting it in case the request was deleted.
        // If it was deleted, the reference taken by the target is the only keep
        // it alive.
        //
        Request->RELEASE(this);
    }
}

VOID
FxIoTarget::SubmitPendedRequests(
    __in PLIST_ENTRY RequestListHead
    )
{
    PLIST_ENTRY ple;

    while (!IsListEmpty(RequestListHead)) {
        ple = RemoveHeadList(RequestListHead);
        SubmitPendedRequest(FxRequestBase::_FromListEntry(ple));
    }
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::Start(
    VOID
    )
{
    LIST_ENTRY head;
    NTSTATUS status;

    InitializeListHead(&head);

    status = GotoStartState(&head);

    SubmitPendedRequests(&head);

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFIOTARGET %p started status %!STATUS!",
        GetHandle(),status);
    return status;
}

#define START_TAG       ((PVOID) ('trtS'))

_Must_inspect_result_
NTSTATUS
FxIoTarget::GotoStartState(
    __in PLIST_ENTRY RequestListHead,
    __in BOOLEAN Lock
    )
{
    NTSTATUS status;
    KIRQL irql;

    irql = PASSIVE_LEVEL;

    ADDREF(START_TAG);

    if (Lock) {
        FxIoTarget::Lock(&irql);
    }

CheckState:
    if (m_State == WdfIoTargetDeleted) {
        status = STATUS_INVALID_DEVICE_STATE;
    }
    else if (m_WaitingForSentIo) {

        PFX_DRIVER_GLOBALS  pFxDriverGlobals = GetDriverGlobals();

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIOTARGET,
            "WDFIOTARGET %p is being started while it is being stopped or "
            "purged by another thread. WdfIoTargetStart and "
            "WdfIoTargetStop/WdfIoTargetPurge must be called synchronously. "
            "After the driver calls one of these functions, it must not call "
            "the other function before the first one returns.",
            GetObjectHandle());

        if ((pFxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel)) &&
            (irql > PASSIVE_LEVEL)) {

            //
            // We cannont wait for a previous stop to complete if we are at
            // dispatch level
            //
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }

        //
        // Wait for the stop code to complete
        //
        ASSERT(m_State == WdfIoTargetStopped || m_State == WdfIoTargetPurged);

        ASSERT(Lock);
        Unlock(irql);

        WaitForSentIoToComplete();

        //
        // Go back and check our state again since we dropped the lock.
        //
        FxIoTarget::Lock(&irql);
        goto CheckState;
    }
    else {
        status = STATUS_SUCCESS;
    }

    //
    // Restart all of the pended i/o.
    //
    if (NT_SUCCESS(status)) {
        m_State = WdfIoTargetStarted;

        m_WaitingForSentIo = FALSE;
        m_SentIoEvent.Clear();

        //
        // TRUE - requests will be resent to the target, so attempt to claim
        //        cancelation  ownership
        //
        DrainPendedRequestsLocked(RequestListHead, TRUE);
    }

    if (Lock) {
        Unlock(irql);
    }

    RELEASE(START_TAG);

    return status;
}

VOID
FxIoTarget::DrainPendedRequestsLocked(
    __in PLIST_ENTRY RequestListHead,
    __in BOOLEAN RequestWillBeResent
    )
{
    PMdIoCsqIrpContext pContext;
    MdIrp pIrp;
    FxIrp* pFxIrp = NULL;
    pContext = NULL;

    while ((pIrp = m_PendedQueue.GetNextRequest(&pContext)) != NULL) {
        FxRequestBase* pRequest;
        BOOLEAN enqueue;

        enqueue = FALSE;

        //
        // Each FxRequestBase on the pending list has a reference taken against
        // it already.  We will release the reference after we call Submit.
        //
        // After this call we are done with the m_CsqContext field.
        //
        pRequest = FxRequestBase::_FromCsqContext(pContext);

        //
        // m_ListEntry and m_CsqContext are a union.  Now that we are done with
        // m_CsqContext, initialize it to be a valid list entry.
        //
        InitializeListHead(&pRequest->m_ListEntry);

        //
        // Undo the affects of the IoSetNextIrpStackLocation made when we
        // enqueued the request.  We want to do this no matter if we can claim
        // cancellation ownership of the request or not.
        //
        pFxIrp = pRequest->GetSubmitFxIrp();
        pFxIrp->SkipCurrentIrpStackLocation();

        //
        // Request is not longer pended.
        //
        ASSERT(pRequest->GetTargetFlags() & FX_REQUEST_PENDED);
        pRequest->ClearTargetFlags(FX_REQUEST_PENDED);

        if (RequestWillBeResent) {
            //
            // Make sure timer callback is not about to run. After the call
            // below the timer was successfully canceled or the timer callback
            // already run.
            //
            if (pRequest->CancelTimer()) {
                //
                // Try to claim cancellation (*) ownership of the request.
                // CanComplete() decrements the irp completion ref count and
                // whomever decrements to zero owns the request.  Ownership in
                // this case is resubmission as well as completion (as the name
                // of the function implies).
                //
                // (*) - cancellation as defined by WDF and the myriad of
                //       functions which are calling FxRequestBase::Cancel().
                //       By this point we have already removed the cancellation
                //       routine by calling m_PendedQueue.GetNextRequest()
                //
                if (pRequest->CanComplete()) {

                    enqueue = TRUE;
                }
            }

            if (FALSE == enqueue) {
                //
                // Some other thread is responsible for canceling this request.
                // We are assuming here that the other thread will complete it
                // in that thread because we are no longer tracking this request
                // in any of our lists.
                //
                pRequest->m_Irp.SetStatus(STATUS_CANCELLED);

                //
                // Mark that the request has been completed so that if the other
                // thread is the timer DPC, it will handle the case properly and
                // complete the request.
                //
                ASSERT((pRequest->GetTargetFlags() & FX_REQUEST_COMPLETED) == 0);
                pRequest->SetTargetFlags(FX_REQUEST_COMPLETED);

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                    "WDFIOTARGET %p, WDFREQUEST %p is being canceled on another thread,"
                    " allowing other thread to complete request, not resending",
                    GetObjectHandle(), pRequest->GetTraceObjectHandle());
            }
        }
        else {
            //
            // The caller is going to attempt to complete the requests.  To make
            // the caller's life easier and let it reuse RequestCompletionRoutine,
            // to handle the completion semantics, keep the completion reference
            // count != 0 and return the request back to the caller.
            //
            enqueue = TRUE;
        }

        if (enqueue) {
            ClearCompletedRequestVerifierFlags(pRequest);
            InsertTailList(RequestListHead, &pRequest->m_ListEntry);
        }

        pContext = NULL;
    }
}

VOID
FxIoTarget::CompletePendedRequest(
    __in FxRequestBase* Request
    )
{
    //
    // This will attempt to claim cancelation ownership and call the
    // request's completion routine.
    //
    FailPendedRequest(Request, STATUS_WDF_DEVICE_REMOVED_NOT_SENT);
}

VOID
FxIoTarget::CompletePendedRequestList(
    __in PLIST_ENTRY RequestListHead
    )
{
    PLIST_ENTRY ple;

    while (!IsListEmpty(RequestListHead)) {
        ple = RemoveHeadList(RequestListHead);
        InitializeListHead(ple);
        CompletePendedRequest(FxRequestBase::_FromListEntry(ple));
    }
}

VOID
FxIoTarget::_CancelSentRequest(
    __in FxRequestBase* Request
    )
{
    //
    // Attempt to cancel the request
    //
    Request->Cancel();

    //
    // Release the reference taken by GetSentRequestsListLocked
    //
    Request->RELEASE(m_SentRequestTag);
}

VOID
FxIoTarget::_CancelSentRequests(
    __in PSINGLE_LIST_ENTRY RequestListHead
    )
/*++

Routine Description:
    Cancels all FxRequestBases in RequestListHead

Arguments:
    RequestListHead - List head containing the requests

Return Value:
    None.

  --*/
{
    PSINGLE_LIST_ENTRY ple;

    while (RequestListHead->Next != NULL) {
        ple = PopEntryList(RequestListHead);

        //
        // Set the Next pointer back to NULL so that if it is reinserted into a
        // cancel list, it will not point to unknown pool.
        //
        ple->Next = NULL;

        _CancelSentRequest(FxRequestBase::_FromDrainEntry(ple));
    }
}

VOID
FxIoTarget::GetSentRequestsListLocked(
    __in PSINGLE_LIST_ENTRY RequestListHead,
    __in PLIST_ENTRY SendList,
    __out PBOOLEAN AddedToList
    )
{
    PLIST_ENTRY ple;

    *AddedToList = IsListEmpty(SendList) ? FALSE : TRUE;

    //
    // Since we are inserting into the head of the single list head, if we walked
    // over the list from first to last, we would reverse the entries.  By walking
    // the list backwards, we build the single list head in the order of SendList.
    //
    for (ple = SendList->Blink; ple != SendList; ple = ple->Blink) {
        FxRequestBase* pRequest;

        pRequest = FxRequestBase::_FromListEntry(ple);

        //
        // Add a reference since the request will be touched outside of the
        // lock being held.
        //
        pRequest->ADDREF(m_SentRequestTag);

        //
        // Add the request at the head of the list.
        //
        pRequest->m_DrainSingleEntry.Next = RequestListHead->Next;
        RequestListHead->Next = &pRequest->m_DrainSingleEntry;
    }
}

VOID
FxIoTarget::GotoStopState(
    __in WDF_IO_TARGET_SENT_IO_ACTION   Action,
    __in PSINGLE_LIST_ENTRY             SentRequestListHead,
    __out PBOOLEAN                      Wait,
    __in BOOLEAN                        LockSelf
    )
/*++

Routine Description:
    Accumulates all pending I/O for cancelling out of this function if
    RequestListHead != NULL.

Arguments:


Return Value:

  --*/

{
    KIRQL irql;
    BOOLEAN getSentList, wait, added;

    getSentList = FALSE;
    wait = FALSE;
    irql = PASSIVE_LEVEL;

    if (LockSelf) {
        Lock(&irql);
    }

    //
    // The following transitions are allowed:
    //  (1) Started -> Stopped
    //  (2) Purged -> Stopped
    //  (3) Stopped -> Stopped
    //
    // A Stopped -> Stopped transition
    // is feasible if the previous stop left all pending i/o and the current
    // stop wants to cancel all i/o.
    //
    if (m_State == WdfIoTargetStarted || m_State == WdfIoTargetPurged) {
        m_State = WdfIoTargetStopped;
    }
    else if (m_State != WdfIoTargetStopped) {
        //
        // Stopping in any state other then stopped or started is not fatal,
        // but should be logged.
        //

        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFIOTARGET %p stopped, but it is currently in the "
            "%!WDF_IO_TARGET_STATE! state, not started or stopped",
            GetHandle(), m_State);
    }

    switch (Action) {
    case WdfIoTargetCancelSentIo:
        getSentList = TRUE;
        //     ||   ||         Drop through     ||     ||
        //     \/   \/                          \/     \/

    case WdfIoTargetWaitForSentIoToComplete:
        if (IsListEmpty(&m_SentIoListHead)) {
            //
            // By using m_WaitingForSentIo as value for wait, we can handle the
            // case where GotoStopState is called when we are already in the
            // stopping case (in which case we would have drained
            // m_SendIoListHead already).
            //
            wait = m_WaitingForSentIo;

            if (m_WaitingForSentIo) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIOTARGET,
                    "WDFIOTARGET %p is already in the process of being stopped "
                    "or purged from another thread. Driver must wait for the "
                    "first WdfIoTargetStop or WdfIoTargetPurge to complete "
                    "before calling it again.",
                    GetObjectHandle());
            }
        }
        else {
            wait = TRUE;

            if (getSentList) {
                //
                // Ignore the assignment to added since we have already computed
                // if we need to wait or not.
                //
                GetSentRequestsListLocked(SentRequestListHead,
                                          &m_SentIoListHead,
                                          &added);
            }
        }

        break;

    case WdfIoTargetLeaveSentIoPending:
        wait = FALSE;
        break;
    }

    m_WaitingForSentIo = wait;
    *Wait = wait;

    if (wait) {
        //
        // If Stop(leave sent io pending) was previously called, m_SentIoEvent
        // will be in the signalled state.  We need to wait for sent i/o to be
        // completed, so make sure it is not signalled while holding the lock
        //
        m_SentIoEvent.Clear();
    }
    else {
        //
        // Even though *Wait is set to FALSE, the caller may wait anyways
        // if it is aggregating targets to move into the stopped state and
        // wait on them all.
        //
        m_SentIoEvent.Set();
    }

    if (LockSelf) {
        Unlock(irql);
    }
}

VOID
FxIoTarget::Stop(
    __in WDF_IO_TARGET_SENT_IO_ACTION Action
    )
{
    SINGLE_LIST_ENTRY head;
    BOOLEAN wait;

    head.Next = NULL;
    wait = FALSE;

    GotoStopState(Action, &head, &wait, TRUE);

    if (head.Next != NULL) {
        _CancelSentRequests(&head);
    }

    if (wait) {
        KIRQL irql;

        //
        // Under the lock, if wait is set, m_WaitingForSentIo is true, but once
        // we drop the lock, all pended i/o could have already been canceled
        // and m_WaitingForSentIo is FALSE at this point.
        //
        // ASSERT(m_WaitingForSentIo);

        WaitForSentIoToComplete();

        //
        // Mark that we are no longer stopping and waiting for i/o to complete.
        //
        Lock(&irql);
        m_WaitingForSentIo = FALSE;
        Unlock(irql);
    }

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFIOTARGET %p stopped ",GetHandle());
}

VOID
FxIoTarget::GotoPurgeState(
    __in WDF_IO_TARGET_PURGE_IO_ACTION  Action,
    __in PLIST_ENTRY                    PendedRequestListHead,
    __in PSINGLE_LIST_ENTRY             SentRequestListHead,
    __out PBOOLEAN                      Wait,
    __in BOOLEAN                        LockSelf
    )
/*++

Routine Description:
    Accumulates all pending and sent I/O.  The I/O target is moved into
    the 'purged' state.

Arguments:


Return Value:

  --*/

{
    KIRQL   irql;
    BOOLEAN wait, added;

    wait = FALSE;
    irql = PASSIVE_LEVEL;

    if (LockSelf) {
        Lock(&irql);
    }

    //
    // The following transitions are allowed:
    //  (1) Started -> Purged
    //  (2) Stop    -> Purged
    //  (3) Purged  -> Purged
    //
    // A Purged -> Purged transition is feasible if the previous purge didn't
    // wait for pending i/o to complete and the current purge wants to wait
    // for them.
    //
    if (m_State == WdfIoTargetStarted || m_State == WdfIoTargetStopped) {
        m_State = WdfIoTargetPurged;
    }
    else if (m_State != WdfIoTargetPurged) {
        //
        // Purging in any state other then purged or started is not fatal,
        // but should be logged.
        //

        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFIOTARGET %p purged, but it is currently in the "
            "%!WDF_IO_TARGET_STATE! state, not started, stopped or purged",
            GetHandle(), m_State);
    }

    //
    // FALSE - requests will not be resent to the target.  As such,
    //         cancellation ownership will not be claimed b/c the request
    //         will subsequently be passed to FailPendedRequest.
    //
    DrainPendedRequestsLocked(PendedRequestListHead, FALSE);

    GetSentRequestsListLocked(SentRequestListHead,
                              &m_SentIoListHead,
                              &added);

    switch (Action) {
    case WdfIoTargetPurgeIoAndWait:
        if (added == FALSE) {
            //
            // By using m_WaitingForSentIo as value for wait, we can handle the
            // case where GotoPurgeState is called when we are already in the
            // purging case (in which case we would have drained
            // m_SendIoListHead already).
            //
            wait = m_WaitingForSentIo;

            if (m_WaitingForSentIo) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "WDFIOTARGET %p is already in the process of being purged "
                    "or stopped from another thread. Driver must wait for the "
                    "first WdfIoTargetPurge or WdfIoTargetStop to complete "
                    "before calling it again.",
                    GetObjectHandle());

                FxVerifierDbgBreakPoint(GetDriverGlobals());
            }
        }
        else {
            wait = TRUE;
        }
        break;

    case WdfIoTargetPurgeIo:
        wait = FALSE;
        break;
    }

    m_WaitingForSentIo = wait;
    *Wait = wait;

    if (wait) {
        //
        // If Stop/Purge(don't wait) was previously called, m_SentIoEvent
        // will be in the signalled state.  We need to wait for sent i/o to be
        // completed, so make sure it is not signalled while holding the lock
        //
        m_SentIoEvent.Clear();
    }
    else {
        //
        // Even though *Wait is set to FALSE, the caller may wait anyways
        // if it is aggregating targets to move into the purged state and
        // wait on them all.
        //
        m_SentIoEvent.Set();
    }

    if (LockSelf) {
        Unlock(irql);
    }
}

VOID
FxIoTarget::Purge(
    __in WDF_IO_TARGET_PURGE_IO_ACTION Action
    )
{
    LIST_ENTRY          pendedHead;
    SINGLE_LIST_ENTRY   sentHead;
    BOOLEAN             wait;

    InitializeListHead(&pendedHead);
    sentHead.Next = NULL;
    wait = FALSE;

    GotoPurgeState(Action, &pendedHead, &sentHead, &wait, TRUE);

    //
    // Complete any requests we might have pulled off of our lists
    //
    CompletePendedRequestList(&pendedHead);
    _CancelSentRequests(&sentHead);

    if (wait) {
        KIRQL irql;

        //
        // Under the lock, if wait is set, m_WaitingForSentIo is true, but once
        // we drop the lock, all pended i/o could have already been canceled
        // and m_WaitingForSentIo is FALSE at this point.
        //
        // ASSERT(m_WaitingForSentIo);

        WaitForSentIoToComplete();

        //
        // Mark that we are no longer purging and waiting for i/o to complete.
        //
        Lock(&irql);
        m_WaitingForSentIo = FALSE;
        Unlock(irql);
    }

    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFIOTARGET %p purged ",GetHandle());
}

VOID
FxIoTarget::GotoRemoveState(
    __in WDF_IO_TARGET_STATE NewState,
    __in PLIST_ENTRY PendedRequestListHead,
    __in PSINGLE_LIST_ENTRY SentRequestListHead,
    __in BOOLEAN Lock,
    __out PBOOLEAN Wait
    )
{
    KIRQL irql;
    BOOLEAN sentAdded, ignoredAdded;

    irql = PASSIVE_LEVEL;

    if (Lock) {
        this->Lock(&irql);
    }

    if (m_WaitingForSentIo) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIOTARGET,
            "WDFIOTARGET %p is being deleted while it is being stopped/purged "
            "by another thread. Driver must wait for WdfIoTargetStop or "
            "WdfIoTargetPurge to complete before deleting the object.",
            GetObjectHandle());

        ASSERT(Lock);

        Unlock(irql);
        WaitForSentIoToComplete();
        this->Lock(&irql);

        ASSERT(m_WaitingForSentIo == FALSE);
    }

    *Wait = FALSE;
    m_State = NewState;

    //
    // FALSE - requests will not be resent to the target.  As such,
    //         cancellation ownership will not be claimed b/c the request
    //         will subsequently be passed to FailPendedRequest.
    //
    DrainPendedRequestsLocked(PendedRequestListHead, FALSE);

    //
    // Now cancel all of the sent I/O since we are being removed for good.
    //
    if (NewState == WdfIoTargetDeleted || NewState == WdfIoTargetClosed ||
        NewState == WdfIoTargetClosedForQueryRemove) {
        //
        // If the caller is aggregating calls to GotoRemoveState among many
        // targets (for instance, WDFUSBDEVICE will do this over its WDFUSBPIPEs
        // on remove), we cannot use the state of SentRequestListHead after these
        // two calls as a test to see if any requests are being canceled (and the
        // diff between removing and removed).  Instead, we rely on the added
        // state returned by each call.
        //
        GetSentRequestsListLocked(SentRequestListHead,
                                  &m_SentIoListHead,
                                  &sentAdded);
        GetSentRequestsListLocked(SentRequestListHead,
                                  &m_IgnoredIoListHead,
                                  &ignoredAdded);

        if (sentAdded || ignoredAdded) {
            //
            // We will have to wait for the i/o to come back.  As such, we are
            // in the transition state and must wait for it to complete.  In this
            // transitionary stage, I/O can still be sent if they are marked to
            // ignore the target state.
            //
            m_Removing = TRUE;
            *Wait = TRUE;

            //
            // If Stop(leave sent io pending) or Purge(do-not-wait) was
            // previously called, m_SentIoEvent will be in the signalled state.
            // We need to wait for sent i/o to be completed, so make sure it
            // is not signalled while hodling the lock
            //
            m_SentIoEvent.Clear();
        }
        else {
            ClearTargetPointers();

            //
            // Even though *Wait is set to FALSE, the caller may wait anyways
            // if it is aggregating targets to move into the remove state and
            // wait on all of them.
            //
            m_SentIoEvent.Set();
        }
    }

    if (Lock) {
        Unlock(irql);
    }

    return;
}

VOID
FxIoTarget::Remove(
    VOID
    )
{
    SINGLE_LIST_ENTRY sentHead;
    LIST_ENTRY pendedHead;
    BOOLEAN wait;

    sentHead.Next = NULL;
    InitializeListHead(&pendedHead);

    GotoRemoveState(WdfIoTargetDeleted,
                             &pendedHead,
                             &sentHead,
                             TRUE,
                             &wait);

    //
    // Complete any requests we might have pulled off of our lists
    //
    CompletePendedRequestList(&pendedHead);
    _CancelSentRequests(&sentHead);

    if (wait) {
        ASSERT(m_State == WdfIoTargetDeleted);

        WaitForSentIoToComplete();
    }

    WaitForDisposeEvent();

    return;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::QueryInterface(
    __inout FxQueryInterfaceParams* Params
    )
{
    switch (Params->Type) {
    case FX_TYPE_IO_TARGET:
        *Params->Object = (FxIoTarget*) this;
        break;
    default:
        return FxNonPagedObject::QueryInterface(Params); // __super call
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::Init(
    __in CfxDeviceBase* Device
    )
{
    NTSTATUS status;

    status = InitModeSpecific(Device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    SetDeviceBase(Device);
    MxDeviceObject deviceObject;

    m_InStackDevice = Device->GetDeviceObject();

    //
    // Note that AttachedDevice can be NULL for UMDF for
    // example when there is only one device in the stack.
    //
    m_TargetDevice = GetTargetDeviceObject(Device);
    m_TargetPdo = Device->GetPhysicalDevice();

    m_Driver = Device->GetDriver();

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    if (m_InStackDevice == NULL || m_Driver == NULL ||
        m_TargetDevice == NULL || m_TargetPdo == NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Init WDFIOTARGET %p, unexpected NULL, m_InStackDevice %p, "
            "m_TargetDevice %p, m_TargetPdo %p, m_Driver %p",
            GetObjectHandle(), m_InStackDevice, m_TargetDevice, m_TargetPdo,
            m_Driver);

        return STATUS_UNSUCCESSFUL;
    }
#else
    if (m_InStackDevice == NULL || m_Driver == NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Init WDFIOTARGET %p, unexpected NULL, m_InStackDevice %p, "
            "m_Driver %p",
            GetObjectHandle(), m_InStackDevice, m_Driver);

        return STATUS_UNSUCCESSFUL;
    }
#endif

    //
    // For UMDF the target device can be NULL if there is only one driver in the
    // stack. In that case m_TargetStackSize retains its initial value (0).
    //
    if (m_TargetDevice != NULL) {
        deviceObject.SetObject(m_TargetDevice);

        m_TargetStackSize = deviceObject.GetStackSize();

        m_TargetIoType = GetTargetIoType();
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxIoTarget, VerifySubmitLocked) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequestBase* Request
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KIRQL irql;
    SHORT flags;
    FxIrp* irp;

    PAGED_CODE_LOCKED();

    irp = Request->GetSubmitFxIrp();
    Request->Lock(&irql);
    flags = Request->GetVerifierFlagsLocked();

    if ((flags & FXREQUEST_FLAG_FORMATTED) == 0x0) {
       status = STATUS_REQUEST_NOT_ACCEPTED;

       DoTraceLevelMessage(
           FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
           "WDFREQUEST %p has not been formatted, cannot send, %!STATUS!",
           Request->GetTraceObjectHandle(), status);
    }
    else if (flags & FXREQUEST_FLAG_SENT_TO_TARGET) {
       //
       // Technically this is the same check as m_IrpCompletionReferenceCount
       // above, but we make this check in many more locations.
       //
       DoTraceLevelMessage(
           FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
           "WDFREQUEST %p is already pending on a WDFIOTARGET",
           Request->GetTraceObjectHandle());

       FxVerifierBugCheck(FxDriverGlobals,
                          WDF_REQUEST_FATAL_ERROR,
                          WDF_REQUEST_FATAL_ERROR_REQUEST_ALREADY_SENT,
                          (ULONG_PTR) Request->GetHandle());
    }
    else if (HasEnoughStackLocations(irp) == FALSE) {
       status = STATUS_REQUEST_NOT_ACCEPTED;

       //
       // For reasons why we subtract 1 from CurrentLocation, see comments
       // in FxIoTarget::HasEnoughStackLocations.
       //
       DoTraceLevelMessage(
           FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
           "WDFREQUEST %p, PIRP %p does not have enough stack locations %d"
           " for this WDFIOTARGET %p (requires %d locations),  %!STATUS!",
           Request->GetTraceObjectHandle(), irp->GetIrp(), irp->GetCurrentIrpStackLocationIndex() - 1,
           GetHandle(), m_TargetStackSize, status);
    }

    Request->Unlock(irql);
    return status;
}

ULONG
FxIoTarget::SubmitLocked(
    __in FxRequestBase* Request,
    __in_opt PWDF_REQUEST_SEND_OPTIONS Options,
    __in ULONG Flags
    )
/*++

Routine Description:
    Core processing logic for submitting a request.  Will return the send flag
    if the request can be submitted immediately.  If the flag is not returned,
    the pended flag may be set.  If neither are set, status in the Request will
    be set with the error.

    NTSTATUS status;
    ULONG action;

    Lock();
    action |= SubmitLocked(...);
    UnLock();

    if (action & Send) {
        // IoCallDriver ....
    }
    else if (action & Pended) {
        // request was pended
    }

    return ...;

Arguments:
    Request - The request that will be submitted to the target

    Options - send options associated with the request being sent

    Flags - Additional flags to control how the request is being sent

Return Value:
    A bit field whose flags are defined by SubmitActionFlags

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    ULONG action;
    BOOLEAN startTimer, stateIgnored, verify;
    BOOLEAN addedRef;
    FxIrp* irp;

    pFxDriverGlobals = GetDriverGlobals();

    action = 0;
    startTimer = FALSE;
    stateIgnored = FALSE;
    addedRef = FALSE;

    //
    // If the reference count is not zero, the irp has not completed and the
    // driver is reusing it before it has returned to the driver.  Not good!
    //
    ASSERT(Request->m_IrpCompletionReferenceCount == 0);
    if (Request->m_IrpCompletionReferenceCount != 0) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFREQUEST %p already sent to a target",
            Request->GetTraceObjectHandle());

        //
        // Last ditch assert
        //
        ASSERT((Request->GetTargetFlags() & FX_REQUEST_PENDED) == 0);

        // no return
        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_REQUEST_FATAL_ERROR,
                           WDF_REQUEST_FATAL_ERROR_REQUEST_ALREADY_SENT,
                           (ULONG_PTR) Request->GetHandle());
    }

    irp =  Request->GetSubmitFxIrp();

    if (pFxDriverGlobals->FxVerifierOn &&
        pFxDriverGlobals->FxVerifierIO) {

        verify = TRUE;
        status = VerifySubmitLocked(pFxDriverGlobals, Request);
        if (!NT_SUCCESS(status)){
            goto Done;
        }
    }
    else {
        verify = FALSE;
    }

    //
    // if WDF_REQUEST_SEND_OPTION_TIMEOUT is set, Options != NULL
    //
    if ((Flags & WDF_REQUEST_SEND_OPTION_TIMEOUT) && Options->Timeout != 0) {
        //
        // Create the timer under the lock
        //
        status = Request->CreateTimer();

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFREQUEST %p, could not create timer, %!STATUS!",
                Request->GetTraceObjectHandle(), status);

            goto Done;
        }

        startTimer = TRUE;
    }

    if (Flags & WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE) {
        //
        // If we are in the deleted or closed state, we must be in the transitioning
        // state to allow I/O at this time.  For any other state, always allow
        // the i/o to go through.
        //
        if ((m_State == WdfIoTargetDeleted ||
             m_State == WdfIoTargetClosed ||
             m_State == WdfIoTargetClosedForQueryRemove) &&
            m_Removing == FALSE) {
            //
            // The target is truly closed or removed, it is not in the
            // transitionary state.   We don't allow I/O anymore.
            //
            status = STATUS_INVALID_DEVICE_STATE;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                "WDFIOTARGET %p state %!WDF_IO_TARGET_STATE!, sending "
                "WDFREQUEST %p cannot ignore current state, %!STATUS!",
                GetObjectHandle(), m_State, Request->GetTraceObjectHandle(),
                status);

            goto Done;
        }
        else {
            action |= SubmitSend;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                "ignoring WDFIOTARGET %p state, sending WDFREQUEST %p, state "
                "%!WDF_IO_TARGET_STATE!",
                GetObjectHandle(), Request->GetTraceObjectHandle(), m_State);

            Request->SetTargetFlags(FX_REQUEST_IGNORE_STATE);

            status = STATUS_SUCCESS;
            stateIgnored = TRUE;
        }
    }
    else {
        switch (m_State) {
        case WdfIoTargetStarted:
            status = STATUS_SUCCESS;
            action |= SubmitSend;
            break;

        case WdfIoTargetStopped:
            if (Flags & WDF_REQUEST_SEND_INTERNAL_OPTION_FAIL_ON_PEND) {
                status = STATUS_INVALID_DEVICE_STATE;
                goto Done;
            }
            else {
                status = STATUS_WDF_QUEUED;
                action |= SubmitQueued;
            }
            break;

        case WdfIoTargetClosedForQueryRemove:
        case WdfIoTargetClosed:
        case WdfIoTargetDeleted:
        case WdfIoTargetPurged:
        default:
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "failing WDFREQUEST %p, WDFIOTARGET %p not accepting requests, "
                "state %!WDF_IO_TARGET_STATE!", Request->GetTraceObjectHandle(),
                GetObjectHandle(), m_State);

            status = STATUS_INVALID_DEVICE_STATE;
            goto Done;
        }
    }

    //
    // Make sure the list entry is initialized so if we call RemoveEntryList
    // later, we don't corrupt whatever Flink and Blink point to.
    //
    InitializeListHead(&Request->m_ListEntry);

    ASSERT(((action & SubmitSend) || (action & SubmitQueued)) && NT_SUCCESS(status));

Done:
    if (NT_SUCCESS(status)) {
        //
        // Request should not be pended
        //
        ASSERT((Request->GetTargetFlags() & FX_REQUEST_PENDED) == 0);

        //
        // Set m_Target before setting the reference count to one because as soon
        // as it is set to 1, it can be decremented to zero and then it will
        // assume that m_Target is valid.
        //
        Request->SetTarget(this);

        //
        // Assume we are successful, we will adjust this value in case of error.
        //
        IncrementIoCount();

        //
        // All paths which are pulling the request off of the list must use this
        // specific tag.  This would include the cancel, timer, and completion
        // routines.
        //
        // We don't add a reference in the error case because the caller
        // will just complete the request back to the caller (for a queue
        // presented request) or free it (for a driver created request).
        //
        // This released at the end of FxRequestBase::CompleteSubmitted
        //
        Request->ADDREF(this);

        //
        // In case of error, we use this flag to know if the IoCount and
        // RequestRef need to be rolled back.
        //
        addedRef = TRUE;

        //
        // Set the reference count to one.  This reference count guards prevents
        // Request::Cancel from touching an invalid PIRP outside of any lock.
        //
        Request->m_IrpCompletionReferenceCount = 1;

        if (Request->m_Canceled) {
            //
            // CanComplete() decrements the count that was set above.  If the
            // count goes to zero, CanComplete() returns TRUE and
            // FxRequestBase::Cancel will not touch the irp.  If it returns
            // FALSE, we indicate that the request was sent, in actuality we
            // don't send the request b/c FxRequestBase::Cancel will call
            // CompleteCanceledRequest, where the irp will complete.
            //
            if (Request->CanComplete()) {
                //
                // This thread owns the irp.  Set the status to !NT_SUCCESS and
                // clear any actions we indicate to the caller.
                //
                action = 0;
            }
            else {
                //
                // There is still an reference count on the completion count,
                // let the other thread complete it.
                //

                //
                // Make the caller think that the request was queued.  By doing
                // this, it will not attempt to call IoCallDriver.  SubmitSend
                // will be cleared after jump to Done: and evaluate status.
                //
                action |= SubmitQueued;
            }

            //
            // Either way, we want to set STATUS_CANCELLED in the PIRP  when we
            // are done.
            //
            status = STATUS_CANCELLED;

            //
            // Just jump to the end and avoid any more compares.
            //
            goto CheckError;
        }

        if (action & SubmitSend) {
            if (stateIgnored) {
                InsertTailList(&m_IgnoredIoListHead, &Request->m_ListEntry);
            }
            else {
                //
                // Keep track of the request so that we can cancel it later if needed
                //
                InsertTailList(&m_SentIoListHead, &Request->m_ListEntry);
            }

            //
            // We know we are going to send the request, set the completion
            // routine now.  Since IoSetCompletionRoutineEx allocates memory
            // which is only freed when the completion routine is called when
            // the request is completing, we can only set the CR when we *KNOW*
            // the request will be sent, ie SubmitSend is set and returned to
            // the caller.
            //
            SetCompletionRoutine(Request);

            //
            // NOTE: No need to reference the file object before we drop the lock
            // because will not deref the file object while there is outstanding
            // I/O.
            //
        }
        else {
            status = PendRequestLocked(Request);
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                "Pending WDFREQUEST %p, WDFIOTARGET %p is paused, %!STATUS!",
                Request->GetTraceObjectHandle(), GetObjectHandle(), status);

            if (!NT_SUCCESS(status)) {
                //
                // CanComplete() decrements the count that was set above.  If the
                // count goes to zero, CanComplete() returns TRUE and
                // FxRequestBase::Cancel will not touch the irp.  If it returns
                // FALSE, we indicate that the request was sent, in actuality we
                // don't send the request b/c FxRequestBase::Cancel will call
                // CompleteCanceledRequest, where the irp will complete.
                //
                if (Request->CanComplete()) {
                    //
                    // This thread owns the irp.
                    // Clear any actions we indicate to the caller.
                    //
                    action = 0;
                }
                else {
                    //
                    // The cancel/timer routine (whoever has ownership of
                    // request) will complete the request.
                    //
                    ASSERT(action & SubmitQueued);
                    DO_NOTHING();
                }
            }
        }

        if (NT_SUCCESS(status)) {
            //
            // Delay starting the timer to the last possible moment where we know
            // there will be no error and we don't have to deal with any cancel
            // logic in the error case.
            //
            if (startTimer) {
                ASSERT(action & (SubmitSend | SubmitQueued));

                //
                // Set the timer under the lock
                //
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                    "Starting timer on WDFREQUEST %p",
                    Request->GetTraceObjectHandle());

                Request->StartTimer(Options->Timeout);
            }
        }
    }

CheckError:
    //
    // Not an else clause to the if (NT_SUCCESS(status)) above b/c status can
    // be changed within the NT_SUCCESS(status) clause.
    //
    if (!NT_SUCCESS(status)) {
        irp->SetStatus(status);
        action &= ~SubmitSend;
    }
    else if (verify) {
        Request->SetVerifierFlags(FXREQUEST_FLAG_SENT_TO_TARGET);
    }

    //
    // Keep the IoCount and Request->AddRef() only if the request is going
    // to be sent, or it is queued, or another thread took ownership of its
    // cancellation.
    //
    if (addedRef && (action & (SubmitSend | SubmitQueued)) == 0) {
        Request->RELEASE(this);
        DecrementIoCount();
    }

    return action;
}

ULONG
FxIoTarget::Submit(
    __in FxRequestBase* Request,
    __in_opt PWDF_REQUEST_SEND_OPTIONS Options,
    __in_opt ULONG Flags
    )
{
    ULONG result;
    KIRQL irql;

    Lock(&irql);
    result = SubmitLocked(Request, Options, Flags);
    Unlock(irql);

    return result;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::SubmitSync(
    __in FxRequestBase* Request,
    __in_opt PWDF_REQUEST_SEND_OPTIONS Options,
    __out_opt PULONG Action
    )
{
    FxTargetSubmitSyncParams params = {0};
    LONGLONG timeout;
    ULONG action;
    NTSTATUS status;
    KIRQL irql;
    BOOLEAN clearContext;

    status = STATUS_SUCCESS;
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFIOTARGET %p, WDFREQUEST %p",
                        GetObjectHandle(), Request->GetTraceObjectHandle());

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    //
    // FxCREvent events needs to be initiliazed in UMDF, and failure handled
    // gracefully. For KMDF, it will result in double initialization which is
    // not a problem. Note that for KMDF, FxCREvent->Initialize will never fail.
    //
    status = params.SynchEvent.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize sync event for "
                            "WDFIOTARGET %p, WDFREQUEST %p",
                            GetObjectHandle(), Request->GetTraceObjectHandle());
        if (Action != NULL) {
            *Action = 0;
        }
        return status;
    }
#endif

    clearContext = Request->ShouldClearContext();

    if (Action != NULL) {
        action =  *Action;
    }
    else {
        action = 0;
    }

    if (Options != NULL &&
        (Options->Flags & WDF_REQUEST_SEND_OPTION_TIMEOUT) &&
        Options->Timeout != 0) {
        //
        // If this flag is set, SubmitLocked will start a timer, which we don't
        // want because we will timeout the I/O using KeWaitForSingleObject.
        //
        // params.Constraints &= ~WDF_REQUEST_SEND_OPTION_TIMEOUT;
        timeout = Options->Timeout;
        action |= SubmitTimeout;
    }

    //
    // Must set the completion routine before calling Submit() so that in the
    // pended or sent case, the completion routine is set in place during
    // cancelation or delayed completion.
    //
    if (action & SubmitSyncCallCompletion) {
        params.OrigTargetCompletionContext = Request->m_TargetCompletionContext;
        params.OrigTargetCompletionRoutine =
            Request->m_CompletionRoutine.m_Completion;
    }
    else {
        params.OrigTargetCompletionContext = NULL;
        params.OrigTargetCompletionRoutine = NULL;
    }

    Request->SetCompletionRoutine(_SyncCompletionRoutine, &params);

    //
    // SubmitLocked will return whether the request should be sent *right now*.
    // If SubmitSend is clear, SubmitQueued must be checked.  If set, then
    // the request was queued, otherwise, the request has failed and the
    // status was already set in the irp.
    //
    // Clear the WDF_REQUEST_SEND_OPTION_TIMEOUT flag so that SumbitLocked doesn't
    // try to allocate a timer
    //
    action |= Submit(
        Request,
        Options,
        (Options != NULL) ? (Options->Flags & ~WDF_REQUEST_SEND_OPTION_TIMEOUT) : 0);

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFREQUEST %p, Action 0x%x", Request->GetTraceObjectHandle(),
                        action);

    //
    // Add reference so that if we call Request->Cancel(), Request is still
    // a valid object in between the wait timeout and the cancel call if
    // request completes before Cancel is called.
    //
    Request->ADDREF(&status);

    if (action & SubmitSend) {
        action |= SubmitSent;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Sending WDFREQUEST %p, Irp %p", Request->GetTraceObjectHandle(),
            Request->GetSubmitIrp());

        Send(Request->GetSubmitIrp());

        //
        // We always wait, even in the synchronous case.  We do this because
        // even though the WDM completion routine ran synchronously in this
        // thread, the WDF processing of the completion could have been post-
        // poned by another thread attempting to cancel the I/O.  The postpone-
        // ment would occur when the canceling thread has an oustanding reference
        // on m_IrpCompletionReferenceCount, which would cause the call to
        // CanComplete() in RequestCompletionRoutine() to return FALSE and not
        // call _SyncCompletionRoutine in the context of the WDM completion
        // routine, but in the context of the canceling thread.
        //
        action |= SubmitWait;








































    }
    else if (action & SubmitQueued) {
        //
        // To the caller, we say we sent the request (and all the cancel
        // semantics of a sent request still work).
        //
        action |= (SubmitSent | SubmitWait);
    }
    else if (action & SubmitSyncCallCompletion) {
        //
        // The request was not sent nor queued, reset the completion routine
        // since we overwrote it.
        //
        Request->m_TargetCompletionContext = params.OrigTargetCompletionContext;
        Request->m_CompletionRoutine.m_Completion =
            params.OrigTargetCompletionRoutine;
        ASSERT(!NT_SUCCESS(Request->GetSubmitFxIrp()->GetStatus()));
    }

    if (action & SubmitSent) {
        if (action & SubmitWait) {
            status = params.SynchEvent.EnterCRAndWaitAndLeave(
                (action & SubmitTimeout) ? &timeout : NULL
                );

            if (status == STATUS_TIMEOUT) {
                //
                // By setting FX_REQUEST_CANCELLED_FROM_TIMER, we match the
                // async timer behavior where we change the completion status
                // from STATUS_CANCELLED to STATUS_IO_TIMEOUT.
                //
                Lock(&irql);
                Request->SetTargetFlags(FX_REQUEST_CANCELLED_FROM_TIMER);
                Unlock(irql);

                Request->Cancel();

                params.SynchEvent.EnterCRAndWaitAndLeave();
            }
        }

        status = params.Status;
    }
    else {
        status = Request->GetSubmitFxIrp()->GetStatus();
    }

    Request->RELEASE(&status);

    if (Action != NULL) {
        *Action = action;
    }

    if (clearContext) {
        Request->ContextReleaseAndRestore();
    }

    return status;
}

VOID
FxIoTarget::FailPendedRequest(
    __in FxRequestBase* Request,
    __in NTSTATUS Status
    )
/*++

Routine Description:
    Completes a request that has failed due to timer expiration or cancellation.

Assumes:
    Assumes that the caller has undone the effects of the
    IoSetNextIrpStackLocation made when we enqueued the request.

Arguments:
    Request - request that failed

    Status  - the status to set in the request

    TakeReference - add a reference before completing the request

Return Value:
    None.

  --*/
{
    FxIrp* irp;

    irp = Request->GetSubmitFxIrp();

    //
    // Simulate failure in the IRP
    //
    irp->SetStatus(Status);
    irp->SetInformation(0);

    //
    // Manaully process the irp as if it has completed back from the target.
    //
    RequestCompletionRoutine(Request);
}

BOOLEAN
FxIoTarget::RemoveCompletedRequestLocked(
    __in FxRequestBase* Request
    )
/*++

Routine Description:
    Removes a previously sent request from the bookkeeping structures

Arguments:
    Request - The request being completed

Assumes:
    This object's Lock is being held by the caller.

Return Value:
    TRUE if the m_SentIoEvent should be set after the caller has released the
    object lock.

  --*/
{
    ULONG oldFlags;

    //
    // We will decrement the pending io count associated with this completed
    // request in FxIoTarget::CompleteRequest
    //
    // DecrementPendingIoCount();

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFIOTARGET %p, WDFREQUEST %p", GetObjectHandle(),
                        Request->GetTraceObjectHandle());

    RemoveEntryList(&Request->m_ListEntry);

    //
    // The request expects not to be on a list when it is destroyed.
    //
    InitializeListHead(&Request->m_ListEntry);

    //
    // By the time we get here, there should never ever be a timer set for the
    // request.
    //
    ASSERT((Request->GetTargetFlags() & FX_REQUEST_TIMER_SET) == 0);

    //
    // Clear flags that may have been set previously.
    //
    oldFlags = Request->ClearTargetFlags(FX_REQUEST_COMPLETED |
                                         FX_REQUEST_TIMER_SET |
                                         FX_REQUEST_CANCELLED_FROM_TIMER |
                                         FX_REQUEST_IGNORE_STATE);

    ClearCompletedRequestVerifierFlags(Request);

    //
    // If we are removing, we must wait for *ALL* requests that were sent down
    // the stack.
    //
    // If we are stopping, we only wait for i/o which do not ignore state
    //
    // NOTE:  if we are completing a request which was inserted onto a list
    //        Cancel()'ed before SubmitLocked was called and the Cancel()
    //        thread already had a completion reference taken we are going to
    //        evaluate a state transition even though the request is not a part
    //        in that transition.  I think this is OK b/c the transition would
    //        have already occurred if the request(s) holding up the transition
    //        have completed and will not occur here if they are still pending.
    //
    if (m_Removing) {
        if (IsListEmpty(&m_SentIoListHead) && IsListEmpty(&m_IgnoredIoListHead)) {
            //
            // We are no longer transitioning, do not allow new I/O of any kind
            // to come in.
            //
            m_Removing = FALSE;

            //
            // Now that all i/o has ceased, clear out our pointers with relation
            // to the target itself.
            //
            ClearTargetPointers();

            return TRUE;
        }
    }
    else if (m_WaitingForSentIo &&
             (oldFlags & FX_REQUEST_IGNORE_STATE) == 0 &&
             IsListEmpty(&m_SentIoListHead)) {
        m_WaitingForSentIo = FALSE;
        return TRUE;
    }

    return FALSE;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::PendRequestLocked(
    __in FxRequestBase* Request
    )
{
    NTSTATUS status;
    FxIrp* irp;

    //
    // Assumes this object's lock is being held
    //
    Request->SetTargetFlags(FX_REQUEST_PENDED);

    irp = Request->GetSubmitFxIrp();

    //
    // Make sure there is a valid current stack location in the irp.  If we
    // allocated the irp ourself, then the current stack location is not valid.
    // Even if we didn't allocate the irp ourself, this will do no harm.  In
    // every spot where we remove the request, we undo this call with a call to
    // IoSkipCurrentIrpStackLocation
    //
    irp->SetNextIrpStackLocation();

    ASSERT(irp->IsCurrentIrpStackLocationValid());

    status = m_PendedQueue.InsertTailRequest(irp->GetIrp(), &Request->m_CsqContext, NULL);

    if (!NT_SUCCESS(status)) {
        //
        // Undo the affects of the IoSetNextIrpStackLocation made when we
        // enqueued the request.
        //
        irp->SkipCurrentIrpStackLocation();

        //
        // Request was not pended.
        //
        Request->ClearTargetFlags(FX_REQUEST_PENDED);
    }

    return status;
}

VOID
FxIoTarget::TimerCallback(
    __in FxRequestBase* Request
    )
/*++

Routine Description:
    Timer routine for when a request has timed out.  This routine will attempt
    to cancel the request if it hasn't yet completed or complete it if it has.

Arguments:
    Request - The request that has timed out

Return Value:
    None.

  --*/

{
    KIRQL irql;
    BOOLEAN completeRequest, setStopEvent;
    LONG completionRefCount;

    completeRequest = FALSE;
    setStopEvent = FALSE;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFIOTARGET %p, WDFREQUEST %p", GetObjectHandle(),
                        Request->GetTraceObjectHandle());

    Lock(&irql);

    //
    // Clear the flag so that when the completion routine runs, there is no attempt
    // to cancel this timer.
    //
    Request->ClearTargetFlags(FX_REQUEST_TIMER_SET);

    if (Request->GetTargetFlags() & FX_REQUEST_COMPLETED) {
        //
        // Completion routine ran on a separate processor as the same time as
        // the timer DPC.  The completion routine will have deferred
        // completion to the timer DPC or the caller of Request::Cancel().
        //
        completeRequest = Request->CanComplete();
    }
    else {
        //
        // Attempt to cancel the request later outside of the lock.  By setting
        // the cancelled from timer flag, the completion routine can morph the
        // status to timeout if the request is returned as cancelled.
        //
        Request->SetTargetFlags(FX_REQUEST_CANCELLED_FROM_TIMER);

        //
        // Make sure the completion routine does not complete the request
        // while the timer callback is still running, in case the completion
        // is invoked in the unlock/lock window below.
        //
        completionRefCount = FxInterlockedIncrementGTZero(
                                &Request->m_IrpCompletionReferenceCount);
        ASSERT(completionRefCount != 0);
        UNREFERENCED_PARAMETER(completionRefCount);

        Unlock(irql);

        Request->Cancel();

        Lock(&irql);

        //
        // CanComplete() returns true if completion ownership was claimed.
        //
        completeRequest = Request->CanComplete();
    }

    //
    // If completion ownership was claimed, complete request.
    //
    if (completeRequest) {
        ASSERT(Request->GetTargetFlags() & FX_REQUEST_COMPLETED);

        setStopEvent = RemoveCompletedRequestLocked(Request);

        if (Request->m_Irp.GetStatus() == STATUS_CANCELLED) {
            //
            // We cancelled the request in another thread and the timer
            // fired at the same time.  Treat this as if we did the cancel
            // from timer directly.
            //
            // Morph the status code into a timeout status.
            //
            // Don't muck with the IoStatus.Information field.
            //
            Request->m_Irp.SetStatus(STATUS_IO_TIMEOUT);
        }
    }

    Unlock(irql);

    if (completeRequest) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFREQUEST %p completed in timer callback",
            Request->GetTraceObjectHandle());
        CompleteRequest(Request);
    }

    if (setStopEvent) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFIOTARGET %p, setting stop event %p in timer callback",
            GetObjectHandle(), m_SentIoEvent.GetEvent());

        m_SentIoEvent.Set();
    }

    if (completeRequest) {
        DecrementIoCount();
    }
}

VOID
FxIoTarget::CompleteCanceledRequest(
    __in FxRequestBase* Request
    )
{
    KIRQL irql;
    BOOLEAN setStopEvent;

    Lock(&irql);

    //
    // RemoveCompletedRequestLocked clears Request->m_TargetFlags, so we must
    // do this check before that call.
    //
    if ((Request->GetTargetFlags() & FX_REQUEST_CANCELLED_FROM_TIMER) &&
        Request->m_Irp.GetStatus() == STATUS_CANCELLED) {
        //
        // We cancelled the request from the timer and it has completed with
        // cancelled.  Morph the status code into a timeout status.
        //
        // Don't muck with the IoStatus.Information field.
        //
        Request->m_Irp.SetStatus(STATUS_IO_TIMEOUT);
    }

    setStopEvent = RemoveCompletedRequestLocked(Request);
    Unlock(irql);
    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFREQUEST %p completed in from cancel",
                        Request->GetTraceObjectHandle());
    CompleteRequest(Request);

    if (setStopEvent) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFIOTARGET %p, setting stop event %p",
                            GetObjectHandle(), m_SentIoEvent.GetEvent());

        m_SentIoEvent.Set();
    }

    DecrementIoCount();
}

VOID
FxIoTarget::HandleFailedResubmit(
    __in FxRequestBase* Request
    )
/*++

Routine Description:
    This function handles the completion of the request when Submit() fails.
    Request is tracked by the 'Io Count' counter, caller is responsible for
    updating its value.

Arguments:
    Request - The request being completed.

Return Value:
    None.

  --*/
{
    KIRQL irql;
    BOOLEAN setStopEvent;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFREQUEST %p", Request->GetTraceObjectHandle());

    setStopEvent = FALSE;

    Lock(&irql);

    //
    // Flag should be clear until we set it below
    //
    ASSERT((Request->GetTargetFlags() & FX_REQUEST_COMPLETED) == 0);

    //
    // Mark that the request has been completed
    //
    Request->SetTargetFlags(FX_REQUEST_COMPLETED);

    //
    // Timer should not have been started.
    //
    ASSERT((Request->GetTargetFlags() & FX_REQUEST_TIMER_SET) == 0);

    //
    // RemoveCompletedRequestLocked clears Request->m_TargetFlags, so we must
    // do this check before that call.
    //
    if ((Request->GetTargetFlags() & FX_REQUEST_CANCELLED_FROM_TIMER) &&
        Request->m_Irp.GetStatus() == STATUS_CANCELLED) {
        //
        // We cancelled the request from the timer and it has completed with
        // cancelled.  Morph the status code into a timeout status.
        //
        // Don't muck with the IoStatus.Information field.
        //
        Request->m_Irp.SetStatus(STATUS_IO_TIMEOUT);
    }

    setStopEvent = RemoveCompletedRequestLocked(Request);

    Unlock(irql);

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFREQUEST %p completed in completion routine",
                        Request->GetTraceObjectHandle());
    CompleteRequest(Request);

    if (setStopEvent) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFIOTARGET %p, setting stop event %p",
                            GetObjectHandle(), m_SentIoEvent.GetEvent());
        m_SentIoEvent.Set();
    }

    DecrementIoCount();
}

VOID
FxIoTarget::RequestCompletionRoutine(
    __in FxRequestBase* Request
    )
/*++

Routine Description:
    The previously submitted request has been completed.  This function will
    handle coordination with the (optional) request timer and the potential
    simultaneous call to FxRequest::Cancel as to which function
    will actually complete the request.

Arguments:
    Request - The request being completed.

Return Value:
    None.

  --*/
{
    KIRQL irql;
    BOOLEAN completeRequest, setStopEvent;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFREQUEST %p", Request->GetTraceObjectHandle());


    setStopEvent = FALSE;
    completeRequest = FALSE;

    Lock(&irql);

    //
    // Flag should be clear until we set it below
    //
    ASSERT((Request->GetTargetFlags() & FX_REQUEST_COMPLETED) == 0);

    //
    // Mark that the request has been completed so that the potential timer
    // DPC will handle the case properly
    //
    Request->SetTargetFlags(FX_REQUEST_COMPLETED);

    //
    // CancelTimer() returns TRUE if the timer was successfully canceled (if
    // queued) or if no timer was queued.
    //
    if (Request->CancelTimer()) {
        //
        // Sync with Request->Cancel() to make sure we can delete the request.
        //
        completeRequest = Request->CanComplete();
    }

    if (completeRequest) {
        //
        // RemoveCompletedRequestLocked clears Request->m_TargetFlags, so we must
        // do this check before that call.
        //
        if ((Request->GetTargetFlags() & FX_REQUEST_CANCELLED_FROM_TIMER) &&
            Request->m_Irp.GetStatus() == STATUS_CANCELLED) {
            //
            // We cancelled the request from the timer and it has completed with
            // cancelled.  Morph the status code into a timeout status.
            //
            // Don't muck with the IoStatus.Information field.
            //
            Request->m_Irp.SetStatus(STATUS_IO_TIMEOUT);
        }

        setStopEvent = RemoveCompletedRequestLocked(Request);
    }
    else {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFREQUEST %p deferring completion due to outstanding completion "
            "references", Request->GetTraceObjectHandle());
    }

    Unlock(irql);

    if (completeRequest) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFREQUEST %p completed in completion routine",
                            Request->GetTraceObjectHandle());
        CompleteRequest(Request);
    }

    if (setStopEvent) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFIOTARGET %p, setting stop event %p",
                            GetObjectHandle(), m_SentIoEvent.GetEvent());
        m_SentIoEvent.Set();
    }

    if (completeRequest) {
        DecrementIoCount();
    }
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::_RequestCompletionRoutine(
    MdDeviceObject DeviceObject,
    MdIrp Irp,
    PVOID Context
    )
/*++

Routine Description:
    Generic I/O completion routine for all submitted requests.

Arguments:
    DeviceObject - Our device object.  Most likely NULL since we created the
        request and we are the top most driver with respect to it
    Irp - Request itself.  Ignored since the context also contains this value
    Context - Our context, FxRequestBase*.

Return Value:
    STATUS_MORE_PROCESSING_REQUIRED since the lifetime of the Irp is controlled
    by the lifetime of our context which may outlive this function call.

  --*/
{
    FxIoTarget* pThis;
    FxRequestBase* pRequest;

    FxIrp irp(Irp);

    UNREFERENCED_PARAMETER(DeviceObject);

    pRequest = (FxRequestBase*) Context;
    pThis = pRequest->m_Target;

    //
    // Only propagate the the pending returned bit in the IRP if this is an
    // asynchronous request
    //
    if (pRequest->m_CompletionRoutine.m_Completion !=
                                                    _SyncCompletionRoutine) {
        irp.PropagatePendingReturned();
    }

    pThis->RequestCompletionRoutine(pRequest);

    return STATUS_MORE_PROCESSING_REQUIRED;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::FormatInternalIoctlOthersRequest(
    __in FxRequestBase* Request,
    __in ULONG Ioctl,
    __in FxRequestBuffer* Buffers
    )
{
    FxInternalIoctlOthersContext *pContext;
    PVOID* bufs[FX_REQUEST_NUM_OTHER_PARAMS];
    NTSTATUS status;
    ULONG i;
    FxIrp* irp;

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Request->HasContextType(FX_RCT_INTERNAL_IOCTL_OTHERS)) {
        pContext = (FxInternalIoctlOthersContext*) Request->GetContext();
    }
    else {
        pContext = new(GetDriverGlobals()) FxInternalIoctlOthersContext();

        if (pContext == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not allocate context for request");

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Request->SetContext(pContext);
    }

    //
    // Save away any references to IFxMemory pointers that are passed.
    // (StoreAndReferenceMemory can only store one buffer, so it doesn't help).
    //
    pContext->StoreAndReferenceOtherMemories(&Buffers[0],
                                             &Buffers[1],
                                             &Buffers[2]);


    irp = Request->GetSubmitFxIrp();
    irp->ClearNextStackLocation();

    irp->SetMajorFunction(IRP_MJ_INTERNAL_DEVICE_CONTROL);
    irp->SetParameterIoctlCode(Ioctl);

    CopyFileObjectAndFlags(Request);

    i = 0;
    bufs[i] = irp->GetNextStackParameterOthersArgument1Pointer();
    bufs[++i] = irp->GetNextStackParameterOthersArgument2Pointer();
    bufs[++i] = irp->GetNextStackParameterOthersArgument4Pointer();

    for (i = 0; i < FX_REQUEST_NUM_OTHER_PARAMS; i++) {
        status = Buffers[i].GetBuffer(bufs[i]);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not retrieve buffer %d, status %!STATUS!", i+1, status);

            Request->ContextReleaseAndRestore();

            return status;
        }
    }

    if (NT_SUCCESS(status)) {
        Request->VerifierSetFormatted();
    }

    return status;
}

VOID
FxIoTarget::_RequestCancelled(
    __in FxIrpQueue* Queue,
    __in MdIrp Irp,
    __in PMdIoCsqIrpContext CsqContext,
    __in KIRQL CallerIrql
    )
{
    FxIoTarget* pThis;
    FxRequestBase* pRequest;
    KIRQL irql;
    FxIrp pFxIrp;

    pThis = CONTAINING_RECORD(Queue, FxIoTarget, m_PendedQueue);

    pThis->Unlock(CallerIrql);

    //
    // Grab the request out of the irp.  After this call we are done with the
    // m_CsqContext field.
    //
    pRequest = FxRequestBase::_FromCsqContext(CsqContext);

    DoTraceLevelMessage(
        pRequest->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
        "Pended WDFREQUEST %p canceled", pRequest->GetTraceObjectHandle());

    //
    // m_ListEntry is union'ed with m_CsqContext.  m_CsqContext was in use up
    // until this function was called.  From this point on, we are going to
    // process the request as if it has been completed.  The completed code path
    // assumes m_ListEntry is on a list head.  To have a valid m_ListEntry when
    // we call RemoveEntryList, initialize it now.  Since we have an outstanding
    // irp completion reference count (which is decremented in the call to
    // FailPendedRequest later), we can safely initialize this field without
    // holding any locks.
    //
    InitializeListHead(&pRequest->m_ListEntry);

    //
    // Undo the affects of the IoSetNextIrpStackLocation made when we
    // enqueued the request.
    //
    pFxIrp.SetIrp(Irp);
    pFxIrp.SkipCurrentIrpStackLocation();

    //
    // Request is no longer pended
    //
    pThis->Lock(&irql);
    ASSERT(pRequest->GetTargetFlags() & FX_REQUEST_PENDED);
    pRequest->ClearTargetFlags(FX_REQUEST_PENDED);
    pThis->Unlock(irql);

    //
    // Call the driver's completion routine
    //
    pThis->FailPendedRequest(pRequest, STATUS_CANCELLED);
}

VOID
FxIoTarget::_SyncCompletionRoutine(
    __in WDFREQUEST Request,
    __in WDFIOTARGET Target,
    __in PWDF_REQUEST_COMPLETION_PARAMS Params,
    __in WDFCONTEXT Context
    )
{
    FxTargetSubmitSyncParams* pParams;

    pParams = (FxTargetSubmitSyncParams*) Context;
    pParams->Status = Params->IoStatus.Status;

    if (pParams->OrigTargetCompletionRoutine != NULL) {
        pParams->OrigTargetCompletionRoutine(
            Request,
            Target,
            Params,
            pParams->OrigTargetCompletionContext
            );
    }

    pParams->SynchEvent.Set();
}


VOID
FxIoTarget::CancelSentIo(
    VOID
    )
/*++

Routine Description:
    This will be used whenever we send a reset request.
    For example if you are sending a reset request
    to USB target, you must cancel outstanding I/O before sending a reset
    or cycle port request for error recovery.
--*/

{
    SINGLE_LIST_ENTRY sentRequestListHead;
    BOOLEAN sentAdded;
    KIRQL irql;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;


    pFxDriverGlobals = GetDriverGlobals();
    sentRequestListHead.Next = NULL;
    Lock(&irql);

    GetSentRequestsListLocked(&sentRequestListHead,
                              &m_SentIoListHead,
                              &sentAdded);

    Unlock(irql);

    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
        "Cancelling pending I/O on WDFIOTARGET %p ",
        GetHandle());

    if (sentAdded) {
        _CancelSentRequests(&sentRequestListHead);
    }
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::SubmitSyncRequestIgnoreTargetState(
    __in FxRequestBase* Request,
    __in_opt PWDF_REQUEST_SEND_OPTIONS RequestOptions
    )
/*++

Routine Description:
    Use this for sending a request which ignores target state.
--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDF_REQUEST_SEND_OPTIONS requestOptions;

    pFxDriverGlobals = GetDriverGlobals();
    if (RequestOptions != NULL) {

        //
        // Do a copy so that the passed in paramters is
        // not modified.
        //
        RtlCopyMemory(&requestOptions,
                      RequestOptions,
                      sizeof(WDF_REQUEST_SEND_OPTIONS));

        if ((requestOptions.Flags & WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE) == 0) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                                "Ignoring WDFIOTARGET %p state to send request",
                                GetHandle());
            requestOptions.Flags |= WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE;
        }
    }
    else{
        WDF_REQUEST_SEND_OPTIONS_INIT(&requestOptions,
                                      WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);
    }

    return SubmitSync(Request, &requestOptions);
}

VOID
FxIoTarget::UpdateTargetIoType(
    VOID
    )
{
    UCHAR ioType = GetTargetIoType();

    //
    // m_IoCount is initialized to 1
    //
    if ((ioType != m_TargetIoType) && (m_IoCount > 1)) {
        DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFIOTARGET %p has changed IoType with outstanding IO",
                GetHandle());
    }
    m_TargetIoType = (UCHAR) ioType;
}
