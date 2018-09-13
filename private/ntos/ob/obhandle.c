/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obhandle.c

Abstract:

    Object handle routines

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

//
//  Define logical sum of all generic accesses.
//

#define GENERIC_ACCESS (GENERIC_READ | GENERIC_WRITE | GENERIC_EXECUTE | GENERIC_ALL)

//
//  The following variable is declared in obinit.c and is used to protect the
//  process object table
//

extern KMUTANT ObpInitKillMutant;

//
// Define local prototypes
//

NTSTATUS
ObpIncrementHandleDataBase (
    IN POBJECT_HEADER ObjectHeader,
    IN PEPROCESS Process,
    OUT PULONG NewProcessHandleCount
    );

NTSTATUS
ObpCaptureHandleInformation (
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PVOID HandleTableEntry,
    IN HANDLE HandleIndex,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtDuplicateObject)
#pragma alloc_text(PAGE,ObGetHandleInformation)
#pragma alloc_text(PAGE,ObpInsertHandleCount)
#pragma alloc_text(PAGE,ObpIncrementHandleCount)
#pragma alloc_text(PAGE,ObpIncrementUnnamedHandleCount)
#pragma alloc_text(PAGE,ObpDecrementHandleCount)
#pragma alloc_text(PAGE,ObpCreateHandle)
#pragma alloc_text(PAGE,ObpCreateUnnamedHandle)
#pragma alloc_text(PAGE,ObpIncrementHandleDataBase)
#endif


//
//  Define routines for incrementing and decrementing the object header
//  counters.  These routines are only used if MPSAFE handle count check
//  is defined
//

#ifdef MPSAFE_HANDLE_COUNT_CHECK

VOID
FASTCALL
ObpIncrPointerCount (
    IN POBJECT_HEADER ObjectHeader
    )
{
    KIRQL OldIrql;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    ObjectHeader->PointerCount += 1;
    ExReleaseFastLock( &ObpLock, OldIrql );
}

VOID
FASTCALL
ObpDecrPointerCount (
    IN POBJECT_HEADER ObjectHeader
    )
{
    KIRQL OldIrql;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    ObjectHeader->PointerCount -= 1;
    ExReleaseFastLock( &ObpLock, OldIrql );
}

BOOLEAN
FASTCALL
ObpDecrPointerCountWithResult (
    IN POBJECT_HEADER ObjectHeader
    )
{
    KIRQL OldIrql;
    LONG Result;

    ExAcquireFastLock( &ObpLock, &OldIrql );

    if (ObjectHeader->PointerCount <= ObjectHeader->HandleCount) {

        DbgPrint( "OB: About to over-dereference object %x (ObjectHeader at %x)\n",
                  ObjectHeader->Object, ObjectHeader );

        DbgBreakPoint();
    }

    ObjectHeader->PointerCount -= 1;
    Result = ObjectHeader->PointerCount;
    ExReleaseFastLock( &ObpLock, OldIrql );
    return Result == 0;
}

VOID
FASTCALL
ObpIncrHandleCount (
    IN POBJECT_HEADER ObjectHeader
    )
{
    KIRQL OldIrql;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    ObjectHeader->HandleCount += 1;
    ExReleaseFastLock( &ObpLock, OldIrql );
    }

BOOLEAN
FASTCALL
ObpDecrHandleCount (
    IN POBJECT_HEADER ObjectHeader
    )
{
    KIRQL OldIrql;
    LONG Old;

    ExAcquireFastLock( &ObpLock, &OldIrql );
    Old = ObjectHeader->HandleCount;
    ObjectHeader->HandleCount -= 1;
    ExReleaseFastLock( &ObpLock, OldIrql );

    return Old == 1;
}

#endif // MPSAFE_HANDLE_COUNT_CHECK


NTSTATUS
NtDuplicateObject (
    IN HANDLE SourceProcessHandle,
    IN HANDLE SourceHandle,
    IN HANDLE TargetProcessHandle OPTIONAL,
    OUT PHANDLE TargetHandle OPTIONAL,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    IN ULONG Options
    )

/*++

Routine Description:

    This function creates a handle that is a duplicate of the specified
    source handle.  The source handle is evaluated in the context of the
    specified source process.  The calling process must have
    PROCESS_DUP_HANDLE access to the source process.  The duplicate
    handle is created with the specified attributes and desired access.
    The duplicate handle is created in the handle table of the specified
    target process.  The calling process must have PROCESS_DUP_HANDLE
    access to the target process.

Arguments:

    SourceProcessHandle - Supplies a handle to the source process for the
        handle being duplicated

    SourceHandle - Supplies the handle being duplicated

    TargetProcessHandle - Optionally supplies a handle to the target process
        that is to receive the new handle

    TargetHandle - Optionally returns a the new duplicated handle

    DesiredAccess - Desired access for the new handle

    HandleAttributes - Desired attributes for the new handle

    Options - Duplication options that control things like close source,
        same access, and same attributes.

Return Value:

    TBS

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID SourceObject;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PEPROCESS SourceProcess;
    PEPROCESS TargetProcess;
    BOOLEAN Attached;
    PVOID ObjectTable;
    HANDLE_TABLE_ENTRY ObjectTableEntry;
    OBJECT_HANDLE_INFORMATION HandleInformation;
    HANDLE NewHandle;
    ACCESS_STATE AccessState;
    AUX_ACCESS_DATA AuxData;
    ACCESS_MASK SourceAccess;
    ACCESS_MASK TargetAccess;
    PACCESS_STATE PassedAccessState = NULL;

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (ARGUMENT_PRESENT( TargetHandle ) && (PreviousMode != KernelMode)) {

        try {

            ProbeForWriteHandle( TargetHandle );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return( GetExceptionCode() );
        }
    }

    //
    //  If the caller is not asking for the same access then
    //  validate the access they are requesting doesn't contain
    //  any bad bits
    //

    if (!(Options & DUPLICATE_SAME_ACCESS)) {

        Status = ObpValidateDesiredAccess( DesiredAccess );

        if (!NT_SUCCESS( Status )) {

            return( Status );
        }
    }

    //
    //  The Attached variable indicates if we needed to
    //  attach to the source process because it was not the
    //  current process.
    //

    Attached = FALSE;

    //
    //  Given the input source process handle get a pointer
    //  to the source process object
    //

    Status = ObReferenceObjectByHandle( SourceProcessHandle,
                                        PROCESS_DUP_HANDLE,
                                        PsProcessType,
                                        PreviousMode,
                                        (PVOID *)&SourceProcess,
                                        NULL );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    //
    //  Lock down access to the process object tables
    //

    KeEnterCriticalRegion();

    KeWaitForSingleObject( &ObpInitKillMutant,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    //
    //  Make sure the source process has an object table still
    //

    if ( SourceProcess->ObjectTable == NULL ) {

        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

        KeLeaveCriticalRegion();

        ObDereferenceObject( SourceProcess );

        return STATUS_PROCESS_IS_TERMINATING;
    }

    //
    //  If the specified source process is not the current process, attach
    //  to the specified source process.  Then after we reference the object
    //  we can detach from the process.
    //

    if (PsGetCurrentProcess() != SourceProcess) {

        KeAttachProcess( &SourceProcess->Pcb );

        Attached = TRUE;
    }

    //
    //  The the input source handle get a pointer to the
    //  source object itself, then detach from the process
    //  if necessary and check if we were given a good
    //  source handle.
    //

    Status = ObReferenceObjectByHandle( SourceHandle,
                                        0,
                                        (POBJECT_TYPE)NULL,
                                        PreviousMode,
                                        &SourceObject,
                                        &HandleInformation );

    if (Attached) {

        KeDetachProcess();

        Attached = FALSE;
    }

    if (!NT_SUCCESS( Status )) {

        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

        KeLeaveCriticalRegion();

        ObDereferenceObject( SourceProcess );

        return( Status );
    }

    //
    //  We are all done if no target process handle was specified.
    //  This is practially a noop because the only really end result
    //  could be that we've closed the source handle.
    //

    if (!ARGUMENT_PRESENT( TargetProcessHandle )) {

        //
        //  If no TargetProcessHandle, then only possible option is to close
        //  the source handle in the context of the source process.
        //

        if (!(Options & DUPLICATE_CLOSE_SOURCE)) {

            Status = STATUS_INVALID_PARAMETER;
        }

        if (Options & DUPLICATE_CLOSE_SOURCE) {

            KeAttachProcess( &SourceProcess->Pcb );

            NtClose( SourceHandle );

            KeDetachProcess();
        }

        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

        KeLeaveCriticalRegion();

        ObDereferenceObject( SourceObject );
        ObDereferenceObject( SourceProcess );

        return( Status );
    }

    SourceAccess = HandleInformation.GrantedAccess;

    //
    //  At this point the caller did specify for a target process
    //  So from the target process handle get a pointer to the
    //  target process object.
    //

    Status = ObReferenceObjectByHandle( TargetProcessHandle,
                                        PROCESS_DUP_HANDLE,
                                        PsProcessType,
                                        PreviousMode,
                                        (PVOID *)&TargetProcess,
                                        NULL );

    //
    //  If we cannot get the traget process object then close the
    //  source down if requsted, cleanup and return to our caller
    //

    if (!NT_SUCCESS( Status )) {

        if (Options & DUPLICATE_CLOSE_SOURCE) {

            KeAttachProcess( &SourceProcess->Pcb );

            NtClose( SourceHandle );

            KeDetachProcess();
        }

        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

        KeLeaveCriticalRegion();

        ObDereferenceObject( SourceObject );
        ObDereferenceObject( SourceProcess );

        return( Status );
    }

    //
    //  Make sure the target process has not exited
    //

    if ( TargetProcess->ObjectTable == NULL ) {

        if (Options & DUPLICATE_CLOSE_SOURCE) {

            KeAttachProcess( &SourceProcess->Pcb );

            NtClose( SourceHandle );

            KeDetachProcess();
        }

        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

        KeLeaveCriticalRegion();

        ObDereferenceObject( SourceObject );
        ObDereferenceObject( SourceProcess );
        ObDereferenceObject( TargetProcess );

        return STATUS_PROCESS_IS_TERMINATING;
    }

    //
    //  If the specified target process is not the current process, attach
    //  to the specified target process.
    //

    if (PsGetCurrentProcess() != TargetProcess) {

        KeAttachProcess( &TargetProcess->Pcb );

        Attached = TRUE;
    }

    //
    //  Construct the proper desired access and attributes for the new handle
    //

    if (Options & DUPLICATE_SAME_ACCESS) {

        DesiredAccess = SourceAccess;
    }

    if (Options & DUPLICATE_SAME_ATTRIBUTES) {

        HandleAttributes = HandleInformation.HandleAttributes;

    } else {

        //
        //  Always propogate auditing information.
        //

        HandleAttributes |= HandleInformation.HandleAttributes & OBJ_AUDIT_OBJECT_CLOSE;
    }

    //
    //  Get the object header for the source object
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( SourceObject );
    ObjectType = ObjectHeader->Type;

    ObjectTableEntry.Object = ObjectHeader;
    ObjectTableEntry.ObAttributes |= (HandleAttributes & OBJ_HANDLE_ATTRIBUTES);

    //
    //  If any of the generic access bits are specified then map those to more
    //  specific access bits
    //

    if ((DesiredAccess & GENERIC_ACCESS) != 0) {

        RtlMapGenericMask( &DesiredAccess,
                           &ObjectType->TypeInfo.GenericMapping );
    }

    //
    //  Make sure to preserve ACCESS_SYSTEM_SECURITY, which most likely is not
    //  found in the ValidAccessMask
    //

    TargetAccess = DesiredAccess &
                   (ObjectType->TypeInfo.ValidAccessMask | ACCESS_SYSTEM_SECURITY);

    //
    //  If the access requested for the target is a superset of the
    //  access allowed in the source, perform full AVR.  If it is a
    //  subset or equal, do not perform any access validation.
    //
    //  Do not allow superset access if object type has a private security
    //  method, as there is no means to call them in this case to do the
    //  access check.
    //
    //  If the AccessState is not passed to ObpIncrementHandleCount
    //  there will be no AVR.
    //

    if (TargetAccess & ~SourceAccess) {

        if (ObjectType->TypeInfo.SecurityProcedure == SeDefaultObjectMethod) {

            Status = SeCreateAccessState( &AccessState,
                                          &AuxData,
                                          TargetAccess,       // DesiredAccess
                                          &ObjectType->TypeInfo.GenericMapping );

            PassedAccessState = &AccessState;

        } else {

            Status = STATUS_ACCESS_DENIED;
        }

    } else {

        //
        //  Do not perform AVR
        //

        PassedAccessState = NULL;

        Status = STATUS_SUCCESS;
    }

    //
    //  Increment the new handle count and get a pointer to
    //  the target processes object table
    //

    if ( NT_SUCCESS( Status )) {

        Status = ObpIncrementHandleCount( ObDuplicateHandle,
                                          PsGetCurrentProcess(), // this is already the target process
                                          SourceObject,
                                          ObjectType,
                                          PassedAccessState,
                                          PreviousMode,
                                          HandleAttributes );

        ObjectTable = ObpGetObjectTable();

        ASSERT(ObjectTable);
    }

    if (Attached) {

        KeDetachProcess();

        Attached = FALSE;
    }

    if (Options & DUPLICATE_CLOSE_SOURCE) {

        KeAttachProcess( &SourceProcess->Pcb );

        NtClose( SourceHandle );

        KeDetachProcess();
    }

    if (!NT_SUCCESS( Status )) {

        if (PassedAccessState != NULL) {

            SeDeleteAccessState( PassedAccessState );
        }

        KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

        KeLeaveCriticalRegion();

        ObDereferenceObject( SourceObject );
        ObDereferenceObject( SourceProcess );
        ObDereferenceObject( TargetProcess );

        return( Status );
    }

    if ((PassedAccessState != NULL) && (PassedAccessState->GenerateOnClose == TRUE)) {

        //
        //  If we performed AVR opening the handle, then mark the handle as needing
        //  auditing when it's closed.
        //

        ObjectTableEntry.ObAttributes |= OBJ_AUDIT_OBJECT_CLOSE;
    }

#if i386 && !FPO

    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

        ObjectTableEntry.GrantedAccessIndex = ObpComputeGrantedAccessIndex( TargetAccess );
        ObjectTableEntry.CreatorBackTraceIndex = RtlLogStackBackTrace();

    } else {

        ObjectTableEntry.GrantedAccess = TargetAccess;
    }

#else

    ObjectTableEntry.GrantedAccess = TargetAccess;

#endif // i386 && !FPO

    //
    //  Now that we've constructed a new object table entry for the duplicated handle
    //  we need to add it to the object table of the target process


    NewHandle = ExCreateHandle( ObjectTable, &ObjectTableEntry );

    if (NewHandle) {

        //
        //  We have a new handle to audit the creation of the new handle if
        //  AVR was done.  And set the optional output handle variable.  Note
        //  that if we reach here the status variable is already a success
        //

        if (PassedAccessState != NULL) {

            SeAuditHandleCreation( PassedAccessState, NewHandle );
        }

        if (SeDetailedAuditing && (ObjectTableEntry.ObAttributes & OBJ_AUDIT_OBJECT_CLOSE)) {

            SeAuditHandleDuplication( SourceHandle,
                                      NewHandle,
                                      SourceProcess,
                                      TargetProcess );
        }

        if (ARGUMENT_PRESENT( TargetHandle )) {

            try {

                *TargetHandle = NewHandle;

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                //  Fall through, since we cannot undo what we have done.
                //
            }
        }

    } else {

        //
        //  We didn't get a new handle to decrement the handle count dereference
        //  the necessary objects, set the optional output variable and indicate
        //  why we're failing
        //

        ObpDecrementHandleCount( TargetProcess,
                                 ObjectHeader,
                                 ObjectType,
                                 TargetAccess );

        ObDereferenceObject( SourceObject );

        if (ARGUMENT_PRESENT( TargetHandle )) {

            try {

                *TargetHandle = (HANDLE)NULL;

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                //
                //  Fall through so we can return the correct status.
                //
            }
        }

        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Cleanup from our selfs and then return to our caller
    //

    if (PassedAccessState != NULL) {

        SeDeleteAccessState( PassedAccessState );
    }

    KeReleaseMutant( &ObpInitKillMutant, 0, FALSE, FALSE );

    KeLeaveCriticalRegion();

    ObDereferenceObject( SourceProcess );
    ObDereferenceObject( TargetProcess );

    return( Status );
}


NTSTATUS
ObGetHandleInformation (
    OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
    )

/*++

Routine Description:

    This routine returns information about the specified handle.

Arguments:

    HandleInformation - Supplies an array of handle information
        structures to fill in

    Length - Supplies the length the handle information array in bytes

    ReturnLength - Receives the number of bytes used by this call

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;
    ULONG RequiredLength;

    PAGED_CODE();

    RequiredLength = FIELD_OFFSET( SYSTEM_HANDLE_INFORMATION, Handles );

    if (Length < RequiredLength) {

        return( STATUS_INFO_LENGTH_MISMATCH );
    }

    HandleInformation->NumberOfHandles = 0;

    //
    //  For every handle in every handle table we'll be calling
    //  our callback routine
    //

    Status = ExSnapShotHandleTables( ObpCaptureHandleInformation,
                                     HandleInformation,
                                     Length,
                                     &RequiredLength );

    if (ARGUMENT_PRESENT( ReturnLength )) {

        *ReturnLength = RequiredLength;
    }

    return( Status );
}


NTSTATUS
ObpCaptureHandleInformation (
    IN OUT PSYSTEM_HANDLE_TABLE_ENTRY_INFO *HandleEntryInfo,
    IN HANDLE UniqueProcessId,
    IN PHANDLE_TABLE_ENTRY ObjectTableEntry,
    IN HANDLE HandleIndex,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )

/*++

Routine Description:

    This is the callback routine of ObGetHandleInformation

Arguments:

    HandleEntryInfo - Supplies a pointer to the output buffer to receive
        the handle information

    UniqueProcessId - Supplies the process id of the caller

    ObjectTableEntry - Supplies the handle table entry that is being
        captured

    HandleIndex - Supplies the index for the preceding handle table entry

    Length - Specifies the length, in bytes, of the original user buffer

    RequiredLength - Specifies the length, in bytes, that has already been
        used in the buffer to store information.  On return this receives
        the updated number of bytes being used.

        Note that the HandleEntryInfo does not necessarily point to the
        start of the original user buffer.  It will have been offset by
        the feed-in RequiredLength value.

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;

    //
    //  Figure out who much size we really need to contain this extra record
    //  and then check that it fits.
    //

    *RequiredLength += sizeof( SYSTEM_HANDLE_TABLE_ENTRY_INFO );

    if (Length < *RequiredLength) {

        Status = STATUS_INFO_LENGTH_MISMATCH;

    } else {

        //
        //  Get the object header from the table entry and then copy over the information
        //

        ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);

        (*HandleEntryInfo)->UniqueProcessId       = (USHORT)((ULONG_PTR)UniqueProcessId);
        (*HandleEntryInfo)->HandleAttributes      = (UCHAR)(ObjectTableEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES);
        (*HandleEntryInfo)->ObjectTypeIndex       = (UCHAR)(ObjectHeader->Type->Index);
        (*HandleEntryInfo)->HandleValue           = (USHORT)((ULONG_PTR)(HandleIndex));
        (*HandleEntryInfo)->Object                = &ObjectHeader->Body;
        (*HandleEntryInfo)->CreatorBackTraceIndex = 0;

#if i386 && !FPO

        if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

            (*HandleEntryInfo)->CreatorBackTraceIndex = ObjectTableEntry->CreatorBackTraceIndex;
            (*HandleEntryInfo)->GrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );

        } else {

            (*HandleEntryInfo)->GrantedAccess = ObjectTableEntry->GrantedAccess;
        }

#else

        (*HandleEntryInfo)->GrantedAccess = ObjectTableEntry->GrantedAccess;

#endif // i386 && !FPO

        (*HandleEntryInfo)++;

        Status = STATUS_SUCCESS;
    }

    return( Status );
}


POBJECT_HANDLE_COUNT_ENTRY
ObpInsertHandleCount (
    POBJECT_HEADER ObjectHeader
    )

/*++

Routine Description:

    This function will increase the size of the handle database
    stored in the handle information of an object header.  If
    necessary it will allocate new and free old handle databases.

    This routine should not be called if there is already free
    space in the handle table.

Arguments:

    ObjectHeader - The object whose handle count is being incremented

Return Value:

    The pointer to the next free handle count entry within the
    handle database.

--*/

{
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_DATABASE OldHandleCountDataBase;
    POBJECT_HANDLE_COUNT_DATABASE NewHandleCountDataBase;
    POBJECT_HANDLE_COUNT_ENTRY FreeHandleCountEntry;
    ULONG CountEntries;
    ULONG OldSize;
    ULONG NewSize;
    OBJECT_HANDLE_COUNT_DATABASE SingleEntryDataBase;

    PAGED_CODE();

    //
    //  Check if the object has any handle information
    //

    HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO(ObjectHeader);

    if (HandleInfo == NULL) {

        return NULL;
    }

    //
    //  The object does have some handle information.  If it has
    //  a single handle entry then we'll construct a local dummy
    //  handle count database and come up with a new data base for
    //  storing two entries.
    //

    if (ObjectHeader->Flags & OB_FLAG_SINGLE_HANDLE_ENTRY) {

        SingleEntryDataBase.CountEntries = 1;
        SingleEntryDataBase.HandleCountEntries[0] = HandleInfo->SingleEntry;

        OldHandleCountDataBase = &SingleEntryDataBase;

        OldSize = sizeof( SingleEntryDataBase );

        CountEntries = 2;

        NewSize = sizeof(OBJECT_HANDLE_COUNT_DATABASE) +
               ((CountEntries - 1) * sizeof( OBJECT_HANDLE_COUNT_ENTRY ));

    } else {

        //
        //  The object already has multiple handles so we reference the
        //  current handle database, and compute a new size bumped by four
        //

        OldHandleCountDataBase = HandleInfo->HandleCountDataBase;

        CountEntries = OldHandleCountDataBase->CountEntries;

        OldSize = sizeof(OBJECT_HANDLE_COUNT_DATABASE) +
               ((CountEntries - 1) * sizeof( OBJECT_HANDLE_COUNT_ENTRY));

        CountEntries += 4;

        NewSize = sizeof(OBJECT_HANDLE_COUNT_DATABASE) +
               ((CountEntries - 1) * sizeof( OBJECT_HANDLE_COUNT_ENTRY));
    }

    //
    //  Now allocate pool for the new handle database.
    //

    NewHandleCountDataBase = ExAllocatePoolWithTag(PagedPool, NewSize,'dHbO');

    if (NewHandleCountDataBase == NULL) {

        return NULL;
    }

    //
    //  Copy over the old database.  Note that this might just copy our
    //  local dummy entry for the single entry case
    //

    RtlMoveMemory(NewHandleCountDataBase, OldHandleCountDataBase, OldSize);

    //
    //  If the previous mode was a single entry then remove that
    //  indication otherwise free up with previous handle database
    //

    if (ObjectHeader->Flags & OB_FLAG_SINGLE_HANDLE_ENTRY) {

        ObjectHeader->Flags &= ~OB_FLAG_SINGLE_HANDLE_ENTRY;

    } else {

        ExFreePool( OldHandleCountDataBase );
    }

    //
    //  Find the end of the new database that is used and zero out
    //  the memory
    //

    FreeHandleCountEntry =
        (POBJECT_HANDLE_COUNT_ENTRY)((PCHAR)NewHandleCountDataBase + OldSize);

    RtlZeroMemory(FreeHandleCountEntry, NewSize - OldSize);

    //
    //  Set the new database to the proper entry count and put it
    //  all in the object
    //

    NewHandleCountDataBase->CountEntries = CountEntries;

    HandleInfo->HandleCountDataBase = NewHandleCountDataBase;

    //
    //  And return to our caller
    //

    return FreeHandleCountEntry;
}


NTSTATUS
ObpIncrementHandleDataBase (
    IN POBJECT_HEADER ObjectHeader,
    IN PEPROCESS Process,
    OUT PULONG NewProcessHandleCount
    )

/*++

Routine Description:

    This function increments the handle count database associated with the
    specified object for a specified process.

Arguments:

    ObjectHeader - Supplies a pointer to the object.

    Process - Supplies a pointer to the process whose handle count is to be
        updated.

    NewProcessHandleCount - Supplies a pointer to a variable that receives
        the new handle count for the process.

Return Value:

    An appropriate status value

--*/

{
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_DATABASE HandleCountDataBase;
    POBJECT_HANDLE_COUNT_ENTRY HandleCountEntry;
    POBJECT_HANDLE_COUNT_ENTRY FreeHandleCountEntry;
    ULONG CountEntries;
    ULONG ProcessHandleCount;

    PAGED_CODE();

    //
    //  Translate the object header to the handle information.
    //

    HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO(ObjectHeader);

    //
    //  Check if the object has space for only a single handle
    //

    if (ObjectHeader->Flags & OB_FLAG_SINGLE_HANDLE_ENTRY) {

        //
        //  If the single handle isn't in use then set the entry
        //  and tell the caller there is only one handle
        //

        if (HandleInfo->SingleEntry.HandleCount == 0) {

            *NewProcessHandleCount = 1;
            HandleInfo->SingleEntry.HandleCount = 1;
            HandleInfo->SingleEntry.Process = Process;

            return STATUS_SUCCESS;

        //
        //  If the single entry is for the same process as specified
        //  then increment the count and we're done
        //

        } else if (HandleInfo->SingleEntry.Process == Process) {

            *NewProcessHandleCount = ++HandleInfo->SingleEntry.HandleCount;

            return STATUS_SUCCESS;

        //
        //  Finally we have a object with a single handle entry already
        //  in use, so we need to grow the handle database before
        //  we can set this new value
        //

        } else {

            FreeHandleCountEntry = ObpInsertHandleCount( ObjectHeader );

            if (FreeHandleCountEntry == NULL) {

                return STATUS_INSUFFICIENT_RESOURCES;
            }

            FreeHandleCountEntry->Process = Process;
            FreeHandleCountEntry->HandleCount = 1;
            *NewProcessHandleCount = 1;

            return STATUS_SUCCESS;
        }
    }

    //
    //  The object does not contain a single entry, therefore we're
    //  assuming it already has a handle count database
    //

    //
    //  **** HandleInfo should first be checked for null
    //

    HandleCountDataBase = HandleInfo->HandleCountDataBase;

    FreeHandleCountEntry = NULL;

    //
    //  **** can we with success if the database is null
    //

    if (HandleCountDataBase != NULL) {

        //
        //  Get the number of entries and a pointer to the first one
        //  in the handle database
        //

        CountEntries = HandleCountDataBase->CountEntries;
        HandleCountEntry = &HandleCountDataBase->HandleCountEntries[ 0 ];

        //
        //  For each entry in the handle database check for a process
        //  match and if so then increment the handle count and return
        //  to our caller.  Otherwise if the entry is free then remember
        //  it so we can store to it later
        //

        while (CountEntries) {

            if (HandleCountEntry->Process == Process) {

                *NewProcessHandleCount = ++HandleCountEntry->HandleCount;

                return STATUS_SUCCESS;

            } else if (HandleCountEntry->HandleCount == 0) {

                FreeHandleCountEntry = HandleCountEntry;
            }

            ++HandleCountEntry;
            --CountEntries;
        }

        //
        //  If we did not find a free handle entry then we have to grow the
        //  handle database before we can set this new value
        //

        if (FreeHandleCountEntry == NULL) {

            FreeHandleCountEntry = ObpInsertHandleCount( ObjectHeader );

            if (FreeHandleCountEntry == NULL) {

                return(STATUS_INSUFFICIENT_RESOURCES);
            }
        }

        FreeHandleCountEntry->Process = Process;
        FreeHandleCountEntry->HandleCount = 1;
        *NewProcessHandleCount = 1;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
ObpIncrementHandleCount (
    OB_OPEN_REASON OpenReason,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    PACCESS_STATE AccessState OPTIONAL,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    )

/*++

Routine Description:

    Increments the count of number of handles to the given object.

    If the object is being opened or created, access validation and
    auditing will be performed as appropriate.

Arguments:

    OpenReason - Supplies the reason the handle count is being incremented.

    Process - Pointer to the process in which the new handle will reside.

    Object - Supplies a pointer to the body of the object.

    ObjectType - Supplies the type of the object.

    AccessState - Optional parameter supplying the current accumulated
        security information describing the attempt to access the object.

    AccessMode - Supplies the mode of the requestor.

    Attributes - Desired attributes for the handle

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;
    ULONG ProcessHandleCount;
    BOOLEAN ExclusiveHandle;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    POBJECT_HEADER ObjectHeader;
    BOOLEAN HasPrivilege = FALSE;
    PRIVILEGE_SET Privileges;
    BOOLEAN NewObject;
    BOOLEAN HoldObjectTypeMutex = FALSE;

    PAGED_CODE();

    ObpValidateIrql( "ObpIncrementHandleCount" );

    //
    //  Get a pointer to the object header
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    //
    //  Charge the user quota for the object
    //

    Status = ObpChargeQuotaForObject( ObjectHeader, ObjectType, &NewObject );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    ObpEnterObjectTypeMutex( ObjectType );
    HoldObjectTypeMutex = TRUE;

    try {

        ExclusiveHandle = FALSE;

        //
        //  Check if the caller wants exlusive access and if so then
        //  make sure the attributes and header flags match up correctly
        //

        if (Attributes & OBJ_EXCLUSIVE) {

            if ((Attributes & OBJ_INHERIT) ||
                ((ObjectHeader->Flags & OB_FLAG_EXCLUSIVE_OBJECT) == 0)) {

                Status = STATUS_INVALID_PARAMETER;
                leave;
            }

            if (((OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) == NULL) &&
                 (ObjectHeader->HandleCount != 0))

                    ||

                ((OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) != NULL) &&
                 (OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) != PsGetCurrentProcess()))) {

                Status = STATUS_ACCESS_DENIED;
                leave;
            }

            ExclusiveHandle = TRUE;

        //
        //  The user doesn't want exclusive access so now check to make sure
        //  the attriutes and header flags match up correctly
        //

        } else if ((ObjectHeader->Flags & OB_FLAG_EXCLUSIVE_OBJECT) &&
                   (OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) != NULL)) {

            Status = STATUS_ACCESS_DENIED;
            leave;
        }

        //
        //  If handle count going from zero to one for an existing object that
        //  maintains a handle count database, but does not have an open procedure
        //  just a close procedure, then fail the call as they are trying to
        //  reopen an object by pointer and the close procedure will not know
        //  that the object has been 'recreated'
        //

        if ((ObjectHeader->HandleCount == 0) &&
            (!NewObject) &&
            (ObjectType->TypeInfo.MaintainHandleCount) &&
            (ObjectType->TypeInfo.OpenProcedure == NULL) &&
            (ObjectType->TypeInfo.CloseProcedure != NULL)) {

            Status = STATUS_UNSUCCESSFUL;
            leave;
        }

        if ((OpenReason == ObOpenHandle) ||
            ((OpenReason == ObDuplicateHandle) && ARGUMENT_PRESENT(AccessState))) {

            //
            //  Perform Access Validation to see if we can open this
            //  (already existing) object.
            //
            
            if (!ObCheckObjectAccess( Object,
                                      AccessState,
                                      TRUE,
                                      AccessMode,
                                      &Status )) {

                leave;
            }
            
        } else if ((OpenReason == ObCreateHandle)) {

            //
            //  We are creating a new instance of this object type.
            //  A total of three audit messages may be generated:
            //
            //  1 - Audit the attempt to create an instance of this
            //      object type.
            //
            //  2 - Audit the successful creation.
            //
            //  3 - Audit the allocation of the handle.
            //

            //
            //  At this point, the RemainingDesiredAccess field in
            //  the AccessState may still contain either Generic access
            //  types, or MAXIMUM_ALLOWED.  We will map the generics
            //  and substitute GenericAll for MAXIMUM_ALLOWED.
            //

            if ( AccessState->RemainingDesiredAccess & MAXIMUM_ALLOWED ) {

                AccessState->RemainingDesiredAccess &= ~MAXIMUM_ALLOWED;
                AccessState->RemainingDesiredAccess |= GENERIC_ALL;
            }

            if ((GENERIC_ACCESS & AccessState->RemainingDesiredAccess) != 0) {

                RtlMapGenericMask( &AccessState->RemainingDesiredAccess,
                                   &ObjectType->TypeInfo.GenericMapping );
            }

            //
            //  Since we are creating the object, we can give any access the caller
            //  wants.  The only exception is ACCESS_SYSTEM_SECURITY, which requires
            //  a privilege.
            //

            if ( AccessState->RemainingDesiredAccess & ACCESS_SYSTEM_SECURITY ) {

                //
                //  We could use SeSinglePrivilegeCheck here, but it
                //  captures the subject context again, and we don't
                //  want to do that in this path for performance reasons.
                //

                Privileges.PrivilegeCount = 1;
                Privileges.Control = PRIVILEGE_SET_ALL_NECESSARY;
                Privileges.Privilege[0].Luid = SeSecurityPrivilege;
                Privileges.Privilege[0].Attributes = 0;

                HasPrivilege = SePrivilegeCheck( &Privileges,
                                                 &AccessState->SubjectSecurityContext,
                                                 KeGetPreviousMode() );

                if (!HasPrivilege) {

                    SePrivilegedServiceAuditAlarm ( NULL,
                                                    &AccessState->SubjectSecurityContext,
                                                    &Privileges,
                                                    FALSE );

                    Status = STATUS_PRIVILEGE_NOT_HELD;
                    leave;
                }

                AccessState->RemainingDesiredAccess &= ~ACCESS_SYSTEM_SECURITY;
                AccessState->PreviouslyGrantedAccess |= ACCESS_SYSTEM_SECURITY;

                (VOID) SeAppendPrivileges( AccessState,
                                           &Privileges );
            }

            //
            //  Get the objects creator info block and insert it on the
            //  global list of objects for that type
            //

            CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );

            if (CreatorInfo != NULL) {

                InsertTailList( &ObjectType->TypeList, &CreatorInfo->TypeList );
            }
        }

        if (ExclusiveHandle) {

            OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader)->ExclusiveProcess = Process;
        }

        ObpIncrHandleCount( ObjectHeader );
        ProcessHandleCount = 0;

        //
        //  If the object type wants us to keep try of the handle counts
        //  then call our routine to do the work
        //

        if (ObjectType->TypeInfo.MaintainHandleCount) {

            Status = ObpIncrementHandleDataBase( ObjectHeader,
                                                 Process,
                                                 &ProcessHandleCount );

            if (!NT_SUCCESS(Status)) {
                // BUG BUG, We probably need more backout than this, NeillC
                ObpDecrHandleCount( ObjectHeader );

                leave;
            }
        }

        //
        //  Set our preliminary status now to success because
        //  the call to the open procedure might change this
        //

        Status = STATUS_SUCCESS;

        //
        //  If the object type has an open procedure
        //  then invoke that procedure
        //

        if (ObjectType->TypeInfo.OpenProcedure != NULL) {

            KIRQL SaveIrql;

            //
            //  Leave the object type mutex when call the OpenProcedure. If an exception
            //  while OpenProcedure the HoldObjectTypeMutex disable leaving the mutex
            //  on finally block
            //

            ObpLeaveObjectTypeMutex( ObjectType );
            HoldObjectTypeMutex = FALSE;

            ObpBeginTypeSpecificCallOut( SaveIrql );

            try {

                (*ObjectType->TypeInfo.OpenProcedure)( OpenReason,
                                                       Process,
                                                       Object,
                                                       AccessState ?
                                                           AccessState->PreviouslyGrantedAccess :
                                                           0,
                                                       ProcessHandleCount );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                Status = GetExceptionCode();
            }

            ObpEndTypeSpecificCallOut( SaveIrql, "Open", ObjectType, Object );

            //
            //  Hold back the object type mutex and set the HoldObjectTypeMutex variable
            //  to allow releasing the mutex while leaving this procedure
            //

            ObpEnterObjectTypeMutex( ObjectType );
            HoldObjectTypeMutex = TRUE;

            if (!NT_SUCCESS(Status)) {

                (VOID)ObpDecrHandleCount( ObjectHeader );
                leave;
            }
        }

        //
        //  Do some simple bookkeeping for the handle counts
        //  and then return to our caller
        //

        ObjectType->TotalNumberOfHandles += 1;

        if (ObjectType->TotalNumberOfHandles > ObjectType->HighWaterNumberOfHandles) {

            ObjectType->HighWaterNumberOfHandles = ObjectType->TotalNumberOfHandles;
        }

    } finally {

        if ( HoldObjectTypeMutex ) {

            ObpLeaveObjectTypeMutex( ObjectType );
        }
    }

    return( Status );
}


NTSTATUS
ObpIncrementUnnamedHandleCount (
    PACCESS_MASK DesiredAccess,
    PEPROCESS Process,
    PVOID Object,
    POBJECT_TYPE ObjectType,
    KPROCESSOR_MODE AccessMode,
    ULONG Attributes
    )

/*++

Routine Description:

    Increments the count of number of handles to the given object.

Arguments:

    Desired Access - Supplies the desired access to the object and receives
        the assign access mask

    Process - Pointer to the process in which the new handle will reside.

    Object - Supplies a pointer to the body of the object.

    ObjectType - Supplies the type of the object.

    AccessMode - Supplies the mode of the requestor.

    Attributes - Desired attributes for the handle

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;
    BOOLEAN ExclusiveHandle;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    POBJECT_HEADER ObjectHeader;
    BOOLEAN NewObject;
    ULONG ProcessHandleCount;
    BOOLEAN HoldObjectTypeMutex = FALSE;

    PAGED_CODE();

    ObpValidateIrql( "ObpIncrementUnnamedHandleCount" );

    //
    //  Get a pointer to the object header
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );

    //
    //  Charge the user quota for the object
    //

    Status = ObpChargeQuotaForObject( ObjectHeader, ObjectType, &NewObject );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    ObpEnterObjectTypeMutex( ObjectType );
    HoldObjectTypeMutex = TRUE;

    try {

        ExclusiveHandle = FALSE;

        //
        //  Check if the caller wants exlusive access and if so then
        //  make sure the attributes and header flags match up correctly
        //

        if (Attributes & OBJ_EXCLUSIVE) {

            if ((Attributes & OBJ_INHERIT) ||
                ((ObjectHeader->Flags & OB_FLAG_EXCLUSIVE_OBJECT) == 0)) {

                Status = STATUS_INVALID_PARAMETER;
                leave;
            }

            if (((OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) == NULL) &&
                 (ObjectHeader->HandleCount != 0))

                        ||

                ((OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) != NULL) &&
                 (OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) != PsGetCurrentProcess()))) {

                Status = STATUS_ACCESS_DENIED;
                leave;
            }

            ExclusiveHandle = TRUE;

        //
        //  The user doesn't want exclusive access so now check to make sure
        //  the attriutes and header flags match up correctly
        //

        } else if ((ObjectHeader->Flags & OB_FLAG_EXCLUSIVE_OBJECT) &&
                   (OBJECT_HEADER_TO_EXCLUSIVE_PROCESS(ObjectHeader) != NULL)) {

            Status = STATUS_ACCESS_DENIED;
            leave;
        }

        //
        //  If handle count going from zero to one for an existing object that
        //  maintains a handle count database, but does not have an open procedure
        //  just a close procedure, then fail the call as they are trying to
        //  reopen an object by pointer and the close procedure will not know
        //  that the object has been 'recreated'
        //

        if ((ObjectHeader->HandleCount == 0) &&
            (!NewObject) &&
            (ObjectType->TypeInfo.MaintainHandleCount) &&
            (ObjectType->TypeInfo.OpenProcedure == NULL) &&
            (ObjectType->TypeInfo.CloseProcedure != NULL)) {

            Status = STATUS_UNSUCCESSFUL;

            leave;
        }

        //
        //  If the user asked for the maximum allowed then remove the bit and
        //  or in generic all access
        //

        if ( *DesiredAccess & MAXIMUM_ALLOWED ) {

            *DesiredAccess &= ~MAXIMUM_ALLOWED;
            *DesiredAccess |= GENERIC_ALL;
        }

        //  If the user asked for any generic bit then translate it to
        //  someone more appropriate for the object type
        //

        if ((GENERIC_ACCESS & *DesiredAccess) != 0) {

            RtlMapGenericMask( DesiredAccess,
                               &ObjectType->TypeInfo.GenericMapping );
        }

        //
        //  Get a pointer to the creator info block for the object and insert
        //  it on the global list of object for that type
        //

        CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );

        if (CreatorInfo != NULL) {

            InsertTailList( &ObjectType->TypeList, &CreatorInfo->TypeList );
        }

        if (ExclusiveHandle) {

            OBJECT_HEADER_TO_QUOTA_INFO(ObjectHeader)->ExclusiveProcess = Process;
        }

        ObpIncrHandleCount( ObjectHeader );
        ProcessHandleCount = 0;

        //
        //  If the object type wants us to keep try of the handle counts
        //  then call our routine to do the work
        //

        if (ObjectType->TypeInfo.MaintainHandleCount) {

            Status = ObpIncrementHandleDataBase( ObjectHeader,
                                                 Process,
                                                 &ProcessHandleCount );

            if (!NT_SUCCESS(Status)) {
                // BUG BUG, We probably need more backout than this, NeillC
                ObpDecrHandleCount( ObjectHeader );
                leave;
            }
        }

        //
        //  Set our preliminary status now to success because
        //  the call to the open procedure might change this
        //

        Status = STATUS_SUCCESS;

        //
        //  If the object type has an open procedure
        //  then invoke that procedure
        //

        if (ObjectType->TypeInfo.OpenProcedure != NULL) {

            KIRQL SaveIrql;

            //
            //  Leave the object type mutex when call the OpenProcedure. If an exception
            //  while OpenProcedure the HoldObjectTypeMutex disable leaving the mutex
            //  on finally block
            //

            ObpLeaveObjectTypeMutex( ObjectType );
            HoldObjectTypeMutex = FALSE;

            ObpBeginTypeSpecificCallOut( SaveIrql );

            try {

                (*ObjectType->TypeInfo.OpenProcedure)( ObCreateHandle,
                                                       Process,
                                                       Object,
                                                       *DesiredAccess,
                                                       ProcessHandleCount );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                Status = GetExceptionCode();
            }

            ObpEndTypeSpecificCallOut( SaveIrql, "Open", ObjectType, Object );

            //
            //  Hold back the object type mutex and set the HoldObjectTypeMutex variable
            //  to allow releasing the mutex while leaving this procedure
            //

            ObpEnterObjectTypeMutex( ObjectType );
            HoldObjectTypeMutex = TRUE;

            if (!NT_SUCCESS(Status)) {

                (VOID)ObpDecrHandleCount( ObjectHeader );
                leave;
            }
        }

        //
        //  Do some simple bookkeeping for the handle counts
        //  and then return to our caller
        //

        ObjectType->TotalNumberOfHandles += 1;

        if (ObjectType->TotalNumberOfHandles > ObjectType->HighWaterNumberOfHandles) {

            ObjectType->HighWaterNumberOfHandles = ObjectType->TotalNumberOfHandles;
        }

    } finally {

        if ( HoldObjectTypeMutex ) {

            ObpLeaveObjectTypeMutex( ObjectType );
        }
    }

    return( Status );
}


NTSTATUS
ObpChargeQuotaForObject (
    IN POBJECT_HEADER ObjectHeader,
    IN POBJECT_TYPE ObjectType,
    OUT PBOOLEAN NewObject
    )

/*++

Routine Description:

    This routine charges quota against the current process for the new
    object

Arguments:

    ObjectHeader - Supplies a pointer to the new object being charged for

    ObjectType - Supplies the type of the new object

    NewObject - Returns true if the object is really new and false otherwise

Return Value:

    An appropriate status value

--*/

{
    POBJECT_HEADER_QUOTA_INFO QuotaInfo;
    ULONG NonPagedPoolCharge;
    ULONG PagedPoolCharge;

    //
    //  Get a pointer to the quota block for this object
    //

    QuotaInfo = OBJECT_HEADER_TO_QUOTA_INFO( ObjectHeader );

    *NewObject = FALSE;

    //
    //  If the object is new then we have work to do otherwise
    //  we'll return with NewObject set to false
    //

    if (ObjectHeader->Flags & OB_FLAG_NEW_OBJECT) {

        //
        //  Say the object now isn't new
        //

        ObjectHeader->Flags &= ~OB_FLAG_NEW_OBJECT;

        //
        //  If there does exist a quota info structure for this
        //  object then calculate what our charge should be from
        //  the information stored in that structure
        //

        if (QuotaInfo != NULL) {

            PagedPoolCharge = QuotaInfo->PagedPoolCharge +
                              QuotaInfo->SecurityDescriptorCharge;
            NonPagedPoolCharge = QuotaInfo->NonPagedPoolCharge;

        } else {

            //
            //  There isn't any quota information so we're on our own
            //  Paged pool charge is the default for the object plus
            //  the security descriptor if present.  Nonpaged pool charge
            //  is the default for the object.
            //

            PagedPoolCharge = ObjectType->TypeInfo.DefaultPagedPoolCharge;

            if (ObjectHeader->SecurityDescriptor != NULL) {

                ObjectHeader->Flags |= OB_FLAG_DEFAULT_SECURITY_QUOTA;
                PagedPoolCharge += SE_DEFAULT_SECURITY_QUOTA;
            }

            NonPagedPoolCharge = ObjectType->TypeInfo.DefaultNonPagedPoolCharge;
        }

        //
        //  Now charge for the quota and make sure it succeeds
        //

        ObjectHeader->QuotaBlockCharged = (PVOID)PsChargeSharedPoolQuota( PsGetCurrentProcess(),
                                                                          PagedPoolCharge,
                                                                          NonPagedPoolCharge );

        if (ObjectHeader->QuotaBlockCharged == NULL) {

            return STATUS_QUOTA_EXCEEDED;
        }

        *NewObject = TRUE;
    }

    return STATUS_SUCCESS;
}


VOID
ObpDecrementHandleCount (
    PEPROCESS Process,
    POBJECT_HEADER ObjectHeader,
    POBJECT_TYPE ObjectType,
    ACCESS_MASK GrantedAccess
    )

/*++

Routine Description:

    This procedure decrements the handle count for the specified object

Arguments:

    Process - Supplies the process where the handle existed

    ObjectHeader - Supplies a pointer to the object header for the object

    ObjectType - Supplies a type of the object

    GrantedAccess - Supplies the current access mask to the object

Return Value:

    None.

--*/

{
    POBJECT_HEADER_HANDLE_INFO HandleInfo;
    POBJECT_HANDLE_COUNT_DATABASE HandleCountDataBase;
    POBJECT_HANDLE_COUNT_ENTRY HandleCountEntry;
    POBJECT_HEADER_CREATOR_INFO CreatorInfo;
    PVOID Object;
    ULONG CountEntries;
    ULONG ProcessHandleCount;
    ULONG SystemHandleCount;
    BOOLEAN HandleCountIsZero;

    PAGED_CODE();

    ObpEnterObjectTypeMutex( ObjectType );

    Object = (PVOID)&ObjectHeader->Body;

    SystemHandleCount = ObjectHeader->HandleCount;
    ProcessHandleCount = 0;

    //
    //  Decrement the handle count and it was one and it
    //  was an exclusive object then zero out the exclusive
    //  process
    //

    HandleCountIsZero = ObpDecrHandleCount( ObjectHeader );
            
    if ( HandleCountIsZero &&
        (ObjectHeader->Flags & OB_FLAG_EXCLUSIVE_OBJECT)) {

        OBJECT_HEADER_TO_QUOTA_INFO( ObjectHeader )->ExclusiveProcess = NULL;
    }
    
    if ( HandleCountIsZero ) {
    
        CreatorInfo = OBJECT_HEADER_TO_CREATOR_INFO( ObjectHeader );
    
        //
        //  If there is a creator info record and we are on the list
        //  for the object type then remove this object from the list
        //

        if (CreatorInfo != NULL && !IsListEmpty( &CreatorInfo->TypeList )) {
                        
            RemoveEntryList( &CreatorInfo->TypeList );
            
            InitializeListHead( &CreatorInfo->TypeList );
        }
    }

    //
    //  If the object maintains a handle count database then
    //  search through the handle database and decrement
    //  the necessary information
    //

    if (ObjectType->TypeInfo.MaintainHandleCount) {

        HandleInfo = OBJECT_HEADER_TO_HANDLE_INFO( ObjectHeader );

        //
        //  Check if there is a single handle entry, then it better
        //  be ours
        //

        if (ObjectHeader->Flags & OB_FLAG_SINGLE_HANDLE_ENTRY) {

            ASSERT(HandleInfo->SingleEntry.Process == Process);
            ASSERT(HandleInfo->SingleEntry.HandleCount > 0);

            ProcessHandleCount = HandleInfo->SingleEntry.HandleCount--;
            HandleCountEntry = &HandleInfo->SingleEntry;

        } else {

            //
            //  Otherwise search the database for a process match
            //

            HandleCountDataBase = HandleInfo->HandleCountDataBase;

            if (HandleCountDataBase != NULL) {

                CountEntries = HandleCountDataBase->CountEntries;
                HandleCountEntry = &HandleCountDataBase->HandleCountEntries[ 0 ];

                while (CountEntries) {

                    if ((HandleCountEntry->HandleCount != 0) &&
                        (HandleCountEntry->Process == Process)) {

                        ProcessHandleCount = HandleCountEntry->HandleCount--;

                        break;
                    }

                    HandleCountEntry++;
                    CountEntries--;
                }
            }
        }

        //
        //  Now if the process is giving up all handles to the object
        //  then zero out the handle count entry.  For a single handle
        //  entry this is just the single entry in the header handle info
        //  structure
        //

        if (ProcessHandleCount == 1) {

            HandleCountEntry->Process = NULL;
            HandleCountEntry->HandleCount = 0;
        }
    }

    //
    //  If the Object Type has a Close Procedure, then release the type
    //  mutex before calling it, and then call ObpDeleteNameCheck without
    //  the mutex held.
    //

    if (ObjectType->TypeInfo.CloseProcedure) {

        KIRQL SaveIrql;

        ObpLeaveObjectTypeMutex( ObjectType );

        ObpBeginTypeSpecificCallOut( SaveIrql );

        (*ObjectType->TypeInfo.CloseProcedure)( Process,
                                                Object,
                                                GrantedAccess,
                                                ProcessHandleCount,
                                                SystemHandleCount );

        ObpEndTypeSpecificCallOut( SaveIrql, "Close", ObjectType, Object );

        ObpDeleteNameCheck( Object, FALSE );

    } else {

        //
        //  If there is no Close Procedure, then just call ObpDeleteNameCheck
        //  with the mutex held.
        //
        //  The following call will release the type mutex
        //

        ObpDeleteNameCheck( Object, TRUE );
    }

    ObpEnterObjectTypeMutex( ObjectType );
    
    ObjectType->TotalNumberOfHandles -= 1;
        
    ObpLeaveObjectTypeMutex( ObjectType );

    return;
}


NTSTATUS
ObpCreateHandle (
    IN OB_OPEN_REASON OpenReason,
    IN PVOID Object,
    IN POBJECT_TYPE ExpectedObjectType OPTIONAL,
    IN PACCESS_STATE AccessState,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN BOOLEAN DirectoryLocked,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This function creates a new handle to an existing object

Arguments:

    OpenReason - The reason why we are doing this work

    Object - A pointer to the body of the new object

    ExpectedObjectType - Optionally Supplies the object type that
        the caller is expecting

    AccessState - Supplies the access state for the handle requested
        by the caller

    ObjectPointerBias - Optionally supplies a count of addition
        increments we do to the pointer count for the object

    Attributes -  Desired attributes for the handle

    DirectoryLocked - Indicates if the root directory mutex is already held

    AccessMode - Supplies the mode of the requestor.

    ReferencedNewObject - Optionally receives a pointer to the body
        of the new object

    Handle - Receives the new handle value

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PVOID ObjectTable;
    HANDLE_TABLE_ENTRY ObjectTableEntry;
    HANDLE NewHandle;
    ACCESS_MASK DesiredAccess;
    ACCESS_MASK GrantedAccess;
    ULONG BiasCount;
    BOOLEAN AttachedToProcess = FALSE;
    BOOLEAN KernelHandle = FALSE;
    KAPC_STATE ApcState;

    PAGED_CODE();

    ObpValidateIrql( "ObpCreateHandle" );

    //
    //  Merge both the remaining desired access and the currently
    //  granted access states into one mask
    //
    //  **** why is this being done here and then later in the this
    //  same routine.
    //

    DesiredAccess = AccessState->RemainingDesiredAccess |
                    AccessState->PreviouslyGrantedAccess;

    //
    //  Get a pointer to the object header and object type
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectType = ObjectHeader->Type;

    //
    //  If the object type isn't what was expected then
    //  return an error to our caller, but first see if
    //  we should release the directory mutex
    //

    if ((ARGUMENT_PRESENT( ExpectedObjectType )) &&
        (ObjectType != ExpectedObjectType )) {

        if (DirectoryLocked) {

            ObpLeaveRootDirectoryMutex();
        }

        return( STATUS_OBJECT_TYPE_MISMATCH );
    }

    //
    //  Set the first ulong of the object table entry to point
    //  back to the object header
    //

    ObjectTableEntry.Object = ObjectHeader;

    //
    //  Now get a pointer to the object table for either the current process
    //  of the kernel handle table
    //

    if ((Attributes & OBJ_KERNEL_HANDLE) && (AccessMode == KernelMode)) {

        ObjectTable = ObpKernelHandleTable;
        KernelHandle = TRUE;

        //
        //  Go to the system process if we have to
        //

        if (PsGetCurrentProcess() != PsInitialSystemProcess) {
            KeStackAttachProcess (&PsInitialSystemProcess->Pcb, &ApcState);
            AttachedToProcess = TRUE;
        }


    } else {

        ObjectTable = ObpGetObjectTable();
    }

    //
    //  ObpIncrementHandleCount will perform access checking on the
    //  object being opened as appropriate.
    //

    Status = ObpIncrementHandleCount( OpenReason,
                                      PsGetCurrentProcess(),
                                      Object,
                                      ObjectType,
                                      AccessState,
                                      AccessMode,
                                      Attributes );

    if (AccessState->GenerateOnClose) {

        Attributes |= OBJ_AUDIT_OBJECT_CLOSE;
    }

    //
    //  Or in some low order bits into the first ulong of the object
    //  table entry
    //

    ObjectTableEntry.ObAttributes |= (Attributes & OBJ_HANDLE_ATTRIBUTES);

    //
    //  Merge both the remaining desired access and the currently
    //  granted access states into one mask and then compute
    //  the granted access
    //

    DesiredAccess = AccessState->RemainingDesiredAccess |
                    AccessState->PreviouslyGrantedAccess;

    GrantedAccess = DesiredAccess &
                    (ObjectType->TypeInfo.ValidAccessMask | ACCESS_SYSTEM_SECURITY );

    //
    //  Unlock the directory if it is locked and make sure
    //  we've been successful so far
    //

    if (DirectoryLocked) {

        ObpLeaveRootDirectoryMutex();
    }

    if (!NT_SUCCESS( Status )) {

        //
        //  If we are attached to the system process then return
        //  back to our caller
        //

        if (AttachedToProcess) {
            KeUnstackDetachProcess(&ApcState);
            AttachedToProcess = FALSE;
        }

        return( Status );
    }

    //
    //  Bias the pointer count if that is what the caller wanted
    //

    if (ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)ObjectPointerBias )) {

        BiasCount = ObjectPointerBias;

        while (BiasCount--) {

            ObpIncrPointerCount( ObjectHeader );
        }
    }

    //
    //  Set the granted access mask in the object table entry (second ulong)
    //

#if i386 && !FPO

    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

        ObjectTableEntry.GrantedAccessIndex = ObpComputeGrantedAccessIndex( GrantedAccess );
        ObjectTableEntry.CreatorBackTraceIndex = RtlLogStackBackTrace();

    } else {

        ObjectTableEntry.GrantedAccess = GrantedAccess;
    }

#else

    ObjectTableEntry.GrantedAccess = GrantedAccess;

#endif // i386 && !FPO

    //
    //  Add this new object table entry to the object table for the process
    //

    NewHandle = ExCreateHandle( ObjectTable, &ObjectTableEntry );

    //
    //  If we didn't get a handle then cleanup after ourselves and return
    //  the error to our caller
    //

    if (NewHandle == NULL) {

        if (ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)ObjectPointerBias )) {

            BiasCount = ObjectPointerBias;

            while (BiasCount--) {

                ObpDecrPointerCount( ObjectHeader );
            }
        }

        ObpDecrementHandleCount( PsGetCurrentProcess(),
                                 ObjectHeader,
                                 ObjectType,
                                 GrantedAccess );

        //
        //  If we are attached to the system process then return
        //  back to our caller
        //

        if (AttachedToProcess) {
            KeUnstackDetachProcess(&ApcState);
            AttachedToProcess = FALSE;
        }

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  We have a new Ex style handle now make it an ob style handle and also
    //  adjust for the kernel handle by setting the sign bit in the handle
    //  value
    //

    if (KernelHandle) {

        NewHandle = EncodeKernelHandle( NewHandle );
    }

    *Handle = NewHandle;

    //
    //  If requested, generate audit messages to indicate that a new handle
    //  has been allocated.
    //
    //  This is the final security operation in the creation/opening of the
    //  object.
    //

    if ( AccessState->GenerateAudit ) {

        SeAuditHandleCreation( AccessState,
                               *Handle );
    }

    if (OpenReason == ObCreateHandle) {

        PAUX_ACCESS_DATA AuxData = AccessState->AuxData;

        if ( ( AuxData->PrivilegesUsed != NULL) && (AuxData->PrivilegesUsed->PrivilegeCount > 0) ) {

            SePrivilegeObjectAuditAlarm( *Handle,
                                         &AccessState->SubjectSecurityContext,
                                         GrantedAccess,
                                         AuxData->PrivilegesUsed,
                                         TRUE,
                                         KeGetPreviousMode() );
        }
    }

    //
    //  If the caller had a pointer bias and wanted the new reference object
    //  then return that value
    //

    if ((ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)ObjectPointerBias )) &&
        (ARGUMENT_PRESENT( ReferencedNewObject ))) {

        *ReferencedNewObject = Object;
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

    return( STATUS_SUCCESS );
}


NTSTATUS
ObpCreateUnnamedHandle (
    IN PVOID Object,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG ObjectPointerBias OPTIONAL,
    IN ULONG Attributes,
    IN KPROCESSOR_MODE AccessMode,
    OUT PVOID *ReferencedNewObject OPTIONAL,
    OUT PHANDLE Handle
    )

/*++

Routine Description:

    This function creates a new unnamed handle for an existing object

Arguments:

    Object - A pointer to the body of the new object

    DesiredAccess - Supplies the access mask being requsted

    ObjectPointerBias - Optionally supplies a count of addition
        increments we do to the pointer count for the object

    Attributes -  Desired attributes for the handle

    AccessMode - Supplies the mode of the requestor.

    ReferencedNewObject - Optionally receives a pointer to the body
        of the new object

    Handle - Receives the new handle value

Return Value:

    An appropriate status value

--*/

{
    NTSTATUS Status;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    PVOID ObjectTable;
    HANDLE_TABLE_ENTRY ObjectTableEntry;
    HANDLE NewHandle;
    ULONG BiasCount;
    ACCESS_MASK GrantedAccess;
    BOOLEAN AttachedToProcess = FALSE;
    BOOLEAN KernelHandle = FALSE;
    KAPC_STATE ApcState;

    PAGED_CODE();

    ObpValidateIrql( "ObpCreateUnnamedHandle" );

    //
    //  Get the object header and type for the new object
    //

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectType = ObjectHeader->Type;

    //
    //  Set the first ulong of the object table entry to point
    //  to the object header and then or in the low order attribute
    //  bits
    //

    ObjectTableEntry.Object = ObjectHeader;

    ObjectTableEntry.ObAttributes |= (Attributes & OBJ_HANDLE_ATTRIBUTES);

    //
    //  Now get a pointer to the object table for either the current process
    //  of the kernel handle table
    //

    if ((Attributes & OBJ_KERNEL_HANDLE) && (AccessMode == KernelMode)) {

        ObjectTable = ObpKernelHandleTable;
        KernelHandle = TRUE;

        //
        //  Go to the system process if we have to
        //

        if (PsGetCurrentProcess() != PsInitialSystemProcess) {
            KeStackAttachProcess (&PsInitialSystemProcess->Pcb, &ApcState);
            AttachedToProcess = TRUE;
        }

    } else {

        ObjectTable = ObpGetObjectTable();
    }

    //
    //  Increment the handle count, this routine also does the access
    //  check if necessary
    //

    Status = ObpIncrementUnnamedHandleCount( &DesiredAccess,
                                             PsGetCurrentProcess(),
                                             Object,
                                             ObjectType,
                                             AccessMode,
                                             Attributes );


    GrantedAccess = DesiredAccess &
                    (ObjectType->TypeInfo.ValidAccessMask | ACCESS_SYSTEM_SECURITY );

    if (!NT_SUCCESS( Status )) {

        //
        //  If we are attached to the system process then return
        //  back to our caller
        //

        if (AttachedToProcess) {
            KeUnstackDetachProcess(&ApcState);
            AttachedToProcess = FALSE;
        }

        return( Status );
    }

    //
    //  Bias the pointer count if that is what the caller wanted
    //

    if (ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)ObjectPointerBias )) {

        BiasCount = ObjectPointerBias;

        while (BiasCount--) {

            ObpIncrPointerCount( ObjectHeader );
        }
    }

    //
    //  Set the granted access mask in the object table entry (second ulong)
    //

#if i386 && !FPO

    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

        ObjectTableEntry.GrantedAccessIndex = ObpComputeGrantedAccessIndex( GrantedAccess );
        ObjectTableEntry.CreatorBackTraceIndex = RtlLogStackBackTrace();

    } else {

        ObjectTableEntry.GrantedAccess = GrantedAccess;
    }

#else

    ObjectTableEntry.GrantedAccess = GrantedAccess;

#endif // i386 && !FPO

    //
    //  Add this new object table entry to the object table for the process
    //

    NewHandle = ExCreateHandle( ObjectTable, &ObjectTableEntry );

    //
    //  If we didn't get a handle then cleanup after ourselves and return
    //  the error to our caller
    //

    if (NewHandle == NULL) {

        if (ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)ObjectPointerBias )) {

            BiasCount = ObjectPointerBias;

            while (BiasCount--) {

                ObpDecrPointerCount( ObjectHeader );
            }
        }

        ObpDecrementHandleCount( PsGetCurrentProcess(),
                                 ObjectHeader,
                                 ObjectType,
                                 GrantedAccess );

        //
        //  If we are attached to the system process then return
        //  back to our caller
        //

        if (AttachedToProcess) {
            KeUnstackDetachProcess(&ApcState);
            AttachedToProcess = FALSE;
        }

        return( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  We have a new Ex style handle now make it an ob style handle and also
    //  adjust for the kernel handle by setting the sign bit in the handle
    //  value
    //

    if (KernelHandle) {

        NewHandle = EncodeKernelHandle( NewHandle );
    }

    *Handle = NewHandle;

    //
    //  If the caller had a pointer bias and wanted the new reference object
    //  then return that value
    //

    if ((ARGUMENT_PRESENT( (PVOID)(ULONG_PTR)ObjectPointerBias )) &&
        (ARGUMENT_PRESENT( ReferencedNewObject ))) {

        *ReferencedNewObject = Object;
    }

    //
    //  If we are attached to the system process then return
    //  back to our caller
    //

    if (AttachedToProcess) {
        KeUnstackDetachProcess(&ApcState);
        AttachedToProcess = FALSE;
    }

    return( STATUS_SUCCESS );
}


NTSTATUS
ObpValidateDesiredAccess (
    IN ACCESS_MASK DesiredAccess
    )

/*++

Routine Description:

    This routine checks the input desired access mask to see that
    some invalid bits are not set.  The invalid bits are the top
    two reserved bits and the top three standard rights bits.
    See \nt\public\sdk\inc\ntseapi.h for more details.

Arguments:

    DesiredAccess - Supplies the mask being checked

Return Value:

    STATUS_ACCESS_DENIED if one or more of the wrongs bits are set and
    STATUS_SUCCESS otherwise

--*/

{
    if (DesiredAccess & 0x0CE00000) {

        return( STATUS_ACCESS_DENIED );

    } else {

        return( STATUS_SUCCESS );
    }
}


#if i386 && !FPO

//
//  The following three variables are just performance counters
//  for the following two routines
//

ULONG ObpXXX1;
ULONG ObpXXX2;
ULONG ObpXXX3;

USHORT
ObpComputeGrantedAccessIndex (
    ACCESS_MASK GrantedAccess
    )

/*++

Routine Description:

    This routine takes a granted access and returns and index
    back to our cache of granted access masks.

Arguments:

    GrantedAccess - Supplies the access mask being added to the cache

Return Value:

    USHORT - returns an index in the cache for the input granted access

--*/

{
    KIRQL OldIrql;
    ULONG GrantedAccessIndex, n;
    PACCESS_MASK p;

    ObpXXX1 += 1;

    //
    //  Lock the global data structure
    //

    ExAcquireFastLock( &ObpLock, &OldIrql );

    n = ObpCurCachedGrantedAccessIndex;
    p = ObpCachedGrantedAccesses;

    //
    //  For each index in our cache look for a match and if found
    //  then unlock the data structure and return that index
    //

    for (GrantedAccessIndex = 0; GrantedAccessIndex < n; GrantedAccessIndex++, p++ ) {

        ObpXXX2 += 1;

        if (*p == GrantedAccess) {

            ExReleaseFastLock( &ObpLock, OldIrql );
            return (USHORT)GrantedAccessIndex;
        }
    }

    //
    //  We didn't find a match now see if we've maxed out the cache
    //

    if (ObpCurCachedGrantedAccessIndex == ObpMaxCachedGrantedAccessIndex) {

        DbgPrint( "OB: GrantedAccess cache limit hit.\n" );
        DbgBreakPoint();
    }

    //
    //  Set the granted access to the next free slot and increment the
    //  number used in the cache, release the lock, and return the
    //  new index to our caller
    //

    *p = GrantedAccess;
    ObpCurCachedGrantedAccessIndex += 1;

    ExReleaseFastLock( &ObpLock, OldIrql );

    return (USHORT)GrantedAccessIndex;
}

ACCESS_MASK
ObpTranslateGrantedAccessIndex (
    USHORT GrantedAccessIndex
    )

/*++

Routine Description:

    This routine takes as input a cache index and returns
    the corresponding granted access mask

Arguments:

    GrantedAccessIndex - Supplies the cache index to look up

Return Value:

    ACCESS_MASK - Returns the corresponding desired access mask

--*/

{
    KIRQL OldIrql;
    ACCESS_MASK GrantedAccess = (ACCESS_MASK)0;

    ObpXXX3 += 1;

    //
    //  Lock the global data structure
    //

    ExAcquireFastLock( &ObpLock, &OldIrql );

    //
    //  If the input index is within bounds then get the granted
    //  access
    //

    if (GrantedAccessIndex < ObpCurCachedGrantedAccessIndex) {

        GrantedAccess = ObpCachedGrantedAccesses[ GrantedAccessIndex ];
    }

    //
    //  Release the lock and return the granted access to our caller
    //

    ExReleaseFastLock( &ObpLock, OldIrql );

    return GrantedAccess;
}

#endif // i386 && !FPO

