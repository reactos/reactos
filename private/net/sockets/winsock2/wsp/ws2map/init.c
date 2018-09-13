/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains DLL initialization code for the Winsock 2
    to Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockInitializeDll()
        SockInitializeThread()

Author:

    Keith Moore (keithmo) 29-May-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Public functions.
//


BOOL
SockInitializeDll(
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )

/*++

Routine Description:

    DLL initialization routine. This routine is invoked to notify the DLL
    of specific events occuring in the process.

Arguments:

    DllHandle - The handle to the current DLL.

    Reason - The current event. This will be one of the following values:

        DLL_PROCESS_ATTACH - The DLL is getting attached to a process's
            address space.

        DLL_PROCESS_DETACH - The DLL is getting detached to a process's
            address space.

        DLL_THREAD_ATTACH - A new thread has been created.

        DLL_THREAD_DETACH - A thread is going away.

    Context - Reserved.

Return Value:

    BOOL - TRUE if initialization is successful, FALSE otherwise. Only
        really used during DLL_PROCESS_ATTACH.

--*/

{

    INT err;

    //
    // On a thread detach, set up the context param so that all
    // necessary deallocations will occur.
    //

    if( Reason == DLL_THREAD_DETACH ) {

        Context = NULL;

    }

    switch( Reason ) {

    case DLL_PROCESS_ATTACH:

        //
        // Remember our module handle.
        //

        SockModuleHandle = (HMODULE)DllHandle;

#if DBG
        SockInitializeDebugData();

        //
        // If there is a file in the current directory called "ws2map.dbg"
        // open it and read the first line to set the debugging flags.
        //

        {
            HANDLE handle;

            handle = CreateFile(
                         "ws2map.dbg",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         OPEN_EXISTING,
                         0,
                         NULL
                         );

            if( handle == INVALID_HANDLE_VALUE ) {

                //
                // Set default value.
                //

                SockDebugFlags = SOCK_DEBUG_DEBUGGER;

            } else {

                CHAR buffer[11];
                DWORD bytesRead;

                ZeroMemory( buffer, sizeof(buffer) );

                if( ReadFile( handle, buffer, 10, &bytesRead, NULL ) ) {

                    buffer[bytesRead] = '\0';

                    SockDebugFlags = strtoul( buffer, NULL, 16 );

                } else {

                    SOCK_PRINT((
                        "read file failed: %ld\n",
                        GetLastError()
                        ));

                }

                CloseHandle( handle );
            }
        }

#endif

        IF_DEBUG(INIT) {

            SOCK_PRINT((
                "SockInitializeDll: process attach, PID = %lx\n",
                GetCurrentProcessId()
                ));

        }

        //
        // Initialize the global lists.
        //

        InitializeListHead( &SockGlobalSocketListHead );
        InitializeListHead( &SockHookerListHead );

        //
        // *** lock acquisition order: it is legal to acquire SockGlobalLock
        // while holding an individual socket lock, but not the other way
        // around!
        //

        try {

            InitializeCriticalSection( &SockGlobalLock );
            err = NO_ERROR;

        } except( SOCK_EXCEPTION_FILTER() ) {

            err = GetExceptionCode();

        }

        if( err != NO_ERROR ) {

            return FALSE;

        }

        //
        // Allocate space in TLS.
        //

        SockTlsSlot = TlsAlloc( );

        if( SockTlsSlot == TLS_OUT_OF_INDEXES ) {

            SOCK_PRINT((
                "SockInitializeDll: TlsAlloc failed: %ld\n",
                GetLastError()
                ));

            DeleteCriticalSection( &SockGlobalLock );
            return FALSE;

        }

        break;

    case DLL_PROCESS_DETACH:

        IF_DEBUG(INIT) {

            SOCK_PRINT((
                "SockInitializeDll: process detach, PID = %lx\n",
                GetCurrentProcessId()
                ));

        }

        //
        // Only clean up resources if we're being called because of a
        // FreeLibrary().  If this is because of process termination,
        // do not clean up, as the system will do it for us.  Also,
        // if we get called at process termination, it is likely that
        // a thread was terminated while it held a winsock lock, which
        // would cause a deadlock if we then tried to grab the lock.
        //

        if( Context == NULL ) {

            INT errTmp;

            WSPCleanup( &errTmp );
            DeleteCriticalSection( &SockGlobalLock );

        }

        SockProcessTerminating = TRUE;

        //
        // Fall through...
        //

    case DLL_THREAD_DETACH:

        IF_DEBUG(INIT) {

            SOCK_PRINT((
                "SockInitializeDll: thread detach, TID = %lx\n",
                GetCurrentThreadId()
                ));

        }

        //
        // If the TLS information for this thread has been initialized,
        // free the thread data buffer.
        //

        if( Context == NULL ) {

            PSOCK_TLS_DATA tlsData;

            tlsData = SOCK_GET_THREAD_DATA();

            if( tlsData != NULL ) {

                SOCK_FREE_HEAP( tlsData );
                SOCK_SET_THREAD_DATA( NULL );

            }

        }

        //
        // If this is a process detach, free the TLS slot we're using.
        //

        if( Reason == DLL_PROCESS_DETACH && Context == NULL ) {

            if ( SockTlsSlot != TLS_OUT_OF_INDEXES ) {

                SOCK_REQUIRE( TlsFree( SockTlsSlot ) );
                SockTlsSlot = TLS_OUT_OF_INDEXES;

            }

            //
            // Also kill the async helper thread.
            //

            SockDestroyAsyncThread();

#if DBG
            //
            // Clean up the debug heap lock
            //

            {
                extern BOOLEAN SockHeapDebugInitialized;
                extern CRITICAL_SECTION SocketHeapLock;

                if ( SockHeapDebugInitialized )
                {
                    DeleteCriticalSection( &SocketHeapLock );
                }
            }
#endif
        }

        break;

    case DLL_THREAD_ATTACH:
        break;

    default:
        SOCK_ASSERT( FALSE );
        break;
    }

    return TRUE;

}   // SockInitializeDll



PSOCK_TLS_DATA
SockInitializeThread(
    VOID
    )

/*++

Routine Description:

    Creates and initializes the per-thread data for the current thread.

Arguments:

    None.

Return Value:

    PSOCK_TLS_DATA - Pointer to the newly created per-thread data if
        successful, FALSE otherwise.

--*/

{

    PSOCK_TLS_DATA tlsData;

    IF_DEBUG(INIT) {

        SOCK_PRINT((
            "SockInitializeThread: TID = %lx\n",
            GetCurrentThreadId()
            ));

    }

    //
    // Allocate space for per-thread data the DLL will have.
    //

    tlsData = SOCK_ALLOCATE_HEAP( sizeof(*tlsData) );

    if( tlsData == NULL ) {

        SOCK_PRINT((
            "SockInitializeThread: unable to allocate thread data.\n"
            ));

        return NULL;

    }

    //
    // Initialize the thread data.
    //

    ZeroMemory(
        tlsData,
        sizeof(*tlsData)
        );

    //
    // Store a pointer to this data area in TLS.
    //

    if( !SOCK_SET_THREAD_DATA(tlsData) ) {

        SOCK_PRINT((
            "SockInitializeThread: TlsSetValue() failed: %d\n",
            GetLastError()
            ));

        SOCK_FREE_HEAP( tlsData );
        return NULL;

    }

    return tlsData;

}   // SockInitializeThread
