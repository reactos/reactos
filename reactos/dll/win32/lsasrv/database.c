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
LsapCreateDatabaseObjects(VOID)
{
    PLSA_DB_OBJECT DbObject = NULL;

    /* Open the 'Policy' object */
    DbObject = (PLSA_DB_OBJECT)LsapCreateDbObject(NULL,
                                L"Policy",
                                TRUE,
                                LsaDbPolicyObject,
                                0);
    if (DbObject != NULL)
    {
        LsapSetObjectAttribute(DbObject,
                               L"PolPrDmN",
                               NULL,
                               0);

        LsapSetObjectAttribute(DbObject,
                               L"PolPrDmS",
                               NULL,
                               0);

        LsapSetObjectAttribute(DbObject,
                               L"PolAcDmN",
                               NULL,
                               0);

        LsapSetObjectAttribute(DbObject,
                               L"PolAcDmS",
                               NULL,
                               0);


        /* Close the 'Policy' object */
        LsapCloseDbObject((LSAPR_HANDLE)DbObject);
    }

    return STATUS_SUCCESS;
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


LSAPR_HANDLE
LsapCreateDbObject(LSAPR_HANDLE ParentHandle,
                   LPWSTR ObjectName,
                   BOOLEAN Open,
                   LSA_DB_OBJECT_TYPE ObjectType,
                   ACCESS_MASK DesiredAccess)
{
    PLSA_DB_OBJECT ParentObject = (PLSA_DB_OBJECT)ParentHandle;
    PLSA_DB_OBJECT DbObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ParentKeyHandle;
    HANDLE ObjectKeyHandle;
    NTSTATUS Status;

    if (ParentHandle != NULL)
        ParentKeyHandle = ParentObject->KeyHandle;
    else
        ParentKeyHandle = SecurityKeyHandle;

    RtlInitUnicodeString(&KeyName,
                         ObjectName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               ParentKeyHandle,
                               NULL);

    if (Open == TRUE)
    {
        Status = NtOpenKey(&ObjectKeyHandle,
                           KEY_ALL_ACCESS,
                           &ObjectAttributes);
    }
    else
    {
        Status = NtCreateKey(&ObjectKeyHandle,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             0,
                             NULL);
    }

    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    DbObject = (PLSA_DB_OBJECT)RtlAllocateHeap(RtlGetProcessHeap(),
                                               0,
                                               sizeof(LSA_DB_OBJECT));
    if (DbObject == NULL)
    {
        NtClose(ObjectKeyHandle);
        return NULL;
    }

    DbObject->Signature = LSAP_DB_SIGNATURE;
    DbObject->RefCount = 0;
    DbObject->ObjectType = ObjectType;
    DbObject->Access = DesiredAccess;
    DbObject->KeyHandle = ObjectKeyHandle;
    DbObject->ParentObject = ParentObject;

    if (ParentObject != NULL)
        ParentObject->RefCount++;

    return (LSAPR_HANDLE)DbObject;
}


NTSTATUS
LsapValidateDbObject(LSAPR_HANDLE Handle,
                     LSA_DB_OBJECT_TYPE ObjectType,
                     ACCESS_MASK GrantedAccess)
{
    PLSA_DB_OBJECT DbObject = (PLSA_DB_OBJECT)Handle;
    BOOLEAN bValid = FALSE;

    _SEH2_TRY
    {
        if (DbObject->Signature == LSAP_DB_SIGNATURE)
        {
            if ((ObjectType == LsaDbIgnoreObject) ||
                (DbObject->ObjectType == ObjectType))
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

    if (GrantedAccess != 0)
    {
        /* FIXME: Check for granted access rights */
    }

    return STATUS_SUCCESS;
}


NTSTATUS
LsapCloseDbObject(LSAPR_HANDLE Handle)
{
    PLSA_DB_OBJECT DbObject = (PLSA_DB_OBJECT)Handle;

    if (DbObject->RefCount != 0)
        return STATUS_UNSUCCESSFUL;

    if (DbObject->ParentObject != NULL)
        DbObject->ParentObject->RefCount--;

    if (DbObject->KeyHandle != NULL)
        NtClose(DbObject->KeyHandle);

    RtlFreeHeap(RtlGetProcessHeap(), 0, DbObject);

    return STATUS_SUCCESS;
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

