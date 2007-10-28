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
#include "cm.h"

UNICODE_STRING CmRegistryMachineSystemName =
    RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM");
UNICODE_STRING CmpSystemFileName =
    RTL_CONSTANT_STRING(L"SYSTEM");

BOOLEAN CmpMiniNTBoot;
BOOLEAN CmpShareSystemHives;
ULONG CmpBootType;
BOOLEAN CmpSpecialBootCondition;
BOOLEAN CmpFlushOnLockRelease;

PEPROCESS CmpSystemProcess;
PCMHIVE CmpMasterHive;
HANDLE CmpRegistryRootHandle;

BOOLEAN CmSelfHeal, CmpSelfHeal;
KGUARDED_MUTEX CmpSelfHealQueueLock;
LIST_ENTRY CmpSelfHealQueueListHead;

HIVE_LIST_ENTRY CmpMachineHiveList[];

EX_PUSH_LOCK CmpLoadHiveLock;

UNICODE_STRING CmpSystemStartOptions;
UNICODE_STRING CmpLoadOptions;

ULONG CmpCallBackCount;

GENERIC_MAPPING CmpKeyMapping =
{
    KEY_READ,
    KEY_WRITE,
    KEY_EXECUTE,
    KEY_ALL_ACCESS
};

EX_CALLBACK CmpCallBackVector[CMP_MAX_CALLBACKS];

PVOID NTAPI CmpRosGetHardwareHive(OUT PULONG Length);

/* FUNCTIONS *****************************************************************/

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

NTSTATUS
NTAPI
CmpCreateObjectTypes(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    GENERIC_MAPPING CmpKeyMapping = {KEY_READ,
        KEY_WRITE,
        KEY_EXECUTE,
        KEY_ALL_ACCESS};
    PAGED_CODE();
    
    /* Initialize the Key object type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Key");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(CM_KEY_BODY);
    ObjectTypeInitializer.GenericMapping = CmpKeyMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.ValidAccessMask = KEY_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.DeleteProcedure = CmpDeleteKeyObject;
    ObjectTypeInitializer.ParseProcedure = CmpParseKey;
    ObjectTypeInitializer.SecurityProcedure = CmpSecurityMethod;
    ObjectTypeInitializer.QueryNameProcedure = CmpQueryKeyName;
    //ObjectTypeInitializer.CloseProcedure = CmpCloseKeyObject;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    
    /* Create it */
    return ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &CmpKeyObjectType);
}

BOOLEAN
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
                            Stable,
                            HCELL_NIL);
    if (*Index == HCELL_NIL) return FALSE;

    /* Set the cell index and get the data */
    Hive->BaseBlock->RootCell = *Index;
    Cell = HvGetCell(Hive, *Index);

    /* Fill out the cell */
    Cell->u.KeyNode.Signature = (USHORT)CM_KEY_NODE_SIGNATURE;
    Cell->u.KeyNode.Flags = KEY_HIVE_ENTRY | KEY_NO_DELETE;
    KeQuerySystemTime(&SystemTime);
    Cell->u.KeyNode.LastWriteTime = SystemTime;
    Cell->u.KeyNode.Parent = HCELL_NIL;
    Cell->u.KeyNode.SubKeyCounts[Stable] = 0;
    Cell->u.KeyNode.SubKeyCounts[Volatile] = 0;
    Cell->u.KeyNode.SubKeyLists[Stable] = 0;
    Cell->u.KeyNode.SubKeyLists[Volatile] = 0;
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
    return TRUE;
}

BOOLEAN
NTAPI
CmpCreateRegistryRoot(VOID)
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PCM_KEY_BODY RootKey;
    HCELL_INDEX RootIndex;
    NTSTATUS Status;
    PCM_KEY_NODE KeyCell;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PCM_KEY_CONTROL_BLOCK Kcb;
    PAGED_CODE();

    /* Setup the root node */
    if (!CmpCreateRootNode(&CmpMasterHive->Hive, L"REGISTRY", &RootIndex))
    {
        /* We failed */
        DPRINT1("Fail\n");
        return FALSE;
    }

    /* Create '\Registry' key. */
    RtlInitUnicodeString(&KeyName, L"\\Registry");
    SecurityDescriptor = CmpHiveRootSecurityDescriptor();
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ObCreateObject(KernelMode,
                            CmpKeyObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(CM_KEY_BODY),
                            0,
                            0,
                            (PVOID*)&RootKey);
    ExFreePool(SecurityDescriptor);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Sanity check, and get the key cell */
    ASSERT((&CmpMasterHive->Hive)->ReleaseCellRoutine == NULL);
    KeyCell = (PCM_KEY_NODE)HvGetCell(&CmpMasterHive->Hive, RootIndex);
    if (!KeyCell) return FALSE;
    
    /* Create the KCB */
    RtlInitUnicodeString(&KeyName, L"Registry");
    Kcb = CmpCreateKeyControlBlock(&CmpMasterHive->Hive,
                                   RootIndex,
                                   KeyCell,
                                   NULL,
                                   0,
                                   &KeyName);
    if (!Kcb) return FALSE;

    /* Setup the key */
    RootKey->Type = TAG('k', 'v', '0', '2');
    RootKey->KeyControlBlock = Kcb;
    RootKey->NotifyBlock = NULL;
    RootKey->ProcessID = PsGetCurrentProcessId();  

    /* Enlist it */
    EnlistKeyBodyWithKCB(RootKey, 0);

    /* Insert the key */
    Status = ObInsertObject(RootKey,
                            NULL,
                            KEY_ALL_ACCESS,
                            0,
                            NULL,
                            &CmpRegistryRootHandle);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Now reference it as a keep alive and return the status */
    Status = ObReferenceObjectByHandle(CmpRegistryRootHandle,
                                       KEY_READ,
                                       NULL,
                                       KernelMode,
                                       (PVOID*)&RootKey,
                                       NULL);
    if (!NT_SUCCESS(Status)) return FALSE;
    
    /* Completely sucessful */
    return TRUE;
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

NTSTATUS
NTAPI
CmpSetSystemValues(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    ASSERT(LoaderBlock != NULL);
    if (ExpInTextModeSetup) return STATUS_SUCCESS;
    
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
    if (!NT_SUCCESS(Status)) goto Quickie;
    
    /* Key opened, now write to the key */
    RtlInitUnicodeString(&KeyName, L"SystemStartOptions");
    Status = NtSetValueKey(KeyHandle,
                           &KeyName,
                           0,
                           REG_SZ,
                           CmpLoadOptions.Buffer,
                           CmpLoadOptions.Length);
    if (!NT_SUCCESS(Status)) goto Quickie;
    
    /* Setup value name for system boot device */
    RtlInitUnicodeString(&KeyName, L"SystemBootDevice");
    RtlCreateUnicodeStringFromAsciiz(&ValueName, LoaderBlock->NtBootPathName);
    Status = NtSetValueKey(KeyHandle,
                           &KeyName,
                           0,
                           REG_SZ,
                           ValueName.Buffer,
                           ValueName.Length);
    
Quickie:
        /* Free the buffers */
        RtlFreeUnicodeString(&ValueName);
    
    /* Close the key and return */
    NtClose(KeyHandle);
    
    /* Return the status */
    return Status;
}

BOOLEAN
NTAPI
CmpInitializeSystemHive(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCMHIVE SystemHive;
    PVOID HiveImageBase;
    BOOLEAN Allocate = FALSE;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    NTSTATUS Status;
    STRING  TempString;
    PAGED_CODE();

    /* Create ANSI_STRING from the loader options */
    RtlInitAnsiString(&TempString, LoaderBlock->LoadOptions);

    /* Setup UNICODE_STRING */
    CmpLoadOptions.Length = 0;
    CmpLoadOptions.MaximumLength = TempString.Length * sizeof(WCHAR) +
                                   sizeof(UNICODE_NULL);
    CmpLoadOptions.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                  CmpLoadOptions.MaximumLength,
                                                  TAG_CM);
    if (!CmpLoadOptions.Buffer)
    {
        /* Fail */
        KeBugCheckEx(CONFIG_INITIALIZATION_FAILED, 5, 6, 0, 0);
    }

    /* Convert it to Unicode and nul-terminate it */
    RtlAnsiStringToUnicodeString(&CmpLoadOptions, &TempString, FALSE);
    CmpLoadOptions.Buffer[TempString.Length] = UNICODE_NULL;
    CmpLoadOptions.Length += sizeof(WCHAR);

    /* Get the image base */
    HiveImageBase = LoaderBlock->RegistryBase;
    if (!HiveImageBase)
    {
        /* Don't have a system hive, create it */
        Status = CmpInitializeHive(&SystemHive,
                                   HINIT_CREATE,
                                   HIVE_NOLAZYFLUSH,
                                   HFILE_TYPE_LOG,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &CmpSystemFileName,
                                   0);
        if (!NT_SUCCESS(Status)) return FALSE;
        Allocate = TRUE;
    }
    else
    {
        /* Make a copy of the loaded hive */
        Status = CmpInitializeHive(&SystemHive,
                                   HINIT_MEMORY,
                                   0,
                                   HFILE_TYPE_LOG,
                                   HiveImageBase,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &CmpSystemFileName,
                                   2);
        if (!NT_SUCCESS(Status)) return FALSE;

        /* Check if we loaded with shared hives */
        if (CmpShareSystemHives) SystemHive->Hive.HvBinHeadersUse = 0;

        /* Set the boot type */
        CmpBootType = SystemHive->Hive.BaseBlock->BootType;

        /* Check if we loaded in self-healing mode */
        if (!CmSelfHeal)
        {
            /* We didn't, check the boot type */
            if (CmpBootType & 4)
            {
                /* Invalid boot type */
                CmpSelfHeal = FALSE;
                KeBugCheckEx(REGISTRY_ERROR, 3, 3, (ULONG_PTR)SystemHive, 0);
            }
        }
    }

    /* Create security descriptor */
    SecurityDescriptor = CmpHiveRootSecurityDescriptor();

    /* Link the hive to the master */
    Status = CmpLinkHiveToMaster(&CmRegistryMachineSystemName,
                                 NULL,
                                 SystemHive,
                                 Allocate,
                                 SecurityDescriptor);

    /* Free the descriptor and check for success */
    ExFreePool(SecurityDescriptor);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Save the hive */
    CmpMachineHiveList[3].CmHive = SystemHive;
    return TRUE;
}

NTSTATUS
NTAPI
CmpCreateControlSet(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNICODE_STRING ConfigName = RTL_CONSTANT_STRING(L"Control\\IDConfigDB");
    UNICODE_STRING SelectName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\Select");
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CHAR ValueInfoBuffer[128];
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    CHAR Buffer[128];
    WCHAR UnicodeBuffer[128];
    HANDLE SelectHandle, KeyHandle, ConfigHandle = NULL, ProfileHandle = NULL;
    HANDLE ParentHandle = NULL;
    ULONG ControlSet, HwProfile;
    ANSI_STRING TempString;
    NTSTATUS Status;
    ULONG ResultLength, Disposition;
    PLOADER_PARAMETER_EXTENSION LoaderExtension;
    PAGED_CODE();
    if (ExpInTextModeSetup) return STATUS_SUCCESS;

    /* Open the select key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SelectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&SelectHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Open the current value */
    RtlInitUnicodeString(&KeyName, L"Current");
    Status = NtQueryValueKey(SelectHandle,
                             &KeyName,
                             KeyValueFullInformation,
                             ValueInfoBuffer,
                             sizeof(ValueInfoBuffer),
                             &ResultLength);
    NtClose(SelectHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the actual value pointer, and get the control set ID */
    ValueInfo = (PKEY_VALUE_FULL_INFORMATION)ValueInfoBuffer;
    ControlSet = *(PULONG)((PUCHAR)ValueInfo + ValueInfo->DataOffset);

    /* Create the current control set key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateKey(&KeyHandle,
                         KEY_CREATE_LINK,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
                         &Disposition);
    if (!NT_SUCCESS(Status)) return Status;

    /* Sanity check */
    ASSERT(Disposition == REG_CREATED_NEW_KEY);

    /* Initialize the symbolic link name */
    sprintf(Buffer,
            "\\Registry\\Machine\\System\\ControlSet%03ld",
            ControlSet);
    RtlInitAnsiString(&TempString, Buffer);

    /* Create a Unicode string out of it */
    KeyName.MaximumLength = sizeof(UnicodeBuffer);
    KeyName.Buffer = UnicodeBuffer;
    Status = RtlAnsiStringToUnicodeString(&KeyName, &TempString, FALSE);

    /* Set the value */
    Status = NtSetValueKey(KeyHandle,
                           &CmSymbolicLinkValueName,
                           0,
                           REG_LINK,
                           KeyName.Buffer,
                           KeyName.Length);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the configuration database key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &ConfigName,
                               OBJ_CASE_INSENSITIVE,
                               KeyHandle,
                               NULL);
    Status = NtOpenKey(&ConfigHandle, KEY_READ, &ObjectAttributes);
    NtClose(KeyHandle);

    /* Check if we don't have one */
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup and exit */
        ConfigHandle = 0;
        goto Cleanup;
    }

    /* Now get the current config */
    RtlInitUnicodeString(&KeyName, L"CurrentConfig");
    Status = NtQueryValueKey(ConfigHandle,
                             &KeyName,
                             KeyValueFullInformation,
                             ValueInfoBuffer,
                             sizeof(ValueInfoBuffer),
                             &ResultLength);

    /* Set pointer to buffer */
    ValueInfo = (PKEY_VALUE_FULL_INFORMATION)ValueInfoBuffer;

    /* Check if we failed or got a non DWORD-value */
    if (!(NT_SUCCESS(Status)) || (ValueInfo->Type != REG_DWORD)) goto Cleanup;

    /* Get the hadware profile */
    HwProfile = *(PULONG)((PUCHAR)ValueInfo + ValueInfo->DataOffset);

    /* Open the hardware profile key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet"
                         L"\\Hardware Profiles");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&ParentHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Exit and clean up */
        ParentHandle = 0;
        goto Cleanup;
    }

    /* Build the profile name */
    sprintf(Buffer, "%04ld", HwProfile);
    RtlInitAnsiString(&TempString, Buffer);

    /* Convert it to Unicode */
    KeyName.MaximumLength = sizeof(UnicodeBuffer);
    KeyName.Buffer = UnicodeBuffer;
    Status = RtlAnsiStringToUnicodeString(&KeyName,
                                          &TempString,
                                          FALSE);
    ASSERT(Status == STATUS_SUCCESS);

    /* Open the associated key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               ParentHandle,
                               NULL);
    Status = NtOpenKey(&ProfileHandle,
                       KEY_READ | KEY_WRITE,
                       &ObjectAttributes);
    if (!NT_SUCCESS (Status))
    {
        /* Cleanup and exit */
        ProfileHandle = 0;
        goto Cleanup;
    }

    /* Check if we have a loader block extension */
    LoaderExtension = LoaderBlock->Extension;
    if (LoaderExtension)
    {
        ASSERTMSG("ReactOS doesn't support NTLDR Profiles yet!\n", FALSE);
    }

    /* Create the current hardware profile key */
    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\"
                         L"Hardware Profiles\\Current");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtCreateKey(&KeyHandle,
                         KEY_CREATE_LINK,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_VOLATILE | REG_OPTION_CREATE_LINK,
                         &Disposition);
    if (NT_SUCCESS(Status))
    {
        /* Sanity check */
        ASSERT(Disposition == REG_CREATED_NEW_KEY);

        /* Create the profile name */
        sprintf(Buffer,
                "\\Registry\\Machine\\System\\CurrentControlSet\\"
                "Hardware Profiles\\%04ld",
                HwProfile);
        RtlInitAnsiString(&TempString, Buffer);

        /* Convert it to Unicode */
        KeyName.MaximumLength = sizeof(UnicodeBuffer);
        KeyName.Buffer = UnicodeBuffer;
        Status = RtlAnsiStringToUnicodeString(&KeyName,
                                              &TempString,
                                              FALSE);
        ASSERT(STATUS_SUCCESS == Status);

        /* Set it */
        Status = NtSetValueKey(KeyHandle,
                               &CmSymbolicLinkValueName,
                               0,
                               REG_LINK,
                               KeyName.Buffer,
                               KeyName.Length);
        NtClose(KeyHandle);
    }

    /* Close every opened handle */
Cleanup:
    if (ConfigHandle) NtClose(ConfigHandle);
    if (ProfileHandle) NtClose(ProfileHandle);
    if (ParentHandle) NtClose(ParentHandle);

    /* Return success */
    return STATUS_SUCCESS;
}

BOOLEAN
NTAPI
CmInitSystem1(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PVOID BaseAddress;
    ULONG Length;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PCMHIVE HardwareHive;
    PAGED_CODE();

    /* Check if this is PE-boot */
    if (InitIsWinPEMode)
    {
        /* Set registry to PE mode */
        CmpMiniNTBoot = TRUE;
        CmpShareSystemHives = TRUE;
    }

    /* Initialize the hive list and lock */
    InitializeListHead(&CmpHiveListHead);
    ExInitializePushLock((PVOID)&CmpHiveListHeadLock);
    ExInitializePushLock((PVOID)&CmpLoadHiveLock);

    /* Initialize registry lock */
    ExInitializeResourceLite(&CmpRegistryLock);

    /* Initialize the cache */
    CmpInitializeCache();

    /* Initialize allocation and delayed dereferencing */
    CmpInitCmPrivateAlloc();
    CmpInitCmPrivateDelayAlloc();
    CmpInitDelayDerefKCBEngine();

    /* Initialize callbacks */
    CmpInitCallback();

    /* Initialize self healing */
    KeInitializeGuardedMutex(&CmpSelfHealQueueLock);
    InitializeListHead(&CmpSelfHealQueueListHead);

    /* Save the current process and lock the registry */
    CmpSystemProcess = PsGetCurrentProcess();

    /* Create the key object types */
    Status = CmpCreateObjectTypes();
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 1, Status, 0);
    }

    /* Create master hive */
    Status = CmpInitializeHive(&CmpMasterHive,
                               HINIT_CREATE,
                               HIVE_VOLATILE | 2, //HIVE_NO_FILE ROS
                               HFILE_TYPE_PRIMARY,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               0);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 2, Status, 0);
    }

    /* Create the \REGISTRY key node */
    if (!CmpCreateRegistryRoot())
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 3, 0, 0);
    }
    
    /* Create the default security descriptor */
    SecurityDescriptor = CmpHiveRootSecurityDescriptor();

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
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 5, Status, 0);
    }
    
    /* Close the handle */
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
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 6, Status, 0);
    }
    
    /* Close the handle */
    NtClose(KeyHandle);

    /* Initialize and load the system hive */
    if (!CmpInitializeSystemHive(KeLoaderBlock))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 7, 0, 0);
    }

    /* Create the CCS symbolic link */
    Status = CmpCreateControlSet(KeLoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 8, Status, 0);
    }
    
    /* Import the hardware hive (FIXME: We should create it from scratch) */
    BaseAddress = CmpRosGetHardwareHive(&Length);
    ((PHBASE_BLOCK)BaseAddress)->Length = Length;
    Status = CmpInitializeHive((PCMHIVE*)&HardwareHive,
                               HINIT_MEMORY, //HINIT_CREATE,
                               HIVE_VOLATILE,
                               HFILE_TYPE_PRIMARY,
                               BaseAddress, // NULL,
                               NULL,
                               NULL,
                               NULL,
                               NULL,
                               0);
    CmPrepareHive(&HardwareHive->Hive);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 11, Status, 0);
    }
    
    /* Attach it to the machine key */
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\HARDWARE");
    Status = CmpLinkHiveToMaster(&KeyName,
                                 NULL,
                                 (PCMHIVE)HardwareHive,
                                 FALSE,
                                 SecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 12, Status, 0);
    }

    /* Fill out the Hardware key with the ARC Data from the Loader */
    Status = CmpInitializeHardwareConfiguration(KeLoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 13, Status, 0);
    }

    /* Initialize machine-dependent information into the registry */
    Status = CmpInitializeMachineDependentConfiguration(KeLoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 14, Status, 0);
    }

    /* Initialize volatile registry settings */
    Status = CmpSetSystemValues(KeLoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 15, Status, 0);
    }
    
    /* Free the load options */
    ExFreePool(CmpLoadOptions.Buffer);
    
    /* If we got here, all went well */
    return TRUE;
}

VOID
CmShutdownRegistry(VOID)
{
    return;
}

