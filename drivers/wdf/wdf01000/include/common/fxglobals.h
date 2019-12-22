

#ifndef _FXGLOBALS_H
#define _FXGLOBALS_H

#include <ntddk.h>
#include "wdf.h"
#include "common/fxldr.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FxLibraryGlobalsType;
extern RTL_OSVERSIONINFOW  gOsVersion;

NTSTATUS
FxLibraryGlobalsCommission(VOID);

VOID
FxLibraryGlobalsDecommission(VOID);

typedef struct _FX_DRIVER_GLOBALS
{
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
	//FxDriver* Driver;
	//FxDriverGlobalsDebugExtension* DebugExtension;
	FxLibraryGlobalsType* LibraryGlobals;
	ULONG WdfTraceDelayTime;

	//
    // WDF internal In-Flight Recorder (IFR) log
    //
	PVOID WdfLogHeader;

	//
    // The driver's memory pool header
    //
	//FX_POOL FxPoolFrameworks;

	//
    // Framworks Pool Tracking
    //
	BOOLEAN FxPoolTrackingOn;

	//
    // FxVerifierLock per driver state
    //
	//MxLock ThreadTableLock;
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


enum FxMachineSleepStates
{
    FxMachineS1Index = 0,
    FxMachineS2Index,
    FxMachineS3Index,
    FxMachineSleepStatesMax,
};

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

	BOOLEAN(* pfn_KdRefresh)(VOID);
	VOID(* pfn_KeFlushQueuedDpcs)(VOID);
	NTSTATUS(* pfn_IoSetCompletionRoutineEx)(PDEVICE_OBJECT, PIRP, PIO_COMPLETION_ROUTINE, PVOID, BOOLEAN, BOOLEAN, BOOLEAN);
	KIRQL(* pfn_KeAcquireInterruptSpinLock)(PKINTERRUPT);
	VOID(* pfn_KeReleaseInterruptSpinLock)(PKINTERRUPT, KIRQL);
	NTSTATUS(* pfn_IoConnectInterruptEx)(PIO_CONNECT_INTERRUPT_PARAMETERS);
	VOID(* pfn_IoDisconnectInterruptEx)(PIO_DISCONNECT_INTERRUPT_PARAMETERS);
	KIRQL(FASTCALL* pfn_KfRaiseIrql)(KIRQL);
	VOID(FASTCALL* pfn_KfLowerIrql)(KIRQL);
	PSINGLE_LIST_ENTRY(FASTCALL* pfn_FxInterlockedPopEntrySList)(PSLIST_HEADER);
	PSINGLE_LIST_ENTRY(FASTCALL* pfn_FxInterlockedPushEntrySList)(PSLIST_HEADER, PSINGLE_LIST_ENTRY);
	BOOLEAN(* pfn_PoGetSystemWake)(PIRP);
	VOID(* pfn_PoSetSystemWake)(PIRP);
	KAFFINITY(* pfn_KeQueryActiveProcessors)(VOID);
	VOID(* pfn_KeSetTargetProcessorDpc)(PKDPC, CCHAR);
	BOOLEAN(* pfn_KeSetCoalescableTimer)(PKTIMER, LARGE_INTEGER, ULONG, ULONG, PKDPC);
	BOOLEAN(* pfn_KeAreApcsDisabled)(VOID);
	NTSTATUS(* pfn_IoUnregisterPlugPlayNotificationEx)(PVOID);
	NTSTATUS(* pfn_NtWmiQueryTraceInformation)(TRACE_INFORMATION_CLASS, PVOID, ULONG, PULONG, PVOID);
	NTSTATUS(* pfn_WmiTraceMessageVa)(TRACEHANDLE, ULONG, LPCGUID, USHORT, va_list);

	OSVERSIONINFOEXW OsVersionInfo;
	//MxLockNoDynam FxDriverGlobalsListLock;
	KSPIN_LOCK FxDriverGlobalsListLock;
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
	//_FX_DUMP_DRIVER_INFO_ENTRY* BugCheckDriverInfo;

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
	//_FX_DRIVER_TRACKER_CACHE_AWARE DriverTracker;

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

#ifdef __cplusplus
}
#endif


#endif //_FXGLOBALS_H
