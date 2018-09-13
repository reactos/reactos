/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    obclose.c

Abstract:

    Object close system service

Author:

    Steve Wood (stevewo) 31-Mar-1989

Revision History:

--*/

#include "obp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtMakeTemporaryObject)
#pragma alloc_text(PAGE,ObMakeTemporaryObject)
#endif

//
//  Indicates if auditing is enabled so we have to close down the object
//  audit alarm
//

extern BOOLEAN SepAdtAuditingEnabled;


NTSTATUS
NtClose (
    IN HANDLE Handle
    )

/*++

Routine Description:

    This function is used to close access to the specified handle

Arguments:

    Handle - Supplies the handle being closed

Return Value:

    An appropriate status value

--*/

{
    PHANDLE_TABLE ObjectTable;
    PHANDLE_TABLE_ENTRY ObjectTableEntry;
    PVOID Object;
    ULONG CapturedGrantedAccess;
    ULONG CapturedAttributes;
    POBJECT_HEADER ObjectHeader;
    POBJECT_TYPE ObjectType;
    NTSTATUS Status;
    BOOLEAN AttachedToProcess = FALSE;
    KAPC_STATE ApcState;

    //
    //  Protect ourselves from being interrupted while we hold a handle table
    //  entry lock
    //

    KeEnterCriticalRegion();

    try {

    #if DBG
        KIRQL SaveIrql;
    #endif // DBG

        ObpValidateIrql( "NtClose" );

        ObpBeginTypeSpecificCallOut( SaveIrql );

    #if DBG

        //
        //  On checked builds, check that if the Kernel handle bit is set, then
        //  we're coming from Kernel mode. We should probably fail the call if
        //  bit set && !Kmode
        //

        if ((Handle != NtCurrentThread()) && (Handle != NtCurrentProcess())) {

            ASSERT((Handle < 0 ) ? (KeGetPreviousMode() == KernelMode) : TRUE);
        }

    #endif
        //
        //  For the current process we will grab its handle/object table and
        //  translate the handle to its corresponding table entry.  If the
        //  call is successful it also lock down the handle table.  But first
        //  check for a kernel handle and attach and use that table if so.
        //

        if (IsKernelHandle( Handle, KeGetPreviousMode() ))  {

            Handle = DecodeKernelHandle( Handle );

            ObjectTable = ObpKernelHandleTable;

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

        ObjectTableEntry = ExMapHandleToPointer( ObjectTable,
                                                 Handle );

        //
        //  Check that the specified handle is legitimate otherwise we can
        //  assume the caller just passed in some bogus handle value
        //

        if (ObjectTableEntry != NULL) {

            //
            //  From the object table entry we can grab a pointer to the object
            //  header, get its type and its body
            //

            ObjectHeader = (POBJECT_HEADER)(((ULONG_PTR)(ObjectTableEntry->Object)) & ~OBJ_HANDLE_ATTRIBUTES);
            ObjectType = ObjectHeader->Type;
            Object = &ObjectHeader->Body;

            //
            //  If the object type specifies an okay to close procedure then we
            //  need to invoke that callback.  If the callback doesn't want us to
            //  close handle then unlock the object table and return the error
            //  to our caller
            //

            if (ObjectType->TypeInfo.OkayToCloseProcedure != NULL) {

                if (!(*ObjectType->TypeInfo.OkayToCloseProcedure)( PsGetCurrentProcess(), Object, Handle )) {

                    ObpEndTypeSpecificCallOut( SaveIrql, "NtClose", ObjectType, Object );

                    ExUnlockHandleTableEntry( ObjectTable, ObjectTableEntry );

                    //
                    //  If we are attached to the system process then return
                    //  back to our caller
                    //

                    if (AttachedToProcess) {

                        KeUnstackDetachProcess(&ApcState);
                        AttachedToProcess = FALSE;
                    }

                    Status = STATUS_HANDLE_NOT_CLOSABLE;
                    leave;
                }
            }

            CapturedAttributes = ObjectTableEntry->ObAttributes;

            //
            //  If the previous mode was user and the handle is protected from
            //  being closed, then we'll either raise or return an error depending
            //  on the global flags and debugger port situation.
            //

            if ((CapturedAttributes & OBJ_PROTECT_CLOSE) != 0) {

                if (KeGetPreviousMode() != KernelMode) {

                    ExUnlockHandleTableEntry( ObjectTable, ObjectTableEntry );

                    if ((NtGlobalFlag & FLG_ENABLE_CLOSE_EXCEPTIONS) ||
                        (PsGetCurrentProcess()->DebugPort != NULL)) {

                        //
                        //  If we are attached to the system process then return
                        //  back to our caller
                        //

                        if (AttachedToProcess) {
                            KeUnstackDetachProcess(&ApcState);
                            AttachedToProcess = FALSE;
                        }

                        Status = KeRaiseUserException(STATUS_HANDLE_NOT_CLOSABLE);
                        leave;

                    } else {

                        //
                        //  If we are attached to the system process then return
                        //  back to our caller
                        //

                        if (AttachedToProcess) {
                            KeUnstackDetachProcess(&ApcState);
                            AttachedToProcess = FALSE;
                        }

                        Status = STATUS_HANDLE_NOT_CLOSABLE;
                        leave;
                    }

                } else {

                    if ((!PsIsThreadTerminating(PsGetCurrentThread())) &&
                        (PsGetCurrentProcess()->Peb != NULL)) {

                        ExUnlockHandleTableEntry( ObjectTable, ObjectTableEntry );

    #if DBG
                        //
                        //  bugcheck here on checked builds if kernel mode code is
                        //  closing a protected handle and process is not exiting.
                        //  Ignore if no PEB as this occurs if process is killed
                        //  before really starting.
                        //

                        KeBugCheckEx(INVALID_KERNEL_HANDLE, (ULONG_PTR)Handle, 0, 0, 0);
    #else
                        //
                        //  If we are attached to the system process then return
                        //  back to our caller
                        //

                        if (AttachedToProcess) {
                            KeUnstackDetachProcess(&ApcState);
                            AttachedToProcess = FALSE;
                        }

                        Status = STATUS_HANDLE_NOT_CLOSABLE;
                        leave;
    #endif // DBG

                    }
                }
            }

            //
            //  Get the granted access for the handle
            //

    #if i386 && !FPO

            if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB) {

                CapturedGrantedAccess = ObpTranslateGrantedAccessIndex( ObjectTableEntry->GrantedAccessIndex );

            } else {

                CapturedGrantedAccess = ObjectTableEntry->GrantedAccess;
            }

    #else

            CapturedGrantedAccess = ObjectTableEntry->GrantedAccess;

    #endif // i386 && !FPO

            //
            //  Now remove the handle from the handle table
            //

            ExDestroyHandle( ObjectTable,
                             Handle,
                             ObjectTableEntry );

            //
            //  perform any auditing required
            //

            //
            //  Extract the value of the GenerateOnClose bit stored
            //  after object open auditing is performed.  This value
            //  was stored by a call to ObSetGenerateOnClosed.
            //

            if (CapturedAttributes & OBJ_AUDIT_OBJECT_CLOSE) {

                if ( SepAdtAuditingEnabled ) {

                    SeCloseObjectAuditAlarm( Object,
                                             (HANDLE)((ULONG_PTR)Handle & ~OBJ_HANDLE_TAGBITS),  // Mask off the tagbits defined for OB objects.
                                             TRUE );
                }
            }

            //
            //  Since we took the handle away we need to decrement the objects
            //  handle count, and remove a reference
            //

            ObpDecrementHandleCount( PsGetCurrentProcess(),
                                     ObjectHeader,
                                     ObjectHeader->Type,
                                     CapturedGrantedAccess );

            ObDereferenceObject( Object );

            ObpEndTypeSpecificCallOut( SaveIrql, "NtClose", ObjectType, Object );

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

            Status = STATUS_SUCCESS;
            leave;

        } else {

            //
            //  At this point the input handle did not translate to a valid
            //  object table entry
            //

            ObpEndTypeSpecificCallOut( SaveIrql, "NtClose", ObpTypeObjectType, Handle );

            //
            //  If we are attached to the system process then return
            //  back to our caller
            //

            if (AttachedToProcess) {
                KeUnstackDetachProcess(&ApcState);
                AttachedToProcess = FALSE;
            }

            //
            //  Now if the handle is not null and it does not represent the
            //  current thread or process then if we're user mode we either raise
            //  or return an error
            //

            if ((Handle != NULL) &&
                (Handle != NtCurrentThread()) &&
                (Handle != NtCurrentProcess())) {

                if (KeGetPreviousMode() != KernelMode) {

                    if ((NtGlobalFlag & FLG_ENABLE_CLOSE_EXCEPTIONS) ||
                        (PsGetCurrentProcess()->DebugPort != NULL)) {

                        Status = KeRaiseUserException(STATUS_INVALID_HANDLE);
                        leave;

                    } else {

                        Status = STATUS_INVALID_HANDLE;
                        leave;
                    }

                } else {

                    //
                    //  bugcheck here if kernel debugger is enabled and if kernel mode code is
                    //  closing a bogus handle and process is not exiting.  Ignore
                    //  if no PEB as this occurs if process is killed before
                    //  really starting.
                    //

                    if (( !PsIsThreadTerminating(PsGetCurrentThread())) &&
                        (PsGetCurrentProcess()->Peb != NULL)) {

                        if (KdDebuggerEnabled) {
                            KeBugCheckEx(INVALID_KERNEL_HANDLE, (ULONG_PTR)Handle, 1, 0, 0);
                        }
                    }

                }
            }

            Status = STATUS_INVALID_HANDLE;
            leave;
        }

    } finally {

        KeLeaveCriticalRegion();
    }

    return Status;
}


NTSTATUS
NtMakeTemporaryObject (
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine makes the specified object non permanent.

Arguments:

    Handle - Supplies a handle to the object being modified

Return Value:

    An appropriate status value.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PVOID Object;
    OBJECT_HANDLE_INFORMATION HandleInformation;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe output argument if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle( Handle,
                                        DELETE,
                                        (POBJECT_TYPE)NULL,
                                        PreviousMode,
                                        &Object,
                                        &HandleInformation );
    if (!NT_SUCCESS( Status )) {

        return( Status );
    }

    //
    //  Make the object temporary.  Note that the object should still
    //  have a name and directory entry because its handle count is not
    //  zero
    //

    ObMakeTemporaryObject( Object );

    //
    //  Check if we need to generate a delete object audit/alarm
    //

    if (HandleInformation.HandleAttributes & OBJ_AUDIT_OBJECT_CLOSE) {

        SeDeleteObjectAuditAlarm( Object,
                                  Handle );
    }

    ObDereferenceObject( Object );

    return( Status );
}


VOID
ObMakeTemporaryObject (
    IN PVOID Object
    )

/*++

Routine Description:

    This routine removes the name of the object from its parent
    directory.  The object is only removed if it has a non zero
    handle count and a name.  Otherwise the object is simply
    made non permanent

Arguments:

    Object - Supplies the object being modified

Return Value:

    None.

--*/

{
    POBJECT_HEADER ObjectHeader;

    PAGED_CODE();

    ObjectHeader = OBJECT_TO_OBJECT_HEADER( Object );
    ObjectHeader->Flags &= ~OB_FLAG_PERMANENT_OBJECT;

    ObpDeleteNameCheck( Object, FALSE );

    return;
}

