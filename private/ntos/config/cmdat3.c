/*++

Copyright (c) 1990, 1991  Microsoft Corporation


Module Name:

    cmdat3.c

Abstract:

    This module contains registry "static" data which we don't
    want pulled into the loader.

Author:

    Bryan Willman (bryanwi) 19-Oct-93


Environment:

    Kernel mode.

Revision History:

--*/

#include "cmp.h"

//
// ***** INIT *****
//

//
// Data for CmGetSystemControlValues
//
//
// ----- CmControlVector -----
//
#ifdef ALLOC_DATA_PRAGMA
#pragma data_seg("INIT")
#endif

//
//  Local examples
//
WCHAR   CmDefaultLanguageId[ 12 ] = { 0 };
ULONG   CmDefaultLanguageIdLength = sizeof( CmDefaultLanguageId );
ULONG   CmDefaultLanguageIdType = REG_NONE;

WCHAR   CmInstallUILanguageId[ 12 ] = { 0 };
ULONG   CmInstallUILanguageIdLength = sizeof( CmInstallUILanguageId );
ULONG   CmInstallUILanguageIdType = REG_NONE;
//
// suite data
//
WCHAR   CmSuiteBuffer[128];
ULONG   CmSuiteBufferLength = sizeof(CmSuiteBuffer);
ULONG   CmSuiteBufferType = REG_NONE;

//
// Verify driver list data
//
extern WCHAR   MmVerifyDriverBuffer[];
extern ULONG   MmVerifyDriverBufferLength;
extern ULONG   MmVerifyDriverBufferType;
extern ULONG   MmVerifyDriverLevel;
extern LOGICAL MmDontVerifyRandomDrivers;

extern ULONG ObpProtectionMode;
extern ULONG ObpAuditBaseDirectories;
extern ULONG ObpAuditBaseObjects;
extern ULONG CmNtGlobalFlag;
extern SIZE_T MmSizeOfPagedPoolInBytes;
extern SIZE_T MmSizeOfNonPagedPoolInBytes;
extern SIZE_T MmOverCommit;
extern ULONG MmLockPagesPercentage;
extern ULONG MmLargeSystemCache;
extern ULONG MmNumberOfSystemPtes;
extern ULONG MmUnusedSegmentTrimLevel;
extern ULONG MmSecondaryColors;
extern ULONG MmDisablePagingExecutive;
extern ULONG MmModifiedPageLifeInSeconds;
extern LOGICAL MmSpecialPoolCatchOverruns;
extern LOGICAL MmDynamicPfn;
extern ULONG MmEnforceWriteProtection;
extern ULONG MmLargePageMinimum;
extern LOGICAL MmSnapUnloads;
extern LOGICAL MmTrackLockedPages;
extern LOGICAL MmMakeLowMemory;
extern LOGICAL MmSupportWriteWatch;
extern LOGICAL MmProtectFreedNonPagedPool;
extern LOGICAL MmTrackPtes;
extern ULONG CmRegistrySizeLimit;
extern ULONG CmRegistrySizeLimitLength;
extern ULONG CmRegistrySizeLimitType;
extern ULONG PspDefaultPagedLimit;
extern ULONG PspDefaultNonPagedLimit;
extern ULONG PspDefaultPagefileLimit;
extern ULONG ExpResourceTimeoutCount;
extern ULONG MmCritsectTimeoutSeconds;
extern SIZE_T MmHeapSegmentReserve;
extern SIZE_T MmHeapSegmentCommit;
extern SIZE_T MmHeapDeCommitTotalFreeThreshold;
extern SIZE_T MmHeapDeCommitFreeBlockThreshold;
extern ULONG ExpAdditionalCriticalWorkerThreads;
extern ULONG ExpAdditionalDelayedWorkerThreads;
extern ULONG MmProductType;
extern ULONG ExpHydraEnabled;
extern ULONG ExpMultiUserTS;
extern LOGICAL IoCountOperations;
extern ULONG IopLargeIrpStackLocations;
extern ULONG IovpVerifierLevel;
extern ULONG MmZeroPageFile;
extern ULONG ExpNtExpirationData[3];
extern ULONG ExpNtExpirationDataLength;
extern ULONG ExpMaxTimeSeperationBeforeCorrect;
extern ULONG PopSimulate;
extern ULONG KiEnableTimerWatchdog;

#if defined(_ALPHA_) || defined(_IA64_)
extern ULONG KiEnableAlignmentFaultExceptions;
#endif

extern ULONG KiMaximumDpcQueueDepth;
extern ULONG KiMinimumDpcRate;
extern ULONG KiAdjustDpcThreshold;
extern ULONG KiIdealDpcRate;
extern LARGE_INTEGER ExpLastShutDown;
ULONG shutdownlength;

#if defined (i386)
extern ULONG KeI386ForceNpxEmulation;
#endif

//Debugger Retries
extern ULONG KdpDefaultRetries;

//
// WMI Control Variables
extern ULONG WmipMaxKmWnodeEventSize;
extern ULONG WmiTraceAlignment;

//
//  Vector - see ntos\inc\cm.h for definition
//
CM_SYSTEM_CONTROL_VECTOR   CmControlVector[] = {

    { L"Session Manager",
      L"ProtectionMode",
      &ObpProtectionMode,
      NULL,
      NULL
    },


    { L"LSA",
      L"AuditBaseDirectories",
      &ObpAuditBaseDirectories,
      NULL,
      NULL
    },


    { L"LSA",
      L"AuditBaseObjects",
      &ObpAuditBaseObjects,
      NULL,
      NULL
    },


    { L"TimeZoneInformation",
      L"ActiveTimeBias",
      &ExpLastTimeZoneBias,
      NULL,
      NULL
    },


    { L"TimeZoneInformation",
      L"Bias",
      &ExpAltTimeZoneBias,
      NULL,
      NULL
    },

    { L"TimeZoneInformation",
      L"RealTimeIsUniversal",
      &ExpRealTimeIsUniversal,
      NULL,
      NULL
    },

    { L"Session Manager",
      L"GlobalFlag",
      &CmNtGlobalFlag,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"DontVerifyRandomDrivers",
      &MmDontVerifyRandomDrivers,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"PagedPoolQuota",
      &PspDefaultPagedLimit,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"NonPagedPoolQuota",
      &PspDefaultNonPagedLimit,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"PagingFileQuota",
      &PspDefaultPagefileLimit,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"DynamicMemory",
      &MmDynamicPfn,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"UnusedFileCache",
      &MmUnusedSegmentTrimLevel,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"PagedPoolSize",
      &MmSizeOfPagedPoolInBytes,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"NonPagedPoolSize",
      &MmSizeOfNonPagedPoolInBytes,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"IoPageLockPercentage",
      &MmLockPagesPercentage,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"LargeSystemCache",
      &MmLargeSystemCache,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"OverCommitSize",
      &MmOverCommit,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"SystemPages",
      &MmNumberOfSystemPtes,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"DisablePagingExecutive",
      &MmDisablePagingExecutive,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"ModifiedPageLife",
      &MmModifiedPageLifeInSeconds,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"SecondLevelDataCache",
      &MmSecondaryColors,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"ClearPageFileAtShutdown",
      &MmZeroPageFile,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"PoolTag",
      &MmSpecialPoolTag,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"PoolTagOverruns",
      &MmSpecialPoolCatchOverruns,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"SnapUnloads",
      &MmSnapUnloads,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"ProtectNonPagedPool",
      &MmProtectFreedNonPagedPool,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"TrackLockedPages",
      &MmTrackLockedPages,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"TrackPtes",
      &MmTrackPtes,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"VerifyDrivers",
      MmVerifyDriverBuffer,
      &MmVerifyDriverBufferLength,
      &MmVerifyDriverBufferType
    },

    { L"Session Manager\\Memory Management",
      L"VerifyDriverLevel",
      &MmVerifyDriverLevel,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"LargePageMinimum",
      &MmLargePageMinimum,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"EnforceWriteProtection",
      &MmEnforceWriteProtection,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"MakeLowMemory",
      &MmMakeLowMemory,
      NULL,
      NULL
    },

    { L"Session Manager\\Memory Management",
      L"WriteWatch",
      &MmSupportWriteWatch,
      NULL,
      NULL
    },

    { L"Session Manager\\Executive",
      L"AdditionalCriticalWorkerThreads",
      &ExpAdditionalCriticalWorkerThreads,
      NULL,
      NULL
    },

    { L"Session Manager\\Executive",
      L"AdditionalDelayedWorkerThreads",
      &ExpAdditionalDelayedWorkerThreads,
      NULL,
      NULL
    },

    { L"Session Manager\\Executive",
      L"PriorityQuantumMatrix",
      &ExpNtExpirationData,
      &ExpNtExpirationDataLength,
      NULL
    },

    { L"Session Manager\\Kernel",
      L"DpcQueueDepth",
      &KiMaximumDpcQueueDepth,
      NULL,
      NULL
    },

    { L"Session Manager\\Kernel",
      L"MinimumDpcRate",
      &KiMinimumDpcRate,
      NULL,
      NULL
    },

    { L"Session Manager\\Kernel",
      L"AdjustDpcThreshold",
      &KiAdjustDpcThreshold,
      NULL,
      NULL
    },

    { L"Session Manager\\Kernel",
      L"IdealDpcRate",
      &KiIdealDpcRate,
      NULL,
      NULL
    },

    { L"Session Manager\\I/O System",
      L"CountOperations",
      &IoCountOperations,
      NULL,
      NULL
    },

    { L"Session Manager\\I/O System",
      L"LargeIrpStackLocations",
      &IopLargeIrpStackLocations,
      NULL,
      NULL
    },

    { L"Session Manager\\I/O System",
      L"IoVerifierLevel",
      &IovpVerifierLevel,
      NULL,
      NULL
    },

    { L"Session Manager",
      L"ResourceTimeoutCount",
      &ExpResourceTimeoutCount,
      NULL,
      NULL
    },

    { L"Session Manager",
      L"CriticalSectionTimeout",
      &MmCritsectTimeoutSeconds,
      NULL,
      NULL
    },

    { L"Session Manager",
      L"HeapSegmentReserve",
      &MmHeapSegmentReserve,
      NULL,
      NULL
    },

    { L"Session Manager",
      L"HeapSegmentCommit",
      &MmHeapSegmentCommit,
      NULL,
      NULL
    },

    { L"Session Manager",
      L"HeapDeCommitTotalFreeThreshold",
      &MmHeapDeCommitTotalFreeThreshold,
      NULL,
      NULL
    },

    { L"Session Manager",
      L"HeapDeCommitFreeBlockThreshold",
      &MmHeapDeCommitFreeBlockThreshold,
      NULL,
      NULL
    },

#if defined(_ALPHA_) || defined(_IA64_)

    { L"Session Manager",
      L"EnableAlignmentFaultExceptions",
      &KiEnableAlignmentFaultExceptions,
      NULL,
      NULL
    },

#endif

    { L"ProductOptions",
      L"ProductType",
      &MmProductType,
      NULL,
      NULL
    },

    { L"Terminal Server",
      L"TSEnabled",
      &ExpHydraEnabled,
      NULL,
      NULL
    },

    { L"Terminal Server",
      L"TSAppCompat",
      &ExpMultiUserTS,
      NULL,
      NULL
    },


    { L"ProductOptions",
      L"ProductSuite",
      CmSuiteBuffer,
      &CmSuiteBufferLength,
      &CmSuiteBufferType
    },

    { L"Windows",
      L"CSDVersion",
      &CmNtCSDVersion,
      NULL,
      NULL
    },

    { L"Nls\\Language",
      L"Default",
      CmDefaultLanguageId,
      &CmDefaultLanguageIdLength,
      &CmDefaultLanguageIdType
    },

    { L"Nls\\Language",
      L"InstallLanguage",
      CmInstallUILanguageId,
      &CmInstallUILanguageIdLength,
      &CmInstallUILanguageIdType
    },

    { L"\0\0",
      L"RegistrySizeLimit",
      &CmRegistrySizeLimit,
      &CmRegistrySizeLimitLength,
      &CmRegistrySizeLimitType
    },

#if defined(i386)
    { L"Session Manager",
      L"ForceNpxEmulation",
      &KeI386ForceNpxEmulation,
      NULL,
      NULL
    },

#endif

#if !defined(NT_UP)
    { L"Session Manager",
      L"RegisteredProcessors",
      &KeRegisteredProcessors,
      NULL,
      NULL
    },
    { L"Session Manager",
      L"LicensedProcessors",
      &KeLicensedProcessors,
      NULL,
      NULL
    },
#endif

    { L"Session Manager",
      L"PowerPolicySimulate",
      &PopSimulate,
      NULL,
      NULL
    },

    { L"Session Manager\\Executive",
      L"MaxTimeSeparationBeforeCorrect",
      &ExpMaxTimeSeperationBeforeCorrect,
      NULL,
      NULL
    },

    { L"Windows",
      L"ShutdownTime",
      &ExpLastShutDown,
      &shutdownlength,
      NULL
    },

    { L"PriorityControl",
      L"Win32PrioritySeparation",
      &PsRawPrioritySeparation,
      NULL,
      NULL
    },
    
#if defined(i386)
    { L"Session Manager",
      L"EnableTimerWatchdog",
      &KiEnableTimerWatchdog,
      NULL,
      NULL
    },
#endif

    { L"Session Manager",
      L"Debugger Retries",
      &KdpDefaultRetries,
      NULL,
      NULL
    },

    { L"WMI",
      L"MaxEventSize",
      &WmipMaxKmWnodeEventSize,
      NULL,
      NULL
    },
    
    { L"WMI\\Trace",
      L"UsePerformanceClock",
      &WmiUsePerfClock,
      NULL,
      NULL
    },

    { L"WMI\\Trace",
      L"TraceAlignment",
      &WmiTraceAlignment,
      NULL,
      NULL
    },

    { NULL, NULL, NULL, NULL, NULL }    // end marker
    };

#ifdef ALLOC_DATA_PRAGMA
#pragma  data_seg()
#endif
