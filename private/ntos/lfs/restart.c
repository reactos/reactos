/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Restart.c

Abstract:

    This module implements the routines which access the client restart
    areas.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RESTART)

VOID
LfsSetBaseLsnPriv (
    IN PLFCB Lfcb,
    IN PLFS_CLIENT_RECORD ClientRecord,
    IN LSN BaseLsn
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsReadRestartArea)
#pragma alloc_text(PAGE, LfsSetBaseLsn)
#pragma alloc_text(PAGE, LfsSetBaseLsnPriv)
#pragma alloc_text(PAGE, LfsWriteRestartArea)
#endif


NTSTATUS
LfsReadRestartArea (
    IN LFS_LOG_HANDLE LogHandle,
    IN OUT PULONG BufferLength,
    IN PVOID Buffer,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine is called by the client when he wishes to read his restart
    area in the log file.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    BufferLength - On entry it is the length of the user buffer.  On exit
                   it is the size of the data stored in the buffer.

    Buffer - Pointer to the buffer where the client restart data is to be
             copied.

    Lsn - This is the Lsn for client restart area.

Return Value:

    None

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    BOOLEAN UsaError;

    PLCH Lch;

    PLFS_CLIENT_RECORD ClientRecord;

    PLFS_RECORD_HEADER RecordHeader;
    PBCB RecordHeaderBcb;

    PLFCB Lfcb;
    NTSTATUS RetStatus = STATUS_SUCCESS;


    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsReadRestartArea:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle    -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Buffer Length -> %08lx\n", *BufferLength );
    DebugTrace(  0, Dbg, "Buffer        -> %08lx\n", Buffer );

    RecordHeaderBcb = NULL;

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

            ClientRecord = Add2Ptr( Lfcb->ClientArray,
                                    Lch->ClientArrayByteOffset,
                                    PLFS_CLIENT_RECORD );

            //
            //  If the client doesn't have a restart area, go ahead and exit
            //  now.
            //

            if ( ClientRecord->ClientRestartLsn.QuadPart == 0 ) {                                                      //**** xxEqlZero( ClientRecord->ClientRestartLsn )

                //
                //  We show there is no restart area by returning a length
                //  of zero.  We also set the Lsn value to zero so that
                //  we can catch it if the user tries to use the Lsn.
                //

                DebugTrace( 0, Dbg, "No client restart area exists\n", 0 );

                *BufferLength = 0;
                *Lsn = LfsZeroLsn;

                try_return( NOTHING );
            }

            //
            //  Release the Lfcb as we won't be modifying any fields in it.
            //

            LfsReleaseLfcb( Lfcb );

            //
            //  Pin the log record for this Lsn.
            //

            LfsPinOrMapLogRecordHeader( Lfcb,
                                        ClientRecord->ClientRestartLsn,
                                        FALSE,
                                        FALSE,
                                        &UsaError,
                                        &RecordHeader,
                                        &RecordHeaderBcb );

            //
            //  If the Lsn values don't match, then the disk is corrupt.
            //

            if ( ClientRecord->ClientRestartLsn.QuadPart != RecordHeader->ThisLsn.QuadPart ) {                         //**** xxNeq( ClientRecord->ClientRestartLsn, RecordHeader->ThisLsn )

                ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
            }


            //
            //  Check that the user's buffer is big enough to hold the restart
            //  data.  We raise an error status for this error.
            //

            if (RecordHeader->ClientDataLength > *BufferLength) {

                DebugTrace( 0, Dbg, "Client buffer is too small\n", 0 );
                *BufferLength = RecordHeader->ClientDataLength;
                *Lsn = LfsZeroLsn;
                try_return( RetStatus = STATUS_BUFFER_TOO_SMALL );
            }


            //
            //  Use the cache manager to copy the data into the user's buffer.
            //

            LfsCopyReadLogRecord( Lfcb,
                                  RecordHeader,
                                  Buffer );

            //
            //  Pass the length and the Lsn of the restart area back to the
            //  caller.
            //

            *BufferLength = RecordHeader->ClientDataLength;
            *Lsn = RecordHeader->ThisLsn;

        try_exit: NOTHING;
        } finally {

            DebugUnwind( LfsReadRestartArea );

            //
            //  Release the log file control block if held.
            //

            LfsReleaseLch( Lch );

            //
            //  Unpin the log record header for the client restart if pinned.
            //

            if (RecordHeaderBcb != NULL) {

                CcUnpinData( RecordHeaderBcb );
            }

            DebugTrace(  0, Dbg, "Lsn (Low)     -> %08lx\n", Lsn->LowPart );
            DebugTrace(  0, Dbg, "Lsn (High)    -> %08lx\n", Lsn->HighPart );
            DebugTrace(  0, Dbg, "Buffer Length -> %08lx\n", *BufferLength );
            DebugTrace( -1, Dbg, "LfsReadRestartArea:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return RetStatus;
}


VOID
LfsWriteRestartArea (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG BufferLength,
    IN PVOID Buffer,
    OUT PLSN Lsn
    )

/*++

Routine Description:

    This routine is called by the client to write a restart area to the
    disk.  This routine will not return to the caller until the client
    restart area and all prior Lsn's have been flushed and the Lfs
    restart area on the disk has been updated.

    On return, all log records up to and including 'Lsn' have been flushed
    to the disk.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    BufferLength - On entry it is the length of the user buffer.

    Buffer - Pointer to the buffer where the client restart data resides.

    Lsn - This is the Lsn for this write operation.  On input, this will be the
          new Base Lsn for this client.

          ****  This was used to prevent adding an interface change to
                the Beta release.

Return Value:

    None

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLCH Lch;

    PLFCB Lfcb;

    PLFS_CLIENT_RECORD ClientRecord;

    LFS_WRITE_ENTRY WriteEntry;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsWriteRestartArea:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle    -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Buffer Length -> %08lx\n", BufferLength );
    DebugTrace(  0, Dbg, "Buffer        -> %08lx\n", Buffer );

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

            ClientRecord = Add2Ptr( Lfcb->ClientArray,
                                    Lch->ClientArrayByteOffset,
                                    PLFS_CLIENT_RECORD );

            //
            //  Go ahead and update the Base Lsn in the client area if the value
            //  given is not zero.
            //

            if ( Lsn->QuadPart != 0 ) {                                                                                //**** xxNeqZero( *Lsn )

                LfsSetBaseLsnPriv( Lfcb,
                                   ClientRecord,
                                   *Lsn );
            }

            //
            //  Write this restart area as a log record into a log page.
            //

            WriteEntry.Buffer = Buffer;
            WriteEntry.ByteLength = BufferLength;

            LfsWriteLogRecordIntoLogPage( Lfcb,
                                          Lch,
                                          1,
                                          &WriteEntry,
                                          LfsClientRestart,
                                          NULL,
                                          LfsZeroLsn,
                                          LfsZeroLsn,
                                          0,
                                          TRUE,
                                          Lsn );

            //
            //  Update the restart area for the client.
            //

            ClientRecord->ClientRestartLsn = *Lsn;

            //
            //  Write the restart area to the disk.
            //

            LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, TRUE );

        } finally {

            DebugUnwind( LfsWriteRestartArea );

            //
            //  Release the log file control block if still held.
            //

            LfsReleaseLch( Lch );

            DebugTrace(  0, Dbg, "Lsn (Low)     -> %08lx\n", Lsn->LowPart );
            DebugTrace(  0, Dbg, "Log (High)    -> %08lx\n", Lsn->HighPart );
            DebugTrace( -1, Dbg, "LfsWriteRestartArea:  Exit\n", 0 );
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
LfsSetBaseLsn (
    IN LFS_LOG_HANDLE LogHandle,
    IN LSN BaseLsn
    )

/*++

Routine Description:

    This routine is called by the client to notify the log service of the
    oldest Lsn he expects to need during restart.  The Lfs is allowed to
    reuse any part of the circular log file which logically precedes
    this Lsn.  A client may only specify a Lsn which follows the previous
    Lsn specified by this client.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    BaseLsn - This is the oldest Lsn the client may require during a
              restart.

Return Value:

    None

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLCH Lch;

    PLFCB Lfcb;

    PLFS_CLIENT_RECORD ClientRecord;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsSetBaseLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle        -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Base Lsn (Low)    -> %08lx\n", BaseLsn.LowPart );
    DebugTrace(  0, Dbg, "Base Lsn (High)   -> %08lx\n", BaseLsn.HighPart );

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

            ClientRecord = Add2Ptr( Lfcb->ClientArray,
                                    Lch->ClientArrayByteOffset,
                                    PLFS_CLIENT_RECORD );

            //
            //  We simply call the worker routine to advance the base lsn.
            //  If we moved forward in the file, we will put our restart area in the
            //  queue.
            //

            LfsSetBaseLsnPriv( Lfcb,
                               ClientRecord,
                               BaseLsn );

            LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, FALSE );

        } finally {

            DebugUnwind( LfsSetBaseLsn );

            //
            //  Release the log file control block if held.
            //

            LfsReleaseLch( Lch );

            DebugTrace( -1, Dbg, "LfsSetBaseLsn:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return;
}


//
//  Local support routine
//

VOID
LfsSetBaseLsnPriv (
    IN PLFCB Lfcb,
    IN PLFS_CLIENT_RECORD ClientRecord,
    IN LSN BaseLsn
    )

/*++

Routine Description:

    This worker routine is called internally by Lfs to modify the
    oldest Lsn a client expects to need during restart.  The Lfs is allowed to
    reuse any part of the circular log file which logically precedes
    this Lsn.  A client may only specify a Lsn which follows the previous
    Lsn specified by this client.

Arguments:

    Lfcb - Log context block for this file.

    ClientRecord - For the client whose base Lsn is being modified.

    BaseLsn - This is the oldest Lsn the client may require during a
              restart.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsSetBaseLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb              -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Base Lsn (Low)    -> %08lx\n", BaseLsn.LowPart );
    DebugTrace(  0, Dbg, "Base Lsn (High)   -> %08lx\n", BaseLsn.HighPart );

    //
    //  We only proceed if the client is moving forward in the file.
    //

    if ( BaseLsn.QuadPart > Lfcb->OldestLsn.QuadPart ) {                                                               //**** xxGtr( BaseLsn, Lfcb->OldestLsn )

        if ( BaseLsn.QuadPart > ClientRecord->OldestLsn.QuadPart ) {                                                   //**** xxGtr( BaseLsn, ClientRecord->OldestLsn )

            ClientRecord->OldestLsn = BaseLsn;
        }

        Lfcb->OldestLsn = BaseLsn;

        //
        //  We walk through all the active clients and find the new
        //  oldest Lsn for the log file.
        //

        LfsFindOldestClientLsn( Lfcb->RestartArea,
                                Lfcb->ClientArray,
                                &Lfcb->OldestLsn );

        Lfcb->OldestLsnOffset = LfsLsnToFileOffset( Lfcb, Lfcb->OldestLsn );
        ClearFlag( Lfcb->Flags, LFCB_NO_OLDEST_LSN );

        LfsFindCurrentAvail( Lfcb );
    }

    DebugTrace( -1, Dbg, "LfsSetBaseLsnPriv:  Exit\n", 0 );

    return;
}

