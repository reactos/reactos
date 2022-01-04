/*++

Copyright (c) 1989-2000 Microsoft Corporation

Module Name:

    AcChkSup.c

Abstract:

    This module implements the FAT access checking routine


--*/

#include "fatprocs.h"

//
//  Our debug trace level
//

#define Dbg                              (DEBUG_TRACE_ACCHKSUP)

NTSTATUS
FatCreateRestrictEveryoneToken(
    IN PACCESS_TOKEN Token,
    OUT PACCESS_TOKEN *RestrictedToken
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FatCheckFileAccess)
#pragma alloc_text(PAGE, FatCheckManageVolumeAccess)
#pragma alloc_text(PAGE, FatCreateRestrictEveryoneToken)
#pragma alloc_text(PAGE, FatExplicitDeviceAccessGranted)
#endif


BOOLEAN
FatCheckFileAccess (
    PIRP_CONTEXT IrpContext,
    IN UCHAR DirentAttributes,
    IN PACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine checks if a desired access is allowed to a file represented
    by the specified DirentAttriubutes.

Arguments:

    DirentAttributes - Supplies the Dirent attributes to check access for

    DesiredAccess - Supplies the desired access mask that we are checking for

Return Value:

    BOOLEAN - TRUE if access is allowed and FALSE otherwise

--*/

{
    BOOLEAN Result;

    DebugTrace(+1, Dbg, "FatCheckFileAccess\n", 0);
    DebugTrace( 0, Dbg, "DirentAttributes = %8lx\n", DirentAttributes);
    DebugTrace( 0, Dbg, "DesiredAccess    = %8lx\n", *DesiredAccess);

    PAGED_CODE();

    //
    //  This procedures is programmed like a string of filters each
    //  filter checks to see if some access is allowed,  if it is not allowed
    //  the filter return FALSE to the user without further checks otherwise
    //  it moves on to the next filter.  The filter check is to check for
    //  desired access flags that are not allowed for a particular dirent
    //

    Result = TRUE;

    _SEH2_TRY {

        //
        //  Check for Volume ID or Device Dirents, these are not allowed user
        //  access at all
        //

        if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_VOLUME_ID) ||
            FlagOn(DirentAttributes, FAT_DIRENT_ATTR_DEVICE)) {

            DebugTrace(0, Dbg, "Cannot access volume id or device\n", 0);

            try_return( Result = FALSE );
        }

        //
        //  Check the desired access for the object - we only blackball that
        //  we do not understand.  The model of filesystems using ACLs is that
        //  they do not type the ACL to the object the ACL is on.  Permissions
        //  are not checked for consistency vs. the object type - dir/file.
        //

        if (FlagOn(*DesiredAccess, ~(DELETE |
                                     READ_CONTROL |
                                     WRITE_OWNER |
                                     WRITE_DAC |
                                     SYNCHRONIZE |
                                     ACCESS_SYSTEM_SECURITY |
                                     FILE_WRITE_DATA |
                                     FILE_READ_EA |
                                     FILE_WRITE_EA |
                                     FILE_READ_ATTRIBUTES |
                                     FILE_WRITE_ATTRIBUTES |
                                     FILE_LIST_DIRECTORY |
                                     FILE_TRAVERSE |
                                     FILE_DELETE_CHILD |
                                     FILE_APPEND_DATA |
                                     MAXIMUM_ALLOWED))) {

            DebugTrace(0, Dbg, "Cannot open object\n", 0);

            try_return( Result = FALSE );
        }

        //
        //  Check for a read-only Dirent
        //

        if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_READ_ONLY)) {

            //
            //  Check the desired access for a read-only dirent.  AccessMask will contain
            //  the flags we're going to allow.
            //

            ACCESS_MASK AccessMask = DELETE | READ_CONTROL | WRITE_OWNER | WRITE_DAC |
                                    SYNCHRONIZE | ACCESS_SYSTEM_SECURITY | FILE_READ_DATA |
                                    FILE_READ_EA | FILE_WRITE_EA | FILE_READ_ATTRIBUTES |
                                    FILE_WRITE_ATTRIBUTES | FILE_EXECUTE | FILE_LIST_DIRECTORY |
                                    FILE_TRAVERSE;

            //
            //  If this is a subdirectory also allow add file/directory and delete.
            //

            if (FlagOn(DirentAttributes, FAT_DIRENT_ATTR_DIRECTORY)) {

                AccessMask |= FILE_ADD_SUBDIRECTORY | FILE_ADD_FILE | FILE_DELETE_CHILD;
            }

            if (FlagOn(*DesiredAccess, ~AccessMask)) {

                DebugTrace(0, Dbg, "Cannot open readonly\n", 0);

                try_return( Result = FALSE );
            }
        }

    try_exit: NOTHING;
    } _SEH2_FINALLY {

        DebugUnwind( FatCheckFileAccess );

        DebugTrace(-1, Dbg, "FatCheckFileAccess -> %08lx\n", Result);
    } _SEH2_END;

    UNREFERENCED_PARAMETER( IrpContext );

    return Result;
}


BOOLEAN
FatCheckManageVolumeAccess (
    _In_ PIRP_CONTEXT IrpContext,
    _In_ PACCESS_STATE AccessState,
    _In_ KPROCESSOR_MODE ProcessorMode
    )

/*++

Routine Description:

    This function checks whether the SID described in the input access state has
    manage volume privilege.

Arguments:

    AccessState - the access state describing the security context to be checked

    ProcessorMode - the mode this check should occur against

Return Value:

    BOOLEAN - TRUE if privilege is held and FALSE otherwise

--*/

{
    PRIVILEGE_SET PrivilegeSet;

    PAGED_CODE();

    PrivilegeSet.PrivilegeCount = 1;
    PrivilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
    PrivilegeSet.Privilege[0].Luid = RtlConvertLongToLuid( SE_MANAGE_VOLUME_PRIVILEGE );
    PrivilegeSet.Privilege[0].Attributes = 0;

    if (SePrivilegeCheck( &PrivilegeSet,
                          &AccessState->SubjectSecurityContext,
                          ProcessorMode )) {

        return TRUE;
    }

    UNREFERENCED_PARAMETER( IrpContext );

    return FALSE;
}


NTSTATUS
FatExplicitDeviceAccessGranted (
    IN PIRP_CONTEXT IrpContext,
    IN PDEVICE_OBJECT DeviceObject,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE ProcessorMode
    )

/*++

Routine Description:

    This function asks whether the SID described in the input access state has
    been granted any explicit access to the given device object.  It does this
    by acquiring a token stripped of its ability to acquire access via the
    Everyone SID and re-doing the access check.

Arguments:

    DeviceObject - the device whose ACL will be checked

    AccessState - the access state describing the security context to be checked

    ProcessorMode - the mode this check should occur against

Return Value:

    NTSTATUS - Indicating whether explicit access was granted.

--*/

{
    NTSTATUS Status;

    PACCESS_TOKEN OriginalAccessToken;
    PACCESS_TOKEN RestrictedAccessToken;

    PACCESS_TOKEN *EffectiveToken;

    ACCESS_MASK GrantedAccess;

    PAGED_CODE();

    UNREFERENCED_PARAMETER( IrpContext );

    //
    //  If the access state indicates that specific access other
    //  than traverse was acquired, either Everyone does have such
    //  access or explicit access was granted.  In both cases, we're
    //  happy to let this proceed.
    //

    if (AccessState->PreviouslyGrantedAccess & (SPECIFIC_RIGHTS_ALL ^
                                                FILE_TRAVERSE)) {

        return STATUS_SUCCESS;
    }

    //
    //  If the manage volume privilege is held, this also permits access.
    //

    if (FatCheckManageVolumeAccess( IrpContext,
                                    AccessState,
                                    ProcessorMode )) {

        return STATUS_SUCCESS;
    }

    //
    //  Capture the subject context as a prelude to everything below.
    //

    SeLockSubjectContext( &AccessState->SubjectSecurityContext );

    //
    //  Convert the token in the subject context into one which does not
    //  acquire access through the Everyone SID.
    //
    //  The logic for deciding which token is effective comes from
    //  SeQuerySubjectContextToken; since there is no natural way
    //  of getting a pointer to it, do it by hand.
    //

    if (ARGUMENT_PRESENT( AccessState->SubjectSecurityContext.ClientToken )) {
        EffectiveToken = &AccessState->SubjectSecurityContext.ClientToken;
    } else {
        EffectiveToken = &AccessState->SubjectSecurityContext.PrimaryToken;
    }

    OriginalAccessToken = *EffectiveToken;
    Status = FatCreateRestrictEveryoneToken( OriginalAccessToken, &RestrictedAccessToken );

    if (!NT_SUCCESS(Status)) {

        SeReleaseSubjectContext( &AccessState->SubjectSecurityContext );
        return Status;
    }

    //
    //  Now see if the resulting context has access to the device through
    //  its explicitly granted access.  We swap in our restricted token
    //  for this check as the effective client token.
    //

    *EffectiveToken = RestrictedAccessToken;

#ifdef _MSC_VER
#pragma prefast( suppress: 28175, "we're a file system, this is ok to touch" )
#endif
    SeAccessCheck( DeviceObject->SecurityDescriptor,
                   &AccessState->SubjectSecurityContext,
                   FALSE,
                   AccessState->OriginalDesiredAccess,
                   0,
                   NULL,
                   IoGetFileObjectGenericMapping(),
                   ProcessorMode,
                   &GrantedAccess,
                   &Status );

    *EffectiveToken = OriginalAccessToken;

    //
    //  Cleanup and return.
    //

    SeUnlockSubjectContext( &AccessState->SubjectSecurityContext );
    ObDereferenceObject( RestrictedAccessToken );

    return Status;
}


NTSTATUS
FatCreateRestrictEveryoneToken (
    IN PACCESS_TOKEN Token,
    OUT PACCESS_TOKEN *RestrictedToken
    )

/*++

Routine Description:

    This function takes a token as the input and returns a new restricted token
    from which Everyone sid has been disabled.  The resulting token may be used
    to find out if access is available to a user-sid by explicit means.

Arguments:

    Token - Input token from which Everyone sid needs to be deactivated.

    RestrictedToken - Receives the the new restricted token.
        This must be released using ObDereferenceObject(*RestrictedToken);

Return Value:

    NTSTATUS - Returned by SeFilterToken.

--*/

{
    //
    // Array of sids to disable.
    //

    TOKEN_GROUPS SidsToDisable;

    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    //
    //  Restricted token will contain the original sids with one change:
    //  If Everyone sid is present in the token, it will be marked for DenyOnly.
    //

    *RestrictedToken = NULL;

    //
    //  Put Everyone sid in the array of sids to disable. This will mark it
    //  for SE_GROUP_USE_FOR_DENY_ONLY and it'll only be applicable for Deny aces.
    //

    SidsToDisable.GroupCount = 1;
    SidsToDisable.Groups[0].Attributes = 0;
    SidsToDisable.Groups[0].Sid = SeExports->SeWorldSid;

    Status = SeFilterToken(
                 Token,            // Token that needs to be restricted.
                 0,                // No flags
                 &SidsToDisable,   // Disable everyone sid
                 NULL,             // Do not create any restricted sids
                 NULL,             // Do not delete any privileges
                 RestrictedToken   // Restricted token
                 );

    return Status;
}

