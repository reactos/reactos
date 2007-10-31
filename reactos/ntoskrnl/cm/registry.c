/*
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/registry.c
 * PURPOSE:         Registry functions
 *
 * PROGRAMMERS:     Hartmut Birr
 *                  Alex Ionescu
 *                  Rex Jolliff
 *                  Eric Kohl
 *                  Matt Pyne
 *                  Jean Michault
 *                  Art Yerkes
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, CmInitSystem1)
#endif

/* GLOBALS ******************************************************************/

extern BOOLEAN ExpInTextModeSetup;

POBJECT_TYPE  CmpKeyObjectType = NULL;
PCMHIVE  CmiVolatileHive = NULL;

LIST_ENTRY CmpHiveListHead;

ERESOURCE CmpRegistryLock;

LIST_ENTRY CmiKeyObjectListHead;
LIST_ENTRY CmiConnectedHiveList;

volatile BOOLEAN CmiHiveSyncEnabled = FALSE;
volatile BOOLEAN CmiHiveSyncPending = FALSE;
KDPC CmiHiveSyncDpc;
KTIMER CmiHiveSyncTimer;

static VOID
NTAPI
CmiHiveSyncDpcRoutine(PKDPC Dpc,
                      PVOID DeferredContext,
                      PVOID SystemArgument1,
                      PVOID SystemArgument2);

extern LIST_ENTRY CmiCallbackHead;
extern FAST_MUTEX CmiCallbackLock;

PVOID
NTAPI
CmpRosGetHardwareHive(OUT PULONG Length)
{
    PLIST_ENTRY ListHead, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock = NULL;

    /* Loop the memory descriptors */
    ListHead = &KeLoaderBlock->MemoryDescriptorListHead;
    NextEntry = ListHead->Flink;
    while (NextEntry != ListHead)
    {
        /* Get the current block */
        MdBlock = CONTAINING_RECORD(NextEntry,
                                    MEMORY_ALLOCATION_DESCRIPTOR,
                                    ListEntry);

        /* Check if this is an registry block */
        if (MdBlock->MemoryType == LoaderRegistryData)
        {
            /* Check if it's not the SYSTEM hive that we already initialized */
            if ((MdBlock->BasePage) !=
                (((ULONG_PTR)KeLoaderBlock->RegistryBase &~ KSEG0_BASE) >> PAGE_SHIFT))
            {
                /* Hardware hive break out */
                break;
            }
        }

        /* Go to the next block */
        NextEntry = MdBlock->ListEntry.Flink;
    }

    /* We need a hardware hive */
    ASSERT(MdBlock);
    *Length = MdBlock->PageCount << PAGE_SHIFT;
    return (PVOID)((MdBlock->BasePage << PAGE_SHIFT) | KSEG0_BASE);
}

/* Precondition: Must not hold the hive lock CmpRegistryLock */
VOID
NTAPI
EnlistKeyBodyWithKeyObject(IN PKEY_OBJECT KeyObject,
                           IN ULONG Flags)
{
    /* Acquire hive lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);

    /* Insert it into the global list (we don't have KCBs here) */
    InsertTailList(&CmiKeyObjectListHead, &KeyObject->KeyBodyList);

    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();
}

NTSTATUS
NTAPI
CmpLinkHiveToMaster(IN PUNICODE_STRING LinkName,
                    IN HANDLE RootDirectory,
                    IN PCMHIVE RegistryHive,
                    IN BOOLEAN Allocate,
                    IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    OBJECT_ATTRIBUTES ObjectAttributes;

    /* Don't do anything if we don't actually have a hive */
    if (Allocate) return STATUS_SUCCESS;

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RootDirectory,
                               SecurityDescriptor);

    /* Connect the hive */
    return CmiConnectHive(&ObjectAttributes, RegistryHive);
}

NTSTATUS
CmiConnectHive(IN POBJECT_ATTRIBUTES KeyObjectAttributes,
               IN PCMHIVE RegistryHive)
{
    UNICODE_STRING RemainingPath;
    PKEY_OBJECT ParentKey;
    PKEY_OBJECT NewKey;
    NTSTATUS Status;
    PWSTR SubName;
    UNICODE_STRING ObjectName;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;

    DPRINT("CmiConnectHive(%p, %p) called.\n",
        KeyObjectAttributes, RegistryHive);

    /* Capture all the info */
    DPRINT("Capturing Create Info\n");
    Status = ObpCaptureObjectAttributes(KeyObjectAttributes,
                                        KernelMode,
                                        FALSE,
                                        &ObjectCreateInfo,
                                        &ObjectName);

    if (!NT_SUCCESS(Status))
    {
        DPRINT("ObpCaptureObjectAttributes() failed (Status %lx)\n", Status);
        return Status;
    }

    Status = CmFindObject(&ObjectCreateInfo,
                          &ObjectName,
                          (PVOID*)&ParentKey,
                          &RemainingPath,
                          CmpKeyObjectType,
                          NULL,
                          NULL);
    /* Yields a new reference */
    ObpReleaseCapturedAttributes(&ObjectCreateInfo);

    if (ObjectName.Buffer) ObpFreeObjectNameBuffer(&ObjectName);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    DPRINT ("RemainingPath %wZ\n", &RemainingPath);

    if ((RemainingPath.Buffer == NULL) || (RemainingPath.Buffer[0] == 0))
    {
        ObDereferenceObject (ParentKey);
        RtlFreeUnicodeString(&RemainingPath);
        return STATUS_OBJECT_NAME_COLLISION;
    }

    /* Ignore leading backslash */
    SubName = RemainingPath.Buffer;
    if (*SubName == L'\\')
        SubName++;

    /* If RemainingPath contains \ we must return error
       because CmiConnectHive() can not create trees */
    if (wcschr (SubName, L'\\') != NULL)
    {
        ObDereferenceObject (ParentKey);
        RtlFreeUnicodeString(&RemainingPath);
        return STATUS_OBJECT_NAME_NOT_FOUND;
    }

    DPRINT("RemainingPath %wZ  ParentKey %p\n",
        &RemainingPath, ParentKey);

    DPRINT ("SubName %S\n", SubName);

    /* Create the key */
    Status = CmpDoCreate(ParentKey->KeyControlBlock->KeyHive,
                         ParentKey->KeyControlBlock->KeyCell,
                         NULL,
                         &RemainingPath,
                         KernelMode,
                         NULL,
                         REG_OPTION_VOLATILE,
                         ParentKey->KeyControlBlock,
                         NULL,
                         (PVOID*)&NewKey);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiAddSubKey() failed (Status %lx)\n", Status);
        ObDereferenceObject (NewKey);
        ObDereferenceObject (ParentKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewKey->KeyControlBlock->KeyCell = RegistryHive->Hive.BaseBlock->RootCell;
    NewKey->KeyControlBlock->KeyHive = &RegistryHive->Hive;

    Status = RtlpCreateUnicodeString(&NewKey->Name,
        SubName, NonPagedPool);
    RtlFreeUnicodeString(&RemainingPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlpCreateUnicodeString() failed (Status %lx)\n", Status);
        ObDereferenceObject (NewKey);
        ObDereferenceObject (ParentKey);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Free the create information */
    ObpFreeAndReleaseCapturedAttributes(OBJECT_TO_OBJECT_HEADER(NewKey)->ObjectCreateInfo);
    OBJECT_TO_OBJECT_HEADER(NewKey)->ObjectCreateInfo = NULL;

    /* FN1 */
    ObReferenceObject (NewKey);

    CmiAddKeyToList (ParentKey, NewKey);
    ObDereferenceObject (ParentKey);

    VERIFY_KEY_OBJECT(NewKey);

    /* We're holding a pointer to the parent key ..  We must keep it
     * referenced */
    /* Note: Do not dereference NewKey here! */

    return STATUS_SUCCESS;
}

static NTSTATUS
CmiInitControlSetLink (VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING ControlSetKeyName = RTL_CONSTANT_STRING(
        L"\\Registry\\Machine\\SYSTEM\\ControlSet001");
    UNICODE_STRING ControlSetLinkName =  RTL_CONSTANT_STRING(
        L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet");
    UNICODE_STRING ControlSetValueName = RTL_CONSTANT_STRING(L"SymbolicLinkValue");
    HANDLE KeyHandle;
    NTSTATUS Status;

    /* Create 'ControlSet001' key */
    InitializeObjectAttributes (&ObjectAttributes,
                                &ControlSetKeyName,
                                OBJ_CASE_INSENSITIVE,
                                NULL,
                                NULL);
    Status = ZwCreateKey (&KeyHandle,
                          KEY_ALL_ACCESS,
                          &ObjectAttributes,
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("ZwCreateKey() failed (Status %lx)\n", Status);
        return Status;
    }
    ZwClose (KeyHandle);

    /* Link 'CurrentControlSet' to 'ControlSet001' key */
    InitializeObjectAttributes (&ObjectAttributes,
                                &ControlSetLinkName,
                                OBJ_CASE_INSENSITIVE | OBJ_OPENIF | OBJ_OPENLINK,
                                NULL,
                                NULL);
    Status = ZwCreateKey (&KeyHandle,
                          KEY_ALL_ACCESS | KEY_CREATE_LINK,
                          &ObjectAttributes,
                          0,
                          NULL,
                          REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
                          NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("ZwCreateKey() failed (Status %lx)\n", Status);
        return Status;
    }

    Status = ZwSetValueKey (KeyHandle,
                            &ControlSetValueName,
                            0,
                            REG_LINK,
                            (PVOID)ControlSetKeyName.Buffer,
                            ControlSetKeyName.Length);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1 ("ZwSetValueKey() failed (Status %lx)\n", Status);
    }
    ZwClose (KeyHandle);

    return STATUS_SUCCESS;
}

NTSTATUS
CmiInitHives(BOOLEAN SetupBoot)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING FileName;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"InstallPath");
    HANDLE KeyHandle;

    NTSTATUS Status;

    WCHAR ConfigPath[MAX_PATH];

    ULONG BufferSize;
    ULONG ResultSize;
    PWSTR EndPtr;

    DPRINT("CmiInitHives() called\n");

    if (SetupBoot == TRUE)
    {
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status =  ZwOpenKey(&KeyHandle,
                            KEY_ALL_ACCESS,
                            &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ZwOpenKey() failed (Status %lx)\n", Status);
            return(Status);
        }

        BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 4096;
        ValueInfo = ExAllocatePool(PagedPool, BufferSize);
        if (ValueInfo == NULL)
        {
            ZwClose(KeyHandle);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 BufferSize,
                                 &ResultSize);
        ZwClose(KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            ExFreePool(ValueInfo);
            return(Status);
        }

        RtlCopyMemory(ConfigPath,
                      ValueInfo->Data,
                      ValueInfo->DataLength);
        ConfigPath[ValueInfo->DataLength / sizeof(WCHAR)] = (WCHAR)0;
        ExFreePool(ValueInfo);
    }
    else
    {
        wcscpy(ConfigPath, L"\\SystemRoot");
    }
    wcscat(ConfigPath, L"\\system32\\config");

    DPRINT("ConfigPath: %S\n", ConfigPath);

    EndPtr = ConfigPath + wcslen(ConfigPath);

    /* FIXME: Save boot log */

    /* Connect the SYSTEM hive only if it has been created */
    if (SetupBoot == TRUE)
    {
        wcscpy(EndPtr, REG_SYSTEM_FILE_NAME);
        DPRINT ("ConfigPath: %S\n", ConfigPath);

        RtlInitUnicodeString (&KeyName,
                              REG_SYSTEM_KEY_NAME);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        RtlInitUnicodeString (&FileName,
                              ConfigPath);
        Status = CmiLoadHive (&ObjectAttributes,
                              &FileName,
                              0);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1 ("CmiLoadHive() failed (Status %lx)\n", Status);
            return Status;
        }

        Status = CmiInitControlSetLink ();
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("CmiInitControlSetLink() failed (Status %lx)\n", Status);
            return(Status);
        }
    }

    /* Connect the SOFTWARE hive */
    wcscpy(EndPtr, REG_SOFTWARE_FILE_NAME);
    RtlInitUnicodeString (&FileName,
                          ConfigPath);
    DPRINT ("ConfigPath: %S\n", ConfigPath);

    RtlInitUnicodeString (&KeyName,
                          REG_SOFTWARE_KEY_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = CmiLoadHive (&ObjectAttributes,
                          &FileName,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
        return(Status);
    }

    /* Connect the SAM hive */
    wcscpy(EndPtr, REG_SAM_FILE_NAME);
    RtlInitUnicodeString (&FileName,
                          ConfigPath);
    DPRINT ("ConfigPath: %S\n", ConfigPath);

    RtlInitUnicodeString (&KeyName,
                          REG_SAM_KEY_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = CmiLoadHive (&ObjectAttributes,
                          &FileName,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
        return(Status);
    }

    /* Connect the SECURITY hive */
    wcscpy(EndPtr, REG_SEC_FILE_NAME);
    RtlInitUnicodeString (&FileName,
                          ConfigPath);
    DPRINT ("ConfigPath: %S\n", ConfigPath);

    RtlInitUnicodeString (&KeyName,
                          REG_SEC_KEY_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = CmiLoadHive (&ObjectAttributes,
                          &FileName,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
        return(Status);
    }

    /* Connect the DEFAULT hive */
    wcscpy(EndPtr, REG_DEFAULT_USER_FILE_NAME);
    RtlInitUnicodeString (&FileName,
                          ConfigPath);
    DPRINT ("ConfigPath: %S\n", ConfigPath);

    RtlInitUnicodeString (&KeyName,
                          REG_DEFAULT_USER_KEY_NAME);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = CmiLoadHive (&ObjectAttributes,
                          &FileName,
                          0);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CmiInitializeHive() failed (Status %lx)\n", Status);
        return(Status);
    }

    //CmiCheckRegistry(TRUE);

    /* Start automatic hive synchronization */
    KeInitializeDpc(&CmiHiveSyncDpc,
                    CmiHiveSyncDpcRoutine,
                    NULL);
    KeInitializeTimer(&CmiHiveSyncTimer);
    CmiHiveSyncEnabled = TRUE;

    DPRINT("CmiInitHives() done\n");

    return(STATUS_SUCCESS);
}

VOID
CmShutdownRegistry(VOID)
{
    PCMHIVE Hive;
    PLIST_ENTRY Entry;

    DPRINT("CmShutdownRegistry() called\n");

    /* Stop automatic hive synchronization */
    CmiHiveSyncEnabled = FALSE;

    /* Cancel pending hive synchronization */
    if (CmiHiveSyncPending == TRUE)
    {
        KeCancelTimer(&CmiHiveSyncTimer);
        CmiHiveSyncPending = FALSE;
    }

    /* Lock the hive list */
    ExAcquirePushLockExclusive(&CmpHiveListHeadLock);

    Entry = CmpHiveListHead.Flink;
    while (Entry != &CmpHiveListHead)
    {
        Hive = CONTAINING_RECORD(Entry, CMHIVE, HiveList);

        if (!(IsNoFileHive(Hive) || IsNoSynchHive(Hive)))
        {
            /* Flush non-volatile hive */
            CmiFlushRegistryHive(Hive);
        }

        Entry = Entry->Flink;
    }

    /* Release the lock */
    ExReleasePushLock(&CmpHiveListHeadLock);

    DPRINT("CmShutdownRegistry() done\n");
}

VOID
NTAPI
CmiHiveSyncRoutine(PVOID DeferredContext)
{
    PCMHIVE Hive;
    PLIST_ENTRY Entry;

    DPRINT("CmiHiveSyncRoutine() called\n");

    CmiHiveSyncPending = FALSE;

    /* Lock the hive list */
    ExAcquirePushLockExclusive(&CmpHiveListHeadLock);

    Entry = CmpHiveListHead.Flink;
    while (Entry != &CmpHiveListHead)
    {
        Hive = CONTAINING_RECORD(Entry, CMHIVE, HiveList);

        if (!(IsNoFileHive(Hive) || IsNoSynchHive(Hive)))
        {
            /* Flush non-volatile hive */
            CmiFlushRegistryHive(Hive);
        }

        Entry = Entry->Flink;
    }

    /* Release the lock */
    ExReleasePushLock(&CmpHiveListHeadLock);

    DPRINT("DeferredContext 0x%p\n", DeferredContext);
    ExFreePool(DeferredContext);

    DPRINT("CmiHiveSyncRoutine() done\n");
}

static VOID
NTAPI
CmiHiveSyncDpcRoutine(PKDPC Dpc,
                      PVOID DeferredContext,
                      PVOID SystemArgument1,
                      PVOID SystemArgument2)
{
    PWORK_QUEUE_ITEM WorkQueueItem;

    WorkQueueItem = ExAllocatePool(NonPagedPool,
                                   sizeof(WORK_QUEUE_ITEM));
    if (WorkQueueItem == NULL)
    {
        DPRINT1("Failed to allocate work item\n");
        return;
    }

    ExInitializeWorkItem(WorkQueueItem,
                         CmiHiveSyncRoutine,
                         WorkQueueItem);

    DPRINT("DeferredContext 0x%p\n", WorkQueueItem);
    ExQueueWorkItem(WorkQueueItem,
                    CriticalWorkQueue);
}

VOID
CmiSyncHives(VOID)
{
    LARGE_INTEGER Timeout;

    DPRINT("CmiSyncHives() called\n");

  if (CmiHiveSyncEnabled == FALSE ||
        CmiHiveSyncPending == TRUE)
        return;

    CmiHiveSyncPending = TRUE;

    Timeout.QuadPart = (LONGLONG)-50000000;
    KeSetTimer(&CmiHiveSyncTimer,
               Timeout,
               &CmiHiveSyncDpc);
}

/* EOF */
