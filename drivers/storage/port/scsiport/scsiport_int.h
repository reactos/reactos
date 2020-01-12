/*
 * SCSI_PORT_TIMER_STATES
 *
 * DESCRIPTION
 *	An enumeration containing the states in the timer DFA
 */

#pragma once

#define VERSION "0.0.3"

#ifndef PAGE_ROUND_UP
#define PAGE_ROUND_UP(x) ( (((ULONG_PTR)x)%PAGE_SIZE) ? ((((ULONG_PTR)x)&(~(PAGE_SIZE-1)))+PAGE_SIZE) : ((ULONG_PTR)x) )
#endif
#ifndef ROUND_UP
#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))
#endif

#define TAG_SCSIPORT 'ISCS'

/* Defines how many logical unit arrays will be in a device extension */
#define LUS_NUMBER 8

#define MAX_SG_LIST 17

/* Flags */
#define SCSI_PORT_DEVICE_BUSY         0x0001
#define SCSI_PORT_LU_ACTIVE           0x0002
#define SCSI_PORT_NOTIFICATION_NEEDED 0x0004
#define SCSI_PORT_NEXT_REQUEST_READY  0x0008
#define SCSI_PORT_FLUSH_ADAPTERS      0x0010
#define SCSI_PORT_MAP_TRANSFER        0x0020
#define SCSI_PORT_RESET               0x0080
#define SCSI_PORT_RESET_REQUEST       0x0100
#define SCSI_PORT_RESET_REPORTED      0x0200
#define SCSI_PORT_REQUEST_PENDING     0x0800
#define SCSI_PORT_DISCONNECT_ALLOWED  0x1000
#define SCSI_PORT_DISABLE_INT_REQUESET 0x2000
#define SCSI_PORT_DISABLE_INTERRUPTS  0x4000
#define SCSI_PORT_ENABLE_INT_REQUEST  0x8000
#define SCSI_PORT_TIMER_NEEDED        0x10000

/* LUN Extension flags*/
#define LUNEX_FROZEN_QUEUE        0x0001
#define LUNEX_NEED_REQUEST_SENSE  0x0004
#define LUNEX_BUSY                0x0008
#define LUNEX_FULL_QUEUE          0x0010
#define LUNEX_REQUEST_PENDING     0x0020
#define SCSI_PORT_SCAN_IN_PROGRESS    0x8000


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

typedef struct _SCSI_PORT_LUN_EXTENSION
{
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;

    ULONG Flags;

    struct _SCSI_PORT_LUN_EXTENSION *Next;

    BOOLEAN DeviceClaimed;
    PDEVICE_OBJECT DeviceObject;

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

    /* More data? */

    UCHAR MiniportLunExtension[1]; /* must be the last entry */
} SCSI_PORT_LUN_EXTENSION, *PSCSI_PORT_LUN_EXTENSION;

/* Structures for inquiries support */

typedef struct _SCSI_LUN_INFO
{
    UCHAR PathId;
    UCHAR TargetId;
    UCHAR Lun;
    BOOLEAN DeviceClaimed;
    PVOID DeviceObject;
    struct _SCSI_LUN_INFO *Next;
    UCHAR InquiryData[INQUIRYDATABUFFERSIZE];
} SCSI_LUN_INFO, *PSCSI_LUN_INFO;

typedef struct _SCSI_BUS_SCAN_INFO
{
    USHORT Length;
    UCHAR LogicalUnitsCount;
    UCHAR BusIdentifier;
    PSCSI_LUN_INFO LunInfo;
} SCSI_BUS_SCAN_INFO, *PSCSI_BUS_SCAN_INFO;

typedef struct _BUSES_CONFIGURATION_INFORMATION
{
    UCHAR NumberOfBuses;
    PSCSI_BUS_SCAN_INFO BusScanInfo[1];
} BUSES_CONFIGURATION_INFORMATION, *PBUSES_CONFIGURATION_INFORMATION;


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
 *	First part of the port objects device extension. The second
 *	part is the miniport-specific device extension.
 */

typedef struct _SCSI_PORT_DEVICE_EXTENSION
{
    ULONG Length;
    ULONG MiniPortExtensionSize;
    PPORT_CONFIGURATION_INFORMATION PortConfig;
    PBUSES_CONFIGURATION_INFORMATION BusesConfig;
    PVOID NonCachedExtension;
    ULONG PortNumber;

    LONG ActiveRequestCounter;
    ULONG SrbFlags;
    ULONG Flags;

    ULONG BusNum;
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
    PSCSI_PORT_LUN_EXTENSION LunExtensionList[LUS_NUMBER];

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

    PDEVICE_OBJECT DeviceObject;
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

    UCHAR MiniPortDeviceExtension[1]; /* must be the last entry */
} SCSI_PORT_DEVICE_EXTENSION, *PSCSI_PORT_DEVICE_EXTENSION;

typedef struct _RESETBUS_PARAMS
{
    ULONG PathId;
    PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
} RESETBUS_PARAMS, *PRESETBUS_PARAMS;
