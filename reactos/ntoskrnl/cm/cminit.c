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

UNICODE_STRING CmRegistryMachineSystemName =
    RTL_CONSTANT_STRING(L"\\REGISTRY\\MACHINE\\SYSTEM");
UNICODE_STRING CmpSystemFileName =
    RTL_CONSTANT_STRING(L"SYSTEM");
UNICODE_STRING CmSymbolicLinkValueName =
    RTL_CONSTANT_STRING(L"SymbolicLinkValue");

BOOLEAN CmpMiniNTBoot;
BOOLEAN CmpShareSystemHives;
ULONG CmpBootType;

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
    Cell->u.KeyNode.Signature = (USHORT)CM_KEY_NODE_SIGNATURE;
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
    Status = ObReferenceObjectByHandle(CmpRegistryRootHandle,
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

    /* Close the key and return */
    NtClose(KeyHandle);
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
                                   0,
                                   HFILE_TYPE_ALTERNATE,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &CmpSystemFileName);
        if (!NT_SUCCESS(Status)) return FALSE;
        Allocate = TRUE;
    }
    else
    {
        /* Make a copy of the loaded hive */
        Status = CmpInitializeHive(&SystemHive,
                                   HINIT_MEMORY,
                                   0,
                                   HFILE_TYPE_ALTERNATE,
                                   HiveImageBase,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &CmpSystemFileName);
        if (!NT_SUCCESS(Status)) return FALSE;

        /* Check if we loaded with shared hives */
        if (CmpShareSystemHives) SystemHive->Hive.HvBinHeadersUse = 0;

        /* Set the boot type */
        CmpBootType = SystemHive->Hive.HiveHeader->BootType;

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
CmpHwProfileDefaultSelect(IN PVOID ProfileList,
                          OUT PULONG ProfileIndexToUse,
                          IN PVOID Context)
{
    /* Clear the index and return success */
    *ProfileIndexToUse = 0;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpAddDockingInfo(IN HANDLE Key,
                  IN PPROFILE_PARAMETER_BLOCK ProfileBlock)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING KeyName;
    ULONG Value;
    PAGED_CODE ();

    /* Get the Value from the profile block, create a Name for it and set it */
    Value = ProfileBlock->DockingState;
    RtlInitUnicodeString(&KeyName, L"DockingState");
    Status = NtSetValueKey(Key,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &Value,
                           sizeof(Value));
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Value from the profile block, create a Name for it and set it */
    Value = ProfileBlock->Capabilities;
    RtlInitUnicodeString(&KeyName, L"Capabilities");
    Status = NtSetValueKey(Key,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &Value,
                           sizeof(Value));
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Value from the profile block, create a Name for it and set it */
    Value = ProfileBlock->DockID;
    RtlInitUnicodeString(&KeyName, L"DockID");
    Status = NtSetValueKey(Key,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &Value,
                           sizeof(Value));
    if (!NT_SUCCESS(Status)) return Status;

    /* Get the Value from the profile block, create a Name for it and set it */
    Value = ProfileBlock->SerialNumber;
    RtlInitUnicodeString(&KeyName, L"SerialNumber");
    Status = NtSetValueKey(Key,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &Value,
                           sizeof(Value));

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
CmpAddAliasEntry(IN HANDLE IDConfigDB,
                 IN PPROFILE_PARAMETER_BLOCK ProfileBlock,
                 IN ULONG ProfileNumber)
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status = STATUS_SUCCESS;
    CHAR Buffer[128];
    WCHAR UnicodeBuffer[128];
    ANSI_STRING TempString;
    HANDLE AliasHandle = NULL, AliasIdHandle = NULL;
    ULONG Value;
    ULONG Disposition;
    ULONG AliasId = 0;
    PAGED_CODE ();

    /* Open the alias key */
    RtlInitUnicodeString(&KeyName, L"Alias");
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               IDConfigDB,
                               NULL);
    Status = NtOpenKey(&AliasHandle, KEY_READ | KEY_WRITE, &ObjectAttributes);

    /* Check if we failed to open it */
    if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        /* Create it instead */
        Status = NtCreateKey(&AliasHandle,
                             KEY_READ | KEY_WRITE,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             &Disposition);
    }

    /* Check if we failed */
    if (!NT_SUCCESS (Status))
    {
        /* Cleanup and exit */
        AliasHandle = NULL;
        goto Exit;
    }

    /* Loop every alias ID */
    while (AliasId++ < 200)
    {
        /* Build the KeyName */
        sprintf(Buffer, "%04d", AliasId);
        RtlInitAnsiString(&TempString, Buffer);

        /* Convert it to Unicode */
        KeyName.MaximumLength = sizeof(UnicodeBuffer);
        KeyName.Buffer = UnicodeBuffer;
        Status = RtlAnsiStringToUnicodeString(&KeyName,
                                              &TempString,
                                              FALSE);
        ASSERT (STATUS_SUCCESS == Status);

        /* Open the key */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   AliasHandle,
                                   NULL);
        Status = NtOpenKey(&AliasIdHandle,
                           KEY_READ | KEY_WRITE,
                           &ObjectAttributes);
        if (NT_SUCCESS (Status))
        {
            /* We opened it, close and keep looping */
            NtClose(AliasIdHandle);
        }
        else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
        {
            /* We couldn't find it, change Status and break out */
            Status = STATUS_SUCCESS;
            break;
        }
        else
        {
            /* Any other error, break out */
            break;
        }
    }

    /* Check if we failed in the alias loop */
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup and exit */
        AliasIdHandle = 0;
        goto Exit;
    }

    /* Otherwise, create the alias key */
    Status = NtCreateKey(&AliasIdHandle,
                         KEY_READ | KEY_WRITE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         &Disposition);
    if (!NT_SUCCESS(Status))
    {
        /* Cleanup and exit */
        AliasIdHandle = 0;
        goto Exit;
    }

    /* Add docking information */
    CmpAddDockingInfo(AliasIdHandle, ProfileBlock);

    /* Set the profile number */
    Value = ProfileNumber;
    RtlInitUnicodeString(&KeyName, L"ProfileNumber");
    Status = NtSetValueKey(AliasIdHandle,
                           &KeyName,
                           0,
                           REG_DWORD,
                           &Value,
                           sizeof(Value));

Exit:
    /* Close every opened key */
    if (AliasHandle) NtClose(AliasHandle);
    if (AliasIdHandle) NtClose(AliasIdHandle);

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
CmSetAcpiHwProfile(IN PPROFILE_ACPI_DOCKING_STATE NewDockState,
                   IN PVOID Select,
                   IN PVOID Context,
                   OUT PHANDLE NewProfile,
                   OUT PBOOLEAN ProfileChanged)
{
    /* FIXME: TODO */
    *ProfileChanged = FALSE;
    *NewProfile = NULL;
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpCloneHwProfile(IN HANDLE ConfigHandle,
                  IN HANDLE Parent,
                  IN HANDLE OldProfile,
                  IN ULONG OldProfileNumber,
                  IN USHORT DockingState,
                  OUT PHANDLE NewProfile,
                  OUT PULONG NewProfileNumber)
{
    /* FIXME: TODO */
    *NewProfileNumber = FALSE;
    *NewProfile = NULL;
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
CmpCreateCurrentControlSetLink(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNICODE_STRING ConfigName = RTL_CONSTANT_STRING(L"Control\\ConfigHandle");
    UNICODE_STRING SelectName =
        RTL_CONSTANT_STRING(L"\\Registry\\Machine\\System\\Select");
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    CHAR ValueInfoBuffer[128];
    PKEY_VALUE_FULL_INFORMATION ValueInfo;
    CHAR Buffer[128];
    WCHAR UnicodeBuffer[128];
    HANDLE SelectHandle, KeyHandle, ConfigHandle = NULL, ProfileHandle = NULL;
    HANDLE ParentHandle = NULL, AcpiHandle;
    ULONG ControlSet, HwProfile;
    ANSI_STRING TempString;
    NTSTATUS Status;
    ULONG ResultLength, Disposition;
    BOOLEAN AcpiProfile = FALSE;
    PLOADER_PARAMETER_EXTENSION LoaderExtension;
    PROFILE_ACPI_DOCKING_STATE AcpiDockState;
    BOOLEAN Active;
    PAGED_CODE();

    /* Open the select key */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SelectName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = NtOpenKey(&SelectHandle, KEY_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))return(Status);

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
            "\\Registry\\Machine\\System\\ControlSet%03d",
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

    /* Check if there is no hardware profile key */
    if (!NT_SUCCESS (Status))
    {
        /* Exit and clean up */
        ParentHandle = 0;
        goto Cleanup;
    }

    /* Build the profile name */
    sprintf(Buffer, "%04d",HwProfile);
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

    /* Check if there's no such key */
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
        /* Check the hardware profile status */
        switch (LoaderExtension->Profile.Status)
        {
            /* Cloned status */
            case 3:

                /* Clone it */
                Status = CmpCloneHwProfile(ConfigHandle,
                                           ParentHandle,
                                           ProfileHandle,
                                           HwProfile,
                                           LoaderExtension->
                                           Profile.DockingState,
                                           &ProfileHandle,
                                           &HwProfile);
                if (!NT_SUCCESS(Status))
                {
                    /* Cloning failed, cleanup and exit */
                    ProfileHandle = 0;
                    goto Cleanup;
                }

                /* Set the current config key */
                RtlInitUnicodeString(&KeyName, L"CurrentConfig");
                Status = NtSetValueKey(ConfigHandle,
                                       &KeyName,
                                       0,
                                       REG_DWORD,
                                       &HwProfile,
                                       sizeof (HwProfile));
                if (!NT_SUCCESS (Status)) goto Cleanup;

            /* Alias status */
            case 1:

                /* Create an alias entry */
                Status = CmpAddAliasEntry(ConfigHandle,
                                          &LoaderExtension->Profile,
                                          HwProfile);

            /* Docking status */
            case 2:

                /* Create the current dock info key */
                RtlInitUnicodeString(&KeyName,
                                     L"CurrentDockInfo");
                InitializeObjectAttributes(&ObjectAttributes,
                                           &KeyName,
                                           OBJ_CASE_INSENSITIVE,
                                           ConfigHandle,
                                           NULL);
                Status = NtCreateKey(&KeyHandle,
                                     KEY_READ | KEY_WRITE,
                                     &ObjectAttributes,
                                     0,
                                     NULL,
                                     REG_OPTION_VOLATILE,
                                     &Disposition);
                ASSERT (STATUS_SUCCESS == Status);

                /* Add the docking information */
                Status = CmpAddDockingInfo(KeyHandle,
                                           &LoaderExtension->Profile);
                break;

            /* Other cases */
            case 0:
            case 0xC001:
                break;

            /* Unknown status */
            default:
                ASSERT(FALSE);
        }
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
                "Hardware Profiles\\%04d",
                HwProfile);
        RtlInitAnsiString(&TempString, Buffer);

        /* Convert it to Unicode */
        KeyName.MaximumLength = sizeof(UnicodeBuffer);
        KeyName.Buffer = UnicodeBuffer;
        Status = RtlAnsiStringToUnicodeString(&KeyName,
                                              &TempString,
                                              FALSE);
        ASSERT (STATUS_SUCCESS == Status);

        /* Set it */
        Status = NtSetValueKey(KeyHandle,
                               &CmSymbolicLinkValueName,
                               0,
                               REG_LINK,
                               KeyName.Buffer,
                               KeyName.Length);
        NtClose(KeyHandle);
    }

    /* Check if we have to set the ACPI Profile */
    if (AcpiProfile)
    {
        /* Setup the docking state to undocked */
        AcpiDockState.DockingState = 1;
        AcpiDockState.SerialLength = 2;
        AcpiDockState.SerialNumber[0] = L'\0';

        /* Set the ACPI profile */
        Status = CmSetAcpiHwProfile(&AcpiDockState,
                                    CmpHwProfileDefaultSelect,
                                    NULL,
                                    &AcpiHandle,
                                    &Active);
        ASSERT(NT_SUCCESS(Status));

        /* Close the key */
        NtClose(AcpiHandle);
    }

    /* Close every opened handle */
Cleanup:
    if (ConfigHandle) NtClose(ConfigHandle);
    if (ProfileHandle) NtClose(ProfileHandle);
    if (ParentHandle) NtClose(ParentHandle);

    /* Return success */
    return STATUS_SUCCESS;
}

VOID
NTAPI
CmInitSystem1(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    HANDLE KeyHandle;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if this is PE-boot */
    if (ExpIsWinPEMode)
    {
        /* Set registry to PE mode */
        CmpMiniNTBoot = TRUE;
        CmpShareSystemHives = TRUE;
    }

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
    ObjectTypeInitializer.GenericMapping = CmpKeyMapping;
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

