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
    ULONG PathSize, PathSizeWithNull;
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
    ULONG PathSize;

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

    if ((FileEntry->DeviceId == DeviceId) && !(_wcsicmp(FileEntry->FilePath, FilePath)) && (FileEntry->Unknown == Unknown))
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

    if ((FileEntry->DeviceId == DeviceId) && !(_wcsicmp(FileEntry->FilePath, FilePath)) && (FileEntry->Unknown == Unknown))
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

NTSTATUS
FileTablePurgeEntry (
    _In_ PVOID Entry
    )
{
    PBL_FILE_ENTRY FileEntry = (PBL_FILE_ENTRY)Entry;
    NTSTATUS Status;

    if (((FileEntry->Flags & 1) || (FileEntry->Flags & 0x10000)) && (FileEntries < 0x200))
    {
        Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        Status = FileTableDestroyEntry(FileEntry, FileEntry->FileId);
    }

    return Status;
}

NTSTATUS
BlFileClose (
    _In_ ULONG FileId
    )
{
    PBL_FILE_ENTRY FileEntry;

    if (FileEntries <= FileId)
    {
        return STATUS_INVALID_PARAMETER;
    }

    FileEntry = FileTable[FileId];
    if (!FileEntry)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (!(FileEntry->Flags & 1))
    {
        return STATUS_INVALID_PARAMETER;
    }

    --FileEntry->ReferenceCount;
    if (!FileEntry->ReferenceCount)
    {
        FileEntry->Flags &= ~1;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
FileIoOpen (
    _In_ ULONG DeviceId,
    _In_ PWCHAR FileName,
    _In_ ULONG Flags,
    _In_ ULONG Unknown,
    _In_ PBL_TBL_LOOKUP_ROUTINE CompareRoutine,
    _Out_ PBL_FILE_ENTRY *ReturnFileEntry
    )
{
    PWCHAR FileNameCopy, ParentFileName;
    NTSTATUS Status;
    PBL_DEVICE_ENTRY DeviceEntry;
    PBL_FILE_SYSTEM_ENTRY FileSystem;
    ULONG FileId;
    PBL_FILE_ENTRY ParentDirectoryEntry, FileEntry;
    PLIST_ENTRY NextEntry, ListHead;

    ParentDirectoryEntry = NULL;
    FileNameCopy = NULL;
    Flags |= 1;
    ParentFileName = NULL;
    Status = STATUS_SUCCESS;

    if (DmTableEntries <= DeviceId)
    {
        return STATUS_ACCESS_DENIED;
    }

    DeviceEntry = DmDeviceTable[DeviceId];
    if (!DeviceEntry)
    {
        return STATUS_ACCESS_DENIED;
    }

    if ((Flags & 1) && (!(DeviceEntry->Flags & 1) || !(DeviceEntry->Flags & 2)))
    {
        EfiPrintf(L"Access denied\r\n");
        return STATUS_ACCESS_DENIED;
    }

    if ((Flags & 2) && (!(DeviceEntry->Flags & 1) || !(DeviceEntry->Flags & 4)))
    {
        EfiPrintf(L"Access denied2\r\n");
        return STATUS_ACCESS_DENIED;
    }

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
        EfiPrintf(L"Entry exists: %p\n", FileEntry);
        goto FileOpened;
    }

    if ((*FileName != OBJ_NAME_PATH_SEPARATOR) || (FileName[1]))
    {
        ParentFileName = FileIoCopyParentDirectoryPath(FileName);
        if (!ParentFileName)
        {
            Status = STATUS_NO_MEMORY;
            goto FileOpenEnd;
        }

        Status = FileIoOpen(DeviceId,
                            ParentFileName,
                            5,
                            Unknown,
                            FileTableCompareWithSubsetAttributes,
                            &ParentDirectoryEntry);
        if (Status < 0)
        {
            goto FileOpenEnd;
        }

        FileNameCopy = FileIoCopyFileName(FileName);
        if (!FileNameCopy)
        {
            Status = STATUS_NO_MEMORY;
            goto FileOpenEnd;
        }

        Status = ParentDirectoryEntry->Callbacks.Open(ParentDirectoryEntry,
                                                      FileNameCopy,
                                                      Flags,
                                                      &FileEntry);
    }
    else
    {
        EfiPrintf(L"Opening root drive\r\n");
        Status = STATUS_UNSUCCESSFUL;

        ListHead = &RegisteredFileSystems;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            FileSystem = CONTAINING_RECORD(NextEntry, BL_FILE_SYSTEM_ENTRY, ListEntry);

            EfiPrintf(L"Calling filesystem %p mount routine: %p\r\n", FileSystem, FileSystem->MountCallback);
            Status = FileSystem->MountCallback(DeviceId, Unknown, &FileEntry);
            if (NT_SUCCESS(Status))
            {
                break;
            }

            NextEntry = NextEntry->Flink;
        }

        FileNameCopy = NULL;
    }

    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Could not open file!: %lx\r\n", Status);
        goto FileOpenEnd;
    }

    FileEntry->Unknown = Unknown;

    if (Flags & 1)
    {
        FileEntry->Flags |= 2u;
    }
    
    if (Flags & 2)
    {
        FileEntry->Flags |= 4u;
    }

    Status = BlTblSetEntry(&FileTable,
                           &FileEntries,
                           (PVOID)FileEntry,
                           &FileId,
                           FileTablePurgeEntry);
    if (!NT_SUCCESS(Status))
    {
        FileEntry->Callbacks.Close(FileEntry);
        goto FileOpenEnd;
    }

    ++DeviceEntry->ReferenceCount;
    Status = STATUS_SUCCESS;

    EfiPrintf(L"File %s opened with ID: %lx\r\n", FileEntry->FilePath, FileId);
    FileEntry->FileId = FileId;

FileOpened:
    if (++FileEntry->ReferenceCount == 1)
    {
        FileEntry->Unknown1 = 0;
        FileEntry->Unknown2 = 0;
    }

    FileEntry->Flags |= 1;
    
    if (Flags & 0x10)
    {
        FileEntry->Flags |= 0x10;
    }

    if (ReturnFileEntry)
    {
        *ReturnFileEntry = FileEntry;
    }

FileOpenEnd:
    if (ParentDirectoryEntry)
    {
        BlFileClose(ParentDirectoryEntry->FileId);
    }
    if (ParentFileName)
    {
        BlMmFreeHeap(ParentFileName);
    }
    if (FileNameCopy)
    {
        BlMmFreeHeap(FileNameCopy);
    }
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

    if (!(FileName) ||
        (*FileName != OBJ_NAME_PATH_SEPARATOR) ||
        !(FileId) ||
        !(Flags & 3))
    {
        EfiPrintf(L"Invalid file options\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    Status = BlDeviceGetInformation(DeviceId, &DeviceInformation);
    if (!NT_SUCCESS(Status))
    {
        EfiPrintf(L"Get device info failed: %lx\r\n", Status);
        return Status;
    }

    if ((DeviceInformation.DeviceType != DiskDevice) &&
        (DeviceInformation.DeviceType != LegacyPartitionDevice) &&
        (DeviceInformation.DeviceType != UdpDevice))
    {
        EfiPrintf(L"Invalid device type\r\n");
        return STATUS_INVALID_PARAMETER;
    }

    Status = FileIoOpen(DeviceId,
                        FileName,
                        Flags,
                        0,
                        FileTableCompareWithSameAttributes,
                        &FileEntry);
    if (NT_SUCCESS(Status))
    {
        EfiPrintf(L"File opened: %lx\r\n", FileEntry->FileId);
        *FileId = FileEntry->FileId;
    }

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
