/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestApi.cpp

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
// #include "FxRequestApi.tmh"

//
// Verifiers
//
// Do not supply Argument names
FX_DECLARE_VF_FUNCTION_P1(
NTSTATUS,
VerifyRequestComplete,
    _In_ FxRequest*
    );

// Do not supply Argument names
FX_DECLARE_VF_FUNCTION_P1(
NTSTATUS,
VerifyWdfRequestIsCanceled,
    _In_ FxRequest*
    );

//Do not supply argument names
FX_DECLARE_VF_FUNCTION_P1(
NTSTATUS,
VerifyWdfRequestForwardToIoQueue,
    _In_ FxRequest*
    );

//Do not supply argument names
FX_DECLARE_VF_FUNCTION_P1(
NTSTATUS,
VerifyWdfRequestForwardToParentDeviceIoQueue,
    _In_ FxRequest*
    );

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest* pRequest;
    FxIoTarget* pTarget;
    NTSTATUS status;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    if (Target != NULL) {
        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       Target,
                                       FX_TYPE_IO_TARGET,
                                       (PVOID*)&pTarget,
                                       &pFxDriverGlobals);
    }
    else {
        pTarget = NULL;

        //
        // For class extension support, get globals from parent object.
        //
        if (RequestAttributes != NULL &&
            RequestAttributes->ParentObject != NULL) {

            FxObjectHandleGetGlobals(
                pFxDriverGlobals,
                RequestAttributes->ParentObject,
                &pFxDriverGlobals);
        }
    }

    FxPointerNotNull(pFxDriverGlobals, Request);
    *Request = NULL;

    status = FxRequest::_Create(pFxDriverGlobals,
                                RequestAttributes,
                                NULL,
                                pTarget,
                                FxRequestOwnsIrp,
                                FxRequestConstructorCallerIsDriver,
                                &pRequest);

    if (NT_SUCCESS(status)) {
        *Request = pRequest->GetHandle();

#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "Created WDFREQUEST %p, %!STATUS!",
            *Request, status);
#endif // FX_VERBOSE_TRACE
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
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
    FxRequest* pRequest;
    NTSTATUS status;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // For class extension support, get globals from parent object.
    //
    if (RequestAttributes != NULL &&
        RequestAttributes->ParentObject != NULL) {

        FxObjectHandleGetGlobals(
            pFxDriverGlobals,
            RequestAttributes->ParentObject,
            &pFxDriverGlobals);
    }

    FxPointerNotNull(pFxDriverGlobals, Irp);
    FxPointerNotNull(pFxDriverGlobals, Request);

    *Request = NULL;

    status = FxRequest::_Create(pFxDriverGlobals,
                                RequestAttributes,
                                Irp,
                                NULL,
                                RequestFreesIrp ? FxRequestOwnsIrp
                                                : FxRequestDoesNotOwnIrp,
                                FxRequestConstructorCallerIsDriver,
                                &pRequest);

    if (NT_SUCCESS(status)) {
        *Request = pRequest->GetHandle();

#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "Irp %p RequestFreesIrp %d WDFREQUEST %p created",
            Irp, RequestFreesIrp, *Request);
#endif // FX_VERBOSE_TRACE
    }

    return status;
}


#define WDF_REQUEST_REUSE_VALID_FLAGS_V1_9 \
    (WDF_REQUEST_REUSE_SET_NEW_IRP)

#define WDF_REQUEST_REUSE_VALID_FLAGS \
    (WDF_REQUEST_REUSE_SET_NEW_IRP | WDF_REQUEST_REUSE_MUST_COMPLETE)







__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
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
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;
    FxRequest           *pRequest;
    ULONG               validFlags;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, ReuseParams);

    if (ReuseParams->Size != sizeof(WDF_REQUEST_REUSE_PARAMS)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "ReuseParams Size %d, expected %d %!STATUS!",
                            ReuseParams->Size, sizeof(WDF_REQUEST_REUSE_PARAMS),
                            STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11)) {
        validFlags = WDF_REQUEST_REUSE_VALID_FLAGS;
    }
    else {
        validFlags = WDF_REQUEST_REUSE_VALID_FLAGS_V1_9;
    }

    if (ReuseParams->Flags & ~validFlags) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "ReuseParams Flags 0x%x, valid mask 0x%x, %!STATUS!",
                            ReuseParams->Flags,
                            (ULONG) ~validFlags,
                            STATUS_INVALID_PARAMETER);
        return STATUS_INVALID_PARAMETER;
    }

    return pRequest->Reuse(ReuseParams);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfRequestChangeTarget)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __in
    WDFIOTARGET IoTarget
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest* pRequest;
    FxIoTarget* pTarget;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         IoTarget,
                         FX_TYPE_IO_TARGET,
                         (PVOID*)&pTarget);

    return pRequest->ValidateTarget(pTarget);
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
    if (NT_SUCCESS(status)) {
        status = pRequest->VerifyRequestCanBeCompleted(FxDriverGlobals);
    }

    pRequest->Unlock(irql);
    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
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
    if (!NT_SUCCESS(status)) {
        return;
    }

    pRequest->Complete(RequestStatus);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
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
    // Use the object's globals, not the caller's
    //
    DoTraceLevelMessage(pRequest->GetDriverGlobals(),
                        TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Completing WDFREQUEST 0x%p, %!STATUS!",
                        Request, RequestStatus);
#endif
    status = VerifyRequestComplete(pRequest->GetDriverGlobals(),
                                   pRequest);
    if (!NT_SUCCESS(status)) {
        return;
    }

    pRequest->CompleteWithPriority(RequestStatus, PriorityBoost);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
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
    if (!NT_SUCCESS(status)) {
        return;
    }


    pRequest->CompleteWithInformation(RequestStatus, Information);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
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
ULONG_PTR
WDFAPI
STDCALL
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
    FxRequest *pRequest;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*)&pRequest);

    return pRequest->GetInformation();
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    if (pRequest->GetFxIrp()->GetMajorFunction() == IRP_MJ_READ) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on the Read request, you should call"
            " WdfRequestRetrieveOutputMemory to get the Memory for WDFREQUEST "
            "0x%p, %!STATUS!", Request, status);

        return status;
    }

    status = pRequest->GetMemoryObject(&pMemory, &pBuffer, &length);
    if (NT_SUCCESS(status)) {
        *Memory = pMemory->GetHandle();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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

    if (majorFunction == IRP_MJ_WRITE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on the Write request, you should call"
            " WdfRequestRetrieveInputMemory to get the Memory for WDFREQUEST "
            "0x%p, %!STATUS!", Request, status);

        return status;
    }

    if( (majorFunction == IRP_MJ_DEVICE_CONTROL) ||
        (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) ) {
        status = pRequest->GetDeviceControlOutputMemoryObject(&pMemory, &pBuffer, &length);
    }
    else {
        status = pRequest->GetMemoryObject(&pMemory, &pBuffer, &length);
    }

    if (NT_SUCCESS(status)) {
        *Memory = pMemory->GetHandle();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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

    //
    // Validate the pointers and set its content to NULL
    //
    FxPointerNotNull(pFxDriverGlobals, Buffer);
    *Buffer = NULL;

    if (Length != NULL) {
        *Length  = 0;
    }

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif //FX_VERBOSE_TRACE

    //
    // This call is not valid on Read request.
    //
    if (pRequest->GetFxIrp()->GetMajorFunction() == IRP_MJ_READ) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on the read request, you should call"
            " WdfRequestRetrieveOutputBuffer to get the buffer for WDFREQUEST "
            "0x%p, %!STATUS!", Request, status);

        return status;
    }

    status = pRequest->GetMemoryObject(&pMemory, &pBuffer, &length);

    if (NT_SUCCESS(status)) {
        if (length < RequiredLength) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                "WDFREQUEST 0x%p buffer size %I64d is less than RequiredLength "
                "%I64d, %!STATUS!", Request, length, RequiredLength, status);

            return status;
        }

        *Buffer = pBuffer;

        if (Length != NULL) {
            *Length = length;
        }
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    IFxMemory* pMemory;
    PVOID pBuffer;
    size_t length;
    UCHAR   majorFunction;

    pMemory = NULL;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    //
    // Validate the pointers and set its content to NULL
    //
    FxPointerNotNull(pFxDriverGlobals, Buffer);
    *Buffer = NULL;

    if (Length != NULL) {
        *Length  = 0;
    }

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif //FX_VERBOSE_TRACE

    //
    // This call is not valid on Write request.
    //
    majorFunction = pRequest->GetFxIrp()->GetMajorFunction();

    if (majorFunction == IRP_MJ_WRITE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on write request, you should call"
            " WdfRequestRetrieveInputBuffer to get the buffer for WDFREQUEST "
            "0x%p, %!STATUS!", Request, status);

        return status;
    }

    if (majorFunction == IRP_MJ_DEVICE_CONTROL ||
        majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) {
        status = pRequest->GetDeviceControlOutputMemoryObject(
            &pMemory, &pBuffer, &length);
    }
    else {
        status = pRequest->GetMemoryObject(&pMemory, &pBuffer, &length);
    }

    if (NT_SUCCESS(status)) {
        if (length < RequiredLength) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                "WDFREQUEST 0x%p buffer size %I64d is less than RequiredLength "
                "%I64d, %!STATUS!", Request, length, RequiredLength, status);

            return status;
        }

        *Buffer = pBuffer;

        if (Length != NULL) {
            *Length = length;
        }
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    UCHAR   majorFunction;
    FxDevice* pDevice;

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
                        "Enter:  WDREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    FxPointerNotNull(pFxDriverGlobals, InputBuffer);
    *InputBuffer = NULL;

    if (Length != NULL) {
        *Length = 0;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Make sure this function is called in the context of in-process callback
    //
    if (pFxDriverGlobals->FxVerifierOn) {
        KIRQL irql;

        pRequest->Lock(&irql);

        status = pRequest->VerifyRequestIsInCallerContext(pFxDriverGlobals);

        pRequest->Unlock(irql);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    FxIrp* irp = pRequest->GetFxIrp();

    pDevice = FxDevice::GetFxDevice(irp->GetDeviceObject());

    //
    // This call is not valid on Read request.
    //
    majorFunction = irp->GetMajorFunction();

    if (majorFunction == IRP_MJ_READ) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on read request, you should call"
            " WdfRequestRetrieveUnsafeUserOutputBuffer to get the buffer for "
            "WDFREQUEST 0x%p, %!STATUS!", Request, status);

        return status;
    }

    if (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on internal-ioctl request, you should call"
            " safer WdfRequestRetrieveInputBuffer to get the buffer for "
            "WDFREQUEST 0x%p, %!STATUS!", Request, status);

        return status;
    }

    if (majorFunction == IRP_MJ_DEVICE_CONTROL &&
        irp->GetParameterIoctlCodeBufferMethod() == METHOD_NEITHER) {

        if (irp->GetParameterIoctlInputBufferLength() < RequiredLength) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                "WDFREQUEST %p buffer size %d is less than RequiredLength %I64d,"
                " %!STATUS!",
                Request, irp->GetParameterIoctlInputBufferLength(),
                RequiredLength, status);

            return status;
        }

        *InputBuffer = irp->GetParameterIoctlType3InputBuffer();
        if (Length != NULL) {
            *Length = irp->GetParameterIoctlInputBufferLength();
        }

        return STATUS_SUCCESS;

    }
    else if (majorFunction == IRP_MJ_WRITE &&
             pDevice->GetIoType() == WdfDeviceIoNeither) {

        if (irp->GetParameterWriteLength() < RequiredLength) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                "WDFREQUEST 0x%p buffer size %d is less than RequiredLength "
                "%I64d, %!STATUS!",
                Request, irp->GetParameterWriteLength(), RequiredLength,
                status);

            return status;
        }

        *InputBuffer = pRequest->GetFxIrp()->GetUserBuffer();
        if (Length != NULL) {
            *Length = irp->GetParameterWriteLength();
        }

        return STATUS_SUCCESS;

    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "Error: This call is valid only on method-neither "
                            "ioctl and write WDFREQUEST %p, %!STATUS!",
                            Request, status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return status;
    }

    // NOTREACHED
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    UCHAR   majorFunction;
    FxDevice* pDevice;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, OutputBuffer);
    *OutputBuffer  = NULL;

    if (Length != NULL) {
        *Length = 0;
    }

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Make sure this function is called in the context of in-process callback
    //
    if (pFxDriverGlobals->FxVerifierOn) {
        KIRQL irql;

        pRequest->Lock(&irql);

        status = pRequest->VerifyRequestIsInCallerContext(pFxDriverGlobals);

        pRequest->Unlock(irql);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", pRequest);
#endif // FX_VERBOSE_TRACE

    FxIrp* irp = pRequest->GetFxIrp();
    pDevice = FxDevice::GetFxDevice(irp->GetDeviceObject());

    //
    // This call is not valid on Write request.
    //
    majorFunction = irp->GetMajorFunction();

    if (majorFunction == IRP_MJ_WRITE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on Write request, you should call"
            " WdfRequestRetrieveUnsafeUserInputBuffer to get the buffer for "
            "WDFREQUEST 0x%p, %!STATUS!", Request, status);

        return status;

    }

    if (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on an internal IOCTL request, you should call"
            " safer WdfRequestRetrieveOutputBuffer to get the buffer for "
            "WDFREQUEST 0x%p, %!STATUS!", Request, status);

        return status;
    }

    if (majorFunction == IRP_MJ_DEVICE_CONTROL &&
        irp->GetParameterIoctlCodeBufferMethod() == METHOD_NEITHER) {

        if (irp->GetParameterIoctlOutputBufferLength() < RequiredLength) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                "WDFREQUEST 0x%p buffer size %d is less than RequiredLength "
                "%I64d, %!STATUS!",
                Request, irp->GetParameterIoctlOutputBufferLength(),
                RequiredLength, status);

            return status;
        }

        *OutputBuffer = pRequest->GetFxIrp()->GetUserBuffer();

        if (Length != NULL) {
            *Length = irp->GetParameterIoctlOutputBufferLength();
        }

        return STATUS_SUCCESS;

    } else if (majorFunction == IRP_MJ_READ &&
               pDevice->GetIoType() == WdfDeviceIoNeither) {

        if (irp->GetParameterReadLength() < RequiredLength) {
            status = STATUS_BUFFER_TOO_SMALL;

            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                "WDFREQUEST 0x%p buffer size %d is less than RequiredLength "
                "%I64d, %!STATUS!",
                Request, irp->GetParameterReadLength(), RequiredLength,
                status);

            return status;
        }

        *OutputBuffer = pRequest->GetFxIrp()->GetUserBuffer();
        if (Length != NULL) {
            *Length = irp->GetParameterReadLength();
        }

        return STATUS_SUCCESS;

    } else {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Error: This call is valid only on method-neither ioctl and read "
            "WDFREQUEST 0x%p, %!STATUS!", Request, status);

        return status;
    }

    // NOTREACHED
}


_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Mdl);
    *Mdl = NULL;

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    //
    // This call is not valid on Read request.
    //
    if (pRequest->GetFxIrp()->GetMajorFunction() == IRP_MJ_READ) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on the Read request, you should call"
            " WdfRequestRetrieveOutputMdl to get the Mdl for WFDREQUEST 0x%p, "
            " %!STATUS!", Request, STATUS_INVALID_DEVICE_REQUEST);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    status = pRequest->GetMdl(Mdl);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    UCHAR majorFunction;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Mdl);
    *Mdl = NULL;

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    //
    // This call is not valid on Write request.
    //
    majorFunction = pRequest->GetFxIrp()->GetMajorFunction();
    if (majorFunction == IRP_MJ_WRITE) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
            "This call is not valid on the Write request, you should call"
            " WdfRequestRetrieveInputMemory to get the Memory for WDFREQUEST 0x%p, "
            "%!STATUS!",Request, STATUS_INVALID_DEVICE_REQUEST);

        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if( (majorFunction == IRP_MJ_DEVICE_CONTROL) ||
        (majorFunction == IRP_MJ_INTERNAL_DEVICE_CONTROL) ) {
        status = pRequest->GetDeviceControlOutputMdl(Mdl);
    }
    else {
        status = pRequest->GetMdl(Mdl);
    }

    return status;
}

void
CheckUnionAssumptions(
    VOID
    )
/*++

Routine Description:
    Make sure our assumptions about using the passed in parameters as locals
    does not exceed the space allocated on the stack for the passed in variables,
    otherwise we could corrupt the stack.

    *DO NOT REMOVE* this function even though no code calls it.  Because it uses
    WDFCASSERT, if our assumptions were false, it would not compile.

Arguments:
    None.

Return Value:
    None

  --*/
{
    // ActionUnion check
    WDFCASSERT(sizeof(ULONG) <= sizeof(PWDF_DRIVER_GLOBALS));
    // RequestUnion check
    WDFCASSERT(sizeof(FxRequest*) <= sizeof(WDFREQUEST));
    // TargetUnion check
    WDFCASSERT(sizeof(FxIoTarget*) <= sizeof(WDFIOTARGET));
}

#define GLOBALS_ACTION(globals)         ((ULONG)(ULONG_PTR)(globals))
#define PGLOBALS_ACTION(globals)        ((PULONG)(PULONG_PTR)(globals))

#define GLOBALS_DEVICE(globals)         ((FxDevice*)(ULONG_PTR)(globals))
#define PGLOBALS_DEVICE(globals)        ((FxDevice**)(PULONG_PTR)(globals))

#define WDFREQUEST_FXREQUEST(handle)    ((FxRequest*)(handle))
#define WDFIOTARGET_FXIOTARGET(handle)  ((FxIoTarget*)(handle))

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
STDCALL
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
    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &Request);

    //
    // Request stack memory now holds an FxRequest pointer.
    // Request as a handle is no longer valid!
    //
    if (!NT_SUCCESS(FxValidateRequestOptions(
                        WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals(),
                        Options, WDFREQUEST_FXREQUEST(Request)))) {

        WDFREQUEST_FXREQUEST(Request)->SetStatus(STATUS_INVALID_PARAMETER);

        FxVerifierDbgBreakPoint(WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals());
        return FALSE;
    }

    FxObjectHandleGetPtr(WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals(),
                        Target,
                        FX_TYPE_IO_TARGET,
                        (PVOID*) &Target);

    //
    // Target stack memory now hold an FxIoTarget pointer.
    // Target as a handle is no longer valid!
    //
    if (Options != NULL &&
        (Options->Flags & (WDF_REQUEST_SEND_OPTION_SYNCHRONOUS |
                           WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET)) != 0x0) {

        if (Options->Flags & WDF_REQUEST_SEND_OPTION_SYNCHRONOUS) {
            //
            // This sets impersonation flags for UMDF. Noop for KMDF.
            //
            WDFREQUEST_FXREQUEST(Request)->SetImpersonationFlags(Options->Flags);

            *PGLOBALS_ACTION(&DriverGlobals) = SubmitSyncCallCompletion;
            (void) WDFIOTARGET_FXIOTARGET(Target)->SubmitSync(
                WDFREQUEST_FXREQUEST(Request),
                Options,
                PGLOBALS_ACTION(&DriverGlobals)
                );
        }
        else if (Options->Flags & WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET) {
            if (WDFREQUEST_FXREQUEST(Request)->IsAllocatedFromIo() == FALSE) {
                DoTraceLevelMessage(
                    WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals(),
                    TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "WDFREQUEST %p must be a WDFQUEUE presented request",
                    WDFREQUEST_FXREQUEST(Request)->GetHandle());

                WDFREQUEST_FXREQUEST(Request)->SetStatus(
                    STATUS_INVALID_DEVICE_STATE
                    );

                *PGLOBALS_ACTION(&DriverGlobals) = 0;
            }
            else if (WDFREQUEST_FXREQUEST(Request)->HasContext()) {
                //
                // Cannot send-and-forget a request with formatted IO context.
                //
                DoTraceLevelMessage(
                    WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals(),
                    TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Cannot send-and-forget WDFREQUEST 0x%p with formatted IO"
                    " context, %!STATUS!",
                    WDFREQUEST_FXREQUEST(Request)->GetHandle(),
                    STATUS_INVALID_DEVICE_REQUEST );

                WDFREQUEST_FXREQUEST(Request)->SetStatus(
                    STATUS_INVALID_DEVICE_REQUEST
                    );

                *PGLOBALS_ACTION(&DriverGlobals) = 0;

                FxVerifierDbgBreakPoint(
                        WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals());
            }
            else {
                //
                // We split processing into pre and post processing to reduce
                // stack usage (by not making the call to IoCallDriver in a
                // deep function.
                //

                //
                // This will skip the current stack location
                //
                WDFREQUEST_FXREQUEST(Request)->PreProcessSendAndForget();

                //
                // This sets impersonation flags for UMDF. Noop for KMDF.
                //
                WDFREQUEST_FXREQUEST(Request)->SetImpersonationFlags(Options->Flags);

                MdIrp submitIrp = WDFREQUEST_FXREQUEST(Request)->GetSubmitIrp();

                WDFIOTARGET_FXIOTARGET(Target)->Send(submitIrp);

                //
                // This will free the request memory and pop the queue
                //
                WDFREQUEST_FXREQUEST(Request)->PostProcessSendAndForget();
                return TRUE;
            }
        }
    }
    else if (WDFREQUEST_FXREQUEST(Request)->IsCompletionRoutineSet() == FALSE &&
             WDFREQUEST_FXREQUEST(Request)->IsAllocatedFromIo()) {
            //
            // Cannot send an asynchronous queue presented request without a
            // completion routine.
            //
            DoTraceLevelMessage(
                WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals(),
                TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFREQUEST %p is a WDFQUEUE presented request with no"
                " completion routine, %!STATUS!",
                WDFREQUEST_FXREQUEST(Request)->GetHandle(),
                STATUS_INVALID_DEVICE_REQUEST );

            WDFREQUEST_FXREQUEST(Request)->SetStatus(
                    STATUS_INVALID_DEVICE_REQUEST
                    );

            *PGLOBALS_ACTION(&DriverGlobals) = 0;

            FxVerifierDbgBreakPoint(
                    WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals());
    }
    else {
#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(
            WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals(),
            TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "Enter: WDFIOTARGET %p, WDFREQUEST %p",
            WDFIOTARGET_FXIOTARGET(Target)->GetObjectHandle(),
            WDFREQUEST_FXREQUEST(Request));
#endif // FX_VERBOSE_TRACE

        //
        // This sets impersonation flags for UMDF. Noop for KMDF.
        //
        if (Options != NULL) {
            WDFREQUEST_FXREQUEST(Request)->SetImpersonationFlags(Options->Flags);
        }

        //
        // Submit will return whether the request should be sent *right now*.
        // If SubmitSend is clear, then SubmitQueued must be checked.  If set,
        // then the request was queued, otherwise, the request has failed.
        //
        // NOTE:  by calling FxIoTarget::Submit instead of acquiring the lock
        //        in this call frame, we don't have expend stack space for the KIRQL
        //        storage
        //
        *PGLOBALS_ACTION(&DriverGlobals) =
            WDFIOTARGET_FXIOTARGET(Target)->Submit(
                WDFREQUEST_FXREQUEST(Request),
                Options,
                (Options != NULL) ? Options->Flags : 0
                );

        // DriverGlobals stack memory now hold a ULONG action value.
        // DriverGlobals as a pointer is no longer valid!

#if FX_VERBOSE_TRACE
        DoTraceLevelMessage(
            WDFIOTARGET_FXIOTARGET(Target)->GetDriverGlobals(),
            TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "action 0x%x", GLOBALS_ACTION(DriverGlobals));
#endif // FX_VERBOSE_TRACE

        if (GLOBALS_ACTION(DriverGlobals) & SubmitSend) {

            *PGLOBALS_ACTION(&DriverGlobals) |= SubmitSent;

            ASSERT((GLOBALS_ACTION(DriverGlobals) & SubmitQueued) == 0);

#if FX_VERBOSE_TRACE
            DoTraceLevelMessage(
                WDFREQUEST_FXREQUEST(Request)->GetDriverGlobals(),
                TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                "Sending FxRequest %p (WDFREQUEST %p), Irp %p",
                WDFREQUEST_FXREQUEST(Request),
                WDFREQUEST_FXREQUEST(Request)->GetHandle(),
                WDFREQUEST_FXREQUEST(Request)->GetSubmitIrp());
#endif // FX_VERBOSE_TRACE

            MdIrp submitIrp = WDFREQUEST_FXREQUEST(Request)->GetSubmitIrp();

            WDFIOTARGET_FXIOTARGET(Target)->Send(submitIrp);
        }
        else if (GLOBALS_ACTION(DriverGlobals) & SubmitQueued) {
            //
            // To the caller, we saw and sent the request (and all the cancel
            // semantics of a sent request still work).
            //
            *(PGLOBALS_ACTION(&DriverGlobals)) |= SubmitSent;
        }
    }

    return (GLOBALS_ACTION(DriverGlobals) & SubmitSent) ? TRUE : FALSE;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfRequestGetStatus)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request
    )
{
    FxRequest* pRequest;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*)&pRequest);

    return pRequest->GetStatus();
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest *pRequest;
    BOOLEAN result;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST %p to be cancelled", Request);

    result = pRequest->Cancel();

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Exit: WDFREQUEST %p, result %d", Request, result);
#endif // FX_VERBOSE_TRACE

    return result;
}

_Must_inspect_result_
__drv_maxIRQL(APC_LEVEL)
BOOLEAN
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest* pRequest;
    BOOLEAN result;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*) &pRequest,
                                   &pFxDriverGlobals);

    result = pRequest->GetFxIrp()->Is32bitProcess();

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST %p is from 32 bit process = %d",
                        Request, result);
#endif // FX_VERBOSE_TRACE

    return result;
}

__drv_maxIRQL(DISPATCH_LEVEL)

VOID
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest *pRequest;
    FxIrp* irp;

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
                        "Enter: WDFREQUEST %p", Request);
#endif // FX_VERBOSE_TRACE

    irp = pRequest->GetSubmitFxIrp();

    if (irp->GetIrp() == NULL) {
        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_REQUEST_FATAL_ERROR,
                           WDF_REQUEST_FATAL_ERROR_NULL_IRP,
                           (ULONG_PTR) Request);
        return; // not reached
    }

    //
    // 1 is the minimum for CurrentLocation.  Since the next stack location is
    // CurrentLocation-1, the CurrentLocation must be at least 2.
    //
    if (irp->HasStack(2) == FALSE) {
        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_REQUEST_FATAL_ERROR,
                           WDF_REQUEST_FATAL_ERROR_NO_MORE_STACK_LOCATIONS,
                           (ULONG_PTR) irp->GetIrp());
        return; // not reached
    }

    pRequest->m_NextStackLocationFormatted = TRUE;
    irp->CopyCurrentIrpStackLocationToNext();

    pRequest->VerifierSetFormatted();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest *pRequest;
    FxIrp* pIrp;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Stack);

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST %p", Request);
#endif // FX_VERBOSE_TRACE

    pIrp = pRequest->GetSubmitFxIrp();

    if (pIrp == NULL) {
        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_REQUEST_FATAL_ERROR,
                           WDF_REQUEST_FATAL_ERROR_NULL_IRP,
                           (ULONG_PTR) Request);
        return; // not reached
    }

    //
    // 1 is the minimum for CurrentLocation.  Since the next stack location is
    // CurrentLocation-1, the CurrentLocation must be at least 2.
    //
    if (pIrp->GetCurrentIrpStackLocationIndex() < 2) {
        FxVerifierBugCheck(pFxDriverGlobals,
                           WDF_REQUEST_FATAL_ERROR,
                           WDF_REQUEST_FATAL_ERROR_NO_MORE_STACK_LOCATIONS,
                           (ULONG_PTR) pIrp);
        return; // not reached
    }

    pRequest->m_NextStackLocationFormatted = TRUE;
    pIrp->CopyToNextIrpStackLocation(Stack);

    pRequest->VerifierSetFormatted();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
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
    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
        "Enter: WDFREQUEST %p, Routine %p, Context %p",
        Request, CompletionRoutine, CompletionContext);
#endif // FX_VERBOSE_TRACE

    pRequest->SetCompletionRoutine(CompletionRoutine, CompletionContext);

    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
WDFEXPORT(WdfRequestGetParameters)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    PWDF_REQUEST_PARAMETERS Parameters
    )
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

    FxPointerNotNull(pFxDriverGlobals, Parameters);

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: Request %p, Parameters %p", Request, Parameters);
#endif // FX_VERBOSE_TRACE

    if (Parameters->Size != sizeof(WDF_REQUEST_PARAMETERS)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "Params size %d incorrect, expected %d",
                            Parameters->Size, sizeof(WDF_REQUEST_PARAMETERS));

        FxVerifierDbgBreakPoint(pFxDriverGlobals);

        return;
    }




    (VOID) pRequest->GetParameters(Parameters);

    return;
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
STDCALL
WDFEXPORT(WdfRequestGetCompletionParams)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFREQUEST Request,
    __out
    PWDF_REQUEST_COMPLETION_PARAMS Params
    )
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
                        "Enter: WDFREQUEST %p, Params %p", Request, Params);
#endif // FX_VERBOSE_TRACE

    FxPointerNotNull(pFxDriverGlobals, Params);

    if (Params->Size != sizeof(WDF_REQUEST_COMPLETION_PARAMS)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "Params Size 0x%x, expected 0x%x",
            Params->Size, sizeof(WDF_REQUEST_COMPLETION_PARAMS));
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

     pRequest->CopyCompletionParams(Params);

     return;

}

__drv_maxIRQL(DISPATCH_LEVEL)
MdIrp
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    MdIrp irp;

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

    status = pRequest->GetIrp(&irp);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST is already completed 0x%p, %!STATUS!",
                            Request, status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return NULL;
    }

    return irp;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    FxRequest* pRequest;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*)&pRequest);

    return pRequest->CreateTimer();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFFILEOBJECT
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    FxFileObject* pFO;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*) &pRequest,
                                   &pFxDriverGlobals);

    pFO = NULL;

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    if (pRequest->GetDriverGlobals()->IsVerificationEnabled(
            1,9, OkForDownLevel)) {
        KIRQL irql;

        pRequest->Lock(&irql);
        status = pRequest->VerifyRequestIsDriverOwned(pFxDriverGlobals);
        pRequest->Unlock(irql);
        if (!NT_SUCCESS(status)) {
            return NULL;
        }
    }

    status = pRequest->GetFileObject(&pFO);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "GetFileobject failed with %!STATUS!", status);
        return NULL;
    }
    else if (NULL == pFO) {
        //
        // Success and NULL file object: driver during init told us that it
        // knows how to handle NULL file objects.
        //
        return NULL;
    }

    return pFO->GetHandle();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    FxRequestMemory* pMemory;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Buffer);
    FxPointerNotNull(pFxDriverGlobals, MemoryObject);
    *MemoryObject = NULL;

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    if (pRequest->GetDriverGlobals()->IsVerificationEnabled(
            1,9, OkForDownLevel)) {
        KIRQL irql;

        pRequest->Lock(&irql);
        status = pRequest->VerifyRequestIsDriverOwned(pFxDriverGlobals);
        pRequest->Unlock(irql);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = pRequest->ProbeAndLockForRead(Buffer, (ULONG) Length, &pMemory);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "ProbeAndLockForRead failed with %!STATUS!", status);
        return status;
    }

    *MemoryObject = (WDFMEMORY) pMemory->GetObjectHandle();

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
STDCALL
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
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    NTSTATUS status;
    FxRequest *pRequest;
    FxRequestMemory* pMemory;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Buffer);
    FxPointerNotNull(pFxDriverGlobals, MemoryObject);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    if (pRequest->GetDriverGlobals()->IsVerificationEnabled(
            1,9, OkForDownLevel)) {
        KIRQL irql;

        pRequest->Lock(&irql);
        status = pRequest->VerifyRequestIsDriverOwned(pFxDriverGlobals);
        pRequest->Unlock(irql);
        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    status = pRequest->ProbeAndLockForWrite(Buffer, (ULONG) Length, &pMemory);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
            "ProbeAndLockForWrite failed with %!STATUS!", status);
        return status;
    }

    *MemoryObject = (WDFMEMORY)pMemory->GetObjectHandle();

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
KPROCESSOR_MODE
WDFAPI
STDCALL
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
    FxRequest *pRequest;

    //
    // Validate the request handle, and get the FxRequest*
    //
    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*)&pRequest);

    return pRequest->GetRequestorMode();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFQUEUE
WDFAPI
STDCALL
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

    if (pRequest->GetCurrentQueue() == NULL) {
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

    if (pRequest->GetFxIrp()->GetMajorFunction() == IRP_MJ_CREATE) {
        //
        // If the queue for Create is the framework internal queue
        // return NULL.
        //
        FxPkgGeneral* devicePkgGeneral = pRequest->GetDevice()->m_PkgGeneral;

        if (devicePkgGeneral->GetDeafultInternalCreateQueue() ==
                pRequest->GetCurrentQueue()) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                                "Getting queue handle for Create request is "
                                "not allowed for WDFREQUEST 0x%p", pRequest);
            FxVerifierDbgBreakPoint(pFxDriverGlobals);
            return  NULL;
        }
    }

    return (WDFQUEUE) pRequest->GetCurrentQueue()->GetObjectHandle();
}

_Must_inspect_result_
NTSTATUS
FX_VF_FUNCTION(VerifyWdfRequestForwardToIoQueue) (
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* request
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    PAGED_CODE_LOCKED();

    //
    // * Is request I/O allocated but without a queue? This should not happen.
    // * Is WDF driver v1.9 or below trying to use this feature? We don't allow it.
    //
    if (request->IsAllocatedDriver() == FALSE ||
        FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11) == FALSE) {
        status =  STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST %p doesn't belong to any queue, %!STATUS!",
                    request->GetHandle(), status);
        FxVerifierDbgBreakPoint(FxDriverGlobals);
        return status;
    }

    //
    // Make sure current IRP stack location is valid. See helper routine for error msgs.
    //
    status = request->VerifyRequestCanBeCompleted(FxDriverGlobals);
    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
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
    PFX_DRIVER_GLOBALS  fxDriverGlobals;
    PFX_DRIVER_GLOBALS  cxDriverGlobals;
    FxRequest*          request;
    FxIoQueue*          queue;
    NTSTATUS            status;

    cxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    //
    // Validate destination queue handle
    //
    FxObjectHandleGetPtrAndGlobals(cxDriverGlobals,
                                   DestinationQueue,
                                   FX_TYPE_QUEUE,
                                   (PVOID*)&queue,
                                   &fxDriverGlobals);

    //
    // Validate request object handle
    //
    FxObjectHandleGetPtr(fxDriverGlobals,
                          Request,
                          FX_TYPE_REQUEST,
                          (PVOID*)&request);

    //
    // If present, let the queue do the heavy lifting.
    //
    if (request->GetCurrentQueue() != NULL) {
        status = request->GetCurrentQueue()->ForwardRequest(queue, request);
        goto Done;
    }

    //
    // Basic verification.
    //
    status = VerifyWdfRequestForwardToIoQueue(fxDriverGlobals, request);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // OK, queue this request.
    //
    status = queue->QueueDriverCreatedRequest(request, FALSE);

Done:
    return status;
}

_Must_inspect_result_
NTSTATUS
FX_VF_FUNCTION(VerifyWdfRequestForwardToParentDeviceIoQueue) (
    _In_ PFX_DRIVER_GLOBALS fxDriverGlobals,
    _In_ FxRequest* request
    )
{
    NTSTATUS status;
    FxIrp* irp;

    PAGED_CODE_LOCKED();

    //
    // * Is request I/O allocated but without a queue? This should not happen.
    // * Is WDF driver v1.9 or below trying to use this feature? We don't allow it.
    //
    if (request->IsAllocatedDriver() == FALSE ||
        fxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,11) == FALSE) {
        status =  STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                    "WDFREQUEST %p doesn't belong to any queue, %!STATUS!",
                    request->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

    //
    // Make sure current IRP stack location is valid.
    //
    status = request->VerifyRequestCanBeCompleted(fxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Make sure IRP has space for at least another stack location.
    //
    irp = request->GetFxIrp();

    ASSERT(irp->GetIrp() != NULL);

    if (irp->GetCurrentIrpStackLocationIndex() <= 1) {
        status =  STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "IRP %p of WDFREQUEST %p doesn't enough stack "
                            "locations, %!STATUS!",
                            irp, request->GetHandle(), status);
        FxVerifierDbgBreakPoint(fxDriverGlobals);
        goto Done;
    }

Done:
    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
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
    PFX_DRIVER_GLOBALS  fxDriverGlobals;
    NTSTATUS            status;
    FxRequest*          request;
    FxIoQueue*          queue;

    //
    // Validate destination queue handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   ParentDeviceQueue,
                                   FX_TYPE_QUEUE,
                                   (PVOID*)&queue,
                                   &fxDriverGlobals);

    //
    // Validate request object handle
    //
    FxObjectHandleGetPtr(fxDriverGlobals,
                          Request,
                          FX_TYPE_REQUEST,
                          (PVOID*)&request);
    FxPointerNotNull(fxDriverGlobals, ForwardOptions);

    if (ForwardOptions->Size != sizeof(WDF_REQUEST_FORWARD_OPTIONS)) {
        //
        // Size is wrong, bale out
        //
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "ForwardOptions %p Size incorrect, expected %d, "
                            "got %d, %!STATUS!",
                            ForwardOptions, sizeof(WDF_REQUEST_FORWARD_OPTIONS),
                            ForwardOptions->Size,
                            status);

        goto Done;
    }

    if ((ForwardOptions->Flags & ~WDF_REQUEST_FORWARD_OPTION_SEND_AND_FORGET) != 0) {
        //
        // Invalid flag
        //
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(fxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGAPIERROR,
                            "ForwardOptions %p Flags 0x%x invalid, "
                            "valid mask is 0x%x, %!STATUS!",
                            ForwardOptions, ForwardOptions->Flags,
                            WDF_REQUEST_FORWARD_OPTION_SEND_AND_FORGET,
                            status);

        goto Done;
    }

    //
    // If present, let the queue do the heavy lifting.
    //
    if (request->GetCurrentQueue() != NULL) {
        status = request->GetCurrentQueue()->ForwardRequestToParent(
                                                            queue,
                                                            request,
                                                            ForwardOptions);
        goto Done;
    }

    //
    // Basic verification.
    //
    status = VerifyWdfRequestForwardToParentDeviceIoQueue(fxDriverGlobals,
                                                          request);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // OK, queue this request.
    //
    status = queue->QueueDriverCreatedRequest(request, TRUE);

Done:
    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
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

    //
    // GetCurrentQueue may return NULL if the request is driver created request
    // or the if the call is made in the context of InProcessContextCallback.
    //
    if (pRequest->GetCurrentQueue() == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST %p doesn't belong to any queue %!STATUS!",
                            Request, STATUS_INVALID_DEVICE_REQUEST);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    return pRequest->GetCurrentQueue()->Requeue(pRequest);
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
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

    if (pRequest->GetCurrentQueue() == NULL) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST %p doesn't belong to any queue",
                            Request);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return;
    }

    status = pRequest->GetCurrentQueue()->RequestCancelable(pRequest,
                                                        TRUE,
                                                        EvtRequestCancel,
                                                        FALSE);
    UNREFERENCED_PARAMETER(status); //for fre build
    ASSERT(status == STATUS_SUCCESS);

}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
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

    if (pRequest->GetCurrentQueue() == NULL) {
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
STDCALL
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

    if (pRequest->GetCurrentQueue() == NULL) {
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
NTSTATUS
FX_VF_FUNCTION(VerifyWdfRequestIsCanceled)(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ FxRequest* pRequest
    )
{
    NTSTATUS status;
    KIRQL irql;

    PAGED_CODE_LOCKED();

    pRequest->Lock(&irql);

    status = pRequest->VerifyRequestIsDriverOwned(FxDriverGlobals);
    if (NT_SUCCESS(status)) {
        status = pRequest->VerifyRequestIsNotCancelable(FxDriverGlobals);
    }

    pRequest->Unlock(irql);
    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
STDCALL
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
    status = VerifyWdfRequestIsCanceled(pRequest->GetDriverGlobals(), pRequest);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return pRequest->IsCancelled();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
STDCALL
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
    FxRequest* pRequest;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

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

    pRequest->StopAcknowledge(Requeue);
}


__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
STDCALL
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
    FxRequest* pRequest;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;

    //
    // Validate request object handle
    //
    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Request,
                                   FX_TYPE_REQUEST,
                                   (PVOID*)&pRequest,
                                   &pFxDriverGlobals);

    return pRequest->IsReserved();
}


} // extern "C" the whole file
