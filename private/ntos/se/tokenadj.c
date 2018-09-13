/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    tokenadj.c

Abstract:

   This module implements the services that perform individual adjustments
   on token objects.

Author:

    Jim Kelly (JimK) 15-June-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "sep.h"
#include "sertlp.h"
#include "tokenp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtAdjustPrivilegesToken)
#pragma alloc_text(PAGE,NtAdjustGroupsToken)
#pragma alloc_text(PAGE,SepAdjustPrivileges)
#pragma alloc_text(PAGE,SepAdjustGroups)
#endif


////////////////////////////////////////////////////////////////////////
//                                                                    //
//           Token Object Routines & Methods                          //
//                                                                    //
////////////////////////////////////////////////////////////////////////


NTSTATUS
NtAdjustPrivilegesToken (
    IN HANDLE TokenHandle,
    IN BOOLEAN DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState OPTIONAL,
    IN ULONG BufferLength OPTIONAL,
    OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
    OUT PULONG ReturnLength
    )


/*++


Routine Description:

    This routine is used to disable or enable privileges in the
    specified token.  The absence of some of the privileges listed to
    be changed won't effect the successful modification of the
    privileges that are in the token.  The previous enabled/disabled
    state of changed privileges may optionally be capture (for
    resetting later).

    TOKEN_ADJUST_PRIVILEGES access is required to enable or disable
    privileges in a token.


Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    DisableAllPrivileges - This boolean parameter may be
        used to disable all privileges assigned to the token.  If
        this parameter is specified as TRUE, then the NewState parameter is
        ignored.

    NewState - This (optional) parameter points to a TOKEN_PRIVILEGES
        data structure containing the privileges whose states are to
        be adjusted (disabled or enabled).  Only the Enabled flag of
        the attributes associated with each privilege is used.  It
        provides the new value that is to be assigned to the
        privilege in the token.

    BufferLength - This optional parameter indicates the length (in
        bytes) of the PreviousState buffer.  This value must be
        provided if the PreviousState parameter is provided.

    PreviousState - This (optional) parameter points to a buffer to
        receive the state of any privileges actually changed by this
        request.  This information is formated as a TOKEN_PRIVILEGES
        data structure which may be passed as the NewState parameter
        in a subsequent call to this routine to restore the original
        state of those privilges.  TOKEN_QUERY access is needed to
        use this parameter.

        If this buffer does not contain enough space to receive the
        complete list of modified privileges, then no privilege
        states are changed and STATUS_BUFFER_TOO_SMALL is returned.
        In this case, the ReturnLength OUT parameter will
        contain the actual number of bytes needed to hold the
        information.

    ReturnLength - Indicates the actual number of bytes needed to
        contain the previous privilege state information.
        This parameter is ignored if the PreviousState argument is not
        passed.

Return Value:

    STATUS_SUCCESS - The service successfully completed the requested
        operation.

    STATUS_NOT_ALL_ASSIGNED - This NT_SUCCESS severity return status
        indicates that not all the specified privileges are currently
        assigned to the caller.  All specified privileges that are
        currently assigned have been successfully adjusted.

    STATUS_BUFFER_TOO_SMALL - Indicates the optional buffer provided
        to receive the previous states of changed privileges wasn't
        large enough to receive that information.  No changes to
        privilege states has been made.  The number of bytes needed
        to hold the state change information is returned via the
        ReturnLength parameter.

    STATUS_INVALID_PARAMETER - Indicates neither the DisableAllPrivileges
        parameter was specified as true, nor was an explicit NewState
        provided.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PTOKEN Token;

    ACCESS_MASK DesiredAccess;

    ULONG CapturedPrivilegeCount;
    PLUID_AND_ATTRIBUTES CapturedPrivileges = NULL;
    ULONG CapturedPrivilegesLength;

    ULONG LocalReturnLength;
    ULONG ChangeCount;
    BOOLEAN ChangesMade;

    ULONG ParameterLength;

    PAGED_CODE();

    //
    //  The semantics of the PreviousState parameter leads to a two-pass
    //  approach to adjusting privileges.  The first pass simply checks
    //  to see which privileges will change and counts them.  This allows
    //  the amount of space needed to be calculated and returned.  If
    //  the caller's PreviousState return buffer is not large enough, then
    //  an error is returned without making any modifications.  Otherwise,
    //  a second pass is made to actually make the changes.
    //
    //

    if (!DisableAllPrivileges && !ARGUMENT_PRESENT(NewState)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get previous processor mode and probe parameters if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {

            //
            // Make sure we can see all of the new state
            //

            if (!DisableAllPrivileges) {

                ProbeForRead(
                    NewState,
                    sizeof(TOKEN_PRIVILEGES),
                    sizeof(ULONG)
                    );

                CapturedPrivilegeCount = NewState->PrivilegeCount;
                ParameterLength = (ULONG)sizeof(TOKEN_PRIVILEGES) +
                                  ( (CapturedPrivilegeCount - ANYSIZE_ARRAY) *
                                  (ULONG)sizeof(LUID_AND_ATTRIBUTES)  );

                ProbeForRead(
                    NewState,
                    ParameterLength,
                    sizeof(ULONG)
                    );

            }


            //
            // Check the PreviousState buffer for writeability
            //

            if (ARGUMENT_PRESENT(PreviousState)) {

                ProbeForWrite(
                    PreviousState,
                    BufferLength,
                    sizeof(ULONG)
                    );

                ProbeForWriteUlong(ReturnLength);
            }


        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }

    } else {

        if (!DisableAllPrivileges) {

            CapturedPrivilegeCount = NewState->PrivilegeCount;
        }
    }



    //
    // Capture NewState if passed.
    //

    if (!DisableAllPrivileges) {

        try {


            Status = SeCaptureLuidAndAttributesArray(
                         (NewState->Privileges),
                         CapturedPrivilegeCount,
                         PreviousMode,
                         NULL, 0,
                         PagedPool,
                         TRUE,
                         &CapturedPrivileges,
                         &CapturedPrivilegesLength
                         );

        } except(EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();

        }

        if (!NT_SUCCESS(Status)) {

            return Status;

        }

    }


    //
    // Reference the token object and validate the caller's right
    // to adjust the privileges.
    //

    if (ARGUMENT_PRESENT(PreviousState)) {
        DesiredAccess = (TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY);
    } else {
        DesiredAccess = TOKEN_ADJUST_PRIVILEGES;
    }

    Status = ObReferenceObjectByHandle(
             TokenHandle,             // Handle
             DesiredAccess,           // DesiredAccess
             SepTokenObjectType,      // ObjectType
             PreviousMode,            // AccessMode
             (PVOID *)&Token,         // Object
             NULL                     // GrantedAccess
             );

    if ( !NT_SUCCESS(Status) ) {

        if (CapturedPrivileges != NULL) {
            SeReleaseLuidAndAttributesArray(
                CapturedPrivileges,
                PreviousMode,
                TRUE
                );
        }

        return Status;
    }

    //
    //  Gain exclusive access to the token.
    //

    SepAcquireTokenWriteLock( Token );

    //
    // First pass through the privileges list - just count the changes
    //


    Status = SepAdjustPrivileges(
                Token,
                FALSE,                // Don't make changes this pass
                DisableAllPrivileges,
                CapturedPrivilegeCount,
                CapturedPrivileges,
                PreviousState,
                &LocalReturnLength,
                &ChangeCount,
                &ChangesMade
                );

    if (ARGUMENT_PRESENT(PreviousState)) {

        try {

            (*ReturnLength) = LocalReturnLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenWriteLock( Token, FALSE );
            ObDereferenceObject( Token );

            if (CapturedPrivileges != NULL) {
                SeReleaseLuidAndAttributesArray(
                    CapturedPrivileges,
                    PreviousMode,
                    TRUE
                    );
            }

            return GetExceptionCode();
        }

    }


    //
    // Make sure there is enough room to return any  requested
    // information.
    //

    if (ARGUMENT_PRESENT(PreviousState)) {
        if (LocalReturnLength > BufferLength) {

            SepReleaseTokenWriteLock( Token, FALSE );
            ObDereferenceObject( Token );

            if (CapturedPrivileges != NULL) {
                SeReleaseLuidAndAttributesArray(
                    CapturedPrivileges,
                    PreviousMode,
                    TRUE
                    );
            }

            return STATUS_BUFFER_TOO_SMALL;
        }
    }

    //
    // Second pass through the privileges list - Make the changes.
    //
    // Note that the internal routine attempts to write the previous
    // state directly to the caller's buffer - and so may get an exception.
    //

    try {

        Status = SepAdjustPrivileges(
                    Token,
                    TRUE,                 // Make the changes this pass
                    DisableAllPrivileges,
                    CapturedPrivilegeCount,
                    CapturedPrivileges,
                    PreviousState,
                    &LocalReturnLength,
                    &ChangeCount,
                    &ChangesMade
                    );


        if (ARGUMENT_PRESENT(PreviousState)) {

            PreviousState->PrivilegeCount = ChangeCount;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        SepReleaseTokenWriteLock( Token, TRUE );
        ObDereferenceObject( Token );
        if (CapturedPrivileges != NULL) {
            SeReleaseLuidAndAttributesArray(
                CapturedPrivileges,
                PreviousMode,
                TRUE
                );
        }
        return GetExceptionCode();

    }


    SepReleaseTokenWriteLock( Token, ChangesMade );
    ObDereferenceObject( Token );
    if (CapturedPrivileges != NULL) {
        SeReleaseLuidAndAttributesArray(
            CapturedPrivileges,
            PreviousMode,
            TRUE
            );
    }

    return Status;

}


NTSTATUS
NtAdjustGroupsToken (
    IN HANDLE TokenHandle,
    IN BOOLEAN ResetToDefault,
    IN PTOKEN_GROUPS NewState OPTIONAL,
    IN ULONG BufferLength OPTIONAL,
    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
    OUT PULONG ReturnLength
    )

/*++


Routine Description:

    This routine is used to disable or enable groups in the specified
    token.  The absence of some of the groups listed to be changed
    won't effect the successful modification of the groups that are in
    the token.  The previous enabled/disabled state of changed groups
    may optionally be capture (for resetting later).

    TOKEN_ADJUST_GROUPS access is required to enable or disable groups
    in a token

    Note that mandatory groups can not be disabled.  An attempt
    disable any mandatory groups will cause the call to fail, leaving
    the state of all groups unchanged.


Arguments:

    TokenHandle - Provides a handle to the token to operate on.

    ResetToDefault - The parameter indicates whether all the groups
        in the token are to be reset to their default enabled/disabled
        state.

    NewState - This parameter points to a TOKEN_GROUPS data structure
        containing the groups whose states are to be adjusted
        (disabled or enabled).  Only the Enabled flag of the
        attributes associated with each group is used.  It provides
        the new value that is to be assigned to the group in the
        token.  If the ResetToDefault argument is specified as TRUE,
        then this argument is ignored.  Otherwise, it must be passed.

    BufferLength - This optional parameter indicates the length (in
        bytes) of the PreviousState buffer.  This value must be
        provided if the PreviousState parameter is provided.

    PreviousState - This (optional) parameter points to a buffer to
        receive the state of any groups actually changed by this
        request.  This information is formated as a TOKEN_GROUPS data
        structure which may be passed as the NewState parameter in a
        subsequent call to NtAdjustGroups to restore the original state
        of those groups.  TOKEN_QUERY access is needed to use this
        parameter.

        If this buffer does not contain enough space to receive the
        complete list of modified groups, then no group states are
        changed and STATUS_BUFFER_TOO_SMALL is returned.  In this
        case, the ReturnLength return parameter will contain the
        actual number of bytes needed to hold the information.

    ReturnLength - Indicates the actual number of bytes needed to
        contain the previous group state information.
        This parameter is ignored if the PreviousState argument is not
        passed.


Return Value:

    STATUS_SUCCESS - The service successfully completed the requested
        operation.

    STATUS_NOT_ALL_ASSIGNED - This NT_SUCCESS severity return status
        indicates that not all the specified groups are currently
        assigned to the caller.  All specified groups that are
        currently assigned have been successfully adjusted.

    STATUS_CANT_DISABLE_MANDATORY - Indicates an attempt was made to
        disable a mandatory group.  The states of all groups remains
        unchanged.

    STATUS_BUFFER_TOO_SMALL - Indicates the optional buffer provided
        to receive the previous states of changed group wasn't large
        enough to receive that information.  No changes to group
        states has been made.  The number of bytes needed to hold the
        state change information is returned via the ReturnLength
        parameter.

    STATUS_INVALID_PARAMETER - Indicates neither the ResetToDefault
        parameter was specified as true, nor was an explicit NewState
        provided.

--*/
{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PTOKEN Token;

    ACCESS_MASK DesiredAccess;

    ULONG CapturedGroupCount;
    PSID_AND_ATTRIBUTES CapturedGroups = NULL;
    ULONG CapturedGroupsLength;

    ULONG LocalReturnLength;
    ULONG ChangeCount;
    BOOLEAN ChangesMade;
    PSID SidBuffer;

    PAGED_CODE();

    //
    //  The semantics of the PreviousState parameter and the
    //  STATUS_CANT_DISABLE_MANDATORY completion status leads to a two-pass
    //  approach to adjusting groups.  The first pass simply checks
    //  to see which groups will change and counts them.  This allows
    //  the amount of space needed to be calculated and returned.  If
    //  the caller's PreviousState return buffer is not large enough, or
    //  one of the specified groups is a mandatory group, then an error
    //  is returned without making any modifications.  Otherwise, a second
    //  pass is made to actually make the changes.
    //

    if (!ResetToDefault && !ARGUMENT_PRESENT(NewState)) {
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Get previous processor mode and probe parameters if necessary.
    //

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {
        try {

            if (!ResetToDefault) {
                ProbeForRead(
                    NewState,
                    sizeof(TOKEN_GROUPS),
                    sizeof(ULONG)
                    );
            }

            if (ARGUMENT_PRESENT(PreviousState)) {

                ProbeForWrite(
                    PreviousState,
                    BufferLength,
                    sizeof(ULONG)
                    );

                //
                // This parameter is only used if PreviousState
                // is present
                //

                ProbeForWriteUlong(ReturnLength);

            }


        } except(EXCEPTION_EXECUTE_HANDLER) {
            return GetExceptionCode();
        }
    }

    //
    // Capture NewState.
    //

    if (!ResetToDefault) {

        try {

            CapturedGroupCount = NewState->GroupCount;
            Status = SeCaptureSidAndAttributesArray(
                         &(NewState->Groups[0]),
                         CapturedGroupCount,
                         PreviousMode,
                         NULL, 0,
                         PagedPool,
                         TRUE,
                         &CapturedGroups,
                         &CapturedGroupsLength
                         );

            if (!NT_SUCCESS(Status)) {

                return Status;

            }

        } except(EXCEPTION_EXECUTE_HANDLER) {

            return GetExceptionCode();

        } // endtry
    } // endif !ResetToDefault


    //
    // Reference the token object and validate the caller's right
    // to adjust the groups.
    //

    if (ARGUMENT_PRESENT(PreviousState)) {
        DesiredAccess = (TOKEN_ADJUST_GROUPS | TOKEN_QUERY);
    } else {
        DesiredAccess = TOKEN_ADJUST_GROUPS;
    }

    Status = ObReferenceObjectByHandle(
             TokenHandle,             // Handle
             DesiredAccess,           // DesiredAccess
             SepTokenObjectType,      // ObjectType
             PreviousMode,            // AccessMode
             (PVOID *)&Token,         // Object
             NULL                     // GrantedAccess
             );

    if ( !NT_SUCCESS(Status) ) {

        if (ARGUMENT_PRESENT(CapturedGroups)) {
            SeReleaseSidAndAttributesArray( CapturedGroups, PreviousMode, TRUE );
        }

        return Status;
    }

    //
    //  Gain exclusive access to the token.
    //

    SepAcquireTokenWriteLock( Token );

    //
    // First pass through the groups list.
    //
    // This pass is always necessary for groups to make sure the caller
    // isn't trying to do anything illegal to mandatory groups.
    //

    Status = SepAdjustGroups(
                 Token,
                 FALSE,                // Don't make changes this pass
                 ResetToDefault,
                 CapturedGroupCount,
                 CapturedGroups,
                 PreviousState,
                 NULL,                // Not returning SIDs this call
                 &LocalReturnLength,
                 &ChangeCount,
                 &ChangesMade
                 );

    if (ARGUMENT_PRESENT(PreviousState)) {

        try {

            (*ReturnLength) = LocalReturnLength;

        } except(EXCEPTION_EXECUTE_HANDLER) {

            SepReleaseTokenWriteLock( Token, FALSE );
            ObDereferenceObject( Token );

            if (ARGUMENT_PRESENT(CapturedGroups)) {
                SeReleaseSidAndAttributesArray(
                    CapturedGroups,
                    PreviousMode,
                    TRUE
                    );
            }

            return GetExceptionCode();
        }
    }

    //
    // Make sure we didn't encounter an error
    //

    if (!NT_SUCCESS(Status)) {

        SepReleaseTokenWriteLock( Token, FALSE );
        ObDereferenceObject( Token );

        if (ARGUMENT_PRESENT(CapturedGroups)) {
            SeReleaseSidAndAttributesArray(
                CapturedGroups,
                PreviousMode,
                TRUE
                );
        }

        return Status;

    }

    //
    // Make sure there is enough room to return requested information.
    // Also go on to calculate where the SID values go.
    //

    if (ARGUMENT_PRESENT(PreviousState)) {
        if (LocalReturnLength > BufferLength) {

            SepReleaseTokenWriteLock( Token, FALSE );
            ObDereferenceObject( Token );

            if (ARGUMENT_PRESENT(CapturedGroups)) {
                SeReleaseSidAndAttributesArray(
                    CapturedGroups,
                    PreviousMode,
                    TRUE
                    );
            }


            return STATUS_BUFFER_TOO_SMALL;
        }

        //
        // Calculate where the SIDs can be placed in the PreviousState
        // buffer.
        //

        SidBuffer = (PSID)(LongAlignPtr(
                            (PCHAR)PreviousState + (ULONG)sizeof(TOKEN_GROUPS) +
                            (ChangeCount * (ULONG)sizeof(SID_AND_ATTRIBUTES)) -
                            (ANYSIZE_ARRAY * (ULONG)sizeof(SID_AND_ATTRIBUTES))
                            ) );

    }

    //
    // Second pass through the groups list.
    //

    try {

        Status = SepAdjustGroups(
                     Token,
                     TRUE,                 // Make changes in this pass
                     ResetToDefault,
                     CapturedGroupCount,
                     CapturedGroups,
                     PreviousState,
                     SidBuffer,
                     &LocalReturnLength,
                     &ChangeCount,
                     &ChangesMade
                     );

        if (ARGUMENT_PRESENT(PreviousState)) {

            PreviousState->GroupCount = ChangeCount;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {

        //SepFreeToken( Token, TRUE );
        SepReleaseTokenWriteLock( Token, TRUE );
        ObDereferenceObject( Token );
        if (ARGUMENT_PRESENT(CapturedGroups)) {
            SeReleaseSidAndAttributesArray( CapturedGroups, PreviousMode, TRUE );
        }
        return GetExceptionCode();

    }

    //SepFreeToken( Token, ChangesMade );
    SepReleaseTokenWriteLock( Token, ChangesMade );
    ObDereferenceObject( Token );

    if (ARGUMENT_PRESENT(CapturedGroups)) {
        SeReleaseSidAndAttributesArray( CapturedGroups, PreviousMode, TRUE );
    }

    return Status;

}

NTSTATUS
SepAdjustPrivileges(
    IN PTOKEN Token,
    IN BOOLEAN MakeChanges,
    IN BOOLEAN DisableAllPrivileges,
    IN ULONG PrivilegeCount OPTIONAL,
    IN PLUID_AND_ATTRIBUTES NewState OPTIONAL,
    OUT PTOKEN_PRIVILEGES PreviousState OPTIONAL,
    OUT PULONG ReturnLength,
    OUT PULONG ChangeCount,
    OUT PBOOLEAN ChangesMade
    )

/*++


Routine Description:

    This routine is used to walk the privileges array in a token as a
    result of a request to adjust privileges.

    If the MakeChanges parameter is FALSE, this routine simply determines
    what changes are needed and how much space is necessary to save the
    current state of changed privileges.

    If the MakeChanges parameter is TRUE, this routine will not only
    calculate the space necessary to save the current state, but will
    actually make the changes.

    This routine makes the following assumptions:

      1) The token is locked for exclusive access.

      2) The PrivilegeCount and NewState parameters (if passed) are captured
         and accesses to them will not result in access violations.

      4) Any access violations encountered may leave the request
         partially completed.  It is the calling routine's responsibility
         to catch exceptions.

      5) The calling routine is responsible for inrementing the token's
         ModifiedId field.

Arguments:

    Token - Pointer to the token to act upon.

    MakeChanges - A boolean value indicating whether the changes should
        actually be made, or just evaluated.  A value of TRUE indicates
        the changes should be made.

    DisableAllPrivilegs - A boolean value indicating whether all privileges
        are to be disabled, or only select, specified privileges.  A value
        of TRUE indicates all privileges are to be disabled.

    PrivilegeCount - This parameter is required only if the NewState parameter
        is used.  In that case, this parameter indicates how many entries are
        in the NewState parameter.

    NewState - This parameter is ignored if the DisableAllPrivileges
        argument is TRUE.  If the DisableAllPrivileges argument is FALSE,
        then this parameter must be provided and specifies the new state
        to set privileges to (enabled or disabled).

    PreviousState - This (optional) parameter points to a buffer to
        receive the state of any privileges actually changed by this
        request.  This information is formated as a TOKEN_PRIVILEGES data
        structure which may be passed as the NewState parameter in a
        subsequent call to NtAdjustPrivileges to restore the original state
        of those privileges.  It is the caller's responsibility to make
        sure this buffer is large enough to receive all the state
        information.

    ReturnLength - Points to a buffer to receive the number of bytes needed
        to retrieve the previous state information of changed privileges.
        This parameter is ignored if the PreviousState argument is not
        passed.

    ChangeCount - Points to a ULONG to receive the number of privileges
        which were adjusted (or would be adjusted, if changes are made).

    ChangesMade - Points to a boolean flag which is to receive an indication
        as to whether any changes were made as a result of this call.  This
        is expected to be used to decide whether or not to increment the
        token's ModifiedId field.

Return Value:

    STATUS_SUCCESS - Call completed sccessfully.

    STATUS_NOT_ALL_ASSIGNED - Indicates not all the specified adjustments
        have been made (or could be made, if update wasn't requested).

--*/
{
    NTSTATUS CompletionStatus = STATUS_SUCCESS;

    ULONG OldIndex;
    ULONG NewIndex;
    BOOLEAN Found;
    ULONG MatchCount = 0;

    LUID_AND_ATTRIBUTES CurrentPrivilege;

    PAGED_CODE();

    //
    //  Walk through the privileges array to determine which need to be
    //  adjusted.
    //

    OldIndex = 0;
    (*ChangeCount) = 0;

    while (OldIndex < Token->PrivilegeCount) {

        CurrentPrivilege = (Token->Privileges)[OldIndex];

        if (DisableAllPrivileges) {

            if (SepTokenPrivilegeAttributes(Token,OldIndex) &
               SE_PRIVILEGE_ENABLED ) {

                //
                // Change, if necessary (saving previous state if
                // appropriate).
                //

                if (MakeChanges) {

                    if (ARGUMENT_PRESENT(PreviousState)) {

                        PreviousState->Privileges[(*ChangeCount)] =
                            CurrentPrivilege;
                    }

                    SepTokenPrivilegeAttributes(Token,OldIndex) &=
                        ~SE_PRIVILEGE_ENABLED;



                } //endif make changes

                //
                // increment the number of changes
                //

                (*ChangeCount) += 1;

            } // endif privilege enabled

        } else {

            //
            //  Selective adjustments - this is a little trickier
            //  Compare the current privilege to each of those in
            //  the NewState array.  If a match is found, then adjust
            //  the current privilege appropriately.
            //

            NewIndex = 0;
            Found = FALSE;

            while ( (NewIndex < PrivilegeCount) && !Found)  {

                //
                // Look for a comparison
                //

                if (RtlEqualLuid(&CurrentPrivilege.Luid,&NewState[NewIndex].Luid)) {

                    Found = TRUE;
                    MatchCount += 1;

                    if ( (SepArrayPrivilegeAttributes( NewState, NewIndex ) &
                          SE_PRIVILEGE_ENABLED)
                        !=
                         (SepTokenPrivilegeAttributes(Token,OldIndex) &
                          SE_PRIVILEGE_ENABLED)  ) {

                        //
                        // Change, if necessary (saving previous state if
                        // appropriate).
                        //

                        if (MakeChanges) {

                            if (ARGUMENT_PRESENT(PreviousState)) {

                                PreviousState->Privileges[(*ChangeCount)] =
                                    CurrentPrivilege;
                            }

                            SepTokenPrivilegeAttributes(Token,OldIndex) &=
                                ~(SepTokenPrivilegeAttributes(Token,OldIndex)
                                  & SE_PRIVILEGE_ENABLED);
                            SepTokenPrivilegeAttributes(Token,OldIndex) |=
                                 (SepArrayPrivilegeAttributes(NewState,NewIndex)
                                  & SE_PRIVILEGE_ENABLED);

                            //
                            // if this is SeChangeNotifyPrivilege, then
                            // change its corresponding bit in TokenFlags
                            //

                            if (RtlEqualLuid(&CurrentPrivilege.Luid,
                                              &SeChangeNotifyPrivilege)) {
                                Token->TokenFlags ^= TOKEN_HAS_TRAVERSE_PRIVILEGE;
                            }



                        } //endif make changes

                        //
                        // increment the number of changes
                        //

                        (*ChangeCount) += 1;


                    } // endif change needed

                } // endif found

                NewIndex += 1;

            } // endwhile searching NewState

        } // endelse

        OldIndex += 1;

    } // endwhile privileges in token

    //
    // If we disabled all privileges, then clear the TokenFlags flag
    // corresponding to the SeChangeNotifyPrivilege privilege.
    //


    if (DisableAllPrivileges) {
        Token->TokenFlags &= ~TOKEN_HAS_TRAVERSE_PRIVILEGE;
    }

    //
    // Set completion status appropriately if some not assigned
    //

    if (!DisableAllPrivileges) {

        if (MatchCount < PrivilegeCount) {
            CompletionStatus = STATUS_NOT_ALL_ASSIGNED;
        }
    }

    //
    //  Indicate whether changes were made
    //

    if ((*ChangeCount) > 0  &&  MakeChanges) {
        (*ChangesMade) = TRUE;
    } else {
        (*ChangesMade) = FALSE;
    }

    //
    // Calculate the space needed to return previous state information
    //

    if (ARGUMENT_PRESENT(PreviousState)) {

        (*ReturnLength) = (ULONG)sizeof(TOKEN_PRIVILEGES) +
                          ((*ChangeCount) *  (ULONG)sizeof(LUID_AND_ATTRIBUTES)) -
                          (ANYSIZE_ARRAY * (ULONG)sizeof(LUID_AND_ATTRIBUTES));
    }

   return CompletionStatus;
}

NTSTATUS
SepAdjustGroups(
    IN PTOKEN Token,
    IN BOOLEAN MakeChanges,
    IN BOOLEAN ResetToDefault,
    IN ULONG GroupCount,
    IN PSID_AND_ATTRIBUTES NewState OPTIONAL,
    OUT PTOKEN_GROUPS PreviousState OPTIONAL,
    OUT PSID SidBuffer OPTIONAL,
    OUT PULONG ReturnLength,
    OUT PULONG ChangeCount,
    OUT PBOOLEAN ChangesMade
    )

/*++


Routine Description:

    This routine is used to walk the groups array in a token as a
    result of a request to adjust groups.

    If the MakeChanges parameter is FALSE, this routine simply determines
    what changes are needed and how much space is necessary to save the
    current state of changed groups.

    If the MakeChanges parameter is TRUE, this routine will not only
    calculate the space necessary to save the current state, but will
    actually make the changes.

    This routine makes the following assumptions:

      1) The token is locked for exclusive access.

      2) The NewState parameter is captured and accesses
         to it will not result in access violations.

      4) Any access violations encountered may leave the request
         partially completed.  It is the calling routine's responsibility
         to catch exceptions.

      5) The calling routine is responsible for inrementing the token's
         ModifiedId field.

Arguments:

    Token - Pointer to the token to act upon.

    MakeChanges - A boolean value indicating whether the changes should
        actually be made, or just evaluated.  A value of TRUE indicates
        the changes should be made.

    ResetToDefault - Indicates that the groups are to be reset to their
        default enabled/disabled state.

    GroupCount - This parameter is required only if the NewState parameter
        is used.  In that case, this parameter indicates how many entries are
        in the NewState parameter.

    NewState - This parameter points to a SID_AND_ATTRIBUTES array
        containing the groups whose states are to be adjusted
        (disabled or enabled).  Only the Enabled flag of the
        attributes associated with each group is used.  It provides
        the new value that is to be assigned to the group in the
        token.  If the ResetToDefault argument is specified as TRUE,
        then this argument is ignored.  Otherwise, it must be passed.

    PreviousState - This (optional) parameter points to a buffer to
        receive the state of any groups actually changed by this
        request.  This information is formated as a TOKEN_GROUPS data
        structure which may be passed as the NewState parameter in a
        subsequent call to NtAdjustGroups to restore the original state
        of those groups.  It is the caller's responsibility to make
        sure this buffer is large enough to receive all the state
        information.

    SidBuffer - Pointer to buffer to receive the SID values corresponding
        to the groups returned in the PreviousState argument.

    ReturnLength - Points to a buffer to receive the number of bytes needed
        to retrieve the previous state information of changed privileges.
        This parameter is ignored if the PreviousState argument is not
        passed.

    ChangeCount - Points to a ULONG to receive the number of groups
        which were adjusted (or would be adjusted, if changes are made).

    ChangesMade - Points to a boolean flag which is to receive an indication
        as to whether any changes were made as a result of this call.  This
        is expected to be used to decide whether or not to increment the
        token's ModifiedId field.

Return Value:

    STATUS_SUCCESS - Call completed sccessfully.

    STATUS_NOT_ALL_ASSIGNED - Indicates not all the specified adjustments
        have been made (or could be made, if update wasn't requested).

    STATUS_CANT_DISABLE_MANDATORY - Not all adjustments were made (or could
        be made, if update not requested) because an attempt was made to
        disable a mandatory group.  The state of the groups is left
        in an underterministic state if update was requested.


--*/
{

    NTSTATUS CompletionStatus = STATUS_SUCCESS;

    ULONG OldIndex;
    ULONG NewIndex;
    ULONG SidLength;
    ULONG LocalReturnLength = 0;
    PSID NextSid;
    BOOLEAN Found;
    ULONG MatchCount = 0;
    BOOLEAN EnableGroup;
    BOOLEAN DisableGroup;
    ULONG TokenGroupAttributes;

    SID_AND_ATTRIBUTES CurrentGroup;

    PAGED_CODE();

    //
    // NextSid is used to copy group SID values if asked for previous state.
    //

    NextSid = SidBuffer;


    //
    //  Walk through the groups array to determine which need to be
    //  adjusted.
    //

    OldIndex = 1;             // Don't evaluate the 0th entry (user ID)
    (*ChangeCount) = 0;

    while (OldIndex < Token->UserAndGroupCount) {

        CurrentGroup = Token->UserAndGroups[OldIndex];

        if (ResetToDefault) {

            TokenGroupAttributes = SepTokenGroupAttributes(Token,OldIndex);

            //
            // If the group is enabled by default and currently disabled,
            // then we must enable it.
            //

            EnableGroup = (BOOLEAN)( (TokenGroupAttributes & SE_GROUP_ENABLED_BY_DEFAULT)
                && !(TokenGroupAttributes & SE_GROUP_ENABLED));

            //
            // If the group is disabled by default and currently enabled,
            // then we must disable it.
            //

            DisableGroup = (BOOLEAN)( !(TokenGroupAttributes & SE_GROUP_ENABLED_BY_DEFAULT)
                && (TokenGroupAttributes & SE_GROUP_ENABLED));

            //
            // Blow up if it's a mandatory group that is not both
            // enabled by default and enabled (SepCreateToken should
            // make sure that this never happens).
            //

            ASSERT(!(TokenGroupAttributes & SE_GROUP_MANDATORY)
                   || (TokenGroupAttributes & (SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED)
                       == (SE_GROUP_ENABLED_BY_DEFAULT | SE_GROUP_ENABLED)));

            if ( EnableGroup || DisableGroup ) {

                SidLength = SeLengthSid( CurrentGroup.Sid );
                SidLength = (ULONG)LongAlignSize(SidLength);
                LocalReturnLength += SidLength;

                //
                // Change, if necessary (saving previous state if
                // appropriate).
                //

                if (MakeChanges) {

                    if (ARGUMENT_PRESENT(PreviousState)) {

                        (*(PreviousState)).Groups[(*ChangeCount)].Attributes =
                            CurrentGroup.Attributes;

                        (*(PreviousState)).Groups[(*ChangeCount)].Sid =
                            NextSid;

                        RtlCopySid( SidLength, NextSid, CurrentGroup.Sid );
                        NextSid = (PSID)((ULONG_PTR)NextSid + SidLength);
                    }

                    if (EnableGroup) {
                        SepTokenGroupAttributes(Token,OldIndex) |= SE_GROUP_ENABLED;
                    } else {
                        SepTokenGroupAttributes(Token,OldIndex) &= ~SE_GROUP_ENABLED;
                    }



                } //endif make changes

                //
                // increment the number of changes
                //

                (*ChangeCount) += 1;

            } // endif group enabled

        } else {

            //
            //  Selective adjustments - this is a little trickier
            //  Compare the current group to each of those in
            //  the NewState array.  If a match is found, then adjust
            //  the current group appropriately.
            //

            NewIndex = 0;
            Found = FALSE;

            while ( (NewIndex < GroupCount) && !Found)  {

                //
                // Look for a comparison
                //

                if (RtlEqualSid(
                        CurrentGroup.Sid,
                        NewState[NewIndex].Sid
                        ) ) {

                    Found = TRUE;
                    MatchCount += 1;


                    //
                    // See if it needs to be changed
                    //

                    if ( (SepArrayGroupAttributes( NewState, NewIndex ) &
                            SE_GROUP_ENABLED ) !=
                         (SepTokenGroupAttributes(Token,OldIndex) &
                            SE_GROUP_ENABLED ) ) {

                        //
                        // Make sure group is not mandatory
                        //

                        if (SepTokenGroupAttributes(Token,OldIndex) &
                              SE_GROUP_MANDATORY ) {
                            return STATUS_CANT_DISABLE_MANDATORY;
                        }

                        //
                        // Make sure group is not deny-only
                        //


                        if (SepTokenGroupAttributes(Token,OldIndex) &
                              SE_GROUP_USE_FOR_DENY_ONLY ) {
                            return STATUS_CANT_ENABLE_DENY_ONLY;
                        }

                        SidLength = SeLengthSid( CurrentGroup.Sid );
                        SidLength = (ULONG)LongAlignSize(SidLength);
                        LocalReturnLength += SidLength;

                        //
                        // Change, if necessary (saving previous state if
                        // appropriate).
                        //

                        if (MakeChanges) {

                            if (ARGUMENT_PRESENT(PreviousState)) {

                                PreviousState->Groups[(*ChangeCount)].Attributes =
                                    CurrentGroup.Attributes;

                                PreviousState->Groups[(*ChangeCount)].Sid =
                                    NextSid;

                                RtlCopySid( SidLength, NextSid, CurrentGroup.Sid );

                                NextSid = (PSID)((ULONG_PTR)NextSid + SidLength);
                            }

                            SepTokenGroupAttributes(Token,OldIndex) &=
                                ~(SepTokenGroupAttributes(Token,OldIndex)
                                  & SE_GROUP_ENABLED);
                            SepTokenGroupAttributes(Token,OldIndex) |=
                                 (SepArrayGroupAttributes(NewState,NewIndex)
                                  & SE_GROUP_ENABLED);



                        } //endif make changes

                        //
                        // increment the number of changes
                        //

                        (*ChangeCount) += 1;


                    } // endif change needed

                } // endif found

                NewIndex += 1;

            } // endwhile searching NewState

        } // endelse

        OldIndex += 1;

    } // endwhile more groups in token

    //
    // Set completion status appropriately if some not assigned
    //

    if (!ResetToDefault) {

        if (MatchCount < GroupCount) {
            CompletionStatus = STATUS_NOT_ALL_ASSIGNED;
        }
    }

    //
    //  Indicate whether changes were made
    //

    if ((*ChangeCount) > 0  &&  MakeChanges) {
        (*ChangesMade) = TRUE;
    } else {
        (*ChangesMade) = FALSE;
    }

    //
    // Calculate the space needed to return previous state information
    // (The SID lengths have already been added up in LocalReturnLength).
    //

    if (ARGUMENT_PRESENT(PreviousState)) {

        (*ReturnLength) = LocalReturnLength +
                          (ULONG)sizeof(TOKEN_GROUPS) +
                          ((*ChangeCount) *  (ULONG)sizeof(SID_AND_ATTRIBUTES)) -
                          (ANYSIZE_ARRAY * (ULONG)sizeof(SID_AND_ATTRIBUTES));
    }

   return CompletionStatus;
}
