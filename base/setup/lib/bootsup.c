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

/*
 * BIG FIXME!!
 * ===========
 *
 * bootsup.c can deal with MBR code (actually it'll have at some point
 * to share or give it to partlist.c, because when we'll support GPT disks,
 * things will change a bit).
 * And, bootsup.c can manage initializing / adding boot entries into NTLDR
 * and FREELDR, and installing the latter, and saving the old MBR / boot
 * sectors in files.
 */

/* FUNCTIONS ****************************************************************/

static VOID
TrimTrailingPathSeparators_UStr(
    IN OUT PUNICODE_STRING UnicodeString)
{
    while (UnicodeString->Length >= sizeof(WCHAR) &&
           UnicodeString->Buffer[UnicodeString->Length / sizeof(WCHAR) - 1] == OBJ_NAME_PATH_SEPARATOR)
    {
        UnicodeString->Length -= sizeof(WCHAR);
    }
}


static VOID
CreateFreeLoaderReactOSEntries(
    IN PVOID BootStoreHandle,
    IN PCWSTR ArcPath)
{
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    BOOT_STORE_OPTIONS BootOptions;

    BootEntry->Version = FreeLdr;
    BootEntry->BootFilePath = NULL;

    BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
    RtlCopyMemory(Options->Signature,
                  NTOS_OPTIONS_SIGNATURE,
                  RTL_FIELD_SIZE(NTOS_OPTIONS, Signature));

    Options->OsLoadPath = ArcPath;

    /* ReactOS */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS");
    BootEntry->FriendlyName = L"\"ReactOS\"";
    Options->OsLoadOptions  = NULL; // L"";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS"));

    /* ReactOS_Debug */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS_Debug");
    BootEntry->FriendlyName = L"\"ReactOS (Debug)\"";
    Options->OsLoadOptions  = L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS_Debug"));

#ifdef _WINKD_
    /* ReactOS_VBoxDebug */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS_VBoxDebug");
    BootEntry->FriendlyName = L"\"ReactOS (VBox Debug)\"";
    Options->OsLoadOptions  = L"/DEBUG /DEBUGPORT=VBOX /SOS";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS_VBoxDebug"));
#endif
#if DBG
#ifndef _WINKD_
    /* ReactOS_KdSerial */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS_KdSerial");
    BootEntry->FriendlyName = L"\"ReactOS (RosDbg)\"";
    Options->OsLoadOptions  = L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /KDSERIAL";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS_KdSerial"));
#endif

    /* ReactOS_Screen */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS_Screen");
    BootEntry->FriendlyName = L"\"ReactOS (Screen)\"";
    Options->OsLoadOptions  = L"/DEBUG /DEBUGPORT=SCREEN /SOS";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS_Screen"));

    /* ReactOS_LogFile */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS_LogFile");
    BootEntry->FriendlyName = L"\"ReactOS (Log file)\"";
    Options->OsLoadOptions  = L"/DEBUG /DEBUGPORT=FILE /SOS";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS_LogFile"));

    /* ReactOS_Ram */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS_Ram");
    BootEntry->FriendlyName = L"\"ReactOS (RAM Disk)\"";
    Options->OsLoadPath     = L"ramdisk(0)\\ReactOS";
    Options->OsLoadOptions  = L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /RDPATH=reactos.img /RDIMAGEOFFSET=32256";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS_Ram"));

    /* ReactOS_EMS */
    // BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS_EMS");
    BootEntry->FriendlyName = L"\"ReactOS (Emergency Management Services)\"";
    Options->OsLoadPath     = ArcPath;
    Options->OsLoadOptions  = L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /redirect=com2 /redirectbaudrate=115200";
    AddBootStoreEntry(BootStoreHandle, BootEntry, MAKESTRKEY(L"ReactOS_EMS"));
#endif


    /* DefaultOS=ReactOS */
#if DBG && !defined(_WINKD_)
    if (IsUnattendedSetup)
    {
        BootOptions.CurrentBootEntryKey = MAKESTRKEY(L"ReactOS_KdSerial");
    }
    else
#endif
    {
#if DBG
        BootOptions.CurrentBootEntryKey = MAKESTRKEY(L"ReactOS_Debug");
#else
        BootOptions.CurrentBootEntryKey = MAKESTRKEY(L"ReactOS");
#endif
    }

#if DBG
    if (IsUnattendedSetup)
#endif
    {
        /* Timeout=0 for unattended or non debug */
        BootOptions.Timeout = 0;
    }
#if DBG
    else
    {
        /* Timeout=10 */
        BootOptions.Timeout = 10;
    }
#endif

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
    LockStatus = NtFsControlFile(PartitionHandle, NULL, NULL, NULL, &IoStatusBlock, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);
    if (!NT_SUCCESS(LockStatus))
    {
        DPRINT1("Unable to lock the volume before installing boot code. Status 0x%08x. Expect problems.\n", LockStatus);
    }

    /* Install the bootcode (MBR, VBR) */
    Status = InstallBootCode(SrcPath, PartitionHandle, PartitionHandle);

    /* dismount & Unlock the volume */
    if (NT_SUCCESS(LockStatus))
    {
        LockStatus = NtFsControlFile(PartitionHandle, NULL, NULL, NULL, &IoStatusBlock, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);
        if (!NT_SUCCESS(LockStatus))
        {
            DPRINT1("Unable to dismount the volume after installing boot code. Status 0x%08x. Expect problems.\n", LockStatus);
        }

        LockStatus = NtFsControlFile(PartitionHandle, NULL, NULL, NULL, &IoStatusBlock, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);
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
                 SourceRootPath->Buffer, L"\\loader\\dosmbr.bin");

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

    DPRINT1("Install MBR bootcode: %S ==> %S\n",
            SourceMbrPathBuffer, DestinationDevicePathBuffer);

    /* Install the MBR */
    return InstallBootCodeToDisk(SourceMbrPathBuffer,
                                 DestinationDevicePathBuffer,
                                 InstallMbrBootCode);
}


static
NTSTATUS
InstallFatBootcodeToPartition(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath,
    IN PCWSTR FileSystemName)
{
    NTSTATUS Status;
    BOOLEAN DoesFreeLdrExist;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* FAT or FAT32 partition */
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
    }

    /* Check for NT and other bootloaders */

    // FIXME: Check for Vista+ bootloader!
    /*** Status = FindBootStore(PartitionHandle, NtLdr, &Version); ***/
    /*** Status = FindBootStore(PartitionHandle, BootMgr, &Version); ***/
    if (DoesFileExist_2(SystemRootPath->Buffer, L"NTLDR") == TRUE ||
        DoesFileExist_2(SystemRootPath->Buffer, L"BOOT.INI") == TRUE)
    {
        /* Search root directory for 'NTLDR' and 'BOOT.INI' */
        DPRINT1("Found Microsoft Windows NT/2000/XP boot loader\n");

        /* Create or update 'freeldr.ini' */
        if (DoesFreeLdrExist == FALSE)
        {
            /* Create new 'freeldr.ini' */
            DPRINT1("Create new 'freeldr.ini'\n");
            Status = CreateFreeLoaderIniForReactOS(SystemRootPath->Buffer, DestinationArcPath->Buffer);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
                return Status;
            }

            /* Install new bootcode into a file */
            CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"bootsect.ros");

            if (wcsicmp(FileSystemName, L"FAT32") == 0)
            {
                /* Install FAT32 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat32.bin");

                DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, DstPath);
                Status = InstallBootCodeToFile(SrcPath, DstPath,
                                               SystemRootPath->Buffer,
                                               InstallFat32BootCode);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallBootCodeToFile(FAT32) failed (Status %lx)\n", Status);
                    return Status;
                }
            }
            else // if (wcsicmp(FileSystemName, L"FAT") == 0)
            {
                /* Install FAT16 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");

                DPRINT1("Install FAT16 bootcode: %S ==> %S\n", SrcPath, DstPath);
                Status = InstallBootCodeToFile(SrcPath, DstPath,
                                               SystemRootPath->Buffer,
                                               InstallFat16BootCode);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallBootCodeToFile(FAT16) failed (Status %lx)\n", Status);
                    return Status;
                }
            }
        }

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
    }
    else
    {
        /* Non-NT bootloaders: install our own bootloader */

        PCWSTR Section;
        PCWSTR Description;
        PCWSTR BootDrive;
        PCWSTR BootPartition;
        PCWSTR BootSector;

        /* Search for COMPAQ MS-DOS 1.x (1.11, 1.12, based on MS-DOS 1.25) boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"IOSYS.COM") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"MSDOS.COM") == TRUE)
        {
            DPRINT1("Found COMPAQ MS-DOS 1.x (1.11, 1.12) / MS-DOS 1.25 boot loader\n");

            Section       = L"CPQDOS";
            Description   = L"\"COMPAQ MS-DOS 1.x / MS-DOS 1.25\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.DOS";
        }
        else
        /* Search for Microsoft DOS or Windows 9x boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"IO.SYS") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"MSDOS.SYS") == TRUE)
            // WINBOOT.SYS
        {
            DPRINT1("Found Microsoft DOS or Windows 9x boot loader\n");

            Section       = L"MSDOS";
            Description   = L"\"MS-DOS/Windows\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.DOS";
        }
        else
        /* Search for IBM PC-DOS or DR-DOS 5.x boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"IBMIO.COM" ) == TRUE || // Some people refer to this file instead of IBMBIO.COM...
            DoesFileExist_2(SystemRootPath->Buffer, L"IBMBIO.COM") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"IBMDOS.COM") == TRUE)
        {
            DPRINT1("Found IBM PC-DOS or DR-DOS 5.x or IBM OS/2 1.0\n");

            Section       = L"IBMDOS";
            Description   = L"\"IBM PC-DOS or DR-DOS 5.x or IBM OS/2 1.0\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.DOS";
        }
        else
        /* Search for DR-DOS 3.x boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"DRBIOS.SYS") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"DRBDOS.SYS") == TRUE)
        {
            DPRINT1("Found DR-DOS 3.x\n");

            Section       = L"DRDOS";
            Description   = L"\"DR-DOS 3.x\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.DOS";
        }
        else
        /* Search for Dell Real-Mode Kernel (DRMK) OS */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"DELLBIO.BIN") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"DELLRMK.BIN") == TRUE)
        {
            DPRINT1("Found Dell Real-Mode Kernel OS\n");

            Section       = L"DRMK";
            Description   = L"\"Dell Real-Mode Kernel OS\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.DOS";
        }
        else
        /* Search for MS OS/2 1.x */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"OS2BOOT.COM") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2BIO.COM" ) == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2DOS.COM" ) == TRUE)
        {
            DPRINT1("Found MS OS/2 1.x\n");

            Section       = L"MSOS2";
            Description   = L"\"MS OS/2 1.x\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.OS2";
        }
        else
        /* Search for MS or IBM OS/2 */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"OS2BOOT") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2LDR" ) == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2KRNL") == TRUE)
        {
            DPRINT1("Found MS/IBM OS/2\n");

            Section       = L"IBMOS2";
            Description   = L"\"MS/IBM OS/2\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.OS2";
        }
        else
        /* Search for FreeDOS boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"kernel.sys") == TRUE)
        {
            DPRINT1("Found FreeDOS boot loader\n");

            Section       = L"FDOS";
            Description   = L"\"FreeDOS\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.DOS";
        }
        else
        {
            /* No or unknown boot loader */
            DPRINT1("No or unknown boot loader found\n");

            Section       = L"Unknown";
            Description   = L"\"Unknown Operating System\"";
            BootDrive     = L"hd0";
            BootPartition = L"1";
            BootSector    = L"BOOTSECT.OLD";
        }

        /* Create or update 'freeldr.ini' */
        if (DoesFreeLdrExist == FALSE)
        {
            /* Create new 'freeldr.ini' */
            DPRINT1("Create new 'freeldr.ini'\n");

            if (IsThereAValidBootSector(SystemRootPath->Buffer))
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

                /* Save current bootsector */
                CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, BootSector);

                DPRINT1("Save bootsector: %S ==> %S\n", SystemRootPath->Buffer, DstPath);
                Status = SaveBootSector(SystemRootPath->Buffer, DstPath, SECTORSIZE);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("SaveBootSector() failed (Status %lx)\n", Status);
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
            if (wcsicmp(FileSystemName, L"FAT32") == 0)
            {
                /* Install FAT32 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat32.bin");

                DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
                Status = InstallBootCodeToDisk(SrcPath, SystemRootPath->Buffer, InstallFat32BootCode);
                DPRINT1("Status: 0x%08X\n", Status);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallBootCodeToDisk(FAT32) failed (Status %lx)\n", Status);
                    return Status;
                }
            }
            else // if (wcsicmp(FileSystemName, L"FAT") == 0)
            {
                /* Install FAT16 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");

                DPRINT1("Install FAT16 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
                Status = InstallBootCodeToDisk(SrcPath, SystemRootPath->Buffer, InstallFat16BootCode);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallBootCodeToDisk(FAT16) failed (Status %lx)\n", Status);
                    return Status;
                }
            }
        }
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
InstallBtrfsBootcodeToPartition(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath)
{
    NTSTATUS Status;
    BOOLEAN DoesFreeLdrExist;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* BTRFS partition */
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
    }

    /* Check for *nix bootloaders */

    /* Create or update 'freeldr.ini' */
    if (DoesFreeLdrExist == FALSE)
    {
        /* Create new 'freeldr.ini' */
        DPRINT1("Create new 'freeldr.ini'\n");

        /* Certainly SysLinux, GRUB, LILO... or an unknown boot loader */
        DPRINT1("*nix or unknown boot loader found\n");

        if (IsThereAValidBootSector(SystemRootPath->Buffer))
        {
            PCWSTR BootSector = L"BOOTSECT.OLD";

            Status = CreateFreeLoaderIniForReactOSAndBootSector(
                         SystemRootPath->Buffer, DestinationArcPath->Buffer,
                         L"Linux", L"\"Linux\"",
                         L"hd0", L"1", BootSector);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateFreeLoaderIniForReactOSAndBootSector() failed (Status %lx)\n", Status);
                return Status;
            }

            /* Save current bootsector */
            CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, BootSector);

            DPRINT1("Save bootsector: %S ==> %S\n", SystemRootPath->Buffer, DstPath);
            Status = SaveBootSector(SystemRootPath->Buffer, DstPath, BTRFS_BOOTSECTOR_SIZE);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SaveBootSector() failed (Status %lx)\n", Status);
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
        /* Install BTRFS bootcode */
        CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\btrfs.bin");

        DPRINT1("Install BTRFS bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
        Status = InstallBootCodeToDisk(SrcPath, SystemRootPath->Buffer, InstallBtrfsBootCode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallBootCodeToDisk(BTRFS) failed (Status %lx)\n", Status);
            return Status;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
InstallVBRToPartition(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath,
    IN PCWSTR FileSystemName)
{
    if (wcsicmp(FileSystemName, L"FAT")   == 0 ||
        wcsicmp(FileSystemName, L"FAT32") == 0)
    {
        return InstallFatBootcodeToPartition(SystemRootPath,
                                             SourceRootPath,
                                             DestinationArcPath,
                                             FileSystemName);
    }
    /*
    else if (wcsicmp(FileSystemName, L"NTFS") == 0)
    {
        DPRINT1("Partitions of type NTFS or HPFS are not supported yet!\n");
        return STATUS_NOT_SUPPORTED;
    }
    */
    else if (wcsicmp(FileSystemName, L"BTRFS") == 0)
    {
        return InstallBtrfsBootcodeToPartition(SystemRootPath,
                                               SourceRootPath,
                                               DestinationArcPath);
    }
    /*
    else if (wcsicmp(FileSystemName, L"EXT2")  == 0 ||
             wcsicmp(FileSystemName, L"EXT3")  == 0 ||
             wcsicmp(FileSystemName, L"EXT4")  == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }
    */
    else
    {
        /* Unknown file system */
        DPRINT1("Unknown file system '%S'\n", FileSystemName);
    }

    return STATUS_NOT_SUPPORTED;
}


NTSTATUS
InstallFatBootcodeToFloppy(
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath)
{
    static const PCWSTR FloppyDevice = L"\\Device\\Floppy0\\";

    NTSTATUS Status;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Verify that the floppy disk is accessible */
    if (DoesDirExist(NULL, FloppyDevice) == FALSE)
        return STATUS_DEVICE_NOT_READY;

    /* Format the floppy disk */
    // FormatPartition(...)
    Status = FormatFileSystem(FloppyDevice,
                              L"FAT",
                              FMIFS_FLOPPY,
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
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, FloppyDevice, L"freeldr.sys");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Create new 'freeldr.ini' */
    DPRINT("Create new 'freeldr.ini'\n");
    Status = CreateFreeLoaderIniForReactOS(FloppyDevice, DestinationArcPath->Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Install FAT12 boosector */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 1, FloppyDevice);

    DPRINT("Install FAT12 bootcode: %S ==> %S\n", SrcPath, DstPath);
    Status = InstallBootCodeToDisk(SrcPath, DstPath, InstallFat12BootCode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallBootCodeToDisk(FAT12) failed (Status %lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

/* EOF */
