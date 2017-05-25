/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/bldrsup.h
 * PURPOSE:         NT 5.x family (MS Windows <= 2003, and ReactOS)
 *                  boot loaders management.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

// TODO: Add support for NT 6.x family! (detection + BCD manipulation).

#pragma once

typedef enum _NTOS_BOOT_LOADER_TYPE
{
    FreeLdr,    // ReactOS' FreeLoader
    NtLdr,      // Windows <= 2k3 NT "FlexBoot" OS Loader NTLDR
//  BootMgr,    // Vista+ BCD-oriented BOOTMGR
    BldrTypeMax
} NTOS_BOOT_LOADER_TYPE;

/*
 * This structure is inspired from the EFI boot entry structures
 * BOOT_ENTRY, BOOT_OPTIONS and FILE_PATH that are defined in ndk/iotypes.h .
 */
typedef struct _NTOS_BOOT_ENTRY
{
    // ULONG Version; // We might use the ntldr version here?? Or the "BootType" as in freeldr?
    // ULONG Length;
    // ULONG Id;            // Boot entry number (position) in the list
/** PCWSTR FriendlyName;    // Human-readable boot entry description **/
    PUNICODE_STRING FriendlyName;
    PCWSTR BootFilePath;    // Path to e.g. osloader.exe, or winload.efi
    PCWSTR OsLoadPath;      // The OS SystemRoot path
    PCWSTR OsOptions;
    PCWSTR OsLoadOptions;
} NTOS_BOOT_ENTRY, *PNTOS_BOOT_ENTRY;


typedef NTSTATUS
(NTAPI *PENUM_BOOT_ENTRIES_ROUTINE)(
    IN NTOS_BOOT_LOADER_TYPE Type,
    IN PNTOS_BOOT_ENTRY BootEntry,
    IN PVOID Parameter OPTIONAL);


NTSTATUS
FindNTOSBootLoader( // By handle
    IN HANDLE PartitionHandle, // OPTIONAL
    IN NTOS_BOOT_LOADER_TYPE Type,
    OUT PULONG Version);

NTSTATUS
EnumerateNTOSBootEntries(
    IN HANDLE PartitionHandle, // OPTIONAL
    IN NTOS_BOOT_LOADER_TYPE Type,
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL);

/* EOF */
