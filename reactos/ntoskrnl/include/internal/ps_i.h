/*
* PROJECT:         ReactOS Kernel
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            ntoskrnl/include/ps_i.h
* PURPOSE:         Info Classes for the Process Manager
* PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
*                  Thomas Weidenmueller (w3seek@reactos.org)
*/

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
        ICIF_QUERY
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
        UCHAR,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessLdtSize */
    IQS_SAME
    (
        UCHAR,
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
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessUserModeIOPL */
    IQS_SAME
    (
        UCHAR,
        ULONG,
        ICIF_SET
    ),

    /* ProcessEnableAlignmentFaultFixup */
    IQS_SAME
    (
        BOOLEAN,
        ULONG,
        ICIF_SET
    ),

    /* ProcessPriorityClass */
    IQS_SAME
    (
        PROCESS_PRIORITY_CLASS,
        USHORT,
        ICIF_QUERY | ICIF_SET
    ),

    /* ProcessWx86Information */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
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
        ((PROCESS_DEVICEMAP_INFORMATION*)0)->Query,
        ((PROCESS_DEVICEMAP_INFORMATION*)0)->Set,
        ULONG,
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
    IQS_SAME
    (
        BOOLEAN,
        ULONG,
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
        ICIF_QUERY | ICIF_SIZE_VARIABLE
    ),

    /* ProcessLUIDDeviceMapsEnabled */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessBreakOnTermination */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessDebugObjectHandle */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessDebugFlags */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessHandleTracing */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessIoPriority */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessExecuteFlags */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessTlsInformation */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessCookie */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessImageInformation */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessCycleTime */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessPagePriority */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
    ),

    /* ProcessInstrumentationCallback */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        0
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
        ICIF_QUERY
    ),

    /* ThreadBasePriority */
    IQS_SAME
    (
        LONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadAffinityMask */
    IQS_SAME
    (
        KAFFINITY,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadImpersonationToken */
    IQS_SAME
    (
        HANDLE,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ThreadDescriptorTableEntry */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadEnableAlignmentFaultFixup */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadEventPair_Reusable */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadQuerySetWin32StartAddress */
    IQS_SAME
    (
        PVOID,
        ULONG,
        ICIF_QUERY | ICIF_SET
    ),

    /* ThreadZeroTlsCell */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
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
        BOOLEAN,
        BOOLEAN,
        ICIF_QUERY
    ),

    /* ThreadIdealProcessor */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadPriorityBoost */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadSetTlsArrayAddress */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
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
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),


    /* ThreadPriorityBoost */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadSetTlsArrayAddress */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
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
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadBreakOnTermination */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadSwitchLegacyState */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
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
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadIoPriority */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadCycleTime */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadPagePriority */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadActualBasePriority */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),

    /* ThreadTebInformation */
    IQS_SAME
    (
        ULONG,
        ULONG,
        ICIF_QUERY
    ),

    /* ThreadCSwitchMon */
    IQS_SAME
    (
        UCHAR,
        UCHAR,
        ICIF_QUERY
    ),
};
