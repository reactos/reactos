/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Security access check control implementation
 * COPYRIGHT:   Copyright 2014 Timo Kreuzer <timo.kreuzer@reactos.org>
 *              Copyright 2014 Eric Kohl
 *              Copyright 2022-2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/**
 * @brief
 * Denies access of a target object and the children objects
 * in an object type list.
 *
 * @param[in,out] ObjectTypeList
 * A pointer to an object type list where access is to be
 * denied for the target object and its children in the
 * hierarchy list.
 *
 * @param[in] ObjectTypeListLength
 * The length of the object type list. This length represents
 * the number of object elements in the list.
 *
 * @param[in] AccessMask
 * The access mask right that is to be denied for the object.
 *
 * @param[in] ObjectTypeGuid
 * A pointer to a object type GUID, that identifies the object.
 * This GUID is used to search for the target object in the list.
 * If this parameter is set to NULL, the function will deny access
 * starting from the object itself in the list (aka the root).
 */
static
VOID
SepDenyAccessObjectTypeResultList(
    _Inout_ POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PGUID ObjectTypeGuid)
{
    ULONG ObjectTypeIndex;
    ULONG ReturnedObjectIndex;
    USHORT Level;

    PAGED_CODE();

    DPRINT("Access rights 0x%08lx\n", AccessMask);

    /*
     * The object type of interest is the one that was supplied
     * by the creator who made the ACE. If the object type was
     * not supplied then we have no clear indication from where
     * shall we start updating the access rights of objects on
     * this list, so we have to begin from the root (aka the
     * object itself).
     */
    if (!ObjectTypeGuid)
    {
        DPRINT("No object type provided, updating access rights from root\n");
        ReturnedObjectIndex = 0;
        goto LoopAndUpdateRightsObjects;
    }

    /* Check if that object exists in the list */
    if (SepObjectTypeGuidInList(ObjectTypeList,
                                ObjectTypeListLength,
                                ObjectTypeGuid,
                                &ReturnedObjectIndex))
    {
LoopAndUpdateRightsObjects:
        /* Update the access rights of the target object */
        ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.DeniedAccessRights |=
            (AccessMask & ~ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.GrantedAccessRights);
        DPRINT("Denied rights 0x%08lx of target object at index %lu\n",
            ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.DeniedAccessRights, ReturnedObjectIndex);

        /* And update the children of the target object */
        for (ObjectTypeIndex = ReturnedObjectIndex + 1;
             ObjectTypeIndex < ObjectTypeListLength;
             ObjectTypeIndex++)
        {
            /*
             * Stop looking for children objects if we hit an object that has
             * the same level as the target object or less.
             */
            Level = ObjectTypeList[ObjectTypeIndex].Level;
            if (Level <= ObjectTypeList[ReturnedObjectIndex].Level)
            {
                DPRINT("We looked for all children objects, stop looking\n");
                break;
            }

            /* Update the access right of the child */
            ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.DeniedAccessRights |=
                (AccessMask & ~ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.GrantedAccessRights);
            DPRINT("Denied rights 0x%08lx of child object at index %lu\n",
                ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.DeniedAccessRights, ObjectTypeIndex);
        }
    }
}

/**
 * @brief
 * Allows access of a target object and the children objects
 * in an object type list.
 *
 * @param[in,out] ObjectTypeList
 * A pointer to an object type list where access is to be
 * allowed for the target object and its children in the
 * hierarchy list.
 *
 * @param[in] ObjectTypeListLength
 * The length of the object type list. This length represents
 * the number of object elements in the list.
 *
 * @param[in] AccessMask
 * The access mask right that is to be allowed for the object.
 *
 * @param[in] ObjectTypeGuid
 * A pointer to a object type GUID, that identifies the object.
 * This GUID is used to search for the target object in the list.
 * If this parameter is set to NULL, the function will allow access
 * starting from the object itself in the list (aka the root).
 */
static
VOID
SepAllowAccessObjectTypeResultList(
    _Inout_ POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PGUID ObjectTypeGuid)
{
    ULONG ObjectTypeIndex;
    ULONG ReturnedObjectIndex;
    USHORT Level;

    PAGED_CODE();

    DPRINT("Access rights 0x%08lx\n", AccessMask);

    /*
     * Begin updating the access rights from the root object
     * (see comment in SepDenyAccessObjectTypeListMaximum).
     */
    if (!ObjectTypeGuid)
    {
        DPRINT("No object type provided, updating access rights from root\n");
        ReturnedObjectIndex = 0;
        goto LoopAndUpdateRightsObjects;
    }

    /* Check if that object exists in the list */
    if (SepObjectTypeGuidInList(ObjectTypeList,
                                ObjectTypeListLength,
                                ObjectTypeGuid,
                                &ReturnedObjectIndex))
    {
LoopAndUpdateRightsObjects:
        /* Update the access rights of the target object */
        ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.GrantedAccessRights |=
            (AccessMask & ~ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.DeniedAccessRights);
        DPRINT("Granted rights 0x%08lx of target object at index %lu\n",
            ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.GrantedAccessRights, ReturnedObjectIndex);

        /* And update the children of the target object */
        for (ObjectTypeIndex = ReturnedObjectIndex + 1;
             ObjectTypeIndex < ObjectTypeListLength;
             ObjectTypeIndex++)
        {
            /*
             * Stop looking for children objects if we hit an object that has
             * the same level as the target object or less.
             */
            Level = ObjectTypeList[ObjectTypeIndex].Level;
            if (Level <= ObjectTypeList[ReturnedObjectIndex].Level)
            {
                break;
            }

            /* Update the access right of the child */
            ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.GrantedAccessRights |=
                (AccessMask & ~ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.DeniedAccessRights);
            DPRINT("Granted rights 0x%08lx of child object at index %lu\n",
                ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.GrantedAccessRights, ObjectTypeIndex);
        }
    }
}

/**
 * @brief
 * Denies access of a target object in the object type
 * list. This access is denied for the whole hierarchy
 * in the list.
 *
 * @param[in,out] ObjectTypeList
 * A pointer to an object type list where access is to be
 * denied for the target object. This operation applies
 * for the entire hierarchy of the object type list.
 *
 * @param[in] ObjectTypeListLength
 * The length of the object type list. This length represents
 * the number of object elements in the list.
 *
 * @param[in] AccessMask
 * The access mask right that is to be denied for the object.
 *
 * @param[in] ObjectTypeGuid
 * A pointer to a object type GUID, that identifies the object.
 * This GUID is used to search for the target object in the list.
 * If this parameter is set to NULL, the function will deny access
 * to the object itself in the list (aka the root).
 *
 * @param[out] BreakOnDeny
 * A pointer returned boolean value to the caller. The function
 * will return TRUE if the requested remaining right is denied
 * by the ACE, otherwise it returns FALSE.
 */
static
VOID
SepDenyAccessObjectTypeList(
    _Inout_ POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ ACCESS_MASK AccessMask,
    _In_opt_ PGUID ObjectTypeGuid,
    _Out_opt_ PBOOLEAN BreakOnDeny)
{
    ULONG ReturnedObjectIndex;
    BOOLEAN MustBreak;

    PAGED_CODE();

    DPRINT("Access rights 0x%08lx\n", AccessMask);

    /* Assume we do not want to break at first */
    MustBreak = FALSE;

    /*
     * If no object type was supplied then tell the caller it has to break on
     * searching for other ACEs if the requested remaining access right is
     * denied by the deny ACE itself. Track down that denied right too.
     */
    if (!ObjectTypeGuid)
    {
        if (ObjectTypeList[0].ObjectAccessRights.RemainingAccessRights & AccessMask)
        {
            DPRINT("Root object requests remaining access right that is denied 0x%08lx\n", AccessMask);
            MustBreak = TRUE;
        }

        ObjectTypeList[0].ObjectAccessRights.DeniedAccessRights |=
            (AccessMask & ~ObjectTypeList[0].ObjectAccessRights.GrantedAccessRights);
        DPRINT("Denied rights of root object 0x%08lx\n", ObjectTypeList[0].ObjectAccessRights.DeniedAccessRights);
        goto Quit;
    }

    /*
     * If the object exists tell the caller it has to break down if the requested
     * remaining access right is denied by the ACE. Track down the denied right too.
     */
    if (SepObjectTypeGuidInList(ObjectTypeList,
                                ObjectTypeListLength,
                                ObjectTypeGuid,
                                &ReturnedObjectIndex))
    {
        if (ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.RemainingAccessRights & AccessMask)
        {
            DPRINT("Object at index %lu requests remaining access right that is denied 0x%08lx\n", ReturnedObjectIndex, AccessMask);
            MustBreak = TRUE;
        }

        ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.DeniedAccessRights |=
            (AccessMask & ~ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.GrantedAccessRights);
        DPRINT("Denied rights 0x%08lx of object at index %lu\n",
            ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.DeniedAccessRights, ReturnedObjectIndex);
    }

Quit:
    /* Signal the caller he has to break if he wants to */
    if (BreakOnDeny)
    {
        *BreakOnDeny = MustBreak;
    }
}

/**
 * @brief
 * Allows access of a target object in the object type
 * list. This access is allowed for the whole hierarchy
 * in the list.
 *
 * @param[in,out] ObjectTypeList
 * A pointer to an object type list where access is to be
 * allowed for the target object. This operation applies
 * for the entire hierarchy of the object type list.
 *
 * @param[in] ObjectTypeListLength
 * The length of the object type list. This length represents
 * the number of object elements in the list.
 *
 * @param[in] AccessMask
 * The access mask right that is to be allowed for the object.
 *
 * @param[in] RemoveRemainingRights
 * If set to TRUE, the function will remove the remaining rights
 * of a target object. It will also grant access of the said object.
 * Otherwise if set to FALSE, the function will only grant access.
 *
 * @param[in] ObjectTypeGuid
 * A pointer to a object type GUID, that identifies the object.
 * This GUID is used to search for the target object in the list.
 * If this parameter is set to NULL, the function will allow access
 * to the object itself in the list (aka the root).
 */
static
VOID
SepAllowAccessObjectTypeList(
    _Inout_ POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ ACCESS_MASK AccessMask,
    _In_ BOOLEAN RemoveRemainingRights,
    _In_opt_ PGUID ObjectTypeGuid)
{
    ULONG ReturnedObjectIndex;

    PAGED_CODE();

    DPRINT("Access rights 0x%08lx\n", AccessMask);

    /*
     * If no object type was supplied then remove the remaining rights
     * of the object itself, the root. Track down that right to the
     * granted rights as well.
     */
    if (!ObjectTypeGuid)
    {
        if (RemoveRemainingRights)
        {
            ObjectTypeList[0].ObjectAccessRights.RemainingAccessRights &= ~AccessMask;
            DPRINT("Remaining rights of root object 0x%08lx\n", ObjectTypeList[0].ObjectAccessRights.RemainingAccessRights);
        }

        ObjectTypeList[0].ObjectAccessRights.GrantedAccessRights |=
            (AccessMask & ~ObjectTypeList[0].ObjectAccessRights.DeniedAccessRights);
        DPRINT("Granted rights of root object 0x%08lx\n", ObjectTypeList[0].ObjectAccessRights.GrantedAccessRights);
        return;
    }

    /*
     * Grant access to the object if it exists by removing the remaining
     * rights. Unlike the NtAccessCheckByTypeResultList variant we do not
     * care about the children of the target object beccause NtAccessCheckByType
     * will either grant or deny access to the entire hierarchy of the list.
     */
    if (SepObjectTypeGuidInList(ObjectTypeList,
                                ObjectTypeListLength,
                                ObjectTypeGuid,
                                &ReturnedObjectIndex))
    {
        /* Remove the remaining rights of that object */
        if (RemoveRemainingRights)
        {
            ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.RemainingAccessRights &= ~AccessMask;
            DPRINT("Remaining rights of object 0x%08lx at index %lu\n",
                ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.RemainingAccessRights, ReturnedObjectIndex);
        }

        /* And track it down to the granted access rights */
        ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.GrantedAccessRights |=
            (AccessMask & ~ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.DeniedAccessRights);
        DPRINT("Granted rights of object 0x%08lx at index %lu\n",
            ObjectTypeList[ReturnedObjectIndex].ObjectAccessRights.GrantedAccessRights, ReturnedObjectIndex);
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
 * @param[in] RemainingAccess
 * The remaining access rights that have yet to be granted to the calling thread
 * whomst requests access to a certain object. This parameter mustn't be 0 as
 * the remaining rights are left to be addressed. This is the case if we have
 * to address the remaining rights on a regular subset basis (the requestor
 * didn't ask for MAXIMUM_ALLOWED). Otherwise this parameter can be 0.
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
 * @param[in] UseResultList
 * This parameter is to used to determine how to perform an object type access check.
 * If set to TRUE, the function will either grant or deny access to the object and sub-objects
 * in the hierarchy list. If set to FALSE, the function will either grant or deny access to
 * the target that will affect the entire hierarchy of the list. This parameter is used
 * if the access action type is AccessCheckMaximum.
 *
 * @param[in,out] AccessCheckRights
 * A pointer to a structure that contains the access check rights. This function fills
 * up this structure with remaining, granted and denied rights to the caller for
 * access check. Henceforth, this parameter must not be NULL!
 */
static
VOID
SepAnalyzeAcesFromDacl(
    _In_ ACCESS_CHECK_RIGHT_TYPE ActionType,
    _In_ ACCESS_MASK RemainingAccess,
    _In_ PACL Dacl,
    _In_ PACCESS_TOKEN AccessToken,
    _In_ PACCESS_TOKEN PrimaryAccessToken,
    _In_ BOOLEAN IsTokenRestricted,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ PGENERIC_MAPPING GenericMapping,
    _In_opt_ POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ BOOLEAN UseResultList,
    _Inout_ PACCESS_CHECK_RIGHTS AccessCheckRights)
{
    NTSTATUS Status;
    PACE CurrentAce;
    ULONG AceIndex;
    ULONG ObjectTypeIndex;
    PSID Sid;
    PGUID ObjectTypeGuid;
    ACCESS_MASK Access;
    BOOLEAN BreakOnDeny;

    PAGED_CODE();

    /* These parameters are really needed */
    ASSERT(Dacl);
    ASSERT(AccessToken);

    /* TODO: To be removed once we support compound ACEs handling in Se */
    DBG_UNREFERENCED_PARAMETER(PrimaryAccessToken);

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
                        Sid = SepGetSidFromAce(CurrentAce);
                        ASSERT(Sid);

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
                            AccessCheckRights->DeniedAccessRights |= (Access & ~AccessCheckRights->GrantedAccessRights);
                            DPRINT("DeniedAccessRights 0x%08lx\n", AccessCheckRights->DeniedAccessRights);
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
                    {
                        /* Get the SID from this ACE */
                        Sid = SepGetSidFromAce(CurrentAce);
                        ASSERT(Sid);

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
                            AccessCheckRights->GrantedAccessRights |= (Access & ~AccessCheckRights->DeniedAccessRights);
                            DPRINT("GrantedAccessRights 0x%08lx\n", AccessCheckRights->GrantedAccessRights);
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_DENIED_OBJECT_ACE_TYPE)
                    {
                        /* Get the SID and object type from this ACE */
                        Sid = SepGetSidFromAce(CurrentAce);
                        ObjectTypeGuid = SepGetObjectTypeGuidFromAce(CurrentAce, TRUE);
                        ASSERT(Sid);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, TRUE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* If no list was passed treat this is as ACCESS_DENIED_ACE_TYPE */
                            if (!ObjectTypeList && !ObjectTypeListLength)
                            {
                                AccessCheckRights->DeniedAccessRights |= (Access & ~AccessCheckRights->GrantedAccessRights);
                                DPRINT("DeniedAccessRights 0x%08lx\n", AccessCheckRights->DeniedAccessRights);
                            }
                            else if (!UseResultList)
                            {
                                /*
                                 * We have an object type list but the caller wants to deny access
                                 * to the entire hierarchy list. Evaluate the rights of the object
                                 * for the whole list. Ignore what the function tells us if we have
                                 * to break on deny or not because we only want to keep track of
                                 * denied rights.
                                 */
                               SepDenyAccessObjectTypeList(ObjectTypeList,
                                                           ObjectTypeListLength,
                                                           Access,
                                                           ObjectTypeGuid,
                                                           NULL);
                            }
                            else
                            {
                                /* Otherwise evaluate the access rights for each sub-object */
                                SepDenyAccessObjectTypeResultList(ObjectTypeList,
                                                                  ObjectTypeListLength,
                                                                  Access,
                                                                  ObjectTypeGuid);
                            }
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE)
                    {
                        /* Get the SID and object type from this ACE */
                        Sid = SepGetSidFromAce(CurrentAce);
                        ObjectTypeGuid = SepGetObjectTypeGuidFromAce(CurrentAce, FALSE);
                        ASSERT(Sid);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, FALSE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* If no list was passed treat this is as ACCESS_ALLOWED_ACE_TYPE */
                            if (!ObjectTypeList && !ObjectTypeListLength)
                            {
                                AccessCheckRights->GrantedAccessRights |= (Access & ~AccessCheckRights->DeniedAccessRights);
                                DPRINT("GrantedAccessRights 0x%08lx\n", AccessCheckRights->GrantedAccessRights);
                            }
                            else if (!UseResultList)
                            {
                                /*
                                 * We have an object type list but the caller wants to allow access
                                 * to the entire hierarchy list. Evaluate the rights of the object
                                 * for the whole list.
                                 */
                                SepAllowAccessObjectTypeList(ObjectTypeList,
                                                             ObjectTypeListLength,
                                                             Access,
                                                             FALSE,
                                                             ObjectTypeGuid);
                            }
                            else
                            {
                                /* Otherwise evaluate the access rights for each sub-object */
                                SepAllowAccessObjectTypeResultList(ObjectTypeList,
                                                                   ObjectTypeListLength,
                                                                   Access,
                                                                   ObjectTypeGuid);
                            }
                        }
                    }
                    else
                    {
                        DPRINT1("Unsupported ACE type 0x%lx\n", CurrentAce->Header.AceType);
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
            ASSERT(RemainingAccess != 0);
            AccessCheckRights->RemainingAccessRights = RemainingAccess;

            /* Fill the remaining rights of each object in the list if we have one */
            if (ObjectTypeList && (ObjectTypeListLength != 0))
            {
                for (ObjectTypeIndex = 0;
                     ObjectTypeIndex < ObjectTypeListLength;
                     ObjectTypeIndex++)
                {
                    ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.RemainingAccessRights = RemainingAccess;
                }
            }

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
                        Sid = SepGetSidFromAce(CurrentAce);
                        ASSERT(Sid);

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
                            if (AccessCheckRights->RemainingAccessRights & Access)
                            {
                                DPRINT("Refuted access 0x%08lx\n", Access);
                                AccessCheckRights->DeniedAccessRights |= Access;
                                break;
                            }
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_ACE_TYPE)
                    {
                        /* Get the SID from this ACE */
                        Sid = SepGetSidFromAce(CurrentAce);
                        ASSERT(Sid);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, FALSE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* Remove the remaining rights */
                            DPRINT("RemainingAccessRights 0x%08lx  Access 0x%08lx\n", AccessCheckRights->RemainingAccessRights, Access);
                            AccessCheckRights->RemainingAccessRights &= ~Access;
                            DPRINT("RemainingAccessRights 0x%08lx\n", AccessCheckRights->RemainingAccessRights);

                            /* Track the granted access right */
                            AccessCheckRights->GrantedAccessRights |= Access;
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_DENIED_OBJECT_ACE_TYPE)
                    {
                        /* Get the SID and object type from this ACE */
                        Sid = SepGetSidFromAce(CurrentAce);
                        ObjectTypeGuid = SepGetObjectTypeGuidFromAce(CurrentAce, TRUE);
                        ASSERT(Sid);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, TRUE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* If no list was passed treat this is as ACCESS_DENIED_ACE_TYPE */
                            if (!ObjectTypeList && !ObjectTypeListLength)
                            {
                                if (AccessCheckRights->RemainingAccessRights & Access)
                                {
                                    DPRINT("Refuted access 0x%08lx\n", Access);
                                    AccessCheckRights->DeniedAccessRights |= Access;
                                    break;
                                }
                            }
                            else
                            {
                                /*
                                 * Otherwise evaluate the rights of the object for the entire list.
                                 * The function will signal us if the caller requested a right that is
                                 * denied by the ACE of an object in the list.
                                 */
                                SepDenyAccessObjectTypeList(ObjectTypeList,
                                                            ObjectTypeListLength,
                                                            Access,
                                                            ObjectTypeGuid,
                                                            &BreakOnDeny);

                                /* We are acknowledged the caller requested a denied right */
                                if (BreakOnDeny)
                                {
                                    DPRINT("Refuted access 0x%08lx\n", Access);
                                    break;
                                }
                            }
                        }
                    }
                    else if (CurrentAce->Header.AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE)
                    {
                        /* Get the SID and object type from this ACE */
                        Sid = SepGetSidFromAce(CurrentAce);
                        ObjectTypeGuid = SepGetObjectTypeGuidFromAce(CurrentAce, FALSE);
                        ASSERT(Sid);

                        if (SepSidInTokenEx(AccessToken, PrincipalSelfSid, Sid, FALSE, IsTokenRestricted))
                        {
                            /* Get this access right from the ACE */
                            Access = CurrentAce->AccessMask;

                            /* Map this access right if it has a generic mask right */
                            if ((Access & GENERIC_ACCESS) && GenericMapping)
                            {
                                RtlMapGenericMask(&Access, GenericMapping);
                            }

                            /* If no list was passed treat this is as ACCESS_ALLOWED_ACE_TYPE */
                            if (!ObjectTypeList && !ObjectTypeListLength)
                            {
                                /* Remove the remaining rights */
                                DPRINT("RemainingAccessRights 0x%08lx  Access 0x%08lx\n", AccessCheckRights->RemainingAccessRights, Access);
                                AccessCheckRights->RemainingAccessRights &= ~Access;
                                DPRINT("RemainingAccessRights 0x%08lx\n", AccessCheckRights->RemainingAccessRights);

                                /* Track the granted access right */
                                AccessCheckRights->GrantedAccessRights |= Access;
                            }
                            else
                            {
                                /* Otherwise evaluate the rights of the object for the entire list */
                                SepAllowAccessObjectTypeList(ObjectTypeList,
                                                             ObjectTypeListLength,
                                                             Access,
                                                             TRUE,
                                                             ObjectTypeGuid);
                            }
                        }
                    }
                    else
                    {
                        DPRINT1("Unsupported ACE type 0x%lx\n", CurrentAce->Header.AceType);
                    }
                }
            }

            /* We're done here */
            break;
        }

        /* We shouldn't reach here */
        DEFAULT_UNREACHABLE;
    }
}

/**
 * @brief
 * Private worker function that determines whether security access rights can be
 * givento the calling thread in order to access an object depending on the
 * security descriptor and other security context entities, such as an owner. This
 * function is the heart and brain of the whole access check algorithm in the kernel.
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
static
BOOLEAN
SepAccessCheckWorker(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PACCESS_TOKEN ClientAccessToken,
    _In_ PACCESS_TOKEN PrimaryAccessToken,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_TYPE_LIST_INTERNAL ObjectTypeList,
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
    ACCESS_MASK WantedRights;
    ACCESS_MASK MaskDesired;
    ACCESS_MASK GrantedRights = 0;
    ULONG ResultListIndex;
    ULONG ObjectTypeIndex;
    PACL Dacl;
    BOOLEAN Present;
    BOOLEAN Defaulted;
    NTSTATUS Status;
    BOOLEAN AccessIsGranted = FALSE;
    PACCESS_TOKEN Token = NULL;
    ACCESS_CHECK_RIGHTS AccessCheckRights = {0};

    PAGED_CODE();

    /* A security descriptor must be expected for access checks */
    ASSERT(SecurityDescriptor);

    /* Check for no access desired */
    if (!DesiredAccess)
    {
        /* Check if we had no previous access */
        if (!PreviouslyGrantedAccess)
        {
            /* Then there's nothing to give */
            DPRINT1("The caller has no previously granted access gained!\n");
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
     * Initialize the required rights if the caller wants to know access
     * for the object and each sub-object in the list.
     */
    if (UseResultList)
    {
        if (DesiredAccess & MAXIMUM_ALLOWED)
        {
            WantedRights = (DesiredAccess | PreviouslyGrantedAccess) & ~MAXIMUM_ALLOWED;
            MaskDesired = ~MAXIMUM_ALLOWED;
        }
        else
        {
            WantedRights = MaskDesired = DesiredAccess | PreviouslyGrantedAccess;
        }
    }

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

    /*
     * HACK: Temporary hack that checks if the caller passed an empty
     * generic mapping. In such cases we cannot mask out the remaining
     * access rights without a proper mapping so the only option we
     * can do is to check if the client is an administrator,
     * since they are powerful users.
     *
     * See CORE-18576 for information.
     */
    if (GenericMapping->GenericRead == 0 &&
        GenericMapping->GenericWrite == 0 &&
        GenericMapping->GenericExecute == 0 &&
        GenericMapping->GenericAll == 0)
    {
        if (SeTokenIsAdmin(Token))
        {
            /* Grant him access */
            PreviouslyGrantedAccess |= RemainingAccess;
            Status = STATUS_SUCCESS;
            goto ReturnCommonStatus;
        }

        /* It's not an admin so bail out */
        PreviouslyGrantedAccess = 0;
        Status = STATUS_ACCESS_DENIED;
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
            DPRINT1("The DACL has no ACEs and the caller has no previously granted access!\n");
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
        }
        goto ReturnCommonStatus;
    }

    /*
     * Determine the MAXIMUM_ALLOWED access rights according to the DACL.
     * Or if the caller is supplying a list of object types then determine
     * the rights of each object on that list.
     */
    if ((DesiredAccess & MAXIMUM_ALLOWED) || UseResultList)
    {
        /* Perform access checks against ACEs from this DACL */
        SepAnalyzeAcesFromDacl(AccessCheckMaximum,
                               0,
                               Dacl,
                               Token,
                               PrimaryAccessToken,
                               FALSE,
                               PrincipalSelfSid,
                               GenericMapping,
                               ObjectTypeList,
                               ObjectTypeListLength,
                               UseResultList,
                               &AccessCheckRights);

        /*
         * Perform further access checks if this token
         * has restricted SIDs.
         */
        if (SeTokenIsRestricted(Token))
        {
            SepAnalyzeAcesFromDacl(AccessCheckMaximum,
                                   0,
                                   Dacl,
                                   Token,
                                   PrimaryAccessToken,
                                   TRUE,
                                   PrincipalSelfSid,
                                   GenericMapping,
                                   ObjectTypeList,
                                   ObjectTypeListLength,
                                   UseResultList,
                                   &AccessCheckRights);
        }

        /* The caller did not provide an object type list, check access only for that object */
        if (!ObjectTypeList && !ObjectTypeListLength)
        {
            /* Fail if some rights have not been granted */
            RemainingAccess &= ~(MAXIMUM_ALLOWED | AccessCheckRights.GrantedAccessRights);
            if (RemainingAccess != 0)
            {
                DPRINT("Failed to grant access rights, access denied. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n", RemainingAccess, DesiredAccess);
                PreviouslyGrantedAccess = 0;
                Status = STATUS_ACCESS_DENIED;
                goto ReturnCommonStatus;
            }

            /* Set granted access right and access status */
            PreviouslyGrantedAccess |= AccessCheckRights.GrantedAccessRights;
            if (PreviouslyGrantedAccess != 0)
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                DPRINT("Failed to grant access rights, access denied. PreviouslyGrantedAccess == 0  DesiredAccess = %08lx\n", DesiredAccess);
                Status = STATUS_ACCESS_DENIED;
            }

            /* We are done here */
            goto ReturnCommonStatus;
        }
        else if (!UseResultList)
        {
            /*
             * We have a list but the caller wants to know if access can be granted
             * to an object in the list. Access will either be granted or denied
             * to the whole hierarchy of the list. Look for every object in the list
             * that has granted access rights and collect them.
             */
            for (ObjectTypeIndex = 0;
                 ObjectTypeIndex < ObjectTypeListLength;
                 ObjectTypeIndex++)
            {
                if (ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.GrantedAccessRights != 0)
                {
                    GrantedRights |= ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.GrantedAccessRights;
                }
            }

            /* Now check if acccess can be granted */
            RemainingAccess &= ~(MAXIMUM_ALLOWED | GrantedRights);
            if (RemainingAccess != 0)
            {
                DPRINT("Failed to grant access rights to the whole object hierarchy list, access denied. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n",
                    RemainingAccess, DesiredAccess);
                PreviouslyGrantedAccess = 0;
                Status = STATUS_ACCESS_DENIED;
                goto ReturnCommonStatus;
            }

            /* Set granted access right and access status */
            PreviouslyGrantedAccess |= GrantedRights;
            if (PreviouslyGrantedAccess != 0)
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                DPRINT("Failed to grant access rights to the whole object hierarchy list, access denied. PreviouslyGrantedAccess == 0  DesiredAccess = %08lx\n",
                    DesiredAccess);
                Status = STATUS_ACCESS_DENIED;
            }

            /* We are done here */
            goto ReturnCommonStatus;
        }
        else
        {
            /*
             * We have a list and the caller wants to know access for each
             * sub-object in the list. Report the access status and granted
             * rights for the object and each sub-object in the list.
             */
            for (ObjectTypeIndex = 0;
                 ObjectTypeIndex < ObjectTypeListLength;
                 ObjectTypeIndex++)
            {
                /* Check if we have some rights */
                GrantedRights = (ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.GrantedAccessRights | PreviouslyGrantedAccess) & MaskDesired;
                if (GrantedRights != 0)
                {
                    /*
                     * If we still have some remaining rights to grant the ultimate
                     * conclusion is that the caller has no access to the object itself.
                     */
                    RemainingAccess = (~GrantedRights & WantedRights);
                    if (RemainingAccess != 0)
                    {
                        DPRINT("Failed to grant access rights at specific object at index %lu, access denied. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n",
                            ObjectTypeIndex, RemainingAccess, DesiredAccess);
                        AccessStatusList[ObjectTypeIndex] = STATUS_ACCESS_DENIED;
                    }
                    else
                    {
                        AccessStatusList[ObjectTypeIndex] = STATUS_SUCCESS;
                    }
                }
                else
                {
                    /* No access is given */
                    DPRINT("Failed to grant access rights at specific object at index %lu. No access is given\n", ObjectTypeIndex);
                    AccessStatusList[ObjectTypeIndex] = STATUS_ACCESS_DENIED;
                }

                /* Return the access rights to the caller */
                GrantedAccessList[ObjectTypeIndex] = GrantedRights;
            }

            /*
             * We have built a list of access statuses for each object but
             * we still need to figure out the common status for the
             * function. The same status code will be used to check if
             * we should report any security debug stuff once we are done.
             */
            Status = STATUS_SUCCESS;
            for (ResultListIndex = 0; ResultListIndex < ObjectTypeListLength; ResultListIndex++)
            {
                /* There is at least one sub-object of which access cannot be granted */
                if (AccessStatusList[ResultListIndex] == STATUS_ACCESS_DENIED)
                {
                    Status = AccessStatusList[ResultListIndex];
                    break;
                }
            }

            /* We are done here */
            goto ReturnCommonStatus;
        }
    }

    /* Grant rights according to the DACL */
    SepAnalyzeAcesFromDacl(AccessCheckRegular,
                           RemainingAccess,
                           Dacl,
                           Token,
                           PrimaryAccessToken,
                           FALSE,
                           PrincipalSelfSid,
                           GenericMapping,
                           ObjectTypeList,
                           ObjectTypeListLength,
                           UseResultList,
                           &AccessCheckRights);

    /* The caller did not provide an object type list, check access only for that object */
    if (!ObjectTypeList && !ObjectTypeListLength)
    {
        /* Fail if some rights have not been granted */
        if (AccessCheckRights.RemainingAccessRights != 0)
        {
            DPRINT("Failed to grant access rights, access denied. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n", AccessCheckRights.RemainingAccessRights, DesiredAccess);
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
            goto ReturnCommonStatus;
        }
    }
    else
    {
        /*
         * We have an object type list, look for the object of which
         * remaining rights are all granted.
         */
        for (ObjectTypeIndex = 0;
             ObjectTypeIndex < ObjectTypeListLength;
             ObjectTypeIndex++)
        {
            if (ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.RemainingAccessRights == 0)
            {
                AccessIsGranted = TRUE;
                break;
            }
        }

        if (!AccessIsGranted)
        {
            DPRINT("Failed to grant access rights to the whole object hierarchy list, access denied. DesiredAccess = 0x%08lx\n", DesiredAccess);
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
            goto ReturnCommonStatus;
        }
    }

    /*
     * Perform further access checks if this token
     * has restricted SIDs.
     */
    if (SeTokenIsRestricted(Token))
    {
        SepAnalyzeAcesFromDacl(AccessCheckRegular,
                               RemainingAccess,
                               Dacl,
                               Token,
                               PrimaryAccessToken,
                               TRUE,
                               PrincipalSelfSid,
                               GenericMapping,
                               ObjectTypeList,
                               ObjectTypeListLength,
                               UseResultList,
                               &AccessCheckRights);

        /* The caller did not provide an object type list, check access only for that object */
        if (!ObjectTypeList && !ObjectTypeListLength)
        {
            /* Fail if some rights have not been granted */
            if (AccessCheckRights.RemainingAccessRights != 0)
            {
                DPRINT("Failed to grant access rights, access denied. RemainingAccess = 0x%08lx  DesiredAccess = 0x%08lx\n", AccessCheckRights.RemainingAccessRights, DesiredAccess);
                PreviouslyGrantedAccess = 0;
                Status = STATUS_ACCESS_DENIED;
                goto ReturnCommonStatus;
            }
        }
        else
        {
            /*
             * We have an object type list, look for the object of which remaining
             * rights are all granted. The user may have access to the requested
             * object but on a restricted token case the user is only granted partial
             * access. If access is denied to restricted SIDs, the bottom line is that
             * access is denied to the user.
             */
            AccessIsGranted = FALSE;
            for (ObjectTypeIndex = 0;
                 ObjectTypeIndex < ObjectTypeListLength;
                 ObjectTypeIndex++)
            {
                if (ObjectTypeList[ObjectTypeIndex].ObjectAccessRights.RemainingAccessRights == 0)
                {
                    AccessIsGranted = TRUE;
                    break;
                }
            }

            if (!AccessIsGranted)
            {
                DPRINT("Failed to grant access rights to the whole object hierarchy list, access denied. DesiredAccess = 0x%08lx\n", DesiredAccess);
                PreviouslyGrantedAccess = 0;
                Status = STATUS_ACCESS_DENIED;
                goto ReturnCommonStatus;
            }
        }
    }

    /* Set granted access rights */
    PreviouslyGrantedAccess |= DesiredAccess;

    /* Fail if no rights have been granted */
    if (PreviouslyGrantedAccess == 0)
    {
        DPRINT("Failed to grant access rights, access denied. PreviouslyGrantedAccess == 0  DesiredAccess = %08lx\n", DesiredAccess);
        Status = STATUS_ACCESS_DENIED;
        goto ReturnCommonStatus;
    }

    /*
     * If we're here then we granted all the desired
     * access rights the caller wanted.
     */
    Status = STATUS_SUCCESS;

ReturnCommonStatus:
    if (!UseResultList)
    {
        *GrantedAccessList = PreviouslyGrantedAccess;
        *AccessStatusList = Status;
    }

#if DBG
    /* Dump security debug info on access denied case */
    if (Status == STATUS_ACCESS_DENIED)
    {
        SepDumpSdDebugInfo(SecurityDescriptor);
        SepDumpTokenDebugInfo(Token);

        if (ObjectTypeList && (ObjectTypeListLength != 0))
        {
            SepDumpAccessAndStatusList(GrantedAccessList,
                                       AccessStatusList,
                                       UseResultList,
                                       ObjectTypeList,
                                       ObjectTypeListLength);
        }
        else
        {
            SepDumpAccessRightsStats(&AccessCheckRights);
        }
    }
#endif

    return NT_SUCCESS(Status);
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

/**
 * @brief
 * Internal function that performs a security check against the
 * client who requests access on a resource object. This function
 * is used by access check NT system calls.
 *
 * @param[in] SecurityDescriptor
 * A pointer to a security descriptor that identifies the security
 * information of an object being accessed. This function walks
 * through this descriptor for any ACLs and respective access
 * rights if access can be granted.
 *
 * @param[in] ClientToken
 * A handle to an access token, that identifies the client of which
 * requests access to the target object.
 *
 * @param[in] PrincipalSelfSid
 * A pointer to a principal self SID. This parameter can be NULL if
 * the associated object being checked for access does not represent
 * a principal.
 *
 * @param[in] DesiredAccess
 * The access right bitmask where the client wants to acquire. This
 * can be an OR'ed set of multiple access rights or MAXIMUM_ALLOWED
 * to request all of possible access rights the target object allows.
 * If only some rights were granted but not all the access is deemed
 * as denied.
 *
 * @param[in] GenericMapping
 * The generic mapping of access rights of an object type.
 *
 * @param[out] PrivilegeSet
 * A pointer to a set of privileges that were used to perform the
 * access check, returned to caller. This function will return no
 * privileges (privilege count set to 0) if no privileges were used
 * to accomplish the access check. This parameter must not be NULL!
 *
 * @param[in,out] PrivilegeSetLength
 * The total length size of a set of privileges. This length represents
 * the count of elements in the privilege set array.
 *
 * @param[in] ObjectTypeList
 * A pointer to a given object type list. If this parameter is not NULL
 * the function will perform an access check against the main object
 * and sub-objects of this list. If this parameter is NULL and
 * ObjectTypeListLength is 0, the function will perform a normal
 * access check instead.
 *
 * @param[in] ObjectTypeListLength
 * The length of the object type list array, pointed by ObjectTypeList.
 * This length in question represents the number of elements in such array.
 * This parameter must be 0 if no array list is provided.
 *
 * @param[in] UseResultList
 * If this parameter is set to TRUE, the function will return the GrantedAccess
 * and AccessStatus parameter as arrays of granted rights and status value for
 * each individual object element pointed by ObjectTypeList.
 *
 * @param[out] GrantedAccess
 * A pointer to granted access rights, returned to the caller. If ObjectTypeList
 * is not NULL this paramater is an array of granted access rights for the object
 * and each individual sub-object of the list.
 *
 * @param[out] AccessStatus
 * A pointer to a status code, returned to the caller. This status code
 * represents whether access is granted or denied to the client on the
 * target object. The difference between the status code of the function
 * is that code indicates whether the function has successfully completed
 * the access check operation. If ObjectTypeList is not NULL, this
 * parameter is an array of access status for the object and each individual
 * sub-object of the list.
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
 * size of the set of privileges. STATUS_INVALID_PARAMETER is returned if the caller
 * did not provide an object type list but the caller wanted to invoke an object type
 * result list access check, or if the list is out of order or the list is invalid.
 * A failure NTSTATUS code is returned otherwise.
 *
 * @remarks
 * The function performs an access check against the object type list,
 * if provided, depending on the UseResultList parameter. That is, if that
 * parameter was set to TRUE the function will either grant or deny access
 * to each individual sub-object and return an array of access status and
 * granted rights for the corresponding object type list. Otherwise the function
 * will grant or deny access to the object type list hierarchy as a whole.
 */
static
NTSTATUS
SepAccessCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ HANDLE ClientToken,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_bytes_(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ BOOLEAN UseResultList,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus)
{
    PSECURITY_DESCRIPTOR CapturedSecurityDescriptor = NULL;
    POBJECT_TYPE_LIST_INTERNAL CapturedObjectTypeList = NULL;
    PSID CapturedPrincipalSelfSid = NULL;
    SECURITY_SUBJECT_CONTEXT SubjectSecurityContext;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ACCESS_MASK PreviouslyGrantedAccess = 0;
    PPRIVILEGE_SET Privileges = NULL;
    ULONG CapturedPrivilegeSetLength, RequiredPrivilegeSetLength;
    ULONG ResultListIndex;
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

        /*
         * Probe the access and status list based on the way
         * we are going to fill data in.
         */
        if (UseResultList)
        {
            /* Bail out on an empty list */
            if (!ObjectTypeListLength)
            {
                DPRINT1("The object type list is empty\n");
                _SEH2_YIELD(return STATUS_INVALID_PARAMETER);
            }

            ProbeForWrite(GrantedAccess, sizeof(ACCESS_MASK) * ObjectTypeListLength, sizeof(ULONG));
            ProbeForWrite(AccessStatus, sizeof(NTSTATUS) * ObjectTypeListLength, sizeof(ULONG));
        }
        else
        {
            ProbeForWrite(GrantedAccess, sizeof(ACCESS_MASK), sizeof(ULONG));
            ProbeForWrite(AccessStatus, sizeof(NTSTATUS), sizeof(ULONG));
        }

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
    {
        DPRINT1("Some generic rights are not mapped\n");
        return STATUS_GENERIC_NOT_MAPPED;
    }

    /* Reference the token */
    Status = ObReferenceObjectByHandle(ClientToken,
                                       TOKEN_QUERY,
                                       SeTokenObjectType,
                                       PreviousMode,
                                       (PVOID*)&Token,
                                       NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to reference token (Status 0x%08lx)\n", Status);
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
        DPRINT1("Impersonation level < SecurityIdentification\n");
        ObDereferenceObject(Token);
        return STATUS_BAD_IMPERSONATION_LEVEL;
    }

    /* Capture the object type list, the list is probed by the function itself */
    Status = SeCaptureObjectTypeList(ObjectTypeList,
                                     ObjectTypeListLength,
                                     PreviousMode,
                                     &CapturedObjectTypeList);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to capture the object type list (Status 0x%08lx)\n", Status);
        ObDereferenceObject(Token);
        return Status;
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
        DPRINT1("SePrivilegePolicyCheck failed (Status 0x%08lx)\n", Status);
        SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);
        ObDereferenceObject(Token);

        /*
         * The caller does not have the required access to do an access check.
         * Propagate the access and status for the whole hierarchy of the list
         * or just to single target object.
         */
        if (UseResultList)
        {
            for (ResultListIndex = 0; ResultListIndex < ObjectTypeListLength; ResultListIndex++)
            {
                AccessStatus[ResultListIndex] = Status;
                GrantedAccess[ResultListIndex] = 0;
            }
        }
        else
        {
            *AccessStatus = Status;
            *GrantedAccess = 0;
        }

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
            SeFreePrivileges(Privileges);
            SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);
            ObDereferenceObject(Token);
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
            SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);
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
        DPRINT1("Failed to capture the Security Descriptor\n");
        SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);
        ObDereferenceObject(Token);
        return Status;
    }

    /* Check the captured security descriptor */
    if (CapturedSecurityDescriptor == NULL)
    {
        DPRINT1("Security Descriptor is NULL\n");
        SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);
        ObDereferenceObject(Token);
        return STATUS_INVALID_SECURITY_DESCR;
    }

    /* Check security descriptor for valid owner and group */
    if (SepGetOwnerFromDescriptor(CapturedSecurityDescriptor) == NULL ||
        SepGetGroupFromDescriptor(CapturedSecurityDescriptor) == NULL)
    {
        DPRINT1("Security Descriptor does not have a valid group or owner\n");
        SeReleaseSecurityDescriptor(CapturedSecurityDescriptor,
                                    PreviousMode,
                                    FALSE);
        SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);
        ObDereferenceObject(Token);
        return STATUS_INVALID_SECURITY_DESCR;
    }

    /* Capture the principal self SID if we have one */
    if (PrincipalSelfSid)
    {
        Status = SepCaptureSid(PrincipalSelfSid,
                               PreviousMode,
                               PagedPool,
                               TRUE,
                               &CapturedPrincipalSelfSid);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to capture the principal self SID (Status 0x%08lx)\n", Status);
            SeReleaseSecurityDescriptor(CapturedSecurityDescriptor,
                                        PreviousMode,
                                        FALSE);
            SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);
            ObDereferenceObject(Token);
            return Status;
        }
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
        /*
         * Propagate the access and status for the whole hierarchy
         * of the list or just to single target object.
         */
        if (UseResultList)
        {
            for (ResultListIndex = 0; ResultListIndex < ObjectTypeListLength; ResultListIndex++)
            {
                AccessStatus[ResultListIndex] = STATUS_SUCCESS;
                GrantedAccess[ResultListIndex] = PreviouslyGrantedAccess;
            }
        }
        else
        {
            *GrantedAccess = PreviouslyGrantedAccess;
            *AccessStatus = STATUS_SUCCESS;
        }
    }
    else
    {
        /* Now perform the access check */
        SepAccessCheckWorker(CapturedSecurityDescriptor,
                             Token,
                             &SubjectSecurityContext.PrimaryToken,
                             CapturedPrincipalSelfSid,
                             DesiredAccess,
                             CapturedObjectTypeList,
                             ObjectTypeListLength,
                             PreviouslyGrantedAccess,
                             GenericMapping,
                             PreviousMode,
                             UseResultList,
                             NULL,
                             GrantedAccess,
                             AccessStatus);
    }

    /* Release subject context and unlock the token */
    SeReleaseSubjectContext(&SubjectSecurityContext);
    SepReleaseTokenLock(Token);

    /* Release the caputed principal self SID */
    SepReleaseSid(CapturedPrincipalSelfSid,
                  PreviousMode,
                  TRUE);

    /* Release the captured security descriptor */
    SeReleaseSecurityDescriptor(CapturedSecurityDescriptor,
                                PreviousMode,
                                FALSE);

    /* Release the object type list */
    SeReleaseObjectTypeList(CapturedObjectTypeList, PreviousMode);

    /* Dereference the token */
    ObDereferenceObject(Token);

    /* Check succeeded */
    return STATUS_SUCCESS;
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
        ret = SepAccessCheckWorker(SecurityDescriptor,
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
 * Determines whether security access can be granted to a client
 * that requests such access on an object.
 *
 * @remarks
 * For more documentation details about the parameters and
 * overall function behavior, see SepAccessCheck.
 */
NTSTATUS
NTAPI
NtAccessCheck(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_bytes_(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus)
{
    PAGED_CODE();

    /* Invoke the internal function to do the job */
    return SepAccessCheck(SecurityDescriptor,
                          ClientToken,
                          NULL,
                          DesiredAccess,
                          GenericMapping,
                          PrivilegeSet,
                          PrivilegeSetLength,
                          NULL,
                          0,
                          FALSE,
                          GrantedAccess,
                          AccessStatus);
}

/**
 * @brief
 * Determines whether security access can be granted to a client
 * that requests such access on the object type list. The access
 * is either granted or denied for the whole object hierarchy
 * in the list.
 *
 * @remarks
 * For more documentation details about the parameters and
 * overall function behavior, see SepAccessCheck.
 */
NTSTATUS
NTAPI
NtAccessCheckByType(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_reads_opt_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_bytes_(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_ PACCESS_MASK GrantedAccess,
    _Out_ PNTSTATUS AccessStatus)
{
    PAGED_CODE();

    /* Invoke the internal function to do the job */
    return SepAccessCheck(SecurityDescriptor,
                          ClientToken,
                          PrincipalSelfSid,
                          DesiredAccess,
                          GenericMapping,
                          PrivilegeSet,
                          PrivilegeSetLength,
                          ObjectTypeList,
                          ObjectTypeListLength,
                          FALSE,
                          GrantedAccess,
                          AccessStatus);
}

/**
 * @brief
 * Determines whether security access can be granted to a client
 * that requests such access on the object type list. Unlike the
 * NtAccessCheckByType variant, this function will grant or deny
 * access to each individual object and sub-object in the list.
 *
 * @remarks
 * For more documentation details about the parameters and
 * overall function behavior, see SepAccessCheck.
 */
NTSTATUS
NTAPI
NtAccessCheckByTypeResultList(
    _In_ PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_opt_ PSID PrincipalSelfSid,
    _In_ HANDLE ClientToken,
    _In_ ACCESS_MASK DesiredAccess,
    _In_reads_(ObjectTypeListLength) POBJECT_TYPE_LIST ObjectTypeList,
    _In_ ULONG ObjectTypeListLength,
    _In_ PGENERIC_MAPPING GenericMapping,
    _Out_writes_bytes_(*PrivilegeSetLength) PPRIVILEGE_SET PrivilegeSet,
    _Inout_ PULONG PrivilegeSetLength,
    _Out_writes_(ObjectTypeListLength) PACCESS_MASK GrantedAccess,
    _Out_writes_(ObjectTypeListLength) PNTSTATUS AccessStatus)
{
    PAGED_CODE();

    /* Invoke the internal function to do the job */
    return SepAccessCheck(SecurityDescriptor,
                          ClientToken,
                          PrincipalSelfSid,
                          DesiredAccess,
                          GenericMapping,
                          PrivilegeSet,
                          PrivilegeSetLength,
                          ObjectTypeList,
                          ObjectTypeListLength,
                          TRUE,
                          GrantedAccess,
                          AccessStatus);
}

/* EOF */
