/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmdata.c
 * PURPOSE:         Configuration Manager - Global Configuration Data
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* GLOBALS *******************************************************************/

ULONG DummyData;
ULONG CmNtGlobalFlag;
ULONG CmNtCSDVersion;

WCHAR CmDefaultLanguageId[12];
ULONG CmDefaultLanguageIdLength = sizeof(CmDefaultLanguageId);
ULONG CmDefaultLanguageIdType;

WCHAR CmInstallUILanguageId[12];
ULONG CmInstallUILanguageIdLength = sizeof(CmInstallUILanguageId);
ULONG CmInstallUILanguageIdType;

WCHAR CmSuiteBuffer[128];
ULONG CmSuiteBufferLength = sizeof(CmSuiteBuffer);
ULONG CmSuiteBufferType;

CMHIVE CmControlHive;

CM_SYSTEM_CONTROL_VECTOR CmControlVector[] =
{
    {
        L"Session Manager",
        L"ProtectionMode",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"ObjectSecurityMode",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"LUIDDeviceMapsDisabled",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"LSA",
        L"AuditBaseDirectories",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"LSA",
        L"AuditBaseObjects",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"LSA\\audit",
        L"ProcessAccessesToAudit",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"TimeZoneInformation",
        L"ActiveTimeBias",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"TimeZoneInformation",
        L"Bias",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"TimeZoneInformation",
        L"RealTimeIsUniversal",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"GlobalFlag",
        &CmNtGlobalFlag,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PagedPoolQuota",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"NonPagedPoolQuota",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PagingFileQuota",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"AllocationPreference",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"DynamicMemory",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"Mirroring",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"Mirroring",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"SystemViewSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"SessionViewSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"SessionImageSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"SessionPoolSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PoolUsageMaximum",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"MapAllocationFragment",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PagedPoolSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"NonPagedPoolSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"NonPagedPoolMaximumPercent",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"LargeSystemCache",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"LargeStackSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"SystemPages",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"LowMemoryThreshold",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"HighMemoryThreshold",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"DisablePagingExecutive",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"ModifiedPageLife",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"SecondLevelDataCache",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"ClearPageFileAtShutdown",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PoolTagSmallTableSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PoolTagBigTableSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PoolTag",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"PoolTagOverruns",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"SnapUnloads",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"ProtectNonPagedPool",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"TrackLockedPages",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"TrackPtes",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"VerifyDrivers",
        &DummyData,
        &DummyData,
        &DummyData
    },

    {
        L"Session Manager\\Memory Management",
        L"VerifyDriverLevel",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"VerifyMode",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"LargePageMinimum",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"EnforceWriteProtection",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"MakeLowMemory",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Memory Management",
        L"WriteWatch",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Executive",
        L"AdditionalCriticalWorkerThreads",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Executive",
        L"AdditionalDelayedWorkerThreads",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Executive",
        L"PriorityQuantumMatrix",
        &DummyData,
        &DummyData,
        NULL
    },

    {
        L"Session Manager\\Kernel",
        L"DpcQueueDepth",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Kernel",
        L"MinimumDpcRate",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Kernel",
        L"AdjustDpcThreshold",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Kernel",
        L"IdealDpcRate",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\I/O System",
        L"CountOperations",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\I/O System",
        L"LargeIrpStackLocations",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\I/O System",
        L"IoVerifierLevel",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"ResourceTimeoutCount",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"CriticalSectionTimeout",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"HeapSegmentReserve",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"HeapSegmentCommit",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"HeapDeCommitTotalFreeThreshold",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"HeapDeCommitFreeBlockThreshold",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"ProductOptions",
        L"ProductType",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Terminal Server",
        L"TSEnabled",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Terminal Server",
        L"TSAppCompat",
        &DummyData,
        NULL,
        NULL
    },


    {
        L"ProductOptions",
        L"ProductSuite",
        CmSuiteBuffer,
        &CmSuiteBufferLength,
        &CmSuiteBufferType
    },

    {
        L"Windows",
        L"CSDVersion",
        &CmNtCSDVersion,
        NULL,
        NULL
    },

    {
        L"Nls\\Language",
        L"Default",
        CmDefaultLanguageId,
        &CmDefaultLanguageIdLength,
        &CmDefaultLanguageIdType
    },

    {
        L"Nls\\Language",
        L"InstallLanguage",
        CmInstallUILanguageId,
        &CmInstallUILanguageIdLength,
        &CmInstallUILanguageIdType
    },

    {
        L"\0\0",
        L"RegistrySizeLimit",
        &DummyData,
        &DummyData,
        &DummyData
    },

    {
        L"Session Manager",
        L"ForceNpxEmulation",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"PowerPolicySimulate",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager\\Executive",
        L"MaxTimeSeparationBeforeCorrect",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Windows",
        L"ShutdownTime",
        &DummyData,
        &DummyData,
        NULL
    },

    {
        L"PriorityControl",
        L"Win32PrioritySeparation",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"EnableTimerWatchdog",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"Session Manager",
        L"Debugger Retries",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"WMI",
        L"MaxEventSize",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"WMI\\Trace",
        L"UsePerformanceClock",
        &DummyData,
        NULL,
        NULL
    },

    {
        L"WMI\\Trace",
        L"TraceAlignment",
        &DummyData,
        NULL,
        NULL
    },

    {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    }
};
