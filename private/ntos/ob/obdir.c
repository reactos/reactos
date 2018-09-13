/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obdir.c

Abstract:

    Directory Object routines

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,NtCreateDirectoryObject)
#pragma alloc_text(PAGE,NtOpenDirectoryObject)
#pragma alloc_text(PAGE,NtQueryDirectoryObject)
#pragma alloc_text(PAGE,ObpLookupDirectoryEntry)
#endif


NTSTATUS
NtCreateDirectoryObject (
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This routine creates a new directory object according to user
    specified object attributes

Arguments:

    DirectoryHandle - Receives the handle for the newly created
        directory object

    DesiredAccess - Supplies the access being requested for this
        new directory object

    ObjectAttributes - Supplies caller specified attributes for new
        directory object

Return Value:

    An appropriate status value.

--*/

{
    POBJECT_DIRECTORY Directory;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    ObpValidateIrql( "NtCreateDirectoryObject" );

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle( DirectoryHandle );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }
    }

    //
    //  Allocate and initialize a new Directory Object.  We don't need
    //  to specify a parse context or charge any quota.  The size of
    //  the object body is simply a directory object.  This call gets
    //  us a new referenced object.
    //

    Status = ObCreateObject( PreviousMode,
                             ObpDirectoryObjectType,
                             ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof( *Directory ),
                             0,
                             0,
                             (PVOID *)&Directory );

    if (!NT_SUCCESS( Status )) {

        return( Status );
    }

    RtlZeroMemory( Directory, sizeof( *Directory ) );

    //
    //  Insert directory object in the current processes handle table,
    //  set directory handle value and return status.
    //
    //  **** If the insert fails should we backout the create?
    //

    Status = ObInsertObject( Directory,
                             NULL,
                             DesiredAccess,
                             0,
                             (PVOID *)NULL,
                             &Handle );

    try {

        *DirectoryHandle = Handle;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        //  Fall through, since we do not want to undo what we have done.
        //
    }

    return( Status );
}


NTSTATUS
NtOpenDirectoryObject (
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This routine opens an existing directory object.

Arguments:

    DirectoryHandle - Receives the handle for the newly opened directory
        object

    DesiredAccess - Supplies the access being requested for this
        directory object

    ObjectAttributes - Supplies caller specified attributes for the
        directory object

Return Value:

    An appropriate status value.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    HANDLE Handle;

    PAGED_CODE();

    ObpValidateIrql( "NtOpenDirectoryObject" );

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWriteHandle( DirectoryHandle );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }
    }

    //
    //  Open handle to the directory object with the specified desired access,
    //  set directory handle value, and return service completion status.
    //

    Status = ObOpenObjectByName( ObjectAttributes,
                                 ObpDirectoryObjectType,
                                 PreviousMode,
                                 NULL,
                                 DesiredAccess,
                                 NULL,
                                 &Handle );

    try {

        *DirectoryHandle = Handle;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        //  Fall through, since we do not want to undo what we have done.
        //
    }

    return Status;
}


NTSTATUS
NtQueryDirectoryObject (
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This function returns information regarding a specified object
    directory.

Arguments:

    DirectoryHandle - Supplies a handle to the directory being queried

    Buffer - Supplies the output buffer to receive the directory
        information.  On return this contains one or more OBJECT DIRECTORY
        INFORMATION structures, the last one being null.  And then this is
        followed by the string names for the directory entries.

    Length - Supplies the length, in bytes, of the user supplied output
        buffer

    ReturnSingleEntry - Indicates if this routine should just return
        one entry in the directory

    RestartScan - Indicates if we are to restart the scan or continue
        relative to the enumeration context passed in as the next
        parameter

    Context - Supplies an enumeration context that must be resupplied
        to this routine on subsequent calls to keep the enumeration
        in sync

    ReturnLength - Optionally receives the length, in bytes, that this
        routine has stuffed into the output buffer

Return Value:

    An appropriate status value.

--*/

{
    POBJECT_DIRECTORY Directory;
    POBJECT_DIRECTORY_ENTRY DirectoryEntry;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    UNICODE_STRING ObjectName;
    POBJECT_DIRECTORY_INFORMATION DirInfo;
    PWCH NameBuffer;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    ULONG Bucket, EntryNumber, CapturedContext;
    ULONG TotalLengthNeeded, LengthNeeded, EntriesFound;
    PCHAR TempBuffer;

    PAGED_CODE();

    ObpValidateIrql( "NtQueryDirectoryObject" );

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWrite( Buffer, Length, sizeof( WCHAR ) );
            ProbeForWriteUlong( Context );

            if (ARGUMENT_PRESENT( ReturnLength )) {

                ProbeForWriteUlong( ReturnLength );
            }

            if (RestartScan) {

                CapturedContext = 0;

            } else {

                CapturedContext = *Context;
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }

    } else {

        if (RestartScan) {

            CapturedContext = 0;

        } else {

            CapturedContext = *Context;
        }
    }

    //
    //  Allocate space for a temporary work buffer, make sure we got it,
    //  and then zero it out.  Make sure the buffer is large enough to
    //  hold at least one dir info record.  This will make the logic work
    //  better when the a bad length is passed in.
    //


    //
    //  Test for 64 bit if Length + sizeof( OBJECT_DIRECTORY_INFORMATION ) is less than Length
    //  Return STATUS_INVALID_PARAMETER if there is an overflow
    //

    if (ObpIsOverflow( Length, sizeof( OBJECT_DIRECTORY_INFORMATION ))) {

        return( STATUS_INVALID_PARAMETER );
    }

    TempBuffer = ExAllocatePoolWithTag( PagedPool,
                                        Length + sizeof( OBJECT_DIRECTORY_INFORMATION ),
                                        'mNbO' );

    if (TempBuffer == NULL) {

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    RtlZeroMemory( TempBuffer, Length );

    //
    //  Reference the directory object
    //

    Status = ObReferenceObjectByHandle( DirectoryHandle,
                                        DIRECTORY_QUERY,
                                        ObpDirectoryObjectType,
                                        PreviousMode,
                                        (PVOID *)&Directory,
                                        NULL );

    if (!NT_SUCCESS( Status )) {

        ExFreePool( TempBuffer );

        return( Status );
    }

    //
    //  Lock down the directory structures for the life of this
    //  procedure
    //

    ObpEnterRootDirectoryMutex();

    //
    //  DirInfo is used to march through the output buffer filling
    //  in directory information.  We'll start off by making sure
    //  there is room for a NULL entry at end.
    //

    DirInfo = (POBJECT_DIRECTORY_INFORMATION)TempBuffer;

    TotalLengthNeeded = sizeof( *DirInfo );

    //
    //  Keep track of the number of entries found and actual
    //  entry that we are processing
    //

    EntryNumber = 0;
    EntriesFound = 0;

    //
    //  By default we'll say there are no more entries until the
    //  following loop put in some data
    //

    Status = STATUS_NO_MORE_ENTRIES;

    //
    //  Our outer loop processes each hash bucket in the directory object
    //

    for (Bucket=0; Bucket<NUMBER_HASH_BUCKETS; Bucket++) {

        DirectoryEntry = Directory->HashBuckets[ Bucket ];

        //
        //  For this hash bucket we'll zip through its list of entries.
        //  This is a singly linked list so when the next pointer is null
        //  (i.e., false) we at the end of the hash list
        //

        while (DirectoryEntry) {

            //
            //  The captured context is simply the entry count unless the
            //  user specified otherwise we start at zero, which means
            //  the first entry is always returned in the enumeration.
            //  If we have an match based on the entry index then we
            //  process this entry.  We bump the captured context further
            //  done in the code.
            //

            if (CapturedContext == EntryNumber++) {

                //
                //  For this directory entry we'll get a pointer to the
                //  object body and see if it has an object name.  If it
                //  doesn't have a name then we'll give it an empty name.
                //

                ObjectHeader = OBJECT_TO_OBJECT_HEADER( DirectoryEntry->Object );
                NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

                if (NameInfo != NULL) {

                    ObjectName = NameInfo->Name;

                } else {

                    RtlInitUnicodeString( &ObjectName, NULL );
                }

                //
                //  Now compute the length needed for this entry.  This would
                //  be the size of the object directory information record,
                //  plus the size of the object name and object type name both
                //  null terminated.
                //

                LengthNeeded = sizeof( *DirInfo ) +
                               ObjectName.Length + sizeof( UNICODE_NULL ) +
                               ObjectHeader->Type->Name.Length + sizeof( UNICODE_NULL );

                //
                //  If there isn't enough room then take the following error
                //  path.   If the user wanted a single entry then tell the
                //  caller what length is really needed and say the buffer was
                //  too small.  Otherwise the user wanted multiple entries,
                //  so we'll just say there are more entries in the directory.
                //  In both cases we drop down the entry number because we
                //  weren't able to fit it in on this call
                //

                if ((TotalLengthNeeded + LengthNeeded) > Length) {

                    if (ReturnSingleEntry) {

                        TotalLengthNeeded += LengthNeeded;

                        Status = STATUS_BUFFER_TOO_SMALL;

                    } else {

                        Status = STATUS_MORE_ENTRIES;
                    }

                    EntryNumber -= 1;
                    goto querydone;
                }

                //
                //  The information will fit in the buffer.  So now fill
                //  in the output buffer.  We temporarily put in pointers
                //  to the name buffer as stored in the object and object
                //  type.  We copy the data buffer to the user buffer
                //  right before we return to the caller
                //

                try {

                    DirInfo->Name.Length            = ObjectName.Length;
                    DirInfo->Name.MaximumLength     = (USHORT)(ObjectName.Length+sizeof( UNICODE_NULL ));
                    DirInfo->Name.Buffer            = ObjectName.Buffer;

                    DirInfo->TypeName.Length        = ObjectHeader->Type->Name.Length;
                    DirInfo->TypeName.MaximumLength = (USHORT)(ObjectHeader->Type->Name.Length+sizeof( UNICODE_NULL ));
                    DirInfo->TypeName.Buffer        = ObjectHeader->Type->Name.Buffer;

                    Status = STATUS_SUCCESS;

                } except( EXCEPTION_EXECUTE_HANDLER ) {

                    Status = GetExceptionCode();
                }

                if (!NT_SUCCESS( Status )) {

                    goto querydone;
                }

                //
                //  Update the total number of bytes needed in this query.
                //  Push the dir info pointer to the next output location,
                //  and indicate how many entries we've processed
                //
                //

                TotalLengthNeeded += LengthNeeded;

                DirInfo++;
                EntriesFound++;

                //
                //  If we are to return only one entry then move on to the
                //  post processing phase, otherwise indicate that we're
                //  processing the next entry and go back to the top of
                //  the inner loop
                //

                if (ReturnSingleEntry) {

                    goto querydone;

                } else {

                    //
                    //  Bump the captured context by one entry.
                    //

                    CapturedContext++;
                }
            }

            //
            //  Get the next directory entry from the singly linked hash
            //  bucket chain
            //

            DirectoryEntry = DirectoryEntry->ChainLink;
        }
    }

    //
    //  At this point we've processed the directory entries and the first
    //  part of the output buffer now contains a bunch of object directory
    //  information records,  but the pointers in them refer to the wrong
    //  copies.  So now we have some fixup to do.
    //

querydone:

    try {

        //
        //  We'll only do this post processing if we've been successful
        //  so far.  Note that this means we could be returning in the
        //  user's output buffer system address that are meaningless, but
        //  then getting back an error status should tell the caller to
        //  forget about everything in the output buffer.  Given back
        //  a system address also isn't harmful because there is nothing
        //  that the user can really do with it.
        //

        if (NT_SUCCESS( Status )) {

            //
            //  Null terminate the string of object directory information
            //  records and point to where the actual names will go
            //

            RtlZeroMemory( DirInfo, sizeof( *DirInfo ));

            DirInfo++;

            NameBuffer = (PWCH)DirInfo;

            //
            //  Now for every entry that we've put in the output buffer
            //  DirInfo will point to the entry and EntriesFound kept the
            //  count.  Note that we are guaranteed space because of
            //  the math we did earlier in computing TotalLengthNeeded.
            //

            DirInfo = (POBJECT_DIRECTORY_INFORMATION)TempBuffer;

            while (EntriesFound--) {

                //
                //  Copy over the object name, set the dir info pointer into
                //  the user's buffer, then null terminate the string.  Note
                //  that we are really copying the data into our temp buffer
                //  but the pointer fix up is for the user's buffer which
                //  we'll copy into right after this loop.
                //

                RtlMoveMemory( NameBuffer,
                               DirInfo->Name.Buffer,
                               DirInfo->Name.Length );

                DirInfo->Name.Buffer = (PVOID)((ULONG_PTR)Buffer + ((ULONG_PTR)NameBuffer - (ULONG_PTR)TempBuffer));
                NameBuffer           = (PWCH)((ULONG_PTR)NameBuffer + DirInfo->Name.Length);
                *NameBuffer++        = UNICODE_NULL;

                //
                //  Do the same copy with the object type name
                //

                RtlMoveMemory( NameBuffer,
                               DirInfo->TypeName.Buffer,
                               DirInfo->TypeName.Length );

                DirInfo->TypeName.Buffer = (PVOID)((ULONG_PTR)Buffer + ((ULONG_PTR)NameBuffer - (ULONG_PTR)TempBuffer));
                NameBuffer               = (PWCH)((ULONG_PTR)NameBuffer + DirInfo->TypeName.Length);
                *NameBuffer++            = UNICODE_NULL;

                //
                //  Move on to the next dir info record
                //

                DirInfo++;
            }

            //
            //  Set the enumeration context to the entry number of the next
            //  entry to return.
            //

            *Context = EntryNumber;
        }

        //
        //  Copy over the results from our temp buffer to the users buffer.
        //  But adjust the amount copied just in case the total length needed
        //  exceeds the length we allocated.
        //

        RtlMoveMemory( Buffer,
                       TempBuffer,
                       (TotalLengthNeeded <= Length ? TotalLengthNeeded : Length) );

        //
        //  In all cases we'll tell the caller how much space if really needed
        //  provided the user asked for this information
        //

        if (ARGUMENT_PRESENT( ReturnLength )) {

            *ReturnLength = TotalLengthNeeded;
        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        //  Fall through, since we do not want to undo what we have done.
        //
    }

    //
    //  Unlock the directroy structures, dereference the directory object,
    //  free up our temp buffer, and return to our caller
    //

    ObpLeaveRootDirectoryMutex();

    ObDereferenceObject( Directory );

    ExFreePool( TempBuffer );

    return( Status );
}


PVOID
ObpLookupDirectoryEntry (
    IN POBJECT_DIRECTORY Directory,
    IN PUNICODE_STRING Name,
    IN ULONG Attributes
    )

/*++

Routine Description:

    This routine will lookup a single directory entry in a given directory.

    I believe this routine assumes that it is called with the root directory
    locked.

    Also note that this routine does not reference the returned object

Arguments:

    Directory - Supplies the directory being searched

    Name - Supplies the name of entry we're looking for

    Attributes - Indicates if the lookup should be case insensitive
        or not

Return Value:

    Returns a pointer to the corresponding object body if found and NULL
    otherwise.

--*/

{
    POBJECT_DIRECTORY_ENTRY *HeadDirectoryEntry;
    POBJECT_DIRECTORY_ENTRY DirectoryEntry;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PWCH Buffer;
    WCHAR Wchar;
    ULONG HashIndex;
    ULONG WcharLength;
    BOOLEAN CaseInSensitive;

    PAGED_CODE();

    //
    //  The caller needs to specify both a directory and a name otherwise
    //  we can't process the request
    //

    if (!Directory || !Name) {

        return( NULL ); // BUG BUG
    }

    //
    //  Set a local variable to tell us if the search is case sensitive
    //

    if (Attributes & OBJ_CASE_INSENSITIVE) {

        CaseInSensitive = TRUE;

    } else {

        CaseInSensitive = FALSE;
    }

    //
    //  Establish our local pointer to the input name buffer and get the
    //  number of unicode characters in the input name.  Also make sure
    //  the caller gave us a non null name
    //

    Buffer = Name->Buffer;
    WcharLength = Name->Length / sizeof( *Buffer );

    if (!WcharLength || !Buffer) {

        return( NULL ); // BUG BUG
    }

    //
    //  Compute the address of the head of the bucket chain for this name.
    //

    HashIndex = 0;
    while (WcharLength--) {

        Wchar = *Buffer++;
        HashIndex += (HashIndex << 1) + (HashIndex >> 1);

        if (Wchar < 'a') {

            HashIndex += Wchar;

        } else if (Wchar > 'z') {

            HashIndex += RtlUpcaseUnicodeChar( Wchar );

        } else {

            HashIndex += (Wchar - ('a'-'A'));
        }
    }

    HashIndex %= NUMBER_HASH_BUCKETS;

    HeadDirectoryEntry = (POBJECT_DIRECTORY_ENTRY *)&Directory->HashBuckets[ HashIndex ];

    Directory->LookupBucket = HeadDirectoryEntry;

    //
    //  Walk the chain of directory entries for this hash bucket, looking
    //  for either a match, or the insertion point if no match in the chain.
    //

    while ((DirectoryEntry = *HeadDirectoryEntry) != NULL) {

        //
        //  Get the object header and name from the object body
        //
        //  This function assumes the name must exist, otherwise it
        //  wouldn't be in a directory
        //

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( DirectoryEntry->Object );
        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

        //
        //  Compare strings using appropriate function.
        //

        if ((Name->Length == NameInfo->Name.Length) &&
            RtlEqualUnicodeString( Name,
                                   &NameInfo->Name,
                                   CaseInSensitive )) {

            //
            //  If name matches, then exit loop with DirectoryEntry
            //  pointing to matching entry.
            //

            break;
        }

        HeadDirectoryEntry = &DirectoryEntry->ChainLink;
    }

    //
    //  At this point, there are two possiblilities:
    //
    //   - we found an entry that matched and DirectoryEntry points to that
    //     entry.  Update the bucket chain so that the entry found is at the
    //     head of the bucket chain.  This is so the ObpDeleteDirectoryEntry
    //     and ObpInsertDirectoryEntry functions will work.  Also repeated
    //     lookups of the same name will succeed quickly.
    //
    //   - we did not find an entry that matched and DirectoryEntry is NULL.
    //

    if (DirectoryEntry) {

        Directory->LookupFound = TRUE;

        //
        //  The following convoluted piece of code moves a directory entry
        //  we've found to the front of the hash list.
        //

        if (HeadDirectoryEntry != Directory->LookupBucket) {

            *HeadDirectoryEntry = DirectoryEntry->ChainLink;
            DirectoryEntry->ChainLink = *(Directory->LookupBucket);
            *(Directory->LookupBucket) = DirectoryEntry;
        }

        //
        //  Now return the object to our caller
        //

        return( DirectoryEntry->Object );

    } else {

        //
        //  Otherwise we didn't find anything so return null
        //

        Directory->LookupFound = FALSE;

        return( NULL );
    }
}


BOOLEAN
ObpInsertDirectoryEntry (
    IN POBJECT_DIRECTORY Directory,
    IN PVOID Object
    )

/*++

Routine Description:

    This routine will insert a new directory entry into a directory
    object.  The directory must have already have been searched using
    ObpLookupDirectoryEntry because that routine sets the LookupBucket

Arguments:

    Directory - Supplies the directory object being modified.  This
        function assumes that we earlier did a lookup on the name
        that was successful or we just did an insertion

    Object - Supplies the object to insert into the directory

Return Value:

    TRUE if the object is inserted successfully and FALSE otherwise

--*/

{
    POBJECT_DIRECTORY_ENTRY *HeadDirectoryEntry;
    POBJECT_DIRECTORY_ENTRY NewDirectoryEntry;
    POBJECT_HEADER_NAME_INFO NameInfo;

    //
    //  Make sure we have a directory and that the last search was
    //  successful, meaning there is a lookupbuket
    //

    if (!Directory || Directory->LookupFound) {

        return( FALSE );
    }

    //
    //  Also verify that we have a good lookupbucket
    //

    HeadDirectoryEntry = Directory->LookupBucket;

    if (!HeadDirectoryEntry) {

        return( FALSE );
    }

    //
    //  Translate the object into a name info record, and make sure
    //  that the object has a name
    //

    NameInfo = OBJECT_HEADER_TO_NAME_INFO( OBJECT_TO_OBJECT_HEADER( Object ) );

    if (NameInfo == NULL) {

        return FALSE;
    }

    //
    //  Allocate memory for a new entry, and fail if not enough memory.
    //

    NewDirectoryEntry = (POBJECT_DIRECTORY_ENTRY)ExAllocatePoolWithTag( PagedPool,
                                                                        sizeof( OBJECT_DIRECTORY_ENTRY ),
                                                                        'iDbO' );

    if (NewDirectoryEntry == NULL) {

        return( FALSE );
    }

    //
    //  Link the new entry into the chain at the insertion point.
    //  This puts the new object right at the head of the current
    //  hash bucket chain
    //

    NewDirectoryEntry->ChainLink = *HeadDirectoryEntry;
    *HeadDirectoryEntry = NewDirectoryEntry;
    NewDirectoryEntry->Object = Object;

    //
    //  Point the object header back to the directory we just inserted
    //  it into.
    //

    NameInfo->Directory = Directory;

    //
    //  Return success.
    //

    Directory->LookupFound = TRUE;

    return( TRUE );
}


BOOLEAN
ObpDeleteDirectoryEntry (
    IN POBJECT_DIRECTORY Directory
    )

/*++

Routine Description:

    This routine deletes the most recently found directory entry from
    the specified directory object.  It will only succeed after a
    successful ObpLookupDirectoryEntry call.

Arguments:

    Directory - Supplies the directory being modified

Return Value:

    TRUE if the deletion succeeded and FALSE otherwise

--*/

{
    POBJECT_DIRECTORY_ENTRY *HeadDirectoryEntry;
    POBJECT_DIRECTORY_ENTRY DirectoryEntry;

    //
    //  Make sure we have a directory and that it has a found entry
    //

    if (!Directory || !Directory->LookupFound) {

        return( FALSE );
    }

    //
    //  Also make sure that the lookup bucket is valid
    //

    HeadDirectoryEntry = Directory->LookupBucket;

    if (!HeadDirectoryEntry) {

        return( FALSE );
    }

    DirectoryEntry = *HeadDirectoryEntry;

    if (!DirectoryEntry) {

        return( FALSE );
    }

    //
    //  Unlink the entry from the head of the bucket chain and free the
    //  memory for the entry.
    //

    *HeadDirectoryEntry = DirectoryEntry->ChainLink;
    DirectoryEntry->ChainLink = NULL;

    ExFreePool( DirectoryEntry );

    //
    //  Return success
    //

    return( TRUE );
}


NTSTATUS
ObpLookupObjectName (
    IN HANDLE RootDirectoryHandle OPTIONAL,
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN PVOID ParseContext OPTIONAL,
    IN PSECURITY_QUALITY_OF_SERVICE SecurityQos OPTIONAL,
    IN PVOID InsertObject OPTIONAL,
    IN OUT PACCESS_STATE AccessState,
    OUT PBOOLEAN DirectoryLocked,
    OUT PVOID *FoundObject
    )

/*++

Routine Description:

    This function will search a given directoroy for a specified
    object name.  It will also create a new object specified by
    InsertObject.

Arguments:

    RootDirectoryHandle - Optionally supplies the directory being
        searched.  If not supplied then this routine searches
        the root directory

    ObjectName - Supplies the name of object to lookup

    Attributes - Specifies the attributes for the lookup (e.g., case
        insensitive)

    ObjectType - Specifies the type of the object to lookup

    AccessMode - Specifies the callers processor mode

    ParseContext - Optionally supplies a parse context that is blindly
        passed to the parse callback routines

    SecurityQos - Optionally supplies a pointer to the passed Security
        Quality of Service parameter that is blindly passed to the parse
        callback routines

    InsertObject - Optionally supplies the object we think will be found.
        This is used if the caller did not give a root directory handle
        and the object name is "\" and the root object directory hasn't
        been created yet.  In other cases where we wind up creating
        a new directory entry this is the object inserted.

    AccessState - Current access state, describing already granted access
        types, the privileges used to get them, and any access types yet to
        be granted.  The access masks may not contain any generic access
        types.

    DirectoryLocked - Receives an indication if this routine has returned
        with the input directory locked

    FoundObject - Receives a pointer to the object body if found

Return Value:

    An appropriate status value

--*/

{
    POBJECT_DIRECTORY RootDirectory;
    POBJECT_DIRECTORY Directory;
    POBJECT_DIRECTORY ParentDirectory = NULL;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PDEVICE_MAP DeviceMap = NULL;
    PVOID Object;
    UNICODE_STRING RemainingName;
    UNICODE_STRING ComponentName;
    PWCH NewName;
    NTSTATUS Status;
    BOOLEAN Reparse;
    ULONG MaxReparse = OBJ_MAX_REPARSE_ATTEMPTS;
    OB_PARSE_METHOD ParseProcedure;
    extern POBJECT_TYPE IoFileObjectType;

    ObpValidateIrql( "ObpLookupObjectName" );

    //
    //  Initialize our output variables to say we haven't lock or found
    //  anything but we were successful at it
    //

    *DirectoryLocked = FALSE;
    *FoundObject = NULL;
    Status = STATUS_SUCCESS;

    Object = NULL;

    //
    //  Check if the caller has given us a directory to search.  Otherwise
    //  we'll search the root object directory
    //

    if (ARGUMENT_PRESENT( RootDirectoryHandle )) {

        //
        //  Otherwise reference the directory object and make sure
        //  that we successfully got the object
        //

        Status = ObReferenceObjectByHandle( RootDirectoryHandle,
                                            0,
                                            NULL,
                                            AccessMode,
                                            (PVOID *)&RootDirectory,
                                            NULL );

        if (!NT_SUCCESS( Status )) {

            return( Status );
        }

        //
        //  Translate the directory object to its object header
        //

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( RootDirectory );

        //
        //  Now if the name we're looking up starts with a "\" and it
        //  does not have a parse procedure then the syntax is bad
        //

        if ((ObjectName->Buffer != NULL) &&
            (*(ObjectName->Buffer) == OBJ_NAME_PATH_SEPARATOR) &&
            (ObjectHeader->Type != IoFileObjectType)) {

            ObDereferenceObject( RootDirectory );

            return( STATUS_OBJECT_PATH_SYNTAX_BAD );
        }

        //
        //  Now make sure that we do not have the directory of the
        //  object types
        //

        if (ObjectHeader->Type != ObpDirectoryObjectType) {

            //
            //  We have an object directory that is not the object type
            //  directory.  So now if it doesn't have a parse routine
            //  then there is nothing we can
            //

            if (ObjectHeader->Type->TypeInfo.ParseProcedure == NULL) {

                ObDereferenceObject( RootDirectory );

                return( STATUS_INVALID_HANDLE );

            } else {

                MaxReparse = OBJ_MAX_REPARSE_ATTEMPTS;

                //
                //  The following loop cycles cycles through the various
                //  parse routine to we could encounter trying to resolve
                //  this name through symbolic links.
                //

                while (TRUE) {

                    KIRQL SaveIrql;

                    RemainingName = *ObjectName;

                    //
                    //  Invoke the callback routine to parse the remaining
                    //  object name
                    //

                    ObpBeginTypeSpecificCallOut( SaveIrql );

                    Status = (*ObjectHeader->Type->TypeInfo.ParseProcedure)( RootDirectory,
                                                                             ObjectType,
                                                                             AccessState,
                                                                             AccessMode,
                                                                             Attributes,
                                                                             ObjectName,
                                                                             &RemainingName,
                                                                             ParseContext,
                                                                             SecurityQos,
                                                                             &Object );

                    ObpEndTypeSpecificCallOut( SaveIrql, "Parse", ObjectHeader->Type, Object );

                    //
                    //  If the status was not to do a reparse and the lookup
                    //  was not successful then we found nothing so we
                    //  dereference the directory and return the status to
                    //  our caller.  If the object we got back was null then
                    //  we'll tell our caller that we couldn't find the name.
                    //  Lastly if we did not get a reparse and we were
                    //  successful and the object is not null then everything
                    //  gets nicely returned to our caller
                    //

                    if ( ( Status != STATUS_REPARSE ) && 
                         ( Status != STATUS_REPARSE_OBJECT )) {

                        if (!NT_SUCCESS( Status )) {

                            Object = NULL;

                        } else if (Object == NULL) {

                            Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        }
    
                        ObDereferenceObject( RootDirectory );

                        *FoundObject = Object;
                        
                        return( Status );

                    //
                    //  We got a status reparse, which means the object
                    //  name has been modified to have use start all over
                    //  again.  If the reparse target is now empty or it
                    //  is a path separator then we start the parse at the
                    //  root directory
                    //

                    } else if ((ObjectName->Length == 0) ||
                               (ObjectName->Buffer == NULL) ||
                               (*(ObjectName->Buffer) == OBJ_NAME_PATH_SEPARATOR)) {

                        //
                        //  Restart the parse relative to the root directory.
                        //

                        ObDereferenceObject( RootDirectory );

                        RootDirectory = ObpRootDirectoryObject;
                        RootDirectoryHandle = NULL;

                        break;

                    //
                    //  We got a reparse and we actually have a new name to
                    //  go to we if we haven't exhausted our reparse attempts
                    //  yet then just continue to the top of this loop.
                    //

                    } else if (--MaxReparse) {

                        continue;

                    //
                    //  We got a reparse and we've exhausted our times through
                    //  the loop so we'll return what we found.
                    //
                    //  **** this doesn't seem right.  It probably should be
                    //  an error
                    //

                    } else {

                        ObDereferenceObject( RootDirectory );

                        *FoundObject = Object;

                        //
                        //  At this point we were failing in stress by
                        //  returning to the caller with a success status but
                        //  a null object pointer.
                        //

                        if (Object == NULL) {

                            Status = STATUS_OBJECT_NAME_NOT_FOUND;
                        }

                        return( Status );
                    }
                }
            }

        //
        //  At this point the caller has given us the directory of object
        //  types.  If the caller didn't specify a name then we'll return
        //  a pointer to the root object directory.
        //

        } else if ((ObjectName->Length == 0) ||
                   (ObjectName->Buffer == NULL)) {

            Status = ObReferenceObjectByPointer( RootDirectory,
                                                 0,
                                                 ObjectType,
                                                 AccessMode );

            if (NT_SUCCESS( Status )) {

                Object = RootDirectory;
            }

            ObDereferenceObject( RootDirectory );

            *FoundObject = Object;

            return( Status );
        }

    //
    //  Otherwise the caller did not specify a directory to search so
    //  we'll default to the object root directory
    //

    } else {

        RootDirectory = ObpRootDirectoryObject;

        //
        //  If the name we're looking for is empty then it is illformed.
        //  Also it has to start with a "\" or it is illformed.
        //

        if ((ObjectName->Length == 0) ||
            (ObjectName->Buffer == NULL) ||
            (*(ObjectName->Buffer) != OBJ_NAME_PATH_SEPARATOR)) {

            return( STATUS_OBJECT_PATH_SYNTAX_BAD );
        }

        //
        //  Check if the name is has only one character (that is the "\")
        //  Which means that the caller really just wants to lookup the
        //  root directory.
        //

        if (ObjectName->Length == sizeof( OBJ_NAME_PATH_SEPARATOR )) {

            //
            //  If there is not a root directory yet.  Then we really
            //  can't return it, however if the caller specified
            //  an insert object that is the one we'll reference and
            //  return to our caller
            //

            if (!RootDirectory) {

                if (InsertObject) {

                    Status = ObReferenceObjectByPointer( InsertObject,
                                                         0,
                                                         ObjectType,
                                                         AccessMode );

                    if (NT_SUCCESS( Status )) {

                        *FoundObject = InsertObject;
                    }

                    return( Status );

                } else {

                    return( STATUS_INVALID_PARAMETER );
                }

            //
            //  At this point the caller did not specify a root directory,
            //  the name is "\" and the root object directory exists so
            //  we'll simply return the real root directory object
            //

            } else {

                Status = ObReferenceObjectByPointer( RootDirectory,
                                                     0,
                                                     ObjectType,
                                                     AccessMode );

                if (NT_SUCCESS( Status )) {

                    *FoundObject = RootDirectory;
                }

                return( Status );
            }

        //
        //  At this pointer the caller did not specify a root directory,
        //  and the name is more than just a "\"
        //
        //  Now if the lookup is case insensitive, and the name buffer is a
        //  legitimate pointer (meaning that is it quadword aligned), and
        //  there is a dos device map for the process.  Then we'll handle
        //  the situation here. First get the device map and make sure it
        //  doesn't go away while we're using it.
        //

        } else {

            KIRQL OldIrql;

            ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );

            if ((DeviceMap = PsGetCurrentProcess()->DeviceMap) != NULL) {

                DeviceMap->ReferenceCount++;
                ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );

                if (!((ULONG_PTR)(ObjectName->Buffer) & (sizeof(ULONGLONG)-1))

                            &&

                    (DeviceMap->DosDevicesDirectory != NULL )) {

                    //
                    //  Check if the object name is actually equal to the
                    //  global dos devices short name prefix "\??\"
                    //

                    if ((ObjectName->Length >= ObpDosDevicesShortName.Length)

                            &&

                        (*(PULONGLONG)(ObjectName->Buffer) == ObpDosDevicesShortNamePrefix.QuadPart)) {

                        //
                        //  The user gave us the dos short name prefix so we'll
                        //  look down the directory, and start the search at the
                        //  dos device directory
                        //

                        *DirectoryLocked = TRUE;

                        ObpEnterRootDirectoryMutex();

                        ParentDirectory = RootDirectory;

                        Directory = DeviceMap->DosDevicesDirectory;

                        RemainingName = *ObjectName;
                        RemainingName.Buffer += (ObpDosDevicesShortName.Length / sizeof( WCHAR ));
                        RemainingName.Length -= ObpDosDevicesShortName.Length;

                        goto quickStart;

                    //
                    //  The name is not equal to "\??\" but check if it is
                    //  equal to "\??"
                    //

                    } else if ((ObjectName->Length == ObpDosDevicesShortName.Length - sizeof( WCHAR ))

                                    &&

                               (*(PULONG)(ObjectName->Buffer) == ObpDosDevicesShortNameRoot.LowPart)

                                    &&

                               (*((PWCHAR)(ObjectName->Buffer)+2) == (WCHAR)(ObpDosDevicesShortNameRoot.HighPart))) {

                        //
                        //  The user specified "\??" so we return to dos devices
                        //  directory to our caller
                        //

                        Status = ObReferenceObjectByPointer( DeviceMap->DosDevicesDirectory,
                                                             0,
                                                             ObjectType,
                                                             AccessMode );

                        if (NT_SUCCESS( Status )) {

                            *FoundObject = DeviceMap->DosDevicesDirectory;
                        }

                        //
                        //  Dereference the Device Map
                        //

                        {
                            KIRQL OldIrql;

                            ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );

                            DeviceMap->ReferenceCount--;

                            if (DeviceMap->ReferenceCount == 0) {

                                ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );

                                DeviceMap->DosDevicesDirectory->DeviceMap = NULL;
                                ObDereferenceObject( DeviceMap->DosDevicesDirectory );

                                ExFreePool( DeviceMap );

                            } else {

                                ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );
                            }
                        }

                        return( Status );
                    }
                }

            } else {

                ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );
            }
        }
    }

    //
    //  At this point either
    //
    //  the user specified a directory that is not the object
    //  type directory and got repase back to the root directory
    //
    //  the user specified the object type directory and gave us
    //  a name to actually look up
    //
    //  the user did not specify a search directory (default
    //  to root object directory) and if the name did start off
    //  with the dos device prefix we've munged outselves back to
    //  it to the dos device directory for the process
    //

    Reparse = TRUE;
    MaxReparse = OBJ_MAX_REPARSE_ATTEMPTS;

    while (Reparse) {

        RemainingName = *ObjectName;

quickStart:

        Reparse = FALSE;

        while (TRUE) {

            Object = NULL;

            //if (RemainingName.Length == 0) {
            //    Status = STATUS_OBJECT_NAME_INVALID;
            //    break;
            //    }

            //
            //  If the remaining name for the object starts with a
            //  "\" then just gobble up the "\"
            //

            if ( (RemainingName.Length != 0) &&
                 (*(RemainingName.Buffer) == OBJ_NAME_PATH_SEPARATOR) ) {

                RemainingName.Buffer++;
                RemainingName.Length -= sizeof( OBJ_NAME_PATH_SEPARATOR );
            }

            //
            //  The following piece of code will calculate the first
            //  component of the remaining name.  If there is not
            //  a remaining component then the object name is illformed
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

                Status = STATUS_OBJECT_NAME_INVALID;
                break;
            }

            //
            //  Now we have the first component name to lookup so we'll
            //  look the directory is necessary
            //

            if (!*DirectoryLocked) {

                *DirectoryLocked = TRUE;
                ObpEnterRootDirectoryMutex();
                Directory = RootDirectory;
            }

            //
            //  Now if the caller does not have traverse privilege and
            //  there is a parent directory then we must check if the
            //  user has traverse access to the directory.  Our local
            //  Reparse variable should be false at this point so we'll
            //  drop out of both loops
            //

            if ( !(AccessState->Flags & TOKEN_HAS_TRAVERSE_PRIVILEGE) && (ParentDirectory != NULL) ) {

                if (!ObpCheckTraverseAccess( ParentDirectory,
                                             DIRECTORY_TRAVERSE,
                                             AccessState,
                                             FALSE,
                                             AccessMode,
                                             &Status )) {

                    break;
                }
            }

            //
            //  If the object already exists in this directory, find it,
            //  else return NULL.
            //

            Object = ObpLookupDirectoryEntry( Directory, &ComponentName, Attributes );

            if (!Object) {

                //
                //  We didn't find the object.  If there is some remaining
                //  name left (meaning the component name is a directory in
                //  path we trying to break) or the caller didn't specify an
                //  insert object then we then we'll break out here with an
                //  error status
                //

                if (RemainingName.Length != 0) {

                    Status = STATUS_OBJECT_PATH_NOT_FOUND;
                    break;
                }

                if (!InsertObject) {

                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    break;
                }

                //
                //  Check that the caller has the access to the directory
                //  to either create a subdirectory (in the object type
                //  directory) or to create an object of the given component
                //  name.  If the call fails then we'll break out of here
                //  with the status value set
                //

                if (!ObCheckCreateObjectAccess( Directory,
                                                ObjectType == ObpDirectoryObjectType ?
                                                        DIRECTORY_CREATE_SUBDIRECTORY :
                                                        DIRECTORY_CREATE_OBJECT,
                                                AccessState,
                                                &ComponentName,
                                                FALSE,
                                                AccessMode,
                                                &Status )) {

                    break;
                }

                //
                //  The object does not exist in the directory and
                //  we are allowed to create one.  So allocate space
                //  for the name and insert the name into the directory
                //

                NewName = ExAllocatePoolWithTag( PagedPool, ComponentName.Length, 'mNbO' );

                if ((NewName == NULL) ||
                    !ObpInsertDirectoryEntry( Directory, InsertObject )) {

                    if (NewName != NULL) {

                        ExFreePool( NewName );
                    }

                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                //
                //  We have an insert object so now get its name info,
                //  because we are going to change its name and insert it
                //  into the directory
                //

                ObReferenceObject( InsertObject );

                ObjectHeader = OBJECT_TO_OBJECT_HEADER( InsertObject );

                NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

                ObReferenceObject( Directory );

                RtlMoveMemory( NewName,
                               ComponentName.Buffer,
                               ComponentName.Length );

                if (NameInfo->Name.Buffer) {

                    ExFreePool( NameInfo->Name.Buffer );
                }

                NameInfo->Name.Buffer = NewName;
                NameInfo->Name.Length = ComponentName.Length;
                NameInfo->Name.MaximumLength = ComponentName.Length;

                Object = InsertObject;

                Status = STATUS_SUCCESS;

                break;
            }

            //
            //  At this point we've found the component name within
            //  the directory.  So we'll now grab the components object
            //  header, and get its parse routine
            //

ReparseObject:

            ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
            ParseProcedure = ObjectHeader->Type->TypeInfo.ParseProcedure;

            //
            //  Now if there is a parse routine for the type and we are not
            //  inserting a new object or the parse routine is for symbolic
            //  links then we'll actually call the parse routine
            //

            if (ParseProcedure && (!InsertObject || (ParseProcedure == ObpParseSymbolicLink))) {

                KIRQL SaveIrql;

                //
                //  Reference the object and then free the directory lock
                //  This will keep the object from going away with the
                //  directory unlocked
                //

                ObpIncrPointerCount( ObjectHeader );

                ASSERT(*DirectoryLocked);

                ObpLeaveRootDirectoryMutex();

                *DirectoryLocked = FALSE;

                ObpBeginTypeSpecificCallOut( SaveIrql );

                //
                //  Call the objects parse routine
                //

                Status = (*ParseProcedure)( Object,
                                            (PVOID)ObjectType,
                                            AccessState,
                                            AccessMode,
                                            Attributes,
                                            ObjectName,
                                            &RemainingName,
                                            ParseContext,
                                            SecurityQos,
                                            &Object );

                ObpEndTypeSpecificCallOut( SaveIrql, "Parse", ObjectHeader->Type, Object );

                //
                //  We can now decrement the object reference count
                //

                ObDereferenceObject( &ObjectHeader->Body );

                //
                //  Check if we have some reparsing to do
                //

                if ((Status == STATUS_REPARSE) || (Status == STATUS_REPARSE_OBJECT)) {

                    //
                    //  See if we've reparsed too many times already and if
                    //  so we'll fail the request
                    //

                    if (--MaxReparse) {

                        //
                        //  Tell the outer loop to continue looping
                        //

                        Reparse = TRUE;

                        //
                        //  Check if we have a reparse object or the name
                        //  starts with a "\"
                        //

                        if ((Status == STATUS_REPARSE_OBJECT) ||
                            (*(ObjectName->Buffer) == OBJ_NAME_PATH_SEPARATOR)) {

                            //
                            //  If the user specified a start directory then
                            //  remove this information because we're taking
                            //  a reparse point to someplace else
                             //

                            if (ARGUMENT_PRESENT( RootDirectoryHandle )) {

                                ObDereferenceObject( RootDirectory );
                                RootDirectoryHandle = NULL;
                            }

                            //
                            //  And where we start is the root directory
                            //  object
                            //

                            ParentDirectory = NULL;
                            RootDirectory = ObpRootDirectoryObject;

                            //
                            //  Now if this is a reparse object (means we have
                            //  encountered a symbolic link that has already been
                            //  snapped so we have an object and remaining
                            //  name that need to be examined) and we didn't
                            //  find an object from the parse routine object
                            //  break out of both loops.
                            //

                            if (Status == STATUS_REPARSE_OBJECT) {

                                Reparse = FALSE;

                                if (Object == NULL) {

                                    Status = STATUS_OBJECT_NAME_NOT_FOUND;

                                } else {

                                    //
                                    //  At this point we have a reparse object
                                    //  so we'll look the directory down and
                                    //  parse the new object
                                    //

                                    *DirectoryLocked = TRUE;
                                    ObpEnterRootDirectoryMutex();

                                    goto ReparseObject;
                                }
                            }

                        //
                        //  We did not have a reparse object and the name
                        //  does not start with a "\".  Meaning we got back
                        //  STATUS_REPASE, so now check if the directory
                        //  is the root object directory and if so then
                        //  we didn't the name otherwise we'll drop out of
                        //  the inner loop and reparse true to get back to
                        //  outer loop
                        //

                        } else if (RootDirectory == ObpRootDirectoryObject) {

                            Object = NULL;
                            Status = STATUS_OBJECT_NAME_NOT_FOUND;

                            Reparse = FALSE;
                        }

                    } else {

                        //
                        //  **** this should probably be a differnt error
                        //  status related to too many reparse points
                        //

                        Object = NULL;
                        Status = STATUS_OBJECT_NAME_NOT_FOUND;
                    }

                //
                //  We are not reparsing and if we did not get success then
                //  the object is null and we'll break out of our loops
                //

                } else if (!NT_SUCCESS( Status )) {

                    Object = NULL;

                //
                //  We are not reparsing and we got back success but check
                //  if the object is null because that means we really didn't
                //  find the object, and then break out of our loops
                //
                //  If the object is not null then we've been successful and
                //  prosperous so break out with the object set.
                //

                } else if (Object == NULL) {

                    Status = STATUS_OBJECT_NAME_NOT_FOUND;
                }

                break;

            } else {

                //
                //  At this point we do not have a parse routine or if there
                //  is a parse routine it is not for symbolic links or there
                //  may not be a specified insert object
                //
                //  Check to see if we have exhausted the remaining name
                //

                if (RemainingName.Length == 0) {

                    //
                    //  Check if the caller specified an object to insert.
                    //  If specified then we'll break out of our loops with
                    //  the object that we've found
                    //

                    if (!InsertObject) {

                        //
                        //  The user did not specify an insert object
                        //  so we're opening an existing object.  Make sure
                        //  we have traverse access to the container
                        //  directory.
                        //

                        if ( !(AccessState->Flags & TOKEN_HAS_TRAVERSE_PRIVILEGE) ) {

                            if (!ObpCheckTraverseAccess( Directory,
                                                         DIRECTORY_TRAVERSE,
                                                         AccessState,
                                                         FALSE,
                                                         AccessMode,
                                                         &Status )) {

                                Object = NULL;
                                break;
                            }
                        }

                        Status = ObReferenceObjectByPointer( Object,
                                                             0,
                                                             ObjectType,
                                                             AccessMode );

                        if (!NT_SUCCESS( Status )) {

                            Object = NULL;
                        }
                    }

                    break;

                } else {

                    //
                    //  There is some name remaining names to process
                    //  if the directory we're looking at is the
                    //  directory of object types and set ourselves
                    //  up to parse it all over again.
                    //

                    if (ObjectHeader->Type == ObpDirectoryObjectType) {

                        ParentDirectory = Directory;
                        Directory = (POBJECT_DIRECTORY)Object;

                    } else {

                        //
                        //  Otherwise there has been a mismatch so we'll
                        //  set our error status and break out of the
                        //  loops
                        //

                        Status = STATUS_OBJECT_TYPE_MISMATCH;
                        Object = NULL;

                        break;
                    }
                }
            }
        }
    }

    //
    //  If the device map has been referenced then dereference it
    //

    if (DeviceMap != NULL) {

        KIRQL OldIrql;

        ExAcquireSpinLock( &ObpDeviceMapLock, &OldIrql );

        DeviceMap->ReferenceCount--;

        if (DeviceMap->ReferenceCount == 0) {

            ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );

            DeviceMap->DosDevicesDirectory->DeviceMap = NULL;
            ObDereferenceObject( DeviceMap->DosDevicesDirectory );

            ExFreePool( DeviceMap );

        } else {

            ExReleaseSpinLock( &ObpDeviceMapLock, OldIrql );
        }
    }

    //
    //  At this point we've parsed the object name as much as possible
    //  going through symbolic links as necessary.  So now set the
    //  output object pointer, and if we really did not find an object
    //  then we might need to modify the error status.  If the
    //  status was repase or some success status then translate it
    //  to name not found.
    //

    if (!(*FoundObject = Object)) {

        if (Status == STATUS_REPARSE) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;

        } else if (NT_SUCCESS( Status )) {

            Status = STATUS_OBJECT_NAME_NOT_FOUND;
        }
    }

    //
    //  If the caller gave us a root directory to search (and we didn't
    //  zero out this value) then free up our reference
    //

    if (ARGUMENT_PRESENT( RootDirectoryHandle )) {

        ObDereferenceObject( RootDirectory );
        RootDirectoryHandle = NULL;
    }

    //
    //  And return to our caller
    //

    return( Status );
}


