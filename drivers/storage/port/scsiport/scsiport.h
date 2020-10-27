/*
 * SCSI_PORT_TIMER_STATES
 *
 * DESCRIPTION
 *	An enumeration containing the states in the timer DFA
 */

#pragma once

#include <ntifs.h>
#include <stdio.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntdddisk.h>
#include <mountdev.h>

#ifdef DBG
#include <debug/driverdbg.h>
#endif

#define TAG_SCSIPORT 'ISCS'

/* Defines how many logical unit arrays will be in a device extension */
#define LUS_NUMBER 8

#define MAX_SG_LIST 17

/* Flags */
#define SCSI_PORT_DEVICE_BUSY            0x00001
#define SCSI_PORT_LU_ACTIVE              0x00002
#define SCSI_PORT_NOTIFICATION_NEEDED    0x00004
#define SCSI_PORT_NEXT_REQUEST_READY     0x00008
#define SCSI_PORT_FLUSH_ADAPTERS         0x00010
#define SCSI_PORT_MAP_TRANSFER           0x00020
#define SCSI_PORT_RESET                  0x00080
#define SCSI_PORT_RESET_REQUEST          0x00100
#define SCSI_PORT_RESET_REPORTED         0x00200
#define SCSI_PORT_REQUEST_PENDING        0x00800
#define SCSI_PORT_DISCONNECT_ALLOWED     0x01000
#define SCSI_PORT_DISABLE_INT_REQUESET   0x02000
#define SCSI_PORT_DISABLE_INTERRUPTS     0x04000
#define SCSI_PORT_ENABLE_INT_REQUEST     0x08000
#define SCSI_PORT_TIMER_NEEDED           0x10000

/* LUN Extension flags*/
#define LUNEX_FROZEN_QUEUE               0x0001
#define LUNEX_NEED_REQUEST_SENSE         0x0004
#define LUNEX_BUSY                       0x0008
#define LUNEX_FULL_QUEUE                 0x0010
#define LUNEX_REQUEST_PENDING            0x0020
#define SCSI_PORT_SCAN_IN_PROGRESS       0x8000


typedef enum _SCSI_PORT_TIMER_STATES
{
    IDETimerIdle,
    IDETimerCmdWait,
    IDETimerResetWaitForBusyNegate,
    IDETimerResetWaitForDrdyAssert
} SCSI_PORT_TIMER_STATES;

typedef struct _CONFIGURATION_INFO
{
    /* Identify info */
    ULONG AdapterNumber;
    ULONG LastAdapterNumber;
    ULONG BusNumber;

    /* Registry related */
    HANDLE BusKey;
    HANDLE ServiceKey;
    HANDLE DeviceKey;

    /* Features */
    BOOLEAN DisableTaggedQueueing;
    BOOLEAN DisableMultipleLun;

    /* Parameters */
    PVOID Parameter;
    PACCESS_RANGE AccessRanges;
} CONFIGURATION_INFO, *PCONFIGURATION_INFO;

typedef struct _SCSI_PORT_DEVICE_BASE
{
    LIST_ENTRY List;

    PVOID MappedAddress;
    ULONG NumberOfBytes;
    SCSI_PHYSICAL_ADDRESS IoAddress;
    ULONG SystemIoBusNumber;
} SCSI_PORT_DEVICE_BASE, *PSCSI_PORT_DEVICE_BASE;

typedef struct _SCSI_SG_ADDRESS
{
    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG Length;
} SCSI_SG_ADDRESS, *PSCSI_SG_ADDRESS;

typedef struct _SCSI_REQUEST_BLOCK_INFO
{
    LIST_ENTRY Requests;
    PSCSI_REQUEST_BLOCK Srb;
    PCHAR DataOffset;
    PVOID SaveSenseRequest;

    ULONG SequenceNumber;

    /* DMA stuff */
    PVOID BaseOfMapRegister;
    ULONG NumberOfMapRegisters;

    struct _SCSI_REQUEST_BLOCK_INFO *CompletedRequests;

    /* Scatter-gather list */
    PSCSI_SG_ADDRESS ScatterGather;
    SCSI_SG_ADDRESS ScatterGatherList[MAX_SG_LIST];
} SCSI_REQUEST_BLOCK_INFO, *PSCSI_REQUEST_BLOCK_INFO;

typedef struct _SCSI_PORT_COMMON_EXTENSION
{
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_OBJECT LowerDevice;
    BOOLEAN IsFDO;
} SCSI_PORT_COMMON_EXTENSION, *PSCSI_PORT_COMMON_EXTENSION;

// PDO device
typedef struct _SCSI_PORT_LUN_EXTENSION
{
    SCSI_PORT_COMMON_EXTENSION Common;

    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    ULONG Flags;

    LIST_ENTRY LunEntry;

    BOOLEAN DeviceClaimed;

    INQUIRYDATA InquiryData;

    KDEVICE_QUEUE DeviceQueue;
    ULONG SortKey;
    ULONG QueueCount;
    ULONG MaxQueueCount;

    ULONG AttemptCount;
    LONG RequestTimeout;

    PIRP BusyRequest;
    PIRP PendingRequest;

    struct _SCSI_PORT_LUN_EXTENSION *ReadyLun;
    struct _SCSI_PORT_LUN_EXTENSION *CompletedAbortRequests;

    SCSI_REQUEST_BLOCK_INFO SrbInfo;

    HANDLE RegistryMapKey;

    /* More data? */

    UCHAR MiniportLunExtension[1]; /* must be the last entry */
} SCSI_PORT_LUN_EXTENSION, *PSCSI_PORT_LUN_EXTENSION;

/* Structures for inquiries support */

typedef struct _SCSI_BUS_INFO
{
    LIST_ENTRY LunsListHead;
    UCHAR LogicalUnitsCount;
    UCHAR TargetsCount;
    UCHAR BusIdentifier;
    HANDLE RegistryMapKey;
} SCSI_BUS_INFO, *PSCSI_BUS_INFO;

typedef struct _SCSI_PORT_INTERRUPT_DATA
{
    ULONG Flags; /* Interrupt-time flags */
    PSCSI_REQUEST_BLOCK_INFO CompletedRequests; /* Linked list of Srb info data */
    PSCSI_PORT_LUN_EXTENSION CompletedAbort;
    PSCSI_PORT_LUN_EXTENSION ReadyLun;
    PHW_TIMER HwScsiTimer;
    ULONG MiniportTimerValue;
} SCSI_PORT_INTERRUPT_DATA, *PSCSI_PORT_INTERRUPT_DATA;


/* Only for interrupt data saving function */
typedef struct _SCSI_PORT_SAVE_INTERRUPT
{
    PSCSI_PORT_INTERRUPT_DATA InterruptData;
    struct _SCSI_PORT_DEVICE_EXTENSION *DeviceExtension;
} SCSI_PORT_SAVE_INTERRUPT, *PSCSI_PORT_SAVE_INTERRUPT;

/*
 * SCSI_PORT_DEVICE_EXTENSION
 *
 * DESCRIPTION
 *  First part of the port objects device extension. The second
 *  part is the miniport-specific device extension.
 */

// FDO
typedef struct _SCSI_PORT_DEVICE_EXTENSION
{
    SCSI_PORT_COMMON_EXTENSION Common;

    ULONG Length;
    ULONG MiniPortExtensionSize;
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    PSCSI_BUS_INFO Buses; // children LUNs are stored here
    PVOID NonCachedExtension;
    ULONG PortNumber;

    LONG ActiveRequestCounter;
    ULONG SrbFlags;
    ULONG Flags;

    UCHAR NumberOfBuses;
    ULONG MaxTargedIds;
    ULONG MaxLunCount;

    KSPIN_LOCK IrqLock; /* Used when there are 2 irqs */
    ULONG SequenceNumber; /* Global sequence number for packets */
    KSPIN_LOCK SpinLock;
    PKINTERRUPT Interrupt[2];
    PIRP CurrentIrp;
    ULONG IrpFlags;

    SCSI_PORT_TIMER_STATES TimerState;
    LONG TimerCount;

    KTIMER MiniportTimer;
    KDPC MiniportTimerDpc;

    PMAPPED_ADDRESS MappedAddressList;

    ULONG LunExtensionSize;

    SCSI_PORT_INTERRUPT_DATA InterruptData;

    /* SRB extension stuff*/
    ULONG SrbExtensionSize;
    PVOID SrbExtensionBuffer;
    PVOID FreeSrbExtensions;

    /* SRB information */
    PSCSI_REQUEST_BLOCK_INFO SrbInfo;
    PSCSI_REQUEST_BLOCK_INFO FreeSrbInfo;
    ULONG SrbDataCount;

    IO_SCSI_CAPABILITIES PortCapabilities;

    PCONTROLLER_OBJECT ControllerObject;

    PHW_INITIALIZE HwInitialize;
    PHW_STARTIO HwStartIo;
    PHW_INTERRUPT HwInterrupt;
    PHW_RESET_BUS HwResetBus;
    PHW_DMA_STARTED HwDmaStarted;
    PHW_TIMER HwScsiTimer;

    PSCSI_REQUEST_BLOCK OriginalSrb;
    SCSI_REQUEST_BLOCK InternalSrb;
    SENSE_DATA InternalSenseData;

    /* DMA related stuff */
    PADAPTER_OBJECT AdapterObject;
    ULONG MapRegisterCount;
    BOOLEAN MapBuffers;
    BOOLEAN MapRegisters;
    PVOID MapRegisterBase;

    /* Features */
    BOOLEAN CachesData;
    BOOLEAN SupportsTaggedQueuing;
    BOOLEAN SupportsAutoSense;
    BOOLEAN MultipleReqsPerLun;
    BOOLEAN ReceiveEvent;

    PHYSICAL_ADDRESS PhysicalAddress;
    ULONG CommonBufferLength;
    ULONG InterruptLevel[2];
    ULONG IoAddress;

    BOOLEAN NeedSrbExtensionAlloc;
    BOOLEAN NeedSrbDataAlloc;

    ULONG RequestsNumber;

    ULONG InterruptCount;

    UNICODE_STRING DeviceName;
    UNICODE_STRING InterfaceName;
    BOOLEAN DeviceStarted;
    UINT8 TotalLUCount;

    UCHAR MiniPortDeviceExtension[1]; /* must be the last entry */
} SCSI_PORT_DEVICE_EXTENSION, *PSCSI_PORT_DEVICE_EXTENSION;

typedef struct _RESETBUS_PARAMS
{
    ULONG PathId;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
} RESETBUS_PARAMS, *PRESETBUS_PARAMS;

typedef struct _SCSIPORT_DRIVER_EXTENSION
{
    PDRIVER_OBJECT DriverObject;
    UNICODE_STRING RegistryPath;
    BOOLEAN IsLegacyDriver;
} SCSI_PORT_DRIVER_EXTENSION, *PSCSI_PORT_DRIVER_EXTENSION;

FORCEINLINE
BOOLEAN
VerifyIrpOutBufferSize(
    _In_ PIRP Irp,
    _In_ SIZE_T Size)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    if (ioStack->Parameters.DeviceIoControl.OutputBufferLength < Size)
    {
        Irp->IoStatus.Information = Size;
        return FALSE;
    }
    return TRUE;
}

FORCEINLINE
BOOLEAN
VerifyIrpInBufferSize(
    _In_ PIRP Irp,
    _In_ SIZE_T Size)
{
    PIO_STACK_LOCATION ioStack = IoGetCurrentIrpStackLocation(Irp);
    if (ioStack->Parameters.DeviceIoControl.InputBufferLength < Size)
    {
        Irp->IoStatus.Information = Size;
        return FALSE;
    }
    return TRUE;
}

// ioctl.c

NTSTATUS
NTAPI
ScsiPortDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

// fdo.c

VOID
FdoScanAdapter(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
FdoCallHWInitialize(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
FdoRemoveAdapter(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
FdoStartAdapter(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
FdoDispatchPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp);

// pdo.c

PDEVICE_OBJECT
PdoCreateLunDevice(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

PSCSI_PORT_LUN_EXTENSION
GetLunByPath(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _In_ UCHAR PathId,
    _In_ UCHAR TargetId,
    _In_ UCHAR Lun);

PSCSI_REQUEST_BLOCK_INFO
SpiGetSrbData(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _In_ PSCSI_PORT_LUN_EXTENSION LunExtension,
    _In_ UCHAR QueueTag);

NTSTATUS
PdoDispatchPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp);

// power.c

DRIVER_DISPATCH ScsiPortDispatchPower;

// registry.c

VOID
SpiInitOpenKeys(
    _Inout_ PCONFIGURATION_INFO ConfigInfo,
    _In_ PSCSI_PORT_DRIVER_EXTENSION DriverExtension);

NTSTATUS
RegistryInitAdapterKey(
    _Inout_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

NTSTATUS
RegistryInitLunKey(
    _Inout_ PSCSI_PORT_LUN_EXTENSION LunExtension);

// scsi.c

VOID
SpiGetNextRequestFromLun(
    _In_ PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
    _Inout_ PSCSI_PORT_LUN_EXTENSION LunExtension);

IO_DPC_ROUTINE ScsiPortDpcForIsr;
DRIVER_DISPATCH ScsiPortDispatchScsi;
KSYNCHRONIZE_ROUTINE ScsiPortStartPacket;
DRIVER_STARTIO ScsiPortStartIo;


// scsiport.c

KSERVICE_ROUTINE ScsiPortIsr;

IO_ALLOCATION_ACTION
NTAPI
SpiAdapterControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID Context);

IO_ALLOCATION_ACTION
NTAPI
ScsiPortAllocateAdapterChannel(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PVOID MapRegisterBase,
    _In_ PVOID Context);
