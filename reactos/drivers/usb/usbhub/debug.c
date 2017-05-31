#include "usbhub.h"

//#define NDEBUG
#include <debug.h>

VOID
NTAPI
USBPORT_DumpingDeviceDescriptor(IN PUSB_DEVICE_DESCRIPTOR DeviceDescriptor)
{
    if (!DeviceDescriptor)
    {
        return;
    }

    DPRINT("Dumping Device Descriptor - %p\n", DeviceDescriptor);
    DPRINT("bLength             - %x\n", DeviceDescriptor->bLength);
    DPRINT("bDescriptorType     - %x\n", DeviceDescriptor->bDescriptorType);
    DPRINT("bcdUSB              - %x\n", DeviceDescriptor->bcdUSB);
    DPRINT("bDeviceClass        - %x\n", DeviceDescriptor->bDeviceClass);
    DPRINT("bDeviceSubClass     - %x\n", DeviceDescriptor->bDeviceSubClass);
    DPRINT("bDeviceProtocol     - %x\n", DeviceDescriptor->bDeviceProtocol);
    DPRINT("bMaxPacketSize0     - %x\n", DeviceDescriptor->bMaxPacketSize0);
    DPRINT("idVendor            - %x\n", DeviceDescriptor->idVendor);
    DPRINT("idProduct           - %x\n", DeviceDescriptor->idProduct);
    DPRINT("bcdDevice           - %x\n", DeviceDescriptor->bcdDevice);
    DPRINT("iManufacturer       - %x\n", DeviceDescriptor->iManufacturer);
    DPRINT("iProduct            - %x\n", DeviceDescriptor->iProduct);
    DPRINT("iSerialNumber       - %x\n", DeviceDescriptor->iSerialNumber);
    DPRINT("bNumConfigurations  - %x\n", DeviceDescriptor->bNumConfigurations);
}

VOID
NTAPI
USBPORT_DumpingConfiguration(IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor)
{
    PUSB_COMMON_DESCRIPTOR Descriptor;
    PUSB_CONFIGURATION_DESCRIPTOR cDescriptor;
    PUSB_INTERFACE_DESCRIPTOR iDescriptor;
    PUSB_ENDPOINT_DESCRIPTOR eDescriptor;

    if (!ConfigDescriptor)
    {
        return;
    }

    Descriptor = (PUSB_COMMON_DESCRIPTOR)ConfigDescriptor;

    do
    {
        if (((ULONG)Descriptor) >= ((ULONG)ConfigDescriptor +
                                    ConfigDescriptor->wTotalLength))
        {
            break;
        }

        if (Descriptor->bDescriptorType == USB_CONFIGURATION_DESCRIPTOR_TYPE)
        {
            cDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)Descriptor;

            DPRINT("Dumping cDescriptor - %p\n", cDescriptor);
            DPRINT("bLength             - %x\n", cDescriptor->bLength);
            DPRINT("bDescriptorType     - %x\n", cDescriptor->bDescriptorType);
            DPRINT("wTotalLength        - %x\n", cDescriptor->wTotalLength);
            DPRINT("bNumInterfaces      - %x\n", cDescriptor->bNumInterfaces);
            DPRINT("bConfigurationValue - %x\n", cDescriptor->bConfigurationValue);
            DPRINT("iConfiguration      - %x\n", cDescriptor->iConfiguration);
            DPRINT("bmAttributes        - %x\n", cDescriptor->bmAttributes);
            DPRINT("MaxPower            - %x\n", cDescriptor->MaxPower);
        }
        else if (Descriptor->bDescriptorType == USB_INTERFACE_DESCRIPTOR_TYPE)
        {
            iDescriptor = (PUSB_INTERFACE_DESCRIPTOR)Descriptor;

            DPRINT("Dumping iDescriptor - %p\n", iDescriptor);
            DPRINT("bLength             - %x\n", iDescriptor->bLength);
            DPRINT("bDescriptorType     - %x\n", iDescriptor->bDescriptorType);
            DPRINT("bInterfaceNumber    - %x\n", iDescriptor->bInterfaceNumber);
            DPRINT("bAlternateSetting   - %x\n", iDescriptor->bAlternateSetting);
            DPRINT("bNumEndpoints       - %x\n", iDescriptor->bNumEndpoints);
            DPRINT("bInterfaceClass     - %x\n", iDescriptor->bInterfaceClass);
            DPRINT("bInterfaceSubClass  - %x\n", iDescriptor->bInterfaceSubClass);
            DPRINT("bInterfaceProtocol  - %x\n", iDescriptor->bInterfaceProtocol);
            DPRINT("iInterface          - %x\n", iDescriptor->iInterface);
        }
        else if (Descriptor->bDescriptorType == USB_ENDPOINT_DESCRIPTOR_TYPE)
        {
            eDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)Descriptor;

            DPRINT("Dumping Descriptor  - %p\n", eDescriptor);
            DPRINT("bLength             - %x\n", eDescriptor->bLength);
            DPRINT("bDescriptorType     - %x\n", eDescriptor->bDescriptorType);
            DPRINT("bEndpointAddress    - %x\n", eDescriptor->bEndpointAddress);
            DPRINT("bmAttributes        - %x\n", eDescriptor->bmAttributes);
            DPRINT("wMaxPacketSize      - %x\n", eDescriptor->wMaxPacketSize);
            DPRINT("bInterval           - %x\n", eDescriptor->bInterval);
        }
        else
        {
            DPRINT("bDescriptorType - %x\n", Descriptor->bDescriptorType);
        }

        if (!Descriptor->bLength) 
        {
            break;
        }

        Descriptor = (PUSB_COMMON_DESCRIPTOR)((ULONG)Descriptor +
                                              Descriptor->bLength);

    } while (TRUE);
}

VOID
NTAPI
USBPORT_DumpingIDs(IN PVOID Id)
{
    PWSTR Ptr;
    ULONG Length;
    ULONG TotalLength = 0;

    Ptr = (PWSTR)Id;
    DPRINT("USBPORT_DumpingIDs:\n");

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
