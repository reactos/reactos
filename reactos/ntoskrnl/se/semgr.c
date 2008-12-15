/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/semgr.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

PSE_EXPORTS SeExports = NULL;
SE_EXPORTS SepExports;

extern ULONG ExpInitializationPhase;
extern ERESOURCE SepSubjectContextLock;

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN INIT_FUNCTION
SepInitExports(VOID)
{
    SepExports.SeCreateTokenPrivilege = SeCreateTokenPrivilege;
    SepExports.SeAssignPrimaryTokenPrivilege = SeAssignPrimaryTokenPrivilege;
    SepExports.SeLockMemoryPrivilege = SeLockMemoryPrivilege;
    SepExports.SeIncreaseQuotaPrivilege = SeIncreaseQuotaPrivilege;
    SepExports.SeUnsolicitedInputPrivilege = SeUnsolicitedInputPrivilege;
    SepExports.SeTcbPrivilege = SeTcbPrivilege;
    SepExports.SeSecurityPrivilege = SeSecurityPrivilege;
    SepExports.SeTakeOwnershipPrivilege = SeTakeOwnershipPrivilege;
    SepExports.SeLoadDriverPrivilege = SeLoadDriverPrivilege;
    SepExports.SeCreatePagefilePrivilege = SeCreatePagefilePrivilege;
    SepExports.SeIncreaseBasePriorityPrivilege = SeIncreaseBasePriorityPrivilege;
    SepExports.SeSystemProfilePrivilege = SeSystemProfilePrivilege;
    SepExports.SeSystemtimePrivilege = SeSystemtimePrivilege;
    SepExports.SeProfileSingleProcessPrivilege = SeProfileSingleProcessPrivilege;
    SepExports.SeCreatePermanentPrivilege = SeCreatePermanentPrivilege;
    SepExports.SeBackupPrivilege = SeBackupPrivilege;
    SepExports.SeRestorePrivilege = SeRestorePrivilege;
    SepExports.SeShutdownPrivilege = SeShutdownPrivilege;
    SepExports.SeDebugPrivilege = SeDebugPrivilege;
    SepExports.SeAuditPrivilege = SeAuditPrivilege;
    SepExports.SeSystemEnvironmentPrivilege = SeSystemEnvironmentPrivilege;
    SepExports.SeChangeNotifyPrivilege = SeChangeNotifyPrivilege;
    SepExports.SeRemoteShutdownPrivilege = SeRemoteShutdownPrivilege;
    
    SepExports.SeNullSid = SeNullSid;
    SepExports.SeWorldSid = SeWorldSid;
    SepExports.SeLocalSid = SeLocalSid;
    SepExports.SeCreatorOwnerSid = SeCreatorOwnerSid;
    SepExports.SeCreatorGroupSid = SeCreatorGroupSid;
    SepExports.SeNtAuthoritySid = SeNtAuthoritySid;
    SepExports.SeDialupSid = SeDialupSid;
    SepExports.SeNetworkSid = SeNetworkSid;
    SepExports.SeBatchSid = SeBatchSid;
    SepExports.SeInteractiveSid = SeInteractiveSid;
    SepExports.SeLocalSystemSid = SeLocalSystemSid;
    SepExports.SeAliasAdminsSid = SeAliasAdminsSid;
    SepExports.SeAliasUsersSid = SeAliasUsersSid;
    SepExports.SeAliasGuestsSid = SeAliasGuestsSid;
    SepExports.SeAliasPowerUsersSid = SeAliasPowerUsersSid;
    SepExports.SeAliasAccountOpsSid = SeAliasAccountOpsSid;
    SepExports.SeAliasSystemOpsSid = SeAliasSystemOpsSid;
    SepExports.SeAliasPrintOpsSid = SeAliasPrintOpsSid;
    SepExports.SeAliasBackupOpsSid = SeAliasBackupOpsSid;
    SepExports.SeAuthenticatedUsersSid = SeAuthenticatedUsersSid;
    SepExports.SeRestrictedSid = SeRestrictedSid;
    SepExports.SeAnonymousLogonSid = SeAnonymousLogonSid;
    
    SepExports.SeUndockPrivilege = SeUndockPrivilege;
    SepExports.SeSyncAgentPrivilege = SeSyncAgentPrivilege;
    SepExports.SeEnableDelegationPrivilege = SeEnableDelegationPrivilege;
    
    SeExports = &SepExports;
    return TRUE;
}


BOOLEAN
NTAPI
SepInitializationPhase0(VOID)
{
    ExpInitLuid();
    if (!SepInitSecurityIDs()) return FALSE;
    if (!SepInitDACLs()) return FALSE;
    if (!SepInitSDs()) return FALSE;
    SepInitPrivileges();
    if (!SepInitExports()) return FALSE;
    
    /* Initialize the subject context lock */
    ExInitializeResource(&SepSubjectContextLock);
    
    /* Initialize token objects */
    SepInitializeTokenImplementation();
    
    /* Clear impersonation info for the idle thread */
    PsGetCurrentThread()->ImpersonationInfo = NULL;
    PspClearCrossThreadFlag(PsGetCurrentThread(),
                            CT_ACTIVE_IMPERSONATION_INFO_BIT);
    
    /* Initialize the boot token */
    ObInitializeFastReference(&PsGetCurrentProcess()->Token, NULL);
    ObInitializeFastReference(&PsGetCurrentProcess()->Token,
                              SepCreateSystemProcessToken());
    return TRUE;
}

BOOLEAN
NTAPI
SepInitializationPhase1(VOID)
{
    NTSTATUS Status;
    PAGED_CODE();
    
    /* Insert the system token into the tree */
    Status = ObInsertObject((PVOID)(PsGetCurrentProcess()->Token.Value &
                                    ~MAX_FAST_REFS),
                            NULL,
                            0,
                            0,
                            NULL,
                            NULL);
    ASSERT(NT_SUCCESS(Status));
    
    /* FIXME: TODO \\ Security directory */
    return TRUE;
}

BOOLEAN
NTAPI
SeInitSystem(VOID)
{
    /* Check the initialization phase */
    switch (ExpInitializationPhase)
    {
        case 0:
            
            /* Do Phase 0 */
            return SepInitializationPhase0();
            
        case 1:
            
            /* Do Phase 1 */
            return SepInitializationPhase1();
            
        default:
            
            /* Don't know any other phase! Bugcheck! */
            KeBugCheckEx(UNEXPECTED_INITIALIZATION_CALL,
                         0,
                         ExpInitializationPhase,
                         0,
                         0);
            return FALSE;
    }
}

BOOLEAN
NTAPI
SeInitSRM(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING Name;
    HANDLE DirectoryHandle;
    HANDLE EventHandle;
    NTSTATUS Status;
    
    /* Create '\Security' directory */
    RtlInitUnicodeString(&Name,
                         L"\\Security");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_PERMANENT,
                               0,
                               NULL);
    Status = ZwCreateDirectoryObject(&DirectoryHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create 'Security' directory!\n");
        return FALSE;
    }
    
    /* Create 'LSA_AUTHENTICATION_INITALIZED' event */
    RtlInitUnicodeString(&Name,
                         L"\\LSA_AUTHENTICATION_INITALIZED");
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_PERMANENT,
                               DirectoryHandle,
                               SePublicDefaultSd);
    Status = ZwCreateEvent(&EventHandle,
                           EVENT_ALL_ACCESS,
                           &ObjectAttributes,
                           SynchronizationEvent,
                           FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create 'LSA_AUTHENTICATION_INITALIZED' event!\n");
        NtClose(DirectoryHandle);
        return FALSE;
    }
    
    ZwClose(EventHandle);
    ZwClose(DirectoryHandle);
    
    /* FIXME: Create SRM port and listener thread */
    
    return TRUE;
}

NTSTATUS
NTAPI
SeDefaultObjectMethod(IN PVOID Object,
                      IN SECURITY_OPERATION_CODE OperationType,
                      IN PSECURITY_INFORMATION SecurityInformation,
                      IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
                      IN OUT PULONG ReturnLength OPTIONAL,
                      IN OUT PSECURITY_DESCRIPTOR *OldSecurityDescriptor,
                      IN POOL_TYPE PoolType,
                      IN PGENERIC_MAPPING GenericMapping)
{
    PAGED_CODE();
    
    /* Select the operation type */
    switch (OperationType)
    {
            /* Setting a new descriptor */
        case SetSecurityDescriptor:
            
            /* Sanity check */
            ASSERT((PoolType == PagedPool) || (PoolType == NonPagedPool));
            
            /* Set the information */
            return ObSetSecurityDescriptorInfo(Object,
                                               SecurityInformation,
                                               SecurityDescriptor,
                                               OldSecurityDescriptor,
                                               PoolType,
                                               GenericMapping);
            
        case QuerySecurityDescriptor:
            
            /* Query the information */
            return ObQuerySecurityDescriptorInfo(Object,
                                                 SecurityInformation,
                                                 SecurityDescriptor,
                                                 ReturnLength,
                                                 OldSecurityDescriptor);
            
        case DeleteSecurityDescriptor:
            
            /* De-assign it */
            return ObDeassignSecurity(OldSecurityDescriptor);
            
        case AssignSecurityDescriptor:
            
            /* Assign it */
            ObAssignObjectSecurityDescriptor(Object, SecurityDescriptor, PoolType);
            return STATUS_SUCCESS;
            
        default:
            
            /* Bug check */
            KeBugCheckEx(SECURITY_SYSTEM, 0, STATUS_INVALID_PARAMETER, 0, 0);
    }
    
    /* Should never reach here */
    ASSERT(FALSE);
    return STATUS_SUCCESS;
}


static BOOLEAN
SepSidInToken(PACCESS_TOKEN _Token,
              PSID Sid)
{
    ULONG i;
    PTOKEN Token = (PTOKEN)_Token;
    
    PAGED_CODE();
    
    if (Token->UserAndGroupCount == 0)
    {
        return FALSE;
    }
    
    for (i=0; i<Token->UserAndGroupCount; i++)
    {
        if (RtlEqualSid(Sid, Token->UserAndGroups[i].Sid))
        {
            if (Token->UserAndGroups[i].Attributes & SE_GROUP_ENABLED)
            {
                return TRUE;
            }
            
            return FALSE;
        }
    }
    
    return FALSE;
}


VOID NTAPI
SeQuerySecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                          OUT PACCESS_MASK DesiredAccess)
{
    *DesiredAccess = 0;
    
    if (SecurityInformation & (OWNER_SECURITY_INFORMATION |
                               GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION))
    {
        *DesiredAccess |= READ_CONTROL;
    }
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
}

VOID NTAPI
SeSetSecurityAccessMask(IN SECURITY_INFORMATION SecurityInformation,
                        OUT PACCESS_MASK DesiredAccess)
{
    *DesiredAccess = 0;
    
    if (SecurityInformation & (OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION))
    {
        *DesiredAccess |= WRITE_OWNER;
    }
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= WRITE_DAC;
    }
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        *DesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOLEAN NTAPI
SeAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
              IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
              IN BOOLEAN SubjectContextLocked,
              IN ACCESS_MASK DesiredAccess,
              IN ACCESS_MASK PreviouslyGrantedAccess,
              OUT PPRIVILEGE_SET* Privileges,
              IN PGENERIC_MAPPING GenericMapping,
              IN KPROCESSOR_MODE AccessMode,
              OUT PACCESS_MASK GrantedAccess,
              OUT PNTSTATUS AccessStatus)
{
    LUID_AND_ATTRIBUTES Privilege;
    ACCESS_MASK CurrentAccess, AccessMask;
    PACCESS_TOKEN Token;
    ULONG i;
    PACL Dacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    PACE CurrentAce;
    PSID Sid;
    NTSTATUS Status;
    PAGED_CODE();
    
    /* Check if this is kernel mode */
    if (AccessMode == KernelMode)
    {
        /* Check if kernel wants everything */
        if (DesiredAccess & MAXIMUM_ALLOWED)
        {
            /* Give it */
            *GrantedAccess = GenericMapping->GenericAll;
            *GrantedAccess |= (DesiredAccess &~ MAXIMUM_ALLOWED);
            *GrantedAccess |= PreviouslyGrantedAccess;
        }
        else
        {
            /* Give the desired and previous access */
            *GrantedAccess = DesiredAccess | PreviouslyGrantedAccess;
        }
        
        /* Success */
        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }
    
    /* Check if we didn't get an SD */
    if (!SecurityDescriptor)
    {
        /* Automatic failure */
        *AccessStatus = STATUS_ACCESS_DENIED;
        return FALSE;
    }
    
    /* Check for invalid impersonation */
    if ((SubjectSecurityContext->ClientToken) &&
        (SubjectSecurityContext->ImpersonationLevel < SecurityImpersonation))
    {
        *AccessStatus = STATUS_BAD_IMPERSONATION_LEVEL;
        return FALSE;
    }
    
    /* Check for no access desired */
    if (!DesiredAccess)
    {
        /* Check if we had no previous access */
        if (!PreviouslyGrantedAccess)
        {
            /* Then there's nothing to give */
            *AccessStatus = STATUS_ACCESS_DENIED;
            return FALSE;
        }
        
        /* Return the previous access only */
        *GrantedAccess = PreviouslyGrantedAccess;
        *AccessStatus = STATUS_SUCCESS;
        *Privileges = NULL;
        return TRUE;
    }
    
    /* Acquire the lock if needed */
    if (!SubjectContextLocked) SeLockSubjectContext(SubjectSecurityContext);
    
    /* Map given accesses */
    RtlMapGenericMask(&DesiredAccess, GenericMapping);
    if (PreviouslyGrantedAccess)
        RtlMapGenericMask(&PreviouslyGrantedAccess, GenericMapping);
    
    
    
    CurrentAccess = PreviouslyGrantedAccess;
    
    
    
    Token = SubjectSecurityContext->ClientToken ?
    SubjectSecurityContext->ClientToken : SubjectSecurityContext->PrimaryToken;
    
    /* Get the DACL */
    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &Dacl,
                                          &Defaulted);
    if (!NT_SUCCESS(Status))
    {
        if (SubjectContextLocked == FALSE)
        {
            SeUnlockSubjectContext(SubjectSecurityContext);
        }
        
        *AccessStatus = Status;
        return FALSE;
    }
    
    /* RULE 1: Grant desired access if the object is unprotected */
    if (Present == TRUE && Dacl == NULL)
    {
        if (SubjectContextLocked == FALSE)
        {
            SeUnlockSubjectContext(SubjectSecurityContext);
        }
        
        *GrantedAccess = DesiredAccess;
        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }
    
    CurrentAccess = PreviouslyGrantedAccess;
    
    /* RULE 2: Check token for 'take ownership' privilege */
    Privilege.Luid = SeTakeOwnershipPrivilege;
    Privilege.Attributes = SE_PRIVILEGE_ENABLED;
    
    if (SepPrivilegeCheck(Token,
                          &Privilege,
                          1,
                          PRIVILEGE_SET_ALL_NECESSARY,
                          AccessMode))
    {
        CurrentAccess |= WRITE_OWNER;
        if ((DesiredAccess & ~VALID_INHERIT_FLAGS) == 
            (CurrentAccess & ~VALID_INHERIT_FLAGS))
        {
            if (SubjectContextLocked == FALSE)
            {
                SeUnlockSubjectContext(SubjectSecurityContext);
            }
            
            *GrantedAccess = CurrentAccess;
            *AccessStatus = STATUS_SUCCESS;
            return TRUE;
        }
    }
    
    /* RULE 3: Check whether the token is the owner */
    Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
                                           &Sid,
                                           &Defaulted);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlGetOwnerSecurityDescriptor() failed (Status %lx)\n", Status);
        if (SubjectContextLocked == FALSE)
        {
            SeUnlockSubjectContext(SubjectSecurityContext);
        }
        
        *AccessStatus = Status;
        return FALSE;
    }
    
    if (Sid && SepSidInToken(Token, Sid))
    {
        CurrentAccess |= (READ_CONTROL | WRITE_DAC);
        if ((DesiredAccess & ~VALID_INHERIT_FLAGS) == 
            (CurrentAccess & ~VALID_INHERIT_FLAGS))
        {
            if (SubjectContextLocked == FALSE)
            {
                SeUnlockSubjectContext(SubjectSecurityContext);
            }
            
            *GrantedAccess = CurrentAccess;
            *AccessStatus = STATUS_SUCCESS;
            return TRUE;
        }
    }
    
    /* Fail if DACL is absent */
    if (Present == FALSE)
    {
        if (SubjectContextLocked == FALSE)
        {
            SeUnlockSubjectContext(SubjectSecurityContext);
        }
        
        *GrantedAccess = 0;
        *AccessStatus = STATUS_ACCESS_DENIED;
        return FALSE;
    }
    
    /* RULE 4: Grant rights according to the DACL */
    CurrentAce = (PACE)(Dacl + 1);
    for (i = 0; i < Dacl->AceCount; i++)
    {
        Sid = (PSID)(CurrentAce + 1);
        if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
        {
            if (SepSidInToken(Token, Sid))
            {
                if (SubjectContextLocked == FALSE)
                {
                    SeUnlockSubjectContext(SubjectSecurityContext);
                }
                
                *GrantedAccess = 0;
                *AccessStatus = STATUS_ACCESS_DENIED;
                return FALSE;
            }
        }
        
        else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            if (SepSidInToken(Token, Sid))
            {
                AccessMask = CurrentAce->AccessMask;
                RtlMapGenericMask(&AccessMask, GenericMapping);
                CurrentAccess |= AccessMask;
            }
        }
        else
        {
            DPRINT1("Unknown Ace type 0x%lx\n", CurrentAce->Header.AceType);
        }
        CurrentAce = (PACE)((ULONG_PTR)CurrentAce + CurrentAce->Header.AceSize);
    }
    
    if (SubjectContextLocked == FALSE)
    {
        SeUnlockSubjectContext(SubjectSecurityContext);
    }
    
    DPRINT("CurrentAccess %08lx\n DesiredAccess %08lx\n",
           CurrentAccess, DesiredAccess);
    
    *GrantedAccess = CurrentAccess & DesiredAccess;
    
    if (DesiredAccess & MAXIMUM_ALLOWED)
    {
        *GrantedAccess = CurrentAccess;
        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }
    else if ((*GrantedAccess & ~VALID_INHERIT_FLAGS) == 
             (DesiredAccess & ~VALID_INHERIT_FLAGS))
    {
        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }
    else
    {
        DPRINT1("Denying access for caller: granted 0x%lx, desired 0x%lx (generic mapping %p)\n",
                *GrantedAccess, DesiredAccess, GenericMapping);
        *AccessStatus = STATUS_ACCESS_DENIED;
        return FALSE;
    }
}

/* SYSTEM CALLS ***************************************************************/

NTSTATUS NTAPI
NtAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
              IN HANDLE TokenHandle,
              IN ACCESS_MASK DesiredAccess,
              IN PGENERIC_MAPPING GenericMapping,
              OUT PPRIVILEGE_SET PrivilegeSet,
              OUT PULONG ReturnLength,
              OUT PACCESS_MASK GrantedAccess,
              OUT PNTSTATUS AccessStatus)
{
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext = { NULL, 0, NULL, NULL };
    KPROCESSOR_MODE PreviousMode;
    PTOKEN Token;
    NTSTATUS Status;
    
    PAGED_CODE();
    
    DPRINT("NtAccessCheck() called\n");
    
    PreviousMode = KeGetPreviousMode();
    if (PreviousMode == KernelMode)
    {
        *GrantedAccess = DesiredAccess;
        *AccessStatus = STATUS_SUCCESS;
        return STATUS_SUCCESS;
    }
    
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       TOKEN_QUERY,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference token (Status %lx)\n", Status);
        return Status;
    }
    
    /* Check token type */
    if (Token->TokenType != TokenImpersonation)
    {
        DPRINT1("No impersonation token\n");
        ObDereferenceObject(Token);
        return STATUS_ACCESS_VIOLATION;
    }
    
    /* Check impersonation level */
    if (Token->ImpersonationLevel < SecurityIdentification)
    {
        DPRINT1("Invalid impersonation level\n");
        ObDereferenceObject(Token);
        return STATUS_ACCESS_VIOLATION;
    }
    
    SubjectSecurityContext.ClientToken = Token;
    SubjectSecurityContext.ImpersonationLevel = Token->ImpersonationLevel;
    
    /* Lock subject context */
    SeLockSubjectContext(&SubjectSecurityContext);
    
    if (SeAccessCheck(SecurityDescriptor,
                      &SubjectSecurityContext,
                      TRUE,
                      DesiredAccess,
                      0,
                      &PrivilegeSet,
                      GenericMapping,
                      PreviousMode,
                      GrantedAccess,
                      AccessStatus))
    {
        Status = *AccessStatus;
    }
    else
    {
        Status = STATUS_ACCESS_DENIED;
    }
    
    /* Unlock subject context */
    SeUnlockSubjectContext(&SubjectSecurityContext);
    
    ObDereferenceObject(Token);
    
    DPRINT("NtAccessCheck() done\n");
    
    return Status;
}

NTSTATUS
NTAPI
NtAccessCheckByType(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                    IN PSID PrincipalSelfSid,
                    IN HANDLE ClientToken,
                    IN ACCESS_MASK DesiredAccess,
                    IN POBJECT_TYPE_LIST ObjectTypeList,
                    IN ULONG ObjectTypeLength,
                    IN PGENERIC_MAPPING GenericMapping,
                    IN PPRIVILEGE_SET PrivilegeSet,
                    IN ULONG PrivilegeSetLength,
                    OUT PACCESS_MASK GrantedAccess,
                    OUT PNTSTATUS AccessStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeAndAuditAlarm(IN PUNICODE_STRING SubsystemName,
                                 IN HANDLE HandleId,
                                 IN PUNICODE_STRING ObjectTypeName,
                                 IN PUNICODE_STRING ObjectName,
                                 IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                 IN PSID PrincipalSelfSid,
                                 IN ACCESS_MASK DesiredAccess,
                                 IN AUDIT_EVENT_TYPE AuditType,
                                 IN ULONG Flags,
                                 IN POBJECT_TYPE_LIST ObjectTypeList,
                                 IN ULONG ObjectTypeLength,
                                 IN PGENERIC_MAPPING GenericMapping,
                                 IN BOOLEAN ObjectCreation,
                                 OUT PACCESS_MASK GrantedAccess,
                                 OUT PNTSTATUS AccessStatus,
                                 OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeResultList(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                              IN PSID PrincipalSelfSid,
                              IN HANDLE ClientToken,
                              IN ACCESS_MASK DesiredAccess,
                              IN POBJECT_TYPE_LIST ObjectTypeList,
                              IN ULONG ObjectTypeLength,
                              IN PGENERIC_MAPPING GenericMapping,
                              IN PPRIVILEGE_SET PrivilegeSet,
                              IN ULONG PrivilegeSetLength,
                              OUT PACCESS_MASK GrantedAccess,
                              OUT PNTSTATUS AccessStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarm(IN PUNICODE_STRING SubsystemName,
                                           IN HANDLE HandleId,
                                           IN PUNICODE_STRING ObjectTypeName,
                                           IN PUNICODE_STRING ObjectName,
                                           IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                           IN PSID PrincipalSelfSid,
                                           IN ACCESS_MASK DesiredAccess,
                                           IN AUDIT_EVENT_TYPE AuditType,
                                           IN ULONG Flags,
                                           IN POBJECT_TYPE_LIST ObjectTypeList,
                                           IN ULONG ObjectTypeLength,
                                           IN PGENERIC_MAPPING GenericMapping,
                                           IN BOOLEAN ObjectCreation,
                                           OUT PACCESS_MASK GrantedAccess,
                                           OUT PNTSTATUS AccessStatus,
                                           OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtAccessCheckByTypeResultListAndAuditAlarmByHandle(IN PUNICODE_STRING SubsystemName,
                                                   IN HANDLE HandleId,
                                                   IN HANDLE ClientToken,
                                                   IN PUNICODE_STRING ObjectTypeName,
                                                   IN PUNICODE_STRING ObjectName,
                                                   IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                                                   IN PSID PrincipalSelfSid,
                                                   IN ACCESS_MASK DesiredAccess,
                                                   IN AUDIT_EVENT_TYPE AuditType,
                                                   IN ULONG Flags,
                                                   IN POBJECT_TYPE_LIST ObjectTypeList,
                                                   IN ULONG ObjectTypeLength,
                                                   IN PGENERIC_MAPPING GenericMapping,
                                                   IN BOOLEAN ObjectCreation,
                                                   OUT PACCESS_MASK GrantedAccess,
                                                   OUT PNTSTATUS AccessStatus,
                                                   OUT PBOOLEAN GenerateOnClose)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
