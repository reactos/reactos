/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbPipeAPI.cpp

Abstract:


Author:

Environment:

    Both kernel and user mode

Revision History:

--*/

#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbPipeAPI.tmh"
}

//
// extern "C" the whole file since we are exporting the APIs by name
//
extern "C" {

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfUsbTargetPipeGetInformation)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __out
    PWDF_USB_PIPE_INFORMATION PipeInformation
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbPipe* pUsbPipe;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, PipeInformation);

    pUsbPipe->GetInformation(PipeInformation);
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFAPI
WDFEXPORT(WdfUsbTargetPipeIsInEndpoint)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    )
{
    DDI_ENTRY();

    FxUsbPipe* pUsbPipe;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Pipe,
                         FX_TYPE_IO_TARGET_USB_PIPE,
                         (PVOID*) &pUsbPipe);

    return pUsbPipe->IsInEndpoint();
}

__drv_maxIRQL(DISPATCH_LEVEL)
BOOLEAN
WDFAPI
WDFEXPORT(WdfUsbTargetPipeIsOutEndpoint)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    )
{
    DDI_ENTRY();

    FxUsbPipe* pUsbPipe;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Pipe,
                         FX_TYPE_IO_TARGET_USB_PIPE,
                         (PVOID*) &pUsbPipe);

    return pUsbPipe->IsOutEndpoint();
}

__drv_maxIRQL(DISPATCH_LEVEL)
WDF_USB_PIPE_TYPE
WDFAPI
WDFEXPORT(WdfUsbTargetPipeGetType)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    )
{
    DDI_ENTRY();

    FxUsbPipe* pUsbPipe;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         Pipe,
                         FX_TYPE_IO_TARGET_USB_PIPE,
                         (PVOID*) &pUsbPipe);

    return pUsbPipe->GetType();
}

__drv_maxIRQL(DISPATCH_LEVEL)
VOID
WDFAPI
WDFEXPORT(WdfUsbTargetPipeSetNoMaximumPacketSizeCheck)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbPipe* pUsbPipe;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p", Pipe);

    pUsbPipe->SetNoCheckPacketSize();
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeWriteSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesWritten
    )
{
    DDI_ENTRY();

    DoTraceLevelMessage(
        GetFxDriverGlobals(DriverGlobals), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFUSBPIPE %p", Pipe);

    return FxUsbPipe::_SendTransfer(GetFxDriverGlobals(DriverGlobals),
                                    Pipe,
                                    Request,
                                    RequestOptions,
                                    MemoryDescriptor,
                                    BytesWritten,
                                    0);
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeFormatRequestForWrite)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY WriteMemory,
    __in_opt
    PWDFMEMORY_OFFSET WriteOffsets
    )
{
    DDI_ENTRY();

    DoTraceLevelMessage(GetFxDriverGlobals(DriverGlobals), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p, WDFREQUEST %p, WDFMEMORY %p",
                        Pipe, Request, WriteMemory);

    return FxUsbPipe::_FormatTransfer(GetFxDriverGlobals(DriverGlobals),
                                      Pipe,
                                      Request,
                                      WriteMemory,
                                      WriteOffsets,
                                      0);
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeReadSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_opt
    PWDF_MEMORY_DESCRIPTOR MemoryDescriptor,
    __out_opt
    PULONG BytesRead
    )
{
    DDI_ENTRY();

    DoTraceLevelMessage(GetFxDriverGlobals(DriverGlobals), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p", Pipe);

    return FxUsbPipe::_SendTransfer(
        GetFxDriverGlobals(DriverGlobals),
        Pipe,
        Request,
        RequestOptions,
        MemoryDescriptor,
        BytesRead,
        USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK
        );
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeFormatRequestForRead)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request,
    __in_opt
    WDFMEMORY ReadMemory,
    __in_opt
    PWDFMEMORY_OFFSET ReadOffsets
    )
{
    DDI_ENTRY();

    DoTraceLevelMessage(
        GetFxDriverGlobals(DriverGlobals), TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
        "WDFUSBPIPE %p, WDFREQUEST %p, WDFMEMORY %p",
        Pipe, Request, ReadMemory);

    return FxUsbPipe::_FormatTransfer(
        GetFxDriverGlobals(DriverGlobals),
        Pipe,
        Request,
        ReadMemory,
        ReadOffsets,
        USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK
        );
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeConfigContinuousReader)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    PWDF_USB_CONTINUOUS_READER_CONFIG Config
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbPipe* pUsbPipe;
    NTSTATUS status;
    size_t total;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    FxPointerNotNull(pFxDriverGlobals, Config);

    if (Config->Size != sizeof(WDF_USB_CONTINUOUS_READER_CONFIG)) {
        status = STATUS_INFO_LENGTH_MISMATCH;

        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Config %p incorrect size %d, expected %d %!STATUS!",
                            Config, Config->Size, sizeof(WDF_USB_CONTINUOUS_READER_CONFIG),
                            status);

        return status;
    }

    if (Config->EvtUsbTargetPipeReadComplete == NULL) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "NULL EvtUsbTargetPipeReadComplete not allowed %!STATUS!", status);
        return status;
    }

    if (Config->TransferLength == 0) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "TransferLength of 0 not allowed %!STATUS!", status);
        return status;
    }

    status = RtlSizeTAdd(Config->HeaderLength,
                         Config->TransferLength,
                         &total);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "HeaderLength + TransferLength overflow %!STATUS!", status);
        return status;
    }

    status = RtlSizeTAdd(total,
                         Config->TrailerLength,
                         &total);

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "HeaderLength + TransferLength + TrailerLength overflow %!STATUS!",
            status);
        return status;
    }

    //
    // Internally WDF will assign a parent to the memory, so do not allow the driver
    // to do so.
    //
    status = FxValidateObjectAttributes(pFxDriverGlobals,
                                        Config->BufferAttributes,
                                        FX_VALIDATE_OPTION_PARENT_NOT_ALLOWED);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // Only bulk or interrrupt is allowed for a continous reader
    //
    if ((pUsbPipe->IsType(WdfUsbPipeTypeBulk) ||
         pUsbPipe->IsType(WdfUsbPipeTypeInterrupt)) == FALSE) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBPIPE %p type %!WDF_USB_PIPE_TYPE!, only bulk or interrupt "
            "pipes can be configured for continous readers, %!STATUS!",
            Pipe, pUsbPipe->GetType(), status);

        return status;
    }

    if (pUsbPipe->IsOutEndpoint()) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "WDFUSBPIPE %p, wrong direction for continuous reader, %!STATUS!",
            Pipe, status);

        return status;
    }

    status = pUsbPipe->ValidateTransferLength(Config->TransferLength);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "TransferLength %I64d not a valid transer length (not integral of max "
            "packet size %d) %!STATUS!", Config->TransferLength,
            pUsbPipe->GetMaxPacketSize(), status);
        return status;
    }

    status = pUsbPipe->InitContinuousReader(Config, total);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeAbortSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbPipe* pUsbPipe;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    FxUsbPipeRequestContext context(FxUrbTypeLegacy);

    FxSyncRequest request(pFxDriverGlobals, &context, Request);

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Pipe %p", Pipe);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(pFxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Invalid request options");
        return status;
    }

    status = pUsbPipe->FormatAbortRequest(request.m_TrueRequest);
    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBPIPE %p, WDFREQUEST %p being submitted",
            Pipe, request.m_TrueRequest->GetTraceObjectHandle());

        status = pUsbPipe->SubmitSync(request.m_TrueRequest, RequestOptions);
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p, %!STATUS!", Pipe, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeFormatRequestForAbort)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest* pRequest;
    FxUsbPipe* pUsbPipe;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Pipe %p, Request %p", Pipe, Request);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    status = pUsbPipe->FormatAbortRequest(pRequest);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Pipe %p, Request %p, status %!STATUS!",
                        Pipe, Request, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeResetSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbPipe* pUsbPipe;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    FxUsbPipeRequestContext context(FxUrbTypeLegacy);

    FxSyncRequest request(pFxDriverGlobals, &context, Request);

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p reset", Pipe);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(pFxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Invalid request options");
        return status;
    }

    status = pUsbPipe->FormatResetRequest(request.m_TrueRequest);

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBPIPE %p, WDFREQUEST %p being submitted",
            Pipe,  request.m_TrueRequest->GetTraceObjectHandle());

        pUsbPipe->CancelSentIo();

        //
        // Even if the previous state of the target was stopped let this IO go through by
        // ignoring target state.
        //
        status = pUsbPipe->SubmitSyncRequestIgnoreTargetState(request.m_TrueRequest, RequestOptions);
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p reset, %!STATUS!", Pipe, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeFormatRequestForReset)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxRequest* pRequest;
    FxUsbPipe* pUsbPipe;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Pipe %p, Request %p", Pipe, Request);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    status = pUsbPipe->FormatResetRequest(pRequest);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Pipe %p, Request %p = 0x%x",
                        Pipe, Request, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(PASSIVE_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeSendUrbSynchronously)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in_opt
    WDFREQUEST Request,
    __in_opt
    PWDF_REQUEST_SEND_OPTIONS RequestOptions,
    __in_xcount("union bug in SAL")
    PURB Urb
    )
{
    DDI_ENTRY();

    FxRequestBuffer buf;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbPipe* pUsbPipe;
    NTSTATUS status;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    FxUsbUrbContext context;
    FxSyncRequest request(pFxDriverGlobals, &context, Request);

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p, Urb %p", Pipe, Urb);

    FxPointerNotNull(pFxDriverGlobals, Urb);

    status = FxVerifierCheckIrqlLevel(pFxDriverGlobals, PASSIVE_LEVEL);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxValidateRequestOptions(pFxDriverGlobals, RequestOptions);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    buf.SetBuffer(Urb, 0);

    status = FxFormatUrbRequest(pFxDriverGlobals,
                                pUsbPipe,
                                request.m_TrueRequest,
                                &buf,
                                pUsbPipe->GetUrbType(),
                                pUsbPipe->GetUSBDHandle());

    if (NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
            "WDFUSBPIPE %p, WDFREQUEST %p being submitted",
            Pipe,  request.m_TrueRequest->GetTraceObjectHandle());

        status = pUsbPipe->SubmitSync(request.m_TrueRequest, RequestOptions);
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "WDFUSBPIPE %p, Urb %p, %!STATUS!",
                        Pipe, Urb, status);

    return status;
}

_Must_inspect_result_
__drv_maxIRQL(DISPATCH_LEVEL)
NTSTATUS
WDFAPI
WDFEXPORT(WdfUsbTargetPipeFormatRequestForUrb)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE Pipe,
    __in
    WDFREQUEST Request,
    __in
    WDFMEMORY UrbMemory,
    __in_opt
    PWDFMEMORY_OFFSET UrbOffsets
    )
{
    DDI_ENTRY();

    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxMemory* pMemory;
    FxUsbPipe* pUsbPipe;
    FxRequest* pRequest;
    FxRequestBuffer buf;
    NTSTATUS status;
    size_t bufferSize;

    FxObjectHandleGetPtrAndGlobals(GetFxDriverGlobals(DriverGlobals),
                                   Pipe,
                                   FX_TYPE_IO_TARGET_USB_PIPE,
                                   (PVOID*) &pUsbPipe,
                                   &pFxDriverGlobals);

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Pipe %p, Request %p, Memory %p",
                        Pipe, Request, UrbMemory);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         UrbMemory,
                         IFX_TYPE_MEMORY,
                         (PVOID*) &pMemory);

    FxObjectHandleGetPtr(pFxDriverGlobals,
                         Request,
                         FX_TYPE_REQUEST,
                         (PVOID*) &pRequest);

    status = pMemory->ValidateMemoryOffsets(UrbOffsets);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    bufferSize = pMemory->GetBufferSize();
    if (UrbOffsets != NULL && UrbOffsets->BufferOffset > 0) {
        bufferSize -= UrbOffsets->BufferOffset;
    }

    if (bufferSize < sizeof(_URB_HEADER)) {
        status = STATUS_INVALID_PARAMETER;
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "UrbMemory %p buffer size, %I64d, smaller then "
                            "_URB_HEADER, %!STATUS!", UrbMemory,
                            pMemory->GetBufferSize(), status);
        return status;
    }

    buf.SetMemory(pMemory, UrbOffsets);

    status = FxFormatUrbRequest(pFxDriverGlobals,
                                pUsbPipe,
                                pRequest,
                                &buf,
                                pUsbPipe->GetUrbType(),
                                pUsbPipe->GetUSBDHandle());

    if (NT_SUCCESS(status)) {
        FxUsbUrbContext* pContext;
        pContext = (FxUsbUrbContext*) pRequest->GetContext();

        pContext->SetUsbType(WdfUsbRequestTypePipeUrb);
        pContext->m_UsbParameters.Parameters.PipeUrb.Buffer = UrbMemory;
    }

    DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                        "Pipe %p, Request %p, Memory %p, status %!STATUS!",
                        Pipe, Request, UrbMemory, status);

    return status;
}

__drv_maxIRQL(DISPATCH_LEVEL)
USBD_PIPE_HANDLE
WDFAPI
WDFEXPORT(WdfUsbTargetPipeWdmGetPipeHandle)(
    __in
    PWDF_DRIVER_GLOBALS DriverGlobals,
    __in
    WDFUSBPIPE UsbPipe
    )
/*++

Routine Description:
    Returns the underlying WDM USBD pipe handle

Arguments:
    UsbPipe - the WDF pipe whose WDM handle will be returned

Return Value:
    valid handle value or NULL on error

  --*/
{
    DDI_ENTRY();

    FxUsbPipe* pUsbPipe;

    FxObjectHandleGetPtr(GetFxDriverGlobals(DriverGlobals),
                         UsbPipe,
                         FX_TYPE_IO_TARGET_USB_PIPE,
                         (PVOID*) &pUsbPipe);

    return pUsbPipe->WdmGetPipeHandle();
}

} // extern "C"
