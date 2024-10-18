/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Public header
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

#pragma once

/* INCLUDES *****************************************************************/

/* Needed PSDK headers when using this library */
#if 0

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <winxxx.h>

#endif

/* NOTE: Please keep the header inclusion order! */

extern HANDLE ProcessHeap;

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
     * http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/boot-and-system-volumes.html
     * http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/arc-boot-process.html
     * http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/efi-boot-process.html
     * http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/determining-system-volume.html
     * http://homepage.ntlworld.com/jonathan.deboynepollard/FGA/determining-boot-volume.html
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


// HACK!!
extern BOOLEAN IsUnattendedSetup;


/* FUNCTIONS ****************************************************************/

#include "substset.h"

VOID
CheckUnattendedSetup(
    IN OUT PUSETUP_DATA pSetupData);

VOID
InstallSetupInfFile(
    IN OUT PUSETUP_DATA pSetupData);

NTSTATUS
GetSourcePaths(
    OUT PUNICODE_STRING SourcePath,
    OUT PUNICODE_STRING SourceRootPath,
    OUT PUNICODE_STRING SourceRootDir);

ERROR_NUMBER
LoadSetupInf(
    IN OUT PUSETUP_DATA pSetupData);

#define ERROR_SYSTEM_PARTITION_NOT_FOUND    (ERROR_LAST_ERROR_CODE + 1)

BOOLEAN
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
IsValidInstallDirectory(
    _In_ PCWSTR InstallDir);

NTSTATUS
InitDestinationPaths(
    _Inout_ PUSETUP_DATA pSetupData,
    _In_ PCWSTR InstallationDir,
    _In_ PVOLENTRY Volume);

// NTSTATUS
ERROR_NUMBER
InitializeSetup(
    IN OUT PUSETUP_DATA pSetupData,
    IN ULONG InitPhase);

VOID
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
UpdateRegistry(
    IN OUT PUSETUP_DATA pSetupData,
    /**/IN BOOLEAN RepairUpdateFlag,     /* HACK HACK! */
    /**/IN PPARTLIST PartitionList,      /* HACK HACK! */
    /**/IN WCHAR DestinationDriveLetter, /* HACK HACK! */
    /**/IN PCWSTR SelectedLanguageId,    /* HACK HACK! */
    IN PREGISTRY_STATUS_ROUTINE StatusRoutine OPTIONAL,
    IN PFONTSUBSTSETTINGS SubstSettings OPTIONAL);

/* EOF */
