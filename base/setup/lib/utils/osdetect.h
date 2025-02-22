/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     NT 5.x family (MS Windows <= 2003, and ReactOS)
 *              operating systems detection code.
 * COPYRIGHT:   Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

/* Language-independent Vendor strings */
#define VENDOR_REACTOS      L"ReactOS"
#define VENDOR_MICROSOFT    L"Microsoft"

typedef struct _NTOS_INSTALLATION
{
    LIST_ENTRY ListEntry;
// BOOLEAN IsDefault;   // TRUE / FALSE whether this installation is marked as "default" in its corresponding loader configuration file.
    USHORT Machine;                 // Target architecture of the NTOS installation
    UNICODE_STRING SystemArcPath;   // Normalized ARC path ("ArcSystemRoot")
    UNICODE_STRING SystemNtPath;    // Corresponding NT path ("NtSystemRoot")
    PCWSTR PathComponent;           // Pointer inside SystemNtPath.Buffer
    ULONG DiskNumber;
    ULONG PartitionNumber;
    PVOLENTRY Volume; // PVOLINFO
    WCHAR InstallationName[MAX_PATH];
    WCHAR VendorName[MAX_PATH];
    // CHAR Data[ANYSIZE_ARRAY];
} NTOS_INSTALLATION, *PNTOS_INSTALLATION;

// EnumerateNTOSInstallations
PGENERIC_LIST
NTAPI
CreateNTOSInstallationsList(
    _In_ PPARTLIST PartList);

PCWSTR
NTAPI
FindSubStrI(
    _In_ PCWSTR str,
    _In_ PCWSTR strSearch);

/* EOF */
