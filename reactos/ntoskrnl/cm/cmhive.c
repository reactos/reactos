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
#include "cm.h"

/* GLOBALS *******************************************************************/

EX_PUSH_LOCK CmpHiveListHeadLock;
LIST_ENTRY CmpHiveListHead;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpInitHiveViewList(IN PCMHIVE Hive)
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
                  IN PVOID HiveData OPTIONAL,
                  IN HANDLE Primary,
                  IN HANDLE Log,
                  IN HANDLE External,
                  IN PCUNICODE_STRING FileName,
                  IN ULONG CheckFlags)
{
    PCMHIVE Hive;
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_FS_SIZE_INFORMATION FileSizeInformation;
    NTSTATUS Status;
    ULONG Cluster;
    FILE_STANDARD_INFORMATION FileInformation;

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
        ((Flags & HIVE_VOLATILE) && ((Primary) || (External) || (Log))) ||
        ((Operation == HINIT_MEMORY) && (!HiveData)) ||
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
    Hive->FileHandles[HFILE_TYPE_LOG] = Log;
    Hive->FileHandles[HFILE_TYPE_EXTERNAL] = External;

    /* Initailize the guarded mutex */
    KeInitializeGuardedMutex(Hive->ViewLock);
    Hive->ViewLockOwner = NULL;

    /* Initialize the flush lock */
    ExInitializeResourceLite(Hive->FlusherLock);

    /* Setup hive locks */
    ExInitializePushLock((PULONG_PTR)&Hive->HiveLock);
    Hive->HiveLockOwner = NULL;
    ExInitializePushLock((PULONG_PTR)&Hive->WriterLock);
    Hive->WriterLockOwner = NULL;
    ExInitializePushLock((PULONG_PTR)&Hive->SecurityLock);
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
    
    /* REACTOS: Check how large the file is */
    ZwQueryInformationFile(Primary,
                           &IoStatusBlock,
                           &FileInformation,
                           sizeof(FileInformation),
                           FileStandardInformation);
    Cluster = FileInformation.EndOfFile.LowPart;

    /* Initialize the hive */
    Status = HvInitialize(&Hive->Hive,
                          Operation,
                          FileType,
                          Flags,
                          HiveData,
                          CmpAllocate,
                          CmpFree,
                          CmpFileSetSize,
                          CmpFileWrite,
                          CmpFileRead,
                          CmpFileFlush,
                          Cluster,
                          (PUNICODE_STRING)FileName);
    if (!NT_SUCCESS(Status))
    {
        /* Free all allocations */
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

NTSTATUS
NTAPI
CmpLinkHiveToMaster(IN PUNICODE_STRING LinkName,
                    IN HANDLE RootDirectory,
                    IN PCMHIVE CmHive,
                    IN BOOLEAN Allocate,
                    IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    CM_PARSE_CONTEXT ParseContext;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PCM_KEY_BODY KeyBody;
    PAGED_CODE();

    /* Fill out the parse context */
    ParseContext.TitleIndex = 0;
    ParseContext.Class.Length = 0;
    ParseContext.Class.MaximumLength = 0;
    ParseContext.Class.Buffer = NULL;
    ParseContext.CreateOptions = 0;
    ParseContext.CreateLink = TRUE;
    ParseContext.Flag2 = TRUE;
    ParseContext.PostActions = 0;
    ParseContext.ChildHive.KeyHive = &CmHive->Hive;

    /* Check if we're allocating */
    if (Allocate)
    {
        /* Set no child */
        ParseContext.ChildHive.KeyCell = HCELL_NIL;
    }
    else
    {
        /* Otherwise, set the root cell */
        ParseContext.ChildHive.KeyCell = CmHive->Hive.BaseBlock->RootCell;
    }

    /* Open the key */
    InitializeObjectAttributes(&ObjectAttributes,
                               LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RootDirectory,
                               SecurityDescriptor);
    Status = ObOpenObjectByName(&ObjectAttributes,
                                CmpKeyObjectType,
                                KernelMode,
                                NULL,
                                KEY_READ | KEY_WRITE,
                                &ParseContext,
                                &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Reference the key body */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       0,
                                       CmpKeyObjectType,
                                       KernelMode,
                                       (PVOID*)&KeyBody,
                                       NULL);
    ASSERT(NT_SUCCESS(Status));

    /* Send a notification */
#define REG_NOTIFY_CHANGE_NAME 1 /* DDK! */
    CmpReportNotify(KeyBody->KeyControlBlock,
                    KeyBody->KeyControlBlock->KeyHive,
                    KeyBody->KeyControlBlock->KeyCell,
                    REG_NOTIFY_CHANGE_NAME);

    /* Dereference the key and close the handle */
    ObDereferenceObject((PVOID)KeyBody);
    ZwClose(KeyHandle);
    return STATUS_SUCCESS;
}
