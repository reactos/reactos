/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obtype.c

Abstract:

    Object type routines.

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,ObCreateObjectType)
#pragma alloc_text(PAGE,ObGetObjectInformation)
#endif

/*

 IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT

 There is currently no system service that permits changing
 the security on an object type object.  Consequently, the object
 manager does not check to make sure that a subject is allowed
 to create an object of a given type.

 Should such a system service be added, the following section of
 code must be re-enabled in obhandle.c:

        //
        // Perform access check to see if we are allowed to create
        // an instance of this object type.
        //
        // This routine will audit the attempt to create the
        // object as appropriate.  Note that this is different
        // from auditing the creation of the object itself.
        //

        if (!ObCheckCreateInstanceAccess( ObjectType,
                                          OBJECT_TYPE_CREATE,
                                          AccessState,
                                          TRUE,
                                          AccessMode,
                                          &Status
                                        ) ) {
            return( Status );

            }

 The code is already there, but is not compiled.

 This will ensure that someone who is denied access to an object
 type is not permitted to create an object of that type.

*/


NTSTATUS
ObCreateObjectType (
    IN PUNICODE_STRING TypeName,
    IN POBJECT_TYPE_INITIALIZER ObjectTypeInitializer,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor OPTIONAL, // currently ignored
    OUT POBJECT_TYPE *ObjectType
    )

/*++

Routine Description:

    This routine creates a new object type.

Arguments:

    TypeName - Supplies the name of the new object type

    ObjectTypeInitializer - Supplies a object initialization
        structure.  This structure denotes the default object
        behavior including callbacks.

    SecurityDescriptor - Currently ignored

    ObjectType - Receives a pointer to the newly created object
        type.

Return Value:

    An appropriate NTSTATUS value.

--*/

{
    POOL_TYPE PoolType;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER NewObjectTypeHeader;
    POBJECT_TYPE NewObjectType;
    ULONG i;
    UNICODE_STRING ObjectName;
    PWCH s;
    NTSTATUS Status;
    ULONG StandardHeaderCharge;

    ObpValidateIrql( "ObCreateObjectType" );

    //
    //  Return an error if invalid type attributes or no type name specified.
    //  No type name is okay if the type directory object does not exist
    //  yet (see init.c).
    //

    PoolType = ObjectTypeInitializer->PoolType;

    if ((!TypeName)

            ||

        (!TypeName->Length)

            ||

        (TypeName->Length % sizeof( WCHAR ))

            ||

        (ObjectTypeInitializer == NULL)

            ||

        (ObjectTypeInitializer->InvalidAttributes & ~OBJ_VALID_ATTRIBUTES)

            ||

        (ObjectTypeInitializer->Length != sizeof( *ObjectTypeInitializer ))

            ||

        (ObjectTypeInitializer->MaintainHandleCount &&
            (ObjectTypeInitializer->OpenProcedure == NULL &&
             ObjectTypeInitializer->CloseProcedure == NULL ))

            ||

        ((!ObjectTypeInitializer->UseDefaultObject) &&
            (PoolType != NonPagedPool))) {

        return( STATUS_INVALID_PARAMETER );
    }

    //
    //  Make sure that the type name does not contain an
    //  path name separator
    //

    s = TypeName->Buffer;
    i = TypeName->Length / sizeof( WCHAR );

    while (i--) {

        if (*s++ == OBJ_NAME_PATH_SEPARATOR) {

            return( STATUS_OBJECT_NAME_INVALID );
        }
    }

    //
    //  See if TypeName string already exists in the \ObjectTypes directory
    //  Return an error if it does.  Otherwise add the name to the directory.
    //  Note that there may not necessarily be a type directory.
    //

    if (ObpTypeDirectoryObject) {

        ObpEnterRootDirectoryMutex();

        if (ObpLookupDirectoryEntry( ObpTypeDirectoryObject,
                                     TypeName,
                                     OBJ_CASE_INSENSITIVE )) {

            ObpLeaveRootDirectoryMutex();

            return( STATUS_OBJECT_NAME_COLLISION );
        }
    }

    //
    //  Allocate a buffer for the type name and then
    //  copy over the name
    //

    ObjectName.Buffer = ExAllocatePoolWithTag( PagedPool,
                                               (ULONG)TypeName->MaximumLength,
                                               'mNbO' );

    if (ObjectName.Buffer == NULL) {

        if (ObpTypeDirectoryObject)

            ObpLeaveRootDirectoryMutex();

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    ObjectName.MaximumLength = TypeName->MaximumLength;

    RtlCopyUnicodeString( &ObjectName, TypeName );

    //
    //  Allocate memory for the object
    //

    Status = ObpAllocateObject( NULL,
                                KernelMode,
                                ObpTypeObjectType,
                                &ObjectName,
                                sizeof( OBJECT_TYPE ),
                                &NewObjectTypeHeader );

    if (!NT_SUCCESS( Status )) {

        if (ObpTypeDirectoryObject)

            ObpLeaveRootDirectoryMutex();

        ExFreePool(ObjectName.Buffer);

        return( Status );
    }

    //
    //  Initialize the create attributes, object ownership. parse context,
    //  and object body pointer.
    //
    //  N.B. This is required since these fields are not initialized.
    //

    NewObjectTypeHeader->Flags |= OB_FLAG_KERNEL_OBJECT |
                                  OB_FLAG_PERMANENT_OBJECT;

    NewObjectType = (POBJECT_TYPE)&NewObjectTypeHeader->Body;
    NewObjectType->Name = ObjectName;

    //
    //  The following call zeros out the number of handles and objects
    //  field plus high water marks
    //

    RtlZeroMemory( &NewObjectType->TotalNumberOfObjects,
                   FIELD_OFFSET( OBJECT_TYPE, TypeInfo ) -
                   FIELD_OFFSET( OBJECT_TYPE, TotalNumberOfObjects ));

    //
    //  If there is not a type object type yet then this must be
    //  that type (i.e., type object type must be the first object type
    //  ever created.  Consequently we'll need to setup some self
    //  referencing pointers.
    //

    if (!ObpTypeObjectType) {

        ObpTypeObjectType = NewObjectType;
        NewObjectTypeHeader->Type = ObpTypeObjectType;
        NewObjectType->TotalNumberOfObjects = 1;

#ifdef POOL_TAGGING

        NewObjectType->Key = 'TjbO';

    } else {

        //
        //  Otherwise this is not the type object type so we'll
        //  try and generate a tag for the new object type provided
        //  pool tagging is turned on.
        //

        ANSI_STRING AnsiName;

        if (NT_SUCCESS( RtlUnicodeStringToAnsiString( &AnsiName, TypeName, TRUE ) )) {

            for (i=3; i>=AnsiName.Length; i--) {

                AnsiName.Buffer[ i ] = ' ';

            }

            NewObjectType->Key = *(PULONG)AnsiName.Buffer;
            ExFreePool( AnsiName.Buffer );

        } else {

            NewObjectType->Key = *(PULONG)TypeName->Buffer;
        }

#endif //POOL_TAGGING

    }

    //
    //  Continue initializing the new object type fields
    //

    NewObjectType->TypeInfo = *ObjectTypeInitializer;
    NewObjectType->TypeInfo.PoolType = PoolType;

    if (NtGlobalFlag & FLG_MAINTAIN_OBJECT_TYPELIST) {

        NewObjectType->TypeInfo.MaintainTypeList = TRUE;
    }

    //
    //  Whack quotas passed in so that headers are properly charged
    //
    //  Quota for object name is charged independently
    //

    StandardHeaderCharge = sizeof( OBJECT_HEADER ) +
                           sizeof( OBJECT_HEADER_NAME_INFO ) +
                           (ObjectTypeInitializer->MaintainHandleCount ?
                                sizeof( OBJECT_HEADER_HANDLE_INFO )
                              : 0 );

    if ( PoolType == NonPagedPool ) {

        NewObjectType->TypeInfo.DefaultNonPagedPoolCharge += StandardHeaderCharge;

    } else {

        NewObjectType->TypeInfo.DefaultPagedPoolCharge += StandardHeaderCharge;
    }

    //
    //  If there is not an object type specific security procedure then set
    //  the default one supplied by Se.
    //

    if (ObjectTypeInitializer->SecurityProcedure == NULL) {

        NewObjectType->TypeInfo.SecurityProcedure = SeDefaultObjectMethod;
    }

    //
    //  Initialize the object type lock and its list of objects created
    //  of this type
    //

    ExInitializeResourceLite( &NewObjectType->Mutex );

    InitializeListHead( &NewObjectType->TypeList );

    //
    //  If we are to use the default object (meaning that we'll have our
    //  private event as our default object) then the type must allow
    //  synchronize and we'll set the default object
    //

    if (NewObjectType->TypeInfo.UseDefaultObject) {

        NewObjectType->TypeInfo.ValidAccessMask |= SYNCHRONIZE;
        NewObjectType->DefaultObject = &ObpDefaultObject;

    //
    //  Otherwise if this is the type file object then we'll put
    //  in the offset to the event of a file object.
    //
    //  **** What a hack
    //

    } else if (ObjectName.Length == 8 && !wcscmp( ObjectName.Buffer, L"File" )) {

        NewObjectType->DefaultObject = ULongToPtr( FIELD_OFFSET( FILE_OBJECT, Event ) );


    //
    // If this is a waitable port, set the offset to the event in the
    // waitableport object.  Another hack
    //

    } else if ( ObjectName.Length == 24 && !wcscmp( ObjectName.Buffer, L"WaitablePort")) {

        NewObjectType->DefaultObject = ULongToPtr( FIELD_OFFSET( LPCP_PORT_OBJECT, WaitEvent ) );

    //
    //  Otherwise indicate that there isn't a default object to wait
    //  on
    //

    } else {

        NewObjectType->DefaultObject = NULL;
    }

    //
    //  Lock down the type object type and if there is a creator info
    //  record then insert this object on that list
    //

    ObpEnterObjectTypeMutex( ObpTypeObjectType );

    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( NewObjectTypeHeader );

    if (CreatorInfo != NULL) {

        InsertTailList( &ObpTypeObjectType->TypeList, &CreatorInfo->TypeList );
    }

    //
    //  Store a pointer to this new object type in the
    //  global object types array.  We'll use the index from
    //  the type object type number of objects count
    //

    NewObjectType->Index = ObpTypeObjectType->TotalNumberOfObjects;

    if (NewObjectType->Index < OBP_MAX_DEFINED_OBJECT_TYPES) {

        ObpObjectTypes[ NewObjectType->Index - 1 ] = NewObjectType;
    }

    //
    //  Unlock the type object type lock
    //

    ObpLeaveObjectTypeMutex( ObpTypeObjectType );

    //
    //  Lastly if there is not a directory object type yet then the following
    //  code will actually drop through and set the output object type
    //  and return success.
    //
    //  Otherwise, there is a directory object type and we try to insert the
    //  new type into the directory.  If this succeeds then we'll reference
    //  the directory type object, unlock the root directory, set the
    //  output type and return success
    //

    if (!ObpTypeDirectoryObject ||
        ObpInsertDirectoryEntry( ObpTypeDirectoryObject, NewObjectType )) {

        if (ObpTypeDirectoryObject) {

            ObReferenceObject( ObpTypeDirectoryObject );
        }

        if (ObpTypeDirectoryObject) {

            ObpLeaveRootDirectoryMutex();
        }

        *ObjectType = NewObjectType;

        return( STATUS_SUCCESS );

    } else {

        //
        //  Otherwise there is a directory object type and
        //  the insertion failed.  So release the root directory
        //  and return failure to our caller.
        //

        ObpLeaveRootDirectoryMutex();

        return( STATUS_INSUFFICIENT_RESOURCES );
    }
}


NTSTATUS
ObEnumerateObjectsByType(
    IN POBJECT_TYPE ObjectType,
    IN OB_ENUM_OBJECT_TYPE_ROUTINE EnumerationRoutine,
    IN PVOID Parameter
    )

/*++

Routine Description:

    This routine, via a callback, will enumerate through all
    the objects of a specified type.  This only works on objects
    that maintain the type list (i.e., have an object creator
    info record).

Arguments:

    ObjectType - Supplies the object type being enumerated

    EnumerationRoutine - Supplies the callback routine to use

    Parameter - Supplies a parameter to pass through to the callback
        routine

Return Value:

    STATUS_SUCCESS if the enumeration finishes because the
    end of the list is reached and STATUS_NO_MORE_ENTRIES if
    the enmeration callback routine ever returns false.

--*/

{
    NTSTATUS Status;
    UNICODE_STRING ObjectName;
    PLIST_ENTRY Next, Head;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER ObjectHeader;

    //
    //  Lock the object type, and get ourselves up
    //  to iterate through the type list (i.e., the list
    //  of all the objects of this specified type)
    //

    ObpEnterObjectTypeMutex( ObjectType );

    Head = &ObjectType->TypeList;
    Next = Head->Flink;
    Status = STATUS_SUCCESS;

    //
    //  Now enumerate through the type list.  The Creator
    //  info record is where we wind up in the object.  And
    //  this record is known to be the first record preceding
    //  the object header.  So to get to the object header
    //  we just jump to the end of the creator info record.
    //

    while (Next != Head) {

        CreatorInfo = CONTAINING_RECORD( Next,
                                         OBJECT_HEADER_CREATOR_INFO,
                                         TypeList );

        ObjectHeader = (POBJECT_HEADER)(CreatorInfo+1);

        //
        //  From the object header see if there is a name for the
        //  object. If there is not a name then we'll supply an
        //  empty name.
        //

        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

        if (NameInfo != NULL) {

            ObjectName = NameInfo->Name;

        } else {

            RtlZeroMemory( &ObjectName, sizeof( ObjectName ) );
        }

        //
        //  Now invoke the callback and if it returns false then
        //  we're done with the enumeration and will return
        //  an alternate ntstatus value
        //

        if (!(EnumerationRoutine)( &ObjectHeader->Body,
                                   &ObjectName,
                                   ObjectHeader->HandleCount,
                                   ObjectHeader->PointerCount,
                                   Parameter )) {

            Status = STATUS_NO_MORE_ENTRIES;

            break;
        }

        //
        //  Get the next record in the type list and continue
        //  the enumeration.
        //

        Next = Next->Flink;
    }

    //
    //  Free the object type lock and return to our caller
    //

    ObpLeaveObjectTypeMutex( ObjectType );

    return Status;
}


typedef struct _OBJECT_TYPE_ARRAY {

    ULONG   Size;
    POBJECT_HEADER_CREATOR_INFO CreatorInfoArray[1];

} OBJECT_TYPE_ARRAY, *POBJECT_TYPE_ARRAY;


POBJECT_TYPE_ARRAY
ObpCreateTypeArray (
    IN POBJECT_TYPE ObjectType
    )

/*++

Routine Description:

    This routine create an array with pointers to all objects queued
    for a given ObjectType. All objects are referenced when are stored
    in the array.

Arguments:

    ObjectType - Supplies the object type for which we make copy
    for all objects.


Return Value:

    The array with objects created. returns NULL if the specified ObjectType
    has the TypeList empty.

--*/

{
    ULONG Count;
    POBJECT_TYPE_ARRAY ObjectArray;
    PLIST_ENTRY Next1, Head1;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER ObjectHeader;
    PVOID Object;

    //
    //  Acquire the ObjectType mutex
    //

    ObpEnterObjectTypeMutex( ObjectType );

    ObjectArray = NULL;

    //
    //  Count the number of elements into the list
    //

    Count = 0;

    Head1 = &ObjectType->TypeList;
    Next1 = Head1->Flink;

    while (Next1 != Head1) {

        Next1 = Next1->Flink;
        Count += 1;
    }

    //
    //  If we have a number of objects > 0 then we'll create an array
    //  and copy all pointers into that array
    //

    if ( Count > 0 ) {

        //
        //  Allocate the memory for array
        //

        ObjectArray = ExAllocatePoolWithTag( PagedPool,
                                             sizeof(OBJECT_TYPE_ARRAY) + sizeof(POBJECT_HEADER_CREATOR_INFO) * (Count - 1),
                                             'rAbO' );
        if ( ObjectArray != NULL ) {

            ObjectArray->Size = Count;

            Count = 0;

            //
            //  Start parsing the TypeList
            //

            Head1 = &ObjectType->TypeList;
            Next1 = Head1->Flink;

            while (Next1 != Head1) {

                ASSERT( Count < ObjectArray->Size );

                //
                //  For each object we'll grab its creator info record,
                //  its object header, and its object body
                //

                CreatorInfo = CONTAINING_RECORD( Next1,
                                                 OBJECT_HEADER_CREATOR_INFO,
                                                 TypeList );

                //
                //  We'll store the CreatorInfo into the ObjectArray
                //

                ObjectArray->CreatorInfoArray[Count] = CreatorInfo;

                //
                //  Find the Object and increment the references to that object
                //  to avoid deleting while are stored copy in this array
                //

                ObjectHeader = (POBJECT_HEADER)(CreatorInfo+1);

                Object = &ObjectHeader->Body;

                ObReferenceObject( Object);

                Next1 = Next1->Flink;
                Count++;

            }
        }
    }

    //
    //  Release the ObjectType mutex
    //

    ObpLeaveObjectTypeMutex( ObjectType );

    return ObjectArray;
}


VOID
ObpDestroyTypeArray (
    IN POBJECT_TYPE_ARRAY ObjectArray
    )

/*++

Routine Description:

    This routine destroy an array with pointers to objects, created by
    ObpCreateTypeArray. Each object is dereferenced before releasing the
    array memory.

Arguments:

    ObjectArray - Supplies the array to be freed

Return Value:


--*/

{
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER ObjectHeader;
    PVOID Object;
    ULONG i;

    if (ObjectArray != NULL) {

        //
        //  Go through array and dereference all objects.
        //

        for (i = 0; i < ObjectArray->Size; i++) {

            //
            //  Retrieving the Object from the CreatorInfo
            //

            CreatorInfo = ObjectArray->CreatorInfoArray[i];

            ObjectHeader = (POBJECT_HEADER)(CreatorInfo+1);

            Object = &ObjectHeader->Body;

            //
            //  Dereference the object
            //

            ObDereferenceObject( Object );
        }

        //
        //  Free the memory alocated for this array
        //

        ExFreePoolWithTag( ObjectArray, 'rAbO' );
    }
}


NTSTATUS
ObGetObjectInformation(
    IN PCHAR UserModeBufferAddress,
    OUT PSYSTEM_OBJECTTYPE_INFORMATION ObjectInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This routine returns information for all the object in the
    system.  It enuermates through all the object types and in
    each type it enumerates through their type list.

Arguments:

    UserModeBufferAddress - Supplies the address of the query buffer
        as specified by the user.

    ObjectInformation - Supplies a buffer to receive the object
        type information.  This is essentially the same as the first
        parameter except that one is a system address and the other
        is in the user's address space.

    Length - Supplies the length, in bytes, of the object information
        buffer

    ReturnLength - Optionally receives the total length, in bytes,
        needed to store the object information


Return Value:

    An appropriate status value

--*/

{
    NTSTATUS ReturnStatus, Status;
    PLIST_ENTRY Next, Head;
    PLIST_ENTRY Next1, Head1;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    PVOID Object;
    BOOLEAN FirstObjectForType;
    PSYSTEM_OBJECTTYPE_INFORMATION TypeInfo;
    PSYSTEM_OBJECT_INFORMATION ObjectInfo;
    ULONG TotalSize, NameSize;
    POBJECT_HEADER ObjectTypeHeader;
    WCHAR NameBuffer[ 260 + 4 ];
    POBJECT_NAME_INFORMATION NameInformation;
    extern POBJECT_TYPE IoFileObjectType;
    PWSTR TempBuffer;
    USHORT TempMaximumLength;
    POBJECT_TYPE_ARRAY ObjectTypeArray;
    ULONG i;

    PAGED_CODE();

    //
    //  Initialize some local variables
    //

    NameInformation = (POBJECT_NAME_INFORMATION)NameBuffer;
    ReturnStatus = STATUS_SUCCESS;
    TotalSize = 0;
    TypeInfo = NULL;

    //
    //  Lock down the type object type to avoid the creation of a new object type while
    //  iterating through the list items
    //

    ObpEnterObjectTypeMutex( ObpTypeObjectType );

    try {

        //
        //  The following outer loop iterates through each object type
        //  in the system
        //

        Head = &ObpTypeObjectType->TypeList;
        Next = Head->Flink;

        while (Next != Head) {

            //
            //  For each object type object we'll grab its creator
            //  info record and which must directly precede the
            //  object header followed by the object body
            //

            CreatorInfo = CONTAINING_RECORD( Next,
                                             OBJECT_HEADER_CREATOR_INFO,
                                             TypeList );

            ObjectTypeHeader = (POBJECT_HEADER)(CreatorInfo+1);
            ObjectType = (POBJECT_TYPE)&ObjectTypeHeader->Body;

            //
            //  Now if this is not the object type object, which is what
            //  the outer loop is going through then we'll jump in one
            //  more loop
            //

            if (ObjectType != ObpTypeObjectType) {

                //
                //  Capture the array with objects queued in the TypeList
                //

                ObjectTypeArray = ObpCreateTypeArray ( ObjectType );

                //
                //  If it is any object in that queue, start
                //  quering information about it
                //

                if (ObjectTypeArray != NULL) {

                    //
                    //  The following loop iterates through each object
                    //  of the specified type.
                    //

                    FirstObjectForType = TRUE;

                    for ( i = 0; i < ObjectTypeArray->Size; i++) {

                        //
                        //  For each object we'll grab its creator info record,
                        //  its object header, and its object body
                        //

                        CreatorInfo = ObjectTypeArray->CreatorInfoArray[i];

                        ObjectHeader = (POBJECT_HEADER)(CreatorInfo+1);

                        Object = &ObjectHeader->Body;

                        //
                        //  If this is the first time through the inner loop for this
                        //  type then we'll fill in the type info buffer
                        //

                        if (FirstObjectForType) {

                            FirstObjectForType = FALSE;

                            //
                            //  If the pointer it not null (i.e., we've been through
                            //  this loop before) and the total size we've used so
                            //  far hasn't exhausted the output buffer then
                            //  set the previous type info record to point to the
                            //  next type info record
                            //

                            if ((TypeInfo != NULL) && (TotalSize < Length)) {

                                TypeInfo->NextEntryOffset = TotalSize;
                            }

                            //
                            //  Set the current type info record to point to the next
                            //  free spot in the output buffer, and adjust the total
                            //  size used so far to account for the object type info
                            //  buffer
                            //

                            TypeInfo = (PSYSTEM_OBJECTTYPE_INFORMATION)((PCHAR)ObjectInformation + TotalSize);

                            TotalSize += FIELD_OFFSET( SYSTEM_OBJECTTYPE_INFORMATION, TypeName );

                            //
                            //  See if the data will fit into the info buffer, and if
                            //  so then fill in the record
                            //

                            if (TotalSize >= Length) {

                                ReturnStatus = STATUS_INFO_LENGTH_MISMATCH;

                            } else {

                                TypeInfo->NextEntryOffset   = 0;
                                TypeInfo->NumberOfObjects   = ObjectType->TotalNumberOfObjects;
                                TypeInfo->NumberOfHandles   = ObjectType->TotalNumberOfHandles;
                                TypeInfo->TypeIndex         = ObjectType->Index;
                                TypeInfo->InvalidAttributes = ObjectType->TypeInfo.InvalidAttributes;
                                TypeInfo->GenericMapping    = ObjectType->TypeInfo.GenericMapping;
                                TypeInfo->ValidAccessMask   = ObjectType->TypeInfo.ValidAccessMask;
                                TypeInfo->PoolType          = ObjectType->TypeInfo.PoolType;
                                TypeInfo->SecurityRequired  = ObjectType->TypeInfo.SecurityRequired;
                            }

                            //
                            //  Now we need to do the object's type name. The name
                            //  goes right after the type info field.  The following
                            //  query type name call knows to take the address of a
                            //  unicode string and assumes that the buffer to stuff
                            //  the string is right after the unicode string structure.
                            //  The routine also assumes that name size is the number
                            //  of bytes already use in the buffer and add to it the
                            //  number of bytes it uses.  That is why we need to
                            //  initialize it to zero before doing the call.
                            //

                            NameSize = 0;

                            Status = ObQueryTypeName( Object,
                                                      &TypeInfo->TypeName,
                                                      TotalSize < Length ? Length - TotalSize : 0,
                                                      &NameSize );

                            //
                            //  Round the name size up to the next ulong boundary
                            //

                            NameSize = (NameSize + sizeof( ULONG ) - 1) & (~(sizeof( ULONG ) - 1));

                            //
                            //  If we were able to successfully get the type name then
                            //  set the max length to the rounded ulong that does not
                            //  include the heading unicode string structure.  Also set
                            //  the buffer to the address that the user would use to
                            //  access the string.
                            //

                            if (NT_SUCCESS( Status )) {

                                TypeInfo->TypeName.MaximumLength = (USHORT)
                                    (NameSize - sizeof( TypeInfo->TypeName ));
                                TypeInfo->TypeName.Buffer = (PWSTR)
                                    (UserModeBufferAddress +
                                     ((PCHAR)TypeInfo->TypeName.Buffer - (PCHAR)ObjectInformation)
                                    );

                            } else {

                                ReturnStatus = Status;
                            }

                            //
                            //  Now we need to bias the total size we've used by the
                            //  size of the object name
                            //

                            TotalSize += NameSize;

                        } else {

                            //
                            //  Otherwise this is not the first time through the inner
                            //  loop for this object type so the only thing we need to
                            //  do is set the previous object info record to "point via
                            //  relative offset" to the next object info record
                            //

                            if (TotalSize < Length) {

                                ObjectInfo->NextEntryOffset = TotalSize;
                            }
                        }

                        //
                        //  We still have an object info record to fill in for this
                        //  record.  The only thing we've done so far is the type info
                        //  record.  So now get a pointer to the new object info record
                        //  and adjust the total size to account for the object record
                        //

                        ObjectInfo = (PSYSTEM_OBJECT_INFORMATION)((PCHAR)ObjectInformation + TotalSize);

                        TotalSize += FIELD_OFFSET( SYSTEM_OBJECT_INFORMATION, NameInfo );

                        //
                        //  If there is room for the object info record then fill
                        //  in the record
                        //

                        if (TotalSize >= Length) {

                            ReturnStatus = STATUS_INFO_LENGTH_MISMATCH;

                        } else {

                            ObjectInfo->NextEntryOffset       = 0;
                            ObjectInfo->Object                = Object;
                            ObjectInfo->CreatorUniqueProcess  = CreatorInfo->CreatorUniqueProcess;
                            ObjectInfo->CreatorBackTraceIndex = CreatorInfo->CreatorBackTraceIndex;
                            ObjectInfo->PointerCount          = ObjectHeader->PointerCount;
                            ObjectInfo->HandleCount           = ObjectHeader->HandleCount;
                            ObjectInfo->Flags                 = (USHORT)ObjectHeader->Flags;
                            ObjectInfo->SecurityDescriptor    = ObjectHeader->SecurityDescriptor;

                            //
                            //  Fill in the appropriate quota information if there is
                            //  any quota information available
                            //

                            QuotaInfo = OBJECT_HEADER_TO_QUOTA_INFO( ObjectHeader );

                            if (QuotaInfo != NULL) {

                                ObjectInfo->PagedPoolCharge    = QuotaInfo->PagedPoolCharge;
                                ObjectInfo->NonPagedPoolCharge = QuotaInfo->NonPagedPoolCharge;

                                if (QuotaInfo->ExclusiveProcess != NULL) {

                                    ObjectInfo->ExclusiveProcessId = QuotaInfo->ExclusiveProcess->UniqueProcessId;
                                }

                            } else {

                                ObjectInfo->PagedPoolCharge    = ObjectType->TypeInfo.DefaultPagedPoolCharge;
                                ObjectInfo->NonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
                            }
                        }

                        //
                        //  Now we are ready to get the object name.  If there is not a
                        //  private routine to get the object name then we can call our
                        //  ob routine to query the object name.  Also if this is not
                        //  a file object we can do the query call.  The call will
                        //  fill in our local name buffer.
                        //

                        NameSize = 0;
                        Status = STATUS_SUCCESS;

                        if ((ObjectType->TypeInfo.QueryNameProcedure == NULL) ||
                            (ObjectType != IoFileObjectType)) {

                            Status = ObQueryNameString( Object,
                                                        NameInformation,
                                                        sizeof( NameBuffer ),
                                                        &NameSize );

                        //
                        //  If this is a file object then we can get the
                        //  name directly from the file object.  We start by
                        //  directly copying the file object unicode string structure
                        //  into our local memory and then adjust the lengths, copy
                        //  the buffer and modify the pointers as necessary.
                        //

                        } else if (ObjectType == IoFileObjectType) {

                            NameInformation->Name = ((PFILE_OBJECT)Object)->FileName;

                            if ((NameInformation->Name.Length != 0) &&
                                (NameInformation->Name.Buffer != NULL)) {

                                NameSize = NameInformation->Name.Length + sizeof( UNICODE_NULL );

                                //
                                //  We will trim down names that are longer than 260 unicode
                                //  characters in length
                                //

                                if (NameSize > (260 * sizeof( WCHAR ))) {

                                    NameSize = (260 * sizeof( WCHAR ));
                                    NameInformation->Name.Length = (USHORT)(NameSize - sizeof( UNICODE_NULL ));
                                }

                                //
                                //  Now copy over the name from the buffer used by the
                                //  file object into our local buffer, adjust the
                                //  fields in the unicode string structure and null
                                //  terminate the string.  In the copy we cannot copy
                                //  the null character from the filename because it
                                //  may not be valid memory
                                //

                                RtlMoveMemory( (NameInformation+1),
                                               NameInformation->Name.Buffer,
                                               NameSize - sizeof( UNICODE_NULL) );

                                NameInformation->Name.Buffer = (PWSTR)(NameInformation+1);
                                NameInformation->Name.MaximumLength = (USHORT)NameSize;
                                NameInformation->Name.Buffer[ NameInformation->Name.Length / sizeof( WCHAR )] = UNICODE_NULL;

                                //
                                //  Adjust the name size to account for the unicode
                                //  string structure
                                //

                                NameSize += sizeof( *NameInformation );

                            } else {

                                //
                                //  The file object does not have a name so the name
                                //  size stays zero
                                //
                                //  **** Name Size should already be zero from about 70
                                //       lines above
                                //

                                NameSize = 0;
                            }
                        }

                        //
                        //  At this point if we have a name then the name size will
                        //  not be zero and the name is stored in our local name
                        //  information variable
                        //

                        if (NameSize != 0) {

                            //
                            //  Adjust the size of the name up to the next ulong
                            //  boundary and modify the total size required when
                            //  we add in the object name
                            //

                            NameSize = (NameSize + sizeof( ULONG ) - 1) & (~(sizeof( ULONG ) - 1));
                            TotalSize += NameSize;

                            //
                            //  If everything has been successful so far, and we have
                            //  a non empty name, and everything fits in the output
                            //  buffer then copy over the name from our local buffer
                            //  into the caller supplied output buffer, append on the
                            //  null terminating character, and adjust the buffer point
                            //  to use the user's buffer
                            //

                            if ((NT_SUCCESS( Status )) &&
                                (NameInformation->Name.Length != 0) &&
                                (TotalSize < Length)) {

                                //
                                //  Use temporary local variable for RltMoveMemory
                                //

                                TempBuffer = (PWSTR)((&ObjectInfo->NameInfo)+1);
                                TempMaximumLength = (USHORT)
                                    (NameInformation->Name.Length + sizeof( UNICODE_NULL ));

                                ObjectInfo->NameInfo.Name.Length = NameInformation->Name.Length;

                                RtlMoveMemory( TempBuffer,
                                               NameInformation->Name.Buffer,
                                               TempMaximumLength);

                                ObjectInfo->NameInfo.Name.Buffer = (PWSTR)
                                    (UserModeBufferAddress +
                                     ((PCHAR)TempBuffer - (PCHAR)ObjectInformation));
                                ObjectInfo->NameInfo.Name.MaximumLength = TempMaximumLength;

                            //
                            //  Otherwise if we've been successful so far but for some
                            //  reason we weren't able to store the object name then
                            //  decide if it was because of an not enough space or
                            //  because the object name is null
                            //

                            } else if (NT_SUCCESS( Status )) {

                                if ((NameInformation->Name.Length != 0) ||
                                    (TotalSize >= Length)) {

                                    ReturnStatus = STATUS_INFO_LENGTH_MISMATCH;

                                } else {

                                    RtlInitUnicodeString( &ObjectInfo->NameInfo.Name, NULL );
                                }

                            //
                            //  Otherwise we have not been successful so far, we'll
                            //  adjust the total size to account for a null unicode
                            //  string, and if it doesn't fit then that's an error
                            //  otherwise we'll put in the null object name
                            //

                            } else {

                                TotalSize += sizeof( ObjectInfo->NameInfo.Name );

                                if (TotalSize >= Length) {

                                    ReturnStatus = STATUS_INFO_LENGTH_MISMATCH;

                                } else {

                                    RtlInitUnicodeString( &ObjectInfo->NameInfo.Name, NULL );

                                    ReturnStatus = Status;
                                }
                            }

                        //
                        //  Otherwise the name size is zero meaning we have not found
                        //  an object name, so we'll adjust total size for the null
                        //  unicode string, and check that it fits in the output
                        //  buffer.  If it fits we'll output a null object name
                        //

                        } else {

                            TotalSize += sizeof( ObjectInfo->NameInfo.Name );

                            if (TotalSize >= Length) {

                                ReturnStatus = STATUS_INFO_LENGTH_MISMATCH;

                            } else {

                                RtlInitUnicodeString( &ObjectInfo->NameInfo.Name, NULL );
                            }
                        }

                    }

                    //
                    //  Release the array with objects
                    //

                    ObpDestroyTypeArray(ObjectTypeArray);
                    ObjectTypeArray = NULL;
                }
            }

            //
            //  Get the next object type from the list of system defined types.
            //  This is the outer loop
            //

            Next = Next->Flink;
        }

        //
        //  Fill in the total size needed to store the buffer if the user wants
        //  that information.  And return to our caller
        //

        if (ARGUMENT_PRESENT( ReturnLength )) {

            *ReturnLength = TotalSize;
        }


    } finally {

        if (ObjectTypeArray != NULL) {

            ObpDestroyTypeArray(ObjectTypeArray);
        }
        //
        //  Unlock the type object type lock
        //

        ObpLeaveObjectTypeMutex( ObpTypeObjectType );
    }


    return( ReturnStatus );
}

