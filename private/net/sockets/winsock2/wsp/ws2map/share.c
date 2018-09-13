/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    share.c

Abstract:

    This module contains shared memory management routines for the
    Winsock 2 to Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockInitializeSharedData()
        SockTerminateSharedData()
        SockAcquireSharedDataLock()
        SockReleaseSharedDataLock()

Author:

    Keith Moore (keithmo) 09-Jul-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public functions.
//


BOOL
SockInitializeSharedData(
    VOID
    )

/*++

Routine Description:

    Initializes the global (cross-process) shared data area and the
    protecting mutex.

Arguments:

    None.

Return Value:

    BOOL - TRUE if successful, FALSE if not.

--*/

{

    BOOL result;

    //
    // Initialize everything to a known state.
    //

    SockSharedData = NULL;
    SockSharedDataMapping = NULL;
    SockSharedDataMutex = NULL;
    result = FALSE;

    //
    // Create the protective mutex.
    //

    SockSharedDataMutex = CreateMutex(
                              NULL,
                              FALSE,
                              SOCK_SHARED_DATA_MUTEX_NAME
                              );

    if( SockSharedDataMutex == NULL ) {

        goto complete;

    }

    //
    // Create the file mapping that will contain the shared data.
    //

    SockSharedDataMapping = CreateFileMapping(
                                INVALID_HANDLE_VALUE,
                                NULL,
                                PAGE_READWRITE,
                                0,
                                sizeof(*SockSharedData),
                                SOCK_SHARED_DATA_MAPPING_NAME
                                );

    if( SockSharedDataMapping == NULL ) {

        goto complete;

    }

    //
    // Map the data into our process.
    //

    SockSharedData = MapViewOfFile(
                         SockSharedDataMapping,
                         FILE_MAP_WRITE,
                         0,
                         0,
                         0
                         );

    if( SockSharedData == NULL ) {

        goto complete;

    }

    //
    // Success!
    //

    result = TRUE;

    IF_DEBUG(SHARED_DATA) {

        SOCK_PRINT((
            "SockInitializeSharedData: shared data @ %08lx\n",
            SockSharedData
            ));

    }

complete:

    if( !result ) {

        SockTerminateSharedData();

    }

    return result;

}   // SockInitializeSharedData



VOID
SockTerminateSharedData(
    VOID
    )

/*++

Routine Description:

    Terminates the global (cross-process) shared data area. Basically
    undoes anything done by SockInitializeSharedData().

Arguments:

    None.

Return Value:

    None.

--*/

{

    if( SockSharedData != NULL ) {

        UnmapViewOfFile( SockSharedData );
        SockSharedData = NULL;

    }

    if( SockSharedDataMapping != NULL ) {

        CloseHandle( SockSharedDataMapping );
        SockSharedDataMapping = NULL;

    }

    if( SockSharedDataMutex != NULL ) {

        CloseHandle( SockSharedDataMutex );
        SockSharedDataMutex = NULL;

    }

}   // SockTerminateSharedData



VOID
SockAcquireSharedDataLock(
    VOID
    )

/*++

Routine Description:

    Acquires the lock protecting the shared data area.

Arguments:

    None.

Return Value:

    None.

--*/

{

    DWORD result;

    //
    // Wait for ownership of the mutex.
    //

    result = WaitForSingleObject(
                 SockSharedDataMutex,
                 INFINITE
                 );

    if( result == WAIT_FAILED ) {

        SOCK_PRINT((
            "SockAcquireSharedDataLock: WaitForSingleObject failed, error %d\n",
            GetLastError()
            ));

    }

}   // SockAcquireSharedDataLock



VOID
SockReleaseSharedDataLock(
    VOID
    )

/*++

Routine Description:

    Releases the lock protecting the shared data area.

Arguments:

    None.

Return Value:

    None.

--*/

{

    BOOL result;

    //
    // Relinquish ownership of the mutex.
    //

    result = ReleaseMutex( SockSharedDataMutex );

    if( !result ) {

        SOCK_PRINT((
            "SockReleaseSharedDataLock: ReleaseMutex failed, error %d\n",
            GetLastError()
            ));

    }

}   // SockReleaseSharedDataLock

