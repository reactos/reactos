/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmdata.c
 * PURPOSE:         Configuration Manager - Global Configuration Data
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
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

ULONG CmpConfigurationAreaSize = PAGE_SIZE * 4;
PCM_FULL_RESOURCE_DESCRIPTOR CmpConfigurationData;

EX_PUSH_LOCK CmpHiveListHeadLock, CmpLoadHiveLock;

HIVE_LIST_ENTRY CmpMachineHiveList[] =
{
    { L"HARDWARE", L"MACHINE\\", NULL, HIVE_VOLATILE    , 0                         ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SECURITY", L"MACHINE\\", NULL, 0                , 0                         ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SOFTWARE", L"MACHINE\\", NULL, 0                , 0                         ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SYSTEM",   L"MACHINE\\", NULL, 0                , 0                         ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"DEFAULT",  L"USER\\.DEFAULT", NULL, 0           , 0   ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SAM",      L"MACHINE\\", NULL, HIVE_NOLAZYFLUSH , 0                         ,   NULL,   FALSE,  FALSE,  FALSE},
    { NULL,        NULL,         0, 0                   , 0                         ,   NULL,   FALSE,  FALSE,  FALSE}
};

UNICODE_STRING CmSymbolicLinkValueName =
    RTL_CONSTANT_STRING(L"SymbolicLinkValue");

UNICODE_STRING CmpLoadOptions;

BOOLEAN CmpShareSystemHives;
BOOLEAN CmSelfHeal = TRUE;
BOOLEAN CmpSelfHeal = TRUE;
BOOLEAN CmpMiniNTBoot;
ULONG CmpBootType;

USHORT CmpUnknownBusCount;
ULONG CmpTypeCount[MaximumType + 1];

HANDLE CmpRegistryRootHandle;

UNICODE_STRING CmClassName[MaximumClass + 1] =
{
    RTL_CONSTANT_STRING(L"System"),
    RTL_CONSTANT_STRING(L"Processor"),
    RTL_CONSTANT_STRING(L"Cache"),
    RTL_CONSTANT_STRING(L"Adapter"),
    RTL_CONSTANT_STRING(L"Controller"),
    RTL_CONSTANT_STRING(L"Peripheral"),
    RTL_CONSTANT_STRING(L"MemoryClass"),
    RTL_CONSTANT_STRING(L"Undefined")
};

UNICODE_STRING CmTypeName[MaximumType + 1] =
{
    RTL_CONSTANT_STRING(L"System"),
    RTL_CONSTANT_STRING(L"CentralProcessor"),
    RTL_CONSTANT_STRING(L"FloatingPointProcessor"),
    RTL_CONSTANT_STRING(L"PrimaryICache"),
    RTL_CONSTANT_STRING(L"PrimaryDCache"),
    RTL_CONSTANT_STRING(L"SecondaryICache"),
    RTL_CONSTANT_STRING(L"SecondaryDCache"),
    RTL_CONSTANT_STRING(L"SecondaryCache"),
    RTL_CONSTANT_STRING(L"EisaAdapter"),
    RTL_CONSTANT_STRING(L"TcAdapter"),
    RTL_CONSTANT_STRING(L"ScsiAdapter"),
    RTL_CONSTANT_STRING(L"DtiAdapter"),
    RTL_CONSTANT_STRING(L"MultifunctionAdapter"),
    RTL_CONSTANT_STRING(L"DiskController"),
    RTL_CONSTANT_STRING(L"TapeController"),
    RTL_CONSTANT_STRING(L"CdRomController"),
    RTL_CONSTANT_STRING(L"WormController"),
    RTL_CONSTANT_STRING(L"SerialController"),
    RTL_CONSTANT_STRING(L"NetworkController"),
    RTL_CONSTANT_STRING(L"DisplayController"),
    RTL_CONSTANT_STRING(L"ParallelController"),
    RTL_CONSTANT_STRING(L"PointerController"),
    RTL_CONSTANT_STRING(L"KeyboardController"),
    RTL_CONSTANT_STRING(L"AudioController"),
    RTL_CONSTANT_STRING(L"OtherController"),
    RTL_CONSTANT_STRING(L"DiskPeripheral"),
    RTL_CONSTANT_STRING(L"FloppyDiskPeripheral"),
    RTL_CONSTANT_STRING(L"TapePeripheral"),
    RTL_CONSTANT_STRING(L"ModemPeripheral"),
    RTL_CONSTANT_STRING(L"MonitorPeripheral"),
    RTL_CONSTANT_STRING(L"PrinterPeripheral"),
    RTL_CONSTANT_STRING(L"PointerPeripheral"),
    RTL_CONSTANT_STRING(L"KeyboardPeripheral"),
    RTL_CONSTANT_STRING(L"TerminalPeripheral"),
    RTL_CONSTANT_STRING(L"OtherPeripheral"),
    RTL_CONSTANT_STRING(L"LinePeripheral"),
    RTL_CONSTANT_STRING(L"NetworkPeripheral"),
    RTL_CONSTANT_STRING(L"SystemMemory"),
    RTL_CONSTANT_STRING(L"DockingInformation"),
    RTL_CONSTANT_STRING(L"RealModeIrqRoutingTable"),
    RTL_CONSTANT_STRING(L"RealModePCIEnumeration"),
    RTL_CONSTANT_STRING(L"Undefined")
};

CMP_MF_TYPE CmpMultifunctionTypes[] =
{
    {"ISA", Isa, 0},
    {"MCA", MicroChannel, 0},
    {"PCI", PCIBus, 0},
    {"VME", VMEBus, 0},
    {"PCMCIA", PCMCIABus, 0},
    {"CBUS", CBus, 0},
    {"MPIPI", MPIBus, 0},
    {"MPSA", MPSABus, 0},
    {NULL, Internal, 0}
};

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
