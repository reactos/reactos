/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/bootsup.c
 * PURPOSE:         Bootloader support functions
 * PROGRAMMER:      Eric Kohl
 */

#include "usetup.h"

#define NDEBUG
#include <debug.h>

#define SECTORSIZE 512

#include <pshpack1.h>
typedef struct _FAT_BOOTSECTOR
{
    UCHAR		JumpBoot[3];				// Jump instruction to boot code
    CHAR		OemName[8];					// "MSWIN4.1" for MS formatted volumes
    USHORT		BytesPerSector;				// Bytes per sector
    UCHAR		SectorsPerCluster;			// Number of sectors in a cluster
    USHORT		ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
    UCHAR		NumberOfFats;				// Number of FAT tables
    USHORT		RootDirEntries;				// Number of root directory entries (fat12/16)
    USHORT		TotalSectors;				// Number of total sectors on the drive, 16-bit
    UCHAR		MediaDescriptor;			// Media descriptor byte
    USHORT		SectorsPerFat;				// Sectors per FAT table (fat12/16)
    USHORT		SectorsPerTrack;			// Number of sectors in a track
    USHORT		NumberOfHeads;				// Number of heads on the disk
    ULONG		HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
    ULONG		TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
    UCHAR		DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
    UCHAR		Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
    UCHAR		BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
    ULONG		VolumeSerialNumber;			// Volume serial number
    CHAR		VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
    CHAR		FileSystemType[8];			// One of the strings "FAT12   ", "FAT16   ", or "FAT     "

    UCHAR		BootCodeAndData[448];		// The remainder of the boot sector

    USHORT		BootSectorMagic;			// 0xAA55

} FAT_BOOTSECTOR, *PFAT_BOOTSECTOR;

typedef struct _FAT32_BOOTSECTOR
{
    UCHAR		JumpBoot[3];				// Jump instruction to boot code
    CHAR		OemName[8];					// "MSWIN4.1" for MS formatted volumes
    USHORT		BytesPerSector;				// Bytes per sector
    UCHAR		SectorsPerCluster;			// Number of sectors in a cluster
    USHORT		ReservedSectors;			// Reserved sectors, usually 1 (the bootsector)
    UCHAR		NumberOfFats;				// Number of FAT tables
    USHORT		RootDirEntries;				// Number of root directory entries (fat12/16)
    USHORT		TotalSectors;				// Number of total sectors on the drive, 16-bit
    UCHAR		MediaDescriptor;			// Media descriptor byte
    USHORT		SectorsPerFat;				// Sectors per FAT table (fat12/16)
    USHORT		SectorsPerTrack;			// Number of sectors in a track
    USHORT		NumberOfHeads;				// Number of heads on the disk
    ULONG		HiddenSectors;				// Hidden sectors (sectors before the partition start like the partition table)
    ULONG		TotalSectorsBig;			// This field is the new 32-bit total count of sectors on the volume
    ULONG		SectorsPerFatBig;			// This field is the FAT32 32-bit count of sectors occupied by ONE FAT. BPB_FATSz16 must be 0
    USHORT		ExtendedFlags;				// Extended flags (fat32)
    USHORT		FileSystemVersion;			// File system version (fat32)
    ULONG		RootDirStartCluster;		// Starting cluster of the root directory (fat32)
    USHORT		FsInfo;						// Sector number of FSINFO structure in the reserved area of the FAT32 volume. Usually 1.
    USHORT		BackupBootSector;			// If non-zero, indicates the sector number in the reserved area of the volume of a copy of the boot record. Usually 6.
    UCHAR		Reserved[12];				// Reserved for future expansion
    UCHAR		DriveNumber;				// Int 0x13 drive number (e.g. 0x80)
    UCHAR		Reserved1;					// Reserved (used by Windows NT). Code that formats FAT volumes should always set this byte to 0.
    UCHAR		BootSignature;				// Extended boot signature (0x29). This is a signature byte that indicates that the following three fields in the boot sector are present.
    ULONG		VolumeSerialNumber;			// Volume serial number
    CHAR		VolumeLabel[11];			// Volume label. This field matches the 11-byte volume label recorded in the root directory
    CHAR		FileSystemType[8];			// Always set to the string "FAT32   "

    UCHAR		BootCodeAndData[420];		// The remainder of the boot sector

    USHORT		BootSectorMagic;			// 0xAA55

} FAT32_BOOTSECTOR, *PFAT32_BOOTSECTOR;

typedef struct _EXT2_BOOTSECTOR
{
    // The EXT2 bootsector is completely user-specific.
    // No FS data is stored there.
    UCHAR Fill[1024];
} EXT2_BOOTSECTOR, *PEXT2_BOOTSECTOR;

// TODO: Add more bootsector structures!

#include <poppack.h>

extern PPARTLIST PartitionList;

/* FUNCTIONS ****************************************************************/


static
VOID
CreateCommonFreeLoaderSections(
    PINICACHE IniCache)
{
    PINICACHESECTION IniSection;

    /* Create "FREELOADER" section */
    IniSection = IniCacheAppendSection(IniCache, L"FREELOADER");

#if DBG
    if (IsUnattendedSetup)
    {
        /* DefaultOS=ReactOS */
        IniCacheInsertKey(IniSection,
                          NULL,
                          INSERT_LAST,
                          L"DefaultOS",
#ifndef _WINKD_
                          L"ReactOS_KdSerial");
#else
                          L"ReactOS_Debug");
#endif
    }
    else
#endif
    {
        /* DefaultOS=ReactOS */
        IniCacheInsertKey(IniSection,
                          NULL,
                          INSERT_LAST,
                          L"DefaultOS",
                          L"ReactOS");
    }

#if DBG
    if (IsUnattendedSetup)
#endif
    {
        /* Timeout=0 for unattended or non debug*/
        IniCacheInsertKey(IniSection,
                          NULL,
                          INSERT_LAST,
                          L"TimeOut",
                          L"0");
    }
#if DBG
    else
    {
        /* Timeout=0 or 10 */
        IniCacheInsertKey(IniSection,
                          NULL,
                          INSERT_LAST,
                          L"TimeOut",
                          L"10");
    }
#endif

    /* Create "Display" section */
    IniSection = IniCacheAppendSection(IniCache, L"Display");

    /* TitleText=ReactOS Boot Manager */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"TitleText",
                      L"ReactOS Boot Manager");

    /* StatusBarColor=Cyan */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"StatusBarColor",
                      L"Cyan");

    /* StatusBarTextColor=Black */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"StatusBarTextColor",
                      L"Black");

    /* BackdropTextColor=White */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BackdropTextColor",
                      L"White");

    /* BackdropColor=Blue */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BackdropColor",
                      L"Blue");

    /* BackdropFillStyle=Medium */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BackdropFillStyle",
                      L"Medium");

    /* TitleBoxTextColor=White */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"TitleBoxTextColor",
                      L"White");

    /* TitleBoxColor=Red */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"TitleBoxColor",
                      L"Red");

    /* MessageBoxTextColor=White */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"MessageBoxTextColor",
                      L"White");

    /* MessageBoxColor=Blue */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"MessageBoxColor",
                      L"Blue");

    /* MenuTextColor=White */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"MenuTextColor",
                      L"Gray");

    /* MenuColor=Blue */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"MenuColor",
                      L"Black");

    /* TextColor=Yellow */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"TextColor",
                      L"Gray");

    /* SelectedTextColor=Black */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"SelectedTextColor",
                      L"Black");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"SelectedColor",
                      L"Gray");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"ShowTime",
                      L"No");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"MenuBox",
                      L"No");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"CenterMenu",
                      L"No");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"MinimalUI",
                      L"Yes");

    /* SelectedColor=Gray */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"TimeText",
                      L"Seconds until highlighted choice will be started automatically:   ");
}

static
NTSTATUS
CreateNTOSEntry(
    PINICACHE IniCache,
    PINICACHESECTION OSSection,
    PWCHAR Section,
    PWCHAR Description,
    PWCHAR BootType,
    PWCHAR ArcPath,
    PWCHAR Options)
{
    PINICACHESECTION IniSection;

    /* Insert entry into "Operating Systems" section */
    IniCacheInsertKey(OSSection,
                      NULL,
                      INSERT_LAST,
                      Section,
                      Description);

    /* Create new section */
    IniSection = IniCacheAppendSection(IniCache, Section);

    /* BootType= */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BootType",
                      BootType);

    /* SystemPath= */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"SystemPath",
                      ArcPath);

    /* Options= */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"Options",
                      Options);

    return STATUS_SUCCESS;
}

static
VOID
CreateFreeLoaderReactOSEntries(
    PINICACHE IniCache,
    PWCHAR ArcPath)
{
    PINICACHESECTION IniSection;

    /* Create "Operating Systems" section */
    IniSection = IniCacheAppendSection(IniCache, L"Operating Systems");

    /* ReactOS */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS", L"\"ReactOS\"",
                    L"Windows2003", ArcPath,
                    L"");

    /* ReactOS_Debug */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS_Debug", L"\"ReactOS (Debug)\"",
                    L"Windows2003", ArcPath,
                    L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS");
#ifdef _WINKD_
    /* ReactOS_VBoxDebug */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS_VBoxDebug", L"\"ReactOS (VBoxDebug)\"",
                    L"Windows2003", ArcPath,
                    L"/DEBUG /DEBUGPORT=VBOX /SOS");
#endif
#if DBG
#ifndef _WINKD_
    /* ReactOS_KdSerial */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS_KdSerial", L"\"ReactOS (RosDbg)\"",
                    L"Windows2003", ArcPath,
                    L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /KDSERIAL");
#endif

    /* ReactOS_Screen */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS_Screen", L"\"ReactOS (Screen)\"",
                    L"Windows2003", ArcPath,
                    L"/DEBUG /DEBUGPORT=SCREEN /SOS");

    /* ReactOS_LogFile */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS_LogFile", L"\"ReactOS (Log file)\"",
                    L"Windows2003", ArcPath,
                    L"/DEBUG /DEBUGPORT=FILE /SOS");

    /* ReactOS_Ram */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS_Ram", L"\"ReactOS (RAM Disk)\"",
                    L"Windows2003", L"ramdisk(0)\\ReactOS",
                    L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /RDPATH=reactos.img /RDIMAGEOFFSET=32256");

    /* ReactOS_EMS */
    CreateNTOSEntry(IniCache, IniSection,
                    L"ReactOS_EMS", L"\"ReactOS (Emergency Management Services)\"",
                    L"Windows2003", ArcPath,
                    L"/DEBUG /DEBUGPORT=COM1 /BAUDRATE=115200 /SOS /redirect=com2 /redirectbaudrate=115200");
#endif
}

static
NTSTATUS
CreateFreeLoaderIniForReactOS(
    PWCHAR IniPath,
    PWCHAR ArcPath)
{
    PINICACHE IniCache;

    /* Initialize the INI file */
    IniCache = IniCacheCreate();

    /* Create the common FreeLdr sections */
    CreateCommonFreeLoaderSections(IniCache);

    /* Add the ReactOS entries */
    CreateFreeLoaderReactOSEntries(IniCache, ArcPath);

    /* Save the INI file */
    IniCacheSave(IniCache, IniPath);
    IniCacheDestroy(IniCache);

    return STATUS_SUCCESS;
}

static
NTSTATUS
CreateFreeLoaderIniForReactOSAndBootSector(
    PWCHAR IniPath,
    PWCHAR ArcPath,
    PWCHAR Section,
    PWCHAR Description,
    PWCHAR BootDrive,
    PWCHAR BootPartition,
    PWCHAR BootSector)
{
    PINICACHE IniCache;
    PINICACHESECTION IniSection;

    /* Initialize the INI file */
    IniCache = IniCacheCreate();

    /* Create the common FreeLdr sections */
    CreateCommonFreeLoaderSections(IniCache);

    /* Add the ReactOS entries */
    CreateFreeLoaderReactOSEntries(IniCache, ArcPath);

    /* Get "Operating Systems" section */
    IniSection = IniCacheGetSection(IniCache, L"Operating Systems");

    /* Insert entry into "Operating Systems" section */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      Section,
                      Description);

    /* Create new section */
    IniSection = IniCacheAppendSection(IniCache, Section);

    /* BootType=BootSector */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BootType",
                      L"BootSector");

    /* BootDrive= */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BootDrive",
                      BootDrive);

    /* BootPartition= */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BootPartition",
                      BootPartition);

    /* BootSector= */
    IniCacheInsertKey(IniSection,
                      NULL,
                      INSERT_LAST,
                      L"BootSectorFile",
                      BootSector);

    /* Save the INI file */
    IniCacheSave(IniCache, IniPath);
    IniCacheDestroy(IniCache);

    return STATUS_SUCCESS;
}

static
NTSTATUS
UpdateFreeLoaderIni(
    PWCHAR IniPath,
    PWCHAR ArcPath)
{
    NTSTATUS Status;
    PINICACHE IniCache;
    PINICACHESECTION IniSection;
    PINICACHESECTION OsIniSection;
    WCHAR SectionName[80];
    WCHAR OsName[80];
    WCHAR SystemPath[200];
    WCHAR SectionName2[200];
    PWCHAR KeyData;
    ULONG i,j;

    Status = IniCacheLoad(&IniCache, IniPath, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Get "Operating Systems" section */
    IniSection = IniCacheGetSection(IniCache, L"Operating Systems");
    if (IniSection == NULL)
    {
        IniCacheDestroy(IniCache);
        return STATUS_UNSUCCESSFUL;
    }

    /* Find an existing usable or an unused section name */
    i = 1;
    wcscpy(SectionName, L"ReactOS");
    wcscpy(OsName, L"\"ReactOS\"");
    while(TRUE)
    {
        Status = IniCacheGetKey(IniSection, SectionName, &KeyData);
        if (!NT_SUCCESS(Status))
            break;

        /* Get operation system section */
        if (KeyData[0] == '"')
        {
            wcscpy(SectionName2, &KeyData[1]);
            j = wcslen(SectionName2);
            if (j > 0)
            {
                SectionName2[j-1] = 0;
            }
        }
        else
        {
            wcscpy(SectionName2, KeyData);
        }

        /* Search for an existing ReactOS entry */
        OsIniSection = IniCacheGetSection(IniCache, SectionName2);
        if (OsIniSection != NULL)
        {
            BOOLEAN UseExistingEntry = TRUE;

            /* Check for boot type "Windows2003" */
            Status = IniCacheGetKey(OsIniSection, L"BootType", &KeyData);
            if (NT_SUCCESS(Status))
            {
                if ((KeyData == NULL) ||
                    ( (_wcsicmp(KeyData, L"Windows2003") != 0) &&
                      (_wcsicmp(KeyData, L"\"Windows2003\"") != 0) ))
                {
                    /* This is not a ReactOS entry */
                    UseExistingEntry = FALSE;
                }
            }
            else
            {
                UseExistingEntry = FALSE;
            }

            if (UseExistingEntry)
            {
                /* BootType is Windows2003. Now check SystemPath. */
                Status = IniCacheGetKey(OsIniSection, L"SystemPath", &KeyData);
                if (NT_SUCCESS(Status))
                {
                    swprintf(SystemPath, L"\"%s\"", ArcPath);
                    if ((KeyData == NULL) ||
                        ( (_wcsicmp(KeyData, ArcPath) != 0) &&
                          (_wcsicmp(KeyData, SystemPath) != 0) ))
                    {
                        /* This entry is a ReactOS entry, but the SystemRoot
                           does not match the one we are looking for. */
                        UseExistingEntry = FALSE;
                    }
                }
                else
                {
                    UseExistingEntry = FALSE;
                }
            }

            if (UseExistingEntry)
            {
                IniCacheDestroy(IniCache);
                return STATUS_SUCCESS;
            }
        }

        swprintf(SectionName, L"ReactOS_%lu", i);
        swprintf(OsName, L"\"ReactOS %lu\"", i);
        i++;
    }

    /* Create a new "ReactOS" entry */
    CreateNTOSEntry(IniCache, IniSection,
                    SectionName, OsName,
                    L"Windows2003", ArcPath,
                    L"");

    IniCacheSave(IniCache, IniPath);
    IniCacheDestroy(IniCache);

    return STATUS_SUCCESS;
}

BOOLEAN
IsThereAValidBootSector(PWSTR RootPath)
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
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
    PUCHAR BootSector;
    ULONG Instruction;

    /* Allocate buffer for bootsector */
    BootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (BootSector == NULL)
        return FALSE; // STATUS_INSUFFICIENT_RESOURCES;

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
        goto Quit;

    RtlZeroMemory(BootSector, SECTORSIZE);

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

    /* Check the instruction; we use a ULONG to read three bytes */
    Instruction = (*(PULONG)BootSector) & 0x00FFFFFF;
    IsValid = (Instruction != 0x00000000);

    /* Check the bootsector signature */
    IsValid &= (*(PUSHORT)(BootSector + 0x1fe) == 0xaa55);

Quit:
    /* Free the boot sector */
    RtlFreeHeap(ProcessHeap, 0, BootSector);
    return IsValid; // Status;
}

NTSTATUS
SaveBootSector(
    PWSTR RootPath,
    PWSTR DstPath,
    ULONG Length)
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

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, BootSector);
        return Status;
    }

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
                               0,
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
InstallFat16BootCodeToFile(
    PWSTR SrcPath,
    PWSTR DstPath,
    PWSTR RootPath)
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

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

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
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
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
                        0,
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

    /* Adjust bootsector (copy a part of the FAT BPB) */
    memcpy(&NewBootSector->OemName,
           &OrigBootSector->OemName,
           FIELD_OFFSET(FAT_BOOTSECTOR, BootCodeAndData) -
           FIELD_OFFSET(FAT_BOOTSECTOR, OemName));

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Write new bootsector to DstPath */
    RtlInitUnicodeString(&Name, DstPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
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
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         SECTORSIZE,
                         NULL,
                         NULL);
    NtClose(FileHandle);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}

static
NTSTATUS
InstallFat32BootCodeToFile(
    PWSTR SrcPath,
    PWSTR DstPath,
    PWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
    PFAT32_BOOTSECTOR OrigBootSector;
    PFAT32_BOOTSECTOR NewBootSector;

    /* Allocate buffer for original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    FileOffset.QuadPart = 0ULL;
    Status = NtReadFile(FileHandle,
                        NULL,
                        NULL,
                        NULL,
                        &IoStatusBlock,
                        OrigBootSector,
                        SECTORSIZE,
                        NULL,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

    /* Allocate buffer for new bootsector (2 sectors) */
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, 2 * SECTORSIZE);
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
                        0,
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
                        2 * SECTORSIZE,
                        NULL,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Adjust bootsector (copy a part of the FAT32 BPB) */
    memcpy(&NewBootSector->OemName,
           &OrigBootSector->OemName,
           FIELD_OFFSET(FAT32_BOOTSECTOR, BootCodeAndData) -
           FIELD_OFFSET(FAT32_BOOTSECTOR, OemName));

    /* Disable the backup boot sector */
    NewBootSector->BackupBootSector = 0;

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Write the first sector of the new bootcode to DstPath */
    RtlInitUnicodeString(&Name, DstPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
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
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandle,
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
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Write the second sector of the new bootcode to boot disk sector 14 */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    FileOffset.QuadPart = (ULONGLONG)(14 * SECTORSIZE);
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
    }
    NtClose(FileHandle);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}


NTSTATUS
InstallMbrBootCodeToDisk(
    PWSTR SrcPath,
    PWSTR RootPath)
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
    OrigBootSector = (PPARTITION_SECTOR)RtlAllocateHeap(ProcessHeap,
                     0,
                     sizeof(PARTITION_SECTOR));
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name,
                         RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

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
    NewBootSector = (PPARTITION_SECTOR)RtlAllocateHeap(ProcessHeap,
                    0,
                    sizeof(PARTITION_SECTOR));
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
                        0,
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
                  sizeof(PARTITION_SECTOR) - offsetof(PARTITION_SECTOR, Signature) /* Length of partition table */);

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Write new bootsector to RootPath */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

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

static
NTSTATUS
InstallFat12BootCodeToFloppy(
    PWSTR SrcPath,
    PWSTR RootPath)
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

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

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
                        0,
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

    /* Write new bootsector to RootPath */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

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
InstallFat16BootCodeToDisk(
    PWSTR SrcPath,
    PWSTR RootPath)
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

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

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
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
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
                        0,
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

    /* Write new bootsector to RootPath */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

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
InstallFat32BootCodeToDisk(
    PWSTR SrcPath,
    PWSTR RootPath)
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

    /* Allocate buffer for original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

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


    /* Allocate buffer for new bootsector (2 sectors) */
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, 2 * SECTORSIZE);
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
                        0,
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
                        2 * SECTORSIZE,
                        NULL,
                        NULL);
    NtClose(FileHandle);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Adjust bootsector (copy a part of the FAT32 BPB) */
    memcpy(&NewBootSector->OemName,
           &OrigBootSector->OemName,
           FIELD_OFFSET(FAT32_BOOTSECTOR, BootCodeAndData) -
           FIELD_OFFSET(FAT32_BOOTSECTOR, OemName));

    /* Get the location of the backup boot sector */
    BackupBootSector = OrigBootSector->BackupBootSector;

    /* Free the original boot sector */
    RtlFreeHeap(ProcessHeap, 0, OrigBootSector);

    /* Write the first sector of the new bootcode to DstPath */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Write sector 0 */
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
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
        NtClose(FileHandle);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

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
            NtClose(FileHandle);
            RtlFreeHeap(ProcessHeap, 0, NewBootSector);
            return Status;
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
    NtClose(FileHandle);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}

static
NTSTATUS
InstallExt2BootCodeToDisk(
    PWSTR SrcPath,
    PWSTR RootPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    LARGE_INTEGER FileOffset;
//  PEXT2_BOOTSECTOR OrigBootSector;
    PEXT2_BOOTSECTOR NewBootSector;
    // USHORT BackupBootSector;

#if 0
    /* Allocate buffer for original bootsector */
    OrigBootSector = RtlAllocateHeap(ProcessHeap, 0, SECTORSIZE);
    if (OrigBootSector == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Read current boot sector into buffer */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
        return Status;
    }

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
    NewBootSector = RtlAllocateHeap(ProcessHeap, 0, sizeof(EXT2_BOOTSECTOR));
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
                        0,
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
                        sizeof(EXT2_BOOTSECTOR),
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

    NewBootSector->HiddenSectors = PartitionList->CurrentDisk->SectorsPerTrack;

    /* Get the location of the backup boot sector */
    BackupBootSector = OrigBootSector->BackupBootSector;

    /* Free the original boot sector */
    // RtlFreeHeap(ProcessHeap, 0, OrigBootSector);
#endif

    /* Write new bootsector to RootPath */
    RtlInitUnicodeString(&Name, RootPath);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               0,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

    /* Write sector 0 */
    FileOffset.QuadPart = 0ULL;
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         NewBootSector,
                         sizeof(EXT2_BOOTSECTOR),
                         &FileOffset,
                         NULL);
#if 0
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtWriteFile() failed (Status %lx)\n", Status);
        NtClose(FileHandle);
        RtlFreeHeap(ProcessHeap, 0, NewBootSector);
        return Status;
    }

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
            NtClose(FileHandle);
            RtlFreeHeap(ProcessHeap, 0, NewBootSector);
            return Status;
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
    NtClose(FileHandle);

    /* Free the new boot sector */
    RtlFreeHeap(ProcessHeap, 0, NewBootSector);

    return Status;
}

static
NTSTATUS
UnprotectBootIni(
    PWSTR FileName,
    PULONG Attributes)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;
    HANDLE FileHandle;

    RtlInitUnicodeString(&Name, FileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (Status == STATUS_NO_SUCH_FILE)
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        *Attributes = 0;
        return STATUS_SUCCESS;
    }
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        return Status;
    }

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile() failed (Status %lx)\n", Status);
        NtClose(FileHandle);
        return Status;
    }

    *Attributes = FileInfo.FileAttributes;

    /* Delete attributes SYSTEM, HIDDEN and READONLY */
    FileInfo.FileAttributes = FileInfo.FileAttributes &
                              ~(FILE_ATTRIBUTE_SYSTEM |
                                FILE_ATTRIBUTE_HIDDEN |
                                FILE_ATTRIBUTE_READONLY);

    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileInfo,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationFile() failed (Status %lx)\n", Status);
    }

    NtClose(FileHandle);
    return Status;
}

static
NTSTATUS
ProtectBootIni(
    PWSTR FileName,
    ULONG Attributes)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_BASIC_INFORMATION FileInfo;
    HANDLE FileHandle;

    RtlInitUnicodeString(&Name, FileName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenFile(&FileHandle,
                        GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        0,
                        FILE_SYNCHRONOUS_IO_NONALERT);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtOpenFile() failed (Status %lx)\n", Status);
        return Status;
    }

    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FILE_BASIC_INFORMATION),
                                    FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationFile() failed (Status %lx)\n", Status);
        NtClose(FileHandle);
        return Status;
    }

    FileInfo.FileAttributes = FileInfo.FileAttributes | Attributes;

    Status = NtSetInformationFile(FileHandle,
                                  &IoStatusBlock,
                                  &FileInfo,
                                  sizeof(FILE_BASIC_INFORMATION),
                                  FileBasicInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationFile() failed (Status %lx)\n", Status);
    }

    NtClose(FileHandle);
    return Status;
}

static
NTSTATUS
UpdateBootIni(
    PWSTR BootIniPath,
    PWSTR EntryName,
    PWSTR EntryValue)
{
    NTSTATUS Status;
    PINICACHE Cache = NULL;
    PINICACHESECTION Section = NULL;
    ULONG FileAttribute;
    PWCHAR OldValue = NULL;

    Status = IniCacheLoad(&Cache, BootIniPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Section = IniCacheGetSection(Cache,
                                 L"operating systems");
    if (Section == NULL)
    {
        IniCacheDestroy(Cache);
        return STATUS_UNSUCCESSFUL;
    }

    /* Check - maybe record already exists */
    Status = IniCacheGetKey(Section, EntryName, &OldValue);

    /* If either key was not found, or contains something else - add new one */
    if (!NT_SUCCESS(Status) || wcscmp(OldValue, EntryValue))
    {
        IniCacheInsertKey(Section,
                          NULL,
                          INSERT_LAST,
                          EntryName,
                          EntryValue);
    }

    Status = UnprotectBootIni(BootIniPath,
                              &FileAttribute);
    if (!NT_SUCCESS(Status))
    {
        IniCacheDestroy(Cache);
        return Status;
    }

    Status = IniCacheSave(Cache, BootIniPath);
    if (!NT_SUCCESS(Status))
    {
        IniCacheDestroy(Cache);
        return Status;
    }

    FileAttribute |= (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_READONLY);
    Status = ProtectBootIni(BootIniPath, FileAttribute);

    IniCacheDestroy(Cache);

    return Status;
}

static
NTSTATUS
InstallFatBootcodeToPartition(
    PUNICODE_STRING SystemRootPath,
    PUNICODE_STRING SourceRootPath,
    PUNICODE_STRING DestinationArcPath,
    UCHAR PartitionType)
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
    Status = SetupCopyFile(SrcPath, DstPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Prepare for possibly updating 'freeldr.ini' */
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"freeldr.ini");

    DoesFreeLdrExist = DoesFileExist(NULL, DstPath);
    if (DoesFreeLdrExist)
    {
        /* Update existing 'freeldr.ini' */
        DPRINT1("Update existing 'freeldr.ini'\n");

        Status = UpdateFreeLoaderIni(DstPath, DestinationArcPath->Buffer);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("UpdateFreeLoaderIni() failed (Status %lx)\n", Status);
            return Status;
        }
    }

    /* Check for NT and other bootloaders */

    // FIXME: Check for Vista+ bootloader!
    /*** Status = FindNTOSBootLoader(PartitionHandle, NtLdr, &Version); ***/
    /*** Status = FindNTOSBootLoader(PartitionHandle, BootMgr, &Version); ***/
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
            // CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"freeldr.ini");

            Status = CreateFreeLoaderIniForReactOS(DstPath, DestinationArcPath->Buffer);
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
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"boot.ini");

        DPRINT1("Update 'boot.ini': %S\n", DstPath);
        Status = UpdateBootIni(DstPath,
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

            Section       = L"DOS";
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

            Section       = L"DOS";
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

            Section       = L"DOS";
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

            Section       = L"DOS";
            Description   = L"\"DR-DOS 3.x\"";
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

            Section       = L"DOS";
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

            Section       = L"DOS";
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

            Section       = L"DOS";
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
            // CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"freeldr.ini");

            if (IsThereAValidBootSector(SystemRootPath->Buffer))
            {
                Status = CreateFreeLoaderIniForReactOSAndBootSector(
                             DstPath, DestinationArcPath->Buffer,
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
                Status = CreateFreeLoaderIniForReactOS(DstPath, DestinationArcPath->Buffer);
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
InstallExt2BootcodeToPartition(
    PUNICODE_STRING SystemRootPath,
    PUNICODE_STRING SourceRootPath,
    PUNICODE_STRING DestinationArcPath,
    UCHAR PartitionType)
{
    NTSTATUS Status;
    BOOLEAN DoesFreeLdrExist;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* EXT2 partition */
    DPRINT("System path: '%wZ'\n", SystemRootPath);

    /* Copy FreeLoader to the system partition, always overwriting the older version */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\freeldr.sys");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"freeldr.sys");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Prepare for possibly copying 'freeldr.ini' */
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"freeldr.ini");

    DoesFreeLdrExist = DoesFileExist(NULL, DstPath);
    if (DoesFreeLdrExist)
    {
        /* Update existing 'freeldr.ini' */
        DPRINT1("Update existing 'freeldr.ini'\n");

        Status = UpdateFreeLoaderIni(DstPath, DestinationArcPath->Buffer);
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
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"\\freeldr.ini");

        /* Certainly SysLinux, GRUB, LILO... or an unknown boot loader */
        DPRINT1("*nix or unknown boot loader found\n");

        if (IsThereAValidBootSector(SystemRootPath->Buffer))
        {
            PCWSTR BootSector = L"BOOTSECT.OLD";

            Status = CreateFreeLoaderIniForReactOSAndBootSector(
                         DstPath, DestinationArcPath->Buffer,
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
            Status = SaveBootSector(SystemRootPath->Buffer, DstPath, sizeof(EXT2_BOOTSECTOR));
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("SaveBootSector() failed (Status %lx)\n", Status);
                return Status;
            }
        }
        else
        {
            Status = CreateFreeLoaderIniForReactOS(DstPath, DestinationArcPath->Buffer);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
                return Status;
            }
        }

        /* Install new bootsector on the disk */
        // if (PartitionType == PARTITION_EXT2)
        {
            /* Install EXT2 bootcode */
            CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\ext2.bin");

            DPRINT1("Install EXT2 bootcode: %S ==> %S\n", SrcPath, SystemRootPath->Buffer);
            Status = InstallExt2BootCodeToDisk(SrcPath, SystemRootPath->Buffer);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("InstallExt2BootCodeToDisk() failed (Status %lx)\n", Status);
                return Status;
            }
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS
InstallVBRToPartition(
    PUNICODE_STRING SystemRootPath,
    PUNICODE_STRING SourceRootPath,
    PUNICODE_STRING DestinationArcPath,
    UCHAR PartitionType)
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

        case PARTITION_EXT2:
        {
            return InstallExt2BootcodeToPartition(SystemRootPath,
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
    PUNICODE_STRING SourceRootPath,
    PUNICODE_STRING DestinationArcPath)
{
    NTSTATUS Status;
    PFILE_SYSTEM FatFS;
    UNICODE_STRING FloppyDevice = RTL_CONSTANT_STRING(L"\\Device\\Floppy0\\");
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Format the floppy first */
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
    Status = SetupCopyFile(SrcPath, DstPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SetupCopyFile() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Create new 'freeldr.ini' */
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, FloppyDevice.Buffer, L"freeldr.ini");

    DPRINT("Create new 'freeldr.ini'\n");
    Status = CreateFreeLoaderIniForReactOS(DstPath, DestinationArcPath->Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CreateFreeLoaderIniForReactOS() failed (Status %lx)\n", Status);
        return Status;
    }

    /* Install FAT12 boosector */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath->Buffer, L"\\loader\\fat.bin");
    StringCchCopyW(DstPath, ARRAYSIZE(DstPath), FloppyDevice.Buffer);

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
