/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Write.c

Abstract:

    This module implements the user routines which write log records into
    or flush portions of the log file.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_WRITE)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsCheckWriteRange)
#pragma alloc_text(PAGE, LfsFlushToLsn)
#pragma alloc_text(PAGE, LfsForceWrite)
#pragma alloc_text(PAGE, LfsWrite)
#endif


BOOLEAN
LfsWrite (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId OPTIONAL,
    IN LSN UndoNextLsn,
    IN LSN PreviousLsn,
    IN LONG UndoRequirement,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine is called by a client to write a log record to the log file.
    The log record is lazy written and is not guaranteed to be on the disk
    until a subsequent LfsForceWrie or LfsWriteRestartArea or until
    an LfsFlushtoLsn is issued withan Lsn greater-than or equal to the Lsn
    returned from this service.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    NumberOfWriteEntries - Number of components of the log record.

    WriteEntries - Pointer to an array of write entries.

    RecordType - Lfs defined type for this log record.

    TransactionId - Id value used to group log records by complete transaction.

    UndoNextLsn - Lsn of a previous log record which needs to be undone in
                  the event of a client restart.

    PreviousLsn - Lsn of the immediately previous log record for this client.

    Lsn - Lsn to be associated with this log record.

Return Value:

    BOOLEAN - Advisory, TRUE indicates that less than 1/4 of the log file is
        available.

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN LogFileFull = FALSE;
    PLCH Lch;

    PLFCB Lfcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsWrite:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle                -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "NumberOfWriteEntries      -> %08lx\n", NumberOfWriteEntries );
    DebugTrace(  0, Dbg, "WriteEntries              -> %08lx\n", WriteEntries );
    DebugTrace(  0, Dbg, "Record Type               -> %08lx\n", RecordType );
    DebugTrace(  0, Dbg, "Transaction Id            -> %08lx\n", TransactionId );
    DebugTrace(  0, Dbg, "UndoNextLsn (Low)         -> %08lx\n", UndoNextLsn.LowPart );
    DebugTrace(  0, Dbg, "UndoNextLsn (High)        -> %08lx\n", UndoNextLsn.HighPart );
    DebugTrace(  0, Dbg, "PreviousLsn (Low)         -> %08lx\n", PreviousLsn.LowPart );
    DebugTrace(  0, Dbg, "PreviousLsn (High)        -> %08lx\n", PreviousLsn.HighPart );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );

    //
    //  Use a try-except to catch errors.
    //

    try {

        //
        //  Use a try-finally to facilitate cleanup.
        //

        try {

            //
            //  Acquire the log file control block for this log file.
            //

            LfsAcquireLch( Lch );
            Lfcb = Lch->Lfcb;

            //
            //  If the Log file has been closed then refuse access.
            //

            if (Lfcb == NULL) {

                ExRaiseStatus( STATUS_ACCESS_DENIED );
            }

            //
            //  Check that the client Id is valid.
            //

            LfsValidateClientId( Lfcb, Lch );

            //
            //  Write the log record.
            //

            LogFileFull = LfsWriteLogRecordIntoLogPage( Lfcb,
                                                        Lch,
                                                        NumberOfWriteEntries,
                                                        WriteEntries,
                                                        RecordType,
                                                        TransactionId,
                                                        UndoNextLsn,
                                                        PreviousLsn,
                                                        UndoRequirement,
                                                        FALSE,
                                                        Lsn );

        } finally {

            DebugUnwind( LfsWrite );

            //
            //  Release the log file control block if held.
            //

            LfsReleaseLch( Lch );

            DebugTrace(  0, Dbg, "Lsn (Low)   -> %08lx\n", Lsn->LowPart );
            DebugTrace(  0, Dbg, "Lsn (High)  -> %08lx\n", Lsn->HighPart );
            DebugTrace( -1, Dbg, "LfsWrite:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return LogFileFull;
}


BOOLEAN
LfsForceWrite (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberOfWriteEntries,
    IN PLFS_WRITE_ENTRY WriteEntries,
    IN LFS_RECORD_TYPE RecordType,
    IN TRANSACTION_ID *TransactionId,
    IN LSN UndoNextLsn,
    IN LSN PreviousLsn,
    IN LONG UndoRequirement,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine is called by a client to write a log record to the log file.
    This is idendical to LfsWrite except that on return the log record is
    guaranteed to be on disk.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    NumberOfWriteEntries - Number of components of the log record.

    WriteEntries - Pointer to an array of write entries.

    RecordType - Lfs defined type for this log record.

    TransactionId - Id value used to group log records by complete transaction.

    UndoNextLsn - Lsn of a previous log record which needs to be undone in
                  the event of a client restart.

    PreviousLsn - Lsn of the immediately previous log record for this client.

    Lsn - Lsn to be associated with this log record.

Return Value:

    BOOLEAN - Advisory, TRUE indicates that less than 1/4 of the log file is
        available.

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLCH Lch;

    PLFCB Lfcb;
    BOOLEAN LogFileFull = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsForceWrite:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle                -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "NumberOfWriteEntries      -> %08lx\n", NumberOfWriteEntries );
    DebugTrace(  0, Dbg, "WriteEntries              -> %08lx\n", WriteEntries );
    DebugTrace(  0, Dbg, "Record Type               -> %08lx\n", RecordType );
    DebugTrace(  0, Dbg, "Transaction Id            -> %08lx\n", TransactionId );
    DebugTrace(  0, Dbg, "UndoNextLsn (Low)         -> %08lx\n", UndoNextLsn.LowPart );
    DebugTrace(  0, Dbg, "UndoNextLsn (High)        -> %08lx\n", UndoNextLsn.HighPart );
    DebugTrace(  0, Dbg, "PreviousLsn (Low)         -> %08lx\n", PreviousLsn.LowPart );
    DebugTrace(  0, Dbg, "PreviousLsn (High)        -> %08lx\n", PreviousLsn.HighPart );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );

    //
    //  Use a try-except to catch errors.
    //

    try {

        //
        //  Use a try-finally to facilitate cleanup.
        //

        try {

            //
            //  Acquire the log file control block for this log file.
            //

            LfsAcquireLch( Lch );
            Lfcb = Lch->Lfcb;

            //
            //  If the Log file has been closed then refuse access.
            //

            if (Lfcb == NULL) {

                ExRaiseStatus( STATUS_ACCESS_DENIED );
            }

            //
            //  Check that the client Id is valid.
            //

            LfsValidateClientId( Lfcb, Lch );

            //
            //  Write the log record.
            //

            LogFileFull = LfsWriteLogRecordIntoLogPage( Lfcb,
                                                        Lch,
                                                        NumberOfWriteEntries,
                                                        WriteEntries,
                                                        RecordType,
                                                        TransactionId,
                                                        UndoNextLsn,
                                                        PreviousLsn,
                                                        UndoRequirement,
                                                        TRUE,
                                                        Lsn );

            //
            //  The call to add this lbcb to the workque is guaranteed to release
            //  the Lfcb if this thread may do the Io.
            //

            LfsFlushToLsnPriv( Lfcb, *Lsn );

        } finally {

            DebugUnwind( LfsForceWrite );

            //
            //  Release the log file control block if held.
            //

            LfsReleaseLch( Lch );

            DebugTrace(  0, Dbg, "Lsn (Low)   -> %08lx\n", Lsn->LowPart );
            DebugTrace(  0, Dbg, "Lsn (High)  -> %08lx\n", Lsn->HighPart );
            DebugTrace( -1, Dbg, "LfsForceWrite:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return LogFileFull;
}


VOID
LfsFlushToLsn (
    IN LFS_LOG_HANDLE LogHandle,
    IN LSN Lsn
    )

/*++

Routine Description:

    This routine is called by a client to insure that all log records
    to a certain point have been flushed to the file.  This is done by
    checking if the desired Lsn has even been written at all.  If so we
    check if it has been flushed to the file.  If not, we simply write
    the current restart area to the disk.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    Lsn - This is the Lsn that must be on the disk on return from this
          routine.

Return Value:

    None

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLCH Lch;

    PLFCB Lfcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFlushToLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle        -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Lsn (Low)         -> %08lx\n", Lsn.LowPart );
    DebugTrace(  0, Dbg, "Lsn (High)        -> %08lx\n", Lsn.HighPart );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );

    //
    //  Use a try-except to catch errors.
    //

    try {

        //
        //  Use a try-finally to facilitate cleanup.
        //

        try {

            //
            //  Acquire the log file control block for this log file.
            //

            LfsAcquireLch( Lch );
            Lfcb = Lch->Lfcb;

            //
            //  If the log file has been closed we will assume the Lsn has been flushed.
            //

            if (Lfcb != NULL) {

                //
                //  Check that the client Id is valid.
                //

                LfsValidateClientId( Lfcb, Lch );

                //
                //  Call our common routine to perform the work.
                //

                LfsFlushToLsnPriv( Lfcb, Lsn );
            }

        } finally {

            DebugUnwind( LfsFlushToLsn );

            //
            //  Release the log file control block if held.
            //

            LfsReleaseLch( Lch );

            DebugTrace( -1, Dbg, "LfsFlushToLsn:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return;
}


VOID
LfsCheckWriteRange (
    IN PLFS_WRITE_DATA WriteData,
    IN OUT PLONGLONG FlushOffset,
    IN OUT PULONG FlushLength
    )

/*++

Routine Description:

    This routine is called Ntfs to Lfs when a flush occurs.  This will give Lfs a chance
    to trim the amount of the flush.  Lfs can then use a 4K log record page size
    for all systems (Intel and Alpha).

    This routine will trim the size of the IO request to the value stored in the
    Lfcb for this volume.  We will also redirty the second half of the page if
    we have begun writing log records into it.

Arguments:

    WriteData - This is the data in the user's data structure which is maintained
        by Lfs to describe the current writes.

    FlushOffset - On input this is the start of the flush passed to Ntfs from MM.
        On output this is the start of the actual range to flush.

    FlushLength - On input this is the length of the flush from the given FlushOffset.
        On output this is the length of the flush from the possibly modified FlushOffset.

Return Value:

    None

--*/

{
    PLIST_ENTRY Links;
    PLFCB Lfcb;
    PLFCB NextLfcb;
    PAGED_CODE();

    //
    //  Find the correct Lfcb for this request.
    //

    Lfcb = WriteData->Lfcb;

    //
    //  Trim the write if not a system page size.
    //

    if (PAGE_SIZE != Lfcb->SystemPageSize) {

        //
        //  Check if we are trimming before the write.
        //

        if (*FlushOffset < WriteData->FileOffset) {

            *FlushLength -= (ULONG) (WriteData->FileOffset - *FlushOffset);
            *FlushOffset = WriteData->FileOffset;
        }

        //
        //  Check that we aren't flushing too much.
        //

        if (*FlushOffset + *FlushLength > WriteData->FileOffset + WriteData->Length) {

            *FlushLength = (ULONG) (WriteData->FileOffset + WriteData->Length - *FlushOffset);
        }

        //
        //  Finally check if we have to redirty a page.
        //

        if ((Lfcb->PageToDirty != NULL) &&
            (*FlushLength + *FlushOffset == Lfcb->PageToDirty->FileOffset)) {

            *((PULONG) (Lfcb->PageToDirty->PageHeader)) = LFS_SIGNATURE_RECORD_PAGE_ULONG;
        }
    }

    return;
}


