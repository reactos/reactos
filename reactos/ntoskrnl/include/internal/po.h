/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/internal/po.h
* PURPOSE:         Internal header for the Power Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*/

#include <guiddef.h>
#include <poclass.h>

//
// Define this if you want debugging support
//
#define _PO_DEBUG_                                      0x00

//
// These define the Debug Masks Supported
//
#define PO_STATE_DEBUG                                  0x01

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
} PO_HIBER_PERF, *PPO_HIBER_PERF;

typedef struct _PO_MEMORY_IMAGE
{
    ULONG Signature;
    ULONG Version;
    ULONG CheckSum;
    ULONG LengthSelf;
    PFN_NUMBER PageSelf;
    ULONG PageSize;
    ULONG ImageType;
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
} PO_MEMORY_IMAGE, *PPO_MEMORY_IMAGE;

typedef struct _PO_MEMORY_RANGE_ARRAY_RANGE
{
    PFN_NUMBER PageNo;
    PFN_NUMBER StartPage;
    PFN_NUMBER EndPage;
    ULONG CheckSum;
} PO_MEMORY_RANGE_ARRAY_RANGE;

typedef struct _PO_MEMORY_RANGE_ARRAY_LINK
{
    struct _PO_MEMORY_RANGE_ARRAY *Next;
    PFN_NUMBER NextTable;
    ULONG CheckSum;
    ULONG EntryCount;
} PO_MEMORY_RANGE_ARRAY_LINK;

typedef struct _PO_MEMORY_RANGE_ARRAY
{
    union
    {
        PO_MEMORY_RANGE_ARRAY_RANGE Range;
        PO_MEMORY_RANGE_ARRAY_LINK Link;
    };
} PO_MEMORY_RANGE_ARRAY, *PPO_MEMORY_RANGE_ARRAY;

typedef struct _POP_HIBER_CONTEXT
{
    BOOLEAN WriteToFile;
    BOOLEAN ReserveLoaderMemory;
    BOOLEAN ReserveFreeMemory;
    BOOLEAN VerifyOnWake;
    BOOLEAN Reset;
    UCHAR HiberFlags;
    BOOLEAN LinkFile;
    HANDLE LinkFileHandle;
    PKSPIN_LOCK Lock;
    BOOLEAN MapFrozen;
    RTL_BITMAP MemoryMap;
    LIST_ENTRY ClonedRanges;
    ULONG ClonedRangeCount;
    PLIST_ENTRY NextCloneRange;
    PFN_NUMBER NextPreserve;
    PMDL LoaderMdl;
    PMDL Clones;
    PUCHAR NextClone;
    ULONG NoClones;
    PMDL Spares;
    ULONGLONG PagesOut;
    PVOID IoPage;
    PVOID CurrentMcb;
    PVOID DumpStack;
    PKPROCESSOR_STATE WakeState;
    ULONG NoRanges;
    ULONG_PTR HiberVa;
    PHYSICAL_ADDRESS HiberPte;
    NTSTATUS Status;
    PPO_MEMORY_IMAGE MemoryImage;
    PPO_MEMORY_RANGE_ARRAY TableHead;
    PVOID CompressionWorkspace;
    PUCHAR CompressedWriteBuffer;
    PULONG PerformanceStats;
    PVOID CompressionBlock;
    PVOID DmaIO;
    PVOID TemporaryHeap;
    PO_HIBER_PERF PerfInfo;
} POP_HIBER_CONTEXT, *PPOP_HIBER_CONTEXT;

typedef struct _PO_NOTIFY_ORDER_LEVEL
{
    KEVENT LevelReady;
    ULONG DeviceCount;
    ULONG ActiveCount;
    LIST_ENTRY WaitSleep;
    LIST_ENTRY ReadySleep;
    LIST_ENTRY Pending;
    LIST_ENTRY Complete;
    LIST_ENTRY ReadyS0;
    LIST_ENTRY WaitS0;
} PO_NOTIFY_ORDER_LEVEL, *PPO_NOTIFY_ORDER_LEVEL;

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

typedef struct _POP_DEVICE_POWER_IRP
{
    SINGLE_LIST_ENTRY Free;
    PIRP Irp;
    PPO_DEVICE_NOTIFY Notify;
    LIST_ENTRY Pending;
    LIST_ENTRY Complete;
    LIST_ENTRY Abort;
    LIST_ENTRY Failed;
} POP_DEVICE_POWER_IRP, *PPOP_DEVICE_POWER_IRP;

typedef struct _PO_DEVICE_NOTIFY_ORDER
{
    ULONG DevNodeSequence;
    PDEVICE_OBJECT *WarmEjectPdoPointer;
    PO_NOTIFY_ORDER_LEVEL OrderLevel[8];
} PO_DEVICE_NOTIFY_ORDER, *PPO_DEVICE_NOTIFY_ORDER;

typedef struct _POP_DEVICE_SYS_STATE
{
    UCHAR IrpMinor;
    SYSTEM_POWER_STATE SystemState;
    PKEVENT Event;
    KSPIN_LOCK SpinLock;
    PKTHREAD Thread;
    BOOLEAN GetNewDeviceList;
    PO_DEVICE_NOTIFY_ORDER Order;
    NTSTATUS Status;
    PDEVICE_OBJECT FailedDevice;
    BOOLEAN Waking;
    BOOLEAN Cancelled;
    BOOLEAN IgnoreErrors;
    BOOLEAN IgnoreNotImplemented;
    BOOLEAN _WaitAny;
    BOOLEAN _WaitAll;
    LIST_ENTRY PresentIrpQueue;
    POP_DEVICE_POWER_IRP Head;
    POP_DEVICE_POWER_IRP PowerIrpState[20];
} POP_DEVICE_SYS_STATE, *PPOP_DEVICE_SYS_STATE;

typedef struct _POP_POWER_ACTION
{
    UCHAR Updates;
    UCHAR State;
    BOOLEAN Shutdown;
    POWER_ACTION Action;
    SYSTEM_POWER_STATE LightestState;
    ULONG Flags;
    NTSTATUS Status;
    UCHAR IrpMinor;
    SYSTEM_POWER_STATE SystemState;
    SYSTEM_POWER_STATE NextSystemState;
    PPOP_SHUTDOWN_BUG_CHECK ShutdownBugCode;
    PPOP_DEVICE_SYS_STATE DevState;
    PPOP_HIBER_CONTEXT HiberContext;
    ULONGLONG WakeTime;
    ULONGLONG SleepTime;
} POP_POWER_ACTION, *PPOP_POWER_ACTION;

typedef enum _POP_DEVICE_IDLE_TYPE
{
    DeviceIdleNormal,
    DeviceIdleDisk,
} POP_DEVICE_IDLE_TYPE, *PPOP_DEVICE_IDLE_TYPE;

typedef struct _POWER_CHANNEL_SUMMARY
{
    ULONG Signature;
    ULONG TotalCount;
    ULONG D0Count;
    LIST_ENTRY NotifyList;
} POWER_CHANNEL_SUMMARY, *PPOWER_CHANNEL_SUMMARY;

typedef struct  _DEVICE_OBJECT_POWER_EXTENSION
{
    ULONG IdleCount;
    ULONG ConservationIdleTime;
    ULONG PerformanceIdleTime;
    PDEVICE_OBJECT DeviceObject;
    LIST_ENTRY IdleList;
    DEVICE_POWER_STATE State;
    LIST_ENTRY NotifySourceList;
    LIST_ENTRY NotifyTargetList;
    POWER_CHANNEL_SUMMARY PowerChannelSummary;
    LIST_ENTRY Volume;
} DEVICE_OBJECT_POWER_EXTENSION, *PDEVICE_OBJECT_POWER_EXTENSION;

typedef struct _POP_SHUTDOWN_WAIT_ENTRY
{
    struct _POP_SHUTDOWN_WAIT_ENTRY *NextEntry;
    PETHREAD Thread;
} POP_SHUTDOWN_WAIT_ENTRY, *PPOP_SHUTDOWN_WAIT_ENTRY;

//
// Initialization routines
//
BOOLEAN
NTAPI
PoInitSystem(
    IN ULONG BootPhase
);

VOID
NTAPI
PoInitializePrcb(
    IN PKPRCB Prcb
);

VOID
NTAPI
PopInitShutdownList(
    VOID
);

//
// I/O Routines
//
VOID
NTAPI
PoInitializeDeviceObject(
    IN OUT PDEVOBJ_EXTENSION DeviceObjectExtension
);

VOID
NTAPI
PoVolumeDevice(
    IN PDEVICE_OBJECT DeviceObject
);

VOID
NTAPI
PoRemoveVolumeDevice(
    IN PDEVICE_OBJECT DeviceObject);

//
// Power State routines
//
NTSTATUS
NTAPI
PopSetSystemPowerState(
    SYSTEM_POWER_STATE PowerState,
    POWER_ACTION PowerAction
);

VOID
NTAPI
PopCleanupPowerState(
    IN PPOWER_STATE PowerState
);

NTSTATUS
NTAPI
PopAddRemoveSysCapsCallback(
    IN PVOID NotificationStructure,
    IN PVOID Context
);

//
// Notifications
//
VOID
NTAPI
PoNotifySystemTimeSet(
    VOID
);

//
// Shutdown routines
//
VOID
NTAPI
PopReadShutdownPolicy(
    VOID
);

VOID
NTAPI
PopGracefulShutdown(
    IN PVOID Context
);

VOID
NTAPI
PopFlushVolumes(
    IN BOOLEAN ShuttingDown
);

//
// Global data inside the Power Manager
//
extern PDEVICE_NODE PopSystemPowerDeviceNode;
extern KGUARDED_MUTEX PopVolumeLock;
extern LIST_ENTRY PopVolumeDevices;
extern KSPIN_LOCK PopDopeGlobalLock;
extern POP_POWER_ACTION PopAction;

