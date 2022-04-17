/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/domain.c
 * PURPOSE:     Domain specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "samsrv.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
SampSetAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                           IN LPCWSTR lpContainerName,
                           IN LPCWSTR lpAccountName,
                           IN ULONG ulRelativeId)
{
    HANDLE ContainerKeyHandle = NULL;
    HANDLE NamesKeyHandle = NULL;
    NTSTATUS Status;

    TRACE("SampSetAccountNameInDomain()\n");

    /* Open the container key */
    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            lpContainerName,
                            KEY_ALL_ACCESS,
                            &ContainerKeyHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open the 'Names' key */
    Status = SampRegOpenKey(ContainerKeyHandle,
                            L"Names",
                            KEY_ALL_ACCESS,
                            &NamesKeyHandle);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Set the alias value */
    Status = SampRegSetValue(NamesKeyHandle,
                             lpAccountName,
                             REG_DWORD,
                             (LPVOID)&ulRelativeId,
                             sizeof(ULONG));

done:
    SampRegCloseKey(&NamesKeyHandle);
    SampRegCloseKey(&ContainerKeyHandle);

    return Status;
}


NTSTATUS
SampRemoveAccountNameFromDomain(IN PSAM_DB_OBJECT DomainObject,
                                IN LPCWSTR lpContainerName,
                                IN LPCWSTR lpAccountName)
{
    HANDLE ContainerKeyHandle = NULL;
    HANDLE NamesKeyHandle = NULL;
    NTSTATUS Status;

    TRACE("(%S %S)\n", lpContainerName, lpAccountName);

    /* Open the container key */
    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            lpContainerName,
                            KEY_ALL_ACCESS,
                            &ContainerKeyHandle);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Open the 'Names' key */
    Status = SampRegOpenKey(ContainerKeyHandle,
                            L"Names",
                            KEY_SET_VALUE,
                            &NamesKeyHandle);
    if (!NT_SUCCESS(Status))
        goto done;

    /* Delete the account name value */
    Status = SampRegDeleteValue(NamesKeyHandle,
                                lpAccountName);

done:
    SampRegCloseKey(&NamesKeyHandle);
    SampRegCloseKey(&ContainerKeyHandle);

    return Status;
}


NTSTATUS
SampCheckAccountNameInDomain(IN PSAM_DB_OBJECT DomainObject,
                             IN LPCWSTR lpAccountName)
{
    HANDLE AccountKey = NULL;
    HANDLE NamesKey = NULL;
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
                SampRegCloseKey(&NamesKey);
                Status = STATUS_ALIAS_EXISTS;
            }
            else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }

        SampRegCloseKey(&AccountKey);
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
                SampRegCloseKey(&NamesKey);
                Status = STATUS_GROUP_EXISTS;
            }
            else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }

        SampRegCloseKey(&AccountKey);
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
                SampRegCloseKey(&NamesKey);
                Status = STATUS_USER_EXISTS;
            }
            else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;
        }

        SampRegCloseKey(&AccountKey);
    }

    if (!NT_SUCCESS(Status))
    {
        TRACE("Checking for user account failed (Status 0x%08lx)\n", Status);
    }

    return Status;
}


NTSTATUS
SampRemoveMemberFromAllAliases(IN PSAM_DB_OBJECT DomainObject,
                               IN PRPC_SID MemberSid)
{
    WCHAR AliasKeyName[64];
    LPWSTR MemberSidString = NULL;
    HANDLE AliasesKey = NULL;
    HANDLE MembersKey = NULL;
    HANDLE AliasKey = NULL;
    ULONG Index;
    NTSTATUS Status;

    TRACE("(%p %p)\n", DomainObject, MemberSid);

    ConvertSidToStringSidW(MemberSid, &MemberSidString);
    TRACE("Member SID: %S\n", MemberSidString);

    Status = SampRegOpenKey(DomainObject->KeyHandle,
                            L"Aliases",
                            KEY_READ,
                            &AliasesKey);
    if (NT_SUCCESS(Status))
    {
        Index = 0;
        while (TRUE)
        {
            Status = SampRegEnumerateSubKey(AliasesKey,
                                            Index,
                                            64,
                                            AliasKeyName);
            if (!NT_SUCCESS(Status))
            {
                if (Status == STATUS_NO_MORE_ENTRIES)
                    Status = STATUS_SUCCESS;
                break;
            }

            TRACE("Alias key name: %S\n", AliasKeyName);

            Status = SampRegOpenKey(AliasesKey,
                                    AliasKeyName,
                                    KEY_READ,
                                    &AliasKey);
            if (NT_SUCCESS(Status))
            {
                Status = SampRegOpenKey(AliasKey,
                                        L"Members",
                                        KEY_WRITE,
                                        &MembersKey);
                if (NT_SUCCESS(Status))
                {
                    Status = SampRegDeleteValue(AliasKey,
                                                MemberSidString);

                    SampRegCloseKey(&MembersKey);
                }
                else if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                    Status = STATUS_SUCCESS;

                SampRegCloseKey(&AliasKey);
            }

            Index++;
        }

        Status = SampRegOpenKey(AliasesKey,
                                L"Members",
                                KEY_WRITE,
                                &MembersKey);
        if (NT_SUCCESS(Status))
        {
            Status = SampRegDeleteKey(MembersKey,
                                      MemberSidString);
            if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
                Status = STATUS_SUCCESS;

            SampRegCloseKey(&MembersKey);
        }

        SampRegCloseKey(&AliasesKey);
    }

    if (MemberSidString != NULL)
        LocalFree(MemberSidString);

    return Status;
}


NTSTATUS
SampCreateAccountSid(IN PSAM_DB_OBJECT DomainObject,
                     IN ULONG ulRelativeId,
                     IN OUT PSID *AccountSid)
{
    PSID DomainSid = NULL;
    ULONG Length = 0;
    NTSTATUS Status;

    Status = SampGetObjectAttribute(DomainObject,
                                    L"SID",
                                    NULL,
                                    NULL,
                                    &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    TRACE("Length: %lu\n", Length);

    DomainSid = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length);
    if (DomainSid == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(DomainObject,
                                    L"SID",
                                    NULL,
                                    DomainSid,
                                    &Length);
    if (!NT_SUCCESS(Status))
    {
        TRACE("Status 0x%08lx\n", Status);
        goto done;
    }

    *AccountSid = AppendRidToSid(DomainSid,
                                 ulRelativeId);

done:
    if (DomainSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, DomainSid);

    return Status;
}

/* EOF */
