/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/domain.c
 * PURPOSE:     Domain specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);


/* FUNCTIONS ***************************************************************/

NTSTATUS
SampSetAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                           IN LPCWSTR lpContainerName,
                           IN LPCWSTR lpAccountName,
                           IN ULONG ulRelativeId)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE NamesKeyHandle = NULL;
    NTSTATUS Status;

    TRACE("SampSetAccountNameInDomain()\n");

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
    RtlInitUnicodeString(&ValueName, lpAccountName);

    Status = NtSetValueKey(NamesKeyHandle,
                           &ValueName,
                           0,
                           REG_DWORD,
                           (LPVOID)&ulRelativeId,
                           sizeof(ULONG));

done:
    if (NamesKeyHandle)
        NtClose(NamesKeyHandle);

    if (ContainerKeyHandle)
        NtClose(ContainerKeyHandle);

    return Status;
}


NTSTATUS
SampRemoveAccountNameFromDomain(IN PSAM_DB_OBJECT DomainObject,
                                IN LPCWSTR lpContainerName,
                                IN LPCWSTR lpAccountName)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    HANDLE ContainerKeyHandle = NULL;
    HANDLE NamesKeyHandle = NULL;
    NTSTATUS Status;

    TRACE("(%S %S)\n", lpContainerName, lpAccountName);

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
                       KEY_SET_VALUE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Delete the account name value */
    Status = SampRegDeleteValue(NamesKeyHandle,
                                lpAccountName);

done:
    if (NamesKeyHandle)
        NtClose(NamesKeyHandle);

    if (ContainerKeyHandle)
        NtClose(ContainerKeyHandle);

    return Status;
}


NTSTATUS
SampCheckAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                             IN LPCWSTR lpAccountName)
{
    HANDLE AccountKey;
    HANDLE NamesKey;
    NTSTATUS Status;

    TRACE("SampCheckAccountNameInDomain()\n");

    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            L"Aliases",
                            KEY_READ,
                            &AccountKey);
    if (NT_SUCCESS(Status))
    {
        Status = SampRegOpenKey(AccountKey,
                                L"Names",
                                KEY_READ,
                                &NamesKey);
        if (NT_SUCCESS(Status))
        {
            Status = SampRegQueryValue(NamesKey,
                                       lpAccountName,
                                       NULL,
                                       NULL,
                                       NULL);
            if (Status == STATUS_SUCCESS)
            {
                SampRegCloseKey(NamesKey);
                Status = STATUS_ALIAS_EXISTS;
            }
            else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }

        SampRegCloseKey(AccountKey);
    }

    if (!NT_SUCCESS(Status))
    {
        TRACE("Checking for alias account failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            L"Groups",
                            KEY_READ,
                            &AccountKey);
    if (NT_SUCCESS(Status))
    {
        Status = SampRegOpenKey(AccountKey,
                                L"Names",
                                KEY_READ,
                                &NamesKey);
        if (NT_SUCCESS(Status))
        {
            Status = SampRegQueryValue(NamesKey,
                                       lpAccountName,
                                       NULL,
                                       NULL,
                                       NULL);
            if (Status == STATUS_SUCCESS)
            {
                SampRegCloseKey(NamesKey);
                Status = STATUS_ALIAS_EXISTS;
            }
            else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }

        SampRegCloseKey(AccountKey);
    }

    if (!NT_SUCCESS(Status))
    {
        TRACE("Checking for group account failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            L"Users",
                            KEY_READ,
                            &AccountKey);
    if (NT_SUCCESS(Status))
    {
        Status = SampRegOpenKey(AccountKey,
                                L"Names",
                                KEY_READ,
                                &NamesKey);
        if (NT_SUCCESS(Status))
        {
            Status = SampRegQueryValue(NamesKey,
                                       lpAccountName,
                                       NULL,
                                       NULL,
                                       NULL);
            if (Status == STATUS_SUCCESS)
            {
                SampRegCloseKey(NamesKey);
                Status = STATUS_ALIAS_EXISTS;
            }
            else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }

        SampRegCloseKey(AccountKey);
    }

    if (!NT_SUCCESS(Status))
    {
        TRACE("Checking for user account failed (Status 0x%08lx)\n", Status);
    }

    return Status;
}

/* EOF */
