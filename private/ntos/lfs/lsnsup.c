/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    LsnSup.c

Abstract:

    This module implements support for manipulating Lsn's.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_LSN_SUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsFindNextLsn)
#pragma alloc_text(PAGE, LfsLsnFinalOffset)
#endif


VOID
LfsLsnFinalOffset (
    IN PLFCB Lfcb,
    IN LSN Lsn,
    IN ULONG DataLength,
    OUT PLONGLONG FinalOffset
    )

/*++

Routine Description:

    This routine will compute the final offset of the last byte of the log
    record.  It does this by computing how many bytes are on the current
    page and then computing how many more pages will be needed.

Arguments:

    Lfcb - This is the file control block for the log file.

    Lsn - This is the log record being considered.

    DataLength - This is the length of the data for this log record.  We will add the
        header length here.

    FinalOffset - Address to store the result.

Return Value:

    None.

--*/

{
    ULONG RemainingPageBytes;
    ULONG PageOffset;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsLsnFinalOffset:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb          ->  %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Lsn (Low)     ->  %08lx\n", Lsn.LowPart );
    DebugTrace(  0, Dbg, "Lsn (High)    ->  %08lx\n", Lsn.HighPart );
    DebugTrace(  0, Dbg, "DataLength    ->  %08lx\n", DataLength );

    //
    //  We compute the starting log page file offset, the number of bytes
    //  remaining in the current log page and the position on this page
    //  before any data bytes.
    //

    LfsTruncateLsnToLogPage( Lfcb, Lsn, FinalOffset );

    PageOffset = LfsLsnToPageOffset( Lfcb, Lsn );

    RemainingPageBytes = (ULONG)Lfcb->LogPageSize - PageOffset;

    PageOffset -= 1;

    //
    //  Add the length of the header.
    //

    DataLength += Lfcb->RecordHeaderLength;

    //
    //  If this Lsn is contained in this log page we are done.
    //  Otherwise we need to walk through several log pages.
    //

    if (DataLength > RemainingPageBytes) {

        DataLength -= RemainingPageBytes;

        RemainingPageBytes = (ULONG)Lfcb->LogPageDataSize;

        PageOffset = (ULONG)Lfcb->LogPageDataOffset - 1;

        while (TRUE) {

            BOOLEAN Wrapped;

            LfsNextLogPageOffset( Lfcb, *FinalOffset, FinalOffset, &Wrapped );

            //
            //  We are done if the remaining bytes fit on this page.
            //

            if (DataLength <= RemainingPageBytes) {

                break;
            }

            DataLength -= RemainingPageBytes;
        }
    }

    //
    //  We add the remaining bytes to our starting position on this page
    //  and then add that value to the file offset of this log page.
    //

    *(PULONG)FinalOffset += (DataLength + PageOffset);

    DebugTrace(  0, Dbg, "FinalOffset (Low)     ->  %08lx\n", LogPageFileOffset.LowPart );
    DebugTrace(  0, Dbg, "FinalOffset (High)    ->  %08lx\n", LogPageFileOffset.HighPart );
    DebugTrace( -1, Dbg, "LfsLsnFinalOffset:  Exit\n", 0 );

    return;
}


BOOLEAN
LfsFindNextLsn (
    IN PLFCB Lfcb,
    IN PLFS_RECORD_HEADER RecordHeader,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine takes as a starting point the log record header of an
    Lsn in the log file.  It searches for the next Lsn in the file and
    returns that value in the 'Lsn' argument.  The boolean return value
    indicates whether there is another Lsn in the file.

Arguments:

    Lfcb - This is the file control block for the log file.

    RecordHeader - This is the log record for the Lsn starting point.

    Lsn - This supplies the address to store the next Lsn, if found.

Return Value:

    BOOLEAN - Indicates whether the next Lsn was found.

--*/

{
    BOOLEAN FoundNextLsn;

    LONGLONG LsnOffset;
    LONGLONG EndOfLogRecord;
    LONGLONG LogHeaderOffset;

    LONGLONG SequenceNumber;

    PLFS_RECORD_PAGE_HEADER LogRecordPage;
    PBCB LogRecordPageBcb;
    BOOLEAN UsaError;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFindNextLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb          -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Record Header -> %08lx\n", RecordHeader );

    LogRecordPageBcb = NULL;
    FoundNextLsn = FALSE;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Find the file offset of the log page which contains the end
        //  of the log record for this Lsn.
        //

        LsnOffset = LfsLsnToFileOffset( Lfcb, RecordHeader->ThisLsn );

        LfsLsnFinalOffset( Lfcb,
                           RecordHeader->ThisLsn,
                           RecordHeader->ClientDataLength,
                           &EndOfLogRecord );

        LfsTruncateOffsetToLogPage( Lfcb, EndOfLogRecord, &LogHeaderOffset );

        //
        //  Remember the sequence number for this page.
        //

        SequenceNumber = LfsLsnToSeqNumber( Lfcb, RecordHeader->ThisLsn );

        //
        //  Remember if we wrapped.
        //

        if ( EndOfLogRecord <= LsnOffset ) {                                                                           //**** xxLeq( EndOfLogRecord, LsnOffset )

            SequenceNumber = SequenceNumber + 1;                                                                       //**** xxAdd( SequenceNumber, LfsLi1 );
        }

        //
        //  Pin the log page header for this page.
        //

        LfsPinOrMapData( Lfcb,
                         LogHeaderOffset,
                         (ULONG)Lfcb->LogPageSize,
                         FALSE,
                         FALSE,
                         FALSE,
                         &UsaError,
                         (PVOID *)&LogRecordPage,
                         &LogRecordPageBcb );

        //
        //  If the Lsn we were given was not the last Lsn on this page, then
        //  the starting offset for the next Lsn is on a quad word boundary
        //  following the last file offset for the current Lsn.  Otherwise
        //  the file offset is the start of the data on the next page.
        //

        if ( RecordHeader->ThisLsn.QuadPart == LogRecordPage->Copy.LastLsn.QuadPart ) {                                //**** xxEql( RecordHeader->ThisLsn, LogRecordPage->Copy.LastLsn )

            BOOLEAN Wrapped;

            LfsNextLogPageOffset( Lfcb,
                                  LogHeaderOffset,
                                  &LogHeaderOffset,
                                  &Wrapped );

            LsnOffset = LogHeaderOffset + Lfcb->LogPageDataOffset;                                                     //**** xxAdd( LogHeaderOffset, Lfcb->LogPageDataOffset );

            //
            //  If we wrapped, we need to increment the sequence number.
            //

            if (Wrapped) {

                SequenceNumber = SequenceNumber + 1;                                                                   //**** xxAdd( SequenceNumber, LfsLi1 );
            }

        } else {

            LiQuadAlign( EndOfLogRecord, &LsnOffset );
        }

        //
        //  Compute the Lsn based on the file offset and the sequence count.
        //

        Lsn->QuadPart = LfsFileOffsetToLsn( Lfcb, LsnOffset, SequenceNumber );

        //
        //  If this Lsn is within the legal range for the file, we return TRUE.
        //  Otherwise FALSE indicates that there are no more Lsn's.
        //

        if (LfsIsLsnInFile( Lfcb, *Lsn )) {

            FoundNextLsn = TRUE;
        }

    } finally {

        DebugUnwind( LfsFindNextLsn );

        //
        //  Unpin the log page header if held.
        //

        if (LogRecordPageBcb != NULL) {

            CcUnpinData( LogRecordPageBcb );
        }

        DebugTrace(  0, Dbg, "Lsn (Low)     -> %08lx\n", Lsn->LowPart );
        DebugTrace(  0, Dbg, "Lsn (High)    -> %08lx\n", Lsn->HighPart );
        DebugTrace( -1, Dbg, "LfsFindNextLsn:  Exit -> %08x\n", FoundNextLsn );
    }

    return FoundNextLsn;
}

