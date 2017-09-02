/*
 * PROJECT:     ReactOS Universal Serial Bus Driver/Helper Library
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        drivers/usb/usbd/usbd.c
 * PURPOSE:     Helper Library for USB
 * PROGRAMMERS:
 *              Filip Navara <xnavara@volny.cz>
 *              Michael Martin <michael.martin@reactos.org>
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
 *    compatibility with existing drivers.
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

#define _USBD_
#define NDEBUG
#include <ntddk.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <debug.h>
#ifndef PLUGPLAY_REGKEY_DRIVER
#define PLUGPLAY_REGKEY_DRIVER              2
#endif

NTSTATUS NTAPI
DriverEntry(PDRIVER_OBJECT DriverObject,
            PUNICODE_STRING RegistryPath)
{
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
ULONG NTAPI
DllInitialize(ULONG Unknown)
{
    return 0;
}

/*
 * @implemented
 */
ULONG NTAPI
DllUnload(VOID)
{
    return 0;
}

/*
 * @implemented
 */
PVOID NTAPI
USBD_Debug_GetHeap(ULONG Unknown1, POOL_TYPE PoolType, ULONG NumberOfBytes,
                   ULONG Tag)
{
    return ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
}

/*
 * @implemented
 */
VOID NTAPI
USBD_Debug_RetHeap(PVOID Heap, ULONG Unknown2, ULONG Unknown3)
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
USBD_AllocateDeviceName(ULONG Unknown)
{
    UNIMPLEMENTED;
    return NULL;
}

/*
 * @implemented
 */
ULONG NTAPI
USBD_CalculateUsbBandwidth(
    ULONG MaxPacketSize,
    UCHAR EndpointType,
    BOOLEAN LowSpeed
    )
{
    ULONG OverheadTable[] = {
            0x00, /* UsbdPipeTypeControl */
            0x09, /* UsbdPipeTypeIsochronous */
            0x00, /* UsbdPipeTypeBulk */
            0x0d  /* UsbdPipeTypeInterrupt */
        };
    ULONG Result;

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
ULONG NTAPI
USBD_Dispatch(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3, ULONG Unknown4)
{
    UNIMPLEMENTED;
    return 1;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_FreeDeviceMutex(PVOID Unknown)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_FreeDeviceName(PVOID Unknown)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_WaitDeviceMutex(PVOID Unknown)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
ULONG NTAPI
USBD_GetSuspendPowerState(ULONG Unknown1)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_InitializeDevice(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3,
    ULONG Unknown4, ULONG Unknown5, ULONG Unknown6)
{
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_RegisterHostController(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3,
    ULONG Unknown4, ULONG Unknown5, ULONG Unknown6, ULONG Unknown7,
    ULONG Unknown8, ULONG Unknown9, ULONG Unknown10)
{
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_GetDeviceInformation(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_CreateDevice(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3,
    ULONG Unknown4, ULONG Unknown5)
{
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_RemoveDevice(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_CompleteRequest(ULONG Unknown1, ULONG Unknown2)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_SetSuspendPowerState(ULONG Unknown1, ULONG Unknown2)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_MakePdoName(ULONG Unknown1, ULONG Unknown2)
{
    UNIMPLEMENTED;
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
    UNIMPLEMENTED;
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
        Version->Supported_USB_Version = 0x200;
    }
}

/*
 * @implemented
 */
NTSTATUS NTAPI
USBD_RestoreDevice(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3)
{
    UNIMPLEMENTED;
    return STATUS_NOT_SUPPORTED;
}

/*
 * @implemented
 */
VOID NTAPI
USBD_RegisterHcDeviceCapabilities(ULONG Unknown1, ULONG Unknown2,
    ULONG Unknown3)
{
    UNIMPLEMENTED;
}

/*
 * @implemented
 */
PURB NTAPI
USBD_CreateConfigurationRequestEx(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PUSBD_INTERFACE_LIST_ENTRY InterfaceList
    )
{
    PURB Urb;
    ULONG UrbSize = 0;
    ULONG InterfaceCount = 0, PipeCount = 0;
    ULONG InterfaceNumber, EndPointNumber;
    PUSBD_INTERFACE_INFORMATION InterfaceInfo;

    while(InterfaceList[InterfaceCount].InterfaceDescriptor)
    {
        // pipe count
        PipeCount += InterfaceList[InterfaceCount].InterfaceDescriptor->bNumEndpoints;

        // interface count
        InterfaceCount++;
    }

    // size of urb
    UrbSize = GET_SELECT_CONFIGURATION_REQUEST_SIZE(InterfaceCount, PipeCount);

    // allocate urb
    Urb = ExAllocatePool(NonPagedPool, UrbSize);
    if (!Urb)
    {
        // no memory
        return NULL;
    }

    // zero urb
    RtlZeroMemory(Urb, UrbSize);

    // init urb header
    Urb->UrbSelectConfiguration.Hdr.Function =  URB_FUNCTION_SELECT_CONFIGURATION;
    Urb->UrbSelectConfiguration.Hdr.Length = UrbSize;
    Urb->UrbSelectConfiguration.ConfigurationDescriptor = ConfigurationDescriptor;

    // init interface information
    InterfaceInfo = &Urb->UrbSelectConfiguration.Interface;
    for (InterfaceNumber = 0; InterfaceNumber < InterfaceCount; InterfaceNumber++)
    {
        // init interface info
        InterfaceList[InterfaceNumber].Interface = InterfaceInfo;
        InterfaceInfo->InterfaceNumber = InterfaceList[InterfaceNumber].InterfaceDescriptor->bInterfaceNumber;
        InterfaceInfo->AlternateSetting = InterfaceList[InterfaceNumber].InterfaceDescriptor->bAlternateSetting;
        InterfaceInfo->NumberOfPipes = InterfaceList[InterfaceNumber].InterfaceDescriptor->bNumEndpoints;

        // store length
        InterfaceInfo->Length = GET_USBD_INTERFACE_SIZE(InterfaceList[InterfaceNumber].InterfaceDescriptor->bNumEndpoints);

        // sanity check
        //C_ASSERT(FIELD_OFFSET(USBD_INTERFACE_INFORMATION, Pipes) == 16);

        for (EndPointNumber = 0; EndPointNumber < InterfaceInfo->NumberOfPipes; EndPointNumber++)
        {
            // init max transfer size
            InterfaceInfo->Pipes[EndPointNumber].MaximumTransferSize = PAGE_SIZE;
        }

        // next interface info
        InterfaceInfo = (PUSBD_INTERFACE_INFORMATION) ((ULONG_PTR)InterfaceInfo + InterfaceInfo->Length);
    }

    return Urb;
}

/*
 * @implemented
 */
PURB NTAPI
USBD_CreateConfigurationRequest(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor,
    PUSHORT Size
    )
{
    /* WindowsXP returns NULL */
    return NULL;
}

/*
 * @implemented
 */
ULONG NTAPI
USBD_GetInterfaceLength(
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor,
    PUCHAR BufferEnd
    )
{
    ULONG_PTR Current;
    PUSB_INTERFACE_DESCRIPTOR CurrentDescriptor = InterfaceDescriptor;
    ULONG Length = 0;
    BOOLEAN InterfaceFound = FALSE;

    for (Current = (ULONG_PTR)CurrentDescriptor;
         Current < (ULONG_PTR)BufferEnd;
         Current += CurrentDescriptor->bLength)
    {
        CurrentDescriptor = (PUSB_INTERFACE_DESCRIPTOR)Current;

        if ((CurrentDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE) && (InterfaceFound))
            break;
        else if (CurrentDescriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
            InterfaceFound = TRUE;

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
    PUSB_COMMON_DESCRIPTOR CommonDescriptor;

    /* use start position */
    CommonDescriptor = (PUSB_COMMON_DESCRIPTOR)StartPosition;


    /* find next available descriptor */
    while(CommonDescriptor)
    {
       if ((ULONG_PTR)CommonDescriptor >= ((ULONG_PTR)DescriptorBuffer + TotalLength))
       {
           /* end reached */
           DPRINT("End reached %p\n", CommonDescriptor);
           return NULL;
       }

       DPRINT("CommonDescriptor Type %x Length %x\n", CommonDescriptor->bDescriptorType, CommonDescriptor->bLength);

       /* is the requested one */
       if (CommonDescriptor->bDescriptorType == DescriptorType)
       {
           /* it is */
           return CommonDescriptor;
       }

       if (CommonDescriptor->bLength == 0)
       {
           /* invalid usb descriptor */
           return NULL;
       }

       /* move to next descriptor */
       CommonDescriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG_PTR)CommonDescriptor + CommonDescriptor->bLength);
    }

    /* no descriptor found */
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
    BOOLEAN Found;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor;

    /* set to start position */
    InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)StartPosition;

    DPRINT("USBD_ParseConfigurationDescriptorEx\n");
    DPRINT("ConfigurationDescriptor %p Length %lu\n", ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength);
    DPRINT("CurrentOffset %p Offset %lu\n", StartPosition, ((ULONG_PTR)StartPosition - (ULONG_PTR)ConfigurationDescriptor));

    while(InterfaceDescriptor)
    {
       /* get interface descriptor */
       InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR) USBD_ParseDescriptors(ConfigurationDescriptor, ConfigurationDescriptor->wTotalLength, InterfaceDescriptor, USB_INTERFACE_DESCRIPTOR_TYPE);
       if (!InterfaceDescriptor)
       {
           /* no more descriptors available */
           break;
       }

       DPRINT("InterfaceDescriptor %p InterfaceNumber %x AlternateSetting %x Length %lu\n", InterfaceDescriptor, InterfaceDescriptor->bInterfaceNumber, InterfaceDescriptor->bAlternateSetting, InterfaceDescriptor->bLength);

       /* set found */
       Found = TRUE;

       /* is there an interface number provided */
       if(InterfaceNumber != -1)
       {
          if(InterfaceNumber != InterfaceDescriptor->bInterfaceNumber)
          {
              /* interface number does not match */
              Found = FALSE;
          }
       }

       /* is there an alternate setting provided */
       if(AlternateSetting != -1)
       {
          if(AlternateSetting != InterfaceDescriptor->bAlternateSetting)
          {
              /* alternate setting does not match */
              Found = FALSE;
          }
       }

       /* match on interface class */
       if(InterfaceClass != -1)
       {
          if(InterfaceClass != InterfaceDescriptor->bInterfaceClass)
          {
              /* no match with interface class criteria */
              Found = FALSE;
          }
       }

       /* match on interface sub class */
       if(InterfaceSubClass != -1)
       {
          if(InterfaceSubClass != InterfaceDescriptor->bInterfaceSubClass)
          {
              /* no interface sub class match */
              Found = FALSE;
          }
       }

       /* interface protocol criteria */
       if(InterfaceProtocol != -1)
       {
          if(InterfaceProtocol != InterfaceDescriptor->bInterfaceProtocol)
          {
              /* no interface protocol match */
              Found = FALSE;
          }
       }

       if (Found)
       {
           /* the chosen one */
           return InterfaceDescriptor;
       }

       /* sanity check */
       ASSERT(InterfaceDescriptor->bLength);

       /* move to next descriptor */
       InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)InterfaceDescriptor + InterfaceDescriptor->bLength);
    }

    DPRINT("No Descriptor With InterfaceNumber %ld AlternateSetting %ld InterfaceClass %ld InterfaceSubClass %ld InterfaceProtocol %ld found\n", InterfaceNumber,
            AlternateSetting, InterfaceClass, InterfaceSubClass, InterfaceProtocol);

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
ULONG NTAPI
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

    /* Open the device key */
    Status = IoOpenDeviceRegistryKey(PhysicalDeviceObject,
        PLUGPLAY_REGKEY_DEVICE, STANDARD_RIGHTS_ALL, &DevInstRegKey);
    if (NT_SUCCESS(Status))
    {
        PKEY_VALUE_PARTIAL_INFORMATION PartialInfo;
        UNICODE_STRING ValueName;
        ULONG Length;

        /* Initialize the unicode string based on caller data */
        ValueName.Buffer = KeyName;
        ValueName.Length = ValueName.MaximumLength = KeyNameLength;

        Length = ParameterLength + sizeof(KEY_VALUE_PARTIAL_INFORMATION);
        PartialInfo = ExAllocatePool(PagedPool, Length);
        if (PartialInfo)
        {
            Status = ZwQueryValueKey(DevInstRegKey, &ValueName,
                KeyValuePartialInformation, PartialInfo, Length, &Length);
            if (Status == STATUS_BUFFER_OVERFLOW || Status == STATUS_BUFFER_TOO_SMALL)
            {
                /* The caller doesn't want all the data */
                ExFreePool(PartialInfo);
                PartialInfo = ExAllocatePool(PagedPool, Length);
                if (PartialInfo)
                {
                    Status = ZwQueryValueKey(DevInstRegKey, &ValueName,
                       KeyValuePartialInformation, PartialInfo, Length, &Length);
                }
                else
                {
                    Status = STATUS_NO_MEMORY;
                }
            }

            if (NT_SUCCESS(Status))
            {
                /* Compute the length to copy back */
                if (ParameterLength < PartialInfo->DataLength)
                    Length = ParameterLength;
                else
                    Length = PartialInfo->DataLength;

                RtlCopyMemory(Parameter,
                              PartialInfo->Data,
                              Length);
            }

            if (PartialInfo)
            {
                ExFreePool(PartialInfo);
            }
        } else
            Status = STATUS_NO_MEMORY;
        ZwClose(DevInstRegKey);
    }
    return Status;
}
