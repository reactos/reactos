#pragma once

#include <hidclass.h>

typedef struct _HID_MINIDRIVER_REGISTRATION
{
    ULONG           Revision;
    PDRIVER_OBJECT  DriverObject;
    PUNICODE_STRING RegistryPath;
    ULONG           DeviceExtensionSize;
    BOOLEAN         DevicesArePolled;
    UCHAR           Reserved[3];
}HID_MINIDRIVER_REGISTRATION, *PHID_MINIDRIVER_REGISTRATION;

typedef struct _HID_DEVICE_EXTENSION
{
    PDEVICE_OBJECT  PhysicalDeviceObject;
    PDEVICE_OBJECT  NextDeviceObject;
    PVOID           MiniDeviceExtension;
}HID_DEVICE_EXTENSION, *PHID_DEVICE_EXTENSION;

typedef struct _HID_DEVICE_ATTRIBUTES
{
    ULONG           Size;
    USHORT          VendorID;
    USHORT          ProductID;
    USHORT          VersionNumber;
    USHORT          Reserved[11];
}HID_DEVICE_ATTRIBUTES, * PHID_DEVICE_ATTRIBUTES;

#include <pshpack1.h>

typedef struct _HID_DESCRIPTOR
{
    UCHAR   bLength;
    UCHAR   bDescriptorType;
    USHORT  bcdHID;
    UCHAR   bCountry;
    UCHAR   bNumDescriptors;

    struct _HID_DESCRIPTOR_DESC_LIST
    {
        UCHAR    bReportType;
        USHORT   wReportLength;
    }DescriptorList [1];
}HID_DESCRIPTOR, * PHID_DESCRIPTOR;

#include <poppack.h>

#define HID_HID_DESCRIPTOR_TYPE             0x21
#define HID_REPORT_DESCRIPTOR_TYPE          0x22
#define HID_PHYSICAL_DESCRIPTOR_TYPE        0x23



typedef
VOID
(NTAPI *HID_SEND_IDLE_CALLBACK)(
    IN PVOID Context
);

typedef struct _HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO
{
    HID_SEND_IDLE_CALLBACK IdleCallback;
    PVOID IdleContext;
}HID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO, *PHID_SUBMIT_IDLE_NOTIFICATION_CALLBACK_INFO;

NTSTATUS
NTAPI
HidRegisterMinidriver(
    IN PHID_MINIDRIVER_REGISTRATION  MinidriverRegistration
);

#if(NTDDI_VERSION>=NTDDI_WINXPSP1)

NTSTATUS
HidNotifyPresence(
    IN PDEVICE_OBJECT DeviceObject,
    IN BOOLEAN IsPresent
);

#endif

#define IOCTL_HID_GET_DEVICE_DESCRIPTOR             HID_CTL_CODE(0)
#define IOCTL_HID_GET_REPORT_DESCRIPTOR             HID_CTL_CODE(1)
#define IOCTL_HID_READ_REPORT                       HID_CTL_CODE(2)
#define IOCTL_HID_WRITE_REPORT                      HID_CTL_CODE(3)
#define IOCTL_HID_GET_STRING                        HID_CTL_CODE(4)
#define IOCTL_HID_ACTIVATE_DEVICE                   HID_CTL_CODE(7)
#define IOCTL_HID_DEACTIVATE_DEVICE                 HID_CTL_CODE(8)
#define IOCTL_HID_GET_DEVICE_ATTRIBUTES             HID_CTL_CODE(9)
#define IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST    HID_CTL_CODE(10)

#define HID_HID_DESCRIPTOR_TYPE             0x21
#define HID_REPORT_DESCRIPTOR_TYPE          0x22
#define HID_PHYSICAL_DESCRIPTOR_TYPE        0x23

#define HID_STRING_ID_IMANUFACTURER     14
#define HID_STRING_ID_IPRODUCT          15
#define HID_STRING_ID_ISERIALNUMBER     16
