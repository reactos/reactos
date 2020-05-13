#include "common/fxglobals.h"
#include "common/fxpool.h"
#include "common/fxmacros.h"
#include "primitives/mxmemory.h"
#include "common/dbgtrace.h"
#include "common/fxregkey.h"
#include "common/fxautoregistry.h"
#include "fxobject/fxobjectinfodata.h"
#include "common/fxtrace.h"

extern "C" {

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
FxOverrideDefaultVerifierSettings(
    __in    HANDLE Key,
    __in    PCWSTR Name,
    //__in    LPWSTR Name,
    __out   PBOOLEAN OverrideValue
    );

void
__cxa_pure_virtual()
{
    __debugbreak();
}


VOID
FxLibraryGlobalsVerifyVersion()
{
	OSVERSIONINFOEXW osvi;
	DWORDLONG dwlConditionMask = 0;
	NTSTATUS status;

	//Check Windows Xp Sp2
	RtlZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 1;
	osvi.wServicePackMajor = 2;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_LESS);

	status = RtlVerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask);
	if (NT_SUCCESS(status))
	{
		FxLibraryGlobals.AllowWmiUpdates = FALSE;
	}

	//Check Windows Server 2003
	RtlZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 5;
	osvi.dwMinorVersion = 2;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_SERVICEPACKMAJOR, VER_EQUAL);

	status = RtlVerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_SERVICEPACKMAJOR, dwlConditionMask);
	if (NT_SUCCESS(status))
	{
		FxLibraryGlobals.AllowWmiUpdates = FALSE;
	}

	//Check Windows Vista
	RtlZeroMemory(&osvi, sizeof(OSVERSIONINFOEXW));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW);
	osvi.dwMajorVersion = 6;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER);

	status = RtlVerifyVersionInfo(&osvi, VER_MAJORVERSION, dwlConditionMask);
	if (NT_SUCCESS(status))
	{
		FxLibraryGlobals.UseTargetSystemPowerState = TRUE;
	}
}

NTSTATUS
FxLibraryGlobalsCommission(VOID)
{
	void(__stdcall * pfnRtlGetVersion)(PRTL_OSVERSIONINFOW);
	UNICODE_STRING SystemRoutineName;
	UNICODE_STRING DestinationString;

	FxLibraryGlobals.StaticallyLinked = FALSE;
	if (!RtlCompareMemory(WdfLdrType, "WdfStatic", 0xAu))
		FxLibraryGlobals.StaticallyLinked = TRUE;
	
	RtlInitUnicodeString(&DestinationString, L"KdRefreshDebuggerNotPresent");
	FxLibraryGlobals.pfn_KdRefresh = (PFNKDREFRESH)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KeFlushQueuedDpcs");
	FxLibraryGlobals.pfn_KeFlushQueuedDpcs = (PFNKEFLUSHQUEUEDDPCS)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KfRaiseIrql");
	FxLibraryGlobals.pfn_KfRaiseIrql = (PFNKFRAISEIRQL)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KfLowerIrql");
	FxLibraryGlobals.pfn_KfLowerIrql = (PFNKKFLOWERIRQL)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"IoSetCompletionRoutineEx");
	FxLibraryGlobals.pfn_IoSetCompletionRoutineEx = (PFNIOSETCOMPLETIONROUTINEEX)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"WmiTraceMessageVa");
	FxLibraryGlobals.pfn_WmiTraceMessageVa = (PFNWMITRACEMESSAGEVA)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"WmiQueryTraceInformation");
	FxLibraryGlobals.pfn_NtWmiQueryTraceInformation = (PFNNTWWIQUERYTRACEINFORMATION)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KeAcquireInterruptSpinLock");
	FxLibraryGlobals.pfn_KeAcquireInterruptSpinLock = (PFNKEACQUIREINTERRRUPTSPINLOCK)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KeReleaseInterruptSpinLock");
	FxLibraryGlobals.pfn_KeReleaseInterruptSpinLock = (PFNKERELEASEINTERRUPTSPINLOCK)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"InterlockedPopEntrySList");
	FxLibraryGlobals.pfn_FxInterlockedPopEntrySList = (PFNINTERLOCKEDPOPENTRYSLIST)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"InterlockedPushEntrySList");
	FxLibraryGlobals.pfn_FxInterlockedPushEntrySList = (PFNINTERLOCKEDPUSHENTRYSLIST)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"IoConnectInterruptEx");
	FxLibraryGlobals.pfn_IoConnectInterruptEx = (PFNIOCONNECTINTERRUPTEX)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"IoDisconnectInterruptEx");
	FxLibraryGlobals.pfn_IoDisconnectInterruptEx = (PFNIODISCONNECTINTERRUPTEX)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"PoGetSystemWake");
	FxLibraryGlobals.pfn_PoGetSystemWake = (PFNPOGETSYSTEMWAKE)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"PoSetSystemWake");
	FxLibraryGlobals.pfn_PoSetSystemWake = (PFNPOSETSYSTEMWAKE)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KeQueryActiveProcessors");
	FxLibraryGlobals.pfn_KeQueryActiveProcessors = (PFNKEQUERYACTIVEPROCESSORS)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KeSetTargetProcessorDpc");
	FxLibraryGlobals.pfn_KeSetTargetProcessorDpc = (PFNKESETTARGETPROCESSORDPC)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KeQueryActiveGroupCount");

	if (MmGetSystemRoutineAddress(&DestinationString))
		FxLibraryGlobals.ProcessorGroupSupport = TRUE;

	RtlInitUnicodeString(&DestinationString, L"KeSetCoalescableTimer");
	FxLibraryGlobals.pfn_KeSetCoalescableTimer = (PFNKESETCOALESCABLETIMER)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"KeAreApcsDisabled");
	FxLibraryGlobals.pfn_KeAreApcsDisabled = (PFNKEAREAPCSDISABLED)MmGetSystemRoutineAddress(&DestinationString);
	RtlInitUnicodeString(&DestinationString, L"IoUnregisterPlugPlayNotificationEx");
	FxLibraryGlobals.pfn_IoUnregisterPlugPlayNotificationEx = (PFNIOUNREGISTERPLUGPLAYNOTIFICATIONEX)MmGetSystemRoutineAddress(&DestinationString);	
	FxLibraryGlobals.OsVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	FxLibraryGlobals.AllowWmiUpdates = TRUE;

	RtlInitUnicodeString(&SystemRoutineName, L"RtlGetVersion");
	pfnRtlGetVersion = (void(__stdcall*)(PRTL_OSVERSIONINFOW))MmGetSystemRoutineAddress(&SystemRoutineName);

	if (pfnRtlGetVersion)
	{
		pfnRtlGetVersion((PRTL_OSVERSIONINFOW)&FxLibraryGlobals.OsVersionInfo);
		FxLibraryGlobalsVerifyVersion();
	}
	else
	{
		SystemRoutineName.Length = sizeof(FxLibraryGlobals.OsVersionInfo.szCSDVersion);
		SystemRoutineName.MaximumLength = sizeof(FxLibraryGlobals.OsVersionInfo.szCSDVersion);
		SystemRoutineName.Buffer = FxLibraryGlobals.OsVersionInfo.szCSDVersion;
		PsGetVersion(&FxLibraryGlobals.OsVersionInfo.dwMajorVersion, 
					 &FxLibraryGlobals.OsVersionInfo.dwMinorVersion, 
					 &FxLibraryGlobals.OsVersionInfo.dwBuildNumber, 
					 &SystemRoutineName);
	}

	RtlZeroMemory(&FxLibraryGlobals.MachineSleepStates, sizeof(FxLibraryGlobals.MachineSleepStates));	
	InitializeListHead(&FxLibraryGlobals.FxDriverGlobalsList);
	FxLibraryGlobals.FxDriverGlobalsListLock.Initialize();
	FxInitializeBugCheckDriverInfo();
	FxLibraryGlobals.DriverTracker.Initialize();
	FxLibraryGlobals.EnhancedVerifierSectionHandle = NULL;

	return STATUS_SUCCESS;
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

_Must_inspect_result_
PWDF_DRIVER_GLOBALS
FxAllocateDriverGlobals(
    VOID
    )
{
    PFX_DRIVER_GLOBALS  pFxDriverGlobals;
    KIRQL               irql;
    //NTSTATUS            status;

    pFxDriverGlobals = (PFX_DRIVER_GLOBALS)
        MxMemory::MxAllocatePoolWithTag(NonPagedPool, sizeof(FX_DRIVER_GLOBALS), FX_TAG);

    if (pFxDriverGlobals == NULL)
	{
        return NULL;
    }

    RtlZeroMemory(pFxDriverGlobals, sizeof(FX_DRIVER_GLOBALS));

    //
    // Initialize this new FxDriverGlobals structure.
    //
    FxLibraryGlobals.FxDriverGlobalsListLock.Acquire(&irql);
    InsertHeadList(&FxLibraryGlobals.FxDriverGlobalsList,
                   &pFxDriverGlobals->Linkage);
    FxLibraryGlobals.FxDriverGlobalsListLock.Release(irql);

    pFxDriverGlobals->WdfHandleMask                  = 0xFFFFFFF8;//FxHandleValueMask;
    pFxDriverGlobals->WdfVerifierAllocateFailCount   = (ULONG) -1;
    pFxDriverGlobals->Driver                         = NULL;
    pFxDriverGlobals->DebugExtension                 = NULL;
    pFxDriverGlobals->LibraryGlobals                 = &FxLibraryGlobals;
	pFxDriverGlobals->WdfTraceDelayTime              = 0;
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
    // Minidump log related settings.
    //
    pFxDriverGlobals->FxForceLogsInMiniDump          = FALSE;

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    pFxDriverGlobals->FxTrackDriverForMiniDumpLog    = TRUE;
//    pFxDriverGlobals->IsUserModeDriver               = FALSE;
#else
    pFxDriverGlobals->FxTrackDriverForMiniDumpLog    = FALSE;
//    pFxDriverGlobals->IsUserModeDriver               = TRUE;
#endif

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    //
    // Minidump driver info related settings.
    //
    pFxDriverGlobals->BugCheckDriverInfoIndex        = 0;
#endif

    pFxDriverGlobals->FxVerifierDbgWaitForSignalTimeoutInSec = 60;

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

    if (pFxDriverGlobals->DebugExtension != NULL)
	{

        FxFreeAllocatedMdlsDebugInfo(pFxDriverGlobals->DebugExtension);

        if (pFxDriverGlobals->DebugExtension->ObjectDebugInfo != NULL)
		{
            MxMemory::MxFreePool(pFxDriverGlobals->DebugExtension->ObjectDebugInfo);
            pFxDriverGlobals->DebugExtension->ObjectDebugInfo = NULL;
        }

        pFxDriverGlobals->DebugExtension->AllocatedTagTrackersLock.Uninitialize();

        MxMemory::MxFreePool(pFxDriverGlobals->DebugExtension);
        pFxDriverGlobals->DebugExtension = NULL;
    }

    MxMemory::MxFreePool(pFxDriverGlobals);
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
    //if(FxDriverGlobals->FxVerifierOn){
    //    UnlockVerifierSection(FxDriverGlobals);
    //}
#endif

    return;
}

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
VOID
UnlockVerifierSection(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if( FxLibraryGlobals.EnhancedVerifierSectionHandle != NULL)
	{
        //LONG count;

        //count = InterlockedDecrement(&FxLibraryGlobals.VerifierSectionHandleRefCount);
        //ASSERT(count >= 0);

        MmUnlockPagableImageSection(FxLibraryGlobals.EnhancedVerifierSectionHandle);
        //DoTraceLevelMessage(FxDriverGlobals, TRACE_LEVEL_INFORMATION, TRACINGDRIVER,
        //                    "Decrement UnLock counter (%d) for Verifier Paged Memory "
        //                    "with driver globals %p",
        //                    count, FxDriverGlobals);
    }
}
#endif

VOID
FxFreeAllocatedMdlsDebugInfo(
    __in FxDriverGlobalsDebugExtension* DebugExtension
    )
{
    FxAllocatedMdls* pNext, *pCur;

    pNext = DebugExtension->AllocatedMdls.Next;

    //
    // MDL leaks were already checked for in FxPoolDestroy, just free all
    // the tables here.
    //
    while (pNext != NULL)
	{
        pCur = pNext;
        pNext = pCur->Next;

        ExFreePool(pCur);
    }
}

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
    if (!NT_SUCCESS(status))
	{
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

    return STATUS_SUCCESS;
}

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
BOOLEAN
NTAPI
IsDriverVerifierActive(
    _In_ MdDriverObject DriverObject
    )
/*++

Routine Description:

    This function checks whether WDF verification is turned on or not.

Arguments:

    DriverObject - Driver to test if WDF verification turned on.

Returns:

    TRUE if WDF verification is turned on. False otherwise.

--*/
{
	return MmIsDriverVerifying (DriverObject) > 0;
}
#endif

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
    windowsVerifierOn = IsDriverVerifierActive(DriverObject);

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

    if (pExtension == NULL)
	{
        return;
    }

    RtlZeroMemory(pExtension, sizeof(*pExtension));

    pExtension->AllocatedTagTrackersLock.Initialize();

    InitializeListHead(&pExtension->AllocatedTagTrackersListHead);

    FxDriverGlobals->DebugExtension = pExtension;

    if (Key != NULL)
	{
        pExtension->ObjectDebugInfo = FxVerifierGetObjectDebugInfo(
                                                        Key,
                                                        FxDriverGlobals
                                                        );
    }

#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    KeInitializeSpinLock(&pExtension->AllocatedMdlsLock);
#endif
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

    (KMDF)
    \REGISTRY\MACHINE\SYSTEM\ControlSetxxx\Services\<driver>\Parameters\Wdf

    (UMDF)
    \REGISTRY\MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\WUDF\Services\<driver>

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
    if (NULL == RegistryPath)
	{
        return;
    }

    status = FxRegKey::_OpenKey(NULL, RegistryPath, &hDriver.m_Key, KEY_READ);
    if (!NT_SUCCESS(status))
	{
        return;
    }

    status = FxRegKey::_OpenKey(hDriver.m_Key, &parametersPath, &hWdf.m_Key, KEY_READ);
    if (!NT_SUCCESS(status))
	{
        //
        // For version >= 1.9 we enable WDF verifier automatically when driver
        // verifier or app verifier is enabled. Since inbox drivers may not have
        // WDF subkey populated as they may not use INF, we need to enable
        // verifier even if we fail to open wdf subkey (if DriverVerifier is on).
        //
        if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9))
		{
            //
            // Verifier settings are all or nothing.  We currently do not support
            // turning on individual sub-verifiers.
            //
            FxDriverGlobals->SetVerifierState(WindowsVerifierOn);
            if (FxDriverGlobals->FxVerifierOn)
			{
                FxDriverGlobalsInitializeDebugExtension(FxDriverGlobals, NULL);
            }
        }

        return;
    }

    RtlZeroMemory (&paramTable[0], sizeof(paramTable));
    i = 0;

    #define ADD_TABLE_ENTRY(ValueName, Value, Default) \
        paramTable[i].Flags = \
            RTL_QUERY_REGISTRY_DIRECT; \
        paramTable[i].Name = L##ValueName; \
        paramTable[i].EntryContext = &Value; \
        paramTable[i].DefaultType = \
            (REG_DWORD) | REG_NONE; \
        paramTable[i].DefaultData = &Default; \
        paramTable[i].DefaultLength = sizeof(ULONG); \
        i++; \
        ASSERT(i < sizeof(paramTable) / sizeof(paramTable[0]));
    
    verboseValue = 0;
    ADD_TABLE_ENTRY("VerboseOn", verboseValue, zero);

    allocateFailValue = (ULONG) -1;
    ADD_TABLE_ENTRY("VerifierAllocateFailCount", allocateFailValue, max);

    verifierOnValue = 0;

    //
    // If the client version is 1.9 or above, the default (i.e when
    // the key is not present) VerifierOn state is tied to the
    // driver verifier.
    //
    if (FxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,9))
	{
        verifierOnValue = WindowsVerifierOn;
    }

    ADD_TABLE_ENTRY("VerifierOn", verifierOnValue, verifierOnValue);

    verifyDownlevelValue = 0;
    ADD_TABLE_ENTRY("VerifyDownLevel", verifyDownlevelValue, zero);

    forceLogsInMiniDump = 0;
    ADD_TABLE_ENTRY("ForceLogsInMiniDump", forceLogsInMiniDump, zero);

    //
    // Track driver for minidump log:
    //  Default for KMDF is on.
    //  Default for UMDF is off.
    //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
    trackDriverForMiniDumpLog = (ULONG) TRUE;
    ADD_TABLE_ENTRY("TrackDriverForMiniDumpLog", trackDriverForMiniDumpLog, defaultTrue);
#else
    trackDriverForMiniDumpLog = 0;
    ADD_TABLE_ENTRY("TrackDriverForMiniDumpLog", trackDriverForMiniDumpLog, zero);
#endif

    requestParentOptimizationOn = (ULONG) TRUE;
    ADD_TABLE_ENTRY("RequestParentOptimizationOn", requestParentOptimizationOn, defaultTrue);

    dsfValue = 0;
    ADD_TABLE_ENTRY("DsfOn", dsfValue, zero);
    
    removeLockOptionFlags = 0;
    ADD_TABLE_ENTRY("RemoveLockOptionFlags", removeLockOptionFlags, zero);

    //
    // The last entry's QueryRoutine and Name fields must be NULL,
    // because that marks the end of the query table.
    //
    ASSERT(i < sizeof(paramTable) / sizeof(paramTable[0]));
    ASSERT(paramTable[i].QueryRoutine == NULL);
    ASSERT(paramTable[i].Name == NULL);

#if (FX_CORE_MODE==FX_CORE_USER_MODE)

    queryFn = (QUERYFN*) GetProcAddress(
        GetModuleHandle(TEXT("ntdll.dll")),
        "RtlQueryRegistryValuesEx"
        );

#else

    RtlInitUnicodeString(&FunctionName, L"RtlQueryRegistryValuesEx");

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable: 4055)
#endif

    queryFn  = (QUERYFN*)MmGetSystemRoutineAddress(&FunctionName);

#ifndef __GNUC__
#pragma warning(pop)
#endif

#endif

    if (queryFn == NULL)
	{
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
    if (NT_SUCCESS(status))
	{
        if (verboseValue)
		{
            FxDriverGlobals->FxVerboseOn = TRUE;
        }
        else
		{
            FxDriverGlobals->FxVerboseOn = FALSE;
        }

        if (allocateFailValue != (ULONG) -1)
		{
            FxDriverGlobals->WdfVerifierAllocateFailCount = (LONG) allocateFailValue;
        }
        else
		{
            FxDriverGlobals->WdfVerifierAllocateFailCount = -1;
        }

        //
        // Verifier settings are all or nothing.  We currently do not support
        // turning on individual sub-verifiers.
        //
        FxDriverGlobals->SetVerifierState(verifierOnValue ? TRUE : FALSE);

        if (FxDriverGlobals->FxVerifierOn)
		{
            FxDriverGlobalsInitializeDebugExtension(FxDriverGlobals, hWdf.m_Key);
        }

        //
        // Update FxVerifyDownLevel independent of FxVerifyOn because for UMDF
        // verifer is always on so it does not consume FxVerifyOn value
        //
        if (verifyDownlevelValue)
		{
            FxDriverGlobals->FxVerifyDownlevel = TRUE;
        }
        else
		{
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

        if (FxDriverGlobals->FxVerifyOn)
		{
            FxDriverGlobals->Public.DriverFlags |= WdfVerifyOn;
        }

        FxOverrideDefaultVerifierSettings(hWdf.m_Key,
                                          L"DbgBreakOnError",
                                          &FxDriverGlobals->FxVerifierDbgBreakOnError);

        FxOverrideDefaultVerifierSettings(hWdf.m_Key,
                                          L"DbgBreakOnDeviceStateError",
                                          &FxDriverGlobals->FxVerifierDbgBreakOnDeviceStateError);

        FxDriverGlobals->FxForceLogsInMiniDump =
                            (forceLogsInMiniDump) ? TRUE : FALSE;

        FxDriverGlobals->FxTrackDriverForMiniDumpLog =
                            (trackDriverForMiniDumpLog) ? TRUE : FALSE;

        FxDriverGlobals->FxRequestParentOptimizationOn =
                            (requestParentOptimizationOn) ? TRUE : FALSE;
    }

    return;
}

_Must_inspect_result_
FxObjectDebugInfo*
FxVerifyAllocateDebugInfo(
    _In_ LPCWSTR HandleNameList,
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    FxObjectDebugInfo* pInfo;
    LPCWCH pCur;
    ULONG i;
    BOOLEAN all;

    //
    // check to see if the multi sz is empty
    //
    if (*HandleNameList == NULL)
	{
		return NULL;
    }
        
    ULONG length = sizeof(FxObjectDebugInfo) * FxObjectsInfoCount;

    //
    // Freed with ExFreePool in FxFreeDriverGlobals.  Must be non paged because
    // objects can be allocated at IRQL > PASSIVE_LEVEL.
    //
    pInfo = (FxObjectDebugInfo*) MxMemory::MxAllocatePoolWithTag(NonPagedPool,
                                                        length,
                                                        FxDriverGlobals->Tag);
    if (pInfo == NULL)
	{
        //return STATUS_MEMORY_NOT_ALLOCATED;
		return NULL;
    }
        
    RtlZeroMemory(pInfo, length);    

    ASSERT(pInfo != NULL);

    all = *HandleNameList == L'*' ? TRUE : FALSE;

    //
    // Iterate over all of the objects in our internal array.  We iterate over
    // this array instead of the multi sz list b/c this way we only convert
    // each ANSI string to UNICODE once.
    //
    for (i = 0; i < FxObjectsInfoCount; i++)
	{
        UNICODE_STRING objectName;
        WCHAR ubuffer[40];
        STRING string;

        pInfo[i].ObjectType = FxObjectsInfo[i].ObjectType;

        //
        // If this is an internal object, just continue past it
        //
        if (FxObjectsInfo[i].HandleName == NULL)
		{
            continue;
        }

        //
        // Short circuit if we are wildcarding
        //
        if (all)
		{
            pInfo[i].u.DebugFlags |= DebugFlag;
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
                                                     FALSE)))
		{
            continue;
        }

        //
        // Now iterate over the multi sz list, comparing handle strings in the
        // list against the current object name.
        //
        pCur = HandleNameList;

        while (*pCur != UNICODE_NULL)
		{
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
            if (RtlCompareUnicodeString(&handleName, &objectName, TRUE) == 0)
			{
                pInfo[i].u.DebugFlags |= DebugFlag;
                break;
            }
        }
    }

	return pInfo;
}

_Must_inspect_result_
FxObjectDebugInfo*
FxVerifierGetObjectDebugInfo(
    __in HANDLE Key,
    __in PFX_DRIVER_GLOBALS  FxDriverGlobals
    )

/*++

Routine Description:
    Attempts to open values under the passed in key and create an array of
    FxObjectDebugInfo.

Arguments:
    Key - Registry key to query the value for

Return Value:
    NULL or a pointer which should be freed by the caller using ExFreePool

--*/

{
    FxObjectDebugInfo *pInfo = NULL;
    NTSTATUS status;
	PVOID dataBuffer = NULL;
	ULONG length, type;    
	DECLARE_CONST_UNICODE_STRING(valueName, L"TrackHandles");
    
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
    if (status != STATUS_BUFFER_OVERFLOW && status != STATUS_BUFFER_TOO_SMALL)
	{
        goto exit;
    }

	//
    // Pool can be paged b/c we are running at PASSIVE_LEVEL and we are going
    // to free it at the end of this function.
    //
    dataBuffer = MxMemory::MxAllocatePoolWithTag(PagedPool, length, FxDriverGlobals->Tag);
    if (dataBuffer == NULL)
	{
        status = STATUS_MEMORY_NOT_ALLOCATED;
        goto exit;
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

	if (NT_SUCCESS(status))
	{
        //
        // Verify that the data from the registry is a valid multi-sz string.
        //
        status = FxRegKey::_VerifyMultiSzString(FxDriverGlobals,
                                                &valueName,
                                                (PWCHAR) dataBuffer,
                                                length);
    }

	if (NT_SUCCESS(status))
	{
#ifndef __GNUC__
		#pragma prefast(push)
		#pragma prefast(suppress:__WARNING_PRECONDITION_NULLTERMINATION_VIOLATION, "FxRegKey::_VerifyMultiSzString makes sure the string is NULL-terminated")
#endif
		pInfo = FxVerifyAllocateDebugInfo((LPCWSTR) dataBuffer, FxDriverGlobals);
#ifndef __GNUC__
		#pragma prefast(pop)
#endif
    }

exit:
    if (NULL != dataBuffer)
	{
        MxMemory::MxFreePool(dataBuffer);
    }
    
    return pInfo;
}

VOID
FxOverrideDefaultVerifierSettings(
    __in    HANDLE Key,
    __in    PCWSTR Name,
    //__in    LPWSTR Name,
    __out   PBOOLEAN OverrideValue
    )
{
    UNICODE_STRING valueName;
    ULONG value = 0;

    RtlInitUnicodeString(&valueName, Name);

    if (NT_SUCCESS(FxRegKey::_QueryULong(Key,
                                         (PCUNICODE_STRING)&valueName,
                                         &value)))
	{
        if (value)
		{
            *OverrideValue = TRUE;
        }
		else
		{
            *OverrideValue = FALSE;
        }
    }

}

PCSTR
FxObjectTypeToHandleName(
    __in WDFTYPE ObjectType
    )
{
    ULONG i;

    for (i = 0; i < FxObjectsInfoCount; i++)
    {
        if (ObjectType == FxObjectsInfo[i].ObjectType)
        {
            return FxObjectsInfo[i].HandleName;
        }
        else if (ObjectType > FxObjectsInfo[i].ObjectType)
        {
            continue;
        }

        return NULL;
    }

    return NULL;
}

_Must_inspect_result_
BOOLEAN
FxVerifyObjectTypeInTable(
    __in USHORT ObjectType
    )
{
    ULONG i;

    for (i = 0; i < FxObjectsInfoCount; i++)
    {
        if (ObjectType == FxObjectsInfo[i].ObjectType)
        {
            return TRUE;
        }
        else if (ObjectType > FxObjectsInfo[i].ObjectType)
        {
            continue;
        }

        return FALSE;
    }

    return FALSE;
}

_Must_inspect_result_
BOOLEAN
FxVerifierIsDebugInfoFlagSetForType(
    _In_ FxObjectDebugInfo* DebugInfo,
    _In_ WDFTYPE ObjectType,
    _In_ FxObjectDebugInfoFlags Flag
    )

/*++

Routine Description:
    For a given object type and flag, returns to the caller if the flag is set.

Arguments:
    DebugInfo - array of object debug info to search through

    ObjectType - the type of the object to check

    FxObjectDebugInfoFlags - Flag to check for

Return Value:
    TRUE if objects of the given type had its flag set.

--*/

{
    ULONG i;

    //
    // Array size of DebugInfo is the same size as FxObjectsInfo
    //
    for (i = 0; i < FxObjectsInfoCount; i++)
    {
        if (ObjectType == DebugInfo[i].ObjectType)
        {
            return FLAG_TO_BOOL(DebugInfo[i].u.DebugFlags,
                                Flag);
        }
        else if (ObjectType > FxObjectsInfo[i].ObjectType)
        {
            continue;
        }

        return FALSE;
    }

    return FALSE;
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

    do
    {
        status = Event->WaitFor(Executive,
                        KernelMode,
                        FALSE, // Non alertable
                        timeOut.QuadPart ? &timeOut : NULL);

        if (status == STATUS_TIMEOUT)
        {
            DbgPrint("Thread 0x%p is %s 0x%p\n",
		      Mx::MxGetCurrentThread(),
                      ReasonForWaiting,
                      Handle);

            if ((WaitSignalFlags & WaitSignalAlwaysBreak) ||
                ((WaitSignalFlags & WaitSignalBreakUnderVerifier) &&
                 FxVerifierDbgBreakOnError) ||
                ((WaitSignalFlags & WaitSignalBreakUnderDebugger) &&
                 IsDebuggerAttached()))
            {
                DbgBreakPoint();
            }
        }
        else
        {
            ASSERT(NT_SUCCESS(status));
            break;
        }
    } while(TRUE);
}

} //extern C

_Must_inspect_result_
BOOLEAN
FX_DRIVER_GLOBALS::IsDebuggerAttached(
    VOID
    )
{
    return (FALSE == KdRefreshDebuggerNotPresent());
}
