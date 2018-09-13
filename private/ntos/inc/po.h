/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation
Copyright (c) 1994  International Business Machines Corporation

Module Name:

    po.h

Abstract:

    This module contains the internal structure definitions and APIs used by
    the NT Poewr Management.

Author:

    Ken Reneris (kenr) 19-July-1994
    N. Yoshiyama [IBM Corp.] 01-Mar-1994


Revision History:


--*/



#ifndef _PO_
#define _PO_

#if DBG

VOID
PoPowerTracePrint(
    ULONG    TracePoint,
    ULONG_PTR Caller,
    ULONG_PTR CallerCaller,
    ULONG_PTR DeviceObject,
    ULONG_PTR Irp,
    ULONG_PTR Ios
    );

#define PoPowerTrace(TracePoint,DevObj,Arg1,Arg2) \
{\
    PVOID pptcCaller;      \
    PVOID pptcCallerCaller;  \
    RtlGetCallersAddress(&pptcCaller, &pptcCallerCaller); \
    PoPowerTracePrint(TracePoint, (ULONG_PTR)pptcCaller, (ULONG_PTR)pptcCallerCaller, (ULONG_PTR)DevObj, (ULONG_PTR)Arg1, (ULONG_PTR)Arg2); \
}
#else
#define PoPowerTrace(TracePoint,DevObj,Arg1,Arg2)
#endif

#define POWERTRACE_CALL         0x1
#define POWERTRACE_PRESENT      0x2
#define POWERTRACE_STARTNEXT    0x4
#define POWERTRACE_SETSTATE     0x8
#define POWERTRACE_COMPLETE     0x10


VOID
FASTCALL
PoInitializePrcb (
    PKPRCB      Prcb
    );

NTKERNELAPI
BOOLEAN
PoInitSystem (
    IN ULONG    Phase
    );

VOID
PoInitDriverServices (
    IN ULONG    Phase
    );

VOID
PoInitHiberServices (
    IN BOOLEAN  Setup
    );

NTKERNELAPI
VOID
PoInitializeDeviceObject (
    IN PDEVOBJ_EXTENSION   DeviceObjectExtension
    );

NTKERNELAPI
VOID
PoRunDownDeviceObject (
    IN PDEVICE_OBJECT   DeviceObject
    );

NTKERNELAPI
VOID
PopCleanupPowerState (
    IN OUT PUCHAR PowerState
    );

#define PoRundownThread(Thread)     \
        PopCleanupPowerState(&Thread->Tcb.PowerState)

#define PoRundownProcess(Process)   \
        PopCleanupPowerState(&Process->Pcb.PowerState)

VOID
PoNotifySystemTimeSet (
    VOID
    );

VOID
PoInvalidateDevicePowerRelations(
    PDEVICE_OBJECT  DeviceObject
    );

VOID
PoShutdownBugCheck (
    IN BOOLEAN  AllowCrashDump,
    IN ULONG    BugCheckCode,
    IN ULONG_PTR BugCheckParameter1,
    IN ULONG_PTR BugCheckParameter2,
    IN ULONG_PTR BugCheckParameter3,
    IN ULONG_PTR BugCheckParameter4
    );

// begin_nthal

NTKERNELAPI
VOID
PoSetHiberRange (
    IN PVOID     MemoryMap,
    IN ULONG     Flags,
    IN PVOID     Address,
    IN ULONG_PTR Length,
    IN ULONG     Tag
    );

// memory_range.Type
#define PO_MEM_PRESERVE         0x00000001      // memory range needs preserved
#define PO_MEM_CLONE            0x00000002      // Clone this range
#define PO_MEM_CL_OR_NCHK       0x00000004      // Either clone or do not checksum
#define PO_MEM_DISCARD          0x00008000      // This range to be removed
#define PO_MEM_PAGE_ADDRESS     0x00004000      // Arguments passed are physical pages

// end_nthal

#define PoWakeTimerSupported()  \
    (PopCapabilities.RtcWake >= PowerSystemSleeping1)

ULONG
PoSimpleCheck (
    IN ULONG                PatialSum,
    IN PVOID                StartVa,
    IN ULONG_PTR            Length
    );

BOOLEAN
PoSystemIdleWorker (
    IN BOOLEAN IdleWorker
    );

VOID
PoVolumeDevice(
    IN PDEVICE_OBJECT DeviceObject
    );

VOID
PoSetWarmEjectDevice(
    IN PDEVICE_OBJECT DeviceObject
    ) ;

NTSTATUS
PoGetLightestSystemStateForEject(
    IN   BOOLEAN              DockBeingEjected,
    IN   BOOLEAN              HotEjectSupported,
    IN   BOOLEAN              WarmEjectSupported,
    OUT  PSYSTEM_POWER_STATE  LightestSleepState
    );

// begin_ntddk begin_wdm

NTKERNELAPI
VOID
PoSetSystemState (
    IN EXECUTION_STATE Flags
    );

NTKERNELAPI
PVOID
PoRegisterSystemState (
    IN PVOID StateHandle,
    IN EXECUTION_STATE Flags
    );

typedef
VOID
(*PREQUEST_POWER_COMPLETE) (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PVOID Context,
    IN PIO_STATUS_BLOCK IoStatus
    );

NTKERNELAPI
NTSTATUS
PoRequestPowerIrp (
    IN PDEVICE_OBJECT DeviceObject,
    IN UCHAR MinorFunction,
    IN POWER_STATE PowerState,
    IN PREQUEST_POWER_COMPLETE CompletionFunction,
    IN PVOID Context,
    OUT PIRP *Irp OPTIONAL
    );

NTKERNELAPI
VOID
PoUnregisterSystemState (
    IN PVOID StateHandle
    );

// end_ntddk end_wdm
// begin_ntddk begin_wdm begin_nthal begin_ntifs

NTKERNELAPI
POWER_STATE
PoSetPowerState (
    IN PDEVICE_OBJECT   DeviceObject,
    IN POWER_STATE_TYPE Type,
    IN POWER_STATE      State
    );

NTKERNELAPI
NTSTATUS
PoCallDriver (
    IN PDEVICE_OBJECT   DeviceObject,
    IN OUT PIRP         Irp
    );

NTKERNELAPI
VOID
PoStartNextPowerIrp(
    IN PIRP    Irp
    );


NTKERNELAPI
PULONG
PoRegisterDeviceForIdleDetection (
    IN PDEVICE_OBJECT     DeviceObject,
    IN ULONG              ConservationIdleTime,
    IN ULONG              PerformanceIdleTime,
    IN DEVICE_POWER_STATE State
    );

#define PoSetDeviceBusy(IdlePointer) \
    *IdlePointer = 0

//
// \Callback\PowerState values
//

#define PO_CB_SYSTEM_POWER_POLICY   0
#define PO_CB_AC_STATUS             1
#define PO_CB_BUTTON_COLLISION      2
#define PO_CB_SYSTEM_STATE_LOCK     3


// end_ntddk end_wdm end_nthal end_ntifs

//
// Broken functions we don't intend on supporting. The code backing these
// should be ripped out in NT5.1
//
typedef
VOID
(*PPO_NOTIFY) (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PVOID            Context,
    IN ULONG            Type,
    IN ULONG            Reserved
    );

#define PO_NOTIFY_D0                        0x00000001
#define PO_NOTIFY_TRANSITIONING_FROM_D0     0x00000002
#define PO_NOTIFY_INVALID                   0x80000000

NTKERNELAPI
NTSTATUS
PoRegisterDeviceNotify (
    IN PDEVICE_OBJECT   DeviceObject,
    IN PPO_NOTIFY       NotificationFunction,
    IN PVOID            NotificationContext,
    IN ULONG            NotificationType,
    OUT PDEVICE_POWER_STATE  DeviceState,
    OUT PVOID           *NotificationHandle
    );

NTKERNELAPI
NTSTATUS
PoCancelDeviceNotify (
    IN PVOID            NotificationHandle
    );


//
// Callout set state failure notification
//

typedef struct {
    NTSTATUS            Status;
    POWER_ACTION        PowerAction;
    SYSTEM_POWER_STATE  MinState;
    ULONG               Flags;
} PO_SET_STATE_FAILURE, *PPO_SET_STATE_FAILURE;



//
// Hibernation file layout:
//      Page 0  - PO_MEMORY_IMAGE
//      Page 1  - Free page array
//      Page 2  - KPROCESSOR_CONTEXT
//      Page 3  - first memory_range_array page
//
// PO_MEMORY_IMAGE:
//      Header on file which contains some information to identify
//      the hibernation, as well as a couple of checksums.
//
// Free page array:
//      A page full of page numbers which identify 4MBs worth of
//      system pages that are not in the restoration image.  These
//      pages are used by the loader (to its self out of the way)
//      when restoring the memory image.
//
// KPROCESSOR_CONTEST
//      The context of the processor which hibernated the system.
//      Rest of page is empty.
//
// memory_range_array
//      A page which contains an array of memory_range_array elements
//      where element 0 is a Link entry, and all other elements are
//      Range entries.   The Link entry is used to link to the next
//      such page, and to supply a count of the number of Range entries
//      in the current page.   The range entries each describe one
//      physical memory range which needs restoration and its location
//      in the file.
//

typedef struct _PO_MEMORY_RANGE_ARRAY {
    union {
        struct {
            PFN_NUMBER      PageNo;
            PFN_NUMBER      StartPage;
            PFN_NUMBER      EndPage;
            ULONG           CheckSum;
        } Range;
        struct {
            struct _PO_MEMORY_RANGE_ARRAY *Next;
            PFN_NUMBER      NextTable;
            ULONG           CheckSum;
            ULONG           EntryCount;
        } Link;
    };
} PO_MEMORY_RANGE_ARRAY, *PPO_MEMORY_RANGE_ARRAY;

#define PO_MAX_RANGE_ARRAY  (PAGE_SIZE / sizeof(PO_MEMORY_RANGE_ARRAY))
#define PO_ENTRIES_PER_PAGE (PO_MAX_RANGE_ARRAY-1)


#define PO_IMAGE_SIGNATURE          'rbih'
#define PO_IMAGE_SIGNATURE_WAKE     'ekaw'
#define PO_IMAGE_SIGNATURE_BREAK    'pkrb'
#define PO_IMAGE_SIGNATURE_LINK     'knil'
#define PO_IMAGE_HEADER_PAGE        0
#define PO_FREE_MAP_PAGE            1
#define PO_PROCESSOR_CONTEXT_PAGE   2
#define PO_FIRST_RANGE_TABLE_PAGE   3

#define PO_COMPRESS_CHUNK_SIZE      4096

//
// Define various HiberFlags to control the behavior when restoring
//
#define PO_HIBER_APM_RECONNECT      1

typedef struct {
    ULONG                   Signature;
    ULONG                   Version;
    ULONG                   CheckSum;
    ULONG                   LengthSelf;
    PFN_NUMBER              PageSelf;
    ULONG                   PageSize;

    ULONG                   ImageType;
    LARGE_INTEGER           SystemTime;
    ULONGLONG               InterruptTime;
    ULONG                   FeatureFlags;
    UCHAR                   HiberFlags;
    UCHAR                   spare[3];

    ULONG                   NoHiberPtes;
    ULONG_PTR               HiberVa;
    PHYSICAL_ADDRESS        HiberPte;

    ULONG                   NoFreePages;
    ULONG                   FreeMapCheck;
    ULONG                   WakeCheck;

    PFN_NUMBER              TotalPages;
    PFN_NUMBER              FirstTablePage;
    PFN_NUMBER              LastFilePage;
} PO_MEMORY_IMAGE, *PPO_MEMORY_IMAGE;

typedef struct {
    ULONG                   Signature;
    WCHAR                   Name[1];
} PO_IMAGE_LINK, *PPO_IMAGE_LINK;

//
// Returned by Io system
//

typedef struct _PO_DEVICE_NOTIFY {
    LIST_ENTRY              Link;
    PDEVICE_OBJECT          TargetDevice;

    BOOLEAN                 WakeNeeded;
    UCHAR                   OrderLevel;

    PDEVICE_OBJECT          DeviceObject;
    PVOID                   Node;
    PWCHAR                  DeviceName;
    PWCHAR                  DriverName;

    LONG                    NodeLevel;
} PO_DEVICE_NOTIFY, *PPO_DEVICE_NOTIFY;

#define PO_ORDER_NOT_VIDEO          0x0001
#define PO_ORDER_ROOT_ENUM          0x0002
#define PO_ORDER_PAGABLE            0x0004
#define PO_ORDER_MAXIMUM            0x0007

// notify GDI before this order level
#define PO_ORDER_GDI_NOTIFICATION   (PO_ORDER_PAGABLE)

typedef struct _PO_DEVICE_NOTIFY_ORDER {
    ULONG                   DevNodeSequence;
    ULONG                   NoLists;
    LONG                    MaxLevel;
    PDEVICE_OBJECT          *WarmEjectPdoPointer;

    LIST_ENTRY              Partial;
    LIST_ENTRY              Rebase;
    PLIST_ENTRY             Notify;
} PO_DEVICE_NOTIFY_ORDER, *PPO_DEVICE_NOTIFY_ORDER;

extern KAFFINITY        PoSleepingSummary;
extern BOOLEAN          PoEnabled;
extern ULONG            PoPowerSequence;
extern BOOLEAN          PoPageLockData;
extern KTIMER           PoSystemIdleTimer;
extern BOOLEAN          PoHiberInProgress;

// PopCapabilities used by some internal macros
extern SYSTEM_POWER_CAPABILITIES PopCapabilities;

#endif

