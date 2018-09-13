/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dprovide.cpp

Abstract:

    This module defines the WinSock2 class dprovder along with its methods.

Author:

    Mark Hamilton (mark_hamilton@jf2.intel.com) 7-July-1995

Revision History:

    21-Aug-1995 dirk@mink.intel.com
        Clean up from code review. Moved single line functions to inlines in
        header file. Added debug traceing code. Rewrote destructor. Removed
        ProviderID from the class.

    7-July-1995 mark_hamilton
      Genesis
--*/

#include "precomp.h"


//
// BUGBUG: Add these prototypes to WS2SPI.H.
//

int
WSPAPI
WPUOpenCurrentThread(
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    );

int
WSPAPI
WPUCloseThread(
    LPWSATHREADID lpThreadId,
    LPINT lpErrno
    );



DPROVIDER::DPROVIDER()
/*++
Routine Description:

    Creates any internal state.

Arguments:

    None

Return Value:

    None

--*/

{
    m_library_handle = NULL;
    m_reference_count = 1;
#ifdef DEBUG_TRACING
    m_lib_name = NULL;
#endif
}


inline INT
DPROVIDER::WSPCleanup(
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Terminate use of the WinSock service provider.

Arguments:

    lpErrno - A pointer to the error code.

Return Value:

    The  return  value  is  0 if the operation has been successfully initiated.
    Otherwise  the  value SOCKET_ERROR is returned,
and a specific error number
    is available in lpErrno.

--*/
{
    INT ReturnValue = NO_ERROR;

    if (m_library_handle) {
        LPWSPCLEANUP    lpWSPCleanup;
        
        lpWSPCleanup = 
            (LPWSPCLEANUP)InterlockedExchangePointer (
                            (PVOID *)&m_proctable.lpWSPCleanup,
                            NULL
                            );
        if (lpWSPCleanup!=NULL) {

            DEBUGF( DBG_TRACE,
                    ("Calling WSPCleanup for provider %X\n", this));

            if (!PREAPINOTIFY(( DTCODE_WSPCleanup,
                               &ReturnValue,
                               m_lib_name,
                               &lpErrno)) ) {

                ReturnValue = lpWSPCleanup(lpErrno);

                POSTAPINOTIFY(( DTCODE_WSPCleanup,
                                &ReturnValue,
                                m_lib_name,
                                &lpErrno));
            }
        }
    }
    return ReturnValue;
}


DPROVIDER::~DPROVIDER()
/*++
Routine Description:

    destroys any internal state.

Arguments:

    None

Return Value:

    None

--*/
{

    if (m_library_handle)
    {
        INT ErrorCode;
#ifdef DEBUG_TRACING
        assert (m_lib_name);
#endif

        // Cleanup service provider DLL if not already done so
        WSPCleanup (&ErrorCode);
        // Free the service provider DLL
        FreeLibrary(m_library_handle);

#ifdef DEBUG_TRACING
        delete m_lib_name;
#endif
    } //if
    DEBUGF( DBG_TRACE,
            ("Destroying provider %X\n", this));
}



INT
DPROVIDER::Initialize(
    IN LPSTR lpszLibFile,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo
    )
/*++
Routine Description:

    Initializes the DPROVIDER object.

Arguments:

    lpszLibFile - A  Null  terminating  string  that  points to the .DLL of the
                  service to load.

    lpProtocolInfo - A pointer to a WSAPROTOCOL_INFOW struct to hand to the
                     provider startup routine.

Return Value:

    If no error occurs, Initialize() returns ERROR_SUCEES.  Otherwise the value
    SOCKET_ERROR  is  returned,  and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    LPWSPSTARTUP        WSPStartupFunc          = NULL;
    WORD                wVersionRequested       = WINSOCK_HIGH_SPI_VERSION;
    INT                 error_code              = 0;
    WSPDATA             WSPData;
    WSPUPCALLTABLE      UpCallTable;
    char                LibraryPath[MAX_PATH];

    DEBUGF( DBG_TRACE,
            ("Initializing provider %X\n", this));

    TRY_START(guard_memalloc) {
        // Zero out contents of m_proctable
        ZeroMemory(
            (PVOID) &m_proctable,     // Destination
            sizeof(m_proctable));   // Length

        //
        // Fill  the  upcall  table  we  will hand to the service provider with the
        // address  of  our  upcall entry points.  We will also pre-fill this thing
        // with  zeros,  mainly  to  help  with  consistency  checks  if we add new
        // functions to the upcall table in the future.
        //
        ZeroMemory(
            (PVOID) & UpCallTable,  // Destination
            sizeof(UpCallTable));   // Length

    #if !defined(DEBUG_TRACING)
        UpCallTable.lpWPUCloseEvent = WPUCloseEvent;
        UpCallTable.lpWPUCloseSocketHandle = WPUCloseSocketHandle;
        UpCallTable.lpWPUCreateEvent = WPUCreateEvent;
        UpCallTable.lpWPUCreateSocketHandle = WPUCreateSocketHandle;
        UpCallTable.lpWPUModifyIFSHandle = WPUModifyIFSHandle;
        UpCallTable.lpWPUQueryBlockingCallback = WPUQueryBlockingCallback;
        UpCallTable.lpWPUQuerySocketHandleContext = WPUQuerySocketHandleContext;
        UpCallTable.lpWPUQueueApc = WPUQueueApc;
        UpCallTable.lpWPUResetEvent = WPUResetEvent;
        UpCallTable.lpWPUSetEvent = WPUSetEvent;
        UpCallTable.lpWPUPostMessage = WPUPostMessage;
        UpCallTable.lpWPUGetProviderPath = WPUGetProviderPath;
        UpCallTable.lpWPUFDIsSet = WPUFDIsSet;
    #else
        UpCallTable.lpWPUCloseEvent = DTHOOK_WPUCloseEvent;
        UpCallTable.lpWPUCloseSocketHandle = DTHOOK_WPUCloseSocketHandle;
        UpCallTable.lpWPUCreateEvent = DTHOOK_WPUCreateEvent;
        UpCallTable.lpWPUCreateSocketHandle = DTHOOK_WPUCreateSocketHandle;
        UpCallTable.lpWPUModifyIFSHandle = DTHOOK_WPUModifyIFSHandle;
        UpCallTable.lpWPUQueryBlockingCallback = DTHOOK_WPUQueryBlockingCallback;
        UpCallTable.lpWPUQuerySocketHandleContext = DTHOOK_WPUQuerySocketHandleContext;
        UpCallTable.lpWPUQueueApc = DTHOOK_WPUQueueApc;
        UpCallTable.lpWPUResetEvent = DTHOOK_WPUResetEvent;
        UpCallTable.lpWPUSetEvent = DTHOOK_WPUSetEvent;
        UpCallTable.lpWPUPostMessage = DTHOOK_WPUPostMessage;
        UpCallTable.lpWPUGetProviderPath = DTHOOK_WPUGetProviderPath;
        UpCallTable.lpWPUFDIsSet = DTHOOK_WPUFDIsSet;
    #endif  // !defined(DEBUG_TRACING)

        //
        // BUGBUG: Make these hookable someday.
        //

        UpCallTable.lpWPUOpenCurrentThread = WPUOpenCurrentThread;
        UpCallTable.lpWPUCloseThread = WPUCloseThread;

        //
        // Expand the library name to pickup environment/registry variables
        //
        if (!( ExpandEnvironmentStrings(lpszLibFile,
                                        LibraryPath,
                                        MAX_PATH))){
            DEBUGF(
                DBG_ERR,
                ("Expansion of environment variables failed\n"));
            error_code = WSASYSCALLFAILURE;
            TRY_THROW(guard_memalloc);
        } //if

#ifdef DEBUG_TRACING
        m_lib_name = (LPSTR) new char[lstrlen(LibraryPath) + 1];
        if (m_lib_name == NULL) {
            DEBUGF(
                DBG_ERR,
                ("Allocation of m_lib_name failed\n"));
            error_code = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(guard_memalloc);
        }
        lstrcpy(m_lib_name, LibraryPath);
#endif DEBUG_TRACING
        //
        // First load the DLL for the service provider. Then get two functions that
        // init the service provider structures.
        //
        m_library_handle = LoadLibrary(LibraryPath);
        if(!m_library_handle){
            DEBUGF(
                DBG_ERR,
                ("Failed to load DLL %s\n",LibraryPath));
            error_code = WSAEPROVIDERFAILEDINIT;
            TRY_THROW(guard_memalloc);
        }

        WSPStartupFunc = (LPWSPSTARTUP)GetProcAddress(
            m_library_handle,
            "WSPStartup"
            );

        if(!(WSPStartupFunc)){

            DEBUGF( DBG_ERR,("Could get startup entry point for %s\n",
                             lpszLibFile));
            error_code = WSAEPROVIDERFAILEDINIT;
            TRY_THROW(guard_memalloc);
        }

        //
        // Set exception handler around this call since we
        // hold critical section (catalog lock).
        //
        __try {
#if !defined(DEBUG_TRACING)
            error_code = (*WSPStartupFunc)(
                wVersionRequested,
                & WSPData,
                lpProtocolInfo,
                UpCallTable,
                &m_proctable);
#else
            { // declaration block
                LPWSPDATA  pWSPData = & WSPData;
                BOOL       bypassing_call;

                bypassing_call = PREAPINOTIFY((
                    DTCODE_WSPStartup,
                    & error_code,
                    LibraryPath,
                    & wVersionRequested,
                    & pWSPData,
                    & lpProtocolInfo,
                    & UpCallTable,
                    & m_proctable));
                if (! bypassing_call) {
                    error_code = (*WSPStartupFunc)(
                        wVersionRequested,
                        & WSPData,
                        lpProtocolInfo,
                        UpCallTable,
                        &m_proctable);
                    POSTAPINOTIFY((
                        DTCODE_WSPStartup,
                        & error_code,
                        LibraryPath,
                        & wVersionRequested,
                        & pWSPData,
                        & lpProtocolInfo,
                        & UpCallTable,
                        & m_proctable));
                } // if ! bypassing_call
            } // declaration block
#endif // !defined(DEBUG_TRACING)
        }
        __except (WS2_EXCEPTION_FILTER ()) {
            DEBUGF(DBG_ERR, ("Unhandled exception in WSPStartup for %s, code:%lx\n",
                    lpszLibFile, GetExceptionCode()));
            assert (FALSE);
        }

        if(ERROR_SUCCESS != error_code){
            DEBUGF(DBG_ERR, ("WSPStartup for %s Failed, err:%ld\n",
                    lpszLibFile, error_code));
            TRY_THROW(guard_memalloc);
        }

        //
        // Make sure that all of the procedures at least have a non null pointer.
        //
        if( !m_proctable.lpWSPAccept              ||
            !m_proctable.lpWSPAddressToString     ||
            !m_proctable.lpWSPAsyncSelect         ||
            !m_proctable.lpWSPBind                ||
            !m_proctable.lpWSPCancelBlockingCall  ||
            !m_proctable.lpWSPCleanup             ||
            !m_proctable.lpWSPCloseSocket         ||
            !m_proctable.lpWSPConnect             ||
            !m_proctable.lpWSPDuplicateSocket     ||
            !m_proctable.lpWSPEnumNetworkEvents   ||
            !m_proctable.lpWSPEventSelect         ||
            !m_proctable.lpWSPGetOverlappedResult ||
            !m_proctable.lpWSPGetPeerName         ||
            !m_proctable.lpWSPGetSockName         ||
            !m_proctable.lpWSPGetSockOpt          ||
            !m_proctable.lpWSPGetQOSByName        ||
            !m_proctable.lpWSPIoctl               ||
            !m_proctable.lpWSPJoinLeaf            ||
            !m_proctable.lpWSPListen              ||
            !m_proctable.lpWSPRecv                ||
            !m_proctable.lpWSPRecvDisconnect      ||
            !m_proctable.lpWSPRecvFrom            ||
            !m_proctable.lpWSPSelect              ||
            !m_proctable.lpWSPSend                ||
            !m_proctable.lpWSPSendDisconnect      ||
            !m_proctable.lpWSPSendTo              ||
            !m_proctable.lpWSPSetSockOpt          ||
            !m_proctable.lpWSPShutdown            ||
            !m_proctable.lpWSPSocket              ||
            !m_proctable.lpWSPStringToAddress ){

            DEBUGF(DBG_ERR,
                   ("Service provider %s returned an invalid procedure table\n",
                    lpszLibFile));
            error_code = WSAEINVALIDPROCTABLE;
            TRY_THROW(guard_memalloc);
        }

        //
        // Confirm that the WinSock service provider supports 2.2. If it supports a
        // version greater then 2.2 it will still return 2.2 since this is the
        // version  we requested.
        //
        if( WSPData.wVersion != WINSOCK_HIGH_SPI_VERSION ) {
            WSPCleanup (&error_code);
            DEBUGF( DBG_ERR,
                    ("Service Provider %s does not support version 2.2\n",
                     lpszLibFile));
            error_code = WSAVERNOTSUPPORTED;
            TRY_THROW(guard_memalloc);
        }
    } TRY_CATCH(guard_memalloc) {
        if (m_library_handle!=NULL) {
            FreeLibrary (m_library_handle);
            m_library_handle = NULL;
        }


#ifdef DEBUG_TRACING
        if (m_lib_name!=NULL) {
            delete m_lib_name;
            m_lib_name = NULL;
        }
#endif
    } TRY_END(guard_memalloc);

    return error_code;
} //Initailize





