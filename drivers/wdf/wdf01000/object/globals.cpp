#include "common/fxglobals.h"
#include "common/fxpool.h"
#include "common/fxmacros.h"
#include "common/mxmemory.h"
#include "common/dbgtrace.h"

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

}