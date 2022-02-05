/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security access state functions support
 * COPYRIGHT:       Copyright Alex Ionescu <alex@relsoft.net>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ERESOURCE SepSubjectContextLock;

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Checks if a SID is present in a token.
 *
 * @param[in] _Token
 * A valid token object.
 *
 * @param[in] PrincipalSelfSid
 * A principal self SID.
 *
 * @param[in] _Sid
 * A regular SID.
 *
 * @param[in] Deny
 * If set to TRUE, the caller expected that a SID in a token
 * must be a deny-only SID, that is, access checks are performed
 * only for deny-only ACEs of the said SID.
 *
 * @param[in] Restricted
 * If set to TRUE, the caller expects that a SID in a token is
 * restricted (by the general definition, a token is restricted).
 *
 * @return
 * Returns TRUE if the specified SID in the call is present in the token,
 * FALSE otherwise.
 */
BOOLEAN
NTAPI
SepSidInTokenEx(
    _In_ PACCESS_TOKEN _Token,
    _In_ PSID PrincipalSelfSid,
    _In_ PSID _Sid,
    _In_ BOOLEAN Deny,
    _In_ BOOLEAN Restricted)
{
    ULONG SidIndex;
    PTOKEN Token = (PTOKEN)_Token;
    PISID TokenSid, Sid = (PISID)_Sid;
    PSID_AND_ATTRIBUTES SidAndAttributes;
    ULONG SidCount, SidLength;
    USHORT SidMetadata;
    PAGED_CODE();

    /* Check if a principal SID was given, and this is our current SID already */
    if ((PrincipalSelfSid) && (RtlEqualSid(SePrincipalSelfSid, Sid)))
    {
        /* Just use the principal SID in this case */
        Sid = PrincipalSelfSid;
    }

    /* Check if this is a restricted token or not */
    if (Restricted)
    {
        /* Use the restricted SIDs and count */
        SidAndAttributes = Token->RestrictedSids;
        SidCount = Token->RestrictedSidCount;
    }
    else
    {
        /* Use the normal SIDs and count */
        SidAndAttributes = Token->UserAndGroups;
        SidCount = Token->UserAndGroupCount;
    }

    /* Do checks here by hand instead of the usual 4 function calls */
    SidLength = FIELD_OFFSET(SID,
                             SubAuthority[Sid->SubAuthorityCount]);
    SidMetadata = *(PUSHORT)&Sid->Revision;

    /* Loop every SID */
    for (SidIndex = 0; SidIndex < SidCount; SidIndex++)
    {
        TokenSid = (PISID)SidAndAttributes->Sid;
#if SE_SID_DEBUG
        UNICODE_STRING sidString;
        RtlConvertSidToUnicodeString(&sidString, TokenSid, TRUE);
        DPRINT1("SID in Token: %wZ\n", &sidString);
        RtlFreeUnicodeString(&sidString);
#endif
        /* Check if the SID metadata matches */
        if (*(PUSHORT)&TokenSid->Revision == SidMetadata)
        {
            /* Check if the SID data matches */
            if (RtlEqualMemory(Sid, TokenSid, SidLength))
            {
                /*
                 * Check if the group is enabled, or used for deny only.
                 * Otherwise we have to check if this is the first user.
                 * We understand that by looking if this SID is not
                 * restricted, this is the first element we are iterating
                 * and that it doesn't have SE_GROUP_USE_FOR_DENY_ONLY
                 * attribute.
                 */
                if ((!Restricted && (SidIndex == 0) && !(SidAndAttributes->Attributes & SE_GROUP_USE_FOR_DENY_ONLY)) ||
                    (SidAndAttributes->Attributes & SE_GROUP_ENABLED) ||
                    ((Deny) && (SidAndAttributes->Attributes & SE_GROUP_USE_FOR_DENY_ONLY)))
                {
                    /* SID is present */
                    return TRUE;
                }
                else
                {
                    /* SID is not present */
                    return FALSE;
                }
            }
        }

        /* Move to the next SID */
        SidAndAttributes++;
    }

    /* SID is not present */
    return FALSE;
}

/**
 * @brief
 * Checks if a SID is present in a token.
 *
 * @param[in] _Token
 * A valid token object.
 *
 * @param[in] _Sid
 * A regular SID.
 *
 * @return
 * Returns TRUE if the specified SID in the call is present in the token,
 * FALSE otherwise.
 */
BOOLEAN
NTAPI
SepSidInToken(
    _In_ PACCESS_TOKEN _Token,
    _In_ PSID Sid)
{
    /* Call extended API */
    return SepSidInTokenEx(_Token, NULL, Sid, FALSE, FALSE);
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
 * Creates a client security context based upon an access token.
 *
 * @param[in] Token
 * A valid token object.
 *
 * @param[in] ClientSecurityQos
 * The Quality of Service (QoS) of a client security context.
 *
 * @param[in] ServerIsRemote
 * If the client is a remote server (TRUE), the function will retrieve the
 * control information of an access token, that is, we're doing delegation
 * and that the server isn't local.
 *
 * @param[in] TokenType
 * Type of token.
 *
 * @param[in] ThreadEffectiveOnly
 * If set to TRUE, the client wants that the current thread wants to modify
 * (enable or disable) privileges and groups.
 *
 * @param[in] ImpersonationLevel
 * Security impersonation level filled in the QoS context.
 *
 * @param[out] ClientContext
 * The returned security client context.
 *
 * @return
 * Returns STATUS_SUCCESS if client security creation has completed successfully.
 * STATUS_INVALID_PARAMETER is returned if one or more of the parameters are bogus.
 * STATUS_BAD_IMPERSONATION_LEVEL is returned if the current impersonation level
 * within QoS context doesn't meet with the conditions required. A failure
 * NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SepCreateClientSecurity(
    _In_ PACCESS_TOKEN Token,
    _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    _In_ BOOLEAN ServerIsRemote,
    _In_ TOKEN_TYPE TokenType,
    _In_ BOOLEAN ThreadEffectiveOnly,
    _In_ SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    _Out_ PSECURITY_CLIENT_CONTEXT ClientContext)
{
    NTSTATUS Status;
    PACCESS_TOKEN NewToken;
    PAGED_CODE();

    /* Check for bogus impersonation level */
    if (!VALID_IMPERSONATION_LEVEL(ClientSecurityQos->ImpersonationLevel))
    {
        /* Fail the call */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check what kind of token this is */
    if (TokenType != TokenImpersonation)
    {
        /* On a primary token, if we do direct access, copy the flag from the QOS */
        ClientContext->DirectAccessEffectiveOnly = ClientSecurityQos->EffectiveOnly;
    }
    else
    {
        /* This is an impersonation token, is the level ok? */
        if (ClientSecurityQos->ImpersonationLevel > ImpersonationLevel)
        {
            /* Nope, fail */
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }

        /* Is the level too low, or are we doing something other than delegation remotely */
        if ((ImpersonationLevel == SecurityAnonymous) ||
            (ImpersonationLevel == SecurityIdentification) ||
            ((ServerIsRemote) && (ImpersonationLevel != SecurityDelegation)))
        {
            /* Fail the call */
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }

        /* Pick either the thread setting or the QOS setting */
        ClientContext->DirectAccessEffectiveOnly =
            ((ThreadEffectiveOnly) || (ClientSecurityQos->EffectiveOnly)) ? TRUE : FALSE;
    }

    /* Is this static tracking */
    if (ClientSecurityQos->ContextTrackingMode == SECURITY_STATIC_TRACKING)
    {
        /* Do not use direct access and make a copy */
        ClientContext->DirectlyAccessClientToken = FALSE;
        Status = SeCopyClientToken(Token,
                                   ClientSecurityQos->ImpersonationLevel,
                                   KernelMode,
                                   &NewToken);
        if (!NT_SUCCESS(Status))
            return Status;
    }
    else
    {
        /* Use direct access and check if this is local */
        ClientContext->DirectlyAccessClientToken = TRUE;
        if (ServerIsRemote)
        {
            /* We are doing delegation, so make a copy of the control data */
            SeGetTokenControlInformation(Token,
                                         &ClientContext->ClientTokenControl);
        }

        /* Keep the same token */
        NewToken = Token;
    }

    /* Fill out the context and return success */
    ClientContext->SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ClientContext->SecurityQos.ImpersonationLevel = ClientSecurityQos->ImpersonationLevel;
    ClientContext->SecurityQos.ContextTrackingMode = ClientSecurityQos->ContextTrackingMode;
    ClientContext->SecurityQos.EffectiveOnly = ClientSecurityQos->EffectiveOnly;
    ClientContext->ServerIsRemote = ServerIsRemote;
    ClientContext->ClientToken = NewToken;
    return STATUS_SUCCESS;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * An extended function that captures the security subject context based upon
 * the specified thread and process.
 *
 * @param[in] Thread
 * A thread where the calling thread's token is to be referenced for
 * the security context.
 *
 * @param[in] Process
 * A process where the main process' token is to be referenced for
 * the security context.
 *
 * @param[out] SubjectContext
 * The returned security subject context.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeCaptureSubjectContextEx(
    _In_ PETHREAD Thread,
    _In_ PEPROCESS Process,
    _Out_ PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    BOOLEAN CopyOnOpen, EffectiveOnly;

    PAGED_CODE();

    /* Save the unique ID */
    SubjectContext->ProcessAuditId = Process->UniqueProcessId;

    /* Check if we have a thread */
    if (!Thread)
    {
        /* We don't, so no token */
        SubjectContext->ClientToken = NULL;
    }
    else
    {
        /* Get the impersonation token */
        SubjectContext->ClientToken = PsReferenceImpersonationToken(Thread,
                                                                    &CopyOnOpen,
                                                                    &EffectiveOnly,
                                                                    &SubjectContext->ImpersonationLevel);
    }

    /* Get the primary token */
    SubjectContext->PrimaryToken = PsReferencePrimaryToken(Process);
}

/**
 * @brief
 * Captures the security subject context of the calling thread and calling
 * process.
 *
 * @param[out] SubjectContext
 * The returned security subject context.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeCaptureSubjectContext(
    _Out_ PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    /* Call the extended API */
    SeCaptureSubjectContextEx(PsGetCurrentThread(),
                              PsGetCurrentProcess(),
                              SubjectContext);
}

/**
 * @brief
 * Locks both the referenced primary and client access tokens of a
 * security subject context.
 *
 * @param[in] SubjectContext
 * A valid security context with both referenced tokens.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeLockSubjectContext(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    PTOKEN PrimaryToken, ClientToken;
    PAGED_CODE();

    /* Read both tokens */
    PrimaryToken = SubjectContext->PrimaryToken;
    ClientToken = SubjectContext->ClientToken;

    /* Always lock the primary */
    SepAcquireTokenLockShared(PrimaryToken);

    /* Lock the impersonation one if it's there */
    if (!ClientToken) return;
    SepAcquireTokenLockShared(ClientToken);
}

/**
 * @brief
 * Unlocks both the referenced primary and client access tokens of a
 * security subject context.
 *
 * @param[in] SubjectContext
 * A valid security context with both referenced tokens.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeUnlockSubjectContext(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    PTOKEN PrimaryToken, ClientToken;
    PAGED_CODE();

    /* Read both tokens */
    PrimaryToken = SubjectContext->PrimaryToken;
    ClientToken = SubjectContext->ClientToken;

    /* Unlock the impersonation one if it's there */
    if (ClientToken)
    {
        SepReleaseTokenLock(ClientToken);
    }

    /* Always unlock the primary one */
    SepReleaseTokenLock(PrimaryToken);
}

/**
 * @brief
 * Releases both the primary and client tokens of a security
 * subject context.
 *
 * @param[in] SubjectContext
 * The captured security context.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeReleaseSubjectContext(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    PAGED_CODE();

    /* Drop reference on the primary */
    ObFastDereferenceObject(&PsGetCurrentProcess()->Token, SubjectContext->PrimaryToken);
    SubjectContext->PrimaryToken = NULL;

    /* Drop reference on the impersonation, if there was one */
    PsDereferenceImpersonationToken(SubjectContext->ClientToken);
    SubjectContext->ClientToken = NULL;
}

/**
 * @brief
 * An extended function that creates an access state.
 *
 * @param[in] Thread
 * Valid thread object where subject context is to be captured.
 *
 * @param[in] Process
 * Valid process object where subject context is to be captured.
 *
 * @param[in,out] AccessState
 * An initialized returned parameter to an access state.
 *
 * @param[in] AuxData
 * Auxiliary security data for access state.
 *
 * @param[in] Access
 * Type of access mask to assign.
 *
 * @param[in] GenericMapping
 * Generic mapping for the access state to assign.
 *
 * @return
 * Returns STATUS_SUCCESS.
 */
NTSTATUS
NTAPI
SeCreateAccessStateEx(
    _In_ PETHREAD Thread,
    _In_ PEPROCESS Process,
    _Inout_ PACCESS_STATE AccessState,
    _In_ PAUX_ACCESS_DATA AuxData,
    _In_ ACCESS_MASK Access,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    ACCESS_MASK AccessMask = Access;
    PTOKEN Token;
    PAGED_CODE();

    /* Map the Generic Acess to Specific Access if we have a Mapping */
    if ((Access & GENERIC_ACCESS) && (GenericMapping))
    {
        RtlMapGenericMask(&AccessMask, GenericMapping);
    }

    /* Initialize the Access State */
    RtlZeroMemory(AccessState, sizeof(ACCESS_STATE));
    ASSERT(AccessState->SecurityDescriptor == NULL);
    ASSERT(AccessState->PrivilegesAllocated == FALSE);

    /* Initialize and save aux data */
    RtlZeroMemory(AuxData, sizeof(AUX_ACCESS_DATA));
    AccessState->AuxData = AuxData;

    /* Capture the Subject Context */
    SeCaptureSubjectContextEx(Thread,
                              Process,
                              &AccessState->SubjectSecurityContext);

    /* Set Access State Data */
    AccessState->RemainingDesiredAccess = AccessMask;
    AccessState->OriginalDesiredAccess = AccessMask;
    ExAllocateLocallyUniqueId(&AccessState->OperationID);

    /* Get the Token to use */
    Token = SeQuerySubjectContextToken(&AccessState->SubjectSecurityContext);

    /* Check for Travers Privilege */
    if (Token->TokenFlags & TOKEN_HAS_TRAVERSE_PRIVILEGE)
    {
        /* Preserve the Traverse Privilege */
        AccessState->Flags = TOKEN_HAS_TRAVERSE_PRIVILEGE;
    }

    /* Set the Auxiliary Data */
    AuxData->PrivilegeSet = (PPRIVILEGE_SET)((ULONG_PTR)AccessState +
                                             FIELD_OFFSET(ACCESS_STATE,
                                                          Privileges));
    if (GenericMapping) AuxData->GenericMapping = *GenericMapping;

    /* Return Sucess */
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Creates an access state.
 *
 * @param[in,out] AccessState
 * An initialized returned parameter to an access state.
 *
 * @param[in] AuxData
 * Auxiliary security data for access state.
 *
 * @param[in] Access
 * Type of access mask to assign.
 *
 * @param[in] GenericMapping
 * Generic mapping for the access state to assign.
 *
 * @return
 * See SeCreateAccessStateEx.
 */
NTSTATUS
NTAPI
SeCreateAccessState(
    _Inout_ PACCESS_STATE AccessState,
    _In_ PAUX_ACCESS_DATA AuxData,
    _In_ ACCESS_MASK Access,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    /* Call the extended API */
    return SeCreateAccessStateEx(PsGetCurrentThread(),
                                 PsGetCurrentProcess(),
                                 AccessState,
                                 AuxData,
                                 Access,
                                 GenericMapping);
}

/**
 * @brief
 * Deletes an allocated access state from the memory.
 *
 * @param[in] AccessState
 * A valid access state.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeDeleteAccessState(
    _In_ PACCESS_STATE AccessState)
{
    PAUX_ACCESS_DATA AuxData;
    PAGED_CODE();

    /* Get the Auxiliary Data */
    AuxData = AccessState->AuxData;

    /* Deallocate Privileges */
    if (AccessState->PrivilegesAllocated)
        ExFreePoolWithTag(AuxData->PrivilegeSet, TAG_PRIVILEGE_SET);

    /* Deallocate Name and Type Name */
    if (AccessState->ObjectName.Buffer)
    {
        ExFreePool(AccessState->ObjectName.Buffer);
    }

    if (AccessState->ObjectTypeName.Buffer)
    {
        ExFreePool(AccessState->ObjectTypeName.Buffer);
    }

    /* Release the Subject Context */
    SeReleaseSubjectContext(&AccessState->SubjectSecurityContext);
}

/**
 * @brief
 * Sets a new generic mapping for an allocated access state.
 *
 * @param[in] AccessState
 * A valid access state.
 *
 * @param[in] GenericMapping
 * New generic mapping to assign.
 *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeSetAccessStateGenericMapping(
    _In_ PACCESS_STATE AccessState,
    _In_ PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    /* Set the Generic Mapping */
    ((PAUX_ACCESS_DATA)AccessState->AuxData)->GenericMapping = *GenericMapping;
}

/**
 * @brief
 * Creates a client security context.
 *
 * @param[in] Thread
 * Thread object of the client where impersonation has to begin.
 *
 * @param[in] Qos
 * Quality of service to specify what kind of impersonation to be done.
 *
 * @param[in] RemoteClient
 * If set to TRUE, the client that we're going to impersonate is remote.
 *
 * @param[out] ClientContext
 * The returned security client context.
 *
 * @return
 * See SepCreateClientSecurity.
 */
NTSTATUS
NTAPI
SeCreateClientSecurity(
    _In_ PETHREAD Thread,
    _In_ PSECURITY_QUALITY_OF_SERVICE Qos,
    _In_ BOOLEAN RemoteClient,
    _Out_ PSECURITY_CLIENT_CONTEXT ClientContext)
{
    TOKEN_TYPE TokenType;
    BOOLEAN ThreadEffectiveOnly;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    PACCESS_TOKEN Token;
    NTSTATUS Status;
    PAGED_CODE();

    /* Reference the correct token */
    Token = PsReferenceEffectiveToken(Thread,
                                      &TokenType,
                                      &ThreadEffectiveOnly,
                                      &ImpersonationLevel);

    /* Create client security from it */
    Status = SepCreateClientSecurity(Token,
                                     Qos,
                                     RemoteClient,
                                     TokenType,
                                     ThreadEffectiveOnly,
                                     ImpersonationLevel,
                                     ClientContext);

    /* Check if we failed or static tracking was used */
    if (!(NT_SUCCESS(Status)) || (Qos->ContextTrackingMode == SECURITY_STATIC_TRACKING))
    {
        /* Dereference our copy since it's not being used */
        ObDereferenceObject(Token);
    }

    /* Return status */
    return Status;
}

/**
 * @brief
 * Creates a client security context based upon the captured security
 * subject context.
 *
 * @param[in] SubjectContext
 * The captured subject context where client security is to be created
 * from.
 *
 * @param[in] ClientSecurityQos
 * Quality of service to specify what kind of impersonation to be done.
 *
 * @param[in] ServerIsRemote
 * If set to TRUE, the client that we're going to impersonate is remote.
 *
 * @param[out] ClientContext
 * The returned security client context.
 *
 * @return
 * See SepCreateClientSecurity.
 */
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext(
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectContext,
    _In_ PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
    _In_ BOOLEAN ServerIsRemote,
    _Out_ PSECURITY_CLIENT_CONTEXT ClientContext)
{
    PACCESS_TOKEN Token;
    NTSTATUS Status;
    PAGED_CODE();

    /* Get the right token and reference it */
    Token = SeQuerySubjectContextToken(SubjectContext);
    ObReferenceObject(Token);

    /* Create the context */
    Status = SepCreateClientSecurity(Token,
                                     ClientSecurityQos,
                                     ServerIsRemote,
                                     SubjectContext->ClientToken ?
                                     TokenImpersonation : TokenPrimary,
                                     FALSE,
                                     SubjectContext->ImpersonationLevel,
                                     ClientContext);

    /* Check if we failed or static tracking was used */
    if (!(NT_SUCCESS(Status)) ||
        (ClientSecurityQos->ContextTrackingMode == SECURITY_STATIC_TRACKING))
    {
        /* Dereference our copy since it's not being used */
        ObDereferenceObject(Token);
    }

    /* Return status */
    return Status;
}

/**
 * @brief
 * Extended function that impersonates a client.
 *
 * @param[in] ClientContext
 * A valid client context.
 *
 * @param[in] ServerThread
 * The thread where impersonation is to be done.
 *
 * @return
 * STATUS_SUCCESS is returned if the calling thread successfully impersonates
 * the client. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
SeImpersonateClientEx(
    _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
    _In_opt_ PETHREAD ServerThread)
{
    BOOLEAN EffectiveOnly;
    PAGED_CODE();

    /* Check if direct access is requested */
    if (!ClientContext->DirectlyAccessClientToken)
    {
        /* No, so get the flag from QOS */
        EffectiveOnly = ClientContext->SecurityQos.EffectiveOnly;
    }
    else
    {
        /* Yes, so see if direct access should be effective only */
        EffectiveOnly = ClientContext->DirectAccessEffectiveOnly;
    }

    /* Use the current thread if one was not passed */
    if (!ServerThread) ServerThread = PsGetCurrentThread();

    /* Call the lower layer routine */
    return PsImpersonateClient(ServerThread,
                               ClientContext->ClientToken,
                               TRUE,
                               EffectiveOnly,
                               ClientContext->SecurityQos.ImpersonationLevel);
}

/**
 * @brief
 * Impersonates a client user.
 *
 * @param[in] ClientContext
 * A valid client context.
 *
 * @param[in] ServerThread
 * The thread where impersonation is to be done.
 * *
 * @return
 * Nothing.
 */
VOID
NTAPI
SeImpersonateClient(
    _In_ PSECURITY_CLIENT_CONTEXT ClientContext,
    _In_opt_ PETHREAD ServerThread)
{
    PAGED_CODE();

    /* Call the new API */
    SeImpersonateClientEx(ClientContext, ServerThread);
}

/* EOF */
