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
                   IN ULONG RelativeId,
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

        if (ObjectType == SamDbAliasObject)
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
                                HEAP_ZERO_MEMORY,
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
    if (NewObject->Name == NULL)
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
    NewObject->RelativeId = RelativeId;
    NewObject->ParentObject = ParentObject;

    if (ParentObject != NULL)
        NewObject->Trusted = ParentObject->Trusted;

    *DbObject = NewObject;

    return STATUS_SUCCESS;
}


NTSTATUS
SampOpenDbObject(IN PSAM_DB_OBJECT ParentObject,
                 IN LPWSTR ContainerName,
                 IN LPWSTR ObjectName,
                 IN ULONG RelativeId,
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

        if (ObjectType == SamDbAliasObject)
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
                                HEAP_ZERO_MEMORY,
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
    if (NewObject->Name == NULL)
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
    NewObject->RelativeId = RelativeId;
    NewObject->ParentObject = ParentObject;

    if (ParentObject != NULL)
        NewObject->Trusted = ParentObject->Trusted;

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

    return Status;
}


NTSTATUS
SampDeleteAccountDbObject(PSAM_DB_OBJECT DbObject)
{
    LPCWSTR ContainerName;
    LPWSTR AccountName = NULL;
    HKEY ContainerKey;
    HKEY NamesKey;
    ULONG Length = 0;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%p)\n", DbObject);

    /* Server and Domain objects cannot be deleted */
    switch (DbObject->ObjectType)
    {
        case SamDbAliasObject:
            ContainerName = L"Aliases";
            break;

        case SamDbGroupObject:
            ContainerName = L"Groups";
            break;

        case SamDbUserObject:
            ContainerName = L"Users";
            break;

        default:
           return STATUS_INVALID_PARAMETER;
    }

    /* Get the account name */
    Status = SampGetObjectAttribute(DbObject,
                                    L"Name",
                                    NULL,
                                    NULL,
                                    &Length);
    if (Status != STATUS_BUFFER_OVERFLOW)
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    AccountName = RtlAllocateHeap(RtlGetProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  Length);
    if (AccountName == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(DbObject,
                                    L"Name",
                                    NULL,
                                    (PVOID)AccountName,
                                    &Length);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampGetObjectAttribute failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    if (DbObject->KeyHandle != NULL)
        NtClose(DbObject->KeyHandle);

    if (DbObject->ObjectType == SamDbAliasObject)
    {
        if (DbObject->MembersKeyHandle != NULL)
            NtClose(DbObject->MembersKeyHandle);

        SampRegDeleteKey(DbObject->KeyHandle,
                         L"Members");
    }

    /* Open the domain container key */
    Status = SampRegOpenKey(DbObject->ParentObject->KeyHandle,
                            ContainerName,
                            DELETE | KEY_SET_VALUE,
                            &ContainerKey);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegOpenKey failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Open the Names key */
    Status = SampRegOpenKey(ContainerKey,
                            L"Names",
                            KEY_SET_VALUE,
                            &NamesKey);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegOpenKey failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Remove the account from the Names key */
    Status = SampRegDeleteValue(NamesKey,
                                AccountName);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegDeleteValue failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Remove the account key from the container */
    Status = SampRegDeleteKey(ContainerKey,
                              DbObject->Name);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegDeleteKey failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Release the database object name */
    if (DbObject->Name != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, DbObject->Name);

    /* Release the database object */
    RtlFreeHeap(RtlGetProcessHeap(), 0, DbObject);

    Status = STATUS_SUCCESS;

done:
    if (NamesKey != NULL)
        SampRegCloseKey(NamesKey);

    if (ContainerKey != NULL)
        SampRegCloseKey(ContainerKey);

    if (AccountName != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, AccountName);

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

    String->Length = (USHORT)(Length - sizeof(WCHAR));
    String->MaximumLength = (USHORT)Length;
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

