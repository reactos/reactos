/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security access check control implementation
 * COPYRIGHT:       Copyright Timo Kreuzer <timo.kreuzer@reactos.org>
 *                  Copyright Eric Kohl
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/


/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Private function that determines whether security access rights can be given
 * to an object depending on the security descriptor and other security context
 * entities, such as an owner.
 *
 * @param[in] SecurityDescriptor
 * Security descriptor of the object that is being accessed.
 *
 * @param[in] SubjectSecurityContext
 * The captured subject security context.
 *
 * @param[in] DesiredAccess
 * Access right bitmask that the calling thread wants to acquire.
 *
 * @param[in] ObjectTypeListLength
 * The length of a object type list.
 *
 * @param[in] PreviouslyGrantedAccess
 * The access rights previously acquired in the past.
 *
 * @param[out] Privileges
 * The returned set of privileges.
 *
 * @param[in] GenericMapping
 * The generic mapping of access rights of an object type.
 *
 * @param[in] AccessMode
 * The processor request level mode.
 *
 * @param[out] GrantedAccessList
 * A list of granted access rights.
 *
 * @param[out] AccessStatusList
 * The returned status code specifying why access cannot be made
 * onto an object (if said access is denied in the first place).
 *
 * @param[in] UseResultList
 * If set to TRUE, the function will return complete lists of
 * access status codes and granted access rights.
 *
 * @return
 * Returns TRUE if access onto the specific object is allowed, FALSE
 * otherwise.
 *
 * @remarks
 * The function is currently incomplete!
 */
BOOLEAN NTAPI
SepAccessCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ ACCESS_MASK PreviouslyGrantedAccess,
    _Out_ PPRIVILEGE_SET* Privileges,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PACCESS_MASK GrantedAccessList,
    _Out_ PNTSTATUS AccessStatusList,
    _In_ BOOLEAN UseResultList)
{
    ACCESS_MASK RemainingAccess;
    ACCESS_MASK TempAccess;
    ACCESS_MASK TempGrantedAccess = 0;
    ACCESS_MASK TempDeniedAccess = 0;
    PACCESS_TOKEN Token;
    ULONG i, ResultListLength;
    PACL Dacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    PACE CurrentAce;
    PSID Sid;
    NTSTATUS Status;
    PAGED_CODE();

    DPRINT("SepAccessCheck()\n");

    /* Check for no access desired */
    if (!DesiredAccess)
    {
        /* Check if we had no previous access */
        if (!PreviouslyGrantedAccess)
        {
            /* Then there's nothing to give */
            Status = STATUS_ACCESS_DENIED;
            goto ReturnCommonStatus;
        }

        /* Return the previous access only */
        Status = STATUS_SUCCESS;
        *Privileges = NULL;
        goto ReturnCommonStatus;
    }

    /* Map given accesses */
    RtlMapGenericMask(&DesiredAccess, GenericMapping);
    if (PreviouslyGrantedAccess)
        RtlMapGenericMask(&PreviouslyGrantedAccess, GenericMapping);

    /* Initialize remaining access rights */
    RemainingAccess = DesiredAccess;

    Token = SubjectSecurityContext->ClientToken ?
        SubjectSecurityContext->ClientToken : SubjectSecurityContext->PrimaryToken;

    /* Check for ACCESS_SYSTEM_SECURITY and WRITE_OWNER access */
    Status = SePrivilegePolicyCheck(&RemainingAccess,
                                    &PreviouslyGrantedAccess,
                                    NULL,
                                    Token,
                                    NULL,
                                    UserMode);
    if (!NT_SUCCESS(Status))
    {
        goto ReturnCommonStatus;
    }

    /* Succeed if there are no more rights to grant */
    if (RemainingAccess == 0)
    {
        Status = STATUS_SUCCESS;
        goto ReturnCommonStatus;
    }

    /* Get the DACL */
    Status = RtlGetDaclSecurityDescriptor(SecurityDescriptor,
                                          &Present,
                                          &Dacl,
                                          &Defaulted);
    if (!NT_SUCCESS(Status))
    {
        goto ReturnCommonStatus;
    }

    /* RULE 1: Grant desired access if the object is unprotected */
    if (Present == FALSE || Dacl == NULL)
    {
        PreviouslyGrantedAccess |= RemainingAccess;
        if (RemainingAccess & MAXIMUM_ALLOWED)
        {
            PreviouslyGrantedAccess &= ~MAXIMUM_ALLOWED;
            PreviouslyGrantedAccess |= GenericMapping->GenericAll;
        }

        Status = STATUS_SUCCESS;
        goto ReturnCommonStatus;
    }

    /* Deny access if the DACL is empty */
    if (Dacl->AceCount == 0)
    {
        if (RemainingAccess == MAXIMUM_ALLOWED && PreviouslyGrantedAccess != 0)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
        }
        goto ReturnCommonStatus;
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
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
            goto ReturnCommonStatus;
        }

        /* Set granted access right and access status */
        PreviouslyGrantedAccess |= TempGrantedAccess;
        if (PreviouslyGrantedAccess != 0)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            Status = STATUS_ACCESS_DENIED;
        }
        goto ReturnCommonStatus;
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
                    /* Map access rights from the ACE */
                    TempAccess = CurrentAce->AccessMask;
                    RtlMapGenericMask(&TempAccess, GenericMapping);

                    /* Leave if a remaining right must be denied */
                    if (RemainingAccess & TempAccess)
                        break;
                }
            }
            else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
            {
                if (SepSidInToken(Token, Sid))
                {
                    /* Map access rights from the ACE */
                    TempAccess = CurrentAce->AccessMask;
                    DPRINT("TempAccess 0x%08lx\n", TempAccess);
                    RtlMapGenericMask(&TempAccess, GenericMapping);

                    /* Remove granted rights */
                    DPRINT("RemainingAccess 0x%08lx  TempAccess 0x%08lx\n", RemainingAccess, TempAccess);
                    RemainingAccess &= ~TempAccess;
                    DPRINT("RemainingAccess 0x%08lx\n", RemainingAccess);
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

    DPRINT("DesiredAccess %08lx\nPreviouslyGrantedAccess %08lx\nRemainingAccess %08lx\n",
           DesiredAccess, PreviouslyGrantedAccess, RemainingAccess);

    /* Fail if some rights have not been granted */
    if (RemainingAccess != 0)
    {
        DPRINT("HACK: RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n", RemainingAccess, DesiredAccess);
#if 0
        /* HACK HACK HACK */
        Status = STATUS_ACCESS_DENIED;
        goto ReturnCommonStatus;
#endif
    }

    /* Set granted access rights */
    PreviouslyGrantedAccess |= DesiredAccess;

    /* Fail if no rights have been granted */
    if (PreviouslyGrantedAccess == 0)
    {
        DPRINT1("PreviouslyGrantedAccess == 0  DesiredAccess = %08lx\n", DesiredAccess);
        Status = STATUS_ACCESS_DENIED;
        goto ReturnCommonStatus;
    }

    Status = STATUS_SUCCESS;
    goto ReturnCommonStatus;

ReturnCommonStatus:
    ResultListLength = UseResultList ? ObjectTypeListLength : 1;
    for (i = 0; i < ResultListLength; i++)
    {
        GrantedAccessList[i] = PreviouslyGrantedAccess;
        AccessStatusList[i] = Status;
    }

    return NT_SUCCESS(Status);
}

/**
 * @brief
 * Retrieves the main user from a security descriptor.
 *
 * @param[in] SecurityDescriptor
 * A valid allocated security descriptor structure where the owner
 * is to be retrieved.
 *
 * @return
 * Returns a SID that represents the main user (owner).
 */
static PSID
SepGetSDOwner(
    _In_ PSECURITY_DESCRIPTOR _SecurityDescriptor)
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

/**
 * @brief
 * Retrieves the group from a security descriptor.
 *
 * @param[in] SecurityDescriptor
 * A valid allocated security descriptor structure where the group
 * is to be retrieved.
 *
 * @return
 * Returns a SID that represents a group.
 */
static PSID
SepGetSDGroup(
    _In_ PSECURITY_DESCRIPTOR _SecurityDescriptor)
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

/**
 * @brief
 * Retrieves the length size of a set list of privileges structure.
 *
 * @param[in] PrivilegeSet
 * A valid set of privileges.
 *
 * @return
 * Returns the total length of a set of privileges.
 */
static
ULONG
SepGetPrivilegeSetLength(
    _In_ PPRIVILEGE_SET PrivilegeSet)
{
    if (PrivilegeSet == NULL)
        return 0;

    if (PrivilegeSet->PrivilegeCount == 0)
        return (ULONG)(sizeof(PRIVILEGE_SET) - sizeof(LUID_AND_ATTRIBUTES));

    return (ULONG)(sizeof(PRIVILEGE_SET) +
                   (PrivilegeSet->PrivilegeCount - 1) * sizeof(LUID_AND_ATTRIBUTES));
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Determines whether security access rights can be given to an object
 * depending on the security descriptor and other security context
 * entities, such as an owner.
 *
 * @param[in] SecurityDescriptor
 * Security descriptor of the object that is being accessed.
 *
 * @param[in] SubjectSecurityContext
 * The captured subject security context.
 *
 * @param[in] SubjectContextLocked
 * If set to TRUE, a lock must be acquired for the security subject
 * context.
 *
 * @param[in] DesiredAccess
 * Access right bitmask that the calling thread wants to acquire.
 *
 * @param[in] PreviouslyGrantedAccess
 * The access rights previously acquired in the past.
 *
 * @param[out] Privileges
 * The returned set of privileges.
 *
 * @param[in] GenericMapping
 * The generic mapping of access rights of an object type.
 *
 * @param[in] AccessMode
 * The processor request level mode.
 *
 * @param[out] GrantedAccess
 * A list of granted access rights.
 *
 * @param[out] AccessStatus
 * The returned status code specifying why access cannot be made
 * onto an object (if said access is denied in the first place).
 *
 * @return
 * Returns TRUE if access onto the specific object is allowed, FALSE
 * otherwise.
 */
BOOLEAN
NTAPI
SeAccessCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    _In_ BOOLEAN SubjectContextLocked,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ ACCESS_MASK PreviouslyGrantedAccess,
    _Out_ PPRIVILEGE_SET* Privileges,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ KPROCESSOR_MODE AccessMode,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus)
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
        if (PreviouslyGrantedAccess == 0)
        {
            DPRINT1("Request for zero access to an object. Denying.\n");
            *AccessStatus = STATUS_ACCESS_DENIED;
            ret = FALSE;
        }
        else
        {
            *AccessStatus = STATUS_SUCCESS;
            ret = TRUE;
        }
    }
    else
    {
        /* Call the internal function */
        ret = SepAccessCheck(SecurityDescriptor,
                             SubjectSecurityContext,
                             DesiredAccess,
                             NULL,
                             0,
                             PreviouslyGrantedAccess,
                             Privileges,
                             GenericMapping,
                             AccessMode,
                             GrantedAccess,
                             AccessStatus,
                             FALSE);
    }

    /* Release the lock if needed */
    if (!SubjectContextLocked)
        SeUnlockSubjectContext(SubjectSecurityContext);

    return ret;
}

/**
 * @brief
 * Determines whether security access rights can be given to an object
 * depending on the security descriptor. Unlike the regular access check
 * procedure in the NT kernel, the fast traverse check is a faster way
 * to quickly check if access can be made into an object.
 *
 * @param[in] SecurityDescriptor
 * Security descriptor of the object that is being accessed.
 *
 * @param[in] AccessState
 * An access state to determine if the access token in the current
 * security context of the object is an restricted token.
 *
 * @param[in] DesiredAccess
 * The access right bitmask where the calling thread wants to acquire.
 *
 * @param[in] AccessMode
 * Process level request mode.
 *
 * @return
 * Returns TRUE if access onto the specific object is allowed, FALSE
 * otherwise.
 */
BOOLEAN
NTAPI
SeFastTraverseCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PACCESS_STATE AccessState,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ KPROCESSOR_MODE AccessMode)
{
    PACL Dacl;
    ULONG AceIndex;
    PKNOWN_ACE Ace;

    PAGED_CODE();

    ASSERT(AccessMode != KernelMode);

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
        if (Ace->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
        {
            /* Check if all accesses are granted */
            if (!(Ace->Mask & DesiredAccess))
                continue;

            /* Check SID and grant access if matching */
            if (RtlEqualSid(SeWorldSid, &(Ace->SidStart)))
                return TRUE;
        }
        /* If access-denied ACE */
        else if (Ace->Header.AceType == ACCESS_DENIED_ACE_TYPE)
        {
            /* Here, only check if it denies any access wanted and deny if so */
            if (Ace->Mask & DesiredAccess)
                return FALSE;
        }
    }

    /* Faulty, deny */
    return FALSE;
}

/* SYSTEM CALLS ***************************************************************/

/**
 * @brief
 * Determines whether security access rights can be given to an object
 * depending on the security descriptor and a valid handle to an access
 * token.
 *
 * @param[in] SecurityDescriptor
 * Security descriptor of the object that is being accessed.
 *
 * @param[in] TokenHandle
 * A handle to a token.
 *
 * @param[in] DesiredAccess
 * The access right bitmask where the calling thread wants to acquire.
 *
 * @param[in] GenericMapping
 * The generic mapping of access rights of an object type.
 *
 * @param[out] PrivilegeSet
 * The returned set of privileges.
 *
 * @param[in,out] PrivilegeSetLength
 * The total length size of a set of privileges.
 *
 * @param[out] GrantedAccess
 * A list of granted access rights.
 *
 * @param[out] AccessStatus
 * The returned status code specifying why access cannot be made
 * onto an object (if said access is denied in the first place).
 *
 * @return
 * Returns STATUS_SUCCESS if access check has been done without problems
 * and that the object can be accessed. STATUS_GENERIC_NOT_MAPPED is returned
 * if no generic access right is mapped. STATUS_NO_IMPERSONATION_TOKEN is returned
 * if the token from the handle is not an impersonation token.
 * STATUS_BAD_IMPERSONATION_LEVEL is returned if the token cannot be impersonated
 * because the current security impersonation level doesn't permit so.
 * STATUS_INVALID_SECURITY_DESCR is returned if the security descriptor given
 * to the call is not a valid one. STATUS_BUFFER_TOO_SMALL is returned if
 * the buffer to the captured privileges has a length that is less than the required
 * size of the set of privileges. A failure NTSTATUS code is returned otherwise.
 */
NTSTATUS
NTAPI
NtAccessCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ HANDLE TokenHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_opt_ PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus)
{
    PSECURITY_DESCRIPTOR CapturedSecurityDescriptor = NULL;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ACCESS_MASK PreviouslyGrantedAccess = 0;
    PPRIVILEGE_SET Privileges = NULL;
    ULONG CapturedPrivilegeSetLength, RequiredPrivilegeSetLength;
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

        /* Capture the privilege set length and the mapping */
        CapturedPrivilegeSetLength = *PrivilegeSetLength;
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

    /* Check for ACCESS_SYSTEM_SECURITY and WRITE_OWNER access */
    Status = SePrivilegePolicyCheck(&DesiredAccess,
                                    &PreviouslyGrantedAccess,
                                    NULL,
                                    Token,
                                    &Privileges,
                                    PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("SePrivilegePolicyCheck failed (Status 0x%08lx)\n", Status);
        ObDereferenceObject(Token);
        *AccessStatus = Status;
        *GrantedAccess = 0;
        return STATUS_SUCCESS;
    }

    /* Check the size of the privilege set and return the privileges */
    if (Privileges != NULL)
    {
        DPRINT("Privileges != NULL\n");

        /* Calculate the required privilege set buffer size */
        RequiredPrivilegeSetLength = SepGetPrivilegeSetLength(Privileges);

        /* Fail if the privilege set buffer is too small */
        if (CapturedPrivilegeSetLength < RequiredPrivilegeSetLength)
        {
            ObDereferenceObject(Token);
            SeFreePrivileges(Privileges);
            *PrivilegeSetLength = RequiredPrivilegeSetLength;
            return STATUS_BUFFER_TOO_SMALL;
        }

        /* Copy the privilege set to the caller */
        RtlCopyMemory(PrivilegeSet,
                      Privileges,
                      RequiredPrivilegeSetLength);

        /* Free the local privilege set */
        SeFreePrivileges(Privileges);
    }
    else
    {
        DPRINT("Privileges == NULL\n");

        /* Fail if the privilege set buffer is too small */
        if (CapturedPrivilegeSetLength < sizeof(PRIVILEGE_SET))
        {
            ObDereferenceObject(Token);
            *PrivilegeSetLength = sizeof(PRIVILEGE_SET);
            return STATUS_BUFFER_TOO_SMALL;
        }

        /* Initialize the privilege set */
        PrivilegeSet->PrivilegeCount = 0;
        PrivilegeSet->Control = 0;
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
    if (SepGetSDOwner(CapturedSecurityDescriptor) == NULL ||
        SepGetSDGroup(CapturedSecurityDescriptor) == NULL)
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
        if (SepTokenIsOwner(Token, CapturedSecurityDescriptor, FALSE))
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
        SepAccessCheck(CapturedSecurityDescriptor,
                       &SubjectSecurityContext,
                       DesiredAccess,
                       NULL,
                       0,
                       PreviouslyGrantedAccess,
                       &PrivilegeSet, //FIXME
                       GenericMapping,
                       PreviousMode,
                       GrantedAccess,
                       AccessStatus,
                       FALSE);
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

/**
 * @brief
 * Determines whether security access could be granted or not on
 * an object by the requestor who wants such access through type.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor with information data for auditing.
 *
 * @param[in] PrincipalSelfSid
 * A principal self user SID.
 *
 * @param[in] ClientToken
 * A client access token.
 *
 * @param[in] DesiredAccess
 * The desired access masks rights requested by the caller.
 *
 * @param[in] ObjectTypeList
 * A list of object types.
 *
 * @param[in] ObjectTypeLength
 * The length size of the list.
 *
 * @param[in] GenericMapping
 * The generic mapping list of access masks rights.
 *
 * @param[in] PrivilegeSet
 * An array set of privileges.
 *
 * @param[in,out] PrivilegeSetLength
 * The length size of the array set of privileges.
 *
 * @param[out] GrantedAccess
 * The returned granted access rights.
 *
 * @param[out] AccessStatus
 * The returned NTSTATUS code indicating the final results
 * of auditing.
 *
 * @return
 * To be added...
 */
NTSTATUS
NTAPI
NtAccessCheckByType(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PSID PrincipalSelfSid,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/**
 * @brief
 * Determines whether security access could be granted or not on
 * an object by the requestor who wants such access through
 * type list.
 *
 * @param[in] SecurityDescriptor
 * A security descriptor with information data for auditing.
 *
 * @param[in] PrincipalSelfSid
 * A principal self user SID.
 *
 * @param[in] ClientToken
 * A client access token.
 *
 * @param[in] DesiredAccess
 * The desired access masks rights requested by the caller.
 *
 * @param[in] ObjectTypeList
 * A list of object types.
 *
 * @param[in] ObjectTypeLength
 * The length size of the list.
 *
 * @param[in] GenericMapping
 * The generic mapping list of access masks rights.
 *
 * @param[in] PrivilegeSet
 * An array set of privileges.
 *
 * @param[in,out] PrivilegeSetLength
 * The length size of the array set of privileges.
 *
 * @param[out] GrantedAccess
 * The returned granted access rights.
 *
 * @param[out] AccessStatus
 * The returned NTSTATUS code indicating the final results
 * of auditing.
 *
 * @return
 * To be added...
 */
NTSTATUS
NTAPI
NtAccessCheckByTypeResultList(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ PSID PrincipalSelfSid,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
