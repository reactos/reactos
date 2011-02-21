#include <ntddk.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbioctl.h>
#include <usb.h>
#include <debug.h>
//BROKEN: #include <usbprotocoldefs.h>

#define USB_HUB_TAG 'hbsu'
#define USB_MAXCHILDREN 127

/* Lifted from broken header above */
#define C_HUB_LOCAL_POWER                    0
#define C_HUB_OVER_CURRENT                   1
#define PORT_CONNECTION                      0
#define PORT_ENABLE                          1
#define PORT_SUSPEND                         2
#define PORT_OVER_CURRENT                    3
#define PORT_RESET                           4
#define PORT_POWER                           8
#define PORT_LOW_SPEED                       9
#define C_PORT_CONNECTION                    16
#define C_PORT_ENABLE                        17
#define C_PORT_SUSPEND                       18
#define C_PORT_OVER_CURRENT                  19
#define C_PORT_RESET                         20
#define PORT_TEST                            21
#define PORT_INDICATOR                       22

typedef struct _USB_ENDPOINT
{
    ULONG Flags;
    LIST_ENTRY  UrbList;
    struct _USB_INTERFACE *Interface;
    USB_ENDPOINT_DESCRIPTOR EndPointDescriptor;
} USB_ENDPOINT, *PUSB_ENDPOINT;

typedef struct _USB_INTERFACE
{
    struct _USB_CONFIGURATION *Config;
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    USB_ENDPOINT *EndPoints[];
} USB_INTERFACE, *PUSB_INTERFACE;

typedef struct _USB_CONFIGURATION
{
    struct _USB_DEVICE *Device;
    USB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
    USB_INTERFACE *Interfaces[];
} USB_CONFIGURATION, *PUSB_CONFIGURATION;

typedef struct _USB_DEVICE
{
    UCHAR Address;
    ULONG Port;
    PVOID ParentDevice;
    BOOLEAN IsHub;
    USB_DEVICE_SPEED DeviceSpeed;
    USB_DEVICE_TYPE DeviceType;
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
    USB_CONFIGURATION *ActiveConfig;
    USB_INTERFACE *ActiveInterface;
    USB_CONFIGURATION **Configs;
} USB_DEVICE, *PUSB_DEVICE;

typedef struct _WORKITEMDATA
{
    WORK_QUEUE_ITEM WorkItem;
    PVOID Context;
} WORKITEMDATA, *PWORKITEMDATA;

typedef struct _HUB_CHILDDEVICE_EXTENSION
{
    BOOLEAN IsFDO;
    PDEVICE_OBJECT Parent;
    PWCHAR DeviceId;          // REG_SZ
    PWCHAR InstanceId;        // REG_SZ
    PWCHAR HardwareIds;       // REG_MULTI_SZ
    PWCHAR CompatibleIds;     // REG_MULTI_SZ
    PWCHAR TextDescription;
    UNICODE_STRING SymbolicLinkName;
} HUB_CHILDDEVICE_EXTENSION, *PHUB_CHILDDEVICE_EXTENSION;

typedef struct _HUB_DEVICE_EXTENSION
{
    BOOLEAN IsFDO;
    USB_DEVICE* dev;
    PDEVICE_OBJECT LowerDevice;
    ULONG ChildCount;
    PDEVICE_OBJECT Children[USB_MAXCHILDREN];

    PUSB_DEVICE UsbChildren[USB_MAXCHILDREN];

    PUSB_DEVICE RootHubUsbDevice;

    PDEVICE_OBJECT RootHubPdo;
    PDEVICE_OBJECT RootHubFdo;

    ULONG HubCount;

    USHORT PortStatus[256];
    URB Urb;

    USB_BUS_INTERFACE_HUB_V5 HubInterface;
    USB_BUS_INTERFACE_USBDI_V2 UsbDInterface;

    USB_HUB_DESCRIPTOR HubDescriptor;
    USB_DEVICE_DESCRIPTOR HubDeviceDescriptor;

    USB_CONFIGURATION_DESCRIPTOR HubConfigDescriptor;
    USB_INTERFACE_DESCRIPTOR HubInterfaceDescriptor;
    USB_ENDPOINT_DESCRIPTOR HubEndPointDescriptor;

    USB_EXTHUB_INFORMATION_0 UsbExtHubInfo;
    USB_DEVICE_INFORMATION_0 DeviceInformation;

    USBD_CONFIGURATION_HANDLE ConfigurationHandle;
    USBD_PIPE_HANDLE PipeHandle;

    UNICODE_STRING SymbolicLinkName;
} HUB_DEVICE_EXTENSION, *PHUB_DEVICE_EXTENSION;

/* createclose.c */
NTSTATUS NTAPI
UsbhubCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS NTAPI
UsbhubClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS NTAPI
UsbhubCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

/* fdo.c */
NTSTATUS NTAPI
UsbhubPnpFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
UsbhubDeviceControlFdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

/* misc.c */
NTSTATUS
ForwardIrpAndWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS NTAPI
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
UsbhubDuplicateUnicodeString(
    OUT PUNICODE_STRING Destination,
    IN PUNICODE_STRING Source,
    IN POOL_TYPE PoolType);

NTSTATUS
UsbhubInitMultiSzString(
    OUT PUNICODE_STRING Destination,
    .../* list of PCSZ */);

/* pdo.c */
NTSTATUS NTAPI
UsbhubPnpPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
UsbhubInternalDeviceControlPdo(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);
