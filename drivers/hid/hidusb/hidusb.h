#pragma once

#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
#include <ntddk.h>
#include <hidport.h>
#include <debug.h>

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
    // hid descriptor
    //
    PHID_DESCRIPTOR HidDescriptor;

    USHORT VendorID;
    USHORT ProductID;
    USHORT VersionNumber;

}HID_USB_DEVICE_EXTENSION, *PHID_USB_DEVICE_EXTENSION;

