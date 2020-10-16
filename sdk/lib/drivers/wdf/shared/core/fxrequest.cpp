/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequest.cpp

Abstract:

    This module implements FxRequest object

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

// Tracing support
extern "C" {
// #include "FxRequest.tmh"
}

FxRequest::FxRequest(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in MdIrp Irp,
    __in FxRequestIrpOwnership Ownership,
    __in FxRequestConstructorCaller Caller,
    __in USHORT ObjectSize
    ) :
    FxRequestBase(FxDriverGlobals,
                  ObjectSize,
                  Irp,
                  Ownership,
                  Caller)
{
    m_OutputBufferOffset = FIELD_OFFSET(FxRequest, m_OutputBufferOffset);
    m_SystemBufferOffset = FIELD_OFFSET(FxRequest, m_SystemBufferOffset);
    m_IoQueue  = NULL;

    m_PowerStopState = FxRequestPowerStopUnknown;

    InitializeListHead(&m_OwnerListEntry);
    InitializeListHead(&m_OwnerListEntry2);
    InitializeListHead(&m_ForwardProgressList);

    m_Presented = (Caller == FxRequestConstructorCallerIsDriver) ? TRUE : FALSE;
    m_Reserved = FALSE;
    m_ForwardProgressQueue = NULL;
    m_ForwardRequestToParent = FALSE;
    m_InternalContext = NULL;
}

#if DBG
FxRequest::~FxRequest(
    VOID
    )
{
    ASSERT(IsListEmpty(&m_OwnerListEntry));
    ASSERT(IsListEmpty(&m_OwnerListEntry2));
}
#endif // DBG

_Must_inspect_result_
NTSTATUS
FxRequest::_CreateForPackage(
    __in CfxDevice* Device,
    __in PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    __in MdIrp Irp,
    __deref_out FxRequest** Request
   )
/*++

Routine Description:

    Creates an FxRequest object and returns its pointer to the caller.

Arguments:

    Device - Pointer to FxDevice object request will be associated with

    RequestAttributes - Specifies the object's attributes for the request.

    Irp    - Pointer to Irp

    Request - Pointer to location to store the returned FxRequest pointer

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS  status;
    FxRequest* pRequest;

    *Request = NULL;

    //
    // Allocate the new FxRequest object in the per driver tracking pool
    //
    pRequest = new(Device, RequestAttributes) FxRequestFromLookaside(Device, Irp);

    if (pRequest == NULL) {
        DoTraceLevelMessage(
            Device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Memory allocation failed %!STATUS!",
            STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // For forward progress the IRP can be NULL.
    //
    if (Irp != NULL) {
        pRequest->AssignMemoryBuffers(Device->GetIoTypeForReadWriteBufferAccess());
    }

    //
    // Improve I/O perf by not parenting it to device. However, if verifier is
    // turned on, the request is parented to the device to help track reference
    // leaks.
    //
    if (Device->GetDriverGlobals()->FxRequestParentOptimizationOn) {
        status = pRequest->Commit(RequestAttributes,
                                  NULL,
                                  NULL,
                                  FALSE);
    }
    else {
        status = pRequest->Commit(RequestAttributes,
                                  NULL,
                                  Device,
                                  FALSE);
    }

    if (NT_SUCCESS(status)) {
        *Request = pRequest;
    }
    else {
        DoTraceLevelMessage(
            Device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Could not commit FxRequest %!STATUS!", status);
        pRequest->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxRequest::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in_opt PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    __in_opt MdIrp Irp,
    __in_opt FxIoTarget* Target,
    __in FxRequestIrpOwnership Ownership,
    __in FxRequestConstructorCaller Caller,
    __deref_out FxRequest** Request
    )
{
    WDFOBJECT hRequest;
    NTSTATUS status;
    FxRequest* pRequest;

    *Request = NULL;

    status = FxValidateObjectAttributes(FxDriverGlobals, RequestAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pRequest = new (FxDriverGlobals, RequestAttributes)
        FxRequest(FxDriverGlobals,
                  Irp,
                  Ownership,
                  Caller,
                  sizeof(FxRequest));

    if (pRequest != NULL) {
        if (Target != NULL) {
            status = pRequest->ValidateTarget(Target);
        }

        if (NT_SUCCESS(status)) {
            status = pRequest->Commit(RequestAttributes, &hRequest, NULL, TRUE);
        }

        if (NT_SUCCESS(status)) {
            *Request = pRequest;
        }
        else {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                                "Handle create failed %!STATUS!", status);

            if (Irp != NULL) {
                //
                // Clear the irp out of the request so that the destructor does
                // not free it.  Since we are returning failure, the caller does
                // not expect the PIRP passed in to be freed.
                //
                pRequest->SetSubmitIrp(NULL, FALSE);
            }

            pRequest->DeleteFromFailedCreate();
        }
    }
    else {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Irp %p Ownership %!FxRequestIrpOwnership! FxRequest %p, status %!STATUS!",
                        Irp, Ownership, *Request, status);

    return status;
}

NTSTATUS
FxRequest::SetInformation(
    __in ULONG_PTR Information
    )
/*++

Routine Description:

    Set the IRP's IoStatus.Information field.

    NOTE: If the caller calls Complete(status, information), as opposed
          to Complete(status), the value will get overwritten.

Arguments:

    Information - Information value to set

Returns:

    NTSTATUS

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    if (pFxDriverGlobals->FxVerifierIO) {
        NTSTATUS status;
        KIRQL irql;

        Lock(&irql);

        status = VerifyRequestIsNotCompleted(pFxDriverGlobals);
        if (NT_SUCCESS(status)) {
            m_Irp.SetInformation(Information);
        }

        Unlock(irql);

        return status;
    }
    else {
        m_Irp.SetInformation(Information);
        return STATUS_SUCCESS;
    }
}

ULONG_PTR
FxRequest::GetInformation(
    VOID
    )
/*++

Routine Description:

    Get the IRP's IoStatus.Information field.


Arguments:

    None

Returns:

    ULONG_PTR

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    // Verifier
    if (pFxDriverGlobals->FxVerifierIO) {
        ULONG_PTR info;
        KIRQL irql;
        NTSTATUS status;

        Lock(&irql);

        status = VerifyRequestIsNotCompleted(pFxDriverGlobals);
        if (!NT_SUCCESS(status)) {
            info = NULL;
        }
        else {
            info = m_Irp.GetInformation();
        }

        Unlock(irql);

        return info;
    }
    else {
        return m_Irp.GetInformation();
    }
}

KPROCESSOR_MODE
FxRequest::GetRequestorMode(
    VOID
    )
/*++

Routine Description:
    Get the Irp->RequestorMode value.

Arguments:
    None

Returns:
    KPROCESSOR_MODE

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    if (pFxDriverGlobals->FxVerifierIO) {
        KPROCESSOR_MODE mode;
        KIRQL irql;
        NTSTATUS status;

        Lock(&irql);

        status = VerifyRequestIsNotCompleted(pFxDriverGlobals);
        if (!NT_SUCCESS(status)) {
            mode = UserMode;
        } else {
            mode = m_Irp.GetRequestorMode();
        }

        Unlock(irql);

        return mode;
    }
    else {
        return m_Irp.GetRequestorMode();
    }
}


VOID
FX_VF_METHOD(FxRequest, VerifyCompleteInternal)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    ULONG length;
    KIRQL irql;
    BOOLEAN validateLength;

    PAGED_CODE_LOCKED();

    Lock(&irql);

    if (GetDriverGlobals()->FxVerifierIO ) {
        (VOID) VerifyRequestIsNotCompleted(GetDriverGlobals());
    } else {
        ASSERT(m_Completed == FALSE);
    }


    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_CANCELABLE) &&
        (m_VerifierFlags & FXREQUEST_FLAG_CANCELLED) == 0x0) {

        //
        // We could trace each sentence separate, but that takes up valuable
        // room in the IFR.  Instead, trace the entire "paragraph" as one
        // message so that we have more room in the IFR.
        //
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Completing Cancelable WDFREQUEST %p.  "

            "This results in a race condition in the device driver that can "
            "cause double completions.  "

            "Call WdfRequestUnmarkCancelable before WdfRequestComplete.  "

            "If WdfRequestUnmarkCancelable returns STATUS_CANCELLED, "
            "do not complete the request until the EvtIoCancel handler is called.  "

            "The straightforward way to ensure this is to complete a canceled "
            "request from the EvIoCancel callback.",

            GetHandle()
            );

            FxVerifierDbgBreakPoint(GetDriverGlobals());

    }

    validateLength = FALSE;
    length = 0;

    switch (m_Irp.GetMajorFunction()) {
    case IRP_MJ_READ:
        length = m_Irp.GetParameterReadLength();
        validateLength = TRUE;
        break;

    case IRP_MJ_WRITE:
        length = m_Irp.GetParameterWriteLength();
        validateLength = TRUE;
        break;

    case IRP_MJ_DEVICE_CONTROL:
        if (m_Irp.GetRequestorMode() == UserMode) {
            length = m_Irp.GetParameterIoctlOutputBufferLength();

            if (length > 0) {
                validateLength = TRUE;
            }
            else {
                //
                // For an output length == 0, a driver can indicate the number
                // of bytes used of the input buffer.
                //
                DO_NOTHING();
            }
        }
        else {
            //
            // If the IOCTL came from kernel mode, the same reasoning applies
            // here as for an internal IOCTL...we don't know deterministically
            // how to find the output buffer length.
            //
            DO_NOTHING();
        }
        break;

    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        //
        // Because the current stack location can use any part of the union
        // (like Parameters.Others instead of Parameters.DeviceIoControl), we
        // cannot deterministically figure out the output buffer length for
        // internal IOCTLs.
        //
        //    ||   || Fall through ||   ||
        //    \/   \/              \/   \/
    default:
        DO_NOTHING();
    }

    //
    // We shouldn't validate the information field if the status is warning
    // because it's valid for a driver to fill partial data in the buffer
    // and ask for large buffer size.
    //
    if (validateLength &&
            NT_SUCCESS(Status) &&
                m_Irp.GetInformation() > length) {

        WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH_DATA data;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p, MJ 0x%x, Information 0x%I64x is greater then "
            "buffer length 0x%x", GetHandle(), m_Irp.GetMajorFunction(),
            m_Irp.GetInformation(), length);

        data.Request = GetHandle();
        data.Irp = reinterpret_cast<PIRP>(m_Irp.GetIrp());
        data.OutputBufferLength = length;
        data.Information = m_Irp.GetInformation();
        data.MajorFunction = m_Irp.GetMajorFunction();

        FxVerifierBugCheck(
            GetDriverGlobals(),
            WDF_REQUEST_FATAL_ERROR,
            WDF_REQUEST_FATAL_ERROR_INFORMATION_LENGTH_MISMATCH,
            (ULONG_PTR) &data
            );

        // will not get here
    }

    // Set IRP to NULL when NT IRP is completed
    m_Completed = TRUE;

    Unlock(irql);
}

NTSTATUS
FxRequest::CompleteInternal(
    __in NTSTATUS Status
    )

/*++

Routine Description:

    Internal worker to complete the current request object.

    This is called with the FxRequest object lock held, and
    state validation as far as IRP completion already done.

    Callers must use Complete(Status), or Complete(Status,Information)

    It returns with the FxRequest object locked *released*

Arguments:

    Status - Status to complete the request with

Returns:

    NTSTATUS

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    MdIrp               pIrp;
    FxRequestCompletionState state;
    FxIoQueue*          queue;
    CfxDevice*           pRefedDevice;

    pFxDriverGlobals = GetDriverGlobals();
    queue = NULL;
    //
    // Store off the irp into a local variable
    //
    pIrp = m_Irp.GetIrp();

    //
    // Lock is no longer required, since it's only used
    // by verifier for already completed requests. This is a
    // serious driver error anyway
    //


    VerifyCompleteInternal(pFxDriverGlobals, Status);

    if (pFxDriverGlobals->FxVerifierOn == FALSE) {
        //
        // No lock needed in non-verifier case since this is only
        // used to detect double completions with verifier on
        //
        ASSERT(m_Completed == FALSE);
        m_Completed = TRUE;
    }






#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)







    ULONG flagsMasked = m_Irp.GetFlags() & IRP_INPUT_OPERATION;
    if (m_Irp.GetMajorFunction()== IRP_MJ_DEVICE_CONTROL &&
        m_Irp.GetParameterIoctlCodeBufferMethod() == METHOD_BUFFERED &&
        m_Irp.GetRequestorMode() == UserMode &&
        m_Irp.GetParameterIoctlOutputBufferLength()== 0 &&
        (flagsMasked != 0)) {

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Driver that handled WDFREQUEST 0x%p is requesting data to  "
            " be written back to the UserBuffer by returing a non zero value "
            " in the Irp 0x%p Information field even though the OutputBufferLength "
            " is zero", GetObjectHandle(), pIrp);

        //
        // We will assert only if the Information field is not zero to warn
        // the developer that it's a bad thing to do. However, we do avoid
        // corruption of UserBuffer on completion by clearing the flag
        // erroneously set by the I/O manager.
        //
        if (m_Irp.GetInformation() != 0L) {
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }

        //
        // Clear the flag to prevent the I/O manager from coping the
        // data back from the SystemBuffer to Irp->UserBuffer
        //
        m_Irp.SetFlags(m_Irp.GetFlags() & (~IRP_INPUT_OPERATION));
    }
#endif

    //
    // If the status code is one of the framework facility codes,
    // map it to a standard NTSTATUS
    //


    if ((Status & 0x0FFF0000) == (FACILITY_DRIVER_FRAMEWORK << 16)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Converting WDF NTSTATUS value 0x%x...", Status);

        switch (Status) {
        case STATUS_WDF_PAUSED:
        case STATUS_WDF_BUSY:
            Status = STATUS_DEVICE_BUSY;
            break;

        case STATUS_WDF_INTERNAL_ERROR:
            Status = STATUS_INTERNAL_ERROR;
            break;

        case STATUS_WDF_TOO_FRAGMENTED:
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;

        default:
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Unknown WDF NTSTATUS 0x%x", Status);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "... to %!STATUS!", Status);
    }

    if (IsAllocatedFromIo() == FALSE && IsCanComplete() == FALSE) {
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
    }

    //
    // It is invalid to complete any requests on an IRPQUEUE
    //
    ASSERTMSG("WDFREQUEST is part of a WDFQUEUE, it could be Cancellable\n",
              (m_IrpQueue == NULL));

    state = (FxRequestCompletionState) m_CompletionState;
    queue = m_IoQueue;

    //
    // Some I/O request cleanup. Do not do this for driver created requests.
    //
    if (IsAllocatedFromIo()) {
        //
        // Set the completion state to none
        //
        m_CompletionState = FxRequestCompletionStateNone;

        if (IsReserved() == FALSE) {
            //
            // Don't set the Queue to NULL for reserved requests.
            //
            m_IoQueue = NULL;
        }
    }

    //
    // IMPORTANT:  the context must free its references before the request is
    // completed.  Any of these references could be on a memory interface that is
    // an embedded interface in this object.  If this reference
    // is left outstanding when we check m_IrpReferenceCount below, we would
    // bugcheck (and the driver writer would be unaware of why things are going
    // wrong).
    //
    // Also, if the driver is freeing the referenced mmemory interface in its
    // cleanup routine, we don't want an oustanding reference against it.
    //
    if (m_RequestContext != NULL) {
        //
        // m_RequestContext will be freed when the FxRequest's desctructor runs
        //
        m_RequestContext->ReleaseAndRestore(this);
    }

    //
    // If the request is not presented to the driver then clear the
    // cleanup & destroy callbacks before calling PerformEarlyDispose.
    //
    if (m_Presented == FALSE) {
        ClearEvtCallbacks();
    }

    if (IsReserved() == FALSE && IsAllocatedFromIo()) {
        //
        // Fire the driver supplied Cleanup callback if set
        //
        // This will allow the driver to release any IRP specific resources such
        // as MDLs before we complete the IRP back to the OS, and release the
        // process/thread reference.
        //
        // This is also the callback used to tell the driver the WDM IRP is going
        // away if it has used the WDM "escape" API's to either get the IRP, or
        // any resources it references.
        //

        //
        // If a cleanup callback has been registered, we call it
        // just before completing the IRP to WDM, which can cause any
        // associated buffers, MDLs, or memory interfaces to be invalidated.
        //
        if (EarlyDispose() == FALSE) {
            VerifierBreakpoint_RequestEarlyDisposeDeferred(GetDriverGlobals());
        }

        //
        // Now that the entire tree is disposed, we want to destroy all of the
        // children.  This will not put this object in the destroyed state.  For
        // m_IrpReferenceCount to go to zero, we need to destroy the child WDFMEMORYs
        // that were created when we probed and locked the buffers.
        //
        DestroyChildren();
    }
    else {
        //
        // We don't call cleanup callback for Reserved Requests.
        // The driver can perform any cleanp it wants before completing the Request
        // or before reusing the Reserved Request in its Dispatch callback.






        DO_NOTHING();
    }

    //
    // If this is non-zero, indicates a reference count problem on any
    // WDFMEMORY objects returned to the device driver from this WDFREQUEST.
    //
    if (m_IrpReferenceCount != 0) {
        //
        // NOTE: you cannot call GetBuffer or GetMdl
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_FATAL, TRACINGREQUEST,
            "WDFREQUEST 0x%p, PIRP 0x%p, Major Function 0x%x,  completed with "
            "outstanding references on WDFMEMORY 0x%p or 0x%p retrieved from "
            "this request",
            GetObjectHandle(), m_Irp.GetIrp(), m_Irp.GetMajorFunction(),
            ((m_RequestBaseFlags & FxRequestBaseSystemMdlMapped) ||
             (m_RequestBaseStaticFlags & FxRequestBaseStaticSystemBufferValid)) ?
                m_SystemBuffer.GetHandle() : NULL,
            ((m_RequestBaseFlags & FxRequestBaseOutputMdlMapped) ||
             (m_RequestBaseStaticFlags & FxRequestBaseStaticOutputBufferValid)) ?
                m_OutputBuffer.GetHandle() : NULL
            );

        if ((m_RequestBaseFlags & FxRequestBaseSystemMdlMapped) ||
            (m_RequestBaseStaticFlags & FxRequestBaseStaticSystemBufferValid)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_FATAL, TRACINGREQUEST,
                "WDFMEMORY 0x%p, buffer %p, PMDL %p, length %I64d bytes",
                m_SystemBuffer.GetHandle(), m_SystemBuffer.GetBuffer(),
                m_SystemBuffer.GetMdl(), m_SystemBuffer.GetBufferSize());
        }

        if ((m_RequestBaseFlags & FxRequestBaseOutputMdlMapped) ||
            (m_RequestBaseStaticFlags & FxRequestBaseStaticOutputBufferValid)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_FATAL, TRACINGREQUEST,
                "IOCTL output WDFMEMORY 0x%p, buffer %p, PMDL %p, length %I64d bytes",
                m_OutputBuffer.GetHandle(), m_OutputBuffer.GetBuffer(),
                m_OutputBuffer.GetMdl(), m_OutputBuffer.GetBufferSize());
        }

        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_VERIFIER_FATAL_ERROR,
                           (ULONG_PTR) GetObjectHandle(),
                           (ULONG_PTR) m_IrpReferenceCount);
    }

    FxIrp irp(pIrp);

    //
    // Complete the NT IRP through the frameworks FxIrp
    //
    irp.SetStatus(Status);

    ASSERT(IsCancelRoutineSet() == FALSE);

    //
    // For driver created requests we need to use two phase
    // completion (pre and post) to detach the request from the queue before the
    // IRP is completed, and then allow a new request to be dispatched.
    // Note that the IRP is actually completed when the reference on this object
    // goes to 1 (See FxRequest::Release() for more info).
    //
    if (IsAllocatedFromIo() == FALSE) {
        //
        // Do not touch the request after this call.
        // Do not clear the request's IRP field before making this call.
        //
        PreProcessCompletionForDriverRequest(state, queue);
    }
    else {
        //
        // We only clear the irp from this object after PerformEarlyDispose has been
        // called.  m_SystemBuffer and m_OutputBuffer use m_Irp to return their
        // buffers and their WDFMEMORY handles should be valid in the cleanup routine
        // for the WDFREQUEST.  We also keep m_Irp valid until after the
        // m_IrpReferenceCount check, so we can trace out the buffers in case of
        // error.
        //
        m_Irp.SetIrp(NULL);

        if (irp.GetMajorFunction() == IRP_MJ_CREATE) {
            //
            // If this is the last handle to be closed on the device, then the call
            // to CreateCompleted can cause the device to be deleted if the create
            // has failed.  We add a reference so that we make sure we have a device
            // to free the request memory back to.
            //
            pRefedDevice = GetDevice();
            pRefedDevice->ADDREF(&irp);

            pRefedDevice->m_PkgGeneral->CreateCompleted(&irp);
        }
        else {
            pRefedDevice = NULL;
        }

        //
        // WDM IRP is completed.
        //
        irp.CompleteRequest(GetPriorityBoost());

        if (IsReserved() == FALSE) {
            PostProcessCompletion(state, queue);
        }
        else {
            PostProcessCompletionForReserved(state, queue);
        }

        if (pRefedDevice != NULL) {
            pRefedDevice->RELEASE(&irp);
            pRefedDevice = NULL;
        }
    }

    return Status;
}


VOID
FxRequest::PostProcessCompletion(
    __in FxRequestCompletionState State,
    __in FxIoQueue* Queue
    )
{
    //
    // Fire frameworks internal callback event if one is set
    // (FxIoQueue or IoTarget's internal use)
    //
    if (State != FxRequestCompletionStateNone) {
        //
        // NOTE: This occurs after the IRP has already been released,
        //       and is only used for notification that the request
        //       has completed.
        //
        if (State & FxRequestCompletionStateIoPkgFlag) {
            GetDevice()->m_PkgIo->RequestCompletedCallback(this);
        }
        else {
            ASSERT(Queue != NULL);
            Queue->RequestCompletedCallback(this);
            //FxIoQueueToMx::RequestCompletedCallback(Queue, this);
        }

        //
        // DeleteObject will dereference the object reference taken when the callback
        // was registered
        //
        DeleteEarlyDisposedObject();
    }
    else {
        //
        // Caller still wants the FxRequest class to be valid on return,
        // but must call DeleteObject in order to ensure the object is
        // no longer assigned any child objects, etc.
        //
        ADDREF(FXREQUEST_COMPLETE_TAG);
        DeleteObject();
    }
}

VOID
FxRequest::PostProcessCompletionForReserved(
    __in FxRequestCompletionState State,
    __in FxIoQueue* Queue
    )
{
    //
    // Fire frameworks internal callback event if one is set
    // (FxIoQueue or IoTarget's internal use)
    //
    if (State != FxRequestCompletionStateNone) {
        //
        // NOTE: This occurs after the IRP has already been released,
        //       and is only used for notification that the request
        //       has completed.
        //
        if (State & FxRequestCompletionStateIoPkgFlag) {
            GetDevice()->m_PkgIo->RequestCompletedCallback(this);
        }
        else {
            ASSERT(m_IoQueue == Queue);
            Queue->RequestCompletedCallback(this);
        }
    }
    else {
        //
        // Caller still wants the FxRequest class to be valid on return,
        // but must call DeleteObject in order to ensure the object is
        // no longer assigned any child objects, etc.
        //
        ADDREF(FXREQUEST_COMPLETE_TAG);
    }

    RELEASE(FXREQUEST_FWDPRG_TAG);
}

//
// Handles pre-process completion for driver-created-requests queued by the driver.
//
VOID
FxRequest::PreProcessCompletionForDriverRequest(
    __in FxRequestCompletionState State,
    __in FxIoQueue* Queue
    )
{
    ASSERT(State == FxRequestCompletionStateNone ||
           State == FxRequestCompletionStateQueue);
    //
    // Fire frameworks internal callback (pre) event if one is set.
    //
    if (FxRequestCompletionStateQueue == State) {
        //
        // NOTE: This occurs before the IRP has already been released,
        //       and is only used to notify the queue to remove this request from this queue's
        //       internal lists. A second notification (lPostProcessCompletionForAllocatedDriver)
        //       is made after the IRP is completed.
        //
        Queue->PreRequestCompletedCallback(this);
    }
    else if (Queue != NULL){
        //
        // On return from PostProcessCompletionForDriverRequest, caller (framework)
        // will try to release the last ref. Increase the ref count so request stays alive until
        // driver invokes WdfObjectDelete on this request.
        //
        ADDREF(FXREQUEST_COMPLETE_TAG);
    }

    //
    // Let the system know that it is OK to complete this request.
    //
    RELEASE(FXREQUEST_DCRC_TAG);
}

//
// Handles post-process completion for driver-created-requests queued by the driver.
// On return the driver can delete the request with WdfObjectDelete.
// NOTE: request may be already gone/reused. Do not dereference this or access any of its
// members... its pointer is only used for logging.
//
VOID
FxRequest::PostProcessCompletionForDriverRequest(
    __in FxRequestCompletionState State,
    __in FxIoQueue* Queue
    )
{
    //
    // NOTE: Do not touch the request object here. The request object may already be
    //           re-used or deleted.
    //

    ASSERT(State == FxRequestCompletionStateNone ||
           State == FxRequestCompletionStateQueue);
    //
    // Fire frameworks internal callback (post) event if one is set.
    //
    if (FxRequestCompletionStateQueue == State) {
        //
        // NOTE: This occurs after the IRP has already been released,  and is only used
        //       to notify the queue to update its internal state and if appropriate, send
        //       another request.
        //
        Queue->PostRequestCompletedCallback(this);
    }
}

VOID
FxRequest::FreeRequest(
    VOID
    )
/*++

Routine Description:

    This routine is called to free a reserved request or in case of Fxpkgio
    a non-reserved request.

--*/
{
    //
    // Restore any fields if necessary
    //
    if (m_RequestContext != NULL) {
        //
        // m_RequestContext will be freed when the FxRequest's destructor runs
        //
        m_RequestContext->ReleaseAndRestore(this);
    }

    //
    // If the request is not presented to the driver then clear the
    // cleanup & destroy callbacks before calling PerformEarlyDispose.
    //
    if (m_Presented == FALSE) {
        ClearEvtCallbacks();
    }

    DeleteObject();
}

VOID
FX_VF_METHOD(FxRequest, VerifyPreProcessSendAndForget) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    PAGED_CODE_LOCKED();

    if (m_CompletionRoutine.m_Completion != NULL) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p cannot send and forget will not execute completion "
            "routine %p",
            GetHandle(), m_CompletionRoutine.m_Completion);

        FxVerifierDbgBreakPoint(FxDriverGlobals);








    }

    //
    // You cannot fire and forget a create irp if we created a WDFFILEOBJECT
    // for it since you must post process the status of the create because
    // the create can fail in the driver to which we are sending the irp.
    //
    if ((m_Irp.GetMajorFunction() == IRP_MJ_CREATE)
        &&
        (FxFileObjectClassNormalize(GetDevice()->GetFileObjectClass()) !=
            WdfFileObjectNotRequired)) {

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p cannot send and forget a create request which "
            "has a WDFFILEOBJECT created for it, it must have a completion "
            "routine and be post processsed", GetHandle());

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
}

VOID
FxRequest::PreProcessSendAndForget(
    VOID
    )
{
    VerifyPreProcessSendAndForget(GetDriverGlobals());

    //
    // To be sent and forgotten, the irp must have been presented
    //
    ASSERT(m_Presented);

    //
    // To be sent and forgotten, the irp must not have a formatted IO context.
    //
    ASSERT(HasContext() == FALSE);

    //
    // If the driver writer setup the next stack location using the current
    // stack location or did a manual IO_STACK_LOCATION copy, we do not want to
    // skip the current stack location that their format is used by the target
    // driver.
    //
    if (m_NextStackLocationFormatted == FALSE) {
        m_Irp.SkipCurrentIrpStackLocation();
    }

    if (IsReserved() == FALSE) {

        //
        // If a cleanup callback has been registered, we call it
        // just before sending the IRP on its way.  The contract is that the cleanup
        // routine for a WDFREQUEST is called while the PIRP is still valid.
        //
        if (EarlyDispose() == FALSE) {
            VerifierBreakpoint_RequestEarlyDisposeDeferred(GetDriverGlobals());
        }

        //
        // Now that the entire tree is disposed, we want to destroy all of the
        // children.  This will not put this object in the destroyed state.  For
        // m_IrpReferenceCount to go to zero, we need to destroy the child WDFMEMORYs
        // that were created when we probed and locked the buffers.
        //
        DestroyChildren();
    }
}

VOID
FxRequest::PostProcessSendAndForget(
    VOID
    )
{
    FxRequestCompletionState state;
    FxIoQueue* pQueue;

    m_Irp.SetIrp(NULL);

    //
    // Capture the m_IoQueue value before making any other calls.
    // Note: m_IoQueue could be NULL if the request is freed before it's queued.
    //
    pQueue = m_IoQueue;

    ASSERT(m_CompletionState != FxRequestCompletionStateNone);

    state = (FxRequestCompletionState) m_CompletionState;
    m_CompletionState = FxRequestCompletionStateNone;

    //
    // Technically we did not complete the irp, but a send and forget is
    // functionally the same.  We  no longer own the irp.
    //
    if (IsReserved() == FALSE) {
        PostProcessCompletion(state, pQueue);
        }
        else {
            //
            // Release checks m_Completed flag to decide whether to return
            // the request to reserved pool.
            //
            m_Completed = TRUE;
        PostProcessCompletionForReserved(state, pQueue);
    }
}

NTSTATUS
FxRequest::GetStatus(
    VOID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

    if (pFxDriverGlobals->FxVerifierIO) {
        KIRQL irql;

        Lock(&irql);








        status = m_Irp.GetStatus();

        Unlock(irql);

        return status;
    }
    else {
        return m_Irp.GetStatus();
    }
}

_Must_inspect_result_
NTSTATUS
FxRequest::GetParameters(
    __out PWDF_REQUEST_PARAMETERS Parameters
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    //
    // No lock needed. Only reason this may be invalid is due
    // to previous completion, which is a serious driver bug
    // that will result in a crash anyway.
    //

    if (pFxDriverGlobals->FxVerifierIO) {
        KIRQL irql;
        NTSTATUS status;

        Lock(&irql);

        status = VerifyRequestIsCurrentStackValid(pFxDriverGlobals);
        if (NT_SUCCESS(status)) {
            status = VerifyRequestIsNotCompleted(pFxDriverGlobals);
        }

        Unlock(irql);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    ASSERT(Parameters->Size >= sizeof(WDF_REQUEST_PARAMETERS));

    // How much we copied
    Parameters->Size = sizeof(WDF_REQUEST_PARAMETERS);


    // Copy parameters
    Parameters->Type = (WDF_REQUEST_TYPE)m_Irp.GetMajorFunction();
    Parameters->MinorFunction = m_Irp.GetMinorFunction();

    // Copy the Parameters structure which we are a subset of
    m_Irp.CopyParameters(Parameters);

    if (pFxDriverGlobals->FxVerifierIO) {
        //
        // If verifier is on, and the operation is an IRP_MJ_DEVICE_CONTROL
        // with METHOD_NEITHER, then set Type3InputBuffer to zero since
        // this should not be used to pass parameters in the normal path
        //
        if((m_Irp.GetMajorFunction() == IRP_MJ_DEVICE_CONTROL) &&
            m_Irp.GetParameterIoctlCodeBufferMethod() == METHOD_NEITHER) {
            Parameters->Parameters.DeviceIoControl.Type3InputBuffer = NULL;
        }
    }

    return STATUS_SUCCESS;
}


_Must_inspect_result_
NTSTATUS
FxRequest::GetMemoryObject(
    __deref_out IFxMemory** MemoryObject,
    __out PVOID* Buffer,
    __out size_t* Length
    )
{
    PMDL  pMdl;
    NTSTATUS status;
    ULONG length;
    KIRQL irql;
    BOOLEAN mapMdl;
    UCHAR majorFunction;

    status = STATUS_SUCCESS;
    length = 0x0;
    mapMdl = FALSE;
    irql = PASSIVE_LEVEL;
    majorFunction = m_Irp.GetMajorFunction();


    //
    // Verifier
    //
    if (GetDriverGlobals()->FxVerifierIO) {
        status = VerifyRequestIsNotCompleted(GetDriverGlobals());
        if (!NT_SUCCESS(status)) {
            goto Done;
        }
        if (m_Irp.GetRequestorMode() == UserMode
            &&
            (majorFunction == IRP_MJ_WRITE ||
             majorFunction == IRP_MJ_READ)
            &&
            GetDevice()->GetIoType() == WdfDeviceIoNeither) {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Attempt to get UserMode Buffer Pointer for WDFDEVICE 0x%p, "
                "WDFREQUEST 0x%p, %!STATUS!",
                GetDevice()->GetHandle(), GetHandle(), status);

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver must use buffered or direct I/O for this call, or use "
                "WdfDeviceInitSetIoInCallerContextCallback to probe and lock "
                "user mode memory");

            FxVerifierDbgBreakPoint(GetDriverGlobals());
        }
    }

    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticSystemBufferValid) == 0x00) {
        Lock(&irql);
    }

    //
    // We must dig into the IRP to get the buffer, length, and readonly status
    //

    switch (majorFunction) {
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        length = m_Irp.GetParameterIoctlInputBufferLength();

        if (length == 0) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST %p InputBufferLength length is zero, %!STATUS!",
                GetObjectHandle(), status);

            goto Done;
        }

        if (m_Irp.GetParameterIoctlCodeBufferMethod() == METHOD_NEITHER) {
            //
            // Internal device controls are kernel mode to kernel mode, and deal
            // with direct unmapped pointers.
            //
            // In addition, a normal device control with
            // RequestorMode == KernelMode is also treated as kernel mode
            // to kernel mode since the I/O Manager will not generate requests
            // with this setting from a user mode request.
            //
            if ((m_Irp.GetRequestorMode() == KernelMode) ||
                (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)) {
                DO_NOTHING();
            }
            else {
                status = STATUS_INVALID_DEVICE_REQUEST;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "Attempt to get UserMode Buffer Pointer for METHOD_NEITHER "
                    "DeviceControl 0x%x, WDFDEVICE 0x%p, WDFREQUEST 0x%p, "
                    "%!STATUS!",
                    m_Irp.GetParameterIoctlCode(),
                    GetDevice()->GetHandle(), GetHandle(), status);

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "Driver must use METHOD_BUFFERED or METHOD_xx_DIRECT I/O for "
                    "this call, or use WdfDeviceInitSetIoInCallerContextCallback to "
                    "probe and lock user mode memory %!STATUS!",
                    STATUS_INVALID_DEVICE_REQUEST);

                goto Done;
            }
        }
        break;

    case IRP_MJ_READ:
        length = m_Irp.GetParameterReadLength();

        if (GetDevice()->GetIoTypeForReadWriteBufferAccess() == WdfDeviceIoDirect) {
            KMDF_ONLY_CODE_PATH_ASSERT();
            mapMdl = TRUE;
        }
        break;

    case IRP_MJ_WRITE:
        length = m_Irp.GetParameterWriteLength();

        if (GetDevice()->GetIoTypeForReadWriteBufferAccess() == WdfDeviceIoDirect) {
            KMDF_ONLY_CODE_PATH_ASSERT();
            mapMdl = TRUE;
        }
        break;

    default:
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Unrecognized Major Function 0x%x on WDFDEVICE 0x%p WDFREQUEST 0x%p",
            majorFunction, GetDevice()->GetHandle(), GetHandle());

        FxVerifierDbgBreakPoint(GetDriverGlobals());

        status = STATUS_INVALID_DEVICE_REQUEST;
        goto Done;
    }

    if (length == 0) {
        status = STATUS_BUFFER_TOO_SMALL;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p length is zero, %!STATUS!",
            GetHandle(), status);

        goto Done;
    }

    //
    // See if we need to map
    //
    if (mapMdl && (m_RequestBaseFlags & FxRequestBaseSystemMdlMapped) == 0x00) {
        pMdl = m_Irp.GetMdl();

        if (pMdl == NULL) {
            //
            // Zero length transfer
            //
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, direct io device, PMDL is NULL, "
                "%!STATUS!", GetHandle(), status);

            ASSERT(length == 0);
        }
        else {
            PVOID pVA;

            //
            // PagePriority may need to be a property, and/or parameter to
            // this call
            //
            //
            // Upon success, MmGetSystemAddressForMdlSafe stores the mapped
            // VA pointer in the pMdl and upon subsequent calls to
            // MmGetSystemAddressForMdlSafe, no mapping is done, just
            // the stored VA is returned.  FxRequestSystemBuffer relies
            // on this behavior and, more importantly, relies on this function
            // to do the initial mapping so that FxRequestSystemBuffer::GetBuffer()
            // will not return a NULL pointer.
            //
            pVA = Mx::MxGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);

            if (pVA == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST 0x%p could not get a system address for PMDL "
                    "0x%p, %!STATUS!", GetHandle(), pMdl, status);
            }
            else {
                //
                // System will automatically release the mapping PTE's when
                // the MDL is released by the I/O request
                //





                m_SystemBuffer.SetMdl(m_Irp.GetMdl());

                m_RequestBaseFlags |= FxRequestBaseSystemMdlMapped;
            }
        }
    }

Done:
    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticSystemBufferValid) == 0x00) {
        Unlock(irql);
    }

    if (NT_SUCCESS(status)) {
        *MemoryObject = &m_SystemBuffer;

        if (mapMdl) {
            *Buffer = Mx::MxGetSystemAddressForMdlSafe(m_SystemBuffer.m_Mdl,
                                                   NormalPagePriority);
        }
        else {
            *Buffer = m_SystemBuffer.m_Buffer;
        }

        *Length = length;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxRequest::GetDeviceControlOutputMemoryObject(
    __deref_out IFxMemory** MemoryObject,
    __out PVOID* Buffer,
    __out size_t* Length
    )
/*++

Routine Description:

    Return the IRP_MJ_DEVICE_CONTROL OutputBuffer.

    The memory buffer is valid in any thread/process context,
    and may be accessed at IRQL > PASSIVE_LEVEL.

    The memory buffer is automatically released when the request
    is completed.

    The memory buffer is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or for any request other than IRP_MJ_DEVICE_CONTROL.

    The Memory buffer is as follows for each buffering mode:

    METHOD_BUFFERED:

        Irp->UserBuffer  // This is actually a system address

    METHOD_IN_DIRECT:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    METHOD_OUT_DIRECT:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    METHOD_NEITHER:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The buffer is only valid until the request is completed.

Arguments:

    MemoryObject - Pointer location to return the memory object interface.

    Buffer - Pointer location to return buffer ptr

    Length - Pointer location to return buffer length.

Returns:

    NTSTATUS

--*/
{
    size_t length;
    NTSTATUS status;
    KIRQL irql;
    BOOLEAN mapMdl;
    UCHAR majorFunction;

    UNREFERENCED_PARAMETER(Buffer);
    UNREFERENCED_PARAMETER(Length);

    status = STATUS_SUCCESS;
    length = 0;
    irql = PASSIVE_LEVEL;
    mapMdl = FALSE;

    //
    // Verifier
    //
    if (GetDriverGlobals()->FxVerifierIO ) {
        status = VerifyRequestIsNotCompleted(GetDriverGlobals());
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticOutputBufferValid) == 0x00) {
        Lock(&irql);
    }

    //
    // See if we already have a validated buffer
    //
    //if (m_RequestBaseFlags & FxRequestBaseOutputBufferValid) {
    //    status = STATUS_SUCCESS;
    //}

    majorFunction = m_Irp.GetMajorFunction();

    ASSERT(majorFunction == IRP_MJ_DEVICE_CONTROL ||
           majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL);

    length = m_Irp.GetParameterIoctlOutputBufferLength();

    if (length == 0) {
        status = STATUS_BUFFER_TOO_SMALL;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p IOCTL output buffer length is zero, %!STATUS!",
            GetHandle(), status);

        goto Done;
    }

    switch (m_Irp.GetParameterIoctlCodeBufferMethod()) {
    //
    // InputBuffer is in SystemBuffer
    // OutputBuffer is in MdlAddress with read access
    //
    case METHOD_IN_DIRECT:
        //  ||  ||   fall     ||  ||
        //  \/  \/   through  \/  \/

    //
    // InputBuffer is in SystemBuffer
    // OutputBuffer is in MdlAddress with read access
    //
    case METHOD_OUT_DIRECT:
        mapMdl = TRUE;
        break;

    case METHOD_NEITHER:
        //
        // Internal device controls are kernel mode to kernel mode, and deal
        // with direct unmapped pointers.
        //
        // In addition, a normal device control with
        // RequestorMode == KernelMode is also treated as kernel mode
        // to kernel mode since the I/O Manager will not generate requests
        // with this setting from a user mode request.
        //
        if ((m_Irp.GetRequestorMode() == KernelMode) ||
            (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL)) {
            DO_NOTHING();
        }
        else {
            status = STATUS_INVALID_DEVICE_REQUEST;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Attempt to get UserMode Buffer Pointer for "
                "METHOD_NEITHER DeviceControl 0x%x, WDFDEVICE 0x%p, "
                "WDFREQUEST 0x%p, %!STATUS!",
                m_Irp.GetParameterIoctlCode(),
                GetDevice()->GetHandle(), GetObjectHandle(), status);

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver must use METHOD_BUFFERED or METHOD_xx_DIRECT "
                "I/O for this call, or use "
                "WdfDeviceInitSetIoInCallerContextCallback to probe and "
                "lock user mode memory");
        }
        break;
    }

    if (mapMdl && (m_RequestBaseFlags & FxRequestBaseOutputMdlMapped) == 0x0) {
        PMDL pMdl;
        PVOID pVA;


        pMdl = m_Irp.GetMdl();

        if (pMdl == NULL) {
            //
            // Zero length transfer
            //
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p, METHOD_IN_DIRECT IOCTL PMDL is NULL, "
                "%!STATUS!", GetHandle(), status);

            ASSERT(
                m_Irp.GetParameterIoctlOutputBufferLength()== 0
                );
        }
        else {
            //
            // PagePriority may need to be a property, and/or parameter to
            // this call
            //
            //
            // Upon success, MmGetSystemAddressForMdlSafe stores the mapped
            // VA pointer in the pMdl and upon subsequent calls to
            // MmGetSystemAddressForMdlSafe, no mapping is done, just
            // the stored VA is returned.  FxRequestOutputBuffer relies
            // on this behavior and, more importantly, relies on this function
            // to do the initial mapping so that FxRequestOutputBuffer::GetBuffer()
            // will not return a NULL pointer.
            //
            pVA = Mx::MxGetSystemAddressForMdlSafe(pMdl, NormalPagePriority);

            if (pVA == NULL) {
                status =  STATUS_INSUFFICIENT_RESOURCES;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST 0x%p could not get a system address for PMDL"
                    "0x%p, %!STATUS!", GetHandle(), pMdl, status);
            }
            else {
                m_OutputBuffer.SetMdl(pMdl);
                m_RequestBaseFlags |= FxRequestBaseOutputMdlMapped;
                status = STATUS_SUCCESS;
            }
        }
    }

Done:
    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticOutputBufferValid) == 0x00) {
        Unlock(irql);
    }

    if (NT_SUCCESS(status)) {
        *MemoryObject = &m_OutputBuffer;
        if (mapMdl) {
            *Buffer = Mx::MxGetSystemAddressForMdlSafe(m_OutputBuffer.m_Mdl,
                                                   NormalPagePriority);
        }
        else {
            *Buffer = m_OutputBuffer.m_Buffer;
        }
        *Length = length;
    }

    return status;
}

FxRequestCompletionState
FxRequest::SetCompletionState(
    __in FxRequestCompletionState NewState
    )
{
    FxRequestCompletionState oldState;

    //
    // The flag by itself should never be specified
    //
    ASSERT(NewState != FxRequestCompletionStateIoPkgFlag);

    //
    // No need to lock. Only the "owner" can set the completion
    // callback function, and this is when under the general
    // FxIoQueue lock.
    //

    // Request is already completed, awaiting final dereference
    if (m_Completed) {
        oldState = FxRequestCompletionStateNone;
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST 0x%p has already been completed",
                            GetHandle());
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }
    else {
        ASSERT(m_Irp.GetIrp() != NULL);

        oldState = (FxRequestCompletionState) m_CompletionState;

        m_CompletionState = (BYTE) NewState;

        if (NewState == FxRequestCompletionStateNone &&
            oldState != FxRequestCompletionStateNone) {
            //
            // Cancelling a callback, so release the callback reference
            //
            RELEASE(FXREQUEST_STATE_TAG);
        }
        else if (NewState != FxRequestCompletionStateNone &&
                 oldState == FxRequestCompletionStateNone) {
            //
            // Adding a callback requires a reference
            //
            ADDREF(FXREQUEST_STATE_TAG);
        }
        else {
            //
            // else we leave the current reference alone
            //
            DO_NOTHING();
        }
    }

    return oldState;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyInsertIrpQueue) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxIrpQueue* IrpQueue
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    //
    // Check to make sure we are not already in an Irp queue
    //
    if (m_IrpQueue != NULL) {
        status = STATUS_INTERNAL_ERROR;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Already in FxIrpQueue 0x%p WDFREQUEST 0x%p %!STATUS!",
            IrpQueue, GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);

        goto Done;
    }

    //
    // If request is completed, fail.
    // This is because the driver can complete the request
    // right away in another thread while returning STATUS_PENDING
    // from EvtIoDefault
    //
    status = VerifyRequestIsNotCompleted(FxDriverGlobals);

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FxRequest::InsertTailIrpQueue(
    __in FxIrpQueue* IrpQueue,
    __out_opt ULONG*      pRequestCount
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

    // No locking required since only one accessor till inserted on queue

    status = VerifyInsertIrpQueue(pFxDriverGlobals, IrpQueue);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If a request is on an IrpQueue, it must be referenced
    //






    ADDREF(FXREQUEST_QUEUE_TAG);

    ASSERT(m_Completed == FALSE);
    ASSERT(m_IrpQueue == NULL);

    m_IrpQueue = IrpQueue;

    status = IrpQueue->InsertTailRequest(m_Irp.GetIrp(),
                                         &m_CsqContext,
                                         pRequestCount);

    //
    // If this insert failed, we must release the extra reference we took
    //
    if (!NT_SUCCESS(status)) {
        MarkRemovedFromIrpQueue();
        RELEASE(FXREQUEST_QUEUE_TAG);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxRequest::InsertHeadIrpQueue(
    __in FxIrpQueue* IrpQueue,
    __out_opt ULONG*      pRequestCount
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;

    pFxDriverGlobals = GetDriverGlobals();

    // No locking required since only one accessor till inserted on queue

    status = VerifyInsertIrpQueue(pFxDriverGlobals, IrpQueue);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // If a request is on an IrpQueue, it must be referenced
    //






    ADDREF(FXREQUEST_QUEUE_TAG);

    ASSERT(m_Completed == FALSE);
    ASSERT(m_IrpQueue == NULL);

    m_IrpQueue = IrpQueue;

    status = IrpQueue->InsertHeadRequest(m_Irp.GetIrp(),
                                         &m_CsqContext,
                                         pRequestCount);

    //
    // If this insert failed, we must release the extra reference we took
    //
    if (!NT_SUCCESS(status)) {
        MarkRemovedFromIrpQueue();
        RELEASE(FXREQUEST_QUEUE_TAG);
    }

    return status;
}

//
// Remove request from its IRP queue
//
_Must_inspect_result_
NTSTATUS
FxRequest::RemoveFromIrpQueue(
    __in FxIrpQueue* IrpQueue
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    MdIrp pIrp;

    pFxDriverGlobals = GetDriverGlobals();

    //
    // Cancel Safe Queues allow this request
    // removal to be race free even if the
    // request has been cancelled already.
    //
    // It signals this by returning NULL for
    // the Irp.
    //
    pIrp = IrpQueue->RemoveRequest(&m_CsqContext);

    if (pIrp == NULL) {

        //
        // Cancel routine removed it from the cancel
        // safe queue.
        //
        // The cancel handler will remove this reference
        // in FxIoQueue::_IrpCancelForDriver /
        // FxIrpQueue::_WdmCancelRoutineInternal
        //

        return STATUS_CANCELLED;
    }
    else {

        //
        // We retrieved the Irp from the cancel safe queue
        // without it having been cancelled first.
        //
        // It is no longer cancelable
        //
        if (pFxDriverGlobals->FxVerifierOn) {
            if (m_IrpQueue == NULL) {
                DoTraceLevelMessage(
                    pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST 0x%p not on IrpQueue",
                    GetHandle());

                FxVerifierDbgBreakPoint(pFxDriverGlobals);
            }
        }

        MarkRemovedFromIrpQueue();

        RELEASE(FXREQUEST_QUEUE_TAG);

        return STATUS_SUCCESS;
    }
}

//
// Function to return the next FxRequest from an FxIrpQueue
//
_Must_inspect_result_
FxRequest*
FxRequest::GetNextRequest(
    __in FxIrpQueue*  IrpQueue
    )
{
    MdIrp pIrp;
    FxRequest* pRequest;
    PMdIoCsqIrpContext pCsqContext;

    pIrp = IrpQueue->GetNextRequest(&pCsqContext);

    if (pIrp == NULL) {
        return NULL;
    }

    // Irp is not cancellable now
    pRequest = FxRequest::RetrieveFromCsqContext(pCsqContext);

    // Must tell the request it's off the queue
    pRequest->MarkRemovedFromIrpQueue();

    // Remove reference placed when on IrpQueue
    pRequest->RELEASE(FXREQUEST_QUEUE_TAG);

    return pRequest;
}

//
// Function to return an FxRequest from an FxIrpQueue based
// on optional context and/or file object.
//
_Must_inspect_result_
NTSTATUS
FxRequest::GetNextRequest(
    __in FxIrpQueue*  IrpQueue,
    __in_opt MdFileObject FileObject,
    __in_opt FxRequest*   TagRequest,
    __deref_out FxRequest** ppOutRequest
    )
{
    NTSTATUS Status;
    FxRequest* pRequest;
    PMdIoCsqIrpContext TagCsqContext;

    if( TagRequest != NULL ) {
        TagCsqContext = TagRequest->GetCsqContext();
    }
    else {
        TagCsqContext = NULL;
    }

    Status = IrpQueue->GetNextRequest( TagCsqContext, FileObject, &pRequest );
    if( !NT_SUCCESS(Status) ) {
        return Status;
    }

    // Irp is not cancellable now

    // Must tell the request its off the queue
    pRequest->MarkRemovedFromIrpQueue();

    // Remove reference placed when on IrpQueue
    pRequest->RELEASE(FXREQUEST_QUEUE_TAG);

    *ppOutRequest = pRequest;

    return STATUS_SUCCESS;
}

//
// Allow peeking at requests in the IrpQueue
//
_Must_inspect_result_
NTSTATUS
FxRequest::PeekRequest(
    __in FxIrpQueue*          IrpQueue,
    __in_opt FxRequest*           TagRequest,
    __in_opt MdFileObject         FileObject,
    __out_opt PWDF_REQUEST_PARAMETERS Parameters,
    __deref_out FxRequest**         ppOutRequest
    )
{
    NTSTATUS Status;

    PMdIoCsqIrpContext TagContext = NULL;

    //
    // IrpQueue::PeekRequest works with CSQ_CONTEXT
    // structures since this is the only value that
    // is valid across cancellation.
    //
    if( TagRequest != NULL ) {
        TagContext = TagRequest->GetCsqContext();
    }

    Status = IrpQueue->PeekRequest(
                           TagContext,
                           FileObject,
                           ppOutRequest
                           );
    if(NT_SUCCESS(Status)) {

        if( Parameters != NULL ) {
            Status = (*ppOutRequest)->GetParameters(Parameters);
        }
    }

    return Status;
}

_Must_inspect_result_
NTSTATUS
FxRequest::Reuse(
    __in PWDF_REQUEST_REUSE_PARAMS ReuseParams
    )
{
    FxIrp               currentIrp;
    PFX_DRIVER_GLOBALS  pFxDriverGlobals = GetDriverGlobals();

    //
    // Make sure request is not pended in IoTarget.
    //
    if (pFxDriverGlobals->IsVerificationEnabled(1, 9, OkForDownLevel)) {
        SHORT flags;
        KIRQL irql;

        Lock(&irql);
        flags = GetVerifierFlagsLocked();
        if (flags & FXREQUEST_FLAG_SENT_TO_TARGET) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver is trying to reuse WDFREQUEST 0x%p while it is still "
                "active on WDFIOTARGET 0x%p. ",
                GetTraceObjectHandle(), GetTarget()->GetHandle());
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }
        Unlock(irql);
    }

    //
    // For drivers 1.9 and above (for maintaining backwards compatibility)
    // deregister previously registered completion routine.
    //
    if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9)) {
       SetCompletionRoutine(NULL, NULL);
    }

    currentIrp.SetIrp(m_Irp.GetIrp());

    if (currentIrp.GetIrp() != NULL) {
        //
        // Release all outstanding references and restore original fields in
        // the PIRP
        //
        if (m_RequestContext != NULL) {
            m_RequestContext->ReleaseAndRestore(this);
        }

        if (m_IrpAllocation == REQUEST_ALLOCATED_FROM_IO) {
            //
            // An irp presented by io queue can only reset a limited state
            //
            if (ReuseParams->Flags & WDF_REQUEST_REUSE_SET_NEW_IRP) {
                //
                // Not allowed to set a new irp
                //
                return STATUS_WDF_REQUEST_INVALID_STATE;
            }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
            currentIrp.SetStatus(ReuseParams->Status);
            currentIrp.SetCancel(FALSE);
#else
            //
            // For UMDF, host sets cancel flag to false as part of Reuse(), so no
            // need to have a separate call for UMDF (that's why host irp doesn't
            // expose any interface to set this independently).




            //
            currentIrp.Reuse(ReuseParams->Status);
#endif
            m_Completed = FALSE;
            m_Canceled = FALSE;

            return STATUS_SUCCESS;
        }
        else if (m_IrpAllocation == REQUEST_ALLOCATED_DRIVER) {
            //
            // Release outstanding reference on a must complete driver request.
            //
            if (m_CanComplete && m_Completed == FALSE) {
                ASSERT(GetRefCnt() >= 2);

                if (pFxDriverGlobals->FxVerifierOn) {
                    ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_OWNED);
                }

                RELEASE(FXREQUEST_DCRC_TAG);
            }
        }
    }
    else {
        //
        // We should not have a m_RequestContext with anything to ReleaseAndRestore
        // because there is no IRP to have anything formatted off of.
        //
        DO_NOTHING();
    }

    //
    // This cannot be a request on a queue
    //
    ASSERT(m_CompletionState == FxRequestCompletionStateNone &&
           m_IoQueue == NULL);

    if (ReuseParams->Flags & WDF_REQUEST_REUSE_SET_NEW_IRP) {
        currentIrp.SetIrp((MdIrp)ReuseParams->NewIrp);

        //
        // If we are replacing an internal irp, we must free it later.
        //
        if (m_IrpAllocation == REQUEST_ALLOCATED_INTERNAL) {
            MdIrp pOldIrp;

            ASSERT(m_CanComplete == FALSE);
            pOldIrp = m_Irp.SetIrp(currentIrp.GetIrp());

            if (pOldIrp != NULL) {
                FxIrp oldIrp(pOldIrp);
                oldIrp.FreeIrp();
            }
        }
        else {
            (void) m_Irp.SetIrp(currentIrp.GetIrp());
        }

        m_IrpAllocation = REQUEST_ALLOCATED_DRIVER;
    }

    //
    // Only reinitialize an internal irp.  If the irp is external, then its
    // still valid.
    //
    if (m_IrpAllocation == REQUEST_ALLOCATED_INTERNAL && currentIrp.GetIrp() != NULL) {
        ASSERT(m_CanComplete == FALSE);
        ASSERT(m_Completed == FALSE);
        currentIrp.Reuse(ReuseParams->Status);

        //
        // For UMDF, host sets cancel flag to false as part of Reuse(), so no
        // need to have a separate call for UMDF (that's why host irp doesn't
        // expose any interface to set this independently).




        //
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        currentIrp.SetCancel(FALSE);
#endif
    }

    //
    // If necessary, reinit the request to support WdfRequestComplete.
    //
    if (ReuseParams->Flags & WDF_REQUEST_REUSE_MUST_COMPLETE) {
        NTSTATUS status;

        //
        // WDF guarantees a successful return code when the driver calls Reuse() from its
        // completion routine with valid input.
        //

        //
        // This feature can only be used from WDF v1.11 and above.
        //
        if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11) == FALSE) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST %p doesn't belong to any queue, %!STATUS!",
                GetTraceObjectHandle(), status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }

        //
        // 'must_complete' flag requires an IRP.
        //
        if (currentIrp.GetIrp() == NULL) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver is trying to reuse WDFREQUEST 0x%p without "
                "specifying an IRP with "
                "WDF_REQUEST_REUSE_MUST_COMPLETE flag, %!STATUS!",
                GetTraceObjectHandle(), status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }

        //
        // Do not support internal IRPs.
        //
        if (m_IrpAllocation == REQUEST_ALLOCATED_INTERNAL) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver is trying to reuse WDFREQUEST 0x%p holding an"
                "internal allocated IRP with "
                "WDF_REQUEST_REUSE_MUST_COMPLETE flag, %!STATUS!",
                GetTraceObjectHandle(), status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }

        //
        // Ref count must be 1.
        //
        if (GetRefCnt() != 1) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Driver is trying to reuse WDFREQUEST 0x%p with "
                "WDF_REQUEST_REUSE_MUST_COMPLETE flag while request is "
                "being referenced, reference count:%d, %!STATUS!",
                GetTraceObjectHandle(), GetRefCnt(), status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }

        //
        // Make sure current IRP stack location is valid.
        //
        if (currentIrp.IsCurrentIrpStackLocationValid() == FALSE) {
            status =  STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "IRP %p of WDFREQUEST %p doesn't have a valid"
                " stack location, %!STATUS!",
                currentIrp.GetIrp(), GetHandle(), status);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return status;
        }

        //
        // This ref is removed when:
        //   (a) the request is completed or
        //   (b) the request is reused and old request/IRP was never sent and completed.
        //
        ADDREF(FXREQUEST_DCRC_TAG);

        //
        // Note: strange why ClearFieldsForReuse() is not used all the time...  just in case,
        // keeping old logic for compatibility.
        //
        ClearFieldsForReuse();
        m_CanComplete = TRUE;

        if (pFxDriverGlobals->FxVerifierOn) {
            SetVerifierFlags(FXREQUEST_FLAG_DRIVER_OWNED);
        }
    }
    else {
        m_CanComplete = FALSE;
        m_Completed = FALSE;
        m_Canceled = FALSE;

        if (pFxDriverGlobals->FxVerifierOn) {
            ClearVerifierFlags(FXREQUEST_FLAG_DRIVER_OWNED);
        }
    }

    return STATUS_SUCCESS;
}

//
// Return the FxFileObject if associated with this request
//
_Must_inspect_result_
NTSTATUS
FxRequest::GetFileObject(
    __deref_out_opt FxFileObject** FileObject
    )
{
    NTSTATUS status;
    FxFileObject* pFileObject;
    CfxDevice* pDevice;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFileObject = NULL;
    pFxDriverGlobals = GetDriverGlobals();

    pDevice = GetDevice();

    if (pFxDriverGlobals->FxVerifierIO) {
        KIRQL irql;
        *FileObject = NULL;

        Lock(&irql);
        status = VerifyRequestIsNotCompleted(pFxDriverGlobals);

        Unlock(irql);
        if (!NT_SUCCESS(status)) {
            return status;
        }

    }

    if (NULL == m_Irp.GetFileObject() && IsAllocatedDriver()) {
        ASSERT(TRUE == m_CanComplete);
        //
        // This is a 'must_complete' driver created request.
        //
        *FileObject = NULL;
        return STATUS_SUCCESS;
    }

    status = FxFileObject::_GetFileObjectFromWdm(
        pDevice,
        pDevice->GetFileObjectClass(),
        m_Irp.GetFileObject(),
        &pFileObject
        );

    if (NT_SUCCESS(status) && pFileObject != NULL) {
        *FileObject = pFileObject;
        return STATUS_SUCCESS;
    }
    else if (NT_SUCCESS(status) &&
             FxIsFileObjectOptional(pDevice->GetFileObjectClass())) {
        //
        // Driver told us that it is ok for the file object to be NULL.
        //
        *FileObject = NULL;
        return STATUS_SUCCESS;
    }
    else {
        return STATUS_INVALID_DEVICE_REQUEST;
    }
}

VOID
FxRequest::AddIrpReference(
    VOID
    )
/*++

Routine Description:

    Adds a reference to the IRP contained in the request.

    This is used to check that FxRequest::Complete is not
    called with outstanding references to any IRP related
    fields such as memory buffers.

Arguments:

    None

Return Value:

    None

--*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    pFxDriverGlobals = GetDriverGlobals();

    if (pFxDriverGlobals->FxVerifierOn) {
        KIRQL irql;

        Lock(&irql);

        (VOID)VerifyRequestIsNotCompleted(pFxDriverGlobals);

        Unlock(irql);
    }

    InterlockedIncrement(&m_IrpReferenceCount);

    return;
}

VOID
FxRequest::ReleaseIrpReference(
    VOID
    )
/*++

Routine Description:

    Release a reference to the IRP contained in the request.

Arguments:

    None

Return Value:

    None

--*/
{
    LONG count;

    count = InterlockedDecrement(&m_IrpReferenceCount);

    if( count < 0 ) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "Attempt to release an IRP reference without adding "
                            "one first WDFREQUEST 0x%p",GetHandle());
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

    return;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyProbeAndLock) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE_LOCKED();

    MdEThread thread = m_Irp.GetThread();

    //
    // Some kernel mode drivers issue I/O without setting this
    //
    if (thread != NULL) {
        //
        // Currently DDK level headers don't let us reach into a threads
        // parent process, so we can't do the process level check, just
        // a thread level check.
        //
        if (m_Irp.GetRequestorMode() == UserMode && thread != Mx::GetCurrentEThread()) {
            status = STATUS_ACCESS_VIOLATION;

            // Error, wrong process context...
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "Attempt to access user mode memory from the wrong process "
                "Irp->Tail.Overlay.Thread 0x%p, PsGetCurrentThread 0x%p, "
                "%!STATUS!", thread, Mx::GetCurrentEThread(), status);

            return status;
        }
    }
    else {
        // Irp->Thread should be issued for all user mode requests
        ASSERT(m_Irp.GetRequestorMode() == KernelMode);
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyStopAcknowledge) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ BOOLEAN Requeue
    )
{
    NTSTATUS status;
    KIRQL irql;

    PAGED_CODE_LOCKED();

    Lock(&irql);

    //
    // Make sure the driver is calling this function in the context
    // of EvtIoStop callback.
    //
    status = VerifyRequestIsInEvtIoStopContext(FxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    if (m_Completed == FALSE && Requeue) {

        // Make sure the driver owns the request

        status = VerifyRequestIsDriverOwned(FxDriverGlobals);
        if (!NT_SUCCESS(status)) {
            goto Done;
        }

        //
        // Can't re-enqueue a cancelable request
        //
        status = VerifyRequestIsNotCancelable(FxDriverGlobals);
        if (!NT_SUCCESS(status)) {
            goto Done;
        }
    }

Done:
    Unlock(irql);
    return status;
}

VOID
FxRequest::StopAcknowledge(
    __in BOOLEAN Requeue
    )
/*++

Routine Description:

    This routine saves the acknowledgement in the request object
    which will be looked at and processed later by the queue
    dispatch event loop

Arguments:

    Requeue - if TRUE, put the request back into the head
              of queue from which it was delivered to the driver.
Return Value:

    None

--*/
{
    NTSTATUS status;

    PFX_DRIVER_GLOBALS FxDriverGlobals = GetDriverGlobals();

    status = VerifyStopAcknowledge(FxDriverGlobals, Requeue);
    if(!NT_SUCCESS(status)) {
        return;
    }

    if (Requeue) {
        m_PowerStopState = FxRequestPowerStopAcknowledgedWithRequeue;
    }
    else {
        m_PowerStopState = FxRequestPowerStopAcknowledged;
    }

    return;
}

ULONG
FxRequest::AddRefOverride(
    __in WDFOBJECT_OFFSET Offset,
    __in PVOID Tag,
    __in LONG Line,
    __in_opt PSTR File
    )
{
    if (Offset != 0x0) {
        ASSERT(Offset == FIELD_OFFSET(FxRequest, m_SystemBufferOffset) ||
               Offset == FIELD_OFFSET(FxRequest, m_OutputBufferOffset));
        AddIrpReference();
        return 2;
    }
    else {
        return FxObject::AddRef(Tag, Line, File);
    }
}

ULONG
FxRequest::ReleaseOverride(
    __in WDFOBJECT_OFFSET Offset,
    __in PVOID Tag,
    __in LONG Line,
    __in_opt PSTR File
    )
{
    if (Offset != 0x0) {
        ASSERT(Offset == FIELD_OFFSET(FxRequest, m_SystemBufferOffset) ||
               Offset == FIELD_OFFSET(FxRequest, m_OutputBufferOffset));
        ReleaseIrpReference();
        return 1;
    }
    else {
        return FxObject::Release(Tag, Line, File);
    }
}

_Must_inspect_result_
NTSTATUS
FxRequest::QueryInterface(
    __in FxQueryInterfaceParams* Params
    )
{
    switch (Params->Type) {
    case FX_TYPE_REQUEST:
        *Params->Object = (FxRequest*) this;
        break;

    case IFX_TYPE_MEMORY:
        if (Params->Offset == FIELD_OFFSET(FxRequest, m_SystemBufferOffset)) {
            *Params->Object = (IFxMemory*) &m_SystemBuffer;
            break;
        }
        else if (Params->Offset == FIELD_OFFSET(FxRequest, m_OutputBufferOffset)) {
            *Params->Object = (IFxMemory*) &m_OutputBuffer;
            break;
        }

        //  ||   ||   Fall      ||  ||
        //  \/   \/   through   \/  \/
    default:
        return FxRequestBase::QueryInterface(Params); // __super call
    }

    return STATUS_SUCCESS;
}

VOID
FX_VF_METHOD(FxRequest, VerifierBreakpoint_RequestEarlyDisposeDeferred) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    PAGED_CODE_LOCKED();

    //
    // For backwards compatibility break only if WDF is v1.11 or above, or if
    // the developer/client enabled these tests on down-level drivers.
    //
    if (FxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel)) {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p deferred the dispose operation. This normally "
            "indicates that at least one of its children asked for passive "
            "level disposal. This is not supported.", GetHandle());

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsDriverOwned) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_OWNED) == 0) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p is not owned by the driver, %!STATUS!",
            GetHandle(), status);

        //
        // See if it's a tag request, since this could be a common mistake
        //
        if (m_VerifierFlags & FXREQUEST_FLAG_TAG_REQUEST) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST 0x%p has been "
                "used as a TagRequest in WdfIoQueueFindRequest. "
                "A TagRequest cannot be used until it is retrieved "
                "by WdfIoQueueRetrieveFoundRequest",
                GetHandle());
        }

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsCancelable)(
     _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_CANCELABLE) == 0) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p is not cancelable, %!STATUS!",
            GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsNotCancelable)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if (m_VerifierFlags & FXREQUEST_FLAG_DRIVER_CANCELABLE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p should be unmarked cancelable by calling "
            "WdfRequestUnmarkCancelable, %!STATUS!",
            GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsInCallerContext)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_INPROCESS_CONTEXT) == 0) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "This call is valid only in EvtIoInCallerContext callback, "
            "WDFREQUEST 0x%p, %!STATUS!", GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsInEvtIoStopContext)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_IN_EVTIOSTOP_CONTEXT) == 0) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "This call is valid only in EvtIoStop callback, "
            "WDFREQUEST 0x%p, %!STATUS!", GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsNotCompleted)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if (m_Completed) {
        status = STATUS_INTERNAL_ERROR;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p is already completed, %!STATUS!",
            GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsTagRequest) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    //
    // A request that has been marked as a tag request can be retrieved
    // by the driver by calling WdfIoQueueRetrieveNextRequest instead of
    // WdfIoQueueRetrieveFoundRequest. Some drivers use multiple threads
    // to scan the queue, not the best design but allowed. This means that
    // it is possible for one thread to remove and complete a request that is
    // used as a tag by another thread.
    //
    if (FALSE == m_Completed && (0x0 == (m_VerifierFlags &
            (FXREQUEST_FLAG_TAG_REQUEST | FXREQUEST_FLAG_DRIVER_OWNED)))) {

       status = STATUS_INVALID_PARAMETER;
       DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                           "Request 0x%p is not returned by WdfIoQueueFindRequest, "
                           "%!STATUS!", GetHandle(), status);
       FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsAllocatedFromIo)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if (IsAllocatedFromIo() == FALSE) {
       status = STATUS_INVALID_PARAMETER;
       DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                           "Request 0x%p was not allocated for an incoming IRP, "
                           "%!STATUS!", GetHandle(), status);
       FxVerifierDbgBreakPoint(FxDriverGlobals);

    }  else {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsCurrentStackValid)(
    _In_ PFX_DRIVER_GLOBALS  FxDriverGlobals
    )
{
    NTSTATUS status;
    MdIrp     irp;

    PAGED_CODE_LOCKED();

    //
    //Make sure there is an IRP.
    //
    irp = GetFxIrp()->GetIrp();
    if (NULL == irp) {
        status =  STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST %p doesn't have an IRP, %!STATUS!",
                            GetHandle(), status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        goto Done;
    }

    //
    // Validate the IRP's stack location values.
    //
    if (m_Irp.IsCurrentIrpStackLocationValid() == FALSE) {
        status =  STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "IRP %p of WDFREQUEST %p doesn't have a valid"
                            " stack location, %!STATUS!",
                            irp, GetHandle(), status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        goto Done;
    }

    status = STATUS_SUCCESS;

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestCanBeCompleted)(
    _In_ PFX_DRIVER_GLOBALS  FxDriverGlobals
    )
{
    NTSTATUS            status;

    PAGED_CODE_LOCKED();

    if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1,11) == FALSE) {
        status = VerifyRequestIsAllocatedFromIo(FxDriverGlobals);
        goto Done;
    }

    //
    // Validate the IRP's stack location.
    //
    status = VerifyRequestIsCurrentStackValid(FxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Note: There is no guarantees that the request has a completion routine in the current
    //          IRP stack location; thus we cannot check for it.
    //

    //
    // Make sure this request can be completed.
    //
    if (IsCanComplete() == FALSE) {
        status =  STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "IRP %p of WDFREQUEST %p cannot be completed, "
                            "%!STATUS!",
                            GetFxIrp()->GetIrp(), GetHandle(), status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        goto Done;
    }

    status = STATUS_SUCCESS;

Done:
    return status;
}

ULONG
FxRequest::Release(
    __in PVOID Tag,
    __in LONG Line,
    __in_opt PSTR File
    )
{
    ULONG   retValue;
    BOOLEAN reservedRequest;
    BOOLEAN allocFromIo;
    BOOLEAN canComplete;

    //
    // This may be the last ref, copy flags before calling Release().
    //
    reservedRequest = IsReserved();
    allocFromIo     = IsAllocatedFromIo();
    canComplete     = IsCanComplete();

    retValue = FxRequestBase::Release(Tag, Line, File); // __super call

    if (reservedRequest && retValue == 1 && m_Completed) {
        //
        // Reserved requests should have an associated queue all the time.
        //
        m_ForwardProgressQueue->ReturnReservedRequest(this);
    }
    else if (allocFromIo == FALSE && canComplete && retValue == 1 && m_Completed) {

        FxRequestCompletionState    state;
        FxIoQueue*                  queue;

        //
        // Make a local copy before request is gone.
        //
        state = (FxRequestCompletionState) m_CompletionState;
        queue = m_IoQueue;

        m_CompletionState = FxRequestCompletionStateNone;
        m_IoQueue = NULL;

        //
        // We are now ready to complete this driver created request.
        //
        FxIrp irp(m_Irp.GetIrp());

        m_Irp.SetIrp(NULL);

        irp.CompleteRequest(GetPriorityBoost());

        PostProcessCompletionForDriverRequest(state, queue);
    }

    return retValue;
}

FxRequestFromLookaside::FxRequestFromLookaside(
    __in CfxDevice* Device,
    __in MdIrp Irp
    ) : FxRequest(Device->GetDriverGlobals(),
                  Irp,
                  FxRequestDoesNotOwnIrp,
                  FxRequestConstructorCallerIsFx,
                  sizeof(FxRequestFromLookaside))
{
    SetDeviceBase(Device->GetDeviceBase());
}

PVOID
FxRequestFromLookaside::operator new(
    __in size_t Size,
    __in CfxDevice* Device,
    __in_opt PWDF_OBJECT_ATTRIBUTES Attributes
    )
{
    UNREFERENCED_PARAMETER(Size);

    //
    // Allocate out of a device specific lookaside list
    //
    return Device->AllocateRequestMemory(Attributes);
}

VOID
FxRequestFromLookaside::SelfDestruct(
    VOID
    )
{
    CfxDevice* pDevice;
    PFX_POOL_HEADER pHeader;

    //
    // Store off the device in case the destructor chain sets it to NULL.
    //
    pDevice = GetDevice();
    ASSERT(pDevice != NULL);

    //
    // Destroy the object
    //
    // FxRequestFromLookaside::~FxRequestFromLookaside(); __REACTOS__

    if (IsRequestForwardedToParent()) {

 #if FX_VERBOSE_TRACE
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                            "Free FxRequest* %p memory", this);
 #endif

        //
        // Remove the request from the list of outstanding requests against this
        // driver.
        //
        pHeader = FxObject::_CleanupPointer(GetDriverGlobals(), this);
        MxMemory::MxFreePool(pHeader->Base);
    }
    else {
        pDevice->FreeRequestMemory(this);
    }
}
