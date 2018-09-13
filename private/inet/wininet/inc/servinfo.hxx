/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    servinfo.hxx

Abstract:

    Contains the CServerInfo class declaration

Author:

    Richard L Firth (rfirth) 02-Oct-1996

Notes:

    In this implementation, we maintain a single host name per server

Revision History:

    02-Oct-1996 rfirth
        Created

--*/

//
// manifests
//

#define SERVER_INFO_SIGNATURE   'fIvS'
#define UNLIMITED_CONNECTIONS   0xffffffff

//
// values for PurgeKeepAlives dwForce parameter, method & function
//

#define PKA_NO_FORCE    0
#define PKA_NOW         1
#define PKA_AUTH_FAILED 2

//
// data (forward references)
//

extern SERIALIZED_LIST GlobalServerInfoList;
extern DWORD GlobalServerInfoTimeout;

//
// forward references
//

class CFsm_GetConnection;
class HTTP_REQUEST_HANDLE_OBJECT;

//
// classes
//

//
// CServerInfo - we maintain an aged list of CServerInfo's. These are used as a
// database of all useful information about a server
//

class CServerInfo {

private:

    //
    // m_List - CServerInfo's are kept in a list
    //

    LIST_ENTRY m_List;

    //
    // m_Expires - system tick count at which this entry will expire
    //

    LONG m_Expires;

    //
    // m_ReferenceCount - number of other structures referencing this
    //

    LONG m_ReferenceCount;

    //
    // m_HostName - primary host name of the server. Stored as LOWER CASE so
    // that we don't have to perform case insensitive string comparisons each
    // time
    //

    ICSTRING m_HostName;

    //
    // m_Hash - hash value of m_HostName. Check this value first
    //

    DWORD m_Hash;

    //
    // m_ProxyLink - if this is an origin server, we link to the proxy
    //   that may carry
    //

    CServerInfo * m_ProxyLink;

    //
    // m_dwProxyVersion - a copy of the Global Proxy Version Count,
    //   the proxy link goes bad if version count changes
    //

    DWORD m_dwProxyVersion;

    //
    // m_HostScheme - Not used for direct connections,
    //   only used for matching proxy info with cached information
    //
    
    INTERNET_SCHEME m_HostScheme;

    //
    // m_HostPort - can also be, proxy port for cached proxy server
    //

    INTERNET_PORT m_HostPort;

private:             

    //
    // m_Services - bitmap of services supported by the server
    //

    union {
        struct {
            unsigned HTTP       : 1;    // HTTP server running at host
            unsigned FTP        : 1;    // FTP     "      "     "   "
            unsigned Gopher     : 1;    // gopher  "      "     "   "
            unsigned GopherPlus : 1;    // gopher+ "      "     "   "
            unsigned NNTP       : 1;    // news    "      "     "   " (future)
            unsigned SMTP       : 1;    // mail    "      "     "   " (future)
            unsigned CERN_Proxy : 1;    // server is running CERN proxy
            unsigned FTP_Proxy  : 1;    // server is running (TIS) FTP proxy/gateway
            unsigned Socks_Proxy: 1;    // server is a Socks Gateway
        } Bits;
        unsigned Word;
    } m_Services;

    //
    // m_HttpSupport - bitmap of HTTP properties supported by the server, such
    // as HTTP version, keep-alive, SSL/PCT, etc.
    //

    union {
        struct {            
            unsigned KeepAlive  : 1;    // HTTP server supports keep-alive
            unsigned Http1_0    : 1;    //  "      "       IS   HTTP/1.0
            unsigned Http1_1    : 1;    //  "      "       "    HTTP/1.1
            unsigned SSL        : 1;    // HTTP server supports SSL
            unsigned PCT        : 1;    // HTTP server supports PCT
            unsigned IIS        : 1;    // HTTP server is MS IIS (could be running others?)
            unsigned BadNS      : 1;    // HTTP server is BAD NS Enterprise server
        } Bits;
        unsigned Word;
    } m_HttpSupport;

    //
    // m_Flags - collection of boolean flags
    //

    union {
        struct {
            unsigned KA_Init    : 1;    // KeepAliveListInitialized
            unsigned Unreachable: 1;    // host (currently) unreachable
            unsigned ProxyByPassSet: 1; // has the next bit been set.
            unsigned ProxyByPassed: 1;  // did we bypass the proxy talking to this server.
            unsigned ProxyScriptCached: 1; // did/are we using this entry to cache the result of JS proxy resolution
        } Bits;
        unsigned Word;
    } m_Flags;

    //
    // m_KeepAliveList - if the server supports keep-alive, a list of the
    // keep-alive connections
    //

    SERIALIZED_LIST m_KeepAliveList;

    //
    // m_PipelinedList - list of HTTP 1.1 pipelined connections
    //

    SERIALIZED_LIST m_PipelinedList;

    //
    // m_Waiters - list of sync/async waiters for connections
    //

    CPriorityList m_Waiters;

    //
    // CConnectionWaiter - private class identifying waiters in m_Waiters list
    //

    class CConnectionWaiter : public CPriorityListEntry {

    private:

        DWORD m_Sync : 1;
        DWORD m_KeepAlive : 1;
        DWORD_PTR m_dwId;
        HANDLE m_hEvent;

    public:

        CConnectionWaiter(
            IN CPriorityList *pList,
            IN BOOL      bSync,
            IN BOOL      bKeepAlive,
            IN DWORD_PTR dwId,
            IN HANDLE    hEvent,
            IN LONG lPriority) : CPriorityListEntry(lPriority) {

            m_Sync = bSync ? 1 : 0;
            m_KeepAlive = bKeepAlive ? 1 : 0;
            m_dwId = dwId;
            m_hEvent = hEvent;
            pList->Insert(this);
        }

        ~CConnectionWaiter() {
        }

        BOOL IsSync(VOID) {
            return (m_Sync == 1) ? TRUE : FALSE;
        }

        BOOL IsKeepAlive(VOID) {
            return (m_KeepAlive == 1) ? TRUE : FALSE;
        }

        DWORD_PTR Id(VOID) {
            return m_dwId;
        }

        VOID Signal(VOID) {
            SetEvent(m_hEvent);
        }
    };

    //
    // m_ConnectionLimit - number of simultaneous connections allowed to this
    // server
    //

    LONG m_ConnectionLimit;

    //
    // m_NewLimit - set when we need to change limit
    //

    LONG m_NewLimit;

    //
    // m_ConnectionsAvailable - number of connections available to this server.
    // If <= 0 (and not unlimited connections) then we have to wait until a
    // connection is freed
    //

    LONG m_ConnectionsAvailable;

    //
    // m_ActiveConnections - number of connections that are active. An active
    // connection is connected and sending or receiving data. This value can be
    // greater than m_ConnectionLimit if we are in the situation of being run
    // out of connections
    //

    //LONG m_ActiveConnections;

    //
    // m_LastActiveTime - timestamp (tick count) of last operation was made on
    // any connection to this server
    //

    DWORD m_LastActiveTime;

    //
    // m_ConnectTime - the average time to connect in milliseconds
    //

    DWORD m_ConnectTime;

    //
    // m_RTT - average Round Trip Time
    //

    DWORD m_RTT;

    //
    // m_DownloadRate - the average download rate in bytes per second
    //

    DWORD m_DownloadRate;

    //
    // m_UploadRate - the average upload rate in bytes per second. Will be
    // different from m_DownloadRate if using ADSL e.g.
    //

    DWORD m_UploadRate;

    //
    // m_AddressList - list of resolved addresses for this server
    //

    CAddressList m_AddressList;

    //
    // m_dwError - error code (mainly for constructor)
    //

    DWORD m_dwError;

#if INET_DEBUG

    DWORD m_Signature;

#define INIT_SERVER_INFO()  m_Signature = SERVER_INFO_SIGNATURE
#define CHECK_SERVER_INFO() INET_ASSERT(m_Signature == SERVER_INFO_SIGNATURE)

#else

#define INIT_SERVER_INFO()  /* NOTHING */
#define CHECK_SERVER_INFO() /* NOTHING */

#endif

    //
    // private methods
    //

    ICSocket *
    FindKeepAliveConnection(
        IN DWORD dwSocketFlags,
        IN INTERNET_PORT nPort
        );

    BOOL
    KeepAliveWaiters(
        VOID
        );

    BOOL
    RunOutOfConnections(
        VOID
        );

    VOID
    UpdateConnectionLimit(
        VOID
        );

public:

    CServerInfo(
        IN LPSTR lpszHostName,
        IN DWORD dwService = INTERNET_SERVICE_HTTP,
        IN DWORD dwMaxConnections = GlobalMaxConnectionsPerServer
        );

    ~CServerInfo();

    CServerInfo * Next(VOID) {
        return (CServerInfo *)m_List.Flink;
    }

    CServerInfo * Prev(VOID) {
        return (CServerInfo *)m_List.Blink;
    }

    VOID SetExpiryTime(DWORD dwMilliseconds = GlobalServerInfoTimeout) {
        m_Expires = GetTickCount() + dwMilliseconds;
    }

    VOID ResetExpiryTime(VOID) {
        m_Expires = 0;
    }

    BOOL Expired(LONG dwTime = (LONG)GetTickCount()) {
        return (dwTime > m_Expires) ? TRUE : FALSE;
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

    LPSTR GetHostName(VOID) const {
        return m_HostName.StringAddress();
    }

    DWORD    
    SetCachedProxyServerInfo(
        IN CServerInfo * pProxyServer,
        IN DWORD dwProxyVersion,
        IN BOOL fUseProxy,
        IN INTERNET_SCHEME HostScheme,
        IN INTERNET_PORT HostPort,
        IN INTERNET_SCHEME ProxyScheme,
        IN INTERNET_PORT ProxyPort
        );

    CServerInfo * 
    GetCachedProxyServerInfo(
        IN INTERNET_SCHEME HostScheme,
        IN INTERNET_PORT HostPort,
        OUT BOOL *pfCachedEntry
        );

    BOOL
    CopyCachedProxyInfoToProxyMsg(
        IN OUT AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo
        );

    BOOL Match(IN DWORD dwHash, IN LPSTR lpszHostName) {
        return (dwHash == m_Hash) ? m_HostName.Strcmp(lpszHostName) : FALSE;
    }

    VOID SetHTTP(VOID) {
        m_Services.Bits.HTTP = 1;
    }

    BOOL IsHTTP(VOID) {
        return m_Services.Bits.HTTP ? TRUE : FALSE;
    }

    VOID SetFTP(VOID) {
        m_Services.Bits.FTP = 1;
    }

    BOOL IsFTP(VOID) {
        return m_Services.Bits.FTP ? TRUE : FALSE;
    }

    VOID SetGopher(VOID) {
        m_Services.Bits.Gopher = 1;
    }

    BOOL IsGopher(VOID) {
        return m_Services.Bits.Gopher ? TRUE : FALSE;
    }

    VOID SetSocksGateway(VOID) {
        m_Services.Bits.Socks_Proxy = 1;
    }

    BOOL IsSocksGateway(VOID) {
        return m_Services.Bits.Socks_Proxy ? TRUE : FALSE;
    }

    VOID SetCernProxy(VOID) {
        m_Services.Bits.CERN_Proxy = 1;
    }

    VOID SetFTPProxy(VOID) {
        m_Services.Bits.FTP_Proxy = 1;
    }

    BOOL IsCernProxy(VOID) {
        return m_Services.Bits.CERN_Proxy ? TRUE : FALSE;
    }

    VOID SetHttp1_1(VOID) {
        m_HttpSupport.Bits.Http1_1 = 1;
    }

    BOOL IsHttp1_1(VOID) {
        return m_HttpSupport.Bits.Http1_1 ? TRUE : FALSE;
    }

    VOID SetHttp1_0(VOID) {
        m_HttpSupport.Bits.Http1_0 = 1;
    }

    VOID SetBadNSServer(VOID) {
        m_HttpSupport.Bits.BadNS = 1;
    }

    BOOL IsBadNSServer(VOID) {
        return m_HttpSupport.Bits.BadNS ? TRUE : FALSE;
    }

    BOOL IsHttp1_0(VOID) {
        return m_HttpSupport.Bits.Http1_0 ? TRUE : FALSE;
    }

    VOID SetKeepAlive(VOID) {
        m_HttpSupport.Bits.KeepAlive = 1;
    }

    BOOL IsKeepAlive(VOID) {
        return m_HttpSupport.Bits.KeepAlive ? TRUE : FALSE;
    }
    
    VOID SetProxyScriptCached(BOOL fSetCached) {
        m_Flags.Bits.ProxyScriptCached = (fSetCached) ? 1 : 0;
    }

    BOOL IsProxyScriptCached(VOID) {
        return m_Flags.Bits.ProxyScriptCached ? TRUE : FALSE;
    }


    VOID SetKeepAliveListInitialized(VOID) {
        m_Flags.Bits.KA_Init = 1;
    }

    BOOL IsKeepAliveListInitialized(VOID) {
        return m_Flags.Bits.KA_Init ? TRUE : FALSE;
    }

    VOID SetUnreachable(VOID) {
        m_Flags.Bits.Unreachable = 1;
    }

    VOID SetReachable(VOID) {
        m_Flags.Bits.Unreachable = 0;
    }

    BOOL IsUnreachable(VOID) {
        return m_Flags.Bits.Unreachable ? TRUE : FALSE;
    }


    VOID SetProxyByPassed(BOOL fProxyByPassed) {
        m_Flags.Bits.ProxyByPassed = fProxyByPassed ? TRUE: FALSE;
        m_Flags.Bits.ProxyByPassSet = TRUE;
    }

    BOOL IsProxyByPassSet(VOID)
    {
        return m_Flags.Bits.ProxyByPassSet ? TRUE : FALSE;
    }

    BOOL WasProxyByPassed(VOID) {
        return m_Flags.Bits.ProxyByPassed ? TRUE : FALSE;
    }

    LONG ConnectionLimit(VOID) const {
        return m_ConnectionLimit;
    }

    VOID SetConnectionLimit(LONG Limit) {
        m_ConnectionLimit = Limit;
    }

    BOOL UnlimitedConnections(VOID) {
        return (ConnectionLimit() == UNLIMITED_CONNECTIONS) ? TRUE : FALSE;
    }

    LONG KeepAliveConnections(VOID) {
        return ElementsOnSerializedList(&m_KeepAliveList);
    }

    LONG AvailableConnections(VOID) const {
        return m_ConnectionsAvailable;
    }

    LONG TotalAvailableConnections(VOID) {
        return KeepAliveConnections() + AvailableConnections();
    }

    LONG GetNewLimit(VOID) const {
        return m_NewLimit;
    }

    VOID SetNewLimit(LONG Limit) {
        m_NewLimit = Limit;
    }

    BOOL IsNewLimit(VOID) {
        return (m_ConnectionLimit != m_NewLimit) ? TRUE : FALSE;
    }

    VOID
    UpdateConnectTime(
        IN DWORD dwConnectTime
        );

    DWORD GetConnectTime(VOID) const {
        return (m_ConnectTime == (DWORD)-1) ? 0 : m_ConnectTime;
    }

    VOID
    UpdateRTT(
        IN DWORD dwTime
        );

    DWORD GetRTT(VOID) const {
        return m_RTT;
    }

    VOID
    UpdateDownloadRate(
        IN DWORD dwBytesPerSecond
        );

    DWORD GetDownloadRate(VOID) const {
        return m_DownloadRate;
    }

    VOID
    UpdateUploadRate(
        IN DWORD dwBytesPerSecond
        );

    DWORD GetUploadRate(VOID) const {
        return m_UploadRate;
    }

    //DWORD
    //GetConnection(
    //    IN DWORD dwSocketFlags,
    //    IN INTERNET_PORT nPort,
    //    IN DWORD dwTimeout,
    //    OUT ICSocket * * lplpSocket
    //    );

    DWORD
    GetConnection_Fsm(
        IN CFsm_GetConnection * Fsm
        );

    DWORD
    ReleaseConnection(
        IN ICSocket * lpSocket OPTIONAL
        );

    VOID
    RemoveWaiter(
        IN DWORD_PTR dwId
        );

    VOID
    PurgeKeepAlives(
        IN DWORD dwForce = PKA_NO_FORCE
        );

    BOOL
    GetNextAddress(
        IN OUT LPDWORD lpdwResolutionId,
        IN OUT LPDWORD lpdwIndex,
        IN INTERNET_PORT nPort,
        OUT LPCSADDR_INFO lpAddress
        ) {
        return m_AddressList.GetNextAddress(lpdwResolutionId,
                                            lpdwIndex,
                                            nPort,
                                            lpAddress
                                            );
    }

    VOID
    InvalidateAddress(
        IN DWORD dwResolutionId,
        IN DWORD dwAddressIndex
        ) {
        m_AddressList.InvalidateAddress(dwResolutionId, dwAddressIndex);
    }

    DWORD
    ResolveHost(
        IN OUT LPDWORD lpdwResolutionId,
        IN DWORD dwFlags
        ) {
        return m_AddressList.ResolveHost(GetHostName(),
                                         lpdwResolutionId,
                                         dwFlags
                                         );
    }

    DWORD GetError(VOID) const {
        return m_dwError;
    }

    VOID SetError(DWORD dwError = GetLastError()) {
        m_dwError = dwError;
    }

    //
    // connection activity methods
    //

    //VOID AddActiveConnection(VOID) {
    //    LockSerializedList(&m_Waiters);
    //
    //    INET_ASSERT(m_ActiveConnections >= 0);
    //
    //    ++m_ActiveConnections;
    //    UnlockSerializedList(&m_Waiters);
    //}
    //
    //VOID RemoveActiveConnection(VOID) {
    //    LockSerializedList(&m_Waiters);
    //    --m_ActiveConnections;
    //
    //    INET_ASSERT(m_ActiveConnections >= 0);
    //
    //    UnlockSerializedList(&m_Waiters);
    //}
    //
    //LONG ActiveConnectionCount(VOID) const {
    //
    //    INET_ASSERT(m_ActiveConnections >= 0);
    //
    //    return m_ActiveConnections;
    //}

    VOID SetLastActiveTime(VOID) {
        m_LastActiveTime = GetTickCount();
    }

    VOID ResetLastActiveTime(VOID) {
        m_LastActiveTime = 0;
    }

    DWORD GetLastActiveTime(VOID) const {
        return m_LastActiveTime;
    }

    BOOL ConnectionActivity(VOID) {

        DWORD lastActiveTime = GetLastActiveTime();

        return (lastActiveTime != 0)
                ? (((GetTickCount() - lastActiveTime)
                   <= GlobalConnectionInactiveTimeout) ? TRUE : FALSE)
                : FALSE;
    }

    BOOL AllConnectionsInactive(VOID) {
        //return ((ActiveConnectionCount() >= ConnectionLimit())
        //        && !ConnectionActivity())
        //            ? TRUE
        //            : FALSE;
        return !ConnectionActivity();
    }

    //
    // friend functions
    //

    friend
    CServerInfo *
    ContainingServerInfo(
        IN LPVOID lpAddress
        );
};

//
// prototypes
//

DWORD
GetServerInfo(
    IN LPSTR lpszHostName,
    IN DWORD dwServiceType,
    IN BOOL bDoResolution,
    OUT CServerInfo * * lplpServerInfo
    );

CServerInfo *
FindServerInfo(
    IN LPSTR lpszHostName
    );

CServerInfo *
FindNearestServer(
    VOID
    );

VOID
ReleaseServerInfo(
    IN CServerInfo * lpServerInfo
    );

VOID
PurgeServerInfoList(
    IN BOOL bForce
    );

VOID
PurgeKeepAlives(
    IN DWORD dwForce = PKA_NO_FORCE
    );

DWORD
PingServerInfoList(
    OUT LPBOOL lpbUnreachable
    );

DWORD
LoadServerInfoDatabase(
    VOID
    );

DWORD
SaveServerInfoDatabase(
    VOID
    );

CServerInfo *
ContainingServerInfo(
    IN LPVOID lpAddress
    );
