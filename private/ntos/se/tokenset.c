/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    Tokenset.c

Abstract:

    This module implements the SET function for the executive
    token object.

Author:

    Jim Kelly (JimK) 15-June-1990


Revision History:

--*/

#include "sep.h"
#include "sertlp.h"
#include "tokenp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtSetInformationToken)
#pragma alloc_text(PAGE,SepFreePrimaryGroup)
#pragma alloc_text(PAGE,SepFreeDefaultDacl)
#pragma alloc_text(PAGE,SepAppendPrimaryGroup)
#pragma alloc_text(PAGE,SepAppendDefaultDacl)
#endif


NTSTATUS
NtSetInformationToken (
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    IN PVOID TokenInformation,
    IN ULONG TokenInformationLength
    )

/*++


Routine Description:

    Modify information in a specified token.

Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    TokenInformationClass - The token information class being set.

    TokenInformation - The buffer containing the new values for the
        specified class of information.  The buffer must be aligned
        on at least a longword boundary.  The actual structures
        provided are dependent upon the information class specified,
        as defined in the TokenInformationClass parameter
        description.

        TokenInformation Format By Information Class:

           TokenUser => This value is not a valid value for this API.
           The User ID may not be replaced.

           TokenGroups => This value is not a valid value for this
           API.  The Group IDs may not be replaced.  However, groups
           may be enabled and disabled using NtAdjustGroupsToken().

           TokenPrivileges => This value is not a valid value for
           this API.  Privilege information may not be replaced.
           However, privileges may be explicitly enabled and disabled
           using the NtAdjustPrivilegesToken API.

           TokenOwner => TOKEN_OWNER data structure.
           TOKEN_ADJUST_DEFAULT access is needed to replace this
           information in a token.  The owner values that may be
           specified are restricted to the user and group IDs with an
           attribute indicating they may be assigned as the owner of
           objects.

           TokenPrimaryGroup => TOKEN_PRIMARY_GROUP data structure.
           TOKEN_ADJUST_DEFAULT access is needed to replace this
           information in a token.  The primary group values that may
           be specified are restricted to be one of the group IDs
           already in the token.

           TokenDefaultDacl => TOKEN_DEFAULT_DACL data structure.
           TOKEN_ADJUST_DEFAULT access is needed to replace this
           information in a token.  The ACL provided as a new default
           discretionary ACL is not validated for structural
           correctness or consistency.

           TokenSource => This value is not a valid value for this
           API.  The source name and context handle  may not be
           replaced.

           TokenStatistics => This value is not a valid value for this
           API.  The statistics of a token are read-only.

    TokenInformationLength - Indicates the length, in bytes, of the
        TokenInformation buffer.  This is only the length of the primary
        buffer.  All extensions of the primary buffer are self describing.

Return Value:

    STATUS_SUCCESS - The operation was successful.

    STATUS_INVALID_OWNER - The ID specified to be an owner (or
        default owner) is not one the caller may assign as the owner
        of an object.

    STATUS_INVALID_INFO_CLASS - The specified information class is
        not one that may be specified in this API.

    STATUS_ALLOTTED_SPACE_EXCEEDED - The space allotted for storage
        of the default discretionary access control and the primary
        group ID is not large enough to accept the new value of one
        of these fields.

--*/
{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PTOKEN Token;

    ULONG Index;
    BOOLEAN Found;
    BOOLEAN TokenModified = FALSE;

    ULONG NewLength;
    ULONG CurrentLength;

    PSID CapturedOwner;
    PSID CapturedPrimaryGroup;
    PACL CapturedDefaultDacl;
    ACCESS_MASK DesiredAccess;

    PAGED_CODE();

    //
    // Get previous processor mode and probe input buffer if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {

            //
            // This just probes the main part of the information buffer.
            // Any information class-specific data hung off the primary
            // buffer are self describing and must be probed separately
            // below.
            //

            ProbeForRead(
                TokenInformation,
                TokenInformationLength,
                sizeof(ULONG)
                );

        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Return error if not legal class
    //
    if ( (TokenInformationClass != TokenOwner)  &&
         (TokenInformationClass != TokenPrimaryGroup) &&
         (TokenInformationClass != TokenSessionId) &&
         (TokenInformationClass != TokenDefaultDacl) ) {

        return STATUS_INVALID_INFO_CLASS;

    }

    //
    // Check access rights and reference token
    //


    DesiredAccess = TOKEN_ADJUST_DEFAULT;
    if (TokenInformationClass == TokenSessionId) {
        DesiredAccess |= TOKEN_ADJUST_SESSIONID;
    }

    Status = ObReferenceObjectByHandle(
             TokenHandle,           // Handle
             DesiredAccess,         // DesiredAccess
             SepTokenObjectType,    // ObjectType
             PreviousMode,          // AccessMode
             (PVOID *)&Token,       // Object
             NULL                   // GrantedAccess
             );

    if ( !NT_SUCCESS(Status) ) {
        return Status;
    }


    //
    // Case on information class.
    //

    switch ( TokenInformationClass ) {

    case TokenOwner:

        //
        //  Make sure the buffer is large enough to hold the
        //  necessary information class data structure.
        //

        if (TokenInformationLength < (ULONG)sizeof(TOKEN_OWNER)) {

            ObDereferenceObject( Token );
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        //  Capture and copy

        try {

            //
            //  Capture Owner SID
            //

            CapturedOwner = ((PTOKEN_OWNER)TokenInformation)->Owner;
            Status = SeCaptureSid(
                         CapturedOwner,
                         PreviousMode,
                         NULL, 0,
                         PagedPool,
                         TRUE,
                         &CapturedOwner
                         );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if (!NT_SUCCESS(Status)) {
            ObDereferenceObject( Token );
            return Status;
        }

        //
        //  Gain write access to the token.
        //

        SepAcquireTokenWriteLock( Token );

        //
        //  Walk through the list of user and group IDs looking
        //  for a match to the specified SID.  If one is found,
        //  make sure it may be assigned as an owner.  If it can,
        //  then set the index in the token's OwnerIndex field.
        //  Otherwise, return invalid owner error.
        //

        Index = 0;
        while (Index < Token->UserAndGroupCount) {

            try {

                Found = RtlEqualSid(
                            CapturedOwner,
                            Token->UserAndGroups[Index].Sid
                            );

                if ( Found ) {

                    if ( SepIdAssignableAsOwner(Token,Index) ){

                        Token->DefaultOwnerIndex = Index;
                        TokenModified = TRUE;
                        Status = STATUS_SUCCESS;

                    } else {

                        Status = STATUS_INVALID_OWNER;

                    } //endif assignable

                    SepReleaseTokenWriteLock( Token, TokenModified );
                    ObDereferenceObject( Token );
                    SeReleaseSid( CapturedOwner, PreviousMode, TRUE);
                    return Status;

                }  //endif Found

            } except(EXCEPTION_EXECUTE_HANDLER) {

                SepReleaseTokenWriteLock( Token, TokenModified );
                ObDereferenceObject( Token );
                SeReleaseSid( CapturedOwner, PreviousMode, TRUE);
                return GetExceptionCode();

            }  //endtry

            Index += 1;

        } //endwhile

        SepReleaseTokenWriteLock( Token, TokenModified );
        ObDereferenceObject( Token );
        SeReleaseSid( CapturedOwner, PreviousMode, TRUE);
        return STATUS_INVALID_OWNER;

    case TokenPrimaryGroup:

        //
        // Assuming everything works out, the strategy is to move everything
        // in the Dynamic part of the token (exept the primary group) to
        // the beginning of the dynamic part, freeing up the entire end of
        // the dynamic part for the new primary group.
        //

        //
        //  Make sure the buffer is large enough to hold the
        //  necessary information class data structure.
        //

        if (TokenInformationLength < (ULONG)sizeof(TOKEN_PRIMARY_GROUP)) {

            ObDereferenceObject( Token );
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Capture And Validate TOKEN_PRIMARY_GROUP and corresponding SID.
        //

        try {

            CapturedPrimaryGroup =
                ((PTOKEN_PRIMARY_GROUP)TokenInformation)->PrimaryGroup;

            Status = SeCaptureSid(
                         CapturedPrimaryGroup,
                         PreviousMode,
                         NULL, 0,
                         PagedPool,
                         TRUE,
                         &CapturedPrimaryGroup
                         );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if (!NT_SUCCESS(Status)) {
            ObDereferenceObject( Token );
            return Status;
        }

        if (!SepIdAssignableAsGroup( Token, CapturedPrimaryGroup )) {
            ObDereferenceObject( Token );
            SeReleaseSid( CapturedPrimaryGroup, PreviousMode, TRUE);
            return STATUS_INVALID_PRIMARY_GROUP;
        }

        //
        //  Gain write access to the token.
        //

        SepAcquireTokenWriteLock( Token );

        //
        // See if there is enough room in the dynamic part of the token
        // to replace the current Primary Group with the one specified.
        //

        NewLength = SeLengthSid( CapturedPrimaryGroup );
        CurrentLength = SeLengthSid( Token->PrimaryGroup );

        if (NewLength > (CurrentLength + Token->DynamicAvailable) ) {

            SepReleaseTokenWriteLock( Token, TokenModified );
            ObDereferenceObject( Token );
            SeReleaseSid( CapturedPrimaryGroup, PreviousMode, TRUE);
            return STATUS_ALLOTTED_SPACE_EXCEEDED;
        }

        //
        // Free up the existing primary group
        //

        SepFreePrimaryGroup( Token );

        //
        // And put the new SID in its place
        //

        SepAppendPrimaryGroup( Token, CapturedPrimaryGroup );

        TokenModified = TRUE;

        //
        // All done.
        //

        SepReleaseTokenWriteLock( Token, TokenModified );
        ObDereferenceObject( Token );
        SeReleaseSid( CapturedPrimaryGroup, PreviousMode, TRUE);
        return STATUS_SUCCESS;


    case TokenDefaultDacl:

        //
        // Assuming everything works out, the strategy is to move everything
        // in the Dynamic part of the token (exept the default Dacl) to
        // the beginning of the dynamic part, freeing up the entire end of
        // the dynamic part for the new default Dacl.
        //

        //
        //  Make sure the buffer is large enough to hold the
        //  necessary information class data structure.
        //

        if (TokenInformationLength < (ULONG)sizeof(TOKEN_DEFAULT_DACL)) {

            ObDereferenceObject( Token );
            return STATUS_INFO_LENGTH_MISMATCH;
        }

        //
        // Capture And Validate TOKEN_DEFAULT_DACL and corresponding ACL.
        //

        try {

            CapturedDefaultDacl =
                ((PTOKEN_DEFAULT_DACL)TokenInformation)->DefaultDacl;

            if (ARGUMENT_PRESENT(CapturedDefaultDacl)) {
                Status = SeCaptureAcl(
                             CapturedDefaultDacl,
                             PreviousMode,
                             NULL, 0,
                             PagedPool,
                             TRUE,
                             &CapturedDefaultDacl,
                             &NewLength
                             );

            } else {
                NewLength = 0;
                Status = STATUS_SUCCESS;
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        if (!NT_SUCCESS(Status)) {
            ObDereferenceObject( Token );
            return Status;
        }

        //
        //  Gain write access to the token.
        //

        SepAcquireTokenWriteLock( Token );

        //
        // See if there is enough room in the dynamic part of the token
        // to replace the current Default Dacl with the one specified.
        //

        if (ARGUMENT_PRESENT(Token->DefaultDacl)) {
            CurrentLength = Token->DefaultDacl->AclSize;
        } else {
            CurrentLength = 0;
        }

        if (NewLength > (CurrentLength + Token->DynamicAvailable) ) {

            SepReleaseTokenWriteLock( Token, TokenModified );
            ObDereferenceObject( Token );
            if (ARGUMENT_PRESENT(CapturedDefaultDacl)) {
                SeReleaseAcl( CapturedDefaultDacl, PreviousMode, TRUE);
            }
            return STATUS_ALLOTTED_SPACE_EXCEEDED;
        }

        //
        // Free up the existing Default Dacl
        //

        SepFreeDefaultDacl( Token );

        //
        // And put the new ACL in its place
        //

        if (ARGUMENT_PRESENT(CapturedDefaultDacl)) {
            SepAppendDefaultDacl( Token, CapturedDefaultDacl );
        }

        TokenModified = TRUE;

        //
        // All done.
        //

        SepReleaseTokenWriteLock( Token, TokenModified );
        ObDereferenceObject( Token );
        if (ARGUMENT_PRESENT(CapturedDefaultDacl)) {
            SeReleaseAcl( CapturedDefaultDacl, PreviousMode, TRUE);
        }
        return STATUS_SUCCESS;

    case TokenSessionId:
    {
       ULONG SessionId;

        if ( TokenInformationLength != sizeof(ULONG) ) {
            ObDereferenceObject( Token );
            return( STATUS_INFO_LENGTH_MISMATCH );
        }

        try {

           SessionId = *(PULONG)TokenInformation;

        } except(EXCEPTION_EXECUTE_HANDLER) {
            ObDereferenceObject( Token );
            return GetExceptionCode();
        }

        //
        // We only allow TCB to set SessionId's
        //
        if ( !SeSinglePrivilegeCheck(SeTcbPrivilege,PreviousMode) ) {
            ObDereferenceObject( Token );
            return( STATUS_PRIVILEGE_NOT_HELD );
        }

        //
        // Set SessionId for the token
        //
        SeSetSessionIdToken( (PACCESS_TOKEN)Token,
                             SessionId );

        ObDereferenceObject( Token );
        return( STATUS_SUCCESS );
    }

    } //endswitch

    ASSERT( TRUE == FALSE );  // Should never reach here.
    return( STATUS_INVALID_PARAMETER );

}


VOID
SepFreePrimaryGroup(
    IN PTOKEN Token
    )

/*++


Routine Description:

    Free up the space in the dynamic part of the token take up by the primary
    group.

    The token is assumed to be locked for write access before calling
    this routine.

Arguments:

    Token - Pointer to the token.

Return Value:

    None.

--*/
{
    PAGED_CODE();

    //
    // Add the size of the primary group to the DynamicAvailable field.
    //

    Token->DynamicAvailable += SeLengthSid( Token->PrimaryGroup );

    //
    // If there is a default discretionary ACL, and it is not already at the
    // beginning of the dynamic part, move it there (remember to update the
    // pointer to it).
    //

    if (ARGUMENT_PRESENT(Token->DefaultDacl)) {
        if (Token->DynamicPart != (PULONG)(Token->DefaultDacl)) {

            RtlMoveMemory(
                (PVOID)(Token->DynamicPart),
                (PVOID)(Token->DefaultDacl),
                Token->DefaultDacl->AclSize
                );

            Token->DefaultDacl = (PACL)(Token->DynamicPart);

        }
    }

    return;

}


VOID
SepFreeDefaultDacl(
    IN PTOKEN Token
    )

/*++


Routine Description:

    Free up the space in the dynamic part of the token take up by the default
    discretionary access control list.

    The token is assumed to be locked for write access before calling
    this routine.

Arguments:

    Token - Pointer to the token.

Return Value:

    None.

--*/
{
   ULONG PrimaryGroupSize;

   PAGED_CODE();

    //
    // Add the size of the Default Dacl (if there is one) to the
    // DynamicAvailable field.
    //
    if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

        Token->DynamicAvailable += Token->DefaultDacl->AclSize;
        Token->DefaultDacl = NULL;

    }

    //
    // If it is not already at the beginning of the dynamic part, move
    // the primary group there (remember to update the pointer to it).
    //

    if (Token->DynamicPart != (PULONG)(Token->PrimaryGroup)) {

        PrimaryGroupSize = SeLengthSid( Token->PrimaryGroup );

        RtlMoveMemory(
            (PVOID)(Token->DynamicPart),
            (PVOID)(Token->PrimaryGroup),
            PrimaryGroupSize
            );

        Token->PrimaryGroup = (PSID)(Token->DynamicPart);
    }

    return;
}

VOID
SepAppendPrimaryGroup(
    IN PTOKEN Token,
    IN PSID PSid
    )

/*++


Routine Description:

    Add a primary group SID to the available space at the end of the dynamic
    part of the token.  It is the caller's responsibility to ensure that the
    primary group SID fits within the available space of the dynamic part of
    the token.

    The token is assumed to be locked for write access before calling
    this routine.

Arguments:

    Token - Pointer to the token.

    PSid - Pointer to the SID to add.

Return Value:

    None.

--*/
{
   ULONG_PTR NextFree;
   ULONG SidSize;

   PAGED_CODE();

    //
    // Add the size of the Default Dacl (if there is one) to the
    // address of the Dynamic Part of the token to establish
    // where the primary group should be placed.
    //

    if (ARGUMENT_PRESENT(Token->DefaultDacl)) {

//        ASSERT( (ULONG)(Token->DefaultDacl->AclSize) ==
//                (ULONG)LongAlignSize(Token->DefaultDacl->AclSize) );

        NextFree = (ULONG_PTR)(Token->DynamicPart) + Token->DefaultDacl->AclSize;

    } else {

        NextFree = (ULONG_PTR)(Token->DynamicPart);

    }

    //
    // Now copy the primary group SID.
    //


    SidSize = SeLengthSid( PSid );

    RtlCopyMemory(
        (PVOID)NextFree,
        (PVOID)PSid,
        SidSize
        );

    Token->PrimaryGroup = (PSID)NextFree;

    //
    // And decrement the amount of the dynamic part that is available.
    //

    ASSERT( SidSize <= (Token->DynamicAvailable) );
    Token->DynamicAvailable -= SidSize;

    return;

}

VOID
SepAppendDefaultDacl(
    IN PTOKEN Token,
    IN PACL PAcl
    )

/*++


Routine Description:

    Add a default discretionary ACL to the available space at the end of the
    dynamic part of the token.  It is the caller's responsibility to ensure
    that the default Dacl fits within the available space of the dynamic
    part of the token.

    The token is assumed to be locked for write access before calling
    this routine.

Arguments:

    Token - Pointer to the token.

    PAcl - Pointer to the ACL to add.

Return Value:

    None.

--*/
{
   ULONG_PTR NextFree;
   ULONG AclSize;

   PAGED_CODE();

    //
    // Add the size of the primary group to the
    // address of the Dynamic Part of the token to establish
    // where the primary group should be placed.
    //

    ASSERT(ARGUMENT_PRESENT(Token->PrimaryGroup));

    NextFree = (ULONG_PTR)(Token->DynamicPart) + SeLengthSid(Token->PrimaryGroup);

    //
    // Now copy the default Dacl
    //

    AclSize = (ULONG)(PAcl->AclSize);
//    ASSERT(AclSize == (ULONG)LongAlignSize(AclSize));

    RtlCopyMemory(
        (PVOID)NextFree,
        (PVOID)PAcl,
        AclSize
        );

    Token->DefaultDacl = (PACL)NextFree;

    //
    // And decrement the amount of the dynamic part that is available.
    //

    ASSERT( AclSize <= (Token->DynamicAvailable) );
    Token->DynamicAvailable -= AclSize;

    return;

}


NTSTATUS
SeSetSessionIdToken(
    PACCESS_TOKEN Token,
    ULONG SessionId
    )
/*++


Routine Description:

    Sets the SessionId for the specified token object.

Arguments:

    pOpaqueToken (input)
      Opaque kernel Token access pointer

    SessionId (input)
      SessionId to store in token

Return Value:

    STATUS_SUCCESS - no error

--*/
{


   //
   //  Gain write access to the token.
   //

   SepAcquireTokenWriteLock( Token );

   ((PTOKEN)Token)->SessionId = SessionId;

   SepReleaseTokenWriteLock( Token, TRUE );

   return( STATUS_SUCCESS );
}
