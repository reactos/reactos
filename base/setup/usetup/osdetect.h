/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     NT 5.x family (MS Windows <= 2003, and ReactOS)
 *              operating systems detection code.
 * COPYRIGHT:   Copyright 2017-2018 Hermes Belusca-Maito
 */

typedef struct _NTOS_INSTALLATION
{
    LIST_ENTRY ListEntry;
    ULONG DiskNumber;
    ULONG PartitionNumber;
    PPARTENTRY PartEntry;
// BOOLEAN IsDefault;   // TRUE / FALSE whether this installation is marked as "default" in its corresponding loader configuration file.
// Vendor???? (Microsoft / ReactOS)
/**/WCHAR SystemRoot[MAX_PATH];/**/
    UNICODE_STRING SystemArcPath;   // Normalized ARC path
    UNICODE_STRING SystemNtPath;    // Corresponding NT path
/**/WCHAR InstallationName[MAX_PATH];/**/
} NTOS_INSTALLATION, *PNTOS_INSTALLATION;

// EnumerateNTOSInstallations
PGENERIC_LIST
CreateNTOSInstallationsList(
    IN PPARTLIST List);
