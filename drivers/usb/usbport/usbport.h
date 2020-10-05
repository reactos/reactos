/*
 * PROJECT:     ReactOS USB Port Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     USBPort declarations
 * COPYRIGHT:   Copyright 2017 Vadim Galyant <vgal@rambler.ru>
 */

#ifndef USBPORT_H__
#define USBPORT_H__

#include <ntifs.h>
#include <windef.h>
#include <stdio.h>
#include <wdmguid.h>
#include <ntstrsafe.h>
#include <usb.h>
#include <hubbusif.h>
#include <usbbusif.h>
#include <usbdlib.h>
#include <usbuser.h>
#include <drivers/usbport/usbmport.h>

#define PCI_INTERFACE_USB_ID_UHCI 0x00
#define PCI_INTERFACE_USB_ID_OHCI 0x10
#define PCI_INTERFACE_USB_ID_EHCI 0x20
#define PCI_INTERFACE_USB_ID_XHCI 0x30

#ifdef USBD_TRANSFER_DIRECTION // due hubbusif.h included usbdi.h (Which overwrites...)
#undef USBD_TRANSFER_DIRECTION
#define USBD_TRANSFER_DIRECTION 0x00000001
#endif

#define USBPORT_RECIPIENT_HUB  BMREQUEST_TO_DEVICE
#define USBPORT_RECIPIENT_PORT BMREQUEST_TO_OTHER

#define INVALIDATE_ENDPOINT_ONLY           0
#define INVALIDATE_ENDPOINT_WORKER_THREAD  1
#define INVALIDATE_ENDPOINT_WORKER_DPC     2
#define INVALIDATE_ENDPOINT_INT_NEXT_SOF   3

#define USBPORT_DMA_DIRECTION_FROM_DEVICE 1
#define USBPORT_DMA_DIRECTION_TO_DEVICE   2

#define USB_PORT_TAG 'pbsu'
#define URB_FUNCTION_MAX 0x31

/* Hub Class Feature Selectors (Recipient - Port) */
#define FEATURE_PORT_CONNECTION     0
#define FEATURE_PORT_ENABLE         1
#define FEATURE_PORT_SUSPEND        2
#define FEATURE_PORT_OVER_CURRENT   3
#define FEATURE_PORT_RESET          4
#define FEATURE_PORT_POWER          8
#define FEATURE_PORT_LOW_SPEED      9
#define FEATURE_C_PORT_CONNECTION   16
#define FEATURE_C_PORT_ENABLE       17
#define FEATURE_C_PORT_SUSPEND      18
#define FEATURE_C_PORT_OVER_CURRENT 19
#define FEATURE_C_PORT_RESET        20

/* Hub Class Feature Selectors (Recipient - Hub) */
#define FEATURE_C_HUB_LOCAL_POWER  0
#define FEATURE_C_HUB_OVER_CURRENT 1

/* Flags */
#define USBPORT_FLAG_INT_CONNECTED     0x00000001
#define USBPORT_FLAG_HC_STARTED        0x00000002
#define USBPORT_FLAG_HC_POLLING        0x00000004
#define USBPORT_FLAG_WORKER_THREAD_ON  0x00000008
#define USBPORT_FLAG_WORKER_THREAD_EXIT 0x00000010
#define USBPORT_FLAG_HC_SUSPEND        0x00000100
#define USBPORT_FLAG_INTERRUPT_ENABLED 0x00000400
#define USBPORT_FLAG_SELECTIVE_SUSPEND 0x00000800
#define USBPORT_FLAG_DOS_SYMBOLIC_NAME 0x00010000
#define USBPORT_FLAG_LEGACY_SUPPORT    0x00080000
#define USBPORT_FLAG_HC_WAKE_SUPPORT   0x00200000
#define USBPORT_FLAG_DIAGNOSTIC_MODE   0x00800000 //IOCTL_USB_DIAGNOSTIC_MODE_ON
#define USBPORT_FLAG_COMPANION_HC      0x01000000
#define USBPORT_FLAG_REGISTERED_FDO    0x02000000
#define USBPORT_FLAG_NO_HACTION        0x04000000
#define USBPORT_FLAG_BIOS_DISABLE_SS   0x08000000 //Selective Suspend
#define USBPORT_FLAG_PWR_AND_CHIRP_LOCK 0x10000000
#define USBPORT_FLAG_POWER_AND_CHIRP_OK 0x40000000
#define USBPORT_FLAG_RH_INIT_CALLBACK  0x80000000

/* PnP state Flags */
#define USBPORT_PNP_STATE_NOT_INIT  0x00000001
#define USBPORT_PNP_STATE_STARTED   0x00000002
#define USBPORT_PNP_STATE_FAILED    0x00000004
#define USBPORT_PNP_STATE_STOPPED   0x00000008

/* Timer Flags */
#define USBPORT_TMFLAG_TIMER_QUEUED       0x00000001
#define USBPORT_TMFLAG_HC_SUSPENDED       0x00000002
#define USBPORT_TMFLAG_HC_RESUME          0x00000004
#define USBPORT_TMFLAG_RH_SUSPENDED       0x00000008
#define USBPORT_TMFLAG_TIMER_STARTED      0x00000010
#define USBPORT_TMFLAG_WAKE               0x00000020
#define USBPORT_TMFLAG_IDLE_QUEUEITEM_ON  0x00000040

/* Miniport Flags */
#define USBPORT_MPFLAG_INTERRUPTS_ENABLED  0x00000001
#define USBPORT_MPFLAG_SUSPENDED           0x00000002

/* Device handle Flags (USBPORT_DEVICE_HANDLE) */
#define DEVICE_HANDLE_FLAG_ROOTHUB     0x00000002
#define DEVICE_HANDLE_FLAG_REMOVED     0x00000008
#define DEVICE_HANDLE_FLAG_USB2HUB     0x00000010

/* Endpoint Flags (USBPORT_ENDPOINT) */
#define ENDPOINT_FLAG_DMA_TYPE      0x00000001
#define ENDPOINT_FLAG_ROOTHUB_EP0   0x00000002
#define ENDPOINT_FLAG_NUKE          0x00000008
#define ENDPOINT_FLAG_QUEUENE_EMPTY 0x00000010
#define ENDPOINT_FLAG_ABORTING      0x00000020
#define ENDPOINT_FLAG_IDLE          0x00000100
#define ENDPOINT_FLAG_OPENED        0x00000200
#define ENDPOINT_FLAG_CLOSED        0x00000400

/* UsbdFlags Flags (URB) */
#define USBD_FLAG_ALLOCATED_MDL      0x00000002
#define USBD_FLAG_NOT_ISO_TRANSFER   0x00000010
#define USBD_FLAG_ALLOCATED_TRANSFER 0x00000020

/* Pipe handle Flags (USBPORT_PIPE_HANDLE) */
#define PIPE_HANDLE_FLAG_CLOSED 0x00000001
#define PIPE_HANDLE_FLAG_NULL_PACKET_SIZE 0x00000002

/* Transfer Flags (USBPORT_TRANSFER) */
#define TRANSFER_FLAG_CANCELED   0x00000001
#define TRANSFER_FLAG_DMA_MAPPED 0x00000002
#define TRANSFER_FLAG_HIGH_SPEED 0x00000004
#define TRANSFER_FLAG_SUBMITED   0x00000008
#define TRANSFER_FLAG_ABORTED    0x00000010
#define TRANSFER_FLAG_ISO        0x00000020
#define TRANSFER_FLAG_DEVICE_GONE 0x00000080
#define TRANSFER_FLAG_SPLITED    0x00000100
#define TRANSFER_FLAG_COMPLETED  0x00000200
#define TRANSFER_FLAG_PARENT     0x00000400

extern KSPIN_LOCK USBPORT_SpinLock;
extern LIST_ENTRY USBPORT_MiniPortDrivers;

typedef USBD_STATUS* PUSBD_STATUS;

typedef struct _USBPORT_COMMON_BUFFER_HEADER {
  ULONG Length;
  ULONG_PTR BaseVA;
  PHYSICAL_ADDRESS LogicalAddress;
  SIZE_T BufferLength;
  ULONG_PTR VirtualAddress;
  ULONG PhysicalAddress;
} USBPORT_COMMON_BUFFER_HEADER, *PUSBPORT_COMMON_BUFFER_HEADER;

typedef struct _USBPORT_ENDPOINT *PUSBPORT_ENDPOINT;

typedef struct _USB2_HC_EXTENSION *PUSB2_HC_EXTENSION;
typedef struct _USB2_TT_EXTENSION *PUSB2_TT_EXTENSION;
typedef struct _USB2_TT *PUSB2_TT;
typedef struct _USB2_TT_ENDPOINT *PUSB2_TT_ENDPOINT;

typedef struct _USBPORT_PIPE_HANDLE {
  ULONG Flags;
  ULONG PipeFlags;
  USB_ENDPOINT_DESCRIPTOR EndpointDescriptor;
  UCHAR Padded;
  PUSBPORT_ENDPOINT Endpoint;
  LIST_ENTRY PipeLink;
} USBPORT_PIPE_HANDLE, *PUSBPORT_PIPE_HANDLE;

typedef struct _USBPORT_CONFIGURATION_HANDLE {
  PUSB_CONFIGURATION_DESCRIPTOR ConfigurationDescriptor;
  LIST_ENTRY InterfaceHandleList;
  //USB_CONFIGURATION_DESCRIPTOR CfgDescriptor; // Body.
} USBPORT_CONFIGURATION_HANDLE, *PUSBPORT_CONFIGURATION_HANDLE;

typedef struct _USBPORT_INTERFACE_HANDLE {
  LIST_ENTRY InterfaceLink;
  UCHAR AlternateSetting;
  UCHAR Pad1[3];
  USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
  UCHAR Pad2[3];
  USBPORT_PIPE_HANDLE PipeHandle[1];
} USBPORT_INTERFACE_HANDLE, *PUSBPORT_INTERFACE_HANDLE;

typedef struct _USBPORT_DEVICE_HANDLE {
  ULONG Flags;
  USHORT DeviceAddress;
  USHORT PortNumber;
  USBPORT_PIPE_HANDLE PipeHandle;
  ULONG DeviceSpeed;
  BOOL IsRootHub;
  LIST_ENTRY PipeHandleList;
  PUSBPORT_CONFIGURATION_HANDLE ConfigHandle;
  struct _USBPORT_DEVICE_HANDLE *HubDeviceHandle;
  USB_DEVICE_DESCRIPTOR DeviceDescriptor;
  LIST_ENTRY DeviceHandleLink;
  LONG DeviceHandleLock;
  ULONG TtCount;
  PUSB2_TT_EXTENSION TtExtension; // Transaction Translator
  LIST_ENTRY TtList;
} USBPORT_DEVICE_HANDLE, *PUSBPORT_DEVICE_HANDLE;

typedef struct _USBPORT_ENDPOINT {
  ULONG Flags;
  PDEVICE_OBJECT FdoDevice;
  PUSBPORT_COMMON_BUFFER_HEADER HeaderBuffer;
  PUSBPORT_DEVICE_HANDLE DeviceHandle;
  PUSB2_TT_EXTENSION TtExtension; // Transaction Translator
  PUSB2_TT_ENDPOINT TtEndpoint;
  USBPORT_ENDPOINT_PROPERTIES EndpointProperties;
  ULONG EndpointWorker;
  ULONG FrameNumber;
  /* Locks */
  KSPIN_LOCK EndpointSpinLock;
  KIRQL EndpointOldIrql;
  KIRQL EndpointStateOldIrql;
  UCHAR Padded[2];
  LONG LockCounter;
  LONG FlushPendingLock;
  /* State */
  ULONG StateLast;
  ULONG StateNext;
  LIST_ENTRY StateChangeLink;
  KSPIN_LOCK StateChangeSpinLock;
  /* Transfer lists */
  LIST_ENTRY PendingTransferList;
  LIST_ENTRY TransferList;
  LIST_ENTRY CancelList;
  LIST_ENTRY AbortList;
  /* Links */
  LIST_ENTRY EndpointLink;
  LIST_ENTRY WorkerLink;
  LIST_ENTRY CloseLink;
  LIST_ENTRY DispatchLink;
  LIST_ENTRY FlushLink;
  LIST_ENTRY FlushControllerLink;
  LIST_ENTRY FlushAbortLink;
  LIST_ENTRY TtLink;
  LIST_ENTRY RebalanceLink;
} USBPORT_ENDPOINT, *PUSBPORT_ENDPOINT;

typedef struct _USBPORT_ISO_BLOCK *PUSBPORT_ISO_BLOCK;

typedef struct _USBPORT_TRANSFER {
  ULONG Flags;
  PIRP Irp;
  PURB Urb;
  PRKEVENT Event;
  PVOID MiniportTransfer;
  SIZE_T PortTransferLength; // Only port part
  SIZE_T FullTransferLength; // Port + miniport
  PUSBPORT_ENDPOINT Endpoint;
  USBPORT_TRANSFER_PARAMETERS TransferParameters;
  PMDL TransferBufferMDL;
  ULONG Direction;
  LIST_ENTRY TransferLink;
  USBD_STATUS USBDStatus;
  ULONG CompletedTransferLen;
  ULONG NumberOfMapRegisters;
  PVOID MapRegisterBase;
  ULONG TimeOut;
  LARGE_INTEGER Time;
  struct _USBPORT_TRANSFER * ParentTransfer;
  KSPIN_LOCK TransferSpinLock;
  LIST_ENTRY SplitTransfersList; // for parent transfers
  LIST_ENTRY SplitLink; // for splitted transfers
  ULONG Period;
  PUSBPORT_ISO_BLOCK IsoBlockPtr; // pointer on IsoBlock
  // SgList should be LAST field
  USBPORT_SCATTER_GATHER_LIST SgList; // variable length
  //USBPORT_ISO_BLOCK IsoBlock; // variable length
} USBPORT_TRANSFER, *PUSBPORT_TRANSFER;

typedef struct _USBPORT_IRP_TABLE {
  struct _USBPORT_IRP_TABLE * LinkNextTable;
  PIRP irp[0X200];
} USBPORT_IRP_TABLE, *PUSBPORT_IRP_TABLE;

typedef struct _USBPORT_COMMON_DEVICE_EXTENSION {
  PDEVICE_OBJECT SelfDevice;
  PDEVICE_OBJECT LowerPdoDevice; // PhysicalDeviceObject
  PDEVICE_OBJECT LowerDevice; // TopOfStackDeviceObject
  ULONG IsPDO;
  UNICODE_STRING SymbolicLinkName;
  BOOL IsInterfaceEnabled;
  DEVICE_POWER_STATE DevicePowerState;
  ULONG PnpStateFlags;
} USBPORT_COMMON_DEVICE_EXTENSION, *PUSBPORT_COMMON_DEVICE_EXTENSION;

typedef struct _USBPORT_DEVICE_EXTENSION {
  USBPORT_COMMON_DEVICE_EXTENSION CommonExtension;
  ULONG Flags;
  PDEVICE_OBJECT RootHubPdo; // RootHubDeviceObject
  KSPIN_LOCK RootHubCallbackSpinLock;
  LONG RHInitCallBackLock;
  LONG ChirpRootPortLock;
  KSEMAPHORE ControllerSemaphore;
  ULONG FdoNameNumber;
  UNICODE_STRING DosDeviceSymbolicName;
  ULONG UsbBIOSx;
  LIST_ENTRY ControllerLink;
  ULONG CommonBufferLimit;
  /* Miniport */
  ULONG MiniPortFlags;
  PVOID MiniPortExt;
  PUSBPORT_MINIPORT_INTERFACE MiniPortInterface;
  USBPORT_RESOURCES UsbPortResources;
  PUSBPORT_COMMON_BUFFER_HEADER MiniPortCommonBuffer;
  KSPIN_LOCK MiniportSpinLock;
  /* Bus Interface */
  BUS_INTERFACE_STANDARD BusInterface;
  USHORT VendorID;
  USHORT DeviceID;
  UCHAR RevisionID;
  UCHAR ProgIf;
  UCHAR SubClass;
  UCHAR BaseClass;
  /* Dma Adapter */
  PDMA_ADAPTER DmaAdapter;
  ULONG NumberMapRegs;
  /* Interrupt */
  PKINTERRUPT InterruptObject;
  KDPC IsrDpc;
  LONG IsrDpcCounter;
  LONG IsrDpcHandlerCounter;
  KSPIN_LOCK MiniportInterruptsSpinLock;
  KTIMER TimerSoftInterrupt;
  KDPC SoftInterruptDpc;
  /* Endpoints */
  ULONG PeriodicEndpoints;
  LIST_ENTRY EndpointList;
  KSPIN_LOCK EndpointListSpinLock;
  LIST_ENTRY EpStateChangeList;
  KSPIN_LOCK EpStateChangeSpinLock;
  LIST_ENTRY EndpointClosedList;
  KSPIN_LOCK EndpointClosedSpinLock;
  LIST_ENTRY WorkerList;
  /* Transfers */
  LIST_ENTRY MapTransferList;
  KSPIN_LOCK MapTransferSpinLock;
  LIST_ENTRY DoneTransferList;
  KSPIN_LOCK DoneTransferSpinLock;
  KDPC TransferFlushDpc;
  KSPIN_LOCK FlushTransferSpinLock;
  KSPIN_LOCK FlushPendingTransferSpinLock;
  /* Timer */
  ULONG TimerValue; // Timer period (500) msec. default
  ULONG TimerFlags;
  KTIMER TimerObject;
  KDPC TimerDpc;
  KSPIN_LOCK TimerFlagsSpinLock;
  /* Worker Thread */
  PRKTHREAD WorkerThread;
  HANDLE WorkerThreadHandle;
  KEVENT WorkerThreadEvent;
  KSPIN_LOCK WorkerThreadEventSpinLock;
  /* Usb Devices */
  ULONG UsbAddressBitMap[4];
  LIST_ENTRY DeviceHandleList;
  KSPIN_LOCK DeviceHandleSpinLock;
  KSEMAPHORE DeviceSemaphore;
  /* Device Capabilities */
  DEVICE_CAPABILITIES Capabilities;
  ULONG BusNumber;
  ULONG PciDeviceNumber;
  ULONG PciFunctionNumber;
  ULONG TotalBusBandwidth;
  /* Idle */
  LARGE_INTEGER IdleTime;
  IO_CSQ IdleIoCsq;
  KSPIN_LOCK IdleIoCsqSpinLock;
  LIST_ENTRY IdleIrpList;
  LONG IdleLockCounter;
  /* Bad Requests */
  IO_CSQ BadRequestIoCsq;
  KSPIN_LOCK BadRequestIoCsqSpinLock;
  LIST_ENTRY BadRequestList;
  LONG BadRequestLockCounter;
  /* Irp Queues */
  PUSBPORT_IRP_TABLE PendingIrpTable;
  PUSBPORT_IRP_TABLE ActiveIrpTable;
  /* Power */
  LONG SetPowerLockCounter;
  KSPIN_LOCK PowerWakeSpinLock;
  KSPIN_LOCK SetPowerD0SpinLock;
  KDPC WorkerRequestDpc;
  KDPC HcWakeDpc;
  /* Usb 2.0 HC Extension */
  PUSB2_HC_EXTENSION Usb2Extension;
  ULONG Bandwidth[32];
  KSPIN_LOCK TtSpinLock;

  /* Miniport extension should be aligned on 0x100 */
#if !defined(_M_X64)
  ULONG Padded[64];
#else
  ULONG Padded[30];
#endif

} USBPORT_DEVICE_EXTENSION, *PUSBPORT_DEVICE_EXTENSION;

#if !defined(_M_X64)
C_ASSERT(sizeof(USBPORT_DEVICE_EXTENSION) == 0x500);
#else
C_ASSERT(sizeof(USBPORT_DEVICE_EXTENSION) == 0x700);
#endif

typedef struct _USBPORT_RH_DESCRIPTORS {
  USB_DEVICE_DESCRIPTOR DeviceDescriptor;
  USB_CONFIGURATION_DESCRIPTOR ConfigDescriptor;
  USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
  USB_ENDPOINT_DESCRIPTOR EndPointDescriptor;
  USB_HUB_DESCRIPTOR Descriptor; // Size may be: 7 + 2[1..32] (7 + 2..64)
} USBPORT_RH_DESCRIPTORS, *PUSBPORT_RH_DESCRIPTORS;

typedef struct _USBPORT_RHDEVICE_EXTENSION {
  USBPORT_COMMON_DEVICE_EXTENSION CommonExtension;
  ULONG Flags;
  PDEVICE_OBJECT FdoDevice;
  ULONG PdoNameNumber;
  USBPORT_DEVICE_HANDLE DeviceHandle;
  PUSBPORT_RH_DESCRIPTORS RootHubDescriptors;
  PUSBPORT_ENDPOINT Endpoint;
  ULONG ConfigurationValue;
  PRH_INIT_CALLBACK RootHubInitCallback;
  PVOID RootHubInitContext;
  DEVICE_CAPABILITIES Capabilities;
  PIRP WakeIrp;
} USBPORT_RHDEVICE_EXTENSION, *PUSBPORT_RHDEVICE_EXTENSION;

typedef struct _USBPORT_ASYNC_CALLBACK_DATA {
  ULONG Reserved;
  PDEVICE_OBJECT FdoDevice;
  KTIMER AsyncTimer;
  KDPC AsyncTimerDpc;
  ASYNC_TIMER_CALLBACK *CallbackFunction;
  ULONG CallbackContext;
} USBPORT_ASYNC_CALLBACK_DATA, *PUSBPORT_ASYNC_CALLBACK_DATA;

C_ASSERT(sizeof(USBPORT_ASYNC_CALLBACK_DATA) == 16 + 18 * sizeof(PVOID));

typedef struct _TIMER_WORK_QUEUE_ITEM {
  WORK_QUEUE_ITEM WqItem;
  PDEVICE_OBJECT FdoDevice;
  ULONG Context;
} TIMER_WORK_QUEUE_ITEM, *PTIMER_WORK_QUEUE_ITEM;

/* Transaction Translator */
/* See Chapter 5 - USB Data Flow Model and Chapter 11 - Hub Specification */

#define USB2_FRAMES           32
#define USB2_MICROFRAMES      8
#define USB2_MAX_MICROFRAMES  (USB2_FRAMES * USB2_MICROFRAMES)
#define USB2_PREV_MICROFRAME  0xFF

#define USB2_MAX_MICROFRAME_ALLOCATION         7000 // bytes
#define USB2_CONTROLLER_DELAY                  100
#define USB2_FS_MAX_PERIODIC_ALLOCATION        1157 // ((12000 / 8 bits) * 0.9) / (7/6) - 90% max, and bits stuffing
#define USB2_FS_SOF_TIME                       6
#define USB2_HUB_DELAY                         30
#define USB2_MAX_FS_LS_TRANSACTIONS_IN_UFRAME  16
#define USB2_FS_RAW_BYTES_IN_MICROFRAME        188  // (12000 / 8 bits / USB2_MICROFRAMES) = 187,5. But we use "best case budget"

/* Overheads */
#define USB2_LS_INTERRUPT_OVERHEAD             117 // FS-bytes
#define USB2_FS_INTERRUPT_OVERHEAD             13
#define USB2_HS_INTERRUPT_OUT_OVERHEAD         45
#define USB2_HS_INTERRUPT_IN_OVERHEAD          25
#define USB2_FS_ISOCHRONOUS_OVERHEAD           9
#define USB2_HS_ISOCHRONOUS_OUT_OVERHEAD       38
#define USB2_HS_ISOCHRONOUS_IN_OVERHEAD        18

#define USB2_HS_SS_INTERRUPT_OUT_OVERHEAD      58
#define USB2_HS_CS_INTERRUPT_OUT_OVERHEAD      36
#define USB2_HS_SS_INTERRUPT_IN_OVERHEAD       39
#define USB2_HS_CS_INTERRUPT_IN_OVERHEAD       38

#define USB2_HS_SS_ISOCHRONOUS_OUT_OVERHEAD    58
#define USB2_HS_SS_ISOCHRONOUS_IN_OVERHEAD     39
#define USB2_HS_CS_ISOCHRONOUS_IN_OVERHEAD     38

#define USB2_BIT_STUFFING_OVERHEAD  (8 * (7/6)) // 7.1.9 Bit Stuffing

typedef union _USB2_TT_ENDPOINT_PARAMS {
  struct {
    ULONG TransferType           : 4;
    ULONG Direction              : 1;
    USB_DEVICE_SPEED DeviceSpeed : 2;
    BOOL EndpointMoved           : 1;
    ULONG Reserved               : 24;
  };
  ULONG AsULONG;
} USB2_TT_ENDPOINT_PARAMS;

C_ASSERT(sizeof(USB2_TT_ENDPOINT_PARAMS) == sizeof(ULONG));

typedef union _USB2_TT_ENDPOINT_NUMS {
  struct {
    ULONG NumStarts     : 4;
    ULONG NumCompletes  : 4;
    ULONG Reserved      : 24;
  };
  ULONG AsULONG;
} USB2_TT_ENDPOINT_NUMS;

C_ASSERT(sizeof(USB2_TT_ENDPOINT_NUMS) == sizeof(ULONG));

typedef struct _USB2_TT_ENDPOINT {
  PUSB2_TT Tt;
  PUSBPORT_ENDPOINT Endpoint;
  struct _USB2_TT_ENDPOINT * NextTtEndpoint;
  USB2_TT_ENDPOINT_PARAMS TtEndpointParams;
  USB2_TT_ENDPOINT_NUMS Nums;
  BOOL IsPromoted;
  USHORT MaxPacketSize;
  USHORT PreviosPeriod;
  USHORT Period;
  USHORT ActualPeriod;
  USHORT CalcBusTime;
  USHORT StartTime;
  USHORT Reserved2;
  UCHAR StartFrame;
  UCHAR StartMicroframe;
} USB2_TT_ENDPOINT, *PUSB2_TT_ENDPOINT;

typedef struct _USB2_FRAME_BUDGET {
  PUSB2_TT_ENDPOINT IsoEndpoint;
  PUSB2_TT_ENDPOINT IntEndpoint;
  PUSB2_TT_ENDPOINT AltEndpoint;
  USHORT TimeUsed;
  USHORT Reserved2;
} USB2_FRAME_BUDGET, *PUSB2_FRAME_BUDGET;

typedef struct _USB2_TT {
  PUSB2_HC_EXTENSION HcExtension;
  ULONG DelayTime;
  ULONG MaxTime;
  USB2_TT_ENDPOINT IntEndpoint[USB2_FRAMES];
  USB2_TT_ENDPOINT IsoEndpoint[USB2_FRAMES];
  USB2_FRAME_BUDGET FrameBudget[USB2_FRAMES];
  ULONG NumStartSplits[USB2_FRAMES][USB2_MICROFRAMES];
  ULONG TimeCS[USB2_FRAMES][USB2_MICROFRAMES];
} USB2_TT, *PUSB2_TT;

#define USB2_TT_EXTENSION_FLAG_DELETED  1

typedef struct _USB2_TT_EXTENSION {
  PDEVICE_OBJECT RootHubPdo;
  ULONG Flags;
  ULONG BusBandwidth;
  ULONG Bandwidth[USB2_FRAMES];
  ULONG MaxBandwidth;
  ULONG MinBandwidth;
  USHORT DeviceAddress;
  USHORT TtNumber;
  LIST_ENTRY EndpointList;
  LIST_ENTRY Link;
  USB2_TT Tt;
} USB2_TT_EXTENSION, *PUSB2_TT_EXTENSION;

typedef struct _USB2_HC_EXTENSION {
  ULONG MaxHsBusAllocation;
  ULONG HcDelayTime;
  ULONG TimeUsed[USB2_FRAMES][USB2_MICROFRAMES];
  USB2_TT HcTt;
} USB2_HC_EXTENSION, *PUSB2_HC_EXTENSION;

typedef struct _USB2_REBALANCE {
  PUSB2_TT_ENDPOINT RebalanceEndpoint[USB2_FRAMES - 2];
} USB2_REBALANCE, *PUSB2_REBALANCE;

/* usbport.c */
NTSTATUS
NTAPI
USBPORT_USBDStatusToNtStatus(
  IN PURB Urb,
  IN USBD_STATUS USBDStatus);

NTSTATUS
NTAPI
USBPORT_Wait(
  IN PVOID MiniPortExtension,
  IN ULONG Milliseconds);

VOID
NTAPI
USBPORT_TransferFlushDpc(
  IN PRKDPC Dpc,
  IN PVOID DeferredContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2);

NTSTATUS
NTAPI
USBPORT_CreateWorkerThread(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_StopWorkerThread(
  IN PDEVICE_OBJECT FdoDevice);

BOOLEAN
NTAPI
USBPORT_StartTimer(
  IN PDEVICE_OBJECT FdoDevice,
  IN ULONG Time);

PUSBPORT_COMMON_BUFFER_HEADER
NTAPI
USBPORT_AllocateCommonBuffer(
  IN PDEVICE_OBJECT FdoDevice,
  IN SIZE_T BufferLength);

VOID
NTAPI
USBPORT_FreeCommonBuffer(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_COMMON_BUFFER_HEADER HeaderBuffer);

USBD_STATUS
NTAPI
USBPORT_AllocateTransfer(
  IN PDEVICE_OBJECT FdoDevice,
  IN PURB Urb,
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PIRP Irp,
  IN PRKEVENT Event);

VOID
NTAPI
USBPORT_FlushMapTransfers(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_IsrDpc(
  IN PRKDPC Dpc,
  IN PVOID DeferredContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2);

BOOLEAN
NTAPI
USBPORT_InterruptService(
  IN PKINTERRUPT Interrupt,
  IN PVOID ServiceContext);

VOID
NTAPI
USBPORT_SignalWorkerThread(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_CompleteTransfer(
  IN PURB Urb,
  IN USBD_STATUS TransferStatus);

VOID
NTAPI
USBPORT_DpcHandler(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_WorkerRequestDpc(
  IN PRKDPC Dpc,
  IN PVOID DeferredContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2);

BOOLEAN
NTAPI
USBPORT_QueueDoneTransfer(
  IN PUSBPORT_TRANSFER Transfer,
  IN USBD_STATUS USBDStatus);

VOID
NTAPI
USBPORT_MiniportInterrupts(
  IN PDEVICE_OBJECT FdoDevice,
  IN BOOLEAN IsEnable);

NTSTATUS
NTAPI
USBPORT_SetRegistryKeyValue(
  IN PDEVICE_OBJECT DeviceObject,
  IN BOOL UseDriverKey,
  IN ULONG Type,
  IN PCWSTR ValueNameString,
  IN PVOID Data,
  IN ULONG DataSize);

NTSTATUS
NTAPI
USBPORT_GetRegistryKeyValueFullInfo(
  IN PDEVICE_OBJECT FdoDevice,
  IN PDEVICE_OBJECT PdoDevice,
  IN BOOL UseDriverKey,
  IN PCWSTR SourceString,
  IN ULONG LengthStr,
  IN PVOID Buffer,
  IN ULONG NumberOfBytes);

VOID
NTAPI
USBPORT_AddUSB1Fdo(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_AddUSB2Fdo(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_RemoveUSBxFdo(
  IN PDEVICE_OBJECT FdoDevice);

PDEVICE_OBJECT
NTAPI
USBPORT_FindUSB2Controller(
  IN PDEVICE_OBJECT FdoDevice);

PDEVICE_RELATIONS
NTAPI
USBPORT_FindCompanionControllers(
  IN PDEVICE_OBJECT USB2FdoDevice,
  IN BOOLEAN IsObRefer,
  IN BOOLEAN IsFDOsReturned);

VOID
NTAPI
USBPORT_InvalidateControllerHandler(
  IN PDEVICE_OBJECT FdoDevice,
  IN ULONG Type);

VOID
NTAPI
USBPORT_DoneTransfer(
  IN PUSBPORT_TRANSFER Transfer);

/* debug.c */
ULONG
NTAPI
USBPORT_DbgPrint(
  IN PVOID MiniPortExtension,
  IN ULONG Level,
  IN PCH Format,
  ...);

ULONG
NTAPI
USBPORT_TestDebugBreak(
  IN PVOID MiniPortExtension);

ULONG
NTAPI
USBPORT_AssertFailure(
  PVOID MiniPortExtension,
  PVOID FailedAssertion,
  PVOID FileName,
  ULONG LineNumber,
  PCHAR Message);

VOID
NTAPI
USBPORT_BugCheck(
  IN PVOID MiniPortExtension);

ULONG
NTAPI
USBPORT_LogEntry(
  IN PVOID MiniPortExtension,
  IN ULONG DriverTag,
  IN ULONG EnumTag,
  IN ULONG P1,
  IN ULONG P2,
  IN ULONG P3);

VOID
NTAPI
USBPORT_DumpingDeviceDescriptor(
  IN PUSB_DEVICE_DESCRIPTOR DeviceDescriptor);

VOID
NTAPI
USBPORT_DumpingConfiguration(
  IN PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor);

VOID
NTAPI
USBPORT_DumpingCapabilities(
  IN PDEVICE_CAPABILITIES Capabilities);

VOID
NTAPI
USBPORT_DumpingSetupPacket(
  IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket);

VOID
NTAPI
USBPORT_DumpingURB(
  IN PURB Urb);

VOID
NTAPI
USBPORT_DumpingIDs(
  IN PVOID Buffer);

/* device.c */
NTSTATUS
NTAPI
USBPORT_HandleSelectConfiguration(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp,
  IN PURB Urb);

VOID
NTAPI
USBPORT_AddDeviceHandle(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle);

VOID
NTAPI
USBPORT_RemoveDeviceHandle(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle);

BOOLEAN
NTAPI
USBPORT_ValidateDeviceHandle(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle);

NTSTATUS
NTAPI
USBPORT_CreateDevice(
  IN OUT PUSB_DEVICE_HANDLE *pUsbdDeviceHandle,
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_DEVICE_HANDLE HubDeviceHandle,
  IN USHORT PortStatus,
  IN USHORT Port);

NTSTATUS
NTAPI
USBPORT_InitializeDevice(
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PDEVICE_OBJECT FdoDevice);

NTSTATUS
NTAPI
USBPORT_GetUsbDescriptor(
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PDEVICE_OBJECT FdoDevice,
  IN UCHAR Type,
  IN PUCHAR ConfigDesc,
  IN PULONG ConfigDescSize);

NTSTATUS
NTAPI
USBPORT_HandleSelectInterface(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp,
  IN PURB Urb);

NTSTATUS
NTAPI
USBPORT_RemoveDevice(
  IN PDEVICE_OBJECT FdoDevice,
  IN OUT PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN ULONG Flags);

NTSTATUS
NTAPI
USBPORT_SendSetupPacket(
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSB_DEFAULT_PIPE_SETUP_PACKET SetupPacket,
  IN PVOID Buffer,
  IN ULONG Length,
  IN OUT PULONG TransferedLen,
  IN OUT PUSBD_STATUS pUSBDStatus);

NTSTATUS
NTAPI
USBPORT_RestoreDevice(
  IN PDEVICE_OBJECT FdoDevice,
  IN OUT PUSBPORT_DEVICE_HANDLE OldDeviceHandle,
  IN OUT PUSBPORT_DEVICE_HANDLE NewDeviceHandle);

NTSTATUS
NTAPI
USBPORT_Initialize20Hub(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_DEVICE_HANDLE HubDeviceHandle,
  IN ULONG TtCount);

/* endpoint.c */
NTSTATUS
NTAPI
USBPORT_OpenPipe(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PUSBPORT_PIPE_HANDLE PipeHandle,
  IN PUSBD_STATUS UsbdStatus);

MPSTATUS
NTAPI
MiniportOpenEndpoint(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint);

NTSTATUS
NTAPI
USBPORT_ReopenPipe(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint);

VOID
NTAPI
USBPORT_ClosePipe(
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_PIPE_HANDLE PipeHandle);

VOID
NTAPI
MiniportCloseEndpoint(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint);

VOID
NTAPI
USBPORT_AddPipeHandle(
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PUSBPORT_PIPE_HANDLE PipeHandle);

VOID
NTAPI
USBPORT_RemovePipeHandle(
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PUSBPORT_PIPE_HANDLE PipeHandle);

BOOLEAN
NTAPI
USBPORT_ValidatePipeHandle(
  IN PUSBPORT_DEVICE_HANDLE DeviceHandle,
  IN PUSBPORT_PIPE_HANDLE PipeHandle);

VOID
NTAPI
USBPORT_FlushClosedEndpointList(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_SetEndpointState(
  IN PUSBPORT_ENDPOINT Endpoint,
  IN ULONG State);

ULONG 
NTAPI
USBPORT_GetEndpointState(
  IN PUSBPORT_ENDPOINT Endpoint);

VOID
NTAPI
USBPORT_InvalidateEndpointHandler(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint,
  IN ULONG Type);

BOOLEAN
NTAPI
USBPORT_EndpointWorker(
  IN PUSBPORT_ENDPOINT Endpoint,
  IN BOOLEAN Flag);

VOID
NTAPI
USBPORT_NukeAllEndpoints(
  IN PDEVICE_OBJECT FdoDevice);

BOOLEAN
NTAPI
USBPORT_EndpointHasQueuedTransfers(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint,
  IN PULONG TransferCount);

/* iface.c */
NTSTATUS
NTAPI
USBPORT_PdoQueryInterface(
  IN PDEVICE_OBJECT FdoDevice,
  IN PDEVICE_OBJECT PdoDevice,
  IN PIRP Irp);

/* ioctl.c */
NTSTATUS
NTAPI
USBPORT_PdoDeviceControl(
  PDEVICE_OBJECT PdoDevice,
  PIRP Irp);

NTSTATUS
NTAPI
USBPORT_FdoDeviceControl(
  PDEVICE_OBJECT FdoDevice,
  PIRP Irp);

NTSTATUS
NTAPI
USBPORT_FdoInternalDeviceControl(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp);

NTSTATUS
NTAPI
USBPORT_PdoInternalDeviceControl(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp);

NTSTATUS
NTAPI
USBPORT_GetSymbolicName(
  IN PDEVICE_OBJECT RootHubPdo,
  IN PUNICODE_STRING DestinationString);

/* iso.c */
USBD_STATUS
NTAPI
USBPORT_InitializeIsoTransfer(
  IN PDEVICE_OBJECT FdoDevice,
  IN struct _URB_ISOCH_TRANSFER * Urb,
  IN PUSBPORT_TRANSFER Transfer);

ULONG
NTAPI
USBPORT_CompleteIsoTransfer(
  IN PVOID MiniPortExtension,
  IN PVOID MiniPortEndpoint,
  IN PVOID TransferParameters,
  IN ULONG TransferLength);

/* pnp.c */
NTSTATUS
NTAPI
USBPORT_FdoPnP(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp);

NTSTATUS
NTAPI
USBPORT_PdoPnP(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp);

/* power.c */
NTSTATUS
NTAPI
USBPORT_PdoPower(
  IN PDEVICE_OBJECT PdoDevice,
  IN PIRP Irp);

NTSTATUS
NTAPI
USBPORT_FdoPower(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp);

NTSTATUS
NTAPI
USBPORT_IdleNotification(
  IN PDEVICE_OBJECT PdoDevice,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_AdjustDeviceCapabilities(
  IN PDEVICE_OBJECT FdoDevice,
  IN PDEVICE_OBJECT PdoDevice);

VOID
NTAPI
USBPORT_DoIdleNotificationCallback(
  IN PVOID Context);

VOID
NTAPI
USBPORT_CompletePdoWaitWake(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_DoSetPowerD0(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_HcWakeDpc(
  IN PRKDPC Dpc,
  IN PVOID DeferredContext,
  IN PVOID SystemArgument1,
  IN PVOID SystemArgument2);

VOID
NTAPI
USBPORT_HcQueueWakeDpc(
  IN PDEVICE_OBJECT FdoDevice);

/* queue.c */
VOID
NTAPI
USBPORT_InsertIdleIrp(
  IN PIO_CSQ Csq,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_RemoveIdleIrp(
  IN PIO_CSQ Csq,
  IN PIRP Irp);

PIRP
NTAPI
USBPORT_PeekNextIdleIrp(
  IN PIO_CSQ Csq,
  IN PIRP Irp,
  IN PVOID PeekContext);

VOID
NTAPI
USBPORT_AcquireIdleLock(
  IN PIO_CSQ Csq,
  IN PKIRQL Irql);

VOID
NTAPI
USBPORT_ReleaseIdleLock(
  IN PIO_CSQ Csq,
  IN KIRQL Irql);

VOID
NTAPI
USBPORT_CompleteCanceledIdleIrp(
  IN PIO_CSQ Csq,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_InsertBadRequest(
  IN PIO_CSQ Csq,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_RemoveBadRequest(
  IN PIO_CSQ Csq,
  IN PIRP Irp);

PIRP
NTAPI
USBPORT_PeekNextBadRequest(
  IN PIO_CSQ Csq,
  IN PIRP Irp,
  IN PVOID PeekContext);

VOID
NTAPI
USBPORT_AcquireBadRequestLock(
  IN PIO_CSQ Csq,
  IN PKIRQL Irql);

VOID
NTAPI
USBPORT_ReleaseBadRequestLock(
  IN PIO_CSQ Csq,
  IN KIRQL Irql);

VOID
NTAPI
USBPORT_CompleteCanceledBadRequest(
  IN PIO_CSQ Csq,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_InsertIrpInTable(
  IN PUSBPORT_IRP_TABLE IrpTable,
  IN PIRP Irp);

PIRP
NTAPI
USBPORT_RemovePendingTransferIrp(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp);

PIRP
NTAPI
USBPORT_RemoveActiveTransferIrp(
  IN PDEVICE_OBJECT FdoDevice,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_FindUrbInIrpTable(
  IN PUSBPORT_IRP_TABLE IrpTable,
  IN PURB Urb,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_CancelActiveTransferIrp(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp);

VOID
NTAPI
USBPORT_FlushAbortList(
  IN PUSBPORT_ENDPOINT Endpoint);

VOID
NTAPI
USBPORT_FlushCancelList(
  IN PUSBPORT_ENDPOINT Endpoint);

VOID
NTAPI
USBPORT_QueueTransferUrb(
  IN PURB Urb);

VOID
NTAPI
USBPORT_FlushAllEndpoints(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_FlushPendingTransfers(
  IN PUSBPORT_ENDPOINT Endpoint);

BOOLEAN
NTAPI
USBPORT_QueueActiveUrbToEndpoint(
  IN PUSBPORT_ENDPOINT Endpoint,
  IN PURB Urb);

VOID
NTAPI
USBPORT_FlushController(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_BadRequestFlush(
  IN PDEVICE_OBJECT FdoDevice);

VOID
NTAPI
USBPORT_AbortEndpoint(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint,
  IN PIRP Irp);

/* roothub.c */
VOID
NTAPI
USBPORT_RootHubEndpointWorker(
  PUSBPORT_ENDPOINT Endpoint);

NTSTATUS
NTAPI
USBPORT_RootHubCreateDevice(
  IN PDEVICE_OBJECT FdoDevice,
  IN PDEVICE_OBJECT PdoDevice);

ULONG
NTAPI
USBPORT_InvalidateRootHub(
  PVOID MiniPortExtension);

VOID
NTAPI
USBPORT_RootHubPowerAndChirpAllCcPorts(
  IN PDEVICE_OBJECT FdoDevice);

/* trfsplit.c */
VOID
NTAPI
USBPORT_SplitTransfer(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint,
  IN PUSBPORT_TRANSFER Transfer,
  IN PLIST_ENTRY List);

VOID
NTAPI
USBPORT_DoneSplitTransfer(
  IN PUSBPORT_TRANSFER SplitTransfer);

VOID
NTAPI
USBPORT_CancelSplitTransfer(
  IN PUSBPORT_TRANSFER SplitTransfer);

/* urb.c */
NTSTATUS
NTAPI
USBPORT_HandleSubmitURB(
  IN PDEVICE_OBJECT PdoDevice,
  IN PIRP Irp,
  IN PURB Urb);

/* usb2.c */
BOOLEAN
NTAPI
USBPORT_AllocateBandwidthUSB2(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint);

VOID
NTAPI
USBPORT_FreeBandwidthUSB2(
  IN PDEVICE_OBJECT FdoDevice,
  IN PUSBPORT_ENDPOINT Endpoint);

VOID
NTAPI
USBPORT_UpdateAllocatedBwTt(
  IN PUSB2_TT_EXTENSION TtExtension);

VOID
NTAPI
USB2_InitTT(
  IN PUSB2_HC_EXTENSION HcExtension,
  IN PUSB2_TT Tt);

VOID
NTAPI
USB2_InitController(
  IN PUSB2_HC_EXTENSION HcExtension);

VOID
NTAPI
USBPORT_DumpingEndpointProperties(
  IN PUSBPORT_ENDPOINT_PROPERTIES EndpointProperties);

VOID
NTAPI
USBPORT_DumpingTtEndpoint(
  IN PUSB2_TT_ENDPOINT TtEndpoint);

#endif /* USBPORT_H__ */
