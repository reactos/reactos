/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/cminit.c
 * PURPOSE:         Initialization and generic utility routines
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

PEPROCESS CmpSystemProcess;
PCMHIVE CmpMasterHive;
HANDLE CmpRegistryRootHandle;

KGUARDED_MUTEX CmpSelfHealQueueLock;
LIST_ENTRY CmpSelfHealQueueListHead;

EX_PUSH_LOCK CmpLoadHiveLock;

UNICODE_STRING CmpSystemStartOptions;

ULONG CmpCallBackCount;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpCreateRootNode(IN PHHIVE Hive,
                  IN PWCHAR Name,
                  OUT HCELL_INDEX *Index)
{
    UNICODE_STRING KeyName;
    PCELL_DATA Cell;
    LARGE_INTEGER SystemTime;
    PAGED_CODE();

    /* Initialize the node name and allocate it */
    RtlInitUnicodeString(&KeyName, Name);
    *Index = HvAllocateCell(Hive,
                            FIELD_OFFSET(CM_KEY_NODE, Name) +
                            CmpNameSize(Hive, &KeyName),
                            HvStable); // FIXME: , -1);
    if (*Index == HCELL_NIL) return STATUS_UNSUCCESSFUL;

    /* Set the cell index and get the data */
    Hive->HiveHeader->RootCell = *Index;
    Cell = HvGetCell(Hive, *Index);

    /* Fill out the cell */
    Cell->u.KeyNode.Signature = CM_KEY_NODE_SIGNATURE;
    Cell->u.KeyNode.Flags = KEY_HIVE_ENTRY | KEY_NO_DELETE;
    KeQuerySystemTime(&SystemTime);
    Cell->u.KeyNode.LastWriteTime = SystemTime;
    Cell->u.KeyNode.Parent = HCELL_NIL;
    Cell->u.KeyNode.SubKeyCounts[HvStable] = 0;
    Cell->u.KeyNode.SubKeyCounts[HvVolatile] = 0;
    Cell->u.KeyNode.SubKeyLists[HvStable] = 0;
    Cell->u.KeyNode.SubKeyLists[HvVolatile] = 0;
    Cell->u.KeyNode.ValueList.Count = 0;
    Cell->u.KeyNode.ValueList.List = HCELL_NIL;
    Cell->u.KeyNode.Class = HCELL_NIL;
    Cell->u.KeyNode.MaxValueDataLen = 0;
    Cell->u.KeyNode.MaxNameLen = 0;
    Cell->u.KeyNode.MaxValueNameLen = 0;
    Cell->u.KeyNode.MaxClassLen = 0;

    /* Copy the name (this will also set the length) */
    Cell->u.KeyNode.NameLength = CmpCopyName(Hive,
                                             Cell->u.KeyNode.Name,
                                             &KeyName);

    /* Check if the name was compressed */
    if (Cell->u.KeyNode.NameLength < KeyName.Length)
    {
        /* Set the flag */
        Cell->u.KeyNode.Flags |= KEY_COMP_NAME;
    }

    /* Return success */
    HvReleaseCell(Hive, *Index);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpCreateRootKey(VOID)
{
    NTSTATUS Status;
    HCELL_INDEX RootCell;
    PCM_KEY_BODY Key;
    PVOID KeyBody;
    HANDLE KeyHandle;
    PCM_KEY_CONTROL_BLOCK RootKcb;
    UNICODE_STRING RootName = RTL_CONSTANT_STRING(L"\\REGISTRY");
    OBJECT_ATTRIBUTES ObjectAttributes;
    PAGED_CODE();

    /* Create the root node */
    Status = CmpCreateRootNode(&CmpMasterHive->Hive, L"REGISTRY", &RootCell);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the object */
    InitializeObjectAttributes(&ObjectAttributes,
                               &RootName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ObCreateObject(KernelMode,
                            CmpKeyObjectType,
                            &ObjectAttributes,
                            UserMode,
                            NULL,
                            sizeof(CM_KEY_BODY),
                            0,
                            0,
                            (PVOID*)&Key);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the KCB */
    RootKcb = CmpCreateKcb(&CmpMasterHive->Hive,
                           RootCell,
                           HvGetCell(&CmpMasterHive->Hive, RootCell),
                           NULL,
                           FALSE,
                           &RootName);
    if (!RootKcb) return STATUS_UNSUCCESSFUL;

    /* Setup the key */
    Key->Type = TAG('k', 'y', '0', '2');
    Key->KeyControlBlock = RootKcb;
    Key->NotifyBlock = NULL;
    Key->ProcessID = PsGetCurrentProcess();

    /* Enlist it */
    CmpEnlistKeyWithKcb(Key, RootKcb);

    /* Insert the key */
    Status = ObInsertObject(Key,
                            NULL,
                            0,
                            0,
                            NULL,
                            &CmpRegistryRootHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Now reference it as a keep alive and return the status */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       KEY_READ,
                                       NULL,
                                       KernelMode,
                                       &KeyBody,
                                       NULL);
    return Status;
}

VOID
NTAPI
CmpInitializeCallbacks(VOID)
{
    ULONG i;
    PAGED_CODE();

    /* Reset counter */
    CmpCallBackCount = 0;

    /* Loop all the callbacks */
    for (i = 0; i < CMP_MAX_CALLBACKS; i++)
    {
        /* Initialize this one */
        ExInitializeCallBack(&CmpCallBackVector[i]);
    }
}

VOID
NTAPI
CmpSetSystemBootValues(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PCHAR CommandLine;
    PCHAR SystemBootDevice;
    ULONG i = 0;
    ASSERT(LoaderBlock != NULL);

    /* Setup attributes for loader options */
    RtlInitUnicodeString(&KeyName,
                         L"\\REGISTRY\\MACHINE\\SYSTEM\\CurrentControlSet\\"
                         L"Control");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&KeyHandle, KEY_WRITE, &ObjectAttributes);
    if (NT_SUCCESS(Status))
    {
        /* Key opened, now write to the key */
        RtlInitUnicodeString(&KeyName, L"SystemStartOptions");
        NtSetValueKey(KeyHandle,
                      &KeyName,
                      0,
                      REG_SZ,
                      CmpSystemStartOptions.Buffer,
                      CmpSystemStartOptions.Length);
    }

    /* Free the options now */
    ExFreePool(CmpSystemStartOptions.Buffer);

    /* Setup value name for system boot device */
    RtlInitUnicodeString(&KeyName, L"SystemBootDevice");
    RtlCreateUnicodeStringFromAsciiz(&ValueName, LoaderBlock->NtBootPathName);
    NtSetValueKey(KeyHandle,
                  &KeyName,
                  0,
                  REG_SZ,
                  ValueName.Buffer,
                  ValueName.Length);

    /* Free the buffers */
    RtlFreeUnicodeString(&ValueName);
    ExFreePool(SystemBootDevice);

    /* Close the key and return */
    NtClose(KeyHandle);
}

VOID
NTAPI
CmInitializeRegistry(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Initialize the hive list and lock */
    InitializeListHead(&CmpHiveListHead);
    ExInitializePushLock(&CmpHiveListHeadLock);
    ExInitializePushLock(&CmpLoadHiveLock);

    /* Initialize registry lock */
    ExInitializeResourceLite(&CmpRegistryLock);

    /* Initialize the cache */
    CmpInitializeCache();

    /* Initialize allocation and delayed dereferencing */
    CmpInitializeCmAllocations();
    CmpInitializeKcbDelayedDeref();

    /* Initialize callbacks */
    CmpInitializeCallbacks();

    /* Initialize self healing */
    KeInitializeGuardedMutex(&CmpSelfHealQueueLock);
    InitializeListHead(&CmpSelfHealQueueListHead);

    /* Save the current process and lock the registry */
    CmpSystemProcess = PsGetCurrentProcess();
    CmpLockRegistryExclusive();

    /*  Initialize the Key object type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Key");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(CM_KEY_BODY);
    ObjectTypeInitializer.GenericMapping = NULL;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.ValidAccessMask = KEY_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.DeleteProcedure = CmpDeleteKeyObject;
    ObjectTypeInitializer.ParseProcedure = CmpParseKey;
    ObjectTypeInitializer.SecurityProcedure = CmpSecurityMethod;
    ObjectTypeInitializer.QueryNameProcedure = CmpQueryKeyName;
    ObjectTypeInitializer.CloseProcedure = CmpCloseKeyObject;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &CmpKeyObjectType);

    /* Create master hive */
    Status = CmpInitializeHive(&CmpMasterHive,
                               HINIT_CREATE,
                               HIVE_VOLATILE,
                               HFILE_TYPE_PRIMARY,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL);
    if (!NT_SUCCESS(Status)) KeBugCheck(0);

    /* Create the \REGISTRY key */
    Status = CmpCreateRootKey();
    if (!NT_SUCCESS(Status)) KeBugCheck(0);

    /* Create '\Registry\Machine' key. */
    RtlInitUnicodeString(&KeyName, L"\\REGISTRY\\MACHINE");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status)) KeBugCheck(0);
    NtClose(KeyHandle);

    /* Create '\Registry\User' key. */
    RtlInitUnicodeString(&KeyName, L"\\REGISTRY\\USER");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status)) KeBugCheck(0);
    NtClose(KeyHandle);

    /* Initialize and load the system hive */
    Status = CmpInitializeSystemHive(LoaderBlock);
    if (!NT_SUCCESS(Status)) KeBugCheck(0);

    /* Create the CCS symbolic link */
    Status = CmpCreateCurrentControlSetLink(LoaderBlock);
    if (!NT_SUCCESS(Status)) KeBugCheck(0);

    /* Load the hardware hive */
    Status = CmpInitializeHardwareHive(LoaderBlock);
    if (!NT_SUCCESS(Status)) KeBugCheck(0);

    /* Unlock the registry now */
    CmpUnlockRegistry();

    /* Set system boot values */
    CmpSetSystemBootValues(LoaderBlock);
}

#if 0
VOID
INIT_FUNCTION
STDCALL
CmInitHives(BOOLEAN SetupBoot)
{
    PCHAR BaseAddress;

    /* Load Registry Hives. This one can be missing. */
    if (CachedModules[SystemRegistry]) {
        BaseAddress = (PCHAR)CachedModules[SystemRegistry]->ModStart;
        CmImportSystemHive(BaseAddress,
                           CachedModules[SystemRegistry]->ModEnd - (ULONG_PTR)BaseAddress);
    }

    BaseAddress = (PCHAR)CachedModules[HardwareRegistry]->ModStart;
    CmImportHardwareHive(BaseAddress,
                         CachedModules[HardwareRegistry]->ModEnd - (ULONG_PTR)BaseAddress);


    /* Create dummy keys if no hardware hive was found */
    CmImportHardwareHive (NULL, 0);

    /* Initialize volatile registry settings */
    if (SetupBoot == FALSE) CmInit2((PCHAR)KeLoaderBlock.CommandLine);
}

  /*
   * Create a CurrentControlSet\Control\MiniNT key that is used
   * to detect WinPE/MiniNT systems.
   */
  if (MiniNT)
    {
      Status = RtlCreateRegistryKey(RTL_REGISTRY_CONTROL, L"MiniNT");
      if (!NT_SUCCESS(Status))
        KEBUGCHECK(CONFIG_INITIALIZATION_FAILED);
    }
#endif
