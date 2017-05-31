#include "usbport.h"

#define NDEBUG
#include <debug.h>

VOID
USB_BUSIFFN
USBI_InterfaceReference(IN PVOID BusContext)
{
    DPRINT("USBI_InterfaceReference \n");
}

VOID
USB_BUSIFFN
USBI_InterfaceDereference(IN PVOID BusContext)
{
    DPRINT("USBI_InterfaceDereference \n");
}

/* USB port driver Interface functions */

NTSTATUS
USB_BUSIFFN
USBHI_CreateUsbDevice(IN PVOID BusContext,
                      IN OUT PUSB_DEVICE_HANDLE *UsbdDeviceHandle,
                      IN PUSB_DEVICE_HANDLE UsbdHubDeviceHandle,
                      IN USHORT PortStatus,
                      IN USHORT PortNumber)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PUSB_DEVICE_HANDLE deviceHandle = NULL;
    NTSTATUS Status;

    DPRINT("USBHI_CreateUsbDevice: ... \n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;

    Status = USBPORT_CreateDevice(&deviceHandle,
                                  PdoExtension->FdoDevice,
                                  (PUSBPORT_DEVICE_HANDLE)UsbdHubDeviceHandle,
                                  PortStatus,
                                  PortNumber);

    *UsbdDeviceHandle = deviceHandle;

    return Status;
}

NTSTATUS
USB_BUSIFFN
USBHI_InitializeUsbDevice(IN PVOID BusContext,
                          OUT PUSB_DEVICE_HANDLE UsbdDeviceHandle)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;

    DPRINT("USBHI_InitializeUsbDevice \n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;

    return USBPORT_InitializeDevice((PUSBPORT_DEVICE_HANDLE)UsbdDeviceHandle,
                                    PdoExtension->FdoDevice);
}

NTSTATUS
USB_BUSIFFN
USBHI_GetUsbDescriptors(IN PVOID BusContext,
                        IN PUSB_DEVICE_HANDLE UsbdDeviceHandle,
                        IN PUCHAR DeviceDescBuffer,
                        IN PULONG DeviceDescBufferLen,
                        IN PUCHAR ConfigDescBuffer,
                        IN PULONG ConfigDescBufferLen)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    NTSTATUS Status;

    DPRINT("USBHI_GetUsbDescriptors ...\n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;

    if (DeviceDescBuffer && *DeviceDescBufferLen)
    {
        if (*DeviceDescBufferLen > sizeof(USB_DEVICE_DESCRIPTOR))
            *DeviceDescBufferLen = sizeof(USB_DEVICE_DESCRIPTOR);

        RtlCopyMemory(DeviceDescBuffer,
                      &(((PUSBPORT_DEVICE_HANDLE)UsbdDeviceHandle)->DeviceDescriptor),
                      *DeviceDescBufferLen);
    }

    Status = USBPORT_GetUsbDescriptor((PUSBPORT_DEVICE_HANDLE)UsbdDeviceHandle,
                                      PdoExtension->FdoDevice,
                                      USB_CONFIGURATION_DESCRIPTOR_TYPE,
                                      ConfigDescBuffer,
                                      ConfigDescBufferLen);

    USBPORT_DumpingDeviceDescriptor((PUSB_DEVICE_DESCRIPTOR)DeviceDescBuffer);

    return Status;
}

NTSTATUS
USB_BUSIFFN
USBHI_RemoveUsbDevice(IN PVOID BusContext,
                      IN OUT PUSB_DEVICE_HANDLE UsbdDeviceHandle,
                      IN ULONG Flags)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;

    DPRINT("USBHI_RemoveUsbDevice: UsbdDeviceHandle - %p, Flags - %x\n",
           UsbdDeviceHandle,
           Flags);

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;

    return USBPORT_RemoveDevice(PdoExtension->FdoDevice,
                                (PUSBPORT_DEVICE_HANDLE)UsbdDeviceHandle,
                                Flags);
}

NTSTATUS
USB_BUSIFFN
USBHI_RestoreUsbDevice(IN PVOID BusContext,
                       OUT PUSB_DEVICE_HANDLE OldUsbdDeviceHandle,
                       OUT PUSB_DEVICE_HANDLE NewUsbdDeviceHandle)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;

    DPRINT("USBHI_RestoreUsbDevice: OldUsbdDeviceHandle - %p, NewUsbdDeviceHandle - %x\n",
           OldUsbdDeviceHandle,
           NewUsbdDeviceHandle);

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;

    return USBPORT_RestoreDevice(PdoExtension->FdoDevice,
                                (PUSBPORT_DEVICE_HANDLE)OldUsbdDeviceHandle,
                                (PUSBPORT_DEVICE_HANDLE)NewUsbdDeviceHandle);
}

NTSTATUS
USB_BUSIFFN
USBHI_QueryDeviceInformation(IN PVOID BusContext,
                             IN PUSB_DEVICE_HANDLE UsbdDeviceHandle,
                             OUT PVOID DeviceInfoBuffer,
                             IN ULONG DeviceInfoBufferLen,
                             OUT PULONG LenDataReturned)
{
    PUSB_DEVICE_INFORMATION_0 DeviceInfo;
    PUSBPORT_CONFIGURATION_HANDLE ConfigHandle;
    PLIST_ENTRY InterfaceEntry;
    PUSBPORT_DEVICE_HANDLE DeviceHandle;
    ULONG NumberOfOpenPipes = 0;
    PUSB_PIPE_INFORMATION_0 PipeInfo;
    PUSBPORT_PIPE_HANDLE PipeHandle;
    PUSBPORT_INTERFACE_HANDLE InterfaceHandle;
    ULONG ActualLength;
    ULONG ix;
    ULONG jx;

    DPRINT("USBHI_QueryDeviceInformation: ... \n");

    *LenDataReturned = 0;

    if (DeviceInfoBufferLen < (2 * sizeof(ULONG)))
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    DeviceInfo = (PUSB_DEVICE_INFORMATION_0)DeviceInfoBuffer;

    if (DeviceInfo->InformationLevel > 0)
    {
        return STATUS_NOT_SUPPORTED;
    }

    DeviceHandle = (PUSBPORT_DEVICE_HANDLE)UsbdDeviceHandle;
    ConfigHandle = DeviceHandle->ConfigHandle;

    if (ConfigHandle)
    {
        InterfaceEntry = ConfigHandle->InterfaceHandleList.Flink;

        if (!IsListEmpty(&ConfigHandle->InterfaceHandleList))
        {
            while (InterfaceEntry)
            {
                if (InterfaceEntry == &ConfigHandle->InterfaceHandleList)
                    break;

                InterfaceHandle = CONTAINING_RECORD(InterfaceEntry,
                                                    USBPORT_INTERFACE_HANDLE,
                                                    InterfaceLink);

                NumberOfOpenPipes += InterfaceHandle->InterfaceDescriptor.bNumEndpoints;

                InterfaceEntry = InterfaceEntry->Flink;
            }
        }
    }

    ActualLength = sizeof(USB_DEVICE_INFORMATION_0) + 
                   (NumberOfOpenPipes - 1) * sizeof(USB_PIPE_INFORMATION_0);

    if (DeviceInfoBufferLen < ActualLength)
    {
        DeviceInfo->ActualLength = ActualLength;
        *LenDataReturned = 2 * sizeof(ULONG);

        return STATUS_BUFFER_TOO_SMALL;
    }

    RtlZeroMemory(DeviceInfo, ActualLength);

    DeviceInfo->InformationLevel = 0;
    DeviceInfo->ActualLength = ActualLength;
    DeviceInfo->DeviceAddress = DeviceHandle->DeviceAddress;
    DeviceInfo->NumberOfOpenPipes = NumberOfOpenPipes;
    DeviceInfo->DeviceSpeed = DeviceHandle->DeviceSpeed;

    RtlCopyMemory(&DeviceInfo->DeviceDescriptor,
                  &DeviceHandle->DeviceDescriptor,
                  sizeof(USB_DEVICE_DESCRIPTOR));

    USBPORT_DumpingDeviceDescriptor(&DeviceInfo->DeviceDescriptor);

    if (DeviceHandle->DeviceSpeed >= 0)
    {
        if (DeviceHandle->DeviceSpeed == UsbFullSpeed ||
            DeviceHandle->DeviceSpeed == UsbLowSpeed)
        {
            DeviceInfo->DeviceType = Usb11Device;
        }
        else if (DeviceHandle->DeviceSpeed == UsbHighSpeed)
        {
            DeviceInfo->DeviceType = Usb20Device;
        }
    }

    DeviceInfo->CurrentConfigurationValue = 0;

    if (!ConfigHandle)
    {
        *LenDataReturned = ActualLength;
        return STATUS_SUCCESS;
    }

    DeviceInfo->CurrentConfigurationValue = 
        ConfigHandle->ConfigurationDescriptor->bConfigurationValue;

    InterfaceEntry = NULL;

    if (!IsListEmpty(&ConfigHandle->InterfaceHandleList))
    {
        InterfaceEntry = ConfigHandle->InterfaceHandleList.Flink;
    }

    jx = 0;

    while (InterfaceEntry && InterfaceEntry != &ConfigHandle->InterfaceHandleList)
    {
        ix = 0;

        InterfaceHandle = CONTAINING_RECORD(InterfaceEntry,
                                            USBPORT_INTERFACE_HANDLE,
                                            InterfaceLink);

        if (InterfaceHandle->InterfaceDescriptor.bNumEndpoints > 0)
        {
            PipeInfo = &DeviceInfo->PipeList[0];
            PipeHandle = &InterfaceHandle->PipeHandle[0];

            do
            {
                if (PipeHandle->Flags & PIPE_HANDLE_FLAG_NULL_PACKET_SIZE)
                {
                    PipeInfo->ScheduleOffset = 1;
                }
                else
                {
                    PipeInfo->ScheduleOffset = 
                        PipeHandle->Endpoint->EndpointProperties.ScheduleOffset;
                }

                RtlCopyMemory(&PipeInfo->EndpointDescriptor,
                              &PipeHandle->EndpointDescriptor,
                              sizeof(USB_ENDPOINT_DESCRIPTOR));

                ++ix;
                ++jx;

                PipeInfo += 1;
                PipeHandle += 1;
            }
            while (ix < InterfaceHandle->InterfaceDescriptor.bNumEndpoints);
        }

        InterfaceEntry = InterfaceEntry->Flink;
    }

    *LenDataReturned = ActualLength;

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_GetControllerInformation(IN PVOID BusContext,
                               OUT PVOID ControllerInfoBuffer,
                               IN ULONG ControllerInfoBufferLen,
                               OUT PULONG LenDataReturned)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSB_CONTROLLER_INFORMATION_0 InfoBuffer;
    NTSTATUS Status;

    DPRINT("USBHI_GetControllerInformation: ControllerInfoBufferLen - %x\n",
           ControllerInfoBufferLen);

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    InfoBuffer = (PUSB_CONTROLLER_INFORMATION_0)ControllerInfoBuffer;

    *LenDataReturned = 0;

    if (ControllerInfoBufferLen < (2 * sizeof(ULONG)))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
        return Status;
    }

    *LenDataReturned = 8;

    if (InfoBuffer->InformationLevel > 0)
    {
        Status = STATUS_NOT_SUPPORTED;
        return Status;
    }

    InfoBuffer->ActualLength = sizeof(USB_CONTROLLER_INFORMATION_0);

    if (ControllerInfoBufferLen >= sizeof(USB_CONTROLLER_INFORMATION_0))
    {
        InfoBuffer->SelectiveSuspendEnabled = 
            (FdoExtension->Flags & USBPORT_FLAG_SELECTIVE_SUSPEND) == 
            USBPORT_FLAG_SELECTIVE_SUSPEND;
    }

    *LenDataReturned = sizeof(USB_CONTROLLER_INFORMATION_0);

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_ControllerSelectiveSuspend(IN PVOID BusContext,
                                 IN BOOLEAN Enable)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    ULONG Flags;
    ULONG HcDisable;
    NTSTATUS Status;

    DPRINT("USBHI_ControllerSelectiveSuspend: Enable - %x\n", Enable);

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    Flags = FdoExtension->Flags;

    if (Flags & USBPORT_FLAG_BIOS_DISABLE_SS)
    {
        return STATUS_SUCCESS;
    }

    if (Enable)
    {
        FdoExtension->Flags |= USBPORT_FLAG_SELECTIVE_SUSPEND;
        HcDisable = 0;
    }
    else
    {
        FdoExtension->Flags &= ~USBPORT_FLAG_SELECTIVE_SUSPEND;
        HcDisable = 1;
    }

    Status = USBPORT_SetRegistryKeyValue(FdoExtension->CommonExtension.LowerPdoDevice,
                                         (HANDLE)1,
                                         REG_DWORD,
                                         L"HcDisableSelectiveSuspend",
                                         &HcDisable,
                                         sizeof(HcDisable));

    if (NT_SUCCESS(Status))
    {
        if (Enable) 
            FdoExtension->Flags |= USBPORT_FLAG_SELECTIVE_SUSPEND;
        else
            FdoExtension->Flags &= ~USBPORT_FLAG_SELECTIVE_SUSPEND;
    }

    return Status;
}

NTSTATUS
USB_BUSIFFN
USBHI_GetExtendedHubInformation(IN PVOID BusContext,
                                IN PDEVICE_OBJECT HubPhysicalDeviceObject,
                                IN OUT PVOID HubInformationBuffer,
                                IN ULONG HubInfoLen,
                                IN OUT PULONG LenDataReturned)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;
    ULONG NumPorts;
    ULONG ix;
    PUSB_EXTHUB_INFORMATION_0 HubInfoBuffer;
    ULONG PortStatus = 0;
    ULONG PortAttrX;

    DPRINT("USBHI_GetExtendedHubInformation: ... \n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    HubInfoBuffer = (PUSB_EXTHUB_INFORMATION_0)HubInformationBuffer;

    if (HubPhysicalDeviceObject != PdoDevice)
    {
        *LenDataReturned = 0;
        return STATUS_NOT_SUPPORTED;
    }

    if (HubInfoLen < sizeof(USB_EXTHUB_INFORMATION_0))
    {
        *LenDataReturned = 0;
        return STATUS_BUFFER_TOO_SMALL;
    }

    NumPorts = PdoExtension->RootHubDescriptors->Descriptor.bNumberOfPorts;
    HubInfoBuffer->NumberOfPorts = NumPorts;

    if (NumPorts == 0)
    {
        *LenDataReturned = sizeof(USB_EXTHUB_INFORMATION_0);
        return STATUS_SUCCESS;
    }

    for (ix = 1; ix <= HubInfoBuffer->NumberOfPorts; ++ix)
    {
        HubInfoBuffer->Port[ix].PhysicalPortNumber = ix;
        HubInfoBuffer->Port[ix].PortLabelNumber = ix;
        HubInfoBuffer->Port[ix].VidOverride = 0;
        HubInfoBuffer->Port[ix].PidOverride = 0;
        HubInfoBuffer->Port[ix].PortAttributes = 0;

        if (Packet->MiniPortFlags & USB_MINIPORT_FLAGS_USB2)
        {
            HubInfoBuffer->Port[ix].PortAttributes = USB_PORTATTR_SHARED_USB2;

            Packet->RH_GetPortStatus(FdoExtension->MiniPortExt,
                                     ix,
                                     &PortStatus);

            if (PortStatus & 0x8000)
            {
                HubInfoBuffer->Port[ix].PortAttributes |= USB_PORTATTR_OWNED_BY_CC;
            }
        }
        else
        {
            if (!(FdoExtension->Flags & USBPORT_FLAG_COMPANION_HC))
            {
                continue;
            }

            if (USBPORT_FindUSB2Controller(FdoDevice))
            {
                HubInfoBuffer->Port[ix].PortAttributes |= USB_PORTATTR_NO_OVERCURRENT_UI;
            }
        }
    }

    for (ix = 1; ix <= HubInfoBuffer->NumberOfPorts; ++ix)
    {
        PortAttrX = 0;

        USBPORT_GetRegistryKeyValueFullInfo(FdoDevice,
                                            FdoExtension->CommonExtension.LowerPdoDevice,
                                            0,
                                            L"PortAttrX",
                                            128,
                                            &PortAttrX,
                                            sizeof(PortAttrX));

        HubInfoBuffer->Port[ix].PortAttributes |= PortAttrX;
    }

    *LenDataReturned = sizeof(USB_EXTHUB_INFORMATION_0);

    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBHI_GetRootHubSymbolicName(IN PVOID BusContext,
                             IN OUT PVOID HubInfoBuffer,
                             IN ULONG HubInfoBufferLen,
                             OUT PULONG HubNameActualLen)
{
    PDEVICE_OBJECT PdoDevice;
    UNICODE_STRING HubName;
    PUNICODE_STRING InfoBuffer;
    NTSTATUS Status;

    DPRINT("USBHI_GetRootHubSymbolicName: ... \n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;

    Status = USBPORT_GetSymbolicName(PdoDevice, &HubName);

    if (HubInfoBufferLen < HubName.Length)
    {
        InfoBuffer = (PUNICODE_STRING)HubInfoBuffer;
        InfoBuffer->Length = 0;
    }
    else
    {
        RtlCopyMemory(HubInfoBuffer, HubName.Buffer, HubName.Length);
    }

    *HubNameActualLen = HubName.Length;

    RtlFreeUnicodeString(&HubName);

    return Status;
}

PVOID
USB_BUSIFFN
USBHI_GetDeviceBusContext(IN PVOID BusContext,
                          IN PVOID DeviceHandle)
{
    DPRINT1("USBHI_GetDeviceBusContext: UNIMPLEMENTED. FIXME. \n");
    return NULL;
}

NTSTATUS
USB_BUSIFFN
USBHI_Initialize20Hub(IN PVOID BusContext,
                      IN PUSB_DEVICE_HANDLE UsbdHubDeviceHandle,
                      IN ULONG TtCount)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;

    DPRINT("USBHI_Initialize20Hub: UsbdHubDeviceHandle - %p, TtCount - %x\n",
           UsbdHubDeviceHandle,
           TtCount);

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;

    return USBPORT_Initialize20Hub(PdoExtension->FdoDevice,
                                   (PUSBPORT_DEVICE_HANDLE)UsbdHubDeviceHandle,
                                   TtCount);
}

NTSTATUS
USB_BUSIFFN
USBHI_RootHubInitNotification(IN PVOID BusContext,
                              IN PVOID CallbackContext,
                              IN PRH_INIT_CALLBACK CallbackFunction)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    KIRQL OldIrql;

    DPRINT("USBHI_RootHubInitNotification \n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    KeAcquireSpinLock(&FdoExtension->RootHubCallbackSpinLock, &OldIrql);
    PdoExtension->RootHubInitContext = CallbackContext;
    PdoExtension->RootHubInitCallback = CallbackFunction;
    KeReleaseSpinLock(&FdoExtension->RootHubCallbackSpinLock, OldIrql);

    return STATUS_SUCCESS;
}

VOID
USB_BUSIFFN
USBHI_FlushTransfers(IN PVOID BusContext,
                     OUT PUSB_DEVICE_HANDLE UsbdDeviceHandle)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;

    DPRINT("USBHI_FlushTransfers: ... \n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;

    USBPORT_BadRequestFlush(PdoExtension->FdoDevice);
}

VOID
USB_BUSIFFN
USBHI_SetDeviceHandleData(IN PVOID BusContext,
                          IN PVOID DeviceHandle,
                          IN PDEVICE_OBJECT UsbDevicePdo)
{
    DPRINT1("USBHI_SetDeviceHandleData: UNIMPLEMENTED. FIXME. \n");
}

/* USB bus driver Interface functions */

VOID
USB_BUSIFFN
USBDI_GetUSBDIVersion(IN PVOID BusContext,
                      OUT PUSBD_VERSION_INFORMATION VersionInfo,
                      OUT PULONG HcdCapabilities)
{
    DPRINT1("USBDI_GetUSBDIVersion: UNIMPLEMENTED. FIXME. \n");
}

NTSTATUS
USB_BUSIFFN
USBDI_QueryBusTime(IN PVOID BusContext,
                   OUT PULONG CurrentFrame)
{
    DPRINT1("USBDI_QueryBusTime: UNIMPLEMENTED. FIXME. \n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBDI_SubmitIsoOutUrb(IN PVOID BusContext,
                      IN PURB Urb)
{
    DPRINT1("USBDI_SubmitIsoOutUrb: UNIMPLEMENTED. FIXME. \n");
    return STATUS_SUCCESS;
}

NTSTATUS
USB_BUSIFFN
USBDI_QueryBusInformation(IN PVOID BusContext,
                          IN ULONG Level,
                          OUT PVOID BusInfoBuffer,
                          OUT PULONG BusInfoBufferLen,
                          OUT PULONG BusInfoActualLen)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    SIZE_T Length;
    //PUSB_BUS_INFORMATION_LEVEL_0 Buffer0;
    PUSB_BUS_INFORMATION_LEVEL_1 Buffer1;

    DPRINT("USBDI_QueryBusInformation: Level - %p\n", Level);

    if ((Level != 0) || (Level != 1))
    {
        DPRINT1("USBDI_QueryBusInformation: Level should be 0 or 1\n");
        return STATUS_NOT_SUPPORTED;
    }

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;

    if (Level == 0)
    {
        if (BusInfoActualLen)
            *BusInfoActualLen = sizeof(USB_BUS_INFORMATION_LEVEL_0);
    
        if (*BusInfoBufferLen < sizeof(USB_BUS_INFORMATION_LEVEL_0))
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
    
        *BusInfoBufferLen = sizeof(USB_BUS_INFORMATION_LEVEL_0);
    
        //Buffer0 = (PUSB_BUS_INFORMATION_LEVEL_0)BusInfoBuffer;
        DPRINT1("USBDI_QueryBusInformation: UNIMPLEMENTED. FIXME. \n");
        //Buffer0->TotalBandwidth = USBPORT_GetTotalBandwidth();
        //Buffer0->ConsumedBandwidth = USBPORT_GetAllocatedBandwidth();
    
        return STATUS_SUCCESS;
    }

    if (Level == 1)
    {
        Length = sizeof(USB_BUS_INFORMATION_LEVEL_1) + 
                 FdoExtension->CommonExtension.SymbolicLinkName.Length;
    
        if (BusInfoActualLen)
            *BusInfoActualLen = Length;
    
        if (*BusInfoBufferLen < Length)
        {
            return STATUS_BUFFER_TOO_SMALL;
        }
    
        *BusInfoBufferLen = Length;
    
        Buffer1 = (PUSB_BUS_INFORMATION_LEVEL_1)BusInfoBuffer;
        DPRINT1("USBDI_QueryBusInformation: UNIMPLEMENTED. FIXME. \n");
        //Buffer1->TotalBandwidth = USBPORT_GetTotalBandwidth();
        //Buffer1->ConsumedBandwidth = USBPORT_GetAllocatedBandwidth();
        Buffer1->ControllerNameLength = FdoExtension->CommonExtension.SymbolicLinkName.Length;
    
        RtlCopyMemory(&Buffer1->ControllerNameUnicodeString,
                      FdoExtension->CommonExtension.SymbolicLinkName.Buffer,
                      FdoExtension->CommonExtension.SymbolicLinkName.Length);
    
        return STATUS_SUCCESS;
    }

    return STATUS_SUCCESS;
}

BOOLEAN
USB_BUSIFFN
USBDI_IsDeviceHighSpeed(IN PVOID BusContext)
{
    PDEVICE_OBJECT PdoDevice;
    PUSBPORT_RHDEVICE_EXTENSION PdoExtension;
    PDEVICE_OBJECT FdoDevice;
    PUSBPORT_DEVICE_EXTENSION FdoExtension;
    PUSBPORT_REGISTRATION_PACKET Packet;

    DPRINT("USBDI_IsDeviceHighSpeed: ... \n");

    PdoDevice = (PDEVICE_OBJECT)BusContext;
    PdoExtension = (PUSBPORT_RHDEVICE_EXTENSION)PdoDevice->DeviceExtension;
    FdoDevice = PdoExtension->FdoDevice;
    FdoExtension = (PUSBPORT_DEVICE_EXTENSION)FdoDevice->DeviceExtension;
    Packet = &FdoExtension->MiniPortInterface->Packet;

    return Packet->MiniPortFlags & USB_MINIPORT_FLAGS_USB2;
}

NTSTATUS
USB_BUSIFFN
USBDI_EnumLogEntry(IN PVOID BusContext,
                   IN ULONG DriverTag,
                   IN ULONG EnumTag,
                   IN ULONG P1,
                   IN ULONG P2)
{
    DPRINT1("USBDI_EnumLogEntry: UNIMPLEMENTED. FIXME. \n");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
USBPORT_PdoQueryInterface(IN PDEVICE_OBJECT FdoDevice,
                          IN PDEVICE_OBJECT PdoDevice,
                          IN PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    PUSB_BUS_INTERFACE_HUB_V5 InterfaceHub;
    PUSB_BUS_INTERFACE_USBDI_V2 InterfaceDI;
    UNICODE_STRING GuidBuffer;
    NTSTATUS Status;

    DPRINT("USBPORT_PdoQueryInterface: ... \n");

    if (IsEqualGUIDAligned((REFGUID)(IoStack->Parameters.QueryInterface.InterfaceType),
                           &USB_BUS_INTERFACE_HUB_GUID))
    {
        // Get request parameters
        InterfaceHub = (PUSB_BUS_INTERFACE_HUB_V5)IoStack->Parameters.QueryInterface.Interface;
        InterfaceHub->Version = IoStack->Parameters.QueryInterface.Version;

        // Check version
        if (IoStack->Parameters.QueryInterface.Version >= 6)
        {
            DPRINT1("USB_BUS_INTERFACE_HUB_GUID version %x not supported!\n",
                    IoStack->Parameters.QueryInterface.Version);

            return STATUS_NOT_SUPPORTED; // Version not supported
        }

        // Interface version 0
        InterfaceHub->Size = IoStack->Parameters.QueryInterface.Size;
        InterfaceHub->BusContext = (PVOID)PdoDevice;

        InterfaceHub->InterfaceReference = USBI_InterfaceReference;
        InterfaceHub->InterfaceDereference = USBI_InterfaceDereference;

        // Interface version 1
        if (IoStack->Parameters.QueryInterface.Version >= 1)
        {
            InterfaceHub->CreateUsbDevice = USBHI_CreateUsbDevice;
            InterfaceHub->InitializeUsbDevice = USBHI_InitializeUsbDevice;
            InterfaceHub->GetUsbDescriptors = USBHI_GetUsbDescriptors;
            InterfaceHub->RemoveUsbDevice = USBHI_RemoveUsbDevice;
            InterfaceHub->RestoreUsbDevice = USBHI_RestoreUsbDevice;
            InterfaceHub->QueryDeviceInformation = USBHI_QueryDeviceInformation;
        }

        // Interface version 2
        if (IoStack->Parameters.QueryInterface.Version >= 2)
        {
            InterfaceHub->GetControllerInformation = USBHI_GetControllerInformation;
            InterfaceHub->ControllerSelectiveSuspend = USBHI_ControllerSelectiveSuspend;
            InterfaceHub->GetExtendedHubInformation = USBHI_GetExtendedHubInformation;
            InterfaceHub->GetRootHubSymbolicName = USBHI_GetRootHubSymbolicName;
            InterfaceHub->GetDeviceBusContext = USBHI_GetDeviceBusContext;
            InterfaceHub->Initialize20Hub = USBHI_Initialize20Hub;
        }

        // Interface version 3
        if (IoStack->Parameters.QueryInterface.Version >= 3)
            InterfaceHub->RootHubInitNotification = USBHI_RootHubInitNotification;

        // Interface version 4
        if (IoStack->Parameters.QueryInterface.Version >= 4)
            InterfaceHub->FlushTransfers = USBHI_FlushTransfers;

        // Interface version 5
        if (IoStack->Parameters.QueryInterface.Version >= 5)
            InterfaceHub->SetDeviceHandleData = USBHI_SetDeviceHandleData;

        // Request completed
        return STATUS_SUCCESS;
    }
    else if (IsEqualGUIDAligned(IoStack->Parameters.QueryInterface.InterfaceType,
                                &USB_BUS_INTERFACE_USBDI_GUID))
    {
        // Get request parameters
        InterfaceDI = (PUSB_BUS_INTERFACE_USBDI_V2)IoStack->Parameters.QueryInterface.Interface;
        InterfaceDI->Version = IoStack->Parameters.QueryInterface.Version;

        // Check version
        if (IoStack->Parameters.QueryInterface.Version >= 3)
        {
            DPRINT1("USB_BUS_INTERFACE_USBDI_GUID version %x not supported!\n",
                    IoStack->Parameters.QueryInterface.Version);

            return STATUS_NOT_SUPPORTED; // Version not supported
        }

        // Interface version 0
        InterfaceDI->Size = IoStack->Parameters.QueryInterface.Size;
        InterfaceDI->BusContext = (PVOID)PdoDevice;
        InterfaceDI->InterfaceReference = USBI_InterfaceReference;
        InterfaceDI->InterfaceDereference = USBI_InterfaceDereference;
        InterfaceDI->GetUSBDIVersion = USBDI_GetUSBDIVersion;
        InterfaceDI->QueryBusTime = USBDI_QueryBusTime;
        InterfaceDI->SubmitIsoOutUrb = USBDI_SubmitIsoOutUrb;
        InterfaceDI->QueryBusInformation = USBDI_QueryBusInformation;

        // Interface version 1
        if (IoStack->Parameters.QueryInterface.Version >= 1)
            InterfaceDI->IsDeviceHighSpeed = USBDI_IsDeviceHighSpeed;

        // Interface version 2
        if (IoStack->Parameters.QueryInterface.Version >= 2)
            InterfaceDI->EnumLogEntry = USBDI_EnumLogEntry;

        return STATUS_SUCCESS;
    }
    else
    {
        // Convert GUID to string
        Status = RtlStringFromGUID(IoStack->Parameters.QueryInterface.InterfaceType,
                                   &GuidBuffer);

        if (NT_SUCCESS(Status))
        {
            // Print interface
            DPRINT1("HandleQueryInterface UNKNOWN INTERFACE GUID: %wZ Version %x\n",
                    &GuidBuffer,
                    IoStack->Parameters.QueryInterface.Version);
            
            RtlFreeUnicodeString(&GuidBuffer); // Free GUID buffer
        }
    }

    return STATUS_NOT_SUPPORTED;
}
