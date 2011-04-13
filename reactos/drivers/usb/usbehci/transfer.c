/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/transfer.c
 * PURPOSE:     Transfers to EHCI.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

/* All QueueHeads, Descriptors and pipe setup packets used by control transfers are allocated from common buffer.
   The routines in physmem manages this buffer. Each allocation is aligned to 32bits, which is requirement of USB 2.0. */

#include "transfer.h"
#include <debug.h>


typedef struct _MAPREGISTERCALLBACKINFO
{
    PVOID MapRegisterBase;
    KEVENT Event;
}MAPREGISTERCALLBACKINFO, *PMAPREGISTERCALLBACKINFO;

/* Fills CtrlSetup parameter with values based on Urb */
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
            DPRINT1("Bulding data for Select Config\n");
            CtrlSetup->bRequest = USB_REQUEST_SET_CONFIGURATION;
            CtrlSetup->wValue.W = Urb->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue;
            CtrlSetup->wIndex.W = 0;
            CtrlSetup->wLength = 0;            
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
            CtrlSetup->bRequest = USB_REQUEST_SET_INTERFACE;
            CtrlSetup->wValue.W = Urb->UrbSelectInterface.Interface.AlternateSetting;
            CtrlSetup->wIndex.W = Urb->UrbSelectInterface.Interface.InterfaceNumber;
            CtrlSetup->wLength = 0;
            CtrlSetup->bmRequestType.B = 0x01;
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

/* Build a Bulk Transfer with a max transfer of 3 descriptors which each has 5 pointers to one page of memory, paged align of course */
PQUEUE_HEAD
BuildBulkTransfer(PEHCI_HOST_CONTROLLER hcd,
                   ULONG DeviceAddress,
                   USBD_PIPE_HANDLE PipeHandle,
                   UCHAR PidDirection,
                   PMDL pMdl,
                   BOOLEAN FreeMdl)
{
    PQUEUE_HEAD QueueHead;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor[3];
    ULONG MdlByteCount = MmGetMdlByteCount(pMdl);
    ULONG NeededDescriptorCount = 1;
    int i;

    QueueHead = CreateQueueHead(hcd);
    QueueHead->EndPointCharacteristics.DeviceAddress = DeviceAddress;

    if (PipeHandle)
    {
        QueueHead->EndPointCharacteristics.EndPointNumber = ((PUSB_ENDPOINT_DESCRIPTOR)PipeHandle)->bEndpointAddress & 0x0F;
        QueueHead->EndPointCharacteristics.MaximumPacketLength = ((PUSB_ENDPOINT_DESCRIPTOR)PipeHandle)->wMaxPacketSize;
    }
    
    QueueHead->FreeMdl = FreeMdl;
    QueueHead->Mdl = pMdl;

    /* FIXME: This is totally screwed */
    /* Calculate how many descriptors would be needed to transfer this buffer */
    if (pMdl)
    {
        if (MdlByteCount < (PAGE_SIZE * 5))
            NeededDescriptorCount = 1;
        else if (MdlByteCount < (PAGE_SIZE * 10))
            NeededDescriptorCount = 2;
        else if (MdlByteCount < (PAGE_SIZE * 15))
            NeededDescriptorCount = 3;
        else
            ASSERT(FALSE);
    }
    /* Limiter Transfers to PAGE_SIZE * 5 * 3, Three Descriptors */

    QueueHead->NumberOfTransferDescriptors = NeededDescriptorCount;
    for (i=0; i< NeededDescriptorCount;i++)
    {
        Descriptor[i] = CreateDescriptor(hcd,
                                         PidDirection,
                                         0);
        Descriptor[i]->AlternateNextPointer = QueueHead->DeadDescriptor->PhysicalAddr;
        
        if (i > 0)
        {
            Descriptor[i-1]->NextDescriptor = Descriptor[i];
            Descriptor[i]->PreviousDescriptor = Descriptor[i-1];
            Descriptor[i-1]->NextPointer = Descriptor[i]->PhysicalAddr;
        }
    }

    Descriptor[0]->Token.Bits.InterruptOnComplete = TRUE;

    /* Save the first descriptor in the QueueHead */
    QueueHead->FirstTransferDescriptor = Descriptor[0];
    QueueHead->NextPointer = Descriptor[0]->PhysicalAddr;

    return QueueHead;
}


/* Builds a QueueHead with 2 to 3 descriptors needed for control transfer
   2 descriptors used for and control request that doesnt return data, such as SetAddress */
PQUEUE_HEAD
BuildControlTransfer(PEHCI_HOST_CONTROLLER hcd,
                     ULONG DeviceAddress,
                     USBD_PIPE_HANDLE PipeHandle,
                     PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup,
                     PMDL pMdl,
                     BOOLEAN FreeMdl)
{
    PQUEUE_HEAD QueueHead;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor[3];
    PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetupVA, CtrlPhysicalPA;

    CtrlSetupVA = (PUSB_DEFAULT_PIPE_SETUP_PACKET)AllocateMemory(hcd,
                                                               sizeof(USB_DEFAULT_PIPE_SETUP_PACKET),
                                                               (ULONG*)&CtrlPhysicalPA);

    RtlCopyMemory(CtrlSetupVA, CtrlSetup, sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    QueueHead = CreateQueueHead(hcd);
    QueueHead->EndPointCharacteristics.DeviceAddress = DeviceAddress;
    if (PipeHandle)
        QueueHead->EndPointCharacteristics.EndPointNumber = ((PUSB_ENDPOINT_DESCRIPTOR)PipeHandle)->bEndpointAddress & 0x0F;

    QueueHead->Token.Bits.DataToggle = TRUE;
    QueueHead->FreeMdl = FreeMdl;
    QueueHead->Mdl = pMdl;

    Descriptor[0] = CreateDescriptor(hcd,
                                     PID_CODE_SETUP_TOKEN,
                                     sizeof(USB_DEFAULT_PIPE_SETUP_PACKET));

    Descriptor[0]->Token.Bits.DataToggle = FALSE;
    /* Save the first descriptor */
    QueueHead->FirstTransferDescriptor = Descriptor[0];

    if (pMdl)
    {
        Descriptor[1] = CreateDescriptor(hcd,
                                         PID_CODE_IN_TOKEN,
                                         MmGetMdlByteCount(pMdl));

        Descriptor[2] = CreateDescriptor(hcd,
                                         PID_CODE_OUT_TOKEN,
                                         0);
    }
    else
    {
        Descriptor[2] = CreateDescriptor(hcd,
                                         PID_CODE_IN_TOKEN,
                                         0);
    }

    Descriptor[2]->Token.Bits.InterruptOnComplete = TRUE;

    /* Link the descriptors */
    if (pMdl)
    {
        Descriptor[0]->NextDescriptor = Descriptor[1];
        Descriptor[1]->NextDescriptor = Descriptor[2];
        Descriptor[1]->PreviousDescriptor = Descriptor[0];
        Descriptor[2]->PreviousDescriptor = Descriptor[1];
    }
    else
    {
        Descriptor[0]->NextDescriptor = Descriptor[2];
        Descriptor[2]->PreviousDescriptor = Descriptor[0];
    }
    
    /* Assign the descriptor buffers */
    Descriptor[0]->BufferPointer[0] = (ULONG)CtrlPhysicalPA;
    Descriptor[0]->BufferPointerVA[0] = (ULONG)CtrlSetupVA;
    
    if (pMdl)
    {
        Descriptor[0]->NextPointer = Descriptor[1]->PhysicalAddr;
        Descriptor[0]->AlternateNextPointer = Descriptor[2]->PhysicalAddr;
        Descriptor[1]->NextPointer = Descriptor[2]->PhysicalAddr;
        Descriptor[1]->AlternateNextPointer = Descriptor[2]->PhysicalAddr;
    }
    else
    {
        Descriptor[0]->NextPointer = Descriptor[2]->PhysicalAddr;
        Descriptor[0]->AlternateNextPointer = Descriptor[2]->PhysicalAddr;
    }
    
    QueueHead->NextPointer = Descriptor[0]->PhysicalAddr;
    if (pMdl)
        QueueHead->NumberOfTransferDescriptors = 3;
    else
        QueueHead->NumberOfTransferDescriptors = 2;
    return QueueHead;
}


IO_ALLOCATION_ACTION NTAPI MapRegisterCallBack(PDEVICE_OBJECT DeviceObject,
                                               PIRP Irp,
                                               PVOID MapRegisterBase,
                                               PVOID Context)
{
    PMAPREGISTERCALLBACKINFO CallBackInfo = (PMAPREGISTERCALLBACKINFO)Context;
    
    CallBackInfo->MapRegisterBase = MapRegisterBase;
    
    KeSetEvent(&CallBackInfo->Event, IO_NO_INCREMENT, FALSE);
    return KeepObject;
}


NTSTATUS
ExecuteTransfer(PDEVICE_OBJECT DeviceObject,
                PUSB_DEVICE UsbDevice,
                USBD_PIPE_HANDLE PipeHandle,
                PUSB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup,
                ULONG TransferFlags,
                PVOID TransferBufferOrMdl,
                ULONG TransferBufferLength,
                PIRP IrpToComplete)
{
    PUSB_ENDPOINT_DESCRIPTOR EndPointDesc = NULL;
    PEHCI_HOST_CONTROLLER hcd;
    PQUEUE_HEAD QueueHead;
    PKEVENT CompleteEvent = NULL;
    PMAPREGISTERCALLBACKINFO CallBackInfo;
    LARGE_INTEGER TimeOut;
    PMDL pMdl = NULL;
    BOOLEAN FreeMdl = FALSE;
    PVOID VirtualAddressOfMdl;
    ULONG NumberOfMapRegisters;
    KIRQL OldIrql;
    NTSTATUS Status = STATUS_SUCCESS;
    UCHAR EndPointType,  PidDirection;
    BOOLEAN IsReadOp = TRUE;
    ULONG TransferBtyesOffset, CurrentTransferBytes;
    PQUEUE_TRANSFER_DESCRIPTOR Descriptor;
    PHYSICAL_ADDRESS PhysicalAddr;
    int i,j;

    hcd = &((PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->hcd;

    /* If no Irp then we will need to wait on completion */
    if (IrpToComplete == NULL)
    {
        CompleteEvent = ExAllocatePool(NonPagedPool, sizeof(KEVENT));
        KeInitializeEvent(CompleteEvent, NotificationEvent, FALSE);
    }

    CallBackInfo = ExAllocatePool(NonPagedPool, sizeof(MAPREGISTERCALLBACKINFO));
    CallBackInfo->MapRegisterBase = 0;

    KeInitializeEvent(&CallBackInfo->Event, NotificationEvent, FALSE);

    /* Determine EndPoint Type */
    if (!PipeHandle)
    {
        EndPointType = USB_ENDPOINT_TYPE_CONTROL;
    }
    else
    {
        EndPointDesc = (PUSB_ENDPOINT_DESCRIPTOR)PipeHandle;
        EndPointType = EndPointDesc->bmAttributes & 0x0F;
    }

    if (TransferBufferOrMdl)
    {
        /* Create MDL for Buffer */
        if (TransferBufferLength)
        {
            pMdl = IoAllocateMdl(TransferBufferOrMdl,
                                 TransferBufferLength,
                                 FALSE,
                                 FALSE,
                                 NULL);
            /* UEHCI created the MDL so it needs to free it */
            FreeMdl = TRUE;
        }
        else
        {
            pMdl = TransferBufferOrMdl;
        }
        MmBuildMdlForNonPagedPool(pMdl);
    }

    switch (EndPointType)
    {
        case USB_ENDPOINT_TYPE_CONTROL:
        {
            QueueHead = BuildControlTransfer(hcd,
                                             UsbDevice->Address,
                                             PipeHandle,
                                             CtrlSetup,
                                             pMdl,
                                             FreeMdl);
            IsReadOp = TRUE;
            break;
        }
        case USB_ENDPOINT_TYPE_BULK:
        {
            PidDirection = EndPointDesc->bEndpointAddress >> 7;
            if (PidDirection)
                IsReadOp = FALSE;

            QueueHead = BuildBulkTransfer(hcd,
                                          UsbDevice->Address,
                                          PipeHandle,
                                          PidDirection,
                                          pMdl,
                                          FreeMdl);
            
            break;
        }
        case USB_ENDPOINT_TYPE_INTERRUPT:
        {
            DPRINT1("Interrupt Endpoints not implemented yet!\n");
            break;
        }
        case USB_ENDPOINT_TYPE_ISOCHRONOUS:
        {
            DPRINT1("Isochronous Endpoints not implemented yet!\n");
            break;
        }
        default:
        {
            DPRINT1("Unknown Endpoint type!!\n");
            break;
        }
    }

    QueueHead->IrpToComplete = IrpToComplete;
    QueueHead->Event = CompleteEvent;

    if (!pMdl)
    {
        ASSERT(QueueHead->NumberOfTransferDescriptors == 2);
        LinkQueueHead(hcd, QueueHead);
        if (IrpToComplete == NULL)
        {
            DPRINT1("Waiting For Completion %x!\n", CompleteEvent);
            TimeOut.QuadPart =  -10000000;
            KeWaitForSingleObject(CompleteEvent, Suspended, KernelMode, FALSE, NULL);//&TimeOut);
            DPRINT1("Request Completed\n");
            ExFreePool(CompleteEvent);
        }
        return Status;
    }
    //ASSERT(FALSE);

    KeFlushIoBuffers(pMdl, IsReadOp, TRUE);
    NumberOfMapRegisters = 15;

    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    hcd->pDmaAdapter->DmaOperations->AllocateAdapterChannel(hcd->pDmaAdapter,
                                                            ((PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension)->ControllerFdo,
                                                            NumberOfMapRegisters,
                                                            MapRegisterCallBack,
                                                            (PVOID)CallBackInfo);
    KeLowerIrql(OldIrql);
    DPRINT1("Waiting for AdapterChannel\n");
    KeWaitForSingleObject(&CallBackInfo->Event, Suspended, KernelMode, FALSE, NULL);
    DPRINT1("Dma controller ready!\n");
    DPRINT1("Getting address for MDL %x\n", pMdl);
    
    VirtualAddressOfMdl = MmGetMdlVirtualAddress(pMdl);

    TransferBtyesOffset = 0;
    while (TransferBtyesOffset < MmGetMdlByteCount(pMdl))
    {
        CurrentTransferBytes = MmGetMdlByteCount(pMdl);
        Descriptor = QueueHead->FirstTransferDescriptor;
        if ((Descriptor->Token.Bits.PIDCode == PID_CODE_SETUP_TOKEN) && (QueueHead->NumberOfTransferDescriptors == 3))
        {
            DPRINT1("QueueHead is for endpoint 0\n");
            DPRINT1("MapRegisterBase %x\n", CallBackInfo->MapRegisterBase);
            DPRINT1("VirtualAddressOfMdl %x, TransferBytesOffset %x\n", VirtualAddressOfMdl, TransferBtyesOffset);
            Descriptor = Descriptor->NextDescriptor;
            PhysicalAddr = hcd->pDmaAdapter->DmaOperations->MapTransfer(hcd->pDmaAdapter,
                                                                        pMdl,
                                                                        CallBackInfo->MapRegisterBase,
                                                                        (PVOID)((ULONG_PTR)VirtualAddressOfMdl + TransferBtyesOffset),
                                                                        &CurrentTransferBytes,
                                                                        !IsReadOp);
            DPRINT1("BufferPointer[0] = %x\n", PhysicalAddr.LowPart);
            Descriptor->BufferPointer[0] = PhysicalAddr.LowPart;
            DPRINT1("CurrentTransferBytes %x\n", CurrentTransferBytes);
            TransferBtyesOffset += CurrentTransferBytes;
            LinkQueueHead(hcd, QueueHead);
            break;
        }
        DPRINT1("PID_CODE_SETUP_TOKEN %x, PidDirection %x, NumDesc %x\n", PID_CODE_SETUP_TOKEN, PidDirection, QueueHead->NumberOfTransferDescriptors);
        for (i=0; i<QueueHead->NumberOfTransferDescriptors; i++)
        {
            if (Descriptor->Token.Bits.PIDCode != PID_CODE_SETUP_TOKEN)
            {
                for (j=0; j<5; j++)
                {
                    PhysicalAddr = hcd->pDmaAdapter->DmaOperations->MapTransfer(hcd->pDmaAdapter,
                                                                                pMdl,
                                                                                CallBackInfo->MapRegisterBase,
                                                                                (PVOID)((ULONG_PTR)VirtualAddressOfMdl + TransferBtyesOffset),
                                                                                &CurrentTransferBytes,
                                                                                !IsReadOp);
                    DPRINT1("BufferPointer[%d] = %x\n", j, PhysicalAddr.LowPart);
                    Descriptor->BufferPointer[j] = PhysicalAddr.LowPart;
                    TransferBtyesOffset += CurrentTransferBytes;
                    if (TransferBtyesOffset >= MmGetMdlByteCount(pMdl))
                        break;
                }
            }
            Descriptor = Descriptor->NextDescriptor;
        }
        LinkQueueHead(hcd, QueueHead);
        break;
    }

    if (TransferBtyesOffset < MmGetMdlByteCount(pMdl)) ASSERT(FALSE);
    if (IrpToComplete == NULL)
    {
        DPRINT1("Waiting For Completion %x!\n", CompleteEvent);
        TimeOut.QuadPart =  -10000000;
        KeWaitForSingleObject(CompleteEvent, Suspended, KernelMode, FALSE, NULL);//&TimeOut);
        DPRINT1("Request Completed\n");
        ExFreePool(CompleteEvent);
    }
    
    KeRaiseIrql(DISPATCH_LEVEL, &OldIrql);
    hcd->pDmaAdapter->DmaOperations->FreeMapRegisters(hcd->pDmaAdapter,
                                                      CallBackInfo->MapRegisterBase,
                                                      NumberOfMapRegisters);

    hcd->pDmaAdapter->DmaOperations->FreeAdapterChannel (hcd->pDmaAdapter);
    KeLowerIrql(OldIrql);
    return Status;
}
