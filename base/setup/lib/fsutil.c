//
// This is basically the code for listing available FileSystem providers
// (currently hardcoded in a list), and for performing a basic FileSystem
// recognition for a given disk partition.
//
// See also: https://git.reactos.org/?p=reactos.git;a=blob;f=reactos/dll/win32/fmifs/init.c;h=e895f5ef9cae4806123f6bbdd3dfed37ec1c8d33;hb=b9db9a4e377a2055f635b2fb69fef4e1750d219c
// for how to get FS providers in a dynamic way. In the (near) future we may
// consider merging some of this code with us into a fmifs / fsutil / fslib library...
//

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "fsutil.h"
#include "partlist.h"

/** For FileSystems **/
#include <fslib/vfatlib.h>
#include <fslib/ext2lib.h>
// #include <fslib/ntfslib.h>

#define NDEBUG
#include <debug.h>



FILE_SYSTEM RegisteredFileSystems[] =
{
    { L"FAT"  , VfatFormat, VfatChkdsk },
//  { L"FAT32", VfatFormat, VfatChkdsk },
#if 0
    { L"EXT2" , Ext2Format, Ext2Chkdsk },
    { L"NTFS" , NtfsFormat, NtfsChkdsk }
#endif
};


/* FUNCTIONS ****************************************************************/

PFILE_SYSTEM
GetRegisteredFileSystems(OUT PULONG Count)
{
    *Count = ARRAYSIZE(RegisteredFileSystems);
    return RegisteredFileSystems;
}

PFILE_SYSTEM
GetFileSystemByName(
    // IN PFILE_SYSTEM_LIST List,
    IN PCWSTR FileSystemName)
{
#if 0 // Reenable when the list of registered FSes will again be dynamic

    PLIST_ENTRY ListEntry;
    PFILE_SYSTEM_ITEM Item;

    ListEntry = List->ListHead.Flink;
    while (ListEntry != &List->ListHead)
    {
        Item = CONTAINING_RECORD(ListEntry, FILE_SYSTEM_ITEM, ListEntry);
        if (Item->FileSystemName && wcsicmp(FileSystemName, Item->FileSystemName) == 0)
            return Item;

        ListEntry = ListEntry->Flink;
    }

#else

    ULONG Count;
    PFILE_SYSTEM FileSystems;

    FileSystems = GetRegisteredFileSystems(&Count);
    if (!FileSystems || Count == 0)
        return NULL;

    while (Count--)
    {
        if (FileSystems->FileSystemName && wcsicmp(FileSystemName, FileSystems->FileSystemName) == 0)
            return FileSystems;

        ++FileSystems;
    }

#endif

    return NULL;
}


//
// FileSystem recognition (using NT OS functionality)
//

#if 0 // FIXME: To be fully enabled when our storage stack & al. will work better!

/* NOTE: Ripped & adapted from base/system/autochk/autochk.c */
static NTSTATUS
_MyGetFileSystem(
    IN struct _PARTENTRY* PartEntry,
    IN OUT PWSTR FileSystemName,
    IN SIZE_T FileSystemNameSize)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_FS_ATTRIBUTE_INFORMATION FileFsAttribute;
    UCHAR Buffer[sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH * sizeof(WCHAR)];

    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING PartitionRootPath;
    WCHAR PathBuffer[MAX_PATH];

    FileFsAttribute = (PFILE_FS_ATTRIBUTE_INFORMATION)Buffer;

    /* Set PartitionRootPath */
    swprintf(PathBuffer,
             // L"\\Device\\Harddisk%lu\\Partition%lu", // Should work! But because ReactOS sucks atm. it actually doesn't work!!
             L"\\Device\\Harddisk%lu\\Partition%lu\\",  // HACK: Use this as a temporary hack!
             PartEntry->DiskEntry->DiskNumber,
             PartEntry->PartitionNumber);
    RtlInitUnicodeString(&PartitionRootPath, PathBuffer);
    DPRINT("PartitionRootPath: %wZ\n", &PartitionRootPath);

    /* Open the partition */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PartitionRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle, // PartitionHandle,
                        FILE_GENERIC_READ /* | SYNCHRONIZE */,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        0 /* FILE_SYNCHRONOUS_IO_NONALERT */);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to open partition %wZ, Status 0x%08lx\n", &PartitionRootPath, Status);
        return Status;
    }

    /* Retrieve the FS attributes */
    Status = NtQueryVolumeInformationFile(FileHandle,
                                          &IoStatusBlock,
                                          FileFsAttribute,
                                          sizeof(Buffer),
                                          FileFsAttributeInformation);
    NtClose(FileHandle);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryVolumeInformationFile failed for partition %wZ, Status 0x%08lx\n", &PartitionRootPath, Status);
        return Status;
    }

    if (FileSystemNameSize * sizeof(WCHAR) < FileFsAttribute->FileSystemNameLength + sizeof(WCHAR))
        return STATUS_BUFFER_TOO_SMALL;

    RtlCopyMemory(FileSystemName,
                  FileFsAttribute->FileSystemName,
                  FileFsAttribute->FileSystemNameLength);
    FileSystemName[FileFsAttribute->FileSystemNameLength / sizeof(WCHAR)] = UNICODE_NULL;

    return STATUS_SUCCESS;
}

#endif

PFILE_SYSTEM
GetFileSystem(
    // IN PFILE_SYSTEM_LIST FileSystemList,
    IN struct _PARTENTRY* PartEntry)
{
    PFILE_SYSTEM CurrentFileSystem;
    PWSTR FileSystemName = NULL;
#if 0 // For code temporarily disabled below
    NTSTATUS Status;
    WCHAR FsRecFileSystemName[MAX_PATH];
#endif

    CurrentFileSystem = PartEntry->FileSystem;

    /* We have a file system, return it */
    if (CurrentFileSystem != NULL && CurrentFileSystem->FileSystemName != NULL)
        return CurrentFileSystem;

    DPRINT1("File system not found, try to guess one...\n");

    CurrentFileSystem = NULL;

#if 0 // FIXME: To be fully enabled when our storage stack & al. will work better!

    /*
     * We don't have one...
     *
     * Try to infer one using NT file system recognition.
     */
    Status = _MyGetFileSystem(PartEntry, FsRecFileSystemName, ARRAYSIZE(FsRecFileSystemName));
    if (NT_SUCCESS(Status) && *FsRecFileSystemName)
    {
        /* Temporary HACK: map FAT32 back to FAT */
        if (wcscmp(FsRecFileSystemName, L"FAT32") == 0)
            wcscpy(FsRecFileSystemName, L"FAT");

        FileSystemName = FsRecFileSystemName;
        goto Quit;
    }

#endif

    /*
     * We don't have one...
     *
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
    if ((PartEntry->PartitionType == PARTITION_FAT_12) ||
        (PartEntry->PartitionType == PARTITION_FAT_16) ||
        (PartEntry->PartitionType == PARTITION_HUGE  ) ||
        (PartEntry->PartitionType == PARTITION_XINT13) ||
        (PartEntry->PartitionType == PARTITION_FAT32 ) ||
        (PartEntry->PartitionType == PARTITION_FAT32_XINT13))
    {
        FileSystemName = L"FAT";
    }
    else if (PartEntry->PartitionType == PARTITION_EXT2)
    {
        // WARNING: See the warning above.
        FileSystemName = L"EXT2";
    }
    else if (PartEntry->PartitionType == PARTITION_IFS)
    {
        // WARNING: See the warning above.
        FileSystemName = L"NTFS"; /* FIXME: Not quite correct! */
    }

#if 0
Quit: // For code temporarily disabled above
#endif

    // HACK: WARNING: We cannot write on this FS yet!
    if (FileSystemName)
    {
        if (PartEntry->PartitionType == PARTITION_EXT2 || PartEntry->PartitionType == PARTITION_IFS)
            DPRINT1("Recognized file system %S that doesn't support write support yet!\n", FileSystemName);
    }

    DPRINT1("GetFileSystem -- PartitionType: 0x%02X ; FileSystemName (guessed): %S\n",
            PartEntry->PartitionType, FileSystemName ? FileSystemName : L"None");

    if (FileSystemName != NULL)
        CurrentFileSystem = GetFileSystemByName(/*FileSystemList,*/ FileSystemName);

    return CurrentFileSystem;
}


//
// Unused code (for now??)
//

#if 0 // Unused anymore. This portion of code is actually called in format.c "FormatPartition" function...
BOOLEAN
NATIVE_FormatPartition(
    IN PFILE_SYSTEM FileSystem, // IN PFILE_SYSTEM_ITEM FileSystem,
    IN PCUNICODE_STRING DriveRoot,
    IN PFMIFSCALLBACK Callback)
{
    NTSTATUS Status;

    Status = FileSystem->FormatFunc((PUNICODE_STRING)DriveRoot,
                                    FMIFS_HARDDISK,          /* MediaFlag */
                                    NULL,                    /* Label */
                                    FileSystem->QuickFormat, /* QuickFormat */
                                    0,                       /* ClusterSize */
                                    Callback);               /* Callback */
    return NT_SUCCESS(Status);
}
#endif
