/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/urbreq.c
 * PURPOSE:     URB Related Functions.
 * PROGRAMMERS:
 *              Michael Martin
 */

#include "usbehci.h"

/* QueueHead must point to the base address in common buffer */
PVOID
IntializeHeadQueueForStandardRequest(PQUEUE_HEAD QueueHead,
                                           PQUEUE_TRANSFER_DESCRIPTOR *CtrlTD1,
                                           PQUEUE_TRANSFER_DESCRIPTOR *CtrlTD2,
                                           PQUEUE_TRANSFER_DESCRIPTOR *CtrlTD3,
                                           PEHCI_SETUP_FORMAT *CtrlSetup,
                                           PVOID *CtrlData,
                                           ULONG Size)
{
    QueueHead->HorizontalLinkPointer = (ULONG)QueueHead | QH_TYPE_QH;

    /* Set NakCountReload to max value possible */
    QueueHead->EndPointCapabilities1.NakCountReload = 0xF;

    /* 1 for non high speed, 0 for high speed device */
    QueueHead->EndPointCapabilities1.ControlEndPointFlag = 0;

    QueueHead->EndPointCapabilities1.HeadOfReclamation = TRUE;
    QueueHead->EndPointCapabilities1.MaximumPacketLength = 64;

    /* Get the Initial Data Toggle from the QEDT */
    QueueHead->EndPointCapabilities1.QEDTDataToggleControl = TRUE;

    /* HIGH SPEED DEVICE */
    QueueHead->EndPointCapabilities1.EndPointSpeed = QH_ENDPOINT_HIGHSPEED;
    QueueHead->EndPointCapabilities1.EndPointNumber = 0;

    /* Only used for Periodic Schedule and Not High Speed devices.
       Setting it to one result in undefined behavior for Async */
    QueueHead->EndPointCapabilities1.InactiveOnNextTransaction = 0;

    QueueHead->EndPointCapabilities1.DeviceAddress = 0;
    QueueHead->EndPointCapabilities2.HubAddr = 0;
    QueueHead->EndPointCapabilities2.NumberOfTransactionPerFrame = 0x01;

    /* 0 For Async, 1 for periodoc schedule */
    QueueHead->EndPointCapabilities2.InterruptScheduleMask = 0;

    QueueHead->EndPointCapabilities2.PortNumber = 0;
    QueueHead->EndPointCapabilities2.SplitCompletionMask = 0;
    QueueHead->CurrentLinkPointer = 0;
    QueueHead->QETDPointer = TERMINATE_POINTER;
    QueueHead->AlternateNextPointer = TERMINATE_POINTER;
    QueueHead->Token.Bits.InterruptOnComplete = TRUE;
    QueueHead->BufferPointer[0] = 0;
    QueueHead->BufferPointer[1] = 0;
    QueueHead->BufferPointer[2] = 0;
    QueueHead->BufferPointer[3] = 0;
    QueueHead->BufferPointer[4] = 0;
    /* Queue Head Initialized */

    /* Queue Transfer Descriptors must be 32 byte aligned and reside in continous physical memory */
    *CtrlTD1 = (PQUEUE_TRANSFER_DESCRIPTOR) (((ULONG)(QueueHead) + sizeof(QUEUE_HEAD) + 0x1F) & ~0x1F);
    *CtrlTD2 = (PQUEUE_TRANSFER_DESCRIPTOR) (((ULONG)(*CtrlTD1) + sizeof(QUEUE_TRANSFER_DESCRIPTOR) + 0x1F) & ~0x1F);
    *CtrlTD3 = (PQUEUE_TRANSFER_DESCRIPTOR) (((ULONG)(*CtrlTD2) + sizeof(QUEUE_TRANSFER_DESCRIPTOR) + 0x1F) & ~0x1F);

    /* Must be Page aligned */
    *CtrlSetup = (PEHCI_SETUP_FORMAT) (( (ULONG)(*CtrlTD3) + sizeof(QUEUE_TRANSFER_DESCRIPTOR) + 0xFFF)  & ~0xFFF);
    *CtrlData = (PUSB_DEVICE_DESCRIPTOR) (( (ULONG)(*CtrlSetup) + sizeof(EHCI_SETUP_FORMAT) + 0xFFF)  & ~0xFFF);

    (*CtrlTD1)->NextPointer = TERMINATE_POINTER;
    (*CtrlTD1)->AlternateNextPointer = TERMINATE_POINTER;
    (*CtrlTD1)->BufferPointer[0] = (ULONG)MmGetPhysicalAddress((PVOID) (*CtrlSetup)).LowPart;
    (*CtrlTD1)->Token.Bits.DataToggle = FALSE;
    (*CtrlTD1)->Token.Bits.InterruptOnComplete = FALSE;
    (*CtrlTD1)->Token.Bits.TotalBytesToTransfer = sizeof(EHCI_SETUP_FORMAT);
    (*CtrlTD1)->Token.Bits.ErrorCounter = 0x03;
    (*CtrlTD1)->Token.Bits.PIDCode = PID_CODE_SETUP_TOKEN;
    (*CtrlTD1)->Token.Bits.Active = TRUE;

    (*CtrlTD2)->NextPointer = TERMINATE_POINTER;
    (*CtrlTD2)->AlternateNextPointer = TERMINATE_POINTER;
    (*CtrlTD2)->BufferPointer[0] = (ULONG)MmGetPhysicalAddress((PVOID) (*CtrlData)).LowPart;
    (*CtrlTD2)->Token.Bits.DataToggle = TRUE;
    (*CtrlTD2)->Token.Bits.InterruptOnComplete = TRUE;
    (*CtrlTD2)->Token.Bits.TotalBytesToTransfer = Size;
    (*CtrlTD2)->Token.Bits.ErrorCounter = 0x03;
    (*CtrlTD2)->Token.Bits.PIDCode = PID_CODE_IN_TOKEN;
    (*CtrlTD2)->Token.Bits.Active = TRUE;

    (*CtrlTD1)->NextPointer = (ULONG)MmGetPhysicalAddress((PVOID)(*CtrlTD2)).LowPart;

    (*CtrlTD3)->NextPointer = TERMINATE_POINTER;
    (*CtrlTD3)->AlternateNextPointer = TERMINATE_POINTER;
    (*CtrlTD3)->BufferPointer[0] = 0;
    (*CtrlTD3)->Token.Bits.Active = TRUE;
    (*CtrlTD3)->Token.Bits.PIDCode = PID_CODE_OUT_TOKEN;
    (*CtrlTD3)->Token.Bits.InterruptOnComplete = TRUE;
    (*CtrlTD3)->Token.Bits.DataToggle = TRUE;
    (*CtrlTD3)->Token.Bits.ErrorCounter = 0x03;
    (*CtrlTD2)->NextPointer = (ULONG) MmGetPhysicalAddress((PVOID)(*CtrlTD3)).LowPart;
    return NULL;
}

BOOLEAN
GetDeviceDescriptor(PFDO_DEVICE_EXTENSION DeviceExtension, UCHAR Index, PUSB_DEVICE_DESCRIPTOR OutBuffer, BOOLEAN Hub)
{
    PEHCI_SETUP_FORMAT CtrlSetup = NULL;
    PUSB_DEVICE_DESCRIPTOR CtrlData = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD1 = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD2 = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD3 = NULL;
    PQUEUE_HEAD QueueHead;
    PEHCI_USBCMD_CONTENT UsbCmd;
    PEHCI_USBSTS_CONTEXT UsbSts;
    LONG Base;
    LONG tmp;

    Base = (ULONG) DeviceExtension->ResourceMemory;

    /* Set up the QUEUE HEAD in memory */
    QueueHead = (PQUEUE_HEAD) ((ULONG)DeviceExtension->AsyncListQueueHeadPtr);

    IntializeHeadQueueForStandardRequest(QueueHead,
                                         &CtrlTD1,
                                         &CtrlTD2,
                                         &CtrlTD3,
                                         &CtrlSetup,
                                         (PVOID)&CtrlData,
                                         sizeof(USB_DEVICE_DESCRIPTOR));

    /* FIXME: Use defines and handle other than Device Desciptors */
    if (Hub)
    {
        CtrlSetup->bmRequestType = 0x80;
        CtrlSetup->wValue = 0x0600;
    }
    else
    {
        CtrlSetup->bmRequestType = 0x80;
        CtrlSetup->wValue = 0x0100;
    }
    CtrlSetup->bRequest = 0x06;
    CtrlSetup->wIndex = 0;
    CtrlSetup->wLength = sizeof(USB_DEVICE_DESCRIPTOR);

    tmp = READ_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
    UsbCmd->Run = FALSE;
    WRITE_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD), tmp);

    /* Wait for the controller to halt */
    for (;;)
    {
        KeStallExecutionProcessor(10);
        tmp = READ_REGISTER_ULONG((PULONG)(Base + EHCI_USBSTS));
        UsbSts = (PEHCI_USBSTS_CONTEXT)&tmp;
        DPRINT("Waiting for Halt, USBSTS: %x\n", READ_REGISTER_ULONG ((PULONG)(Base + EHCI_USBSTS)));
        if (UsbSts->HCHalted)
        {
            break;
        }
    }

    /* Set to TRUE on interrupt for async completion */
    DeviceExtension->AsyncComplete = FALSE;
    QueueHead->QETDPointer = (ULONG) MmGetPhysicalAddress((PVOID)(CtrlTD1)).LowPart;

    tmp = READ_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
    UsbCmd->AsyncEnable = TRUE;

    WRITE_REGISTER_ULONG((PULONG)(Base + EHCI_USBCMD), tmp);

    tmp = READ_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;

    /* Interrupt on Async completion */
    UsbCmd->DoorBell = TRUE;
    UsbCmd->Run = TRUE;
    WRITE_REGISTER_ULONG((PULONG)(Base + EHCI_USBCMD), tmp);

    for (;;)
    {
        KeStallExecutionProcessor(10);
        DPRINT("Waiting for completion!\n");
        if (DeviceExtension->AsyncComplete == TRUE)
            break;
    }

    if (OutBuffer != NULL)
    {
        OutBuffer->bLength = CtrlData->bLength;
        OutBuffer->bDescriptorType = CtrlData->bDescriptorType;
        OutBuffer->bcdUSB = CtrlData->bcdUSB;
        OutBuffer->bDeviceClass = CtrlData->bDeviceClass;
        OutBuffer->bDeviceSubClass = CtrlData->bDeviceSubClass;
        OutBuffer->bDeviceProtocol = CtrlData->bDeviceProtocol;
        OutBuffer->bMaxPacketSize0 = CtrlData->bMaxPacketSize0;
        OutBuffer->idVendor = CtrlData->idVendor;
        OutBuffer->idProduct = CtrlData->idProduct;
        OutBuffer->bcdDevice = CtrlData->bcdDevice;
        OutBuffer->iManufacturer = CtrlData->iManufacturer;
        OutBuffer->iProduct = CtrlData->iProduct;
        OutBuffer->iSerialNumber = CtrlData->iSerialNumber;
        OutBuffer->bNumConfigurations = CtrlData->bNumConfigurations;
    }

    DPRINT1("bLength %d\n", CtrlData->bLength);
    DPRINT1("bDescriptorType %x\n", CtrlData->bDescriptorType);
    DPRINT1("bcdUSB %x\n", CtrlData->bcdUSB);
    DPRINT1("CtrlData->bDeviceClass %x\n", CtrlData->bDeviceClass);
    DPRINT1("CtrlData->bDeviceSubClass %x\n", CtrlData->bDeviceSubClass);
    DPRINT1("CtrlData->bDeviceProtocal %x\n", CtrlData->bDeviceProtocol);
    DPRINT1("CtrlData->bMaxPacketSize %x\n", CtrlData->bMaxPacketSize0);
    DPRINT1("CtrlData->idVendor %x\n", CtrlData->idVendor);
    DPRINT1("CtrlData->idProduct %x\n", CtrlData->idProduct);
    DPRINT1("CtrlData->bcdDevice %x\n", CtrlData->bcdDevice);
    DPRINT1("CtrlData->iManufacturer %x\n", CtrlData->iManufacturer);
    DPRINT1("CtrlData->iProduct %x\n", CtrlData->iProduct);
    DPRINT1("CtrlData->iSerialNumber %x\n", CtrlData->iSerialNumber);
    DPRINT1("CtrlData->bNumConfigurations %x\n", CtrlData->bNumConfigurations);

    /* Temporary: Remove */
    if (CtrlData->bLength > 0)
    {
        /* We got valid data, try for strings */
        UCHAR Manufacturer = CtrlData->iManufacturer;
        UCHAR Product = CtrlData->iProduct;
        UCHAR SerialNumber = CtrlData->iSerialNumber;

        GetDeviceStringDescriptor(DeviceExtension, Manufacturer);
        GetDeviceStringDescriptor(DeviceExtension, Product);
        GetDeviceStringDescriptor(DeviceExtension, SerialNumber);
    }

    return TRUE;
}

BOOLEAN
GetDeviceStringDescriptor(PFDO_DEVICE_EXTENSION DeviceExtension, UCHAR Index)
{
    PEHCI_SETUP_FORMAT CtrlSetup = NULL;
    PSTRING_DESCRIPTOR CtrlData = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD1 = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD2 = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD3 = NULL;
    PQUEUE_HEAD QueueHead;
    PEHCI_USBCMD_CONTENT UsbCmd;
    PEHCI_USBSTS_CONTEXT UsbSts;
    LONG Base;
    LONG tmp;

    Base = (ULONG) DeviceExtension->ResourceMemory;

    /* Set up the QUEUE HEAD in memory */
    QueueHead = (PQUEUE_HEAD) ((ULONG)DeviceExtension->AsyncListQueueHeadPtr);

    IntializeHeadQueueForStandardRequest(QueueHead,
                                         &CtrlTD1,
                                         &CtrlTD2,
                                         &CtrlTD3,
                                         &CtrlSetup,
                                         (PVOID)&CtrlData,
                                         sizeof(STRING_DESCRIPTOR) + 256);

    /* FIXME: Use defines and handle other than Device Desciptors */
    CtrlSetup->bmRequestType = 0x80;
    CtrlSetup->bRequest = 0x06;
    CtrlSetup->wValue = 0x0300 | Index;
    CtrlSetup->wIndex = 0;
    /* 256 pulled from thin air */
    CtrlSetup->wLength = sizeof(STRING_DESCRIPTOR) + 256;

    tmp = READ_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
    UsbCmd->Run = FALSE;
    WRITE_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD), tmp);

    /* Wait for the controller to halt */
    for (;;)
    {
        KeStallExecutionProcessor(10);
        tmp = READ_REGISTER_ULONG((PULONG)(Base + EHCI_USBSTS));
        UsbSts = (PEHCI_USBSTS_CONTEXT)&tmp;
        DPRINT("Waiting for Halt, USBSTS: %x\n", READ_REGISTER_ULONG ((PULONG)(Base + EHCI_USBSTS)));
        if (UsbSts->HCHalted)
        {
            break;
        }
    }

    /* Set to TRUE on interrupt for async completion */
    DeviceExtension->AsyncComplete = FALSE;
    QueueHead->QETDPointer = (ULONG) MmGetPhysicalAddress((PVOID)(CtrlTD1)).LowPart;

    tmp = READ_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;
    UsbCmd->AsyncEnable = TRUE;

    WRITE_REGISTER_ULONG((PULONG)(Base + EHCI_USBCMD), tmp);

    tmp = READ_REGISTER_ULONG((PULONG) (Base + EHCI_USBCMD));
    UsbCmd = (PEHCI_USBCMD_CONTENT) &tmp;

    /* Interrupt on Async completion */
    UsbCmd->DoorBell = TRUE;
    UsbCmd->Run = TRUE;
    WRITE_REGISTER_ULONG((PULONG)(Base + EHCI_USBCMD), tmp);

    for (;;)
    {
        KeStallExecutionProcessor(10);
        DPRINT("Waiting for completion!\n");
        if (DeviceExtension->AsyncComplete == TRUE)
            break;
    }

    DPRINT1("String %S\n", &CtrlData->bString);

    return TRUE;
}

