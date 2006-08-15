/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cmhive.c
 * PURPOSE:         Routines for managing Registry Hives in general
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

EX_PUSH_LOCK CmpHiveListHeadLock;
LIST_ENTRY CmpHiveListHead;

/* FUNCTIONS *****************************************************************/

PVOID
NTAPI
CmpAllocate(IN ULONG Size,
            IN BOOLEAN Paged)
{
    /* FIXME: TODO */
    DbgBreakPoint();
    return NULL;
}

VOID
NTAPI
CmpFree(IN PVOID Block)
{
    /* FIXME: TODO */
    DbgBreakPoint();
}

BOOLEAN
NTAPI
CmpFileRead(IN PHHIVE Hive,
            IN ULONG FileType,
            IN ULONG FileOffset,
            IN PVOID Buffer,
            IN ULONG BufferLength)
{
    /* FIXME: TODO */
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
NTAPI
CmpFileWrite(IN PHHIVE Hive,
             IN ULONG FileType,
             IN ULONG FileOffset,
             IN PVOID Buffer,
             IN ULONG BufferLength)
{
    /* FIXME: TODO */
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
NTAPI
CmpFileSetSize(IN PHHIVE Hive,
               IN ULONG FileType,
               IN ULONG FileSize)
{
    /* FIXME: TODO */
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
NTAPI
CmpFileFlush(IN PHHIVE Hive,
             IN ULONG FileType)
{
    /* FIXME: TODO */
    DbgBreakPoint();
    return FALSE;
}

VOID
NTAPI
CmpInitializeSecurityCache(IN PCMHIVE Hive)
{
    ULONG i;
    PAGED_CODE();

    /* Initialize defaults */
    Hive->SecurityCache = NULL;
    Hive->SecurityCount = 0;
    Hive->SecurityHitHint = -1;
    Hive->SecurityCacheSize = 0;

    /* Loop the lists */
    for (i = 0; i < CMP_SECURITY_HASH_LISTS; i++)
    {
        /* Initialize this list */
        InitializeListHead(&Hive->SecurityHash[i]);
    }
}

VOID
NTAPI
CmpInitializeHiveViewList(IN PCMHIVE Hive)
{
    PAGED_CODE();

    /* Initialize the lists */
    InitializeListHead(&Hive->PinViewListHead);
    InitializeListHead(&Hive->LRUViewListHead);

    /* Setup defaults */
    Hive->MappedViews = 0;
    Hive->PinnedViews = 0;
    Hive->UseCount = 0;
}

NTSTATUS
NTAPI
CmpInitializeHive(OUT PCMHIVE *CmHive,
                  IN ULONG Operation,
                  IN ULONG Flags,
                  IN ULONG FileType,
                  IN PVOID HiveData,
                  IN HANDLE Primary,
                  IN HANDLE Alternate,
                  IN HANDLE Log,
                  IN HANDLE External,
                  IN PUNICODE_STRING FileName)
{
    PCMHIVE Hive;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION FileSizeInformation;
    NTSTATUS Status;
    ULONG Cluster;

    /*
     * The following are invalid:
     * An alternate hive that's also a log file.
     * An external hive that is also internal.
     * An alternate hive or a log hive that's not a primary hive too.
     * A volatile hive that's linked to permanent storage.
     * An in-memory initailization without hive data.
     * A log hive or alternative hive that's not linked to a correct file type.
     */
    if ((Alternate && Log) ||
        (External && (Primary || Alternate || Log)) ||
        (Alternate && !Primary) ||
        (Log && !Primary) ||
        ((Flags & HIVE_VOLATILE) &&
         (Alternate || Primary || External || Log)) ||
        ((Operation == HINIT_MEMORY) && (!HiveData)) ||
        (Log && (FileType != HFILE_TYPE_LOG)) ||
        (Alternate && (FileType != HFILE_TYPE_ALTERNATE)))
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

    /* Allocate a hive */
    Hive = ExAllocatePoolWithTag(PagedPool,
                                 sizeof(CMHIVE),
                                 TAG_CM);
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
    Hive->FileHandles[HFILE_TYPE_ALTERNATE] = Alternate;
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

    /* Initialize the hive */
    Status = HvInitialize(&Hive->Hive,
                          Operation,
                          Flags,
                          FileType,
                          (ULONG_PTR)HiveData,
                          CmpAllocate,
                          CmpFree,
                          CmpFileRead,
                          CmpFileWrite,
                          CmpFileSetSize,
                          CmpFileFlush,
                          Cluster,
                          FileName);
    if (NT_SUCCESS(Status))
    {
        /* Free all alocations */
        ExFreePool(Hive->ViewLock);
        ExFreePool(Hive->FlusherLock);
        ExFreePool(Hive);
        return Status;
    }

    /* Check if we should verify the registry */
    if ((Operation == HINIT_FILE) ||
        (Operation == HINIT_MEMORY) ||
        (Operation == HINIT_MEMORY_INPLACE) ||
        (Operation == HINIT_MAPFILE))
    {
        /* Verify integrity */
        if (CmCheckRegistry(Hive, TRUE))
        {
            /* Free all alocations */
            ExFreePool(Hive->ViewLock);
            ExFreePool(Hive->FlusherLock);
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
    *CmHive = Hive;
    return STATUS_SUCCESS;
}

