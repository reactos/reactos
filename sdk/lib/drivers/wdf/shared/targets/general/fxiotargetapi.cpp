/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetAPI.cpp

Abstract:

    This module implements the IO Target APIs

Author:

Environment:

    kernel mode only

Revision History:

--*/

#include "../fxtargetsshared.hpp"

extern "C" {
// #include "FxIoTargetAPI.tmh"
}

//
// Extern the entire file
//
extern "C" {

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetStart)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Changes the target's state to started.  In the started state, the target
    can send I/O.

Arguments:
    IoTarget - the target whose state will change

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    FxIoTarget* pTarget;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         IoTarget,
                         FX_TYPE_IO_TARGET,
                         (PVOID*) &pTarget);

    return pTarget->Start();
}

__drv_when(Action == 3, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(Action == 0 || Action == 1 || Action == 2, __drv_maxIRQL(PASSIVE_LEVEL))
VOID
STDCALL
WDFEXPORT(WdfIoTargetStop)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    __drv_strictTypeMatch(__drv_typeConst)
    WDF_IO_TARGET_SENT_IO_ACTION Action
    )
/*++

Routine Description:
    This function puts the target into the stopped state.  Depending on the value
    of Action, this function may not return until sent I/O has been canceled and/or
    completed.

Arguments:
    IoTarget - the target whose state is being changed

    Action - what to do with the I/O that is pending in the target already

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget* pTarget;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    if (Action == WdfIoTargetSentIoUndefined ||
        Action > WdfIoTargetLeaveSentIoPending) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Action %d undefined or out of range", Action);
        return;
    }

    if (Action == WdfIoTargetCancelSentIo ||
        Action == WdfIoTargetWaitForSentIoToComplete) {
        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return;
        }
    }

    pTarget->Stop(Action);
}

__drv_when(Action == 2, __drv_maxIRQL(DISPATCH_LEVEL))
__drv_when(Action == 0 || Action == 1, __drv_maxIRQL(PASSIVE_LEVEL))
VOID
STDCALL
WDFEXPORT(WdfIoTargetPurge)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    __drv_strictTypeMatch(__drv_typeConst)
    WDF_IO_TARGET_PURGE_IO_ACTION Action
    )
/*++

Routine Description:
    This function puts the target into the purged state.  Depending on the value
    of Action, this function may not return until pending and sent I/O has been
    canceled and completed.

Arguments:
    IoTarget - the target whose state is being changed

    Action - purge action: wait or not for I/O to complete after doing the purge.

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget* pTarget;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    if (Action == WdfIoTargetPurgeIoUndefined ||
        Action > WdfIoTargetPurgeIo) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Action %d undefined or out of range", Action);
        return;
    }

    if (Action == WdfIoTargetPurgeIoAndWait) {
        status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
        if (!NT_SUCCESS(status)) {
            return;
        }
    }

    pTarget->Purge(Action);
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_IO_TARGET_STATE
STDCALL
WDFEXPORT(WdfIoTargetGetState)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the current state of the target

Arguments:
    IoTarget - target whose state is being returned

Return Value:
    current target state

  --*/
{
    DDI_ENTRY();

    FxIoTarget* pTarget;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         IoTarget,
                         FX_TYPE_IO_TARGET,
                         (PVOID*) &pTarget);

    return pTarget->GetState();
}

_Must_inspect_result_
NTSTATUS
FxIoTargetValidateOpenParams(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_IO_TARGET_OPEN_PARAMS OpenParams
    )
/*++

Routine Description:
    Validates the target open parameters structure.

Arguments:
    FxDriverGlobals - driver globals

    OpenParams - the structure to validate

Return Value:
    NTSTATUS

  --*/

{
    NTSTATUS status;

    //
    // Check specific fields based on Type
    //
    switch (OpenParams->Type) {
    case WdfIoTargetOpenUseExistingDevice:
        if (OpenParams->TargetDeviceObject == NULL) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Expected non NULL TargetDeviceObject in OpenParams, %!STATUS!",
                status);
            return status;
        }

        //
        // This type is supported only in KMDF.
        //
        if (FxDriverGlobals->IsUserModeDriver) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "The open type WdfIoTargetOpenUseExistingDevice is not "
                "supported in UMDF drivers. It is supported in KMDF "
                "drivers only, %!STATUS!",
                status);
            return status;
        }

        if (OpenParams->TargetFileObject == NULL &&
            (OpenParams->EvtIoTargetQueryRemove != NULL ||
             OpenParams->EvtIoTargetRemoveCanceled != NULL ||
             OpenParams->EvtIoTargetRemoveComplete != NULL)) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "OpenParams %p TargetFileObject is NULL but a state callback "
                "(query remove %p, remove canceled %p, remove complete %p)"
                " was specified, %!STATUS!",
                OpenParams, OpenParams->EvtIoTargetQueryRemove,
                OpenParams->EvtIoTargetRemoveCanceled,
                OpenParams->EvtIoTargetRemoveComplete, status);

            return status;
        }
        break;

    case WdfIoTargetOpenByName:
        if (OpenParams->TargetDeviceName.Buffer == NULL ||
            OpenParams->TargetDeviceName.Length == 0 ||
            OpenParams->TargetDeviceName.MaximumLength == 0) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Expected valid OpenParams TargetDeviceName string, %!STATUS!",
                status);
            return status;
        }
        break;

    case WdfIoTargetOpenLocalTargetByFile:
        //
        // This type is supported only in UMDF.
        //
        if (FxDriverGlobals->IsUserModeDriver == FALSE) {
            status = STATUS_INVALID_PARAMETER;
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "The open type WdfIoTargetOpenLocalTargetByFile is not "
                "supported in KMDF drivers. It is supported in UMDF "
                "drivers only, %!STATUS!",
                status);
            return status;
        }

        if ((OpenParams->EvtIoTargetQueryRemove != NULL ||
             OpenParams->EvtIoTargetRemoveCanceled != NULL ||
             OpenParams->EvtIoTargetRemoveComplete != NULL)) {
            status = STATUS_INVALID_PARAMETER;

            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "The open type is WdfIoTargetOpenLocalTargetByFile but a state"
                " callback (query remove %p, remove canceled %p, remove"
                " complete %p) was specified, %!STATUS!",
                OpenParams->EvtIoTargetQueryRemove,
                OpenParams->EvtIoTargetRemoveCanceled,
                OpenParams->EvtIoTargetRemoveComplete, status);

            return status;
        }

        if (OpenParams->FileName.Buffer != NULL ||
            OpenParams->FileName.Length != 0 ||
            OpenParams->FileName.MaximumLength != 0) {
            status = FxValidateUnicodeString(FxDriverGlobals,
                                             &OpenParams->FileName);
            if (!NT_SUCCESS(status)) {
                return status;
            }
        }


        break;

    case WdfIoTargetOpenReopen:
        break;

    default:
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "OpenParams Type (%d) incorrect, %!STATUS!",
                            OpenParams->Type, status);

        return status;
    }

    return STATUS_SUCCESS;
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetCreate)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFDEVICE Device,
    __in_opt
    PWDF_OBJECT_ATTRIBUTES IoTargetAttributes,
    __out
    WDFIOTARGET* IoTarget
    )
/*++

Routine Description:
    Creates a WDFIOTARGET which can be opened upon success.

Arguments:
    Device - the device which will own the target.  The target will be parented
             by the owning device

    IoTargetAttributes - optional attributes to apply to the target

    IoTarget - pointer which will receive the created target handle

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pTarget;
    FxDeviceBase* pDevice;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Device,
                                   FX_TYPE_DEVICE_BASE,
                                   (PWDFOBJECT) &pDevice,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, IoTarget);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFDEVICE 0x%p", Device);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Since the target is auto parented to the Device, we don't allow the client
    // to specify a parent.
    //
    status = FxValidateObjectAttributes(pFxDriverGlobals, IoTargetAttributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxIoTargetRemote::_Create(
        pFxDriverGlobals, IoTargetAttributes, pDevice, &pTarget);

    if (NT_SUCCESS(status)) {
        *IoTarget = pTarget->GetHandle();
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetOpen)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    PWDF_IO_TARGET_OPEN_PARAMS OpenParams
    )
/*++

Routine Description:
    Opens a target.  The target must be in the closed state for an open to
    succeed.  Open is either wrapping an existing PDEVICE_OBJECT + PFILE_OBJECT
    that the client provides or opening a PDEVICE_OBJECT by name.

Arguments:
    IoTarget - Target to be opened

    OpenParams - structure which describes how to open the target

Return Value:
    NTSTATUS

  --*/
{
    //
    // UMDF only, noop for KMDF.
    // It's ok to be impersonated here, because it can be required in order for
    // the CreateFile() call to succeed.
    //
    DDI_ENTRY_IMPERSONATION_OK();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pTarget;
    NTSTATUS status;
    ULONG expectedConfigSize;
    WDF_IO_TARGET_OPEN_PARAMS openParams;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET_REMOTE,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter WDFIOTARGET 0x%p", IoTarget);

    FxPointerNotNull(pFxDriverGlobals, OpenParams);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    expectedConfigSize = pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,13) ?
                            sizeof(WDF_IO_TARGET_OPEN_PARAMS) :
                            sizeof(WDF_IO_TARGET_OPEN_PARAMS_V1_11);

    //
    // Check Size
    //
    if (OpenParams->Size != sizeof(WDF_IO_TARGET_OPEN_PARAMS) &&
        OpenParams->Size != sizeof(WDF_IO_TARGET_OPEN_PARAMS_V1_11)) {
        status = STATUS_INFO_LENGTH_MISMATCH;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "OpenParams size (%d) incorrect, expected %d, %!STATUS!",
                            OpenParams->Size, expectedConfigSize, status);
        return status;
    }

    //
    // Normalize WDF_IO_TARGET_OPEN_PARAMS structure.
    //
    if (OpenParams->Size < sizeof(WDF_IO_TARGET_OPEN_PARAMS)) {
        RtlZeroMemory(&openParams, sizeof(WDF_IO_TARGET_OPEN_PARAMS));

        //
        // Copy over existing fields and readjust the struct size.
        //
        RtlCopyMemory(&openParams, OpenParams, OpenParams->Size);
        openParams.Size = sizeof(openParams);

        //
        // Use new open params structure from now on.
        //
        OpenParams = &openParams;
    }

    status = FxIoTargetValidateOpenParams(pFxDriverGlobals, OpenParams);
    if (!NT_SUCCESS(status)) {
        //
        // FxIoTargetValidateCreateParams traces the error
        //
        return status;
    }

    status = pTarget->Open(OpenParams);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, %!STATUS!", IoTarget, status);

    return status;
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfIoTargetCloseForQueryRemove)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Closes a target in response to a query remove notification callback.  This
    will pend all i/o sent after the call returns.

Arguments:
    IoTarget - Target to be closed

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pTarget;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET_REMOTE,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    DoTraceLevelMessage(
        pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "enter WDFIOTARGET 0x%p", IoTarget);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    pTarget->Close(FxIoTargetRemoteCloseReasonQueryRemove);
}

__drv_maxIRQL(PASSIVE_LEVEL)
VOID
STDCALL
WDFEXPORT(WdfIoTargetClose)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Closes the target for good.  The target can be in either a query removed or
    opened state.

Arguments:
    IoTarget - target to close

Return Value:
    None

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pTarget;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET_REMOTE,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter WDFIOTARGET 0x%p", IoTarget);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return;
    }

    pTarget->Close(FxIoTargetRemoteCloseReasonPlainClose);
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDFDEVICE
STDCALL
WDFEXPORT(WdfIoTargetGetDevice)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the owning WDFDEVICE for the WDFIOTARGET, this is not necessarily
    the PDEVICE_OBJECT of the target itself.

Arguments:
    IoTarget - the target being retrieved

Return Value:
    a valid WDFDEVICE handle , NULL if there are any problems

  --*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget* pTarget;
    WDFDEVICE device;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter WDFIOTARGET 0x%p", IoTarget);

    device = pTarget->GetDeviceHandle();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, WDFDEVICE 0x%p", IoTarget, device);

    return device;
}

static
_Must_inspect_result_
NTSTATUS
FxIoTargetSendIo(
    __in
    PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __inout_opt
    WDFREQUEST Request,
    __in
    UCHAR MajorCode,
    __inout_opt
    PWDF_MEMORY_DESCRIPTOR IoBuffer,
    __in_opt
    PLONGLONG DeviceOffset,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
/*++

Routine Description:
    This routine sends a read or write synchronously to the target.  If Request
    is not NULL, the PIRP it contains will be used to send the IO.

Arguments:
    IoTarget - target to where the IO is going to be sent

    Request - optional.  If specified, the PIRP it contains will be used
              to send the I/O

    MajorCode - read or write major code

    IoBuffer - Buffer which will be used in the I/O.  The buffer can be a PMDL,
               buffer, or WDFMEMORY

    DeviceOffset - offset into the target (and not the memory) in which the I/O
                   will start

    RequestOptions - optional.  If specified, the timeout value is used to cancel
                     the sent i/o if the timeout is exceeded

    BytesReturned - upon success, the number of bytes transferred in the I/O
                    request

Return Value:
    NTSTATUS

  --*/

{
    FxIoTarget* pTarget;
    FxRequestBuffer ioBuf;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(FxDriverGlobals,
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &FxDriverGlobals);

    //
    // Minimize the points of failure by using the stack instead of allocating
    // out of pool. For UMDF, request initialization can fail so we still need
    // to call initialize for FxSyncRequest. Initialization always succeeds for
    // KM.
    //
    FxIoContext context;
    FxSyncRequest request(FxDriverGlobals, &context, Request);

    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest for WDFIOTARGET "
                            "0x%p", IoTarget);
        return status;
    }

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter:  WDFIOTARGET 0x%p, WDFREQUEST 0x%p, MJ code 0x%x",
                        IoTarget, Request, MajorCode);

    //
    // Since we are synchronously waiting, we must be at passive level
    //
    status = FxVerifierCheckIrqlLevel(FxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(FxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "invalid options, %!STATUS!", status);
        return status;
    }

    if (IoBuffer != NULL) {
        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        status = ioBuf.ValidateMemoryDescriptor(FxDriverGlobals, IoBuffer);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "invalid input buffer descriptor 0x%p, %!STATUS!",
                                IoBuffer, status);
            return status;
        }
    }

    //
    // Format the next stack location in the PIRP
    //
    status = pTarget->FormatIoRequest(
        request.m_TrueRequest, MajorCode, &ioBuf, DeviceOffset, NULL);

    if (NT_SUCCESS(status)) {
        //
        // And send it
        //
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFIOTARGET 0x%p, WDFREQUEST 0x%p being submitted",
                            IoTarget, request.m_TrueRequest->GetTraceObjectHandle());

        status = pTarget->SubmitSync(request.m_TrueRequest, RequestOptions);

        if (BytesReturned != NULL) {
            *BytesReturned = request.m_TrueRequest->GetSubmitFxIrp()->GetInformation();

        }
    }
    else {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "could not format MJ 0x%x request, %!STATUS!",
                            MajorCode, status);
    }

    return status;
}

static
_Must_inspect_result_
NTSTATUS
FxIoTargetFormatIo(
    __in
    PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __inout
    WDFREQUEST Request,
    __in
    UCHAR MajorCode,
    __inout_opt
    WDFMEMORY IoBuffer,
    __in_opt
    PWDFMEMORY_OFFSET IoBufferOffsets,
    __in_opt
    PLONGLONG DeviceOffset
    )
/*++

Routine Description:
    Formats a Request for a read or write.  The request can later be sent to the
    target using WdfRequestSend. Upon success, this will take a reference on the
    Iobuffer handle if it is not NULL.  This reference will be released when
    one of the following occurs:
    1)  the request is completed through WdfRequestComplete
    2)  the request is reused through WdfRequestReuse
    3)  the request is reformatted through any target format DDI

Arguments:
    IoTarget - the request that the read or write will later be sent to

    Request - the request that will be formatted

    MajorCode - read or write major code

    IoBuffer - optional reference counted memory handle

    IoBufferOffset - optional offset into the IoBuffer.  This can specify the
                     starting offset and/or length of the transfer

    DeviceOffset - offset into the target in which the i/o will start

Return Value:
    NTSTATUS

  --*/
{
    FxIoTarget *pTarget;
    FxRequest *pRequest;
    IFxMemory* pIoMemory;
    FxRequestBuffer ioBuf;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(FxDriverGlobals,
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &FxDriverGlobals);

    DoTraceLevelMessage(FxDriverGlobals,
        TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "enter: WDFIOTARGET 0x%p, WDFREQUEST 0x%p, MJ code 0x%x, WDFMEMORY 0x%p",
        IoTarget, Request, MajorCode, IoBuffer);

    FxObjectHandleGetPtr(FxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    if (IoBuffer != NULL) {
        FxObjectHandleGetPtr(FxDriverGlobals,
                             IoBuffer,
                             IFX_TYPE_MEMORY,
                             (PVOID*) &pIoMemory);

        status = pIoMemory->ValidateMemoryOffsets(IoBufferOffsets);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "invalid memory offsets, %!STATUS!",
                                status);
            return status;
        }

        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        ioBuf.SetMemory(pIoMemory, IoBufferOffsets);
    }
    else {
        pIoMemory = NULL;
    }

    //
    // Format the next stack locaiton in the PIRP
    //
    status = pTarget->FormatIoRequest(
        pRequest, MajorCode, &ioBuf, DeviceOffset, NULL);

    if (NT_SUCCESS(status)) {
        if (MajorCode == IRP_MJ_WRITE) {
            pRequest->GetContext()->FormatWriteParams(pIoMemory, IoBufferOffsets);
        }
        else if (MajorCode == IRP_MJ_READ) {
            pRequest->GetContext()->FormatReadParams(pIoMemory, IoBufferOffsets);
        }
    }

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, WDFREQUEST 0x%p, %!STATUS!",
                        IoTarget, Request, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetSendReadSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    __in_opt
    PLONGLONG DeviceOffset,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesRead
    )
{
    DDI_ENTRY();

    return FxIoTargetSendIo(GetFxDriverGlobals(DriverGlobals),
                            IoTarget,
                            Request,
                            IRP_MJ_READ,
                            OutputBuffer,
                            DeviceOffset,
                            RequestOptions,
                            BytesRead);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetFormatRequestForRead)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY OutputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET OutputBufferOffsets,
    __in_opt
    PLONGLONG DeviceOffset
    )
{
    DDI_ENTRY();

    return FxIoTargetFormatIo(GetFxDriverGlobals(DriverGlobals),
                              IoTarget,
                              Request,
                              IRP_MJ_READ,
                              OutputBuffer,
                              OutputBufferOffsets,
                              DeviceOffset);
}


_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetSendWriteSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    __in_opt
    PLONGLONG DeviceOffset,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesWritten
    )
{
    DDI_ENTRY();

    return FxIoTargetSendIo(GetFxDriverGlobals(DriverGlobals),
                            IoTarget,
                            Request,
                            IRP_MJ_WRITE,
                            InputBuffer,
                            DeviceOffset,
                            RequestOptions,
                            BytesWritten);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetFormatRequestForWrite)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY InputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET InputBufferOffsets,
    __in_opt
    PLONGLONG DeviceOffset
    )
{
    DDI_ENTRY();

    return FxIoTargetFormatIo(GetFxDriverGlobals(DriverGlobals),
                              IoTarget,
                              Request,
                              IRP_MJ_WRITE,
                              InputBuffer,
                              InputBufferOffsets,
                              DeviceOffset);
}

_Must_inspect_result_
NTSTATUS
FxIoTargetSendIoctl(
    __in
     PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in
    BOOLEAN Internal,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
/*++

Routine Description:
    Sends an external or internal IOCTL to the target.  If the optional request
    is specified, this function will use it's PIRP to send the request to the
    target.  Both buffers are optional.

Arguments:
    IoTarget - the target to which the IOCTL is being sent

    IOCTL - the device io control value

    Internal - if TRUE, an internal IOCTL, if FALSE, a normal IOCTL

    InputBuffer - optional.  Can be one of the following:  PMDL, PVOID, or WDFMEMORY

    OutputBuffer - optional.  Can be one of the following:  PMDL, PVOID, or WDFMEMORY

    RequestOptions - optional.  Specifies a timeout to be used if the sent IO
                     does not return within the time specified.

    BytesReturned - number of bytes transfered

Return Value:
    NTSTATUS

  --*/
{
    FxIoTarget* pTarget;
    FxRequestBuffer inputBuf, outputBuf;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(FxDriverGlobals,
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &FxDriverGlobals);

    //
    // Minimize the points of failure by using the stack instead of allocating
    // out of pool. For UMDF, request initialization can fail so we still need
    // to call initialize for FxSyncRequest. Initialization always succeeds for
    // KM.
    //
    FxIoContext context;
    FxSyncRequest request(FxDriverGlobals, &context, Request);

    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest for WDFIOTARGET "
                            "0x%p", IoTarget);
        return status;
    }

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter:  WDFIOTARGET 0x%p, WDFREQUEST 0x%p, IOCTL 0x%x, "
                        "internal %d", IoTarget, Request, Ioctl, Internal);

    //
    // Since we are synchronously waiting, we must be at passive
    //
    status = FxVerifierCheckIrqlLevel(FxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(FxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "invalid options, %!STATUS!", status);
        return status;
    }

    if (InputBuffer != NULL) {
        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        status = inputBuf.ValidateMemoryDescriptor(FxDriverGlobals, InputBuffer);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "invalid input buffer descriptor 0x%p, %!STATUS!",
                                InputBuffer, status);
            return status;
        }
    }

    if (OutputBuffer != NULL) {
        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        status = outputBuf.ValidateMemoryDescriptor(FxDriverGlobals, OutputBuffer);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "invalid output buffer descriptor 0x%p, %!STATUS!",
                                OutputBuffer, status);
            return status;
        }
    }

    //
    // Format the next stack location
    //
    status = pTarget->FormatIoctlRequest(
        request.m_TrueRequest, Ioctl, Internal, &inputBuf, &outputBuf, NULL);

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFIOTARGET 0x%p, WDFREQUEST 0x%p being submitted",
                            IoTarget,
                            request.m_TrueRequest->GetTraceObjectHandle());

        status = pTarget->SubmitSync(request.m_TrueRequest, RequestOptions);

        if (BytesReturned != NULL) {
            *BytesReturned = request.m_TrueRequest->GetSubmitFxIrp()->GetInformation();
        }
    }
    else {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "could not format IOCTL 0x%x request, %!STATUS!",
                            Ioctl, status);
    }

    return status;
}

static
_Must_inspect_result_
NTSTATUS
FxIoTargetFormatIoctl(
    __in
    PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in
    BOOLEAN Internal,
    __in_opt
    WDFMEMORY InputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET InputBufferOffsets,
    __in_opt
    WDFMEMORY OutputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET OutputBufferOffsets
    )
/*++

Routine Description:
    Formats a request as an IOCTL to be sent to the specified target.  Upon
    success, this will take a reference on each of the WDFMEMORY handles that
    are passed in.  This reference will be released when one of the following
    occurs:
    1)  the request is completed through WdfRequestComplete
    2)  the request is reused through WdfRequestReuse
    3)  the request is reformatted through any target format DDI

Arguments:
    IoTarget - the target to which the IOCTL will be formatted for

    Request - the request which will be formatted

    IOCTL - the device IO control itself to be used

    Internal - if TRUE, an internal IOCTL, if FALSE, a normal IOCTL

    InputBuffer - optional.  If specified, a reference counted memory handle to
                  be placed in the next stack location.

    InputBufferOffsets - optional.  If specified, it can override the starting
                         offset of the buffer and the length of the buffer used

    OutputBuffer - optional.  If specified, a reference counted memory handle to
                   be placed in the next stack location.

    OutputBufferOffsets - optional.  If specified, it can override the starting
                          offset of the buffer and the length of the buffer used

Return Value:
    NTSTATUS

  --*/
{
    FxIoTarget *pTarget;
    FxRequest *pRequest;
    IFxMemory *pInputMemory, *pOutputMemory;
    FxRequestBuffer inputBuf, outputBuf;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(FxDriverGlobals,
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &FxDriverGlobals);

    DoTraceLevelMessage(
        FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "enter: WDFIOTARGET 0x%p, WDFREQUEST 0x%p, IOCTL 0x%x, internal %d, input "
        "WDFMEMORY 0x%p, output WDFMEMORY 0x%p",
        IoTarget, Request, Ioctl, Internal, InputBuffer, OutputBuffer);

    FxObjectHandleGetPtr(FxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    if (InputBuffer != NULL) {
        FxObjectHandleGetPtr(FxDriverGlobals,
                             InputBuffer,
                             IFX_TYPE_MEMORY,
                             (PVOID*) &pInputMemory);

        status = pInputMemory->ValidateMemoryOffsets(InputBufferOffsets);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "Invalid input memory offsets, %!STATUS!",
                                status);
            return status;
        }

        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        inputBuf.SetMemory(pInputMemory, InputBufferOffsets);
    }

    if (OutputBuffer != NULL) {
        FxObjectHandleGetPtr(FxDriverGlobals,
                             OutputBuffer,
                             IFX_TYPE_MEMORY,
                             (PVOID*) &pOutputMemory);

        status = pOutputMemory->ValidateMemoryOffsets(OutputBufferOffsets);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "Invalid output memory offsets, %!STATUS!",
                                status);
            return status;
        }

        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        outputBuf.SetMemory(pOutputMemory, OutputBufferOffsets);
    }

    //
    // format the next stack location
    //
    status = pTarget->FormatIoctlRequest(
        pRequest, Ioctl, Internal, &inputBuf, &outputBuf, NULL);

    if (NT_SUCCESS(status)) {
        FxRequestContext* pContext;

        //
        // Upon a successful format,  a FxRequestContext will have been
        // associated with the FxRequest
        //
        pContext = pRequest->GetContext();

        pContext->m_CompletionParams.Parameters.Ioctl.IoControlCode = Ioctl;

        if (Internal) {
            pContext->m_CompletionParams.Type = WdfRequestTypeDeviceControlInternal;
        }
        else {
            pContext->m_CompletionParams.Type = WdfRequestTypeDeviceControl;
        }

        pContext->m_CompletionParams.Parameters.Ioctl.Input.Buffer = InputBuffer;
        if (InputBufferOffsets != NULL) {
            pContext->m_CompletionParams.Parameters.Ioctl.Input.Offset =
                InputBufferOffsets->BufferOffset;
        }

        pContext->m_CompletionParams.Parameters.Ioctl.Output.Buffer = OutputBuffer;
        if (OutputBufferOffsets != NULL) {
            pContext->m_CompletionParams.Parameters.Ioctl.Output.Offset =
                OutputBufferOffsets->BufferOffset;
        }
    }

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Exit WDFIOTARGET 0x%p, WDFREQUEST 0x%p, %!STATUS!",
                        IoTarget, Request, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetSendIoctlSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
{
    DDI_ENTRY();

    return FxIoTargetSendIoctl(
        GetFxDriverGlobals(DriverGlobals),
        IoTarget,
        Request,
        Ioctl,
        FALSE,
        InputBuffer,
        OutputBuffer,
        RequestOptions,
        BytesReturned
        );
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetFormatRequestForIoctl)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    WDFMEMORY InputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET InputBufferOffsets,
    __in_opt
    WDFMEMORY OutputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET OutputBufferOffsets
    )
{
    DDI_ENTRY();

    return FxIoTargetFormatIoctl(GetFxDriverGlobals(DriverGlobals),
                                 IoTarget,
                                 Request,
                                 Ioctl,
                                 FALSE,
                                 InputBuffer,
                                 InputBufferOffsets,
                                 OutputBuffer,
                                 OutputBufferOffsets);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetSendInternalIoctlSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR InputBuffer,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OutputBuffer,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
{
    DDI_ENTRY();

    return FxIoTargetSendIoctl(
        GetFxDriverGlobals(DriverGlobals),
        IoTarget,
        Request,
        Ioctl,
        TRUE,
        InputBuffer,
        OutputBuffer,
        RequestOptions,
        BytesReturned
        );
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctl)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    WDFMEMORY InputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET InputBufferOffsets,
    __in_opt
    WDFMEMORY OutputBuffer,
    __in_opt
    PWDFMEMORY_OFFSET OutputBufferOffsets
    )
{
    DDI_ENTRY();

    return FxIoTargetFormatIoctl(GetFxDriverGlobals(DriverGlobals),
                                 IoTarget,
                                 Request,
                                 Ioctl,
                                 TRUE,
                                 InputBuffer,
                                 InputBufferOffsets,
                                 OutputBuffer,
                                 OutputBufferOffsets);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetSendInternalIoctlOthersSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in_opt
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OtherArg1,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OtherArg2,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR OtherArg4,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __out_opt
    PULONG_PTR BytesReturned
    )
/*++

Routine Description:
    Sends an internal IOCTL to the target synchronously.  Since all 3 buffers can
    be used, we cannot overload WdfIoTargetSendInternalIoctlSynchronously since
    it can only take 2 buffers.

Arguments:
    IoTarget - the target to which the request will be sent

    Request - optional.  If specified, the request's PIRP will be used to send
              the i/o to the target.

    Ioctl - internal ioctl value to send

    OtherArg1
    OtherArg2
    OtherArg4 - arguments to use in the stack locations's Others field.  There
                is no OtherArg3 because 3 is where the IOCTL value is written.
                All buffers are optional.

    RequestOptions - optional.  If specified, the timeout indicated will be used
                     if the request exceeds the timeout.

    BytesReturned - the number of bytes returned by the target

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget* pTarget;
    FxRequestBuffer args[FX_REQUEST_NUM_OTHER_PARAMS];
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    //
    // Minimize the points of failure by using the stack instead of allocating
    // out of pool. For UMDF, request initialization can fail so we still need
    // to call initialize for FxSyncRequest. Initialization always succeeds for
    // KM.
    //
    FxInternalIoctlOthersContext context;
    FxSyncRequest request(pFxDriverGlobals, &context, Request);

    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest for WDFIOTARGET "
                            "0x%p", IoTarget);
        return status;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "enter:  WDFIOTARGET 0x%p, WDFREQUEST 0x%p, IOCTL 0x%x, Args %p %p %p",
        IoTarget, Request, Ioctl, OtherArg1, OtherArg2, OtherArg4);

    //
    // Since we are waiting synchronously, we must be at pasisve
    //
    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(pFxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Invalid options, %!STATUS!", status);
        return status;
    }

    ULONG i;

    i = 0;
    if (OtherArg1 != NULL) {
        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        status = args[i].ValidateMemoryDescriptor(pFxDriverGlobals, OtherArg1);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "invalid OtherArg1 buffer descriptor 0x%p, %!STATUS!",
                                OtherArg1, status);
            return status;
        }
    }

    i++;
    if (OtherArg2 != NULL) {
        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        status = args[i].ValidateMemoryDescriptor(pFxDriverGlobals, OtherArg2);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "invalid OtherArg2 buffer descriptor 0x%p, %!STATUS!",
                                OtherArg2, status);
            return status;
        }
    }

    i++;
    if (OtherArg4 != NULL) {
        //
        // This transcribes the client union into a local structure which we
        // can change w/out altering the client's buffer.
        //
        status = args[i].ValidateMemoryDescriptor(pFxDriverGlobals, OtherArg4);
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "invalid OtherArg4 buffer descriptor 0x%p, %!STATUS!",
                                OtherArg4, status);
            return status;
        }
    }

    //
    // Format the next stack location
    //
    status = pTarget->FormatInternalIoctlOthersRequest(request.m_TrueRequest,
                                                       Ioctl,
                                                       args);

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "WDFIOTARGET 0x%p, WDFREQUEST 0x%p being submitted",
                            IoTarget,
                            request.m_TrueRequest->GetTraceObjectHandle());

        status = pTarget->SubmitSync(request.m_TrueRequest, RequestOptions);

        if (BytesReturned != NULL) {
            *BytesReturned = request.m_TrueRequest->GetSubmitFxIrp()->GetInformation();
        }
    }
    else {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Could not format IOCTL 0x%x request, %!STATUS!",
                            Ioctl, status);
    }

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetFormatRequestForInternalIoctlOthers)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget,
    __in
    WDFREQUEST Request,
    __in
    ULONG Ioctl,
    __in_opt
    WDFMEMORY OtherArg1,
    __in_opt
    PWDFMEMORY_OFFSET OtherArg1Offsets,
    __in_opt
    WDFMEMORY OtherArg2,
    __in_opt
    PWDFMEMORY_OFFSET OtherArg2Offsets,
    __in_opt
    WDFMEMORY OtherArg4,
    __in_opt
    PWDFMEMORY_OFFSET OtherArg4Offsets
    )
/*++

Routine Description:
    Formats an internal IOCTL so that it can sent to the target.  Since all 3
    buffers can be used, we cannot overload
    WdfIoTargetFormatRequestForInternalIoctlOthers since it can only take 2 buffers.

    Upon success, this will take a reference on each of the WDFMEMORY handles that
    are passed in.  This reference will be released when one of the following
    occurs:
    1)  the request is completed through WdfRequestComplete
    2)  the request is reused through WdfRequestReuse
    3)  the request is reformatted through any target format DDI

Arguments:
    IoTarget - the target to which the request will be sent

    Request - the request to be formatted

    Ioctl - internal ioctl value to send

    OtherArg1
    OtherArg2
    OtherArg4 - arguments to use in the stack locations's Others field.  There
                is no OtherArg3 because 3 is where the IOCTL value is written.
                All buffers are optional

    OterhArgXOffsets - offset into each buffer which can override the starting
                       offset of the buffer.  Length does not matter since
                       there is no way of generically describing the length of
                       each of the 3 buffers in the PIRP

Return Value:
    NTSTATUS

  --*/
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTarget *pTarget;
    FxRequest *pRequest;
    IFxMemory *pMemory[FX_REQUEST_NUM_OTHER_PARAMS];
    FxRequestBuffer args[FX_REQUEST_NUM_OTHER_PARAMS];
    WDFMEMORY memoryHandles[FX_REQUEST_NUM_OTHER_PARAMS];
    PWDFMEMORY_OFFSET offsets[FX_REQUEST_NUM_OTHER_PARAMS];
    NTSTATUS status;
    ULONG i;
    FxInternalIoctlParams InternalIoctlParams;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Enter: WDFIOTARGET 0x%p, WDFREQUEST 0x%p, IOCTL 0x%x, "
                        "WDFMEMORY 1 0x%p, 2 0x%p, 3 0x%p",
                        IoTarget, Request, Ioctl, OtherArg1, OtherArg2,
                        OtherArg4);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    i = 0;
    InternalIoctlParams.Argument1 = memoryHandles[i] = OtherArg1;
    offsets[i] = OtherArg1Offsets;

    InternalIoctlParams.Argument2 = memoryHandles[++i] = OtherArg2;
    offsets[i] = OtherArg2Offsets;

    InternalIoctlParams.Argument4 = memoryHandles[++i] = OtherArg4;
    offsets[i] = OtherArg4Offsets;

    for (i = 0; i < FX_REQUEST_NUM_OTHER_PARAMS; i++) {
        if (memoryHandles[i] != NULL) {

            FxObjectHandleGetPtr(pFxDriverGlobals,
                                 memoryHandles[i],
                                 IFX_TYPE_MEMORY,
                                 (PVOID*) &pMemory[i]);

            status = pMemory[i]->ValidateMemoryOffsets(offsets[i]);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                    "Invalid OtherArg%d memory offsets, %!STATUS!",
                                    i+1, status);
                return status;
            }

            //
            // This transcribes the client union into a local structure which we
            // can change w/out altering the client's buffer.
            //
            args[i].SetMemory(pMemory[i], offsets[i]);
        }
    }

    status = pTarget->FormatInternalIoctlOthersRequest(pRequest, Ioctl, args);
    if (NT_SUCCESS(status)) {
        pRequest->GetContext()->FormatOtherParams(&InternalIoctlParams);
    }
    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Exit: WDFIOTARGET %p, WDFREQUEST %p, IOCTL 0x%x, "
                        "Arg Handles %p %p %p, status %!STATUS!",
                        IoTarget, Request, Ioctl, OtherArg1, OtherArg2,
                        OtherArg4, status);

    return status;
}

_Must_inspect_result_
_IRQL_requires_max_(DISPATCH_LEVEL)
NTSTATUS
STDCALL
WDFEXPORT(WdfIoTargetSelfAssignDefaultIoQueue)(
    _In_
    PWDF_DRIVER_GLOBALS DriverGlobals,
    _In_
    WDFIOTARGET IoTarget,
    _In_
    WDFQUEUE Queue
    )

/*++

Routine Description:

    Assigns a default queue for the Self IO Target.

    By default the IO sent to the Self IO Target is dispatched ot the
    client's default / top level queue.

    This routine assigns a default queue for the Intenral I/O target.
    If a client calls this API, all the I/O directed to the Self IO target
    is dispatched to the queue specified.

Arguments:

    IoTarget - Handle to the Self Io Target.

    Queue - Handle to a queue that is being assigned as the default queue for
       the Self io target.

Returns:

    NTSTATUS

--*/

{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pGlobals;
    NTSTATUS status;
    FxIoTargetSelf* pTargetSelf;
    FxDevice*  pDevice;
    FxIoQueue* pFxIoQueue;

    pDevice = NULL;
    pFxIoQueue = NULL;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET_SELF,
                                   (PVOID *) &pTargetSelf,
                                   &pGlobals);

    pDevice = pTargetSelf->GetDevice();

    //
    // Validate the Queue handle
    //
    FxObjectHandleGetPtr(pGlobals,
                         Queue,
                         FX_TYPE_QUEUE,
                         (PVOID*)&pFxIoQueue);

    if (pDevice != pFxIoQueue->GetDevice()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
            "Input WDFQUEUE 0x%p belongs to WDFDEVICE 0x%p, but "
            "Self Io Target 0x%p corresponds to the WDFDEVICE 0x%p, %!STATUS!",
            Queue, pFxIoQueue->GetDevice()->GetHandle(), IoTarget,
            pDevice->GetHandle(), status);

        return status;
    }

    if (pDevice->IsLegacy()) {
        //
        // This is a controldevice. Make sure the create is called after the device
        // is initialized and ready to accept I/O.
        //
        MxDeviceObject deviceObject(pDevice->GetDeviceObject());
        if ((deviceObject.GetFlags() & DO_DEVICE_INITIALIZING) == 0x0) {

            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                pGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Queue cannot be configured for automatic dispatching"
                " after WdfControlDeviceFinishInitializing"
                "is called on the WDFDEVICE %p is called %!STATUS!",
                pDevice->GetHandle(),
                status);
            return status;
        }
    }
    else {
        //
        // This is either FDO or PDO. Make sure it's not started yet.
        //
        if (pDevice->GetDevicePnpState() != WdfDevStatePnpInit) {
            status = STATUS_INVALID_DEVICE_STATE;
            DoTraceLevelMessage(
                pGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
                "Queue cannot be configured for automatic dispatching"
                "after the WDFDEVICE %p is started, %!STATUS!",
                pDevice->GetHandle(), status);
            return status;
        }
    }

    pTargetSelf->SetDispatchQueue(pFxIoQueue);

    return STATUS_SUCCESS;
}

__drv_maxIRQL(DISPATCH_LEVEL)
HANDLE
STDCALL
WDFEXPORT(WdfIoTargetWdmGetTargetFileHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFIOTARGET IoTarget
    )
/*++

Routine Description:
    Returns the file handle that the target represents. For KMDF, the handle is a kernel
    handle, so it is not tied to any process context. For UMDF it is a Win32 handle opened
    in the host process context. Not all targets have a file handle associated with them,
    so NULL is a valid return value that does not indicate error.

Arguments:
    IoTarget - target whose file handle is being returned

Return Value:
    A valid kernel/win32 handle or NULL

  --*/
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxIoTargetRemote* pTarget;
    PVOID handle;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   IoTarget,
                                   FX_TYPE_IO_TARGET_REMOTE,
                                   (PVOID*) &pTarget,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "enter WDFIOTARGET 0x%p", IoTarget);

    handle = pTarget->GetTargetHandle();

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "exit WDFIOTARGET 0x%p, WDM file handle 0x%p",
                        IoTarget, handle);

    return handle;
}


} // extern "C"
