/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obref.c

Abstract:

    Object open API

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

//
//  A local variable to tell us if the worker queue is active
//

BOOLEAN ObpRemoveQueueActive;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,ObGetObjectPointerCount)
#pragma alloc_text(PAGE,ObOpenObjectByName)
#pragma alloc_text(PAGE,ObOpenObjectByPointer)
#pragma alloc_text(PAGE,ObReferenceObjectByName)
#pragma alloc_text(PAGE,ObpRemoveObjectRoutine)
#pragma alloc_text(PAGE,ObpDeleteNameCheck)
#endif


#ifndef LpcpAcquireLpcpLock

//
//  ********************  WARNING  ************************
//
//  The following include is neccessary in order to hold the LPC lock
//  while setting the  port flag to PORT_DELETED.
//  A cleanest solution could be a callback on DereferenceObject.
//  In that case we'll define the callback for the LPC to do all these things.
//

#include "..\lpc\lpcp.h"

#endif //#ifndef LpcpAcquireLpcpLock




ULONG
ObGetObjectPointerCount (
    IN PVOID Object
    )

/*++

Routine Description:

    This routine returns the current pointer count for a specified object.

Arguments:

    Object - Pointer to the object whose pointer count is to be returned.

Return Value:

    The current pointer count for the specified object is returned.

Note:

    This function cannot be made a macro, since fields in the thread object
    move from release to release, so this must remain a full function.

--*/

{
    PAGED_CODE();

    //
    //  Simply return the current pointer count for the object.
    //

    return OBJECT_TO_OBJECT_HEADER( Object )->PointerCount;
}


NTSTATUS
ObOpenObjectByName (
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PACCESS_STATE AccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:


    This function opens an object with full access validation and auditing.
    Soon after entering we capture the SubjectContext for the caller. This
    context must remain captured until auditing is complete, and passed to
    any routine that may have to do access checking or auditing.

Arguments:

    ObjectAttributes - Supplies a pointer to the object attributes.

    ObjectType - Supplies an optional pointer to the object type descriptor.

    AccessMode - Supplies the processor mode of the access.

    AccessState - Supplies an optional pointer to the current access status
        describing already granted access types, the privileges used to get
        them, and any access types yet to be granted.

    DesiredAcess - Supplies the desired access to the object.

    ParseContext - Supplies an optional pointer to parse context.

    Handle - Supplies a pointer to a variable that receives the handle value.

Return Value:

    If the object is successfully opened, then a handle for the object is
    created and a success status is returned. Otherwise, an error status is
    returned.

--*/

{
    NTSTATUS Status;
    NTSTATUS HandleStatus;
    PVOID ExistingObject;
    HANDLE NewHandle;
    BOOLEAN DirectoryLocked;
    OB_OPEN_REASON OpenReason;
    POBJECT_HEADER ObjectHeader;
    OBJECT_CREATE_INFORMATION ObjectCreateInfo;
    UNICODE_STRING CapturedObjectName;
    ACCESS_STATE LocalAccessState;
    AUX_ACCESS_DATA AuxData;
    PGENERIC_MAPPING GenericMapping;

    PAGED_CODE();

    ObpValidateIrql("ObOpenObjectByName");

    //
    //  If the object attributes are not specified, then return an error.
    //

    *Handle = NULL;

    if (!ARGUMENT_PRESENT(ObjectAttributes)) {

        Status = STATUS_INVALID_PARAMETER;

    } else {

        //
        //  Capture the object creation information.
        //

        Status = ObpCaptureObjectCreateInformation( ObjectType,
                                                    AccessMode,
                                                    ObjectAttributes,
                                                    &CapturedObjectName,
                                                    &ObjectCreateInfo,
                                                    TRUE );

        //
        //  If the object creation information is successfully captured,
        //  then generate the access state.
        //

        if (NT_SUCCESS(Status)) {

            if (!ARGUMENT_PRESENT(AccessState)) {

                //
                //  If an object type descriptor is specified, then use
                //  associated generic mapping. Otherwise, use no generic
                //  mapping.
                //

                GenericMapping = NULL;

                if (ARGUMENT_PRESENT(ObjectType)) {

                    GenericMapping = &ObjectType->TypeInfo.GenericMapping;
                }

                AccessState = &LocalAccessState;

                Status = SeCreateAccessState( &LocalAccessState,
                                              &AuxData,
                                              DesiredAccess,
                                              GenericMapping );

                if (!NT_SUCCESS(Status)) {

                    goto FreeCreateInfo;
                }
            }

            //
            //  If there is a security descriptor specified in the object
            //  attributes, then capture it in the access state.
            //

            if (ObjectCreateInfo.SecurityDescriptor != NULL) {

                AccessState->SecurityDescriptor = ObjectCreateInfo.SecurityDescriptor;
            }

            //
            //  Validate the access state.
            //

            Status = ObpValidateAccessMask(AccessState);

            //
            //  If the access state is valid, then lookup the object by
            //  name.
            //

            if (NT_SUCCESS(Status)) {

                Status = ObpLookupObjectName( ObjectCreateInfo.RootDirectory,
                                              &CapturedObjectName,
                                              ObjectCreateInfo.Attributes,
                                              ObjectType,
                                              AccessMode,
                                              ParseContext,
                                              ObjectCreateInfo.SecurityQos,
                                              NULL,
                                              AccessState,
                                              &DirectoryLocked,
                                              &ExistingObject );

                //
                //  If the object was successfully looked up, then attempt
                //  to create or open a handle.
                //

                if (NT_SUCCESS(Status)) {

                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(ExistingObject);

                    //
                    //  If the object is being created, then the operation
                    //  must be a open-if operation. Otherwise, a handle to
                    //  an object is being opened.
                    //

                    if (ObjectHeader->Flags & OB_FLAG_NEW_OBJECT) {

                        OpenReason = ObCreateHandle;

                        if (ObjectHeader->ObjectCreateInfo != NULL) {

                            ObpFreeObjectCreateInformation(ObjectHeader->ObjectCreateInfo);
                            ObjectHeader->ObjectCreateInfo = NULL;
                        }

                    } else {

                        OpenReason = ObOpenHandle;
                    }

                    //
                    //  If any of the object attributes are invalid, then
                    //  return an error status.
                    //

                    if (ObjectHeader->Type->TypeInfo.InvalidAttributes & ObjectCreateInfo.Attributes) {

                        Status = STATUS_INVALID_PARAMETER;

                        if (DirectoryLocked) {

                            ObpLeaveRootDirectoryMutex();
                        }

                    } else {

                        //
                        //  The status returned by the object lookup routine
                        //  must be returned if the creation of a handle is
                        //  successful. Otherwise, the handle creation status
                        //  is returned.
                        //

                        HandleStatus = ObpCreateHandle( OpenReason,
                                                        ExistingObject,
                                                        ObjectType,
                                                        AccessState,
                                                        0,
                                                        ObjectCreateInfo.Attributes,
                                                        DirectoryLocked,
                                                        AccessMode,
                                                        (PVOID *)NULL,
                                                        &NewHandle );

                        if (!NT_SUCCESS(HandleStatus)) {

                            ObDereferenceObject(ExistingObject);

                            Status = HandleStatus;

                        } else {

                            *Handle = NewHandle;
                        }
                    }

                } else {

                    if (DirectoryLocked) {

                        ObpLeaveRootDirectoryMutex();
                    }
                }
            }

            //
            //  If the access state was generated, then delete the access
            //  state.
            //

            if (AccessState == &LocalAccessState) {

                SeDeleteAccessState(AccessState);
            }

            //
            //  Free the create information.
            //

        FreeCreateInfo:

            ObpReleaseObjectCreateInformation(&ObjectCreateInfo);

            if (CapturedObjectName.Buffer != NULL) {

                ObpFreeObjectNameBuffer(&CapturedObjectName);
            }
        }
    }

    return Status;
}


NTSTATUS
ObOpenObjectByPointer (
    IN PVOID Object,
    IN ULONG HandleAttributes,
    IN PACCESS_STATE PassedAccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This routine opens an object referenced by a pointer.

Arguments:

    Object - A pointer to the object being opened.

    HandleAttributes - The desired attributes for the handle, such
        as OBJ_INHERIT, OBJ_PERMANENT, OBJ_EXCLUSIVE, OBJ_CASE_INSENSITIVE,
        OBJ_OPENIF, and OBJ_OPENLINK

    PassedAccessState - Supplies an optional pointer to the current access
        status describing already granted access types, the privileges used
        to get them, and any access types yet to be granted.

    DesiredAcess - Supplies the desired access to the object.

    ObjectType - Supplies the type of the object being opened

    AccessMode - Supplies the processor mode of the access.

    Handle - Supplies a pointer to a variable that receives the handle value.

Return Value:

    An appropriate NTSTATUS value

--*/

{
    NTSTATUS Status;
    HANDLE NewHandle;
    POBJECT_HEADER ObjectHeader;
    ACCESS_STATE LocalAccessState;
    PACCESS_STATE AccessState = NULL;
    AUX_ACCESS_DATA AuxData;

    PAGED_CODE();

    ObpValidateIrql( "ObOpenObjectByPointer" );

    //
    //  First increment the pointer count for the object.  This routine
    //  also checks the object types
    //

    Status = ObReferenceObjectByPointer( Object,
                                         0,
                                         ObjectType,
                                         AccessMode );

    if (NT_SUCCESS( Status )) {

        //
        //  Get the object header for the input object body
        //

        ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

        //
        //  If the caller did not pass in an access state then
        //  we will create a new one based on the desired access
        //  and the object types generic mapping
        //

        if (!ARGUMENT_PRESENT( PassedAccessState )) {

            Status = SeCreateAccessState( &LocalAccessState,
                                          &AuxData,
                                          DesiredAccess,
                                          &ObjectHeader->Type->TypeInfo.GenericMapping );

            if (!NT_SUCCESS( Status )) {

                ObDereferenceObject( Object );

                return(Status);
            }

            AccessState = &LocalAccessState;

        //
        //  Otherwise the caller did specify an access state so
        //  we use the one passed in.
        //

        } else {

            AccessState = PassedAccessState;
        }

        //
        //  Make sure the caller is asking for handle attributes that are
        //  valid for the given object type
        //

        if (ObjectHeader->Type->TypeInfo.InvalidAttributes & HandleAttributes) {

            if (AccessState == &LocalAccessState) {

                SeDeleteAccessState( AccessState );
            }

            ObDereferenceObject( Object );

            return( STATUS_INVALID_PARAMETER );
        }

        //
        //  We've referenced the object and have an access state to give
        //  the new handle so now create a new handle for the object.
        //

        Status = ObpCreateHandle( ObOpenHandle,
                                  Object,
                                  ObjectType,
                                  AccessState,
                                  0,
                                  HandleAttributes,
                                  FALSE,
                                  AccessMode,
                                  (PVOID *)NULL,
                                  &NewHandle );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( Object );
        }
    }

    //
    //  If we successfully opened by object and created a new handle
    //  then set the output variable correctly
    //

    if (NT_SUCCESS( Status )) {

        *Handle = NewHandle;

    } else {

        *Handle = NULL;
    }

    //
    //  Check if we used our own access state and now need to cleanup
    //

    if (AccessState == &LocalAccessState) {

        SeDeleteAccessState( AccessState );
    }

    //
    //  And return to our caller
    //

    return( Status );
}


NTSTATUS
ObReferenceObjectByHandle (
    IN HANDLE Handle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType OPTIONAL,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *Object,
    OUT POBJECT_HANDLE_INFORMATION HandleInformation OPTIONAL
    )

/*++

Routine Description:

    Given a handle to an object this routine returns a pointer
    to the body of the object with proper ref counts

Arguments:

    Handle - Supplies a handle to the object being referenced.  It can
        also be the result of NtCurrentProcess or NtCurrentThread

    DesiredAccess - Supplies the access being requested by the caller

    ObjectType - Optionally supplies the type of the object we
        are expecting

    AccessMode - Supplies the processor mode of the access

    Object - Receives a pointer to the object body if the operation
        is successful

    HandleInformation - Optionally receives information regarding the
        input handle.

Return Value:

    An appropriate NTSTATUS value

--*/

{
    ACCESS_MASK GrantedAccess;
    PHANDLE_TABLE HandleTable;
    POBJECT_HEADER ObjectHeader;
    PHANDLE_TABLE_ENTRY ObjectTableEntry;
    PEPROCESS Process;
    NTSTATUS Status;
    PETHREAD Thread;
    BOOLEAN AttachedToProcess = FALSE;

    ObpValidateIrql("ObReferenceObjectByHandle");

    //
    //  Protect ourselves from being interrupted while we hold a handle table
    //  entry lock
    //

    KeEnterCriticalRegion();

    try {

        //
        //  If the handle is equal to the current process handle and the object
        //  type is NULL or type process, then attempt to translate a handle to
        //  the current process. Otherwise, check if the handle is the current
        //  thread handle.
        //

        if (Handle == NtCurrentProcess()) {

            if ((ObjectType == PsProcessType) || (ObjectType == NULL)) {

                Process = PsGetCurrentProcess();
                GrantedAccess = Process->GrantedAccess;

                if ((SeComputeDeniedAccesses(GrantedAccess, DesiredAccess) == 0) ||
                    (AccessMode == KernelMode)) {

                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Process);

                    if (ARGUMENT_PRESENT(HandleInformation)) {

                        HandleInformation->GrantedAccess = GrantedAccess;
                        HandleInformation->HandleAttributes = 0;
                    }

                    ObpIncrPointerCount(ObjectHeader);
                    *Object = Process;

                    ASSERT( *Object != NULL );

                    Status = STATUS_SUCCESS;
                    leave;

                } else {

                    Status = STATUS_ACCESS_DENIED;
                }

            } else {

                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }

        //
        //  If the handle is equal to the current thread handle and the object
        //  type is NULL or type thread, then attempt to translate a handle to
        //  the current thread. Otherwise, the we'll try and translate the
        //  handle
        //

        } else if (Handle == NtCurrentThread()) {

            if ((ObjectType == PsThreadType) || (ObjectType == NULL)) {

                Thread = PsGetCurrentThread();
                GrantedAccess = Thread->GrantedAccess;

                if ((SeComputeDeniedAccesses(GrantedAccess, DesiredAccess) == 0) ||
                    (AccessMode == KernelMode)) {

                    ObjectHeader = OBJECT_TO_OBJECT_HEADER(Thread);

                    if (ARGUMENT_PRESENT(HandleInformation)) {

                        HandleInformation->GrantedAccess = GrantedAccess;
                        HandleInformation->HandleAttributes = 0;
                    }

                    ObpIncrPointerCount(ObjectHeader);
                    *Object = Thread;

                    ASSERT( *Object != NULL );

                    Status = STATUS_SUCCESS;
                    leave;

                } else {

                    Status = STATUS_ACCESS_DENIED;
                }

            } else {

                Status = STATUS_OBJECT_TYPE_MISMATCH;
            }

        //
        //  Otherwise the handle is not a built in value.  It must be an index
        //  into a handle table.
        //

        } else {

    #if DBG
            //
            //  On checked builds, check that if the Kernel handle bit is set,
            //  then we're coming from Kernel mode. We should probably fail the
            //  call if bit set && !Kmode
            //
            //  We know we're NOT a builtin handle at this point
            //

            ASSERT((Handle < 0) ? (AccessMode == KernelMode) : TRUE);
    #endif

            //
            //  First get a pointer to either the processes handle table or the
            //  global kernel handle table.  If it is the kernel handle then we
            //  need to clear the handle bit and attach to the system process
            //

            if (IsKernelHandle( Handle, AccessMode )) {

                //
                //  Make the handle look like a regular handle
                //

                Handle = DecodeKernelHandle( Handle );

                //
                //  The global kernel handle table
                //

                HandleTable = ObpKernelHandleTable;

            } else {

                HandleTable = ObpGetObjectTable();
            }

            ASSERT(HandleTable != NULL);

            //
            //  Translate the specified handle to an object table index.
            //

            ObjectTableEntry = ExMapHandleToPointer( HandleTable, Handle );

            //
            //  Make sure the object table entry really does exist
            //

            if (ObjectTableEntry != NULL) {

                ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

                if ((ObjectHeader->Type == ObjectType) || (ObjectType == NULL)) {

    #if i386 && !FPO
                    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

                        if ((AccessMode != KernelMode) || ARGUMENT_PRESENT(HandleInformation)) {

                            GrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );
                        }

                    } else {

                        GrantedAccess = ObjectTableEntry->GrantedAccess;
                    }
    #else
                    GrantedAccess = ObjectTableEntry->GrantedAccess;

    #endif // i386 && !FPO

                    if ((SeComputeDeniedAccesses(GrantedAccess, DesiredAccess) == 0) ||
                        (AccessMode == KernelMode)) {

                        //
                        //  Access to the object is allowed. Return the handle
                        //  information is requested, increment the object
                        //  pointer count, unlock the handle table and return
                        //  a success status.
                        //
                        //  Note that this is the only successful return path
                        //  out of this routine if the user did not specify
                        //  the current process or current thread in the input
                        //  handle.
                        //

                        if (ARGUMENT_PRESENT(HandleInformation)) {

                            HandleInformation->GrantedAccess = GrantedAccess;
                            HandleInformation->HandleAttributes = ObjectTableEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES;
                        }

                        ObpIncrPointerCount(ObjectHeader);
                        *Object = &ObjectHeader->Body;

                        ASSERT( *Object != NULL );

                        ExUnlockHandleTableEntry( HandleTable, ObjectTableEntry );

                        Status = STATUS_SUCCESS;
                        leave;

                    } else {

                        Status = STATUS_ACCESS_DENIED;
                    }

                } else {

                    Status = STATUS_OBJECT_TYPE_MISMATCH;
                }

                ExUnlockHandleTableEntry( HandleTable, ObjectTableEntry );

            } else {

                Status = STATUS_INVALID_HANDLE;
            }
        }

        //
        //  If we are attached to the system process then return
        //  back to our caller
        //

        if (AttachedToProcess) {

            KeDetachProcess();
        }

        //
        //  No handle translation is possible. Set the object address to NULL
        //  and return an error status.
        //

        *Object = NULL;

    } finally {

        KeLeaveCriticalRegion();
    }

    return Status;
}


NTSTATUS
ObReferenceObjectByName (
    IN PUNICODE_STRING ObjectName,
    IN ULONG Attributes,
    IN PACCESS_STATE AccessState OPTIONAL,
    IN ACCESS_MASK DesiredAccess OPTIONAL,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode,
    IN OUT PVOID ParseContext OPTIONAL,
    OUT PVOID *Object
    )

/*++

Routine Description:

    Given a name of an object this routine returns a pointer
    to the body of the object with proper ref counts

Arguments:

    ObjectName - Supplies the name of the object being referenced

    Attributes - Supplies the desired handle attributes

    AccessState - Supplies an optional pointer to the current access
        status describing already granted access types, the privileges used
        to get them, and any access types yet to be granted.

    DesiredAccess - Optionally supplies the desired access to the
        for the object

    ObjectType - Specifies the object type according to the caller

    AccessMode - Supplies the processor mode of the access

    ParseContext - Optionally supplies a context to pass down to the
        parse routine

    Object - Receives a pointer to the referenced object body

Return Value:

    An appropriate NTSTATUS value

--*/

{
    UNICODE_STRING CapturedObjectName;
    BOOLEAN DirectoryLocked;
    PVOID ExistingObject;
    ACCESS_STATE LocalAccessState;
    AUX_ACCESS_DATA AuxData;
    NTSTATUS Status;

    PAGED_CODE();

    ObpValidateIrql("ObReferenceObjectByName");

    //
    //  If the object name descriptor is not specified, or the object name
    //  length is zero (tested after capture), then the object name is
    //  invalid.
    //

    if (ObjectName == NULL) {

        return STATUS_OBJECT_NAME_INVALID;
    }

    //
    //  Capture the object name.
    //

    Status = ObpCaptureObjectName( AccessMode,
                                   ObjectName,
                                   &CapturedObjectName,
                                   TRUE );

    if (NT_SUCCESS(Status)) {

        //
        //  No buffer has been allocated for a zero length name so no free
        //  needed
        //

        if (CapturedObjectName.Length == 0) {

           return STATUS_OBJECT_NAME_INVALID;
        }

        //
        //  If the access state is not specified, then create the access
        //  state.
        //

        if (!ARGUMENT_PRESENT(AccessState)) {

            AccessState = &LocalAccessState;

            Status = SeCreateAccessState( &LocalAccessState,
                                          &AuxData,
                                          DesiredAccess,
                                          &ObjectType->TypeInfo.GenericMapping );

            if (!NT_SUCCESS(Status)) {

                goto FreeBuffer;
            }
        }

        //
        //  Lookup object by name.
        //

        Status = ObpLookupObjectName( NULL,
                                      &CapturedObjectName,
                                      Attributes,
                                      ObjectType,
                                      AccessMode,
                                      ParseContext,
                                      NULL,
                                      NULL,
                                      AccessState,
                                      &DirectoryLocked,
                                      &ExistingObject );

        //
        //  If the directory is returned locked, then unlock it.
        //

        if (DirectoryLocked) {

            ObpLeaveRootDirectoryMutex();
        }

        //
        //  If the lookup was successful, then return the existing
        //  object if access is allowed. Otherwise, return NULL.
        //

        *Object = NULL;

        if (NT_SUCCESS(Status)) {

            if (ObpCheckObjectReference( ExistingObject,
                                         AccessState,
                                         FALSE,
                                         AccessMode,
                                         &Status )) {

                *Object = ExistingObject;
            }
        }

        //
        //  If the access state was generated, then delete the access
        //  state.
        //

        if (AccessState == &LocalAccessState) {

            SeDeleteAccessState(AccessState);
        }

        //
        //  Free the object name buffer.
        //

FreeBuffer:

        ObpFreeObjectNameBuffer(&CapturedObjectName);
    }

    return Status;
}


NTSTATUS
ObReferenceObjectByPointer (
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_TYPE ObjectType,
    IN KPROCESSOR_MODE AccessMode
    )

/*++

Routine Description:

    This routine adds another reference count to an object denoted by
    a pointer to the object body

Arguments:

    Object - Supplies a pointer to the object being referenced

    DesiredAccess - Specifies the desired access for the reference

    ObjectType - Specifies the object type according to the caller

    AccessMode - Supplies the processor mode of the access

Return Value:

    STATUS_SUCCESS if successful and STATUS_OBJECT_TYPE_MISMATCH otherwise

--*/

{
    POBJECT_HEADER ObjectHeader;

    //
    //  Translate the pointer to the object body to a pointer to the
    //  object header
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    //
    //  If the specified object type does not match and either the caller is
    //  not kernel mode or it is not a symbolic link object then it is an
    //  error
    //

    if ((ObjectHeader->Type != ObjectType) && (AccessMode != KernelMode ||
                                               ObjectType == ObpSymbolicLinkObjectType)) {

        return( STATUS_OBJECT_TYPE_MISMATCH );
    }

    //
    //  Otherwise increment the pointer count and return success to
    //  our caller
    //

    ObpIncrPointerCount( ObjectHeader );

    return( STATUS_SUCCESS );
}


VOID
FASTCALL
ObfReferenceObject (
    IN PVOID Object
    )

/*++

Routine Description:

    This function increments the reference count for an object.

    N.B. This function should be used to increment the reference count
        when the accessing mode is kernel or the objct type is known.

Arguments:

    Object - Supplies a pointer to the object whose reference count is
        incremented.

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    ObpIncrPointerCount( ObjectHeader );

    return;
}


VOID
FASTCALL
ObfDereferenceObject (
    IN PVOID Object
    )

/*++

Routine Description:

    This routine decrments the refernce count of the specified object and
    does whatever cleanup there is if the count goes to zero.

Arguments:

    Object - Supplies a pointer to the body of the object being dereferenced

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    KIRQL OldIrql;
    BOOLEAN StartWorkerThread;

    PLPCP_PORT_OBJECT Port = NULL;

#if DBG

    POBJECT_HEADER_NAME_INFO NameInfo;

#endif

    //
    //  Translate a pointer to the object body to a pointer to the object
    //  header.
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

#if DBG

    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    if (NameInfo) {

       InterlockedDecrement(&NameInfo->DbgDereferenceCount) ;
    }

#endif


    //
    //  Decrement the point count and if the result is now then
    //  there is extra work to do
    //

    ObjectType = ObjectHeader->Type;

    if ( (ObjectType == LpcPortObjectType) ||
         (ObjectType == LpcWaitablePortObjectType) ) {

        Port = Object;
        LpcpAcquireLpcpLock();
    }

    if (ObpDecrPointerCountWithResult( ObjectHeader )) {

        //
        //  Find out the level we're at and the object type
        //

        OldIrql = KeGetCurrentIrql();

        ASSERT(ObjectHeader->HandleCount == 0);


        if (Port != NULL) {

            Port->Flags |= PORT_DELETED;
            Port = NULL;

            LpcpReleaseLpcpLock();
        }


        //
        //  If we're at the passive level then go ahead and delete the
        //  object now.  Or if we're at APC level and object say's we're
        //  allocated out of paged pool then also go ahead and delete
        //  the object right now.
        //

        if ((OldIrql == PASSIVE_LEVEL) ||
            ((OldIrql == APC_LEVEL) &&
             ((ObjectType != NULL) && (ObjectType->TypeInfo.PoolType != NonPagedPool)))) {


                ObpRemoveObjectRoutine( Object );

                return;    

        } else {

            //
            //  Objects can't be deleted from an IRQL above APC_LEVEL.
            //  Nonpaged objects can't be deleted from APC_LEVEL.
            //  So queue the delete operation.
            //

            ASSERT((ObjectHeader->Type == NULL) || (ObjectHeader->Type->TypeInfo.PoolType == NonPagedPool));

            //
            //  Lock the work queue, enqueue the new work item, and if the
            //  work queue is not active then make it active, indicate that
            //  we need to start the worker thread, and unlock the work
            //  queue.
            //

            ExAcquireSpinLock( &ObpLock, &OldIrql );

            PushEntryList((PSINGLE_LIST_ENTRY)&ObpRemoveObjectQueue, (PSINGLE_LIST_ENTRY)&ObjectHeader->SEntry );

            if (!ObpRemoveQueueActive) {

                ObpRemoveQueueActive = TRUE;
                StartWorkerThread = TRUE;

            } else {

                StartWorkerThread = FALSE;
            }

#if 0
            if (StartWorkerThread) {

                KdPrint(( "OB: %08x Starting ObpProcessRemoveObjectQueue thread.\n", Object ));

            } else {

                KdPrint(( "OB: %08x Queued to ObpProcessRemoveObjectQueue thread.\n", Object ));
            }

#endif  // 1

            ExReleaseSpinLock( &ObpLock, OldIrql );

            //
            //  If we have to start the worker thread then go ahead
            //  and enqueue the work item
            //

            if (StartWorkerThread) {

                ExInitializeWorkItem( &ObpRemoveObjectWorkItem,
                                      ObpProcessRemoveObjectQueue,
                                      NULL );

                ExQueueWorkItem( &ObpRemoveObjectWorkItem, CriticalWorkQueue );
            }
        }
    }

    if ( Port != NULL ) {

        LpcpReleaseLpcpLock();
    }

    return;
}


VOID
ObpProcessRemoveObjectQueue (
    PVOID Parameter
    )

/*++

Routine Description:

    This is the work routine for the remove object work queue.  Its
    job is to remove and process items from the remove object queue.

Arguments:

    Parameter - Ignored

Return Value:

    None.

--*/

{
    PSINGLE_LIST_ENTRY Entry;
    POBJECT_HEADER ObjectHeader;
    KIRQL OldIrql;

    //
    //  Lock the work queue this will keep the preceding routine
    //  from monkeying with the queue
    //

    ExAcquireSpinLock( &ObpLock, &OldIrql );

    //
    //  While there are items in our private remove object work queue
    //  then we remove each item, get back up to the object header,
    //  and delete the object.  The latter part done outside of the
    //  work queue lock
    //

    Entry = PopEntryList( (PSINGLE_LIST_ENTRY)&ObpRemoveObjectQueue );

    while ( Entry != NULL ) {

        ExReleaseSpinLock( &ObpLock, OldIrql );

        ObjectHeader = CONTAINING_RECORD( Entry,
                                          OBJECT_HEADER,
                                          SEntry );

        ObpRemoveObjectRoutine( &ObjectHeader->Body );

        ExAcquireSpinLock( &ObpLock, &OldIrql );

        Entry = PopEntryList((PSINGLE_LIST_ENTRY)&ObpRemoveObjectQueue );
    }

    //
    //  Indicate that we are now going inactive and then unlock
    //  the work queue
    //

    ObpRemoveQueueActive = FALSE;

    ExReleaseSpinLock( &ObpLock, OldIrql );

    return;
}


VOID
ObpRemoveObjectRoutine (
    PVOID Object
    )

/*++

Routine Description:

    This routine is used to delete an object whose reference count has
    gone to zero.

Arguments:

    Object - Supplies a pointer to the body of the object being deleted

Return Value:

    None.

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_NAME_INFO NameInfo;

    PAGED_CODE();

    ObpValidateIrql( "ObpRemoveObjectRoutine" );

    //
    //  Retrieve an object header from the object body, and also get
    //  the object type, creator and name info if available
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectType = ObjectHeader->Type;
    CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );

    //
    //  Get exclusive access to the object type object
    //

    ObpEnterObjectTypeMutex( ObjectType );

    //
    //  If there is a creator info record and we are on the list
    //  for the object type then remove this object from the list
    //

    if (CreatorInfo != NULL && !IsListEmpty( &CreatorInfo->TypeList )) {

        RemoveEntryList( &CreatorInfo->TypeList );
    }

    //
    //  If there is a name info record and the name buffer is not null
    //  then free the buffer and zero out the name record
    //

    if (NameInfo != NULL && NameInfo->Name.Buffer != NULL) {

        ExFreePool( NameInfo->Name.Buffer );

        NameInfo->Name.Buffer = NULL;
        NameInfo->Name.Length = 0;
        NameInfo->Name.MaximumLength = 0;
    }

    //
    //  We are done with the object type object so we can now release it
    //

    ObpLeaveObjectTypeMutex( ObjectType );

    //
    //  Security descriptor deletion must precede the
    //  call to the object's DeleteProcedure.  Check if we have
    //  a security descriptor and if so then call the routine
    //  to delete the security descritpor.
    //

    if (ObjectHeader->SecurityDescriptor != NULL) {

        KIRQL SaveIrql;

        ObpBeginTypeSpecificCallOut( SaveIrql );

        Status = (ObjectType->TypeInfo.SecurityProcedure)( Object,
                                                           DeleteSecurityDescriptor,
                                                           NULL, NULL, NULL,
                                                           &ObjectHeader->SecurityDescriptor,
                                                           0,
                                                           NULL );

        ObpEndTypeSpecificCallOut( SaveIrql, "Security", ObjectType, Object );
    }

    //
    //  Now if there is a delete callback for the object type invoke
    //  the routine
    //

    if (ObjectType->TypeInfo.DeleteProcedure) {

        KIRQL SaveIrql;

        ObpBeginTypeSpecificCallOut( SaveIrql );

        (*(ObjectType->TypeInfo.DeleteProcedure))( Object );

        ObpEndTypeSpecificCallOut( SaveIrql, "Delete", ObjectType, Object );
    }

    //
    //  Finally return the object back to pool including releasing any quota
    //  charges
    //

    ObpFreeObject( Object );
}


VOID
ObpDeleteNameCheck (
    IN PVOID Object,
    IN BOOLEAN TypeMutexHeld
    )

/*++

Routine Description:

    This routine removes the name of an object from its parent directory

Arguments:

    Object - Supplies a pointer to the object body whose name is being checked

    TypeMutexHeld - Indicates if the lock on object type is being held by the
        caller

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    POBJECT_HEADER_NAME_INFO NameInfo;
    PVOID DirObject;

    PAGED_CODE();

    ObpValidateIrql( "ObpDeleteNameCheck" );

    //
    //  Translate the object body to an object header also get
    //  the object type and name info if present
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    NameInfo = OBJECT_HEADER_TO_NAME_INFO( ObjectHeader );
    ObjectType = ObjectHeader->Type;

    //
    //  If the lock is not held then get the lock now
    //

    if (!TypeMutexHeld) {

        ObpEnterObjectTypeMutex( ObjectType );
    }

    //
    //  Make sure that the object has a zero handle count, has a non
    //  empty name buffer, and is not a permanent object
    //

    if ((ObjectHeader->HandleCount == 0) &&
        (NameInfo != NULL) &&
        (NameInfo->Name.Length != 0) &&
        (!(ObjectHeader->Flags & OB_FLAG_PERMANENT_OBJECT))) {

        //
        //  Give up the lock on the object type and instead
        //  get the lock on the object directories
        //

        ObpLeaveObjectTypeMutex( ObjectType );
        ObpEnterRootDirectoryMutex();
        DirObject = NULL;

        //
        //  Check that the object we is still in the directory otherwise
        //  then is nothing for us to remove
        //

        if (Object == ObpLookupDirectoryEntry( NameInfo->Directory,
                                               &NameInfo->Name,
                                               0 )) {

            //
            //  Now reacquire the lock on the object type and
            //  check check the handle count again.  If it is still
            //  zero then we can do the actual delete name operation
            //

            ObpEnterObjectTypeMutex( ObjectType );

            if (ObjectHeader->HandleCount == 0) {

                KIRQL SaveIrql;

                //
                //  Delete the directory entry 
                //
                
                ObpDeleteDirectoryEntry( NameInfo->Directory );
                
                //
                //  If security is not required invoke the 
                //  security callback to delete the security descriptor
                //
                
                if ( !ObjectType->TypeInfo.SecurityRequired ) {
                    
                    ObpBeginTypeSpecificCallOut( SaveIrql );

                    (ObjectType->TypeInfo.SecurityProcedure)( Object,
                                                              DeleteSecurityDescriptor,
                                                              NULL,
                                                              NULL,
                                                              NULL,
                                                              &ObjectHeader->SecurityDescriptor,
                                                              ObjectType->TypeInfo.PoolType,
                                                              NULL );

                    ObpEndTypeSpecificCallOut( SaveIrql, "Security", ObjectType, Object );
                }
                
                //
                //  If this is a symbolic link object then we also need to
                //  delete the symbolic link
                //

                if (ObjectType == ObpSymbolicLinkObjectType) {

                    ObpDeleteSymbolicLinkName( (POBJECT_SYMBOLIC_LINK)Object );
                }

                //
                //  Free the name buffer and zero out the name data fields
                //

                ExFreePool( NameInfo->Name.Buffer );

                NameInfo->Name.Buffer = NULL;
                NameInfo->Name.Length = 0;
                NameInfo->Name.MaximumLength = 0;

                DirObject = NameInfo->Directory;
                NameInfo->Directory = NULL;
            }

            ObpLeaveObjectTypeMutex( ObjectType );
        }

        ObpLeaveRootDirectoryMutex();

        //
        //  If there is a directory object for the name then decrement
        //  its reference count for it and for the object
        //

        if (DirObject != NULL) {

            ObDereferenceObject( DirObject );
            ObDereferenceObject( Object );
        }

    } else {

        //
        //  Otherwise don't do any work but simply release the object type
        //  lock and return to our caller
        //

        ObpLeaveObjectTypeMutex( ObjectType );
    }

    return;
}


//
// Thunks to support standard call callers
//

#ifdef ObDereferenceObject
#undef ObDereferenceObject
#endif

VOID
ObDereferenceObject (
    IN PVOID Object
    )

/*++

Routine Description:

    This is really just a thunk for the Obf version of the dereference
    routine

Arguments:

    Object - Supplies a pointer to the body of the object being dereferenced

Return Value:

    None.

--*/

{
    ObfDereferenceObject (Object) ;
}
