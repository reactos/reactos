/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    haltypes.h

Abstract:

    Type definitions for the HAL.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _HALTYPES_H
#define _HALTYPES_H

//
// Dependencies
//
#include <umtypes.h>

#ifndef NTOS_MODE_USER

//
// Multi-Boot Flags (REMOVE ME)
//
#ifdef _REACTOS_
#define MB_FLAGS_MEM_INFO                   (0x1)
#define MB_FLAGS_BOOT_DEVICE                (0x2)
#define MB_FLAGS_COMMAND_LINE               (0x4)
#define MB_FLAGS_MODULE_INFO                (0x8)
#define MB_FLAGS_AOUT_SYMS                  (0x10)
#define MB_FLAGS_ELF_SYMS                   (0x20)
#define MB_FLAGS_MMAP_INFO                  (0x40)
#define MB_FLAGS_DRIVES_INFO                (0x80)
#define MB_FLAGS_CONFIG_TABLE               (0x100)
#define MB_FLAGS_BOOT_LOADER_NAME           (0x200)
#define MB_FLAGS_APM_TABLE                  (0x400)
#define MB_FLAGS_GRAPHICS_TABLE             (0x800)
#define MB_FLAGS_ACPI_TABLE                 (0x1000)
#endif

//
// HalShutdownSystem Types
//
typedef enum _FIRMWARE_ENTRY
{
    HalHaltRoutine,
    HalPowerDownRoutine,
    HalRestartRoutine,
    HalRebootRoutine,
    HalInteractiveModeRoutine,
    HalMaximumRoutine
} FIRMWARE_REENTRY, *PFIRMWARE_REENTRY;

//
// Hal Private dispatch Table
//
#define HAL_PRIVATE_DISPATCH_VERSION        2
typedef struct _HAL_PRIVATE_DISPATCH
{
    ULONG Version;
    PVOID HalHandlerForBus;
    PVOID HalHandlerForBus2;
    PVOID HalLocateHiberRanges;
    PVOID HalRegisterBusHandler;
    PVOID HalSetWakeEnable;
    PVOID HalSetWakeAlarm;
    PVOID HalTranslateBusAddress;
    PVOID HalTranslateBusAddress2;
    PVOID HalHaltSystem;
    PVOID Null;
    PVOID Null2;
    PVOID HalAllocateMapRegisters;
    PVOID KdSetupPciDeviceForDebugging;
    PVOID KdReleasePciDeviceforDebugging;
    PVOID KdGetAcpiTablePhase0;
    PVOID HalReferenceHandler;
    PVOID HalVectorToIDTEntry;
    PVOID MatchAll;
    PVOID KdUnmapVirtualAddress;
} HAL_PRIVATE_DISPATCH, *PHAL_PRIVATE_DISPATCH;

#ifndef _REACTOS_
//
// NLS Data Block
//
typedef struct _NLS_TABLE_DATA
{
    PVOID AnsiCodePageData;
    PVOID OemCodePageData;
    PVOID UnicodeCodePageData;
} NLS_TABLE_DATA, *PNLS_TABLE_DATA;

//
// Subsystem Specific Loader Blocks
//
typedef struct _PROFILE_PARAMETER_BLOCK
{
    USHORT DockData0;
    USHORT DockData1;
    USHORT DockData2;
    USHORT DockData3;
    ULONG DockData4;
    ULONG DockData5;
} PROFILE_PARAMETER_BLOCK, *PPROFILE_PARAMETER_BLOCK;

typedef struct _HEADLESS_LOADER_BLOCK
{
    UCHAR Unknown[0xC];
} HEADLESS_LOADER_BLOCK, *PHEADLESS_LOADER_BLOCK;

typedef struct _NETWORK_LOADER_BLOCK
{
    UCHAR Unknown[0xC];
} NETWORK_LOADER_BLOCK, *PNETWORK_LOADER_BLOCK;

//
// Extended Loader Parameter Block
//
typedef struct _LOADER_PARAMETER_EXTENSION
{
    ULONG Size;
    PROFILE_PARAMETER_BLOCK ProfileParameterBlock;
    ULONG MajorVersion;
    ULONG MinorVersion;
    PVOID SpecialConfigInfFile;
    ULONG SpecialConfigInfSize;
    PVOID TriageDumpData;
    //
    // NT 5.1
    //
    ULONG NumberOfPages;
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
    PVOID Unknown1;
    PVOID PrefetchDatabaseBase;
    ULONG PrefetchDatabaseSize;
    PNETWORK_LOADER_BLOCK NetworkLoaderBlock;
    //
    // NT 5.2+
    //
    PVOID Reserved[2];
    LIST_ENTRY FirmwareListEntry;
    PVOID AcpiTableBase;
    ULONG AcpiTableSize;
} LOADER_PARAMETER_EXTENSION, *PLOADER_PARAMETER_EXTENSION;

//
// Architecture specific Loader Parameter Blocks
//
typedef struct _I386_LOADER_BLOCK
{
    PVOID CommonDataArea;
    ULONG MachineType;
    ULONG Reserved;
} I386_LOADER_BLOCK, *PI386_LOADER_BLOCK;

//
// Setup Loader Parameter Block
//
typedef struct _SETUP_LOADER_BLOCK
{
    ULONG Unknown[139];
    ULONG Flags;
} SETUP_LOADER_BLOCK, *PSETUP_LOADER_BLOCK;

//
// Loader Parameter Block
//
typedef struct _LOADER_PARAMETER_BLOCK
{
    LIST_ENTRY LoadOrderListHead;
    LIST_ENTRY MemoryDescriptorListHead;
    LIST_ENTRY DriverList;
    PVOID KernelStack;
    PVOID Prcb;
    PVOID Process;
    PVOID Thread;
    ULONG RegistryLength;
    PVOID RegistryBase;
    PCONFIGURATION_COMPONENT_DATA ConfigurationRoot;
    LPSTR ArcBootDeviceName;
    LPSTR ArcHalDeviceName;
    LPSTR SystemRoot;
    LPSTR BootRoot;
    LPSTR CommandLine;
    PNLS_TABLE_DATA NlsTables;
    PARC_DISK_INFORMATION ArcDevices;
    PVOID OEMFont;
    PSETUP_LOADER_BLOCK SetupLdrBlock;
    PLOADER_PARAMETER_EXTENSION LpbExtension;
    union
    {
        I386_LOADER_BLOCK I386;
    } u;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

#else

//
// FIXME: ReactOS ONLY
//
typedef struct _LOADER_MODULE
{
    ULONG ModStart;
    ULONG ModEnd;
    ULONG String;
    ULONG Reserved;
} LOADER_MODULE, *PLOADER_MODULE;
typedef struct _LOADER_PARAMETER_BLOCK
{
    ULONG Flags;
    ULONG MemLower;
    ULONG MemHigher;
    ULONG BootDevice;
    ULONG CommandLine;
    ULONG ModsCount;
    ULONG ModsAddr;
    UCHAR Syms[12];
    ULONG MmapLength;
    ULONG MmapAddr;
    ULONG DrivesCount;
    ULONG DrivesAddr;
    ULONG ConfigTable;
    ULONG BootLoaderName;
    ULONG PageDirectoryStart;
    ULONG PageDirectoryEnd;
    ULONG KernelBase;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;
#endif

//
// Kernel Exports
//
#ifdef __NTOSKRNL__
extern HAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#else
extern PHAL_PRIVATE_DISPATCH HalPrivateDispatchTable;
#endif
extern ULONG KdComPortInUse;

#endif
#endif

