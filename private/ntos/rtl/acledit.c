/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Acledit.c

Abstract:

    This Module implements the Acl rtl editing functions that are defined in
    ntseapi.h

Author:

    Gary Kimura     (GaryKi)    9-Nov-1989

Environment:

    Pure Runtime Library Routine

Revision History:

--*/

#include <ntrtlp.h>
#include <seopaque.h>

//
//  Define the local macros and procedure for this module
//

//
//  Return a pointer to the first Ace in an Acl (even if the Acl is empty).
//
//      PACE_HEADER
//      FirstAce (
//          IN PACL Acl
//          );
//

#define FirstAce(Acl) ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)))

//
//  Return a pointer to the next Ace in a sequence (even if the input
//  Ace is the one in the sequence).
//
//      PACE_HEADER
//      NextAce (
//          IN PACE_HEADER Ace
//          );
//

#define NextAce(Ace) ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize))

#define LongAligned( ptr )  (LongAlign(ptr) == ((PVOID)(ptr)))
#define WordAligned( ptr )  (WordAlign(ptr) == ((PVOID)(ptr)))


    VOID
RtlpAddData (
    IN PVOID From,
    IN ULONG FromSize,
    IN PVOID To,
    IN ULONG ToSize
    );

VOID
RtlpDeleteData (
    IN PVOID Data,
    IN ULONG RemoveSize,
    IN ULONG TotalSize
    );

#if defined(ALLOC_PRAGMA) && defined(NTOS_KERNEL_RUNTIME)
#pragma alloc_text(PAGE,RtlpAddData)
#pragma alloc_text(PAGE,RtlpDeleteData)
#pragma alloc_text(PAGE,RtlCreateAcl)
#pragma alloc_text(PAGE,RtlValidAcl)
#pragma alloc_text(PAGE,RtlQueryInformationAcl)
#pragma alloc_text(PAGE,RtlSetInformationAcl)
#pragma alloc_text(PAGE,RtlAddAce)
#pragma alloc_text(PAGE,RtlDeleteAce)
#pragma alloc_text(PAGE,RtlGetAce)
#pragma alloc_text(PAGE,RtlAddAccessAllowedAce)
#pragma alloc_text(PAGE,RtlAddAccessAllowedAceEx)
#pragma alloc_text(PAGE,RtlAddAccessAllowedObjectAce)
#pragma alloc_text(PAGE,RtlAddAccessDeniedAce)
#pragma alloc_text(PAGE,RtlAddAccessDeniedAceEx)
#pragma alloc_text(PAGE,RtlAddAccessDeniedObjectAce)
#pragma alloc_text(PAGE,RtlAddAuditAccessAce)
#pragma alloc_text(PAGE,RtlAddAuditAccessAceEx)
#pragma alloc_text(PAGE,RtlAddAuditAccessObjectAce)
#pragma alloc_text(PAGE,RtlFirstFreeAce)
#endif


NTSTATUS
RtlCreateAcl (
    IN PACL Acl,
    IN ULONG AclLength,
    IN ULONG AclRevision
    )

/*++

Routine Description:

    This routine initializes an ACL data structure.  After initialization
    it is an ACL with no ACE (i.e., a deny all access type ACL)

Arguments:

    Acl - Supplies the buffer containing the ACL being initialized

    AclLength - Supplies the length of the ace buffer in bytes

    AclRevision - Supplies the revision for this Acl

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful

               STATUS_BUFFER_TOO_SMALL if the AclLength is too small,

               STATUS_INVALID_PARAMETER if the revision is out of range

--*/

{
    RTL_PAGED_CODE();

    //
    //  Check to see the size of the buffer is large enough to hold at
    //  least the ACL header
    //

    if (AclLength < sizeof(ACL)) {

        //
        //  Buffer to small even for the ACL header
        //

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    //  Check to see if the revision is currently valid.  Later versions
    //  of this procedure might accept more revision levels
    //

    if (AclRevision < MIN_ACL_REVISION || AclRevision > MAX_ACL_REVISION) {

        //
        //  Revision not current
        //

        return STATUS_INVALID_PARAMETER;

    }

    if ( AclLength > MAXUSHORT ) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Initialize the ACL
    //

    Acl->AclRevision = (UCHAR)AclRevision;  // Used to hardwire ACL_REVISION2 here
    Acl->Sbz1 = 0;
    Acl->AclSize = (USHORT) (AclLength & 0xfffc);
    Acl->AceCount = 0;
    Acl->Sbz2 = 0;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


BOOLEAN
RtlValidAcl (
    IN PACL Acl
    )

/*++

Routine Description:

    This procedure validates an ACL.

    This involves validating the revision level of the ACL and ensuring
    that the number of ACEs specified in the AceCount fit in the space
    specified by the AclSize field of the ACL header.

Arguments:

    Acl - Pointer to the ACL structure to validate.

Return Value:

    BOOLEAN - TRUE if the structure of Acl is valid.

--*/

{
    RTL_PAGED_CODE();

    try {
        PACE_HEADER Ace;
        PISID Sid;
        PISID Sid2;
        ULONG i;
        UCHAR AclRevision = ACL_REVISION2;


        //
        //  Check the ACL revision level
        //
        if (!ValidAclRevision(Acl)) {
            return(FALSE);
        }


        if (!WordAligned(&Acl->AclSize)) {
            return(FALSE);
        }

        if (Acl->AclSize < sizeof(ACL)) {
            return(FALSE);
        }
        //
        // Validate all of the ACEs.
        //

        Ace = ((PVOID)((PUCHAR)(Acl) + sizeof(ACL)));

        for (i = 0; i < Acl->AceCount; i++) {

            //
            //  Check to make sure we haven't overrun the Acl buffer
            //  with our ace pointer.  Make sure the ACE_HEADER is in
            //  the ACL also.
            //

            if ((PUCHAR)Ace + sizeof(ACE_HEADER) >= ((PUCHAR)Acl + Acl->AclSize)) {
                return(FALSE);
            }

            if (!WordAligned(&Ace->AceSize)) {
                return(FALSE);
            }

            if ((PUCHAR)Ace + Ace->AceSize > ((PUCHAR)Acl + Acl->AclSize)) {
                return(FALSE);
            }

            //
            // It is now safe to reference fields in the ACE header.
            //

            //
            // The ACE header fits into the ACL, if this is a known type of ACE,
            // make sure the SID is within the bounds of the ACE
            //

            if (IsKnownAceType(Ace)) {

                if (!LongAligned(Ace->AceSize)) {
                    return(FALSE);
                }

                if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + sizeof(SID) - sizeof(ULONG)) {
                    return(FALSE);
                }

                //
                // It's now safe to reference the parts of the SID structure, though
                // not the SID itself.
                //

                Sid = (PISID) & (((PKNOWN_ACE)Ace)->SidStart);

                if (Sid->Revision != SID_REVISION) {
                    return(FALSE);
                }

                if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                    return(FALSE);
                }

                //
                // SeLengthSid computes the size of the SID based on the subauthority count,
                // so it is safe to use even though we don't know that the body of the SID
                // is safe to reference.
                //

                if (Ace->AceSize < sizeof(KNOWN_ACE) - sizeof(ULONG) + SeLengthSid( Sid )) {
                    return(FALSE);
                }


            //
            // If it's a compound ACE, then perform roughly the same set of tests, but
            // check the validity of both SIDs.
            //

            } else if (IsCompoundAceType(Ace)) {

                //
                // Compound ACEs became valid in revision 3
                //
                if ( Acl->AclRevision < ACL_REVISION3 ) {
                    return FALSE;
                }

                if (!LongAligned(Ace->AceSize)) {
                    return(FALSE);
                }

                if (Ace->AceSize < sizeof(KNOWN_COMPOUND_ACE) - sizeof(ULONG) + sizeof(SID)) {
                    return(FALSE);
                }

                //
                // The only currently defined Compound ACE is an Impersonation ACE.
                //

                if (((PKNOWN_COMPOUND_ACE)Ace)->CompoundAceType != COMPOUND_ACE_IMPERSONATION) {
                    return(FALSE);
                }

                //
                // Examine the first SID and make sure it's structurally valid,
                // and it lies within the boundaries of the ACE.
                //

                Sid = (PISID) & (((PKNOWN_COMPOUND_ACE)Ace)->SidStart);

                if (Sid->Revision != SID_REVISION) {
                    return(FALSE);
                }

                if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                    return(FALSE);
                }

                //
                // Compound ACEs contain two SIDs.  Make sure this ACE is large enough to contain
                // not only the first SID, but the body of the 2nd.
                //

                if (Ace->AceSize < sizeof(KNOWN_COMPOUND_ACE) - sizeof(ULONG) + SeLengthSid( Sid ) + sizeof(SID)) {
                    return(FALSE);
                }

                //
                // It is safe to reference the interior of the 2nd SID.
                //

                Sid2 = (PISID) ((PUCHAR)Sid + SeLengthSid( Sid ));

                if (Sid2->Revision != SID_REVISION) {
                    return(FALSE);
                }

                if (Sid2->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                    return(FALSE);
                }

                if (Ace->AceSize < sizeof(KNOWN_COMPOUND_ACE) - sizeof(ULONG) + SeLengthSid( Sid ) + SeLengthSid( Sid2 )) {
                    return(FALSE);
                }


            //
            // If it's an object ACE, then perform roughly the same set of tests.
            //

            } else if (IsObjectAceType(Ace)) {
                ULONG GuidSize=0;

                //
                // Object ACEs became valid in revision 4
                //
                if ( Acl->AclRevision < ACL_REVISION4 ) {
                    return FALSE;
                }

                if (!LongAligned(Ace->AceSize)) {
                    return(FALSE);
                }

                //
                // Ensure there is room for the ACE header.
                //
                if (Ace->AceSize < sizeof(KNOWN_OBJECT_ACE) - sizeof(ULONG)) {
                    return(FALSE);
                }


                //
                // Ensure there is room for the GUIDs and SID header
                //
                if ( RtlObjectAceObjectTypePresent( Ace ) ) {
                    GuidSize += sizeof(GUID);
                }

                if ( RtlObjectAceInheritedObjectTypePresent( Ace ) ) {
                    GuidSize += sizeof(GUID);
                }

                if (Ace->AceSize < sizeof(KNOWN_OBJECT_ACE) - sizeof(ULONG) + GuidSize + sizeof(SID)) {
                    return(FALSE);
                }

                //
                // It's now safe to reference the parts of the SID structure, though
                // not the SID itself.
                //

                Sid = (PISID) RtlObjectAceSid( Ace );

                if (Sid->Revision != SID_REVISION) {
                    return(FALSE);
                }

                if (Sid->SubAuthorityCount > SID_MAX_SUB_AUTHORITIES) {
                    return(FALSE);
                }

                if (Ace->AceSize < sizeof(KNOWN_OBJECT_ACE) - sizeof(ULONG) + GuidSize + SeLengthSid( Sid ) ) {
                    return(FALSE);
                }
            }

            //
            //  And move Ace to the next ace position
            //

            Ace = ((PVOID)((PUCHAR)(Ace) + ((PACE_HEADER)(Ace))->AceSize));
        }

        return(TRUE);

    } except(EXCEPTION_EXECUTE_HANDLER) {

        return FALSE;
    }

}


NTSTATUS
RtlQueryInformationAcl (
    IN PACL Acl,
    OUT PVOID AclInformation,
    IN ULONG AclInformationLength,
    IN ACL_INFORMATION_CLASS AclInformationClass
    )

/*++

Routine Description:

    This routine returns to the caller information about an ACL.  The requested
    information can be AclRevisionInformation, or AclSizeInformation.

Arguments:

    Acl - Supplies the Acl being examined

    AclInformation - Supplies the buffer to receive the information being
        requested

    AclInformationLength - Supplies the length of the AclInformation buffer
        in bytes

    AclInformationClass - Supplies the type of information being requested

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    PACL_REVISION_INFORMATION RevisionInfo;
    PACL_SIZE_INFORMATION SizeInfo;


    PVOID FirstFree;
    NTSTATUS Status;

    RTL_PAGED_CODE();

    //
    //  Check the ACL revision level
    //

    if (!ValidAclRevision( Acl )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Case on the information class being requested
    //

    switch (AclInformationClass) {

    case AclRevisionInformation:

        //
        //  Make sure the buffer size is correct
        //

        if (AclInformationLength < sizeof(ACL_REVISION_INFORMATION)) {

            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        //  Get the Acl revision and return
        //

        RevisionInfo = (PACL_REVISION_INFORMATION)AclInformation;
        RevisionInfo->AclRevision = Acl->AclRevision;

        break;

    case AclSizeInformation:

        //
        //  Make sure the buffer size is correct
        //

        if (AclInformationLength < sizeof(ACL_SIZE_INFORMATION)) {

            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        //  Locate the first free spot in the Acl
        //

        if (!RtlFirstFreeAce( Acl, &FirstFree )) {

            //
            //  The input Acl is ill-formed
            //

            return STATUS_INVALID_PARAMETER;

        }

        //
        //  Given a pointer to the first free spot we can now easily compute
        //  the number of free bytes and used bytes in the Acl.
        //

        SizeInfo = (PACL_SIZE_INFORMATION)AclInformation;
        SizeInfo->AceCount = Acl->AceCount;

        if (FirstFree == NULL) {

            //
            //  With a null first free we don't have any free space in the Acl
            //

            SizeInfo->AclBytesInUse = Acl->AclSize;

            SizeInfo->AclBytesFree = 0;

        } else {

            //
            //  The first free is not null so we have some free room left in
            //  the acl
            //

            SizeInfo->AclBytesInUse = (ULONG)((PUCHAR)FirstFree - (PUCHAR)Acl);

            SizeInfo->AclBytesFree = Acl->AclSize - SizeInfo->AclBytesInUse;

        }

        break;

    default:

        return STATUS_INVALID_INFO_CLASS;

    }

    //
    //  and return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlSetInformationAcl (
    IN PACL Acl,
    IN PVOID AclInformation,
    IN ULONG AclInformationLength,
    IN ACL_INFORMATION_CLASS AclInformationClass
    )

/*++

Routine Description:

    This routine sets the state of an ACL.  For now only the revision
    level can be set and for now only a revision level of 1 is accepted
    so this procedure is rather simple

Arguments:

    Acl - Supplies the Acl being altered

    AclInformation - Supplies the buffer containing the information being
        set

    AclInformationLength - Supplies the length of the Acl information buffer

    AclInformationClass - Supplies the type of information begin set

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    PACL_REVISION_INFORMATION RevisionInfo;

    RTL_PAGED_CODE();

    //
    //  Check the ACL revision level
    //

    if (!ValidAclRevision( Acl )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Case on the information class being requested
    //

    switch (AclInformationClass) {

    case AclRevisionInformation:

        //
        //  Make sure the buffer size is correct
        //

        if (AclInformationLength < sizeof(ACL_REVISION_INFORMATION)) {

            return STATUS_BUFFER_TOO_SMALL;

        }

        //
        //  Get the Acl requested ACL revision level
        //

        RevisionInfo = (PACL_REVISION_INFORMATION)AclInformation;

        //
        //  Don't let them lower the revision of an ACL.
        //

        if (RevisionInfo->AclRevision < Acl->AclRevision ) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        // Assign the new revision.
        //

        Acl->AclRevision = (UCHAR)RevisionInfo->AclRevision;

        break;

    default:

        return STATUS_INVALID_INFO_CLASS;

    }

    //
    //  and return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlAddAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG StartingAceIndex,
    IN PVOID AceList,
    IN ULONG AceListLength
    )

/*++

Routine Description:

    This routine adds a string of ACEs to an ACL.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    StartingAceIndex - Supplies the ACE index which will be the index of
        the first ace inserted in the acl. 0 for the beginning of the list
        and MAXULONG for the end of the list.

    AceList - Supplies the list of Aces to be added to the Acl

    AceListLength - Supplies the size, in bytes, of the AceList buffer

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful, and an appropriate error
        status otherwise

--*/

{
    PVOID FirstFree;

    PACE_HEADER Ace;
    ULONG NewAceCount;

    PVOID AcePosition;
    ULONG i;
    UCHAR NewRevision;

    RTL_PAGED_CODE();

    //
    //  Check the ACL structure
    //

    if (!RtlValidAcl(Acl)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Locate the first free ace and check to see that the Acl is
    //  well formed.
    //

    if (!RtlFirstFreeAce( Acl, &FirstFree )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    // If the AceRevision is greater than the ACL revision, then we want to
    // increase the ACL revision to be the same as the new ACE revision.
    // We can do this because our previously defined ACE types ( 0 -> 3 ) have
    // not changed structure nor been discontinued in the new revision.  So
    // we can bump the revision and the older types will not be misinterpreted.
    //
    // Compute what the final revision of the ACL is going to be, and save it
    // for later so we can update it once we know we're going to succeed.
    //

    NewRevision = (UCHAR)AceRevision > Acl->AclRevision ? (UCHAR)AceRevision : Acl->AclRevision;

    //
    // Check that the AceList is well formed, we do this by simply zooming
    // down the Ace list until we're equal to or have exceeded the ace list
    // length.  If we are equal to the length then we're well formed otherwise
    // we're ill-formed.  We'll also calculate how many Ace's there are
    // in the AceList
    //
    // In addition, now we have to make sure that we haven't been handed an
    // ACE type that is inappropriate for the AceRevision that was passed
    // in.
    //

    for (Ace = AceList, NewAceCount = 0;
         Ace < (PACE_HEADER)((PUCHAR)AceList + AceListLength);
         Ace = NextAce( Ace ), NewAceCount++) {

        //
        // Ensure the ACL revision allows this ACE type.
        //

        if ( Ace->AceType <= ACCESS_MAX_MS_V2_ACE_TYPE ) {
            // V2 ACE are always valid.
        } else if ( Ace->AceType <= ACCESS_MAX_MS_V3_ACE_TYPE ) {
            if ( AceRevision < ACL_REVISION3 ) {
                return STATUS_INVALID_PARAMETER;
            }
        } else if ( Ace->AceType <= ACCESS_MAX_MS_V4_ACE_TYPE ) {
            if ( AceRevision < ACL_REVISION4 ) {
                return STATUS_INVALID_PARAMETER;
            }
        }
    }

    //
    //  Check to see if we've exceeded the ace list length
    //

    if (Ace > (PACE_HEADER)((PUCHAR)AceList + AceListLength)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Check to see if there is enough room in the Acl to store the additional
    //  Ace list
    //

    if (FirstFree == NULL ||
        (PUCHAR)FirstFree + AceListLength > (PUCHAR)Acl + Acl->AclSize) {

        return STATUS_BUFFER_TOO_SMALL;

    }

    //
    //  All of the input has checked okay, we now need to locate the position
    //  where to insert the new ace list.  We won't check the acl for
    //  validity because we did earlier when got the first free ace position.
    //

    AcePosition = FirstAce( Acl );

    for (i = 0; i < StartingAceIndex && i < Acl->AceCount; i++) {

        AcePosition = NextAce( AcePosition );

    }

    //
    //  Now Ace points to where we want to insert the ace list,  We do the
    //  insertion by adding ace list to the acl and shoving over the remainder
    //  of the list down the acl.  We know this will work because we earlier
    //  check to make sure the new acl list will fit in the acl size
    //

    RtlpAddData( AceList, AceListLength,
             AcePosition, (ULONG) ((PUCHAR)FirstFree - (PUCHAR)AcePosition));

    //
    //  Update the Acl Header
    //

    Acl->AceCount = (USHORT)(Acl->AceCount + NewAceCount);

    Acl->AclRevision = NewRevision;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlDeleteAce (
    IN OUT PACL Acl,
    IN ULONG AceIndex
    )

/*++

Routine Description:

    This routine deletes one ACE from an ACL.

Arguments:

    Acl - Supplies the Acl being modified

    AceIndex - Supplies the index of the Ace to delete.

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    PVOID FirstFree;

    PACE_HEADER Ace;
    ULONG i;

    RTL_PAGED_CODE();

    //
    //  Check the ACL structure
    //

    if (!RtlValidAcl(Acl)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Make sure the AceIndex is within proper range, it's ulong so we know
    //  it can't be negative
    //

    if (AceIndex >= Acl->AceCount) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Locate the first free spot, this will tell us how much data
    //  we'll need to colapse.  If the results is false then the acl is
    //  ill-formed
    //

    if (!RtlFirstFreeAce( Acl, &FirstFree )) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Now locate the ace that we're going to delete.  This loop
    //  doesn't need to check the acl for being well formed.
    //

    Ace = FirstAce( Acl );

    for (i = 0; i < AceIndex; i++) {

        Ace = NextAce( Ace );

    }

    //
    //  We've found the ace to delete to simply copy over the rest of
    //  the acl over this ace.  The delete data procedure also deletes
    //  rest of the string that it's moving over so we don't have to
    //

    RtlpDeleteData( Ace, Ace->AceSize, (ULONG) ((PUCHAR)FirstFree - (PUCHAR)Ace));

    //
    //  Update the Acl header
    //

    Acl->AceCount--;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlGetAce (
    IN PACL Acl,
    ULONG AceIndex,
    OUT PVOID *Ace
    )

/*++

Routine Description:

    This routine returns a pointer to an ACE in an ACl referenced by
    ACE index

Arguments:

    Acl - Supplies the ACL being queried

    AceIndex - Supplies the Ace index to locate

    Ace - Receives the address of the ACE within the ACL

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

--*/

{
    ULONG i;

    RTL_PAGED_CODE();

    //
    //  Check the ACL revision level
    //

    if (!ValidAclRevision(Acl)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  Check the AceIndex against the Ace count of the Acl, it's ulong so
    //  we know it can't be negative
    //

    if (AceIndex >= Acl->AceCount) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  To find the Ace requested by zooming down the Ace List.
    //

    *Ace = FirstAce( Acl );

    for (i = 0; i < AceIndex; i++) {

        //
        //  Check to make sure we haven't overrun the Acl buffer
        //  with our ace pointer.  If we have then our input is bogus
        //

        if (*Ace >= (PVOID)((PUCHAR)Acl + Acl->AclSize)) {

            return STATUS_INVALID_PARAMETER;

        }

        //
        //  And move Ace to the next ace position
        //

        *Ace = NextAce( *Ace );

    }

    //
    //  Now Ace points to the Ace we're after, but make sure we aren't
    //  beyond the Acl.
    //

    if (*Ace >= (PVOID)((PUCHAR)Acl + Acl->AclSize)) {

        return STATUS_INVALID_PARAMETER;

    }

    //
    //  The Ace is still within the Acl so return success to our caller
    //

    return STATUS_SUCCESS;

}


NTSTATUS
RtlAddCompoundAce (
    IN PACL Acl,
    IN ULONG AceRevision,
    IN UCHAR CompoundAceType,
    IN ACCESS_MASK AccessMask,
    IN PSID ServerSid,
    IN PSID ClientSid
    )

/*++

Routine Description:

    This routine adds a KNOWN_COMPOUND_ACE to an ACL.  This is
    expected to be a common form of ACL modification.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    CompoundAceType - Supplies the type of compound ACE being added.
        Currently the only defined type is COMPOUND_ACE_IMPERSONATION.

    AccessMask - The mask of accesses to be granted to the specified SID pair.

    ServerSid - Pointer to the Server SID to be placed in the ACE.

    ClientSid - Pointer to the Client SID to be placed in the ACE.

Return Value:

    NTSTATUS - STATUS_SUCCESS if successful and an appropriate error
        status otherwise

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/




{
    PVOID FirstFree;
    USHORT AceSize;
    PKNOWN_COMPOUND_ACE GrantAce;
    UCHAR NewRevision;

    RTL_PAGED_CODE();

    //
    // Validate the structure of the SID
    //

    if (!RtlValidSid(ServerSid) || !RtlValidSid(ClientSid)) {
        return STATUS_INVALID_SID;
    }

    //
    //  Check the ACL & ACE revision levels
    // Compund ACEs become valid in version 3.
    //

    if ( Acl->AclRevision > ACL_REVISION4 ||
         AceRevision < ACL_REVISION3 ||
         AceRevision > ACL_REVISION4 ) {
        return STATUS_REVISION_MISMATCH;
    }

    //
    // Calculate the new revision of the ACL.  The new revision is the maximum
    // of the old revision and and new ACE's revision.  This is possible because
    // the format of previously defined ACEs did not change across revisions.
    //

    NewRevision = Acl->AclRevision > (UCHAR)AceRevision ? Acl->AclRevision : (UCHAR)AceRevision;

    //
    //  Locate the first free ace and check to see that the Acl is
    //  well formed.
    //

    if (!RtlValidAcl( Acl )) {
        return STATUS_INVALID_ACL;
    }

    if (!RtlFirstFreeAce( Acl, &FirstFree )) {

        return STATUS_INVALID_ACL;
    }

    //
    //  Check to see if there is enough room in the Acl to store the new
    //  ACE
    //

    AceSize = (USHORT)(sizeof(KNOWN_COMPOUND_ACE) -
                       sizeof(ULONG)              +
                       SeLengthSid(ClientSid)    +
                       SeLengthSid(ServerSid)
                       );

    if (  FirstFree == NULL ||
          ((PUCHAR)FirstFree + AceSize > ((PUCHAR)Acl + Acl->AclSize))
       ) {

        return STATUS_ALLOTTED_SPACE_EXCEEDED;
    }

    //
    // Add the ACE to the end of the ACL
    //

    GrantAce = (PKNOWN_COMPOUND_ACE)FirstFree;
    GrantAce->Header.AceFlags = 0;
    GrantAce->Header.AceType = ACCESS_ALLOWED_COMPOUND_ACE_TYPE;
    GrantAce->Header.AceSize = AceSize;
    GrantAce->Mask = AccessMask;
    GrantAce->CompoundAceType = CompoundAceType;
    RtlCopySid( SeLengthSid(ServerSid), (PSID)(&GrantAce->SidStart), ServerSid );
    RtlCopySid( SeLengthSid(ClientSid), (PSID)(((PCHAR)&GrantAce->SidStart) + SeLengthSid(ServerSid)), ClientSid );

    //
    // Increment the number of ACEs by 1.
    //

    Acl->AceCount += 1;

    //
    // Adjust the Acl revision, if necessary
    //

    Acl->AclRevision = NewRevision;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlpAddKnownAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    IN UCHAR NewType
    )

/*++

Routine Description:

    This routine adds KNOWN_ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance and no ACE flags.  The type is specified by the caller.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be denied to the specified SID.

    Sid - Pointer to the SID being denied access.

    NewType - Type of ACE to be added.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    PVOID FirstFree;
    USHORT AceSize;
    PKNOWN_ACE GrantAce;
    UCHAR NewRevision;
    ULONG TestedAceFlags;

    RTL_PAGED_CODE();

    //
    // Validate the structure of the SID
    //

    if (!RtlValidSid(Sid)) {
        return STATUS_INVALID_SID;
    }

    //
    //  Check the ACL & ACE revision levels
    //

    if ( Acl->AclRevision > ACL_REVISION4 || AceRevision > ACL_REVISION4 ) {

        return STATUS_REVISION_MISMATCH;
    }

    //
    // Calculate the new revision of the ACL.  The new revision is the maximum
    // of the old revision and and new ACE's revision.  This is possible because
    // the format of previously defined ACEs did not change across revisions.
    //

    NewRevision = Acl->AclRevision > (UCHAR)AceRevision ? Acl->AclRevision : (UCHAR)AceRevision;

    //
    // Validate the AceFlags.
    //

    TestedAceFlags = AceFlags & ~VALID_INHERIT_FLAGS;
    if ( TestedAceFlags != 0 ) {

        if ( NewType == SYSTEM_AUDIT_ACE_TYPE ) {
            TestedAceFlags &=
                ~(SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG);
        }

        if ( TestedAceFlags != 0 ) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Locate the first free ace and check to see that the Acl is
    //  well formed.
    //

    if (!RtlValidAcl( Acl )) {
        return STATUS_INVALID_ACL;
    }
    if (!RtlFirstFreeAce( Acl, &FirstFree )) {

        return STATUS_INVALID_ACL;
    }

    //
    //  Check to see if there is enough room in the Acl to store the new
    //  ACE
    //

    AceSize = (USHORT)(sizeof(ACE_HEADER) +
                      sizeof(ACCESS_MASK) +
                      SeLengthSid(Sid));

    if (  FirstFree == NULL ||
          ((PUCHAR)FirstFree + AceSize > ((PUCHAR)Acl + Acl->AclSize))
       ) {

        return STATUS_ALLOTTED_SPACE_EXCEEDED;
    }

    //
    // Add the ACE to the end of the ACL
    //

    GrantAce = (PKNOWN_ACE)FirstFree;
    GrantAce->Header.AceFlags = (UCHAR)AceFlags;
    GrantAce->Header.AceType = NewType;
    GrantAce->Header.AceSize = AceSize;
    GrantAce->Mask = AccessMask;
    RtlCopySid( SeLengthSid(Sid), (PSID)(&GrantAce->SidStart), Sid );

    //
    // Increment the number of ACEs by 1.
    //

    Acl->AceCount += 1;

    //
    // Adjust the Acl revision, if necessary
    //

    Acl->AclRevision = NewRevision;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}

NTSTATUS
RtlpAddKnownObjectAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN GUID *ObjectTypeGuid OPTIONAL,
    IN GUID *InheritedObjectTypeGuid OPTIONAL,
    IN PSID Sid,
    IN UCHAR NewType
    )

/*++

Routine Description:

    This routine adds KNOWN_ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance and no ACE flags.  The type is specified by the caller.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be denied to the specified SID.

    ObjectTypeGuid - Supplies the GUID of the object this ACE applies to.
        If NULL, no object type GUID is placed in the ACE.

    InheritedObjectTypeGuid - Supplies the GUID of the object type that will
        inherit this ACE.  If NULL, no inherited object type GUID is placed in
        the ACE.

    Sid - Pointer to the SID being denied access.

    NewType - Type of ACE to be added.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    PVOID FirstFree;
    USHORT AceSize;
    PKNOWN_OBJECT_ACE GrantAce;
    UCHAR NewRevision;
    ULONG TestedAceFlags;
    ULONG AceObjectFlags = 0;
    ULONG SidSize;
    PCHAR Where;

    RTL_PAGED_CODE();

    //
    // Validate the structure of the SID
    //

    if (!RtlValidSid(Sid)) {
        return STATUS_INVALID_SID;
    }

    //
    //  Check the ACL & ACE revision levels
    // Object ACEs became valid in version 4.
    //

    if ( Acl->AclRevision > ACL_REVISION4 || AceRevision != ACL_REVISION4 ) {

        return STATUS_REVISION_MISMATCH;
    }

    //
    // Calculate the new revision of the ACL.  The new revision is the maximum
    // of the old revision and and new ACE's revision.  This is possible because
    // the format of previously defined ACEs did not change across revisions.
    //

    NewRevision = Acl->AclRevision > (UCHAR)AceRevision ? Acl->AclRevision : (UCHAR)AceRevision;

    //
    // Validate the AceFlags.
    //


    TestedAceFlags = AceFlags & ~VALID_INHERIT_FLAGS;
    if ( TestedAceFlags != 0 ) {

        if ( NewType == SYSTEM_AUDIT_ACE_TYPE ||
             NewType == SYSTEM_AUDIT_OBJECT_ACE_TYPE ) {
            TestedAceFlags &=
                ~(SUCCESSFUL_ACCESS_ACE_FLAG|FAILED_ACCESS_ACE_FLAG);
        }

        if ( TestedAceFlags != 0 ) {
            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Locate the first free ace and check to see that the Acl is
    //  well formed.
    //

    if (!RtlValidAcl( Acl )) {
        return STATUS_INVALID_ACL;
    }
    if (!RtlFirstFreeAce( Acl, &FirstFree )) {

        return STATUS_INVALID_ACL;
    }

    //
    //  Check to see if there is enough room in the Acl to store the new
    //  ACE
    //

    SidSize = SeLengthSid(Sid);
    AceSize = (USHORT)(sizeof(ACE_HEADER) +
                      sizeof(ACCESS_MASK) +
                      sizeof(ULONG) +
                      SidSize);

    if ( ARGUMENT_PRESENT(ObjectTypeGuid) ) {
        AceObjectFlags |= ACE_OBJECT_TYPE_PRESENT;
        AceSize += sizeof(GUID);
    }

    if ( ARGUMENT_PRESENT(InheritedObjectTypeGuid) ) {
        AceObjectFlags |= ACE_INHERITED_OBJECT_TYPE_PRESENT;
        AceSize += sizeof(GUID);
    }

    if (  FirstFree == NULL ||
          ((PUCHAR)FirstFree + AceSize > ((PUCHAR)Acl + Acl->AclSize))
       ) {

        return STATUS_ALLOTTED_SPACE_EXCEEDED;
    }

    //
    // Add the ACE to the end of the ACL
    //

    GrantAce = (PKNOWN_OBJECT_ACE)FirstFree;
    GrantAce->Header.AceFlags = (UCHAR) AceFlags;
    GrantAce->Header.AceType = NewType;
    GrantAce->Header.AceSize = AceSize;
    GrantAce->Mask = AccessMask;
    GrantAce->Flags = AceObjectFlags;
    Where = (PCHAR) (&GrantAce->SidStart);
    if ( ARGUMENT_PRESENT(ObjectTypeGuid) ) {
        RtlCopyMemory( Where, ObjectTypeGuid, sizeof(GUID) );
        Where += sizeof(GUID);
    }
    if ( ARGUMENT_PRESENT(InheritedObjectTypeGuid) ) {
        RtlCopyMemory( Where, InheritedObjectTypeGuid, sizeof(GUID) );
        Where += sizeof(GUID);
    }
    RtlCopySid( SidSize, (PSID)Where, Sid );
    Where += SidSize;

    //
    // Increment the number of ACEs by 1.
    //

    Acl->AceCount += 1;

    //
    // Adjust the Acl revision, if necessary
    //

    Acl->AclRevision = NewRevision;

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


NTSTATUS
RtlAddAccessAllowedAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )

/*++

Routine Description:

    This routine adds an ACCESS_ALLOWED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance and no ACE flags.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AccessMask - The mask of accesses to be granted to the specified SID.

    Sid - Pointer to the SID being granted access.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

--*/

{
    RTL_PAGED_CODE();

    return RtlpAddKnownAce (
               Acl,
               AceRevision,
               0,   // No inherit flags
               AccessMask,
               Sid,
               ACCESS_ALLOWED_ACE_TYPE
               );
}


NTSTATUS
RtlAddAccessAllowedAceEx (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )

/*++

Routine Description:

    This routine adds an ACCESS_ALLOWED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be granted to the specified SID.

    Sid - Pointer to the SID being granted access.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    RTL_PAGED_CODE();

    return RtlpAddKnownAce (
               Acl,
               AceRevision,
               AceFlags,
               AccessMask,
               Sid,
               ACCESS_ALLOWED_ACE_TYPE
               );
}


NTSTATUS
RtlAddAccessDeniedAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )

/*++

Routine Description:

    This routine adds an ACCESS_DENIED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance and no ACE flags.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AccessMask - The mask of accesses to be denied to the specified SID.

    Sid - Pointer to the SID being denied access.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

--*/

{
    RTL_PAGED_CODE();

    return RtlpAddKnownAce (
               Acl,
               AceRevision,
               0,   // No inherit flags
               AccessMask,
               Sid,
               ACCESS_DENIED_ACE_TYPE
               );

}


NTSTATUS
RtlAddAccessDeniedAceEx (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid
    )

/*++

Routine Description:

    This routine adds an ACCESS_DENIED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be denied to the specified SID.

    Sid - Pointer to the SID being denied access.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    RTL_PAGED_CODE();

    return RtlpAddKnownAce (
               Acl,
               AceRevision,
               AceFlags,
               AccessMask,
               Sid,
               ACCESS_DENIED_ACE_TYPE
               );

}


NTSTATUS
RtlAddAuditAccessAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    IN BOOLEAN AuditSuccess,
    IN BOOLEAN AuditFailure
    )

/*++

Routine Description:

    This routine adds a SYSTEM_AUDIT ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance.

    Parameters are used to indicate whether auditing is to be performed
    on success, failure, or both.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AccessMask - The mask of accesses to be denied to the specified SID.

    Sid - Pointer to the SID to be audited.

    AuditSuccess - If TRUE, indicates successful access attempts are to be
        audited.

    AuditFailure - If TRUE, indicated failed access attempts are to be
        audited.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

--*/

{
    ULONG AceFlags = 0;
    RTL_PAGED_CODE();

    if (AuditSuccess) {
        AceFlags |= SUCCESSFUL_ACCESS_ACE_FLAG;
    }
    if (AuditFailure) {
        AceFlags |= FAILED_ACCESS_ACE_FLAG;
    }

    return RtlpAddKnownAce (
                Acl,
                AceRevision,
                AceFlags,
                AccessMask,
                Sid,
                SYSTEM_AUDIT_ACE_TYPE );

}

NTSTATUS
RtlAddAuditAccessAceEx (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN PSID Sid,
    IN BOOLEAN AuditSuccess,
    IN BOOLEAN AuditFailure
    )

/*++

Routine Description:

    This routine adds a SYSTEM_AUDIT ACE to an ACL.  This is
    expected to be a common form of ACL modification.

    A very bland ACE header is placed in the ACE.  It provides no
    inheritance.

    Parameters are used to indicate whether auditing is to be performed
    on success, failure, or both.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be denied to the specified SID.

    Sid - Pointer to the SID to be audited.

    AuditSuccess - If TRUE, indicates successful access attempts are to be
        audited.

    AuditFailure - If TRUE, indicated failed access attempts are to be
        audited.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    RTL_PAGED_CODE();

    if (AuditSuccess) {
        AceFlags |= SUCCESSFUL_ACCESS_ACE_FLAG;
    }
    if (AuditFailure) {
        AceFlags |= FAILED_ACCESS_ACE_FLAG;
    }

    return RtlpAddKnownAce (
                Acl,
                AceRevision,
                AceFlags,
                AccessMask,
                Sid,
                SYSTEM_AUDIT_ACE_TYPE );

}


NTSTATUS
RtlAddAccessAllowedObjectAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN GUID *ObjectTypeGuid OPTIONAL,
    IN GUID *InheritedObjectTypeGuid OPTIONAL,
    IN PSID Sid
    )

/*++

Routine Description:

    This routine adds an object specific ACCESS_ALLOWED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be granted to the specified SID.

    ObjectTypeGuid - Supplies the GUID of the object this ACE applies to.
        If NULL, no object type GUID is placed in the ACE.

    InheritedObjectTypeGuid - Supplies the GUID of the object type that will
        inherit this ACE.  If NULL, no inherited object type GUID is placed in
        the ACE.

    Sid - Pointer to the SID being granted access.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    RTL_PAGED_CODE();

    //
    // If no object types are specified,
    //  build a non-object ACE.
    //
    if (ObjectTypeGuid == NULL && InheritedObjectTypeGuid == NULL ) {
        return RtlpAddKnownAce (
                   Acl,
                   AceRevision,
                   AceFlags,
                   AccessMask,
                   Sid,
                   ACCESS_ALLOWED_ACE_TYPE
                   );
    }

    return RtlpAddKnownObjectAce (
               Acl,
               AceRevision,
               AceFlags,
               AccessMask,
               ObjectTypeGuid,
               InheritedObjectTypeGuid,
               Sid,
               ACCESS_ALLOWED_OBJECT_ACE_TYPE
               );
}


NTSTATUS
RtlAddAccessDeniedObjectAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN GUID *ObjectTypeGuid OPTIONAL,
    IN GUID *InheritedObjectTypeGuid OPTIONAL,
    IN PSID Sid
    )

/*++

Routine Description:

    This routine adds an object specific ACCESS_DENIED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be granted to the specified SID.

    ObjectTypeGuid - Supplies the GUID of the object this ACE applies to.
        If NULL, no object type GUID is placed in the ACE.

    InheritedObjectTypeGuid - Supplies the GUID of the object type that will
        inherit this ACE.  If NULL, no inherited object type GUID is placed in
        the ACE.

    Sid - Pointer to the SID being denied access.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    RTL_PAGED_CODE();

    //
    // If no object types are specified,
    //  build a non-object ACE.
    //
    if (ObjectTypeGuid == NULL && InheritedObjectTypeGuid == NULL ) {
        return RtlpAddKnownAce (
                   Acl,
                   AceRevision,
                   AceFlags,
                   AccessMask,
                   Sid,
                   ACCESS_DENIED_ACE_TYPE
                   );
    }

    return RtlpAddKnownObjectAce (
               Acl,
               AceRevision,
               AceFlags,
               AccessMask,
               ObjectTypeGuid,
               InheritedObjectTypeGuid,
               Sid,
               ACCESS_DENIED_OBJECT_ACE_TYPE
               );
}


NTSTATUS
RtlAddAuditAccessObjectAce (
    IN OUT PACL Acl,
    IN ULONG AceRevision,
    IN ULONG AceFlags,
    IN ACCESS_MASK AccessMask,
    IN GUID *ObjectTypeGuid OPTIONAL,
    IN GUID *InheritedObjectTypeGuid OPTIONAL,
    IN PSID Sid,
    IN BOOLEAN AuditSuccess,
    IN BOOLEAN AuditFailure
    )

/*++

Routine Description:

    This routine adds an object specific ACCESS_DENIED ACE to an ACL.  This is
    expected to be a common form of ACL modification.

Arguments:

    Acl - Supplies the Acl being modified

    AceRevision - Supplies the Acl/Ace revision of the ACE being added

    AceFlags - Supplies the inherit flags for the ACE.

    AccessMask - The mask of accesses to be granted to the specified SID.

    ObjectTypeGuid - Supplies the GUID of the object this ACE applies to.
        If NULL, no object type GUID is placed in the ACE.

    InheritedObjectTypeGuid - Supplies the GUID of the object type that will
        inherit this ACE.  If NULL, no inherited object type GUID is placed in
        the ACE.

    Sid - Pointer to the SID to be audited.

    AuditSuccess - If TRUE, indicates successful access attempts are to be
        audited.

    AuditFailure - If TRUE, indicated failed access attempts are to be
        audited.

Return Value:

    STATUS_SUCCESS - The ACE was successfully added.

    STATUS_INVALID_ACL - The specified ACL is not properly formed.

    STATUS_REVISION_MISMATCH - The specified revision is not known
        or is incompatible with that of the ACL.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The new ACE does not fit into the
        ACL.  A larger ACL buffer is required.

    STATUS_INVALID_SID - The provided SID is not a structurally valid
        SID.

    STATUS_INVALID_PARAMETER - The AceFlags parameter was invalid.

--*/

{
    RTL_PAGED_CODE();

    if (AuditSuccess) {
        AceFlags |= SUCCESSFUL_ACCESS_ACE_FLAG;
    }
    if (AuditFailure) {
        AceFlags |= FAILED_ACCESS_ACE_FLAG;
    }

    //
    // If no object types are specified,
    //  build a non-object ACE.
    //
    if (ObjectTypeGuid == NULL && InheritedObjectTypeGuid == NULL ) {
        return RtlpAddKnownAce (
                   Acl,
                   AceRevision,
                   AceFlags,
                   AccessMask,
                   Sid,
                   SYSTEM_AUDIT_ACE_TYPE
                   );
    }

    return RtlpAddKnownObjectAce (
               Acl,
               AceRevision,
               AceFlags,
               AccessMask,
               ObjectTypeGuid,
               InheritedObjectTypeGuid,
               Sid,
               SYSTEM_AUDIT_OBJECT_ACE_TYPE
               );
}

#if 0

NTSTATUS
RtlMakePosixAcl(
    IN ULONG AclRevision,
    IN PSID UserSid,
    IN PSID GroupSid,
    IN ACCESS_MASK UserAccess,
    IN ACCESS_MASK GroupAccess,
    IN ACCESS_MASK OtherAccess,
    IN ULONG AclLength,
    OUT PACL Acl,
    OUT PULONG ReturnLength
    )
/*++

Routine Description:

    NOTE: THIS ROUTINE IS STILL BEING SPEC'D.

    Make an ACL representing Posix protection from AccessMask and
    security account ID (SID) information.

Arguments:

    AclRevision - Indicates the ACL revision level of the access masks
        provided.  The ACL generated will be revision compatible with this
        value and will not be a higher revision than this value.

    UserSid - Provides the SID of the user (owner).

    GroupSid - Provides the SID of the primary group.

    UserAccess - Specifies the accesses to be given to the user (owner).

    GroupAccess - Specifies the accesses to be given to the primary group.

    OtherAccess - Specifies the accesses to be given to others (WORLD).

    AclLength - Provides the length (in bytes) of the Acl buffer.

    Acl - Points to a buffer to receive the generated ACL.

    ReturnLength - Returns the actual length needed to store the resultant
        ACL.  If this length is greater than that specified in AclLength,
        then STATUS_BUFFER_TOO_SMALL is returned and no ACL is generated.

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_UNKNOWN_REVISION - The revision level specified is not supported
        by this service.

    STATUS_BUFFER_TOO_SMALL - Indicates the length of the output buffer
        wasn't large enough to hold the generated ACL.  The length needed
        is returned via the ReturnLength parameter.

--*/

{

    SID_IDENTIFIER_AUTHORITY WorldSidAuthority = SECURITY_WORLD_SID_AUTHORITY;

    ULONG UserSidLength;
    ULONG GroupSidLength;
    ULONG WorldSidLength;
    ULONG RequiredAclSize;
    ULONG AceSize;
    ULONG CurrentAce;
    PACCESS_ALLOWED_ACE Ace;
    NTSTATUS Status;

    RTL_PAGED_CODE();

    if (!RtlValidSid( UserSid ) || !RtlValidSid( GroupSid )) {
        return( STATUS_INVALID_SID );
    }

    UserSidLength = SeLengthSid( UserSid );
    GroupSidLength = SeLengthSid( GroupSid );
    WorldSidLength = RtlLengthRequiredSid( 1 );

    //
    // Figure out how much room we need for an ACL and three
    // ACCESS_ALLOWED Ace's
    //

    RequiredAclSize = sizeof( ACL );

    AceSize = sizeof( ACCESS_ALLOWED_ACE ) - sizeof( ULONG );

    RequiredAclSize += (AceSize * 3)  +
                       UserSidLength  +
                       GroupSidLength +
                       WorldSidLength ;

    if (RequiredAclSize > AclLength) {
        *ReturnLength = RequiredAclSize;
        return( STATUS_BUFFER_TOO_SMALL );
    }

    //
    // The passed buffer is big enough, build the ACL in it.
    //

    Status = RtlCreateAcl(
                 Acl,
                 RequiredAclSize,
                 AclRevision
                 );

    if (!NT_SUCCESS( Status )) {
        return( Status );
    }

    CurrentAce = (ULONG)Acl + sizeof( ACL );
    Ace = (PACCESS_ALLOWED_ACE)CurrentAce;

    //
    // Build the user (owner) ACE
    //

    Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceSize = (USHORT)(UserSidLength + AceSize);
    Ace->Header.AceFlags = 0;

    Ace->Mask = UserAccess;

    RtlMoveMemory(
        (PVOID)(Ace->SidStart),
        UserSid,
        UserSidLength
        );

    CurrentAce += (ULONG)(Ace->Header.AceSize);
    Ace = (PACCESS_ALLOWED_ACE)CurrentAce;

    //
    // Build the group ACE
    //

    Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceSize = (USHORT)(GroupSidLength + AceSize);
    Ace->Header.AceFlags = 0;

    Ace->Mask = GroupAccess;

    RtlMoveMemory(
        (PVOID)(Ace->SidStart),
        GroupSid,
        GroupSidLength
        );

    CurrentAce += (ULONG)(Ace->Header.AceSize);
    Ace = (PACCESS_ALLOWED_ACE)CurrentAce;

    //
    // Build the World ACE
    //

    Ace->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    Ace->Header.AceSize = (USHORT)(GroupSidLength + AceSize);
    Ace->Header.AceFlags = 0;

    Ace->Mask = OtherAccess;

    RtlInitializeSid(
        (PSID)(Ace->SidStart),
        &WorldSidAuthority,
        1
        );

    *(RtlSubAuthoritySid((PSID)(Ace->SidStart), 0 )) = SECURITY_WORLD_RID;

    return( STATUS_SUCCESS );

}

NTSTATUS
RtlInterpretPosixAcl(
    IN ULONG AclRevision,
    IN PSID UserSid,
    IN PSID GroupSid,
    IN PACL Acl,
    OUT PACCESS_MASK UserAccess,
    OUT PACCESS_MASK GroupAccess,
    OUT PACCESS_MASK OtherAccess
    )
/*++

Routine Description:

    NOTE: THIS ROUTINE IS STILL BEING SPEC'D.

    Interpret an ACL representing Posix protection, returning AccessMasks.
    Use security account IDs (SIDs) for object owner and primary group
    identification.

    This algorithm will pick up the first match of a given SID and ignore
    all further matches of that SID.  The first unrecognized SID becomes
    the "other" SID.

Arguments:

    AclRevision - Indicates the ACL revision level of the access masks to
        be returned.

    UserSid - Provides the SID of the user (owner).

    GroupSid - Provides the SID of the primary group.

    Acl - Points to a buffer containing the ACL to interpret.

    UserAccess - Receives the accesses allowed for the user (owner).

    GroupAccess - Receives the accesses allowed for the primary group.

    OtherAccess - Receives the accesses allowed for others (WORLD).

Return Values:

    STATUS_SUCCESS - The service completed successfully.

    STATUS_UNKNOWN_REVISION - The revision level specified is not supported
        by this service.

    STATUS_EXTRENEOUS_INFORMATION - This warning status value indicates the
        ACL contained protection or other information unrelated to Posix
        style protection.  This is a warning only.  The interpretation was
        otherwise successful and all access masks were returned.

    STATUS_COULD_NOT_INTERPRET - Indicates the ACL does not contain
        sufficient Posix style (user/group) protection information.  The
        ACL could not be interpreted.

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN UserFound = FALSE;
    BOOLEAN GroupFound = FALSE;
    BOOLEAN OtherFound = FALSE;
    ULONG i;
    PKNOWN_ACE Ace;

    RTL_PAGED_CODE();

    if (AclRevision != ACL_REVISION2) {
        return( STATUS_UNKNOWN_REVISION );
    }

    if (Acl->AceCount > 3) {
        Status = STATUS_EXTRANEOUS_INFORMATION;
    }

    for (i=0, Ace = FirstAce( Acl );
        (i < Acl->AceCount) && (!UserFound || !GroupFound || !OtherFound);
        i++, Ace = NextAce( Ace )) {

        if (Ace->Header.AceType != ACCESS_ALLOWED_ACE_TYPE) {
            Status = STATUS_EXTRANEOUS_INFORMATION;
            continue;
        }

        if (RtlEqualSid(
               (PSID)(Ace->SidStart),
               UserSid
               ) && !UserFound) {

            *UserAccess = Ace->Mask;
            UserFound = TRUE;
            continue;
        }

        if (RtlEqualSid(
               (PSID)(Ace->SidStart),
               GroupSid
               ) && !GroupFound) {

            *GroupAccess = Ace->Mask;
            GroupFound = TRUE;
            continue;
        }

        //
        // It isn't the user, and it isn't the group, pick it up
        // as "other"
        //

        if (!OtherFound) {
            *OtherAccess = Ace->Mask;
            OtherFound = TRUE;
            continue;
        }

    }

    //
    // Make sure we got everything we need, error otherwise
    //

    if (!UserFound || !GroupFound || !OtherFound) {
        Status = STATUS_COULD_NOT_INTERPRET;
    }

    return( Status );

}

#endif // 0


//
//  Internal support routine
//

BOOLEAN
RtlFirstFreeAce (
    IN PACL Acl,
    OUT PVOID *FirstFree
    )

/*++

Routine Description:

    This routine returns a pointer to the first free byte in an Acl
    or NULL if the acl is ill-formed.  If the Acl is full then the
    return pointer is to the byte immediately following the acl, and
    TRUE will be returned.

Arguments:

    Acl - Supplies a pointer to the Acl to examine

    FirstFree - Receives a pointer to the first free position in the Acl

Return Value:

    BOOLEAN - TRUE if the Acl is well formed and FALSE otherwise

--*/

{
    PACE_HEADER Ace;
    ULONG i;

    RTL_PAGED_CODE();

    //
    //  To find the first free spot in the Acl we need to search for
    //  the last ace.  We do this by zooming down the list until
    //  we've exhausted the ace count or the ace size (which ever comes
    //  first).  In the following loop Ace points to the next spot
    //  for an Ace and I is the ace index
    //

    *FirstFree = NULL;

    for ( i=0, Ace = FirstAce( Acl );
          i < Acl->AceCount;
          i++, Ace = NextAce( Ace )) {

        //
        //  Check to make sure we haven't overrun the Acl buffer
        //  with our Ace pointer.  If we have then our input is bogus.
        //

        if (Ace >= (PACE_HEADER)((PUCHAR)Acl + Acl->AclSize)) {

            return FALSE;

        }

    }

    //
    //  Now Ace points to the first free spot in the Acl so set the
    //  output variable and check to make sure it is still in the Acl
    //  or just one beyond the end of the acl (i.e., the acl is full).
    //

    if (Ace <= (PACE_HEADER)((PUCHAR)Acl + Acl->AclSize)) {

        *FirstFree = Ace;
    }

    //
    //  The Acl is well formed so return the first free spot we've found
    //  (or NULL if there is no free space for another ACE)
    //

    return TRUE;

}


//
//  Internal support routine
//

VOID
RtlpAddData (
    IN PVOID From,
    IN ULONG FromSize,
    IN PVOID To,
    IN ULONG ToSize
    )

/*++

Routine Description:

    This routine copies data to a string of bytes.  It does this by moving
    over data in the to string so that the from string will fit.  It also
    assumes that the checks that the data will fit in memory have already
    been done.  Pictorally the results are as follows.

    Before:

        From -> ffffffffff

        To   -> tttttttttttttttt

    After:

        From -> ffffffffff

        To   -> fffffffffftttttttttttttttt

Arguments:

    From - Supplies a pointer to the source buffer

    FromSize - Supplies the size of the from buffer in bytes

    To - Supplies a pointer to the destination buffer

    ToSize - Supplies the size of the to buffer in bytes

Return Value:

    None

--*/

{
    LONG i;

    //
    //  Shift over the To buffer enough to fit in the From buffer
    //

    for (i = ToSize - 1; i >= 0; i--) {

        ((PUCHAR)To)[i+FromSize] = ((PUCHAR)To)[i];
    }

    //
    //  Now copy over the From buffer
    //

    for (i = 0; (ULONG)i < FromSize; i += 1) {

        ((PUCHAR)To)[i] = ((PUCHAR)From)[i];

    }

    //
    //  and return to our caller
    //

    return;

}


//
//  Internal support routine
//

VOID
RtlpDeleteData (
    IN PVOID Data,
    IN ULONG RemoveSize,
    IN ULONG TotalSize
    )

/*++

Routine Description:

    This routine deletes a string of bytes from the front of a data buffer
    and compresses the data.  It also zeros out the part of the string
    that is no longer in use.  Pictorially the results are as follows

    Before:

        Data       = DDDDDddddd
        RemoveSize = 5
        TotalSize  = 10

    After:

        Data      = ddddd00000

Arguments:

    Data - Supplies a pointer to the data being altered

    RemoveSize - Supplies the number of bytes to delete from the front
        of the data buffer

    TotalSize - Supplies the total number of bytes in the data buffer
        before the delete operation

Return Value:

    None

--*/

{
    ULONG i;

    //
    //  Shift over the buffer to remove the amount
    //

    for (i = RemoveSize; i < TotalSize; i++) {

        ((PUCHAR)Data)[i-RemoveSize] = ((PUCHAR)Data)[i];

    }

    //
    //  Now as a safety precaution we'll zero out the rest of the string
    //

    for (i = TotalSize - RemoveSize; i < TotalSize; i++) {

        ((PUCHAR)Data)[i] = 0;
    }

    //
    //  And return to our caller
    //

    return;

}
