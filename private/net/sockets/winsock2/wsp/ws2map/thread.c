/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    thread.c

Abstract:

    This module contains routines for managing worker threads and special
    "hooker threads" for the Winsock 2 to Winsock 1.1 Mapper Service
    Provider.

    The following routines are exported by this module:

        SockCreateWorkerThread()
        SockCreateHookerThread()

Author:

    Keith Moore (keithmo) 09-Jul-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private types.
//

typedef struct _SOCK_THREAD_STARTUP_CONTEXT {

    LPTHREAD_START_ROUTINE OriginalRoutine;
    LPVOID OriginalParameter;

} SOCK_THREAD_STARTUP_CONTEXT, *PSOCK_THREAD_STARTUP_CONTEXT;


//
// Private prototypes.
//

DWORD
WINAPI
SockWorkerThreadStarter(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
SockHookerThreadStarter(
    LPVOID lpThreadParameter
    );

BOOL
SockAddDllReference(
    VOID
    );

#define SockRemoveDllReference() FreeLibrary( SockModuleHandle )


//
// Public functions.
//


HANDLE
WINAPI
SockCreateWorkerThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )

/*++

Routine Description:

    Creates a worker thread.

Arguments:

    Identical to the CreateThread() Win32 API.

Return Value:

    Ditto.

--*/

{

    HANDLE threadHandle;
    PSOCK_THREAD_STARTUP_CONTEXT context = NULL;

    //
    // Add an artificial reference to our DLL. This will prevent the
    // DLL from getting detached from the process's address space
    // until this worker thread exits.
    //

    if( !SockAddDllReference() ) {

        return NULL;

    }

    //
    // Create the startup context.
    //

    context = SOCK_ALLOCATE_HEAP( sizeof(*context) );

    if( context == NULL ) {

        SockRemoveDllReference();
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;

    }

    context->OriginalRoutine = lpStartAddress;
    context->OriginalParameter = lpParameter;

    //
    // Create the thread.
    //

    threadHandle = CreateThread(
                       lpThreadAttributes,
                       dwStackSize,
                       &SockWorkerThreadStarter,
                       context,
                       dwCreationFlags,
                       lpThreadId
                       );

    if( threadHandle == NULL && context != NULL ) {

        DWORD err;

        //
        // CreateThread() failed. Destroy the context before returning,
        // but be careful to preserve LastError.
        //

        err = GetLastError();
        SOCK_FREE_HEAP( context );
        SockRemoveDllReference();
        SetLastError( err );

    }

    return threadHandle;

}   // SockCreateHookerThread



HANDLE
WINAPI
SockCreateHookerThread(
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    DWORD dwStackSize,
    LPTHREAD_START_ROUTINE lpStartAddress,
    LPVOID lpParameter,
    DWORD dwCreationFlags,
    LPDWORD lpThreadId
    )

/*++

Routine Description:

    Special wrapper around the standard CreateThread() Win32 API. This
    routine creates a thread that is tagged to "avoid" the 2:1 mapper
    on future socket calls. This is necessary for hooker DLLs that create
    worker threads that expect to make socket calls to the "original"
    (hooked) DLL.

Arguments:

    Identical to the CreateThread() Win32 API.

Return Value:

    Ditto.

--*/

{

    HANDLE threadHandle;
    PSOCK_THREAD_STARTUP_CONTEXT context = NULL;

    //
    // We only need to go through this pain if this DLL has been
    // properly initialized.
    //

    if( SockWspStartupCount > 0 ) {

        //
        // Create the startup context. If this fails, then we'll just
        // press on regardless and create the user's worker thread.
        //

        context = SOCK_ALLOCATE_HEAP( sizeof(*context) );

        if( context != NULL ) {

            context->OriginalRoutine = lpStartAddress;
            context->OriginalParameter = lpParameter;

            //
            // Forward off to our starter routine.
            //

            lpStartAddress = &SockHookerThreadStarter;
            lpParameter = context;

        }

    }

    //
    // Create the thread.
    //

    threadHandle = CreateThread(
                       lpThreadAttributes,
                       dwStackSize,
                       lpStartAddress,
                       lpParameter,
                       dwCreationFlags,
                       lpThreadId
                       );

    if( threadHandle == NULL && context != NULL ) {

        DWORD err;

        //
        // CreateThread() failed. Destroy the context before returning,
        // but be careful to preserve LastError.
        //

        err = GetLastError();
        SOCK_FREE_HEAP( context );
        SetLastError( err );

    }

    return threadHandle;

}   // SockCreateHookerThread


//
// Private functions.
//


DWORD
WINAPI
SockWorkerThreadStarter(
    LPVOID lpThreadParameter
    )

/*++

Routine Description:

    Helper routine for starting worker threads. This is the actual starting
    point for the thread. We unpack the necessary data from the passed-in
    context, call the original starting point, then exit via the special
    FreeLibraryAndExitThread() API.

Arguments:

    lpThreadParameter - Actually points to a SOCK_THREAD_STARTUP_CONTEXT
        structure that contains the original thread entrypoint and parameter.

Return Value:

    DWORD - The return value from the worker thread.

--*/

{

    PSOCK_THREAD_STARTUP_CONTEXT context;
    LPTHREAD_START_ROUTINE routine;
    LPVOID param;
    DWORD result;

    //
    // Capture the startup parameters, then free the context.
    //

    context = (PSOCK_THREAD_STARTUP_CONTEXT)lpThreadParameter;

    routine = context->OriginalRoutine;
    param = context->OriginalParameter;

    SOCK_FREE_HEAP( context );

    //
    // Let the original routine do its thang.
    //

    result = routine( param );

    //
    // Get outta here.
    //

    FreeLibraryAndExitThread(
        SockModuleHandle,
        result
        );

    //
    // We should never make it this far, but just in case...
    //

    return result;

}   // SockWorkerThreadStarter



DWORD
WINAPI
SockHookerThreadStarter(
    LPVOID lpThreadParameter
    )

/*++

Routine Description:

    Helper routine for starting special threads. This is the actual starting
    point for the thread. We unpack the necessary data from the passed-in
    context, mark the current thread (if it's a hooker thread), then call the
    original starting point.

Arguments:

    lpThreadParameter - Actually points to a SOCK_THREAD_STARTUP_CONTEXT
        structure that contains the original thread entrypoint and parameter.

Return Value:

    DWORD - The return value from the worker thread.

--*/

{

    PSOCK_THREAD_STARTUP_CONTEXT context;
    PSOCK_TLS_DATA tlsData;
    LPTHREAD_START_ROUTINE routine;
    LPVOID param;

    //
    // Initialize this thread.
    //

    tlsData = SockInitializeThread();

    if( tlsData != NULL ) {

        tlsData->ReentrancyFlag = TRUE;

    }

    //
    // Capture the startup parameters, then free the context.
    //

    context = (PSOCK_THREAD_STARTUP_CONTEXT)lpThreadParameter;

    routine = context->OriginalRoutine;
    param = context->OriginalParameter;

    SOCK_FREE_HEAP( context );

    //
    // Let the original routine do its thang.
    //

    return routine( param );

}   // SockHookerThreadStarter



BOOL
SockAddDllReference(
    VOID
    )

/*++

Routine Description:

    Adds an artificial reference to our DLL so that it won't get prematurely
    detached from the process's address space while we have worker threads
    active.

Arguments:

    None.

Return Value:

    BOOL - TRUE if successful, FALSE otherwise.

--*/

{

    HINSTANCE instance;
    DWORD result;
    CHAR modulePath[MAX_PATH];

    //
    // Find our fully qualified DLL path.
    //

    result = GetModuleFileName(
                 SockModuleHandle,
                 modulePath,
                 sizeof(modulePath) / sizeof(modulePath[0])
                 );

    if( result == 0 ) {

        return FALSE;

    }

    //
    // Try to load it.
    //

    instance = LoadLibrary( modulePath );

    if( instance == NULL ) {

        return FALSE;

    }

    //
    // Success!
    //

    SOCK_ASSERT( instance == SockModuleHandle );
    return TRUE;

}   // SockAddDllReference

