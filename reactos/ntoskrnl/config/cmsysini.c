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

/* GLOBALS *******************************************************************/

HIVE_LIST_ENTRY CmpMachineHiveList[5];

UNICODE_STRING CmSymbolicLinkValueName =
    RTL_CONSTANT_STRING(L"SymbolicLinkValue");

UNICODE_STRING CmpSystemStartOptions;
UNICODE_STRING CmpLoadOptions;

BOOLEAN CmpShareSystemHives;
BOOLEAN CmSelfHeal = TRUE;
BOOLEAN CmpSelfHeal = TRUE;
ULONG CmpBootType;

extern BOOLEAN ExpInTextModeSetup;

/* FUNCTIONS *****************************************************************/

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
                           CmpSystemStartOptions.Buffer,
                           CmpSystemStartOptions.Length);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Free the options now */
    ExFreePool(CmpSystemStartOptions.Buffer);

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
    PEREGISTRY_HIVE SystemHive = NULL;
    UNICODE_STRING HiveName = RTL_CONSTANT_STRING(L"SYSTEM");
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    PAGED_CODE();

    /* Setup the ansi string */
    RtlInitAnsiString(&LoadString, LoaderBlock->LoadOptions);

    /* Allocate the unicode buffer */
    Length = LoadString.Length * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    Buffer = ExAllocatePoolWithTag(PagedPool, Length, 0);
    if (!Buffer)
    {
        /* Fail */
        KEBUGCHECKEX(BAD_SYSTEM_CONFIG_INFO, 3, 1, (ULONG_PTR)LoaderBlock, 0);
    }

    /* Setup the unicode string */
    RtlInitEmptyUnicodeString(&CmpLoadOptions, Buffer, Length);

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
        RtlCreateUnicodeString(&SystemHive->HiveFileName, SYSTEM_REG_FILE);

        /* Set the log filename */
        RtlCreateUnicodeString(&SystemHive->LogFileName, SYSTEM_LOG_FILE);

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
    if (SystemHive) CmpBootType = SystemHive->Hive.HiveHeader->BootType;

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

