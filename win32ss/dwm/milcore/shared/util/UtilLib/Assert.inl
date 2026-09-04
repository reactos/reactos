// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        Contains implementation for failed Asserts and RIPs.  On
//                   checked builds logic is provided to disable up to N unique
//                   stacks.
//
//  Notes:           This source is currently shared with DebugDll which has
//                   special requirements and can't directly link to UtilLib.
//
//                   Portions of code are ported from RtlAssert and
//                   DbgExAssertThreadDisable.
//

#include <dpfilter.h>

// copied from ntexapi.h, which cannot be included because it increases build errors.

typedef struct _SYSTEM_KERNEL_DEBUGGER_INFORMATION {
    BOOLEAN KernelDebuggerEnabled;
    BOOLEAN KernelDebuggerNotPresent;
} SYSTEM_KERNEL_DEBUGGER_INFORMATION, *PSYSTEM_KERNEL_DEBUGGER_INFORMATION;

typedef enum _SYSTEM_INFORMATION_CLASS {
    SystemBasicInformation,
    SystemProcessorInformation,             // obsolete...delete
    SystemPerformanceInformation,
    SystemTimeOfDayInformation,
    SystemPathInformation,
    SystemProcessInformation,
    SystemCallCountInformation,
    SystemDeviceInformation,
    SystemProcessorPerformanceInformation,
    SystemFlagsInformation,
    SystemCallTimeInformation,
    SystemModuleInformation,
    SystemLocksInformation,
    SystemStackTraceInformation,
    SystemPagedPoolInformation,
    SystemNonPagedPoolInformation,
    SystemHandleInformation,
    SystemObjectInformation,
    SystemPageFileInformation,
    SystemVdmInstemulInformation,
    SystemVdmBopInformation,
    SystemFileCacheInformation,
    SystemPoolTagInformation,
    SystemInterruptInformation,
    SystemDpcBehaviorInformation,
    SystemFullMemoryInformation,
    SystemLoadGdiDriverInformation,
    SystemUnloadGdiDriverInformation,
    SystemTimeAdjustmentInformation,
    SystemSummaryMemoryInformation,
    SystemMirrorMemoryInformation,
    SystemPerformanceTraceInformation,
    SystemObsolete0,
    SystemExceptionInformation,
    SystemCrashDumpStateInformation,
    SystemKernelDebuggerInformation,
    SystemContextSwitchInformation,
    SystemRegistryQuotaInformation,
    SystemExtendServiceTableInformation,
    SystemPrioritySeperation,
    SystemVerifierAddDriverInformation,
    SystemVerifierRemoveDriverInformation,
    SystemProcessorIdleInformation,
    SystemLegacyDriverInformation,
    SystemCurrentTimeZoneInformation,
    SystemLookasideInformation,
    SystemTimeSlipNotification,
    SystemSessionCreate,
    SystemSessionDetach,
    SystemSessionInformation,
    SystemRangeStartInformation,
    SystemVerifierInformation,
    SystemVerifierThunkExtend,
    SystemSessionProcessInformation,
    SystemLoadGdiDriverInSystemSpace,
    SystemNumaProcessorMap,
    SystemPrefetcherInformation,
    SystemExtendedProcessInformation,
    SystemRecommendedSharedDataAlignment,
    SystemComPlusPackage,
    SystemNumaAvailableMemory,
    SystemProcessorPowerInformation,
    SystemEmulationBasicInformation,
    SystemEmulationProcessorInformation,
    SystemExtendedHandleInformation,
    SystemLostDelayedWriteInformation,
    SystemBigPoolInformation,
    SystemSessionPoolTagInformation,
    SystemSessionMappedViewInformation,
    SystemHotpatchInformation,
    SystemObjectSecurityMode,
    SystemWatchdogTimerHandler,
    SystemWatchdogTimerInformation,
    SystemLogicalProcessorInformation,
    SystemWow64SharedInformationObsolete,
    SystemRegisterFirmwareTableInformationHandler,
    SystemFirmwareTableInformation,
    SystemModuleInformationEx,
    SystemVerifierTriageInformation,
    SystemSuperfetchInformation,
    SystemMemoryListInformation,
    SystemFileCacheInformationEx,
    SystemThreadPriorityClientIdInformation,
    SystemProcessorIdleCycleTimeInformation,
    SystemVerifierCancellationInformation,
    SystemProcessorPowerInformationEx,
    SystemRefTraceInformation,
    SystemSpecialPoolInformation,
    SystemProcessIdInformation,
    SystemErrorPortInformation,
    SystemBootEnvironmentInformation,
    SystemHypervisorInformation,
    SystemVerifierInformationEx,
    SystemTimeZoneInformation,
    SystemImageFileExecutionOptionsInformation,
    SystemCoverageInformation,
    SystemPrefetchPatchInformation,
    SystemVerifierFaultsInformation,
    SystemSystemPartitionInformation,
    SystemSystemDiskInformation,
    SystemProcessorPerformanceDistribution,
    SystemNumaProximityNodeInformation,
    SystemDynamicTimeZoneInformation,
    SystemCodeIntegrityInformation,
    SystemProcessorMicrocodeUpdateInformation,
    SystemProcessorBrandString,
    SystemVirtualAddressInformation,
    SystemLogicalProcessorAndGroupInformation,
    SystemProcessorCycleTimeInformation,
    SystemStoreInformation,
    SystemRegistryAppendString,
    SystemAitSamplingValue,
    SystemVhdBootInformation,
    SystemCpuQuotaInformation,
    SystemSpare0,
    SystemSpare1,
    SystemLowPriorityIoInformation,
    SystemTpmBootEntropyInformation,
    SystemVerifierCountersInformation,
    SystemPagedPoolInformationEx,
    SystemSystemPtesInformationEx,
    SystemNodeDistanceInformation,
    SystemAcpiAuditInformation,
    SystemBasicPerformanceInformation,
    SystemSessionBigPoolInformation,
    SystemBootGraphicsInformation,
    SystemScrubPhysicalMemoryInformation,
    SystemBadPageInformation,
    MaxSystemInfoClass  // MaxSystemInfoClass should always be the last enum
} SYSTEM_INFORMATION_CLASS;

#define NTSTATUS LONG

#ifdef __cplusplus
extern "C" {
#endif

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformation (
    __in SYSTEM_INFORMATION_CLASS SystemInformationClass,
    __out_bcount_part_opt(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    __in ULONG SystemInformationLength,
    __out_opt PULONG ReturnLength
    );

// copied from ntstatus.h

#define NT_SUCCESS(Status) ((NTSTATUS)(Status) >= 0)

#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001L)

// copied from wdm.h

__analysis_noreturn
VOID
NTAPI
DbgBreakPoint(
    VOID
    );

NTSYSAPI
ULONG
__cdecl
DbgPrintEx (
    __in ULONG ComponentId,
    __in ULONG Level,
    __in_z __drv_formatString(printf) PCSTR Format,
    ...
    );

// copied from ntddk.h

NTSYSAPI
ULONG
NTAPI
DbgPrompt (
    __in_z PCCH Prompt,
    __out_bcount(Length) PCH Response,
    __in ULONG Length
    );

#ifdef __cplusplus
}
#endif

// Number of unique stack traces that may be disabled
#define MAX_DISABLED_UNIQUE_ASSERT_STACKS   100

// Depths of stack used to track disabled asserts
#define ASSERT_STACK_CAPTURE_DEPTH          3

// Minimum successful stack capture required to allow an assert to be disabled.
// This number should probably not go below 2 since AssertA may contribute to
// captured stack.
#define ASSERT_STACK_CAPTURE_DEPTH_MINIMUM  2


ULONG g_uDPFltrID = DPFLTR_DEFAULT_ID;


#if DBG


//+----------------------------------------------------------------------------
//
//  Class:     CDbgBookmarkStack
//
//  Synopsis:  This class can store _cMax stack captures of depth _cStackDepth
//             and later search for matches.
//
//-----------------------------------------------------------------------------

template <UINT _cMax, UINT _cStackDepth>
class CDbgBookmarkStack
{
protected:
    UINT m_cUsed;                               // used entries
    PVOID m_rgpvStack[_cMax][_cStackDepth];     // the array of stacks

public:

    enum {
        kStackDepth = _cStackDepth
    };

    //+------------------------------------------------------------------------
    //
    //  Member:    CDbgBookmarkStack
    //
    //  Synopsis:  constructor
    //
    //-------------------------------------------------------------------------

    CDbgBookmarkStack()
    {
        m_cUsed = 0;
    }

    //+------------------------------------------------------------------------
    //
    // Function:    AreMarksAvailable
    //
    // Synopsis:    Returns true if there is bookmark space left.
    //
    //-------------------------------------------------------------------------
    bool
    AreMarksAvailable(
        )
    {
        return (m_cUsed < _cMax);
    }

    //+------------------------------------------------------------------------
    //
    // Function:    FindMark
    //
    // Synopsis:    Returns true if there is a bookmark at stack by
    //              searching linearly through all used bookmarks.
    //
    //-------------------------------------------------------------------------
    bool
    FindMark(
        __in_bcount(sizeof(rgStack)) const PVOID (&rgStack)[_cStackDepth],
        __out_ecount_opt(1) UINT *pMarkID = NULL
        )
    {
        for (UINT i = 0; i < m_cUsed; i++)
        {
            if (RtlEqualMemory(m_rgpvStack[i], rgStack, sizeof(rgStack)))
            {
                if (pMarkID)
                {
                    *pMarkID = i;
                }
                return true;
            }
        }

        return false;
    }

    //+------------------------------------------------------------------------
    //
    // Function:    Mark
    //
    // Synopsis:    Set bookmark for stack.  Mark ID is returned.
    //
    //-------------------------------------------------------------------------
    UINT
    Mark(
        __in_bcount(sizeof(rgStack)) const PVOID (&rgStack)[_cStackDepth]
        )
    {
        Assert(!FindMark(rgStack));
        Assert(m_cUsed < _cMax);

        UINT uMarkLocation = m_cUsed++;
        RtlCopyMemory(m_rgpvStack[uMarkLocation], rgStack, sizeof(rgStack));

        return uMarkLocation;
    }

protected:

};

//
// Array of disabled asserts.
//
// Unique assert stacks (of depth ASSERT_STACK_CAPTURE_DEPTH) that have been
// marked to be ignored (disabled) are tracked in this class.
//
// You can't disable more than MAX_UNIQUE_BOOKMARKED_ASSERT_STACKS asserts.
//
// Warning: This global/class is not multi-thread safe in anyway, but as debug
//          instrumentation we are not worried about that.
//
CDbgBookmarkStack<MAX_DISABLED_UNIQUE_ASSERT_STACKS, ASSERT_STACK_CAPTURE_DEPTH> g_rgbmkDisabledAsserts;

#endif DBG


//+----------------------------------------------------------------------------
//
//  Function:  IsKernelDebuggerEnabled
//
//  Synopsis:  Query system to see if a kernel debugger is enabled (independent
//             of being attached.)
//

BOOL
IsKernelDebuggerEnabled()
{
    static bool fSuccessfullyQueried = false;
    // Default reporting that kernel debugger is not enabled.
    static SYSTEM_KERNEL_DEBUGGER_INFORMATION kdInfo = { FALSE };

    // Kernel debugger enabled status isn't expected to change once system
    // boots; so once we successfully query it don't bother querying again.
    if (!fSuccessfullyQueried)
    {
        if (NT_SUCCESS(NtQuerySystemInformation(
                SystemKernelDebuggerInformation,
                &kdInfo,
                sizeof(kdInfo),
                NULL)))
        {
            fSuccessfullyQueried = true;
        }
        else
        {
            // Assume kernel debugger is not enable - leave var as FALSE
        }
    }

    return kdInfo.KernelDebuggerEnabled;
}


//+----------------------------------------------------------------------------
//
//  Function:  IsKernelDebuggerPresent
//
//  Synopsis:  Query system to see if a kernel debugger is present.
//

BOOL
IsKernelDebuggerPresent()
{
    static SYSTEM_KERNEL_DEBUGGER_INFORMATION kdInfo = { TRUE, TRUE };

    // Once we find that kernel debugger is enabled and present, behave like
    // it is always present (even if it has been detached) and don't bother
    // querying again.
    if (   kdInfo.KernelDebuggerEnabled
        && kdInfo.KernelDebuggerNotPresent)
    {
        if (!NT_SUCCESS(NtQuerySystemInformation(
                SystemKernelDebuggerInformation,
                &kdInfo,
                sizeof(kdInfo),
                NULL)))
        {
            // Force to default value on failure which will trigger requeries.
            kdInfo.KernelDebuggerEnabled = TRUE;
            kdInfo.KernelDebuggerNotPresent = TRUE;
        }
    }

    // Make sure to check KernelDebuggerEnabled, because if it is not TRUE then
    // KernelDebuggerNotPresent will be FALSE, even though no kernel debugger
    // is actually present. 
    return kdInfo.KernelDebuggerEnabled && !kdInfo.KernelDebuggerNotPresent;
}


//+----------------------------------------------------------------------------
//
//  Function:  AssertA
//
//  Synopsis:  Convert string to unicode and pass to AssertW
//

VOID
AssertA(
    __in_opt PCSTR Message,
    __in_opt PCWSTR FailedAssertion,
    __in PCWSTR Function,
    __in PCWSTR FileName,
    ULONG LineNumber
    )
{
    WCHAR wszBuffer[1024] = L"";

    IGNORE_HR(StringCbPrintfW(
        wszBuffer, sizeof(wszBuffer), L"%hs",
        Message
        ));

    AssertW(
        wszBuffer,
        FailedAssertion,
        Function,
        FileName,
        LineNumber
        );
}



//+----------------------------------------------------------------------------
//
//  Function:  AssertW
//
//  Synopsis:  Handle notifying system of an assertion failure.
//
//             Unless this assertion has been disabled.  Basic information
//             about the failure is display and then the user is prompted for
//             how to handle the failure.
//

VOID
AssertW(
    __in_opt PCWSTR Message,
    __in_opt PCWSTR FailedAssertion,
    __in PCWSTR Function,
    __in PCWSTR FileName,
    ULONG LineNumber
    )
{
    // NOTE: This function has a variety of exit points, but none at the end.

#if DBG
    //
    // In debug builds enable the ability to disable future hits of assert.
    //

    bool fCanDisable = false;
    PVOID rgStackCapture[g_rgbmkDisabledAsserts.kStackDepth];

    // Set rgStackCapture to a recognizable invalid value if no frames are captured.
    RtlFillMemory(rgStackCapture, sizeof(rgStackCapture), 0xE0);

    if (ASSERT_STACK_CAPTURE_DEPTH_MINIMUM <=
            RtlCaptureStackBackTrace(
                1,                         // Skip this frame
                ARRAYSIZE(rgStackCapture), // Max # of frames 
                rgStackCapture,            // Place capture here
                NULL)                      // Ignored optional param
       )
    {
        //
        // Check if this stack is disabled.
        //
        if (g_rgbmkDisabledAsserts.FindMark(rgStackCapture))
        {
            // Exit without doing anything.
            return;
        }

        // We can disable this assertion failure if there is space available
        // since we have successfully captured stack.
        fCanDisable = g_rgbmkDisabledAsserts.AreMarksAvailable();
    }
#endif DBG

    PSTR szKDPrompt = 
#if DBG
        fCanDisable ?
        "Break, Go (continue), Ignore all, terminate Process, or terminate Thread (bgipt)? " :
#endif DBG
        "Break, Go (continue), terminate Process, or terminate Thread (bgpt)? ";

    //
    // Use goto label loop construct as an alternative to RtlAssert which uses
    // an unreachable assert to protect against an accidental break.
    //
    Prompt:
    {
        const BOOL fKDPresent = IsKernelDebuggerPresent();

        //
        // Set default response.
        //
        // When KD is present require a valid response.
        //
        // When KD is NOT present and
        //  - no debugger is present require a valid reponse.  This basicially
        //    forces a redisplay of the messages once a debugger is attached
        //    and the operator hits 'g'.
        //  - debugger is present assume the messages are displayed and the
        //    operator hitting 'g' means continue as message below suggests.
        //

        char Response[ 2 ] = { '?', 0 };

        if (!fKDPresent && IsDebuggerPresent())
        {
            Response[0] = 'g';
        }

        //
        // Show assertion failure message
        //

        DbgPrintEx(
            g_uDPFltrID,
            DPFLTR_ERROR_LEVEL,
            "\n*** Assertion failed: %ls%ls%ls\n***   %s%ls%sSource: `%ls:%ld`\n\n",
            Message ? Message : L"",
            (Message && FailedAssertion) ? L"\n***  " : L"",
            FailedAssertion ? FailedAssertion : L"",
            Function ? "Function: " : "",
            Function ?  Function    : L"",
            Function ? ", "         : "",
            FileName,
            LineNumber
            );

        //
        // Show assertion failure prompt
        //

        if (fKDPresent)
        {
            DbgPrompt(
                szKDPrompt,
                Response,
                sizeof(Response)
                );
        }
        else
        {
            DbgPrintEx(
                g_uDPFltrID,
                DPFLTR_ERROR_LEVEL,
                "(No kernel debugger is present.) Respond with:\n"
                "  g                    -- Go (continue)\n"
#if DBG
                "  eb 0x%p 'i';g  -- %s\n"
#endif DBG
                "  eb 0x%p 'p';g  -- terminate Process\n"
                "  eb 0x%p 't';g  -- terminate Thread\n"
                " or regular debugging.\n",
#if DBG
                &Response[0],
                fCanDisable ?
                "Ignore all future hits" :
                "<not available>",
#endif DBG
                &Response[0],
                &Response[0]
                );

            DbgBreakPoint();
        }

        //
        // Interpret response
        //

        switch (Response[0])
        {
            // Break
        case 'B':
        case 'b':
            DbgBreakPoint();
            return;

            // Go (Continue)
        case 'G':
        case 'g':
            return;

            // Ignore all - disable this stack
        case 'I':
        case 'i':
#if DBG
            if (fCanDisable)
            {
                UINT uMarkID = g_rgbmkDisabledAsserts.Mark(rgStackCapture);

                DbgPrintEx(g_uDPFltrID, DPFLTR_ERROR_LEVEL,
                           "Future hits will be ignored.  (New bookmark ID is %u.)\n",
                           uMarkID);
                return;
            }

            DbgPrintEx(g_uDPFltrID, DPFLTR_ERROR_LEVEL,
                       "'i' is not available.  Stack trace is insufficient or mark limit reached.\n"
                       );
#else
            DbgPrintEx(g_uDPFltrID, DPFLTR_ERROR_LEVEL,
                       "'i' is only supported with debug builds.\n"
                       );
#endif DBG
            goto Prompt;

            // terminate Process
        case 'P':
        case 'p':
            TerminateProcess( GetCurrentProcess(), static_cast<DWORD>(STATUS_UNSUCCESSFUL) );
            break;

            // terminate Thread
        case 'T':
        case 't':
            TerminateThread( GetCurrentThread(), static_cast<DWORD>(STATUS_UNSUCCESSFUL) );
            break;
        }

        // Loop until response is recognized
        DbgPrintEx(g_uDPFltrID, DPFLTR_ERROR_LEVEL, "Unrecognized response.\n");
    } goto Prompt;
}




