/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/database.c
 * PURPOSE:     SAM object database
 * COPYRIGHT:   Copyright 2012 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);


/* GLOBALS *****************************************************************/

static HANDLE SamKeyHandle = NULL;


/* FUNCTIONS ***************************************************************/

static NTSTATUS
SampOpenSamKey(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyName,
                         L"\\Registry\\Machine\\SAM");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = RtlpNtOpenKey(&SamKeyHandle,
                           KEY_READ | KEY_CREATE_SUB_KEY | KEY_ENUMERATE_SUB_KEYS,
                           &ObjectAttributes,
                           0);

    return Status;
}


NTSTATUS
SampInitDatabase(VOID)
{
    NTSTATUS Status;

    TRACE("SampInitDatabase()\n");

    Status = SampOpenSamKey();
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the SAM key (Status: 0x%08lx)\n", Status);
        return Status;
    }

#if 0
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
#endif

    TRACE("SampInitDatabase() done\n");

    return STATUS_SUCCESS;
}


NTSTATUS
SampCreateDbObject(IN PSAM_DB_OBJECT ParentObject,
                   IN LPWSTR ContainerName,
                   IN LPWSTR ObjectName,
                   IN SAM_DB_OBJECT_TYPE ObjectType,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PSAM_DB_OBJECT *DbObject)
{
    PSAM_DB_OBJECT NewObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ParentKeyHandle;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE ObjectKeyHandle = NULL;
    HANDLE MembersKeyHandle = NULL;
    NTSTATUS Status;

    if (DbObject == NULL)
        return STATUS_INVALID_PARAMETER;

    if (ParentObject == NULL)
        ParentKeyHandle = SamKeyHandle;
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

        if ((ObjectType == SamDbAliasObject) ||
            (ObjectType == SamDbGroupObject))
        {
            /* Open the object key */
            RtlInitUnicodeString(&KeyName,
                                 L"Members");

            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                       ContainerKeyHandle,
                                       NULL);

            Status = NtCreateKey(&MembersKeyHandle,
                                 KEY_ALL_ACCESS,
                                 &ObjectAttributes,
                                 0,
                                 NULL,
                                 0,
                                 NULL);
        }

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
                                sizeof(SAM_DB_OBJECT));
    if (NewObject == NULL)
    {
        if (MembersKeyHandle != NULL)
            NtClose(MembersKeyHandle);
        NtClose(ObjectKeyHandle);
        return STATUS_NO_MEMORY;
    }

    NewObject->Name = RtlAllocateHeap(RtlGetProcessHeap(),
                                      0,
                                      (wcslen(ObjectName) + 1) * sizeof(WCHAR));
    if (NewObject == NULL)
    {
        if (MembersKeyHandle != NULL)
            NtClose(MembersKeyHandle);
        NtClose(ObjectKeyHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NewObject);
        return STATUS_NO_MEMORY;
    }

    wcscpy(NewObject->Name, ObjectName);

    NewObject->Signature = SAMP_DB_SIGNATURE;
    NewObject->RefCount = 1;
    NewObject->ObjectType = ObjectType;
    NewObject->Access = DesiredAccess;
    NewObject->KeyHandle = ObjectKeyHandle;
    NewObject->MembersKeyHandle = MembersKeyHandle;
    NewObject->ParentObject = ParentObject;

    if (ParentObject != NULL)
        ParentObject->RefCount++;

    *DbObject = NewObject;

    return STATUS_SUCCESS;
}


NTSTATUS
SampOpenDbObject(IN PSAM_DB_OBJECT ParentObject,
                 IN LPWSTR ContainerName,
                 IN LPWSTR ObjectName,
                 IN SAM_DB_OBJECT_TYPE ObjectType,
                 IN ACCESS_MASK DesiredAccess,
                 OUT PSAM_DB_OBJECT *DbObject)
{
    PSAM_DB_OBJECT NewObject;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ParentKeyHandle;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE ObjectKeyHandle = NULL;
    HANDLE MembersKeyHandle = NULL;
    NTSTATUS Status;

    if (DbObject == NULL)
        return STATUS_INVALID_PARAMETER;

    if (ParentObject == NULL)
        ParentKeyHandle = SamKeyHandle;
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

        if ((ObjectType == SamDbAliasObject) ||
            (ObjectType == SamDbGroupObject))
        {
            /* Open the object key */
            RtlInitUnicodeString(&KeyName,
                                 L"Members");

            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                                       ContainerKeyHandle,
                                       NULL);

            Status = NtCreateKey(&MembersKeyHandle,
                                 KEY_ALL_ACCESS,
                                 &ObjectAttributes,
                                 0,
                                 NULL,
                                 0,
                                 NULL);
        }

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
                                sizeof(SAM_DB_OBJECT));
    if (NewObject == NULL)
    {
        if (MembersKeyHandle != NULL)
            NtClose(MembersKeyHandle);
        NtClose(ObjectKeyHandle);
        return STATUS_NO_MEMORY;
    }

    NewObject->Name = RtlAllocateHeap(RtlGetProcessHeap(),
                                      0,
                                      (wcslen(ObjectName) + 1) * sizeof(WCHAR));
    if (NewObject == NULL)
    {
        if (MembersKeyHandle != NULL)
            NtClose(MembersKeyHandle);
        NtClose(ObjectKeyHandle);
        RtlFreeHeap(RtlGetProcessHeap(), 0, NewObject);
        return STATUS_NO_MEMORY;
    }

    wcscpy(NewObject->Name, ObjectName);
    NewObject->Signature = SAMP_DB_SIGNATURE;
    NewObject->RefCount = 1;
    NewObject->ObjectType = ObjectType;
    NewObject->Access = DesiredAccess;
    NewObject->KeyHandle = ObjectKeyHandle;
    NewObject->MembersKeyHandle = MembersKeyHandle;
    NewObject->ParentObject = ParentObject;

    if (ParentObject != NULL)
        ParentObject->RefCount++;

    *DbObject = NewObject;

    return STATUS_SUCCESS;
}


NTSTATUS
SampValidateDbObject(SAMPR_HANDLE Handle,
                     SAM_DB_OBJECT_TYPE ObjectType,
                     ACCESS_MASK DesiredAccess,
                     PSAM_DB_OBJECT *DbObject)
{
    PSAM_DB_OBJECT LocalObject = (PSAM_DB_OBJECT)Handle;
    BOOLEAN bValid = FALSE;

    _SEH2_TRY
    {
        if (LocalObject->Signature == SAMP_DB_SIGNATURE)
        {
            if ((ObjectType == SamDbIgnoreObject) ||
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
            ERR("SampValidateDbObject access check failed %08lx  %08lx\n",
                LocalObject->Access, DesiredAccess);
            return STATUS_ACCESS_DENIED;
        }
    }

    if (DbObject != NULL)
        *DbObject = LocalObject;

    return STATUS_SUCCESS;
}


NTSTATUS
SampCloseDbObject(PSAM_DB_OBJECT DbObject)
{
    PSAM_DB_OBJECT ParentObject = NULL;
    NTSTATUS Status = STATUS_SUCCESS;

    DbObject->RefCount--;

    if (DbObject->RefCount > 0)
        return STATUS_SUCCESS;

    if (DbObject->KeyHandle != NULL)
        NtClose(DbObject->KeyHandle);

    if (DbObject->MembersKeyHandle != NULL)
        NtClose(DbObject->MembersKeyHandle);

    if (DbObject->ParentObject != NULL)
        ParentObject = DbObject->ParentObject;

    if (DbObject->Name != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, DbObject->Name);

    RtlFreeHeap(RtlGetProcessHeap(), 0, DbObject);

    if (ParentObject != NULL)
    {
        ParentObject->RefCount--;

        if (ParentObject->RefCount == 0)
            Status = SampCloseDbObject(ParentObject);
    }

    return Status;
}


NTSTATUS
SampSetDbObjectNameAlias(IN PSAM_DB_OBJECT DomainObject,
                         IN LPCWSTR lpContainerName,
                         IN LPCWSTR lpAliasName,
                         IN ULONG ulAliasValue)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE NamesKeyHandle = NULL;
    NTSTATUS Status;

    /* Open the container key */
    RtlInitUnicodeString(&KeyName, lpContainerName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               DomainObject->KeyHandle,
                               NULL);

    Status = NtOpenKey(&ContainerKeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open the 'Names' key */
    RtlInitUnicodeString(&KeyName, L"Names");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               ContainerKeyHandle,
                               NULL);

    Status = NtOpenKey(&NamesKeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the alias value */
    RtlInitUnicodeString(&ValueName, lpAliasName);

    Status = NtSetValueKey(NamesKeyHandle,
                           &ValueName,
                           0,
                           REG_DWORD,
                           (LPVOID)&ulAliasValue,
                           sizeof(ULONG));

done:
    if (NamesKeyHandle)
        NtClose(NamesKeyHandle);

    if (ContainerKeyHandle)
        NtClose(ContainerKeyHandle);

    return Status;
}


NTSTATUS
SampCheckDbObjectNameAlias(IN PSAM_DB_OBJECT DomainObject,
                           IN LPCWSTR lpContainerName,
                           IN LPCWSTR lpAliasName,
                           OUT PBOOL bAliasExists)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE NamesKeyHandle = NULL;
    ULONG BufferLength = sizeof(ULONG);
    NTSTATUS Status;

    /* Open the container key */
    RtlInitUnicodeString(&KeyName, lpContainerName);

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               DomainObject->KeyHandle,
                               NULL);

    Status = NtOpenKey(&ContainerKeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open the 'Names' key */
    RtlInitUnicodeString(&KeyName, L"Names");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               ContainerKeyHandle,
                               NULL);

    Status = NtOpenKey(&NamesKeyHandle,
                       KEY_ALL_ACCESS,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Get the alias value */
    RtlInitUnicodeString(&ValueName, lpAliasName);

    BufferLength += FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    /* Allocate memory for the value */
    ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (ValueInfo == NULL)
        return STATUS_NO_MEMORY;

    /* Query the value */
    Status = ZwQueryValueKey(NamesKeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueInfo,
                             BufferLength,
                             &BufferLength);

    *bAliasExists = (Status != STATUS_OBJECT_NAME_NOT_FOUND);

    Status = STATUS_SUCCESS;

    /* Free the memory and return status */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ValueInfo);

done:
    if (NamesKeyHandle)
        NtClose(NamesKeyHandle);

    if (ContainerKeyHandle)
        NtClose(ContainerKeyHandle);

    return Status;
}


NTSTATUS
SampSetObjectAttribute(PSAM_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       ULONG AttributeType,
                       LPVOID AttributeData,
                       ULONG AttributeSize)
{
    UNICODE_STRING ValueName;

    RtlInitUnicodeString(&ValueName,
                         AttributeName);

    return ZwSetValueKey(DbObject->KeyHandle,
                         &ValueName,
                         0,
                         AttributeType,
                         AttributeData,
                         AttributeSize);
}


NTSTATUS
SampGetObjectAttribute(PSAM_DB_OBJECT DbObject,
                       LPWSTR AttributeName,
                       PULONG AttributeType,
                       LPVOID AttributeData,
                       PULONG AttributeSize)
{
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    UNICODE_STRING ValueName;
    ULONG BufferLength = 0;
    NTSTATUS Status;

    RtlInitUnicodeString(&ValueName,
                         AttributeName);

    if (AttributeSize != NULL)
        BufferLength = *AttributeSize;

    BufferLength += FIELD_OFFSET(KEY_VALUE_PARTIAL_INFORMATION, Data);

    /* Allocate memory for the value */
    ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, BufferLength);
    if (ValueInfo == NULL)
        return STATUS_NO_MEMORY;

    /* Query the value */
    Status = ZwQueryValueKey(DbObject->KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueInfo,
                             BufferLength,
                             &BufferLength);
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
    {
        if (AttributeType != NULL)
            *AttributeType = ValueInfo->Type;

        if (AttributeSize != NULL)
            *AttributeSize = ValueInfo->DataLength;
    }

    /* Check if the caller wanted data back, and we got it */
    if ((NT_SUCCESS(Status)) && (AttributeData != NULL))
    {
        /* Copy it */
        RtlMoveMemory(AttributeData,
                      ValueInfo->Data,
                      ValueInfo->DataLength);
    }

    /* Free the memory and return status */
    RtlFreeHeap(RtlGetProcessHeap(), 0, ValueInfo);

    return Status;
}


NTSTATUS
SampGetObjectAttributeString(PSAM_DB_OBJECT DbObject,
                             LPWSTR AttributeName,
                             RPC_UNICODE_STRING *String)
{
    ULONG Length = 0;
    NTSTATUS Status;

    Status = SampGetObjectAttribute(DbObject,
                                    AttributeName,
                                    NULL,
                                    NULL,
                                    &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    String->Length = Length - sizeof(WCHAR);
    String->MaximumLength = Length;
    String->Buffer = midl_user_allocate(Length);
    if (String->Buffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    TRACE("Length: %lu\n", Length);
    Status = SampGetObjectAttribute(DbObject,
                                    AttributeName,
                                    NULL,
                                    (PVOID)String->Buffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

done:
    if (!NT_SUCCESS(Status))
    {
        if (String->Buffer != NULL)
        {
            midl_user_free(String->Buffer);
            String->Buffer = NULL;
        }
    }

    return Status;
}

/* EOF */

