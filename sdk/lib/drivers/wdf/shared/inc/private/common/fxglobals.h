/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxGlobals.h

Abstract:

    This module contains globals definitions for the frameworks.

Author:




Environment:

    Both kernel and user mode

Revision History:

        Made it mode agnostic
        Moved km specific portions to FxGlobalsKm.h









        New failure paths:
            AllocatedTagTrackersLock initialization -
                If this fails we free debug extensions structure and not use it
            ThreadTableLock initialization -
                If this fails we turn off lock verification
            FxDriverGlobalsListLock initialization -
                If this fails we fail FxLibraryGlobalsCommission

--*/

#ifndef _FXGLOBALS_H
#define _FXGLOBALS_H

#include "wdfglobals.h"

#ifdef __cplusplus
extern "C" {
#endif

struct FxLibraryGlobalsType;

class CWudfDriverGlobals; //UMDF driver globals

//
// NOTE: any time you add a value to this enum, you must add a field to the
// union in FxObjectDebugInfo.
//
enum FxObjectDebugInfoFlags {
    FxObjectDebugTrackReferences = 0x0001,
};

typedef enum FxTrackPowerOption : UCHAR {
    FxTrackPowerNone = 0,
    FxTrackPowerRefs,
    FxTrackPowerRefsAndStack,
    FxTrackPowerMaxValue
};

typedef enum FxVerifierDownlevelOption {
    NotOkForDownLevel = 0,
    OkForDownLevel = 1,
} FxVerifierDownLevelOption;

typedef enum WaitSignalFlags {
    WaitSignalBreakUnderVerifier      = 0x01,
    WaitSignalBreakUnderDebugger      = 0x02,
    WaitSignalAlwaysBreak             = 0x04
} WaitSignalFlags;


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
        } Bits;
    } u;
};

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

    //
    // Whether we track power references for WDFDEVICE objects
    // and optionally capture stack frames.
    //
    FxTrackPowerOption TrackPower;
};

//
// A telemetry context that is allocated if the telemetry provider is enabled.
//
typedef struct _FX_TELEMETRY_CONTEXT{
    //
    // A GUID representing the driver session
    //
    GUID DriverSessionGUID;

    //
    // A general purpose bitmap that can be used
    // by various telemetry events that may want to
    // fire once per driver session.
    //
    volatile LONG DoOnceFlagsBitmap;
}  FX_TELEMETRY_CONTEXT, *PFX_TELEMETRY_CONTEXT;

typedef struct _FX_DRIVER_GLOBALS {
public:
    ULONG
    __inline
    AddRef(
        __in_opt   PVOID Tag = NULL,
        __in       LONG Line = 0,
        __in_opt   PSTR File = NULL
        )
    {
        ULONG c;

        UNREFERENCED_PARAMETER(Tag);
        UNREFERENCED_PARAMETER(Line);
        UNREFERENCED_PARAMETER(File);

        c = InterlockedIncrement(&Refcnt);

        //
        // Catch the transition from 0 to 1.  Since the RefCount starts off at 1,
        // we should never have to increment to get to this value.
        //
        ASSERT(c > 1);
        return c;
    }

    ULONG
    __inline
    Release(
        __in_opt    PVOID Tag = NULL,
        __in        LONG Line = 0,
        __in_opt    PSTR File = NULL
        )
    {
        ULONG c;

        UNREFERENCED_PARAMETER(Tag);
        UNREFERENCED_PARAMETER(Line);
        UNREFERENCED_PARAMETER(File);

        c = InterlockedDecrement(&Refcnt);
        ASSERT((LONG)c >= 0);
        if (c == 0) {
            DestroyEvent.Set();
        }

        return c;
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
        if (FxVerifierHandle) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

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
        // Following two can be overridden by the registry settings
        // WDFVERIFY matches the state of the verifier.
        //
        FxVerifyOn                  = State;
        FxVerifierDbgBreakOnError   = State;
        FxVerifierDbgBreakOnDeviceStateError = FALSE;

        //
        // Set the public flags for consumption by client drivers.
        //
        if (State) {
            Public.DriverFlags |= (WdfVerifyOn | WdfVerifierOn);
        }
    }

    _Must_inspect_result_
    BOOLEAN
    IsVersionGreaterThanOrEqualTo(
        __in ULONG  Major,
        __in ULONG  Minor
        );

    _Must_inspect_result_
    BOOLEAN
    IsCorrectVersionRegistered(
        _In_ PCUNICODE_STRING ServiceKeyName
        );

    VOID
    RegisterClientVersion(
        _In_ PCUNICODE_STRING ServiceKeyName
        );

    _Must_inspect_result_
    BOOLEAN
    IsVerificationEnabled(
        __in ULONG  Major,
        __in ULONG  Minor,
        __in FxVerifierDownlevelOption DownLevel
        )
    {
        //
        // those verifier checks that are restricted to specific version can be
        // applied to previous version drivers if driver opts-in by setting a
        // reg key (whose value is stored in FxVerifyDownlevel)
        //
        if (FxVerifierOn &&
            (IsVersionGreaterThanOrEqualTo(Major, Minor) ||
             (DownLevel ? FxVerifyDownlevel : FALSE))) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    //
    // To be used in code path where it is already determined that the driver
    // is down-level, otherwise use IsVerificationEnabled.
    //
    __inline
    _Must_inspect_result_
    BOOLEAN
    IsDownlevelVerificationEnabled(
        )
    {
        return FxVerifyDownlevel;
    }

    VOID
    WaitForSignal(
        __in MxEvent* Event,
        __in PCSTR ReasonForWaiting,
        __in PVOID Handle,
        __in ULONG WarningTimeoutInSec,
        __in ULONG WaitSignalFlags
        );

    _Must_inspect_result_
    BOOLEAN
    IsDebuggerAttached(
        VOID
        );

public:
    //
    // Link list of driver FxDriverGlobals on this WDF Version.
    //
    LIST_ENTRY Linkage;

    //
    // Reference count is operated on with interlocked operations
    //
    LONG Refcnt;

    //
    // This event is signaled when globals can be freed. Unload thread waits
    // on this event to make sure driver's threads are done and driver unload
    // can proceed.
    //
    MxEvent DestroyEvent;

    //
    // Mask to XOR all outgoing handles against
    //
    ULONG_PTR WdfHandleMask;

    //
    // If verifier is on, this is the count of allocations
    // to fail at
    //
    LONG    WdfVerifierAllocateFailCount;

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

    //
    // WDF internal In-Flight Recorder (IFR) log
    //
    PVOID  WdfLogHeader;

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
    // Parent queue presented requests (to device).
    //
    BOOLEAN FxRequestParentOptimizationOn;

    //
    // Enable/Disable support for device simulation framework (DSF).
    //
    BOOLEAN FxDsfOn;

    //
    // Force copy of IFR data to mini-dump when a bugcheck happens.
    //
    BOOLEAN FxForceLogsInMiniDump;

    //
    // TRUE to enable run-time driver tracking. The dump callback logic
    // uses this info for finding the right log to write in the minidump.
    //
    BOOLEAN FxTrackDriverForMiniDumpLog;

    //
    // TRUE if compiled for user-mode
    //
    BOOLEAN IsUserModeDriver;

    //
    // Remove lock options, these are also specified through
    // WdfDeviceInitSetRemoveLockOptions.
    //
    ULONG RemoveLockOptionFlags;

    //
    // Bug check callback data for kernel mode only



    //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // 0-based index into the BugCheckDriverInfo holding this driver info.
    //
    ULONG BugCheckDriverInfoIndex;

    //
    // Bug check callback record for processing bugchecks.
    //
    KBUGCHECK_REASON_CALLBACK_RECORD BugCheckCallbackRecord;

#endif

    //
    // Enhanced Verifier Options.
    //
    ULONG FxEnhancedVerifierOptions;

    //
    // If FxVerifierDbgBreakOnError is true, WaitForSignal interrupts the
    // execution of the system after waiting for the specified number
    // of seconds. Developer will have an opportunity to validate the state
    // of the driver when breakpoint is hit. Developer can continue to wait
    // by entering 'g' in the debugger.
    //
    ULONG FxVerifierDbgWaitForSignalTimeoutInSec;

    //
    // Timeout used by the wake interrupt ISR in WaitForSignal to catch
    // scenarios where the interrupt ISR is blocked because the device stack
    // is taking too long to power up
    //
    ULONG DbgWaitForWakeInterruptIsrTimeoutInSec;

#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    CWudfDriverGlobals * UfxDriverGlobals;
#endif

    PFX_TELEMETRY_CONTEXT TelemetryContext;

    //
    // The public version of WDF_DRIVER_GLOBALS
    //
    DECLSPEC_ALIGN(MEMORY_ALLOCATION_ALIGNMENT) WDF_DRIVER_GLOBALS  Public;

} FX_DRIVER_GLOBALS, *PFX_DRIVER_GLOBALS;

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

typedef struct _WDF_DRIVER_CONFIG *PWDF_DRIVER_CONFIG;

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
VOID
LockVerifierSection(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ PCUNICODE_STRING RegistryPath
    );

VOID
UnlockVerifierSection(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    );
#endif

BOOLEAN
IsWindowsVerifierOn(
    _In_ MdDriverObject DriverObject
    );

_Must_inspect_result_
NTSTATUS
FxInitialize(
    __inout     PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        MdDriverObject DriverObject,
    __in        PCUNICODE_STRING RegistryPath,
    __in_opt    PWDF_DRIVER_CONFIG DriverConfig //optional in user mode
    );

VOID
FxDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

_Must_inspect_result_
NTSTATUS
FxLibraryGlobalsCommission(
    VOID
    );

VOID
FxLibraryGlobalsDecommission(
    VOID
    );

VOID
FxCheckAssumptions(
    VOID
    );

void
FxVerifierLockInitialize(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

void
FxVerifierLockDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

_Must_inspect_result_
BOOLEAN
FxVerifierGetTrackReferences(
    __in FxObjectDebugInfo* DebugInfo,
    __in WDFTYPE ObjectType
    );

PCSTR
FxObjectTypeToHandleName(
    __in WDFTYPE ObjectType
    );

typedef
NTSTATUS
(*PFN_WMI_QUERY_TRACE_INFORMATION)(
    __in      TRACE_INFORMATION_CLASS TraceInformationClass,
    __out     PVOID TraceInformation,
    __in      ULONG TraceInformationLength,
    __out_opt PULONG RequiredLength,
    __in_opt  PVOID Buffer
    );

typedef
NTSTATUS
(*PFN_WMI_TRACE_MESSAGE_VA)(
    __in TRACEHANDLE  LoggerHandle,
    __in ULONG        MessageFlags,
    __in LPGUID       MessageGuid,
    __in USHORT       MessageNumber,
    __in va_list      MessageArgList
    );

enum FxMachineSleepStates {
    FxMachineS1Index = 0,
    FxMachineS2Index,
    FxMachineS3Index,
    FxMachineSleepStatesMax,
};

//
// Private Globals for the entire DLL

//
struct FxLibraryGlobalsType {

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)

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

    PFN_IO_CONNECT_INTERRUPT_EX IoConnectInterruptEx;

    PFN_IO_DISCONNECT_INTERRUPT_EX IoDisconnectInterruptEx;

    PFN_KE_QUERY_ACTIVE_PROCESSORS KeQueryActiveProcessors;

    PFN_KE_SET_TARGET_PROCESSOR_DPC KeSetTargetProcessorDpc;

    PFN_KE_SET_COALESCABLE_TIMER KeSetCoalescableTimer;

    PFN_IO_UNREGISTER_PLUGPLAY_NOTIFICATION_EX IoUnregisterPlugPlayNotificationEx;

    PFN_POX_REGISTER_DEVICE PoxRegisterDevice;

    PFN_POX_START_DEVICE_POWER_MANAGEMENT PoxStartDevicePowerManagement;

    PFN_POX_UNREGISTER_DEVICE PoxUnregisterDevice;

    PFN_POX_ACTIVATE_COMPONENT PoxActivateComponent;

    PFN_POX_IDLE_COMPONENT PoxIdleComponent;

    PFN_POX_REPORT_DEVICE_POWERED_ON PoxReportDevicePoweredOn;

    PFN_POX_COMPLETE_IDLE_STATE PoxCompleteIdleState;

    PFN_POX_COMPLETE_IDLE_CONDITION PoxCompleteIdleCondition;

    PFN_POX_COMPLETE_DEVICE_POWER_NOT_REQUIRED PoxCompleteDevicePowerNotRequired;

    PFN_POX_SET_DEVICE_IDLE_TIMEOUT PoxSetDeviceIdleTimeout;

    PFN_IO_REPORT_INTERRUPT_ACTIVE IoReportInterruptActive;

    PFN_IO_REPORT_INTERRUPT_INACTIVE IoReportInterruptInactive;

    PFN_VF_CHECK_NX_POOL_TYPE VfCheckNxPoolType;

#endif

    RTL_OSVERSIONINFOEXW OsVersionInfo;

    MxLockNoDynam FxDriverGlobalsListLock;

    LIST_ENTRY   FxDriverGlobalsList;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
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
    PFX_DUMP_DRIVER_INFO_ENTRY   BugCheckDriverInfo;

    //
    // Library bug-check callback record for processing bugchecks.
    //
    KBUGCHECK_REASON_CALLBACK_RECORD  BugCheckCallbackRecord;

    BOOLEAN ProcessorGroupSupport;

#endif
    //
    // WPP tracing.
    //
    BOOLEAN InternalTracingInitialized;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
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
#endif

    BOOLEAN PassiveLevelInterruptSupport;

    //
    // TRUE if compiled for user-mode
    //
    BOOLEAN IsUserModeFramework;

    //






    //

    BOOLEAN MachineSleepStates[FxMachineSleepStatesMax];

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // used for locking/unlocking Enhanced-verifier image section
    //
    PVOID VerifierSectionHandle;

    //
    // This keeps track of the # of times we pinned the paged memory down.
    // This is only used to aid debugging.
    //
    volatile LONG VerifierSectionHandleRefCount;

    //
    // Routines provided by the kernel SystemTraceProvider for perf
    // tracing of WDF operations. The size member of this structure
    // allows versioning across multiple OS versions.
    //
    PWMI_WDF_NOTIFY_ROUTINES PerfTraceRoutines;

    //
    //  PerfTraceRoutines points here if the SystemTraceProvider failed
    //  to provide trace routines.
    //
    WMI_WDF_NOTIFY_ROUTINES DummyPerfTraceRoutines;

#endif

    //
    // Registry setting to disable IFR on low-memory systems.
    //
    BOOLEAN IfrDisabled;
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

//
// Define to declare an internal entry.  An internal entry has no external WDF
// handle.
//
#define FX_INTERNAL_OBJECT_INFO_ENTRY(_obj, _type)  \
    { #_obj, NULL, sizeof(_obj), _type, }

//
// Define to declare an external entry.  An external entry has an external WDF
// handle.
//
// By casting #_handletype to a (_handletype), we make sure that _handletype is
// actually a valid WDF handle name.  The cast forces the parameter to be
// evaluated as true handle type vs a string (ie the # preprocesor operator).
// For instance, this would catch the following error:
//
//    FX_EXTERNAL_OBJECT_INFO_ENTRY(FxDevice, FX_TYPE_DEVICE, WDFDEVICES),
//
// because the statement would evaluate to (const CHAR*) (WDFDEVICES) "WDFDEVICES"
// and WDFDEVICES is not a valid type (WDFDEVICE is though).
//
#define FX_EXTERNAL_OBJECT_INFO_ENTRY(_obj, _type, _handletype) \
    { #_obj,                                                    \
      (const CHAR*) (_handletype) #_handletype,                 \
      sizeof(_obj),                                             \
      _type,                                                    \
    }

_Must_inspect_result_
BOOLEAN
FxVerifyObjectTypeInTable(
    __in USHORT ObjectType
    );

VOID
FxFlushQueuedDpcs(
    VOID
    );

VOID
FxFreeAllocatedMdlsDebugInfo(
    __in FxDriverGlobalsDebugExtension* DebugExtension
    );

//













//

_Must_inspect_result_
__inline
BOOLEAN
FxIsClassExtension(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PFX_DRIVER_GLOBALS ExtDriverGlobals
    )
{
    return (FxDriverGlobals == ExtDriverGlobals) ? FALSE : TRUE;
}


_Must_inspect_result_
BOOLEAN
__inline
FxIsEqualGuid(
    __in CONST GUID* Lhs,
    __in CONST GUID* Rhs
    )
{
    return RtlCompareMemory(Lhs, Rhs, sizeof(GUID)) == sizeof(GUID);
}

__inline
size_t
FxSizeTMax(
    __in size_t A,
    __in size_t B
    )
{
    return A > B ? A : B;
}

__inline
size_t
FxSizeTMin(
    __in size_t A,
    __in size_t B
    )
{
    return A < B ? A : B;
}

__inline
LONG
FxInterlockedIncrementFloor(
    __inout LONG  volatile *Target,
    __in LONG Floor
    )
{
    LONG startVal;
    LONG currentVal;

    currentVal = *Target;

    do {
        if (currentVal <= Floor) {
            return currentVal;
        }

        startVal = currentVal;

        //
        // currentVal will be the value that used to be Target if the exchange was made
        // or its current value if the exchange was not made.
        //
        currentVal = InterlockedCompareExchange(Target, startVal+1, startVal);

        //
        // If startVal == currentVal, then no one updated Target in between the deref at the top
        // and the InterlockedCompareExchange afterward.
        //
    } while (startVal != currentVal);

    //
    // startVal is the old value of Target. Since InterlockedIncrement returns the new
    // incremented value of Target, we should do the same here.
    //
    return startVal+1;
}

__inline
LONG
FxInterlockedIncrementGTZero(
    __inout LONG  volatile *Target
    )
{
    return FxInterlockedIncrementFloor(Target, 0);
}

__inline
ULONG
FxRandom(
    __inout PULONG RandomSeed
    )
/*++

Routine Description:

    Simple threadsafe random number generator to use at DISPATCH_LEVEL
    (in kernel mode) because the system provided function RtlRandomEx
    can be called at only passive-level.

    This function requires the user to provide a variable used to seed
    the generator, and it must be valid and initialized to some number.

Return Value:

   ULONG

--*/
{
    *RandomSeed = *RandomSeed * 1103515245 + 12345;
    return (ULONG)(*RandomSeed / 65536) % 32768;
}

_Must_inspect_result_
__inline
BOOLEAN
FxIsPassiveLevelInterruptSupported(
    VOID
    )
{
    //
    // Passive-level interrupt handling is supported in Win 8 and forward.
    //
    return FxLibraryGlobals.PassiveLevelInterruptSupport;
}

__inline
_Must_inspect_result_
BOOLEAN
IsOsVersionGreaterThanOrEqualTo(
    __in ULONG Major,
    __in ULONG Minor
    )
{
    return ((FxLibraryGlobals.OsVersionInfo.dwMajorVersion > Major) ||
            ((FxLibraryGlobals.OsVersionInfo.dwMajorVersion == Major) &&
             (FxLibraryGlobals.OsVersionInfo.dwMinorVersion >= Minor)));
}

#ifdef __cplusplus
}
#endif
#endif // _FXGLOBALS_H

