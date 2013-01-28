/*
 * PROJECT:     Local Security Authority Server DLL
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        dll/win32/samsrv/user.c
 * PURPOSE:     User specific helper functions
 * COPYRIGHT:   Copyright 2013 Eric Kohl
 */

/* INCLUDES ****************************************************************/

#include "samsrv.h"

WINE_DEFAULT_DEBUG_CHANNEL(samsrv);


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
    swprintf(szRid, L"%08lX", UserId);

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

/* EOF */
