#include "common/fxrequest.h"
#include "common/fxverifier.h"
#include "common/fxdevice.h"



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
    if (m_Completed)
    {
        oldState = FxRequestCompletionStateNone;
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST 0x%p has already been completed",
                            GetHandle());
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }
    else
    {
        ASSERT(m_Irp.GetIrp() != NULL);

        oldState = (FxRequestCompletionState) m_CompletionState;

        m_CompletionState = (BYTE) NewState;

        if (NewState == FxRequestCompletionStateNone &&
            oldState != FxRequestCompletionStateNone)
        {
            //
            // Cancelling a callback, so release the callback reference
            //
            RELEASE(FXREQUEST_STATE_TAG);
        }
        else if (NewState != FxRequestCompletionStateNone &&
                 oldState == FxRequestCompletionStateNone)
        {
            //
            // Adding a callback requires a reference
            //
            ADDREF(FXREQUEST_STATE_TAG);
        }
        else
        {
            //
            // else we leave the current reference alone
            //
            DO_NOTHING();
        }
    }

    return oldState;
}

//__inline
NTSTATUS
FxRequest::Complete(
    __in NTSTATUS Status
)
{
    CfxDevice* const fxDevice = GetDevice();
  
    //
    // Complete the current request object. Can be called directly
    // by the FxIoQueue to complete a request.
    //
    // When an FxRequest is completed, it is marked as completed,
    // removed from any CSQ it may be a member of, and any registered
    // callback functions are called. Then the NT IRP is completed,
    // and the reference count on the object due to the callback routine
    // is released if a callback routine was specified.
    //
    // Completing a request object can cause its reference
    // count to go to zero, thus deleting it. So the caller
    // must either reference it explicitly, or not touch it
    // any more after calling complete.
    //
    DoTraceLevelMessage(
        GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
        "Completing WDFREQUEST 0x%p for IRP 0x%p with "
        "Information 0x%I64x, %!STATUS!",
        GetHandle(), m_Irp.GetIrp(), m_Irp.GetInformation(), Status);
  
    if (fxDevice != NULL)
    {
        SetPriorityBoost(fxDevice->GetDefaultPriorityBoost());
    }
    else
    {
        SetPriorityBoost(0);
    }
  
    return CompleteInternal(Status);
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsNotCompleted)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if (m_Completed)
    {
        status = STATUS_INTERNAL_ERROR;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p is already completed, %!STATUS!",
            GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else
    {
        status = STATUS_SUCCESS;
    }

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
    if (!NT_SUCCESS(status))
    {
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
    if (!NT_SUCCESS(status))
    {
        MarkRemovedFromIrpQueue();
        RELEASE(FXREQUEST_QUEUE_TAG);
    }

    return status;
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
    if (m_IrpQueue != NULL)
    {
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

    if (pIrp == NULL)
    {
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

    if ( TagRequest != NULL )
    {
        TagCsqContext = TagRequest->GetCsqContext();
    }
    else
    {
        TagCsqContext = NULL;
    }

    Status = IrpQueue->GetNextRequest( TagCsqContext, FileObject, &pRequest );
    if ( !NT_SUCCESS(Status) )
    {
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
    if (m_RequestContext != NULL)
    {
        //
        // m_RequestContext will be freed when the FxRequest's destructor runs
        //
        m_RequestContext->ReleaseAndRestore(this);
    }

    //
    // If the request is not presented to the driver then clear the
    // cleanup & destroy callbacks before calling PerformEarlyDispose.
    //
    if (m_Presented == FALSE)
    {
        ClearEvtCallbacks();
    }

    DeleteObject();
}

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

    if (pRequest == NULL)
    {
        DoTraceLevelMessage(
            Device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Memory allocation failed %!STATUS!",
            STATUS_INSUFFICIENT_RESOURCES);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // For forward progress the IRP can be NULL.
    //
    if (Irp != NULL)
    {
        pRequest->AssignMemoryBuffers(Device->GetIoTypeForReadWriteBufferAccess());
    }

    //
    // Improve I/O perf by not parenting it to device. However, if verifier is
    // turned on, the request is parented to the device to help track reference
    // leaks.
    //
    if (Device->GetDriverGlobals()->FxRequestParentOptimizationOn)
    {
        status = pRequest->Commit(RequestAttributes,
                                  NULL,
                                  NULL,
                                  FALSE);
    }
    else
    {
        status = pRequest->Commit(RequestAttributes,
                                  NULL,
                                  Device,
                                  FALSE);
    }

    if (NT_SUCCESS(status))
    {
        *Request = pRequest;
    }
    else
    {
        DoTraceLevelMessage(
            Device->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Could not commit FxRequest %!STATUS!", status);
        pRequest->DeleteFromFailedCreate();
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

    if (m_VerifierFlags & FXREQUEST_FLAG_DRIVER_CANCELABLE)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p should be unmarked cancelable by calling "
            "WdfRequestUnmarkCancelable, %!STATUS!",
            GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsDriverOwned) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_OWNED) == 0)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p is not owned by the driver, %!STATUS!",
            GetHandle(), status);

        //
        // See if it's a tag request, since this could be a common mistake
        //
        if (m_VerifierFlags & FXREQUEST_FLAG_TAG_REQUEST)
        {
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
    else
    {
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
            (FXREQUEST_FLAG_TAG_REQUEST | FXREQUEST_FLAG_DRIVER_OWNED))))
    {
       status = STATUS_INVALID_PARAMETER;
       DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                           "Request 0x%p is not returned by WdfIoQueueFindRequest, "
                           "%!STATUS!", GetHandle(), status);
       FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    return status;
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
    FxRequestFromLookaside::~FxRequestFromLookaside();

    if (IsRequestForwardedToParent())
    {
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
    else
    {
        pDevice->FreeRequestMemory(this);
    }
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

    if (pFxDriverGlobals->FxVerifierOn == FALSE)
    {
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
        (flagsMasked != 0))
    {
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
        if (m_Irp.GetInformation() != 0L)
        {
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


    if ((Status & 0x0FFF0000) == (FACILITY_DRIVER_FRAMEWORK << 16))
    {
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

        //case STATUS_WDF_TOO_MANY_TRANSFERS:
        //    Status = STATUS_INVALID_DEVICE_REQUEST;
        //    break;

        //case STATUS_WDF_NOT_ENOUGH_MAP_REGISTERS:
        //    Status = STATUS_INSUFFICIENT_RESOURCES;
        //    break;

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

    if (IsAllocatedFromIo() == FALSE && IsCanComplete() == FALSE)
    {
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
    if (IsAllocatedFromIo())
    {
        //
        // Set the completion state to none
        //
        m_CompletionState = FxRequestCompletionStateNone;

        if (IsReserved() == FALSE)
        {
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
    if (m_RequestContext != NULL)
    {
        //
        // m_RequestContext will be freed when the FxRequest's desctructor runs
        //
        m_RequestContext->ReleaseAndRestore(this);
    }

    //
    // If the request is not presented to the driver then clear the
    // cleanup & destroy callbacks before calling PerformEarlyDispose.
    //
    if (m_Presented == FALSE)
    {
        ClearEvtCallbacks();
    }

    if (IsReserved() == FALSE && IsAllocatedFromIo())
    {
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
        if (EarlyDispose() == FALSE)
        {
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
    else
    {
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
    if (m_IrpReferenceCount != 0)
    {
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
            (m_RequestBaseStaticFlags & FxRequestBaseStaticSystemBufferValid))
        {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_FATAL, TRACINGREQUEST,
                "WDFMEMORY 0x%p, buffer %p, PMDL %p, length %I64d bytes",
                m_SystemBuffer.GetHandle(), m_SystemBuffer.GetBuffer(),
                m_SystemBuffer.GetMdl(), m_SystemBuffer.GetBufferSize());
        }

        if ((m_RequestBaseFlags & FxRequestBaseOutputMdlMapped) ||
            (m_RequestBaseStaticFlags & FxRequestBaseStaticOutputBufferValid))
        {
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
    else
    {
        //
        // We only clear the irp from this object after PerformEarlyDispose has been
        // called.  m_SystemBuffer and m_OutputBuffer use m_Irp to return their
        // buffers and their WDFMEMORY handles should be valid in the cleanup routine
        // for the WDFREQUEST.  We also keep m_Irp valid until after the
        // m_IrpReferenceCount check, so we can trace out the buffers in case of
        // error.
        //
        m_Irp.SetIrp(NULL);

        if (irp.GetMajorFunction() == IRP_MJ_CREATE)
        {
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
        else
        {
            pRefedDevice = NULL;
        }

        //
        // WDM IRP is completed.
        //
        irp.CompleteRequest(GetPriorityBoost());

        if (IsReserved() == FALSE)
        {
            PostProcessCompletion(state, queue);
        }
        else
        {
            PostProcessCompletionForReserved(state, queue);
        }

        if (pRefedDevice != NULL)
        {
            pRefedDevice->RELEASE(&irp);
            pRefedDevice = NULL;
        }
    }

    return Status;
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
    if (State != FxRequestCompletionStateNone)
    {
        //
        // NOTE: This occurs after the IRP has already been released,
        //       and is only used for notification that the request
        //       has completed.
        //
        if (State & FxRequestCompletionStateIoPkgFlag)
        {
            GetDevice()->m_PkgIo->RequestCompletedCallback(this);
        }
        else
        {
            ASSERT(m_IoQueue == Queue);
            Queue->RequestCompletedCallback(this);
        }
    }
    else
    {
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
    if (FxRequestCompletionStateQueue == State)
    {
        //
        // NOTE: This occurs after the IRP has already been released,  and is only used
        //       to notify the queue to update its internal state and if appropriate, send
        //       another request.
        //
        Queue->PostRequestCompletedCallback(this);
    }
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
    if (State != FxRequestCompletionStateNone)
    {
        //
        // NOTE: This occurs after the IRP has already been released,
        //       and is only used for notification that the request
        //       has completed.
        //
        if (State & FxRequestCompletionStateIoPkgFlag)
        {
            GetDevice()->m_PkgIo->RequestCompletedCallback(this);
        }
        else
        {
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
    else
    {
        //
        // Caller still wants the FxRequest class to be valid on return,
        // but must call DeleteObject in order to ensure the object is
        // no longer assigned any child objects, etc.
        //
        ADDREF(FXREQUEST_COMPLETE_TAG);
        DeleteObject();
    }
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
    if (FxRequestCompletionStateQueue == State)
    {
        //
        // NOTE: This occurs before the IRP has already been released,
        //       and is only used to notify the queue to remove this request from this queue's
        //       internal lists. A second notification (lPostProcessCompletionForAllocatedDriver)
        //       is made after the IRP is completed.
        //
        Queue->PreRequestCompletedCallback(this);
    }
    else if (Queue != NULL)
    {
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

    if (GetDriverGlobals()->FxVerifierIO )
    {
        (VOID) VerifyRequestIsNotCompleted(GetDriverGlobals());
    }
    else
    {
        ASSERT(m_Completed == FALSE);
    }


    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_CANCELABLE) &&
        (m_VerifierFlags & FXREQUEST_FLAG_CANCELLED) == 0x0)
    {
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
        if (m_Irp.GetRequestorMode() == UserMode)
        {
            length = m_Irp.GetParameterIoctlOutputBufferLength();

            if (length > 0)
            {
                validateLength = TRUE;
            }
            else
            {
                //
                // For an output length == 0, a driver can indicate the number
                // of bytes used of the input buffer.
                //
                DO_NOTHING();
            }
        }
        else
        {
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
                m_Irp.GetInformation() > length)
    {
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
    if (FxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel))
    {
        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST %p deferred the dispose operation. Usually this "
            "indicates that some child object of the WDFREQUEST requires "
            "passive level disposal (e.g. WDFTIMER). This is not supported. "
            "Either ensure that the WDFREQUEST always completes at passive "
            "level, or do not parent the object to the WDFREQUEST.", GetHandle());

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
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
    BOOLEAN mapMdl, mdlMapped;
    UCHAR majorFunction;

    status = STATUS_SUCCESS;
    length = 0x0;
    mapMdl = FALSE;
    mdlMapped = FALSE;
    irql = PASSIVE_LEVEL;
    majorFunction = m_Irp.GetMajorFunction();


    //
    // Verifier
    //
    if (GetDriverGlobals()->FxVerifierIO)
    {
        status = VerifyRequestIsNotCompleted(GetDriverGlobals());
        if (!NT_SUCCESS(status))
        {
            goto Done;
        }
        if (m_Irp.GetRequestorMode() == UserMode
            &&
            (majorFunction == IRP_MJ_WRITE ||
             majorFunction == IRP_MJ_READ)
            &&
            GetDevice()->GetIoType() == WdfDeviceIoNeither)
        {
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

    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticSystemBufferValid) == 0x00)
    {
        Lock(&irql);
    }

    //
    // We must dig into the IRP to get the buffer, length, and readonly status
    //

    switch (majorFunction) {
    case IRP_MJ_DEVICE_CONTROL:
    case IRP_MJ_INTERNAL_DEVICE_CONTROL:
        length = m_Irp.GetParameterIoctlInputBufferLength();

        if (length == 0)
        {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                "WDFREQUEST %p InputBufferLength length is zero, %!STATUS!",
                GetObjectHandle(), status);

            goto Done;
        }

        if (m_Irp.GetParameterIoctlCodeBufferMethod() == METHOD_NEITHER)
        {
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
                (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL))
            {
                DO_NOTHING();
            }
            else
            {
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

        if (GetDevice()->GetIoTypeForReadWriteBufferAccess() == WdfDeviceIoDirect)
        {
            KMDF_ONLY_CODE_PATH_ASSERT();
            mapMdl = TRUE;
        }
        break;

    case IRP_MJ_WRITE:
        length = m_Irp.GetParameterWriteLength();

        if (GetDevice()->GetIoTypeForReadWriteBufferAccess() == WdfDeviceIoDirect)
        {
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

    if (length == 0)
    {
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
    if (mapMdl && (m_RequestBaseFlags & FxRequestBaseSystemMdlMapped) == 0x00)
    {
        pMdl = m_Irp.GetMdl();

        if (pMdl == NULL)
        {
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
        else
        {
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

            if (pVA == NULL)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST 0x%p could not get a system address for PMDL "
                    "0x%p, %!STATUS!", GetHandle(), pMdl, status);
            }
            else
            {
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
    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticSystemBufferValid) == 0x00)
    {
        Unlock(irql);
    }

    if (NT_SUCCESS(status))
    {
        *MemoryObject = &m_SystemBuffer;

        if (mapMdl)
        {
            *Buffer = Mx::MxGetSystemAddressForMdlSafe(m_SystemBuffer.m_Mdl,
                                                   NormalPagePriority);
        }
        else
        {
            *Buffer = m_SystemBuffer.m_Buffer;
        }

        *Length = length;
    }

    return status;
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
        if (Params->Offset == FIELD_OFFSET(FxRequest, m_SystemBufferOffset))
        {
            *Params->Object = (IFxMemory*) &m_SystemBuffer;
            break;
        }
        else if (Params->Offset == FIELD_OFFSET(FxRequest, m_OutputBufferOffset))
        {
            *Params->Object = (IFxMemory*) &m_OutputBuffer;
            break;
        }

        //  ||   ||   Fall      ||  ||
        //  \/   \/   through   \/  \/
    default:
        return __super::QueryInterface(Params);
    }

    return STATUS_SUCCESS;
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

    if (pFxDriverGlobals->FxVerifierIO)
    {
        NTSTATUS status;
        KIRQL irql;

        Lock(&irql);

        status = VerifyRequestIsNotCompleted(pFxDriverGlobals);
        if (NT_SUCCESS(status))
        {
            m_Irp.SetInformation(Information);
        }

        Unlock(irql);

        return status;
    }
    else
    {
        m_Irp.SetInformation(Information);
        return STATUS_SUCCESS;
    }
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

    if (pIrp == NULL)
    {
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
    else
    {
        //
        // We retrieved the Irp from the cancel safe queue
        // without it having been cancelled first.
        //
        // It is no longer cancelable
        //
        if (pFxDriverGlobals->FxVerifierOn)
        {
            if (m_IrpQueue == NULL)
            {
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

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsCancelable)(
     _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if ((m_VerifierFlags & FXREQUEST_FLAG_DRIVER_CANCELABLE) == 0)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "WDFREQUEST 0x%p is not cancelable, %!STATUS!",
            GetHandle(), status);

        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
    else
    {
        status = STATUS_SUCCESS;
    }

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

    if (GetDriverGlobals()->IsVersionGreaterThanOrEqualTo(1,11) == FALSE)
    {
        status = VerifyRequestIsAllocatedFromIo(FxDriverGlobals);
        goto Done;
    }

    //
    // Validate the IRP's stack location.
    //
    status = VerifyRequestIsCurrentStackValid(FxDriverGlobals);
    if (!NT_SUCCESS(status))
    {
        goto Done;
    }

    //
    // Note: There is no guarantees that the request has a completion routine in the current
    //          IRP stack location; thus we cannot check for it.
    //

    //
    // Make sure this request can be completed.
    //
    if (IsCanComplete() == FALSE)
    {
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

_Must_inspect_result_
NTSTATUS
FX_VF_METHOD(FxRequest, VerifyRequestIsAllocatedFromIo)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    NTSTATUS status;

    PAGED_CODE_LOCKED();

    if (IsAllocatedFromIo() == FALSE)
    {
       status = STATUS_INVALID_PARAMETER;
       DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                           "Request 0x%p was not allocated for an incoming IRP, "
                           "%!STATUS!", GetHandle(), status);
       FxVerifierDbgBreakPoint(FxDriverGlobals);

    }
    else
    {
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
    if (NULL == irp)
    {
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
    if (m_Irp.IsCurrentIrpStackLocationValid() == FALSE)
    {
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
    if (GetDriverGlobals()->FxVerifierIO )
    {
        status = VerifyRequestIsNotCompleted(GetDriverGlobals());
        if (!NT_SUCCESS(status))
        {
            return status;
        }
    }

    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticOutputBufferValid) == 0x00)
    {
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

    if (length == 0)
    {
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
            (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL))
        {
            DO_NOTHING();
        }
        else
        {
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

    if (mapMdl && (m_RequestBaseFlags & FxRequestBaseOutputMdlMapped) == 0x0)
    {
        PMDL pMdl;
        PVOID pVA;

        pMdl = m_Irp.GetMdl();

        if (pMdl == NULL)
        {
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
        else
        {
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

            if (pVA == NULL)
            {
                status =  STATUS_INSUFFICIENT_RESOURCES;

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST 0x%p could not get a system address for PMDL"
                    "0x%p, %!STATUS!", GetHandle(), pMdl, status);
            }
            else
            {
                m_OutputBuffer.SetMdl(pMdl);
                m_RequestBaseFlags |= FxRequestBaseOutputMdlMapped;
                status = STATUS_SUCCESS;
            }
        }
    }

Done:
    if ((m_RequestBaseStaticFlags & FxRequestBaseStaticOutputBufferValid) == 0x00)
    {
        Unlock(irql);
    }

    if (NT_SUCCESS(status))
    {
        *MemoryObject = &m_OutputBuffer;
        if (mapMdl)
        {
            *Buffer = Mx::MxGetSystemAddressForMdlSafe(m_OutputBuffer.m_Mdl,
                                                   NormalPagePriority);
        }
        else
        {
            *Buffer = m_OutputBuffer.m_Buffer;
        }
        *Length = length;
    }

    return status;
}
