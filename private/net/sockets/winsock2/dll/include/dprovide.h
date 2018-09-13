/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

        dprovide.h

Abstract:

        This module defines the WinSock2 class dprovder along with its methods.

Author:

        Mark Hamilton (mark_hamilton@ccm.jf.intel.com) 7-July-1995

Revision History:

    23-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Changed names of private data
        members. Moved single line functions into this header.

    25-July-1995 dirk@mink.intel.com
        Removed process linage data member.

    7-July-1995     mark_hamilton

                Genesis
--*/
#ifndef _DPROVIDER_
#define _DPROVIDER_

#include <winsock2.h>
#include <ws2spi.h>
#include "llist.h"
#include "dthook.h"


class DPROVIDER {

  public:

    DPROVIDER();

    INT
    Initialize(
        IN LPSTR lpszLibFile,
        IN LPWSAPROTOCOL_INFOW lpProtocolInfo);

    SOCKET
    WSPAccept(
        IN SOCKET s,
        OUT struct sockaddr FAR *addr,
        OUT INT FAR *addrlen,
        IN LPCONDITIONPROC lpfnCondition,
        IN DWORD_PTR dwCallbackData,
        OUT INT FAR *lpErrno);

    INT
    WSPAddressToString(
        IN     LPSOCKADDR lpsaAddress,
        IN     DWORD dwAddressLength,
        IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
        OUT    LPWSTR lpszAddressString,
        IN OUT LPDWORD lpdwAddressStringLength,
        OUT    LPINT lpErrno );

    INT
    WSPAsyncSelect(
        IN SOCKET s,
        IN HWND hWnd,
        IN unsigned int wMsg,
        IN long lEvent,
        OUT INT FAR *lpErrno);

    INT
    WSPBind(
        IN SOCKET s,
        IN const struct sockaddr FAR *name,
        IN INT namelen,
        OUT INT FAR *lpErrno);

    INT
    WSPCancelBlockingCall(
        OUT INT FAR *lpErrno);

    INT
    WSPCleanup(
        OUT INT FAR *lpErrno);

    INT
    WSPCloseSocket(
        IN SOCKET s,
        OUT INT FAR *lpErrno);

    INT
    WSPConnect(
        IN SOCKET s,
        IN const struct sockaddr FAR *name,
        IN INT namelen,
        IN LPWSABUF lpCallerData,
        IN LPWSABUF lpCalleeData,
        IN LPQOS lpSQOS,
        IN LPQOS lpGQOS,
        OUT INT FAR *lpErrno);

    INT
    WSPDuplicateSocket(
        IN SOCKET s,
        IN DWORD dwProcessID,
        IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
        OUT INT FAR *lpErrno);

    INT
    WSPEnumNetworkEvents(
        IN SOCKET s,
        OUT WSAEVENT hEventObject,
        OUT LPWSANETWORKEVENTS lpNetworkEvents,
        OUT INT FAR *lpErrno);

    INT
    WSPEventSelect(
        IN SOCKET s,
        IN OUT WSAEVENT hEventObject,
        IN long lNetworkEvents,
        OUT INT FAR *lpErrno);

    INT
    WSPGetOverlappedResult(
        IN SOCKET s,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPDWORD lpcbTransfer,
        IN BOOL fWait,
        OUT LPDWORD lpdwFlags,
        OUT INT FAR *lpErrno);

    INT
    WSPGetPeerName(
        IN SOCKET s,
        OUT struct sockaddr FAR *name,
        OUT INT FAR *namelen,
        OUT INT FAR *lpErrno);

    INT
    WSPGetQOSByName(
        IN SOCKET s,
        IN LPWSABUF lpQOSName,
        IN LPQOS lpQOS,
        OUT INT FAR *lpErrno);

    INT
    WSPGetSockName(
        IN SOCKET s,
        OUT struct sockaddr FAR *name,
        OUT INT FAR *namelen,
        OUT INT FAR *lpErrno);

    INT
    WSPGetSockOpt(
        IN SOCKET s,
        IN INT level,
        IN INT optname,
        OUT char FAR *optval,
        OUT INT FAR *optlen,
        OUT INT FAR *lpErrno);

    INT
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
        OUT INT FAR *lpErrno);

    SOCKET
    WSPJoinLeaf(
        IN SOCKET s,
        IN const struct sockaddr FAR *name,
        IN INT namelen,
        IN LPWSABUF lpCallerData,
        IN LPWSABUF lpCalleeData,
        IN LPQOS lpSQOS,
        IN LPQOS lpGQOS,
        IN DWORD dwFlags,
        OUT INT FAR *lpErrno);

    INT
    WSPListen(
        IN SOCKET s,
        IN INT backlog,
        OUT INT FAR *lpErrno);


    INT
    WSPRecv(
        IN SOCKET s,
        IN LPWSABUF lpBuffers,
        IN DWORD dwBufferCount,
        IN LPDWORD lpNumberOfBytesRecvd,
        IN OUT LPDWORD lpFlags,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
        IN LPWSATHREADID lpThreadId,
        OUT INT FAR *lpErrno);


    INT
    WSPRecvDisconnect(
        IN SOCKET s,
        IN LPWSABUF lpInboundDisconnectData,
        OUT INT FAR *lpErrno);


    INT
    WSPRecvFrom(
        IN SOCKET s,
        IN LPWSABUF lpBuffers,
        IN DWORD dwBufferCount,
        IN LPDWORD lpNumberOfBytesRecvd,
        IN OUT LPDWORD lpFlags,
        OUT  struct sockaddr FAR *  lpFrom,
        IN LPINT lpFromlen,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
        IN LPWSATHREADID lpThreadId,
        OUT INT FAR *lpErrno);


    INT
    WSPSelect(
        IN INT nfds,
        IN OUT fd_set FAR *readfds,
        IN OUT fd_set FAR *writefds,
        IN OUT fd_set FAR *exceptfds,
        IN const struct timeval FAR *timeout,
        OUT INT FAR *lpErrno);


    INT
    WSPSend(
        IN SOCKET s,
        IN LPWSABUF lpBuffers,
        IN DWORD dwBufferCount,
        OUT LPDWORD lpNumberOfBytesSent,
        IN DWORD dwFlags,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
        IN LPWSATHREADID lpThreadId,
        OUT INT FAR *lpErrno);

    INT
    WSPSendDisconnect(
        IN SOCKET s,
        IN LPWSABUF lpOutboundDisconnectData,
        OUT INT FAR *lpErrno);

    INT
    WSPSendTo(
        IN SOCKET s,
        IN LPWSABUF lpBuffers,
        IN DWORD dbBufferCount,
        IN LPDWORD lpNumberOfBytesSent,
        IN DWORD dwFlags,
        IN const struct sockaddr FAR * lpTo,
        IN INT iTolen,
        IN LPWSAOVERLAPPED lpOverlapped,
        IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
        IN LPWSATHREADID lpThreadId,
        OUT INT FAR *lpErrno);

    INT
    WSPSetSockOpt(
        IN SOCKET s,
        IN INT level,
        IN INT optname,
        IN const char FAR *optval,
        IN INT optlen,
        OUT INT FAR *lpErrno);

    INT
    WSPShutdown(
        IN SOCKET s,
        IN INT how,
        OUT INT FAR *lpErrno);

    SOCKET
    WSPSocket(
        IN int af,
        IN int type,
        IN int protocol,
        IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
        IN GROUP g,
        IN DWORD dwFlags,
        OUT INT FAR *lpErrno);

    INT
    WSPStringToAddress(
        IN     LPWSTR AddressString,
        IN     INT AddressFamily,
        IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
        OUT    LPSOCKADDR lpAddress,
        IN OUT LPINT lpAddressLength,
        IN OUT LPINT lpErrno );

    DWORD_PTR
    GetCancelCallPtr();

    VOID
    Reference ();

    VOID
    Dereference ();

private:
    // Destruction should be done using dereferencing
    ~DPROVIDER();

    // Variables
    LONG             m_reference_count;
    HINSTANCE        m_library_handle;
    WSPPROC_TABLE    m_proctable;
#ifdef DEBUG_TRACING
    LPSTR            m_lib_name;
#endif
};

inline
VOID
DPROVIDER::Reference () {
    //
    // Object is created with reference count of 1
    // and is destroyed whenever it gets back to 0.
    //
    assert (m_reference_count>0);
    InterlockedIncrement (&m_reference_count);
}


inline
VOID
DPROVIDER::Dereference () {
    assert (m_reference_count>0);
    if (InterlockedDecrement (&m_reference_count)==0)
        delete this;
}

inline
DWORD_PTR
DPROVIDER::GetCancelCallPtr()
{
    return((DWORD_PTR)m_proctable.lpWSPCancelBlockingCall);
}



inline SOCKET
DPROVIDER::WSPAccept(
    IN SOCKET s,
    OUT struct sockaddr FAR *addr,
    OUT INT FAR *addrlen,
    IN LPCONDITIONPROC lpfnCondition,
    IN DWORD_PTR dwCallbackData,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Conditionally  accept a connection based on the return value of a condition
    function, and optionally create and/or join a socket group.

Arguments:

    s              - A  descriptor  identiying  a socket which is listening for
                     connections after a WSPListen().

    addr           - An optional pointer to a buffer which receives the address
                     of   the  connecting  entity,  as  known  to  the  service
                     provider.   The  exact  format  of  the  addr arguement is
                     determined  by  the  address  family  established when the
                     socket was created.

    addrlen        - An  optional  pointer  to  an  integer  which contains the
                     length of the address addr.

    lpfnCondition  - The  procedure  instance address of an optional, WinSock 2
                     client  supplied  condition  function  which  will make an
                     accept/reject  decision  based  on  the caller information
                     passed  in  as  parameters,  and optionally creaetd and/or
                     join  a  socket group by assigning an appropriate value to
                     the result parameter of this function.

    dwCallbackData - Callback data to be passed back to the WinSock 2 client as
                     a  condition  function  parameter.   This parameter is not
                     interpreted by the service provider.

    lpErrno        - A pointer to the error code.

Return Value:

    If  no  error occurs, WSPAccept() returns a value of type SOCKET which is a
    descriptor  for  the accepted socket.  Otherwise, a value of INVALID_SOCKET
    is returned, and a specific error code is available in lpErrno.

--*/
{
    SOCKET ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPAccept,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &addr,
                       &addrlen,
                       &lpfnCondition,
                       &dwCallbackData,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPAccept(
        s,
        addr,
        addrlen,
        lpfnCondition,
        dwCallbackData,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPAccept,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &addr,
                    &addrlen,
                    &lpfnCondition,
                    &dwCallbackData,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}




inline INT
DPROVIDER::WSPAddressToString(
    IN     LPSOCKADDR lpsaAddress,
    IN     DWORD dwAddressLength,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT    LPWSTR lpszAddressString,
    IN OUT LPDWORD lpdwAddressStringLength,
    OUT    LPINT lpErrno )
/*++

Routine Description:

    WSPAddressToString() converts a SOCKADDR structure into a human-readable
    string representation of the address.  This is intended to be used mainly
    for display purposes. If the caller wishes the translation to be done by a
    particular provider, it should supply the corresponding WSAPROTOCOL_INFOW
    struct in the lpProtocolInfo parameter.

Arguments:

    lpsaAddress - points to a SOCKADDR structure to translate into a string.

    dwAddressLength - the length of the Address SOCKADDR.

    lpProtocolInfo - (optional) the WSAPROTOCOL_INFOW struct for a particular
                     provider.

    lpszAddressString - a buffer which receives the human-readable address
                        string.

    lpdwAddressStringLength - on input, the length of the AddressString buffer.
                              On output, returns the length of  the string
                              actually copied into the buffer.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned
--*/
{
     INT ReturnValue;

     assert (m_reference_count>0);
     if (PREAPINOTIFY(( DTCODE_WSPAddressToString,
                        &ReturnValue,
                        m_lib_name,
                        &lpsaAddress,
                        &dwAddressLength,
                        &lpProtocolInfo,
                        &lpszAddressString,
                        &lpdwAddressStringLength,
                        &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPAddressToString(
        lpsaAddress,
        dwAddressLength,
        lpProtocolInfo,
        lpszAddressString,
        lpdwAddressStringLength,
        lpErrno);


    POSTAPINOTIFY(( DTCODE_WSPAddressToString,
                    &ReturnValue,
                    m_lib_name,
                    &lpsaAddress,
                    &dwAddressLength,
                    &lpProtocolInfo,
                    &lpszAddressString,
                    &lpdwAddressStringLength,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}





inline INT
DPROVIDER::WSPAsyncSelect(
    IN SOCKET s,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN long lEvent,
    OUT INT FAR *lpErrno
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

    lpErrno - A pointer to the error code.

Return Value:

    The  return  value  is 0 if the WinSock client's declaration of interest in
    the  netowrk event set was successful.  Otherwise the value SOCKET_ERROR is
    returned, and a specific error code is available in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPAsyncSelect,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &hWnd,
                       &wMsg,
                       &lEvent,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPAsyncSelect(
        s,
        hWnd,
        wMsg,
        lEvent,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPAsyncSelect,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &hWnd,
                    &wMsg,
                    &lEvent,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPBind(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN INT namelen,
    OUT INT FAR *lpErrno
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

    lpErrno - A pointer to the error code.

Return Value:

    If   no   erro   occurs,  WSPBind()  returns  0.   Otherwise, it  returns
    SOCKET_ERROR, and a specific error code is available in lpErrno.

--*/
{
    INT ReturnValue;
    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPBind,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &name,
                       &namelen,
                       &lpErrno)) ) {

        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPBind(
        s,
        name,
        namelen,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPBind,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &name,
                    &namelen,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPCancelBlockingCall(OUT INT FAR *lpErrno)
/*++
Routine Description:

    Cancel a blocking call which is currently in progress.

Arguments:

    lpErrno - A pointer to the error code.

Return Value:

    The  value  returned  by  WSPCancelBlockingCall() is 0 if the operation was
    successfully canceled.  Otherwise the value SOCKET_ERROR is returned,
and a
    specific error code is available in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPCancelBlockingCall,
                       &ReturnValue,
                       m_lib_name,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPCancelBlockingCall(
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPCancelBlockingCall,
                    &ReturnValue,
                    m_lib_name,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPCloseSocket(
    IN SOCKET s,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Close a socket.

Arguments:

    s       - A descriptor identifying a socket.

    lpErrno - A pointer to the error code.

Return Value:

    If  no  erro  occurs, WSPCloseSocket()  returns  0.  Otherwise, a value of
    SOCKET_ERROR  is  returned,  and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPCloseSocket,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPCloseSocket(
        s,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPCloseSocket,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPConnect(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN INT namelen,
    IN LPWSABUF lpCallerData,
    IN LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Establish a connection to a peer,
exchange connect data,
and specify needed
    quality of service based on the supplied flow spec.

Arguments:

    s            - A descriptor identifying an unconnected socket.

    name         - The name of the peer to which the socket is to be connected.

    namelen      - The length of the name.

    lpCallerData - A  pointer to the user data that is to be transferred to the
                   peer during connection established.

    lpCalleeData - A pointer to a buffer into which may be copied any user data
                   received from the peer during connection establishment.

    lpSQOS       - A  pointer  to  the  flow  specs  for socket s, one for each
                   direction.

    lpGQOS       - A  pointer  to  the  flow  specs  for  the  socket group (if
                   applicable).

    lpErrno      - A pointer to the error code.

Return Value:

    If  no  error  occurs, WSPConnect()  returns ERROR_SUCCESS.  Otherwise, it
    returns SOCKET_ERROR, and a specific erro rcode is available in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPConnect,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &name,
                       &namelen,
                       &lpCallerData,
                       &lpCalleeData,
                       &lpSQOS,
                       &lpGQOS,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPConnect(
        s,
        name,
        namelen,
        lpCallerData,
        lpCalleeData,
        lpSQOS,
        lpGQOS,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPConnect,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &name,
                    &namelen,
                    &lpCallerData,
                    &lpCalleeData,
                    &lpSQOS,
                    &lpGQOS,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPDuplicateSocket(
    IN SOCKET s,
    IN DWORD dwProcessID,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    descriptor for a shared socket.


Arguments:

    s              - Specifies the local socket descriptor.

    dwProcessID    - Specifies  the  ID  of  the  target  process for which the
                     shared socket will be used.

    lpProtocolInfo - A  pointer  to  a  buffer  allocated by the client that is
                     large enough to contain a WSAPROTOCOL_INFOW struct.  The
                     service  provider copies the protocol info struct contents
                     to this buffer.

    lpErrno        - A pointer to the error code

Return Value:

    If  no  error  occurs, WPSDuplicateSocket()  returns zero.  Otherwise, the
    value of SOCKET_ERROR is returned, and a specific error number is available
    in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPDuplicateSocket,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &dwProcessID,
                       &lpProtocolInfo,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPDuplicateSocket(
        s,
        dwProcessID,
        lpProtocolInfo,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPDuplicateSocket,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &dwProcessID,
                    &lpProtocolInfo,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPEnumNetworkEvents(
    IN SOCKET s,
    OUT WSAEVENT hEventObject,
    OUT LPWSANETWORKEVENTS lpNetworkEvents,
    OUT INT FAR *lpErrno
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

    lpErrno         - A pointer to the error code.

Return Value:

    The  return  value  is  ERROR_SUCCESS  if  the  operation  was  successful.
    Otherwise  the  value SOCKET_ERROR is returned, and a specific error number
    is available in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPEnumNetworkEvents,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &hEventObject,
                       &lpNetworkEvents,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPEnumNetworkEvents(
        s,
        hEventObject,
        lpNetworkEvents,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPEnumNetworkEvents,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &hEventObject,
                    &lpNetworkEvents,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPEventSelect(
    IN SOCKET s,
    IN OUT WSAEVENT hEventObject,
    IN long lNetworkEvents,
    OUT INT FAR *lpErrno
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

    lpErrno        - A pointer to the error code.

Return Value:

    The return value is 0 if the WinSock client's specification of the network
    events and the associated event object was successful. Otherwise the value
    SOCKET_ERROR is returned, and a specific error number is available in
    lpErrno

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPEventSelect,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &hEventObject,
                       &lNetworkEvents,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPEventSelect(
        s,
        hEventObject,
        lNetworkEvents,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPEventSelect,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &hEventObject,
                    &lNetworkEvents,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPGetOverlappedResult(
    IN SOCKET s,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPDWORD lpcbTransfer,
    IN BOOL fWait,
    OUT LPDWORD lpdwFlags,
    OUT INT FAR *lpErrno
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

    lpErrno      - A pointer to the error code.

Return Value:

    If WSPGetOverlappedResult() succeeds,the return value is TRUE.  This means
    that the overlapped operation has completed successfully and that the value
    pointed  to  by lpcbTransfer has been updated.  If WSPGetOverlappedResult()
    returns  FALSE,  this  means  that  either the overlapped operation has not
    completed  or  the  overlapped operation completed but with errors, or that
    completion  status  could  not  be  determined due to errors in one or more
    parameters  to  WSPGetOverlappedResult().  On failure, the value pointed to
    by  lpcbTransfer  will  not be updated.  lpErrno indicates the cause of the
    failure (either of WSPGetOverlappedResult() or of the associated overlapped
    operation).

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPGetOverlappedResult,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpOverlapped,
                       &lpcbTransfer,
                       &fWait,
                       &lpdwFlags,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPGetOverlappedResult(
        s,
        lpOverlapped,
        lpcbTransfer,
        fWait,
        lpdwFlags,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPGetOverlappedResult,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpOverlapped,
                    &lpcbTransfer,
                    &fWait,
                    &lpdwFlags,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline INT
DPROVIDER::WSPGetPeerName(
    IN SOCKET s,
    OUT struct sockaddr FAR *name,
    OUT INT FAR *namelen,
    OUT INT FAR *lpErrno
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

    lpErrno - A pointer to the error code.

Return Value:

    If  no  error occurs, WSPGetPeerName() returns ERROR_SUCCESS.  Otherwise, a
    value  of  SOCKET_ERROR is returned, and a specific error code is available
    in lpErrno

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPGetPeerName,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &name,
                       &namelen,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPGetPeerName(
        s,
        name,
        namelen,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPGetPeerName,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &name,
                    &namelen,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline INT
DPROVIDER::WSPGetQOSByName(
    IN SOCKET s,
    IN LPWSABUF lpQOSName,
    IN LPQOS lpQOS,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Initializes a QOS structure based on a named template.

Arguments:

    s         - A descriptor identifying a socket.

    lpQOSName - Specifies the QOS template name.

    lpQOS     - A pointer to the QOS structure to be filled.

    lpErrno   - A pointer to the error code.

Return Value:

    If the function succeeds, the return value is TRUE.  If the function fails,
    the  return  value  is  FALSE, and  a  specific error code is available in
    lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPGetQOSByName,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpQOSName,
                       &lpQOS,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPGetQOSByName(
        s,
        lpQOSName,
        lpQOS,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPGetQOSByName,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpQOSName,
                    &lpQOS,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline INT
DPROVIDER::WSPGetSockName(
    IN SOCKET s,
    OUT struct sockaddr FAR *name,
    OUT INT FAR *namelen,
    OUT INT FAR *lpErrno
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

    lpErrno - A Pointer to the error code.

Return Value:

    If  no  error occurs, WSPGetSockName() returns ERROR_SUCCESS.  Otherwise, a
    value  of  SOCKET_ERROR is returned, and a specific error code is available
    in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPGetSockName,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &name,
                       &namelen,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPGetSockName(
        s,
        name,
        namelen,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPGetSockName,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &name,
                    &namelen,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline INT
DPROVIDER::WSPGetSockOpt(
    IN SOCKET s,
    IN INT level,
    IN INT optname,
    OUT char FAR *optval,
    OUT INT FAR *optlen,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Retrieve a socket option.

Arguments:

    s       - A descriptor identifying a socket.

    level   - The  level  at  which the option is defined; the supported levels
              include SOL_SOCKET (See annex for more protocol-specific levels.)

    optname - The socket option for which the value is to be retrieved.

    optval  - A  pointer  to  the  buffer  in which the value for the requested
              option is to be returned.

    optlen  - A pointer to the size of the optval buffer.

    lpErrno - A pointer to the error code.

Return Value:

    If  no  error  occurs,  WSPGetSockOpt()  returns  0.  Otherwise, a value of
    SOCKET_ERROR  is  returned,  and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPGetSockOpt,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &level,
                       &optname,
                       &optval,
                       &optlen,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPGetSockOpt(
        s,
        level,
        optname,
        optval,
        optlen,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPGetSockOpt,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &level,
                    &optname,
                    &optval,
                    &optlen,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}


inline INT
DPROVIDER::WSPIoctl(
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
    OUT INT FAR *lpErrno
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

    lpErrno             - A pointer to the error code.

Return Value:

    If  no error occurs and the operation has completed immediately, WSPIoctl()
    returns  0.   Note  that in this case the completion routine, if specified,
    will  have  already  been  queued.   Otherwise, a value of SOCKET_ERROR is
    returned, and  a  specific  error code is available in lpErrno.  The error
    code  WSA_IO_PENDING  indicates  that  an  overlapped  operation  has  been
    successfully  initiated  and  that  conpletion will be indicated at a later
    time.   Any  other  error  code  indicates that no overlapped operation was
    initiated and no completion indication will occur.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPIoctl,
                       &ReturnValue,
                       m_lib_name,
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
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPIoctl(
        s,
        dwIoControlCode,
        lpvInBuffer,
        cbInBuffer,
        lpvOutBuffer,
        cbOutBuffer,
        lpcbBytesReturned,
        lpOverlapped,
        lpCompletionRoutine,
        lpThreadId,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPIoctl,
                    &ReturnValue,
                    m_lib_name,
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
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline SOCKET
DPROVIDER::WSPJoinLeaf(
    IN SOCKET s,
    IN const struct sockaddr FAR *name,
    IN INT namelen,
    IN LPWSABUF lpCallerData,
    IN LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS,
    IN DWORD dwFlags,
    OUT INT FAR *lpErrno
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

    lpErrno      - A pointer to the error code.

Return Value:

    If no error occurs,
WSPJoinLeaf() returns a value of type SOCKET which is a
    descriptor  for the newly created multipoint socket.  Otherwise,a value of
    INVALID_SOCKET  is  returned, and  a  specific  error code is available in
    lpErrno.

--*/
{
    SOCKET ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPJoinLeaf,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &name,
                       &namelen,
                       &lpCallerData,
                       &lpCalleeData,
                       &lpSQOS,
                       &lpGQOS,
                       &dwFlags,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPJoinLeaf(
        s,
        name,
        namelen,
        lpCallerData,
        lpCalleeData,
        lpSQOS,
        lpGQOS,
        dwFlags,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPJoinLeaf,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &name,
                    &namelen,
                    &lpCallerData,
                    &lpCalleeData,
                    &lpSQOS,
                    &lpGQOS,
                    &dwFlags,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPListen(
    IN SOCKET s,
    IN INT backlog,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Establish a socket to listen for incoming connections.

Arguments:

    s       - A descriptor identifying a bound,
unconnected socket.

    backlog - The  maximum length to which the queue of pending connections may
              grow.   If  this  value  is  SOMAXCONN,
then the service provider
              should set the backlog to a maximum "reasonable" value.

    lpErrno - A pointer to the error code.

Return Value:

    If  no  error  occurs, WSPListen()  returns  0.   Otherwise, a  value  of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPListen,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &backlog,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPListen(
        s,
        backlog,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPListen,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &backlog,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPRecv(
    IN SOCKET s,
    IN LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    IN LPDWORD lpNumberOfBytesRecvd,
    IN OUT LPDWORD lpFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT INT FAR *lpErrno
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

    lpErrno              - A pointer to the error code.

Return Value:

    If  no  error  occurs  and the receive operation has completed immediately,
    WSPRecv() returns the number of bytes received.  If the connection has been
    closed, it  returns  0.  Note that in this case the completion routine, if
    specified,  will   have  already  been  queued.   Otherwise, a  value  of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.   The  error  code WSA_IO_PENDING indicates that the overlapped an
    operation  has  been  successfully  initiated  and  that completion will be
    indicated  at  a  later  time.   Any  other  error  code  indicates that no
    overlapped  operations  was  initiated  and  no  completion indication will
    occur.
--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPRecv,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesRecvd,
                       &lpFlags,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPRecv(
        s,
        lpBuffers,
        dwBufferCount,
        lpNumberOfBytesRecvd,
        lpFlags,
        lpOverlapped,
        lpCompletionRoutine,
        lpThreadId,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPRecv,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesRecvd,
                    &lpFlags,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline INT
DPROVIDER::WSPRecvDisconnect(
    IN SOCKET s,
    IN LPWSABUF lpInboundDisconnectData,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Terminate  reception  on  a socket, and retrieve the disconnect data if the
    socket is connection-oriented.

Arguments:

    s                       - A descriptor identifying a socket.

    lpInboundDisconnectData - A  pointer to a buffer into which disconnect data
                              is to be copied.

    lpErrno                 - A pointer to the error code.

Return Value:

    If  no error occurs, WSPRecvDisconnect() returns ERROR_SUCCESS.  Otherwise,
    a value of SOCKET_ERROR is returned, and a specific error code is available
    in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPRecvDisconnect,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpInboundDisconnectData,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPRecvDisconnect(
        s,
        lpInboundDisconnectData,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPRecvDisconnect,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpInboundDisconnectData,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPRecvFrom(
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
    OUT INT FAR *lpErrno
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

    lpErrno              - A pointer to the error code.

Return Value:

    If  no  error  occurs  and the receive operation has completed immediately,
    WSPRecvFrom()  returns the number of bytes received.  If the connection has
    been  closed, it returns 0.  Note that in this case the completion routine,
    if  specified  will  have  already  been  queued.   Otherwise,  a  value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.   The  error  code  WSA_IO_PENDING  indicates  that the overlapped
    operation  has  been  successfully  initiated  and  that completion will be
    indicated  at  a  later  time.   Any  other  error  code  indicates that no
    overlapped  operations  was  initiated  and  no  completion indication will
    occur.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPRecvFrom,
                       &ReturnValue,
                       m_lib_name,
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
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPRecvFrom(
        s,
        lpBuffers,
        dwBufferCount,
        lpNumberOfBytesRecvd,
        lpFlags,
        lpFrom,
        lpFromlen,
        lpOverlapped,
        lpCompletionRoutine,
        lpThreadId,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPRecvFrom,
                    &ReturnValue,
                    m_lib_name,
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
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline INT
DPROVIDER::WSPSelect(
    IN INT nfds,
    IN OUT fd_set FAR *readfds,
    IN OUT fd_set FAR *writefds,
    IN OUT fd_set FAR *exceptfds,
    IN const struct timeval FAR *timeout,
    OUT INT FAR *lpErrno
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

    lpErrno   - A pointer to the error code.

Return Value:

    WSPSelect()  returns  the  total  number of descriptors which are ready and
    contained  in  the  fd_set  structures, 0  if  the  time limit expired, or
    SOCKET_ERROR  if an error occurred.  If the return value is SOCKET_ERROR, a
    specific error code is available in lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPSelect,
                       &ReturnValue,
                       m_lib_name,
                       &nfds,
                       &readfds,
                       &writefds,
                       &exceptfds,
                       &timeout,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPSelect(
        nfds,
        readfds,
        writefds,
        exceptfds,
        timeout,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPSelect,
                    &ReturnValue,
                    m_lib_name,
                    &nfds,
                    &readfds,
                    &writefds,
                    &exceptfds,
                    &timeout,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}




inline INT
DPROVIDER::WSPSend(
    IN SOCKET s,
    IN LPWSABUF lpBuffers,
    IN DWORD dwBufferCount,
    IN LPDWORD lpNumberOfBytesSent,
    IN DWORD dwFlags,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine,
    IN LPWSATHREADID lpThreadId,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Send data on a connected socket.

Arguments:

    s                   - A descriptor identifying a connected socket.

    lpBuffers           - A  pointer  to  an  array of WSABUF structures.  Each
                          WSABUF  structure  contains a pointer to a buffer and
                          the length of the buffer.

    dwBufferCount       - The  number  of  WSABUF  structures  in the lpBuffers
                          array.

    lpNumberOfBytesSent - A pointer to the number of bytes sent by this call.

    dwFlags             - Flags.

    lpOverlapped        - A pointer to a WSAOVERLAPPED structure.

    lpCompletionRoutine - A  pointer  to the completion routine called when the
                          send operation has been completed.

    lpThreadId          - A  pointer to a thread ID structure to be used by the
                          provider in a subsequent call to WPUQueueApc().

    lpErrno             - A pointer to the error code.

Return Value:

    If  no  error  occurs  and  the  send  operation has completed immediately,
    WSPSend() returns the number of bytes received.  If the connection has been
    closed,  it  returns  0.  Note that in this case the completion routine, if
    specified, will   have  already  been  queued.   Otherwise, a  value  of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.   The  error  code  WSA_IO_PENDING  indicates  that the overlapped
    operation  has  been  successfully  initiated  and  that completion will be
    indicated  at  a  later  time.   Any  other  error  code  indicates that no
    overlapped operation was initiated and no completion indication will occur.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPSend,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesSent,
                       &dwFlags,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPSend(
        s,
        lpBuffers,
        dwBufferCount,
        lpNumberOfBytesSent,
        dwFlags,
        lpOverlapped,
        lpCompletionRoutine,
        lpThreadId,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPSend,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesSent,
                    &dwFlags,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline INT
DPROVIDER::WSPSendDisconnect(
    IN SOCKET s,
    IN LPWSABUF lpOutboundDisconnectData,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Initiate  termination  of the connection for the socket and send disconnect
    data.

Arguments:

    s                        - A descriptor identifying a socket.

    lpOutboundDisconnectData - A pointer to the outgoing disconnect data.

    lpErrno                  - A pointer to the error code.

Return Value:

    If  no  error occurs, WSPSendDisconnect() returns 0.  Otherwise, a value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPSendDisconnect,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpOutboundDisconnectData,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPSendDisconnect(
        s,
        lpOutboundDisconnectData,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPSendDisconnect,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpOutboundDisconnectData,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPSendTo(
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
    OUT INT FAR *lpErrno
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

    lpErrno             - A pointer to the error code.

Return Value:

    If  no  error  occurs  and the receive operation has completed immediately,
    WSPSendTo()  returns  the  number of bytes received.  If the connection has
    been  closed,it returns 0.  Note that in this case the completion routine,
    if  specified, will  have  already  been  queued.   Otherwise, a value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.   The  error  code  WSA_IO_PENDING  indicates  that the overlapped
    operation  has  been  successfully  initiated  and  that completion will be
    indicated  at  a  later  time.   Any  other  error  code  indicates that no
    overlapped operation was initiated and no completion indication will occur.

--*/


{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPSendTo,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &lpBuffers,
                       &dwBufferCount,
                       &lpNumberOfBytesSent,
                       &dwFlags,
                       &lpTo,
                       &iTolen,
                       &lpOverlapped,
                       &lpCompletionRoutine,
                       &lpThreadId,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPSendTo(
        s,
        lpBuffers,
        dwBufferCount,
        lpNumberOfBytesSent,
        dwFlags,
        lpTo,
        iTolen,
        lpOverlapped,
        lpCompletionRoutine,
        lpThreadId,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPSendTo,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &lpBuffers,
                    &dwBufferCount,
                    &lpNumberOfBytesSent,
                    &dwFlags,
                    &lpTo,
                    &iTolen,
                    &lpOverlapped,
                    &lpCompletionRoutine,
                    &lpThreadId,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}


inline INT
DPROVIDER::WSPSetSockOpt(
    IN SOCKET s,
    IN INT level,
    IN INT optname,
    IN const char FAR *optval,
    IN INT optlen,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Set a socket option.

Arguments:

    s       - A descriptor identifying a socket.

    level   - The  level  at  which the option is defined; the supported levels
              include   SOL_SOCKET.   (See  annex  for  more  protocol-specific
              levels.)

    optname - The socket option for which the value is to be set.

    optval  - A  pointer  to  the  buffer  in which the value for the requested
              option is supplied.

    optlen  - The size of the optval buffer.

    lpErrno - A pointer to the error code.

Return Value:

    If  no  error  occurs, WSPSetSockOpt()  returns  0.  Otherwise, a value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPSetSockOpt,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &level,
                       &optname,
                       &optval,
                       &optlen,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPSetSockOpt(
        s,
        level,
        optname,
        optval,
        optlen,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPSetSockOpt,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &level,
                    &optname,
                    &optval,
                    &optlen,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);

}



inline INT
DPROVIDER::WSPShutdown(
    IN SOCKET s,
    IN INT how,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Disable sends and/or receives on a socket.

Arguments:

    s       - A descriptor identifying a socket.

    how     - A  flag  that describes what types of operation will no longer be
              allowed.

    lpErrno - A pointer to the error code.

Return Value:

    If  no  error  occurs, WSPShutdown()  returns  0.   Otherwise, a value of
    SOCKET_ERROR  is  returned, and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPShutdown,
                       &ReturnValue,
                       m_lib_name,
                       &s,
                       &how,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPShutdown(
        s,
        how,
        lpErrno);

    POSTAPINOTIFY(( DTCODE_WSPShutdown,
                    &ReturnValue,
                    m_lib_name,
                    &s,
                    &how,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}



inline SOCKET
DPROVIDER::WSPSocket(
    IN int af,
    IN int type,
    IN int protocol,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN GROUP g,
    IN DWORD dwFlags,
    OUT INT FAR *lpErrno
    )
/*++
Routine Description:

    Initialize  internal  data  and  prepare sockets for usage.  Must be called
    before any other socket routine.

Arguments:

    lpProtocolInfo - Supplies  a pointer to a WSAPROTOCOL_INFOW struct that
                     defines  the characteristics of the socket to be created.

    g              - Supplies  the identifier of the socket group which the new
                     socket is to join.

    dwFlags        - Supplies the socket attribute specification.

    lpErrno        - Returns the error code

Return Value:

    WSPSocket() returns zero if successful.  Otherwise it returns an error code
    as outlined in the SPI.

--*/
{
    SOCKET ReturnValue;

    assert (m_reference_count>0);
    // Debug/Trace stuff
    if (PREAPINOTIFY(( DTCODE_WSPSocket,
                       &ReturnValue,
                       m_lib_name,
                       &af,
                       &type,
                       &protocol,
                       &lpProtocolInfo,
                       &g,
                       &dwFlags,
                       &lpErrno)) ) {
        return(ReturnValue);
    }

    // Actual code...
    ReturnValue = m_proctable.lpWSPSocket(
        af,
        type,
        protocol,
        lpProtocolInfo,
        g,
        dwFlags,
        lpErrno);


    // Debug/Trace stuff
    POSTAPINOTIFY(( DTCODE_WSPSocket,
                    &ReturnValue,
                    m_lib_name,
                    &af,
                    &type,
                    &protocol,
                    &lpProtocolInfo,
                    &g,
                    &dwFlags,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}




inline INT
DPROVIDER::WSPStringToAddress(
    IN     LPWSTR AddressString,
    IN     INT AddressFamily,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    OUT    LPSOCKADDR lpAddress,
    IN OUT LPINT lpAddressLength,
    IN OUT LPINT lpErrno )
/*++

Routine Description:

    WSPStringToAddress() converts a human-readable string to a socket address
    structure (SOCKADDR) suitable for pass to Windows Sockets routines which
    take such a structure.  If the caller wishes the translation to be done by
    a particular provider, it should supply the corresponding WSAPROTOCOL_INFOW
    struct in the lpProtocolInfo parameter.

Arguments:

    AddressString - points to the zero-terminated human-readable string to
                    convert.

    AddressFamily - the address family to which the string belongs.

    lpProtocolInfo - (optional) the WSAPROTOCOL_INFOW struct for a particular
                     provider.

    Address - a buffer which is filled with a single SOCKADDR structure.

    lpAddressLength - The length of the Address buffer.  Returns the size of
                      the resultant SOCKADDR structure.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_WSPStringToAddress,
                        &ReturnValue,
                        m_lib_name,
                        &AddressString,
                        &AddressFamily,
                        &lpProtocolInfo,
                        &lpAddress,
                        &lpAddressLength,
                        &lpErrno)) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.lpWSPStringToAddress(
        AddressString,
        AddressFamily,
        lpProtocolInfo,
        lpAddress,
        lpAddressLength,
        lpErrno);


    POSTAPINOTIFY(( DTCODE_WSPStringToAddress,
                    &ReturnValue,
                    m_lib_name,
                    &AddressString,
                    &AddressFamily,
                    &lpProtocolInfo,
                    &lpAddress,
                    &lpAddressLength,
                    &lpErrno));

    assert (m_reference_count>0);
    return(ReturnValue);
}

#endif


