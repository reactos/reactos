/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cminit.c
 * PURPOSE:         Configuration Manager - Hive Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitializeHive(OUT PCMHIVE *CmHive,
                  IN ULONG OperationType,
                  IN ULONG HiveFlags,
                  IN ULONG FileType,
                  IN PVOID HiveData OPTIONAL,
                  IN HANDLE Primary,
                  IN HANDLE Log,
                  IN HANDLE External,
                  IN PCUNICODE_STRING FileName OPTIONAL,
                  IN ULONG CheckFlags)
{
    PCMHIVE Hive;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION FileSizeInformation;
    NTSTATUS Status;
    ULONG Cluster;

    /* Assume failure */
    *CmHive = NULL;

    /*
     * The following are invalid:
     * - An external hive that is also internal.
     * - A log hive that is not a primary hive too.
     * - A volatile hive that is linked to permanent storage,
     *   unless this hive is a shared system hive.
     * - An in-memory initialization without hive data.
     * - A log hive that is not linked to a correct file type.
     */
    if (((External) && ((Primary) || (Log))) ||
        ((Log) && !(Primary)) ||
        (!(CmpShareSystemHives) && (HiveFlags & HIVE_VOLATILE) &&
            ((Primary) || (External) || (Log))) ||
        ((OperationType == HINIT_MEMORY) && (!HiveData)) ||
        ((Log) && (FileType != HFILE_TYPE_LOG)))
    {
        /* Fail the request */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if this is a primary hive */
    if (Primary)
    {
        /* Get the cluster size */
        Status = ZwQueryVolumeInformationFile(Primary,
                                              &IoStatusBlock,
                                              &FileSizeInformation,
                                              sizeof(FILE_FS_SIZE_INFORMATION),
                                              FileFsSizeInformation);
        if (!NT_SUCCESS(Status)) return Status;

        /* Make sure it's not larger then the block size */
        if (FileSizeInformation.BytesPerSector > HBLOCK_SIZE)
        {
            /* Fail */
            return STATUS_REGISTRY_IO_FAILED;
        }

        /* Otherwise, calculate the cluster */
        Cluster = FileSizeInformation.BytesPerSector / HSECTOR_SIZE;
        Cluster = max(1, Cluster);
    }
    else
    {
        /* Otherwise use cluster 1 */
        Cluster = 1;
    }

    /* Allocate the hive */
    Hive = ExAllocatePoolWithTag(NonPagedPool, sizeof(CMHIVE), TAG_CMHIVE);
    if (!Hive) return STATUS_INSUFFICIENT_RESOURCES;

    /* Setup null fields */
    Hive->UnloadEvent = NULL;
    Hive->RootKcb = NULL;
    Hive->Frozen = FALSE;
    Hive->UnloadWorkItem = NULL;
    Hive->GrowOnlyMode = FALSE;
    Hive->GrowOffset = 0;
    Hive->CellRemapArray = NULL;
    Hive->UseCountLog.Next = 0;
    Hive->LockHiveLog.Next = 0;
    Hive->FileObject = NULL;
    Hive->NotifyList.Flink = NULL;
    Hive->NotifyList.Blink = NULL;

    /* Set the loading flag */
    Hive->HiveIsLoading = TRUE;

    /* Set the current thread as creator */
    Hive->CreatorOwner = KeGetCurrentThread();

    /* Initialize lists */
    InitializeListHead(&Hive->KcbConvertListHead);
    InitializeListHead(&Hive->KnodeConvertListHead);
    InitializeListHead(&Hive->TrustClassEntry);

    /* Allocate the view log */
    Hive->ViewLock = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(KGUARDED_MUTEX),
                                           TAG_CMHIVE);
    if (!Hive->ViewLock)
    {
        /* Cleanup allocation and fail */
        ExFreePoolWithTag(Hive, TAG_CMHIVE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Allocate the flush lock */
    Hive->FlusherLock = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(ERESOURCE),
                                              TAG_CMHIVE);
    if (!Hive->FlusherLock)
    {
        /* Cleanup allocations and fail */
        ExFreePoolWithTag(Hive->ViewLock, TAG_CMHIVE);
        ExFreePoolWithTag(Hive, TAG_CMHIVE);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Setup the handles */
    Hive->FileHandles[HFILE_TYPE_PRIMARY] = Primary;
    Hive->FileHandles[HFILE_TYPE_LOG] = Log;
    Hive->FileHandles[HFILE_TYPE_EXTERNAL] = External;

    /* Initailize the guarded mutex */
    KeInitializeGuardedMutex(Hive->ViewLock);
    Hive->ViewLockOwner = NULL;

    /* Initialize the flush lock */
    ExInitializeResourceLite(Hive->FlusherLock);

    /* Setup hive locks */
    ExInitializePushLock(&Hive->HiveLock);
    Hive->HiveLockOwner = NULL;
    ExInitializePushLock(&Hive->WriterLock);
    Hive->WriterLockOwner = NULL;
    ExInitializePushLock(&Hive->SecurityLock);
    Hive->HiveSecurityLockOwner = NULL;

    /* Clear file names */
    RtlInitEmptyUnicodeString(&Hive->FileUserName, NULL, 0);
    RtlInitEmptyUnicodeString(&Hive->FileFullPath, NULL, 0);

    /* Initialize the view list */
    CmpInitHiveViewList(Hive);

    /* Initailize the security cache */
    CmpInitSecurityCache(Hive);

    /* Setup flags */
    Hive->Flags = 0;
    Hive->FlushCount = 0;

    /* Initialize it */
    Status = HvInitialize(&Hive->Hive,
                          OperationType,
                          HiveFlags,
                          FileType,
                          HiveData,
                          CmpAllocate,
                          CmpFree,
                          CmpFileSetSize,
                          CmpFileWrite,
                          CmpFileRead,
                          CmpFileFlush,
                          Cluster,
                          FileName);
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup allocations and fail */
        ExDeleteResourceLite(Hive->FlusherLock);
        ExFreePoolWithTag(Hive->FlusherLock, TAG_CMHIVE);
        ExFreePoolWithTag(Hive->ViewLock, TAG_CMHIVE);
        ExFreePoolWithTag(Hive, TAG_CMHIVE);
        return Status;
    }

    /* Check if we should verify the registry */
    if ((OperationType == HINIT_FILE) ||
        (OperationType == HINIT_MEMORY) ||
        (OperationType == HINIT_MEMORY_INPLACE) ||
        (OperationType == HINIT_MAPFILE))
    {
        /* Verify integrity */
        CM_CHECK_REGISTRY_STATUS CheckStatus = CmCheckRegistry(Hive, CheckFlags);
        if (!CM_CHECK_REGISTRY_SUCCESS(CheckStatus))
        {
            /* Cleanup allocations and fail */
            ExDeleteResourceLite(Hive->FlusherLock);
            ExFreePoolWithTag(Hive->FlusherLock, TAG_CMHIVE);
            ExFreePoolWithTag(Hive->ViewLock, TAG_CMHIVE);
            ExFreePoolWithTag(Hive, TAG_CMHIVE);
            return STATUS_REGISTRY_CORRUPT;
        }
    }

    /* Reset the loading flag */
    Hive->HiveIsLoading = FALSE;

    /* Lock the hive list */
    ExAcquirePushLockExclusive(&CmpHiveListHeadLock);

    /* Insert this hive */
    InsertHeadList(&CmpHiveListHead, &Hive->HiveList);

    /* Release the lock */
    ExReleasePushLock(&CmpHiveListHeadLock);

    /* Return the hive and success */
    *CmHive = Hive;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpDestroyHive(IN PCMHIVE CmHive)
{
    /* Remove the hive from the list */
    ExAcquirePushLockExclusive(&CmpHiveListHeadLock);
    RemoveEntryList(&CmHive->HiveList);
    ExReleasePushLock(&CmpHiveListHeadLock);

    /* Destroy the security descriptor cache */
    CmpDestroySecurityCache(CmHive);

    /* Destroy the view list */
    CmpDestroyHiveViewList(CmHive);

    /* Delete the flusher lock */
    ExDeleteResourceLite(CmHive->FlusherLock);
    ExFreePoolWithTag(CmHive->FlusherLock, TAG_CMHIVE);

    /* Delete the view lock */
    ExFreePoolWithTag(CmHive->ViewLock, TAG_CMHIVE);

    /* Free the hive storage */
    HvFree(&CmHive->Hive);

    /* Free the hive */
    CmpFree(CmHive, TAG_CM);

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpOpenHiveFiles(IN PCUNICODE_STRING BaseName,
                 IN PCWSTR Extension OPTIONAL,
                 OUT PHANDLE Primary,
                 OUT PHANDLE Log,
                 OUT PULONG PrimaryDisposition,
                 OUT PULONG LogDisposition,
                 IN BOOLEAN CreateAllowed,
                 IN BOOLEAN MarkAsSystemHive,
                 IN BOOLEAN NoBuffering,
                 OUT PULONG ClusterSize OPTIONAL)
{
    HANDLE EventHandle;
    PKEVENT Event;
    NTSTATUS Status;
    UNICODE_STRING FullName, ExtensionName;
    PWCHAR NameBuffer;
    USHORT Length;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    ULONG AttributeFlags, ShareMode, DesiredAccess, CreateDisposition, IoFlags;
    USHORT CompressionState;
    FILE_STANDARD_INFORMATION FileInformation;
    FILE_FS_SIZE_INFORMATION FsSizeInformation;

    /* Create event */
    Status = CmpCreateEvent(NotificationEvent, &EventHandle, &Event);
    if (!NT_SUCCESS(Status)) return Status;

    /* Initialize the full name */
    RtlInitEmptyUnicodeString(&FullName, NULL, 0);
    Length = BaseName->Length;

    /* Check if we have an extension */
    if (Extension)
    {
        /* Update the name length */
        Length += (USHORT)wcslen(Extension) * sizeof(WCHAR) + sizeof(UNICODE_NULL);

        /* Allocate the buffer for the full name */
        NameBuffer = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
        if (!NameBuffer)
        {
            /* Fail */
            ObDereferenceObject(Event);
            ZwClose(EventHandle);
            return STATUS_NO_MEMORY;
        }

        /* Build the full name */
        FullName.Buffer = NameBuffer;
        FullName.MaximumLength = Length;
        RtlCopyUnicodeString(&FullName, BaseName);
    }
    else
    {
        /* The base name is the full name */
        FullName = *BaseName;
        NameBuffer = NULL;
    }

    /* Initialize the attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &FullName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Check if we can create the hive */
    if ((CreateAllowed) && !(CmpShareSystemHives))
    {
        /* Open only or create */
        CreateDisposition = FILE_OPEN_IF;
    }
    else
    {
        /* Open only */
        CreateDisposition = FILE_OPEN;
    }

    /* Setup the flags */
    // FIXME : FILE_OPEN_FOR_BACKUP_INTENT is unimplemented and breaks 3rd stage boot
    IoFlags = //FILE_OPEN_FOR_BACKUP_INTENT |
              FILE_NO_COMPRESSION |
              FILE_RANDOM_ACCESS |
              (NoBuffering ? FILE_NO_INTERMEDIATE_BUFFERING : 0);

    /* Set share and access modes */
    if ((CmpMiniNTBoot) && (CmpShareSystemHives))
    {
        /* We're on Live CD or otherwise sharing */
        DesiredAccess = FILE_READ_DATA;
        ShareMode = FILE_SHARE_READ;
    }
    else
    {
        /* We want to write exclusively */
        ShareMode = 0;
        DesiredAccess = FILE_READ_DATA | FILE_WRITE_DATA;
    }

    /* Default attributes */
    AttributeFlags = FILE_ATTRIBUTE_NORMAL;

    /* Now create the file.
     * Note: We use FILE_SYNCHRONOUS_IO_NONALERT here to simplify CmpFileRead/CmpFileWrite.
     *       Windows does async I/O and therefore does not use this flag (or SYNCHRONIZE).
     */
    Status = ZwCreateFile(Primary,
                          DesiredAccess | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          AttributeFlags,
                          ShareMode,
                          CreateDisposition,
                          FILE_SYNCHRONOUS_IO_NONALERT | IoFlags,
                          NULL,
                          0);
    /* Check if anything failed until now */
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwCreateFile(%wZ) failed, Status 0x%08lx.\n", ObjectAttributes.ObjectName, Status);

        /* Close handles and free buffers */
        if (NameBuffer) ExFreePoolWithTag(NameBuffer, TAG_CM);
        ObDereferenceObject(Event);
        ZwClose(EventHandle);
        *Primary = NULL;
        return Status;
    }

    if (MarkAsSystemHive)
    {
        /* We opened it, mark it as a system hive */
        Status = ZwFsControlFile(*Primary,
                                 EventHandle,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_MARK_AS_SYSTEM_HIVE,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        /* If we don't support it, ignore the failure */
        if (Status == STATUS_INVALID_DEVICE_REQUEST) Status = STATUS_SUCCESS;

        if (!NT_SUCCESS(Status))
        {
            /* Close handles and free buffers */
            if (NameBuffer) ExFreePoolWithTag(NameBuffer, TAG_CM);
            ObDereferenceObject(Event);
            ZwClose(EventHandle);
            ZwClose(*Primary);
            *Primary = NULL;
            return Status;
        }
    }

    /* Disable compression */
    CompressionState = 0;
    Status = ZwFsControlFile(*Primary,
                             EventHandle,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_SET_COMPRESSION,
                             &CompressionState,
                             sizeof(CompressionState),
                             NULL,
                             0);
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        KeWaitForSingleObject(Event,
                              Executive,
                              KernelMode,
                              FALSE,
                              NULL);
    }

    /* Get the disposition */
    *PrimaryDisposition = (ULONG)IoStatusBlock.Information;
    if (IoStatusBlock.Information != FILE_CREATED)
    {
        /* Check how large the file is */
        Status = ZwQueryInformationFile(*Primary,
                                        &IoStatusBlock,
                                        &FileInformation,
                                        sizeof(FileInformation),
                                        FileStandardInformation);
        if (NT_SUCCESS(Status))
        {
            /* Check if it's 0 bytes */
            if (!FileInformation.EndOfFile.QuadPart)
            {
                /* Assume it's a new file */
                *PrimaryDisposition = FILE_CREATED;
            }
        }
    }

    /* Check if the caller wants cluster size returned */
    if (ClusterSize)
    {
        /* Query it */
        Status = ZwQueryVolumeInformationFile(*Primary,
                                              &IoStatusBlock,
                                              &FsSizeInformation,
                                              sizeof(FsSizeInformation),
                                              FileFsSizeInformation);
        if (!NT_SUCCESS(Status))
        {
            /* Close handles and free buffers */
            if (NameBuffer) ExFreePoolWithTag(NameBuffer, TAG_CM);
            ObDereferenceObject(Event);
            ZwClose(EventHandle);
            return Status;
        }

        /* Check if the sector size is invalid */
        if (FsSizeInformation.BytesPerSector > HBLOCK_SIZE)
        {
            /* Close handles and free buffers */
            if (NameBuffer) ExFreePoolWithTag(NameBuffer, TAG_CM);
            ObDereferenceObject(Event);
            ZwClose(EventHandle);
            return STATUS_CANNOT_LOAD_REGISTRY_FILE;
        }

        /* Return cluster size */
        *ClusterSize = max(1, FsSizeInformation.BytesPerSector / HSECTOR_SIZE);
    }

    /* Check if we don't need to create a log file */
    if (!Extension)
    {
        /* We're done, close handles */
        ObDereferenceObject(Event);
        ZwClose(EventHandle);
        return STATUS_SUCCESS;
    }

    /* Check if we can create the hive */
    CreateDisposition = CmpShareSystemHives ? FILE_OPEN : FILE_OPEN_IF;
    if (*PrimaryDisposition == FILE_CREATED)
    {
        /* Over-write the existing log file, since this is a new hive */
        CreateDisposition = FILE_SUPERSEDE;
    }

    /* Setup the name */
    RtlInitUnicodeString(&ExtensionName, Extension);
    RtlAppendUnicodeStringToString(&FullName, &ExtensionName);

    /* Initialize the attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               &FullName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Setup the flags */
    IoFlags = FILE_NO_COMPRESSION | FILE_NO_INTERMEDIATE_BUFFERING;

    /* Check if this is a log file */
    if (!_wcsnicmp(Extension, L".log", 4))
    {
        /* Hide log files */
        AttributeFlags |= FILE_ATTRIBUTE_HIDDEN;
    }

    /* Now create the file.
     * Note: We use FILE_SYNCHRONOUS_IO_NONALERT here to simplify CmpFileRead/CmpFileWrite.
     *       Windows does async I/O and therefore does not use this flag (or SYNCHRONIZE).
     */
    Status = ZwCreateFile(Log,
                          DesiredAccess | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          AttributeFlags,
                          ShareMode,
                          CreateDisposition,
                          FILE_SYNCHRONOUS_IO_NONALERT | IoFlags,
                          NULL,
                          0);
    if ((NT_SUCCESS(Status)) && (MarkAsSystemHive))
    {
        /* We opened it, mark it as a system hive */
        Status = ZwFsControlFile(*Log,
                                 EventHandle,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_MARK_AS_SYSTEM_HIVE,
                                 NULL,
                                 0,
                                 NULL,
                                 0);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
            Status = IoStatusBlock.Status;
        }

        /* If we don't support it, ignore the failure */
        if (Status == STATUS_INVALID_DEVICE_REQUEST) Status = STATUS_SUCCESS;

        /* If we failed, close the handle */
        if (!NT_SUCCESS(Status)) ZwClose(*Log);
    }

    /* Check if anything failed until now */
    if (!NT_SUCCESS(Status))
    {
        /* Clear the handle */
        *Log = NULL;
    }
    else
    {
        /* Disable compression */
        Status = ZwFsControlFile(*Log,
                                 EventHandle,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_SET_COMPRESSION,
                                 &CompressionState,
                                 sizeof(CompressionState),
                                 NULL,
                                 0);
        if (Status == STATUS_PENDING)
        {
            /* Wait for completion */
            KeWaitForSingleObject(Event,
                                  Executive,
                                  KernelMode,
                                  FALSE,
                                  NULL);
        }

        /* Return the disposition */
        *LogDisposition = (ULONG)IoStatusBlock.Information;
    }

    /* We're done, close handles and free buffers */
    if (NameBuffer) ExFreePoolWithTag(NameBuffer, TAG_CM);
    ObDereferenceObject(Event);
    ZwClose(EventHandle);
    return STATUS_SUCCESS;
}

VOID
NTAPI
CmpCloseHiveFiles(IN PCMHIVE Hive)
{
    ULONG i;

    for (i = 0; i < HFILE_TYPE_MAX; i++)
    {
        if (Hive->FileHandles[i] != NULL)
        {
            ZwClose(Hive->FileHandles[i]);
            Hive->FileHandles[i] = NULL;
        }
    }
}
