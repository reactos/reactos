/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    StackOvf.c

Abstract:

    The file lock package provides a worker thread to handle
    stack overflow conditions in the file systems.  When the
    file system detects that it is near the end of its stack
    during a paging I/O read request it will post the request
    to this extra thread.

Author:

    Gary Kimura     [GaryKi]    24-Nov-1992

Revision History:

--*/

#include "FsRtlP.h"
//
// Queue object that is used to hold work queue entries and synchronize
// worker thread activity.
//

KQUEUE FsRtlWorkerQueues[2];

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('srSF')


//
//  Local Support Routine
//

VOID
FsRtlStackOverflowRead (
    IN PVOID Context
    );

VOID
FsRtlpPostStackOverflow (
    IN PVOID Context,
    IN PKEVENT Event,
    IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine,
    IN BOOLEAN PagingFile
    );

//
// Procedure prototype for the worker thread.
//

VOID
FsRtlWorkerThread(
    IN PVOID StartContext
    );

//
//  The following type is used to store an enqueue work item
//

typedef struct _STACK_OVERFLOW_ITEM {

    WORK_QUEUE_ITEM Item;

    //
    //  This is the call back routine
    //

    PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine;

    //
    //  Here are the parameters for the call back routine
    //

    PVOID Context;
    PKEVENT Event;

} STACK_OVERFLOW_ITEM;
typedef STACK_OVERFLOW_ITEM *PSTACK_OVERFLOW_ITEM;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, FsRtlInitializeWorkerThread)
#endif



NTSTATUS
FsRtlInitializeWorkerThread (
    VOID
    )
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Thread;
    ULONG i;

    //
    // Create worker threads to handle normal and paging overflow reads.
    //

    InitializeObjectAttributes(&ObjectAttributes, NULL, 0, NULL, NULL);

    for (i=0; i < 2; i++) {

        //
        // Initialize the FsRtl stack overflow work Queue objects.
        //

        KeInitializeQueue(&FsRtlWorkerQueues[i], 0);

        if (!NT_SUCCESS(PsCreateSystemThread(&Thread,
                                             THREAD_ALL_ACCESS,
                                             &ObjectAttributes,
                                             0L,
                                             NULL,
                                             FsRtlWorkerThread,
                                             ULongToPtr( i )))) {

            return FALSE;
        }
    }

    ZwClose( Thread );

    return TRUE;
}

VOID
FsRtlPostStackOverflow (
    IN PVOID Context,
    IN PKEVENT Event,
    IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine
    )

/*++

Routine Description:

    This routines posts a stack overflow item to the stack overflow
    thread and returns.

Arguments:

    Context - Supplies the context to pass to the stack overflow
        call back routine.  If the low order bit is set, then
        this overflow was a read to a paging file.

    Event - Supplies a pointer to an event to pass to the stack
        overflow call back routine.

    StackOverflowRoutine - Supplies the call back to use when
        processing the request in the overflow thread.

Return Value:

    None.

--*/

{
    FsRtlpPostStackOverflow( Context, Event, StackOverflowRoutine, FALSE );
    return;
}


VOID
FsRtlPostPagingFileStackOverflow (
    IN PVOID Context,
    IN PKEVENT Event,
    IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine
    )

/*++

Routine Description:

    This routines posts a stack overflow item to the stack overflow
    thread and returns.

Arguments:

    Context - Supplies the context to pass to the stack overflow
        call back routine.  If the low order bit is set, then
        this overflow was a read to a paging file.

    Event - Supplies a pointer to an event to pass to the stack
        overflow call back routine.

    StackOverflowRoutine - Supplies the call back to use when
        processing the request in the overflow thread.

Return Value:

    None.

--*/

{
    FsRtlpPostStackOverflow( Context, Event, StackOverflowRoutine, TRUE );
    return;
}


VOID
FsRtlpPostStackOverflow (
    IN PVOID Context,
    IN PKEVENT Event,
    IN PFSRTL_STACK_OVERFLOW_ROUTINE StackOverflowRoutine,
    IN BOOLEAN PagingFile
    )

/*++

Routine Description:

    This routines posts a stack overflow item to the stack overflow
    thread and returns.

Arguments:

    Context - Supplies the context to pass to the stack overflow
        call back routine.  If the low order bit is set, then
        this overflow was a read to a paging file.

    Event - Supplies a pointer to an event to pass to the stack
        overflow call back routine.

    StackOverflowRoutine - Supplies the call back to use when
        processing the request in the overflow thread.

    PagingFile - Indicates if the read is destined to a paging file.

Return Value:

    None.

--*/

{
    PSTACK_OVERFLOW_ITEM StackOverflowItem;

    //
    //  Allocate a stack overflow work item it will later be deallocated by
    //  the stack overflow thread
    //

    StackOverflowItem = FsRtlpAllocatePool( PagingFile ?
                                            NonPagedPoolMustSucceed :
                                            NonPagedPool,
                                            sizeof(STACK_OVERFLOW_ITEM) );

    //
    //  Fill in the fields in the new item
    //

    StackOverflowItem->Context              = Context;
    StackOverflowItem->Event                = Event;
    StackOverflowItem->StackOverflowRoutine = StackOverflowRoutine;

    ExInitializeWorkItem( &StackOverflowItem->Item,
                          &FsRtlStackOverflowRead,
                          StackOverflowItem );

    //
    //  Safely add it to the overflow queue
    //

    KeInsertQueue( &FsRtlWorkerQueues[PagingFile],
                   &StackOverflowItem->Item.List );

    //
    //  And return to our caller
    //

    return;
}


//
//  Local Support Routine
//

VOID
FsRtlStackOverflowRead (
    IN PVOID Context
    )

/*++

Routine Description:

    This routine processes all of the stack overflow request posted by
    the various file systems

Arguments:

Return Value:

    None.

--*/

{
    PSTACK_OVERFLOW_ITEM StackOverflowItem;

    //
    //  Since stack overflow reads are always recursive, set the
    //  TopLevelIrp field appropriately so that recurive reads
    //  from this point will not think they are top level.
    //

    PsGetCurrentThread()->TopLevelIrp = FSRTL_FSP_TOP_LEVEL_IRP;

    //
    //  Get a pointer to the stack overflow item and then call
    //  the callback routine to do the work
    //

    StackOverflowItem = (PSTACK_OVERFLOW_ITEM)Context;

    (StackOverflowItem->StackOverflowRoutine)(StackOverflowItem->Context,
                                              StackOverflowItem->Event);

    //
    //  Deallocate the work item and then go back to the loop to
    //  to wait for another work item
    //

    ExFreePool( StackOverflowItem );

    PsGetCurrentThread()->TopLevelIrp = (ULONG_PTR)NULL;
}

VOID
FsRtlWorkerThread(
    IN PVOID StartContext
    )

{
    PLIST_ENTRY Entry;
    PWORK_QUEUE_ITEM WorkItem;
    ULONG PagingFile = (ULONG)(ULONG_PTR)StartContext;

    //
    //  Set our priority to low realtime, or +1 for PagingFile.
    //

    (VOID)KeSetPriorityThread( &PsGetCurrentThread()->Tcb,
                               LOW_REALTIME_PRIORITY + PagingFile );

    //
    // Loop forever waiting for a work queue item, calling the processing
    // routine, and then waiting for another work queue item.
    //

    do {

        //
        // Wait until something is put in the queue.
        //
        // By specifying a wait mode of KernelMode, the thread's kernel stack is
        // NOT swappable
        //

        Entry = KeRemoveQueue(&FsRtlWorkerQueues[PagingFile], KernelMode, NULL);
        WorkItem = CONTAINING_RECORD(Entry, WORK_QUEUE_ITEM, List);

        //
        // Execute the specified routine.
        //

        (WorkItem->WorkerRoutine)(WorkItem->Parameter);
        if (KeGetCurrentIrql() != 0) {
            KeBugCheckEx(
                IRQL_NOT_LESS_OR_EQUAL,
                (ULONG_PTR)WorkItem->WorkerRoutine,
                (ULONG_PTR)KeGetCurrentIrql(),
                (ULONG_PTR)WorkItem->WorkerRoutine,
                (ULONG_PTR)WorkItem
                );
        }

    } while(TRUE);
}
