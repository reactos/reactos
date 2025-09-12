/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmwraprs.c
 * PURPOSE:         Configuration Manager - Wrappers for Hive Operations
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpCreateEvent(IN EVENT_TYPE EventType,
               OUT PHANDLE EventHandle,
               OUT PKEVENT *Event)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Create the event */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateEvent(EventHandle,
                           EVENT_ALL_ACCESS,
                           &ObjectAttributes,
                           EventType,
                           FALSE);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get a pointer to the object itself */
    Status = ObReferenceObjectByHandle(*EventHandle,
                                       EVENT_ALL_ACCESS,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)Event,
                                       NULL);
    if (!NT_SUCCESS(Status)) ZwClose(*EventHandle);

    /* Return status */
    return Status;
}

PVOID
NTAPI
CmpAllocate(IN SIZE_T Size,
            IN BOOLEAN Paged,
            IN ULONG Tag)
{
    return ExAllocatePoolWithTag(Paged ? PagedPool : NonPagedPool,
                                 Size,
                                 Tag);
}

VOID
NTAPI
CmpFree(IN PVOID Ptr,
        IN ULONG Quota)
{
    ExFreePool(Ptr);
}

BOOLEAN
NTAPI
CmpFileRead(IN PHHIVE RegistryHive,
            IN ULONG FileType,
            IN PULONG FileOffset,
            OUT PVOID Buffer,
            IN SIZE_T BufferLength)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    HANDLE HiveHandle = CmHive->FileHandles[FileType];
    LARGE_INTEGER _FileOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Just return success if no file is associated with this hive */
    if (HiveHandle == NULL)
        return TRUE;

    _FileOffset.QuadPart = *FileOffset;
    Status = ZwReadFile(HiveHandle, NULL, NULL, NULL, &IoStatusBlock,
                        Buffer, (ULONG)BufferLength, &_FileOffset, NULL);
    /* We do synchronous I/O for simplicity - see CmpOpenHiveFiles. */
    ASSERT(Status != STATUS_PENDING);
    return NT_SUCCESS(Status) ? TRUE : FALSE;
}

BOOLEAN
NTAPI
CmpFileWrite(IN PHHIVE RegistryHive,
             IN ULONG FileType,
             IN PULONG FileOffset,
             IN PVOID Buffer,
             IN SIZE_T BufferLength)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    HANDLE HiveHandle = CmHive->FileHandles[FileType];
    LARGE_INTEGER _FileOffset;
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Just return success if no file is associated with this hive */
    if (HiveHandle == NULL)
        return TRUE;

    /* Don't do anything if we're not supposed to */
    if (CmpNoWrite)
        return TRUE;

    _FileOffset.QuadPart = *FileOffset;
    Status = ZwWriteFile(HiveHandle, NULL, NULL, NULL, &IoStatusBlock,
                         Buffer, (ULONG)BufferLength, &_FileOffset, NULL);
    /* We do synchronous I/O for simplicity - see CmpOpenHiveFiles.
     * Windows optimizes here by starting an async write for each 64k chunk,
     * then waiting for all writes to complete at once.
     */
    ASSERT(Status != STATUS_PENDING);
    return NT_SUCCESS(Status) ? TRUE : FALSE;
}

BOOLEAN
NTAPI
CmpFileSetSize(
    _In_ PHHIVE RegistryHive,
    _In_ ULONG FileType,
    _In_ ULONG FileSize,
    _In_ ULONG OldFileSize)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    HANDLE HiveHandle = CmHive->FileHandles[FileType];
    FILE_END_OF_FILE_INFORMATION EndOfFileInfo;
    FILE_ALLOCATION_INFORMATION FileAllocationInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    BOOLEAN HardErrors;
    NTSTATUS Status;

    /* Just return success if no file is associated with this hive */
    if (HiveHandle == NULL)
    {
        DPRINT1("No hive handle associated with the given hive\n");
        return TRUE;
    }

    /*
     * Disable hard errors so that we don't deadlock
     * when touching with the hive files.
     */
    HardErrors = IoSetThreadHardErrorMode(FALSE);

    EndOfFileInfo.EndOfFile.QuadPart = FileSize;
    Status = ZwSetInformationFile(HiveHandle,
                                  &IoStatusBlock,
                                  &EndOfFileInfo,
                                  sizeof(FILE_END_OF_FILE_INFORMATION),
                                  FileEndOfFileInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwSetInformationFile failed to set new size of end of file (Status 0x%lx)\n", Status);
        IoSetThreadHardErrorMode(HardErrors);
        return FALSE;
    }

    FileAllocationInfo.AllocationSize.QuadPart = FileSize;
    Status = ZwSetInformationFile(HiveHandle,
                                  &IoStatusBlock,
                                  &FileAllocationInfo,
                                  sizeof(FILE_ALLOCATION_INFORMATION),
                                  FileAllocationInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwSetInformationFile failed to set new of allocation file (Status 0x%lx)\n", Status);
        IoSetThreadHardErrorMode(HardErrors);
        return FALSE;
    }

    /* Reset the hard errors back */
    IoSetThreadHardErrorMode(HardErrors);
    return TRUE;
}

BOOLEAN
NTAPI
CmpFileFlush(IN PHHIVE RegistryHive,
             IN ULONG FileType,
             IN OUT PLARGE_INTEGER FileOffset,
             IN ULONG Length)
{
    PCMHIVE CmHive = (PCMHIVE)RegistryHive;
    HANDLE HiveHandle = CmHive->FileHandles[FileType];
    IO_STATUS_BLOCK IoStatusBlock;
    NTSTATUS Status;

    /* Just return success if no file is associated with this hive */
    if (HiveHandle == NULL)
        return TRUE;

    /* Don't do anything if we're not supposed to */
    if (CmpNoWrite)
        return TRUE;

    Status = ZwFlushBuffersFile(HiveHandle, &IoStatusBlock);

    /* This operation is always synchronous */
    ASSERT(Status != STATUS_PENDING);
    ASSERT(Status == IoStatusBlock.Status);

    return NT_SUCCESS(Status) ? TRUE : FALSE;
}
