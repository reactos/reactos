/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestBase.cpp

Abstract:

    This module implements FxRequestBase object

Author:

Environment:

    Both kernel and user mode

Revision History:



--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxRequestBase.tmh"
}

FxRequestBase::FxRequestBase(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USHORT ObjectSize,
    __in_opt MdIrp Irp,
    __in FxRequestIrpOwnership Ownership,
    __in FxRequestConstructorCaller Caller,
    __in FxObjectType ObjectType
    ) : FxNonPagedObject(FX_TYPE_REQUEST, ObjectSize, FxDriverGlobals, ObjectType),
        m_Irp(Irp)
{
    //
    // By default requests cannot be completed except for request associated with
    // IRP allocated from I/O (upper drivers).
    //
    m_CanComplete = FALSE;

    //
    // After is all said and done with assigning to m_IrpAllocation value, if
    // m_Irp().GetIrp  == NULL, then m_IrpAllocation can be overridden in
    // ValidateTarget.
    //
    if (Caller == FxRequestConstructorCallerIsDriver) {
        if (Ownership == FxRequestOwnsIrp) {
            //
            // Driver writer gave the irp to the framework but still owns it
            // or there is no irp passed in when the FxRequest was created.
            //
            m_IrpAllocation = REQUEST_ALLOCATED_INTERNAL;
        }
        else {
            //
            // Driver writer gave the irp to the framework but still owns it
            //
            m_IrpAllocation = REQUEST_ALLOCATED_DRIVER;
        }

        //
        // Cleanup request's context in Dispose. Please see Dispose below
        // for specific scenario we are trying to fix.  Enable Dispose only if
        // driver is v1.11 or above b/c it may be possible that older driver
        // may have used invalid/bad scenarios similar to this one:
        // - create target object.
        // - create one or more driver created requests parented to the target.
        // - send one or more of these requests to the target (lower stack).
        // - delete the target object while requests are pending.
        // Deleting a target while it has a pending request is a valid
        // operation, what is not valid is for these request to be also
        // parented to the target at the same time.
        // In this scenario if we turn on Dispose, the Dispose callback will
        // be called before the request is completed.
        // Note that if the driver had specified any Cleanup callbacks on
        // these requests, these also are called before the requests are
        // completed.
        //
        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1, 11)) {
            MarkDisposeOverride();
        }
    }
    else if (Ownership == FxRequestOwnsIrp) {
        //
        // The request will own the irp and free it when the request is freed
        //
        m_IrpAllocation = REQUEST_ALLOCATED_INTERNAL;
    }
    else {
        //
        // Request is owned by the io queue
        //
        m_IrpAllocation = REQUEST_ALLOCATED_FROM_IO;
        m_CanComplete = TRUE;
    }

    m_Target = NULL;
    m_TargetFlags = 0;

    m_TargetCompletionContext = NULL;

    m_Completed = m_Irp.GetIrp() ? FALSE : TRUE;
    m_Canceled = FALSE;

    m_PriorityBoost = 0;

    m_RequestContext = NULL;
    m_Timer = NULL;

    InitializeListHead(&m_ListEntry);
    m_DrainSingleEntry.Next = NULL;

    m_IrpReferenceCount = 0;

    m_IrpQueue = NULL;

    m_SystemBufferOffset = 0;
    m_OutputBufferOffset = 0;
    m_IrpCompletionReferenceCount = 0;

    m_AllocatedMdl = NULL;

    m_VerifierFlags = 0;
    m_RequestBaseFlags = 0;
    m_RequestBaseStaticFlags = 0x0;
    m_CompletionState = FxRequestCompletionStateNone;
}

FxRequestBase::~FxRequestBase(
    VOID
    )
{
    MdIrp irp;

    //
    // Since m_ListEntry is a union with the CSQ context and the irp have just
    // come off of a CSQ, we cannot be guaranteed that the m_ListEntry is
    // initialized to point to itself.
    //
    // ASSERT(IsListEmpty(&m_ListEntry));


#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    //
    // If an MDL is associated with the request, free it
    //
    if (m_AllocatedMdl != NULL) {
        FxMdlFree(GetDriverGlobals(), m_AllocatedMdl);
    }
#endif

    irp = m_Irp.GetIrp();

    //
    // If the request was created through WdfRequestCreate, formatted, and not
    // reused, we can still have a request context.
    //
    if (m_RequestContext != NULL) {
        if (irp != NULL) {
            m_RequestContext->ReleaseAndRestore(this);
        }

        delete m_RequestContext;
    }

    if (irp != NULL && m_IrpAllocation == REQUEST_ALLOCATED_INTERNAL) {
        m_Irp.FreeIrp();
    }

    if (m_Timer != NULL) {
        delete m_Timer;
    }
}

VOID
FX_VF_METHOD(FxRequestBase, VerifyDispose) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    SHORT flags;
    KIRQL irql;

    PAGED_CODE_LOCKED();

    Lock(&irql);
    flags = GetVerifierFlagsLocked();
    if (flags & FXREQUEST_FLAG_SENT_TO_TARGET) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Driver is trying to delete WDFREQUEST 0x%p while it is still "
            "active on WDFIOTARGET 0x%p. ",
            GetTraceObjectHandle(), GetTarget()->GetHandle());
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    Unlock(irql);
}

BOOLEAN
FxRequestBase::Dispose()
{
    //
    // Make sure request is not in use.
    //
    VerifyDispose(GetDriverGlobals());

    //
    // Now call Cleanup on any handle context's exposed
    // to the device driver.
    //
    CallCleanup();

    //
    // Call the request's cleanup (~dtor or Dispose).
    //
    if (m_RequestContext != NULL) {
        if (IsAllocatedFromIo() == FALSE && m_Irp.GetIrp() != NULL) {
            //
            // This code allows the following scenario to work correctly b/c
            // the original request can be completed without worrying that the
            // new request's context has references on the original request.
            //
            // * Driver receives an ioctl request.
            // * Driver creates a new request.
            // * Driver formats the new request with buffers from original ioctl.
            // * Driver sends the new request synchronously.
            // * Driver retrieves info/status from the new request.
            // * Driver deletes the new request.
            // * Driver completes the original request.
            //
            m_RequestContext->ReleaseAndRestore(this);

            //
            // ~dtor cleans up everything. No need to call its Dispose method.
            //
            delete m_RequestContext;
            m_RequestContext = NULL;
        }
        else {
            //
            // Let request's context know that Dispose is in progress.
            // RequestContext may receive the following two calls after Dispose:
            //      . ReleaseAndRestore if an IRP is still present.
            //      . Destructor
            //
            m_RequestContext->Dispose();
        }
    }

    return FALSE;
}

VOID
FxRequestBase::ClearFieldsForReuse(
    VOID
    )
{
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    if (m_AllocatedMdl != NULL) {
        FxMdlFree(GetDriverGlobals(), m_AllocatedMdl);
        m_AllocatedMdl = NULL;
    }
#endif

    m_RequestBaseFlags = 0;
    m_RequestBaseStaticFlags = 0x0;
    m_VerifierFlags = 0;
    m_Canceled = FALSE;

    SetCompleted(FALSE);
    SetPriorityBoost(0);

    m_NextStackLocationFormatted = FALSE;

    if (m_Timer != NULL) {
        delete m_Timer;
        m_Timer = NULL;
    }

    m_Target = NULL;
    m_TargetFlags = 0;
    m_TargetCompletionContext = NULL;

    InitializeListHead(&m_ListEntry);

    m_DrainSingleEntry.Next = NULL;
    m_IrpCompletionReferenceCount = 0;
    m_CompletionState = FxRequestCompletionStateNone;
}

_Must_inspect_result_
NTSTATUS
FxRequestBase::ValidateTarget(
    __in FxIoTarget* Target
    )
{
    MdIrp pIrp, pOldIrp;
    FxIrp fxIrp;
    NTSTATUS status;

    pOldIrp = NULL;

    pIrp = GetSubmitIrp();
    fxIrp.SetIrp(pIrp);

    //
    // Must restore to the previous irp in case we reallocate the irp
    //
    ContextReleaseAndRestore();

    if (Target->HasValidStackSize() == FALSE) {

        //
        // Target is closed down
        //
        status = STATUS_INVALID_DEVICE_STATE;

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFIOTARGET %p is closed, cannot validate, %!STATUS!",
                            Target->GetHandle(), status);
    }
    else if (pIrp != NULL && Target->HasEnoughStackLocations(&fxIrp)) {
        status = STATUS_SUCCESS;
    }
    else if (pIrp == NULL || m_IrpAllocation == REQUEST_ALLOCATED_INTERNAL) {
        //
        // Try to allocate a new irp.
        //
        pIrp = FxIrp::AllocateIrp(Target->m_TargetStackSize, Target->GetDevice());

        if (pIrp == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not allocate irp for WDFREQUEST %p for WDFIOTARGET %p,"
                " %!STATUS!", GetTraceObjectHandle(), Target->GetHandle(), status);
        }
        else {
            pOldIrp = SetSubmitIrp(pIrp, FALSE);
            m_IrpAllocation = REQUEST_ALLOCATED_INTERNAL;
            status = STATUS_SUCCESS;
        }
    }
    else {
        //
        // The internal IRP is not owned by this object, so we can't reallocate
        // it.
        //







        status = STATUS_REQUEST_NOT_ACCEPTED;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Cannot reallocate PIRP for WDFREQUEST %p using WDFIOTARGET %p,"
            " %!STATUS!", GetTraceObjectHandle(), Target->GetHandle(), status);
    }

    if (pOldIrp != NULL) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Freeing irp %p from WDFREQUEST %p\n",
                             pOldIrp, GetTraceObjectHandle());
        FxIrp oldIrp(pOldIrp);
        oldIrp.FreeIrp();
    }

    return status;
}

MdIrp
FxRequestBase::SetSubmitIrp(
    __in_opt MdIrp NewIrp,
    __in BOOLEAN FreeIrp
    )
{
    MdIrp pOldIrp, pIrpToFree;

    pIrpToFree = NULL;
    pOldIrp = m_Irp.SetIrp(NewIrp);

    if (NewIrp != NULL) {
        m_Completed = FALSE;
    }

    //
    // If there is a previous irp that is not the current value and we
    // allocated it ourselves and the caller wants us to free it, do so.
    //
    if (pOldIrp != NULL &&
        pOldIrp != NewIrp &&
        m_IrpAllocation == REQUEST_ALLOCATED_INTERNAL) {
        if (FreeIrp) {
            FxIrp oldIrp(pOldIrp);
            oldIrp.FreeIrp();
        }
        else {
            pIrpToFree = pOldIrp;
        }
    }

    return pIrpToFree;
}

VOID
FxRequestBase::CompleteSubmittedNoContext(
    VOID
    )
/*++

Routine Description:
    Invokes the completion routine and uses a completion params that is on the
    stack. This is in a separate function so that we only consume the stack
    space if we need to.

Arguments:
    None

Return Value:
    None

  --*/
{
    WDF_REQUEST_COMPLETION_PARAMS params;

    params.Type = WdfRequestTypeNoFormat;

    GetSubmitFxIrp()->CopyStatus(&params.IoStatus);

    RtlZeroMemory(&params.Parameters, sizeof(params.Parameters));


    //
    // Once we call the completion routine we can't touch any fields anymore
    // since the request may be resent down the stack.
    //
    ClearCompletionRoutine()(GetHandle(),
                             m_Target->GetHandle(),
                             &params,
                             ClearCompletionContext());
}

VOID
FxRequestBase::CompleteSubmitted(
    VOID
    )
/*++

Routine Description:
    Routine that handles the setting up of the request packet for being passed
    back to the completion routine.  This includes copying over parameters from
    the PIRP and any dereferences necessary.

Arguments:
    None.

Return Value:
    None.

  --*/
{
    FxIoTarget* pTarget;

    pTarget = m_Target;

    FX_TRACK_DRIVER(GetDriverGlobals());

    if (GetDriverGlobals()->FxVerifierOn) {
        //
        // Zero out any values previous driver may have set; when completing the irp
        // through FxRequest::CompleteInternal, we check to see what the lastest
        // package was (stored off in the DriverContext).  Since the request was
        // sent off to another devobj, don't assume any valid values in the
        // DriverContext anymore.
        //
        ZeroOutDriverContext();

        //
        // ClearFormatted also checks for WdfVefiefierOn, but that's OK
        //
        VerifierClearFormatted();
    }

    if (m_RequestContext != NULL) {
        //
        // Always do the copy because the driver can retrieve the parameters
        // later even if there is no completion routine set.
        //
        GetSubmitFxIrp()->CopyStatus(
                    &m_RequestContext->m_CompletionParams.IoStatus
                    );

        m_RequestContext->CopyParameters(this);

        //
        // Call the completion routine if present.  Once we call the completion
        // routine we can't touch any fields anymore since the request may be resent
        // down the stack.
        //
        if (m_CompletionRoutine.m_Completion != NULL) {
            ClearCompletionRoutine()(GetHandle(),
                                     pTarget->GetHandle(),
                                     &m_RequestContext->m_CompletionParams,
                                     ClearCompletionContext());
        }
    }
    else if (m_CompletionRoutine.m_Completion != NULL) {
        //
        // Only put a completion parameters struct on the stack if we have to.
        // By putting it into a separate function, we can control stack usage
        // in this way.
        //
        CompleteSubmittedNoContext();
    }

    //
    // Release the tag that was acquired when the request was submitted or
    // pended.
    //
    RELEASE(pTarget);
}

BOOLEAN
FxRequestBase::Cancel(
    VOID
    )
/*++

Routine Description:
    Attempts to cancel a previously submitted or pended request.

Arguments:
    None

Return Value:
    TRUE if the request was successfully cancelled, FALSE otherwise

  --*/
{
    BOOLEAN result;

    DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                        "Request %p", this);

    //
    // It is critical to set m_Canceled before we check the reference count.
    // We could be racing with FxIoTarget::SubmitLocked and if this call executes
    // before SubmitLocked, the completion reference count will still be zero.
    // SubmitLocked will check m_Canceled after setting the reference count to
    // one so that it decrement the count back.
    //
    m_Canceled = TRUE;

    //
    // If the ref count is zero, the irp has completed already.  This means we
    // cannot safely touch the underlying PIRP because we cannot guarantee it
    // will not be completed from underneath us.
    //
    if (FxInterlockedIncrementGTZero(&m_IrpCompletionReferenceCount) != 0) {
        //
        // Successfully incremented the ref count.  The PIRP will not be completed
        // until the count goes to zero.
        //
        // Cancelling the irp handles all 2 states:
        //
        // 1)  the request is pended in a target.  the target will attempt to
        //     complete the request immediately in the cancellation routine, but
        //     will not be able to because of the added count to the ref count
        //     done above.  The count will move back to zero below and
        //     CompletedCanceledRequest will complete the request
        //
        // 2)  The irp is in flight to the target WDM device.  In which case the
        //     target WDM device should complete the request immediatley.  If
        //     it does not, it becomes the same as the case where the target WDM
        //     device has already pended it and placed a cancellation routine
        //     on the request and the request will (a)synchronously complete
        //
        result = m_Irp.Cancel();

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Request %p, PIRP %p, cancel result %d",
                            this, m_Irp.GetIrp(), result);

        //
        // If the count goes to zero, the completion routine ran, but deferred
        // completion ownership to this thread since we had the outstanding
        // refeference.
        //
        if (InterlockedDecrement(&m_IrpCompletionReferenceCount) == 0) {

            //
            // Since completion ownership was claimed, m_Target will be valid
            // until m_Target->CompleteRequest executes because the target will
            // not delete while there is outstanding I/O.
            //
            ASSERT(m_Target != NULL);

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                "Request %p, PIRP %p, completed synchronously in cancel call, "
                "completing request on target %p", this, m_Irp.GetIrp(), m_Target);

            m_Target->CompleteCanceledRequest(this);
        }
    }
    else {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Could not cancel request %p, already completed", this);

        result = FALSE;
    }

    return result;
}

VOID
FxRequestBase::_TimerDPC(
    __in PKDPC Dpc,
    __in_opt PVOID Context,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
/*++

Routine Description:
    DPC for the request timer. It lets the FxIoTarget associated with the
    request handle the cancellation synchronization with the PIRP's
    completion routine.

Arguments:
    Dpc - The DPC itself (part of FxRequestExtension)

    Context - FxRequest*  that has timed out

    SystemArgument1 - Ignored

    SystemArgument2 - Ignored

Return Value:
    None.

  --*/
{
    FxRequest* pRequest;
    FxIoTarget* pTarget;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    //
    // No need to grab FxRequest::Lock b/c if there is a timer running, then the
    // request is guaranteed to be associated with a target.
    //
    pRequest = (FxRequest*) Context;
    pTarget = pRequest->m_Target;

    ASSERT(pTarget != NULL);
    pTarget->TimerCallback(pRequest);
}

_Must_inspect_result_
NTSTATUS
FxRequestBase::CreateTimer(
    VOID
    )
/*++

Routine Description:
    Late time initialization of timer related structures, we only init
    timer structures if we are going to use them.

Arguments:
    None

Assumes:
    m_Target->Lock() is being held by the caller

Return Value:
    None

  --*/

{
    FxRequestTimer* pTimer;
    PVOID pResult;
    NTSTATUS status;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    if (m_Timer != NULL) {
        return STATUS_SUCCESS;
    }





    pTimer = new (FxDriverGlobals) FxRequestTimer();

    if(pTimer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pTimer->Timer.Initialize(this, _TimerDPC, 0);
    if(!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Failed to initialize timer for request %p", this);
        delete pTimer;
        return status;
    }

    pResult = InterlockedCompareExchangePointer((PVOID*)&m_Timer, pTimer, NULL);

    if (pResult != NULL) {
        //
        // Another value was set before we could set it, free our timer now
        //
        delete pTimer;
    }

    return STATUS_SUCCESS;
}

VOID
FxRequestBase::StartTimer(
    __in LONGLONG Timeout
    )
/*++

Routine Description:
    Starts a timer for the request

Arguments:
    Timeout - How long the timeout should be

Assumes:
    m_Target->Lock() is being held by the caller.

Return Value:
    None

  --*/
{
    LARGE_INTEGER timeout;
    timeout.QuadPart = Timeout;

    m_TargetFlags |= FX_REQUEST_TIMER_SET;

    m_Timer->Timer.Start(timeout);

}

_Must_inspect_result_
BOOLEAN
FxRequestBase::CancelTimer(
    VOID
    )
/*++

Routine Description:
    Cancel a previously queued timer based on this request if one was set.

Arguments:
    None

Assumes:
    Caller is providing synchronization around the call of this function with
    regard to m_TargetFlags.

Return Value:
    TRUE if the timer was cancelled successfully or if there was no timer set,
    otherwise FALSE if the timer was not cancelled and has fired.

  --*/
{
    if (m_TargetFlags & FX_REQUEST_TIMER_SET) {
        //
        // If we can successfully cancel the timer, release the reference
        // taken in StartTimer and mark the timer as not queued.
        //

        if (m_Timer->Timer.Stop() == FALSE) {

            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                                "Request %p, did not cancel timer", this);

            //
            // Leave FX_REQUEST_TIMER_SET set.  The timer DPC will clear the it
            //

            return FALSE;
        }

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIO,
                            "Request %p, canceled timer successfully", this);

        m_TargetFlags &= ~FX_REQUEST_TIMER_SET;
    }

    return TRUE;
}

DECLSPEC_NORETURN
VOID
FxRequestBase::FatalError(
    __in NTSTATUS Status
    )
{
    WDF_QUEUE_FATAL_ERROR_DATA data;

    RtlZeroMemory(&data, sizeof(data));

    data.Queue = NULL;
    data.Request = (WDFREQUEST) GetTraceObjectHandle();
    data.Status = Status;

    FxVerifierBugCheck(GetDriverGlobals(),
                       WDF_QUEUE_FATAL_ERROR,
                       (ULONG_PTR) &data);
}

