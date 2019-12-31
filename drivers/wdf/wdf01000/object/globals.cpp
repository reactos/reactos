#include "common/fxglobals.h"
#include "common/fxpool.h"

extern "C" {

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

//#if ((FX_CORE_MODE)==(FX_CORE_KERNEL_MODE))
    //
    // Cleanup for the driver usage tracker.
    //
    FxLibraryGlobals.DriverTracker.Uninitialize();

    //
    // Deregister from the global (library) bugcheck callbacks.
    //
    FxUninitializeBugCheckDriverInfo();
//#endif

    FxLibraryGlobals.FxDriverGlobalsListLock.Uninitialize();

	return;
}

_Must_inspect_result_
PWDF_DRIVER_GLOBALS
FxAllocateDriverGlobals(
    VOID
    )
{
    //PFX_DRIVER_GLOBALS  pFxDriverGlobals;
    //KIRQL               irql;
    //NTSTATUS            status;

	return NULL;
}

}