/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cminit.c
 * PURPOSE:         Configuration Manager - Hive Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitializeHive(OUT PCMHIVE *RegistryHive,
                  IN ULONG OperationType,
                  IN ULONG HiveFlags,
                  IN ULONG FileType,
                  IN PVOID HiveData OPTIONAL,
                  IN HANDLE Primary,
                  IN HANDLE Log,
                  IN HANDLE External,
                  IN PUNICODE_STRING FileName OPTIONAL,
                  IN ULONG CheckFlags)
{
#if 0
    PCMHIVE Hive;
#else
    PEREGISTRY_HIVE Hive;
#endif
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION FileSizeInformation;
    NTSTATUS Status;
    ULONG Cluster;

    /* Assume failure */
    *RegistryHive = NULL;

    /*
     * The following are invalid:
     * An external hive that is also internal.
     * A log hive that's not a primary hive too.
     * A volatile hive that's linked to permanent storage.
     * An in-memory initialization without hive data.
     * A log hive that's not linked to a correct file type.
     */
    if (((External) && ((Primary) || (Log))) ||
        ((Log) && !(Primary)) ||
        ((HiveFlags & HIVE_VOLATILE) && ((Primary) || (External) || (Log))) ||
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
    Hive = ExAllocatePoolWithTag(NonPagedPool, sizeof(EREGISTRY_HIVE), TAG_CM);
    if (!Hive) return STATUS_INSUFFICIENT_RESOURCES;
#if 0
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

    /* Set loading flag */
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
                                           TAG_CM);
    if (!Hive->ViewLock) return STATUS_INSUFFICIENT_RESOURCES;

    /* Allocate the flush lock */
    Hive->FlusherLock = ExAllocatePoolWithTag(NonPagedPool,
                                              sizeof(ERESOURCE),
                                              TAG_CM);
    if (!Hive->FlusherLock) return STATUS_INSUFFICIENT_RESOURCES;

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
    CmpInitializeHiveViewList(Hive);

    /* Initailize the security cache */
    CmpInitializeSecurityCache(Hive);

    /* Setup flags */
    Hive->Flags = 0;
    Hive->FlushCount = 0;
#else
    /* Clear it */
    RtlZeroMemory(Hive, sizeof(EREGISTRY_HIVE));

    /* Set flags */
    Hive->Flags = HiveFlags;
#endif

    /* Initialize it */
    Status = HvInitialize(&Hive->Hive,
                          OperationType,
                          HiveFlags,
                          FileType,
                          (ULONG_PTR)HiveData,
                          Cluster,
                          CmpAllocate,
                          CmpFree,
                          CmpFileRead,
                          CmpFileWrite,
                          CmpFileSetSize,
                          CmpFileFlush,
                          FileName);
    if (!NT_SUCCESS(Status))
    {
        /* Clear allocations and fail */
#if 0
        ExFreePool(Hive->ViewLock);
        ExFreePool(Hive->FlusherLock);
#endif
        ExFreePool(Hive);
        return Status;
    }

    /* Check if we should verify the registry */
    if ((OperationType == HINIT_FILE) ||
        (OperationType == HINIT_MEMORY) ||
        (OperationType == HINIT_MEMORY_INPLACE) ||
        (OperationType == HINIT_MAPFILE))
    {
        /* Verify integrity */
        if (CmCheckRegistry((PCMHIVE)Hive, TRUE))
        {
            /* Free all alocations */
#if 0
            ExFreePool(Hive->ViewLock);
            ExFreePool(Hive->FlusherLock);
#endif
            ExFreePool(Hive);
            return STATUS_REGISTRY_CORRUPT;
        }
    }

    /* Lock the hive list */
    ExAcquirePushLockExclusive(&CmpHiveListHeadLock);

    /* Insert this hive */
    InsertHeadList(&CmpHiveListHead, &Hive->HiveList);

    /* Release the lock */
    ExReleasePushLock(&CmpHiveListHeadLock);

    /* Return the hive and success */
    *RegistryHive = (PCMHIVE)Hive;
    return STATUS_SUCCESS;
}
