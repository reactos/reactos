/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/group.c
 * PURPOSE:     Group specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

#include "samsrv.h"

/* FUNCTIONS ***************************************************************/

NTSTATUS
SampOpenGroupObject(IN PSAM_DB_OBJECT DomainObject,
                    IN ULONG GroupId,
                    IN ACCESS_MASK DesiredAccess,
                    OUT PSAM_DB_OBJECT *GroupObject)
{
    WCHAR szRid[9];

    TRACE("(%p %lu %lx %p)\n",
          DomainObject, GroupId, DesiredAccess, GroupObject);

    /* Convert the RID into a string (hex) */
    swprintf(szRid, L"%08lX", GroupId);

    /* Create the user object */
    return SampOpenDbObject(DomainObject,
                            L"Groups",
                            szRid,
                            GroupId,
                            SamDbGroupObject,
                            DesiredAccess,
                            GroupObject);
}


NTSTATUS
SampAddMemberToGroup(IN PSAM_DB_OBJECT GroupObject,
                     IN ULONG MemberId)
{
    PULONG MembersBuffer = NULL;
    ULONG MembersCount = 0;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    Status = SampGetObjectAttribute(GroupObject,
                                    L"Members",
                                    NULL,
                                    NULL,
                                    &Length);
    if (!NT_SUCCESS(Status) && Status != STATUS_OBJECT_NAME_NOT_FOUND)
        goto done;

    MembersBuffer = midl_user_allocate(Length + sizeof(ULONG));
    if (MembersBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        Status = SampGetObjectAttribute(GroupObject,
                                        L"Members",
                                        NULL,
                                        MembersBuffer,
                                        &Length);
        if (!NT_SUCCESS(Status))
            goto done;

        MembersCount = Length / sizeof(ULONG);
    }

    for (i = 0; i < MembersCount; i++)
    {
        if (MembersBuffer[i] == MemberId)
        {
            Status = STATUS_MEMBER_IN_GROUP;
            goto done;
        }
    }

    MembersBuffer[MembersCount] = MemberId;
    Length += sizeof(ULONG);

    Status = SampSetObjectAttribute(GroupObject,
                                    L"Members",
                                    REG_BINARY,
                                    MembersBuffer,
                                    Length);

done:
    if (MembersBuffer != NULL)
        midl_user_free(MembersBuffer);

    return Status;
}


NTSTATUS
SampRemoveMemberFromGroup(IN PSAM_DB_OBJECT GroupObject,
                          IN ULONG MemberId)
{
    PULONG MembersBuffer = NULL;
    ULONG MembersCount = 0;
    ULONG Length = 0;
    ULONG i;
    NTSTATUS Status;

    SampGetObjectAttribute(GroupObject,
                           L"Members",
                           NULL,
                           NULL,
                           &Length);

    if (Length == 0)
        return STATUS_MEMBER_NOT_IN_GROUP;

    MembersBuffer = midl_user_allocate(Length);
    if (MembersBuffer == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto done;
    }

    Status = SampGetObjectAttribute(GroupObject,
                                    L"Members",
                                    NULL,
                                    MembersBuffer,
                                    &Length);
    if (!NT_SUCCESS(Status))
        goto done;

    Status = STATUS_MEMBER_NOT_IN_GROUP;

    MembersCount = Length / sizeof(ULONG);
    for (i = 0; i < MembersCount; i++)
    {
        if (MembersBuffer[i] == MemberId)
        {
            Length -= sizeof(ULONG);
            Status = STATUS_SUCCESS;

            if (MembersCount - i - 1 > 0)
            {
                CopyMemory(&MembersBuffer[i],
                           &MembersBuffer[i + 1],
                           (MembersCount - i - 1) * sizeof(ULONG));
            }

            break;
        }
    }

    if (!NT_SUCCESS(Status))
        goto done;

    Status = SampSetObjectAttribute(GroupObject,
                                    L"Members",
                                    REG_BINARY,
                                    MembersBuffer,
                                    Length);

done:
    if (MembersBuffer != NULL)
        midl_user_free(MembersBuffer);

    return Status;
}

/* EOF */
