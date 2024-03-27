/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/bootsup.c
 * PURPOSE:         Bootloader support functions
 * PROGRAMMERS:     ...
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "bldrsup.h"
#include "filesup.h"
#include "partlist.h"
#include "bootcode.h"
#include "fsutil.h"

#include "setuplib.h" // HAXX for IsUnattendedSetup!!

#include "bootsup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static VOID
TrimTrailingPathSeparators_UStr(
    _Inout_ PUNICODE_STRING UnicodeString)
{
    while (UnicodeString->Length >= sizeof(WCHAR) &&
           UnicodeString->Buffer[UnicodeString->Length / sizeof(WCHAR) - 1] == OBJ_NAME_PATH_SEPARATOR)
    {
        UnicodeString->Length -= sizeof(WCHAR);
    }
}

//
// TODO: The description/contents of these ROS entries
// should instead be stored in an external INI file.
// This function would then instead, loop through these
// entries from the file, and add each of them in turn.
//
static const struct
{
    ULONG_PTR BootEntryKey;
    PCWSTR FriendlyName;    // LoadIdentifier
    PCWSTR BootFilePath;    // EfiOsLoaderFilePath
    PCWSTR OsLoadPath;      // OsLoaderFilePath // OsFilePath
    PCWSTR OsLoadOptions;
} ReactOSEntries[] =
{
    {
        MAKESTRKEY(L"ReactOS"),
        L"\"ReactOS\"",
        NULL,
        NULL, // Use default
        L"/FASTDETECT"
    },

#if DBG
    {
        MAKESTRKEY(L"ReactOS_Debug"),
        L"\"ReactOS (Debug)\"",
        NULL,
        NULL, // Use default
        L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS"
    },
#ifdef _WINKD_
    {
        MAKESTRKEY(L"ReactOS_VBoxDebug"),
        L"\"ReactOS (VBox Debug)\"",
        NULL,
        NULL, // Use default
        L"/DEBUG /DEBUGPORT=VBOX /SOS"
    },
#else // _WINKD_
    {
        MAKESTRKEY(L"ReactOS_KdSerial"),
        L"\"ReactOS (RosDbg)\"",
        NULL,
        NULL, // Use default
        L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /KDSERIAL"
    },
#endif // _WINKD_
    {
        MAKESTRKEY(L"ReactOS_Screen"),
        L"\"ReactOS (Screen)\"",
        NULL,
        NULL, // Use default
        L"/DEBUG /DEBUGPORT=SCREEN /SOS"
    },
    {
        MAKESTRKEY(L"ReactOS_LogFile"),
        L"\"ReactOS (Log file)\"",
        NULL,
        NULL, // Use default
        L"/DEBUG /DEBUGPORT=FILE /SOS"
    },
    {
        MAKESTRKEY(L"ReactOS_Ram"),
        L"\"ReactOS (RAM Disk)\"",
        NULL,
        L"ramdisk(0)\\ReactOS",
        L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /RDPATH=reactos.img /RDIMAGEOFFSET=32256"
    },
    {
        MAKESTRKEY(L"ReactOS_EMS"),
        L"\"ReactOS (Emergency Management Services)\"",
        NULL,
        NULL, // Use default
        L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /redirect=com2 /redirectbaudrate=115200"
    },
#endif // DBG
};

#if DBG
#ifndef _WINKD_
    ULONG_PTR CurrentBootEntryUnattend = MAKESTRKEY(L"ReactOS_KdSerial");
#else
    ULONG_PTR CurrentBootEntryUnattend = MAKESTRKEY(L"ReactOS_Debug");
#endif // !_WINKD_
#else  // DBG
ULONG_PTR CurrentBootEntryUnattend = MAKESTRKEY(L"ReactOS");
#endif // DBG

#if DBG
ULONG_PTR CurrentBootEntry = MAKESTRKEY(L"ReactOS_Debug");
#else
ULONG_PTR CurrentBootEntry = MAKESTRKEY(L"ReactOS");
#endif

#if DBG
ULONG BootTimeout = 10;
#else
ULONG BootTimeout = 0; // Timeout 0 for non-debug
#endif


static VOID
CreateFreeLoaderReactOSEntries(
    _In_ PVOID BootStoreHandle,
    _In_ PCWSTR ArcPath)
{
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    BOOT_STORE_OPTIONS BootOptions;
    ULONG i;

    BootEntry->Version = FreeLdr;
    BootEntry->BootFilePath = NULL;

    BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
    RtlCopyMemory(Options->Signature,
                  NTOS_OPTIONS_SIGNATURE,
                  RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));

    Options->OsLoadPath = ArcPath;

    /* Add the boot entries */
    for (i = 0; i < RTL_NUMBER_OF(ReactOSEntries); ++i)
    {
        // BootEntry->BootEntryKey = ReactOSEntries[i].BootEntryKey;
        BootEntry->FriendlyName = ReactOSEntries[i].FriendlyName;
        // BootEntry->BootFilePath = ReactOSEntries[i].BootFilePath;
        Options->OsLoadPath =
            (ReactOSEntries[i].OsLoadPath ? ReactOSEntries[i].OsLoadPath : ArcPath);
        Options->OsLoadOptions = ReactOSEntries[i].OsLoadOptions;
        AddBootStoreEntry(BootStoreHandle, BootEntry, ReactOSEntries[i].BootEntryKey);
    }

    if (IsUnattendedSetup)
    {
        BootOptions.CurrentBootEntryKey = CurrentBootEntryUnattend;
        BootOptions.Timeout = 0; // Timeout 0 for unattended
    }
    else
    {
        BootOptions.CurrentBootEntryKey = CurrentBootEntry;
        BootOptions.Timeout = BootTimeout;
    }

    BootOptions.Version = FreeLdr;
    SetBootStoreOptions(BootStoreHandle, &BootOptions, 2 | 1);
}

static NTSTATUS
CreateFreeLoaderIniForReactOS(
    IN PCWSTR IniPath,
    IN PCWSTR ArcPath)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;

    /* Initialize the INI file and create the common FreeLdr sections */
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Add the ReactOS entries */
    CreateFreeLoaderReactOSEntries(BootStoreHandle, ArcPath);

    /* Close the INI file */
    CloseBootStore(BootStoreHandle);
    return STATUS_SUCCESS;
}

static NTSTATUS
CreateFreeLoaderIniForReactOSAndBootSector(
    IN PCWSTR IniPath,
    IN PCWSTR ArcPath,
    IN PCWSTR Section,
    IN PCWSTR Description,
    IN PCWSTR BootDrive,
    IN PCWSTR BootPartition,
    IN PCWSTR BootSector)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(BOOT_SECTOR_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PBOOT_SECTOR_OPTIONS Options = (PBOOT_SECTOR_OPTIONS)&BootEntry->OsOptions;

    /* Initialize the INI file and create the common FreeLdr sections */
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr, TRUE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Add the ReactOS entries */
    CreateFreeLoaderReactOSEntries(BootStoreHandle, ArcPath);

    BootEntry->Version = FreeLdr;
    BootEntry->BootFilePath = NULL;

    BootEntry->OsOptionsLength = sizeof(BOOT_SECTOR_OPTIONS);
    RtlCopyMemory(Options->Signature,
                  BOOT_SECTOR_OPTIONS_SIGNATURE,
                  RTL_FIELD_SIZE(BOOT_SECTOR_OPTIONS, Signature));

    Options->Drive = BootDrive;
    Options->Partition = BootPartition;
    Options->BootSectorFileName = BootSector;

    // BootEntry->BootEntryKey = MAKESTRKEY(Section);
    BootEntry->FriendlyName = Description;
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(Section));

    /* Close the INI file */
    CloseBootStore(BootStoreHandle);
    return STATUS_SUCCESS;
}

//
// I think this function can be generalizable as:
// "find the corresponding 'ReactOS' boot entry in this loader config file
// (here abstraction comes there), and if none, add a new one".
//

typedef struct _ENUM_REACTOS_ENTRIES_DATA
{
    ULONG i;
    BOOLEAN UseExistingEntry;
    PCWSTR ArcPath;
    WCHAR SectionName[80];
    WCHAR OsName[80];
} ENUM_REACTOS_ENTRIES_DATA, *PENUM_REACTOS_ENTRIES_DATA;

// PENUM_BOOT_ENTRIES_ROUTINE
static NTSTATUS
NTAPI
EnumerateReactOSEntries(
    IN BOOT_STORE_TYPE Type,
    IN PBOOT_STORE_ENTRY BootEntry,
    IN PVOID Parameter OPTIONAL)
{
    NTSTATUS Status;
    PENUM_REACTOS_ENTRIES_DATA Data = (PENUM_REACTOS_ENTRIES_DATA)Parameter;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    WCHAR SystemPath[MAX_PATH];

    /* We have a boot entry */

    /* Check for supported boot type "Windows2003" */
    if (BootEntry->OsOptionsLength < sizeof(NTOS_OPTIONS) ||
        RtlCompareMemory(&BootEntry->OsOptions /* Signature */,
                         NTOS_OPTIONS_SIGNATURE,
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature)) !=
                         RTL_FIELD_SIZE(NTOS_OPTIONS, Signature))
    {
        /* This is not a ReactOS entry */
        // DPRINT("    An installation '%S' of unsupported type '%S'\n",
               // BootEntry->FriendlyName, BootEntry->Version ? BootEntry->Version : L"n/a");
        DPRINT("    An installation '%S' of unsupported type %lu\n",
               BootEntry->FriendlyName, BootEntry->OsOptionsLength);
        /* Continue the enumeration */
        goto SkipThisEntry;
    }

    /* BootType is Windows2003, now check OsLoadPath */
    if (!Options->OsLoadPath || !*Options->OsLoadPath)
    {
        /* Certainly not a ReactOS installation */
        DPRINT1("    A Win2k3 install '%S' without an ARC path?!\n", BootEntry->FriendlyName);
        /* Continue the enumeration */
        goto SkipThisEntry;
    }

    if (_wcsicmp(Options->OsLoadPath, Data->ArcPath) != 0)
    {
        /* Not found, retry with a quoted path */
        Status = RtlStringCchPrintfW(SystemPath, ARRAYSIZE(SystemPath), L"\"%s\"", Data->ArcPath);
        if (!NT_SUCCESS(Status) || _wcsicmp(Options->OsLoadPath, SystemPath) != 0)
        {
            /*
             * This entry is a ReactOS entry, but the SystemRoot
             * does not match the one we are looking for.
             */
            /* Continue the enumeration */
            goto SkipThisEntry;
        }
    }

    DPRINT("    Found a candidate Win2k3 install '%S' with ARC path '%S'\n",
           BootEntry->FriendlyName, Options->OsLoadPath);
    // DPRINT("    Found a Win2k3 install '%S' with ARC path '%S'\n",
           // BootEntry->FriendlyName, Options->OsLoadPath);

    DPRINT("EnumerateReactOSEntries: OsLoadPath: '%S'\n", Options->OsLoadPath);

    Data->UseExistingEntry = TRUE;
    RtlStringCchCopyW(Data->OsName, ARRAYSIZE(Data->OsName), BootEntry->FriendlyName);

    /* We have found our entry, stop the enumeration now! */
    return STATUS_NO_MORE_ENTRIES;

SkipThisEntry:
    Data->UseExistingEntry = FALSE;
    if (Type == FreeLdr && wcscmp(Data->SectionName, (PWSTR)BootEntry->BootEntryKey)== 0)
    {
        RtlStringCchPrintfW(Data->SectionName, ARRAYSIZE(Data->SectionName),
                            L"ReactOS_%lu", Data->i);
        RtlStringCchPrintfW(Data->OsName, ARRAYSIZE(Data->OsName),
                            L"\"ReactOS %lu\"", Data->i);
        Data->i++;
    }
    return STATUS_SUCCESS;
}

static
NTSTATUS
UpdateFreeLoaderIni(
    IN PCWSTR IniPath,
    IN PCWSTR ArcPath)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;
    ENUM_REACTOS_ENTRIES_DATA Data;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

    /* Open the INI file */
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr, /*TRUE*/ FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Find an existing usable or an unused section name */
    Data.UseExistingEntry = TRUE;
    Data.i = 1;
    Data.ArcPath = ArcPath;
    RtlStringCchCopyW(Data.SectionName, ARRAYSIZE(Data.SectionName), L"ReactOS");
    RtlStringCchCopyW(Data.OsName, ARRAYSIZE(Data.OsName), L"\"ReactOS\"");

    //
    // FIXME: We temporarily use EnumerateBootStoreEntries, until
    // both QueryBootStoreEntry and ModifyBootStoreEntry get implemented.
    //
    Status = EnumerateBootStoreEntries(BootStoreHandle, EnumerateReactOSEntries, &Data);

    /* Create a new "ReactOS" entry if there is none already existing that suits us */
    if (!Data.UseExistingEntry)
    {
        // RtlStringCchPrintfW(Data.SectionName, ARRAYSIZE(Data.SectionName), L"ReactOS_%lu", Data.i);
        // RtlStringCchPrintfW(Data.OsName, ARRAYSIZE(Data.OsName), L"\"ReactOS %lu\"", Data.i);

        BootEntry->Version = FreeLdr;
        BootEntry->BootFilePath = NULL;

        BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
        RtlCopyMemory(Options->Signature,
                      NTOS_OPTIONS_SIGNATURE,
                      RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));

        Options->OsLoadPath = ArcPath;

        // BootEntry->BootEntryKey = MAKESTRKEY(Data.SectionName);
        BootEntry->FriendlyName = Data.OsName;
        Options->OsLoadOptions  = NULL; // L"";
        AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(Data.SectionName));
    }

    /* Close the INI file */
    CloseBootStore(BootStoreHandle);
    return STATUS_SUCCESS;
}

static
NTSTATUS
UpdateBootIni(
    IN PCWSTR IniPath,
    IN PCWSTR EntryName,    // ~= ArcPath
    IN PCWSTR EntryValue)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;
    ENUM_REACTOS_ENTRIES_DATA Data;

    // NOTE: Technically it would be "BootSector"...
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

    /* Open the INI file */
    Status = OpenBootStore(&BootStoreHandle, IniPath, NtLdr, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Find an existing usable or an unused section name */
    Data.UseExistingEntry = TRUE;
    // Data.i = 1;
    Data.ArcPath = EntryName;
    // RtlStringCchCopyW(Data.SectionName, ARRAYSIZE(Data.SectionName), L"ReactOS");
    RtlStringCchCopyW(Data.OsName, ARRAYSIZE(Data.OsName), L"\"ReactOS\"");

    //
    // FIXME: We temporarily use EnumerateBootStoreEntries, until
    // both QueryBootStoreEntry and ModifyBootStoreEntry get implemented.
    //
    Status = EnumerateBootStoreEntries(BootStoreHandle, EnumerateReactOSEntries, &Data);

    /* If either the key was not found, or contains something else, add a new one */
    if (!Data.UseExistingEntry /* ||
        ( (Status == STATUS_NO_MORE_ENTRIES) && wcscmp(Data.OsName, EntryValue) ) */)
    {
        BootEntry->Version = NtLdr;
        BootEntry->BootFilePath = NULL;

        BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
        RtlCopyMemory(Options->Signature,
                      NTOS_OPTIONS_SIGNATURE,
                      RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));

        Options->OsLoadPath = EntryName;

        // BootEntry->BootEntryKey = MAKESTRKEY(Data.SectionName);
        // BootEntry->FriendlyName = Data.OsName;
        BootEntry->FriendlyName = EntryValue;
        Options->OsLoadOptions  = NULL; // L"";
        AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(0 /*Data.SectionName*/));
    }

    /* Close the INI file */
    CloseBootStore(BootStoreHandle);
    return STATUS_SUCCESS; // Status;
}


static
BOOLEAN
IsThereAValidBootSector(
    IN PCWSTR RootPath)
{
    /*
     * We first demand that the bootsector has a valid signature at its end.
     * We then check the first 3 bytes (as a ULONG) of the bootsector for a
     * potential "valid" instruction (the BIOS starts execution of the bootsector
     * at its beginning). Currently this criterium is that this ULONG must be
     * non-zero. If both these tests pass, then the bootsector is valid; otherwise
     * it is invalid and certainly needs to be overwritten.
     */

    BOOLEAN IsValid = FALSE;
    NTSTATUS Status;
    UNICODE_STRING RootPartition;
    BOOTCODE BootSector = {0};

    /* Allocate and read the root partition bootsector.
     * Remove any trailing backslash if needed. */
    RtlInitUnicodeString(&RootPartition, RootPath);
    TrimTrailingPathSeparators_UStr(&RootPartition);
    Status = ReadBootCodeFromFile(&BootSector, &RootPartition, SECTORSIZE);
    if (!NT_SUCCESS(Status))
        return FALSE;

    /* Check for the existence of the bootsector signature */
    IsValid = (*(PUSHORT)((PUCHAR)BootSector.BootCode + 0x1FE) == 0xAA55);
    if (IsValid)
    {
        /* Check for the first instruction encoded on three bytes */
        IsValid = (((*(PULONG)BootSector.BootCode) & 0x00FFFFFF) != 0x00000000);
    }

    /* Free the bootsector and return */
    FreeBootCode(&BootSector);
    return IsValid;
}

static
NTSTATUS
SaveBootSector(
    IN PCWSTR RootPath,
    IN PCWSTR DstPath,
    IN ULONG Length)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    // LARGE_INTEGER FileOffset;
    BOOTCODE BootSector = {0};

    /* Allocate and read the root partition bootsector.
     * Remove any trailing backslash if needed. */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);
    Status = ReadBootCodeFromFile(&BootSector, &Name, Length);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Write the bootsector to DstPath */
    RtlInitUnicodeString(&Name, DstPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_SUPERSEDE,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        FreeBootCode(&BootSector);
        return Status;
    }

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootSector.BootCode,
                         BootSector.Length,
                         NULL,
                         NULL);
    NtClose(FileHandle);

    /* Free the bootsector and return */
    FreeBootCode(&BootSector);
    return Status;
}


static
NTSTATUS
InstallBootCodeToDisk(
    IN PCWSTR SrcPath,
    IN PCWSTR RootPath,
    IN PFS_INSTALL_BOOTCODE InstallBootCode)
{
    NTSTATUS Status, LockStatus;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle;

    /*
     * Open the root partition from which the bootcode (MBR, VBR) parameters
     * will be obtained; this is also where we will write the updated bootcode.
     * Remove any trailing backslash if needed.
     */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&PartitionHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT /* | FILE_SEQUENTIAL_ONLY */);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Lock the volume */
    LockStatus = NtFsControlFile(PartitionHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_LOCK_VOLUME,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Unable to lock the volume before installing boot code. Status 0x%08x. Expect problems.\n", LockStatus);
    }

    /* Install the bootcode (MBR, VBR) */
    Status = InstallBootCode(SrcPath, PartitionHandle, PartitionHandle);

    /* Dismount & unlock the volume */
    if (NT_SUCCESS(LockStatus))
    {
        LockStatus = NtFsControlFile(PartitionHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     FSCTL_DISMOUNT_VOLUME,
                                     NULL,
                                     0,
                                     NULL,
                                     0);
        if (!NT_SUCCESS(LockStatus))
        {
            DPRINT1("Unable to dismount the volume after installing boot code. Status 0x%08x. Expect problems.\n", LockStatus);
        }

        LockStatus = NtFsControlFile(PartitionHandle,
                                     NULL,
                                     NULL,
                                     NULL,
                                     &IoStatusBlock,
                                     FSCTL_UNLOCK_VOLUME,
                                     NULL,
                                     0,
                                     NULL,
                                     0);
        if (!NT_SUCCESS(LockStatus))
        {
            DPRINT1("Unable to unlock the volume after installing boot code. Status 0x%08x. Expect problems.\n", LockStatus);
        }
    }

    /* Close the partition */
    NtClose(PartitionHandle);

    return Status;
}

static
NTSTATUS
InstallBootCodeToFile(
    IN PCWSTR SrcPath,
    IN PCWSTR DstPath,
    IN PCWSTR RootPath,
    IN PFS_INSTALL_BOOTCODE InstallBootCode)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle, FileHandle;

    /*
     * Open the root partition from which the bootcode (MBR, VBR)
     * parameters will be obtained.
     *
     * FIXME? It might be possible that we need to also open it for writing
     * access in case we really need to still write the second portion of
     * the boot sector ????
     *
     * Remove any trailing backslash if needed.
     */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&PartitionHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT /* | FILE_SEQUENTIAL_ONLY */);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open or create the file where the new bootsector will be saved */
    RtlInitUnicodeString(&Name, DstPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_SUPERSEDE, // FILE_OVERWRITE_IF
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile() failed (Status %lx)\n", Status);
        NtClose(PartitionHandle);
        return Status;
    }

    /* Install the bootcode (MBR, VBR) */
    Status = InstallBootCode(SrcPath, FileHandle, PartitionHandle);

    /* Close the file and the partition */
    NtClose(FileHandle);
    NtClose(PartitionHandle);

    return Status;
}


static
NTSTATUS
InstallMbrBootCode(
    IN PCWSTR SrcPath,      // MBR source file (on the installation medium)
    IN HANDLE DstPath,      // Where to save the bootsector built from the source + disk information
    IN HANDLE DiskHandle)   // Disk holding the (old) MBR information
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    IO_STATUS_BLOCK IoStatusBlock;
    LARGE_INTEGER FileOffset;
    BOOTCODE OrigBootSector = {0};
    BOOTCODE NewBootSector  = {0};

C_ASSERT(sizeof(PARTITION_SECTOR) == SECTORSIZE);

    /* Allocate and read the current original MBR bootsector */
    Status = ReadBootCodeByHandle(&OrigBootSector,
                                  DiskHandle,
                                  sizeof(PARTITION_SECTOR));
    if (!NT_SUCCESS(Status))
        return Status;

    /* Allocate and read the new bootsector from SrcPath */
    RtlInitUnicodeString(&Name, SrcPath);
    Status = ReadBootCodeFromFile(&NewBootSector,
                                  &Name,
                                  sizeof(PARTITION_SECTOR));
    if (!NT_SUCCESS(Status))
    {
        FreeBootCode(&OrigBootSector);
        return Status;
    }

    /*
     * Copy the disk signature, the reserved fields and
     * the partition table from the old MBR to the new one.
     */
    RtlCopyMemory(&((PPARTITION_SECTOR)NewBootSector.BootCode)->Signature,
                  &((PPARTITION_SECTOR)OrigBootSector.BootCode)->Signature,
                  sizeof(PARTITION_SECTOR) -
                  FIELD_OFFSET(PARTITION_SECTOR, Signature)
                  /* Length of partition table */);

    /* Free the original bootsector */
    FreeBootCode(&OrigBootSector);

    /* Write the new bootsector to DstPath */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector.BootCode,
                         NewBootSector.Length,
                         &FileOffset,
                         NULL);

    /* Free the new bootsector */
    FreeBootCode(&NewBootSector);

    return Status;
}

static const struct
{
    PCSTR BootCodeName;
    PFS_INSTALL_BOOTCODE InstallBootCode;
    ULONG SaveLength;
    PCWSTR BootCodeFilePath; // TODO: Hardcoded paths. Retrieve from INF file?
} BootCodes[] =
{
    {"MBR"  , InstallMbrBootCode  , sizeof(PARTITION_SECTOR), L"\\loader\\dosmbr.bin"},
    {"FAT12", InstallFat12BootCode, FAT_BOOTSECTOR_SIZE     , L"\\loader\\fat.bin"   },
    {"FAT16", InstallFat16BootCode, FAT_BOOTSECTOR_SIZE     , L"\\loader\\fat.bin"   },
    {"FAT32", InstallFat32BootCode, FAT32_BOOTSECTOR_SIZE   , L"\\loader\\fat32.bin" },
    {"NTFS" , InstallNtfsBootCode , NTFS_BOOTSECTOR_SIZE    , L"\\loader\\ntfs.bin"  },
    {"BTRFS", InstallBtrfsBootCode, BTRFS_BOOTSECTOR_SIZE   , L"\\loader\\btrfs.bin" },
};

//
// TODO: Add "DiskNew" boolean. If TRUE, don't bother saving the old MBR.
//
/**
 * SystemRootPath : System partition path.
 * SourceRootPath : Installation source, where to copy freeldr from.
 * DestinationDevicePathBuffer : the L"\\Device\\Harddisk%d\\Partition0"
 *      of the hard disk containing the system partition.
 **/
NTSTATUS
InstallMbrBootCodeToDisk(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PCWSTR DestinationDevicePathBuffer)
{
    NTSTATUS Status;
    WCHAR SourceMbrPathBuffer[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

#if 0
    /*
     * The DestinationDevicePathBuffer parameter has been built with
     * the following instruction by the caller; I'm not yet sure whether
     * I actually want this function to build the path instead, hence
     * I keep this code here but disabled for now...
     */
    WCHAR DestinationDevicePathBuffer[MAX_PATH];
    RtlStringCchPrintfW(DestinationDevicePathBuffer, ARRAYSIZE(DestinationDevicePathBuffer),
                        L"\\Device\\Harddisk%d\\Partition0",
                        DiskNumber);
#endif

    CombinePaths(SourceMbrPathBuffer, ARRAYSIZE(SourceMbrPathBuffer), 2,
                 SourceRootPath->Buffer, BootCodes[0].BootCodeFilePath);

    if (IsThereAValidBootSector(DestinationDevicePathBuffer))
    {
        /* Save current MBR */
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2,
                     SystemRootPath->Buffer, L"mbr.old");

        DPRINT1("Save MBR: %S ==> %S\n", DestinationDevicePathBuffer, DstPath);
        Status = SaveBootSector(DestinationDevicePathBuffer, DstPath, sizeof(PARTITION_SECTOR));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SaveBootSector() failed (Status %lx)\n", Status);
            // Don't care if we succeeded or not saving the old MBR, just go ahead.
        }
    }

    /* Install the MBR */
    DPRINT1("Install %s bootcode: %S ==> %S\n",
            BootCodes[0].BootCodeName, SourceMbrPathBuffer, DestinationDevicePathBuffer);
    return InstallBootCodeToDisk(SourceMbrPathBuffer,
                                 DestinationDevicePathBuffer,
                                 BootCodes[0].InstallBootCode);
}


/**
 * @brief
 * Attempts to recognize well-known DOS, OS/2 or Windows 9x boot loaders
 * (for FAT12/16/32 volumes only).
 *
 * @param[in]   SystemRootPath
 * Drive where to check for a boot loader.
 *
 * @param[out]  Section, Description, BootSector, BootSectorSize
 * Suggested boot sector parameters for creating a freeldr.ini replacement entry.
 *
 * @return  TRUE in case a boot loader was recognized; FALSE if not.
 **/
static
BOOLEAN
RecognizeDOSLoader(
    _In_ PUNICODE_STRING SystemRootPath,
    _Out_ const PCWSTR* Section,
    _Out_ const PCWSTR* Description,
    _Out_ const PCWSTR* BootSector,
    _Out_ PULONG BootSectorSize)
{
    /* Search for COMPAQ MS-DOS 1.x (1.11, 1.12, based on MS-DOS 1.25) boot loader */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"IOSYS.COM") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"MSDOS.COM"))
    {
        DPRINT1("Found COMPAQ MS-DOS 1.x (1.11, 1.12) / MS-DOS 1.25 boot loader\n");

        *Section       = L"CPQDOS";
        *Description   = L"\"COMPAQ MS-DOS 1.x / MS-DOS 1.25\"";
        *BootSector    = L"BOOTSECT.DOS";
    }
    else
    /* Search for Microsoft DOS or Windows 9x boot loader */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"IO.SYS") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"MSDOS.SYS"))
        // WINBOOT.SYS
    {
        DPRINT1("Found Microsoft DOS or Windows 9x boot loader\n");

        *Section       = L"MSDOS";
        *Description   = L"\"MS-DOS/Windows\"";
        *BootSector    = L"BOOTSECT.DOS";
    }
    else
    /* Search for IBM PC-DOS or DR-DOS 5.x boot loader */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"IBMIO.COM" ) || // Some people refer to this file instead of IBMBIO.COM...
        DoesFileExist_2(SystemRootPath->Buffer, L"IBMBIO.COM") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"IBMDOS.COM"))
    {
        DPRINT1("Found IBM PC-DOS or DR-DOS 5.x or IBM OS/2 1.0\n");

        *Section       = L"IBMDOS";
        *Description   = L"\"IBM PC-DOS or DR-DOS 5.x or IBM OS/2 1.0\"";
        *BootSector    = L"BOOTSECT.DOS";
    }
    else
    /* Search for DR-DOS 3.x boot loader */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"DRBIOS.SYS") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"DRBDOS.SYS"))
    {
        DPRINT1("Found DR-DOS 3.x\n");

        *Section       = L"DRDOS";
        *Description   = L"\"DR-DOS 3.x\"";
        *BootSector    = L"BOOTSECT.DOS";
    }
    else
    /* Search for Dell Real-Mode Kernel (DRMK) OS */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"DELLBIO.BIN") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"DELLRMK.BIN"))
    {
        DPRINT1("Found Dell Real-Mode Kernel OS\n");

        *Section       = L"DRMK";
        *Description   = L"\"Dell Real-Mode Kernel OS\"";
        *BootSector    = L"BOOTSECT.DOS";
    }
    else
    /* Search for MS OS/2 1.x */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"OS2BOOT.COM") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"OS2BIO.COM" ) ||
        DoesFileExist_2(SystemRootPath->Buffer, L"OS2DOS.COM" ))
    {
        DPRINT1("Found MS OS/2 1.x\n");

        *Section       = L"MSOS2";
        *Description   = L"\"MS OS/2 1.x\"";
        *BootSector    = L"BOOTSECT.OS2";
    }
    else
    /* Search for MS or IBM OS/2 */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"OS2BOOT") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"OS2LDR" ) ||
        DoesFileExist_2(SystemRootPath->Buffer, L"OS2KRNL"))
    {
        DPRINT1("Found MS/IBM OS/2\n");

        *Section       = L"IBMOS2";
        *Description   = L"\"MS/IBM OS/2\"";
        *BootSector    = L"BOOTSECT.OS2";
    }
    else
    /* Search for FreeDOS boot loader */
    if (DoesFileExist_2(SystemRootPath->Buffer, L"kernel.sys"))
    {
        DPRINT1("Found FreeDOS boot loader\n");

        *Section       = L"FDOS";
        *Description   = L"\"FreeDOS\"";
        *BootSector    = L"BOOTSECT.DOS";
    }
    else
    {
        /* No or unknown boot loader */
        DPRINT1("No or unknown boot loader found\n");

        *Section       = L"Unknown";
        *Description   = L"\"Unknown Operating System\"";
        *BootSector    = L"BOOTSECT.OLD";
    }

    // *BootSectorSize = SECTORSIZE;

    return TRUE;
}

static
BOOLEAN
RecognizeNTLoader(
    _In_ PUNICODE_STRING SystemRootPath,
    _Out_ PBOOT_STORE_TYPE Type/*,
    _Out_ const PCWSTR* Section,
    _Out_ const PCWSTR* Description,
    _Out_ const PCWSTR* BootSector,
    _Out_ PULONG BootSectorSize*/)
{
    // *Section        = NULL;
    // *Description    = NULL;
    // *BootSector     = NULL;
    // // *BootSectorSize = 0;

    // FIXME: Check for Vista+ bootloader!

    /*** Status = FindBootStore(PartitionHandle, NtLdr, &Version); ***/
    /*** Status = FindBootStore(PartitionHandle, BootMgr, &Version); ***/
    if (DoesFileExist_2(SystemRootPath->Buffer, L"NTLDR") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"BOOT.INI"))
    {
        /* Search root directory for 'NTLDR' and 'BOOT.INI' */
        DPRINT1("Found Microsoft Windows NT/2000/XP boot loader\n");

        // TODO: Return actual NT loader type: FreeLdr, BootMgr
        *Type = NtLdr;
        return TRUE;
    }
    return FALSE;
}

static
BOOLEAN
RecognizeLinuxLoader(
    _In_ PUNICODE_STRING SystemRootPath,
    _Out_ const PCWSTR* Section,
    _Out_ const PCWSTR* Description,
    _Out_ const PCWSTR* BootSector,
    _Out_ PULONG BootSectorSize)
{
    // if (IsThereAValidBootSector(SystemRootPath->Buffer))
    {
        /* Certainly SysLinux, GRUB, LILO... or an unknown boot loader */
        DPRINT1("*nix or unknown boot loader found\n");

        *Section       = L"Linux";
        *Description   = L"\"Linux\"";
        *BootSector    = L"BOOTSECT.OLD";
        // *BootSectorSize = BTRFS_BOOTSECTOR_SIZE; // FIXME: Hardcoded size

        return TRUE;
    }
    return FALSE;
}


//
// TODO: Add "preformatted" boolean. If TRUE, don't bother
// recognizing any existing boot loader and saving the old VBR.
//
/**
 * SystemRootPath     : System partition path.
 * SourceRootPath     : Installation source, where to copy freeldr from.
 * DestinationArcPath : ReactOS installation ARC path.
 * FileSystemName     : File system.
 **/
NTSTATUS
InstallVBRToPartition(
    _In_ PUNICODE_STRING SystemRootPath,
    _In_ PUNICODE_STRING SourceRootPath,
    _In_ PUNICODE_STRING DestinationArcPath,
    _In_ PCWSTR FileSystemName)
{
    NTSTATUS Status;
    ULONG FsId;
    BOOLEAN DoesFreeLdrExist;
    BOOLEAN IsValidBootSector;
    BOOLEAN ReplaceBootCode;

    PCWSTR Section;
    PCWSTR Description;
    PCWSTR BootDrive;
    PCWSTR BootPartition;
    PCWSTR BootSector;
    ULONG  BootSectorSize;

    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    ASSERT(FileSystemName);
    /*if (wcsicmp(FileSystemName, L"FAT12") == 0)
        FsId = 1;
    else*/ if (wcsicmp(FileSystemName, L"FAT") == 0)
        FsId = 2;
    else if (wcsicmp(FileSystemName, L"FAT16") == 0)
        FsId = 2;
    else if (wcsicmp(FileSystemName, L"FAT32") == 0)
        FsId = 3;
    else if (wcsicmp(FileSystemName, L"NTFS" ) == 0)
        FsId = 4;
    else if (wcsicmp(FileSystemName, L"BTRFS") == 0)
        FsId = 5;
    /*
    else if (wcsicmp(FileSystemName, L"EXT2") == 0 ||
             wcsicmp(FileSystemName, L"EXT3") == 0 ||
             wcsicmp(FileSystemName, L"EXT4") == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }
    */
    else
    {
        /* Unknown file system */
        DPRINT1("Unknown or unsupported file system '%S'\n", FileSystemName);
        return STATUS_NOT_SUPPORTED;
    }

    DPRINT("System path: '%wZ'\n", SystemRootPath);

    /* Copy FreeLoader to the system partition, always overwriting the older version */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\freeldr.sys");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"freeldr.sys");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
        return Status;
    }

//
// FIXME: Improve this? Because, what if we don't have freeldr installed
// as the active boot code, but the user just placed a freeldr.ini file
// there? In that case we should first install the boot code, and only then
// decide whether we're going to update freeldr.ini ...
//
    /* Prepare for possibly updating 'freeldr.ini' */
    DoesFreeLdrExist = DoesFileExist_2(SystemRootPath->Buffer, L"freeldr.ini");
    if (DoesFreeLdrExist)
    {
        /* Update existing 'freeldr.ini' */
        DPRINT1("Update existing 'freeldr.ini'\n");
        Status = UpdateFreeLoaderIni(SystemRootPath->Buffer, DestinationArcPath->Buffer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
            return Status;
        }

        // return STATUS_SUCCESS; // Do it or not??
    }


    //
    // Newly-formatted partition --> install VBR and freeldr only.
    //
    // Otherwise:
    //
    // - If no valid boot sector --> install VBR and freeldr only.
    //
    // - If valid boot sector, try to recognize loader:
    //
    //   * NT boot loader: try adding ourselves to the boot.ini or BCD.
    //     NOTE: We don't currently support BCD, so we'll throw error.
    //
    //   * DOS, Linux boot loader or unknown: backup old VBR,
    //     install new VBR and freeldr, and add extra boot entry
    //     to boot old VBR.
    //


    /* Suppose non-NT bootloaders: install our own bootloader */
    ReplaceBootCode = TRUE;

    /* Set up default boot sector backup size */
    BootSectorSize = BootCodes[FsId].BootCodeFilePath;

    /*
     * Check for the existence of known bootloaders, depending
     * on the target filesystem, and determine whether to replace
     * the boot code or use existing one instead (e.g. NTLDR).
     */
    if (FsId == 2 || FsId == 3) // FAT16/32
    {
        /* Check for NT and other bootloaders */

        IsValidBootSector = IsThereAValidBootSector(SystemRootPath->Buffer);
        if (IsValidBootSector)
        {
            BOOT_STORE_TYPE Type;
            if (RecognizeNTLoader(SystemRootPath,
                                  &Type/*,
                                  &Section,
                                  &Description,
                                  &BootDrive,
                                  &BootPartition,
                                  &BootSector,
                                  &BootSectorSize*/))
            {
                /* NT bootloader: Add ourselves to existing bootloader */
                ReplaceBootCode = FALSE;
            }
            else
            {
                IsValidBootSector = RecognizeDOSLoader(SystemRootPath,
                                                       &Section,
                                                       &Description,
                                                       &BootSector,
                                                       &BootSectorSize);
                if (!IsValidBootSector)
                {
                    IsValidBootSector = RecognizeLinuxLoader(SystemRootPath,
                                                             &Section,
                                                             &Description,
                                                             &BootSector,
                                                             &BootSectorSize);
                }
            }
        }
    }
    else if (FsId == 4) // NTFS
    {
        /* Check for NT and other bootloaders */

        IsValidBootSector = IsThereAValidBootSector(SystemRootPath->Buffer);
        if (IsValidBootSector)
        {
            BOOT_STORE_TYPE Type;
            if (RecognizeNTLoader(SystemRootPath,
                                  &Type/*,
                                  &Section,
                                  &Description,
                                  &BootSector,
                                  &BootSectorSize*/))
            {
                /* NT bootloader: Add ourselves to existing bootloader */
                ReplaceBootCode = FALSE;
            }
            else
            {
                IsValidBootSector = RecognizeLinuxLoader(SystemRootPath,
                                                         &Section,
                                                         &Description,
                                                         &BootSector,
                                                         &BootSectorSize);
            }
        }
    }
    else if (FsId == 5) // BTRFS
    {
        /* Check for other bootloaders */

        IsValidBootSector = IsThereAValidBootSector(SystemRootPath->Buffer);
        if (IsValidBootSector)
        {
            IsValidBootSector = RecognizeLinuxLoader(SystemRootPath,
                                                     &Section,
                                                     &Description,
                                                     &BootSector,
                                                     &BootSectorSize);
        }
    }
    else
    {
        ASSERT(FALSE);
    }


    if (ReplaceBootCode)
    {
        /* We've got a valid boot sector in SystemRootPath that we should replace:
         * get the corresponding BIOS boot drive and partition identifiers */
        if (IsValidBootSector)
        {
            // FIXME: These are hardcoded values!
            BootDrive     = L"hd0";
            BootPartition = L"1";
        }

        /* Backup old VBR if valid */
        if (IsValidBootSector)
        {
            /* Save current bootsector */
            CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, BootSector);

            DPRINT1("Save bootsector: %S ==> %S\n", SystemRootPath->Buffer, DstPath);
            Status = SaveBootSector(SystemRootPath->Buffer, DstPath, BootSectorSize);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SaveBootSector() failed (Status %lx)\n", Status);
                return Status;
            }
        }

        // FIXME: What about if DoesFreeLdrExist ??

        /* Create or update 'freeldr.ini' */
        DPRINT1("Create new 'freeldr.ini'\n");
        /* Add boot entry to old VBR if valid */
        // FIXME: Split this in separate operations
        if (IsValidBootSector)
        {
            Status = CreateFreeLoaderIniForReactOSAndBootSector(
                         SystemRootPath->Buffer, DestinationArcPath->Buffer,
                         Section, Description,
                         BootDrive, BootPartition, BootSector);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateFreeLoaderIniForReactOSAndBootSector() failed (Status %lx)\n", Status);
                return Status;
            }
        }
        else
        {
            Status = CreateFreeLoaderIniForReactOS(SystemRootPath->Buffer, DestinationArcPath->Buffer);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
                return Status;
            }
        }

        /* Install new bootsector on the disk */
        CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2,
                     SourceRootPath->Buffer, BootCodes[FsId].BootCodeFilePath);

        DPRINT1("Install %s bootcode: %S ==> %S\n",
                BootCodes[FsId].BootCodeName, SrcPath, SystemRootPath->Buffer);
        Status = InstallBootCodeToDisk(SrcPath,
                                       SystemRootPath->Buffer,
                                       BootCodes[FsId].InstallBootCode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallBootCodeToDisk(%s) failed (Status %lx)\n",
                    BootCodes[FsId].BootCodeName, Status);
            return Status;
        }
    }
    else
    {
        // TODO: ASSERT that it's either Ntldr or BootMgr

        /* Create or update 'freeldr.ini' */
        if (!DoesFreeLdrExist)
        {
            /* Create new 'freeldr.ini' */
            DPRINT1("Create new 'freeldr.ini'\n");
            Status = CreateFreeLoaderIniForReactOS(SystemRootPath->Buffer, DestinationArcPath->Buffer);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
                return Status;
            }

            // FIXME: Do "Install new bootcode into a file"
            // here or below?
        }
        else
        {
            /* Update 'freeldr.ini' */
            // TODO
        }

        /* Add ourselves to existing bootloader */
        // FIXME: Add support for BCD (BootMgr)
        /* Update 'boot.ini' */
        /* Windows' NTLDR loads an external bootsector file when the specified drive
           letter is C:, otherwise it will interpret it as a boot DOS path specifier. */
        DPRINT1("Update 'boot.ini'\n");
        Status = UpdateBootIni(SystemRootPath->Buffer,
                               L"C:\\bootsect.ros",
                               L"\"ReactOS\"");
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("UpdateBootIni() failed (Status %lx)\n", Status);
            return Status;
        }

        /* Install the new bootcode into a file */
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"bootsect.ros");
        CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2,
                     SourceRootPath->Buffer, BootCodes[FsId].BootCodeFilePath);

        DPRINT1("Install %s bootcode: %S ==> %S\n",
                BootCodes[FsId].BootCodeName, SrcPath, DstPath);
        Status = InstallBootCodeToFile(SrcPath, DstPath,
                                       SystemRootPath->Buffer,
                                       BootCodes[FsId].InstallBootCode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallBootCodeToFile(%s) failed (Status %lx)\n",
                    BootCodes[FsId].BootCodeName, Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}


/**
 * SourceRootPath     : Installation source, where to copy freeldr from.
 * DestinationArcPath : ReactOS installation ARC path.
 *
 * No SystemRootPath because we install to 1st floppy (hardcoded) (~ System partition path)
 * No FileSystemName because we hardcode to FAT12.
 **/
NTSTATUS
InstallFatBootcodeToRemovable(
    IN PUNICODE_STRING RemovableRootPath, // SystemRootPath
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath/*,
    IN PCWSTR FileSystemName*/)
{
    NTSTATUS Status;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Verify that the removable disk is accessible */
    if (!DoesDirExist(NULL, RemovableRootPath->Buffer))
        return STATUS_DEVICE_NOT_READY;

    /* Format the removable disk */
    // FormatPartition(...)
    Status = FormatFileSystem(RemovableRootPath->Buffer,
                              L"FAT", // FileSystemName,
                              FMIFS_FLOPPY, // FIXME: How to determine dynamically?
                              NULL,
                              TRUE,
                              0,
                              NULL);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_NOT_SUPPORTED)
            DPRINT1("FAT FS non existent on this system?!\n");
        else
            DPRINT1("VfatFormat() failed (Status %lx)\n", Status);

        return Status;
    }

    /* Copy FreeLoader to the boot partition */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\freeldr.sys");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, RemovableRootPath->Buffer, L"freeldr.sys");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Create new 'freeldr.ini' */
    DPRINT("Create new 'freeldr.ini'\n");
    Status = CreateFreeLoaderIniForReactOS(RemovableRootPath->Buffer, DestinationArcPath->Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Install FAT12 boosector */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2,
                 SourceRootPath->Buffer, BootCodes[1].BootCodeFilePath);
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 1, RemovableRootPath->Buffer);

    DPRINT("Install %s bootcode: %S ==> %S\n",
           BootCodes[1].BootCodeName, SrcPath, DstPath);
    Status = InstallBootCodeToDisk(SrcPath,
                                   DstPath,
                                   BootCodes[1].InstallBootCode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallBootCodeToDisk(%s) failed (Status %lx)\n",
                BootCodes[1].BootCodeName, Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

//
// TODO: Remove once InstallFatBootcodeToRemovable() is reliable
//
NTSTATUS
InstallFatBootcodeToFloppy(
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath)
{
    UNICODE_STRING FloppyDevice = RTL_CONSTANT_STRING(L"\\Device\\Floppy0\\");

    return InstallFatBootcodeToRemovable(&FloppyDrive,
                                         SourceRootPath,
                                         DestinationArcPath/*,
                                         L"FAT"*/);
}

/* EOF */
