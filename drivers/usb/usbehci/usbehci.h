#pragma once

#include "hardware.h"
#include <ntifs.h>
#include <ntddk.h>
#include <stdio.h>
#define	NDEBUG
#include <debug.h>
#include <hubbusif.h>
#include <usbioctl.h>
#include <usb.h>

#define USB_POOL_TAG (ULONG)'ebsu'

#define	DEVICEINTIALIZED		0x01
#define	DEVICESTARTED			0x02
#define	DEVICEBUSY			0x04
#define DEVICESTOPPED			0x08
#define DEVICESTALLED          0x10


#define	MAX_USB_DEVICES			127
#define	EHCI_MAX_SIZE_TRANSFER		0x100000

#define C_HUB_LOCAL_POWER   0
#define C_HUB_OVER_CURRENT  1
#define PORT_CONNECTION     0
#define PORT_ENABLE         1
#define PORT_SUSPEND        2
#define PORT_OVER_CURRENT   3
#define PORT_RESET          4
#define PORT_POWER          8
#define PORT_LOW_SPEED      9
#define PORT_HIGH_SPEED     9
#define C_PORT_CONNECTION   16
#define C_PORT_ENABLE       17
#define C_PORT_SUSPEND      18
#define C_PORT_OVER_CURRENT 19
#define C_PORT_RESET        20
#define PORT_TEST           21
#define PORT_INDICATOR      22
#define USB_PORT_STATUS_CHANGE 0x4000

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
    UNICODE_STRING LanguageIDs;
    UNICODE_STRING iManufacturer;
    UNICODE_STRING iProduct;
    UNICODE_STRING iSerialNumber;
    USB_CONFIGURATION *ActiveConfig;
    USB_INTERFACE *ActiveInterface;
    USB_CONFIGURATION **Configs;
} USB_DEVICE, *PUSB_DEVICE;



typedef struct _COMMON_DEVICE_EXTENSION
{
    BOOLEAN IsFdo;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _EHCIPORTS
{
    ULONG PortNumber;
    ULONG PortType;
    USHORT PortStatus;
    USHORT PortChange;
} EHCIPORTS, *PEHCIPORTS;

typedef struct _FDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT Pdo;
    ULONG DeviceState;

    PVOID RootHubDeviceHandle;
    PDMA_ADAPTER pDmaAdapter;

    ULONG Vector;
    KIRQL Irql;

    KTIMER UpdateTimer;
    KINTERRUPT_MODE Mode;
    BOOLEAN IrqShared;
    PKINTERRUPT EhciInterrupt;
    KDPC DpcObject;
    KDPC TimerDpcObject;

    KAFFINITY Affinity;

    ULONG MapRegisters;

    ULONG BusNumber;
    ULONG BusAddress;
    ULONG PCIAddress;
    USHORT VendorId;
    USHORT DeviceId;

    BUS_INTERFACE_STANDARD BusInterface;

    union
    {
        ULONG ResourcePort;
        ULONG ResourceMemory;
    };

    EHCI_HOST_CONTROLLER hcd;
    PERIODICFRAMELIST PeriodicFrameList;
    
    FAST_MUTEX FrameListMutex;

    BOOLEAN AsyncComplete;

    //PULONG ResourceBase;
    //ULONG Size;
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;

typedef struct _PDO_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT ControllerFdo;
    PUSB_DEVICE UsbDevices[127];
    LIST_ENTRY IrpQueue;
    KSPIN_LOCK IrpQueueLock;
    PIRP CurrentIrp;
    HANDLE ThreadHandle;
    ULONG ChildDeviceCount;
    BOOLEAN HaltQueue;
    PVOID CallbackContext;
    RH_INIT_CALLBACK *CallbackRoutine;
    USB_IDLE_CALLBACK IdleCallback;
    PVOID IdleContext;
    ULONG NumberOfPorts;
    EHCIPORTS Ports[32];
    KTIMER Timer;
    KEVENT QueueDrainedEvent;
    FAST_MUTEX ListLock;
} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

typedef struct _WORKITEMDATA
{
    WORK_QUEUE_ITEM WorkItem;
    PVOID Context;
} WORKITEMDATA, *PWORKITEMDATA;

VOID NTAPI
UrbWorkerThread(PVOID Context);

NTSTATUS NTAPI
GetBusInterface(PDEVICE_OBJECT pcifido, PBUS_INTERFACE_STANDARD busInterface);

NTSTATUS NTAPI
ForwardAndWaitCompletionRoutine(PDEVICE_OBJECT DeviceObject, PIRP Irp, PKEVENT Event);

NTSTATUS NTAPI
ForwardAndWait(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
ForwardIrpAndForget(PDEVICE_OBJECT DeviceObject,PIRP Irp);

NTSTATUS NTAPI
FdoDispatchPnp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS NTAPI
PdoDispatchPnp(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

NTSTATUS NTAPI
AddDevice(PDRIVER_OBJECT DriverObject, PDEVICE_OBJECT Pdo);

NTSTATUS
DuplicateUnicodeString(ULONG Flags, PCUNICODE_STRING SourceString, PUNICODE_STRING DestinationString);

PWSTR
GetSymbolicName(PDEVICE_OBJECT DeviceObject);

PWSTR
GetPhysicalDeviceObjectName(PDEVICE_OBJECT DeviceObject);

NTSTATUS NTAPI
PdoDispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS NTAPI
FdoDispatchInternalDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);

USBD_STATUS
ExecuteControlRequest(PFDO_DEVICE_EXTENSION DeviceExtension, PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket, UCHAR Address, ULONG Port, PVOID Buffer, ULONG BufferLength);

VOID
RequestURBCancel (PPDO_DEVICE_EXTENSION DeviceExtension, PIRP Irp);

VOID
RemoveUrbRequest(PPDO_DEVICE_EXTENSION PdoDeviceExtension, PIRP Irp);

VOID
QueueURBRequest(PPDO_DEVICE_EXTENSION DeviceExtension, PIRP Irp);

VOID
CompletePendingURBRequest(PPDO_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
HandleUrbRequest(PPDO_DEVICE_EXTENSION DeviceExtension, PIRP Irp);

PUSB_DEVICE
DeviceHandleToUsbDevice(PPDO_DEVICE_EXTENSION PdoDeviceExtension, PUSB_DEVICE_HANDLE DeviceHandle);

VOID
DumpDeviceDescriptor(PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);

VOID
DumpFullConfigurationDescriptor(PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);


VOID
DumpTransferDescriptor(PQUEUE_TRANSFER_DESCRIPTOR TransferDescriptor);

VOID
DumpQueueHead(PQUEUE_HEAD QueueHead);

