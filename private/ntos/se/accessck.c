/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Accessck.c

Abstract:

    This Module implements the access check procedures.  Both NtAccessCheck
    and SeAccessCheck check to is if a user (denoted by an input token) can
    be granted the desired access rights to object protected by a security
    descriptor and an optional object owner.  Both procedures use a common
    local procedure to do the test.

Author:

    Robert Reichel  (RobertRe)    11-30-90

Environment:

    Kernel Mode

Revision History:

    Richard Ward     (RichardW)     14-Apr-92   Changed ACE_HEADER
--*/

#include "tokenp.h"
#include <sertlp.h>


//
//  Define the local macros and procedure for this module
//

#if DBG

extern BOOLEAN SepDumpSD;
extern BOOLEAN SepDumpToken;
BOOLEAN SepShowAccessFail;

#endif // DBG



VOID
SepUpdateParentTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN ULONG StartIndex
    );

typedef enum {
    UpdateRemaining,
    UpdateCurrentGranted,
    UpdateCurrentDenied
} ACCESS_MASK_FIELD_TO_UPDATE;

VOID
SepAddAccessTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN ULONG StartIndex,
    IN ACCESS_MASK AccessMask,
    IN ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate
    );

NTSTATUS
SeAccessCheckByType (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    IN OUT PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus,
    IN BOOLEAN ReturnResultList
    );

VOID
SepMaximumAccessCheck(
    IN PTOKEN EToken,
    IN PTOKEN PrimaryToken,
    IN PACL Dacl,
    IN PSID PrincipalSelfSid,
    IN ULONG LocalTypeListLength,
    IN PIOBJECT_TYPE_LIST LocalTypeList,
    IN ULONG ObjectTypeListLength,
    IN BOOLEAN Restricted
    );

VOID
SepNormalAccessCheck(
    IN ACCESS_MASK Remaining,
    IN PTOKEN EToken,
    IN PTOKEN PrimaryToken,
    IN PACL Dacl,
    IN PSID PrincipalSelfSid,
    IN ULONG LocalTypeListLength,
    IN PIOBJECT_TYPE_LIST LocalTypeList,
    IN ULONG ObjectTypeListLength,
    IN BOOLEAN Restricted
    );

BOOLEAN
SepSidInTokenEx (
    IN PACCESS_TOKEN AToken,
    IN PSID PrincipalSelfSid,
    IN PSID Sid,
    IN BOOLEAN DenyAce,
    IN BOOLEAN Restricted
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,SeCaptureObjectTypeList)
#pragma alloc_text(PAGE,SeFreeCapturedObjectTypeList)
#pragma alloc_text(PAGE,SepUpdateParentTypeList)
#pragma alloc_text(PAGE,SepObjectInTypeList)
#pragma alloc_text(PAGE,SepAddAccessTypeList)
#pragma alloc_text(PAGE,SepSidInToken)
#pragma alloc_text(PAGE,SepSidInTokenEx)
#pragma alloc_text(PAGE,SepAccessCheck)
#pragma alloc_text(PAGE,NtAccessCheck)
#pragma alloc_text(PAGE,NtAccessCheckByType)
#pragma alloc_text(PAGE,SeAccessCheckByType)
#pragma alloc_text(PAGE,SeFreePrivileges)
#pragma alloc_text(PAGE,SeAccessCheck)
#pragma alloc_text(PAGE,SePrivilegePolicyCheck)
#pragma alloc_text(PAGE,SepTokenIsOwner)
#pragma alloc_text(PAGE,SeFastTraverseCheck)
#pragma alloc_text(PAGE,SepMaximumAccessCheck)
#pragma alloc_text(PAGE,SepNormalAccessCheck)
#endif


NTSTATUS
SeCaptureObjectTypeList (
    IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN KPROCESSOR_MODE RequestorMode,
    OUT PIOBJECT_TYPE_LIST *CapturedObjectTypeList
)
/*++

Routine Description:

    This routine probes and captures a copy of any object type list
    that might have been provided via the ObjectTypeList argument.

    The object type list is converted to the internal form that explicitly
    specifies the hierarchical relationship between the entries.

    The object typs list is validated to ensure a valid hierarchical
    relationship is represented.

Arguments:

    ObjectTypeList - The object type list from which the type list
        information is to be retrieved.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    RequestorMode - Indicates the processor mode by which the access
        is being requested.

    CapturedObjectTypeList - Receives the captured type list which
        must be freed using SeFreeCapturedObjectTypeList().

Return Value:

    STATUS_SUCCESS indicates no exceptions were encountered.

    Any access violations encountered will be returned.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG i;
    PIOBJECT_TYPE_LIST LocalTypeList = NULL;

    ULONG Levels[ACCESS_MAX_LEVEL+1];

    PAGED_CODE();

    //
    //  Set default return
    //

    *CapturedObjectTypeList = NULL;

    if (RequestorMode != UserMode) {
        return STATUS_NOT_IMPLEMENTED;
    }

    try {

        if ( ObjectTypeListLength == 0 ) {

            // Drop through

        } else if ( !ARGUMENT_PRESENT(ObjectTypeList) ) {

            Status = STATUS_INVALID_PARAMETER;

        } else {

            if ( !IsValidElementCount( ObjectTypeListLength, IOBJECT_TYPE_LIST ) )
            {
                Status = STATUS_INVALID_PARAMETER ;

                //
                // No more to do, get out of the try statement:
                //

                leave ;
            }

            ProbeForRead( ObjectTypeList,
                          sizeof(OBJECT_TYPE_LIST) * ObjectTypeListLength,
                          sizeof(ULONG)
                          );

            //
            // Allocate a buffer to copy into.
            //

            LocalTypeList = ExAllocatePoolWithTag( PagedPool, sizeof(IOBJECT_TYPE_LIST) * ObjectTypeListLength, 'tOeS' );

            if ( LocalTypeList == NULL ) {
                Status = STATUS_INSUFFICIENT_RESOURCES;

            //
            // Copy the callers structure to the local structure.
            //

            } else {
                GUID * CapturedObjectType;
                for ( i=0; i<ObjectTypeListLength; i++ ) {
                    USHORT CurrentLevel;

                    //
                    // Limit ourselves
                    //
                    CurrentLevel = ObjectTypeList[i].Level;
                    if ( CurrentLevel > ACCESS_MAX_LEVEL ) {
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }

                    //
                    // Copy data the caller passed in
                    //
                    LocalTypeList[i].Level = CurrentLevel;
                    LocalTypeList[i].Flags = 0;
                    CapturedObjectType = ObjectTypeList[i].ObjectType;
                    ProbeForRead(
                        CapturedObjectType,
                        sizeof(GUID),
                        sizeof(ULONG)
                        );
                    LocalTypeList[i].ObjectType = *CapturedObjectType;
                    LocalTypeList[i].Remaining = 0;
                    LocalTypeList[i].CurrentGranted = 0;
                    LocalTypeList[i].CurrentDenied = 0;

                    //
                    // Ensure that the level number is consistent with the
                    //  level number of the previous entry.
                    //

                    if ( i == 0 ) {
                        if ( CurrentLevel != 0 ) {
                            Status = STATUS_INVALID_PARAMETER;
                            break;
                        }

                    } else {

                        //
                        // The previous entry is either:
                        //  my immediate parent,
                        //  my sibling, or
                        //  the child (or grandchild, etc.) of my sibling.
                        //
                        if ( CurrentLevel > LocalTypeList[i-1].Level + 1 ) {
                            Status = STATUS_INVALID_PARAMETER;
                            break;
                        }

                        //
                        // Don't support two roots.
                        //
                        if ( CurrentLevel == 0 ) {
                            Status = STATUS_INVALID_PARAMETER;
                            break;
                        }

                    }

                    //
                    // If the above rules are maintained,
                    //  then my parent object is the last object seen that
                    //  has a level one less than mine.
                    //

                    if ( CurrentLevel == 0 ) {
                        LocalTypeList[i].ParentIndex = -1;
                    } else {
                        LocalTypeList[i].ParentIndex = Levels[CurrentLevel-1];
                    }

                    //
                    // Save this obect as the last object seen at this level.
                    //

                    Levels[CurrentLevel] = i;

                }

            }

        } // end_if

    } except(EXCEPTION_EXECUTE_HANDLER) {


        //
        // If we captured any proxy data, we need to free it now.
        //

        if ( LocalTypeList != NULL ) {
            ExFreePool( LocalTypeList );
            LocalTypeList = NULL;
        }

        Status = GetExceptionCode();
    } // end_try

    *CapturedObjectTypeList = LocalTypeList;
    return Status;
}


VOID
SeFreeCapturedObjectTypeList(
    IN PVOID ObjectTypeList
    )

/*++

Routine Description:

    This routine frees the data associated with a captured ObjectTypeList
    structure.

Arguments:

    ObjectTypeList - Points to a captured object type list structure.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    if ( ObjectTypeList != NULL ) {
        ExFreePool( ObjectTypeList );
    }

    return;
}




BOOLEAN
SepObjectInTypeList (
    IN GUID *ObjectType,
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    OUT PULONG ReturnedIndex
)
/*++

Routine Description:

    This routine searches an ObjectTypeList to determine if the specified
    object type is in the list.

Arguments:

    ObjectType - Object Type to search for.

    ObjectTypeList - The object type list to search.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    ReturnedIndex - Index to the element ObjectType was found in


Return Value:

    TRUE: ObjectType was found in list.
    FALSE: ObjectType was not found in list.

--*/

{
    ULONG Index;
    GUID *LocalObjectType;

    PAGED_CODE();

    ASSERT( sizeof(GUID) == sizeof(ULONG) * 4 );
    for ( Index=0; Index<ObjectTypeListLength; Index++ ) {

        LocalObjectType = &ObjectTypeList[Index].ObjectType;

        if  ( RtlpIsEqualGuid( ObjectType, LocalObjectType ) ) {
            *ReturnedIndex = Index;
            return TRUE;
        }
    }

    return FALSE;
}


VOID
SepUpdateParentTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN ULONG StartIndex
)
/*++

Routine Description:

    Update the Access fields of the parent object of the specified object.


        The "remaining" field of a parent object is the logical OR of
        the remaining field of all of its children.

        The CurrentGranted field of the parent is the collection of bits
        granted to every one of its children..

        The CurrentDenied fields of the parent is the logical OR of
        the bits denied to any of its children.

    This routine takes an index to one of the children and updates the
    remaining field of the parent (and grandparents recursively).

Arguments:

    ObjectTypeList - The object type list to update.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    StartIndex - Index to the "child" element whose parents are to be updated.

Return Value:

    None.


--*/

{
    ULONG Index;
    ULONG ParentIndex;
    ULONG Level;
    ACCESS_MASK NewRemaining = 0;
    ACCESS_MASK NewCurrentGranted = 0xFFFFFFFF;
    ACCESS_MASK NewCurrentDenied = 0;

    PAGED_CODE();

    //
    // If the target node is at the root,
    //  we're all done.
    //

    if ( ObjectTypeList[StartIndex].ParentIndex == -1 ) {
        return;
    }

    //
    // Get the index to the parent that needs updating and the level of
    // the siblings.
    //

    ParentIndex = ObjectTypeList[StartIndex].ParentIndex;
    Level = ObjectTypeList[StartIndex].Level;

    //
    // Loop through all the children.
    //

    for ( Index=ParentIndex+1; Index<ObjectTypeListLength; Index++ ) {

        //
        // By definition, the children of an object are all those entries
        // immediately following the target.  The list of children (or
        // grandchildren) stops as soon as we reach an entry the has the
        // same level as the target (a sibling) or lower than the target
        // (an uncle).
        //

        if ( ObjectTypeList[Index].Level <= ObjectTypeList[ParentIndex].Level ) {
            break;
        }

        //
        // Only handle direct children of the parent.
        //

        if ( ObjectTypeList[Index].Level != Level ) {
            continue;
        }

        //
        // Compute the new bits for the parent.
        //

        NewRemaining |= ObjectTypeList[Index].Remaining;
        NewCurrentGranted &= ObjectTypeList[Index].CurrentGranted;
        NewCurrentDenied |= ObjectTypeList[Index].CurrentDenied;

    }

    //
    // If we've not changed the access to the parent,
    //  we're done.
    //

    if ( NewRemaining == ObjectTypeList[ParentIndex].Remaining &&
         NewCurrentGranted == ObjectTypeList[ParentIndex].CurrentGranted &&
        NewCurrentDenied == ObjectTypeList[ParentIndex].CurrentDenied ) {
        return;
    }


    //
    // Change the parent.
    //

    ObjectTypeList[ParentIndex].Remaining = NewRemaining;
    ObjectTypeList[ParentIndex].CurrentGranted = NewCurrentGranted;
    ObjectTypeList[ParentIndex].CurrentDenied = NewCurrentDenied;

    //
    // Go update the grand parents.
    //

    SepUpdateParentTypeList( ObjectTypeList,
                             ObjectTypeListLength,
                             ParentIndex );
}


VOID
SepAddAccessTypeList (
    IN PIOBJECT_TYPE_LIST ObjectTypeList,
    IN ULONG ObjectTypeListLength,
    IN ULONG StartIndex,
    IN ACCESS_MASK AccessMask,
    IN ACCESS_MASK_FIELD_TO_UPDATE FieldToUpdate
)
/*++

Routine Description:

    This routine grants the specified AccessMask to all of the objects that
    are descendents of the object specified by StartIndex.

    The Access fields of the parent objects are also recomputed as needed.

    For example, if an ACE granting access to a Property Set is found,
        that access is granted to all the Properties in the Property Set.

Arguments:

    ObjectTypeList - The object type list to update.

    ObjectTypeListLength - Number of elements in ObjectTypeList

    StartIndex - Index to the target element to update.

    AccessMask - Mask of access to grant to the target element and
        all of its decendents

    FieldToUpdate - Indicate which fields to update in object type list

Return Value:

    None.

--*/

{
    ULONG Index;
    ACCESS_MASK OldRemaining;
    ACCESS_MASK OldCurrentGranted;
    ACCESS_MASK OldCurrentDenied;
    BOOLEAN AvoidParent = FALSE;

    PAGED_CODE();

    //
    // Update the requested field.
    //
    // Always handle the target entry.
    //
    // If we've not actually changed the bits,
    //  early out.
    //

    switch (FieldToUpdate ) {
    case UpdateRemaining:

        OldRemaining = ObjectTypeList[StartIndex].Remaining;
        ObjectTypeList[StartIndex].Remaining = OldRemaining & ~AccessMask;

        if ( OldRemaining == ObjectTypeList[StartIndex].Remaining ) {
            return;
        }
        break;

    case UpdateCurrentGranted:

        OldCurrentGranted = ObjectTypeList[StartIndex].CurrentGranted;
        ObjectTypeList[StartIndex].CurrentGranted |=
            AccessMask & ~ObjectTypeList[StartIndex].CurrentDenied;

        if ( OldCurrentGranted == ObjectTypeList[StartIndex].CurrentGranted ) {
            //
            // We can't simply return here.
            // We have to visit our children.  Consider the case where there
            // was a previous deny ACE on a child.  That deny would have
            // propagated up the tree to this entry.  However, this allow ACE
            // needs to be added all of the children that haven't been
            // explictly denied.
            //
            AvoidParent = TRUE;
        }
        break;

    case UpdateCurrentDenied:

        OldCurrentDenied = ObjectTypeList[StartIndex].CurrentDenied;
        ObjectTypeList[StartIndex].CurrentDenied |=
            AccessMask & ~ObjectTypeList[StartIndex].CurrentGranted;

        if ( OldCurrentDenied == ObjectTypeList[StartIndex].CurrentDenied ) {
            return;
        }
        break;

    default:
        return;
    }


    //
    // Go update parent of the target.
    //

    if ( !AvoidParent ) {
        SepUpdateParentTypeList( ObjectTypeList,
                                 ObjectTypeListLength,
                                 StartIndex );
    }

    //
    // Loop handling all children of the target.
    //

    for ( Index=StartIndex+1; Index<ObjectTypeListLength; Index++ ) {

        //
        // By definition, the children of an object are all those entries
        // immediately following the target.  The list of children (or
        // grandchildren) stops as soon as we reach an entry the has the
        // same level as the target (a sibling) or lower than the target
        // (an uncle).
        //

        if ( ObjectTypeList[Index].Level <= ObjectTypeList[StartIndex].Level ) {
            break;
        }

        //
        // Grant access to the children
        //

        switch (FieldToUpdate) {
        case UpdateRemaining:

            ObjectTypeList[Index].Remaining &= ~AccessMask;
            break;

        case UpdateCurrentGranted:

            ObjectTypeList[Index].CurrentGranted |=
                AccessMask & ~ObjectTypeList[Index].CurrentDenied;
            break;

        case UpdateCurrentDenied:

            ObjectTypeList[Index].CurrentDenied |=
                AccessMask & ~ObjectTypeList[Index].CurrentGranted;
            break;

        default:
            return;
        }
    }
}

BOOLEAN
SepSidInToken (
    IN PACCESS_TOKEN AToken,
    IN PSID PrincipalSelfSid,
    IN PSID Sid,
    IN BOOLEAN DenyAce
    )

/*++

Routine Description:

    Checks to see if a given SID is in the given token.

    N.B. The code to compute the length of a SID and test for equality
         is duplicated from the security runtime since this is such a
         frequently used routine.

Arguments:

    Token - Pointer to the token to be examined

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.


    Sid - Pointer to the SID of interest

Return Value:

    A value of TRUE indicates that the SID is in the token, FALSE
    otherwise.

--*/

{

    ULONG i;
    PISID MatchSid;
    ULONG SidLength;
    PTOKEN Token;
    PSID_AND_ATTRIBUTES TokenSid;
    ULONG UserAndGroupCount;

    PAGED_CODE();

#if DBG

    SepDumpTokenInfo(AToken);

#endif

    //
    // If Sid is the constant PrincipalSelfSid,
    //  replace it with the passed in PrincipalSelfSid.
    //

    if ( PrincipalSelfSid != NULL &&
         RtlEqualSid( SePrincipalSelfSid, Sid ) ) {
        Sid = PrincipalSelfSid;
    }

    //
    // Get the length of the source SID since this only needs to be computed
    // once.
    //

    SidLength = 8 + (4 * ((PISID)Sid)->SubAuthorityCount);

    //
    // Get address of user/group array and number of user/groups.
    //

    Token = (PTOKEN)AToken;
    TokenSid = Token->UserAndGroups;
    UserAndGroupCount = Token->UserAndGroupCount;

    //
    // Scan through the user/groups and attempt to find a match with the
    // specified SID.
    //

    for (i = 0 ; i < UserAndGroupCount ; i += 1) {
        MatchSid = (PISID)TokenSid->Sid;

        //
        // If the SID revision and length matches, then compare the SIDs
        // for equality.
        //

        if ((((PISID)Sid)->Revision == MatchSid->Revision) &&
            (SidLength == (8 + (4 * (ULONG)MatchSid->SubAuthorityCount)))) {
            if (RtlEqualMemory(Sid, MatchSid, SidLength)) {

                //
                // If this is the first one in the list, then it is the User,
                // and return success immediately.
                //
                // If this is not the first one, then it represents a group,
                // and we must make sure the group is currently enabled before
                // we can say that the group is "in" the token.
                //

                if ((i == 0) || (TokenSid->Attributes & SE_GROUP_ENABLED) ||
                    (DenyAce && (TokenSid->Attributes & SE_GROUP_USE_FOR_DENY_ONLY))) {
                    return TRUE;

                } else {
                    return FALSE;
                }
            }
        }

        TokenSid += 1;
    }

    return FALSE;
}

BOOLEAN
SepSidInTokenEx (
    IN PACCESS_TOKEN AToken,
    IN PSID PrincipalSelfSid,
    IN PSID Sid,
    IN BOOLEAN DenyAce,
    IN BOOLEAN Restricted
    )

/*++

Routine Description:

    Checks to see if a given restricted SID is in the given token.

    N.B. The code to compute the length of a SID and test for equality
         is duplicated from the security runtime since this is such a
         frequently used routine.

Arguments:

    Token - Pointer to the token to be examined

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.


    Sid - Pointer to the SID of interest

    DenyAce - The ACE being evaluated is a DENY or ACCESS DENIED ace

    Restricted - The access check being performed uses the restricted sids.

Return Value:

    A value of TRUE indicates that the SID is in the token, FALSE
    otherwise.

--*/

{

    ULONG i;
    PISID MatchSid;
    ULONG SidLength;
    PTOKEN Token;
    PSID_AND_ATTRIBUTES TokenSid;
    ULONG UserAndGroupCount;

    PAGED_CODE();

#if DBG

    SepDumpTokenInfo(AToken);

#endif

    //
    // If Sid is the constant PrincipalSelfSid,
    //  replace it with the passed in PrincipalSelfSid.
    //

    if ( PrincipalSelfSid != NULL &&
         RtlEqualSid( SePrincipalSelfSid, Sid ) ) {
        Sid = PrincipalSelfSid;
    }

    //
    // Get the length of the source SID since this only needs to be computed
    // once.
    //

    SidLength = 8 + (4 * ((PISID)Sid)->SubAuthorityCount);

    //
    // Get address of user/group array and number of user/groups.
    //

    Token = (PTOKEN)AToken;
    if (Restricted) {
        TokenSid = Token->RestrictedSids;
        UserAndGroupCount = Token->RestrictedSidCount;
    } else {
        TokenSid = Token->UserAndGroups;
        UserAndGroupCount = Token->UserAndGroupCount;
    }

    //
    // Scan through the user/groups and attempt to find a match with the
    // specified SID.
    //

    for (i = 0; i < UserAndGroupCount ; i += 1) {
        MatchSid = (PISID)TokenSid->Sid;

        //
        // If the SID revision and length matches, then compare the SIDs
        // for equality.
        //

        if ((((PISID)Sid)->Revision == MatchSid->Revision) &&
            (SidLength == (8 + (4 * (ULONG)MatchSid->SubAuthorityCount)))) {
            if (RtlEqualMemory(Sid, MatchSid, SidLength)) {

                //
                // If this is the first one in the list and not deny-only it
                // is not a restricted token then it is the User, and return
                // success immediately.
                //
                // If this is not the first one, then it represents a group,
                // and we must make sure the group is currently enabled before
                // we can say that the group is "in" the token.
                //

                if ((!Restricted && (i == 0) && ((TokenSid->Attributes & SE_GROUP_USE_FOR_DENY_ONLY) == 0)) ||
                    (TokenSid->Attributes & SE_GROUP_ENABLED) ||
                    (DenyAce && (TokenSid->Attributes & SE_GROUP_USE_FOR_DENY_ONLY))) {
                    return TRUE;

                } else {
                    return FALSE;
                }

            }
        }

        TokenSid += 1;
    }

    return FALSE;
}

VOID
SepMaximumAccessCheck(
    IN PTOKEN EToken,
    IN PTOKEN PrimaryToken,
    IN PACL Dacl,
    IN PSID PrincipalSelfSid,
    IN ULONG LocalTypeListLength,
    IN PIOBJECT_TYPE_LIST LocalTypeList,
    IN ULONG ObjectTypeListLength,
    IN BOOLEAN Restricted
    )
/*++

Routine Description:

    Does an access check for maximum allowed or with a result list. If the
    Restricted flag is set, it is done for a restricted token. The sids
    checked are the restricted sids, not the users and groups. The current
    granted access is stored in the Remaining access and then another access
    check is run.

Arguments:

    EToken - Effective token of caller.

    PrimaryToken - Process token of calling process

    Dacl - ACL to check

    PrincipalSelfSid - Sid to use in replacing the well-known self sid

    LocalTypeListLength - Length of list of types.

    LocalTypeList - List of types.

    ObjectTypeList - Length of caller-supplied list of object types.

Return Value:

    none

--*/

{
    ULONG i,j;
    PVOID Ace;
    ULONG AceCount;
    ULONG Index;
    ULONG ResultListIndex;

    //
    // The remaining bits are the granted bits for each object type on a
    // restricted check
    //

    if ( Restricted ) {
        for ( j=0; j<LocalTypeListLength; j++ ) {
            LocalTypeList[j].Remaining = LocalTypeList[j].CurrentGranted;
            LocalTypeList[j].CurrentGranted = 0;
        }
    }


    AceCount = Dacl->AceCount;

    //
    // granted == NUL
    // denied == NUL
    //
    //  for each ACE
    //
    //      if grant
    //          for each SID
    //              if SID match, then add all that is not denied to grant mask
    //
    //      if deny
    //          for each SID
    //              if SID match, then add all that is not granted to deny mask
    //

    for ( i = 0, Ace = FirstAce( Dacl ) ;
          i < AceCount  ;
          i++, Ace = NextAce( Ace )
        ) {

        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) {

            if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_ACE_TYPE) ) {

                if (SepSidInTokenEx( EToken, PrincipalSelfSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart, FALSE, Restricted )) {

                    //
                    // Only grant access types from this mask that have
                    // not already been denied
                    //

                    // Optimize 'normal' case
                    if ( LocalTypeListLength == 1 ) {
                        LocalTypeList->CurrentGranted |=
                           (((PACCESS_ALLOWED_ACE)Ace)->Mask & ~LocalTypeList->CurrentDenied);
                    } else {
                       //
                       // The zeroeth object type represents the object itself.
                       //
                       SepAddAccessTypeList(
                            LocalTypeList,          // List to modify
                            LocalTypeListLength,    // Length of list
                            0,                      // Element to update
                            ((PACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                            UpdateCurrentGranted );
                    }
                 }


             //
             // Handle an object specific Access Allowed ACE
             //
             } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) ) {
                 GUID *ObjectTypeInAce;

                 //
                 // If no object type is in the ACE,
                 //  treat this as an ACCESS_ALLOWED_ACE.
                 //

                 ObjectTypeInAce = RtlObjectAceObjectType(Ace);

                 if ( ObjectTypeInAce == NULL ) {

                     if ( SepSidInTokenEx( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), FALSE, Restricted ) ) {

                         // Optimize 'normal' case
                         if ( LocalTypeListLength == 1 ) {
                             LocalTypeList->CurrentGranted |=
                                (((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask & ~LocalTypeList->CurrentDenied);
                         } else {
                             SepAddAccessTypeList(
                                 LocalTypeList,          // List to modify
                                 LocalTypeListLength,    // Length of list
                                 0,                      // Element to update
                                 ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                 UpdateCurrentGranted );
                         }
                     }

                 //
                 // If no object type list was passed,
                 //  don't grant access to anyone.
                 //

                 } else if ( ObjectTypeListLength == 0 ) {

                     // Drop through


                //
                // If an object type is in the ACE,
                //   Find it in the LocalTypeList before using the ACE.
                //
                } else {

                     if ( SepSidInTokenEx( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), FALSE, Restricted ) ) {

                         if ( SepObjectInTypeList( ObjectTypeInAce,
                                                   LocalTypeList,
                                                   LocalTypeListLength,
                                                   &Index ) ) {
                             SepAddAccessTypeList(
                                  LocalTypeList,          // List to modify
                                  LocalTypeListLength,   // Length of list
                                  Index,                  // Element already updated
                                  ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                  UpdateCurrentGranted );
                         }
                     }
                }

             } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE) ) {

                 //
                 //  If we're impersonating, EToken is set to the Client, and if we're not,
                 //  EToken is set to the Primary.  According to the DSA architecture, if
                 //  we're asked to evaluate a compound ACE and we're not impersonating,
                 //  pretend we are impersonating ourselves.  So we can just use the EToken
                 //  for the client token, since it's already set to the right thing.
                 //


                 if ( SepSidInTokenEx(EToken, PrincipalSelfSid, RtlCompoundAceClientSid( Ace ), FALSE, Restricted) &&
                      SepSidInTokenEx(PrimaryToken, NULL, RtlCompoundAceServerSid( Ace ), FALSE, FALSE)
                    ) {

                     //
                     // Only grant access types from this mask that have
                     // not already been denied
                     //

                     // Optimize 'normal' case
                     if ( LocalTypeListLength == 1 ) {
                         LocalTypeList->CurrentGranted |=
                            (((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask & ~LocalTypeList->CurrentDenied);
                     } else {
                        //
                        // The zeroeth object type represents the object itself.
                        //
                        SepAddAccessTypeList(
                             LocalTypeList,          // List to modify
                             LocalTypeListLength,    // Length of list
                             0,                      // Element to update
                             ((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                             UpdateCurrentGranted );
                     }

                 }


             } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_DENIED_ACE_TYPE) ) {

                 if ( SepSidInTokenEx( EToken, PrincipalSelfSid, &((PACCESS_DENIED_ACE)Ace)->SidStart, TRUE, Restricted )) {

                      //
                      // Only deny access types from this mask that have
                      // not already been granted
                      //

                     // Optimize 'normal' case
                     if ( LocalTypeListLength == 1 ) {
                         LocalTypeList->CurrentDenied |=
                             (((PACCESS_DENIED_ACE)Ace)->Mask & ~LocalTypeList->CurrentGranted);
                     } else {
                         //
                         // The zeroeth object type represents the object itself.
                         //
                         SepAddAccessTypeList(
                             LocalTypeList,          // List to modify
                             LocalTypeListLength,    // Length of list
                             0,                      // Element to update
                             ((PACCESS_DENIED_ACE)Ace)->Mask, // Access denied
                             UpdateCurrentDenied );
                    }
                 }


             //
             // Handle an object specific Access Denied ACE
             //
             } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_DENIED_OBJECT_ACE_TYPE) ) {

                 if ( SepSidInTokenEx( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), TRUE, Restricted ) ) {
                     GUID *ObjectTypeInAce;

                     //
                     // If there is no object type in the ACE,
                     //  or if the caller didn't specify an object type list,
                     //  apply this deny ACE to the entire object.
                     //

                     ObjectTypeInAce = RtlObjectAceObjectType(Ace);

                     if ( ObjectTypeInAce == NULL ) {

                         if ( LocalTypeListLength == 1 ) {
                             LocalTypeList->CurrentDenied |=
                                 (((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask & ~LocalTypeList->CurrentGranted);
                         } else {

                             //
                             // The zeroeth object type represents the object itself.
                             //

                             SepAddAccessTypeList(
                                 LocalTypeList,          // List to modify
                                 LocalTypeListLength,    // Length of list
                                 0,
                                 ((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask, // Access denied
                                 UpdateCurrentDenied );
                         }
                     //
                     // If no object type list was passed,
                     //  don't grant access to anyone.
                     //

                     } else if ( ObjectTypeListLength == 0 ) {

                         LocalTypeList->CurrentDenied |=
                             (((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask & ~LocalTypeList->CurrentGranted);


                     //
                     // If an object type is in the ACE,
                     //   Find it in the LocalTypeList before using the ACE.
                     //

                     } else if ( SepObjectInTypeList( ObjectTypeInAce,
                                                          LocalTypeList,
                                                          LocalTypeListLength,
                                                          &Index ) ) {

                            SepAddAccessTypeList(
                                LocalTypeList,          // List to modify
                                LocalTypeListLength,    // Length of list
                                Index,                  // Element to update
                                ((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask, // Access denied
                                UpdateCurrentDenied );

                    }
                }
            }
        }
    }
}

VOID
SepNormalAccessCheck(
    IN ACCESS_MASK Remaining,
    IN PTOKEN EToken,
    IN PTOKEN PrimaryToken,
    IN PACL Dacl,
    IN PSID PrincipalSelfSid,
    IN ULONG LocalTypeListLength,
    IN PIOBJECT_TYPE_LIST LocalTypeList,
    IN ULONG ObjectTypeListLength,
    IN BOOLEAN Restricted
    )
/*++

Routine Description:

    Does an access check when the caller isn't asking for MAXIMUM_ALLOWED or
    a type result list. If the Restricted flag is set, the sids checked are
    the restricted sids, not the users and groups. The Remaining field is
    reset to the original remaining value and then another access check is run.

Arguments:

    Remaining - Remaining access desired after special checks

    EToken - Effective token of caller.

    PrimaryToken - Process token of calling process

    Dacl - ACL to check

    PrincipalSelfSid - Sid to use in replacing the well-known self sid

    LocalTypeListLength - Length of list of types.

    LocalTypeList - List of types.

    ObjectTypeList - Length of caller-supplied list of object types.

    Restricted - Use the restricted sids for the access check.

Return Value:

    none

--*/
{
    ULONG i,j;
    PVOID Ace;
    ULONG AceCount;
    ULONG Index;

    AceCount = Dacl->AceCount;

    //
    // The remaining bits are "remaining" at all levels
    //

    for ( j=0; j<LocalTypeListLength; j++ ) {
        LocalTypeList[j].Remaining = Remaining;
    }

    //
    // Process the DACL handling individual access bits.
    //

    for ( i = 0, Ace = FirstAce( Dacl ) ;
          ( i < AceCount ) && ( LocalTypeList->Remaining != 0 )  ;
          i++, Ace = NextAce( Ace ) ) {

        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) {

            //
            // Handle an Access Allowed ACE
            //

            if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_ACE_TYPE) ) {

               if ( SepSidInTokenEx( EToken, PrincipalSelfSid, &((PACCESS_ALLOWED_ACE   )Ace)->SidStart, FALSE, Restricted ) ) {

                   // Optimize 'normal' case
                   if ( LocalTypeListLength == 1 ) {
                       LocalTypeList->Remaining &= ~((PACCESS_ALLOWED_ACE)Ace)->Mask;
                   } else {
                       //
                       // The zeroeth object type represents the object itself.
                       //
                       SepAddAccessTypeList(
                            LocalTypeList,          // List to modify
                            LocalTypeListLength,    // Length of list
                            0,                      // Element to update
                            ((PACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                            UpdateRemaining );
                   }

               }


            //
            // Handle an object specific Access Allowed ACE
            //
            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) ) {
                GUID *ObjectTypeInAce;

                //
                // If no object type is in the ACE,
                //  treat this as an ACCESS_ALLOWED_ACE.
                //

                ObjectTypeInAce = RtlObjectAceObjectType(Ace);

                if ( ObjectTypeInAce == NULL ) {

                    if ( SepSidInTokenEx( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), FALSE, Restricted ) ) {

                       // Optimize 'normal' case
                       if ( LocalTypeListLength == 1 ) {
                           LocalTypeList->Remaining &= ~((PACCESS_ALLOWED_ACE)Ace)->Mask;
                       } else {
                           SepAddAccessTypeList(
                                LocalTypeList,          // List to modify
                                LocalTypeListLength,    // Length of list
                                0,                      // Element to update
                                ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                UpdateRemaining );
                       }
                    }

                //
                // If no object type list was passed,
                //  don't grant access to anyone.
                //

                } else if ( ObjectTypeListLength == 0 ) {

                    // Drop through


               //
               // If an object type is in the ACE,
               //   Find it in the LocalTypeList before using the ACE.
               //
               } else {

                    if ( SepSidInTokenEx( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), FALSE, Restricted ) ) {

                        if ( SepObjectInTypeList( ObjectTypeInAce,
                                                  LocalTypeList,
                                                  LocalTypeListLength,
                                                  &Index ) ) {
                            SepAddAccessTypeList(
                                 LocalTypeList,          // List to modify
                                 LocalTypeListLength,   // Length of list
                                 Index,                  // Element already updated
                                 ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                 UpdateRemaining );
                        }
                    }
               }


            //
            // Handle a compound Access Allowed ACE
            //

            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE) ) {

                //
                // See comment in MAXIMUM_ALLOWED case as to why we can use EToken here
                // for the client.
                //

                if ( SepSidInTokenEx(EToken, PrincipalSelfSid, RtlCompoundAceClientSid( Ace ), FALSE, Restricted) &&
                     SepSidInTokenEx(PrimaryToken, NULL, RtlCompoundAceServerSid( Ace ), FALSE, Restricted) ) {

                    // Optimize 'normal' case
                    if ( LocalTypeListLength == 1 ) {
                        LocalTypeList->Remaining &= ~((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask;
                    } else {
                        SepAddAccessTypeList(
                             LocalTypeList,          // List to modify
                             LocalTypeListLength,    // Length of list
                             0,                      // Element to update
                             ((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                             UpdateRemaining );
                    }
                }



            //
            // Handle an Access Denied ACE
            //

            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_DENIED_ACE_TYPE) ) {

                if ( SepSidInTokenEx( EToken, PrincipalSelfSid, &((PACCESS_DENIED_ACE)Ace)->SidStart, TRUE, Restricted ) ) {

                    //
                    // The zeroeth element represents the object itself.
                    //  Just check that element.
                    //
                    if (LocalTypeList->Remaining & ((PACCESS_DENIED_ACE)Ace)->Mask) {

                        break;
                    }
                }


            //
            // Handle an object specific Access Denied ACE
            //
            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_DENIED_OBJECT_ACE_TYPE) ) {

                if ( SepSidInTokenEx( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), TRUE, Restricted ) ) {
                    GUID *ObjectTypeInAce;

                    //
                    // If there is no object type in the ACE,
                    //  or if the caller didn't specify an object type list,
                    //  apply this deny ACE to the entire object.
                    //

                    ObjectTypeInAce = RtlObjectAceObjectType(Ace);
                    if ( ObjectTypeInAce == NULL ||
                         ObjectTypeListLength == 0 ) {

                        //
                        // The zeroeth element represents the object itself.
                        //  Just check that element.
                        //
                        if (LocalTypeList->Remaining & ((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask) {
                            break;
                        }

                    //
                    // Otherwise apply the deny ACE to the object specified
                    //  in the ACE.
                    //

                    } else if ( SepObjectInTypeList( ObjectTypeInAce,
                                                  LocalTypeList,
                                                  LocalTypeListLength,
                                                  &Index ) ) {

                        if (LocalTypeList[Index].Remaining & ((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask) {
                            break;
                        }

                    }
               }
            }

        }
    }


}


VOID
SepAccessCheck (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN PTOKEN PrimaryToken,
    IN PTOKEN ClientToken OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN PIOBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    IN KPROCESSOR_MODE PreviousMode,
    OUT PACCESS_MASK GrantedAccess,
    OUT PPRIVILEGE_SET *Privileges OPTIONAL,
    OUT PNTSTATUS AccessStatus,
    IN BOOLEAN ReturnResultList,
    OUT PBOOLEAN ReturnSomeAccessGranted,
    OUT PBOOLEAN ReturnSomeAccessDenied
    )

/*++

Routine Description:

    Worker routine for SeAccessCheck and NtAccessCheck.  We actually do the
    access checking here.

    Whether or not we actually evaluate the DACL is based on the following
    interaction between the SE_DACL_PRESENT bit in the security descriptor
    and the value of the DACL pointer itself.


                          SE_DACL_PRESENT

                        SET          CLEAR

                   +-------------+-------------+
                   |             |             |
             NULL  |    GRANT    |    GRANT    |
                   |     ALL     |     ALL     |
     DACL          |             |             |
     Pointer       +-------------+-------------+
                   |             |             |
            !NULL  |  EVALUATE   |    GRANT    |
                   |    ACL      |     ALL     |
                   |             |             |
                   +-------------+-------------+

Arguments:

    SecurityDescriptor - Pointer to the security descriptor from the object
        being accessed.

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    Token - Pointer to user's token object.

    TokenLocked - Boolean describing whether or not there is a read lock
        on the token.

    DesiredAccess - Access mask describing the user's desired access to the
        object.  This mask is assumed not to contain generic access types.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies a pointer to the generic mapping associated
        with this object type.

    PreviouslyGrantedAccess - Access mask indicating any access' that have
        already been granted by higher level routines

    PrivilgedAccessMask - Mask describing access types that may not be
        granted without a privilege.

    GrantedAccess - Returns an access mask describing all granted access',
        or NULL.

    Privileges - Optionally supplies a pointer in which will be returned
        any privileges that were used for the access.  If this is null,
        it will be assumed that privilege checks have been done already.

    AccessStatus - Returns STATUS_SUCCESS or other error code to be
        propogated back to the caller

    ReturnResultList - If true, GrantedAccess and AccessStatus is actually
        an array of entries ObjectTypeListLength elements long.

    ReturnSomeAccessGranted - Returns a value of TRUE to indicate that some access'
        were granted, FALSE otherwise.

    ReturnSomeAccessDenied - Returns a value of FALSE if some of the requested
        access was not granted.  This will alway be an inverse of SomeAccessGranted
        unless ReturnResultList is TRUE.  In that case,

Return Value:

    A value of TRUE indicates that some access' were granted, FALSE
    otherwise.

--*/
{
    NTSTATUS Status;
    ACCESS_MASK Remaining;


    PACL Dacl;

    PVOID Ace;
    ULONG AceCount;

    ULONG i;
    ULONG j;
    ULONG Index;
    ULONG PrivilegeCount = 0;
    BOOLEAN Success = FALSE;
    BOOLEAN SystemSecurity = FALSE;
    BOOLEAN WriteOwner = FALSE;
    PTOKEN EToken;

    IOBJECT_TYPE_LIST FixedTypeList;
    PIOBJECT_TYPE_LIST LocalTypeList;
    ULONG LocalTypeListLength;
    ULONG ResultListIndex;

    PAGED_CODE();

#if DBG

    SepDumpSecurityDescriptor(
        SecurityDescriptor,
        "Input to SeAccessCheck\n"
        );

    if (ARGUMENT_PRESENT( ClientToken )) {
        SepDumpTokenInfo( ClientToken );
    }

    SepDumpTokenInfo( PrimaryToken );

#endif


    EToken = ARGUMENT_PRESENT( ClientToken ) ? ClientToken : PrimaryToken;

    //
    // Assert that there are no generic accesses in the DesiredAccess
    //

    SeAssertMappedCanonicalAccess( DesiredAccess );

    Remaining = DesiredAccess;


    //
    // Check for ACCESS_SYSTEM_SECURITY here,
    // fail if he's asking for it and doesn't have
    // the privilege.
    //

    if ( Remaining & ACCESS_SYSTEM_SECURITY ) {

        //
        // Bugcheck if we weren't given a pointer to return privileges
        // into.  Our caller was supposed to have taken care of this
        // in that case.
        //

        ASSERT( ARGUMENT_PRESENT( Privileges ));

        Success = SepSinglePrivilegeCheck (
                    SeSecurityPrivilege,
                    EToken,
                    PreviousMode
                    );

        if (!Success) {
            PreviouslyGrantedAccess = 0;
            Status = STATUS_PRIVILEGE_NOT_HELD;
            goto ReturnOneStatus;
        }

        //
        // Success, remove ACCESS_SYSTEM_SECURITY from remaining, add it
        // to PreviouslyGrantedAccess
        //

        Remaining &= ~ACCESS_SYSTEM_SECURITY;
        PreviouslyGrantedAccess |= ACCESS_SYSTEM_SECURITY;

        PrivilegeCount++;
        SystemSecurity = TRUE;

        if ( Remaining == 0 ) {
            Status = STATUS_SUCCESS;
            goto ReturnOneStatus;
        }

    }


    //
    // Get pointer to client SID's
    //

    Dacl = RtlpDaclAddrSecurityDescriptor( (PISECURITY_DESCRIPTOR)SecurityDescriptor );

    //
    //  If the SE_DACL_PRESENT bit is not set, the object has no
    //  security, so all accesses are granted.  If he's asking for
    //  MAXIMUM_ALLOWED, return the GENERIC_ALL field from the generic
    //  mapping.
    //
    //  Also grant all access if the Dacl is NULL.
    //

    if ( !RtlpAreControlBitsSet(
             (PISECURITY_DESCRIPTOR)SecurityDescriptor,
             SE_DACL_PRESENT) || (Dacl == NULL)) {


        //
        // Restricted tokens treat a NULL dacl the same as a DACL with no
        // ACEs.
        //
#ifdef SECURE_NULL_DACLS
        if (SeTokenIsRestricted( EToken )) {
            //
            // We know that Remaining != 0 here, because we
            // know it was non-zero coming into this routine,
            // and we've checked it against 0 every time we've
            // cleared a bit.
            //

            ASSERT( Remaining != 0 );

            //
            // There are ungranted accesses.  Since there is
            // nothing in the DACL, they will not be granted.
            // If, however, the only ungranted access at this
            // point is MAXIMUM_ALLOWED, and something has been
            // granted in the PreviouslyGranted mask, return
            // what has been granted.
            //

            if ( (Remaining == MAXIMUM_ALLOWED) && (PreviouslyGrantedAccess != (ACCESS_MASK)0) ) {
                Status = STATUS_SUCCESS;
                goto ReturnOneStatus;

            } else {
                PreviouslyGrantedAccess = 0;
                Status = STATUS_ACCESS_DENIED;
                goto ReturnOneStatus;
            }
        }
#endif //SECURE_NULL_DACLS
        if (DesiredAccess & MAXIMUM_ALLOWED) {

            //
            // Give him:
            //   GenericAll
            //   Anything else he asked for
            //

            PreviouslyGrantedAccess =
                GenericMapping->GenericAll |
                (DesiredAccess | PreviouslyGrantedAccess) & ~MAXIMUM_ALLOWED;

        } else {

            PreviouslyGrantedAccess |= DesiredAccess;
        }

        Status = STATUS_SUCCESS;
        goto ReturnOneStatus;
    }

    //
    // There is security on this object.  Check to see
    // if he's asking for WRITE_OWNER, and perform the
    // privilege check if so.
    //

    if ( (Remaining & WRITE_OWNER) && ARGUMENT_PRESENT( Privileges ) ) {

        Success = SepSinglePrivilegeCheck (
                    SeTakeOwnershipPrivilege,
                    EToken,
                    PreviousMode
                    );

        if (Success) {

            //
            // Success, remove WRITE_OWNER from remaining, add it
            // to PreviouslyGrantedAccess
            //

            Remaining &= ~WRITE_OWNER;
            PreviouslyGrantedAccess |= WRITE_OWNER;

            PrivilegeCount++;
            WriteOwner = TRUE;

            if ( Remaining == 0 ) {
                Status = STATUS_SUCCESS;
                goto ReturnOneStatus;
            }
        }
    }


    //
    // If the DACL is empty,
    // deny all access immediately.
    //

    if ((AceCount = Dacl->AceCount) == 0) {

        //
        // We know that Remaining != 0 here, because we
        // know it was non-zero coming into this routine,
        // and we've checked it against 0 every time we've
        // cleared a bit.
        //

        ASSERT( Remaining != 0 );

        //
        // There are ungranted accesses.  Since there is
        // nothing in the DACL, they will not be granted.
        // If, however, the only ungranted access at this
        // point is MAXIMUM_ALLOWED, and something has been
        // granted in the PreviouslyGranted mask, return
        // what has been granted.
        //

        if ( (Remaining == MAXIMUM_ALLOWED) && (PreviouslyGrantedAccess != (ACCESS_MASK)0) ) {
            Status = STATUS_SUCCESS;
            goto ReturnOneStatus;

        } else {
            PreviouslyGrantedAccess = 0;
            Status = STATUS_ACCESS_DENIED;
            goto ReturnOneStatus;
        }
    }

    //
    // Fake out a top level ObjectType list if none is passed by the caller.
    //

    if ( ObjectTypeListLength == 0 ) {
        LocalTypeList = &FixedTypeList;
        LocalTypeListLength = 1;
        RtlZeroMemory( &FixedTypeList, sizeof(FixedTypeList) );
        FixedTypeList.ParentIndex = -1;
    } else {
        LocalTypeList = ObjectTypeList;
        LocalTypeListLength = ObjectTypeListLength;
    }

    //
    // If the caller wants the MAXIMUM_ALLOWED or the caller wants the
    //  results on all objects and subobjects, use a slower algorithm
    //  that traverses all the ACEs.
    //

    if ( (DesiredAccess & MAXIMUM_ALLOWED) != 0 ||
         ReturnResultList ) {

        //
        // Do the normal maximum-allowed access check
        //

        SepMaximumAccessCheck(
            EToken,
            PrimaryToken,
            Dacl,
            PrincipalSelfSid,
            LocalTypeListLength,
            LocalTypeList,
            ObjectTypeListLength,
            FALSE
            );

        //
        // If this is a restricted token, do the additional access check
        //

        if (SeTokenIsRestricted( EToken ) ) {
            SepMaximumAccessCheck(
                EToken,
                PrimaryToken,
                Dacl,
                PrincipalSelfSid,
                LocalTypeListLength,
                LocalTypeList,
                ObjectTypeListLength,
                TRUE
                );
        }


        //
        // If the caller wants to know the individual results of each sub-object,
        //  sub-object,
        //  break it down for him.
        //

        if ( ReturnResultList ) {
            ACCESS_MASK GrantedAccessMask;
            ACCESS_MASK RequiredAccessMask;
            BOOLEAN SomeAccessGranted = FALSE;
            BOOLEAN SomeAccessDenied = FALSE;

            //
            // Compute mask of Granted access bits to tell the caller about.
            //  If he asked for MAXIMUM_ALLOWED,
            //      tell him everything,
            //  otherwise
            //      tell him what he asked about.
            //

            if (DesiredAccess & MAXIMUM_ALLOWED) {
                GrantedAccessMask = (ACCESS_MASK) ~MAXIMUM_ALLOWED;
                RequiredAccessMask = (DesiredAccess | PreviouslyGrantedAccess) & ~MAXIMUM_ALLOWED;
            } else {
                GrantedAccessMask = DesiredAccess | PreviouslyGrantedAccess;
                RequiredAccessMask = DesiredAccess | PreviouslyGrantedAccess;
            }




            //
            // Loop computing the access granted to each object and sub-object.
            //
            for ( ResultListIndex=0;
                  ResultListIndex<LocalTypeListLength;
                  ResultListIndex++ ) {

                //
                // Return the subset of the access granted that the caller
                //  expressed interest in.
                //

                GrantedAccess[ResultListIndex] =
                    (LocalTypeList[ResultListIndex].CurrentGranted |
                     PreviouslyGrantedAccess ) &
                    GrantedAccessMask;

                //
                // If absolutely no access was granted,
                //  indicate so.
                //
                if ( GrantedAccess[ResultListIndex] == 0 ) {
                    AccessStatus[ResultListIndex] = STATUS_ACCESS_DENIED;
                    SomeAccessDenied = TRUE;
                } else {

                    //
                    // If some requested access is still missing,
                    //  the bottom line is that access is denied.
                    //
                    // Note, that ByTypeResultList actually returns the
                    // partially granted access mask even though the caller
                    // really has no access to the object.
                    //

                    if  ( ((~GrantedAccess[ResultListIndex]) & RequiredAccessMask ) != 0 ) {
                        AccessStatus[ResultListIndex] = STATUS_ACCESS_DENIED;
                        SomeAccessDenied = TRUE;
                    } else {
                        AccessStatus[ResultListIndex] = STATUS_SUCCESS;
                        SomeAccessGranted = TRUE;
                    }
                }
            }

            if ( SomeAccessGranted && PrivilegeCount != 0 ) {

                SepAssemblePrivileges(
                    PrivilegeCount,
                    SystemSecurity,
                    WriteOwner,
                    Privileges
                    );
            }

            if ( ARGUMENT_PRESENT(ReturnSomeAccessGranted)) {
                *ReturnSomeAccessGranted = SomeAccessGranted;
            }
            if ( ARGUMENT_PRESENT(ReturnSomeAccessDenied)) {
                *ReturnSomeAccessDenied = SomeAccessDenied;
            }
            return;

        //
        // If the caller is only interested in the access to the object itself,
        //  just summarize.
        //

        } else {

            //
            // Turn off the MAXIMUM_ALLOWED bit and whatever we found that
            // he was granted.  If the user passed in extra bits in addition
            // to MAXIMUM_ALLOWED, make sure that he was granted those access
            // types.  If not, he didn't get what he wanted, so return failure.
            //

            Remaining &= ~(MAXIMUM_ALLOWED | LocalTypeList->CurrentGranted);

            if (Remaining != 0) {

                Status = STATUS_ACCESS_DENIED;
                PreviouslyGrantedAccess = 0;
                goto ReturnOneStatus;

            }



            PreviouslyGrantedAccess |= LocalTypeList->CurrentGranted;
            Status = STATUS_SUCCESS;
            goto ReturnOneStatus;

        }

    } // if MAXIMUM_ALLOWED...


#ifdef notdef
    //
    // The remaining bits are "remaining" at all levels

    for ( j=0; j<LocalTypeListLength; j++ ) {
        LocalTypeList[j].Remaining = Remaining;
    }

    //
    // Process the DACL handling individual access bits.
    //

    for ( i = 0, Ace = FirstAce( Dacl ) ;
          ( i < AceCount ) && ( LocalTypeList->Remaining != 0 )  ;
          i++, Ace = NextAce( Ace ) ) {

        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) {

            //
            // Handle an Access Allowed ACE
            //

            if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_ACE_TYPE) ) {

               if ( SepSidInToken( EToken, PrincipalSelfSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart, FALSE ) ) {

                   // Optimize 'normal' case
                   if ( LocalTypeListLength == 1 ) {
                       LocalTypeList->Remaining &= ~((PACCESS_ALLOWED_ACE)Ace)->Mask;
                   } else {
                       //
                       // The zeroeth object type represents the object itself.
                       //
                       SepAddAccessTypeList(
                            LocalTypeList,          // List to modify
                            LocalTypeListLength,    // Length of list
                            0,                      // Element to update
                            ((PACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                            UpdateRemaining );
                   }

               }


            //
            // Handle an object specific Access Allowed ACE
            //
            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_OBJECT_ACE_TYPE) ) {
                GUID *ObjectTypeInAce;

                //
                // If no object type is in the ACE,
                //  treat this as an ACCESS_ALLOWED_ACE.
                //

                ObjectTypeInAce = RtlObjectAceObjectType(Ace);

                if ( ObjectTypeInAce == NULL ) {

                    if ( SepSidInToken( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), FALSE ) ) {

                       // Optimize 'normal' case
                       if ( LocalTypeListLength == 1 ) {
                           LocalTypeList->Remaining &= ~((PACCESS_ALLOWED_ACE)Ace)->Mask;
                       } else {
                           SepAddAccessTypeList(
                                LocalTypeList,          // List to modify
                                LocalTypeListLength,    // Length of list
                                0,                      // Element to update
                                ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                UpdateRemaining );
                       }
                    }

                //
                // If no object type list was passed,
                //  don't grant access to anyone.
                //

                } else if ( ObjectTypeListLength == 0 ) {

                    // Drop through


               //
               // If an object type is in the ACE,
               //   Find it in the LocalTypeList before using the ACE.
               //
               } else {

                    if ( SepSidInToken( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), FALSE ) ) {

                        if ( SepObjectInTypeList( ObjectTypeInAce,
                                                  LocalTypeList,
                                                  LocalTypeListLength,
                                                  &Index ) ) {
                            SepAddAccessTypeList(
                                 LocalTypeList,          // List to modify
                                 LocalTypeListLength,   // Length of list
                                 Index,                  // Element already updated
                                 ((PACCESS_ALLOWED_OBJECT_ACE)Ace)->Mask, // Access Granted
                                 UpdateRemaining );
                        }
                    }
               }


            //
            // Handle a compound Access Allowed ACE
            //
            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_COMPOUND_ACE_TYPE) ) {

                //
                // See comment in MAXIMUM_ALLOWED case as to why we can use EToken here
                // for the client.
                //

                if ( SepSidInToken(EToken, PrincipalSelfSid, RtlCompoundAceClientSid( Ace ), FALSE) &&
                     SepSidInToken(PrimaryToken, NULL, RtlCompoundAceServerSid( Ace ), FALSE) ) {

                    // Optimize 'normal' case
                    if ( LocalTypeListLength == 1 ) {
                        LocalTypeList->Remaining &= ~((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask;
                    } else {
                        SepAddAccessTypeList(
                             LocalTypeList,          // List to modify
                             LocalTypeListLength,    // Length of list
                             0,                      // Element to update
                             ((PCOMPOUND_ACCESS_ALLOWED_ACE)Ace)->Mask, // Access Granted
                             UpdateRemaining );
                    }
                }



            //
            // Handle an Access Denied ACE
            //

            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_DENIED_ACE_TYPE) ) {

                if ( SepSidInToken( EToken, PrincipalSelfSid, &((PACCESS_DENIED_ACE)Ace)->SidStart, TRUE ) ) {

                    //
                    // The zeroeth element represents the object itself.
                    //  Just check that element.
                    //
                    if (LocalTypeList->Remaining & ((PACCESS_DENIED_ACE)Ace)->Mask) {

                        break;
                    }
                }


            //
            // Handle an object specific Access Denied ACE
            //
            } else if ( (((PACE_HEADER)Ace)->AceType == ACCESS_DENIED_OBJECT_ACE_TYPE) ) {

                if ( SepSidInToken( EToken, PrincipalSelfSid, RtlObjectAceSid(Ace), TRUE ) ) {
                    GUID *ObjectTypeInAce;

                    //
                    // If there is no object type in the ACE,
                    //  or if the caller didn't specify an object type list,
                    //  apply this deny ACE to the entire object.
                    //

                    ObjectTypeInAce = RtlObjectAceObjectType(Ace);
                    if ( ObjectTypeInAce == NULL ||
                         ObjectTypeListLength == 0 ) {

                        //
                        // The zeroeth element represents the object itself.
                        //  Just check that element.
                        //
                        if (LocalTypeList->Remaining & ((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask) {
                            break;
                        }

                    //
                    // Otherwise apply the deny ACE to the object specified
                    //  in the ACE.
                    //

                    } else if ( SepObjectInTypeList( ObjectTypeInAce,
                                                  LocalTypeList,
                                                  LocalTypeListLength,
                                                  &Index ) ) {

                        if (LocalTypeList[Index].Remaining & ((PACCESS_DENIED_OBJECT_ACE)Ace)->Mask) {
                            break;
                        }

                    }
               }
            }

        }
    }

#endif

    //
    // Do the normal access check first
    //

    SepNormalAccessCheck(
        Remaining,
        EToken,
        PrimaryToken,
        Dacl,
        PrincipalSelfSid,
        LocalTypeListLength,
        LocalTypeList,
        ObjectTypeListLength,
        FALSE
        );

    if (LocalTypeList->Remaining != 0) {
        Status = STATUS_ACCESS_DENIED;
        PreviouslyGrantedAccess = 0;
        goto ReturnOneStatus;
    }

    //
    // If this is a restricted token, do the additional access check
    //

    if (SeTokenIsRestricted( EToken ) ) {
        SepNormalAccessCheck(
            Remaining,
            EToken,
            PrimaryToken,
            Dacl,
            PrincipalSelfSid,
            LocalTypeListLength,
            LocalTypeList,
            ObjectTypeListLength,
            TRUE
            );
    }


    if (LocalTypeList->Remaining != 0) {
        Status = STATUS_ACCESS_DENIED;
        PreviouslyGrantedAccess = 0;
        goto ReturnOneStatus;
    }

    Status = STATUS_SUCCESS;
    PreviouslyGrantedAccess |= DesiredAccess;

    //
    // Return a single status code to the caller.
    //

    ReturnOneStatus:
    if ( Status == STATUS_SUCCESS && PreviouslyGrantedAccess == 0 ) {
        Status = STATUS_ACCESS_DENIED;
    }

    //
    // If the caller asked for a list of status',
    //  duplicate the status all over.
    //
    if ( ReturnResultList ) {
        for ( ResultListIndex=0; ResultListIndex<ObjectTypeListLength; ResultListIndex++ ) {
            AccessStatus[ResultListIndex] = Status;
            GrantedAccess[ResultListIndex] = PreviouslyGrantedAccess;
        }
    } else {
        *AccessStatus = Status;
        *GrantedAccess = PreviouslyGrantedAccess;
    }

    if ( NT_SUCCESS(Status) ) {
        if ( PrivilegeCount > 0 ) {

            SepAssemblePrivileges(
                PrivilegeCount,
                SystemSecurity,
                WriteOwner,
                Privileges
                );
        }

        if ( ARGUMENT_PRESENT(ReturnSomeAccessGranted)) {
            *ReturnSomeAccessGranted = TRUE;
        }
        if ( ARGUMENT_PRESENT(ReturnSomeAccessDenied)) {
            *ReturnSomeAccessDenied = FALSE;
        }
    } else {
        if ( ARGUMENT_PRESENT(ReturnSomeAccessGranted)) {
            *ReturnSomeAccessGranted = FALSE;
        }
        if ( ARGUMENT_PRESENT(ReturnSomeAccessDenied)) {
            *ReturnSomeAccessDenied = TRUE;;
        }
    }
    return;

}




NTSTATUS
NtAccessCheck (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    IN OUT PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )


/*++

Routine Description:

    See module abstract.

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the object
        being accessed

    ClientToken - Supplies the handle of the user's token.

    DesiredAccess - Supplies the desired access mask.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    PrivilegeSet - A pointer to a buffer that upon return will contain
        any privileges that were used to perform the access validation.
        If no privileges were used, the buffer will contain a privilege
        set consisting of zero privileges.

    PrivilegeSetLength - The size of the PrivilegeSet buffer in bytes.

    GrantedAccess - Returns an access mask describing the granted access.

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

Return Value:

    STATUS_SUCCESS - The attempt proceeded normally.  This does not
        mean access was granted, rather that the parameters were
        correct.

    STATUS_GENERIC_NOT_MAPPED - The DesiredAccess mask contained
        an unmapped generic access.

    STATUS_BUFFER_TOO_SMALL - The passed buffer was not large enough
        to contain the information being returned.

    STATUS_NO_IMPERSONTAION_TOKEN - The passed Token was not an impersonation
        token.

--*/

{

    PAGED_CODE();

    return SeAccessCheckByType (
                 SecurityDescriptor,
                 NULL,      // No Principal Self sid
                 ClientToken,
                 DesiredAccess,
                 NULL,      // No ObjectType List
                 0,         // No ObjectType List
                 GenericMapping,
                 PrivilegeSet,
                 PrivilegeSetLength,
                 GrantedAccess,
                 AccessStatus,
                 FALSE );  // Return a single GrantedAccess and AccessStatus


}






NTSTATUS
NtAccessCheckByType (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    IN OUT PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )


/*++

Routine Description:

    See module abstract.

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the object
        being accessed

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    ClientToken - Supplies the handle of the user's token.

    DesiredAccess - Supplies the desired access mask.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    PrivilegeSet - A pointer to a buffer that upon return will contain
        any privileges that were used to perform the access validation.
        If no privileges were used, the buffer will contain a privilege
        set consisting of zero privileges.

    PrivilegeSetLength - The size of the PrivilegeSet buffer in bytes.

    GrantedAccess - Returns an access mask describing the granted access.

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

Return Value:

    STATUS_SUCCESS - The attempt proceeded normally.  This does not
        mean access was granted, rather that the parameters were
        correct.

    STATUS_GENERIC_NOT_MAPPED - The DesiredAccess mask contained
        an unmapped generic access.

    STATUS_BUFFER_TOO_SMALL - The passed buffer was not large enough
        to contain the information being returned.

    STATUS_NO_IMPERSONTAION_TOKEN - The passed Token was not an impersonation
        token.

--*/

{

    PAGED_CODE();

    return SeAccessCheckByType (
                 SecurityDescriptor,
                 PrincipalSelfSid,
                 ClientToken,
                 DesiredAccess,
                 ObjectTypeList,
                 ObjectTypeListLength,
                 GenericMapping,
                 PrivilegeSet,
                 PrivilegeSetLength,
                 GrantedAccess,
                 AccessStatus,
                 FALSE );  // Return a single GrantedAccess and AccessStatus
}





NTSTATUS
NtAccessCheckByTypeResultList (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    IN OUT PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )


/*++

Routine Description:

    See module abstract.

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the object
        being accessed

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    ClientToken - Supplies the handle of the user's token.

    DesiredAccess - Supplies the desired access mask.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    PrivilegeSet - A pointer to a buffer that upon return will contain
        any privileges that were used to perform the access validation.
        If no privileges were used, the buffer will contain a privilege
        set consisting of zero privileges.

    PrivilegeSetLength - The size of the PrivilegeSet buffer in bytes.

    GrantedAccess - Returns an access mask describing the granted access.

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

Return Value:

    STATUS_SUCCESS - The attempt proceeded normally.  This does not
        mean access was granted, rather that the parameters were
        correct.

    STATUS_GENERIC_NOT_MAPPED - The DesiredAccess mask contained
        an unmapped generic access.

    STATUS_BUFFER_TOO_SMALL - The passed buffer was not large enough
        to contain the information being returned.

    STATUS_NO_IMPERSONTAION_TOKEN - The passed Token was not an impersonation
        token.

--*/

{

    PAGED_CODE();

    return SeAccessCheckByType (
                 SecurityDescriptor,
                 PrincipalSelfSid,
                 ClientToken,
                 DesiredAccess,
                 ObjectTypeList,
                 ObjectTypeListLength,
                 GenericMapping,
                 PrivilegeSet,
                 PrivilegeSetLength,
                 GrantedAccess,
                 AccessStatus,
                 TRUE );  // Return an array of GrantedAccess and AccessStatus
}






NTSTATUS
SeAccessCheckByType (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE_LIST ObjectTypeList OPTIONAL,
    IN ULONG ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    IN OUT PULONG PrivilegeSetLength,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus,
    IN BOOLEAN ReturnResultList
    )


/*++

Routine Description:

    See module abstract.

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the object
        being accessed

    PrincipalSelfSid - If the object being access checked is an object which
        represents a principal (e.g., a user object), this parameter should
        be the SID of the object.  Any ACE containing the constant
        PRINCIPAL_SELF_SID is replaced by this SID.

        The parameter should be NULL if the object does not represent a principal.

    ClientToken - Supplies the handle of the user's token.

    DesiredAccess - Supplies the desired access mask.

    ObjectTypeList - Supplies a list of GUIDs representing the object (and
        sub-objects) being accessed.  If no list is present, AccessCheckByType
        behaves identically to AccessCheck.

    ObjectTypeListLength - Specifies the number of elements in the ObjectTypeList.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    PrivilegeSet - A pointer to a buffer that upon return will contain
        any privileges that were used to perform the access validation.
        If no privileges were used, the buffer will contain a privilege
        set consisting of zero privileges.

    PrivilegeSetLength - The size of the PrivilegeSet buffer in bytes.

    GrantedAccess - Returns an access mask describing the granted access.

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.

    ReturnResultList - If true, GrantedAccess and AccessStatus are actually
        arrays of entries ObjectTypeListLength elements long.

Return Value:

    STATUS_SUCCESS - The attempt proceeded normally.  This does not
        mean access was granted, rather that the parameters were
        correct.

    STATUS_GENERIC_NOT_MAPPED - The DesiredAccess mask contained
        an unmapped generic access.

    STATUS_BUFFER_TOO_SMALL - The passed buffer was not large enough
        to contain the information being returned.

    STATUS_NO_IMPERSONTAION_TOKEN - The passed Token was not an impersonation
        token.

--*/

{
    ACCESS_MASK LocalGrantedAccess;
    PACCESS_MASK LocalGrantedAccessPointer = NULL;
    NTSTATUS LocalAccessStatus;
    PNTSTATUS LocalAccessStatusPointer = NULL;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;
    PTOKEN Token = NULL;
    PSECURITY_DESCRIPTOR CapturedSecurityDescriptor = NULL;
    PSID CapturedPrincipalSelfSid = NULL;
    ACCESS_MASK PreviouslyGrantedAccess = 0;
    GENERIC_MAPPING LocalGenericMapping;
    PIOBJECT_TYPE_LIST LocalObjectTypeList = NULL;
    PPRIVILEGE_SET Privileges = NULL;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    ULONG LocalPrivilegeSetLength;
    ULONG ResultListIndex;

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode == KernelMode) {
        ASSERT( !ReturnResultList );
        *AccessStatus = STATUS_SUCCESS;
        *GrantedAccess = DesiredAccess;
        return(STATUS_SUCCESS);
    }

    try {

        if ( ReturnResultList ) {

            if ( ObjectTypeListLength == 0 ) {
                Status = STATUS_INVALID_PARAMETER;
                leave ;
            }

            if ( !IsValidElementCount( ObjectTypeListLength, OBJECT_TYPE_LIST ) )
            {
                Status = STATUS_INVALID_PARAMETER ;

                leave ;
            }

            ProbeForWrite(
                AccessStatus,
                sizeof(NTSTATUS) * ObjectTypeListLength,
                sizeof(ULONG)
                );

            ProbeForWrite(
                GrantedAccess,
                sizeof(ACCESS_MASK) * ObjectTypeListLength,
                sizeof(ULONG)
                );

        } else {
            ProbeForWriteUlong((PULONG)AccessStatus);
            ProbeForWriteUlong((PULONG)GrantedAccess);
        }

        LocalPrivilegeSetLength = ProbeAndReadUlong( PrivilegeSetLength );
        ProbeForWriteUlong(
            PrivilegeSetLength
            );

        ProbeForWrite(
            PrivilegeSet,
            LocalPrivilegeSetLength,
            sizeof(ULONG)
            );

        ProbeForRead(
            GenericMapping,
            sizeof(GENERIC_MAPPING),
            sizeof(ULONG)
            );

        LocalGenericMapping = *GenericMapping;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }
    if (!NT_SUCCESS( Status ) ) {
        return( Status );
    }

    if (DesiredAccess &
        ( GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL )) {


        Status = STATUS_GENERIC_NOT_MAPPED;
        goto Cleanup;
    }

    //
    // Obtain a pointer to the passed token
    //

    Status = ObReferenceObjectByHandle(
                 ClientToken,                  // Handle
                 (ACCESS_MASK)TOKEN_QUERY,     // DesiredAccess
                 SepTokenObjectType,           // ObjectType
                 PreviousMode,                 // AccessMode
                 (PVOID *)&Token,              // Object
                 0                             // GrantedAccess
                 );

    if (!NT_SUCCESS(Status)) {
        Token = NULL;
        goto Cleanup;
    }

    //
    // It must be an impersonation token, and at impersonation
    // level of Identification or above.
    //

    if (Token->TokenType != TokenImpersonation) {
        Status = STATUS_NO_IMPERSONATION_TOKEN;
        goto Cleanup;
    }

    if ( Token->ImpersonationLevel < SecurityIdentification ) {
        Status = STATUS_BAD_IMPERSONATION_LEVEL;
        goto Cleanup;
    }

    //
    // Capture any Object type list
    //

    Status = SeCaptureObjectTypeList( ObjectTypeList,
                                      ObjectTypeListLength,
                                      PreviousMode,
                                      &LocalObjectTypeList );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }

    //
    // Compare the DesiredAccess with the privileges in the
    // passed token, and see if we can either satisfy the requested
    // access with a privilege, or bomb out immediately because
    // we don't have a privilege we need.
    //

    Status = SePrivilegePolicyCheck(
                 &DesiredAccess,
                 &PreviouslyGrantedAccess,
                 NULL,
                 (PACCESS_TOKEN)Token,
                 &Privileges,
                 PreviousMode
                 );

    if (!NT_SUCCESS( Status )) {

        try {

            if ( ReturnResultList ) {
                for ( ResultListIndex=0; ResultListIndex<ObjectTypeListLength; ResultListIndex++ ) {
                    AccessStatus[ResultListIndex] = Status;
                    GrantedAccess[ResultListIndex] = 0;
                }

            } else {
                *AccessStatus = Status;
                *GrantedAccess = 0;
            }

            Status = STATUS_SUCCESS;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();
        }

        goto Cleanup;
    }

    //
    // Make sure the passed privileges buffer is large enough for
    // whatever we have to put into it.
    //

    if (Privileges != NULL) {

        if ( ((ULONG)SepPrivilegeSetSize( Privileges )) > LocalPrivilegeSetLength ) {

            try {

                *PrivilegeSetLength = SepPrivilegeSetSize( Privileges );
                Status = STATUS_BUFFER_TOO_SMALL;

            } except ( EXCEPTION_EXECUTE_HANDLER ) {

                Status = GetExceptionCode();
            }

            SeFreePrivileges( Privileges );

            goto Cleanup;

        } else {

            try {

                RtlCopyMemory(
                    PrivilegeSet,
                    Privileges,
                    SepPrivilegeSetSize( Privileges )
                    );

            } except ( EXCEPTION_EXECUTE_HANDLER ) {

                SeFreePrivileges( Privileges );
                Status = GetExceptionCode();
                goto Cleanup;
            }

        }
        SeFreePrivileges( Privileges );

    } else {

        //
        // No privileges were used, construct an empty privilege set
        //

        if ( LocalPrivilegeSetLength < sizeof(PRIVILEGE_SET) ) {

            try {

                *PrivilegeSetLength = sizeof(PRIVILEGE_SET);
                Status = STATUS_BUFFER_TOO_SMALL;

            } except ( EXCEPTION_EXECUTE_HANDLER ) {

                Status = GetExceptionCode();
            }

            goto Cleanup;
        }

        try {

            PrivilegeSet->PrivilegeCount = 0;
            PrivilegeSet->Control = 0;

        } except ( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            goto Cleanup;

        }

    }

    //
    // Capture the PrincipalSelfSid.
    //

    if ( PrincipalSelfSid != NULL ) {
        Status = SeCaptureSid(
                     PrincipalSelfSid,
                     PreviousMode,
                     NULL, 0,
                     PagedPool,
                     TRUE,
                     &CapturedPrincipalSelfSid );

        if (!NT_SUCCESS(Status)) {
            CapturedPrincipalSelfSid = NULL;
            goto Cleanup;
        }
    }


    //
    // Capture the passed security descriptor.
    //
    // SeCaptureSecurityDescriptor probes the input security descriptor,
    // so we don't have to
    //

    Status = SeCaptureSecurityDescriptor (
                SecurityDescriptor,
                PreviousMode,
                PagedPool,
                FALSE,
                &CapturedSecurityDescriptor
                );

    if (!NT_SUCCESS(Status)) {
        goto Cleanup;
    }


    //
    // If there's no security descriptor, then we've been
    // called without all the parameters we need.
    // Return invalid security descriptor.
    //

    if ( CapturedSecurityDescriptor == NULL ) {
        Status = STATUS_INVALID_SECURITY_DESCR;
        goto Cleanup;
    }

    //
    // A valid security descriptor must have an owner and a group
    //

    if ( RtlpOwnerAddrSecurityDescriptor(
                (PISECURITY_DESCRIPTOR)CapturedSecurityDescriptor
                ) == NULL ||
         RtlpGroupAddrSecurityDescriptor(
                (PISECURITY_DESCRIPTOR)CapturedSecurityDescriptor
                ) == NULL ) {

        SeReleaseSecurityDescriptor (
            CapturedSecurityDescriptor,
            PreviousMode,
            FALSE
            );

        Status = STATUS_INVALID_SECURITY_DESCR;
        goto Cleanup;
    }


    SeCaptureSubjectContext( &SubjectContext );

    SepAcquireTokenReadLock( Token );

    //
    // If the user in the token is the owner of the object, we
    // must automatically grant ReadControl and WriteDac access
    // if desired.  If the DesiredAccess mask is empty after
    // these bits are turned off, we don't have to do any more
    // access checking (ref section 4, DSA ACL Arch)
    //


    if ( DesiredAccess & (WRITE_DAC | READ_CONTROL | MAXIMUM_ALLOWED) ) {

        if (SepTokenIsOwner( Token, CapturedSecurityDescriptor, TRUE )) {

            if ( DesiredAccess & MAXIMUM_ALLOWED ) {

                PreviouslyGrantedAccess |= (WRITE_DAC | READ_CONTROL);

            } else {

                PreviouslyGrantedAccess |= (DesiredAccess & (WRITE_DAC | READ_CONTROL));
            }

            DesiredAccess &= ~(WRITE_DAC | READ_CONTROL);
        }

    }

    if (DesiredAccess == 0) {

        try {


            if ( ReturnResultList ) {
                for ( ResultListIndex=0; ResultListIndex<ObjectTypeListLength; ResultListIndex++ ) {
                    AccessStatus[ResultListIndex] = STATUS_SUCCESS;
                    GrantedAccess[ResultListIndex] = PreviouslyGrantedAccess;
                }

            } else {
                *AccessStatus = STATUS_SUCCESS;
                *GrantedAccess = PreviouslyGrantedAccess;
            }
            Status = STATUS_SUCCESS;

        } except (EXCEPTION_EXECUTE_HANDLER) {

            Status = GetExceptionCode();

        }

        SepReleaseTokenReadLock( Token );

        SeReleaseSubjectContext( &SubjectContext );

        SeReleaseSecurityDescriptor (
            CapturedSecurityDescriptor,
            PreviousMode,
            FALSE
            );

        goto Cleanup;

    }


    //
    // Finally, handle the case where we actually have to check the DACL.
    //

    if ( ReturnResultList ) {
        LocalGrantedAccessPointer =
            ExAllocatePoolWithTag( PagedPool, (sizeof(ACCESS_MASK)+sizeof(NTSTATUS)) * ObjectTypeListLength, 'aGeS' );

        if (LocalGrantedAccessPointer == NULL) {

            SepReleaseTokenReadLock( Token );

            SeReleaseSubjectContext( &SubjectContext );

            SeReleaseSecurityDescriptor (
                CapturedSecurityDescriptor,
                PreviousMode,
                FALSE
                );

            Status = STATUS_INSUFFICIENT_RESOURCES;
            goto Cleanup;
        }
        LocalAccessStatusPointer = (PNTSTATUS)(LocalGrantedAccessPointer + ObjectTypeListLength);
    } else {
        LocalGrantedAccessPointer = &LocalGrantedAccess;
        LocalAccessStatusPointer =  &LocalAccessStatus;
    }

    SepAccessCheck (
        CapturedSecurityDescriptor,
        CapturedPrincipalSelfSid,
        SubjectContext.PrimaryToken,
        Token,
        DesiredAccess,
        LocalObjectTypeList,
        ObjectTypeListLength,
        &LocalGenericMapping,
        PreviouslyGrantedAccess,
        PreviousMode,
        LocalGrantedAccessPointer,
        NULL,
        LocalAccessStatusPointer,
        ReturnResultList,
        NULL,
        NULL );

    SepReleaseTokenReadLock( Token );

    SeReleaseSubjectContext( &SubjectContext );

    SeReleaseSecurityDescriptor (
        CapturedSecurityDescriptor,
        PreviousMode,
        FALSE
        );

    try {

        if ( ReturnResultList ) {
            for ( ResultListIndex=0; ResultListIndex<ObjectTypeListLength; ResultListIndex++ ) {
                AccessStatus[ResultListIndex] = LocalAccessStatusPointer[ResultListIndex];
                GrantedAccess[ResultListIndex] = LocalGrantedAccessPointer[ResultListIndex];
            }

        } else {
            *AccessStatus = *LocalAccessStatusPointer;
            *GrantedAccess = *LocalGrantedAccessPointer;
        }

        Status = STATUS_SUCCESS;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if ( ReturnResultList ) {
        if ( LocalGrantedAccessPointer != NULL ) {
            ExFreePool( LocalGrantedAccessPointer );
        }
    }


    //
    // Free locally used resources.
    //
Cleanup:

    if ( Token != NULL ) {
        ObDereferenceObject( Token );
    }

    if ( LocalObjectTypeList != NULL ) {
        SeFreeCapturedObjectTypeList( LocalObjectTypeList );
    }

    if (CapturedPrincipalSelfSid != NULL) {
        SeReleaseSid( CapturedPrincipalSelfSid, PreviousMode, TRUE);
    }

    return Status;
}



VOID
SeFreePrivileges(
    IN PPRIVILEGE_SET Privileges
    )

/*++

Routine Description:

    This routine frees a privilege set returned by SeAccessCheck.

Arguments:

    Privileges - Supplies a pointer to the privilege set to be freed.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    ExFreePool( Privileges );
}



BOOLEAN
SeAccessCheck (
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN BOOLEAN SubjectContextLocked,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    OUT PPRIVILEGE_SET *Privileges OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN KPROCESSOR_MODE AccessMode,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )

/*++

Routine Description:

    See module abstract

    This routine MAY perform tests for the following
    privileges:

                SeTakeOwnershipPrivilege
                SeSecurityPrivilege

    depending upon the accesses being requested.

    This routine may also check to see if the subject is the owner
    of the object (to grant WRITE_DAC access).

Arguments:

    SecurityDescriptor - Supplies the security descriptor protecting the
         object being accessed

    SubjectSecurityContext - A pointer to the subject's captured security
         context

    SubjectContextLocked - Supplies a flag indiciating whether or not
        the user's subject context is locked, so that it does not have
        to be locked again.

    DesiredAccess - Supplies the access mask that the user is attempting to
         acquire

    PreviouslyGrantedAccess - Supplies any accesses that the user has
        already been granted, for example, as a result of holding a
        privilege.

    Privileges - Supplies a pointer in which will be returned a privilege
        set indicating any privileges that were used as part of the
        access validation.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    AccessMode - Supplies the access mode to be used in the check

    GrantedAccess - Pointer to a returned access mask indicatating the
         granted access

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.


Return Value:

    BOOLEAN - TRUE if access is allowed and FALSE otherwise

--*/

{
    BOOLEAN Success;

    PAGED_CODE();

    if (AccessMode == KernelMode) {

        if (DesiredAccess & MAXIMUM_ALLOWED) {

            //
            // Give him:
            //   GenericAll
            //   Anything else he asked for
            //

            *GrantedAccess = GenericMapping->GenericAll;
            *GrantedAccess |= (DesiredAccess & ~MAXIMUM_ALLOWED);
            *GrantedAccess |= PreviouslyGrantedAccess;

        } else {

            *GrantedAccess = DesiredAccess | PreviouslyGrantedAccess;
        }
        *AccessStatus = STATUS_SUCCESS;
        return(TRUE);
    }

    //
    // If the object doesn't have a security descriptor (and it's supposed
    // to), return access denied.
    //

    if ( SecurityDescriptor == NULL) {

       *AccessStatus = STATUS_ACCESS_DENIED;
       return( FALSE );

    }

    //
    // If we're impersonating a client, we have to be at impersonation level
    // of SecurityImpersonation or above.
    //

    if ( (SubjectSecurityContext->ClientToken != NULL) &&
         (SubjectSecurityContext->ImpersonationLevel < SecurityImpersonation)
       ) {
           *AccessStatus = STATUS_BAD_IMPERSONATION_LEVEL;
           return( FALSE );
    }

    if ( DesiredAccess == 0 ) {

        if ( PreviouslyGrantedAccess == 0 ) {
            *AccessStatus = STATUS_ACCESS_DENIED;
            return( FALSE );
        }

        *GrantedAccess = PreviouslyGrantedAccess;
        *AccessStatus = STATUS_SUCCESS;
        *Privileges = NULL;
        return( TRUE );

    }

    SeAssertMappedCanonicalAccess( DesiredAccess );


    //
    // If the caller did not lock the subject context for us,
    // lock it here to keep lower level routines from having
    // to lock it.
    //

    if ( !SubjectContextLocked ) {
        SeLockSubjectContext( SubjectSecurityContext );
    }

    //
    // If the user in the token is the owner of the object, we
    // must automatically grant ReadControl and WriteDac access
    // if desired.  If the DesiredAccess mask is empty after
    // these bits are turned off, we don't have to do any more
    // access checking (ref section 4, DSA ACL Arch)
    //

    if ( DesiredAccess & (WRITE_DAC | READ_CONTROL | MAXIMUM_ALLOWED) ) {

        if ( SepTokenIsOwner(
                 EffectiveToken( SubjectSecurityContext ),
                 SecurityDescriptor,
                 TRUE
                 ) ) {

            if ( DesiredAccess & MAXIMUM_ALLOWED ) {

                PreviouslyGrantedAccess |= (WRITE_DAC | READ_CONTROL);

            } else {

                PreviouslyGrantedAccess |= (DesiredAccess & (WRITE_DAC | READ_CONTROL));
            }

            DesiredAccess &= ~(WRITE_DAC | READ_CONTROL);
        }
    }

    if (DesiredAccess == 0) {

        if ( !SubjectContextLocked ) {
            SeUnlockSubjectContext( SubjectSecurityContext );
        }

        *GrantedAccess = PreviouslyGrantedAccess;
        *AccessStatus = STATUS_SUCCESS;
        return( TRUE );

    } else {

        SepAccessCheck(
                    SecurityDescriptor,
                    NULL,   // No PrincipalSelfSid
                    SubjectSecurityContext->PrimaryToken,
                    SubjectSecurityContext->ClientToken,
                    DesiredAccess,
                    NULL,   // No object type list
                    0,      // No object type list
                    GenericMapping,
                    PreviouslyGrantedAccess,
                    AccessMode,
                    GrantedAccess,
                    Privileges,
                    AccessStatus,
                    FALSE,   // Don't return a list
                    &Success,
                    NULL
                    );
#if DBG
          if (!Success && SepShowAccessFail) {
              DbgPrint("SE: Access check failed, DesiredAccess = 0x%x\n",
                DesiredAccess);
              SepDumpSD = TRUE;
              SepDumpSecurityDescriptor(
                  SecurityDescriptor,
                  "Input to SeAccessCheck\n"
                  );
              SepDumpSD = FALSE;
              SepDumpToken = TRUE;
              SepDumpTokenInfo( EffectiveToken( SubjectSecurityContext ) );
              SepDumpToken = FALSE;
          }
#endif

        //
        // If we locked it in this routine, unlock it before we
        // leave.
        //

        if ( !SubjectContextLocked ) {
            SeUnlockSubjectContext( SubjectSecurityContext );
        }

        return( Success );
    }
}


BOOLEAN
SeProxyAccessCheck (
    IN PUNICODE_STRING Volume,
    IN PUNICODE_STRING RelativePath,
    IN BOOLEAN ContainerObject,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext,
    IN BOOLEAN SubjectContextLocked,
    IN ACCESS_MASK DesiredAccess,
    IN ACCESS_MASK PreviouslyGrantedAccess,
    OUT PPRIVILEGE_SET *Privileges OPTIONAL,
    IN PGENERIC_MAPPING GenericMapping,
    IN KPROCESSOR_MODE AccessMode,
    OUT PACCESS_MASK GrantedAccess,
    OUT PNTSTATUS AccessStatus
    )

/*++

Routine Description:


Arguments:

    Volume - Supplies the volume information of the file being opened.

    RelativePath - The volume-relative path of the file being opened.  The full path of the
        file is the RelativePath appended to the Volume string.

    ContainerObject - Indicates if the access is to a container object (TRUE), or a leaf object (FALSE).

    SecurityDescriptor - Supplies the security descriptor protecting the
         object being accessed

    SubjectSecurityContext - A pointer to the subject's captured security
         context

    SubjectContextLocked - Supplies a flag indiciating whether or not
        the user's subject context is locked, so that it does not have
        to be locked again.

    DesiredAccess - Supplies the access mask that the user is attempting to
         acquire

    PreviouslyGrantedAccess - Supplies any accesses that the user has
        already been granted, for example, as a result of holding a
        privilege.

    Privileges - Supplies a pointer in which will be returned a privilege
        set indicating any privileges that were used as part of the
        access validation.

    GenericMapping - Supplies the generic mapping associated with this
        object type.

    AccessMode - Supplies the access mode to be used in the check

    GrantedAccess - Pointer to a returned access mask indicatating the
         granted access

    AccessStatus - Status value that may be returned indicating the
         reason why access was denied.  Routines should avoid hardcoding a
         return value of STATUS_ACCESS_DENIED so that a different value can
         be returned when mandatory access control is implemented.


Return Value:

    BOOLEAN - TRUE if access is allowed and FALSE otherwise

--*/

{
    return SeAccessCheck (
                SecurityDescriptor,
                SubjectSecurityContext,
                SubjectContextLocked,
                DesiredAccess,
                PreviouslyGrantedAccess,
                Privileges,
                GenericMapping,
                AccessMode,
                GrantedAccess,
                AccessStatus
               );
}


NTSTATUS
SePrivilegePolicyCheck(
    IN OUT PACCESS_MASK RemainingDesiredAccess,
    IN OUT PACCESS_MASK PreviouslyGrantedAccess,
    IN PSECURITY_SUBJECT_CONTEXT SubjectSecurityContext OPTIONAL,
    IN PACCESS_TOKEN ExplicitToken OPTIONAL,
    OUT PPRIVILEGE_SET *PrivilegeSet,
    IN KPROCESSOR_MODE PreviousMode
    )

/*++

Routine Description:

    This routine implements privilege policy by examining the bits in
    a DesiredAccess mask and adjusting them based on privilege checks.

    Currently, a request for ACCESS_SYSTEM_SECURITY may only be satisfied
    by the caller having SeSecurityPrivilege.  WRITE_OWNER may optionally
    be satisfied via SeTakeOwnershipPrivilege.

Arguments:

    RemainingDesiredAccess - The desired access for the current operation.
        Bits may be cleared in this if the subject has particular privileges.

    PreviouslyGrantedAccess - Supplies an access mask describing any
        accesses that have already been granted.  Bits may be set in
        here as a result of privilge checks.

    SubjectSecurityContext - Optionally provides the subject's security
        context.

    ExplicitToken - Optionally provides the token to be examined.

    PrivilegeSet - Supplies a pointer to a location in which will be
        returned a pointer to a privilege set.

    PreviousMode - The previous processor mode.

Return Value:

    STATUS_SUCCESS - Any access requests that could be satisfied via
        privileges were done.

    STATUS_PRIVILEGE_NOT_HELD - An access type was being requested that
        requires a privilege, and the current subject did not have the
        privilege.



--*/

{
    BOOLEAN Success;
    PTOKEN Token;
    BOOLEAN WriteOwner = FALSE;
    BOOLEAN SystemSecurity = FALSE;
    ULONG PrivilegeNumber = 0;
    ULONG PrivilegeCount = 0;
    ULONG SizeRequired;

    PAGED_CODE();

    if (ARGUMENT_PRESENT( SubjectSecurityContext )) {

        Token = (PTOKEN)EffectiveToken( SubjectSecurityContext );

    } else {

        Token = (PTOKEN)ExplicitToken;
    }


    if (*RemainingDesiredAccess & ACCESS_SYSTEM_SECURITY) {

        Success = SepSinglePrivilegeCheck (
                    SeSecurityPrivilege,
                    Token,
                    PreviousMode
                    );

        if (!Success) {

            return( STATUS_PRIVILEGE_NOT_HELD );
        }

        PrivilegeCount++;
        SystemSecurity = TRUE;

        *RemainingDesiredAccess &= ~ACCESS_SYSTEM_SECURITY;
        *PreviouslyGrantedAccess |= ACCESS_SYSTEM_SECURITY;
    }

    if (*RemainingDesiredAccess & WRITE_OWNER) {

        Success = SepSinglePrivilegeCheck (
                    SeTakeOwnershipPrivilege,
                    Token,
                    PreviousMode
                    );

        if (Success) {

            PrivilegeCount++;
            WriteOwner = TRUE;

            *RemainingDesiredAccess &= ~WRITE_OWNER;
            *PreviouslyGrantedAccess |= WRITE_OWNER;

        }
    }

    if (PrivilegeCount > 0) {
        SizeRequired = sizeof(PRIVILEGE_SET) +
                        (PrivilegeCount - ANYSIZE_ARRAY) *
                        (ULONG)sizeof(LUID_AND_ATTRIBUTES);

        *PrivilegeSet = ExAllocatePoolWithTag( PagedPool, SizeRequired, 'rPeS' );

        if ( *PrivilegeSet == NULL ) {
            return( STATUS_INSUFFICIENT_RESOURCES );
        }

        (*PrivilegeSet)->PrivilegeCount = PrivilegeCount;
        (*PrivilegeSet)->Control = 0;

        if (WriteOwner) {
            (*PrivilegeSet)->Privilege[PrivilegeNumber].Luid = SeTakeOwnershipPrivilege;
            (*PrivilegeSet)->Privilege[PrivilegeNumber].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
            PrivilegeNumber++;
        }

        if (SystemSecurity) {
            (*PrivilegeSet)->Privilege[PrivilegeNumber].Luid = SeSecurityPrivilege;
            (*PrivilegeSet)->Privilege[PrivilegeNumber].Attributes = SE_PRIVILEGE_USED_FOR_ACCESS;
        }
    }

    return( STATUS_SUCCESS );
}



BOOLEAN
SepTokenIsOwner(
    IN PACCESS_TOKEN EffectiveToken,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN BOOLEAN TokenLocked
    )

/*++

Routine Description:

    This routine will determine of the Owner of the passed security descriptor
    is in the passed token. If the token is restricted it cannot be the
    owner.


Arguments:

    Token - The token representing the current user.

    SecurityDescriptor - The security descriptor for the object being
        accessed.

    TokenLocked - A boolean describing whether the caller has taken
        a read lock for the token.


Return Value:

    TRUE - The user of the token is the owner of the object.

    FALSE - The user of the token is not the owner of the object.

--*/

{
    PSID Owner;
    BOOLEAN rc;

    PISECURITY_DESCRIPTOR ISecurityDescriptor;
    PTOKEN Token;

    PAGED_CODE();

    ISecurityDescriptor = (PISECURITY_DESCRIPTOR)SecurityDescriptor;
    Token = (PTOKEN)EffectiveToken;

    if (!TokenLocked) {
        SepAcquireTokenReadLock( Token );
    }

    Owner = RtlpOwnerAddrSecurityDescriptor( ISecurityDescriptor );
    ASSERT( Owner != NULL );

    rc = SepSidInToken( Token, NULL, Owner, FALSE );

    //
    // For restricted tokens, check the restricted sids too.
    //

    if (rc && (Token->TokenFlags & TOKEN_IS_RESTRICTED) != 0) {
        rc = SepSidInTokenEx( Token, NULL, Owner, FALSE, TRUE );

    }

    if (!TokenLocked) {
        SepReleaseTokenReadLock( Token );
    }

    return( rc );
}




BOOLEAN
SeFastTraverseCheck(
    PSECURITY_DESCRIPTOR SecurityDescriptor,
    ACCESS_MASK TraverseAccess,
    KPROCESSOR_MODE AccessMode
    )
/*++

Routine Description:

    This routine will examine the DACL of the passed Security Descriptor
    to see if WORLD has Traverse access.  If so, no further access checking
    is necessary.

    Note that the SubjectContext for the client process does not have
    to be locked to make this call, since it does not examine any data
    structures in the Token.

Arguments:

    SecurityDescriptor - The Security Descriptor protecting the container
        object being traversed.

    TraverseAccess - Access mask describing Traverse access for this
        object type.

    AccessMode - Supplies the access mode to be used in the check


Return Value:

    TRUE - WORLD has Traverse access to this container.  FALSE
    otherwise.

--*/

{
    PACL Dacl;
    ULONG i;
    PVOID Ace;
    ULONG AceCount;

    PAGED_CODE();

    if ( AccessMode == KernelMode ) {
        return( TRUE );
    }

    if (SecurityDescriptor == NULL) {
        return( FALSE );
    }

    //
    // See if there is a valid DACL in the passed Security Descriptor.
    // No DACL, no security, all is granted.
    //

    Dacl = RtlpDaclAddrSecurityDescriptor( (PISECURITY_DESCRIPTOR)SecurityDescriptor );

    //
    //  If the SE_DACL_PRESENT bit is not set, the object has no
    //  security, so all accesses are granted.
    //
    //  Also grant all access if the Dacl is NULL.
    //

    if ( !RtlpAreControlBitsSet(
            (PISECURITY_DESCRIPTOR)SecurityDescriptor, SE_DACL_PRESENT
            )
         || (Dacl == NULL)) {

        return(TRUE);
    }

    //
    // There is security on this object.  If the DACL is empty,
    // deny all access immediately
    //

    if ((AceCount = Dacl->AceCount) == 0) {

        return( FALSE );
    }

    //
    // There's stuff in the DACL, walk down the list and see
    // if WORLD has been granted TraverseAccess
    //

    for ( i = 0, Ace = FirstAce( Dacl ) ;
          i < AceCount  ;
          i++, Ace = NextAce( Ace )
        ) {

        if ( !(((PACE_HEADER)Ace)->AceFlags & INHERIT_ONLY_ACE)) {

            if ( (((PACE_HEADER)Ace)->AceType == ACCESS_ALLOWED_ACE_TYPE) ) {

                if ( (TraverseAccess & ((PACCESS_ALLOWED_ACE)Ace)->Mask) ) {

                    if ( RtlEqualSid( SeWorldSid, &((PACCESS_ALLOWED_ACE)Ace)->SidStart ) ) {

                        return( TRUE );
                    }
                }

            } else {

                if ( (((PACE_HEADER)Ace)->AceType == ACCESS_DENIED_ACE_TYPE) ) {

                    if ( (TraverseAccess & ((PACCESS_DENIED_ACE)Ace)->Mask) ) {

                        if ( RtlEqualSid( SeWorldSid, &((PACCESS_DENIED_ACE)Ace)->SidStart ) ) {

                            return( FALSE );
                        }
                    }
                }
            }
        }
    }

    return( FALSE );
}
