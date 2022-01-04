//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef __FX_USB_UTIL_H__
#define __FX_USB_UTIL_H__

BOOLEAN
__inline
FxBitArraySet(
    __inout_xcount((BitNumber / sizeof(UCHAR)) + 1) PUCHAR BitArray,
    __in UCHAR  BitNumber
    )
/*++

Routine Description:
    The following function sets a bit in the array and returns a value indicating
    the bits previous state.

Arguments:
    BitArray - the array to check

    BitNumber - zero based index into BitArray

Return Value:
    TRUE if the bit was already set
    FALSE if was previously clear

  --*/
{
    UCHAR index;
    UCHAR bit;

    index = (BitNumber) / sizeof(UCHAR);
    bit = (UCHAR) (1 << (BitNumber % sizeof(UCHAR)) );

    if ((BitArray[index] & bit) == 0x0) {
        //
        // bit not set
        //
        BitArray[index] |= bit;
        return FALSE;
    }

    return TRUE;
}

VOID
__inline
FxBitArrayClear(
    __inout_xcount((BitNumber / sizeof(UCHAR)) + 1) PUCHAR BitArray,
    __in UCHAR  BitNumber
    )
/*++

Routine Description:
    Clear the bit in the BitArray

Arguments:
    BitArray - bit array to change

    BitNumber - zero based bit index into the array to clear

  --*/
{
    UCHAR index;
    UCHAR bit;

    index =  BitNumber / sizeof(UCHAR);
    bit = (UCHAR) (1 << (BitNumber % sizeof(UCHAR)) );

    BitArray[index] &= ~bit;
}

VOID
FxFormatUsbRequest(
    __in FxRequestBase* Request,
    __in PURB Urb,
    __in FX_URB_TYPE FxUrbType,
    __drv_when(FxUrbType == FxUrbTypeUsbdAllocated, __in)
    __drv_when(FxUrbType != FxUrbTypeUsbdAllocated, __in_opt)
         USBD_HANDLE UsbdHandle
    );

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
    );

PUSB_INTERFACE_DESCRIPTOR
FxUsbParseConfigurationDescriptor(
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    __in UCHAR InterfaceNumber = -1,
    __in UCHAR AlternateSetting = 1
    );

PURB
FxUsbCreateConfigRequest(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigDesc,
    __in PUSBD_INTERFACE_LIST_ENTRY InterfaceList,
    __in ULONG DefaultMaxPacketSize
    );

NTSTATUS
FxUsbValidateConfigDescriptorHeaders(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor,
    __in size_t ConfigDescriptorLength
    );

PUSB_COMMON_DESCRIPTOR
FxUsbFindDescriptorType(
    __in PVOID Buffer,
    __in size_t BufferLength,
    __in PVOID Start,
    __in LONG DescriptorType
    );

enum FxUsbValidateDescriptorOp {
    FxUsbValidateDescriptorOpEqual,
    FxUsbValidateDescriptorOpAtLeast
};

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
    );

#if (FX_CORE_MODE == FX_CORE_USER_MODE)
VOID
FxUsbUmFormatRequest(
    __in FxRequestBase* Request,
    __in_xcount(Urb->Length) PUMURB_HEADER Urb,
    __in IWudfFile* HostFile,
    __in BOOLEAN Reuse = FALSE
    );

VOID
FxUsbUmInitDescriptorUrb(
    __inout PUMURB UmUrb,
    __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
    __in UCHAR DescriptorType,
    __in ULONG BufferLength,
    __in PVOID Buffer
    );

VOID
FxUsbUmInitControlTransferUrb(
    __inout PUMURB UmUrb,
    __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
    __in ULONG BufferLength,
    __in PVOID Buffer
    );

VOID
FxUsbUmInitInformationUrb(
    __inout PUMURB UmUrb,
    __in WINUSB_INTERFACE_HANDLE WinUsbHandle,
    __in ULONG BufferLength,
    __in PVOID Buffer
    );
#endif // (FX_CORE_MODE == FX_CORE_USER_MODE)

#endif // __FX_USB_UTIL_H__
