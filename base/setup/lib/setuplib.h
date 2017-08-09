/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/setuplib.h
 * PURPOSE:         Setup Library - Public header
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
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
#include "linklist.h"
#include "ntverrsrc.h"
// #include "arcname.h"
#include "bldrsup.h"
#include "filesup.h"
#include "fsutil.h"
#include "genlist.h"
#include "infsupp.h"
#include "inicache.h"
#include "partlist.h"
#include "arcname.h"
#include "osdetect.h"
#include "regutil.h"
#include "registry.h"


/* DEFINES ******************************************************************/

#define KB ((ULONGLONG)1024)
#define MB (KB*KB)
#define GB (KB*KB*KB)
// #define TB (KB*KB*KB*KB)
// #define PB (KB*KB*KB*KB*KB)


/* TYPEDEFS *****************************************************************/

typedef struct _USETUP_DATA
{
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
    UNICODE_STRING DestinationPath;     /** Equivalent of 'NTOS_INSTALLATION::SystemNtPath' **/
    UNICODE_STRING DestinationArcPath;  /** Equivalent of 'NTOS_INSTALLATION::SystemArcPath' **/
    UNICODE_STRING DestinationRootPath;

    LONG DestinationDiskNumber;
    LONG DestinationPartitionNumber;
    LONG MBRInstallType;

    LONG FormatPartition;
    LONG AutoPartition;

    WCHAR LocaleID[9];
    LANGID LanguageId;

    ULONG RequiredPartitionDiskSpace;
    WCHAR InstallationDirectory[MAX_PATH];
} USETUP_DATA, *PUSETUP_DATA;

// HACK!!
extern BOOLEAN IsUnattendedSetup;


/* FUNCTIONS ****************************************************************/

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
    OUT HINF* SetupInf,
    IN OUT PUSETUP_DATA pSetupData);


/* EOF */
