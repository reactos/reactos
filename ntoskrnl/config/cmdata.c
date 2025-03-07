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

/* TRUE if all hives must be loaded in shared mode */
ULONG CmpVolatileBoot;
/* TRUE if the system hives must be loaded in shared mode */
BOOLEAN CmpShareSystemHives;
/* TRUE when the registry is in PE mode */
BOOLEAN CmpMiniNTBoot;

ULONG CmpBootType;
ULONG CmSelfHeal = TRUE;
BOOLEAN CmpSelfHeal = TRUE;

USHORT CmpUnknownBusCount;
ULONG CmpTypeCount[MaximumType + 1];

HANDLE CmpRegistryRootHandle;

DATA_SEG("INITDATA") UNICODE_STRING CmClassName[MaximumClass + 1] =
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

DATA_SEG("INITDATA") UNICODE_STRING CmTypeName[MaximumType + 1] =
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

DATA_SEG("INITDATA") CMP_MF_TYPE CmpMultifunctionTypes[] =
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

DATA_SEG("INITDATA") CM_SYSTEM_CONTROL_VECTOR CmControlVector[] =
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
        &ExpResourceTimeoutCount,
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
        L"Session Manager\\Configuration Manager",
        L"SelfHealingEnabled",
        &CmSelfHeal,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Configuration Manager",
        L"RegistryLazyFlushInterval",
        &CmpLazyFlushIntervalInSeconds,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Configuration Manager",
        L"RegistryLazyFlushHiveCount",
        &CmpLazyFlushHiveCount,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Configuration Manager",
        L"DelayCloseSize",
        &CmpDelayedCloseSize,
        NULL,
        NULL
    },
    {
        L"Session Manager\\Configuration Manager",
        L"VolatileBoot",
        &CmpVolatileBoot,
        NULL,
        NULL
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

#define WTEXT(s) L##s
#define CM_DEBUG_PRINT_FILTER(Name) \
    { \
        L"Session Manager\\Debug Print Filter", \
        WTEXT(#Name), \
        &Kd_##Name##_Mask, \
        NULL, \
        NULL \
    }

    CM_DEBUG_PRINT_FILTER(WIN2000),
    CM_DEBUG_PRINT_FILTER(SYSTEM),
    CM_DEBUG_PRINT_FILTER(SMSS),
    CM_DEBUG_PRINT_FILTER(SETUP),
    CM_DEBUG_PRINT_FILTER(NTFS),
    CM_DEBUG_PRINT_FILTER(FSTUB),
    CM_DEBUG_PRINT_FILTER(CRASHDUMP),
    CM_DEBUG_PRINT_FILTER(CDAUDIO),
    CM_DEBUG_PRINT_FILTER(CDROM),
    CM_DEBUG_PRINT_FILTER(CLASSPNP),
    CM_DEBUG_PRINT_FILTER(DISK),
    CM_DEBUG_PRINT_FILTER(REDBOOK),
    CM_DEBUG_PRINT_FILTER(STORPROP),
    CM_DEBUG_PRINT_FILTER(SCSIPORT),
    CM_DEBUG_PRINT_FILTER(SCSIMINIPORT),
    CM_DEBUG_PRINT_FILTER(CONFIG),
    CM_DEBUG_PRINT_FILTER(I8042PRT),
    CM_DEBUG_PRINT_FILTER(SERMOUSE),
    CM_DEBUG_PRINT_FILTER(LSERMOUS),
    CM_DEBUG_PRINT_FILTER(KBDHID),
    CM_DEBUG_PRINT_FILTER(MOUHID),
    CM_DEBUG_PRINT_FILTER(KBDCLASS),
    CM_DEBUG_PRINT_FILTER(MOUCLASS),
    CM_DEBUG_PRINT_FILTER(TWOTRACK),
    CM_DEBUG_PRINT_FILTER(WMILIB),
    CM_DEBUG_PRINT_FILTER(ACPI),
    CM_DEBUG_PRINT_FILTER(AMLI),
    CM_DEBUG_PRINT_FILTER(HALIA64),
    CM_DEBUG_PRINT_FILTER(VIDEO),
    CM_DEBUG_PRINT_FILTER(SVCHOST),
    CM_DEBUG_PRINT_FILTER(VIDEOPRT),
    CM_DEBUG_PRINT_FILTER(TCPIP),
    CM_DEBUG_PRINT_FILTER(DMSYNTH),
    CM_DEBUG_PRINT_FILTER(NTOSPNP),
    CM_DEBUG_PRINT_FILTER(FASTFAT),
    CM_DEBUG_PRINT_FILTER(SAMSS),
    CM_DEBUG_PRINT_FILTER(PNPMGR),
    CM_DEBUG_PRINT_FILTER(NETAPI),
    CM_DEBUG_PRINT_FILTER(SCSERVER),
    CM_DEBUG_PRINT_FILTER(SCCLIENT),
    CM_DEBUG_PRINT_FILTER(SERIAL),
    CM_DEBUG_PRINT_FILTER(SERENUM),
    CM_DEBUG_PRINT_FILTER(UHCD),
    CM_DEBUG_PRINT_FILTER(RPCPROXY),
    CM_DEBUG_PRINT_FILTER(AUTOCHK),
    CM_DEBUG_PRINT_FILTER(DCOMSS),
    CM_DEBUG_PRINT_FILTER(UNIMODEM),
    CM_DEBUG_PRINT_FILTER(SIS),
    CM_DEBUG_PRINT_FILTER(FLTMGR),
    CM_DEBUG_PRINT_FILTER(WMICORE),
    CM_DEBUG_PRINT_FILTER(BURNENG),
    CM_DEBUG_PRINT_FILTER(IMAPI),
    CM_DEBUG_PRINT_FILTER(SXS),
    CM_DEBUG_PRINT_FILTER(FUSION),
    CM_DEBUG_PRINT_FILTER(IDLETASK),
    CM_DEBUG_PRINT_FILTER(SOFTPCI),
    CM_DEBUG_PRINT_FILTER(TAPE),
    CM_DEBUG_PRINT_FILTER(MCHGR),
    CM_DEBUG_PRINT_FILTER(IDEP),
    CM_DEBUG_PRINT_FILTER(PCIIDE),
    CM_DEBUG_PRINT_FILTER(FLOPPY),
    CM_DEBUG_PRINT_FILTER(FDC),
    CM_DEBUG_PRINT_FILTER(TERMSRV),
    CM_DEBUG_PRINT_FILTER(W32TIME),
    CM_DEBUG_PRINT_FILTER(PREFETCHER),
    CM_DEBUG_PRINT_FILTER(RSFILTER),
    CM_DEBUG_PRINT_FILTER(FCPORT),
    CM_DEBUG_PRINT_FILTER(PCI),
    CM_DEBUG_PRINT_FILTER(DMIO),
    CM_DEBUG_PRINT_FILTER(DMCONFIG),
    CM_DEBUG_PRINT_FILTER(DMADMIN),
    CM_DEBUG_PRINT_FILTER(WSOCKTRANSPORT),
    CM_DEBUG_PRINT_FILTER(VSS),
    CM_DEBUG_PRINT_FILTER(PNPMEM),
    CM_DEBUG_PRINT_FILTER(PROCESSOR),
    CM_DEBUG_PRINT_FILTER(DMSERVER),
    CM_DEBUG_PRINT_FILTER(SR),
    CM_DEBUG_PRINT_FILTER(INFINIBAND),
    CM_DEBUG_PRINT_FILTER(IHVDRIVER),
    CM_DEBUG_PRINT_FILTER(IHVVIDEO),
    CM_DEBUG_PRINT_FILTER(IHVAUDIO),
    CM_DEBUG_PRINT_FILTER(IHVNETWORK),
    CM_DEBUG_PRINT_FILTER(IHVSTREAMING),
    CM_DEBUG_PRINT_FILTER(IHVBUS),
    CM_DEBUG_PRINT_FILTER(HPS),
    CM_DEBUG_PRINT_FILTER(RTLTHREADPOOL),
    CM_DEBUG_PRINT_FILTER(LDR),
    CM_DEBUG_PRINT_FILTER(TCPIP6),
    CM_DEBUG_PRINT_FILTER(ISAPNP),
    CM_DEBUG_PRINT_FILTER(SHPC),
    CM_DEBUG_PRINT_FILTER(STORPORT),
    CM_DEBUG_PRINT_FILTER(STORMINIPORT),
    CM_DEBUG_PRINT_FILTER(PRINTSPOOLER),
    CM_DEBUG_PRINT_FILTER(VSSDYNDISK),
    CM_DEBUG_PRINT_FILTER(VERIFIER),
    CM_DEBUG_PRINT_FILTER(VDS),
    CM_DEBUG_PRINT_FILTER(VDSBAS),
    CM_DEBUG_PRINT_FILTER(VDSDYN),  // Specified in Vista+
    CM_DEBUG_PRINT_FILTER(VDSDYNDR),
    CM_DEBUG_PRINT_FILTER(VDSLDR),  // Specified in Vista+
    CM_DEBUG_PRINT_FILTER(VDSUTIL),
    CM_DEBUG_PRINT_FILTER(DFRGIFC),
    CM_DEBUG_PRINT_FILTER(DEFAULT),
    CM_DEBUG_PRINT_FILTER(MM),
    CM_DEBUG_PRINT_FILTER(DFSC),
    CM_DEBUG_PRINT_FILTER(WOW64),
//
// Components specified in Vista+, some of which we also use in ReactOS
//
    CM_DEBUG_PRINT_FILTER(ALPC),
    CM_DEBUG_PRINT_FILTER(WDI),
    CM_DEBUG_PRINT_FILTER(PERFLIB),
    CM_DEBUG_PRINT_FILTER(KTM),
    CM_DEBUG_PRINT_FILTER(IOSTRESS),
    CM_DEBUG_PRINT_FILTER(HEAP),
    CM_DEBUG_PRINT_FILTER(WHEA),
    CM_DEBUG_PRINT_FILTER(USERGDI),
    CM_DEBUG_PRINT_FILTER(MMCSS),
    CM_DEBUG_PRINT_FILTER(TPM),
    CM_DEBUG_PRINT_FILTER(THREADORDER),
    CM_DEBUG_PRINT_FILTER(ENVIRON),
    CM_DEBUG_PRINT_FILTER(EMS),
    CM_DEBUG_PRINT_FILTER(WDT),
    CM_DEBUG_PRINT_FILTER(FVEVOL),
    CM_DEBUG_PRINT_FILTER(NDIS),
    CM_DEBUG_PRINT_FILTER(NVCTRACE),
    CM_DEBUG_PRINT_FILTER(LUAFV),
    CM_DEBUG_PRINT_FILTER(APPCOMPAT),
    CM_DEBUG_PRINT_FILTER(USBSTOR),
    CM_DEBUG_PRINT_FILTER(SBP2PORT),
    CM_DEBUG_PRINT_FILTER(COVERAGE),
    CM_DEBUG_PRINT_FILTER(CACHEMGR),
    CM_DEBUG_PRINT_FILTER(MOUNTMGR),
    CM_DEBUG_PRINT_FILTER(CFR),
    CM_DEBUG_PRINT_FILTER(TXF),
    CM_DEBUG_PRINT_FILTER(KSECDD),
    CM_DEBUG_PRINT_FILTER(FLTREGRESS),
    CM_DEBUG_PRINT_FILTER(MPIO),
    CM_DEBUG_PRINT_FILTER(MSDSM),
    CM_DEBUG_PRINT_FILTER(UDFS),
    CM_DEBUG_PRINT_FILTER(PSHED),
    CM_DEBUG_PRINT_FILTER(STORVSP),
    CM_DEBUG_PRINT_FILTER(LSASS),
    CM_DEBUG_PRINT_FILTER(SSPICLI),
    CM_DEBUG_PRINT_FILTER(CNG),
    CM_DEBUG_PRINT_FILTER(EXFAT),
    CM_DEBUG_PRINT_FILTER(FILETRACE),
    CM_DEBUG_PRINT_FILTER(XSAVE),
    CM_DEBUG_PRINT_FILTER(SE),
    CM_DEBUG_PRINT_FILTER(DRIVEEXTENDER),
//
// Components specified in Windows 8
//
    CM_DEBUG_PRINT_FILTER(POWER),
    CM_DEBUG_PRINT_FILTER(CRASHDUMPXHCI),
    CM_DEBUG_PRINT_FILTER(GPIO),
    CM_DEBUG_PRINT_FILTER(REFS),
    CM_DEBUG_PRINT_FILTER(WER),
//
// Components specified in Windows 10
//
    CM_DEBUG_PRINT_FILTER(CAPIMG),
    CM_DEBUG_PRINT_FILTER(VPCI),
    CM_DEBUG_PRINT_FILTER(STORAGECLASSMEMORY),
    CM_DEBUG_PRINT_FILTER(FSLIB),

#undef WTEXT
#undef CM_DEBUG_PRINT_FILTER

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
        L"CrashControl",
        L"AutoReboot",
        &IopAutoReboot,
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
