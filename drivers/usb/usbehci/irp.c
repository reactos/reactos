/*
 * PROJECT:     ReactOS Universal Serial Bus Bulk Enhanced Host Controller Interface
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbehci/irp.c
 * PURPOSE:     IRP Handling.
 * PROGRAMMERS:
 *              Michael Martin (michael.martin@reactos.org)
 */

#include "usbehci.h"
#include "hwiface.h"
#include "physmem.h"
#include "transfer.h"

VOID NTAPI
WorkerThread(IN PVOID Context)
{
    PWORKITEMDATA WorkItemData = (PWORKITEMDATA)Context;
    
    CompletePendingURBRequest((PPDO_DEVICE_EXTENSION)WorkItemData->Context);
    
    ExFreePool(WorkItemData);
}

VOID
RemoveUrbRequest(PPDO_DEVICE_EXTENSION DeviceExtension, PIRP Irp)
{
    KIRQL OldIrql;
    KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &OldIrql);
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
    KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, OldIrql);
}

VOID
RequestURBCancel (PPDO_DEVICE_EXTENSION PdoDeviceExtension, PIRP Irp)
{
    KIRQL OldIrql = Irp->CancelIrql;
    IoReleaseCancelSpinLock(DISPATCH_LEVEL);
    DPRINT1("IRP CANCELLED\n");
    ASSERT(FALSE);
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

    if ((Irp->Cancel) && (IoSetCancelRoutine(Irp, RequestURBCancel)))
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

NTSTATUS HandleUrbRequest(PPDO_DEVICE_EXTENSION PdoDeviceExtension, PIRP Irp)
{
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    ULONG_PTR Information = 0;
    PIO_STACK_LOCATION Stack;
    PUSB_DEVICE UsbDevice = NULL;
    URB *Urb;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    FdoDeviceExtension = (PFDO_DEVICE_EXTENSION) PdoDeviceExtension->ControllerFdo->DeviceExtension;

    Stack = IoGetCurrentIrpStackLocation(Irp);
    ASSERT(Stack);

    Urb = (PURB) Stack->Parameters.Others.Argument1;
    ASSERT(Urb);

    Information = 0;
    Status = STATUS_SUCCESS;

    DPRINT("TransferBuffer %x\n", Urb->UrbControlDescriptorRequest.TransferBuffer);
    DPRINT("TransferBufferLength %x\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);
    DPRINT("UsbdDeviceHandle = %x\n", Urb->UrbHeader.UsbdDeviceHandle);

    UsbDevice = Urb->UrbHeader.UsbdDeviceHandle;

    /* UsbdDeviceHandle of 0 is root hub */
    if (UsbDevice == NULL)
        UsbDevice = PdoDeviceExtension->UsbDevices[0];

    /* Assume URB success */
    Urb->UrbHeader.Status = USBD_STATUS_SUCCESS;
    /* Set the DeviceHandle to the Internal Device */
    Urb->UrbHeader.UsbdDeviceHandle = UsbDevice;

    switch (Urb->UrbHeader.Function)
    {
        case URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER:
        {
            DPRINT1("URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER UsbDevice %x\n", UsbDevice);
            if (&UsbDevice->ActiveInterface->EndPoints[0]->EndPointDescriptor != Urb->UrbBulkOrInterruptTransfer.PipeHandle)
            {
                DPRINT1("Invalid Parameter, Expected EndPointDescriptor of %x, but got %x\n", &UsbDevice->ActiveInterface->EndPoints[0]->EndPointDescriptor, Urb->UrbBulkOrInterruptTransfer.PipeHandle);
                Status = STATUS_INVALID_PARAMETER;
                ASSERT(FALSE);
                break;
            }

            ASSERT(Urb->UrbBulkOrInterruptTransfer.TransferBuffer != NULL);
            RtlZeroMemory(Urb->UrbBulkOrInterruptTransfer.TransferBuffer, Urb->UrbBulkOrInterruptTransfer.TransferBufferLength);

            if (UsbDevice == PdoDeviceExtension->UsbDevices[0])
            {
                if (Urb->UrbBulkOrInterruptTransfer.TransferFlags & (USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK))
                {
                    LONG i;
                    for (i = 0; i < PdoDeviceExtension->NumberOfPorts; i++)
                    {
                        if (PdoDeviceExtension->Ports[i].PortChange)
                        {
                            DPRINT1("Inform hub driver that port %d has changed\n", i+1);
                            ((PUCHAR)Urb->UrbBulkOrInterruptTransfer.TransferBuffer)[0] = 1 << ((i + 1) & 7);
                        }
                    }
                }
                else
                {
                    Urb->UrbHeader.Status = USBD_STATUS_INVALID_PARAMETER;
                    Status = STATUS_UNSUCCESSFUL;
                    DPRINT1("Invalid transfer flags for SCE\n");
                }
            }
            else
                DPRINT1("Interrupt Transfer not for hub\n");
            break;
        }
        case URB_FUNCTION_GET_STATUS_FROM_DEVICE:
        {
            DPRINT1("URB_FUNCTION_GET_STATUS_FROM_DEVICE\n");
            /* If for the hub device */
            if ((Urb->UrbControlGetStatusRequest.Index == 0) && (UsbDevice == PdoDeviceExtension->UsbDevices[0]))
            {
                ASSERT(Urb->UrbBulkOrInterruptTransfer.TransferBuffer != NULL);
                *(PUSHORT)Urb->UrbControlGetStatusRequest.TransferBuffer = USB_PORT_STATUS_CONNECT /*| USB_PORT_STATUS_ENABLE*/;
            }
            else
            {
                DPRINT1("UsbDeviceHandle %x, Index %x not implemented yet\n", UsbDevice, Urb->UrbControlGetStatusRequest.Index);
                Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
                Status = STATUS_UNSUCCESSFUL;
            }
            break;
        }
        case URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE:
        {
            USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
            
            DPRINT1("URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE\n");
            switch(Urb->UrbControlDescriptorRequest.DescriptorType)
            {
                case USB_DEVICE_DESCRIPTOR_TYPE:
                {
                    PUCHAR BufPtr;
                    DPRINT1("Device Descr Type\n");

                    if (Urb->UrbControlDescriptorRequest.TransferBufferLength >= sizeof(USB_DEVICE_DESCRIPTOR))
                    {
                        Urb->UrbControlDescriptorRequest.TransferBufferLength = sizeof(USB_DEVICE_DESCRIPTOR);
                    }
                    if (UsbDevice == PdoDeviceExtension->UsbDevices[0])
                    {
                        DPRINT1("ROOTHUB!\n");
                        BufPtr = (PUCHAR)Urb->UrbControlDescriptorRequest.TransferBuffer;

                        DPRINT1("Length %x\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);

                        /* Copy the Device Descriptor */
                        RtlCopyMemory(BufPtr, &UsbDevice->DeviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR));
                        DumpDeviceDescriptor((PUSB_DEVICE_DESCRIPTOR)Urb->UrbControlDescriptorRequest.TransferBuffer);
                        break;
                    }

                    ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer != NULL);

                    BuildSetupPacketFromURB(&FdoDeviceExtension->hcd, Urb, &CtrlSetup);
                    IoMarkIrpPending(Irp);
                    Status = STATUS_PENDING;
                    SubmitControlTransfer(&FdoDeviceExtension->hcd,
                                          &CtrlSetup,
                                          Urb->UrbControlDescriptorRequest.TransferBuffer,
                                          Urb->UrbControlDescriptorRequest.TransferBufferLength,
                                          Irp);
                    break;
                }
                case USB_CONFIGURATION_DESCRIPTOR_TYPE:
                {
                    PUCHAR BufPtr;
                    LONG i, j;
                    DPRINT1("Config Descr Type\n");
                    if (UsbDevice == PdoDeviceExtension->UsbDevices[0])
                    {
                        DPRINT1("ROOTHUB!\n");
                    }
                    if (Urb->UrbControlDescriptorRequest.TransferBufferLength >= UsbDevice->ActiveConfig->ConfigurationDescriptor.wTotalLength)
                    {
                        Urb->UrbControlDescriptorRequest.TransferBufferLength = UsbDevice->ActiveConfig->ConfigurationDescriptor.wTotalLength;
                    }
                    else
                    {
                        DPRINT1("TransferBufferLenth %x is too small!!!\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);
                        if (Urb->UrbControlDescriptorRequest.TransferBufferLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
                        {
                            DPRINT1("Configuration Descriptor cannot fit into given buffer!\n");
                            break;
                        }
                    }

                    ASSERT(Urb->UrbControlDescriptorRequest.TransferBuffer);
                    BufPtr = (PUCHAR)Urb->UrbControlDescriptorRequest.TransferBuffer;

                    DPRINT1("Length %x\n", Urb->UrbControlDescriptorRequest.TransferBufferLength);

                    /* Copy the Configuration Descriptor */
                    RtlCopyMemory(BufPtr, &UsbDevice->ActiveConfig->ConfigurationDescriptor, sizeof(USB_CONFIGURATION_DESCRIPTOR));

                    /* If there is no room for all the configs then bail */
                    if (!(Urb->UrbControlDescriptorRequest.TransferBufferLength > sizeof(USB_CONFIGURATION_DESCRIPTOR)))
                    {
                        DPRINT("All Descriptors cannot fit into given buffer! Only USB_CONFIGURATION_DESCRIPTOR given\n");
                        break;
                    }

                    BufPtr += sizeof(USB_CONFIGURATION_DESCRIPTOR);
                    for (i = 0; i < UsbDevice->ActiveConfig->ConfigurationDescriptor.bNumInterfaces; i++)
                    {
                        /* Copy the Interface Descriptor */
                        RtlCopyMemory(BufPtr,
                                      &UsbDevice->ActiveConfig->Interfaces[i]->InterfaceDescriptor,
                                      sizeof(USB_INTERFACE_DESCRIPTOR));
                        BufPtr += sizeof(USB_INTERFACE_DESCRIPTOR);
                        for (j = 0; j < UsbDevice->ActiveConfig->Interfaces[i]->InterfaceDescriptor.bNumEndpoints; j++)
                        {
                            /* Copy the EndPoint Descriptor */
                            RtlCopyMemory(BufPtr,
                                          &UsbDevice->ActiveConfig->Interfaces[i]->EndPoints[j]->EndPointDescriptor,
                                          sizeof(USB_ENDPOINT_DESCRIPTOR));
                            BufPtr += sizeof(USB_ENDPOINT_DESCRIPTOR);
                        }
                    }
                    DumpFullConfigurationDescriptor((PUSB_CONFIGURATION_DESCRIPTOR)Urb->UrbControlDescriptorRequest.TransferBuffer);
                    break;
                }
                case USB_STRING_DESCRIPTOR_TYPE:
                {
                    DPRINT1("StringDescriptorType\n");
                    DPRINT1("Urb->UrbControlDescriptorRequest.Index %x\n", Urb->UrbControlDescriptorRequest.Index);
                    DPRINT1("Urb->UrbControlDescriptorRequest.LanguageId %x\n", Urb->UrbControlDescriptorRequest.LanguageId);

                    if (Urb->UrbControlDescriptorRequest.Index == 0)
                        DPRINT1("Requesting LANGID's\n");

                    BuildSetupPacketFromURB(&FdoDeviceExtension->hcd, Urb, &CtrlSetup);
                    IoMarkIrpPending(Irp);
                    Status = STATUS_PENDING;

                    SubmitControlTransfer(&FdoDeviceExtension->hcd,
                                          &CtrlSetup,
                                          Urb->UrbControlDescriptorRequest.TransferBuffer,
                                          Urb->UrbControlDescriptorRequest.TransferBufferLength,
                                          Irp);
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

            DPRINT1("Selecting Configuration\n");
            DPRINT1("ConfigurationHandle %x\n",Urb->UrbSelectConfiguration.ConfigurationHandle);
            if (Urb->UrbSelectConfiguration.ConfigurationDescriptor)
            {
                Urb->UrbSelectConfiguration.ConfigurationHandle = &UsbDevice->ActiveConfig->ConfigurationDescriptor;
                DPRINT1("ConfigHandle %x\n", Urb->UrbSelectConfiguration.ConfigurationHandle);
                ASSERT(FALSE);
                InterfaceInfo = &Urb->UrbSelectConfiguration.Interface;

                DPRINT1("Length %x\n", InterfaceInfo->Length);
                DPRINT1("NumberOfPipes %x\n", InterfaceInfo->NumberOfPipes);

                for (iCount = 0; iCount < Urb->UrbSelectConfiguration.ConfigurationDescriptor->bNumInterfaces; iCount++)
                {
                    InterfaceInfo->InterfaceHandle = (PVOID)&UsbDevice->ActiveInterface->InterfaceDescriptor;
                    InterfaceInfo->Class = UsbDevice->ActiveInterface->InterfaceDescriptor.bInterfaceClass;
                    InterfaceInfo->SubClass = UsbDevice->ActiveInterface->InterfaceDescriptor.bInterfaceSubClass;
                    InterfaceInfo->Protocol = UsbDevice->ActiveInterface->InterfaceDescriptor.bInterfaceProtocol;
                    InterfaceInfo->Reserved = 0;

                    for (pCount = 0; pCount < InterfaceInfo->NumberOfPipes; pCount++)
                    {
                        InterfaceInfo->Pipes[pCount].MaximumPacketSize = UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor.wMaxPacketSize;
                        InterfaceInfo->Pipes[pCount].EndpointAddress = UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor.bEndpointAddress;
                        InterfaceInfo->Pipes[pCount].Interval = UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor.bInterval;
                        InterfaceInfo->Pipes[pCount].PipeType = UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor.bmAttributes;
                        InterfaceInfo->Pipes[pCount].PipeHandle = (PVOID)&UsbDevice->ActiveInterface->EndPoints[pCount]->EndPointDescriptor;
                        if (InterfaceInfo->Pipes[pCount].MaximumTransferSize == 0)
                            InterfaceInfo->Pipes[pCount].MaximumTransferSize = 4096;
                        /* InterfaceInfo->Pipes[j].PipeFlags = 0; */
                    }
                    InterfaceInfo = (PUSBD_INTERFACE_INFORMATION)((PUCHAR)InterfaceInfo + InterfaceInfo->Length);
                    if (InterfaceInfo->Length == 0) break;
                }
            }
            else
            {
                /* FIXME: Set device to unconfigured state */
                DPRINT1("Setting device to unconfigured state not implemented!\n");
            }
            break;
        }
        case URB_FUNCTION_SELECT_INTERFACE:
        {
            USB_DEFAULT_PIPE_SETUP_PACKET CtrlSetup;
            PUSBD_INTERFACE_INFORMATION InterfaceInfo;
            int i;

            DPRINT1("Select Interface!\n");
            DPRINT1("Config Handle %x\n", Urb->UrbSelectInterface.ConfigurationHandle);

            InterfaceInfo = &Urb->UrbSelectInterface.Interface;
            DPRINT1("InterfaceNumber %x\n", InterfaceInfo->InterfaceNumber);
            DPRINT1("AlternateSetting %x\n", InterfaceInfo->AlternateSetting);
            DPRINT1("NumPipes %x\n", InterfaceInfo->NumberOfPipes);
            for (i=0;i<InterfaceInfo->NumberOfPipes;i++)
            {
                InterfaceInfo->Pipes[i].PipeHandle = (PVOID)&UsbDevice->ActiveInterface->EndPoints[i]->EndPointDescriptor;
            }

            BuildSetupPacketFromURB(&FdoDeviceExtension->hcd, Urb, &CtrlSetup);
            IoMarkIrpPending(Irp);
            Status = STATUS_PENDING;

            SubmitControlTransfer(&FdoDeviceExtension->hcd,
                                  &CtrlSetup,
                                  NULL,
                                  0,
                                  Irp);
            break;
        }
        case URB_FUNCTION_CLASS_DEVICE:
        {
            DPRINT1("URB_FUNCTION_CLASS_DEVICE\n");
            switch (Urb->UrbControlVendorClassRequest.Request)
            {
                case USB_REQUEST_GET_DESCRIPTOR:
                {
                    switch (Urb->UrbControlVendorClassRequest.Value >> 8)
                    {
                        case USB_DEVICE_CLASS_AUDIO:
                        {
                            DPRINT1("USB_DEVICE_CLASS_AUDIO not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_COMMUNICATIONS:
                        {
                            DPRINT1("USB_DEVICE_CLASS_COMMUNICATIONS not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_HUMAN_INTERFACE:
                        {
                            DPRINT1("USB_DEVICE_CLASS_HUMAN_INTERFACE not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_MONITOR:
                        {
                            DPRINT1("USB_DEVICE_CLASS_MONITOR not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_PHYSICAL_INTERFACE:
                        {
                            DPRINT1("USB_DEVICE_CLASS_PHYSICAL_INTERFACE not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_POWER:
                        {
                            DPRINT1("USB_DEVICE_CLASS_POWER not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_PRINTER:
                        {
                            DPRINT1("USB_DEVICE_CLASS_PRINTER not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_STORAGE:
                        {
                            DPRINT1("USB_DEVICE_CLASS_STORAGE not implemented\n");
                            break;
                        }
                        case USB_DEVICE_CLASS_RESERVED:
                            DPRINT1("Reserved!!!\n");
                        case USB_DEVICE_CLASS_HUB:
                        {
                            PUSB_HUB_DESCRIPTOR UsbHubDescr = Urb->UrbControlVendorClassRequest.TransferBuffer;

                            DPRINT1("Length %x\n", Urb->UrbControlVendorClassRequest.TransferBufferLength);
                            ASSERT(Urb->UrbControlVendorClassRequest.TransferBuffer != 0);
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
                            UsbHubDescr->bNumberOfPorts = PdoDeviceExtension->NumberOfPorts;
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
                        ASSERT(Urb->UrbControlVendorClassRequest.TransferBuffer != 0);
                        ((PULONG)Urb->UrbControlVendorClassRequest.TransferBuffer)[0] = 0;
                    }
                    break;
                }
                default:
                {
                    DPRINT1("Unhandled URB request for class device\n");
                    Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
                    ASSERT(FALSE);
                }
            }
            break;
        }
        case URB_FUNCTION_CLASS_OTHER:
        {
            DPRINT("URB_FUNCTION_CLASS_OTHER\n");
            /* FIXME: Each one of these needs to make sure that the index value is a valid for the number of ports and return STATUS_UNSUCCESSFUL if not */

            switch (Urb->UrbControlVendorClassRequest.Request)
            {
                case USB_REQUEST_GET_STATUS:
                {
                    DPRINT("USB_REQUEST_GET_STATUS Port %d\n", Urb->UrbControlVendorClassRequest.Index);

                    ASSERT(Urb->UrbControlVendorClassRequest.TransferBuffer != 0);
                    DPRINT("PortStatus %x\n", PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortStatus);
                    DPRINT("PortChange %x\n", PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange);
                    ((PUSHORT)Urb->UrbControlVendorClassRequest.TransferBuffer)[0] = PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortStatus;
                    ((PUSHORT)Urb->UrbControlVendorClassRequest.TransferBuffer)[1] = PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange;
                    break;
                }
                case USB_REQUEST_CLEAR_FEATURE:
                {
                    switch (Urb->UrbControlVendorClassRequest.Value)
                    {
                        case C_PORT_CONNECTION:
                            DPRINT("C_PORT_CONNECTION\n");
                            PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange &= ~USB_PORT_STATUS_CONNECT;
                            break;
                        case C_PORT_RESET:
                            DPRINT("C_PORT_RESET\n");
                            PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange &= ~USB_PORT_STATUS_RESET;
                            PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortStatus |= USB_PORT_STATUS_ENABLE;
                            break;
                        default:
                            DPRINT("Unknown Value for Clear Feature %x \n", Urb->UrbControlVendorClassRequest.Value);
                            break;
                    }
                    break;
                }
                case USB_REQUEST_SET_FEATURE:
                {
                    DPRINT("USB_REQUEST_SET_FEATURE Port %d, value %x\n", Urb->UrbControlVendorClassRequest.Index,
                        Urb->UrbControlVendorClassRequest.Value);

                    switch(Urb->UrbControlVendorClassRequest.Value)
                    {
                        case PORT_RESET:
                        {
                            DPRINT("Port Reset %d\n", Urb->UrbControlVendorClassRequest.Index-1);
                            PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortChange |= USB_PORT_STATUS_RESET;
                            PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortStatus &= ~USB_PORT_STATUS_ENABLE;
                            ResetPort(&FdoDeviceExtension->hcd, Urb->UrbControlVendorClassRequest.Index-1);
                            break;
                        }
                        case PORT_ENABLE:
                        {
                            DPRINT("PORT_ENABLE not implemented\n");
                            break;
                        }
                        case PORT_POWER:
                        {
                            DPRINT("PORT_POWER not implemented\n");
                            break;
                        }
                        default:
                        {
                            DPRINT("Unknown Set Feature!\n");
                            break;
                        }
                    }

                    if (!(PdoDeviceExtension->Ports[Urb->UrbControlVendorClassRequest.Index-1].PortStatus & 0x8000))
                        CompletePendingURBRequest(PdoDeviceExtension);


                    break;
                }
                case USB_REQUEST_SET_ADDRESS:
                {
                    DPRINT1("USB_REQUEST_SET_ADDRESS\n");
                    ASSERT(FALSE);
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
                    break;
                }
            }
            break;
        }
        case URB_FUNCTION_CONTROL_TRANSFER:
        {
            DPRINT1("URB_FUNCTION_CONTROL_TRANSFER\n");
            DPRINT1("PipeHandle %x\n", Urb->UrbControlTransfer.PipeHandle);
            DPRINT1("TransferFlags %x\n", Urb->UrbControlTransfer.TransferFlags);
            DPRINT1("TransferLength %x\n", Urb->UrbControlTransfer.TransferBufferLength);
            DPRINT1("TransferBuffer %x\n", Urb->UrbControlTransfer.TransferBuffer);
            DPRINT1("TransferMDL %x\n", Urb->UrbControlTransfer.TransferBufferMDL);
            DPRINT1("SetupPacket %x\n", Urb->UrbControlTransfer.SetupPacket);
            ASSERT(FALSE);
            break;
        }
        case URB_FUNCTION_CLASS_INTERFACE:
        {
            DPRINT1("URB_FUNCTION_CLASS_INTERFACE\n");
            break;
        }
        default:
        {
            DPRINT1("Unhandled URB %x\n", Urb->UrbHeader.Function);
            Urb->UrbHeader.Status = USBD_STATUS_INVALID_URB_FUNCTION;
        }
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    return Status;
}

VOID
CompletePendingURBRequest(PPDO_DEVICE_EXTENSION DeviceExtension)
{
    PLIST_ENTRY NextIrp = NULL;
    KIRQL oldIrql;
    PIRP Irp = NULL;

    KeAcquireSpinLock(&DeviceExtension->IrpQueueLock, &oldIrql);

    if (IsListEmpty(&DeviceExtension->IrpQueue))
    {
        DPRINT1("There should have been one SCE request pending\n");
        KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);
        return;
    }
    NextIrp = RemoveHeadList(&DeviceExtension->IrpQueue);
    Irp = CONTAINING_RECORD(NextIrp, IRP, Tail.Overlay.ListEntry);

    if (!Irp)
    {
        DPRINT1("No Irp\n");
        return;
    }

    IoSetCancelRoutine(Irp, NULL);
    KeReleaseSpinLock(&DeviceExtension->IrpQueueLock, oldIrql);

    HandleUrbRequest(DeviceExtension, Irp);
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}

