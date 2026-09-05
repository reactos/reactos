/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
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
#define PO_DRIPS_DEBUG                                  0x200
#define PO_AOAC_DEBUG                                   0x400
#define PO_WAKESRC_DEBUG                                0x600
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
#define POP_PO_INIT_FAILURE                                  1
#define POP_IDLE_DETECT_UNKNOWN_DEVICE                       2
#define POP_DEVICE_POLICY_IRP_ALLOC_FAILED                   3
#define POP_INVALID_CONTROL_SWITCH_MODE                      4
#define POP_BATTERY_UNKNOWN_MODE_REQUEST                     5
#define POP_POWER_SETTING_CALLBACK_THREAD_CREATION_FAILURE   6
#define POP_POWER_SETTING_CALLBACK_DEPLOY_WORKERS_FAILURE    7

/******************************************************************************
 *                                      hibersup.c                            *
 ******************************************************************************/

//
// Hibernation image file signature
//
#define POP_HIBER_FILE_SIGNATURE           0x72626968 // "hibr"

//
// Hibernation image file name
//
#define POP_HIBER_FILE_NAME                L"hiberfil.sys"

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
// IRP worker system threads priority
//
#define POP_IRP_WORKER_THREAD_PRIORITY                      5
#define POP_IRP_MASTER_DISPATCHER_THREAD_PRIORITY           7

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
 *                                      thermreq.c                            *
 ******************************************************************************/

//
// Thermal Request Interface Function Signatures
//
_Function_class_(POP_ACTIVE_COOLING_INTERFACE)
typedef VOID
(NTAPI *PPOP_ACTIVE_COOLING_INTERFACE) (
    _In_ PVOID ThermalRequest,
    _In_ BOOLEAN Engaged);

_Function_class_(POP_PASSIVE_COOLING_INTERFACE)
typedef VOID
(NTAPI *PPOP_PASSIVE_COOLING_INTERFACE) (
    _In_ PVOID ThermalRequest,
    _In_ UCHAR Throttle);

_Function_class_(POP_INTERFACE_REFERENCE)
typedef VOID
(NTAPI *PPOP_INTERFACE_REFERENCE) (
    _In_opt_ PVOID Context);

_Function_class_(POP_INTERFACE_DEREFERENCE)
typedef VOID
(NTAPI *PPOP_INTERFACE_DEREFERENCE) (
    _In_opt_ PVOID Context);

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
 *                                      posett.c                              *
 ******************************************************************************/

//
// Maximum number of power setting elements the PopPowerSettingsDatabase array can hold
//
#define POP_MAX_POWER_SETTINGS                  109

//
// Power setting callback flags
//
#define POP_PSC_REGISTERED                                  0x0
#define POP_PSC_ENTERING_CALLBACK                           0x2
#define POP_PSC_GETTING_NOTIFIED                            0x4
#define POP_PSC_UNREGISTERED                                0x10

//
// Power setting worker thread priority
//
#define POP_POWER_SETTING_WORKER_THREAD_PRIORITY            5

/******************************************************************************
 *                                      state.c                               *
 ******************************************************************************/

//
// Power system idle worker function signature
//
_Function_class_(POP_SYSTEM_IDLE_WORKER)
typedef VOID
(NTAPI *PPOP_SYSTEM_IDLE_WORKER) (
    VOID);

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
    /* Amount of I/O ticks counted at the start and end of a write I/O operation done onto the hiber file */
    ULONGLONG IoTicks;

    /* Amount of ticks counted at the start and end of writing dump stack into the hibernation context */
    ULONGLONG InitTicks;

    /* Amount of ticks counted  at the start and of copying page blocks into the hiber file */
    ULONGLONG CopyTicks;

    /* The start count at the time of beginning writing the hiber image file for the first time */
    ULONGLONG StartCount;

    /* The amount of time that was taken to write a hibernation file */
    ULONG ElapsedTime;

    /* The amount of time that was taken to do a write I/O operation onto the hiber file */
    ULONG IoTime;

    /* The amount of time that was taken to copy page blocks into the hiber file */
    ULONG CopyTime;

    /* The amount of time it was taken to create and write a hiber file */
    ULONG InitTime;

    /* Number of pages that have been written into the hiber file and pages that have been processed */
    ULONG PagesWritten;
    ULONG PagesProcessed;

    /* Amount of bytes that have been copied (the bytes represent the page blocks) into the hiber file */
    ULONG BytesCopied;

    /*
     * Number of times hibernation pages have been written to the hiber file. A complete writing
     * into the hiber file counts as one dump count.
     */
    ULONG DumpCount;

    /* Amount of map control blocks the hiber file has */
    ULONG FileRuns;

    /* Miscellaneous "Resume" times */
    ULONGLONG ResumeAppStartTime;
    ULONGLONG ResumeAppEndTime;
    ULONGLONG HiberFileResumeTime;
} PO_HIBER_PERF, *PPO_HIBER_PERF;

//
// Power hibernation metadata image file
//
typedef struct _PO_MEMORY_IMAGE
{
    /*
     * The signature that proves the legitimacy of the hibernation image file.
     * This signature is "hibr" (represented as 0x72626968). If this signature
     * is tampered, the image file is considered corrupt. The image file in
     * question is hiberfil.sys.
     */
    ULONG Signature;

    /*
     * The type of the image file, bounding to the system architecture (KeProcessorArchitecture).
     * AMD, Intel, ARM and other various architectures interpret and manage memory differently.
     */
    ULONG ImageType;

    /*
     * The checksum of the hibernation image file. If the checksum no longer identifies the
     * legitimacy of the file upon checksum calculation, the file is considered corrupt.
     */
    ULONG CheckSum;

    /* The length of the hibernation image file, if the following fields are tampered then the file is corrupt */
    ULONG LengthSelf;
    PFN_NUMBER PageSelf;
    ULONG PageSize;

    /* Time I/O information of the hibernation image file that's been created */
    LARGE_INTEGER SystemTime;
    ULONGLONG InterruptTime;

    /* Feature flags for the hibernation image file, passed from KeFeatureBits */
    ULONG FeatureFlags;

    /* Hibernation flags that  */
    UCHAR HiberFlags;

    /* Reserved field */
    UCHAR spare[3];

    /* Number of non-hibernation PTEs */
    ULONG NoHiberPtes;

    /* The virtual and physical address of the hibernation image, if the following fields are tampered then the file is corrupt */
    ULONG_PTR HiberVa;
    PHYSICAL_ADDRESS HiberPte;

    /* Page counter checks */
    ULONG NoFreePages;
    ULONG FreeMapCheck;
    ULONG WakeCheck;

    /* Number count of pages */
    PFN_NUMBER TotalPages;
    PFN_NUMBER FirstTablePage;
    PFN_NUMBER LastFilePage;

    /* Hibernation performance stats */
    PO_HIBER_PERF PerfInfo;

    /* Log of bootloader pages */
    PFN_NUMBER NoBootLoaderLogPages;
    PFN_NUMBER BootLoaderLogPages[8];

    /* Total count of physical memory present in the system */
    ULONG TotalPhysicalMemoryCount;
} PO_MEMORY_IMAGE, *PPO_MEMORY_IMAGE;

//
// Hibernation memory array range
//
typedef struct _PO_MEMORY_RANGE_ARRAY_RANGE
{
    /* Number of page tables */
    PFN_NUMBER PageNo;

    /* Numbers that represents the starting and ending page in the range */
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;

    /* The checksum of the said page table range */
    ULONG CheckSum;
} PO_MEMORY_RANGE_ARRAY_RANGE;

//
// Hibernation memory array linkage
//
typedef struct _PO_MEMORY_RANGE_ARRAY_LINK
{
    /* The next hibernation memory page entry and its next table */
    struct _PO_MEMORY_RANGE_ARRAY *Next;
    PFN_NUMBER NextTable;

    /* The checksum of the hibernation page and number of page entries in this linkage */
    ULONG CheckSum;
    ULONG EntryCount;
} PO_MEMORY_RANGE_ARRAY_LINK;

//
// Hibernation memory array
//
typedef struct _PO_MEMORY_RANGE_ARRAY
{
    /* Fields used to anchor the next hibernation pages together */
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
    /*
     * This field is to TRUE to indicate writes can be made onto the hibernation image file.
     * If this field is set to FALSE, writes cannot be made and every attempt will result in failure.
     */
    BOOLEAN WriteToFile;

    /* These fields are either set to TRUE or FALSE to reserve memory into the hibernation file */
    BOOLEAN ReserveLoaderMemory;
    BOOLEAN ReserveFreeMemory;

    /*
     * If this field is set to TRUE, the hibernation file is validated first for inconsistencies
     * upon wake of the system. NOTE that if any inconsistency or violations have been found
     * within the hibernation image file, the whole wake-from-hiber operation is cancelled
     * and the hibernation image is trashed.
     */
    BOOLEAN VerifyOnWake;

    /*
     * If this field is set to TRUE, it'll instruct the Power Manager to reboot the machine
     * upon the completion of the hibernation process rather than powering down the machine.
     */
    BOOLEAN Reset;

    /* Bit flags that govern how is the hibernation context managed */
    UCHAR HiberFlags;

    /* Indicates whether a hibernation image file has been created or not */
    BOOLEAN WroteHiberFile;

    /* Lock that protects the hibernation context and the associated image file */
    KSPIN_LOCK Lock;

    /* If this field is set to TRUE, the memory map pages are frozen and cannot go away */
    BOOLEAN MapFrozen;

    /* The bitmap containing the memory pages, discarded pages and cloned range entries */
    RTL_BITMAP MemoryMap;
    RTL_BITMAP DiscardedMemoryPages;
    LIST_ENTRY ClonedRanges;

    /* Count of memory ranges that have been cloned */
    ULONG ClonedRangeCount;

    /* Entries of next cloned memory ranges */
    PLIST_ENTRY NextCloneRange;
    PFN_NUMBER NextPreserve;

    /* Hibernation image file properties */
    PMDL LoaderMdl;
    PMDL AllocatedMdl;
    ULONGLONG PagesOut;
    PVOID IoPages;
    PVOID CurrentMcb;
    PDUMP_STACK_CONTEXT DumpStack;
    PKPROCESSOR_STATE WakeState;
    ULONG_PTR HiberVa;
    PHYSICAL_ADDRESS HiberPte;
    NTSTATUS Status;

    /* Core hibernation file image structure and underlying I/O fields */
    PPO_MEMORY_IMAGE MemoryImage;
    PPO_MEMORY_RANGE_ARRAY TableHead;

    /* Image file compression related fields */
    PUCHAR CompressionWorkspace;
    PUCHAR CompressedWriteBuffer;
    PULONG PerformanceStats;
    PVOID CompressionBlock;
    PVOID DmaIO;
    PVOID TemporaryHeap;

    /* Hibernation performance statistics */
    PO_HIBER_PERF PerfInfo;

    /* FreeLdr memory descriptor log information */
    PMDL BootLoaderLogMdl;
} POP_HIBER_CONTEXT, *PPOP_HIBER_CONTEXT;

//
// Power notification order level
//
typedef struct _PO_NOTIFY_ORDER_LEVEL
{
    /* Total count of devices inserted at the respective level */
    ULONG DeviceCount;

    /* Total count of devices currently active that have been notified at the respective level */
    ULONG ActiveCount;

    /* List of devices currently waiting to sleep */
    LIST_ENTRY WaitSleep;

    /* List of devices ready to sleep */
    LIST_ENTRY ReadySleep;

    /* List of devices ready to receive a system power IRP, of S0 state type */
    LIST_ENTRY ReadyS0;

    /* List of devices that are waiting to receive a S0 system power IRP */
    LIST_ENTRY WaitS0;
} PO_NOTIFY_ORDER_LEVEL, *PPO_NOTIFY_ORDER_LEVEL;

//
// Power shutdown bugcheck reasoning
//
typedef struct _POP_SHUTDOWN_BUG_CHECK
{
    /* Thread and Process information that submitted a bugcheck request */
    HANDLE ThreadHandle;
    HANDLE ThreadId;
    HANDLE ProcessId;

    /* The code of the bugcheck */
    ULONG Code;

    /* The additional extra parameters that describe the reason of the bugcheck */
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
    /* If this condition is set to TRUE, the notification order cannot be updated until further notice */
    BOOLEAN Locked;

    /* The device object that is responsible for doing a warm eject operation */
    PDEVICE_OBJECT *WarmEjectPdoPointer;

    /*
     * The order level of which devices are to be powered up or down based on the broadcast
     * notification and system state. This array can hold up to 8 order levels deep.
     */
    PO_NOTIFY_ORDER_LEVEL OrderLevel[8];
} PO_DEVICE_NOTIFY_ORDER, *PPO_DEVICE_NOTIFY_ORDER;

//
// Power device system state
//
typedef struct _POP_DEVICE_SYS_STATE
{
    /*
     * The power code used to submit the system power IRP to all devices. This can be IRP_MN_SET_POWER or IRP_MN_QUERY_POWER only.
     * SystemState points to the system power state the system is currently entering into it and this must be broadcasted
     * to every single device.
     */
    UCHAR IrpMinor;
    SYSTEM_POWER_STATE SystemState;

    /* Lock that protects the devices state structure and the thread that owns the lock */
    KSPIN_LOCK SpinLock;
    PKTHREAD Thread;

    /*
     * The notification events for the devices state. AbortEvent is used to signal an event
     * the broadcasting of system power IRP to all devices has been aborted due to reasons.
     * ReadySemaphore is used to signal the Power Manager is ready to broadcast the system
     * state across all devices while FinishedSemaphore is used to indicate the operation
     * has finished successfully.
     */
    PKEVENT AbortEvent;
    PKSEMAPHORE ReadySemaphore;
    PKSEMAPHORE FinishedSemaphore;

    /* If this field is set to TRUE a new list of devices for broadcasting is to be made */
    BOOLEAN GetNewDeviceList;

    /*
     * This field is used to help the Power Manager in which order should devices be
     * broadcasted for system power state notification. Some devices are best to be powered
     * down or up depending on their nature, their states and whatnot. For example, when
     * the system undergoes power down, the display device should be the last one to be
     * powered down.
     */
    PO_DEVICE_NOTIFY_ORDER Order;

    /*
     * List of device objects that are currently processing the system power IRP.
     * A system power broadcast is considered successful if this list is empty
     * and FailedDevice is NULL.
     */
    LIST_ENTRY Pending;

    /*
     * The status code returned by the last device that completed the system power IRP.
     * The system state broadcast operation is aborted when this status code is no longer
     * a successful status code, and FailedDevice points to the device that failed the operation.
     */
    NTSTATUS Status;
    PDEVICE_OBJECT FailedDevice;

    /*
     * Toggle switches that influence how the devices state operation should be done
     * and what shouldn't be done.
     *
     * Waking - This field is set to TRUE if all devices are to be broadcasted of the S0
     * system power state. If set to FALSE, the devices will be broadcasted of the pointed
     * system state.
     *
     * Cancelled - This condition is set to TRUE if at least one device has failed processing
     * the system power IRP and the devices system state broadcasting must be aborted.
     * AbortEvent will be signaled soon.
     *
     * IgnoreErrors - This condition is set to TRUE to tell the Power Manager to ignore
     * errors committed by the devices when sending them the system power IRP during system
     * transition.
     *
     * IgnoreNotImplemented - This condition is set to TRUE to tell the Power Manager to
     * ignore devices that don't implement a certain request imposed by the devices state
     * operation.
     *
     * TimeRefreshLockAcquired - This condition is set to TRUE if the timer refresh lock
     * has been acquired by the calling thread, so that to avoid the Power Manager acquiring
     * it twice.
     */
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
    /* The current state of the global power action */
    UCHAR Updates;
    UCHAR State;

    /*
     * The shutdown action signal. If this field is set to TRUE, the system
     * is incurring in a shutdown process and this action must be broadcasted
     * to the whole system. During that condition, the Power Actions Manager
     * is performing shutdown related stuff.
     */
    BOOLEAN Shutdown;

    /* The power action to be taken globally by PAM (Power Actions Manager) */
    POWER_ACTION Action;

    /*
     * The lightest system state PAM has to take into consideration. The Power
     * Manager cannot undergo the system into the lowest system state than
     * the lightest one permitted unless this field is to be ignored due to
     * conditions like critical power down, user's request sleep and whatnot.
     * The lightest system power state is also the state that the system can
     * enter into that's not a deep state, like PowerSystemHibernate.
     */
    SYSTEM_POWER_STATE LightestState;

    /* The flags that govern the centralized power actions mechanism and the status returned */
    ULONG Flags;
    NTSTATUS Status;

    /* The policy device that is currently controlling the power action */
    POWER_POLICY_DEVICE_TYPE DeviceType;

    /* The extra flags passed by the policy device */
    ULONG DeviceTypeFlags;

    /*
     * The power minor code. This can be IRP_MN_SET_POWER, IRP_MN_QUERY_POWER or IRP_MN_WAIT_WAKE.
     * The Power Actions Manager uses to centralize the action across all devices because of
     * a change in the state of the system.
     */
    UCHAR IrpMinor;

    /* If this field is set to TRUE, the system is waking due to this power action */
    BOOLEAN Waking;

    /* The system state fields */
    SYSTEM_POWER_STATE SystemState;
    SYSTEM_POWER_STATE NextSystemState;
    SYSTEM_POWER_STATE EffectiveSystemState;
    SYSTEM_POWER_STATE CurrentSystemState;

    /* The shutdown bugcode fields supplied by PAM due to a serious issue that occurred while a power action was undergoing */
    PPOP_SHUTDOWN_BUG_CHECK ShutdownBugCode;

    /*
     * The system state of devices. This field is used by PAM to keep track and order command
     * to all devices to change their states or perform actions due to a change in the state
     * of the system. For example, if the system is shutting down, this field is used to
     * broadcast in an orderly fashion this system state to all devices.
     */
    PPOP_DEVICE_SYS_STATE DevState;

    /* The hibernation context passed to the centralized power action if the system undergoes hibernation */
    PPOP_HIBER_CONTEXT HiberContext;

    /* The times when the system was being awake or sleeping */
    ULONGLONG WakeTime;
    ULONGLONG SleepTime;

    /*
     * The power condition set to check against at which condition the wake alarms must be signaled.
     * For instance if the condition is set to PoDc, the wake alarms can ONLY be woken up if the
     * current enforced power policy id DC, which means the system is powered up by batteries.
     */
    SYSTEM_POWER_CONDITION WakeAlarmSignaled;

    /* Wake alarms array, initialized for each power condition (PoAc, PoDc and PoHot) */
    struct
    {
        ULONGLONG ProgrammedTime;
        struct _DIAGNOSTIC_BUFFER* TimeInfo;
    } WakeAlarm[3];

    /*
     * The filtered power capabilities. PAM uses this field to perform power actions in accordance
     * with the supported capabilities pointed by this field. This is to ensure the Power Manager
     * does not perform actions based on capabilities that the system no longer supports.
     */
    SYSTEM_POWER_CAPABILITIES FilteredCapabilities;
} POP_POWER_ACTION, *PPOP_POWER_ACTION;

//
// Power waitable trigger
//
typedef struct _POP_TRIGGER_WAIT
{
    /*
     * Event that is triggered once the wait conditions have been met and
     * the action must be triggered soon.
     */
    KEVENT Event;

    /* The status code returned after the action has been performed */
    NTSTATUS Status;

    /* List entry linked with the waitable triggers list and the associated action trigger */
    LIST_ENTRY Link;
    struct _POP_ACTION_TRIGGER* Trigger;
} POP_TRIGGER_WAIT, *PPOP_TRIGGER_WAIT;

//
// Power action trigger
//
typedef struct _POP_ACTION_TRIGGER
{
    /* The type of the dvice policy that is triggering an action. Refer to the type enumeration for details */
    POWER_POLICY_DEVICE_TYPE Type;

    /* The flags that govern the behavior of the action trigger */
    ULONG Flags;

    /* The wait conditions that need to be satisfied for an action to be triggered */
    PPOP_TRIGGER_WAIT Wait;

    /* Devices that setup condition values for an action to be triggered */
    union
    {
        /*
         * The battery capacity in percentage level. The Power Manager will
         * trigger an action based on the information in this structure and
         * if the composite battery has reached that percent level.
         */
        struct
        {
            ULONG PercentLevel;
        } Battery;

        /*
         * The type of the button. This will trigger an action once the specific
         * button of this type has been pressed.
         */
        struct
        {
            ULONG Type;
        } Button;
    } DUMMYUNIONNAME;
} POP_ACTION_TRIGGER, *PPOP_ACTION_TRIGGER;

typedef struct _THERMAL_COOLING_INTERFACE
{
    /* The size of the thermal cooling interface structure. Set this to sizeof(THERMAL_COOLING_INTERFACE). */
    USHORT Size;

    /* The version of the thermal cooling interface. Set this to THERMAL_COOLING_INTERFACE_V1. */
    USHORT Version;

    /* An argument context that is passed to the interface routines */
    PVOID Context;

    /*
     * The interface reference/dereference routines. These routines are used to add up or
     * remove active count references to the existing interface installed to the thermal request.
     */
    PPOP_INTERFACE_REFERENCE InterfaceReference;
    PPOP_INTERFACE_DEREFERENCE InterfaceDereference;

    /* The flags passed to this thermal interface */
    ULONG Flags;

    /* The active/passive cooling routines called by Power Manager whenever a cooling process is being undertaken */
    PPOP_ACTIVE_COOLING_INTERFACE ActiveCooling;
    PPOP_PASSIVE_COOLING_INTERFACE PassiveCooling;
} THERMAL_COOLING_INTERFACE, *PTHERMAL_COOLING_INTERFACE;

typedef struct _POP_RW_LOCK
{
    /* Push lock that protects a certain object */
    EX_PUSH_LOCK Lock;

    /* Thread that currently owns the lock */
    PKTHREAD Thread;
} POP_RW_LOCK, *PPOP_RW_LOCK;

typedef struct _POP_COOLING_EXTENSION
{
    /* List entry that is linked with the centralized thermal request */
    LIST_ENTRY Link;

    /* The requests list head, this field is currently unused */
    LIST_ENTRY RequestListHead;

    /* The Read/Write lock that protects the request list, this field is currently unused */
    POP_RW_LOCK Lock;

    /* The policy power owner (PPO) device that actually owns the thermal request */
    PDEVICE_OBJECT DeviceObject;

    /*
     * A notification entry. The PPO device gets notified when the target device
     * has been fully cooled as per the thermal request requirements.
     */
    PVOID NotificationEntry;

    /*
     * Determines whether the thermal request is fully enabled or disabled.
     * A disabled thermal request is a request that no longer has any active
     * references to it.
     */
    BOOLEAN Enabled;

    /*
     * The active cooling engagement. This field is either set to TRUE or FALSE depending
     * on a call to PoSetThermalActiveCooling.
     */
    BOOLEAN ActiveEngaged;

    /*
     * The passive throttle limit level. This field is updated by a call to PoSetThermalPassiveCooling.
     * The Power Manager must not allow the thermal request to passive cool by underthrottling the
     * target device below the enforced limit.
     */
    UCHAR ThrottleLimit;

    /*
     * If this field is to TRUE, the thermal request's cooling interface is undergoing
     * updates in its properties. This field is currently unused.
     */
    BOOLEAN UpdatingToCurrent;

    /* PnP flush events of the target device being cooled */
    PKEVENT RemovalFlushEvent;
    PKEVENT PnpFlushEvent;

    /* The cooling interface schema of this thermal request */
    THERMAL_COOLING_INTERFACE Interface;
} POP_COOLING_EXTENSION, *PPOP_COOLING_EXTENSION;

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

    /*
     * The cooling extension of this device. A device may support thermal requests or
     * not depending on the information passed by this field.
     */
    PPOP_COOLING_EXTENSION CoolingExtension;

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
    /* The next shutdown entry to be processed */
    struct _POP_SHUTDOWN_WAIT_ENTRY *NextEntry;

    /* The thread that owns the shutdown entry to be processed */
    PETHREAD Thread;
} POP_SHUTDOWN_WAIT_ENTRY, *PPOP_SHUTDOWN_WAIT_ENTRY;

//
// Power flush volumes
//
typedef struct _POP_FLUSH_VOLUME
{
    /* List entry that is linked with the flush volumes list */
    LIST_ENTRY List;

    /* The counter of flush workers that have completed flushing an individual volume */
    LONG Count;

    /* The event that is triggered when a flush worker has finished flushing the volume */
    KEVENT Wait;
} POP_FLUSH_VOLUME, *PPOP_FLUSH_VOLUME;

//
// Power system idle
//
typedef struct _POP_SYSTEM_IDLE
{
    /*
     * The average idleness of the system. The average is calculated by suming up the last
     * captured idle times and divided by the number of the said captured said last idle times.
     */
    LONG AverageIdleness;

    /*
     * The lowest idleness recorded. This is determined by checking against the current idle
     * counter if the previously recorded idle time is lower than the current.
     */
    LONG LowestIdleness;

    /*
     * The idle counter that is increased each second the system reports no activity.
     * The DPC core system idle worker increments such idle counter and calculates the
     * average and lowest idlenesses. This counter is reset every time a call to PoSetPowerRequest(PowerRequestSystemRequired)
     * or PoSetSystemState(ES_SYSTEM_REQUIRED) or PoRegisterSystemState(ES_SYSTEM_REQUIRED) is invoked. Power requests
     * prevent the Power Manager to update this counter if there are devices or applications that make the system busy.
     * Refer to the SystemRequired field.
     */
    ULONG Time;

    /*
     * The timeout idle value. This timeout represents the maximum threshold a system can stay
     * idle, in other words, if the Time idle counter hits this timeout, the system is considered
     * to be idling for too long and too much and Power Manager must perform a power action
     * on behalf of the system depending on the information provided in Action and MinState fields.
     */
    ULONG Timeout;

    /* Last time the user prompted an input */
    ULONG LastUserInput;

    /* The power action to be taken and the minimum power state to be  */
    POWER_ACTION_POLICY Action;
    SYSTEM_POWER_STATE MinState;

    /*
     * If this condition is set to TRUE, it indicates that at least a power request has required to
     * make the system busy. The Time idle counter is not updated.
     */
    BOOLEAN SystemRequired;

    /* The idle worker routine */
    PPOP_SYSTEM_IDLE_WORKER IdleWorker;

    /* The idle sampling and last tick of the system */
    UCHAR Sampling;
    ULONGLONG LastTick;

    /* The last time the system was required to be busy */
    ULONG LastSystemRequiredTime;
} POP_SYSTEM_IDLE, *PPOP_SYSTEM_IDLE;

//
// Power thermal zone
//
typedef struct _POP_THERMAL_ZONE
{
    /* List entry that links with the centalized thermal zones list */
    LIST_ENTRY Link;

    /* The current state of the thermal zone */
    UCHAR State;

    /* Bit flags that govern how the thermal zone behaves */
    UCHAR Flags;

    /* The operation mode of the thermal zone currently taking action */
    UCHAR Mode;
    UCHAR PendingMode;

    /*
     * Active cooling constraints. The active point constraint points to
     * the threshold the Power Manager shall ever hit in order to kick in
     * the active cooling mechanism.
     */
    UCHAR ActivePoint;
    UCHAR PendingActivePoint;

    /*
     * The current throttle level this thermal zone is currently operating at,
     * the last time the throttle sampling rate was updated and the throttle
     * sample rate.
     */
    LONG Throttle;
    ULONGLONG LastTime;
    ULONG SampleRate;

    /*
     * The temperature of the last time the thermal zone perfomed a cooling
     * attempt (passive or active).
     */
    ULONG LastTemp;

    /* The passive DPC that is fired when passive cooling is to be kicked */
    KTIMER PassiveTimer;
    KDPC PassiveDpc;

    /* Determines the conditions to trigger a power action during an over throttle situation */
    POP_ACTION_TRIGGER OverThrottled;

    /* The IRP that is used to transport I/O thermal data to the driver */
    PIRP Irp;

    /* The additional thermal information */
    THERMAL_INFORMATION_EX Info;
} POP_THERMAL_ZONE, *PPOP_THERMAL_ZONE;

//
// Power composite battery
//
typedef struct _POP_BATTERY
{
    /*
     * Bit flags that govern the behavior of the composite battery, the following
     * flag constructs are:
     *
     * POP_CB_NO_BATTERY - No battery was ever reported by ACPI therefor the composite
     *                     battery policy device is not connected with the Power Manager.
     *
     * POP_CB_PENDING_NEW_BATTERY - A new battery has been detected and we got further
     *                              notification of this from OSPM. The Power Manager
     *                              sees this flag and invalidates the whole battery
     *                              constructs such as tag, information and whatnot and
     *                              inquires the CB handler to re-read the battery information
     *                              again.
     *
     * POP_CB_PROCESSING_MODE_REQUEST - The composite battery is currently undergoing a mode
     *                                  that needs to be processed.
     *
     * POP_CB_WAIT_ON_BATTERY_TAG - The CB handler requested to read the tag from the battery and
     *                              it has failed, therefore it is now waiting for the tag to be read.
     *                              This can happen in two circumstances, where the battery hasn't been
     *                              fully initialized yet by OSPM or the battery had disappeared. In either
     *                              of the cases the Power Manager can only wait for another battery to appear.
     *
     * POP_CB_REMOVE_BATTERY - Instructs the power manager the composite battery policy device
     *                         needs to be disconnected and whole battery information nulled.
     *                         This typically happens when some serious error has occurred during
     *                         composite battery operations.
     */
    ULONG Flags;

    /*
     * The operation mode the composite battery is currently taking action, the following are:
     *
     * POP_CB_NO_MODE - The composite battery is taking no mode. This usually happens when
     *                  the composite battery kernel structure has just been initialized and
     *                  no battery policy device was ever connected with the Power Manager.
     *
     * POP_CB_READ_TAG_MODE - The composite battery tag is about to be read.
     *
     * POP_CB_QUERY_INFORMATION_MODE - The composite battery information is about to be read.
     *
     * POP_CB_QUERY_STATUS_MODE - The composite battery status is about to be read.
     *
     * POP_CB_QUERY_BATTERY_ESTIMATION_TIME_MODE - The composite battery estimation time is about to be read.
     *
     * POP_CB_QUERY_TEMPERATURE_MODE -- The composite battery temperature is about to be read.
     *
     * The operation modes are then translated to appropriate IOCTL codes for the composite battery
     * driver to understand what kind of I/O operation must be made.
     */
    UCHAR Mode;

    /*
     * The device power policy owner that owns the composite battery and the IRP that
     * transports battery I/O data to the said device. This device usually points to
     * the Composite Battery driver (COMPBATT).
     */
    PDEVICE_OBJECT DeviceObject;
    PIRP Irp;

    /*
     * Power action to be taken when a certain condition or circumstance is met that
     * triggered a power action. For the context of the composite battery, the trigger
     * is a certain battery capacity level when, it hits that capacity, it triggers
     * a power action.
     */
    POP_ACTION_TRIGGER Trigger;

    /* If this is set to TRUE, this indicates a I/O error occurred and CB operations cannot continue */
    BOOLEAN IoError;

    /* Battery information details */
    ULONG BatteryTag;
    ULONG Temperature;
    BATTERY_STATUS Status;
    BATTERY_INFORMATION BattInfo;

    /* The estimation of the battery time */
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
    /* List entry that links with the centalized power fans list */
    LIST_ENTRY Link;

    /* The owner device that actually created and owns this fan */
    PDEVICE_OBJECT DeviceObject;

    /* The power I/O packet request used to transport fan data */
    PIRP Irp;

    /* The current control percent of the fan, which is the speed level of the fan */
    UCHAR ControlPercent;

    /* The active cooling trip point that corresponds to a performance state of the fan */
    ULONG TripPoint;

    /* Indicates the current speed of the fan spinning, denoted in RPM */
    ULONG Speed;

    /*
     * The noise level currently produced by this fan. Note that not every system may
     * support this feature and this field may return 0xFFFFFFFF to indicate this fact.
     */
    ULONG NoiseLevel;

    /*
     * The power consumption of this fan, in milliwatts. Note that not every system may
     * support this feature and this field may return 0xFFFFFFFF to indicate this fact.
     */
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

    /* The minor power function code (IRP_MN_SET_POWER/IRP_MN_QUERY_POWER/IRP_MN_WAIT_WAKE) */
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
    /* List entry that links with the power IRP queue list */
    LIST_ENTRY Link;

    /* Contains data and information about the power IRP that is queued */
    PPOP_IRP_DATA IrpData;
} POP_IRP_QUEUE_ENTRY, *PPOP_IRP_QUEUE_ENTRY;

typedef struct _POP_IRP_THREAD_ENTRY
{
    /* List entry that links with the IRP thread entry list */
    LIST_ENTRY Link;

    /*
     * The thread and the power IRP. these fields are used for debugging purposes,
     * where Thread points to a IRP dispatcher worker thread that deploys the IRP
     * to the device driver for processing and Irp points to the actual power IRP
     * being processed.
     */
    PKTHREAD Thread;
    PIRP Irp;
} POP_IRP_THREAD_ENTRY, *PPOP_IRP_THREAD_ENTRY;

//
// Active power requests
//
typedef struct _POP_ACTIVE_POWER_REQUESTS
{
    /* Represents the count number of active "system required" requests */
    ULONG ActiveSystemRequiredRequests;

    /* Represents the count number of active "display required" requests */
    ULONG ActiveDisplayRequiredRequests;

    /* Represents the count number of active "away mode required" requests */
    ULONG ActiveAwayModeRequiredRequests;

    /* Represents the count number of active "execution" requests */
    ULONG ActiveExecutionRequests;

    /* Represents the count number of active "user present" requests */
    ULONG ActiveUserPresentRequests;
} POP_ACTIVE_POWER_REQUESTS;

//
// Power request object
//
typedef struct _POP_POWER_REQUEST
{
    /* A list entry that links with the global power requests list */
    LIST_ENTRY Link;

    /* The device that is requesting a power request */
    PDEVICE_OBJECT DeviceRequestor;

    /*
     * The reference count of this power request. For legacy power requests,
     * this count is served for statistical purposes. For modern power requests
     * though, this count is used to keep track of active references of a
     * power request.
     */
    ULONG UseCount;

    /*
     * If this field is to TRUE, the power request is a legacy request. Legacy requests
     * are made with API calls to PoRegisterSystemState and PoUnregisterSystemState which
     * tipically are used in 2000, XP and Server 2003 Windows versions.
     *
     * If this field is to FALSE, this is a Vista+ (modern) power request. These requests
     * are tipically made with API calls to PoCreatePowerRequest and friends. Windows 7,
     * 8, 8.1, 10 and 11 use them.
     */
    BOOLEAN Legacy;

    /*
     * If this field is set to TRUE, the power request is terminated and no longer counts
     * for overriding the default Power Manager actions, such as sleeping or idling.
     * The termination of power requests is usually done on AoAc systems. Power requests
     * are terminated by the Power Manager if a certain time limit threshold is reached.
     */
    BOOLEAN Terminate;

    /*
     * The requestor mode that determines if the said requestor was coming from user
     * mode or kernel mode. This field allows the Power Manager to create the power
     * request accordingly for both kernel and user modes (the latter typically coming
     * from a call to power request related user mode API functions).
     */
    KPROCESSOR_MODE RequestorMode;

    /* The state execution flags of this power request, reserved only for legacy requests */
    EXECUTION_STATE LegacyStateFlags;

    /* An entry of active requests this power request has currently made */
    POP_ACTIVE_POWER_REQUESTS ActiveRequests;

    /* The filename of an image process that made request for a power request, used for debugging purposes */
    UCHAR ImageFileName[16];

    /* Context reason provided by the requestor that describes the reason for this power request to be created */
    PCOUNTED_REASON_CONTEXT Context;
} POP_POWER_REQUEST, *PPOP_POWER_REQUEST;

//
// Power setting callback
//
typedef struct _POP_POWER_SETTING_CALLBACK
{
    /* A list entry that links with the global power settings list */
    LIST_ENTRY Link;

    /*
     * Bit flags that govern the current state and situation of a power setting
     * callback. The following flags are:
     *
     * POP_PSC_REGISTERED - The callback has been fully registered with the Power
     *                      Manager and its setting has been set for the first time.
     *                      This setting callback will further receive notifications
     *                      as soon as the Power Manager is making changes to the
     *                      respective power setting.
     *
     * POP_PSC_ENTERING_CALLBACK - Indicates the power setting callback is currently
     *                             into the driver's supplied callback routine as
     *                             due to a notification of a change in the power setting.
     *
     * POP_PSC_GETTING_NOTIFIED - The power setting callback is about to be notified soon
     *                            as the master worker will deploy a worker thread to serve
     *                            this power setting callback. The difference between this
     *                            and the flag above is that the setting callback is expecting
     *                            to be notified very soon whilst with the flag above, the callback
     *                            is already notified and is executing the driver's supplied callback
     *                            function.
     *
     * POP_PSC_UNREGISTERED -- The callback is marked as unregistered and is soon to be
     *                         delisted from the list and freed from memory.
     */
    ULONG Flags;

    /*
     * The Power Manager triggers this event when the power setting callback has
     * completed executing the callback routine.
     */
    KEVENT CallbackReturned;

    /* The device that registers a power setting callback for notifications */
    PDEVICE_OBJECT DeviceObject;

    /* The identification of the power setting this device (or callback) is registering for */
    GUID SettingGuid;

    /*
     * The power setting callback routine and the argument context that is passed to
     * the callback routine. While the device object is not necessarily mandatory to be
     * passed, the callback routine is though. The Power Manager calls this routine when
     * it is to notify the driver with this callback.
     */
    PPOWER_SETTING_CALLBACK Callback;
    PVOID Context;
} POP_POWER_SETTING_CALLBACK, *PPOP_POWER_SETTING_CALLBACK;

//
// Power setting notify block
//
typedef struct _POP_POWER_SETTING_NOTIFY_BLOCK
{
    /* The work item notification that is deployed to the power setting master worker */
    WORK_QUEUE_ITEM NotifyWorkItem;

    /*
     * The GUID identification of a power setting of which all the registered
     * callbacks for this power setting are to be notified. The master worker deploys
     * worker threads for each of the setting callback.
     */
    GUID NotifySettingGuid;
} POP_POWER_SETTING_NOTIFY_BLOCK, *PPOP_POWER_SETTING_NOTIFY_BLOCK;

//
// Power settings database
//
typedef struct _POP_POWER_SETTING_DATABASE
{
    /* Represents the identification of a power setting as a GUID */
    LPCGUID SettingGuid;

    /* The corer worker routine serving the power setting */
    PKSTART_ROUTINE SettingRoutine;
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

_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopUnimplementedPowerSettingWorker(
    _In_ PVOID StartContext);

_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopBatteryRemainingSettingWorker(
    _In_ PVOID StartContext);

_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopLidStateChangeSettingWorker(
    _In_ PVOID StartContext);

_Function_class_(KSTART_ROUTINE)
VOID
NTAPI
PopPowerSourceSettingWorker(
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

VOID
NTAPI
PopChangeSystemSystemStateCapability(
    _In_ PPOWER_STATE_HANDLER StateHandler,
    _In_ BOOLEAN Enable);

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

NTSTATUS
NTAPI
PopCreateWorkerThread(
    _In_ PKSTART_ROUTINE WorkerRoutine,
    _In_opt_ PVOID Context,
    _In_ KPRIORITY Priority);

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
    _In_ ULONG_PTR ProcessHandleCount,
    _In_ ULONG_PTR SystemHandleCount);

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

VOID
NTAPI
PopReleasePowerSettingCallback(
    _In_ _Post_invalid_ PPOP_POWER_SETTING_CALLBACK PowerSettingCallback);

PPOP_POWER_SETTING_CALLBACK
NTAPI
PopFindPowerSettingCallbackByCallback(
    _In_ PPOWER_SETTING_CALLBACK Callback);

VOID
NTAPI
PopNotifyPowerSettingChange(
    _In_ LPCGUID SettingGuid);

PKSTART_ROUTINE
NTAPI
PopGetPowerSettingHelper(
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
extern KSEMAPHORE PopWakeSourceResetSemaphore;
extern KEVENT PopWakeSourceResetComplete;

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
extern BOOLEAN PopCbConected;
extern PPOP_BATTERY PopBattery;

/* Power Manager Power Request constructs */
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

/* Power Manager Power Setting Callbacks constructs */
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
