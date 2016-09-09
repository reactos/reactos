/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/access.c
 * PURPOSE:         Access state functions
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) -
 *                               Based on patch by Javier M. Mellid
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ERESOURCE SepSubjectContextLock;

/* PRIVATE FUNCTIONS **********************************************************/

BOOLEAN
NTAPI
SepSidInTokenEx(IN PACCESS_TOKEN _Token,
                IN PSID PrincipalSelfSid,
                IN PSID _Sid,
                IN BOOLEAN Deny,
                IN BOOLEAN Restricted)
{
    ULONG i;
    PTOKEN Token = (PTOKEN)_Token;
    PISID TokenSid, Sid = (PISID)_Sid;
    PSID_AND_ATTRIBUTES SidAndAttributes;
    ULONG SidCount, SidLength;
    USHORT SidMetadata;
    PAGED_CODE();

    /* Not yet supported */
    ASSERT(PrincipalSelfSid == NULL);
    ASSERT(Restricted == FALSE);

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
    for (i = 0; i < SidCount; i++)
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
                /* Check if the group is enabled, or used for deny only */
                if ((!(i) && !(SidAndAttributes->Attributes & SE_GROUP_USE_FOR_DENY_ONLY)) ||
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

BOOLEAN
NTAPI
SepSidInToken(IN PACCESS_TOKEN _Token,
              IN PSID Sid)
{
    /* Call extended API */
    return SepSidInTokenEx(_Token, NULL, Sid, FALSE, FALSE);
}

BOOLEAN
NTAPI
SepTokenIsOwner(IN PACCESS_TOKEN _Token,
                IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                IN BOOLEAN TokenLocked)
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

VOID
NTAPI
SeGetTokenControlInformation(IN PACCESS_TOKEN _Token,
                             OUT PTOKEN_CONTROL TokenControl)
{
    PTOKEN Token = _Token;
    PAGED_CODE();

    /* Capture the main fields */
    TokenControl->AuthenticationId = Token->AuthenticationId;
    TokenControl->TokenId = Token->TokenId;
    TokenControl->TokenSource = Token->TokenSource;

    /* Lock the token */
    SepAcquireTokenLockShared(Token);

    /* Capture the modified it */
    TokenControl->ModifiedId = Token->ModifiedId;

    /* Unlock it */
    SepReleaseTokenLock(Token);
}

NTSTATUS
NTAPI
SepCreateClientSecurity(IN PACCESS_TOKEN Token,
                        IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
                        IN BOOLEAN ServerIsRemote,
                        IN TOKEN_TYPE TokenType,
                        IN BOOLEAN ThreadEffectiveOnly,
                        IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
                        OUT PSECURITY_CLIENT_CONTEXT ClientContext)
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
        ClientContext->DirectAccessEffectiveOnly = ((ThreadEffectiveOnly) ||
                                                    (ClientSecurityQos->EffectiveOnly)) ? TRUE : FALSE;
    }

    /* Is this static tracking */
    if (ClientSecurityQos->ContextTrackingMode == SECURITY_STATIC_TRACKING)
    {
        /* Do not use direct access and make a copy */
        ClientContext->DirectlyAccessClientToken = FALSE;
        Status = SeCopyClientToken(Token, ImpersonationLevel, 0, &NewToken);
        if (!NT_SUCCESS(Status)) return Status;
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

/*
 * @implemented
 */
VOID
NTAPI
SeCaptureSubjectContextEx(IN PETHREAD Thread,
                          IN PEPROCESS Process,
                          OUT PSECURITY_SUBJECT_CONTEXT SubjectContext)
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

/*
 * @implemented
 */
VOID
NTAPI
SeCaptureSubjectContext(OUT PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    /* Call the extended API */
    SeCaptureSubjectContextEx(PsGetCurrentThread(),
                              PsGetCurrentProcess(),
                              SubjectContext);
}

/*
 * @implemented
 */
VOID
NTAPI
SeLockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
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

/*
 * @implemented
 */
VOID
NTAPI
SeUnlockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    PTOKEN PrimaryToken, ClientToken;
    PAGED_CODE();

    /* Read both tokens */
    PrimaryToken = SubjectContext->PrimaryToken;
    ClientToken = SubjectContext->ClientToken;

    /* Always unlock the primary one */
    SepReleaseTokenLock(PrimaryToken);

    /* Unlock the impersonation one if it's there */
    if (!ClientToken) return;
    SepReleaseTokenLock(ClientToken);
}

/*
 * @implemented
 */
VOID
NTAPI
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    PAGED_CODE();

    /* Drop reference on the primary */
    ObFastDereferenceObject(&PsGetCurrentProcess()->Token, SubjectContext->PrimaryToken);
    SubjectContext->PrimaryToken = NULL;

    /* Drop reference on the impersonation, if there was one */
    PsDereferenceImpersonationToken(SubjectContext->ClientToken);
    SubjectContext->ClientToken = NULL;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeCreateAccessStateEx(IN PETHREAD Thread,
                      IN PEPROCESS Process,
                      IN OUT PACCESS_STATE AccessState,
                      IN PAUX_ACCESS_DATA AuxData,
                      IN ACCESS_MASK Access,
                      IN PGENERIC_MAPPING GenericMapping)
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeCreateAccessState(IN OUT PACCESS_STATE AccessState,
                    IN PAUX_ACCESS_DATA AuxData,
                    IN ACCESS_MASK Access,
                    IN PGENERIC_MAPPING GenericMapping)
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

/*
 * @implemented
 */
VOID
NTAPI
SeDeleteAccessState(IN PACCESS_STATE AccessState)
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

/*
 * @implemented
 */
VOID
NTAPI
SeSetAccessStateGenericMapping(IN PACCESS_STATE AccessState,
                               IN PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();

    /* Set the Generic Mapping */
    ((PAUX_ACCESS_DATA)AccessState->AuxData)->GenericMapping = *GenericMapping;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeCreateClientSecurity(IN PETHREAD Thread,
                       IN PSECURITY_QUALITY_OF_SERVICE Qos,
                       IN BOOLEAN RemoteClient,
                       OUT PSECURITY_CLIENT_CONTEXT ClientContext)
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
                                         IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
                                         IN BOOLEAN ServerIsRemote,
                                         OUT PSECURITY_CLIENT_CONTEXT ClientContext)
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
SeImpersonateClientEx(IN PSECURITY_CLIENT_CONTEXT ClientContext,
                      IN PETHREAD ServerThread OPTIONAL)
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

/*
 * @implemented
 */
VOID
NTAPI
SeImpersonateClient(IN PSECURITY_CLIENT_CONTEXT ClientContext,
                    IN PETHREAD ServerThread OPTIONAL)
{
    PAGED_CODE();

    /* Call the new API */
    SeImpersonateClientEx(ClientContext, ServerThread);
}

/* EOF */
