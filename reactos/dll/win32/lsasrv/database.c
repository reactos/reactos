/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/lsasrv/database.c
 * PURPOSE:     LSA object database
 * COPYRIGHT:   Copyright 2011 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "lsasrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(lsasrv);


/* GLOBALS *****************************************************************/

static HANDLE SecurityKeyHandle = NULL;


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
    SID_IDENTIFIER_AUTHORITY SystemAuthority = {SECURITY_NT_AUTHORITY};
    LARGE_INTEGER SystemTime;
    PULONG Seed;

    NtQuerySystemTime(&SystemTime);
    Seed = &SystemTime.u.LowPart;

    return RtlAllocateAndInitializeSid(&SystemAuthority,
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
    PLSA_DB_OBJECT PolicyObject = NULL;
    PSID AccountDomainSid = NULL;
    NTSTATUS Status;

    /* Create a random domain SID */
    Status = LsapCreateRandomDomainSid(&AccountDomainSid);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open the 'Policy' object */
    Status = LsapOpenDbObject(NULL,
                              L"Policy",
                              LsaDbPolicyObject,
                              0,
                              &PolicyObject);
    if (!NT_SUCCESS(Status))
        goto done;

    LsapSetObjectAttribute(PolicyObject,
                           L"PolPrDmN",
                           NULL,
                           0);

    LsapSetObjectAttribute(PolicyObject,
                           L"PolPrDmS",
                           NULL,
                           0);

    LsapSetObjectAttribute(PolicyObject,
                           L"PolAcDmN",
                           NULL,
                           0);

    LsapSetObjectAttribute(PolicyObject,
                           L"PolAcDmS",
                           AccountDomainSid,
                           RtlLengthSid(AccountDomainSid));

done:
    if (PolicyObject != NULL)
        LsapCloseDbObject(PolicyObject);

    if (AccountDomainSid != NULL)
        RtlFreeSid(AccountDomainSid);

    return Status;
}


static NTSTATUS
LsapUpdateDatabase(VOID)
{
    return STATUS_SUCCESS;
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

    TRACE("LsapInitDatabase() done\n");

    return STATUS_SUCCESS;
}


NTSTATUS
LsapCreateDbObject(IN PLSA_DB_OBJECT ParentObject,
                   IN LPWSTR ObjectName,
                   IN LSA_DB_OBJECT_TYPE ObjectType,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PLSA_DB_OBJECT *DbObject)
{
    PLSA_DB_OBJECT NewObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ParentKeyHandle;
    HANDLE ObjectKeyHandle;
    NTSTATUS Status;

    if (DbObject == NULL)
        return STATUS_INVALID_PARAMETER;

    if (ParentObject == NULL)
        ParentKeyHandle = SecurityKeyHandle;
    else
        ParentKeyHandle = ParentObject->KeyHandle;

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

    if (ParentObject != NULL)
        ParentObject->RefCount++;

    *DbObject = NewObject;

    return STATUS_SUCCESS;
}


NTSTATUS
LsapOpenDbObject(IN PLSA_DB_OBJECT ParentObject,
                 IN LPWSTR ObjectName,
                 IN LSA_DB_OBJECT_TYPE ObjectType,
                 IN ACCESS_MASK DesiredAccess,
                 OUT PLSA_DB_OBJECT *DbObject)
{
    PLSA_DB_OBJECT NewObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ParentKeyHandle;
    HANDLE ObjectKeyHandle;
    NTSTATUS Status;

    if (DbObject == NULL)
        return STATUS_INVALID_PARAMETER;

    if (ParentObject == NULL)
        ParentKeyHandle = SecurityKeyHandle;
    else
        ParentKeyHandle = ParentObject->KeyHandle;

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

        return Status;
    }

    Status = RtlpNtSetValueKey(AttributeKey,
                               REG_NONE,
                               AttributeData,
                               AttributeSize);

    NtClose(AttributeKey);

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

/* EOF */

