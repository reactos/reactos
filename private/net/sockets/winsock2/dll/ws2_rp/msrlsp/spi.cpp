//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//
//  spi.cpp
//
//  Implements all of the WSP entry points for the WinSock2 restricted
//  process LSP.
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version. Parts of this code are modeled
//                         from the example LSP written by Intel.
//
//---------------------------------------------------------------------------

#include "precomp.h"
#include <svcguid.h>
#include <ws2tcpip.h>
#include "ws2_rp.h"

// WinSock2 UpCallTable to access the WPUxxx functions.
WSPUPCALLTABLE g_UpCallTable;

// Variables to track Startup/Cleanup Pairing.
CRITICAL_SECTION  g_InitCriticalSection;
DWORD             g_StartupCount = 0;

// The catalog of providers
PDCATALOG gProviderCatalog;

// The worker thread for async and overlapped functions
PRWORKERTHREAD gWorkerThread = 0;

// The buffer manager for providers that modify the data stream
PDBUFFERMANAGER gBufferManager;

#ifdef DEBUG_TRACING
static char g_szLibraryName[MAX_PATH] = "MSRLSP.DLL";
#endif

//------------------------------------------------------------------------
// Types to support dynamically loading OLE.
//------------------------------------------------------------------------

typedef HRESULT (*PF_COINITIALIZE)( LPVOID );

typedef HRESULT (*PF_COCREATEINSTANCE)( REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID* );

typedef struct _OLE_INFO
   {
   PF_COINITIALIZE      CoInitialize;
   PF_COCREATEINSTANCE  CoCreateInstance;
   } OLE_INFO;

//------------------------------------------------------------------------
// Globals:
//------------------------------------------------------------------------

static HINSTANCE            g_hOleLib = 0;
static OLE_INFO            *g_pOle = 0;

static GUID HostnameGuid = SVCID_INET_HOSTADDRBYNAME;
static GUID AddressGuid  = SVCID_INET_HOSTADDRBYINETSTRING;

//--------------------------------------------------------------------
// Restricted Process WinSock Helpers
//
//    Object:    {570da105-3c30-11d1-8bf1-0000f8754035}
//    Interface: {3ae0b7e0-3c19-11d1-8bf1-0000f8754035}
//--------------------------------------------------------------------

static const GUID CLSID_RestrictedProcess =
    {
    0x570da105,
    0x3c30,
    0x11d1,
    0x8b, 0xf1, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35
    };

#if FALSE
static const GUID IID_IRestrictedProcess =
    {
    0x3ae0b7e0,
    0x3c19,
    0x11d1,
    0x8b, 0xf1, 0x00, 0x00, 0xf8, 0x75, 0x40, 0x35
    };
#endif

//------------------------------------------------------------------------
// RP_InitOle()
//
// We don't want to load OLE32.DLL unless we actually need it. This one
// loads OLE32 for our use.
//
// Return TRUE on success, FALSE on failure.
//------------------------------------------------------------------------
BOOL RP_InitOle()
    {
    HRESULT  hr;

    if (g_hOleLib)
        {
        return TRUE;
        }

    g_hOleLib = LoadLibrary(TEXT("ole32.dll"));
    if (!g_hOleLib)
        {
        return FALSE;
        }

    g_pOle = new OLE_INFO;
    if (!g_pOle)
        {
        FreeLibrary(g_hOleLib);
        return FALSE;
        }

    g_pOle->CoInitialize = (PF_COINITIALIZE)GetProcAddress(g_hOleLib,
                                                           "CoInitialize");

    g_pOle->CoCreateInstance = (PF_COCREATEINSTANCE)GetProcAddress(g_hOleLib,
                                                           "CoCreateInstance");

    if ((!g_pOle->CoInitialize) || (!g_pOle->CoCreateInstance))
        {
        delete g_pOle;
        g_pOle = 0;
        FreeLibrary(g_hOleLib);
        return FALSE;
        }

    hr = g_pOle->CoInitialize(0);
    if (FAILED(hr))
        {
        delete g_pOle;
        g_pOle = 0;
        FreeLibrary(g_hOleLib);
        return FALSE;
        }

    return TRUE;
    }

//---------------------------------------------------------------------------
//  CreateSocketHandle()
//
//---------------------------------------------------------------------------
SOCKET CreateSocketHandle( IRestrictedProcess *pIRestrictedProcess,
                           DWORD               dwCatalogEntryId,
                           ULONG_PTR            dwContext,
                           INT                *pError )
    {
    SOCKET  Socket;

    #if defined(LOCAL_HANDLES)

    Socket = g_UpCallTable.lpWPUCreateSocketHandle( dwCatalogEntryId,
                                                    dwContext,
                                                    pError);

    #elif defined(LOCAL_EVENT_HANDLES)

    Socket = (SOCKET)CreateEvent(NULL,FALSE,FALSE,NULL);
    if (!Socket)
        {
        Socket = SOCKET_ERROR;
        *pError = GetLastError();
        }
    else
        {
        Socket = g_UpCallTable.lpWPUModifyIFSHandle( dwCatalogEntryId,
                                                     Socket,
                                                     pError );
        }

    #else

    HRESULT  hr;
    DWORD    dwPid = GetCurrentProcessId();
    DWORD    dwStatus;
    HANDLE   hHelperHandle;

    hr = pIRestrictedProcess->RP_WahOpenHandleHelper( dwPid,
                                                      (DWORD*)&hHelperHandle,
                                                      &dwStatus );
    if (FAILED(hr) || (*pError != NO_ERROR) )
        {
        *pError = WSASYSCALLFAILURE;
        return SOCKET_ERROR;
        }

    hr = pIRestrictedProcess->RP_WahCreateSocketHandle( dwPid,
                                                        (DWORD)hHelperHandle,
                                                        (DWORD*)&Socket,
                                                        &dwStatus );
    if (FAILED(hr) || (*pError != NO_ERROR) )
        {
        // BUGBUG: Whoa! need to close the hHelperHandle here!
        *pError = WSASYSCALLFAILURE;
        return SOCKET_ERROR;
        }

    Socket = g_UpCallTable.lpWPUModifyIFSHandle( dwCatalogEntryId,
                                                 Socket,
                                                 pError );


    #endif

    return Socket;
    }

//---------------------------------------------------------------------------
//  CloseSocketHandle()
//
//---------------------------------------------------------------------------
INT CloseSocketHandle( RSOCKET *pRSocket,
                       INT     *pError  )
    {
    INT  iStatus;

    #if defined(LOCAL_HANDLES)

    iStatus = g_UpCallTable.lpWPUCloseSocketHandle( pRSocket->GetContext(),
                                                    pError );

    #elif defined(LOCAL_EVENT_HANDLES)

    if ( CloseHandle( (HANDLE)(pRSocket->GetContext()) ) )
        {
        iStatus = NO_ERROR;
        *pError = NO_ERROR;
        }
    else
        {
        iStatus = SOCKET_ERROR;
        *pError = GetLastError();
        }

    #else

    // BUGBUG: Need to code in WahCloseSocketHandle()...

    #endif

    return iStatus;
    }

//---------------------------------------------------------------------------
//  WSPAccept()
//
//  Conditionally  accept a connection based on the return value of a condition
//  function, and optionally create and/or join a socket group.
//
//  If no error occurs, WSPAccept() returns a value of type SOCKET which is a
//  new descriptor for the accepted socket. Otherwise, a value of
//  INVALID_SOCKET is returned, and a specific error code is available in
//  *pError.
//
//  NOTE: For a restricted process, WSPAccept() always fails.
//
//  s              - A descriptor identiying a socket which is listening for
//                   connections after a WSPListen().
//
//  addr           - An optional pointer to a buffer which receives the address
//                   of the connecting  entity, as known to  the  service
//                   provider. The exact format of the  addr arguement is
//                   determined by the address family established when the
//                   socket was created.
//
//  addrlen        - An optional pointer to an integer which contains the
//                   length of the address addr.
//
//  lpfnCondition  - The procedure instance address of an optional, WinSock2
//                   client supplied condition function which will make an
//                   accept/reject decision based on the caller information
//                   passed in as parameters, and optionally creaetd and/or
//                   join a socket group by assigning an appropriate value
//                   to the result parameter of this function.
//
//  dwCallbackData - Callback data to be passed back to the WinSock2 client
//                   as a condition function parameter. This parameter is
//                   not interpreted by the service provider.
//
//  pError         - A pointer to the error code.
//
//---------------------------------------------------------------------------
SOCKET WSPAPI WSPAccept( IN  SOCKET s,
                         OUT struct sockaddr FAR *addr,
                         OUT INT FAR *addrlen,
                         IN  LPCONDITIONPROC lpfnCondition,
                         IN  ULONG_PTR dwCallbackData,
                         OUT INT FAR *pError )
    {
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pSocket;
    RPROVIDER   *pProvider;
    RSOCKET     *pNewRSocket;
    SOCKET       ProviderSocket;
    SOCKET       NewProviderSocket;


    // Debug/Trace stuff

    if (PREAPINOTIFY(( DTCODE_WSPAccept,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &addr,
                       &addrlen,
                       &lpfnCondition,
                       &dwCallbackData,
                       &pError)) )
        {
        return ReturnValue;
        }

    #if FALSE
    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);

    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        NewProviderSocket = Provider->WSPAccept( ProviderSocket,
                                                 addr,
                                                 addrlen,
                                                 lpfnCondition,
                                                 dwCallbackData,
                                                 pError);

        if (INVALID_SOCKET != NewProviderSocket)
            {
            // Create a new socket object and initialize it.
            pNewRSocket = new RSOCKET;
            if (pNewRSocket)
                {
                ReturnValue = CreateSocketHandle(
                                       0,
                                       pRSocket->GetCatalogEntryId(),
                                       (ULONG_PTR)pNewRSocket,
                                       pError );

                DEBUGF( DBG_TRACE,
                        ("Accept Returning Socket %X\n", ReturnValue));

                if (INVALID_SOCKET != ReturnValue)
                    {
                    pNewRSocket->Initialize( Provider,
                                             NewProviderSocket,
                                             pRSocket->GetCatalogEntryId(),
                                             ReturnValue,
                                             0  ); // pIRestrictedProcess

                    // Add this socket to the list of open sockets:
                    RSOCKET::AddListDSocket( pNewRSocket );
                    }
                else
                    {
                    delete pNewRSocket;
                    }
                }
            }
        }
    #else

    ReturnValue = SOCKET_ERROR;
    *pError = WSAEACCES;

    #endif

    POSTAPINOTIFY(( DTCODE_WSPAccept,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &addr,
                    &addrlen,
                    &lpfnCondition,
                    &dwCallbackData,
                    &pError));

    return ReturnValue;
    }


//---------------------------------------------------------------------------
//  WSPAddressToString()
//
//  Convert a SOCKADDR structure into a human-readable string representation
//  of the address. This is intended to be used mainly for display purposes.
//  If the caller wishes the translation to be done by a particular provider,
//  it should supply the corresponding WSAPROTOCOL_INFO struct in the
//  lpProtocolInfo parameter.
//
//  The return value is NO_ERROR if the operation was successful. Otherwise
//  SOCKET_ERROR is returned.
//
//  lpsaAddress     - SOCKADDR pointer to translate into a string.
//
//  dwAddressLength - Length of the Address SOCKADDR.
//
//  lpProtocolInfo  - (optional) the WSAPROTOCOL_INFO struct for a particular
//                    provider.
//
//  lpszAddressString - Buffer which receives the human-readable address
//                      string.
//
//  lpdwAddressStringLength - On input, the length of the AddressString buffer.
//                            On output, returns the length of  the string
//                            actually copied into the buffer.
//---------------------------------------------------------------------------
INT WSPAPI WSPAddressToString( IN     LPSOCKADDR lpsaAddress,
                               IN     DWORD dwAddressLength,
                               IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
                               OUT    LPWSTR lpszAddressString,
                               IN OUT LPDWORD lpdwAddressStringLength,
                               OUT    LPINT pError )
    {
    INT                 ReturnValue;
    DWORD               NextProviderCatalogEntryId;
    DWORD               ThisProviderCatalogEntryId;
    PPROTO_CATALOG_ITEM CatalogItem;
    RPROVIDER          *pProvider;

    if (PREAPINOTIFY(( DTCODE_WSPAddressToString,
                       &ReturnValue,
                       g_szLibraryName,
                       &lpsaAddress,
                       &dwAddressLength,
                       &lpProtocolInfo,
                       &lpszAddressString,
                       &lpdwAddressStringLength,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get the catlog entry for the next provider in the chain
    //
    ReturnValue = gProviderCatalog->FindNextProviderInChain(
                                          lpProtocolInfo,
                                          &ThisProviderCatalogEntryId,
                                          &NextProviderCatalogEntryId );

    if (ReturnValue == NO_ERROR)
        {
        //
        // Get the provider for the catlog entry
        //
        ReturnValue = gProviderCatalog->GetCatalogItemFromCatalogEntryId(
                                              NextProviderCatalogEntryId,
                                              &CatalogItem );
        if (ReturnValue == NO_ERROR)
            {
            //
            // Get the provider for the catlog entry
            //
            ReturnValue = gProviderCatalog->GetCatalogItemFromCatalogEntryId(
                                              NextProviderCatalogEntryId,
                                              &CatalogItem);
            if (ReturnValue == NO_ERROR)
                {
                //
                // The the next providers WSPSocket
                //
                pProvider = CatalogItem->GetProvider();
                ReturnValue = pProvider->WSPAddressToString(
                                             lpsaAddress,
                                             dwAddressLength,
                                             lpProtocolInfo,
                                             lpszAddressString,
                                             lpdwAddressStringLength,
                                             pError );
                }
            else
                {
                *pError = WSAEINVAL;
                }
            }
        else
            {
            *pError = WSAEINVAL;
            }
        }
    else
        {
        *pError = WSAEINVAL;
        }

    POSTAPINOTIFY(( DTCODE_WSPAddressToString,
                    &ReturnValue,
                    g_szLibraryName,
                    &lpsaAddress,
                    &dwAddressLength,
                    &lpProtocolInfo,
                    &lpszAddressString,
                    &lpdwAddressStringLength,
                    &pError));

    return(ReturnValue);
}





INT
WSPAPI
WSPAsyncSelect(
    IN SOCKET s,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN long lEvent,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Request  Windows  message-based  event notification of network events for a
    socket.

Arguments:

    s       - A  descriptor identiying a socket for which event notification is
              required.

    hWnd    - A  handle  identifying  the window which should receive a message
              when a network event occurs.

    wMsg    - The message to be sent when a network event occurs.

    lEvent  - bitmask  which specifies a combination of network events in which
              the WinSock client is interested.

    pError - A pointer to the error code.

Return Value:

    The  return  value  is 0 if the WinSock client's declaration of interest in
    the  netowrk event set was successful.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error code is available in pError.

--*/
{
    INT      ReturnValue = NO_ERROR;
    RSOCKET *pRSocket;

    if (PREAPINOTIFY(( DTCODE_WSPAsyncSelect,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &hWnd,
                       &wMsg,
                       &lEvent,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        *pError = pRSocket->RegisterAsyncOperation( hWnd,
                                                    wMsg,
                                                    lEvent );
        if (NO_ERROR == *pError)
            {
            ReturnValue = NO_ERROR;
            }
        else
            {
            ReturnValue = SOCKET_ERROR;
            }
        }

    POSTAPINOTIFY(( DTCODE_WSPAsyncSelect,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &hWnd,
                    &wMsg,
                    &lEvent,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPBind(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN INT namelen,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Associate a local address (i.e. name) with a socket.

Arguments:

    s       - A descriptor identifying an unbound socket.

    name    - The  address  to assign to the socket.  The sockaddr structure is
              defined as follows:

              struct sockaddr {
                  u_short sa_family;
                  char    sa_data[14];
              };

              Except  for  the sa_family field,
sockaddr contents are epxressed
              in network byte order.

    namelen - The length of the name.

    pError - A pointer to the error code.

Return Value:

    If   no   erro   occurs,  WSPBind()  returns  0.   Otherwise, it  returns
    SOCKET_ERROR, and a specific error code is available in pError.

--*/
{
    INT          ReturnValue = NO_ERROR;
    RPROVIDER   *pProvider;
    RSOCKET     *pRSocket;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPBind,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &name,
                       &namelen,
                       &pError)) ) {

        return(ReturnValue);
    }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPBind( ProviderSocket,
                                          name,
                                          namelen,
                                          pError);

        POSTAPINOTIFY(( DTCODE_WSPBind,
                        &ReturnValue,
                        g_szLibraryName,
                        &s,
                        &name,
                        &namelen,
                        &pError));
        }

    return ReturnValue;
    }



INT
WSPAPI
WSPCancelBlockingCall(OUT INT FAR *pError)
/*++
Routine Description:

    Cancel a blocking call which is currently in progress.

Arguments:

    pError - A pointer to the error code.

Return Value:

    The  value  returned  by  WSPCancelBlockingCall() is 0 if the operation was
    successfully canceled.  Otherwise the value SOCKET_ERROR is returned,
and a
    specific error code is available in pError.

--*/
{
 //     INT ReturnValue;

//      if (PREAPINOTIFY(( DTCODE_WSPCancelBlockingCall,
//                         &ReturnValue,
//                         g_szLibraryName,
//                         &pError)) ) {
//          return(ReturnValue);
//      }

//      ReturnValue = gProcTable.lpWSPCancelBlockingCall(
//          pError);

//      POSTAPINOTIFY(( DTCODE_WSPCancelBlockingCall,
//                      &ReturnValue,
//                      g_szLibraryName,
//                      &pError))
//      return(ReturnValue);
    // ****
    // Note:
    // We are failing this call right now since I am lobbing for a spec change
    // so we dont have to keep per thread info. If the spec change does not go
    // through we will have to write down the last provider called by a thread
    // in thread local storage so we can know what provider to send this call
    // to.
    return(WSAEINVAL);

}



//---------------------------------------------------------------------------
//  WSPCleanup()
//
//  Terminate use of the WinSock service provider. Return NO_ERROR if the
//  service provider terminates successfully. On error, return SOCKET_ERROR
//  with a specific error code in *pError.
//---------------------------------------------------------------------------
INT WSPAPI WSPCleanup( OUT INT *pError )
{
    INT          ReturnValue;
    INT          Errno;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;

    if (PREAPINOTIFY(( DTCODE_WSPCleanup,
                       &ReturnValue,
                       g_szLibraryName,
                       &pError)) )
        {
        return(ReturnValue);
        }

    EnterCriticalSection(&g_InitCriticalSection);

    if (g_StartupCount > 0)
        {
        g_StartupCount--;
        if (g_StartupCount == 0)
            {

            DEBUGF( DBG_TRACE, ("Shutdown down LSP\n") );

            // Kill all currently open sockets:
            while (pRSocket = RSOCKET::RemoveHeadListDSocket())
                {
                ProviderSocket = pRSocket->GetSocketHandle();

                pProvider = pRSocket->GetDProvider();

                pProvider->WSPCloseSocket( ProviderSocket, &Errno );

                CloseSocketHandle( pRSocket, &Errno );

                delete pRSocket;
                }

            // Kill the ProviderCatalog
            delete(gProviderCatalog);

            // Terminate worker thread:
            delete(gWorkerThread);

            // Kill the buffer manager:
            delete(gBufferManager);
            DEBUGF( DBG_TRACE, ("Shutdown of LSP finished\n") );

            }
       }

    ReturnValue = NO_ERROR;

    LeaveCriticalSection(&g_InitCriticalSection);

    POSTAPINOTIFY(( DTCODE_WSPCleanup,
                    &ReturnValue,
                    g_szLibraryName,
                    &pError));

    return ReturnValue;
    }

//---------------------------------------------------------------------------
//  WSPCloseSocket()
//
//  Close a currently open socket.
//
//  If no error occurs, WSPCloseSocket() returns NO_ERROR. Otherwise, a
//  value of SOCKET_ERROR is returned, and the specific error code is
//  returned in *pError.
//---------------------------------------------------------------------------
INT WSPAPI WSPCloseSocket( IN  SOCKET s,
                           OUT INT   *pError )
{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPCloseSocket,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);

    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        DEBUGF( DBG_TRACE,
                ("Closing socket %X\n",s));

        ReturnValue = pProvider->WSPCloseSocket( ProviderSocket, pError );

        CloseSocketHandle( pRSocket, pError );

        RSOCKET::RemoveListDSocket( pRSocket );

        delete(pRSocket);
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }


    POSTAPINOTIFY(( DTCODE_WSPCloseSocket,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &pError));

    return ReturnValue;
    }

//---------------------------------------------------------------------------
//  WSPConnect()
//
//  Establish a connection to the specified sockaddr, exchange any connect
//  data, and specify any needed quality of service (QOS) attributes.
//
//  If no error occurs, WSPConnect() returns NO_ERROR. Otherwise,
//  return SOCKET_ERROR, and a specific error code is returned in pError.
//
//  s            - A descriptor identifying an unconnected socket.
//
//  name         - The name of the peer to which the socket is to be connected.
//
//  namelen      - The length of the name.
//
//  lpCallerData - A  pointer to the user data that is to be transferred to
//                 the peer during connection established.
//
//  lpCalleeData - A pointer to a buffer into which may be copied any user data
//                 received from the peer during connection establishment.
//
//  lpSQOS       - A pointer to the flow specs for socket s, one for each
//                 direction.
//
//  lpGQOS       - A pointer to the flow spec for the socket group (if
//                 applicable).
//
//  pError       - A pointer to the error code.
//
//---------------------------------------------------------------------------
INT WSPAPI WSPConnect( IN SOCKET s,
                       IN const struct sockaddr FAR *name,
                       IN INT namelen,
                       IN LPWSABUF lpCallerData,
                       IN LPWSABUF lpCalleeData,
                       IN LPQOS lpSQOS,
                       IN LPQOS lpGQOS,
                       OUT INT FAR *pError )
    {
    INT             ReturnValue = NO_ERROR;
    RSOCKET        *pRSocket;
    SOCKET          ProviderSocket;
    DWORD           dwPid;
    HRESULT         hr;
    IRestrictedProcess *pIRestrictedProcess;

    if (PREAPINOTIFY(( DTCODE_WSPConnect,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &name,
                       &namelen,
                       &lpCallerData,
                       &lpCalleeData,
                       &lpSQOS,
                       &lpGQOS,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (!pRSocket)
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }
    else
        {
        *pError = NO_ERROR;
        }

    if (ReturnValue != SOCKET_ERROR)
        {
        pIRestrictedProcess = pRSocket->GetIRestrictedProcess();

        if (pIRestrictedProcess)
            {
            dwPid = GetCurrentProcessId();
            hr = pIRestrictedProcess->RP_WSPConnect(
                                        (RPC_SOCKADDR_IN*)name,
                                        namelen,
                                        (RPC_WSABUF*)lpCallerData,
                                        (RPC_WSABUF*)lpCalleeData,
                                        (RPC_QOS*)lpSQOS,
                                        (RPC_QOS*)lpGQOS,
                                        dwPid,
                                        (DWORD*)&ProviderSocket,
                                        (long*)pError );
            if (FAILED(hr))
                {
                *pError = WSAEACCES;
                ReturnValue = SOCKET_ERROR;
                }

            if (*pError == ERROR_SUCCESS)
                {
                pRSocket->SetSocketHandle(ProviderSocket);
                }
            else
                {
                ReturnValue = SOCKET_ERROR;
                }
            }
        else
            {
            *pError = WSAEACCES;
            ReturnValue = SOCKET_ERROR;
            }
        }

    pIRestrictedProcess->Release();
    pRSocket->SetIRestrictedProcess(0);

    POSTAPINOTIFY(( DTCODE_WSPConnect,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &name,
                    &namelen,
                    &lpCallerData,
                    &lpCalleeData,
                    &lpSQOS,
                    &lpGQOS,
                    &pError));

    return ReturnValue;
    }

INT
WSPAPI
WSPDuplicateSocket(
    IN SOCKET s,
    IN DWORD dwProcessID,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    descriptor for a shared socket.


Arguments:

    s              - Specifies the local socket descriptor.

    dwProcessID    - Specifies  the  ID  of  the  target  process for which the
                     shared socket will be used.

    lpProtocolInfo - A  pointer  to  a  buffer  allocated by the client that is
                     large enough to contain a WSAPROTOCOL_INFOA struct.  The
                     service  provider copies the protocol info struct contents
                     to this buffer.

    pError        - A pointer to the error code

Return Value:

    If  no  error  occurs, WPSDuplicateSocket()  returns zero.  Otherwise, the
    value of SOCKET_ERROR is returned, and a specific error number is available
    in pError.

--*/
{
    INT ReturnValue=SOCKET_ERROR;

//      if (PREAPINOTIFY(( DTCODE_WSPDuplicateSocket,
//                         &ReturnValue,
//                         g_szLibraryName,
//                         &s,
//                         &dwProcessID,
//                         &lpProtocolInfo,
//                         &pError)) ) {
//          return(ReturnValue);
//      }

//      ReturnValue = gProcTable.lpWSPDuplicateSocket(
//          s,
//          dwProcessID,
//          lpProtocolInfo,
//          pError);

//      POSTAPINOTIFY(( DTCODE_WSPDuplicateSocket,
//                      &ReturnValue,
//                      g_szLibraryName,
//                      &s,
//                      &dwProcessID,
//                      &lpProtocolInfo,
//                      &pError));

    return(ReturnValue);

}



INT
WSPAPI
WSPEnumNetworkEvents(
    IN SOCKET s,
    OUT WSAEVENT hEventObject,
    OUT LPWSANETWORKEVENTS lpNetworkEvents,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Report occurrences of network events for the indicated socket.

Arguments:

    s               - A descriptor identifying the socket.

    hEventObject    - An optional handle identifying an associated event object
                      to be reset.

    lpNetworkEvents - A  pointer  to  a WSANETWORKEVENTS struct which is filled
                      with   a  record  of  occurred  network  events  and  any
                      associated error codes.

    pError         - A pointer to the error code.

Return Value:

    The  return  value  is  NO_ERROR  if  the  operation  was  successful.
    Otherwise  the  value SOCKET_ERROR is returned, and a specific error number
    is available in pError.

--*/
{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;

    if (PREAPINOTIFY(( DTCODE_WSPEnumNetworkEvents,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &hEventObject,
                       &lpNetworkEvents,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPEnumNetworkEvents(
                                     ProviderSocket,
                                     hEventObject,
                                     lpNetworkEvents,
                                     pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPEnumNetworkEvents,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &hEventObject,
                    &lpNetworkEvents,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPEventSelect(
    IN SOCKET s,
    IN OUT WSAEVENT hEventObject,
    IN long lNetworkEvents,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Specify  an  event object to be associated with the supplied set of network
    events.

Arguments:

    s              - A descriptor identifying the socket.

    hEventObject   - A  handle  identifying  the  event object to be associated
                     with the supplied set of network events.

    lNetworkEvents - A  bitmask  which  specifies  the  combination  of network
                     events in which the WinSock client has interest.

    pError        - A pointer to the error code.

Return Value:

    The return value is 0 if the WinSock client's specification of the network
    events and the associated event object was successful. Otherwise the value
    SOCKET_ERROR is returned, and a specific error number is available in
    pError

--*/
{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPEventSelect,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &hEventObject,
                       &lNetworkEvents,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPEventSelect( ProviderSocket,
                                                 hEventObject,
                                                 lNetworkEvents,
                                                 pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPEventSelect,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &hEventObject,
                    &lNetworkEvents,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPGetOverlappedResult(
    IN SOCKET s,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPDWORD lpcbTransfer,
    IN BOOL fWait,
    OUT LPDWORD lpdwFlags,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Returns the results of an overlapped operation on the specified socket.

Arguments:

    s            - Identifies  the  socket.   This  is the same socket that was
                   specified  when  the  overlapped  operation was started by a
                   call to WSPRecv(), WSPRecvFrom(), WSPSend(), WSPSendTo(), or
                   WSPIoctl().

    lpOverlapped - Points to a WSAOVERLAPPED structure that was specified
                   when the overlapped operation was started.

    lpcbTransfer - Points to a 32-bit variable that receives the number of
                   bytes that were actually transferred by a send or receive
                   operation, or by WSPIoctl().

    fWait        - Specifies  whether  the function should wait for the pending
                   overlapped  operation  to  complete.   If TRUE, the function
                   does  not return until the operation has been completed.  If
                   FALSE  and  the  operation  is  still  pending, the function
                   returns FALSE and lperrno is WSA_IO_INCOMPLETE.

    lpdwFlags    - Points  to  a  32-bit variable that will receive one or more
                   flags   that  supplement  the  completion  status.   If  the
                   overlapped   operation   was   initiated  via  WSPRecv()  or
                   WSPRecvFrom(), this parameter will contain the results value
                   for lpFlags parameter.

    pError      - A pointer to the error code.

Return Value:

    If WSPGetOverlappedResult() succeeds,the return value is TRUE.  This means
    that the overlapped operation has completed successfully and that the value
    pointed  to  by lpcbTransfer has been updated.  If WSPGetOverlappedResult()
    returns  FALSE,  this  means  that  either the overlapped operation has not
    completed  or  the  overlapped operation completed but with errors, or that
    completion  status  could  not  be  determined due to errors in one or more
    parameters  to  WSPGetOverlappedResult().  On failure, the value pointed to
    by  lpcbTransfer  will  not be updated.  pError indicates the cause of the
    failure (either of WSPGetOverlappedResult() or of the associated overlapped
    operation).

--*/
{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPGetOverlappedResult,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpOverlapped,
                       &lpcbTransfer,
                       &fWait,
                       &lpdwFlags,
                       &pError)) ) {
        return(ReturnValue);
    }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        // BUGBUG: Need to get more involved here?
        ReturnValue = pProvider->WSPGetOverlappedResult(
                                         ProviderSocket,
                                         lpOverlapped,
                                         lpcbTransfer,
                                         fWait,
                                         lpdwFlags,
                                         pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPGetOverlappedResult,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpOverlapped,
                    &lpcbTransfer,
                    &fWait,
                    &lpdwFlags,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPGetPeerName(
    IN SOCKET s,
    OUT struct sockaddr FAR *name,
    OUT INT FAR *namelen,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Get the address of the peer to which a socket is connected.

Arguments:

    s       - A descriptor identifying a connected socket.

    name    - A  pointer  to  the structure which is to receive the name of the
              peer.

    namelen - A  pointer  to  an integer which, on input, indicates the size of
              the  structure  pointed  to  by name, and on output indicates the
              size of the returned name.

    pError - A pointer to the error code.

Return Value:

    If  no  error occurs, WSPGetPeerName() returns NO_ERROR.  Otherwise, a
    value  of  SOCKET_ERROR is returned, and a specific error code is available
    in pError

--*/
    {
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPGetPeerName,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &name,
                       &namelen,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPGetPeerName( ProviderSocket,
                                                 name,
                                                 namelen,
                                                 pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPGetPeerName,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &name,
                    &namelen,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPGetQOSByName(
    IN SOCKET s,
    IN LPWSABUF lpQOSName,
    IN LPQOS lpQOS,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Initializes a QOS structure based on a named template.

Arguments:

    s         - A descriptor identifying a socket.

    lpQOSName - Specifies the QOS template name.

    lpQOS     - A pointer to the QOS structure to be filled.

    pError   - A pointer to the error code.

Return Value:

    If the function succeeds, the return value is TRUE.  If the function fails,
    the  return  value  is  FALSE, and  a  specific error code is available in
    pError.

--*/
    {
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPGetQOSByName,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpQOSName,
                       &lpQOS,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPGetQOSByName( ProviderSocket,
                                                  lpQOSName,
                                                  lpQOS,
                                                  pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPGetQOSByName,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpQOSName,
                    &lpQOS,
                    &pError));

    return(ReturnValue);
}



INT
WSPAPI
WSPGetSockName(
    IN SOCKET s,
    OUT struct sockaddr FAR *name,
    OUT INT FAR *namelen,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Get the local name for a socket.

Arguments:

    s       - A descriptor identifying a bound socket.

    name    - A pointer to a structure used to supply the address (name) of the
              socket.

    namelen - A  pointer  to  an integer which, on input, indicates the size of
              the  structure  pointed  to  by name, and on output indicates the
              size of the returned name

    pError - A Pointer to the error code.

Return Value:

    If  no  error occurs, WSPGetSockName() returns NO_ERROR.  Otherwise, a
    value  of  SOCKET_ERROR is returned, and a specific error code is available
    in pError.

--*/
    {
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPGetSockName,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &name,
                       &namelen,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPGetSockName( ProviderSocket,
                                                 name,
                                                 namelen,
                                                 pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPGetSockName,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &name,
                    &namelen,
                    &pError));

    return ReturnValue;
    }

//---------------------------------------------------------------------------
//  WSPGetSockOpt()
//
//  Query a socket for a specific option setting.
//
//  If no error occurs, WSPGetSockOpt() returns 0. Otherwise, a value of
//  SOCKET_ERROR is returned, and a specific error code is returned in
//  pError.
//
//  s       - A descriptor identifying a socket.
//
//  level   - The level at  which the option is defined; the supported levels
//            include SOL_SOCKET (See annex for more protocol-specific levels.)
//
//  optname - The socket option for which the value is to be retrieved.
//
//  optval  - A pointer to the buffer in which the value for the requested
//            option is to be returned.
//
//  optlen  - A pointer to the size of the optval buffer.
//
//  pError  - A pointer to the error code.
//
//---------------------------------------------------------------------------
INT WSPAPI WSPGetSockOpt( IN SOCKET s,
                          IN INT level,
                          IN INT optname,
                          OUT char FAR *optval,
                          OUT INT FAR *optlen,
                          OUT INT FAR *pError )
    {
    INT ReturnValue = NO_ERROR;
    PRSOCKET     pRSocket;
    PRPROVIDER   pProvider;
    SOCKET       ProviderSocket;
    HRESULT      hr;
    IRestrictedProcess *pIRestrictedProcess;


    if (PREAPINOTIFY(( DTCODE_WSPGetSockOpt,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &level,
                       &optname,
                       &optval,
                       &optlen,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pIRestrictedProcess = pRSocket->GetIRestrictedProcess();
        if (pIRestrictedProcess)
            {
            // Note that in this case, the Provider Socket is still
            // remote, in the helper process.
            hr = pIRestrictedProcess->RP_GetSockOpt(level,
                                                    optname,
                                                    (UCHAR*)optval,
                                                    optlen,
                                                    pError );
            if (FAILED(hr))
                {
                *pError = WSAEACCES;
                ReturnValue = SOCKET_ERROR;
                }
            else if (*pError)
                {
                ReturnValue = SOCKET_ERROR;
                }
            }
        else
            {
            pProvider = pRSocket->GetDProvider();
            ProviderSocket = pRSocket->GetSocketHandle();

            ReturnValue = pProvider->WSPGetSockOpt( ProviderSocket,
                                                    level,
                                                    optname,
                                                    optval,
                                                    optlen,
                                                    pError);
            }
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPGetSockOpt,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &level,
                    &optname,
                    &optval,
                    &optlen,
                    &pError));

    return ReturnValue;
    }


 INT
WSPAPI
WSPIoctl(
    IN SOCKET s,
    IN DWORD dwIoControlCode,
    IN LPVOID lpvInBuffer,
    IN DWORD cbInBuffer,
    IN LPVOID lpvOutBuffer,
    IN DWORD cbOutBuffer,
    IN LPDWORD lpcbBytesReturned,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Control the mode of a socket.

Arguments:

    s                   - Handle to a socket

    dwIoControlCode     - Control code of operation to perform

    lpvInBuffer         - Address of input buffer

    cbInBuffer          - Size of input buffer

    lpvOutBuffer        - Address of output buffer

    cbOutBuffer         - Size of output buffer

    lpcbBytesReturned   - A pointer to the size of output buffer's contents.

    lpOverlapped        - Address of WSAOVERLAPPED structure

    lpCompletionRoutine - A  pointer  to the completion routine called when the
                          operation has been completed.

    lpThreadId          - A  pointer to a thread ID structure to be used by the
                          provider

    pError             - A pointer to the error code.

Return Value:

    If  no error occurs and the operation has completed immediately, WSPIoctl()
    returns  0.   Note  that in this case the completion routine, if specified,
    will  have  already  been  queued.   Otherwise, a value of SOCKET_ERROR is
    returned, and  a  specific  error code is available in pError.  The error
    code  WSA_IO_PENDING  indicates  that  an  overlapped  operation  has  been
    successfully  initiated  and  that  conpletion will be indicated at a later
    time.   Any  other  error  code  indicates that no overlapped operation was
    initiated and no completion indication will occur.

--*/
{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPIoctl,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &dwIoControlCode,
                       &lpvInBuffer,
                       &cbInBuffer,
                       &lpvOutBuffer,
                       &cbOutBuffer,
                       &lpcbBytesReturned,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    if (  (dwIoControlCode == SIO_GET_INTERFACE_LIST)
       || (dwIoControlCode == SIO_ADDRESS_LIST_CHANGE)
       || (dwIoControlCode == SIO_ADDRESS_LIST_QUERY)
       || (dwIoControlCode == SIO_ROUTING_INTERFACE_CHANGE)
       || (dwIoControlCode == SIO_ROUTING_INTERFACE_QUERY) )
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAEACCES;
        }
    else if (pRSocket=RSOCKET::FindListDSocket(s))
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPIoctl( ProviderSocket,
                                           dwIoControlCode,
                                           lpvInBuffer,
                                           cbInBuffer,
                                           lpvOutBuffer,
                                           cbOutBuffer,
                                           lpcbBytesReturned,
                                           lpOverlapped,
                                           lpCompletionRoutine,
                                           lpThreadId,
                                           pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPIoctl,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &dwIoControlCode,
                    &lpvInBuffer,
                    &cbInBuffer,
                    &lpvOutBuffer,
                    &cbOutBuffer,
                    &lpcbBytesReturned,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &pError));

    return ReturnValue;
    }


SOCKET
WSPAPI
WSPJoinLeaf(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN INT namelen,
    IN LPWSABUF lpCallerData,
    IN LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS,
    IN DWORD dwFlags,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Join  a  leaf  node  into  a multipoint session, exchange connect data, and
    specify needed quality of service based on the supplied flow specs.

Arguments:

    s            - A descriptor identifying a multipoint socket.

    name         - The name of the peer to which the socket is to be joined.

    namelen      - The length of the name.

    lpCallerData - A  pointer to the user data that is to be transferred to the
                   peer during multipoint session establishment.

    lpCalleeData - A  pointer  to  the user data that is to be transferred back
                   from the peer during multipoint session establishment.

    lpSQOS       - A  pointer  to  the  flow  specs  for socket s, one for each
                   direction.

    lpGQOS       - A  pointer  to  the  flow  specs  for  the  socket group (if
                   applicable).

    dwFlags      - Flags  to  indicate  that  the socket is acting as a sender,
                   receiver, or both.

    pError      - A pointer to the error code.

Return Value:

    If no error occurs, WSPJoinLeaf() returns a value of type SOCKET which is a
    descriptor  for the newly created multipoint socket.  Otherwise,a value of
    INVALID_SOCKET  is  returned, and  a  specific  error code is available in
    pError.

--*/
    {
    SOCKET       ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPJoinLeaf,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &name,
                       &namelen,
                       &lpCallerData,
                       &lpCalleeData,
                       &lpSQOS,
                       &lpGQOS,
                       &dwFlags,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPJoinLeaf( ProviderSocket,
                                              name,
                                              namelen,
                                              lpCallerData,
                                              lpCalleeData,
                                              lpSQOS,
                                              lpGQOS,
                                              dwFlags,
                                              pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPJoinLeaf,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &name,
                    &namelen,
                    &lpCallerData,
                    &lpCalleeData,
                    &lpSQOS,
                    &lpGQOS,
                    &dwFlags,
                    &pError));

    return ReturnValue;
    }

//---------------------------------------------------------------------------
//  WSPListen()
//
//  Sets a socket to listen for incoming connections.
//
//  If no error occurs, WSPListen() returns 0. Otherwise, a value of
//  SOCKET_ERROR is returned, and a specific error code is returned in
//  pError.
//
//  s       - A descriptor identifying the bound, unconnected socket.
//
//  backlog - The maximum length to which the queue of pending connections may
//            grow. If this value is SOMAXCONN then the service provider
//            should set the backlog to a maximum "reasonable" value.
//
//  pError  - A pointer to the return error code.
//
//---------------------------------------------------------------------------
INT WSPAPI WSPListen( IN SOCKET s,
                      IN INT backlog,
                      OUT INT FAR *pError )
    {
    INT          ReturnValue = SOCKET_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPListen,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &backlog,
                       &pError)) )
        {
        return(ReturnValue);
        }

    #if FALSE
    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPListen( ProviderSocket,
                                            backlog,
                                            pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }
    #else

    *pError = WSAEACCES;

    #endif

    POSTAPINOTIFY(( DTCODE_WSPListen,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &backlog,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPRecv(
    IN SOCKET s,
    IN LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    IN LPDWORD lpNumberOfBytesRecvd,
    IN OUT LPDWORD lpFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Receive data on a socket.

Arguments:

    s                    - A descriptor identifying a connected socket.

    lpBuffers            - A  pointer  to  an array of WSABUF structures.  Each
                           WSABUF  structure contains a pointer to a buffer and
                           the length of the buffer.

    dwBufferCount        - The  number  of  WSABUF  structures in the lpBuffers
                           array.

    lpNumberOfBytesRecvd - A  pointer  to  the number of bytes received by this
                           call.

    lpFlags              - A pointer to flags.

    lpOverlapped         - A pointer to a WSAOVERLAPPED structure.

    lpCompletionRoutine  - A  pointer to the completion routine called when the
                           receive operation has been completed.

    lpThreadId           - A pointer to a thread ID structure to be used by the
                           provider in a subsequent call to WPUQueueApc().

    pError              - A pointer to the error code.

Return Value:

    If  no  error  occurs  and the receive operation has completed immediately,
    WSPRecv() returns the number of bytes received.  If the connection has been
    closed, it  returns  0.  Note that in this case the completion routine, if
    specified,  will   have  already  been  queued.   Otherwise, a  value  of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    pError.   The  error  code WSA_IO_PENDING indicates that the overlapped an
    operation  has  been  successfully  initiated  and  that completion will be
    indicated  at  a  later  time.   Any  other  error  code  indicates that no
    overlapped  operations  was  initiated  and  no  completion indication will
    occur.
--*/
    {
    INT              ReturnValue = NO_ERROR;
    RSOCKET         *pRSocket;
    RPROVIDER       *pProvider;
    SOCKET           ProviderSocket;
    LPWSABUF         InternalBuffers;
    DWORD            InternalBufferCount;


    if (PREAPINOTIFY(( DTCODE_WSPRecv,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesRecvd,
                       &lpFlags,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        // Get Internal buffers to send down to the lower provider.
        ReturnValue = gBufferManager->AllocBuffer( lpBuffers,
                                                   dwBufferCount,
                                                   &InternalBuffers,
                                                   &InternalBufferCount);

        if (ReturnValue == NO_ERROR)
            {
            // Is this a overlapped operation?
            if (lpOverlapped)
                {
                // Setup the user overlapped struct
                lpOverlapped->Internal = WSA_IO_PENDING;
                lpOverlapped->InternalHigh = 0;
                ReturnValue = gWorkerThread->QueueOverlappedRecv(
                                                pProvider,
                                                ProviderSocket,
                                                lpBuffers,
                                                dwBufferCount,
                                                lpNumberOfBytesRecvd,
                                                lpFlags,
                                                lpOverlapped,
                                                lpCompletionRoutine,
                                                lpThreadId,
                                                InternalBuffers,
                                                InternalBufferCount,
                                                pError );
                }
            else
                {
                ReturnValue = pProvider->WSPRecv( ProviderSocket,
                                                  lpBuffers,
                                                  dwBufferCount,
                                                  lpNumberOfBytesRecvd,
                                                  lpFlags,
                                                  lpOverlapped,
                                                  lpCompletionRoutine,
                                                  lpThreadId,
                                                  pError );
                }
            }
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPRecv,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesRecvd,
                    &lpFlags,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &pError));

    return ReturnValue;
    }


 INT
WSPAPI
WSPRecvDisconnect(
    IN SOCKET s,
    IN LPWSABUF lpInboundDisconnectData,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Terminate  reception  on  a socket, and retrieve the disconnect data if the
    socket is connection-oriented.

Arguments:

    s                       - A descriptor identifying a socket.

    lpInboundDisconnectData - A  pointer to a buffer into which disconnect data
                              is to be copied.

    pError                 - A pointer to the error code.

Return Value:

    If  no error occurs, WSPRecvDisconnect() returns NO_ERROR.  Otherwise,
    a value of SOCKET_ERROR is returned, and a specific error code is available
    in pError.

--*/
{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPRecvDisconnect,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpInboundDisconnectData,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPRecvDisconnect( ProviderSocket,
                                                    lpInboundDisconnectData,
                                                    pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPRecvDisconnect,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpInboundDisconnectData,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPRecvFrom(
    IN  SOCKET s,
    IN  LPWSABUF lpBuffers,
    IN  DWORD dwBufferCount,
    IN  LPDWORD lpNumberOfBytesRecvd,
    IN  OUT LPDWORD lpFlags,
    OUT struct sockaddr FAR *  lpFrom,
    IN  LPINT lpFromlen,
    IN  LPWSAOVERLAPPED lpOverlapped,
    IN  LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Receive a datagram and store the source address.

Arguments:

    s                    - A descriptor identifying a socket.

    lpBuffers            - A  pointer  to  an array of WSABUF structures.  Each
                           WSABUF  structure contains a pointer to a buffer and
                           the length of the buffer.

    dwBufferCount        - The  number  of  WSABUF  structures in the lpBuffers
                           array.

    lpNumberOfBytesRecvd - A  pointer  to  the number of bytes received by this
                           call.

    lpFlags              - A pointer to flags.

    lpFrom               - An  optional pointer to a buffer which will hold the
                           source address upon the completion of the overlapped
                           operation.

    lpFromlen            - A  pointer  to the size of the from buffer, required
                           only if lpFrom is specified.

    lpOverlapped         - A pointer to a WSAOVERLAPPED structure.

    CompletionRoutine    - A  pointer to the completion routine called when the
                           receive operation has been completed.

    lpThreadId           - A pointer to a thread ID structure to be used by the
                           provider in a subsequent call to WPUQueueApc().

    pError              - A pointer to the error code.

Return Value:

    If  no  error  occurs  and the receive operation has completed immediately,
    WSPRecvFrom()  returns the number of bytes received.  If the connection has
    been  closed, it returns 0.  Note that in this case the completion routine,
    if  specified  will  have  already  been  queued.   Otherwise,  a  value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    pError.   The  error  code  WSA_IO_PENDING  indicates  that the overlapped
    operation  has  been  successfully  initiated  and  that completion will be
    indicated  at  a  later  time.   Any  other  error  code  indicates that no
    overlapped  operations  was  initiated  and  no  completion indication will
    occur.

--*/
    {
    INT              ReturnValue = NO_ERROR;
    RSOCKET         *pRSocket;
    RPROVIDER       *pProvider;
    SOCKET           ProviderSocket;
    LPWSABUF         InternalBuffers;
    DWORD            InternalBufferCount;


    if (PREAPINOTIFY(( DTCODE_WSPRecvFrom,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesRecvd,
                       &lpFlags,
                       &lpFrom,
                       &lpFromlen,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        // Get Internal buffers to send down to the lower provider.
        ReturnValue = gBufferManager->AllocBuffer( lpBuffers,
                                                   dwBufferCount,
                                                   &InternalBuffers,
                                                   &InternalBufferCount);
        if (ReturnValue == NO_ERROR)
            {
            //Is this a overlapped operation.
            if (lpOverlapped)
                {
                // Setup the user overlapped struct
                lpOverlapped->Internal = WSA_IO_PENDING;
                lpOverlapped->InternalHigh = 0;
                ReturnValue = gWorkerThread->QueueOverlappedRecvFrom(
                                                  pProvider,
                                                  ProviderSocket,
                                                  InternalBuffers,
                                                  InternalBufferCount,
                                                  lpNumberOfBytesRecvd,
                                                  lpFlags,
                                                  lpFrom,
                                                  lpFromlen,
                                                  lpOverlapped,
                                                  lpCompletionRoutine,
                                                  lpThreadId,
                                                  InternalBuffers,
                                                  InternalBufferCount,
                                                  pError );
                }
            else
                {
                ReturnValue = pProvider->WSPRecvFrom( ProviderSocket,
                                                      lpBuffers,
                                                      dwBufferCount,
                                                      lpNumberOfBytesRecvd,
                                                      lpFlags,
                                                      lpFrom,
                                                      lpFromlen,
                                                      lpOverlapped,
                                                      lpCompletionRoutine,
                                                      lpThreadId,
                                                      pError );
                }
            }
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPRecvFrom,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesRecvd,
                    &lpFlags,
                    &lpFrom,
                    &lpFromlen,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &pError));

    return ReturnValue;
    }


typedef struct association
    {
    SOCKET  ProviderSocket;
    SOCKET  UserSocket;
    } SOCKETASSOCIATION, *PSOCKETASSOCIATION;

typedef struct
    {
    UINT                AssociationCount;
    PSOCKETASSOCIATION  Associations;
    } SOCKETMAP, *PSOCKETMAP;



INT TransferUserFdSetToProviderFdSet( IN  fd_set * UserSet,
                                      OUT fd_set * ProviderSet,
                                      OUT PSOCKETMAP SocketMap,
                                      OUT LPINT    Errno)
    {
    INT      ReturnCode;
    UINT     Index;
    RSOCKET *pRSocket;

    ReturnCode = NO_ERROR;
    SocketMap->AssociationCount = 0;
    SocketMap->Associations     = NULL;
    ProviderSet->fd_count       = 0;

    if (UserSet && (UserSet->fd_count > 0))
        {
        if (UserSet->fd_count > FD_SETSIZE)
            {
            *Errno = WSAENOBUFS;
            return SOCKET_ERROR;
            }

        SocketMap->Associations = (PSOCKETASSOCIATION)new BYTE[
            (sizeof(SOCKETASSOCIATION) * UserSet->fd_count)];
        if (SocketMap->Associations)
            {
            for (Index=0;Index < UserSet->fd_count  ;Index++ )
                {
                pRSocket = RSOCKET::FindListDSocket(UserSet->fd_array[Index]);
                if (!pRSocket)
                    {
                    delete SocketMap->Associations;
                    SocketMap->Associations = NULL;
                    *Errno = WSAEINVAL;
                    break;
                    }

                SocketMap->Associations[Index].ProviderSocket
                    = pRSocket->GetSocketHandle();

                SocketMap->Associations[Index].UserSocket
                    = UserSet->fd_array[Index];
                ProviderSet->fd_array[Index]
                    = SocketMap->Associations[Index].ProviderSocket;

                ProviderSet->fd_count++;
                SocketMap->AssociationCount++;
                }
            }
        else
            {
            ReturnCode = SOCKET_ERROR;
            *Errno = WSAENOBUFS;
            }
        }

    return ReturnCode;
    }

INT TransferProviderFdSetToUserFdSet( IN  fd_set *   UserSet,
                                      OUT fd_set *   ProviderSet,
                                      IN  PSOCKETMAP SocketMap,
                                      OUT LPINT      Errno )
    {
    INT  ReturnCode = NO_ERROR;
    UINT ProviderIndex;
    UINT AssociationIndex;

    if (UserSet && (ProviderSet->fd_count > 0))
        {
        UserSet->fd_count = 0;

        for (ProviderIndex = 0;
             ProviderIndex < ProviderSet->fd_count;
             ProviderIndex++)
            {
            for (AssociationIndex =0;
                 AssociationIndex < SocketMap->AssociationCount;
                 AssociationIndex++)
                {
                if (ProviderSet->fd_array[ProviderIndex] ==
                    SocketMap->Associations[AssociationIndex].ProviderSocket)
                    {
                    UserSet->fd_array[ProviderIndex] =
                        SocketMap->Associations[AssociationIndex].UserSocket;
                    UserSet->fd_count++;
                    }
                }
            }

        delete SocketMap->Associations;
        }

    return ReturnCode;
    }

INT
WSPAPI
WSPSelect(
    IN INT nfds,
    IN OUT fd_set FAR *readfds,
    IN OUT fd_set FAR *writefds,
    IN OUT fd_set FAR *exceptfds,
    IN const struct timeval FAR *timeout,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Determine the status of one or more sockets.

Arguments:

    nfds      - This  argument  is  ignored  and  included only for the sake of
                compatibility.

    readfds   - An  optional  pointer  to  a  set  of sockets to be checked for
                readability.

    writefds  - An  optional  pointer  to  a  set  of sockets to be checked for
                writability

    exceptfds - An  optional  pointer  to  a  set  of sockets to be checked for
                errors.

    timeout   - The  maximum  time  for  WSPSelect()  to  wait, or  NULL for a
                blocking operation.

    pError   - A pointer to the error code.

Return Value:

    WSPSelect()  returns  the  total  number of descriptors which are ready and
    contained  in  the  fd_set  structures, 0  if  the  time limit expired, or
    SOCKET_ERROR  if an error occurred.  If the return value is SOCKET_ERROR, a
    specific error code is available in pError.

--*/
    {
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       SocketHandle;
    BOOL         FoundSocket=FALSE;
    fd_set       InternalReadfds;
    fd_set       InternalWritefds;
    fd_set       InternalExceptfds;
    SOCKETMAP    ReadMap;
    SOCKETMAP    WriteMap;
    SOCKETMAP    ExceptMap;

    if (PREAPINOTIFY(( DTCODE_WSPSelect,
                       &ReturnValue,
                       g_szLibraryName,
                       &nfds,
                       &readfds,
                       &writefds,
                       &exceptfds,
                       &timeout,
                       &pError)) )
        {
        return ReturnValue;
        }

    // Look for a socket in the three fd_sets handed in. The first
    // socket found will be used to select the service provider to
    // service this call
    if (readfds && readfds->fd_count)
        {
        SocketHandle = readfds->fd_array[0];
        FoundSocket = TRUE;
        }

    if (!FoundSocket && writefds && writefds->fd_count )
        {
        SocketHandle = writefds->fd_array[0];
        FoundSocket = TRUE;
        }

    if (!FoundSocket && exceptfds && exceptfds->fd_count )
        {
        SocketHandle = exceptfds->fd_array[0];
        FoundSocket = TRUE;
        }

    if (FoundSocket)
        {
        //
        // Get our RSOCKET object
        //
        pRSocket = RSOCKET::FindListDSocket(SocketHandle);
        if (pRSocket)
            {
            pProvider = pRSocket->GetDProvider();

            TransferUserFdSetToProviderFdSet( readfds,
                                              &InternalReadfds,
                                              &ReadMap,
                                              pError );

            TransferUserFdSetToProviderFdSet( writefds,
                                              &InternalWritefds,
                                              &WriteMap,
                                              pError );

            TransferUserFdSetToProviderFdSet( exceptfds,
                                              &InternalExceptfds,
                                              &ExceptMap,
                                              pError );
            if (*pError == NO_ERROR)
                {
                ReturnValue = pProvider->WSPSelect( nfds,
                                                    &InternalReadfds,
                                                    &InternalWritefds,
                                                    &InternalExceptfds,
                                                    timeout,
                                                    pError );

                TransferProviderFdSetToUserFdSet( readfds,
                                                  &InternalReadfds,
                                                  &ReadMap,
                                                  pError );

                TransferProviderFdSetToUserFdSet( writefds,
                                                  &InternalWritefds,
                                                  &WriteMap,
                                                  pError );

                TransferProviderFdSetToUserFdSet( exceptfds,
                                                  &InternalExceptfds,
                                                  &ExceptMap,
                                                  pError );

                DEBUGF( DBG_TRACE, ("Select Returns: %X\n",ReturnValue));

                }
            else
                {
                DEBUGF( DBG_TRACE, ("**Select failed**\n"));
                ReturnValue = SOCKET_ERROR;
                }
            }
        else
            {
            ReturnValue = SOCKET_ERROR;
            *pError = WSAENOTSOCK;
            }
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError    = WSAEINVAL;
        }

    POSTAPINOTIFY(( DTCODE_WSPSelect,
                    &ReturnValue,
                    g_szLibraryName,
                    &nfds,
                    &readfds,
                    &writefds,
                    &exceptfds,
                    &timeout,
                    &pError));

    return ReturnValue;
    }

//---------------------------------------------------------------------------
//  WSPSend()
//
//  Send data on a connected socket.
//
//  If no error occurs and the send operation has completed immediately,
//  WSPSend() returns the number of bytes sent. Note that in this case
//  the completion routine, if specified, will have already been queued.
//  Otherwise, the value SOCKET_ERROR is returned, and a specific error
//  code is available in pError. The error code WSA_IO_PENDING indicates
//  that the overlapped operation has been successfully initiated and that
//  completion will be indicated at a later time. Any other error code
//  indicates that no overlapped operation was initiated and no completion
//  indication will occur.
//
//  s              - A descriptor identifying a connected socket.
//
//  lpBuffers      - A pointer to an array of WSABUF structures. Each
//                   WSABUF structure contains a pointer to a buffer and
//                   the length of the buffer.
//
//  dwBufferCount  - The number of WSABUF structures in the lpBuffers array.
//
//  lpNumberOfBytesSent - A pointer to the number of bytes sent by this call.
//
//  dwFlags        - Flags.
//
//  lpOverlapped   - A pointer to a WSAOVERLAPPED structure.
//
//  lpCompletionRoutine - A pointer to the completion routine called when the
//                        send operation has been completed.
//
//  lpThreadId     - A pointer to a thread ID structure to be used by the
//                   provider in a subsequent call to WPUQueueApc().
//
//  pError         - Return status.
//
//
//---------------------------------------------------------------------------
INT WSPAPI WSPSend( IN SOCKET s,
                    IN LPWSABUF lpBuffers,
                    IN DWORD dwBufferCount,
                    IN LPDWORD lpNumberOfBytesSent,
                    IN DWORD dwFlags,
                    IN LPWSAOVERLAPPED lpOverlapped,
                    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
                    IN LPWSATHREADID lpThreadId,
                    OUT INT FAR *pError )
    {
    INT              ReturnValue = NO_ERROR;
    RSOCKET         *pRSocket;
    RPROVIDER       *pProvider;
    SOCKET           ProviderSocket;
    LPWSABUF         InternalBuffers;
    DWORD            InternalBufferCount;
    DWORD            InternalBytesTransfered;

    if (PREAPINOTIFY(( DTCODE_WSPSend,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesSent,
                       &dwFlags,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object:
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (!pRSocket)
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }
    else
        {
        *pError = NO_ERROR;
        }

    if (ReturnValue != SOCKET_ERROR)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        // Get internal buffers to send down to the lower provider.
        ReturnValue = gBufferManager->AllocBuffer(
                                         lpBuffers,
                                         dwBufferCount,
                                         &InternalBuffers,
                                         &InternalBufferCount);

        if (NO_ERROR == ReturnValue)
            {
            // Copy the supplied buffers:
            ReturnValue = gBufferManager->CopyBuffer(
                                             lpBuffers,
                                             dwBufferCount,
                                             InternalBuffers,
                                             InternalBufferCount,
                                             &InternalBytesTransfered);
            }

        if (NO_ERROR == ReturnValue)
            {
            // Is this a overlapped operation?
            if (lpOverlapped)
                {
                ReturnValue = gWorkerThread->QueueOverlappedSend(
                                                pProvider,
                                                ProviderSocket,
                                                InternalBuffers,
                                                InternalBufferCount,
                                                lpNumberOfBytesSent,
                                                dwFlags,
                                                lpOverlapped,
                                                lpCompletionRoutine,
                                                lpThreadId,
                                                pError);
                }
            else
                {
                ReturnValue = pProvider->WSPSend( ProviderSocket,
                                                  InternalBuffers,
                                                  InternalBufferCount,
                                                  lpNumberOfBytesSent,
                                                  dwFlags,
                                                  lpOverlapped,
                                                  lpCompletionRoutine,
                                                  lpThreadId,
                                                  pError );
                }
            }
        }

    POSTAPINOTIFY(( DTCODE_WSPSend,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesSent,
                    &dwFlags,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &pError));

    return ReturnValue;
    }



 INT
WSPAPI
WSPSendDisconnect(
    IN SOCKET s,
    IN LPWSABUF lpOutboundDisconnectData,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Initiate  termination  of the connection for the socket and send disconnect
    data.

Arguments:

    s                        - A descriptor identifying a socket.

    lpOutboundDisconnectData - A pointer to the outgoing disconnect data.

    pError                  - A pointer to the error code.

Return Value:

    If  no  error occurs, WSPSendDisconnect() returns 0.  Otherwise, a value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    pError.

--*/
{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPSendDisconnect,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpOutboundDisconnectData,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        ReturnValue = pProvider->WSPSendDisconnect( ProviderSocket,
                                                    lpOutboundDisconnectData,
                                                    pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPSendDisconnect,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpOutboundDisconnectData,
                    &pError));

    return ReturnValue;
    }



INT
WSPAPI
WSPSendTo(
    IN SOCKET s,
    IN LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    IN LPDWORD lpNumberOfBytesSent,
    IN DWORD dwFlags,
    IN const struct sockaddr FAR *  lpTo,
    IN INT iTolen,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT INT FAR *pError
    )
/*++
Routine Description:

    Send data to a specific destination using overlapped I/O.

Arguments:

    s                   - A descriptor identifying a socket.

    lpBuffers           - A  pointer  to  an  array of WSABUF structures.  Each
                          WSABUF  structure  contains a pointer to a buffer and
                          the length of the buffer.

    dwBufferCount       - The  number  of  WSABUF  structures  in the lpBuffers
                          array.

    lpNumberOfBytesSent - A pointer to the number of bytes sent by this call.

    dwFlags             - Flags.

    lpTo                - An  optional  pointer  to  the  address of the target
                          socket.

    iTolen              - The size of the address in lpTo.

    lpOverlapped        - A pointer to a WSAOVERLAPPED structure.

    lpCompletionRoutine - A  pointer  to the completion routine called when the
                          send operation has been completed.

    lpThreadId          - A  pointer to a thread ID structure to be used by the
                          provider in a subsequent call to WPUQueueApc().

    pError             - A pointer to the error code.

Return Value:

    If  no  error  occurs  and the receive operation has completed immediately,
    WSPSendTo()  returns  the  number of bytes received.  If the connection has
    been  closed,it returns 0.  Note that in this case the completion routine,
    if  specified, will  have  already  been  queued.   Otherwise, a value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    pError.   The  error  code  WSA_IO_PENDING  indicates  that the overlapped
    operation  has  been  successfully  initiated  and  that completion will be
    indicated  at  a  later  time.   Any  other  error  code  indicates that no
    overlapped operation was initiated and no completion indication will occur.

--*/


{
    INT          ReturnValue = NO_ERROR;
    RSOCKET     *pRSocket;
    RPROVIDER   *pProvider;
    SOCKET       ProviderSocket;
    LPWSABUF     InternalBuffers;
    DWORD        InternalBufferCount;
    DWORD        InternalBytesTransfered;


    if (PREAPINOTIFY(( DTCODE_WSPSendTo,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &lpBuffers,
                       &lpNumberOfBytesSent,
                       &dwFlags,
                       &lpTo,
                       &iTolen,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        // Get Internal buffers to send down to the lower provider.
        ReturnValue = gBufferManager->AllocBuffer( lpBuffers,
                                                   dwBufferCount,
                                                   &InternalBuffers,
                                                   &InternalBufferCount);

        if (ReturnValue == NO_ERROR)
            {
            // Copy the user buffers
            ReturnValue = gBufferManager->CopyBuffer(
                                                lpBuffers,
                                                dwBufferCount,
                                                InternalBuffers,
                                                InternalBufferCount,
                                                &InternalBytesTransfered);
            }

        if (ReturnValue == NO_ERROR)
            {
            //Is this a overlapped operation?
            if (lpOverlapped)
                {
                // Setup the user overlapped struct
                lpOverlapped->Internal = WSA_IO_PENDING;
                lpOverlapped->InternalHigh = 0;
                ReturnValue = gWorkerThread->QueueOverlappedSendTo(
                                                   pProvider,
                                                   ProviderSocket,
                                                   InternalBuffers,
                                                   InternalBufferCount,
                                                   lpNumberOfBytesSent,
                                                   dwFlags,
                                                   lpTo,
                                                   iTolen,
                                                   lpOverlapped,
                                                   lpCompletionRoutine,
                                                   lpThreadId,
                                                   pError );
                }
            else
                {
                ReturnValue = pProvider->WSPSendTo( ProviderSocket,
                                                    lpBuffers,
                                                    dwBufferCount,
                                                    lpNumberOfBytesSent,
                                                    dwFlags,
                                                    lpTo,
                                                    iTolen,
                                                    lpOverlapped,
                                                    lpCompletionRoutine,
                                                    lpThreadId,
                                                    pError);
                }
            }
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPSendTo,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &lpBuffers,
                    &lpNumberOfBytesSent,
                    &dwFlags,
                    &lpTo,
                    &iTolen,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &pError));

    return ReturnValue;
    }

//---------------------------------------------------------------------------
//  WSPSetSockOpt()
//
//  Set an option for the specified socket.
//
//  If no error occurs then WSPSetSockOpt() returns 0. Otherwise, a value of
//  SOCKET_ERROR is returned, and a specific error code is returned in
//  pError.
//
//  s       - A descriptor identifying a socket.
//
//  level   - The level at which the option is defined. The supported levels
//            include SOL_SOCKET. (See annex for more protocol-specific
//            levels).
//
//  optname - The socket option for which the value is to be set.
//
//  optval  - A pointer to the buffer in which the value for the requested
//            option is supplied.
//
//  optlen  - The size of the optval buffer.
//
//  pError  - A pointer to the error code.
//
//---------------------------------------------------------------------------
INT WSPAPI WSPSetSockOpt( IN SOCKET s,
                          IN INT level,
                          IN INT optname,
                          IN const char FAR *optval,
                          IN INT optlen,
                          OUT INT FAR *pError )
    {
    INT          ReturnValue = NO_ERROR;
    PRSOCKET     pRSocket;
    PRPROVIDER   pProvider;
    SOCKET       ProviderSocket;
    HRESULT      hr;
    IRestrictedProcess *pIRestrictedProcess;

    if (PREAPINOTIFY(( DTCODE_WSPSetSockOpt,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &level,
                       &optname,
                       &optval,
                       &optlen,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // If its an allowed option, get our RSOCKET object
    //
    if ( (optname != SO_KEEPALIVE)
       || (optname != SO_LINGER)
       || (optname != TCP_NODELAY) )
        {
        *pError = WSAENOPROTOOPT;
        ReturnValue = SOCKET_ERROR;
        }
    else if (pRSocket=RSOCKET::FindListDSocket(s))
        {
        pIRestrictedProcess = pRSocket->GetIRestrictedProcess();
        if (pIRestrictedProcess)
            {
            // Note that in this case, the Provider Socket is still
            // remote, in the helper process.
            hr = pIRestrictedProcess->RP_SetSockOpt(level,
                                                    optname,
                                                    (UCHAR*)optval,
                                                    optlen,
                                                    pError );
            if (FAILED(hr))
                {
                *pError = WSAEACCES;
                ReturnValue = SOCKET_ERROR;
                }
            else if (*pError)
                {
                ReturnValue = SOCKET_ERROR;
                }
            }
        else
            {
            // The Provider socket is local.
            pProvider = pRSocket->GetDProvider();
            ProviderSocket = pRSocket->GetSocketHandle();

            ReturnValue = pProvider->WSPSetSockOpt( ProviderSocket,
                                                    level,
                                                    optname,
                                                    optval,
                                                    optlen,
                                                    pError );
            }
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPSetSockOpt,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &level,
                    &optname,
                    &optval,
                    &optlen,
                    &pError));

    return ReturnValue;
    }

//---------------------------------------------------------------------------
//  WSPShutdown()
//
//  Disable send and/or receive on a socket.
//
//  If no error occurs, WSPShutdown() returns 0. Otherwise, a value of
//  SOCKET_ERROR is returned, and a specific error code is returned in
//  pError.
//
//  s       - A descriptor identifying a socket.
//
//  how     - A flag that describes what types of operation will no longer be
//            allowed (SD_SEND, SD_RECEIVE or SD_BOTH).
//
//  pError  - A pointer to the error code.
//
//---------------------------------------------------------------------------
INT WSPAPI WSPShutdown( IN SOCKET s,
                        IN INT how,
                        OUT INT FAR *pError )
    {
    INT ReturnValue;
    PRSOCKET     pRSocket;
    PRPROVIDER   pProvider;
    SOCKET       ProviderSocket;


    if (PREAPINOTIFY(( DTCODE_WSPShutdown,
                       &ReturnValue,
                       g_szLibraryName,
                       &s,
                       &how,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get our RSOCKET object
    //
    pRSocket = RSOCKET::FindListDSocket(s);
    if (pRSocket)
        {
        pProvider = pRSocket->GetDProvider();
        ProviderSocket = pRSocket->GetSocketHandle();

        DEBUGF( DBG_TRACE, ("Shutdown socket %X\n",s));

        ReturnValue = pProvider->WSPShutdown( ProviderSocket,
                                              how,
                                              pError );
        }
    else
        {
        ReturnValue = SOCKET_ERROR;
        *pError = WSAENOTSOCK;
        }

    POSTAPINOTIFY(( DTCODE_WSPShutdown,
                    &ReturnValue,
                    g_szLibraryName,
                    &s,
                    &how,
                    &pError));

    return ReturnValue;
    }

//-----------------------------------------------------------------------
//  WSPSocket()
//
//  Create a socket.
//
//  WSPSocket() returns a descriptor for the new socket if successful.
//  Otherwise it returns INVALID_SOCKET with the specific error code
//  returned in *pError.
//
//  af             - Address family.
//
//  type           - Socket type.
//
//  protocol       -
//
//  lpProtocolInfo - Supplies  a pointer to a WSAPROTOCOL_INFOA struct that
//                   defines  the characteristics of the socket to be created.
//
//  g              - Supplies  the identifier of the socket group which the new
//                   socket is to join.
//
//  dwFlags        - Supplies the socket attribute specification.
//
//  pError        - Returns the error code
//
//-----------------------------------------------------------------------
SOCKET WSPAPI WSPSocket( IN int af,
                         IN int type,
                         IN int protocol,
                         IN LPWSAPROTOCOL_INFOW pProtocolInfo,
                         IN GROUP g,
                         IN DWORD dwFlags,
                         OUT INT FAR *pError )
    {
    SOCKET              ReturnValue;
    DWORD               NextProviderCatalogEntryId;
    DWORD               ThisProviderCatalogEntryId;
    PPROTO_CATALOG_ITEM CatalogItem;
    PRPROVIDER          pProvider;
    PRSOCKET            pRSocket;
    SOCKET              ProviderSocket = INVALID_SOCKET;
    HRESULT             hr;
    IRestrictedProcess *pIRestrictedProcess = 0;

    // Debug/Trace code:
    if (PREAPINOTIFY(( DTCODE_WSPSocket,
                       &ReturnValue,
                       g_szLibraryName,
                       &af,
                       &type,
                       &protocol,
                       &pProtocolInfo,
                       &g,
                       &dwFlags,
                       &pError)) )
        {
        return ReturnValue;
        }

    if (RP_IsRestrictedProcess())
       {
        //
        // Dynamically load OLE if necessary:
        //
        if (!RP_InitOle())
           {
           *pError = WSAEACCES;
           return INVALID_SOCKET;
           }

        //
        // The socket is represented by an object in the socket helper
        // process.
        //
        hr = g_pOle->CoCreateInstance( CLSID_RestrictedProcess,
                                       NULL,
                                       CLSCTX_LOCAL_SERVER,
                                       IID_IRestrictedProcess,
                                       (VOID**)&pIRestrictedProcess );
        if (FAILED(hr))
            {
            *pError = WSAEACCES;
            return INVALID_SOCKET;
            }

        //
        // Do a remote call to the helper process to create a socket for
        // us. Note that the socket won't be returned until the call to
        // WSPConnect().
        //
        if (pIRestrictedProcess)
            {
            hr = pIRestrictedProcess->RP_HelperInit();
            if (FAILED(hr))
                {
                pIRestrictedProcess->Release();
                *pError = WSAEACCES;
                return INVALID_SOCKET;
                }

            hr = pIRestrictedProcess
                     ->RP_WSPSocket( af,
                                     type,
                                     protocol,
                                     0,// (RPC_WSAPROTOCOL_INFOW*)pProtocolInfo,
                                     (RPC_GROUP)g,
                                     dwFlags,
                                     (long*)pError );
            if (FAILED(hr))
                {
                pIRestrictedProcess->Release();
                *pError = WSAEACCES;
                return INVALID_SOCKET;
                }

            if (*pError != NO_ERROR)
                {
                pIRestrictedProcess->Release();
                return INVALID_SOCKET;
                }
            }
        else
            {
            // Shouldn't really ever get here...
            *pError = WSAEACCES;
            return INVALID_SOCKET;
            }
        }

    //
    // Get the catlog entry for the next provider in the chain
    //
    ReturnValue = gProviderCatalog
                  ->FindNextProviderInChain( pProtocolInfo,
                                             &ThisProviderCatalogEntryId,
                                             &NextProviderCatalogEntryId );

    if (NO_ERROR == ReturnValue)
        {
        //
        // Get the provider for the catlog entry
        //
        ReturnValue = gProviderCatalog->GetCatalogItemFromCatalogEntryId(
                                       NextProviderCatalogEntryId,
                                       &CatalogItem );
        if (NO_ERROR == ReturnValue)
            {
            //
            // The the next providers WSPSocket
            //
            pProvider = CatalogItem->GetProvider();

            if (!RP_IsRestrictedProcess())
                {
                // If not a restricted process, go ahead and make the
                // socket, by calling down the chain...
                ReturnValue = pProvider->WSPSocket(
                                            af,
                                            type,
                                            protocol,
                                            CatalogItem->GetProtocolInfo(),
                                            g,
                                            dwFlags,
                                            pError );
                if (ReturnValue == INVALID_SOCKET)
                    {
                    return INVALID_SOCKET;
                    }
                else
                    {
                    ProviderSocket = ReturnValue;
                    }
                }

            //
            // Create our socket object
            //
            pRSocket = new RSOCKET;
            if (pRSocket)
                {
                ReturnValue = CreateSocketHandle( pIRestrictedProcess,
                                                  ThisProviderCatalogEntryId,
                                                  (ULONG_PTR)pRSocket,
                                                  pError );

                DEBUGF( DBG_TRACE,
                        ("Socket Returning Socket %X\n", ReturnValue));

                if (ReturnValue != INVALID_SOCKET)
                    {
                    pRSocket->Initialize( pProvider,
                                          ProviderSocket,
                                          ThisProviderCatalogEntryId,
                                          ReturnValue,  // dwContext
                                          pIRestrictedProcess );

                    // Add this socket to the list of sockets.
                    RSOCKET::AddListDSocket( pRSocket );
                    }
                else
                    {
                    delete(pRSocket);
                    // *pError already set...
                    }
                }
            else
                {
                *pError = WSA_NOT_ENOUGH_MEMORY;
                ReturnValue = INVALID_SOCKET;
                }
            }
        else
            {
            *pError = (ULONG)ReturnValue;
            ReturnValue = INVALID_SOCKET;
            }
        }
    else
        {
        *pError = (ULONG)ReturnValue;
        ReturnValue = INVALID_SOCKET;
        }

    // If its a restricted process and we have an error, then we
    // need to release the IRestrictedProcess object.
    if ( (ReturnValue == INVALID_SOCKET)
       && (pIRestrictedProcess) )
        {
        pIRestrictedProcess->Release();
        }

    // Debug/Trace code:
    POSTAPINOTIFY(( DTCODE_WSPSocket,
                    &ReturnValue,
                    g_szLibraryName,
                    &af,
                    &type,
                    &protocol,
                    &lpProtocolInfo,
                    &g,
                    &dwFlags,
                    &pError));

    return ReturnValue;
}


//-----------------------------------------------------------------------
//  WSPStringToAddress()
//
//  WSPStringToAddress() converts a human-readable string to a socket address
//  structure (SOCKADDR) suitable for pass to Windows Sockets routines which
//  takes such a structure. If the caller wishes the translation to be done by
//  a particular provider, it should supply the corresponding WSAPROTOCOL_INFO
//  struct in the lpProtocolInfo parameter.
//
//  The return value is 0 if the operation was successful. Otherwise the value
//  SOCKET_ERROR is returned.
//
//
//  AddressString  - Pointer to the zero-terminated human-readable string
//                   to convert.
//
//  AddressFamily  - The address family to which the string belongs.
//
//  lpProtocolInfo - (optional) the WSAPROTOCOL_INFO struct for a particular
//                   provider. May be NULL.
//
//  Address        - Buffer which is filled with a single SOCKADDR structure.
//
//  lpAddressLength - The length of the Address buffer.  Returns the size of
//                    the resultant SOCKADDR structure.
//
//-----------------------------------------------------------------------
INT WSPAPI WSPStringToAddress( IN     LPWSTR AddressString,
                               IN     INT AddressFamily,
                               IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
                               OUT    LPSOCKADDR lpAddress,
                               IN OUT LPINT lpAddressLength,
                               IN OUT LPINT pError )
    {
    INT                 ReturnValue;
    DWORD               NextProviderCatalogEntryId;
    DWORD               ThisProviderCatalogEntryId;
    PPROTO_CATALOG_ITEM CatalogItem;
    PRPROVIDER          Provider;

    if (PREAPINOTIFY(( DTCODE_WSPAddressToString,
                       &ReturnValue,
                       g_szLibraryName,
                       &AddressString,
                       &AddressFamily,
                       &lpProtocolInfo,
                       &lpAddress,
                       &lpAddressLength,
                       &pError)) )
        {
        return ReturnValue;
        }

    //
    // Get the catlog entry for the next provider in the chain
    //
    ReturnValue = gProviderCatalog->FindNextProviderInChain(
                                       lpProtocolInfo,
                                       &ThisProviderCatalogEntryId,
                                       &NextProviderCatalogEntryId);

    if (NO_ERROR == ReturnValue)
        {
        //
        // Get the provider for the catlog entry
        //
        ReturnValue = gProviderCatalog->GetCatalogItemFromCatalogEntryId(
                                              NextProviderCatalogEntryId,
                                              &CatalogItem );
        if (NO_ERROR == ReturnValue)
            {
            //
            // Get the provider for the catlog entry
            //
            ReturnValue = gProviderCatalog->GetCatalogItemFromCatalogEntryId(
                                              NextProviderCatalogEntryId,
                                              &CatalogItem );
            if (NO_ERROR == ReturnValue)
                {
                //
                // The the next providers WSPSocket
                //
                Provider = CatalogItem->GetProvider();
                ReturnValue = Provider->WSPStringToAddress(
                                             AddressString,
                                             AddressFamily,
                                             lpProtocolInfo,
                                             lpAddress,
                                             lpAddressLength,
                                             pError );
                }
            }
        }

    POSTAPINOTIFY(( DTCODE_WSPAddressToString,
                    &ReturnValue,
                    g_szLibraryName,
                    &AddressString,
                    &AddressFamily,
                    &lpProtocolInfo,
                    &lpAddress,
                    &lpAddressLength,
                    &pError));

    return ReturnValue;
    }



//-----------------------------------------------------------------------
//  WSPStartup()
//
//  Initialize LSP. On success return NO_ERROR. Otherwise return a WSA
//  error code.
//
//  wVersion  - The highest version of WinSock SPI support that the
//              caller  can use. The high order byte specifies the
//              minor version (revision) number; the low-order byte
//              specifies the major version number.
//
//  lpWSPData - A pointer to the WSPDATA data structure that is to receive
//              details of the WinSock service provider.
//
//  lpProtocolInfo - A pointer to a WSAPROTOCOL_INFO struct that defines
//              the characteristics of the desired protocol. This is
//              especially useful when a single provider DLL is capable
//              of instantiating multiple different service providers.
//
//  UpcallTable - The WinSock2 upcall dispatch table.
//
//  lpProcTable - A pointer to the table of SPI function pointers.
//
//-----------------------------------------------------------------------
int WSPAPI WSPStartup( IN  WORD wVersion,
                       OUT LPWSPDATA lpWSPData,
                       IN  LPWSAPROTOCOL_INFOW lpProtocolInfo,
                       IN  WSPUPCALLTABLE UpcallTable,
                       OUT LPWSPPROC_TABLE lpProcTable )
    {
    INT ReturnCode;

    EnterCriticalSection(&g_InitCriticalSection);

    ReturnCode = NO_ERROR;

    if (g_StartupCount == 0)
        {
        ReturnCode = WSASYSNOTREADY;

        // This is the first time that WSPStartup() has been called so lets get
        // ourselves ready to do bussiness

        // Save the WinSock2 upcall table
        g_UpCallTable = UpcallTable;

        // Initialize the socket list
        RSOCKET::InitListDSocket();

        //
        // Init the provider catalog
        //
        gProviderCatalog = new DCATALOG;
        if (gProviderCatalog)
            {
            ReturnCode = gProviderCatalog->Initialize();
            }

        //
        // Init the worker thread
        //
        if (  (NO_ERROR == ReturnCode)
           && (RP_IsRestrictedProcess()) )
            {
            gWorkerThread = new RWORKERTHREAD;
            if (gWorkerThread)
                {
                ReturnCode = gWorkerThread->Initialize();
                }
            }

        //
        // Init the buffer manager
        //
        if (NO_ERROR == ReturnCode)
            {
            gBufferManager = new DBUFFERMANAGER;
            if (gBufferManager)
                {
                ReturnCode = gBufferManager->Initialize();
                }
            }
        }

    // If we succeded incremant the startup count
    if (NO_ERROR == ReturnCode)
        {
        g_StartupCount++;
        }

    LeaveCriticalSection(&g_InitCriticalSection);

    //
    // Fill in the function table with our entry points.
    //

    lpProcTable->lpWSPAccept = WSPAccept;
    lpProcTable->lpWSPAddressToString = WSPAddressToString;
    lpProcTable->lpWSPAsyncSelect = WSPAsyncSelect;
    lpProcTable->lpWSPBind = WSPBind;
    lpProcTable->lpWSPCancelBlockingCall = WSPCancelBlockingCall;
    lpProcTable->lpWSPCleanup = WSPCleanup;
    lpProcTable->lpWSPCloseSocket = WSPCloseSocket;
    lpProcTable->lpWSPConnect = WSPConnect;
    lpProcTable->lpWSPDuplicateSocket = WSPDuplicateSocket;
    lpProcTable->lpWSPEnumNetworkEvents = WSPEnumNetworkEvents;
    lpProcTable->lpWSPEventSelect = WSPEventSelect;
    lpProcTable->lpWSPGetOverlappedResult = WSPGetOverlappedResult;
    lpProcTable->lpWSPGetPeerName = WSPGetPeerName;
    lpProcTable->lpWSPGetSockName = WSPGetSockName;
    lpProcTable->lpWSPGetSockOpt = WSPGetSockOpt;
    lpProcTable->lpWSPGetQOSByName = WSPGetQOSByName;
    lpProcTable->lpWSPIoctl = WSPIoctl;
    lpProcTable->lpWSPJoinLeaf = WSPJoinLeaf;
    lpProcTable->lpWSPListen = WSPListen;
    lpProcTable->lpWSPRecv = WSPRecv;
    lpProcTable->lpWSPRecvDisconnect = WSPRecvDisconnect;
    lpProcTable->lpWSPRecvFrom = WSPRecvFrom;
    lpProcTable->lpWSPSelect = WSPSelect;
    lpProcTable->lpWSPSend = WSPSend;
    lpProcTable->lpWSPSendDisconnect = WSPSendDisconnect;
    lpProcTable->lpWSPSendTo = WSPSendTo;
    lpProcTable->lpWSPSetSockOpt = WSPSetSockOpt;
    lpProcTable->lpWSPShutdown = WSPShutdown;
    lpProcTable->lpWSPSocket = WSPSocket;
    lpProcTable->lpWSPStringToAddress = WSPStringToAddress;

    // Set the version info
    lpWSPData->wVersion = MAKEWORD(2,2);
    lpWSPData->wHighVersion = MAKEWORD(2,2);

    return ReturnCode;
    }
