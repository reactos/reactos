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
                                           PUSB_DEFAULT_PIPE_SETUP_PACKET *CtrlSetup,
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
    *CtrlSetup = (PUSB_DEFAULT_PIPE_SETUP_PACKET) (( (ULONG)(*CtrlTD3) + sizeof(QUEUE_TRANSFER_DESCRIPTOR) + 0xFFF)  & ~0xFFF);
    *CtrlData = (PVOID) (( (ULONG)(*CtrlSetup) + sizeof(USB_DEFAULT_PIPE_SETUP_PACKET) + 0xFFF)  & ~0xFFF);

    (*CtrlTD1)->NextPointer = TERMINATE_POINTER;
    (*CtrlTD1)->AlternateNextPointer = TERMINATE_POINTER;
    (*CtrlTD1)->BufferPointer[0] = (ULONG)MmGetPhysicalAddress((PVOID) (*CtrlSetup)).LowPart;
    (*CtrlTD1)->Token.Bits.DataToggle = FALSE;
    (*CtrlTD1)->Token.Bits.InterruptOnComplete = FALSE;
    (*CtrlTD1)->Token.Bits.TotalBytesToTransfer = sizeof(USB_DEFAULT_PIPE_SETUP_PACKET);
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
ExecuteControlRequest(PFDO_DEVICE_EXTENSION DeviceExtension, PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, UCHAR Address, ULONG Port, PVOID Buffer, ULONG BufferLength)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup = NULL;
    PVOID CtrlData = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD1 = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD2 = NULL;
    PQUEUE_TRANSFER_DESCRIPTOR CtrlTD3 = NULL;

    PQUEUE_HEAD QueueHead;
    PEHCI_USBCMD_CONTENT UsbCmd;
    PEHCI_USBSTS_CONTEXT UsbSts;
    LONG Base;
    LONG tmp;

    DPRINT1("ExecuteControlRequest: Buffer %x, Length %x\n", Buffer, BufferLength);

    Base = (ULONG) DeviceExtension->ResourceMemory;

    /* Set up the QUEUE HEAD in memory */
    QueueHead = (PQUEUE_HEAD) ((ULONG)DeviceExtension->AsyncListQueueHeadPtr);

    /* Initialize the memory pointers */
    IntializeHeadQueueForStandardRequest(QueueHead,
                                         &CtrlTD1,
                                         &CtrlTD2,
                                         &CtrlTD3,
                                         &CtrlSetup,
                                         (PVOID)&CtrlData,
                                         BufferLength);

    CtrlSetup->bmRequestType._BM.Recipient = SetupPacket->bmRequestType._BM.Recipient;
    CtrlSetup->bmRequestType._BM.Type = SetupPacket->bmRequestType._BM.Type;
    CtrlSetup->bmRequestType._BM.Dir = SetupPacket->bmRequestType._BM.Dir;
    CtrlSetup->bRequest = SetupPacket->bRequest;
    CtrlSetup->wValue.LowByte = SetupPacket->wValue.LowByte;
    CtrlSetup->wValue.HiByte = SetupPacket->wValue.HiByte;
    CtrlSetup->wIndex.W = SetupPacket->wIndex.W;
    CtrlSetup->wLength = SetupPacket->wLength;


    QueueHead->EndPointCapabilities1.DeviceAddress = Address;
    //QueueHead->EndPointCapabilities2.PortNumber = Port;

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

    if (CtrlSetup->bmRequestType._BM.Dir == BMREQUEST_DEVICE_TO_HOST)
    {
        if ((Buffer) && (BufferLength))
        {
            RtlCopyMemory(Buffer, CtrlData, BufferLength);
        }
        else
            DPRINT1("Unable to copy data to buffer\n");
    }

    return TRUE;
}
