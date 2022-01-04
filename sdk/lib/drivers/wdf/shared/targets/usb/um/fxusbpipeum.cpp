//
//    Copyright (C) Microsoft.  All rights reserved.
//
#include "fxusbpch.hpp"

extern "C" {
#include "FxUsbPipeUm.tmh"
}

#include "Fxglobals.h"

VOID
FxUsbPipeRequestContext::SetInfo(
    __in WDF_USB_REQUEST_TYPE Type,
    __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
    __in UCHAR PipeId,
    __in USHORT Function
    )
{
    RtlZeroMemory(&m_UmUrb, sizeof(m_UmUrb));

    m_UmUrb.UmUrbPipeRequest.Hdr.InterfaceHandle = WinUsbHandle;
    m_UmUrb.UmUrbPipeRequest.Hdr.Function = Function;
    m_UmUrb.UmUrbPipeRequest.Hdr.Length = sizeof(m_UmUrb.UmUrbPipeRequest);

    m_UmUrb.UmUrbPipeRequest.PipeID = PipeId;

    SetUsbType(Type);
}

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
    RtlZeroMemory(&m_UmUrb, sizeof(m_UmUrb));

    m_UmUrb.UmUrbHeader.Function = UMURB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER;
    m_UmUrb.UmUrbHeader.Length = sizeof(_UMURB_BULK_OR_INTERRUPT_TRANSFER);

    FxUsbRequestContext::StoreAndReferenceMemory(Buffer); // __super call

    Buffer->AssignValues(&m_UmUrb.UmUrbBulkOrInterruptTransfer.TransferBuffer,
                         NULL,
                         &m_UmUrb.UmUrbBulkOrInterruptTransfer.TransferBufferLength);
}

VOID
FxUsbPipeContinuousReader::_ReadWorkItem(
    __in MdDeviceObject /*DeviceObject*/,
    __in_opt PVOID Context
    )
{
    FxUsbPipeRepeatReader * pRepeater;
    pRepeater = (FxUsbPipeRepeatReader *)Context;

    pRepeater->RequestIrp->Forward();
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

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    if (TotalBufferLength <= MAXUSHORT) {
        m_Lookaside = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
            FxNPagedLookasideList(pFxDriverGlobals, pFxDriverGlobals->Tag);
    }
    else {
        m_Lookaside = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
            FxNPagedLookasideListFromPool(pFxDriverGlobals, pFxDriverGlobals->Tag);
    }
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    m_Lookaside = new(pFxDriverGlobals, WDF_NO_OBJECT_ATTRIBUTES)
            FxNPagedLookasideList(pFxDriverGlobals, pFxDriverGlobals->Tag);
#endif

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

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        KeInitializeDpc(&pRepeater->Dpc, _FxUsbPipeContinuousReadDpc, NULL);
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
        pRepeater->m_ReadWorkItem.Allocate(m_Pipe->m_Device->GetDeviceObject());
#endif

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

VOID
FxUsbPipe::InitPipe(
    __in PWINUSB_PIPE_INFORMATION PipeInfo,
    __in UCHAR InterfaceNumber,
    __in FxUsbInterface* UsbInterface
    )
{
    RtlCopyMemory(&m_PipeInformationUm, PipeInfo, sizeof(m_PipeInformationUm));
    m_InterfaceNumber = InterfaceNumber;

    if (m_UsbInterface != NULL) {
        m_UsbInterface->RELEASE(this);
        m_UsbInterface = NULL;
    }

    m_UsbInterface = UsbInterface;
    m_UsbInterface->ADDREF(this);
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
            (bufferSize % m_PipeInformationUm.MaximumPacketSize) != 0) {
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
        pContext = new(GetDriverGlobals()) FxUsbPipeTransferContext(FxUrbTypeLegacy);
        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Request->SetContext(pContext);
    }

    pContext->StoreAndReferenceMemory(Buffer);

    pContext->m_UmUrb.UmUrbHeader.InterfaceHandle = m_UsbInterface->m_WinUsbHandle;

    pContext->m_UmUrb.UmUrbBulkOrInterruptTransfer.PipeID = m_PipeInformationUm.PipeId;
    pContext->m_UmUrb.UmUrbBulkOrInterruptTransfer.InPipe = IsInEndpoint();

    FxUsbUmFormatRequest(Request, &pContext->m_UmUrb.UmUrbHeader, m_UsbDevice->m_pHostTargetFile);

    return STATUS_SUCCESS;
}

VOID
FxUsbPipe::GetInformation(
    __out PWDF_USB_PIPE_INFORMATION PipeInformation
    )
{




    PipeInformation->MaximumPacketSize = m_PipeInformationUm.MaximumPacketSize;
    PipeInformation->EndpointAddress = m_PipeInformationUm.PipeId;
    PipeInformation->Interval = m_PipeInformationUm.Interval;
    PipeInformation->PipeType = _UsbdPipeTypeToWdf(m_PipeInformationUm.PipeType);
    PipeInformation->SettingIndex = m_UsbInterface->GetConfiguredSettingIndex();
}

WDF_USB_PIPE_TYPE
FxUsbPipe::GetType(
    VOID
    )
{
    return _UsbdPipeTypeToWdf(m_PipeInformationUm.PipeType);
}

BOOLEAN
FxUsbPipe::IsType(
    __in WDF_USB_PIPE_TYPE Type
    )
{
    return _UsbdPipeTypeToWdf(m_PipeInformationUm.PipeType) == Type ? TRUE : FALSE;
}

