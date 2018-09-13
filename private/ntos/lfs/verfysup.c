/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    VerfySup.c

Abstract:

    This module implements consistency checking and structure comparisions
    on Lfs structures.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

#ifdef LFS_RAISE
BOOLEAN LfsRaiseFull = FALSE;
#endif

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsCurrentAvailSpace)
#pragma alloc_text(PAGE, LfsFindCurrentAvail)
#pragma alloc_text(PAGE, LfsVerifyLogSpaceAvail)
#endif


VOID
LfsCurrentAvailSpace (
    IN PLFCB Lfcb,
    OUT PLONGLONG CurrentAvailSpace,
    OUT PULONG CurrentPageBytes
    )

/*++

Routine Description:

    This routine is called to determine the available log space in the log file.
    It returns the total number of free bytes and the number available on the
    active page if present.  The total free bytes will reflect all of the empty
    pages as well as the number in the active page.

Arguments:

    Lfcb - Lfcb for this log file.

    CurrentAvailSpace - This is the number of bytes available for log
                        records.

    CurrentPageBytes - This is the number of bytes remaining on the
                       current log page.

Return Value:

    None.

--*/

{
    *CurrentPageBytes = 0;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsCurrentAvailSpace:  Entered\n", 0 );

    //
    //  Get the total number from the Lfcb.
    //

    *CurrentAvailSpace = Lfcb->CurrentAvailable;

    //
    //  We now look to see if there are any bytes available on the Lbcb in
    //  the active queue.  We can add this to the bytes available in the
    //  log pages and also give this back to the caller.
    //

    if (!IsListEmpty( &Lfcb->LbcbActive )) {

        PLBCB ThisLbcb;

        ThisLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                      LBCB,
                                      ActiveLinks );

        //
        //  If the page is not empty or the page is empty but this is a
        //  restart page then add the remaining bytes on this page.
        //

        if (FlagOn( ThisLbcb->LbcbFlags, LBCB_NOT_EMPTY | LBCB_FLUSH_COPY )) {

            *CurrentPageBytes = (ULONG)Lfcb->LogPageSize - (ULONG)ThisLbcb->BufferOffset;

            *CurrentAvailSpace = *CurrentAvailSpace + *CurrentPageBytes;                                               //**** xxAdd( *CurrentAvailSpace, xxFromUlong( *CurrentPageBytes ));
        }
    }

    DebugTrace( +1, Dbg, "LfsCurrentAvailSpace:  Exit\n", 0 );

    return;
}


BOOLEAN
LfsVerifyLogSpaceAvail (
    IN PLFCB Lfcb,
    IN PLCH Lch,
    IN ULONG RemainingLogBytes,
    IN LONG UndoRequirement,
    IN BOOLEAN ForceToDisk
    )

/*++

Routine Description:

    This routine is called to verify that we may write this log record into the
    log file.  We want to always leave room for each transaction to abort.

    We determine how much space the current log record will take and the
    worst case for its undo operation.  If this space is available we
    update the corresponding values in the Lfcb and Lch for bookkeeping.
    Otherwise we raise a status indicating that the log file is full.

    The disk usage is different for the packed and unpacked cases.  Make the
    following adjustments after finding the total available and amount still
    remaining on the last active page,

    Packed Case:

        Size needed for log record is data size plus header size.

        Undo requirement is the undo data size plus the header size.
            We have already taken into account the end of the pages
            except for the current page.

        Add the log record size to the undo requirement to get the
            log file usage.  Compare this number with the actual available
            space (Available - CommittedUndo).  If the space is not
            available, then raise LOG_FILE_FULL.  Must take into account
            any unused bytes at the end of the current page.

    Unpacked Case:

        Size needed is initially header size plus data size.

        If the log record can't begin on the current page then
            add the bytes being thrown away to the log record size.

        If the page is being forced to disk then add any remaining
            bytes on the last page.  To the bytes being used.

        Undo requirement is twice the sum of the header size and
            undo size.  We double the requested size since the log
            record will always fit on a page.  This can be a
            positive or negative number.

        Add the log record usage to the undo usage to get the log file
            usage.  Compare this number with the actual available
            space (Available - CommittedUndo).  If the space is not
            available, then raise LOG_FILE_FULL.

Arguments:

    Lfcb - Lfcb for this log file.

    Lch - Client handle

    RemainingLogBytes - Number of bytes for the current log record

    UndoRequirement - User's requirement for the undo record.

    ForceToDisk - Indicates if this log record will be flushed to disk.

Return Value:

    BOOLEAN - Advisory, indicates that there is less than 1/4 of the log file available.

--*/

{
    ULONG CurrentLogRecordSize;
    ULONG LogRecordStart;
    ULONG TailBytes;

    LONGLONG CurrentAvailSpace;
    ULONG CurrentPageBytes;

    LONGLONG LogFileUsage;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsVerifyLogSpaceAvail:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb              -> %08x\n", Lfcb );
    DebugTrace(  0, Dbg, "Lch               -> %08lx\n", Lch );
    DebugTrace(  0, Dbg, "RemainingLogBytes -> %08lx\n", RemainingLogBytes );
    DebugTrace(  0, Dbg, "UndoRequirement   -> %08lx\n", UndoRequirement );
    DebugTrace(  0, Dbg, "ForceToDisk       -> %04x\n", ForceToDisk );

    //
    //  Start by collecting the current data on the file.
    //

    LfsCurrentAvailSpace( Lfcb,
                          &CurrentAvailSpace,
                          &CurrentPageBytes );

    //
    //  We compute the amount of space needed for the current log record by
    //  adding up the following:
    //
    //      Space at end of current log page which won't be used.
    //      Size of header for log record.
    //      Size of client data in log record.
    //      Size of wasted portion of log page if this is forced to disk.
    //

    //
    //  Start with the size of the header and the client data.
    //

    CurrentLogRecordSize = RemainingLogBytes + Lfcb->RecordHeaderLength;

    //
    //  If the log is packed and there are bytes on the current page we need
    //  to take into account any bytes at the end of the page which won't
    //  be used.  This will happen if the log record spills into the end of
    //  the log page but doesn't use up the page.  If the remaining bytes are
    //  less than a record header size we must throw them away.
    //

    if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

        if (CurrentPageBytes != 0
            && CurrentLogRecordSize < CurrentPageBytes
            && (CurrentPageBytes - CurrentLogRecordSize) < Lfcb->RecordHeaderLength) {

            CurrentLogRecordSize += (CurrentPageBytes - CurrentLogRecordSize);
        }

    //
    //  If this is the unpacked case we need to check for bytes being thrown away
    //  on the current page or the last page.
    //

    } else {

        //
        //  If there is an active Lbcb, we need to add any bytes that
        //  would be thrown away at the end.
        //

        if (CurrentPageBytes != 0) {

            //
            //  We won't use this log page unless the new log record will fit or
            //  unless this is the first log record in the page.
            //

            if ((CurrentPageBytes != (ULONG)Lfcb->LogPageDataSize)
                && (CurrentLogRecordSize > CurrentPageBytes)) {

                CurrentLogRecordSize += CurrentPageBytes;

                //
                //  Remember that we will start this log record at the first
                //  byte in the data portion of a page.
                //

                LogRecordStart = 0;

            //
            //  Otherwise this will start at the current offset into the
            //  data portion of the log page.
            //

            } else {

                LogRecordStart = (ULONG)Lfcb->LogPageDataSize - CurrentPageBytes;
            }

        //
        //  If there was no Lbcb, then we know that we will start at the first
        //  byte of the data portion.
        //

        } else {

            LogRecordStart = 0;
        }

        //
        //  We always assume that we will use up the rest of the bytes on the last page
        //  in computing whether the log record will fit in the available space.  We
        //  only subtract that space from the available space if this is a force write.
        //

        if (ForceToDisk) {

            //
            //  We take into account where we start on a log page and continue
            //  to subtract log pages until we know the amount on the last
            //  page.
            //

            TailBytes = RemainingLogBytes + Lfcb->RecordHeaderLength + LogRecordStart;

            while (TailBytes > (ULONG)Lfcb->LogPageDataSize) {

                TailBytes -= (ULONG)Lfcb->LogPageDataSize;
            }

            TailBytes = (ULONG)Lfcb->LogPageDataSize - TailBytes;

            CurrentLogRecordSize += TailBytes;
        }
    }

    //
    //  We now know the number of bytes needed for the current log page.
    //  Next we compute the number of bytes being reserved by UndoRequirement.
    //  If the UndoRequirement is positive, we will add to the amount reserved
    //  in the log file.  If it is negative, we will subtract from the amount
    //  reserved in the log file.
    //

    //
    //  When we have an actual reserve amount, we convert it to positive
    //  and then reserve twice the space required to hold the data and
    //  its header (up to the maximum of a single page.
    //

    if (UndoRequirement != 0) {

        if (!FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

            UndoRequirement *= 2;
        }

        if (UndoRequirement < 0) {

            UndoRequirement -= (2 * Lfcb->RecordHeaderLength);
        } else {

            UndoRequirement += (2 * Lfcb->RecordHeaderLength);
        }
    }

    //
    //  Now compute the net log file usage.  The result may be positive or
    //  negative.
    //

    LogFileUsage = ((LONG) CurrentLogRecordSize)  + UndoRequirement;                                                   //**** xxFromLong( ((LONG) CurrentLogRecordSize)  + UndoRequirement );

    //
    //  The actual available space is the CurrentAvail minus the reserved
    //  undo value in the Lfcb.
    //

    CurrentAvailSpace = CurrentAvailSpace - Lfcb->TotalUndoCommitment;                                                 //**** xxSub( CurrentAvailSpace, Lfcb->TotalUndoCommitment );

    //
    //  If this log file usage is greater than the available log file space
    //  then we raise a status code.
    //

#ifdef LFS_RAISE
    if (LfsRaiseFull) {

        LfsRaiseFull = FALSE;
        DebugTrace( -1, Dbg, "LfsVerifyLogSpaceAvail:  About to raise\n", 0 );
        ExRaiseStatus( STATUS_LOG_FILE_FULL );
    }
#endif

    if (LogFileUsage > CurrentAvailSpace) {

        DebugTrace( -1, Dbg, "LfsVerifyLogSpaceAvail:  About to raise\n", 0 );
        ExRaiseStatus( STATUS_LOG_FILE_FULL );
    }

    Lfcb->TotalUndoCommitment = Lfcb->TotalUndoCommitment + UndoRequirement;                                           //**** xxAdd( Lfcb->TotalUndoCommitment, xxFromLong( UndoRequirement ));

    Lch->ClientUndoCommitment = Lch->ClientUndoCommitment + UndoRequirement;                                           //**** xxAdd( Lch->ClientUndoCommitment, xxFromLong( UndoRequirement ));

    DebugTrace( -1, Dbg, "LfsVerifyLogSpaceAvail:  Exit\n", 0 );

    //
    //  Now check if the log file is almost used up.
    //

    if ((CurrentAvailSpace - LogFileUsage) < (Lfcb->TotalAvailable >> 2)) {

        return TRUE;
    }

    return FALSE;
}


VOID
LfsFindCurrentAvail (
    IN PLFCB Lfcb
    )

/*++

Routine Description:

    This routine is called to calculate the number of bytes available for log
    records which are in completely empty log record pages.  It ignores any
    partial pages in the active work queue and ignores any page which is
    going to be reused.

Arguments:

    Lfcb - Lfcb for this log file.

Return Value:

    None.

--*/

{
    LONGLONG OldestPageOffset;
    LONGLONG NextFreePageOffset;
    LONGLONG FreeBytes;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFindCurrentAvail:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb              -> %08x\n", Lfcb );

    //
    //  If there is a last lsn in the restart area then we know
    //  that we will have to compute the free range.
    //

    if (!FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN )) {

        //
        //  If there is no oldest Lsn then start at the
        //  first page of the file.
        //

        if (FlagOn( Lfcb->Flags, LFCB_NO_OLDEST_LSN )) {

            OldestPageOffset = Lfcb->FirstLogPage;

        } else {

            LfsTruncateOffsetToLogPage( Lfcb,
                                        Lfcb->OldestLsnOffset,
                                        &OldestPageOffset );
        }

        //
        //  We will use the next log page offset to compute the
        //  next free page.  If we are going to reuse this page
        //  go to the next page,  if we are at the first page then
        //  use the end of the file.
        //

        if (FlagOn( Lfcb->Flags, LFCB_REUSE_TAIL )) {

            NextFreePageOffset = Lfcb->NextLogPage + Lfcb->LogPageSize;                                                //**** xxAdd( Lfcb->NextLogPage, Lfcb->LogPageSize );

        } else if ( Lfcb->NextLogPage == Lfcb->FirstLogPage ) {                                                        //**** xxEql( Lfcb->NextLogPage, Lfcb->FirstLogPage )

            NextFreePageOffset = Lfcb->FileSize;

        } else {

            NextFreePageOffset = Lfcb->NextLogPage;
        }

        //
        //  If the two offsets are the same then there is no available space.
        //

        if ( OldestPageOffset == NextFreePageOffset ) {                                                                //**** xxEql( OldestPageOffset, NextFreePageOffset )

            Lfcb->CurrentAvailable = 0;

        } else {

            //
            //  If the free offset follows the oldest offset then subtract
            //  this range from the total available pages.
            //

            if ( OldestPageOffset < NextFreePageOffset ) {                                                             //**** xxLtr( OldestPageOffset, NextFreePageOffset )

                FreeBytes = Lfcb->TotalAvailInPages - ( NextFreePageOffset - OldestPageOffset );                       //**** xxSub( Lfcb->TotalAvailInPages, xxSub( NextFreePageOffset, OldestPageOffset ));

            } else {

                FreeBytes = OldestPageOffset - NextFreePageOffset;                                                     //**** xxSub( OldestPageOffset, NextFreePageOffset );
            }

            //
            //  We now have the total bytes in the pages available.  We
            //  now have to subtract the size of the page header to get
            //  the total available bytes.
            //
            //  We will convert the bytes to pages and then multiple
            //  by the data size of each page.
            //

            FreeBytes = Int64ShrlMod32(((ULONGLONG)(FreeBytes)), Lfcb->LogPageShift);

            Lfcb->CurrentAvailable = FreeBytes * (ULONG)Lfcb->ReservedLogPageSize;                                     //**** xxXMul( FreeBytes, Lfcb->ReservedLogPageSize.LowPart );
        }

    //
    //  Otherwise the entire file is available.
    //

    } else {

        Lfcb->CurrentAvailable = Lfcb->MaxCurrentAvail;
    }

    DebugTrace( -1, Dbg, "LfsFindCurrentAvail:  Exit\n", 0 );

    return;
}
