/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    util.c

Abstract:

    This module contains utility functions used by the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockEnterApi

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public functions.
//


INT
SockEnterApi(
    IN BOOL MustBeStarted,
    IN BOOL AllowReentrancy
    )

/*++

Routine Description:

    Checks the current state of the service provider to determine if an
    SPI is valid at this time.

Arguments:

    MustBeStarted - TRUE if WSPStartup() must have already been called,
        FALSE otherwise.

    AllowReentrancy - TRUE if reentrancy on this thread is allowed, FALSE
        if it isn't.

Return Value:

    INT - 0 if successful, WSAE* if not.

--*/

{

    PSOCK_TLS_DATA tlsData;

    //
    // Bail if we're already detached from the process.
    //

    if( SockProcessTerminating ) {

        IF_DEBUG(ENTER) {

            SOCK_PRINT((
                "SockEnterApi: process terminating\n"
                ));

        }

        return WSANOTINITIALISED;

    }

    //
    // Make sure that WSAStartup has been called, if necessary.
    //

    if( MustBeStarted &&
        ( SockWspStartupCount == 0 || SockTerminating ) ) {

        IF_DEBUG(ENTER) {

            SOCK_PRINT((
                "SockEnterApi: WSAStartup() not called!\n"
                ));

        }

        return WSANOTINITIALISED;

    }

    //
    // If this thread has not been initialized, do it now.
    //

    tlsData = SOCK_GET_THREAD_DATA();

    if( tlsData == NULL ) {

        tlsData = SockInitializeThread();

        if( tlsData == NULL ) {

            IF_DEBUG(ENTER) {

                SOCK_PRINT((
                    "SockEnterApi: SockInitializeThread failed.\n"
                    ));

            }

            return WSAENOBUFS;

        }

    }

    SOCK_ASSERT( tlsData != NULL );

    //
    // Bail if we're being reentered on the same thread. Note that
    // WSAEINPROGRESS is a distinguised error code for this very condition.
    //

    if( !AllowReentrancy && tlsData->ReentrancyFlag ) {

        IF_DEBUG(ENTER) {

            SOCK_PRINT((
                "SockEnterApi: Thread reentrancy\n"
                ));

        }

        return WSAEINPROGRESS;

    }

    //
    // Initialize the cancelled thread variable. We'll use this to
    // tell whether the operation has been cancelled.
    //

    tlsData->IoCancelled = FALSE;

    //
    // Everything's cool. Proceed.
    //

    return NO_ERROR;

}   // SockEnterApi

