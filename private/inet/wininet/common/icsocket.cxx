/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    icsocket.cxx

Abstract:

    Contains sockets functions and ICSocket methods

    Contents:
        ContainingICSocket
        MapNetAddressToName
        ICSocket::ICSocket
        ICSocket::~ICSocket
        ICSocket::Destroy
        ICSocket::Reference
        ICSocket::Dereference
        ICSocket::EnableSocks
        ICSocket::Connect
        CFsm_SocketConnect::RunSM
        ICSocket::Connect_Start
        ICSocket::Connect_Continue
        ICSocket::Connect_Error
        ICSocket::Connect_Finish
        ICSocket::SocksConnect
        ICSocket::Disconnect
        ICSocket::Close
        ICSocket::Abort
        ICSocket::Shutdown
        ICSocket::IsReset
        ICSocket::SetTimeout
        ICSocket::SetLinger
        ICSocket::SetNonBlockingMode
        ICSocket::GetBufferLength(SOCKET_BUFFER_ID)
        ICSocket::GetBufferLength(SOCKET_BUFFER_ID, LPDWORD)
        ICSocket::SetBufferLength
        ICSocket::SetSendCoalescing
        SetSourcePort
        ICSocket::Send
        CFsm_SocketSend::RunSM
        ICSocket::Send_Start
        ICSocket::SendTo
        ICSocket::Receive
        CFsm_SocketReceive::RunSM
        ICSocket::Receive_Start
        ICSocket::Receive_Continue
        ICSocket::AllocateQueryBuffer
        //ICSocket::FreeQueryBuffer
        //ICSocket::ReceiveFrom
        ICSocket::DataAvailable
        //ICSocket::DataAvailable2
        ICSocket::WaitForReceive
        //ICSocket::GetBytesAvailable
        ICSocket::CreateSocket
        ICSocket::GetSockName
        ICSocket::Listen
        ICSocket::DirectConnect
        ICSocket::SelectAccept

Author:

    Richard L Firth (rfirth) 08-Apr-1997

Environment:

    Win32 user mode

Revision History:

    08-Apr-1997 rfirth
        Created from ixport.cxx

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

//
// private prototypes
//

//
// functions
//

#if INET_DEBUG

PRIVATE LPSTR MapFamily(int family) {
    switch (family) {
    case AF_UNSPEC:     return "AF_UNSPEC";
    case AF_UNIX:       return "AF_UNIX";
    case AF_INET:       return "AF_INET";
    case AF_IMPLINK:    return "AF_IMPLINK";
    case AF_PUP:        return "AF_PUP";
    case AF_CHAOS:      return "AF_CHAOS";
    case AF_IPX:        return "AF_IPX";
    case AF_OSI:        return "AF_OSI";
    case AF_ECMA:       return "AF_ECMA";
    case AF_DATAKIT:    return "AF_DATAKIT";
    case AF_CCITT:      return "AF_CCITT";
    case AF_SNA:        return "AF_SNA";
    case AF_DECnet:     return "AF_DECnet";
    case AF_DLI:        return "AF_DLI";
    case AF_LAT:        return "AF_LAT";
    case AF_HYLINK:     return "AF_HYLINK";
    case AF_APPLETALK:  return "AF_APPLETALK";
    case AF_NETBIOS:    return "AF_NETBIOS";
#if defined(AF_VOICEVIEW)
    case AF_VOICEVIEW:  return "AF_VOICEVIEW";
#endif /* AF_VOICEVIEW */
#if defined(AF_FIREFOX)
    case AF_FIREFOX:    return "AF_FIREFOX";
#endif /* AF_FIREFOX */
#if defined(AF_UNKNOWN1)
    case AF_UNKNOWN1:   return "AF_UNKNOWN1";
#endif /* AF_UNKNOWN1 */
#if defined(AF_BAN)
    case AF_BAN:        return "AF_BAN";
#endif /* AF_BAN */
    }
    return "?";
}

PRIVATE LPSTR MapSock(int sock) {
    switch (sock) {
    case SOCK_STREAM:       return "SOCK_STREAM";
    case SOCK_DGRAM:        return "SOCK_DGRAM";
    case SOCK_RAW:          return "SOCK_RAW";
    case SOCK_RDM:          return "SOCK_RDM";
    case SOCK_SEQPACKET:    return "SOCK_SEQPACKET";
    }
    return "?";
}

PRIVATE LPSTR MapProto(int proto) {
    switch (proto) {
    case IPPROTO_IP:    return "IPPROTO_IP";
    case IPPROTO_ICMP:  return "IPPROTO_ICMP";
    case IPPROTO_IGMP:  return "IPPROTO_IGMP";
    case IPPROTO_GGP:   return "IPPROTO_GGP";
    case IPPROTO_TCP:   return "IPPROTO_TCP";
    case IPPROTO_PUP:   return "IPPROTO_PUP";
    case IPPROTO_UDP:   return "IPPROTO_UDP";
    case IPPROTO_IDP:   return "IPPROTO_IDP";
    case IPPROTO_ND:    return "IPPROTO_ND";
    }
    return "?";
}

#endif // INET_DEBUG



ICSocket *
ContainingICSocket(
    LPVOID lpAddress
    )

/*++

Routine Description:

    Returns address of start of ICSocket (i.e. vtable) given address of list

Arguments:

    lpAddress   - address of m_List part of ICSocket

Return Value:

    ICSocket *  - address of start of ICSocket object (also ICSecureSocket)

--*/

{
    return CONTAINING_RECORD(lpAddress, ICSocket, m_List);
}

//
// ICSocket methods
//


ICSocket::ICSocket(
    VOID
    )

/*++

Routine Description:

    ICSocket constructor

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "ICSocket::ICSocket",
                 "{%#x}",
                 this
                 ));

    SIGN_ICSOCKET();

    m_List.Flink = NULL;
    m_List.Blink = NULL;
    m_dwTimeout = 0;
    m_Socket = INVALID_SOCKET;
    m_dwFlags = 0;
    m_bAborted = FALSE;
    m_SocksAddress = 0;
    m_SocksPort = 0;
    m_ReferenceCount = 1;

    DEBUG_LEAVE(0);
}


ICSocket::~ICSocket()

/*++

Routine Description:

    ICSocket destructor. Virtual function

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "ICSocket::~ICSocket",
                 "{%#x [sock=%#x, port=%d, ref=%d]}",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 ReferenceCount()
                 ));

    CHECK_ICSOCKET();

    INET_ASSERT(!IsOnList());
    INET_ASSERT(m_ReferenceCount == 0);

    if (IsOpen()) {
        Close();
    }

    DEBUG_LEAVE(0);
}


VOID
ICSocket::Destroy(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "ICSocket::Destroy",
                 "{%#x [%#x/%d]}",
                 this,
                 GetSocket(),
                 GetSourcePort()
                 ));

    INET_ASSERT(ReferenceCount() == 1);

    m_ReferenceCount = 0;
    delete this;

    DEBUG_LEAVE(0);
}


VOID
ICSocket::Reference(
    VOID
    )

/*++

Routine Description:

    Just increases the reference count

Arguments:

    None.

Return Value:

    None.

--*/

{
    CHECK_ICSOCKET();

    InterlockedIncrement(&m_ReferenceCount);
}


BOOL
ICSocket::Dereference(
    VOID
    )

/*++

Routine Description:

    Reduces the reference count. If it goes to zero, the object is deleted

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - object deleted

        FALSE   - object still alive

--*/

{
    CHECK_ICSOCKET();

    if (InterlockedDecrement(&m_ReferenceCount) == 0) {

        INET_ASSERT(m_ReferenceCount == 0);

        delete this;
        return TRUE;
    }
    return FALSE;
}


PRIVATE
DWORD
ICSocket::EnableSocks(
    IN LPSTR lpSocksHost,
    IN INTERNET_PORT ipSocksPort
    )

/*++

Routine Description:

    Set SOCKS gateway IP address and port in this socket object

Arguments:

    lpSocksHost - IP address or host name of SOCKS host

    ipSocksPort - port address of SOCKS host

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_NAME_NOT_RESOLVED
                    failed to resolve SOCKS host name

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::EnableSocks",
                 "{%#x/%d} %q, %d",
                 GetSocket(),
                 GetSourcePort(),
                 lpSocksHost,
                 ipSocksPort
                 ));

    DWORD error = ERROR_SUCCESS;

    m_SocksPort = ipSocksPort;
    m_SocksAddress = _I_inet_addr(lpSocksHost);
    if (m_SocksAddress == INADDR_NONE) {    // 0xffffffff

        LPHOSTENT lpHostent = _I_gethostbyname(lpSocksHost);

        if (lpHostent != NULL) {
            m_SocksAddress = **(LPDWORD*)&lpHostent->h_addr_list[0];
        } else {
            m_SocksAddress = 0;
            error = ERROR_INTERNET_NAME_NOT_RESOLVED;
        }
    }

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("SOCKS address = %d.%d.%d.%d:%d\n",
                ((BYTE*)&m_SocksAddress)[0] & 0xff,
                ((BYTE*)&m_SocksAddress)[1] & 0xff,
                ((BYTE*)&m_SocksAddress)[2] & 0xff,
                ((BYTE*)&m_SocksAddress)[3] & 0xff,
                m_SocksPort
                ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::SocketConnect(
    IN LONG Timeout,
    IN INT Retries,
    IN DWORD dwFlags,
    IN CServerInfo *pServerInfo
    )

/*++

Routine Description:

    Initiate connection with server

Arguments:

    Timeout - maximum amount of time (mSec) to wait for connection

    Retries - maximum number of attempts to connect

    dwFlags - flags controlling request

    pServerInfo - Server Info to connect with

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't create FSM

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::SocketConnect",
                 "{%#x [%#x]} %d, %d, %#x, %x",
                 this,
                 m_Socket,
                 Timeout,
                 Retries,
                 dwFlags,
                 pServerInfo
                 ));


    DWORD error;

    CFsm_SocketConnect * pFsm;

    pFsm = new CFsm_SocketConnect(Timeout, Retries, dwFlags, this);

    if ( pFsm )
    {
        pFsm->SetServerInfo(pServerInfo);
    }

    error = DoFsm(pFsm);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
ICSocket::Connect(
    IN LONG Timeout,
    IN INT Retries,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Initiate connection with server

Arguments:

    Timeout - maximum amount of time (mSec) to wait for connection

    Retries - maximum number of attempts to connect

    dwFlags - flags controlling request

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't create FSM

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Connect",
                 "{%#x [%#x]} %d, %d, %#x",
                 this,
                 m_Socket,
                 Timeout,
                 Retries,
                 dwFlags
                 ));

#ifdef TEST_CODE
    Timeout *= 20;
    Retries *= 20;
#endif

    DWORD error;

    error = DoFsm(new CFsm_SocketConnect(Timeout, Retries, dwFlags, this));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SocketConnect::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    Runs next CFsm_SocketConnect state

Arguments:

    Fsm - FSM controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SocketConnect::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSocket * pSocket = (ICSocket *)Fsm->GetContext();
    CFsm_SocketConnect * stateMachine = (CFsm_SocketConnect *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
        error = pSocket->Connect_Start(stateMachine);
        break;

    case FSM_STATE_CONTINUE:
        error = pSocket->Connect_Continue(stateMachine);
        break;

    case FSM_STATE_ERROR:
        error = pSocket->Connect_Error(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Connect_Start(
    IN CFsm_SocketConnect * Fsm
    )

/*++

Routine Description:

    Starts a socket connect operation - creates a socket and connects it to a
    server using the address information returned by GetServiceAddress(). There
    may be several addresses to try. We return as soon as we successfully
    generate a connection, or after we have tried <Retries> attempts, or until
    <Timeout> milliseconds have elapsed

Arguments:

    Fsm - socket connect FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Connect_Start",
                 "{%#x [%#x]}, %#x(%d, %d, %#x)",
                 this,
                 m_Socket,
                 Fsm,
                 Fsm->m_Timeout,
                 Fsm->m_Retries,
                 Fsm->m_dwFlags
                 ));

    PERF_ENTER(Connect_Start);

    CFsm_SocketConnect & fsm = *Fsm;
    LPINTERNET_THREAD_INFO lpThreadInfo = fsm.GetThreadInfo();

    INET_ASSERT(lpThreadInfo != NULL);

    DWORD error = ERROR_SUCCESS;
    int serr = SOCKET_ERROR;

    INET_ASSERT(IsClosed());

    //
    // ensure the next state is CONTINUE. It may be INIT because we could have
    // been looping through bad addresses (if sufficient timeout & retries)
    //

    fsm.SetNextState(FSM_STATE_CONTINUE);

    //
    // if we are offline then quit now - we can't make any network requests
    //

    if (IsOffline()) {
        error = ERROR_INTERNET_OFFLINE;
        goto quit;
    }

    //
    // get address to use. If exhausted, re-resolve
    //

    if (fsm.GetFunctionState() == FSM_STATE_2) {
        fsm.SetFunctionState(FSM_STATE_1);
        goto resolve_continue;
    }
    if (!fsm.m_pServerInfo->GetNextAddress(&fsm.m_dwResolutionId,
                                           &fsm.m_dwAddressIndex,
                                           GetPort(),
                                           fsm.m_pAddress
                                           )) {
        if (fsm.m_bResolved) {
            error = ERROR_INTERNET_CANNOT_CONNECT;
        } else {
            fsm.SetFunctionState(FSM_STATE_2);
            fsm.SetNextState(FSM_STATE_INIT);
            error = fsm.m_pServerInfo->ResolveHost(&fsm.m_dwResolutionId,
                                                   fsm.m_dwFlags
                                                   );
            if (error == ERROR_IO_PENDING) {
                goto quit;
            }

resolve_continue:

            fsm.m_bResolved = TRUE;
            if (error == ERROR_SUCCESS) {
                if (!fsm.m_pServerInfo->GetNextAddress(&fsm.m_dwResolutionId,
                                                       &fsm.m_dwAddressIndex,
                                                       GetPort(),
                                                       fsm.m_pAddress
                                                       )) {
                    error = ERROR_INTERNET_CANNOT_CONNECT;
                }
            }
            else if (error == ERROR_INTERNET_NAME_NOT_RESOLVED)
            {
                fsm.SetNextState(FSM_STATE_CONTINUE);
                goto quit; // exit out NOW with ERROR_INTERNET_NAME_NOT_RESOLVED, instead of CANNOT_CONNECT
            }
        }
    }
    if (error != ERROR_SUCCESS) {

        //
        // name resolution failed - done
        //

        goto quit;
    }

    //
    // update port for keep-alive info
    //

    SetPort(_I_ntohs(((LPSOCKADDR_IN)fsm.m_pAddress->RemoteAddr.lpSockaddr)->sin_port));

    //
    // BUGBUG - this code was supplying AF_UNSPEC to socket(), which should
    //          be okay, but because of a bug in the Win95 wsipx driver
    //          which manifests itself when we call bind(), we must send in
    //          the address family supplied in the local socket address by
    //          GetAddressByName()
    //

    int protocol;
    DWORD dwConnFlags;

    protocol = fsm.m_pAddress->iProtocol;

#if defined(SITARA)

    //
    // Only enable Sitara if we're connected via modem
    //

//dprintf("connect_start: IsSitara = %B, IsModemConn=%B\n",GlobalEnableSitara, GlobalHasSitaraModemConn);

    if (GlobalEnableSitara && GlobalHasSitaraModemConn) {
        protocol = (int)GetSitaraProtocol();
    }

#endif // SITARA

    m_Socket = _I_socket(fsm.m_pAddress->LocalAddr.lpSockaddr->sa_family,
                         fsm.m_pAddress->iSocketType,
                         protocol
                         );
    if (m_Socket == INVALID_SOCKET) {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("failed to create socket\n"
                    ));

        goto check_socket_error;
    }

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("created socket %#x\n",
                m_Socket
                ));

    //
    // inform the app that we are connecting to the server (but only on the
    // first attempt)
    //

    //if ((fsm.m_dwFlags & SF_INDICATE) && (error == ERROR_SUCCESS)) {
    if (fsm.m_dwFlags & SF_INDICATE) {
        InternetIndicateStatusAddress(INTERNET_STATUS_CONNECTING_TO_SERVER,
                                      fsm.m_pAddress->RemoteAddr.lpSockaddr,
                                      fsm.m_pAddress->RemoteAddr.iSockaddrLength
                                      );
    }

    //
    // if requested to, put the socket in non-blocking mode
    //

    if (fsm.m_dwFlags & SF_NON_BLOCKING
    && (GlobalRunningNovellClient32 ? GlobalNonBlockingClient32 : TRUE)) {
        error = SetNonBlockingMode(TRUE);
        if (error != ERROR_SUCCESS) {
            fsm.SetErrorState(error);
            goto quit;
        }
    }

    //
    // bind the socket to the local address
    //

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("binding to local address %d.%d.%d.%d, port %d, index %d\n",
                ((LPBYTE)fsm.m_pAddress->LocalAddr.lpSockaddr)[4] & 0xff,
                ((LPBYTE)fsm.m_pAddress->LocalAddr.lpSockaddr)[5] & 0xff,
                ((LPBYTE)fsm.m_pAddress->LocalAddr.lpSockaddr)[6] & 0xff,
                ((LPBYTE)fsm.m_pAddress->LocalAddr.lpSockaddr)[7] & 0xff,
                _I_ntohs(((LPSOCKADDR_IN)fsm.m_pAddress->LocalAddr.lpSockaddr)->sin_port),
                fsm.m_dwAddressIndex
                ));

    serr = _I_bind(m_Socket,
                   fsm.m_pAddress->LocalAddr.lpSockaddr,
                   fsm.m_pAddress->LocalAddr.iSockaddrLength
                   );
    if (serr == SOCKET_ERROR) {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("failed to bind socket %#x\n",
                    m_Socket
                    ));

        goto check_socket_error;
    }

    //
    // record source port (useful for matching with net sniff)
    //

    SetSourcePort();

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("socket %#x bound to port %d (%#x)\n",
                m_Socket,
                m_SourcePort,
                m_SourcePort
                ));

    //
    // let another thread know the socket to cancel if it wants to kill
    // this operation
    //

    INET_ASSERT(fsm.GetMappedHandleObject() != NULL);

    if (fsm.GetMappedHandleObject() != NULL) {
        fsm.GetMappedHandleObject()->SetAbortHandle(this);
    }

    //
    // try to connect to the next address
    //

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("connecting %#x/%d to remote address %d.%d.%d.%d, port %d, index %d\n",
                m_Socket,
                m_SourcePort,
                ((LPBYTE)fsm.m_pAddress->RemoteAddr.lpSockaddr)[4] & 0xff,
                ((LPBYTE)fsm.m_pAddress->RemoteAddr.lpSockaddr)[5] & 0xff,
                ((LPBYTE)fsm.m_pAddress->RemoteAddr.lpSockaddr)[6] & 0xff,
                ((LPBYTE)fsm.m_pAddress->RemoteAddr.lpSockaddr)[7] & 0xff,
                _I_ntohs(((LPSOCKADDR_IN)fsm.m_pAddress->RemoteAddr.lpSockaddr)->sin_port),
                fsm.m_dwAddressIndex
                ));

    fsm.SetNextState(FSM_STATE_CONTINUE);
    fsm.StartTimer();

#ifdef TEST_CODE
    SetLastError(-1);
    serr = -1;
#else
    if (IsSocks()) {
        serr = SocksConnect((LPSOCKADDR_IN)fsm.m_pAddress->RemoteAddr.lpSockaddr,
                            fsm.m_pAddress->RemoteAddr.iSockaddrLength
                            );
    } else {
        serr = _I_connect(m_Socket,
                          fsm.m_pAddress->RemoteAddr.lpSockaddr,
                          fsm.m_pAddress->RemoteAddr.iSockaddrLength
                          );
    }
#endif

    //
    // here if a socket operation failed, in which case serr will be SOCKET_ERROR
    //

check_socket_error:

    if (serr == 0) {

        //
        // successful (probably synchronous) connect completion
        //

        //
        // in the sync case, we just call the continue handler. No need to
        // return to the state handler
        //

        Connect_Continue(Fsm);
        goto quit;
    }

    //
    // here if a socket operation failed. We have to read the socket error in
    // this thread before doing anything else or we'll lose the error. We handle
    // it in Connect_Error()
    //

    error = _I_WSAGetLastError();

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("connect(%#x) returns %d\n",
                m_Socket,
                error
                ));

    //
    // if we are using non-blocking sockets then we need to wait until the
    // connect has completed, or an error occurs.
    // If we got any status other than WSAEWOULDBLOCK then we have to handle
    // the error
    //

    if (IsNonBlocking() && (error == WSAEWOULDBLOCK)) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("connect() blocked, socket %#x, port %d\n",
                    m_Socket,
                    m_SourcePort
                    ));

        fsm.SetAction(FSM_ACTION_CONNECT);

        DWORD timeout = GetTimeoutValue(INTERNET_OPTION_CONNECT_TIMEOUT);
        INTERNET_HANDLE_OBJECT * pObject = fsm.GetMappedHandleObject();

        if (pObject != NULL) {
            if (pObject->IsFromCacheTimeoutSet()
            && (pObject->GetObjectType() == TypeHttpRequestHandle)
            && ((HTTP_REQUEST_HANDLE_OBJECT *)pObject)->CanRetrieveFromCache()) {
                timeout = GetTimeoutValue(INTERNET_OPTION_FROM_CACHE_TIMEOUT);

                DWORD connectTime = fsm.m_pServerInfo->GetConnectTime();

                if (connectTime == 0) {
                    connectTime = timeout;
                }
                timeout += connectTime;
            }
        }
        fsm.SetTimeout(timeout);
        fsm.SetNextState(FSM_STATE_CONTINUE);

        //
        // after we set the state to waiting, and get ERROR_IO_PENDING from
        // QueueSocketWorkItem() then we can no longer touch this FSM until
        // it completes asynchronously
        //

        //
        // perf - test the socket. If this completes quickly we don't take a
        // context switch
        //

        //error = WaitForReceive(0);
        //if (error == ERROR_INTERNET_TIMEOUT) {
            error = QueueSocketWorkItem(Fsm, m_Socket);
        //}
        if (error == ERROR_SUCCESS) {

            //
            // in the unlikely event the request completed quickly and
            // successfully
            //

            serr = 0;
            goto check_socket_error;
        } else if (error == ERROR_IO_PENDING) {

            //
            // the request is pending. We already set waiting state
            //

            goto quit;
        }

        //
        // if here then QueueSocketWorkItem() returned some other error
        //

    } else {

        //
        // some other socket error occurred. Convert to INTERNET error
        //

        fsm.SetErrorState(MapInternetError(error));
        error = Connect_Continue(Fsm);
    }

    fsm.SetErrorState(error);

quit:

    //
    // we are done if not pending AND we will not re-enter this state in order
    // to re-do the name resolution/find another address
    //

    if ((error != ERROR_IO_PENDING) && (fsm.GetNextState() != FSM_STATE_INIT)) {
        fsm.SetDone();

        PERF_LEAVE(Connect_Start);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Connect_Continue(
    IN CFsm_SocketConnect * Fsm
    )

/*++

Routine Description:

    Performs common processing after connect completion or failure

Arguments:

    Fsm - reference to socket connect finite state machine

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Connect_Continue",
                 "{%#x [%#x/%d]}, %#x(%d, %d, %#x)",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 Fsm,
                 Fsm->m_Timeout,
                 Fsm->m_Retries,
                 Fsm->m_dwFlags
                 ));

    PERF_ENTER(Connect_Continue);

    CFsm_SocketConnect & fsm = *Fsm;
    fsm.StopTimer();

//    INET_ASSERT((fsm.GetMappedHandleObject() != NULL)
//        ? (fsm.GetMappedHandleObject()->GetAbortHandle() != NULL)
//        : TRUE);

    DWORD error = fsm.GetError();

    //INET_ASSERT(error != SOCKET_ERROR);

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("connect() resumed, socket %#x, port %d\n",
                m_Socket,
                m_SourcePort
                ));

    //
    // check for aborted request
    //

    if (IsAborted()) {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
    } else if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
        error = ERROR_INTERNET_CANNOT_CONNECT;
    }
    if (error == ERROR_SUCCESS) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("socket %#x/port %d connected; time = %d mSec\n",
                    m_Socket,
                    m_SourcePort,
                    fsm.ReadTimer()
                    ));

        error = Connect_Finish(Fsm);
    } else {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("failed to connect socket %#x/port %d: error %s\n",
                    m_Socket,
                    m_SourcePort,
                    InternetMapError(error)
                    ));

        fsm.SetError(error);
        error = Connect_Error(Fsm);
    }

    PERF_LEAVE(Connect_Continue);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Connect_Error(
    IN CFsm_SocketConnect * Fsm
    )

/*++

Routine Description:

    Called to handle a connect error. Either causes the FSM to terminate or
    prepares the FSM to try another connection

Arguments:

    Fsm - socket connect FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Connect_Error",
                 "{%#x [%#x/%d]}, %#x(%d, %d, %#x)",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 Fsm,
                 Fsm->m_Timeout,
                 Fsm->m_Retries,
                 Fsm->m_dwFlags
                 ));

    PERF_ENTER(Connect_Error);

    CFsm_SocketConnect & fsm = *Fsm;

    fsm.StopTimer();

    INTERNET_HANDLE_OBJECT * pObject = fsm.GetMappedHandleObject();

    //
    // no longer performing socket operation - clear abort handle
    //

    INET_ASSERT(pObject != NULL);

    if (pObject != NULL) {
        pObject->ResetAbortHandle();
    }

    DWORD error = fsm.GetError();
    BOOL bRestartable = FALSE;

    //INET_ASSERT(error != SOCKET_ERROR);
    INET_ASSERT(error != ERROR_SUCCESS);

    //
    // check for aborted request - this overrides any socket error
    //

    if (IsAborted() || error == ERROR_INTERNET_OPERATION_CANCELLED) {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
    } else if (fsm.IsCountedOut()
               || fsm.IsTimedOut()  // entire request timeout
               || (error == ERROR_INTERNET_TIMEOUT)) {  // just this request t/o

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("counted out or timed out\n"
                    ));

        //
        // CANNOT_CONNECT takes precedence over TIMEOUT
        //

        if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
            error = ERROR_INTERNET_CANNOT_CONNECT;
        } else if (fsm.IsTimedOut()) {
            error = ERROR_INTERNET_TIMEOUT;
        } else if (fsm.IsCountedOut()) {
            error = ERROR_INTERNET_CANNOT_CONNECT;
        }
    } else if (error != ERROR_INTERNET_OFFLINE) {

        //
        // not aborted, timed-out, counted-out, or offline. We can try again
        //

        bRestartable = TRUE;
    }

    //
    // if the socket is open, close it and try the next connection. Invalidate
    // the address we tried
    //

    if (IsOpen()) {
        Close();
    }

    DWORD mappedError = fsm.GetMappedError();

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("mapped error = %d [%s]\n",
                mappedError,
                InternetMapError(mappedError)
                ));

    //
    // don't invalidate address if from-cache-if-net-fail timeout
    //

    BOOL bInvalidate = TRUE;

    if ((pObject != NULL) && pObject->IsFromCacheTimeoutSet()) {
        bInvalidate = FALSE;
    }
    if ((mappedError == WSAENETUNREACH)
        || (mappedError == WSAETIMEDOUT)
        || ((error == ERROR_INTERNET_TIMEOUT) && bInvalidate)
        || (error == ERROR_INTERNET_CANNOT_CONNECT)
#ifdef TEST_CODE
        || (error == (DWORD)-1)
#endif
        ) {
        fsm.m_pServerInfo->InvalidateAddress(fsm.m_dwResolutionId,
                                             fsm.m_dwAddressIndex
                                             );
    }

    //
    // if the operation was cancelled or we lost connectivity then quit
    //

    if (bRestartable) {
        fsm.SetNextState(FSM_STATE_INIT);
    } else {
        fsm.SetDone(error);

        PERF_LEAVE(Connect_Error);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Connect_Finish(
    IN CFsm_SocketConnect * Fsm
    )

/*++

Routine Description:

    Called when the connection has been successfully established

Arguments:

    Fsm - socket connect FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Connect_Finish",
                 "{%#x [%#x/%d]}, %#x(%d, %d, %#x)",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 Fsm,
                 Fsm->m_Timeout,
                 Fsm->m_Retries,
                 Fsm->m_dwFlags
                 ));

    PERF_ENTER(Connect_Finish);

    CFsm_SocketConnect & fsm = *Fsm;

    INET_ASSERT(IsOpen());

    //
    // store the average connect time to this server in our CServerInfo
    //

    if (fsm.m_pServerInfo != NULL) {
        fsm.m_pServerInfo->UpdateConnectTime(fsm.ReadTimer());
    }
    if (fsm.m_pOriginServer != NULL) {
        fsm.m_pOriginServer->UpdateConnectTime(fsm.ReadTimer());
    }

#ifdef TEST_CODE
    BOOL optval;
    int optlen = sizeof(optval);
    int serr = _I_getsockopt(GetSocket(),
                             IPPROTO_TCP,
                             TCP_NODELAY,
                             (char FAR *)&optval,
                             &optlen
                             );

    if (serr != 0) {
        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("getsockopt(TCP_NODELAY) returns %s (%d)\n",
                    InternetMapError(_I_WSAGetLastError()),
                    _I_WSAGetLastError()
                    ));
    }
#endif

    //
    // no longer performing socket operation - clear abort handle
    //

    INET_ASSERT(fsm.GetMappedHandleObject() != NULL);

    if (fsm.GetMappedHandleObject() != NULL) {
        fsm.GetMappedHandleObject()->ResetAbortHandle();
    }

    //
    // set the send & receive buffer sizes if not -1 (meaning don't change)
    //

    DWORD bufferLength;

    bufferLength = GetBufferLength(ReceiveBuffer);
    if (bufferLength != (DWORD)-1) {
        SetBufferLength(ReceiveBuffer, bufferLength);
    }
    bufferLength = GetBufferLength(SendBuffer);
    if (bufferLength != (DWORD)-1) {
        SetBufferLength(SendBuffer, bufferLength);
    }

    //
    // disable send coalescing
    //

    SetSendCoalescing(FALSE);

    //
    // let the app know we connected to the server successfully
    //

    if (fsm.m_dwFlags & SF_INDICATE) {
        InternetIndicateStatusAddress(INTERNET_STATUS_CONNECTED_TO_SERVER,
                                      fsm.m_pAddress->RemoteAddr.lpSockaddr,
                                      fsm.m_pAddress->RemoteAddr.iSockaddrLength
                                      );
    }

    fsm.SetDone();

    PERF_LEAVE(Connect_Finish);

    DEBUG_LEAVE(ERROR_SUCCESS);

    return ERROR_SUCCESS;
}


int
ICSocket::SocksConnect(
    IN LPSOCKADDR_IN pSockaddr,
    IN INT nLen
    )

/*++

Routine Description:

    Connect to remote host via SOCKS proxy. Modified from original. If we are
    here then we are going specifically via a known SOCKS proxy. There is now
    only one Hosts object, containing a single SOCKD socks proxy address and
    user name

    N.B. Irrespective of whether we are non-blocking, this function executes
    in blocking mode (we expect that we are on an intranet and complete quickly)

Arguments:

    pSockaddr   - address of remote host (on other side of SOCKS firewall)

    nLen        - length of *pSockaddr

Return Value:

    int
        Success - 0

        Failure - -1

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Int,
                 "ICSocket::SocksConnect",
                 "{%#x} %#x, %d",
                 GetSocket(),
                 pSockaddr,
                 nLen
                 ));

    //
    // BUGBUG - should check if the socket type is SOCK_STREAM or if we have
    //          already connected this socket. This code was part of original
    //          general purpose solution. We don't need it
    //

    //
    // initialize sockaddr for connecting to SOCKS firewall
    //

    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = _I_htons(m_SocksPort);
    sin.sin_addr.s_addr = m_SocksAddress;
    memset(&sin.sin_zero, 0, sizeof(sin.sin_zero));

    //
    // initialize SOCKS request packet
    //

    struct {
        unsigned char VN;
        unsigned char CD;
        unsigned short DSTPORT;
        unsigned long  DSTIP;
        char UserId[255];
    } request;

    request.VN = 4;
    request.CD = 1;
    request.DSTPORT = pSockaddr->sin_port;
    request.DSTIP = pSockaddr->sin_addr.s_addr;

    DWORD length = sizeof(request.UserId);

    GlobalUserName.Get(request.UserId, &length);

    length += 8 + 1; // 8 == sizeof fixed portion of request;
                     // +1 for additional '\0'

    //
    // put socket into blocking mode
    //

    BOOL non_blocking = IsNonBlocking();

    if (non_blocking) {
        SetNonBlockingMode(FALSE);
    }

    //
    // communicate with SOCKS firewall: send SOCKS request & receive response
    //

    int serr = _I_connect(m_Socket, (LPSOCKADDR)&sin, sizeof(sin));

    if (serr != SOCKET_ERROR) {
        serr = _I_send(m_Socket, (char *)&request, length, 0);
        if (serr == (int)length) {

            char response[256];


            serr = _I_recv(m_Socket, (char *)response, sizeof(response), 0);
            if( serr == 1 ) {
                // need to read at least 2 bytes
                DEBUG_PRINT(SOCKETS,
                            ERROR,
                            ("need to read one more byte\n"));
                serr = _I_recv(  
                    m_Socket, (char *)(&response[1]), sizeof(response) - 1, 0);
            }

            if (serr != SOCKET_ERROR) {
                if (response[1] != 90) {
                    serr = SOCKET_ERROR;
                }

            } else {

                DEBUG_PRINT(SOCKETS,
                            ERROR,
                            ("recv(%#x) returns %d\n",
                            m_Socket,
                            _I_WSAGetLastError()
                            ));

            }
        } else {

            DEBUG_PRINT(SOCKETS,
                        ERROR,
                        ("send(%#x) returns %d\n",
                        m_Socket,
                        _I_WSAGetLastError()
                        ));

            serr = SOCKET_ERROR;
        }
    } else {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("connect(%#x) returns %d\n",
                    m_Socket,
                    _I_WSAGetLastError()
                    ));

    }

    //
    // if originally non-blocking, make socket non-blocking again
    //

    if (non_blocking) {
        SetNonBlockingMode(TRUE);
    }

    //
    // if success, mark the socket as being connected through firewall
    //

    if (serr == SOCKET_ERROR) {
        _I_WSASetLastError(WSAECONNREFUSED);
    }

    DEBUG_LEAVE(serr);

    return serr;
}


DWORD
ICSocket::Disconnect(
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Undoes the work of ConnectSocket - i.e. closes a connected socket. We make
    callbacks to inform the app that this socket is being closed

Arguments:

    dwFlags - controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Disconnect",
                 "{%#x/%d} %#x",
                 GetSocket(),
                 GetSourcePort(),
                 dwFlags
                 ));

    //
    // let the app know we are closing the connection
    //

    if (dwFlags & SF_INDICATE) {
        InternetIndicateStatus(INTERNET_STATUS_CLOSING_CONNECTION, NULL, 0);
    }

    DWORD error = Close();

    if ((error == ERROR_SUCCESS) && (dwFlags & SF_INDICATE)) {

        //
        // let the app know the connection is closed
        //

        InternetIndicateStatus(INTERNET_STATUS_CONNECTION_CLOSED, NULL, 0);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Close(
    VOID
    )

/*++

Routine Description:

    Closes a connected socket. Assumes that any linger or shutdown etc.
    requirements have already been applied to the socket

Arguments:

    none.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Close",
                 "{%#x/%d}",
                 GetSocket(),
                 GetSourcePort()
                 ));

    DWORD error = ERROR_SUCCESS;

    if (IsOpen()) {
//dprintf("**** closing %#x\n", m_Socket);

        int serr;

        __try {
            serr = _I_closesocket(m_Socket);
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            serr = 0;
        }
        ENDEXCEPT
        error = (serr == SOCKET_ERROR)
            ? MapInternetError(_I_WSAGetLastError())
            : ERROR_SUCCESS;
    }

    //
    // the socket is now closed
    //

    m_Socket = INVALID_SOCKET;

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Abort(
    VOID
    )

/*++

Routine Description:

    Aborts a socket by simply closing it

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Abort",
                 "{%#x/%d}",
                 GetSocket(),
                 GetSourcePort()
                 ));

    DWORD error = Close();

    if (error == ERROR_SUCCESS) {
        SetAborted();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Shutdown(
    IN DWORD dwControl
    )

/*++

Routine Description:

    Stops any more send/receives from the socket

Arguments:

    dwControl   - 0 to stop receives, 1 to stop sends, 2 to stop both

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Shutdown",
                 "{%#x/%d}",
                 GetSocket(),
                 GetSourcePort()
                 ));

    DWORD error = ERROR_SUCCESS;

    if (IsOpen()) {

        int serr = _I_shutdown(m_Socket, dwControl);

        if (serr == SOCKET_ERROR) {

            //
            // map any sockets error to WinInet error
            //

            error = MapInternetError(_I_WSAGetLastError());
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


BOOL
ICSocket::IsReset(
    VOID
    )

/*++

Routine Description:

    Determines if the socket has been closed. We peek the socket for 1 byte. If
    the socket is in blocking mode, we temporarily switch to non-blocking to
    perform the test - we don't want to block, nor remove any data from the
    socket

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - socket reset (closed by server)

        FALSE   - socket alive

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Bool,
                 "ICSocket::IsReset",
                 "{%#x [%#x/%d]}",
                 this,
                 GetSocket(),
                 GetSourcePort()
                 ));

    CHECK_ICSOCKET();

    BOOL bReset = FALSE;
    BOOL bSetBlocking = FALSE;

    if (IsOpen()) {
        if (!IsNonBlocking()) {
            SetNonBlockingMode(TRUE);
            bSetBlocking = TRUE;
        }

        char ch;
#ifndef unix
        int n = _I_recv(m_Socket, &ch, 1, MSG_PEEK);
        if (n < 0) {

            DWORD error = _I_WSAGetLastError();

            if (error != WSAEWOULDBLOCK) {

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("recv() returns %s (%d)\n",
                            InternetMapError(error),
                            error
                            ));

                n = 0;
            }
        }
        if (n == 0) {
#else
        DWORD dwAvail = 0;
        int n = _I_ioctlsocket(m_Socket,FIONREAD,&dwAvail);
        if (n != 0) {
#endif /* unix */
            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("socket %#x/port %d is reset\n",
                        m_Socket,
                        m_SourcePort
                        ));

            bReset = TRUE;
        }
        if (bSetBlocking) {
            SetNonBlockingMode(FALSE);
        }
    } else {
        bReset = TRUE;
    }

    DEBUG_LEAVE(bReset);

    return bReset;
}


DWORD
ICSocket::SetTimeout(
    IN DWORD Type,
    IN int Timeout
    )

/*++

Routine Description:

    Sets a timeout value for a connected socket

Arguments:

    Type            - type of timeout to set - send, or receive

    Timeout         - timeout value to set

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::SetTimeout",
                 "{%#x/%d} %s (%d), %d",
                 GetSocket(),
                 GetSourcePort(),
                 (Type == SEND_TIMEOUT) ? "SEND_TIMEOUT"
                    : (Type == RECEIVE_TIMEOUT) ? "RECEIVE_TIMEOUT"
                    : "?",
                 Type,
                 Timeout
                 ));

    INET_ASSERT((Type == SEND_TIMEOUT) || (Type == RECEIVE_TIMEOUT));

    int serr = _I_setsockopt(m_Socket,
                             SOL_SOCKET,
                             (Type == SEND_TIMEOUT)
                                ? SO_SNDTIMEO
                                : SO_RCVTIMEO,
                             (const char FAR *)&Timeout,
                             sizeof(Timeout)
                             );

    DWORD error = ERROR_SUCCESS;

    if (serr == SOCKET_ERROR) {
        if (IsAborted()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        } else {
            error = MapInternetError(_I_WSAGetLastError());
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::SetLinger(
    IN BOOL Linger,
    IN int Timeout
    )

/*++

Routine Description:

    Sets the linger option for a connected socket

Arguments:

    Linger  - FALSE if the caller wants immediate shutdown of the socket
              when closed, or TRUE if we are to wait around until
              queued data has been sent

    Timeout - timeout value to use if Linger is TRUE

Return Value:

    DWORD
        Success - ERROR_SUCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::SetLinger",
                 "{%#x/%d} %B, %d",
                 GetSocket(),
                 GetSourcePort(),
                 Linger,
                 Timeout
                 ));

    DWORD error = ERROR_SUCCESS;

    if (IsAborted()) {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
    } else if (IsOpen()) {

        LINGER linger;

        INET_ASSERT(Timeout <= USHRT_MAX);

        linger.l_onoff = (u_short)(Linger ? 1 : 0);
        linger.l_linger = (u_short)Timeout;


        //
        // in some shutdown situations, we are hitting exception in winsock
        // on win95 (!). Handle exception
        //

        __try {
            if (_I_setsockopt(m_Socket,
                              SOL_SOCKET,
                              SO_LINGER,
                              (const char FAR *)&linger,
                              sizeof(linger)
                              ) == SOCKET_ERROR) {
                error = MapInternetError(_I_WSAGetLastError());
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {

            //
            // do nothing except catch exception in retail
            //

            DEBUG_PRINT(SOCKETS,
                        ERROR,
                        ("exception closing socket %#x/%d\n",
                        GetSocket(),
                        GetSourcePort()
                        ));

            INET_ASSERT(IsOpen());

        }
        ENDEXCEPT
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::SetNonBlockingMode(
    IN BOOL bNonBlocking
    )

/*++

Routine Description:

    Sets socket non-blocking/blocking mode

Arguments:

    bNonBlocking    - TRUE if non-blocking, FALSE if blocking

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error mapped to INTERNET error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::SetNonBlockingMode",
                 "{%#x/%d} %B",
                 GetSocket(),
                 GetSourcePort(),
                 bNonBlocking
                 ));

    u_long on = (bNonBlocking) ? 1 : 0;
    DWORD error = ERROR_SUCCESS;

    if (_I_ioctlsocket(m_Socket, FIONBIO, &on) == 0) {
        if (on) {
            m_dwFlags |= SF_NON_BLOCKING;
        } else {
            m_dwFlags &= ~SF_NON_BLOCKING;
        }
    } else {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("failed to put socket %#x/port %d into %sblocking mode\n",
                    m_Socket,
                    m_SourcePort,
                    on ? "non-" : ""
                    ));

        if (IsAborted()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        } else {
            error = MapInternetError(_I_WSAGetLastError());
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::GetBufferLength(
    IN SOCKET_BUFFER_ID SocketBufferId
    )

/*++

Routine Description:

    Returns the send or receive buffer length for this socket object

Arguments:

    SocketBufferId  - which buffer length to return

Return Value:

    DWORD

--*/

{
    //
    // BUGBUG - RLF 04/29/96
    //
    // This function should access first the current object, then the parent
    // object, then the globals for this data
    //

    switch (SocketBufferId) {
    case ReceiveBuffer:
        return GlobalSocketReceiveBufferLength;

    case SendBuffer:
        return GlobalSocketSendBufferLength;
    }
    return (DWORD)-1;
}


DWORD
ICSocket::GetBufferLength(
    IN SOCKET_BUFFER_ID SocketBufferId,
    OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Gets the socket send or receive buffer length (if supported)

Arguments:

    SocketBufferId      - which buffer to set

    lpdwBufferLength    - where to write length

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Int,
                 "ICSocket::GetBufferLength",
                 "{%#x/%d} %s, %#x",
                 GetSocket(),
                 GetSourcePort(),
                 (SocketBufferId == ReceiveBuffer)
                    ? "ReceiveBuffer"
                    : ((SocketBufferId == SendBuffer)
                        ? "SendBuffer"
                        : "?")
                 ));

    DWORD size = sizeof(*lpdwBufferLength);

    int serr = _I_getsockopt(m_Socket,
                             SOL_SOCKET,
                             SocketBufferId,
                             (char FAR *)lpdwBufferLength,
                             (int FAR *)&size
                             );

    DWORD error;

    if (serr != SOCKET_ERROR) {
        error = ERROR_SUCCESS;
    } else {
        error = MapInternetError(_I_WSAGetLastError());
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::SetBufferLength(
    IN SOCKET_BUFFER_ID SocketBufferId,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Sets the socket send or receive buffer length

Arguments:

    SocketBufferId  - which buffer to set

    dwBufferLength  - length to set it to

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error mapped to INTERNET error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::SetBufferLength",
                 "{%#x/%d} %s, %d",
                 GetSocket(),
                 GetSourcePort(),
                 (SocketBufferId == ReceiveBuffer)
                    ? "ReceiveBuffer"
                    : (SocketBufferId == SendBuffer)
                        ? "SendBuffer"
                        : "?",
                 dwBufferLength
                 ));

    INET_ASSERT((int)dwBufferLength >= 0);

    DWORD size = sizeof(dwBufferLength);

    int serr = _I_setsockopt(m_Socket,
                             SOL_SOCKET,
                             SocketBufferId,
                             (const char FAR *)&dwBufferLength,
                             (int)size
                             );

    DWORD error = ERROR_SUCCESS;

    if (serr == SOCKET_ERROR) {
        if (IsAborted()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        } else {
            error = MapInternetError(_I_WSAGetLastError());
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::SetSendCoalescing(
    IN BOOL bOnOff
    )

/*++

Routine Description:

    Enables or disables Nagle algorithm

Arguments:

    bOnOff  - FALSE to disable, TRUE to enable

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::SetSendCoalescing",
                 "{%#x/%d} %B",
                 GetSocket(),
                 GetSourcePort(),
                 bOnOff
                 ));

    int optval = bOnOff ? 0 : 1;
    int serr = _I_setsockopt(m_Socket,
                             IPPROTO_TCP,
                             TCP_NODELAY,
                             (const char FAR *)&optval,
                             sizeof(optval)
                             );

    DWORD error = ERROR_SUCCESS;

    if (serr == SOCKET_ERROR) {
        if (IsAborted()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        } else {
            error = MapInternetError(_I_WSAGetLastError());
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


VOID
ICSocket::SetSourcePort(
    VOID
    )

/*++

Routine Description:

    Record the port we are connected to locally. Useful for debugging & matching
    up socket with net sniff

Arguments:

    None.

Return Value:

    None.

--*/

{
    SOCKADDR_IN address;
    int namelen = sizeof(address);

    if (_I_getsockname(GetSocket(), (LPSOCKADDR)&address, &namelen) == 0) {
        m_SourcePort = (INTERNET_PORT)_I_ntohs(address.sin_port);
    } else {
        m_SourcePort = 0;
    }
}


DWORD
ICSocket::Send(
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Sends data over connected socket

Arguments:

    lpBuffer        - pointer to buffer containing data to send

    dwBufferLength  - length of lpBuffer in bytes

    dwFlags         - flags controlling send:

                        SF_INDICATE     - make status callbacks to the app when
                                          we are starting to send data and when
                                          we finish

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't create FSM

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Send",
                 "{%#x [%#x/%d]} %#x, %d, %#x",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 lpBuffer,
                 dwBufferLength,
                 dwFlags
                 ));

    INET_ASSERT(lpBuffer != NULL);
    INET_ASSERT((int)dwBufferLength > 0);

    DWORD error = DoFsm(new CFsm_SocketSend(lpBuffer,
                                            dwBufferLength,
                                            dwFlags,
                                            this
                                            ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SocketSend::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    Runs next CFsm_SocketSend state

Arguments:

    Fsm - socket send FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SocketSend::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSocket * pSocket = (ICSocket *)Fsm->GetContext();
    CFsm_SocketSend * stateMachine = (CFsm_SocketSend *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
    case FSM_STATE_ERROR:
        error = pSocket->Send_Start(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Send_Start(
    IN CFsm_SocketSend * Fsm
    )

/*++

Routine Description:

    Continues send request - sends the data

Arguments:

    Fsm - socket send FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Send_Start",
                 "{%#x [%#x/%d]} %#x(%#x, %d, %#x)",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 Fsm,
                 Fsm->m_lpBuffer,
                 Fsm->m_dwBufferLength,
                 Fsm->m_dwFlags
                 ));

    CFsm_SocketSend & fsm = *Fsm;
    DWORD error = fsm.GetError();
    FSM_STATE state = fsm.GetState();
    INTERNET_HANDLE_OBJECT * pObject = fsm.GetMappedHandleObject();

    if (error != ERROR_SUCCESS) {
        if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
            error = ERROR_INTERNET_CONNECTION_RESET;
        }
        goto quit;
    }
    if (state == FSM_STATE_INIT) {
        if (!(m_dwFlags & (SF_ENCRYPT | SF_DECRYPT))) {

            DEBUG_DUMP_API(SOCKETS,
                           "sending data:\n",
                           fsm.m_lpBuffer,
                           fsm.m_dwBufferLength
                           );

        }

        if (pObject != NULL) {
            pObject->SetAbortHandle(this);
        }

        if (fsm.m_dwFlags & SF_INDICATE) {
            InternetIndicateStatus(INTERNET_STATUS_SENDING_REQUEST, NULL, 0);
        }

        fsm.StartTimer();
    }
    while (fsm.m_dwBufferLength != 0) {

        //
        // if we are offline then quit now - we can't make any network
        // requests
        //

        if (IsOffline()) {
            error = ERROR_INTERNET_OFFLINE;
            break;
        }

        //
        // the socket may have already been aborted
        //

        if (IsAborted()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
            break;
        }

        if (fsm.m_pServerInfo != NULL) {
            fsm.m_pServerInfo->SetLastActiveTime();
        }

        int nSent = _I_send(m_Socket,
                            (char FAR *)fsm.m_lpBuffer,
                            fsm.m_dwBufferLength,
                            0
                            );
        if (nSent != SOCKET_ERROR) {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("sent %d bytes @ %#x to socket %#x/port %d\n",
                        nSent,
                        fsm.m_lpBuffer,
                        m_Socket,
                        m_SourcePort
                        ));

            fsm.m_iTotalSent += nSent;
            fsm.m_lpBuffer = (LPBYTE)fsm.m_lpBuffer + nSent;
            fsm.m_dwBufferLength -= nSent;
        } else {

            //
            // check first to see if the error was due to the socket being
            // closed as a result of the request being cancelled
            //

            if (IsAborted()) {
                error = ERROR_INTERNET_OPERATION_CANCELLED;
                break;
            } else {
                error = _I_WSAGetLastError();
            }
            if (IsNonBlocking() && (error == WSAEWOULDBLOCK)) {

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("send() blocked, socket %#x, port %d\n",
                            m_Socket,
                            m_SourcePort
                            ));
//dprintf("!!! send() blocked - socket %#x\n", m_Socket);
                fsm.SetAction(FSM_ACTION_SEND);

                DWORD timeout = GetTimeoutValue(INTERNET_OPTION_SEND_TIMEOUT);

                if (pObject != NULL) {
                    if (pObject->IsFromCacheTimeoutSet()
                    && (pObject->GetObjectType() == TypeHttpRequestHandle)
                    && ((HTTP_REQUEST_HANDLE_OBJECT *)pObject)->CanRetrieveFromCache()) {
                        timeout = GetTimeoutValue(INTERNET_OPTION_FROM_CACHE_TIMEOUT);

                        DWORD RTT = fsm.m_pOriginServer->GetRTT();

                        if (RTT == 0) {
                            RTT = timeout;
                        }
                        timeout += RTT;
                    }
                }
                fsm.SetTimeout(timeout);
                error = QueueSocketWorkItem(Fsm, m_Socket);
                if (error != ERROR_SUCCESS) {
                    break;
                }
            } else {

                //
                // map any sockets error to WinInet error and terminate this
                // request
                //

                DEBUG_PRINT(SOCKETS,
                            ERROR,
                            ("send() returns %d (%s)\n",
                            error,
                            InternetMapError(error)
                            ));

                error = MapInternetError(error);
                break;
            }
        }
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.StopTimer();
        if (fsm.GetMappedHandleObject() != NULL) {
            fsm.GetMappedHandleObject()->ResetAbortHandle();
        }
        if (IsAborted()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        } else if (error == ERROR_SUCCESS) {
            if (fsm.m_dwFlags & SF_INDICATE) {
                InternetIndicateStatus(INTERNET_STATUS_REQUEST_SENT,
                                       &fsm.m_iTotalSent,
                                       sizeof(fsm.m_iTotalSent)
                                       );
            }
            if (fsm.m_pServerInfo != NULL) {
                //fsm.m_pServerInfo->UpdateSendTime(fsm.ReadTimer());
            }
        }
        fsm.SetDone(error);
    }

    DEBUG_LEAVE(error);

    return error;
}

//
//DWORD
//ICSocket::SendTo(
//    IN LPSOCKADDR lpDestination,
//    IN DWORD dwDestinationLength,
//    IN LPVOID lpBuffer,
//    IN DWORD dwBufferLength,
//    OUT LPDWORD lpdwBytesSent,
//    IN DWORD dwWinsockFlags,
//    IN DWORD dwFlags
//    )
//
///*++
//
//Routine Description:
//
//    Wrapper for sendto()
//
//Arguments:
//
//    lpDestination       - pointer to remote address to send to
//
//    dwDestinationLength - length of *lpDestination
//
//    lpBuffer            - pointer to buffer containing data to send
//
//    dwBufferLength      - number of bytes to send from lpBuffer
//
//    lpdwBytesSent       - number of bytes sent to destination
//
//    dwWinsockFlags      - flags to pass through to sendto()
//
//    dwFlags             - ICSocket flags
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - ERROR_INTERNET_OPERATION_CANCELLED
//                    The operation was cancelled by the caller
//
//                  ERROR_INTERNET_TIMEOUT
//                    The operation timed out
//
//                  ERROR_INTERNET_CONNECTION_RESET
//                    An error occurred. We approximate to connection reset
//
//                  WSA error
//                    Some other sockets error occurred
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SOCKETS,
//                 Dword,
//                 "ICSocket::SendTo",
//                 "{%#x} %#x, %d, %#x, %d, %#x, %#x, %#x",
//                 m_Socket,
//                 lpDestination,
//                 dwDestinationLength,
//                 lpBuffer,
//                 dwBufferLength,
//                 lpdwBytesSent,
//                 dwWinsockFlags,
//                 dwFlags
//                 ));
//
//    INET_ASSERT(IsSocketValid());
//    INET_ASSERT(lpdwBytesSent != NULL);
//
//    int totalSent = 0;
//    DWORD error = ERROR_SUCCESS;
//    INTERNET_HANDLE_OBJECT * pObject = NULL;
//    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
//    BOOL fNonBlocking;
//
//    //
//    // if we are offline then quit now - we can't make any network requests
//    //
//
//    if (IsOffline()) {
//        error = ERROR_INTERNET_OFFLINE;
//        goto quit;
//    }
//
//    if (IsAborted()) {
//        error = ERROR_INTERNET_OPERATION_CANCELLED;
//        goto quit;
//    }
//
//    if (lpThreadInfo == NULL) {
//
//        INET_ASSERT(FALSE);
//
//        error = ERROR_INTERNET_INTERNAL_ERROR;
//        goto quit;
//    }
//
//    fNonBlocking = lpThreadInfo->IsAsyncWorkerThread;
//
//    //
//    // set the cancel socket in the object
//    //
//
//    pObject = (INTERNET_HANDLE_OBJECT * )lpThreadInfo->hObjectMapped;
//    if (pObject != NULL) {
//        pObject->SetAbortHandle(this);
//    }
//
//    //
//    // if we are in async (== non-blocking) mode, let the async request
//    // scheduler know what operation we will be waiting on
//    //
//
//    if (fNonBlocking) {
//
//        INET_ASSERT(lpThreadInfo->lpArb != NULL);
//
//        SET_ARB_SOCKET_OPERATION(lpThreadInfo->lpArb, m_Socket, SEND);
//    }
//
//    if (dwFlags & SF_INDICATE) {
//
//        //
//        // let the app know we are starting to send data
//        //
//
//        InternetIndicateStatus(INTERNET_STATUS_SENDING_REQUEST,
//                               NULL,
//                               0
//                               );
//    }
//
//    DEBUG_DUMP(SOCKETS,
//               "sending data:\n",
//               lpBuffer,
//               dwBufferLength
//               );
//
//    int nSent;
//
//    //
//    // loop until all data sent
//    //
//
//    do {
//
//        nSent = _I_sendto(m_Socket,
//                          (char FAR *)lpBuffer + totalSent,
//                          dwBufferLength,
//                          dwWinsockFlags,
//                          lpDestination,
//                          dwDestinationLength
//                          );
//        if (nSent != SOCKET_ERROR) {
//
//            DEBUG_PRINT(SOCKETS,
//                        INFO,
//                        ("sent %d bytes @ %#x on socket %#x\n",
//                        nSent,
//                        (LPBYTE)lpBuffer + totalSent,
//                        m_Socket
//                        ));
//
//            INET_ASSERT(nSent > 0);
//
//            totalSent += nSent;
//            dwBufferLength -= nSent;
//        } else {
//            error = _I_WSAGetLastError();
//            if ((error == WSAEWOULDBLOCK) && fNonBlocking) {
//
//                INET_ASSERT(_dwFlags & SF_NON_BLOCKING);
//
//                DEBUG_PRINT(SOCKETS,
//                            INFO,
//                            ("sendto(%#x) would block\n",
//                            m_Socket
//                            ));
//
//                lpThreadInfo->lpArb->Header.dwResultCode = ERROR_SUCCESS;
//
//                SwitchToAsyncScheduler(m_Socket);
//
//                error = lpThreadInfo->lpArb->Header.dwResultCode;
//
//                DEBUG_PRINT(SOCKETS,
//                            INFO,
//                            ("sendto(%#x) resumed, returns %s\n",
//                            m_Socket,
//                            InternetMapError(error)
//                            ));
//
//                if (error != ERROR_SUCCESS) {
//                    if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
//                        error = ERROR_INTERNET_CONNECTION_RESET;
//                    }
//                }
//            } else {
//
//                //
//                // some other error
//                //
//
//                error = MapInternetError(error);
//            }
//        }
//
//        INET_ASSERT((int)dwBufferLength >= 0);
//
//    } while ((dwBufferLength != 0) && (error == ERROR_SUCCESS));
//
//    if ((dwFlags & SF_INDICATE) && (error == ERROR_SUCCESS)) {
//
//        //
//        // let the app know we have finished sending
//        //
//
//        InternetIndicateStatus(INTERNET_STATUS_REQUEST_SENT,
//                               &totalSent,
//                               sizeof(totalSent)
//                               );
//    }
//
//    //
//    // if we are in async (== non-blocking) mode, let the async request
//    // scheduler know that we no longer require this socket
//    //
//
//    if (fNonBlocking) {
//
//        INET_ASSERT(lpThreadInfo->lpArb != NULL);
//
//        SET_ARB_SOCKET_OPERATION(lpThreadInfo->lpArb, INVALID_SOCKET, SEND);
//    }
//
//quit:
//
//    *lpdwBytesSent = totalSent;
//
//    //
//    // no longer performing operation on this socket
//    //
//
//    if (pObject != NULL) {
//        pObject->ResetAbortHandle();
//
//        //
//        // if the operation has been cancelled, then this error overrides any
//        // other
//        //
//
//        //if (pObject->IsInvalidated()) {
//        //    error = pObject->GetError();
//        //    if (error == ERROR_SUCCESS) {
//        //        error = ERROR_INTERNET_OPERATION_CANCELLED;
//        //    }
//        //}
//        if (IsAborted()) {
//            error = ERROR_INTERNET_OPERATION_CANCELLED;
//        }
//    }
//
//    INET_ASSERT((pObject != NULL) ? (pObject->GetAbortHandle() == NULL) : TRUE);
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}


DWORD
ICSocket::Receive(
    IN OUT LPVOID * lplpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwBufferRemaining,
    IN OUT LPDWORD lpdwBytesReceived,
    IN DWORD dwExtraSpace,
    IN DWORD dwFlags,
    OUT LPBOOL lpbEof
    )

/*++

Routine Description:

    Receives data from connected socket. Depending on flags settings, we will
    perform a single receive, loop until we have filled the buffer and/or loop
    until we have received all the data.

    This function returns user data, so if the stream we are receiving from is
    encrypted, we must decrypt the data before returning. This may require
    receiving more data than the user expects because we have to decrypt at
    message boundaries

    This function is intended to be called in a loop. The buffer pointer and
    buffer sizes are intended to be updated by each successive call to this
    function, and should therefore have the same values the next time this
    function is called

Arguments:

    lplpBuffer          - pointer to pointer to users buffer. If supplied, the
                          buffer should be LMEM_FIXED

    lpdwBufferLength    - size of buffer

    lpdwBufferRemaining - number of bytes left in the buffer

    lpdwBytesReceived   - number of bytes received

    dwExtraSpace        - number of additional bytes caller wants at end of
                          buffer (only useful if resizing AND only applied at
                          end of receive)

    dwFlags             - flags controlling receive:

                            SF_EXPAND       - lpBuffer can be expanded to fit
                                              data

                            SF_COMPRESS     - if set, we will shrink the buffer
                                              to compress out any unused space

                            SF_RECEIVE_ALL  - if set, this function will loop
                                              until all data received, or the
                                              supplied buffer is filled

                            SF_INDICATE     - if set, we will make status
                                              callbacks to the app when we are
                                              starting to receive data, and when
                                              we finish

                            SF_WAIT         - (used with SF_NON_BLOCKING). Even
                                              though the socket is non-blocking,
                                              the caller wants us to not
                                              relinquish control under the
                                              request has been satisfied

    lpbEof              - TRUE if we got end-of-connection indication
                          (recv() returns 0)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't allocate/grow buffer

                  ERROR_INSUFFICIENT_BUFFER
                    The initial buffer was insufficient (i.e. caller supplied
                    buffer pointer was NULL, or we ran out of buffer space and
                    are not allowed to resize it)

                  WSA error
                    Sockets error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Receive",
                 "%#x [%#x], %#x [%d], %#x [%d], %#x [%d], %d, %#x, %#x [%B]",
                 lplpBuffer,
                 *lplpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 lpdwBufferRemaining,
                 *lpdwBufferRemaining,
                 lpdwBytesReceived,
                 *lpdwBytesReceived,
                 dwExtraSpace,
                 dwFlags,
                 lpbEof,
                 *lpbEof
                 ));

    INET_ASSERT((int)*lpdwBufferLength >= 0);
    INET_ASSERT((int)*lpdwBufferRemaining >= 0);
    INET_ASSERT((int)*lpdwBytesReceived >= 0);

#define SF_MUTEX_FLAGS  (SF_RECEIVE_ALL | SF_NO_WAIT)

    INET_ASSERT((dwFlags & SF_MUTEX_FLAGS) != SF_MUTEX_FLAGS);

    DWORD error = DoFsm(new CFsm_SocketReceive(lplpBuffer,
                                               lpdwBufferLength,
                                               lpdwBufferRemaining,
                                               lpdwBytesReceived,
                                               dwExtraSpace,
                                               dwFlags,
                                               lpbEof,
                                               this
                                               ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_SocketReceive::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    Runs next CFsm_SocketReceive state

Arguments:

    Fsm - socket receive FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "CFsm_SocketReceive::RunSM",
                 "%#x",
                 Fsm
                 ));

    ICSocket * pSocket = (ICSocket *)Fsm->GetContext();
    CFsm_SocketReceive * stateMachine = (CFsm_SocketReceive *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
        error = pSocket->Receive_Start(stateMachine);
        break;

    case FSM_STATE_CONTINUE:
    case FSM_STATE_ERROR:
        error = pSocket->Receive_Continue(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Receive_Start(
    IN CFsm_SocketReceive * Fsm
    )

/*++

Routine Description:

    Initiates a receive request - grows the buffer if required and kicks off the
    first receive operation

Arguments:

    Fsm - reference to FSM controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Receive_Start",
                 "{%#x [%#x/%d]} %#x(%#x [%#x], %#x [%d], %#x [%d], %#x [%d], %d, %#x, %#x [%B])",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 Fsm,
                 Fsm->m_lplpBuffer,
                 *Fsm->m_lplpBuffer,
                 Fsm->m_lpdwBufferLength,
                 *Fsm->m_lpdwBufferLength,
                 Fsm->m_lpdwBufferRemaining,
                 *Fsm->m_lpdwBufferRemaining,
                 Fsm->m_lpdwBytesReceived,
                 *Fsm->m_lpdwBytesReceived,
                 Fsm->m_dwExtraSpace,
                 Fsm->m_dwFlags,
                 Fsm->m_lpbEof,
                 *Fsm->m_lpbEof
                 ));

    CFsm_SocketReceive & fsm = *Fsm;
    DWORD error = ERROR_SUCCESS;

    //
    // if we weren't given a buffer, but the caller told us its okay to resize
    // then we allocate the initial buffer
    //

    if ((fsm.m_dwBufferLength == 0) || (fsm.m_dwBufferLeft == 0)) {

        INET_ASSERT((fsm.m_dwBufferLength == 0) ? (fsm.m_dwBufferLeft == 0) : TRUE);

        if (fsm.m_dwFlags & SF_EXPAND) {

            //
            // allocate a fixed memory buffer
            //

            //
            // BUGBUG - the initial buffer size should come from the handle
            //          object
            //

            fsm.m_dwBufferLeft = DEFAULT_RECEIVE_BUFFER_INCREMENT;
            if (fsm.m_dwBufferLength == 0) {
                fsm.m_bAllocated = TRUE;
            }
            fsm.m_dwBufferLength += fsm.m_dwBufferLeft;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("resizing %#x to %d\n",
                        fsm.m_hBuffer,
                        fsm.m_dwBufferLength
                        ));

            fsm.m_hBuffer = ResizeBuffer(fsm.m_hBuffer, fsm.m_dwBufferLength, FALSE);
            if (fsm.m_hBuffer == (HLOCAL)NULL) {
                error = GetLastError();

                INET_ASSERT(error != ERROR_SUCCESS);

                fsm.m_bAllocated = FALSE;
            }
        } else {

            //
            // the caller didn't say its okay to resize
            //

            error = ERROR_INSUFFICIENT_BUFFER;
        }
    } else if (fsm.m_hBuffer == (HLOCAL)NULL) {
        error = ERROR_INSUFFICIENT_BUFFER;
    }
    if (error == ERROR_SUCCESS) {
        if (fsm.GetMappedHandleObject() != NULL) {
            fsm.GetMappedHandleObject()->SetAbortHandle(this);
        }

        //
        // keep the app informed (if requested to do so)
        //

        if (fsm.m_dwFlags & SF_INDICATE) {
            InternetIndicateStatus(INTERNET_STATUS_RECEIVING_RESPONSE, NULL, 0);
        }

        //
        // kick off the receive request. If we complete synchronously (with
        // an error or successfully), then call the finish handler here
        //

        error = Receive_Continue(Fsm);
    } else {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::Receive_Continue(
    IN CFsm_SocketReceive * Fsm
    )

/*++

Routine Description:

    Receives data from connected socket. Depending on flags settings, we will
    perform a single receive, loop until we have filled the buffer and/or loop
    until we have received all the data.

Arguments:

    Fsm - reference to FSM controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::Receive_Continue",
                 "{%#x [%#x/%d]} %#x(%#x [%#x], %#x [%d], %#x [%d], %#x [%d], %d, %#x, %#x [%B])",
                 this,
                 GetSocket(),
                 GetSourcePort(),
                 Fsm,
                 Fsm->m_lplpBuffer,
                 *Fsm->m_lplpBuffer,
                 Fsm->m_lpdwBufferLength,
                 *Fsm->m_lpdwBufferLength,
                 Fsm->m_lpdwBufferRemaining,
                 *Fsm->m_lpdwBufferRemaining,
                 Fsm->m_lpdwBytesReceived,
                 *Fsm->m_lpdwBytesReceived,
                 Fsm->m_dwExtraSpace,
                 Fsm->m_dwFlags,
                 Fsm->m_lpbEof,
                 *Fsm->m_lpbEof
                 ));

    CFsm_SocketReceive & fsm = *Fsm;
    DWORD error = fsm.GetError();
    INTERNET_HANDLE_OBJECT * pObject = fsm.GetMappedHandleObject();

    if (error != ERROR_SUCCESS) {
        goto error_exit;
    }

    fsm.m_lpBuffer = (LPBYTE)fsm.m_hBuffer + fsm.m_dwBytesReceived;

    //
    // receive some data
    //

    do {
        if (fsm.m_pServerInfo != NULL) {
            fsm.m_pServerInfo->SetLastActiveTime();
        }

        INET_ASSERT((int)fsm.m_dwBufferLeft > 0);

        int nRead = _I_recv(m_Socket,
                            (char FAR *)fsm.m_lpBuffer,
                            (int)fsm.m_dwBufferLeft,
                            0
                            );

        //
        // hackorama # 95, subparagraph 13
        //
        // RLF 07/15/96
        //
        // On Win95 (wouldn't you know it?) in low-memory conditions, we can get
        // into a situation where one or more pages of our receive buffer is
        // filled with zeroes.
        //
        // The reason this happens is that the winsock VxD creates an alias to
        // our buffer, locks the buffer & writes into it, then marks the alias
        // dirty, but not the original buffer. If the buffer is paged out then
        // back in, one or more pages are zeroed because the O/S didn't know
        // they had been written to; it decides to initialize the pages with
        // zeroes.
        //
        // We try to circumvent this by immediately probing each page (we read
        // a byte then write it back).
        //
        // This doesn't fix the problem, just makes the window a lot smaller.
        // However, apart from writing a device driver or modifying the VxD,
        // there's not much else we can do
        //

        ProbeWriteBuffer(fsm.m_lpBuffer, fsm.m_dwBufferLeft);

        if (nRead == 0) {

            //
            // done
            //

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("EOF connection %#x/port %d\n",
                        m_Socket,
                        m_SourcePort
                        ));

            fsm.m_bEof = TRUE;
            break;
        } else if (nRead > 0) {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("received %d bytes from socket %#x/port %d\n",
                        nRead,
                        m_Socket,
                        m_SourcePort
                        ));

            fsm.m_dwBytesReceived += nRead;
            fsm.m_dwBytesRead += nRead;
            fsm.m_lpBuffer += nRead;
            fsm.m_dwBufferLeft -= nRead;

            //
            // if SF_RECEIVE_ALL is not set then the caller just wants us to
            // perform a single receive. We're done
            //

            if (!(fsm.m_dwFlags & SF_RECEIVE_ALL) ) {
                break;
            }

            //
            // if we've filled the current buffer, then either we're done, or
            // the caller wants us to receive the entire response, in which
            // case we attempt to grow the buffer and receive the next part
            // of the message. Note that we may have already received the
            // entire response if it just happened to be the same size as our
            // buffer
            //

            // BUGBUG [arthurbi] we're broken for SSL/PCT case !!!
            //  We need to handle expanding the buffer.
            //

            if (fsm.m_dwBufferLeft == 0) {

                //
                // BUGBUG - RLF - why are we testing for SF_DECRYPT here?
                //

                if (!(fsm.m_dwFlags & SF_EXPAND) || (m_dwFlags & SF_DECRYPT)) {
                    break;
                } else {

                    //
                    // BUGBUG - the buffer increment should come from the handle
                    //          object
                    //

                    fsm.m_dwBufferLeft = DEFAULT_RECEIVE_BUFFER_INCREMENT;
                    fsm.m_dwBufferLength += DEFAULT_RECEIVE_BUFFER_INCREMENT;

                    DEBUG_PRINT(SOCKETS,
                                INFO,
                                ("resizing %#x to %d\n",
                                fsm.m_hBuffer,
                                fsm.m_dwBufferLength
                                ));

                    fsm.m_hBuffer = ResizeBuffer(fsm.m_hBuffer,
                                                 fsm.m_dwBufferLength,
                                                 FALSE
                                                 );
                    if (fsm.m_hBuffer != NULL) {
                        fsm.m_lpBuffer = (LPBYTE)fsm.m_hBuffer + fsm.m_dwBytesReceived;
                    } else {
                        error = GetLastError();

                        INET_ASSERT(error != ERROR_SUCCESS);

                        fsm.m_dwBytesReceived = 0;
                        fsm.m_dwBufferLength = 0;
                        fsm.m_dwBufferLeft = 0;
                    }
                }
            }
        } else {
            error = _I_WSAGetLastError();
            if ((error != WSAEWOULDBLOCK) || (fsm.m_dwFlags & SF_NO_WAIT)) {

                //
                // a real error occurred. We need to get out
                //

                DEBUG_PRINT(SOCKETS,
                            ERROR,
                            ("recv() on socket %#x/port %d returns %d\n",
                            m_Socket,
                            m_SourcePort,
                            error
                            ));

                if (!(fsm.m_dwFlags & SF_NO_WAIT)) {
                    error = MapInternetError(error);
                }
                break;
            }

            //
            // socket would block. If SF_NON_BLOCKING is set then the caller is
            // expecting that we complete asynchronously. If SF_WAIT is set then
            // the caller wants to force synchronous behaviour, so we wait here
            // until the socket unblocks. If neither is set then the caller just
            // wants us to return what we have
            //

            if (IsNonBlocking()) {

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("recv() blocked, socket %#x/port %d\n",
                            m_Socket,
                            m_SourcePort
                            ));

                fsm.SetAction(FSM_ACTION_RECEIVE);

                DWORD timeout = GetTimeoutValue(INTERNET_OPTION_RECEIVE_TIMEOUT);

                if (pObject != NULL) {
                    if (pObject->IsFromCacheTimeoutSet()
                    && (pObject->GetObjectType() == TypeHttpRequestHandle)
                    && ((HTTP_REQUEST_HANDLE_OBJECT *)pObject)->CanRetrieveFromCache()) {
                        timeout = GetTimeoutValue(INTERNET_OPTION_FROM_CACHE_TIMEOUT);

                        DWORD RTT = fsm.m_pOriginServer->GetRTT();

                        if (RTT == 0) {
                            RTT = timeout;
                        }
                        timeout += RTT;
                    }
                }
                fsm.SetTimeout(timeout);
                error = QueueSocketWorkItem(Fsm, m_Socket);
                if (error != ERROR_SUCCESS) {
                    break;
                }

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("recv() resumed, socket %#x/port %d, returns %s\n",
                            m_Socket,
                            m_SourcePort,
                            InternetMapError(error)
                            ));

            } else if (fsm.m_dwFlags & SF_WAIT) {

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("waiting for socket %#x/port %d to become unblocked\n",
                            m_Socket,
                            m_SourcePort
                            ));

                //error = WaitForReceive(INFINITE);
            } else {

                //
                // return what we have (non-blocking, non-waiting read). But we
                // *should* have read *something*
                //

                if (fsm.m_dwBytesRead == 0) {

                    DEBUG_PRINT(SOCKETS,
                                ERROR,
                                ("bogus - 0 bytes read from non-blocking recv(). Waiting\n"
                                ));

                    //
                    // AOL problem:
                    //

                    //error = WaitForReceive(INFINITE);
                } else {
                    error = ERROR_SUCCESS;
                    break;
                }
            }
        }
    } while (error == ERROR_SUCCESS);

error_exit:

    //
    // get correct error based on settings
    //

    if (error == ERROR_IO_PENDING) {
        goto done;
    } else if (IsAborted()) {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
    } else if (IsOffline()) {
        error = ERROR_INTERNET_OFFLINE;
    } else if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
        error = ERROR_INTERNET_CONNECTION_RESET;
    }

    if (pObject != NULL) {
        pObject->ResetAbortHandle();
    }

    if (error == ERROR_SUCCESS) {

        //
        // inform the app that we finished, and tell it how much we received
        // this time
        //

        if (fsm.m_dwFlags & SF_INDICATE) {
            InternetIndicateStatus(INTERNET_STATUS_RESPONSE_RECEIVED,
                                   &fsm.m_dwBytesRead,
                                   sizeof(fsm.m_dwBytesRead)
                                   );
        }

        //
        // if we received the entire response and the caller specified
        // SF_COMPRESS then we shrink the buffer to fit. We may end up growing
        // the buffer to contain dwExtraSpace if it is not zero and we just
        // happened to fill the current buffer
        //

        if (fsm.m_bEof && (fsm.m_dwFlags & SF_COMPRESS)) {

            fsm.m_dwBufferLeft = fsm.m_dwExtraSpace;

            //
            // include any extra that the caller required
            //

            fsm.m_dwBufferLength = fsm.m_dwBytesReceived + fsm.m_dwExtraSpace;

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("shrinking buffer %#x to %d (%#x) bytes (includes %d extra)\n",
                        fsm.m_hBuffer,
                        fsm.m_dwBufferLength,
                        fsm.m_dwBufferLength,
                        fsm.m_dwExtraSpace
                        ));

            fsm.m_hBuffer = ResizeBuffer(fsm.m_hBuffer,
                                         fsm.m_dwBufferLength,
                                         FALSE
                                         );

            INET_ASSERT((fsm.m_hBuffer == NULL)
                        ? ((fsm.m_dwBytesReceived + fsm.m_dwExtraSpace) == 0)
                        : TRUE
                        );

        }

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("read %d bytes @ %#x from socket %#x/port %d\n",
                    fsm.m_dwBytesRead,
                    (LPBYTE)fsm.m_hBuffer + *fsm.m_lpdwBytesReceived,
                    m_Socket,
                    m_SourcePort
                    ));

        DEBUG_DUMP_API(SOCKETS,
                       "received data:\n",
                       (LPBYTE)fsm.m_hBuffer + *fsm.m_lpdwBytesReceived,
                       fsm.m_dwBytesRead
                       );

    } else if (fsm.m_bAllocated && (fsm.m_hBuffer != NULL)) {

        //
        // if we failed but allocated a buffer then we need to free it (we were
        // leaking this buffer if the request was cancelled)
        //

        fsm.m_hBuffer = FREE_MEMORY(fsm.m_hBuffer);

        INET_ASSERT(fsm.m_hBuffer == NULL);

        fsm.m_dwBufferLength = 0;
        fsm.m_dwBufferLeft = 0;
        fsm.m_dwBytesReceived = 0;
        fsm.m_bEof = TRUE;
    }

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("returning: lpBuffer=%#x, bufferLength=%d, bufferLeft=%d, bytesReceived=%d\n",
                fsm.m_hBuffer,
                fsm.m_dwBufferLength,
                fsm.m_dwBufferLeft,
                fsm.m_dwBytesReceived
                ));

    //
    // update output parameters
    //

    *fsm.m_lplpBuffer = (LPVOID)fsm.m_hBuffer;
    *fsm.m_lpdwBufferLength = fsm.m_dwBufferLength;
    *fsm.m_lpdwBufferRemaining = fsm.m_dwBufferLeft;
    *fsm.m_lpdwBytesReceived = fsm.m_dwBytesReceived;
    *fsm.m_lpbEof = fsm.m_bEof;

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone(error);
    }

done:

    DEBUG_LEAVE(error);

    return error;
}

//
//DWORD
//ICSocket::ReceiveFrom(
//    IN LPVOID lpBuffer,
//    IN DWORD dwBufferLength,
//    OUT LPDWORD lpdwBytesReceived,
//    OUT LPSOCKADDR lpDestination OPTIONAL,
//    IN OUT LPDWORD lpdwDestinationLength OPTIONAL,
//    IN DWORD dwTimeout,
//    IN DWORD dwWinsockFlags,
//    IN DWORD dwFlags
//    )
//
///*++
//
//Routine Description:
//
//    Wrapper for recvfrom()
//
//Arguments:
//
//    lpBuffer                - pointer to buffer where data returned
//
//    dwBufferLength          - size of lpBuffer in bytes
//
//    lpdwBytesReceived       - pointer to returned number of bytes received
//
//    lpDestination           - pointer to returned destination address
//
//    lpdwDestinationLength   - IN: size of lpDestination buffer
//                              OUT: length of returned destination address info
//
//    dwTimeout               - number of milliseconds to wait for response
//
//    dwWinsockFlags          - flags to pass through to recvfrom()
//
//    dwFlags                 - ICSocket flags
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - ERROR_INTERNET_OPERATION_CANCELLED
//                    The operation was cancelled by the caller
//
//                  ERROR_INTERNET_TIMEOUT
//                    The operation timed out
//
//                  ERROR_INTERNET_CONNECTION_RESET
//                    An error occurred. We approximate to connection reset
//
//                  WSA error
//                    Some other sockets error occurred
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SOCKETS,
//                 Dword,
//                 "ICSocket::ReceiveFrom",
//                 "{%#x} %#x, %d, %#x, %#x, %#x [%d], %d, %#x, %#x",
//                 m_Socket,
//                 lpBuffer,
//                 dwBufferLength,
//                 lpdwBytesReceived,
//                 lpDestination,
//                 lpdwDestinationLength,
//                 lpdwDestinationLength ? *lpdwDestinationLength : 0,
//                 dwTimeout,
//                 dwWinsockFlags,
//                 dwFlags
//                 ));
//
//    //INET_ASSERT(IsSocketValid());
//    INET_ASSERT(lpdwBytesReceived != NULL);
//
//    //
//    // most ICSocket flags not allowed for this operation
//    //
//
//    INET_ASSERT(!(dwFlags
//                  & (SF_ENCRYPT
//                     | SF_DECRYPT
//                     | SF_EXPAND
//                     | SF_COMPRESS
//                     | SF_SENDING_DATA
//                     | SF_SCH_REDO
//                     )
//                  )
//                );
//
//    DWORD error = ERROR_SUCCESS;
//    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
//    BOOL bStopOfflineTimer = FALSE;
//    BOOL fNonBlocking;
//    DWORD bytesReceived;
//    INTERNET_HANDLE_OBJECT * pObject = NULL;
//
//    if (lpThreadInfo == NULL) {
//
//        INET_ASSERT(FALSE);
//
//        error = ERROR_INTERNET_INTERNAL_ERROR;
//        goto quit;
//    }
//
//    //
//    // the socket may have already been aborted
//    //
//
//    if (IsAborted()) {
//        error = ERROR_INTERNET_OPERATION_CANCELLED;
//        goto quit;
//    }
//
//    //
//    // if we are offline then quit now - we can't make any network requests
//    //
//
//    if (IsOffline()) {
//        error = ERROR_INTERNET_OFFLINE;
//        goto quit;
//    }
//
//    //
//    // let another thread know the socket to cancel if it wants to kill this
//    // operation
//    //
//
//    pObject = (INTERNET_HANDLE_OBJECT * )lpThreadInfo->hObjectMapped;
//    if (pObject != NULL) {
//        pObject->SetAbortHandle(this);
//    }
//
//    //
//    // keep the app informed (if requested to do so)
//    //
//
//    if (dwFlags & SF_INDICATE) {
//        InternetIndicateStatus(INTERNET_STATUS_RECEIVING_RESPONSE,
//                               NULL,
//                               0
//                               );
//    }
//
//    //
//    // if we are in async (== non-blocking) mode, let the async request
//    // scheduler know what operation we will be waiting on
//    //
//
//    fNonBlocking = lpThreadInfo->IsAsyncWorkerThread;
//    if (fNonBlocking) {
//
//        INET_ASSERT(lpThreadInfo->lpArb != NULL);
//
//        SET_ARB_SOCKET_OPERATION_TIMEOUT(lpThreadInfo->lpArb,
//                                         m_Socket,
//                                         RECEIVE,
//                                         dwTimeout
//                                         );
//
//        DWORD timerError = StartOfflineTimerForArb(lpThreadInfo->lpArb);
//
//        INET_ASSERT(timerError == ERROR_SUCCESS);
//
//        bStopOfflineTimer = (timerError == ERROR_SUCCESS) ? TRUE : FALSE;
//    }
//
//    int nBytes;
//
//    bytesReceived = 0;
//
//    do {
//
//        nBytes = _I_recvfrom(m_Socket,
//                             (char FAR *)lpBuffer + bytesReceived,
//                             dwBufferLength,
//                             dwWinsockFlags,
//                             lpDestination,
//                             (int FAR *)lpdwDestinationLength
//                             );
//        if (nBytes != SOCKET_ERROR) {
//
//            DEBUG_PRINT(SOCKETS,
//                        INFO,
//                        ("received %d bytes from socket %#x\n",
//                        nBytes,
//                        m_Socket
//                        ));
//
//            INET_ASSERT(nBytes > 0);
//
//            bytesReceived += nBytes;
//            dwBufferLength -= nBytes;
//
//            //
//            // for recvfrom(), we quit as soon as we get some data
//            //
//
//            error = ERROR_SUCCESS;
//            break;
//        } else {
//            error = _I_WSAGetLastError();
//            if ((error == WSAEWOULDBLOCK) && fNonBlocking) {
//
//                INET_ASSERT(_dwFlags & SF_NON_BLOCKING);
//
//                //
//                // if this function is called expedited (we expect the request
//                // to complete quickly) then we test to see if it already
//                // completed before switching to the async scheduler
//                //
//
//                BOOL switchFiber = TRUE;
//
//                if (dwFlags & SF_EXPEDITED) {
//                    error = WaitForReceive(1);
//
//                    //
//                    // if the socket is already readable then we don't switch
//                    // fibers (only to virtually immediately come back here,
//                    // incurring a couple of thread switches
//                    //
//
//                    if (error == ERROR_SUCCESS) {
//                        switchFiber = FALSE;
//
//                        //
//                        // use this error to go round loop once again
//                        //
//
//                        error = WSAEWOULDBLOCK;
//                    }
//                }
//                if (switchFiber) {
//
//                    DEBUG_PRINT(SOCKETS,
//                                INFO,
//                                ("recvfrom(%#x) blocked\n",
//                                m_Socket
//                                ));
//
//                    lpThreadInfo->lpArb->Header.dwResultCode = ERROR_SUCCESS;
//
//                    SwitchToAsyncScheduler(m_Socket);
//
//                    error = lpThreadInfo->lpArb->Header.dwResultCode;
//
//                    DEBUG_PRINT(SOCKETS,
//                                INFO,
//                                ("recvfrom(%#x) resumed, returns %s\n",
//                                m_Socket,
//                                InternetMapError(error)
//                                ));
//
//                    if (error != ERROR_SUCCESS) {
//                        if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
//                            error = ERROR_INTERNET_CONNECTION_RESET;
//                        }
//                    } else {
//
//                        //
//                        // use this error to force another loop now we believe
//                        // we have the data
//                        //
//
//                        error = WSAEWOULDBLOCK;
//                    }
//                }
//            } else {
//
//                //
//                // real error
//                //
//
//                error = MapInternetError(error);
//            }
//        }
//    } while (error == WSAEWOULDBLOCK);
//
//    if (error == ERROR_SUCCESS) {
//
//        DEBUG_DUMP(SOCKETS,
//                   "received data:\n",
//                   lpBuffer,
//                   bytesReceived
//                   );
//
//    }
//
//    if (fNonBlocking) {
//
//        INET_ASSERT(lpThreadInfo->lpArb != NULL);
//
//        SET_ARB_SOCKET_OPERATION(lpThreadInfo->lpArb, INVALID_SOCKET, RECEIVE);
//
//        if (bStopOfflineTimer) {
//            StopOfflineTimerForArb(lpThreadInfo->lpArb);
//        }
//    }
//
//    //
//    // inform the app that we finished, and tell it how much we received this
//    // time
//    //
//
//    if ((dwFlags & SF_INDICATE) && (error == ERROR_SUCCESS)) {
//        InternetIndicateStatus(INTERNET_STATUS_RESPONSE_RECEIVED,
//                               &bytesReceived,
//                               sizeof(bytesReceived)
//                               );
//    }
//
//    *lpdwBytesReceived = bytesReceived;
//
//    if (pObject != NULL) {
//        pObject->ResetAbortHandle();
//
//        //
//        // if the operation has been cancelled, then this error overrides any
//        // other
//        //
//
//        //if (pObject->IsInvalidated()) {
//        //    error = pObject->GetError();
//        //    if (error == ERROR_SUCCESS) {
//        //        error = ERROR_INTERNET_OPERATION_CANCELLED;
//        //    }
//        //}
//        if (IsAborted()) {
//            error = ERROR_INTERNET_OPERATION_CANCELLED;
//        }
//    }
//
//quit:
//
//    INET_ASSERT((pObject != NULL) ? (pObject->GetAbortHandle() == NULL) : TRUE);
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}
//

DWORD
ICSocket::DataAvailable(
    OUT LPDWORD lpdwBytesAvailable
    )

/*++

Routine Description:

    Determines the amount of data available to be read on the socket

Arguments:

    lpdwBytesAvailable  - pointer to returned data available


Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::DataAvailable",
                 "%#x",
                 lpdwBytesAvailable
                 ));

    //
    // sanity check parameters
    //

    INET_ASSERT(m_Socket != INVALID_SOCKET);
    INET_ASSERT(lpdwBytesAvailable != NULL);

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();
    DWORD error;

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // if we are offline then quit now - we can't make any network requests
    //

    if (IsOffline()) {
        error = ERROR_INTERNET_OFFLINE;
        goto quit;
    }

    //
    // the socket may already be aborted
    //

    if (IsAborted()) {
        error = ERROR_INTERNET_OPERATION_CANCELLED;
        goto quit;
    }

    //
    // if we're in async mode, we have to perform a zero-length receive in order
    // to get the information from the socket
    //

    int nRead;

    //
    // we actually have to peek a non-zero number of bytes because on Win95,
    // attempting to perform a receive of 0 bytes (to put the socket in blocked
    // read mode) results in zero bytes being returned, and the socket never
    // blocks
    //

    nRead = _I_recv(m_Socket, NULL, 0, 0);

    //
    // N.B. buf[] will only ever be used if there is data to peek right now
    //

    char buf[1];

    PERF_LOG(PE_PEEK_RECEIVE_START,
             m_Socket,
             lpThreadInfo->ThreadId,
             lpThreadInfo->hObject
             );

    nRead = _I_recv(m_Socket, buf, sizeof(buf), MSG_PEEK);
    if (nRead == SOCKET_ERROR) {
        error = _I_WSAGetLastError();
        if ((error == WSAEWOULDBLOCK) && (m_dwFlags & SF_NON_BLOCKING)) {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("peek(1) blocked, socket %#x\n",
                        m_Socket
                        ));

            PERF_LOG(PE_PEEK_RECEIVE_END,
                     m_Socket,
                     lpThreadInfo->ThreadId,
                     lpThreadInfo->hObject
                     );

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("peek(1) resumed, socket %#x, returns %s\n",
                        m_Socket,
                        InternetMapError(error)
                        ));

            if (error != ERROR_SUCCESS) {
                if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
                    error = ERROR_INTERNET_CONNECTION_RESET;
                }
            }
        }
    /*} else if ((nRead == 0) && !(m_dwFlags & SF_NON_BLOCKING)) {

        PERF_LOG(PE_PEEK_RECEIVE_END,
                 m_Socket,
                 lpThreadInfo->ThreadId,
                 lpThreadInfo->hObject
                 );

        //
        // nothing to peek right now. If the socket is in blocking mode then
        // we wait here until there is something to receive
        //

        error = WaitForReceive(INFINITE);*/
    } else {

        PERF_LOG(PE_PEEK_RECEIVE_END,
                 m_Socket,
                 lpThreadInfo->ThreadId,
                 lpThreadInfo->hObject
                 );

        //
        // nRead == 0 but non-blocking, or nRead > 0
        //

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("peek(1) returns %d\n",
                    nRead
                    ));

        error = ERROR_SUCCESS;
    }

    if (error == ERROR_SUCCESS) {

        //
        // now we can get the amount from the socket
        //

        error = (DWORD)_I_ioctlsocket(m_Socket,
                                      FIONREAD,
                                      (u_long FAR *)lpdwBytesAvailable
                                      );

        //
        // N.B. assumes ioctlsocket() returns 0 on success == ERROR_SUCCESS
        //

        if (error == SOCKET_ERROR) {
            error = _I_WSAGetLastError();
        } else {

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("ioctlsocket(FIONREAD) returns %d\n",
                        *lpdwBytesAvailable
                        ));

        }
    }

    //
    // map any sockets error to WinInet error
    //

    if (error != ERROR_SUCCESS) {
        error = MapInternetError(error);
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}

//
//DWORD
//ICSocket::DataAvailable2(
//    OUT LPVOID lpBuffer,
//    IN DWORD dwBufferLength,
//    OUT LPDWORD lpdwBytesAvailable
//    )
//
///*++
//
//Routine Description:
//
//    Determines the amount of data available to be read on the socket
//
//Arguments:
//
//    lplpBuffer          - pointer to pointer to buffer where data read
//
//    dwBufferLength      - size of the buffer
//
//    lpdwBytesAvailable  - pointer to returned data available
//
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure - WSA error
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SOCKETS,
//                 Dword,
//                 "ICSocket::DataAvailable2",
//                 "%#x, %d, %#x",
//                 lpBuffer,
//                 dwBufferLength,
//                 lpdwBytesAvailable
//                 ));
//
//    //
//    // sanity check parameters
//    //
//
//    INET_ASSERT(lpdwBytesAvailable != NULL);
//
//    //
//    // we're about to receive data from the socket. The amount of data currently
//    // on hand must be 0
//    //
//
//    INET_ASSERT(*lpdwBytesAvailable == 0);
//    INET_ASSERT(lpBuffer != NULL);
//
//    DWORD error;
//
//    //
//    // new scheme: actually read the data from sockets into our buffer. This is
//    // the only way on Win95 to determine the correct number of bytes available.
//    // We only perform a single receive
//    //
//
//    DWORD bufferLeft = dwBufferLength;
//    BOOL eof;
//
//    error = Receive(&lpBuffer,
//                    &dwBufferLength,
//                    &bufferLeft,  // don't care about this
//                    lpdwBytesAvailable,
//                    0,
//                    0,
//                    &eof          // don't care about this either
//                    );
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}


DWORD
ICSocket::WaitForReceive(
    IN DWORD Timeout
    )

/*++

Routine Description:

    Waits until a receive socket becomes unblocked (readable)

Arguments:

    Timeout - milliseconds to wait, or INFINITE

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error
                    sockets error

                  ERROR_INTERNET_TIMEOUT
                    Receive timed out

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::WaitForReceive",
                 "{%#x} %d",
                 m_Socket,
                 Timeout
                 ));

    struct fd_set read_fds;
    struct fd_set except_fds;

    FD_ZERO(&read_fds);
    FD_ZERO(&except_fds);

    FD_SET(m_Socket, &read_fds);
    FD_SET(m_Socket, &except_fds);

    int n;

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("waiting on socket %#x\n",
                m_Socket
                ));

    TIMEVAL timeout;
    LPTIMEVAL lpTimeout;

    if (Timeout != INFINITE) {
        timeout.tv_sec  = Timeout / 1000;
        timeout.tv_usec = (Timeout % 1000) * 1000;
        lpTimeout = &timeout;
    } else {
        lpTimeout = NULL;
    }

    n = _I_select(0, &read_fds, NULL, &except_fds, lpTimeout);

    DWORD error;

    if (n == SOCKET_ERROR) {

        //
        // real error?
        //

        error = _I_WSAGetLastError();

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("select() returns %d\n",
                    error
                    ));

        INET_ASSERT(FALSE);

        error = MapInternetError(error);
    } else if (n != 0) {
        if (FD_ISSET(m_Socket, &except_fds)) {

            DEBUG_PRINT(SOCKETS,
                        ERROR,
                        ("socket %#x exception\n",
                        m_Socket
                        ));

            error = ERROR_INTERNET_CONNECTION_RESET;
        } else {

            //
            // it *must* be unblocked (i.e. readable)
            //

            INET_ASSERT(FD_ISSET(m_Socket, &read_fds));

            DEBUG_PRINT(SOCKETS,
                        INFO,
                        ("socket %#x unblocked\n",
                        m_Socket
                        ));

            error = ERROR_SUCCESS;
        }
    } else {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("timed out\n"
                    ));

        error = ERROR_INTERNET_TIMEOUT;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::AllocateQueryBuffer(
    OUT LPVOID * lplpBuffer,
    OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Allocates a query buffer for the socket

Arguments:

    lplpBuffer          - returned pointer to allocated query buffer

    lpdwBufferLength    - returned length of allocated query buffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::AllocateQueryBuffer",
                 "{%#x/%d} %#x, %#x",
                 GetSocket(),
                 GetSourcePort(),
                 lplpBuffer,
                 lpdwBufferLength
                 ));

    DWORD error;
    DWORD bufferLength;
    DWORD size = sizeof(bufferLength);

    int serr = _I_getsockopt(m_Socket,
                             SOL_SOCKET,
                             SO_RCVBUF,
                             (char FAR *)&bufferLength,
                             (int FAR *)&size
                             );
    if (serr != SOCKET_ERROR) {
        bufferLength = min(bufferLength, DEFAULT_SOCKET_QUERY_BUFFER_LENGTH);
        if (bufferLength == 0) {
            bufferLength = DEFAULT_SOCKET_QUERY_BUFFER_LENGTH;
        }
        *lplpBuffer = (LPVOID)ALLOCATE_MEMORY(LMEM_FIXED, bufferLength);
        if (*lplpBuffer != NULL) {
            *lpdwBufferLength = bufferLength;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        error = MapInternetError(_I_WSAGetLastError());
    }

    DEBUG_LEAVE(error);

    return error;
}

//
//VOID
//ICSocket::FreeQueryBuffer(
//    IN LPVOID lpBuffer
//    )
//
///*++
//
//Routine Description:
//
//    description-of-function.
//
//Arguments:
//
//    lpBuffer    -
//
//Return Value:
//
//    None.
//
//--*/
//
//{
//    lpBuffer = (LPVOID)FREE_MEMORY((HLOCAL)lpBuffer);
//
//    INET_ASSERT(lpBuffer == NULL);
//}

//
//DWORD
//ICSocket::GetBytesAvailable(
//    OUT LPDWORD lpdwBytesAvailable
//    )
//
///*++
//
//Routine Description:
//
//    Determines amount of data available to be read from socket
//
//Arguments:
//
//    lpdwBytesAvailable  - pointer to returned available length
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure -
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SOCKETS,
//                 Dword,
//                 "ICSocket::GetBytesAvailable",
//                 "{%#x} %#x",
//                 m_Socket,
//                 lpdwBytesAvailable
//                 ));
//
//    //INET_ASSERT(m_Socket != INVALID_SOCKET);
//    INET_ASSERT(lpdwBytesAvailable != NULL);
//
//    //
//    // get the amount from the socket. If the socket has been reset or shutdown
//    // by the server then we expect to get an error, else 0 (== ERROR_SUCCESS)
//    //
//
//    DWORD error = (DWORD)_I_ioctlsocket(m_Socket,
//                                        FIONREAD,
//                                        (u_long FAR *)lpdwBytesAvailable
//                                        );
//    if (error == SOCKET_ERROR) {
//        error = _I_WSAGetLastError();
//    } else {
//
//        DEBUG_PRINT(SOCKETS,
//                    INFO,
//                    ("ioctlsocket(FIONREAD) returns %d\n",
//                    *lpdwBytesAvailable
//                    ));
//
//    }
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}
//

DWORD
ICSocket::CreateSocket(
    IN DWORD dwFlags,
    IN int nAddressFamily,
    IN int nType,
    IN int nProtocol
    )

/*++

Routine Description:

    Opens a socket handle for this ICSocket object

Arguments:

    dwFlags         - flags to use for new socket

    nAddressFamily  - parameter to socket()

    nType           - parameter to socket()

    nProtocol       - parameter to socket()

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error mapped to INTERNET error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::CreateSocket",
                 "%#x, %s (%d), %s (%d), %s (%d)",
                 dwFlags,
                 MapFamily(nAddressFamily),
                 nAddressFamily,
                 MapSock(nType),
                 nType,
                 MapProto(nProtocol),
                 nProtocol
                 ));

    INET_ASSERT(m_Socket == INVALID_SOCKET);

    int serr;
    DWORD error;
    DWORD dwConnFlags;

#if defined(SITARA)

    //
    // Only enable Sitara if we're connected via modem
    //

//dprintf("create socket: IsSitara = %B, IsModemConn=%B\n",GlobalEnableSitara, GlobalHasSitaraModemConn);
    if (GlobalEnableSitara && GlobalHasSitaraModemConn) {
        nProtocol = (int)GetSitaraProtocol();
    }

#endif

    m_Socket = _I_socket(nAddressFamily, nType, nProtocol);
    if (m_Socket == INVALID_SOCKET) {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("failed to create socket\n"
                    ));

        goto socket_error;
    }

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("created socket %#x\n",
                m_Socket
                ));

    if (dwFlags & SF_NON_BLOCKING) {
        INET_ASSERT(FALSE);

        error = SetNonBlockingMode(TRUE);
        if (error == ERROR_SUCCESS) {

            //
            //  ICSocket is non-blocking socket object
            //

            m_dwFlags |= SF_NON_BLOCKING;
        } else {
            goto close_socket;
        }
    }

    //
    // bind our data socket to an endpoint, so that we know an address to
    // tell the FTP server
    //

    SOCKADDR_IN ourDataAddr;

    ourDataAddr.sin_family = AF_INET;
    *((long *)&ourDataAddr.sin_addr) = INADDR_ANY;
    ourDataAddr.sin_port = 0;

    serr = _I_bind(m_Socket,
                   (PSOCKADDR)&ourDataAddr,
                   sizeof(ourDataAddr)
                   );

    if (serr == SOCKET_ERROR) {
        goto socket_error;
    }

    error = ERROR_SUCCESS;

quit:

    DEBUG_LEAVE(error);

    return error;

socket_error:

    error = MapInternetError(_I_WSAGetLastError());

close_socket:

    Close();
    m_dwFlags &= ~SF_NON_BLOCKING;
    goto quit;
}


DWORD
ICSocket::GetSockName(
    PSOCKADDR psaSockName
    )
{
    INET_ASSERT(m_Socket != INVALID_SOCKET);
    INET_ASSERT(psaSockName);

    int serr;
    int cbAddrLen;
    DWORD error;

    serr = ERROR_SUCCESS;
    error = ERROR_SUCCESS;

    //
    // get the address info .
    //

    cbAddrLen = sizeof(SOCKADDR_IN);


    serr = _I_getsockname(m_Socket,
                          psaSockName,
                          &cbAddrLen
                          );


    if ( serr == SOCKET_ERROR )
    {
        error = _I_WSAGetLastError();
    }

    return error;
}


DWORD
ICSocket::Listen(
    VOID
    )
{
    INET_ASSERT(m_Socket != INVALID_SOCKET);

    DWORD error = ERROR_SUCCESS;

    //
    // Listen on the socket.
    //

    if (_I_listen(m_Socket, 1) == SOCKET_ERROR) {
        error = _I_WSAGetLastError();
    }
    return error;
}


DWORD
ICSocket::DirectConnect(
    PSOCKADDR psaRemoteSock
    )

/*++

Routine Description:

    Connects a ICSocket to the remote address

Arguments:

    psaRemoteSock   - pointer to remote socket address (TCP/IP!)

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error mapped to INTERNET error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::DirectConnectSocket",
                 "{%#x} %#x",
                 m_Socket,
                 psaRemoteSock
                 ));

    INET_ASSERT(m_Socket != INVALID_SOCKET);

    DWORD error;
    BOOL bStopOfflineTimer = FALSE;

    //
    // we need the thread info for async processing
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // if we are offline then quit now - we can't make any network requests
    //

    if (IsOffline()) {
        error = ERROR_INTERNET_OFFLINE;
        goto quit;
    }

    BOOL isAsync;

    isAsync = lpThreadInfo->IsAsyncWorkerThread;

    //
    // BUGBUG - this is essentially common to ConnectSocket()
    //

    //
    // let another thread know the socket to cancel if it wants to kill
    // this operation
    //

    INTERNET_HANDLE_OBJECT * pObject;

    pObject = (INTERNET_HANDLE_OBJECT * )lpThreadInfo->hObjectMapped;
    if (pObject != NULL) {
        pObject->SetAbortHandle(this);
    }

#if defined(UNIX) && defined(ux10)
    DEBUG_PRINT(SOCKETS,
                INFO,
                ("connecting to remote address %d.%d.%d.%d, port %d\n",
                ((LPBYTE)&((LPSOCKADDR_IN)psaRemoteSock)->sin_addr)[0],
                ((LPBYTE)&((LPSOCKADDR_IN)psaRemoteSock)->sin_addr)[1],
                ((LPBYTE)&((LPSOCKADDR_IN)psaRemoteSock)->sin_addr)[2],
                ((LPBYTE)&((LPSOCKADDR_IN)psaRemoteSock)->sin_addr)[3],
                _I_ntohs(((LPSOCKADDR_IN)psaRemoteSock)->sin_port)
                ));
#else
    DEBUG_PRINT(SOCKETS,
                INFO,
                ("connecting to remote address %d.%d.%d.%d, port %d\n",
                ((LPSOCKADDR_IN)psaRemoteSock)->sin_addr.S_un.S_un_b.s_b1,
                ((LPSOCKADDR_IN)psaRemoteSock)->sin_addr.S_un.S_un_b.s_b2,
                ((LPSOCKADDR_IN)psaRemoteSock)->sin_addr.S_un.S_un_b.s_b3,
                ((LPSOCKADDR_IN)psaRemoteSock)->sin_addr.S_un.S_un_b.s_b4,
                _I_ntohs(((LPSOCKADDR_IN)psaRemoteSock)->sin_port)
                ));
#endif

    DWORD connectTime;

    connectTime = GetTickCount();

    int serr;

    PERF_LOG(PE_CONNECT_START,
             m_Socket,
             lpThreadInfo->ThreadId,
             lpThreadInfo->hObject
             );

    if (IsSocks()) {
        serr = SocksConnect((LPSOCKADDR_IN)psaRemoteSock, sizeof(SOCKADDR_IN));
    } else {
        serr = _I_connect(m_Socket, psaRemoteSock, sizeof(SOCKADDR_IN));
    }
    if (serr != 0) {
        error = _I_WSAGetLastError();

        //
        // if we are using non-blocking sockets then we need to wait until
        // the connect has completed, or an error occurs
        //

        if (isAsync) {
            if (error == WSAEWOULDBLOCK) {

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("connect() blocked, socket %#x\n",
                            m_Socket
                            ));

                PERF_LOG(PE_CONNECT_END,
                         m_Socket,
                         lpThreadInfo->ThreadId,
                         lpThreadInfo->hObject
                         );

                connectTime = GetTickCount() - connectTime;

                DEBUG_PRINT(SOCKETS,
                            INFO,
                            ("connect() resumed, socket %#x, returns %s\n",
                            m_Socket,
                            InternetMapError(error)
                            ));

                if (error != ERROR_SUCCESS) {
                    if (error == ERROR_INTERNET_INTERNAL_SOCKET_ERROR) {
                        error = ERROR_INTERNET_CANNOT_CONNECT;
                    }
                }
            } else {

                DEBUG_PRINT(SOCKETS,
                            ERROR,
                            ("failed to connect non-blocking socket %#x, error %d\n",
                            m_Socket,
                            error
                            ));

            }
        } else {

            DEBUG_PRINT(SOCKETS,
                        ERROR,
                        ("failed to connect blocking socket %#x, error %d\n",
                        m_Socket,
                        error
                        ));

        }
    } else {

        PERF_LOG(PE_CONNECT_END,
                 m_Socket,
                 lpThreadInfo->ThreadId,
                 lpThreadInfo->hObject
                 );

        connectTime = GetTickCount() - connectTime;

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("socket %#x connected, time = %d mSec\n",
                    m_Socket,
                    connectTime
                    ));

        error = ERROR_SUCCESS;
    }

    if (error != ERROR_SUCCESS) {
        error = MapInternetError(error);
    }

    if (pObject != NULL) {
        pObject->ResetAbortHandle();

        //
        // if the operation has been cancelled, then this error overrides any
        // other
        //

        if (pObject->IsInvalidated()) {
            error = pObject->GetError();
            if (error == ERROR_SUCCESS) {
                error = ERROR_INTERNET_OPERATION_CANCELLED;
            }
        }
        if (IsAborted()) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        }
    }

quit:

    INET_ASSERT((pObject != NULL) ? (pObject->GetAbortHandle() == NULL) : TRUE);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ICSocket::SelectAccept(
    IN ICSocket & acceptSocket,
    IN DWORD dwTimeout
    )

/*++

Routine Description:

    Wait until listening socket has connection to accept. We use the socket
    handle in this ICSocket object to accept a connection & create a socket
    handle in another ICSocket object (in acceptSocket)

Arguments:

    acceptSocket    - socket object to wait on

    dwTimeout       - number of milliseconds to wait

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error mapped to INTERNET error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                 Dword,
                 "ICSocket::SelectAccept",
                 "%#x, %d",
                 &acceptSocket,
                 dwTimeout
                 ));

    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    fd_set read_fds;
    fd_set except_fds;

    FD_ZERO(&read_fds);
    FD_ZERO(&except_fds);
    FD_SET(m_Socket, &read_fds);
    FD_SET(m_Socket, &except_fds);

    TIMEVAL timeout;

    timeout.tv_sec  = dwTimeout / 1000;
    timeout.tv_usec = dwTimeout % 1000;

    int n;

    n = _I_select(0, &read_fds, NULL, &except_fds, &timeout);
    if (n == 1) {
        if (FD_ISSET(m_Socket, &read_fds)) {
            error = ERROR_SUCCESS;
        } else if (FD_ISSET(m_Socket, &except_fds)) {
            error = ERROR_INTERNET_CANNOT_CONNECT;

            DEBUG_PRINT(FTP,
                        ERROR,
                        ("select(): listening socket %#x in error (%d)\n",
                        m_Socket,
                        error
                        ));

            INET_ASSERT(acceptSocket.m_Socket == INVALID_SOCKET);
        }
    } else if (n == 0) {

        //
        // timeout
        //

        error = ERROR_INTERNET_TIMEOUT;

        DEBUG_PRINT(FTP,
                    WARNING,
                    ("select() timed out (%d.%d)\n",
                    timeout.tv_sec,
                    timeout.tv_usec
                    ));

        INET_ASSERT(acceptSocket.m_Socket == INVALID_SOCKET);

    } else {

        //
        // socket error
        //

        DEBUG_PRINT(FTP,
                    ERROR,
                    ("select() returns %d\n",
                    _I_WSAGetLastError()
                    ));

        INET_ASSERT(acceptSocket.m_Socket == INVALID_SOCKET);

        goto socket_error;
    }

    //
    // if we have a success indication then accept the connection; it may still
    // fail
    //

    if (error == ERROR_SUCCESS) {
        acceptSocket.m_Socket = _I_accept(m_Socket, NULL, NULL);
        if (acceptSocket.m_Socket != INVALID_SOCKET) {

            //
            // copy non-blocking indication to new socket
            //

            INET_ASSERT(!(m_dwFlags & SF_NON_BLOCKING));
            //acceptSocket.m_dwFlags |= (m_dwFlags & SF_NON_BLOCKING);
        } else {

            DEBUG_PRINT(FTP,
                        ERROR,
                        ("accept() returns %d\n",
                        error
                        ));

            goto socket_error;
        }
    }

quit:

    DEBUG_LEAVE(error);

    return error;

socket_error:

    error = MapInternetError(_I_WSAGetLastError());
    goto quit;
}


LPSTR
MapNetAddressToName(
    IN LPSTR lpszAddress,
    OUT LPSTR * lplpszMappedName
    )

/*++

Routine Description:

    Given a network address, tries to map it to the corresponding host name. We
    consult the name resolution cache to determine this

Arguments:

    lpszAddress         - pointer to network address to map

    lplpszMappedName    - pointer to pointer to mapped name. Caller must free

Return Value:

    LPSTR
        Success - pointer to mapped name

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                Pointer,
                "MapNetAddressToName",
                "%q, %#x",
                lpszAddress,
                lplpszMappedName
                ));

    INET_ASSERT(lpszAddress != NULL);
    INET_ASSERT(lplpszMappedName != NULL);

    LPSTR lpszMappedName = NULL;

    //
    // now try to find the address in the cache. If it's not in the cache then
    // we don't resolve it, simply return the address
    //

    //
    // BUGBUG - if required, we need to resolve the name, but we need to know
    //          whether the address can be resolved on the intranet
    //

    DWORD ipAddr = _I_inet_addr(lpszAddress);

    //
    // inet_addr() shouldn't fail - we should have called IsNetAddress() already
    //

    //INET_ASSERT(ipAddr != INADDR_NONE);

    if (ipAddr != INADDR_NONE) {

        LPHOSTENT lpHostent;
        DWORD ttl;

        if (QueryHostentCache(NULL, (LPBYTE)&ipAddr, &lpHostent, &ttl)) {

            INET_ASSERT(lpHostent != NULL);

            lpszAddress = lpszMappedName = NewString(lpHostent->h_name);
            ReleaseHostentCacheEntry(lpHostent);
        }
    }

    DEBUG_PRINT(SOCKETS,
                INFO,
                ("mapped name is %q\n",
                lpszAddress
                ));

    DEBUG_LEAVE(lpszAddress);

    *lplpszMappedName = lpszMappedName;

    return lpszAddress;
}



DWORD
AUTO_PROXY_HELPER_APIS::ResolveHostName(
    IN LPSTR lpszHostName,
    IN OUT LPSTR   lpszIPAddress,
    IN OUT LPDWORD lpdwIPAddressSize
    )

/*++

Routine Description:

    Resolves a HostName to an IP address by using Winsock DNS.

Arguments:

    lpszHostName   - the host name that should be used.

    lpszIPAddress  - the output IP address as a string.

    lpdwIPAddressSize - the size of the outputed IP address string.

Return Value:

    DWORD
        Win32 error code.

--*/

{
    //
    // figure out if we're being asked to resolve a name or an address. If
    // inet_addr() succeeds then we were given a string respresentation of an
    // address
    //

    DWORD ipAddr;
    LPBYTE address;
    LPHOSTENT lpHostent;
    DWORD ttl;
    DWORD dwIPAddressSize;
    BOOL bFromCache = FALSE;

    DWORD error = ERROR_SUCCESS;

    ipAddr = _I_inet_addr(lpszHostName);
    if (ipAddr != INADDR_NONE)
    {
        dwIPAddressSize = lstrlen(lpszHostName);

        if ( *lpdwIPAddressSize < dwIPAddressSize ||
              lpszIPAddress == NULL )
        {
            *lpdwIPAddressSize = dwIPAddressSize+1;
            error = ERROR_INSUFFICIENT_BUFFER;
            goto quit;
        }

        lstrcpy(lpszIPAddress, lpszHostName);
        goto quit;
    }

    ipAddr = 0;
    address = (LPBYTE) &ipAddr;

    //
    // now try to find the name or address in the cache. If it's not in the
    // cache then resolve it
    //

    if (QueryHostentCache(lpszHostName, address, &lpHostent, &ttl)) {
        bFromCache = TRUE;
    } else {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("resolving %q\n",
                    lpszHostName
                    ));

        PERF_LOG(PE_NAMERES_START, 0);

        lpHostent = _I_gethostbyname(lpszHostName);

        PERF_LOG(PE_NAMERES_END, 0);

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("%q %sresolved\n",
                    lpszHostName,
                    lpHostent ? "" : "NOT "
                    ));


        //
        // if we successfully resolved the name or address then add the
        // information to the cache
        //

        if (lpHostent != NULL)
        {
            CacheHostent(lpszHostName, lpHostent, LIVE_DEFAULT);
        }
    }


    if ( lpHostent )
    {
        char *pszAddressStr;
        LPBYTE * addressList;
        struct  in_addr sin_addr;

        //     *(LPDWORD)&lpSin->sin_addr = *(LPDWORD)addressList[i];
        //              ((struct sockaddr_in*)lpSockAddr)->sin_addr
        //                   struct  in_addr sin_addr

        addressList         = (LPBYTE *)lpHostent->h_addr_list;
        *(LPDWORD)&sin_addr = *(LPDWORD)addressList[0] ;

        pszAddressStr = _I_inet_ntoa (sin_addr);

        INET_ASSERT(pszAddressStr);

        dwIPAddressSize = lstrlen(pszAddressStr);

        if ( *lpdwIPAddressSize < dwIPAddressSize ||
              lpszIPAddress == NULL )
        {
            *lpdwIPAddressSize = dwIPAddressSize+1;
            error = ERROR_INSUFFICIENT_BUFFER;
            goto quit;
        }

        lstrcpy(lpszIPAddress, pszAddressStr);

        goto quit;

    }

    //
    // otherwise, if we get here its an error
    //

    error = ERROR_INTERNET_NAME_NOT_RESOLVED;

quit:

    if (bFromCache) {

        INET_ASSERT(lpHostent != NULL);

        ReleaseHostentCacheEntry(lpHostent);
    }

    return error;
}




BOOL
AUTO_PROXY_HELPER_APIS::IsResolvable(
    IN LPSTR lpszHost
    )

/*++

Routine Description:

    Determines wheter a HostName can be resolved.  Performs a Winsock DNS query,
      and if it succeeds returns TRUE.

Arguments:

    lpszHost   - the host name that should be used.

Return Value:

    BOOL
        TRUE - the host is resolved.

        FALSE - could not resolve.

--*/

{

    DWORD dwDummySize;
    DWORD error;

    error = ResolveHostName(
                lpszHost,
                NULL,
                &dwDummySize
                );

    if ( error == ERROR_INSUFFICIENT_BUFFER )
    {
        return TRUE;
    }
    else
    {
        INET_ASSERT(error != ERROR_SUCCESS );
        return FALSE;
    }

}

DWORD
AUTO_PROXY_HELPER_APIS::GetIPAddress(
    IN OUT LPSTR   lpszIPAddress,
    IN OUT LPDWORD lpdwIPAddressSize
    )

/*++

Routine Description:

    Acquires the IP address string of this client machine WININET is running on.

Arguments:

    lpszIPAddress   - the IP address of the machine, returned.

    lpdwIPAddressSize - size of the IP address string.

Return Value:

    DWORD
        Win32 Error.

--*/

{

    CHAR szHostBuffer[255];
    int serr;

    serr = _I_gethostname(
                szHostBuffer,
                ARRAY_ELEMENTS(szHostBuffer)-1
                );

    if ( serr != ERROR_SUCCESS)
    {
        return ERROR_INTERNET_INTERNAL_ERROR;
    }

    return ResolveHostName(
                szHostBuffer,
                lpszIPAddress,
                lpdwIPAddressSize
                );

}



BOOL
AUTO_PROXY_HELPER_APIS::IsInNet(
    IN LPSTR   lpszIPAddress,
    IN LPSTR   lpszDest,
    IN LPSTR   lpszMask
    )

/*++

Routine Description:

    Determines whether a given IP address is in a given dest/mask IP address.

Arguments:

    lpszIPAddress   - the host name that should be used.

    lpszDest        - the IP address dest to check against.

    lpszMask        - the IP mask string

Return Value:

    BOOL
        TRUE - the IP address is in the given dest/mask

        FALSE - the IP address is NOT in the given dest/mask

--*/

{
    DWORD dwDest, dwIpAddr, dwMask;

    INET_ASSERT(lpszIPAddress);
    INET_ASSERT(lpszDest);
    INET_ASSERT(lpszMask);

    dwIpAddr = _I_inet_addr(lpszIPAddress);
    dwDest = _I_inet_addr(lpszDest);
    dwMask = _I_inet_addr(lpszMask);

    if ( dwDest   == INADDR_NONE ||
         dwIpAddr == INADDR_NONE  )

    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

        if ( (dwIpAddr & dwMask) != dwDest)
    {
        return FALSE;
        }

    //
    // Pass, its Matches.
    //

    return TRUE;
}
