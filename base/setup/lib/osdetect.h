/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/lib/osdetect.c
 * PURPOSE:         NT 5.x family (MS Windows <= 2003, and ReactOS)
 *                  operating systems detection code.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#pragma once

typedef struct _NTOS_INSTALLATION
{
    LIST_ENTRY ListEntry;
// BOOLEAN IsDefault;   // TRUE / FALSE whether this installation is marked as "default" in its corresponding loader configuration file.
// Vendor???? (Microsoft / ReactOS)
    UNICODE_STRING SystemArcPath;   // Normalized ARC path
    UNICODE_STRING SystemNtPath;    // Corresponding NT path
    PCWSTR PathComponent;           // Pointer inside SystemNtPath.Buffer
    ULONG DiskNumber;
    ULONG PartitionNumber;
    PPARTENTRY PartEntry;
    WCHAR InstallationName[MAX_PATH];
} NTOS_INSTALLATION, *PNTOS_INSTALLATION;

// EnumerateNTOSInstallations
PGENERIC_LIST
CreateNTOSInstallationsList(
    IN PPARTLIST List);

/* EOF */
