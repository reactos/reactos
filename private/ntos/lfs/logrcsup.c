/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    LogRcSup.c

Abstract:

    This module implements support for dealing with log records, both
    writing and recovering them.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOG_RECORD_SUP)

VOID
LfsPrepareLfcbForLogRecord (
    IN OUT PLFCB Lfcb,
    IN ULONG RemainingLogBytes
    );

VOID
LfsTransferLogBytes (
    IN PLBCB Lbcb,
    IN OUT PLFS_WRITE_ENTRY *ThisWriteEntry,
    IN OUT PCHAR *CurrentBuffer,
    IN OUT PULONG CurrentByteCount,
    IN OUT PULONG PadBytes,
    IN OUT PULONG RemainingPageBytes,
    IN OUT PULONG RemainingLogBytes
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsPrepareLfcbForLogRecord)
#pragma alloc_text(PAGE, LfsTransferLogBytes)
#pragma alloc_text(PAGE, LfsWriteLogRecordIntoLogPage)
#endif


BOOLEAN
LfsWriteLogRecordIntoLogPage (
    IN PLFCB Lfcb,
    IN PLCH Lch,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId OPTIONAL,
    IN LSN ClientUndoNextLsn OPTIONAL,
    IN LSN ClientPreviousLsn OPTIONAL,
    IN LONG UndoRequirement,
    IN BOOLEAN ForceToDisk,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine is called to write a log record into the log file
    using the cache manager.  If there is room in the current log
    page it is added to that.  Otherwise we allocate a new log page
    and write the log record header for this log page.  We then
    write the log record into the remaining bytes of this page and
    into any subsequent pages if needed.

Arguments:

    Lfcb - File control block for this log file.

    Lch - This is the client handle, we may update the undo space for this
          client.

    NumberOfWriteEntries - Number of components of the log record.

    WriteEntries - Pointer to an array of write entries.

    UndoRequirement - Signed value indicating the requirement to write
                      an abort log record for this log record.  A negative
                      value indicates that this is the abort record.

    RecordType - The Lfs-defined type of this log record.

    TransactionId - Pointer to the transaction structure containing the
                    Id for transaction containing this operation.

    ClientUndoNextLsn - This is the Lsn provided by the client for use
                        in his restart.  Will be the zero Lsn for
                        a restart log record.

    ClientPreviousLsn - This is the Lsn provided by the client for use
                        in his restart.  Will the the zero Lsn for a
                        restart log record.

    UndoRequirement - This is the data size for the undo record for
                      this log record.

    ForceToDisk - Indicates if this log record will be flushed immediately
                  to disk.

    Lsn - A pointer to store the Lsn for this log record.

Return Value:

    BOOLEAN - Advisory, TRUE indicates that less than 1/4 of the log file is
        available.

--*/

{
    PLFS_WRITE_ENTRY ThisWriteEntry;

    ULONG RemainingLogBytes;
    ULONG OriginalLogBytes;

    ULONG RemainingPageBytes;
    ULONG HeaderAdjust;

    PLBCB ThisLbcb;

    LSN NextLsn;

    PLFS_RECORD_HEADER RecordHeader;

    PCHAR CurrentBuffer;
    ULONG CurrentByteCount;
    ULONG PadBytes;

    BOOLEAN LogFileFull = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsWriteLogRecordIntoLogPage:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb                      -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Lch                       -> %08lx\n", Lch );
    DebugTrace(  0, Dbg, "Number of Write Entries   -> %08lx\n", NumberOfWriteEntries );
    DebugTrace(  0, Dbg, "Write Entries             -> %08lx\n", WriteEntries );
    DebugTrace(  0, Dbg, "Record Type               -> %08lx\n", RecordType );
    DebugTrace(  0, Dbg, "Transaction Id            -> %08lx\n", TransactionId );
    DebugTrace(  0, Dbg, "ClientUndoNextLsn (Low)   -> %08lx\n", ClientUndoNextLsn.LowPart );
    DebugTrace(  0, Dbg, "ClientUndoNextLsn (High)  -> %08lx\n", ClientUndoNextLsn.HighPart );
    DebugTrace(  0, Dbg, "ClientPreviousLsn (Low)   -> %08lx\n", ClientPreviousLsn.LowPart );
    DebugTrace(  0, Dbg, "ClientPreviousLsn (High)  -> %08lx\n", ClientPreviousLsn.HighPart );
    DebugTrace(  0, Dbg, "UndoRequirement           -> %08lx\n", UndoRequirement );
    DebugTrace(  0, Dbg, "ForceToDisk               -> %04x\n", ForceToDisk );

    //
    //  We compute the size of this log record.
    //

    ThisWriteEntry = WriteEntries;

    RemainingLogBytes = 0;

    while (NumberOfWriteEntries--) {

        RemainingLogBytes += QuadAlign( ThisWriteEntry->ByteLength );

        ThisWriteEntry++;
    }

    OriginalLogBytes = RemainingLogBytes;

    ThisWriteEntry = WriteEntries;

    //
    //  Loop until we have the Lbcb and we know it is not part of
    //  a partial page transfer.  We need to make sure we have
    //  a Bcb for this page.
    //

    while (TRUE) {

        LogFileFull = LfsVerifyLogSpaceAvail( Lfcb,
                                              Lch,
                                              RemainingLogBytes,
                                              UndoRequirement,
                                              ForceToDisk );

        //
        //  We update the Lfcb so that we can start putting the log record into
        //  the top of the Lbcb active list.
        //

        LfsPrepareLfcbForLogRecord( Lfcb,
                                    RemainingLogBytes + Lfcb->RecordHeaderLength );

        ThisLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                      LBCB,
                                      ActiveLinks );

        //
        //  If there is a Bcb then we are golden.
        //

        if (ThisLbcb->LogPageBcb != NULL) { break; }

        //
        //  Otherwise we want to drop the Lfcb and wait for the IO to complete.
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
    }

    RemainingPageBytes = (ULONG)Lfcb->LogPageSize - (ULONG)ThisLbcb->BufferOffset;

    //
    //  Compute the Lsn starting in the next log buffer.
    //

    NextLsn.QuadPart = LfsComputeLsnFromLbcb( Lfcb, ThisLbcb );

    //
    //  We get a pointer to the log record header and the start of the
    //  log record in the pinned buffer.
    //

    RecordHeader = Add2Ptr( ThisLbcb->PageHeader,
                            (ULONG)ThisLbcb->BufferOffset,
                            PLFS_RECORD_HEADER );

    //
    //  We update the record header.
    //

    //
    //  Zero out the structure initially.
    //

    RtlZeroMemory( RecordHeader, Lfcb->RecordHeaderLength );

    //
    //  Update all the fields.
    //

    RecordHeader->ThisLsn = NextLsn;
    RecordHeader->ClientPreviousLsn = ClientPreviousLsn;
    RecordHeader->ClientUndoNextLsn = ClientUndoNextLsn;

    if (TransactionId != NULL) {
        RecordHeader->TransactionId = *TransactionId;
    }

    RecordHeader->ClientDataLength = RemainingLogBytes;
    RecordHeader->ClientId = Lch->ClientId;
    RecordHeader->RecordType = RecordType;

    //
    //  Check if this is a multi-page record.
    //

    if (RemainingLogBytes + Lfcb->RecordHeaderLength > RemainingPageBytes) {

        SetFlag( RecordHeader->Flags, LOG_RECORD_MULTI_PAGE );
    }

    RemainingPageBytes -= Lfcb->RecordHeaderLength;

    //
    //  Update the buffer position in the Lbcb
    //

    (ULONG)ThisLbcb->BufferOffset += Lfcb->RecordHeaderLength;
    HeaderAdjust = Lfcb->RecordHeaderLength;

    //
    //  Remember the values in the current write entry.
    //

    CurrentBuffer = ThisWriteEntry->Buffer;
    CurrentByteCount = ThisWriteEntry->ByteLength;

    PadBytes = (8 - (CurrentByteCount & ~(0xfffffff8))) & ~(0xfffffff8);

    //
    //  Continue to transfer bytes until all the client's data has
    //  been transferred.
    //

    while (RemainingLogBytes != 0) {

        PLFS_RECORD_PAGE_HEADER PageHeader;

        PageHeader = (PLFS_RECORD_PAGE_HEADER) ThisLbcb->PageHeader;

        //
        //  If the Lbcb is empty and we are about to store data into it we
        //  subtract the data size of the page from the available space.
        //  Update all the information we want to put in the header.
        //

        if (!FlagOn( ThisLbcb->LbcbFlags, LBCB_NOT_EMPTY )) {

            //
            //  We subtract this page from the available pages only if
            //  we are at the beginning of the page.  Otherwise this
            //  could be a reuse page.  In that case it has already
            //  been subtracted.
            //

            if ((ULONG)ThisLbcb->BufferOffset - HeaderAdjust == (ULONG)Lfcb->LogPageDataOffset) {


                Lfcb->CurrentAvailable = Lfcb->CurrentAvailable - Lfcb->ReservedLogPageSize;                           //**** xxSub( Lfcb->CurrentAvailable, Lfcb->ReservedLogPageSize );
            }

            InsertTailList( &Lfcb->LbcbWorkque, &ThisLbcb->WorkqueLinks );
            SetFlag( ThisLbcb->LbcbFlags, LBCB_NOT_EMPTY );
        }

        HeaderAdjust = 0;

        //
        //  Compute the number of transfer bytes.  Update the remaining
        //  page bytes, remaining log bytes and position in the write
        //  buffer array.  This routine also copies the bytes into the buffer.
        //

        LfsTransferLogBytes( ThisLbcb,
                             &ThisWriteEntry,
                             &CurrentBuffer,
                             &CurrentByteCount,
                             &PadBytes,
                             &RemainingPageBytes,
                             &RemainingLogBytes );

        //
        //  This log record ends on this page.  Update the fields for the
        //  ending Lsn.
        //

        if (RemainingLogBytes == 0) {

            SetFlag( ThisLbcb->Flags, LOG_PAGE_LOG_RECORD_END );
            ThisLbcb->LastEndLsn = NextLsn;

            if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

                PageHeader->Header.Packed.LastEndLsn = NextLsn;
                PageHeader->Header.Packed.NextRecordOffset = (USHORT)ThisLbcb->BufferOffset;
            }
        }

        //
        //  We are done with this page, update the fields in the page header.
        //

        if (RemainingPageBytes == 0
            || RemainingLogBytes == 0) {

            //
            //  We are done with this page.  Update the Lbcb and page header.
            //

            ThisLbcb->LastLsn = NextLsn;
            PageHeader->Copy.LastLsn = NextLsn;
            PageHeader->Flags = ThisLbcb->Flags;

            //
            //  We can't put any more log records on this page.  Remove
            //  it from the active queue.
            //

            if (RemainingPageBytes < Lfcb->RecordHeaderLength) {

                RemoveHeadList( &Lfcb->LbcbActive );
                ClearFlag( ThisLbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );

                //
                //  If there are more log bytes then get the next Lbcb.
                //

                if (RemainingLogBytes != 0) {

                    ThisLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                                  LBCB,
                                                  ActiveLinks );

                    RemainingPageBytes = (ULONG)Lfcb->LogPageSize
                                         - (ULONG)ThisLbcb->BufferOffset;
                }
            }
        }
    }

    *Lsn = NextLsn;

    Lfcb->RestartArea->CurrentLsn = NextLsn;

    Lfcb->RestartArea->LastLsnDataLength = OriginalLogBytes;

    ClearFlag( Lfcb->Flags, LFCB_NO_LAST_LSN );

    DebugTrace(  0, Dbg, "Lsn (Low)   -> %08lx\n", Lsn->LowPart );
    DebugTrace(  0, Dbg, "Lsn (High)  -> %08lx\n", Lsn->HighPart );
    DebugTrace( -1, Dbg, "LfsWriteLogRecordIntoLogPage:  Exit\n", 0 );

    return LogFileFull;
}


//
//  Local support routine.
//

VOID
LfsPrepareLfcbForLogRecord (
    IN OUT PLFCB Lfcb,
    IN ULONG RemainingLogBytes
    )

/*++

Routine Description:

    This routine is called to insure that the Lfcb has a Lbcb in the
    active queue to perform the next log record transfer.
    This condition is met when there is a least one buffer block and
    the log record data will fit entirely on this page or this buffer
    block contains no other data in the unpacked case.  For the packed
    case we just need to make sure that there are sufficient Lbcb's.

Arguments:

    Lfcb - File control block for this log file.

    RemainingLogBytes - The number of bytes remaining for this log record.

Return Value:

    None

--*/

{
    PLBCB ThisLbcb;
    ULONG RemainingPageBytes;
    PLIST_ENTRY LbcbLinks;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsPrepareLfcbForLogRecord:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb              -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "RemainingLogBytes -> %08lx\n", RemainingLogBytes );

    //
    //  If there is no Lbcb in the active queue, we don't check it for size.
    //

    if (!IsListEmpty( &Lfcb->LbcbActive )) {

        //
        //  If the log record won't fit in the remaining bytes of this page,
        //  we queue this log buffer.
        //

        ThisLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                      LBCB,
                                      ActiveLinks );

        RemainingPageBytes = (ULONG)Lfcb->LogPageSize
                             - (ULONG)ThisLbcb->BufferOffset;

        //
        //  This log page won't do if the remaining bytes won't hold the data
        //  unless this is the first log record in the page or we are packing
        //  the log file.
        //

        if (RemainingLogBytes > RemainingPageBytes
            && !FlagOn( Lfcb->Flags, LFCB_PACK_LOG )
            && (ULONG)ThisLbcb->BufferOffset != (ULONG)Lfcb->LogPageDataOffset) {

            RemoveHeadList( &Lfcb->LbcbActive );
            ClearFlag( ThisLbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );
        }
    }

    //
    //  We now make sure we can allocate enough Lbcb's for all of the log pages
    //  we will need.  We now include the bytes for the log record reader.
    //

    LbcbLinks = Lfcb->LbcbActive.Flink;

    while (TRUE) {

        //
        //  If the Lbcb link we have is the head of the list, we will need another
        //  Lbcb.
        //

        if (LbcbLinks == &Lfcb->LbcbActive) {

            ThisLbcb = LfsGetLbcb( Lfcb );

        } else {

            ThisLbcb = CONTAINING_RECORD( LbcbLinks,
                                          LBCB,
                                          ActiveLinks );
        }

        //
        //  Remember the bytes remaining on this page.  This will always be quad
        //  aligned.
        //

        RemainingPageBytes = (ULONG)Lfcb->LogPageSize - (ULONG)ThisLbcb->BufferOffset;

        if (RemainingPageBytes >= RemainingLogBytes) {

            break;
        }

        //
        //  Move to the next log record.
        //

        RemainingLogBytes -= RemainingPageBytes;

        LbcbLinks = ThisLbcb->ActiveLinks.Flink;
    }

    DebugTrace( -1, Dbg, "LfsPrepareLfcbForLogRecord:  Exit\n", 0 );

    return;
}


VOID
LfsTransferLogBytes (
    IN PLBCB Lbcb,
    IN OUT PLFS_WRITE_ENTRY *ThisWriteEntry,
    IN OUT PCHAR *CurrentBuffer,
    IN OUT PULONG CurrentByteCount,
    IN OUT PULONG PadBytes,
    IN OUT PULONG RemainingPageBytes,
    IN OUT PULONG RemainingLogBytes
    )

/*++

Routine Description:

    This routine is called to transfer the next block of bytes into
    a log page.  It is given a pointer to the current position in the
    current Lfs write entry and the number of bytes remaining on that
    log page.  It will transfer as many of the client's bytes from the
    current buffer that will fit and update various pointers.

Arguments:

    Lbcb - This is the buffer block for this log page.

    ThisWriteEntry - This is a pointer to a pointer to the current Lfs
                     write entry.

    CurrentBuffer - This is a pointer to a pointer to the current position
                    in the current write entry buffer.  If this points to a NULL
                    value it means to put zero bytes into the log.

    CurrentByteCount - This is a pointer to the number of bytes remaining
                       in the current buffer.

    PadBytes - This is a pointer to the number of padding byes for
        this write entry.

    RemainingPageBytes - This is pointer to the number of bytes remaining
                         in this page.

    RemainingLogBytes - This is the number of bytes remaining to transfer
                        for this log record.

Return Value:

    None

--*/

{
    PCHAR CurrentLogPagePosition;
    PCHAR CurrentClientPosition;

    ULONG TransferBytes;
    ULONG ThisPadBytes;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsTransferLogBytes:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lbcb                      -> %08lx\n", Lbcb );
    DebugTrace(  0, Dbg, "ThisWriteEntry            -> %08lx\n", *ThisWriteEntry );
    DebugTrace(  0, Dbg, "CurrentBuffer             -> %08lx\n", *CurrentBuffer );
    DebugTrace(  0, Dbg, "CurrentByteCount          -> %08lx\n", *CurrentByteCount );
    DebugTrace(  0, Dbg, "RemainingPageBytes        -> %08lx\n", *RemainingPageBytes );
    DebugTrace(  0, Dbg, "RemainingLogBytes         -> %08lx\n", *RemainingLogBytes );

    //
    //  Remember the current client buffer position and current position
    //  in log page.
    //

    CurrentLogPagePosition = Add2Ptr( Lbcb->PageHeader, (ULONG)Lbcb->BufferOffset, PCHAR );
    CurrentClientPosition = *CurrentBuffer;

    //
    //  The limiting factor is either the number of bytes remaining in a
    //  write entry or the number remaining in the log page.
    //

    if (*CurrentByteCount <= *RemainingPageBytes) {

        TransferBytes = *CurrentByteCount;

        ThisPadBytes = *PadBytes;

        if (*RemainingLogBytes != (*CurrentByteCount + *PadBytes) ) {

            (*ThisWriteEntry)++;

            *CurrentBuffer = (*ThisWriteEntry)->Buffer;
            *CurrentByteCount = (*ThisWriteEntry)->ByteLength;

            *PadBytes = (8 - (*CurrentByteCount & ~(0xfffffff8))) & ~(0xfffffff8);
        }

    } else {

        TransferBytes = *RemainingPageBytes;

        ThisPadBytes = 0;

        *CurrentByteCount -= TransferBytes;

        if (*CurrentBuffer != NULL) {

            *CurrentBuffer += TransferBytes;
        }
    }

    //
    //  Transfer the requested bytes.
    //

    if (CurrentClientPosition != NULL) {

        RtlCopyMemory( CurrentLogPagePosition, CurrentClientPosition, TransferBytes );

    } else {

        RtlZeroMemory( CurrentLogPagePosition, TransferBytes );
    }

    //
    //  Reduce the remaining page and log bytes by the transfer amount and
    //  move forward in the log page.
    //

    *RemainingLogBytes -= (TransferBytes + ThisPadBytes);
    *RemainingPageBytes -= (TransferBytes + ThisPadBytes);

    (ULONG)Lbcb->BufferOffset += (TransferBytes + ThisPadBytes);

    DebugTrace( -1, Dbg, "LfsTransferLogBytes:  Exit\n", 0 );

    return;
}
