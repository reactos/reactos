/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/user.c
 * PURPOSE:     User specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "samsrv.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
SampOpenUserObject(IN PSAM_DB_OBJECT DomainObject,
                   IN ULONG UserId,
                   IN ACCESS_MASK DesiredAccess,
                   OUT PSAM_DB_OBJECT *UserObject)
{
    WCHAR szRid[9];

    TRACE("(%p %lu %lx %p)\n",
          DomainObject, UserId, DesiredAccess, UserObject);

    /* Convert the RID into a string (hex) */
    _swprintf(szRid, L"%08lX", UserId);

    /* Create the user object */
    return SampOpenDbObject(DomainObject,
                            L"Users",
                            szRid,
                            UserId,
                            SamDbUserObject,
                            DesiredAccess,
                            UserObject);
}


NTSTATUS
SampAddGroupMembershipToUser(IN PSAM_DB_OBJECT UserObject,
                             IN ULONG GroupId,
                             IN ULONG Attributes)
{
    PGROUP_MEMBERSHIP GroupsBuffer = NULL;
    ULONG GroupsCount = 0;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    TRACE("(%p %lu %lx)\n",
          UserObject, GroupId, Attributes);

    Status = SampGetObjectAttribute(UserObject,
                                    L"Groups",
                                    NULL,
                                    NULL,
                                    &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND)
        goto done;

    GroupsBuffer = midl_user_allocate(Length + sizeof(GROUP_MEMBERSHIP));
    if (GroupsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Status = SampGetObjectAttribute(UserObject,
                                        L"Groups",
                                        NULL,
                                        GroupsBuffer,
                                        &Length);
        if (!NT_SUCCESS(Status))
            goto done;

        GroupsCount = Length / sizeof(GROUP_MEMBERSHIP);
    }

    for (i = 0; i < GroupsCount; i++)
    {
        if (GroupsBuffer[i].RelativeId == GroupId)
        {
            Status = STATUS_MEMBER_IN_GROUP;
            goto done;
        }
    }

    GroupsBuffer[GroupsCount].RelativeId = GroupId;
    GroupsBuffer[GroupsCount].Attributes = Attributes;
    Length += sizeof(GROUP_MEMBERSHIP);

    Status = SampSetObjectAttribute(UserObject,
                                    L"Groups",
                                    REG_BINARY,
                                    GroupsBuffer,
                                    Length);

done:
    if (GroupsBuffer != NULL)
        midl_user_free(GroupsBuffer);

    return Status;
}


NTSTATUS
SampRemoveGroupMembershipFromUser(IN PSAM_DB_OBJECT UserObject,
                                  IN ULONG GroupId)
{
    PGROUP_MEMBERSHIP GroupsBuffer = NULL;
    ULONG GroupsCount = 0;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status = STATUS_SUCCESS;

    TRACE("(%p %lu)\n",
          UserObject, GroupId);

    SampGetObjectAttribute(UserObject,
                           L"Groups",
                           NULL,
                           NULL,
                           &Length);

    if (Length == 0)
        return STATUS_MEMBER_NOT_IN_GROUP;

    GroupsBuffer = midl_user_allocate(Length);
    if (GroupsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(UserObject,
                                    L"Groups",
                                    NULL,
                                    GroupsBuffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = STATUS_MEMBER_NOT_IN_GROUP;

    GroupsCount = Length / sizeof(GROUP_MEMBERSHIP);
    for (i = 0; i < GroupsCount; i++)
    {
        if (GroupsBuffer[i].RelativeId == GroupId)
        {
            Length -= sizeof(GROUP_MEMBERSHIP);
            Status = STATUS_SUCCESS;

            if (GroupsCount - i - 1 > 0)
            {
                CopyMemory(&GroupsBuffer[i],
                           &GroupsBuffer[i + 1],
                           (GroupsCount - i - 1) * sizeof(GROUP_MEMBERSHIP));
            }

            break;
        }
    }

    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampSetObjectAttribute(UserObject,
                                    L"Groups",
                                    REG_BINARY,
                                    GroupsBuffer,
                                    Length);

done:
    if (GroupsBuffer != NULL)
        midl_user_free(GroupsBuffer);

    return Status;
}


NTSTATUS
SampGetUserGroupAttributes(IN PSAM_DB_OBJECT DomainObject,
                           IN ULONG UserId,
                           IN ULONG GroupId,
                           OUT PULONG GroupAttributes)
{
    PSAM_DB_OBJECT UserObject = NULL;
    PGROUP_MEMBERSHIP GroupsBuffer = NULL;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    Status = SampOpenUserObject(DomainObject,
                                UserId,
                                0,
                                &UserObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    SampGetObjectAttribute(UserObject,
                           L"Groups",
                           NULL,
                           NULL,
                           &Length);

    if (Length == 0)
        return STATUS_UNSUCCESSFUL; /* FIXME */

    GroupsBuffer = midl_user_allocate(Length);
    if (GroupsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(UserObject,
                                    L"Groups",
                                    NULL,
                                    GroupsBuffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    for (i = 0; i < (Length / sizeof(GROUP_MEMBERSHIP)); i++)
    {
        if (GroupsBuffer[i].RelativeId == GroupId)
        {
            *GroupAttributes = GroupsBuffer[i].Attributes;
            goto done;
        }
    }

done:
    if (GroupsBuffer != NULL)
        midl_user_free(GroupsBuffer);

    if (UserObject != NULL)
        SampCloseDbObject(UserObject);

    return Status;
}


NTSTATUS
SampSetUserGroupAttributes(IN PSAM_DB_OBJECT DomainObject,
                           IN ULONG UserId,
                           IN ULONG GroupId,
                           IN ULONG GroupAttributes)
{
    PSAM_DB_OBJECT UserObject = NULL;
    PGROUP_MEMBERSHIP GroupsBuffer = NULL;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    Status = SampOpenUserObject(DomainObject,
                                UserId,
                                0,
                                &UserObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    SampGetObjectAttribute(UserObject,
                           L"Groups",
                           NULL,
                           NULL,
                           &Length);

    if (Length == 0)
        return STATUS_UNSUCCESSFUL; /* FIXME */

    GroupsBuffer = midl_user_allocate(Length);
    if (GroupsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(UserObject,
                                    L"Groups",
                                    NULL,
                                    GroupsBuffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    for (i = 0; i < (Length / sizeof(GROUP_MEMBERSHIP)); i++)
    {
        if (GroupsBuffer[i].RelativeId == GroupId)
        {
            GroupsBuffer[i].Attributes = GroupAttributes;
            break;
        }
    }

    Status = SampSetObjectAttribute(UserObject,
                                    L"Groups",
                                    REG_BINARY,
                                    GroupsBuffer,
                                    Length);

done:
    if (GroupsBuffer != NULL)
        midl_user_free(GroupsBuffer);

    if (UserObject != NULL)
        SampCloseDbObject(UserObject);

    return Status;
}


NTSTATUS
SampRemoveUserFromAllGroups(IN PSAM_DB_OBJECT UserObject)
{
    PGROUP_MEMBERSHIP GroupsBuffer = NULL;
    PSAM_DB_OBJECT GroupObject;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    SampGetObjectAttribute(UserObject,
                           L"Groups",
                           NULL,
                           NULL,
                           &Length);

    if (Length == 0)
        return STATUS_SUCCESS;

    GroupsBuffer = midl_user_allocate(Length);
    if (GroupsBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(UserObject,
                                    L"Groups",
                                    NULL,
                                    GroupsBuffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    for (i = 0; i < (Length / sizeof(GROUP_MEMBERSHIP)); i++)
    {
        Status = SampOpenGroupObject(UserObject->ParentObject,
                                     GroupsBuffer[i].RelativeId,
                                     0,
                                     &GroupObject);
        if (!NT_SUCCESS(Status))
        {
            goto done;
        }

        Status = SampRemoveMemberFromGroup(GroupObject,
                                           UserObject->RelativeId);
        if (Status == STATUS_MEMBER_NOT_IN_GROUP)
            Status = STATUS_SUCCESS;

        SampCloseDbObject(GroupObject);

        if (!NT_SUCCESS(Status))
        {
            goto done;
        }
    }

    /* Remove all groups from the Groups attribute */
    Status = SampSetObjectAttribute(UserObject,
                                    L"Groups",
                                    REG_BINARY,
                                    NULL,
                                    0);

done:
    if (GroupsBuffer != NULL)
        midl_user_free(GroupsBuffer);

    return Status;
}


static
NTSTATUS
SampRemoveUserFromDomainAliases(
    _In_ HANDLE DomainKeyHandle,
    _In_ PSAM_DB_OBJECT UserObject,
    _In_ PWSTR UserSidString)
{
    WCHAR UserIdBuffer[64];
    ULONG Index, NameLength;
    HANDLE AliasesKeyHandle = NULL, AliasesMembersKeyHandle = NULL;
    HANDLE UserSidKeyHandle = NULL, UserIdKeyHandle, UserIdMembersKeyHandle;
    NTSTATUS Status;

    TRACE("SampRemoveUserFromDomainAliases(%p %p)\n", DomainKeyHandle, UserObject);

    Status = SampRegOpenKey(DomainKeyHandle,
                            L"Aliases",
                            KEY_READ,
                            &AliasesKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the Aliases key (Status %08lX)\n", Status);
        goto done;
    }

    Status = SampRegOpenKey(AliasesKeyHandle,
                            L"Members",
                            KEY_READ | KEY_WRITE,
                            &AliasesMembersKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("Failed to open the Members key (Status %08lX)\n", Status);
        goto done;
    }

    Status = SampRegOpenKey(AliasesMembersKeyHandle,
                            UserSidString,
                            KEY_READ,
                            &UserSidKeyHandle);
    if (NT_SUCCESS(Status))
    {
        TRACE("Opend the key for the users SID (%S)!\n", UserSidString);
        Index = 0;
        for (;;)
        {
            NameLength = sizeof(UserIdBuffer);
            Status = SampRegEnumerateValue(UserSidKeyHandle,
                                        Index,
                                        UserIdBuffer,
                                        &NameLength,
                                        NULL,
                                        NULL,
                                        NULL);
            if (!NT_SUCCESS(Status))
            {
                if (Status == STATUS_NO_MORE_ENTRIES)
                    Status = STATUS_SUCCESS;
                break;
            }

            TRACE("User ID: %S\n", UserIdBuffer);

            Status = SampRegOpenKey(AliasesKeyHandle,
                                    UserIdBuffer,
                                    KEY_READ,
                                    &UserIdKeyHandle);
            if (NT_SUCCESS(Status))
            {
                Status = SampRegOpenKey(UserIdKeyHandle,
                                        L"Members",
                                        KEY_READ | KEY_WRITE,
                                        &UserIdMembersKeyHandle);
                if (NT_SUCCESS(Status))
                {
                    Status = SampRegDeleteValue(UserIdMembersKeyHandle,
                                                UserSidString);
                    SampRegCloseKey(&UserIdMembersKeyHandle);

                    if (!NT_SUCCESS(Status))
                        break;
                }

                SampRegCloseKey(&UserIdKeyHandle);
            }

            Index++;
        }

        SampRegCloseKey(&UserSidKeyHandle);
    }
    else
    {
        TRACE("No key for the user SID (%S) here!\n", UserSidString);
        Status = STATUS_SUCCESS;
        goto done;
    }

    Status = SampRegDeleteKey(AliasesMembersKeyHandle,
                              UserSidString);

done:
    SampRegCloseKey(&AliasesMembersKeyHandle);
    SampRegCloseKey(&AliasesKeyHandle);

    return Status;
}


NTSTATUS
SampRemoveUserFromAllAliases(
    _In_ PSAM_DB_OBJECT UserObject)
{
    PSAM_DB_OBJECT DomainObject;
    PSID UserSid = NULL;
    LPWSTR UserSidString = NULL;
    WCHAR DomainKeyName[64];
    HANDLE DomainsKeyHandle = NULL, DomainKeyHandle = NULL;
    ULONG DomainEnumIndex;
    NTSTATUS Status;

    TRACE("SampRemoveUserFromAllAliases(%p)\n", UserObject);

    DomainObject = UserObject->ParentObject;
    if (DomainObject == NULL || DomainObject->ObjectType != SamDbDomainObject)
    {
        ERR("Invalid domain object!\n");
        return STATUS_UNSUCCESSFUL;
    }

    Status = SampCreateAccountSid(DomainObject,
                                  UserObject->RelativeId,
                                  &UserSid);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampCreateAccountSid() failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    ConvertSidToStringSidW(UserSid, &UserSidString);
    TRACE("User SID: %S\n", UserSidString);

    Status = SampRegOpenKey(NULL, //SamKeyHandle,
                            L"\\Registry\\Machine\\SAM\\SAM\\Domains", //L"SAM\\Domains",
                            KEY_READ | KEY_ENUMERATE_SUB_KEYS,
                            &DomainsKeyHandle);
    if (!NT_SUCCESS(Status))
    {
        ERR("SampRegOpenKey() failed (Status 0x%08lx)\n", Status);
        goto done;
    }

    DomainEnumIndex = 0;
    while (TRUE)
    {
        Status = SampRegEnumerateSubKey(DomainsKeyHandle,
                                        DomainEnumIndex,
                                        64 * sizeof(WCHAR),
                                        DomainKeyName);
        if (!NT_SUCCESS(Status))
        {
            Status = STATUS_SUCCESS;
            break;
        }

        TRACE("DomainEnumIndex: %lu\n", DomainEnumIndex);
        TRACE("Domain key name: %S\n", DomainKeyName);

        Status = SampRegOpenKey(DomainsKeyHandle,
                                DomainKeyName,
                                KEY_READ,
                                &DomainKeyHandle);
        if (NT_SUCCESS(Status))
        {
            Status = SampRemoveUserFromDomainAliases(DomainKeyHandle,
                                                     UserObject,
                                                     UserSidString);
            SampRegCloseKey(&DomainKeyHandle);

            if (!NT_SUCCESS(Status))
                break;
        }

        DomainEnumIndex++;
    }

done:
    if (DomainsKeyHandle != NULL)
        SampRegCloseKey(&DomainsKeyHandle);

    if (UserSidString != NULL)
        LocalFree(UserSidString);

    if (UserSid != NULL)
        RtlFreeHeap(RtlGetProcessHeap(), 0, UserSid);

    return Status;
}


NTSTATUS
SampSetUserPassword(IN PSAM_DB_OBJECT UserObject,
                    IN PENCRYPTED_NT_OWF_PASSWORD NtPassword,
                    IN BOOLEAN NtPasswordPresent,
                    IN PENCRYPTED_LM_OWF_PASSWORD LmPassword,
                    IN BOOLEAN LmPasswordPresent)
{
    PENCRYPTED_NT_OWF_PASSWORD NtHistory = NULL;
    PENCRYPTED_LM_OWF_PASSWORD LmHistory = NULL;
    ULONG NtHistoryLength = 0;
    ULONG LmHistoryLength = 0;
    ULONG CurrentHistoryLength;
    ULONG MaxHistoryLength = 3;
    ULONG Length = 0;
    BOOLEAN UseNtPassword;
    BOOLEAN UseLmPassword;
    NTSTATUS Status;

    UseNtPassword =
       ((NtPasswordPresent != FALSE) &&
        (NtPassword != NULL) &&
        (memcmp(NtPassword, &EmptyNtHash, sizeof(ENCRYPTED_NT_OWF_PASSWORD)) != 0));

    UseLmPassword =
       ((LmPasswordPresent != FALSE) &&
        (LmPassword != NULL) &&
        (memcmp(LmPassword, &EmptyLmHash, sizeof(ENCRYPTED_LM_OWF_PASSWORD)) != 0));

    /* Update the NT password history only if we have a new non-empty NT password */
    if (UseNtPassword)
    {
        /* Get the size of the NT history */
        SampGetObjectAttribute(UserObject,
                               L"NTPwdHistory",
                               NULL,
                               NULL,
                               &Length);

        CurrentHistoryLength = Length / sizeof(ENCRYPTED_NT_OWF_PASSWORD);
        if (CurrentHistoryLength < MaxHistoryLength)
        {
            NtHistoryLength = (CurrentHistoryLength + 1) * sizeof(ENCRYPTED_NT_OWF_PASSWORD);
        }
        else
        {
            NtHistoryLength = MaxHistoryLength * sizeof(ENCRYPTED_NT_OWF_PASSWORD);
        }

        /* Allocate the history buffer */
        NtHistory = midl_user_allocate(NtHistoryLength);
        if (NtHistory == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        if (Length > 0)
        {
            /* Get the history */
            Status = SampGetObjectAttribute(UserObject,
                                            L"NTPwdHistory",
                                            NULL,
                                            NtHistory,
                                            &Length);
            if (!NT_SUCCESS(Status))
                goto done;
        }

        /* Move the old passwords down by one entry */
        if (NtHistoryLength > sizeof(ENCRYPTED_NT_OWF_PASSWORD))
        {
            MoveMemory(&(NtHistory[1]),
                       &(NtHistory[0]),
                       NtHistoryLength - sizeof(ENCRYPTED_NT_OWF_PASSWORD));
        }

        /* Add the new password to the top of the history */
        if (NtPasswordPresent)
        {
            CopyMemory(&(NtHistory[0]),
                       NtPassword,
                       sizeof(ENCRYPTED_NT_OWF_PASSWORD));
        }
        else
        {
            ZeroMemory(&(NtHistory[0]),
                       sizeof(ENCRYPTED_NT_OWF_PASSWORD));
        }

        /* Set the history */
        Status = SampSetObjectAttribute(UserObject,
                                        L"NTPwdHistory",
                                        REG_BINARY,
                                        (PVOID)NtHistory,
                                        NtHistoryLength);
        if (!NT_SUCCESS(Status))
            goto done;
    }

    /* Update the LM password history only if we have a new non-empty LM password */
    if (UseLmPassword)
    {
        /* Get the size of the LM history */
        Length = 0;
        SampGetObjectAttribute(UserObject,
                               L"LMPwdHistory",
                               NULL,
                               NULL,
                               &Length);

        CurrentHistoryLength = Length / sizeof(ENCRYPTED_LM_OWF_PASSWORD);
        if (CurrentHistoryLength < MaxHistoryLength)
        {
            LmHistoryLength = (CurrentHistoryLength + 1) * sizeof(ENCRYPTED_LM_OWF_PASSWORD);
        }
        else
        {
            LmHistoryLength = MaxHistoryLength * sizeof(ENCRYPTED_LM_OWF_PASSWORD);
        }

        /* Allocate the history buffer */
        LmHistory = midl_user_allocate(LmHistoryLength);
        if (LmHistory == NULL)
            return STATUS_INSUFFICIENT_RESOURCES;

        if (Length > 0)
        {
            /* Get the history */
            Status = SampGetObjectAttribute(UserObject,
                                            L"LMPwdHistory",
                                            NULL,
                                            LmHistory,
                                            &Length);
            if (!NT_SUCCESS(Status))
                goto done;
        }

        /* Move the old passwords down by one entry */
        if (LmHistoryLength > sizeof(ENCRYPTED_LM_OWF_PASSWORD))
        {
            MoveMemory(&(LmHistory[1]),
                       &(LmHistory[0]),
                       LmHistoryLength - sizeof(ENCRYPTED_LM_OWF_PASSWORD));
        }

        /* Add the new password to the top of the history */
        if (LmPasswordPresent)
        {
            CopyMemory(&(LmHistory[0]),
                       LmPassword,
                       sizeof(ENCRYPTED_LM_OWF_PASSWORD));
        }
        else
        {
            ZeroMemory(&(LmHistory[0]),
                       sizeof(ENCRYPTED_LM_OWF_PASSWORD));
        }

        /* Set the LM password history */
        Status = SampSetObjectAttribute(UserObject,
                                        L"LMPwdHistory",
                                        REG_BINARY,
                                        (PVOID)LmHistory,
                                        LmHistoryLength);
        if (!NT_SUCCESS(Status))
            goto done;
    }

    /* Set the new NT password */
    if (UseNtPassword)
    {
        Status = SampSetObjectAttribute(UserObject,
                                        L"NTPwd",
                                        REG_BINARY,
                                        (PVOID)NtPassword,
                                        sizeof(ENCRYPTED_NT_OWF_PASSWORD));
        if (!NT_SUCCESS(Status))
            goto done;
    }
    else
    {
        Status = SampSetObjectAttribute(UserObject,
                                        L"NTPwd",
                                        REG_BINARY,
                                        &EmptyNtHash,
                                        sizeof(ENCRYPTED_NT_OWF_PASSWORD));
        if (!NT_SUCCESS(Status))
            goto done;
    }

    /* Set the new LM password */
    if (UseLmPassword)
    {
        Status = SampSetObjectAttribute(UserObject,
                                        L"LMPwd",
                                        REG_BINARY,
                                        (PVOID)LmPassword,
                                        sizeof(ENCRYPTED_LM_OWF_PASSWORD));
        if (!NT_SUCCESS(Status))
            goto done;
    }
    else
    {
        Status = SampSetObjectAttribute(UserObject,
                                        L"LMPwd",
                                        REG_BINARY,
                                        &EmptyLmHash,
                                        sizeof(ENCRYPTED_LM_OWF_PASSWORD));
        if (!NT_SUCCESS(Status))
            goto done;
    }

done:
    if (NtHistory != NULL)
        midl_user_free(NtHistory);

    if (LmHistory != NULL)
        midl_user_free(LmHistory);

    return Status;
}


NTSTATUS
SampGetLogonHoursAttribute(IN PSAM_DB_OBJECT UserObject,
                          IN OUT PSAMPR_LOGON_HOURS LogonHours)
{
    PUCHAR RawBuffer = NULL;
    ULONG Length = 0;
    ULONG BufferLength = 0;
    NTSTATUS Status;

    Status = SampGetObjectAttribute(UserObject,
                                    L"LogonHours",
                                    NULL,
                                    NULL,
                                    &Length);
    if (Status != STATUS_BUFFER_OVERFLOW)
    {
        TRACE("SampGetObjectAttribute failed (Status 0x%08lx)\n", Status);
        return Status;
    }

    Status = STATUS_SUCCESS;

    if (Length == 0)
    {
        LogonHours->UnitsPerWeek = 0;
        LogonHours->LogonHours = NULL;
    }
    else
    {
        RawBuffer = midl_user_allocate(Length);
        if (RawBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        Status = SampGetObjectAttribute(UserObject,
                                        L"LogonHours",
                                        NULL,
                                        (PVOID)RawBuffer,
                                        &Length);
        if (!NT_SUCCESS(Status))
            goto done;

        LogonHours->UnitsPerWeek = *((PUSHORT)RawBuffer);

        BufferLength = (((ULONG)LogonHours->UnitsPerWeek) + 7) / 8;

        LogonHours->LogonHours = midl_user_allocate(BufferLength);
        if (LogonHours->LogonHours == NULL)
        {
            TRACE("Failed to allocate LogonHours buffer!\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        memcpy(LogonHours->LogonHours,
               &(RawBuffer[2]),
               BufferLength);
    }

done:

    if (RawBuffer != NULL)
        midl_user_free(RawBuffer);

    return Status;
}


NTSTATUS
SampSetLogonHoursAttribute(IN PSAM_DB_OBJECT UserObject,
                          IN PSAMPR_LOGON_HOURS LogonHours)
{
    PUCHAR RawBuffer = NULL;
    ULONG BufferLength;
    ULONG Length = 0;
    NTSTATUS Status;

    if (LogonHours->UnitsPerWeek > 0)
    {
        BufferLength = (((ULONG)LogonHours->UnitsPerWeek) + 7) / 8;

        Length = BufferLength + sizeof(USHORT);

        RawBuffer = midl_user_allocate(Length);
        if (RawBuffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto done;
        }

        *((PUSHORT)RawBuffer) = LogonHours->UnitsPerWeek;

        memcpy(&(RawBuffer[2]),
               LogonHours->LogonHours,
               BufferLength);
    }

    Status = SampSetObjectAttribute(UserObject,
                                    L"LogonHours",
                                    REG_BINARY,
                                    RawBuffer,
                                    Length);

done:
    if (RawBuffer != NULL)
        midl_user_free(RawBuffer);

    return Status;
}

/* EOF */
