/*
 * PROJECT:     ReactOS USB Hub Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBHub declarations
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef _USBHUB_H_
#define _USBHUB_H_

#include <ntddk.h>
#include <windef.h>
#include <stdio.h>
#include <wmistr.h>
#include <wmilib.h>
#include <wdmguid.h>
#include <ntstrsafe.h>
#include <usb.h>
#include <usbioctl.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <ks.h>
#include <drivers/usbport/usbmport.h>

#define USB_HUB_TAG 'BUHU'

#define USBH_EXTENSION_TYPE_HUB        0x01
#define USBH_EXTENSION_TYPE_PORT       0x02
#define USBH_EXTENSION_TYPE_PARENT     0x04
#define USBH_EXTENSION_TYPE_FUNCTION   0x08

#define USBHUB_FDO_FLAG_DEVICE_STARTED    (1 << 0)
#define USBHUB_FDO_FLAG_DEVICE_STOPPING   (1 << 2)
#define USBHUB_FDO_FLAG_DEVICE_FAILED     (1 << 3)
#define USBHUB_FDO_FLAG_REMOTE_WAKEUP     (1 << 4)
#define USBHUB_FDO_FLAG_DEVICE_STOPPED    (1 << 5)
#define USBHUB_FDO_FLAG_HUB_BUSY          (1 << 6)
#define USBHUB_FDO_FLAG_PENDING_WAKE_IRP  (1 << 7)
#define USBHUB_FDO_FLAG_RESET_PORT_LOCK   (1 << 8)
#define USBHUB_FDO_FLAG_ESD_RECOVERING    (1 << 9)
#define USBHUB_FDO_FLAG_SET_D0_STATE      (1 << 10)
#define USBHUB_FDO_FLAG_NOT_D0_STATE      (1 << 11)
#define USBHUB_FDO_FLAG_WAIT_IDLE_REQUEST (1 << 12)
#define USBHUB_FDO_FLAG_STATE_CHANGING    (1 << 13)
#define USBHUB_FDO_FLAG_DEVICE_REMOVED    (1 << 14)
#define USBHUB_FDO_FLAG_USB20_HUB         (1 << 15)
#define USBHUB_FDO_FLAG_DEFER_CHECK_IDLE  (1 << 16)
#define USBHUB_FDO_FLAG_WAKEUP_START      (1 << 17)
#define USBHUB_FDO_FLAG_MULTIPLE_TTS      (1 << 18)  // High-speed Operating Hub with Multiple TTs
#define USBHUB_FDO_FLAG_ENUM_POST_RECOVER (1 << 19)
#define USBHUB_FDO_FLAG_DO_ENUMERATION    (1 << 20)
#define USBHUB_FDO_FLAG_CHECK_IDLE_LOCK   (1 << 21)
#define USBHUB_FDO_FLAG_HIBERNATE_STATE   (1 << 22)
#define USBHUB_FDO_FLAG_NOT_ENUMERATED    (1 << 23)
#define USBHUB_FDO_FLAG_DO_SUSPENSE       (1 << 24)
#define USBHUB_FDO_FLAG_GOING_IDLE        (1 << 25)
#define USBHUB_FDO_FLAG_DEVICE_SUSPENDED  (1 << 26)
#define USBHUB_FDO_FLAG_WITEM_INIT        (1 << 27)

#define USBHUB_PDO_FLAG_HUB_DEVICE        (1 << 0)
#define USBHUB_PDO_FLAG_MULTI_INTERFACE   (1 << 1)
#define USBHUB_PDO_FLAG_INIT_PORT_FAILED  (1 << 2)
#define USBHUB_PDO_FLAG_PORT_LOW_SPEED    (1 << 3)
#define USBHUB_PDO_FLAG_REMOTE_WAKEUP     (1 << 4)
#define USBHUB_PDO_FLAG_WAIT_WAKE         (1 << 5)
#define USBHUB_PDO_FLAG_NOT_CONNECTED     (1 << 6)
#define USBHUB_PDO_FLAG_DELETE_PENDING    (1 << 7)
#define USBHUB_PDO_FLAG_POWER_D3          (1 << 8)
#define USBHUB_PDO_FLAG_DEVICE_STARTED    (1 << 9)
#define USBHUB_PDO_FLAG_HS_USB1_DUALMODE  (1 << 10)
#define USBHUB_PDO_FLAG_REG_DEV_INTERFACE (1 << 11)  // SymbolicLink
#define USBHUB_PDO_FLAG_PORT_RESTORE_FAIL (1 << 12)
#define USBHUB_PDO_FLAG_POWER_D1_OR_D2    (1 << 13)
#define USBHUB_PDO_FLAG_OVERCURRENT_PORT  (1 << 14)
#define USBHUB_PDO_FLAG_REMOVING_PORT_PDO (1 << 15)
#define USBHUB_PDO_FLAG_INSUFFICIENT_PWR  (1 << 16)
#define USBHUB_PDO_FLAG_ALLOC_BNDW_FAILED (1 << 18)
#define USBHUB_PDO_FLAG_PORT_RESSETING    (1 << 19)
#define USBHUB_PDO_FLAG_IDLE_NOTIFICATION (1 << 22)
#define USBHUB_PDO_FLAG_PORT_HIGH_SPEED   (1 << 23)
#define USBHUB_PDO_FLAG_ENUMERATED        (1 << 26)

#define USBHUB_ENUM_FLAG_DEVICE_PRESENT   0x01
#define USBHUB_ENUM_FLAG_GHOST_DEVICE     0x02

/* Hub Class Feature Selectors */
#define USBHUB_FEATURE_C_HUB_LOCAL_POWER  0
#define USBHUB_FEATURE_C_HUB_OVER_CURRENT 1

#define USBHUB_FEATURE_PORT_CONNECTION     0
#define USBHUB_FEATURE_PORT_ENABLE         1
#define USBHUB_FEATURE_PORT_SUSPEND        2
#define USBHUB_FEATURE_PORT_OVER_CURRENT   3
#define USBHUB_FEATURE_PORT_RESET          4
#define USBHUB_FEATURE_PORT_POWER          8
#define USBHUB_FEATURE_PORT_LOW_SPEED      9
#define USBHUB_FEATURE_C_PORT_CONNECTION   16
#define USBHUB_FEATURE_C_PORT_ENABLE       17
#define USBHUB_FEATURE_C_PORT_SUSPEND      18
#define USBHUB_FEATURE_C_PORT_OVER_CURRENT 19
#define USBHUB_FEATURE_C_PORT_RESET        20
#define USBHUB_FEATURE_PORT_TEST           21
#define USBHUB_FEATURE_PORT_INDICATOR      22

#define USBHUB_MAX_CASCADE_LEVELS  6
#define USBHUB_RESET_PORT_MAX_RETRY  3
#define USBHUB_MAX_REQUEST_ERRORS    3


#define USBHUB_FAIL_NO_FAIL            5
#define USBHUB_FAIL_NESTED_TOO_DEEPLY  6
#define USBHUB_FAIL_OVERCURRENT        7

extern PWSTR GenericUSBDeviceString;

typedef struct _USBHUB_PORT_DATA {
  USB_PORT_STATUS_AND_CHANGE PortStatus;
  PDEVICE_OBJECT DeviceObject;
  USB_CONNECTION_STATUS ConnectionStatus;
  ULONG PortAttributes;
} USBHUB_PORT_DATA, *PUSBHUB_PORT_DATA;

typedef struct _USBHUB_FDO_EXTENSION *PUSBHUB_FDO_EXTENSION;

typedef VOID
(NTAPI * PUSBHUB_WORKER_ROUTINE)(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PVOID Context);

typedef struct _USBHUB_IO_WORK_ITEM {
  ULONG Reserved;
  LIST_ENTRY HubWorkItemLink;
  LONG HubWorkerQueued;
  PIO_WORKITEM HubWorkItem;
  WORK_QUEUE_TYPE HubWorkItemType;
  PUSBHUB_FDO_EXTENSION HubExtension;
  PUSBHUB_WORKER_ROUTINE HubWorkerRoutine;
  PVOID HubWorkItemBuffer;
} USBHUB_IO_WORK_ITEM, *PUSBHUB_IO_WORK_ITEM;

typedef struct _COMMON_DEVICE_EXTENSION {
  ULONG ExtensionType;
  PDEVICE_OBJECT SelfDevice;
} COMMON_DEVICE_EXTENSION, *PCOMMON_DEVICE_EXTENSION;

typedef struct _USBHUB_FDO_EXTENSION {
  COMMON_DEVICE_EXTENSION Common;
  PDEVICE_OBJECT LowerPDO;
  PDEVICE_OBJECT LowerDevice;
  PDEVICE_OBJECT RootHubPdo;
  PDEVICE_OBJECT RootHubPdo2;
  KEVENT LowerDeviceEvent;
  ULONG HubFlags;
  USB_BUS_INTERFACE_HUB_V5 BusInterface;
  USB_BUS_INTERFACE_USBDI_V2 BusInterfaceUSBDI;
  DEVICE_POWER_STATE DeviceState[POWER_SYSTEM_MAXIMUM];
  SYSTEM_POWER_STATE SystemWake;
  DEVICE_POWER_STATE DeviceWake;
  POWER_STATE CurrentPowerState;
  POWER_STATE SystemPowerState;
  ULONG MaxPowerPerPort;
  USB_DEVICE_DESCRIPTOR HubDeviceDescriptor;
  USHORT Port;
  PUSB_CONFIGURATION_DESCRIPTOR HubConfigDescriptor;
  PUSB_HUB_DESCRIPTOR HubDescriptor;
  PUSBHUB_PORT_DATA PortData;
  USBD_CONFIGURATION_HANDLE ConfigHandle;
  USBD_PIPE_INFORMATION PipeInfo;
  PIRP SCEIrp;
  PIRP ResetPortIrp;
  PVOID SCEBitmap; // 11.12.4 Hub and Port Status Change Bitmap (USB 2.0 Specification)
  ULONG SCEBitmapLength;
  KEVENT RootHubNotificationEvent;
  struct _URB_CONTROL_VENDOR_OR_CLASS_REQUEST SCEWorkerUrb;
  KEVENT StatusChangeEvent;
  KSEMAPHORE IdleSemaphore;
  KSPIN_LOCK RelationsWorkerSpinLock;
  LIST_ENTRY PdoList;
  LONG PendingRequestCount;
  KEVENT PendingRequestEvent;
  KSEMAPHORE ResetDeviceSemaphore;
  PRKEVENT pResetPortEvent;
  KSEMAPHORE HubPortSemaphore;
  LONG ResetRequestCount;
  KEVENT ResetEvent;
  PIRP PendingIdleIrp;
  PIRP PendingWakeIrp;
  LONG FdoWaitWakeLock;
  LIST_ENTRY WorkItemList;
  KSPIN_LOCK WorkItemSpinLock;
  KSPIN_LOCK CheckIdleSpinLock;
  KEVENT IdleEvent;
  LONG IdleRequestLock;
  ULONG RequestErrors;
  KSEMAPHORE HubSemaphore;
  PUSBHUB_IO_WORK_ITEM WorkItemToQueue;
  USB_IDLE_CALLBACK_INFO IdleCallbackInfo;
  USB_PORT_STATUS_AND_CHANGE PortStatus;
  PIRP PowerIrp;
} USBHUB_FDO_EXTENSION, *PUSBHUB_FDO_EXTENSION;

typedef struct _USBHUB_PORT_PDO_EXTENSION {
  COMMON_DEVICE_EXTENSION Common;
  ULONG PortPdoFlags;
  ULONG EnumFlags;
  UNICODE_STRING SymbolicLinkName;
  WCHAR InstanceID[4];
  PUSBHUB_FDO_EXTENSION HubExtension;
  PUSBHUB_FDO_EXTENSION RootHubExtension;
  PUSB_DEVICE_HANDLE DeviceHandle;
  USHORT PortNumber;
  USHORT SN_DescriptorLength;
  BOOL IgnoringHwSerial;
  LPWSTR SerialNumber; // serial number string
  USB_DEVICE_DESCRIPTOR DeviceDescriptor;
  USB_DEVICE_DESCRIPTOR OldDeviceDescriptor;
  USB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;
  USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
  USHORT Reserved1;
  PIRP IdleNotificationIrp;
  POWER_STATE CurrentPowerState;
  DEVICE_CAPABILITIES Capabilities;
  ULONG MaxPower;
  PVOID BndwTimeoutContext;
  KSPIN_LOCK PortTimeoutSpinLock;
  LIST_ENTRY PortLink;
  LONG PoRequestCounter;
  LONG PendingSystemPoRequest;
  LONG PendingDevicePoRequest;
  LONG StateBehindD2;
  PIRP PdoWaitWakeIrp;
  LIST_ENTRY PortPowerList;
  KSPIN_LOCK PortPowerListSpinLock;
} USBHUB_PORT_PDO_EXTENSION, *PUSBHUB_PORT_PDO_EXTENSION;

typedef struct _USBHUB_URB_TIMEOUT_CONTEXT {
  PIRP Irp;
  KEVENT UrbTimeoutEvent;
  KDPC UrbTimeoutDPC;
  KTIMER UrbTimeoutTimer;
  KSPIN_LOCK UrbTimeoutSpinLock;
  BOOL IsNormalCompleted;
} USBHUB_URB_TIMEOUT_CONTEXT, *PUSBHUB_URB_TIMEOUT_CONTEXT;

typedef struct _USBHUB_STATUS_CHANGE_CONTEXT {
  ULONG Reserved;
  BOOL IsRequestErrors;
  PUSBHUB_FDO_EXTENSION HubExtension;
} USBHUB_STATUS_CHANGE_CONTEXT, *PUSBHUB_STATUS_CHANGE_CONTEXT;

typedef struct _USBHUB_IDLE_HUB_CONTEXT {
  ULONG Reserved;
  NTSTATUS Status;
} USBHUB_IDLE_HUB_CONTEXT, *PUSBHUB_IDLE_HUB_CONTEXT;

typedef struct _USBHUB_IDLE_PORT_CONTEXT {
  ULONG Reserved;
  LIST_ENTRY PwrList;
  NTSTATUS Status;
} USBHUB_IDLE_PORT_CONTEXT, *PUSBHUB_IDLE_PORT_CONTEXT;

typedef struct _USBHUB_IDLE_PORT_CANCEL_CONTEXT {
  ULONG Reserved;
  PIRP Irp;
} USBHUB_IDLE_PORT_CANCEL_CONTEXT, *PUSBHUB_IDLE_PORT_CANCEL_CONTEXT;

typedef struct _USBHUB_RESET_PORT_CONTEXT {
  ULONG Reserved;
  PUSBHUB_PORT_PDO_EXTENSION PortExtension;
  PIRP Irp;
} USBHUB_RESET_PORT_CONTEXT, *PUSBHUB_RESET_PORT_CONTEXT;

/* debug.c */
VOID
NTAPI
USBHUB_DumpingDeviceDescriptor(
  IN PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);

VOID
NTAPI
USBHUB_DumpingConfiguration(
  IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor);

VOID
NTAPI
USBHUB_DumpingIDs(
  IN PVOID Id);

/* ioctl.c */
NTSTATUS
NTAPI
USBH_DeviceControl(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PIRP Irp);

NTSTATUS
NTAPI
USBH_PdoInternalControl(
  IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
  IN PIRP Irp);

/* pnp.c */
NTSTATUS
NTAPI
USBH_PdoRemoveDevice(
  IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBH_FdoPnP(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PIRP Irp,
  IN UCHAR Minor);

NTSTATUS
NTAPI
USBH_PdoPnP(
  IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
  IN PIRP Irp,
  IN UCHAR Minor,
  OUT BOOLEAN * IsCompleteIrp);

/* power.c */
VOID
NTAPI
USBH_CompletePowerIrp(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PIRP Irp,
  IN NTSTATUS NtStatus);

NTSTATUS
NTAPI
USBH_HubSetD0(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBH_FdoPower(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PIRP Irp,
  IN UCHAR Minor);

NTSTATUS
NTAPI
USBH_PdoPower(
  IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
  IN PIRP Irp,
  IN UCHAR Minor);

VOID
NTAPI
USBH_HubCompletePortWakeIrps(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN NTSTATUS NtStatus);

VOID
NTAPI
USBH_HubCancelWakeIrp(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PIRP Irp);

VOID
NTAPI
USBH_IdleCancelPowerHubWorker(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PVOID Context);

/* usbhub.c */
NTSTATUS
NTAPI
USBH_Wait(
  IN ULONG Milliseconds);

VOID
NTAPI
USBH_CompleteIrp(
  IN PIRP Irp,
  IN NTSTATUS CompleteStatus);

NTSTATUS
NTAPI
USBH_PassIrp(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp);

PUSBHUB_PORT_PDO_EXTENSION
NTAPI
PdoExt(
  IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
NTAPI
USBH_WriteFailReasonID(
  IN PDEVICE_OBJECT DeviceObject,
  IN ULONG Data);

NTSTATUS
NTAPI
USBH_SetPdoRegistryParameter(
  IN PDEVICE_OBJECT DeviceObject,
  IN PCWSTR SourceString,
  IN PVOID Data,
  IN ULONG DataSize,
  IN ULONG Type,
  IN ULONG DevInstKeyType);

NTSTATUS
NTAPI
USBH_SyncSubmitUrb(
  IN PDEVICE_OBJECT DeviceObject,
  IN PURB Urb);

NTSTATUS
NTAPI
USBH_FdoSyncSubmitUrb(
  IN PDEVICE_OBJECT FdoDevice,
  IN PURB Urb);

NTSTATUS
NTAPI
USBH_SyncResetPort(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN USHORT Port);

NTSTATUS
NTAPI
USBH_GetDeviceType(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  OUT USB_DEVICE_TYPE * OutDeviceType);

PUSBHUB_FDO_EXTENSION
NTAPI
USBH_GetRootHubExtension(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBH_SyncGetRootHubPdo(
  IN PDEVICE_OBJECT DeviceObject,
  IN OUT PDEVICE_OBJECT * OutPdo1,
  IN OUT PDEVICE_OBJECT * OutPdo2);

NTSTATUS
NTAPI
USBH_SyncGetHubCount(
  IN PDEVICE_OBJECT DeviceObject,
  IN OUT PULONG OutHubCount);

PUSB_DEVICE_HANDLE
NTAPI
USBH_SyncGetDeviceHandle(
  IN PDEVICE_OBJECT DeviceObject);

NTSTATUS
NTAPI
USBH_GetDeviceDescriptor(
  IN PDEVICE_OBJECT DeviceObject,
  IN PUSB_DEVICE_DESCRIPTOR HubDeviceDescriptor);

NTSTATUS
NTAPI
USBH_GetConfigurationDescriptor(
  IN PDEVICE_OBJECT DeviceObject,
  IN PUSB_CONFIGURATION_DESCRIPTOR * pConfigurationDescriptor);

NTSTATUS
NTAPI
USBH_SyncGetHubDescriptor(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBH_SyncGetStringDescriptor(
  IN PDEVICE_OBJECT DeviceObject,
  IN UCHAR Index,
  IN USHORT LanguageId,
  IN PUSB_STRING_DESCRIPTOR Descriptor,
  IN ULONG NumberOfBytes,
  IN PULONG OutLength,
  IN BOOLEAN IsValidateLength);

NTSTATUS
NTAPI
USBH_SyncGetPortStatus(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN USHORT Port,
  IN PUSB_PORT_STATUS_AND_CHANGE PortStatus,
  IN ULONG Length);

NTSTATUS
NTAPI
USBH_SyncClearPortStatus(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN USHORT Port,
  IN USHORT RequestValue);

NTSTATUS
NTAPI
USBH_SyncPowerOnPorts(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBH_SyncDisablePort(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN USHORT Port);

BOOLEAN
NTAPI
USBH_HubIsBusPowered(
  IN PDEVICE_OBJECT DeviceObject,
  IN PUSB_CONFIGURATION_DESCRIPTOR HubConfigDescriptor);

NTSTATUS
NTAPI
USBH_SubmitStatusChangeTransfer(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBD_CreateDeviceEx(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PUSB_DEVICE_HANDLE * OutDeviceHandle,
  IN USB_PORT_STATUS UsbPortStatus,
  IN USHORT Port);

NTSTATUS
NTAPI
USBD_RemoveDeviceEx(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  IN ULONG Flags);

NTSTATUS
NTAPI
USBD_InitializeDeviceEx(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PUSB_DEVICE_HANDLE DeviceHandle,
  IN PUCHAR DeviceDescriptorBuffer,
  IN ULONG DeviceDescriptorBufferLength,
  IN PUCHAR ConfigDescriptorBuffer,
  IN ULONG ConfigDescriptorBufferLength);

VOID
NTAPI
USBHUB_SetDeviceHandleData(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PDEVICE_OBJECT UsbDevicePdo,
  IN PVOID DeviceHandle);

VOID
NTAPI
USBHUB_FlushAllTransfers(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBD_GetDeviceInformationEx(
  IN PUSBHUB_PORT_PDO_EXTENSION PortExtension,
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PUSB_NODE_CONNECTION_INFORMATION_EX Info,
  IN ULONG Length,
  IN PUSB_DEVICE_HANDLE DeviceHandle);

NTSTATUS
NTAPI
USBD_RestoreDeviceEx(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN OUT PUSB_DEVICE_HANDLE OldDeviceHandle,
  IN OUT PUSB_DEVICE_HANDLE NewDeviceHandle);

NTSTATUS
NTAPI
USBH_AllocateWorkItem(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  OUT PUSBHUB_IO_WORK_ITEM * OutHubIoWorkItem,
  IN PUSBHUB_WORKER_ROUTINE WorkerRoutine,
  IN SIZE_T BufferLength,
  OUT PVOID * OutHubWorkItemBuffer,
  IN WORK_QUEUE_TYPE Type);

VOID
NTAPI
USBH_QueueWorkItem(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PUSBHUB_IO_WORK_ITEM HubIoWorkItem);

VOID
NTAPI
USBH_FreeWorkItem(
  IN PUSBHUB_IO_WORK_ITEM HubIoWorkItem);

NTSTATUS
NTAPI
USBD_RegisterRootHubCallBack(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBD_UnRegisterRootHubCallBack(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

VOID
NTAPI
USBH_HubCancelIdleIrp(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN PIRP IdleIrp);

BOOLEAN
NTAPI
USBH_CheckIdleAbort(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN BOOLEAN IsWait,
  IN BOOLEAN IsExtCheck);

VOID
NTAPI
USBH_CheckHubIdle(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

VOID
NTAPI
USBH_CheckIdleDeferred(
  IN PUSBHUB_FDO_EXTENSION HubExtension);

NTSTATUS
NTAPI
USBH_CheckDeviceLanguage(
  IN PDEVICE_OBJECT DeviceObject,
  IN USHORT LanguageId);

NTSTATUS
NTAPI
USBH_CreateDevice(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN USHORT Port,
  IN USB_PORT_STATUS UsbPortStatus,
  IN ULONG IsWait);

NTSTATUS
NTAPI
USBH_ResetDevice(
  IN PUSBHUB_FDO_EXTENSION HubExtension,
  IN USHORT Port,
  IN BOOLEAN IsKeepDeviceData,
  IN BOOLEAN IsWait);

NTSTATUS
NTAPI
DriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath);

#endif /* _USBHUB_H_ */
