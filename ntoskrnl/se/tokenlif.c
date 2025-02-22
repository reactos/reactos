/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Access token lifetime management implementation (Creation/Duplication/Filtering)
 * COPYRIGHT:   Copyright David Welch <welch@cwcom.net>
 *              Copyright 2021-2022 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* DEFINES ********************************************************************/

#define SE_TOKEN_DYNAMIC_SLIM 500

/* PRIVATE FUNCTIONS *********************************************************/

/**
 * @brief
 * Internal function responsible for access token object creation in the kernel.
 * A fully created token objected is inserted into the token handle, thus the handle
 * becoming a valid handle to an access token object and ready for use.
 *
 * @param[out] TokenHandle
 * Valid token handle that's ready for use after token creation and object insertion.
 *
 * @param[in] PreviousMode
 * Processor request level mode.
 *
 * @param[in] DesiredAccess
 * Desired access right for the token object to be granted. This kind of access right
 * impacts how the token can be used and who.
 *
 * @param[in] ObjectAttributes
 * Object attributes for the token to be created.
 *
 * @param[in] TokenType
 * Type of token to assign upon creation.
 *
 * @param[in] ImpersonationLevel
 * Security impersonation level of token to assign upon creation.
 *
 * @param[in] AuthenticationId
 * Authentication ID that represents the authentication information of the token.
 *
 * @param[in] ExpirationTime
 * Expiration time of the token to assign. A value of -1 means that the token never
 * expires and its life depends upon the amount of references this token object has.
 *
 * @param[in] User
 * User entry to assign to the token.
 *
 * @param[in] GroupCount
 * The total number of groups count for the token.
 *
 * @param[in] Groups
 * The group entries for the token.
 *
 * @param[in] GroupsLength
 * The length size of the groups array, pointed by the Groups parameter.
 *
 * @param[in] PrivilegeCount
 * The total number of priivleges that the newly created token has.
 *
 * @param[in] Privileges
 * The privileges for the token.
 *
 * @param[in] Owner
 * The main user (or also owner) that represents the token that we create.
 *
 * @param[in] PrimaryGroup
 * The main group that represents the token that we create.
 *
 * @param[in] DefaultDacl
 * A discretionary access control list for the token.
 *
 * @param[in] TokenSource
 * Source (or the origin) of the access token that creates it.
 *
 * @param[in] SystemToken
 * If set to TRUE, the newly created token is a system token and only in charge
 * by the internal system. The function directly returns a pointer to the
 * created token object for system kernel use. Otherwise if set to FALSE, the
 * function inserts the object to a handle making it a regular access token.
 *
 * @return
 * Returns STATUS_SUCCESS if token creation has completed successfully.
 * STATUS_INSUFFICIENT_RESOURCES is returned if the dynamic area of memory of the
 * token hasn't been allocated because of lack of memory resources. A failure
 * NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SepCreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ KPROCESSOR_MODE PreviousMode,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TOKEN_TYPE TokenType,
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _In_ PLUID AuthenticationId,
    _In_ PLARGE_INTEGER ExpirationTime,
    _In_ PSID_AND_ATTRIBUTES User,
    _In_ ULONG GroupCount,
    _In_ PSID_AND_ATTRIBUTES Groups,
    _In_ ULONG GroupsLength,
    _In_ ULONG PrivilegeCount,
    _In_ PLUID_AND_ATTRIBUTES Privileges,
    _In_opt_ PSID Owner,
    _In_ PSID PrimaryGroup,
    _In_opt_ PACL DefaultDacl,
    _In_ PTOKEN_SOURCE TokenSource,
    _In_ BOOLEAN SystemToken)
{
    NTSTATUS Status;
    PTOKEN AccessToken;
    ULONG TokenFlags = 0;
    ULONG PrimaryGroupIndex, DefaultOwnerIndex;
    LUID TokenId;
    LUID ModifiedId;
    PVOID EndMem;
    ULONG PrivilegesLength;
    ULONG UserGroupsLength;
    ULONG VariableLength;
    ULONG DynamicPartSize, TotalSize;
    ULONG TokenPagedCharges;
    ULONG i;

    PAGED_CODE();

    /* Loop all groups */
    for (i = 0; i < GroupCount; i++)
    {
        /* Check for mandatory groups */
        if (Groups[i].Attributes & SE_GROUP_MANDATORY)
        {
            /* Force them to be enabled */
            Groups[i].Attributes |= (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT);
        }

        /* Check of the group is an admin group */
        if (RtlEqualSid(SeAliasAdminsSid, Groups[i].Sid))
        {
            /* Remember this so we can optimize queries later */
            TokenFlags |= TOKEN_HAS_ADMIN_GROUP;
        }
    }

    /* Allocate unique IDs for the token */
    ExAllocateLocallyUniqueId(&TokenId);
    ExAllocateLocallyUniqueId(&ModifiedId);

    /* Compute how much size we need to allocate for the token */

    /* Privileges size */
    PrivilegesLength = PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
    PrivilegesLength = ALIGN_UP_BY(PrivilegesLength, sizeof(PVOID));

    /* User and groups size */
    UserGroupsLength = (1 + GroupCount) * sizeof(SID_AND_ATTRIBUTES);
    UserGroupsLength += RtlLengthSid(User->Sid);
    for (i = 0; i < GroupCount; i++)
    {
        UserGroupsLength += RtlLengthSid(Groups[i].Sid);
    }
    UserGroupsLength = ALIGN_UP_BY(UserGroupsLength, sizeof(PVOID));

    /* Add the additional groups array length */
    UserGroupsLength += ALIGN_UP_BY(GroupsLength, sizeof(PVOID));

    VariableLength = PrivilegesLength + UserGroupsLength;
    TotalSize = FIELD_OFFSET(TOKEN, VariablePart) + VariableLength;

    /*
     * A token is considered slim if it has the default dynamic
     * contents, or in other words, the primary group and ACL.
     * We judge if such contents are default by checking their
     * total size if it's over the range. On Windows this range
     * is 0x1F4 (aka 500). If the size of the whole dynamic contents
     * is over that range then the token is considered fat and
     * the token will be charged the whole of its token body length
     * plus the dynamic size.
     */
    DynamicPartSize = DefaultDacl ? DefaultDacl->AclSize : 0;
    DynamicPartSize += RtlLengthSid(PrimaryGroup);
    if (DynamicPartSize > SE_TOKEN_DYNAMIC_SLIM)
    {
        TokenPagedCharges = DynamicPartSize + TotalSize;
    }
    else
    {
        TokenPagedCharges = SE_TOKEN_DYNAMIC_SLIM + TotalSize;
    }

    Status = ObCreateObject(PreviousMode,
                            SeTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            TotalSize,
                            TokenPagedCharges,
                            0,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObject() failed (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Zero out the buffer and initialize the token */
    RtlZeroMemory(AccessToken, TotalSize);

    AccessToken->TokenId = TokenId;
    AccessToken->TokenType = TokenType;
    AccessToken->ImpersonationLevel = ImpersonationLevel;

    /* Initialise the lock for the access token */
    Status = SepCreateTokenLock(AccessToken);
    if (!NT_SUCCESS(Status))
        goto Quit;

    AccessToken->TokenSource.SourceIdentifier = TokenSource->SourceIdentifier;
    RtlCopyMemory(AccessToken->TokenSource.SourceName,
                  TokenSource->SourceName,
                  sizeof(TokenSource->SourceName));

    AccessToken->ExpirationTime = *ExpirationTime;
    AccessToken->ModifiedId = ModifiedId;
    AccessToken->DynamicCharged = TokenPagedCharges - TotalSize;

    AccessToken->TokenFlags = TokenFlags & ~TOKEN_SESSION_NOT_REFERENCED;

    /* Copy and reference the logon session */
    AccessToken->AuthenticationId = *AuthenticationId;
    Status = SepRmReferenceLogonSession(&AccessToken->AuthenticationId);
    if (!NT_SUCCESS(Status))
    {
        /* No logon session could be found, bail out */
        DPRINT1("SepRmReferenceLogonSession() failed (Status 0x%lx)\n", Status);
        /* Set the flag for proper cleanup by the delete procedure */
        AccessToken->TokenFlags |= TOKEN_SESSION_NOT_REFERENCED;
        goto Quit;
    }

    /* Insert the referenced logon session into the token */
    Status = SepRmInsertLogonSessionIntoToken(AccessToken);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to insert the logon session into the token, bail out */
        DPRINT1("SepRmInsertLogonSessionIntoToken() failed (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /* Fill in token debug information */
#if DBG
    /*
     * We must determine ourselves that the current
     * process is not the initial CPU one. The initial
     * process is not a "real" process, that is, the
     * Process Manager has not yet been initialized and
     * as a matter of fact we are creating a token before
     * any process gets created by Ps. If it turns out
     * that the current process is the initial CPU process
     * where token creation execution takes place, don't
     * do anything.
     */
    if (PsGetCurrentProcess() != &KiInitialProcess)
    {
        RtlCopyMemory(AccessToken->ImageFileName,
                      PsGetCurrentProcess()->ImageFileName,
                      min(sizeof(AccessToken->ImageFileName), sizeof(PsGetCurrentProcess()->ImageFileName)));

        AccessToken->ProcessCid = PsGetCurrentProcessId();
        AccessToken->ThreadCid = PsGetCurrentThreadId();
    }

    AccessToken->CreateMethod = TOKEN_CREATE_METHOD;
#endif

    /* Assign the data that reside in the token's variable information area */
    AccessToken->VariableLength = VariableLength;
    EndMem = (PVOID)&AccessToken->VariablePart;

    /* Copy the privileges */
    AccessToken->PrivilegeCount = PrivilegeCount;
    AccessToken->Privileges = NULL;
    if (PrivilegeCount > 0)
    {
        AccessToken->Privileges = EndMem;
        EndMem = (PVOID)((ULONG_PTR)EndMem + PrivilegesLength);
        VariableLength -= PrivilegesLength;

        if (PreviousMode != KernelMode)
        {
            _SEH2_TRY
            {
                RtlCopyMemory(AccessToken->Privileges,
                              Privileges,
                              PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
        else
        {
            RtlCopyMemory(AccessToken->Privileges,
                          Privileges,
                          PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
        }

        if (!NT_SUCCESS(Status))
            goto Quit;
    }

    /* Update the privilege flags */
    SepUpdatePrivilegeFlagsToken(AccessToken);

    /* Copy the user and groups */
    AccessToken->UserAndGroupCount = 1 + GroupCount;
    AccessToken->UserAndGroups = EndMem;
    EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];
    VariableLength -= ((ULONG_PTR)EndMem - (ULONG_PTR)AccessToken->UserAndGroups);

    Status = RtlCopySidAndAttributesArray(1,
                                          User,
                                          VariableLength,
                                          &AccessToken->UserAndGroups[0],
                                          EndMem,
                                          &EndMem,
                                          &VariableLength);
    if (!NT_SUCCESS(Status))
        goto Quit;

    Status = RtlCopySidAndAttributesArray(GroupCount,
                                          Groups,
                                          VariableLength,
                                          &AccessToken->UserAndGroups[1],
                                          EndMem,
                                          &EndMem,
                                          &VariableLength);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Find the token primary group and default owner */
    Status = SepFindPrimaryGroupAndDefaultOwner(AccessToken,
                                                PrimaryGroup,
                                                Owner,
                                                &PrimaryGroupIndex,
                                                &DefaultOwnerIndex);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepFindPrimaryGroupAndDefaultOwner failed (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /*
     * Now allocate the token's dynamic information area
     * and set the data. The dynamic part consists of two
     * contents, the primary group SID and the default DACL
     * of the token, in this strict order.
     */
    AccessToken->DynamicPart = ExAllocatePoolWithTag(PagedPool,
                                                     DynamicPartSize,
                                                     TAG_TOKEN_DYNAMIC);
    if (AccessToken->DynamicPart == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Unused memory in the dynamic area */
    AccessToken->DynamicAvailable = 0;

    /*
     * Assign the primary group to the token
     * and put it in the dynamic part as well.
     */
    EndMem = (PVOID)AccessToken->DynamicPart;
    AccessToken->PrimaryGroup = EndMem;
    RtlCopySid(RtlLengthSid(AccessToken->UserAndGroups[PrimaryGroupIndex].Sid),
                            EndMem,
                            AccessToken->UserAndGroups[PrimaryGroupIndex].Sid);
    AccessToken->DefaultOwnerIndex = DefaultOwnerIndex;
    EndMem = (PVOID)((ULONG_PTR)EndMem + RtlLengthSid(AccessToken->UserAndGroups[PrimaryGroupIndex].Sid));

    /*
     * We have assigned a primary group and put it in the
     * dynamic part, now it's time to copy the provided
     * default DACL (if it's provided to begin with) into
     * the DACL field of the token and put it at the end
     * tail of the dynamic part too.
     */
    if (DefaultDacl != NULL)
    {
        AccessToken->DefaultDacl = EndMem;

        RtlCopyMemory(EndMem,
                      DefaultDacl,
                      DefaultDacl->AclSize);
    }

    /* Insert the token only if it's not the system token, otherwise return it directly */
    if (!SystemToken)
    {
        Status = ObInsertObject(AccessToken,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                TokenHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("ObInsertObject() failed (Status 0x%lx)\n", Status);
        }
    }
    else
    {
        /* Return pointer instead of handle */
        *TokenHandle = (HANDLE)AccessToken;
    }

Quit:
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the token, the delete procedure will clean it up */
        ObDereferenceObject(AccessToken);
    }

    return Status;
}

/**
 * @brief
 * Duplicates an access token, from an existing valid token.
 *
 * @param[in] Token
 * Access token to duplicate.
 *
 * @param[in] ObjectAttributes
 * Object attributes for the new token.
 *
 * @param[in] EffectiveOnly
 * If set to TRUE, the function removes all the disabled privileges and groups of the token
 * to duplicate.
 *
 * @param[in] TokenType
 * Type of token.
 *
 * @param[in] Level
 * Security impersonation level of a token.
 *
 * @param[in] PreviousMode
 * The processor request level mode.
 *
 * @param[out] NewAccessToken
 * The duplicated token.
 *
 * @return
 * Returns STATUS_SUCCESS if the token has been duplicated. STATUS_INSUFFICIENT_RESOURCES is returned
 * if memory pool allocation of the dynamic part of the token for duplication has failed due to the lack
 * of memory resources. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SepDuplicateToken(
    _In_ PTOKEN Token,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN EffectiveOnly,
    _In_ TOKEN_TYPE TokenType,
    _In_ SECURITY_IMPERSONATION_LEVEL Level,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PTOKEN* NewAccessToken)
{
    NTSTATUS Status;
    PTOKEN AccessToken;
    PVOID EndMem;
    ULONG PrimaryGroupIndex;
    ULONG VariableLength;
    ULONG DynamicPartSize, TotalSize;
    ULONG PrivilegesIndex, GroupsIndex;

    PAGED_CODE();

    /* Compute how much size we need to allocate for the token */
    VariableLength = Token->VariableLength;
    TotalSize = FIELD_OFFSET(TOKEN, VariablePart) + VariableLength;

    /*
     * Compute how much size we need to allocate
     * the dynamic part of the newly duplicated
     * token.
     */
    DynamicPartSize = Token->DefaultDacl ? Token->DefaultDacl->AclSize : 0;
    DynamicPartSize += RtlLengthSid(Token->PrimaryGroup);

    Status = ObCreateObject(PreviousMode,
                            SeTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            TotalSize,
                            Token->DynamicCharged,
                            TotalSize,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObject() failed (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Zero out the buffer and initialize the token */
    RtlZeroMemory(AccessToken, TotalSize);

    ExAllocateLocallyUniqueId(&AccessToken->TokenId);

    AccessToken->TokenType = TokenType;
    AccessToken->ImpersonationLevel = Level;

    /* Initialise the lock for the access token */
    Status = SepCreateTokenLock(AccessToken);
    if (!NT_SUCCESS(Status))
    {
        ObDereferenceObject(AccessToken);
        return Status;
    }

    /* Copy the immutable fields */
    AccessToken->TokenSource.SourceIdentifier = Token->TokenSource.SourceIdentifier;
    RtlCopyMemory(AccessToken->TokenSource.SourceName,
                  Token->TokenSource.SourceName,
                  sizeof(Token->TokenSource.SourceName));

    AccessToken->AuthenticationId = Token->AuthenticationId;
    AccessToken->ParentTokenId = Token->ParentTokenId;
    AccessToken->ExpirationTime = Token->ExpirationTime;
    AccessToken->OriginatingLogonSession = Token->OriginatingLogonSession;
    AccessToken->DynamicCharged = Token->DynamicCharged;

    /* Lock the source token and copy the mutable fields */
    SepAcquireTokenLockShared(Token);

    AccessToken->SessionId = Token->SessionId;
    AccessToken->ModifiedId = Token->ModifiedId;

    AccessToken->TokenFlags = Token->TokenFlags & ~TOKEN_SESSION_NOT_REFERENCED;

    /* Reference the logon session */
    Status = SepRmReferenceLogonSession(&AccessToken->AuthenticationId);
    if (!NT_SUCCESS(Status))
    {
        /* No logon session could be found, bail out */
        DPRINT1("SepRmReferenceLogonSession() failed (Status 0x%lx)\n", Status);
        /* Set the flag for proper cleanup by the delete procedure */
        AccessToken->TokenFlags |= TOKEN_SESSION_NOT_REFERENCED;
        goto Quit;
    }

    /* Insert the referenced logon session into the token */
    Status = SepRmInsertLogonSessionIntoToken(AccessToken);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to insert the logon session into the token, bail out */
        DPRINT1("SepRmInsertLogonSessionIntoToken() failed (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /* Fill in token debug information */
#if DBG
    RtlCopyMemory(AccessToken->ImageFileName,
                  PsGetCurrentProcess()->ImageFileName,
                  min(sizeof(AccessToken->ImageFileName), sizeof(PsGetCurrentProcess()->ImageFileName)));

    AccessToken->ProcessCid = PsGetCurrentProcessId();
    AccessToken->ThreadCid = PsGetCurrentThreadId();
    AccessToken->CreateMethod = TOKEN_DUPLICATE_METHOD;
#endif

    /* Assign the data that reside in the token's variable information area */
    AccessToken->VariableLength = VariableLength;
    EndMem = (PVOID)&AccessToken->VariablePart;

    /* Copy the privileges */
    AccessToken->PrivilegeCount = 0;
    AccessToken->Privileges = NULL;
    if (Token->Privileges && (Token->PrivilegeCount > 0))
    {
        ULONG PrivilegesLength = Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
        PrivilegesLength = ALIGN_UP_BY(PrivilegesLength, sizeof(PVOID));

        ASSERT(VariableLength >= PrivilegesLength);

        AccessToken->PrivilegeCount = Token->PrivilegeCount;
        AccessToken->Privileges = EndMem;
        EndMem = (PVOID)((ULONG_PTR)EndMem + PrivilegesLength);
        VariableLength -= PrivilegesLength;

        RtlCopyMemory(AccessToken->Privileges,
                      Token->Privileges,
                      AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
    }

    /* Copy the user and groups */
    AccessToken->UserAndGroupCount = 0;
    AccessToken->UserAndGroups = NULL;
    if (Token->UserAndGroups && (Token->UserAndGroupCount > 0))
    {
        AccessToken->UserAndGroupCount = Token->UserAndGroupCount;
        AccessToken->UserAndGroups = EndMem;
        EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];
        VariableLength -= ((ULONG_PTR)EndMem - (ULONG_PTR)AccessToken->UserAndGroups);

        Status = RtlCopySidAndAttributesArray(AccessToken->UserAndGroupCount,
                                              Token->UserAndGroups,
                                              VariableLength,
                                              AccessToken->UserAndGroups,
                                              EndMem,
                                              &EndMem,
                                              &VariableLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlCopySidAndAttributesArray(UserAndGroups) failed (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /* Find the token primary group */
    Status = SepFindPrimaryGroupAndDefaultOwner(AccessToken,
                                                Token->PrimaryGroup,
                                                NULL,
                                                &PrimaryGroupIndex,
                                                NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepFindPrimaryGroupAndDefaultOwner failed (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /* Copy the restricted SIDs */
    AccessToken->RestrictedSidCount = 0;
    AccessToken->RestrictedSids = NULL;
    if (Token->RestrictedSids && (Token->RestrictedSidCount > 0))
    {
        AccessToken->RestrictedSidCount = Token->RestrictedSidCount;
        AccessToken->RestrictedSids = EndMem;
        EndMem = &AccessToken->RestrictedSids[AccessToken->RestrictedSidCount];
        VariableLength -= ((ULONG_PTR)EndMem - (ULONG_PTR)AccessToken->RestrictedSids);

        Status = RtlCopySidAndAttributesArray(AccessToken->RestrictedSidCount,
                                              Token->RestrictedSids,
                                              VariableLength,
                                              AccessToken->RestrictedSids,
                                              EndMem,
                                              &EndMem,
                                              &VariableLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("RtlCopySidAndAttributesArray(RestrictedSids) failed (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /* Now allocate the token's dynamic information area and set the data */
    AccessToken->DynamicPart = ExAllocatePoolWithTag(PagedPool,
                                                     DynamicPartSize,
                                                     TAG_TOKEN_DYNAMIC);
    if (AccessToken->DynamicPart == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Unused memory in the dynamic area */
    AccessToken->DynamicAvailable = 0;

    /*
     * Assign the primary group to the token
     * and put it in the dynamic part as well.
     */
    EndMem = (PVOID)AccessToken->DynamicPart;
    AccessToken->PrimaryGroup = EndMem;
    RtlCopySid(RtlLengthSid(AccessToken->UserAndGroups[PrimaryGroupIndex].Sid),
                            EndMem,
                            AccessToken->UserAndGroups[PrimaryGroupIndex].Sid);
    AccessToken->DefaultOwnerIndex = Token->DefaultOwnerIndex;
    EndMem = (PVOID)((ULONG_PTR)EndMem + RtlLengthSid(AccessToken->UserAndGroups[PrimaryGroupIndex].Sid));

    /*
     * The existing token has a default DACL only
     * if it has an allocated dynamic part.
     */
    if (Token->DynamicPart && Token->DefaultDacl)
    {
        AccessToken->DefaultDacl = EndMem;

        RtlCopyMemory(EndMem,
                      Token->DefaultDacl,
                      Token->DefaultDacl->AclSize);
    }

    /*
     * Filter the token by removing the disabled privileges
     * and groups if the caller wants to duplicate an access
     * token as effective only.
     */
    if (EffectiveOnly)
    {
        /*
         * Begin querying the groups and search for disabled ones. Do not touch the
         * user which is at the first position because it cannot be disabled, no
         * matter what attributes it has.
         */
        for (GroupsIndex = 1; GroupsIndex < AccessToken->UserAndGroupCount; GroupsIndex++)
        {
            /*
             * A group is considered disabled if its attributes is either
             * 0 or SE_GROUP_ENABLED is not included in the attributes flags list.
             * That is because a certain user and/or group can have several attributes
             * that bear no influence on whether a user/group is enabled or not
             * (SE_GROUP_ENABLED_BY_DEFAULT for example which is a mere indicator
             * that the group has just been enabled by default). A mandatory
             * group (that is, the group has SE_GROUP_MANDATORY attribute)
             * by standards it's always enabled and no one can disable it.
             */
            if (AccessToken->UserAndGroups[GroupsIndex].Attributes == 0 ||
                (AccessToken->UserAndGroups[GroupsIndex].Attributes & SE_GROUP_ENABLED) == 0)
            {
                /*
                 * If this group is an administrators group
                 * and the token belongs to such group,
                 * we've to take away TOKEN_HAS_ADMIN_GROUP
                 * for the fact that's not enabled and as
                 * such the token no longer belongs to
                 * this group.
                 */
                if (RtlEqualSid(SeAliasAdminsSid,
                                &AccessToken->UserAndGroups[GroupsIndex].Sid))
                {
                    AccessToken->TokenFlags &= ~TOKEN_HAS_ADMIN_GROUP;
                }

                /*
                 * A group is not enabled, it's time to remove
                 * from the token and update the groups index
                 * accordingly and continue with the next group.
                 */
                SepRemoveUserGroupToken(AccessToken, GroupsIndex);
                GroupsIndex--;
            }
        }

        /* Begin querying the privileges and search for disabled ones */
        for (PrivilegesIndex = 0; PrivilegesIndex < AccessToken->PrivilegeCount; PrivilegesIndex++)
        {
            /*
             * A privilege is considered disabled if its attributes is either
             * 0 or SE_PRIVILEGE_ENABLED is not included in the attributes flags list.
             * That is because a certain privilege can have several attributes
             * that bear no influence on whether a privilege is enabled or not
             * (SE_PRIVILEGE_ENABLED_BY_DEFAULT for example which is a mere indicator
             * that the privilege has just been enabled by default).
             */
            if (AccessToken->Privileges[PrivilegesIndex].Attributes == 0 ||
                (AccessToken->Privileges[PrivilegesIndex].Attributes & SE_PRIVILEGE_ENABLED) == 0)
            {
                /*
                 * A privilege is not enabled, therefor it's time
                 * to strip it from the token and continue with the next
                 * privilege. Of course we must also want to update the
                 * privileges index accordingly.
                 */
                SepRemovePrivilegeToken(AccessToken, PrivilegesIndex);
                PrivilegesIndex--;
            }
        }
    }

    /* Return the token to the caller */
    *NewAccessToken = AccessToken;
    Status = STATUS_SUCCESS;

Quit:
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the token, the delete procedure will clean it up */
        ObDereferenceObject(AccessToken);
    }

    /* Unlock the source token */
    SepReleaseTokenLock(Token);

    return Status;
}

/**
 * @brief
 * Private helper function responsible for creating a restricted access
 * token, that is, a filtered token from privileges and groups and with
 * restricted SIDs added into the token on demand by the caller.
 *
 * @param[in] Token
 * An existing and valid access token.
 *
 * @param[in] PrivilegesToBeDeleted
 * A list of privileges to be deleted within the token that's going
 * to be filtered. This parameter is ignored if the caller wants to disable
 * all the privileges by specifying DISABLE_MAX_PRIVILEGE in the flags
 * parameter.
 *
 * @param[in] SidsToBeDisabled
 * A list of group SIDs to be disabled within the token. This parameter
 * can be NULL.
 *
 * @param[in] RestrictedSidsIntoToken
 * A list of restricted SIDs to be added into the token. This parameter
 * can be NULL.
 *
 * @param[in] PrivilegesCount
 * The privilege count of the privileges list.
 *
 * @param[in] RegularGroupsSidCount
 * The SIDs count of the group SIDs list.
 *
 * @param[in] RestrictedSidsCount
 * The restricted SIDs count of restricted SIDs list.
 *
 * @param[in] PrivilegeFlags
 * Influences how the privileges should be filtered in an access
 * token. See NtFilterToken syscall for more information.
 *
 * @param[in] PreviousMode
 * Processor level access mode.
 *
 * @param[out] FilteredToken
 * The filtered token, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if token token filtering has completed successfully.
 * STATUS_INVALID_PARAMETER is returned if one or more of the parameters
 * do not meet the conditions imposed by the function. A failure NTSTATUS
 * code is returned otherwise.
 *
 * @remarks
 * The final outcome of privileges and/or SIDs filtering is not always
 * deterministic. That is, any privileges or SIDs that aren't present
 * in the access token are ignored and the function continues with the
 * next privilege or SID to find for filtering. For a fully deterministic
 * outcome the caller is responsible for querying the information details
 * of privileges and SIDs present in the token and then afterwards use
 * such obtained information to do any kind of filtering to the token.
 */
static
NTSTATUS
SepPerformTokenFiltering(
    _In_ PTOKEN Token,
    _In_opt_ PLUID_AND_ATTRIBUTES PrivilegesToBeDeleted,
    _In_opt_ PSID_AND_ATTRIBUTES SidsToBeDisabled,
    _In_opt_ PSID_AND_ATTRIBUTES RestrictedSidsIntoToken,
    _When_(PrivilegesToBeDeleted != NULL, _In_) ULONG PrivilegesCount,
    _When_(SidsToBeDisabled != NULL, _In_) ULONG RegularGroupsSidCount,
    _When_(RestrictedSidsIntoToken != NULL, _In_) ULONG RestrictedSidsCount,
    _In_ ULONG PrivilegeFlags,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PTOKEN *FilteredToken)
{
    NTSTATUS Status;
    PTOKEN AccessToken;
    PVOID EndMem;
    ULONG DynamicPartSize;
    ULONG RestrictedSidsLength;
    ULONG PrivilegesLength;
    ULONG PrimaryGroupIndex;
    ULONG RestrictedSidsInList;
    ULONG RestrictedSidsInToken;
    ULONG VariableLength, TotalSize;
    ULONG PrivsInToken, PrivsInList;
    ULONG GroupsInToken, GroupsInList;
    BOOLEAN WantPrivilegesDisabled;
    BOOLEAN FoundPrivilege;
    BOOLEAN FoundGroup;

    PAGED_CODE();

    /* Ensure that the source token is valid, and lock it */
    ASSERT(Token);
    SepAcquireTokenLockShared(Token);

    /* Assume the caller doesn't want privileges disabled */
    WantPrivilegesDisabled = FALSE;

    /* Assume we haven't found anything */
    FoundPrivilege = FALSE;
    FoundGroup = FALSE;

    /*
     * Take the size that we need for filtered token
     * allocation based upon the existing access token
     * we've been given.
     */
    VariableLength = Token->VariableLength;

    if (RestrictedSidsIntoToken != NULL)
    {
        /*
         * If the caller provided a list of restricted SIDs
         * to be added onto the filtered access token then
         * we must compute the size which is the total space
         * of the current token and the length of the restricted
         * SIDs for the filtered token.
         */
        RestrictedSidsLength = RestrictedSidsCount * sizeof(SID_AND_ATTRIBUTES);
        RestrictedSidsLength += RtlLengthSidAndAttributes(RestrictedSidsCount, RestrictedSidsIntoToken);
        RestrictedSidsLength = ALIGN_UP_BY(RestrictedSidsLength, sizeof(PVOID));

        /*
         * The variable length of the token is not just
         * the actual space length of the existing token
         * but also the sum of the restricted SIDs length.
         */
        VariableLength += RestrictedSidsLength;
        TotalSize = FIELD_OFFSET(TOKEN, VariablePart) + VariableLength + RestrictedSidsLength;
    }
    else
    {
        /* Otherwise the size is of the actual current token */
        TotalSize = FIELD_OFFSET(TOKEN, VariablePart) + VariableLength;
    }

    /*
     * Compute how much size we need to allocate
     * the dynamic part of the newly duplicated
     * token.
     */
    DynamicPartSize = Token->DefaultDacl ? Token->DefaultDacl->AclSize : 0;
    DynamicPartSize += RtlLengthSid(Token->PrimaryGroup);

    /* Set up a filtered token object */
    Status = ObCreateObject(PreviousMode,
                            SeTokenObjectType,
                            NULL,
                            PreviousMode,
                            NULL,
                            TotalSize,
                            Token->DynamicCharged,
                            TotalSize,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepPerformTokenFiltering(): Failed to create the filtered token object (Status 0x%lx)\n", Status);

        /* Unlock the source token and bail out */
        SepReleaseTokenLock(Token);
        return Status;
    }

    /* Initialize the token and begin filling stuff to it */
    RtlZeroMemory(AccessToken, TotalSize);

    /* Set up a lock for the new token */
    Status = SepCreateTokenLock(AccessToken);
    if (!NT_SUCCESS(Status))
        goto Quit;

    /* Allocate new IDs for the token */
    ExAllocateLocallyUniqueId(&AccessToken->TokenId);
    ExAllocateLocallyUniqueId(&AccessToken->ModifiedId);

    /* Copy the type and impersonation level from the token */
    AccessToken->TokenType = Token->TokenType;
    AccessToken->ImpersonationLevel = Token->ImpersonationLevel;

    /* Copy the immutable fields */
    AccessToken->TokenSource.SourceIdentifier = Token->TokenSource.SourceIdentifier;
    RtlCopyMemory(AccessToken->TokenSource.SourceName,
                  Token->TokenSource.SourceName,
                  sizeof(Token->TokenSource.SourceName));

    AccessToken->AuthenticationId = Token->AuthenticationId;
    AccessToken->ParentTokenId = Token->TokenId;
    AccessToken->OriginatingLogonSession = Token->OriginatingLogonSession;
    AccessToken->DynamicCharged = Token->DynamicCharged;

    AccessToken->ExpirationTime = Token->ExpirationTime;

    /* Copy the mutable fields */
    AccessToken->SessionId = Token->SessionId;
    AccessToken->TokenFlags = Token->TokenFlags & ~TOKEN_SESSION_NOT_REFERENCED;

    /* Reference the logon session */
    Status = SepRmReferenceLogonSession(&AccessToken->AuthenticationId);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, bail out*/
        DPRINT1("SepPerformTokenFiltering(): Failed to reference the logon session (Status 0x%lx)\n", Status);
        AccessToken->TokenFlags |= TOKEN_SESSION_NOT_REFERENCED;
        goto Quit;
    }

    /* Insert the referenced logon session into the token */
    Status = SepRmInsertLogonSessionIntoToken(AccessToken);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to insert the logon session into the token, bail out */
        DPRINT1("SepPerformTokenFiltering(): Failed to insert the logon session into token (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /* Fill in token debug information */
#if DBG
    RtlCopyMemory(AccessToken->ImageFileName,
                  PsGetCurrentProcess()->ImageFileName,
                  min(sizeof(AccessToken->ImageFileName), sizeof(PsGetCurrentProcess()->ImageFileName)));

    AccessToken->ProcessCid = PsGetCurrentProcessId();
    AccessToken->ThreadCid = PsGetCurrentThreadId();
    AccessToken->CreateMethod = TOKEN_FILTER_METHOD;
#endif

    /* Assign the data that reside in the token's variable information area */
    AccessToken->VariableLength = VariableLength;
    EndMem = (PVOID)&AccessToken->VariablePart;

    /* Copy the privileges from the existing token */
    AccessToken->PrivilegeCount = 0;
    AccessToken->Privileges = NULL;
    if (Token->Privileges && (Token->PrivilegeCount > 0))
    {
        PrivilegesLength = Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES);
        PrivilegesLength = ALIGN_UP_BY(PrivilegesLength, sizeof(PVOID));

        /*
         * Ensure that the token can actually hold all
         * the privileges from the existing token.
         * Otherwise something's seriously wrong and
         * we've to guard ourselves.
         */
        ASSERT(VariableLength >= PrivilegesLength);

        AccessToken->PrivilegeCount = Token->PrivilegeCount;
        AccessToken->Privileges = EndMem;
        EndMem = (PVOID)((ULONG_PTR)EndMem + PrivilegesLength);
        VariableLength -= PrivilegesLength;

        RtlCopyMemory(AccessToken->Privileges,
                      Token->Privileges,
                      AccessToken->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));
    }

    /* Copy the user and groups */
    AccessToken->UserAndGroupCount = 0;
    AccessToken->UserAndGroups = NULL;
    if (Token->UserAndGroups && (Token->UserAndGroupCount > 0))
    {
        AccessToken->UserAndGroupCount = Token->UserAndGroupCount;
        AccessToken->UserAndGroups = EndMem;
        EndMem = &AccessToken->UserAndGroups[AccessToken->UserAndGroupCount];
        VariableLength -= ((ULONG_PTR)EndMem - (ULONG_PTR)AccessToken->UserAndGroups);

        Status = RtlCopySidAndAttributesArray(AccessToken->UserAndGroupCount,
                                              Token->UserAndGroups,
                                              VariableLength,
                                              AccessToken->UserAndGroups,
                                              EndMem,
                                              &EndMem,
                                              &VariableLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SepPerformTokenFiltering(): Failed to copy the groups into token (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /* Copy the restricted SIDs */
    AccessToken->RestrictedSidCount = 0;
    AccessToken->RestrictedSids = NULL;
    if (Token->RestrictedSids && (Token->RestrictedSidCount > 0))
    {
        AccessToken->RestrictedSidCount = Token->RestrictedSidCount;
        AccessToken->RestrictedSids = EndMem;
        EndMem = &AccessToken->RestrictedSids[AccessToken->RestrictedSidCount];
        VariableLength -= ((ULONG_PTR)EndMem - (ULONG_PTR)AccessToken->RestrictedSids);

        Status = RtlCopySidAndAttributesArray(AccessToken->RestrictedSidCount,
                                              Token->RestrictedSids,
                                              VariableLength,
                                              AccessToken->RestrictedSids,
                                              EndMem,
                                              &EndMem,
                                              &VariableLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SepPerformTokenFiltering(): Failed to copy the restricted SIDs into token (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /*
     * Insert the restricted SIDs into the token on
     * the request by the caller.
     */
    if (RestrictedSidsIntoToken != NULL)
    {
        for (RestrictedSidsInList = 0; RestrictedSidsInList < RestrictedSidsCount; RestrictedSidsInList++)
        {
            /* Did the caller assign attributes to the restricted SIDs? */
            if (RestrictedSidsIntoToken[RestrictedSidsInList].Attributes != 0)
            {
                /* There mustn't be any attributes, bail out */
                DPRINT1("SepPerformTokenFiltering(): There mustn't be any attributes to restricted SIDs!\n");
                Status = STATUS_INVALID_PARAMETER;
                goto Quit;
            }
        }

        /*
         * Ensure that the token can hold the restricted SIDs
         * (the variable length is calculated at the beginning
         * of the routine call).
         */
        ASSERT(VariableLength >= RestrictedSidsLength);

        /*
         * Now let's begin inserting the restricted SIDs into the filtered
         * access token from the list the caller gave us.
         */
        AccessToken->RestrictedSidCount = RestrictedSidsCount;
        AccessToken->RestrictedSids = EndMem;
        EndMem = (PVOID)((ULONG_PTR)EndMem + RestrictedSidsLength);
        VariableLength -= RestrictedSidsLength;

        RtlCopyMemory(AccessToken->RestrictedSids,
                      RestrictedSidsIntoToken,
                      AccessToken->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES));

        /*
         * As we've copied the restricted SIDs into
         * the token, we must assign them the following
         * combination of attributes SE_GROUP_ENABLED,
         * SE_GROUP_ENABLED_BY_DEFAULT and SE_GROUP_MANDATORY.
         * With such attributes we estabilish that restricting
         * SIDs into the token are enabled for access checks.
         */
        for (RestrictedSidsInToken = 0; RestrictedSidsInToken < AccessToken->RestrictedSidCount; RestrictedSidsInToken++)
        {
            AccessToken->RestrictedSids[RestrictedSidsInToken].Attributes |= (SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_MANDATORY);
        }

        /*
         * As we added restricted SIDs into the token, mark
         * it as restricted.
         */
        AccessToken->TokenFlags |= TOKEN_IS_RESTRICTED;
    }

    /* Search for the primary group */
    Status = SepFindPrimaryGroupAndDefaultOwner(AccessToken,
                                                Token->PrimaryGroup,
                                                NULL,
                                                &PrimaryGroupIndex,
                                                NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepPerformTokenFiltering(): Failed searching for the primary group (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /* Now allocate the token's dynamic information area and set the data */
    AccessToken->DynamicPart = ExAllocatePoolWithTag(PagedPool,
                                                     DynamicPartSize,
                                                     TAG_TOKEN_DYNAMIC);
    if (AccessToken->DynamicPart == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Unused memory in the dynamic area */
    AccessToken->DynamicAvailable = 0;

    /*
     * Assign the primary group to the token
     * and put it in the dynamic part as well.
     */
    EndMem = (PVOID)AccessToken->DynamicPart;
    AccessToken->PrimaryGroup = EndMem;
    RtlCopySid(RtlLengthSid(AccessToken->UserAndGroups[PrimaryGroupIndex].Sid),
                            EndMem,
                            AccessToken->UserAndGroups[PrimaryGroupIndex].Sid);
    AccessToken->DefaultOwnerIndex = Token->DefaultOwnerIndex;
    EndMem = (PVOID)((ULONG_PTR)EndMem + RtlLengthSid(AccessToken->UserAndGroups[PrimaryGroupIndex].Sid));

    /*
     * The existing token has a default DACL only
     * if it has an allocated dynamic part.
     */
    if (Token->DynamicPart && Token->DefaultDacl)
    {
        AccessToken->DefaultDacl = EndMem;

        RtlCopyMemory(EndMem,
                      Token->DefaultDacl,
                      Token->DefaultDacl->AclSize);
    }

    /*
     * Now figure out what does the caller
     * want with the privileges.
     */
    if (PrivilegeFlags & DISABLE_MAX_PRIVILEGE)
    {
        /*
         * The caller wants them disabled, cache this request
         * for later operations.
         */
        WantPrivilegesDisabled = TRUE;
    }

    if (PrivilegeFlags & SANDBOX_INERT)
    {
        /* The caller wants an inert token, store the TOKEN_SANDBOX_INERT flag now */
        AccessToken->TokenFlags |= TOKEN_SANDBOX_INERT;
    }

    /*
     * Now it's time to filter the token's privileges.
     * Loop all the privileges in the token.
     */
    for (PrivsInToken = 0; PrivsInToken < AccessToken->PrivilegeCount; PrivsInToken++)
    {
        if (WantPrivilegesDisabled)
        {
            /*
             * We got the acknowledgement that the caller wants
             * to disable all the privileges so let's just do it.
             * However, as per the general documentation is stated
             * that only SE_CHANGE_NOTIFY_PRIVILEGE must be kept
             * therefore in that case we must skip this privilege.
             */
            if (AccessToken->Privileges[PrivsInToken].Luid.LowPart == SE_CHANGE_NOTIFY_PRIVILEGE)
            {
                continue;
            }
            else
            {
                /*
                 * The act of disabling privileges actually means
                 * "deleting" them from the access token entirely.
                 * First we must disable them so that we can update
                 * token flags accordingly.
                 */
                AccessToken->Privileges[PrivsInToken].Attributes &= ~SE_PRIVILEGE_ENABLED;
                SepUpdateSinglePrivilegeFlagToken(AccessToken, PrivsInToken);

                /* Remove the privileges now */
                SepRemovePrivilegeToken(AccessToken, PrivsInToken);
                PrivsInToken--;
            }
        }
        else
        {
            if (PrivilegesToBeDeleted != NULL)
            {
                /* Loop the privileges we've got to delete */
                for (PrivsInList = 0; PrivsInList < PrivilegesCount; PrivsInList++)
                {
                    /* Does this privilege exist in the token? */
                    if (RtlEqualLuid(&AccessToken->Privileges[PrivsInToken].Luid,
                                     &PrivilegesToBeDeleted[PrivsInList].Luid))
                    {
                        /* Mark that we found it */
                        FoundPrivilege = TRUE;
                        break;
                    }
                }

                /* Did we find the privilege? */
                if (PrivsInList == PrivilegesCount)
                {
                    /* We didn't, continue with next one */
                    continue;
                }
            }
        }

        /*
         * If we have found the target privilege in the token
         * based on the privileges list given by the caller
         * then begin deleting it.
         */
        if (FoundPrivilege)
        {
            /* Disable the privilege and update the flags */
            AccessToken->Privileges[PrivsInToken].Attributes &= ~SE_PRIVILEGE_ENABLED;
            SepUpdateSinglePrivilegeFlagToken(AccessToken, PrivsInToken);

            /* Delete the privilege */
            SepRemovePrivilegeToken(AccessToken, PrivsInToken);

            /*
             * Adjust the index and reset the FoundPrivilege indicator
             * so that we can continue with the next privilege to delete.
             */
            PrivsInToken--;
            FoundPrivilege = FALSE;
            continue;
        }
    }

    /*
     * Loop the group SIDs that we want to disable as
     * per on the request by the caller.
     */
    if (SidsToBeDisabled != NULL)
    {
        for (GroupsInToken = 0; GroupsInToken < AccessToken->UserAndGroupCount; GroupsInToken++)
        {
            for (GroupsInList = 0; GroupsInList < RegularGroupsSidCount; GroupsInList++)
            {
                /* Does this group SID exist in the token? */
                if (RtlEqualSid(&AccessToken->UserAndGroups[GroupsInToken].Sid,
                                &SidsToBeDisabled[GroupsInList].Sid))
                {
                    /* Mark that we found it */
                    FoundGroup = TRUE;
                    break;
                }
            }

            /* Did we find the group? */
            if (GroupsInList == RegularGroupsSidCount)
            {
                /* We didn't, continue with next one */
                continue;
            }

            /* If we have found the group, disable it */
            if (FoundGroup)
            {
                /*
                 * If the acess token belongs to the administrators
                 * group and this is the target group, we must take
                 * away TOKEN_HAS_ADMIN_GROUP flag from the token.
                 */
                if (RtlEqualSid(SeAliasAdminsSid,
                                &AccessToken->UserAndGroups[GroupsInToken].Sid))
                {
                    AccessToken->TokenFlags &= ~TOKEN_HAS_ADMIN_GROUP;
                }

                /*
                 * If the target group that we have found it is the
                 * owner then from now on it no longer is but the user.
                 * Therefore assign the default owner index as the user.
                 */
                if (AccessToken->DefaultOwnerIndex == GroupsInToken)
                {
                    AccessToken->DefaultOwnerIndex = 0;
                }

                /*
                 * The principle of disabling a group SID is by
                 * taking away SE_GROUP_ENABLED_BY_DEFAULT and
                 * SE_GROUP_ENABLED attributes and assign
                 * SE_GROUP_USE_FOR_DENY_ONLY. This renders
                 * SID a "Deny only" SID.
                 */
                AccessToken->UserAndGroups[GroupsInToken].Attributes &= ~(SE_GROUP_ENABLED | SE_GROUP_ENABLED_BY_DEFAULT);
                AccessToken->UserAndGroups[GroupsInToken].Attributes |= SE_GROUP_USE_FOR_DENY_ONLY;

                /* Adjust the index and continue with the next group */
                GroupsInToken--;
                FoundGroup = FALSE;
                continue;
            }
        }
    }

    /* We've finally filtered the token, return it to the caller */
    *FilteredToken = AccessToken;
    Status = STATUS_SUCCESS;
    DPRINT("SepPerformTokenFiltering(): The token has been filtered!\n");

Quit:
    if (!NT_SUCCESS(Status))
    {
        /* Dereference the created token */
        ObDereferenceObject(AccessToken);
    }

    /* Unlock the source token */
    SepReleaseTokenLock(Token);

    return Status;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Filters an access token from an existing token, making it more restricted
 * than the previous one.
 *
 * @param[in] ExistingToken
 * An existing token for filtering.
 *
 * @param[in] Flags
 * Privilege flag options. This parameter argument influences how the token
 * is filtered. Such parameter can be 0. See NtFilterToken syscall for
 * more information.
 *
 * @param[in] SidsToDisable
 * Array of SIDs to disable. Such parameter can be NULL.
 *
 * @param[in] PrivilegesToDelete
 * Array of privileges to delete. If DISABLE_MAX_PRIVILEGE flag is specified
 * in the Flags parameter, PrivilegesToDelete is ignored.
 *
 * @param[in] RestrictedSids
 * An array of restricted SIDs for the new filtered token. Such parameter
 * can be NULL.
 *
 * @param[out] FilteredToken
 * The newly filtered token, returned to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully completed its
 * operations and that the access token has been filtered. STATUS_INVALID_PARAMETER
 * is returned if one or more of the parameter are not valid. A failure NTSTATUS code
 * is returned otherwise.
 */
NTSTATUS
NTAPI
SeFilterToken(
    _In_ PACCESS_TOKEN ExistingToken,
    _In_ ULONG Flags,
    _In_opt_ PTOKEN_GROUPS SidsToDisable,
    _In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
    _In_opt_ PTOKEN_GROUPS RestrictedSids,
    _Out_ PACCESS_TOKEN *FilteredToken)
{
    NTSTATUS Status;
    PTOKEN AccessToken;
    ULONG PrivilegesCount = 0;
    ULONG SidsCount = 0;
    ULONG RestrictedSidsCount = 0;

    PAGED_CODE();

    /* Begin copying the counters */
    if (SidsToDisable != NULL)
    {
        SidsCount = SidsToDisable->GroupCount;
    }

    if (PrivilegesToDelete != NULL)
    {
        PrivilegesCount = PrivilegesToDelete->PrivilegeCount;
    }

    if (RestrictedSids != NULL)
    {
        RestrictedSidsCount = RestrictedSids->GroupCount;
    }

    /* Call the internal API */
    Status = SepPerformTokenFiltering(ExistingToken,
                                      PrivilegesToDelete->Privileges,
                                      SidsToDisable->Groups,
                                      RestrictedSids->Groups,
                                      PrivilegesCount,
                                      SidsCount,
                                      RestrictedSidsCount,
                                      Flags,
                                      KernelMode,
                                      &AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SeFilterToken(): Failed to filter the token (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Insert the filtered token */
    Status = ObInsertObject(AccessToken,
                            NULL,
                            0,
                            0,
                            NULL,
                            NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SeFilterToken(): Failed to insert the filtered token (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Return it to the caller */
    *FilteredToken = AccessToken;
    return Status;
}

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Creates an access token.
 *
 * @param[out] TokenHandle
 * The returned created token handle to the caller.
 *
 * @param[in] DesiredAccess
 * The desired access rights for the token that we're creating.
 *
 * @param[in] ObjectAttributes
 * The object attributes for the token object that we're creating.
 *
 * @param[in] TokenType
 * The type of token to assign for the newly created token.
 *
 * @param[in] AuthenticationId
 * Authentication ID that represents the token's identity.
 *
 * @param[in] ExpirationTime
 * Expiration time for the token. If set to -1, the token never expires.
 *
 * @param[in] TokenUser
 * The main user entity for the token to assign.
 *
 * @param[in] TokenGroups
 * Group list of SIDs for the token to assign.
 *
 * @param[in] TokenPrivileges
 * Privileges for the token.
 *
 * @param[in] TokenOwner
 * The main user that owns the newly created token.
 *
 * @param[in] TokenPrimaryGroup
 * The primary group that represents as the main group of the token.
 *
 * @param[in] TokenDefaultDacl
 * Discretionary access control list for the token. This limits on how
 * the token can be used, accessed and used by whom.
 *
 * @param[in] TokenSource
 * The source origin of the token who creates it.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully created the token.
 * A failure NTSTATUS code is returned otherwise.
 */
__kernel_entry
NTSTATUS
NTAPI
NtCreateToken(
    _Out_ PHANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ TOKEN_TYPE TokenType,
    _In_ PLUID AuthenticationId,
    _In_ PLARGE_INTEGER ExpirationTime,
    _In_ PTOKEN_USER TokenUser,
    _In_ PTOKEN_GROUPS TokenGroups,
    _In_ PTOKEN_PRIVILEGES TokenPrivileges,
    _In_opt_ PTOKEN_OWNER TokenOwner,
    _In_ PTOKEN_PRIMARY_GROUP TokenPrimaryGroup,
    _In_opt_ PTOKEN_DEFAULT_DACL TokenDefaultDacl,
    _In_ PTOKEN_SOURCE TokenSource)
{
    HANDLE hToken;
    KPROCESSOR_MODE PreviousMode;
    ULONG PrivilegeCount, GroupCount;
    PSID OwnerSid, PrimaryGroupSid;
    PACL DefaultDacl;
    LARGE_INTEGER LocalExpirationTime = {{0, 0}};
    LUID LocalAuthenticationId;
    TOKEN_SOURCE LocalTokenSource;
    SECURITY_QUALITY_OF_SERVICE LocalSecurityQos;
    PLUID_AND_ATTRIBUTES CapturedPrivileges = NULL;
    PSID_AND_ATTRIBUTES CapturedUser = NULL;
    PSID_AND_ATTRIBUTES CapturedGroups = NULL;
    PSID CapturedOwnerSid = NULL;
    PSID CapturedPrimaryGroupSid = NULL;
    PACL CapturedDefaultDacl = NULL;
    ULONG PrivilegesLength, UserLength, GroupsLength;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(TokenHandle);

            if (ObjectAttributes != NULL)
            {
                ProbeForRead(ObjectAttributes,
                             sizeof(OBJECT_ATTRIBUTES),
                             sizeof(ULONG));
                LocalSecurityQos = *(SECURITY_QUALITY_OF_SERVICE*)ObjectAttributes->SecurityQualityOfService;
            }

            ProbeForRead(AuthenticationId,
                         sizeof(LUID),
                         sizeof(ULONG));
            LocalAuthenticationId = *AuthenticationId;

            LocalExpirationTime = ProbeForReadLargeInteger(ExpirationTime);

            ProbeForRead(TokenUser,
                         sizeof(TOKEN_USER),
                         sizeof(ULONG));

            ProbeForRead(TokenGroups,
                         sizeof(TOKEN_GROUPS),
                         sizeof(ULONG));
            GroupCount = TokenGroups->GroupCount;

            ProbeForRead(TokenPrivileges,
                         sizeof(TOKEN_PRIVILEGES),
                         sizeof(ULONG));
            PrivilegeCount = TokenPrivileges->PrivilegeCount;

            if (TokenOwner != NULL)
            {
                ProbeForRead(TokenOwner,
                             sizeof(TOKEN_OWNER),
                             sizeof(ULONG));
                OwnerSid = TokenOwner->Owner;
            }
            else
            {
                OwnerSid = NULL;
            }

            ProbeForRead(TokenPrimaryGroup,
                         sizeof(TOKEN_PRIMARY_GROUP),
                         sizeof(ULONG));
            PrimaryGroupSid = TokenPrimaryGroup->PrimaryGroup;

            if (TokenDefaultDacl != NULL)
            {
                ProbeForRead(TokenDefaultDacl,
                             sizeof(TOKEN_DEFAULT_DACL),
                             sizeof(ULONG));
                DefaultDacl = TokenDefaultDacl->DefaultDacl;
            }
            else
            {
                DefaultDacl = NULL;
            }

            ProbeForRead(TokenSource,
                         sizeof(TOKEN_SOURCE),
                         sizeof(ULONG));
            LocalTokenSource = *TokenSource;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }
    else
    {
        if (ObjectAttributes != NULL)
            LocalSecurityQos = *(SECURITY_QUALITY_OF_SERVICE*)ObjectAttributes->SecurityQualityOfService;
        LocalAuthenticationId = *AuthenticationId;
        LocalExpirationTime = *ExpirationTime;
        GroupCount = TokenGroups->GroupCount;
        PrivilegeCount = TokenPrivileges->PrivilegeCount;
        OwnerSid = TokenOwner ? TokenOwner->Owner : NULL;
        PrimaryGroupSid = TokenPrimaryGroup->PrimaryGroup;
        DefaultDacl = TokenDefaultDacl ? TokenDefaultDacl->DefaultDacl : NULL;
        LocalTokenSource = *TokenSource;
    }

    /* Check token type */
    if ((TokenType < TokenPrimary) ||
        (TokenType > TokenImpersonation))
    {
        return STATUS_BAD_TOKEN_TYPE;
    }

    /* Check for token creation privilege */
    if (!SeSinglePrivilegeCheck(SeCreateTokenPrivilege, PreviousMode))
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Capture the user SID and attributes */
    Status = SeCaptureSidAndAttributesArray(&TokenUser->User,
                                            1,
                                            PreviousMode,
                                            NULL,
                                            0,
                                            PagedPool,
                                            FALSE,
                                            &CapturedUser,
                                            &UserLength);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture the groups SID and attributes array */
    Status = SeCaptureSidAndAttributesArray(&TokenGroups->Groups[0],
                                            GroupCount,
                                            PreviousMode,
                                            NULL,
                                            0,
                                            PagedPool,
                                            FALSE,
                                            &CapturedGroups,
                                            &GroupsLength);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture privileges */
    Status = SeCaptureLuidAndAttributesArray(&TokenPrivileges->Privileges[0],
                                             PrivilegeCount,
                                             PreviousMode,
                                             NULL,
                                             0,
                                             PagedPool,
                                             FALSE,
                                             &CapturedPrivileges,
                                             &PrivilegesLength);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture the token owner SID */
    if (TokenOwner != NULL)
    {
        Status = SepCaptureSid(OwnerSid,
                               PreviousMode,
                               PagedPool,
                               FALSE,
                               &CapturedOwnerSid);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    /* Capture the token primary group SID */
    Status = SepCaptureSid(PrimaryGroupSid,
                           PreviousMode,
                           PagedPool,
                           FALSE,
                           &CapturedPrimaryGroupSid);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    /* Capture DefaultDacl */
    if (DefaultDacl != NULL)
    {
        Status = SepCaptureAcl(DefaultDacl,
                               PreviousMode,
                               NonPagedPool,
                               FALSE,
                               &CapturedDefaultDacl);
        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    /* Call the internal function */
    Status = SepCreateToken(&hToken,
                            PreviousMode,
                            DesiredAccess,
                            ObjectAttributes,
                            TokenType,
                            LocalSecurityQos.ImpersonationLevel,
                            &LocalAuthenticationId,
                            &LocalExpirationTime,
                            CapturedUser,
                            GroupCount,
                            CapturedGroups,
                            GroupsLength,
                            PrivilegeCount,
                            CapturedPrivileges,
                            CapturedOwnerSid,
                            CapturedPrimaryGroupSid,
                            CapturedDefaultDacl,
                            &LocalTokenSource,
                            FALSE);
    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            *TokenHandle = hToken;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

Cleanup:

    /* Release what we captured */
    SeReleaseSidAndAttributesArray(CapturedUser, PreviousMode, FALSE);
    SeReleaseSidAndAttributesArray(CapturedGroups, PreviousMode, FALSE);
    SeReleaseLuidAndAttributesArray(CapturedPrivileges, PreviousMode, FALSE);
    SepReleaseSid(CapturedOwnerSid, PreviousMode, FALSE);
    SepReleaseSid(CapturedPrimaryGroupSid, PreviousMode, FALSE);
    SepReleaseAcl(CapturedDefaultDacl, PreviousMode, FALSE);

    return Status;
}

/**
 * @brief
 * Duplicates a token.
 *
 * @param[in] ExistingTokenHandle
 * An existing token to duplicate.
 *
 * @param[in] DesiredAccess
 * The desired access rights for the new duplicated token.
 *
 * @param[in] ObjectAttributes
 * Object attributes for the new duplicated token.
 *
 * @param[in] EffectiveOnly
 * If set to TRUE, the function removes all the disabled privileges and groups
 * of the token to duplicate.
 *
 * @param[in] TokenType
 * Type of token to assign to the duplicated token.
 *
 * @param[out] NewTokenHandle
 * The returned duplicated token handle.
 *
 * @return
 * STATUS_SUCCESS is returned if token duplication has completed successfully.
 * STATUS_BAD_IMPERSONATION_LEVEL is returned if the caller erroneously wants
 * to raise the impersonation level even though the conditions do not permit
 * it. A failure NTSTATUS code is returned otherwise.
 *
 * @remarks
 * Some sources claim 4th param is ImpersonationLevel, but on W2K
 * this is certainly NOT true, although I can't say for sure that EffectiveOnly
 * is correct either. -Gunnar
 * This is true. EffectiveOnly overrides SQOS.EffectiveOnly. - IAI
 * NOTE for readers: https://hex.pp.ua/nt/NtDuplicateToken.php is therefore
 * wrong in that regard, while MSDN documentation is correct.
 */
_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtDuplicateToken(
    _In_ HANDLE ExistingTokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ BOOLEAN EffectiveOnly,
    _In_ TOKEN_TYPE TokenType,
    _Out_ PHANDLE NewTokenHandle)
{
    KPROCESSOR_MODE PreviousMode;
    HANDLE hToken;
    PTOKEN Token;
    PTOKEN NewToken;
    PSECURITY_QUALITY_OF_SERVICE CapturedSecurityQualityOfService;
    BOOLEAN QoSPresent;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    NTSTATUS Status;

    PAGED_CODE();

    if (TokenType != TokenImpersonation &&
        TokenType != TokenPrimary)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(NewTokenHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    Status = SepCaptureSecurityQualityOfService(ObjectAttributes,
                                                PreviousMode,
                                                PagedPool,
                                                FALSE,
                                                &CapturedSecurityQualityOfService,
                                                &QoSPresent);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtDuplicateToken() failed to capture QoS! Status: 0x%x\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(ExistingTokenHandle,
                                       TOKEN_DUPLICATE,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       &HandleInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference token (Status 0x%lx)\n", Status);
        SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                           PreviousMode,
                                           FALSE);
        return Status;
    }

    /*
     * Fail, if the original token is an impersonation token and the caller
     * tries to raise the impersonation level of the new token above the
     * impersonation level of the original token.
     */
    if (Token->TokenType == TokenImpersonation)
    {
        if (QoSPresent &&
            CapturedSecurityQualityOfService->ImpersonationLevel >Token->ImpersonationLevel)
        {
            ObDereferenceObject(Token);
            SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                               PreviousMode,
                                               FALSE);
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }
    }

    /*
     * Fail, if a primary token is to be created from an impersonation token
     * and and the impersonation level of the impersonation token is below SecurityImpersonation.
     */
    if (Token->TokenType == TokenImpersonation &&
        TokenType == TokenPrimary &&
        Token->ImpersonationLevel < SecurityImpersonation)
    {
        ObDereferenceObject(Token);
        SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                           PreviousMode,
                                           FALSE);
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    Status = SepDuplicateToken(Token,
                               ObjectAttributes,
                               EffectiveOnly,
                               TokenType,
                               (QoSPresent ? CapturedSecurityQualityOfService->ImpersonationLevel : SecurityAnonymous),
                               PreviousMode,
                               &NewToken);

    ObDereferenceObject(Token);

    if (NT_SUCCESS(Status))
    {
        Status = ObInsertObject(NewToken,
                                NULL,
                                (DesiredAccess ? DesiredAccess : HandleInformation.GrantedAccess),
                                0,
                                NULL,
                                &hToken);
        if (NT_SUCCESS(Status))
        {
            _SEH2_TRY
            {
                *NewTokenHandle = hToken;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
    }

    /* Free the captured structure */
    SepReleaseSecurityQualityOfService(CapturedSecurityQualityOfService,
                                       PreviousMode,
                                       FALSE);

    return Status;
}

/**
 * @brief
 * Creates an access token in a restricted form
 * from the original existing token, that is, such
 * action is called filtering.
 *
 * @param[in] ExistingTokenHandle
 * A handle to an access token which is to be filtered.
 *
 * @param[in] Flags
 * Privilege flag options. This parameter argument influences how the
 * token's privileges are filtered. For further details see remarks.
 *
 * @param[in] SidsToDisable
 * Array of SIDs to disable. The action of doing so assigns the
 * SE_GROUP_USE_FOR_DENY_ONLY attribute to the respective group
 * SID and takes away SE_GROUP_ENABLED and SE_GROUP_ENABLED_BY_DEFAULT.
 * This parameter can be NULL. This can be a UM pointer.
 *
 * @param[in] PrivilegesToDelete
 * Array of privileges to delete. The function will walk within this
 * array to determine if the specified privileges do exist in the
 * access token. Any missing privileges gets ignored. This parameter
 * can be NULL. This can be a UM pointer.
 *
 * @param[in] RestrictedSids
 * An array list of restricted groups SID to be added in the access
 * token. A token that is already restricted the newly added restricted
 * SIDs are redundant information in addition to the existing restricted
 * SIDs in the token. This parameter can be NULL. This can be a UM pointer.
 *
 * @param[out] NewTokenHandle
 * A new handle to the restricted (filtered) access token. This can be a
 * UM pointer.
 *
 * @return
 * Returns STATUS_SUCCESS if the routine has successfully filtered the
 * access token. STATUS_INVALID_PARAMETER is returned if one or more
 * parameters are not valid (see SepPerformTokenFiltering routine call
 * for more information). A failure NTSTATUS code is returned otherwise.
 *
 * @remarks
 * The Flags parameter determines the final outcome of how the privileges
 * in an access token are filtered. This parameter can take these supported
 * values (these can be combined):
 *
 * 0 -- Filter the token's privileges in the usual way. The function expects
 *      that the caller MUST PROVIDE a valid array list of privileges to be
 *      deleted (that is, PrivilegesToDelete MUSTN'T BE NULL).
 *
 * DISABLE_MAX_PRIVILEGE -- Disables (deletes) all the privileges except SeChangeNotifyPrivilege
 *                          in the new access token. Bear in mind if this flag is specified
 *                          the routine ignores PrivilegesToDelete.
 *
 * SANDBOX_INERT -- Stores the TOKEN_SANDBOX_INERT token flag within the access token.
 *
 * LUA_TOKEN -- The newly filtered access token is a LUA token. This flag is not
 *              supported in Windows Server 2003.
 *
 * WRITE_RESTRICTED -- The newly filtered token has the restricted SIDs that are
 *                     considered only when evaluating write access onto the token.
 *                     This value is not supported in Windows Server 2003.
 */
NTSTATUS
NTAPI
NtFilterToken(
    _In_ HANDLE ExistingTokenHandle,
    _In_ ULONG Flags,
    _In_opt_ PTOKEN_GROUPS SidsToDisable,
    _In_opt_ PTOKEN_PRIVILEGES PrivilegesToDelete,
    _In_opt_ PTOKEN_GROUPS RestrictedSids,
    _Out_ PHANDLE NewTokenHandle)
{
    PTOKEN Token, FilteredToken;
    HANDLE FilteredTokenHandle;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    OBJECT_HANDLE_INFORMATION HandleInfo;
    ULONG ResultLength;
    ULONG CapturedSidsCount = 0;
    ULONG CapturedPrivilegesCount = 0;
    ULONG CapturedRestrictedSidsCount = 0;
    ULONG ProbeSize = 0;
    PSID_AND_ATTRIBUTES CapturedSids = NULL;
    PSID_AND_ATTRIBUTES CapturedRestrictedSids = NULL;
    PLUID_AND_ATTRIBUTES CapturedPrivileges = NULL;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    _SEH2_TRY
    {
        /* Probe SidsToDisable */
        if (SidsToDisable != NULL)
        {
            /* Probe the header */
            ProbeForRead(SidsToDisable, sizeof(*SidsToDisable), sizeof(ULONG));

            CapturedSidsCount = SidsToDisable->GroupCount;
            ProbeSize = FIELD_OFFSET(TOKEN_GROUPS, Groups[CapturedSidsCount]);

            ProbeForRead(SidsToDisable, ProbeSize, sizeof(ULONG));
        }

        /* Probe PrivilegesToDelete */
        if (PrivilegesToDelete != NULL)
        {
            /* Probe the header */
            ProbeForRead(PrivilegesToDelete, sizeof(*PrivilegesToDelete), sizeof(ULONG));

            CapturedPrivilegesCount = PrivilegesToDelete->PrivilegeCount;
            ProbeSize = FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges[CapturedPrivilegesCount]);

            ProbeForRead(PrivilegesToDelete, ProbeSize, sizeof(ULONG));
        }

        /* Probe RestrictedSids */
        if (RestrictedSids != NULL)
        {
            /* Probe the header */
            ProbeForRead(RestrictedSids, sizeof(*RestrictedSids), sizeof(ULONG));

            CapturedRestrictedSidsCount = RestrictedSids->GroupCount;
            ProbeSize = FIELD_OFFSET(TOKEN_GROUPS, Groups[CapturedRestrictedSidsCount]);

            ProbeForRead(RestrictedSids, ProbeSize, sizeof(ULONG));
        }

        /* Probe the handle */
        ProbeForWriteHandle(NewTokenHandle);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Reference the token */
    Status = ObReferenceObjectByHandle(ExistingTokenHandle,
                                       TOKEN_DUPLICATE,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       &HandleInfo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFilterToken(): Failed to reference the token (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Capture the group SIDs */
    if (SidsToDisable != NULL)
    {
        Status = SeCaptureSidAndAttributesArray(SidsToDisable->Groups,
                                                CapturedSidsCount,
                                                PreviousMode,
                                                NULL,
                                                0,
                                                PagedPool,
                                                TRUE,
                                                &CapturedSids,
                                                &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtFilterToken(): Failed to capture the SIDs (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /* Capture the privileges */
    if (PrivilegesToDelete != NULL)
    {
        Status = SeCaptureLuidAndAttributesArray(PrivilegesToDelete->Privileges,
                                                 CapturedPrivilegesCount,
                                                 PreviousMode,
                                                 NULL,
                                                 0,
                                                 PagedPool,
                                                 TRUE,
                                                 &CapturedPrivileges,
                                                 &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtFilterToken(): Failed to capture the privileges (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /* Capture the restricted SIDs */
    if (RestrictedSids != NULL)
    {
        Status = SeCaptureSidAndAttributesArray(RestrictedSids->Groups,
                                                CapturedRestrictedSidsCount,
                                                PreviousMode,
                                                NULL,
                                                0,
                                                PagedPool,
                                                TRUE,
                                                &CapturedRestrictedSids,
                                                &ResultLength);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtFilterToken(): Failed to capture the restricted SIDs (Status 0x%lx)\n", Status);
            goto Quit;
        }
    }

    /* Call the internal API */
    Status = SepPerformTokenFiltering(Token,
                                      CapturedPrivileges,
                                      CapturedSids,
                                      CapturedRestrictedSids,
                                      CapturedPrivilegesCount,
                                      CapturedSidsCount,
                                      CapturedRestrictedSidsCount,
                                      Flags,
                                      PreviousMode,
                                      &FilteredToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFilterToken(): Failed to filter the token (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /* Insert the filtered token and retrieve a handle to it */
    Status = ObInsertObject(FilteredToken,
                            NULL,
                            HandleInfo.GrantedAccess,
                            0,
                            NULL,
                            &FilteredTokenHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtFilterToken(): Failed to insert the filtered token (Status 0x%lx)\n", Status);
        goto Quit;
    }

    /* And return it to the caller once we're done */
    _SEH2_TRY
    {
        *NewTokenHandle = FilteredTokenHandle;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        _SEH2_YIELD(goto Quit);
    }
    _SEH2_END;

Quit:
    /* Dereference the token */
    ObDereferenceObject(Token);

    /* Release all the captured data */
    if (CapturedSids != NULL)
    {
        SeReleaseSidAndAttributesArray(CapturedSids,
                                       PreviousMode,
                                       TRUE);
    }

    if (CapturedPrivileges != NULL)
    {
        SeReleaseLuidAndAttributesArray(CapturedPrivileges,
                                        PreviousMode,
                                        TRUE);
    }

    if (CapturedRestrictedSids != NULL)
    {
        SeReleaseSidAndAttributesArray(CapturedRestrictedSids,
                                       PreviousMode,
                                       TRUE);
    }

    return Status;
}

/* EOF */
