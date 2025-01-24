/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Public header
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _SETUPLIB_
#define SPLIBAPI DECLSPEC_IMPORT
#else
#define SPLIBAPI
#endif

/* INCLUDES *****************************************************************/

/* Needed PSDK headers when using this library */
#if 0

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <winxxx.h>

#endif

extern SPLIBAPI BOOLEAN IsUnattendedSetup; // HACK

/* NOTE: Please keep the header inclusion order! */

#include "errorcode.h"
#include "spapisup/fileqsup.h"
#include "spapisup/infsupp.h"
#include "utils/linklist.h"
#include "utils/ntverrsrc.h"
// #include "utils/arcname.h"
#include "utils/bldrsup.h"
#include "utils/filesup.h"
#include "utils/fsrec.h"
#include "utils/genlist.h"
#include "utils/inicache.h"
#include "utils/partinfo.h"
#include "utils/partlist.h"
#include "utils/arcname.h"
#include "utils/osdetect.h"
#include "utils/regutil.h"

typedef enum _ARCHITECTURE_TYPE
{
    ARCH_PcAT,      //< Standard BIOS-based PC-AT
    ARCH_NEC98x86,  //< NEC PC-98
    ARCH_Xbox,      //< Original Xbox
    ARCH_Arc,       //< ARC-based (MIPS, SGI)
    ARCH_Efi,       //< EFI and UEFI
// Place other architectures supported by the Setup below.
} ARCHITECTURE_TYPE;

#include "bootcode.h"
#include "fsutil.h"
#include "bootsup.h"
#include "registry.h"
#include "mui.h"
#include "settings.h"

// #include "install.h" // See at the end...


/* DEFINES ******************************************************************/

#define KB ((ULONGLONG)1024)
#define MB (KB*KB)
#define GB (KB*KB*KB)
// #define TB (KB*KB*KB*KB)
// #define PB (KB*KB*KB*KB*KB)


/* TYPEDEFS *****************************************************************/

struct _USETUP_DATA;

typedef VOID
(__cdecl *PSETUP_ERROR_ROUTINE)(IN struct _USETUP_DATA*, ...);

typedef struct _USETUP_DATA
{
/* Error handling *****/
    ERROR_NUMBER LastErrorNumber;
    PSETUP_ERROR_ROUTINE ErrorRoutine;

/* Setup INFs *****/
    HINF SetupInf;

/* Installation *****/
    PVOID SetupFileQueue; // HSPFILEQ

/* SOURCE Paths *****/
    UNICODE_STRING SourceRootPath;
    UNICODE_STRING SourceRootDir;
    UNICODE_STRING SourcePath;

/* DESTINATION Paths *****/
    /*
     * Path to the system partition, where the boot manager resides.
     * On x86 PCs, this is usually the active partition.
     * On ARC, (u)EFI, ... platforms, this is a dedicated partition.
     *
     * For more information, see:
     * https://en.wikipedia.org/wiki/System_partition_and_boot_partition
     * https://web.archive.org/web/20160604095323/http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/boot-and-system-volumes.html
     * https://web.archive.org/web/20160604095238/http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/arc-boot-process.html
     * https://web.archive.org/web/20160508052211/http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/efi-boot-process.html
     * https://web.archive.org/web/20160604093304/http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/determining-system-volume.html
     * https://web.archive.org/web/20160604095540/http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/determining-boot-volume.html
     */
    UNICODE_STRING SystemRootPath;

    /* Path to the installation directory inside the ReactOS boot partition */
    UNICODE_STRING DestinationArcPath;  /** Equivalent of 'NTOS_INSTALLATION::SystemArcPath' **/
    UNICODE_STRING DestinationPath;     /** Equivalent of 'NTOS_INSTALLATION::SystemNtPath' **/
    UNICODE_STRING DestinationRootPath;

    // FIXME: This is only temporary!! Must be removed later!
    UNICODE_STRING InstallPath;

    LONG DestinationDiskNumber;
    LONG DestinationPartitionNumber;

    LONG BootLoaderLocation;
    LONG FormatPartition;
    LONG AutoPartition;
    LONG FsType;

/* Settings lists *****/
    PGENERIC_LIST ComputerList;
    PGENERIC_LIST DisplayList;
    PGENERIC_LIST KeyboardList;
    PGENERIC_LIST LayoutList;
    PGENERIC_LIST LanguageList;

/* Settings *****/
    ARCHITECTURE_TYPE ArchType; //< Target architecture (MachineType)
    PCWSTR ComputerType;
    PCWSTR DisplayType;
    // PCWSTR KeyboardDriver;
    // PCWSTR MouseDriver;
    PCWSTR LayoutId; // DefaultKBLayout

/* Other stuff *****/
    WCHAR LocaleID[9];
    LANGID LanguageId;

    ULONG RequiredPartitionDiskSpace;
    WCHAR InstallationDirectory[MAX_PATH];
} USETUP_DATA, *PUSETUP_DATA;


#include "install.h"


/* FUNCTIONS ****************************************************************/

#include "substset.h"

VOID
NTAPI
CheckUnattendedSetup(
    IN OUT PUSETUP_DATA pSetupData);

VOID
NTAPI
InstallSetupInfFile(
    IN OUT PUSETUP_DATA pSetupData);

NTSTATUS
GetSourcePaths(
    _Out_ PUNICODE_STRING SourcePath,
    _Out_ PUNICODE_STRING SourceRootPath,
    _Out_ PUNICODE_STRING SourceRootDir);

ERROR_NUMBER
LoadSetupInf(
    IN OUT PUSETUP_DATA pSetupData);

#define ERROR_SYSTEM_PARTITION_NOT_FOUND    (ERROR_LAST_ERROR_CODE + 1)

BOOLEAN
NTAPI
InitSystemPartition(
    /**/_In_ PPARTLIST PartitionList,       /* HACK HACK! */
    /**/_In_ PPARTENTRY InstallPartition,   /* HACK HACK! */
    /**/_Out_ PPARTENTRY* pSystemPartition, /* HACK HACK! */
    _In_opt_ PFSVOL_CALLBACK FsVolCallback,
    _In_opt_ PVOID Context);

/**
 * @brief
 * Defines the class of characters valid for the installation directory.
 *
 * The valid characters are: ASCII alphanumericals (a-z, A-Z, 0-9),
 * and: '.', '\\', '-', '_' . Spaces are not allowed.
 **/
#define IS_VALID_INSTALL_PATH_CHAR(c) \
    (isalnum(c) || (c) == L'.' || (c) == L'\\' || (c) == L'-' || (c) == L'_')

BOOLEAN
NTAPI
IsValidInstallDirectory(
    _In_ PCWSTR InstallDir);

NTSTATUS
NTAPI
InitDestinationPaths(
    _Inout_ PUSETUP_DATA pSetupData,
    _In_ PCWSTR InstallationDir,
    _In_ PVOLENTRY Volume);

// NTSTATUS
ERROR_NUMBER
NTAPI
InitializeSetup(
    _Inout_ PUSETUP_DATA pSetupData,
    _In_opt_ PSETUP_ERROR_ROUTINE ErrorRoutine,
    _In_ PSPFILE_EXPORTS pSpFileExports,
    _In_ PSPINF_EXPORTS pSpInfExports);

VOID
NTAPI
FinishSetup(
    IN OUT PUSETUP_DATA pSetupData);


typedef enum _REGISTRY_STATUS
{
    Success = 0,
    RegHiveUpdate,
    ImportRegHive,
    DisplaySettingsUpdate,
    LocaleSettingsUpdate,
    KeybLayouts,
    KeybSettingsUpdate,
    CodePageInfoUpdate,
} REGISTRY_STATUS;

typedef VOID
(__cdecl *PREGISTRY_STATUS_ROUTINE)(IN REGISTRY_STATUS, ...);

ERROR_NUMBER
NTAPI
UpdateRegistry(
    IN OUT PUSETUP_DATA pSetupData,
    /**/IN BOOLEAN RepairUpdateFlag,     /* HACK HACK! */
    /**/IN PPARTLIST PartitionList,      /* HACK HACK! */
    /**/IN WCHAR DestinationDriveLetter, /* HACK HACK! */
    /**/IN PCWSTR SelectedLanguageId,    /* HACK HACK! */
    IN PREGISTRY_STATUS_ROUTINE StatusRoutine OPTIONAL,
    IN PFONTSUBSTSETTINGS SubstSettings OPTIONAL);

#ifdef __cplusplus
}
#endif

/* EOF */
