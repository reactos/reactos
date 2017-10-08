#pragma once

#define _HIDPI_
#define _HIDPI_NO_FUNCTION_MACROS_
#define NDEBUG
#include <ntddk.h>
#include <hidport.h>
#include <debug.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usb.h>
#include <usbdlib.h>

#include <hidport.h>

typedef struct
{
    //
    // event for completion
    //
    KEVENT Event;

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

} HID_USB_DEVICE_EXTENSION, *PHID_USB_DEVICE_EXTENSION;

typedef struct
{
    //
    // request irp
    //
    PIRP Irp;

    //
    // work item
    //
    PIO_WORKITEM WorkItem;

    //
    // device object
    //
    PDEVICE_OBJECT DeviceObject;

} HID_USB_RESET_CONTEXT, *PHID_USB_RESET_CONTEXT;


NTSTATUS
Hid_GetDescriptor(
    IN PDEVICE_OBJECT DeviceObject,
    IN USHORT UrbFunction,
    IN USHORT UrbLength,
    IN OUT PVOID *UrbBuffer,
    IN OUT PULONG UrbBufferLength,
    IN UCHAR DescriptorType,
    IN UCHAR Index,
    IN USHORT LanguageIndex);

NTSTATUS
Hid_DispatchUrb(
    IN PDEVICE_OBJECT DeviceObject,
    IN PURB Urb);

#define USB_SET_IDLE_REQUEST 0xA
#define USB_GET_PROTOCOL_REQUEST 0x3

#define HIDUSB_TAG 'UdiH'
#define HIDUSB_URB_TAG 'rUiH'
