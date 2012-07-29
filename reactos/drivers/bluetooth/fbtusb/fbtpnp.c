// Copyright (c) 2004, Antony C. Roberts

// Use of this file is subject to the terms
// described in the LICENSE.TXT file that
// accompanies this file.
//
// Your use of this file indicates your
// acceptance of the terms described in
// LICENSE.TXT.
//
// http://www.freebt.net

#include "stdio.h"
#include "fbtusb.h"
#include "fbtpnp.h"
#include "fbtpwr.h"
#include "fbtdev.h"
#include "fbtrwr.h"
#include "fbtwmi.h"

#include "fbtusr.h"

// Handle PNP events
NTSTATUS NTAPI FreeBT_DispatchPnP(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PIO_STACK_LOCATION irpStack;
    PDEVICE_EXTENSION  deviceExtension;
    //KEVENT             startDeviceEvent;
    NTSTATUS           ntStatus;

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // since the device is removed, fail the Irp.
    if (Removed == deviceExtension->DeviceState)
    {
        ntStatus = STATUS_DELETE_PENDING;
        Irp->IoStatus.Status = ntStatus;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        return ntStatus;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPnP::"));
    FreeBT_IoIncrement(deviceExtension);
    if (irpStack->MinorFunction == IRP_MN_START_DEVICE)
    {
        ASSERT(deviceExtension->IdleReqPend == 0);

    }

    else
    {
        if (deviceExtension->SSEnable)
        {
            CancelSelectSuspend(deviceExtension);

        }

    }

    FreeBT_DbgPrint(3, ("FBTUSB: ///////////////////////////////////////////\n"));
    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPnP::"));
    FreeBT_DbgPrint(2, (PnPMinorFunctionString(irpStack->MinorFunction)));
    switch (irpStack->MinorFunction)
    {
    case IRP_MN_START_DEVICE:
        ntStatus = HandleStartDevice(DeviceObject, Irp);
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        // if we cannot stop the device, we fail the query stop irp
        ntStatus = CanStopDevice(DeviceObject, Irp);
        if(NT_SUCCESS(ntStatus))
        {
            ntStatus = HandleQueryStopDevice(DeviceObject, Irp);
            return ntStatus;

        }

        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        ntStatus = HandleCancelStopDevice(DeviceObject, Irp);
        break;

    case IRP_MN_STOP_DEVICE:
        ntStatus = HandleStopDevice(DeviceObject, Irp);
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPnP::IRP_MN_STOP_DEVICE::"));
        FreeBT_IoDecrement(deviceExtension);

        return ntStatus;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        // if we cannot remove the device, we fail the query remove irp
        ntStatus = HandleQueryRemoveDevice(DeviceObject, Irp);

        return ntStatus;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        ntStatus = HandleCancelRemoveDevice(DeviceObject, Irp);
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        ntStatus = HandleSurpriseRemoval(DeviceObject, Irp);
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPnP::IRP_MN_SURPRISE_REMOVAL::"));
        FreeBT_IoDecrement(deviceExtension);
        return ntStatus;

    case IRP_MN_REMOVE_DEVICE:
        ntStatus = HandleRemoveDevice(DeviceObject, Irp);
        return ntStatus;

    case IRP_MN_QUERY_CAPABILITIES:
        ntStatus = HandleQueryCapabilities(DeviceObject, Irp);
        break;

    default:
        IoSkipCurrentIrpStackLocation(Irp);

        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPnP::default::"));
        FreeBT_IoDecrement(deviceExtension);

        return ntStatus;

    }

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchPnP::"));
    FreeBT_IoDecrement(deviceExtension);

    return ntStatus;

}

NTSTATUS NTAPI HandleStartDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    KEVENT            startDeviceEvent;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;
    LARGE_INTEGER     dueTime;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleStartDevice: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    deviceExtension->UsbConfigurationDescriptor = NULL;
    deviceExtension->UsbInterface = NULL;
    deviceExtension->PipeContext = NULL;

    // We cannot touch the device (send it any non pnp irps) until a
    // start device has been passed down to the lower drivers.
    // first pass the Irp down
    KeInitializeEvent(&startDeviceEvent, NotificationEvent, FALSE);
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine,
                           (PVOID)&startDeviceEvent,
                           TRUE,
                           TRUE,
                           TRUE);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&startDeviceEvent, Executive, KernelMode, FALSE, NULL);
        ntStatus = Irp->IoStatus.Status;

    }

    if (!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: HandleStartDevice: Lower drivers failed this Irp (0x%08x)\n", ntStatus));
        return ntStatus;

    }

    // Read the device descriptor, configuration descriptor
    // and select the interface descriptors
    ntStatus = ReadandSelectDescriptors(DeviceObject);
    if (!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: HandleStartDevice: ReadandSelectDescriptors failed (0x%08x)\n", ntStatus));
        return ntStatus;

    }

    // enable the symbolic links for system components to open
    // handles to the device
    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->InterfaceName, TRUE);
    if (!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: HandleStartDevice: IoSetDeviceInterfaceState failed (0x%08x)\n", ntStatus));
        return ntStatus;

    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    SET_NEW_PNP_STATE(deviceExtension, Working);
    deviceExtension->QueueState = AllowRequests;

    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    deviceExtension->FlagWWOutstanding = 0;
    deviceExtension->FlagWWCancel = 0;
    deviceExtension->WaitWakeIrp = NULL;

    if (deviceExtension->WaitWakeEnable)
    {
        IssueWaitWake(deviceExtension);

    }

    ProcessQueuedRequests(deviceExtension);
    if (WinXpOrBetter == deviceExtension->WdmVersion)
    {
        deviceExtension->SSEnable = deviceExtension->SSRegistryEnable;

        // set timer.for selective suspend requests
        if (deviceExtension->SSEnable)
        {
            dueTime.QuadPart = -10000 * IDLE_INTERVAL;               // 5000 ms
            KeSetTimerEx(&deviceExtension->Timer, dueTime, IDLE_INTERVAL, &deviceExtension->DeferredProcCall);
            deviceExtension->FreeIdleIrpCount = 0;

        }

    }

    FreeBT_DbgPrint(3, ("FBTUSB: HandleStartDevice: Leaving\n"));

    return ntStatus;

}


NTSTATUS NTAPI ReadandSelectDescriptors(IN PDEVICE_OBJECT DeviceObject)
{
    PURB                   urb;
    ULONG                  siz;
    NTSTATUS               ntStatus;
    PUSB_DEVICE_DESCRIPTOR deviceDescriptor;

    urb = NULL;
    deviceDescriptor = NULL;

    // 1. Read the device descriptor
    urb = (PURB) ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    if(urb)
    {
        siz = sizeof(USB_DEVICE_DESCRIPTOR);
        deviceDescriptor = (PUSB_DEVICE_DESCRIPTOR) ExAllocatePool(NonPagedPool, siz);
        if (deviceDescriptor)
        {
            UsbBuildGetDescriptorRequest(
                    urb,
                    (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                    USB_DEVICE_DESCRIPTOR_TYPE,
                    0,
                    0,
                    deviceDescriptor,
                    NULL,
                    siz,
                    NULL);

            ntStatus = CallUSBD(DeviceObject, urb);
            if (NT_SUCCESS(ntStatus))
            {
                ASSERT(deviceDescriptor->bNumConfigurations);
                ntStatus = ConfigureDevice(DeviceObject);

            }

            ExFreePool(urb);
            ExFreePool(deviceDescriptor);

        }

        else
        {
            FreeBT_DbgPrint(1, ("FBTUSB: ReadandSelectDescriptors: Failed to allocate memory for deviceDescriptor"));
            ExFreePool(urb);
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;

        }

    }

    else
    {
        FreeBT_DbgPrint(1, ("FBTUSB: ReadandSelectDescriptors: Failed to allocate memory for urb"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    }


    return ntStatus;

}

NTSTATUS NTAPI ConfigureDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PURB                          urb;
    ULONG                         siz;
    NTSTATUS                      ntStatus;
    PDEVICE_EXTENSION             deviceExtension;
    PUSB_CONFIGURATION_DESCRIPTOR configurationDescriptor;

    urb = NULL;
    configurationDescriptor = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // Read the first configuration descriptor
    // This requires two steps:
    // 1. Read the fixed sized configuration desciptor (CD)
    // 2. Read the CD with all embedded interface and endpoint descriptors
    urb = (PURB) ExAllocatePool(NonPagedPool, sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST));
    if (urb)
    {
        siz = sizeof(USB_CONFIGURATION_DESCRIPTOR);
        configurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR) ExAllocatePool(NonPagedPool, siz);

        if(configurationDescriptor)
        {
            UsbBuildGetDescriptorRequest(
                    urb,
                    (USHORT) sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                    USB_CONFIGURATION_DESCRIPTOR_TYPE,
                    0,
                    0,
                    configurationDescriptor,
                    NULL,
                    sizeof(USB_CONFIGURATION_DESCRIPTOR),
                    NULL);

            ntStatus = CallUSBD(DeviceObject, urb);
            if(!NT_SUCCESS(ntStatus))
            {
                FreeBT_DbgPrint(1, ("FBTUSB: ConfigureDevice: UsbBuildGetDescriptorRequest failed\n"));
                goto ConfigureDevice_Exit;

            }

        }

        else
        {
            FreeBT_DbgPrint(1, ("FBTUSB: ConfigureDevice: Failed to allocate mem for config Descriptor\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto ConfigureDevice_Exit;

        }

        siz = configurationDescriptor->wTotalLength;
        ExFreePool(configurationDescriptor);

        configurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR) ExAllocatePool(NonPagedPool, siz);
        if (configurationDescriptor)
        {
            UsbBuildGetDescriptorRequest(
                    urb,
                    (USHORT)sizeof(struct _URB_CONTROL_DESCRIPTOR_REQUEST),
                    USB_CONFIGURATION_DESCRIPTOR_TYPE,
                    0,
                    0,
                    configurationDescriptor,
                    NULL,
                    siz,
                    NULL);

            ntStatus = CallUSBD(DeviceObject, urb);
            if (!NT_SUCCESS(ntStatus))
            {
                FreeBT_DbgPrint(1,("FBTUSB: ConfigureDevice: Failed to read configuration descriptor"));
                goto ConfigureDevice_Exit;

            }

        }

        else
        {
            FreeBT_DbgPrint(1, ("FBTUSB: ConfigureDevice: Failed to alloc mem for config Descriptor\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            goto ConfigureDevice_Exit;

        }

    }

    else
    {
        FreeBT_DbgPrint(1, ("FBTUSB: ConfigureDevice: Failed to allocate memory for urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
        goto ConfigureDevice_Exit;

    }

    if (configurationDescriptor)
    {
        // save a copy of configurationDescriptor in deviceExtension
        // remember to free it later.
        deviceExtension->UsbConfigurationDescriptor = configurationDescriptor;

        if (configurationDescriptor->bmAttributes & REMOTE_WAKEUP_MASK)
        {
            // this configuration supports remote wakeup
            deviceExtension->WaitWakeEnable = 1;

        }

        else
        {
            deviceExtension->WaitWakeEnable = 0;

        }

        ntStatus = SelectInterfaces(DeviceObject, configurationDescriptor);

    }

    else
    {
        deviceExtension->UsbConfigurationDescriptor = NULL;

    }

ConfigureDevice_Exit:
    if (urb)
    {
        ExFreePool(urb);

    }

    return ntStatus;

}

NTSTATUS NTAPI SelectInterfaces(IN PDEVICE_OBJECT DeviceObject, IN PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor)
{
    LONG                        numberOfInterfaces, interfaceNumber, interfaceindex;
    ULONG                       i;
    PURB                        urb;
    //PUCHAR                      pInf;
    NTSTATUS                    ntStatus;
    PDEVICE_EXTENSION           deviceExtension;
    PUSB_INTERFACE_DESCRIPTOR   interfaceDescriptor;
    PUSBD_INTERFACE_LIST_ENTRY  interfaceList,
                                tmp;
    PUSBD_INTERFACE_INFORMATION Interface;

    urb = NULL;
    Interface = NULL;
    interfaceDescriptor = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    numberOfInterfaces = ConfigurationDescriptor->bNumInterfaces;
    interfaceindex = interfaceNumber = 0;

    // Parse the configuration descriptor for the interface;
    tmp = interfaceList = (PUSBD_INTERFACE_LIST_ENTRY)
        ExAllocatePool(NonPagedPool, sizeof(USBD_INTERFACE_LIST_ENTRY) * (numberOfInterfaces + 1));

    if (!tmp)
    {

        FreeBT_DbgPrint(1, ("FBTUSB: SelectInterfaces: Failed to allocate mem for interfaceList\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }


    FreeBT_DbgPrint(3, ("FBTUSB: -------------\n"));
    FreeBT_DbgPrint(3, ("FBTUSB: Number of interfaces %d\n", numberOfInterfaces));

    while (interfaceNumber < numberOfInterfaces)
    {
        interfaceDescriptor = USBD_ParseConfigurationDescriptorEx(
                                            ConfigurationDescriptor,
                                            ConfigurationDescriptor,
                                            interfaceindex,
                                            0, -1, -1, -1);

        if (interfaceDescriptor)
        {
            interfaceList->InterfaceDescriptor = interfaceDescriptor;
            interfaceList->Interface = NULL;
            interfaceList++;
            interfaceNumber++;

        }

        interfaceindex++;

    }

    interfaceList->InterfaceDescriptor = NULL;
    interfaceList->Interface = NULL;
    urb = USBD_CreateConfigurationRequestEx(ConfigurationDescriptor, tmp);

    if (urb)
    {
        Interface = &urb->UrbSelectConfiguration.Interface;
        for (i=0; i<Interface->NumberOfPipes; i++)
        {
            // perform pipe initialization here
            // set the transfer size and any pipe flags we use
            // USBD sets the rest of the Interface struct members
            Interface->Pipes[i].MaximumTransferSize = USBD_DEFAULT_MAXIMUM_TRANSFER_SIZE;

        }

        ntStatus = CallUSBD(DeviceObject, urb);
        if (NT_SUCCESS(ntStatus))
        {
            // save a copy of interface information in the device extension.
            deviceExtension->UsbInterface = (PUSBD_INTERFACE_INFORMATION) ExAllocatePool(NonPagedPool, Interface->Length);
            if (deviceExtension->UsbInterface)
            {
                RtlCopyMemory(deviceExtension->UsbInterface, Interface, Interface->Length);

            }

            else
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                FreeBT_DbgPrint(1, ("FBTUSB: SelectInterfaces: Memory alloc for UsbInterface failed\n"));

            }

            // Dump the interface to the debugger
            Interface = &urb->UrbSelectConfiguration.Interface;

            FreeBT_DbgPrint(3, ("FBTUSB: ---------\n"));
            FreeBT_DbgPrint(3, ("FBTUSB: NumberOfPipes 0x%x\n", Interface->NumberOfPipes));
            FreeBT_DbgPrint(3, ("FBTUSB: Length 0x%x\n", Interface->Length));
            FreeBT_DbgPrint(3, ("FBTUSB: Alt Setting 0x%x\n", Interface->AlternateSetting));
            FreeBT_DbgPrint(3, ("FBTUSB: Interface Number 0x%x\n", Interface->InterfaceNumber));
            FreeBT_DbgPrint(3, ("FBTUSB: Class, subclass, protocol 0x%x 0x%x 0x%x\n",
                                 Interface->Class,
                                 Interface->SubClass,
                                 Interface->Protocol));

            if (Interface->Class==FREEBT_USB_STDCLASS && Interface->SubClass==FREEBT_USB_STDSUBCLASS &&
                Interface->Protocol==FREEBT_USB_STDPROTOCOL)
            {
                FreeBT_DbgPrint(3, ("FBTUSB: This is a standard USB Bluetooth device\n"));

            }

            else
            {
                FreeBT_DbgPrint(3, ("FBTUSB: WARNING: This device does not report itself as a standard USB Bluetooth device\n"));

            }

            // Initialize the PipeContext
            // Dump the pipe info
            deviceExtension->PipeContext = (PFREEBT_PIPE_CONTEXT) ExAllocatePool(
                                                NonPagedPool,
                                                Interface->NumberOfPipes *
                                                sizeof(FREEBT_PIPE_CONTEXT));

            if (!deviceExtension->PipeContext)
            {
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                FreeBT_DbgPrint(1, ("FBTUSB: Memory alloc for UsbInterface failed\n"));

            }

            else
            {
                FreeBT_DbgPrint(3, ("FBTUSB: SelectInterfaces: Allocated PipeContext %p\n", deviceExtension->PipeContext));
                for (i=0; i<Interface->NumberOfPipes; i++)
                {
                    deviceExtension->PipeContext[i].PipeOpen = FALSE;

                    FreeBT_DbgPrint(3, ("FBTUSB: ---------\n"));
                    FreeBT_DbgPrint(3, ("FBTUSB: PipeType 0x%x\n", Interface->Pipes[i].PipeType));
                    FreeBT_DbgPrint(3, ("FBTUSB: EndpointAddress 0x%x\n", Interface->Pipes[i].EndpointAddress));
                    FreeBT_DbgPrint(3, ("FBTUSB: MaxPacketSize 0x%x\n", Interface->Pipes[i].MaximumPacketSize));
                    FreeBT_DbgPrint(3, ("FBTUSB: Interval 0x%x\n", Interface->Pipes[i].Interval));
                    FreeBT_DbgPrint(3, ("FBTUSB: Handle 0x%x\n", Interface->Pipes[i].PipeHandle));
                    FreeBT_DbgPrint(3, ("FBTUSB: MaximumTransferSize 0x%x\n", Interface->Pipes[i].MaximumTransferSize));

                    // Log the pipes
                    // Note the HCI Command endpoint won't appear here, because the Default Control Pipe
                    // is used for this. The Default Control Pipe is always present at EndPointAddress 0x0
                    switch (Interface->Pipes[i].EndpointAddress)
                    {
                        case FREEBT_STDENDPOINT_HCIEVENT:
                            deviceExtension->PipeContext[i].PipeType=HciEventPipe;
                            deviceExtension->EventPipe=Interface->Pipes[i];
                            FreeBT_DbgPrint(3, ("FBTUSB: HCI Event Endpoint\n"));
                            break;

                        case FREEBT_STDENDPOINT_ACLIN:
                            deviceExtension->PipeContext[i].PipeType=AclDataIn;
                            deviceExtension->DataInPipe=Interface->Pipes[i];
                            FreeBT_DbgPrint(3, ("FBTUSB: ACL Data In Endpoint\n"));
                            break;

                        case FREEBT_STDENDPOINT_ACLOUT:
                            deviceExtension->PipeContext[i].PipeType=AclDataOut;
                            deviceExtension->DataOutPipe=Interface->Pipes[i];
                            FreeBT_DbgPrint(3, ("FBTUSB: ACL Data Out Endpoint\n"));
                            break;

                        case FREEBT_STDENDPOINT_AUDIOIN:
                            deviceExtension->PipeContext[i].PipeType=SCODataIn;
                            deviceExtension->AudioInPipe=Interface->Pipes[i];
                            FreeBT_DbgPrint(3, ("FBTUSB: ACL Data Out Endpoint\n"));
                            break;

                        case FREEBT_STDENDPOINT_AUDIOOUT:
                            deviceExtension->PipeContext[i].PipeType=SCODataOut;
                            deviceExtension->AudioOutPipe=Interface->Pipes[i];
                            FreeBT_DbgPrint(3, ("FBTUSB: ACL Data Out Endpoint\n"));
                            break;

                    }

                }

            }

            FreeBT_DbgPrint(3, ("FBTUSB: ---------\n"));

        }

        else
        {
            FreeBT_DbgPrint(1, ("FBTUSB: SelectInterfaces: Failed to select an interface\n"));

        }

    }

    else
    {
        FreeBT_DbgPrint(1, ("FBTUSB: SelectInterfaces: USBD_CreateConfigurationRequestEx failed\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    }

    if (tmp)
    {
        ExFreePool(tmp);

    }

    if (urb)
    {
        ExFreePool(urb);

    }

    return ntStatus;
}


NTSTATUS NTAPI DeconfigureDevice(IN PDEVICE_OBJECT DeviceObject)
{
    PURB     urb;
    ULONG    siz;
    NTSTATUS ntStatus;

    siz = sizeof(struct _URB_SELECT_CONFIGURATION);
    urb = (PURB) ExAllocatePool(NonPagedPool, siz);
    if (urb)
    {
        UsbBuildSelectConfigurationRequest(urb, (USHORT)siz, NULL);
        ntStatus = CallUSBD(DeviceObject, urb);
        if(!NT_SUCCESS(ntStatus))
        {
            FreeBT_DbgPrint(3, ("FBTUSB: DeconfigureDevice: Failed to deconfigure device\n"));

        }

        ExFreePool(urb);

    }

    else
    {
        FreeBT_DbgPrint(1, ("FBTUSB: DeconfigureDevice: Failed to allocate urb\n"));
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;

    }

    return ntStatus;

}

NTSTATUS NTAPI CallUSBD(IN PDEVICE_OBJECT DeviceObject, IN PURB Urb)
{
    PIRP               irp;
    KEVENT             event;
    NTSTATUS           ntStatus;
    IO_STATUS_BLOCK    ioStatus;
    PIO_STACK_LOCATION nextStack;
    PDEVICE_EXTENSION  deviceExtension;

    irp = NULL;
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    KeInitializeEvent(&event, NotificationEvent, FALSE);
    irp = IoBuildDeviceIoControlRequest(IOCTL_INTERNAL_USB_SUBMIT_URB,
                                        deviceExtension->TopOfStackDeviceObject,
                                        NULL,
                                        0,
                                        NULL,
                                        0,
                                        TRUE,
                                        &event,
                                        &ioStatus);

    if (!irp)
    {
        FreeBT_DbgPrint(1, ("FBTUSB: CallUSBD: IoBuildDeviceIoControlRequest failed\n"));
        return STATUS_INSUFFICIENT_RESOURCES;

    }

    nextStack = IoGetNextIrpStackLocation(irp);
    ASSERT(nextStack != NULL);
    nextStack->Parameters.Others.Argument1 = Urb;

    FreeBT_DbgPrint(3, ("FBTUSB: CallUSBD::"));
    FreeBT_IoIncrement(deviceExtension);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, irp);
    if (ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        ntStatus = ioStatus.Status;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: CallUSBD::"));
    FreeBT_IoDecrement(deviceExtension);
    return ntStatus;

}

NTSTATUS NTAPI HandleQueryStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryStopDevice: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // If we can stop the device, we need to set the QueueState to
    // HoldRequests so further requests will be queued.
    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    SET_NEW_PNP_STATE(deviceExtension, PendingStop);
    deviceExtension->QueueState = HoldRequests;

    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    // wait for the existing ones to be finished.
    // first, decrement this operation
    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryStopDevice::"));
    FreeBT_IoDecrement(deviceExtension);

    KeWaitForSingleObject(&deviceExtension->StopEvent, Executive, KernelMode, FALSE, NULL);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);

    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryStopDevice: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI HandleCancelStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    KEVENT            event;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleCancelStopDevice: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // Send this IRP down and wait for it to come back.
    // Set the QueueState flag to AllowRequests,
    // and process all the previously queued up IRPs.

    // First check to see whether you have received cancel-stop
    // without first receiving a query-stop. This could happen if someone
    // above us fails a query-stop and passes down the subsequent
    // cancel-stop.
    if(PendingStop == deviceExtension->DeviceState)
    {
        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine,
                               (PVOID)&event,
                               TRUE,
                               TRUE,
                               TRUE);

        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
        if(ntStatus == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            ntStatus = Irp->IoStatus.Status;

        }

        if(NT_SUCCESS(ntStatus))
        {
            KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

            RESTORE_PREVIOUS_PNP_STATE(deviceExtension);
            deviceExtension->QueueState = AllowRequests;
            ASSERT(deviceExtension->DeviceState == Working);

            KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

            ProcessQueuedRequests(deviceExtension);

        }

    }

    else
    {
        // spurious Irp
        ntStatus = STATUS_SUCCESS;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: HandleCancelStopDevice: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI HandleStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleStopDevice: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    if(WinXpOrBetter == deviceExtension->WdmVersion)
    {
        if(deviceExtension->SSEnable)
        {
            // Cancel the timer so that the DPCs are no longer fired.
            // Thus, we are making judicious usage of our resources.
            // we do not need DPCs because the device is stopping.
            // The timers are re-initialized while handling the start
            // device irp.
            KeCancelTimer(&deviceExtension->Timer);

            // after the device is stopped, it can be surprise removed.
            // we set this to 0, so that we do not attempt to cancel
            // the timer while handling surprise remove or remove irps.
            // when we get the start device request, this flag will be
            // reinitialized.
            deviceExtension->SSEnable = 0;

            // make sure that if a DPC was fired before we called cancel timer,
            // then the DPC and work-time have run to their completion
            KeWaitForSingleObject(&deviceExtension->NoDpcWorkItemPendingEvent, Executive, KernelMode, FALSE, NULL);

            // make sure that the selective suspend request has been completed.
            KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent, Executive, KernelMode, FALSE, NULL);

        }

    }

    // after the stop Irp is sent to the lower driver object,
    // the driver must not send any more Irps down that touch
    // the device until another Start has occurred.
    if (deviceExtension->WaitWakeEnable)
    {
        CancelWaitWake(deviceExtension);

    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    SET_NEW_PNP_STATE(deviceExtension, Stopped);

    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    // This is the right place to actually give up all the resources used
    // This might include calls to IoDisconnectInterrupt, MmUnmapIoSpace,
    // etc.
    ReleaseMemory(DeviceObject);

    ntStatus = DeconfigureDevice(DeviceObject);

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleStopDevice: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI HandleQueryRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryRemoveDevice: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // If we can allow removal of the device, we should set the QueueState
    // to HoldRequests so further requests will be queued. This is required
    // so that we can process queued up requests in cancel-remove just in
    // case somebody else in the stack fails the query-remove.
    ntStatus = CanRemoveDevice(DeviceObject, Irp);

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

    deviceExtension->QueueState = HoldRequests;
    SET_NEW_PNP_STATE(deviceExtension, PendingRemove);

    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryRemoveDevice::"));
    FreeBT_IoDecrement(deviceExtension);

    // Wait for all the requests to be completed
    KeWaitForSingleObject(&deviceExtension->StopEvent, Executive, KernelMode, FALSE, NULL);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryRemoveDevice: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI HandleCancelRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    KEVENT            event;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleCancelRemoveDevice: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // We need to reset the QueueState flag to ProcessRequest,
    // since the device resume its normal activities.

    // First check to see whether you have received cancel-remove
    // without first receiving a query-remove. This could happen if
    // someone above us fails a query-remove and passes down the
    // subsequent cancel-remove.
    if(PendingRemove == deviceExtension->DeviceState)
    {

        KeInitializeEvent(&event, NotificationEvent, FALSE);

        IoCopyCurrentIrpStackLocationToNext(Irp);
        IoSetCompletionRoutine(Irp,
                               (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine,
                               (PVOID)&event,
                               TRUE,
                               TRUE,
                               TRUE);

        ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
        if(ntStatus == STATUS_PENDING)
        {
            KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
            ntStatus = Irp->IoStatus.Status;

        }

        if (NT_SUCCESS(ntStatus))
        {
            KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);

            deviceExtension->QueueState = AllowRequests;
            RESTORE_PREVIOUS_PNP_STATE(deviceExtension);

            KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

            // process the queued requests that arrive between
            // QUERY_REMOVE and CANCEL_REMOVE
            ProcessQueuedRequests(deviceExtension);

        }

    }

    else
    {
        // spurious cancel-remove
        ntStatus = STATUS_SUCCESS;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: HandleCancelRemoveDevice: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI HandleSurpriseRemoval(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleSurpriseRemoval: Entered\n"));

    // initialize variables
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // 1. fail pending requests
    // 2. return device and memory resources
    // 3. disable interfaces
    if(deviceExtension->WaitWakeEnable)
    {
        CancelWaitWake(deviceExtension);

    }


    if (WinXpOrBetter == deviceExtension->WdmVersion)
    {
        if (deviceExtension->SSEnable)
        {
            // Cancel the timer so that the DPCs are no longer fired.
            // we do not need DPCs because the device has been surprise
            // removed
            KeCancelTimer(&deviceExtension->Timer);

            deviceExtension->SSEnable = 0;

            // make sure that if a DPC was fired before we called cancel timer,
            // then the DPC and work-time have run to their completion
            KeWaitForSingleObject(&deviceExtension->NoDpcWorkItemPendingEvent, Executive, KernelMode, FALSE, NULL);

            // make sure that the selective suspend request has been completed.
            KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent, Executive, KernelMode, FALSE, NULL);

        }

    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);
    deviceExtension->QueueState = FailRequests;
    SET_NEW_PNP_STATE(deviceExtension, SurpriseRemoved);
    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

    ProcessQueuedRequests(deviceExtension);

    ntStatus = IoSetDeviceInterfaceState(&deviceExtension->InterfaceName, FALSE);
    if(!NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(1, ("FBTUSB: HandleSurpriseRemoval: IoSetDeviceInterfaceState::disable:failed\n"));

    }

    FreeBT_AbortPipes(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleSurpriseRemoval: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI HandleRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    KIRQL             oldIrql;
    //KEVENT            event;
    ULONG             requestCount;
    NTSTATUS          ntStatus;
    PDEVICE_EXTENSION deviceExtension;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleRemoveDevice: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;

    // The Plug & Play system has dictated the removal of this device.  We
    // have no choice but to detach and delete the device object.
    // (If we wanted to express an interest in preventing this removal,
    // we should have failed the query remove IRP).
    if(SurpriseRemoved != deviceExtension->DeviceState)
    {

        // we are here after QUERY_REMOVE
        KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);
        deviceExtension->QueueState = FailRequests;
        KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);

        if(deviceExtension->WaitWakeEnable)
        {
            CancelWaitWake(deviceExtension);

        }

        if(WinXpOrBetter == deviceExtension->WdmVersion)
        {
            if (deviceExtension->SSEnable)
            {
                // Cancel the timer so that the DPCs are no longer fired.
                // we do not need DPCs because the device has been removed
                KeCancelTimer(&deviceExtension->Timer);

                deviceExtension->SSEnable = 0;

                // make sure that if a DPC was fired before we called cancel timer,
                // then the DPC and work-time have run to their completion
                KeWaitForSingleObject(&deviceExtension->NoDpcWorkItemPendingEvent, Executive, KernelMode, FALSE, NULL);

                // make sure that the selective suspend request has been completed.
                KeWaitForSingleObject(&deviceExtension->NoIdleReqPendEvent, Executive, KernelMode, FALSE, NULL);

            }

        }

        ProcessQueuedRequests(deviceExtension);

        ntStatus = IoSetDeviceInterfaceState(&deviceExtension->InterfaceName, FALSE);
        if(!NT_SUCCESS(ntStatus))
        {
            FreeBT_DbgPrint(1, ("FBTUSB: HandleRemoveDevice: IoSetDeviceInterfaceState::disable:failed\n"));

        }

        FreeBT_AbortPipes(DeviceObject);

    }

    KeAcquireSpinLock(&deviceExtension->DevStateLock, &oldIrql);
    SET_NEW_PNP_STATE(deviceExtension, Removed);
    KeReleaseSpinLock(&deviceExtension->DevStateLock, oldIrql);
#ifdef ENABLE_WMI
    FreeBT_WmiDeRegistration(deviceExtension);
#endif

    // Need 2 decrements
    FreeBT_DbgPrint(3, ("FBTUSB: HandleRemoveDevice::"));
    requestCount = FreeBT_IoDecrement(deviceExtension);

    ASSERT(requestCount > 0);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleRemoveDevice::"));
    requestCount = FreeBT_IoDecrement(deviceExtension);

    KeWaitForSingleObject(&deviceExtension->RemoveEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    ReleaseMemory(DeviceObject);

    // We need to send the remove down the stack before we detach,
    // but we don't need to wait for the completion of this operation
    // (and to register a completion routine).
    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoSkipCurrentIrpStackLocation(Irp);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);

    IoDetachDevice(deviceExtension->TopOfStackDeviceObject);
    IoDeleteDevice(DeviceObject);

    FreeBT_DbgPrint(3, ("FBTUSB: HandleRemoveDevice: Leaving\n"));

    return ntStatus;

}

NTSTATUS NTAPI HandleQueryCapabilities(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    ULONG                i;
    KEVENT               event;
    NTSTATUS             ntStatus;
    PDEVICE_EXTENSION    deviceExtension;
    PDEVICE_CAPABILITIES pdc;
    PIO_STACK_LOCATION   irpStack;

    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryCapabilities: Entered\n"));

    irpStack = IoGetCurrentIrpStackLocation(Irp);
    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    pdc = irpStack->Parameters.DeviceCapabilities.Capabilities;

    if(pdc->Version < 1 || pdc->Size < sizeof(DEVICE_CAPABILITIES))
    {

        FreeBT_DbgPrint(1, ("FBTUSB: HandleQueryCapabilities::request failed\n"));
        ntStatus = STATUS_UNSUCCESSFUL;
        return ntStatus;

    }

    // Add in the SurpriseRemovalOK bit before passing it down.
    pdc->SurpriseRemovalOK = TRUE;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    KeInitializeEvent(&event, NotificationEvent, FALSE);

    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp,
                           (PIO_COMPLETION_ROUTINE)IrpCompletionRoutine,
                           (PVOID)&event,
                           TRUE,
                           TRUE,
                           TRUE);
    ntStatus = IoCallDriver(deviceExtension->TopOfStackDeviceObject, Irp);
    if(ntStatus == STATUS_PENDING)
    {
        KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);
        ntStatus = Irp->IoStatus.Status;

    }

    // initialize PowerDownLevel to disabled
    deviceExtension->PowerDownLevel = PowerDeviceUnspecified;
    if(NT_SUCCESS(ntStatus))
    {
        deviceExtension->DeviceCapabilities = *pdc;
        for(i = PowerSystemSleeping1; i <= PowerSystemSleeping3; i++)
        {
            if(deviceExtension->DeviceCapabilities.DeviceState[i] < PowerDeviceD3)
            {
                deviceExtension->PowerDownLevel = deviceExtension->DeviceCapabilities.DeviceState[i];

            }

        }

        // since its safe to surprise-remove this device, we shall
        // set the SurpriseRemoveOK flag to supress any dialog to
        // user.
        pdc->SurpriseRemovalOK = 1;

    }

    if(deviceExtension->PowerDownLevel == PowerDeviceUnspecified ||
        deviceExtension->PowerDownLevel <= PowerDeviceD0)
    {
        deviceExtension->PowerDownLevel = PowerDeviceD2;

    }

    FreeBT_DbgPrint(3, ("FBTUSB: HandleQueryCapabilities: Leaving\n"));

    return ntStatus;
}


VOID NTAPI DpcRoutine(IN PKDPC Dpc, IN PVOID DeferredContext, IN PVOID SystemArgument1, IN PVOID SystemArgument2)
/*++

    DPC routine triggered by the timer to check the idle state
    of the device and submit an idle request for the device.

 --*/
{
    NTSTATUS          ntStatus;
    PDEVICE_OBJECT    deviceObject;
    PDEVICE_EXTENSION deviceExtension;
    PIO_WORKITEM      item;

    FreeBT_DbgPrint(3, ("FBTUSB: DpcRoutine: Entered\n"));

    deviceObject = (PDEVICE_OBJECT)DeferredContext;
    deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;

    // Clear this event since a DPC has been fired!
    KeClearEvent(&deviceExtension->NoDpcWorkItemPendingEvent);

    if(CanDeviceSuspend(deviceExtension))
    {
        FreeBT_DbgPrint(3, ("FBTUSB: DpcRoutine: Device is Idle\n"));
        item = IoAllocateWorkItem(deviceObject);

        if (item)
        {
            IoQueueWorkItem(item, IdleRequestWorkerRoutine, DelayedWorkQueue, item);
            ntStatus = STATUS_PENDING;

        }

        else
        {
            FreeBT_DbgPrint(3, ("FBTUSB: DpcRoutine: Cannot alloc memory for work item\n"));
            ntStatus = STATUS_INSUFFICIENT_RESOURCES;
            KeSetEvent(&deviceExtension->NoDpcWorkItemPendingEvent, IO_NO_INCREMENT, FALSE);

        }

    }

    else
    {
        FreeBT_DbgPrint(3, ("FBTUSB: DpcRoutine: Idle event not signaled\n"));
        KeSetEvent(&deviceExtension->NoDpcWorkItemPendingEvent, IO_NO_INCREMENT, FALSE);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: DpcRoutine: Leaving\n"));
}


VOID NTAPI IdleRequestWorkerRoutine(IN PDEVICE_OBJECT DeviceObject, IN PVOID Context)
{
    //PIRP                   irp;
    NTSTATUS               ntStatus;
    PDEVICE_EXTENSION      deviceExtension;
    PIO_WORKITEM           workItem;

    FreeBT_DbgPrint(3, ("FBTUSB: IdleRequestWorkerRoutine: Entered\n"));

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    workItem = (PIO_WORKITEM) Context;

    if(CanDeviceSuspend(deviceExtension))
    {
        FreeBT_DbgPrint(3, ("FBTUSB: IdleRequestWorkerRoutine: Device is idle\n"));
        ntStatus = SubmitIdleRequestIrp(deviceExtension);
        if(!NT_SUCCESS(ntStatus))
        {
            FreeBT_DbgPrint(1, ("FBTUSB: IdleRequestWorkerRoutine: SubmitIdleRequestIrp failed\n"));

        }

    }

    else
    {
        FreeBT_DbgPrint(3, ("FBTUSB: IdleRequestWorkerRoutine: Device is not idle\n"));

    }

    IoFreeWorkItem(workItem);

    KeSetEvent(&deviceExtension->NoDpcWorkItemPendingEvent, IO_NO_INCREMENT, FALSE);

    FreeBT_DbgPrint(3, ("FBTUSB: IdleRequestsWorkerRoutine: Leaving\n"));

}


VOID NTAPI ProcessQueuedRequests(IN OUT PDEVICE_EXTENSION DeviceExtension)
/*++

Routine Description:

    Remove and process the entries in the queue. If this routine is called
    when processing IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE
    or IRP_MN_START_DEVICE, the requests are passed to the next lower driver.
    If the routine is called when IRP_MN_REMOVE_DEVICE is received, the IRPs
    are complete with STATUS_DELETE_PENDING

Arguments:

    DeviceExtension - pointer to device extension

Return Value:

    None

--*/
{
    KIRQL       oldIrql;
    PIRP        nextIrp,
                cancelledIrp;
    PVOID       cancelRoutine;
    LIST_ENTRY  cancelledIrpList;
    PLIST_ENTRY listEntry;

    FreeBT_DbgPrint(3, ("FBTUSB: ProcessQueuedRequests: Entered\n"));

    cancelRoutine = NULL;
    InitializeListHead(&cancelledIrpList);

    // 1.  dequeue the entries in the queue
    // 2.  reset the cancel routine
    // 3.  process them
    // 3a. if the device is active, send them down
    // 3b. else complete with STATUS_DELETE_PENDING
    while(1)
    {
        KeAcquireSpinLock(&DeviceExtension->QueueLock, &oldIrql);
        if(IsListEmpty(&DeviceExtension->NewRequestsQueue))
        {
            KeReleaseSpinLock(&DeviceExtension->QueueLock, oldIrql);
            break;

        }

        listEntry = RemoveHeadList(&DeviceExtension->NewRequestsQueue);
        nextIrp = CONTAINING_RECORD(listEntry, IRP, Tail.Overlay.ListEntry);

        cancelRoutine = IoSetCancelRoutine(nextIrp, NULL);

        // check if its already cancelled
        if (nextIrp->Cancel)
        {
            if(cancelRoutine)
            {
                // the cancel routine for this IRP hasnt been called yet
                // so queue the IRP in the cancelledIrp list and complete
                // after releasing the lock
                InsertTailList(&cancelledIrpList, listEntry);

            }

            else
            {
                // the cancel routine has run
                // it must be waiting to hold the queue lock
                // so initialize the IRPs listEntry
                InitializeListHead(listEntry);

            }

            KeReleaseSpinLock(&DeviceExtension->QueueLock, oldIrql);

        }

        else
        {
            KeReleaseSpinLock(&DeviceExtension->QueueLock, oldIrql);
            if(FailRequests == DeviceExtension->QueueState)
            {
                nextIrp->IoStatus.Information = 0;
                nextIrp->IoStatus.Status = STATUS_DELETE_PENDING;
                IoCompleteRequest(nextIrp, IO_NO_INCREMENT);

            }

            else
            {
                //PIO_STACK_LOCATION irpStack;

                FreeBT_DbgPrint(3, ("FBTUSB: ProcessQueuedRequests::"));
                FreeBT_IoIncrement(DeviceExtension);

                IoSkipCurrentIrpStackLocation(nextIrp);
                IoCallDriver(DeviceExtension->TopOfStackDeviceObject, nextIrp);

                FreeBT_DbgPrint(3, ("FBTUSB: ProcessQueuedRequests::"));
                FreeBT_IoDecrement(DeviceExtension);

            }

        }

    }

    while(!IsListEmpty(&cancelledIrpList))
    {
        PLIST_ENTRY cancelEntry = RemoveHeadList(&cancelledIrpList);

        cancelledIrp = CONTAINING_RECORD(cancelEntry, IRP, Tail.Overlay.ListEntry);
        cancelledIrp->IoStatus.Status = STATUS_CANCELLED;
        cancelledIrp->IoStatus.Information = 0;

        IoCompleteRequest(cancelledIrp, IO_NO_INCREMENT);

    }

    FreeBT_DbgPrint(3, ("FBTUSB: ProcessQueuedRequests: Leaving\n"));

    return;

}

NTSTATUS NTAPI FreeBT_GetRegistryDword(IN PWCHAR RegPath, IN PWCHAR ValueName, IN OUT PULONG Value)
{
    ULONG                    defaultData;
    WCHAR                    buffer[MAXIMUM_FILENAME_LENGTH];
    NTSTATUS                 ntStatus;
    UNICODE_STRING           regPath;
    RTL_QUERY_REGISTRY_TABLE paramTable[2];

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetRegistryDword: Entered\n"));

    regPath.Length = 0;
    regPath.MaximumLength = MAXIMUM_FILENAME_LENGTH * sizeof(WCHAR);
    regPath.Buffer = buffer;

    RtlZeroMemory(regPath.Buffer, regPath.MaximumLength);
    RtlMoveMemory(regPath.Buffer, RegPath, wcslen(RegPath) * sizeof(WCHAR));
    RtlZeroMemory(paramTable, sizeof(paramTable));

    paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[0].Name = ValueName;
    paramTable[0].EntryContext = Value;
    paramTable[0].DefaultType = REG_DWORD;
    paramTable[0].DefaultData = &defaultData;
    paramTable[0].DefaultLength = sizeof(ULONG);

    ntStatus = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE |
                                      RTL_REGISTRY_OPTIONAL,
                                      regPath.Buffer,
                                      paramTable,
                                      NULL,
                                      NULL);

    if (NT_SUCCESS(ntStatus))
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetRegistryDword: Success, Value = %X\n", *Value));
        return STATUS_SUCCESS;
    }

    else
    {
        FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_GetRegistryDword: Failed\n"));
        *Value = 0;
        return STATUS_UNSUCCESSFUL;

    }
}


NTSTATUS NTAPI FreeBT_DispatchClean(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    PDEVICE_EXTENSION  deviceExtension;
    KIRQL              oldIrql;
    LIST_ENTRY         cleanupList;
    PLIST_ENTRY        thisEntry,
                       nextEntry,
                       listHead;
    PIRP               pendingIrp;
    PIO_STACK_LOCATION pendingIrpStack,
                       irpStack;
    //NTSTATUS           ntStatus;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);
    InitializeListHead(&cleanupList);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchClean::"));
    FreeBT_IoIncrement(deviceExtension);

    KeAcquireSpinLock(&deviceExtension->QueueLock, &oldIrql);

    listHead = &deviceExtension->NewRequestsQueue;
    for(thisEntry = listHead->Flink, nextEntry = thisEntry->Flink;
       thisEntry != listHead;
       thisEntry = nextEntry, nextEntry = thisEntry->Flink)
    {
        pendingIrp = CONTAINING_RECORD(thisEntry, IRP, Tail.Overlay.ListEntry);
        pendingIrpStack = IoGetCurrentIrpStackLocation(pendingIrp);
        if (irpStack->FileObject == pendingIrpStack->FileObject)
        {
            RemoveEntryList(thisEntry);

            if (NULL == IoSetCancelRoutine(pendingIrp, NULL))
            {
                InitializeListHead(thisEntry);

            }

            else
            {
                InsertTailList(&cleanupList, thisEntry);

            }

        }

    }

    KeReleaseSpinLock(&deviceExtension->QueueLock, oldIrql);

    while(!IsListEmpty(&cleanupList))
    {
        thisEntry = RemoveHeadList(&cleanupList);
        pendingIrp = CONTAINING_RECORD(thisEntry, IRP, Tail.Overlay.ListEntry);

        pendingIrp->IoStatus.Information = 0;
        pendingIrp->IoStatus.Status = STATUS_CANCELLED;
        IoCompleteRequest(pendingIrp, IO_NO_INCREMENT);

    }

    Irp->IoStatus.Information = 0;
    Irp->IoStatus.Status = STATUS_SUCCESS;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_DispatchClean::"));
    FreeBT_IoDecrement(deviceExtension);

    return STATUS_SUCCESS;

}


BOOLEAN NTAPI CanDeviceSuspend(IN PDEVICE_EXTENSION DeviceExtension)
{
    FreeBT_DbgPrint(3, ("FBTUSB: CanDeviceSuspend: Entered\n"));

    if ((DeviceExtension->OpenHandleCount == 0) && (DeviceExtension->OutStandingIO == 1))
        return TRUE;

    return FALSE;

}

NTSTATUS NTAPI FreeBT_AbortPipes(IN PDEVICE_OBJECT DeviceObject)
{
    PURB                        urb;
    ULONG                       i;
    NTSTATUS                    ntStatus;
    PDEVICE_EXTENSION           deviceExtension;
    PFREEBT_PIPE_CONTEXT        pipeContext;
    //PUSBD_PIPE_INFORMATION      pipeInformation;
    PUSBD_INTERFACE_INFORMATION interfaceInfo;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    pipeContext = deviceExtension->PipeContext;
    interfaceInfo = deviceExtension->UsbInterface;

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_AbortPipes: Entered\n"));

    if(interfaceInfo == NULL || pipeContext == NULL)
        return STATUS_SUCCESS;

    for(i=0; i<interfaceInfo->NumberOfPipes; i++)
    {
        if(pipeContext[i].PipeOpen)
        {
            FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_AbortPipes: Aborting open pipe %d\n", i));

            urb = (PURB) ExAllocatePool(NonPagedPool, sizeof(struct _URB_PIPE_REQUEST));
            if (urb)
            {
                urb->UrbHeader.Length = sizeof(struct _URB_PIPE_REQUEST);
                urb->UrbHeader.Function = URB_FUNCTION_ABORT_PIPE;
                urb->UrbPipeRequest.PipeHandle = interfaceInfo->Pipes[i].PipeHandle;

                ntStatus = CallUSBD(DeviceObject, urb);

                ExFreePool(urb);

            }

            else
            {
                FreeBT_DbgPrint(1, ("FBTUSB: FreeBT_AbortPipes: Failed to alloc memory for urb\n"));
                ntStatus = STATUS_INSUFFICIENT_RESOURCES;
                return ntStatus;

            }

            if(NT_SUCCESS(ntStatus))
                pipeContext[i].PipeOpen = FALSE;


        }

    }

    FreeBT_DbgPrint(3, ("FBTUSB: FreeBT_AbortPipes: Leaving\n"));

    return STATUS_SUCCESS;

}

// Completion routine for PNP IRPs
NTSTATUS NTAPI IrpCompletionRoutine(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PKEVENT event = (PKEVENT) Context;
    KeSetEvent(event, 0, FALSE);

    return STATUS_MORE_PROCESSING_REQUIRED;

}


LONG NTAPI FreeBT_IoIncrement(IN OUT PDEVICE_EXTENSION DeviceExtension)
{
    LONG  result = 0;
    KIRQL oldIrql;

    KeAcquireSpinLock(&DeviceExtension->IOCountLock, &oldIrql);
    result = InterlockedIncrement((PLONG)(&DeviceExtension->OutStandingIO));

    // When OutStandingIO bumps from 1 to 2, clear the StopEvent
    if (result == 2)
        KeClearEvent(&DeviceExtension->StopEvent);

    KeReleaseSpinLock(&DeviceExtension->IOCountLock, oldIrql);

    FreeBT_DbgPrint(3, ("FreeBT_IoIncrement::%d\n", result));

    return result;

}

LONG NTAPI FreeBT_IoDecrement(IN OUT PDEVICE_EXTENSION DeviceExtension)
{
    LONG  result = 0;
    KIRQL oldIrql;

    KeAcquireSpinLock(&DeviceExtension->IOCountLock, &oldIrql);

    result = InterlockedDecrement((PLONG)(&DeviceExtension->OutStandingIO));

    if (result == 1)
        KeSetEvent(&DeviceExtension->StopEvent, IO_NO_INCREMENT, FALSE);

    if(result == 0)
    {
        ASSERT(Removed == DeviceExtension->DeviceState);
        KeSetEvent(&DeviceExtension->RemoveEvent, IO_NO_INCREMENT, FALSE);

    }

    KeReleaseSpinLock(&DeviceExtension->IOCountLock, oldIrql);

    FreeBT_DbgPrint(3, ("FreeBT_IoDecrement::%d\n", result));

    return result;

}

NTSTATUS NTAPI CanStopDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
   // For the time being, just allow it to be stopped
   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);

   return STATUS_SUCCESS;

}

NTSTATUS NTAPI CanRemoveDevice(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)

{
   // For the time being, just allow it to be removed
   UNREFERENCED_PARAMETER(DeviceObject);
   UNREFERENCED_PARAMETER(Irp);

   return STATUS_SUCCESS;

}

NTSTATUS NTAPI ReleaseMemory(IN PDEVICE_OBJECT DeviceObject)
{
    // Disconnect from the interrupt and unmap any I/O ports
    PDEVICE_EXTENSION   deviceExtension;
    UNICODE_STRING      uniDeviceName;
    NTSTATUS            ntStatus;

    deviceExtension = (PDEVICE_EXTENSION) DeviceObject->DeviceExtension;
    if (deviceExtension->UsbConfigurationDescriptor)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: ReleaseMemory: Freeing UsbConfigurationDescriptor\n"));
        ExFreePool(deviceExtension->UsbConfigurationDescriptor);
        deviceExtension->UsbConfigurationDescriptor = NULL;

    }

    if(deviceExtension->UsbInterface)
    {
        FreeBT_DbgPrint(3, ("FBTUSB: ReleaseMemory: Freeing UsbInterface\n"));
        ExFreePool(deviceExtension->UsbInterface);
        deviceExtension->UsbInterface = NULL;

    }

    if(deviceExtension->PipeContext)
    {
        RtlInitUnicodeString(&uniDeviceName, deviceExtension->wszDosDeviceName);
        ntStatus = IoDeleteSymbolicLink(&uniDeviceName);
        if (!NT_SUCCESS(ntStatus))
            FreeBT_DbgPrint(3, ("FBTUSB: Failed to delete symbolic link %ws\n", deviceExtension->wszDosDeviceName));

        FreeBT_DbgPrint(3, ("FBTUSB: ReleaseMemory: Freeing PipeContext %p\n", deviceExtension->PipeContext));
        ExFreePool(deviceExtension->PipeContext);
        deviceExtension->PipeContext = NULL;

    }

    return STATUS_SUCCESS;

}

PCHAR NTAPI PnPMinorFunctionString (UCHAR MinorFunction)
{
    switch (MinorFunction)
    {
        case IRP_MN_START_DEVICE:
            return "IRP_MN_START_DEVICE\n";

        case IRP_MN_QUERY_REMOVE_DEVICE:
            return "IRP_MN_QUERY_REMOVE_DEVICE\n";

        case IRP_MN_REMOVE_DEVICE:
            return "IRP_MN_REMOVE_DEVICE\n";

        case IRP_MN_CANCEL_REMOVE_DEVICE:
            return "IRP_MN_CANCEL_REMOVE_DEVICE\n";

        case IRP_MN_STOP_DEVICE:
            return "IRP_MN_STOP_DEVICE\n";

        case IRP_MN_QUERY_STOP_DEVICE:
            return "IRP_MN_QUERY_STOP_DEVICE\n";

        case IRP_MN_CANCEL_STOP_DEVICE:
            return "IRP_MN_CANCEL_STOP_DEVICE\n";

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            return "IRP_MN_QUERY_DEVICE_RELATIONS\n";

        case IRP_MN_QUERY_INTERFACE:
            return "IRP_MN_QUERY_INTERFACE\n";

        case IRP_MN_QUERY_CAPABILITIES:
            return "IRP_MN_QUERY_CAPABILITIES\n";

        case IRP_MN_QUERY_RESOURCES:
            return "IRP_MN_QUERY_RESOURCES\n";

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            return "IRP_MN_QUERY_RESOURCE_REQUIREMENTS\n";

        case IRP_MN_QUERY_DEVICE_TEXT:
            return "IRP_MN_QUERY_DEVICE_TEXT\n";

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            return "IRP_MN_FILTER_RESOURCE_REQUIREMENTS\n";

        case IRP_MN_READ_CONFIG:
            return "IRP_MN_READ_CONFIG\n";

        case IRP_MN_WRITE_CONFIG:
            return "IRP_MN_WRITE_CONFIG\n";

        case IRP_MN_EJECT:
            return "IRP_MN_EJECT\n";

        case IRP_MN_SET_LOCK:
            return "IRP_MN_SET_LOCK\n";

        case IRP_MN_QUERY_ID:
            return "IRP_MN_QUERY_ID\n";

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            return "IRP_MN_QUERY_PNP_DEVICE_STATE\n";

        case IRP_MN_QUERY_BUS_INFORMATION:
            return "IRP_MN_QUERY_BUS_INFORMATION\n";

        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            return "IRP_MN_DEVICE_USAGE_NOTIFICATION\n";

        case IRP_MN_SURPRISE_REMOVAL:
            return "IRP_MN_SURPRISE_REMOVAL\n";

        default:
            return "IRP_MN_?????\n";

    }

}

