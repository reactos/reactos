/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Info Classes for the Process Manager
 * COPYRIGHT:   Copyright Alex Ionescu <alex.ionescu@reactos.org>
 *              Copyright Thomas Weidenmueller <w3seek@reactos.org>
 *              Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "icif.h"

//
// Process Information Classes
//
static const INFORMATION_CLASS_INFO PsProcessInfoClass[] =
{
    /* ProcessBasicInformation */
    IQS_SAME
    (
        PROCESS_BASIC_INFORMATION,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessQuotaLimits */
    IQS_SAME
    (
        QUOTA_LIMITS,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessIoCounters */
    IQS_SAME
    (
        IO_COUNTERS,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessVmCounters */
    IQS_SAME
    (
        VM_COUNTERS,
        ULONG,
        ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE
    ),

    /* ProcessTimes */
    IQS_SAME
    (
        KERNEL_USER_TIMES,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessBasePriority */
    IQS_SAME
    (
        KPRIORITY,
        ULONG,
        ICIF_SET
    ),

    /* ProcessRaisePriority */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_SET
    ),

    /* ProcessDebugPort */
    IQS_SAME
    (
        HANDLE,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessExceptionPort */
    IQS_SAME
    (
        HANDLE,
        ULONG,
        ICIF_SET
    ),

    /* ProcessAccessToken */
    IQS_SAME
    (
        PROCESS_ACCESS_TOKEN,
        ULONG,
        ICIF_SET
    ),

    /* ProcessLdtInformation */
    IQS_SAME
    (
        PROCESS_LDT_INFORMATION,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessLdtSize */
    IQS_SAME
    (
        PROCESS_LDT_SIZE,
        ULONG,
        ICIF_SET
    ),

    /* ProcessDefaultHardErrorMode */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessIoPortHandlers */
    IQS_SAME
    (
        UCHAR,
        ULONG,
        ICIF_SET
    ),

    /* ProcessPooledUsageAndLimits */
    IQS_SAME
    (
        POOLED_USAGE_AND_LIMITS,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessWorkingSetWatch */
    IQS_SAME
    (
        PROCESS_WS_WATCH_INFORMATION,
        ULONG,
        ICIF_QUERY | ICIF_SET | ICIF_SET_SIZE_VARIABLE
    ),

    /* ProcessUserModeIOPL */
    IQS_SAME
    (
        UCHAR,
        ULONG,
        ICIF_SET
    ),

    /* ProcessEnableAlignmentFaultFixup */
    IQS
    (
        BOOLEAN,
        CHAR,
        BOOLEAN,
        CHAR,
        ICIF_SET
    ),

    /* ProcessPriorityClass */
    IQS
    (
        PROCESS_PRIORITY_CLASS,
        ULONG,
        PROCESS_PRIORITY_CLASS,
        CHAR,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessWx86Information */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessHandleCount */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessAffinityMask */
    IQS_SAME
    (
        KAFFINITY,
        ULONG,
        ICIF_SET
    ),

    /* ProcessPriorityBoost */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessDeviceMap */
    IQS
    (
        RTL_FIELD_TYPE(PROCESS_DEVICEMAP_INFORMATION, Query),
        ULONG,
        RTL_FIELD_TYPE(PROCESS_DEVICEMAP_INFORMATION, Set),
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessSessionInformation */
    IQS_SAME
    (
        PROCESS_SESSION_INFORMATION,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessForegroundInformation */
    IQS
    (
        CHAR,
        CHAR,
        BOOLEAN,
        CHAR,
        ICIF_SET
    ),

    /* ProcessWow64Information */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessImageFileName */
    IQS_SAME
    (
        UNICODE_STRING,
        ULONG,
        ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE
    ),

    /* ProcessLUIDDeviceMapsEnabled */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessBreakOnTermination */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessDebugObjectHandle */
    IQS_SAME
    (
        HANDLE,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessDebugFlags */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessHandleTracing */
    IQS
    (
        PROCESS_HANDLE_TRACING_QUERY,
        ULONG,
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessIoPriority */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessExecuteFlags */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessTlsInformation */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessCookie */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessImageInformation */
    IQS_SAME
    (
        SECTION_IMAGE_INFORMATION,
        ULONG,
        ICIF_QUERY
    ),

    /* ProcessCycleTime */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessPagePriority */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessInstrumentationCallback */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessThreadStackAllocation */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessWorkingSetWatchEx */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessImageFileNameWin32 */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessImageFileMapping */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessAffinityUpdateMode */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ProcessMemoryAllocationMode */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),
};

//
// Thread Information Classes
//
static const INFORMATION_CLASS_INFO PsThreadInfoClass[] =
{
    /* ThreadBasicInformation */
    IQS_SAME
    (
        THREAD_BASIC_INFORMATION,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadTimes */
    IQS_SAME
    (
        KERNEL_USER_TIMES,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadPriority */
    IQS_SAME
    (
        KPRIORITY,
        ULONG,
        ICIF_SET
    ),

    /* ThreadBasePriority */
    IQS_SAME
    (
        LONG,
        ULONG,
        ICIF_SET
    ),

    /* ThreadAffinityMask */
    IQS_SAME
    (
        KAFFINITY,
        ULONG,
        ICIF_SET
    ),

    /* ThreadImpersonationToken */
    IQS_SAME
    (
        HANDLE,
        ULONG,
        ICIF_SET
    ),

    /* ThreadDescriptorTableEntry is only implemented in x86 as well as the descriptor entry */
    #if defined(_X86_)
        /* ThreadDescriptorTableEntry */
        IQS_SAME
        (
            DESCRIPTOR_TABLE_ENTRY,
            ULONG,
            ICIF_QUERY
        ),
    #else
        IQS_NONE,
    #endif

    /* ThreadEnableAlignmentFaultFixup */
    IQS
    (
        CHAR,
        CHAR,
        BOOLEAN,
        UCHAR,
        ICIF_SET
    ),

    /* ThreadEventPair_Reusable */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ThreadQuerySetWin32StartAddress */
    IQS
    (
        PVOID,
        ULONG,
        ULONG_PTR,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ThreadZeroTlsCell */
    IQS_SAME
    (
        ULONG_PTR,
        ULONG,
        ICIF_SET
    ),

    /* ThreadPerformanceCount */
    IQS_SAME
    (
        LARGE_INTEGER,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadAmILastThread */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadIdealProcessor */
    IQS_SAME
    (
        ULONG_PTR,
        ULONG,
        ICIF_SET
    ),

    /* ThreadPriorityBoost */
    IQS
    (
        ULONG,
        ULONG,
        ULONG_PTR,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ThreadSetTlsArrayAddress */
    IQS_SAME
    (
        PVOID,
        ULONG,
        ICIF_SET | ICIF_SIZE_VARIABLE
    ),

    /* ThreadIsIoPending */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadHideFromDebugger */
    IQS_SAME
    (
        CHAR,
        ULONG,
        ICIF_SET | ICIF_SET_SIZE_VARIABLE
    ),

    /* ThreadBreakOnTermination */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ThreadSwitchLegacyState */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_SET
    ),

    /* ThreadIsTerminated */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadLastSystemCall */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ThreadIoPriority */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ThreadCycleTime */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ThreadPagePriority */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ThreadActualBasePriority */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ThreadTebInformation */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),

    /* ThreadCSwitchMon */
    IQS_SAME
    (
        CHAR,
        CHAR,
        ICIF_NONE
    ),
};
