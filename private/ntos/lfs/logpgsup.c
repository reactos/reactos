/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    LogPgSup.c

Abstract:

    This module implements support for manipulating log pages.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_LOG_PAGE_SUP)

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('PsfL')

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsAllocateSpanningBuffer)
#pragma alloc_text(PAGE, LfsFreeSpanningBuffer)
#pragma alloc_text(PAGE, LfsNextLogPageOffset)
#endif


VOID
LfsNextLogPageOffset (
    IN PLFCB Lfcb,
    IN LONGLONG CurrentLogPageOffset,
    OUT PLONGLONG NextLogPageOffset,
    OUT PBOOLEAN Wrapped
    )

/*++

Routine Description:

    This routine will compute the offset in the log file of the next log
    page.

Arguments:

    Lfcb - This is the file control block for the log file.

    CurrentLogPageOffset - This is the file offset of the current log page.

    NextLogPageOffset - Address to store the next log page to use.

    Wrapped - This is a pointer to a boolean variable that, if present,
              we use to indicate whether we wrapped in the log file.

Return Value:

    None.

--*/

{
    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsNextLogPageOffset:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb                          ->  %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "CurrentLogPageOffset (Low)    ->  %08lx\n", CurrentLogPageOffset.LowPart );
    DebugTrace(  0, Dbg, "CurrentLogPageOffset (High)   ->  %08lx\n", CurrentLogPageOffset.HighPart );
    DebugTrace(  0, Dbg, "Wrapped                       ->  %08lx\n", Wrapped );

    //
    //  We add the log page size to the current log offset.
    //

    LfsTruncateOffsetToLogPage( Lfcb, CurrentLogPageOffset, &CurrentLogPageOffset );
    *NextLogPageOffset = CurrentLogPageOffset + Lfcb->LogPageSize;                                                     //**** xxAdd( CurrentLogPageOffset, Lfcb->LogPageSize );

    //
    //  If the result is larger than the file, we use the first page offset
    //  in the file.
    //

    if ( *NextLogPageOffset >= Lfcb->FileSize ) {                                                                      //**** xxGeq( *NextLogPageOffset, Lfcb->FileSize )

        *NextLogPageOffset = Lfcb->FirstLogPage;

        *Wrapped = TRUE;

    } else {

        *Wrapped = FALSE;
    }

    DebugTrace(  0, Dbg, "NextLogPageOffset (Low)    ->  %08lx\n", NextLogPageOffset->LowPart );
    DebugTrace(  0, Dbg, "NextLogPageOffset (High)   ->  %08lx\n", NextLogPageOffset->HighPart );
    DebugTrace(  0, Dbg, "Wrapped                    ->  %08x\n", *Wrapped );
    DebugTrace( -1, Dbg, "LfsNextLogPageOffset:  Exit\n", 0 );

    return;
}


PVOID
LfsAllocateSpanningBuffer (
    IN PLFCB Lfcb,
    IN ULONG Length
    )

/*++

Routine Description:

    This routine is called to allocate a spare buffer to read a file record
    which spans a log page.  We will first try to allocate one.  If that
    fails we will use one of the existing spare buffers.  If that fails then
    we will raise.

Arguments:

    Lfcb - This is the file control block for the log file.

    Length - Length of the buffer required.

Return Value:

    PVOID - Pointer to the buffer to use for reading the log record.
        May be either from pool or from the auxilary buffer pool.

--*/

{
    PVOID NewBuffer = NULL;
    ERESOURCE_THREAD Thread;
    BOOLEAN Wait = FALSE;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsAllocateSpanningBuffer:  Entered\n", 0 );

    //
    //  Loop while we don't have a buffer.  First try to get our reserved buffer
    //  without waiting.  Then try to allocate a buffer.  Finally wait for the reserved
    //  buffer as the final alternative.
    //

    do {

        //
        //  Skip the reserved buffer if the request is larger than we can read into it.
        //

        if (Length <= LFS_BUFFER_SIZE) {

            //
            //  If this thread already owns one buffer it can get the second directly.
            //

            Thread = ExGetCurrentResourceThread();

            if (Thread == LfsData.BufferOwner) {

                if (!FlagOn( LfsData.BufferFlags, LFS_BUFFER1_OWNED )) {

                    SetFlag( LfsData.BufferFlags, LFS_BUFFER1_OWNED );
                    NewBuffer = LfsData.Buffer1;
                    break;

                } else if (!FlagOn( LfsData.BufferFlags, LFS_BUFFER2_OWNED )) {

                    SetFlag( LfsData.BufferFlags, LFS_BUFFER2_OWNED );
                    NewBuffer = LfsData.Buffer2;
                    break;

                } else if (Wait) {

                    //
                    //  This shouldn't happen but handle anyway.
                    //

                    DebugTrace( -1, Dbg, "LfsAllocateSpanningBuffer:  Exit\n", 0 );
                    ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
                }

            //
            //  Otherwise acquire the buffer lock and check the state of the buffers.
            //

            } else {

                BOOLEAN LfcbOwned = TRUE;

                while (TRUE) {

                    LfsAcquireBufferLock();

                    //
                    //  Check to see if the buffers are available.  No
                    //  need to drop the Lfcb in the typical case.
                    //

                    if (LfsData.BufferOwner == (ERESOURCE_THREAD) NULL) {

                        ASSERT( !FlagOn( LfsData.BufferFlags, LFS_BUFFER1_OWNED | LFS_BUFFER2_OWNED ));
                        NewBuffer = LfsData.Buffer1;
                        LfsData.BufferOwner = Thread;
                        SetFlag( LfsData.BufferFlags, LFS_BUFFER1_OWNED );
                        LfsBlockBufferWaiters();
                        
                        //
                        //  Reacquire the Lfcb if needed.
                        //

                        if (!LfcbOwned) { 

                            LfsAcquireLfcb( Lfcb );
                        }

                        //
                        //  Break out.
                        //

                        LfsReleaseBufferLock();
                        break;
                    }

                    //
                    //  Release the Lfcb and wait on the notification for the buffers.
                    //

                    if (Wait) {

                        if (LfcbOwned) { 
                            LfsReleaseLfcb( Lfcb );
                            LfcbOwned = FALSE;
                        }

                        LfsReleaseBufferLock();
                        LfsWaitForBufferNotification();

                    } else {

                        //
                        //  Go ahead and try to allocate a buffer from pool next.
                        //

                        LfsReleaseBufferLock();
                        break;
                    }
                }
            }

        //
        //  Raise if we already tried the allocate path.
        //

        } else if (Wait) {

            DebugTrace( -1, Dbg, "LfsAllocateSpanningBuffer:  Exit\n", 0 );
            ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
        }

        //
        //  Try pool if we didn't get a buffer above.
        //

        if (NewBuffer == NULL) {

            //
            //  Try pool next but don't let this fail on pool allocation.
            //

            NewBuffer = LfsAllocatePoolNoRaise( PagedPool, Length );
        }

        //
        //  Wait on the next pass through the loop.
        //

        Wait = TRUE;

    } while (NewBuffer == NULL);

    DebugTrace( -1, Dbg, "LfsAllocateSpanningBuffer:  Exit\n", 0 );
    return NewBuffer;
}

VOID
LfsFreeSpanningBuffer (
    IN PVOID Buffer
    )

/*++

Routine Description:

    This routine is called to free a buffer used to read a log record
    which spans pages.  We will check if it is one of our special buffers
    and deal with synchronization in that case.

Arguments:

    Buffer - Buffer to free.

Return Value:

    None.

--*/

{
    ERESOURCE_THREAD Thread;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFreeSpanningBuffer:  Entered\n", 0 );

    //
    //  Do an unsafe test of the buffer flags.  If we own a buffer then they must be non-zero.
    //  Otherwise do the correct check of the resource thread.
    //

    if (!FlagOn( LfsData.BufferFlags, LFS_BUFFER1_OWNED | LFS_BUFFER2_OWNED ) ||
        (LfsData.BufferOwner != ExGetCurrentResourceThread())) {

        LfsFreePool( Buffer );

    } else {

        //
        //  Acquire the lock for synchronization.
        //

        LfsAcquireBufferLock();

        //
        //  Check which buffer it is.
        //

        if (Buffer == LfsData.Buffer1) {

            ClearFlag( LfsData.BufferFlags, LFS_BUFFER1_OWNED );

        } else {

            //
            //  It better be buffer2
            //

            ASSERT( FlagOn( LfsData.BufferFlags, LFS_BUFFER2_OWNED ));
            ClearFlag( LfsData.BufferFlags, LFS_BUFFER2_OWNED );
        }

        //
        //  If no buffers owned then signal the waiters.
        //

        if (!FlagOn( LfsData.BufferFlags, LFS_BUFFER1_OWNED | LFS_BUFFER2_OWNED )) {

            LfsData.BufferOwner = (ERESOURCE_THREAD) NULL;
            LfsNotifyBufferWaiters();
        }

        LfsReleaseBufferLock();
    }

    DebugTrace( -1, Dbg, "LfsFreeSpanningBuffer:  Exit\n", 0 );

    return;
}

