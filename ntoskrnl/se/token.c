/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security access token implementation base support routines
 * COPYRIGHT:   Copyright David Welch <welch@cwcom.net>
 *              Copyright 2021-2022 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

POBJECT_TYPE SeTokenObjectType = NULL;

TOKEN_SOURCE SeSystemTokenSource = {"*SYSTEM*", {0}};
LUID SeSystemAuthenticationId = SYSTEM_LUID;
LUID SeAnonymousAuthenticationId = ANONYMOUS_LOGON_LUID;

static GENERIC_MAPPING SepTokenMapping = {
    TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_EXECUTE,
    TOKEN_ALL_ACCESS
};

/* PRIVATE FUNCTIONS *****************************************************************/

/**
 * @brief
 * Creates a lock for the token.
 *
 * @param[in,out] Token
 * A token which lock has to be created.
 *
 * @return
 * STATUS_SUCCESS if the pool allocation and resource initialisation have
 * completed successfully, otherwise STATUS_INSUFFICIENT_RESOURCES on a
 * pool allocation failure.
 */
NTSTATUS
SepCreateTokenLock(
    _Inout_ PTOKEN Token)
{
    PAGED_CODE();

    Token->TokenLock = ExAllocatePoolWithTag(NonPagedPool,
                                             sizeof(ERESOURCE),
                                             TAG_SE_TOKEN_LOCK);
    if (Token->TokenLock == NULL)
    {
        DPRINT1("SepCreateTokenLock(): Failed to allocate memory!\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ExInitializeResourceLite(Token->TokenLock);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Deletes a lock of a token.
 *
 * @param[in,out] Token
 * A token which contains the lock.
 *
 * @return
 * Nothing.
 */
VOID
SepDeleteTokenLock(
    _Inout_ PTOKEN Token)
{
    PAGED_CODE();

    ExDeleteResourceLite(Token->TokenLock);
    ExFreePoolWithTag(Token->TokenLock, TAG_SE_TOKEN_LOCK);
}

/**
 * @brief
 * Compares the elements of SID arrays provided by tokens.
 * The elements that are being compared for equality are
 * the SIDs and their attributes.
 *
 * @param[in] SidArrayToken1
 * SID array from the first token.
 *
 * @param[in] CountSidArray1
 * SID count array from the first token.
 *
 * @param[in] SidArrayToken2
 * SID array from the second token.
 *
 * @param[in] CountSidArray2
 * SID count array from the second token.
 *
 * @return
 * Returns TRUE if the elements match from either arrays,
 * FALSE otherwise.
 */
static
BOOLEAN
SepCompareSidAndAttributesFromTokens(
    _In_ PSID_AND_ATTRIBUTES SidArrayToken1,
    _In_ ULONG CountSidArray1,
    _In_ PSID_AND_ATTRIBUTES SidArrayToken2,
    _In_ ULONG CountSidArray2)
{
    ULONG FirstCount, SecondCount;
    PSID_AND_ATTRIBUTES FirstSidArray, SecondSidArray;
    PAGED_CODE();

    /* Bail out if index counters provided are not equal */
    if (CountSidArray1 != CountSidArray2)
    {
        DPRINT("SepCompareSidAndAttributesFromTokens(): Index counters are not the same!\n");
        return FALSE;
    }

    /* Loop over the SID arrays and compare them */
    for (FirstCount = 0; FirstCount < CountSidArray1; FirstCount++)
    {
        for (SecondCount = 0; SecondCount < CountSidArray2; SecondCount++)
        {
            FirstSidArray = &SidArrayToken1[FirstCount];
            SecondSidArray = &SidArrayToken2[SecondCount];

            if (RtlEqualSid(FirstSidArray->Sid, SecondSidArray->Sid) &&
                FirstSidArray->Attributes == SecondSidArray->Attributes)
            {
                break;
            }
        }

        /* We've exhausted the array of the second token without finding this one */
        if (SecondCount == CountSidArray2)
        {
            DPRINT("SepCompareSidAndAttributesFromTokens(): No matching elements could be found in either token!\n");
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief
 * Compares the elements of privilege arrays provided by tokens.
 * The elements that are being compared for equality are
 * the privileges and their attributes.
 *
 * @param[in] PrivArrayToken1
 * Privilege array from the first token.
 *
 * @param[in] CountPrivArray1
 * Privilege count array from the first token.
 *
 * @param[in] PrivArrayToken2
 * Privilege array from the second token.
 *
 * @param[in] CountPrivArray2
 * Privilege count array from the second token.
 *
 * @return
 * Returns TRUE if the elements match from either arrays,
 * FALSE otherwise.
 */
static
BOOLEAN
SepComparePrivilegeAndAttributesFromTokens(
    _In_ PLUID_AND_ATTRIBUTES PrivArrayToken1,
    _In_ ULONG CountPrivArray1,
    _In_ PLUID_AND_ATTRIBUTES PrivArrayToken2,
    _In_ ULONG CountPrivArray2)
{
    ULONG FirstCount, SecondCount;
    PLUID_AND_ATTRIBUTES FirstPrivArray, SecondPrivArray;
    PAGED_CODE();

    /* Bail out if index counters provided are not equal */
    if (CountPrivArray1 != CountPrivArray2)
    {
        DPRINT("SepComparePrivilegeAndAttributesFromTokens(): Index counters are not the same!\n");
        return FALSE;
    }

    /* Loop over the privilege arrays and compare them */
    for (FirstCount = 0; FirstCount < CountPrivArray1; FirstCount++)
    {
        for (SecondCount = 0; SecondCount < CountPrivArray2; SecondCount++)
        {
            FirstPrivArray = &PrivArrayToken1[FirstCount];
            SecondPrivArray = &PrivArrayToken2[SecondCount];

            if (RtlEqualLuid(&FirstPrivArray->Luid, &SecondPrivArray->Luid) &&
                FirstPrivArray->Attributes == SecondPrivArray->Attributes)
            {
                break;
            }
        }

        /* We've exhausted the array of the second token without finding this one */
        if (SecondCount == CountPrivArray2)
        {
            DPRINT("SepComparePrivilegeAndAttributesFromTokens(): No matching elements could be found in either token!\n");
            return FALSE;
        }
    }

    return TRUE;
}

/**
 * @brief
 * Compares tokens if they're equal based on all the following properties. If all
 * of the said conditions are met then the tokens are deemed as equal.
 *
 * - Every SID that is present in either token is also present in the other one.
 * - Both or none of the tokens are restricted.
 * - If both tokens are restricted, every SID that is restricted in either token is
 *   also restricted in the other one.
 * - Every privilege present in either token is also present in the other one.
 *
 * @param[in] FirstToken
 * The first token.
 *
 * @param[in] SecondToken
 * The second token.
 *
 * @param[out] Equal
 * The retrieved value which determines if the tokens are
 * equal or not.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
static
NTSTATUS
SepCompareTokens(
    _In_ PTOKEN FirstToken,
    _In_ PTOKEN SecondToken,
    _Out_ PBOOLEAN Equal)
{
    BOOLEAN Restricted, IsEqual = FALSE;
    PAGED_CODE();

    ASSERT(FirstToken != SecondToken);

    /* Lock the tokens */
    SepAcquireTokenLockShared(FirstToken);
    SepAcquireTokenLockShared(SecondToken);

    /* Check if every SID that is present in either token is also present in the other one */
    if (!SepCompareSidAndAttributesFromTokens(FirstToken->UserAndGroups,
                                              FirstToken->UserAndGroupCount,
                                              SecondToken->UserAndGroups,
                                              SecondToken->UserAndGroupCount))
    {
        goto Quit;
    }

    /* Is one token restricted but the other isn't? */
    Restricted = SeTokenIsRestricted(FirstToken);
    if (Restricted != SeTokenIsRestricted(SecondToken))
    {
        /* If that's the case then bail out */
        goto Quit;
    }

    /*
     * If both tokens are restricted check if every SID
     * that is restricted in either token is also restricted
     * in the other one.
     */
    if (Restricted)
    {
        if (!SepCompareSidAndAttributesFromTokens(FirstToken->RestrictedSids,
                                                  FirstToken->RestrictedSidCount,
                                                  SecondToken->RestrictedSids,
                                                  SecondToken->RestrictedSidCount))
        {
            goto Quit;
        }
    }

    /* Check if every privilege present in either token is also present in the other one */
    if (!SepComparePrivilegeAndAttributesFromTokens(FirstToken->Privileges,
                                                    FirstToken->PrivilegeCount,
                                                    SecondToken->Privileges,
                                                    SecondToken->PrivilegeCount))
    {
        goto Quit;
    }

    /* If we're here then the tokens are equal */
    IsEqual = TRUE;
    DPRINT("SepCompareTokens(): Tokens are equal!\n");

Quit:
    /* Unlock the tokens */
    SepReleaseTokenLock(SecondToken);
    SepReleaseTokenLock(FirstToken);

    *Equal = IsEqual;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Private function that impersonates the system's anonymous logon token.
 * The major bulk of the impersonation procedure is done here.
 *
 * @param[in] Thread
 * The executive thread object that is to impersonate the client.
 *
 * @param[in] PreviousMode
 * The access processor mode, indicating if the call is executed
 * in kernel or user mode.
 *
 * @return
 * Returns STATUS_SUCCESS if the impersonation has succeeded.
 * STATUS_UNSUCCESSFUL is returned if the primary token couldn't be
 * obtained from the current process to perform additional tasks.
 * STATUS_ACCESS_DENIED is returned if the process' primary token is
 * restricted, which for this matter we cannot impersonate onto a
 * restricted process. Otherwise a failure NTSTATUS code is returned.
 */
static
NTSTATUS
SepImpersonateAnonymousToken(
    _In_ PETHREAD Thread,
    _In_ KPROCESSOR_MODE PreviousMode)
{
    NTSTATUS Status;
    PTOKEN TokenToImpersonate, ProcessToken;
    ULONG IncludeEveryoneValueData;
    PAGED_CODE();

    /*
     * We must check first which kind of token
     * shall we assign for the thread to impersonate,
     * the one with Everyone Group SID or the other
     * without. Invoke the registry helper to
     * return the data value for us.
     */
    Status = SepRegQueryHelper(L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\Lsa",
                               L"EveryoneIncludesAnonymous",
                               REG_DWORD,
                               sizeof(IncludeEveryoneValueData),
                               &IncludeEveryoneValueData);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepRegQueryHelper(): Failed to query the registry value (Status 0x%lx)\n", Status);
        return Status;
    }

    if (IncludeEveryoneValueData == 0)
    {
        DPRINT("SepImpersonateAnonymousToken(): Assigning the token not including the Everyone Group SID...\n");
        TokenToImpersonate = SeAnonymousLogonTokenNoEveryone;
    }
    else
    {
        DPRINT("SepImpersonateAnonymousToken(): Assigning the token including the Everyone Group SID...\n");
        TokenToImpersonate = SeAnonymousLogonToken;
    }

    /*
     * Tell the object manager that we're going to use this token
     * object now by incrementing the reference count.
    */
    Status = ObReferenceObjectByPointer(TokenToImpersonate,
                                        TOKEN_IMPERSONATE,
                                        SeTokenObjectType,
                                        PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepImpersonateAnonymousToken(): Couldn't be able to use the token, bail out...\n");
        return Status;
    }

    /*
     * Reference the primary token of the current process that the anonymous
     * logon token impersonation procedure is being performed. We'll be going
     * to use the process' token to figure out if the process is actually
     * restricted or not.
     */
    ProcessToken = PsReferencePrimaryToken(PsGetCurrentProcess());
    if (!ProcessToken)
    {
        DPRINT1("SepImpersonateAnonymousToken(): Couldn't be able to get the process' primary token, bail out...\n");
        ObDereferenceObject(TokenToImpersonate);
        return STATUS_UNSUCCESSFUL;
    }

    /* Now, is the token from the current process restricted? */
    if (SeTokenIsRestricted(ProcessToken))
    {
        DPRINT1("SepImpersonateAnonymousToken(): The process is restricted, can't do anything. Bail out...\n");
        PsDereferencePrimaryToken(ProcessToken);
        ObDereferenceObject(TokenToImpersonate);
        return STATUS_ACCESS_DENIED;
    }

    /*
     * Finally it's time to impersonate! But first, fast dereference the
     * process' primary token as we no longer need it.
     */
    ObFastDereferenceObject(&PsGetCurrentProcess()->Token, ProcessToken);
    Status = PsImpersonateClient(Thread, TokenToImpersonate, TRUE, FALSE, SecurityImpersonation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("SepImpersonateAnonymousToken(): Failed to impersonate, bail out...\n");
        ObDereferenceObject(TokenToImpersonate);
        return Status;
    }

    return Status;
}

/**
 * @brief
 * Updates the token's flags based upon the privilege that the token
 * has been granted. The flag can either be taken out or given to the token
 * if the attributes of the specified privilege is enabled or not.
 *
 * @param[in,out] Token
 * The token where the flags are to be changed.
 *
 * @param[in] Index
 * The index count which represents the total sum of privileges. The count in question
 * MUST NOT exceed the expected privileges count of the token.
 *
 * @return
 * Nothing.
 */
VOID
SepUpdateSinglePrivilegeFlagToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index)
{
    ULONG TokenFlag;
    ASSERT(Index < Token->PrivilegeCount);

    /* The high part of all values we are interested in is 0 */
    if (Token->Privileges[Index].Luid.HighPart != 0)
    {
        return;
    }

    /* Check for certain privileges to update flags */
    if (Token->Privileges[Index].Luid.LowPart == SE_CHANGE_NOTIFY_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_TRAVERSE_PRIVILEGE;
    }
    else if (Token->Privileges[Index].Luid.LowPart == SE_BACKUP_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_BACKUP_PRIVILEGE;
    }
    else if (Token->Privileges[Index].Luid.LowPart == SE_RESTORE_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_RESTORE_PRIVILEGE;
    }
    else if (Token->Privileges[Index].Luid.LowPart == SE_IMPERSONATE_PRIVILEGE)
    {
        TokenFlag = TOKEN_HAS_IMPERSONATE_PRIVILEGE;
    }
    else
    {
        /* Nothing to do */
        return;
    }

    /* Check if the specified privilege is enabled */
    if (Token->Privileges[Index].Attributes & SE_PRIVILEGE_ENABLED)
    {
        /* It is enabled, so set the flag */
        Token->TokenFlags |= TokenFlag;
    }
    else
    {
        /* Is is disabled, so remove the flag */
        Token->TokenFlags &= ~TokenFlag;
    }
}

/**
 * @brief
 * Checks if a token belongs to the main user, being the owner.
 *
 * @param[in] _Token
 * A valid token object.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor where the owner is to be found.
 *
 * @param[in] TokenLocked
 * If set to TRUE, the token has been already locked and there's
 * no need to lock it again. Otherwise the function will acquire
 * the lock.
 *
 * @return
 * Returns TRUE if the token belongs to a owner, FALSE otherwise.
 */
BOOLEAN
NTAPI
SepTokenIsOwner(
    _In_ PACCESS_TOKEN _Token,
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ BOOLEAN TokenLocked)
{
    PSID Sid;
    BOOLEAN Result;
    PTOKEN Token = _Token;

    /* Get the owner SID */
    Sid = SepGetOwnerFromDescriptor(SecurityDescriptor);
    ASSERT(Sid != NULL);

    /* Lock the token if needed */
    if (!TokenLocked) SepAcquireTokenLockShared(Token);

    /* Check if the owner SID is found, handling restricted case as well */
    Result = SepSidInToken(Token, Sid);
    if ((Result) && (Token->TokenFlags & TOKEN_IS_RESTRICTED))
    {
        Result = SepSidInTokenEx(Token, NULL, Sid, FALSE, TRUE);
    }

    /* Release the lock if we had acquired it */
    if (!TokenLocked) SepReleaseTokenLock(Token);

    /* Return the result */
    return Result;
}

/**
 * @brief
 * Updates the token's flags based upon the privilege that the token
 * has been granted. The function uses the private helper, SepUpdateSinglePrivilegeFlagToken,
 * in order to update the flags of a token.
 *
 * @param[in,out] Token
 * The token where the flags are to be changed.
 *
 * @return
 * Nothing.
 */
VOID
SepUpdatePrivilegeFlagsToken(
    _Inout_ PTOKEN Token)
{
    ULONG i;

    /* Loop all privileges */
    for (i = 0; i < Token->PrivilegeCount; i++)
    {
        /* Updates the flags for this privilege */
        SepUpdateSinglePrivilegeFlagToken(Token, i);
    }
}

/**
 * @brief
 * Removes a privilege from the token.
 *
 * @param[in,out] Token
 * The token where the privilege is to be removed.
 *
 * @param[in] Index
 * The index count which represents the number position of the privilege
 * we want to remove.
 *
 * @return
 * Nothing.
 */
VOID
SepRemovePrivilegeToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index)
{
    ULONG MoveCount;
    ASSERT(Index < Token->PrivilegeCount);

    /* Calculate the number of trailing privileges */
    MoveCount = Token->PrivilegeCount - Index - 1;
    if (MoveCount != 0)
    {
        /* Move them one location ahead */
        RtlMoveMemory(&Token->Privileges[Index],
                      &Token->Privileges[Index + 1],
                      MoveCount * sizeof(LUID_AND_ATTRIBUTES));
    }

    /* Update privilege count */
    Token->PrivilegeCount--;
}

/**
 * @brief
 * Removes a group from the token.
 *
 * @param[in,out] Token
 * The token where the group is to be removed.
 *
 * @param[in] Index
 * The index count which represents the number position of the group
 * we want to remove.
 *
 * @return
 * Nothing.
 */
VOID
SepRemoveUserGroupToken(
    _Inout_ PTOKEN Token,
    _In_ ULONG Index)
{
    ULONG MoveCount;
    ASSERT(Index < Token->UserAndGroupCount);

    /* Calculate the number of trailing groups */
    MoveCount = Token->UserAndGroupCount - Index - 1;
    if (MoveCount != 0)
    {
        /* Time to remove the group by moving one location ahead */
        RtlMoveMemory(&Token->UserAndGroups[Index],
                      &Token->UserAndGroups[Index + 1],
                      MoveCount * sizeof(SID_AND_ATTRIBUTES));
    }

    /* Remove one group count */
    Token->UserAndGroupCount--;
}

/**
 * @brief
 * Computes the exact available dynamic area of an access
 * token whilst querying token statistics.
 *
 * @param[in] DynamicCharged
 * The current charged dynamic area of an access token.
 * This must not be 0!
 *
 * @param[in] PrimaryGroup
 * A pointer to a primary group SID.
 *
 * @param[in] DefaultDacl
 * If provided, this pointer points to a default DACL of an
 * access token.
 *
 * @return
 * Returns the calculated available dynamic area.
 */
ULONG
SepComputeAvailableDynamicSpace(
    _In_ ULONG DynamicCharged,
    _In_ PSID PrimaryGroup,
    _In_opt_ PACL DefaultDacl)
{
    ULONG DynamicAvailable;

    PAGED_CODE();

    /* A token's dynamic area is always charged */
    ASSERT(DynamicCharged != 0);

    /*
     * Take into account the default DACL if
     * the token has one. Otherwise the occupied
     * space is just the present primary group.
     */
    DynamicAvailable = DynamicCharged - RtlLengthSid(PrimaryGroup);
    if (DefaultDacl)
    {
        DynamicAvailable -= DefaultDacl->AclSize;
    }

    return DynamicAvailable;
}

/**
 * @brief
 * Re-builds the dynamic part area of an access token
 * during an a default DACL or primary group replacement
 * within the said token if the said dynamic area can't
 * hold the new security content.
 *
 * @param[in] AccessToken
 * A pointer to an access token where its dynamic part
 * is to be re-built and expanded based upon the new
 * dynamic part size provided by the caller. Dynamic
 * part expansion is not always guaranteed. See Remarks
 * for further information.
 *
 * @param[in] NewDynamicPartSize
 * The new dynamic part size.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has completed its
 * operations successfully. STATUS_INSUFFICIENT_RESOURCES
 * is returned if the new dynamic part could not be allocated.
 *
 * @remarks
 * STATUS_SUCCESS does not indicate if the function has re-built
 * the dynamic part of a token. If the current dynamic area size
 * suffices the new dynamic area length provided by the caller
 * then the dynamic area can hold the new security content buffer
 * so dynamic part expansion is not necessary.
 */
NTSTATUS
SepRebuildDynamicPartOfToken(
    _Inout_ PTOKEN AccessToken,
    _In_ ULONG NewDynamicPartSize)
{
    PVOID NewDynamicPart;
    PVOID PreviousDynamicPart;
    ULONG CurrentDynamicLength;

    PAGED_CODE();

    /* Sanity checks */
    ASSERT(AccessToken);
    ASSERT(NewDynamicPartSize != 0);

    /*
     * Compute the exact length of the available
     * dynamic part of the access token.
     */
    CurrentDynamicLength = AccessToken->DynamicAvailable + RtlLengthSid(AccessToken->PrimaryGroup);
    if (AccessToken->DefaultDacl)
    {
        CurrentDynamicLength += AccessToken->DefaultDacl->AclSize;
    }

    /*
     * Figure out if the current dynamic part is too small
     * to fit new contents inside the said dynamic part.
     * Rebuild the dynamic area and expand it if necessary.
     */
    if (CurrentDynamicLength < NewDynamicPartSize)
    {
        NewDynamicPart = ExAllocatePoolWithTag(PagedPool,
                                               NewDynamicPartSize,
                                               TAG_TOKEN_DYNAMIC);
        if (NewDynamicPart == NULL)
        {
            DPRINT1("SepRebuildDynamicPartOfToken(): Insufficient resources to allocate new dynamic part!\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Copy the existing dynamic part */
        PreviousDynamicPart = AccessToken->DynamicPart;
        RtlCopyMemory(NewDynamicPart, PreviousDynamicPart, CurrentDynamicLength);

        /* Update the available dynamic area and assign new dynamic */
        AccessToken->DynamicAvailable += NewDynamicPartSize - CurrentDynamicLength;
        AccessToken->DynamicPart = NewDynamicPart;

        /* Move the contents (primary group and default DACL) addresses as well */
        AccessToken->PrimaryGroup = (PSID)((ULONG_PTR)AccessToken->DynamicPart +
                                    ((ULONG_PTR)AccessToken->PrimaryGroup - (ULONG_PTR)PreviousDynamicPart));
        if (AccessToken->DefaultDacl != NULL)
        {
            AccessToken->DefaultDacl = (PACL)((ULONG_PTR)AccessToken->DynamicPart +
                                       ((ULONG_PTR)AccessToken->DefaultDacl - (ULONG_PTR)PreviousDynamicPart));
        }

        /* And discard the previous dynamic part */
        DPRINT("SepRebuildDynamicPartOfToken(): The dynamic part has been re-built with success!\n");
        ExFreePoolWithTag(PreviousDynamicPart, TAG_TOKEN_DYNAMIC);
    }

    return STATUS_SUCCESS;
}

/**
 * @unimplemented
 * @brief
 * Frees (de-allocates) the proxy data memory block of a token.
 *
 * @param[in,out] ProxyData
 * The proxy data to be freed.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SepFreeProxyData(
    _Inout_ PVOID ProxyData)
{
    UNIMPLEMENTED;
}

/**
 * @unimplemented
 * @brief
 * Copies the proxy data from the source into the destination of a token.
 *
 * @param[out] Dest
 * The destination path where the proxy data is to be copied to.
 *
 * @param[in] Src
 * The source path where the proxy data is be copied from.
 *
 * @return
 * To be added...
 */
NTSTATUS
NTAPI
SepCopyProxyData(
    _Out_ PVOID* Dest,
    _In_ PVOID Src)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Replaces the old access token of a process (pointed by the EPROCESS kernel structure) with a
 * new access token. The new access token must be a primary token for use.
 *
 * @param[in] Process
 * The process instance where its access token is about to be replaced.
 *
 * @param[in] NewAccessToken
 * The new token that it's going to replace the old one.
 *
 * @param[out] OldAccessToken
 * The returned old token that's been replaced, which the caller can do anything.
 *
 * @return
 * Returns STATUS_SUCCESS if the exchange operation between tokens has completed successfully.
 * STATUS_BAD_TOKEN_TYPE is returned if the new token is not a primary one so that we cannot
 * exchange it with the old one from the process. STATUS_TOKEN_ALREADY_IN_USE is returned if
 * both tokens aren't equal which means one of them has different properties (groups, privileges, etc.)
 * and as such one of them is currently in use. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SeExchangePrimaryToken(
    _In_ PEPROCESS Process,
    _In_ PACCESS_TOKEN NewAccessToken,
    _Out_ PACCESS_TOKEN* OldAccessToken)
{
    PTOKEN OldToken;
    PTOKEN NewToken = (PTOKEN)NewAccessToken;

    PAGED_CODE();

    if (NewToken->TokenType != TokenPrimary)
        return STATUS_BAD_TOKEN_TYPE;

    if (NewToken->TokenInUse)
    {
        BOOLEAN IsEqual;
        NTSTATUS Status;

        /* Maybe we're trying to set the same token */
        OldToken = PsReferencePrimaryToken(Process);
        if (OldToken == NewToken)
        {
            /* So it's a nop. */
            *OldAccessToken = OldToken;
            return STATUS_SUCCESS;
        }

        Status = SepCompareTokens(OldToken, NewToken, &IsEqual);
        if (!NT_SUCCESS(Status))
        {
            PsDereferencePrimaryToken(OldToken);
            *OldAccessToken = NULL;
            return Status;
        }

        if (!IsEqual)
        {
            PsDereferencePrimaryToken(OldToken);
            *OldAccessToken = NULL;
            return STATUS_TOKEN_ALREADY_IN_USE;
        }
        /* Silently return STATUS_SUCCESS but do not set the new token,
         * as it's already in use elsewhere. */
        *OldAccessToken = OldToken;
        return STATUS_SUCCESS;
    }

    /* Lock the new token */
    SepAcquireTokenLockExclusive(NewToken);

    /* Mark new token in use */
    NewToken->TokenInUse = TRUE;

    /* Set the session ID for the new token */
    NewToken->SessionId = MmGetSessionId(Process);

    /* Unlock the new token */
    SepReleaseTokenLock(NewToken);

    /* Reference the new token */
    ObReferenceObject(NewToken);

    /* Replace the old with the new */
    OldToken = ObFastReplaceObject(&Process->Token, NewToken);

    /* Lock the old token */
    SepAcquireTokenLockExclusive(OldToken);

    /* Mark the old token as free */
    OldToken->TokenInUse = FALSE;

    /* Unlock the old token */
    SepReleaseTokenLock(OldToken);

    *OldAccessToken = (PACCESS_TOKEN)OldToken;
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Removes the primary token of a process.
 *
 * @param[in,out] Process
 * The process instance with the access token to be removed.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeDeassignPrimaryToken(
    _Inout_ PEPROCESS Process)
{
    PTOKEN OldToken;

    /* Remove the Token */
    OldToken = ObFastReplaceObject(&Process->Token, NULL);

    /* Mark the Old Token as free */
    OldToken->TokenInUse = FALSE;

    /* Dereference the Token */
    ObDereferenceObject(OldToken);
}

/**
 * @brief
 * Computes the length size of a SID.
 *
 * @param[in] Count
 * Total count of entries that have SIDs in them (that being PSID_AND_ATTRIBUTES in this context).
 *
 * @param[in] Src
 * Source that points to the attributes and SID entry structure.
 *
 * @return
 * Returns the total length of a SID size.
 */
ULONG
RtlLengthSidAndAttributes(
    _In_ ULONG Count,
    _In_ PSID_AND_ATTRIBUTES Src)
{
    ULONG i;
    ULONG uLength;

    PAGED_CODE();

    uLength = Count * sizeof(SID_AND_ATTRIBUTES);
    for (i = 0; i < Count; i++)
        uLength += RtlLengthSid(Src[i].Sid);

    return uLength;
}

/**
 * @brief
 * Finds the primary group and default owner entity based on the submitted primary group instance
 * and an access token.
 *
 * @param[in] Token
 * Access token to begin the search query of primary group and default owner.
 *
 * @param[in] PrimaryGroup
 * A primary group SID to be used for search query, determining if user & groups of a token
 * and the submitted primary group do match.
 *
 * @param[in] DefaultOwner
 * The default owner. If specified, it's used to determine if the token belongs to the actual user,
 * that is, being the owner himself.
 *
 * @param[out] PrimaryGroupIndex
 * Returns the primary group index.
 *
 * @param[out] DefaultOwnerIndex
 * Returns the default owner index.
 *
 * @return
 * Returns STATUS_SUCCESS if the find query operation has completed successfully and that at least one
 * search result is requested by the caller. STATUS_INVALID_PARAMETER is returned if the caller hasn't requested
 * any search result. STATUS_INVALID_OWNER is returned if the specified default user owner does not match with the other
 * user from the token. STATUS_INVALID_PRIMARY_GROUP is returned if the specified default primary group does not match with the
 * other group from the token.
 */
NTSTATUS
SepFindPrimaryGroupAndDefaultOwner(
    _In_ PTOKEN Token,
    _In_ PSID PrimaryGroup,
    _In_opt_ PSID DefaultOwner,
    _Out_opt_ PULONG PrimaryGroupIndex,
    _Out_opt_ PULONG DefaultOwnerIndex)
{
    ULONG i;

    /* We should return at least a search result */
    if (!PrimaryGroupIndex && !DefaultOwnerIndex)
        return STATUS_INVALID_PARAMETER;

    if (PrimaryGroupIndex)
    {
        /* Initialize with an invalid index */
        // Token->PrimaryGroup = NULL;
        *PrimaryGroupIndex = Token->UserAndGroupCount;
    }

    if (DefaultOwnerIndex)
    {
        if (DefaultOwner)
        {
            /* An owner is specified: check whether this is actually the user */
            if (RtlEqualSid(Token->UserAndGroups[0].Sid, DefaultOwner))
            {
                /*
                 * It's the user (first element in array): set it
                 * as the owner and stop the search for it.
                 */
                *DefaultOwnerIndex = 0;
                DefaultOwnerIndex = NULL;
            }
            else
            {
                /* An owner is specified: initialize with an invalid index */
                *DefaultOwnerIndex = Token->UserAndGroupCount;
            }
        }
        else
        {
            /*
             * No owner specified: set the user (first element in array)
             * as the owner and stop the search for it.
             */
            *DefaultOwnerIndex = 0;
            DefaultOwnerIndex = NULL;
        }
    }

    /* Validate and set the primary group and default owner indices */
    for (i = 0; i < Token->UserAndGroupCount; i++)
    {
        /* Stop the search if we have found what we searched for */
        if (!PrimaryGroupIndex && !DefaultOwnerIndex)
            break;

        if (DefaultOwnerIndex && DefaultOwner &&
            RtlEqualSid(Token->UserAndGroups[i].Sid, DefaultOwner) &&
            (Token->UserAndGroups[i].Attributes & SE_GROUP_OWNER))
        {
            /* Owner is found, stop the search for it */
            *DefaultOwnerIndex = i;
            DefaultOwnerIndex = NULL;
        }

        if (PrimaryGroupIndex &&
            RtlEqualSid(Token->UserAndGroups[i].Sid, PrimaryGroup))
        {
            /* Primary group is found, stop the search for it */
            // Token->PrimaryGroup = Token->UserAndGroups[i].Sid;
            *PrimaryGroupIndex = i;
            PrimaryGroupIndex = NULL;
        }
    }

    if (DefaultOwnerIndex)
    {
        if (*DefaultOwnerIndex == Token->UserAndGroupCount)
            return STATUS_INVALID_OWNER;
    }

    if (PrimaryGroupIndex)
    {
        if (*PrimaryGroupIndex == Token->UserAndGroupCount)
        // if (Token->PrimaryGroup == NULL)
            return STATUS_INVALID_PRIMARY_GROUP;
    }

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Subtracts a token in exchange of duplicating a new one.
 *
 * @param[in] ParentToken
 * The parent access token for duplication.
 *
 * @param[out] Token
 * The new duplicated token.
 *
 * @param[in] InUse
 * Set this to TRUE if the token is about to be used immediately after the call execution
 * of this function, FALSE otherwise.
 *
 * @param[in] SessionId
 * Session ID for the token to be assigned.
 *
 * @return
 * Returns STATUS_SUCCESS if token subtracting and duplication have completed successfully.
 * A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SeSubProcessToken(
    _In_ PTOKEN ParentToken,
    _Out_ PTOKEN *Token,
    _In_ BOOLEAN InUse,
    _In_ ULONG SessionId)
{
    PTOKEN NewToken;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    /* Initialize the attributes and duplicate it */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    Status = SepDuplicateToken(ParentToken,
                               &ObjectAttributes,
                               FALSE,
                               TokenPrimary,
                               ParentToken->ImpersonationLevel,
                               KernelMode,
                               &NewToken);
    if (NT_SUCCESS(Status))
    {
        /* Insert it */
        Status = ObInsertObject(NewToken,
                                NULL,
                                0,
                                0,
                                NULL,
                                NULL);
        if (NT_SUCCESS(Status))
        {
            /* Set the session ID */
            NewToken->SessionId = SessionId;
            NewToken->TokenInUse = InUse;

            /* Return the token */
            *Token = NewToken;
        }
    }

    /* Return status */
    return Status;
}

/**
 * @brief
 * Checks if the token is a child of the other token
 * of the current process that the calling thread is invoking this function.
 *
 * @param[in] Token
 * An access token to determine if it's a child or not.
 *
 * @param[out] IsChild
 * The returned boolean result.
 *
 * @return
 * Returns STATUS_SUCCESS when the function finishes its operation. STATUS_UNSUCCESSFUL is
 * returned if primary token of the current calling process couldn't be referenced otherwise.
 */
NTSTATUS
NTAPI
SeIsTokenChild(
    _In_ PTOKEN Token,
    _Out_ PBOOLEAN IsChild)
{
    PTOKEN ProcessToken;
    LUID ProcessTokenId, CallerParentId;

    /* Assume failure */
    *IsChild = FALSE;

    /* Reference the process token */
    ProcessToken = PsReferencePrimaryToken(PsGetCurrentProcess());
    if (!ProcessToken)
        return STATUS_UNSUCCESSFUL;

    /* Get its token ID */
    ProcessTokenId = ProcessToken->TokenId;

    /* Dereference the token */
    ObFastDereferenceObject(&PsGetCurrentProcess()->Token, ProcessToken);

    /* Get our parent token ID */
    CallerParentId = Token->ParentTokenId;

    /* Compare the token IDs */
    if (RtlEqualLuid(&CallerParentId, &ProcessTokenId))
        *IsChild = TRUE;

    /* Return success */
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Checks if the token is a sibling of the other token of
 * the current process that the calling thread is invoking this function.
 *
 * @param[in] Token
 * An access token to determine if it's a sibling or not.
 *
 * @param[out] IsSibling
 * The returned boolean result.
 *
 * @return
 * Returns STATUS_SUCCESS when the function finishes its operation. STATUS_UNSUCCESSFUL is
 * returned if primary token of the current calling process couldn't be referenced otherwise.
 */
NTSTATUS
NTAPI
SeIsTokenSibling(
    _In_ PTOKEN Token,
    _Out_ PBOOLEAN IsSibling)
{
    PTOKEN ProcessToken;
    LUID ProcessParentId, ProcessAuthId;
    LUID CallerParentId, CallerAuthId;

    /* Assume failure */
    *IsSibling = FALSE;

    /* Reference the process token */
    ProcessToken = PsReferencePrimaryToken(PsGetCurrentProcess());
    if (!ProcessToken)
        return STATUS_UNSUCCESSFUL;

    /* Get its parent and authentication IDs */
    ProcessParentId = ProcessToken->ParentTokenId;
    ProcessAuthId = ProcessToken->AuthenticationId;

    /* Dereference the token */
    ObFastDereferenceObject(&PsGetCurrentProcess()->Token, ProcessToken);

    /* Get our parent and authentication IDs */
    CallerParentId = Token->ParentTokenId;
    CallerAuthId = Token->AuthenticationId;

    /* Compare the token IDs */
    if (RtlEqualLuid(&CallerParentId, &ProcessParentId) &&
        RtlEqualLuid(&CallerAuthId, &ProcessAuthId))
    {
        *IsSibling = TRUE;
    }

    /* Return success */
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Copies an existing access token (technically duplicating a new one).
 *
 * @param[in] Token
 * Token to copy.
 *
 * @param[in] Level
 * Impersonation security level to assign to the newly copied token.
 *
 * @param[in] PreviousMode
 * Processor request level mode.
 *
 * @param[out] NewToken
 * The newly copied token.
 *
 * @return
 * Returns STATUS_SUCCESS when token copying has finished successfully. A failure
 * NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SeCopyClientToken(
    _In_ PACCESS_TOKEN Token,
    _In_ SECURITY_IMPERSONATION_LEVEL Level,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PACCESS_TOKEN* NewToken)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;

    PAGED_CODE();

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    Status = SepDuplicateToken(Token,
                               &ObjectAttributes,
                               FALSE,
                               TokenImpersonation,
                               Level,
                               PreviousMode,
                               (PTOKEN*)NewToken);

    return Status;
}

/**
 * @brief
 * Determines if a token is a sandbox inert token or not,
 * based upon the token flags.
 *
 * @param[in] Token
 * A valid access token to determine if such token is inert.
 *
 * @return
 * Returns TRUE if the token is inert, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeTokenIsInert(
    _In_ PTOKEN Token)
{
    PAGED_CODE();

    return (((PTOKEN)Token)->TokenFlags & TOKEN_SANDBOX_INERT) != 0;
}

/**
 * @brief
 * Internal function that deals with access token object destruction and deletion.
 * The function is used solely by the object manager mechanism that handles the life
 * management of a token object.
 *
 * @param[in] ObjectBody
 * The object body that represents an access token object.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SepDeleteToken(
    _In_ PVOID ObjectBody)
{
    NTSTATUS Status;
    PTOKEN AccessToken = (PTOKEN)ObjectBody;

    DPRINT("SepDeleteToken()\n");

    /* Remove the referenced logon session from token */
    if (AccessToken->LogonSession)
    {
        Status = SepRmRemoveLogonSessionFromToken(AccessToken);
        if (!NT_SUCCESS(Status))
        {
            /* Something seriously went wrong */
            DPRINT1("SepDeleteToken(): Failed to remove the logon session from token (Status: 0x%lx)\n", Status);
            return;
        }
    }

    /* Dereference the logon session */
    if ((AccessToken->TokenFlags & TOKEN_SESSION_NOT_REFERENCED) == 0)
        SepRmDereferenceLogonSession(&AccessToken->AuthenticationId);

    /* Delete the token lock */
    if (AccessToken->TokenLock)
        SepDeleteTokenLock(AccessToken);

    /* Delete the dynamic information area */
    if (AccessToken->DynamicPart)
        ExFreePoolWithTag(AccessToken->DynamicPart, TAG_TOKEN_DYNAMIC);
}

/**
 * @brief
 * Internal function that initializes critical kernel data for access
 * token implementation in SRM.
 *
 * @return
 * Nothing.
 */
CODE_SEG("INIT")
VOID
NTAPI
SepInitializeTokenImplementation(VOID)
{
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;

    DPRINT("Creating Token Object Type\n");

    /* Initialize the Token type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Token");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(TOKEN);
    ObjectTypeInitializer.GenericMapping = SepTokenMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.ValidAccessMask = TOKEN_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.DeleteProcedure = SepDeleteToken;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &SeTokenObjectType);
}

/**
 * @brief
 * Assigns a primary access token to a given process.
 *
 * @param[in] Process
 * Process where the token is about to be assigned.
 *
 * @param[in] Token
 * The token to be assigned.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeAssignPrimaryToken(
    _In_ PEPROCESS Process,
    _In_ PTOKEN Token)
{
    PAGED_CODE();

    /* Sanity checks */
    ASSERT(Token->TokenType == TokenPrimary);
    ASSERT(!Token->TokenInUse);

    /* Clean any previous token */
    if (Process->Token.Object) SeDeassignPrimaryToken(Process);

    /* Set the new token */
    ObReferenceObject(Token);
    Token->TokenInUse = TRUE;
    ObInitializeFastReference(&Process->Token, Token);
}

/**
 * @brief
 * Retrieves token control information.
 *
 * @param[in] _Token
 * A valid token object.
 *
 * @param[out] SecurityDescriptor
 * The returned token control information.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeGetTokenControlInformation(
    _In_ PACCESS_TOKEN _Token,
    _Out_ PTOKEN_CONTROL TokenControl)
{
    PTOKEN Token = _Token;
    PAGED_CODE();

    /* Capture the main fields */
    TokenControl->AuthenticationId = Token->AuthenticationId;
    TokenControl->TokenId = Token->TokenId;
    TokenControl->TokenSource = Token->TokenSource;

    /* Lock the token */
    SepAcquireTokenLockShared(Token);

    /* Capture the modified ID */
    TokenControl->ModifiedId = Token->ModifiedId;

    /* Unlock it */
    SepReleaseTokenLock(Token);
}

/**
 * @brief
 * Creates the system process token.
 *
 * @return
 * Returns the system process token if the operations have
 * completed successfully.
 */
CODE_SEG("INIT")
PTOKEN
NTAPI
SepCreateSystemProcessToken(VOID)
{
    LUID_AND_ATTRIBUTES Privileges[25];
    ULONG GroupAttributes, OwnerAttributes;
    SID_AND_ATTRIBUTES Groups[32];
    LARGE_INTEGER Expiration;
    SID_AND_ATTRIBUTES UserSid;
    ULONG GroupsLength;
    PSID PrimaryGroup;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PSID Owner;
    ULONG i;
    PTOKEN Token;
    NTSTATUS Status;

    /* Don't ever expire */
    Expiration.QuadPart = -1;

    /* All groups mandatory and enabled */
    GroupAttributes = SE_GROUP_ENABLED | SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT;
    OwnerAttributes = SE_GROUP_ENABLED | SE_GROUP_OWNER | SE_GROUP_ENABLED_BY_DEFAULT;

    /* User is Local System */
    UserSid.Sid = SeLocalSystemSid;
    UserSid.Attributes = 0;

    /* Primary group is Local System */
    PrimaryGroup = SeLocalSystemSid;

    /* Owner is Administrators */
    Owner = SeAliasAdminsSid;

    /* Groups are Administrators, World, and Authenticated Users */
    Groups[0].Sid = SeAliasAdminsSid;
    Groups[0].Attributes = OwnerAttributes;
    Groups[1].Sid = SeWorldSid;
    Groups[1].Attributes = GroupAttributes;
    Groups[2].Sid = SeAuthenticatedUsersSid;
    Groups[2].Attributes = GroupAttributes;
    GroupsLength = sizeof(SID_AND_ATTRIBUTES) +
                   SeLengthSid(Groups[0].Sid) +
                   SeLengthSid(Groups[1].Sid) +
                   SeLengthSid(Groups[2].Sid);
    ASSERT(GroupsLength <= sizeof(Groups));

    /* Setup the privileges */
    i = 0;
    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeTcbPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeCreateTokenPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeTakeOwnershipPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeCreatePagefilePrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeLockMemoryPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeAssignPrimaryTokenPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeIncreaseQuotaPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeIncreaseBasePriorityPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeCreatePermanentPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeDebugPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeAuditPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeSecurityPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeSystemEnvironmentPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeChangeNotifyPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeBackupPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeRestorePrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeShutdownPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeLoadDriverPrivilege;

    Privileges[i].Attributes = SE_PRIVILEGE_ENABLED_BY_DEFAULT | SE_PRIVILEGE_ENABLED;
    Privileges[i++].Luid = SeProfileSingleProcessPrivilege;

    Privileges[i].Attributes = 0;
    Privileges[i++].Luid = SeSystemtimePrivilege;
    ASSERT(i == 20);

    /* Setup the object attributes */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    ASSERT(SeSystemDefaultDacl != NULL);

    /* Create the token */
    Status = SepCreateToken((PHANDLE)&Token,
                            KernelMode,
                            0,
                            &ObjectAttributes,
                            TokenPrimary,
                            SecurityAnonymous,
                            &SeSystemAuthenticationId,
                            &Expiration,
                            &UserSid,
                            3,
                            Groups,
                            GroupsLength,
                            20,
                            Privileges,
                            Owner,
                            PrimaryGroup,
                            SeSystemDefaultDacl,
                            &SeSystemTokenSource,
                            TRUE);
    ASSERT(Status == STATUS_SUCCESS);

    /* Return the token */
    return Token;
}

/**
 * @brief
 * Creates the anonymous logon token for the system. The difference between this
 * token and the other one is the inclusion of everyone SID group (being SeWorldSid).
 * The other token lacks such group.
 *
 * @return
 * Returns the system's anonymous logon token if the operations have
 * completed successfully.
 */
CODE_SEG("INIT")
PTOKEN
SepCreateSystemAnonymousLogonToken(VOID)
{
    SID_AND_ATTRIBUTES Groups[32], UserSid;
    PSID PrimaryGroup;
    PTOKEN Token;
    ULONG GroupsLength;
    LARGE_INTEGER Expiration;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    /* The token never expires */
    Expiration.QuadPart = -1;

    /* The user is the anonymous logon */
    UserSid.Sid = SeAnonymousLogonSid;
    UserSid.Attributes = 0;

    /* The primary group is also the anonymous logon */
    PrimaryGroup = SeAnonymousLogonSid;

    /* The only group for the token is the World */
    Groups[0].Sid = SeWorldSid;
    Groups[0].Attributes = SE_GROUP_ENABLED | SE_GROUP_MANDATORY | SE_GROUP_ENABLED_BY_DEFAULT;
    GroupsLength = sizeof(SID_AND_ATTRIBUTES) +
                   SeLengthSid(Groups[0].Sid);
    ASSERT(GroupsLength <= sizeof(Groups));

    /* Initialise the object attributes for the token */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    ASSERT(SeSystemAnonymousLogonDacl != NULL);

    /* Create token */
    Status = SepCreateToken((PHANDLE)&Token,
                            KernelMode,
                            0,
                            &ObjectAttributes,
                            TokenPrimary,
                            SecurityAnonymous,
                            &SeAnonymousAuthenticationId,
                            &Expiration,
                            &UserSid,
                            1,
                            Groups,
                            GroupsLength,
                            0,
                            NULL,
                            NULL,
                            PrimaryGroup,
                            SeSystemAnonymousLogonDacl,
                            &SeSystemTokenSource,
                            TRUE);
    ASSERT(Status == STATUS_SUCCESS);

    /* Return the anonymous logon token */
    return Token;
}

/**
 * @brief
 * Creates the anonymous logon token for the system. This kind of token
 * doesn't include the everyone SID group (being SeWorldSid).
 *
 * @return
 * Returns the system's anonymous logon token if the operations have
 * completed successfully.
 */
CODE_SEG("INIT")
PTOKEN
SepCreateSystemAnonymousLogonTokenNoEveryone(VOID)
{
    SID_AND_ATTRIBUTES UserSid;
    PSID PrimaryGroup;
    PTOKEN Token;
    LARGE_INTEGER Expiration;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    /* The token never expires */
    Expiration.QuadPart = -1;

    /* The user is the anonymous logon */
    UserSid.Sid = SeAnonymousLogonSid;
    UserSid.Attributes = 0;

    /* The primary group is also the anonymous logon */
    PrimaryGroup = SeAnonymousLogonSid;

    /* Initialise the object attributes for the token */
    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);
    ASSERT(SeSystemAnonymousLogonDacl != NULL);

    /* Create token */
    Status = SepCreateToken((PHANDLE)&Token,
                            KernelMode,
                            0,
                            &ObjectAttributes,
                            TokenPrimary,
                            SecurityAnonymous,
                            &SeAnonymousAuthenticationId,
                            &Expiration,
                            &UserSid,
                            0,
                            NULL,
                            0,
                            0,
                            NULL,
                            NULL,
                            PrimaryGroup,
                            SeSystemAnonymousLogonDacl,
                            &SeSystemTokenSource,
                            TRUE);
    ASSERT(Status == STATUS_SUCCESS);

    /* Return the anonymous (not including everyone) logon token */
    return Token;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Queries the session ID of an access token.
 *
 * @param[in] Token
 * A valid access token where the session ID has to be gathered.
 *
 * @param[out] pSessionId
 * The returned pointer to a session ID to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
NTSTATUS
NTAPI
SeQuerySessionIdToken(
    _In_ PACCESS_TOKEN Token,
    _Out_ PULONG pSessionId)
{
    PAGED_CODE();

    /* Lock the token */
    SepAcquireTokenLockShared(Token);

    *pSessionId = ((PTOKEN)Token)->SessionId;

    /* Unlock the token */
    SepReleaseTokenLock(Token);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Queries the authentication ID of an access token.
 *
 * @param[in] Token
 * A valid access token where the authentication ID has to be gathered.
 *
 * @param[out] pSessionId
 * The returned pointer to an authentication ID to the caller.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
NTSTATUS
NTAPI
SeQueryAuthenticationIdToken(
    _In_ PACCESS_TOKEN Token,
    _Out_ PLUID LogonId)
{
    PAGED_CODE();

    *LogonId = ((PTOKEN)Token)->AuthenticationId;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Gathers the security impersonation level of an access token.
 *
 * @param[in] Token
 * A valid access token where the impersonation level has to be gathered.
 *
 * @return
 * Returns the security impersonation level from a valid token.
 */
SECURITY_IMPERSONATION_LEVEL
NTAPI
SeTokenImpersonationLevel(
    _In_ PACCESS_TOKEN Token)
{
    PAGED_CODE();

    return ((PTOKEN)Token)->ImpersonationLevel;
}

/**
 * @brief
 * Gathers the token type of an access token. A token ca be either
 * a primary token or impersonation token.
 *
 * @param[in] Token
 * A valid access token where the token type has to be gathered.
 *
 * @return
 * Returns the token type from a valid token.
 */
TOKEN_TYPE
NTAPI
SeTokenType(
    _In_ PACCESS_TOKEN Token)
{
    PAGED_CODE();

    return ((PTOKEN)Token)->TokenType;
}

/**
 * @brief
 * Determines if a token is either an admin token or not. Such
 * condition is checked based upon TOKEN_HAS_ADMIN_GROUP flag,
 * which means if the respective access token belongs to an
 * administrator group or not.
 *
 * @param[in] Token
 * A valid access token to determine if such token is admin or not.
 *
 * @return
 * Returns TRUE if the token is an admin one, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeTokenIsAdmin(
    _In_ PACCESS_TOKEN Token)
{
    PAGED_CODE();

    // NOTE: Win7+ instead really checks the list of groups in the token
    // (since TOKEN_HAS_ADMIN_GROUP == TOKEN_WRITE_RESTRICTED ...)
    return (((PTOKEN)Token)->TokenFlags & TOKEN_HAS_ADMIN_GROUP) != 0;
}

/**
 * @brief
 * Determines if a token is restricted or not, based upon the token
 * flags.
 *
 * @param[in] Token
 * A valid access token to determine if such token is restricted.
 *
 * @return
 * Returns TRUE if the token is restricted, FALSE otherwise.
 */
BOOLEAN
NTAPI
SeTokenIsRestricted(
    _In_ PACCESS_TOKEN Token)
{
    PAGED_CODE();

    return (((PTOKEN)Token)->TokenFlags & TOKEN_IS_RESTRICTED) != 0;
}

/**
 * @brief
 * Determines if a token is write restricted, that is, nobody can write anything
 * to it.
 *
 * @param[in] Token
 * A valid access token to determine if such token is write restricted.
 *
 * @return
 * Returns TRUE if the token is write restricted, FALSE otherwise.
 *
 * @remarks
 * First introduced in NT 5.1 SP2 x86 (5.1.2600.2622), absent in NT 5.2,
 * then finally re-introduced in Vista+.
 */
BOOLEAN
NTAPI
SeTokenIsWriteRestricted(
    _In_ PACCESS_TOKEN Token)
{
    PAGED_CODE();

    // NOTE: NT 5.1 SP2 x86 checks the SE_BACKUP_PRIVILEGES_CHECKED flag
    // while Vista+ checks the TOKEN_WRITE_RESTRICTED flag as one expects.
    return (((PTOKEN)Token)->TokenFlags & SE_BACKUP_PRIVILEGES_CHECKED) != 0;
}

/**
 * @brief
 * Ensures that client impersonation can occur by checking if the token
 * we're going to assign as the impersonation token can be actually impersonated
 * in the first place. The routine is used primarily by PsImpersonateClient.
 *
 * @param[in] ProcessToken
 * Token from a process.
 *
 * @param[in] TokenToImpersonate
 * Token that we are going to impersonate.
 *
 * @param[in] ImpersonationLevel
 * Security impersonation level grade.
 *
 * @return
 * Returns TRUE if the conditions checked are met for token impersonation,
 * FALSE otherwise.
 */
BOOLEAN
NTAPI
SeTokenCanImpersonate(
    _In_ PTOKEN ProcessToken,
    _In_ PTOKEN TokenToImpersonate,
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    BOOLEAN CanImpersonate;
    PAGED_CODE();

    /*
     * SecurityAnonymous and SecurityIdentification levels do not
     * allow impersonation.
     */
    if (ImpersonationLevel == SecurityAnonymous ||
        ImpersonationLevel == SecurityIdentification)
    {
        return FALSE;
    }

    /* Time to lock our tokens */
    SepAcquireTokenLockShared(ProcessToken);
    SepAcquireTokenLockShared(TokenToImpersonate);

    /* What kind of authentication ID does the token have? */
    if (RtlEqualLuid(&TokenToImpersonate->AuthenticationId,
                     &SeAnonymousAuthenticationId))
    {
        /*
         * OK, it looks like the token has an anonymous
         * authentication. Is that token created by the system?
         */
        if (TokenToImpersonate->TokenSource.SourceName != SeSystemTokenSource.SourceName &&
            !RtlEqualLuid(&TokenToImpersonate->TokenSource.SourceIdentifier, &SeSystemTokenSource.SourceIdentifier))
        {
            /* It isn't, we can't impersonate regular tokens */
            DPRINT("SeTokenCanImpersonate(): Token has an anonymous authentication ID, can't impersonate!\n");
            CanImpersonate = FALSE;
            goto Quit;
        }
    }

    /* Are the SID values from both tokens equal? */
    if (!RtlEqualSid(ProcessToken->UserAndGroups->Sid,
                     TokenToImpersonate->UserAndGroups->Sid))
    {
        /* They aren't, bail out */
        DPRINT("SeTokenCanImpersonate(): Tokens SIDs are not equal!\n");
        CanImpersonate = FALSE;
        goto Quit;
    }

    /*
     * Make sure the tokens aren't diverged in terms of
     * restrictions, that is, one token is restricted
     * but the other one isn't.
     */
    if (SeTokenIsRestricted(ProcessToken) !=
        SeTokenIsRestricted(TokenToImpersonate))
    {
        /*
         * One token is restricted so we cannot
         * continue further at this point, bail out.
         */
        DPRINT("SeTokenCanImpersonate(): One token is restricted, can't continue!\n");
        CanImpersonate = FALSE;
        goto Quit;
    }

    /* If we've reached that far then we can impersonate! */
    DPRINT("SeTokenCanImpersonate(): We can impersonate.\n");
    CanImpersonate = TRUE;

Quit:
    /* We're done, unlock the tokens now */
    SepReleaseTokenLock(ProcessToken);
    SepReleaseTokenLock(TokenToImpersonate);

    return CanImpersonate;
}

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Opens a token that is tied to a thread handle.
 *
 * @param[out] ThreadHandle
 * Thread handle where the token is about to be opened.
 *
 * @param[in] DesiredAccess
 * The request access right for the token.
 *
 * @param[in] OpenAsSelf
 * If set to TRUE, the access check will be made with the security context
 * of the process of the calling thread (opening as self). Otherwise the access
 * check will be made with the security context of the calling thread instead.
 *
 * @param[in] HandleAttributes
 * Handle attributes for the opened thread token handle.
 *
 * @param[out] TokenHandle
 * The opened token handle returned to the caller for use.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully opened the thread
 * token. STATUS_CANT_OPEN_ANONYMOUS is returned if a token has SecurityAnonymous
 * as impersonation level and we cannot open it. A failure NTSTATUS code is returned
 * otherwise.
 */
NTSTATUS
NTAPI
NtOpenThreadTokenEx(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _In_ ULONG HandleAttributes,
    _Out_ PHANDLE TokenHandle)
{
    PETHREAD Thread;
    HANDLE hToken;
    PTOKEN Token, NewToken = NULL, PrimaryToken;
    BOOLEAN CopyOnOpen, EffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    SE_IMPERSONATION_STATE ImpersonationState;
    OBJECT_ATTRIBUTES ObjectAttributes;
    SECURITY_DESCRIPTOR SecurityDescriptor;
    PACL Dacl = NULL;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    BOOLEAN RestoreImpersonation = FALSE;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteHandle(TokenHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Validate object attributes */
    HandleAttributes = ObpValidateAttributes(HandleAttributes, PreviousMode);

    /*
     * At first open the thread token for information access and verify
     * that the token associated with thread is valid.
     */

    Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_QUERY_INFORMATION,
                                       PsThreadType, PreviousMode, (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Token = PsReferenceImpersonationToken(Thread, &CopyOnOpen, &EffectiveOnly,
                                          &ImpersonationLevel);
    if (Token == NULL)
    {
        ObDereferenceObject(Thread);
        return STATUS_NO_TOKEN;
    }

    if (ImpersonationLevel == SecurityAnonymous)
    {
        PsDereferenceImpersonationToken(Token);
        ObDereferenceObject(Thread);
        return STATUS_CANT_OPEN_ANONYMOUS;
    }

    /*
     * Revert to self if OpenAsSelf is specified.
     */

    if (OpenAsSelf)
    {
        RestoreImpersonation = PsDisableImpersonation(PsGetCurrentThread(),
                                                      &ImpersonationState);
    }

    if (CopyOnOpen)
    {
        PrimaryToken = PsReferencePrimaryToken(Thread->ThreadsProcess);

        Status = SepCreateImpersonationTokenDacl(Token, PrimaryToken, &Dacl);

        ObFastDereferenceObject(&Thread->ThreadsProcess->Token, PrimaryToken);

        if (NT_SUCCESS(Status))
        {
            if (Dacl)
            {
                Status = RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                                     SECURITY_DESCRIPTOR_REVISION);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("NtOpenThreadTokenEx(): Failed to create a security descriptor (Status 0x%lx)\n", Status);
                }

                Status = RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl,
                                                      FALSE);
                if (!NT_SUCCESS(Status))
                {
                    DPRINT1("NtOpenThreadTokenEx(): Failed to set a DACL to the security descriptor (Status 0x%lx)\n", Status);
                }
            }

            InitializeObjectAttributes(&ObjectAttributes, NULL, HandleAttributes,
                                       NULL, Dacl ? &SecurityDescriptor : NULL);

            Status = SepDuplicateToken(Token, &ObjectAttributes, EffectiveOnly,
                                       TokenImpersonation, ImpersonationLevel,
                                       KernelMode, &NewToken);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtOpenThreadTokenEx(): Failed to duplicate the token (Status 0x%lx)\n", Status);
            }

            ObReferenceObject(NewToken);
            Status = ObInsertObject(NewToken, NULL, DesiredAccess, 0, NULL,
                                    &hToken);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("NtOpenThreadTokenEx(): Failed to insert the token object (Status 0x%lx)\n", Status);
            }
        }
        else
        {
            DPRINT1("NtOpenThreadTokenEx(): Failed to impersonate token from DACL (Status 0x%lx)\n", Status);
        }
    }
    else
    {
        Status = ObOpenObjectByPointer(Token, HandleAttributes,
                                       NULL, DesiredAccess, SeTokenObjectType,
                                       PreviousMode, &hToken);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtOpenThreadTokenEx(): Failed to open the object (Status 0x%lx)\n", Status);
        }
    }

    if (Dacl) ExFreePoolWithTag(Dacl, TAG_ACL);

    if (RestoreImpersonation)
    {
        PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }

    ObDereferenceObject(Token);

    if (NT_SUCCESS(Status) && CopyOnOpen)
    {
        Status = PsImpersonateClient(Thread, NewToken, FALSE, EffectiveOnly, ImpersonationLevel);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("NtOpenThreadTokenEx(): Failed to impersonate the client (Status 0x%lx)\n", Status);
        }
    }

    if (NewToken) ObDereferenceObject(NewToken);

    ObDereferenceObject(Thread);

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

    return Status;
}

/**
 * @brief
 * Opens a token that is tied to a thread handle.
 *
 * @param[out] ThreadHandle
 * Thread handle where the token is about to be opened.
 *
 * @param[in] DesiredAccess
 * The request access right for the token.
 *
 * @param[in] OpenAsSelf
 * If set to TRUE, the access check will be made with the security context
 * of the process of the calling thread (opening as self). Otherwise the access
 * check will be made with the security context of the calling thread instead.
 *
 * @param[out] TokenHandle
 * The opened token handle returned to the caller for use.
 *
 * @return
 * See NtOpenThreadTokenEx.
 */
NTSTATUS
NTAPI
NtOpenThreadToken(
    _In_ HANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ BOOLEAN OpenAsSelf,
    _Out_ PHANDLE TokenHandle)
{
    return NtOpenThreadTokenEx(ThreadHandle, DesiredAccess, OpenAsSelf, 0,
                               TokenHandle);
}

/**
 * @brief
 * Compares tokens if they're equal or not.
 *
 * @param[in] FirstToken
 * The first token.
 *
 * @param[in] SecondToken
 * The second token.
 *
 * @param[out] Equal
 * The retrieved value which determines if the tokens are
 * equal or not.
 *
 * @return
 * Returns STATUS_SUCCESS, otherwise it returns a failure NTSTATUS code.
 */
NTSTATUS
NTAPI
NtCompareTokens(
    _In_ HANDLE FirstTokenHandle,
    _In_ HANDLE SecondTokenHandle,
    _Out_ PBOOLEAN Equal)
{
    KPROCESSOR_MODE PreviousMode;
    PTOKEN FirstToken, SecondToken;
    BOOLEAN IsEqual;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWriteBoolean(Equal);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    Status = ObReferenceObjectByHandle(FirstTokenHandle,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&FirstToken,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObReferenceObjectByHandle() failed (Status 0x%lx)\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(SecondTokenHandle,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&SecondToken,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObReferenceObjectByHandle() failed (Status 0x%lx)\n", Status);
        ObDereferenceObject(FirstToken);
        return Status;
    }

    if (FirstToken != SecondToken)
    {
        Status = SepCompareTokens(FirstToken,
                                  SecondToken,
                                  &IsEqual);
    }
    else
    {
        IsEqual = TRUE;
    }

    ObDereferenceObject(SecondToken);
    ObDereferenceObject(FirstToken);

    if (NT_SUCCESS(Status))
    {
        _SEH2_TRY
        {
            *Equal = IsEqual;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}

/**
 * @brief
 * Allows the calling thread to impersonate the system's anonymous
 * logon token.
 *
 * @param[in] ThreadHandle
 * A handle to the thread to start the procedure of logon token
 * impersonation. The thread must have the THREAD_IMPERSONATE
 * access right.
 *
 * @return
 * Returns STATUS_SUCCESS if the thread has successfully impersonated the
 * anonymous logon token, otherwise a failure NTSTATUS code is returned.
 *
 * @remarks
 * By default the system gives the opportunity to the caller to impersonate
 * the anonymous logon token without including the Everyone Group SID.
 * In cases where the caller wants to impersonate the token including such
 * group, the EveryoneIncludesAnonymous registry value setting has to be set
 * to 1, from HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Lsa registry
 * path. The calling thread must invoke PsRevertToSelf when impersonation
 * is no longer needed or RevertToSelf if the calling execution is done
 * in user mode.
 */
NTSTATUS
NTAPI
NtImpersonateAnonymousToken(
    _In_ HANDLE ThreadHandle)
{
    PETHREAD Thread;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* Obtain the thread object from the handle */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_IMPERSONATE,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtImpersonateAnonymousToken(): Failed to reference the object (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Call the private routine to impersonate the token */
    Status = SepImpersonateAnonymousToken(Thread, PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtImpersonateAnonymousToken(): Failed to impersonate the token (Status 0x%lx)\n", Status);
    }

    ObDereferenceObject(Thread);
    return Status;
}

/* EOF */
