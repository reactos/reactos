/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obquery.c

Abstract:

    Query Object system service

Author:

    Steve Wood (stevewo) 12-May-1989

Revision History:

--*/

#include "obp.h"

//
//  Local procedure prototypes
//

//
//  The following structure is used to pass the call back routine
//  "ObpSetHandleAttributes" the captured object information and
//  the processor mode of the caller.
//

typedef struct __OBP_SET_HANDLE_ATTRIBUTES {

    OBJECT_HANDLE_FLAG_INFORMATION ObjectInformation;

    KPROCESSOR_MODE PreviousMode;

} OBP_SET_HANDLE_ATTRIBUTES, *POBP_SET_HANDLE_ATTRIBUTES;

BOOLEAN
ObpSetHandleAttributes (
    IN OUT PVOID TableEntry,
    IN ULONG_PTR Parameter
    );

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,NtQueryObject)
#pragma alloc_text(PAGE,ObQueryNameString)
#pragma alloc_text(PAGE,ObQueryTypeName)
#pragma alloc_text(PAGE,ObQueryTypeInfo)
#pragma alloc_text(PAGE,ObQueryObjectAuditingByHandle)
#pragma alloc_text(PAGE,NtSetInformationObject)
#endif


NTSTATUS
NtQueryObject (
    IN HANDLE Handle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG ObjectInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine description:

    This routine is used to query information about a given object

Arguments:

    Handle - Supplies a handle to the object being queried.  This value
        is ignored if the requested information class is for type
        information.

    ObjectInformationClass - Specifies the type of information to return

    ObjectInformation - Supplies an output buffer for the information being
        returned

    ObjectInformationLength - Specifies, in bytes, the length of the
        preceding object information buffer

    ReturnLength - Optionally receives the length, in bytes, used to store
        the object information

Return Value:

    An appropriate status value

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID Object;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER ObjectDirectoryHeader;
    POBJECT_DIRECTORY ObjectDirectory;
    ACCESS_MASK GrantedAccess;
    POBJECT_HANDLE_FLAG_INFORMATION HandleFlags;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    ULONG NameInfoSize;
    ULONG SecurityDescriptorSize;
    ULONG TempReturnLength;
    OBJECT_BASIC_INFORMATION ObjectBasicInfo;
    POBJECT_TYPES_INFORMATION TypesInformation;
    POBJECT_TYPE_INFORMATION TypeInfo;
    ULONG i;

    PAGED_CODE();

    //
    //  Initialize our local variables
    //

    TempReturnLength = 0;

    //
    //  Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            if (ObjectInformationClass != ObjectHandleFlagInformation) {

                ProbeForWrite( ObjectInformation,
                               ObjectInformationLength,
                               sizeof( ULONG ));

            } else {

                ProbeForWrite( ObjectInformation,
                               ObjectInformationLength,
                               1 );
            }

            //
            //  We'll use a local temp return length variable to pass
            //  through to the later ob query calls which will increment
            //  its value.  We can't pass the users return length directly
            //  because the user might also be altering its value behind
            //  our back.
            //

            if (ARGUMENT_PRESENT( ReturnLength )) {

                ProbeForWriteUlong( ReturnLength );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }
    }

    //
    //  If the query is not for types information then we
    //  will have to get the object in question. Otherwise
    //  for types information there really isn't an object
    //  to grab.
    //

    if (ObjectInformationClass != ObjectTypesInformation) {

        Status = ObReferenceObjectByHandle( Handle,
                                            0,
                                            NULL,
                                            PreviousMode,
                                            &Object,
                                            &HandleInformation );

        if (!NT_SUCCESS( Status )) {

            return( Status );
        }

        GrantedAccess = HandleInformation.GrantedAccess;

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
        ObjectType = ObjectHeader->Type;

    } else {

        GrantedAccess = 0;
        Object = NULL;
        ObjectHeader = NULL;
        ObjectType = NULL;
    }

    //
    //  Now process the particular information class being
    //  requested
    //

    switch( ObjectInformationClass ) {

    case ObjectBasicInformation:

        //
        //  Make sure the output buffer is long enough and then
        //  fill in the appropriate fields into our local copy
        //  of basic information.
        //

        if (ObjectInformationLength != sizeof( OBJECT_BASIC_INFORMATION )) {

            ObDereferenceObject( Object );

            return( STATUS_INFO_LENGTH_MISMATCH );
        }

        ObjectBasicInfo.Attributes = HandleInformation.HandleAttributes;

        if (ObjectHeader->Flags & OB_FLAG_PERMANENT_OBJECT) {

            ObjectBasicInfo.Attributes |= OBJ_PERMANENT;
        }

        if (ObjectHeader->Flags & OB_FLAG_EXCLUSIVE_OBJECT) {

            ObjectBasicInfo.Attributes |= OBJ_EXCLUSIVE;
        }

        ObjectBasicInfo.GrantedAccess = GrantedAccess;
        ObjectBasicInfo.HandleCount = ObjectHeader->HandleCount;
        ObjectBasicInfo.PointerCount = ObjectHeader->PointerCount;

        QuotaInfo = OBJECT_HEADER_TO_QUOTA_INFO( ObjectHeader );

        if (QuotaInfo != NULL) {

            ObjectBasicInfo.PagedPoolCharge = QuotaInfo->PagedPoolCharge;
            ObjectBasicInfo.NonPagedPoolCharge = QuotaInfo->NonPagedPoolCharge;

        } else {

            ObjectBasicInfo.PagedPoolCharge = 0;
            ObjectBasicInfo.NonPagedPoolCharge = 0;
        }

        if (ObjectType == ObpSymbolicLinkObjectType) {

            ObjectBasicInfo.CreationTime = ((POBJECT_SYMBOLIC_LINK)Object)->CreationTime;

        } else {

            RtlZeroMemory( &ObjectBasicInfo.CreationTime,
                           sizeof( ObjectBasicInfo.CreationTime ));
        }

        //
        //  Compute the size of the object name string by taking its name plus
        //  seperators and traversing up to the root adding each directories
        //  name length plus seperators
        //

        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

        if ((NameInfo != NULL) && (NameInfo->Directory != NULL)) {
        
            ObpEnterRootDirectoryMutex();

            ObjectDirectory = NameInfo->Directory;

            //
            //  **** this "IF" is probably not needed because of the preceding
            //  "IF"
            //

            if (ObjectDirectory) {

                NameInfoSize = sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;

                while (ObjectDirectory) {

                    ObjectDirectoryHeader = OBJECT_TO_OBJECT_HEADER( ObjectDirectory );
                    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectDirectoryHeader );

                    if ((NameInfo != NULL) && (NameInfo->Directory != NULL)) {

                        NameInfoSize += sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;
                        ObjectDirectory = NameInfo->Directory;

                    } else {

                        break;
                    }
                }

                NameInfoSize += sizeof( OBJECT_NAME_INFORMATION ) + sizeof( UNICODE_NULL );
            }
            
            ObpLeaveRootDirectoryMutex();

        } else {

            NameInfoSize = 0;
        }

        ObjectBasicInfo.NameInfoSize = NameInfoSize;
        ObjectBasicInfo.TypeInfoSize = ObjectType->Name.Length + sizeof( UNICODE_NULL ) +
                                        sizeof( OBJECT_TYPE_INFORMATION );

        ObpAcquireDescriptorCacheReadLock();
        
        if ((GrantedAccess & READ_CONTROL) &&
            ARGUMENT_PRESENT( ObjectHeader->SecurityDescriptor )) {

            SecurityDescriptorSize = RtlLengthSecurityDescriptor(
                                         ObjectHeader->SecurityDescriptor);

        } else {

            SecurityDescriptorSize = 0;
        }

        ObpReleaseDescriptorCacheLock();

        ObjectBasicInfo.SecurityDescriptorSize = SecurityDescriptorSize;

        //
        //  Now that we've packaged up our local copy of basic info we need
        //  to copy it into the output buffer and set the return
        //  length
        //

        try {

            *(POBJECT_BASIC_INFORMATION) ObjectInformation = ObjectBasicInfo;

            TempReturnLength = ObjectInformationLength;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            // Fall through, since we cannot undo what we have done.
            //
        }

        break;

    case ObjectNameInformation:

        //
        //  Call a local worker routine
        //

        Status = ObQueryNameString( Object,
                                    (POBJECT_NAME_INFORMATION)ObjectInformation,
                                    ObjectInformationLength,
                                    &TempReturnLength );
        break;

    case ObjectTypeInformation:

        //
        //  Call a local worker routine
        //

        Status = ObQueryTypeInfo( ObjectType,
                                  (POBJECT_TYPE_INFORMATION)ObjectInformation,
                                  ObjectInformationLength,
                                  &TempReturnLength );
        break;

    case ObjectTypesInformation:

        try {

            //
            //  The first thing we do is set the return length to cover the
            //  types info record.  Later in each call to query type info
            //  this value will be updated as necessary
            //

            TempReturnLength = sizeof( OBJECT_TYPES_INFORMATION );

            //
            //  Make sure there is enough room to hold the types info record
            //  and if so then compute the number of defined types there are
            //

            TypesInformation = (POBJECT_TYPES_INFORMATION)ObjectInformation;

            if (ObjectInformationLength < sizeof( OBJECT_TYPES_INFORMATION ) ) {

                Status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                TypesInformation->NumberOfTypes = 0;

                for (i=0; i<OBP_MAX_DEFINED_OBJECT_TYPES; i++) {

                    ObjectType = ObpObjectTypes[ i ];

                    if (ObjectType == NULL) {

                        break;
                    }

                    TypesInformation->NumberOfTypes += 1;
                }
            }

            //
            //  For each defined type we will query the type info for the
            //  object type and adjust the TypeInfo pointer to the next
            //  free spot
            //

            TypeInfo = (POBJECT_TYPE_INFORMATION)(TypesInformation + 1);

            for (i=0; i<OBP_MAX_DEFINED_OBJECT_TYPES; i++) {

                ObjectType = ObpObjectTypes[ i ];

                if (ObjectType == NULL) {

                    break;
                }

                Status = ObQueryTypeInfo( ObjectType,
                                          TypeInfo,
                                          ObjectInformationLength,
                                          &TempReturnLength );

                if (NT_SUCCESS( Status )) {

                    TypeInfo = (POBJECT_TYPE_INFORMATION)
                        ((PCHAR)(TypeInfo+1) + ALIGN_UP( TypeInfo->TypeName.MaximumLength, ULONG ));
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

        break;

    case ObjectHandleFlagInformation:

        try {

            //
            //  Set the amount of data we are going to return
            //

            TempReturnLength = sizeof(OBJECT_HANDLE_FLAG_INFORMATION);

            HandleFlags = (POBJECT_HANDLE_FLAG_INFORMATION)ObjectInformation;

            //
            //  Make sure we have enough room for the query, and if so we'll
            //  set the output based on the flags stored in the handle
            //

            if (ObjectInformationLength < sizeof( OBJECT_HANDLE_FLAG_INFORMATION)) {

                Status = STATUS_INFO_LENGTH_MISMATCH;

            } else {

                HandleFlags->Inherit = FALSE;

                if (HandleInformation.HandleAttributes & OBJ_INHERIT) {

                    HandleFlags->Inherit = TRUE;
                }

                HandleFlags->ProtectFromClose = FALSE;

                if (HandleInformation.HandleAttributes & OBJ_PROTECT_CLOSE) {

                    HandleFlags->ProtectFromClose = TRUE;
                }
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

        break;

    default:

        //
        //  To get to this point we must have had an object and the
        //  information class is not defined, so we should dereference the
        //  object and return to our user the bad status
        //

        ObDereferenceObject( Object );

        return( STATUS_INVALID_INFO_CLASS );
    }

    //
    //  Now if the caller asked for a return length we'll set it from
    //  our local copy
    //

    try {

        if (ARGUMENT_PRESENT( ReturnLength ) ) {

            *ReturnLength = TempReturnLength;
        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        //
        //  Fall through, since we cannot undo what we have done.
        //
    }

    //
    //  In the end we can free the object if there was one and return
    //  to our caller
    //

    if (Object != NULL) {

        ObDereferenceObject( Object );
    }

    return( Status );
}


NTSTATUS
NTAPI
NtSetInformationObject (
    IN HANDLE Handle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    IN PVOID ObjectInformation,
    IN ULONG ObjectInformationLength
    )

/*++

Routine description:

    This routine is used to set handle information about a specified
    handle

Arguments:

    Handle - Supplies the handle being modified

    ObjectInformationClass - Specifies the class of information being
        modified.  The only accepted value is ObjectHandleFlagInformation

    ObjectInformation - Supplies the buffer containing the handle
        flag information structure

    ObjectInformationLength - Specifies the length, in bytes, of the
        object information buffer

Return Value:

    An appropriate status value

--*/

{
    OBP_SET_HANDLE_ATTRIBUTES CapturedInformation;
    HANDLE ObjectHandle;
    PVOID ObjectTable;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    BOOLEAN AttachedToProcess = FALSE;
    KAPC_STATE ApcState;

    PAGED_CODE();

    //
    //  Check if the information class and information length are correct.
    //

    if (ObjectInformationClass != ObjectHandleFlagInformation) {

        return STATUS_INVALID_INFO_CLASS;
    }

    if (ObjectInformationLength != sizeof(OBJECT_HANDLE_FLAG_INFORMATION)) {

        return STATUS_INFO_LENGTH_MISMATCH;
    }

    //
    //  Get previous processor mode and probe and capture the input
    //  buffer
    //

    CapturedInformation.PreviousMode = KeGetPreviousMode();

    try {

        if (CapturedInformation.PreviousMode != KernelMode) {

            ProbeForRead(ObjectInformation, ObjectInformationLength, 1);
        }

        CapturedInformation.ObjectInformation = *(POBJECT_HANDLE_FLAG_INFORMATION)ObjectInformation;

    } except(ExSystemExceptionFilter()) {

        return GetExceptionCode();
    }

#if 0

    //
    //  On checked builds, check that if the Kernel handle bit is set,
    //  then we're coming from Kernel mode. We should probably fail the
    //  call if bit set && !Kmode
    //

    if ((Handle != NtCurrentThread()) && (Handle != NtCurrentProcess())) {

        ASSERT((Handle < 0 ) ? (PreviousMode == KernelMode) : TRUE);
    }

#endif

    //
    //  Get the address of the object table for the current process.  Or
    //  get the system handle table if this is a kernel handle and we are
    //  in kernel mode
    //

    if (IsKernelHandle( Handle, CapturedInformation.PreviousMode )) {

        //
        //  Make the handle look like a regular handle
        //

        ObjectHandle = DecodeKernelHandle( Handle );

        //
        //  The global kernel handle table
        //

        ObjectTable = ObpKernelHandleTable;

        //
        //  Go to the system process
        //

        if (PsGetCurrentProcess() != PsInitialSystemProcess) {
            KeStackAttachProcess (&PsInitialSystemProcess->Pcb, &ApcState);
            AttachedToProcess = TRUE;
        }

    } else {

        ObjectTable = ObpGetObjectTable();
        ObjectHandle = Handle;
    }

    //
    //  Make the change to the handle table entry.  The callback
    //  routine will do the actual change
    //

    if (ExChangeHandle( ObjectTable,
                        ObjectHandle,
                        ObpSetHandleAttributes,
                        (ULONG_PTR)&CapturedInformation) ) {

        Status = STATUS_SUCCESS;

    } else {

        Status = STATUS_ACCESS_DENIED;
    }

    //
    //  If we are attached to the system process then return
    //  back to our caller
    //

    if (AttachedToProcess) {
        KeUnstackDetachProcess(&ApcState);
        AttachedToProcess = FALSE;
    }

    //
    //  And return to our caller
    //

    return Status;
}


#define OBP_MISSING_NAME_LITERAL L"..."
#define OBP_MISSING_NAME_LITERAL_SIZE (sizeof( OBP_MISSING_NAME_LITERAL ) - sizeof( UNICODE_NULL ))

NTSTATUS
ObQueryNameString (
    IN PVOID Object,
    OUT POBJECT_NAME_INFORMATION ObjectNameInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    )

/*++

Routine description:

    This routine processes a query of an object's name information

Arguments:

    Object - Supplies the object being queried

    ObjectNameInfo - Supplies the buffer to store the name string
        information

    Length - Specifies the length, in bytes, of the original object
        name info buffer.

    ReturnLength - Contains the number of bytes already used up
        in the object name info. On return this receives an updated
        byte count.

        (Length minus ReturnLength) is really now many bytes are left
        in the output buffer.  The buffer supplied to this call may
        actually be offset within the original users buffer

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;
    POBJECT_HEADER ObjectDirectoryHeader;
    POBJECT_DIRECTORY ObjectDirectory;
    ULONG NameInfoSize;
    PUNICODE_STRING String;
    PWCH StringBuffer;
    ULONG NameSize;

    PAGED_CODE();

    //
    //  Get the object header and name info record if it exists
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    //
    //  If the object type has a query name callback routine then
    //  that is how we get the name
    //

    if (ObjectHeader->Type->TypeInfo.QueryNameProcedure != NULL) {

        try {

            KIRQL SaveIrql;

            ObpBeginTypeSpecificCallOut( SaveIrql );
            ObpEndTypeSpecificCallOut( SaveIrql, "Query", ObjectHeader->Type, Object );

            Status = (*ObjectHeader->Type->TypeInfo.QueryNameProcedure)( Object,
                                                                         (BOOLEAN)((NameInfo != NULL) && (NameInfo->Name.Length != 0)),
                                                                         ObjectNameInfo,
                                                                         Length,
                                                                         ReturnLength );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
        }

        return( Status );
    }

    //
    //  Otherwise, the object type does not specify a query name
    //  procedure so we get to do the work.  The first thing
    //  to check is if the object doesn't even have a name.  If
    //  object doesn't have a name then we'll return an empty name
    //  info structure.
    //

    if ((NameInfo == NULL) || (NameInfo->Name.Buffer == NULL)) {

        //
        //  Compute the length of our return buffer, set the output
        //  if necessary and make sure the supplied buffer is large
        //  enough
        //

        NameInfoSize = sizeof( OBJECT_NAME_INFORMATION );

        try {

            *ReturnLength = NameInfoSize;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }

        if (Length < NameInfoSize) {

            return( STATUS_INFO_LENGTH_MISMATCH );
        }

        //
        //  Initialize the output buffer to be an empty string
        //  and then return to our caller
        //

        try {

            ObjectNameInfo->Name.Length = 0;
            ObjectNameInfo->Name.MaximumLength = 0;
            ObjectNameInfo->Name.Buffer = NULL;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            //  Fall through, since we cannot undo what we have done.
            //
            //  **** This should probably get the exception code and return
            //  that value
            //
        }

        return( STATUS_SUCCESS );
    }

    //
    //  First lock the directory mutex to stop before we chase stuff down
    //

    ObpEnterRootDirectoryMutex();

    try {

        //
        //  The object does have a name but now see if this is
        //  just the root directory object in which case the name size
        //  is only the "\" character
        //

        if (Object == ObpRootDirectoryObject) {

            NameSize = sizeof( OBJ_NAME_PATH_SEPARATOR );

        } else {

            //
            //  The named object is not the root so for every directory
            //  working out way up we'll add its size to the name keeping
            //  track of "\" characters inbetween each component.  We first
            //  start with the object name itself and then move on to
            //  the directories
            //

            ObjectDirectory = NameInfo->Directory;
            NameSize = sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;

            //
            //  While we are not at the root we'll keep moving up
            //

            while ((ObjectDirectory != ObpRootDirectoryObject) && (ObjectDirectory)) {

                //
                //  Get the name information for this directory
                //

                ObjectDirectoryHeader = OBJECT_TO_OBJECT_HEADER( ObjectDirectory );
                NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectDirectoryHeader );

                if ((NameInfo != NULL) && (NameInfo->Directory != NULL)) {

                    //
                    //  This directory has a name so add it to the accomulated
                    //  size and move up the tree
                    //

                    NameSize += sizeof( OBJ_NAME_PATH_SEPARATOR ) + NameInfo->Name.Length;
                    ObjectDirectory = NameInfo->Directory;

                } else {

                    //
                    //  This directory does not have a name so we'll give it
                    //  the "..." name and stop the loop
                    //

                    NameSize += sizeof( OBJ_NAME_PATH_SEPARATOR ) + OBP_MISSING_NAME_LITERAL_SIZE;
                    break;
                }
            }
        }

        //
        //  At this point NameSize is the number of bytes we need to store the
        //  name of the object from the root down.  The total buffer size we are
        //  going to need will include this size, plus object name information
        //  structure, plus an ending null character
        //

        NameInfoSize = NameSize + sizeof( OBJECT_NAME_INFORMATION ) + sizeof( UNICODE_NULL );

        //
        //  Set the output size and make sure the supplied buffer is large enough
        //  to hold the information
        //

        try {

            *ReturnLength = NameInfoSize;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();
            leave;
        }

        if (Length < NameInfoSize) {

            Status = STATUS_INFO_LENGTH_MISMATCH;
            leave;
        }

        //
        //  **** the following IF isn't necessary because name info size is
        //  already guaranteed to be nonzero from about 23 lines above
        //

        if (NameInfoSize != 0) {

            //
            //  Set the String buffer to point to the byte right after the
            //  last byte in the output string.  This following logic actually
            //  fills in the buffer backwards working from the name back to the
            //  root
            //

            StringBuffer = (PWCH)ObjectNameInfo;
            StringBuffer = (PWCH)((PCH)StringBuffer + NameInfoSize);

            NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

            try {

                //
                //  Terminate the string with a null and backup one unicode
                //  character
                //

                *--StringBuffer = UNICODE_NULL;

                //
                //  If the object in question is not the root directory
                //  then we are going to put its name in the string buffer
                //  When we finally reach the root directory we'll append on
                //  the final "\"
                //

                if (Object != ObpRootDirectoryObject) {

                    //
                    //  Add in the objects name
                    //

                    String = &NameInfo->Name;
                    StringBuffer = (PWCH)((PCH)StringBuffer - String->Length);
                    RtlMoveMemory( StringBuffer, String->Buffer, String->Length );

                    //
                    //  While we are not at the root directory we'll keep
                    //  moving up
                    //

                    ObjectDirectory = NameInfo->Directory;

                    while ((ObjectDirectory != ObpRootDirectoryObject) && (ObjectDirectory)) {

                        //
                        //  Get the name information for this directory
                        //

                        ObjectDirectoryHeader = OBJECT_TO_OBJECT_HEADER( ObjectDirectory );
                        NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectDirectoryHeader );

                        //
                        //  Tack on the "\" between the last name we added and
                        //  this new name
                        //

                        *--StringBuffer = OBJ_NAME_PATH_SEPARATOR;

                        //
                        //  Preappend the directory name, if it has one, and
                        //  move up to the next directory.
                        //

                        if ((NameInfo != NULL) && (NameInfo->Directory != NULL)) {

                            String = &NameInfo->Name;
                            StringBuffer = (PWCH)((PCH)StringBuffer - String->Length);
                            RtlMoveMemory( StringBuffer, String->Buffer, String->Length );
                            ObjectDirectory = NameInfo->Directory;

                        } else {

                            //
                            //  The directory is nameless so use the "..." for
                            //  its name and break out of the loop
                            //

                            StringBuffer = (PWCH)((PCH)StringBuffer - OBP_MISSING_NAME_LITERAL_SIZE);
                            RtlMoveMemory( StringBuffer,
                                           OBP_MISSING_NAME_LITERAL,
                                           OBP_MISSING_NAME_LITERAL_SIZE );
                            break;
                        }
                    }
                }

                //
                //  Tack on the "\" for the root directory and then set the
                //  output unicode string variable to have the right size
                //  and point to the right spot.
                //

                *--StringBuffer = OBJ_NAME_PATH_SEPARATOR;

                ObjectNameInfo->Name.Length = (USHORT)NameSize;
                ObjectNameInfo->Name.MaximumLength = (USHORT)(NameSize+sizeof( UNICODE_NULL ));
                ObjectNameInfo->Name.Buffer = StringBuffer;

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                // Fall through, since we cannot undo what we have done.
                //
                //  **** This should probably get the exception code and return
                //  that value
                //
            }
        }

        Status = STATUS_SUCCESS;

    } finally {

        ObpLeaveRootDirectoryMutex();
    }

    return Status;
}


NTSTATUS
ObQueryTypeName (
    IN PVOID Object,
    PUNICODE_STRING ObjectTypeName,
    IN ULONG Length,
    OUT PULONG ReturnLength
    )

/*++

Routine description:

    This routine processes a query of an object's type name

Arguments:

    Object - Supplies the object being queried

    ObjectTypeName - Supplies the buffer to store the type name
        string information

    Length - Specifies the length, in bytes, of the object type
        name buffer

    ReturnLength - Contains the number of bytes already used up
        in the object type name buffer. On return this receives
        an updated byte count

        (Length minus ReturnLength) is really now many bytes are left
        in the output buffer.  The buffer supplied to this call may
        actually be offset within the original users buffer

Return Value:

    An appropriate status value

--*/

{
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER ObjectHeader;
    ULONG TypeNameSize;
    PUNICODE_STRING String;
    PWCH StringBuffer;
    ULONG NameSize;

    PAGED_CODE();

    //
    //  From the object get its object type and from that get the size of
    //  the object type name.  The total size for we need for the output
    //  buffer must fit the name, a terminating null, and a preceding
    //  unicode string structure
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectType = ObjectHeader->Type;

    NameSize = ObjectType->Name.Length;
    TypeNameSize = NameSize + sizeof( UNICODE_NULL ) + sizeof( UNICODE_STRING );

    //
    //  Update the number of bytes we need and make sure the output buffer is
    //  large enough
    //
    //  **** to be consistent with all our callers and other routines in
    //  this module this should probably be a "+=" and not just "="
    //

    try {

        *ReturnLength = TypeNameSize;

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        return( GetExceptionCode() );
    }

    if (Length < TypeNameSize) {

        return( STATUS_INFO_LENGTH_MISMATCH );
    }

    //
    //  **** this IF is probably not needed because of the earlier computation
    //  of type name size
    //

    if (TypeNameSize != 0) {

        //
        //  Set string buffer to point to the one byte beyond the
        //  buffer that we're going to fill in
        //

        StringBuffer = (PWCH)ObjectTypeName;
        StringBuffer = (PWCH)((PCH)StringBuffer + TypeNameSize);

        String = &ObjectType->Name;

        try {

            //
            //  Tack on the terminating null character and copy over
            //  the type name
            //

            *--StringBuffer = UNICODE_NULL;

            StringBuffer = (PWCH)((PCH)StringBuffer - String->Length);

            RtlMoveMemory( StringBuffer, String->Buffer, String->Length );

            //
            //  Now set the preceding unicode string to have the right
            //  lengths and to point to this buffer
            //

            ObjectTypeName->Length = (USHORT)NameSize;
            ObjectTypeName->MaximumLength = (USHORT)(NameSize+sizeof( UNICODE_NULL ));
            ObjectTypeName->Buffer = StringBuffer;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            //
            // Fall through, since we cannot undo what we have done.
            //
        }
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
ObQueryTypeInfo (
    IN POBJECT_TYPE ObjectType,
    OUT POBJECT_TYPE_INFORMATION ObjectTypeInfo,
    IN ULONG Length,
    OUT PULONG ReturnLength
    )

/*++

Routine description:

    This routine processes the query for object type information

Arguments:

    Object - Supplies a pointer to the object type being queried

    ObjectTypeInfo - Supplies the buffer to store the type information

    Length - Specifies the length, in bytes, of the object type
        information buffer

    ReturnLength - Contains the number of bytes already used up
        in the object type information buffer. On return this receives
        an updated byte count

        (Length minus ReturnLength) is really now many bytes are left
        in the output buffer.  The buffer supplied to this call may
        actually be offset within the original users buffer

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;

    try {

        //
        //  The total number of bytes needed for this query includes the
        //  object type information structure plus the name of the type
        //  rounded up to a ulong boundary
        //

        *ReturnLength += sizeof( *ObjectTypeInfo ) + ALIGN_UP( ObjectType->Name.MaximumLength, ULONG );

        //
        //  Make sure the buffer is large enough for this information and
        //  then fill in the record
        //

        if (Length < *ReturnLength) {

            Status = STATUS_INFO_LENGTH_MISMATCH;

        } else {

            ObjectTypeInfo->TotalNumberOfObjects = ObjectType->TotalNumberOfObjects;
            ObjectTypeInfo->TotalNumberOfHandles = ObjectType->TotalNumberOfHandles;
            ObjectTypeInfo->HighWaterNumberOfObjects = ObjectType->HighWaterNumberOfObjects;
            ObjectTypeInfo->HighWaterNumberOfHandles = ObjectType->HighWaterNumberOfHandles;
            ObjectTypeInfo->InvalidAttributes = ObjectType->TypeInfo.InvalidAttributes;
            ObjectTypeInfo->GenericMapping = ObjectType->TypeInfo.GenericMapping;
            ObjectTypeInfo->ValidAccessMask = ObjectType->TypeInfo.ValidAccessMask;
            ObjectTypeInfo->SecurityRequired = ObjectType->TypeInfo.SecurityRequired;
            ObjectTypeInfo->MaintainHandleCount = ObjectType->TypeInfo.MaintainHandleCount;
            ObjectTypeInfo->PoolType = ObjectType->TypeInfo.PoolType;
            ObjectTypeInfo->DefaultPagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;
            ObjectTypeInfo->DefaultNonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;

            //
            //  The type name goes right after this structure.  We cannot use
            //  rtl routine like RtlCopyUnicodeString that might use the local
            //  memory to keep state, because this is the user buffer and it
            //  could be changing by user
            //

            ObjectTypeInfo->TypeName.Buffer = (PWSTR)(ObjectTypeInfo+1);
            ObjectTypeInfo->TypeName.Length = ObjectType->Name.Length;
            ObjectTypeInfo->TypeName.MaximumLength = ObjectType->Name.MaximumLength;

            RtlMoveMemory( (PWSTR)(ObjectTypeInfo+1),
                           ObjectType->Name.Buffer,
                           ObjectType->Name.Length );

            ((PWSTR)(ObjectTypeInfo+1))[ ObjectType->Name.Length/sizeof(WCHAR) ] = UNICODE_NULL;

            Status = STATUS_SUCCESS;
        }

    } except( EXCEPTION_EXECUTE_HANDLER ) {

        Status = GetExceptionCode();
    }

    return Status;
}


NTSTATUS
ObQueryObjectAuditingByHandle (
    IN HANDLE Handle,
    OUT PBOOLEAN GenerateOnClose
    )

/*++

Routine description:

    This routine tells the caller if the indicated handle will
    generate an audit if it is closed

Arguments:

    Handle - Supplies the handle being queried

    GenerateOnClose - Receives TRUE if the handle will generate
        an audit if closed and FALSE otherwise

Return Value:

    An appropriate status value

--*/

{
    PHANDLE_TABLE ObjectTable;
    PHANDLE_TABLE_ENTRY ObjectTableEntry;
    PVOID Object;
    ULONG CapturedGrantedAccess;
    ULONG CapturedAttributes;
    NTSTATUS Status;

    PAGED_CODE();

    //
    //  Protect ourselves from being interrupted while we hold a handle table
    //  entry lock
    //

    KeEnterCriticalRegion();

    try {

        ObpValidateIrql( "ObQueryObjectAuditingByHandle" );


        //
        //  For the current process we'll grab its object table and
        //  then get the object table entry
        //

        ObjectTable = ObpGetObjectTable();

        ObjectTableEntry = ExMapHandleToPointer( ObjectTable,
                                                  Handle );

        //
        //  If we were given a valid handle we'll look at the attributes
        //  stored in the object table entry to decide if we generate
        //  an audit on close
        //

        if (ObjectTableEntry != NULL) {

            CapturedAttributes = ObjectTableEntry->ObAttributes;

            if (CapturedAttributes & OBJ_AUDIT_OBJECT_CLOSE) {

                *GenerateOnClose = TRUE;

            } else {

                *GenerateOnClose = FALSE;
            }

            ExUnlockHandleTableEntry( ObjectTable, ObjectTableEntry );

            Status = STATUS_SUCCESS;

        } else {

            Status = STATUS_INVALID_HANDLE;
        }

    } finally {

        KeLeaveCriticalRegion();
    }

    return Status;
}


#if DBG
PUNICODE_STRING
ObGetObjectName (
    IN PVOID Object
    )

/*++

Routine description:

    This routine returns a pointer to the name of object

Arguments:

    Object - Supplies the object being queried

Return Value:

    The address of the unicode string that stores the object
    name if available and NULL otherwise

--*/

{
    POBJECT_HEADER ObjectHeader;
    POBJECT_HEADER_NAME_INFO NameInfo;

    //
    //  Translate the input object to a name info structure
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    //
    //  If the object has a name then return the address of
    //  the name otherwise return null
    //

    if ((NameInfo != NULL) && (NameInfo->Name.Length != 0)) {

        return &NameInfo->Name;

    } else {

        return NULL;
    }
}
#endif // DBG


//
//  Local support routine
//

BOOLEAN
ObpSetHandleAttributes (
    IN OUT PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN ULONG_PTR Parameter
    )

/*++

Routine description:

    This is the call back routine for the ExChangeHandle from
    NtSetInformationObject

Arguments:

    ObjectTableEntry - Supplies a pointer to the object table entry being
        modified

    Parameter - Supplies a pointer to the OBJECT_HANDLE_FLAG_INFORMATION
        structure to set into the table entry

Return Value:

    Returns TRUE if the operation is successful otherwise FALSE

--*/

{
    POBP_SET_HANDLE_ATTRIBUTES ObjectInformation;
    POBJECT_HEADER ObjectHeader;

    ObjectInformation = (POBP_SET_HANDLE_ATTRIBUTES)Parameter;

    //
    //  Get a pointer to the object type via the object header and if the
    //  caller has asked for inherit but the object type says that inherit
    //  is an invalid flag then return false
    //

    ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

    if ((ObjectInformation->ObjectInformation.Inherit) &&
        ((ObjectHeader->Type->TypeInfo.InvalidAttributes & OBJ_INHERIT) != 0)) {

        return FALSE;
    }

    //
    //  User mode cannot set the object attributes for kernel objects
    //

    if ((ObjectHeader->Flags & OB_FLAG_KERNEL_OBJECT) &&
        (ObjectInformation->PreviousMode != KernelMode)) {

        return FALSE;
    }

    //
    //  For each piece of information (inheriit and protect from close) that
    //  is in the object information buffer we'll set or clear the bits in
    //  the object table entry.  The bits modified are the low order bits of
    //  used to store the pointer to the object header.
    //

    if (ObjectInformation->ObjectInformation.Inherit) {

        ObjectTableEntry->ObAttributes |= OBJ_INHERIT;

    } else {

        ObjectTableEntry->ObAttributes &= ~OBJ_INHERIT;
    }

    if (ObjectInformation->ObjectInformation.ProtectFromClose) {

        ObjectTableEntry->ObAttributes |= OBJ_PROTECT_CLOSE;

    } else {

        ObjectTableEntry->ObAttributes &= ~OBJ_PROTECT_CLOSE;
    }

    //
    //  And return to our caller
    //

    return TRUE;
}


