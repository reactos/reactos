/*
 * COPYRIGHT:       See COPYING.ARM in the top level directory
 * PROJECT:         ReactOS UEFI Boot Library
 * FILE:            boot/environ/lib/io/file.c
 * PURPOSE:         Boot Library File Management Routines
 * PROGRAMMER:      Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "bl.h"

/* DATA VARIABLES ************************************************************/

PVOID* FileTable;
ULONG FileEntries;

LIST_ENTRY RegisteredFileSystems;
BL_FILE_SYSTEM_REGISTRATION_TABLE FatRegisterFunctionTable =
{
    FatInitialize,
    NULL,
    FatMount,
    NULL
};
BL_FILE_SYSTEM_REGISTRATION_TABLE EtfsRegisterFunctionTable =
{
    EtfsInitialize,
    NULL,
    EtfsMount,
    NULL
};

extern ULONG DmTableEntries;
extern PVOID* DmDeviceTable;

/* FUNCTIONS *****************************************************************/

PWCHAR
FileIoCopyParentDirectoryPath (
    _In_ PWCHAR FilePath
    )
{
    SIZE_T PathSize, PathSizeWithNull;
    PWCHAR Backslash, ParentCopy;

    PathSize = wcslen(FilePath) * sizeof(WCHAR);

    PathSizeWithNull = PathSize + sizeof(UNICODE_NULL);
    if (PathSizeWithNull < PathSize)
    {
        return NULL;
    }

    ParentCopy = BlMmAllocateHeap(PathSizeWithNull);
    if (!ParentCopy)
    {
        return NULL;
    }
    wcsncpy(ParentCopy, FilePath, PathSizeWithNull / sizeof(WCHAR));

    Backslash = wcsrchr(ParentCopy, '\\');
    if (!Backslash)
    {
        BlMmFreeHeap(ParentCopy);
        return NULL;
    }

    if (Backslash == ParentCopy)
    {
        ++Backslash;
    }

    *Backslash = UNICODE_NULL;
    return ParentCopy;
}

PWCHAR
FileIoCopyFileName (
    _In_ PWCHAR FilePath
    )
{
    PWCHAR Separator, FileCopy;
    SIZE_T PathSize;

    Separator = wcsrchr(FilePath, '\\');
    if (!Separator)
    {
        return NULL;
    }

    PathSize = wcslen(Separator) * sizeof(WCHAR);

    FileCopy = BlMmAllocateHeap(PathSize);
    if (!FileCopy)
    {
        return NULL;
    }

    wcsncpy(FileCopy, Separator + 1, PathSize / sizeof(WCHAR));
    return FileCopy;
}

BOOLEAN
FileTableCompareWithSubsetAttributes (
    _In_ PVOID Entry,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _In_ PVOID Argument3,
    _In_ PVOID Argument4
    )
{
    PBL_FILE_ENTRY FileEntry = (PBL_FILE_ENTRY)Entry;
    ULONG DeviceId = *(PULONG)Argument1;
    PWCHAR FilePath = (PWCHAR)Argument2;
    ULONG Flags = *(PULONG)Argument3;
    ULONG Unknown = *(PULONG)Argument4;
    BOOLEAN Found;

    Found = FALSE;

    if ((FileEntry->DeviceId == DeviceId) &&
        !(_wcsicmp(FileEntry->FilePath, FilePath)) &&
        (FileEntry->Unknown == Unknown))
    {
        if ((!(Flags & 1) || (FileEntry->Flags & 2)) && (!(Flags & 2) || (FileEntry->Flags & 4)))
        {
            if ((!(Flags & 4) || (FileEntry->Flags & 0x10000)) && ((Flags & 4) || !(FileEntry->Flags & 0x10000)))
            {
                Found = TRUE;
            }
        }
    }
    return Found;
}

BOOLEAN
FileTableCompareWithSameAttributes (
    _In_ PVOID Entry,
    _In_ PVOID Argument1,
    _In_ PVOID Argument2,
    _In_ PVOID Argument3,
    _In_ PVOID Argument4
    )
{
    PBL_FILE_ENTRY FileEntry = (PBL_FILE_ENTRY)Entry;
    ULONG DeviceId = *(PULONG)Argument1;
    PWCHAR FilePath = (PWCHAR)Argument2;
    ULONG Flags = *(PULONG)Argument3;
    ULONG Unknown = *(PULONG)Argument4;
    BOOLEAN Found;

    Found = FALSE;

    if ((FileEntry->DeviceId == DeviceId) &&
        !(_wcsicmp(FileEntry->FilePath, FilePath)) &&
        (FileEntry->Unknown == Unknown))
    {
        if ((!(Flags & 1) || (FileEntry->Flags & 2)) && ((Flags & 1) || !(FileEntry->Flags & 2)) && (!(Flags & 2) || (FileEntry->Flags & 4)) && ((Flags & 2) || !(FileEntry->Flags & 4)))
        {
            if ((!(Flags & 4) || (FileEntry->Flags & 0x10000)) && ((Flags & 4) || !(FileEntry->Flags & 0x10000)))
            {
                Found = TRUE;
            }
        }
    }
    return Found;
}

NTSTATUS
FileTableDestroyEntry (
    _In_ PBL_FILE_ENTRY FileEntry,
    _In_ ULONG Index
    )
{
    ULONG DeviceId;
    PBL_DEVICE_ENTRY DeviceEntry;
    NTSTATUS Status;

    DeviceId = FileEntry->DeviceId;
    if (DmTableEntries > DeviceId)
    {
        DeviceEntry = DmDeviceTable[DeviceId];
        if (DeviceEntry)
        {
            --DeviceEntry->ReferenceCount;
        }
    }

    Status = FileEntry->Callbacks.Close(FileEntry);

    BlMmFreeHeap(FileEntry);

    FileTable[Index] = NULL;
    return Status;
}

#define BL_FILE_PURGE_LIMIT 512

NTSTATUS
FileTablePurgeEntry (
    _In_ PVOID Entry
    )
{
    PBL_FILE_ENTRY FileEntry = (PBL_FILE_ENTRY)Entry;

    /* Don't purge opened files, or if there's less than 512 files cached */
    if (((FileEntry->Flags & BL_FILE_ENTRY_OPENED) ||
        (FileEntry->Flags & 0x10000)) &&
        (FileEntries < BL_FILE_PURGE_LIMIT))
    {
        return STATUS_UNSUCCESSFUL;
    }

    /* Purge the entry otherwise */
    return FileTableDestroyEntry(FileEntry, FileEntry->FileId);
}

NTSTATUS
BlFileClose (
    _In_ ULONG FileId
    )
{
    PBL_FILE_ENTRY FileEntry;

    /* Validate the file ID */
    if (FileEntries <= FileId)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure a file entry actually exists */
    FileEntry = FileTable[FileId];
    if (!FileEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* And that it's actually open */
    if (!(FileEntry->Flags & BL_FILE_ENTRY_OPENED))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Drop a reference, check if this was the last one */
    --FileEntry->ReferenceCount;
    if (!FileEntry->ReferenceCount)
    {
        /* File is no longer open */
        FileEntry->Flags &= ~BL_FILE_ENTRY_OPENED;
    }

    /* All good */
    return STATUS_SUCCESS;
}

NTSTATUS
FileIoOpen (
    _In_ ULONG DeviceId,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _In_ ULONG Unknown,
    _In_ PBL_TBL_LOOKUP_ROUTINE CompareRoutine,
    _Out_opt_ PBL_FILE_ENTRY *NewFileEntry
    )
{
    PWCHAR FileNameCopy, ParentFileName;
    NTSTATUS Status;
    PBL_DEVICE_ENTRY DeviceEntry;
    PBL_FILE_SYSTEM_ENTRY FileSystem;
    ULONG FileId, CheckFlags;
    PBL_FILE_ENTRY DirectoryEntry, FileEntry;
    PLIST_ENTRY NextEntry, ListHead;

    /* Preinitialize variables for failure */
    DirectoryEntry = NULL;
    FileNameCopy = NULL;
    ParentFileName = NULL;
    Status = STATUS_SUCCESS;

    /* Bail out if the device ID is invalid */
    if (DmTableEntries <= DeviceId)
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Bail out if there's no device entry */
    DeviceEntry = DmDeviceTable[DeviceId];
    if (!DeviceEntry)
    {
        return STATUS_ACCESS_DENIED;
    }

    /* Read access is always required for touching the device */
    CheckFlags = Flags | BL_FILE_READ_ACCESS;

    /* Check if the device is granting us read access */
    if ((CheckFlags & BL_FILE_READ_ACCESS) &&
        (!(DeviceEntry->Flags & BL_DEVICE_ENTRY_OPENED) ||
         !(DeviceEntry->Flags & BL_DEVICE_ENTRY_READ_ACCESS)))
    {
        EfiPrintf(L"Access denied\r\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check if the device is granting us write access */
    if ((CheckFlags & BL_FILE_WRITE_ACCESS) &&
        (!(DeviceEntry->Flags & BL_DEVICE_ENTRY_OPENED) ||
         !(DeviceEntry->Flags & BL_DEVICE_ENTRY_WRITE_ACCESS)))
    {
        EfiPrintf(L"Access denied2\r\n");
        return STATUS_ACCESS_DENIED;
    }

    /* Check if we already have this file open */
    FileEntry = (PBL_FILE_ENTRY )BlTblFindEntry(FileTable,
                                                FileEntries,
                                                &FileId,
                                                CompareRoutine,
                                                &DeviceId,
                                                FileName,
                                                &Flags,
                                                &Unknown);
    if (FileEntry)
    {
        goto FileOpened;
    }

    /* Check if we are opening the root drive or an actual file/directory */
    if ((*FileName != OBJ_NAME_PATH_SEPARATOR) || (FileName[1]))
    {
        /* Get the name of the directory */
        ParentFileName = FileIoCopyParentDirectoryPath(FileName);
        if (!ParentFileName)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Open it */
        Status = FileIoOpen(DeviceId,
                            ParentFileName,
                            BL_FILE_READ_ACCESS | BL_DIRECTORY_ACCESS,
                            Unknown,
                            FileTableCompareWithSubsetAttributes,
                            &DirectoryEntry);
        if (!NT_SUCCESS(Status))
        {
            goto Quickie;
        }

        /* Now get the the file name itself */
        FileNameCopy = FileIoCopyFileName(FileName);
        if (!FileNameCopy)
        {
            Status = STATUS_NO_MEMORY;
            goto Quickie;
        }

        /* Open it */
        Status = DirectoryEntry->Callbacks.Open(DirectoryEntry,
                                                FileNameCopy,
                                                Flags,
                                                &FileEntry);
    }
    else
    {
        /* We're opening the root, scan through all the file systems */
        Status = STATUS_UNSUCCESSFUL;
        ListHead = &RegisteredFileSystems;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Try to mount this one */
            FileSystem = CONTAINING_RECORD(NextEntry, BL_FILE_SYSTEM_ENTRY, ListEntry);
            Status = FileSystem->MountCallback(DeviceId, Unknown, &FileEntry);
            if (NT_SUCCESS(Status))
            {
                /* Mount successful */
                break;
            }

            /* Try the next file system */
            NextEntry = NextEntry->Flink;
        }

        /* Nothing to free on this path */
        FileNameCopy = NULL;
    }

    /* Handle failure */
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Could not open file!: %lx\r\n", Status);
        goto Quickie;
    }

    /* Save the unknown */
    FileEntry->Unknown = Unknown;

    /* Convert open flags into entry flags */
    if (Flags & BL_FILE_READ_ACCESS)
    {
        FileEntry->Flags |= BL_FILE_ENTRY_READ_ACCESS;
    }
    if (Flags & BL_FILE_WRITE_ACCESS)
    {
        FileEntry->Flags |= BL_FILE_ENTRY_WRITE_ACCESS;
    }

    /* Save the file into the file table */
    Status = BlTblSetEntry(&FileTable,
                           &FileEntries,
                           (PVOID)FileEntry,
                           &FileId,
                           FileTablePurgeEntry);
    if (!NT_SUCCESS(Status))
    {
        /* Close it if that failed */
        FileEntry->Callbacks.Close(FileEntry);
        goto Quickie;
    }

    /* Add a reference on the device, and save our file ID  */
    ++DeviceEntry->ReferenceCount;
    Status = STATUS_SUCCESS;
    FileEntry->FileId = FileId;

FileOpened:
    /* Add a reference to the file entry, and see if this is the first one */
    if (++FileEntry->ReferenceCount == 1)
    {
        /* Reset unknowns */
        FileEntry->TotalBytesRead = 0;
        FileEntry->Unknown2 = 0;
    }

    /* Set the file as opened */
    FileEntry->Flags |= BL_FILE_ENTRY_OPENED;

    /* Not sure what this flag does */
    if (Flags & BL_UNKNOWN_ACCESS)
    {
        FileEntry->Flags |= BL_FILE_ENTRY_UNKNOWN_ACCESS;
    }

    /* If the caller wanted the entry back, return it */
    if (NewFileEntry)
    {
        *NewFileEntry = FileEntry;
    }

Quickie:
    /* Close the parent */
    if (DirectoryEntry)
    {
        BlFileClose(DirectoryEntry->FileId);
    }

    /* Free the parent name copy */
    if (ParentFileName)
    {
        BlMmFreeHeap(ParentFileName);
    }

    /* Free the file name copy */
    if (FileNameCopy)
    {
        BlMmFreeHeap(FileNameCopy);
    }

    /* Return back to caller */
    return Status;
}

NTSTATUS
BlFileOpen (
    _In_ ULONG DeviceId,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _Out_ PULONG FileId
    )
{
    NTSTATUS Status;
    PBL_FILE_ENTRY FileEntry;
    BL_DEVICE_INFORMATION DeviceInformation;

    /* Make sure we have a valid file name, access flags and parameters */
    if (!(FileName) ||
        (*FileName != OBJ_NAME_PATH_SEPARATOR) ||
        !(FileId) ||
        !(Flags & (BL_FILE_READ_ACCESS | BL_FILE_WRITE_ACCESS)))
    {
        EfiPrintf(L"Invalid file options\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Get information on the underlying device */
    Status = BlDeviceGetInformation(DeviceId, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Get device info failed: %lx\r\n", Status);
        return Status;
    }

    /* Make sure it's a device that can host files */
    if ((DeviceInformation.DeviceType != DiskDevice) &&
        (DeviceInformation.DeviceType != LegacyPartitionDevice) &&
        (DeviceInformation.DeviceType != UdpDevice))
    {
        EfiPrintf(L"Invalid device type\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Open a file on this device, creating one if needed */
    Status = FileIoOpen(DeviceId,
                        FileName,
                        Flags,
                        0,
                        FileTableCompareWithSameAttributes,
                        &FileEntry);
    if (NT_SUCCESS(Status))
    {
        /* Return the file ID back to the caller */
        *FileId = FileEntry->FileId;
    }

    /* All good */
    return Status;
}

NTSTATUS
BlFileSetInformation (
    _In_ ULONG FileId,
    _Out_ PBL_FILE_INFORMATION FileInfo
    )
{
    PBL_FILE_ENTRY FileEntry;

    /* Make sure caller passed this in */
    if (!FileInfo)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate file ID */
    if (FileId > FileEntries)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure an opened file exits with this ID */
    FileEntry = FileTable[FileId];
    if (!(FileEntry) || !(FileEntry->Flags & BL_FILE_ENTRY_OPENED))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Do the I/O operation */
    return FileEntry->Callbacks.SetInfo(FileEntry, FileInfo);
}

NTSTATUS
BlFileGetInformation (
    _In_ ULONG FileId,
    _In_ PBL_FILE_INFORMATION FileInfo
    )
{
    PBL_FILE_ENTRY FileEntry;

    /* Make sure caller passed this in */
    if (!FileInfo)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Validate file ID */
    if (FileId > FileEntries)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Make sure an opened file exits with this ID */
    FileEntry = FileTable[FileId];
    if (!(FileEntry) || !(FileEntry->Flags & BL_FILE_ENTRY_OPENED))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Do the I/O operation */
    return FileEntry->Callbacks.GetInfo(FileEntry, FileInfo);
}

NTSTATUS
FileInformationCheck (
    _In_ PBL_FILE_INFORMATION FileInformation,
    _In_ BOOLEAN Write,
    _In_opt_ PULONG InputSize,
    _In_opt_ PULONG BytesReturned,
    _Out_opt_ PULONG RequiredSize
    )
{
    NTSTATUS Status;
    ULONG Size;

    /* Initialize variables */
    Status = STATUS_SUCCESS;
    Size = 0;

    /* Make sure we didn't overshoot */
    if (FileInformation->Offset > FileInformation->Size)
    {
        /* Bail out */
        Status = STATUS_INVALID_PARAMETER;
        goto Quickie;
    }

    /* Compute the appropriate 32-bit size of this read, based on file size */
    Size = ULONG_MAX;
    if ((FileInformation->Size - FileInformation->Offset) <= ULONG_MAX)
    {
        Size = (ULONG)(FileInformation->Size) - (ULONG)(FileInformation->Offset);
    }

    /* Check if the caller has an input buffer */
    if (InputSize)
    {
        /* Is the size bigger than what the caller can handle? */
        if (Size >= *InputSize)
        {
            /* Yes, so cap it at the size of the caller's buffer */
            Size = *InputSize;
        }
        else if (!(BytesReturned) || (Write))
        {
            /* Caller's input buffer is too smaller is fatal for writes */
            Status = STATUS_INVALID_PARAMETER;
            goto Quickie;
        }
    }

Quickie:
    /* Does the caller want to know how big to make their buffer? */
    if (RequiredSize)
    {
        /* Let them know*/
        *RequiredSize = Size;
    }

    /* Return final status */
    return Status;
}

NTSTATUS
BlFileReadEx (
    _In_ ULONG FileId,
    _Out_ PVOID Buffer,
    _In_ ULONG Size,
    _Out_ PULONG BytesReturned,
    _In_ ULONG Flags
    )
{
    PBL_FILE_ENTRY FileEntry;
    NTSTATUS Status;
    ULONG OldUnknown, RequiredSize;
    BOOLEAN ChangedUnknown;
    BL_DEVICE_INFORMATION DeviceInfo;
    BL_FILE_INFORMATION fileInfo;

    /* Initialize variables */
    RtlZeroMemory(&DeviceInfo, sizeof(DeviceInfo));
    OldUnknown = 0;
    ChangedUnknown = FALSE;

    /* Bail out if there's no buffer */
    if (!Buffer)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Bail out of the file ID is invalid */
    if (FileId > FileEntries)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Bail out if there's no file opened for read access */
    FileEntry = FileTable[FileId];
    if (!(FileEntry) ||
        !(FileEntry->Flags & (BL_FILE_ENTRY_OPENED | BL_FILE_ENTRY_READ_ACCESS)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Bail out if we can't read the file's information */
    Status = BlFileGetInformation(FileId, &fileInfo);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Ensure the read attempt is valid, and fix up the size if needed */
    RequiredSize = Size;
    Status = FileInformationCheck(&fileInfo,
                                  FALSE,
                                  &RequiredSize,
                                  BytesReturned,
                                  &RequiredSize);
    if (!NT_SUCCESS(Status))
    {
        /* Invalid or illegal read attempt */
        return Status;
    }

    /* Is there anything left to read after all? */
    if (RequiredSize)
    {
        /* Check if flags 2 or 4 are set */
        if ((Flags & 2) || (Flags & 4))
        {
            /* Check if this is a disk or partition device */
            BlDeviceGetInformation(FileEntry->DeviceId, &DeviceInfo);
            if ((DeviceInfo.DeviceType == DiskDevice) ||
                (DeviceInfo.DeviceType == LegacyPartitionDevice))
            {
                /* Check if request flags are incompatible with device flags */
                if ((!(DeviceInfo.BlockDeviceInfo.Unknown & 1) && (Flags & 2)) ||
                    (!(DeviceInfo.BlockDeviceInfo.Unknown & 2) && (Flags & 4)))
                {
                    /* We're going to change the device flags */
                    ChangedUnknown = TRUE;

                    /* Set unknown flag 1 for request flag 2 */
                    if (Flags & 2)
                    {
                        DeviceInfo.BlockDeviceInfo.Unknown |= 1;
                    }

                    /* Set unknown flag 2 for request flag 4 */
                    if (Flags & 4)
                    {
                        DeviceInfo.BlockDeviceInfo.Unknown |= 2;
                    }

                    /* Save the new device flags */
                    BlDeviceSetInformation(FileEntry->DeviceId, &DeviceInfo);
                }
            }
        }

        /* Issue the read to the underlying file system */
        Status = FileEntry->Callbacks.Read(FileEntry,
                                           Buffer,
                                           RequiredSize,
                                           BytesReturned);
        if (!NT_SUCCESS(Status))
        {
            /* Don't update the bytes read on failure */
            RequiredSize = 0;
        }
    }
    else
    {
        /* There's nothing to do, return success and 0 bytes */
        Status = STATUS_SUCCESS;
        if (BytesReturned)
        {
            *BytesReturned = 0;
        }
    }

    /* Increment the number of bytes read */
    FileEntry->TotalBytesRead += RequiredSize;

    /* Check if the unknown flag on the device was changed during this routine */
    if (ChangedUnknown)
    {
        /* Reset it back to its original value */
        DeviceInfo.BlockDeviceInfo.Unknown = OldUnknown;
        BlDeviceSetInformation(FileEntry->DeviceId, &DeviceInfo);
    }

    /* Return the final status */
    return Status;
}

NTSTATUS
BlFileReadAtOffsetEx (
    _In_ ULONG FileId,
    _In_ ULONG Size,
    _In_ ULONGLONG ByteOffset,
    _In_ PVOID Buffer,
    _Out_ PULONG BytesReturned,
    _In_ ULONG Flags
    )
{
    NTSTATUS Status;
    BL_FILE_INFORMATION FileInfo;
    ULONG RequiredSize;
    ULONGLONG FileOffset;

    /* Get information on the specified file */
    Status = BlFileGetInformation(FileId, &FileInfo);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Save the current offset, and overwrite it with the one we want */
    FileOffset = FileInfo.Offset;
    FileInfo.Offset = ByteOffset;

    /* Check the validity of the read and the actual size to read */
    RequiredSize = Size;
    Status = FileInformationCheck(&FileInfo,
                                  FALSE,
                                  &RequiredSize,
                                  BytesReturned,
                                  &RequiredSize);
    if (!NT_SUCCESS(Status))
    {
        /* Bail out if the read is invalid */
        EfiPrintf(L"File info check failure: %lx\r\n", Status);
        return Status;
    }

    /* Check if the offset we're requesting is not the current offset */
    if (FileInfo.Offset != FileOffset)
    {
        /* Set the new offset to use */
        Status = BlFileSetInformation(FileId, &FileInfo);
        if (!NT_SUCCESS(Status))
        {
            /* Can't do much if that failed */
            return Status;
        }
    }

    /* Do the read at the required offset now */
    Status = BlFileReadEx(FileId,
                          Buffer,
                          RequiredSize,
                          BytesReturned,
                          Flags);
    if (!NT_SUCCESS(Status))
    {
        /* The read failed -- had we modified the offset? */
        if (FileInfo.Offset != FileOffset)
        {
            /* Restore the offset back to its original value */
            FileInfo.Offset = FileOffset;
            BlFileSetInformation(FileId, &FileInfo);
        }
    }

    /* Return the status of the read */
    return Status;
}

NTSTATUS
BlpFileRegisterFileSystem (
    _In_ PBL_FS_INIT_CALLBACK InitCallback,
    _In_ PBL_FS_DESTROY_CALLBACK DestroyCallback,
    _In_ PBL_FS_MOUNT_CALLBACK MountCallback,
    _In_ PBL_FS_PURGE_CALLBACK PurgeCallback,
    _In_ ULONG Flags
    )
{
    PBL_FILE_SYSTEM_ENTRY FsEntry;
    NTSTATUS Status;

    /* Allocate an entry */
    FsEntry = BlMmAllocateHeap(sizeof(*FsEntry));
    if (!FsEntry)
    {
        return STATUS_NO_MEMORY;
    }

    /* Initialize the file system */
    Status = InitCallback();
    if (!NT_SUCCESS(Status))
    {
        BlMmFreeHeap(FsEntry);
        return Status;
    }

    /* Register the callbacks */
    FsEntry->MountCallback = MountCallback;
    FsEntry->DestroyCallback = DestroyCallback;
    FsEntry->InitCallback = InitCallback;
    FsEntry->PurgeCallback = PurgeCallback;

    /* Insert in the right location in the list */
    if (Flags & BL_FS_REGISTER_AT_HEAD_FLAG)
    {
        InsertHeadList(&RegisteredFileSystems, &FsEntry->ListEntry);
    }
    else
    {
        InsertTailList(&RegisteredFileSystems, &FsEntry->ListEntry);
    }

    /* Return */
    return STATUS_SUCCESS;
}

NTSTATUS
BlpFileInitialize (
    VOID
    )
{
    NTSTATUS Status;

    /* Allocate the file table */
    FileEntries = 16;
    FileTable = BlMmAllocateHeap(sizeof(PBL_FILE_ENTRY) * FileEntries);
    if (!FileTable)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Initialize it */
    RtlZeroMemory(FileTable, sizeof(PBL_FILE_ENTRY) * FileEntries);
    InitializeListHead(&RegisteredFileSystems);

#if 0
    /* Initialize the network file system */
    Status = BlpFileRegisterFileSystem(NetRegisterFunctionTable.Init,
                                       NetRegisterFunctionTable.Destroy,
                                       NetRegisterFunctionTable.Mount,
                                       NetRegisterFunctionTable.Purge,
                                       1);
    if (NT_SUCCESS(Status))
    {
        /* Initialize NTFS */
        Status = BlpFileRegisterFileSystem(NtfsRegisterFunctionTable.Init,
                                           NtfsRegisterFunctionTable.Destroy,
                                           NtfsRegisterFunctionTable.Mount,
                                           NtfsRegisterFunctionTable.Purge,
                                           0);
    }

    if (NT_SUCCESS(Status))
#endif
    {
        /* Initialize FAT */
        Status = BlpFileRegisterFileSystem(FatRegisterFunctionTable.Init,
                                           FatRegisterFunctionTable.Destroy,
                                           FatRegisterFunctionTable.Mount,
                                           FatRegisterFunctionTable.Purge,
                                           0);
    }

#if 0
    if (NT_SUCCESS(Status))
    {
        /* Initialize EXFAT (FatPlus) */
        Status = BlpFileRegisterFileSystem(FppRegisterFunctionTable.Init,
                                           FppRegisterFunctionTable.Destroy,
                                           FppRegisterFunctionTable.Mount,
                                           FppRegisterFunctionTable.Purge,
                                           0);
    }

    if (NT_SUCCESS(Status))
    {
        /* Initialize WIM */
        Status = BlpFileRegisterFileSystem(WimRegisterFunctionTable.Init,
                                           WimRegisterFunctionTable.Destroy,
                                           WimRegisterFunctionTable.Mount,
                                           WimRegisterFunctionTable.Purge,
                                           0);
    }

    if (NT_SUCCESS(Status))
    {
        /* Initialize UDFS */
        Status = BlpFileRegisterFileSystem(UdfsRegisterFunctionTable.Init,
                                           UdfsRegisterFunctionTable.Destroy,
                                           UdfsRegisterFunctionTable.Mount,
                                           UdfsRegisterFunctionTable.Purge,
                                           0);
    }
#endif
    if (NT_SUCCESS(Status))
    {
        /* Initialize El-Torito CDFS */
        Status = BlpFileRegisterFileSystem(EtfsRegisterFunctionTable.Init,
                                           EtfsRegisterFunctionTable.Destroy,
                                           EtfsRegisterFunctionTable.Mount,
                                           EtfsRegisterFunctionTable.Purge,
                                           0);
    }

    /* Destroy the file manager if any of the file systems didn't initialize */
    if (!NT_SUCCESS(Status))
    {
        if (FileTable)
        {
            //BlpFileDestroy();
        }
    }
    return Status;
}
