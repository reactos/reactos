/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort debugging functions
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#include "usbport.h"

#define NDEBUG
#include <debug.h>

#define NDEBUG_USBPORT_MINIPORT
#define NDEBUG_USBPORT_URB
//#define NDEBUG_USBPORT_USB2
#include "usbdebug.h"

ULONG
USBPORT_DbgPrint(IN PVOID MiniPortExtension,
                 IN ULONG Level,
                 IN PCH Format,
                 ...)
{
    DPRINT("USBPORT_DbgPrint: UNIMPLEMENTED. FIXME. \n");
    return 0;
}

ULONG
NTAPI
USBPORT_TestDebugBreak(IN PVOID MiniPortExtension)
{
    DPRINT("USBPORT_TestDebugBreak: UNIMPLEMENTED. FIXME. \n");
    return 0;
}

ULONG
NTAPI
USBPORT_AssertFailure(PVOID MiniPortExtension,
                      PVOID FailedAssertion,
                      PVOID FileName,
                      ULONG LineNumber,
                      PCHAR Message)
{
    DPRINT("USBPORT_AssertFailure: ... \n");
    RtlAssert(FailedAssertion, FileName, LineNumber, Message);
    return 0;
}

VOID
NTAPI
USBPORT_BugCheck(IN PVOID MiniPortExtension)
{
    DPRINT1("USBPORT_BugCheck: FIXME \n");
    //KeBugCheckEx(BUGCODE_USB_DRIVER, ...);
    ASSERT(FALSE);
}

ULONG
NTAPI
USBPORT_LogEntry(IN PVOID MiniPortExtension,
                 IN ULONG DriverTag,
                 IN ULONG EnumTag,
                 IN ULONG P1,
                 IN ULONG P2,
                 IN ULONG P3)
{
    DPRINT_MINIPORT("USBPORT_LogEntry: MiniPortExtension - %p, EnumTag - %lx, P1 - %lx, P2 - %lx, P3 - %lx\n",
           MiniPortExtension,
           EnumTag,
           P1,
           P2,
           P3);

    return 0;
}

VOID
NTAPI
USBPORT_DumpingDeviceDescriptor(IN PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    if (!DeviceDescriptor)
    {
        return;
    }

    DPRINT_URB("Dumping Device Descriptor - %p\n", DeviceDescriptor);
    DPRINT_URB("bLength             - %x\n", DeviceDescriptor->bLength);
    DPRINT_URB("bDescriptorType     - %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT_URB("bcdUSB              - %x\n", DeviceDescriptor->bcdUSB);
    DPRINT_URB("bDeviceClass        - %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT_URB("bDeviceSubClass     - %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT_URB("bDeviceProtocol     - %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT_URB("bMaxPacketSize0     - %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT_URB("idVendor            - %x\n", DeviceDescriptor->idVendor);
    DPRINT_URB("idProduct           - %x\n", DeviceDescriptor->idProduct);
    DPRINT_URB("bcdDevice           - %x\n", DeviceDescriptor->bcdDevice);
    DPRINT_URB("iManufacturer       - %x\n", DeviceDescriptor->iManufacturer);
    DPRINT_URB("iProduct            - %x\n", DeviceDescriptor->iProduct);
    DPRINT_URB("iSerialNumber       - %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT_URB("bNumConfigurations  - %x\n", DeviceDescriptor->bNumConfigurations);
}

VOID
NTAPI
USBPORT_DumpingConfiguration(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor)
{
    PUSB_INTERFACE_DESCRIPTOR iDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR Descriptor;
    ULONG ix;

    if (!ConfigDescriptor ||
        ConfigDescriptor->bLength < sizeof(USB_CONFIGURATION_DESCRIPTOR))
    {
        return;
    }

    DPRINT_URB("Dumping ConfigDescriptor - %p\n", ConfigDescriptor);
    DPRINT_URB("bLength             - %x\n", ConfigDescriptor->bLength);
    DPRINT_URB("bDescriptorType     - %x\n", ConfigDescriptor->bDescriptorType);
    DPRINT_URB("wTotalLength        - %x\n", ConfigDescriptor->wTotalLength);
    DPRINT_URB("bNumInterfaces      - %x\n", ConfigDescriptor->bNumInterfaces);
    DPRINT_URB("bConfigurationValue - %x\n", ConfigDescriptor->bConfigurationValue);
    DPRINT_URB("iConfiguration      - %x\n", ConfigDescriptor->iConfiguration);
    DPRINT_URB("bmAttributes        - %x\n", ConfigDescriptor->bmAttributes);
    DPRINT_URB("MaxPower            - %x\n", ConfigDescriptor->MaxPower);

    iDescriptor = (PUSB_INTERFACE_DESCRIPTOR)((ULONG_PTR)ConfigDescriptor +
                                                         ConfigDescriptor->bLength);

    if (iDescriptor->bLength < sizeof(USB_INTERFACE_DESCRIPTOR))
    {
        return;
    }

    DPRINT_URB("Dumping iDescriptor - %p\n", iDescriptor);
    DPRINT_URB("bLength             - %x\n", iDescriptor->bLength);
    DPRINT_URB("bDescriptorType     - %x\n", iDescriptor->bDescriptorType);
    DPRINT_URB("bInterfaceNumber    - %x\n", iDescriptor->bInterfaceNumber);
    DPRINT_URB("bAlternateSetting   - %x\n", iDescriptor->bAlternateSetting);
    DPRINT_URB("bNumEndpoints       - %x\n", iDescriptor->bNumEndpoints);
    DPRINT_URB("bInterfaceClass     - %x\n", iDescriptor->bInterfaceClass);
    DPRINT_URB("bInterfaceSubClass  - %x\n", iDescriptor->bInterfaceSubClass);
    DPRINT_URB("bInterfaceProtocol  - %x\n", iDescriptor->bInterfaceProtocol);
    DPRINT_URB("iInterface          - %x\n", iDescriptor->iInterface);

    Descriptor = (PUSB_ENDPOINT_DESCRIPTOR)((ULONG_PTR)iDescriptor +
                                                       iDescriptor->bLength);

    for (ix = 0; ix < iDescriptor->bNumEndpoints; ix++)
    {
        if (Descriptor->bLength < sizeof(USB_ENDPOINT_DESCRIPTOR))
        {
            return;
        }

        DPRINT_URB("Dumping Descriptor  - %p\n", Descriptor);
        DPRINT_URB("bLength             - %x\n", Descriptor->bLength);
        DPRINT_URB("bDescriptorType     - %x\n", Descriptor->bDescriptorType);
        DPRINT_URB("bEndpointAddress    - %x\n", Descriptor->bEndpointAddress);
        DPRINT_URB("bmAttributes        - %x\n", Descriptor->bmAttributes);
        DPRINT_URB("wMaxPacketSize      - %x\n", Descriptor->wMaxPacketSize);
        DPRINT_URB("bInterval           - %x\n", Descriptor->bInterval);

        Descriptor += 1;
    }
}

VOID
NTAPI
USBPORT_DumpingCapabilities(IN PDEVICE_CAPABILITIES Capabilities)
{
    if (!Capabilities)
    {
        return;
    }

    DPRINT("Capabilities->Size              - %x\n", Capabilities->Size);
    DPRINT("Capabilities->Version           - %x\n", Capabilities->Version);

    DPRINT("Capabilities->DeviceD1          - %x\n", Capabilities->DeviceD1);
    DPRINT("Capabilities->DeviceD2          - %x\n", Capabilities->DeviceD2);
    DPRINT("Capabilities->LockSupported     - %x\n", Capabilities->LockSupported);
    DPRINT("Capabilities->EjectSupported    - %x\n", Capabilities->EjectSupported);
    DPRINT("Capabilities->Removable         - %x\n", Capabilities->Removable);
    DPRINT("Capabilities->DockDevice        - %x\n", Capabilities->DockDevice);
    DPRINT("Capabilities->UniqueID          - %x\n", Capabilities->UniqueID);
    DPRINT("Capabilities->SilentInstall     - %x\n", Capabilities->SilentInstall);
    DPRINT("Capabilities->RawDeviceOK       - %x\n", Capabilities->RawDeviceOK);
    DPRINT("Capabilities->SurpriseRemovalOK - %x\n", Capabilities->SurpriseRemovalOK);

    DPRINT("Capabilities->Address           - %x\n", Capabilities->Address);
    DPRINT("Capabilities->UINumber          - %x\n", Capabilities->UINumber);

    DPRINT("Capabilities->DeviceState[0]    - %x\n", Capabilities->DeviceState[0]);
    DPRINT("Capabilities->DeviceState[1]    - %x\n", Capabilities->DeviceState[1]);
    DPRINT("Capabilities->DeviceState[2]    - %x\n", Capabilities->DeviceState[2]);
    DPRINT("Capabilities->DeviceState[3]    - %x\n", Capabilities->DeviceState[3]);
    DPRINT("Capabilities->DeviceState[4]    - %x\n", Capabilities->DeviceState[4]);
    DPRINT("Capabilities->DeviceState[5]    - %x\n", Capabilities->DeviceState[5]);
    DPRINT("Capabilities->DeviceState[6]    - %x\n", Capabilities->DeviceState[6]);

    DPRINT("Capabilities->SystemWake        - %x\n", Capabilities->SystemWake);
    DPRINT("Capabilities->DeviceWake        - %x\n", Capabilities->DeviceWake);

    DPRINT("Capabilities->D1Latency         - %x\n", Capabilities->D1Latency);
    DPRINT("Capabilities->D2Latency         - %x\n", Capabilities->D2Latency);
    DPRINT("Capabilities->D3Latency         - %x\n", Capabilities->D3Latency);
}

VOID
NTAPI
USBPORT_DumpingSetupPacket(IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket)
{
    DPRINT("SetupPacket->bmRequestType.B - %x\n", SetupPacket->bmRequestType.B);
    DPRINT("SetupPacket->bRequest        - %x\n", SetupPacket->bRequest);
    DPRINT("SetupPacket->wValue.LowByte  - %x\n", SetupPacket->wValue.LowByte);
    DPRINT("SetupPacket->wValue.HiByte   - %x\n", SetupPacket->wValue.HiByte);
    DPRINT("SetupPacket->wIndex.W        - %x\n", SetupPacket->wIndex.W);
    DPRINT("SetupPacket->wLength         - %x\n", SetupPacket->wLength);
}

VOID
NTAPI
USBPORT_DumpingURB(IN PURB Urb)
{
    PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket;

    DPRINT_URB("UrbHeader.Length           - %x\n", Urb->UrbHeader.Length);
    DPRINT_URB("UrbHeader.Function         - %x\n", Urb->UrbHeader.Function);
    DPRINT_URB("UrbHeader.Status           - %x\n", Urb->UrbHeader.Status);
    DPRINT_URB("UrbHeader.UsbdDeviceHandle - %p\n", Urb->UrbHeader.UsbdDeviceHandle);
    DPRINT_URB("UrbHeader.UsbdFlags        - %x\n", Urb->UrbHeader.UsbdFlags);

    if (Urb->UrbHeader.Length < 0x48)
    {
        return;
    }

    DPRINT_URB("PipeHandle                - %p\n", Urb->UrbControlTransfer.PipeHandle);
    DPRINT_URB("TransferFlags             - %x\n", Urb->UrbControlTransfer.TransferFlags);
    DPRINT_URB("TransferBufferLength      - %x\n", Urb->UrbControlTransfer.TransferBufferLength);
    DPRINT_URB("TransferBuffer            - %p\n", Urb->UrbControlTransfer.TransferBuffer);
    DPRINT_URB("TransferBufferMDL         - %p\n", Urb->UrbControlTransfer.TransferBufferMDL);
    DPRINT_URB("UrbLink                   - %p\n", Urb->UrbControlTransfer.UrbLink);

    if (Urb->UrbHeader.Length < 0x50)
    {
        return;
    }

    SetupPacket = (PUSB_DEFAULT_PIPE_SETUP_PACKET)&Urb->UrbControlTransfer.SetupPacket;
    USBPORT_DumpingSetupPacket(SetupPacket);
}

VOID
NTAPI
USBPORT_DumpingIDs(IN PVOID Buffer)
{
    PWSTR Ptr;
    ULONG Length;
    ULONG TotalLength = 0;

    Ptr = (PWSTR)Buffer;

    while (*Ptr)
    {
        DPRINT("  %S\n", Ptr);
        Length = (ULONG)wcslen(Ptr) + 1;

        Ptr += Length;
        TotalLength += Length;
    }

    DPRINT("TotalLength: %hu\n", TotalLength);
    DPRINT("\n");
}

VOID
NTAPI
USBPORT_DumpingEndpointProperties(IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties)
{
    DPRINT_USB2("DeviceAddress         - %X\n", EndpointProperties->DeviceAddress);
    DPRINT_USB2("EndpointAddress       - %X\n", EndpointProperties->EndpointAddress);
    DPRINT_USB2("TotalMaxPacketSize    - %X\n", EndpointProperties->TotalMaxPacketSize);
    DPRINT_USB2("Period                - %X\n", EndpointProperties->Period);
    DPRINT_USB2("DeviceSpeed           - %X\n", EndpointProperties->DeviceSpeed);
    DPRINT_USB2("UsbBandwidth          - %X\n", EndpointProperties->UsbBandwidth);
    DPRINT_USB2("ScheduleOffset        - %X\n", EndpointProperties->ScheduleOffset);
    DPRINT_USB2("TransferType          - %X\n", EndpointProperties->TransferType);
    DPRINT_USB2("MaxTransferSize       - %X\n", EndpointProperties->MaxTransferSize);
    DPRINT_USB2("HubAddr               - %X\n", EndpointProperties->HubAddr);
    DPRINT_USB2("PortNumber            - %X\n", EndpointProperties->PortNumber);
    DPRINT_USB2("InterruptScheduleMask - %X\n", EndpointProperties->InterruptScheduleMask);
    DPRINT_USB2("SplitCompletionMask   - %X\n", EndpointProperties->SplitCompletionMask);
    DPRINT_USB2("MaxPacketSize         - %X\n", EndpointProperties->MaxPacketSize);
}

VOID
NTAPI
USBPORT_DumpingTtEndpoint(IN PUSB2_TT_ENDPOINT TtEndpoint)
{
    DPRINT_USB2("MaxPacketSize   - %X\n", TtEndpoint->MaxPacketSize);
    DPRINT_USB2("Period          - %X\n", TtEndpoint->Period);
    DPRINT_USB2("TtEndpointParams- %X\n", TtEndpoint->TtEndpointParams.AsULONG);
    DPRINT_USB2("CalcBusTime     - %X\n", TtEndpoint->CalcBusTime);
    DPRINT_USB2("StartTime       - %X\n", TtEndpoint->StartTime);
    DPRINT_USB2("ActualPeriod    - %X\n", TtEndpoint->ActualPeriod);
    DPRINT_USB2("StartFrame      - %X\n", TtEndpoint->StartFrame);
    DPRINT_USB2("StartMicroframe - %X\n", TtEndpoint->StartMicroframe);
    DPRINT_USB2("Nums            - %X\n", TtEndpoint->Nums.AsULONG);
    DPRINT_USB2("nextTtEndpoint  - %X\n", TtEndpoint->NextTtEndpoint);
}

