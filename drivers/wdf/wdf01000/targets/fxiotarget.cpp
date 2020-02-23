#include "common/fxiotarget.h"
#include "common/dbgtrace.h"
#include "common/fxdevice.h"


VOID
FxIoTarget::UpdateTargetIoType(
    VOID
    )
{
    UCHAR ioType = GetTargetIoType();

    //
    // m_IoCount is initialized to 1
    //
    if ((ioType != m_TargetIoType) && (m_IoCount > 1))
    {
        DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFIOTARGET %p has changed IoType with outstanding IO",
                GetHandle());
    }

    m_TargetIoType = (UCHAR) ioType;
}

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

_Must_inspect_result_
NTSTATUS
FxIoTarget::Init(
    __in CfxDeviceBase* Device
    )
{
    NTSTATUS status;

    status = InitModeSpecific(Device);
    if (!NT_SUCCESS(status))
    {
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
        m_TargetDevice == NULL || m_TargetPdo == NULL)
    {
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
    if (m_TargetDevice != NULL)
    {
        deviceObject.SetObject(m_TargetDevice);

        m_TargetStackSize = deviceObject.GetStackSize();

        m_TargetIoType = GetTargetIoType();
    }

    return STATUS_SUCCESS;
}

MdDeviceObject
FxIoTarget::GetTargetDeviceObject(
    _In_ CfxDeviceBase* Device
    )
{
    return Device->GetAttachedDevice();
}

FxIoTarget::~FxIoTarget()
{
    ASSERT(IsListEmpty(&m_SentIoListHead));
    ASSERT(IsListEmpty(&m_IgnoredIoListHead));
    ASSERT(m_IoCount == 0);
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
    if (Request->CancelTimer())
    {
        //
        // Sync with Request->Cancel() to make sure we can delete the request.
        //
        completeRequest = Request->CanComplete();
    }

    if (completeRequest)
    {
        //
        // RemoveCompletedRequestLocked clears Request->m_TargetFlags, so we must
        // do this check before that call.
        //
        if ((Request->GetTargetFlags() & FX_REQUEST_CANCELLED_FROM_TIMER) &&
            Request->m_Irp.GetStatus() == STATUS_CANCELLED)
        {
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
    else
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
            "WDFREQUEST %p deferring completion due to outstanding completion "
            "references", Request->GetTraceObjectHandle());
    }

    Unlock(irql);

    if (completeRequest)
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFREQUEST %p completed in completion routine",
                            Request->GetTraceObjectHandle());
        CompleteRequest(Request);
    }

    if (setStopEvent)
    {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFIOTARGET %p, setting stop event %p",
                            GetObjectHandle(), m_SentIoEvent.GetEvent());
        m_SentIoEvent.Set();
    }

    if (completeRequest)
    {
        DecrementIoCount();
    }
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
    if (m_Removing)
    {
        if (IsListEmpty(&m_SentIoListHead) && IsListEmpty(&m_IgnoredIoListHead))
        {
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
             IsListEmpty(&m_SentIoListHead))
    {
        m_WaitingForSentIo = FALSE;
        return TRUE;
    }

    return FALSE;
}
