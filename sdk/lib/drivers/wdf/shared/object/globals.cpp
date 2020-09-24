
/*++

Copyright (c) Microsoft Corporation

Module Name:

    globals.cpp

Abstract:

    This contains all Driver Frameworks configuration globals.

Author:




Environment:

    Both kernel and user mode

Revision History:













--*/


#include "fxobjectpch.hpp"

// Tracing support
extern "C" {
#if defined(EVENT_TRACING)
#include "globals.tmh"
#endif
}

extern "C" {

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
VOID
VerifierPageLockHandle (
    VOID
    );
#ifdef ALLOC_PRAGMA
#pragma alloc_text(WDF_FX_VF_SECTION_NAME, VerifierPageLockHandle)
#endif
#endif

//
// Private methods.
//

VOID
FxLibraryGlobalsQueryRegistrySettings(
    VOID
    );

VOID
FxRegistrySettingsInitialize(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegistryPath,
    __in BOOLEAN WindowsVerifierOn
    );

_Must_inspect_result_
FxObjectDebugInfo*
FxVerifierGetObjectDebugInfo(
    __in HANDLE Key,
    __in PFX_DRIVER_GLOBALS  FxDriverGlobals
    );

VOID
FxVerifierQueryTrackPower(
    __in HANDLE Key,
    __out FxTrackPowerOption* TrackPower
    );

//
// Global allocation tracker
//
FX_POOL FxPoolFrameworks;

FxLibraryGlobalsType FxLibraryGlobals = { 0 };

//
// These are defined in FxObjectInfo.cpp to account for the facts that
//     1. FxObjectInfo array is different for UMDF and KMDF,
//     2. and not all the types are available in the common code
//
extern const FX_OBJECT_INFO FxObjectsInfo[];
extern ULONG FxObjectsInfoCount;

//
// Prevent compiler/linker/BBT from optimizing the global variable away
//
#if defined(_M_IX86)
#pragma comment(linker, "/include:_FxObjectsInfoCount")
#else
#pragma comment(linker, "/include:FxObjectsInfoCount")
#endif


_Must_inspect_result_
BOOLEAN
FxVerifyObjectTypeInTable(
    __in USHORT ObjectType
    )
{
    ULONG i;

    for (i = 0; i < FxObjectsInfoCount; i++) {
        if (ObjectType == FxObjectsInfo[i].ObjectType) {
            return TRUE;
        }
        else if (ObjectType > FxObjectsInfo[i].ObjectType) {
            continue;
        }

        return FALSE;
    }

    return FALSE;
}

_Must_inspect_result_
FxObjectDebugInfo*
FxVerifyAllocateDebugInfo(
    __in LPWSTR HandleNameList,
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )

/*++

Routine Description:
    Allocates an array of FxObjectDebugInfo's.  The length of this array is the
    same length as FxObjectsInfo.  The array is sorted the same as
    FxObjectDebugInfo, ObjectInfo is ascending in the list.

    If HandleNameList's first string is "*", we treat this as a wildcard and
    track all external handles.

Arguments:
    HandleNameList - a multi-sz of handle names.  It is assumed the multi sz is
        well formed.

Return Value:
    a pointer allocated by ExAllocatePoolWithTag.  The caller is responsible for
    eventually freeing the pointer by calling ExFreePool.

--*/

{
    FxObjectDebugInfo* pInfo;
    PWCHAR pCur;
    ULONG i, length;
    BOOLEAN all;

    //
    // check to see if the multi sz is empty
    //
    if (*HandleNameList == NULL) {
        return NULL;
    }

    length = sizeof(FxObjectDebugInfo) * FxObjectsInfoCount;

    //
    // Freed with ExFreePool in FxFreeDriverGlobals.  Must be non paged because
    // objects can be allocated at IRQL > PASSIVE_LEVEL.
    //
    pInfo = (FxObjectDebugInfo*) MxMemory::MxAllocatePoolWithTag(NonPagedPool,
                                                       length,
                                                       FxDriverGlobals->Tag);

    if (pInfo == NULL) {
        return NULL;
    }

    all = *HandleNameList == L'*' ? TRUE : FALSE;

    RtlZeroMemory(pInfo, length);

    //
    // Iterate over all of the objects in our internal array.  We iterate over
    // this array instead of the multi sz list b/c this way we only convert
    // each ANSI string to UNICODE once.
    //
    for (i = 0; i < FxObjectsInfoCount; i++) {
        UNICODE_STRING objectName;
        WCHAR ubuffer[40];
        STRING string;

        pInfo[i].ObjectType = FxObjectsInfo[i].ObjectType;

        //
        // If this is an internal object, just continue past it
        //
        if (FxObjectsInfo[i].HandleName == NULL) {
            continue;
        }

        //
        // Short circuit if we are wildcarding
        //
        if (all) {
            pInfo[i].u.DebugFlags |= FxObjectDebugTrackReferences;
            continue;
        }

        RtlInitAnsiString(&string, FxObjectsInfo[i].HandleName);

        RtlZeroMemory(ubuffer, sizeof(ubuffer));
        objectName.Buffer = ubuffer;
        objectName.Length = 0;
        objectName.MaximumLength = sizeof(ubuffer);

        //
        // Conversion failed, just continue.  Failure is not critical to
        // returning the list.
        //
        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&objectName,
                                                     &string,
                                                     FALSE))) {
            continue;
        }

        //
        // Now iterate over the multi sz list, comparing handle strings in the
        // list against the current object name.
        //
        pCur = HandleNameList;

        while (*pCur != UNICODE_NULL) {
            UNICODE_STRING handleName;

            RtlInitUnicodeString(&handleName, pCur);

            //
            // Increment to the next string now.  Add one so that we skip past
            // terminating null for this sz as well.
            // Length is the number of bytes, not the number of characters.
            //
            pCur += handleName.Length / sizeof(WCHAR) + 1;

            //
            // Case insensitive compare
            //
            if (RtlCompareUnicodeString(&handleName, &objectName, TRUE) == 0) {
                pInfo[i].u.DebugFlags |= FxObjectDebugTrackReferences;
                break;
            }
        }
    }

    return pInfo;
}

VOID
FxDriverGlobalsInitializeDebugExtension(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in_opt    HANDLE Key
    )
{
    FxDriverGlobalsDebugExtension* pExtension;

    //
    // The wdf subkey may not be present for inbox drivers that do not use inf.
    // Since Mdl tracking doen't need regsitry info we go ahead and allocate
    // debug extension for use in Mdl tracking. Tag tracker depends on registry
    // info and it won't be available if registry info is not present.
    //

    pExtension = (FxDriverGlobalsDebugExtension*) MxMemory::MxAllocatePoolWithTag(
        NonPagedPool, sizeof(FxDriverGlobalsDebugExtension), FxDriverGlobals->Tag);

    if (pExtension == NULL) {
        return;
    }

    RtlZeroMemory(pExtension, sizeof(*pExtension));

    pExtension->AllocatedTagTrackersLock.Initialize();

    InitializeListHead(&pExtension->AllocatedTagTrackersListHead);

    pExtension->TrackPower = FxTrackPowerNone;

    FxDriverGlobals->DebugExtension = pExtension;

    if (Key != NULL) {
        pExtension->ObjectDebugInfo = FxVerifierGetObjectDebugInfo(
                                                        Key,
                                                        FxDriverGlobals
                                                        );
        FxVerifierQueryTrackPower(Key, &pExtension->TrackPower);
    }

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    KeInitializeSpinLock(&pExtension->AllocatedMdlsLock);
#endif
}

PCSTR
FxObjectTypeToHandleName(
    __in WDFTYPE ObjectType
    )
{
    ULONG i;

    for (i = 0; i < FxObjectsInfoCount; i++) {
        if (ObjectType == FxObjectsInfo[i].ObjectType) {
            return FxObjectsInfo[i].HandleName;
        }
        else if (ObjectType > FxObjectsInfo[i].ObjectType) {
            continue;
        }

        return NULL;
    }

    return NULL;
}

_Must_inspect_result_
BOOLEAN
FxVerifierGetTrackReferences(
    __in FxObjectDebugInfo* DebugInfo,
    __in WDFTYPE ObjectType
    )

/*++

Routine Description:
    For a given object type, returns to the caller if it should track references
    to the object.

Arguments:
    DebugInfo - array of object debug info to search through
    ObjectType - the type of the object to check

Return Value:
    TRUE if references should be tracked, FALSE otherwise

--*/

{
    ULONG i;

    //
    // Array size of DebugInfo is the same size as FxObjectsInfo
    //
    for (i = 0; i < FxObjectsInfoCount; i++) {
        if (ObjectType == DebugInfo[i].ObjectType) {
            return FLAG_TO_BOOL(DebugInfo[i].u.DebugFlags,
                                FxObjectDebugTrackReferences);
        }
        else if (ObjectType > FxObjectsInfo[i].ObjectType) {
            continue;
        }

        return FALSE;
    }

    return FALSE;
}


VOID
FxVerifyObjectTableIsSorted(
    VOID
    )
{
    ULONG i;
    USHORT prevType;

    prevType = FxObjectsInfo[0].ObjectType;

    for (i = 1; i < FxObjectsInfoCount; i++) {
        if (prevType >= FxObjectsInfo[i].ObjectType) {
            ASSERTMSG("FxObjectsInfo table is not in sorted order\n",
                         prevType < FxObjectsInfo[i].ObjectType);
        }

        prevType = FxObjectsInfo[i].ObjectType;
    }
}

typedef
NTSTATUS
(*PFN_RTL_GET_VERSION)(
    __out PRTL_OSVERSIONINFOW VersionInformation
    );

typedef
NTSTATUS
(*PFN_RTL_VERIFY_VERSION_INFO)(
    __in PRTL_OSVERSIONINFOEXW VersionInfo,
    __in ULONG TypeMask,
    __in ULONGLONG  ConditionMask
    );

typedef
ULONGLONG
(*PFN_VER_SET_CONDITION_MASK)(
    __in  ULONGLONG   ConditionMask,
    __in  ULONG   TypeMask,
    __in  UCHAR   Condition
    );

VOID
FxLibraryGlobalsVerifyVersion(
    VOID
    )
{
    RTL_OSVERSIONINFOEXW info;
    PFN_RTL_VERIFY_VERSION_INFO pRtlVerifyVersionInfo;
    PFN_VER_SET_CONDITION_MASK pVerSetConditionMask;
    ULONGLONG condition;
    NTSTATUS status;

    pRtlVerifyVersionInfo = (PFN_RTL_VERIFY_VERSION_INFO)
        Mx::MxGetSystemRoutineAddress(MAKE_MX_FUNC_NAME("RtlVerifyVersionInfo"));

    if (pRtlVerifyVersionInfo == NULL) {
        return;
    }

    pVerSetConditionMask = (PFN_VER_SET_CONDITION_MASK)
        Mx::MxGetSystemRoutineAddress(MAKE_MX_FUNC_NAME("VerSetConditionMask"));

    //
    // Check for Win8 (6.2) and later for passive-level interrupt support.
    //
    RtlZeroMemory(&info, sizeof(info));
    info.dwOSVersionInfoSize = sizeof(info);
    info.dwMajorVersion = 6;
    info.dwMinorVersion = 2;

    condition = 0;
    condition = pVerSetConditionMask(condition, VER_MAJORVERSION, VER_GREATER_EQUAL);
    condition = pVerSetConditionMask(condition, VER_MINORVERSION, VER_GREATER_EQUAL);

    status = pRtlVerifyVersionInfo(&info,
                                   VER_MAJORVERSION | VER_MINORVERSION,
                                   condition);
    if (NT_SUCCESS(status)) {
        FxLibraryGlobals.PassiveLevelInterruptSupport = TRUE;
    }
}

VOID
FxLibraryGlobalsQueryRegistrySettings(
    VOID
    )
{
    FxAutoRegKey hWdf;
    NTSTATUS status = STATUS_SUCCESS;
    DECLARE_CONST_UNICODE_STRING(path, WDF_REGISTRY_BASE_PATH);
    DECLARE_CONST_UNICODE_STRING(ifrDisabledName, WDF_GLOBAL_VALUE_IFRDISABLED);
    ULONG ifrDisabled = 0;

    status = FxRegKey::_OpenKey(NULL, &path, &hWdf.m_Key, KEY_READ);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    status = FxRegKey::_QueryULong(hWdf.m_Key, &ifrDisabledName, &ifrDisabled);
    if (!NT_SUCCESS(status)) {
        goto exit;
    }

    if (ifrDisabled == 1) {
        FxLibraryGlobals.IfrDisabled = TRUE;
    }

exit:
    return;
}

_Must_inspect_result_
NTSTATUS
FxLibraryGlobalsCommission(
    VOID
    )
{
    PFN_RTL_GET_VERSION pRtlGetVersion;
    NTSTATUS status;

    //
    // Global initialization for mode-agnostic primitives library
    //
    Mx::MxGlobalInit();















#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    FxLibraryGlobals.IsUserModeFramework = FALSE;
#else
    FxLibraryGlobals.IsUserModeFramework = TRUE;
#endif

    //
    // IFR is enabled by default
    //
    FxLibraryGlobals.IfrDisabled = FALSE;

    //
    // Query global WDF settings (both KMDF and UMDF).
    //
    FxLibraryGlobalsQueryRegistrySettings();

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    UNICODE_STRING funcName;

    // For DSF support.
    RtlInitUnicodeString(&funcName, L"IoConnectInterruptEx");
    FxLibraryGlobals.IoConnectInterruptEx = (PFN_IO_CONNECT_INTERRUPT_EX)
        MmGetSystemRoutineAddress(&funcName);

    RtlInitUnicodeString(&funcName, L"IoDisconnectInterruptEx");
    FxLibraryGlobals.IoDisconnectInterruptEx = (PFN_IO_DISCONNECT_INTERRUPT_EX)
        MmGetSystemRoutineAddress(&funcName);

    // 32 bit: W2k and forward.
    // 64 bit: W2k -> Windows Server 2008 (obsolete otherwise).
    RtlInitUnicodeString(&funcName, L"KeQueryActiveProcessors");
    FxLibraryGlobals.KeQueryActiveProcessors = (PFN_KE_QUERY_ACTIVE_PROCESSORS)
        MmGetSystemRoutineAddress(&funcName);

    RtlInitUnicodeString(&funcName, L"KeSetTargetProcessorDpc");
    FxLibraryGlobals.KeSetTargetProcessorDpc = (PFN_KE_SET_TARGET_PROCESSOR_DPC)
        MmGetSystemRoutineAddress(&funcName);

    // These should always be there (obsolete in 64 bit Win 7 and forward).
    ASSERT(FxLibraryGlobals.KeQueryActiveProcessors != NULL &&
           FxLibraryGlobals.KeSetTargetProcessorDpc != NULL);

    // Win 7 and forward.
    RtlInitUnicodeString(&funcName, L"KeQueryActiveGroupCount");
    if (MmGetSystemRoutineAddress(&funcName) != NULL) {
        FxLibraryGlobals.ProcessorGroupSupport = TRUE;
    }

    // Win 7 and forward.
    RtlInitUnicodeString(&funcName, L"KeSetCoalescableTimer");
    FxLibraryGlobals.KeSetCoalescableTimer = (PFN_KE_SET_COALESCABLE_TIMER)
        MmGetSystemRoutineAddress(&funcName);

    // Win 7 and forward.
    RtlInitUnicodeString(&funcName, L"IoUnregisterPlugPlayNotificationEx");
    FxLibraryGlobals.IoUnregisterPlugPlayNotificationEx = (PFN_IO_UNREGISTER_PLUGPLAY_NOTIFICATION_EX)
        MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxRegisterDevice");
    FxLibraryGlobals.PoxRegisterDevice =
      (PFN_POX_REGISTER_DEVICE) MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxStartDevicePowerManagement");
    FxLibraryGlobals.PoxStartDevicePowerManagement =
                                    (PFN_POX_START_DEVICE_POWER_MANAGEMENT)
                                        MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxUnregisterDevice");
    FxLibraryGlobals.PoxUnregisterDevice =
                                (PFN_POX_UNREGISTER_DEVICE)
                                    MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxActivateComponent");
    FxLibraryGlobals.PoxActivateComponent = (PFN_POX_ACTIVATE_COMPONENT)
                                          MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxIdleComponent");
    FxLibraryGlobals.PoxIdleComponent = (PFN_POX_IDLE_COMPONENT)
                                          MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxReportDevicePoweredOn");
    FxLibraryGlobals.PoxReportDevicePoweredOn =
      (PFN_POX_REPORT_DEVICE_POWERED_ON) MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxCompleteIdleState");
    FxLibraryGlobals.PoxCompleteIdleState =
      (PFN_POX_COMPLETE_IDLE_STATE) MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxCompleteIdleCondition");
    FxLibraryGlobals.PoxCompleteIdleCondition =
      (PFN_POX_COMPLETE_IDLE_CONDITION) MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxCompleteDevicePowerNotRequired");
    FxLibraryGlobals.PoxCompleteDevicePowerNotRequired =
      (PFN_POX_COMPLETE_DEVICE_POWER_NOT_REQUIRED) MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"PoFxSetDeviceIdleTimeout");
    FxLibraryGlobals.PoxSetDeviceIdleTimeout =
      (PFN_POX_SET_DEVICE_IDLE_TIMEOUT) MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"IoReportInterruptActive");
    FxLibraryGlobals.IoReportInterruptActive =
      (PFN_IO_REPORT_INTERRUPT_ACTIVE) MmGetSystemRoutineAddress(&funcName);

    // Win 8 and forward
    RtlInitUnicodeString(&funcName, L"IoReportInterruptInactive");
    FxLibraryGlobals.IoReportInterruptInactive =
      (PFN_IO_REPORT_INTERRUPT_INACTIVE) MmGetSystemRoutineAddress(&funcName);

    // Win 8.2 and forward
    RtlInitUnicodeString(&funcName, L"VfCheckNxPoolType");
    FxLibraryGlobals.VfCheckNxPoolType =
      (PFN_VF_CHECK_NX_POOL_TYPE) MmGetSystemRoutineAddress(&funcName);

#endif //((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))

    FxLibraryGlobals.OsVersionInfo.dwOSVersionInfoSize = sizeof(FxLibraryGlobals.OsVersionInfo);

    // User/Kernel agnostic.

    pRtlGetVersion = (PFN_RTL_GET_VERSION)
                Mx::MxGetSystemRoutineAddress(MAKE_MX_FUNC_NAME("RtlGetVersion"));

    ASSERT(pRtlGetVersion != NULL);
    pRtlGetVersion((PRTL_OSVERSIONINFOW) &FxLibraryGlobals.OsVersionInfo);
    FxLibraryGlobalsVerifyVersion();

    //
    // Initialize power management-related stuff.
    //
    RtlZeroMemory(&FxLibraryGlobals.MachineSleepStates[0],
                  sizeof(FxLibraryGlobals.MachineSleepStates));

    //
    // Insure that the FxObject is layed-up correctly.
    //
    FxVerifyObjectTableIsSorted();

    //
    // Initialize the list of FxDriverGlobals.
    // This is essentially the list of drivers on this WDF version.
    //
    InitializeListHead(&FxLibraryGlobals.FxDriverGlobalsList);
    FxLibraryGlobals.FxDriverGlobalsListLock.Initialize();

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // Register for the global (library) bugcheck callbacks.
    //
    FxInitializeBugCheckDriverInfo();

    //
    // Init driver usage tracker. This tracker is used by the debug dump
    // callback routines for finding the driver's dump log file to write
    // in the minidump. Ignore any tracker's errors.
    //
    (VOID)FxLibraryGlobals.DriverTracker.Initialize();

    //
    // Initialize enhanced-verifier section handle
    //
    FxLibraryGlobals.VerifierSectionHandle = NULL;
    FxLibraryGlobals.VerifierSectionHandleRefCount = 0;

    //
    // Retrieve a pointer to the data structure that cotains trace routines
    // corresponding to WdfNotifyRoutinesClass from the SystemTraceProvider
    // that we'll use for perf tracing of WDF operations. The trace
    // routines inside the structuyre are present only when tracing is enabled
    // by some trace client (e.g. tracelog or xperf)  for WDF specific perf
    // groups. Note that no unregistration is necessary.
    //
    status = WmiQueryTraceInformation(WdfNotifyRoutinesClass,
                                      &FxLibraryGlobals.PerfTraceRoutines,
                                      sizeof(PWMI_WDF_NOTIFY_ROUTINES),
                                      NULL,
                                      NULL);

    if (!NT_SUCCESS(status)) {
        //
        // WDF trace routines are available only on win8+, so failure is
        // expected on pre-Win8 OS. Use the dummy routines on failure.
        //
        RtlZeroMemory(&FxLibraryGlobals.DummyPerfTraceRoutines,
                      sizeof(WMI_WDF_NOTIFY_ROUTINES));
        FxLibraryGlobals.DummyPerfTraceRoutines.Size =
            sizeof(WMI_WDF_NOTIFY_ROUTINES);
        FxLibraryGlobals.PerfTraceRoutines =
            &FxLibraryGlobals.DummyPerfTraceRoutines;
        status = STATUS_SUCCESS;
    }

    //
    // The Size member of WMI_WDF_NOTIFY_ROUTINES allows versioning. When
    // the WMI_WDF_NOTIFY_ROUTINES structure is revised with additional
    // members in future OS versions, the Size member will allow validating
    // the various versions, and initializeing the structure correctly.
    //
    ASSERT(FxLibraryGlobals.PerfTraceRoutines->Size >=
        sizeof(WMI_WDF_NOTIFY_ROUTINES));

#else
    status = STATUS_SUCCESS;
#endif

    return status;
}

VOID
FxLibraryGlobalsDecommission(
    VOID
    )
{
    //
    // Assure the all driver's FxDriverGlobals have been freed.
    //
    ASSERT(IsListEmpty(&FxLibraryGlobals.FxDriverGlobalsList));

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // Cleanup for the driver usage tracker.
    //
    FxLibraryGlobals.DriverTracker.Uninitialize();

    //
    // Deregister from the global (library) bugcheck callbacks.
    //
    FxUninitializeBugCheckDriverInfo();
#endif

    FxLibraryGlobals.FxDriverGlobalsListLock.Uninitialize();

    return;
}

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
//
// This function is only used to lock down verifier section code in memory.
// It uses the #pragma alloc_text(...) style for paging.
//
VOID
VerifierPageLockHandle (
    VOID
    )
{
    PAGED_CODE_LOCKED();
    DO_NOTHING();
}

VOID
LockVerifierSection(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ PCUNICODE_STRING RegistryPath
    )
{
    LONG count;

    //
    // This asserts  makes sure the struct is not pack(1) and the counter
    // is correctly aligned on a 32 bit boundary.
    //
    C_ASSERT((FIELD_OFFSET(FxLibraryGlobalsType, VerifierSectionHandleRefCount)
              % __alignof(LONG)) == 0);

    count = InterlockedIncrement(&FxLibraryGlobals.VerifierSectionHandleRefCount);
    ASSERT(count > 0);

    //
    // If verifier section is unlocked, lock it in.
    //
    if(FxLibraryGlobals.VerifierSectionHandle == NULL) {
        //
        //First time verifier section is being locked.
        //
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
                            "First time Locking (%d) in Verifier Paged Memory "
                            "from  %!wZ! from driver globals %p",
                            count, RegistryPath, FxDriverGlobals);
        //
        // VerifierLockHandle is a function that we use to lock in all the code from it's section
        // since all the verifier code is in the same section as VerifierLockHandle.
        //
        FxLibraryGlobals.VerifierSectionHandle = MmLockPagableCodeSection(VerifierPageLockHandle);
    }
    else {
        MmLockPagableSectionByHandle(FxLibraryGlobals.VerifierSectionHandle);
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
                            "Increment Lock counter (%d) for Verifier Paged Memory "
                            "from  %!wZ! from driver globals %p",
                            count, RegistryPath, FxDriverGlobals);
    }
}

VOID
UnlockVerifierSection(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if( FxLibraryGlobals.VerifierSectionHandle != NULL) {
        LONG count;

        count = InterlockedDecrement(&FxLibraryGlobals.VerifierSectionHandleRefCount);
        ASSERT(count >= 0);

        MmUnlockPagableImageSection(FxLibraryGlobals.VerifierSectionHandle);
        DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
                            "Decrement UnLock counter (%d) for Verifier Paged Memory "
                            "with driver globals %p",
                            count, FxDriverGlobals);
    }
}
#endif

_Must_inspect_result_
NTSTATUS
FxInitialize(
    __inout     PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in        MdDriverObject DriverObject,
    __in        PCUNICODE_STRING RegistryPath,
    __in_opt    PWDF_DRIVER_CONFIG DriverConfig
    )

/*++

Routine Description:

    This is the global framework initialization routine.

    This is called when the framework is loaded, and by
    any drivers that use the framework.

    It is safe to call if already initialized, to handle
    cases where multiple drivers are sharing a common
    kernel DLL.

    This method is used instead of relying on C++ static class
    constructors in kernel mode.

Arguments:


Returns:

    NTSTATUS

--*/

{
    NTSTATUS status;
    BOOLEAN windowsVerifierOn = FALSE;

    UNREFERENCED_PARAMETER(DriverConfig);

    //
    // Check if windows driver verifier is on for this driver
    // We need this when initializing wdf verifier
    //
    windowsVerifierOn = IsWindowsVerifierOn(DriverObject);

    //
    // Get registry values first since these effect the
    // rest of initialization
    //
    FxRegistrySettingsInitialize(FxDriverGlobals,
                                 RegistryPath,
                                 windowsVerifierOn);

    //
    // Initialize IFR logging
    //
    FxIFRStart(FxDriverGlobals, RegistryPath, DriverObject);

    DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_VERBOSE, TRACINGDRIVER,
                        "Initialize globals for %!wZ!", RegistryPath);

    //
    // Only first one initializes the frameworks globals
    //
    status = FxPoolPackageInitialize(FxDriverGlobals);
    if (!NT_SUCCESS(status)) {
        //
        // FxPoolPackageInitialize logs a message in case of failure so
        // we don't need to log failure here.
        //
        FxIFRStop(FxDriverGlobals);
        return status;
    }

    //
    // Lock verifier package
    //
    FxVerifierLockInitialize(FxDriverGlobals);

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // Cache driver info for bugcheck callback.
    //
    FxCacheBugCheckDriverInfo(FxDriverGlobals);

    //
    // Register for bugcheck callbacks.
    //
    FxRegisterBugCheckCallback(FxDriverGlobals, DriverObject);
#endif

    if  (NULL != RegistryPath) {
        if  (FALSE == FxDriverGlobals->IsCorrectVersionRegistered(RegistryPath))
            FxDriverGlobals->RegisterClientVersion(RegistryPath);
    }

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    if(FxDriverGlobals->FxVerifierOn){
        LockVerifierSection(FxDriverGlobals, RegistryPath);
    }
#endif

    return STATUS_SUCCESS;
}

BOOLEAN
IsWindowsVerifierOn(
    _In_ MdDriverObject DriverObject
    )
{
    BOOLEAN windowsVerifierOn = FALSE;

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // Check if windows driver verifier is on for this driver
    // We need this when initializing wdf verifier
    //
    windowsVerifierOn = MmIsDriverVerifying(DriverObject) ? TRUE: FALSE;

#else
    UNREFERENCED_PARAMETER(DriverObject);

    //
    // For user-mode we check if app verifier's verifier.dll is loaded in this
    // process (since app verifier doesn't provide any other way to detect its
    // presence).
    //
    windowsVerifierOn = (GetModuleHandleW(L"verifier.dll") == NULL ? FALSE : TRUE);
#endif

    return windowsVerifierOn;
}

VOID
FxDestroy(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )

/*++

Routine Description:

    This is the global framework uninitialization routine.

    It is here for symmetry, and to allow a shared DLL based frameworks
    to unload safely.

Arguments:


Returns:

    NTSTATUS

--*/

{
    //
    // Release the last reference.
    //
    FxDriverGlobals->RELEASE(FxDestroy);

    //
    // Wait for everyone else to be done.
    //
    Mx::MxEnterCriticalRegion();
    FxDriverGlobals->DestroyEvent.WaitFor(Executive, KernelMode, FALSE, NULL);
    Mx::MxLeaveCriticalRegion();

    //
    // Lock verifier package
    //
    FxVerifierLockDestroy(FxDriverGlobals);

    //
    // Cleanup frameworks structures
    //
    FxPoolPackageDestroy(FxDriverGlobals);

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // Deregister from the bugcheck callbacks.
    //
    FxUnregisterBugCheckCallback(FxDriverGlobals);

    //
    // Purge driver info from bugcheck data.
    //
    FxPurgeBugCheckDriverInfo(FxDriverGlobals);
#endif

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // unlock verifier image sections
    //
    if(FxDriverGlobals->FxVerifierOn){
        UnlockVerifierSection(FxDriverGlobals);
    }
#endif

    return;
}

_Must_inspect_result_
PWDF_DRIVER_GLOBALS
FxAllocateDriverGlobals(
    VOID
    )
{
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;
    KIRQL               irql;
    NTSTATUS            status;

    pFxDriverGlobals = (PFX_DRIVER_GLOBALS)
        MxMemory::MxAllocatePoolWithTag(NonPagedPool, sizeof(FX_DRIVER_GLOBALS), FX_TAG);

    if (pFxDriverGlobals == NULL) {
        return NULL;
    }

    RtlZeroMemory(pFxDriverGlobals, sizeof(FX_DRIVER_GLOBALS));

    pFxDriverGlobals->Refcnt = 1;

    status = pFxDriverGlobals->DestroyEvent.Initialize(NotificationEvent, FALSE);
#if (FX_CORE_MODE==FX_CORE_USER_MODE)
    if (!NT_SUCCESS(status)) {
        MxMemory::MxFreePool(pFxDriverGlobals);
        return NULL;
    }
#else
    UNREFERENCED_PARAMETER(status);
#endif

    //
    // Initialize this new FxDriverGlobals structure.
    //
    FxLibraryGlobals.FxDriverGlobalsListLock.Acquire(&irql);
    InsertHeadList(&FxLibraryGlobals.FxDriverGlobalsList,
                   &pFxDriverGlobals->Linkage);
    FxLibraryGlobals.FxDriverGlobalsListLock.Release(irql);

    pFxDriverGlobals->WdfHandleMask                  = FxHandleValueMask;
    pFxDriverGlobals->WdfVerifierAllocateFailCount   = (ULONG) -1;
    pFxDriverGlobals->Driver                         = NULL;
    pFxDriverGlobals->DebugExtension                 = NULL;
    pFxDriverGlobals->LibraryGlobals                 = &FxLibraryGlobals;
    pFxDriverGlobals->WdfLogHeader                   = NULL;

    //
    // Verifier settings.  Off by default.
    //
    pFxDriverGlobals->SetVerifierState(FALSE);

    //
    // By default don't apply latest-version restricted verifier checks
    // to downlevel version drivers.
    //
    pFxDriverGlobals->FxVerifyDownlevel              = FALSE;

    //
    // Verbose is separate knob
    //
    pFxDriverGlobals->FxVerboseOn                    = FALSE;

    //
    // Do not parent queue presented requests.
    // This performance optimization is on by default.
    //
    pFxDriverGlobals->FxRequestParentOptimizationOn  = TRUE;

    //
    // Enhanced verifier options. Off by default
    //
    pFxDriverGlobals->FxEnhancedVerifierOptions = 0;

    //
    // If FxVerifierDbgBreakOnError is true, WaitForSignal interrupts the
    // execution of the system after waiting for the specified number
    // of seconds. Developer will have an opportunity to validate the state
    // of the driver when breakpoint is hit. Developer can continue to wait
    // by entering 'g' in the debugger.
    //
    pFxDriverGlobals->FxVerifierDbgWaitForSignalTimeoutInSec = 60;

    //
    // Timeout used by the wake interrupt ISR in WaitForSignal to catch
    // scenarios where the interrupt ISR is blocked because the device stack
    // is taking too long to power up
    //
    pFxDriverGlobals->DbgWaitForWakeInterruptIsrTimeoutInSec = 60;

    //
    // Minidump log related settings.
    //
    pFxDriverGlobals->FxForceLogsInMiniDump          = FALSE;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    pFxDriverGlobals->FxTrackDriverForMiniDumpLog    = TRUE;
    pFxDriverGlobals->IsUserModeDriver               = FALSE;
#else
    pFxDriverGlobals->FxTrackDriverForMiniDumpLog    = FALSE;
    pFxDriverGlobals->IsUserModeDriver               = TRUE;
#endif

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // Minidump driver info related settings.
    //
    pFxDriverGlobals->BugCheckDriverInfoIndex        = 0;
#endif

    //
    // By default disable the support for device simulation framework (DSF).
    //
    pFxDriverGlobals->FxDsfOn  = FALSE;

    //
    // Allocate a telemetry context if a telemetry client is enabled, for any level/keyword.
    //
    pFxDriverGlobals->TelemetryContext = NULL;
    if (TraceLoggingProviderEnabled(g_TelemetryProvider, 0 ,0)) {
        AllocAndInitializeTelemetryContext(&(pFxDriverGlobals->TelemetryContext));
    }

    return &pFxDriverGlobals->Public;
}

VOID
FxFreeDriverGlobals(
    __in PWDF_DRIVER_GLOBALS DriverGlobals
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    KIRQL irql;

    pFxDriverGlobals = GetFxDriverGlobals(DriverGlobals);

    FxLibraryGlobals.FxDriverGlobalsListLock.Acquire(&irql);
    RemoveEntryList(&pFxDriverGlobals->Linkage);
    InitializeListHead(&pFxDriverGlobals->Linkage);
    FxLibraryGlobals.FxDriverGlobalsListLock.Release(irql);

    if (pFxDriverGlobals->DebugExtension != NULL) {

        FxFreeAllocatedMdlsDebugInfo(pFxDriverGlobals->DebugExtension);

        if (pFxDriverGlobals->DebugExtension->ObjectDebugInfo != NULL) {
            MxMemory::MxFreePool(pFxDriverGlobals->DebugExtension->ObjectDebugInfo);
            pFxDriverGlobals->DebugExtension->ObjectDebugInfo = NULL;
        }

        pFxDriverGlobals->DebugExtension->AllocatedTagTrackersLock.Uninitialize();

        MxMemory::MxFreePool(pFxDriverGlobals->DebugExtension);
        pFxDriverGlobals->DebugExtension = NULL;
    }

    //
    // Cleanup event b/c d'tor is not called for MxAllocatePoolWithTag.
    //
    pFxDriverGlobals->DestroyEvent.Uninitialize();

    if (NULL != pFxDriverGlobals->TelemetryContext) {
        MxMemory::MxFreePool(pFxDriverGlobals->TelemetryContext);
        pFxDriverGlobals->TelemetryContext = NULL;
    }

    MxMemory::MxFreePool(pFxDriverGlobals);
}

_Must_inspect_result_
FxObjectDebugInfo*
FxVerifierGetObjectDebugInfo(
    __in HANDLE Key,
    __in PFX_DRIVER_GLOBALS  FxDriverGlobals
    )

/*++

Routine Description:
    Attempts to open a value under the passed in key and create an array of
    FxObjectDebugInfo.

Arguments:
    Key - Registry key to query the value for

Return Value:
    NULL or a pointer which should be freed by the caller using ExFreePool

--*/

{
    FxObjectDebugInfo* pInfo;
    PVOID dataBuffer;
    NTSTATUS status;
    ULONG length, type;
    DECLARE_CONST_UNICODE_STRING(valueName, L"TrackHandles");

    pInfo = NULL;
    type = REG_MULTI_SZ;
    length = 0;

    //
    // Find out how big a buffer we need to allocate if the value is present
    //
    status = FxRegKey::_QueryValue(FxDriverGlobals,
                                   Key,
                                   &valueName,
                                   length,
                                   NULL,
                                   &length,
                                   &type);

    //
    // We expect the list to be bigger then a standard partial, so if it is
    // not, just bail now.
    //
    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL) {
        return NULL;
    }

    //
    // Pool can be paged b/c we are running at PASSIVE_LEVEL and we are going
    // to free it at the end of this function.
    //
    dataBuffer = MxMemory::MxAllocatePoolWithTag(PagedPool, length, FxDriverGlobals->Tag);
    if (dataBuffer == NULL) {
        return NULL;
    }

    //
    // Requery now that we have a big enough buffer
    //
    status = FxRegKey::_QueryValue(FxDriverGlobals,
                                   Key,
                                   &valueName,
                                   length,
                                   dataBuffer,
                                   &length,
                                   &type);
    if (NT_SUCCESS(status)) {
        //
        // Verify that the data from the registry is a valid multi-sz string.
        //
        status = FxRegKey::_VerifyMultiSzString(FxDriverGlobals,
                                                &valueName,
                                                (PWCHAR) dataBuffer,
                                                length);
    }

    if (NT_SUCCESS(status)) {
#pragma prefast(push)
#pragma prefast(suppress:__WARNING_PRECONDITION_NULLTERMINATION_VIOLATION, "FxRegKey::_VerifyMultiSzString makes sure the string is NULL-terminated")
        pInfo = FxVerifyAllocateDebugInfo((LPWSTR) dataBuffer, FxDriverGlobals);
#pragma prefast(pop)

    }

    MxMemory::MxFreePool(dataBuffer);

    return pInfo;
}

VOID
FxVerifierQueryTrackPower(
    __in HANDLE Key,
    __out FxTrackPowerOption* TrackPower
    )
{
    NTSTATUS status;
    ULONG value = 0;
    DECLARE_CONST_UNICODE_STRING(valueName, L"TrackPower");

    status = FxRegKey::_QueryULong(Key, &valueName, &value);
    if (NT_SUCCESS(status) && value < FxTrackPowerMaxValue) {
        *TrackPower = (FxTrackPowerOption)value;
    }
    else {
        *TrackPower = FxTrackPowerNone;
    }
}

VOID
FxOverrideDefaultVerifierSettings(
    __in    HANDLE Key,
    __in    LPWSTR Name,
    __out   PBOOLEAN OverrideValue
    )
{
    UNICODE_STRING valueName;
    ULONG value = 0;

    RtlInitUnicodeString(&valueName, Name);

    if (NT_SUCCESS(FxRegKey::_QueryULong(Key,
                                         (PCUNICODE_STRING)&valueName,
                                         &value))) {
        if (value) {
            *OverrideValue = TRUE;
        } else {
            *OverrideValue = FALSE;
        }
    }

}


VOID
FxRegistrySettingsInitialize(
    __inout PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PCUNICODE_STRING RegistryPath,
    __in BOOLEAN WindowsVerifierOn
    )

/*++

Routine Description:

    Initialize Driver Framework settings from the driver
    specific registry settings under

    \REGISTRY\MACHINE\SYSTEM\ControlSetxxx\Services\<driver>\Parameters\Wdf

Arguments:

    RegistryPath - Registry path passed to DriverEntry

--*/

{
    NTSTATUS status;
    RTL_QUERY_REGISTRY_TABLE paramTable[10];
    ULONG verifierOnValue;
    ULONG verifyDownlevelValue;
    ULONG verboseValue;
    ULONG allocateFailValue;
    ULONG forceLogsInMiniDump;
    ULONG trackDriverForMiniDumpLog;
    ULONG requestParentOptimizationOn;
    ULONG dsfValue;
    ULONG removeLockOptionFlags;
    ULONG zero = 0;
    ULONG max = 0xFFFFFFFF;
    ULONG defaultTrue = (ULONG) TRUE;
    ULONG i;
    ULONG timeoutValue = 0;
    FxAutoRegKey hDriver, hWdf;
    DECLARE_CONST_UNICODE_STRING(parametersPath, L"Parameters\\Wdf");

    typedef NTSTATUS NTAPI QUERYFN(
        ULONG, PCWSTR, PRTL_QUERY_REGISTRY_TABLE, PVOID, PVOID);

    QUERYFN* queryFn;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    UNICODE_STRING FunctionName;
#endif

    //
    // UMDF may not provide this registry path
    //
    if (NULL == RegistryPath) {
        return;
    }

    status = FxRegKey::_OpenKey(NULL, RegistryPath, &hDriver.m_Key, KEY_READ);
    if (!NT_SUCCESS(status)) {
        return;
    }

    status = FxRegKey::_OpenKey(hDriver.m_Key, &parametersPath, &hWdf.m_Key, KEY_READ);
    if (!NT_SUCCESS(status)) {
        //
        // For version >= 1.9 we enable WDF verifier automatically when driver
        // verifier or app verifier is enabled. Since inbox drivers may not have
        // WDF subkey populated as they may not use INF, we need to enable
        // verifier even if we fail to open wdf subkey (if DriverVerifier is on).
        //
        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9)) {
            //
            // Verifier settings are all or nothing.  We currently do not support
            // turning on individual sub-verifiers.
            //
            FxDriverGlobals->SetVerifierState(WindowsVerifierOn);
            if (FxDriverGlobals->FxVerifierOn) {
                FxDriverGlobalsInitializeDebugExtension(FxDriverGlobals, NULL);
            }
        }

        return;
    }

    RtlZeroMemory (&paramTable[0], sizeof(paramTable));
    i = 0;

    verboseValue = 0;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"VerboseOn";
    paramTable[i].EntryContext  = &verboseValue;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &zero;
    paramTable[i].DefaultLength = sizeof(ULONG);

    allocateFailValue = (ULONG) -1;
    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"VerifierAllocateFailCount";
    paramTable[i].EntryContext  = &allocateFailValue;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &max;
    paramTable[i].DefaultLength = sizeof(ULONG);

    verifierOnValue = 0;

    //
    // If the client version is 1.9 or above, the defaut (i.e when
    // the key is not present) VerifierOn state is tied to the
    // driver verifier.
    //
    if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9)) {
        verifierOnValue = WindowsVerifierOn;
    }

    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"VerifierOn";
    paramTable[i].EntryContext  = &verifierOnValue;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &verifierOnValue;
    paramTable[i].DefaultLength = sizeof(ULONG);

    verifyDownlevelValue = 0;
    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"VerifyDownLevel";
    paramTable[i].EntryContext  = &verifyDownlevelValue;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &zero;
    paramTable[i].DefaultLength = sizeof(ULONG);

    forceLogsInMiniDump = 0;
    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"ForceLogsInMiniDump";
    paramTable[i].EntryContext  = &forceLogsInMiniDump;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &zero;
    paramTable[i].DefaultLength = sizeof(ULONG);

    //
    // Track driver for minidump log:
    //  Default for KMDF is on.
    //  Default for UMDF is off.
    //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    trackDriverForMiniDumpLog = (ULONG) TRUE;
#else
    trackDriverForMiniDumpLog = 0;
#endif
    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"TrackDriverForMiniDumpLog";
    paramTable[i].EntryContext  = &trackDriverForMiniDumpLog;
    paramTable[i].DefaultType   = REG_DWORD;
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    paramTable[i].DefaultData    = &defaultTrue;
#else
    paramTable[i].DefaultData    = &zero;
#endif
    paramTable[i].DefaultLength = sizeof(ULONG);

    requestParentOptimizationOn = (ULONG) TRUE;
    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"RequestParentOptimizationOn";
    paramTable[i].EntryContext  = &requestParentOptimizationOn;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &defaultTrue;
    paramTable[i].DefaultLength = sizeof(ULONG);

    dsfValue = 0;
    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"DsfOn";
    paramTable[i].EntryContext  = &dsfValue;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &zero;
    paramTable[i].DefaultLength = sizeof(ULONG);

    removeLockOptionFlags = 0;
    i++;

    paramTable[i].Flags         = RTL_QUERY_REGISTRY_DIRECT;
    paramTable[i].Name          = L"RemoveLockOptionFlags";
    paramTable[i].EntryContext  = &removeLockOptionFlags;
    paramTable[i].DefaultType   = REG_DWORD;
    paramTable[i].DefaultData   = &zero;
    paramTable[i].DefaultLength = sizeof(ULONG);

    ASSERT(i < sizeof(paramTable) / sizeof(paramTable[0]));

#if (FX_CORE_MODE==FX_CORE_USER_MODE)

    queryFn = (QUERYFN*) GetProcAddress(
        GetModuleHandle(TEXT("ntdll.dll")),
        "RtlQueryRegistryValuesEx"
        );

#else

    RtlInitUnicodeString(&FunctionName, L"RtlQueryRegistryValuesEx");

#pragma warning(push)
#pragma warning(disable: 4055)

    queryFn  = (QUERYFN*)MmGetSystemRoutineAddress(&FunctionName);

#pragma warning(pop)

#endif

    if (queryFn == NULL) {
        queryFn = &RtlQueryRegistryValues;
    }

    status = queryFn(
        RTL_REGISTRY_OPTIONAL | RTL_REGISTRY_HANDLE,
        (PWSTR) hWdf.m_Key,
        &paramTable[0],
        NULL,
        NULL
        );

    //
    // Only examine key values on success
    //
    if (NT_SUCCESS(status)) {

        if (verboseValue) {
            FxDriverGlobals->FxVerboseOn = TRUE;
        }
        else {
            FxDriverGlobals->FxVerboseOn = FALSE;
        }

        if (allocateFailValue != (ULONG) -1) {
            FxDriverGlobals->WdfVerifierAllocateFailCount = (LONG) allocateFailValue;
        }
        else {
            FxDriverGlobals->WdfVerifierAllocateFailCount = -1;
        }

        //
        // Verifier settings are all or nothing.  We currently do not support
        // turning on individual sub-verifiers.
        //
        FxDriverGlobals->SetVerifierState(verifierOnValue ? TRUE : FALSE);

        if (FxDriverGlobals->FxVerifierOn) {
            FxDriverGlobalsInitializeDebugExtension(FxDriverGlobals, hWdf.m_Key);
        }

        //
        // Update FxVerifyDownLevel independent of FxVerifyOn because for UMDF
        // verifer is always on so it does not consume FxVerifyOn value
        //
        if (verifyDownlevelValue) {
            FxDriverGlobals->FxVerifyDownlevel = TRUE;
        }
        else {
            FxDriverGlobals->FxVerifyDownlevel = FALSE;
        }

        //
        // See if there exists an override in the registry for WDFVERIFY state.
        // We query for this separately so that we can establish a default state
        // based on verifierOnValue, and then know if the value was present in
        // the registry to override the default.
        //
        FxOverrideDefaultVerifierSettings(hWdf.m_Key,
                                          L"VerifyOn",
                                          &FxDriverGlobals->FxVerifyOn);

        if (FxDriverGlobals->FxVerifyOn) {
            FxDriverGlobals->Public.DriverFlags |= WdfVerifyOn;
        }

        FxOverrideDefaultVerifierSettings(hWdf.m_Key,
                                          L"DbgBreakOnError",
                                          &FxDriverGlobals->FxVerifierDbgBreakOnError);

        FxOverrideDefaultVerifierSettings(hWdf.m_Key,
                                          L"DbgBreakOnDeviceStateError",
                                          &FxDriverGlobals->FxVerifierDbgBreakOnDeviceStateError);

        if (FxDriverGlobals->FxVerifierDbgBreakOnError) {
            timeoutValue = 0;
            DECLARE_CONST_UNICODE_STRING(timeoutName, L"DbgWaitForSignalTimeoutInSec");

            //
            // Get the override value for the WaitForSignal's timeout if present.
            //
            if (NT_SUCCESS(FxRegKey::_QueryULong(hWdf.m_Key,
                                                 &timeoutName,
                                                 &timeoutValue))) {

                FxDriverGlobals->FxVerifierDbgWaitForSignalTimeoutInSec = timeoutValue;
            }
        }

        timeoutValue = 0;
        DECLARE_CONST_UNICODE_STRING(timeoutName, L"DbgWaitForWakeInterruptIsrTimeoutInSec");

        //
        // Get the override value for the Wake Interrupt ISR timeout if present.
        // Since the wake interrupt feature is only supported for 1.13 and higher,
        // avoid querying the reg key for older versions
        //
        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,13) &&
            NT_SUCCESS(FxRegKey::_QueryULong(hWdf.m_Key,
                                             &timeoutName,
                                             &timeoutValue))) {

            FxDriverGlobals->DbgWaitForWakeInterruptIsrTimeoutInSec = timeoutValue;
        }

        FxDriverGlobals->FxForceLogsInMiniDump =
                            (forceLogsInMiniDump) ? TRUE : FALSE;

        FxDriverGlobals->FxTrackDriverForMiniDumpLog =
                            (trackDriverForMiniDumpLog) ? TRUE : FALSE;

        FxDriverGlobals->FxRequestParentOptimizationOn =
                            (requestParentOptimizationOn) ? TRUE : FALSE;

        FxDriverGlobals->FxDsfOn = (dsfValue) ? TRUE : FALSE;

        FxDriverGlobals->RemoveLockOptionFlags = removeLockOptionFlags;
    }

    return;
}

VOID
FX_DRIVER_GLOBALS::WaitForSignal(
    __in MxEvent* Event,
    __in PCSTR ReasonForWaiting,
    __in WDFOBJECT Handle,
    __in ULONG WarningTimeoutInSec,
    __in ULONG WaitSignalFlags
    )
{
    LARGE_INTEGER timeOut;
    NTSTATUS status;

    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    timeOut.QuadPart = WDF_REL_TIMEOUT_IN_SEC(((ULONGLONG)WarningTimeoutInSec));

    do {
        status = Event->WaitFor(Executive,
                        KernelMode,
                        FALSE, // Non alertable
                        timeOut.QuadPart ? &timeOut : NULL);

        if(status == STATUS_TIMEOUT) {
            DbgPrint("Thread 0x%p is %s 0x%p\n",
		      Mx::MxGetCurrentThread(),
                      ReasonForWaiting,
                      Handle);

            if ((WaitSignalFlags & WaitSignalAlwaysBreak) ||
                ((WaitSignalFlags & WaitSignalBreakUnderVerifier) &&
                 FxVerifierDbgBreakOnError) ||
                ((WaitSignalFlags & WaitSignalBreakUnderDebugger) &&
                 IsDebuggerAttached())) {

                DbgBreakPoint();
            }
        } else {
            ASSERT(NT_SUCCESS(status));
            break;
        }
    } WHILE(TRUE);
}

} // extern "C"
