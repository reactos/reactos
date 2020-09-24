//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbPipeKm.tmh"
}

#include "Fxglobals.h"

VOID
FxUsbPipeTransferContext::StoreAndReferenceMemory(
    __in FxRequestBuffer* Buffer
    )
/*++

Routine Description:
    virtual function which stores and references memory if it is an FxObject
    and then fills in the appropriate fields in the URB.

Arguments:
    Buffer - union which can be many types of memory

Return Value:
    None

  --*/
{
    RtlZeroMemory(m_Urb, sizeof(*m_Urb));

    m_Urb->Hdr.Function = URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    m_Urb->Hdr.Length = sizeof(*m_Urb);

    __super::StoreAndReferenceMemory(Buffer);

    Buffer->AssignValues(&m_Urb->TransferBuffer,
                         &m_Urb->TransferBufferMDL,
                         &m_Urb->TransferBufferLength);

    //
    // If we have built a partial MDL, use that instead.  TransferBufferLength
    // is still valid because the Offsets or length in Buffer will have been
    // used to create this PartialMdl by the caller.
    //
    if (m_PartialMdl != NULL) {
        m_Urb->TransferBufferMDL = m_PartialMdl;
    }
}

__drv_functionClass(KDEFERRED_ROUTINE)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_requiresIRQL(DISPATCH_LEVEL)
__drv_sameIRQL
VOID
FxUsbPipeContinuousReader::_FxUsbPipeContinuousReadDpc(
    __in struct _KDPC *Dpc,
    __in_opt PVOID DeferredContext,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
{
    FxUsbPipeRepeatReader* pRepeater;
    FxUsbPipe* pPipe;

    UNREFERENCED_PARAMETER(DeferredContext);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    #pragma prefast(push);


    pRepeater = CONTAINING_RECORD(Dpc, FxUsbPipeRepeatReader, Dpc);
    pPipe = pRepeater->Parent->m_Pipe;

    //
    // Ignore the return value because once we have sent the request, we
    // want all processing to be done in the completion routine.
    //
    (void) IoCallDriver(pPipe->m_TargetDevice,
                        pRepeater->Request->GetSubmitIrp());
    #pragma prefast(pop);
}

_Must_inspect_result_
NTSTATUS
FxUsbPipeContinuousReader::Config(
    __in PWDF_USB_CONTINUOUS_READER_CONFIG Config,
    __in size_t TotalBufferLength
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    WDF_OBJECT_ATTRIBUTES attributes;
    NTSTATUS status;
    LONG i;

    pFxDriverGlobals = m_Pipe->GetDriverGlobals();

    if (TotalBufferLength <= MAXUSHORT) {
        m_Lookaside = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
            FxNPagedLookasideList(pFxDriverGlobals, pFxDriverGlobals->Tag);
    }
    else {
        m_Lookaside = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
            FxNPagedLookasideListFromPool(pFxDriverGlobals, pFxDriverGlobals->Tag);
    }

    if (m_Lookaside == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (Config->BufferAttributes == NULL) {
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    }
    else {
        RtlCopyMemory(&attributes,
                      Config->BufferAttributes,
                      sizeof(WDF_OBJECT_ATTRIBUTES));
    }

    //
    // By specifying the loookaside as the parent for the memory objects that
    // will be created, when we destroy the lookaside list, we will destroy any
    // outstanding memory objects that have been allocated.  This can happen if
    // we initialize the repeater, but never send any i/o.  (Normally the
    // memory object would be freed when the read completes.)
    //
    attributes.ParentObject = m_Lookaside->GetObjectHandle();

    status = m_Lookaside->Initialize(TotalBufferLength, &attributes);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = FxSystemWorkItem::_Create(pFxDriverGlobals,
                                      m_Pipe->m_Device->GetDeviceObject(),
                                      &m_WorkItem
                                      );
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIO,
                            "Could not allocate workitem: %!STATUS!", status);
        return status;
    }

    m_Offsets.BufferLength = Config->TransferLength;
    m_Offsets.BufferOffset = Config->HeaderLength;

    for (i = 0; i < m_NumReaders; i++) {
        FxUsbPipeRepeatReader* pRepeater;

        pRepeater = &m_Readers[i];

        pRepeater->Parent = this;

        KeInitializeDpc(&pRepeater->Dpc, _FxUsbPipeContinuousReadDpc, NULL);

        //
        // This will allocate the PIRP
        //
        status = FxRequest::_Create(pFxDriverGlobals,
                                    WDF_NO_OBJECT_ATTRIBUTES,
                                    NULL,
                                    m_Pipe,
                                    FxRequestOwnsIrp,
                                    FxRequestConstructorCallerIsFx,
                                    &pRepeater->Request);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        pRepeater->RequestIrp = pRepeater->Request->GetSubmitIrp();

        //
        // Initialize the event before FormatRepeater clears it
        //
        status = pRepeater->ReadCompletedEvent.Initialize(NotificationEvent, TRUE);

        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGIOTARGET,
                "Could not initialize ReadCompletedEvent: %!STATUS!",
                status);

            return status;
        }

        //
        // This will allocate the context
        //
        status = FormatRepeater(pRepeater);

        if (!NT_SUCCESS(status)) {
            return status;
        }
    }

    return STATUS_SUCCESS;
}

_Must_inspect_result_
NTSTATUS
FxUsbPipe::FormatTransferRequest(
    __in FxRequestBase* Request,
    __in FxRequestBuffer* Buffer,
    __in ULONG TransferFlags
    )
{
    FxUsbPipeTransferContext* pContext;
    NTSTATUS status;
    size_t bufferSize;
    ULONG dummyLength;
    FX_URB_TYPE urbType;

    //
    // Make sure request is for the right type
    //
    if (!(IsType(WdfUsbPipeTypeBulk) || IsType(WdfUsbPipeTypeInterrupt))) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFUSBPIPE %p not the right type, %!STATUS!",
                            GetHandle(), status);

        return status;
    }

    bufferSize = Buffer->GetBufferLength();

    status = RtlSizeTToULong(bufferSize, &dummyLength);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "WDFUSBPIPE %p, buffer size truncated, %!STATUS!",
                            GetHandle(), status);
        return status;
    }

    //
    // On reads, check to make sure the read in value is an integral number of
    // packet sizes
    //
    if (TransferFlags & USBD_TRANSFER_DIRECTION_IN) {
        if (IsInEndpoint() == FALSE) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "Pipe %p, sending __in transaction on a __out endpoint",
                                this);

            return STATUS_INVALID_DEVICE_REQUEST;
        }

        if (m_CheckPacketSize &&
            (bufferSize % m_PipeInformation.MaximumPacketSize) != 0) {
            return STATUS_INVALID_BUFFER_SIZE;
        }
    }
    else {
        if (IsOutEndpoint() == FALSE) {
            DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                                "Pipe %p, sending __out transaction on an __in endpoint",
                                this);

            return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    status = Request->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Pipe %p, Request %p, setting target failed, "
                            "status %!STATUS!", this, Request, status);

        return status;
    }

    if (Request->HasContextType(FX_RCT_USB_PIPE_XFER)) {
        pContext = (FxUsbPipeTransferContext*) Request->GetContext();
    }
    else {
        urbType = m_UsbDevice->GetFxUrbTypeForRequest(Request);

        pContext = new(GetDriverGlobals()) FxUsbPipeTransferContext(urbType);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        if (urbType == FxUrbTypeUsbdAllocated) {
            status = pContext->AllocateUrb(m_USBDHandle);
            if (!NT_SUCCESS(status)) {
                delete pContext;
                return status;
            }
            //
            // Since the AllocateUrb routine calls USBD_xxxUrbAllocate APIs to allocate an Urb, it is
            // important to release those resorces before the devnode is removed. Those
            // resoruces are removed at the time Request is disposed.
            //
            Request->EnableContextDisposeNotification();
        }

        Request->SetContext(pContext);
    }

    //
    // Always set the memory after determining the context.  This way we can
    // free a previously referenced memory object if necessary.
    //
    if (Buffer->HasMdl()) {
        PMDL pMdl;

        pMdl=NULL;
        ASSERT(pContext->m_PartialMdl == NULL);

        //
        // If it is an __in endpoint, the buffer will be written to by the
        // controller, so request IoWriteAccess locking.
        //
        status = Buffer->GetOrAllocateMdl(
            GetDriverGlobals(),
            &pMdl,
            &pContext->m_PartialMdl,
            &pContext->m_UnlockPages,
            IsInEndpoint() ? IoWriteAccess : IoReadAccess);

        if (!NT_SUCCESS(status)) {
            return status;
        }

        ASSERT(pMdl != NULL);
    }

    pContext->StoreAndReferenceMemory(Buffer);

    pContext->SetUrbInfo(m_PipeInformation.PipeHandle, TransferFlags);

    if (pContext->m_Urb == &pContext->m_UrbLegacy) {
        urbType = FxUrbTypeLegacy;
    }
    else {
        urbType = FxUrbTypeUsbdAllocated;
    }

    FxFormatUsbRequest(Request, (PURB)pContext->m_Urb, urbType, m_USBDHandle);

    return STATUS_SUCCESS;
}

VOID
FxUsbPipe::GetInformation(
    __out PWDF_USB_PIPE_INFORMATION PipeInformation
    )
{
    //
    // Do a field by field copy for the WDF structure, since fields could change.
    //
    PipeInformation->MaximumPacketSize = m_PipeInformation.MaximumPacketSize;
    PipeInformation->EndpointAddress = m_PipeInformation.EndpointAddress;
    PipeInformation->Interval = m_PipeInformation.Interval;
    PipeInformation->PipeType = _UsbdPipeTypeToWdf(m_PipeInformation.PipeType);
    PipeInformation->MaximumTransferSize = m_PipeInformation.MaximumTransferSize;
    PipeInformation->SettingIndex = m_UsbInterface->GetConfiguredSettingIndex();
}

WDF_USB_PIPE_TYPE
FxUsbPipe::GetType(
    VOID
    )
{
    return _UsbdPipeTypeToWdf(m_PipeInformation.PipeType);
}

BOOLEAN
FxUsbPipe::IsType(
    __in WDF_USB_PIPE_TYPE Type
    )
{
    return _UsbdPipeTypeToWdf(m_PipeInformation.PipeType) == Type ? TRUE : FALSE;
}

