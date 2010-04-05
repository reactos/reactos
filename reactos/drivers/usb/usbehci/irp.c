/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/irp.c
 * PURPOSE:     IRP Handling.
 * PROGRAMMERS:
 *              Michael Martin
 */

#include "usbehci.h"

VOID
RequestURBCancel (PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
    PPDO_DEVICE_EXTENSION PdoDeviceExtension;

    PdoDeviceExtension = (PPDO_DEVICE_EXTENSION) DeviceObject->DeviceExtension;

    KIRQL OldIrql = Irp->CancelIrql;
    IoReleaseCancelSpinLock(DISPATCH_LEVEL);

    KeAcquireSpinLockAtDpcLevel(&PdoDeviceExtension->IrpQueueLock);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);

    KeReleaseSpinLock(&PdoDeviceExtension->IrpQueueLock, OldIrql);

    Irp->IoStatus.Status = STATUS_CANCELLED;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

VOID
QueueURBRequest(PPDO_DEVICE_EXTENSION DeviceExtension, PIRP Irp)
{
    KIRQL OldIrql;

    KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &OldIrql);

    if (Irp->Cancel && IoSetCancelRoutine(Irp, NULL))
    {
        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, OldIrql);
        Irp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }
    else
    {
        InsertTailList(&DeviceExtension->IrpQueue, &Irp->Tail.Overlay.ListEntry);
        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, OldIrql);
    }
}

VOID
CompletePendingURBRequest(PPDO_DEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY NextIrp = NULL;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR Information = 0;
    PIO_STACK_LOCATION Stack;
    PUSB_DEVICE UsbDevice = NULL;
    KIRQL oldIrql;
    PIRP Irp = NULL;
    URB *Urb;

    KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &oldIrql);

    while(!IsListEmpty(&DeviceExtension->IrpQueue))
    {
        NextIrp = RemoveHeadList(&DeviceExtension->IrpQueue);
        Irp = CONTAINING_RECORD(NextIrp, IRP, Tail.Overlay.ListEntry);

        if (!Irp)
            break;

        Stack = IoGetCurrentIrpStackLocation(Irp);
        ASSERT(Stack);

        Urb = (PURB) Stack->Parameters.Others.Argument1;
        ASSERT(Urb);

        Information = 0;
        Status = STATUS_SUCCESS;

        DPRINT1("TransferBuffer %x\n", Urb->UrbControlDescriptorRequest.TransferBuffer);
        DPRINT1("TransferBufferLength %x\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);
        DPRINT1("UsbdDeviceHandle = %x\n", Urb->UrbHeader.UsbdDeviceHandle);

        UsbDevice = Urb->UrbHeader.UsbdDeviceHandle;
        /* UsbdDeviceHandle of 0 is root hub */
        if (UsbDevice == NULL)
            UsbDevice = DeviceExtension->UsbDevices[0];

        /* Assume URB success */
        Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
        /* Set the DeviceHandle to the Internal Device */
        Urb->UrbHeader.UsbdDeviceHandle = UsbDevice;

        switch (Urb->UrbHeader.Function)
        {
            case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
            {
                DPRINT1("URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:\n");
                DPRINT1("--->TransferBufferLength %x\n",Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
                DPRINT1("--->TransferBuffer %x\n",Urb->UrbBulkOrInterruptTransfer.TransferBuffer);
                DPRINT1("--->PipeHandle %x\n",Urb->UrbBulkOrInterruptTransfer.PipeHandle);
                DPRINT1("---->(PVOID)&UsbDevice->EndPointDescriptor %x\n", (PVOID)&UsbDevice->ActiveInterface->EndPoints[0]->EndPointDescriptor);
                DPRINT1("--->TransferFlags %x\n", Urb->UrbBulkOrInterruptTransfer.TransferFlags);

                RtlZeroMemory(Urb->UrbBulkOrInterruptTransfer.TransferBuffer, Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);
                if (UsbDevice == DeviceExtension->UsbDevices[0])
                {
                    if (Urb->UrbBulkOrInterruptTransfer.TransferFlags & (USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK))
                    {
                        LONG i;
                        for (i = 0; i < 8; i++)
                        {
                            if (DeviceExtension->Ports[i].PortChange)
                            {
                                DPRINT1("Inform hub driver that port %d has changed\n", i+1);
                                ((PUCHAR)Urb->UrbBulkOrInterruptTransfer.TransferBuffer)[0] = 1 << (i + 1);
                            }
                        }
                    }
                    else
                    {
                        Urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
                        Status = STATUS_UNSUCCESSFUL;
                    }
                }
                break;
            }
            case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
            {
                DPRINT("Get Status from Device\n");
                DPRINT("Index : %d\n", Urb->UrbControlGetStatusRequest.Index);

                /* Copied from pvdrivers */
                if (Urb->UrbControlGetStatusRequest.Index == 0)
                {
                    *(PUSHORT)Urb->UrbControlGetStatusRequest.TransferBuffer = USB_PORT_STATUS_CONNECT | USB_PORT_STATUS_ENABLE;
                }
                else
                {
                    DPRINT1("Uknown identifier\n");
                    Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
                    Status = STATUS_UNSUCCESSFUL;
                    ASSERT(FALSE);
                }
                break;
            }
            case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
            {
                switch(Urb->UrbControlDescriptorRequest.DescriptorType)
                {
                    case USB_DEVICE_DESCRIPTOR_TYPE:
                    {
                        DPRINT1("USB DEVICE DESC\n");
                        if (Urb->UrbControlDescriptorRequest.TransferBufferLength >= sizeof(USB_DEVICE_DESCRIPTOR))
                        {
                            Urb->UrbControlDescriptorRequest.TransferBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
                        }

                        RtlCopyMemory(Urb->UrbControlDescriptorRequest.TransferBuffer,
                                      &UsbDevice->DeviceDescriptor,
                                      Urb->UrbControlDescriptorRequest.TransferBufferLength);
                        break;
                    }
                    case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                    {
                        PUCHAR BufPtr;
                        LONG i, j;

                        DPRINT1("USB CONFIG DESC\n");

                        if (Urb->UrbControlDescriptorRequest.TransferBufferLength >= UsbDevice->ActiveConfig->ConfigurationDescriptor.wTotalLength)
                        {
                            Urb->UrbControlDescriptorRequest.TransferBufferLength = UsbDevice->ActiveConfig->ConfigurationDescriptor.wTotalLength;
                        }
                        else
                        {
                            DPRINT1("Buffer to small!!!\n");
                            ASSERT(FALSE);
                        }

                        BufPtr = (PUCHAR)Urb->UrbControlDescriptorRequest.TransferBuffer;

                        /* Copy the Configuration Descriptor */
                        RtlCopyMemory(BufPtr, &UsbDevice->ActiveConfig->ConfigurationDescriptor, sizeof(USB_CONFIGURATION_DESCRIPTOR));
                        BufPtr += sizeof(USB_CONFIGURATION_DESCRIPTOR);
                        for (i = 0; i < UsbDevice->ActiveConfig->ConfigurationDescriptor.bNumInterfaces; i++)
                        {
                            /* Copy the Interface Descriptor */
                            RtlCopyMemory(BufPtr, &UsbDevice->ActiveConfig->Interfaces[i]->InterfaceDescriptor, sizeof(USB_INTERFACE_DESCRIPTOR));
                            BufPtr += sizeof(USB_INTERFACE_DESCRIPTOR);
                            for (j = 0; j < UsbDevice->ActiveConfig->Interfaces[i]->InterfaceDescriptor.bNumEndpoints; j++)
                            {
                                /* Copy the EndPoint Descriptor */
                                RtlCopyMemory(BufPtr, &UsbDevice->ActiveConfig->Interfaces[i]->EndPoints[j]->EndPointDescriptor, sizeof(USB_ENDPOINT_DESCRIPTOR));
                                BufPtr += sizeof(USB_ENDPOINT_DESCRIPTOR);
                            }
                        }

                        break;
                    }
                    case USB_STRING_DESCRIPTOR_TYPE:
                    {
                        DPRINT1("Usb String Descriptor not implemented\n");
                        break;
                    }
                    default:
                    {
                        DPRINT1("Descriptor Type %x not supported!\n", Urb->UrbControlDescriptorRequest.DescriptorType);
                    }
                }
                break;
            }
            case URB_FUNCTION_SELECT_CONFIGURATION:
            {
                PUSBD_INTERFACE_INFORMATION InterfaceInfo;
                LONG iCount, pCount;

                DPRINT("Selecting Configuration\n");
                DPRINT("Length %x\n", Urb->UrbHeader.Length);
                DPRINT("Urb->UrbSelectConfiguration.ConfigurationHandle %x\n",Urb->UrbSelectConfiguration.ConfigurationHandle);

                if (Urb->UrbSelectConfiguration.ConfigurationDescriptor)
                {
                    DPRINT("ConfigurationDescriptor = %p\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor);
                    DPRINT(" bLength = %d\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->bLength);
                    DPRINT(" bDescriptorType = %d\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->bDescriptorType);
                    DPRINT(" wTotalLength = %d\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->wTotalLength);
                    DPRINT(" bNumInterfaces = %d\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces);
                    DPRINT(" bConfigurationValue = %d\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->bConfigurationValue);
                    DPRINT(" iConfiguration = %d\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->iConfiguration);
                    DPRINT(" bmAttributes = %04x\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->bmAttributes);
                    DPRINT(" MaxPower = %d\n", Urb->UrbSelectConfiguration.ConfigurationDescriptor->MaxPower);


                    Urb->UrbSelectConfiguration.ConfigurationHandle = (PVOID)&DeviceExtension->UsbDevices[0]->ActiveConfig->ConfigurationDescriptor;
                    DPRINT("ConfigHandle %x\n", Urb->UrbSelectConfiguration.ConfigurationHandle);
                    InterfaceInfo = &Urb->UrbSelectConfiguration.Interface;

                    for (iCount = 0; iCount < Urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces; iCount++)
                    {
                        DPRINT("InterfaceInformation[%d]\n", iCount);
                        DPRINT(" Length = %d\n", InterfaceInfo->Length);
                        DPRINT(" InterfaceNumber = %d\n", InterfaceInfo->InterfaceNumber);
                        DPRINT(" AlternateSetting = %d\n", InterfaceInfo->AlternateSetting);
                        DPRINT(" Class = %02x\n", (ULONG)InterfaceInfo->Class);
                        DPRINT(" SubClass = %02x\n", (ULONG)InterfaceInfo->SubClass);
                        DPRINT(" Protocol = %02x\n", (ULONG)InterfaceInfo->Protocol);
                        DPRINT(" Reserved = %02x\n", (ULONG)InterfaceInfo->Reserved);
                        DPRINT(" InterfaceHandle = %p\n", InterfaceInfo->InterfaceHandle);
                        DPRINT(" NumberOfPipes = %d\n", InterfaceInfo->NumberOfPipes);
                        InterfaceInfo->InterfaceHandle = (PVOID)&UsbDevice->ActiveInterface->InterfaceDescriptor;
                        InterfaceInfo->Class = UsbDevice->ActiveInterface->InterfaceDescriptor.bInterfaceClass;
                        InterfaceInfo->SubClass = UsbDevice->ActiveInterface->InterfaceDescriptor.bInterfaceSubClass;
                        InterfaceInfo->Protocol = UsbDevice->ActiveInterface->InterfaceDescriptor.bInterfaceProtocol;
                        InterfaceInfo->Reserved = 0;

                        for (pCount = 0; pCount < InterfaceInfo->NumberOfPipes; pCount++)
                        {
                          DPRINT("Pipe[%d]\n", pCount);
                          DPRINT(" MaximumPacketSize = %d\n", InterfaceInfo->Pipes[pCount].MaximumPacketSize);
                          DPRINT(" EndpointAddress = %d\n", InterfaceInfo->Pipes[pCount].EndpointAddress);
                          DPRINT(" Interval = %d\n", InterfaceInfo->Pipes[pCount].Interval);
                          DPRINT(" PipeType = %d\n", InterfaceInfo->Pipes[pCount].PipeType);
                          DPRINT(" PipeHandle = %x\n", InterfaceInfo->Pipes[pCount].PipeHandle);
                          DPRINT(" MaximumTransferSize = %d\n", InterfaceInfo->Pipes[pCount].MaximumTransferSize);
                          DPRINT(" PipeFlags = %08x\n", InterfaceInfo->Pipes[pCount].PipeFlags);
                          InterfaceInfo->Pipes[pCount].MaximumPacketSize = UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor.wMaxPacketSize;
                          InterfaceInfo->Pipes[pCount].EndpointAddress = UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor.bEndpointAddress;
                          InterfaceInfo->Pipes[pCount].Interval = UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor.bInterval;
                          InterfaceInfo->Pipes[pCount].PipeType = UsbdPipeTypeInterrupt;
                          InterfaceInfo->Pipes[pCount].PipeHandle = (PVOID)&UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor;
                          if (InterfaceInfo->Pipes[pCount].MaximumTransferSize == 0)
                              InterfaceInfo->Pipes[pCount].MaximumTransferSize = 4096;
                          /* InterfaceInfo->Pipes[j].PipeFlags = 0; */
                        }
                        InterfaceInfo = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)InterfaceInfo + InterfaceInfo->Length);
                    }
                }
                else
                {
                    /* FIXME: Set device to unconfigured state */
                }
                break;
            }
            case URB_FUNCTION_CLASS_DEVICE:
            {
                switch (Urb->UrbControlVendorClassRequest.Request)
                {
                    case USB_REQUEST_GET_DESCRIPTOR:
                    {
                        DPRINT1("TransferFlags %x\n", Urb->UrbControlVendorClassRequest.TransferFlags);
                        DPRINT1("Urb->UrbControlVendorClassRequest.Value %x\n", Urb->UrbControlVendorClassRequest.Value);

                        switch (Urb->UrbControlVendorClassRequest.Value >> 8)
                        {
                            case USB_DEVICE_CLASS_AUDIO:
                            {
                                DPRINT1("USB_DEVICE_CLASS_AUDIO\n");
                                break;
                            }
                            case USB_DEVICE_CLASS_COMMUNICATIONS:
                            {
                                DPRINT1("USB_DEVICE_CLASS_COMMUNICATIONS\n");
                                break;
                            }
                            case USB_DEVICE_CLASS_HUMAN_INTERFACE:
                            {
                                DPRINT1("USB_DEVICE_CLASS_HUMAN_INTERFACE\n");
                                break;
                            }
                            case USB_DEVICE_CLASS_MONITOR:
                            {
                                DPRINT1("USB_DEVICE_CLASS_MONITOR\n");
                                break;
                            }
                            case USB_DEVICE_CLASS_PHYSICAL_INTERFACE:
                            {
                                DPRINT1("USB_DEVICE_CLASS_PHYSICAL_INTERFACE\n");
                                break;
                            }
                            case USB_DEVICE_CLASS_POWER:
                            {
                                DPRINT1("USB_DEVICE_CLASS_POWER\n");
                                break;
                            }
                            case USB_DEVICE_CLASS_PRINTER:
                            {
                                DPRINT1("USB_DEVICE_CLASS_PRINTER\n");
                                break;
                            }
                            case USB_DEVICE_CLASS_STORAGE:
                            {
                               DPRINT1("USB_DEVICE_CLASS_STORAGE\n");
                               break;
                            }
                            case USB_DEVICE_CLASS_RESERVED:
                                DPRINT1("Reserved!!!\n");
                            case USB_DEVICE_CLASS_HUB:
                            {
                                PUSB_HUB_DESCRIPTOR UsbHubDescr = Urb->UrbControlVendorClassRequest.TransferBuffer;
                                /* FIXME: Handle more than root hub? */
                                if(Urb->UrbControlVendorClassRequest.TransferBufferLength >= sizeof(USB_HUB_DESCRIPTOR))
                                {
                                    Urb->UrbControlVendorClassRequest.TransferBufferLength = sizeof(USB_HUB_DESCRIPTOR);
                                }
                                else
                                {
                                    /* FIXME: Handle this correctly */
                                    UsbHubDescr->bDescriptorLength = sizeof(USB_HUB_DESCRIPTOR);
                                    UsbHubDescr->bDescriptorType = 0x29;
                                    break;
                                }
                                DPRINT1("USB_DEVICE_CLASS_HUB request\n");
                                UsbHubDescr->bDescriptorLength = sizeof(USB_HUB_DESCRIPTOR);
                                UsbHubDescr->bDescriptorType = 0x29;
                                UsbHubDescr->bNumberOfPorts = 0x08;
                                UsbHubDescr->wHubCharacteristics = 0x0012;
                                UsbHubDescr->bPowerOnToPowerGood = 0x01;
                                UsbHubDescr->bHubControlCurrent = 0x00;
                                UsbHubDescr->bRemoveAndPowerMask[0] = 0x00;
                                UsbHubDescr->bRemoveAndPowerMask[1] = 0x00;
                                UsbHubDescr->bRemoveAndPowerMask[2] = 0xff;
                                break;
                            }
                            default:
                            {
                                DPRINT1("Unknown UrbControlVendorClassRequest Value\n");
                            }
                        }
                        break;
                    }
                    case USB_REQUEST_GET_STATUS:
                    {
                        DPRINT1("DEVICE: USB_REQUEST_GET_STATUS for port %d\n", Urb->UrbControlVendorClassRequest.Index);

                        if (Urb->UrbControlVendorClassRequest.Index == 1)
                        {
                            ((PULONG)Urb->UrbControlVendorClassRequest.TransferBuffer)[0] = 0;
                        }
                        break;
                    }
                    default:
                    {
                        DPRINT1("Unhandled URB request for class device\n");
                        Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
                    }
                }
                break;
            }
            case URB_FUNCTION_CLASS_OTHER:
            {
                switch (Urb->UrbControlVendorClassRequest.Request)
                {
                    case USB_REQUEST_GET_STATUS:
                    {
                        DPRINT1("OTHER: USB_REQUEST_GET_STATUS for port %d\n", Urb->UrbControlVendorClassRequest.Index);

                        ((PUSHORT)Urb->UrbControlVendorClassRequest.TransferBuffer)[0] = DeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortStatus;
                        ((PUSHORT)Urb->UrbControlVendorClassRequest.TransferBuffer)[1] = DeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange;
                        break;
                    }
                    case USB_REQUEST_CLEAR_FEATURE:
                    {
                        DPRINT1("USB_REQUEST_CLEAR_FEATURE Port %d, value %x\n", Urb->UrbControlVendorClassRequest.Index,
                            Urb->UrbControlVendorClassRequest.Value);
                        switch (Urb->UrbControlVendorClassRequest.Value)
                        {
                            case C_PORT_CONNECTION:
                                DeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange &= ~USB_PORT_STATUS_CONNECT;
                                break;
                            case C_PORT_RESET:
                                DeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange &= ~USB_PORT_STATUS_RESET;
                                break;
                            default:
                                DPRINT1("Unknown Value for Clear Feature %x \n", Urb->UrbControlVendorClassRequest.Value);
                                break;
                        }
                        break;
                    }
                    case USB_REQUEST_SET_FEATURE:
                    {
                        DPRINT1("USB_REQUEST_SET_FEATURE Port %d, value %x\n", Urb->UrbControlVendorClassRequest.Index,
                            Urb->UrbControlVendorClassRequest.Value);

                        switch(Urb->UrbControlVendorClassRequest.Value)
                        {
                            case PORT_RESET:
                            {
                                DeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange |= USB_PORT_STATUS_RESET;
                                break;
                            }
                            case PORT_ENABLE:
                            {
                                DPRINT1("Unhandled Set Feature\n");
                                break;
                            }
                            default:
                            {
                                DPRINT1("Unknown Set Feature!\n");
                                break;
                            }
                        }
                        break;
                    }
                    case USB_REQUEST_SET_ADDRESS:
                    {
                        DPRINT1("USB_REQUEST_SET_ADDRESS\n");
                        break;
                    }
                    case USB_REQUEST_GET_DESCRIPTOR:
                    {
                        DPRINT1("USB_REQUEST_GET_DESCRIPTOR\n");
                        break;
                    }
                    case USB_REQUEST_SET_DESCRIPTOR:
                    {
                        DPRINT1("USB_REQUEST_SET_DESCRIPTOR\n");
                        break;
                    }
                    case USB_REQUEST_GET_CONFIGURATION:
                    {
                        DPRINT1("USB_REQUEST_GET_CONFIGURATION\n");
                        break;
                    }
                    case USB_REQUEST_SET_CONFIGURATION:
                    {
                        DPRINT1("USB_REQUEST_SET_CONFIGURATION\n");
                        break;
                    }
                    case USB_REQUEST_GET_INTERFACE:
                    {
                        DPRINT1("USB_REQUEST_GET_INTERFACE\n");
                        break;
                    }
                    case USB_REQUEST_SET_INTERFACE:
                    {
                        DPRINT1("USB_REQUEST_SET_INTERFACE\n");
                        break;
                    }
                    case USB_REQUEST_SYNC_FRAME:
                    {
                        DPRINT1("USB_REQUEST_SYNC_FRAME\n");
                        break;
                    }
                    default:
                    {
                        DPRINT1("Unknown Function Class Unknown request\n");
                        ASSERT(FALSE);
                        break;
                    }
                }
                break;
            }
            default:
            {
                DPRINT1("Unhandled URB %x\n", Urb->UrbHeader.Function);
                Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
                ASSERT(FALSE);
            }

        }

        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);

        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = Information;

        if (Urb->UrbHeader.Status == USBD_STATUS_SUCCESS)
        {
            /* Fake a successful Control Transfer */
            Urb->UrbHeader.Function = 0x08;
            Urb->UrbHeader.UsbdFlags = 0;
        }

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &oldIrql);
    }

    KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);
}

