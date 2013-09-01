/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/alias.c
 * PURPOSE:     Alias specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);


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

/* EOF */
