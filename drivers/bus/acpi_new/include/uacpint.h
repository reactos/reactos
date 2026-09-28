/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     uACPI-NT main header
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#include <ntifs.h>
#include <ntstrsafe.h>

#include <uacpi/uacpi.h>
#include <uacpi/namespace.h>
#include <uacpi/utilities.h>
#include <uacpi/resources.h>
#include <uacpi/event.h>
#include <uacpi/sleep.h>
#include <uacpi/status.h>
#include <uacpi/types.h>

#define UACPINT_POOL_TAG    'IpcA'

#define UACPINT_STA_PRESENT      0x00000001u
#define UACPINT_STA_FUNCTIONING  0x00000008u

/* Most power resources a single device references through _PRx or _PRW */
#define UACPINT_MAX_POWER_RESOURCES 8

/* One entry per possible EC query value */
#define UACPINT_EC_QUERY_CODES  256

typedef enum _UACPINT_DEVOBJECT_TYPE
{
    UacpiNtDevObjFdo    = 'FDO_',
    UacpiNtDevObjPdo    = 'PDO_',
    UacpiNtDevObjFilter = 'FLT_',
} UACPINT_DEVOBJECT_TYPE;

typedef struct _UACPINT_SHARED
{
    UACPINT_DEVOBJECT_TYPE Type;
    PDEVICE_OBJECT         Self;
} UACPINT_SHARED, *PUACPINT_SHARED;

/*
 * Translated SCI interrupt taken from START_DEVICE
 */
typedef struct _UACPINT_SCI_RESOURCE
{
    PDEVICE_OBJECT  DeviceObject;   ///< PDO beneath the ACPI FDO
    ULONG           Vector;
    KIRQL           Irql;
    KAFFINITY       Affinity;
    KINTERRUPT_MODE Mode;
    BOOLEAN         ShareVector;
    ULONG           Gsi;            ///< FADT SCI_INT
} UACPINT_SCI_RESOURCE, *PUACPINT_SCI_RESOURCE;

/*
 * ACPI\PNP0C08
 */
typedef struct _UACPINT_FDO
{
    UACPINT_SHARED       Shared;
    PDEVICE_OBJECT       LowerDevice;
    PDEVICE_OBJECT       PhysicalDeviceObject;
    BOOLEAN              Started;
    BOOLEAN              InterpreterReady;
    BOOLEAN              SciFound;
    UACPINT_SCI_RESOURCE Sci;
    FAST_MUTEX           ChildLock;
    LIST_ENTRY           ChildList;         ///< UACPINT_PDO.Link
    LIST_ENTRY           FilterList;        ///< UACPINT_FLT.Link
} UACPINT_FDO, *PUACPINT_FDO;

/*
 * PowerResource object, reference counted by the devices holding it on
 */
typedef struct _UACPINT_POWER_RESOURCE
{
    LIST_ENTRY            Link;
    uacpi_namespace_node *Node;
    USHORT                PowerOnOrder;
    UCHAR                 SleepLevel;
    LONG                  RefCount;
    BOOLEAN               On;
} UACPINT_POWER_RESOURCE, *PUACPINT_POWER_RESOURCE;

/*
 * Thermal zone data, the superset of every release's query layout.
 * Temperatures are in tenths of a Kelvin.
 */
typedef struct _UACPINT_THERMAL_INFO
{
    ULONG     ThermalStamp;
    ULONG     ThermalConstant1;
    ULONG     ThermalConstant2;
    ULONG     SamplingPeriod;
    ULONG     CurrentTemperature;
    ULONG     PassiveTripPoint;
    ULONG     StandbyTripPoint;
    ULONG     CriticalTripPoint;
    ULONG     S4TripPoint;
    ULONG     ActiveTripPointCount;
    ULONG     ActiveTripPoint[10];
    ULONG     MinimumThrottle;
    ULONG     OverThrottleThreshold;
    KAFFINITY Processors;
} UACPINT_THERMAL_INFO, *PUACPINT_THERMAL_INFO;

/*
 * Thermal zone state. Lock guards the queue, the work and flag fields,
 * the request values and Info.ThermalStamp. The worker owns the rest.
 */
typedef struct _UACPINT_THERMAL_ZONE
{
    KSPIN_LOCK           Lock;
    LIST_ENTRY           IrpQueue;
    ULONG                Work;
    BOOLEAN              Active;
    BOOLEAN              Running;
    BOOLEAN              Delivered;
    BOOLEAN              DtiArmed;
    UCHAR                CoolingPolicy;
    UCHAR                FanLevel;
    UCHAR                ThrottleLimit;
    ULONG                TripDelta;
    ULONG                NotifiedTemperature;
    ULONG                DsmFunctions;
    UNICODE_STRING       Description;
    UACPINT_THERMAL_INFO Info;
} UACPINT_THERMAL_ZONE, *PUACPINT_THERMAL_ZONE;

/*
 * WAIT_WAKE and _PRW GPE state. Shared by PDOs and filters so the same
 * arming code covers foreign devices too. Lock guards the cancel race.
 */
typedef struct _UACPINT_WAKE
{
    uacpi_namespace_node    *Node;
    CHAR                     Name[8];
    KSPIN_LOCK               Lock;
    PIRP                     WaitWakeIrp;
    uacpi_namespace_node    *GpeDevice;         ///< NULL means \_GPE
    UCHAR                    GpeLine;
    BOOLEAN                  GpeParsed;
    BOOLEAN                  GpeValid;
    SYSTEM_POWER_STATE       SystemWake;        ///< deepest wake S state from _PRW
    DEVICE_POWER_STATE       DeviceWake;        ///< D state passed to _DSW
    SYSTEM_POWER_STATE       RequestedSystemState;
    BOOLEAN                  PswParsed;
    BOOLEAN                  HasDsw;
    BOOLEAN                  HasPsw;
    BOOLEAN                  SetupDone;
    BOOLEAN                  Armed;
    BOOLEAN                  DepthSuspended;
    WORK_QUEUE_ITEM          DisarmWork;
    volatile LONG            DisarmQueued;
    PUACPINT_POWER_RESOURCE  WakeResources[UACPINT_MAX_POWER_RESOURCES];
    ULONG                    WakeResourceCount;
} UACPINT_WAKE, *PUACPINT_WAKE;

/*
 * EC query handler registration. The caller hands this layout in through
 * a METHOD_NEITHER IOCTL on the PNP0C09 PDO, so it must not change.
 */
typedef
VOID
(NTAPI *PUACPINT_EC_QUERY_ROUTINE)(
    _In_ ULONG QueryCode,
    _In_opt_ PVOID Context);

typedef struct _UACPINT_EC_QUERY_REGISTRATION
{
    UCHAR     QueryCode;
    PVOID     Handler;
    PVOID     Context;
    ULONG_PTR Cookie;
} UACPINT_EC_QUERY_REGISTRATION, *PUACPINT_EC_QUERY_REGISTRATION;

#define IOCTL_UACPINT_EC_REGISTER_QUERY_HANDLER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 5, METHOD_NEITHER, FILE_ANY_ACCESS)

#define IOCTL_UACPINT_EC_UNREGISTER_QUERY_HANDLER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 6, METHOD_NEITHER, FILE_ANY_ACCESS)

/*
 * Embedded controller. The boot EC is brought up before the namespace is
 * initialized and its PNP0C09 PDO adopts it at start.
 */
typedef struct _UACPINT_EC
{
    struct _UACPINT_EC        *Next;
    uacpi_namespace_node      *Node;
    PVOID                      Pdo;             ///< PUACPINT_PDO once started
    USHORT                     DataPort;
    USHORT                     ControlPort;
    uacpi_namespace_node      *GpeDevice;       ///< NULL means \_GPE
    uacpi_u16                  GpeLine;
    BOOLEAN                    RegionInstalled;
    BOOLEAN                    FromEcdt;
    BOOLEAN                    GlobalLock;      ///< _GLK
    FAST_MUTEX                 Lock;
    KSPIN_LOCK                 QueryLock;
    PUACPINT_EC_QUERY_ROUTINE  QueryRoutine[UACPINT_EC_QUERY_CODES];
    PVOID                      QueryContext[UACPINT_EC_QUERY_CODES];
    WORK_QUEUE_ITEM            Work;
    KDPC                       Dpc;
    LONG                       WorkQueued;
} UACPINT_EC, *PUACPINT_EC;

/*
 * ACPI Enumerated Device
 */
typedef struct _UACPINT_PDO
{
    UACPINT_SHARED          Shared;
    PUACPINT_FDO            Parent;
    LIST_ENTRY              Link;
    uacpi_namespace_node   *Node;
    uacpi_namespace_node   *ScopeNode;
    BOOLEAN                 Present;
    BOOLEAN                 Reported;
    BOOLEAN                 Started;
    BOOLEAN                 HasAdr;
    BOOLEAN                 IsThermalZone;
    BOOLEAN                 IsProcessor;       ///< legacy Processor() object
    ULONG64                 Adr;
    CHAR                    Hid[16];
    CHAR                    Name[8];
    CHAR                    Instance[64];
    UCHAR                   PciRootBaseBus;    ///< cached _BBN

    /* PCI roots only */
    struct
    {
        BOOLEAN OscEvaluated;
        ULONG   OscControlGranted;
        BOOLEAN DsmEvaluated;
        BOOLEAN BusCapsFound;
        ULONG   CurrentSpeedAndMode;
        ULONG   SupportedSpeedsAndModes;
        ULONG   BusCapAttributes;
        BOOLEAN IsExpress;
    } HostBridge;

    PDEVICE_NOTIFY_CALLBACK NotifyRoutine;
    PVOID                   NotifyContext;

    UACPINT_WAKE            Wake;

    /* Cached S to D map, QUERY_CAPABILITIES repeats often */
    BOOLEAN                 CapsMapDone;
    DEVICE_CAPABILITIES     CapsMap;

    /* SYS_BUTTON state, ButtonCaps != 0 marks a button */
    ULONG                   ButtonCaps;
    ULONG                   ButtonEvents;
    KSPIN_LOCK              ButtonLock;
    LIST_ENTRY              ButtonIrpQueue;
    KDPC                    ButtonDpc;
    volatile LONG           ButtonDeferred;
    UNICODE_STRING          ButtonSymLink;
    BOOLEAN                 ButtonIfRegistered;

    /* Lid close policy from \Callback\PowerState */
    PCALLBACK_OBJECT        LidPowerCallback;
    PVOID                   LidPowerCallbackHandle;
    BOOLEAN                 LidCloseNoAction;
    BOOLEAN                 LidInitialReported;

    /* Class device interfaces */
    UNICODE_STRING          ClassIfLink;
    BOOLEAN                 ClassIfOn;
    UNICODE_STRING          CoolingIfLink;
    BOOLEAN                 CoolingIfOn;
    PVOID                   ThermalWmi;        ///< PWMILIB_CONTEXT
    UACPINT_THERMAL_ZONE    Thermal;
    PVOID                   ResArb;            ///< resarb.c private
    PUACPINT_EC             Ec;

    /* Power resources held for CurrentDState */
    PUACPINT_POWER_RESOURCE HeldResources[UACPINT_MAX_POWER_RESOURCES];
    ULONG                   HeldCount;
    DEVICE_POWER_STATE      CurrentDState;
    BOOLEAN                 DStateKnown;

    /* Paging, hibernate and dump usage, nonzero blocks disable */
    LONG                    UsageCount;
} UACPINT_PDO, *PUACPINT_PDO;

/*
 * ACPI Filter Device Object
 */
typedef struct _UACPINT_FLT
{
    UACPINT_SHARED          Shared;
    PUACPINT_FDO            Fdo;
    LIST_ENTRY              Link;
    uacpi_namespace_node   *Node;
    PDEVICE_OBJECT          ForeignPdo;
    PDEVICE_OBJECT          LowerDevice;
    LONG                    PagingCount;
    LONG                    HibernationCount;
    LONG                    DumpCount;
    PDEVICE_NOTIFY_CALLBACK NotifyRoutine;
    PVOID                   NotifyContext;
    UACPINT_WAKE            Wake;
} UACPINT_FLT, *PUACPINT_FLT;

extern PUACPINT_FDO GlobalAcpiFdo;
extern PDRIVER_OBJECT GlobalAcpiDriverObj;
extern UNICODE_STRING GlobalDriverRegPath;

/* 0 = PIC, 1 = APIC, from the HAL PM handshake */
extern ULONG GlobalAcpiInterruptModel;

/* "<VendorIdentifier> - <Identifier>" from CentralProcessor\0, empty if unknown */
extern CHAR UacpiNtProcessorString[128];

/* CPUID brand string, only used on Windows 10 and later */
extern CHAR UacpiNtProcessorBrand[64];

/* Registry tunables from Services\ACPI\Parameters */
extern ULONG UacpiNtEnumDiagEnabled;
extern ULONG UacpiNtEnumDiagDelaySeconds;
extern ULONG UacpiNtIrqArbEnabled;
extern ULONG UacpiNtIrqArbVerbose;
extern ULONG UacpiNtIrqLibHalOverrides;
extern ULONG UacpiNtMsiDiagEnabled;
extern ULONG UacpiNtMsiDiagDelaySeconds;
extern ULONG UacpiNtResArbEnabled;
extern ULONG UacpiNtResVerbose;
extern ULONG UacpiNtHostVerbose;

/* Nonzero from the sleep IRP until \_WAK, a fixed button press here is the wake press */
extern volatile LONG UacpiNtSystemResuming;

/* First WAIT_WAKE completed in the resume window claims the system wake source */
extern volatile LONG UacpiNtWakeSourceClaimed;

/* guid.c */
extern const GUID GUID_DEVICE_SYS_BUTTON;
extern const GUID GUID_DEVICE_THERMAL_ZONE;
extern const GUID GUID_DEVICE_PROCESSOR;
extern const GUID GUID_DEVICE_FAN;
extern const GUID GUID_DEVINTERFACE_THERMAL_COOLING;
extern const GUID MSAcpi_ThermalZoneTemperature_GUID;
extern const GUID GUID_DEVICE_APPLICATIONLAUNCH_BUTTON;
extern const GUID UacpiNtSbOscUuid;
#if (NTDDI_VERSION < NTDDI_WIN8)
extern const GUID GUID_BUS_TYPE_ACPI;
#endif
extern const GUID UacpiNtPciRootOscUuid;
extern const GUID UacpiNtPciRootDsmUuid;

/* driver.c */

DRIVER_INITIALIZE DriverEntry;

NTSTATUS
NTAPI
UacpiNtCompleteIrp(
    _In_ PIRP Irp,
    _In_ NTSTATUS Status,
    _In_ ULONG_PTR Information);

NTSTATUS
NTAPI
UacpiNtForwardAndForget(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp);

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UacpiNtForwardAndWait(
    _In_ PDEVICE_OBJECT LowerDevice,
    _In_ PIRP Irp);

/* glue.c */

NTSTATUS
NTAPI
UacpiNtBringUpInterpreter(
    _In_ PUACPINT_FDO Fdo);

VOID
NTAPI
UacpiNtTearDownInterpreter(
    _In_ PUACPINT_FDO Fdo);

/* Public API version of uACPI's internal uacpi_clear_all_events */
uacpi_status
NTAPI
UacpiNtClearAllEvents(VOID);

/* enum.c */

NTSTATUS
NTAPI
UacpiNtEnumerateNamespace(
    _In_ PUACPINT_FDO Fdo);

NTSTATUS
NTAPI
UacpiNtBuildBusRelations(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
UacpiNtPdoQueryId(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp);

VOID
NTAPI
UacpiNtBuildChildPdosForNode(
    _In_ PUACPINT_FDO Fdo,
    _In_opt_ uacpi_namespace_node *Parent);

NTSTATUS
NTAPI
UacpiNtMergeChildRelations(
    _In_ PUACPINT_FDO Fdo,
    _In_opt_ uacpi_namespace_node *Parent,
    _In_opt_ uacpi_namespace_node *SecondParent,
    _In_ PIRP Irp);

BOOLEAN
NTAPI
UacpiNtNodeIsPresent(
    _In_ uacpi_namespace_node *Node);

BOOLEAN
NTAPI
UacpiNtGetProcessorInfo(
    _In_ uacpi_namespace_node *Node,
    _Out_ uacpi_processor_info *Info);

BOOLEAN
NTAPI
UacpiNtHidIsPciRoot(
    _In_z_ const char *Hid);

BOOLEAN
NTAPI
UacpiNtNodeWillBecomePdo(
    _In_ uacpi_namespace_node *Node);

VOID
NTAPI
UacpiNtEnumDiagDump(
    _In_ PUACPINT_FDO Fdo,
    _In_ BOOLEAN Settled);

VOID
NTAPI
UacpiNtEnumDiagArm(
    _In_opt_ PUACPINT_FDO Fdo);

/* filter.c */

NTSTATUS
NTAPI
UacpiNtDetectFilterDevices(
    _In_opt_ PUACPINT_FDO Fdo,
    _In_opt_ uacpi_namespace_node *Parent,
    _In_opt_ PDEVICE_RELATIONS Relations);

NTSTATUS
NTAPI
UacpiNtFilterPnp(
    _In_ PUACPINT_FLT Filter,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
UacpiNtFilterPower(
    _In_ PUACPINT_FLT Filter,
    _In_ PIRP Irp);

/* resource.c */

NTSTATUS
NTAPI
UacpiNtCrsToCmList(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_ PCM_RESOURCE_LIST *ResourceList);

ULONG
NTAPI
UacpiNtCrsEmitConnectionsCm(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_opt_ PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptors);

NTSTATUS
NTAPI
UacpiNtPrsToRequirements(
    _In_ uacpi_namespace_node *Node,
    _In_ BOOLEAN Possible,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Requirements);

NTSTATUS
NTAPI
UacpiNtCrsToRequirements(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_ PIO_RESOURCE_REQUIREMENTS_LIST *Requirements);

ULONG
NTAPI
UacpiNtCrsConnectionCount(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject);

ULONG
NTAPI
UacpiNtCrsEmitConnections(
    _In_ uacpi_namespace_node *Node,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _Out_opt_ PIO_RESOURCE_DESCRIPTOR Descriptors);

/* reshub.c */

NTSTATUS
NTAPI
UacpiNtConnectResourceHub(VOID);

NTSTATUS
NTAPI
UacpiNtAddBiosNameDeviceAssociation(
    _In_ PCUNICODE_STRING ReferenceName,
    _In_ PDEVICE_OBJECT DeviceObject);

VOID
NTAPI
UacpiNtRegisterBiosNameForPdo(
    _In_ PUACPINT_PDO Pdo);

NTSTATUS
NTAPI
UacpiNtTranslateConnectionDescriptor(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_reads_bytes_(DescriptorLength) PVOID Descriptor,
    _In_ ULONG DescriptorLength,
    _Out_ PIO_RESOURCE_DESCRIPTOR IoDescriptor);

/* interfaces.c */

NTSTATUS
NTAPI
UacpiNtPdoQueryInterface(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp);

PUACPINT_PDO
NTAPI
UacpiNtFindPdoByNode(
    _In_opt_ PUACPINT_FDO Fdo,
    _In_ uacpi_namespace_node *Node);

VOID
NTAPI
UacpiNtRouteNotify(
    _In_ uacpi_namespace_node *Node,
    _In_ ULONG Value);

NTSTATUS
NTAPI
UacpiNtBuildIrqTranslator(
    _In_z_ const char *TraceName,
    _In_ PIO_STACK_LOCATION IoStack);

NTSTATUS
NTAPI
UacpiNtBuildDeviceResetInterface(
    _In_ uacpi_namespace_node *Node,
    _In_z_ const char *TraceName,
    _In_ PIO_STACK_LOCATION IoStack);

NTSTATUS
NTAPI
UacpiNtBuildAcpiInterface(
    _In_ PDEVICE_OBJECT Context,
    _In_z_ const char *TraceName,
    _In_ PIO_STACK_LOCATION IoStack);

VOID
NTAPI
UacpiNtPlatformOscNegotiate(VOID);

/* ioctl.c */

NTSTATUS
NTAPI
UacpiNtPdoDeviceControl(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
UacpiNtFilterDeviceControl(
    _In_ PUACPINT_FLT Filter,
    _In_ PIRP Irp);

/* irqarb.c */

NTSTATUS
NTAPI
UacpiNtIrqArbiterInitialize(
    _In_ PUACPINT_FDO Fdo);

NTSTATUS
NTAPI
UacpiNtQueryIrqArbiter(
    _In_ PIO_STACK_LOCATION IoStack);

BOOLEAN
NTAPI
UacpiNtIrqLinkDecide(
    _In_ uacpi_namespace_node *Node,
    _Out_ PULONG Gsiv);

VOID
NTAPI
UacpiNtIrqLinksResume(VOID);

ULONG
NTAPI
UacpiNtReadPciClassCode(
    _In_ PDEVICE_OBJECT Pdo);

VOID
NTAPI
UacpiNtMsiDiagArm(VOID);

VOID
NTAPI
UacpiNtMsiDiagDisarm(VOID);

/* resarb.c */

NTSTATUS
NTAPI
UacpiNtQueryResArbiter(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIO_STACK_LOCATION IoStack);

VOID
NTAPI
UacpiNtResArbiterTeardown(
    _In_ PUACPINT_PDO Pdo);

/* hal.c */

NTSTATUS
NTAPI
UacpiNtHalPmHandshake(VOID);

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UacpiNtHalRefreshWakeCache(VOID);

ULONG
NTAPI
UacpiNtHalPciReadConfig(
    _In_opt_ PVOID Context,
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length);

ULONG
NTAPI
UacpiNtHalPciWriteConfig(
    _In_opt_ PVOID Context,
    _In_ ULONG Bus,
    _In_ ULONG Slot,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ ULONG Offset,
    _In_ ULONG Length);

/* irqlib.c */

VOID
NTAPI
UacpiNtIrqLibSetHalVectors(
    _In_opt_ PCM_RESOURCE_LIST Resources);

NTSTATUS
NTAPI
UacpiNtIrqLibInitialize(VOID);

NTSTATUS
NTAPI
UacpiNtIrqLibResolveVector(
    _In_ ULONG Gsiv,
    _Out_ PULONG Vector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity,
    _Out_ PULONG Polarity,
    _Out_ PULONG Mode);

NTSTATUS
NTAPI
UacpiNtIrqLibWriteConnectionData(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ ULONG Gsiv,
    _In_ ULONG Count);

NTSTATUS
NTAPI
UacpiNtIrqLibResolveMessageVector(
    _In_ PVOID Owner,
    _In_ ULONG MessageGsiv,
    _In_ ULONG Count,
    _Out_ PULONG BaseVector,
    _Out_ PKIRQL Irql,
    _Out_ PKAFFINITY Affinity);

VOID
NTAPI
UacpiNtIrqLibNoteLevelGsiv(
    _In_ ULONG Gsiv);

VOID
NTAPI
UacpiNtIrqLibNoteEdgeGsiv(
    _In_ ULONG Gsiv);

BOOLEAN
NTAPI
UacpiNtIrqLibGsivForcedEdge(
    _In_ ULONG Gsiv);

/* sleep.c */

NTSTATUS
NTAPI
UacpiNtSystemSetPower(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp);

/* power.c */

BOOLEAN
NTAPI
UacpiNtNodeHasChild(
    _In_ uacpi_namespace_node *Node,
    _In_z_ const char *Name);

NTSTATUS
NTAPI
UacpiNtPowerSetDeviceState(
    _In_ PUACPINT_PDO Pdo,
    _In_ DEVICE_POWER_STATE DeviceState);

VOID
NTAPI
UacpiNtPowerBuildStateMap(
    _In_ PUACPINT_PDO Pdo,
    _Inout_ PDEVICE_CAPABILITIES Capabilities);

VOID
NTAPI
UacpiNtPowerMergeWakeCaps(
    _In_ uacpi_namespace_node *Node,
    _Inout_ PDEVICE_CAPABILITIES Capabilities);

NTSTATUS
NTAPI
UacpiNtPdoSetPower(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp);

VOID
NTAPI
UacpiNtWakeInit(
    _Out_ PUACPINT_WAKE Wake,
    _In_ uacpi_namespace_node *Node);

VOID
NTAPI
UacpiNtWakeReArmAfterHibernate(VOID);

VOID
NTAPI
UacpiNtWakeSuspendShallow(
    _In_ SYSTEM_POWER_STATE Target);

VOID
NTAPI
UacpiNtWakeRestoreSuspended(VOID);

BOOLEAN
NTAPI
UacpiNtWakeHasPrw(
    _Inout_ PUACPINT_WAKE Wake);

NTSTATUS
NTAPI
UacpiNtWakeArm(
    _Inout_ PUACPINT_WAKE Wake,
    _In_ PIRP Irp);

VOID
NTAPI
UacpiNtWakeComplete(
    _Inout_ PUACPINT_WAKE Wake);

VOID
NTAPI
UacpiNtWakeTeardown(
    _Inout_ PUACPINT_WAKE Wake);

/* powerres.c */

VOID
NTAPI
UacpiNtPowerResInit(VOID);

VOID
NTAPI
UacpiNtPowerResResume(VOID);

VOID
NTAPI
UacpiNtPowerResAcquireForState(
    _In_ PUACPINT_PDO Pdo,
    _In_ DEVICE_POWER_STATE DeviceState,
    _Out_writes_to_(UACPINT_MAX_POWER_RESOURCES, *NewCount) PUACPINT_POWER_RESOURCE *NewList,
    _Out_ PULONG NewCount);

VOID
NTAPI
UacpiNtPowerResReleaseDelta(
    _In_ PUACPINT_PDO Pdo,
    _In_reads_(NewCount) PUACPINT_POWER_RESOURCE *NewList,
    _In_ ULONG NewCount);

ULONG
NTAPI
UacpiNtPowerResAcquireWake(
    _In_ uacpi_namespace_node *Device,
    _Out_writes_to_(MaxCount, return) PUACPINT_POWER_RESOURCE *List,
    _In_ ULONG MaxCount);

VOID
NTAPI
UacpiNtPowerResReleaseWake(
    _In_reads_(Count) PUACPINT_POWER_RESOURCE *List,
    _In_ ULONG Count);

/* drivers/button.c */

BOOLEAN
NTAPI
UacpiNtButtonClassify(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtButtonStart(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtButtonArmWaitWake(
    _In_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtButtonDeliverEvent(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG Event);

VOID
NTAPI
UacpiNtFixedButtonInit(
    _In_ PUACPINT_FDO Fdo);

VOID
NTAPI
UacpiNtButtonRemove(
    _Inout_ PUACPINT_PDO Pdo);

NTSTATUS
NTAPI
UacpiNtButtonDeviceControl(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled);

VOID
NTAPI
UacpiNtButtonNotify(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG NotifyValue);

/* drivers/devif.c */

VOID
NTAPI
UacpiNtDevIfRegister(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ const GUID *InterfaceGuid,
    _In_z_ const char *Label);

VOID
NTAPI
UacpiNtCoolingIfRegister(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtDevIfRemove(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtDriversStartDevice(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtDriversRemoveDevice(
    _Inout_ PUACPINT_PDO Pdo);

/* drivers/processor.c, thermal.c, fan.c, applaunch.c */

VOID
NTAPI
UacpiNtProcessorStart(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtThermalStart(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtThermalRemove(
    _Inout_ PUACPINT_PDO Pdo);

NTSTATUS
NTAPI
UacpiNtThermalSystemControl(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled);

NTSTATUS
NTAPI
UacpiNtThermalDeviceControl(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled);

VOID
NTAPI
UacpiNtThermalNotify(
    _Inout_ PUACPINT_PDO Pdo,
    _In_ ULONG NotifyValue);

VOID
NTAPI
UacpiNtFanStart(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtApplaunchStart(
    _Inout_ PUACPINT_PDO Pdo);

/* drivers/ec.c */

VOID
NTAPI
UacpiNtEcInitialize(VOID);

VOID
NTAPI
UacpiNtEcStart(
    _Inout_ PUACPINT_PDO Pdo);

VOID
NTAPI
UacpiNtEcRemove(
    _Inout_ PUACPINT_PDO Pdo);

NTSTATUS
NTAPI
UacpiNtEcReadWrite(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
UacpiNtEcDeviceControl(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp,
    _Out_ PBOOLEAN Handled);

BOOLEAN
NTAPI
UacpiNtEcIsEcNode(
    _In_ uacpi_namespace_node *Node);

/* fdo.c, pdo.c */

NTSTATUS
NTAPI
UacpiNtFdoPnp(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
UacpiNtFdoPower(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
UacpiNtPdoPnp(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp);

NTSTATUS
NTAPI
UacpiNtPdoPower(
    _In_ PUACPINT_PDO Pdo,
    _In_ PIRP Irp);

/* uacpi-host/uacpi_host.c, uacpi_isr.c */

VOID
NTAPI
UacpiNtHostSetSciResource(
    _In_ PUACPINT_SCI_RESOURCE Resource);

NTSTATUS
NTAPI
UacpiNtHostConnectSci(VOID);

VOID
NTAPI
UacpiNtHostRsdpOverride(
    _In_ PHYSICAL_ADDRESS RsdpPhysical);

#include <uacpintreg.h>
#include <uacpihal.h>
