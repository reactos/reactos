/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security token implementation support
 * COPYRIGHT:       Copyright David Welch <welch@cwcom.net>
 *                  Copyright 2021 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
#include <ntlsa.h>

typedef struct _TOKEN_AUDIT_POLICY_INFORMATION
{
    ULONG PolicyCount;
    struct
    {
        ULONG Category;
        UCHAR Value;
    } Policies[1];
} TOKEN_AUDIT_POLICY_INFORMATION, *PTOKEN_AUDIT_POLICY_INFORMATION;

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

static const INFORMATION_CLASS_INFO SeTokenInformationClass[] = {

    /* Class 0 not used, blame MS! */
    IQS_SAME(0, 0, 0),

    /* TokenUser */
    IQS_SAME(TOKEN_USER, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE),
    /* TokenGroups */
    IQS_SAME(TOKEN_GROUPS, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE),
    /* TokenPrivileges */
    IQS_SAME(TOKEN_PRIVILEGES, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE),
    /* TokenOwner */
    IQS_SAME(TOKEN_OWNER, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE),
    /* TokenPrimaryGroup */
    IQS_SAME(TOKEN_PRIMARY_GROUP, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE),
    /* TokenDefaultDacl */
    IQS_SAME(TOKEN_DEFAULT_DACL, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET | ICIF_SET_SIZE_VARIABLE),
    /* TokenSource */
    IQS_SAME(TOKEN_SOURCE, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE),
    /* TokenType */
    IQS_SAME(TOKEN_TYPE, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenImpersonationLevel */
    IQS_SAME(SECURITY_IMPERSONATION_LEVEL, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenStatistics */
    IQS_SAME(TOKEN_STATISTICS, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE | ICIF_SET_SIZE_VARIABLE),
    /* TokenRestrictedSids */
    IQS_SAME(TOKEN_GROUPS, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenSessionId */
    IQS_SAME(ULONG, ULONG, ICIF_QUERY | ICIF_SET),
    /* TokenGroupsAndPrivileges */
    IQS_SAME(TOKEN_GROUPS_AND_PRIVILEGES, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenSessionReference */
    IQS_SAME(ULONG, ULONG, ICIF_SET | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenSandBoxInert */
    IQS_SAME(ULONG, ULONG, ICIF_QUERY | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenAuditPolicy */
    IQS_SAME(/* FIXME */0, ULONG, ICIF_QUERY | ICIF_SET | ICIF_QUERY_SIZE_VARIABLE),
    /* TokenOrigin */
    IQS_SAME(TOKEN_ORIGIN, ULONG, ICIF_QUERY | ICIF_SET | ICIF_QUERY_SIZE_VARIABLE),
};

/* FUNCTIONS *****************************************************************/

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
static
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
static
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

static
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

static
VOID
SepUpdatePrivilegeFlagsToken(
    _Inout_ PTOKEN Token)
{
    ULONG i;

    /* Loop all privileges */
    for (i = 0; i < Token->PrivilegeCount; i++)
    {
        /* Updates the flags dor this privilege */
        SepUpdateSinglePrivilegeFlagToken(Token, i);
    }
}

static
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

VOID
NTAPI
SepFreeProxyData(PVOID ProxyData)
{
    UNIMPLEMENTED;
}

NTSTATUS
NTAPI
SepCopyProxyData(PVOID* Dest,
                 PVOID Src)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

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

VOID
NTAPI
SeDeassignPrimaryToken(PEPROCESS Process)
{
    PTOKEN OldToken;

    /* Remove the Token */
    OldToken = ObFastReplaceObject(&Process->Token, NULL);

    /* Mark the Old Token as free */
    OldToken->TokenInUse = FALSE;

    /* Dereference the Token */
    ObDereferenceObject(OldToken);
}

static ULONG
RtlLengthSidAndAttributes(ULONG Count,
                          PSID_AND_ATTRIBUTES Src)
{
    ULONG i;
    ULONG uLength;

    PAGED_CODE();

    uLength = Count * sizeof(SID_AND_ATTRIBUTES);
    for (i = 0; i < Count; i++)
        uLength += RtlLengthSid(Src[i].Sid);

    return uLength;
}


static NTSTATUS
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
    ULONG VariableLength;
    ULONG TotalSize;

    PAGED_CODE();

    /* Compute how much size we need to allocate for the token */
    VariableLength = Token->VariableLength;
    TotalSize = FIELD_OFFSET(TOKEN, VariablePart) + VariableLength;

    Status = ObCreateObject(PreviousMode,
                            SeTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            TotalSize,
                            0,
                            0,
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
    RtlCopyLuid(&AccessToken->TokenSource.SourceIdentifier,
                &Token->TokenSource.SourceIdentifier);
    RtlCopyMemory(AccessToken->TokenSource.SourceName,
                  Token->TokenSource.SourceName,
                  sizeof(Token->TokenSource.SourceName));

    AccessToken->AuthenticationId = Token->AuthenticationId;
    AccessToken->ParentTokenId = Token->ParentTokenId;
    AccessToken->ExpirationTime = Token->ExpirationTime;
    AccessToken->OriginatingLogonSession = Token->OriginatingLogonSession;

    /* Lock the source token and copy the mutable fields */
    SepAcquireTokenLockExclusive(Token);

    AccessToken->SessionId = Token->SessionId;
    RtlCopyLuid(&AccessToken->ModifiedId, &Token->ModifiedId);

    AccessToken->TokenFlags = Token->TokenFlags & ~TOKEN_SESSION_NOT_REFERENCED;

    /* Copy and reference the logon session */
    // RtlCopyLuid(&AccessToken->AuthenticationId, &Token->AuthenticationId);
    Status = SepRmReferenceLogonSession(&AccessToken->AuthenticationId);
    if (!NT_SUCCESS(Status))
    {
        /* No logon session could be found, bail out */
        DPRINT1("SepRmReferenceLogonSession() failed (Status 0x%lx)\n", Status);
        /* Set the flag for proper cleanup by the delete procedure */
        AccessToken->TokenFlags |= TOKEN_SESSION_NOT_REFERENCED;
        goto Quit;
    }

    /* Assign the data that reside in the TOKEN's variable information area */
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

#if 1
    {
    ULONG PrimaryGroupIndex;

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
    AccessToken->PrimaryGroup = AccessToken->UserAndGroups[PrimaryGroupIndex].Sid;
    }
#else
    AccessToken->PrimaryGroup = (PVOID)((ULONG_PTR)AccessToken + (ULONG_PTR)Token->PrimaryGroup - (ULONG_PTR)Token->UserAndGroups);
#endif
    AccessToken->DefaultOwnerIndex = Token->DefaultOwnerIndex;

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


    //
    // FIXME: Implement the "EffectiveOnly" option, that removes all
    // the disabled parts (privileges and groups) of the token.
    //


    //
    // NOTE: So far our dynamic area only contains
    // the default dacl, so this makes the following
    // code pretty simple. The day where it stores
    // other data, the code will require adaptations.
    //

    /* Now allocate the TOKEN's dynamic information area and set the data */
    AccessToken->DynamicAvailable = 0; // Unused memory in the dynamic area.
    AccessToken->DynamicPart = NULL;
    if (Token->DynamicPart && Token->DefaultDacl)
    {
        AccessToken->DynamicPart = ExAllocatePoolWithTag(PagedPool,
                                                         Token->DefaultDacl->AclSize,
                                                         TAG_TOKEN_DYNAMIC);
        if (AccessToken->DynamicPart == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quit;
        }
        EndMem = (PVOID)AccessToken->DynamicPart;

        AccessToken->DefaultDacl = EndMem;

        RtlCopyMemory(AccessToken->DefaultDacl,
                      Token->DefaultDacl,
                      Token->DefaultDacl->AclSize);
    }

    /* Unlock the source token */
    SepReleaseTokenLock(Token);

    /* Return the token */
    *NewAccessToken = AccessToken;
    Status = STATUS_SUCCESS;

Quit:
    if (!NT_SUCCESS(Status))
    {
        /* Unlock the source token */
        SepReleaseTokenLock(Token);

        /* Dereference the token, the delete procedure will clean it up */
        ObDereferenceObject(AccessToken);
    }

    return Status;
}

NTSTATUS
NTAPI
SeSubProcessToken(IN PTOKEN ParentToken,
                  OUT PTOKEN *Token,
                  IN BOOLEAN InUse,
                  IN ULONG SessionId)
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

NTSTATUS
NTAPI
SeIsTokenChild(IN PTOKEN Token,
               OUT PBOOLEAN IsChild)
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

NTSTATUS
NTAPI
SeIsTokenSibling(IN PTOKEN Token,
                 OUT PBOOLEAN IsSibling)
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

NTSTATUS
NTAPI
SeCopyClientToken(IN PACCESS_TOKEN Token,
                  IN SECURITY_IMPERSONATION_LEVEL Level,
                  IN KPROCESSOR_MODE PreviousMode,
                  OUT PACCESS_TOKEN* NewToken)
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

VOID
NTAPI
SepDeleteToken(PVOID ObjectBody)
{
    PTOKEN AccessToken = (PTOKEN)ObjectBody;

    DPRINT("SepDeleteToken()\n");

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

VOID
NTAPI
SeAssignPrimaryToken(IN PEPROCESS Process,
                     IN PTOKEN Token)
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
    ULONG TotalSize;
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

    Status = ObCreateObject(PreviousMode,
                            SeTokenObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            TotalSize,
                            0,
                            0,
                            (PVOID*)&AccessToken);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ObCreateObject() failed (Status 0x%lx)\n", Status);
        return Status;
    }

    /* Zero out the buffer and initialize the token */
    RtlZeroMemory(AccessToken, TotalSize);

    RtlCopyLuid(&AccessToken->TokenId, &TokenId);

    AccessToken->TokenType = TokenType;
    AccessToken->ImpersonationLevel = ImpersonationLevel;

    /* Initialise the lock for the access token */
    Status = SepCreateTokenLock(AccessToken);
    if (!NT_SUCCESS(Status))
        goto Quit;

    RtlCopyLuid(&AccessToken->TokenSource.SourceIdentifier,
                &TokenSource->SourceIdentifier);
    RtlCopyMemory(AccessToken->TokenSource.SourceName,
                  TokenSource->SourceName,
                  sizeof(TokenSource->SourceName));

    AccessToken->ExpirationTime = *ExpirationTime;
    RtlCopyLuid(&AccessToken->ModifiedId, &ModifiedId);

    AccessToken->TokenFlags = TokenFlags & ~TOKEN_SESSION_NOT_REFERENCED;

    /* Copy and reference the logon session */
    RtlCopyLuid(&AccessToken->AuthenticationId, AuthenticationId);
    Status = SepRmReferenceLogonSession(&AccessToken->AuthenticationId);
    if (!NT_SUCCESS(Status))
    {
        /* No logon session could be found, bail out */
        DPRINT1("SepRmReferenceLogonSession() failed (Status 0x%lx)\n", Status);
        /* Set the flag for proper cleanup by the delete procedure */
        AccessToken->TokenFlags |= TOKEN_SESSION_NOT_REFERENCED;
        goto Quit;
    }

    /* Assign the data that reside in the TOKEN's variable information area */
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

    AccessToken->PrimaryGroup = AccessToken->UserAndGroups[PrimaryGroupIndex].Sid;
    AccessToken->DefaultOwnerIndex = DefaultOwnerIndex;

    /* Now allocate the TOKEN's dynamic information area and set the data */
    AccessToken->DynamicAvailable = 0; // Unused memory in the dynamic area.
    AccessToken->DynamicPart = NULL;
    if (DefaultDacl != NULL)
    {
        AccessToken->DynamicPart = ExAllocatePoolWithTag(PagedPool,
                                                         DefaultDacl->AclSize,
                                                         TAG_TOKEN_DYNAMIC);
        if (AccessToken->DynamicPart == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Quit;
        }
        EndMem = (PVOID)AccessToken->DynamicPart;

        AccessToken->DefaultDacl = EndMem;

        RtlCopyMemory(AccessToken->DefaultDacl,
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

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeFilterToken(IN PACCESS_TOKEN ExistingToken,
              IN ULONG Flags,
              IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
              IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
              IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
              OUT PACCESS_TOKEN * FilteredToken)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Queries information details about the given token to the call. The difference
 * between NtQueryInformationToken and this routine is that the system call has
 * user mode buffer data probing and additional protection checks whereas this
 * routine doesn't have any of these. The routine is used exclusively in kernel
 * mode.
 *
 * @param[in] AccessToken
 * An access token to be given.
 *
 * @param[in] TokenInformationClass
 * Token information class.
 *
 * @param[out] TokenInformation
 * Buffer with retrieved information. Such information is arbitrary, depending
 * on the requested information class.
 *
 * @return
 * Returns STATUS_SUCCESS if the operation to query the desired information
 * has completed successfully. STATUS_INSUFFICIENT_RESOURCES is returned if
 * pool memory allocation has failed to satisfy an operation. Otherwise
 * STATUS_INVALID_INFO_CLASS is returned indicating that the information
 * class provided is not supported by the routine.
 *
 * @remarks
 * Only certain information classes are not implemented in this function and
 * these are TokenOrigin, TokenGroupsAndPrivileges, TokenRestrictedSids and
 * TokenSandBoxInert. The following classes are implemented in NtQueryInformationToken
 * only.
 */
NTSTATUS
NTAPI
SeQueryInformationToken(
    _In_ PACCESS_TOKEN AccessToken,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Outptr_result_buffer_(_Inexpressible_(token-dependent)) PVOID *TokenInformation)
{
    NTSTATUS Status;
    PTOKEN Token = (PTOKEN)AccessToken;
    ULONG RequiredLength;
    union
    {
        PSID PSid;
        ULONG Ulong;
    } Unused;

    PAGED_CODE();

    /* Lock the token */
    SepAcquireTokenLockShared(Token);

    switch (TokenInformationClass)
    {
        case TokenUser:
        {
            PTOKEN_USER tu;

            DPRINT("SeQueryInformationToken(TokenUser)\n");
            RequiredLength = sizeof(TOKEN_USER) +
                RtlLengthSid(Token->UserAndGroups[0].Sid);

            /* Allocate the output buffer */
            tu = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tu == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            Status = RtlCopySidAndAttributesArray(1,
                                                  &Token->UserAndGroups[0],
                                                  RequiredLength - sizeof(TOKEN_USER),
                                                  &tu->User,
                                                  (PSID)(tu + 1),
                                                  &Unused.PSid,
                                                  &Unused.Ulong);

            /* Return the structure */
            *TokenInformation = tu;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenGroups:
        {
            PTOKEN_GROUPS tg;
            ULONG SidLen;
            PSID Sid;

            DPRINT("SeQueryInformationToken(TokenGroups)\n");
            RequiredLength = sizeof(tg->GroupCount) +
                RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]);

            SidLen = RequiredLength - sizeof(tg->GroupCount) -
                ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES));

            /* Allocate the output buffer */
            tg = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tg == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            Sid = (PSID)((ULONG_PTR)tg + sizeof(tg->GroupCount) +
                         ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES)));

            tg->GroupCount = Token->UserAndGroupCount - 1;
            Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
                                                  &Token->UserAndGroups[1],
                                                  SidLen,
                                                  &tg->Groups[0],
                                                  Sid,
                                                  &Unused.PSid,
                                                  &Unused.Ulong);

            /* Return the structure */
            *TokenInformation = tg;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenPrivileges:
        {
            PTOKEN_PRIVILEGES tp;

            DPRINT("SeQueryInformationToken(TokenPrivileges)\n");
            RequiredLength = sizeof(tp->PrivilegeCount) +
                (Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

            /* Allocate the output buffer */
            tp = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tp == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            tp->PrivilegeCount = Token->PrivilegeCount;
            RtlCopyLuidAndAttributesArray(Token->PrivilegeCount,
                                          Token->Privileges,
                                          &tp->Privileges[0]);

            /* Return the structure */
            *TokenInformation = tp;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenOwner:
        {
            PTOKEN_OWNER to;
            ULONG SidLen;

            DPRINT("SeQueryInformationToken(TokenOwner)\n");
            SidLen = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
            RequiredLength = sizeof(TOKEN_OWNER) + SidLen;

            /* Allocate the output buffer */
            to = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (to == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            to->Owner = (PSID)(to + 1);
            Status = RtlCopySid(SidLen,
                                to->Owner,
                                Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);

            /* Return the structure */
            *TokenInformation = to;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenPrimaryGroup:
        {
            PTOKEN_PRIMARY_GROUP tpg;
            ULONG SidLen;

            DPRINT("SeQueryInformationToken(TokenPrimaryGroup)\n");
            SidLen = RtlLengthSid(Token->PrimaryGroup);
            RequiredLength = sizeof(TOKEN_PRIMARY_GROUP) + SidLen;

            /* Allocate the output buffer */
            tpg = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tpg == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            tpg->PrimaryGroup = (PSID)(tpg + 1);
            Status = RtlCopySid(SidLen,
                                tpg->PrimaryGroup,
                                Token->PrimaryGroup);

            /* Return the structure */
            *TokenInformation = tpg;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenDefaultDacl:
        {
            PTOKEN_DEFAULT_DACL tdd;

            DPRINT("SeQueryInformationToken(TokenDefaultDacl)\n");
            RequiredLength = sizeof(TOKEN_DEFAULT_DACL);

            if (Token->DefaultDacl != NULL)
                RequiredLength += Token->DefaultDacl->AclSize;

            /* Allocate the output buffer */
            tdd = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tdd == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            if (Token->DefaultDacl != NULL)
            {
                tdd->DefaultDacl = (PACL)(tdd + 1);
                RtlCopyMemory(tdd->DefaultDacl,
                              Token->DefaultDacl,
                              Token->DefaultDacl->AclSize);
            }
            else
            {
                tdd->DefaultDacl = NULL;
            }

            /* Return the structure */
            *TokenInformation = tdd;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenSource:
        {
            PTOKEN_SOURCE ts;

            DPRINT("SeQueryInformationToken(TokenSource)\n");
            RequiredLength = sizeof(TOKEN_SOURCE);

            /* Allocate the output buffer */
            ts = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (ts == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            *ts = Token->TokenSource;

            /* Return the structure */
            *TokenInformation = ts;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenType:
        {
            PTOKEN_TYPE tt;

            DPRINT("SeQueryInformationToken(TokenType)\n");
            RequiredLength = sizeof(TOKEN_TYPE);

            /* Allocate the output buffer */
            tt = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (tt == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            *tt = Token->TokenType;

            /* Return the structure */
            *TokenInformation = tt;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenImpersonationLevel:
        {
            PSECURITY_IMPERSONATION_LEVEL sil;

            DPRINT("SeQueryInformationToken(TokenImpersonationLevel)\n");
            RequiredLength = sizeof(SECURITY_IMPERSONATION_LEVEL);

            /* Fail if the token is not an impersonation token */
            if (Token->TokenType != TokenImpersonation)
            {
                Status = STATUS_INVALID_INFO_CLASS;
                break;
            }

            /* Allocate the output buffer */
            sil = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (sil == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            *sil = Token->ImpersonationLevel;

            /* Return the structure */
            *TokenInformation = sil;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenStatistics:
        {
            PTOKEN_STATISTICS ts;

            DPRINT("SeQueryInformationToken(TokenStatistics)\n");
            RequiredLength = sizeof(TOKEN_STATISTICS);

            /* Allocate the output buffer */
            ts = ExAllocatePoolWithTag(PagedPool, RequiredLength, TAG_SE);
            if (ts == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ts->TokenId = Token->TokenId;
            ts->AuthenticationId = Token->AuthenticationId;
            ts->ExpirationTime = Token->ExpirationTime;
            ts->TokenType = Token->TokenType;
            ts->ImpersonationLevel = Token->ImpersonationLevel;
            ts->DynamicCharged = Token->DynamicCharged;
            ts->DynamicAvailable = Token->DynamicAvailable;
            ts->GroupCount = Token->UserAndGroupCount - 1;
            ts->PrivilegeCount = Token->PrivilegeCount;
            ts->ModifiedId = Token->ModifiedId;

            /* Return the structure */
            *TokenInformation = ts;
            Status = STATUS_SUCCESS;
            break;
        }

        case TokenSessionId:
        {
            DPRINT("SeQueryInformationToken(TokenSessionId)\n");
            Status = SeQuerySessionIdToken(Token, (PULONG)TokenInformation);
            break;
        }

        default:
            DPRINT1("SeQueryInformationToken(%d) invalid information class\n", TokenInformationClass);
            Status = STATUS_INVALID_INFO_CLASS;
            break;
    }

    /* Release the lock of the token */
    SepReleaseTokenLock(Token);

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeQuerySessionIdToken(IN PACCESS_TOKEN Token,
                      IN PULONG pSessionId)
{
    PAGED_CODE();

    /* Lock the token */
    SepAcquireTokenLockShared(Token);

    *pSessionId = ((PTOKEN)Token)->SessionId;

    /* Unlock the token */
    SepReleaseTokenLock(Token);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeQueryAuthenticationIdToken(IN PACCESS_TOKEN Token,
                             OUT PLUID LogonId)
{
    PAGED_CODE();

    *LogonId = ((PTOKEN)Token)->AuthenticationId;

    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
SECURITY_IMPERSONATION_LEVEL
NTAPI
SeTokenImpersonationLevel(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();

    return ((PTOKEN)Token)->ImpersonationLevel;
}


/*
 * @implemented
 */
TOKEN_TYPE NTAPI
SeTokenType(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();

    return ((PTOKEN)Token)->TokenType;
}


/*
 * @implemented
 */
BOOLEAN
NTAPI
SeTokenIsAdmin(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();

    // NOTE: Win7+ instead really checks the list of groups in the token
    // (since TOKEN_HAS_ADMIN_GROUP == TOKEN_WRITE_RESTRICTED ...)
    return (((PTOKEN)Token)->TokenFlags & TOKEN_HAS_ADMIN_GROUP) != 0;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
SeTokenIsRestricted(IN PACCESS_TOKEN Token)
{
    PAGED_CODE();

    return (((PTOKEN)Token)->TokenFlags & TOKEN_IS_RESTRICTED) != 0;
}

/*
 * @implemented
 * @note First introduced in NT 5.1 SP2 x86 (5.1.2600.2622), absent in NT 5.2,
 *       then finally re-introduced in Vista+.
 */
BOOLEAN
NTAPI
SeTokenIsWriteRestricted(IN PACCESS_TOKEN Token)
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
     * allow impersonation. If we get such levels from the call
     * then something's seriously wrong.
     */
    ASSERT(ImpersonationLevel != SecurityAnonymous ||
           ImpersonationLevel != SecurityIdentification);

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

/*
 * @implemented
 */
_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtQueryInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _Out_writes_bytes_to_opt_(TokenInformationLength, *ReturnLength)
        PVOID TokenInformation,
    _In_ ULONG TokenInformationLength,
    _Out_ PULONG ReturnLength)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PTOKEN Token;
    ULONG RequiredLength;
    union
    {
        PSID PSid;
        ULONG Ulong;
    } Unused;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /* Check buffers and class validity */
    Status = DefaultQueryInfoBufferCheck(TokenInformationClass,
                                         SeTokenInformationClass,
                                         RTL_NUMBER_OF(SeTokenInformationClass),
                                         TokenInformation,
                                         TokenInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }

    Status = ObReferenceObjectByHandle(TokenHandle,
                                       (TokenInformationClass == TokenSource) ? TOKEN_QUERY_SOURCE : TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Lock the token */
        SepAcquireTokenLockShared(Token);

        switch (TokenInformationClass)
        {
            case TokenUser:
            {
                PTOKEN_USER tu = (PTOKEN_USER)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenUser)\n");
                RequiredLength = sizeof(TOKEN_USER) +
                    RtlLengthSid(Token->UserAndGroups[0].Sid);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        Status = RtlCopySidAndAttributesArray(1,
                                                              &Token->UserAndGroups[0],
                                                              RequiredLength - sizeof(TOKEN_USER),
                                                              &tu->User,
                                                              (PSID)(tu + 1),
                                                              &Unused.PSid,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenGroups:
            {
                PTOKEN_GROUPS tg = (PTOKEN_GROUPS)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenGroups)\n");
                RequiredLength = sizeof(tg->GroupCount) +
                    RtlLengthSidAndAttributes(Token->UserAndGroupCount - 1, &Token->UserAndGroups[1]);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        ULONG SidLen = RequiredLength - sizeof(tg->GroupCount) -
                            ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES));
                        PSID Sid = (PSID_AND_ATTRIBUTES)((ULONG_PTR)tg + sizeof(tg->GroupCount) +
                                                         ((Token->UserAndGroupCount - 1) * sizeof(SID_AND_ATTRIBUTES)));

                        tg->GroupCount = Token->UserAndGroupCount - 1;
                        Status = RtlCopySidAndAttributesArray(Token->UserAndGroupCount - 1,
                                                              &Token->UserAndGroups[1],
                                                              SidLen,
                                                              &tg->Groups[0],
                                                              Sid,
                                                              &Unused.PSid,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenPrivileges:
            {
                PTOKEN_PRIVILEGES tp = (PTOKEN_PRIVILEGES)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenPrivileges)\n");
                RequiredLength = sizeof(tp->PrivilegeCount) +
                    (Token->PrivilegeCount * sizeof(LUID_AND_ATTRIBUTES));

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        tp->PrivilegeCount = Token->PrivilegeCount;
                        RtlCopyLuidAndAttributesArray(Token->PrivilegeCount,
                                                      Token->Privileges,
                                                      &tp->Privileges[0]);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenOwner:
            {
                PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
                ULONG SidLen;

                DPRINT("NtQueryInformationToken(TokenOwner)\n");
                SidLen = RtlLengthSid(Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
                RequiredLength = sizeof(TOKEN_OWNER) + SidLen;

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        to->Owner = (PSID)(to + 1);
                        Status = RtlCopySid(SidLen,
                                            to->Owner,
                                            Token->UserAndGroups[Token->DefaultOwnerIndex].Sid);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenPrimaryGroup:
            {
                PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
                ULONG SidLen;

                DPRINT("NtQueryInformationToken(TokenPrimaryGroup)\n");
                SidLen = RtlLengthSid(Token->PrimaryGroup);
                RequiredLength = sizeof(TOKEN_PRIMARY_GROUP) + SidLen;

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        tpg->PrimaryGroup = (PSID)(tpg + 1);
                        Status = RtlCopySid(SidLen,
                                            tpg->PrimaryGroup,
                                            Token->PrimaryGroup);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenDefaultDacl:
            {
                PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenDefaultDacl)\n");
                RequiredLength = sizeof(TOKEN_DEFAULT_DACL);

                if (Token->DefaultDacl != NULL)
                    RequiredLength += Token->DefaultDacl->AclSize;

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        if (Token->DefaultDacl != NULL)
                        {
                            tdd->DefaultDacl = (PACL)(tdd + 1);
                            RtlCopyMemory(tdd->DefaultDacl,
                                          Token->DefaultDacl,
                                          Token->DefaultDacl->AclSize);
                        }
                        else
                        {
                            tdd->DefaultDacl = NULL;
                        }
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenSource:
            {
                PTOKEN_SOURCE ts = (PTOKEN_SOURCE)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenSource)\n");
                RequiredLength = sizeof(TOKEN_SOURCE);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *ts = Token->TokenSource;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenType:
            {
                PTOKEN_TYPE tt = (PTOKEN_TYPE)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenType)\n");
                RequiredLength = sizeof(TOKEN_TYPE);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *tt = Token->TokenType;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenImpersonationLevel:
            {
                PSECURITY_IMPERSONATION_LEVEL sil = (PSECURITY_IMPERSONATION_LEVEL)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenImpersonationLevel)\n");

                /* Fail if the token is not an impersonation token */
                if (Token->TokenType != TokenImpersonation)
                {
                    Status = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                RequiredLength = sizeof(SECURITY_IMPERSONATION_LEVEL);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        *sil = Token->ImpersonationLevel;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenStatistics:
            {
                PTOKEN_STATISTICS ts = (PTOKEN_STATISTICS)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenStatistics)\n");
                RequiredLength = sizeof(TOKEN_STATISTICS);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        ts->TokenId = Token->TokenId;
                        ts->AuthenticationId = Token->AuthenticationId;
                        ts->ExpirationTime = Token->ExpirationTime;
                        ts->TokenType = Token->TokenType;
                        ts->ImpersonationLevel = Token->ImpersonationLevel;
                        ts->DynamicCharged = Token->DynamicCharged;
                        ts->DynamicAvailable = Token->DynamicAvailable;
                        ts->GroupCount = Token->UserAndGroupCount - 1;
                        ts->PrivilegeCount = Token->PrivilegeCount;
                        ts->ModifiedId = Token->ModifiedId;
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenOrigin:
            {
                PTOKEN_ORIGIN to = (PTOKEN_ORIGIN)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenOrigin)\n");
                RequiredLength = sizeof(TOKEN_ORIGIN);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        RtlCopyLuid(&to->OriginatingLogonSession,
                                    &Token->AuthenticationId);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenGroupsAndPrivileges:
                DPRINT1("NtQueryInformationToken(TokenGroupsAndPrivileges) not implemented\n");
                Status = STATUS_NOT_IMPLEMENTED;
                break;

            case TokenRestrictedSids:
            {
                PTOKEN_GROUPS tg = (PTOKEN_GROUPS)TokenInformation;

                DPRINT("NtQueryInformationToken(TokenRestrictedSids)\n");
                RequiredLength = sizeof(tg->GroupCount) +
                RtlLengthSidAndAttributes(Token->RestrictedSidCount, Token->RestrictedSids);

                _SEH2_TRY
                {
                    if (TokenInformationLength >= RequiredLength)
                    {
                        ULONG SidLen = RequiredLength - sizeof(tg->GroupCount) -
                            (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES));
                        PSID Sid = (PSID)((ULONG_PTR)tg + sizeof(tg->GroupCount) +
                                          (Token->RestrictedSidCount * sizeof(SID_AND_ATTRIBUTES)));

                        tg->GroupCount = Token->RestrictedSidCount;
                        Status = RtlCopySidAndAttributesArray(Token->RestrictedSidCount,
                                                              Token->RestrictedSids,
                                                              SidLen,
                                                              &tg->Groups[0],
                                                              Sid,
                                                              &Unused.PSid,
                                                              &Unused.Ulong);
                    }
                    else
                    {
                        Status = STATUS_BUFFER_TOO_SMALL;
                    }

                    if (ReturnLength != NULL)
                    {
                        *ReturnLength = RequiredLength;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                break;
            }

            case TokenSandBoxInert:
                DPRINT1("NtQueryInformationToken(TokenSandboxInert) not implemented\n");
                Status = STATUS_NOT_IMPLEMENTED;
                break;

            case TokenSessionId:
            {
                ULONG SessionId = 0;

                DPRINT("NtQueryInformationToken(TokenSessionId)\n");

                Status = SeQuerySessionIdToken(Token, &SessionId);
                if (NT_SUCCESS(Status))
                {
                    _SEH2_TRY
                    {
                        /* Buffer size was already verified, no need to check here again */
                        *(PULONG)TokenInformation = SessionId;

                        if (ReturnLength != NULL)
                        {
                            *ReturnLength = sizeof(ULONG);
                        }
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                }

                break;
            }

            default:
                DPRINT1("NtQueryInformationToken(%d) invalid information class\n", TokenInformationClass);
                Status = STATUS_INVALID_INFO_CLASS;
                break;
        }

        /* Unlock and dereference the token */
        SepReleaseTokenLock(Token);
        ObDereferenceObject(Token);
    }

    return Status;
}


/*
 * NtSetTokenInformation: Partly implemented.
 * Unimplemented:
 *  TokenOrigin, TokenDefaultDacl
 */
_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtSetInformationToken(
    _In_ HANDLE TokenHandle,
    _In_ TOKEN_INFORMATION_CLASS TokenInformationClass,
    _In_reads_bytes_(TokenInformationLength) PVOID TokenInformation,
    _In_ ULONG TokenInformationLength)
{
    NTSTATUS Status;
    PTOKEN Token;
    KPROCESSOR_MODE PreviousMode;
    ULONG NeededAccess = TOKEN_ADJUST_DEFAULT;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    Status = DefaultSetInfoBufferCheck(TokenInformationClass,
                                       SeTokenInformationClass,
                                       RTL_NUMBER_OF(SeTokenInformationClass),
                                       TokenInformation,
                                       TokenInformationLength,
                                       PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        /* Invalid buffers */
        DPRINT("NtSetInformationToken() failed, Status: 0x%x\n", Status);
        return Status;
    }

    if (TokenInformationClass == TokenSessionId)
    {
        NeededAccess |= TOKEN_ADJUST_SESSIONID;
    }

    Status = ObReferenceObjectByHandle(TokenHandle,
                                       NeededAccess,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        switch (TokenInformationClass)
        {
            case TokenOwner:
            {
                if (TokenInformationLength >= sizeof(TOKEN_OWNER))
                {
                    PTOKEN_OWNER to = (PTOKEN_OWNER)TokenInformation;
                    PSID InputSid = NULL, CapturedSid;
                    ULONG DefaultOwnerIndex;

                    _SEH2_TRY
                    {
                        InputSid = to->Owner;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        _SEH2_YIELD(goto Cleanup);
                    }
                    _SEH2_END;

                    Status = SepCaptureSid(InputSid,
                                           PreviousMode,
                                           PagedPool,
                                           FALSE,
                                           &CapturedSid);
                    if (NT_SUCCESS(Status))
                    {
                        /* Lock the token */
                        SepAcquireTokenLockExclusive(Token);

                        /* Find the owner amongst the existing token user and groups */
                        Status = SepFindPrimaryGroupAndDefaultOwner(Token,
                                                                    NULL,
                                                                    CapturedSid,
                                                                    NULL,
                                                                    &DefaultOwnerIndex);
                        if (NT_SUCCESS(Status))
                        {
                            /* Found it */
                            Token->DefaultOwnerIndex = DefaultOwnerIndex;
                            ExAllocateLocallyUniqueId(&Token->ModifiedId);
                        }

                        /* Unlock the token */
                        SepReleaseTokenLock(Token);

                        SepReleaseSid(CapturedSid,
                                      PreviousMode,
                                      FALSE);
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }

            case TokenPrimaryGroup:
            {
                if (TokenInformationLength >= sizeof(TOKEN_PRIMARY_GROUP))
                {
                    PTOKEN_PRIMARY_GROUP tpg = (PTOKEN_PRIMARY_GROUP)TokenInformation;
                    PSID InputSid = NULL, CapturedSid;
                    ULONG PrimaryGroupIndex;

                    _SEH2_TRY
                    {
                        InputSid = tpg->PrimaryGroup;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        _SEH2_YIELD(goto Cleanup);
                    }
                    _SEH2_END;

                    Status = SepCaptureSid(InputSid,
                                           PreviousMode,
                                           PagedPool,
                                           FALSE,
                                           &CapturedSid);
                    if (NT_SUCCESS(Status))
                    {
                        /* Lock the token */
                        SepAcquireTokenLockExclusive(Token);

                        /* Find the primary group amongst the existing token user and groups */
                        Status = SepFindPrimaryGroupAndDefaultOwner(Token,
                                                                    CapturedSid,
                                                                    NULL,
                                                                    &PrimaryGroupIndex,
                                                                    NULL);
                        if (NT_SUCCESS(Status))
                        {
                            /* Found it */
                            Token->PrimaryGroup = Token->UserAndGroups[PrimaryGroupIndex].Sid;
                            ExAllocateLocallyUniqueId(&Token->ModifiedId);
                        }

                        /* Unlock the token */
                        SepReleaseTokenLock(Token);

                        SepReleaseSid(CapturedSid,
                                      PreviousMode,
                                      FALSE);
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }

            case TokenDefaultDacl:
            {
                if (TokenInformationLength >= sizeof(TOKEN_DEFAULT_DACL))
                {
                    PTOKEN_DEFAULT_DACL tdd = (PTOKEN_DEFAULT_DACL)TokenInformation;
                    PACL InputAcl = NULL;

                    _SEH2_TRY
                    {
                        InputAcl = tdd->DefaultDacl;
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        Status = _SEH2_GetExceptionCode();
                        _SEH2_YIELD(goto Cleanup);
                    }
                    _SEH2_END;

                    if (InputAcl != NULL)
                    {
                        PACL CapturedAcl;

                        /* Capture and copy the dacl */
                        Status = SepCaptureAcl(InputAcl,
                                               PreviousMode,
                                               PagedPool,
                                               TRUE,
                                               &CapturedAcl);
                        if (NT_SUCCESS(Status))
                        {
                            ULONG DynamicLength;

                            /* Lock the token */
                            SepAcquireTokenLockExclusive(Token);

                            //
                            // NOTE: So far our dynamic area only contains
                            // the default dacl, so this makes the following
                            // code pretty simple. The day where it stores
                            // other data, the code will require adaptations.
                            //

                            DynamicLength = Token->DynamicAvailable;
                            // Add here any other data length present in the dynamic area...
                            if (Token->DefaultDacl)
                                DynamicLength += Token->DefaultDacl->AclSize;

                            /* Reallocate the dynamic area if it is too small */
                            Status = STATUS_SUCCESS;
                            if ((DynamicLength < CapturedAcl->AclSize) ||
                                (Token->DynamicPart == NULL))
                            {
                                PVOID NewDynamicPart;

                                NewDynamicPart = ExAllocatePoolWithTag(PagedPool,
                                                                       CapturedAcl->AclSize,
                                                                       TAG_TOKEN_DYNAMIC);
                                if (NewDynamicPart == NULL)
                                {
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                }
                                else
                                {
                                    if (Token->DynamicPart != NULL)
                                    {
                                        // RtlCopyMemory(NewDynamicPart, Token->DynamicPart, DynamicLength);
                                        ExFreePoolWithTag(Token->DynamicPart, TAG_TOKEN_DYNAMIC);
                                    }
                                    Token->DynamicPart = NewDynamicPart;
                                    Token->DynamicAvailable = 0;
                                }
                            }
                            else
                            {
                                Token->DynamicAvailable = DynamicLength - CapturedAcl->AclSize;
                            }

                            if (NT_SUCCESS(Status))
                            {
                                /* Set the new dacl */
                                Token->DefaultDacl = (PVOID)Token->DynamicPart;
                                RtlCopyMemory(Token->DefaultDacl,
                                              CapturedAcl,
                                              CapturedAcl->AclSize);

                                ExAllocateLocallyUniqueId(&Token->ModifiedId);
                            }

                            /* Unlock the token */
                            SepReleaseTokenLock(Token);

                            ExFreePoolWithTag(CapturedAcl, TAG_ACL);
                        }
                    }
                    else
                    {
                        /* Lock the token */
                        SepAcquireTokenLockExclusive(Token);

                        /* Clear the default dacl if present */
                        if (Token->DefaultDacl != NULL)
                        {
                            Token->DynamicAvailable += Token->DefaultDacl->AclSize;
                            RtlZeroMemory(Token->DefaultDacl, Token->DefaultDacl->AclSize);
                            Token->DefaultDacl = NULL;

                            ExAllocateLocallyUniqueId(&Token->ModifiedId);
                        }

                        /* Unlock the token */
                        SepReleaseTokenLock(Token);
                    }
                }
                else
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                break;
            }

            case TokenSessionId:
            {
                ULONG SessionId = 0;

                _SEH2_TRY
                {
                    /* Buffer size was already verified, no need to check here again */
                    SessionId = *(PULONG)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                Token->SessionId = SessionId;
                ExAllocateLocallyUniqueId(&Token->ModifiedId);

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            case TokenSessionReference:
            {
                ULONG SessionReference;

                _SEH2_TRY
                {
                    /* Buffer size was already verified, no need to check here again */
                    SessionReference = *(PULONG)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    goto Cleanup;
                }

                /* Check if it is 0 */
                if (SessionReference == 0)
                {
                    ULONG OldTokenFlags;

                    /* Lock the token */
                    SepAcquireTokenLockExclusive(Token);

                    /* Atomically set the flag in the token */
                    OldTokenFlags = RtlInterlockedSetBits(&Token->TokenFlags,
                                                          TOKEN_SESSION_NOT_REFERENCED);
                    /*
                     * If the flag was already set, do not dereference again
                     * the logon session. Use SessionReference as an indicator
                     * to know whether to really dereference the session.
                     */
                    if (OldTokenFlags == Token->TokenFlags)
                        SessionReference = ULONG_MAX;

                    /* Unlock the token */
                    SepReleaseTokenLock(Token);
                }

                /* Dereference the logon session if needed */
                if (SessionReference == 0)
                    SepRmDereferenceLogonSession(&Token->AuthenticationId);

                break;
            }

            case TokenAuditPolicy:
            {
                PTOKEN_AUDIT_POLICY_INFORMATION PolicyInformation =
                    (PTOKEN_AUDIT_POLICY_INFORMATION)TokenInformation;
                SEP_AUDIT_POLICY AuditPolicy;
                ULONG i;

                _SEH2_TRY
                {
                    ProbeForRead(PolicyInformation,
                                 FIELD_OFFSET(TOKEN_AUDIT_POLICY_INFORMATION,
                                              Policies[PolicyInformation->PolicyCount]),
                                 sizeof(ULONG));

                    /* Loop all policies in the structure */
                    for (i = 0; i < PolicyInformation->PolicyCount; i++)
                    {
                        /* Set the corresponding bits in the packed structure */
                        switch (PolicyInformation->Policies[i].Category)
                        {
                            case AuditCategorySystem:
                                AuditPolicy.PolicyElements.System = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryLogon:
                                AuditPolicy.PolicyElements.Logon = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryObjectAccess:
                                AuditPolicy.PolicyElements.ObjectAccess = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryPrivilegeUse:
                                AuditPolicy.PolicyElements.PrivilegeUse = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryDetailedTracking:
                                AuditPolicy.PolicyElements.DetailedTracking = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryPolicyChange:
                                AuditPolicy.PolicyElements.PolicyChange = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryAccountManagement:
                                AuditPolicy.PolicyElements.AccountManagement = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryDirectoryServiceAccess:
                                AuditPolicy.PolicyElements.DirectoryServiceAccess = PolicyInformation->Policies[i].Value;
                                break;

                            case AuditCategoryAccountLogon:
                                AuditPolicy.PolicyElements.AccountLogon = PolicyInformation->Policies[i].Value;
                                break;
                        }
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                /* Set the new audit policy */
                Token->AuditPolicy = AuditPolicy;
                ExAllocateLocallyUniqueId(&Token->ModifiedId);

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            case TokenOrigin:
            {
                TOKEN_ORIGIN TokenOrigin;

                _SEH2_TRY
                {
                    /* Copy the token origin */
                    TokenOrigin = *(PTOKEN_ORIGIN)TokenInformation;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(goto Cleanup);
                }
                _SEH2_END;

                /* Check for TCB privilege */
                if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
                {
                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* Lock the token */
                SepAcquireTokenLockExclusive(Token);

                /* Check if there is no token origin set yet */
                if (RtlIsZeroLuid(&Token->OriginatingLogonSession))
                {
                    /* Set the token origin */
                    Token->OriginatingLogonSession =
                        TokenOrigin.OriginatingLogonSession;

                    ExAllocateLocallyUniqueId(&Token->ModifiedId);
                }

                /* Unlock the token */
                SepReleaseTokenLock(Token);

                break;
            }

            default:
            {
                DPRINT1("Invalid TokenInformationClass: 0x%lx\n",
                        TokenInformationClass);
                Status = STATUS_INVALID_INFO_CLASS;
                break;
            }
        }
Cleanup:
        ObDereferenceObject(Token);
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationToken failed with Status 0x%lx\n", Status);
    }

    return Status;
}


/*
 * @implemented
 *
 * NOTE: Some sources claim 4th param is ImpersonationLevel, but on W2K
 * this is certainly NOT true, although I can't say for sure that EffectiveOnly
 * is correct either. -Gunnar
 * This is true. EffectiveOnly overrides SQOS.EffectiveOnly. - IAI
 * NOTE for readers: http://hex.pp.ua/nt/NtDuplicateToken.php is therefore
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

NTSTATUS NTAPI
NtAdjustGroupsToken(IN HANDLE TokenHandle,
                    IN BOOLEAN ResetToDefault,
                    IN PTOKEN_GROUPS NewState,
                    IN ULONG BufferLength,
                    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
                    OUT PULONG ReturnLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}


static
NTSTATUS
SepAdjustPrivileges(
    _Inout_ PTOKEN Token,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PLUID_AND_ATTRIBUTES NewState,
    _In_ ULONG NewStateCount,
    _Out_opt_ PTOKEN_PRIVILEGES PreviousState,
    _In_ BOOLEAN ApplyChanges,
    _Out_ PULONG ChangedPrivileges,
    _Out_ PBOOLEAN ChangesMade)
{
    ULONG i, j, PrivilegeCount, ChangeCount, NewAttributes;

    /* Count the found privileges and those that need to be changed */
    PrivilegeCount = 0;
    ChangeCount = 0;
    *ChangesMade = FALSE;

    /* Loop all privileges in the token */
    for (i = 0; i < Token->PrivilegeCount; i++)
    {
        /* Shall all of them be disabled? */
        if (DisableAllPrivileges)
        {
            /* The new attributes are the old ones, but disabled */
            NewAttributes = Token->Privileges[i].Attributes & ~SE_PRIVILEGE_ENABLED;
        }
        else
        {
            /* Otherwise loop all provided privileges */
            for (j = 0; j < NewStateCount; j++)
            {
                /* Check if this is the LUID we are looking for */
                if (RtlEqualLuid(&Token->Privileges[i].Luid, &NewState[j].Luid))
                {
                    DPRINT("Found privilege\n");

                    /* Copy SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_REMOVED */
                    NewAttributes = NewState[j].Attributes;
                    NewAttributes &= (SE_PRIVILEGE_ENABLED | SE_PRIVILEGE_REMOVED);
                    NewAttributes |= Token->Privileges[i].Attributes & ~SE_PRIVILEGE_ENABLED;

                    /* Stop looking */
                    break;
                }
            }

            /* Check if we didn't find the privilege */
            if (j == NewStateCount)
            {
                /* Continue with the token's next privilege */
                continue;
            }
        }

        /* We found a privilege, count it */
        PrivilegeCount++;

        /* Does the privilege need to be changed? */
        if (Token->Privileges[i].Attributes != NewAttributes)
        {
            /* Does the caller want the old privileges? */
            if (PreviousState != NULL)
            {
                /* Copy the old privilege */
                PreviousState->Privileges[ChangeCount] = Token->Privileges[i];
            }

            /* Does the caller want to apply the changes? */
            if (ApplyChanges)
            {
                /* Shall we remove the privilege? */
                if (NewAttributes & SE_PRIVILEGE_REMOVED)
                {
                    /* Set the token as disabled and update flags for it */
                    Token->Privileges[i].Attributes &= ~SE_PRIVILEGE_ENABLED;
                    SepUpdateSinglePrivilegeFlagToken(Token, i);

                    /* Remove the privilege */
                    SepRemovePrivilegeToken(Token, i);

                    *ChangesMade = TRUE;

                    /* Fix the running index and continue with next one */
                    i--;
                    continue;
                }

                /* Set the new attributes and update flags */
                Token->Privileges[i].Attributes = NewAttributes;
                SepUpdateSinglePrivilegeFlagToken(Token, i);
                *ChangesMade = TRUE;
            }

            /* Increment the change count */
            ChangeCount++;
        }
    }

    /* Set the number of saved privileges */
    if (PreviousState != NULL)
        PreviousState->PrivilegeCount = ChangeCount;

    /* Return the number of changed privileges */
    *ChangedPrivileges = ChangeCount;

    /* Check if we missed some */
    if (!DisableAllPrivileges && (PrivilegeCount < NewStateCount))
    {
        return STATUS_NOT_ALL_ASSIGNED;
    }

    return STATUS_SUCCESS;
}


/*
 * @implemented
 */
_Must_inspect_result_
__kernel_entry
NTSTATUS
NTAPI
NtAdjustPrivilegesToken(
    _In_ HANDLE TokenHandle,
    _In_ BOOLEAN DisableAllPrivileges,
    _In_opt_ PTOKEN_PRIVILEGES NewState,
    _In_ ULONG BufferLength,
    _Out_writes_bytes_to_opt_(BufferLength,*ReturnLength)
        PTOKEN_PRIVILEGES PreviousState,
    _When_(PreviousState!=NULL, _Out_) PULONG ReturnLength)
{
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;
    PTOKEN Token;
    PLUID_AND_ATTRIBUTES CapturedPrivileges = NULL;
    ULONG CapturedCount = 0;
    ULONG CapturedLength = 0;
    ULONG NewStateSize = 0;
    ULONG ChangeCount;
    ULONG RequiredLength;
    BOOLEAN ChangesMade = FALSE;

    PAGED_CODE();

    DPRINT("NtAdjustPrivilegesToken() called\n");

    /* Fail, if we do not disable all privileges but NewState is NULL */
    if (DisableAllPrivileges == FALSE && NewState == NULL)
        return STATUS_INVALID_PARAMETER;

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe NewState */
            if (DisableAllPrivileges == FALSE)
            {
                /* First probe the header */
                ProbeForRead(NewState, sizeof(TOKEN_PRIVILEGES), sizeof(ULONG));

                CapturedCount = NewState->PrivilegeCount;
                NewStateSize = FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges[CapturedCount]);

                ProbeForRead(NewState, NewStateSize, sizeof(ULONG));
            }

            /* Probe PreviousState and ReturnLength */
            if (PreviousState != NULL)
            {
                ProbeForWrite(PreviousState, BufferLength, sizeof(ULONG));
                ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            }
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
        /* This is kernel mode, we trust the caller */
        if (DisableAllPrivileges == FALSE)
            CapturedCount = NewState->PrivilegeCount;
    }

    /* Do we need to capture the new state? */
    if (DisableAllPrivileges == FALSE)
    {
        _SEH2_TRY
        {
            /* Capture the new state array of privileges */
            Status = SeCaptureLuidAndAttributesArray(NewState->Privileges,
                                                     CapturedCount,
                                                     PreviousMode,
                                                     NULL,
                                                     0,
                                                     PagedPool,
                                                     TRUE,
                                                     &CapturedPrivileges,
                                                     &CapturedLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        if (!NT_SUCCESS(Status))
            return Status;
    }

    /* Reference the token */
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       TOKEN_ADJUST_PRIVILEGES | (PreviousState != NULL ? TOKEN_QUERY : 0),
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference token (Status 0x%lx)\n", Status);

        /* Release the captured privileges */
        if (CapturedPrivileges != NULL)
        {
            SeReleaseLuidAndAttributesArray(CapturedPrivileges,
                                            PreviousMode,
                                            TRUE);
        }

        return Status;
    }

    /* Lock the token */
    SepAcquireTokenLockExclusive(Token);

    /* Count the privileges that need to be changed, do not apply them yet */
    Status = SepAdjustPrivileges(Token,
                                 DisableAllPrivileges,
                                 CapturedPrivileges,
                                 CapturedCount,
                                 NULL,
                                 FALSE,
                                 &ChangeCount,
                                 &ChangesMade);

    /* Check if the caller asked for the previous state */
    if (PreviousState != NULL)
    {
        /* Calculate the required length */
        RequiredLength = FIELD_OFFSET(TOKEN_PRIVILEGES, Privileges[ChangeCount]);

        /* Try to return the required buffer length */
        _SEH2_TRY
        {
            *ReturnLength = RequiredLength;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Do cleanup and return the exception code */
            Status = _SEH2_GetExceptionCode();
            _SEH2_YIELD(goto Cleanup);
        }
        _SEH2_END;

        /* Fail, if the buffer length is smaller than the required length */
        if (BufferLength < RequiredLength)
        {
            Status = STATUS_BUFFER_TOO_SMALL;
            goto Cleanup;
        }
    }

    /* Now enter SEH, since we might return the old privileges */
    _SEH2_TRY
    {
        /* This time apply the changes */
        Status = SepAdjustPrivileges(Token,
                                     DisableAllPrivileges,
                                     CapturedPrivileges,
                                     CapturedCount,
                                     PreviousState,
                                     TRUE,
                                     &ChangeCount,
                                     &ChangesMade);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Do cleanup and return the exception code */
        Status = _SEH2_GetExceptionCode();
        ChangesMade = TRUE; // Force write.
        _SEH2_YIELD(goto Cleanup);
    }
    _SEH2_END;

Cleanup:
    /* Touch the token if we made changes */
    if (ChangesMade)
        ExAllocateLocallyUniqueId(&Token->ModifiedId);

    /* Unlock and dereference the token */
    SepReleaseTokenLock(Token);
    ObDereferenceObject(Token);

    /* Release the captured privileges */
    if (CapturedPrivileges != NULL)
    {
        SeReleaseLuidAndAttributesArray(CapturedPrivileges,
                                        PreviousMode,
                                        TRUE);
    }

    DPRINT ("NtAdjustPrivilegesToken() done\n");
    return Status;
}

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
                            0, // FIXME: Should capture
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenThreadTokenEx(IN HANDLE ThreadHandle,
                    IN ACCESS_MASK DesiredAccess,
                    IN BOOLEAN OpenAsSelf,
                    IN ULONG HandleAttributes,
                    OUT PHANDLE TokenHandle)
{
    PETHREAD Thread, NewThread;
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
        Status = ObReferenceObjectByHandle(ThreadHandle, THREAD_ALL_ACCESS,
                                           PsThreadType, KernelMode,
                                           (PVOID*)&NewThread, NULL);
        if (NT_SUCCESS(Status))
        {
            PrimaryToken = PsReferencePrimaryToken(NewThread->ThreadsProcess);

            Status = SepCreateImpersonationTokenDacl(Token, PrimaryToken, &Dacl);

            ObFastDereferenceObject(&NewThread->ThreadsProcess->Token, PrimaryToken);

            if (NT_SUCCESS(Status))
            {
                if (Dacl)
                {
                    RtlCreateSecurityDescriptor(&SecurityDescriptor,
                                                SECURITY_DESCRIPTOR_REVISION);
                    RtlSetDaclSecurityDescriptor(&SecurityDescriptor, TRUE, Dacl,
                                                 FALSE);
                }

                InitializeObjectAttributes(&ObjectAttributes, NULL, HandleAttributes,
                                           NULL, Dacl ? &SecurityDescriptor : NULL);

                Status = SepDuplicateToken(Token, &ObjectAttributes, EffectiveOnly,
                                           TokenImpersonation, ImpersonationLevel,
                                           KernelMode, &NewToken);
                if (NT_SUCCESS(Status))
                {
                    ObReferenceObject(NewToken);
                    Status = ObInsertObject(NewToken, NULL, DesiredAccess, 0, NULL,
                                            &hToken);
                }
            }
        }
    }
    else
    {
        Status = ObOpenObjectByPointer(Token, HandleAttributes,
                                       NULL, DesiredAccess, SeTokenObjectType,
                                       PreviousMode, &hToken);
    }

    if (Dacl) ExFreePoolWithTag(Dacl, TAG_ACL);

    if (RestoreImpersonation)
    {
        PsRestoreImpersonation(PsGetCurrentThread(), &ImpersonationState);
    }

    ObDereferenceObject(Token);

    if (NT_SUCCESS(Status) && CopyOnOpen)
    {
        PsImpersonateClient(Thread, NewToken, FALSE, EffectiveOnly, ImpersonationLevel);
    }

    if (NewToken) ObDereferenceObject(NewToken);

    if (CopyOnOpen && NewThread) ObDereferenceObject(NewThread);

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

/*
 * @implemented
 */
NTSTATUS NTAPI
NtOpenThreadToken(IN HANDLE ThreadHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN BOOLEAN OpenAsSelf,
                  OUT PHANDLE TokenHandle)
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

NTSTATUS
NTAPI
NtFilterToken(IN HANDLE ExistingTokenHandle,
              IN ULONG Flags,
              IN PTOKEN_GROUPS SidsToDisable OPTIONAL,
              IN PTOKEN_PRIVILEGES PrivilegesToDelete OPTIONAL,
              IN PTOKEN_GROUPS RestrictedSids OPTIONAL,
              OUT PHANDLE NewTokenHandle)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
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
