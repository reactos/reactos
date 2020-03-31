#include "common/fxglobals.h"
#include "common/fxrequest.h"
#include "common/ifxmemory.h"
#include "common/fxioqueue.h"

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
    WDFNOTIMPLEMENTED();
    return NULL;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
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
    WDFNOTIMPLEMENTED();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
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
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
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
    WDFNOTIMPLEMENTED();
    return STATUS_UNSUCCESSFUL;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
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

} // extern "C"
