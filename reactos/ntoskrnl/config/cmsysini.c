/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmsysini.c
 * PURPOSE:         Configuration Manager - System Initialization Code
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#include "cm.h"
#define NDEBUG
#include "debug.h"

KGUARDED_MUTEX CmpSelfHealQueueLock;
LIST_ENTRY CmpSelfHealQueueListHead;
PEPROCESS CmpSystemProcess;
BOOLEAN HvShutdownComplete;
PVOID CmpRegistryLockCallerCaller, CmpRegistryLockCaller;
BOOLEAN CmpFlushStarveWriters;
BOOLEAN CmpFlushOnLockRelease;
BOOLEAN CmpSpecialBootCondition;
BOOLEAN CmpNoWrite;
BOOLEAN CmpForceForceFlush;

/* FUNCTIONS *****************************************************************/

NTSTATUS
NTAPI
CmpInitHiveFromFile(IN PCUNICODE_STRING HiveName,
                    IN ULONG HiveFlags,
                    OUT PCMHIVE *Hive,
                    IN OUT PBOOLEAN New,
                    IN ULONG CheckFlags)
{
    ULONG HiveDisposition, LogDisposition;
    HANDLE FileHandle = NULL, LogHandle = NULL;
    NTSTATUS Status;
    ULONG Operation, FileType;
    PCMHIVE NewHive;
    PAGED_CODE();

    /* Assume failure */
    *Hive = NULL;

    /* Open or create the hive files */
    Status = CmpOpenHiveFiles(HiveName,
                              L".LOG",
                              &FileHandle,
                              &LogHandle,
                              &HiveDisposition,
                              &LogDisposition,
                              *New,
                              FALSE,
                              TRUE,
                              NULL);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if we have a log handle */
    FileType = (LogHandle) ? HFILE_TYPE_LOG : HFILE_TYPE_PRIMARY;

    /* Check if we created or opened the hive */
    if (HiveDisposition == FILE_CREATED)
    {
        /* Do a create operation */
        Operation = HINIT_CREATE;
        *New = TRUE;
    }
    else
    {
        /* Open it as a file */
        Operation = HINIT_FILE;
        *New = FALSE;
    }

    /* Check if we're sharing hives */
    if (CmpShareSystemHives)
    {
        /* Then force using the primary hive */
        FileType = HFILE_TYPE_PRIMARY;
        if (LogHandle)
        {
            /* Get rid of the log handle */
            ZwClose(LogHandle);
            LogHandle = NULL;
        }
    }

    /* Check if we're too late */
    if (HvShutdownComplete)
    {
        /* Fail */
        ZwClose(FileHandle);
        if (LogHandle) ZwClose(LogHandle);
        return STATUS_TOO_LATE;
    }

    /* Initialize the hive */
    Status = CmpInitializeHive((PCMHIVE*)&NewHive,
                               Operation,
                               HiveFlags,
                               FileType,
                               NULL,
                               FileHandle,
                               LogHandle,
                               NULL,
                               HiveName,
                               0);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ZwClose(FileHandle);
        if (LogHandle) ZwClose(LogHandle);
        return Status;
    }

    /* Success, return hive */
    *Hive = NewHive;

    /* ROS: Init root key cell and prepare the hive */
    if (Operation == HINIT_CREATE) CmCreateRootNode(&NewHive->Hive, L"");
    CmPrepareHive(&NewHive->Hive);

    /* Duplicate the hive name */
    NewHive->FileFullPath.Buffer = ExAllocatePoolWithTag(PagedPool,
                                                         HiveName->Length,
                                                         TAG_CM);
    if (NewHive->FileFullPath.Buffer)
    {
        /* Copy the string */
        RtlCopyMemory(NewHive->FileFullPath.Buffer,
                      HiveName->Buffer,
                      HiveName->Length);
        NewHive->FileFullPath.Length = HiveName->Length;
        NewHive->FileFullPath.MaximumLength = HiveName->MaximumLength;
    }

    /* ROS: Close the hive files */
    ZwClose(FileHandle);
    if (LogHandle) ZwClose(LogHandle);

    /* Return success */
    return STATUS_SUCCESS;
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
CmpInitializeSystemHive(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PVOID HiveBase;
    ANSI_STRING LoadString;
    PVOID Buffer;
    ULONG Length;
    NTSTATUS Status;
    BOOLEAN Allocate;
    UNICODE_STRING KeyName;
    PCMHIVE SystemHive = NULL;
    UNICODE_STRING HiveName = RTL_CONSTANT_STRING(L"SYSTEM");
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PAGED_CODE();

    /* Setup the ansi string */
    RtlInitAnsiString(&LoadString, LoaderBlock->LoadOptions);

    /* Allocate the unicode buffer */
    Length = LoadString.Length * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    Buffer = ExAllocatePoolWithTag(PagedPool, Length, TAG_CM);
    if (!Buffer)
    {
        /* Fail */
        KEBUGCHECKEX(BAD_SYSTEM_CONFIG_INFO, 3, 1, (ULONG_PTR)LoaderBlock, 0);
    }

    /* Setup the unicode string */
    RtlInitEmptyUnicodeString(&CmpLoadOptions, Buffer, (USHORT)Length);

    /* Add the load options and null-terminate */
    RtlAnsiStringToUnicodeString(&CmpLoadOptions, &LoadString, FALSE);
    CmpLoadOptions.Buffer[LoadString.Length] = UNICODE_NULL;
    CmpLoadOptions.Length += sizeof(WCHAR);

    /* Get the System Hive base address */
    HiveBase = LoaderBlock->RegistryBase;
    if (HiveBase)
    {
        /* Import it */
        ((PHBASE_BLOCK)HiveBase)->Length = LoaderBlock->RegistryLength;
        Status = CmpInitializeHive((PCMHIVE*)&SystemHive,
                                   HINIT_MEMORY,
                                   0, //HIVE_NOLAZYFLUSH,
                                   HFILE_TYPE_LOG,
                                   HiveBase,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &HiveName,
                                   2);
        if (!NT_SUCCESS(Status)) return FALSE;
        CmPrepareHive(&SystemHive->Hive);

        /* Set the hive filename */
        RtlCreateUnicodeString(&SystemHive->FileFullPath, SYSTEM_REG_FILE);

        /* We imported, no need to create a new hive */
        Allocate = FALSE;

        /* Manually set the hive as volatile, if in Live CD mode */
        if (CmpShareSystemHives) SystemHive->Hive.HiveFlags = HIVE_VOLATILE;
    }
    else
    {
#if 0
        /* Create it */
        Status = CmpInitializeHive((PCMHIVE*)&SystemHive,
                                   HINIT_CREATE,
                                   HIVE_NOLAZYFLUSH,
                                   HFILE_TYPE_LOG,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &HiveName,
                                   0);
        if (!NT_SUCCESS(Status)) return FALSE;
#endif

        /* Tell CmpLinkHiveToMaster to allocate a hive */
        Allocate = TRUE;
    }

    /* Save the boot type */
    if (SystemHive) CmpBootType = SystemHive->Hive.BaseBlock->BootType;

    /* Are we in self-healing mode? */
    if (!CmSelfHeal)
    {
        /* Disable self-healing internally and check if boot type wanted it */
        CmpSelfHeal = FALSE;
        if (CmpBootType & 4)
        {
            /* We're disabled, so bugcheck */
            KEBUGCHECKEX(BAD_SYSTEM_CONFIG_INFO,
                         3,
                         3,
                         (ULONG_PTR)SystemHive,
                         0);
        }
    }

    /* Create the default security descriptor */
    SecurityDescriptor = CmpHiveRootSecurityDescriptor();

    /* Attach it to the system key */
    RtlInitUnicodeString(&KeyName, REG_SYSTEM_KEY_NAME);
    Status = CmpLinkHiveToMaster(&KeyName,
                                 NULL,
                                 (PCMHIVE)SystemHive,
                                 Allocate,
                                 SecurityDescriptor);

    /* Free the security descriptor */
    ExFreePool(SecurityDescriptor);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Add the hive to the hive list */
    CmpMachineHiveList[3].CmHive = (PCMHIVE)SystemHive;

    /* Success! */
    return TRUE;
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
                  IN PCWSTR Name,
                  OUT PHCELL_INDEX Index)
{
    UNICODE_STRING KeyName;
    PCM_KEY_NODE KeyCell;
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
    KeyCell = (PCM_KEY_NODE)HvGetCell(Hive, *Index);
    if (!KeyCell) return FALSE;

    /* Setup the cell */
    KeyCell->Signature = (USHORT)CM_KEY_NODE_SIGNATURE;
    KeyCell->Flags = KEY_HIVE_ENTRY | KEY_NO_DELETE;
    KeQuerySystemTime(&SystemTime);
    KeyCell->LastWriteTime = SystemTime;
    KeyCell->Parent = HCELL_NIL;
    KeyCell->SubKeyCounts[Stable] = 0;
    KeyCell->SubKeyCounts[Volatile] = 0;
    KeyCell->SubKeyLists[Stable] = HCELL_NIL;
    KeyCell->SubKeyLists[Volatile] = HCELL_NIL;
    KeyCell->ValueList.Count = 0;
    KeyCell->ValueList.List = HCELL_NIL;
    KeyCell->Security = HCELL_NIL;
    KeyCell->Class = HCELL_NIL;
    KeyCell->ClassLength = 0;
    KeyCell->MaxNameLen = 0;
    KeyCell->MaxClassLen = 0;
    KeyCell->MaxValueNameLen = 0;
    KeyCell->MaxValueDataLen = 0;

    /* Copy the name (this will also set the length) */
    KeyCell->NameLength = CmpCopyName(Hive, (PWCHAR)KeyCell->Name, &KeyName);

    /* Check if the name was compressed */
    if (KeyCell->NameLength < KeyName.Length)
    {
        /* Set the flag */
        KeyCell->Flags |= KEY_COMP_NAME;
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
#if 0
    PCM_KEY_BODY RootKey;
#else
    PKEY_OBJECT RootKey;
#endif
    HCELL_INDEX RootIndex;
    NTSTATUS Status;
    PCM_KEY_NODE KeyCell;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PCM_KEY_CONTROL_BLOCK Kcb;
    PAGED_CODE();

    /* Setup the root node */
    if (!CmpCreateRootNode(&CmiVolatileHive->Hive, L"REGISTRY", &RootIndex))
    {
        /* We failed */
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
                            sizeof(KEY_OBJECT),
                            0,
                            0,
                            (PVOID*)&RootKey);
    ExFreePool(SecurityDescriptor);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Sanity check, and get the key cell */
    ASSERT((&CmiVolatileHive->Hive)->ReleaseCellRoutine == NULL);
    KeyCell = (PCM_KEY_NODE)HvGetCell(&CmiVolatileHive->Hive, RootIndex);
    if (!KeyCell) return FALSE;

    /* Create the KCB */
    RtlInitUnicodeString(&KeyName, L"Registry");
    Kcb = CmpCreateKeyControlBlock(&CmiVolatileHive->Hive,
                                   RootIndex,
                                   KeyCell,
                                   NULL,
                                   0,
                                   &KeyName);
    if (!Kcb) return FALSE;

    /* Initialize the object */
#if 0
    RootKey->Type = TAG('k', 'v', '0', '2');
    RootKey->KeyControlBlock = Kcb;
    RootKey->NotifyBlock = NULL;
    RootKey->ProcessID = PsGetCurrentProcessId();
#else
    RtlpCreateUnicodeString(&RootKey->Name, L"Registry", NonPagedPool);
    RootKey->KeyControlBlock = Kcb;
    RootKey->RegistryHive = CmiVolatileHive;
    RootKey->KeyCellOffset = RootIndex;
    RootKey->KeyCell = KeyCell;
    RootKey->ParentKey = RootKey;
    RootKey->Flags = 0;
    RootKey->SubKeyCounts = 0;
    RootKey->SubKeys = NULL;
    RootKey->SizeOfSubKeys = 0;
#endif

    /* Insert it into the object list head */
    EnlistKeyBodyWithKeyObject(RootKey, 0);

    /* Insert the key into the namespace */
    Status = ObInsertObject(RootKey,
                            NULL,
                            KEY_ALL_ACCESS,
                            0,
                            NULL,
                            &CmpRegistryRootHandle);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Reference the key again so that we never lose it */
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

BOOLEAN
NTAPI
CmInitSystem1(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    LARGE_INTEGER DueTime;
    HANDLE ThreadHandle;
    CLIENT_ID ThreadId;
    PCMHIVE HardwareHive;
    PVOID BaseAddress;
    ULONG Length;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
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

#if 1
    /* OLD CM: Initialize the key object list */
    InitializeListHead(&CmiKeyObjectListHead);
    InitializeListHead(&CmiConnectedHiveList);

    /* OLD CM: Initialize the worker timer */
    KeInitializeTimerEx(&CmiWorkerTimer, SynchronizationTimer);

    /* OLD CM: Initialize the worker thread */
    Status = PsCreateSystemThread(&ThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  NULL,
                                  NULL,
                                  &ThreadId,
                                  CmiWorkerThread,
                                  NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* OLD CM: Start the timer */
    DueTime.QuadPart = -1;
    KeSetTimerEx(&CmiWorkerTimer, DueTime, 5000, NULL); /* 5sec */
#endif

    /* Create the key object types */
    Status = CmpCreateObjectTypes();
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 1, Status, 0);
    }

    /* Build the master hive */
    Status = CmpInitializeHive((PCMHIVE*)&CmiVolatileHive,
                               HINIT_CREATE,
                               HIVE_VOLATILE | HIVE_NO_FILE,
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
                               SecurityDescriptor);
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
                               SecurityDescriptor);
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

    /* Initialize the system hive */
    if (!CmpInitializeSystemHive(KeLoaderBlock))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 7, 0, 0);
    }

    /* Create the 'CurrentControlSet' link. */
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
                               HIVE_NO_FILE, //HIVE_VOLATILE,
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
    RtlInitUnicodeString(&KeyName, REG_HARDWARE_KEY_NAME);
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
NTAPI
CmpLockRegistryExclusive(VOID)
{
    /* Enter a critical region and lock the registry */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&CmpRegistryLock, TRUE);
    
    /* Sanity check */
    ASSERT(CmpFlushStarveWriters == 0);
    RtlGetCallersAddress(&CmpRegistryLockCaller, &CmpRegistryLockCallerCaller);
}

BOOLEAN
NTAPI
CmpTestRegistryLock(VOID)
{
    /* Test the lock */
    return (BOOLEAN)ExIsResourceAcquiredSharedLite(&CmpRegistryLock);
}

BOOLEAN
NTAPI
CmpTestRegistryLockExclusive(VOID)
{
    /* Test the lock */
    return ExIsResourceAcquiredExclusiveLite(&CmpRegistryLock);
}

VOID
NTAPI
CmpUnlockRegistry(VOID)
{
    /* Sanity check */
    CMP_ASSERT_REGISTRY_LOCK();
    
    /* Check if we should flush the registry */
    if (CmpFlushOnLockRelease)
    {
        /* The registry should be exclusively locked for this */
        CMP_ASSERT_EXCLUSIVE_REGISTRY_LOCK();
        
        /* Flush the registry */
        CmpFlushEntireRegistry(TRUE);
        CmpFlushOnLockRelease = FALSE;
    }
    
    /* Release the lock and leave the critical region */
    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();
}

BOOLEAN
NTAPI
CmpFlushEntireRegistry(IN BOOLEAN ForceFlush)
{
    BOOLEAN Flushed = TRUE;
    
    /* Make sure that the registry isn't read-only now */
    if (CmpNoWrite) return TRUE;
    
    /* Otherwise, acquire the hive list lock and disable force flush */
    CmpForceForceFlush = FALSE;
    ExAcquirePushLockShared(&CmpHiveListHeadLock);
    
    /* Check if the hive list isn't empty */
    if (!IsListEmpty(&CmpHiveListHead))
    {
        /* FIXME: TODO */
        ASSERT(FALSE);
    }
    
    /* Release the lock and return the flush state */
    ExReleasePushLock(&CmpHiveListHeadLock);
    return Flushed;
}
