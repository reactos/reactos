/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Init.h

Abstract:

    This module contains initialization code for WinSock.DLL.

Author:

    David Treadwell (davidtr)    20-Feb-1992

Revision History:

--*/

#include "winsockp.h"
#include <stdlib.h>

#define ENABLE_DEBUG_LOGGING 1
#include "logit.h"

#if defined(USE_TEB_FIELD)
#error "WinSock TEB field now used by MSAFD.DLL!"
#endif

#define h_addr_ptrsBigBuf ACCESS_THREAD_DATA( h_addr_ptrsBigBuf, GETHOST )
#define hostaddrBigBuf    ACCESS_THREAD_DATA( hostaddrBigBuf, GETHOST )

BOOL
SockInitialize (
    IN PVOID DllHandle,
    IN ULONG Reason,
    IN PVOID Context OPTIONAL
    )
{
    NTSTATUS status;
    SYSTEM_INFO systemInfo;

    //
    // On a thread detach, set up the context param so that all
    // necessary deallocations will occur.
    //

    if ( Reason == DLL_THREAD_DETACH ) {
        Context = NULL;
    }

    switch ( Reason ) {

    case DLL_PROCESS_ATTACH:

        WS2LOG_INIT();

        SockModuleHandle = (HMODULE)DllHandle;
        {
            //
            // These are defined in the respective static libraries
            // We need to set them so that code in the library knows
            // which module it is used in and thus can pick up message
            // strings from it
            //
            extern HMODULE SockUtilMsgModuleHandle;
            extern HMODULE LibuemulMsgModuleHandle;
            SockUtilMsgModuleHandle = (HMODULE)DllHandle;
            LibuemulMsgModuleHandle = (HMODULE)DllHandle;
        }

#if DBG
        //
        // If there is a file in the current directory called "wsdebug"
        // open it and read the first line to set the debugging flags.
        //

        {
            HANDLE handle;

            handle = CreateFile(
                         "WsDebug",
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

                WsDebug = WINSOCK_DEBUG_DEBUGGER;

            } else {

                CHAR buffer[11];
                DWORD bytesRead;

                RtlZeroMemory( buffer, sizeof(buffer) );

                if ( ReadFile( handle, buffer, 10, &bytesRead, NULL ) ) {

                    buffer[bytesRead] = '\0';

                    WsDebug = strtoul( buffer, NULL, 16 );

                } else {

                    WS_PRINT(( "read file failed: %ld\n", GetLastError( ) ));
                }

                CloseHandle( handle );
            }
        }
#endif

        IF_DEBUG(INIT) {
            WS_PRINT(( "SockInitialize: process attach, PEB = %lx\n",
                           NtCurrentPeb( ) ));
        }

        //
        // Initialize the lists of sockets and helper DLLs.
        //

        InitializeListHead( &SockHelperDllListHead );
        InitializeListHead( &SocketListHead );

        //
        // Initialize the global post routine pointer.  We have to do it
        // here rather than statically because it otherwise won't be
        // thunked correctly.
        //

        SockPostRoutine = PostMessage;

        //
        // *** lock acquisition order: it is legal to acquire SocketLock
        // while holding an individual socket lock, but not the other way
        // around!
        //

        _try
        {
            InitializeCriticalSection( &SocketLock );
            pSocketLock = &SocketLock;
        }
        _except( STATUS_NO_MEMORY == GetExceptionCode() )
        {
            pSocketLock = NULL;
            pcsRnRLock = NULL;
            return FALSE;
        }

        _try
        {
            InitializeCriticalSection( &csRnRLock);
            pcsRnRLock = &csRnRLock;
        }
        _except( STATUS_NO_MEMORY == GetExceptionCode() )
        {
            DeleteCriticalSection( &SocketLock );
            pSocketLock = NULL;
            pcsRnRLock = NULL;
            return FALSE;
        }

#if !defined(USE_TEB_FIELD)
        //
        // Allocate space in TLS so that we can convert global variables
        // to thread variables.
        //

        SockTlsSlot = TlsAlloc( );

        if ( SockTlsSlot == 0xFFFFFFFF ) {

            WS_PRINT(( "SockInitialize: TlsAlloc failed: %ld\n", GetLastError( ) ));
            //
            // Don't delete critical sections here, a call to SockInitialize
            // will be made where these are freed then.
            //

            return FALSE;
        }
#endif  // !USE_TEB_FIELD

        //
        // Create private WinSock heap on MP machines.  UP machines
        // just use the process heap.
        //

        GetSystemInfo( &systemInfo );

        if( systemInfo.dwNumberOfProcessors > 1 ) {

            SockPrivateHeap = RtlCreateHeap( HEAP_GROWABLE |    // Flags
                                             HEAP_CLASS_1,
                                             NULL,              // HeapBase
                                             0,                 // ReserveSize
                                             0,                 // CommitSize
                                             NULL,              // Lock
                                             NULL );            // Parameters

        } else {

            WS_ASSERT( SockPrivateHeap == NULL );

        }

        if ( SockPrivateHeap == NULL ) {

            //
            // This is either a UP box, or RtlCreateHeap() failed.  In
            // either case, just use the process heap.
            //

            SockPrivateHeap = RtlProcessHeap();

        }

        break;

    case DLL_PROCESS_DETACH:

        IF_DEBUG(INIT) {
            WS_PRINT(( "SockInitialize: process detach, PEB = %lx\n",
                           NtCurrentPeb( ) ));
        }

        //
        // Only clean up resources if we're being called because of a
        // FreeLibrary().  If this is because of process termination,
        // do not clean up, as the system will do it for us.  Also,
        // if we get called at process termination, it is likely that
        // a thread was terminated while it held a winsock lock, which
        // would cause a deadlock if we then tried to grab the lock.
        //

        if ( Context == NULL )
        {
            GetHostCleanup();

            if ( pSocketLock )
            {
                DeleteCriticalSection( &SocketLock );
                pSocketLock = NULL;
            }

            if ( pcsRnRLock )
            {
                DeleteCriticalSection( &csRnRLock );
                pcsRnRLock = NULL;
            }
        }

        SockProcessTerminating = TRUE;

        // *** lack of break is intentional!

    case DLL_THREAD_DETACH:

        IF_DEBUG(INIT) {
            WS_PRINT(( "SockInitialize: thread detach, TEB = %lx\n",
                           NtCurrentTeb( ) ));
        }

        //
        // If the TLS information for this thread has been initialized,
        // free the thread data buffer.
        //

        if ( Context == NULL &&
             GET_THREAD_DATA() != NULL )
        {
            if ( SockBigBufSize > 0 )
            {
                if ( h_addr_ptrsBigBuf )
                {
                    FREE_HEAP( hostaddrBigBuf );
                    hostaddrBigBuf = NULL;
                }

                if ( h_addr_ptrsBigBuf )
                {
                    FREE_HEAP( h_addr_ptrsBigBuf );
                    h_addr_ptrsBigBuf = NULL;
                }

                SockBigBufSize = 0;
            }
            
            FREE_HEAP( GET_THREAD_DATA() );
            SET_THREAD_DATA( NULL );
        }

        //
        // If this is a process detach, free the TLS slot we're using.
        //

        if ( Reason == DLL_PROCESS_DETACH && Context == NULL ) {
            //
            // Defined in msext.c
            //
            extern UINT_PTR    *SockBufferKeyTable;
            if (SockBufferKeyTable!=NULL) {
                GlobalFree (SockBufferKeyTable);
            }

#if !defined(USE_TEB_FIELD)
            if ( SockTlsSlot != 0xFFFFFFFF ) {

                BOOL ret;

                ret = TlsFree( SockTlsSlot );
                WS_ASSERT( ret );

                SockTlsSlot = 0xFFFFFFFF;
            }
#endif  // !USE_TEB_FIELD

            //
            //  Also destroy any private WinSock heap.
            //

            if ( SockPrivateHeap != RtlProcessHeap() ) {

                WS_ASSERT( SockPrivateHeap != NULL );
                RtlDestroyHeap( SockPrivateHeap );
                SockPrivateHeap = NULL;

            }
        }

        break;

    case DLL_THREAD_ATTACH:
        break;

    default:

        WS_ASSERT( FALSE );
        break;
    }

    return TRUE;

} // SockInitialize

VOID
WEP (
    VOID
    )

/*++

Routine Description:

Arguments:

Return Value:

--*/

{
    return;
} // WEP
