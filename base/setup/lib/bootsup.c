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
#include "fsutil.h"
#include "partlist.h"

#include "setuplib.h" // HAXX for IsUnattendedSetup!!

#include "bootsup.h"

#define NDEBUG
#include <debug.h>


/* TYPEDEFS *****************************************************************/

/*
 * BIG FIXME!!
 * ===========
 *
 * All that stuff *MUST* go into the fsutil.c module.
 * Indeed, all that relates to filesystem formatting details and as such
 * *MUST* be abstracted out from this module (bootsup.c).
 * However, bootsup.c can still deal with MBR code (actually it'll have
 * at some point to share or give it to partlist.c, because when we'll
 * support GPT disks, things will change a bit).
 * And, bootsup.c can still manage initializing / adding boot entries
 * into NTLDR and FREELDR, and installing the latter, and saving the old
 * MBR / boot sectors in files.
 */
#define SECTORSIZE 512

#include <pshpack1.h>
typedef struct _FAT_BOOTSECTOR
{
    UCHAR       JumpBoot[3];                // Jump instruction to boot code
    CHAR        OemName[8];                 // "MSWIN4.1" for MS formatted volumes
    USHORT      BytesPerSector;             // Bytes per sector
    UCHAR       SectorsPerCluster;          // Number of sectors in a cluster
    USHORT      ReservedSectors;            // Reserved sectors, usually 1 (the bootsector)
    UCHAR       NumberOfFats;               // Number of FAT tables
    USHORT      RootDirEntries;             // Number of root directory entries (fat12/16)
    USHORT      TotalSectors;               // Number of total sectors on the drive, 16-bit
    UCHAR       MediaDescriptor;            // Media descriptor byte
    USHORT      SectorsPerFat;              // Sectors per FAT table (fat12/16)
    USHORT      SectorsPerTrack;            // Number of sectors in a track
    USHORT      NumberOfHeads;              // Number of heads on the disk
    ULONG       HiddenSectors;              // Hidden sectors (sectors before the partition start like the partition table)
    ULONG       TotalSectorsBig;            // This field is the new 32-bit total count of sectors on the volume
    UCHAR       DriveNumber;                // Int 0x13 drive number (e.g. 0x80)
    UCHAR       Reserved1;                  // Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
    UCHAR       BootSignature;              // Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
    ULONG       VolumeSerialNumber;         // Volume serial number
    CHAR        VolumeLabel[11];            // Volume label. This field matches the 11-byte volume label recorded in the root directory
    CHAR        FileSystemType[8];          // One of the strings "FAT12   ", "FAT16   ", or "FAT     "

    UCHAR       BootCodeAndData[448];       // The remainder of the boot sector

    USHORT      BootSectorMagic;            // 0xAA55

} FAT_BOOTSECTOR, *PFAT_BOOTSECTOR;

typedef struct _FAT32_BOOTSECTOR
{
    UCHAR       JumpBoot[3];                // Jump instruction to boot code
    CHAR        OemName[8];                 // "MSWIN4.1" for MS formatted volumes
    USHORT      BytesPerSector;             // Bytes per sector
    UCHAR       SectorsPerCluster;          // Number of sectors in a cluster
    USHORT      ReservedSectors;            // Reserved sectors, usually 1 (the bootsector)
    UCHAR       NumberOfFats;               // Number of FAT tables
    USHORT      RootDirEntries;             // Number of root directory entries (fat12/16)
    USHORT      TotalSectors;               // Number of total sectors on the drive, 16-bit
    UCHAR       MediaDescriptor;            // Media descriptor byte
    USHORT      SectorsPerFat;              // Sectors per FAT table (fat12/16)
    USHORT      SectorsPerTrack;            // Number of sectors in a track
    USHORT      NumberOfHeads;              // Number of heads on the disk
    ULONG       HiddenSectors;              // Hidden sectors (sectors before the partition start like the partition table)
    ULONG       TotalSectorsBig;            // This field is the new 32-bit total count of sectors on the volume
    ULONG       SectorsPerFatBig;           // This field is the FAT32 32-bit count of sectors occupied by ONE FAT. BPB_FATSz16 must be 0
    USHORT      ExtendedFlags;              // Extended flags (fat32)
    USHORT      FileSystemVersion;          // File system version (fat32)
    ULONG       RootDirStartCluster;        // Starting cluster of the root directory (fat32)
    USHORT      FsInfo;                     // Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
    USHORT      BackupBootSector;           // If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record. Usually 6.
    UCHAR       Reserved[12];               // Reserved for future expansion
    UCHAR       DriveNumber;                // Int 0x13 drive number (e.g. 0x80)
    UCHAR       Reserved1;                  // Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
    UCHAR       BootSignature;              // Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
    ULONG       VolumeSerialNumber;         // Volume serial number
    CHAR        VolumeLabel[11];            // Volume label. This field matches the 11-byte volume label recorded in the root directory
    CHAR        FileSystemType[8];          // Always set to the string "FAT32   "

    UCHAR       BootCodeAndData[420];       // The remainder of the boot sector

    USHORT      BootSectorMagic;            // 0xAA55

} FAT32_BOOTSECTOR, *PFAT32_BOOTSECTOR;

typedef struct _BTRFS_BOOTSECTOR
{
    UCHAR JumpBoot[3];
    UCHAR ChunkMapSize;
    UCHAR BootDrive;
    ULONGLONG PartitionStartLBA;
    UCHAR Fill[1521]; // 1536 - 15
    USHORT BootSectorMagic;
} BTRFS_BOOTSECTOR, *PBTRFS_BOOTSECTOR;
C_ASSERT(sizeof(BTRFS_BOOTSECTOR) == 3 * 512);

// TODO: Add more bootsector structures!

#include <poppack.h>

/* End of BIG FIXME!! */


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
    BootEntry->FriendlyName = L"\"ReactOS (VBoxDebug)\"";
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


#if DBG
    if (IsUnattendedSetup)
    {
        /* DefaultOS=ReactOS */
#ifndef _WINKD_
        BootOptions.CurrentBootEntryKey = MAKESTRKEY(L"ReactOS_KdSerial");
#else
        BootOptions.CurrentBootEntryKey = MAKESTRKEY(L"ReactOS_Debug");
#endif
    }
    else
#endif
    {
        /* DefaultOS=ReactOS */
        BootOptions.CurrentBootEntryKey = MAKESTRKEY(L"ReactOS");
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
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
    PUCHAR BootSector;

    /* Allocate buffer for bootsector */
    BootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (BootSector == NULL)
        return FALSE; // STATUS_INSUFFICIENT_RESOURCES;
    RtlZeroMemory(BootSector, SECTORSIZE);

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&RootPartition, RootPath);
    TrimTrailingPathSeparators_UStr(&RootPartition);

    InitializeObjectAttributes(&ObjectAttributes,
                               &RootPartition,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Read current boot sector into buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        BootSector,
                        SECTORSIZE,
                        &FileOffset,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Check for the existence of the bootsector signature */
    IsValid = (*(PUSHORT)(BootSector + 0x1FE) == 0xAA55);
    if (IsValid)
    {
        /* Check for the first instruction encoded on three bytes */
        IsValid = (((*(PULONG)BootSector) & 0x00FFFFFF) != 0x00000000);
    }

Quit:
    /* Free the boot sector */
    RtlFreeHeap(ProcessHeap, 0, BootSector);
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
    LARGE_INTEGER FileOffset;
    PUCHAR BootSector;

    /* Allocate buffer for bootsector */
    BootSector = RtlAllocateHeap(ProcessHeap, 0, Length);
    if (BootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, BootSector);
        return Status;
    }

    /* Read current boot sector into buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        BootSector,
                        Length,
                        &FileOffset,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, BootSector);
        return Status;
    }

    /* Write bootsector to DstPath */
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
        RtlFreeHeap(ProcessHeap, 0, BootSector);
        return Status;
    }

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootSector,
                         Length,
                         NULL,
                         NULL);
    NtClose(FileHandle);

    /* Free the boot sector */
    RtlFreeHeap(ProcessHeap, 0, BootSector);

    return Status;
}


static
NTSTATUS
InstallMbrBootCodeToDiskHelper(
    IN PCWSTR SrcPath,
    IN PCWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
    PPARTITION_SECTOR OrigBootSector;
    PPARTITION_SECTOR NewBootSector;

    /* Allocate buffer for original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, sizeof(PARTITION_SECTOR));
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Read current boot sector into buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        OrigBootSector,
                        sizeof(PARTITION_SECTOR),
                        &FileOffset,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Allocate buffer for new bootsector */
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, sizeof(PARTITION_SECTOR));
    if (NewBootSector == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Read new bootsector from SrcPath */
    RtlInitUnicodeString(&Name, SrcPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        NewBootSector,
                        sizeof(PARTITION_SECTOR),
                        NULL,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /*
     * Copy the disk signature, the reserved fields and
     * the partition table from the old MBR to the new one.
     */
    RtlCopyMemory(&NewBootSector->Signature,
                  &OrigBootSector->Signature,
                  sizeof(PARTITION_SECTOR) - offsetof(PARTITION_SECTOR, Signature)
                    /* Length of partition table */);

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Write new bootsector to RootPath */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         sizeof(PARTITION_SECTOR),
                         &FileOffset,
                         NULL);
    NtClose(FileHandle);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

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

    return InstallMbrBootCodeToDiskHelper(SourceMbrPathBuffer,
                                          DestinationDevicePathBuffer);
}


static
NTSTATUS
InstallFat12BootCodeToFloppy(
    IN PCWSTR SrcPath,
    IN PCWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
    PFAT_BOOTSECTOR OrigBootSector;
    PFAT_BOOTSECTOR NewBootSector;

    /* Allocate buffer for original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Read current boot sector into buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        OrigBootSector,
                        SECTORSIZE,
                        &FileOffset,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Allocate buffer for new bootsector */
    NewBootSector = RtlAllocateHeap(ProcessHeap,
                                    0,
                                    SECTORSIZE);
    if (NewBootSector == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Read new bootsector from SrcPath */
    RtlInitUnicodeString(&Name, SrcPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        NewBootSector,
                        SECTORSIZE,
                        NULL,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Adjust bootsector (copy a part of the FAT16 BPB) */
    memcpy(&NewBootSector->OemName,
           &OrigBootSector->OemName,
           FIELD_OFFSET(FAT_BOOTSECTOR, BootCodeAndData) -
           FIELD_OFFSET(FAT_BOOTSECTOR, OemName));

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Write new bootsector to RootPath */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         SECTORSIZE,
                         &FileOffset,
                         NULL);
    NtClose(FileHandle);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}

static
NTSTATUS
InstallFat16BootCode(
    IN PCWSTR SrcPath,          // FAT16 bootsector source file (on the installation medium)
    IN HANDLE DstPath,          // Where to save the bootsector built from the source + partition information
    IN HANDLE RootPartition)    // Partition holding the (old) FAT16 information
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
    PFAT_BOOTSECTOR OrigBootSector;
    PFAT_BOOTSECTOR NewBootSector;

    /* Allocate a buffer for the original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read the current partition boot sector into the buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(RootPartition,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        OrigBootSector,
                        SECTORSIZE,
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Allocate a buffer for the new bootsector */
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (NewBootSector == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Read the new bootsector from SrcPath */
    RtlInitUnicodeString(&Name, SrcPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        NewBootSector,
                        SECTORSIZE,
                        &FileOffset,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Adjust the bootsector (copy a part of the FAT16 BPB) */
    memcpy(&NewBootSector->OemName,
           &OrigBootSector->OemName,
           FIELD_OFFSET(FAT_BOOTSECTOR, BootCodeAndData) -
           FIELD_OFFSET(FAT_BOOTSECTOR, OemName));

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Write the new bootsector to DstPath */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         SECTORSIZE,
                         &FileOffset,
                         NULL);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}

static
NTSTATUS
InstallFat16BootCodeToFile(
    IN PCWSTR SrcPath,
    IN PCWSTR DstPath,
    IN PCWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle, FileHandle;

    /*
     * Open the root partition from which the boot sector
     * parameters will be obtained.
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
                          FILE_OVERWRITE_IF,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile() failed (Status %lx)\n", Status);
        NtClose(PartitionHandle);
        return Status;
    }

    /* Install the FAT16 boot sector */
    Status = InstallFat16BootCode(SrcPath, FileHandle, PartitionHandle);

    /* Close the file and the partition */
    NtClose(FileHandle);
    NtClose(PartitionHandle);

    return Status;
}

static
NTSTATUS
InstallFat16BootCodeToDisk(
    IN PCWSTR SrcPath,
    IN PCWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle;

    /*
     * Open the root partition from which the boot sector parameters will be
     * obtained; this is also where we will write the updated boot sector.
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
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Install the FAT16 boot sector */
    Status = InstallFat16BootCode(SrcPath, PartitionHandle, PartitionHandle);

    /* Close the partition */
    NtClose(PartitionHandle);

    return Status;
}


static
NTSTATUS
InstallFat32BootCode(
    IN PCWSTR SrcPath,          // FAT32 bootsector source file (on the installation medium)
    IN HANDLE DstPath,          // Where to save the bootsector built from the source + partition information
    IN HANDLE RootPartition)    // Partition holding the (old) FAT32 information
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
    PFAT32_BOOTSECTOR OrigBootSector;
    PFAT32_BOOTSECTOR NewBootSector;
    USHORT BackupBootSector;

    /* Allocate a buffer for the original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read the current boot sector into the buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(RootPartition,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        OrigBootSector,
                        SECTORSIZE,
                        &FileOffset,
                        NULL);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Allocate a buffer for the new bootsector (2 sectors) */
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, 2 * SECTORSIZE);
    if (NewBootSector == NULL)
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Read the new bootsector from SrcPath */
    RtlInitUnicodeString(&Name, SrcPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        NewBootSector,
                        2 * SECTORSIZE,
                        &FileOffset,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Adjust the bootsector (copy a part of the FAT32 BPB) */
    memcpy(&NewBootSector->OemName,
           &OrigBootSector->OemName,
           FIELD_OFFSET(FAT32_BOOTSECTOR, BootCodeAndData) -
           FIELD_OFFSET(FAT32_BOOTSECTOR, OemName));

    /*
     * We know we copy the boot code to a file only when DstPath != RootPartition,
     * otherwise the boot code is copied to the specified root partition.
     */
    if (DstPath != RootPartition)
    {
        /* Copy to a file: Disable the backup boot sector */
        NewBootSector->BackupBootSector = 0;
    }
    else
    {
        /* Copy to a disk: Get the location of the backup boot sector */
        BackupBootSector = OrigBootSector->BackupBootSector;
    }

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Write the first sector of the new bootcode to DstPath sector 0 */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(DstPath,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         SECTORSIZE,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    if (DstPath == RootPartition)
    {
        /* Copy to a disk: Write the backup boot sector */
        if ((BackupBootSector != 0x0000) && (BackupBootSector != 0xFFFF))
        {
            FileOffset.QuadPart = (ULONGLONG)((ULONG)BackupBootSector * SECTORSIZE);
            Status = NtWriteFile(DstPath,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 NewBootSector,
                                 SECTORSIZE,
                                 &FileOffset,
                                 NULL);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
                RtlFreeHeap(ProcessHeap, 0, NewBootSector);
                return Status;
            }
        }
    }

    /* Write the second sector of the new bootcode to boot disk sector 14 */
    // FileOffset.QuadPart = (ULONGLONG)(14 * SECTORSIZE);
    FileOffset.QuadPart = 14 * SECTORSIZE;
    Status = NtWriteFile(DstPath,   // or really RootPartition ???
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         ((PUCHAR)NewBootSector + SECTORSIZE),
                         SECTORSIZE,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
    }

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}

static
NTSTATUS
InstallFat32BootCodeToFile(
    IN PCWSTR SrcPath,
    IN PCWSTR DstPath,
    IN PCWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle, FileHandle;

    /*
     * Open the root partition from which the boot sector parameters
     * will be obtained.
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

    /* Open or create the file where (the first sector of ????) the new bootsector will be saved */
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
                          FILE_SUPERSEDE, // FILE_OVERWRITE_IF, <- is used for FAT16
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateFile() failed (Status %lx)\n", Status);
        NtClose(PartitionHandle);
        return Status;
    }

    /* Install the FAT32 boot sector */
    Status = InstallFat32BootCode(SrcPath, FileHandle, PartitionHandle);

    /* Close the file and the partition */
    NtClose(FileHandle);
    NtClose(PartitionHandle);

    return Status;
}

static
NTSTATUS
InstallFat32BootCodeToDisk(
    IN PCWSTR SrcPath,
    IN PCWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE PartitionHandle;

    /*
     * Open the root partition from which the boot sector parameters will be
     * obtained; this is also where we will write the updated boot sector.
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

    /* Install the FAT32 boot sector */
    Status = InstallFat32BootCode(SrcPath, PartitionHandle, PartitionHandle);

    /* Close the partition */
    NtClose(PartitionHandle);

    return Status;
}

static
NTSTATUS
InstallBtrfsBootCodeToDisk(
    IN PCWSTR SrcPath,
    IN PCWSTR RootPath)
{
    NTSTATUS Status;
    NTSTATUS LockStatus;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
//  PEXT2_BOOTSECTOR OrigBootSector;
    PBTRFS_BOOTSECTOR NewBootSector;
    // USHORT BackupBootSector;
    PARTITION_INFORMATION_EX PartInfo;

#if 0
    /* Allocate buffer for original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Read current boot sector into buffer */
    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        OrigBootSector,
                        SECTORSIZE,
                        &FileOffset,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }
#endif

    /* Allocate buffer for new bootsector */
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, sizeof(BTRFS_BOOTSECTOR));
    if (NewBootSector == NULL)
    {
        // RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Read new bootsector from SrcPath */
    RtlInitUnicodeString(&Name, SrcPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        // RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        NewBootSector,
                        sizeof(BTRFS_BOOTSECTOR),
                        NULL,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        // RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

#if 0
    /* Adjust bootsector (copy a part of the FAT32 BPB) */
    memcpy(&NewBootSector->OemName,
           &OrigBootSector->OemName,
           FIELD_OFFSET(FAT32_BOOTSECTOR, BootCodeAndData) -
           FIELD_OFFSET(FAT32_BOOTSECTOR, OemName));

    /* Get the location of the backup boot sector */
    BackupBootSector = OrigBootSector->BackupBootSector;

    /* Free the original boot sector */
    // RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
#endif

    /* Open the root partition - Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, RootPath);
    TrimTrailingPathSeparators_UStr(&Name);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /*
     * The BTRFS driver requires the volume to be locked in order to modify
     * the first sectors of the partition, even though they are outside the
     * file-system space / in the reserved area (they are situated before
     * the super-block at 0x1000) and is in principle allowed by the NT
     * storage stack.
     * So we lock here in order to write the bootsector at sector 0.
     * If locking fails, we ignore and continue nonetheless.
     */
    LockStatus = NtFsControlFile(FileHandle,
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
        DPRINT1("WARNING: Failed to lock BTRFS volume for writing bootsector! Operations may fail! (Status 0x%lx)\n", LockStatus);
    }

    /* Obtaining partition info and writing it to bootsector */
    Status = NtDeviceIoControlFile(FileHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_DISK_GET_PARTITION_INFO_EX,
                                   NULL,
                                   0,
                                   &PartInfo,
                                   sizeof(PartInfo));
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IOCTL_DISK_GET_PARTITION_INFO_EX failed (Status %lx)\n", Status);
        goto Quit;
    }

    /* Write new bootsector to RootPath */

    NewBootSector->PartitionStartLBA = PartInfo.StartingOffset.QuadPart / SECTORSIZE;

    /* Write sector 0 */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         sizeof(BTRFS_BOOTSECTOR),
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
        goto Quit;
    }

#if 0
    /* Write backup boot sector */
    if ((BackupBootSector != 0x0000) && (BackupBootSector != 0xFFFF))
    {
        FileOffset.QuadPart = (ULONGLONG)((ULONG)BackupBootSector * SECTORSIZE);
        Status = NtWriteFile(FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             NewBootSector,
                             SECTORSIZE,
                             &FileOffset,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
            goto Quit;
        }
    }

    /* Write sector 14 */
    FileOffset.QuadPart = 14 * SECTORSIZE;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         ((PUCHAR)NewBootSector + SECTORSIZE),
                         SECTORSIZE,
                         &FileOffset,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
    }
#endif

Quit:
    /* Unlock the volume */
    LockStatus = NtFsControlFile(FileHandle,
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
        DPRINT1("Failed to unlock BTRFS volume (Status 0x%lx)\n", LockStatus);
    }

    /* Close the volume */
    NtClose(FileHandle);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}


static
NTSTATUS
InstallFatBootcodeToPartition(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath,
    IN UCHAR PartitionType)
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

            if (PartitionType == PARTITION_FAT32 ||
                PartitionType == PARTITION_FAT32_XINT13)
            {
                /* Install FAT32 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat32.bin");

                DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, DstPath);
                Status = InstallFat32BootCodeToFile(SrcPath, DstPath,
                                                    SystemRootPath->Buffer);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallFat32BootCodeToFile() failed (Status %lx)\n", Status);
                    return Status;
                }
            }
            else
            {
                /* Install FAT16 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");

                DPRINT1("Install FAT bootcode: %S ==> %S\n", SrcPath, DstPath);
                Status = InstallFat16BootCodeToFile(SrcPath, DstPath,
                                                    SystemRootPath->Buffer);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallFat16BootCodeToFile() failed (Status %lx)\n", Status);
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
            if (PartitionType == PARTITION_FAT32 ||
                PartitionType == PARTITION_FAT32_XINT13)
            {
                /* Install FAT32 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat32.bin");

                DPRINT1("Install FAT32 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
                Status = InstallFat32BootCodeToDisk(SrcPath, SystemRootPath->Buffer);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallFat32BootCodeToDisk() failed (Status %lx)\n", Status);
                    return Status;
                }
            }
            else
            {
                /* Install FAT16 bootcode */
                CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");

                DPRINT1("Install FAT16 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
                Status = InstallFat16BootCodeToDisk(SrcPath, SystemRootPath->Buffer);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("InstallFat16BootCodeToDisk() failed (Status %lx)\n", Status);
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
    IN PUNICODE_STRING DestinationArcPath,
    IN UCHAR PartitionType)
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
            Status = SaveBootSector(SystemRootPath->Buffer, DstPath, sizeof(BTRFS_BOOTSECTOR));
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
        // if (PartitionType == PARTITION_EXT2)
        {
            /* Install BTRFS bootcode */
            CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\btrfs.bin");

            DPRINT1("Install BTRFS bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
            Status = InstallBtrfsBootCodeToDisk(SrcPath, SystemRootPath->Buffer);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("InstallBtrfsBootCodeToDisk() failed (Status %lx)\n", Status);
                return Status;
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
InstallVBRToPartition(
    IN PUNICODE_STRING SystemRootPath,
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath,
    IN UCHAR PartitionType)
{
    switch (PartitionType)
    {
        case PARTITION_FAT_12:
        case PARTITION_FAT_16:
        case PARTITION_HUGE:
        case PARTITION_XINT13:
        case PARTITION_FAT32:
        case PARTITION_FAT32_XINT13:
        {
            return InstallFatBootcodeToPartition(SystemRootPath,
                                                 SourceRootPath,
                                                 DestinationArcPath,
                                                 PartitionType);
        }

        case PARTITION_LINUX:
        {
            return InstallBtrfsBootcodeToPartition(SystemRootPath,
                                                   SourceRootPath,
                                                   DestinationArcPath,
                                                   PartitionType);
        }

        case PARTITION_IFS:
            DPRINT1("Partitions of type NTFS or HPFS are not supported yet!\n");
            break;

        default:
            DPRINT1("PartitionType 0x%02X unknown!\n", PartitionType);
            break;
    }

    return STATUS_UNSUCCESSFUL;
}


NTSTATUS
InstallFatBootcodeToFloppy(
    IN PUNICODE_STRING SourceRootPath,
    IN PUNICODE_STRING DestinationArcPath)
{
    NTSTATUS Status;
    PFILE_SYSTEM FatFS;
    UNICODE_STRING FloppyDevice = RTL_CONSTANT_STRING(L"\\Device\\Floppy0\\");
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Verify that the floppy disk is accessible */
    if (DoesDirExist(NULL, FloppyDevice.Buffer) == FALSE)
        return STATUS_DEVICE_NOT_READY;

    /* Format the floppy disk */
    FatFS = GetFileSystemByName(L"FAT");
    if (!FatFS)
    {
        DPRINT1("FAT FS non existent on this system?!\n");
        return STATUS_NOT_SUPPORTED;
    }
    Status = FatFS->FormatFunc(&FloppyDevice,
                               FMIFS_FLOPPY,
                               NULL,
                               TRUE,
                               0,
                               NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("VfatFormat() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Copy FreeLoader to the boot partition */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\freeldr.sys");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, FloppyDevice.Buffer, L"freeldr.sys");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Create new 'freeldr.ini' */
    DPRINT("Create new 'freeldr.ini'\n");
    Status = CreateFreeLoaderIniForReactOS(FloppyDevice.Buffer, DestinationArcPath->Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Install FAT12 boosector */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 1, FloppyDevice.Buffer);

    DPRINT("Install FAT bootcode: %S ==> %S\n", SrcPath, DstPath);
    Status = InstallFat12BootCodeToFloppy(SrcPath, DstPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallFat12BootCodeToFloppy() failed (Status %lx)\n", Status);
        return Status;
    }

    return STATUS_SUCCESS;
}

/* EOF */
