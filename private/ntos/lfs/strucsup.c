/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    StrucSup.c

Abstract:

    This module provides support routines for creation and deletion
    of Lfs structures.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_STRUC_SUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsAllocateLbcb)
#pragma alloc_text(PAGE, LfsAllocateLfcb)
#pragma alloc_text(PAGE, LfsDeallocateLbcb)
#pragma alloc_text(PAGE, LfsDeallocateLfcb)
#pragma alloc_text(PAGE, LfsAllocateLcb)
#pragma alloc_text(PAGE, LfsDeallocateLcb)
#endif


PLFCB
LfsAllocateLfcb (
    )

/*++

Routine Description:

    This routine allocates and initializes a log file control block.

Arguments:

Return Value:

    PLFCB - A pointer to the log file control block just
                              allocated and initialized.

--*/

{
    PLFCB Lfcb = NULL;
    ULONG Count;
    PLBCB NextLbcb;
    PLCB  NextLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsAllocateLfcb:  Entered\n", 0 );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Allocate and zero the structure for the Lfcb.
        //

        Lfcb = FsRtlAllocatePool( PagedPool, sizeof( LFCB ));

        //
        //  Zero out the structure initially.
        //

        RtlZeroMemory( Lfcb, sizeof( LFCB ));

        //
        //  Initialize the log file control block.
        //

        Lfcb->NodeTypeCode = LFS_NTC_LFCB;
        Lfcb->NodeByteSize = sizeof( LFCB );

        //
        //  Initialize the client links.
        //

        InitializeListHead( &Lfcb->LchLinks );

        //
        //  Initialize the Lbcb links.
        //

        InitializeListHead( &Lfcb->LbcbWorkque );
        InitializeListHead( &Lfcb->LbcbActive );

        //
        //  Initialize and allocate the spare Lbcb queue.
        //

        InitializeListHead( &Lfcb->SpareLbcbList );

        for (Count = 0; Count < LFCB_RESERVE_LBCB_COUNT; Count++) {

            NextLbcb = ExAllocatePoolWithTag( PagedPool, sizeof( LBCB ), ' sfL' );

            if (NextLbcb != NULL) {

                InsertHeadList( &Lfcb->SpareLbcbList, (PLIST_ENTRY) NextLbcb );
                Lfcb->SpareLbcbCount += 1;
            }
        }

        //
        //  Initialize and allocate the spare Lcb queue.
        //

        InitializeListHead( &Lfcb->SpareLcbList );

        for (Count = 0; Count < LFCB_RESERVE_LCB_COUNT; Count++)  {

            NextLcb = ExAllocatePoolWithTag( PagedPool, sizeof( LCB ), ' sfL' );

            if (NextLcb != NULL) {

                InsertHeadList( &Lfcb->SpareLcbList, (PLIST_ENTRY) NextLcb );
                Lfcb->SpareLcbCount += 1;
            }
        }

        //
        //  Allocate the Lfcb synchronization event.
        //

        Lfcb->Sync = FsRtlAllocatePool( NonPagedPool, sizeof( LFCB_SYNC ));

        ExInitializeResource( &Lfcb->Sync->Resource );

        //
        //  Initialize the pseudo Lsn for the restart Lbcb's
        //

        Lfcb->NextRestartLsn = LfsLi1;

        //
        //  Initialize the event to the signalled state.
        //

        KeInitializeEvent( &Lfcb->Sync->Event, NotificationEvent, TRUE );

        Lfcb->Sync->UserCount = 0;

        //
        //  Initialize the spare list mutex
        //

        ExInitializeFastMutex( &(Lfcb->Sync->SpareListMutex) );

    } finally {

        DebugUnwind( LfsAllocateFileControlBlock );

        if (AbnormalTermination()
            && Lfcb != NULL) {

            LfsDeallocateLfcb( Lfcb, TRUE );
            Lfcb = NULL;
        }

        DebugTrace( -1, Dbg, "LfsAllocateLfcb:  Exit -> %08lx\n", Lfcb );
    }

    return Lfcb;
}


VOID
LfsDeallocateLfcb (
    IN PLFCB Lfcb,
    IN BOOLEAN CompleteTeardown
    )

/*++

Routine Description:

    This routine releases the resources associated with a log file control
    block.

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    CompleteTeardown - Indicates if we are to completely remove this Lfcb.

Return Value:

    None

--*/

{
    PLBCB NextLbcb;
    PLCB  NextLcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsDeallocateLfcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb  -> %08lx\n", Lfcb );

    //
    //  Check that there are no buffer blocks.
    //

    ASSERT( IsListEmpty( &Lfcb->LbcbActive ));
    ASSERT( IsListEmpty( &Lfcb->LbcbWorkque ));

    //
    //  Check that we have no clients.
    //

    ASSERT( IsListEmpty( &Lfcb->LchLinks ));

    //
    //  If there is a restart area we deallocate it.
    //

    if (Lfcb->RestartArea != NULL) {

        LfsDeallocateRestartArea( Lfcb->RestartArea );
    }

    //
    //  If there are any of the tail Lbcb's, deallocate them now.
    //

    if (Lfcb->ActiveTail != NULL) {

        LfsDeallocateLbcb( Lfcb, Lfcb->ActiveTail );
        Lfcb->ActiveTail = NULL;
    }

    if (Lfcb->PrevTail != NULL) {

        LfsDeallocateLbcb( Lfcb, Lfcb->PrevTail );
        Lfcb->PrevTail = NULL;
    }

    //
    //  Only do the following if we are to remove the Lfcb completely.
    //

    if (CompleteTeardown) {

        //
        //  If there is a resource structure we deallocate it.
        //

        if (Lfcb->Sync != NULL) {

            ExDeleteResource( &Lfcb->Sync->Resource );

            ExFreePool( Lfcb->Sync );
        }
    }

    //
    //  Deallocate all of the spare Lbcb's.
    //

    while (!IsListEmpty( &Lfcb->SpareLbcbList )) {

        NextLbcb = (PLBCB) Lfcb->SpareLbcbList.Flink;

        RemoveHeadList( &Lfcb->SpareLbcbList );

        ExFreePool( NextLbcb );
    }

    //
    //  Deallocate all of the spare Lcb's.
    //

    while (!IsListEmpty( &Lfcb->SpareLcbList )) {

        NextLcb = (PLCB) Lfcb->SpareLcbList.Flink;

        RemoveHeadList( &Lfcb->SpareLcbList );

        ExFreePool( NextLcb );
    }

    //
    //  Discard the Lfcb structure.
    //

    ExFreePool( Lfcb );

    DebugTrace( -1, Dbg, "LfsDeallocateLfcb:  Exit\n", 0 );
    return;
}


VOID
LfsAllocateLbcb (
    IN PLFCB Lfcb,
    OUT PLBCB *Lbcb
    )

/*++

Routine Description:

    This routine will allocate the next Lbcb.  If the pool allocation fails
    we will look at the private queue of Lbcb's.

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Lbcb - Address to store the allocated Lbcb.

Return Value:

    None

--*/

{
    PLBCB NewLbcb = NULL;

    PAGED_CODE();

    //
    //  If there are enough entries on the look-aside list then get one from
    //  there.
    //

    if (Lfcb->SpareLbcbCount > LFCB_RESERVE_LBCB_COUNT) {

        NewLbcb = (PLBCB) Lfcb->SpareLbcbList.Flink;

        Lfcb->SpareLbcbCount -= 1;
        RemoveHeadList( &Lfcb->SpareLbcbList );

    //
    //  Otherwise try to allocate from pool.
    //

    } else {

        NewLbcb = ExAllocatePoolWithTag( PagedPool, sizeof( LBCB ), ' sfL' );
    }

    //
    //  If we didn't get one then look at the look-aside list.
    //

    if (NewLbcb == NULL) {

        if (Lfcb->SpareLbcbCount != 0) {

            NewLbcb = (PLBCB) Lfcb->SpareLbcbList.Flink;

            Lfcb->SpareLbcbCount -= 1;
            RemoveHeadList( &Lfcb->SpareLbcbList );

        } else {

            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }
    }

    //
    //  Initialize the structure.
    //

    RtlZeroMemory( NewLbcb, sizeof( LBCB ));
    NewLbcb->NodeTypeCode = LFS_NTC_LBCB;
    NewLbcb->NodeByteSize = sizeof( LBCB );

    //
    //  Return it to the user.
    //

    *Lbcb = NewLbcb;
    return;
}


VOID
LfsDeallocateLbcb (
    IN PLFCB Lfcb,
    IN PLBCB Lbcb
    )

/*++

Routine Description:

    This routine will deallocate the Lbcb.  If we need one for the look-aside
    list we will put it there.

Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Lbcb - This is the Lbcb to deallocate.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Deallocate any restart area attached to this Lbcb.
    //

    if (FlagOn( Lbcb->LbcbFlags, LBCB_RESTART_LBCB ) &&
        (Lbcb->PageHeader != NULL)) {

        LfsDeallocateRestartArea( Lbcb->PageHeader );
    }

    //
    //  Put this in the Lbcb queue if it is short.
    //

    if (Lfcb->SpareLbcbCount < LFCB_MAX_LBCB_COUNT) {

        InsertHeadList( &Lfcb->SpareLbcbList, (PLIST_ENTRY) Lbcb );
        Lfcb->SpareLbcbCount += 1;

    //
    //  Otherwise just free the pool block.
    //

    } else {

        ExFreePool( Lbcb );
    }

    return;
}



VOID
LfsAllocateLcb (
    IN PLFCB Lfcb,
    OUT PLCB *NewLcb
    )
/*++

Routine Description:

    This routine will allocate an Lcb. If the pool fails we will fall back
    on our spare list. A failure then will result in an exception
    
Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Lcb - This will contain the new lcb 

Return Value:

    None

--*/
{

    ExAcquireFastMutex( &(Lfcb->Sync->SpareListMutex) );
    
    try {
        
        *NewLcb = NULL;
        if (Lfcb->SpareLcbCount < LFCB_RESERVE_LCB_COUNT) {                   
            (*NewLcb) = ExAllocatePoolWithTag( PagedPool, sizeof( LCB ), ' sfL' );  
        }
        
        if ((*NewLcb) == NULL) {                                                   
            if (Lfcb->SpareLcbCount > 0) {                                    
                *NewLcb = (PLCB) Lfcb->SpareLcbList.Flink;                     
                Lfcb->SpareLcbCount -= 1;                                     
                RemoveHeadList( &Lfcb->SpareLcbList );                        
            } else {                                                           
                ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );                 
            }                                                                   
        }                                                                       
        
        RtlZeroMemory( (*NewLcb), sizeof( LCB ) );                      
        (*NewLcb)->NodeTypeCode = LFS_NTC_LCB;                         
        (*NewLcb)->NodeByteSize = sizeof( LCB );                       
    
    } finally {
        ExReleaseFastMutex( &(Lfcb->Sync->SpareListMutex) );
    }
}



VOID
LfsDeallocateLcb (
    IN PLFCB Lfcb,
    IN PLCB Lcb
    )
/*++

Routine Description:

    This routine will deallocate an Lcb. We'll cache the old lcb if there
    aren't too many already on the spare list
    
Arguments:

    Lfcb - Supplies a pointer to the log file control block.

    Lcb - This will contain the lcb to release

Return Value:

    None

--*/

{
    if (Lcb->RecordHeaderBcb != NULL) {                               
        CcUnpinData( Lcb->RecordHeaderBcb );                          
    }                                                                   
    if ((Lcb->CurrentLogRecord != NULL) && Lcb->AuxilaryBuffer) {                             
        LfsFreeSpanningBuffer( Lcb->CurrentLogRecord );                          
    }                                                                   

    ExAcquireFastMutex( &(Lfcb->Sync->SpareListMutex) );

    try {
        if (Lfcb->SpareLcbCount < LFCB_MAX_LCB_COUNT) {                   
            InsertHeadList( &Lfcb->SpareLcbList, (PLIST_ENTRY) Lcb );   
            Lfcb->SpareLcbCount += 1;                                     
        } else {                                                            
            ExFreePool( Lcb );                                              
        }                                                                   
    } finally {
        ExReleaseFastMutex( &(Lfcb->Sync->SpareListMutex) );    
    }
}



