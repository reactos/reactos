/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Windows-compatible NT Loader Block.
 * COPYRIGHT:   Copyright 2024-2025 Daniel Victor <ilauncherdeveloper@gmail.com>
 */

#pragma once

#include <arc/setupblk.h>

typedef struct _LOADER_PARAMETER_BLOCK1
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
} LOADER_PARAMETER_BLOCK1, *PLOADER_PARAMETER_BLOCK1;

typedef struct _LOADER_PARAMETER_BLOCK2
{
    PVOID Extension;
    union
    {
        I386_LOADER_BLOCK I386;
        ALPHA_LOADER_BLOCK Alpha;
        IA64_LOADER_BLOCK IA64;
        PPC_LOADER_BLOCK PowerPC;
        ARM_LOADER_BLOCK Arm;
    } u;
    FIRMWARE_INFORMATION_LOADER_BLOCK FirmwareInformation;
} LOADER_PARAMETER_BLOCK2, *PLOADER_PARAMETER_BLOCK2;

typedef struct _LOADER_PARAMETER_BLOCK_VISTA
{
    LOADER_PARAMETER_BLOCK1 Block1;
    PSETUP_LOADER_BLOCK SetupLdrBlock;
    LOADER_PARAMETER_BLOCK2 Block2;
} LOADER_PARAMETER_BLOCK_VISTA, *PLOADER_PARAMETER_BLOCK_VISTA;

typedef struct _LOADER_PARAMETER_EXTENSION1
{
    ULONG Size;
    PROFILE_PARAMETER_BLOCK Profile;
} LOADER_PARAMETER_EXTENSION1, *PLOADER_PARAMETER_EXTENSION1;

#include <pshpack1.h>

typedef struct _LOADER_PARAMETER_EXTENSION2
{
    PVOID EmInfFileImage;
    ULONG_PTR EmInfFileSize;
    PVOID TriageDumpBlock;
    //
    // NT 5.1
    //
    ULONG_PTR LoaderPagesSpanned;   /* Not anymore present starting NT 6.2 */
    PHEADLESS_LOADER_BLOCK HeadlessLoaderBlock;
    PSMBIOS_TABLE_HEADER SMBiosEPSHeader;
    PVOID DrvDBImage;
    ULONG_PTR DrvDBSize;
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
    ULONG_PTR ResumePages;
    PVOID DumpHeader;
} LOADER_PARAMETER_EXTENSION2, *PLOADER_PARAMETER_EXTENSION2;

typedef struct _LOADER_PARAMETER_EXTENSION_VISTA
{
    LOADER_PARAMETER_EXTENSION1 Extension1;
    ULONG MajorVersion;
    ULONG MinorVersion;
#ifdef _WIN64
    ULONG Padding1;
#endif
    LOADER_PARAMETER_EXTENSION2 Extension2;
} LOADER_PARAMETER_EXTENSION_VISTA, *PLOADER_PARAMETER_EXTENSION_VISTA;

#include <poppack.h>
