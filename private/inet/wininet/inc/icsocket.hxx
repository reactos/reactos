/*++

Copyright (c) 1995-1997 Microsoft Corporation

Module Name:

    icsocket.hxx

Abstract:

    Contains types, manifests, prototypes for Internet Socket Class (ICSocket)
    functions and methods (in common\icsocket.cxx)

Author:

    Richard L Firth (rfirth) 24-May-1995

Revision History:

    24-May-1995 rfirth
        Created

    20-March-1996 arthurbi
        Created the CSOCKET class.

    08-Apr-1997 rfirth
        Changed to ICSocket class (Internet CSocket to avoid issues with MFC).
        Base socket implementation. Secure sockets is a derived class in
        ssocket.hxx/.cxx

--*/

//
// manifests
//

#define HOST_INADDR_ANY         0x00000000
#define HOST_INADDR_NONE        0xffffffff
#define HOST_INADDR_LOOPBACK    0x0100007f

//
// common flags for ConnectSocket(), SocketSend(), SocketReceive(),
// SocketDataAvailable()
//

#define SF_ENCRYPT          0x00000001  // encrypt data (send)
#define SF_DECRYPT          0x00000002  // decrypt data (receive)
#define SF_EXPAND           0x00000004  // input buffer can be expanded to fit (receive)
#define SF_COMPRESS         0x00000008  // input buffer can be compressed to fit (receive)
#define SF_RECEIVE_ALL      0x00000010  // loop until buffer full/all data received (receive)
#define SF_INDICATE         0x00000020  // provide status callbacks
#define SF_NON_BLOCKING     0x00000040  // socket is non-blocking
#define SF_WAIT             0x00000080  // wait for data if non-blocking
#define SF_IGNORE_CONNRESET 0x00000100  // SPX_SUPPORT
#define SF_SENDING_DATA     0x00000200  // data is being sent through the socket, errors may now apply.
#define SF_SCH_REDO         0x00000400  // schannel is redone.
#define SF_CONNECTIONLESS   0x00000800  // send/receive datagrams
#define SF_EXPEDITED        0x00001000  // function expected to complete quickly (test with select())
#define SF_AUTHORIZED       0x00002000  // set if we added an authorization header
#define SF_RANDOM           0x00004000  // set if we connect to address list entry chosen at random
#define SF_FORCE            0x00008000  // set if the name must be resolved (ResolveHost)
#define SF_SECURE           0x00010000  // set if this is a secure (SSL/PCT) socket object
#define SF_NO_WAIT          0x00020000  // set if one-shot operation required (Receive)
#define SF_KEEP_ALIVE       0x00040000  // set if connection is keep-alive
#define SF_PIPELINED        0x00080000  // set if connection is pipelined (HTTP 1.1)
#define SF_PERUSER          0x00100000  // set if authorization header is added so response marked per-user in shared cache
#define SF_AUTHENTICATED    0x00200000  // set if authentication was successful on this socket.
#define SF_TUNNEL           0x00400000  // set if this connection is a nested CONNECT directly to a proxy

//
// types
//

//
// SOCKET_BUFFER_ID - which socket buffer we are dealing with
//

typedef enum {
    ReceiveBuffer = SO_RCVBUF,
    SendBuffer = SO_SNDBUF
} SOCKET_BUFFER_ID;

//
// timeout types for SetSocketTimeout
//

#define SEND_TIMEOUT    1
#define RECEIVE_TIMEOUT 0

//
// macros
//

#define IS_VALID_NON_LOOPBACK_IP_ADDRESS(address) \
    (((address) != HOST_INADDR_ANY) \
    && ((address) != HOST_INADDR_NONE) \
    && ((address) != HOST_INADDR_LOOPBACK))

//
// prototypes
//

LPSTR
MapNetAddressToName(
    IN LPSTR lpszAddress,
    OUT LPSTR * lplpszMappedName
    );

//DWORD
//GetServiceAddress(
//    IN LPSTR HostName,
//    IN DWORD Port,
//    OUT LPADDRESS_INFO_LIST AddressList
//    );
//
//BOOL
//IsNetAddress(
//    IN LPSTR lpszAddress
//    );
//
//#if INET_DEBUG
//
//DEBUG_FUNCTION
//VOID
//InitializeAddressList(
//    IN LPADDRESS_INFO_LIST AddressList
//    );
//
//DEBUG_FUNCTION
//VOID
//FreeAddressList(
//    IN LPADDRESS_INFO_LIST AddressList
//    );
//
//DEBUG_FUNCTION
//BOOL
//IsAddressListEmpty(
//    IN LPADDRESS_INFO_LIST AddressList
//    );
//
//#else
//
//#define InitializeAddressList(AddressList) \
//    (AddressList)->AddressCount = 0; \
//    (AddressList)->Addresses = NULL
//
//#define FreeAddressList(AddressList) \
//    if ((AddressList)->AddressCount != 0) { \
//        (AddressList)->Addresses = (LPCSADDR_INFO)FREE_MEMORY((HLOCAL)((AddressList)->Addresses)); \
//        (AddressList)->AddressCount = 0; \
//    }
//
//#define IsAddressListEmpty(AddressList) \
//    (((AddressList)->AddressCount == 0) ? TRUE : FALSE)
//
//#endif // INET_DEBUG
//
//DWORD
//DestinationAddressFromAddressList(
//    IN LPADDRESS_INFO_LIST lpAddressList,
//    IN DWORD dwIndex,
//    OUT LPBYTE lpbDestinationAddress,
//    IN OUT LPDWORD lpdwDestinationAddressLength
//    );
//
//DWORD
//InterfaceAddressFromSocket(
//    IN SOCKET Socket,
//    OUT LPBYTE lpbInterfaceAddress,
//    IN OUT LPDWORD lpdwInterfaceAddressLength
//    );

//
// classes
//

//
// forward references
//

class CFsm_SocketConnect;
class CFsm_SocketSend;
class CFsm_SocketReceive;
class CServerInfo;

//
// ICSocket - abstracts a TCP/IP connection
//

class ICSocket {

protected:

    LIST_ENTRY m_List;      // keep-alive list
    DWORD m_dwTimeout;      // keep-alive expiry
    LONG m_ReferenceCount;
    SOCKET m_Socket;
    DWORD m_dwFlags;
    INTERNET_PORT m_Port;   // needed for keep-alive
    INTERNET_PORT m_SourcePort;
    BOOL m_bAborted;
    DWORD m_SocksAddress;
    INTERNET_PORT m_SocksPort;
    //HINTERNET m_hRequest;

#if INET_DEBUG

#define ICSOCKET_SIGNATURE  0x6b636f53  // "Sock"

    DWORD m_Signature;

#define SIGN_ICSOCKET() \
    m_Signature = ICSOCKET_SIGNATURE

#define CHECK_ICSOCKET() \
    INET_ASSERT((m_Signature == ICSOCKET_SIGNATURE) || (m_Signature == SECURE_SOCKET_SIGNATURE))

#else

#define SIGN_ICSOCKET() \
    /* NOTHING */

#define CHECK_ICSOCKET() \
    /* NOTHING */

#endif

public:

    ICSocket();

    virtual ~ICSocket();

    VOID
    Destroy(
        VOID
        );

    PLIST_ENTRY List(VOID) {
        return &m_List;
    }

    PLIST_ENTRY Next(VOID) {
        return m_List.Flink;
    }

    BOOL IsOnList(VOID) {
        return ((m_List.Flink == NULL) && (m_List.Blink == NULL)) ? FALSE : TRUE;
    }

    VOID
    Reference(
        VOID
        );

    BOOL
    Dereference(
        VOID
        );

    LONG ReferenceCount(VOID) const {
        return m_ReferenceCount;
    }

    BOOL IsValid(VOID) {
        return (m_Socket != INVALID_SOCKET) ? TRUE : FALSE;
    }

    BOOL IsInvalid(VOID) {
        return !IsValid();
    }

    BOOL IsOpen(VOID) {
        return IsValid();
    }

    BOOL IsClosed(VOID) {
        return !IsOpen();
    }

    SOCKET GetSocket(VOID) const {
        return m_Socket;
    }

    VOID SetSocket(SOCKET Socket) {
        m_Socket = Socket;
    }

    BOOL IsNonBlocking(VOID) {
        return (m_dwFlags & SF_NON_BLOCKING) ? TRUE : FALSE;
    }

    BOOL IsSecure(VOID) const {
        return (m_dwFlags & SF_SECURE) ? TRUE : FALSE;
    }

    VOID SetEncryption(VOID) {
        m_dwFlags |= SF_ENCRYPT | SF_DECRYPT;
    }

    VOID ResetEncryption(VOID) {
        m_dwFlags &= ~(SF_ENCRYPT | SF_DECRYPT);
    }

    DWORD GetFlags(VOID) const {
        return m_dwFlags;
    }

    VOID SetAuthorized(VOID) {
        m_dwFlags |= SF_AUTHORIZED;
    }

    BOOL IsAuthorized(VOID) {
        return (m_dwFlags & SF_AUTHORIZED) ? TRUE : FALSE;
    }

    VOID SetAuthenticated(VOID) {
        m_dwFlags |= SF_AUTHENTICATED;
    }

    BOOL IsAuthenticated(VOID) {
        return (m_dwFlags & SF_AUTHENTICATED) ? TRUE : FALSE;
    }

    VOID SetPerUser (VOID) {
        m_dwFlags |= SF_PERUSER;
    }

    BOOL IsPerUser(VOID) {
        return (m_dwFlags & SF_PERUSER) ? TRUE : FALSE;
    }

    VOID SetKeepAlive(VOID) {
        m_dwFlags |= SF_KEEP_ALIVE;
    }

    VOID ResetKeepAlive(VOID) {
        m_dwFlags &= ~SF_KEEP_ALIVE;
    }

    BOOL IsKeepAlive(VOID) {
        return (m_dwFlags & SF_KEEP_ALIVE) ? TRUE : FALSE;
    }

    VOID SetPipelined(VOID) {
        m_dwFlags |= SF_PIPELINED;
    }

    VOID ResetPipelined(VOID) {
        m_dwFlags &= ~SF_PIPELINED;
    }

    BOOL IsPipelined(VOID) {
        return (m_dwFlags & SF_PIPELINED) ? TRUE : FALSE;
    }

    BOOL Match(DWORD dwFlags) {
        return ((m_dwFlags & dwFlags) == dwFlags) ? TRUE : FALSE;
    }

    BOOL MatchTunnelSemantics(DWORD dwFlags) {
        return ((m_dwFlags & SF_TUNNEL) == (dwFlags & SF_TUNNEL)) ? TRUE : FALSE;
    }

    VOID SetPort(INTERNET_PORT Port) {
        m_Port = Port;
    }

    INTERNET_PORT GetPort(VOID) const {
        return m_Port;
    }

    VOID SetSourcePort(VOID);

    VOID SetSourcePort(INTERNET_PORT Port) {
        m_SourcePort = Port;
    }

    INTERNET_PORT GetSourcePort(VOID) const {
        return m_SourcePort;
    }

    VOID SetAborted(VOID) {
        m_bAborted = TRUE;
    }

    BOOL IsAborted(VOID) const {
        return m_bAborted;
    }

    DWORD
    GetServiceAddress(
        IN LPSTR HostName,
        IN DWORD Port
        );

    virtual
    DWORD
    Connect(
        IN LONG Timeout,
        IN INT Retries,
        IN DWORD dwFlags
        );

    DWORD
    SocketConnect(
        IN LONG Timeout,
        IN INT Retries,
        IN DWORD dwFlags,
        IN CServerInfo *pServerInfo
        );

    DWORD
    Connect_Start(
        IN CFsm_SocketConnect * Fsm
        );

    DWORD
    Connect_Continue(
        IN CFsm_SocketConnect * Fsm
        );

    DWORD
    Connect_Error(
        IN CFsm_SocketConnect * Fsm
        );

    DWORD
    Connect_Finish(
        IN CFsm_SocketConnect * Fsm
        );

    int
    SocksConnect(
        IN LPSOCKADDR_IN pSockaddr,
        IN INT nLen
        );

    virtual
    DWORD
    Disconnect(
        IN DWORD dwFlags = 0
        );

    DWORD
    Close(
        VOID
        );

    DWORD
    Abort(
        VOID
        );

    DWORD
    Shutdown(
        IN DWORD dwControl
        );

    BOOL
    IsReset(
        VOID
        );

    virtual
    DWORD
    Send(
        IN LPVOID lpBuffer,
        IN DWORD dwBufferLength,
        IN DWORD dwFlags
        );

    DWORD
    Send_Start(
        IN CFsm_SocketSend * Fsm
        );

    virtual
    DWORD
    Receive(
        IN OUT LPVOID* lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwBufferRemaining,
        IN OUT LPDWORD lpdwBytesReceived,
        IN DWORD dwExtraSpace,
        IN DWORD dwFlags,
        OUT LPBOOL lpbEof
        );

    DWORD
    Receive_Start(
        IN CFsm_SocketReceive * Fsm
        );

    DWORD
    Receive_Continue(
        IN CFsm_SocketReceive * Fsm
        );

    DWORD
    Receive_Finish(
        IN CFsm_SocketReceive * Fsm
        );

    DWORD
    SetTimeout(
        IN DWORD Type,
        IN int Timeout
        );

    DWORD
    SetLinger(
        IN BOOL Linger,
        IN int Timeout
        );

    DWORD
    SetNonBlockingMode(
        IN BOOL bNonBlocking
        );

    DWORD
    GetBufferLength(
        IN SOCKET_BUFFER_ID SocketBufferId
        );

    DWORD
    GetBufferLength(
        IN SOCKET_BUFFER_ID SocketBufferId,
        OUT LPDWORD lpdwBufferLength
        );

    DWORD
    SetBufferLength(
        IN SOCKET_BUFFER_ID SocketBufferId,
        IN DWORD dwBufferLength
        );

    DWORD
    SetSendCoalescing(
        IN BOOL bOnOff
        );

    VOID
    SetExpiryTime(
        IN DWORD dwTimeout = GlobalKeepAliveSocketTimeout
        ) {
        m_dwTimeout = GetTickCount() + dwTimeout;
    }

    DWORD GetExpiryTime(VOID) const {
        return m_dwTimeout;
    }

    BOOL HasExpired(
        IN DWORD dwTime = GetTickCount()
        ) {
        return (m_dwTimeout == 0)
            ? FALSE
            : ((dwTime > m_dwTimeout) ? TRUE : FALSE);
    }

    DWORD
    DataAvailable(
        OUT LPDWORD lpdwDataAvailable
        );

    DWORD
    DataAvailable2(
        OUT LPVOID lpBuffer,
        IN DWORD dwBufferLength,
        OUT LPDWORD lpdwBytesAvailable
        );

    DWORD
    WaitForReceive(
        IN DWORD Timeout
        );

    DWORD
    AllocateQueryBuffer(
        OUT LPVOID * lplpBuffer,
        OUT LPDWORD lpdwBufferLength
        );

    VOID
    FreeQueryBuffer(
        IN LPVOID lpBuffer
        );

    DWORD
    EnableSocks(
        IN LPSTR lpSocksHost,
        IN INTERNET_PORT ipSocksPort
        );

    BOOL IsSocks(VOID) {
        return m_SocksAddress != 0;
    }

    DWORD
    CreateSocket(
        IN DWORD dwFlags,
        IN int nFamily = AF_INET,
        IN int nType = SOCK_STREAM,
        IN int nProtocol = IPPROTO_TCP
        );

    DWORD
    GetSockName(
        PSOCKADDR psaSockName
        );

    DWORD
    Listen(
        VOID
        );

    DWORD
    DirectConnect(
        IN PSOCKADDR psaRemoteSock
        );

    DWORD
    SelectAccept(
        IN ICSocket & acceptSocket,
        IN DWORD dwTimeout
        );

    DWORD
    GetBytesAvailable(
        OUT LPDWORD lpdwBytesAvailable
        );

    //VOID
    //SetServiceAddress(
    //    IN LPADDRESS_INFO_LIST AddressList
    //    )
    //{
    //    m_fOwnAddressList = FALSE;
    //    m_AddressList.Addresses = AddressList->Addresses;
    //    m_AddressList.AddressCount = AddressList->AddressCount;
    //}

    //DWORD
    //GetServiceAddress(
    //    IN LPSTR HostName OPTIONAL,
    //    IN LPSTR ServiceName OPTIONAL,
    //    IN LPGUID ServiceGuid OPTIONAL,
    //    IN DWORD NameSpace,
    //    IN DWORD Port,
    //    IN DWORD ProtocolCharacteristics
    //    )
    //{
    //    return ::GetServiceAddress(
    //                HostName,
    //                ServiceName,
    //                ServiceGuid,
    //                NameSpace,
    //                Port,
    //                ProtocolCharacteristics,
    //                &m_AddressList
    //                );
    //}


    //
    // friend functions
    //

    friend
    ICSocket *
    ContainingICSocket(
        LPVOID lpAddress
        );
};
