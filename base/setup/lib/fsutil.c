/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Filesystem Format and ChkDsk support functions.
 * COPYRIGHT:   Copyright 2003-2019 Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca@sfr.fr>
 */

//
// See also: https://git.reactos.org/?p=reactos.git;a=blob;f=reactos/dll/win32/fmifs/init.c;h=e895f5ef9cae4806123f6bbdd3dfed37ec1c8d33;hb=b9db9a4e377a2055f635b2fb69fef4e1750d219c
// for how to get FS providers in a dynamic way. In the (near) future we may
// consider merging some of this code with us into a fmifs / fsutil / fslib library...
//

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "partlist.h"
#include "fsrec.h"
#include "fsutil.h"

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
    PULIB_FORMAT FormatFunc;
    PULIB_CHKDSK ChkdskFunc;
} FILE_SYSTEM, *PFILE_SYSTEM;

/* The list of file systems on which we can install ReactOS */
static FILE_SYSTEM RegisteredFileSystems[] =
{
    /* NOTE: The FAT formatter will automatically
     * determine whether to use FAT12/16 or FAT32 */
    { L"FAT"  , VfatFormat, VfatChkdsk },
    { L"FAT32", VfatFormat, VfatChkdsk },
#if 0
    { L"FATX" , VfatxFormat, VfatxChkdsk },
    { L"NTFS" , NtfsFormat, NtfsChkdsk },
#endif
    { L"BTRFS", BtrfsFormat, BtrfsChkdsk },
#if 0
    { L"EXT2" , Ext2Format, Ext2Chkdsk },
    { L"EXT3" , Ext2Format, Ext2Chkdsk },
    { L"EXT4" , Ext2Format, Ext2Chkdsk },
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
            (wcsicmp(FileSystemName, Item->FileSystemName) == 0))
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
            (wcsicmp(FileSystemName, FileSystems->FileSystemName) == 0))
        {
            return FileSystems;
        }

        ++FileSystems;
    }

#endif

    return NULL;
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
    NTSTATUS Status;
    BOOLEAN Success;

    FileSystem = GetFileSystemByName(FileSystemName);

    if (!FileSystem || !FileSystem->ChkdskFunc)
    {
        // Success = FALSE;
        // Callback(DONE, 0, &Success);
        return STATUS_NOT_SUPPORTED;
    }

    Status = STATUS_SUCCESS;
    Success = FileSystem->ChkdskFunc(DriveRoot,
                                     Callback,
                                     FixErrors,
                                     Verbose,
                                     CheckOnlyIfDirty,
                                     ScanDrive,
                                     NULL,
                                     NULL,
                                     NULL,
                                     NULL,
                                     (PULONG)&Status);
    if (!Success)
        DPRINT1("ChkdskFunc() failed with Status 0x%lx\n", Status);

    // Callback(DONE, 0, &Success);

    return Status;
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
    BOOLEAN Success;
    BOOLEAN BackwardCompatible = FALSE; // Default to latest FS versions.
    MEDIA_TYPE MediaType;

    FileSystem = GetFileSystemByName(FileSystemName);

    if (!FileSystem || !FileSystem->FormatFunc)
    {
        // Success = FALSE;
        // Callback(DONE, 0, &Success);
        return STATUS_NOT_SUPPORTED;
    }

    /* Set the BackwardCompatible flag in case we format with older FAT12/16 */
    if (wcsicmp(FileSystemName, L"FAT") == 0)
        BackwardCompatible = TRUE;
    // else if (wcsicmp(FileSystemName, L"FAT32") == 0)
        // BackwardCompatible = FALSE;

    /* Convert the FMIFS MediaFlag to a NT MediaType */
    // FIXME: Actually covert all the possible flags.
    switch (MediaFlag)
    {
    case FMIFS_FLOPPY:
        MediaType = F5_320_1024; // FIXME: This is hardfixed!
        break;
    case FMIFS_REMOVABLE:
        MediaType = RemovableMedia;
        break;
    case FMIFS_HARDDISK:
        MediaType = FixedMedia;
        break;
    default:
        DPRINT1("Unknown FMIFS MediaFlag %d, converting 1-to-1 to NT MediaType\n",
                MediaFlag);
        MediaType = (MEDIA_TYPE)MediaFlag;
        break;
    }

    Success = FileSystem->FormatFunc(DriveRoot,
                                     Callback,
                                     QuickFormat,
                                     BackwardCompatible,
                                     MediaType,
                                     Label,
                                     ClusterSize);
    if (!Success)
        DPRINT1("FormatFunc() failed\n");

    // Callback(DONE, 0, &Success);

    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
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


NTSTATUS
ChkdskPartition(
    IN PPARTENTRY PartEntry,
    IN BOOLEAN FixErrors,
    IN BOOLEAN Verbose,
    IN BOOLEAN CheckOnlyIfDirty,
    IN BOOLEAN ScanDrive,
    IN PFMIFSCALLBACK Callback)
{
    NTSTATUS Status;
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    // UNICODE_STRING PartitionRootPath;
    WCHAR PartitionRootPath[MAX_PATH]; // PathBuffer

    ASSERT(PartEntry->IsPartitioned && PartEntry->PartitionNumber != 0);

    /* HACK: Do not try to check a partition with an unknown filesystem */
    if (!*PartEntry->FileSystem)
    {
        PartEntry->NeedsCheck = FALSE;
        return STATUS_SUCCESS;
    }

    /* Set PartitionRootPath */
    RtlStringCchPrintfW(PartitionRootPath, ARRAYSIZE(PartitionRootPath),
                        L"\\Device\\Harddisk%lu\\Partition%lu",
                        DiskEntry->DiskNumber,
                        PartEntry->PartitionNumber);
    DPRINT("PartitionRootPath: %S\n", PartitionRootPath);

    /* Check the partition */
    Status = ChkdskFileSystem(PartitionRootPath,
                              PartEntry->FileSystem,
                              FixErrors,
                              Verbose,
                              CheckOnlyIfDirty,
                              ScanDrive,
                              Callback);
    if (!NT_SUCCESS(Status))
        return Status;

    PartEntry->NeedsCheck = FALSE;
    return STATUS_SUCCESS;
}

NTSTATUS
FormatPartition(
    IN PPARTENTRY PartEntry,
    IN PCWSTR FileSystemName,
    IN FMIFS_MEDIA_FLAG MediaFlag,
    IN PCWSTR Label,
    IN BOOLEAN QuickFormat,
    IN ULONG ClusterSize,
    IN PFMIFSCALLBACK Callback)
{
    NTSTATUS Status;
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    UCHAR PartitionType;
    // UNICODE_STRING PartitionRootPath;
    WCHAR PartitionRootPath[MAX_PATH]; // PathBuffer

    ASSERT(PartEntry->IsPartitioned && PartEntry->PartitionNumber != 0);

    if (!FileSystemName || !*FileSystemName)
    {
        DPRINT1("No file system specified?\n");
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    /*
     * Prepare the partition for formatting (for MBR disks, reset the
     * partition type), and adjust the filesystem name in case of FAT
     * vs. FAT32, depending on the geometry of the partition.
     */

// FIXME: Do this only if QuickFormat == FALSE? What about FAT handling?

    /*
     * Retrieve a partition type as a hint only. It will be used to determine
     * whether to actually use FAT12/16 or FAT32 filesystem, depending on the
     * geometry of the partition. If the partition resides on an MBR disk,
     * the partition style will be reset to this value as well, unless the
     * partition is OEM.
     */
    PartitionType = FileSystemToMBRPartitionType(FileSystemName,
                                                 PartEntry->StartSector.QuadPart,
                                                 PartEntry->SectorCount.QuadPart);
    if (PartitionType == PARTITION_ENTRY_UNUSED)
    {
        /* Unknown file system */
        DPRINT1("Unknown file system '%S'\n", FileSystemName);
        return STATUS_UNRECOGNIZED_VOLUME;
    }

    /* Reset the MBR partition type, unless this is an OEM partition */
    if (DiskEntry->DiskStyle == PARTITION_STYLE_MBR)
    {
        if (!IsOEMPartition(PartEntry->PartitionType))
            SetMBRPartitionType(PartEntry, PartitionType);
    }

    /*
     * Adjust the filesystem name in case of FAT vs. FAT32, according to
     * the type of partition returned by FileSystemToMBRPartitionType().
     */
    if (wcsicmp(FileSystemName, L"FAT") == 0)
    {
        if ((PartitionType == PARTITION_FAT32) ||
            (PartitionType == PARTITION_FAT32_XINT13))
        {
            FileSystemName = L"FAT32";
        }
    }

    /* Commit the partition changes to the disk */
    Status = WritePartitions(DiskEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("WritePartitions(disk %lu) failed, Status 0x%08lx\n",
                DiskEntry->DiskNumber, Status);
        return STATUS_PARTITION_FAILURE;
    }

    /* Set PartitionRootPath */
    RtlStringCchPrintfW(PartitionRootPath, ARRAYSIZE(PartitionRootPath),
                        L"\\Device\\Harddisk%lu\\Partition%lu",
                        DiskEntry->DiskNumber,
                        PartEntry->PartitionNumber);
    DPRINT("PartitionRootPath: %S\n", PartitionRootPath);

    /* Format the partition */
    Status = FormatFileSystem(PartitionRootPath,
                              FileSystemName,
                              MediaFlag,
                              Label,
                              QuickFormat,
                              ClusterSize,
                              Callback);
    if (!NT_SUCCESS(Status))
        return Status;

//
// TODO: Here, call a partlist.c function that update the actual
// FS name and the label fields of the volume.
//
    PartEntry->FormatState = Formatted;

    /* Set the new partition's file system proper */
    RtlStringCbCopyW(PartEntry->FileSystem,
                     sizeof(PartEntry->FileSystem),
                     FileSystemName);

    PartEntry->New = FALSE;

    return STATUS_SUCCESS;
}

/* EOF */
