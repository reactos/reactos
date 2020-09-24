/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIoTargetUm.cpp

Abstract:

    This module implements the IO Target APIs

Author:

Environment:

    kernel mode only

Revision History:

--*/


#include "..\..\FxTargetsShared.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "FxIoTargetUm.tmh"
#endif
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::InitModeSpecific(
    __in CfxDeviceBase* Device
    )
{
    NTSTATUS status;

    //
    // FxCREvent can fail in UMDF so it is initialized outside of constuctor
    // for UMDF. It always succeeds for KMDF so it gets initialized in
    // event's constructor.
    //

    status = m_SentIoEvent.Initialize(SynchronizationEvent, FALSE);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGIOTARGET,
                            "Could not initialize m_SentIoEvent event for "
                            "WFIOTARGET %p, %!STATUS!",
                            GetObjectHandle(), status);
        return status;
    }

    status = m_DisposeEventUm.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR,
                            TRACINGIOTARGET,
                            "Could not initialize m_DisposeEventUm event for "
                            "WFIOTARGET %p, %!STATUS!",
                            GetObjectHandle(), status);
        return status;
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::FormatIoRequest(
    __inout FxRequestBase* Request,
    __in UCHAR MajorCode,
    __in FxRequestBuffer* IoBuffer,
    __in_opt PLONGLONG DeviceOffset,
    __in_opt FxFileObject* FileObject
    )
{
    FxIoContext* pContext;
    NTSTATUS status;
    ULONG ioLength;
    FxIrp* irp;
    PVOID buffer;

    UNREFERENCED_PARAMETER(FileObject);

    ASSERT(MajorCode == IRP_MJ_WRITE || MajorCode == IRP_MJ_READ);

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Request->HasContextType(FX_RCT_IO)) {
        pContext = (FxIoContext*) Request->GetContext();
    }
    else {
        pContext = new(GetDriverGlobals()) FxIoContext();
        if (pContext == NULL) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "could not allocate context for request");

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        //
        // Since we can error out and return, remember the allocation before
        // we do anything so we can free it later.
        //
        Request->SetContext(pContext);
    }

    //
    // Save away any references to IFxMemory pointers that are passed
    //
    pContext->StoreAndReferenceMemory(IoBuffer);

    //
    // Setup irp stack
    //
    irp = Request->GetSubmitFxIrp();
    irp->ClearNextStackLocation();

    //
    // copy File object and flags
    //
    CopyFileObjectAndFlags(Request);

    irp->SetMajorFunction(MajorCode);
    pContext->m_MajorFunction = MajorCode;

    ioLength = IoBuffer->GetBufferLength();

    status = IoBuffer->GetBuffer(&buffer);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve i/o buffer, %!STATUS!",
            status);
        goto exit;
    }

    //
    // Since we don't support buffer transformations (buffered->Direct->Neither)
    // we are analogous to "Neither" method in KMDF
    // in which case we just set the Irp buffer to the buffer that is passed in
    //
    if (IRP_MJ_READ == MajorCode) {
        pContext->SwapIrpBuffer(Request,
                                0,
                                NULL,
                                ioLength,
                                buffer);

        irp->GetIoIrp()->SetReadParametersForNextStackLocation(
                    ioLength,
                    DeviceOffset,
                    0
                    );
    }
    else if (IRP_MJ_WRITE == MajorCode) {
        pContext->SwapIrpBuffer(Request,
                                ioLength,
                                buffer,
                                0,
                                NULL);
        irp->GetIoIrp()->SetWriteParametersForNextStackLocation(
                    ioLength,
                    DeviceOffset,
                    0
                    );
    }
    /*
    else if (WdfRequestQueryInformation == RequestType)
    {
        pContext->SwapIrpBuffer(pRequest,
                                0,
                                NULL,
                                ioLength,
                                buffer);
    }
    else if (WdfRequestSetInformation == RequestType)
    {
        pContext->SwapIrpBuffer(pRequest,
                                ioLength,
                                buffer,
                                0,
                                NULL);
    }
    */
    else {
        pContext->SwapIrpBuffer(Request, 0, NULL, 0, NULL);
    }

exit:

    if (NT_SUCCESS(status)) {
        Request->VerifierSetFormatted();
    }
    else {
        Request->ContextReleaseAndRestore();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxIoTarget::FormatIoctlRequest(
    __in FxRequestBase* Request,
    __in ULONG Ioctl,
    __in BOOLEAN Internal,
    __in FxRequestBuffer* InputBuffer,
    __in FxRequestBuffer* OutputBuffer,
    __in_opt FxFileObject* FileObject
    )
{
    FxIoContext* pContext;
    NTSTATUS status;
    ULONG inLength, outLength;
    FxIrp* irp;
    PVOID inputBuffer;
    PVOID outputBuffer;

    UNREFERENCED_PARAMETER(FileObject);

    irp = Request->GetSubmitFxIrp();

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (Request->HasContextType(FX_RCT_IO)) {
        pContext = (FxIoContext*) Request->GetContext();
    }
    else {
        pContext = new(GetDriverGlobals()) FxIoContext();
        if (pContext == NULL) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not allocate context for request");

            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Request->SetContext(pContext);
    }

    inLength = InputBuffer->GetBufferLength();
    outLength = OutputBuffer->GetBufferLength();

    //
    // Capture irp buffers in context and set driver-provided buffers in the irp
    //
    status = InputBuffer->GetBuffer(&inputBuffer);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve input buffer, %!STATUS!",
            status);
        goto exit;
    }

    status = OutputBuffer->GetBuffer(&outputBuffer);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not retrieve output buffer, %!STATUS!",
            status);
        goto exit;
    }

    //
    // Save away any references to IFxMemory pointers that are passed
    //
    pContext->StoreAndReferenceMemory(InputBuffer);
    pContext->StoreAndReferenceOtherMemory(OutputBuffer);
    pContext->m_MajorFunction = IRP_MJ_DEVICE_CONTROL;

    //
    // Format next stack location
    //
    irp->ClearNextStackLocation();
    irp->SetMajorFunction(IRP_MJ_DEVICE_CONTROL);

    //
    // copy File object and flags
    //
    CopyFileObjectAndFlags(Request);

    irp->GetIoIrp()->SetDeviceIoControlParametersForNextStackLocation(
                    Ioctl,
                    inLength,
                    outLength
                    );

    pContext->SwapIrpBuffer(Request,
                            InputBuffer->GetBufferLength(),
                            inputBuffer,
                            OutputBuffer->GetBufferLength(),
                            outputBuffer);
exit:

    if (NT_SUCCESS(status)) {
        Request->VerifierSetFormatted();
    }
    else {
        Request->ContextReleaseAndRestore();
    }

    return status;;
}


