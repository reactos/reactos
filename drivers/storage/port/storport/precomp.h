/*
 * PROJECT:     ReactOS Storport Driver
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Storport driver common header file
 * COPYRIGHT:   Copyright 2017 Eric Kohl (eric.kohl@reactos.org)
 */

#ifndef _STORPORT_PCH_
#define _STORPORT_PCH_

#include <wdm.h>
#include <ntddk.h>
#include <stdio.h>
#include <memory.h>

/* Declare STORPORT_API functions as exports rather than imports */
#define _STORPORT_
#include <storport.h>

#include <ntddscsi.h>
#include <ntdddisk.h>
#include <mountdev.h>
#include <wdmguid.h>

/* Memory Tags */
#define TAG_GLOBAL_DATA     'DGtS'
#define TAG_INIT_DATA       'DItS'
#define TAG_DEVICE_RELATION 'RDtS'
#define TAG_DEVICE_ID       'DItS'
#define TAG_DEVICE_TEXT     'XTtS'
#define TAG_MINIPORT_DATA   'DMtS'
#define TAG_ACCRESS_RANGE   'RAtS'
#define TAG_RESOURCE_LIST   'LRtS'
#define TAG_ADDRESS_MAPPING 'MAtS'
#define TAG_INQUIRY_DATA    'QItS'
#define TAG_REPORT_LUN_DATA 'LRtS'
#define TAG_SENSE_DATA      'NStS'
#define TAG_QUEUED_REQUEST  'RQtS'
#define TAG_SRB_EXTENSION   'XStS'

#define SET_FLAG(Flags, Bit)    ((Flags) |= (Bit))
#define CLEAR_FLAG(Flags, Bit)  ((Flags) &= ~(Bit))
#define TEST_FLAG(Flags, Bit)   (((Flags) & (Bit)) != 0)

typedef enum
{
    dsStopped,
    dsStarted,
    dsPaused,
    dsRemoved,
    dsSurpriseRemoved
} DEVICE_STATE;

typedef enum
{
    InvalidExtension = 0,
    DriverExtension,
    FdoExtension,
    PdoExtension
} EXTENSION_TYPE;

typedef struct _DRIVER_INIT_DATA
{
    LIST_ENTRY Entry;
    HW_INITIALIZATION_DATA HwInitData;
} DRIVER_INIT_DATA, *PDRIVER_INIT_DATA;

typedef struct _DRIVER_OBJECT_EXTENSION
{
    EXTENSION_TYPE ExtensionType;
    PDRIVER_OBJECT DriverObject;

    KSPIN_LOCK AdapterListLock;
    LIST_ENTRY AdapterListHead;
    ULONG AdapterCount;

    LIST_ENTRY InitDataListHead;
} DRIVER_OBJECT_EXTENSION, *PDRIVER_OBJECT_EXTENSION;

typedef struct _MINIPORT_DEVICE_EXTENSION
{
    struct _MINIPORT *Miniport;
    UCHAR HwDeviceExtension[0];
} MINIPORT_DEVICE_EXTENSION, *PMINIPORT_DEVICE_EXTENSION;

typedef struct _MINIPORT
{
    struct _FDO_DEVICE_EXTENSION *DeviceExtension;
    PHW_INITIALIZATION_DATA InitData;
    PORT_CONFIGURATION_INFORMATION PortConfig;
    PMINIPORT_DEVICE_EXTENSION MiniportExtension;
} MINIPORT, *PMINIPORT;

typedef struct _UNIT_DATA
{
    LIST_ENTRY ListEntry;
    INQUIRYDATA InquiryData;
} UNIT_DATA, *PUNIT_DATA;

typedef struct _TIMER_ENTRY
{
    LIST_ENTRY ListEntry;
    KTIMER Timer;
    KDPC TimerCallbackDpc;
    LONG TimerAlreadySet;
    PVOID HwDeviceExtension;
    PHW_TIMER_EX TimerCallback;
    PVOID Context;
} TIMER_ENTRY, *PTIMER_ENTRY;

/*
 * Storport implements some scheduling on how outstanding requests on the PDO are handled.
 *
 * Important facts:
 * A PDO by default can handle (Storport will actually issue that many) 254 outstanding requests;
 * If miniport actually ate that many requests, incoming requests will wait;
 * If miniport thinks a device is overloaded, PDO can be marked Busy/Ready by miniport;
 * A busy PDO device will exit Busy state when completing specified amount (or all) of IOs;
 * The miniport can pause a PDO for a while, for stuff like load balancing;
 * By default Storport uses tagged queuing. If an IO is untagged, it has to wait until all tagged
 *     IOs finish, then Storport will schedule these requests synchronously;
 * If an IO error occured (SCSISTAT_CHECK_CONDITION, SCSISTAT_COMMAND_TERMINATED), a LUN is reset
 *     because of timeout, or a bus reset occured (affects all LUNs on the HBA), the PDO queue is
 *     put into a third halt state known as Frozen;
 * Class driver can use special SRBs to Unfreeze the device queue and clear all requests it held;
 * When class driver asks a device for power functionalities, it puts the device to a fourth holdup
 *     state known as Locked (with special SRB function, unlock as well but with a bypass flag).
 *
 * Access to IO Flow control is synchronized by a lock in FDO flow control, and contains all
 * information about pausing, busy state, freezing, untagged requests and possibly other stuff
 * that affects how Storport schedules IOs sent to the LUN.
 */
typedef struct _PDO_IO_FLOW_CONTROL
{
    // KSPIN_LOCK Lock; // FIXME: Remove

    /*
     * Basic attributes to determine if a request must go waiting. Actually, awaiting request queue
     * should be checked too (if we already got waiting requests, new requests must be served later)
     */
    BOOLEAN IsBusy;
    BOOLEAN IsPaused;
    BOOLEAN IsLocked;
    BOOLEAN IsFrozen;

    /*
     * There are cases when, even the LUN can accept more requests, a new request should still not
     * be issued until all issued requests have completed:
     *
     * 1. A request can have Srb->QueueAction == SRB_ORDERED_QUEUE_TAG_REQUEST which can only be
     * issued after all previously issued and queued requests have completed;
     * 2. An untagged request must wait until all tagged requests complete before it can be issued;
     *
     * Such ordered requests are always issued only after the last active request completes. To
     * accomplish this, we check if the request needs strong ordering before taking it out of
     * approved queue, if it does AND the active list is not empty then this flag is set to TRUE,
     * the command will stay at the tail of the approved list and only be issued when next time the
     * active list is empty.
     */
    BOOLEAN IsWaitingQueueClear;

    /*
     * Ordered tagged & Untagged requests are strong ordered. When we have them in our wait queue,
     * new requests must go waiting. This value is counted by SCSI IRP and completion handler.
     */
    ULONG StrongOrderedCount;

    /* How many active requests are still in the hands of miniport */
    ULONG OutstandingRequestCount;

    /* How many requests needs to be completed before PDO is no longer busy */
    ULONG RemainingBusyRequests;

    /* How many seconds remaining before PDO is no longer paused */
    ULONG RemainingPauseTime;

    /* Requests that PDO and FDO both agreed to send, and already went to the device */
    LIST_ENTRY ActiveRequestsListHead;

    /* Requests that PDO approved to send, awaiting FDO judgement */
    // LIST_ENTRY ApprovedRequestsListHead;

    /* Requests that PDO is too busy to handle, awaiting PDO scheduling */
    LIST_ENTRY AwaitingRequestsListHead;
} PDO_IO_FLOW_CONTROL, *PPDO_IO_FLOW_CONTROL;

/*
 * Storport implements some flow control on FDO.
 *
 * By default FDO has 1000 total outstanding requests limit;
 * Storport will actually issue that many requests (from all LUNs);
 * If miniport thinks the HBA is overloaded, FDO can be marked Busy/Ready by miniport;
 * A busy FDO will exit Busy state when completing specified amount (or all) of IOs;
 * The miniport can pause FDO for a while, for stuff like load balancing;
 *
 * FDO flow control structure has a lock that synchronizes all flow controlling on an HBA.
 */
typedef struct _FDO_IO_FLOW_CONTROL
{
    KSPIN_LOCK Lock;

    BOOLEAN IsBusy;
    BOOLEAN IsPaused;

    /* Active outstanding request count */
    ULONG OutstandingRequestCount;

    ULONG RemainingBusyRequests;
    ULONG RemainingPauseTime;

    /*
     * If FDO blocks a request from being issued, it'll be queued on both the PDO queue and
     * this queue. This is used to implement first-come-first-serve when FDO becomes ready again
     */
    LIST_ENTRY FdoBlockedRequestsListHead;
} FDO_IO_FLOW_CONTROL, *PFDO_IO_FLOW_CONTROL;

typedef struct _FDO_DEVICE_EXTENSION
{
    EXTENSION_TYPE ExtensionType;

    PDEVICE_OBJECT Device;
    PDEVICE_OBJECT LowerDevice;
    PDEVICE_OBJECT PhysicalDevice;
    PDRIVER_OBJECT_EXTENSION DriverExtension;
    DEVICE_STATE PnpState;
    LIST_ENTRY AdapterListEntry;
    MINIPORT Miniport;
    ULONG BusNumber;
    ULONG SlotNumber;
    LONG ScsiPortNumber;
    PCM_RESOURCE_LIST AllocatedResources;
    PCM_RESOURCE_LIST TranslatedResources;
    BUS_INTERFACE_STANDARD BusInterface;
    BOOLEAN BusInitialized;
    PMAPPED_ADDRESS MappedAddressList;
    PVOID UncachedExtensionVirtualBase;
    PHYSICAL_ADDRESS UncachedExtensionPhysicalBase;
    ULONG UncachedExtensionSize;
    PDMA_ADAPTER DmaAdapter;
    PHW_PASSIVE_INITIALIZE_ROUTINE HwPassiveInitRoutine;
    PKINTERRUPT Interrupt;
    ULONG InterruptIrql;

    KSPIN_LOCK PdoListLock;
    LIST_ENTRY PdoListHead;
    ULONG PdoCount;

    /*
     * HBA timers. The first timer, DPC & callback are for StorPortNotification's RequestTimerCall,
     * and the timer list is for StorPortInitializeTimer (implemented in ExtendedFunction), list
     * entry type is TIMER_ENTRY.
     */
    KTIMER Timer;
    KDPC TimerDpc;
    PHW_TIMER TimerCallback;
    KSPIN_LOCK TimerListLock;
    LIST_ENTRY TimerListHead;
    ULONG TimerCount;

    /* Request "flow control" */
    FDO_IO_FLOW_CONTROL FlowControl;

    /* This maximum number is set in port configuration */
    ULONG OutstandingRequestMax;

    /* The requests awaiting completion DPC processing */
    SLIST_HEADER CompletionList;

    /*
     * FIXME: It REALLY should be cached here. The function pointers inside are extremely frequently
     * used but Eric didn't put them here. Need review.
     */
    PHW_INITIALIZATION_DATA HwInitData;
    
} FDO_DEVICE_EXTENSION, *PFDO_DEVICE_EXTENSION;


typedef struct _PDO_DEVICE_EXTENSION
{
    EXTENSION_TYPE ExtensionType;

    PDEVICE_OBJECT Device;
    PFDO_DEVICE_EXTENSION FdoExtension;
    DEVICE_STATE PnpState;
    LIST_ENTRY PdoListEntry;

    ULONG Bus;
    ULONG Target;
    ULONG Lun;
    PINQUIRYDATA InquiryBuffer;

    LONG IsClaimed;

    /* Same as FDO flow control */
    PDO_IO_FLOW_CONTROL FlowControl;
    ULONG OutstandingRequestMax;
    LONG TagCounter;

    LONG SpecialRequestCounter; /* FIXME: DELETE AFTER DEBUG */

} PDO_DEVICE_EXTENSION, *PPDO_DEVICE_EXTENSION;

/* 
 * Storport sends SRBs to HBA, and needs to notify IRPs about IO status.
 * Conventionally SRB has an OriginalRequest field to point to IRP, and IRP tail overlay
 * has got a device queue entry, and that should be enough.
 *
 * But in Storport we also might wanna store more stuff like per request timeout value,
 * and possibly request origin processor to redirect completion DPC to other processors,
 * we use an additional structure here, to store some Storport private per-request data.
 *
 * This structure is put into FDO/PDO request queue. It is referenced by SRB OriginalRequest
 * field, also contains SRB back reference, and source IRP is referenced by this structure.
 *
 * FIXME: Before completing a request, restore OriginalRequest as Classpnp will CHECK it.
 */
typedef struct _QUEUED_REQUEST_REFERENCE
{
    SLIST_ENTRY CompletionEntry;
    LIST_ENTRY FdoEntry;
    LIST_ENTRY PdoEntry;
    PPDO_DEVICE_EXTENSION PdoExtension;
    PSCSI_REQUEST_BLOCK Srb;
    PIRP Irp;
    PSTOR_SCATTER_GATHER_LIST ScatterGatherList;
    PVOID MappedSystemVa;
    ULONG TimeoutCounter; // FIXME: Implement timeout with a heap
    /*
     * Indicates if the current request is outstanding. Some requests are for HBA, and shouldn't be
     * affected by outstanding requests flow controlling.
     * If this field is set to FALSE, this request will not trigger flow control code, flow control
     * lock will not be acquired, and no new request will be scheduled upon its completion.
     */
    BOOLEAN IsOutstanding;
    BOOLEAN StrongOrdered;
    BOOLEAN WriteToDevice;

    /* FIXME: This is solely for debugging and should be removed */
    BOOLEAN DumpSpecialRequest;
    LONG SpecialRequestId;
} QUEUED_REQUEST_REFERENCE, *PQUEUED_REQUEST_REFERENCE;

/* This and next one are for REPORT_LUNS command return structure, seems to be found nowhere else */
typedef struct _LUN_DESCRIPTOR
{
    UCHAR BusId : 6;
    UCHAR AddressMethod : 2;
    UCHAR Level1Address;
    UCHAR Level2Address[2];
    UCHAR Level3Address[2];
    UCHAR Level4Address[2];
} LUN_DESCRIPTOR, *PLUN_DESCRIPTOR;

typedef struct _REPORT_LUNS_DATA
{
    UCHAR LunListLength[4];
    UCHAR Reserved[4];
    LUN_DESCRIPTOR LunDescriptor[1];
} REPORT_LUNS_DATA, *PREPORT_LUNS_DATA;

/* From SCSIport*/
/* we need this to be compatible with ReactOS' classpnp (which is compiled with NTDDI_WIN8) */
typedef struct _STORAGE_ADAPTER_DESCRIPTOR_WIN8 {
    ULONG Version;
    ULONG Size;
    ULONG MaximumTransferLength;
    ULONG MaximumPhysicalPages;
    ULONG AlignmentMask;
    BOOLEAN AdapterUsesPio;
    BOOLEAN AdapterScansDown;
    BOOLEAN CommandQueueing;
    BOOLEAN AcceleratedTransfer;
    UCHAR BusType;
    USHORT BusMajorVersion;
    USHORT BusMinorVersion;
    UCHAR SrbType;
    UCHAR AddressType;
} STORAGE_ADAPTER_DESCRIPTOR_WIN8, *PSTORAGE_ADAPTER_DESCRIPTOR_WIN8;

/* fdo.c */

PPDO_DEVICE_EXTENSION
FdoFindLun(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Bus,
    _In_ ULONG Target,
    _In_ ULONG Lun);

NTSTATUS
NTAPI
PortFdoScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
PortFdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
PortFdoDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);


/* miniport.c */

NTSTATUS
MiniportInitialize(
    _In_ PMINIPORT Miniport,
    _In_ PFDO_DEVICE_EXTENSION DeviceExtension,
    _In_ PHW_INITIALIZATION_DATA HwInitializationData);

NTSTATUS
MiniportFindAdapter(
    _In_ PMINIPORT Miniport);

NTSTATUS
MiniportHwInitialize(
    _In_ PMINIPORT Miniport);

BOOLEAN
MiniportHwInterrupt(
    _In_ PMINIPORT Miniport);

BOOLEAN
MiniportStartIo(
    _In_ PMINIPORT Miniport,
    _In_ PSCSI_REQUEST_BLOCK Srb);

/* misc.c */

NTSTATUS
NTAPI
ForwardIrpAndForget(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp);

INTERFACE_TYPE
GetBusInterface(
    PDEVICE_OBJECT DeviceObject);

PCM_RESOURCE_LIST
CopyResourceList(
    POOL_TYPE PoolType,
    PCM_RESOURCE_LIST Source);

NTSTATUS
QueryBusInterface(
    PDEVICE_OBJECT DeviceObject,
    PGUID Guid,
    USHORT Size,
    USHORT Version,
    PBUS_INTERFACE_STANDARD Interface,
    PVOID InterfaceSpecificData);

BOOLEAN
TranslateResourceListAddress(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    INTERFACE_TYPE BusType,
    ULONG SystemIoBusNumber,
    STOR_PHYSICAL_ADDRESS IoAddress,
    ULONG NumberOfBytes,
    BOOLEAN InIoSpace,
    PPHYSICAL_ADDRESS TranslatedAddress);

NTSTATUS
GetResourceListInterrupt(
    PFDO_DEVICE_EXTENSION DeviceExtension,
    PULONG Vector,
    PKIRQL Irql,
    KINTERRUPT_MODE *InterruptMode,
    PBOOLEAN ShareVector,
    PKAFFINITY Affinity);

NTSTATUS
AllocateAddressMapping(
    PMAPPED_ADDRESS *MappedAddressList,
    STOR_PHYSICAL_ADDRESS IoAddress,
    PVOID MappedAddress,
    ULONG NumberOfBytes,
    ULONG BusNumber);

VOID
NTAPI
SimpleTimerCallbackDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2);

VOID
NTAPI
TimerCallbackDpcRoutine(
    PKDPC Dpc,
    PVOID DeferredContext,
    PVOID SystemArgument1,
    PVOID SystemArgument2
);

/* pdo.c */

NTSTATUS
PortCreatePdo(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Bus,
    _In_ ULONG Target,
    _In_ ULONG Lun,
    _Out_ PPDO_DEVICE_EXTENSION *PdoExtension);

NTSTATUS
PortDeletePdo(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension);

NTSTATUS
PortPdoIssueRequest(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PQUEUED_REQUEST_REFERENCE RequestReference);

BOOLEAN
PortPdoScheduleRequestFlowControlled(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ PQUEUED_REQUEST_REFERENCE RequestReference);

NTSTATUS
NTAPI
PortAllocateResourceForNewRequest(
    _In_ PPDO_DEVICE_EXTENSION PdoExtension,
    _In_ PIRP Irp,
    _In_ PSCSI_REQUEST_BLOCK Srb,
    _In_ ULONG SrbExtensionSize);

NTSTATUS
NTAPI
PortPdoScsi(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
PortPdoPnp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
PortPdoDeviceControl(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp);


/* storport.c */

PHW_INITIALIZATION_DATA
PortGetDriverInitData(
    PDRIVER_OBJECT_EXTENSION DriverExtension,
    INTERFACE_TYPE InterfaceType);

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath);

#endif /* _STORPORT_PCH_ */
