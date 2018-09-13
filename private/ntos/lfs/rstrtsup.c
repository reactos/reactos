/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    RstrtSup.c

Abstract:

    This module implements support for dealing with the Lfs restart area.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_RESTART_SUP)

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsFindOldestClientLsn)
#pragma alloc_text(PAGE, LfsWriteLfsRestart)
#endif


VOID
LfsWriteLfsRestart (
    IN PLFCB Lfcb,
    IN ULONG ThisRestartSize,
    IN BOOLEAN WaitForIo
    )

/*++

Routine Description:

    This routine puts the Lfs restart area on the queue of operations to
    write to the file.  We do this by allocating a second restart area
    and attaching it to the Lfcb.  We also allocate a buffer control
    block to use for this write.  We look at the WaitForIo boolean to
    determine whether this thread can perform the I/O.  This also indicates
    whether this thread gives up the Lfcb.

Arguments:

    Lfcb - A pointer to the log file control block for this operation.

    ThisRestartSize - This is the size to use for the restart area.

    WaitForIo - Indicates if this thread is to perform the work.

Return Value:

    None.

--*/

{
    PLBCB NewLbcb = NULL;
    PLFS_RESTART_AREA NewRestart = NULL;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsWriteLfsRestart:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "Lfcb          -> %08lx\n", Lfcb );
    DebugTrace(  0, Dbg, "Write Chkdsk  -> %04x\n", WriteChkdsk );
    DebugTrace(  0, Dbg, "Restart Size  -> %08lx\n", ThisRestartSize );
    DebugTrace(  0, Dbg, "WaitForIo     -> %08lx\n", WaitForIo );

    //
    //  Use a try-finally to facilitate cleanup.
    //

    try {

        PLBCB ActiveLbcb;

        //
        //  We allocate another restart area and
        //  copy the current area into it.  Attach the new area to the Lfcb.
        //

        LfsAllocateRestartArea( &NewRestart, ThisRestartSize );

        //
        //  We allocate a Lbcb structure and update the values to
        //  reflect this restart area.
        //

        LfsAllocateLbcb( Lfcb, &NewLbcb );
        SetFlag( NewLbcb->LbcbFlags, LBCB_RESTART_LBCB );

        //
        //  If this is the second page, then add a system page to the offset.
        //

        if (!Lfcb->InitialRestartArea) {

            NewLbcb->FileOffset = Lfcb->SystemPageSize + NewLbcb->FileOffset;
        }

        (ULONG)NewLbcb->Length = ThisRestartSize;

        NewLbcb->PageHeader = (PVOID) Lfcb->RestartArea;

        //
        //  Lets put the current lsn in the Lbcb.
        //

        NewLbcb->LastEndLsn = NewLbcb->LastLsn = Lfcb->NextRestartLsn;
        Lfcb->NextRestartLsn.QuadPart = 1 + Lfcb->NextRestartLsn.QuadPart;

        //
        //  Copy the existing restart area into the new area.
        //

        RtlCopyMemory( NewRestart, Lfcb->RestartArea, ThisRestartSize );
        Lfcb->RestartArea = NewRestart;

        Lfcb->ClientArray = Add2Ptr( NewRestart, Lfcb->ClientArrayOffset, PLFS_CLIENT_RECORD );

        NewRestart = NULL;

        //
        //  Update the Lfcb to indicate that the other restart area
        //  on the disk is to be used.
        //

        Lfcb->InitialRestartArea = !Lfcb->InitialRestartArea;

        //
        //  Add this Lbcb to the end of the workque and flush to that point.
        //

        InsertTailList( &Lfcb->LbcbWorkque, &NewLbcb->WorkqueLinks );

        //
        //  If we don't support a packed log file then we need to make
        //  sure that all file records written out ahead of this
        //  restart area make it out to disk and we don't add anything
        //  to this page.
        //

        if (!FlagOn( Lfcb->Flags, LFCB_PACK_LOG )
            && !IsListEmpty( &Lfcb->LbcbActive )) {

            ActiveLbcb = CONTAINING_RECORD( Lfcb->LbcbActive.Flink,
                                            LBCB,
                                            ActiveLinks );

            if (FlagOn( ActiveLbcb->LbcbFlags, LBCB_NOT_EMPTY )) {

                RemoveEntryList( &ActiveLbcb->ActiveLinks );
                ClearFlag( ActiveLbcb->LbcbFlags, LBCB_ON_ACTIVE_QUEUE );
            }
        }

        if (WaitForIo) {

            LfsFlushLbcb( Lfcb, NewLbcb );
        }

    } finally {

        DebugUnwind( LfsWriteLfsRestart );

        if (NewRestart != NULL) {

            ExFreePool( NewRestart );
        }

        DebugTrace( -1, Dbg, "LfsWriteLfsRestart:  Exit\n", 0 );
    }

    return;
}


VOID
LfsFindOldestClientLsn (
    IN PLFS_RESTART_AREA RestartArea,
    IN PLFS_CLIENT_RECORD ClientArray,
    OUT PLSN OldestLsn
    )

/*++

Routine Description:

    This routine walks through the active clients to determine the oldest
    Lsn the system must maintain.

Arguments:

    RestartArea - This is the Restart Area to examine.

    ClientArray - This is the start of the client data array.

    OldestLsn - We store the oldest Lsn we find in this value.  It is
        initialized with a starting value, we won't return a more recent
        Lsn.

Return Value:

    None.

--*/

{
    USHORT NextClient;

    PLFS_CLIENT_RECORD ClientBlock;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsFindOldestClientLsn:  Entered\n", 0 );
    DebugTrace(  0, Dbg, "RestartArea       -> %08lx\n", RestartArea );
    DebugTrace(  0, Dbg, "Base Lsn (Low)    -> %08lx\n", BaseLsn.LowPart );
    DebugTrace(  0, Dbg, "Base Lsn (High)   -> %08lx\n", BaseLsn.HighPart );

    //
    //  Take the first client off the in use list.
    //

    NextClient = RestartArea->ClientInUseList;

    //
    //  While there are more clients, compare their oldest Lsn with the
    //  current oldest.
    //

    while (NextClient != LFS_NO_CLIENT) {

        ClientBlock = ClientArray + NextClient;

        //
        //  We ignore this block if it's oldest Lsn is 0.
        //

        if (( ClientBlock->OldestLsn.QuadPart != 0 )
            && ( ClientBlock->OldestLsn.QuadPart < OldestLsn->QuadPart )) {

            *OldestLsn = ClientBlock->OldestLsn;
        }

        //
        //  Try the next client block.
        //

        NextClient = ClientBlock->NextClient;
    }

    DebugTrace(  0, Dbg, "OldestLsn (Low)   -> %08lx\n", BaseLsn.LowPart );
    DebugTrace(  0, Dbg, "OldestLsn (High)  -> %08lx\n", BaseLsn.HighPart );
    DebugTrace( -1, Dbg, "LfsFindOldestClientLsn:  Exit\n", 0 );

    return;
}

