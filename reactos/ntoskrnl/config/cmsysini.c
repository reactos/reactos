/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/config/cmsysini.c
 * PURPOSE:         Configuration Manager - System Initialization Code
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include "ntoskrnl.h"
#define NDEBUG
#include "debug.h"

POBJECT_TYPE CmpKeyObjectType;
PCMHIVE CmiVolatileHive;
LIST_ENTRY CmpHiveListHead;
ERESOURCE CmpRegistryLock;
KGUARDED_MUTEX CmpSelfHealQueueLock;
LIST_ENTRY CmpSelfHealQueueListHead;
KEVENT CmpLoadWorkerEvent;
LONG CmpLoadWorkerIncrement;
PEPROCESS CmpSystemProcess;
BOOLEAN HvShutdownComplete;
PVOID CmpRegistryLockCallerCaller, CmpRegistryLockCaller;
BOOLEAN CmpFlushStarveWriters;
BOOLEAN CmpFlushOnLockRelease;
BOOLEAN CmpSpecialBootCondition;
BOOLEAN CmpNoWrite;
BOOLEAN CmpForceForceFlush;
BOOLEAN CmpWasSetupBoot;

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
CmpDeleteKeyObject(PVOID DeletedObject)
{
    PCM_KEY_BODY KeyBody = (PCM_KEY_BODY)DeletedObject;
    PCM_KEY_CONTROL_BLOCK Kcb;
    REG_KEY_HANDLE_CLOSE_INFORMATION KeyHandleCloseInfo;
    REG_POST_OPERATION_INFORMATION PostOperationInfo;
    NTSTATUS Status;
    PAGED_CODE();
    
    /* First off, prepare the handle close information callback */
    PostOperationInfo.Object = KeyBody;
    KeyHandleCloseInfo.Object = KeyBody;
    Status = CmiCallRegisteredCallbacks(RegNtPreKeyHandleClose,
                                        &KeyHandleCloseInfo);
    if (!NT_SUCCESS(Status))
    {
        /* If we failed, notify the post routine */
        PostOperationInfo.Status = Status;
        CmiCallRegisteredCallbacks(RegNtPostKeyHandleClose, &PostOperationInfo);
        return;
    }
    
    /* Acquire hive lock */
    CmpLockRegistry();
    
    /* Make sure this is a valid key body */
    if (KeyBody->Type == TAG('k', 'y', '0', '2'))
    {
        /* Get the KCB */
        Kcb = KeyBody->KeyControlBlock;
        if (Kcb)
        {
            /* Delist the key (once new parse routines are used) */
            //DelistKeyBodyFromKCB(KeyBody, FALSE);
        }
        
        /* Dereference the KCB */
        CmpDelayDerefKeyControlBlock(Kcb);
        
    }
    
    /* Release the registry lock */
    CmpUnlockRegistry();
    
    /* Do the post callback */
    PostOperationInfo.Status = STATUS_SUCCESS;
    CmiCallRegisteredCallbacks(RegNtPostKeyHandleClose, &PostOperationInfo);
}

VOID
NTAPI
CmpCloseKeyObject(IN PEPROCESS Process OPTIONAL,
                  IN PVOID Object,
                  IN ACCESS_MASK GrantedAccess,
                  IN ULONG ProcessHandleCount,
                  IN ULONG SystemHandleCount)
{
    PCM_KEY_BODY KeyBody = (PCM_KEY_BODY)Object;
    PAGED_CODE();
    
    /* Don't do anything if we're not the last handle */
    if (SystemHandleCount > 1) return;
    
    /* Make sure we're a valid key body */
    if (KeyBody->Type == TAG('k', 'y', '0', '2'))
    {
        /* Don't do anything if we don't have a notify block */
        if (!KeyBody->NotifyBlock) return;
        
        /* This shouldn't happen yet */
        ASSERT(FALSE);
    }
}

NTSTATUS
NTAPI
CmpQueryKeyName(IN PVOID ObjectBody,
                IN BOOLEAN HasName,
                IN OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
                IN ULONG Length,
                OUT PULONG ReturnLength,
                IN KPROCESSOR_MODE PreviousMode)
{
    DPRINT1("CmpQueryKeyName() called\n");
    while (TRUE);
    return STATUS_SUCCESS;
}

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

    /* Return success */
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpSetSystemValues(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName = {0};
    HANDLE KeyHandle;
    NTSTATUS Status;
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
    return (ExpInTextModeSetup ? STATUS_SUCCESS : Status);
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

    /* Open the select key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SelectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&SelectHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* ReactOS Hack: Hard-code current to 001 for SetupLdr */
        if (!LoaderBlock->RegistryBase)
        {
            /* Build the ControlSet001 key */
            RtlInitUnicodeString(&KeyName,
                                 L"\\Registry\\Machine\\System\\ControlSet001");
            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
            Status = NtCreateKey(&KeyHandle,
                                 KEY_ALL_ACCESS,
                                 &ObjectAttributes,
                                 0,
                                 NULL,
                                 0,
                                 &Disposition);
            if (!NT_SUCCESS(Status)) return Status;

            /* Don't need the handle */
            ZwClose(KeyHandle);

            /* Use hard-coded setting */
            ControlSet = 1;
            goto UseSet;
        }

        /* Fail for real boots */
        return Status;
    }

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
UseSet:
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

NTSTATUS
NTAPI
CmpLinkHiveToMaster(IN PUNICODE_STRING LinkName,
                    IN HANDLE RootDirectory,
                    IN PCMHIVE RegistryHive,
                    IN BOOLEAN Allocate,
                    IN PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    CM_PARSE_CONTEXT ParseContext = {0};
    HANDLE KeyHandle;
    PCM_KEY_BODY KeyBody;
    PAGED_CODE();
    
    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               RootDirectory,
                               SecurityDescriptor);
    
    /* Setup the parse context */
    ParseContext.CreateLink = TRUE;
    ParseContext.CreateOperation = TRUE;
    ParseContext.ChildHive.KeyHive = &RegistryHive->Hive;
    
    /* Check if we have a root keycell or if we need to create it */
    if (Allocate)
    {
        /* Create it */
        ParseContext.ChildHive.KeyCell = HCELL_NIL;
    }
    else
    {
        /* We have one */
        ParseContext.ChildHive.KeyCell = RegistryHive->Hive.BaseBlock->RootCell;   
    }

    /* Create the link node */
    Status = ObOpenObjectByName(&ObjectAttributes,
                                CmpKeyObjectType,
                                KernelMode,
                                NULL,
                                KEY_READ | KEY_WRITE,
                                (PVOID)&ParseContext,
                                &KeyHandle);
    if (!NT_SUCCESS(Status)) return Status;
    
    /* Mark the hive as clean */
    RegistryHive->Hive.DirtyFlag = FALSE;
    
    /* ReactOS Hack: Keep alive */
    Status = ObReferenceObjectByHandle(KeyHandle,
                                       0,
                                       CmpKeyObjectType,
                                       KernelMode,
                                       (PVOID*)&KeyBody,
                                       NULL);
    ASSERT(NT_SUCCESS(Status));

    /* Close the extra handle */
    ZwClose(KeyHandle);
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
                                   HIVE_NOLAZYFLUSH,
                                   HFILE_TYPE_LOG,
                                   HiveBase,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &HiveName,
                                   2);
        if (!NT_SUCCESS(Status)) return FALSE;

        /* Set the hive filename */
        RtlCreateUnicodeString(&SystemHive->FileFullPath,
                               L"\\SystemRoot\\System32\\Config\\SYSTEM");

        /* We imported, no need to create a new hive */
        Allocate = FALSE;

        /* Manually set the hive as volatile, if in Live CD mode */
        if (CmpShareSystemHives) SystemHive->Hive.HiveFlags = HIVE_VOLATILE;
    }
    else
    {
        /* Create it */
        Status = CmpInitializeHive(&SystemHive,
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
        
        /* Set the hive filename */
        RtlCreateUnicodeString(&SystemHive->FileFullPath,
                               L"\\SystemRoot\\System32\\Config\\SYSTEM");

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
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\SYSTEM");
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
    ObjectTypeInitializer.CloseProcedure = CmpCloseKeyObject;
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
    PCM_KEY_BODY RootKey;
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
                            sizeof(CM_KEY_BODY),
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
    RootKey->KeyControlBlock = Kcb;
    RootKey->Type = TAG('k', 'y', '0', '2');
    RootKey->NotifyBlock = NULL;
    RootKey->ProcessID = PsGetCurrentProcessId();

    /* Link with KCB */
    EnlistKeyBodyWithKCB(RootKey, 0);

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

NTSTATUS
NTAPI
CmpGetRegistryPath(IN PWCHAR ConfigPath)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;
    HANDLE KeyHandle;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\HARDWARE");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"InstallPath");
    ULONG BufferSize, ResultSize;

    /* Check if we are booted in setup */
    if (ExpInTextModeSetup)
    {
        /* Setup the object attributes */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        /* Open the key */
        Status =  ZwOpenKey(&KeyHandle,
                            KEY_ALL_ACCESS,
                            &ObjectAttributes);
        if (!NT_SUCCESS(Status)) return Status;
        
        /* Allocate the buffer */
        BufferSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 4096;
        ValueInfo = ExAllocatePoolWithTag(PagedPool, BufferSize, TAG_CM);
        if (!ValueInfo)
        {
            /* Fail */
            ZwClose(KeyHandle);
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Query the value */
        Status = ZwQueryValueKey(KeyHandle,
                                 &ValueName,
                                 KeyValuePartialInformation,
                                 ValueInfo,
                                 BufferSize,
                                 &ResultSize);
        ZwClose(KeyHandle);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            ExFreePool(ValueInfo);
            return Status;
        }

        /* Copy the config path and null-terminate it */
        RtlCopyMemory(ConfigPath,
                      ValueInfo->Data,
                      ValueInfo->DataLength);
        ConfigPath[ValueInfo->DataLength / sizeof(WCHAR)] = UNICODE_NULL;
        ExFreePool(ValueInfo);
    }
    else
    {
        /* Just use default path */
        wcscpy(ConfigPath, L"\\SystemRoot");
    }

    /* Add registry path */
    wcscat(ConfigPath, L"\\System32\\Config\\");

    /* Done */
    return STATUS_SUCCESS;
}

VOID
NTAPI
CmpLoadHiveThread(IN PVOID StartContext)
{
    WCHAR FileBuffer[MAX_PATH], RegBuffer[MAX_PATH], ConfigPath[MAX_PATH];
    UNICODE_STRING TempName, FileName, RegName;
    ULONG FileStart, RegStart, i, ErrorResponse, WorkerCount, Length;
    ULONG PrimaryDisposition, SecondaryDisposition, ClusterSize;
    PCMHIVE CmHive;
    HANDLE PrimaryHandle, LogHandle;
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID ErrorParameters;
    PAGED_CODE();
    
    /* Get the hive index, make sure it makes sense */
    i = (ULONG)StartContext;
    ASSERT(CmpMachineHiveList[i].Name != NULL);
   
    /* We were started */
    CmpMachineHiveList[i].ThreadStarted = TRUE;
    
    /* Build the file name and registry name strings */
    RtlInitEmptyUnicodeString(&FileName, FileBuffer, MAX_PATH);
    RtlInitEmptyUnicodeString(&RegName, RegBuffer, MAX_PATH);
    
    /* Now build the system root path */
    CmpGetRegistryPath(ConfigPath);
    RtlInitUnicodeString(&TempName, ConfigPath);
    RtlAppendStringToString((PSTRING)&FileName, (PSTRING)&TempName);
    FileStart = FileName.Length;
    
    /* And build the registry root path */
    RtlInitUnicodeString(&TempName, L"\\REGISTRY\\");
    RtlAppendStringToString((PSTRING)&RegName, (PSTRING)&TempName);
    RegStart = RegName.Length;
    
    /* Build the base name */
    RegName.Length = RegStart;
    RtlInitUnicodeString(&TempName, CmpMachineHiveList[i].BaseName);
    RtlAppendStringToString((PSTRING)&RegName, (PSTRING)&TempName);
    
    /* Check if this is a child of the root */
    if (RegName.Buffer[RegName.Length / sizeof(WCHAR) - 1] == '\\')
    {
        /* Then setup the whole name */
        RtlInitUnicodeString(&TempName, CmpMachineHiveList[i].Name);
        RtlAppendStringToString((PSTRING)&RegName, (PSTRING)&TempName);
    }
    
    /* Now Add tge rest if the file name */
    RtlInitUnicodeString(&TempName, CmpMachineHiveList[i].Name);
    FileName.Length = FileStart;
    RtlAppendStringToString((PSTRING)&FileName, (PSTRING)&TempName);
    if (!CmpMachineHiveList[i].CmHive)
    {
        /* We need to allocate a new hive structure */
        CmpMachineHiveList[i].Allocate = TRUE;
        
        /* Load the hive file */
        Status = CmpInitHiveFromFile(&FileName,
                                     CmpMachineHiveList[i].HHiveFlags,
                                     &CmHive,
                                     &CmpMachineHiveList[i].Allocate,
                                     0);
        if (!(NT_SUCCESS(Status)) ||
            (!(CmHive->FileHandles[HFILE_TYPE_LOG]) && !(CmpMiniNTBoot))) // hak
        {
            /* We failed or couldn't get a log file, raise a hard error */
            ErrorParameters = &FileName;
            NtRaiseHardError(STATUS_CANNOT_LOAD_REGISTRY_FILE,
                             1,
                             1,
                             (PULONG_PTR)&ErrorParameters,
                             OptionOk,
                             &ErrorResponse);
        }
        
        /* Set the hive flags and newly allocated hive pointer */
        CmHive->Flags = CmpMachineHiveList[i].CmHiveFlags;
        CmpMachineHiveList[i].CmHive2 = CmHive;
    }
    else
    {
        /* We already have a hive, is it volatile? */
        CmHive = CmpMachineHiveList[i].CmHive;
        if (!(CmHive->Hive.HiveFlags & HIVE_VOLATILE))
        {
            /* It's now, open the hive file and log */
            Status = CmpOpenHiveFiles(&FileName,
                                      L".LOG",
                                      &PrimaryHandle,
                                      &LogHandle,
                                      &PrimaryDisposition,
                                      &SecondaryDisposition,
                                      TRUE,
                                      TRUE,
                                      FALSE,
                                      &ClusterSize);
            if (!(NT_SUCCESS(Status)) || !(LogHandle))
            {
                /* Couldn't open the hive or its log file, raise a hard error */
                ErrorParameters = &FileName;
                NtRaiseHardError(STATUS_CANNOT_LOAD_REGISTRY_FILE,
                                 1,
                                 1,
                                 (PULONG_PTR)&ErrorParameters,
                                 OptionOk,
                                 &ErrorResponse);
                
                /* And bugcheck for posterity's sake */
                KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO, 9, 0, i, Status);
            }
            
            /* Save the file handles. This should remove our sync hacks */
            CmHive->FileHandles[HFILE_TYPE_LOG] = LogHandle;
            CmHive->FileHandles[HFILE_TYPE_PRIMARY] = PrimaryHandle;

            /* Allow lazy flushing since the handles are there -- remove sync hacks */
            //ASSERT(CmHive->Hive.HiveFlags & HIVE_NOLAZYFLUSH);
            CmHive->Hive.HiveFlags &= ~HIVE_NOLAZYFLUSH;

            /* Get the real size of the hive */
            Length = CmHive->Hive.Storage[Stable].Length + HBLOCK_SIZE;
          
            /* Check if the cluster size doesn't match */
            if (CmHive->Hive.Cluster != ClusterSize) ASSERT(FALSE);
            
            /* Set the file size */
            //if (!CmpFileSetSize((PHHIVE)CmHive, HFILE_TYPE_PRIMARY, Length, Length))
            {
                /* This shouldn't fail */
                //ASSERT(FALSE);
            }
     
            /* Another thing we don't support is NTLDR-recovery */
            if (CmHive->Hive.BaseBlock->BootRecover) ASSERT(FALSE);
            
            /* Finally, set our allocated hive to the same hive we've had */
            CmpMachineHiveList[i].CmHive2 = CmHive;
            ASSERT(CmpMachineHiveList[i].CmHive == CmpMachineHiveList[i].CmHive2);
        }
    }
    
    /* We're done */
    CmpMachineHiveList[i].ThreadFinished = TRUE;
    
    /* Check if we're the last worker */
    WorkerCount = InterlockedIncrement(&CmpLoadWorkerIncrement);
    if (WorkerCount == CM_NUMBER_OF_MACHINE_HIVES)
    {
        /* Signal the event */
        KeSetEvent(&CmpLoadWorkerEvent, 0, FALSE);
    }

    /* Kill the thread */
    PsTerminateSystemThread(Status);
}

VOID
NTAPI
CmpInitializeHiveList(IN USHORT Flag)
{
    WCHAR FileBuffer[MAX_PATH], RegBuffer[MAX_PATH], ConfigPath[MAX_PATH];
    UNICODE_STRING TempName, FileName, RegName;
    HANDLE Thread;
    NTSTATUS Status;
    ULONG FileStart, RegStart, i;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PAGED_CODE();
    
    /* Allow writing for now */
    CmpNoWrite = FALSE;
    
    /* Build the file name and registry name strings */
    RtlInitEmptyUnicodeString(&FileName, FileBuffer, MAX_PATH);
    RtlInitEmptyUnicodeString(&RegName, RegBuffer, MAX_PATH);
    
    /* Now build the system root path */
    CmpGetRegistryPath(ConfigPath);
    RtlInitUnicodeString(&TempName, ConfigPath);
    RtlAppendStringToString((PSTRING)&FileName, (PSTRING)&TempName);
    FileStart = FileName.Length;
    
    /* And build the registry root path */
    RtlInitUnicodeString(&TempName, L"\\REGISTRY\\");
    RtlAppendStringToString((PSTRING)&RegName, (PSTRING)&TempName);
    RegStart = RegName.Length;
    
    /* Setup the event to synchronize workers */
    KeInitializeEvent(&CmpLoadWorkerEvent, SynchronizationEvent, FALSE);
    
    /* Enter special boot condition */
    CmpSpecialBootCondition = TRUE;
    
    /* Create the SD for the root hives */
    SecurityDescriptor = CmpHiveRootSecurityDescriptor();      
    
    /* Loop every hive we care about */
    for (i = 0; i < CM_NUMBER_OF_MACHINE_HIVES; i++)
    {
        /* Make sure the list is setup */
        ASSERT(CmpMachineHiveList[i].Name != NULL);
        
        /* Create a thread to handle this hive */
        Status = PsCreateSystemThread(&Thread,
                                      THREAD_ALL_ACCESS,
                                      NULL,
                                      0,
                                      NULL,
                                      CmpLoadHiveThread,
                                      (PVOID)i);
        if (NT_SUCCESS(Status))
        {
            /* We don't care about the handle -- the thread self-terminates */
            ZwClose(Thread);
        }
        else
        {
            /* Can't imagine this happening */
            KeBugCheckEx(BAD_SYSTEM_CONFIG_INFO, 9, 3, i, Status);
        }
    }
    
    /* Make sure we've reached the end of the list */
    ASSERT(CmpMachineHiveList[i].Name == NULL);
    
    /* Wait for hive loading to finish */
    KeWaitForSingleObject(&CmpLoadWorkerEvent,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);
    
    /* Exit the special boot condition and make sure all workers completed */
    CmpSpecialBootCondition = FALSE;
    ASSERT(CmpLoadWorkerIncrement == CM_NUMBER_OF_MACHINE_HIVES);
    
    /* Loop hives again */
    for (i = 0; i < CM_NUMBER_OF_MACHINE_HIVES; i++)
    {
        /* Make sure the thread ran and finished */
        ASSERT(CmpMachineHiveList[i].ThreadFinished == TRUE);
        ASSERT(CmpMachineHiveList[i].ThreadStarted == TRUE);
        
        /* Check if this was a new hive */
        if (!CmpMachineHiveList[i].CmHive)
        {
            /* Make sure we allocated something */
            ASSERT(CmpMachineHiveList[i].CmHive2 != NULL);
            
            /* Build the base name */
            RegName.Length = RegStart;
            RtlInitUnicodeString(&TempName, CmpMachineHiveList[i].BaseName);
            RtlAppendStringToString((PSTRING)&RegName, (PSTRING)&TempName);
            
            /* Check if this is a child of the root */
            if (RegName.Buffer[RegName.Length / sizeof(WCHAR) - 1] == '\\')
            {
                /* Then setup the whole name */
                RtlInitUnicodeString(&TempName, CmpMachineHiveList[i].Name);
                RtlAppendStringToString((PSTRING)&RegName, (PSTRING)&TempName);
            }
            
            /* Now link the hive to its master */
            Status = CmpLinkHiveToMaster(&RegName,
                                         NULL,
                                         CmpMachineHiveList[i].CmHive2,
                                         CmpMachineHiveList[i].Allocate,
                                         SecurityDescriptor);
            if (Status != STATUS_SUCCESS)
            {
                /* Linking needs to work */
                KeBugCheckEx(CONFIG_LIST_FAILED, 11, Status, i, (ULONG_PTR)&RegName);
            }
            
            /* Check if we had to allocate a new hive */
			if (CmpMachineHiveList[i].Allocate)
            {
                /* Sync the new hive */
				//HvSyncHive((PHHIVE)(CmpMachineHiveList[i].CmHive2));
			}   
        }
        
        /* Check if we created a new hive */
        if (CmpMachineHiveList[i].CmHive2)
        {
            /* TODO: Add to HiveList key */
        }
    }
    
    /* Get rid of the SD */
    ExFreePool(SecurityDescriptor);

    /* FIXME: Link SECURITY to SAM */
    
    /* FIXME: Link S-1-5-18 to .Default */
}

BOOLEAN
NTAPI
CmInitSystem1(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PCMHIVE HardwareHive;
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
                               HIVE_VOLATILE,
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

    /* Create the hardware hive */
    Status = CmpInitializeHive((PCMHIVE*)&HardwareHive,
                               HINIT_CREATE,
                               HIVE_VOLATILE,
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
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 11, Status, 0);
    }
    
    /* Add the hive to the hive list */
    CmpMachineHiveList[0].CmHive = (PCMHIVE)HardwareHive;

    /* Attach it to the machine key */
    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\HARDWARE");
    Status = CmpLinkHiveToMaster(&KeyName,
                                 NULL,
                                 (PCMHIVE)HardwareHive,
                                 TRUE,
                                 SecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KEBUGCHECKEX(CONFIG_INITIALIZATION_FAILED, 1, 12, Status, 0);
    }
    
    /* FIXME: Add to HiveList key */
    
    /* Free the security descriptor */
    ExFreePool(SecurityDescriptor);

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

VOID
NTAPI
CmpLockRegistry(VOID)
{
    /* Enter a critical region */
    KeEnterCriticalRegion();
    
    /* Check if we have to starve writers */
    if (CmpFlushStarveWriters)
    {
        /* Starve exlusive waiters */
        ExAcquireSharedStarveExclusive(&CmpRegistryLock, TRUE);
    }
    else
    {
        /* Just grab the lock */
        ExAcquireResourceSharedLite(&CmpRegistryLock, TRUE);
    }
}

BOOLEAN
NTAPI
CmpTestRegistryLock(VOID)
{
    /* Test the lock */
    return !ExIsResourceAcquiredSharedLite(&CmpRegistryLock) ? FALSE : TRUE;
}

BOOLEAN
NTAPI
CmpTestRegistryLockExclusive(VOID)
{
    /* Test the lock */
    return !ExIsResourceAcquiredExclusiveLite(&CmpRegistryLock) ? FALSE : TRUE;
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
        CmpDoFlushAll(TRUE);
        CmpFlushOnLockRelease = FALSE;
    }
    
    /* Release the lock and leave the critical region */
    ExReleaseResourceLite(&CmpRegistryLock);
    KeLeaveCriticalRegion();
}

VOID
NTAPI
CmpAcquireTwoKcbLocksExclusiveByKey(IN ULONG ConvKey1,
                                    IN ULONG ConvKey2)
{
    ULONG Index1, Index2;
    
    /* Sanity check */
    CMP_ASSERT_REGISTRY_LOCK();

    /* Get hash indexes */
    Index1 = GET_HASH_INDEX(ConvKey1);
    Index2 = GET_HASH_INDEX(ConvKey2);

    /* See which one is highest */
    if (Index1 < Index2)
    {
        /* Grab them in the proper order */
        CmpAcquireKcbLockExclusiveByKey(ConvKey1);
        CmpAcquireKcbLockExclusiveByKey(ConvKey2);
    }
    else
    {
        /* Grab the second one first, then the first */
        CmpAcquireKcbLockExclusiveByKey(ConvKey2);
        if (Index1 != Index2) CmpAcquireKcbLockExclusiveByKey(ConvKey1);        
    }
}

VOID
NTAPI
CmpReleaseTwoKcbLockByKey(IN ULONG ConvKey1,
                          IN ULONG ConvKey2)
{
    ULONG Index1, Index2;
    
    /* Sanity check */
    CMP_ASSERT_REGISTRY_LOCK();
    
    /* Get hash indexes */
    Index1 = GET_HASH_INDEX(ConvKey1);
    Index2 = GET_HASH_INDEX(ConvKey2);
    ASSERT((GET_HASH_ENTRY(CmpCacheTable, ConvKey2).Owner == KeGetCurrentThread()) ||
           (CmpTestRegistryLockExclusive()));
    
    /* See which one is highest */
    if (Index1 < Index2)
    {
        /* Grab them in the proper order */
        ASSERT((GET_HASH_ENTRY(CmpCacheTable, ConvKey1).Owner == KeGetCurrentThread()) ||
               (CmpTestRegistryLockExclusive()));
        CmpReleaseKcbLockByKey(ConvKey2);
        CmpReleaseKcbLockByKey(ConvKey1);
    }
    else
    {
        /* Release the first one first, then the second */
        if (Index1 != Index2)
        {
            ASSERT((GET_HASH_ENTRY(CmpCacheTable, ConvKey1).Owner == KeGetCurrentThread()) ||
                   (CmpTestRegistryLockExclusive()));
            CmpReleaseKcbLockByKey(ConvKey1);        
        }
        CmpReleaseKcbLockByKey(ConvKey2);
    }
}

VOID
NTAPI
CmShutdownSystem(VOID)
{
    /* Kill the workers and fush all hives */
    CmpShutdownWorkers();
    CmpDoFlushAll(TRUE);
}
