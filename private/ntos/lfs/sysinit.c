/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    SysInit.c

Abstract:

    This module implements the Log File Service initialization.

Author:

    Brian Andrew    [BrianAn]   20-June-1991

Revision History:

--*/

#include "lfsprocs.h"

//
//  The debug trace level
//

#define Dbg                              (DEBUG_TRACE_INITIALIZATION)

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('IsfL')

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LfsInitializeLogFileService)
#endif

extern USHORT LfsUsaSeqNumber;


BOOLEAN
LfsInitializeLogFileService (
    )

/*++

Routine Description:

    This routine must be called during system initialization before the
    first call to logging service, to allow the Log File Service to initialize
    its global data structures.  This routine has no dependencies on other
    system components being initialized.

    This routine will initialize the global structures used by the logging
    service and start the Lfs worker thread.

Arguments:

    None

Return Value:

    TRUE if initialization was successful

--*/

{
    LARGE_INTEGER CurrentTime;

    PAGED_CODE();

    DebugTrace( +1, Dbg, "LfsInitializeLogFileService:  Enter\n", 0 );

    //
    //  If the structure has already been initialized then we can return
    //  immediately.
    //

    if (LfsData.NodeTypeCode == LFS_NTC_DATA
        && LfsData.NodeByteSize == sizeof( LFS_DATA )
        && FlagOn( LfsData.Flags, LFS_DATA_INITIALIZED )) {

        DebugTrace( -1, Dbg, "LfsInitializeLogFileService:  Exit  ->  %01x\n", TRUE );

        return TRUE;
    }

    //
    //  Zero out the structure initially.
    //

    RtlZeroMemory( &LfsData, sizeof( LFS_DATA ));

    //
    //  Assume the operation will fail.
    //

    LfsData.Flags = LFS_DATA_INIT_FAILED;

    //
    //  Initialize the global structure for Lfs.
    //

    LfsData.NodeTypeCode = LFS_NTC_DATA;
    LfsData.NodeByteSize = sizeof( LFS_DATA );

    InitializeListHead( &LfsData.LfcbLinks );

    //
    //  Initialize the synchronization objects.
    //

    ExInitializeFastMutex( &LfsData.LfsDataLock );

    //
    //  Initialize the buffer allocation.  System will be robust enough to tolerate
    //  allocation failures.
    //

    ExInitializeFastMutex( &LfsData.BufferLock );
    KeInitializeEvent( &LfsData.BufferNotification, NotificationEvent, TRUE );
    LfsData.Buffer1 = LfsAllocatePoolNoRaise( PagedPool, LFS_BUFFER_SIZE );

    if (LfsData.Buffer1 == NULL) {

        return FALSE;
    }

    LfsData.Buffer2 = LfsAllocatePoolNoRaise( PagedPool, LFS_BUFFER_SIZE );

    //
    //  Make sure we got both.
    //

    if (LfsData.Buffer2 == NULL) {

        LfsFreePool( LfsData.Buffer1 );
        LfsData.Buffer1 = NULL;
        return FALSE;
    }

    //
    //  Initialization has been successful.
    //

    ClearFlag( LfsData.Flags, LFS_DATA_INIT_FAILED );
    SetFlag( LfsData.Flags, LFS_DATA_INITIALIZED );

    //
    //  Get a random number as a seed for the Usa sequence numbers.  Use the lower
    //  bits of the current time.
    //

    KeQuerySystemTime( &CurrentTime );
    LfsUsaSeqNumber = (USHORT) CurrentTime.LowPart;

    DebugTrace( -1, Dbg, "LfsInitializeLogFileService:  Exit  ->  %01x\n", TRUE );

    return TRUE;
}
