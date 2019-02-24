/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem support functions
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2019 Hermes Belusca-Maito
 */

//
// See also: https://git.reactos.org/?p=reactos.git;a=blob;f=reactos/dll/win32/fmifs/init.c;h=e895f5ef9cae4806123f6bbdd3dfed37ec1c8d33;hb=b9db9a4e377a2055f635b2fb69fef4e1750d219c
// for how to get FS providers in a dynamic way. In the (near) future we may
// consider merging some of this code with us into a fmifs / fsutil / fslib library...
//

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "fsutil.h"
#include "partlist.h"

#include <fslib/vfatlib.h>
#include <fslib/btrfslib.h>
// #include <fslib/ext2lib.h>
// #include <fslib/ntfslib.h>

#define NDEBUG
#include <debug.h>


/* LOCALS *******************************************************************/

/** IFS_PROVIDER **/
typedef struct _FILE_SYSTEM
{
    PCWSTR FileSystemName;
    FORMATEX FormatFunc;
    CHKDSKEX ChkdskFunc;
} FILE_SYSTEM, *PFILE_SYSTEM;

/* The list of file systems on which we can install ReactOS */
static FILE_SYSTEM RegisteredFileSystems[] =
{
    /* NOTE: The FAT formatter automatically determines
     * whether it will use FAT-16 or FAT-32. */
    { L"FAT"  , VfatFormat, VfatChkdsk },
#if 0
    { L"FAT32", VfatFormat, VfatChkdsk }, // Do we support specific FAT sub-formats specifications?
    { L"FATX" , VfatxFormat, VfatxChkdsk },
    { L"NTFS" , NtfsFormat, NtfsChkdsk },
#endif
    { L"BTRFS", BtrfsFormatEx, BtrfsChkdskEx },
#if 0
    { L"EXT2" , Ext2Format, Ext2Chkdsk },
    { L"EXT3" , Ext2Format, Ext2Chkdsk },
    { L"EXT4" , Ext2Format, Ext2Chkdsk },
    { L"FFS"  , FfsFormat , FfsChkdsk  },
    { L"REISERFS", ReiserfsFormat, ReiserfsChkdsk },
#endif
};


/* FUNCTIONS ****************************************************************/

/** QueryAvailableFileSystemFormat() **/
BOOLEAN
GetRegisteredFileSystems(
    IN ULONG Index,
    OUT PCWSTR* FileSystemName)
{
    if (Index >= ARRAYSIZE(RegisteredFileSystems))
        return FALSE;

    *FileSystemName = RegisteredFileSystems[Index].FileSystemName;

    return TRUE;
}


/** GetProvider() **/
static PFILE_SYSTEM
GetFileSystemByName(
    IN PCWSTR FileSystemName)
{
#if 0 // Reenable when the list of registered FSes will again be dynamic

    PLIST_ENTRY ListEntry;
    PFILE_SYSTEM_ITEM Item;

    ListEntry = List->ListHead.Flink;
    while (ListEntry != &List->ListHead)
    {
        Item = CONTAINING_RECORD(ListEntry, FILE_SYSTEM_ITEM, ListEntry);
        if (Item->FileSystemName &&
            (wcsicmp(FileSystemName, Item->FileSystemName) == 0 ||
            /* Map FAT32 back to FAT */
            (wcsicmp(FileSystemName, L"FAT32") == 0 && wcsicmp(Item->FileSystemName, L"FAT") == 0)))
        {
            return Item;
        }

        ListEntry = ListEntry->Flink;
    }

#else

    ULONG Count = ARRAYSIZE(RegisteredFileSystems);
    PFILE_SYSTEM FileSystems = RegisteredFileSystems;

    ASSERT(FileSystems && Count != 0);

    while (Count--)
    {
        if (FileSystems->FileSystemName &&
            (wcsicmp(FileSystemName, FileSystems->FileSystemName) == 0 ||
            /* Map FAT32 back to FAT */
            (wcsicmp(FileSystemName, L"FAT32") == 0 && wcsicmp(FileSystems->FileSystemName, L"FAT") == 0)))
        {
            return FileSystems;
        }

        ++FileSystems;
    }

#endif

    return NULL;
}


//
// FileSystem recognition, using NT OS functionality
//

/* NOTE: Ripped & adapted from base/system/autochk/autochk.c */
NTSTATUS
GetFileSystemNameByHandle(
    IN HANDLE PartitionHandle,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    UCHAR Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    /* Retrieve the FS attributes */
    Status = NtQueryVolumeInformationFile(PartitionHandle,
                                          &IoStatusBlock,
                                          FileFsAttribute,
                                          sizeof(Buffer),
                                          FileFsAttributeInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryVolumeInformationFile failed, Status 0x%08lx\n", Status);
        return Status;
    }

    if (FileSystemNameSize < FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    return RtlStringCbCopyNW(FileSystemName, FileSystemNameSize,
                             FileFsAttribute->FileSystemName,
                             FileFsAttribute->FileSystemNameLength);
}

NTSTATUS
GetFileSystemName_UStr(
    IN PUNICODE_STRING PartitionPath,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE PartitionHandle;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Open the partition */
    InitializeObjectAttributes(&ObjectAttributes,
                               PartitionPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionHandle,
                        FILE_GENERIC_READ /* | SYNCHRONIZE */,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0 /* FILE_SYNCHRONOUS_IO_NONALERT */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n", PartitionPath, Status);
        return Status;
    }

    /* Retrieve the FS attributes */
    Status = GetFileSystemNameByHandle(PartitionHandle, FileSystemName, FileSystemNameSize);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetFileSystemNameByHandle() failed for partition '%wZ', Status 0x%08lx\n",
                PartitionPath, Status);
    }

    /* Close the partition */
    NtClose(PartitionHandle);

    return Status;
}

NTSTATUS
GetFileSystemName(
    IN PCWSTR Partition,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    UNICODE_STRING PartitionPath;

    RtlInitUnicodeString(&PartitionPath, Partition);
    return GetFileSystemName_UStr(&PartitionPath,
                                  FileSystemName,
                                  FileSystemNameSize);
}

NTSTATUS
InferFileSystemByHandle(
    IN HANDLE PartitionHandle,
    IN UCHAR PartitionType,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;

    if (FileSystemNameSize < sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    *FileSystemName = L'\0';

    /* Try to infer a file system using NT file system recognition */
    Status = GetFileSystemNameByHandle(PartitionHandle,
                                       FileSystemName,
                                       FileSystemNameSize);
    if (NT_SUCCESS(Status) && *FileSystemName)
    {
        goto Quit;
    }

    /*
     * Try to infer a preferred file system for this partition, given its ID.
     *
     * WARNING: This is partly a hack, since partitions with the same ID can
     * be formatted with different file systems: for example, usual Linux
     * partitions that are formatted in EXT2/3/4, ReiserFS, etc... have the
     * same partition ID 0x83.
     *
     * The proper fix is to make a function that detects the existing FS
     * from a given partition (not based on the partition ID).
     * On the contrary, for unformatted partitions with a given ID, the
     * following code is OK.
     */
    if ((PartitionType == PARTITION_FAT_12) ||
        (PartitionType == PARTITION_FAT_16) ||
        (PartitionType == PARTITION_HUGE  ) ||
        (PartitionType == PARTITION_XINT13))
    {
        /* FAT12 or FAT16 */
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"FAT");
    }
    else if ((PartitionType == PARTITION_FAT32) ||
             (PartitionType == PARTITION_FAT32_XINT13))
    {
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"FAT32");
    }
    else if (PartitionType == PARTITION_LINUX)
    {
        // WARNING: See the warning above.
        /* Could also be EXT2/3/4, ReiserFS, ... */
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"BTRFS");
    }
    else if (PartitionType == PARTITION_IFS)
    {
        // WARNING: See the warning above.
        /* Could also be HPFS */
        Status = RtlStringCbCopyW(FileSystemName, FileSystemNameSize, L"NTFS");
    }

Quit:
    if (*FileSystemName)
    {
        // WARNING: We cannot write on this FS yet!
        if (PartitionType == PARTITION_IFS)
        {
            DPRINT1("Recognized file system '%S' that doesn't have write support yet!\n",
                    FileSystemName);
        }
    }

    DPRINT1("InferFileSystem -- PartitionType: 0x%02X ; FileSystem (guessed): %S\n",
            PartitionType, *FileSystemName ? FileSystemName : L"None");

    return Status;
}

NTSTATUS
InferFileSystem(
    IN PCWSTR Partition,
    IN UCHAR PartitionType,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    UNICODE_STRING PartitionPath;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE PartitionHandle;
    IO_STATUS_BLOCK IoStatusBlock;

    /* Open the partition */
    RtlInitUnicodeString(&PartitionPath, Partition);
    InitializeObjectAttributes(&ObjectAttributes,
                               &PartitionPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&PartitionHandle,
                        FILE_GENERIC_READ /* | SYNCHRONIZE */,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        0 /* FILE_SYNCHRONOUS_IO_NONALERT */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition '%wZ', Status 0x%08lx\n", &PartitionPath, Status);
        return Status;
    }

    /* Retrieve the FS */
    Status = InferFileSystemByHandle(PartitionHandle,
                                     PartitionType,
                                     FileSystemName,
                                     FileSystemNameSize);

    /* Close the partition */
    NtClose(PartitionHandle);

    return Status;
}

/** ChkdskEx() **/
NTSTATUS
ChkdskFileSystem_UStr(
    IN PUNICODE_STRING DriveRoot,
    IN PCWSTR FileSystemName,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PFMIFSCALLBACK Callback)
{
    PFILE_SYSTEM FileSystem;

    FileSystem = GetFileSystemByName(FileSystemName);

    if (!FileSystem || !FileSystem->ChkdskFunc)
    {
        // BOOLEAN Argument = FALSE;
        // Callback(DONE, 0, &Argument);
        return STATUS_NOT_SUPPORTED;
    }

    return FileSystem->ChkdskFunc(DriveRoot,
                                  FixErrors,
                                  Verbose,
                                  CheckOnlyIfDirty,
                                  ScanDrive,
                                  Callback);
}

NTSTATUS
ChkdskFileSystem(
    IN PCWSTR DriveRoot,
    IN PCWSTR FileSystemName,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PFMIFSCALLBACK Callback)
{
    UNICODE_STRING DriveRootU;

    RtlInitUnicodeString(&DriveRootU, DriveRoot);
    return ChkdskFileSystem_UStr(&DriveRootU,
                                 FileSystemName,
                                 FixErrors,
                                 Verbose,
                                 CheckOnlyIfDirty,
                                 ScanDrive,
                                 Callback);
}


/** FormatEx() **/
NTSTATUS
FormatFileSystem_UStr(
    IN PUNICODE_STRING DriveRoot,
    IN PCWSTR FileSystemName,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PUNICODE_STRING Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback)
{
    PFILE_SYSTEM FileSystem;

    FileSystem = GetFileSystemByName(FileSystemName);

    if (!FileSystem || !FileSystem->FormatFunc)
    {
        // BOOLEAN Argument = FALSE;
        // Callback(DONE, 0, &Argument);
        return STATUS_NOT_SUPPORTED;
    }

    return FileSystem->FormatFunc(DriveRoot,
                                  MediaFlag,
                                  Label,
                                  QuickFormat,
                                  ClusterSize,
                                  Callback);
}

NTSTATUS
FormatFileSystem(
    IN PCWSTR DriveRoot,
    IN PCWSTR FileSystemName,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PCWSTR Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback)
{
    UNICODE_STRING DriveRootU;
    UNICODE_STRING LabelU;

    RtlInitUnicodeString(&DriveRootU, DriveRoot);
    RtlInitUnicodeString(&LabelU, Label);

    return FormatFileSystem_UStr(&DriveRootU,
                                 FileSystemName,
                                 MediaFlag,
                                 &LabelU,
                                 QuickFormat,
                                 ClusterSize,
                                 Callback);
}


UCHAR
FileSystemToPartitionType(
    IN PCWSTR FileSystem,
    IN PULARGE_INTEGER StartSector,
    IN PULARGE_INTEGER SectorCount)
{
    ASSERT(FileSystem && StartSector && SectorCount);

    if (wcsicmp(FileSystem, L"FAT")   == 0 ||
        wcsicmp(FileSystem, L"FAT32") == 0 ||
        wcsicmp(FileSystem, L"RAW")   == 0)
    {
        if (SectorCount->QuadPart < 8192)
        {
            /* FAT12 CHS partition (disk is smaller than 4.1MB) */
            return PARTITION_FAT_12;
        }
        else if (StartSector->QuadPart < 1450560)
        {
            /* Partition starts below the 8.4GB boundary ==> CHS partition */

            if (SectorCount->QuadPart < 65536)
            {
                /* FAT16 CHS partition (partition size < 32MB) */
                return PARTITION_FAT_16;
            }
            else if (SectorCount->QuadPart < 1048576)
            {
                /* FAT16 CHS partition (partition size < 512MB) */
                return PARTITION_HUGE;
            }
            else
            {
                /* FAT32 CHS partition (partition size >= 512MB) */
                return PARTITION_FAT32;
            }
        }
        else
        {
            /* Partition starts above the 8.4GB boundary ==> LBA partition */

            if (SectorCount->QuadPart < 1048576)
            {
                /* FAT16 LBA partition (partition size < 512MB) */
                return PARTITION_XINT13;
            }
            else
            {
                /* FAT32 LBA partition (partition size >= 512MB) */
                return PARTITION_FAT32_XINT13;
            }
        }
    }
    else if (wcsicmp(FileSystem, L"NTFS") == 0)
    {
        return PARTITION_IFS;
    }
    else if (wcsicmp(FileSystem, L"BTRFS") == 0 ||
             wcsicmp(FileSystem, L"EXT2")  == 0 ||
             wcsicmp(FileSystem, L"EXT3")  == 0 ||
             wcsicmp(FileSystem, L"EXT4")  == 0 ||
             wcsicmp(FileSystem, L"FFS")   == 0 ||
             wcsicmp(FileSystem, L"REISERFS") == 0)
    {
        return PARTITION_LINUX;
    }
    else
    {
        /* Unknown file system */
        DPRINT1("Unknown file system '%S'\n", FileSystem);
        return PARTITION_ENTRY_UNUSED;
    }
}


//
// Formatting routines
//

BOOLEAN
PreparePartitionForFormatting(
    IN struct _PARTENTRY* PartEntry,
    IN PCWSTR FileSystemName)
{
    UCHAR PartitionType;

    if (!FileSystemName || !*FileSystemName)
    {
        DPRINT1("No file system specified?\n");
        return FALSE;
    }

    PartitionType = FileSystemToPartitionType(FileSystemName,
                                              &PartEntry->StartSector,
                                              &PartEntry->SectorCount);
    if (PartitionType == PARTITION_ENTRY_UNUSED)
    {
        /* Unknown file system */
        DPRINT1("Unknown file system '%S'\n", FileSystemName);
        return FALSE;
    }

    SetPartitionType(PartEntry, PartitionType);

//
// FIXME: Do this now, or after the partition was actually formatted??
//
    /* Set the new partition's file system proper */
    RtlStringCbCopyW(PartEntry->FileSystem,
                     sizeof(PartEntry->FileSystem),
                     FileSystemName);

    return TRUE;
}

/* EOF */
