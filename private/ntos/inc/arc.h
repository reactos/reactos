/*++ BUILD Version: 0010    // Increment this if a change has global effects

Copyright (c) 1991  Microsoft Corporation

Module Name:

    arc.h

Abstract:

    This header file defines the ARC system firmware interface and the
    NT structures that are dependent on ARC types.

Author:

    David N. Cutler (davec) 18-May-1991


Revision History:

--*/

#ifndef _ARC_
#define _ARC_

#include "profiles.h"


//
// Define console input and console output file ids.
//

#define ARC_CONSOLE_INPUT 0
#define ARC_CONSOLE_OUTPUT 1

//
// Define ARC_STATUS type.
//

typedef ULONG ARC_STATUS;

//
// Define the firmware entry point numbers.
//

typedef enum _FIRMWARE_ENTRY {
    LoadRoutine,
    InvokeRoutine,
    ExecuteRoutine,
    HaltRoutine,
    PowerDownRoutine,
    RestartRoutine,
    RebootRoutine,
    InteractiveModeRoutine,
    Reserved1,
    GetPeerRoutine,
    GetChildRoutine,
    GetParentRoutine,
    GetDataRoutine,
    AddChildRoutine,
    DeleteComponentRoutine,
    GetComponentRoutine,
    SaveConfigurationRoutine,
    GetSystemIdRoutine,
    MemoryRoutine,
    Reserved2,
    GetTimeRoutine,
    GetRelativeTimeRoutine,
    GetDirectoryEntryRoutine,
    OpenRoutine,
    CloseRoutine,
    ReadRoutine,
    ReadStatusRoutine,
    WriteRoutine,
    SeekRoutine,
    MountRoutine,
    GetEnvironmentRoutine,
    SetEnvironmentRoutine,
    GetFileInformationRoutine,
    SetFileInformationRoutine,
    FlushAllCachesRoutine,
    TestUnicodeCharacterRoutine,
    GetDisplayStatusRoutine,
    MaximumRoutine
} FIRMWARE_ENTRY;

//
// Define software loading and execution routine types.
//

typedef
ARC_STATUS
(*PARC_EXECUTE_ROUTINE) (
    IN CHAR * FIRMWARE_PTR ImagePath,
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Envp
    );

typedef
ARC_STATUS
(*PARC_INVOKE_ROUTINE) (
    IN ULONG EntryAddress,
    IN ULONG StackAddress,
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Envp
    );

typedef
ARC_STATUS
(*PARC_LOAD_ROUTINE) (
    IN CHAR * FIRMWARE_PTR ImagePath,
    IN ULONG TopAddress,
    OUT ULONG * FIRMWARE_PTR EntryAddress,
    OUT ULONG * FIRMWARE_PTR LowAddress
    );

//
// Define firmware software loading and execution prototypes.
//

ARC_STATUS
FwExecute (
    IN CHAR * FIRMWARE_PTR ImagePath,
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Envp
    );

ARC_STATUS
FwInvoke (
    IN ULONG EntryAddress,
    IN ULONG StackAddress,
    IN ULONG Argc,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Argv,
    IN CHAR * FIRMWARE_PTR * FIRMWARE_PTR Envp
    );

ARC_STATUS
FwLoad (
    IN CHAR * FIRMWARE_PTR ImagePath,
    IN ULONG TopAddress,
    OUT ULONG * FIRMWARE_PTR EntryAddress,
    OUT ULONG * FIRMWARE_PTR LowAddress
    );

//
// Define program termination routine types.
//

typedef
VOID
(*PARC_HALT_ROUTINE) (
    VOID
    );

typedef
VOID
(*PARC_POWERDOWN_ROUTINE) (
    VOID
    );

typedef
VOID
(*PARC_RESTART_ROUTINE) (
    VOID
    );

typedef
VOID
(*PARC_REBOOT_ROUTINE) (
    VOID
    );

typedef
VOID
(*PARC_INTERACTIVE_MODE_ROUTINE) (
    VOID
    );

//
// Define firmware program termination prototypes.
//

VOID
FwHalt (
    VOID
    );

VOID
FwPowerDown (
    VOID
    );

VOID
FwRestart (
    VOID
    );

VOID
FwReboot (
    VOID
    );

VOID
FwEnterInteractiveMode (
    VOID
    );

// begin_ntddk
//
// Define configuration routine types.
//
// Configuration information.
//
// end_ntddk

typedef enum _CONFIGURATION_CLASS {
    SystemClass,
    ProcessorClass,
    CacheClass,
    AdapterClass,
    ControllerClass,
    PeripheralClass,
    MemoryClass,
    MaximumClass
} CONFIGURATION_CLASS, *PCONFIGURATION_CLASS;

// begin_ntddk

typedef enum _CONFIGURATION_TYPE {
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
    MaximumType
} CONFIGURATION_TYPE, *PCONFIGURATION_TYPE;

// end_ntddk

typedef struct _CONFIGURATION_COMPONENT {
    CONFIGURATION_CLASS Class;
    CONFIGURATION_TYPE Type;
    DEVICE_FLAGS Flags;
    USHORT Version;
    USHORT Revision;
    ULONG Key;
    ULONG AffinityMask;
    ULONG ConfigurationDataLength;
    ULONG IdentifierLength;
    CHAR * FIRMWARE_PTR Identifier;
} CONFIGURATION_COMPONENT, * FIRMWARE_PTR PCONFIGURATION_COMPONENT;

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_CHILD_ROUTINE) (
    IN PCONFIGURATION_COMPONENT Component OPTIONAL
    );

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_PARENT_ROUTINE) (
    IN PCONFIGURATION_COMPONENT Component
    );

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_PEER_ROUTINE) (
    IN PCONFIGURATION_COMPONENT Component
    );

typedef
PCONFIGURATION_COMPONENT
(*PARC_ADD_CHILD_ROUTINE) (
    IN PCONFIGURATION_COMPONENT Component,
    IN PCONFIGURATION_COMPONENT NewComponent,
    IN VOID * FIRMWARE_PTR ConfigurationData
    );

typedef
ARC_STATUS
(*PARC_DELETE_COMPONENT_ROUTINE) (
    IN PCONFIGURATION_COMPONENT Component
    );

typedef
PCONFIGURATION_COMPONENT
(*PARC_GET_COMPONENT_ROUTINE) (
    IN CHAR * FIRMWARE_PTR Path
    );

typedef
ARC_STATUS
(*PARC_GET_DATA_ROUTINE) (
    OUT VOID * FIRMWARE_PTR ConfigurationData,
    IN PCONFIGURATION_COMPONENT Component
    );

typedef
ARC_STATUS
(*PARC_SAVE_CONFIGURATION_ROUTINE) (
    VOID
    );

//
// Define firmware configuration prototypes.
//

PCONFIGURATION_COMPONENT
FwGetChild (
    IN PCONFIGURATION_COMPONENT Component OPTIONAL
    );

PCONFIGURATION_COMPONENT
FwGetParent (
    IN PCONFIGURATION_COMPONENT Component
    );

PCONFIGURATION_COMPONENT
FwGetPeer (
    IN PCONFIGURATION_COMPONENT Component
    );

PCONFIGURATION_COMPONENT
FwAddChild (
    IN PCONFIGURATION_COMPONENT Component,
    IN PCONFIGURATION_COMPONENT NewComponent,
    IN VOID * FIRMWARE_PTR ConfigurationData OPTIONAL
    );

ARC_STATUS
FwDeleteComponent (
    IN PCONFIGURATION_COMPONENT Component
    );

PCONFIGURATION_COMPONENT
FwGetComponent(
    IN CHAR * FIRMWARE_PTR Path
    );

ARC_STATUS
FwGetConfigurationData (
    OUT VOID * FIRMWARE_PTR ConfigurationData,
    IN PCONFIGURATION_COMPONENT Component
    );

ARC_STATUS
FwSaveConfiguration (
    VOID
    );

//
// System information.
//

typedef struct _SYSTEM_ID {
    CHAR VendorId[8];
    CHAR ProductId[8];
} SYSTEM_ID, * FIRMWARE_PTR PSYSTEM_ID;

typedef
PSYSTEM_ID
(*PARC_GET_SYSTEM_ID_ROUTINE) (
    VOID
    );

//
// Define system identifier query routine type.
//

PSYSTEM_ID
FwGetSystemId (
    VOID
    );

//
// Memory information.
//

typedef enum _MEMORY_TYPE {
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

typedef struct _MEMORY_DESCRIPTOR {
    MEMORY_TYPE MemoryType;
    ULONG BasePage;
    ULONG PageCount;
} MEMORY_DESCRIPTOR, * FIRMWARE_PTR PMEMORY_DESCRIPTOR;

#if defined(_IA64_)

//
// Cache Attribute.
//

#define WTU    0x1
#define WTO    0x2
#define WBO    0x4
#define WBU    0x8
#define WCU    0x10
#define UCU    0x20
#define UCUE   0x40
#define UCO    0x80

typedef enum _MEMORY_USAGE_TYPE {
    RegularMemory,
    MemoryMappedIo,
    ProcessorIoBlock,
    BootServicesCode,
    BootServicesData,
    RuntimeServicesCode,
    RuntimeServicesData,
    FirmwareSpace,
    DisplayMemory,
    CacheAttributeMaximum
} MEMORY_USAGE_TYPE;

typedef struct _CACHE_ATTRIBUTE_DESCRIPTOR {
    LIST_ENTRY ListEntry;
    MEMORY_USAGE_TYPE MemoryUsage;
    ULONG CacheAttribute;
    ULONG BasePage;
    ULONG PageCount;
} CACHE_ATTRIBUTE_DESCRIPTOR, *PCACHE_ATTRIBUTE_DESCRIPTOR;

CACHE_ATTRIBUTE_DESCRIPTOR CacheDescriptorList[10];

#endif // defined(_IA64_)

typedef
PMEMORY_DESCRIPTOR
(*PARC_MEMORY_ROUTINE) (
    IN PMEMORY_DESCRIPTOR MemoryDescriptor OPTIONAL
    );

//
// Define memory query routine type.
//

PMEMORY_DESCRIPTOR
FwGetMemoryDescriptor (
    IN PMEMORY_DESCRIPTOR MemoryDescriptor OPTIONAL
    );

//
// Query time functions.
//

typedef
PTIME_FIELDS
(*PARC_GET_TIME_ROUTINE) (
    VOID
    );

typedef
ULONG
(*PARC_GET_RELATIVE_TIME_ROUTINE) (
    VOID
    );

//
// Define query time routine types.
//

PTIME_FIELDS
FwGetTime (
    VOID
    );

ULONG
FwGetRelativeTime (
    VOID
    );

//
// Define I/O routine types.
//

#define ArcReadOnlyFile   1
#define ArcHiddenFile     2
#define ArcSystemFile     4
#define ArcArchiveFile    8
#define ArcDirectoryFile 16
#define ArcDeleteFile    32

typedef enum _OPEN_MODE {
    ArcOpenReadOnly,
    ArcOpenWriteOnly,
    ArcOpenReadWrite,
    ArcCreateWriteOnly,
    ArcCreateReadWrite,
    ArcSupersedeWriteOnly,
    ArcSupersedeReadWrite,
    ArcOpenDirectory,
    ArcCreateDirectory,
    ArcOpenMaximumMode
} OPEN_MODE;

typedef struct _FILE_INFORMATION {
    LARGE_INTEGER StartingAddress;
    LARGE_INTEGER EndingAddress;
    LARGE_INTEGER CurrentPosition;
    CONFIGURATION_TYPE Type;
    ULONG FileNameLength;
    UCHAR Attributes;
    CHAR FileName[32];
} FILE_INFORMATION, * FIRMWARE_PTR PFILE_INFORMATION;

typedef enum _SEEK_MODE {
    SeekAbsolute,
    SeekRelative,
    SeekMaximum
} SEEK_MODE;

typedef enum _MOUNT_OPERATION {
    MountLoadMedia,
    MountUnloadMedia,
    MountMaximum
} MOUNT_OPERATION;

typedef struct _DIRECTORY_ENTRY {
        ULONG FileNameLength;
        UCHAR FileAttribute;
        CHAR FileName[32];
} DIRECTORY_ENTRY, * FIRMWARE_PTR PDIRECTORY_ENTRY;

typedef
ARC_STATUS
(*PARC_CLOSE_ROUTINE) (
    IN ULONG FileId
    );

typedef
ARC_STATUS
(*PARC_MOUNT_ROUTINE) (
    IN CHAR * FIRMWARE_PTR MountPath,
    IN MOUNT_OPERATION Operation
    );

typedef
ARC_STATUS
(*PARC_OPEN_ROUTINE) (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    );

typedef
ARC_STATUS
(*PARC_READ_ROUTINE) (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

typedef
ARC_STATUS
(*PARC_READ_STATUS_ROUTINE) (
    IN ULONG FileId
    );

typedef
ARC_STATUS
(*PARC_SEEK_ROUTINE) (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    );

typedef
ARC_STATUS
(*PARC_WRITE_ROUTINE) (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

typedef
ARC_STATUS
(*PARC_GET_FILE_INFO_ROUTINE) (
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInformation
    );

typedef
ARC_STATUS
(*PARC_SET_FILE_INFO_ROUTINE) (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

typedef
ARC_STATUS
(*PARC_GET_DIRECTORY_ENTRY_ROUTINE) (
    IN ULONG FileId,
    OUT PDIRECTORY_ENTRY Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

//
// Define firmware I/O prototypes.
//

ARC_STATUS
FwClose (
    IN ULONG FileId
    );

ARC_STATUS
FwMount (
    IN CHAR * FIRMWARE_PTR MountPath,
    IN MOUNT_OPERATION Operation
    );

ARC_STATUS
FwOpen (
    IN CHAR * FIRMWARE_PTR OpenPath,
    IN OPEN_MODE OpenMode,
    OUT ULONG * FIRMWARE_PTR FileId
    );

ARC_STATUS
FwRead (
    IN ULONG FileId,
    OUT VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
FwGetReadStatus (
    IN ULONG FileId
    );

ARC_STATUS
FwSeek (
    IN ULONG FileId,
    IN LARGE_INTEGER * FIRMWARE_PTR Offset,
    IN SEEK_MODE SeekMode
    );

ARC_STATUS
FwWrite (
    IN ULONG FileId,
    IN VOID * FIRMWARE_PTR Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );

ARC_STATUS
FwGetFileInformation (
    IN ULONG FileId,
    OUT PFILE_INFORMATION FileInformation
    );

ARC_STATUS
FwSetFileInformation (
    IN ULONG FileId,
    IN ULONG AttributeFlags,
    IN ULONG AttributeMask
    );

ARC_STATUS
FwGetDirectoryEntry (
    IN ULONG FileId,
    OUT PDIRECTORY_ENTRY Buffer,
    IN ULONG Length,
    OUT ULONG * FIRMWARE_PTR Count
    );


//
// Define environment routine types.
//

typedef
CHAR * FIRMWARE_PTR
(*PARC_GET_ENVIRONMENT_ROUTINE) (
    IN CHAR * FIRMWARE_PTR Variable
    );

typedef
ARC_STATUS
(*PARC_SET_ENVIRONMENT_ROUTINE) (
    IN CHAR * FIRMWARE_PTR Variable,
    IN CHAR * FIRMWARE_PTR Value
    );

//
// Define firmware environment prototypes.
//

CHAR * FIRMWARE_PTR
FwGetEnvironmentVariable (
    IN CHAR * FIRMWARE_PTR Variable
    );

ARC_STATUS
FwSetEnvironmentVariable (
    IN CHAR * FIRMWARE_PTR Variable,
    IN CHAR * FIRMWARE_PTR Value
    );

//
// Define inline functions to acquire and release the firmware lock
// and the stub function prototypes necessary to interface with the
// 32-bit firmware on 64-bit systems.
//
// These routines are required for the 64-bit system until (if) 64-bit
// firmware is ever supplied.
//
#if defined(_AXP64_) && defined(_NTHAL_)

extern KSPIN_LOCK HalpFirmwareLock;

CHAR * FIRMWARE_PTR
HalpArcGetEnvironmentVariable(
    IN PCHAR Variable
    );

ARC_STATUS
HalpArcSetEnvironmentVariable(
    IN PCHAR Variable,
    IN PCHAR Value
    );

KIRQL
FwAcquireFirmwareLock(
    VOID
    );

VOID
FwReleaseFirmwareLock(
    IN KIRQL OldIrql
    );

#endif // _AXP64_  && defined(_NTHAL_)


//
// Define cache flush routine types
//

typedef
VOID
(*PARC_FLUSH_ALL_CACHES_ROUTINE) (
    VOID
    );

//
// Define firmware cache flush prototypes.
//

VOID
FwFlushAllCaches (
    VOID
    );

//
// Define TestUnicodeCharacter and GetDisplayStatus routines.
//

typedef struct _ARC_DISPLAY_STATUS {
    USHORT CursorXPosition;
    USHORT CursorYPosition;
    USHORT CursorMaxXPosition;
    USHORT CursorMaxYPosition;
    UCHAR ForegroundColor;
    UCHAR BackgroundColor;
    BOOLEAN HighIntensity;
    BOOLEAN Underscored;
    BOOLEAN ReverseVideo;
} ARC_DISPLAY_STATUS, * FIRMWARE_PTR PARC_DISPLAY_STATUS;

typedef
ARC_STATUS
(*PARC_TEST_UNICODE_CHARACTER_ROUTINE) (
    IN ULONG FileId,
    IN WCHAR UnicodeCharacter
    );

typedef
PARC_DISPLAY_STATUS
(*PARC_GET_DISPLAY_STATUS_ROUTINE) (
    IN ULONG FileId
    );

ARC_STATUS
FwTestUnicodeCharacter(
    IN ULONG FileId,
    IN WCHAR UnicodeCharacter
    );

PARC_DISPLAY_STATUS
FwGetDisplayStatus(
    IN ULONG FileId
    );


//
// Define low memory data structures.
//
// Define debug block structure.
//

typedef struct _DEBUG_BLOCK {
    ULONG Signature;
    ULONG Length;
} DEBUG_BLOCK, * FIRMWARE_PTR PDEBUG_BLOCK;

//
// Define restart block structure.
//

#define ARC_RESTART_BLOCK_SIGNATURE 0x42545352

typedef struct _BOOT_STATUS {
    ULONG BootStarted : 1;
    ULONG BootFinished : 1;
    ULONG RestartStarted : 1;
    ULONG RestartFinished : 1;
    ULONG PowerFailStarted : 1;
    ULONG PowerFailFinished : 1;
    ULONG ProcessorReady : 1;
    ULONG ProcessorRunning : 1;
    ULONG ProcessorStart : 1;
} BOOT_STATUS, * FIRMWARE_PTR PBOOT_STATUS;

typedef struct _ALPHA_RESTART_STATE {

#if defined(_ALPHA_) || defined(_AXP64_)

    //
    // Control information
    //

    ULONG HaltReason;
    VOID * FIRMWARE_PTR LogoutFrame;
    ULONGLONG PalBase;

    //
    // Integer Save State
    //

    ULONGLONG IntV0;
    ULONGLONG IntT0;
    ULONGLONG IntT1;
    ULONGLONG IntT2;
    ULONGLONG IntT3;
    ULONGLONG IntT4;
    ULONGLONG IntT5;
    ULONGLONG IntT6;
    ULONGLONG IntT7;
    ULONGLONG IntS0;
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntS4;
    ULONGLONG IntS5;
    ULONGLONG IntFp;
    ULONGLONG IntA0;
    ULONGLONG IntA1;
    ULONGLONG IntA2;
    ULONGLONG IntA3;
    ULONGLONG IntA4;
    ULONGLONG IntA5;
    ULONGLONG IntT8;
    ULONGLONG IntT9;
    ULONGLONG IntT10;
    ULONGLONG IntT11;
    ULONGLONG IntRa;
    ULONGLONG IntT12;
    ULONGLONG IntAT;
    ULONGLONG IntGp;
    ULONGLONG IntSp;
    ULONGLONG IntZero;

    //
    // Floating Point Save State
    //

    ULONGLONG Fpcr;
    ULONGLONG FltF0;
    ULONGLONG FltF1;
    ULONGLONG FltF2;
    ULONGLONG FltF3;
    ULONGLONG FltF4;
    ULONGLONG FltF5;
    ULONGLONG FltF6;
    ULONGLONG FltF7;
    ULONGLONG FltF8;
    ULONGLONG FltF9;
    ULONGLONG FltF10;
    ULONGLONG FltF11;
    ULONGLONG FltF12;
    ULONGLONG FltF13;
    ULONGLONG FltF14;
    ULONGLONG FltF15;
    ULONGLONG FltF16;
    ULONGLONG FltF17;
    ULONGLONG FltF18;
    ULONGLONG FltF19;
    ULONGLONG FltF20;
    ULONGLONG FltF21;
    ULONGLONG FltF22;
    ULONGLONG FltF23;
    ULONGLONG FltF24;
    ULONGLONG FltF25;
    ULONGLONG FltF26;
    ULONGLONG FltF27;
    ULONGLONG FltF28;
    ULONGLONG FltF29;
    ULONGLONG FltF30;
    ULONGLONG FltF31;

    //
    // Architected Internal Processor State.
    //

    ULONG Asn;
    VOID * FIRMWARE_PTR GeneralEntry;
    VOID * FIRMWARE_PTR Iksp;
    VOID * FIRMWARE_PTR InterruptEntry;
    VOID * FIRMWARE_PTR Kgp;
    ULONG Mces;
    VOID * FIRMWARE_PTR MemMgmtEntry;
    VOID * FIRMWARE_PTR PanicEntry;
    VOID * FIRMWARE_PTR Pcr;
    VOID * FIRMWARE_PTR Pdr;
    ULONG Psr;
    VOID * FIRMWARE_PTR ReiRestartAddress;
    ULONG Sirr;
    VOID * FIRMWARE_PTR SyscallEntry;
    VOID * FIRMWARE_PTR Teb;
    VOID * FIRMWARE_PTR Thread;

    //
    // Processor Implementation-dependent State.
    //

    ULONGLONG PerProcessorState[175];   // allocate 2K maximum restart block

#else

    ULONG PlaceHolder;

#endif

} ALPHA_RESTART_STATE, * FIRMWARE_PTR PALPHA_RESTART_STATE;

typedef struct _I386_RESTART_STATE {

#if defined(_X86_)

    //
    // Put state structure here.
    //

    ULONG PlaceHolder;

#else

    ULONG PlaceHolder;

#endif

} I386_RESTART_STATE, *PI386_RESTART_STATE;

#if defined(_IA64_)
#include "pshpck16.h"
#endif


typedef struct _IA64_RESTART_STATE {

#if defined(_IA64_)

// @@BEGIN_DDKSPLIT

    //
    // This structure is copied from CONTEXT structure in sdk/ntia64.h.
    //

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a thread's context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    ULONG ContextFlags;
    ULONG Fill1[3];         // for alignment of following on 16-byte boundary

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_DEBUG.
    //
    // N.B. CONTEXT_DEBUG is *not* part of CONTEXT_FULL.
    //

    ULONGLONG DbI0;         // Instruction debug registers
    ULONGLONG DbI1;
    ULONGLONG DbI2;
    ULONGLONG DbI3;
    ULONGLONG DbI4;
    ULONGLONG DbI5;
    ULONGLONG DbI6;
    ULONGLONG DbI7;

    ULONGLONG DbD0;         // Data debug registers
    ULONGLONG DbD1;
    ULONGLONG DbD2;
    ULONGLONG DbD3;
    ULONGLONG DbD4;
    ULONGLONG DbD5;
    ULONGLONG DbD6;
    ULONGLONG DbD7;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT.
    //

    FLOAT128 FltS0;                         // Lower saved (preserved)
    FLOAT128 FltS1;
    FLOAT128 FltS2;
    FLOAT128 FltS3;
    FLOAT128 FltS4;
    FLOAT128 FltS5;
    FLOAT128 FltT0;                         // Lower temporary (scratch)
    FLOAT128 FltT1;
    FLOAT128 FltT2;
    FLOAT128 FltT3;
    FLOAT128 FltT4;
    FLOAT128 FltT5;
    FLOAT128 FltT6;
    FLOAT128 FltT7;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_HIGHER_FLOATING_POINT.
    //

    FLOAT128 FltS6;                         // Higher saved (preserved) floats
    FLOAT128 FltS7;
    FLOAT128 FltS8;
    FLOAT128 FltS9;
    FLOAT128 FltS10;
    FLOAT128 FltS11;
    FLOAT128 FltS12;
    FLOAT128 FltS13;
    FLOAT128 FltS14;
    FLOAT128 FltS15;
    FLOAT128 FltS16;
    FLOAT128 FltS17;
    FLOAT128 FltS18;
    FLOAT128 FltS19;
    FLOAT128 FltS20;
    FLOAT128 FltS21;

    FLOAT128 FltF32;
    FLOAT128 FltF33;
    FLOAT128 FltF34;
    FLOAT128 FltF35;
    FLOAT128 FltF36;
    FLOAT128 FltF37;
    FLOAT128 FltF38;
    FLOAT128 FltF39;

    FLOAT128 FltF40;
    FLOAT128 FltF41;
    FLOAT128 FltF42;
    FLOAT128 FltF43;
    FLOAT128 FltF44;
    FLOAT128 FltF45;
    FLOAT128 FltF46;
    FLOAT128 FltF47;
    FLOAT128 FltF48;
    FLOAT128 FltF49;

    FLOAT128 FltF50;
    FLOAT128 FltF51;
    FLOAT128 FltF52;
    FLOAT128 FltF53;
    FLOAT128 FltF54;
    FLOAT128 FltF55;
    FLOAT128 FltF56;
    FLOAT128 FltF57;
    FLOAT128 FltF58;
    FLOAT128 FltF59;

    FLOAT128 FltF60;
    FLOAT128 FltF61;
    FLOAT128 FltF62;
    FLOAT128 FltF63;
    FLOAT128 FltF64;
    FLOAT128 FltF65;
    FLOAT128 FltF66;
    FLOAT128 FltF67;
    FLOAT128 FltF68;
    FLOAT128 FltF69;

    FLOAT128 FltF70;
    FLOAT128 FltF71;
    FLOAT128 FltF72;
    FLOAT128 FltF73;
    FLOAT128 FltF74;
    FLOAT128 FltF75;
    FLOAT128 FltF76;
    FLOAT128 FltF77;
    FLOAT128 FltF78;
    FLOAT128 FltF79;

    FLOAT128 FltF80;
    FLOAT128 FltF81;
    FLOAT128 FltF82;
    FLOAT128 FltF83;
    FLOAT128 FltF84;
    FLOAT128 FltF85;
    FLOAT128 FltF86;
    FLOAT128 FltF87;
    FLOAT128 FltF88;
    FLOAT128 FltF89;

    FLOAT128 FltF90;
    FLOAT128 FltF91;
    FLOAT128 FltF92;
    FLOAT128 FltF93;
    FLOAT128 FltF94;
    FLOAT128 FltF95;
    FLOAT128 FltF96;
    FLOAT128 FltF97;
    FLOAT128 FltF98;
    FLOAT128 FltF99;

    FLOAT128 FltF100;
    FLOAT128 FltF101;
    FLOAT128 FltF102;
    FLOAT128 FltF103;
    FLOAT128 FltF104;
    FLOAT128 FltF105;
    FLOAT128 FltF106;
    FLOAT128 FltF107;
    FLOAT128 FltF108;
    FLOAT128 FltF109;

    FLOAT128 FltF110;
    FLOAT128 FltF111;
    FLOAT128 FltF112;
    FLOAT128 FltF113;
    FLOAT128 FltF114;
    FLOAT128 FltF115;
    FLOAT128 FltF116;
    FLOAT128 FltF117;
    FLOAT128 FltF118;
    FLOAT128 FltF119;

    FLOAT128 FltF120;
    FLOAT128 FltF121;
    FLOAT128 FltF122;
    FLOAT128 FltF123;
    FLOAT128 FltF124;
    FLOAT128 FltF125;
    FLOAT128 FltF126;
    FLOAT128 FltF127;

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_LOWER_FLOATING_POINT | CONTEXT_HIGHER_FLOATING_POINT.
    // **** TBD **** in some cases it may more efficient to return with
    // CONTEXT_CONTROL
    //

    ULONGLONG StFPSR;   // FP status

    //
    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_INTEGER.
    //
    // N.B. The registers gp, sp, rp are part of the control context
    //

    ULONGLONG IntGp;                        // global pointer (r1)
    ULONGLONG IntT0;
    ULONGLONG IntT1;
    ULONGLONG IntS0;
    ULONGLONG IntS1;
    ULONGLONG IntS2;
    ULONGLONG IntS3;
    ULONGLONG IntV0;                        // return value (r8)
    ULONGLONG IntAp;                        // argument pointer (r9)
    ULONGLONG IntT2;
    ULONGLONG IntT3;
    ULONGLONG IntSp;                        // stack pointer (r12)
    ULONGLONG IntT4;
    ULONGLONG IntT5;
    ULONGLONG IntT6;
    ULONGLONG IntT7;
    ULONGLONG IntT8;
    ULONGLONG IntT9;
    ULONGLONG IntT10;
    ULONGLONG IntT11;
    ULONGLONG IntT12;
    ULONGLONG IntT13;
    ULONGLONG IntT14;
    ULONGLONG IntT15;
    ULONGLONG IntT16;
    ULONGLONG IntT17;
    ULONGLONG IntT18;
    ULONGLONG IntT19;
    ULONGLONG IntT20;
    ULONGLONG IntT21;
    ULONGLONG IntT22;

    ULONGLONG IntNats;                      // Nat bits for general registers
                                            // r1-r31 in bit positions 1 to 31.
    ULONGLONG Preds;                        // Predicates

    ULONGLONG BrS0l;                        // Branch registers
    ULONGLONG BrS0h;
    ULONGLONG BrS1l;
    ULONGLONG BrS1h;
    ULONGLONG BrS2l;
    ULONGLONG BrS2h;
    ULONGLONG BrS3l;
    ULONGLONG BrS3h;
    ULONGLONG BrS4l;
    ULONGLONG BrS4h;
    ULONGLONG BrT0l;
    ULONGLONG BrT0h;
    ULONGLONG BrT1l;
    ULONGLONG BrT1h;
    ULONGLONG BrRpl;                        // return pointer
    ULONGLONG BrRph;
    ULONGLONG BrT2l;
    ULONGLONG BrT2h;
    ULONGLONG BrT3l;
    ULONGLONG BrT3h;
    ULONGLONG BrT4l;
    ULONGLONG BrT4h;
    ULONGLONG BrT5l;
    ULONGLONG BrT5h;
    ULONGLONG BrT6l;
    ULONGLONG BrT6h;
    ULONGLONG BrT7l;
    ULONGLONG BrT7h;
    ULONGLONG BrT8l;
    ULONGLONG BrT8h;
    ULONGLONG BrT9l;
    ULONGLONG BrT9h;

    // This section is specified/returned if the ContextFlags word contains
    // the flag CONTEXT_CONTROL.
    //

    // Other application registers
    ULONGLONG ApSunatcr;          // User Nat collection register (preserved)
    ULONGLONG ApSlc;              // Loop counter register (preserved)
    ULONGLONG ApTccv;             // CMPXCHG value register (volatile)

    ULONGLONG ApDCR;

    // Register stack info
    ULONGLONG RsPFS;
    ULONGLONG RsBSP;
    ULONGLONG RsBSPStore;
    ULONGLONG RsRSC;
    ULONGLONG RsRNAT;

    // Trap Status Information
    ULONGLONG StIPSR;   // Interrupt Processor Status
    ULONGLONG StIIP;    // Interrupt IP
    ULONGLONG StIFS;    // Interrupt Frame Marker

    ULONGLONG Fill;     // padding for 16-byte alignment on stack, if needed

// @@END_DDKSPLIT

#else

    ULONG PlaceHolder;

#endif // defined(_IA64_)

} IA64_RESTART_STATE, *PIA64_RESTART_STATE;



typedef struct _RESTART_BLOCK {
    ULONG Signature;
    ULONG Length;
    USHORT Version;
    USHORT Revision;
    struct _RESTART_BLOCK * FIRMWARE_PTR NextRestartBlock;
    VOID * FIRMWARE_PTR RestartAddress;
    ULONG BootMasterId;
    ULONG ProcessorId;
    volatile BOOT_STATUS BootStatus;
    ULONG CheckSum;
    ULONG SaveAreaLength;
    union {
        ULONG SaveArea[1];
        ALPHA_RESTART_STATE Alpha;
        I386_RESTART_STATE I386;
        IA64_RESTART_STATE Ia64;
    } u;

} RESTART_BLOCK, * FIRMWARE_PTR PRESTART_BLOCK;

#if defined(_IA64_)
#include "poppack.h"
#endif

//
// Define system parameter block structure.
//

typedef struct _SYSTEM_PARAMETER_BLOCK {
    ULONG Signature;
    ULONG Length;
    USHORT Version;
    USHORT Revision;
    PRESTART_BLOCK RestartBlock;
    PDEBUG_BLOCK DebugBlock;
    VOID * FIRMWARE_PTR GenerateExceptionVector;
    VOID * FIRMWARE_PTR TlbMissExceptionVector;
    ULONG FirmwareVectorLength;
    VOID * FIRMWARE_PTR * FIRMWARE_PTR FirmwareVector;
    ULONG VendorVectorLength;
    VOID * FIRMWARE_PTR * FIRMWARE_PTR VendorVector;
    ULONG AdapterCount;
    ULONG Adapter0Type;
    ULONG Adapter0Length;
    VOID * FIRMWARE_PTR * FIRMWARE_PTR Adapter0Vector;
} SYSTEM_PARAMETER_BLOCK, * FIRMWARE_PTR PSYSTEM_PARAMETER_BLOCK;

//
// Define macros that call firmware routines indirectly through the firmware
// vector and provide type checking of argument values.
//

#if defined(_ALPHA_) || defined(_AXP64_)

#define SYSTEM_BLOCK ((SYSTEM_PARAMETER_BLOCK *)(KSEG0_BASE | 0x6FE000))

#elif defined(_IA64_)

extern SYSTEM_PARAMETER_BLOCK GlobalSystemBlock;

#define SYSTEM_BLOCK (&GlobalSystemBlock)

#elif defined(_X86_)

#if defined(ARCI386)
#define SYSTEM_BLOCK ((PSYSTEM_PARAMETER_BLOCK)(0x1000))

#else
extern SYSTEM_PARAMETER_BLOCK GlobalSystemBlock;

#define SYSTEM_BLOCK (&GlobalSystemBlock)

#endif

#endif

//
// Define software loading and execution functions.
//

#define ArcExecute(ImagePath, Argc, Argv, Envp) \
    ((PARC_EXECUTE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[ExecuteRoutine])) \
        ((ImagePath), (Argc), (Argv), (Envp))

#define ArcInvoke(EntryAddress, StackAddress, Argc, Argv, Envp) \
    ((PARC_INVOKE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[InvokeRoutine])) \
        ((EntryAddress), (StackAddress), (Argc), (Argv), (Envp))

#define ArcLoad(ImagePath, TopAddress, EntryAddress, LowAddress) \
    ((PARC_LOAD_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[LoadRoutine])) \
        ((ImagePath), (TopAddress), (EntryAddress), (LowAddress))

//
// Define program termination functions.
//

#define ArcHalt() \
    ((PARC_HALT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[HaltRoutine]))()

#define ArcPowerDown() \
    ((PARC_POWERDOWN_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[PowerDownRoutine]))()

#define ArcRestart() \
    ((PARC_RESTART_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[RestartRoutine]))()

#define ArcReboot() \
    ((PARC_REBOOT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[RebootRoutine]))()

#define ArcEnterInteractiveMode() \
    ((PARC_INTERACTIVE_MODE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[InteractiveModeRoutine]))()

//
// Define configuration functions.
//

#define ArcGetChild(Component) \
    ((PARC_GET_CHILD_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetChildRoutine])) \
        ((Component))

#define ArcGetParent(Component) \
    ((PARC_GET_PARENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetParentRoutine])) \
        ((Component))

#define ArcGetPeer(Component) \
    ((PARC_GET_PEER_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetPeerRoutine])) \
        ((Component))

#define ArcAddChild(Component, NewComponent, ConfigurationData) \
    ((PARC_ADD_CHILD_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[AddChildRoutine])) \
        ((Component), (NewComponent), (ConfigurationData))

#define ArcDeleteComponent(Component) \
    ((PARC_DELETE_COMPONENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[DeleteComponentRoutine])) \
        ((Component))

#define ArcGetComponent(Path) \
    ((PARC_GET_COMPONENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetComponentRoutine])) \
        ((Path))

#define ArcGetConfigurationData(ConfigurationData, Component) \
    ((PARC_GET_DATA_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetDataRoutine])) \
        ((ConfigurationData), (Component))

#define ArcSaveConfiguration() \
    ((PARC_SAVE_CONFIGURATION_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SaveConfigurationRoutine]))()

#define ArcGetSystemId() \
    ((PARC_GET_SYSTEM_ID_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetSystemIdRoutine]))()

#define ArcGetMemoryDescriptor(MemoryDescriptor) \
    ((PARC_MEMORY_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[MemoryRoutine])) \
        ((MemoryDescriptor))

#define ArcGetTime() \
    ((PARC_GET_TIME_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetTimeRoutine]))()

#define ArcGetRelativeTime() \
    ((PARC_GET_RELATIVE_TIME_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetRelativeTimeRoutine]))()

//
// Define I/O functions.
//

#define ArcClose(FileId) \
    ((PARC_CLOSE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[CloseRoutine])) \
        ((FileId))

#define ArcGetReadStatus(FileId) \
    ((PARC_READ_STATUS_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[ReadStatusRoutine])) \
        ((FileId))

#define ArcMount(MountPath, Operation) \
    ((PARC_MOUNT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[MountRoutine])) \
        ((MountPath), (Operation))

#define ArcOpen(OpenPath, OpenMode, FileId) \
    ((PARC_OPEN_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[OpenRoutine])) \
        ((OpenPath), (OpenMode), (FileId))

#define ArcRead(FileId, Buffer, Length, Count) \
    ((PARC_READ_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[ReadRoutine])) \
        ((FileId), (Buffer), (Length), (Count))

#define ArcSeek(FileId, Offset, SeekMode) \
    ((PARC_SEEK_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SeekRoutine])) \
        ((FileId), (Offset), (SeekMode))

#define ArcWrite(FileId, Buffer, Length, Count) \
    ((PARC_WRITE_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[WriteRoutine])) \
        ((FileId), (Buffer), (Length), (Count))

#define ArcGetFileInformation(FileId, FileInformation) \
    ((PARC_GET_FILE_INFO_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetFileInformationRoutine])) \
        ((FileId), (FileInformation))

#define ArcSetFileInformation(FileId, AttributeFlags, AttributeMask) \
    ((PARC_SET_FILE_INFO_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SetFileInformationRoutine])) \
        ((FileId), (AttributeFlags), (AttributeMask))

#define ArcGetDirectoryEntry(FileId, Buffer, Length, Count) \
    ((PARC_GET_DIRECTORY_ENTRY_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetDirectoryEntryRoutine])) \
        ((FileId), (Buffer), (Length), (Count))


//
// Define environment functions.
//
#if defined(_AXP64_) && defined(_NTHAL_)

__inline
CHAR * FIRMWARE_PTR
ArcGetEnvironmentVariable(
    IN PCHAR Variable
    )
{
    CHAR * FIRMWARE_PTR FwPtr;
    KIRQL OldIrql = FwAcquireFirmwareLock();
    FwPtr = HalpArcGetEnvironmentVariable(Variable);
    FwReleaseFirmwareLock(OldIrql);
    return FwPtr;
}
#else

#define ArcGetEnvironmentVariable(Variable) \
    ((PARC_GET_ENVIRONMENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetEnvironmentRoutine])) \
        ((Variable))
#endif // _AXP64_  && defined(_NTHAL_)

#if defined(_AXP64_) && defined(_NTHAL_)

__inline
ARC_STATUS
ArcSetEnvironmentVariable(
    IN PCHAR Variable,
    IN PCHAR Value
    )
{
    ARC_STATUS ArcStatus;
    KIRQL OldIrql = FwAcquireFirmwareLock();
    ArcStatus = HalpArcSetEnvironmentVariable(Variable, Value);
    FwReleaseFirmwareLock(OldIrql);
    return ArcStatus;
}
#else

#define ArcSetEnvironmentVariable(Variable, Value) \
    ((PARC_SET_ENVIRONMENT_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[SetEnvironmentRoutine])) \
        ((Variable), (Value))
#endif // _AXP64_ && defined(_NTHAL_)


//
// Define cache flush functions.
//

#define ArcFlushAllCaches() \
    ((PARC_FLUSH_ALL_CACHES_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[FlushAllCachesRoutine]))()

//
// Define TestUnicodeCharacter and GetDisplayStatus functions.
//

#define ArcTestUnicodeCharacter(FileId, UnicodeCharacter) \
    ((PARC_TEST_UNICODE_CHARACTER_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[TestUnicodeCharacterRoutine])) \
        ((FileId), (UnicodeCharacter))

#define ArcGetDisplayStatus(FileId) \
    ((PARC_GET_DISPLAY_STATUS_ROUTINE)(SYSTEM_BLOCK->FirmwareVector[GetDisplayStatusRoutine])) \
        ((FileId))


//
// Define configuration data structure used in all systems.
//

typedef struct _CONFIGURATION_COMPONENT_DATA {
    struct _CONFIGURATION_COMPONENT_DATA *Parent;
    struct _CONFIGURATION_COMPONENT_DATA *Child;
    struct _CONFIGURATION_COMPONENT_DATA *Sibling;
    CONFIGURATION_COMPONENT ComponentEntry;
    PVOID ConfigurationData;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;

//
// Define generic display configuration data structure.
//

typedef struct _MONITOR_CONFIGURATION_DATA {
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

//
// Define generic floppy configuration data structure.
//

typedef struct _FLOPPY_CONFIGURATION_DATA {
    USHORT Version;
    USHORT Revision;
    CHAR Size[8];
    ULONG MaxDensity;
    ULONG MountDensity;
} FLOPPY_CONFIGURATION_DATA, *PFLOPPY_CONFIGURATION_DATA;

//
// Define memory allocation structures used in all systems.
//

typedef enum _TYPE_OF_MEMORY {
    LoaderExceptionBlock = MemoryExceptionBlock,            //  0
    LoaderSystemBlock = MemorySystemBlock,                  //  1
    LoaderFree = MemoryFree,                                //  2
    LoaderBad = MemoryBad,                                  //  3
    LoaderLoadedProgram = MemoryLoadedProgram,              //  4
    LoaderFirmwareTemporary = MemoryFirmwareTemporary,      //  5
    LoaderFirmwarePermanent = MemoryFirmwarePermanent,      //  6
    LoaderOsloaderHeap,                                     //  7
    LoaderOsloaderStack,                                    //  8
    LoaderSystemCode,                                       //  9
    LoaderHalCode,                                          //  a
    LoaderBootDriver,                                       //  b
    LoaderConsoleInDriver,                                  //  c
    LoaderConsoleOutDriver,                                 //  d
    LoaderStartupDpcStack,                                  //  e
    LoaderStartupKernelStack,                               //  f
    LoaderStartupPanicStack,                                // 10
    LoaderStartupPcrPage,                                   // 11
    LoaderStartupPdrPage,                                   // 12
    LoaderRegistryData,                                     // 13
    LoaderMemoryData,                                       // 14
    LoaderNlsData,                                          // 15
    LoaderSpecialMemory,                                    // 16
    LoaderBBTMemory,                                        // 17
    LoaderMaximum                                           // 18
} TYPE_OF_MEMORY;

typedef struct _MEMORY_ALLOCATION_DESCRIPTOR {
    LIST_ENTRY ListEntry;
    TYPE_OF_MEMORY MemoryType;
    ULONG BasePage;
    ULONG PageCount;
} MEMORY_ALLOCATION_DESCRIPTOR, *PMEMORY_ALLOCATION_DESCRIPTOR;


//
// Define loader parameter block structure.
//

typedef struct _NLS_DATA_BLOCK {
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCaseTableData;
} NLS_DATA_BLOCK, *PNLS_DATA_BLOCK;

typedef struct _ARC_DISK_SIGNATURE {
    LIST_ENTRY ListEntry;
    ULONG   Signature;
    PCHAR   ArcName;
    ULONG   CheckSum;
    BOOLEAN ValidPartitionTable;
    BOOLEAN xInt13;
} ARC_DISK_SIGNATURE, *PARC_DISK_SIGNATURE;

typedef struct _ARC_DISK_INFORMATION {
    LIST_ENTRY DiskSignatures;
} ARC_DISK_INFORMATION, *PARC_DISK_INFORMATION;

typedef struct _I386_LOADER_BLOCK {

#if defined(_X86_)

    PVOID CommonDataArea;
    ULONG MachineType;      // Temporary only
    ULONG VirtualBias;

#else

    ULONG PlaceHolder;

#endif

} I386_LOADER_BLOCK, *PI386_LOADER_BLOCK;

typedef struct _ALPHA_LOADER_BLOCK {

#if defined(_ALPHA_) || defined(_AXP64_)

    ULONG_PTR DpcStack;
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG_PTR GpBase;
    ULONG_PTR PanicStack;
    ULONG PcrPage;
    ULONG PdrPage;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;
    ULONG PhysicalAddressBits;
    ULONG MaximumAddressSpaceNumber;
    UCHAR SystemSerialNumber[16];
    UCHAR SystemType[8];
    ULONG SystemVariant;
    ULONG SystemRevision;
    ULONG ProcessorType;
    ULONG ProcessorRevision;
    ULONG CycleClockPeriod;
    ULONG PageSize;
    PVOID RestartBlock;
    ULONGLONG FirmwareRestartAddress;
    ULONG FirmwareRevisionId;
    PVOID PalBaseAddress;
    UCHAR FirmwareVersion[16];
    UCHAR FirmwareBuildTimeStamp[20];

#else

    ULONG PlaceHolder;

#endif

} ALPHA_LOADER_BLOCK, *PALPHA_LOADER_BLOCK;

#if defined(_IA64_)
typedef struct _TR_INFO {
    ULONG Index;
    ULONG PageSize;
    ULONGLONG VirtualAddress;
    ULONGLONG PhysicalAddress;
} TR_INFO, *PTR_INFO;
#endif

typedef struct _IA64_LOADER_BLOCK {

#if defined(_IA64_)

    ULONG_PTR SalSystemTable;
    ULONG_PTR MPSConfigTable;
    ULONG_PTR AcpiRsdt;
    ULONG_PTR KernelPhysicalBase;
    ULONG_PTR KernelVirtualBase;
    ULONG_PTR InterruptStack;
    ULONG_PTR PanicStack;
    ULONG_PTR PcrPage;
    ULONG_PTR PdrPage;
    ULONG_PTR PcrPage2;
    ULONG MachineType;
    TR_INFO ItrInfo[8];
    TR_INFO DtrInfo[8];
    ULONG_PTR EfiSystemTable;
    ULONG_PTR PalProcVirtual;

#else

    ULONG PlaceHolder;

#endif

} IA64_LOADER_BLOCK, *PIA64_LOADER_BLOCK;

typedef struct _LOADER_PARAMETER_EXTENSION {
    ULONG   Size; // set to sizeof (struct _LOADER_PARAMETER_EXTENSION)
    PROFILE_PARAMETER_BLOCK Profile;
    ULONG   MajorVersion;
    ULONG   MinorVersion;
    PVOID   InfFileImage;   // Inf used to identify "broken" machines.
    ULONG   InfFileSize;

    //
    // Pointer to the triage block, if present.
    //
    
    PVOID TriageDumpBlock;

    
} LOADER_PARAMETER_EXTENSION, *PLOADER_PARAMETER_EXTENSION;

struct _SETUP_LOADER_BLOCK;

typedef struct _LOADER_PARAMETER_BLOCK {
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
    PCHAR ArcBootDeviceName;
    PCHAR ArcHalDeviceName;
    PCHAR NtBootPathName;
    PCHAR NtHalPathName;
    PCHAR LoadOptions;
    PNLS_DATA_BLOCK NlsData;
    PARC_DISK_INFORMATION ArcDiskInformation;
    PVOID OemFontFile;
    struct _SETUP_LOADER_BLOCK *SetupLoaderBlock;
    PLOADER_PARAMETER_EXTENSION Extension;

    union {
        I386_LOADER_BLOCK I386;
        ALPHA_LOADER_BLOCK Alpha;
        IA64_LOADER_BLOCK Ia64;
    } u;

} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

#endif // _ARC_

