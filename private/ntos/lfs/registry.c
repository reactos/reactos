/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    Registry.c

Abstract:

    This module implements the routines which clients use to register
    themselves with the Log File Service.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_REGISTRY)

PLFCB
LfsRestartLogFile (
    IN PFILE_OBJECT LogFile,
    IN USHORT MaximumClients,
    IN ULONG LogPageSize OPTIONAL,
    IN LONGLONG FileSize,
    IN OUT PLFS_INFO LfsInfo,
    OUT PLFS_WRITE_DATA WriteData
    );

VOID
LfsNormalizeBasicLogFile (
    IN OUT PLONGLONG FileSize,
    IN OUT PULONG LogPageSize,
    IN OUT PUSHORT LogClients,
    IN BOOLEAN UseDefaultLogPage
    );

VOID
LfsUpdateLfcbFromPgHeader (
    IN PLFCB Lfcb,
    IN ULONG SystemPageSize,
    IN ULONG LogPageSize,
    IN SHORT MajorVersion,
    IN SHORT MinorVersion,
    IN BOOLEAN PackLog
    );

VOID
LfsUpdateLfcbFromNoRestart (
    IN PLFCB Lfcb,
    IN LONGLONG FileSize,
    IN LSN LastLsn,
    IN ULONG LogClients,
    IN ULONG OpenLogCount,
    IN BOOLEAN LogFileWrapped,
    IN BOOLEAN UseMultiplePageIo
    );

VOID
LfsUpdateLfcbFromRestart (
    IN PLFCB Lfcb,
    IN PLFS_RESTART_AREA RestartArea,
    IN USHORT RestartOffset
    );

VOID
LfsUpdateRestartAreaFromLfcb (
    IN PLFCB Lfcb,
    IN PLFS_RESTART_AREA RestartArea
    );

VOID
LfsInitializeLogFilePriv (
    IN PLFCB Lfcb,
    IN BOOLEAN ForceRestartToDisk,
    IN ULONG RestartAreaSize,
    IN LONGLONG StartOffsetForClear,
    IN BOOLEAN ClearLogFile
    );

VOID
LfsFindLastLsn (
    IN OUT PLFCB Lfcb
    );

BOOLEAN
LfsCheckSubsequentLogPage (
    IN PLFCB Lfcb,
    IN PLFS_RECORD_PAGE_HEADER RecordPageHeader,
    IN LONGLONG LogFileOffset,
    IN LONGLONG SequenceNumber
    );

VOID
LfsFlushLogPage (
    IN PLFCB Lfcb,
    PVOID LogPage,
    IN LONGLONG FileOffset,
    OUT PBCB *Bcb
    );

VOID
LfsRemoveClientFromList (
    IN PLFS_CLIENT_RECORD ClientArray,
    IN PLFS_CLIENT_RECORD ClientRecord,
    IN PUSHORT ListHead
    );

VOID
LfsAddClientToList (
    IN PLFS_CLIENT_RECORD ClientArray,
    IN USHORT ClientIndex,
    IN PUSHORT ListHead
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsAddClientToList)
#pragma alloc_text(PAGE, LfsCheckSubsequentLogPage)
#pragma alloc_text(PAGE, LfsCloseLogFile)
#pragma alloc_text(PAGE, LfsDeleteLogHandle)
#pragma alloc_text(PAGE, LfsFindLastLsn)
#pragma alloc_text(PAGE, LfsFlushLogPage)
#pragma alloc_text(PAGE, LfsInitializeLogFile)
#pragma alloc_text(PAGE, LfsInitializeLogFilePriv)
#pragma alloc_text(PAGE, LfsNormalizeBasicLogFile)
#pragma alloc_text(PAGE, LfsOpenLogFile)
#pragma alloc_text(PAGE, LfsReadLogFileInformation)
#pragma alloc_text(PAGE, LfsRemoveClientFromList)
#pragma alloc_text(PAGE, LfsResetUndoTotal)
#pragma alloc_text(PAGE, LfsRestartLogFile)
#pragma alloc_text(PAGE, LfsUpdateLfcbFromRestart)
#pragma alloc_text(PAGE, LfsUpdateLfcbFromNoRestart)
#pragma alloc_text(PAGE, LfsUpdateLfcbFromPgHeader)
#pragma alloc_text(PAGE, LfsUpdateRestartAreaFromLfcb)
#pragma alloc_text(PAGE, LfsVerifyLogFile)
#endif


VOID
LfsInitializeLogFile (
    IN PFILE_OBJECT LogFile,
    IN USHORT MaximumClients,
    IN ULONG LogPageSize OPTIONAL,
    IN LONGLONG FileSize,
    OUT PLFS_WRITE_DATA WriteData
    )

/*++

Routine Description:

    This routine is called to initialize a log file to prepare it for use
    by log clients.  Any previous data in the file will be overwritten.

    Lfs will partition the file into 2 Lfs restart areas and as many log
    pages as will fit in the file.  The restart area size is determined
    by computing the amount of space needed for the maximum number

Arguments:

    LogFile - This is the file object for the file to be used as a log file.

    MaximumClients - This is the maximum number of clients that will be
                     active in the log file at any one time.

    LogPageSize - If specified (not 0), this is the recommended size of
                  the log page.  Lfs will use this as a guide in
                  determining the log page size.

    FileSize - This is the available size of the log file.

    WriteData - Pointer to WRITE_DATA in caller's data structure.

Return Value:

    None

--*/

{
    PLFCB Lfcb;
    LARGE_INTEGER CurrentTime;

    PLFS_RESTART_AREA RestartArea = NULL;

    volatile NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsInitializeLogFile:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log File          -> %08lx\n", LogFile );
    DebugTrace(  0, Dbg, "Maximum Clients   -> %04x\n", MaximumClients );
    DebugTrace(  0, Dbg, "Log Page Size     -> %08lx\n", LogPageSize );
    DebugTrace(  0, Dbg, "File Size (Low)   -> %08lx\n", FileSize.LowPart );
    DebugTrace(  0, Dbg, "File Size (High)  -> %08lx\n", FileSize.HighPart );

    Lfcb = NULL;

    //
    //  Protect this entry point with a try-except.
    //

    try {

        //
        //  We grab the global data exclusively.
        //

        LfsAcquireLfsData();

        //
        //  Use a try-finally to facilitate cleanup.
        //

        try {

            //
            //  We allocate an Lfcb and point it to the log file.
            //

            Lfcb = LfsAllocateLfcb();

            LfsAcquireLfcb( Lfcb );

            Lfcb->FileObject = LogFile;

            //
            //  Call the Cache Manager to disable read ahead and write behind;
            //  we flush the log file explicitly.
            //

            CcSetAdditionalCacheAttributes( LogFile, TRUE, TRUE );

            //
            //  Normalize the values passed in with this call.
            //

            LfsNormalizeBasicLogFile( &FileSize,
                                      &LogPageSize,
                                      &MaximumClients,
                                      (BOOLEAN) ((PAGE_SIZE >= LFS_DEFAULT_LOG_PAGE_SIZE) &&
                                                 (PAGE_SIZE <= LFS_DEFAULT_LOG_PAGE_SIZE * 2)));

            //
            //  We can go directly to version 1.1 since we can immediately pack
            //  the log file and use the default system log page size.
            //

            LfsUpdateLfcbFromPgHeader( Lfcb,
                                       LogPageSize,
                                       LogPageSize,
                                       1,
                                       1,
                                       TRUE );

            KeQuerySystemTime( &CurrentTime );

            LfsUpdateLfcbFromNoRestart( Lfcb,
                                        FileSize,
                                        LfsLi0,
                                        MaximumClients,
                                        CurrentTime.LowPart,
                                        FALSE,
                                        FALSE );

            LfsAllocateRestartArea( &RestartArea, Lfcb->RestartDataSize );

            LfsUpdateRestartAreaFromLfcb( Lfcb, RestartArea );

            Lfcb->RestartArea = RestartArea;
            Lfcb->ClientArray = Add2Ptr( RestartArea, Lfcb->ClientArrayOffset, PLFS_CLIENT_RECORD );
            RestartArea = NULL;

            Lfcb->InitialRestartArea = TRUE;

            //
            //  Now update the caller's WRITE_DATA structure.
            //

            Lfcb->UserWriteData = WriteData;
            WriteData->LfsStructureSize = LogPageSize;
            WriteData->Lfcb = Lfcb;

            //
            //  Force two restart areas to disk and reinitialize the file.
            //

            LfsInitializeLogFilePriv( Lfcb,
                                      TRUE,
                                      Lfcb->RestartDataSize,
                                      Lfcb->FirstLogPage,
                                      TRUE );

            //
            //  Put the Lfcb into the global queue.
            //

            InsertHeadList( &LfsData.LfcbLinks, &Lfcb->LfcbLinks );

        } finally {

            DebugUnwind( LfsInitializeLogFile );

            //
            //  If the Lfcb has been acquired, we release it now.
            //

            if (Lfcb != NULL) {

                LfsReleaseLfcb( Lfcb );

                //
                //  If an error occurred and we allocated an Lfcb.  We deallocate
                //  it now.
                //

                if (AbnormalTermination()) {

                    LfsDeallocateLfcb( Lfcb, TRUE );
                }
            }

            //
            //  Deallocate the restart area.
            //

            if (RestartArea != NULL) {

                LfsDeallocateRestartArea( RestartArea );
            }

            //
            //  We release the global data.
            //

            LfsReleaseLfsData();

            DebugTrace( -1, Dbg, "LfsInitializeLogFile:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return;
}


ULONG
LfsOpenLogFile (
    IN PFILE_OBJECT LogFile,
    IN UNICODE_STRING ClientName,
    IN USHORT MaximumClients,
    IN ULONG LogPageSize OPTIONAL,
    IN LONGLONG FileSize,
    IN OUT PLFS_INFO LfsInfo,
    OUT PLFS_LOG_HANDLE LogHandle,
    OUT PLFS_WRITE_DATA WriteData
    )

/*++

Routine Description:

    This routine is called when a client wishes to register with logging
    service.  This can be a reregistration (i.e. restart after a crash)
    or an initial registration.  There can be no other active clients
    with the same name.  The Log Handle returned is then used for any
    subsequent access by this client.

    If an Lfs restart has not been done on the log file, it will be done
    at this time.

Arguments:

    LogFile - A file object for a file previously initialized for use
              as a log file.

    ClientName - This unicode string is used to uniquely identify clients
                 of the logging service.  A case-sensitive comparison is
                 used to check this name against active clients of the
                 log file.

    MaximumClients - The maximum number of clients if the log file has
                     never been initialized.

    LogPageSize - This is the recommeded size for the log page.

    FileSize - This is the size of the log file.

    LfsInfo - On entry, indicates the log file state the user may
        know about.  On exit, indicates the log file state that Lfs
        knows about.  This is a conduit for Lfs to communicate with its
        clients.

    LogHandle - The address to store the identifier the logging service
                will use to identify this client in all other Lfs calls.

    WriteData - Pointer to WRITE_DATA in caller's data structure.

Return Value:

    ULONG - Amount to add to reservation value for header for log record.

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLIST_ENTRY Link;
    PLFCB ThisLfcb = NULL;
    PLFCB NewLfcb = NULL;

    USHORT ThisClient;
    PLFS_CLIENT_RECORD ClientRecord;

    PLCH Lch = NULL;

    ULONG ReservedHeader;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsOpenLogFile:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log File          -> %08lx\n", LogFile );
    DebugTrace(  0, Dbg, "Client Name       -> %08lx\n", &ClientName );
    DebugTrace(  0, Dbg, "Maximum Clients   -> %04x\n", MaximumClients );
    DebugTrace(  0, Dbg, "Log Page Size     -> %08lx\n", LogPageSize );
    DebugTrace(  0, Dbg, "File Size (Low)   -> %08lx\n", FileSize.LowPart );
    DebugTrace(  0, Dbg, "File Size (High)  -> %08lx\n", FileSize.HighPart );

    //
    //  Check that the client name length is a legal length.
    //

    if (ClientName.Length > LFS_CLIENT_NAME_MAX) {

        DebugTrace(  0, Dbg, "Illegal name length for client\n", 0 );
        DebugTrace( -1, Dbg, "LfsOpenLogFile:  Exit\n", 0 );
        ExRaiseStatus( STATUS_INVALID_PARAMETER );
    }

    //
    //  Protect this call with a try-except.
    //

    try {

        //
        //  Aqcuire the global data.
        //

        LfsAcquireLfsData();

        //
        //  Use a try-finally to facilitate cleanup.
        //

        try {

            //
            //  Walk through the list searching for this file object.
            //

            Link = LfsData.LfcbLinks.Flink;

            while (Link != &LfsData.LfcbLinks) {

                ThisLfcb = CONTAINING_RECORD( Link, LFCB, LfcbLinks );

                if (ThisLfcb->FileObject == LogFile) {

                    DebugTrace( 0, Dbg, "Found matching log file\n", 0 );
                    break;
                }

                Link = Link->Flink;
            }

            //
            //  If the log file doesn't exist, create an Lfcb and perform an
            //  Lfs restart.
            //

            if (Link == &LfsData.LfcbLinks) {

                //
                //  Call the Cache Manager to disable read ahead and write behind;
                //  we flush the log file explicitly.
                //

                CcSetAdditionalCacheAttributes( LogFile, TRUE, TRUE );

                //
                //  Perform Lfs restart on this file object.
                //

                ThisLfcb = NewLfcb = LfsRestartLogFile( LogFile,
                                                        MaximumClients,
                                                        0,
                                                        FileSize,
                                                        LfsInfo,
                                                        WriteData );

                //
                //  Insert this Lfcb into the global list.
                //

                InsertHeadList( &LfsData.LfcbLinks, &ThisLfcb->LfcbLinks );
            }

            //
            //  At this point we have the log file control block for the file
            //  object given us.  We first check whether the log file is fatally
            //  corrupt.
            //

            if (FlagOn( ThisLfcb->Flags, LFCB_LOG_FILE_CORRUPT )) {

                //
                //  We leave the in-memory data alone and raise an error if
                //  anyone attempts to access this file.
                //

                DebugTrace( 0, Dbg, "The Lfcb is corrupt\n", 0 );
                ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
            }

            //
            //  Search through and look for a client match.
            //

            ThisClient = ThisLfcb->RestartArea->ClientInUseList;

            while (ThisClient != LFS_NO_CLIENT) {

                ClientRecord = ThisLfcb->ClientArray + ThisClient;

                if (ClientRecord->ClientNameLength == (ULONG) ClientName.Length
                    && RtlCompareMemory( ClientRecord->ClientName,
                                         ClientName.Buffer,
                                         ClientName.Length ) == (ULONG) ClientName.Length) {

                    DebugTrace( 0, Dbg, "Matching client name found\n", 0 );
                    break;
                }

                ThisClient = ClientRecord->NextClient;
            }

            //
            //  Allocate an Lch structure and link it into the Lfcb.
            //

            LfsAllocateLch( &Lch );
            InsertTailList( &ThisLfcb->LchLinks, &Lch->LchLinks );

            //
            //  Initialize the client handle with the data from the Lfcb.
            //

            Lch->Lfcb = ThisLfcb;
            Lch->Sync = ThisLfcb->Sync;
            Lch->Sync->UserCount += 1;

            //
            //  If a match isn't found, take a client block off the free list
            //  if available.
            //

            if (ThisClient == LFS_NO_CLIENT) {

                //
                //  Raise an error status if out of client blocks.
                //

                ThisClient = ThisLfcb->RestartArea->ClientFreeList;

                if (ThisClient == LFS_NO_CLIENT) {

                    DebugTrace( 0, Dbg, "No free client records available\n", 0 );
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }

                //
                //  Initialize the client block.
                //

                ClientRecord = ThisLfcb->ClientArray + ThisClient;

                LfsRemoveClientFromList( ThisLfcb->ClientArray,
                                         ClientRecord,
                                         &ThisLfcb->RestartArea->ClientFreeList );

                ClientRecord->ClientRestartLsn = LfsZeroLsn;
                ClientRecord->OldestLsn = ThisLfcb->OldestLsn;
                ClientRecord->ClientNameLength = ClientName.Length;
                RtlCopyMemory( ClientRecord->ClientName,
                               ClientName.Buffer,
                               ClientName.Length );

                //
                //  Add it to the in use list.
                //

                LfsAddClientToList( ThisLfcb->ClientArray,
                                    ThisClient,
                                    &ThisLfcb->RestartArea->ClientInUseList );
            }

            //
            //  Update the client handle with the client block information.
            //

            Lch->ClientId.SeqNumber = ClientRecord->SeqNumber;
            Lch->ClientId.ClientIndex = ThisClient;

            Lch->ClientArrayByteOffset = PtrOffset( ThisLfcb->ClientArray,
                                                    ClientRecord );

            *LogHandle = (LFS_LOG_HANDLE) Lch;

        } finally {

            DebugUnwind( LfsOpenLogFile );

            //
            //  If the Lfcb has been acquired, we release it now.
            //

            if (ThisLfcb != NULL) {

                //
                //  Pass information back to our caller for the number
                //  of bytes to add to the reserved amount for a
                //  log header.
                //

                ReservedHeader = ThisLfcb->RecordHeaderLength;
                if (FlagOn( ThisLfcb->Flags, LFCB_PACK_LOG )) {

                    ReservedHeader *= 2;
                }

                LfsReleaseLfcb( ThisLfcb );
            }

            //
            //  If there is an error then deallocate the Lch and any new Lfcb.
            //

            if (AbnormalTermination()) {

                if (Lch != NULL) {

                    LfsDeallocateLch( Lch );
                    ThisLfcb->Sync->UserCount -= 1;
                }

                if (NewLfcb != NULL) {

                    LfsDeallocateLfcb( NewLfcb, TRUE );
                }
            }

            //
            //  Always free the global.
            //

            LfsReleaseLfsData();

            DebugTrace(  0, Dbg, "Log Handle    -> %08ln\n", *LogHandle );
            DebugTrace( -1, Dbg, "LfsOpenLogFile:  Exit\n", 0 );
        }

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        Status = GetExceptionCode();
    }

    if (Status != STATUS_SUCCESS) {

        ExRaiseStatus( Status );
    }

    return ReservedHeader;
}


VOID
LfsCloseLogFile (
    IN LFS_LOG_HANDLE LogHandle
    )

/*++

Routine Description:

    This routine is called when a client detaches itself from the log
    file.  On return, all prior references to this client in the log
    file are inaccessible.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

Return Value:

    None

--*/

{
    volatile NTSTATUS Status = STATUS_SUCCESS;

    PLCH Lch;

    PLFCB Lfcb;

    USHORT ClientIndex;
    PLFS_CLIENT_RECORD ClientRecord;

    BOOLEAN FlushRestart;
    BOOLEAN ExitLoop;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsCloseLogFile:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "LogHandle  ->  %08lx\n", LogHandle );

    Lch = (PLCH) LogHandle;

    //
    //  Enclose this in a loop.  We will loop as long as there are waiters or there is an IO
    //  in progress.
    //

    while (TRUE) {

        //
        //  Always assume we exit the loop.
        //

        ExitLoop = TRUE;

        //
        //  Check that the structure is a valid log handle structure.
        //

        LfsValidateLch( Lch );

        //
        //  Protect this entry point with a try-except.
        //

        try {

            //
            //  Use a try-finally to facilitate cleanup.
            //

            //
            //  Acquire the global data block and the log file control block.
            //

            LfsAcquireLfsData();

            try {

                PLBCB ThisLbcb;

                LfsAcquireLch( Lch );

                Lfcb = Lch->Lfcb;

                //
                //  If the Log file has been closed then return immediately.
                //

                if (Lfcb == NULL) {

                    try_return( NOTHING );
                }

                //
                //  Check that there are no waiters or IO in progress before proceeding.
                //

                if ((Lfcb->Waiters != 0) ||
                    (Lfcb->LfsIoState != LfsNoIoInProgress)) {

                    ExitLoop = FALSE;
                    Lfcb->Waiters += 1;
                    try_return( NOTHING );
                }

                //
                //  Check that the client Id is valid.
                //

                LfsValidateClientId( Lfcb, Lch );

                ClientRecord = Add2Ptr( Lfcb->ClientArray,
                                        Lch->ClientArrayByteOffset,
                                        PLFS_CLIENT_RECORD );

#if 1
                //
                //  Increment the client sequence number in the restart area.
                //  This will prevent anyone else from accessing this client block.
                //

                ClientIndex = Lch->ClientId.ClientIndex;

                ClientRecord->SeqNumber++;
#endif

                //
                //  Remember if this client wrote a restart area.
                //

                FlushRestart = (BOOLEAN) ( LfsZeroLsn.QuadPart != ClientRecord->ClientRestartLsn.QuadPart );

#if 1
                //
                //  Remove the client from the log file in use list.
                //

                LfsRemoveClientFromList( Lfcb->ClientArray,
                                         ClientRecord,
                                         &Lfcb->RestartArea->ClientInUseList );

                //
                //  Add the client block to the log file free list
                //

                LfsAddClientToList( Lfcb->ClientArray,
                                    ClientIndex,
                                    &Lfcb->RestartArea->ClientFreeList );

                //
                //  If this is the last client then move the last active Lbcb off
                //  the active queue.
                //

                if (Lfcb->RestartArea->ClientInUseList == LFS_NO_CLIENT) {
#endif
                    //
                    //  Set the flag to indicate we are at the final close.
                    //

                    SetFlag( Lfcb->Flags, LFCB_FINAL_SHUTDOWN );

                    //
                    //  Walk through the active queue and remove any Lbcb's with
                    //  data from that queue.  That will allow them to get out to disk.
                    //

                    while (!IsListEmpty( &Lfcb->LbcbActive )) {

                        ThisLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                                      LBCB,
                                                      ActiveLinks );

                        RemoveEntryList( &ThisLbcb->ActiveLinks );
                        ClearFlag( ThisLbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );

                        //
                        //  If this page has some new entries, allow it to
                        //  be flushed to disk.  Otherwise deallocate it.
                        //

                        if (!FlagOn( ThisLbcb->LbcbFlags, LBCB_NOT_EMPTY )) {

                            RemoveEntryList( &ThisLbcb->WorkqueLinks );

                            if (ThisLbcb->LogPageBcb != NULL) {

                                CcUnpinDataForThread( ThisLbcb->LogPageBcb,
                                                      ThisLbcb->ResourceThread );
                            }

                            LfsDeallocateLbcb( Lfcb, ThisLbcb );
                        }
                    }

                    //
                    //  It's possible that we have the two restart areas in the workque.
                    //  They can be removed and the memory deallocated if we have no
                    //  more clients.
                    //
                    //  We skip this action if the there is Io in progress or the user
                    //  had a restart area.
                    //

                    if ((Lfcb->LfsIoState == LfsNoIoInProgress) && !FlushRestart) {

                        PLIST_ENTRY Links;

                        //
                        //  Now walk through the workque list looking for a non-restart
                        //  entry.
                        //

                        Links = Lfcb->LbcbWorkque.Flink;

                        while (Links != &Lfcb->LbcbWorkque) {

                            ThisLbcb = CONTAINING_RECORD( Links,
                                                          LBCB,
                                                          WorkqueLinks );

                            //
                            //  If this is not a restart area, we exit and remember that
                            //  we need to flush the restart areas.
                            //

                            if (!LfsLbcbIsRestart( ThisLbcb )) {

                                FlushRestart = TRUE;
                                break;
                            }

                            Links = Links->Flink;
                        }

                        //
                        //  If we are still not to flush the restart areas remove
                        //  all of the restart areas from the queue.
                        //

                        if (!FlushRestart) {

                            while (!IsListEmpty( &Lfcb->LbcbWorkque)) {

                                ThisLbcb = CONTAINING_RECORD( Lfcb->LbcbWorkque.Blink,
                                                              LBCB,
                                                              WorkqueLinks );

                                RemoveEntryList( &ThisLbcb->WorkqueLinks );
                                LfsDeallocateLbcb( Lfcb, ThisLbcb );
                            }
                        }

                    } else {

                        FlushRestart = TRUE;
                    }

#if 1
                //
                //  We will have to flush the restart area in this case.
                //

                } else {

                    FlushRestart = TRUE;
                }
#endif

                //
                //  Flush the new restart area if we need to.
                //

                if (FlushRestart) {

                    LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, FALSE );
                    LfsWriteLfsRestart( Lfcb, Lfcb->RestartAreaSize, TRUE );
                }

                //
                //  Clear the Lfcb pointer in the client handle.
                //

                Lch->Lfcb = NULL;
                RemoveEntryList( &Lch->LchLinks );

                //
                //  If there are no active clients, we can remove this log file
                //  control block from the active queue.
                //

#if 1
                if (Lfcb->RestartArea->ClientInUseList == LFS_NO_CLIENT) {
#endif

                    RemoveEntryList( &Lfcb->LfcbLinks );
                    LfsDeallocateLfcb( Lfcb, FALSE );
#if 1
                }
#endif

            try_exit:  NOTHING;
            } finally {

                DebugUnwind( LfsCloseLogFile );

                //
                //   Release the log file control block if held.
                //

                LfsReleaseLch( Lch );

                //
                //  Release the global data block if held.
                //

                LfsReleaseLfsData();

                DebugTrace( -1, Dbg, "LfsCloseLogFile:  Exit\n", 0 );
            }

        } except (LfsExceptionFilter( GetExceptionInformation() )) {

            Status = GetExceptionCode();
        }

        //
        //  Test if we want to exit the loop now.
        //

        if (ExitLoop) { break; }

        //
        //  Wait for the io to complete.
        //

        KeWaitForSingleObject( &Lfcb->Sync->Event,
                               Executive,
                               KernelMode,
                               FALSE,
                               NULL );

        LfsAcquireLfcb( Lfcb );
        Lfcb->Waiters -= 1;
        LfsReleaseLfcb( Lfcb );
    }

    //
    //  We always let this operation succeed.
    //

    return;
}

VOID
LfsDeleteLogHandle (
    IN LFS_LOG_HANDLE LogHandle
    )

/*++

Routine Description:

    This routine is called when a client is tearing down the last of
    his volume structures.  There will be no more references to this
    handle.  If it is the last handle for the log file then we will
    deallocate the Sync structure as well.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

Return Value:

    None

--*/

{
    PLCH Lch;

    PAGED_CODE();

    //
    //  If the log handle is null then return immediately.
    //

    Lch = (PLCH) LogHandle;

    if ((Lch == NULL) ||
        (Lch->NodeTypeCode != LFS_NTC_LCH)) {

        return;
    }

    //
    //  Ignore all errors from now on.
    //

    try {

        LfsAcquireLch( Lch );

        Lch->Sync->UserCount -= 1;

        //
        //  If we are the last user then deallocate the sync structure.
        //

        if (Lch->Sync->UserCount == 0) {

            ExDeleteResource( &Lch->Sync->Resource );

            ExFreePool( Lch->Sync );

        } else {

            LfsReleaseLch( Lch );
        }

        LfsDeallocateLch( Lch );

    } except (LfsExceptionFilter( GetExceptionInformation() )) {

        NOTHING;
    }

    return;
}


VOID
LfsReadLogFileInformation (
    IN LFS_LOG_HANDLE LogHandle,
    IN PLOG_FILE_INFORMATION Buffer,
    IN OUT PULONG Length
    )

/*++

Routine Description:

    This routine returns information about the current state of the log
    file, primarily to aid the client perform its checkpoint processing.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    Buffer - Pointer to buffer to return the log file information.

    Length - On input this is the length of the user's buffer.  On output,
             it is the amount of data stored by the Lfs in the buffer.

Return Value:

    None

--*/

{
    PLCH Lch;
    PLFCB Lfcb;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsReadLogFileInformation:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle    -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Buffer        -> %08lx\n", Buffer );
    DebugTrace(  0, Dbg, "Length        -> %08lx\n", *Length );

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
        //  If the Log file has been closed then return immediately.
        //

        if (Lfcb == NULL) {

            try_return( *Length = 0 );
        }

        //
        //  Check that the client Id is valid.
        //

        LfsValidateClientId( Lfcb, Lch );

        //
        //  The buffer better be large enough.
        //

        if (*Length >= sizeof( LOG_FILE_INFORMATION )) {

            PLOG_FILE_INFORMATION Information;
            LONGLONG CurrentAvail;
            ULONG UnusedBytes;

            LfsCurrentAvailSpace( Lfcb,
                                  &CurrentAvail,
                                  &UnusedBytes );

            //
            //  Cast a pointer to the buffer and fill in the
            //  data.
            //

            Information = (PLOG_FILE_INFORMATION) Buffer;

            Information->TotalAvailable = Lfcb->TotalAvailable;
            Information->CurrentAvailable =  CurrentAvail;
            Information->TotalUndoCommitment = Lfcb->TotalUndoCommitment;
            Information->ClientUndoCommitment = Lch->ClientUndoCommitment;

            Information->OldestLsn = Lfcb->OldestLsn;
            Information->LastFlushedLsn = Lfcb->LastFlushedLsn;
            Information->LastLsn = Lfcb->RestartArea->CurrentLsn;

            *Length = sizeof( LOG_FILE_INFORMATION );

        } else {

            *Length = 0;
        }

    try_exit:  NOTHING;
    } finally {

        DebugUnwind( LfsReadLogFileInformation );

        //
        //  Release the log file control block if held.
        //

        LfsReleaseLch( Lch );

        DebugTrace( -1, Dbg, "LfsReadLogFileInformation:  Exit\n", 0 );
    }

    return;
}


BOOLEAN
LfsVerifyLogFile (
    IN LFS_LOG_HANDLE LogHandle,
    IN PVOID LogFileHeader,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is called by a client to verify that the volume has not been removed
    from the system and then reattached.  We will verify the log file open count on
    disk matches the value in the user's handle.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    LogFileHeader - Pointer to start of log file.

    Length - Number bytes returned with the read.

Return Value:

    BOOLEAN - TRUE if the log file has not been altered externally, FALSE if we
        fail for any reason.

--*/

{
    BOOLEAN ValidLogFile = FALSE;
    PLCH Lch;
    PLFCB Lfcb;

    PLFS_RESTART_PAGE_HEADER RestartPage = LogFileHeader;

    PAGED_CODE();

    Lch = (PLCH) LogHandle;

    //
    //  Check that the structure is a valid log handle structure.
    //

    if ((Lch == NULL) ||
        (Lch->NodeTypeCode != LFS_NTC_LCH) ||
        ((Lch->Lfcb != NULL) &&
         (Lch->Lfcb->NodeTypeCode != LFS_NTC_LFCB))) {

        return FALSE;
    }

    //
    //  Acquire the log file control block for this log file.
    //

    LfsAcquireLch( Lch );
    Lfcb = Lch->Lfcb;

    //
    //  If the Log file has been closed then return immediately.
    //

    if (Lfcb == NULL) {

        LfsReleaseLch( Lch );
        return FALSE;
    }

    //
    //  Check that we have at least one page and that the page is valid.
    //

    if ((Length >= (ULONG) Lfcb->SystemPageSize) &&
        (*((PULONG) RestartPage) == LFS_SIGNATURE_RESTART_PAGE_ULONG) &&
        ((RestartPage->RestartOffset + sizeof( LFS_RESTART_AREA )) < (ULONG) Lfcb->SystemPageSize) &&
        ((Add2Ptr( RestartPage, RestartPage->RestartOffset, PLFS_RESTART_AREA ))->RestartOpenLogCount == Lfcb->CurrentOpenLogCount)) {

        ValidLogFile = TRUE;
    }

    LfsReleaseLfcb( Lfcb );
    return ValidLogFile;
}


VOID
LfsResetUndoTotal (
    IN LFS_LOG_HANDLE LogHandle,
    IN ULONG NumberRecords,
    IN LONG ResetTotal
    )

/*++

Routine Description:

    This routine is called to adjust the undo commitment for this client.
    If the reset total is positive, then we absolutely set the
    reserve value for the client using this as the basis.  If the value
    is negative, we will adjust the current value for the client.

    To adjust the values in the Lfcb, we first return the Undo commitment
    in the handle and then adjust by the values passed in.

    To adjust the value in the client handle, we simply set it if
    the reset value is positive, adjust it if the value is negative.

    For a packed log file we just reserve the space requested.  We
    have already taken into account the loss of the tail of each page.
    For an unpacked log file we double each value.

Arguments:

    LogHandle - Pointer to private Lfs structure used to identify this
                client.

    NumberRecords - This is the number of records we should assume the
                    reset total covers.  We allow an Lfs header for
                    each one.

    ResetTotal - This is the amount to adjust (or set) the undo
                 commitment.

Return Value:

    None

--*/

{
    PLCH Lch;

    PLFCB Lfcb;

    LONGLONG AdjustedUndoTotal;
    LONG LfsHeaderBytes;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsResetUndoTotal:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Log Handle        -> %08lx\n", LogHandle );
    DebugTrace(  0, Dbg, "Number Records    -> %08lx\n", NumberRecords );
    DebugTrace(  0, Dbg, "ResetTotal        -> %08lx\n", ResetTotal );

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
        //  Compute the adjusted reset total.  Start by computing the
        //  bytes needed for the Lfs log headers.  Add (or subtract) this
        //  from the reset total and multiply by 2 (only if not packing the
        //  log).
        //

        LfsHeaderBytes = NumberRecords * Lfcb->RecordHeaderLength;
        LfsHeaderBytes *= 2;

        if (!FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

            ResetTotal *= 2;
        }

        //
        //  If the reset total is positive, add the header bytes.
        //

        if (ResetTotal > 0) {

            //
            //  Subtract the client's current value from the TotalUndo
            //  commit if he is setting his value exactly.
            //

            Lfcb->TotalUndoCommitment = Lfcb->TotalUndoCommitment - Lch->ClientUndoCommitment;

            //
            //  We can clear the values in the user's handle at this
            //  time.
            //

            Lch->ClientUndoCommitment = 0;


            ResetTotal += LfsHeaderBytes;

        //
        //  Otherwise subtract the value for the header bytes.
        //

        } else {

            ResetTotal -= LfsHeaderBytes;
        }

        //
        //  Now we adjust the Lfcb and Lch values by the adjustment amount.
        //

        AdjustedUndoTotal = ResetTotal;

        Lfcb->TotalUndoCommitment = Lfcb->TotalUndoCommitment + AdjustedUndoTotal;

        Lch->ClientUndoCommitment = Lch->ClientUndoCommitment + AdjustedUndoTotal;

    } finally {

        DebugUnwind( LfsResetUndoTotal );

        //
        //  Release the log file control block if held.
        //

        LfsReleaseLch( Lch );

        DebugTrace( -1, Dbg, "LfsResetUndoTotal:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine.
//

PLFCB
LfsRestartLogFile (
    IN PFILE_OBJECT LogFile,
    IN USHORT MaximumClients,
    IN ULONG LogPageSize OPTIONAL,
    IN LONGLONG FileSize,
    IN OUT PLFS_INFO LfsInfo,
    OUT PLFS_WRITE_DATA WriteData
    )

/*++

Routine Description:

    This routine is called to process an existing log file when it opened
    for the first time on a running system.  We walk through the beginning
    of the file looking for a valid restart area.  Once we have a restart
    area, we can find the next restart area and determine which is the
    most recent.  The data in the restart area will tell us if the system
    has been gracefully shutdown and whether the log file in its current
    state can run on the current system.

    If the file is usable, we perform any necessary initialization on the
    file to prepare it for operation.

Arguments:

    LogFile - This is the file to use as a log file.

    MaximumClients - This is the maximum number of clients that will be
                     active in the log file at any one time.

    LogPageSize - If specified (not 0), this is the recommended size of
                  the log page.  Lfs will use this as a guide in
                  determining the log page size.

    FileSize - This is the available size of the log file.

    LfsInfo - On entry, indicates the log file state the user may
        know about.  On exit, indicates the log file state that Lfs
        knows about.  This is a conduit for Lfs to communicate with its
        clients.

    WriteData - Pointer to WRITE_DATA in caller's data structure.

Return Value:

    PLFCB - A pointer to an initialized Lfcb to use for
                              this log file.

--*/

{
    PLFCB ThisLfcb = NULL;
    PLFS_RESTART_AREA RestartArea = NULL;
    PLFS_RESTART_AREA DiskRestartArea;

    BOOLEAN UninitializedFile;

    LONGLONG OriginalFileSize = FileSize;
    LONGLONG FirstRestartOffset;
    PLFS_RESTART_PAGE_HEADER FirstRestartPage;
    BOOLEAN FirstChkdskWasRun;
    BOOLEAN FirstValidPage;
    BOOLEAN FirstLogPacked;
    LSN FirstRestartLastLsn;

    PBCB FirstRestartPageBcb = NULL;
    PBCB SecondRestartPageBcb = NULL;

    BOOLEAN PackLogFile = TRUE;
    BOOLEAN UseDefaultLogPage = FALSE;
    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsRestartLogFile:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "LogFile    -> %08lx\n", LogFile );
    DebugTrace(  0, Dbg, "Maximum Clients   -> %04x\n", MaximumClients );
    DebugTrace(  0, Dbg, "Log Page Size     -> %08lx\n", LogPageSize );
    DebugTrace(  0, Dbg, "File Size (Low)   -> %08lx\n", FileSize.LowPart );
    DebugTrace(  0, Dbg, "File Size (High)  -> %08lx\n", FileSize.HighPart );
    DebugTrace(  0, Dbg, "Pack Log           -> %04x\n", *LfsInfo );

    //
    //  Remember if we are to pack the log file.  Once a log file has
    //  been packed we will attempt to keep it that way.
    //

    ASSERT( *LfsInfo >= LfsPackLog );

    if ((PAGE_SIZE >= LFS_DEFAULT_LOG_PAGE_SIZE) &&
        (PAGE_SIZE <= LFS_DEFAULT_LOG_PAGE_SIZE * 2) &&
        (*LfsInfo >= LfsFixedPageSize)) {

        UseDefaultLogPage = TRUE;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  Normalize the values passed in with this call.
        //

        LfsNormalizeBasicLogFile( &FileSize,
                                  &LogPageSize,
                                  &MaximumClients,
                                  UseDefaultLogPage );

        //
        //  Allocate an Lfcb to use for this file.
        //

        ThisLfcb = LfsAllocateLfcb();

        //
        //  Acquire the Lfcb and store it in the global queue.
        //

        LfsAcquireLfcb( ThisLfcb );

        //
        //  Remember this log file in the Lfcb.
        //

        ThisLfcb->FileObject = LogFile;

        SetFlag( ThisLfcb->Flags,
                 (LFCB_READ_FIRST_RESTART |
                  LFCB_READ_SECOND_RESTART) );

        //
        //  Look for a restart area on the disk.
        //

        if (LfsReadRestart( ThisLfcb,
                            FileSize,
                            TRUE,
                            &FirstRestartOffset,
                            &FirstRestartPage,
                            &FirstRestartPageBcb,
                            &FirstChkdskWasRun,
                            &FirstValidPage,
                            &UninitializedFile,
                            &FirstLogPacked,
                            &FirstRestartLastLsn )) {

            BOOLEAN DoubleRestart;

            LONGLONG SecondRestartOffset;
            PLFS_RESTART_PAGE_HEADER SecondRestartPage;
            BOOLEAN SecondChkdskWasRun;
            BOOLEAN SecondValidPage;
            BOOLEAN SecondLogPacked;
            LSN SecondRestartLastLsn;

            //
            //  If the restart offset above wasn't zero then we
            //  won't look for a second restart.
            //

            if (FirstRestartOffset == 0) {

                ClearFlag( ThisLfcb->Flags, LFCB_READ_FIRST_RESTART );

                DoubleRestart = LfsReadRestart( ThisLfcb,
                                                FileSize,
                                                FALSE,
                                                &SecondRestartOffset,
                                                &SecondRestartPage,
                                                &SecondRestartPageBcb,
                                                &SecondChkdskWasRun,
                                                &SecondValidPage,
                                                &UninitializedFile,
                                                &SecondLogPacked,
                                                &SecondRestartLastLsn );

                if (DoubleRestart) {

                    ClearFlag( ThisLfcb->Flags, LFCB_READ_SECOND_RESTART );
                }

            } else {

                ClearFlag( ThisLfcb->Flags, LFCB_READ_SECOND_RESTART );
                DoubleRestart = FALSE;
            }

            //
            //  Determine which restart area to use.
            //

            if (DoubleRestart
                && (SecondRestartLastLsn.QuadPart > FirstRestartLastLsn.QuadPart)) {

                BOOLEAN UseSecondPage = TRUE;
                PULONG SecondPage;
                PBCB SecondPageBcb = NULL;
                BOOLEAN UsaError;

                //
                //  In a very strange case we could have crashed on a system with
                //  a different page size and then run chkdsk on the new system.
                //  The second restart page may not have the chkdsk signature in
                //  that case but could have a higher final Lsn.
                //  We want to ignore the second restart area in that case.
                //

                if (FirstChkdskWasRun &&
                    (SecondRestartOffset != PAGE_SIZE)) {

                    if (NT_SUCCESS( LfsPinOrMapData( ThisLfcb,
                                                     PAGE_SIZE,
                                                     PAGE_SIZE,
                                                     FALSE,
                                                     TRUE,
                                                     TRUE,
                                                     &UsaError,
                                                     &SecondPage,
                                                     &SecondPageBcb )) &&
                        (*SecondPage == LFS_SIGNATURE_MODIFIED_ULONG)) {

                        UseSecondPage = FALSE;
                    }

                    if (SecondPageBcb != NULL) {

                        CcUnpinData( SecondPageBcb );
                    }
                }

                if (UseSecondPage) {

                    FirstRestartOffset = SecondRestartOffset;
                    FirstRestartPage = SecondRestartPage;
                    FirstChkdskWasRun = SecondChkdskWasRun;
                    FirstValidPage = SecondValidPage;
                    FirstLogPacked = SecondLogPacked;
                    FirstRestartLastLsn = SecondRestartLastLsn;
                }
            }

            //
            //  If the restart area is at offset 0, we want to write
            //  the second restart area out first.
            //

            if (FirstRestartOffset != 0) {

                ThisLfcb->InitialRestartArea = TRUE;
            }

            //
            //  If we have a valid page then grab a pointer to the restart area.
            //

            if (FirstValidPage) {

                DiskRestartArea = Add2Ptr( FirstRestartPage, FirstRestartPage->RestartOffset, PLFS_RESTART_AREA );
            }

            //
            //  If checkdisk was run or there are no active clients,
            //  then we will begin at the start of the log file.
            //

            if (FirstChkdskWasRun
                || DiskRestartArea->ClientInUseList == LFS_NO_CLIENT) {

                //
                //  Default version is 1.1.
                //

                SHORT MajorVersion = 1;
                SHORT MinorVersion = 1;

                BOOLEAN LogFileWrapped = FALSE;
                BOOLEAN UseMultiplePageIo = FALSE;

                BOOLEAN ForceRestartToDisk = TRUE;

                BOOLEAN ClearLogFile = TRUE;

                LONGLONG StartOffsetForClear;
                StartOffsetForClear = LogPageSize * 2;

                //
                //  Do some checks based on whether we have a valid log page.
                //

                if (FirstValidPage) {

                    CurrentTime.LowPart = DiskRestartArea->RestartOpenLogCount;

                    //
                    //  If the restart page size isn't changing then we want to
                    //  check how much work we need to do.
                    //

                    if (LogPageSize == FirstRestartPage->SystemPageSize) {

                        //
                        //  If the file size is changing we want to remember
                        //  at which point we want to start clearing the file.
                        //

                        if (FileSize > DiskRestartArea->FileSize) {

                            StartOffsetForClear = DiskRestartArea->FileSize;

                        } else {

                            if (!FlagOn( DiskRestartArea->Flags, RESTART_SINGLE_PAGE_IO )) {

                                UseMultiplePageIo = TRUE;
                                LogFileWrapped = TRUE;
                            }

                            //
                            //  If the page is valid we don't need to clear the log
                            //  file or force the data to disk.
                            //

                            ForceRestartToDisk = FALSE;
                            ClearLogFile = FALSE;
                        }
                    }

                } else {

                    KeQuerySystemTime( &CurrentTime );
                }

                //
                //  Initialize our Lfcb for the current log page values.
                //

                LfsUpdateLfcbFromPgHeader( ThisLfcb,
                                           LogPageSize,
                                           LogPageSize,
                                           MajorVersion,
                                           MinorVersion,
                                           PackLogFile );

                LfsUpdateLfcbFromNoRestart( ThisLfcb,
                                            FileSize,
                                            FirstRestartLastLsn,
                                            MaximumClients,
                                            CurrentTime.LowPart,
                                            LogFileWrapped,
                                            UseMultiplePageIo );

                LfsAllocateRestartArea( &RestartArea, ThisLfcb->RestartDataSize );

                LfsUpdateRestartAreaFromLfcb( ThisLfcb, RestartArea );

                ThisLfcb->RestartArea = RestartArea;
                ThisLfcb->ClientArray = Add2Ptr( RestartArea,
                                                 ThisLfcb->ClientArrayOffset,
                                                 PLFS_CLIENT_RECORD );
                RestartArea = NULL;

                //
                //  Unpin any pages pinned here.
                //

                if (FirstRestartPageBcb != NULL) {

                    CcUnpinData( FirstRestartPageBcb );
                    FirstRestartPageBcb = NULL;
                }

                if (SecondRestartPageBcb != NULL) {

                    CcUnpinData( SecondRestartPageBcb );
                    SecondRestartPageBcb = NULL;
                }

                //
                //  Now update the caller's WRITE_DATA structure.
                //
    
                ThisLfcb->UserWriteData = WriteData;
                WriteData->LfsStructureSize = LogPageSize;
                WriteData->Lfcb = ThisLfcb;
    
                //
                //  Put the restart areas out and initialize the log file
                //  as required.
                //

                LfsInitializeLogFilePriv( ThisLfcb,
                                          ForceRestartToDisk,
                                          ThisLfcb->RestartDataSize,
                                          StartOffsetForClear,
                                          ClearLogFile );

            //
            //  If the log page or the system page sizes have changed,
            //  we can't use the log file.  We must use the system
            //  page size instead of the default size if there is not
            //  a clean shutdown.
            //

            } else {

                if (LogPageSize != FirstRestartPage->SystemPageSize) {

                    FileSize = OriginalFileSize;
                    LfsNormalizeBasicLogFile( &FileSize,
                                              &LogPageSize,
                                              &MaximumClients,
                                              (BOOLEAN) (FirstRestartPage->SystemPageSize == LFS_DEFAULT_LOG_PAGE_SIZE) );
                }

                if ((LogPageSize != FirstRestartPage->SystemPageSize) || 
                    (LogPageSize != FirstRestartPage->LogPageSize)) {

                    DebugTrace( 0, Dbg, "Page size mismatch\n", 0 );
                    ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );

                //
                //  If the file size has shrunk then we won't mount it.
                //

                } else if (FileSize < DiskRestartArea->FileSize) {

                    DebugTrace( 0, Dbg, "Log file has shrunk without clean shutdown\n", 0 );
                    ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );

                //
                //  Otherwise we have a restart area to deal with.
                //

                } else {

                    //
                    //  We preserve the packed status from the disk.
                    //

                    PackLogFile = FirstLogPacked;

                    //
                    //  Update the Lfcb from the values in the restart area
                    //  page header and the active restart page.
                    //

                    LfsUpdateLfcbFromPgHeader( ThisLfcb,
                                               LogPageSize,
                                               LogPageSize,
                                               FirstRestartPage->MajorVersion,
                                               FirstRestartPage->MinorVersion,
                                               FirstLogPacked );

                    LfsUpdateLfcbFromRestart( ThisLfcb,
                                              DiskRestartArea,
                                              FirstRestartPage->RestartOffset );

                    //
                    //  Now allocate a restart area.
                    //

                    LfsAllocateRestartArea( &RestartArea, ThisLfcb->RestartDataSize );

                    //
                    //  We may need to grow the restart area to allow room for the open
                    //  log file count.
                    //

                    if (ThisLfcb->ClientArrayOffset == FIELD_OFFSET( LFS_RESTART_AREA, LogClientArray )) {

                        RtlCopyMemory( RestartArea, DiskRestartArea, ThisLfcb->RestartAreaSize );

                    } else {

                        LARGE_INTEGER CurrentTime;

                        //
                        //  Copy the start of the restart area over.
                        //

                        RtlCopyMemory( RestartArea, DiskRestartArea, ThisLfcb->ClientArrayOffset );

                        //
                        //  Now copy over the client data to its new location.
                        //

                        RtlCopyMemory( RestartArea->LogClientArray,
                                       Add2Ptr( DiskRestartArea, ThisLfcb->ClientArrayOffset, PVOID ),
                                       DiskRestartArea->RestartAreaLength - ThisLfcb->ClientArrayOffset );

                        //
                        //  Update the system open count.
                        //

                        KeQuerySystemTime( &CurrentTime );

                        ThisLfcb->CurrentOpenLogCount =
                        RestartArea->RestartOpenLogCount = CurrentTime.LowPart;

                        //
                        //  Now update the numbers in the Lfcb and restart area.
                        //

                        ThisLfcb->ClientArrayOffset = FIELD_OFFSET( LFS_RESTART_AREA, LogClientArray );
                        ThisLfcb->RestartAreaSize = ThisLfcb->ClientArrayOffset
                                                    + (sizeof( LFS_CLIENT_RECORD ) * ThisLfcb->LogClients );

                        RestartArea->ClientArrayOffset = ThisLfcb->ClientArrayOffset;
                        RestartArea->RestartAreaLength = (USHORT) ThisLfcb->RestartAreaSize;
                    }

                    //
                    //  Update the log file open count.
                    //

                    RestartArea->RestartOpenLogCount += 1;

                    ThisLfcb->RestartArea = RestartArea;

                    ThisLfcb->ClientArray = Add2Ptr( RestartArea, ThisLfcb->ClientArrayOffset, PLFS_CLIENT_RECORD );
                    RestartArea = NULL;

                    //
                    //  Unpin any pages pinned here.
                    //

                    if (FirstRestartPageBcb != NULL) {

                        CcUnpinData( FirstRestartPageBcb );
                        FirstRestartPageBcb = NULL;
                    }

                    if (SecondRestartPageBcb != NULL) {

                        CcUnpinData( SecondRestartPageBcb );
                        SecondRestartPageBcb = NULL;
                    }

                    //
                    //  Now update the caller's WRITE_DATA structure.
                    //
        
                    ThisLfcb->UserWriteData = WriteData;
                    WriteData->LfsStructureSize = LogPageSize;
                    WriteData->Lfcb = ThisLfcb;
        
                    //
                    //  Now we need to walk through looking for the last
                    //  Lsn.
                    //

                    LfsFindLastLsn( ThisLfcb );

                    //
                    //  Recalculate the available pages in the Lfcb.
                    //

                    LfsFindCurrentAvail( ThisLfcb );

                    //
                    //  Remember which restart area to write out first.
                    //

                    if (FirstRestartOffset != 0) {

                        ThisLfcb->InitialRestartArea = TRUE;
                    }

                    //
                    //  Queue the restart areas.
                    //

                    LfsInitializeLogFilePriv( ThisLfcb,
                                              FALSE,
                                              ThisLfcb->RestartDataSize,
                                              0,
                                              FALSE );
                }
            }

        //
        //  If the file is uninitialized, we will initialized it with new
        //  restart areas.  We can move to version 1.0 where we use
        //  update sequence array support but don't have to force the values
        //  to disk.
        //

        } else if (UninitializedFile) {

            //
            //  We go to a packed system if possible.
            //

            LfsUpdateLfcbFromPgHeader( ThisLfcb,
                                       LogPageSize,
                                       LogPageSize,
                                       1,
                                       1,
                                       PackLogFile );

            KeQuerySystemTime( &CurrentTime );
            LfsUpdateLfcbFromNoRestart( ThisLfcb,
                                        FileSize,
                                        LfsLi0,
                                        MaximumClients,
                                        CurrentTime.LowPart,
                                        FALSE,
                                        FALSE );

            LfsAllocateRestartArea( &RestartArea, ThisLfcb->RestartDataSize );

            LfsUpdateRestartAreaFromLfcb( ThisLfcb, RestartArea );

            ThisLfcb->RestartArea = RestartArea;
            ThisLfcb->ClientArray = Add2Ptr( RestartArea, ThisLfcb->ClientArrayOffset, PLFS_CLIENT_RECORD );

            ThisLfcb->InitialRestartArea = TRUE;
            RestartArea = NULL;

            //
            //  Now update the caller's WRITE_DATA structure.
            //

            ThisLfcb->UserWriteData = WriteData;
            WriteData->LfsStructureSize = LogPageSize;
            WriteData->Lfcb = ThisLfcb;

            //
            //  Put both restart areas in the queue to be flushed but don't
            //  force them to disk.
            //

            LfsInitializeLogFilePriv( ThisLfcb,
                                      FALSE,
                                      ThisLfcb->RestartDataSize,
                                      0,
                                      FALSE );

        //
        //  We didn't find a restart area but the file is not initialized.
        //  This is a corrupt disk.
        //

        } else {

            DebugTrace( 0, Dbg, "Log file has no restart area\n", 0 );
            ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
        }

    } finally {

        DebugUnwind( LfsRestartLogFile );

        //
        //  Free the Lfcb if allocated.
        //

        if (ThisLfcb != NULL) {

            LfsReleaseLfcb( ThisLfcb );

            //
            //  Free the Lfcb and Restart areas in the event of an error.
            //

            if (AbnormalTermination()) {

                LfsDeallocateLfcb( ThisLfcb, TRUE );

                if (RestartArea != NULL) {

                    LfsDeallocateRestartArea( RestartArea );
                }
            }
        }

        if (FirstRestartPageBcb != NULL) {

            CcUnpinData( FirstRestartPageBcb );
        }

        if (SecondRestartPageBcb != NULL) {

            CcUnpinData( SecondRestartPageBcb );
        }

        DebugTrace( -1, Dbg, "LfsRestartLogFile:  Exit\n", 0 );
    }

    //
    //  Indicate whether the log is packed.
    //

    if (PackLogFile
        && *LfsInfo < LfsPackLog) {

        *LfsInfo = LfsPackLog;
    }

    return ThisLfcb;
}


//
//  Local support routine
//

VOID
LfsNormalizeBasicLogFile (
    IN OUT PLONGLONG FileSize,
    IN OUT PULONG LogPageSize,
    IN OUT PUSHORT LogClients,
    IN BOOLEAN UseDefaultLogPage
    )

/*++

Routine Description:

    This routine is called to normalize the values which describe the
    log file.  It will make the log page a multiple of the system page.
    Finally we make sure the file size ends on a log page boundary.

    On input all of the parameters have the requested values, on return
    they have the values to use.

Arguments:

    FileSize - Stated size of the log file.

    LogPageSize - Suggested size for the log page.

    LogClients - Requested number of log clients.

    UseDefaultLogPage - Indicates if we should use the hardwired log page size or base
        it on the system page size.

Return Value:

    None.

--*/

{
    ULONG LocalLogPageSize;
    LONGLONG RestartPageBytes;
    LONGLONG LogPages;

    USHORT MaximumClients;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsNormalizeBasicLogFile:  Entered\n", 0 );

    //
    //  We always make the log page the same as the system page size.
    //  This may change in the future.
    //

    if (!UseDefaultLogPage) {

        *LogPageSize = PAGE_SIZE;

    } else {

        *LogPageSize = LFS_DEFAULT_LOG_PAGE_SIZE;
    }

    //
    //  If the log file is greater than the maximum log file size, we
    //  set the log file size to the maximum size.
    //

    if (*FileSize > LfsMaximumFileSize) {

        *FileSize = LfsMaximumFileSize;
    }

    //
    //  We round the file size down to a system page boundary.  This
    //  may also change if we allow non-system page sized log pages.
    //

    *(PULONG)FileSize &= ~(*LogPageSize - 1);

    //
    //  There better be at least 2 restart pages.
    //

    RestartPageBytes = 2 * *LogPageSize;

    if (*FileSize <= RestartPageBytes) {

        DebugTrace(  0, Dbg, "Log file is too small\n", 0 );
        DebugTrace( -1, Dbg, "LfsValidateBasicLogFile:  Abnormal Exit\n", 0 );

        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  Now compute the number of log pages.
    //

    LogPages = *FileSize - RestartPageBytes;
    LocalLogPageSize = *LogPageSize >> 1;

    while (LocalLogPageSize) {

        LocalLogPageSize = LocalLogPageSize >> 1;
        LogPages = ((ULONGLONG)(LogPages)) >> 1;
    }

    //
    //  If there aren't enough log pages then raise an error condition.
    //

    if (((PLARGE_INTEGER)&LogPages)->HighPart == 0
        && (ULONG)LogPages < MINIMUM_LFS_PAGES) {

        DebugTrace(  0, Dbg, "Not enough log pages -> %08lx\n", LogPages.LowPart );
        DebugTrace( -1, Dbg, "LfsValidateBasicLogFile:  Abnormal Exit\n", 0 );

        ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
    }

    //
    //  Now we compute the amount of space available for log clients.
    //  We will limit the clients to half of the restart system page.
    //

    MaximumClients = (USHORT) ((*LogPageSize / 2) / sizeof( LFS_CLIENT_RECORD ));

    if (*LogClients == 0) {

        *LogClients = 1;

    } else if (*LogClients > MaximumClients) {

        *LogClients = MaximumClients;
    }

    DebugTrace( -1, Dbg, "LfsNormalizeBasicLogFile:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
LfsUpdateLfcbFromPgHeader (
    IN PLFCB Lfcb,
    IN ULONG SystemPageSize,
    IN ULONG LogPageSize,
    IN SHORT MajorVersion,
    IN SHORT MinorVersion,
    IN BOOLEAN PackLog
    )

/*++

Routine Description:

    This routine updates the values in the Lfcb which depend on values in the
    restart page header.

Arguments:

    Lfcb - Log file control block to update.

    SystemPageSize - System page size to use.

    LogPageSize - Log page size to use.

    MajorVersion - Major version number for Lfs.

    MinorVersion - Minor version number for Lfs.

    PackLog - Indicates if we are packing the log file.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsUpdateLfcbFromPgHeader:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb              -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "System Page Size  -> %08lx\n", SystemPageSize );
    DebugTrace(  0, Dbg, "Log Page Size     -> %08lx\n", LogPageSize );
    DebugTrace(  0, Dbg, "Major Version     -> %04x\n", MajorVersion );
    DebugTrace(  0, Dbg, "Minor Version     -> %04x\n", MinorVersion );

    //
    //  Compute the restart page values.
    //

    Lfcb->SystemPageSize = SystemPageSize;
    Lfcb->SystemPageMask = SystemPageSize - 1;
    Lfcb->SystemPageInverseMask = ~Lfcb->SystemPageMask;

    //
    //  Do the same for the log pages.
    //

    Lfcb->LogPageSize = LogPageSize;
    Lfcb->LogPageMask = LogPageSize - 1;
    Lfcb->LogPageInverseMask = ~Lfcb->LogPageMask;

    Lfcb->LogPageShift = 0;

    while (TRUE) {

        LogPageSize = LogPageSize >> 1;

        if (LogPageSize == 0) {

            break;
        }

        Lfcb->LogPageShift += 1;
    }

    //
    //  If we are packing the log file then the first log page is page
    //  4.  Otherwise it is page 2.  Use the PackLog value to determine the
    //  Usa values.
    //

    Lfcb->FirstLogPage = Lfcb->SystemPageSize << 1;

    if (PackLog) {

        Lfcb->FirstLogPage = ( Lfcb->LogPageSize << 1 ) + Lfcb->FirstLogPage;

        Lfcb->LogRecordUsaOffset = (USHORT) LFS_PACKED_RECORD_PAGE_HEADER_SIZE;

        SetFlag( Lfcb->Flags, LFCB_PACK_LOG );

    } else {

        Lfcb->LogRecordUsaOffset = (USHORT) LFS_UNPACKED_RECORD_PAGE_HEADER_SIZE;
    }

    //
    //  Remember the values for the version numbers.
    //

    Lfcb->MajorVersion = MajorVersion;
    Lfcb->MinorVersion = MinorVersion;

    //
    //  Compute the offsets for the update sequence arrays.
    //

    Lfcb->RestartUsaOffset = LFS_RESTART_PAGE_HEADER_SIZE;

    Lfcb->RestartUsaArraySize = (USHORT) UpdateSequenceArraySize( (ULONG)Lfcb->SystemPageSize );
    Lfcb->LogRecordUsaArraySize = (USHORT) UpdateSequenceArraySize( (ULONG)Lfcb->LogPageSize );

    DebugTrace( -1, Dbg, "LfsUpdateLfcbFromPgHeader:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
LfsUpdateLfcbFromNoRestart (
    IN PLFCB Lfcb,
    IN LONGLONG FileSize,
    IN LSN LastLsn,
    IN ULONG LogClients,
    IN ULONG OpenLogCount,
    IN BOOLEAN LogFileWrapped,
    IN BOOLEAN UseMultiplePageIo
    )

/*++

Routine Description:

    This routine updates the values in the Lfcb in cases when we don't have a
    restart area to use.

Arguments:

    Lfcb - Log file control block to update.

    FileSize - Log file size.  This is the usable size of the log file.  It has
        already been adjusted to the log page size.

    LastLsn - This is the last Lsn to use for the disk.

    LogClients - This is the number of clients supported.

    OpenLogCount - This is the current count of opens for this log file.

    LogFileWrapped - Indicates if the log file has wrapped.

    UseMultiplePageIo - Indicates if we should be using large i/o transfers.

Return Value:

    None.

--*/

{
    ULONG Count;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsUpdateLfcbFromNoRestart:  Entered\n", 0 );

    Lfcb->FileSize = FileSize;

    //
    //  We can compute the number of bits needed for the file size by shifting
    //  until the size is 0.  We then can subtract 3 bits to account for
    //  quadaligning all file offsets for log records.
    //

    for (Count = 0;
         ( FileSize != 0 );
         Count += 1,
         FileSize = ((ULONGLONG)(FileSize)) >> 1) {
    }

    Lfcb->FileDataBits = Count - 3;

    Lfcb->SeqNumberBits = (sizeof( LSN ) * 8) - Lfcb->FileDataBits;

    //
    //  We get a starting sequence number from the given Lsn.
    //  We add 2 to this for our starting sequence number.
    //

    Lfcb->SeqNumber = LfsLsnToSeqNumber( Lfcb, LastLsn ) + 2;

    Lfcb->SeqNumberForWrap = Lfcb->SeqNumber + 1;

    Lfcb->NextLogPage = Lfcb->FirstLogPage;

    SetFlag( Lfcb->Flags, LFCB_NO_LAST_LSN | LFCB_NO_OLDEST_LSN );

    //
    //  The oldest Lsn is contructed from the sequence number.
    //

    Lfcb->OldestLsn.QuadPart = LfsFileOffsetToLsn( Lfcb, 0, Lfcb->SeqNumber );
    Lfcb->OldestLsnOffset = 0;

    Lfcb->LastFlushedLsn = Lfcb->OldestLsn;

    //
    //  Set the correct flags for the I/O and indicate if we have wrapped.
    //

    if (LogFileWrapped) {

        SetFlag( Lfcb->Flags, LFCB_LOG_WRAPPED );
    }

    if (UseMultiplePageIo) {

        SetFlag( Lfcb->Flags, LFCB_MULTIPLE_PAGE_IO );
    }

    //
    //  Compute the Log page values.
    //

    (ULONG)Lfcb->LogPageDataOffset = QuadAlign( Lfcb->LogRecordUsaOffset
                                                 + (sizeof( UPDATE_SEQUENCE_NUMBER ) * Lfcb->LogRecordUsaArraySize) );

    Lfcb->LogPageDataSize = Lfcb->LogPageSize - Lfcb->LogPageDataOffset;
    Lfcb->RecordHeaderLength = LFS_RECORD_HEADER_SIZE;

    if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

        //
        //  Allocate the Lbcb for the tail of the packed log file.
        //

        LfsAllocateLbcb( Lfcb, &Lfcb->PrevTail );
        Lfcb->PrevTail->FileOffset = Lfcb->FirstLogPage - Lfcb->LogPageSize;

        LfsAllocateLbcb( Lfcb, &Lfcb->ActiveTail );
        Lfcb->ActiveTail->FileOffset = Lfcb->PrevTail->FileOffset - Lfcb->LogPageSize;

        //
        //  Remember the different page sizes for reservation.
        //

        (ULONG)Lfcb->ReservedLogPageSize = (ULONG)Lfcb->LogPageDataSize - Lfcb->RecordHeaderLength;

    } else {

        (ULONG)Lfcb->ReservedLogPageSize = (ULONG)Lfcb->LogPageDataSize;
    }

    //
    //  Compute the restart page values.
    //

    Lfcb->RestartDataOffset = QuadAlign( LFS_RESTART_PAGE_HEADER_SIZE
                                         + (sizeof( UPDATE_SEQUENCE_NUMBER ) * Lfcb->RestartUsaArraySize) );

    Lfcb->RestartDataSize = (ULONG)Lfcb->SystemPageSize - Lfcb->RestartDataOffset;

    Lfcb->LogClients = (USHORT) LogClients;

    Lfcb->ClientArrayOffset = FIELD_OFFSET( LFS_RESTART_AREA, LogClientArray );

    Lfcb->RestartAreaSize = Lfcb->ClientArrayOffset
                            + (sizeof( LFS_CLIENT_RECORD ) * Lfcb->LogClients );

    Lfcb->CurrentOpenLogCount = OpenLogCount;

    //
    //  The total available log file space is the number of log file pages times
    //  the space available on each page.
    //

    Lfcb->TotalAvailInPages = Lfcb->FileSize - Lfcb->FirstLogPage;
    Lfcb->TotalAvailable = Int64ShrlMod32(((ULONGLONG)(Lfcb->TotalAvailInPages)), Lfcb->LogPageShift);

    //
    //  If the log file is packed we assume that we can't use the end of the
    //  page less than the file record size.  Then we won't need to reserve more
    //  than the caller asks for.
    //

    Lfcb->MaxCurrentAvail = Lfcb->TotalAvailable * (ULONG)Lfcb->ReservedLogPageSize;

    Lfcb->TotalAvailable = Lfcb->TotalAvailable * (ULONG)Lfcb->LogPageDataSize;

    Lfcb->CurrentAvailable = Lfcb->MaxCurrentAvail;

    DebugTrace( -1, Dbg, "LfsUpdateLfcbFromNoRestart:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//

VOID
LfsUpdateLfcbFromRestart (
    IN OUT PLFCB Lfcb,
    IN PLFS_RESTART_AREA RestartArea,
    IN USHORT RestartOffset
    )

/*++

Routine Description:

    This routine updates the values in the Lfcb based on data in the
    restart area.

Arguments:

    Lfcb - Log file control block to update.

    RestartArea - Restart area to use to update the Lfcb.

    RestartOffset - This is the offset to the restart area in the restart page.

Return Value:

    None.

--*/

{
    LONGLONG LsnFileOffset;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsUpdateLfcbFromRestartArea:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb          -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "RestartArea   -> %08lx\n", RestartArea );

    Lfcb->FileSize = RestartArea->FileSize;

    //
    //  We get the sequence number bits from the restart area and compute the
    //  file data bits.
    //

    Lfcb->SeqNumberBits = RestartArea->SeqNumberBits;
    Lfcb->FileDataBits = (sizeof( LSN ) * 8) - Lfcb->SeqNumberBits;

    //
    //  We look at the last flushed Lsn to determine the current sequence count and
    //  the next log page to examine.
    //

    Lfcb->LastFlushedLsn = RestartArea->CurrentLsn;

    Lfcb->SeqNumber = LfsLsnToSeqNumber( Lfcb, Lfcb->LastFlushedLsn );
    Lfcb->SeqNumberForWrap = Lfcb->SeqNumber + 1;

    //
    //  The restart area size depends on the number of clients and whether the
    //  the file is packed.
    //

    Lfcb->LogClients = RestartArea->LogClients;

    //
    //  Compute the restart page values from the restart offset.
    //

    Lfcb->RestartDataOffset = RestartOffset;
    Lfcb->RestartDataSize = (ULONG)Lfcb->SystemPageSize - RestartOffset;

    //
    //  For a packed log file we can find the following values in the restart
    //  area.  Otherwise we compute them from the current structure sizes.
    //

    if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

        Lfcb->RecordHeaderLength = RestartArea->RecordHeaderLength;

        Lfcb->ClientArrayOffset = RestartArea->ClientArrayOffset;

        Lfcb->RestartAreaSize = RestartArea->RestartAreaLength;

        (ULONG)Lfcb->LogPageDataOffset = RestartArea->LogPageDataOffset;
        Lfcb->LogPageDataSize = Lfcb->LogPageSize - Lfcb->LogPageDataOffset;

        //
        //  For packed files we allocate the tail Lbcbs.
        //

        //
        //  Allocate the Lbcb for the tail of the packed log file.
        //

        LfsAllocateLbcb( Lfcb, &Lfcb->PrevTail );
        Lfcb->PrevTail->FileOffset = Lfcb->FirstLogPage - Lfcb->LogPageSize;

        LfsAllocateLbcb( Lfcb, &Lfcb->ActiveTail );
        Lfcb->ActiveTail->FileOffset = Lfcb->PrevTail->FileOffset - Lfcb->LogPageSize;

        //
        //  Remember the different page sizes for reservation.
        //

        (ULONG)Lfcb->ReservedLogPageSize = (ULONG)Lfcb->LogPageDataSize - Lfcb->RecordHeaderLength;

    } else {

        Lfcb->RecordHeaderLength = LFS_RECORD_HEADER_SIZE;
        Lfcb->ClientArrayOffset = FIELD_OFFSET( LFS_OLD_RESTART_AREA, LogClientArray );

        Lfcb->RestartAreaSize = Lfcb->ClientArrayOffset
                                + (sizeof( LFS_CLIENT_RECORD ) * Lfcb->LogClients);

        (ULONG)Lfcb->LogPageDataOffset = QuadAlign( Lfcb->LogRecordUsaOffset
                                                     + (sizeof( UPDATE_SEQUENCE_NUMBER ) * Lfcb->LogRecordUsaArraySize) );

        Lfcb->LogPageDataSize = Lfcb->LogPageSize - Lfcb->LogPageDataOffset;

        (ULONG)Lfcb->ReservedLogPageSize = (ULONG)Lfcb->LogPageDataSize;
    }

    //
    //  If the current last flushed Lsn offset is before the first log page
    //  then this is a pseudo Lsn.
    //

    LsnFileOffset = LfsLsnToFileOffset( Lfcb, Lfcb->LastFlushedLsn );

    if ( LsnFileOffset < Lfcb->FirstLogPage ) {

        SetFlag( Lfcb->Flags, LFCB_NO_LAST_LSN );
        Lfcb->NextLogPage = Lfcb->FirstLogPage;

    //
    //  Otherwise look at the last Lsn to determine where it ends in the file.
    //

    } else {

        LONGLONG LsnFinalOffset;
        BOOLEAN Wrapped;

        ULONG DataLength;
        ULONG RemainingPageBytes;

        DataLength = RestartArea->LastLsnDataLength;

        //
        //  Find the end of this log record.
        //

        LfsLsnFinalOffset( Lfcb,
                           Lfcb->LastFlushedLsn,
                           DataLength,
                           &LsnFinalOffset );

        //
        //  If we wrapped in the file then increment the sequence number.
        //

        if ( LsnFinalOffset <= LsnFileOffset ) {

            Lfcb->SeqNumber = 1 + Lfcb->SeqNumber;

            SetFlag( Lfcb->Flags, LFCB_LOG_WRAPPED );
        }

        //
        //  Now compute the next log page to use.  If we are packing the log file
        //  we will attempt to use the same page.
        //

        LfsTruncateOffsetToLogPage( Lfcb, LsnFinalOffset, &LsnFileOffset );

        RemainingPageBytes = (ULONG)Lfcb->LogPageSize
                             - ((((ULONG)LsnFinalOffset) & Lfcb->LogPageMask) + 1);

        //
        //  If we are packing the log file and we can fit another log record on the
        //  page, move back a page in the log file.
        //

        if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG )
            && (RemainingPageBytes >= Lfcb->RecordHeaderLength)) {

            SetFlag( Lfcb->Flags, LFCB_REUSE_TAIL );
            Lfcb->NextLogPage = LsnFileOffset;
            Lfcb->ReusePageOffset = (ULONG)Lfcb->LogPageSize - RemainingPageBytes;

        } else {

            LfsNextLogPageOffset( Lfcb, LsnFileOffset, &Lfcb->NextLogPage, &Wrapped );
        }
    }

    //
    //  Find the oldest client Lsn.  Use the last flushed Lsn as a starting point.
    //

    Lfcb->OldestLsn = Lfcb->LastFlushedLsn;

    LfsFindOldestClientLsn( RestartArea,
                            Add2Ptr( RestartArea, Lfcb->ClientArrayOffset, PLFS_CLIENT_RECORD ),
                            &Lfcb->OldestLsn );

    Lfcb->OldestLsnOffset = LfsLsnToFileOffset( Lfcb, Lfcb->OldestLsn );

    //
    //  If there is no oldest client Lsn, then update the flag in the Lfcb.
    //

    if ( Lfcb->OldestLsnOffset < Lfcb->FirstLogPage ) {

        SetFlag( Lfcb->Flags, LFCB_NO_OLDEST_LSN );
    }

    //
    //  We need to determine the flags for the Lfcb.  These flags let us know
    //  if we wrapped in the file and if we are using multiple page I/O.
    //

    if (!FlagOn( RestartArea->Flags, RESTART_SINGLE_PAGE_IO )) {

        SetFlag( Lfcb->Flags, LFCB_LOG_WRAPPED | LFCB_MULTIPLE_PAGE_IO );
    }

    //
    //  Remember the current open log count from the disk.  We may be plucking random data out
    //  of the client area if the restart area hasn't been grown yet but we will detect that
    //  elsewhere.
    //

    Lfcb->CurrentOpenLogCount = RestartArea->RestartOpenLogCount;

    //
    //  The total available log file space is the number of log file pages times
    //  the space available on each page.
    //

    Lfcb->TotalAvailInPages = Lfcb->FileSize - Lfcb->FirstLogPage;

    Lfcb->TotalAvailable = Int64ShrlMod32(((ULONGLONG)(Lfcb->TotalAvailInPages)), Lfcb->LogPageShift);

    //
    //  If the log file is packed we assume that we can't use the end of the
    //  page less than the file record size.  Then we won't need to reserve more
    //  than the caller asks for.
    //

    Lfcb->MaxCurrentAvail = Lfcb->TotalAvailable * (ULONG)Lfcb->ReservedLogPageSize;

    Lfcb->TotalAvailable = Lfcb->TotalAvailable * (ULONG)Lfcb->LogPageDataSize;

    LfsFindCurrentAvail( Lfcb );

    DebugTrace( -1, Dbg, "LfsUpdateLfcbFromRestartArea:  Exit\n", 0 );

    return;
}


//
//  Local support routine
//

VOID
LfsUpdateRestartAreaFromLfcb (
    IN PLFCB Lfcb,
    IN PLFS_RESTART_AREA RestartArea
    )

/*++

Routine Description:

    This routine is called to update a restart area from the values stored
    in the Lfcb.  This is typically done in a case where we won't use
    any of the current values in the restart area.

Arguments:

    Lfcb - Log file control block.

    RestartArea - Restart area to update.

Return Value:

    None.

--*/

{
    PLFS_CLIENT_RECORD Client;
    USHORT ClientIndex;
    USHORT PrevClient = LFS_NO_CLIENT;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsUpdateRestartAreaFromLfcb:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb              -> %08lx\n", Lfcb );

    //
    //  We can copy most of the fields directly out of Lfcb.
    //

    RestartArea->CurrentLsn = Lfcb->LastFlushedLsn;
    RestartArea->LogClients = Lfcb->LogClients;

    if (!FlagOn( Lfcb->Flags, LFCB_MULTIPLE_PAGE_IO )) {

        SetFlag( RestartArea->Flags, RESTART_SINGLE_PAGE_IO );
    }

    RestartArea->SeqNumberBits = Lfcb->SeqNumberBits;

    RestartArea->FileSize = Lfcb->FileSize;
    RestartArea->LastLsnDataLength = 0;
    RestartArea->ClientArrayOffset = Lfcb->ClientArrayOffset;
    RestartArea->RestartAreaLength = (USHORT) Lfcb->RestartAreaSize;

    RestartArea->RecordHeaderLength = Lfcb->RecordHeaderLength;
    RestartArea->LogPageDataOffset = (USHORT)Lfcb->LogPageDataOffset;

    //
    //  We set the in use list as empty and the free list as containing
    //  all of the client entries.
    //

    RestartArea->ClientInUseList = LFS_NO_CLIENT;
    RestartArea->ClientFreeList = 0;

    for (ClientIndex = 1,
         Client = Add2Ptr( RestartArea, Lfcb->ClientArrayOffset, PLFS_CLIENT_RECORD );
         ClientIndex < Lfcb->LogClients;
         ClientIndex += 1,
         Client++) {

        Client->PrevClient = PrevClient;
        Client->NextClient = ClientIndex;

        PrevClient = ClientIndex - 1;
    }

    //
    //  We're now at the last client.
    //

    Client->PrevClient = PrevClient;
    Client->NextClient = LFS_NO_CLIENT;

    //
    //  Use the current value out of the Lfcb to stamp this usage of the log file.
    //

    RestartArea->RestartOpenLogCount = Lfcb->CurrentOpenLogCount + 1;

    DebugTrace( -1, Dbg, "LfsUpdateRestartAreaFromLfcb:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//

VOID
LfsInitializeLogFilePriv (
    IN PLFCB Lfcb,
    IN BOOLEAN ForceRestartToDisk,
    IN ULONG RestartAreaSize,
    IN LONGLONG StartOffsetForClear,
    IN BOOLEAN ClearLogFile
    )

/*++

Routine Description:

    This routine is our internal routine for initializing a log file.
    This can be the case where we are updating the log file for
    update sequence array, or differing page size or new log file size.

Arguments:

    Lfcb - This is the Lfcb for this log file.  It should already have
        the version number information stored.

    ForceRestartToDisk - Indicates that we want to actually force restart
        areas to disk instead of simply queueing them to the start of the
        workqueue.

    RestartAreaSize - This is the size for the restart areas.  This may
        be larger than the size in the Lfcb because we may be clearing
        stale data out of the file.

    StartOffsetForClear - If we are clearing the file we want to uninitialize
        from this point.

    ClearLogFile - Indicates if we want to uninitialize the log file to
        remove stale data.  This is done specifically when changing
        system page sizes.

Return Value:

    None

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsInitializeLogFilePriv:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb                  -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Force Restart         -> %04x\n", ForceRestartToDisk );
    DebugTrace(  0, Dbg, "RestartAreaSize       -> %08lx\n", RestartAreaSize );
    DebugTrace(  0, Dbg, "StartOffset (Low)     -> %08lx\n", StartOffsetForClear.LowPart );
    DebugTrace(  0, Dbg, "StartOffset (High)    -> %08lx\n", StartOffsetForClear.HighPart );
    DebugTrace(  0, Dbg, "Clear Log File        -> %04x\n", ClearLogFile );

    //
    //  We start by queueing the restart areas.
    //

    LfsWriteLfsRestart( Lfcb,
                        RestartAreaSize,
                        FALSE );

    LfsWriteLfsRestart( Lfcb,
                        RestartAreaSize,
                        ForceRestartToDisk );

    //
    //  If we are to clear the log file, we write all 0xff into the
    //  log pages beginning at the log page offset.
    //

    if (ClearLogFile) {

        PCHAR LogPage;
        PBCB LogPageBcb = NULL;

        try {

            while ( StartOffsetForClear < Lfcb->FileSize ) {

                BOOLEAN UsaError;

                //
                //  We'll do the best we can and ignore all errors.
                //

                if (NT_SUCCESS( LfsPinOrMapData( Lfcb,
                                                 StartOffsetForClear,
                                                 (ULONG)Lfcb->LogPageSize,
                                                 TRUE,
                                                 FALSE,
                                                 TRUE,
                                                 &UsaError,
                                                 (PVOID *) &LogPage,
                                                 &LogPageBcb ))) {

                    RtlFillMemoryUlong( (PVOID)LogPage,
                                        (ULONG)Lfcb->LogPageSize,
                                        LFS_SIGNATURE_UNINITIALIZED_ULONG );

                    LfsFlushLogPage( Lfcb,
                                     LogPage,
                                     StartOffsetForClear,
                                     &LogPageBcb );

                    StartOffsetForClear = Lfcb->LogPageSize + StartOffsetForClear;
                }
            }

        } finally {

            if (LogPageBcb != NULL) {

                CcUnpinData( LogPageBcb );
            }
        }
    }

    DebugTrace( -1, Dbg, "LfsInitializeLogFilePriv:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//

VOID
LfsFindLastLsn (
    IN OUT PLFCB Lfcb
    )

/*++

Routine Description:

    This routine walks through the log pages for a file, searching for the
    last log page written to the file.  It updates the Lfcb and the current
    restart area as well.

    We proceed in the following manner.

        1 - Walk through and find all of the log pages successfully
            flushed to disk.  This search terminates when either we find
            an error or when we find a previous page on the disk.

        2 - For the error case above, we want to insure that the error found
            was due to a system crash and that there are no complete I/O
            transfers after the bad region.

        3 - We will look at the 2 pages with the tail copies if the log file
            is packed to check on pages with errors.

    At the end of this routine we will repair the log file by copying the tail
    copies back to their correct location in the log file.

Arguments:

    Lfcb - Log file control block for this log file.

Return Value:

    None.

--*/

{
    USHORT PageCount;
    USHORT PagePosition;

    LONGLONG CurrentLogPageOffset;
    LONGLONG NextLogPageOffset;

    LSN LastKnownLsn;

    BOOLEAN Wrapped;
    BOOLEAN WrappedLogFile = FALSE;

    LONGLONG ExpectedSeqNumber;

    LONGLONG FirstPartialIo;
    ULONG PartialIoCount = 0;

    PLFS_RECORD_PAGE_HEADER LogPageHeader;
    PBCB LogPageHeaderBcb = NULL;

    PLFS_RECORD_PAGE_HEADER TestPageHeader;
    PBCB TestPageHeaderBcb = NULL;

    LONGLONG FirstTailFileOffset;
    PLFS_RECORD_PAGE_HEADER FirstTailPage;
    LONGLONG FirstTailOffset = 0;
    PBCB FirstTailPageBcb = NULL;

    LONGLONG SecondTailFileOffset;
    PLFS_RECORD_PAGE_HEADER SecondTailPage;
    LONGLONG SecondTailOffset = 0;
    PBCB SecondTailPageBcb = NULL;

    PLFS_RECORD_PAGE_HEADER TailPage;

    BOOLEAN UsaError;
    BOOLEAN ReplacePage = FALSE;
    BOOLEAN ValidFile = FALSE;

    BOOLEAN InitialReusePage = FALSE;

    NTSTATUS Status;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFindLastLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb  -> %08lx\n", Lfcb );

    //
    //  The page count and page position are from the last page
    //  sucessfully read.  Initialize these to indicate the
    //  'previous' transfer was complete.
    //

    PageCount = 1;
    PagePosition = 1;

    //
    //  We have the current Lsn in the restart area.  This is the last
    //  Lsn on a log page.  We compute the next file offset and sequence
    //  number.
    //

    CurrentLogPageOffset = Lfcb->NextLogPage;

    //
    //  If the next log page is the first log page in the file and
    //  the last Lsn represented a log record, then remember that we
    //  have wrapped in the log file.
    //

    if (( CurrentLogPageOffset == Lfcb->FirstLogPage )
        && !FlagOn( Lfcb->Flags, LFCB_NO_LAST_LSN | LFCB_REUSE_TAIL )) {

        ExpectedSeqNumber = Lfcb->SeqNumber + 1;
        WrappedLogFile = TRUE;

    } else {

        ExpectedSeqNumber = Lfcb->SeqNumber;
    }

    //
    //  If we are going to try to reuse the tail of the last known
    //  page, then remember the last Lsn on this page.
    //

    if (FlagOn( Lfcb->Flags, LFCB_REUSE_TAIL )) {

        LastKnownLsn = Lfcb->LastFlushedLsn;

        //
        //  There are some special conditions allowed for this page when
        //  we read it.  It could be either the first or last of the transfer.
        //  It may also have a tail copy.
        //

        InitialReusePage = TRUE;

    } else {

        LastKnownLsn = LfsLi0;
    }

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        //
        //  If this is a packed log file, let's pin the two tail copy pages.
        //

        if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

            //
            //  Start with the second page.
            //

            SecondTailFileOffset = Lfcb->FirstLogPage - Lfcb->LogPageSize;

            if (NT_SUCCESS( LfsPinOrMapData( Lfcb,
                                             SecondTailFileOffset,
                                             (ULONG)Lfcb->LogPageSize,
                                             TRUE,
                                             TRUE,
                                             TRUE,
                                             &UsaError,
                                             &SecondTailPage,
                                             &SecondTailPageBcb ))) {

                //
                //  If this isn't a valid page then ignore it.
                //

                if (UsaError
                    || *((PULONG) &SecondTailPage->MultiSectorHeader.Signature) != LFS_SIGNATURE_RECORD_PAGE_ULONG) {

                    CcUnpinData( SecondTailPageBcb );
                    SecondTailPageBcb = SecondTailPage = NULL;

                } else {

                    SecondTailOffset = SecondTailPage->Copy.FileOffset;
                }

            } else if (SecondTailPageBcb != NULL) {

                CcUnpinData( SecondTailPageBcb );
                SecondTailPageBcb = SecondTailPage = NULL;
            }

            FirstTailFileOffset = SecondTailFileOffset - Lfcb->LogPageSize;

            //
            //  Now try the first.
            //

            if (NT_SUCCESS( LfsPinOrMapData( Lfcb,
                                             FirstTailFileOffset,
                                             (ULONG)Lfcb->LogPageSize,
                                             TRUE,
                                             TRUE,
                                             TRUE,
                                             &UsaError,
                                             &FirstTailPage,
                                             &FirstTailPageBcb ))) {

                //
                //  If this isn't a valid page then ignore it.
                //

                if (UsaError
                    || *((PULONG) &FirstTailPage->MultiSectorHeader.Signature) != LFS_SIGNATURE_RECORD_PAGE_ULONG) {

                    CcUnpinData( FirstTailPageBcb );
                    FirstTailPageBcb = FirstTailPage = NULL;

                } else {

                    FirstTailOffset = FirstTailPage->Copy.FileOffset;
                }

            } else if (FirstTailPageBcb != NULL) {

                CcUnpinData( FirstTailPageBcb );
                FirstTailPageBcb = FirstTailPage = NULL;
            }
        }

        //
        //  We continue walking through the file, log page by log page looking
        //  for the end of the data transferred.  The loop below looks for
        //  a log page which contains the end of a log record.  Each time a
        //  log record is successfully read from the disk, we update our in-memory
        //  structures to reflect this.  We exit this loop when we are at a point
        //  where we don't want to find any subsequent pages.  This occurs when
        //
        //      - we get an I/O error reading a page
        //      - we get a Usa error reading a page
        //      - we have a tail copy with more recent data than contained on the page
        //

        while (TRUE) {

            LONGLONG ActualSeqNumber;
            TailPage = NULL;

            //
            //  Pin the next log page, allowing errors.
            //

            Status = LfsPinOrMapData( Lfcb,
                                      CurrentLogPageOffset,
                                      (ULONG)Lfcb->LogPageSize,
                                      TRUE,
                                      TRUE,
                                      TRUE,
                                      &UsaError,
                                      (PVOID *) &LogPageHeader,
                                      &LogPageHeaderBcb );

            //
            //  Compute the next log page offset in the file.
            //

            LfsNextLogPageOffset( Lfcb,
                                  CurrentLogPageOffset,
                                  &NextLogPageOffset,
                                  &Wrapped );

            //
            //  If we are at the expected first page of a transfer
            //  check to see if either tail copy is at this offset.
            //  If this page is the last page of a transfer, check
            //  if we wrote a subsequent tail copy.
            //

            if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG ) &&
                ((PageCount == PagePosition) ||
                 (PageCount == PagePosition + 1))) {

                //
                //  Check if the offset matches either the first or second
                //  tail copy.  It is possible it will match both.
                //

                if (CurrentLogPageOffset == FirstTailOffset) {

                    TailPage = FirstTailPage;
                }

                if (CurrentLogPageOffset == SecondTailOffset) {

                    //
                    //  If we already matched on the first page then
                    //  check the ending Lsn's.
                    //

                    if ((TailPage == NULL) ||
                        (SecondTailPage->Header.Packed.LastEndLsn.QuadPart >
                         FirstTailPage->Header.Packed.LastEndLsn.QuadPart )) {

                        TailPage = SecondTailPage;
                    }
                }

                //
                //  If we have a candidate for a tail copy, check and see if it is
                //  in the expected pass through the file.  For that to be true we
                //  must be at the first page of an I/O block. Also the last Lsn on the
                //  copy page must match the last known flushed Lsn or the sequence
                //  number on the page must be the expected sequence number.
                //

                if (TailPage) {

                    if (LastKnownLsn.QuadPart < TailPage->Header.Packed.LastEndLsn.QuadPart) {

                        ActualSeqNumber = LfsLsnToSeqNumber( Lfcb, TailPage->Header.Packed.LastEndLsn );

                        //
                        //  If the sequence number is not expected, then don't use the tail
                        //  copy.
                        //

                        if (ExpectedSeqNumber != ActualSeqNumber) {

                            TailPage = NULL;
                        }

                    //
                    //  If the last Lsn is greater than the one on this page
                    //  then forget this tail.
                    //

                    } else if (LastKnownLsn.QuadPart > TailPage->Header.Packed.LastEndLsn.QuadPart) {

                        TailPage = NULL;
                    }
                }
            }

            //
            //  If we have an error on the current page, we will break out of
            //  this loop.
            //

            if (!NT_SUCCESS( Status ) || UsaError) {

                break;
            }

            //
            //  If the last Lsn on this page doesn't match the previous
            //  known last Lsn or the sequence number is not expected
            //  we are done.
            //

            ActualSeqNumber = LfsLsnToSeqNumber( Lfcb,
                                                 LogPageHeader->Copy.LastLsn );

            if ((LastKnownLsn.QuadPart != LogPageHeader->Copy.LastLsn.QuadPart) &&
                (ActualSeqNumber != ExpectedSeqNumber)) {

                break;
            }

            //
            //  Check that the page position and page count values are correct.
            //  If this is the first page of a transfer the position must be
            //  1 and the count will be unknown.
            //

            if (PageCount == PagePosition) {

                //
                //  If the current page is the first page we are looking at
                //  and we are reusing this page then it can be either the
                //  first or last page of a transfer.  Otherwise it can only
                //  be the first.
                //

                if ((LogPageHeader->PagePosition != 1) &&
                    (!InitialReusePage ||
                     (LogPageHeader->PagePosition != LogPageHeader->PageCount))) {

                    break;
                }

            //
            //  The page position better be 1 more than the last page position
            //  and the page count better match.
            //

            } else if ((LogPageHeader->PageCount != PageCount) ||
                       (LogPageHeader->PagePosition != PagePosition + 1)) {

                break;
            }

            //
            //  We have a valid page in the file and may have a valid page in
            //  the tail copy area.  If the tail page was written after
            //  the page in the file then break out of the loop.
            //

            if (TailPage &&
                (TailPage->Header.Packed.LastEndLsn.QuadPart >= LogPageHeader->Copy.LastLsn.QuadPart)) {

                //
                //  Remember if we will replace the page.
                //

                ReplacePage = TRUE;
                break;
            }

            TailPage = NULL;

            //
            //  The log page is expected.  If this contains the end of
            //  some log record we can update some fields in the Lfcb.
            //

            if (FlagOn( LogPageHeader->Flags, LOG_PAGE_LOG_RECORD_END )) {

                //
                //  Since we have read this page we know the Lfcb sequence
                //  number is the same as our expected value.  We also
                //  assume we will not reuse the tail.
                //

                Lfcb->SeqNumber = ExpectedSeqNumber;
                ClearFlag( Lfcb->Flags, LFCB_REUSE_TAIL );

                if (FlagOn( Lfcb->Flags, LFCB_PACK_LOG )) {

                    Lfcb->LastFlushedLsn = LogPageHeader->Header.Packed.LastEndLsn;

                    //
                    //  If there is room on this page for another header then
                    //  remember we want to reuse the page.
                    //

                    if (Lfcb->RecordHeaderLength <=
                        ((ULONG)Lfcb->LogPageSize - LogPageHeader->Header.Packed.NextRecordOffset )) {

                        SetFlag( Lfcb->Flags, LFCB_REUSE_TAIL );
                        Lfcb->ReusePageOffset = LogPageHeader->Header.Packed.NextRecordOffset;
                    }

                } else {

                    Lfcb->LastFlushedLsn = LogPageHeader->Copy.LastLsn;
                }

                Lfcb->RestartArea->CurrentLsn = Lfcb->LastFlushedLsn;

                ClearFlag( Lfcb->Flags, LFCB_NO_LAST_LSN );

                //
                //  If we may try to reuse the current page then use
                //  that as the next page offset.  Otherwise move to the
                //  next page in the file.
                //

                if (FlagOn( Lfcb->Flags, LFCB_REUSE_TAIL )) {

                    Lfcb->NextLogPage = CurrentLogPageOffset;

                } else {

                    Lfcb->NextLogPage = NextLogPageOffset;
                }

                //
                //  If we wrapped the log file, then we set the bit indicating so.
                //

                if (WrappedLogFile) {

                    SetFlag( Lfcb->Flags, LFCB_LOG_WRAPPED );
                }
            }

            //
            //  Remember the last page count and position.  Also remember
            //  the last known lsn.
            //

            PageCount = LogPageHeader->PageCount;
            PagePosition = LogPageHeader->PagePosition;
            LastKnownLsn = LogPageHeader->Copy.LastLsn;

            //
            //  If we are wrapping to the beginning of the file then update
            //  the expected sequence number.
            //

            if (Wrapped) {

                ExpectedSeqNumber = ExpectedSeqNumber + 1;
                WrappedLogFile = TRUE;
            }

            CurrentLogPageOffset = NextLogPageOffset;

            //
            //  Unpin the last log page pinned.
            //

            CcUnpinData( LogPageHeaderBcb );
            LogPageHeaderBcb = NULL;

            InitialReusePage = FALSE;
        }

        //
        //  At this point we expect that there will be no more new pages in
        //  the log file.  We could have had an error of some sort on the most recent
        //  page or we may have found a tail copy for the current page.
        //  If the error occurred on the last Io to the file then
        //  this log file is useful.  Otherwise the log file can't be used.
        //

        //
        //  If we have a tail copy page then update the values in the
        //  Lfcb and restart area.
        //

        if (TailPage != NULL) {

            //
            //  Since we have read this page we know the Lfcb sequence
            //  number is the same as our expected value.
            //

            Lfcb->SeqNumber = ExpectedSeqNumber;

            Lfcb->LastFlushedLsn = TailPage->Header.Packed.LastEndLsn;

            Lfcb->RestartArea->CurrentLsn = Lfcb->LastFlushedLsn;

            ClearFlag( Lfcb->Flags, LFCB_NO_LAST_LSN );

            //
            //  If there is room on this page for another header then
            //  remember we want to reuse the page.
            //

            if (((ULONG)Lfcb->LogPageSize - TailPage->Header.Packed.NextRecordOffset )
                >= Lfcb->RecordHeaderLength) {

                SetFlag( Lfcb->Flags, LFCB_REUSE_TAIL );
                Lfcb->NextLogPage = CurrentLogPageOffset;
                Lfcb->ReusePageOffset = TailPage->Header.Packed.NextRecordOffset;

            } else {

                ClearFlag( Lfcb->Flags, LFCB_REUSE_TAIL );
                Lfcb->NextLogPage = NextLogPageOffset;
            }

            //
            //  If we wrapped the log file, then we set the bit indicating so.
            //

            if (WrappedLogFile) {

                SetFlag( Lfcb->Flags, LFCB_LOG_WRAPPED );
            }
        }

        //
        //  Remember that the partial IO will start at the next page.
        //

        FirstPartialIo = NextLogPageOffset;

        //
        //  If the next page is the first page of the file then update
        //  the sequence number for log records which begin on the next
        //  page.
        //

        if (Wrapped) {

            ExpectedSeqNumber = ExpectedSeqNumber + 1;
        }

        //
        //  If we know the length of the transfer containing the page we stopped
        //  on we can just go to the page following the transfer and check
        //  the sequence number.  If we replaced the page then we have already
        //  modified the numbers.  If we know that only single pages were written
        //  to disk then we will munge the numbers now.  If we were in the
        //  middle of a multi-page I/O then the numbers are already set up.
        //

        //
        //  If we have a tail copy or are performing single page I/O
        //  we can immediately look at the next page.
        //

        if (ReplacePage ||
            FlagOn( Lfcb->RestartArea->Flags, RESTART_SINGLE_PAGE_IO )) {

            //
            //  Fudge the counts to show that we don't need swallow any pages.
            //

            PageCount = 2;
            PagePosition = 1;

        //
        //  If the counts match it means the current page should be the first
        //  page of a transfer.  We need to walk forward enough to guarantee
        //  that there was no subsequent transfer that made it out to disk.
        //

        } else if (PagePosition == PageCount) {

            USHORT CurrentPosition;

            //
            //  If the next page causes us to wrap to the beginning of the log
            //  file then we know which page to check next.
            //

            if (Wrapped) {

                //
                //  Fudge the counts to show that we don't need swallow any pages.
                //

                PageCount = 2;
                PagePosition = 1;

            //
            //  Walk forward looking for a page which is from a different IO transfer
            //  from the page we failed on.
            //

            } else {

                //
                //  We need to find a log page we know is not part of the log
                //  page which caused the original error.
                //
                //  Maintain the count within the current transfer.
                //

                CurrentPosition = 2;

                do {

                    //
                    //  We walk through the file, reading log pages.  If we find
                    //  a readable log page that must lie in a subsequent Io block,
                    //  we exit.
                    //

                    if (TestPageHeaderBcb != NULL) {

                        CcUnpinData( TestPageHeaderBcb );
                        TestPageHeaderBcb = NULL;
                    }

                    Status = LfsPinOrMapData( Lfcb,
                                              NextLogPageOffset,
                                              (ULONG)Lfcb->LogPageSize,
                                              TRUE,
                                              TRUE,
                                              TRUE,
                                              &UsaError,
                                              (PVOID *) &TestPageHeader,
                                              &TestPageHeaderBcb );

                    //
                    //  If we get a USA error then assume that we correctly
                    //  found the end of the original transfer.
                    //

                    if (UsaError) {

                        ValidFile = TRUE;
                        break;

                    //
                    //  If we were able to read the page, we examine it to see
                    //  if it is in the same or different Io block.
                    //

                    } else if (NT_SUCCESS( Status )) {

                        //
                        //  If this page is part of the error causing I/O, we will
                        //  use the transfer length to determine the page to
                        //  read for a subsequent error.
                        //

                        if (TestPageHeader->PagePosition == CurrentPosition
                            && LfsCheckSubsequentLogPage( Lfcb,
                                                          TestPageHeader,
                                                          NextLogPageOffset,
                                                          ExpectedSeqNumber )) {

                            PageCount = TestPageHeader->PageCount + 1;
                            PagePosition = TestPageHeader->PagePosition;

                            break;

                        //
                        //  We found know the Io causing the error didn't
                        //  complete.  So we have no more checks to do.
                        //

                        } else {

                            ValidFile = TRUE;
                            break;
                        }

                    //
                    //  Try the next page.
                    //

                    } else {

                        //
                        //  Move to the next log page.
                        //

                        LfsNextLogPageOffset( Lfcb,
                                              NextLogPageOffset,
                                              &NextLogPageOffset,
                                              &Wrapped );

                        //
                        //  If the file wrapped then initialize the page count
                        //  and position so that we will not skip over any
                        //  pages in the final verification below.
                        //

                        if (Wrapped) {

                            ExpectedSeqNumber = ExpectedSeqNumber + 1;

                            PageCount = 2;
                            PagePosition = 1;
                        }

                        CurrentPosition += 1;
                    }

                    //
                    //  This is one more page we will want to uninitialize.
                    //

                    PartialIoCount += 1;

                } while( !Wrapped );
            }
        }

        //
        //  If we are unsure whether the file is valid then we will have
        //  the count and position in the current transfer.  We will walk through
        //  this transfer and read the subsequent page.
        //

        if (!ValidFile) {

            ULONG RemainingPages;

            //
            //  Skip over the remaining pages in this transfer.
            //

            RemainingPages = (PageCount - PagePosition) - 1;

            PartialIoCount += RemainingPages;

            while (RemainingPages--) {

                LfsNextLogPageOffset( Lfcb,
                                      NextLogPageOffset,
                                      &NextLogPageOffset,
                                      &Wrapped );

                if (Wrapped) {

                    ExpectedSeqNumber = ExpectedSeqNumber + 1;
                }
            }

            //
            //  Call our routine to check this log page.
            //

            if (TestPageHeaderBcb != NULL) {

                CcUnpinData( TestPageHeaderBcb );
                TestPageHeaderBcb = NULL;
            }

            Status = LfsPinOrMapData( Lfcb,
                                      NextLogPageOffset,
                                      (ULONG)Lfcb->LogPageSize,
                                      TRUE,
                                      TRUE,
                                      TRUE,
                                      &UsaError,
                                      (PVOID *) &TestPageHeader,
                                      &TestPageHeaderBcb );

            if (NT_SUCCESS( Status )
                && !UsaError) {

                if (LfsCheckSubsequentLogPage( Lfcb,
                                               TestPageHeader,
                                               NextLogPageOffset,
                                               ExpectedSeqNumber )) {

                    DebugTrace( 0, Dbg, "Log file is fatally flawed\n", 0 );
                    ExRaiseStatus( STATUS_DISK_CORRUPT_ERROR );
                }
            }

            ValidFile = TRUE;
        }

        //
        //  Make sure the current page is unpinned.
        //

        if (LogPageHeaderBcb != NULL) {

            CcUnpinData( LogPageHeaderBcb );
            LogPageHeaderBcb = NULL;
        }

        //
        //  We have a valid file.  The Lfcb is initialized to the point where
        //  the last log record was found.  We possibly have a copy of the
        //  last page in the log file stored as a copy.  Or we could just have
        //  a page that we would like to reuse the end of.
        //

        if (TailPage != NULL) {

#ifdef BENL_DBG
            KdPrint(( "LFS: copying tail page at 0x%x to 0x%I64x, 1st:0x%x 2nd:0x%x \n", TailPage, TailPage->Copy.FileOffset, FirstTailPage, SecondTailPage ));
#endif

            //
            //  We will pin the correct page and copy the data from this
            //  page into it.  We will then flush it out to disk.
            //

            LfsPinOrMapData( Lfcb,
                             TailPage->Copy.FileOffset,
                             (ULONG)Lfcb->LogPageSize,
                             TRUE,
                             FALSE,
                             TRUE,
                             &UsaError,
                             (PVOID *) &LogPageHeader,
                             &LogPageHeaderBcb );

            RtlCopyMemory( LogPageHeader,
                           TailPage,
                           (ULONG)Lfcb->LogPageSize );

            //
            //  Fill in last flushed lsn value flush the page.
            //

            LogPageHeader->Copy.LastLsn = TailPage->Header.Packed.LastEndLsn;

            LfsFlushLogPage( Lfcb,
                             LogPageHeader,
                             TailPage->Copy.FileOffset,
                             &LogPageHeaderBcb );
        }

        //
        //  We also want to write over any partial I/O so it doesn't cause
        //  us problems on a subsequent restart.  We have the starting offset
        //  and the number of blocks.  We will simply write a Baad signature into
        //  each of these pages.  Any subsequent reads will have a Usa error.
        //

        while (PartialIoCount--) {

            //
            //  Make sure the current page is unpinned.
            //
    
            if (LogPageHeaderBcb != NULL) {
    
                CcUnpinData( LogPageHeaderBcb );
                LogPageHeaderBcb = NULL;
            }
    
            if (NT_SUCCESS( LfsPinOrMapData( Lfcb,
                                             FirstPartialIo,
                                             (ULONG)Lfcb->LogPageSize,
                                             TRUE,
                                             TRUE,
                                             TRUE,
                                             &UsaError,
                                             (PVOID *) &LogPageHeader,
                                             &LogPageHeaderBcb ))) {

                //
                //  Just store a the usa array header in the multi-section
                //  header.
                //

                *((PULONG) &LogPageHeader->MultiSectorHeader.Signature) = LFS_SIGNATURE_BAD_USA_ULONG;

                LfsFlushLogPage( Lfcb,
                                 LogPageHeader,
                                 FirstPartialIo,
                                 &LogPageHeaderBcb );
            }

            LfsNextLogPageOffset( Lfcb,
                                  FirstPartialIo,
                                  &FirstPartialIo,
                                  &Wrapped );
        }

        //
        //  We used to invalidate any tail pages we reused, now we let them
        //  be recopied every restart even if we fail a little later
        //  
                       
#ifdef BENL_DBG
        
        if (FirstTailPageBcb != NULL) {
            KdPrint(( "LFS: not spitting BAAD to 1st page\n" ));
        }

        if (SecondTailPageBcb != NULL) {
             KdPrint(( "LFS: not spitting BAAD to 2nd page\n" ));
        }
#endif
        

    } finally {

        DebugUnwind( LfsFindLastLsn );

        //
        //  Unpin the tail pages is pinned.
        //

        if (SecondTailPageBcb != NULL) {

            CcUnpinData( SecondTailPageBcb );
        }

        if (FirstTailPageBcb != NULL) {

            CcUnpinData( FirstTailPageBcb );
        }

        //
        //  Unpin the log page header if neccessary.
        //

        if (LogPageHeaderBcb != NULL) {

            CcUnpinData( LogPageHeaderBcb );
        }

        if (TestPageHeaderBcb != NULL) {

            CcUnpinData( TestPageHeaderBcb );
        }

        DebugTrace( -1, Dbg, "LfsFindLastLsn:  Exit\n", 0 );
    }

    return;
}


//
//  Local support routine.
//

BOOLEAN
LfsCheckSubsequentLogPage (
    IN PLFCB Lfcb,
    IN PLFS_RECORD_PAGE_HEADER RecordPageHeader,
    IN LONGLONG LogFileOffset,
    IN LONGLONG SequenceNumber
    )

/*++

Routine Description:

    This routine is called to check that a particular log page could not
    have been written after a prior Io transfer.  What we are looking for
    is the start of a transfer which was written after an Io which we
    we cannot read from during restart.  The presence of an additional
    Io means that we cannot guarantee that we can recover all of the
    restart data for the disk.  This makes the disk unrecoverable.

    We are given the sequence number of the Lsn that would occur on this page
    (if it is not part of an Log record which spans the end of a file).
    If we haven't wrapped the file and find an Lsn whose
    sequence number matches this, then we have an error.  If we have
    wrapped the file, and the sequence number in the Lsn in the
    first log page is
    written subsequent to a previous failing Io.

Arguments:

    Lfcb - Log file control block for this log file.

    RecordPageHeader - This is the header of a log page to check.

    LogFileOffset - This is the offset in the log file of this page.

    SequenceNumber - This is the sequence number that this log page should
                     not have.  This will be the sequence number for
                     any log records which begin on this page if written
                     after the page that failed.

Return Value:

    BOOLEAN - TRUE if this log page was written after some previous page,
              FALSE otherwise.

--*/

{
    BOOLEAN IsSubsequent;

    LSN Lsn;
    LONGLONG LsnSeqNumber;
    LONGLONG SeqNumberMinus1;
    LONGLONG LogPageFileOffset;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsCheckSubsequentLogPage:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb                  -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "RecordPageHeader      -> %08lx\n", RecordPageHeader );
    DebugTrace(  0, Dbg, "LogFileOffset (Low)   -> %08lx\n", LogFileOffset.LowPart );
    DebugTrace(  0, Dbg, "LogFileOffset (High)  -> %08lx\n", LogFileOffset.HighPart );
    DebugTrace(  0, Dbg, "SequenceNumber (Low)  -> %08lx\n", SequenceNumber.LowPart );
    DebugTrace(  0, Dbg, "SequenceNumber (High) -> %08lx\n", SequenceNumber.HighPart );

    //
    //  If the page header is either 0 or -1 then we say this page was not written
    //  after some previous page.
    //

    if (*((PULONG) RecordPageHeader->MultiSectorHeader.Signature) == LFS_SIGNATURE_UNINITIALIZED_ULONG
        || *((PULONG) RecordPageHeader->MultiSectorHeader.Signature) == 0) {

        DebugTrace( -1, Dbg, "LfsCheckSubsequentLogPage:  Exit -> %08x\n", FALSE );
        return FALSE;
    }

    //
    //  If the last Lsn on the page occurs was
    //  written after the page that caused the original error.  Then we
    //  have a fatal error.
    //

    Lsn = RecordPageHeader->Copy.LastLsn;

    LfsTruncateLsnToLogPage( Lfcb, Lsn, &LogPageFileOffset );
    LsnSeqNumber = LfsLsnToSeqNumber( Lfcb, Lsn );

    SeqNumberMinus1 = SequenceNumber - 1;

    //
    //  If the sequence number for the Lsn in the page is equal or greater than
    //  Lsn we expect, then this is a subsequent write.
    //

    if ( LsnSeqNumber >= SequenceNumber ) {

        IsSubsequent = TRUE;

    //
    //  If this page is the start of the file and the sequence number is 1 less
    //  than we expect and the Lsn indicates that we wrapped the file, then it
    //  is also part of a subsequent io.
    //
    //  The following test checks
    //
    //      1 - The sequence number for the Lsn is from the previous pass
    //          through the file.
    //      2 - We are at the first page in the file.
    //      3 - The log record didn't begin on the current page.
    //

    } else if (( LsnSeqNumber == SeqNumberMinus1 )
               && ( Lfcb->FirstLogPage == LogFileOffset )
               && ( LogFileOffset != LogPageFileOffset )) {

        IsSubsequent = TRUE;

    } else {

        IsSubsequent = FALSE;
    }

    DebugTrace( -1, Dbg, "LfsCheckSubsequentLogPage:  Exit -> %08x\n", IsSubsequent );

    return IsSubsequent;
}


//
//  Local support routine
//

VOID
LfsFlushLogPage (
    IN PLFCB Lfcb,
    PVOID LogPage,
    IN LONGLONG FileOffset,
    OUT PBCB *Bcb
    )

/*++

Routine Description:

    This routine is called to write a single log page to the log file.  We will
    mark it dirty in the cache, unpin it and call our flush routine.

Arguments:

    Lfcb - Log file control block for this log file.

    LogPage - Pointer to the log page in the cache.

    FileOffset - Offset of the page in the stream.

    Bcb - Address of the Bcb pointer for the cache.

Return Value:

    None

--*/

{
    PAGED_CODE();

    //
    //  Set the page dirty and unpin it.
    //

    CcSetDirtyPinnedData( *Bcb, NULL );
    CcUnpinData( *Bcb );
    *Bcb = NULL;

    //
    //  Now flush the data.
    //

    Lfcb->UserWriteData->FileOffset = FileOffset;
    Lfcb->UserWriteData->Length = (ULONG) Lfcb->LogPageSize;

    CcFlushCache( Lfcb->FileObject->SectionObjectPointer,
                  (PLARGE_INTEGER) &FileOffset,
                  (ULONG) Lfcb->LogPageSize,
                  NULL );

    return;
}


//
//  Local support routine.
//

VOID
LfsRemoveClientFromList (
    PLFS_CLIENT_RECORD ClientArray,
    PLFS_CLIENT_RECORD ClientRecord,
    IN PUSHORT ListHead
    )

/*++

Routine Description:

    This routine is called to remove a client record from a client record
    list in an Lfs restart area.

Arguments:

    ClientArray - Base of client records in restart area.

    ClientRecord - A pointer to the record to add.

    ListHead - A pointer to the beginning of the list.  This points to a
               USHORT which is the value of the first element in the list.

Return Value:

    None.

--*/

{
    PLFS_CLIENT_RECORD TempClientRecord;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsRemoveClientFromList:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Client Array  -> %08lx\n", ClientArray );
    DebugTrace(  0, Dbg, "Client Record -> %08lx\n", ClientRecord );
    DebugTrace(  0, Dbg, "List Head     -> %08lx\n", ListHead );

    //
    //  If this is the first element in the list, then the head of the list
    //  points to the element after this record.
    //

    if (ClientRecord->PrevClient == LFS_NO_CLIENT) {

        DebugTrace( 0, Dbg, "Element is first element in the list\n", 0 );
        *ListHead = ClientRecord->NextClient;

    //
    //  Otherwise the previous element points to the next element.
    //

    } else {

        TempClientRecord = ClientArray + ClientRecord->PrevClient;
        TempClientRecord->NextClient = ClientRecord->NextClient;
    }

    //
    //  If this is not the last element in the list, the previous element
    //  becomes the last element.
    //

    if (ClientRecord->NextClient != LFS_NO_CLIENT) {

        TempClientRecord = ClientArray + ClientRecord->NextClient;
        TempClientRecord->PrevClient = ClientRecord->PrevClient;
    }

    DebugTrace( -1, Dbg, "LfsRemoveClientFromList:  Exit\n", 0 );

    return;
}


//
//  Local support routine.
//

VOID
LfsAddClientToList (
    IN PLFS_CLIENT_RECORD ClientArray,
    IN USHORT ClientIndex,
    IN PUSHORT ListHead
    )

/*++

Routine Description:

    This routine is called to add a client record to the start of a list.

Arguments:

    ClientArray - This is the base of the client record.

    ClientIndex - The index for the record to add.

    ListHead - A pointer to the beginning of the list.  This points to a
               USHORT which is the value of the first element in the list.

Return Value:

    None.

--*/

{
    PLFS_CLIENT_RECORD ClientRecord;
    PLFS_CLIENT_RECORD TempClientRecord;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsAddClientToList:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Client Array  -> %08lx\n", ClientArray );
    DebugTrace(  0, Dbg, "Client Index  -> %04x\n", ClientIndex );
    DebugTrace(  0, Dbg, "List Head     -> %08lx\n", ListHead );

    ClientRecord = ClientArray + ClientIndex;

    //
    //  This element will become the first element on the list.
    //

    ClientRecord->PrevClient = LFS_NO_CLIENT;

    //
    //  The next element for this record is the previous head of the list.
    //

    ClientRecord->NextClient = *ListHead;

    //
    //  If there is at least one element currently on the list, we point
    //  the first element to this new record.
    //

    if (*ListHead != LFS_NO_CLIENT) {

        TempClientRecord = ClientArray + *ListHead;
        TempClientRecord->PrevClient = ClientIndex;
    }

    //
    //  This index is now the head of the list.
    //

    *ListHead = ClientIndex;

    DebugTrace( -1, Dbg, "LfsAddClientToList:  Exit\n", 0 );

    return;
}

