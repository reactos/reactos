/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Internal header for the Power Manager
 * COPYRIGHT:   Copyright 2006 Alex Ionescu <alex.ionescu@reactos.org>
 *              Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

#include <guiddef.h>
#include <poclass.h>

//
// Power Manager Dependencies
//
#include "pep.h"
#include "pofx.h"
#include "ppm.h"

//
// Define this if you want debugging support
//
#define _PO_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define PO_STATE_DEBUG                                  0x01
#define PO_HIBER_DEBUG                                  0x02
#define PO_THERMAL_DEBUG                                0x04
#define PO_THROTTLE_DEBUG                               0x06
#define PO_POWER_ACTION_DEBUG                           0x08
#define PO_VOLUME_DOPE_DEBUG                            0x10
#define PO_BATTERY_MGR_DEBUG                            0x12
#define PO_IRP_DEBUG                                    0x14
#define PO_NOTIFY_DEBUG                                 0x16
#define PO_SHUTDOWN_DEBUG                               0x18
#define PO_POLICY_DEBUG                                 0x20
#define PO_IDLE_STATE_DEBUG                             0x40
#define PO_NT_SYSCALL_DEBUG                             0x60
#define PO_MISC_DEBUG                                   0x80
#define PO_CONTROL_SWITCH_DEBUG                         0x100
#define PO_INIT_SUBSYSTEM_DEBUG                         0x900

//
// Debug/Tracing support
//
#if _PO_DEBUG_
#ifdef NEW_DEBUG_SYSTEM_IMPLEMENTED // enable when Debug Filters are implemented
#define POTRACE DbgPrintEx
#else
#define POTRACE(x, ...)                                 \
    if (x & PopTraceLevel) DbgPrint(__VA_ARGS__)
#endif
#else
#define POTRACE(x, fmt, ...) DPRINT(fmt, ##__VA_ARGS__)
#endif

//
// Internal bugcheck code reasons (for INTERNAL_POWER_ERROR)
//
#define POP_PO_INIT_FAILURE                 1
#define POP_IDLE_DETECT_UNKNOWN_DEVICE      2
#define POP_DEVICE_POLICY_IRP_ALLOC_FAILED  3
#define POP_INVALID_CONTROL_SWITCH_MODE     4
#define POP_BATTERY_UNKNOWN_MODE_REQUEST    5

/******************************************************************************
 *                                      irp.c                                 *
 ******************************************************************************/

//
// Device Power Failure Triage (0x9F) Signature
//
#define POP_9F_TRIAGE_SIGNATURE             0x8000

//
// Device Power Failure Triage (0x9F) Revision
//
#define POP_9F_TRIAGE_REVISION_V1           1

//
// IRP watchdog duetime in seconds (600 s = 10 min)
//
#define POP_IRP_WATCHDOG_DUETIME            60 * 10

//
// Maximum number of IRP dispatch worker threads the system can create
//
#define POP_MAX_IRP_WORKERS_COUNT           10

//
// Maximum number of IRPs that can be queued
//
#define POP_MAX_IRP_QUEUE_LIST              100
#define POP_MAX_INRUSH_IRP_QUEUE_LIST       60

//
// IRP worker system thread priority
//
#define POP_WORKER_THREAD_PRIORITY          2

//
// PEXTENDED_DEVOBJ_EXTENSION power flags
//
#define POP_DOE_SYSTEM_IRP_ACTIVE           0x200
#define POP_DOE_DEVICE_IRP_ACTIVE           0x400
#define POP_DOE_PENDING_PROCESS             0x600
#define POP_DOE_HAS_INRUSH_DEVICE           0x900

//
// Power system device context flag
//
#define POP_SYS_CONTEXT_SYSTEM_IRP              0xA
#define POP_SYS_CONTEXT_DEVICE_PWR_REQUEST      0xC
#define POP_SYS_CONTEXT_WAKE_REQUEST            0xD

/******************************************************************************
 *                                      thrmzn.c                              *
 ******************************************************************************/

//
// Processor throttling constants
//
#define POP_CURRENT_THROTTLE_MAX            100

//
// Thermal zone flags
//
#define POP_THERMAL_ZONE_NONE               0x0
#define POP_THERMAL_ZONE_IS_ACTIVE          0x2

/******************************************************************************
 *                                      policy.c                              *
 ******************************************************************************/

//
// Power Policy Revision
//
#define POP_SYSTEM_POWER_POLICY_REVISION_V1     1

//
// Policy Worker Function Signature
//
_Function_class_(POP_POLICY_WORKER_FUNC)
typedef VOID
(NTAPI *PPOP_POLICY_WORKER_FUNC) (
    VOID);

/******************************************************************************
 *                                      pocs.c                                *
 ******************************************************************************/

//
// POP_CONTROL_SWITCH flags
//
#define POP_CS_INITIALIZING             0x00000001
#define POP_CS_CLEANUP                  0x00000100

//
// Power control switch modes
//
#define POP_CS_NO_MODE                  0
#define POP_CS_QUERY_CAPS_MODE          1
#define POP_CS_QUERY_EVENT_MODE         2

/******************************************************************************
 *                                      batt.c                                *
 ******************************************************************************/

//
// POP_BATTERY flags
//
#define POP_CB_NO_BATTERY               0x00000001
#define POP_CB_PENDING_NEW_BATTERY      0x00000002
#define POP_CB_PROCESSING_MODE_REQUEST  0x00000004
#define POP_CB_WAIT_ON_BATTERY_TAG      0x00000020
#define POP_CB_REMOVE_BATTERY           0x00000100

//
// Power composite battery modes
//
#define POP_CB_NO_MODE                                   0
#define POP_CB_READ_TAG_MODE                             1
#define POP_CB_QUERY_INFORMATION_MODE                    2
#define POP_CB_QUERY_STATUS_MODE                         3
#define POP_CB_QUERY_BATTERY_ESTIMATION_TIME_MODE        4
#define POP_CB_QUERY_TEMPERATURE_MODE                    5

//
// Battery status wait interval (3000 ms = 3 s)
//
#define POP_CB_STATUS_WAIT_INTERVAL         3000

/******************************************************************************
 *                                      voldope.c                             *
 ******************************************************************************/

//
// Volume DOE flags
//
#define POP_DOE_SYSTEM_POWER_FLAG_BIT           0xF
#define POP_DOE_DEVICE_POWER_FLAG_BIT           0xF0

//
// Volume flushing flags (PopFlushVolumes)
//
#define POP_FLUSH_REGISTRY                       1
#define POP_FLUSH_NON_REM_DEVICES                2

/******************************************************************************
 *                                      poapi.c                               *
 ******************************************************************************/

//
// BusyCount & BusyReference field offsets (PoSetDeviceBusyEx and PoStartDeviceBusy/PoEndDeviceBusy)
//
#define POP_BUSY_COUNT_OFFSET                   1
#define POP_BUSY_REFERENCE_OFFSET               2

/******************************************************************************
 *                                      posett.c                              *
 ******************************************************************************/

//
// Maximum number of power setting elements the PopPowerSettingsDatabase array can hold
//
#define POP_MAX_POWER_SETTINGS                  110

/******************************************************************************
 *                             Data Structures & Enums                        *
 ******************************************************************************/

//
// Power policy worker types
//
typedef enum _POP_POWER_POLICY_WORKER_TYPES
{
    PolicyWorkerNotification,
    PolicyWorkerSystemIdle,
    PolicyWorkerTimeChange,
    PolicyWorkerMax
} POP_POWER_POLICY_WORKER_TYPES;

//
// Power policy type enumeration
//
typedef enum _POP_POWER_POLICY_TYPE
{
    PolicyAc,
    PolicyDc
} POP_POWER_POLICY_TYPE;

//
// Device idle type enumeration
//
typedef enum _POP_DEVICE_IDLE_TYPE
{
    DeviceIdleNormal,
    DeviceIdleDisk
} POP_DEVICE_IDLE_TYPE;

//
// Power control switch type (lid, power button, etc.)
//
typedef enum _POP_SWITCH_TYPE
{
    SwitchNone,
    SwitchLid,
    SwitchButtonPower,
    SwitchButtonSleep,
} POP_SWITCH_TYPE;

//
// Power state handler comands enumeration
//
typedef enum _POP_POWER_HANDLER_COMMAND
{
    SaveFloatingPointContext = 1,
    InvokeNotifyHandler,
    InvokePowerStateHandler,
    RestoreFloatingPointContext,
    QuitDpc
} POP_POWER_HANDLER_COMMAND;

//
// Search IRP data by mode enumeration
//
typedef enum _POP_SEARCH_BY
{
    SearchByIrp,
    SearchByDevice
} POP_SEARCH_BY;

//
// Battery query information type enumeration
//
typedef enum _POP_BATTERY_INFORMATION_TYPE
{
    BatteryInfo,
    BatteryEstTime,
    BatteryTemp
} POP_BATTERY_INFORMATION_TYPE;

//
// Power request inquire type
//
typedef enum _POP_POWER_REQUEST_INQUIRE_TYPE
{
    RegisterLegacyRequest,
    RegisterALaVistaRequest
} POP_POWER_REQUEST_INQUIRE_TYPE;

//
// Hibernation performance counters
//
typedef struct _PO_HIBER_PERF
{
    ULONGLONG IoTicks;
    ULONGLONG InitTicks;
    ULONGLONG CopyTicks;
    ULONGLONG StartCount;
    ULONG ElapsedTime;
    ULONG IoTime;
    ULONG CopyTime;
    ULONG InitTime;
    ULONG PagesWritten;
    ULONG PagesProcessed;
    ULONG BytesCopied;
    ULONG DumpCount;
    ULONG FileRuns;
    ULONGLONG ResumeAppStartTime;
    ULONGLONG ResumeAppEndTime;
    ULONGLONG HiberFileResumeTime;
} PO_HIBER_PERF, *PPO_HIBER_PERF;

//
// Power hibernation metadata image file
//
typedef struct _PO_MEMORY_IMAGE
{
    ULONG Signature;
    ULONG ImageType;
    ULONG CheckSum;
    ULONG LengthSelf;
    PFN_NUMBER PageSelf;
    ULONG PageSize;
    LARGE_INTEGER SystemTime;
    ULONGLONG InterruptTime;
    ULONG FeatureFlags;
    UCHAR HiberFlags;
    UCHAR spare[3];
    ULONG NoHiberPtes;
    ULONG_PTR HiberVa;
    PHYSICAL_ADDRESS HiberPte;
    ULONG NoFreePages;
    ULONG FreeMapCheck;
    ULONG WakeCheck;
    PFN_NUMBER TotalPages;
    PFN_NUMBER FirstTablePage;
    PFN_NUMBER LastFilePage;
    PO_HIBER_PERF PerfInfo;
    PFN_NUMBER NoBootLoaderLogPages;
    PFN_NUMBER BootLoaderLogPages[8];
    ULONG TotalPhysicalMemoryCount;
} PO_MEMORY_IMAGE, *PPO_MEMORY_IMAGE;

//
// Hibernation memory array range
//
typedef struct _PO_MEMORY_RANGE_ARRAY_RANGE
{
    PFN_NUMBER PageNo;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    ULONG CheckSum;
} PO_MEMORY_RANGE_ARRAY_RANGE;

//
// Hibernation memory array linkage
//
typedef struct _PO_MEMORY_RANGE_ARRAY_LINK
{
    struct _PO_MEMORY_RANGE_ARRAY *Next;
    PFN_NUMBER NextTable;
    ULONG CheckSum;
    ULONG EntryCount;
} PO_MEMORY_RANGE_ARRAY_LINK;

//
// Hibernation memory array
//
typedef struct _PO_MEMORY_RANGE_ARRAY
{
    union
    {
        PO_MEMORY_RANGE_ARRAY_RANGE Range;
        PO_MEMORY_RANGE_ARRAY_LINK Link;
    };
} PO_MEMORY_RANGE_ARRAY, *PPO_MEMORY_RANGE_ARRAY;

//
// Hibernation context data
//
typedef struct _POP_HIBER_CONTEXT
{
    BOOLEAN WriteToFile;
    BOOLEAN ReserveLoaderMemory;
    BOOLEAN ReserveFreeMemory;
    BOOLEAN VerifyOnWake;
    BOOLEAN Reset;
    UCHAR HiberFlags;
    BOOLEAN WroteHiberFile;
    KSPIN_LOCK Lock;
    BOOLEAN MapFrozen;
    RTL_BITMAP MemoryMap;
    RTL_BITMAP DiscardedMemoryPages;
    LIST_ENTRY ClonedRanges;
    ULONG ClonedRangeCount;
    PLIST_ENTRY NextCloneRange;
    PFN_NUMBER NextPreserve;
    PMDL LoaderMdl;
    PMDL AllocatedMdl;
    ULONGLONG PagesOut;
    PVOID IoPages;
    PVOID CurrentMcb;
    PDUMP_STACK_CONTEXT DumpStack;
    PKPROCESSOR_STATE WakeState;
    ULONG HiberVa;
    PHYSICAL_ADDRESS HiberPte;
    NTSTATUS Status;
    PPO_MEMORY_IMAGE MemoryImage;
    PPO_MEMORY_RANGE_ARRAY TableHead;
    PUCHAR CompressionWorkspace;
    PUCHAR CompressedWriteBuffer;
    PULONG PerformanceStats;
    PVOID CompressionBlock;
    PVOID DmaIO;
    PVOID TemporaryHeap;
    PO_HIBER_PERF PerfInfo;
    PMDL BootLoaderLogMdl;
} POP_HIBER_CONTEXT, *PPOP_HIBER_CONTEXT;

//
// Power notification order level
//
typedef struct _PO_NOTIFY_ORDER_LEVEL
{
    ULONG DeviceCount;
    ULONG ActiveCount;
    LIST_ENTRY WaitSleep;
    LIST_ENTRY ReadySleep;
    LIST_ENTRY ReadyS0;
    LIST_ENTRY WaitS0;
} PO_NOTIFY_ORDER_LEVEL, *PPO_NOTIFY_ORDER_LEVEL;

//
// Power shutdown bugcheck reasoning
//
typedef struct _POP_SHUTDOWN_BUG_CHECK
{
    HANDLE ThreadHandle;
    HANDLE ThreadId;
    HANDLE ProcessId;
    ULONG Code;
    ULONG_PTR Parameter1;
    ULONG_PTR Parameter2;
    ULONG_PTR Parameter3;
    ULONG_PTR Parameter4;
} POP_SHUTDOWN_BUG_CHECK, *PPOP_SHUTDOWN_BUG_CHECK;

//
// Power device notification order
//
typedef struct _PO_DEVICE_NOTIFY_ORDER
{
    BOOLEAN Locked;
    PDEVICE_OBJECT *WarmEjectPdoPointer;
    PO_NOTIFY_ORDER_LEVEL OrderLevel[8];
} PO_DEVICE_NOTIFY_ORDER, *PPO_DEVICE_NOTIFY_ORDER;

//
// Power device system state
//
typedef struct _POP_DEVICE_SYS_STATE
{
    UCHAR IrpMinor;
    SYSTEM_POWER_STATE SystemState;
    KSPIN_LOCK SpinLock;
    PKTHREAD Thread;
    PKEVENT AbortEvent;
    PKSEMAPHORE ReadySemaphore;
    PKSEMAPHORE FinishedSemaphore;
    BOOLEAN GetNewDeviceList;
    PO_DEVICE_NOTIFY_ORDER Order;
    LIST_ENTRY Pending;
    NTSTATUS Status;
    PDEVICE_OBJECT FailedDevice;
    BOOLEAN Waking;
    BOOLEAN Cancelled;
    BOOLEAN IgnoreErrors;
    BOOLEAN IgnoreNotImplemented;
    BOOLEAN TimeRefreshLockAcquired;
} POP_DEVICE_SYS_STATE, *PPOP_DEVICE_SYS_STATE;

//
// Power actions
//
typedef struct _POP_POWER_ACTION
{
    UCHAR Updates;
    UCHAR State;
    BOOLEAN Shutdown;
    POWER_ACTION Action;
    SYSTEM_POWER_STATE LightestState;
    ULONG Flags;
    NTSTATUS Status;
    POWER_POLICY_DEVICE_TYPE DeviceType;
    ULONG DeviceTypeFlags;
    UCHAR IrpMinor;
    BOOLEAN Waking;
    SYSTEM_POWER_STATE SystemState;
    SYSTEM_POWER_STATE NextSystemState;
    SYSTEM_POWER_STATE EffectiveSystemState;
    SYSTEM_POWER_STATE CurrentSystemState;
    PPOP_SHUTDOWN_BUG_CHECK ShutdownBugCode;
    PPOP_DEVICE_SYS_STATE DevState;
    PPOP_HIBER_CONTEXT HiberContext;
    ULONGLONG WakeTime;
    ULONGLONG SleepTime;
    SYSTEM_POWER_CONDITION WakeAlarmSignaled;
    struct
    {
        ULONGLONG ProgrammedTime;
        struct _DIAGNOSTIC_BUFFER* TimeInfo;
    } WakeAlarm[3];
    SYSTEM_POWER_CAPABILITIES FilteredCapabilities;
} POP_POWER_ACTION, *PPOP_POWER_ACTION;

//
// Power waitable trigger
//
typedef struct _POP_TRIGGER_WAIT
{
    KEVENT Event;
    NTSTATUS Status;
    LIST_ENTRY Link;
    struct _POP_ACTION_TRIGGER* Trigger;
} POP_TRIGGER_WAIT, *PPOP_TRIGGER_WAIT;

//
// Power action trigger
//
typedef struct _POP_ACTION_TRIGGER
{
    POWER_POLICY_DEVICE_TYPE Type;
    ULONG Flags;
    PPOP_TRIGGER_WAIT Wait;
    union
    {
        struct
        {
            ULONG Level;
        } Battery;

        struct
        {
            ULONG Type;
        } Button;
    } DUMMYUNIONNAME;
} POP_ACTION_TRIGGER, *PPOP_ACTION_TRIGGER;

//
// Device object power extensions (DOPE)
//
typedef struct  _DEVICE_OBJECT_POWER_EXTENSION
{
    /*
     * The device idle counter. This gets incremented every second until it hits the
     * idle timers defined by ConservationIdleTime and PerformanceIdleTime. The idle
     * counter could reset as a result of the device getting busy by an instance of
     * PoSetDeviceBusy call.
     */
    volatile ULONG IdleCount;

    /*
     * The device busy counter and busy reference. The busy counter gets incremented
     * by one each time the device explicitly reports as being busy for a short period
     * of time by the following function - PoSetDeviceBusyEx. The busy reference is used
     * to keep active references of PoStartDeviceBusy instance calls.
     */
    volatile ULONG BusyCount;
    volatile ULONG BusyReference;

    /* The total count of busy times the device has been busy, for debugging purposes */
    ULONG TotalBusyCount;

    /*
     * Idle time values, defined by the device owner. ConservationIdleTime is for idle
     * time when the system must conserve power after this time value was hit.
     * PerformanceIdleTime is for idle time when the system must use all its power for
     * performance reasons.
     */
    ULONG ConservationIdleTime;
    ULONG PerformanceIdleTime;

    /* The device object and link list of which it is linked with the global idle detect list */
    PDEVICE_OBJECT DeviceObject;
    LIST_ENTRY IdleList;

    /* The type of device that is being idle (normal device or disk/mass storage device) */
    POP_DEVICE_IDLE_TYPE IdleType;

    /* The requested device power state to be enforced when the device is idling */
    DEVICE_POWER_STATE IdleState;

    /*
     * The current power state of which the device currently operates. Usually this is set
     * to PowerDeviceD0 at the time the device has requested for idle detection. Afterwards
     * the state is then modified to the state of which the caller requested after the device
     * is fully idle.
     */
    DEVICE_POWER_STATE CurrentState;

    /* The associated power volume with this device */
    LIST_ENTRY Volume;

    /* The idle and non-idle time values of the disk/mass storage device (currently not used) */
    union
    {
        struct
        {
            ULONG IdleTime;
            ULONG NonIdleTime;
        } Disk;
    } Specific;
} DEVICE_OBJECT_POWER_EXTENSION, *PDEVICE_OBJECT_POWER_EXTENSION;

//
// Power shutdown wait entry list
//
typedef struct _POP_SHUTDOWN_WAIT_ENTRY
{
    struct _POP_SHUTDOWN_WAIT_ENTRY *NextEntry;
    PETHREAD Thread;
} POP_SHUTDOWN_WAIT_ENTRY, *PPOP_SHUTDOWN_WAIT_ENTRY;

//
// Power flush volumes
//
typedef struct _POP_FLUSH_VOLUME
{
    LIST_ENTRY List;
    LONG Count;
    KEVENT Wait;
} POP_FLUSH_VOLUME, *PPOP_FLUSH_VOLUME;

//
// Power system idle
//
typedef struct _POP_SYSTEM_IDLE
{
    LONG AverageIdleness;
    LONG LowestIdleness;
    ULONG Time;
    ULONG Timeout;
    ULONG LastUserInput;
    POWER_ACTION_POLICY Action;
    SYSTEM_POWER_STATE MinState;
    ULONG SystemRequired;
    UCHAR IdleWorker;
    UCHAR Sampling;
    ULONGLONG LastTick;
    ULONG LastSystemRequiredTime;
} POP_SYSTEM_IDLE, *PPOP_SYSTEM_IDLE;

//
// Power thermal zone
//
typedef struct _POP_THERMAL_ZONE
{
    LIST_ENTRY Link;
    UCHAR State;
    UCHAR Flags;
    UCHAR Mode;
    BOOLEAN PendingMode;
    BOOLEAN ActivePoint;
    BOOLEAN PendingActivePoint;
    LONG Throttle;
    ULONGLONG LastTime;
    ULONG SampleRate;
    ULONG LastTemp;
    KTIMER PassiveTimer;
    KDPC PassiveDpc;
    POP_ACTION_TRIGGER OverThrottled;
    PIRP Irp;
    THERMAL_INFORMATION_EX Info;
} POP_THERMAL_ZONE, *PPOP_THERMAL_ZONE;

//
// Power composite battery
//
typedef struct _POP_BATTERY
{
    ULONG Flags;

    UCHAR Mode;

    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;

    POP_ACTION_TRIGGER Trigger;

    BOOLEAN IoError;

    ULONG BatteryTag;
    ULONG Temperature;
    BATTERY_STATUS Status;
    BATTERY_INFORMATION BattInfo;

    ULONG EstimatedBatteryTime;
} POP_BATTERY, *PPOP_BATTERY;

//
// Power control switch
//
typedef struct _POP_CONTROL_SWITCH
{
    /* List entry that links with the global control switches list */
    LIST_ENTRY Link;

    /*
     * Control switch flags that govern the existence of the switch, the following
     * flag constructs are:
     *
     * POP_CS_INITIALIZING - The switch has been freshly created and awaits for querying
     * its capabilities by the CS handler;
     *
     * POP_CS_CLEANUP - The switch is about to be delisted and freed. Mostly this happens
     * when this switch was disabled by the device owner or we got an unexpected error
     * during a I/O operation.
     */
    ULONG Flags;

    /*
     * Control switch operation modes, that govern what is the switch doing at the moment.
     * The following values are:
     *
     * POP_CS_NO_MODE - No operation mode (set during control switch creation);
     *
     * POP_CS_QUERY_CAPS_MODE - Query power capabilities operation mode;
     *
     * POP_CS_QUERY_EVENT_MODE - Query power button event operation mode.
     */
    UCHAR Mode;

    /* The associated device object of this switch and current I/O packet holding the request */
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;

    /* The type of switch (power, sleep buttons or lid) */
    POP_SWITCH_TYPE SwitchType;

    /* Specific switch constructs */
    union
    {
        struct
        {
            BOOLEAN Opened;
        } Lid;

        struct
        {
            BOOLEAN Triggered;
        } Button;
    } Switch;
} POP_CONTROL_SWITCH, *PPOP_CONTROL_SWITCH;

//
// Fans
//
typedef struct _POP_FAN
{
    LIST_ENTRY Link;
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;
    UCHAR ControlPercent;
    ULONG TripPoint;
    ULONG Speed;
    ULONG NoiseLevel;
    ULONG PowerConsumed;
} POP_FAN, *PPOP_FAN;

typedef struct _POP_DEVICE_POLICY_WORKITEM_DATA
{
    /* Policy queue work item used to queue a device policy handler */
    WORK_QUEUE_ITEM WorkItem;

    /*
     * A pointer to an arbitrary data that refers to a policy device to be dispatched
     * to the respective policy device handler. Typically such a policy could be a
     * control switch (button or lid), thermal zone, battery, fan, etc.
     */
    PVOID PolicyData;

    /* The type of power policy device being dispatched */
    POWER_POLICY_DEVICE_TYPE PolicyType;
} POP_DEVICE_POLICY_WORKITEM_DATA, *PPOP_DEVICE_POLICY_WORKITEM_DATA;

//
// Power policy worker
//
typedef struct _POP_POLICY_WORKER
{
    /* Set to TRUE if a worker of this type has been requested; FALSE otherwise */
    BOOLEAN Pending;

    /* Thread that requested this worker (for debugging purposes) */
    PKTHREAD Thread;

    /* Policy worker dispatch function */
    PPOP_POLICY_WORKER_FUNC WorkerFunction;
} POP_POLICY_WORKER, *PPOP_POLICY_WORKER;

//
// Power IRP data
//
typedef struct _POP_IRP_DATA
{
    /* List entry that links with the centralized power IRP data list */
    LIST_ENTRY Link;

    /* The power I/O packet request used to transport power I/O data */
    PIRP Irp;

    /* The Policy Device Owner (PDO) that inquired the power I/O request */
    PDEVICE_OBJECT Pdo;

    /*
     * The target device object. Typically this points to the top device object
     * of the device stack depth of which the power request IRP must be dispatched.
     */
    PDEVICE_OBJECT TargetDevice;

    /*
     * The current device object that takes the power IRP request. This member gets
     * updated each time the IRP gets walked down in the device stack as it is being
     * dispatched with a call of IoCallDriver. This is for debugging purposes.
     */
    PDEVICE_OBJECT CurrentDevice;

    /*
     * The start of the IRP watchdog timer interval, in seconds. This gets decremented
     * by one second by the watchdog timer DPC.
     */
    ULONGLONG WatchdogStart;

    /* The IRP watchdog timer object */
    KTIMER WatchdogTimer;

    /*
     * The IRP watchdog DPC object that executes the watchdog deferred routine.
     * Such DPC fires up every second by the timer.
     */
    KDPC WatchdogDpc;

    /* The minor power function (IRP_MN_SET_POWER/IRP_MN_QUERY_POWER/IRP_MN_WAIT_WAKE) */
    UCHAR MinorFunction;

    /* The type of the power state request instantiated (device or system) */
    POWER_STATE_TYPE PowerStateType;

    /* The power state requested by the caller of PoRequestPowerIrp */
    POWER_STATE PowerState;

    /* If set to TRUE, the IRP watchdog is enabled as a result of the IRP being dispatched, FALSE otherwise */
    BOOLEAN WatchdogEnabled;

    /* A pointer to a framework device (PoFx) containing specific framework data */
    PPOP_FX_DEVICE FxDevice;

    /* If set to TRUE, this IRP makes the system take a new power transition */
    BOOLEAN SystemTransition;

    /* If set to TRUE, this IRP causes a PEP to be notified upon IRP completion */
    BOOLEAN NotifyPEP;

    union
    {
        /* Device related power IRP data */
        struct
        {
            PREQUEST_POWER_COMPLETE CallerCompletion;
            PVOID CallerContext;
            PDEVICE_OBJECT CallerDevice;
            BOOLEAN SystemWake;
        } Device;

        /* System related power IRP data */
        struct
        {
            PPO_DEVICE_NOTIFY NotifyDevice;
            BOOLEAN FxDeviceActivated;
        } System;
    } DUMMYUNIONNAME;
} POP_IRP_DATA, *PPOP_IRP_DATA;

typedef struct _POP_IRP_QUEUE_ENTRY
{
    LIST_ENTRY Link;
    PPOP_IRP_DATA IrpData;
} POP_IRP_QUEUE_ENTRY, *PPOP_IRP_QUEUE_ENTRY;

typedef struct _POP_IRP_THREAD_ENTRY
{
    LIST_ENTRY Link;
    PKTHREAD Thread;
    PIRP Irp;
} POP_IRP_THREAD_ENTRY, *PPOP_IRP_THREAD_ENTRY;

//
// Power state handler command context
//
typedef struct _POP_POWER_STATE_HANDLER_COMMAND_CONTEXT
{
    PPOWER_STATE_HANDLER StateHandler;
    PPOWER_STATE_NOTIFY_HANDLER StateNotifyHandler;

    PPOP_HIBER_CONTEXT HiberContext;

    POP_POWER_HANDLER_COMMAND ExecutingCommand;

    BOOLEAN InitializingDpcs;
    ULONG DpcReadyForProcess;

    ULONG ProcessorHandledCommand;

    union
    {
        struct
        {
            PVOID Context;
            PENTER_STATE_SYSTEM_HANDLER SystemHandler;
            PVOID SystemContext;
            LONG NumberProcessors;
            volatile LONG Number;
        } StateHandlerData;

        struct
        {
            POWER_STATE_HANDLER_TYPE State;
            PVOID Context;
            BOOLEAN Entering;
        } StateNotifyHandlerData;
    } ContextData;
} POP_POWER_STATE_HANDLER_COMMAND_CONTEXT, *PPOP_POWER_STATE_HANDLER_COMMAND_CONTEXT;

//
// Power state handler processor context
//
typedef struct _POP_POWER_STATE_HANDLER_PROCESSOR_CONTEXT
{
    KFLOATING_SAVE FpContext;
    NTSTATUS FpStatus;
    NTSTATUS Status;
    POP_POWER_HANDLER_COMMAND CurrentCommand;
} POP_POWER_STATE_HANDLER_PROCESSOR_CONTEXT, *PPOP_POWER_STATE_HANDLER_PROCESSOR_CONTEXT;

//
// Active power requests
//
typedef struct _POP_ACTIVE_POWER_REQUESTS
{
    ULONG ActiveSystemRequiredRequests;
    ULONG ActiveDisplayRequiredRequests;
    ULONG ActiveAwayModeRequiredRequests;
    ULONG ActiveExecutionRequests;
    ULONG ActiveUserPresentRequests;
} POP_ACTIVE_POWER_REQUESTS;

//
// Power request object
//
typedef struct _POP_POWER_REQUEST
{
    LIST_ENTRY Link;
    PDEVICE_OBJECT DeviceRequestor;
    ULONG UseCount;
    BOOLEAN Legacy;
    BOOLEAN Terminate;
    KPROCESSOR_MODE RequestorMode;
    EXECUTION_STATE LegacyStateFlags;
    POP_ACTIVE_POWER_REQUESTS ActiveRequests;
    UCHAR ImageFileName[16];
    PCOUNTED_REASON_CONTEXT Context;
} POP_POWER_REQUEST, *PPOP_POWER_REQUEST;

//
// Power setting callback
//
typedef struct _POP_POWER_SETTING_CALLBACK
{
    LIST_ENTRY Link;
    BOOLEAN EnterCallback;
    KEVENT CallbackReturned;
    PDEVICE_OBJECT DeviceObject;
    GUID SettingGuid;
    PPOWER_SETTING_CALLBACK Callback;
    PVOID Context;
} POP_POWER_SETTING_CALLBACK, *PPOP_POWER_SETTING_CALLBACK;

//
// Power setting notify block
//
typedef struct _POP_POWER_SETTING_NOTIFY_BLOCK
{
    WORK_QUEUE_ITEM NotifyWorkItem;
    GUID NotifySettingGuid;
} POP_POWER_SETTING_NOTIFY_BLOCK, *PPOP_POWER_SETTING_NOTIFY_BLOCK;

//
// Power settings database
//
typedef struct _POP_POWER_SETTING_DATABASE
{
    LPCGUID SettingGuid;
    PWORKER_THREAD_ROUTINE SettingRoutine;
} POP_POWER_SETTING_DATABASE, *PPOP_POWER_SETTING_DATABASE;

/******************************************************************************
 *                                   Functions                                *
 ******************************************************************************/

//
// Power Manager System Worker Thread routines
//
_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopMasterDispatchIrp(
    _In_ PVOID StartContext);

//
// Power Manager Executive Worker Thread routines
//
_Use_decl_annotations_
VOID
NTAPI
PopGracefulShutdown(
    _In_ PVOID Parameter);

_Use_decl_annotations_
VOID
NTAPI
PopPolicyManagerWorker(
    _In_ PVOID Parameter);

_Use_decl_annotations_
VOID
NTAPI
PopUnlockMemoryWorker(
    _In_ PVOID Parameter);

_Use_decl_annotations_
VOID
NTAPI
PopControlSwitchHandler(
    _In_ PVOID Parameter);

_Use_decl_annotations_
VOID
NTAPI
PopCompositeBatteryHandler(
    _In_ PVOID Parameter);

//
// Initialization routines
//
CODE_SEG("INIT")
BOOLEAN
NTAPI
PoInitSystem(
    _In_ ULONG BootPhase);

CODE_SEG("INIT")
VOID
NTAPI
PoInitializePrcb(
    _Inout_ PKPRCB Prcb);

//
// Power Policy Manager routines
//
VOID
NTAPI
PopPowerPolicyNotification(
    VOID);

VOID
NTAPI
PopPowerPolicySystemIdle(
    VOID);

VOID
NTAPI
PopPowerPolicyTimeChange(
    VOID);

VOID
NTAPI
PopInitializePowerPolicy(
    _Out_ PSYSTEM_POWER_POLICY PowerPolicy);

VOID
NTAPI
PopDefaultPolicies(
    VOID);

VOID
NTAPI
PopRegisterPowerPolicyWorker(
    _In_ POP_POWER_POLICY_WORKER_TYPES WorkerType,
    _In_ PPOP_POLICY_WORKER_FUNC WorkerFunction);

VOID
NTAPI
PopRequestPolicyWorker(
    _In_ POP_POWER_POLICY_WORKER_TYPES WorkerType);

VOID
NTAPI
PopCheckForPendingWorkers(
    VOID);

NTSTATUS
NTAPI
PopDevicePolicyCallback(
    _In_ PVOID NotificationStructure,
    _In_ PVOID Context);

NTSTATUS
NTAPI
PopGetPolicyDeviceObject(
    _In_ PUNICODE_STRING DeviceName,
    _Out_ PDEVICE_OBJECT *DeviceObject);

//
// Power Manager Composite Battery routines
//
NTSTATUS
NTAPI
PopDisconnectCompositeBattery(
    VOID);

NTSTATUS
NTAPI
PopConnectCompositeBattery(
    _In_ PDEVICE_OBJECT BatteryDevice);

VOID
NTAPI
PopMarkNewBatteryPending(
    _In_ PUNICODE_STRING BatteryName);

VOID
NTAPI
PopQueryBatteryState(
    _Out_ PSYSTEM_BATTERY_STATE BatteryState);

//
// Power Manager Switches Control routines
//
PPOP_CONTROL_SWITCH
NTAPI
PopGetControlSwitchByDevice(
    _In_ PDEVICE_OBJECT DeviceObject);

NTSTATUS
NTAPI
PopCreateControlSwitch(
    _In_ PDEVICE_OBJECT DeviceObject,
    _Out_ PPOP_CONTROL_SWITCH *ControlSwitch);

VOID
NTAPI
PopSetButtonPowerAction(
    _In_ PPOWER_ACTION_POLICY Button,
    _In_ POWER_ACTION Action);

//
// Power State routines
//
NTSTATUS
NTAPI
PopInvokeSystemStateHandler(
    _In_ POWER_STATE_HANDLER_TYPE HandlerType,
    _In_opt_ PPOP_HIBER_CONTEXT HiberContext);

ULONG
NTAPI
PopGetDoePowerState(
    _In_ PEXTENDED_DEVOBJ_EXTENSION DevObjExts,
    _In_ BOOLEAN GetSystem);

VOID
NTAPI
PopSetDoePowerState(
    _In_ PEXTENDED_DEVOBJ_EXTENSION DevObjExts,
    _In_ POWER_STATE NewState,
    _In_ BOOLEAN SetSystem);

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopScanForIdleStateDevicesDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2);

NTSTATUS
NTAPI
PopRegisterSystemStateHandler(
    _In_ POWER_STATE_HANDLER_TYPE Type,
    _In_ BOOLEAN RtcWake,
    _In_ PENTER_STATE_HANDLER Handler,
    _In_opt_ PVOID Context);

VOID
NTAPI
PopIndicateSystemStateActivity(
    _In_ EXECUTION_STATE StateActivity);

//
// Notification routines
//
VOID
NTAPI
PoNotifySystemTimeSet(
    VOID);

//
// Shutdown routines
//
_Function_class_(ENTER_STATE_HANDLER)
NTSTATUS
NTAPI
PopShutdownHandler(
    _In_opt_ PVOID Context,
    _In_opt_ PENTER_STATE_SYSTEM_HANDLER SystemHandler,
    _In_opt_ PVOID SystemContext,
    _In_ LONG NumberProcessors,
    _In_opt_ LONG volatile *Number);

//
// IRP routines
//
PPOP_IRP_DATA
NTAPI
PopFindIrpData(
    _In_opt_ PIRP Irp,
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ POP_SEARCH_BY SearchBy);

BOOLEAN
NTAPI
PopHasDoOutstandingIrp(
    _In_ PDEVICE_OBJECT DeviceObject);

NTSTATUS
NTAPI
PopRequestPowerIrp(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ UCHAR MinorFunction,
    _In_ POWER_STATE PowerState,
    _In_ BOOLEAN IsFxDevice,
    _In_ BOOLEAN NotifyPEP,
    _In_opt_ PREQUEST_POWER_COMPLETE CompletionFunction,
    _In_opt_ __drv_aliasesMem PVOID Context,
    _Outptr_opt_ PIRP *Irp);

NTSTATUS
NTAPI
PopCreateIrpWorkerThread(
    _In_ PKSTART_ROUTINE WorkerRoutine);

NTSTATUS
FASTCALL
PoHandlePowerIrp(
    _In_ PIRP Irp);

//
// Volume and device object power extension (DOPE) routines
//
PDEVICE_OBJECT_POWER_EXTENSION
NTAPI
PopGetDope(
    _In_ PDEVICE_OBJECT DeviceObject);

VOID
NTAPI
PopFlushVolumes(
    _In_ BOOLEAN ShuttingDown);

VOID
NTAPI
PopRemoveVolumeDevice(
    _In_ PDEVICE_OBJECT DeviceObject);

VOID
NTAPI
PoVolumeDevice(
    _In_ PDEVICE_OBJECT DeviceObject);

VOID
NTAPI
PoInitializeDeviceObject(
    _Inout_ PDEVOBJ_EXTENSION DeviceObjectExtension);

//
// Miscellaneous routines
//
VOID
NTAPI
PopCreatePowerPolicyDatabase(
    VOID);

NTSTATUS
NTAPI
PopReadPowerSettings(
    _In_ PUNICODE_STRING PowerValue,
    _In_ ULONG ValueType,
    _Out_ PKEY_VALUE_PARTIAL_INFORMATION *ReturnedData);

PVOID
NTAPI
PopAllocatePool(
    _In_ SIZE_T PoolSize,
    _In_ BOOLEAN Paged,
    _In_ ULONG Tag);

VOID
NTAPI
PopFreePool(
    _In_ _Post_invalid_ PVOID PoolBuffer,
    _In_ ULONG Tag);

VOID
NTAPI
PoRundownDeviceObject(
    _In_ PDEVICE_OBJECT DeviceObject);

ULONG
NTAPI
PopQueryActiveProcessors(
    VOID);

BOOLEAN
NTAPI
PopIsEqualGuid(
    _In_ CONST GUID *FirstGuid,
    _In_ CONST GUID *SecondGuid);

//
// Debugging routines
//
VOID
NTAPI
PopReportWatchdogTime(
    _In_ PPOP_IRP_DATA IrpData);

PCSTR
NTAPI
PopTranslateSystemPowerStateToString(
    _In_ SYSTEM_POWER_STATE SystemState);

PCSTR
NTAPI
PopTranslateDevicePowerStateToString(
    _In_ DEVICE_POWER_STATE DeviceState);

PCWSTR
NTAPI
PopGetPowerInformationLevelName(
    _In_ POWER_INFORMATION_LEVEL InformationLevel);

VOID
NTAPI
PopReportBatteryInformation(
    _In_ PBATTERY_INFORMATION Info);

VOID
NTAPI
PopReportBatteryStatus(
    _In_ PBATTERY_STATUS Status);

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopScanForActivePowerRequestsDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2);

VOID
NTAPI
PopReportPowerRequest(
    _In_ PPOP_POWER_REQUEST PowerRequest);

//
// Power request routines
//
_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopReapTerminatePowerRequestsDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2);

NTSTATUS
NTAPI
PopChangePowerRequestProperties(
    _In_ PPOP_POWER_REQUEST PowerRequest,
    _In_ POWER_REQUEST_TYPE RequestType,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ BOOLEAN ClearRequest);

VOID
NTAPI
PopClosePowerRequestObject(
    _In_opt_ PEPROCESS Process,
    _In_ PVOID PowerRequestObject,
    _In_ ACCESS_MASK GrantedAccess,
    _In_ ULONG ProcessHandleCount,
    _In_ ULONG SystemHandleCount);

NTSTATUS
NTAPI
PopRegisterPowerRequest(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ POP_POWER_REQUEST_INQUIRE_TYPE Request,
    _In_ BOOLEAN NewRegister,
    _In_ EXECUTION_STATE EsFlags,
    _In_opt_ PCOUNTED_REASON_CONTEXT Context,
    _Inout_opt_ PPOP_POWER_REQUEST *PowerRequestHandle);

VOID
NTAPI
PopTerminatePowerRequests(
    VOID);

VOID
NTAPI
PoRundownPowerRequestThread(
    _In_ PETHREAD Thread);

//
// Power setting callback routines
//
NTSTATUS
NTAPI
PopAllocatePowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_ PPOP_POWER_SETTING_CALLBACK *PowerSettingCallback);

PPOP_POWER_SETTING_CALLBACK
NTAPI
PopFindPowerSettingCallbackByCallback(
    _In_ PPOWER_SETTING_CALLBACK Callback);

VOID
NTAPI
PopNotifyPowerSettingChange(
    _In_ LPCGUID SettingGuid);

PWORKER_THREAD_ROUTINE
NTAPI
PopGetPowerSettingHelper(
    _In_ LPCGUID SettingGuid);

BOOLEAN
NTAPI
PopIsPowerSettingValid(
    _In_ LPCGUID SettingGuid);

//
// Global data inside the Power Manager
//

/* Power Manager synchronization objects */
extern KGUARDED_MUTEX PopVolumeLock;
extern KSPIN_LOCK PopDopeGlobalLock;
extern KGUARDED_MUTEX PopShutdownListMutex;
extern KSPIN_LOCK PopIrpLock;
extern KSEMAPHORE PopIrpDispatchMasterSemaphore;
extern ERESOURCE PopNotifyDeviceLock;
extern ERESOURCE PopPowerPolicyLock;
extern KSPIN_LOCK PopPowerPolicyWorkerLock;
extern FAST_MUTEX PopPowerSettingLock;
extern KSPIN_LOCK PopThermalZoneLock;
extern KSPIN_LOCK PopPowerRequestLock;

/* Power Manager Policy constructs */
extern POP_POLICY_WORKER PopPolicyWorker[];
extern BOOLEAN PopPendingPolicyWorker;
extern ULONG PopShutdownPowerOffPolicy;
extern LIST_ENTRY PopPowerPolicyIrpQueueList;
extern WORK_QUEUE_ITEM PopPowerPolicyWorkItem;
extern PKTHREAD PopPowerPolicyOwnerLockThread;
extern SYSTEM_POWER_POLICY PopAcPowerPolicy;
extern SYSTEM_POWER_POLICY PopDcPowerPolicy;
extern PSYSTEM_POWER_POLICY PopDefaultPowerPolicy;
extern PKWIN32_POWEREVENT_CALLOUT PopEventCallout;

/* Power Manager Shutdown constructs */
extern BOOLEAN PopShutdownCleanly;
extern BOOLEAN PopShutdownListAvailable;
extern WORK_QUEUE_ITEM PopShutdownWorkItem;
extern KEVENT PopShutdownEvent;
extern PPOP_SHUTDOWN_WAIT_ENTRY PopShutdownThreadList;
extern LIST_ENTRY PopShutdownQueue;

/* Power Manager Callbacks */
extern PCALLBACK_OBJECT SetSystemTimeCallback;

/* Power Manager Volumes & Device Nodes */
extern PDEVICE_NODE PopSystemPowerDeviceNode;
extern LIST_ENTRY PopVolumeDevices;
extern ULONG PopVolumeFlushPolicy;

/* Power Manager Centralized Actions & Capabilities */
extern POP_POWER_ACTION PopAction;
extern SYSTEM_POWER_CAPABILITIES PopCapabilities;
extern ADMINISTRATOR_POWER_POLICY PopAdminPowerPolicy;

/* Power Manager IRP constructs */
extern LIST_ENTRY PopDispatchWorkerIrpList;
extern LIST_ENTRY PopQueuedIrpList;
extern LIST_ENTRY PopQueuedInrushIrpList;
extern LIST_ENTRY PopIrpThreadList;
extern LIST_ENTRY PopIrpDataList;
extern PIRP PopInrushIrp;
extern ULONG PopIrpWatchdogTickIntervalInSeconds;
extern ULONG PopPendingIrpDispatcWorkerCount;
extern ULONG PopIrpDispatchWorkerCount;
extern PKTHREAD PopIrpOwnerLockThread;
extern KEVENT PopIrpDispatchPendingEvent;
extern BOOLEAN PopIrpDispatchWorkerPending;

/* Power Manager Wake Source constructs */
extern LIST_ENTRY PopWakeSourceDevicesList;
extern ULONG PopSystemFullWake;

/* Power Manager Thermal constructs */
extern LIST_ENTRY PopThermalZones;
extern ULONG PopCoolingSystemMode;

/* Power Manager States constructs */
extern ULONG PopIdleScanIntervalInSeconds;
extern KDPC PopIdleScanDevicesDpc;
extern KTIMER PopIdleScanDevicesTimer;
extern LIST_ENTRY PopIdleDetectList;
extern BOOLEAN PopResumeAutomatic;
extern POWER_STATE_HANDLER PopDefaultPowerStateHandlers[];

/* Power Manager Switch constructs */
extern LIST_ENTRY PopControlSwitches;

/* Power Manager Actions constructs */
extern LIST_ENTRY PopActionWaiters;

/* Power Manager Composite Battery constructs */
extern PPOP_BATTERY PopBattery;

/* Power Manager power request constructs */
extern GENERIC_MAPPING PopPowerRequestGenericMapping;
extern POBJECT_TYPE PoPowerRequestObjectType;
extern LIST_ENTRY PopPowerRequestsList;
extern ULONG PopTotalKernelPowerRequestsCount;
extern ULONG PopTotalUserPowerRequestsCount;
extern PKTHREAD PopPowerRequestOwnerLockThread;
extern KDPC PopScanActivePowerRequestsDpc;
extern KDPC PopReapTerminatePowerRequestsDpc;
extern KTIMER PopReapTerminatePowerRequestsTimer;
extern KTIMER PopScanActivePowerRequestsTiimer;
extern ULONG PopScanActivePowerRequestsIntervalInSeconds;
extern ULONG PopReapTerminatePowerRequestsIntervalInSeconds;
extern BOOLEAN PopReaperTerminateActivated;

/* Power Manager power setting callbacks constructs */
extern LIST_ENTRY PopPowerSettingCallbacksList;
extern ULONG PopPowerSettingCallbacksCount;
extern PKTHREAD PopPowerSettingOwnerLockThread;
extern POP_POWER_SETTING_DATABASE PopPowerSettingsDatabase[];

/* Power Manager miscellaneous constructs */
extern BOOLEAN PopSimulate;
extern BOOLEAN PopAcpiPresent;
extern BOOLEAN PopAoAcPresent;
extern WORK_QUEUE_ITEM PopUnlockMemoryWorkItem;
extern KEVENT PopUnlockMemoryCompleteEvent;

/* Power Manager registry constructs */
extern UNICODE_STRING PopPowerRegPath;
extern UNICODE_STRING RegAcPolicy;
extern UNICODE_STRING RegDcPolicy;

//
// Inlined functions
//
#include "po_x.h"

/* EOF */
