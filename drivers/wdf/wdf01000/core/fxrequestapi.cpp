/*
 * PROJECT:     ReactOS Wdf01000 driver
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Request api functions
 * COPYRIGHT:   Copyright 2020 mrmks04 (mrmks04@yandex.ru)
 */


#include "common/fxglobals.h"
#include "common/fxrequest.h"
#include "common/ifxmemory.h"
#include "common/fxioqueue.h"
#include "common/fxdevice.h"
#include "common/fxpkggeneral.h"


extern "C" {


//
// Verifiers
//
// Do not supply Argument names 
FX_DECLARE_VF_FUNCTION_P1(
NTSTATUS, 
VerifyRequestComplete, 
    _In_ FxRequest*
    );


__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFAPI
WDFEXPORT(WdfRequestGetIoQueue)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    Returns the queue handle that currently owns the request.


Arguments:

    Request - Handle to the Request object


Returns:

    WDFQUEUE

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest *pRequest;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    if (pRequest->GetCurrentQueue() == NULL)
    {
        //
        // For a driver-created request, the queue can be NULL. It is not 
        // necessarily an error to call WdfRequestGetIoQueue on a driver-
        // created request, because the caller may not really know whether or 
        // not the request is driver-created. 
        // 
        // For example, it is possible for a class extension to create a request
        // and pass it to the client driver, in which case the client driver 
        // wouldn't really know whether or not it was driver-created. Or a 
        // client driver is might be using a helper library for some of its 
        // tasks and it might pass in a request object to the helper library. In
        // this case, the helper library wouldn't really know whether or not the
        // request was driver-created. Therefore, the log message below is at 
        // verbose level and not at error or warning level.
        //
        DoTraceLevelMessage(pFxDriverGlobals, 
                            TRACE_LEVEL_VERBOSE, 
                            TRACINGREQUEST,
                            "WDFREQUEST %p doesn't belong to any queue",
                            Request);
        return NULL;
    }

    if (pRequest->GetFxIrp()->GetMajorFunction() == IRP_MJ_CREATE)
    {
        //
        // If the queue for Create is the framework internal queue
        // return NULL. 
        //
        FxPkgGeneral* devicePkgGeneral = pRequest->GetDevice()->m_PkgGeneral;
        
        if (devicePkgGeneral->GetDeafultInternalCreateQueue() == 
                pRequest->GetCurrentQueue())
        {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "Getting queue handle for Create request is "
                                "not allowed for WDFREQUEST 0x%p", pRequest);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return  NULL;
        }
    }

    return (WDFQUEUE) pRequest->GetCurrentQueue()->GetObjectHandle();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
NTAPI
WDFEXPORT(WdfRequestCompleteWithInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus,
    __in
    ULONG_PTR Information
    )

/*++

Routine Description:

    Complete the request with supplied status and information.

    Any default reference counts implied by handle are invalid after
    completion.

Arguments:

    Request        - Handle to the Request object

    RequestStatus  - Wdm Status to complete the request with

    Information    - Information to complete request with

Returns:

    None

--*/

{
    FxRequest *pRequest;
    NTSTATUS status;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*)&pRequest);

#if FX_VERBOSE_TRACE
    //
    // Use the object's globals, not the caller's
    //
    DoTraceLevelMessage(pRequest->GetDriverGlobals(),
                        TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Completing WDFREQUEST 0x%p, %!STATUS!",
                        Request, RequestStatus);
#endif
    status = VerifyRequestComplete(pRequest->GetDriverGlobals(), pRequest);
    if (!NT_SUCCESS(status))
    {
        return;
    }


    pRequest->CompleteWithInformation(RequestStatus, Information);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
NTAPI
WDFEXPORT(WdfRequestRetrieveOutputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    )

/*++

Routine Description:

    Return the WDFMEMORY buffer associated with the request.

    The memory buffer is valid in any thread/process context,
    and may be accessed at IRQL > PASSIVE_LEVEL.

    The memory buffer is automatically released when the request
    is completed.

    The memory buffers access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The memory buffer is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The Memory buffer is as follows for each buffering mode:

    DO_BUFFERED_IO:

        Irp->AssociatedIrp.SystemBuffer

    DO_DIRECT_IO:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The buffer is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    Memory - Pointer location to return WDFMEMORY handle

Returns:

    NTSTATUS

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    IFxMemory* pMemory;
    PVOID pBuffer;
    size_t length;
    UCHAR majorFunction;

    pMemory = NULL;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Memory);

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    //
    // This call is not valid on Write request.
    //
    majorFunction = pRequest->GetFxIrp()->GetMajorFunction();

    if (majorFunction == IRP_MJ_WRITE)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on the Write request, you should call"
            " WdfRequestRetrieveInputMemory to get the Memory for WDFREQUEST "
            "0x%p, %!STATUS!", Request, status);

        return status;
    }

    if ( (majorFunction == IRP_MJ_DEVICE_CONTROL) ||
        (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) )
    {
        status = pRequest->GetDeviceControlOutputMemoryObject(&pMemory, &pBuffer, &length);
    }
    else
    {
        status = pRequest->GetMemoryObject(&pMemory, &pBuffer, &length);
    }

    if (NT_SUCCESS(status))
    {
        *Memory = pMemory->GetHandle();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_FUNCTION(VerifyRequestComplete) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals, 
    _In_ FxRequest* pRequest
    )
{
    NTSTATUS status;
    KIRQL irql;

    PAGED_CODE_LOCKED();

    pRequest->Lock(&irql);

    status = pRequest->VerifyRequestIsDriverOwned(FxDriverGlobals);
    if (NT_SUCCESS(status))
    {
        status = pRequest->VerifyRequestCanBeCompleted(FxDriverGlobals);
    }

    pRequest->Unlock(irql);
    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
NTAPI
WDFEXPORT(WdfRequestComplete)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus
    )

/*++

Routine Description:

    Complete the request with supplied status.

    Any default reference counts implied by handle are invalid after
    completion.

Arguments:

    Request        - Handle to the Request object

    RequestStatus  - Wdm Status to complete the request with

Returns:

    None

--*/
{
    NTSTATUS status;
    FxRequest *pRequest;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                          Request,
                          FX_TYPE_REQUEST,
                          (PVOID*)&pRequest);
#if FX_VERBOSE_TRACE
    //
    // Use object's globals, not the caller's
    //
    DoTraceLevelMessage(pRequest->GetDriverGlobals(),
                        TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Completing WDFREQUEST 0x%p, %!STATUS!",
                        Request, RequestStatus);
#endif    
    status = VerifyRequestComplete(pRequest->GetDriverGlobals(), pRequest );
    if (!NT_SUCCESS(status))
    {
        return;
    }

    pRequest->Complete(RequestStatus);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
NTAPI
WDFEXPORT(WdfRequestSetInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    ULONG_PTR Information
    )

/*++

Routine Description:

    Set the transfer information for the request.

    This sets the NT Irp->Status.Information field.

Arguments:

    Request     - Handle to the Request object

    Information - Value to be set

Returns:

    None

--*/

{
    FxRequest *pRequest;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*)&pRequest);

#if FX_VERBOSE_TRACE
    //
    // Use the object's globals, not the caller's
    //
    DoTraceLevelMessage(pRequest->GetDriverGlobals(),
                        TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p, Information 0x%p",
                        Request, (VOID*)Information);
#endif // FX_VERBOSE_TRACE

    pRequest->SetInformation(Information);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfRequestMarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   PFN_WDF_REQUEST_CANCEL  EvtRequestCancel
   )

/*++

Routine Description:

    Mark the specified request as cancelable

Arguments:

    Request - Request to mark as cancelable.
    
    EvtRequestCancel - cancel routine to be invoked when the
                            request is cancelled.

Returns:

    None

--*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
NTAPI
WDFEXPORT(WdfRequestRetrieveInputMemory)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    WDFMEMORY *Memory
    )
/*++

Routine Description:

    Return the WDFMEMORY buffer associated with the request.

    The memory buffer is valid in any thread/process context,
    and may be accessed at IRQL > PASSIVE_LEVEL.

    The memory buffer is automatically released when the request
    is completed.

    The memory buffers access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The memory buffer is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The Memory buffer is as follows for each buffering mode:

    DO_BUFFERED_IO:

        Irp->AssociatedIrp.SystemBuffer

    DO_DIRECT_IO:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The buffer is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    Memory - Pointer location to return WDFMEMORY handle

Returns:

    NTSTATUS

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    IFxMemory* pMemory;
    PVOID pBuffer;
    size_t length;

    pMemory = NULL;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Memory);

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    //
    // This call is not valid on Read request.
    //
    if (pRequest->GetFxIrp()->GetMajorFunction() == IRP_MJ_READ)
    {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on the Read request, you should call"
            " WdfRequestRetrieveOutputMemory to get the Memory for WDFREQUEST "
            "0x%p, %!STATUS!", Request, status);

        return status;
    }

    status = pRequest->GetMemoryObject(&pMemory, &pBuffer, &length);
    if (NT_SUCCESS(status))
    {
        *Memory = pMemory->GetHandle();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfRequestUnmarkCancelable)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request
   )

/*++

Routine Description:

    Unmark the specified request as cancelable

Arguments:

    Request - Request to unmark as cancelable.

Returns:

    NTSTATUS

--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest* pRequest;

    //
    // Validate request object handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    if (pRequest->GetCurrentQueue() == NULL)
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST %p doesn't belong to any queue %!STATUS!",
                            Request, STATUS_INVALID_DEVICE_REQUEST);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return pRequest->GetCurrentQueue()->RequestCancelable(pRequest, 
                                                        FALSE, 
                                                        NULL, 
                                                        FALSE);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
NTAPI
WDFEXPORT(WdfRequestMarkCancelableEx)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   PFN_WDF_REQUEST_CANCEL  EvtRequestCancel
   )

/*++

Routine Description:

    Mark the specified request as cancelable. Do not call the specified cancel 
    routine if IRP is already cancelled but instead return STATUS_CANCELLED.
    Caller is responsible for completing the request with STATUS_CANCELLED.

Arguments:

    Request - Request to mark as cancelable.
    
    EvtRequestCancel - cancel routine to be invoked when the
                                request is cancelled.

Returns:

    STATUS_SUCCESS      - The request has been marked cancelable.
    STATUS_CANCELLED    - The IRP is already cancelled.
    NTSTATUS            - Other values are possible when verifier is enabled.
    
--*/

{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest* pRequest;
    NTSTATUS status;

    //
    // Validate request object handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, EvtRequestCancel);

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    if (pRequest->GetCurrentQueue() == NULL)
    {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST %p doesn't belong to any queue %!STATUS!",
                            Request, STATUS_INVALID_DEVICE_REQUEST);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    status = pRequest->GetCurrentQueue()->RequestCancelable(pRequest,
                                                        TRUE,
                                                        EvtRequestCancel,
                                                        TRUE);

    ASSERT(status == STATUS_SUCCESS || status == STATUS_CANCELLED);
    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    __in_opt
    WDFIOTARGET Target,
    __out
    WDFREQUEST* Request
    )

/*++

Routine Description:
    Creates a WDFREQUEST handle that is suitable to be submitted to the provided
    target

Arguments:
    RequestAttributes - Attributes associated with the request.  If NULL, the
        size of the user context associated with the request will be
        the default size specified in WdfDriverCreate.

    Target - Target for which the request will be sent to.  If NULL, then
        WdfRequestChangeTarget must be called before the request is formatted
        or sent to any target

    Request - Pointer which will receive the newly created request

Return Value:
    NT_SUCCESS if successful, otherwise appropriate error code

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestCreateFromIrp)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES RequestAttributes,
    __in
    MdIrp Irp,
    __in
    BOOLEAN RequestFreesIrp,
    __out
    WDFREQUEST* Request
    )

/*++

Routine Description:
    Creates a request handle that uses an external IRP instead of an internally
    allocated irp.

Arguments:
    RequestAttributes - Attributes associated with the request.  If NULL, the
        size of the user context associated with the request will be
        the default size specified in WdfDriverCreate.

    Irp - The IRP to use

    RequestFreesIrp - If TRUE, when the request handle is destroyed, it will
        free the IRP with IoFreeIrp.  If FALSE, it is the responsibility of the
        calller to free the IRP

    Request - Pointer which will receive the newly created request

Return Value:
    NT_SUCCESS or appropriate error code

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestReuse)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    PWDF_REQUEST_REUSE_PARAMS ReuseParams
    )
/*++

Routine Description:
    Clears out the internal state of the irp, which includes, but is not limited
    to:
    a)  Any internal allocations for the previously formatted request
    b)  The completion routine and its context
    c)  The request's intended i/o target
    d)  All of the internal IRP's stack locations

Arguments:
    Request - The request to be reused.

    ReuseParams - Parameters controlling the reuse of the request, see comments
        for each field in the structure for usage

Return Value:
    NT_SUCCESS or appropriate error code

  --*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestChangeTarget)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    WDFIOTARGET IoTarget
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfRequestFormatRequestUsingCurrentType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )
/*++

Routine Description:
    Copies the current Irp stack location to the next one.  This is the
    equivalent of IoCopyCurrentIrpStackLocationToNext.

Arguments:
    Request - The request that will be formatted.

Return Value:
    None

  --*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestWdmFormatUsingStackLocation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    PIO_STACK_LOCATION Stack
    )

/*++

Routine Description:
    Sets the next IRP stack location to the one provided by the caller.  The
    Context and CompletionRoutine values will be ignored.  If the caller wants
    to set a completion routine, WdfRequestSetCompletionRoutine should be used
    instead.

Arguments:
    Request - The request to be formatted

    Stack - A pointer to an IO_STACK_LOCATION structure that contains 
        driver-supplied information

Return Value:
    None.

  --*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfRequestSend)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    WDFIOTARGET Target,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS Options
    )
/*++

Routine Description:
    Sends a previously created and formatted request to the target device
    object.  The target device object will typically be the device object that
    this device is attached to.  The submission can also be controlled by a set
    of options.

Arguments:
    Request - The request to be submitted

    Target - The target of the request

    Options - Optional options applied to the sending of the request

    In the aggressive attempt to conserve stack space, the passed in parameters
    are unionized with the locals this function would need.  On an optimized
    build, the compiler may already do this, but we want to be extra aggressive
    and ensure that this type of stack reuse is done.

Return Value:
    TRUE if the request was sent to the target, FALSE otherwise.

    To retrieve the status of the request, call WdfRequestGetStatus.
    WdfRequestGetStatus should only be called if WdfRequestSend returns FALSE
    or if the caller specified that the request be synchronous in
    WDF_REQUEST_SEND_OPTIONS.  Otherwise, the request is asynchronous and the status
    will be returned in the request's completion routine.

  --*/
{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestGetStatus)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfRequestIsCanceled)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request
   )

/*++

Routine Description:

    Check to see if the request is cancelled by the I/O manager.
    This call is valid only on a driver owned non-cancelable request.

Arguments:

    Request - Request being checked.

Returns:

    BOOLEAN

--*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFAPI
WDFEXPORT(WdfRequestCancelSentRequest)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:
    Cancels a previously submitted request.

Arguments:
    Request - The previously submitted request

Return Value:
    TRUE if the cancel was *attempted*.  The caller must still synchronize with
    the request's completion routine since TRUE just means the owner of the
    request was successfully asked to cancel the request.

  --*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

_Must_inspect_result_
__drv_maxIRQL(APC_LEVEL)
BOOLEAN
WDFEXPORT(WdfRequestIsFrom32BitProcess)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
   )
/*++

Routine Description:
    Indicates to the caller if the I/O request originated in a 32 bit process
    or not.  On 32 bit systems, this function always returns TRUE.

Arguments:
    Request - The request being queried

Return Value:
    TRUE if the request came from a 32 bit process, FALSE otherwise.

  --*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestSetCompletionRoutine)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in_opt
    PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
    __in_opt
    WDFCONTEXT CompletionContext
    )
{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestGetCompletionParams)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    PWDF_REQUEST_COMPLETION_PARAMS Params
    )
{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestAllocateTimer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )
/*++

Routine Description:
    Preallocates a timer to be associated with the passed in request object.
    By preallocating the timer, WdfSendRequest cannot fail with insufficient
    resources when attempting to allocate a timer when a timeout constraint has
    been passed in.

    If the request already has a timer allocated for it, then the function will
    succeed.

Arguments:
    Request - the request to allocate a timer for

Return Value:
    NT_SUCCESS upon success, STATUS_INSUFFICIENT_RESOURCES upon failure

  --*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestCompleteWithPriorityBoost)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    NTSTATUS RequestStatus,
    __in
    CCHAR PriorityBoost
    )

/*++

Routine Description:

    Complete the request with supplied status.

    Any default reference counts implied by handle are invalid after
    completion.

Arguments:

    Request        - Handle to the Request object

    RequestStatus  - Wdm Status to complete the request with

    PriorityBoost  - A system-defined constant value by which to increment the 
        run-time priority of the original thread that requested the operation.

Returns:

    None

--*/

{
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveInputBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    size_t RequiredLength,
    __deref_out_bcount(*Length)
    PVOID* Buffer,
    __out_opt
    size_t* Length
    )
/*++

Routine Description:

    Return the memory buffer associated with the request along
    with its maximum length.

    The memory buffer is valid in any thread/process context,
    and may be accessed at IRQL > PASSIVE_LEVEL.

    The memory buffer is automatically released when the request
    is completed.

    The memory buffers access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The memory buffer is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The Memory buffer is as follows for each buffering mode:

    DO_BUFFERED_IO:

        Irp->AssociatedIrp.SystemBuffer

    DO_DIRECT_IO:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The buffer is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    RequiredLength - This is the minimum size expected by the caller

    Buffer - Pointer location to return buffer ptr

    Length - actual size of the buffer. This is >= to RequiredLength

Returns:

    NTSTATUS

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveOutputBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    size_t RequiredLength,
    __deref_out_bcount(*Length)
    PVOID* Buffer,
    __out_opt
    size_t* Length
    )
/*++

Routine Description:

    Return the memory buffer associated with the request along
    with its maximum length.

    The memory buffer is valid in any thread/process context,
    and may be accessed at IRQL > PASSIVE_LEVEL.

    The memory buffer is automatically released when the request
    is completed.

    The memory buffers access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The memory buffer is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The Memory buffer is as follows for each buffering mode:

    DO_BUFFERED_IO:

        Irp->AssociatedIrp.SystemBuffer

    DO_DIRECT_IO:

        MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority)

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The buffer is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    RequiredLength - This is the minimum size expected by the caller

    Buffer - Pointer location to return buffer ptr

    Length - actual size of the buffer. This is >= to RequiredLength

Returns:

    NTSTATUS

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveInputWdmMdl)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __deref_out
    PMDL *Mdl
    )

/*++

Routine Description:

    Return the MDL associated with the request.

    The MDL is automatically released when the request is completed.

    The MDL's access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The MDL is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The MDL is as follows for each buffering mode:

    DO_BUFFERED_IO:

        MmBuildMdlForNonPagedPool(IoAllocateMdl(Irp->AssociatedIrp.SystemBuffer, ... ))

    DO_DIRECT_IO:

        Irp->MdlAddress

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The MDL is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    Mdl - Pointer location to return MDL ptr

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveOutputWdmMdl)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __deref_out
    PMDL *Mdl
    )

/*++

Routine Description:

    Return the MDL associated with the request.

    The MDL is automatically released when the request is completed.

    The MDL's access permissions are validated according
    to the command type (IRP_MJ_READ, IRP_MJ_WRITE), and may
    only be accessed according to the access semantics of the request.

    The MDL is not valid for a METHOD_NEITHER IRP_MJ_DEVICE_CONTROL,
    or if neither of the DO_BUFFERED_IO or DO_DIRECT_IO flags are
    configured for the device object.

    The MDL is as follows for each buffering mode:

    DO_BUFFERED_IO:

        MmBuildMdlForNonPagedPool(IoAllocateMdl(Irp->AssociatedIrp.SystemBuffer, ... ))

    DO_DIRECT_IO:

        Irp->MdlAddress

    NEITHER flag set:

        NULL. Must use WdfDeviceInitSetIoInCallerContextCallback in order
        to access the request in the calling threads address space before
        it is placed into any I/O Queues.

    The MDL is only valid until the request is completed.

Arguments:

    Request - Handle to the Request object

    Mdl - Pointer location to return MDL ptr

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveUnsafeUserInputBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    size_t RequiredLength,
    __deref_out_bcount_opt(*Length)
    PVOID* InputBuffer,
    __out_opt
    size_t* Length
    )
/*++

Routine Description:

    Returns input buffer of a method-neither request. This function can be
    called only in the context of EvtDeviceIoInProcessContextCallback at
    PASSIVE_LEVEL.

    This call is valid on public IOCTL and Write request.

    The returned buffer is valid only in the caller's process context. This
    call should be typically used in a toplevel or monolithic driver to
    guarantee the caller's context.


    The Memory buffer is as follows for each type of request:

    For IOCTL, it will return irpStack->Parameters.DeviceIoControl.Type3InputBuffer

    For Write, it will return  Irp->UserBuffer.

    For read and internal-device control and other type of request, this call
    will return an error.


Arguments:

    Request - Handle to the Request object

    RequiredLength - minimum length of the buffer expected by the caller.
                     If it's not this call will return an error.


    InputBuffer - Pointer location to return buffer ptr

    Length - actual size of the buffer. This is >= to RequiredLength

Returns:

    NTSTATUS

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestRetrieveUnsafeUserOutputBuffer)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    size_t RequiredLength,
    __deref_out_bcount_opt(*Length)
    PVOID* OutputBuffer,
    __out_opt
    size_t* Length
    )
/*++

Routine Description:

    Returns output buffer of a method-neither request. This function can be called only
    in the context of EvtDeviceIoInProcessContextCallback at PASSIVE_LEVEL.

    This call is valid on public IOCTL and Read request.

    The returned buffer is valid only in the caller's process context. This call should
    be typically used in a toplevel or monolithic driver to guarantee the caller's context.


    The Memory buffer is as follows for each type of request:

    For IOCTL, it will return Irp->UserBuffer

    For Read, it will return  Irp->UserBuffer.

    For Write and internal-device control and other type of request, this call will return an error.


Arguments:

    Request - Handle to the Request object

    RequiredLength - minimum length of the buffer expected by the caller. If it's not
                        this call will return an error.


    OutputBuffer - Pointer location to return buffer ptr

    Length - actual size of the buffer. This is >= to RequiredLength


Returns:

    NTSTATUS

--*/
{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
ULONG_PTR
WDFAPI
WDFEXPORT(WdfRequestGetInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    Get the transfer information for the reuqest.


Arguments:

    Request     - Handle to the Request object


Returns:

   Returns Irp->IoStatus.Information value.

--*/

{
    WDFNOTIMPLEMENTED();
    return 0;
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFFILEOBJECT
WDFAPI
WDFEXPORT(WdfRequestGetFileObject)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    This routine returns the WDFFILEOBJECT associated with the given
    request.

Arguments:

    Request - Handle to the Request object

Returns:

    WDFFILEOBJECT handle.

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestProbeAndLockUserBufferForRead)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in_bcount(Length)
    PVOID Buffer,
    __in
    size_t Length,
    __out
    WDFMEMORY* MemoryObject
    )

/*++

Routine Description:

    This routine probes and locks the specified user mode address into
    an MDL, and associates it with the WDFREQUEST object.

    The MDL, and its associated system buffer is represented by a WDFMEMORY
    object.

    The WDFMEMORY object and the MDL is automatically released when the
    WDFREQUEST is completed by WdfRequestComplete.

Arguments:

    Request - Handle to the Request object

    Buffer - Buffer to probe and lock into an MDL

    Length - Length of buffer

    MemoryObject - Location to return WDFMEMORY handle

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfRequestProbeAndLockUserBufferForWrite)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in_bcount(Length)
    PVOID Buffer,
    __in
    size_t Length,
    __out
    WDFMEMORY* MemoryObject
    )

/*++

Routine Description:

    This routine probes and locks the specified user mode address into
    an MDL, and associates it with the WDFREQUEST object.

    The MDL, and its associated system buffer is represented by a WDFMEMORY
    object.

    The WDFMEMORY object and the MDL is automatically released when the
    WDFREQUEST is completed by WdfRequestComplete.

Arguments:

    Request - Handle to the Request object

    Buffer - Buffer to probe and lock into an MDL

    Length - Length of buffer

    MemoryObject - Location to return WDFMEMORY handle

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
KPROCESSOR_MODE
WDFAPI
WDFEXPORT(WdfRequestGetRequestorMode)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    Returns the RequestorMode information from the IRP.


Arguments:

    Request - Handle to the Request object


Returns:

    KPROCESSOR_MODE is CCHAR

--*/

{
    WDFNOTIMPLEMENTED();
    return MODE::MaximumMode;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestForwardToIoQueue)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   WDFQUEUE DestinationQueue
   )

/*++

Routine Description:

    Forward a request presented on one queue to another driver
    managed queue.

    A request may only be forwarded from a queues dispatch routine.

    If the request is successfully forwarded to the DestinationQueue, it
    is no longer owned by the driver, but by the DestinationQueue.

    Both the source queue and destination queue should be part of the
    same device.

    The driver gets ownership of the request when it receives it
    from the DestinationQueue through EvtIo callback, or WdfIoQueueRetrieveNextRequest.

Arguments:


    Request - Request object to forward.

    DestinationQueue - Queue that is to receive the request.

Returns:

    STATUS_SUCCESS - Request was forwarded to Queue and driver no
                          longer owns it.

   !STATUS_SUCCESS - Request was not forwarded to the Queue, and
                          the driver still owns the request and is responsible
                          for either completing it, or eventually successfully
                          forwarding it to a Queue.
--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestRequeue)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request
   )

/*++

Routine Description:

    Requeue the request - only allowed if the queue is a manual queue.

Arguments:

    Request - Request to requeue

Returns:

    NTSTATUS

--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFEXPORT(WdfRequestStopAcknowledge)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   BOOLEAN Requeue
   )

/*++

Routine Description:

    The driver calls this to acknowledge that it is no longer
    attempting to perform I/O on the request which was provided in
    the EvtIoStop event callback notification.

    The device driver must arrange to no longer touch any hardware
    resources before making this call.

Arguments:

    Request - Request being stopped

    Requeue - True if the request is to be placed back on the front of the queue,
              and re-delivered to the device driver on resume.

Returns:

    None

--*/

{
    WDFNOTIMPLEMENTED();
}

__drv_maxIRQL(DISPATCH_LEVEL)
MdIrp
WDFAPI
WDFEXPORT(WdfRequestWdmGetIrp)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )

/*++

Routine Description:

    This routine returns the WDM IRP associated with the given
    request.

    The WDM IRP is invalid once WdfRequestComplete is called, regardless
    of any reference counts on the WDFREQUEST object.

Arguments:

    Request - Handle to the Request object

Returns:

    PIRP

--*/

{
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFEXPORT(WdfRequestIsReserved)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request
   )
/*++

Routine Description:
    This is used to determine if a Request is a reserved request. Reserved
    Requests are used for forward progress. 
    
Arguments:

    Request - Request being checked 

    
Returns:

    BOOLEAN

--*/

{
    WDFNOTIMPLEMENTED();
    return FALSE;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFEXPORT(WdfRequestForwardToParentDeviceIoQueue)(
   __in
   PWDF_DRIVER_GLOBALS DriverGlobals,
   __in
   WDFREQUEST Request,
   __in
   WDFQUEUE ParentDeviceQueue,
   __in
   PWDF_REQUEST_FORWARD_OPTIONS ForwardOptions
   )

/*++

Routine Description:

    Forward a request presented on one queue to parent Device queue.

    A request may only be forwarded from a queues dispatch routine.

    If the request is successfully forwarded to the ParentDeviceQueue, it
    is no longer owned by the driver, but by the ParentDeviceQueue.

    The driver gets ownership of the request when it receives it
    from the DestinationQueue through EvtIo callback, or WdfIoQueueRetrieveNextRequest.

Arguments:


    Request - Request object to forward.

    ParentDeviceQueue - Queue that is to receive the request.

    ForwardOptions - A pointer to a caller-allocated WDF_REQUEST_FORWARD_OPTIONS
        structure

Returns:

    STATUS_SUCCESS - Request was forwarded to Queue and driver no
                          longer owns it.

   !STATUS_SUCCESS - Request was not forwarded to the Queue, and
                          the driver still owns the request and is responsible
                          for either completing it, or eventually successfully
                          forwarding it to a Queue.
--*/

{
    WDFNOTIMPLEMENTED();
    return STATUS_NOT_IMPLEMENTED;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfRequestGetParameters)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    PWDF_REQUEST_PARAMETERS Parameters
    )
{
    WDFNOTIMPLEMENTED();
}

} // extern "C"
