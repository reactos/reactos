/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         Security access check control implementation
 * COPYRIGHT:       Copyright 2014 Timo Kreuzer <timo.kreuzer@reactos.org>
 *                  Copyright 2014 Eric Kohl
 *                  Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Allocates memory for the internal access check rights
 * data structure and initializes it for use for the kernel.
 * The purpose of this piece of data is to track down the
 * remaining, granted and denied access rights whilst we
 * are doing an access check procedure.
 *
 * @return
 * Returns a pointer to allocated and initialized access
 * check rights, otherwise NULL is returned.
 */
PACCESS_CHECK_RIGHTS
SepInitAccessCheckRights(VOID)
{
    PACCESS_CHECK_RIGHTS AccessRights;

    PAGED_CODE();

    /* Allocate some pool for access check rights */
    AccessRights = ExAllocatePoolWithTag(PagedPool,
                                         sizeof(ACCESS_CHECK_RIGHTS),
                                         TAG_ACCESS_CHECK_RIGHT);

    /* Bail out if we failed */
    if (!AccessRights)
    {
        return NULL;
    }

    /* Initialize the structure */
    AccessRights->RemainingAccessRights = 0;
    AccessRights->GrantedAccessRights = 0;
    AccessRights->DeniedAccessRights = 0;

    return AccessRights;
}

/**
 * @brief
 * Frees an allocated access check rights from
 * memory space after access check procedures
 * have finished.
 *
 * @param[in] AccessRights
 * A pointer to access check rights of which is
 * to be freed from memory.
 *
 * @return
 * Nothing.
 */
VOID
SepFreeAccessCheckRights(
    _In_ PACCESS_CHECK_RIGHTS AccessRights)
{
    PAGED_CODE();

    if (AccessRights)
    {
        ExFreePoolWithTag(AccessRights, TAG_ACCESS_CHECK_RIGHT);
    }
}

/**
 * @brief
 * Analyzes an access control entry that is present in a discretionary
 * access control list (DACL) for access right masks of each entry with
 * the purpose to judge whether the calling thread can be warranted
 * access check to a certain object or not.
 *
 * @param[in] ActionType
 * The type of analysis to be done against an access entry. This type
 * influences how access rights are gathered. This can either be AccessCheckMaximum
 * which means the algorithm will perform analysis against ACEs on behalf of the
 * requestor that gave us the acknowledgement that he desires MAXIMUM_ALLOWED access
 * right or AccessCheckRegular if the requestor wants a subset of access rights.
 *
 * @param[in] Dacl
 * The discretionary access control list to be given to this function. This DACL
 * must have at least one ACE currently present in the list.
 *
 * @param[in] AccessToken
 * A pointer to an access token, where an equality comparison check is performed if
 * the security identifier (SID) from a ACE of a certain object is present in this
 * token. This token represents the effective (calling thread) token of the caller.
 *
 * @param[in] PrimaryAccessToken
 * A pointer to an access token, represented as an access token associated with the
 * primary calling process. This token describes the primary security context of the
 * main process.
 *
 * @param[in] IsTokenRestricted
 * If this parameter is set to TRUE, the function considers the token pointed by
 * AccessToken parameter argument as restricted. That is, the token has restricted
 * SIDs therefore the function will act accordingly against that token by checking
 * for restricted SIDs only when doing an equaility comparison check between the
 * two identifiers.
 *
 * @param[in] AccessRightsAllocated
 * If this parameter is set to TRUE, the function will not allocate the access
 * check rights again. This is typical when we have to do additional analysis
 * of ACEs because a token has restricted SIDs (see IsTokenRestricted parameter)
 * of which we already initialized the access check rights pointer before.
 *
 * @param[in] PrincipalSelfSid
 * A pointer to a security identifier that represents a principal. A principal
 * identifies a user object which is associated with its own security descriptor.
 *
 * @param[in] GenericMapping
 * A pointer to a generic mapping that is associated with the object in question
 * being checked for access. If certain set of desired access rights have
 * a generic access right, this parameter is needed to map generic rights.
 *
 * @param[in] ObjectTypeList
 * A pointer to a list array of object types. If such array is provided to the
 * function, the algorithm will perform a different approach by doing analysis
 * against ACEs each sub-object of an object of primary level (level 0) or sub-objects
 * of a sub-object of an object. If this parameter is NULL, the function will normally
 * analyze the ACEs of a DACL of the target object itself.
 *
 * @param[in] ObjectTypeListLength
 * The length of the object type list array, pointed by ObjectTypeList. This length in
 * question represents the number of elements in such array. This parameter must be 0
 * if no array list is provided.
 *
 * @param[in] RemainingAccess
 * The remaining access rights that have yet to be granted to the calling thread
 * whomst requests access to a certain object. This parameter mustn't be 0 as
 * the remaining rights are left to be addressed. This is the case if we have
 * to address the remaining rights on a regular subset basis (the requestor
 * didn't ask for MAXIMUM_ALLOWED). Otherwise this parameter can be 0.
 *
 * @return
 * Returns a pointer to initialized access check rights after ACE analysis
 * has finished. This pointer contains the rights that have been acquired
 * in order to determine if access can be granted to the calling thread.
 * Typically this pointer contains the remaining, denied and granted rights.
 *
 * Otherwise NULL is returned and thus access check procedure can't any longer
 * continue further. We have prematurely failed this access check operation
 * at this point.
 */
PACCESS_CHECK_RIGHTS
SepAnalyzeAcesFromDacl(
    _In_ ACCESS_CHECK_RIGHT_TYPE ActionType,
    _In_ PACL Dacl,
    _In_ PACCESS_TOKEN AccessToken,
    _In_ PACCESS_TOKEN PrimaryAccessToken,
    _In_ BOOLEAN IsTokenRestricted,
    _In_ BOOLEAN AccessRightsAllocated,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_opt_ POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ ACCESS_MASK RemainingAccess)
{
    NTSTATUS Status;
    PACE CurrentAce;
    ULONG AceIndex;
    PSID Sid;
    ACCESS_MASK Access;
    PACCESS_CHECK_RIGHTS AccessRights;

    PAGED_CODE();

    /* These parameters are really needed */
    ASSERT(Dacl);
    ASSERT(AccessToken);

    /* TODO: To be removed once we support object type handling in Se */
    DBG_UNREFERENCED_PARAMETER(ObjectTypeList);
    DBG_UNREFERENCED_PARAMETER(ObjectTypeListLength);

    /* TODO: To be removed once we support compound ACEs handling in Se */
    DBG_UNREFERENCED_PARAMETER(PrimaryAccessToken);

    /*
     * Allocate memory for access check rights if
     * we have not done it so. Otherwise just use
     * the already allocated pointer. This is
     * typically when we have to do additional
     * ACEs analysis because the token has
     * restricted SIDs so we have allocated this
     * pointer before.
     */
    if (!AccessRightsAllocated)
    {
        AccessRights = SepInitAccessCheckRights();
        if (!AccessRights)
        {
            DPRINT1("SepAnalyzeAcesFromDacl(): Failed to initialize the access check rights!\n");
            return NULL;
        }
    }

    /* Determine how we should analyze the ACEs */
    switch (ActionType)
    {
        /*
         * We got the acknowledgement the calling thread desires
         * maximum rights (as according to MAXIMUM_ALLOWED access
         * mask). Analyze the ACE of the given DACL.
         */
        case AccessCheckMaximum:
        {
            /* Loop over the DACL to retrieve ACEs */
            for (AceIndex = 0; AceIndex < Dacl->AceCount; AceIndex++)
            {
                /* Obtain a ACE now */
                Status = RtlGetAce(Dacl, AceIndex, (PVOID*)&CurrentAce);

                /* Getting this ACE is important, otherwise something is seriously wrong */
                ASSERT(NT_SUCCESS(Status));

                /*
                 * Now it's time to analyze it based upon the
                 * type of this ACE we're being given.
                 */
                if (!(CurrentAce->Header.AceFlags & INHERIT_ONLY_ACE))
                {
                    if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
                    {
                        /* Get the SID from this ACE */
                        Sid = SepGetSidFromAce(ACCESS_DENIED_ACE_TYPE, CurrentAce);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, TRUE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* Deny access rights that have not been granted yet */
                            AccessRights->DeniedAccessRights |= (Access & ~AccessRights->GrantedAccessRights);
                            DPRINT("SepAnalyzeAcesFromDacl(): DeniedAccessRights 0x%08lx\n", AccessRights->DeniedAccessRights);
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
                    {
                        /* Get the SID from this ACE */
                        Sid = SepGetSidFromAce(ACCESS_ALLOWED_ACE_TYPE, CurrentAce);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, FALSE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* Grant access rights that have not been denied yet */
                            AccessRights->GrantedAccessRights |= (Access & ~AccessRights->DeniedAccessRights);
                            DPRINT("SepAnalyzeAcesFromDacl(): GrantedAccessRights 0x%08lx\n", AccessRights->GrantedAccessRights);
                        }
                    }
                    else
                    {
                        DPRINT1("SepAnalyzeAcesFromDacl(): Unsupported ACE type 0x%lx\n", CurrentAce->Header.AceType);
                    }
                }
            }

            /* We're done here */
            break;
        }

        /*
         * We got the acknowledgement the calling thread desires
         * only a subset of rights therefore we have to act a little
         * different here.
         */
        case AccessCheckRegular:
        {
            /* Cache the remaining access rights to be addressed */
            AccessRights->RemainingAccessRights = RemainingAccess;

            /* Loop over the DACL to retrieve ACEs */
            for (AceIndex = 0; AceIndex < Dacl->AceCount; AceIndex++)
            {
                /* Obtain a ACE now */
                Status = RtlGetAce(Dacl, AceIndex, (PVOID*)&CurrentAce);

                /* Getting this ACE is important, otherwise something is seriously wrong */
                ASSERT(NT_SUCCESS(Status));

                /*
                 * Now it's time to analyze it based upon the
                 * type of this ACE we're being given.
                 */
                if (!(CurrentAce->Header.AceFlags & INHERIT_ONLY_ACE))
                {
                    if (CurrentAce->Header.AceType == ACCESS_DENIED_ACE_TYPE)
                    {
                        /* Get the SID from this ACE */
                        Sid = SepGetSidFromAce(ACCESS_DENIED_ACE_TYPE, CurrentAce);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, TRUE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /*
                             * The caller requests a right that cannot be
                             * granted. Access is implicitly denied for
                             * the calling thread. Track this access right.
                             */
                            if (AccessRights->RemainingAccessRights & Access)
                            {
                                DPRINT("SepAnalyzeAcesFromDacl(): Refuted access 0x%08lx\n", Access);
                                AccessRights->DeniedAccessRights |= Access;
                                break;
                            }
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
                    {
                        /* Get the SID from this ACE */
                        Sid = SepGetSidFromAce(ACCESS_ALLOWED_ACE_TYPE, CurrentAce);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, FALSE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* Remove granted rights */
                            DPRINT("SepAnalyzeAcesFromDacl(): RemainingAccessRights 0x%08lx  Access 0x%08lx\n", AccessRights->RemainingAccessRights, Access);
                            AccessRights->RemainingAccessRights &= ~Access;
                            DPRINT("SepAnalyzeAcesFromDacl(): RemainingAccessRights 0x%08lx\n", AccessRights->RemainingAccessRights);

                            /* Track the granted access right */
                            AccessRights->GrantedAccessRights |= Access;
                        }
                    }
                    else
                    {
                        DPRINT1("SepAnalyzeAcesFromDacl(): Unsupported ACE type 0x%lx\n", CurrentAce->Header.AceType);
                    }
                }
            }

            /* We're done here */
            break;
        }

        /* We shouldn't reach here */
        DEFAULT_UNREACHABLE;
    }

    /* Return the access rights that we've got */
    return AccessRights;
}

/**
 * @brief
 * Private function that determines whether security access rights can be given
 * to the calling thread in order to access an object depending on the security
 * descriptor and other security context entities, such as an owner. This
 * function is the heart and brain of the whole access check algorithm in
 * the kernel.
 *
 * @param[in] ClientAccessToken
 * A pointer to a client (thread) access token that requests access rights
 * of an object or subset of multiple objects.
 *
 * @param[in] PrimaryAccessToken
 * A pointer to a primary access token that describes the primary security
 * context of the main calling process.
 *
 * @param[in] PrincipalSelfSid
 * A pointer to a security identifier that represents a security principal,
 * that is, a user object associated with its security descriptor.
 *
 * @param[in] DesiredAccess
 * The access rights desired by the calling thread to acquire in order to
 * access an object.
 *
 * @param[in] ObjectTypeList
 * An array list of object types to be checked against for access. The function
 * will act accordingly in this case by checking each sub-object of an object
 * of primary level and such. If this parameter is NULL, the function will
 * perform a normal access check against the target object itself.
 *
 * @param[in] ObjectTypeListLength
 * The length of a object type list. Such length represents the number of
 * elements in this list.
 *
 * @param[in] PreviouslyGrantedAccess
 * The access rights previously acquired in the past. If this parameter is 0,
 * it is deemed that the calling thread hasn't acquired any rights. Access checks
 * are more tighten in this case.
 *
 * @param[in] GenericMapping
 * A pointer to a generic mapping of access rights of the target object.
 *
 * @param[in] AccessMode
 * The processor request level mode.
 *
 * @param[in] UseResultList
 * If set to TRUE, the function will return a list of granted access rights
 * of each sub-object as well as status code for each. If this parameter is
 * set to FALSE, then the function will just return only the granted access
 * rights and status code for single object that's been target for access
 * checks.
 *
 * @param[out] Privileges
 * A pointer to a definite set of privileges that have been audited
 * whilst doing access check procedures. Such set of privileges are
 * optionally returned to the caller. This can be set to NULL if
 * the caller doesn't want to obtain a set of privileges.
 *
 * @param[out] GrantedAccessList
 * A list of granted access rights returned to the caller. This list
 * can comprehend multiple elements which represent the sub-objects
 * that have been checked or a single element which is the target
 * object itself.
 *
 * @param[out] AccessStatusList
 * A list of access status codes returned to the caller. This list
 * can comprehend multiple elements which represent the sub-objects
 * that have been checked or a single element which is the target
 * object itself.
 *
 * @return
 * Returns TRUE if access onto the specific object is allowed, FALSE
 * otherwise.
 */
BOOLEAN
NTAPI
SepAccessCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PACCESS_TOKEN ClientAccessToken,
    _In_ PACCESS_TOKEN PrimaryAccessToken,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ ACCESS_MASK PreviouslyGrantedAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_ KPROCESSOR_MODE AccessMode,
    _In_ BOOLEAN UseResultList,
    _Out_opt_ PPRIVILEGE_SET* Privileges,
    _Out_ PACCESS_MASK GrantedAccessList,
    _Out_ PNTSTATUS AccessStatusList)
{
    ACCESS_MASK RemainingAccess;
    PACCESS_CHECK_RIGHTS AccessCheckRights;
    PACCESS_TOKEN Token;
    ULONG ResultListLength;
    ULONG ResultListIndex;
    PACL Dacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    NTSTATUS Status;

    PAGED_CODE();

    /* A security descriptor must be expected for access checks */
    ASSERT(SecurityDescriptor);

    /* Assume no access check rights first */
    AccessCheckRights = NULL;

    /* Check for no access desired */
    if (!DesiredAccess)
    {
        /* Check if we had no previous access */
        if (!PreviouslyGrantedAccess)
        {
            /* Then there's nothing to give */
            DPRINT1("SepAccessCheck(): The caller has no previously granted access gained!\n");
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

    /*
     * Obtain the token provided by the caller. Client (or also
     * called impersonation or thread) token takes precedence over
     * the primary token which is the token associated with the security
     * context of the main calling process. This is because it is the
     * client itself that requests access of an object or subset of
     * multiple objects. Otherwise obtain the security context of the
     * main process (the actual primary token).
     */
    Token = ClientAccessToken ? ClientAccessToken : PrimaryAccessToken;

    /*
     * We should at least expect a primary token
     * to be present if client token is not
     * available.
     */
    ASSERT(Token);

    /*
     * Check for ACCESS_SYSTEM_SECURITY and WRITE_OWNER access.
     * Write down a set of privileges that have been checked
     * if the caller wants it.
     */
    Status = SePrivilegePolicyCheck(&RemainingAccess,
                                    &PreviouslyGrantedAccess,
                                    NULL,
                                    Token,
                                    Privileges,
                                    AccessMode);
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

    /* Grant desired access if the object is unprotected */
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
            DPRINT1("SepAccessCheck(): The DACL has no ACEs and the caller has no previously granted access!\n");
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
        }
        goto ReturnCommonStatus;
    }

    /* Determine the MAXIMUM_ALLOWED access rights according to the DACL */
    if (DesiredAccess & MAXIMUM_ALLOWED)
    {
        /* Perform access checks against ACEs from this DACL */
        AccessCheckRights = SepAnalyzeAcesFromDacl(AccessCheckMaximum,
                                                   Dacl,
                                                   Token,
                                                   PrimaryAccessToken,
                                                   FALSE,
                                                   FALSE,
                                                   PrincipalSelfSid,
                                                   GenericMapping,
                                                   ObjectTypeList,
                                                   ObjectTypeListLength,
                                                   0);

        /*
         * Getting the access check rights is very
         * important as we have to do access checks
         * depending on the kind of rights we get.
         * Fail prematurely if we can't...
         */
        if (!AccessCheckRights)
        {
            DPRINT1("SepAccessCheck(): Failed to obtain access check rights!\n");
            Status = STATUS_INSUFFICIENT_RESOURCES;
            PreviouslyGrantedAccess = 0;
            goto ReturnCommonStatus;
        }

        /*
         * Perform further access checks if this token
         * has restricted SIDs.
         */
        if (SeTokenIsRestricted(Token))
        {
            AccessCheckRights = SepAnalyzeAcesFromDacl(AccessCheckMaximum,
                                                       Dacl,
                                                       Token,
                                                       PrimaryAccessToken,
                                                       TRUE,
                                                       TRUE,
                                                       PrincipalSelfSid,
                                                       GenericMapping,
                                                       ObjectTypeList,
                                                       ObjectTypeListLength,
                                                       0);
        }

        /* Fail if some rights have not been granted */
        RemainingAccess &= ~(MAXIMUM_ALLOWED | AccessCheckRights->GrantedAccessRights);
        if (RemainingAccess != 0)
        {
            DPRINT1("SepAccessCheck(): Failed to grant access rights. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n", RemainingAccess, DesiredAccess);
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
            goto ReturnCommonStatus;
        }

        /* Set granted access right and access status */
        PreviouslyGrantedAccess |= AccessCheckRights->GrantedAccessRights;
        if (PreviouslyGrantedAccess != 0)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            DPRINT1("SepAccessCheck(): Failed to grant access rights. PreviouslyGrantedAccess == 0  DesiredAccess = %08lx\n", DesiredAccess);
            Status = STATUS_ACCESS_DENIED;
        }

        /* We have successfully granted all the rights */
        goto ReturnCommonStatus;
    }

    /* Grant rights according to the DACL */
    AccessCheckRights = SepAnalyzeAcesFromDacl(AccessCheckRegular,
                                               Dacl,
                                               Token,
                                               PrimaryAccessToken,
                                               FALSE,
                                               FALSE,
                                               PrincipalSelfSid,
                                               GenericMapping,
                                               ObjectTypeList,
                                               ObjectTypeListLength,
                                               RemainingAccess);

    /*
     * Getting the access check rights is very
     * important as we have to do access checks
     * depending on the kind of rights we get.
     * Fail prematurely if we can't...
     */
    if (!AccessCheckRights)
    {
        DPRINT1("SepAccessCheck(): Failed to obtain access check rights!\n");
        Status = STATUS_INSUFFICIENT_RESOURCES;
        PreviouslyGrantedAccess = 0;
        goto ReturnCommonStatus;
    }

    /* Fail if some rights have not been granted */
    if (AccessCheckRights->RemainingAccessRights != 0)
    {
        DPRINT1("SepAccessCheck(): Failed to grant access rights. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n", AccessCheckRights->RemainingAccessRights, DesiredAccess);
        PreviouslyGrantedAccess = 0;
        Status = STATUS_ACCESS_DENIED;
        goto ReturnCommonStatus;
    }

    /*
     * Perform further access checks if this token
     * has restricted SIDs.
     */
    if (SeTokenIsRestricted(Token))
    {
        AccessCheckRights = SepAnalyzeAcesFromDacl(AccessCheckRegular,
                                                   Dacl,
                                                   Token,
                                                   PrimaryAccessToken,
                                                   TRUE,
                                                   TRUE,
                                                   PrincipalSelfSid,
                                                   GenericMapping,
                                                   ObjectTypeList,
                                                   ObjectTypeListLength,
                                                   RemainingAccess);

        /* Fail if some rights have not been granted */
        if (AccessCheckRights->RemainingAccessRights != 0)
        {
            DPRINT1("SepAccessCheck(): Failed to grant access rights. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n", AccessCheckRights->RemainingAccessRights, DesiredAccess);
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
            goto ReturnCommonStatus;
        }
    }

    /* Set granted access rights */
    PreviouslyGrantedAccess |= DesiredAccess;

    /* Fail if no rights have been granted */
    if (PreviouslyGrantedAccess == 0)
    {
        DPRINT1("SepAccessCheck(): Failed to grant access rights. PreviouslyGrantedAccess == 0  DesiredAccess = %08lx\n", DesiredAccess);
        Status = STATUS_ACCESS_DENIED;
        goto ReturnCommonStatus;
    }

    /*
     * If we're here then we granted all the desired
     * access rights the caller wanted.
     */
    Status = STATUS_SUCCESS;

ReturnCommonStatus:
    ResultListLength = UseResultList ? ObjectTypeListLength : 1;
    for (ResultListIndex = 0; ResultListIndex < ResultListLength; ResultListIndex++)
    {
        GrantedAccessList[ResultListIndex] = PreviouslyGrantedAccess;
        AccessStatusList[ResultListIndex] = Status;
    }

    /* Free the allocated access check rights */
    SepFreeAccessCheckRights(AccessCheckRights);
    AccessCheckRights = NULL;

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
 * If set to TRUE, the caller acknowledges that the subject context
 * has already been locked by the caller himself. If set to FALSE,
 * the function locks the subject context.
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
                             SubjectSecurityContext->ClientToken,
                             SubjectSecurityContext->PrimaryToken,
                             NULL,
                             DesiredAccess,
                             NULL,
                             0,
                             PreviouslyGrantedAccess,
                             GenericMapping,
                             AccessMode,
                             FALSE,
                             Privileges,
                             GrantedAccess,
                             AccessStatus);
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
                       Token,
                       &SubjectSecurityContext.PrimaryToken,
                       NULL,
                       DesiredAccess,
                       NULL,
                       0,
                       PreviouslyGrantedAccess,
                       GenericMapping,
                       PreviousMode,
                       FALSE,
                       NULL,
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
