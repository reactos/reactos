/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    LbcbSup.c

Abstract:

    This module provides support for manipulating log buffer control blocks.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_LBCB_SUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsFlushLbcb)
#pragma alloc_text(PAGE, LfsFlushToLsnPriv)
#pragma alloc_text(PAGE, LfsGetLbcb)
#endif


VOID
LfsFlushLbcb (
    IN PLFCB Lfcb,
    IN PLBCB Lbcb
    )

/*++

Routine Description:

    This routine is called to make sure the data within an Lbcb makes it out
    to disk.  The Lbcb must either already be in the workque or it must be
    a restart Lbcb.

Arguments:

    Lfcb - This is the file control block for the log file.

    Lbcb - This is the Lbcb to flush.

Return Value:

    None.

--*/

{
    LSN LastLsn;
    PLSN FlushedLsn;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFlushLbcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb      -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Lbcb      -> %08lx\n", Lbcb );

    LastLsn = Lbcb->LastEndLsn;

    //
    //  If this is a restart area we use the restart counter in the
    //  Lfcb.  Otherwise we can use the LastFlushedLsn value in the
    //  Lfcb.  This way we can determine that the Lbcb that interests
    //  us has made it out to disk.
    //

    if (LfsLbcbIsRestart( Lbcb )) {

        FlushedLsn = &Lfcb->LastFlushedRestartLsn;

    } else {

        FlushedLsn = &Lfcb->LastFlushedLsn;
    }

    //
    //  We loop here until the desired Lsn has made it to disk.
    //  If we are able to do the I/O, we will perform it.
    //

    do {

        //
        //
        //  If we can do the Io, call down to flush the Lfcb.
        //

        if (Lfcb->LfsIoState == LfsNoIoInProgress) {

            LfsFlushLfcb( Lfcb, Lbcb );

            break;
        }

        //
        //  Otherwise we release the Lfcb and immediately wait on the event.
        //

        Lfcb->Waiters += 1;

        LfsReleaseLfcb( Lfcb );

        KeWaitForSingleObject( &Lfcb->Sync->Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        LfsAcquireLfcb( Lfcb );
        Lfcb->Waiters -= 1;

    } while ( LastLsn.QuadPart > FlushedLsn->QuadPart );

    DebugTrace( -1, Dbg, "LfsFlushLbcb:  Exit\n", 0 );
    return;
}


VOID
LfsFlushToLsnPriv (
    IN PLFCB Lfcb,
    IN LSN Lsn
    )

/*++

Routine Description:

    This routine is the worker routine which performs the work of flushing
    a particular Lsn to disk.  This routine is always called with the
    Lfcb acquired.  This routines makes no guarantee about whether the Lfcb
    is acquired on exit.

Arguments:

    Lfcb - This is the file control block for the log file.

    Lsn - This is the Lsn to flush to disk.

Return Value:

    None.

--*/

{
    BOOLEAN UseLastRecordLbcb = FALSE;
    PLBCB LastRecordLbcb = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFlushToLsnPriv:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb          -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Lsn (Low)     -> %08lx\n", Lsn.LowPart );
    DebugTrace(  0, Dbg, "Lsn (High)    -> %08lx\n", Lsn.HighPart );

    //
    //  We check if the Lsn is in the valid range.  Raising an
    //  exception if not.
    //

    if (Lsn.QuadPart > Lfcb->RestartArea->CurrentLsn.QuadPart) {

        UseLastRecordLbcb = TRUE;
    }

    //
    //  If the Lsn has already been flushed we are done.
    //  Otherwise we need to look through the workqueues and the
    //  active queue.
    //

    if (Lsn.QuadPart > Lfcb->LastFlushedLsn.QuadPart) {

        PLIST_ENTRY ThisEntry;
        PLBCB ThisLbcb;

        //
        //  Check the workqueue first.  We are looking for the last
        //  buffer block of a log page block which contains this
        //  Lsn.
        //

        ThisEntry = Lfcb->LbcbWorkque.Flink;

        //
        //  We keep looping.
        //

        while (TRUE) {

            ThisLbcb = CONTAINING_RECORD( ThisEntry,
                                          LBCB,
                                          WorkqueLinks );

            //
            //  We pass over any restart areas.  We also skip any
            //  Lbcb's which do not contain the end of a log record.
            //

            if (!LfsLbcbIsRestart( ThisLbcb )
                && FlagOn( ThisLbcb->Flags, LOG_PAGE_LOG_RECORD_END )) {

                LastRecordLbcb = ThisLbcb;

                //
                //  If the last complete Lsn in this Lbcb is greater or equal
                //  to the desired Lsn, we exit the loop.
                //

                if (ThisLbcb->LastEndLsn.QuadPart >= Lsn.QuadPart) {

                    break;
                }
            }

            //
            //  Otherwise move to the next Lbcb.
            //

            ThisEntry = ThisEntry->Flink;

            //
            //  If we have reached the end of the list then break out.  We
            //  were given an Lsn which is larger than any flushed Lsn so
            //  we will just flush to the end of the log file.
            //

            if (ThisEntry == &Lfcb->LbcbWorkque) {

                if (UseLastRecordLbcb) {

                    ThisLbcb = LastRecordLbcb;
                }

                break;
            }
        }

        if (ThisLbcb != NULL) {

            //
            //  If we are not supporting a packed log file and this Lbcb is from
            //  the active queue, we need to check that losing the tail of the
            //  will not swallow up any of our reserved space.
            //

            if (!FlagOn( Lfcb->Flags, LFCB_PACK_LOG )
                && FlagOn( ThisLbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE )) {

                LONGLONG CurrentAvail;
                LONGLONG UnusedBytes;

                //
                //  Find the unused bytes.
                //

                UnusedBytes = 0;

                LfsCurrentAvailSpace( Lfcb,
                                      &CurrentAvail,
                                      (PULONG)&UnusedBytes );

                CurrentAvail = CurrentAvail - Lfcb->TotalUndoCommitment;

                if (UnusedBytes > CurrentAvail) {

                    DebugTrace( -1, Dbg, "Have to preserve these bytes for possible aborts\n", 0 );

                    ExRaiseStatus( STATUS_LOG_FILE_FULL );
                }

                //
                //  We want to make sure we don't write any more data into this
                //  page.  Remove this from the active queue.
                //

                RemoveEntryList( &ThisLbcb->ActiveLinks );
                ClearFlag( ThisLbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );
            }

            //
            //  We now have the Lbcb we want to flush to disk.
            //

            LfsFlushLbcb( Lfcb, ThisLbcb );
        }
    }

    DebugTrace( -1, Dbg, "LfsFlushToLsnPriv:  Exit\n", 0 );

    return;
}


PLBCB
LfsGetLbcb (
    IN PLFCB Lfcb
    )

/*++

Routine Description:

    This routine is called to add a Lbcb to the active queue.

Arguments:

    Lfcb - This is the file control block for the log file.

Return Value:

    PLBCB - Pointer to the Lbcb allocated.

--*/

{
    PLBCB Lbcb = NULL;
    PVOID PageHeader;
    PBCB PageHeaderBcb = NULL;

    BOOLEAN WrappedOrUsaError;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsGetLbcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb      -> %08lx\n", Lfcb );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Pin the desired record page.
        //

        LfsPreparePinWriteData( Lfcb,
                                Lfcb->NextLogPage,
                                (ULONG)Lfcb->LogPageSize,
                                &PageHeader,
                                &PageHeaderBcb );

        //
        //  Put our signature into the page so we won't fail if we
        //  see a previous 'BAAD' signature.
        //

        *((PULONG) PageHeader) = LFS_SIGNATURE_RECORD_PAGE_ULONG;

        //
        //  Now allocate an Lbcb.
        //

        LfsAllocateLbcb( Lfcb, &Lbcb );

        //
        //  If we are at the beginning of the file we test that the
        //  sequence number won't wrap to 0.
        //

        if (!FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN | LFCB_REUSE_TAIL )
            && ( Lfcb->NextLogPage == Lfcb->FirstLogPage )) {

            Lfcb->SeqNumber = Lfcb->SeqNumber + 1;

            //
            //  If the sequence number is going from 0 to 1, then
            //  this is the first time the log file has wrapped.  We want
            //  to remember this because it means that we can now do
            //  large spiral writes.
            //

            if (Int64ShllMod32( Lfcb->SeqNumber, Lfcb->FileDataBits ) == 0) {

                DebugTrace( 0, Dbg, "Log sequence number about to wrap:  Lfcb -> %08lx\n", Lfcb );
                KeBugCheck( FILE_SYSTEM );
            }

            //
            //  If this number is greater or equal to  the wrap sequence number in
            //  the Lfcb, set the wrap flag in the Lbcb.
            //

            if (!FlagOn( Lfcb->Flags, LFCB_LOG_WRAPPED )
                && ( Lfcb->SeqNumber >= Lfcb->SeqNumberForWrap )) {

                SetFlag( Lbcb->LbcbFlags, LBCB_LOG_WRAPPED );
                SetFlag( Lfcb->Flags, LFCB_LOG_WRAPPED );
            }
        }

        //
        //  Now initialize the rest of the Lbcb fields.
        //

        Lbcb->FileOffset = Lfcb->NextLogPage;
        Lbcb->SeqNumber = Lfcb->SeqNumber;
        Lbcb->BufferOffset = Lfcb->LogPageDataOffset;

        //
        //  Store the next page in the Lfcb.
        //

        LfsNextLogPageOffset( Lfcb,
                              Lfcb->NextLogPage,
                              &Lfcb->NextLogPage,
                              &WrappedOrUsaError );

        Lbcb->Length = Lfcb->LogPageSize;
        Lbcb->PageHeader = PageHeader;
        Lbcb->LogPageBcb = PageHeaderBcb;

        Lbcb->ResourceThread = ExGetCurrentResourceThread();
        Lbcb->ResourceThread = (ERESOURCE_THREAD) ((ULONG) Lbcb->ResourceThread | 3);

        //
        //  If we are reusing a previous page then set a flag in
        //  the Lbcb to indicate that we should flush a copy
        //  first.
        //

        if (FlagOn( Lfcb->Flags, LFCB_REUSE_TAIL )) {

            SetFlag( Lbcb->LbcbFlags, LBCB_FLUSH_COPY );
            ClearFlag( Lfcb->Flags, LFCB_REUSE_TAIL );

            (ULONG)Lbcb->BufferOffset = Lfcb->ReusePageOffset;

            Lbcb->Flags = ((PLFS_RECORD_PAGE_HEADER) PageHeader)->Flags;
            Lbcb->LastLsn = ((PLFS_RECORD_PAGE_HEADER) PageHeader)->Copy.LastLsn;
            Lbcb->LastEndLsn = ((PLFS_RECORD_PAGE_HEADER) PageHeader)->Header.Packed.LastEndLsn;
        }

        //
        //  Put the Lbcb on the active queue
        //

        InsertTailList( &Lfcb->LbcbActive, &Lbcb->ActiveLinks );

        SetFlag( Lbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );

        //
        //  Now that we have succeeded, set the owner thread to Thread + 1 so the resource
        //  package will know not to peek in this thread.  It may be deallocated before
        //  we release the Bcb during flush.
        //

        CcSetBcbOwnerPointer( Lbcb->LogPageBcb, (PVOID) Lbcb->ResourceThread );

    } finally {

        DebugUnwind( LfsGetLbcb );

        //
        //  If an error occurred, we need to clean up any blocks which
        //  have not been added to the active queue.
        //

        if (AbnormalTermination()) {

            if (Lbcb != NULL) {

                LfsDeallocateLbcb( Lfcb, Lbcb );
                Lbcb = NULL;
            }

            //
            //  Unpin the system page if pinned.
            //

            if (PageHeaderBcb != NULL) {

                CcUnpinData( PageHeaderBcb );
            }
        }

        DebugTrace( -1, Dbg, "LfsGetLbcb:  Exit\n", 0 );
    }

    return Lbcb;
}
