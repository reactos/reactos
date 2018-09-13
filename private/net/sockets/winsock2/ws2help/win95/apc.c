/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    apc.c

Abstract:

    This module implements the APC helper functions for the WinSock 2.0
    helper library.

Author:

    Keith Moore (keithmo)       20-Jun-1995

Revision History:

--*/


#include "precomp.h"


//
//  Private constants.
//

#define FAKE_HELPER_HANDLE  ((HANDLE)'MKC ')


//
//  Public functions.
//


DWORD
WINAPI
WahOpenApcHelper(
    OUT LPHANDLE HelperHandle
    )

/*++

Routine Description:

    This routine opens the WinSock 2.0 APC helper device.

Arguments:

    HelperHandle - Points to a HANDLE that will receive an open handle
        to the APC helper device.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    //
    //  Validate parameters.
    //

    if( HelperHandle == NULL ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Just return a fake handle.
    //

    *HelperHandle = FAKE_HELPER_HANDLE;

    return NO_ERROR;

}   // WahOpenApcHelper


DWORD
WINAPI
WahCloseApcHelper(
    IN HANDLE HelperHandle
    )

/*++

Routine Description:

    This function closes the WinSock 2.0 APC helper device.

Arguments:

    HelperHandle - The handle to close.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    //
    //  Validate parameters.
    //

    if( HelperHandle != FAKE_HELPER_HANDLE ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Nothing to do.
    //

    return NO_ERROR;

}   // WahCloseApcHelper


DWORD
WINAPI
WahOpenCurrentThread(
    IN  HANDLE HelperHandle,
    OUT LPWSATHREADID ThreadId
    )

/*++

Routine Description:

    This function opens a handle to the current thread.

Arguments:

    HelperHandle - An open handle to the APC helper device.

    ThreadId - Points to a WSATHREADID structure that will receive
        an open handle to the current thread and an (optional) OS-
        dependent thread identifier.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    HANDLE currentProcess;
    HANDLE currentThread;

    //
    //  Validate parameters.
    //

    if( ( HelperHandle != FAKE_HELPER_HANDLE ) ||
        ( ThreadId == NULL ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Grab the current process & thread handles.
    //

    currentProcess = GetCurrentProcess();
    currentThread = GetCurrentThread();

    //
    //  Duplicate the current thread pseudo handle.
    //

    if( DuplicateHandle(
            currentProcess,                         // hSourceProcessHandle
            currentThread,                          // hSourceHandle
            currentProcess,                         // hTargetProcessHandle
            &ThreadId->ThreadHandle,                // lpTargetHandle
            0,                                      // dwDesiredAttributes
            FALSE,                                  // bInheritHandle
            DUPLICATE_SAME_ACCESS                   // dwOptions
            ) ) {

        //
        //  The NT implementation of the APC helper does not really
        //  need the OS-dependent thread identifier, but we'll store
        //  the current thread ID in the structure just for completeness.
        //

        ThreadId->Reserved = (DWORD)currentThread;

        return NO_ERROR;

    }

    return GetLastError();

}   // WahOpenCurrentThread


DWORD
WINAPI
WahCloseThread(
    IN HANDLE HelperHandle,
    IN LPWSATHREADID ThreadId
    )

/*++

Routine Description:

    This routine closes an open thread handle.

Arguments:

    HelperHandle - An open handle to the APC helper device.

    ThreadId - Points to a WSATHREADID structure initialized by a
        previous call to WahOpenCurrentThread().

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    //
    //  Validate parameters.
    //

    if( ( HelperHandle != FAKE_HELPER_HANDLE ) ||
        ( ThreadId == NULL ) ||
        ( ThreadId->ThreadHandle == NULL ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Close the handle.
    //

    if( CloseHandle( ThreadId->ThreadHandle ) ) {

        //
        //  Clear the fields in case the client tries something stupid.
        //

        ThreadId->ThreadHandle = NULL;
        ThreadId->Reserved = 0;

        return NO_ERROR;

    }

    return GetLastError();

}   // WahCloseThread


DWORD
WINAPI
WahQueueUserApc(
    IN HANDLE HelperHandle,
    IN LPWSATHREADID ThreadId,
    IN LPWSAUSERAPC ApcRoutine,
    IN DWORD ApcContext OPTIONAL
    )

/*++

Routine Description:

    This routine queues a user-mode APC for the specified thread.

Arguments:

    HelperHandle - An open handle to the APC helper device.

    ThreadId - Points to a WSATHREADID structure initialized by a
        previous call to WahOpenCurrentThread().

    ApcRoutine - Points to the APC code to execute when the specified
        thread enters an alertable wait.

    ApcContext - An uninterpreted context value to pass to the APC routine.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    //
    //  Validate parameters.
    //

    if( ( HelperHandle != FAKE_HELPER_HANDLE ) ||
        ( ThreadId == NULL ) ||
        ( ThreadId->ThreadHandle == NULL ) ||
        ( ApcRoutine == NULL ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Queue the APC.
    //

    if( QueueUserAPC(
            (PAPCFUNC)ApcRoutine,                   // pfnAPC
            ThreadId->ThreadHandle,                 // hThread
            ApcContext                              // dwData
            ) ) {

        //
        //  Success.
        //

        return NO_ERROR;

    }

    return GetLastError();

}   // WahQueueUserApc

