/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxUsbDevice.cpp

Abstract:

Author:

Environment:

    kernel mode only

Revision History:

--*/

extern "C" {
#include <initguid.h>
}

#include "fxusbpch.hpp"


extern "C" {
#include "FxUsbDevice.tmh"
}

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
#define UCHAR_MAX (0xff)
#endif

FxUsbDeviceControlContext::FxUsbDeviceControlContext(
    __in FX_URB_TYPE FxUrbType
    ) :
    FxUsbRequestContext(FX_RCT_USB_CONTROL_REQUEST)
{
    m_PartialMdl = NULL;
    m_UnlockPages = FALSE;
    m_USBDHandle = NULL;

    if (FxUrbType == FxUrbTypeLegacy) {
        m_Urb = &m_UrbLegacy;
    }
    else {
        m_Urb = NULL;
    }
}

FxUsbDeviceControlContext::~FxUsbDeviceControlContext(
    VOID
    )
{
    if (m_Urb && (m_Urb != &m_UrbLegacy)) {
        USBD_UrbFree(m_USBDHandle, (PURB)m_Urb);
    }
    m_Urb = NULL;
    m_USBDHandle = NULL;
}

__checkReturn
NTSTATUS
FxUsbDeviceControlContext::AllocateUrb(
    __in USBD_HANDLE USBDHandle
    )
{
    NTSTATUS status;

    ASSERT(USBDHandle != NULL);
    ASSERT(m_Urb == NULL);

    status = USBD_UrbAllocate(USBDHandle, (PURB*)&m_Urb);

    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    m_USBDHandle = USBDHandle;

Done:
    return status;
}

VOID
FxUsbDeviceControlContext::Dispose(
    VOID
    )
{
    if (m_Urb && (m_Urb != &m_UrbLegacy)){
        USBD_UrbFree(m_USBDHandle, (PURB) m_Urb);
        m_Urb = NULL;
        m_USBDHandle = NULL;
    }
}

VOID
FxUsbDeviceControlContext::CopyParameters(
    __in FxRequestBase* Request
    )
{
#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    m_CompletionParams.IoStatus.Information = m_Urb->TransferBufferLength;
    m_UsbParameters.Parameters.DeviceControlTransfer.Length = m_Urb->TransferBufferLength;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    m_CompletionParams.IoStatus.Information = m_UmUrb.UmUrbControlTransfer.TransferBufferLength;
    m_UsbParameters.Parameters.DeviceControlTransfer.Length = m_UmUrb.UmUrbControlTransfer.TransferBufferLength;
#endif
    FxUsbRequestContext::CopyParameters(Request); // __super call
}

VOID
FxUsbDeviceControlContext::ReleaseAndRestore(
    __in FxRequestBase* Request
    )
{
    //
    // Check now because Init will NULL out the field
    //
    if (m_PartialMdl != NULL) {
        if (m_UnlockPages) {
            Mx::MxUnlockPages(m_PartialMdl);
            m_UnlockPages = FALSE;
        }

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
        FxMdlFree(Request->GetDriverGlobals(), m_PartialMdl);
#endif
        m_PartialMdl = NULL;
    }

    FxUsbRequestContext::ReleaseAndRestore(Request); // __super call
}

USBD_STATUS
FxUsbDeviceControlContext::GetUsbdStatus(
    VOID
    )
{
    return m_Urb->Hdr.Status;
}

FxUsbDeviceStringContext::FxUsbDeviceStringContext(
    __in FX_URB_TYPE FxUrbType
    ) :
    FxUsbRequestContext(FX_RCT_USB_STRING_REQUEST)
{
    m_USBDHandle = NULL;
    m_StringDescriptor = NULL;
    m_StringDescriptorLength = 0;
    RtlZeroMemory(&m_UrbLegacy, sizeof(m_UrbLegacy));

    if (FxUrbType == FxUrbTypeLegacy) {
        m_Urb = &m_UrbLegacy;
        m_Urb->Hdr.Function =  URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
        m_Urb->Hdr.Length = sizeof(*m_Urb);
        m_Urb->DescriptorType = USB_STRING_DESCRIPTOR_TYPE;
    }
    else {
        m_Urb = NULL;
    }
}

FxUsbDeviceStringContext::~FxUsbDeviceStringContext(
    VOID
    )
{
    if (m_StringDescriptor != NULL) {
        FxPoolFree(m_StringDescriptor);
        m_StringDescriptor = NULL;
    }

    if (m_Urb && (m_Urb != &m_UrbLegacy)){
        USBD_UrbFree(m_USBDHandle, (PURB) m_Urb);
    }
    m_Urb = NULL;
    m_USBDHandle = NULL;
}

__checkReturn
NTSTATUS
FxUsbDeviceStringContext::AllocateUrb(
    __in USBD_HANDLE USBDHandle
    )
{
    NTSTATUS status;

    ASSERT(USBDHandle != NULL);
    ASSERT(m_Urb == NULL);

    status = USBD_UrbAllocate(USBDHandle, (PURB*)&m_Urb);

    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    m_USBDHandle = USBDHandle;

    m_Urb->Hdr.Function =  URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE;
    m_Urb->Hdr.Length = sizeof(*m_Urb);
    m_Urb->DescriptorType = USB_STRING_DESCRIPTOR_TYPE;

Done:
    return status;
}

VOID
FxUsbDeviceStringContext::Dispose(
    VOID
    )
{
    if (m_Urb && (m_Urb != &m_UrbLegacy)){
        USBD_UrbFree(m_USBDHandle, (PURB) m_Urb);
        m_Urb = NULL;
        m_USBDHandle = NULL;
    }

}

VOID
FxUsbDeviceStringContext::CopyParameters(
    __in FxRequestBase* Request
    )
{
    //
    // Make sure we got an even number of bytes and that we got a header
    //
    if ((m_StringDescriptor->bLength & 0x1) ||
        m_StringDescriptor->bLength < sizeof(USB_COMMON_DESCRIPTOR)) {
        m_CompletionParams.IoStatus.Status = STATUS_DEVICE_DATA_ERROR;
    }
    else if (NT_SUCCESS(Request->GetSubmitFxIrp()->GetStatus())) {
        //
        // No matter what, indicate the required size to the caller
        //
        m_UsbParameters.Parameters.DeviceString.RequiredSize =
            m_StringDescriptor->bLength - sizeof(USB_COMMON_DESCRIPTOR);

        if (m_UsbParameters.Parameters.DeviceString.RequiredSize >
                                            m_RequestMemory->GetBufferSize()) {
            //
            // Too much string to fit into the buffer supplied by the client.
            // Morph the status into a warning.  Copy as much as we can.
            //
            m_CompletionParams.IoStatus.Status = STATUS_BUFFER_OVERFLOW;
            RtlCopyMemory(m_RequestMemory->GetBuffer(),
                          &m_StringDescriptor->bString[0],
                          m_RequestMemory->GetBufferSize());
        }
        else {
            //
            // Everything fits, copy it over
            //
            m_CompletionParams.IoStatus.Information =
                m_UsbParameters.Parameters.DeviceString.RequiredSize;

            RtlCopyMemory(m_RequestMemory->GetBuffer(),
                          &m_StringDescriptor->bString[0],
                          m_UsbParameters.Parameters.DeviceString.RequiredSize);
        }
    }

    FxUsbRequestContext::CopyParameters(Request); // __super call
}

VOID
FxUsbDeviceStringContext::SetUrbInfo(
    __in UCHAR StringIndex,
    __in USHORT LangID
    )
{
    SetUsbType(WdfUsbRequestTypeDeviceString);

    ASSERT(m_StringDescriptor != NULL && m_StringDescriptorLength != 0);

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    m_Urb->TransferBuffer = m_StringDescriptor;
    m_Urb->TransferBufferLength = m_StringDescriptorLength;

    m_Urb->Index = StringIndex;
    m_Urb->LanguageId = LangID;
#elif (FX_CORE_MODE == FX_CORE_USER_MODE)
    m_UmUrb.UmUrbDescriptorRequest.Buffer = m_StringDescriptor;
    m_UmUrb.UmUrbDescriptorRequest.BufferLength = m_StringDescriptorLength;

    m_UmUrb.UmUrbDescriptorRequest.Index = StringIndex;
    m_UmUrb.UmUrbDescriptorRequest.LanguageID = LangID;
#endif
}

USBD_STATUS
FxUsbDeviceStringContext::GetUsbdStatus(
    VOID
    )
{
    return m_Urb->Hdr.Status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDeviceStringContext::AllocateDescriptor(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in size_t BufferSize
    )
{
    PUSB_STRING_DESCRIPTOR pDescriptor;
    size_t length;
    NTSTATUS status;

    if (BufferSize <= m_StringDescriptorLength) {
        return STATUS_SUCCESS;
    }

    length = sizeof(USB_STRING_DESCRIPTOR) - sizeof(pDescriptor->bString[0]) +
             BufferSize;

    pDescriptor = (PUSB_STRING_DESCRIPTOR) FxPoolAllocate(
        FxDriverGlobals,
        NonPagedPool,
        length);

    if (pDescriptor == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (m_StringDescriptor != NULL) {
        FxPoolFree(m_StringDescriptor);
    }

    RtlZeroMemory(pDescriptor, length);

    m_StringDescriptor = pDescriptor;

    status = RtlSizeTToULong(length, &m_StringDescriptorLength);

    return status;
}

FxUsbUrb::FxUsbUrb(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in USBD_HANDLE USBDHandle,
    __in_bcount(BufferSize) PVOID Buffer,
    __in size_t BufferSize
    ) :
    FxMemoryBufferPreallocated(FxDriverGlobals, sizeof(*this), Buffer, BufferSize),
    m_USBDHandle(USBDHandle)
{
    MarkDisposeOverride();
}

FxUsbUrb::~FxUsbUrb()
{
}

BOOLEAN
FxUsbUrb::Dispose(
    VOID
    )
{
    ASSERT(m_USBDHandle != NULL);
    ASSERT(m_pBuffer != NULL);
    USBD_UrbFree(m_USBDHandle, (PURB)m_pBuffer);
    m_pBuffer = NULL;
    m_USBDHandle = NULL;

    return FxMemoryBufferPreallocated::Dispose(); // __super call
}

FxUsbDevice::FxUsbDevice(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxIoTarget(FxDriverGlobals, sizeof(FxUsbDevice), FX_TYPE_IO_TARGET_USB_DEVICE)
{
    RtlZeroMemory(&m_DeviceDescriptor, sizeof(m_DeviceDescriptor));
    RtlZeroMemory(&m_UsbdVersionInformation, sizeof(m_UsbdVersionInformation));

    m_OnUSBD = FALSE;
    m_Interfaces = NULL;;
    m_NumInterfaces = 0;

    m_Traits = 0;
    m_HcdPortCapabilities = 0;
    m_ControlPipe = NULL;
    m_QueryBusTime = NULL;
    m_BusInterfaceContext = NULL;
    m_BusInterfaceDereference = NULL;
    m_ConfigHandle = NULL;
    m_ConfigDescriptor = NULL;

    m_MismatchedInterfacesInConfigDescriptor = FALSE;

    m_USBDHandle = NULL;
    m_UrbType = FxUrbTypeLegacy;

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    m_pHostTargetFile = NULL;
    m_WinUsbHandle = NULL;
#endif

    MarkDisposeOverride(ObjectDoNotLock);
}

BOOLEAN
FxUsbDevice::Dispose(
    VOID
    )
{
#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    KeFlushQueuedDpcs();
#endif

    if (m_USBDHandle) {
        USBD_CloseHandle(m_USBDHandle);
        m_USBDHandle = NULL;
    }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    IWudfDevice* device = NULL;
    IWudfDeviceStack* devstack = NULL;

    device = m_DeviceBase->GetDeviceObject();
    devstack = device->GetDeviceStackInterface();

    if (m_pHostTargetFile) {
        devstack->CloseFile(m_pHostTargetFile);
        SAFE_RELEASE(m_pHostTargetFile);
    }
#endif

    return FxIoTarget::Dispose(); // __super call
}

FxUsbDevice::~FxUsbDevice()
{
    UCHAR i;

    if (m_BusInterfaceDereference != NULL) {
        m_BusInterfaceDereference(m_BusInterfaceContext);
        m_BusInterfaceDereference = NULL;
    }

    if (m_ConfigDescriptor != NULL) {
        FxPoolFree(m_ConfigDescriptor);
        m_ConfigDescriptor = NULL;
    }

    for (i = 0; i < m_NumInterfaces; i++) {
        ASSERT(m_Interfaces[i] == NULL);
    }

    if (m_Interfaces != NULL){
        FxPoolFree(m_Interfaces);
        m_Interfaces = NULL;
    }

    m_NumInterfaces = 0;
}

VOID
FxUsbDevice::RemoveDeletedInterface(
    __in FxUsbInterface* Interface
    )
{
    UCHAR i;

    if (m_Interfaces == NULL) {
        return;
    }

    for (i = 0; i < m_NumInterfaces; i++) {
        if (m_Interfaces[i] == Interface) {
            m_Interfaces[i] = NULL;
            return;
        }
    }
}

VOID
FxUsbDevice::GetInformation(
    __out PWDF_USB_DEVICE_INFORMATION Information
    )
{
    Information->Traits = m_Traits;
    Information->HcdPortCapabilities = m_HcdPortCapabilities;

    RtlCopyMemory(&Information->UsbdVersionInformation,
                  &m_UsbdVersionInformation,
                  sizeof(m_UsbdVersionInformation));
}

ULONG
FxUsbDevice::GetDefaultMaxTransferSize(
    VOID
    )
/*++

Routine Description:
    Determines the default max transfer size based on the usb host controller
    and OS we are running on.  What it boils down to is that on XP and later
    the usb core ignores the default max transfer size, but it does use the
    value on Windows 2000.  To make life fun, there is only one definition of
    USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE whose value changes depending on header
    versioning.  Since we are versioned for the latest OS, we do not pick up the
    Win2k value by using the #define, rather we have to use the value that we
    *would* have picked up if we were header versioned for Win2k.

    NOTE:  we could be on win2k with a usbport serviced stack.  in this case,
           usbport doesn't care about max transfer sizes

Arguments:
    None

Return Value:
    usb core and OS appropriate default max transfer size.

  --*/
{
    //
    // On a usbport serviced stack (which can be running on Win2k) or on a
    // usbd stack on XP and later.  In any case, always use the current max
    // transfer size definition.
    //
    return USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::Start(
    VOID
    )
{
    NTSTATUS status;

    status = FxIoTarget::Start();

    if (NT_SUCCESS(status)) {
        FxRequestBase* pRequest;
        LIST_ENTRY head, *ple;
        ULONG i, iInterface;
        FxUsbInterface *pUsbInterface;
        KIRQL irql;

        InitializeListHead(&head);

        Lock(&irql);

        //
        // Iterate over all of the interfaces.  For each pipe on each interface,
        // grab all pended i/o for later submission.
        //
        for (iInterface = 0; iInterface < m_NumInterfaces; iInterface++){

            pUsbInterface = m_Interfaces[iInterface];

            for (i = 0; i < pUsbInterface->m_NumberOfConfiguredPipes; i++) {
                pUsbInterface->m_ConfiguredPipes[i]->GotoStartState(&head);
            }
        }
        Unlock(irql);

        //
        // Since we are going to reference the FxUsbPipe's outside of the
        // lock and the interface can go away in the meantime, add a reference
        // (multiple times perhaps) to each FxUsbPipe to make sure it sticks
        // around.
        //
        for (ple = head.Flink; ple != &head; ple = ple->Flink) {
            pRequest = FxRequestBase::_FromListEntry(ple);
            pRequest->GetTarget()->ADDREF(this);
        }

        //
        // Drain the list of pended requests.
        //
        while (!IsListEmpty(&head)) {
            FxIoTarget* pTarget;

            ple = RemoveHeadList(&head);

            pRequest = FxRequestBase::_FromListEntry(ple);

            pTarget = pRequest->GetTarget();

            pTarget->SubmitPendedRequest(pRequest);

            //
            // Release the reference taken above when accumulating pended i/o.
            //
            pTarget->RELEASE(this);
        }
    }

    return status;
}

#define STOP_TAG (PVOID) 'pots'

VOID
FxUsbDevice::Stop(
    __in WDF_IO_TARGET_SENT_IO_ACTION Action
    )
{
    FxUsbInterface *pUsbInterface;
    SINGLE_LIST_ENTRY head;
    ULONG iPipe, iInterface;
    KIRQL irql;

    head.Next = NULL;

    //
    // Stop all of our own I/O first
    //
    FxIoTarget::Stop(Action);

    //
    // if we are just canceling i/o, then we just acquire the spin lock b/c
    // we can be called at dispatch level for this action code.
    //
    if (Action != WdfIoTargetLeaveSentIoPending) {
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

        AcquireInterfaceIterationLock();
    }

    //
    // Since we don't have to synchronize on the I/O already sent, just set
    // each pipe's state to stop.
    //
    Lock(&irql);

    for (iInterface = 0; iInterface < m_NumInterfaces; iInterface++) {
        pUsbInterface = m_Interfaces[iInterface];

        if (pUsbInterface->m_ConfiguredPipes != NULL) {
            for (iPipe = 0;
                 iPipe < pUsbInterface->m_NumberOfConfiguredPipes;
                 iPipe++) {

                if (pUsbInterface->m_ConfiguredPipes[iPipe] != NULL) {
                    BOOLEAN wait;

                    wait = FALSE;

                    pUsbInterface->m_ConfiguredPipes[iPipe]->GotoStopState(
                        Action,
                        &head,
                        &wait,
                        TRUE
                        );
                }
            }
        }
    }
    Unlock(irql);

    //
    // If we are leaving sent IO pending, the io target will set the IO
    // completion event during stop even if there is still outstanding IO.
    //

    if (head.Next != NULL) {
        _CancelSentRequests(&head);
    }

    for (iInterface = 0; iInterface < m_NumInterfaces;  iInterface++) {
        pUsbInterface = m_Interfaces[iInterface];

        if (pUsbInterface->m_ConfiguredPipes != NULL) {
            //
            // Iterate over the pipes and clean each one up
            //
            for (iPipe = 0;
                 iPipe < pUsbInterface->m_NumberOfConfiguredPipes;
                 iPipe++) {
                //
                // Same reason as above
                //
                if (pUsbInterface->m_ConfiguredPipes[iPipe] != NULL) {
                    pUsbInterface->m_ConfiguredPipes[iPipe]->
                        WaitForSentIoToComplete();
                }
            }
        }
    }

    if (Action != WdfIoTargetLeaveSentIoPending) {
        ReleaseInterfaceIterationLock();
    }
}

VOID
FxUsbDevice::Purge(
    __in WDF_IO_TARGET_PURGE_IO_ACTION Action
    )
{
    FxUsbInterface      *pUsbInterface;
    SINGLE_LIST_ENTRY   sentHead;
    ULONG               iPipe, iInterface;
    KIRQL               irql;

    sentHead.Next = NULL;

    //
    // Purge all of our own I/O first
    //
    FxIoTarget::Purge(Action);

    //
    // if we are just canceling i/o, then we just acquire the spin lock b/c
    // we can be called at dispatch level for this action code.
    //
    if (Action != WdfIoTargetPurgeIo) {
        ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

        AcquireInterfaceIterationLock();
    }

    //
    // Since we don't have to synchronize on the I/O already sent, just set
    // each pipe's state to purged.
    //
    Lock(&irql);

    for (iInterface = 0; iInterface < m_NumInterfaces; iInterface++) {
        pUsbInterface = m_Interfaces[iInterface];

        if (pUsbInterface->m_ConfiguredPipes != NULL) {
            for (iPipe = 0;
                 iPipe < pUsbInterface->m_NumberOfConfiguredPipes;
                 iPipe++) {

                if (pUsbInterface->m_ConfiguredPipes[iPipe] != NULL) {
                    BOOLEAN wait;
                    LIST_ENTRY pendedHead;

                    wait = FALSE;
                    InitializeListHead(&pendedHead);

                    pUsbInterface->m_ConfiguredPipes[iPipe]->GotoPurgeState(
                        Action,
                        &pendedHead,
                        &sentHead,
                        &wait,
                        TRUE
                        );

                    //
                    // Complete any requests pulled off from this pipe.
                    //
                    pUsbInterface->m_ConfiguredPipes[iPipe]->
                        CompletePendedRequestList(&pendedHead);
                }
            }
        }
    }
    Unlock(irql);

    //
    // Cancel all sent requests.
    //
    _CancelSentRequests(&sentHead);

    for (iInterface = 0; iInterface < m_NumInterfaces;  iInterface++) {
        pUsbInterface = m_Interfaces[iInterface];

        if (pUsbInterface->m_ConfiguredPipes != NULL) {
            //
            // Iterate over the pipes and clean each one up
            //
            for (iPipe = 0;
                 iPipe < pUsbInterface->m_NumberOfConfiguredPipes;
                 iPipe++) {
                //
                // Same reason as above
                //
                if (pUsbInterface->m_ConfiguredPipes[iPipe] != NULL) {
                    pUsbInterface->m_ConfiguredPipes[iPipe]->
                        WaitForSentIoToComplete();
                }
            }
        }
    }

    if (Action != WdfIoTargetPurgeIo) {
        ReleaseInterfaceIterationLock();
    }
}

VOID
FxUsbDevice::_CleanupPipesRequests(
    __in PLIST_ENTRY PendHead,
    __in PSINGLE_LIST_ENTRY SentHead
    )
{
    while (!IsListEmpty(PendHead)) {
        PLIST_ENTRY ple;
        FxRequestBase* pRequest;

        ple = RemoveHeadList(PendHead);

        InitializeListHead(ple);

        pRequest = FxRequestBase::_FromListEntry(ple);
        pRequest->GetTarget()->CompletePendedRequest(pRequest);
    }

    _CancelSentRequests(SentHead);
}

VOID
FxUsbDevice::PipesGotoRemoveState(
    __in BOOLEAN ForceRemovePipes
    )
{
    SINGLE_LIST_ENTRY sentHead;
    LIST_ENTRY pendHead, interfaceHead;
    FxUsbInterface* pUsbInterface;
    ULONG iPipe, intfIndex;
    KIRQL irql;

    sentHead.Next = NULL;
    InitializeListHead(&pendHead);
    InitializeListHead(&interfaceHead);

    AcquireInterfaceIterationLock();

    Lock(&irql);
    for (intfIndex = 0; intfIndex < m_NumInterfaces;  intfIndex++  ) {
        pUsbInterface = m_Interfaces[intfIndex];

        if (pUsbInterface->m_ConfiguredPipes != NULL) {
            for (iPipe = 0;
                 iPipe < pUsbInterface->m_NumberOfConfiguredPipes;
                 iPipe++) {
                BOOLEAN wait;

                wait = FALSE;

                //
                // Pipe can be NULL if the interface is half initialized
                //
                if (pUsbInterface->m_ConfiguredPipes[iPipe] != NULL) {
                    pUsbInterface->m_ConfiguredPipes[iPipe]->GotoRemoveState(
                        WdfIoTargetDeleted,
                        &pendHead,
                        &sentHead,
                        TRUE,
                        &wait);

                }
            }
        }
    }
    Unlock(irql);

    //
    // We cleanup requests no matter what the new state is because we complete all
    // pended requests in the surprise removed case.
    //
    _CleanupPipesRequests(&pendHead, &sentHead);

    //
    // Only destroy child pipe objects when the parent is going away or the
    // caller indicates that this is the desired action.
    //
    if (m_State == WdfIoTargetDeleted || ForceRemovePipes) {
        for (intfIndex = 0; intfIndex < m_NumInterfaces;  intfIndex++) {
            pUsbInterface = m_Interfaces[intfIndex];

            if (pUsbInterface->m_ConfiguredPipes != NULL) {
                //
                // Iterate over the pipes and clean each one up
                //
                for (iPipe = 0;
                     iPipe < pUsbInterface->m_NumberOfConfiguredPipes;
                     iPipe++) {
                    //
                    // Same reason as above
                    //
                    if (pUsbInterface->m_ConfiguredPipes[iPipe] != NULL) {
                        pUsbInterface->m_ConfiguredPipes[iPipe]->
                            WaitForSentIoToComplete();
                    }
                }
            }

            pUsbInterface->CleanUpAndDelete(FALSE);
        }
    }

    ReleaseInterfaceIterationLock();
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::CreateInterfaces(
    VOID
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    UCHAR descCountBitMap[UCHAR_MAX / sizeof(UCHAR)];
    PUSB_INTERFACE_DESCRIPTOR  pInterfaceDescriptor;
    UCHAR iInterface, numFound;
    NTSTATUS status;
    ULONG size;
    ULONG totalLength;

    pFxDriverGlobals = GetDriverGlobals();
    status = STATUS_SUCCESS;
    totalLength = m_ConfigDescriptor->wTotalLength;

    //
    // Make sure each PCOMMON_DESCRIPTOR_HEADER within the entire config descriptor is well formed.
    // If successful, we can walk the config descriptor using common headers without any more top
    // level error checking. Task specific checking of the specialized header types must still occur.
    //
    status = FxUsbValidateConfigDescriptorHeaders(
        pFxDriverGlobals,
        m_ConfigDescriptor,
        m_ConfigDescriptor->wTotalLength
        );

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Validation of the config descriptor failed due to a bad common descriptor header, %!STATUS!",
            status);
        return status;
    }

    //
    // Validate all interface descriptors in config descriptor are at least
    // sizeof(USB_INTERFACE_DESCRIPTOR).
    //





    status =  FxUsbValidateDescriptorType(
        pFxDriverGlobals,
        m_ConfigDescriptor,
        m_ConfigDescriptor,
        WDF_PTR_ADD_OFFSET(m_ConfigDescriptor, m_ConfigDescriptor->wTotalLength),
        USB_INTERFACE_DESCRIPTOR_TYPE,
        sizeof(USB_INTERFACE_DESCRIPTOR),
        FxUsbValidateDescriptorOpAtLeast,
        0
        );

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Validation of interface descriptors in config descriptor failed, %!STATUS!",
            status);

        return status;
    }

    if (m_ConfigDescriptor->bNumInterfaces == 0) {
        //
        // Use an array of one in the zero case
        //
        size = sizeof(FxUsbInterface*);
    }
    else {
        size = sizeof(FxUsbInterface*) * m_ConfigDescriptor->bNumInterfaces;
    }

    //
    // Allocate an array large enough to hold pointers to interfaces
    //
    m_Interfaces = (FxUsbInterface**)
        FxPoolAllocate(pFxDriverGlobals, NonPagedPool, size);

    if (m_Interfaces == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not allocate memory for %d interfaces, %!STATUS!",
            m_ConfigDescriptor->bNumInterfaces, status);

        goto Done;
    }

    RtlZeroMemory(m_Interfaces, size);
    m_NumInterfaces = m_ConfigDescriptor->bNumInterfaces;

    //
    // Iterate over the desciptors again, this time allocating an FxUsbInterface
    // for each one and capturing the interface information.
    //
    RtlZeroMemory(descCountBitMap, sizeof(descCountBitMap));
    iInterface = 0;
    numFound = 0;

    pInterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) FxUsbFindDescriptorType(
        m_ConfigDescriptor,
        m_ConfigDescriptor->wTotalLength,
        m_ConfigDescriptor,
        USB_INTERFACE_DESCRIPTOR_TYPE
        );

    while (pInterfaceDescriptor != NULL &&
           iInterface < m_ConfigDescriptor->bNumInterfaces) {

        //
        // This function will retun false if the bit wasn't already set
        //
        if (FxBitArraySet(descCountBitMap,
                          pInterfaceDescriptor->bInterfaceNumber) == FALSE) {
            FxUsbInterface* pInterface;

            pInterface = new (GetDriverGlobals(), WDF_NO_OBJECT_ATTRIBUTES)
                FxUsbInterface(pFxDriverGlobals,
                               this,
                               pInterfaceDescriptor);

            if (pInterface == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not allocate memory for interface object #%d, %!STATUS!",
                    iInterface, status);
                goto Done;
            }

            status = pInterface->Commit(WDF_NO_OBJECT_ATTRIBUTES, NULL, this);

            //
            // This should never fail
            //
            ASSERT(NT_SUCCESS(status));

            if (!NT_SUCCESS(status)) {
                goto Done;
            }

            status = pInterface->CreateSettings();
            if (!NT_SUCCESS(status)) {
                goto Done;
            }

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
            status = pInterface->SetWinUsbHandle(iInterface);
            if (!NT_SUCCESS(status)) {
                goto Done;
            }

            status = pInterface->MakeAndConfigurePipes(WDF_NO_OBJECT_ATTRIBUTES,
                                                       pInterfaceDescriptor->bNumEndpoints);
            if (!NT_SUCCESS(status)) {
                goto Done;
            }
#endif

            m_Interfaces[iInterface] = pInterface;

            iInterface++;
        }

        pInterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) FxUsbFindDescriptorType(
            m_ConfigDescriptor,
            totalLength,
            WDF_PTR_ADD_OFFSET(pInterfaceDescriptor,
                               pInterfaceDescriptor->bLength),
            USB_INTERFACE_DESCRIPTOR_TYPE
            );
    }

    //
    // We cannot check for the error case of
    //
    // pInterfaceDescriptor != NULL &&
    //  iInterface == m_ConfigDescriptor->bNumInterfaces
    //
    // Because if there are multiple alternative settings for the last interface
    // in the config descriptor, we will have hit the limit of interfaces
    // (correctly), but have found another pInterfaceDescriptor (the next alt
    // setting).
    //

    //
    // We already logged and found the case where iInterface >= m_NumInterfaces.
    // Check for the case where we found too few interfaces and when we found
    // no interfaces even though the config descriptor says otherwise.
    //
    //
    if (iInterface == 0 && m_NumInterfaces > 0) {
        status = STATUS_INVALID_DEVICE_REQUEST;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Config descriptor indicated there were %d interfaces, but did not "
            "find any interface descriptors in config descriptor %p, %!STATUS!",
            m_NumInterfaces, m_ConfigDescriptor, status);
    }
    else if (pInterfaceDescriptor != NULL && m_NumInterfaces == 0) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGIOTARGET,
            "Config descriptor indicated there were 0 interfaces, but an interface "
            "descriptor was found");

        m_MismatchedInterfacesInConfigDescriptor = TRUE;
    }
    else if (iInterface < m_NumInterfaces) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Config descriptor indicated there were %d interfaces, only found "
            "%d interfaces", m_NumInterfaces, iInterface);

        //
        // Instead of considering this an error, just use the number found.
        // This will not have an adverse affect elsewhere and since the framework
        // is probably more strict then previous USB code, this would have not
        // been found earlier by a WDM driver.
        //
        m_NumInterfaces = iInterface;
    }

Done:
    return status;
}


_Must_inspect_result_
NTSTATUS
FxUsbDevice::GetPortStatus(
    __out PULONG PortStatus
    )
{
    NTSTATUS status;

    FxInternalIoctlOthersContext context;
    FxRequestBuffer args[FX_REQUEST_NUM_OTHER_PARAMS];
    FxSyncRequest syncRequest(GetDriverGlobals(), &context);

    //
    // FxSyncRequest always succeesds for KM.
    //
    status = syncRequest.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    *PortStatus = 0;
    args[0].SetBuffer(PortStatus, 0);
    args[1].SetBuffer(NULL, 0);
    args[2].SetBuffer(NULL, 0);

    status = FormatInternalIoctlOthersRequest(syncRequest.m_TrueRequest,
                                              IOCTL_INTERNAL_USB_GET_PORT_STATUS,
                                              args);

    if (NT_SUCCESS(status)) {
        WDF_REQUEST_SEND_OPTIONS options;

        WDF_REQUEST_SEND_OPTIONS_INIT(
            &options, WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);

        status = SubmitSync(syncRequest.m_TrueRequest, &options);
    }

    return status;
}



BOOLEAN
FxUsbDevice::IsEnabled(
    VOID
    )
{
    NTSTATUS status;
    ULONG portStatus;
    BOOLEAN enabled;

    enabled = TRUE;
    status = GetPortStatus(&portStatus);

    //
    // Inability to get STATUS_SUCCESS from GetPortStatus is more likely a resource
    // issue rather than a device issue so return FALSE from this function only if
    // we were able to read the PortStatus and the port was disabled.
    // What you don't want is to continuosly reset the device (by returning FALSE)
    // instead of resetting pipe (by returning TRUE) under low memory conditions.
    //
    if (NT_SUCCESS(status) && (portStatus & USBD_PORT_ENABLED) == 0) {
        enabled = FALSE;
    }

    return enabled;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::IsConnected(
    VOID
    )
{
#if (FX_CORE_MODE == FX_CORE_USER_MODE)




    return STATUS_UNSUCCESSFUL;
#elif (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    NTSTATUS status;
    ULONG portStatus;

    status = GetPortStatus(&portStatus);
    if (NT_SUCCESS(status) && (portStatus & USBD_PORT_CONNECTED) == 0) {
        status = STATUS_DEVICE_DOES_NOT_EXIST;
    }

    return status;
#endif
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::CyclePort(
    VOID
    )
{
    FxIoContext context;
    FxSyncRequest request(GetDriverGlobals(), &context);
    NTSTATUS status;

    //
    // FxSyncRequest always succeesds for KM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    status = FormatCycleRequest(request.m_TrueRequest);

    if (NT_SUCCESS(status)) {
        CancelSentIo();
        status = SubmitSyncRequestIgnoreTargetState(request.m_TrueRequest, NULL);
        //
        // NOTE: CyclePort causes the device to be removed and re-enumerated so
        // don't do anymore operations after this point.
        //
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::FormatCycleRequest(
    __in FxRequestBase* Request
    )
{
    FxRequestBuffer emptyBuffer;

    return FormatIoctlRequest(Request,
                              IOCTL_INTERNAL_USB_CYCLE_PORT,
                              TRUE,
                              &emptyBuffer,
                              &emptyBuffer);
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::GetConfigDescriptor(
    __out PVOID ConfigDescriptor,
    __inout PUSHORT ConfigDescriptorLength
    )
{
    NTSTATUS status;
    USHORT copyLength;

    if (ConfigDescriptor == NULL) {
        //
        // Caller wants length to allocate
        //
        *ConfigDescriptorLength = m_ConfigDescriptor->wTotalLength;
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (*ConfigDescriptorLength < m_ConfigDescriptor->wTotalLength) {
        //
        // Valid ConfigDescriptor passed in, but its too small.  Copy as many
        // bytes as we can.
        //
        copyLength = *ConfigDescriptorLength;
        status = STATUS_BUFFER_TOO_SMALL;
    }
    else {
        copyLength = m_ConfigDescriptor->wTotalLength;
        status = STATUS_SUCCESS;
    }

    //
    // Always indicate to the caller the number of required bytes or the
    // number of bytes we copied.
    //
    RtlCopyMemory(ConfigDescriptor, m_ConfigDescriptor, copyLength);
    *ConfigDescriptorLength = m_ConfigDescriptor->wTotalLength;

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SelectConfigDescriptor(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in PWDF_USB_DEVICE_SELECT_CONFIG_PARAMS Params
    )
{
    PUSBD_INTERFACE_LIST_ENTRY pInterfaces;
    PURB urb;
    NTSTATUS status;
    ULONG i, size;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;
    PUSB_INTERFACE_DESCRIPTOR* interfaceDescriptors;
    ULONG numInterfaces;
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    configurationDescriptor = Params->Types.Descriptor.ConfigurationDescriptor;
    interfaceDescriptors = Params->Types.Descriptor.InterfaceDescriptors;
    numInterfaces = Params->Types.Descriptor.NumInterfaceDescriptors;

    for (i = 0; i < numInterfaces; i++) {
        if (interfaceDescriptors[i] == NULL) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    // eventually size = sizeof(USBD_INTERFACE_LIST_ENTRY) * (numInterfaces + 1)
    //
    status = RtlULongAdd(numInterfaces, 1, &size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlULongMult(size, sizeof(USBD_INTERFACE_LIST_ENTRY), &size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    pInterfaces = (PUSBD_INTERFACE_LIST_ENTRY) FxPoolAllocate(
        pFxDriverGlobals, NonPagedPool, size);

    if (pInterfaces == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not allocate array of USBD_INTERFACE_LIST_ENTRY, %!STATUS!",
            status);

        return status;
    }

    RtlZeroMemory(pInterfaces, size);

    for (i = 0; i < numInterfaces; i++) {
        pInterfaces[i].InterfaceDescriptor = interfaceDescriptors[i];
    }

    if (configurationDescriptor == NULL) {
        configurationDescriptor = m_ConfigDescriptor;
    }

    //
    // NOTE:
    //
    // Creating a config request using the caller's config descriptor does not
    // currently work if the provided config descriptor is not the same as the
    // descriptor reported by the device.   It does not work because we try to
    // validate the interface number in the URB against an existing
    // FxUsbInterface (which is based on the config descriptor described by the
    // device, not the provided one), and if that validation fails, we return
    // !NT_SUCCESS from SelectConfig().
    //
    urb = FxUsbCreateConfigRequest(GetDriverGlobals(),
                                   configurationDescriptor,
                                   pInterfaces,
                                   GetDefaultMaxTransferSize());
    if (urb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        status = SelectConfig(PipesAttributes, urb, FxUrbTypeLegacy, NULL);
        FxPoolFree(urb);
        urb = NULL;
    }

    FxPoolFree(pInterfaces);
    pInterfaces = NULL;

    return status;
}

struct FxInterfacePipeInformation {
    //
    // Array of pipes
    //
    FxUsbPipe** Pipes;

    //
    // Number of entries in Pipes
    //
    ULONG NumPipes;
};

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SelectConfig(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in PURB Urb,
    __in FX_URB_TYPE FxUrbType,
    __out_opt PUCHAR NumConfiguredInterfaces
    )
/*++

Routine Description:
    Selects the configuration as described by the parameter Urb.  If there is a
    previous active configuration, the WDFUSBPIPEs for it are stopped and
    destroyed before the new configuration is selected

Arguments:
    PipesAttributes - object attributes to apply to each created WDFUSBPIPE

    Urb -  the URB describing the configuration to select

Return Value:
    NTSTATUS

  --*/
{
    WDF_REQUEST_SEND_OPTIONS options;
    PUCHAR pCur, pEnd;
    PUSBD_INTERFACE_INFORMATION pIface;
    FxUsbPipe* pPipe;
    PURB pSelectUrb;
    NTSTATUS status;
    ULONG iPipe;
    USHORT maxNumPipes, size;
    UCHAR numPipes;
    FxInterfacePipeInformation* pPipeInfo;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbInterface * pUsbInterface ;
    UCHAR intfIndex;
    FxIrp* irp;

    pFxDriverGlobals = GetDriverGlobals();
    FxSyncRequest request(GetDriverGlobals(), NULL);

    pIface = NULL;
    maxNumPipes = 0;
    size = 0;
    pPipeInfo = NULL;
    pSelectUrb = NULL;

    //
    // Callers to this function have guaranteed that there are interfaces
    // reported on this device.
    //
    ASSERT(m_NumInterfaces != 0);

    if (NumConfiguredInterfaces != NULL) {
        *NumConfiguredInterfaces = 0;
    }

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        goto Done;
    }

    //
    // Allocate a PIRP for the select config and possible select interface(s)
    //
    status = request.m_TrueRequest->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    //
    // Allocate a pool for storing the FxUsbPipe ** before we
    // assign them to the Interfaces just in case all doesn't go well.
    //
    if (m_NumInterfaces == 0) {
        //
        // Use one in the zero case to make the logic simpler
        //
        size = sizeof(FxInterfacePipeInformation);
    }
    else {
        size = m_NumInterfaces * sizeof(FxInterfacePipeInformation);
    }

    pPipeInfo = (FxInterfacePipeInformation*) FxPoolAllocate(
        pFxDriverGlobals, NonPagedPool, size
        );

    if (pPipeInfo == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not internal allocate tracking info for selecting a config on "
            "WDFUSBDEVICE 0x%p, %!STATUS!", GetHandle(), status);
        goto Done;
    }

    RtlZeroMemory(pPipeInfo, size);

    //
    // The following code and the one in select setting have a lot in common
    // but can't leverage on that as that sends the select interface URB down
    // Our goal is allocate the resources -- the pipes and the select setting URB
    // before sending down the select config URB so that the tear down is simpler.
    //

    //
    // Each FxUsbInterface will need a FxUsbPipe* for each pipe contained in
    // the interface.
    //
    pCur = (PUCHAR) &Urb->UrbSelectConfiguration.Interface;
    pEnd = ((PUCHAR) Urb) + Urb->UrbSelectConfiguration.Hdr.Length;
    intfIndex = 0;

    for ( ; pCur < pEnd; pCur += pIface->Length, intfIndex++) {
        FxUsbPipe** ppPipes;

        pIface = (PUSBD_INTERFACE_INFORMATION) pCur;
        if (pIface->NumberOfPipes > UCHAR_MAX) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "WDFUSBDEVICE supports a maximum of %d pipes per interface, "
                "USBD_INTERFACE_INFORMATION %p specified %d, %!STATUS!",
                UCHAR_MAX, pIface, pIface->NumberOfPipes, status);
            goto Done;
        }

        pUsbInterface = GetInterfaceFromNumber(pIface->InterfaceNumber);
        if (pUsbInterface == NULL) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not find an instance of an interface descriptor with "
                "InterfaceNumber %d, %!STATUS!",
                pIface->InterfaceNumber, status);
            goto Done;
        }

        numPipes = (UCHAR) pIface->NumberOfPipes;

        //
        // Track the maximum number of pipes we have seen in an interface in
        // case we need to allocate a select interface URB.
        //
        if (numPipes > maxNumPipes) {
            maxNumPipes = (USHORT) numPipes;
        }

        if (numPipes > 0) {
            size = numPipes * sizeof(FxUsbPipe *);
        }
        else {
            //
            // It is valid to have an interface with zero pipes in it.  In that
            // case, we just allocate one entry so that we have a valid array
            // and keep the remaining code simple.
            //
            size = sizeof(FxUsbPipe*);
        }

        ppPipes = (FxUsbPipe**) FxPoolAllocate(
            pFxDriverGlobals,
            NonPagedPool,
            size
            );

        if (ppPipes == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not allocate memory for Pipes "
                "InterfaceNumber %d, %!STATUS!",
                pIface->InterfaceNumber, status);
            goto Done;
        }

        RtlZeroMemory(ppPipes, size);

        //
        // We store the pointer to the newly allocated arary in a temporary b/c
        // we can still fail and we don't want a half baked interface.
        //

        pPipeInfo[intfIndex].Pipes = ppPipes;
        pPipeInfo[intfIndex].NumPipes = numPipes;

        for (iPipe = 0; iPipe < numPipes; iPipe++) {
            ppPipes[iPipe] = new (GetDriverGlobals(), PipesAttributes)
                FxUsbPipe(GetDriverGlobals(),this);

            if (ppPipes[iPipe] == NULL) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not allocate a pipe object, %!STATUS!", status);
                goto Done;
            }

            pPipe = ppPipes[iPipe];
            status = pPipe->Init(m_Device);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not Init the pipe object, %!STATUS!", status);
                goto Done;
            }

            status = pPipe->Commit(PipesAttributes, NULL, pUsbInterface);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "Could not commit the pipe object, %!STATUS!", status);
                goto Done;
            }
        }









        if (pUsbInterface->IsInterfaceConfigured()) {
            CleanupInterfacePipesAndDelete(pUsbInterface);
        }
    }

    //
    // If we are selecting more then one interface and at least one of them
    // has pipes in them, allocate a select interface URB that will be used
    // after the select config to select each interface.  The select interface
    // URB will be big enough to select the largest interface in the group.
    //
    // The select interface URB is allocated before the select config so that
    // we know once the select config has completed successfully, we can be
    // guaranteed that we can select the interfaces we want and not have to
    // handle undoing the select config upon URB allocation failure.
    //
    if (m_NumInterfaces > 1 && maxNumPipes > 0) {
        size = GET_SELECT_INTERFACE_REQUEST_SIZE(maxNumPipes);

        pSelectUrb = (PURB) FxPoolAllocate(GetDriverGlobals(),
                                           NonPagedPool,
                                           size
                                           );

        if (pSelectUrb == NULL) {
            status = STATUS_INSUFFICIENT_RESOURCES;
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Could not allocate a select interface URB, %!STATUS!", status);
            goto Done;
        }

        RtlZeroMemory(pSelectUrb, size);
    }

    //
    // Send the select config to the usb core
    //
    WDF_REQUEST_SEND_OPTIONS_INIT(&options,
                                  WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);
    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&options, WDF_REL_TIMEOUT_IN_SEC(2));

    FxFormatUsbRequest(request.m_TrueRequest, Urb, FxUrbType, m_USBDHandle);
    status = SubmitSync(request.m_TrueRequest, &options);

    if (NT_SUCCESS(status)) {
        //
        // Save the config handle and store off all the pipes
        //
        m_ConfigHandle = Urb->UrbSelectConfiguration.ConfigurationHandle;

        //
        // Initialize the first interface
        //
        pIface = &Urb->UrbSelectConfiguration.Interface;

        pUsbInterface = GetInterfaceFromNumber(pIface->InterfaceNumber);
        pUsbInterface->SetNumConfiguredPipes((UCHAR)pIface->NumberOfPipes);

        //
        // The interface now owns the array of pipes
        //
        pUsbInterface->SetConfiguredPipes(pPipeInfo[0].Pipes);
        pPipeInfo[0].Pipes = NULL;
        pPipeInfo[0].NumPipes = 0;

        pUsbInterface->SetInfo(pIface);

        //
        // Since this could be the only interface, set the index now, so if we
        // return the number of configured interfaces, it will be valid for the
        // single interface case.
        //
        intfIndex = 1;

        //
        // If we have a more then one interface, we must select each of the addtional
        // interfaces after the select config because usbccgp (the generic parent
        // driver) didn't fill out all of the pipe info for IAD (interface
        // association descriptor) devices.  The first interface is filled out
        // property, it is 2->N that are left blank.
        //
        // pSelectUrb can be equal to NULL and have more then one interface if
        // all interfaces have no pipes associated with them.  In that case, we
        // still want to iterate over the remaining pipes so that we can
        // initialize our structures properly.
        //
        if (m_NumInterfaces > 1) {
            //
            // Loop over the interfaces again.
            //
            pCur = (PUCHAR) &Urb->UrbSelectConfiguration.Interface;
            pIface = (PUSBD_INTERFACE_INFORMATION) pCur;
            pEnd = ((PUCHAR) Urb) + Urb->UrbSelectConfiguration.Hdr.Length;

            //
            // Start at the 2nd one since the first is already selected
            //
            pCur += pIface->Length;

            for ( ; pCur < pEnd; pCur += pIface->Length, intfIndex++) {
#pragma prefast(suppress: __WARNING_UNUSED_POINTER_ASSIGNMENT, "pIface is used in the for loop in many places. It looks like a false positive")
                pIface = (PUSBD_INTERFACE_INFORMATION) pCur;
                ASSERT(pIface->NumberOfPipes <= maxNumPipes);

                pUsbInterface = GetInterfaceFromNumber(pIface->InterfaceNumber);
                ASSERT(pUsbInterface != NULL);

                //
                // Valid to have an interface with no pipes.  If there are no
                // pipes, GET_SELECT_INTERFACE_REQUEST_SIZE will compute value
                // too small of a size to pass validation.
                //
                if (pIface->NumberOfPipes > 0) {

                    pUsbInterface->FormatSelectSettingUrb(
                        pSelectUrb,
                        (USHORT) pIface->NumberOfPipes,
                        pIface->AlternateSetting
                        );
                    irp = request.m_TrueRequest->GetSubmitFxIrp();
                    irp->Reuse(STATUS_SUCCESS);

                    request.m_TrueRequest->ClearFieldsForReuse();
                    FxFormatUsbRequest(request.m_TrueRequest,
                                       pSelectUrb,
                                       FxUrbTypeLegacy,
                                       NULL);

                    status = SubmitSync(request.m_TrueRequest, &options);
                    if (!NT_SUCCESS(status)) {
                        DoTraceLevelMessage(
                            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "USB core failed Select Interface URB, %!STATUS!",
                            status);
                        goto Done;
                    }

                    //
                    // Copy the info back into the original select config URB
                    //

                    RtlCopyMemory(pIface,
                                  &pSelectUrb->UrbSelectInterface.Interface,
                                  pSelectUrb->UrbSelectInterface.Interface.Length);
                }

                //
                // Update the pointer to point to the new pipes with the pointer
                // to the pipes stored earlier.
                //
                ASSERT(pPipeInfo[intfIndex].NumPipes == pIface->NumberOfPipes);
                pUsbInterface->SetNumConfiguredPipes((UCHAR)pIface->NumberOfPipes);

                //
                // The interface now owns the array of pipes
                //
                pUsbInterface->SetConfiguredPipes(pPipeInfo[intfIndex].Pipes);
                pPipeInfo[intfIndex].Pipes = NULL;
                pPipeInfo[intfIndex].NumPipes = 0;

                //
                // SetInfo only after a successful select config so that we can
                // copy the USBD_PIPE_INFORMATION .
                //
                pUsbInterface->SetInfo(pIface);
            }
        }

        if (NumConfiguredInterfaces != NULL) {
            *NumConfiguredInterfaces = intfIndex;
        }
    }
    else {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "USB core failed Select Configuration, %!STATUS!", status);
    }

Done:
    if (pSelectUrb != NULL) {
        FxPoolFree(pSelectUrb);
        pSelectUrb = NULL;
    }

    if (pPipeInfo != NULL) {
        //
        // Free all arrays that may have been allocated
        //
        for (intfIndex = 0; intfIndex < m_NumInterfaces; intfIndex++) {
            //
            // We can have NumPipes == 0 and still have an allocated array, so
            // use the array != NULL as the check.
            //
            if (pPipeInfo[intfIndex].Pipes != NULL) {
                //
                // Delete any pipes that might have been created
                //
                for (iPipe = 0;  iPipe < pPipeInfo[intfIndex].NumPipes; iPipe++) {
                    if (pPipeInfo[intfIndex].Pipes[iPipe] != NULL) {
                        pPipeInfo[intfIndex].Pipes[iPipe]->DeleteFromFailedCreate();
                        pPipeInfo[intfIndex].Pipes[iPipe] = NULL;
                    }
                }

                //
                // Now free the array itself and clear out the count
                //
                FxPoolFree(pPipeInfo[intfIndex].Pipes);
                pPipeInfo[intfIndex].Pipes = NULL;
                pPipeInfo[intfIndex].NumPipes = 0;
            }
        }

        FxPoolFree(pPipeInfo);
        pPipeInfo = NULL;
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::Deconfig(
    VOID
    )
{
    WDF_REQUEST_SEND_OPTIONS options;
    _URB_SELECT_CONFIGURATION urb;

    FxSyncRequest request(GetDriverGlobals(), NULL);
    NTSTATUS status;

    //
    // FxSyncRequest always succeesds for KM but can fail for UM.
    //
    status = request.Initialize();
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                            "Failed to initialize FxSyncRequest");
        return status;
    }

    status = request.m_TrueRequest->ValidateTarget(this);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // This will remove and free all interfaces and associated pipes
    //
    PipesGotoRemoveState(TRUE);

    RtlZeroMemory(&urb, sizeof(urb));

#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "this annotation change in usb.h is communicated to usb team");
    UsbBuildSelectConfigurationRequest((PURB) &urb, sizeof(urb), NULL);
#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "this annotation change in usb.h is communicated to usb team");
    FxFormatUsbRequest(request.m_TrueRequest, (PURB) &urb, FxUrbTypeLegacy, NULL);

    WDF_REQUEST_SEND_OPTIONS_INIT(
        &options, WDF_REQUEST_SEND_OPTION_IGNORE_TARGET_STATE);

    status = SubmitSync(request.m_TrueRequest, &options);

    return status;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::SelectConfigInterfaces(
    __in PWDF_OBJECT_ATTRIBUTES PipesAttributes,
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    __in_ecount(NumInterfaces) PUSB_INTERFACE_DESCRIPTOR* InterfaceDescriptors,
    __in ULONG NumInterfaces
    )
{
    PUSBD_INTERFACE_LIST_ENTRY pInterfaces;
    PURB urb;
    NTSTATUS status;
    ULONG i, size;
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;

    pFxDriverGlobals = GetDriverGlobals();

    //
    // eventually size = sizeof(USBD_INTERFACE_LIST_ENTRY) * (NumInterfaces + 1)
    //
    status = RtlULongAdd(NumInterfaces, 1, &size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = RtlULongMult(size, sizeof(USBD_INTERFACE_LIST_ENTRY), &size);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    for (i = 0; i < NumInterfaces; i++) {
        if (InterfaceDescriptors[i] == NULL) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    pInterfaces = (PUSBD_INTERFACE_LIST_ENTRY) FxPoolAllocate(
        pFxDriverGlobals,
        NonPagedPool,
        size
        );

    if (pInterfaces == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;

        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "Could not allocate array of USBD_INTERFACE_LIST_ENTRY, %!STATUS!",
            status);

        return status;
    }

    RtlZeroMemory(pInterfaces, size);

    for (i = 0; i < NumInterfaces; i++) {
        pInterfaces[i].InterfaceDescriptor = InterfaceDescriptors[i];
    }

    if (ConfigurationDescriptor == NULL) {
        ConfigurationDescriptor = m_ConfigDescriptor;
    }

    urb = FxUsbCreateConfigRequest(GetDriverGlobals(),
                                   ConfigurationDescriptor,
                                   pInterfaces,
                                   GetDefaultMaxTransferSize());
    if (urb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    else {
        status = SelectConfig(PipesAttributes, urb, FxUrbTypeLegacy, NULL);
        FxPoolFree(urb);
        urb = NULL;
    }

    FxPoolFree(pInterfaces);
    pInterfaces = NULL;

    return status;
}

FxUsbInterface *
FxUsbDevice::GetInterfaceFromIndex(
    __in UCHAR InterfaceIndex
    )
{
    if (InterfaceIndex < m_NumInterfaces){
        return m_Interfaces[InterfaceIndex];
    }

    return NULL;
}

_Must_inspect_result_
NTSTATUS
FxUsbDevice::GetInterfaceNumberFromInterface(
    __in WDFUSBINTERFACE UsbInterface,
    __out PUCHAR InterfaceNumber
    )
{
     FxUsbInterface * pUsbInterface;

     FxObjectHandleGetPtr(GetDriverGlobals(),
                          UsbInterface,
                          FX_TYPE_USB_INTERFACE ,
                          (PVOID*) &pUsbInterface);

     *InterfaceNumber = pUsbInterface->GetInterfaceNumber();

     return STATUS_SUCCESS;
}

FxUsbInterface *
FxUsbDevice::GetInterfaceFromNumber(
    __in UCHAR InterfaceNumber
    )
{
    ULONG i;

    for (i = 0; i < m_NumInterfaces; i++) {
        if (m_Interfaces[i]->GetInterfaceNumber() == InterfaceNumber) {
            return m_Interfaces[i];
        }
    }

    return NULL;
}

VOID
FxUsbDevice::CleanupInterfacePipesAndDelete(
    __in FxUsbInterface * UsbInterface
    )
{
    SINGLE_LIST_ENTRY sentHead;
    LIST_ENTRY pendHead;
    ULONG iPipe;
    FxUsbPipe *pPipe;
    KIRQL irql;

    sentHead.Next = NULL;
    InitializeListHead(&pendHead);

    AcquireInterfaceIterationLock();

    Lock(&irql);
    for (iPipe = 0; iPipe < UsbInterface->m_NumberOfConfiguredPipes; iPipe++) {
        BOOLEAN wait;

        wait = FALSE;
        pPipe = UsbInterface->m_ConfiguredPipes[iPipe];
        pPipe->GotoRemoveState(
            WdfIoTargetDeleted,
            &pendHead,
            &sentHead,
            TRUE,
            &wait);
    }
    Unlock(irql);

    _CleanupPipesRequests(&pendHead, &sentHead);

    for (iPipe = 0; iPipe < UsbInterface->m_NumberOfConfiguredPipes; iPipe++) {
        pPipe = UsbInterface->m_ConfiguredPipes[iPipe];
        pPipe->WaitForSentIoToComplete();
    }

    UsbInterface->CleanUpAndDelete(FALSE);

    ReleaseInterfaceIterationLock();
}

VOID
FxUsbDevice::CancelSentIo(
    VOID
    )
{
    FxUsbInterface *pUsbInterface;
    ULONG iInterface, iPipe;

    for (iInterface = 0; iInterface < m_NumInterfaces; iInterface++) {
        pUsbInterface = m_Interfaces[iInterface];

        if (pUsbInterface->m_ConfiguredPipes != NULL) {
            for (iPipe = 0;
                 iPipe < pUsbInterface->m_NumberOfConfiguredPipes;
                 iPipe++) {

                if (pUsbInterface->m_ConfiguredPipes[iPipe] != NULL) {
                    pUsbInterface->m_ConfiguredPipes[iPipe]->CancelSentIo();
                }

            }
        }
    }
    FxIoTarget::CancelSentIo(); // __super call
}

__checkReturn
NTSTATUS
FxUsbDevice::CreateUrb(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __out
    WDFMEMORY* UrbMemory,
    __deref_opt_out_bcount(sizeof(URB))
    PURB* Urb
    )
{
    NTSTATUS status;
    PURB urbLocal = NULL;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbUrb * pUrb = NULL;
    WDFMEMORY hMemory;
    FxObject* pParent;

    //
    // Get the parent's globals if it is present, else use the ones for FxUsbDevice
    //
    pFxDriverGlobals = GetDriverGlobals();
    status = FxValidateObjectAttributesForParentHandle(pFxDriverGlobals, Attributes);

    if (NT_SUCCESS(status)) {

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       Attributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);

        if (FALSE == IsObjectDisposedOnRemove(pParent)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Urb must be parented to FxDevice or an IoAllocated Request");
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

    }
    else if (status == STATUS_WDF_PARENT_NOT_SPECIFIED) {

        pFxDriverGlobals = GetDriverGlobals();
        pParent = this;
        status = STATUS_SUCCESS;

    }
    else {

        goto Done;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, Attributes);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    FxPointerNotNull(pFxDriverGlobals, UrbMemory);

    *UrbMemory = NULL;

    status = USBD_UrbAllocate(m_USBDHandle, &urbLocal);

    if (!NT_SUCCESS(status)) {

        urbLocal = NULL;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "USBDEVICE Must have been created with Client Contract Version Info, %!STATUS!",
            status);

        goto Done;

    }

    pUrb = new(pFxDriverGlobals, Attributes)
            FxUsbUrb(pFxDriverGlobals, m_USBDHandle, urbLocal, sizeof(URB));

    if (pUrb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    urbLocal = NULL;

    status = pUrb->Commit(Attributes, (WDFOBJECT*)&hMemory, pParent);

    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    *UrbMemory = hMemory;

    if (Urb) {
        *Urb = (PURB) pUrb->GetBuffer();
    }

Done:

    if (!NT_SUCCESS(status)) {

        if (pUrb) {
            pUrb->DeleteFromFailedCreate();
        }

        if (urbLocal) {
            USBD_UrbFree(m_USBDHandle, urbLocal);
        }

    }

    return status;
}

__checkReturn
NTSTATUS
FxUsbDevice::CreateIsochUrb(
    __in_opt
    PWDF_OBJECT_ATTRIBUTES Attributes,
    __in
    ULONG NumberOfIsochPackets,
    __out
    WDFMEMORY* UrbMemory,
    __deref_opt_out_bcount(GET_ISOCH_URB_SIZE(NumberOfIsochPackets))
    PURB* Urb
    )
{
    NTSTATUS status;
    PURB urbLocal = NULL;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    FxUsbUrb * pUrb = NULL;
    WDFMEMORY hMemory;
    ULONG size;
    FxObject* pParent;

    //
    // Get the parent's globals if it is present, else use the ones for FxUsbDevice
    //
    pFxDriverGlobals = GetDriverGlobals();
    status = FxValidateObjectAttributesForParentHandle(pFxDriverGlobals, Attributes);

    if (NT_SUCCESS(status)) {

        FxObjectHandleGetPtrAndGlobals(pFxDriverGlobals,
                                       Attributes->ParentObject,
                                       FX_TYPE_OBJECT,
                                       (PVOID*)&pParent,
                                       &pFxDriverGlobals);

        if (FALSE == IsObjectDisposedOnRemove(pParent)) {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "Urb must be parented to FxDevice or IoAllocated Request");
            status = STATUS_INVALID_PARAMETER;
            goto Done;
        }

    }
    else if (status == STATUS_WDF_PARENT_NOT_SPECIFIED) {

        pFxDriverGlobals = GetDriverGlobals();
        pParent = this;
        status = STATUS_SUCCESS;

    }
    else {

        goto Done;
    }

    status = FxValidateObjectAttributes(pFxDriverGlobals, Attributes);
    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    FxPointerNotNull(pFxDriverGlobals, UrbMemory);

    *UrbMemory = NULL;

    status = USBD_IsochUrbAllocate(m_USBDHandle, NumberOfIsochPackets, &urbLocal);

    if (!NT_SUCCESS(status)) {

        urbLocal = NULL;
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGIOTARGET,
            "USBDEVICE Must have been created with Client Contract Version Info, %!STATUS!",
            status);

        goto Done;

    }

    size = GET_ISO_URB_SIZE(NumberOfIsochPackets);

    pUrb = new(pFxDriverGlobals, Attributes)
            FxUsbUrb(pFxDriverGlobals, m_USBDHandle, urbLocal, size);

    if (pUrb == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Done;
    }

    urbLocal = NULL;

    status = pUrb->Commit(Attributes, (WDFOBJECT*)&hMemory, pParent);

    if (!NT_SUCCESS(status)) {
        goto Done;
    }

    *UrbMemory = hMemory;

    if (Urb) {
        *Urb = (PURB) pUrb->GetBuffer();
    }

Done:

    if (!NT_SUCCESS(status)) {

        if (pUrb) {
            pUrb->DeleteFromFailedCreate();
        }

        if (urbLocal) {
            USBD_UrbFree(m_USBDHandle, urbLocal);
        }

    }

    return status;
}

BOOLEAN
FxUsbDevice::IsObjectDisposedOnRemove(
    __in FxObject* Object
    )
/*++

Routine Description:
    This routine determines if the Object being passed is guranteed to be disposed on a
    Pnp remove operation.

    The object will be disposed on Pnp remove operation if it is a somewhere in the child
    tree of the FxDevice assocaited with this FxUsbDevice object, or in the child tree of an
    Io Allocated Request.

Arguments:

    Object - The Object being checked

Return Value:

    TRUE if the Object is guranteed to be disposed on a pnp remove operation
    FALSE otherwise.

  --*/
{
    FxObject * obj;
    FxObject * parent;
    BOOLEAN isObjectDisposedOnRemove = FALSE;

    obj = Object;

    //
    // By adding a reference now, we simulate what GetParentObjectReferenced
    // does later, thus allowing simple logic on when/how to release the
    // reference on exit.
    //
    obj->ADDREF(Object);

    while (obj != NULL) {

        if (obj == (FxObject*) m_Device) {

            isObjectDisposedOnRemove = TRUE;
            break;
        }

        if (FxObjectCheckType(obj, FX_TYPE_REQUEST)) {

            FxRequestBase* request = (FxRequestBase*) obj;
            if (request->IsAllocatedFromIo()) {

                isObjectDisposedOnRemove = TRUE;
                break;
            }
        }

        parent = obj->GetParentObjectReferenced(Object);

        //
        // Release the reference previously taken by the top of the function
        // or GetParentObjectReferenced in a previous pass in the loop.
        //
        obj->RELEASE(Object);
        obj = parent;
    }

    if (obj != NULL) {

        //
        // Release the reference previously taken by the top of the function
        // or GetParentObjectReferenced in a last pass in the loop.
        //
        obj->RELEASE(Object);
    }

    return isObjectDisposedOnRemove;
}

FX_URB_TYPE
FxUsbDevice::GetFxUrbTypeForRequest(
    __in FxRequestBase* Request
    )
/*++

Routine Description:
    This routine essentially determines whether this routine can use a urb allocated by
    the USBD_xxxUrbAllocate APIs

    The USBD_xxxUrbAllocate APIs are only used for those requests that could be disposed at the
    time the client driver devnode is being removed.

    If we cannot make that gurantee about that request, FxUrbTypeLegacy is returned and the
    USBD_xxxUrbAllocate api's must not be used to allocate an Urb.

    Else FxUrbTypeUsbdAllocated is returned.

Arguments:

    Request - FxRequest

Return Value:

    FxUrbTypeUsbdAllocated, or FxUrbTypeLegacy

  --*/
{
    if (m_UrbType == FxUrbTypeLegacy) {
        return FxUrbTypeLegacy;
    }

    if (Request->IsAllocatedFromIo()) {
        return FxUrbTypeUsbdAllocated;
    }

    if (IsObjectDisposedOnRemove((FxObject*) Request)) {
        return FxUrbTypeUsbdAllocated;
    }

    return FxUrbTypeLegacy;
}

