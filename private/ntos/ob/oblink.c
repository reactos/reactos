/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    oblink.c

Abstract:

    Symbolic Link Object routines

Author:

    Steve Wood (stevewo) 3-Aug-1989

Revision History:

--*/

#include "obp.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,NtCreateSymbolicLinkObject)
#pragma alloc_text(PAGE,NtOpenSymbolicLinkObject)
#pragma alloc_text(PAGE,NtQuerySymbolicLinkObject)
#pragma alloc_text(PAGE,ObpParseSymbolicLink)
#pragma alloc_text(PAGE,ObpDeleteSymbolicLink)
#endif

//
//  This is the object type for device objects.
//

extern POBJECT_TYPE IoDeviceObjectType;

//
//  Local procedure prototypes
//

#define CREATE_SYMBOLIC_LINK 0
#define DELETE_SYMBOLIC_LINK 1

VOID
ObpProcessDosDeviceSymbolicLink (
    POBJECT_SYMBOLIC_LINK SymbolicLink,
    ULONG Action
    );


NTSTATUS
NtCreateSymbolicLinkObject (
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN PUNICODE_STRING LinkTarget
    )

/*++

Routine Description:

    This function creates a symbolic link object, sets it initial value to
    value specified in the LinkTarget parameter, and opens a handle to the
    object with the specified desired access.

Arguments:

    LinkHandle - Supplies a pointer to a variable that will receive the
        symbolic link object handle.

    DesiredAccess - Supplies the desired types of access for the symbolic link
        object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

    LinkTarget - Supplies the target name for the symbolic link object.

Return Value:

    An appropriate status value

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    PVOID Object;
    HANDLE Handle;
    UNICODE_STRING CapturedLinkTarget;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            //
            //  **** we probably don't need to do this first one because
            //  the ob create object will cause the probe to happen for us
            //

            ProbeForRead( ObjectAttributes,
                          sizeof( OBJECT_ATTRIBUTES ),
                          sizeof( ULONG ));

            ProbeForRead( LinkTarget,
                          sizeof( *LinkTarget ),
                          sizeof( UCHAR ));

            CapturedLinkTarget = *LinkTarget;

            ProbeForRead( CapturedLinkTarget.Buffer,
                          CapturedLinkTarget.MaximumLength,
                          sizeof( UCHAR ));

            ProbeForWriteHandle( LinkHandle );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }

    } else {

        CapturedLinkTarget = *LinkTarget;
    }

    //
    //  Check if there is an odd MaximumLength
    //

    if (CapturedLinkTarget.MaximumLength % sizeof( WCHAR )) {

        //
        //  Round down the MaximumLength to a valid even size
        //

        CapturedLinkTarget.MaximumLength = (CapturedLinkTarget.MaximumLength / sizeof( WCHAR )) * sizeof( WCHAR );
    }

    //
    //  Error if link target name length is odd, the length is greater than
    //  the maximum length, or zero and creating.
    //

    if ((CapturedLinkTarget.MaximumLength == 0) ||
        (CapturedLinkTarget.Length > CapturedLinkTarget.MaximumLength) ||
        (CapturedLinkTarget.Length % sizeof( WCHAR ))) {
        
        KdPrint(( "OB: Invalid symbolic link target - %wZ\n", &CapturedLinkTarget ));

        return( STATUS_INVALID_PARAMETER );
    }

    //
    //  Create the symbolic link object
    //

    Status = ObCreateObject( PreviousMode,
                             ObpSymbolicLinkObjectType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof( *SymbolicLink ),
                             0,
                             0,
                             (PVOID *)&SymbolicLink );

    if (!NT_SUCCESS( Status )) {

        return( Status );
    }

    //
    //  Fill in symbolic link object with link target name string
    //

    KeQuerySystemTime( &SymbolicLink->CreationTime );

    SymbolicLink->DosDeviceDriveIndex = 0;
    SymbolicLink->LinkTargetObject = NULL;

    RtlInitUnicodeString( &SymbolicLink->LinkTargetRemaining,  NULL );

    SymbolicLink->LinkTarget.MaximumLength = CapturedLinkTarget.MaximumLength;
    SymbolicLink->LinkTarget.Length = CapturedLinkTarget.Length;
    SymbolicLink->LinkTarget.Buffer = (PWCH)ExAllocatePoolWithTag( PagedPool,
                                                                   CapturedLinkTarget.MaximumLength,
                                                                   'tmyS' );

    if (SymbolicLink->LinkTarget.Buffer == NULL) {

        ObDereferenceObject( SymbolicLink );

        return STATUS_NO_MEMORY;
    }

    try {

        RtlMoveMemory( SymbolicLink->LinkTarget.Buffer,
                       CapturedLinkTarget.Buffer,
                       CapturedLinkTarget.MaximumLength );

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        ObDereferenceObject( SymbolicLink );

        return( GetExceptionCode() );
    }

    //
    //  Insert symbolic link object in the current processes object table,
    //  set symbolic link handle value and return status.
    //

    Status = ObInsertObject( SymbolicLink,
                             NULL,
                             DesiredAccess,
                             0,
                             (PVOID *)&Object,
                             &Handle );

    try {

        *LinkHandle = Handle;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        // Fall through, since we do not want to undo what we have done.
        //
    }

    return( Status );
}


NTSTATUS
NtOpenSymbolicLinkObject (
    OUT PHANDLE LinkHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to an symbolic link object with the specified
    desired access.

Arguments:

    LinkHandle - Supplies a pointer to a variable that will receive the
        symbolic link object handle.

    DesiredAccess - Supplies the desired types of access for the symbolic link
        object.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

Return Value:

    An appropriate status value

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    HANDLE Handle;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //  The object attributes does not need to be probed because the
    //  ObOpenObjectByName does the probe for us
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle( LinkHandle );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }
    }

    //
    //  Open handle to the symbolic link object with the specified desired
    //  access, set symbolic link handle value, and return service completion
    //  status.
    //

    Status = ObOpenObjectByName( ObjectAttributes,
                                 ObpSymbolicLinkObjectType,
                                 PreviousMode,
                                 NULL,
                                 DesiredAccess,
                                 NULL,
                                 &Handle );

    try {

        *LinkHandle = Handle;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        //  Fall through, since we do not want to undo what we have done.
        //
    }

    return( Status );
}


NTSTATUS
NtQuerySymbolicLinkObject (
    IN HANDLE LinkHandle,
    IN OUT PUNICODE_STRING LinkTarget,
    OUT PULONG ReturnedLength OPTIONAL
    )

/*++

Routine Description:

    This function queries the state of a symbolic link object and returns the
    requested information in the specified record structure.

Arguments:

    LinkHandle - Supplies a handle to a symbolic link object.  This handle
        must have SYMBOLIC_LINK_QUERY access granted.

    LinkTarget - Supplies a pointer to a record that is to receive the
        target name of the symbolic link object.

    ReturnedLength - Optionally receives the maximum length, in bytes, of
        the link target on return

Return Value:

    An appropriate status value

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    UNICODE_STRING CapturedLinkTarget;

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PAGED_CODE();

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForRead( LinkTarget,
                          sizeof( *LinkTarget ),
                          sizeof( WCHAR ) );

            ProbeForWriteUshort( &LinkTarget->Length );

            ProbeForWriteUshort( &LinkTarget->MaximumLength );

            CapturedLinkTarget = *LinkTarget;

            ProbeForWrite( CapturedLinkTarget.Buffer,
                           CapturedLinkTarget.MaximumLength,
                           sizeof( UCHAR ) );

            if (ARGUMENT_PRESENT( ReturnedLength )) {

                ProbeForWriteUlong( ReturnedLength );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }

    } else {

        CapturedLinkTarget = *LinkTarget;
    }

    //
    //  Reference symbolic link object by handle, read current state, deference
    //  symbolic link object, fill in target name structure and return service
    //  status.
    //

    Status = ObReferenceObjectByHandle( LinkHandle,
                                        SYMBOLIC_LINK_QUERY,
                                        ObpSymbolicLinkObjectType,
                                        PreviousMode,
                                        (PVOID *)&SymbolicLink,
                                        NULL );

    if (NT_SUCCESS( Status )) {

        ObpEnterObjectTypeMutex( ObpSymbolicLinkObjectType );

        //
        //  If the caller wants a return length and what we found can easily
        //  fit in the output buffer then we copy into the output buffer all
        //  the bytes from the link.
        //
        //  If the caller did not want a return length and we found can still
        //  easily fit in the output buffer then copy over the bytes that just
        //  make up the string and nothing extra
        //

        if ((ARGUMENT_PRESENT( ReturnedLength ) &&
                (SymbolicLink->LinkTarget.MaximumLength <= CapturedLinkTarget.MaximumLength))

                    ||

            (!ARGUMENT_PRESENT( ReturnedLength ) &&
                (SymbolicLink->LinkTarget.Length <= CapturedLinkTarget.MaximumLength)) ) {

            try {

                RtlMoveMemory( CapturedLinkTarget.Buffer,
                               SymbolicLink->LinkTarget.Buffer,
                               ARGUMENT_PRESENT( ReturnedLength ) ? SymbolicLink->LinkTarget.MaximumLength
                                                                  : SymbolicLink->LinkTarget.Length );

                LinkTarget->Length = SymbolicLink->LinkTarget.Length;

                if (ARGUMENT_PRESENT( ReturnedLength )) {

                    *ReturnedLength = SymbolicLink->LinkTarget.MaximumLength;
                }

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                // Fall through, since we do cannot undo what we have done.
                //
            }

        } else {

            //
            //  The output buffer is just too small for the link target, but
            //  we'll tell the user how much is needed if they asked for that
            //  return value
            //

            if (ARGUMENT_PRESENT( ReturnedLength )) {

                try {

                    *ReturnedLength = SymbolicLink->LinkTarget.MaximumLength;

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    //
                    // Fall through, since we do cannot undo what we have done.
                    //
                }
            }

            Status = STATUS_BUFFER_TOO_SMALL;
        }

        ObpLeaveObjectTypeMutex( ObpSymbolicLinkObjectType );

        ObDereferenceObject( SymbolicLink );
    }

    return( Status );
}


NTSTATUS
ObpParseSymbolicLink (
    IN PVOID ParseObject,
    IN PVOID ObjectType,
    IN PACCESS_STATE AccessState,
    IN KPROCESSOR_MODE AccessMode,
    IN ULONG Attributes,
    IN OUT PUNICODE_STRING CompleteName,
    IN OUT PUNICODE_STRING RemainingName,
    IN OUT PVOID Context OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    OUT PVOID *Object
    )

/*++

Routine Description:

    This is the call back routine for parsing symbolic link objects.  It is invoked
    as part of ObpLookupObjectName

Arguments:

    ParseObject - This will actually be a symbolic link object

    ObjectType - Specifies the type of the object to lookup

    AccessState - Current access state, describing already granted access
        types, the privileges used to get them, and any access types yet to
        be granted.  The access masks may not contain any generic access
        types.

    AccessMode - Specifies the callers processor mode

    Attributes - Specifies the attributes for the lookup (e.g., case
        insensitive)

    CompleteName - Supplies a pointer to the complete name that we are trying
        to open.  On return this could be modified to fit the new reparse
        buffer

    RemainingName - Supplies a pointer to the remaining name that we are
        trying to open.  On return this will point to what remains after
        we processed the symbolic link.

    Context - Unused

    SecurityQos - Unused

    Object - Receives a pointer to the symbolic link object that we
        resolve to

Return Value:

    STATUS_REPARSE_OBJECT if the parse object is already a snapped
        symbolic link meaning we've modified the remaining name and
        and have returned the target object of the symbolic link


    STATUS_REPARSE if the parse object has not been snapped.  In this
        case the Complete name has been modified with the link target
        name added in front of the remaining name.  The parameters
        remaining name and object must now be ignored by the caller

    An appropriate error value

--*/

{
    ULONG NewLength;
    USHORT Length;
    USHORT MaximumLength;
    PWCHAR NewName, NewRemainingName;
    ULONG InsertAmount;
    NTSTATUS Status;
    POBJECT_SYMBOLIC_LINK SymbolicLink;
    PUNICODE_STRING LinkTargetName;

    PAGED_CODE();

    //
    //  This routine needs to be synchonized with the delete symbolic link
    //  operation.  Which uses the root directory mutex.
    //

    ObpEnterRootDirectoryMutex();

    try {

        *Object = NULL;

        //
        //  If there isn't any name remaining and the caller gave us
        //  an object type then we'll reference the parse object.  If
        //  this is successful then that's the object we return.  Otherwise
        //  if the status is anything but a type mismatch then we'll
        //  return that error status
        //

        if (RemainingName->Length == 0) {

            if ( ObjectType ) {

                Status = ObReferenceObjectByPointer( ParseObject,
                                                     0,
                                                     ObjectType,
                                                     AccessMode );

                if (NT_SUCCESS( Status )) {

                    *Object = ParseObject;

                    leave;

                } else if (Status != STATUS_OBJECT_TYPE_MISMATCH) {

                    leave;
                }
           }

        //
        //  If the remaining name does not start with a "\" then
        //  its is illformed and we'll call it a type mismatch
        //

        } else if (*(RemainingName->Buffer) != OBJ_NAME_PATH_SEPARATOR) {

            Status = STATUS_OBJECT_TYPE_MISMATCH;
            leave;
        }

        //
        //  A symbolic link has been encountered. See if this link has been snapped
        //  to a particular object.
        //

        SymbolicLink = (POBJECT_SYMBOLIC_LINK)ParseObject;

        if (SymbolicLink->LinkTargetObject != NULL) {

            //
            //  This is a snapped link.  Get the remaining portion of the
            //  symbolic link target, if any.
            //

            LinkTargetName = &SymbolicLink->LinkTargetRemaining;

            if (LinkTargetName->Length == 0) {

                //
                //  Remaining link target string is zero, so return to caller
                //  quickly with snapped object pointer and remaining object name
                //  which we haven't touched yet.
                //

                *Object = SymbolicLink->LinkTargetObject;

                Status = STATUS_REPARSE_OBJECT;
                leave;
            }

            //
            //  We have a snapped symbolic link that has additional text.
            //  Insert that in front of the current remaining name, preserving
            //  and text between CompleteName and RemainingName
            //

            InsertAmount = LinkTargetName->Length;

            if ((LinkTargetName->Buffer[ (InsertAmount / sizeof( WCHAR )) - 1 ] == OBJ_NAME_PATH_SEPARATOR)

                    &&

                (*(RemainingName->Buffer) == OBJ_NAME_PATH_SEPARATOR)) {

                //
                //  Both the link target name ends in a "\" and the remaining
                //  starts with a "\" but we only need one when we're done
                //

                InsertAmount -= sizeof( WCHAR );
            }

            //
            //  **** don't see why we need to bias the differnce between two
            //  pointers with * sizeof(wchar)
            //

            NewLength = (ULONG)(((RemainingName->Buffer - CompleteName->Buffer) * sizeof( WCHAR )) +
                        InsertAmount +
                        RemainingName->Length);

            if (NewLength > 0xFFF0) {

                Status = STATUS_NAME_TOO_LONG;
                leave;
            }

            Length = (USHORT)NewLength;

            //
            //  Now check if the new computed length is too big for the input
            //  buffer containing the complete name
            //

            if (CompleteName->MaximumLength <= Length) {

                //
                //  The new concatentated name is larger than the buffer supplied for
                //  the complete name.  Allocate space for this new string
                //

                MaximumLength = Length + sizeof( UNICODE_NULL );
                NewName = ExAllocatePoolWithTag( NonPagedPool, MaximumLength, 'mNbO' );

                if (NewName == NULL) {

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    leave;
                }

                //
                //  Calculate the pointer within this buffer for the remaining
                //  name.  This value has not been biased by the new link
                //  target name
                //

                NewRemainingName = NewName + (RemainingName->Buffer - CompleteName->Buffer);

                //
                //  Copy over all the names that we've processed so far
                //

                RtlMoveMemory( NewName,
                               CompleteName->Buffer,
                               ((RemainingName->Buffer - CompleteName->Buffer) * sizeof( WCHAR )));

                //
                //  If we have some remaining names then those over at the
                //  the location offset to hold the link target name
                //

                if (RemainingName->Length != 0) {

                    RtlMoveMemory( (PVOID)((PUCHAR)NewRemainingName + InsertAmount),
                                   RemainingName->Buffer,
                                   RemainingName->Length );
                }

                //
                //  Now insert the link target name
                //

                RtlMoveMemory( NewRemainingName, LinkTargetName->Buffer, InsertAmount );

                //
                //  Free the old complete name buffer and reset the input
                //  strings to use the new buffer
                //

                ExFreePool( CompleteName->Buffer );

                CompleteName->Buffer = NewName;
                CompleteName->Length = Length;
                CompleteName->MaximumLength = MaximumLength;

                RemainingName->Buffer = NewRemainingName;
                RemainingName->Length = Length - (USHORT)((PCHAR)NewRemainingName - (PCHAR)NewName);
                RemainingName->MaximumLength = RemainingName->Length + sizeof( UNICODE_NULL );

            } else {

                //
                //  Insert extra text associated with this symbolic link name before
                //  existing remaining name, if any.
                //
                //  First shove over the remaining name to make a hole for the
                //  link target name
                //

                if (RemainingName->Length != 0) {

                    RtlMoveMemory( (PVOID)((PUCHAR)RemainingName->Buffer + InsertAmount),
                                   RemainingName->Buffer,
                                   RemainingName->Length );
                }

                //
                //  Now insert the link target name
                //

                RtlMoveMemory( RemainingName->Buffer, LinkTargetName->Buffer, InsertAmount );

                //
                //  Adjust input strings to account for this inserted text
                //

                CompleteName->Length += LinkTargetName->Length;

                RemainingName->Length += LinkTargetName->Length;
                RemainingName->MaximumLength += RemainingName->Length + sizeof( UNICODE_NULL );

                CompleteName->Buffer[ CompleteName->Length / sizeof( WCHAR ) ] = UNICODE_NULL;
            }

            //
            //  Return the object address associated with snapped symbolic link
            //  and the reparse object status code.
            //

            *Object = SymbolicLink->LinkTargetObject;

            Status = STATUS_REPARSE_OBJECT;
            leave;
        }

        //
        //  The symbolic has not yet been snapped
        //
        //  Compute the size of the new name and check if the name will
        //  fit in the existing complete name buffer.
        //

        LinkTargetName = &SymbolicLink->LinkTarget;
        NewLength = LinkTargetName->Length + RemainingName->Length;

        if (NewLength > 0xFFF0) {

            Status = STATUS_NAME_TOO_LONG;
            leave;
        }

        Length = (USHORT)NewLength;

        if (CompleteName->MaximumLength <= Length) {

            //
            //  The new concatentated name is larger than the buffer supplied for
            //  the complete name.
            //

            MaximumLength = Length + sizeof( UNICODE_NULL );
            NewName = ExAllocatePoolWithTag( NonPagedPool, MaximumLength, 'mNbO' );

            if (NewName == NULL) {

                Status = STATUS_INSUFFICIENT_RESOURCES;
                leave;
            }

        } else {

            MaximumLength = CompleteName->MaximumLength;
            NewName = CompleteName->Buffer;
        }

        //
        //  Concatenate the symbolic link name with the remaining name,
        //  if any.  What this does is overwrite the front of the complete
        //  name up to the remaining name with the links target name
        //

        if (RemainingName->Length != 0) {

            RtlMoveMemory( (PVOID)((PUCHAR)NewName + LinkTargetName->Length),
                           RemainingName->Buffer,
                           RemainingName->Length );
        }

        RtlMoveMemory( NewName, LinkTargetName->Buffer, LinkTargetName->Length );

        NewName[ Length / sizeof( WCHAR ) ] = UNICODE_NULL;

        //
        //  If a new name buffer was allocated, then free the original complete
        //  name buffer.
        //

        if (NewName != CompleteName->Buffer) {

            ExFreePool( CompleteName->Buffer );
        }

        //
        //  Set the new complete name buffer parameters and return a reparse
        //  status.
        //

        CompleteName->Buffer = NewName;
        CompleteName->Length = Length;
        CompleteName->MaximumLength = MaximumLength;

        Status = STATUS_REPARSE;

    } finally {

        ObpLeaveRootDirectoryMutex();
    }

    return Status;
}


VOID
ObpDeleteSymbolicLink (
    IN PVOID Object
    )

/*++

Routine Description:

    This routine is called when a reference to a symbolic link goes to zero.
    Its job is to clean up the memory used to the symbolic link string

Arguments:

    Object - Supplies a pointer to the symbolic link object being deleted

Return Value:

    None.

--*/

{
    POBJECT_SYMBOLIC_LINK SymbolicLink = (POBJECT_SYMBOLIC_LINK)Object;

    PAGED_CODE();

    //
    //  The only cleaning up we need to do is to free the link target
    //  buffer
    //

    if (SymbolicLink->LinkTarget.Buffer != NULL) {

        ExFreePool( SymbolicLink->LinkTarget.Buffer );
    }

    SymbolicLink->LinkTarget.Buffer = NULL;

    //
    //  And return to our caller
    //

    return;
}


VOID
ObpDeleteSymbolicLinkName (
    POBJECT_SYMBOLIC_LINK SymbolicLink
    )

/*++

Routine Description:

    This routine delete the symbolic from the system

Arguments:

    SymbolicLink - Supplies a pointer to the object body for the symbolic
        link object to delete

Return Value:

    None.

--*/

{
    PVOID CapturedLinkTargetObject;

    CapturedLinkTargetObject = NULL;

    //
    //  Lock the root directory mutex while we look at the target path to
    //  prevent names from disappearing out from under us.  Also protects
    //  against collisions with NtCreateSymbolicLink
    //

    ObpEnterRootDirectoryMutex();

    //
    //  Capture the link target object and referenced one more time
    //  to avoid destroying while the RootDirectoryMutex is hold
    //

    CapturedLinkTargetObject = SymbolicLink->LinkTargetObject;

    if (CapturedLinkTargetObject != NULL) {

        ObReferenceObject(CapturedLinkTargetObject);
    }

    ObpProcessDosDeviceSymbolicLink( SymbolicLink, DELETE_SYMBOLIC_LINK );

    ObpLeaveRootDirectoryMutex();

    //
    //  Remove the extra-reference for the link target object
    //

    if (CapturedLinkTargetObject != NULL) {

        ObDereferenceObject(CapturedLinkTargetObject);
    }

    return;
}


VOID
ObpCreateSymbolicLinkName (
    POBJECT_SYMBOLIC_LINK SymbolicLink
    )

/*++

Routine Description:

    This routine does extra processing for symbolic links being created in
    object directories controlled by device map objects.

    This processing consists of:

    1.  Determine if the name of the symbolic link is a drive letter.
        If so, then we will need to update the drive type in the
        associated device map object.

    2.  Process the link target, trying to resolve it into a pointer to
        an object other than a object directory object.  All object
        directories traversed must grant world traverse access other
        wise we bail out.  If we successfully find a non object
        directory object, then reference the object pointer and store it
        in the symbolic link object, along with a remaining string if
        any.  ObpLookupObjectName will used this cache object pointer to
        short circuit the name lookup directly to the cached object's
        parse routine.  For any object directory objects traversed along
        the way, increment their symbolic link SymbolicLinkUsageCount
        field.  This field is used whenever an object directory is
        deleted or its security is changed such that it no longer grants
        world traverse access.  In either case, if the field is non-zero
        we walk all the symbolic links and resnap them.

Arguments:

    SymbolicLink - pointer to symbolic link object being created.

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    WCHAR DosDeviceDriveLetter;
    ULONG DosDeviceDriveIndex;

    //
    //  Now see if this symbolic link is being created in an object directory
    //  controlled by a device map object.  Since we are only called from
    //  NtCreateSymbolicLinkObject, after the handle to this symbolic link
    //  has been created but before it is returned to the caller the handle can't
    //  be closed while we are executing, unless via a random close,
    //  So no need to hold the type specific mutex while we look at the name.
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( SymbolicLink );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    if ((NameInfo == NULL) ||
        (NameInfo->Directory == NULL) ||
        (NameInfo->Directory->DeviceMap == NULL)) {

        return;
    }

    //
    //  Here if we are creating a symbolic link in an object directory controlled
    //  by a device map object.  See if this is a drive letter definition.  If so
    //  calculate the drive letter index and remember in the symbolic link object.
    //

    DosDeviceDriveIndex = 0;

    if ((NameInfo->Name.Length == (2 * sizeof( WCHAR ))) &&
        (NameInfo->Name.Buffer[ 1 ] == L':')) {

        DosDeviceDriveLetter = RtlUpcaseUnicodeChar( NameInfo->Name.Buffer[ 0 ] );

        if ((DosDeviceDriveLetter >= L'A') && (DosDeviceDriveLetter <= L'Z')) {

            DosDeviceDriveIndex = DosDeviceDriveLetter - L'A';
            DosDeviceDriveIndex += 1;

            SymbolicLink->DosDeviceDriveIndex = DosDeviceDriveIndex;
        }
    }

    //
    //  Now traverse the target path seeing if we can snap the link now.
    //

    ObpProcessDosDeviceSymbolicLink( SymbolicLink, CREATE_SYMBOLIC_LINK );

    return;
}


//
//  Local support routine
//

#define MAX_DEPTH 16

VOID
ObpProcessDosDeviceSymbolicLink (
    POBJECT_SYMBOLIC_LINK SymbolicLink,
    ULONG Action
    )

/*++

Routine Description:

    This function is called whenever a symbolic link is created or deleted
    in an object directory controlled by a device map object.

    For creates, it attempts to snap the symbolic link to a non-object
    directory object.  It does this by walking the symbolic link target
    string, until it sees a non-directory object or a directory object
    that does NOT allow World traverse access.  It stores a referenced
    pointer to this object in the symbolic link object.  It also
    increments a count in each of the object directory objects that it
    walked over.  This count is used to disallow any attempt to remove
    World traverse access from a directory object after it has
    participated in a snapped symbolic link.

    For deletes, it repeats the walk of the target string, decrementing
    the count associated with each directory object walked over.  It also
    dereferences the snapped object pointer.

Arguments:

    SymbolicLink - pointer to symbolic link object being created or deleted.

    Action - describes whether this is a create or a delete action

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    PVOID Object;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PSECURITY_DESCRIPTOR SecurityDescriptor;
    BOOLEAN MemoryAllocated;
    UNICODE_STRING RemainingName;
    UNICODE_STRING ComponentName;
    BOOLEAN HaveWorldTraverseAccess;
    ULONG Depth;
    POBJECT_DIRECTORY Directories[ MAX_DEPTH ], Directory, ParentDirectory;
    PDEVICE_OBJECT DeviceObject;
    PDEVICE_MAP DeviceMap = NULL;
    ULONG DosDeviceDriveType;
    BOOLEAN DeviceMapUsed = FALSE;    
    

    Object = NULL;

    //
    //  Check if we are creating a symbolic link or if the link has already
    //  been snapped
    //

    if ((Action == CREATE_SYMBOLIC_LINK) ||
        (SymbolicLink->LinkTargetObject != NULL)) {

        ParentDirectory = NULL;
        Depth    = 0;
        Directory = ObpRootDirectoryObject;
        RemainingName = SymbolicLink->LinkTarget;

        DeviceMap = PsGetCurrentProcess()->DeviceMap;


ReCalcDeviceMap:

        if (DeviceMap) {


            if (!((ULONG_PTR)(RemainingName.Buffer) & (sizeof(ULONGLONG)-1))

                        &&

                (DeviceMap->DosDevicesDirectory != NULL )) {

                //
                //  Check if the object name is actually equal to the
                //  global dos devices short name prefix "\??\"
                //

                if ((RemainingName.Length >= ObpDosDevicesShortName.Length)

                        &&

                    (*(PULONGLONG)(RemainingName.Buffer) == ObpDosDevicesShortNamePrefix.QuadPart)) {

                    //
                    //  The user gave us the dos short name prefix so we'll
                    //  look down the directory, and start the search at the
                    //  dos device directory
                    //

                    Directory = DeviceMap->DosDevicesDirectory;

                    RemainingName.Buffer += (ObpDosDevicesShortName.Length / sizeof( WCHAR ));
                    RemainingName.Length -= ObpDosDevicesShortName.Length;

                    DeviceMapUsed = TRUE;

                } 
            }
        }

        //
        //  The following loop will dissect the link target checking
        //  that each directory exists and that we have access to
        //  the directory.  When we pop out the local directories
        //  array will contain a list of directories that we need
        //  to traverse to process this action.
        //

        while (TRUE) {
                        
            //
            //  Gobble up the "\" in the remaining name
            //

            if (*(RemainingName.Buffer) == OBJ_NAME_PATH_SEPARATOR) {

                RemainingName.Buffer++;
                RemainingName.Length -= sizeof( OBJ_NAME_PATH_SEPARATOR );
            }

            //
            //  And dissect the name into its first component and any
            //  remaining part
            //

            ComponentName = RemainingName;

            while (RemainingName.Length != 0) {

                if (*(RemainingName.Buffer) == OBJ_NAME_PATH_SEPARATOR) {

                    break;
                }

                RemainingName.Buffer++;
                RemainingName.Length -= sizeof( OBJ_NAME_PATH_SEPARATOR );
            }

            ComponentName.Length -= RemainingName.Length;

            if (ComponentName.Length == 0) {

                return;
            }

            //
            //  See if we have world traverse access to look this name up
            //

            if (ParentDirectory != NULL) {

                HaveWorldTraverseAccess = FALSE;

                //
                //  Obtain the object's security descriptor
                //

                Status = ObGetObjectSecurity( ParentDirectory,
                                              &SecurityDescriptor,
                                              &MemoryAllocated );

                if (NT_SUCCESS( Status )) {

                    //
                    //  Check to see if WORLD has TRAVERSE access and then release
                    //  the security descriptor
                    //

                    HaveWorldTraverseAccess = SeFastTraverseCheck( SecurityDescriptor,
                                                                   DIRECTORY_TRAVERSE,
                                                                   UserMode );

                    ObReleaseObjectSecurity( SecurityDescriptor,
                                             MemoryAllocated );
                }

                if (!HaveWorldTraverseAccess) {

                    Object = NULL;
                    break;
                }

                if (Depth >= MAX_DEPTH) {

                    Object = NULL;
                    break;
                }

                Directories[ Depth++ ] = ParentDirectory;
            }

            //
            //  Look this component name up in this directory.  If not found, then
            //  bail.
            //

            Object = ObpLookupDirectoryEntry( Directory, &ComponentName, 0 );

            if (Object == NULL) {

                break;
            }

            //
            //  See if this is a object directory object.  If so, keep going
            //

            ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

            if (ObjectHeader->Type == ObpDirectoryObjectType) {

                ParentDirectory = Directory;
                Directory = (POBJECT_DIRECTORY)Object;

            } else if ((ObjectHeader->Type == ObpSymbolicLinkObjectType) &&
                       (((POBJECT_SYMBOLIC_LINK)Object)->DosDeviceDriveIndex == 0)) {

                //
                //  Found a symbolic link to another symbolic link that is
                //  not already snapped.  So switch to its target string
                //  so we can chase down the real device object
                //

                ParentDirectory = NULL;
                Depth = 0;
                Directory = ObpRootDirectoryObject;
                RemainingName = ((POBJECT_SYMBOLIC_LINK)Object)->LinkTarget;

                goto ReCalcDeviceMap;

            } else {

                //
                //  Not an object directory object, or a symbolic link to an
                //  unsnapped symbolic link, so all done.  Exit the loop
                //

                break;
            }
        }

        //
        //  Done walking the target path.  Now update the counts associated
        //  with each directory object walked over.
        //

        while (Depth--) {

            Directory = Directories[ Depth ];

            if (Action == CREATE_SYMBOLIC_LINK) {

                if (Object != NULL) {

                    Directory->SymbolicLinkUsageCount += 1;
                }

            } else {

                Directory->SymbolicLinkUsageCount -= 1;
            }
        }
    }

    //
    //  Done processing symbolic link target path.  Update symbolic link
    //  object as appropriate for passed in reason
    //

    //
    //  If this is a drive letter symbolic link, get the address of the device
    //  map object that is controlling the containing object directory so we
    //  can update the drive type in the device map.
    //

    DeviceMap = NULL;

    if (SymbolicLink->DosDeviceDriveIndex != 0) {

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( SymbolicLink );
        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

        if (NameInfo != NULL && NameInfo->Directory) {

            DeviceMap = NameInfo->Directory->DeviceMap;
        }
    }

    //
    //  Check if we are creating a symbolic link
    //

    if (Action == CREATE_SYMBOLIC_LINK) {

        DosDeviceDriveType = DOSDEVICE_DRIVE_CALCULATE;

        if (Object != NULL) {


            //
            // We only want to do snapping for console session. When we create a
            // remote session all the symbolic links stored in the console dos devices
            // directory (\??) are copied into the per session DosDevices object directory
            // (\Session\<id>\DosDevices). We don't want to do snapping for the copied
            // symbolic links since for each copy we will increment the ref count on the
            // target object. All these counts have to go to zero before the device can be
            // deleted.
            //
            if ((PsGetCurrentProcess()->SessionId == 0) || (DeviceMapUsed)) {
                //
                //  Create action.  Store a referenced pointer to the snapped object
                //  along with the description of any remaining name string.  Also,
                //  for Dos drive letters, update the drive type in the appropriate
                //  device map object.
                //

                ObReferenceObject( Object );

                SymbolicLink->LinkTargetObject = Object;

                if ((*(RemainingName.Buffer) == OBJ_NAME_PATH_SEPARATOR) &&
                    (RemainingName.Length == sizeof( OBJ_NAME_PATH_SEPARATOR))) {

                    RtlInitUnicodeString( &SymbolicLink->LinkTargetRemaining, NULL );

                } else {

                    SymbolicLink->LinkTargetRemaining = RemainingName;
                }
            }

            if (SymbolicLink->DosDeviceDriveIndex != 0) {

                //
                //  Default is to calculate the drive type in user mode if we are
                //  unable to snap the symbolic link or it does not resolve to a
                //  DEVICE_OBJECT we know about.
                //

                ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

                if (ObjectHeader->Type == IoDeviceObjectType) {

                    DeviceObject = (PDEVICE_OBJECT)Object;

                    switch (DeviceObject->DeviceType) {

                    case FILE_DEVICE_CD_ROM:
                    case FILE_DEVICE_CD_ROM_FILE_SYSTEM:

                        DosDeviceDriveType = DOSDEVICE_DRIVE_CDROM;

                        break;

                    case FILE_DEVICE_DISK:
                    case FILE_DEVICE_DISK_FILE_SYSTEM:
                    case FILE_DEVICE_FILE_SYSTEM:

                        if (DeviceObject->Characteristics & FILE_REMOVABLE_MEDIA) {

                            DosDeviceDriveType = DOSDEVICE_DRIVE_REMOVABLE;

                        } else {

                            DosDeviceDriveType = DOSDEVICE_DRIVE_FIXED;
                        }

                        break;

                    case FILE_DEVICE_MULTI_UNC_PROVIDER:
                    case FILE_DEVICE_NETWORK:
                    case FILE_DEVICE_NETWORK_BROWSER:
                    case FILE_DEVICE_NETWORK_REDIRECTOR:

                        DosDeviceDriveType = DOSDEVICE_DRIVE_REMOTE;

                        break;

                    case FILE_DEVICE_NETWORK_FILE_SYSTEM:

#if defined(REMOTE_BOOT)
                        //
                        //  If this is a remote boot workstation, the X:
                        //  drive is a redirected drive, but needs to look
                        //  like a local drive.
                        //

                        if (IoRemoteBootClient &&
                            (SymbolicLink->DosDeviceDriveIndex == 24)) {

                            DosDeviceDriveType = DOSDEVICE_DRIVE_FIXED;

                        } else
#endif // defined(REMOTE_BOOT)
                        {
                            DosDeviceDriveType = DOSDEVICE_DRIVE_REMOTE;
                        }

                        break;

                    case FILE_DEVICE_VIRTUAL_DISK:

                        DosDeviceDriveType = DOSDEVICE_DRIVE_RAMDISK;

                        break;

                    default:

                        DosDeviceDriveType = DOSDEVICE_DRIVE_UNKNOWN;

                        break;
                    }
                }
            }
        }

        //
        //  If this is a drive letter symbolic link, update the drive type and
        //  and mark as valid drive letter.
        //

        if (DeviceMap != NULL) {

            DeviceMap->DriveType[ SymbolicLink->DosDeviceDriveIndex-1 ] = (UCHAR)DosDeviceDriveType;
            DeviceMap->DriveMap |= 1 << (SymbolicLink->DosDeviceDriveIndex-1) ;
        }

    } else {

        //
        //  Deleting the symbolic link.  Dereference the snapped object pointer if any
        //  and zero out the snapped object fields.
        //

        RtlInitUnicodeString( &SymbolicLink->LinkTargetRemaining, NULL );

        Object = SymbolicLink->LinkTargetObject;

        if (Object != NULL) {

            SymbolicLink->LinkTargetObject = NULL;
            ObDereferenceObject( Object );
        }

        //
        //  If this is a drive letter symbolic link, set the drive type to
        //  unknown and clear the bit in the drive letter bit map.
        //

        if (DeviceMap != NULL) {

            DeviceMap->DriveMap &= ~(1 << (SymbolicLink->DosDeviceDriveIndex-1));
            DeviceMap->DriveType[ SymbolicLink->DosDeviceDriveIndex-1 ] = DOSDEVICE_DRIVE_UNKNOWN;

            SymbolicLink->DosDeviceDriveIndex = 0;
        }

        //
        //  If we allocated a target buffer, free it.
        //

        if (SymbolicLink->LinkTarget.Buffer != NULL) {

            ExFreePool( SymbolicLink->LinkTarget.Buffer );

            RtlInitUnicodeString( &SymbolicLink->LinkTarget, NULL );
        }
    }
    
    return;
}

