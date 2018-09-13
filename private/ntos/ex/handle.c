/*++

Copyright (c) 1989-1995  Microsoft Corporation

Module Name:

    handle.c

Abstract:

    This module implements a set of functions for supporting handles.

Author:

    Steve Wood (stevewo) 25-Apr-1989
    David N. Cutler (davec) 17-May-1995 (rewrite)
    Gary Kimura (GaryKi) 9-Dec-1997 (rerewrite)

Revision History:

--*/

#include "exp.h"
#pragma hdrstop


//
//  Local constants and support routines
//

//
//  Define global structures that link all handle tables together except the
//  ones where the user has called RemoveHandleTable
//

ERESOURCE HandleTableListLock;
LIST_ENTRY HandleTableListHead;


//
//  This is the sign bit used to lock handle table entries
//

#define EXHANDLE_TABLE_ENTRY_LOCK_BIT    ((ULONG_PTR)1 << ((sizeof(ULONG_PTR) * 8) - 1))

//
//  This is the delay we willing to do to try and get a handle table
//  entry lock
//

LARGE_INTEGER Ex10Milliseconds = {(ULONG)(-10 * 1000 * 10), -1};

//
//  Local support routines
//

PHANDLE_TABLE
ExpAllocateHandleTable (
    IN PEPROCESS Process OPTIONAL
    );

VOID
ExpFreeHandleTable (
    IN PHANDLE_TABLE HandleTable
    );

PHANDLE_TABLE_ENTRY
ExpAllocateHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    OUT PEXHANDLE Handle
    );

VOID
ExpFreeHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    );

PHANDLE_TABLE_ENTRY
ExpLookupHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExInitializeHandleTablePackage)
#pragma alloc_text(PAGE, ExLockHandleTableShared)
#pragma alloc_text(PAGE, ExLockHandleTableExclusive)
#pragma alloc_text(PAGE, ExUnlockHandleTableShared)
#pragma alloc_text(PAGE, ExUnlockHandleTableExclusive)
#pragma alloc_text(PAGE, ExLockHandleTableEntry)
#pragma alloc_text(PAGE, ExUnlockHandleTableEntry)
#pragma alloc_text(PAGE, ExCreateHandleTable)
#pragma alloc_text(PAGE, ExRemoveHandleTable)
#pragma alloc_text(PAGE, ExDestroyHandleTable)
#pragma alloc_text(PAGE, ExEnumHandleTable)
#pragma alloc_text(PAGE, ExDupHandleTable)
#pragma alloc_text(PAGE, ExSnapShotHandleTables)
#pragma alloc_text(PAGE, ExCreateHandle)
#pragma alloc_text(PAGE, ExDestroyHandle)
#pragma alloc_text(PAGE, ExChangeHandle)
#pragma alloc_text(PAGE, ExMapHandleToPointer)
#pragma alloc_text(PAGE, ExpAllocateHandleTable)
#pragma alloc_text(PAGE, ExpFreeHandleTable)
#pragma alloc_text(PAGE, ExpAllocateHandleTableEntry)
#pragma alloc_text(PAGE, ExpFreeHandleTableEntry)
#pragma alloc_text(PAGE, ExpLookupHandleTableEntry)
#endif


NTKERNELAPI
VOID
ExLockHandleTableShared (
    PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This routine locks the specified handle table for shared access.  Once
    locked new entries cannot be added to deleted from the handle table.

Arguments:

    HandleTable - Supplies the handle table being locked.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    (VOID)ExAcquireResourceShared( &HandleTable->HandleTableLock, TRUE );

    return;
}


NTKERNELAPI
VOID
ExLockHandleTableExclusive (
    PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This routine locks the specified handle table for exclusive access.  Once
    locked the onwer can add or delete entries from the handle table.

Arguments:

    HandleTable - Supplies the handle table being locked.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    (VOID)ExAcquireResourceExclusive( &HandleTable->HandleTableLock, TRUE );

    return;
}


NTKERNELAPI
VOID
ExUnlockHandleTableShared (
    PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This routine unlocks the specified handle table from shared access.

Arguments:

    HandleTable - Supplies the handle table being unlocked.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    (VOID)ExReleaseResource( &HandleTable->HandleTableLock );

    return;
}


NTKERNELAPI
VOID
ExUnlockHandleTableExclusive (
    PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This routine unlocks the specified handle table from exclusive access.

Arguments:

    HandleTable - Supplies the handle table being unlocked.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    (VOID)ExReleaseResource( &HandleTable->HandleTableLock );

    return;
}


NTKERNELAPI
BOOLEAN
ExLockHandleTableEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    )

/*++

Routine Description:

    This routine locks the specified handle table entry.  After the entry is
    locked the sign bit will be set.

Arguments:

    HandleTable - Supplies the handle table containing the entry being locked.

    HandleTableEntry - Supplies the handle table entry being locked.

Return Value:

    TRUE if the entry is valid and locked, and FALSE if the entry is
    marked free.

--*/

{
    LONG_PTR NewValue;
    LONG_PTR CurrentValue;
    ULONG LoopCount = 0;

    PAGED_CODE();

    //
    //  We'll keep on looping reading in the value, making sure it is not null,
    //  and if it is not currently locked we'll try for the lock and return
    //  true if we get it.  Otherwise we'll pause a bit and then try again.
    //

    while (TRUE) {

        NewValue =
        CurrentValue = *((volatile LONG_PTR *)&HandleTableEntry->Object);

        //
        //  Make sure the handle table entry is not freed
        //

        if (CurrentValue == 0) {

            return FALSE;
        }

        //
        //  If the handle value is greater than zero then it is not currently
        //  locked and we should try for the lock, but setting the lock bit and
        //  doing an interlocked exchange.
        //

        if (CurrentValue > 0) {

            NewValue |= EXHANDLE_TABLE_ENTRY_LOCK_BIT;

            if ((LONG_PTR)(InterlockedCompareExchangePointer( &HandleTableEntry->Object,
                                                             (PVOID)NewValue,
                                                             (PVOID)CurrentValue )) == CurrentValue) {

                return TRUE;
            }
        }

        //
        //  Either someone already has the entry locked or the value changed
        //  somehow, either way we'll wait a bit and try again.  The first
        //  time we'll wait by spinning for 10us and if that doesn't work
        //  then we'll wait for the handle contention event to go off or
        //  for 10ms, whichever comes first.
        //

        if (LoopCount++ < 1) {

            KeStallExecutionProcessor( 10 );

        } else {

            KeWaitForSingleObject( &HandleTable->HandleContentionEvent,
                                   Executive,
                                   KernelMode,
                                   FALSE,
                                   &Ex10Milliseconds );
        }
    }
}


NTKERNELAPI
VOID
ExUnlockHandleTableEntry (
    PHANDLE_TABLE HandleTable,
    PHANDLE_TABLE_ENTRY HandleTableEntry
    )

/*++

Routine Description:

    This routine unlocks the specified handle table entry.  After the entry is
    unlocked the sign bit will be clear.

Arguments:

    HandleTable - Supplies the handle table containing the entry being unlocked.

    HandleTableEntry - Supplies the handle table entry being unlocked.

Return Value:

    None.

--*/

{
    LONG_PTR NewValue;
    LONG_PTR CurrentValue;

    PAGED_CODE();

    //
    //  This routine does not need to loop and attempt the unlock opeation more
    //  than once because by defintion the caller has the entry already locked
    //  and no one can be changing the value without the lock.
    //
    //  So we'll read in the current value, check that the entry is really
    //  locked, clear the lock bit and make sure the compare exchange worked.
    //

    NewValue = CurrentValue = *((volatile LONG_PTR *)&HandleTableEntry->Object);

    if (CurrentValue >= 0) {

        KeBugCheckEx( BAD_EXHANDLE, __LINE__, (LONG_PTR)HandleTableEntry, NewValue, CurrentValue );
    }

    NewValue &= ~EXHANDLE_TABLE_ENTRY_LOCK_BIT;

    if ((LONG_PTR)(InterlockedCompareExchangePointer( &HandleTableEntry->Object,
                                                     (PVOID)NewValue,
                                                     (PVOID)CurrentValue )) != CurrentValue) {

        KeBugCheckEx( BAD_EXHANDLE, __LINE__, (LONG_PTR)HandleTableEntry, NewValue, CurrentValue );
    }

    //
    //  Now that we've unlocked the event we'll see if there are any waiters
    //  for handle table entries in this table.  If there are any waiters
    //  we'll wake them up and let them try for their lock again.
    //
    //  Note that this will wake up all the waiters for all locks in a given
    //  handle table.   Each waiter also has a short time out so the worse
    //  that can happen is that we might wake someone too early.  It can also
    //  be that no one is waiting for this exact lock, or the one waiting for
    //  it has already gone off, but these are all benign.
    //
    //  **** Note that we're testing for a non empty event header wait list
    //       head.  This is sort of behind the back and a better design
    //       would get a macro from ke.h to do the test, but this will
    //       suffice for now.  This is actually an unsafe test in that it
    //       might return the wrong answer, but even then we are willing to
    //       live with it because any waiters will wake up in 10ms anyway and
    //       any extra call to pulse the event without waiters is benign.
    //

    if (!IsListEmpty( &HandleTable->HandleContentionEvent.Header.WaitListHead )) {

        KePulseEvent( &HandleTable->HandleContentionEvent, EVENT_INCREMENT, FALSE );
    }

    return;
}


NTKERNELAPI
VOID
ExInitializeHandleTablePackage (
    VOID
    )

/*++

Routine Description:

    This routine is called once at system initialization to setup the ex handle
    table package

Arguments:

    None.

Return Value:

    None.

--*/

{
    //
    //  Initialize the handle table synchronization resource and list head
    //

    InitializeListHead( &HandleTableListHead );
    ExInitializeResource( &HandleTableListLock );

    return;
}


NTKERNELAPI
PHANDLE_TABLE
ExCreateHandleTable (
    IN struct _EPROCESS *Process OPTIONAL
    )

/*++

Routine Description:

    This function allocate and initialize a new new handle table

Arguments:

    Process - Supplies an optional pointer to the process against which quota
        will be charged.

Return Value:

    If a handle table is successfully created, then the address of the
    handle table is returned as the function value. Otherwize, a value
    NULL is returned.

--*/

{
    PHANDLE_TABLE HandleTable;

    PAGED_CODE();

    //
    //  Allocate and initialize a handle table descriptor
    //

    HandleTable = ExpAllocateHandleTable( Process );

    //
    //  And return to our caller
    //

    return HandleTable;
}


NTKERNELAPI
VOID
ExRemoveHandleTable (
    IN PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This function removes the specified exhandle table from the list of
    exhandle tables.  Used by PS and ATOM packages to make sure their handle
    tables are not in the list enumerated by the ExSnapShotHandleTables
    routine and the !handle debugger extension.

Arguments:

    HandleTable - Supplies a pointer to a handle table

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  First, acquire the global handle table lock
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive( &HandleTableListLock, TRUE );

    //
    //  Remove the handle table from the handle table list.  This routine is
    //  written so that multiple calls to remove a handle table will not
    //  corrupt the system.
    //

    if (!IsListEmpty( &HandleTable->HandleTableList )) {

        RemoveEntryList( &HandleTable->HandleTableList );

        InitializeListHead( &HandleTable->HandleTableList );
    }

    //
    //  Now release the global lock and return to our caller
    //

    ExReleaseResource( &HandleTableListLock );
    KeLeaveCriticalRegion();

    return;
}


NTKERNELAPI
VOID
ExDestroyHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_DESTROY_HANDLE_ROUTINE DestroyHandleProcedure OPTIONAL
    )

/*++

Routine Description:

    This function destroys the specified handle table.

Arguments:

    HandleTable - Supplies a pointer to a handle table

    DestroyHandleProcedure - Supplies a pointer to a function to call for each
        valid handle entry in the handle table.

Return Value:

    None.

--*/

{
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    //
    //  Remove the handle table from the handle table list
    //

    ExRemoveHandleTable( HandleTable );

    //
    //  Iterate through the handle table and for each handle that is allocated
    //  we'll invoke the call back.  Note that this loop exits when we get a
    //  null handle table entry.  We know there will be no more possible
    //  entries after the first null one is encountered because we allocate
    //  memory of the handles in a dense fashion.  But first test that we have
    //  call back to use
    //

    if (ARGUMENT_PRESENT(DestroyHandleProcedure)) {

        for (Handle.GenericHandleOverlay = NULL; // does essentially the following "Handle.Index = 0, Handle.TagBits = 0;"
             (HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, Handle )) != NULL;
             Handle.Index += 1) {

            //
            //  Only do the callback if the entry is not free
            //

            if (HandleTableEntry->Object != NULL) {

                (*DestroyHandleProcedure)( Handle.GenericHandleOverlay );
            }
        }
    }

    //
    //  Now free up the handle table memory and return to our caller
    //

    ExpFreeHandleTable( HandleTable );

    return;
}


NTKERNELAPI
BOOLEAN
ExEnumHandleTable (
    IN PHANDLE_TABLE HandleTable,
    IN EX_ENUMERATE_HANDLE_ROUTINE EnumHandleProcedure,
    IN PVOID EnumParameter,
    OUT PHANDLE Handle OPTIONAL
    )

/*++

Routine Description:

    This function enumerates all the valid handles in a handle table.
    For each valid handle in the handle table, the specified eumeration
    function is called. If the enumeration function returns TRUE, then
    the enumeration is stopped, the current handle is returned to the
    caller via the optional Handle parameter, and this function returns
    TRUE to indicated that the enumeration stopped at a specific handle.

Arguments:

    HandleTable - Supplies a pointer to a handle table.

    EnumHandleProcedure - Supplies a pointer to a fucntion to call for
        each valid handle in the enumerated handle table.

    EnumParameter - Supplies an uninterpreted 32-bit value that is passed
        to the EnumHandleProcedure each time it is called.

    Handle - Supplies an optional pointer a variable that receives the
        Handle value that the enumeration stopped at. Contents of the
        variable only valid if this function returns TRUE.

Return Value:

    If the enumeration stopped at a specific handle, then a value of TRUE
    is returned. Otherwise, a value of FALSE is returned.

--*/

{
    BOOLEAN ResultValue;
    EXHANDLE LocalHandle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    //
    //  First lock the handle table shared to stop anyone from creating or
    //  destroying new handles.  We need to do this because out iteration
    //  function doesn't want the handles to be closed while we're doing
    //  our work
    //

    KeEnterCriticalRegion();
    ExLockHandleTableShared( HandleTable );

    try {

        //
        //  Our initial return value is false until the enumeration callback
        //  function tells us otherwise
        //

        ResultValue = FALSE;

        //
        //  Iterate through the handle table and for each handle that is
        //  allocated we'll invoke the call back.  Note that this loop exits
        //  when we get a null handle table entry.  We know there will be no
        //  more possible entries after the first null one is encountered
        //  because we allocate memory of the handles in a dense fashion
        //

        for (LocalHandle.GenericHandleOverlay = NULL; // does essentially the following "LocalHandle.Index = 0, LocalHandle.TagBits = 0;"
             (HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, LocalHandle )) != NULL;
             LocalHandle.Index += 1) {

            //
            //  Only do the callback if the entry is not free
            //

            if (HandleTableEntry->Object != NULL) {

                //
                //  Lock the handle table entry because we're about to give
                //  it to the callback function, then release the entry
                //  right after the call back.
                //

                if (ExLockHandleTableEntry( HandleTable, HandleTableEntry )) {

                    try {

                        //
                        //  Invoke the callback, and if it returns true then set
                        //  the proper output values and break out of the loop.
                        //

                        if ((*EnumHandleProcedure)( HandleTableEntry,
                                                    LocalHandle.GenericHandleOverlay,
                                                    EnumParameter )) {

                            if (ARGUMENT_PRESENT( Handle )) {

                                *Handle = LocalHandle.GenericHandleOverlay;
                            }

                            ResultValue = TRUE;
                            break;
                        }

                    } finally {

                        ExUnlockHandleTableEntry( HandleTable, HandleTableEntry );
                    }
                }
            }
        }

    } finally {

        ExUnlockHandleTableShared( HandleTable );
        KeLeaveCriticalRegion();
    }

    return ResultValue;
}


NTKERNELAPI
PHANDLE_TABLE
ExDupHandleTable (
    IN struct _EPROCESS *Process OPTIONAL,
    IN PHANDLE_TABLE OldHandleTable,
    IN EX_DUPLICATE_HANDLE_ROUTINE DupHandleProcedure OPTIONAL
    )

/*++

Routine Description:

    This function creates a duplicate copy of the specified handle table.

Arguments:

    Process - Supplies an optional to the process to charge quota to.

    OldHandleTable - Supplies a pointer to a handle table.

    DupHandleProcedure - Supplies an optional pointer to a function to call
        for each valid handle in the duplicated handle table.

Return Value:

    If the specified handle table is successfully duplicated, then the
    address of the new handle table is returned as the function value.
    Otherwize, a value NULL is returned.

--*/

{
    PHANDLE_TABLE NewHandleTable;

    PHANDLE_TABLE_ENTRY AdditionalFreeEntries;

    EXHANDLE Handle;

    PHANDLE_TABLE_ENTRY OldHandleTableEntry;
    PHANDLE_TABLE_ENTRY NewHandleTableEntry;

    PAGED_CODE();

    //
    //  First allocate a new handle table.  If this fails then
    //  return immediately to our caller
    //

    NewHandleTable = ExpAllocateHandleTable( Process );

    if (NewHandleTable == NULL) {

        return NULL;
    }

    //
    //  Now lock down the old handle table.  We will release it
    //  right after enumerating through the table
    //

    KeEnterCriticalRegion();
    ExLockHandleTableShared( OldHandleTable );

    AdditionalFreeEntries = NULL;

    try {

        //
        //  Now we'll build up the new handle table. We do this by calling
        //  allocating new handle table entries, and "fooling" the worker
        //  routine to allocate keep on allocating until the next free
        //  index needing pool are equal
        //

        while (NewHandleTable->NextIndexNeedingPool < OldHandleTable->NextIndexNeedingPool) {

            //
            //  If we set the first free table entry to -1 then the worker
            //  routine will allocate another buffer
            //

            NewHandleTable->FirstFreeTableEntry = -1;

            //
            //  Call the worker routine to grow the new handle table.  If
            //  not successful then free the new table as far as we got,
            //  set out output variable and exit out here
            //

            if (ExpAllocateHandleTableEntry( NewHandleTable, &Handle ) == NULL) {

                ExpFreeHandleTable( NewHandleTable );

                NewHandleTable = NULL;

                leave;
            }
        }

        //
        //  Now modify the new handle table to think it has zero handles
        //  and set its free list to start on the same index as the old
        //  free list
        //

        NewHandleTable->HandleCount = 0;
        NewHandleTable->FirstFreeTableEntry = OldHandleTable->FirstFreeTableEntry;

        //
        //  Now for every valid index value we'll copy over the old entry into
        //  the new entry
        //

        for (Handle.GenericHandleOverlay = NULL; // does essentially the following "Handle.Index = 0, Handle.TagBits = 0;"
             (OldHandleTableEntry = ExpLookupHandleTableEntry( OldHandleTable, Handle )) != NULL;
             Handle.Index += 1) {

            //
            //  The conditinal in the loop gives up the old handle table
            //  entry, by definition there must exist a corresponding new
            //  handle table entry, so now look it up
            //

            NewHandleTableEntry = ExpLookupHandleTableEntry( NewHandleTable,
                                                             Handle );

            //
            //  If the old entry is free then simply copy over the entire
            //  old entry to the new entry.  The lock command will tell us
            //  it entry is free.
            //

            if (!ExLockHandleTableEntry( OldHandleTable, OldHandleTableEntry )) {

                *NewHandleTableEntry = *OldHandleTableEntry;

            } else {

                //
                //  Otherwise we have a non empty entry.  So now copy it
                //  over, and unlock the old entry.  In both cases we bump
                //  the handle count because either the entry is going into
                //  the new table or we're going to remove it with Exp Free
                //  Handle Table Entry which will decrement the handle count
                //

                *NewHandleTableEntry = *OldHandleTableEntry;
                NewHandleTable->HandleCount += 1;

                ExUnlockHandleTableEntry( OldHandleTable, OldHandleTableEntry );

                //
                //  Invoke the callback and if it returns true then we
                //  unlock the new entry
                //

                if ((*DupHandleProcedure)( Process, NewHandleTableEntry )) {

                    ExUnlockHandleTableEntry( NewHandleTable, NewHandleTableEntry );

                } else {

                    //
                    //  Otherwise this entry is going to be freed in the
                    //  new handle table.  So add it to the stack list of
                    //  additional freed entries
                    //

                    NewHandleTableEntry->Object = AdditionalFreeEntries;
                    NewHandleTableEntry->NextFreeTableEntry = Handle.Index;

                    AdditionalFreeEntries = NewHandleTableEntry;
                }
            }
        }

    } finally {

        ExUnlockHandleTableShared( OldHandleTable );
        KeLeaveCriticalRegion();
    }

    //
    //  At this point we are through with the old handle table,
    //  and if present the new handle table is done with except
    //  for adding back the newly freed entrires.  The only time
    //  the additionalFreeEntries variable will not be null is if
    //  we successfully built the new table and the dup routine
    //  came back false.
    //
    //  While there are additional entries to add to the free list
    //  pop the entry off the stack and add it to the table
    //

    Handle.GenericHandleOverlay = NULL;
    while (AdditionalFreeEntries != NULL) {

        PVOID Next;

        Next = AdditionalFreeEntries->Object;
        Handle.Index = AdditionalFreeEntries->NextFreeTableEntry;

        AdditionalFreeEntries->Object = NULL;

        ExpFreeHandleTableEntry( NewHandleTable,
                                 Handle,
                                 AdditionalFreeEntries );

        AdditionalFreeEntries = Next;
    }

    //
    //  lastly return the new handle table to our caller
    //

    return NewHandleTable;
}


NTKERNELAPI
NTSTATUS
ExSnapShotHandleTables (
    IN PEX_SNAPSHOT_HANDLE_ENTRY SnapShotHandleEntry,
    IN OUT PSYSTEM_HANDLE_INFORMATION HandleInformation,
    IN ULONG Length,
    IN OUT PULONG RequiredLength
    )

/*++

Routine Description:

    This function visits and invokes the specified callback for every valid
    handle that it can find off of the handle table.

Arguments:

    SnapShotHandleEntry - Supplies a pointer to a function to call for
        each valid handle we encounter.

    HandleInformation - Supplies a handle information structure to
        be filled in for each handle table we encounter.  This routine
        fills in the handle count, but relies on a callback to fill in
        entry info fields.

    Length - Supplies a parameter for the callback.  In reality this is
        the total size, in bytes, of the Handle Information buffer.

    RequiredLength - Supplies a parameter for the callback.  In reality
        this is a final size in bytes used to store the requested
        information.

Return Value:

    The last return status of the callback

--*/

{
    NTSTATUS Status;
    PSYSTEM_HANDLE_TABLE_ENTRY_INFO HandleEntryInfo;
    PLIST_ENTRY NextEntry;
    PHANDLE_TABLE HandleTable;
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    //
    //  Lock the handle table list exclusive and traverse the list of handle
    //  tables.
    //

    Status = STATUS_SUCCESS;

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive( &HandleTableListLock, TRUE );

    try {

        //
        //  Setup the output buffer pointer that the callback will maintain
        //

        HandleEntryInfo = &HandleInformation->Handles[0];

        //
        //  Zero out the handle count
        //

        HandleInformation->NumberOfHandles = 0;

        //
        //  Iterate through all the handle tables in the system.
        //

        for (NextEntry = HandleTableListHead.Flink;
             NextEntry != &HandleTableListHead;
             NextEntry = NextEntry->Flink) {

            //
            //  Get the address of the next handle table, lock the handle
            //  table exclusive, and scan the list of handle entries.
            //

            HandleTable = CONTAINING_RECORD( NextEntry,
                                             HANDLE_TABLE,
                                             HandleTableList );

            ExLockHandleTableExclusive( HandleTable );

            try {

                //  Iterate through the handle table and for each handle that
                //  is allocated we'll invoke the call back.  Note that this
                //  loop exits when we get a null handle table entry.  We know
                //  there will be no more possible entries after the first null
                //  one is encountered because we allocate memory of the
                //  handles in a dense fashion
                //

                for (Handle.Index = 0, Handle.TagBits = 0;
                     (HandleTableEntry = ExpLookupHandleTableEntry( HandleTable, Handle )) != NULL;
                     Handle.Index += 1) {

                    //
                    //  Only do the callback if the entry is not free
                    //

                    if (HandleTableEntry->Object != NULL) {

                        //
                        //  Increment the handle count information in the
                        //  information buffer
                        //

                        HandleInformation->NumberOfHandles += 1;

                        //
                        //  Lock the handle table entry because we're about to
                        //  give it to the callback function, then release the
                        //  entry right after the call back.
                        //

                        if (ExLockHandleTableEntry( HandleTable, HandleTableEntry )) {

                            try {

                                Status = (*SnapShotHandleEntry)( &HandleEntryInfo,
                                                                 HandleTable->UniqueProcessId,
                                                                 HandleTableEntry,
                                                                 Handle.GenericHandleOverlay,
                                                                 Length,
                                                                 RequiredLength );

                            } finally {

                                ExUnlockHandleTableEntry( HandleTable, HandleTableEntry );
                            }
                        }
                    }
                }

            } finally {

                ExUnlockHandleTableExclusive( HandleTable );
            }
        }

    } finally {

        ExReleaseResource( &HandleTableListLock );
        KeLeaveCriticalRegion();
    }

    return Status;
}


NTKERNELAPI
HANDLE
ExCreateHandle (
    IN PHANDLE_TABLE HandleTable,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    )

/*++

Routine Description:

    This function creates a handle entry in the specified handle table and
    returns a handle for the entry.

Arguments:

    HandleTable - Supplies a pointer to a handle table

    HandleEntry - Supplies a poiner to the handle entry for which a
        handle entry is created.

Return Value:

    If the handle entry is successfully created, then value of the created
    handle is returned as the function value.  Otherwise, a value of zero is
    returned.

--*/

{
    EXHANDLE Handle;
    PHANDLE_TABLE_ENTRY NewHandleTableEntry;

    PAGED_CODE();

    //
    //  Set out output variable to zero (i.e., null) before going on
    //

    //
    // Clears Handle.Index and Handle.TagBits
    //

    Handle.GenericHandleOverlay = NULL;

    //
    //  Lock the handle table for exclusive access
    //

    KeEnterCriticalRegion();
    ExLockHandleTableExclusive( HandleTable );

    try {

        //
        //  Allocate a new handle table entry, and get the handle value
        //

        NewHandleTableEntry = ExpAllocateHandleTableEntry( HandleTable,
                                                           &Handle );

        //
        //  If we really got a handle then copy over the template and unlock
        //  the entry
        //

        if (NewHandleTableEntry != NULL) {

            *NewHandleTableEntry = *HandleTableEntry;

            ExUnlockHandleTableEntry( HandleTable, NewHandleTableEntry );
        }

    } finally {

        ExUnlockHandleTableExclusive( HandleTable );
        KeLeaveCriticalRegion();
    }

    return Handle.GenericHandleOverlay;
}


NTKERNELAPI
BOOLEAN
ExDestroyHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry OPTIONAL
    )

/*++

Routine Description:

    This function removes a handle from a handle table.

Arguments:

    HandleTable - Supplies a pointer to a handle table

    Handle - Supplies the handle value of the entry to remove.

    HandleTableEntry - Optionally supplies a pointer to the handle
        table entry being destroyed.  If supplied the entry is
        assume to be locked.

Return Value:

    If the specified handle is successfully removed, then a value of
    TRUE is returned. Otherwise, a value of FALSE is returned.

--*/

{
    EXHANDLE LocalHandle;

    PAGED_CODE();

    LocalHandle.GenericHandleOverlay = Handle;

    //
    //  If the caller did not supply the optional handle table entry then
    //  locate the entry via the supplied handle, make sure it is real, and
    //  then lock the entry.
    //

    if (HandleTableEntry == NULL) {

        HandleTableEntry = ExpLookupHandleTableEntry( HandleTable,
                                                      LocalHandle );

        if (HandleTableEntry == NULL) {

            return FALSE;
        }

        if (!ExLockHandleTableEntry( HandleTable, HandleTableEntry )) {

            return FALSE;
        }
    }

    //
    //  At this point we have a locked handle table entry.  Now mark it free
    //  which does the implicit unlock.  The system will not allocate it
    //  again until we add it to the free list which we will do right after
    //  we take out the lock
    //

    HandleTableEntry->Object = NULL;

    KeEnterCriticalRegion();
    ExLockHandleTableExclusive( HandleTable );

    try {

        ExpFreeHandleTableEntry( HandleTable,
                                 LocalHandle,
                                 HandleTableEntry );

    } finally {

        ExUnlockHandleTableExclusive( HandleTable );
        KeLeaveCriticalRegion();
    }

    return TRUE;
}


NTKERNELAPI
BOOLEAN
ExChangeHandle (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle,
    IN PEX_CHANGE_HANDLE_ROUTINE ChangeRoutine,
    IN ULONG_PTR Parameter
    )

/*++

Routine Description:

    This function provides the capability to change the contents of the
    handle entry corrsponding to the specified handle.

Arguments:

    HandleTable - Supplies a pointer to a handle table.

    Handle - Supplies the handle for the handle entry that is changed.

    ChangeRoutine - Supplies a pointer to a function that is called to
        perform the change.

    Parameter - Supplies an uninterpreted parameter that is passed to
        the change routine.

Return Value:

    If the operation was successfully performed, then a value of TRUE
    is returned. Otherwise, a value of FALSE is returned.

--*/

{
    EXHANDLE LocalHandle;

    PHANDLE_TABLE_ENTRY HandleTableEntry;
    BOOLEAN ReturnValue;

    PAGED_CODE();

    LocalHandle.GenericHandleOverlay = Handle;

    //
    //  Translate the input handle to a handle table entry and make
    //  sure it is a valid handle.
    //

    HandleTableEntry = ExpLookupHandleTableEntry( HandleTable,
                                                  LocalHandle );

    if (HandleTableEntry == NULL) {

        return FALSE;
    }

    //
    //  Try and lock the handle table entry,  If this fails then that's
    //  because someone freed the handle
    //

    if (!ExLockHandleTableEntry( HandleTable, HandleTableEntry )) {

        return FALSE;
    }

    //
    //  Make sure we can't get suspended and then invoke the callback
    //

    KeEnterCriticalRegion();

    try {

        ReturnValue = (*ChangeRoutine)( HandleTableEntry, Parameter );

    } finally {

        ExUnlockHandleTableEntry( HandleTable, HandleTableEntry );
        KeLeaveCriticalRegion();
    }

    return ReturnValue;
}


NTKERNELAPI
PHANDLE_TABLE_ENTRY
ExMapHandleToPointer (
    IN PHANDLE_TABLE HandleTable,
    IN HANDLE Handle
    )

/*++

Routine Description:

    This function maps a handle to a pointer to a handle table entry. If the
    map operation is successful then the handle table entry is locked when
    we return.

Arguments:

    HandleTable - Supplies a pointer to a handle table.

    Handle - Supplies the handle to be mapped to a handle entry.

Return Value:

    If the handle was successfully mapped to a pointer to a handle entry,
    then the address of the handle table entry is returned as the function
    value with the entry locked. Otherwise, a value of NULL is returned.

--*/

{
    EXHANDLE LocalHandle;

    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    LocalHandle.GenericHandleOverlay = Handle;

    //
    //  Translate the input handle to a handle table entry and make
    //  sure it is a valid handle.
    //

    HandleTableEntry = ExpLookupHandleTableEntry( HandleTable,
                                                  LocalHandle );

    if (HandleTableEntry == NULL) {

        return NULL;
    }

    //
    //  Try and lock the handle table entry,  If this fails then that's
    //  because someone freed the handle
    //

    if (!ExLockHandleTableEntry( HandleTable, HandleTableEntry )) {

        return NULL;
    }

    //
    //  Return the locked valid handle table entry
    //

    return HandleTableEntry;
}


//
//  Local Support Routine
//

PHANDLE_TABLE
ExpAllocateHandleTable (
    IN PEPROCESS Process OPTIONAL
    )

/*++

Routine Description:

    This worker routine will allocate and initialize a new handle table
    structure.  The new structure consists of the basic handle table
    struct plus the first allocation needed to store handles.  This is
    really one page divided up into the top level node, the first mid
    level node, and one bottom level node.

Arguments:

    Process - Optionally supplies the process to charge quota for the
        handle table

Return Value:

    A pointer to the new handle table or NULL if unsuccessful at getting
    pool.

--*/

{
    PHANDLE_TABLE HandleTable;
    BOOLEAN HandleTableQuotaCharged;

    PVOID HandleTableTable;
    BOOLEAN HandleTableTableQuotaCharged;

    ULONG i;

    PAGED_CODE();

    HandleTable = NULL;
    HandleTableQuotaCharged = FALSE;

    HandleTableTable = NULL;
    HandleTableTableQuotaCharged = FALSE;

    //
    //  If any alloation or quota failures happen we will catch it in the
    //  following try-except clause and cleanup after outselves before
    //  we return null
    //

    try {

        //
        //  First allocate the handle table, make sure we got one, charge quota
        //  for it and then zero it out
        //

        HandleTable = (PHANDLE_TABLE)ExAllocatePoolWithTag( NonPagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                            sizeof(HANDLE_TABLE),
                                                            'btbO' );

        if (ARGUMENT_PRESENT(Process)) {

            PsChargePoolQuota( Process,
                               NonPagedPool,
                               sizeof(HANDLE_TABLE));

            HandleTableQuotaCharged = TRUE;
        }

        RtlZeroMemory( HandleTable, sizeof(HANDLE_TABLE) );

        //
        //  Now allocate space of the top level, one mid level and one bottom
        //  level table structure.  This will all fit on a page, maybe two.
        //

        HandleTableTable =
        HandleTable->Table = ExAllocatePoolWithTag( PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                    (2 * sizeof(ULONG_PTR) * 256) + (sizeof(HANDLE_TABLE_ENTRY) * 256),
                                                    'btbO' );

        if (ARGUMENT_PRESENT(Process)) {

            PsChargePoolQuota( Process,
                               PagedPool,
                               (2 * sizeof(ULONG_PTR) * 256) + (sizeof(HANDLE_TABLE_ENTRY) * 256) );

            HandleTableTableQuotaCharged = TRUE;

        }

        RtlZeroMemory( HandleTable->Table,
                       (2 * sizeof(ULONG_PTR) * 256) + (sizeof(HANDLE_TABLE_ENTRY) * 256) );

    } except (EXCEPTION_EXECUTE_HANDLER) {

        if (HandleTable != NULL) {

            ExFreePool( HandleTable );

            if (HandleTableQuotaCharged) {

                PsReturnPoolQuota( Process,
                                   NonPagedPool,
                                   sizeof(HANDLE_TABLE));
            }
        }

        if (HandleTableTable != NULL) {

            ExFreePool( HandleTableTable );

            if (HandleTableTableQuotaCharged) {

                PsReturnPoolQuota( Process,
                                   PagedPool,
                                   (2 * sizeof(ULONG_PTR) * 256) + (sizeof(HANDLE_TABLE_ENTRY) * 256) );
            }
        }

        return NULL;
    }

    //
    //  Now setup the pointers for the initial handle table tree
    //

    HandleTable->Table[0]    = (PVOID)(((PCHAR)(HandleTable->Table)) + 1 * (sizeof(ULONG_PTR) * 256));
    HandleTable->Table[0][0] = (PVOID)(((PCHAR)(HandleTable->Table)) + 2 * (sizeof(ULONG_PTR) * 256));

    //
    //  Now setup the free list.  We do this by chaining together the free
    //  entries such that each free entry give the next free index (i.e.,
    //  like a fat chain).  The chain is terminated with a -1.  Note that
    //  we'll skip handle zero because our callers will get that value
    //  confused with null.
    //

    for (i = 0; i < 255; i += 1) {

        (HandleTable->Table[0][0])[i].NextFreeTableEntry = i+1;
    }

    (HandleTable->Table[0][0])[255].NextFreeTableEntry = -1;

    HandleTable->FirstFreeTableEntry = 1;
    HandleTable->NextIndexNeedingPool = 256;

    //
    //  Setup the necessary process information
    //

    HandleTable->QuotaProcess = Process;
    HandleTable->UniqueProcessId = PsGetCurrentProcess()->UniqueProcessId;

    //
    //  Initialize the lock handle table resource
    //

    ExInitializeResource( &HandleTable->HandleTableLock );

    //
    //  Initialize the notification event for handle table entry contention
    //

    KeInitializeEvent( &HandleTable->HandleContentionEvent, NotificationEvent, FALSE );

    //
    //  Insert the handle table in the handle table list.
    //

    KeEnterCriticalRegion();
    ExAcquireResourceExclusive( &HandleTableListLock, TRUE );

    InsertTailList( &HandleTableListHead, &HandleTable->HandleTableList );

    ExReleaseResource( &HandleTableListLock );
    KeLeaveCriticalRegion();

    //
    //  And return to our caller
    //

    return HandleTable;
}


//
//  Local Support Routine
//

VOID
ExpFreeHandleTable (
    IN PHANDLE_TABLE HandleTable
    )

/*++

Routine Description:

    This worker routine tearsdown and frees the specified handle table.

Arguments:

    HandleTable - Supplies the handle table being freed

Return Value:

    None.

--*/

{
    PEPROCESS Process;
    ULONG i,j;

    PAGED_CODE();

    Process = HandleTable->QuotaProcess;

    //
    //  First free the lock for the local handle table
    //

    ExDeleteResource( &HandleTable->HandleTableLock );

    //
    //  We first return all of the additional exhandle table entry buffers
    //  that we allocated.  Note that each pool buffer we allocated is for
    //  two subindex buffers in the table, and note also that we have to
    //  start a little funny to compensate for the first that really contains
    //  the top level and first mid level, and first entry buffer
    //
    //  Take care of the special case where the [0][0] index is really
    //  allocated up front, and we only need to examine [0][1] to see if
    //  anything special needs to be deallocated, followed by [0][3], [0][5],
    //  up to and including [0][255]
    //

    for (j = 1; j < 256; j += 2) {

        if (HandleTable->Table[0][j] == NULL) {

            break;
        }

        ExFreePool( HandleTable->Table[0][j] );

        if (Process != NULL) {

            PsReturnPoolQuota( Process,
                               PagedPool,
                               2 * sizeof(HANDLE_TABLE_ENTRY) * 256 );
        }
    }

    //
    //  Now that we've handle the special case we can do the rest of the table
    //  starting with index [1][0], [1][2], ... [1][254], [2][0], [2][2] ...
    //  up to and including [255][254]
    //

    for (i = 1; i < 256; i += 1) {

        if (HandleTable->Table[i] == NULL) {

            break;
        }

        for (j = 0; j < 256; j += 2) {

            if (HandleTable->Table[i][j] == NULL) {

                break;
            }

            ExFreePool( HandleTable->Table[i][j] );

            if (Process != NULL) {

                PsReturnPoolQuota( Process,
                                   PagedPool,
                                   2 * sizeof(HANDLE_TABLE_ENTRY) * 256 );
            }
        }
    }

    //
    //  Now that we've deallocated the handle table entry buffer we can
    //  deallocate the additional mid level buffers.  Note that that these
    //  start at index [0][1] and were allocated four a time from pool.
    //

    for (i = 1; i < 256; i += 4) {

        if (HandleTable->Table[i] == NULL) {

            break;
        }

        ExFreePool( HandleTable->Table[i] );

        if (Process != NULL) {

            PsReturnPoolQuota( Process,
                               PagedPool,
                               4 * sizeof(ULONG_PTR) * 256 );
        }
    }

    //
    //  Now deallocate the original handle table buffer used to store
    //  the top level, first mid level, and first table entry buffer
    //

    ExFreePool( HandleTable->Table );

    if (Process != NULL) {

        PsReturnPoolQuota( Process,
                           PagedPool,
                           (2 * sizeof(ULONG_PTR) * 256) + (sizeof(HANDLE_TABLE_ENTRY) * 256) );
    }

    //
    //  Finally deallocate the handle table itself
    //

    ExFreePool( HandleTable );

    if (Process != NULL) {

        PsReturnPoolQuota( Process,
                           NonPagedPool,
                           sizeof(HANDLE_TABLE) );
    }

    //
    //  And return to our caller
    //

    return;
}


//
//  Local Support Routine
//

PHANDLE_TABLE_ENTRY
ExpAllocateHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    OUT PEXHANDLE Handle
    )

/*++

Routine Description:

    This worker routine allocates a new handle table entry for the specified
    handle table.

    Note: The caller must have already locked the handle table

Arguments:

    HandleTable - Supplies the handle table being used

    Handle - Returns the handle of the new entry if the allocation is
        successful otherwise the value is null

Return Value:

    Returns a pointer to the new handle table entry is the allocation is
        successful otherwise the return value is null.

--*/

{
    ULONG i,j,k;

    PUCHAR NewMidLevel;
    BOOLEAN MidTableQuotaCharged;

    PUCHAR NewLowLevel;
    BOOLEAN LowTableQuotaCharged;

    PHANDLE_TABLE_ENTRY HandleTableEntry;

    PAGED_CODE();

    Handle->GenericHandleOverlay = NULL;

    //
    //  First see if the free stack is emtpy and if so then we need
    //  to add some more entries to it
    //

    if (HandleTable->FirstFreeTableEntry == -1) {

        //
        //  We need to allocate some more pool.  We start by seeing
        //  if the next index is too large to allocate.  We only have
        //  24 bits of handle values to use
        //

        if (HandleTable->NextIndexNeedingPool >= (1 << 24)) {

            return NULL;
        }

        //  Now find the next index to allocate a buffer for
        //

        i = (HandleTable->NextIndexNeedingPool >> 16) & 255;
        j = (HandleTable->NextIndexNeedingPool >> 8)  & 255;

        //
        //  Within the following try body we'll be allocating and initializing
        //  a low level buffer and possibly a new mid level buffer.  The
        //  might seem a little odd but is necessary to handle the situation
        //  where our first buffer is allocated off a little bit (i.e., the
        //  next buffer index starts are [0][1]).  By the time we reach
        //  [0][255] we add only one new buffer.  Then start with even numbers
        //  at [1][0] and work our way to completion
        //

        NewMidLevel = NULL;
        MidTableQuotaCharged = FALSE;

        NewLowLevel = NULL;
        LowTableQuotaCharged = FALSE;

        try {

            //
            //  Check if we need to allocate a new mid level buffer,  We do
            //  these allocations four at a time
            //

            if (HandleTable->Table[i] == NULL) {

                NewMidLevel = ExAllocatePoolWithTag( PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                     4 * sizeof(ULONG_PTR) * 256,
                                                     'btbO' );

                RtlZeroMemory( NewMidLevel, 4 * sizeof(ULONG_PTR) * 256 );

                if (HandleTable->QuotaProcess != NULL) {

                    PsChargePoolQuota( HandleTable->QuotaProcess,
                                       PagedPool,
                                       (4 * sizeof(ULONG_PTR) * 256) );

                    MidTableQuotaCharged = TRUE;
                }
            }

            //
            //  Now Allocate two new table entry buffers
            //

            NewLowLevel = ExAllocatePoolWithTag( PagedPool | POOL_RAISE_IF_ALLOCATION_FAILURE,
                                                 2 * sizeof(HANDLE_TABLE_ENTRY) * 256,
                                                 'btbO' );

            RtlZeroMemory( NewLowLevel, 2 * sizeof(HANDLE_TABLE_ENTRY) * 256 );

            if (HandleTable->QuotaProcess != NULL) {

                PsChargePoolQuota( HandleTable->QuotaProcess,
                                   PagedPool,
                                   (2 * sizeof(HANDLE_TABLE_ENTRY) * 256) );

                LowTableQuotaCharged = TRUE;
            }

            //
            //  Now to guard against reordering of code we need to do
            //  something to ensure that the last zero memory really has
            //  happened before setting up the rest of our work.  An
            //  interlocked exchance will do the trick.
            //

            InterlockedExchangePointer( (PVOID *) NewLowLevel, NULL );

            //
            //  Now if we've allocated a new mid level buffer we need to
            //  update the pointers from our handle table.  The if's are
            //  needed when we get near the end of the table
            //

            if (NewMidLevel != NULL) {

                if (i+0 < 256) { HandleTable->Table[i+0] = (PVOID)(NewMidLevel + 0 * sizeof(ULONG_PTR) * 256); }
                if (i+1 < 256) { HandleTable->Table[i+1] = (PVOID)(NewMidLevel + 1 * sizeof(ULONG_PTR) * 256); }
                if (i+2 < 256) { HandleTable->Table[i+2] = (PVOID)(NewMidLevel + 2 * sizeof(ULONG_PTR) * 256); }
                if (i+3 < 256) { HandleTable->Table[i+3] = (PVOID)(NewMidLevel + 3 * sizeof(ULONG_PTR) * 256); }
            }

            //
            //  Now fix up the low level buffers.  We are actually guaranteed
            //  that the first buffer will fit, because j starts out from a
            //  byte.
            //

                           { HandleTable->Table[i][j+0] = (PVOID)(NewLowLevel + 0 * sizeof(HANDLE_TABLE_ENTRY) * 256); }
            if (j+1 < 256) { HandleTable->Table[i][j+1] = (PVOID)(NewLowLevel + 1 * sizeof(HANDLE_TABLE_ENTRY) * 256); }

            //
            //  Now add the new entries to the free list.  To do this we
            //  chain the new free entries together.  We are guaranteed to
            //  have at least one new buffer.  The second buffer we need
            //  to check for.
            //
            //  Start by starting our stack top
            //

            HandleTable->FirstFreeTableEntry = HandleTable->NextIndexNeedingPool;

            //
            //  Do the guaranteed first buffer
            //

            for (k = 0; k < 256; k += 1) {

                HandleTable->Table[i][j+0][k].NextFreeTableEntry = HandleTable->NextIndexNeedingPool + k + 1;
            }

            //
            //  Check if there is a second buffer
            //

            if ((j+1 < 256) && (HandleTable->Table[i][j+1] != NULL)) {

                //
                //  Do the second buffer
                //

                for (k = 0; k < 255; k += 1) {

                    HandleTable->Table[i][j+1][k].NextFreeTableEntry = HandleTable->NextIndexNeedingPool + k + 1 + 256;
                }

                //
                //  Fixup the last entry and update the next index needing
                //  pool.
                //

                HandleTable->Table[i][j+1][255].NextFreeTableEntry = -1;

                HandleTable->NextIndexNeedingPool += 512;

            } else {

                //
                //  Fixup the last entry and update the next index needing
                //  pool.
                //

                HandleTable->Table[i][j+0][255].NextFreeTableEntry = -1;

                HandleTable->NextIndexNeedingPool += 256;
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            //  We only execute this exception handler if the pool allocation
            //  raises or if the quota request fails.  In these cases we need
            //  to cleanup after ourselves and return null to our caller
            //

            if (NewMidLevel != NULL) {

                ExFreePool( NewMidLevel );

                if (MidTableQuotaCharged) {

                    PsReturnPoolQuota( HandleTable->QuotaProcess,
                                       PagedPool,
                                       (4 * sizeof(ULONG_PTR) * 256) );
                }
            }

            if (NewLowLevel != NULL) {

                ExFreePool( NewLowLevel );

                if (LowTableQuotaCharged) {

                    PsReturnPoolQuota( HandleTable->QuotaProcess,
                                       PagedPool,
                                       (2 * sizeof(HANDLE_TABLE_ENTRY) * 256) );
                }
            }

            Handle->Index = 0;

            return NULL;
        }
    }

    //
    //  At this point the free stack has some elements in it.  So we
    //  only need to pop off the first entry.
    //
    //  Get the top of the stack, both the index and table entry
    //

    Handle->Index = HandleTable->FirstFreeTableEntry;

    HandleTableEntry = ExpLookupHandleTableEntry( HandleTable,
                                                  *Handle );

    //
    //  Do the pop
    //

    HandleTable->FirstFreeTableEntry = HandleTableEntry->NextFreeTableEntry;

    //
    //  Zero out the new table entry
    //

    RtlZeroMemory( HandleTableEntry, sizeof(HANDLE_TABLE_ENTRY ));

    //
    //  Update our bookkeeping counters and return the entry to our
    //  caller
    //

    HandleTable->HandleCount += 1;

    return HandleTableEntry;
}


//
//  Local Support Routine
//

//
//  The following is a global variable only present in the checked builds
//  to help catch apps that reuse handles values after they're closed.
//

#if DBG
BOOLEAN ExReuseHandles = 1;
#endif //DBG

VOID
ExpFreeHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle,
    IN PHANDLE_TABLE_ENTRY HandleTableEntry
    IN
    )

/*++

Routine Description:

    This worker routine returns the specified handle table entry to the free
    list for the handle table.

    Note: The caller must have already locked the handle table

Arguments:

    HandleTable - Supplies the parent handle table being modified

    Handle - Supplies the handle of the entry being freed

    HandleTableEntry - Supplies the table entry being freed

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  A free is simply a push onto the free table entry stack, or in the
    //  debug case we'll sometimes just float the entry to catch apps who
    //  reuse a recycled handle value.
    //

#if DBG
    if (ExReuseHandles) {
#endif //DBG

        HandleTableEntry->NextFreeTableEntry = HandleTable->FirstFreeTableEntry;
        HandleTable->FirstFreeTableEntry = Handle.Index;

#if DBG
    } else {

        HandleTableEntry->NextFreeTableEntry = 0;
    }
#endif //DBG

    //
    //  And then update our handle count before returning to our caller
    //

    HandleTable->HandleCount -= 1;

    return;
}


//
//  Local Support Routine
//

PHANDLE_TABLE_ENTRY
ExpLookupHandleTableEntry (
    IN PHANDLE_TABLE HandleTable,
    IN EXHANDLE Handle
    )

/*++

Routine Description:

    This routine looks up and returns the table entry for the
    specified handle value.

Arguments:

    HandleTable - Supplies the handle table being queried

    Handle - Supplies the handle value being queried

Return Value:

    Returns a pointer to the corresponding table entry for the input
        handle.  Or NULL if the handle value is invalid (i.e., too large
        for the tables current allocation.

--*/

{
    ULONG i,j,k,l;

    PAGED_CODE();

    //
    //  Decode the handle index into its separate table indicies
    //

    l = (Handle.Index >> 24) & 255;
    i = (Handle.Index >> 16) & 255;
    j = (Handle.Index >> 8)  & 255;
    k = (Handle.Index)       & 255;

    //
    //  The last bits should be 0 into a valid handle. If a function calls
    //  ExpLookupHandleTableEntry for a kernel handle, it should decode the handle
    //  before.
    //

    if ( l != 0 ) {
        
        //
        //  Invalid handle. Return a NULL table entry.
        //

        return NULL;
    }

    //
    //  Check that the top level table is present
    //

    if (HandleTable->Table[i] == NULL) {

        return NULL;
    }

    //
    //  Check that the mid level table is present
    //

    if (HandleTable->Table[i][j] == NULL) {

        return NULL;
    }

    //
    //  Return a pointer to the table entry
    //

    return &(HandleTable->Table[i][j][k]);
}
