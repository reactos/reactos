#ifndef _USBHUB_H_
#define _USBHUB_H_

#include <wdm.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>

#define USB_HUB_TAG 'hbsu'
#define USB_MAXCHILDREN 127

// Lifted from broken header above
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

typedef struct _PORT_STATUS_CHANGE
{
    USHORT Status;
    USHORT Change;
} PORT_STATUS_CHANGE, *PPORT_STATUS_CHANGE;

typedef struct _WORK_ITEM_DATA
{
    WORK_QUEUE_ITEM WorkItem;
    PVOID Context;
} WORK_ITEM_DATA, *PWORK_ITEM_DATA;


//
// Definitions for device's PnP state tracking, all this states are described
// in PnP Device States diagram of DDK documentation.
//
typedef enum _DEVICE_PNP_STATE {

    NotStarted = 0,         // Not started
    Started,                // After handling of START_DEVICE IRP
    StopPending,            // After handling of QUERY_STOP IRP
    Stopped,                // After handling of STOP_DEVICE IRP
    RemovePending,          // After handling of QUERY_REMOVE IRP
    SurpriseRemovePending,  // After handling of SURPRISE_REMOVE IRP
    Deleted,                // After handling of REMOVE_DEVICE IRP
    UnKnown                 // Unknown state

} DEVICE_PNP_STATE;

#define INITIALIZE_PNP_STATE(Data) \
(Data).PnPState = NotStarted;\
(Data).PreviousPnPState = NotStarted;

#define SET_NEW_PNP_STATE(Data, state) \
(Data).PreviousPnPState = (Data).PnPState;\
(Data).PnPState = (state);

#define RESTORE_PREVIOUS_PNP_STATE(Data) \
(Data).PnPState = (Data).PreviousPnPState;

typedef struct
{
    BOOLEAN IsFDO;
    // We'll track device PnP state via this variables
    DEVICE_PNP_STATE PnPState;
    DEVICE_PNP_STATE PreviousPnPState;
    // Remove lock
    IO_REMOVE_LOCK RemoveLock;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _HUB_CHILDDEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    PDEVICE_OBJECT ParentDeviceObject;
    PUSB_DEVICE_HANDLE UsbDeviceHandle;
    ULONG PortNumber;
    UNICODE_STRING usDeviceId;
    UNICODE_STRING usInstanceId;
    UNICODE_STRING usHardwareIds;
    UNICODE_STRING usCompatibleIds;
    UNICODE_STRING usTextDescription;
    UNICODE_STRING usLocationInformation;
    USB_DEVICE_DESCRIPTOR DeviceDesc;
    PUSB_CONFIGURATION_DESCRIPTOR FullConfigDesc;
    UNICODE_STRING SymbolicLinkName;
    USB_BUS_INTERFACE_USBDI_V2 DeviceInterface;
    USB_DEVICE_INFORMATION_0 DeviceInformation;
} HUB_CHILDDEVICE_EXTENSION, *PHUB_CHILDDEVICE_EXTENSION;

typedef struct _HUB_DEVICE_EXTENSION
{
    COMMON_DEVICE_EXTENSION Common;
    PDEVICE_OBJECT LowerDeviceObject;
    ULONG ChildCount;
    PDEVICE_OBJECT ChildDeviceObject[USB_MAXCHILDREN];
    PDEVICE_OBJECT RootHubPhysicalDeviceObject;
    PDEVICE_OBJECT RootHubFunctionalDeviceObject;

    KGUARDED_MUTEX HubMutexLock;

    ULONG NumberOfHubs;
    KEVENT ResetComplete;

    PORT_STATUS_CHANGE *PortStatusChange;
    URB PendingSCEUrb;
    PIRP PendingSCEIrp;

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
    PVOID RootHubHandle;

    UNICODE_STRING SymbolicLinkName;
    ULONG InstanceCount;

} HUB_DEVICE_EXTENSION, *PHUB_DEVICE_EXTENSION;

// createclose.c
NTSTATUS NTAPI
USBHUB_Create(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS NTAPI
USBHUB_Close(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS NTAPI
USBHUB_Cleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

// fdo.c
NTSTATUS
USBHUB_FdoHandleDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
USBHUB_FdoHandlePnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

// misc.c
NTSTATUS
ForwardIrpAndWait(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
ForwardIrpAndForget(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

NTSTATUS
SubmitRequestToRootHub(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG IoControlCode,
    OUT PVOID OutParameter1,
    OUT PVOID OutParameter2);

NTSTATUS
FDO_QueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUSB_BUS_INTERFACE_USBDI_V2 Interface);

// pdo.c
NTSTATUS
USBHUB_PdoHandlePnp(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
USBHUB_PdoHandleInternalDeviceControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

VOID
DumpDeviceDescriptor(
    PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);

VOID
DumpConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);

VOID
DumpFullConfigurationDescriptor(
    PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor);

NTSTATUS
GetPortStatusAndChange(
    IN PDEVICE_OBJECT RootHubDeviceObject,
    IN ULONG PortId,
    OUT PPORT_STATUS_CHANGE StatusChange);

// hub_fdo.c

NTSTATUS
USBHUB_ParentFDOStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp);

#endif /* _USBHUB_H_ */
