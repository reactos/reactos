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
#include "./../mm/ARM3/miarm.h"

/* GLOBALS *******************************************************************/

ULONG DummyData;
ULONG CmNtGlobalFlag;
extern ULONG MmProductType;

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
    { L"HARDWARE", L"MACHINE\\", NULL, HIVE_VOLATILE    , 0 ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SECURITY", L"MACHINE\\", NULL, 0                , 0 ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SOFTWARE", L"MACHINE\\", NULL, 0                , 0 ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SYSTEM",   L"MACHINE\\", NULL, 0                , 0 ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"DEFAULT",  L"USER\\.DEFAULT", NULL, 0           , 0 ,   NULL,   FALSE,  FALSE,  FALSE},
    { L"SAM",      L"MACHINE\\", NULL, HIVE_NOLAZYFLUSH , 0 ,   NULL,   FALSE,  FALSE,  FALSE},
    { NULL,        NULL,         0, 0                   , 0 ,   NULL,   FALSE,  FALSE,  FALSE}
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

INIT_SECTION UNICODE_STRING CmClassName[MaximumClass + 1] =
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

INIT_SECTION UNICODE_STRING CmTypeName[MaximumType + 1] =
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

INIT_SECTION CMP_MF_TYPE CmpMultifunctionTypes[] =
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

INIT_SECTION CM_SYSTEM_CONTROL_VECTOR CmControlVector[] =
{
    {
        L"Session Manager",
        L"ProtectionMode",
        &ObpProtectionMode,
        NULL,
        NULL
    },
    {
        L"Session Manager",
        L"ObjectSecurityMode",
        &ObpObjectSecurityMode,
        NULL,
        NULL
    },
    {
        L"Session Manager",
        L"LUIDDeviceMapsDisabled",
        &ObpLUIDDeviceMapsDisabled,
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
        &MmDynamicPfn,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"Mirroring",
        &MmMirroring,
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
        &MmConsumedPoolPercentage,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"MapAllocationFragment",
        &MmAllocationFragment,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"PagedPoolSize",
        &MmSizeOfPagedPoolInBytes,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"NonPagedPoolSize",
        &MmSizeOfNonPagedPoolInBytes,
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
        &MmLargeSystemCache,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"LargeStackSize",
        &MmLargeStackSize,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"SystemPages",
        &MmNumberOfSystemPtes,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"LowMemoryThreshold",
        &MmLowMemoryThreshold,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"HighMemoryThreshold",
        &MmHighMemoryThreshold,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"DisablePagingExecutive",
        &MmDisablePagingExecutive,
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
        &MmSecondaryColors,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"ClearPageFileAtShutdown",
        &MmZeroPageFile,
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
        &MmProtectFreedNonPagedPool,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"TrackLockedPages",
        &MmTrackLockedPages,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"TrackPtes",
        &MmTrackPtes,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"VerifyDrivers",
        MmVerifyDriverBuffer,
        &MmVerifyDriverBufferLength,
        &MmVerifyDriverBufferType
    },
    {
        L"Session Manager\\Memory Management",
        L"VerifyDriverLevel",
        &MmVerifyDriverLevel,
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
        &MmEnforceWriteProtection,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Memory Management",
        L"MakeLowMemory",
        &MmMakeLowMemory,
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
        L"Session Manager\\Memory Management",
        L"MinimumStackCommitInBytes",
        &MmMinimumStackCommitInBytes,
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
        L"Session Manager\\Kernel",
        L"ObUnsecureGlobalNames",
        ObpUnsecureGlobalNamesBuffer,
        &ObpUnsecureGlobalNamesLength,
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
        &MmCritsectTimeoutSeconds,
        NULL,
        NULL
    },
    {
        L"Session Manager",
        L"HeapSegmentReserve",
        &MmHeapSegmentReserve,
        NULL,
        NULL
    },
    {
        L"Session Manager",
        L"HeapSegmentCommit",
        &MmHeapSegmentCommit,
        NULL,
        NULL
    },
    {
        L"Session Manager",
        L"HeapDeCommitTotalFreeThreshold",
        &MmHeapDeCommitTotalFreeThreshold,
        NULL,
        NULL
    },
    {
        L"Session Manager",
        L"HeapDeCommitFreeBlockThreshold",
        &MmHeapDeCommitFreeBlockThreshold,
        NULL,
        NULL
    },
    {
        L"ProductOptions",
        L"ProductType",
        &MmProductType,
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
        L"Windows",
        L"CSDReleaseType",
        &CmNtCSDReleaseType,
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
        &PsRawPrioritySeparation,
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
        &KdpContext.KdpDefaultRetries,
        NULL,
        NULL
    },

//
// Debug Filter Masks - See kd64/kddata.c
//
    {
        L"Session Manager\\Debug Print Filter",
        L"WIN2000",
        &Kd_WIN2000_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SYSTEM",
        &Kd_SYSTEM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SMSS",
        &Kd_SMSS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SETUP",
        &Kd_SETUP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"NTFS",
        &Kd_NTFS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FSTUB",
        &Kd_FSTUB_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CRASHDUMP",
        &Kd_CRASHDUMP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CDAUDIO",
        &Kd_CDAUDIO_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CDROM",
        &Kd_CDROM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CLASSPNP",
        &Kd_CLASSPNP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DISK",
        &Kd_DISK_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"REDBOOK",
        &Kd_REDBOOK_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"STORPROP",
        &Kd_STORPROP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SCSIPORT",
        &Kd_SCSIPORT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SCSIMINIPORT",
        &Kd_SCSIMINIPORT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CONFIG",
        &Kd_CONFIG_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"I8042PRT",
        &Kd_I8042PRT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SERMOUSE",
        &Kd_SERMOUSE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"LSERMOUS",
        &Kd_LSERMOUS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"KBDHID",
        &Kd_KBDHID_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MOUHID",
        &Kd_MOUHID_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"KBDCLASS",
        &Kd_KBDCLASS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MOUCLASS",
        &Kd_MOUCLASS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"TWOTRACK",
        &Kd_TWOTRACK_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"WMILIB",
        &Kd_WMILIB_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"ACPI",
        &Kd_ACPI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"AMLI",
        &Kd_AMLI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"HALIA64",
        &Kd_HALIA64_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VIDEO",
        &Kd_VIDEO_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SVCHOST",
        &Kd_SVCHOST_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VIDEOPRT",
        &Kd_VIDEOPRT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"TCPIP",
        &Kd_TCPIP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DMSYNTH",
        &Kd_DMSYNTH_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"NTOSPNP",
        &Kd_NTOSPNP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FASTFAT",
        &Kd_FASTFAT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SAMSS",
        &Kd_SAMSS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PNPMGR",
        &Kd_PNPMGR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"NETAPI",
        &Kd_NETAPI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SCSERVER",
        &Kd_SCSERVER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SCCLIENT",
        &Kd_SCCLIENT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SERIAL",
        &Kd_SERIAL_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SERENUM",
        &Kd_SERENUM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"UHCD",
        &Kd_UHCD_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"RPCPROXY",
        &Kd_RPCPROXY_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"AUTOCHK",
        &Kd_AUTOCHK_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DCOMSS",
        &Kd_DCOMSS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"UNIMODEM",
        &Kd_UNIMODEM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SIS",
        &Kd_SIS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FLTMGR",
        &Kd_FLTMGR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"WMICORE",
        &Kd_WMICORE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"BURNENG",
        &Kd_BURNENG_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IMAPI",
        &Kd_IMAPI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SXS",
        &Kd_SXS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FUSION",
        &Kd_FUSION_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IDLETASK",
        &Kd_IDLETASK_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SOFTPCI",
        &Kd_SOFTPCI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"TAPE",
        &Kd_TAPE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MCHGR",
        &Kd_MCHGR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IDEP",
        &Kd_IDEP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PCIIDE",
        &Kd_PCIIDE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FLOPPY",
        &Kd_FLOPPY_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FDC",
        &Kd_FDC_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"TERMSRV",
        &Kd_TERMSRV_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"W32TIME",
        &Kd_W32TIME_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PREFETCHER",
        &Kd_PREFETCHER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"RSFILTER",
        &Kd_RSFILTER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FCPORT",
        &Kd_FCPORT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PCI",
        &Kd_PCI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DMIO",
        &Kd_DMIO_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DMCONFIG",
        &Kd_DMCONFIG_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DMADMIN",
        &Kd_DMADMIN_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"WSOCKTRANSPORT",
        &Kd_WSOCKTRANSPORT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VSS",
        &Kd_VSS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PNPMEM",
        &Kd_PNPMEM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PROCESSOR",
        &Kd_PROCESSOR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DMSERVER",
        &Kd_DMSERVER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SR",
        &Kd_SR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"INFINIBAND",
        &Kd_INFINIBAND_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IHVDRIVER",
        &Kd_IHVDRIVER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IHVVIDEO",
        &Kd_IHVVIDEO_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IHVAUDIO",
        &Kd_IHVAUDIO_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IHVNETWORK",
        &Kd_IHVNETWORK_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IHVSTREAMING",
        &Kd_IHVSTREAMING_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IHVBUS",
        &Kd_IHVBUS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"HPS",
        &Kd_HPS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"RTLTHREADPOOL",
        &Kd_RTLTHREADPOOL_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"LDR",
        &Kd_LDR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"TCPIP6",
        &Kd_TCPIP6_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"ISAPNP",
        &Kd_ISAPNP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SHPC",
        &Kd_SHPC_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"STORPORT",
        &Kd_STORPORT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"STORMINIPORT",
        &Kd_STORMINIPORT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PRINTSPOOLER",
        &Kd_PRINTSPOOLER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VSSDYNDISK",
        &Kd_VSSDYNDISK_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VERIFIER",
        &Kd_VERIFIER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VDS",
        &Kd_VDS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VDSBAS",
        &Kd_VDSBAS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VDSDYN",
        &Kd_VDSDYN_Mask,    // Specified in Vista+
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VDSDYNDR",
        &Kd_VDSDYNDR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VDSLDR",
        &Kd_VDSLDR_Mask,    // Specified in Vista+
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"VDSUTIL",
        &Kd_VDSUTIL_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DFRGIFC",
        &Kd_DFRGIFC_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DEFAULT",
        &Kd_DEFAULT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MM",
        &Kd_MM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DFSC",
        &Kd_DFSC_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"WOW64",
        &Kd_WOW64_Mask,
        NULL,
        NULL
    },
//
// Components specified in Vista+, some of which we also use in ReactOS
//
    {
        L"Session Manager\\Debug Print Filter",
        L"ALPC",
        &Kd_ALPC_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"WDI",
        &Kd_WDI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PERFLIB",
        &Kd_PERFLIB_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"KTM",
        &Kd_KTM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"IOSTRESS",
        &Kd_IOSTRESS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"HEAP",
        &Kd_HEAP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"WHEA",
        &Kd_WHEA_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"USERGDI",
        &Kd_USERGDI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MMCSS",
        &Kd_MMCSS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"TPM",
        &Kd_TPM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"THREADORDER",
        &Kd_THREADORDER_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"ENVIRON",
        &Kd_ENVIRON_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"EMS",
        &Kd_EMS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"WDT",
        &Kd_WDT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FVEVOL",
        &Kd_FVEVOL_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"NDIS",
        &Kd_NDIS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"NVCTRACE",
        &Kd_NVCTRACE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"LUAFV",
        &Kd_LUAFV_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"APPCOMPAT",
        &Kd_APPCOMPAT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"USBSTOR",
        &Kd_USBSTOR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SBP2PORT",
        &Kd_SBP2PORT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"COVERAGE",
        &Kd_COVERAGE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CACHEMGR",
        &Kd_CACHEMGR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MOUNTMGR",
        &Kd_MOUNTMGR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CFR",
        &Kd_CFR_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"TXF",
        &Kd_TXF_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"KSECDD",
        &Kd_KSECDD_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FLTREGRESS",
        &Kd_FLTREGRESS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MPIO",
        &Kd_MPIO_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"MSDSM",
        &Kd_MSDSM_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"UDFS",
        &Kd_UDFS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"PSHED",
        &Kd_PSHED_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"STORVSP",
        &Kd_STORVSP_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"LSASS",
        &Kd_LSASS_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SSPICLI",
        &Kd_SSPICLI_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"CNG",
        &Kd_CNG_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"EXFAT",
        &Kd_EXFAT_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"FILETRACE",
        &Kd_FILETRACE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"XSAVE",
        &Kd_XSAVE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"SE",
        &Kd_SE_Mask,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Debug Print Filter",
        L"DRIVEEXTENDER",
        &Kd_DRIVEEXTENDER_Mask,
        NULL,
        NULL
    },
//
// END OF Debug Filter Masks
//

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
