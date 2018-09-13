//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//  Copyright (c) 1996 Microsoft Corporation
//
//  rprovide.cpp
//
//  Implements all of the RPROVIDER class.
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version. Parts of this code are modeled
//                         from the example LSP written by Intel.
//
//---------------------------------------------------------------------------

#include "precomp.h"
#include "globals.h"
#include <stdlib.h>



RPROVIDER::RPROVIDER()
/*++
Routine Description:

    Creates any internal state.

Arguments:

    None

Return Value:

    None

--*/

{
    m_proctable = NULL;
    m_library_handle = NULL;
    m_lib_name = NULL;
}



RPROVIDER::~RPROVIDER()
/*++
Routine Description:

    destroys any internal state.

Arguments:

    None

Return Value:

    None

--*/
{
    int ErrorCode;


    if (m_library_handle)
    {
        if (m_proctable->lpWSPCleanup)
        {
            DEBUGF( DBG_TRACE,
                    ("\nCalling WSPCleanup for provider %X", this));
            //Call the servce provider cleanup routine
            WSPCleanup(&ErrorCode);
        } //if

        // Free the service provider DLL
        FreeLibrary(m_library_handle);
    } //if

    if(m_proctable){
        delete m_proctable;
    }
    delete m_lib_name;
    DEBUGF( DBG_TRACE,
            ("\nDestroying provider %X", this));
}



INT
RPROVIDER::Initialize(
    IN PWCHAR lpszLibFile,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo
    )
/*++
Routine Description:

    Initializes the RPROVIDER object.

Arguments:

    lpszLibFile - A  Null  terminating  string  that  points to the .DLL of the
                  service to load.

    lpProtocolInfo - A pointer to a WSAPROTOCOL_INFO struct to hand to the
                     provider startup routine.

Return Value:

    If no error occurs, Initialize() returns ERROR_SUCEES.  Otherwise the value
    SOCKET_ERROR  is  returned,  and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    LPWSPSTARTUP        WSPStartupFunc          = NULL;
    WORD                wVersionRequested       = MAKEWORD(2,2);
    INT                 error_code              = 0;
    WSPDATA             WSPData;
    CHAR                LibraryPath[MAX_PATH];
    CHAR                lpszLibFileA[MAX_PATH];

    DEBUGF( DBG_TRACE,
            ("\nInitializing provider %X", this));

    m_proctable = new WSPPROC_TABLE;
    if(!m_proctable){
        DEBUGF(
            DBG_ERR,
            ("\nFailed to allocate WSPPROC_TABLE for provider object"));
        return WSA_NOT_ENOUGH_MEMORY;
    }

    // Zero out contents of m_proctable
    ZeroMemory(
        (PVOID) m_proctable,      // Destination
        sizeof(LPWSPPROC_TABLE)); // Length

    //
    // Expand the library name to pickup environment/registry variables
    //
    wcstombs (lpszLibFileA, lpszLibFile, MAX_PATH);
    if (!( ExpandEnvironmentStrings(lpszLibFileA,
                                    LibraryPath,
                                    MAX_PATH))){
        DEBUGF(
            DBG_ERR,
            ("\nExpansion of environment variables failed"));
        return WSASYSCALLFAILURE;
    } //if

    m_lib_name = (PWCHAR) new WCHAR[strlen(LibraryPath) + 1];
    if (m_lib_name == NULL) {
        DEBUGF(
            DBG_ERR,
            ("Allocation of m_lib_name failed\n"));
        return WSA_NOT_ENOUGH_MEMORY;
    }

    // fill m_lib_name with correct wide character string
    mbstowcs ( m_lib_name, LibraryPath, strlen(LibraryPath));

    //
    // First load the DLL for the service provider. Then get two functions that
    // init the service provider structures.
    //
    m_library_handle = LoadLibrary(LibraryPath);
    if(!m_library_handle){
        DEBUGF(
            DBG_ERR,
            ("\nFailed to load DLL %s",LibraryPath));
        return  WSAEPROVIDERFAILEDINIT;
    }

    WSPStartupFunc = (LPWSPSTARTUP)GetProcAddress(
        m_library_handle,
        "WSPStartup"
        );

    if(!(WSPStartupFunc)){

        DEBUGF( DBG_ERR,("\nCould get startup entry point for %s",
                         lpszLibFile));
        return  WSAEPROVIDERFAILEDINIT;
    }

#if !defined(DEBUG_TRACING)
    error_code = (*WSPStartupFunc)(
        wVersionRequested,
        & WSPData,
        lpProtocolInfo,
        g_UpCallTable,
        m_proctable);
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
            & g_UpCallTable,
            & m_proctable));
        if (! bypassing_call) {
            error_code = (*WSPStartupFunc)(
                wVersionRequested,
                & WSPData,
                lpProtocolInfo,
                g_UpCallTable,
                m_proctable);
            POSTAPINOTIFY((
                DTCODE_WSPStartup,
                & error_code,
                LibraryPath,
                & wVersionRequested,
                & pWSPData,
                & lpProtocolInfo,
                & g_UpCallTable,
                & m_proctable));
        } // if ! bypassing_call
    } // declaration block
#endif // !defined(DEBUG_TRACING)

    if(ERROR_SUCCESS != error_code){
        DEBUGF(DBG_ERR, ("\nWSPStartup for %s Failed",lpszLibFile));
        return error_code;
    }

    //
    // Make sure that all of the procedures at least have a non null pointer.
    //
    if( !m_proctable->lpWSPAccept              ||
        !m_proctable->lpWSPAddressToString     ||
        !m_proctable->lpWSPAsyncSelect         ||
        !m_proctable->lpWSPBind                ||
        !m_proctable->lpWSPCancelBlockingCall  ||
        !m_proctable->lpWSPCleanup             ||
        !m_proctable->lpWSPCloseSocket         ||
        !m_proctable->lpWSPConnect             ||
        !m_proctable->lpWSPDuplicateSocket     ||
        !m_proctable->lpWSPEnumNetworkEvents   ||
        !m_proctable->lpWSPEventSelect         ||
        !m_proctable->lpWSPGetOverlappedResult ||
        !m_proctable->lpWSPGetPeerName         ||
        !m_proctable->lpWSPGetSockName         ||
        !m_proctable->lpWSPGetSockOpt          ||
        !m_proctable->lpWSPGetQOSByName        ||
        !m_proctable->lpWSPIoctl               ||
        !m_proctable->lpWSPJoinLeaf            ||
        !m_proctable->lpWSPListen              ||
        !m_proctable->lpWSPRecv                ||
        !m_proctable->lpWSPRecvDisconnect      ||
        !m_proctable->lpWSPRecvFrom            ||
        !m_proctable->lpWSPSelect              ||
        !m_proctable->lpWSPSend                ||
        !m_proctable->lpWSPSendDisconnect      ||
        !m_proctable->lpWSPSendTo              ||
        !m_proctable->lpWSPSetSockOpt          ||
        !m_proctable->lpWSPShutdown            ||
        !m_proctable->lpWSPSocket              ||
        !m_proctable->lpWSPStringToAddress ){

        DEBUGF(DBG_ERR,
               ("\nService provider %s returned an invalid procedure table",
                lpszLibFile));
        return WSAEINVALIDPROCTABLE;
    }

    //
    // Confirm that the WinSock service provider supports 2.0. If it supports a
    // version greater then 2.0 it will still return 2.0 since this is the
    // version  we requested.
    //
    if(WSPData.wVersion != MAKEWORD(2,2)){
        if(m_proctable->lpWSPCleanup) {
            if(m_proctable->lpWSPCleanup(&error_code)){
                DEBUGF( DBG_ERR,
                        ("\nService Provider %s does not support version 2.2",
                         lpszLibFile));
                return WSAVERNOTSUPPORTED;
            }
        }
        return WSAEINVALIDPROVIDER;
    }

    return ERROR_SUCCESS;
} //Initailize
