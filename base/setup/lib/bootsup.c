/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Bootloader support functions
 * COPYRIGHT:   ...
 *              Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include <ntddstor.h> // For STORAGE_DEVICE_NUMBER

#include "bldrsup.h"
#include "devutils.h"
#include "filesup.h"
#include "partlist.h"
#include "bootcode.h"
#include "fsutil.h"

#include "setuplib.h" // HACK for IsUnattendedSetup

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
    Options->OsLoadOptions  = L"/FASTDETECT";
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
        BootOptions.NextBootEntryKey = MAKESTRKEY(L"ReactOS_KdSerial");
    }
    else
#endif
    {
#if DBG
        BootOptions.NextBootEntryKey = MAKESTRKEY(L"ReactOS_Debug");
#else
        BootOptions.NextBootEntryKey = MAKESTRKEY(L"ReactOS");
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

    SetBootStoreOptions(BootStoreHandle, &BootOptions,
                        BOOT_OPTIONS_TIMEOUT | BOOT_OPTIONS_NEXT_BOOTENTRY_KEY);
}

static NTSTATUS
CreateFreeLoaderIniForReactOS(
    IN PCWSTR IniPath,
    IN PCWSTR ArcPath)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;

    /* Initialize the INI file and create the common FreeLdr sections */
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr,
                           BS_CreateAlways /* BS_OpenAlways */, BS_ReadWriteAccess);
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
    IN PCWSTR BootPath,
    IN PCWSTR BootSector)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(BOOTSECTOR_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PBOOTSECTOR_OPTIONS Options = (PBOOTSECTOR_OPTIONS)&BootEntry->OsOptions;
    WCHAR BootPathBuffer[MAX_PATH] = L"";

    /* Since the BootPath given here is in NT format
     * (not ARC), we need to hack-generate a mapping */
    ULONG DiskNumber = 0, PartitionNumber = 0;
    PCWSTR PathComponent = NULL;

    /* From the NT path, compute the disk, partition and path components */
    // NOTE: this function doesn't support stuff like \Device\FloppyX ...
    if (NtPathToDiskPartComponents(BootPath, &DiskNumber, &PartitionNumber, &PathComponent))
    {
        DPRINT1("BootPath = '%S' points to disk #%d, partition #%d, path '%S'\n",
               BootPath, DiskNumber, PartitionNumber, PathComponent);

        /* HACK-build a possible ARC path:
         * Hard disk path: multi(0)disk(0)rdisk(x)partition(y)[\path] */
        RtlStringCchPrintfW(BootPathBuffer, _countof(BootPathBuffer),
                            L"multi(0)disk(0)rdisk(%lu)partition(%lu)",
                            DiskNumber, PartitionNumber);
        if (PathComponent && *PathComponent &&
            (PathComponent[0] != L'\\' || PathComponent[1]))
        {
            RtlStringCchCatW(BootPathBuffer, _countof(BootPathBuffer),
                             PathComponent);
        }
    }
    else
    {
        PCWSTR Path = BootPath;

        if ((_wcsnicmp(Path, L"\\Device\\Floppy", 14) == 0) &&
            (Path += 14) && iswdigit(*Path))
        {
            DiskNumber = wcstoul(Path, (PWSTR*)&PathComponent, 10);
            if (PathComponent && *PathComponent && *PathComponent != L'\\')
                PathComponent = NULL;

            /* HACK-build a possible ARC path:
             * Floppy disk path: multi(0)disk(0)fdisk(x)[\path] */
            RtlStringCchPrintfW(BootPathBuffer, _countof(BootPathBuffer),
                                L"multi(0)disk(0)fdisk(%lu)", DiskNumber);
            if (PathComponent && *PathComponent &&
                (PathComponent[0] != L'\\' || PathComponent[1]))
            {
                RtlStringCchCatW(BootPathBuffer, _countof(BootPathBuffer),
                                 PathComponent);
            }
        }
        else
        {
            /* HACK: Just keep the unresolved NT path and hope for the best... */

            /* Remove any trailing backslash if needed */
            UNICODE_STRING RootPartition;
            RtlInitUnicodeString(&RootPartition, BootPath);
            TrimTrailingPathSeparators_UStr(&RootPartition);

            /* RootPartition is BootPath without counting any trailing
             * path separator. Because of this, we need to copy the string
             * in the buffer, instead of just using a pointer to it. */
            RtlStringCchPrintfW(BootPathBuffer, _countof(BootPathBuffer),
                                L"%wZ", &RootPartition);

            DPRINT1("Unhandled NT path '%S'\n", BootPath);
        }
    }

    /* Initialize the INI file and create the common FreeLdr sections */
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr,
                           BS_CreateAlways /* BS_OpenAlways */, BS_ReadWriteAccess);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Add the ReactOS entries */
    CreateFreeLoaderReactOSEntries(BootStoreHandle, ArcPath);

    BootEntry->Version = FreeLdr;
    BootEntry->BootFilePath = NULL;

    BootEntry->OsOptionsLength = sizeof(BOOTSECTOR_OPTIONS);
    RtlCopyMemory(Options->Signature,
                  BOOTSECTOR_OPTIONS_SIGNATURE,
                  RTL_FIELD_SIZE(BOOTSECTOR_OPTIONS, Signature));

    Options->BootPath = BootPathBuffer;
    Options->FileName = BootSector;

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
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr,
                           BS_OpenExisting /* BS_OpenAlways */, BS_ReadWriteAccess);
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
    Status = OpenBootStore(&BootStoreHandle, IniPath, NtLdr,
                           BS_OpenExisting /* BS_OpenAlways */, BS_ReadWriteAccess);
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

static
NTSTATUS
InstallMbrBootCodeToDisk(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCWSTR DestinationDevicePathBuffer)
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
InstallBootloaderFiles(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath)
{
    NTSTATUS Status;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Copy FreeLoader to the system partition, always overwriting the older version */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\freeldr.sys");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"freeldr.sys");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    /* Copy rosload to the system partition, always overwriting the older version */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\rosload.exe");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"rosload.exe");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
InstallFatBootcodeToPartition(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ PCWSTR FileSystemName)
{
    NTSTATUS Status;
    BOOLEAN DoesFreeLdrExist;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* FAT or FAT32 partition */
    DPRINT("System path: '%wZ'\n", SystemRootPath);

    /* Install the bootloader */
    Status = InstallBootloaderFiles(SystemRootPath, SourceRootPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallBootloaderFiles() failed (Status %lx)\n", Status);
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

            if (_wcsicmp(FileSystemName, L"FAT32") == 0)
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
        PCWSTR BootSector;

        /* Search for COMPAQ MS-DOS 1.x (1.11, 1.12, based on MS-DOS 1.25) boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"IOSYS.COM") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"MSDOS.COM") == TRUE)
        {
            DPRINT1("Found COMPAQ MS-DOS 1.x (1.11, 1.12) / MS-DOS 1.25 boot loader\n");

            Section     = L"CPQDOS";
            Description = L"\"COMPAQ MS-DOS 1.x / MS-DOS 1.25\"";
            BootSector  = L"BOOTSECT.DOS";
        }
        else
        /* Search for Microsoft DOS or Windows 9x boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"IO.SYS") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"MSDOS.SYS") == TRUE)
            // WINBOOT.SYS
        {
            DPRINT1("Found Microsoft DOS or Windows 9x boot loader\n");

            Section     = L"MSDOS";
            Description = L"\"MS-DOS/Windows\"";
            BootSector  = L"BOOTSECT.DOS";
        }
        else
        /* Search for IBM PC-DOS or DR-DOS 5.x boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"IBMIO.COM" ) == TRUE || // Some people refer to this file instead of IBMBIO.COM...
            DoesFileExist_2(SystemRootPath->Buffer, L"IBMBIO.COM") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"IBMDOS.COM") == TRUE)
        {
            DPRINT1("Found IBM PC-DOS or DR-DOS 5.x or IBM OS/2 1.0\n");

            Section     = L"IBMDOS";
            Description = L"\"IBM PC-DOS or DR-DOS 5.x or IBM OS/2 1.0\"";
            BootSector  = L"BOOTSECT.DOS";
        }
        else
        /* Search for DR-DOS 3.x boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"DRBIOS.SYS") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"DRBDOS.SYS") == TRUE)
        {
            DPRINT1("Found DR-DOS 3.x\n");

            Section     = L"DRDOS";
            Description = L"\"DR-DOS 3.x\"";
            BootSector  = L"BOOTSECT.DOS";
        }
        else
        /* Search for Dell Real-Mode Kernel (DRMK) OS */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"DELLBIO.BIN") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"DELLRMK.BIN") == TRUE)
        {
            DPRINT1("Found Dell Real-Mode Kernel OS\n");

            Section     = L"DRMK";
            Description = L"\"Dell Real-Mode Kernel OS\"";
            BootSector  = L"BOOTSECT.DOS";
        }
        else
        /* Search for MS OS/2 1.x */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"OS2BOOT.COM") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2BIO.COM" ) == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2DOS.COM" ) == TRUE)
        {
            DPRINT1("Found MS OS/2 1.x\n");

            Section     = L"MSOS2";
            Description = L"\"MS OS/2 1.x\"";
            BootSector  = L"BOOTSECT.OS2";
        }
        else
        /* Search for MS or IBM OS/2 */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"OS2BOOT") == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2LDR" ) == TRUE ||
            DoesFileExist_2(SystemRootPath->Buffer, L"OS2KRNL") == TRUE)
        {
            DPRINT1("Found MS/IBM OS/2\n");

            Section     = L"IBMOS2";
            Description = L"\"MS/IBM OS/2\"";
            BootSector  = L"BOOTSECT.OS2";
        }
        else
        /* Search for FreeDOS boot loader */
        if (DoesFileExist_2(SystemRootPath->Buffer, L"kernel.sys") == TRUE)
        {
            DPRINT1("Found FreeDOS boot loader\n");

            Section     = L"FDOS";
            Description = L"\"FreeDOS\"";
            BootSector  = L"BOOTSECT.DOS";
        }
        else
        {
            /* No or unknown boot loader */
            DPRINT1("No or unknown boot loader found\n");

            Section     = L"Unknown";
            Description = L"\"Unknown Operating System\"";
            BootSector  = L"BOOTSECT.OLD";
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
                             SystemRootPath->Buffer, BootSector);
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
            if (_wcsicmp(FileSystemName, L"FAT32") == 0)
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
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath)
{
    NTSTATUS Status;
    BOOLEAN DoesFreeLdrExist;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* BTRFS partition */
    DPRINT("System path: '%wZ'\n", SystemRootPath);

    /* Install the bootloader */
    Status = InstallBootloaderFiles(SystemRootPath, SourceRootPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallBootloaderFiles() failed (Status %lx)\n", Status);
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
                         SystemRootPath->Buffer, BootSector);
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

static
NTSTATUS
InstallNtfsBootcodeToPartition(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath)
{
    NTSTATUS Status;
    BOOLEAN DoesFreeLdrExist;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* NTFS partition */
    DPRINT("System path: '%wZ'\n", SystemRootPath);

    /* Install the bootloader */
    Status = InstallBootloaderFiles(SystemRootPath, SourceRootPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallBootloaderFiles() failed (Status %lx)\n", Status);
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

        return STATUS_SUCCESS;
    }

    /* Check for *nix bootloaders */

    DPRINT1("Create new 'freeldr.ini'\n");

    /* Certainly SysLinux, GRUB, LILO... or an unknown boot loader */
    DPRINT1("*nix or unknown boot loader found\n");

    if (IsThereAValidBootSector(SystemRootPath->Buffer))
    {
        PCWSTR BootSector = L"BOOTSECT.OLD";

        Status = CreateFreeLoaderIniForReactOSAndBootSector(
                     SystemRootPath->Buffer, DestinationArcPath->Buffer,
                     L"Linux", L"\"Linux\"",
                     SystemRootPath->Buffer, BootSector);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateFreeLoaderIniForReactOSAndBootSector() failed (Status %lx)\n", Status);
            return Status;
        }

        /* Save current bootsector */
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, BootSector);

        DPRINT1("Save bootsector: %S ==> %S\n", SystemRootPath->Buffer, DstPath);
        Status = SaveBootSector(SystemRootPath->Buffer, DstPath, NTFS_BOOTSECTOR_SIZE);
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

    /* Install NTFS bootcode */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\ntfs.bin");

    DPRINT1("Install NTFS bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
    Status = InstallBootCodeToDisk(SrcPath, SystemRootPath->Buffer, InstallNtfsBootCode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallBootCodeToDisk(NTFS) failed (Status %lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
InstallVBRToPartition(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ PCWSTR FileSystemName)
{
    if (_wcsicmp(FileSystemName, L"FAT")   == 0 ||
        _wcsicmp(FileSystemName, L"FAT32") == 0)
    {
        return InstallFatBootcodeToPartition(SystemRootPath,
                                             SourceRootPath,
                                             DestinationArcPath,
                                             FileSystemName);
    }
    else if (_wcsicmp(FileSystemName, L"NTFS") == 0)
    {
        return InstallNtfsBootcodeToPartition(SystemRootPath,
                                              SourceRootPath,
                                              DestinationArcPath);
    }
    else if (_wcsicmp(FileSystemName, L"BTRFS") == 0)
    {
        return InstallBtrfsBootcodeToPartition(SystemRootPath,
                                               SourceRootPath,
                                               DestinationArcPath);
    }
    /*
    else if (_wcsicmp(FileSystemName, L"EXT2")  == 0 ||
             _wcsicmp(FileSystemName, L"EXT3")  == 0 ||
             _wcsicmp(FileSystemName, L"EXT4")  == 0)
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


/* GENERIC FUNCTIONS *********************************************************/

/**
 * @brief
 * Helper for InstallBootManagerAndBootEntries().
 *
 * @param[in]   ArchType
 * @param[in]   SystemRootPath
 * See InstallBootManagerAndBootEntries() parameters.
 *
 * @param[in]   DiskNumber
 * The NT disk number of the system disk that contains the system partition.
 *
 * @param[in]   DiskStyle
 * The partitioning style of the system disk.
 *
 * @param[in]   IsSuperFloppy
 * Whether the system disk is a super-floppy.
 *
 * @param[in]   FileSystem
 * The file system of the system partition.
 *
 * @param[in]   SourceRootPath
 * @param[in]   DestinationArcPath
 * @param[in]   Options
 * See InstallBootManagerAndBootEntries() parameters.
 *
 * @return  An NTSTATUS code indicating success or failure.
 **/
static
NTSTATUS
InstallBootManagerAndBootEntriesWorker(
    _In_ ARCHITECTURE_TYPE ArchType,
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ ULONG DiskNumber, // const STORAGE_DEVICE_NUMBER* DeviceNumber,
    _In_ PARTITION_STYLE DiskStyle,
    _In_ BOOLEAN IsSuperFloppy,
    _In_ PCWSTR FileSystem,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ ULONG_PTR Options)
{
    NTSTATUS Status;
    BOOLEAN IsBIOS = ((ArchType == ARCH_PcAT) || (ArchType == ARCH_NEC98x86));
    UCHAR InstallType = (Options & 0x03);

    // FIXME: We currently only support BIOS-based PCs
    // TODO: Support other platforms
    if (!IsBIOS)
        return STATUS_NOT_SUPPORTED;

    if (InstallType <= 1)
    {
        /* Step 1: Write the VBR */
        Status = InstallVBRToPartition(SystemRootPath,
                                       SourceRootPath,
                                       DestinationArcPath,
                                       FileSystem);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallVBRToPartition() failed (Status 0x%08lx)\n", Status);
            return ERROR_WRITE_BOOT; // Status; STATUS_BAD_MASTER_BOOT_RECORD;
        }

        /* Step 2: Write the MBR if the disk containing the
         * system partition is MBR and not a super-floppy */
        if ((InstallType == 1) && (DiskStyle == PARTITION_STYLE_MBR) && !IsSuperFloppy)
        {
            WCHAR SystemDiskPath[MAX_PATH];
            RtlStringCchPrintfW(SystemDiskPath, _countof(SystemDiskPath),
                                L"\\Device\\Harddisk%d\\Partition0",
                                DiskNumber);
            Status = InstallMbrBootCodeToDisk(SystemRootPath,
                                              SourceRootPath,
                                              SystemDiskPath);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("InstallMbrBootCodeToDisk() failed (Status 0x%08lx)\n", Status);
                return ERROR_INSTALL_BOOTCODE; // Status; STATUS_BAD_MASTER_BOOT_RECORD;
            }
        }
    }
    else if (InstallType == 2)
    {
        WCHAR SrcPath[MAX_PATH];

        // FIXME: We currently only support FAT12 file system.
        if (_wcsicmp(FileSystem, L"FAT") != 0)
            return STATUS_NOT_SUPPORTED;

        // TODO: In the future, we'll be able to use InstallVBRToPartition()
        // directly, instead of re-doing manually the copy steps below.

        /* Install the bootloader to the boot partition */
        Status = InstallBootloaderFiles(SystemRootPath, SourceRootPath);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallBootloaderFiles() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        /* Create new 'freeldr.ini' */
        DPRINT("Create new 'freeldr.ini'\n");
        Status = CreateFreeLoaderIniForReactOS(SystemRootPath->Buffer, DestinationArcPath->Buffer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status 0x%08lx)\n", Status);
            return Status;
        }

        /* Install FAT12 bootsector */
        CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");

        DPRINT1("Install FAT12 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
        Status = InstallBootCodeToDisk(SrcPath, SystemRootPath->Buffer, InstallFat12BootCode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallBootCodeToDisk(FAT12) failed (Status 0x%08lx)\n", Status);
            return Status;
        }
    }

    return Status;
}


NTSTATUS
GetDeviceInfo_UStr(
    _In_opt_ PCUNICODE_STRING DeviceName,
    _In_opt_ HANDLE DeviceHandle,
    _Out_ PFILE_FS_DEVICE_INFORMATION DeviceInfo)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;

    if (DeviceName && DeviceHandle)
        return STATUS_INVALID_PARAMETER_MIX;

    /* Open the device if a name has been given;
     * otherwise just use the provided handle. */
    if (DeviceName)
    {
        Status = pOpenDeviceEx_UStr(DeviceName, &DeviceHandle,
                                    FILE_READ_ATTRIBUTES,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Cannot open device '%wZ' (Status 0x%08lx)\n",
                    DeviceName, Status);
            return Status;
        }
    }

    /* Query the device */
    Status = NtQueryVolumeInformationFile(DeviceHandle,
                                          &IoStatusBlock,
                                          DeviceInfo,
                                          sizeof(*DeviceInfo),
                                          FileFsDeviceInformation);
    if (!NT_SUCCESS(Status))
        DPRINT1("FileFsDeviceInformation failed (Status 0x%08lx)\n", Status);

    /* Close the device if we've opened it */
    if (DeviceName)
        NtClose(DeviceHandle);

    return Status;
}

NTSTATUS
GetDeviceInfo(
    _In_opt_ PCWSTR DeviceName,
    _In_opt_ HANDLE DeviceHandle,
    _Out_ PFILE_FS_DEVICE_INFORMATION DeviceInfo)
{
    UNICODE_STRING DeviceNameU;

    if (DeviceName && DeviceHandle)
        return STATUS_INVALID_PARAMETER_MIX;

    if (DeviceName)
        RtlInitUnicodeString(&DeviceNameU, DeviceName);

    return GetDeviceInfo_UStr(DeviceName ? &DeviceNameU : NULL,
                              DeviceName ? NULL : DeviceHandle,
                              DeviceInfo);
}


/**
 * @brief
 * Installs FreeLoader on the system and configure the boot entries.
 *
 * @todo
 * Split this function into just the InstallBootManager, and a separate one
 * for just the boot entries.
 *
 * @param[in]   ArchType
 * The target architecture.
 *
 * @param[in]   SystemRootPath
 * The system partition path, where the FreeLdr boot manager and its
 * settings are saved to.
 *
 * @param[in]   SourceRootPath
 * The installation source, where to copy the FreeLdr boot manager from.
 *
 * @param[in]   DestinationArcPath
 * The ReactOS installation path in ARC format.
 *
 * @param[in]   Options
 * For BIOS-based PCs:
 * LOBYTE:
 *      0: Install only on VBR;
 *      1: Install on both VBR and MBR.
 *      2: Install on removable disk.
 *
 * @return  An NTSTATUS code indicating success or failure.
 **/
NTSTATUS
NTAPI
InstallBootManagerAndBootEntries(
    _In_ ARCHITECTURE_TYPE ArchType,
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ ULONG_PTR Options)
{
    NTSTATUS Status;
    HANDLE DeviceHandle;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    ULONG DiskNumber;
    PARTITION_STYLE PartitionStyle;
    BOOLEAN IsSuperFloppy;
    WCHAR FileSystem[MAX_PATH+1];

    /* Remove any trailing backslash if needed */
    UNICODE_STRING RootPartition = *SystemRootPath;
    TrimTrailingPathSeparators_UStr(&RootPartition);

    /* Open the volume */
    Status = pOpenDeviceEx_UStr(&RootPartition, &DeviceHandle,
                                GENERIC_READ,
                                FILE_SHARE_READ | FILE_SHARE_WRITE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Cannot open %wZ for bootloader installation (Status 0x%08lx)\n",
                &RootPartition, Status);
        return Status;
    }

    /* Retrieve the volume file system (it will also be mounted) */
    Status = GetFileSystemName_UStr(NULL, DeviceHandle,
                                    FileSystem, sizeof(FileSystem));
    if (!NT_SUCCESS(Status) || !*FileSystem)
    {
        DPRINT1("GetFileSystemName() failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* Retrieve the device type and characteristics */
    Status = GetDeviceInfo_UStr(NULL, DeviceHandle, &DeviceInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("FileFsDeviceInformation failed (Status 0x%08lx)\n", Status);
        goto Quit;
    }

    /* Ignore volumes that are NOT on usual disks */
    if (DeviceInfo.DeviceType != FILE_DEVICE_DISK /*&&
        DeviceInfo.DeviceType != FILE_DEVICE_VIRTUAL_DISK*/)
    {
        DPRINT1("Invalid volume; device type %lu\n", DeviceInfo.DeviceType);
        Status = STATUS_INVALID_DEVICE_REQUEST;
        goto Quit;
    }


    /* Check whether this is a floppy or a partitionable device */
    if (DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE)
    {
        /* Floppies don't have partitions */
        // NOTE: See ntoskrnl/io/iomgr/rawfs.c!RawQueryFsSizeInfo()
        DiskNumber = ULONG_MAX;
        PartitionStyle = PARTITION_STYLE_MBR;
        IsSuperFloppy = TRUE;
    }
    else
    {
        IO_STATUS_BLOCK IoStatusBlock;
        STORAGE_DEVICE_NUMBER DeviceNumber;

        /* The maximum information a DISK_GEOMETRY_EX dynamic structure can contain */
        typedef struct _DISK_GEOMETRY_EX_INTERNAL
        {
            DISK_GEOMETRY Geometry;
            LARGE_INTEGER DiskSize;
            DISK_PARTITION_INFO Partition;
            /* Followed by: DISK_DETECTION_INFO Detection; unused here */
        } DISK_GEOMETRY_EX_INTERNAL, *PDISK_GEOMETRY_EX_INTERNAL;

        DISK_GEOMETRY_EX_INTERNAL DiskGeoEx;
        PARTITION_INFORMATION PartitionInfo;

        /* Retrieve the disk number. NOTE: Fails for floppy disks. */
        Status = NtDeviceIoControlFile(DeviceHandle,
                                       NULL, NULL, NULL,
                                       &IoStatusBlock,
                                       IOCTL_STORAGE_GET_DEVICE_NUMBER,
                                       NULL, 0,
                                       &DeviceNumber, sizeof(DeviceNumber));
        if (!NT_SUCCESS(Status))
            goto Quit; /* This may be a dynamic volume, which is unsupported */
        ASSERT(DeviceNumber.DeviceType == DeviceInfo.DeviceType);
        if (DeviceNumber.DeviceNumber == ULONG_MAX)
        {
            DPRINT1("Invalid disk number reported, bail out\n");
            Status = STATUS_NOT_FOUND;
            goto Quit;
        }

        /* Retrieve the drive geometry. NOTE: Fails for floppy disks;
         * use IOCTL_DISK_GET_DRIVE_GEOMETRY instead. */
        Status = NtDeviceIoControlFile(DeviceHandle,
                                       NULL, NULL, NULL,
                                       &IoStatusBlock,
                                       IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
                                       NULL, 0,
                                       &DiskGeoEx,
                                       sizeof(DiskGeoEx));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IOCTL_DISK_GET_DRIVE_GEOMETRY_EX failed (Status 0x%08lx)\n", Status);
            goto Quit;
        }

        /*
         * Retrieve the volume's partition information.
         * NOTE: Fails for floppy disks.
         *
         * NOTE: We can use the non-EX IOCTL because the super-floppy test will
         * fail anyway if the disk is NOT MBR-partitioned. (If the disk is GPT,
         * the IOCTL would return only the MBR protective partition, but the
         * super-floppy test would fail due to the wrong partitioning style.)
         */
        Status = NtDeviceIoControlFile(DeviceHandle,
                                       NULL, NULL, NULL,
                                       &IoStatusBlock,
                                       IOCTL_DISK_GET_PARTITION_INFO,
                                       NULL, 0,
                                       &PartitionInfo,
                                       sizeof(PartitionInfo));
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("IOCTL_DISK_GET_PARTITION_INFO failed (Status 0x%08lx)\n", Status);
            goto Quit;
        }

        DiskNumber = DeviceNumber.DeviceNumber;
        PartitionStyle = DiskGeoEx.Partition.PartitionStyle;
        IsSuperFloppy = IsDiskSuperFloppy2(&DiskGeoEx.Partition,
                                           (PULONGLONG)&DiskGeoEx.DiskSize.QuadPart,
                                           &PartitionInfo);
    }

    Status = InstallBootManagerAndBootEntriesWorker(
                ArchType, SystemRootPath,
                DiskNumber, PartitionStyle, IsSuperFloppy, FileSystem,
                SourceRootPath, DestinationArcPath, Options);

Quit:
    NtClose(DeviceHandle);
    return Status;
}

NTSTATUS
NTAPI
InstallBootcodeToRemovable(
    _In_ ARCHITECTURE_TYPE ArchType,
    _In_ PCUNICODE_STRING RemovableRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath)
{
    NTSTATUS Status;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    PCWSTR FileSystemName;
    BOOLEAN IsFloppy;

    /* Remove any trailing backslash if needed */
    UNICODE_STRING RootDrive = *RemovableRootPath;
    TrimTrailingPathSeparators_UStr(&RootDrive);

    /* Verify that the removable disk is accessible */
    if (!DoesDirExist(NULL, RemovableRootPath->Buffer))
        return STATUS_DEVICE_NOT_READY;

    /* Retrieve the device type and characteristics */
    Status = GetDeviceInfo_UStr(&RootDrive, NULL, &DeviceInfo);
    if (!NT_SUCCESS(Status))
    {
        static const UNICODE_STRING DeviceFloppy = RTL_CONSTANT_STRING(L"\\Device\\Floppy");

        DPRINT1("FileFsDeviceInformation failed (Status 0x%08lx)\n", Status);

        /* Definitively fail if the device is not a floppy */
        if (!RtlPrefixUnicodeString(&DeviceFloppy, &RootDrive, TRUE))
            return Status; /* We cannot cope with a failure */

        /* Try to fall back to something "sane" if the device may be a floppy */
        DeviceInfo.DeviceType = FILE_DEVICE_DISK;
        DeviceInfo.Characteristics = FILE_REMOVABLE_MEDIA | FILE_FLOPPY_DISKETTE;
    }

    /* Ignore volumes that are NOT on usual disks */
    if (DeviceInfo.DeviceType != FILE_DEVICE_DISK /*&&
        DeviceInfo.DeviceType != FILE_DEVICE_VIRTUAL_DISK*/)
    {
        DPRINT1("Invalid volume; device type %lu\n", DeviceInfo.DeviceType);
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Fail if the disk is not removable */
    if (!(DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA))
    {
        DPRINT1("Device is NOT removable!\n");
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    /* Check whether this is a floppy or another removable device */
    IsFloppy = !!(DeviceInfo.Characteristics & FILE_FLOPPY_DISKETTE);

    /* Use FAT32, unless the device is a floppy disk */
    FileSystemName = (IsFloppy ? L"FAT" : L"FAT32");

    /* Format the removable disk */
    Status = FormatFileSystem_UStr(&RootDrive,
                                   FileSystemName,
                                   (IsFloppy ? FMIFS_FLOPPY : FMIFS_REMOVABLE),
                                   NULL,
                                   TRUE,
                                   0,
                                   NULL);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_NOT_SUPPORTED)
            DPRINT1("%s FS non-existent on this system!\n", FileSystemName);
        else
            DPRINT1("FormatFileSystem(%s) failed (Status 0x%08lx)\n", FileSystemName, Status);
        return Status;
    }

    /* Copy FreeLoader to the removable disk and save the boot entries */
    Status = InstallBootManagerAndBootEntries(ArchType,
                                              RemovableRootPath,
                                              SourceRootPath,
                                              DestinationArcPath,
                                              2 /* Install on removable media */);
    if (!NT_SUCCESS(Status))
        DPRINT1("InstallBootManagerAndBootEntries() failed (Status 0x%08lx)\n", Status);
    return Status;
}

/* EOF */
