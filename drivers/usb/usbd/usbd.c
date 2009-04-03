/*
 * PROJECT:     ReactOS Universal Serial Bus Driver/Helper Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbd/usbd.c
 * PURPOSE:     Helper Library for USB
 * PROGRAMMERS:
 *              Filip Navara <xnavara@volny.cz>
 *
 */

/*
 * Universal Serial Bus Driver/Helper Library
 *
 * Written by Filip Navara <xnavara@volny.cz>
 *
 * Notes:
 *    This driver was obsoleted in Windows XP and most functions
 *    became pure stubs. But some of them were retained for backward
 *    compatibilty with existing drivers.
 *
 *    Preserved functions:
 *
 *    USBD_Debug_GetHeap (implemented)
 *    USBD_Debug_RetHeap (implemented)
 *    USBD_CalculateUsbBandwidth (implemented, tested)
 *    USBD_CreateConfigurationRequestEx (implemented)
 *    USBD_CreateConfigurationRequest
 *    USBD_GetInterfaceLength (implemented)
 *    USBD_ParseConfigurationDescriptorEx (implemented)
 *    USBD_ParseDescriptors (implemented)
 *    USBD_GetPdoRegistryParameters (implemented)
 */

#include <ntddk.h>
#include <usbdi.h>
#ifndef PLUGPLAY_REGKEY_DRIVER
#define PLUGPLAY_REGKEY_DRIVER              2
#endif
typedef struct _USBD_INTERFACE_LIST_ENTRY {
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    PUSBD_INTERFACE_INFORMATION Interface;
} USBD_INTERFACE_LIST_ENTRY, *PUSBD_INTERFACE_LIST_ENTRY;

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
DWORD NTAPI
DllInitialize(DWORD Unknown)
{
    return 0;
}

/*
 * @implemented
 */
DWORD NTAPI
DllUnload(VOID)
{
    return 0;
}

/*
 * @implemented
 */
PVOID NTAPI
USBD_Debug_GetHeap(DWORD Unknown1, POOL_TYPE PoolType, ULONG NumberOfBytes,
	ULONG Tag)
{
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */
VOID NTAPI
USBD_Debug_RetHeap(PVOID Heap, DWORD Unknown2, DWORD Unknown3)
{
    ExFreePool(Heap);
}

/*
 * @implemented
 */
VOID NTAPI
USBD_Debug_LogEntry(PCHAR Name, ULONG_PTR Info1, ULONG_PTR Info2,
    ULONG_PTR Info3)
{
}

/*
 * @implemented
 */
PVOID NTAPI
USBD_AllocateDeviceName(DWORD Unknown)
{
    return NULL;
}

/*
 * @implemented
 */
DWORD NTAPI
USBD_CalculateUsbBandwidth(
    ULONG MaxPacketSize,
    UCHAR EndpointType,
    BOOLEAN LowSpeed
    )
{
    DWORD OverheadTable[] = {
            0x00, /* UsbdPipeTypeControl */
            0x09, /* UsbdPipeTypeIsochronous */
            0x00, /* UsbdPipeTypeBulk */
            0x0d  /* UsbdPipeTypeInterrupt */
        };
    DWORD Result;

    if (OverheadTable[EndpointType] != 0)
    {
        Result = ((MaxPacketSize + OverheadTable[EndpointType]) * 8 * 7) / 6;
        if (LowSpeed)
           return Result << 3;
        return Result;
    }
    return 0;
}

/*
 * @implemented
 */
DWORD NTAPI
USBD_Dispatch(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
    return 1;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_FreeDeviceMutex(PVOID Unknown)
{
}

/*
 * @implemented
 */
VOID NTAPI
USBD_FreeDeviceName(PVOID Unknown)
{
}

/*
 * @implemented
 */
VOID NTAPI
USBD_WaitDeviceMutex(PVOID Unknown)
{
}

/*
 * @implemented
 */
DWORD NTAPI
USBD_GetSuspendPowerState(DWORD Unknown1)
{
    return 0;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_InitializeDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3,
    DWORD Unknown4, DWORD Unknown5, DWORD Unknown6)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_RegisterHostController(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3,
    DWORD Unknown4, DWORD Unknown5, DWORD Unknown6, DWORD Unknown7,
    DWORD Unknown8, DWORD Unknown9, DWORD Unknown10)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_GetDeviceInformation(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_CreateDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3,
    DWORD Unknown4, DWORD Unknown5)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_RemoveDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_CompleteRequest(DWORD Unknown1, DWORD Unknown2)
{
}

/*
 * @implemented
 */
VOID NTAPI
USBD_RegisterHcFilter(
    PDEVICE_OBJECT DeviceObject,
    PDEVICE_OBJECT FilterDeviceObject
    )
{
}

/*
 * @implemented
 */
VOID NTAPI
USBD_SetSuspendPowerState(DWORD Unknown1, DWORD Unknown2)
{
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_MakePdoName(DWORD Unknown1, DWORD Unknown2)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_QueryBusTime(
    PDEVICE_OBJECT RootHubPdo,
    PULONG CurrentFrame
    )
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_GetUSBDIVersion(
    PUSBD_VERSION_INFORMATION Version
    )
{
    if (Version != NULL)
    {
        Version->USBDI_Version = USBDI_VERSION;
        Version->Supported_USB_Version = 0x100;
    }
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_RestoreDevice(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_RegisterHcDeviceCapabilities(DWORD Unknown1, DWORD Unknown2,
    DWORD Unknown3)
{
}

/*
 * @implemented
 * FIXME: Test
 */
PURB
NTAPI
USBD_CreateConfigurationRequestEx(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList
    )
{
    PURB Urb;
    DWORD UrbSize;
    DWORD InterfaceCount;

    for (InterfaceCount = 0;
         InterfaceList[InterfaceCount].InterfaceDescriptor != NULL;
         ++InterfaceCount)
       ;
    /* Include the NULL entry */
    ++InterfaceCount;

    UrbSize = sizeof(Urb->UrbSelectConfiguration) +
       (InterfaceCount * sizeof(PUSBD_INTERFACE_LIST_ENTRY));
    Urb = ExAllocatePool(NonPagedPool, UrbSize);
    Urb->UrbSelectConfiguration.Hdr.Function =
        URB_FUNCTION_SELECT_CONFIGURATION;
    Urb->UrbSelectConfiguration.Hdr.Length =
        sizeof(Urb->UrbSelectConfiguration);
    Urb->UrbSelectConfiguration.ConfigurationDescriptor =
       ConfigurationDescriptor;
    memcpy((PVOID)&Urb->UrbSelectConfiguration.Interface, (PVOID)InterfaceList,
       InterfaceCount * sizeof(PUSBD_INTERFACE_LIST_ENTRY));

    return Urb;
}

/*
 * @unimplemented
 */
PURB NTAPI
USBD_CreateConfigurationRequest(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PUSHORT Size
    )
{
    return NULL;
}

/*
 * @unimplemented
 */
ULONG NTAPI
USBD_GetInterfaceLength(
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    PUCHAR BufferEnd
    )
{
    ULONG_PTR Current;
    PUSB_INTERFACE_DESCRIPTOR CurrentDescriptor = InterfaceDescriptor;
    DWORD Length = CurrentDescriptor->bLength;

    // USB_ENDPOINT_DESCRIPTOR_TYPE
    if (CurrentDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
    {
        for (Current = (ULONG_PTR)CurrentDescriptor;
             Current < (ULONG_PTR)BufferEnd;
             Current += CurrentDescriptor->bLength)
            CurrentDescriptor = (PUSB_INTERFACE_DESCRIPTOR)Current;
            Length += CurrentDescriptor->bLength;

    }
    return Length;
}

/*
 * @implemented
 */
PUSB_COMMON_DESCRIPTOR NTAPI
USBD_ParseDescriptors(
    PVOID  DescriptorBuffer,
    ULONG  TotalLength,
    PVOID  StartPosition,
    LONG  DescriptorType
    )
{
    PUSB_COMMON_DESCRIPTOR PComDes = StartPosition;

    while(PComDes)
    {
       if (PComDes >= (PUSB_COMMON_DESCRIPTOR)
                            ((PLONG)DescriptorBuffer + TotalLength) ) break;
       if (PComDes->bDescriptorType == DescriptorType) return PComDes;
       if (PComDes->bLength == 0) break;
       PComDes = PComDes + PComDes->bLength;
    }
    return NULL;
}


/*
 * @implemented
 */
PUSB_INTERFACE_DESCRIPTOR NTAPI
USBD_ParseConfigurationDescriptorEx(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PVOID StartPosition,
    LONG InterfaceNumber,
    LONG AlternateSetting,
    LONG InterfaceClass,
    LONG InterfaceSubClass,
    LONG InterfaceProtocol
    )
{
    int x = 0;
    PUSB_INTERFACE_DESCRIPTOR UsbInterfaceDesc = StartPosition;

    while(UsbInterfaceDesc)
    {
       UsbInterfaceDesc = (PUSB_INTERFACE_DESCRIPTOR)
                           USBD_ParseDescriptors(ConfigurationDescriptor,
                                        ConfigurationDescriptor->wTotalLength,
                                        UsbInterfaceDesc,
                                        USB_INTERFACE_DESCRIPTOR_TYPE);

       if (!UsbInterfaceDesc) break;

       if(InterfaceNumber != -1)
       {
          if(InterfaceNumber != UsbInterfaceDesc->bInterfaceNumber) x = 1;
       }
       if(AlternateSetting != -1)
       {
          if(AlternateSetting != UsbInterfaceDesc->bAlternateSetting) x = 1;
       }
       if(InterfaceClass != -1)
       {
          if(InterfaceClass != UsbInterfaceDesc->bInterfaceClass) x = 1;
       }
       if(InterfaceSubClass != -1)
       {
          if(InterfaceSubClass != UsbInterfaceDesc->bInterfaceSubClass) x = 1;
       }
       if(InterfaceProtocol != -1)
       {
          if(InterfaceProtocol != UsbInterfaceDesc->bInterfaceProtocol) x = 1;
       }

       if (!x) return UsbInterfaceDesc;

       if (UsbInterfaceDesc->bLength == 0) break;
       UsbInterfaceDesc = UsbInterfaceDesc + UsbInterfaceDesc->bLength;
    }
    return NULL;
}

/*
 * @implemented
 */
PUSB_INTERFACE_DESCRIPTOR NTAPI
USBD_ParseConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    UCHAR InterfaceNumber,
    UCHAR AlternateSetting
    )
{
    return USBD_ParseConfigurationDescriptorEx(ConfigurationDescriptor,
        (PVOID)ConfigurationDescriptor, InterfaceNumber, AlternateSetting,
        -1, -1, -1);
}


/*
 * @implemented
 */
DWORD NTAPI
USBD_GetPdoRegistryParameter(
    PDEVICE_OBJECT PhysicalDeviceObject,
    PVOID Parameter,
    ULONG ParameterLength,
    PWCHAR KeyName,
    ULONG KeyNameLength
    )
{
    NTSTATUS Status;
    HANDLE DevInstRegKey;

    Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
        PLUGPLAY_REGKEY_DRIVER, STANDARD_RIGHTS_ALL, &DevInstRegKey);
    if (NT_SUCCESS(Status))
    {
        PKEY_VALUE_FULL_INFORMATION FullInfo;
        UNICODE_STRING ValueName;
        ULONG Length;

        RtlInitUnicodeString(&ValueName, KeyName);
        Length = ParameterLength + KeyNameLength + sizeof(KEY_VALUE_FULL_INFORMATION);
        FullInfo = ExAllocatePool(PagedPool, Length);
        if (FullInfo)
        {
            Status = ZwQueryValueKey(DevInstRegKey, &ValueName,
                KeyValueFullInformation, FullInfo, Length, &Length);
            if (NT_SUCCESS(Status))
            {
                RtlCopyMemory(Parameter,
                    ((PUCHAR)FullInfo) + FullInfo->DataOffset,
                    ParameterLength /*FullInfo->DataLength*/);
            }
            ExFreePool(FullInfo);
        } else
            Status = STATUS_NO_MEMORY;
        ZwClose(DevInstRegKey);
    }
    return Status;
}
