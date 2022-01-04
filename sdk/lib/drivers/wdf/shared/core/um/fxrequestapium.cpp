/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxRequestApiUm.cpp

Abstract:

    This module implements FxRequest object

Author:

Environment:

    User mode only

Revision History:


--*/
#include "coreprivshared.hpp"

// Tracing support
extern "C" {
#include "FxRequestApiUm.tmh"

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestImpersonate)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_
    PFN_WDF_REQUEST_IMPERSONATE EvtRequestImpersonate,
    _In_opt_
    PVOID Context
    )

/*++

Routine Description:

    The WdfRequestImpersonate method registers the event that the framework
    should call for impersonation.

Arguments:

    Request : Request object

    ImpersonationLevel : A SECURITY_IMPERSONATION_LEVEL-typed value that identifies
        the level of impersonation.

    EvtRequestImpersonate : A pointer to the IImpersonateCallback interface whose
        method the framework calls for impersonation.

Returns:

    Impersonate returns STATUS_SUCCESS if the operation succeeds. Otherwise,
    this method returns one of the error codes that are defined in ntstatus.h.

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

    if (VALID_IMPERSONATION_LEVEL(ImpersonationLevel) == FALSE) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "ImpersonationLevel is not a valid level, %!STATUS!",
                            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }

    if (EvtRequestImpersonate == NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "EvtRequestImpersonate must not be NULL, %!STATUS!",
                            status);
        FxVerifierDbgBreakPoint(pFxDriverGlobals);
        return status;
    }


#if FX_VERBOSE_TRACE
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGREQUEST,
                        "Enter: WDFREQUEST 0x%p", Request);
#endif // FX_VERBOSE_TRACE

    status = pRequest->Impersonate(ImpersonationLevel,
                                   EvtRequestImpersonate,
                                   Context);

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
ULONG
WDFEXPORT(WdfRequestGetRequestorProcessId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
/*++

Routine Description:

    This routine returns the identifier of the process that sent the I/O request.

    The WDM IRP is invalid once WdfRequestComplete is called, regardless
    of any reference counts on the WDFREQUEST object.

Arguments:

    Request - Handle to the Request object

Returns:

    Process ID

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
        return 0;
    }

    return irp->GetRequestorProcessId();
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestIsFromUserModeDriver)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
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
        return FALSE;
    }

    return (pRequest->GetFxIrp()->GetIoIrp()->IsDriverCreated() ? TRUE : FALSE);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestSetUserModeDriverInitiatedIo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    BOOLEAN IsUserModeDriverInitiated
    )
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
        return;
    }

    pRequest->GetFxIrp()->GetIoIrp()->SetUserModeDriverInitiatedIo(
                                            IsUserModeDriverInitiated
                                            );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
BOOLEAN
WDFEXPORT(WdfRequestGetUserModeDriverInitiatedIo)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
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
        return FALSE;
    }

    return (pRequest->GetFxIrp()->GetIoIrp()->GetUserModeDriverInitiatedIo()
        ? TRUE : FALSE);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
VOID
WDFEXPORT(WdfRequestSetActivityId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _In_
    LPGUID ActivityId
    )
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
        return;
    }

    pRequest->GetFxIrp()->GetIoIrp()->SetActivityId(ActivityId);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
NTSTATUS
WDFEXPORT(WdfRequestRetrieveActivityId)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request,
    _Out_
    LPGUID ActivityId
    )
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

    FxPointerNotNull(pFxDriverGlobals, ActivityId);

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
        return status;
    }

    if (pRequest->GetFxIrp()->GetIoIrp()->IsActivityIdSet() == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGREQUEST,
                            "WDFREQUEST 0x%p Activity ID is not set for the "
                            "request, %!STATUS!", Request, status);
        return status;
    }

    *ActivityId = *(pRequest->GetFxIrp()->GetIoIrp()->GetActivityId());
    status = STATUS_SUCCESS;

    return status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
WDFAPI
WDF_DEVICE_IO_TYPE
WDFEXPORT(WdfRequestGetEffectiveIoType)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFREQUEST Request
    )
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
        return WdfDeviceIoUndefined;
    }

    return (WDF_DEVICE_IO_TYPE)(pRequest->GetFxIrp()->GetIoIrp()->GetTransferMode());
}

} // extern "C" the whole file
