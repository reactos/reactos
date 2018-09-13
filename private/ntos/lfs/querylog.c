/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    QueryLog.c

Abstract:

    This module implements the user routines which query for log records
    in a log file.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_QUERY)

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('QsfL')

VOID
LfsFindLogRecord (
    IN PLFCB Lfcb,
    IN OUT PLCB Lcb,
    IN LSN Lsn,
    OUT PLFS_RECORD_TYPE RecordType,
    OUT TRANSACTION_ID *TransactionId,
    OUT PLSN UndoNextLsn,
    OUT PLSN PreviousLsn,
    OUT PULONG BufferLength,
    OUT PVOID *Buffer
    );

BOOLEAN
LfsFindClientNextLsn (
    IN PLFCB Lfcb,
    IN PLCB Lcb,
    OUT PLSN Lsn
    );

BOOLEAN
LfsSearchForwardByClient (
    IN PLFCB Lfcb,
    IN OUT PLCB Lcb,
    OUT PLSN Lsn
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsFindClientNextLsn)
#pragma alloc_text(PAGE, LfsFindLogRecord)
#pragma alloc_text(PAGE, LfsQueryLastLsn)
#pragma alloc_text(PAGE, LfsReadLogRecord)
#pragma alloc_text(PAGE, LfsReadNextLogRecord)
#pragma alloc_text(PAGE, LfsSearchForwardByClient)
#pragma alloc_text(PAGE, LfsTerminateLogQuery)
#endif


VOID
LfsReadLogRecord (
    IN LFS_LOG_HANDLE LogHandle,
    IN LSN FirstLsn,
    IN LFS_CONTEXT_MODE ContextMode,
    OUT PLFS_LOG_CONTEXT Context,
    OUT PLFS_RECORD_TYPE RecordType,
    OUT TRANSACTION_ID *TransactionId,
    OUT PLSN UndoNextLsn,
    OUT PLSN PreviousLsn,
    OUT PULONG BufferLength,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine initiates the query operation.  It returns the log record
    in question and a context structure used by the Lfs to return related
    log records.  The caller specifies what mode of query to use.  He may
    walk backwards through the file by Undo records or all records for
    this client linked through the previous Lsn fields.  He may also look
    forwards through the file for all records for the issuing client.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    FirstLsn - Starting record for this query operation.

    ContextMode - Method of query.

    Context - Supplies the address to store a pointer to the Lfs created
              context structure.

    RecordType - Supplies the address to store the record type of this
                 log record.

    TransactionId - Supplies the address to store the transaction Id of
                    this log record.

    UndoNextLsn - Supplies the address to store the Undo Next Lsn for this
                  log record.

    PreviousLsn - Supplies the address to store the Previous Lsn for this
                  log record.

    BufferLength - This is the length of the log data.

    Buffer - This is a pointer to the start of the log data.

Return Value:

    None

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLFS_CLIENT_RECORD ClientRecord;

    PLCH Lch;

    PLFCB Lfcb;

    PLCB Lcb = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsReadLogRecord:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle        -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "First Lsn (Low)   -> %08lx\n", FirstLsn.LowPart );
    DebugTrace(  0, Dbg, "First Lsn (High)  -> %08lx\n", FirstLsn.HighPart );
    DebugTrace(  0, Dbg, "Context Mode      -> %08lx\n", ContextMode );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the context mode is valid.
    //

    switch (ContextMode) {

    case LfsContextUndoNext :
    case LfsContextPrevious :
    case LfsContextForward :

        break;

    default:

        DebugTrace( 0, Dbg, "Invalid context mode -> %08x\n", ContextMode );
        ExRaiseStatus( STATUS_INVALID_PARAMETER );
    }

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
            //  Check that the given Lsn is in the legal range for this client.
            //

            ClientRecord = Add2Ptr( Lfcb->ClientArray,
                                    Lch->ClientArrayByteOffset,
                                    PLFS_CLIENT_RECORD );

            if (!LfsVerifyClientLsnInRange( Lfcb, ClientRecord, FirstLsn )) {

                ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
            }

            //
            //  We can give up the Lfcb as we know the Lsn is within the file.
            //

            LfsReleaseLch( Lch );

            //
            //  Allocate and initialize a context structure.
            //

            LfsAllocateLcb( Lfcb, &Lcb );

            LfsInitializeLcb( Lcb,
                              Lch->ClientId,
                              ContextMode );

            //
            //  Find the log record indicated by the given Lsn.
            //

            LfsFindLogRecord( Lfcb,
                              Lcb,
                              FirstLsn,
                              RecordType,
                              TransactionId,
                              UndoNextLsn,
                              PreviousLsn,
                              BufferLength,
                              Buffer );

            //
            //  Update the client's arguments.
            //

            *Context = Lcb;
            Lcb = NULL;

        } finally {

            DebugUnwind( LfsReadLogRecord );

            //
            //  Release the log file control block if held.
            //

            LfsReleaseLch( Lch );

            //
            //  Deallocate the context block if an error occurred.
            //

            if (Lcb != NULL) {

                LfsDeallocateLcb( Lfcb, Lcb );
            }

            DebugTrace(  0, Dbg, "Context       -> %08lx\n", *Context );
            DebugTrace(  0, Dbg, "Buffer Length -> %08lx\n", *BufferLength );
            DebugTrace(  0, Dbg, "Buffer        -> %08lx\n", *Buffer );
            DebugTrace( -1, Dbg, "LfsReadLogRecord:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return;
}


BOOLEAN
LfsReadNextLogRecord (
    IN LFS_LOG_HANDLE LogHandle,
    IN OUT LFS_LOG_CONTEXT Context,
    OUT PLFS_RECORD_TYPE RecordType,
    OUT TRANSACTION_ID *TransactionId,
    OUT PLSN UndoNextLsn,
    OUT PLSN PreviousLsn,
    OUT PLSN Lsn,
    OUT PULONG BufferLength,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine is called to continue a query operation.  The Lfs uses
    private information stored in the context structure to determine the
    next log record to return to the caller.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    Context - Supplies the address to store a pointer to the Lfs created
              context structure.

    Lsn - Lsn for this log record.

    RecordType - Supplies the address to store the record type of this
                 log record.

    TransactionId - Supplies the address to store the transaction Id of
                    this log record.

    UndoNextLsn - Supplies the address to store the Undo Next Lsn for this
                  log record.

    PreviousLsn - Supplies the address to store the Previous Lsn for this
                  log record.

    BufferLength - This is the length of the log data.

    Buffer - This is a pointer to the start of the log data.

Return Value:

    None

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLCH Lch;

    PLFCB Lfcb;

    PLCB Lcb;

    BOOLEAN FoundNextLsn;

    BOOLEAN UnwindRememberLcbFields;
    PBCB UnwindRecordHeaderBcb;
    PLFS_RECORD_HEADER UnwindRecordHeader;
    PVOID UnwindCurrentLogRecord;
    BOOLEAN UnwindAuxilaryBuffer;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsReadNextLogRecord:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle    -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Context       -> %08lx\n", Context );

    FoundNextLsn = FALSE;

    UnwindRememberLcbFields = FALSE;

    Lch = (PLCH) LogHandle;
    Lcb = (PLCB) Context;

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
            //  Check that the context structure is valid.
            //

            LfsValidateLcb( Lcb, Lch );

            //
            //  Remember any context fields to be overwritten.
            //

            UnwindRememberLcbFields = TRUE;

            UnwindRecordHeaderBcb = Lcb->RecordHeaderBcb;
            Lcb->RecordHeaderBcb = NULL;

            UnwindRecordHeader = Lcb->RecordHeader;
            UnwindCurrentLogRecord = Lcb->CurrentLogRecord;

            UnwindAuxilaryBuffer = Lcb->AuxilaryBuffer;
            Lcb->AuxilaryBuffer = FALSE;

            //
            //  Find the next Lsn number based on the current Lsn number in
            //  the context block.
            //

            if (LfsFindClientNextLsn( Lfcb, Lcb, Lsn )) {

                //
                //  We can give up the Lfcb as we know the Lsn is within the file.
                //

                LfsReleaseLfcb( Lfcb );

                //
                //  Cleanup the context block so we can do the next search.
                //

                Lcb->CurrentLogRecord = NULL;
                Lcb->AuxilaryBuffer = FALSE;

                //
                //  Perform the work of getting the log record.
                //

                LfsFindLogRecord( Lfcb,
                                  Lcb,
                                  *Lsn,
                                  RecordType,
                                  TransactionId,
                                  UndoNextLsn,
                                  PreviousLsn,
                                  BufferLength,
                                  Buffer );

                FoundNextLsn = TRUE;
            }

        } finally {

            DebugUnwind( LfsReadNextLogRecord );

            //
            //  If we exited due to an error, we have to restore the context
            //  block.
            //

            if (UnwindRememberLcbFields) {

                if (AbnormalTermination()) {

                    //
                    //  If the record header in the context block is not
                    //  the same as we started with.  Then we unpin that
                    //  data.
                    //

                    if (Lcb->RecordHeaderBcb != NULL) {

                        CcUnpinData( Lcb->RecordHeaderBcb );

                    }

                    if (Lcb->CurrentLogRecord != NULL
                        && Lcb->AuxilaryBuffer == TRUE) {

                        LfsFreeSpanningBuffer( Lcb->CurrentLogRecord );
                    }

                    Lcb->RecordHeaderBcb = UnwindRecordHeaderBcb;
                    Lcb->RecordHeader = UnwindRecordHeader;
                    Lcb->CurrentLogRecord = UnwindCurrentLogRecord;
                    Lcb->AuxilaryBuffer = UnwindAuxilaryBuffer;

                //
                //  Otherwise, if we have successfully found the next Lsn,
                //  we free up any resources being held from the previous search.
                //

                } else if (FoundNextLsn ) {

                    if (UnwindRecordHeaderBcb != NULL) {

                        CcUnpinData( UnwindRecordHeaderBcb );
                    }

                    if (UnwindCurrentLogRecord != NULL
                        && UnwindAuxilaryBuffer == TRUE) {

                        LfsFreeSpanningBuffer( UnwindCurrentLogRecord );
                    }

                //
                //  Restore the Bcb and auxilary buffer field for the final
                //  cleanup.
                //

                } else {

                    if (UnwindRecordHeaderBcb != NULL) {

                        if (Lcb->RecordHeaderBcb != NULL) {

                            CcUnpinData( UnwindRecordHeaderBcb );

                        } else {

                            Lcb->RecordHeaderBcb = UnwindRecordHeaderBcb;
                        }
                    }

                    if (UnwindAuxilaryBuffer) {

                        if (Lcb->CurrentLogRecord == UnwindCurrentLogRecord) {

                            Lcb->AuxilaryBuffer = TRUE;

                        } else {

                            LfsFreeSpanningBuffer( UnwindCurrentLogRecord );
                        }
                    }
                }
            }

            //
            //  Release the log file control block if held.
            //

            LfsReleaseLch( Lch );

            DebugTrace(  0, Dbg, "Lsn (Low)     -> %08lx\n", Lsn->LowPart );
            DebugTrace(  0, Dbg, "Lsn (High)    -> %08lx\n", Lsn->HighPart );
            DebugTrace(  0, Dbg, "Buffer Length -> %08lx\n", *BufferLength );
            DebugTrace(  0, Dbg, "Buffer        -> %08lx\n", *Buffer );
            DebugTrace( -1, Dbg, "LfsReadNextLogRecord:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return FoundNextLsn;
}


VOID
LfsTerminateLogQuery (
    IN LFS_LOG_HANDLE LogHandle,
    IN LFS_LOG_CONTEXT Context
    )

/*++

Routine Description:

    This routine is called when a client has completed his query operation
    and wishes to deallocate any resources acquired by the Lfs to
    perform the log file query.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    Context - Supplies the address to store a pointer to the Lfs created
              context structure.

Return Value:

    None

--*/

{
    PLCH Lch;
    PLCB Lcb;

    PLFCB Lfcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsTerminateLogQuery:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle    -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Context       -> %08lx\n", Context );

    Lch = (PLCH) LogHandle;
    Lcb = (PLCB) Context;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );

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

            try_return( NOTHING );
        }

        //
        //  Check that the client Id is valid.
        //

        LfsValidateClientId( Lfcb, Lch );

        //
        //  Check that the context structure is valid.
        //

        LfsValidateLcb( Lcb, Lch );

        //
        //  Deallocate the context block.
        //

        LfsDeallocateLcb( Lfcb, Lcb );

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( LfsTerminateLogQuery );

        //
        //  Release the Lfcb if acquired.
        //

        LfsReleaseLch( Lch );

        DebugTrace( -1, Dbg, "LfsTerminateLogQuery:  Exit\n", 0 );
    }

    return;
}


LSN
LfsQueryLastLsn (
    IN LFS_LOG_HANDLE LogHandle
    )

/*++

Routine Description:

    This routine will return the most recent Lsn for this log record.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

Return Value:

    LSN - This is the last Lsn assigned in this log file.

--*/

{
    PLCH Lch;

    PLFCB Lfcb;

    LSN LastLsn;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsQueryLastLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle    -> %08lx\n", LogHandle );

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    LfsValidateLch( Lch );

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
        //  Copy the last Lsn out of the Lfcb.  If the last Lsn is
        //  does not correspond to a log record, we will return the
        //  zero Lsn.
        //

        if (FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN )) {

            LastLsn = LfsZeroLsn;

        } else {

            LastLsn = Lfcb->RestartArea->CurrentLsn;
        }

    } finally {

        DebugUnwind( LfsQueryLastLsn );

        //
        //  Release the Lfcb if acquired.
        //

        LfsReleaseLch( Lch );

        DebugTrace(  0, Dbg, "Last Lsn (Low)    -> %08lx\n", LastLsn.LowPart );
        DebugTrace(  0, Dbg, "Last Lsn (High)   -> %08lx\n", LastLsn.HighPart );
        DebugTrace( -1, Dbg, "LfsQueryLastLsn:  Exit\n", 0 );
    }

    return LastLsn;
}


//
//  Local support routine.
//

VOID
LfsFindLogRecord (
    IN PLFCB Lfcb,
    IN OUT PLCB Lcb,
    IN LSN Lsn,
    OUT PLFS_RECORD_TYPE RecordType,
    OUT TRANSACTION_ID *TransactionId,
    OUT PLSN UndoNextLsn,
    OUT PLSN PreviousLsn,
    OUT PULONG BufferLength,
    OUT PVOID *Buffer
    )

/*++

Routine Description:

    This routine is called recover a log record for a client.

Arguments:

    Lfcb - Log file control block for this file.

    Lcb - Pointer to the context block to update.

    Lsn - This is the Lsn for the log record.

    RecordType - Supplies the address to store the record type of this
                 log record.

    TransactionId - Supplies the address to store the transaction Id of
                    this log record.

    UndoNextLsn - Supplies the address to store the Undo Next Lsn for this
                  log record.

    PreviousLsn - Supplies the address to store the Previous Lsn for this
                  log record.

    BufferLength - Pointer to address to store the length in bytes of the
                   log record.

    Buffer - Pointer to store the address where the log record data begins.

Return Value:

    None

--*/

{
    PCHAR NewBuffer;
    BOOLEAN UsaError;
    LONGLONG LogRecordLength;
    ULONG PageOffset;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFindLogRecord:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb          -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Context Block -> %08lx\n", Lcb );
    DebugTrace(  0, Dbg, "Lsn (Low)     -> %08lx\n", Lsn.LowPart );

    NewBuffer = NULL;

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Map the record header for this Lsn if we haven't already.
        //

        if (Lcb->RecordHeader == NULL) {

            LfsPinOrMapLogRecordHeader( Lfcb,
                                        Lsn,
                                        FALSE,
                                        FALSE,
                                        &UsaError,
                                        &Lcb->RecordHeader,
                                        &Lcb->RecordHeaderBcb );
        }

        //
        //  We now have the log record desired.  If the Lsn in the
        //  log record doesn't match the desired Lsn then the disk is
        //  corrupt.
        //

        if ( Lsn.QuadPart != Lcb->RecordHeader->ThisLsn.QuadPart ) {                                                   //**** xxNeq( Lsn, Lcb->RecordHeader->ThisLsn )

            ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
        }

        //
        //  Check that the length field isn't greater than the total available space
        //  in the log file.
        //

        LogRecordLength = Lcb->RecordHeader->ClientDataLength + Lfcb->RecordHeaderLength;                              //**** xxFromUlong( Lcb->RecordHeader->ClientDataLength + Lfcb->RecordHeaderLength );

        if ( LogRecordLength >= Lfcb->TotalAvailable ) {                                                               //**** xxGeq( LogRecordLength, Lfcb->TotalAvailable )

            ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
        }

        //
        //  If the entire log record is on this log page, put a pointer to
        //  the log record in the context block.
        //

        if (!FlagOn( Lcb->RecordHeader->Flags, LOG_RECORD_MULTI_PAGE )) {

            //
            //  If client size indicates that we have to go beyond the end of the current
            //  page, we raise an error.
            //

            PageOffset = LfsLsnToPageOffset( Lfcb, Lsn );

            if ((PageOffset + Lcb->RecordHeader->ClientDataLength + Lfcb->RecordHeaderLength)
                > (ULONG)Lfcb->LogPageSize) {

                ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
            }

            Lcb->CurrentLogRecord = Add2Ptr( Lcb->RecordHeader, LFS_RECORD_HEADER_SIZE, PVOID );
            Lcb->AuxilaryBuffer = FALSE;

        //
        //  Else we copy the data and remember that we allocated a buffer.
        //

        } else {

            NewBuffer = LfsAllocateSpanningBuffer( Lfcb, Lcb->RecordHeader->ClientDataLength );

            //
            //  Copy the data into the buffer returned.
            //

            LfsCopyReadLogRecord( Lfcb,
                                  Lcb->RecordHeader,
                                  NewBuffer );

            Lcb->CurrentLogRecord = NewBuffer;

            Lcb->AuxilaryBuffer = TRUE;

            NewBuffer = NULL;
        }

        //
        //  We need to update the caller's parameters and the context block.
        //

        *RecordType = Lcb->RecordHeader->RecordType;
        *TransactionId = Lcb->RecordHeader->TransactionId;

        *UndoNextLsn = Lcb->RecordHeader->ClientUndoNextLsn;
        *PreviousLsn = Lcb->RecordHeader->ClientPreviousLsn;

        *Buffer = Lcb->CurrentLogRecord;
        *BufferLength = Lcb->RecordHeader->ClientDataLength;

    } finally {

        DebugUnwind( LfsFindLogRecord );

        //
        //  If an error occurred we unpin the record header and the log
        //  We also free the buffer if allocated by us.
        //

        if (NewBuffer != NULL) {

            LfsFreeSpanningBuffer( NewBuffer );
        }

        DebugTrace(  0, Dbg, "Buffer Length -> %08lx\n", *BufferLength );
        DebugTrace(  0, Dbg, "Buffer        -> %08lx\n", *Buffer );
        DebugTrace( -1, Dbg, "LfsFindLogRecord:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine.
//

BOOLEAN
LfsFindClientNextLsn (
    IN PLFCB Lfcb,
    IN PLCB Lcb,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine will attempt to find the next Lsn to return to a client
    based on the context mode.

Arguments:

    Lfcb - File control block for this log file.

    Lcb - Pointer to the context block for this query operation.

    Lsn - Pointer to store the Lsn found (if any)

Return Value:

    BOOLEAN - TRUE if an Lsn is found, FALSE otherwise.

--*/

{
    LSN NextLsn;
    BOOLEAN NextLsnFound;

    PLFS_CLIENT_RECORD ClientRecord;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFindClientNextLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lcb  -> %08lx\n", Lcb );

    ClientRecord = Lfcb->ClientArray + Lcb->ClientId.ClientIndex;

    //
    //  The context block has the last Lsn returned.  If the user wanted
    //  one of the Lsn's in that log header then our job is simple.
    //

    switch (Lcb->ContextMode) {

    case LfsContextUndoNext:
    case LfsContextPrevious:

        NextLsn = (Lcb->ContextMode == LfsContextUndoNext
                   ? Lcb->RecordHeader->ClientUndoNextLsn
                   : Lcb->RecordHeader->ClientPreviousLsn);

        if ( NextLsn.QuadPart == 0 ) {                                                                                 //**** xxEqlZero( NextLsn )

            NextLsnFound = FALSE;

        } else if (LfsVerifyClientLsnInRange( Lfcb, ClientRecord, NextLsn )) {

            BOOLEAN UsaError;

            LfsPinOrMapLogRecordHeader( Lfcb,
                                        NextLsn,
                                        FALSE,
                                        FALSE,
                                        &UsaError,
                                        &Lcb->RecordHeader,
                                        &Lcb->RecordHeaderBcb );

            NextLsnFound = TRUE;

        } else {

            NextLsnFound = FALSE;
        }

        break;

    case LfsContextForward:

        //
        //  We search forward for the next log record for this client.
        //

        NextLsnFound = LfsSearchForwardByClient( Lfcb, Lcb, &NextLsn );
        break;

    default:

        NextLsnFound = FALSE;
        break;
    }

    if (NextLsnFound) {

        *Lsn = NextLsn;
    }

    DebugTrace(  0, Dbg, "NextLsn (Low)     -> %08lx\n", NextLsn.LowPart );
    DebugTrace(  0, Dbg, "NextLsn (High)    -> %08lx\n", NextLsn.HighPart );
    DebugTrace( -1, Dbg, "LfsFindClientNextLsn:  Exit -> %08x\n", NextLsnFound );

    return NextLsnFound;
}


//
//  Local support routine.
//

BOOLEAN
LfsSearchForwardByClient (
    IN PLFCB Lfcb,
    IN OUT PLCB Lcb,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine will attempt to find the next Lsn for this client by searching
    forward in the file, looking for a match.

Arguments:

    Lfcb - Pointer to the file control block for this log file.

    Lcb - Pointer to the context block for this query operation.

    Lsn - Points to the location to store the next Lsn if found.

Return Value:

    BOOLEAN - TRUE if another Lsn for this client is found.  FALSE otherwise.

--*/

{
    PLFS_RECORD_HEADER CurrentRecordHeader;
    PBCB CurrentBcb;

    BOOLEAN FoundNextLsn;

    LSN CurrentLsn;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsSearchForwardByClient:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lcb  -> %08lx\n", Lcb );

    //
    //  The log record header is in the log context
    //  block.  We set the current Bcb to NULL so that we don't
    //  unpin the log record in the context block until we're sure
    //  of success.
    //

    CurrentRecordHeader = Lcb->RecordHeader;

    CurrentBcb = NULL;

    //
    //  We use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  We assume we won't find another Lsn.
        //

        FoundNextLsn = FALSE;

        //
        //  Loop as long as another Lsn can be found.
        //

        while (LfsFindNextLsn( Lfcb, CurrentRecordHeader, &CurrentLsn )) {

            BOOLEAN UsaError;

            //
            //  Unpin the previous log record header.
            //

            if (CurrentBcb != NULL) {

                CcUnpinData( CurrentBcb );
                CurrentBcb = NULL;
            }

            //
            //  Pin the log record header for this Lsn.
            //

            LfsPinOrMapLogRecordHeader( Lfcb,
                                        CurrentLsn,
                                        FALSE,
                                        FALSE,
                                        &UsaError,
                                        &CurrentRecordHeader,
                                        &CurrentBcb );

            //
            //  If the client values match, then we update the
            //  context block and exit.
            //

            if (LfsClientIdMatch( &CurrentRecordHeader->ClientId,
                                  &Lcb->ClientId )
                && CurrentRecordHeader->RecordType == LfsClientRecord) {

                //
                //  We remember this one.
                //

                Lcb->RecordHeader = CurrentRecordHeader;
                Lcb->RecordHeaderBcb = CurrentBcb;

                CurrentBcb = NULL;
                FoundNextLsn = TRUE;

                *Lsn = CurrentLsn;
                break;
            }
        }

    } finally {

        DebugUnwind( LfsSearchForwardByClient );

        //
        //  Unpin any log record headers still pinned for no reason.
        //

        if (CurrentBcb != NULL) {

            CcUnpinData( CurrentBcb );
        }

        DebugTrace(  0, Dbg, "NextLsn (Low)     -> %08lx\n", Lsn->LowPart );
        DebugTrace(  0, Dbg, "NextLsn (High)    -> %08lx\n", Lsn->HighPart );
        DebugTrace( -1, Dbg, "LfsSearchForwardByClient:  Exit -> %08x\n", FoundNextLsn );
    }

    return FoundNextLsn;
}
