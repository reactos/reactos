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

/* FUNCTIONS ******************************************************************/

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
    PAGED_CODE();
    
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&SepSubjectContextLock, TRUE);
}

/*
 * @implemented
 */
VOID
NTAPI
SeUnlockSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    PAGED_CODE();
    
    ExReleaseResourceLite(&SepSubjectContextLock);
    KeLeaveCriticalRegion();
}

/*
 * @implemented
 */
VOID
NTAPI
SeReleaseSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext)
{
    PAGED_CODE();
    
    if (SubjectContext->PrimaryToken != NULL)
    {
        ObFastDereferenceObject(&PsGetCurrentProcess()->Token, SubjectContext->PrimaryToken);
    }
    
    if (SubjectContext->ClientToken != NULL)
    {
        ObDereferenceObject(SubjectContext->ClientToken);
    }
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

    /* Capture the Subject Context */
    SeCaptureSubjectContextEx(Thread,
                              Process,
                              &AccessState->SubjectSecurityContext);

    /* Set Access State Data */
    AccessState->AuxData = AuxData;
    AccessState->RemainingDesiredAccess  = AccessMask;
    AccessState->OriginalDesiredAccess = AccessMask;
    ExpAllocateLocallyUniqueId(&AccessState->OperationID);

    /* Get the Token to use */
    Token =  AccessState->SubjectSecurityContext.ClientToken ?
             (PTOKEN)&AccessState->SubjectSecurityContext.ClientToken :
             (PTOKEN)&AccessState->SubjectSecurityContext.PrimaryToken;

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
    if (AccessState->PrivilegesAllocated) ExFreePool(AuxData->PrivilegeSet);

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
    PACCESS_TOKEN NewToken;
    PAGED_CODE();
    
    Token = PsReferenceEffectiveToken(Thread,
                                      &TokenType,
                                      &ThreadEffectiveOnly,
                                      &ImpersonationLevel);
    if (TokenType != TokenImpersonation)
    {
        ClientContext->DirectAccessEffectiveOnly = Qos->EffectiveOnly;
    }
    else
    {
        if (Qos->ImpersonationLevel > ImpersonationLevel)
        {
            if (Token) ObDereferenceObject(Token);
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }
        
        if ((ImpersonationLevel == SecurityAnonymous) ||
            (ImpersonationLevel == SecurityIdentification) ||
            ((RemoteClient) && (ImpersonationLevel != SecurityDelegation)))
        {
            if (Token) ObDereferenceObject(Token);
            return STATUS_BAD_IMPERSONATION_LEVEL;
        }
        
        ClientContext->DirectAccessEffectiveOnly = ((ThreadEffectiveOnly) ||
                                                    (Qos->EffectiveOnly)) ?
        TRUE : FALSE;
    }
    
    if (Qos->ContextTrackingMode == SECURITY_STATIC_TRACKING)
    {
        ClientContext->DirectlyAccessClientToken = FALSE;
        Status = SeCopyClientToken(Token, ImpersonationLevel, 0, &NewToken);
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        ClientContext->DirectlyAccessClientToken = TRUE;
        if (RemoteClient != FALSE)
        {
#if 0
            SeGetTokenControlInformation(Token,
                                         &ClientContext->ClientTokenControl);
#endif
        }
        
        NewToken = Token;
    }
    
    ClientContext->SecurityQos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
    ClientContext->SecurityQos.ImpersonationLevel = Qos->ImpersonationLevel;
    ClientContext->SecurityQos.ContextTrackingMode = Qos->ContextTrackingMode;
    ClientContext->SecurityQos.EffectiveOnly = Qos->EffectiveOnly;
    ClientContext->ServerIsRemote = RemoteClient;
    ClientContext->ClientToken = NewToken;
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeCreateClientSecurityFromSubjectContext(IN PSECURITY_SUBJECT_CONTEXT SubjectContext,
                                         IN PSECURITY_QUALITY_OF_SERVICE ClientSecurityQos,
                                         IN BOOLEAN ServerIsRemote,
                                         OUT PSECURITY_CLIENT_CONTEXT ClientContext)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
SeImpersonateClientEx(IN PSECURITY_CLIENT_CONTEXT ClientContext,
                      IN PETHREAD ServerThread OPTIONAL)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @implemented
 */
VOID
NTAPI
SeImpersonateClient(IN PSECURITY_CLIENT_CONTEXT ClientContext,
                    IN PETHREAD ServerThread OPTIONAL)
{
    UCHAR b;
    
    PAGED_CODE();
    
    if (ClientContext->DirectlyAccessClientToken == FALSE)
    {
        b = ClientContext->SecurityQos.EffectiveOnly;
    }
    else
    {
        b = ClientContext->DirectAccessEffectiveOnly;
    }
    if (ServerThread == NULL)
    {
        ServerThread = PsGetCurrentThread();
    }
    PsImpersonateClient(ServerThread,
                        ClientContext->ClientToken,
                        1,
                        b,
                        ClientContext->SecurityQos.ImpersonationLevel);
}

/* EOF */
