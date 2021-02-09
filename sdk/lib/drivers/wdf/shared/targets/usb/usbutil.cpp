/*++

Copyright (c) Microsoft Corporation

Module Name:

    usbutil.cpp

Abstract:


Author:

Environment:

    Both kernel and user mode

Revision History:

--*/

#include "fxusbpch.hpp"

extern "C" {
#include "UsbUtil.tmh"
}

VOID
FxFormatUsbRequest(
    __in FxRequestBase* Request,
    __in PURB Urb,
    __in FX_URB_TYPE FxUrbType,
    __drv_when(FxUrbType == FxUrbTypeUsbdAllocated, __in)
    __drv_when(FxUrbType != FxUrbTypeUsbdAllocated, __in_opt)
         USBD_HANDLE UsbdHandle
    )
{
    FxIrp* irp;

    ASSERT(Urb->UrbHeader.Length >= sizeof(_URB_HEADER));

    irp = Request->GetSubmitFxIrp();
    irp->ClearNextStackLocation();
    irp->SetMajorFunction(IRP_MJ_INTERNAL_DEVICE_CONTROL);
    irp->SetParameterIoctlCode(IOCTL_INTERNAL_USB_SUBMIT_URB);





    if (FxUrbType == FxUrbTypeUsbdAllocated) {
        USBD_AssignUrbToIoStackLocation(UsbdHandle, irp->GetNextIrpStackLocation(), Urb);
    }
    else {
        irp->SetNextStackParameterOthersArgument1(Urb);
    }

    Request->VerifierSetFormatted();
}

NTSTATUS
FxFormatUrbRequest(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxIoTarget* Target,
    __in FxRequestBase* Request,
    __in FxRequestBuffer* Buffer,
    __in FX_URB_TYPE FxUrbType,
    __drv_when(FxUrbType == FxUrbTypeUsbdAllocated, __in)
    __drv_when(FxUrbType != FxUrbTypeUsbdAllocated, __in_opt)
         USBD_HANDLE UsbdHandle
    )
{
    FxUsbUrbContext* pContext;
    NTSTATUS status;

    status = Request->ValidateTarget(Target);
    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGIOTARGET,
                            "FormatUrbRequest:  Target %p, Request %p, "
                            "setting target failed, %!STATUS!",
                            Target, Request, status);

        return status;
    }

    if (Request->HasContextType(FX_RCT_USB_URB_REQUEST)) {
        pContext = (FxUsbUrbContext*) Request->GetContext();
    }
    else {
        pContext = new(Target->GetDriverGlobals()) FxUsbUrbContext();

        if (pContext == NULL) {
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        Request->SetContext(pContext);
    }

    //
    // Always set the memory after determining the context.  This way we can
    // free a previously referenced memory object if necessary.
    //
    pContext->StoreAndReferenceMemory(Buffer);

    FxFormatUsbRequest(Request, pContext->m_pUrb, FxUrbType, UsbdHandle);

    return STATUS_SUCCESS;
}

NTSTATUS
FxUsbValidateConfigDescriptorHeaders(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    __in size_t ConfigDescriptorLength
    )
{
    PUCHAR pCur, pEnd;

    pCur = (PUCHAR) ConfigDescriptor;
    pEnd = pCur + ConfigDescriptorLength;

    while (pCur < pEnd) {
        PUSB_COMMON_DESCRIPTOR pDescriptor = (PUSB_COMMON_DESCRIPTOR) pCur;

        //
        // Make sure we can safely deref bLength, bDescriptorType
        //
        if (pCur + sizeof(USB_COMMON_DESCRIPTOR) > pEnd) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "USB Configuration packet contains bad data, expected "
                "at least %d remaining bytes in config descriptor at "
                "offset %I64d, total size is %I64d",
                sizeof(USB_COMMON_DESCRIPTOR), pCur-(PUCHAR)ConfigDescriptor,
                ConfigDescriptorLength
                );
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Make sure bLength is within bounds of the config descriptor
        //
        if ((pCur + pDescriptor->bLength) > pEnd) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "USB Configuration packet contains bad data, descriptor at offset "
                "%I64d specified bLength %d, only %I64d bytes remaining in config "
                "descriptor, total size is %I64d",
                pCur-(PUCHAR)ConfigDescriptor, pDescriptor->bLength, pEnd-pCur,
                ConfigDescriptorLength
                );
            return STATUS_INVALID_PARAMETER;
        }

        if (pDescriptor->bLength == 0) {
            DoTraceLevelMessage(
                FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                "USB Configuration packet contains bad data, descriptor at offset "
                "%I64d contains bLength == 0, this is due to a broken device or driver",
                pCur - (PUCHAR) ConfigDescriptor
                );
            return STATUS_INVALID_PARAMETER;
        }


        pCur += pDescriptor->bLength;
    }

    return STATUS_SUCCESS;
}


PUSB_COMMON_DESCRIPTOR
FxUsbFindDescriptorType(
    __in PVOID Buffer,
    __in size_t BufferLength,
    __in PVOID Start,
    __in LONG DescriptorType
    )
{
    PUSB_COMMON_DESCRIPTOR descriptor;
    PUCHAR pCur, pEnd;

    pCur = (PUCHAR) Start;
    pEnd = ((PUCHAR) Buffer) + BufferLength;

    while (pCur < pEnd) {
        //
        // Check to see if the current point is the desired descriptor type
        //
        descriptor = (PUSB_COMMON_DESCRIPTOR) pCur;

        if (descriptor->bDescriptorType == DescriptorType) {
            return descriptor;
        }

        pCur += descriptor->bLength;
    }

    //
    // Iterated to the end and found nothing
    //
    return NULL;
}

NTSTATUS
FxUsbValidateDescriptorType(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    __in PVOID Start,
    __in PVOID End,
    __in LONG DescriptorType,
    __in size_t SizeToValidate,
    __in FxUsbValidateDescriptorOp Op,
    __in ULONG MaximumNumDescriptorsToValidate
    )
/*++

Routine Description:
    Validates the size of all descriptors within [Start, End] are of the correct
    size.  Correct can either be == or >= SizeToValidate. Furthermore, the caller
    can specify that validation only occur for the first N descriptors found

Arguments:
    FxDriverDriverGlobals - client driver globals
    ConfigDescriptor - the entire config descriptor of the usb device
    Start - the beginning of the buffer to start to validating descriptors within
    End - end of the buffer of validation
    DescriptorType - the type of descriptor to validate
    SizeToValidate - the size of the descriptor required. Op determines if the
                     check is == or >=
    Op - the operation, == or >=, to perform for the size validation
    MaximumNumDescriptorsToValidate - limit of the number of descriptors to validate,
                                      if zero, all instances are validated

Return Value:
    NTSTATUS - !NT_SUCCESS if validation fails, NT_SUCCESS upon success

--*/
{
    PUSB_COMMON_DESCRIPTOR pDescriptor = NULL;
    PVOID pCur = Start;
    ULONG i = 1;

    while ((pDescriptor = FxUsbFindDescriptorType(Start,
                                                  (PUCHAR)End-(PUCHAR)Start,
                                                  pCur,
                                                  DescriptorType
                                                  )) != NULL) {
        //
        // Make sure bLength is the correct value
        //
        if (Op == FxUsbValidateDescriptorOpEqual) {
            if (pDescriptor->bLength != SizeToValidate) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "USB Configuration packet contains bad data, found descriptor "
                    "#%d of type %d at offset %I64d, expected bLength of %I64d, found %d",
                    i, DescriptorType, ((PUCHAR)pDescriptor)-((PUCHAR)ConfigDescriptor),
                    SizeToValidate, pDescriptor->bLength
                    );
                return STATUS_INVALID_PARAMETER;
            }
        }
        else if (Op == FxUsbValidateDescriptorOpAtLeast) {
            if (pDescriptor->bLength < SizeToValidate) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "USB Configuration packet contains bad data, found descriptor "
                    "#%d of type %d at offset %I64d, expected minimum bLength of %I64d, "
                    "found %d",
                    i, DescriptorType, ((PUCHAR)pDescriptor)-((PUCHAR)ConfigDescriptor),
                    SizeToValidate, pDescriptor->bLength
                    );
                return STATUS_INVALID_PARAMETER;
            }
        }

        //
        // No need to validate WDF_PTR_ADD_OFFSET(pDescriptor, pDescriptor->bLength) > End
        // because FxUsbValidateConfigDescriptorHeaders already has done so. End is either
        // 1) == end of the config descriptor, in which case FxUsbValidateConfigDescriptorHeaders
        //    directly validated bLength
        // or
        // 2) End is somewere in the middle of the config descriptor and ASSUMED to be the start
        //    of a common descriptor header. To find that value of End, we would have had to use
        //    pDescriptor->bLength to get to End, we would not overrun
        //

        //
        // i is one based, so the current value is Nth descriptor we have validated. If
        // the caller wants to limit the number of descriptors validated by indicating a
        // Max > 0, stop with success if the condition is met.
        //
        if (MaximumNumDescriptorsToValidate > 0 && i == MaximumNumDescriptorsToValidate) {
            break;
        }

        pCur = WDF_PTR_ADD_OFFSET(pDescriptor, pDescriptor->bLength);
        i++;
    }

    return STATUS_SUCCESS;
}

PUSB_INTERFACE_DESCRIPTOR
FxUsbParseConfigurationDescriptor(
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    __in UCHAR InterfaceNumber,
    __in UCHAR AlternateSetting
    )
{
    PUSB_INTERFACE_DESCRIPTOR found, usbInterface;
    PVOID pStart;

    found = NULL,

    ASSERT(ConfigDesc->bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE);

    //
    // Walk the table of descriptors looking for an interface descriptor with
    // parameters matching those passed in.
    //
    pStart = ConfigDesc;

    do {
        //
        // Search for descriptor type 'interface'
        //
        usbInterface = (PUSB_INTERFACE_DESCRIPTOR)
            FxUsbFindDescriptorType(ConfigDesc,
                                    ConfigDesc->wTotalLength,
                                    pStart,
                                    USB_INTERFACE_DESCRIPTOR_TYPE);

        //
        // Check to see if have a matching descriptor by eliminating mismatches
        //
        if (usbInterface != NULL) {
            found = usbInterface;

            if (InterfaceNumber != -1 &&
                usbInterface->bInterfaceNumber != InterfaceNumber) {
                found = NULL;
            }

            if (AlternateSetting != -1 &&
                usbInterface->bAlternateSetting != AlternateSetting) {
                found = NULL;
            }

            pStart = ((PUCHAR)usbInterface) + usbInterface->bLength;
        }

        if (found != NULL) {
            break;
        }
    } while (usbInterface!= NULL);

    return found;
}

PURB
FxUsbCreateConfigRequest(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    __in PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    __in ULONG DefaultMaxPacketSize
    )
{
    PURB urb;
    PUSBD_INTERFACE_LIST_ENTRY pList;
    USHORT size;

    //
    // Our mission here is to construct a URB of the proper
    // size and format for a select_configuration request.
    //
    // This function uses the configuration descriptor as a
    // reference and builds a URB with interface_information
    // structures for each interface requested in the interface
    // list passed in
    //
    // NOTE: the config descriptor may contain interfaces that
    // the caller does not specify in the list -- in this case
    // the other interfaces will be ignored.
    //

    //
    // First figure out how many interfaces we are dealing with
    //
    pList = InterfaceList;

    //
    // For a multiple interface configuration, GET_SELECT_CONFIGURATION_REQUEST_SIZE
    // doesn't work as expected.  This is because it subtracts one from totalPipes
    // to compensate for the embedded USBD_PIPE_INFORMATION in USBD_INTERFACE_INFORMATION.
    // A multiple interface device will have more then one embedded
    // USBD_PIPE_INFORMATION in its configuration request and any of these interfaces
    // might not have any pipes on them.
    //
    // To fix, just iterate and compute the size incrementally.
    //
    if (InterfaceList[0].InterfaceDescriptor != NULL) {
        size = sizeof(_URB_SELECT_CONFIGURATION) - sizeof(USBD_INTERFACE_INFORMATION);

        while (pList->InterfaceDescriptor != NULL) {
            NTSTATUS status;

            status = RtlUShortAdd(size,
                                  GET_USBD_INTERFACE_SIZE(
                                        pList->InterfaceDescriptor->bNumEndpoints),
                                  &size);
            if (!NT_SUCCESS(status)) {
                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGIOTARGET,
                    "InterfaceList %p, NumEndPoints 0x%x, "
                    "Integer overflow while calculating interface size, %!STATUS!",
                    InterfaceList,
                    pList->InterfaceDescriptor->bNumEndpoints,
                    status);
                return NULL;
            }
            pList++;
        }
    }
    else {
        size = sizeof(_URB_SELECT_CONFIGURATION);
    }

    //
    // Make sure we are dealing with a value that fits into a USHORT
    //
    ASSERT(size <= 0xFFFF);

    urb = (PURB) FxPoolAllocate(FxDriverGlobals, NonPagedPool, size);

    if (urb != NULL) {
        PUCHAR pCur;

        //
        // now all we have to do is initialize the urb
        //
        RtlZeroMemory(urb, size);

        pList = InterfaceList;

        pCur = (PUCHAR) &urb->UrbSelectConfiguration.Interface;
        while (pList->InterfaceDescriptor != NULL) {
            PUSB_INTERFACE_DESCRIPTOR pInterfaceDesc;
            PUSBD_INTERFACE_INFORMATION pInterfaceInfo;

            pInterfaceDesc = pList->InterfaceDescriptor;

            pInterfaceInfo = (PUSBD_INTERFACE_INFORMATION) pCur;
#pragma prefast(suppress: __WARNING_BUFFER_OVERFLOW, "Esp:675");
            pInterfaceInfo->InterfaceNumber = pInterfaceDesc->bInterfaceNumber;
            pInterfaceInfo->AlternateSetting = pInterfaceDesc->bAlternateSetting;
            pInterfaceInfo->NumberOfPipes = pInterfaceDesc->bNumEndpoints;
            pInterfaceInfo->Length = (USHORT)
                GET_USBD_INTERFACE_SIZE(pInterfaceDesc->bNumEndpoints);

            for (LONG j = 0; j < pInterfaceDesc->bNumEndpoints; j++) {
                pInterfaceInfo->Pipes[j].PipeFlags = 0;
                pInterfaceInfo->Pipes[j].MaximumTransferSize = DefaultMaxPacketSize;
            }

            ASSERT(pCur + pInterfaceInfo->Length <= ((PUCHAR) urb) + size);

            pList->Interface = pInterfaceInfo;

            pList++;
            pCur += pInterfaceInfo->Length;
        }

        urb->UrbHeader.Length   = size;
        urb->UrbHeader.Function = URB_FUNCTION_SELECT_CONFIGURATION;
        urb->UrbSelectConfiguration.ConfigurationDescriptor = ConfigDesc;
    }

    return urb;
}

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
VOID
FxUsbUmFormatRequest(
    __in FxRequestBase* Request,
    __in_xcount(Urb->Length) PUMURB_HEADER Urb,
    __in IWudfFile* HostFile,
    __in BOOLEAN Reuse
    )
/*++

Routine Description:
    Formats a request to become a USB request.
    The next stack location is formatted with the UM URB
    and dispatcher is set to be the USB dispatcher

Arguments:
    Request - Request to be formatted
    Urb - UM URB the Request is to be formatted with
          the pointer is passed as the header so
          that SAL checkers don't complain on bigger size usage than what's passed in
          (Total size of the URB union can be more than the the specific member that is passed in)
    File - The host file used for internal I/O.

Return Value:
    None

--*/
{
    FX_VERIFY(INTERNAL, CHECK_NOT_NULL(Request));
    FX_VERIFY(INTERNAL, CHECK_NOT_NULL(Urb));
    FX_VERIFY(INTERNAL, CHECK_TODO(Urb->Length >= sizeof(_UMURB_HEADER)));
    FX_VERIFY(INTERNAL, CHECK_NOT_NULL(HostFile));

    IWudfIoIrp* pIoIrp = NULL;
    IWudfIrp* pIrp = Request->GetSubmitIrp();

    if (Reuse) {
        //
        // This allows us to use the same FxSyncRequest
        // stack object multiple times.
        //
        Request->GetSubmitFxIrp()->Reuse(STATUS_SUCCESS);
        Request->ClearFieldsForReuse();
    }

    HRESULT hrQI = pIrp->QueryInterface(IID_IWudfIoIrp, (PVOID*)&pIoIrp);
    FX_VERIFY(INTERNAL, CHECK_QI(hrQI, pIoIrp));

    pIoIrp->SetTypeForNextStackLocation(UMINT::WdfRequestInternalIoctl);




    pIoIrp->SetDeviceIoControlParametersForNextStackLocation(IOCTL_INETRNAL_USB_SUBMIT_UMURB,
                                                             0,
                                                             0);

    pIoIrp->SetOtherParametersForNextStackLocation((PVOID*)&Urb,
                                                   NULL,
                                                   NULL,
                                                   NULL);

    pIoIrp->SetFileForNextIrpStackLocation(HostFile);

    //
    // Release reference taken by QI
    //
    SAFE_RELEASE(pIoIrp);
    Request->VerifierSetFormatted();
}

VOID
FxUsbUmInitDescriptorUrb(
    __inout PUMURB UmUrb,
    __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
    __in UCHAR DescriptorType,
    __in ULONG BufferLength,
    __in PVOID Buffer
    )
{
    RtlZeroMemory(UmUrb, sizeof(UMURB));

    UmUrb->UmUrbDescriptorRequest.Hdr.InterfaceHandle = WinUsbHandle;
    UmUrb->UmUrbDescriptorRequest.Hdr.Function = UMURB_FUNCTION_GET_DESCRIPTOR;
    UmUrb->UmUrbDescriptorRequest.Hdr.Length = sizeof(_UMURB_DESCRIPTOR_REQUEST);

    UmUrb->UmUrbDescriptorRequest.DescriptorType = DescriptorType;
    UmUrb->UmUrbDescriptorRequest.Index = 0;
    UmUrb->UmUrbDescriptorRequest.LanguageID = 0;
    UmUrb->UmUrbDescriptorRequest.BufferLength = BufferLength;
    UmUrb->UmUrbDescriptorRequest.Buffer = Buffer;
}

VOID
FxUsbUmInitControlTransferUrb(
    __inout PUMURB UmUrb,
    __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
    __in ULONG BufferLength,
    __in PVOID Buffer
    )
{
    RtlZeroMemory(UmUrb, sizeof(UMURB));

    UmUrb->UmUrbControlTransfer.Hdr.InterfaceHandle = WinUsbHandle;
    UmUrb->UmUrbControlTransfer.Hdr.Function = UMURB_FUNCTION_CONTROL_TRANSFER;
    UmUrb->UmUrbControlTransfer.Hdr.Length = sizeof(_UMURB_CONTROL_TRANSFER);

    UmUrb->UmUrbControlTransfer.TransferBufferLength = BufferLength;
    UmUrb->UmUrbControlTransfer.TransferBuffer = Buffer;
}

VOID
FxUsbUmInitInformationUrb(
    __inout PUMURB UmUrb,
    __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
    __in ULONG BufferLength,
    __in PVOID Buffer
    )
{
    RtlZeroMemory(UmUrb, sizeof(UMURB));

    UmUrb->UmUrbDeviceInformation.Hdr.InterfaceHandle = WinUsbHandle;
    UmUrb->UmUrbDeviceInformation.Hdr.Function = UMURB_FUNCTION_GET_DEVICE_INFORMATION;
    UmUrb->UmUrbDeviceInformation.Hdr.Length = sizeof(_UMURB_DEVICE_INFORMATION);

    UmUrb->UmUrbDeviceInformation.InformationType = DEVICE_SPEED;
    UmUrb->UmUrbDeviceInformation.BufferLength = BufferLength;
    UmUrb->UmUrbDeviceInformation.Buffer = Buffer;
}
#endif

