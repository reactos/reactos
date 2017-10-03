/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/alias.c
 * PURPOSE:     Alias specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "samsrv.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
SampOpenAliasObject(IN PSAM_DB_OBJECT DomainObject,
                    IN ULONG AliasId,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PSAM_DB_OBJECT *AliasObject)
{
    WCHAR szRid[9];

    TRACE("(%p %lu %lx %p)\n",
          DomainObject, AliasId, DesiredAccess, AliasObject);

    /* Convert the RID into a string (hex) */
    swprintf(szRid, L"%08lX", AliasId);

    /* Create the user object */
    return SampOpenDbObject(DomainObject,
                            L"Aliases",
                            szRid,
                            AliasId,
                            SamDbAliasObject,
                            DesiredAccess,
                            AliasObject);
}


NTSTATUS
SampAddMemberToAlias(IN PSAM_DB_OBJECT AliasObject,
                     IN PRPC_SID MemberId)
{
    LPWSTR MemberIdString = NULL;
    HANDLE MembersKeyHandle = NULL;
    HANDLE MemberKeyHandle = NULL;
    ULONG MemberIdLength;
    NTSTATUS Status;

    TRACE("(%p %p)\n",
          AliasObject, MemberId);

    ConvertSidToStringSidW(MemberId, &MemberIdString);
    TRACE("Member SID: %S\n", MemberIdString);

    MemberIdLength = RtlLengthSid(MemberId);

    Status = SampRegCreateKey(AliasObject->KeyHandle,
                              L"Members",
                              KEY_WRITE,
                              &MembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegCreateKey failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegSetValue(MembersKeyHandle,
                             MemberIdString,
                             REG_BINARY,
                             MemberId,
                             MemberIdLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegSetValue failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegCreateKey(AliasObject->MembersKeyHandle,
                              MemberIdString,
                              KEY_WRITE,
                              &MemberKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegCreateKey failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegSetValue(MemberKeyHandle,
                             AliasObject->Name,
                             REG_BINARY,
                             MemberId,
                             MemberIdLength);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegSetValue failed with status 0x%08lx\n", Status);
        goto done;
    }

done:
    SampRegCloseKey(&MemberKeyHandle);
    SampRegCloseKey(&MembersKeyHandle);

    if (MemberIdString != NULL)
        LocalFree(MemberIdString);

    return Status;
}


NTSTATUS
NTAPI
SampRemoveMemberFromAlias(IN PSAM_DB_OBJECT AliasObject,
                          IN PRPC_SID MemberId)
{
    LPWSTR MemberIdString = NULL;
    HANDLE MembersKeyHandle = NULL;
    HANDLE MemberKeyHandle = NULL;
    ULONG ulValueCount;
    NTSTATUS Status;

    TRACE("(%p %p)\n",
          AliasObject, MemberId);

    ConvertSidToStringSidW(MemberId, &MemberIdString);
    TRACE("Member SID: %S\n", MemberIdString);

    Status = SampRegOpenKey(AliasObject->MembersKeyHandle,
                            MemberIdString,
                            KEY_WRITE | KEY_QUERY_VALUE,
                            &MemberKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegOpenKey failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegDeleteValue(MemberKeyHandle,
                                AliasObject->Name);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegDeleteValue failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegQueryKeyInfo(MemberKeyHandle,
                                 NULL,
                                 &ulValueCount);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegQueryKeyInfo failed with status 0x%08lx\n", Status);
        goto done;
    }

    if (ulValueCount == 0)
    {
        SampRegCloseKey(&MemberKeyHandle);

        Status = SampRegDeleteKey(AliasObject->MembersKeyHandle,
                                  MemberIdString);
        if (!NT_SUCCESS(Status))
        {
            TRACE("SampRegDeleteKey failed with status 0x%08lx\n", Status);
            goto done;
        }
    }

    Status = SampRegOpenKey(AliasObject->KeyHandle,
                            L"Members",
                            KEY_WRITE | KEY_QUERY_VALUE,
                            &MembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegOpenKey failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegDeleteValue(MembersKeyHandle,
                                MemberIdString);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegDeleteValue failed with status 0x%08lx\n", Status);
        goto done;
    }

    Status = SampRegQueryKeyInfo(MembersKeyHandle,
                                 NULL,
                                 &ulValueCount);
    if (!NT_SUCCESS(Status))
    {
        TRACE("SampRegQueryKeyInfo failed with status 0x%08lx\n", Status);
        goto done;
    }

    if (ulValueCount == 0)
    {
        SampRegCloseKey(&MembersKeyHandle);

        Status = SampRegDeleteKey(AliasObject->KeyHandle,
                                  L"Members");
        if (!NT_SUCCESS(Status))
        {
            TRACE("SampRegDeleteKey failed with status 0x%08lx\n", Status);
            goto done;
        }
    }

done:
    SampRegCloseKey(&MemberKeyHandle);
    SampRegCloseKey(&MembersKeyHandle);

    if (MemberIdString != NULL)
        LocalFree(MemberIdString);

    return Status;
}


NTSTATUS
SampGetMembersInAlias(IN PSAM_DB_OBJECT AliasObject,
                      OUT PULONG MemberCount,
                      OUT PSAMPR_SID_INFORMATION *MemberArray)
{
    HANDLE MembersKeyHandle = NULL;
    PSAMPR_SID_INFORMATION Members = NULL;
    ULONG Count = 0;
    ULONG DataLength;
    ULONG Index;
    NTSTATUS Status;

    /* Open the members key of the alias object */
    Status = SampRegOpenKey(AliasObject->KeyHandle,
                            L"Members",
                            KEY_READ,
                            &MembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampRegOpenKey failed with status 0x%08lx\n", Status);
        goto done;
    }

    /* Get the number of members */
    Status = SampRegQueryKeyInfo(MembersKeyHandle,
                                 NULL,
                                 &Count);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampRegQueryKeyInfo failed with status 0x%08lx\n", Status);
        goto done;
    }

    /* Allocate the member array */
    Members = midl_user_allocate(Count * sizeof(SAMPR_SID_INFORMATION));
    if (Members == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    /* Enumerate the members */
    Index = 0;
    while (TRUE)
    {
        /* Get the size of the next SID */
        DataLength = 0;
        Status = SampRegEnumerateValue(MembersKeyHandle,
                                       Index,
                                       NULL,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &DataLength);
        if (!NT_SUCCESS(Status))
        {
            if (Status == STATUS_NO_MORE_ENTRIES)
                Status = STATUS_SUCCESS;
            break;
        }

        /* Allocate a buffer for the SID */
        Members[Index].SidPointer = midl_user_allocate(DataLength);
        if (Members[Index].SidPointer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        /* Read the SID into the buffer */
        Status = SampRegEnumerateValue(MembersKeyHandle,
                                       Index,
                                       NULL,
                                       NULL,
                                       NULL,
                                       (PVOID)Members[Index].SidPointer,
                                       &DataLength);
        if (!NT_SUCCESS(Status))
        {
            goto done;
        }

        Index++;
    }

    if (NT_SUCCESS(Status))
    {
        *MemberCount = Count;
        *MemberArray = Members;
    }

done:
    return Status;
}


NTSTATUS
SampRemoveAllMembersFromAlias(IN PSAM_DB_OBJECT AliasObject)
{
    HANDLE MembersKeyHandle = NULL;
    PSAMPR_SID_INFORMATION MemberArray = NULL;
    ULONG MemberCount = 0;
    ULONG Index;
    NTSTATUS Status;

    TRACE("(%p)\n", AliasObject);

    /* Open the members key of the alias object */
    Status = SampRegOpenKey(AliasObject->KeyHandle,
                            L"Members",
                            KEY_READ,
                            &MembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampRegOpenKey failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Get a list of all members of the alias */
    Status = SampGetMembersInAlias(AliasObject,
                                   &MemberCount,
                                   &MemberArray);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampGetMembersInAlias failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    /* Remove all members from the alias */
    for (Index = 0; Index < MemberCount; Index++)
    {
        Status = SampRemoveMemberFromAlias(AliasObject,
                                           MemberArray[Index].SidPointer);
        if (!NT_SUCCESS(Status))
            goto done;
    }

done:
    if (MemberArray != NULL)
    {
        for (Index = 0; Index < MemberCount; Index++)
        {
            if (MemberArray[Index].SidPointer != NULL)
                midl_user_free(MemberArray[Index].SidPointer);
        }

        midl_user_free(MemberArray);
    }

    SampRegCloseKey(&MembersKeyHandle);

    return Status;
}

/* EOF */
