/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/se/accesschk.c
 * PURPOSE:         Security manager
 *
 * PROGRAMMERS:     No programmer listed.
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/


/* PRIVATE FUNCTIONS **********************************************************/

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
        *GrantedAccess = DesiredAccess;
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
                            SecurityDescriptor,
                            FALSE))
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

/*
 * @implemented
 */
BOOLEAN
NTAPI
SeFastTraverseCheck(IN PSECURITY_DESCRIPTOR SecurityDescriptor,
                    IN PACCESS_STATE AccessState,
                    IN ACCESS_MASK DesiredAccess,
                    IN KPROCESSOR_MODE AccessMode)
{
    PACL Dacl;
    ULONG AceIndex;
    PKNOWN_ACE Ace;

    PAGED_CODE();

    NT_ASSERT(AccessMode != KernelMode);

    if (SecurityDescriptor == NULL)
        return FALSE;

    /* Get DACL */
    Dacl = SepGetDaclFromDescriptor(SecurityDescriptor);
    /* If no DACL, grant access */
    if (Dacl == NULL)
        return TRUE;

    /* No ACE -> Deny */
    if (!Dacl->AceCount)
        return FALSE;

    /* Can't perform the check on restricted token */
    if (AccessState->Flags & TOKEN_IS_RESTRICTED)
        return FALSE;

    /* Browse the ACEs */
    for (AceIndex = 0, Ace = (PKNOWN_ACE)((ULONG_PTR)Dacl + sizeof(ACL));
         AceIndex < Dacl->AceCount;
         AceIndex++, Ace = (PKNOWN_ACE)((ULONG_PTR)Ace + Ace->Header.AceSize))
    {
        if (Ace->Header.AceFlags & INHERIT_ONLY_ACE)
            continue;

        /* If access-allowed ACE */
        if (Ace->Header.AceType & ACCESS_ALLOWED_ACE_TYPE)
        {
            /* Check if all accesses are granted */
            if (!(Ace->Mask & DesiredAccess))
                continue;

            /* Check SID and grant access if matching */
            if (RtlEqualSid(SeWorldSid, &(Ace->SidStart)))
                return TRUE;
        }
        /* If access-denied ACE */
        else if (Ace->Header.AceType & ACCESS_DENIED_ACE_TYPE)
        {
            /* Here, only check if it denies all the access wanted and deny if so */
            if (Ace->Mask & DesiredAccess)
                return FALSE;
        }
    }

    /* Faulty, deny */
    return FALSE;
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
                                       SeTokenObjectType,
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
    SeCaptureSubjectContext(&SubjectSecurityContext);

    /* Lock the token */
    SepAcquireTokenLockShared(Token);

    /* Check if the token is the owner and grant WRITE_DAC and READ_CONTROL rights */
    if (DesiredAccess & (WRITE_DAC | READ_CONTROL | MAXIMUM_ALLOWED))
    {
        if (SepTokenIsOwner(Token, SecurityDescriptor, FALSE)) // FIXME: use CapturedSecurityDescriptor
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

    /* Release subject context and unlock the token */
    SeReleaseSubjectContext(&SubjectSecurityContext);
    SepReleaseTokenLock(Token);

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
                    IN OUT PULONG PrivilegeSetLength,
                    OUT PACCESS_MASK GrantedAccess,
                    OUT PNTSTATUS AccessStatus)
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
                              IN OUT PULONG PrivilegeSetLength,
                              OUT PACCESS_MASK GrantedAccess,
                              OUT PNTSTATUS AccessStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
