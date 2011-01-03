/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/transfer.c
 * PURPOSE:     Transfers to EHCI.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#include "transfer.h"
#include <debug.h>

VOID
BuildSetupPacketFromURB(PEHCI_HOST_CONTROLLER hcd, PURB Urb, PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup)
{
    switch (Urb->UrbHeader.Function)
    {
    /* CLEAR FEATURE */
        case URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE:
        case URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE:
        case URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT:
            DPRINT1("Not implemented!\n");
            break;

    /* GET CONFIG */
        case URB_FUNCTION_GET_CONFIGURATION:
            CtrlSetup->bRequest = USB_REQUEST_GET_CONFIGURATION;
            CtrlSetup->bmRequestType.B = 0x80;
            CtrlSetup->wLength = 1;
            break;

    /* GET DESCRIPTOR */
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
            CtrlSetup->bRequest = USB_REQUEST_GET_DESCRIPTOR;
            CtrlSetup->wValue.LowByte = Urb->UrbControlDescriptorRequest.Index;
            CtrlSetup->wValue.HiByte = Urb->UrbControlDescriptorRequest.DescriptorType;
            CtrlSetup->wIndex.W = Urb->UrbControlDescriptorRequest.LanguageId;
            CtrlSetup->wLength = Urb->UrbControlDescriptorRequest.TransferBufferLength;
            CtrlSetup->bmRequestType.B = 0x80;
            break;

    /* GET INTERFACE */
        case URB_FUNCTION_GET_INTERFACE:
            CtrlSetup->bRequest = USB_REQUEST_GET_CONFIGURATION;
            CtrlSetup->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            CtrlSetup->bmRequestType.B = 0x80;
            CtrlSetup->wLength = 1;
            break;

    /* GET STATUS */
        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
            CtrlSetup->bRequest = USB_REQUEST_GET_STATUS;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            CtrlSetup->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            CtrlSetup->bmRequestType.B = 0x80;
            CtrlSetup->wLength = 2;
            break;

    case URB_FUNCTION_GET_STATUS_FROM_INTERFACE:
            CtrlSetup->bRequest = USB_REQUEST_GET_STATUS;
            ASSERT(Urb->UrbControlGetStatusRequest.Index != 0);
            CtrlSetup->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            CtrlSetup->bmRequestType.B = 0x81;
            CtrlSetup->wLength = 2;
            break;

    case URB_FUNCTION_GET_STATUS_FROM_ENDPOINT:
            CtrlSetup->bRequest = USB_REQUEST_GET_STATUS;
            ASSERT(Urb->UrbControlGetStatusRequest.Index != 0);
            CtrlSetup->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            CtrlSetup->bmRequestType.B = 0x82;
            CtrlSetup->wLength = 2;
            break;

    /* SET ADDRESS */

    /* SET CONFIG */
        case URB_FUNCTION_SELECT_CONFIGURATION:
            CtrlSetup->bRequest = USB_REQUEST_SET_CONFIGURATION;
            CtrlSetup->bmRequestType.B = 0x00;
            break;

    /* SET DESCRIPTOR */
        case URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE:
        case URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE:
        case URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT:
            DPRINT1("Not implemented\n");
            break;

    /* SET FEATURE */
        case URB_FUNCTION_SET_FEATURE_TO_DEVICE:
            CtrlSetup->bRequest = USB_REQUEST_SET_FEATURE;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            CtrlSetup->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            CtrlSetup->bmRequestType.B = 0x80;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_INTERFACE:
            CtrlSetup->bRequest = USB_REQUEST_SET_FEATURE;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            CtrlSetup->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            CtrlSetup->bmRequestType.B = 0x81;
            break;

        case URB_FUNCTION_SET_FEATURE_TO_ENDPOINT:
            CtrlSetup->bRequest = USB_REQUEST_SET_FEATURE;
            ASSERT(Urb->UrbControlGetStatusRequest.Index == 0);
            CtrlSetup->wIndex.W = Urb->UrbControlGetStatusRequest.Index;
            CtrlSetup->bmRequestType.B = 0x82;
            break;

    /* SET INTERFACE*/
        case URB_FUNCTION_SELECT_INTERFACE:
            DPRINT1("Not implemented\n");
            break;

    /* SYNC FRAME */
        case URB_FUNCTION_SYNC_RESET_PIPE_AND_CLEAR_STALL:
            DPRINT1("Not implemented\n");
            break;
        default:
            DPRINT1("Unknown USB Request!\n");
            break;
    }
}

BOOLEAN
SubmitControlTransfer(PEHCI_HOST_CONTROLLER hcd,
                      PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup,
                      PVOID TransferBuffer,
                      ULONG TransferBufferLength,
                      PIRP IrpToComplete)
{
    PQUEUE_HEAD QueueHead;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor[3];
    ULONG MdlPhysicalAddr;
    PKEVENT Event = NULL;
    PMDL pMdl = NULL;
    PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetupVA, CtrlPhysicalPA;

    CtrlSetupVA = (PUSB_DEFAULT_PIPE_SETUP_PACKET)AllocateMemory(hcd,
                                                               sizeof(USB_DEFAULT_PIPE_SETUP_PACKET),
                                                               (ULONG*)&CtrlPhysicalPA);

    RtlCopyMemory(CtrlSetupVA, CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));
    /* If no Irp then wait on completion */
    if (IrpToComplete == NULL)
    {
        Event = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
        KeInitializeEvent(Event, NotificationEvent, FALSE);
    }

    /* Allocate Mdl for Buffer */
    pMdl = IoAllocateMdl(TransferBuffer,
                         TransferBufferLength,
                         FALSE,
                         FALSE,
                         NULL);

    /* Lock Physical Pages */
    MmBuildMdlForNonPagedPool(pMdl);
    //MmProbeAndLockPages(pMdl, KernelMode, IoReadAccess);

    MdlPhysicalAddr = MmGetPhysicalAddress((PVOID)TransferBuffer).LowPart;

    QueueHead = CreateQueueHead(hcd);

    Descriptor[0] = CreateDescriptor(hcd,
                                     PID_CODE_SETUP_TOKEN,
                                     sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    Descriptor[0]->Token.Bits.InterruptOnComplete = FALSE;
    Descriptor[0]->Token.Bits.DataToggle = FALSE;

    /* Save the first descriptor */
    QueueHead->TransferDescriptor = Descriptor[0];

    Descriptor[1] = CreateDescriptor(hcd,
                                     PID_CODE_IN_TOKEN,
                                     TransferBufferLength);

    Descriptor[2] = CreateDescriptor(hcd,
                                     PID_CODE_OUT_TOKEN,
                                     0);

    Descriptor[1]->Token.Bits.InterruptOnComplete = FALSE;

    /* Link the descriptors */
    Descriptor[0]->NextDescriptor = Descriptor[1];
    Descriptor[1]->NextDescriptor = Descriptor[2];
    Descriptor[1]->PreviousDescriptor = Descriptor[0];
    Descriptor[2]->PreviousDescriptor = Descriptor[1];

    /* Assign the descritors buffers */
    Descriptor[0]->BufferPointer[0] = (ULONG)CtrlPhysicalPA;
    Descriptor[1]->BufferPointer[0] = MdlPhysicalAddr;

    Descriptor[0]->NextPointer = Descriptor[1]->PhysicalAddr;
    Descriptor[1]->NextPointer = Descriptor[2]->PhysicalAddr;
    QueueHead->NextPointer = Descriptor[0]->PhysicalAddr;

    QueueHead->IrpToComplete = IrpToComplete;
    QueueHead->MdlToFree = pMdl;
    QueueHead->Event = Event;

    /* Link in the QueueHead */
    LinkQueueHead(hcd, QueueHead);

    if (IrpToComplete == NULL)
    {
        DPRINT1("Waiting For Completion %x!\n", Event);
        KeWaitForSingleObject(Event, Suspended, KernelMode, FALSE, NULL);
        ExFreePool(Event);
    }

    return TRUE;
}
