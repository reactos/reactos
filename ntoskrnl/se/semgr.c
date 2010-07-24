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
ULONG SidInTokenCalls = 0;

extern ULONG ExpInitializationPhase;
extern ERESOURCE SepSubjectContextLock;

/* PRIVATE FUNCTIONS **********************************************************/

static BOOLEAN
INIT_FUNCTION
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
    PAGED_CODE();

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

    SidInTokenCalls++;
    if (!(SidInTokenCalls % 10000)) DPRINT1("SidInToken Calls: %d\n", SidInTokenCalls);

    if (Token->UserAndGroupCount == 0)
    {
        return FALSE;
    }

    for (i=0; i<Token->UserAndGroupCount; i++)
    {
        if (RtlEqualSid(Sid, Token->UserAndGroups[i].Sid))
        {
            if ((i == 0)|| (Token->UserAndGroups[i].Attributes & SE_GROUP_ENABLED))
            {
                return TRUE;
            }

            return FALSE;
        }
    }

    return FALSE;
}

static BOOLEAN
SepTokenIsOwner(PACCESS_TOKEN Token,
                PSECURITY_DESCRIPTOR SecurityDescriptor)
{
    NTSTATUS Status;
    PSID Sid = NULL;
    BOOLEAN Defaulted;

    Status = RtlGetOwnerSecurityDescriptor(SecurityDescriptor,
                                           &Sid,
                                           &Defaulted);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlGetOwnerSecurityDescriptor() failed (Status %lx)\n", Status);
        return FALSE;
    }

    if (Sid == NULL)
    {
        DPRINT1("Owner Sid is NULL\n");
        return FALSE;
    }

    return SepSidInToken(Token, Sid);
}

VOID
NTAPI
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

VOID
NTAPI
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


#define OLD_ACCESS_CHECK

BOOLEAN NTAPI
SepAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
               IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
               IN ACCESS_MASK DesiredAccess,
               IN ACCESS_MASK PreviouslyGrantedAccess,
               OUT PPRIVILEGE_SET* Privileges,
               IN PGENERIC_MAPPING GenericMapping,
               IN KPROCESSOR_MODE AccessMode,
               OUT PACCESS_MASK GrantedAccess,
               OUT PNTSTATUS AccessStatus)
{
    LUID_AND_ATTRIBUTES Privilege;
#ifdef OLD_ACCESS_CHECK
    ACCESS_MASK CurrentAccess, AccessMask;
#endif
    ACCESS_MASK RemainingAccess;
    ACCESS_MASK TempAccess;
    ACCESS_MASK TempGrantedAccess = 0;
    ACCESS_MASK TempDeniedAccess = 0;
    PACCESS_TOKEN Token;
    ULONG i;
    PACL Dacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    PACE CurrentAce;
    PSID Sid;
    NTSTATUS Status;
    PAGED_CODE();

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

    /* Map given accesses */
    RtlMapGenericMask(&DesiredAccess, GenericMapping);
    if (PreviouslyGrantedAccess)
        RtlMapGenericMask(&PreviouslyGrantedAccess, GenericMapping);

#ifdef OLD_ACCESS_CHECK
    CurrentAccess = PreviouslyGrantedAccess;
#endif
    /* Initialize remaining access rights */
    RemainingAccess = DesiredAccess;

    Token = SubjectSecurityContext->ClientToken ?
    SubjectSecurityContext->ClientToken : SubjectSecurityContext->PrimaryToken;

    /* Check for system security access */
    if (RemainingAccess & ACCESS_SYSTEM_SECURITY)
    {
        Privilege.Luid = SeSecurityPrivilege;
        Privilege.Attributes = SE_PRIVILEGE_ENABLED;

        /* Fail if we do not the SeSecurityPrivilege */
        if (!SepPrivilegeCheck(Token,
                               &Privilege,
                               1,
                               PRIVILEGE_SET_ALL_NECESSARY,
                               AccessMode))
        {
            *AccessStatus = STATUS_PRIVILEGE_NOT_HELD;
            return FALSE;
        }

        /* Adjust access rights */
        RemainingAccess &= ~ACCESS_SYSTEM_SECURITY;
        PreviouslyGrantedAccess |= ACCESS_SYSTEM_SECURITY;

        /* Succeed if there are no more rights to grant */
        if (RemainingAccess == 0)
        {
            *GrantedAccess = PreviouslyGrantedAccess;
            *AccessStatus = STATUS_SUCCESS;
            return TRUE;
        }
    }

    /* Get the DACL */
    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &Dacl,
                                          &Defaulted);
    if (!NT_SUCCESS(Status))
    {
        *AccessStatus = Status;
        return FALSE;
    }

    /* RULE 1: Grant desired access if the object is unprotected */
    if (Present == FALSE || Dacl == NULL)
    {
        if (DesiredAccess & MAXIMUM_ALLOWED)
        {
            *GrantedAccess = GenericMapping->GenericAll;
            *GrantedAccess |= (DesiredAccess & ~MAXIMUM_ALLOWED);
        }
        else
        {
            *GrantedAccess = DesiredAccess | PreviouslyGrantedAccess;
        }

        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }

#ifdef OLD_ACCESS_CHECK
    CurrentAccess = PreviouslyGrantedAccess;
#endif

    /* RULE 2: Check token for 'take ownership' privilege */
    if (DesiredAccess & WRITE_OWNER)
    {
        Privilege.Luid = SeTakeOwnershipPrivilege;
        Privilege.Attributes = SE_PRIVILEGE_ENABLED;

        if (SepPrivilegeCheck(Token,
                              &Privilege,
                              1,
                              PRIVILEGE_SET_ALL_NECESSARY,
                              AccessMode))
        {
            /* Adjust access rights */
            RemainingAccess &= ~WRITE_OWNER;
            PreviouslyGrantedAccess |= WRITE_OWNER;
#ifdef OLD_ACCESS_CHECK
            CurrentAccess |= WRITE_OWNER;
#endif

            /* Succeed if there are no more rights to grant */
            if (RemainingAccess == 0)
            {
                *GrantedAccess = PreviouslyGrantedAccess;
                *AccessStatus = STATUS_SUCCESS;
                return TRUE;
            }
        }
    }

    /* Deny access if the DACL is empty */
    if (Dacl->AceCount == 0)
    {
        if (RemainingAccess == MAXIMUM_ALLOWED && PreviouslyGrantedAccess != 0)
        {
            *GrantedAccess = PreviouslyGrantedAccess;
            *AccessStatus = STATUS_SUCCESS;
            return TRUE;
        }
        else
        {
            *GrantedAccess = 0;
            *AccessStatus = STATUS_ACCESS_DENIED;
            return FALSE;
        }
    }

    /* Fail if DACL is absent */
    if (Present == FALSE)
    {
        *GrantedAccess = 0;
        *AccessStatus = STATUS_ACCESS_DENIED;
        return FALSE;
    }

    /* Determine the MAXIMUM_ALLOWED access rights according to the DACL */
    if (DesiredAccess & MAXIMUM_ALLOWED)
    {
        CurrentAce = (PACE)(Dacl + 1);
        for (i = 0; i < Dacl->AceCount; i++)
        {
            if (!(CurrentAce->Header.AceFlags & INHERIT_ONLY_ACE))
            {
                Sid = (PSID)(CurrentAce + 1);
                if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
                {
                    if (SepSidInToken(Token, Sid))
                    {
                        /* Map access rights from the ACE */
                        TempAccess = CurrentAce->AccessMask;
                        RtlMapGenericMask(&TempAccess, GenericMapping);

                        /* Deny access rights that have not been granted yet */
                        TempDeniedAccess |= (TempAccess & ~TempGrantedAccess);
                    }
                }
                else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
                {
                    if (SepSidInToken(Token, Sid))
                    {
                        /* Map access rights from the ACE */
                        TempAccess = CurrentAce->AccessMask;
                        RtlMapGenericMask(&TempAccess, GenericMapping);

                        /* Grant access rights that have not been denied yet */
                        TempGrantedAccess |= (TempAccess & ~TempDeniedAccess);
                    }
                }
                else
                {
                    DPRINT1("Unsupported ACE type 0x%lx\n", CurrentAce->Header.AceType);
                }
            }

            /* Get the next ACE */
            CurrentAce = (PACE)((ULONG_PTR)CurrentAce + CurrentAce->Header.AceSize);
        }

        /* Fail if some rights have not been granted */
        RemainingAccess &= ~(MAXIMUM_ALLOWED | TempGrantedAccess);
        if (RemainingAccess != 0)
        {
            *GrantedAccess = 0;
            *AccessStatus = STATUS_ACCESS_DENIED;
            return FALSE;
        }

        /* Set granted access right and access status */
        *GrantedAccess = TempGrantedAccess | PreviouslyGrantedAccess;
        if (*GrantedAccess != 0)
        {
            *AccessStatus = STATUS_SUCCESS;
            return TRUE;
        }
        else
        {
            *AccessStatus = STATUS_ACCESS_DENIED;
            return FALSE;
        }
    }

    /* RULE 4: Grant rights according to the DACL */
    CurrentAce = (PACE)(Dacl + 1);
    for (i = 0; i < Dacl->AceCount; i++)
    {
        if (!(CurrentAce->Header.AceFlags & INHERIT_ONLY_ACE))
        {
            Sid = (PSID)(CurrentAce + 1);
            if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
            {
                if (SepSidInToken(Token, Sid))
                {
#ifdef OLD_ACCESS_CHECK
                    *GrantedAccess = 0;
                    *AccessStatus = STATUS_ACCESS_DENIED;
                    return FALSE;
#else
                    /* Map access rights from the ACE */
                    TempAccess = CurrentAce->AccessMask;
                    RtlMapGenericMask(&TempAccess, GenericMapping);

                    /* Leave if a remaining right must be denied */
                    if (RemainingAccess & TempAccess)
                        break;
#endif
                }
            }
            else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
            {
                if (SepSidInToken(Token, Sid))
                {
#ifdef OLD_ACCESS_CHECK
                    AccessMask = CurrentAce->AccessMask;
                    RtlMapGenericMask(&AccessMask, GenericMapping);
                    CurrentAccess |= AccessMask;
#else
                    /* Map access rights from the ACE */
                    TempAccess = CurrentAce->AccessMask;
                    RtlMapGenericMask(&TempAccess, GenericMapping);

                    /* Remove granted rights */
                    RemainingAccess &= ~TempAccess;
#endif
                }
            }
            else
            {
                DPRINT1("Unsupported ACE type 0x%lx\n", CurrentAce->Header.AceType);
            }
        }

        /* Get the next ACE */
        CurrentAce = (PACE)((ULONG_PTR)CurrentAce + CurrentAce->Header.AceSize);
    }

#ifdef OLD_ACCESS_CHECK
    DPRINT("CurrentAccess %08lx\n DesiredAccess %08lx\n",
           CurrentAccess, DesiredAccess);

    *GrantedAccess = CurrentAccess & DesiredAccess;

    if ((*GrantedAccess & ~VALID_INHERIT_FLAGS) == 
        (DesiredAccess & ~VALID_INHERIT_FLAGS))
    {
        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }
    else
    {
        DPRINT1("HACK: Should deny access for caller: granted 0x%lx, desired 0x%lx (generic mapping %p).\n",
                *GrantedAccess, DesiredAccess, GenericMapping);
        //*AccessStatus = STATUS_ACCESS_DENIED;
        //return FALSE;
        *AccessStatus = STATUS_SUCCESS;
        return TRUE;
    }
#else
    DPRINT("DesiredAccess %08lx\nPreviouslyGrantedAccess %08lx\nRemainingAccess %08lx\n",
           DesiredAccess, PreviouslyGrantedAccess, RemainingAccess);

    /* Fail if some rights have not been granted */
    if (RemainingAccess != 0)
    {
        *GrantedAccess = 0;
        *AccessStatus = STATUS_ACCESS_DENIED;
        return FALSE;
    }

    /* Set granted access rights */
    *GrantedAccess = DesiredAccess | PreviouslyGrantedAccess;

    DPRINT("GrantedAccess %08lx\n", *GrantedAccess);

    /* Fail if no rights have been granted */
    if (*GrantedAccess == 0)
    {
        *AccessStatus = STATUS_ACCESS_DENIED;
        return FALSE;
    }

    *AccessStatus = STATUS_SUCCESS;
    return TRUE;
#endif
}

static PSID
SepGetSDOwner(IN PSECURITY_DESCRIPTOR _SecurityDescriptor)
{
    PISECURITY_DESCRIPTOR SecurityDescriptor = _SecurityDescriptor;
    PSID Owner;

    if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
        Owner = (PSID)((ULONG_PTR)SecurityDescriptor->Owner +
                       (ULONG_PTR)SecurityDescriptor);
    else
        Owner = (PSID)SecurityDescriptor->Owner;

    return Owner;
}

static PSID
SepGetSDGroup(IN PSECURITY_DESCRIPTOR _SecurityDescriptor)
{
    PISECURITY_DESCRIPTOR SecurityDescriptor = _SecurityDescriptor;
    PSID Group;

    if (SecurityDescriptor->Control & SE_SELF_RELATIVE)
        Group = (PSID)((ULONG_PTR)SecurityDescriptor->Group +
                       (ULONG_PTR)SecurityDescriptor);
    else
        Group = (PSID)SecurityDescriptor->Group;

    return Group;
}


/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
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
    BOOLEAN ret;

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

    /* Acquire the lock if needed */
    if (!SubjectContextLocked)
        SeLockSubjectContext(SubjectSecurityContext);

    /* Check if the token is the owner and grant WRITE_DAC and READ_CONTROL rights */
    if (DesiredAccess & (WRITE_DAC | READ_CONTROL | MAXIMUM_ALLOWED))
    {
         PACCESS_TOKEN Token = SubjectSecurityContext->ClientToken ?
             SubjectSecurityContext->ClientToken : SubjectSecurityContext->PrimaryToken;

        if (SepTokenIsOwner(Token,
                            SecurityDescriptor))
        {
            if (DesiredAccess & MAXIMUM_ALLOWED)
                PreviouslyGrantedAccess |= (WRITE_DAC | READ_CONTROL);
            else
                PreviouslyGrantedAccess |= (DesiredAccess & (WRITE_DAC | READ_CONTROL));

            DesiredAccess &= ~(WRITE_DAC | READ_CONTROL);
        }
    }

    if (DesiredAccess == 0)
    {
        *GrantedAccess = PreviouslyGrantedAccess;
        *AccessStatus = STATUS_SUCCESS;
        ret = TRUE;
    }
    else
    {
        /* Call the internal function */
        ret = SepAccessCheck(SecurityDescriptor,
                             SubjectSecurityContext,
                             DesiredAccess,
                             PreviouslyGrantedAccess,
                             Privileges,
                             GenericMapping,
                             AccessMode,
                             GrantedAccess,
                             AccessStatus);
    }

    /* Release the lock if needed */
    if (!SubjectContextLocked)
        SeUnlockSubjectContext(SubjectSecurityContext);

    return ret;
}

/* SYSTEM CALLS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAccessCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
              IN HANDLE TokenHandle,
              IN ACCESS_MASK DesiredAccess,
              IN PGENERIC_MAPPING GenericMapping,
              OUT PPRIVILEGE_SET PrivilegeSet OPTIONAL,
              IN OUT PULONG PrivilegeSetLength,
              OUT PACCESS_MASK GrantedAccess,
              OUT PNTSTATUS AccessStatus)
{
    PSECURITY_DESCRIPTOR CapturedSecurityDescriptor = NULL;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ACCESS_MASK PreviouslyGrantedAccess = 0;
    PTOKEN Token;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if this is kernel mode */
    if (PreviousMode == KernelMode)
    {
        /* Check if kernel wants everything */
        if (DesiredAccess & MAXIMUM_ALLOWED)
        {
            /* Give it */
            *GrantedAccess = GenericMapping->GenericAll;
            *GrantedAccess |= (DesiredAccess &~ MAXIMUM_ALLOWED);
        }
        else
        {
            /* Just give the desired access */
            *GrantedAccess = DesiredAccess;
        }

        /* Success */
        *AccessStatus = STATUS_SUCCESS;
        return STATUS_SUCCESS;
    }

    /* Protect probe in SEH */
    _SEH2_TRY
    {
        /* Probe all pointers */
        ProbeForRead(GenericMapping, sizeof(GENERIC_MAPPING), sizeof(ULONG));
        ProbeForRead(PrivilegeSetLength, sizeof(ULONG), sizeof(ULONG));
        ProbeForWrite(PrivilegeSet, *PrivilegeSetLength, sizeof(ULONG));
        ProbeForWrite(GrantedAccess, sizeof(ACCESS_MASK), sizeof(ULONG));
        ProbeForWrite(AccessStatus, sizeof(NTSTATUS), sizeof(ULONG));
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Return the exception code */
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Check for unmapped access rights */
    if (DesiredAccess & (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL))
        return STATUS_GENERIC_NOT_MAPPED;

    /* Reference the token */
    Status = ObReferenceObjectByHandle(TokenHandle,
                                       TOKEN_QUERY,
                                       SepTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to reference token (Status %lx)\n", Status);
        return Status;
    }

    /* Check token type */
    if (Token->TokenType != TokenImpersonation)
    {
        DPRINT("No impersonation token\n");
        ObDereferenceObject(Token);
        return STATUS_NO_IMPERSONATION_TOKEN;
    }

    /* Check the impersonation level */
    if (Token->ImpersonationLevel < SecurityIdentification)
    {
        DPRINT("Impersonation level < SecurityIdentification\n");
        ObDereferenceObject(Token);
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    /* Capture the security descriptor */
    Status = SeCaptureSecurityDescriptor(SecurityDescriptor,
                                         PreviousMode,
                                         PagedPool,
                                         FALSE,
                                         &CapturedSecurityDescriptor);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("Failed to capture the Security Descriptor\n");
        ObDereferenceObject(Token);
        return Status;
    }

    /* Check the captured security descriptor */
    if (CapturedSecurityDescriptor == NULL)
    {
        DPRINT("Security Descriptor is NULL\n");
        ObDereferenceObject(Token);
        return STATUS_INVALID_SECURITY_DESCR;
    }

    /* Check security descriptor for valid owner and group */
    if (SepGetSDOwner(SecurityDescriptor) == NULL ||  // FIXME: use CapturedSecurityDescriptor
        SepGetSDGroup(SecurityDescriptor) == NULL)    // FIXME: use CapturedSecurityDescriptor
    {
        DPRINT("Security Descriptor does not have a valid group or owner\n");
        SeReleaseSecurityDescriptor(CapturedSecurityDescriptor,
                                    PreviousMode,
                                    FALSE);
        ObDereferenceObject(Token);
        return STATUS_INVALID_SECURITY_DESCR;
    }

    /* Set up the subject context, and lock it */
    SubjectSecurityContext.ClientToken = Token;
    SubjectSecurityContext.ImpersonationLevel = Token->ImpersonationLevel;
    SubjectSecurityContext.PrimaryToken = NULL;
    SubjectSecurityContext.ProcessAuditId = NULL;
    SeLockSubjectContext(&SubjectSecurityContext);

    /* Check if the token is the owner and grant WRITE_DAC and READ_CONTROL rights */
    if (DesiredAccess & (WRITE_DAC | READ_CONTROL | MAXIMUM_ALLOWED))
    {
        if (SepTokenIsOwner(Token, SecurityDescriptor)) // FIXME: use CapturedSecurityDescriptor
        {
            if (DesiredAccess & MAXIMUM_ALLOWED)
                PreviouslyGrantedAccess |= (WRITE_DAC | READ_CONTROL);
            else
                PreviouslyGrantedAccess |= (DesiredAccess & (WRITE_DAC | READ_CONTROL));

            DesiredAccess &= ~(WRITE_DAC | READ_CONTROL);
        }
    }

    if (DesiredAccess == 0)
    {
        *GrantedAccess = PreviouslyGrantedAccess;
        *AccessStatus = STATUS_SUCCESS;
    }
    else
    {
        /* Now perform the access check */
        SepAccessCheck(SecurityDescriptor, // FIXME: use CapturedSecurityDescriptor
                       &SubjectSecurityContext,
                       DesiredAccess,
                       PreviouslyGrantedAccess,
                       &PrivilegeSet, //FIXME
                       GenericMapping,
                       PreviousMode,
                       GrantedAccess,
                       AccessStatus);
    }

    /* Unlock subject context */
    SeUnlockSubjectContext(&SubjectSecurityContext);

    /* Release the captured security descriptor */
    SeReleaseSecurityDescriptor(CapturedSecurityDescriptor,
                                PreviousMode,
                                FALSE);

    /* Dereference the token */
    ObDereferenceObject(Token);

    /* Check succeeded */
    return STATUS_SUCCESS;
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
