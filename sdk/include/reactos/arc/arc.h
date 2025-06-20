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
    CreateReadOnly,
    SupersedeWriteOnly,
    SupersedeReadOnly,
    SupersedeReadWrite,
    OpenDirectory,
    CreateDirectory,
} OPENMODE;

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

typedef struct _TIMEINFO
{
    USHORT Year;
    USHORT Month;
    USHORT Day;
    USHORT Hour;
    USHORT Minute;
    USHORT Second;
} TIMEINFO;

typedef struct _MEMORY_DESCRIPTOR
{
    MEMORY_TYPE MemoryType;
    PFN_NUMBER BasePage;
    PFN_NUMBER PageCount;
} MEMORY_DESCRIPTOR, *PMEMORY_DESCRIPTOR;

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
    PVOID Unknown;
} ARC_DISK_SIGNATURE, *PARC_DISK_SIGNATURE;

typedef struct _ARC_DISK_INFORMATION
{
    LIST_ENTRY DiskSignatureListHead;
} ARC_DISK_INFORMATION, *PARC_DISK_INFORMATION;

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
} TPM_BOOT_ENTROPY_RESULT_CODE;

typedef struct _TPM_BOOT_ENTROPY_LDR_RESULT
{
    ULONGLONG Policy;
    TPM_BOOT_ENTROPY_RESULT_CODE ResultCode;
    LONG ResultStatus;
    ULONGLONG Time;
    ULONG EntropyLength;
    UCHAR EntropyData[40];
} TPM_BOOT_ENTROPY_LDR_RESULT, *PTPM_BOOT_ENTROPY_LDR_RESULT;

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
    ULONG MajorVersion;             /* Not anymore present starting NT 6.1 */
    ULONG MinorVersion;             /* Not anymore present starting NT 6.1 */
    PVOID EmInfFileImage;
    ULONG EmInfFileSize;
    PVOID TriageDumpBlock;
    //
    // NT 5.1
    //
    ULONG_PTR LoaderPagesSpanned;   /* Not anymore present starting NT 6.2 */
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
    PSMBIOS_TABLE_HEADER SMBiosEPSHeader;
    PVOID DrvDBImage;
    ULONG DrvDBSize;
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock;
    //
    // NT 5.2+
    //
#ifdef _X86_
    PUCHAR HalpIRQLToTPR;
    PUCHAR HalpVectorToIRQL;
#endif
    LIST_ENTRY FirmwareDescriptorListHead;
    PVOID AcpiTable;
    ULONG AcpiTableSize;
    //
    // NT 5.2 SP1+
    //
/** NT-version-dependent flags **/
    ULONG BootViaWinload:1;
    ULONG BootViaEFI:1;
    ULONG Reserved:30;
/********************************/
    PLOADER_PERFORMANCE_DATA LoaderPerformanceData;
    LIST_ENTRY BootApplicationPersistentData;
    PVOID WmdTestResult;
    GUID BootIdentifier;
    //
    // NT 6
    //
    ULONG ResumePages;
    PVOID DumpHeader;
} LOADER_PARAMETER_EXTENSION, *PLOADER_PARAMETER_EXTENSION;

typedef struct _LOADER_PARAMETER_EXTENSION_WIN7
{
    ULONG Size;
    PROFILE_PARAMETER_BLOCK Profile;
    PVOID EmInfFileImage;
    ULONG EmInfFileSize;
    PVOID TriageDumpBlock;
    //
    // NT 5.1
    //
    ULONG_PTR LoaderPagesSpanned;   /* Not anymore present starting NT 6.2 */
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
    PSMBIOS_TABLE_HEADER SMBiosEPSHeader;
    PVOID DrvDBImage;
    ULONG DrvDBSize;
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock;
    //
    // NT 5.2+
    //
#ifdef _X86_
    PUCHAR HalpIRQLToTPR;
    PUCHAR HalpVectorToIRQL;
#endif
    LIST_ENTRY FirmwareDescriptorListHead;
    PVOID AcpiTable;
    ULONG AcpiTableSize;
    //
    // NT 5.2 SP1+
    //
/** NT-version-dependent flags **/
    ULONG LastBootSucceeded:1;
    ULONG LastBootShutdown:1;
    ULONG IoPortAccessSupported:1;
    ULONG Reserved:29;
/********************************/
    PLOADER_PERFORMANCE_DATA LoaderPerformanceData;
    LIST_ENTRY BootApplicationPersistentData;
    PVOID WmdTestResult;
    GUID BootIdentifier;
    //
    // NT 6
    //
    ULONG ResumePages;
    PVOID DumpHeader;
    //
    // NT 6.1
    //
    PVOID BgContext;
    PVOID NumaLocalityInfo;
    PVOID NumaGroupAssignment;
    LIST_ENTRY AttachedHives;
    ULONG MemoryCachingRequirementsCount;
    PVOID MemoryCachingRequirements;
    TPM_BOOT_ENTROPY_LDR_RESULT TpmBootEntropyResult;
    ULONGLONG ProcessorCounterFrequency;
} LOADER_PARAMETER_EXTENSION_WIN7, *PLOADER_PARAMETER_EXTENSION_WIN7;

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
} EFI_FIRMWARE_INFORMATION, *PEFI_FIRMWARE_INFORMATION;

typedef struct _PCAT_FIRMWARE_INFORMATION
{
    ULONG PlaceHolder;
} PCAT_FIRMWARE_INFORMATION, *PPCAT_FIRMWARE_INFORMATION;

typedef struct _FIRMWARE_INFORMATION_LOADER_BLOCK
{
    ULONG FirmwareTypeEfi:1;
    ULONG Reserved:31;
    union
    {
        EFI_FIRMWARE_INFORMATION EfiInformation;
        PCAT_FIRMWARE_INFORMATION PcatInformation;
    } u;
} FIRMWARE_INFORMATION_LOADER_BLOCK, *PFIRMWARE_INFORMATION_LOADER_BLOCK;

typedef union _LOADER_ARCHITECTURE_BLOCK
{
    I386_LOADER_BLOCK I386;
    ALPHA_LOADER_BLOCK Alpha;
    IA64_LOADER_BLOCK IA64;
    PPC_LOADER_BLOCK PowerPC;
    ARM_LOADER_BLOCK Arm;
} LOADER_ARCHITECTURE_BLOCK, *PLOADER_ARCHITECTURE_BLOCK;

//
// Loader Parameter Block
//
// See http://www.geoffchappell.com/studies/windows/km/ntoskrnl/structs/loader_parameter_block.htm
// for more details.
//
typedef struct _LOADER_PARAMETER_BLOCK
{
    LIST_ENTRY LoadOrderListHead;
    LIST_ENTRY MemoryDescriptorListHead;
    LIST_ENTRY BootDriverListHead;
    ULONG_PTR KernelStack;
    ULONG_PTR Prcb;
    ULONG_PTR Process;
    ULONG_PTR Thread;
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
    PVOID OemFontFile;
    struct _SETUP_LOADER_BLOCK *SetupLdrBlock;
    PLOADER_PARAMETER_EXTENSION Extension;
    LOADER_ARCHITECTURE_BLOCK u;
    FIRMWARE_INFORMATION_LOADER_BLOCK FirmwareInformation;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

typedef struct _LOADER_PARAMETER_BLOCK_WIN7
{
    ULONG OsMajorVersion;
    ULONG OsMinorVersion;
    ULONG Size;
    ULONG Reserved;
    LIST_ENTRY LoadOrderListHead;
    LIST_ENTRY MemoryDescriptorListHead;
    LIST_ENTRY BootDriverListHead;
    ULONG_PTR KernelStack;
    ULONG_PTR Prcb;
    ULONG_PTR Process;
    ULONG_PTR Thread;
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
    PVOID OemFontFile;
    PLOADER_PARAMETER_EXTENSION_WIN7 Extension;
    LOADER_ARCHITECTURE_BLOCK u;
    FIRMWARE_INFORMATION_LOADER_BLOCK FirmwareInformation;
} LOADER_PARAMETER_BLOCK_WIN7, *PLOADER_PARAMETER_BLOCK_WIN7;

typedef int CONFIGTYPE;
typedef struct tagFILEINFORMATION
{
    LARGE_INTEGER StartingAddress;
    LARGE_INTEGER EndingAddress;
    LARGE_INTEGER CurrentAddress;
    CONFIGTYPE Type;
    ULONG FileNameLength;
    UCHAR Attributes;
    CHAR Filename[32];
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

#endif
