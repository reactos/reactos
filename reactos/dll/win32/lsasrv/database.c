/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/database.c
 * PURPOSE:     LSA object database
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

#include "lsasrv.h"

/* GLOBALS *****************************************************************/

static HANDLE SecurityKeyHandle = NULL;

SID_IDENTIFIER_AUTHORITY NullSidAuthority    = {SECURITY_NULL_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY WorldSidAuthority   = {SECURITY_WORLD_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY LocalSidAuthority   = {SECURITY_LOCAL_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY CreatorSidAuthority = {SECURITY_CREATOR_SID_AUTHORITY};
SID_IDENTIFIER_AUTHORITY NtAuthority         = {SECURITY_NT_AUTHORITY};

PSID BuiltinDomainSid = NULL;
PSID AccountDomainSid = NULL;
UNICODE_STRING BuiltinDomainName = {0, 0, NULL};
UNICODE_STRING AccountDomainName = {0, 0, NULL};


/* FUNCTIONS ***************************************************************/

static NTSTATUS
LsapOpenServiceKey(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\SECURITY");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = RtlpNtOpenKey(&SecurityKeyHandle,
                           KEY_READ | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                           &ObjectAttributes,
                           0);

    return Status;
}


static BOOLEAN
LsapIsDatabaseInstalled(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyName,
                         L"Policy");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               SecurityKeyHandle,
                               NULL);

    Status = RtlpNtOpenKey(&KeyHandle,
                           KEY_READ,
                           &ObjectAttributes,
                           0);
    if (!NT_SUCCESS(Status))
        return FALSE;

    NtClose(KeyHandle);

    return TRUE;
}


static NTSTATUS
LsapCreateDatabaseKeys(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE PolicyKeyHandle = NULL;
    HANDLE AccountsKeyHandle = NULL;
    HANDLE DomainsKeyHandle = NULL;
    HANDLE SecretsKeyHandle = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("LsapInstallDatabase()\n");

    /* Create the 'Policy' key */
    RtlInitUnicodeString(&KeyName,
                         L"Policy");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               SecurityKeyHandle,
                               NULL);

    Status = NtCreateKey(&PolicyKeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create the 'Policy' key (Status: 0x%08lx)\n", Status);
        goto Done;
    }

    /* Create the 'Accounts' key */
    RtlInitUnicodeString(&KeyName,
                         L"Accounts");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               PolicyKeyHandle,
                               NULL);

    Status = NtCreateKey(&AccountsKeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create the 'Accounts' key (Status: 0x%08lx)\n", Status);
        goto Done;
    }

    /* Create the 'Domains' key */
    RtlInitUnicodeString(&KeyName,
                         L"Domains");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               PolicyKeyHandle,
                               NULL);

    Status = NtCreateKey(&DomainsKeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create the 'Domains' key (Status: 0x%08lx)\n", Status);
        goto Done;
    }

    /* Create the 'Secrets' key */
    RtlInitUnicodeString(&KeyName,
                         L"Secrets");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               PolicyKeyHandle,
                               NULL);

    Status = NtCreateKey(&SecretsKeyHandle,
                         KEY_ALL_ACCESS,
                         &ObjectAttributes,
                         0,
                         NULL,
                         0,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to create the 'Secrets' key (Status: 0x%08lx)\n", Status);
        goto Done;
    }

Done:
    if (SecretsKeyHandle != NULL)
        NtClose(SecretsKeyHandle);

    if (DomainsKeyHandle != NULL)
        NtClose(DomainsKeyHandle);

    if (AccountsKeyHandle != NULL)
        NtClose(AccountsKeyHandle);

    if (PolicyKeyHandle != NULL)
        NtClose(PolicyKeyHandle);

    TRACE("LsapInstallDatabase() done (Status: 0x%08lx)\n", Status);

    return Status;
}


static NTSTATUS
LsapCreateRandomDomainSid(OUT PSID *Sid)
{
    LARGE_INTEGER SystemTime;
    PULONG Seed;

    NtQuerySystemTime(&SystemTime);
    Seed = &SystemTime.u.LowPart;

    return RtlAllocateAndInitializeSid(&NtAuthority,
                                       4,
                                       SECURITY_NT_NON_UNIQUE,
                                       RtlUniform(Seed),
                                       RtlUniform(Seed),
                                       RtlUniform(Seed),
                                       SECURITY_NULL_RID,
                                       SECURITY_NULL_RID,
                                       SECURITY_NULL_RID,
                                       SECURITY_NULL_RID,
                                       Sid);
}


static NTSTATUS
LsapCreateDatabaseObjects(VOID)
{
    PLSAP_POLICY_AUDIT_EVENTS_DATA AuditEventsInfo = NULL;
    POLICY_DEFAULT_QUOTA_INFO QuotaInfo;
    POLICY_MODIFICATION_INFO ModificationInfo;
    POLICY_AUDIT_FULL_QUERY_INFO AuditFullInfo = {FALSE, FALSE};
    POLICY_AUDIT_LOG_INFO AuditLogInfo;
    GUID DnsDomainGuid;
    PLSA_DB_OBJECT PolicyObject = NULL;
    PSID AccountDomainSid = NULL;
    PSECURITY_DESCRIPTOR PolicySd = NULL;
    ULONG PolicySdSize = 0;
    ULONG i;
    NTSTATUS Status;

    /* Initialize the default quota limits */
    QuotaInfo.QuotaLimits.PagedPoolLimit = 0x2000000;
    QuotaInfo.QuotaLimits.NonPagedPoolLimit = 0x100000;
    QuotaInfo.QuotaLimits.MinimumWorkingSetSize = 0x10000;
    QuotaInfo.QuotaLimits.MaximumWorkingSetSize = 0xF000000;
    QuotaInfo.QuotaLimits.PagefileLimit = 0;
    QuotaInfo.QuotaLimits.TimeLimit.QuadPart = 0;

    /* Initialize the audit log attribute */
    AuditLogInfo.AuditLogPercentFull = 0;
    AuditLogInfo.MaximumLogSize = 0;                    // DWORD
    AuditLogInfo.AuditRetentionPeriod.QuadPart = 0;     // LARGE_INTEGER
    AuditLogInfo.AuditLogFullShutdownInProgress = 0;    // BYTE
    AuditLogInfo.TimeToShutdown.QuadPart = 0;           // LARGE_INTEGER
    AuditLogInfo.NextAuditRecordId = 0;                 // DWORD

    /* Initialize the Audit Events attribute */
    AuditEventsInfo = RtlAllocateHeap(RtlGetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      sizeof(LSAP_POLICY_AUDIT_EVENTS_DATA));
    if (AuditEventsInfo == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    AuditEventsInfo->AuditingMode = FALSE;
    AuditEventsInfo->MaximumAuditEventCount = POLICY_AUDIT_EVENT_TYPE_COUNT;
    for (i = 0; i < POLICY_AUDIT_EVENT_TYPE_COUNT; i++)
        AuditEventsInfo->AuditEvents[i] = 0;

    /* Initialize the DNS Domain GUID attribute */
    RtlZeroMemory(&DnsDomainGuid, sizeof(DnsDomainGuid));

    /* Initialize the modification attribute */
    ModificationInfo.ModifiedId.QuadPart = 0;
    NtQuerySystemTime(&ModificationInfo.DatabaseCreationTime);

    /* Create a random domain SID */
    Status = LsapCreateRandomDomainSid(&AccountDomainSid);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = LsapCreatePolicySd(&PolicySd, &PolicySdSize);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Open the 'Policy' object */
    Status = LsapOpenDbObject(NULL,
                              NULL,
                              L"Policy",
                              LsaDbPolicyObject,
                              0,
                              TRUE,
                              &PolicyObject);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the Primary Domain Name attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolPrDmN",
                           NULL,
                           0);

    /* Set the Primary Domain SID attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolPrDmS",
                           NULL,
                           0);

    /* Set the Account Domain Name attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolAcDmN",
                           NULL,
                           0);

    /* Set the Account Domain SID attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolAcDmS",
                           AccountDomainSid,
                           RtlLengthSid(AccountDomainSid));

    /* Set the default quota limits attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"DefQuota",
                           &QuotaInfo,
                           sizeof(QuotaInfo));

    /* Set the modification attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolMod",
                           &ModificationInfo,
                           sizeof(ModificationInfo));

    /* Set the audit full attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolAdtFl",
                           &AuditFullInfo,
                           sizeof(AuditFullInfo));

    /* Set the audit log attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolAdtLg",
                           &AuditLogInfo,
                           sizeof(AuditLogInfo));

    /* Set the audit events attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolAdtEv",
                           AuditEventsInfo,
                           sizeof(*AuditEventsInfo));

    /* Set the DNS Domain Name attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolDnDDN",
                           NULL,
                           0);

    /* Set the DNS Forest Name attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolDnTrN",
                           NULL,
                           0);

    /* Set the DNS Domain GUID attribute */
    LsapSetObjectAttribute(PolicyObject,
                           L"PolDnDmG",
                           &DnsDomainGuid,
                           sizeof(DnsDomainGuid));

    /* Set the Security Descriptor */
    LsapSetObjectAttribute(PolicyObject,
                           L"SecDesc",
                           PolicySd,
                           PolicySdSize);

done:
    if (AuditEventsInfo != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AuditEventsInfo);

    if (PolicyObject != NULL)
        LsapCloseDbObject(PolicyObject);

    if (AccountDomainSid != NULL)
        RtlFreeSid(AccountDomainSid);

    if (PolicySd != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, PolicySd);

    return Status;
}


static NTSTATUS
LsapUpdateDatabase(VOID)
{
    return STATUS_SUCCESS;
}


static NTSTATUS
LsapGetDomainInfo(VOID)
{
    PLSA_DB_OBJECT PolicyObject = NULL;
    PUNICODE_STRING DomainName = NULL;
    ULONG AttributeSize;
    LPWSTR SidString = NULL;
    NTSTATUS Status;

    /* Get the built-in domain SID and name */
    Status = RtlAllocateAndInitializeSid(&NtAuthority,
                                         1,
                                         SECURITY_BUILTIN_DOMAIN_RID,
                                         0, 0, 0, 0, 0, 0, 0,
                                         &BuiltinDomainSid);
    if (!NT_SUCCESS(Status))
        return Status;

    /**/
    RtlInitUnicodeString(&BuiltinDomainName,
                         L"BUILTIN");

    /* Open the 'Policy' object */
    Status = LsapOpenDbObject(NULL,
                              NULL,
                              L"Policy",
                              LsaDbPolicyObject,
                              0,
                              TRUE,
                              &PolicyObject);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the account domain SID */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAcDmS",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
        goto done;

    if (AttributeSize > 0)
    {
        AccountDomainSid = RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           AttributeSize);
        if (AccountDomainSid == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolAcDmS",
                                        AccountDomainSid,
                                        &AttributeSize);
        if (!NT_SUCCESS(Status))
            goto done;
    }

    /* Get the account domain name */
    AttributeSize = 0;
    Status = LsapGetObjectAttribute(PolicyObject,
                                    L"PolAcDmN",
                                    NULL,
                                    &AttributeSize);
    if (!NT_SUCCESS(Status))
        goto done;

    if (AttributeSize > 0)
    {
        DomainName = RtlAllocateHeap(RtlGetProcessHeap(),
                                     HEAP_ZERO_MEMORY,
                                     AttributeSize);
        if (DomainName == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        Status = LsapGetObjectAttribute(PolicyObject,
                                        L"PolAcDmN",
                                        DomainName,
                                        &AttributeSize);
        if (!NT_SUCCESS(Status))
            goto done;

        DomainName->Buffer = (LPWSTR)((ULONG_PTR)DomainName + (ULONG_PTR)DomainName->Buffer);

        AccountDomainName.Length = DomainName->Length;
        AccountDomainName.MaximumLength = DomainName->Length + sizeof(WCHAR);
        AccountDomainName.Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                                   HEAP_ZERO_MEMORY,
                                                   AccountDomainName.MaximumLength);
        if (AccountDomainName.Buffer == NULL)
        {
            ERR("Failed to allocate the account domain name buffer\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        RtlCopyMemory(AccountDomainName.Buffer,
                      DomainName->Buffer,
                      DomainName->Length);
    }

    ConvertSidToStringSidW(BuiltinDomainSid, &SidString);
    TRACE("Builtin Domain SID: %S\n", SidString);
    LocalFree(SidString);
    SidString = NULL;

    TRACE("Builtin Domain Name: %wZ\n", &BuiltinDomainName);

    ConvertSidToStringSidW(AccountDomainSid, &SidString);
    TRACE("Account Domain SID: %S\n", SidString);
    LocalFree(SidString);
    SidString = NULL;

    TRACE("Account Domain Name: %wZ\n", &AccountDomainName);

done:
    if (DomainName != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainName);

    if (PolicyObject != NULL)
        LsapCloseDbObject(PolicyObject);

    return Status;
}


NTSTATUS
LsapInitDatabase(VOID)
{
    NTSTATUS Status;

    TRACE("LsapInitDatabase()\n");

    Status = LsapOpenServiceKey();
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the service key (Status: 0x%08lx)\n", Status);
        return Status;
    }

    if (!LsapIsDatabaseInstalled())
    {
        Status = LsapCreateDatabaseKeys();
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to create the LSA database keys (Status: 0x%08lx)\n", Status);
            return Status;
        }

        Status = LsapCreateDatabaseObjects();
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to create the LSA database objects (Status: 0x%08lx)\n", Status);
            return Status;
        }
    }
    else
    {
        Status = LsapUpdateDatabase();
        if (!NT_SUCCESS(Status))
        {
            ERR("Failed to update the LSA database (Status: 0x%08lx)\n", Status);
            return Status;
        }
    }

    Status = LsapGetDomainInfo();
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to get the domain information (Status: 0x%08lx)\n", Status);
        return Status;
    }

    TRACE("LsapInitDatabase() done\n");

    return STATUS_SUCCESS;
}


NTSTATUS
LsapCreateDbObject(IN PLSA_DB_OBJECT ParentObject,
                   IN LPWSTR ContainerName,
                   IN LPWSTR ObjectName,
                   IN LSA_DB_OBJECT_TYPE ObjectType,
                   IN ACCESS_MASK DesiredAccess,
                   IN BOOLEAN Trusted,
                   OUT PLSA_DB_OBJECT *DbObject)
{
    PLSA_DB_OBJECT NewObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ParentKeyHandle;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE ObjectKeyHandle = NULL;
    NTSTATUS Status;

    if (DbObject == NULL)
        return STATUS_INVALID_PARAMETER;

    if (ParentObject == NULL)
        ParentKeyHandle = SecurityKeyHandle;
    else
        ParentKeyHandle = ParentObject->KeyHandle;

    if (ContainerName != NULL)
    {
        /* Open the container key */
        RtlInitUnicodeString(&KeyName,
                             ContainerName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ParentKeyHandle,
                                   NULL);

        Status = NtOpenKey(&ContainerKeyHandle,
                           KEY_ALL_ACCESS,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Open the object key */
        RtlInitUnicodeString(&KeyName,
                             ObjectName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ContainerKeyHandle,
                                   NULL);

        Status = NtCreateKey(&ObjectKeyHandle,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             NULL);

        NtClose(ContainerKeyHandle);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        RtlInitUnicodeString(&KeyName,
                             ObjectName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ParentKeyHandle,
                                   NULL);

        Status = NtCreateKey(&ObjectKeyHandle,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    NewObject = RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                sizeof(LSA_DB_OBJECT));
    if (NewObject == NULL)
    {
        NtClose(ObjectKeyHandle);
        return STATUS_NO_MEMORY;
    }

    NewObject->Signature = LSAP_DB_SIGNATURE;
    NewObject->RefCount = 1;
    NewObject->ObjectType = ObjectType;
    NewObject->Access = DesiredAccess;
    NewObject->KeyHandle = ObjectKeyHandle;
    NewObject->ParentObject = ParentObject;
    NewObject->Trusted = Trusted;

    if (ParentObject != NULL)
        ParentObject->RefCount++;

    *DbObject = NewObject;

    return STATUS_SUCCESS;
}


NTSTATUS
LsapOpenDbObject(IN PLSA_DB_OBJECT ParentObject,
                 IN LPWSTR ContainerName,
                 IN LPWSTR ObjectName,
                 IN LSA_DB_OBJECT_TYPE ObjectType,
                 IN ACCESS_MASK DesiredAccess,
                 IN BOOLEAN Trusted,
                 OUT PLSA_DB_OBJECT *DbObject)
{
    PLSA_DB_OBJECT NewObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ParentKeyHandle;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE ObjectKeyHandle = NULL;
    NTSTATUS Status;

    if (DbObject == NULL)
        return STATUS_INVALID_PARAMETER;

    if (ParentObject == NULL)
        ParentKeyHandle = SecurityKeyHandle;
    else
        ParentKeyHandle = ParentObject->KeyHandle;

    if (ContainerName != NULL)
    {
        /* Open the container key */
        RtlInitUnicodeString(&KeyName,
                             ContainerName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ParentKeyHandle,
                                   NULL);

        Status = NtOpenKey(&ContainerKeyHandle,
                           KEY_ALL_ACCESS,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        /* Open the object key */
        RtlInitUnicodeString(&KeyName,
                             ObjectName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ContainerKeyHandle,
                                   NULL);

        Status = NtOpenKey(&ObjectKeyHandle,
                           KEY_ALL_ACCESS,
                           &ObjectAttributes);

        NtClose(ContainerKeyHandle);

        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }
    else
    {
        /* Open the object key */
        RtlInitUnicodeString(&KeyName,
                             ObjectName);

        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   ParentKeyHandle,
                                   NULL);

        Status = NtOpenKey(&ObjectKeyHandle,
                           KEY_ALL_ACCESS,
                           &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }
    }

    NewObject = RtlAllocateHeap(RtlGetProcessHeap(),
                                0,
                                sizeof(LSA_DB_OBJECT));
    if (NewObject == NULL)
    {
        NtClose(ObjectKeyHandle);
        return STATUS_NO_MEMORY;
    }

    NewObject->Signature = LSAP_DB_SIGNATURE;
    NewObject->RefCount = 1;
    NewObject->ObjectType = ObjectType;
    NewObject->Access = DesiredAccess;
    NewObject->KeyHandle = ObjectKeyHandle;
    NewObject->ParentObject = ParentObject;
    NewObject->Trusted = Trusted;

    if (ParentObject != NULL)
        ParentObject->RefCount++;

    *DbObject = NewObject;

    return STATUS_SUCCESS;
}


NTSTATUS
LsapValidateDbObject(LSAPR_HANDLE Handle,
                     LSA_DB_OBJECT_TYPE ObjectType,
                     ACCESS_MASK DesiredAccess,
                     PLSA_DB_OBJECT *DbObject)
{
    PLSA_DB_OBJECT LocalObject = (PLSA_DB_OBJECT)Handle;
    BOOLEAN bValid = FALSE;

    _SEH2_TRY
    {
        if (LocalObject->Signature == LSAP_DB_SIGNATURE)
        {
            if ((ObjectType == LsaDbIgnoreObject) ||
                (LocalObject->ObjectType == ObjectType))
                bValid = TRUE;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        bValid = FALSE;
    }
    _SEH2_END;

    if (bValid == FALSE)
        return STATUS_INVALID_HANDLE;

    if (DesiredAccess != 0)
    {
        /* Check for granted access rights */
        if ((LocalObject->Access & DesiredAccess) != DesiredAccess)
        {
            ERR("LsapValidateDbObject access check failed %08lx  %08lx\n",
                LocalObject->Access, DesiredAccess);
            return STATUS_ACCESS_DENIED;
        }
    }

    if (DbObject != NULL)
        *DbObject = LocalObject;

    return STATUS_SUCCESS;
}


NTSTATUS
LsapCloseDbObject(PLSA_DB_OBJECT DbObject)
{
    PLSA_DB_OBJECT ParentObject = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    DbObject->RefCount--;

    if (DbObject->RefCount > 0)
        return STATUS_SUCCESS;

    if (DbObject->KeyHandle != NULL)
        NtClose(DbObject->KeyHandle);

    if (DbObject->ParentObject != NULL)
        ParentObject = DbObject->ParentObject;

    RtlFreeHeap(RtlGetProcessHeap(), 0, DbObject);

    if (ParentObject != NULL)
    {
        ParentObject->RefCount--;

        if (ParentObject->RefCount == 0)
            Status = LsapCloseDbObject(ParentObject);
    }

    return Status;
}


NTSTATUS
LsapDeleteDbObject(IN PLSA_DB_OBJECT DbObject)
{
    PLSA_DB_OBJECT ParentObject = NULL;
    WCHAR KeyName[64];
    ULONG Index;
    NTSTATUS Status = STATUS_SUCCESS;

    DbObject->RefCount--;

    if (DbObject->RefCount > 0)
        return STATUS_SUCCESS;

    if (DbObject->KeyHandle != NULL)
    {
        Index = 0;

        while (TRUE)
        {
            Status = LsapRegEnumerateSubKey(DbObject->KeyHandle,
                                            Index,
                                            sizeof(KeyName),
                                            KeyName);
            if (!NT_SUCCESS(Status))
                break;

            TRACE("Index: %lu\n", Index);
            TRACE("Key name: %S\n", KeyName);

            Status = LsapRegDeleteSubKey(DbObject->KeyHandle,
                                         KeyName);
            if (!NT_SUCCESS(Status))
                break;
        }

        if (Status == STATUS_NO_MORE_ENTRIES)
            Status = STATUS_SUCCESS;

        LsapRegDeleteKey(DbObject->KeyHandle);

        NtClose(DbObject->KeyHandle);
    }

    if (DbObject->ParentObject != NULL)
        ParentObject = DbObject->ParentObject;

    RtlFreeHeap(RtlGetProcessHeap(), 0, DbObject);

    if (ParentObject != NULL)
    {
        ParentObject->RefCount--;

        if (ParentObject->RefCount == 0)
            Status = LsapCloseDbObject(ParentObject);
    }

    return Status;
}


NTSTATUS
LsapSetObjectAttribute(PLSA_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       LPVOID AttributeData,
                       ULONG AttributeSize)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE AttributeKey;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyName,
                         AttributeName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               DbObject->KeyHandle,
                               NULL);

    Status = NtCreateKey(&AttributeKey,
                         KEY_SET_VALUE,
                         &ObjectAttributes,
                         0,
                         NULL,
                         REG_OPTION_NON_VOLATILE,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        ERR("NtCreateKey failed for '%S' with status 0x%lx\n",
            AttributeName, Status);
        return Status;
    }

    Status = RtlpNtSetValueKey(AttributeKey,
                               REG_NONE,
                               AttributeData,
                               AttributeSize);

    NtClose(AttributeKey);

    if (!NT_SUCCESS(Status))
    {
        ERR("RtlpNtSetValueKey failed for '%S' with status 0x%lx\n",
            AttributeName, Status);
    }

    return Status;
}


NTSTATUS
LsapGetObjectAttribute(PLSA_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       LPVOID AttributeData,
                       PULONG AttributeSize)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE AttributeKey;
    ULONG ValueSize;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyName,
                         AttributeName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               DbObject->KeyHandle,
                               NULL);

    Status = NtOpenKey(&AttributeKey,
                       KEY_QUERY_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    ValueSize = *AttributeSize;
    Status = RtlpNtQueryValueKey(AttributeKey,
                                 NULL,
                                 NULL,
                                 &ValueSize,
                                 0);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        goto Done;
    }

    if (AttributeData == NULL || *AttributeSize == 0)
    {
        *AttributeSize = ValueSize;
        Status = STATUS_SUCCESS;
        goto Done;
    }
    else if (*AttributeSize < ValueSize)
    {
        *AttributeSize = ValueSize;
        Status = STATUS_BUFFER_OVERFLOW;
        goto Done;
    }

    Status = RtlpNtQueryValueKey(AttributeKey,
                                 NULL,
                                 AttributeData,
                                 &ValueSize,
                                 0);
    if (NT_SUCCESS(Status))
    {
        *AttributeSize = ValueSize;
    }

Done:
    NtClose(AttributeKey);

    return Status;
}


NTSTATUS
LsapDeleteObjectAttribute(PLSA_DB_OBJECT DbObject,
                          LPWSTR AttributeName)
{
    return LsapRegDeleteSubKey(DbObject->KeyHandle,
                               AttributeName);
}

/* EOF */
