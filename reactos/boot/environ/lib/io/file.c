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
#if 0
    FatDestroy,
    FatMount,
    NULL
#endif
};

/* FUNCTIONS *****************************************************************/

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

    FsEntry = BlMmAllocateHeap(sizeof(*FsEntry));
    if (!FsEntry)
    {
        return STATUS_NO_MEMORY;
    }

    Status = InitCallback();
    if (NT_SUCCESS(Status))
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

    if (NT_SUCCESS(Status))
    {
        /* Initialize El-Torito CDFS */
        Status = BlpFileRegisterFileSystem(EtfsRegisterFunctionTable.Init,
                                           EtfsRegisterFunctionTable.Destroy,
                                           EtfsRegisterFunctionTable.Mount,
                                           EtfsRegisterFunctionTable.Purge,
                                           0);
    }
#endif

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
