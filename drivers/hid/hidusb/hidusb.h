#pragma once

#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
#include <ntddk.h>
#include <hidport.h>
#include <debug.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usb.h>
#include <usbdlib.h>

typedef struct
{
    //
    // event for completion
    //
    KEVENT Event;

    //
    // list for pending requests
    //
    LIST_ENTRY PendingRequests;

    //
    // device descriptor
    //
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor;

    //
    // configuration descriptor
    //
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;

    //
    // interface information
    //
    PUSBD_INTERFACE_INFORMATION InterfaceInfo;

    //
    // configuration handle
    //
    USBD_CONFIGURATION_HANDLE ConfigurationHandle;

    //
    // hid descriptor
    //
    PHID_DESCRIPTOR HidDescriptor;
}HID_USB_DEVICE_EXTENSION, *PHID_USB_DEVICE_EXTENSION;

