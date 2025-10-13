#ifndef _ARC_
#define _ARC_

typedef ULONG ARC_STATUS;

/* Avoid conflicts with errno.h */
#undef E2BIG
#undef EACCES
#undef EAGAIN
#undef EBADF
#undef EBUSY
#undef EFAULT
#undef EINVAL
#undef EIO
#undef EISDIR
#undef EMFILE
#undef EMLINK
#undef ENAMETOOLONG
#undef ENODEV
#undef ENOENT
#undef ENOEXEC
#undef ENOMEM
#undef ENOSPC
#undef ENOTDIR
#undef ENOTTY
#undef ENXIO
#undef EROFS
#undef EMAXIMUM

typedef enum _ARC_CODES
{
    ESUCCESS,
    E2BIG,
    EACCES,
    EAGAIN,
    EBADF,
    EBUSY,
    EFAULT,
    EINVAL,
    EIO,
    EISDIR,
    EMFILE,
    EMLINK,
    ENAMETOOLONG,
    ENODEV,
    ENOENT,
    ENOEXEC,
    ENOMEM,
    ENOSPC,
    ENOTDIR,
    ENOTTY,
    ENXIO,
    EROFS,
    EMAXIMUM
} ARC_CODES;

typedef enum _SEEKMODE
{
    SeekAbsolute,
    SeekRelative,
} SEEKMODE;

typedef enum _OPENMODE
{
    OpenReadOnly,
    OpenWriteOnly,
    OpenReadWrite,
    CreateWriteOnly,
    CreateReadWrite,
    SupersedeWriteOnly,
    SupersedeReadWrite,
    OpenDirectory,
    CreateDirectory,
} OPENMODE;

typedef enum _FILEATTRIBUTES
{
    ReadOnlyFile = 0x01,
    HiddenFile = 0x02,
    SystemFile = 0x04,
    ArchiveFile = 0x08,
    DirectoryFile = 0x10,
    DeleteFile = 0x20
} FILEATTRIBUTES;

typedef enum _IDENTIFIER_FLAG
{
    Failed = 0x01,
    ReadOnly = 0x02,
    Removable = 0x04,
    ConsoleIn = 0x08,
    ConsoleOut = 0x10,
    Input = 0x20,
    Output = 0x40
} IDENTIFIER_FLAG;

typedef enum _CONFIGURATION_CLASS
{
    SystemClass,
    ProcessorClass,
    CacheClass,
    AdapterClass,
    ControllerClass,
    PeripheralClass,
    MemoryClass,
    MaximumClass
} CONFIGURATION_CLASS;

// CONFIGURATION_TYPE is also defined in ntddk.h
#ifndef _ARC_DDK_
typedef enum _CONFIGURATION_TYPE
{
    ArcSystem,
    CentralProcessor,
    FloatingPointProcessor,
    PrimaryIcache,
    PrimaryDcache,
    SecondaryIcache,
    SecondaryDcache,
    SecondaryCache,
    EisaAdapter,
    TcAdapter,
    ScsiAdapter,
    DtiAdapter,
    MultiFunctionAdapter,
    DiskController,
    TapeController,
    CdromController,
    WormController,
    SerialController,
    NetworkController,
    DisplayController,
    ParallelController,
    PointerController,
    KeyboardController,
    AudioController,
    OtherController,
    DiskPeripheral,
    FloppyDiskPeripheral,
    TapePeripheral,
    ModemPeripheral,
    MonitorPeripheral,
    PrinterPeripheral,
    PointerPeripheral,
    KeyboardPeripheral,
    TerminalPeripheral,
    OtherPeripheral,
    LinePeripheral,
    NetworkPeripheral,
    SystemMemory,
    DockingInformation,
    RealModeIrqRoutingTable,
    RealModePCIEnumeration,
    MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;
#endif /* _ARC_DDK_ */

typedef struct _CONFIGURATION_COMPONENT
{
    CONFIGURATION_CLASS Class;
    CONFIGURATION_TYPE Type;
    IDENTIFIER_FLAG Flags;
    USHORT Version;
    USHORT Revision;
    ULONG Key;
    ULONG AffinityMask;
    ULONG ConfigurationDataLength;
    ULONG IdentifierLength;
    PCHAR Identifier;
} CONFIGURATION_COMPONENT, *PCONFIGURATION_COMPONENT;

typedef struct _MONITOR_CONFIGURATION_DATA
{
    USHORT Version;
    USHORT Revision;
    USHORT HorizontalResolution;
    USHORT HorizontalDisplayTime;
    USHORT HorizontalBackPorch;
    USHORT HorizontalFrontPorch;
    USHORT HorizontalSync;
    USHORT VerticalResolution;
    USHORT VerticalBackPorch;
    USHORT VerticalFrontPorch;
    USHORT VerticalSync;
    USHORT HorizontalScreenSize;
    USHORT VerticalScreenSize;
} MONITOR_CONFIGURATION_DATA, *PMONITOR_CONFIGURATION_DATA;

typedef struct _FLOPPY_CONFIGURATION_DATA
{
    USHORT Version;
    USHORT Revision;
    CHAR Size[8];
    ULONG MaxDensity;
    ULONG MountDensity;
} FLOPPY_CONFIGURATION_DATA, *PFLOPPY_CONFIGURATION_DATA;

typedef struct _TIMEINFO
{
    USHORT Year;
    USHORT Month;
    USHORT Day;
    USHORT Hour;
    USHORT Minute;
    USHORT Second;
} TIMEINFO;

typedef enum _MEMORY_TYPE
{
    MemoryExceptionBlock,
    MemorySystemBlock,
    MemoryFree,
    MemoryBad,
    MemoryLoadedProgram,
    MemoryFirmwareTemporary,
    MemoryFirmwarePermanent,
    MemoryFreeContiguous,
    MemorySpecialMemory,
    MemoryMaximum
} MEMORY_TYPE;

typedef struct _MEMORY_DESCRIPTOR
{
    MEMORY_TYPE MemoryType;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} MEMORY_DESCRIPTOR, *PMEMORY_DESCRIPTOR;

typedef struct _FILEINFORMATION
{
    LARGE_INTEGER StartingAddress;
    LARGE_INTEGER EndingAddress;
    LARGE_INTEGER CurrentAddress;
    CONFIGURATION_TYPE Type;
    ULONG FileNameLength;
    UCHAR Attributes;
    CHAR FileName[32];
} FILEINFORMATION;

typedef
ARC_STATUS
(*ARC_CLOSE)(
    ULONG FileId
);

typedef
ARC_STATUS
(*ARC_GET_FILE_INFORMATION)(
    ULONG FileId,
    FILEINFORMATION* Information
);

typedef
ARC_STATUS
(*ARC_OPEN)(
    CHAR* Path,
    OPENMODE OpenMode,
    ULONG* FileId
);

typedef
ARC_STATUS
(*ARC_READ)(
    ULONG FileId,
    VOID* Buffer,
    ULONG N, ULONG* Count
);

typedef
ARC_STATUS
(*ARC_SEEK)(
    ULONG FileId,
    LARGE_INTEGER* Position,
    SEEKMODE SeekMode
);


//
// Definitions for the NT OS Loader and the Loader Parameter Block
//

typedef struct _CONFIGURATION_COMPONENT_DATA
{
    struct _CONFIGURATION_COMPONENT_DATA *Parent;
    struct _CONFIGURATION_COMPONENT_DATA *Child;
    struct _CONFIGURATION_COMPONENT_DATA *Sibling;
    CONFIGURATION_COMPONENT ComponentEntry;
    PVOID ConfigurationData;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;

typedef enum _TYPE_OF_MEMORY
{
    LoaderExceptionBlock,
    LoaderSystemBlock,
    LoaderFree,
    LoaderBad,
    LoaderLoadedProgram,
    LoaderFirmwareTemporary,
    LoaderFirmwarePermanent,
    LoaderOsloaderHeap,
    LoaderOsloaderStack,
    LoaderSystemCode,
    LoaderHalCode,
    LoaderBootDriver,
    LoaderConsoleInDriver,
    LoaderConsoleOutDriver,
    LoaderStartupDpcStack,
    LoaderStartupKernelStack,
    LoaderStartupPanicStack,
    LoaderStartupPcrPage,
    LoaderStartupPdrPage,
    LoaderRegistryData,
    LoaderMemoryData,
    LoaderNlsData,
    LoaderSpecialMemory,
    LoaderBBTMemory,
    LoaderReserve,
    LoaderXIPRom,
    LoaderHALCachedMemory,
    LoaderLargePageFiller,
    LoaderErrorLogMemory,
    LoaderMaximum
} TYPE_OF_MEMORY;

typedef struct _MEMORY_ALLOCATION_DESCRIPTOR
{
    LIST_ENTRY ListEntry;
    TYPE_OF_MEMORY MemoryType;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} MEMORY_ALLOCATION_DESCRIPTOR, *PMEMORY_ALLOCATION_DESCRIPTOR;

typedef struct _BOOT_DRIVER_LIST_ENTRY
{
    LIST_ENTRY Link;
    UNICODE_STRING FilePath;
    UNICODE_STRING RegistryPath;
    struct _LDR_DATA_TABLE_ENTRY *LdrEntry;
} BOOT_DRIVER_LIST_ENTRY, *PBOOT_DRIVER_LIST_ENTRY;

typedef struct _ARC_DISK_SIGNATURE
{
    LIST_ENTRY ListEntry;
    ULONG Signature;
    PCHAR ArcName;
    ULONG CheckSum;
    BOOLEAN ValidPartitionTable;
    BOOLEAN xInt13;
    BOOLEAN IsGpt;
    BOOLEAN Reserved;
    CHAR GptSignature[16];
} ARC_DISK_SIGNATURE, *PARC_DISK_SIGNATURE;

typedef struct _ARC_DISK_INFORMATION
{
    LIST_ENTRY DiskSignatureListHead;
} ARC_DISK_INFORMATION, *PARC_DISK_INFORMATION;

//
// SMBIOS Table Header (FIXME: maybe move to smbios.h?)
//
typedef struct _SMBIOS_TABLE_HEADER
{
   CHAR Signature[4];
   UCHAR Checksum;
   UCHAR Length;
   UCHAR MajorVersion;
   UCHAR MinorVersion;
   USHORT MaximumStructureSize;
   UCHAR EntryPointRevision;
   UCHAR Reserved[5];
   CHAR Signature2[5];
   UCHAR IntermediateChecksum;
   USHORT StructureTableLength;
   ULONG StructureTableAddress;
   USHORT NumberStructures;
   UCHAR Revision;
} SMBIOS_TABLE_HEADER, *PSMBIOS_TABLE_HEADER;

typedef struct _SMBIOS3_TABLE_HEADER
{
    UCHAR Signature[5];
    UCHAR Checksum;
    UCHAR Length;
    UCHAR MajorVersion;
    UCHAR MinorVersion;
    UCHAR Docrev;
    UCHAR EntryPointRevision;
    UCHAR Reserved;
    ULONG StructureTableMaximumSize;
    ULONGLONG StructureTableAddress;
} SMBIOS3_TABLE_HEADER, *PSMBIOS3_TABLE_HEADER;

//
// NLS Data Block
//
typedef struct _NLS_DATA_BLOCK
{
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCodePageData;
} NLS_DATA_BLOCK, *PNLS_DATA_BLOCK;

//
// ACPI Docking State
//
typedef struct _PROFILE_ACPI_DOCKING_STATE
{
    USHORT DockingState;
    USHORT SerialLength;
    WCHAR SerialNumber[1];
} PROFILE_ACPI_DOCKING_STATE, *PPROFILE_ACPI_DOCKING_STATE;

//
// Subsystem Specific Loader Blocks
//
typedef struct _PROFILE_PARAMETER_BLOCK
{
    USHORT Status;
    USHORT Reserved;
    USHORT DockingState;
    USHORT Capabilities;
    ULONG DockID;
    ULONG SerialNumber;
} PROFILE_PARAMETER_BLOCK, *PPROFILE_PARAMETER_BLOCK;

typedef struct _HEADLESS_LOADER_BLOCK
{
    UCHAR UsedBiosSettings;
    UCHAR DataBits;
    UCHAR StopBits;
    UCHAR Parity;
    ULONG BaudRate;
    ULONG PortNumber;
    PUCHAR PortAddress;
    USHORT PciDeviceId;
    USHORT PciVendorId;
    UCHAR PciBusNumber;
    UCHAR PciSlotNumber;
    UCHAR PciFunctionNumber;
    ULONG PciFlags;
    GUID SystemGUID;
    UCHAR IsMMIODevice;
    UCHAR TerminalType;
} HEADLESS_LOADER_BLOCK, *PHEADLESS_LOADER_BLOCK;

typedef struct _NETWORK_LOADER_BLOCK
{
    PCHAR DHCPServerACK;
    ULONG DHCPServerACKLength;
    PCHAR BootServerReplyPacket;
    ULONG BootServerReplyPacketLength;
} NETWORK_LOADER_BLOCK, *PNETWORK_LOADER_BLOCK;

typedef struct _LOADER_PERFORMANCE_DATA
{
    ULONGLONG StartTime;
    ULONGLONG EndTime;
} LOADER_PERFORMANCE_DATA, *PLOADER_PERFORMANCE_DATA;

typedef enum _TPM_BOOT_ENTROPY_RESULT_CODE
{
    TpmBootEntropyStructureUninitialized = 0,
    TpmBootEntropyDisabledByPolicy = 1,
    TpmBootEntropyNoTpmFound = 2,
    TpmBootEntropyTpmError = 3,
    TpmBootEntropySuccess = 4
} TPM_BOOT_ENTROPY_RESULT_CODE, *PTPM_BOOT_ENTROPY_RESULT_CODE;

typedef struct _TPM_BOOT_ENTROPY_LDR_RESULT
{
    ULONGLONG Policy;
    TPM_BOOT_ENTROPY_RESULT_CODE ResultCode;
    LONG ResultStatus;
    ULONGLONG Time;
    ULONG EntropyLength;
    UCHAR EntropyData[40];
} TPM_BOOT_ENTROPY_LDR_RESULT, *PTPM_BOOT_ENTROPY_LDR_RESULT;

typedef enum _BOOT_ENTROPY_SOURCE_RESULT_CODE
{
    BootEntropySourceStructureUninitialized = 0,
    BootEntropySourceDisabledByPolicy = 1,
    BootEntropySourceNotPresent = 2,
    BootEntropySourceError = 3,
    BootEntropySourceSuccess = 4,
} BOOT_ENTROPY_SOURCE_RESULT_CODE, *PBOOT_ENTROPY_SOURCE_RESULT_CODE;

typedef enum _BOOT_ENTROPY_SOURCE_ID
{
    BootEntropySourceNone = 0,
    BootEntropySourceSeedfile = 1,
    BootEntropySourceExternal = 2,
    BootEntropySourceTpm = 3,
    BootEntropySourceRdrand = 4,
    BootEntropySourceTime = 5,
    BootEntropySourceAcpiOem0 = 6,
    BootEntropySourceUefi = 7,
    BootEntropySourceCng = 8,
    BootMaxEntropySources = 8,
} BOOT_ENTROPY_SOURCE_ID, *PBOOT_ENTROPY_SOURCE_ID;

typedef struct _BOOT_ENTROPY_SOURCE_LDR_RESULT
{
    BOOT_ENTROPY_SOURCE_ID SourceId;
    ULONGLONG Policy;
    BOOT_ENTROPY_SOURCE_RESULT_CODE ResultCode;
    NTSTATUS ResultStatus;
    ULONGLONG Time;
    ULONG EntropyLength;
    UCHAR EntropyData[64];
} BOOT_ENTROPY_SOURCE_LDR_RESULT, *PBOOT_ENTROPY_SOURCE_LDR_RESULT;

typedef struct _BOOT_ENTROPY_LDR_RESULT
{
    ULONG maxEntropySources;
    BOOT_ENTROPY_SOURCE_LDR_RESULT EntropySourceResult[BootMaxEntropySources];
    UCHAR SeedBytesForCng[48];
    UCHAR RngBytesForNtoskrnl[1024];
} BOOT_ENTROPY_LDR_RESULT, *PBOOT_ENTROPY_LDR_RESULT;

typedef struct _LOADER_PARAMETER_HYPERVISOR_EXTENSION
{
    ULONG HypervisorCrashdumpAreaPageCount;
    ULONGLONG HypervisorCrashdumpAreaSpa;
    ULONGLONG HypervisorLaunchStatus;
    ULONGLONG HypervisorLaunchStatusArg1;
    ULONGLONG HypervisorLaunchStatusArg2;
    ULONGLONG HypervisorLaunchStatusArg3;
    ULONGLONG HypervisorLaunchStatusArg4;
} LOADER_PARAMETER_HYPERVISOR_EXTENSION, *PLOADER_PARAMETER_HYPERVISOR_EXTENSION;

typedef struct _LOADER_BUGCHECK_PARAMETERS
{
    ULONG BugcheckCode;
    ULONG_PTR BugcheckParameter1;
    ULONG_PTR BugcheckParameter2;
    ULONG_PTR BugcheckParameter3;
    ULONG_PTR BugcheckParameter4;
} LOADER_BUGCHECK_PARAMETERS, *PLOADER_BUGCHECK_PARAMETERS;

typedef struct _OFFLINE_CRASHDUMP_CONFIGURATION_TABLE
{
    ULONG Version;
    ULONG AbnormalResetOccurred;
    ULONG OfflineMemoryDumpCapable;
} OFFLINE_CRASHDUMP_CONFIGURATION_TABLE, *POFFLINE_CRASHDUMP_CONFIGURATION_TABLE;

typedef struct _LOADER_PARAMETER_CI_EXTENSION
{
    ULONG RevocationListOffset;
    ULONG RevocationListSize;
    _Field_size_bytes_(RevocationListSize)
    UCHAR SerializedData[ANYSIZE_ARRAY];
} LOADER_PARAMETER_CI_EXTENSION, *PLOADER_PARAMETER_CI_EXTENSION;

//
// Extended Loader Parameter Block
//
// See http://www.geoffchappell.com/studies/windows/km/ntoskrnl/structs/loader_parameter_extension.htm
// for more details.
//
typedef struct _LOADER_PARAMETER_EXTENSION
{
    ULONG Size;
    PROFILE_PARAMETER_BLOCK Profile;
#if (NTDDI_VERSION < NTDDI_WIN7)
    ULONG MajorVersion;
    ULONG MinorVersion;
#endif
    PVOID EmInfFileImage;
    ULONG EmInfFileSize;
    PVOID TriageDumpBlock;
#if (NTDDI_VERSION >= NTDDI_WINXP)
#if (NTDDI_VERSION < NTDDI_WIN8)
    ULONG_PTR LoaderPagesSpanned;
#endif
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
#if (NTDDI_VERSION < NTDDI_WIN10)
    PSMBIOS_TABLE_HEADER SMBiosEPSHeader;
#else
    PSMBIOS3_TABLE_HEADER SMBiosEPSHeader;
#endif
    PVOID DrvDBImage;
    ULONG DrvDBSize;
#endif
#if (NTDDI_VERSION >= NTDDI_WINXPSP1)
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock;
#endif
#if (NTDDI_VERSION >= NTDDI_WS03)
#ifdef _X86_
    PUCHAR HalpIRQLToTPR;
    PUCHAR HalpVectorToIRQL;
#endif
    LIST_ENTRY FirmwareDescriptorListHead;
#endif
#if (NTDDI_VERSION >= NTDDI_WS03SP1)
    PVOID AcpiTable;
    ULONG AcpiTableSize;
#endif
/** NT-version-dependent flags **/
#if (OSVER(NTDDI_VERSION) == NTDDI_LONGHORN)
    ULONG BootViaWinload:1;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN7)
    ULONG LastBootSucceeded:1;
    ULONG LastBootShutdown:1;
    ULONG IoPortAccessSupported:1;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
    ULONG BootDebuggerActive:1;
#endif
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    ULONG StrongCodeGuarantees:1;
    ULONG HardStrongCodeGuarantees:1;
    ULONG SidSharingDisabled:1;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10)
    ULONG TpmInitialized:1;
    ULONG VsmConfigured:1;
    ULONG IumEnabled:1;
#endif
#if (OSVER(NTDDI_VERSION) == NTDDI_LONGHORN)
    ULONG Reserved:31;
#elif (NTDDI_VERSION == NTDDI_WIN7)
    ULONG Reserved:29;
#elif (NTDDI_VERSION == NTDDI_WIN8)
    ULONG Reserved:28;
#elif (NTDDI_VERSION == NTDDI_WINBLUE)
    ULONG Reserved:25;
#elif (NTDDI_VERSION == NTDDI_WIN10)
    ULONG Reserved:22;
#elif defined(__REACTOS__)
    ULONG BootViaWinload:1;
    ULONG BootViaEFI:1;
    ULONG Reserved:30;
#endif
/********************************/
    PLOADER_PERFORMANCE_DATA LoaderPerformanceData;
    LIST_ENTRY BootApplicationPersistentData;
    PVOID WmdTestResult;
    GUID BootIdentifier;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    ULONG ResumePages;
    PVOID DumpHeader;
    PVOID BgContext;
    PVOID NumaLocalityInfo;
    PVOID NumaGroupAssignment;
    LIST_ENTRY AttachedHives;
    ULONG MemoryCachingRequirementsCount;
    PVOID MemoryCachingRequirements;
#if (NTDDI_VERSION < NTDDI_WIN8)
    TPM_BOOT_ENTROPY_LDR_RESULT TpmBootEntropyResult;
#else
    BOOT_ENTROPY_LDR_RESULT BootEntropyResult;
#endif
    ULONGLONG ProcessorCounterFrequency;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN8)
    LOADER_PARAMETER_HYPERVISOR_EXTENSION HypervisorExtension;
    GUID HardwareConfigurationId;
    LIST_ENTRY HalExtensionModuleList;
    LARGE_INTEGER SystemTime;
    ULONGLONG TimeStampAtSystemTimeRead;
    ULONGLONG BootFlags;
    ULONGLONG InternalBootFlags;
    PVOID WfsFPData;
    ULONG WfsFPDataSize;
#if (NTDDI_VERSION < NTDDI_WINBLUE)
    PVOID KdExtension[12]; //LOADER_PARAMETER_KD_EXTENSION KdExtension;
#else
    LOADER_BUGCHECK_PARAMETERS BugcheckParameters;
    PVOID ApiSetSchema;
    ULONG ApiSetSchemaSize;
    LIST_ENTRY ApiSetSchemaExtensions;
#endif
    UNICODE_STRING AcpiBiosVersion;
    UNICODE_STRING SmbiosVersion;
    UNICODE_STRING EfiVersion;
#endif
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    DEBUG_DEVICE_DESCRIPTOR *KdDebugDevice;
    OFFLINE_CRASHDUMP_CONFIGURATION_TABLE OfflineCrashdumpConfigurationTable;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10)
    UNICODE_STRING ManufacturingProfile;
    PVOID BbtBuffer;
    ULONG64 XsaveAllowedFeatures;
    ULONG XsaveFlags;
    PVOID BootOptions;
    ULONG BootId;
    LOADER_PARAMETER_CI_EXTENSION *CodeIntegrityData;
    ULONG CodeIntegrityDataSize;
#endif
} LOADER_PARAMETER_EXTENSION, *PLOADER_PARAMETER_EXTENSION;

//
// Architecture specific Loader Parameter Blocks
//
typedef struct _IA64_LOADER_BLOCK
{
    ULONG PlaceHolder;
} IA64_LOADER_BLOCK, *PIA64_LOADER_BLOCK;

typedef struct _ALPHA_LOADER_BLOCK
{
    ULONG PlaceHolder;
} ALPHA_LOADER_BLOCK, *PALPHA_LOADER_BLOCK;

typedef struct _I386_LOADER_BLOCK
{
    PVOID CommonDataArea;
    ULONG MachineType;
    ULONG VirtualBias;
} I386_LOADER_BLOCK, *PI386_LOADER_BLOCK;

typedef struct _PPC_LOADER_BLOCK
{
    PVOID BootInfo;
    ULONG MachineType;
} PPC_LOADER_BLOCK, *PPPC_LOADER_BLOCK;

typedef struct _ARM_LOADER_BLOCK
{
#ifdef _ARM_
    ULONG InterruptStack;
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG GpBase;
    ULONG PanicStack;
    ULONG PcrPage;
    ULONG PdrPage;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;
    ULONG PcrPage2;
#else
    ULONG PlaceHolder;
#endif
} ARM_LOADER_BLOCK, *PARM_LOADER_BLOCK;

//
// Firmware information block (NT6+)
//

typedef struct _VIRTUAL_EFI_RUNTIME_SERVICES
{
    ULONG_PTR GetTime;
    ULONG_PTR SetTime;
    ULONG_PTR GetWakeupTime;
    ULONG_PTR SetWakeupTime;
    ULONG_PTR SetVirtualAddressMap;
    ULONG_PTR ConvertPointer;
    ULONG_PTR GetVariable;
    ULONG_PTR GetNextVariableName;
    ULONG_PTR SetVariable;
    ULONG_PTR GetNextHighMonotonicCount;
    ULONG_PTR ResetSystem;
    ULONG_PTR UpdateCapsule;
    ULONG_PTR QueryCapsuleCapabilities;
    ULONG_PTR QueryVariableInfo;
} VIRTUAL_EFI_RUNTIME_SERVICES, *PVIRTUAL_EFI_RUNTIME_SERVICES;

typedef struct _EFI_FIRMWARE_INFORMATION
{
    ULONG FirmwareVersion;
    PVIRTUAL_EFI_RUNTIME_SERVICES VirtualEfiRuntimeServices;
    ULONG SetVirtualAddressMapStatus;
    ULONG MissedMappingsCount;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    LIST_ENTRY FirmwareResourceList;
#endif
#if (NTDDI_VERSION >= NTDDI_WINBLUE)
    PVOID EfiMemoryMap;
    ULONG EfiMemoryMapSize;
    ULONG EfiMemoryMapDescriptorSize;
#endif
} EFI_FIRMWARE_INFORMATION, *PEFI_FIRMWARE_INFORMATION;

typedef struct _PCAT_FIRMWARE_INFORMATION
{
    ULONG PlaceHolder;
} PCAT_FIRMWARE_INFORMATION, *PPCAT_FIRMWARE_INFORMATION;

typedef struct _FIRMWARE_INFORMATION_LOADER_BLOCK
{
    ULONG FirmwareTypeEfi:1;
#if (NTDDI_VERSION < NTDDI_WIN10)
    ULONG Reserved:31;
#else
    ULONG EfiRuntimeUseIum:1;
    ULONG EfiRuntimePageProtectionEnabled:1;
    ULONG EfiRuntimePageProtectionSupported:1;
    ULONG Reserved:28;
#endif
    union
    {
        EFI_FIRMWARE_INFORMATION EfiInformation;
        PCAT_FIRMWARE_INFORMATION PcatInformation;
    } u;
} FIRMWARE_INFORMATION_LOADER_BLOCK, *PFIRMWARE_INFORMATION_LOADER_BLOCK;

//
// Loader Parameter Block
//
// See http://www.geoffchappell.com/studies/windows/km/ntoskrnl/structs/loader_parameter_block.htm
// for more details.
//
typedef struct _LOADER_PARAMETER_BLOCK
{
#if (NTDDI_VERSION >= NTDDI_WIN7)
    ULONG OsMajorVersion;
    ULONG OsMinorVersion;
    ULONG Size;
    ULONG Reserved;
#endif
    LIST_ENTRY LoadOrderListHead;
    LIST_ENTRY MemoryDescriptorListHead;
    LIST_ENTRY BootDriverListHead;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    LIST_ENTRY EarlyLaunchListHead;
    LIST_ENTRY CoreDriverListHead;
#endif
#if (NTDDI_VERSION >= NTDDI_WIN10)
    LIST_ENTRY CoreExtensionsDriverListHead;
    LIST_ENTRY TpmCoreDriverListHead;
#endif
    ULONG_PTR KernelStack;
    ULONG_PTR Prcb;
    ULONG_PTR Process;
    ULONG_PTR Thread;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    ULONG KernelStackSize;
#endif
    ULONG RegistryLength;
    PVOID RegistryBase;
    PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    PSTR ArcBootDeviceName;
    PSTR ArcHalDeviceName;
    PSTR NtBootPathName;
    PSTR NtHalPathName;
    PSTR LoadOptions;
    PNLS_DATA_BLOCK NlsData;
    PARC_DISK_INFORMATION ArcDiskInformation;
#if (NTDDI_VERSION < NTDDI_WIN8)
    PVOID OemFontFile;
#endif
#if (NTDDI_VERSION < NTDDI_WIN7)
    struct _SETUP_LOADER_BLOCK *SetupLdrBlock;
#endif
#if (NTDDI_VERSION < NTDDI_WIN2K)
    ULONG Spare1;
#else
    PLOADER_PARAMETER_EXTENSION Extension;
#endif
    union
    {
        I386_LOADER_BLOCK I386;
        ALPHA_LOADER_BLOCK Alpha;
        IA64_LOADER_BLOCK IA64;
        PPC_LOADER_BLOCK PowerPC;
        ARM_LOADER_BLOCK Arm;
    } u;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    FIRMWARE_INFORMATION_LOADER_BLOCK FirmwareInformation;
#endif
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

#endif
