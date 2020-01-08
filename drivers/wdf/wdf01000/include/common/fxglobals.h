#ifndef _FXGLOBALS_H
#define _FXGLOBALS_H

#include <ntddk.h>
#include "wdf.h"
#include "common/fxldr.h"
#include "fxbugcheck.h"
#include "common/mxlock.h"
#include "common/fxtypes.h"
#include "common/fxpool.h"
#include "common/mxgeneral.h"

#ifdef __cplusplus
extern "C" {
#endif

const LONG FX_OBJECT_LEAK_DETECTION_DISABLED = 0xFFFFFFFF;

struct FxLibraryGlobalsType;
class FxObject;
class FxDriver;
extern RTL_OSVERSIONINFOW  gOsVersion;
extern PCCH WdfLdrType;

#define DDI_ENTRY_IMPERSONATION_OK()
#define DDI_ENTRY()

NTSTATUS
FxLibraryGlobalsCommission(VOID);

VOID
FxLibraryGlobalsDecommission(VOID);

VOID
FxInitializeBugCheckDriverInfo();

VOID
FxUninitializeBugCheckDriverInfo();

VOID
FxDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

void
FxVerifierLockDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

VOID
FxUnregisterBugCheckCallback(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals
    );

VOID
FxPurgeBugCheckDriverInfo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
VOID
UnlockVerifierSection(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    );
#endif

_Must_inspect_result_
NTSTATUS
FxInitialize(
    __inout     PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        MdDriverObject DriverObject,
    __in        PCUNICODE_STRING RegistryPath,
    __in_opt    PWDF_DRIVER_CONFIG DriverConfig //optional in user mode
    );

BOOLEAN
IsWindowsVerifierOn(
    _In_ MdDriverObject DriverObject
    );

void
FxVerifierLockInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

VOID
FxCacheBugCheckDriverInfo(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

VOID
FxRegisterBugCheckCallback(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in    PDRIVER_OBJECT DriverObject
    );

PCSTR
FxObjectTypeToHandleName(
    __in WDFTYPE ObjectType
    );

_Must_inspect_result_
BOOLEAN
FxVerifyObjectTypeInTable(
    __in USHORT ObjectType
    );




struct FxMdlDebugInfo {
    PMDL Mdl;
    FxObject* Owner;
    PVOID Caller;
};

#define NUM_MDLS_IN_INFO (16)

struct FxAllocatedMdls {
    FxMdlDebugInfo Info[NUM_MDLS_IN_INFO];
    ULONG Count;
    struct FxAllocatedMdls* Next;
};

//
// NOTE: any time you add a value to this enum, you must add a field to the
// union in FxObjectDebugInfo.
//
enum FxObjectDebugInfoFlags {
    FxObjectDebugTrackReferences = 0x0001,
    FxObjectDebugTrackObjectCount = 0x0002,
};

struct FxObjectDebugInfo {
    //
    // FX_OBJECT_TYPES enum value
    //
    USHORT ObjectType;

    union {
        //
        // Combo of values from FxObjectDebugInfoFlags
        //
        USHORT DebugFlags;

        //
        // Break out of DebugFlags as individual fields.  This is used by the
        // debugger extension to reference the values w/out knowing the actual
        // enum values.
        //
        struct {
            USHORT TrackReferences : 1;
            USHORT TrackObjectCountForLeak : 1;
        } Bits;
    } u;
};

_Must_inspect_result_
BOOLEAN
FxVerifierIsDebugInfoFlagSetForType(
    _In_ FxObjectDebugInfo* DebugInfo,
    _In_ WDFTYPE ObjectType,
    _In_ FxObjectDebugInfoFlags Flag
    );

struct FxDriverGlobalsDebugExtension {
    //
    // Debug information per object.  List is sorted by
    // FxObjectDebugInfo::ObjectType, length is the same as FxObjectsInfo.
    //
    FxObjectDebugInfo* ObjectDebugInfo;

    //
    // Track allocated Mdls only in kernel mode version
    //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    FxAllocatedMdls AllocatedMdls;

    KSPIN_LOCK AllocatedMdlsLock;
#endif

    //
    // List of all allocated tag trackers for this driver.  This is used to keep
    // track of orphaned objects due to leaked reference counts in the debugger
    // extension.
    //
    LIST_ENTRY AllocatedTagTrackersListHead;

    //
    // Synchronizes access to AllocatedTagTrackersListHead
    //
    MxLock AllocatedTagTrackersLock;
};

VOID
FxFreeAllocatedMdlsDebugInfo(
    __in FxDriverGlobalsDebugExtension* DebugExtension
    );

typedef struct _FX_DRIVER_GLOBALS
{
public:
	VOID
    SetVerifierState(
        __in BOOLEAN State
        )
    {
        //
        // Master switch
        //
        FxVerifierOn                = State;

        FxVerifierHandle            = State;
        FxVerifierIO                = State;
        FxVerifierLock              = State;
        FxPoolTrackingOn            = State;

        //
        // Following can be overridden by the registry settings
        // WDFVERIFY matches the state of the verifier.
        //
        FxVerifyOn                  = State;
        FxVerifierDbgBreakOnError   = State;
        FxVerifierDbgBreakOnDeviceStateError = FALSE;

        //
        // Set the public flags for consumption by client drivers.
        //
        if (State)
		{
			//Public.DriverFlags |= (WdfVerifyOn | WdfVerifierOn);
            Public.DriverFlags |= 0xC;
        }
    }

	BOOLEAN
    IsPoolTrackingOn(
        VOID
        )
    {
        return (FxPoolTrackingOn) ? TRUE : FALSE;
    }

    BOOLEAN
    IsObjectDebugOn(
        VOID
        )
    {
        if (FxVerifierHandle)
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

	_Must_inspect_result_
    BOOLEAN
    IsVersionGreaterThanOrEqualTo(
        __in ULONG  Major,
        __in ULONG  Minor
        )
	{
		if ((WdfBindInfo->Version.Major > Major) ||
                (WdfBindInfo->Version.Major == Major &&
                  WdfBindInfo->Version.Minor >= Minor))
		{
        	return TRUE;
    	}
    	else
		{
        	return FALSE;
    	}
	}
	

	//
    // Link list of driver FxDriverGlobals on this WDF Version.
    //
	LIST_ENTRY Linkage;

	//
    // Mask to XOR all outgoing handles against
    //
	ULONG_PTR WdfHandleMask;

	//
    // If verifier is on, this is the count of allocations
    // to fail at
    //
	LONG WdfVerifierAllocateFailCount;

	//
    // Tag to be used for allocations on behalf of the driver writer.  This is
    // based off of the service name (which might be different than the binary
    // name).
    //
	ULONG Tag;

	//
    // Backpointer to Fx driver object
    //
	FxDriver* Driver;
	FxDriverGlobalsDebugExtension* DebugExtension;
	FxLibraryGlobalsType* LibraryGlobals;
	ULONG WdfTraceDelayTime;

	//
    // WDF internal In-Flight Recorder (IFR) log
    //
	PVOID WdfLogHeader;

	//
    // The driver's memory pool header
    //
	FX_POOL FxPoolFrameworks;

	//
    // Framworks Pool Tracking
    //
	BOOLEAN FxPoolTrackingOn;

	//
    // FxVerifierLock per driver state
    //
	MxLock ThreadTableLock;
	PLIST_ENTRY ThreadTable;

	//
    // Embedded pointer to driver's WDF_BIND_INFO structure (in stub)
    //
	PWDF_BIND_INFO WdfBindInfo;

	//
    // The base address of the image.
    //
	PVOID ImageAddress;

	//
    // The size of the image.
    //
	ULONG ImageSize;

	//
    // Top level verifier flag.
    //
	BOOLEAN FxVerifierOn;

	//
    // Apply latest-version-restricted verifier checks to downlevel drivers.
    // Drivers set this value in registry.
    //
	BOOLEAN FxVerifyDownlevel;

	//
    // Breakpoint on errors.
    //
	BOOLEAN FxVerifierDbgBreakOnError;

	//
    // Breakpoint on device state errors.
    //
	BOOLEAN FxVerifierDbgBreakOnDeviceStateError;

	//
    // Handle verifier.
    //
	BOOLEAN FxVerifierHandle;

	//
    // I/O verifier.
    //
	BOOLEAN FxVerifierIO;

	//
    // Lock verifier.
    //
	BOOLEAN FxVerifierLock;

	//
    // Not a verifier option.  Rather, controls whether WDFVERIFY macros are
    // live.
    //
	BOOLEAN FxVerifyOn;

	//
    // Capture IFR Verbose messages.
    //
	BOOLEAN FxVerboseOn;

	//
    // Force copy of IFR data to mini-dump when a bugcheck happens.
    //
	BOOLEAN FxForceLogsInMiniDump;

	//
    // Parent queue presented requests (to device).
    //
	BOOLEAN FxRequestParentOptimizationOn;

	//
    // TRUE to enable run-time driver tracking. The dump callback logic
    // uses this info for finding the right log to write in the minidump.
    //
	BOOLEAN FxTrackDriverForMiniDumpLog;

	//
    // 0-based index into the BugCheckDriverInfo holding this driver info.
    //
	ULONG BugCheckDriverInfoIndex;

	//
    // Bug check callback record for processing bugchecks.
    //
	KBUGCHECK_REASON_CALLBACK_RECORD BugCheckCallbackRecord;

	//
    // Enhanced Verifier Options.
    //
	ULONG FxEnhancedVerifierOptions;

	//
    // The public version of WDF_DRIVER_GLOBALS
    //
	DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) WDF_DRIVER_GLOBALS Public;
} FX_DRIVER_GLOBALS, *PFX_DRIVER_GLOBALS;

__bcount(Size)
PVOID
FORCEINLINE
FxPoolAllocateWithTag(
    __in PFX_DRIVER_GLOBALS Globals,
    __in POOL_TYPE Type,
    __in size_t Size,
    __in ULONG Tag
    )
{
    return FxPoolAllocator(
        Globals,
        &Globals->FxPoolFrameworks,
        Type,
        Size,
        Tag,
        Globals->FxPoolTrackingOn ? _ReturnAddress() : NULL
        );
}

enum FxMachineSleepStates
{
    FxMachineS1Index = 0,
    FxMachineS2Index,
    FxMachineS3Index,
    FxMachineSleepStatesMax,
};

typedef
BOOLEAN
(*PFNKDREFRESH)(
	VOID
	);

typedef
VOID
(*PFNKEFLUSHQUEUEDDPCS)(
	VOID
	);

typedef
NTSTATUS
(*PFNIOSETCOMPLETIONROUTINEEX)(
	PDEVICE_OBJECT, PIRP, PIO_COMPLETION_ROUTINE, PVOID, BOOLEAN, BOOLEAN, BOOLEAN
	);

typedef
KIRQL
(*PFNKEACQUIREINTERRRUPTSPINLOCK)(
	PKINTERRUPT
	);

typedef
VOID
(*PFNKERELEASEINTERRUPTSPINLOCK)(
	PKINTERRUPT, KIRQL
	);

typedef
NTSTATUS
(*PFNIOCONNECTINTERRUPTEX)(
	PIO_CONNECT_INTERRUPT_PARAMETERS
	);

typedef
VOID
(*PFNIODISCONNECTINTERRUPTEX)(
	PIO_DISCONNECT_INTERRUPT_PARAMETERS
	);

typedef
KIRQL
(*PFNKFRAISEIRQL)(
	KIRQL
	);

typedef
VOID
(FASTCALL *PFNKKFLOWERIRQL)(
	KIRQL
	);

typedef
PSINGLE_LIST_ENTRY
(FASTCALL *PFNINTERLOCKEDPOPENTRYSLIST)(
	PSLIST_HEADER
	);

typedef
BOOLEAN
(FASTCALL *PFNINTERLOCKEDPUSHENTRYSLIST)(
	PSLIST_HEADER, PSINGLE_LIST_ENTRY
	);

typedef
PSINGLE_LIST_ENTRY
(*PFNPOGETSYSTEMWAKE)(
	PIRP
	);

typedef
VOID
(*PFNPOSETSYSTEMWAKE)(
	PIRP
	);

typedef
KAFFINITY
(*PFNKEQUERYACTIVEPROCESSORS)(
	VOID
	);

typedef
VOID
(*PFNKESETTARGETPROCESSORDPC)(
	PKDPC, CCHAR
	);

typedef
BOOLEAN
(*PFNKESETCOALESCABLETIMER)(
	PKTIMER, LARGE_INTEGER, ULONG, ULONG, PKDPC
	);

typedef
BOOLEAN
(*PFNKEAREAPCSDISABLED)(
	VOID
	);

typedef
NTSTATUS
(*PFNIOUNREGISTERPLUGPLAYNOTIFICATIONEX)(
	PVOID
	);

typedef
NTSTATUS
(*PFNNTWWIQUERYTRACEINFORMATION)(
	TRACE_INFORMATION_CLASS, PVOID, ULONG, PULONG, PVOID
	);

typedef
NTSTATUS
(*PFNWMITRACEMESSAGEVA)(
	TRACEHANDLE, ULONG, LPCGUID, USHORT, va_list
	);

typedef struct _FX_DRIVER_TRACKER_CACHE_AWARE {
	//
    // Internal data types.
    //
private:
    typedef struct _FX_DRIVER_TRACKER_ENTRY {
         volatile PFX_DRIVER_GLOBALS FxDriverGlobals;
    } FX_DRIVER_TRACKER_ENTRY, *PFX_DRIVER_TRACKER_ENTRY;

public:
    _Must_inspect_result_
    NTSTATUS
    Initialize();

	VOID
    Uninitialize();	

	VOID
    Deregister(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        );

    //
    // Tracks the driver usage on the current processor.
    // KeGetCurrentProcessorNumberEx is called directly because the procgrp.lib
    // provides the downlevel support for Vista, XP and Win2000.
    //
    __inline
    VOID
    TrackDriver(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals
        )
    {
        ASSERT(m_PoolToFree != NULL);

        //
        // TODO: KeGetCurrentProcessorIndex
        //
        //GetProcessorDriverEntryRef(
        //    KeGetCurrentProcessorIndex())->FxDriverGlobals =
        //        FxDriverGlobals;
    }

private:
    //
    // Returns the per-processor cache-aligned driver usage ref structure for
    // given processor.
    //
    __inline
    PFX_DRIVER_TRACKER_ENTRY
    GetProcessorDriverEntryRef(
        __in ULONG Index
        )
    {
        return ((PFX_DRIVER_TRACKER_ENTRY) (((PUCHAR)m_DriverUsage) +
                    Index * m_EntrySize));
    }

	//
    // Data members.
    //
private:
    //
    // Pointer to array of cache-line aligned tracking driver structures.
    //
    PFX_DRIVER_TRACKER_ENTRY    m_DriverUsage;

    //
    // Points to pool of per-proc tracking entries that needs to be freed.
    //
    PVOID                       m_PoolToFree;

    //
    // Size of each padded tracking driver structure.
    //
    ULONG                       m_EntrySize;

    //
    // Indicates # of entries in the array of tracking driver structures.
    //
    ULONG                       m_Number;
} FX_DRIVER_TRACKER_CACHE_AWARE, *PFX_DRIVER_TRACKER_CACHE_AWARE;


struct FxLibraryGlobalsType
{
	int FxInitialized;
	//
    // The driver object for the library.
    //
	PDRIVER_OBJECT DriverObject;
	
	//
    // As long as this device object is around, the library cannot be unloaded.
    // This prevents the following scenario from unloading the service
    // 1  wdfldr.sys loads the library
    // 2  user tries to run "net stop <service>" while there are outstanding clients
    //    through wdfldr
    //
	PDEVICE_OBJECT LibraryDeviceObject;

	PFNKDREFRESH pfn_KdRefresh;
	PFNKEFLUSHQUEUEDDPCS pfn_KeFlushQueuedDpcs;
	PFNIOSETCOMPLETIONROUTINEEX pfn_IoSetCompletionRoutineEx;
	PFNKEACQUIREINTERRRUPTSPINLOCK pfn_KeAcquireInterruptSpinLock;
	PFNKERELEASEINTERRUPTSPINLOCK pfn_KeReleaseInterruptSpinLock;
	PFNIOCONNECTINTERRUPTEX pfn_IoConnectInterruptEx;
	PFNIODISCONNECTINTERRUPTEX pfn_IoDisconnectInterruptEx;
	PFNKFRAISEIRQL pfn_KfRaiseIrql;
	PFNKKFLOWERIRQL pfn_KfLowerIrql;
	PFNINTERLOCKEDPOPENTRYSLIST pfn_FxInterlockedPopEntrySList;
	PFNINTERLOCKEDPUSHENTRYSLIST pfn_FxInterlockedPushEntrySList;
	PFNPOGETSYSTEMWAKE pfn_PoGetSystemWake;
	PFNPOSETSYSTEMWAKE pfn_PoSetSystemWake;
	PFNKEQUERYACTIVEPROCESSORS pfn_KeQueryActiveProcessors;
	PFNKESETTARGETPROCESSORDPC pfn_KeSetTargetProcessorDpc;
	PFNKESETCOALESCABLETIMER pfn_KeSetCoalescableTimer;
	PFNKEAREAPCSDISABLED pfn_KeAreApcsDisabled;
	PFNIOUNREGISTERPLUGPLAYNOTIFICATIONEX pfn_IoUnregisterPlugPlayNotificationEx;
	PFNNTWWIQUERYTRACEINFORMATION pfn_NtWmiQueryTraceInformation;
	PFNWMITRACEMESSAGEVA pfn_WmiTraceMessageVa;

	OSVERSIONINFOEXW OsVersionInfo;
	MxLockNoDynam FxDriverGlobalsListLock;
	LIST_ENTRY FxDriverGlobalsList;
	
	//
    // Index to first free entry in BugCheckDriverInfo array.
    //	
	ULONG BugCheckDriverInfoIndex;
	
	//
    // # of entries in BugCheckDriverInfo array.
    //
	ULONG BugCheckDriverInfoCount;

	//
    // Array of info about loaded driver. The library bugcheck callback
    // writes this data into the minidump.
    //
	PFX_DUMP_DRIVER_INFO_ENTRY BugCheckDriverInfo;

	//
    // Library bug-check callback record for processing bugchecks.
    //
	KBUGCHECK_REASON_CALLBACK_RECORD BugCheckCallbackRecord;
	
	BOOLEAN ProcessorGroupSupport;

	//
    // The following field is used by the debug dump callback routine for
    // finding which driver's dump log file to write in the minidump if an
    // exact match is not found.
    //
	FX_DRIVER_TRACKER_CACHE_AWARE DriverTracker;

	//
    // Best driver match for the mini-dump log.
    //
	PFX_DRIVER_GLOBALS BestDriverForDumpLog;


	BOOLEAN StaticallyLinked;
	BOOLEAN AllowWmiUpdates;
	BOOLEAN UseTargetSystemPowerState;
	BOOLEAN MachineSleepStates[FxMachineSleepStatesMax];
	PVOID EnhancedVerifierSectionHandle;
};

extern FxLibraryGlobalsType FxLibraryGlobals;

typedef struct _FX_OBJECT_INFO {
    //
    // The name of the object, ie "FxObject"
    //
    const CHAR* Name;

    //
    // The name of the external WDF handle that represents the object, ie
    // WDFDEVICE.  If the object does not have an external handle, this field
    // may be NULL.
    //
    const CHAR* HandleName;

    //
    // The minimum size of the object, ie sizeof(FxObject).  There are objects
    // which allocate more than their sizeof() length.
    //
    USHORT Size;

    //
    // FX_OBJECT_TYPES value
    //
    USHORT ObjectType;

} FX_OBJECT_INFO, *PFX_OBJECT_INFO;



typedef
BOOLEAN
(*PFN_KE_REGISTER_BUGCHECK_REASON_CALLBACK) (
    __in PKBUGCHECK_REASON_CALLBACK_RECORD  CallbackRecord,
    __in PKBUGCHECK_REASON_CALLBACK_ROUTINE CallbackRoutine,
    __in KBUGCHECK_CALLBACK_REASON Reason,
    __in PUCHAR Component
    );

typedef
BOOLEAN
(*PFN_KE_DEREGISTER_BUGCHECK_REASON_CALLBACK) (
    __in PKBUGCHECK_REASON_CALLBACK_RECORD  CallbackRecords
    );


//
// Get FxDriverGlobals from api's DriverGlobals
//
__inline
PFX_DRIVER_GLOBALS
GetFxDriverGlobals(
    __in PWDF_DRIVER_GLOBALS DriverGlobals
    )
{
    return CONTAINING_RECORD( DriverGlobals, FX_DRIVER_GLOBALS, Public );
}

__bcount(Size)
PVOID
FORCEINLINE
FxPoolAllocate(
    __in PFX_DRIVER_GLOBALS Globals,
    __in POOL_TYPE Type,
    __in size_t Size
    )
{
    //
    // Always pass in the return address, regardless of the value of
    // Globals->WdfPoolTrackingOn.
    //
    return FxPoolAllocator(
        Globals,
        &Globals->FxPoolFrameworks,
        Type,
        Size,
        Globals->Tag,
        _ReturnAddress()
        );
}

//
// This inline function tracks driver usage; This info is used by the
// debug dump callback routine for selecting which driver's log to save
// in the minidump. At this time we track the following OS to framework calls:
//  (a) IRP dispatch (general, I/O, PnP, WMI).
//  (b) Request's completion callbacks.
//  (c) Work Item's (& System Work Item's) callback handlers.
//  (d) Timer's callback handlers.
//
__inline
VOID
FX_TRACK_DRIVER(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxTrackDriverForMiniDumpLog)
    {
        FxLibraryGlobals.DriverTracker.TrackDriver(FxDriverGlobals);
    }
}

#ifdef __cplusplus
}
#endif


#endif //_FXGLOBALS_H
