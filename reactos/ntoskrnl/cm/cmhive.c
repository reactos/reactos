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
CmpAllocate(IN SIZE_T Size,
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
            IN ULONGLONG FileOffset,
            IN PVOID Buffer,
            IN SIZE_T BufferLength)
{
    /* FIXME: TODO */
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
NTAPI
CmpFileWrite(IN PHHIVE Hive,
             IN ULONG FileType,
             IN ULONGLONG FileOffset,
             IN PVOID Buffer,
             IN SIZE_T BufferLength)
{
    /* FIXME: TODO */
    DbgBreakPoint();
    return FALSE;
}

BOOLEAN
NTAPI
CmpFileSetSize(IN PHHIVE Hive,
               IN ULONG FileType,
               IN ULONGLONG FileSize)
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
                          (ULONG_PTR)HiveData,
                          Cluster,
                          Flags,
                          FileType,
                          CmpAllocate,
                          CmpFree,
                          CmpFileRead,
                          CmpFileWrite,
                          CmpFileSetSize,
                          CmpFileFlush,
                          FileName);
    if (NT_SUCCESS(Status))
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

PSECURITY_DESCRIPTOR
NTAPI
CmpHiveRootSecurityDescriptor(VOID)
{
    SID_IDENTIFIER_AUTHORITY WorldAuthority = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID WorldSid;
    PSID RestrictedSid;
    PSID SystemSid;
    PSID AdminSid;
    NTSTATUS Status;
    ULONG AceSize, AclSize;
    PACL Acl, SdAcl;
    PACE_HEADER AceHeader;
    PSECURITY_DESCRIPTOR HiveSd;
    PAGED_CODE();

    /* Allocate all the SIDs */
    WorldSid  = ExAllocatePoolWithTag(PagedPool,
                                      RtlLengthRequiredSid(1),
                                      TAG_CM);
    RestrictedSid  = ExAllocatePoolWithTag(PagedPool,
                                           RtlLengthRequiredSid(1),
                                           TAG_CM);
    SystemSid = ExAllocatePoolWithTag(PagedPool,
                                      RtlLengthRequiredSid(1),
                                      TAG_CM);
    AdminSid  = ExAllocatePoolWithTag(PagedPool,
                                      RtlLengthRequiredSid(2),
                                      TAG_CM);

    /* Make sure that all were allocated */
    if (!(WorldSid) || !(RestrictedSid) || !(SystemSid) || !(AdminSid))
    {
        /* Fail */
        KeBugCheckEx(REGISTRY_ERROR, 10, 0, 0, 0);
    }

    /* Now initialize all the SIDs */
    Status = RtlInitializeSid(WorldSid, &WorldAuthority, 1);
    if (NT_SUCCESS(Status))
    {
        Status = RtlInitializeSid(RestrictedSid, &NtAuthority, 1);
    }
    if (NT_SUCCESS(Status))
    {
        Status = RtlInitializeSid(SystemSid, &NtAuthority, 1);
    }
    if (NT_SUCCESS(Status))
    {
        Status = RtlInitializeSid(AdminSid, &NtAuthority, 2);
    }
    if (NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 10, 1, 0, 0);

    /* Setup the sub-authority SIDs */
    *(RtlSubAuthoritySid(WorldSid, 0)) = SECURITY_WORLD_RID;
    *(RtlSubAuthoritySid(RestrictedSid, 0)) = SECURITY_RESTRICTED_CODE_RID;
    *(RtlSubAuthoritySid(SystemSid, 0)) = SECURITY_LOCAL_SYSTEM_RID;
    *(RtlSubAuthoritySid(AdminSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;
    *(RtlSubAuthoritySid(AdminSid, 1)) = DOMAIN_ALIAS_RID_ADMINS;

    /* Sanity checks */
    ASSERT(RtlValidSid(WorldSid));
    ASSERT(RtlValidSid(RestrictedSid));
    ASSERT(RtlValidSid(SystemSid));
    ASSERT(RtlValidSid(AdminSid));

    /* Calculate length of the ACE */
    AceSize = (SeLengthSid(WorldSid) -
               sizeof(ULONG) + sizeof(ACCESS_ALLOWED_ACE)) +
              (SeLengthSid(RestrictedSid) -
               sizeof(ULONG) + sizeof(ACCESS_ALLOWED_ACE))+
              (SeLengthSid(SystemSid) -
               sizeof(ULONG) + sizeof(ACCESS_ALLOWED_ACE)) +
              (SeLengthSid(AdminSid) -
               sizeof(ULONG) + sizeof(ACCESS_ALLOWED_ACE));

    /* Calculate the ACL length and allocate it */
    AclSize = AceSize + sizeof(ACL);
    Acl = ExAllocatePoolWithTag(PagedPool, AclSize, TAG_CM);
    if (!Acl) KeBugCheckEx(REGISTRY_ERROR, 10, 2, 0, 0);

    /* Create it */
    Status = RtlCreateAcl(Acl, AclSize, ACL_REVISION);
    if (!NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 10, 3, 0, 0);

    /* Add our ACEs */
    Status = RtlAddAccessAllowedAce(Acl,
                                    ACL_REVISION,
                                    KEY_ALL_ACCESS,
                                    SystemSid);
    if (NT_SUCCESS(Status))
    {
        Status = RtlAddAccessAllowedAce(Acl,
                                        ACL_REVISION,
                                        KEY_ALL_ACCESS,
                                        AdminSid);
    }
    if (NT_SUCCESS(Status))
    {
        Status = RtlAddAccessAllowedAce(Acl,
                                        ACL_REVISION,
                                        KEY_READ,
                                        WorldSid);
    }
    if (NT_SUCCESS(Status))
    {
        Status = RtlAddAccessAllowedAce(Acl,
                                        ACL_REVISION,
                                        KEY_READ,
                                        RestrictedSid);
    }
    if (!NT_SUCCESS(Status)) KeBugCheckEx(REGISTRY_ERROR, 10, 4, 0, 0);

    /* Get every ACE and turn on the inherit flag */
    Status = RtlGetAce(Acl,0,&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;
    Status = RtlGetAce(Acl,1,&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;
    Status = RtlGetAce(Acl,2,&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;
    Status = RtlGetAce(Acl,3,&AceHeader);
    ASSERT(NT_SUCCESS(Status));
    AceHeader->AceFlags |= CONTAINER_INHERIT_ACE;

    /* Allocate the SD */
    HiveSd = ExAllocatePoolWithTag(PagedPool,
                                   AclSize + sizeof(SECURITY_DESCRIPTOR),
                                   TAG_CM);
    if (!HiveSd) KeBugCheckEx(REGISTRY_ERROR, 10, 5, 0, 0);

    /* Copy the ACL into it */
    SdAcl = (PACL)((PISECURITY_DESCRIPTOR)HiveSd + 1);
    RtlMoveMemory(SdAcl, Acl, AclSize);

    /* Create the SD */
    Status = RtlCreateSecurityDescriptor(HiveSd, SECURITY_DESCRIPTOR_REVISION);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, free the descriptor and bugcheck */
        ExFreePool(HiveSd);
        KeBugCheckEx(REGISTRY_ERROR, 10, 6, 0, 0);
    }

    /* Set the DACL */
    Status = RtlSetDaclSecurityDescriptor(HiveSd, TRUE, SdAcl, FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, free the descriptor and bugcheck */
        ExFreePool(HiveSd);
        KeBugCheckEx(REGISTRY_ERROR, 10, 7, 0, 0);
    }

    /* Free all allocations */
    ExFreePool(WorldSid);
    ExFreePool(RestrictedSid);
    ExFreePool(SystemSid);
    ExFreePool(AdminSid);
    ExFreePool(Acl);

    /* Return the SD */
    return HiveSd;
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
        ParseContext.ChildHive.KeyCell = CmHive->Hive.HiveHeader->RootCell;
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
                                       &KeyBody,
                                       NULL);
    ASSERT(NT_SUCCESS(Status));

    /* Send a notification */
    CmpReportNotify(KeyBody->KeyControlBlock,
                    KeyBody->KeyControlBlock->KeyHive,
                    KeyBody->KeyControlBlock->KeyCell,
                    REG_NOTIFY_CHANGE_NAME);

    /* Dereference the key and close the handle */
    ObDereferenceObject((PVOID)KeyBody);
    ZwClose(KeyHandle);
    return STATUS_SUCCESS;
}
