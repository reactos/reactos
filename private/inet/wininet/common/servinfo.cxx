/*++

Copyright (c) 1996-1997 Microsoft Corporation

Module Name:

    servinfo.cxx

Abstract:

    Class implementation for global server info list

    Contents:
        GetServerInfo
        FindServerInfo
        FindNearestServer
        ReleaseServerInfo
        PurgeServerInfoList
        PingServerInfoList
        LoadServerInfoDatabase
        SaveServerInfoDatabase
        CServerInfo::CServerInfo
        CServerInfo::~CServerInfo
        CServerInfo::Reference
        CServerInfo::Dereference
        CServerInfo::ResolveHostName
        CServerInfo::UpdateConnectTime
        CServerInfo::UpdateRTT
        CServerInfo::UpdateDownloadRate
        CServerInfo::UpdateUploadRate
        CServerInfo::GetConnection
        CFsm_GetConnection::RunSM
        CServerInfo::GetConnection_Fsm
        CServerInfo::ReleaseConnection
        CServerInfo::RemoveWaiter
        (CServerInfo::FindKeepAliveConnection)
        (CServerInfo::KeepAliveWaiters)
        (CServerInfo::RunOutOfConnections)
        (CServerInfo::UpdateConnectionLimit)
        CServerInfo::PurgeKeepAlives
        ContainingServerInfo

Author:

    Richard L Firth (rfirth) 07-Oct-1996

Revision History:

    07-Oct-1996 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>

//
// private macros
//

//#define CHECK_CONNECTION_COUNT() \
//    INET_ASSERT(!UnlimitedConnections() \
//        ? (TotalAvailableConnections() <= ConnectionLimit()) : TRUE)

#define CHECK_CONNECTION_COUNT()    /* NOTHING */

//#define RLF_DEBUG   1

#if INET_DEBUG
#ifdef RLF_DEBUG
#define DPRINTF dprintf
#else
#define DPRINTF (void)
#endif
#else
#define DPRINTF (void)
#endif

//
// functions
//


DWORD
GetServerInfo(
    IN LPSTR lpszHostName,
    IN DWORD dwServiceType,
    IN BOOL bDoResolution,
    OUT CServerInfo * * lplpServerInfo
    )

/*++

Routine Description:

    Finds or creates a CServerInfo entry

Arguments:

    lpszHostName    - pointer to server name to get info for

    dwServiceType   - type of service for which CServerInfo requested

    bDoResolution   - TRUE if we are to resolve host name

    lplpServerInfo  - pointer to created/found CServerInfo if successful

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't create the CServerInfo

                  ERROR_INTERNET_NAME_NOT_RESOLVED
                    We were asked to resolve the name, but failed

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 Dword,
                 "GetServerInfo",
                 "%q, %s (%d), %B, %#x",
                 lpszHostName,
                 InternetMapService(dwServiceType),
                 dwServiceType,
                 bDoResolution,
                 lplpServerInfo
                 ));

    ICSTRING hostName(lpszHostName);
    CServerInfo * lpServerInfo;
    BOOL bCreated = FALSE;
    DWORD error = ERROR_SUCCESS;

    if (hostName.HaveString()) {
        hostName.MakeLowerCase();

        LPSTR lpszHostNameLower = hostName.StringAddress();

        LockSerializedList(&GlobalServerInfoList);

        lpServerInfo = FindServerInfo(lpszHostNameLower);

        if (lpServerInfo == NULL) {
            lpServerInfo = new CServerInfo(lpszHostNameLower,
                                           dwServiceType
                                           );
            if (lpServerInfo != NULL) {
                bCreated = TRUE;
            } else {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }

        //
        // add a reference to the server info to keep it alive
        //

        if (lpServerInfo != NULL) {
            lpServerInfo->Reference();
        }

        UnlockSerializedList(&GlobalServerInfoList);
    } else {

        //
        // failed to create ICSTRING
        //

        error = GetLastError();

        INET_ASSERT(error != ERROR_SUCCESS);

        lpServerInfo = NULL;
    }

    //
    // if we created a new CServerInfo and we are instructed to resolve the host
    // name then do it now, outside of the global server info list lock. This
    // operation may take some time
    //

    if (bDoResolution && (lpServerInfo != NULL)) {
        //error = lpServerInfo->ResolveHostName();
        if (error != ERROR_SUCCESS) {
            ReleaseServerInfo(lpServerInfo);
            lpServerInfo = NULL;
        }
    }

    *lplpServerInfo = lpServerInfo;

    DEBUG_LEAVE(error);

    return error;
}


CServerInfo *
FindServerInfo(
    IN LPSTR lpszHostName
    )

/*++

Routine Description:

    Walks the server info list looking for the requested server

Arguments:

    lpszHostName    - pointer to server name to find (IN LOWER CASE!)

Return Value:

    CServerInfo *
        Success - pointer to found list entry

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 Pointer,
                 "FindServerInfo",
                 "%q",
                 lpszHostName
                 ));

    DWORD hashHostName = CalculateHashValue(lpszHostName);

    CServerInfo * lpServerInfo;
    BOOL found = FALSE;

    LockSerializedList(&GlobalServerInfoList);

    for (lpServerInfo = (CServerInfo *)HeadOfSerializedList(&GlobalServerInfoList);
        lpServerInfo != (CServerInfo *)SlSelf(&GlobalServerInfoList);
        lpServerInfo = lpServerInfo->Next()) {

        if (lpServerInfo->Match(hashHostName, lpszHostName)) {
            found = TRUE;
            break;
        }
    }

    UnlockSerializedList(&GlobalServerInfoList);

    if (!found) {
        lpServerInfo = NULL;
    }

    DEBUG_LEAVE(lpServerInfo);

    return lpServerInfo;
}


CServerInfo *
FindNearestServer(
    VOID
    )

/*++

Routine Description:

    Returns pointer to the CServerInfo which has the shortest connect time.
    Returned info is referenced, so caller must call ReleaseServerInfo() when
    done

Arguments:

    None.

Return Value:

    CServerInfo *
        Success - valid pointer

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 Pointer,
                 "FindNearestServer",
                 NULL
                 ));

    CServerInfo * lpServerInfo;
    CServerInfo * lpsiResult = NULL;
    DWORD connectTime = (DWORD)-1;

    LockSerializedList(&GlobalServerInfoList);

    for (lpServerInfo = (CServerInfo *)HeadOfSerializedList(&GlobalServerInfoList);
        lpServerInfo != (CServerInfo *)SlSelf(&GlobalServerInfoList);
        lpServerInfo = lpServerInfo->Next()) {

        if (lpServerInfo->GetConnectTime() < connectTime) {
            lpsiResult = lpServerInfo;
            connectTime = lpServerInfo->GetConnectTime();
        }
    }

    UnlockSerializedList(&GlobalServerInfoList);

    DEBUG_LEAVE(lpsiResult);

    return lpsiResult;
}


VOID
ReleaseServerInfo(
    IN CServerInfo * lpServerInfo
    )

/*++

Routine Description:

    Release a CServerInfo by dereferencing it. If the reference count goes to
    zero, the CServerInfo will be destroyed

Arguments:

    lpServerInfo    - pointer to CServerInfo to release

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "ReleaseServerInfo",
                 "%#x [%q]",
                 lpServerInfo,
                 lpServerInfo->GetHostName()
                 ));

    lpServerInfo->Dereference();

    DEBUG_LEAVE(0);
}


VOID
PurgeServerInfoList(
    IN BOOL bForce
    )

/*++

Routine Description:

    Throw out any CServerInfo entries that have expired or any KEEP_ALIVE
    entries (for any CServerInfo) that have expired

Arguments:

    bForce  - TRUE if we forcibly remove entries which have not yet expired but
              which have a reference count of 1, else FALSE to remove only
              entries that have expired

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "PurgeServerInfoList",
                 "%B",
                 bForce
                 ));

    LockSerializedList(&GlobalServerInfoList);

    PLIST_ENTRY pEntry = HeadOfSerializedList(&GlobalServerInfoList);
    PLIST_ENTRY pPrevious = (PLIST_ENTRY)SlSelf(&GlobalServerInfoList);

    while (TRUE) {
        if (pEntry == (PLIST_ENTRY)SlSelf(&GlobalServerInfoList)) {
            break;
        }

        CServerInfo * pServerInfo;

        //pServerInfo = (CServerInfo *)pEntry;
        //pServerInfo = CONTAINING_RECORD(pEntry, CONNECTION_LIMIT, m_List);
        pServerInfo = ContainingServerInfo(pEntry);

        BOOL deleted = FALSE;

        if (pServerInfo->ReferenceCount() == 1) {
            if (bForce || pServerInfo->Expired()) {
//dprintf("purging server info entry for %q\n", pServerInfo->GetHostName());
                deleted = pServerInfo->Dereference();
            } else {
                pServerInfo->PurgeKeepAlives(PKA_NO_FORCE);
            }
        }
        if (!deleted) {
            pPrevious = pEntry;
        }
        pEntry = pPrevious->Flink;
    }

    UnlockSerializedList(&GlobalServerInfoList);

    DEBUG_LEAVE(0);
}


VOID
PurgeKeepAlives(
    IN DWORD dwForce
    )

/*++

Routine Description:

    Throw out any KEEP_ALIVE entries from any CServerInfo that have expired or
    which have failed authentication or which are unused, depending on dwForce

Arguments:

    dwForce - force to apply when purging. Value can be:

                PKA_NO_FORCE    - only purge timed-out sockets or sockets in
                                  close-wait state (default)

                PKA_NOW         - purge all sockets

                PKA_AUTH_FAILED - purge sockets that have been marked as failing
                                  authentication

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "PurgeKeepAlives",
                 "%s [%d]",
                 (dwForce == PKA_NO_FORCE) ? "NO_FORCE"
                 : (dwForce == PKA_NOW) ? "NOW"
                 : (dwForce == PKA_AUTH_FAILED) ? "AUTH_FAILED"
                 : "?",
                 dwForce
                 ));

    LockSerializedList(&GlobalServerInfoList);

    PLIST_ENTRY pEntry = HeadOfSerializedList(&GlobalServerInfoList);

    while (pEntry != (PLIST_ENTRY)SlSelf(&GlobalServerInfoList)) {

        CServerInfo * lpServerInfo = ContainingServerInfo(pEntry);

        lpServerInfo->PurgeKeepAlives(dwForce);
        pEntry = pEntry->Flink;
    }

    UnlockSerializedList(&GlobalServerInfoList);

    DEBUG_LEAVE(0);
}

//
//DWORD
//PingServerInfoList(
//    OUT LPBOOL lpbUnreachable
//    )
//
///*++
//
//Routine Description:
//
//    Determines online/offline state by attempting to ping a known server address.
//    If any ping succeeds, this function succeeds. If all pings fail, then this
//    function fails
//
//    Assumes:    1. global ping object has been instantiated
//
//Arguments:
//
//    lpbUnreachable  - TRUE if one or more servers were unreachable (the network
//                      seems to be alive, just that we couldn't reach one or more
//                      servers. Useful to indicate that an address we are in the
//                      process of resolving is bad
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//                    at least one address pinged
//
//        Failure - ERROR_INTERNET_NO_KNOWN_SERVERS (internal)
//                    There are no known server addresses (there may be items in
//                    the list, but the addresses are not yet resolved)
//
//                  ERROR_INTERNET_PING_FAILED (internal)
//                    We have (resolved) addresses, but couldn't successfully ping
//                    any. We believe we have connectivity
//
//                  ERROR_INTERNET_NO_PING_SUPPORT (internal)
//                    We couldn't ping any addresses because ping support is not
//                    loaded, or globally disabled
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SOCKETS,
//                 Dword,
//                 "PingServerInfoList",
//                 "%#x",
//                 lpbUnreachable
//                 ));
//
//    DWORD error;
//
//    *lpbUnreachable = FALSE;
//
//    CServerInfo * lpServerInfo;
//
//    error = ERROR_INTERNET_NO_KNOWN_SERVERS;
//
//    LockSerializedList(&GlobalServerInfoList);
//
//    for (lpServerInfo = (CServerInfo *)HeadOfSerializedList(&GlobalServerInfoList);
//        lpServerInfo != (CServerInfo *)SlSelf(&GlobalServerInfoList);
//        lpServerInfo = lpServerInfo->Next()) {
//
//        DWORD err;
//        DWORD dwIpAddress;
//        DWORD addressLength = sizeof(dwIpAddress);
//
//        //
//        // just use first address from each address list
//        //
//
//        err = DestinationAddressFromAddressList(lpServerInfo->GetAddressList(),
//                                                0,
//                                                (LPBYTE)&dwIpAddress,
//                                                &addressLength
//                                                );
//
//        INET_ASSERT(IS_VALID_NON_LOOPBACK_IP_ADDRESS(dwIpAddress));
//
//        if ((err == ERROR_SUCCESS)
//        && IS_VALID_NON_LOOPBACK_IP_ADDRESS(dwIpAddress)) {
//            error = Ping(dwIpAddress);
//            if (error == ERROR_SUCCESS) {
//
//                //
//                // ping succeeded, net is alive. All we need to know for now
//                //
//
//                lpServerInfo->SetReachable();
//                break;
//            } else if (error == ERROR_INTERNET_SERVER_UNREACHABLE) {
//
//                //
//                // our equivalent of net unreachable
//                //
//
//                lpServerInfo->SetUnreachable();
//                *lpbUnreachable = TRUE;
//
//                //
//                // although the server is unreachable, we still have
//                // connectivity (the ping would have failed completely
//                // otherwise)
//                //
//
//                error = ERROR_SUCCESS;
//            } else if (error == ERROR_INTERNET_NO_PING_SUPPORT) {
//
//                //
//                // can't ping - no ping support. quit
//                //
//
//                break;
//            }
//        }
//    }
//
//    UnlockSerializedList(&GlobalServerInfoList);
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}
//
//
//DWORD
//LoadServerInfoDatabase(
//    VOID
//    )
//
///*++
//
//Routine Description:
//
//    Populates the server info database from the registry. This allows us to
//    avoid server capability negotiation each time we start IE/Wininet
//
//Arguments:
//
//    None.
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
//    return ERROR_SUCCESS;
//}
//
//
//DWORD
//SaveServerInfoDatabase(
//    VOID
//    )
//
///*++
//
//Routine Description:
//
//    Copies the contents of the current server info database to the registry.
//    This information is read the next time we start IE/Wininet
//
//Arguments:
//
//    None.
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
//    return ERROR_SUCCESS;
//}

//
// methods
//


CServerInfo::CServerInfo(
    IN LPSTR lpszHostName,
    IN DWORD dwService,
    IN DWORD dwMaxConnections
    )

/*++

Routine Description:

    CServerInfo constructor

Arguments:

    lpszHostName        - server for which to create CServerInfo

    dwService           - which service to create CServerInfo for

    dwMaxConnections    - maximum number of simultaneous connections to this
                          server

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "CServerInfo::CServerInfo",
                 "%q, %s (%d), %d",
                 lpszHostName,
                 InternetMapService(dwService),
                 dwService,
                 dwMaxConnections
                 ));

    INIT_SERVER_INFO();

    //GlobalServerInfoAllocCount++;

    InitializeListHead(&m_List);
    m_Expires = 0;
    m_ReferenceCount = 1;
    m_HostName = lpszHostName;
    m_HostName.MakeLowerCase();
    m_Hash = CalculateHashValue(m_HostName.StringAddress());
    m_Services.Word = 0;
    m_HttpSupport.Word = 0;
    m_Flags.Word = 0;
    m_ProxyLink = NULL;

    switch (dwService) {
    case INTERNET_SERVICE_HTTP:
        SetHTTP();
        break;

    case INTERNET_SERVICE_FTP:
        SetFTP();
        break;

    case INTERNET_SERVICE_GOPHER:
        SetGopher();
        break;

    default:

        INET_ASSERT(FALSE);

    }

    //
    // only initialize the keep-alive and connection limit lists if we are
    // creating the server info entry for a HTTP server (or CERN proxy)
    //

    //
    // BUGBUG - we only want to do this on demand
    //

    //if (IsHTTP()) {
        InitializeSerializedList(&m_KeepAliveList);
        InitializeSerializedList(&m_PipelinedList);
        //InitializeSerializedList(&m_Waiters);
        SetKeepAliveListInitialized();

        //
        // the maximum number of connections per server is initialized to the
        // default (registry) value unless overridden by the caller
        //

if (dwMaxConnections == 0) {
    dwMaxConnections = DEFAULT_MAX_CONNECTIONS_PER_SERVER;
}
        m_ConnectionLimit = dwMaxConnections;
    //} else {
    //    m_ConnectionLimit = UNLIMITED_CONNECTIONS;
    //}
//dprintf("*** %s: limit = %d\n", GetHostName(), m_ConnectionLimit);
    //
    // BUGBUG - only create event if limiting connections. Need method to manage
    //          connection limit count/event creation
    //

    m_NewLimit = m_ConnectionLimit;
    m_ConnectionsAvailable = m_ConnectionLimit;
    //m_ActiveConnections = 0;
    m_LastActiveTime = 0;
    m_ConnectTime = (DWORD)-1;
    m_RTT = 0;
    m_DownloadRate = 0;
    m_UploadRate = 0;
    m_dwError = ERROR_SUCCESS;

    //
    // add to the global list. We are assuming here that the caller has already
    // checked for dupes
    //

    InsertAtHeadOfSerializedList(&GlobalServerInfoList, &m_List);

    DEBUG_LEAVE(0);
}


CServerInfo::~CServerInfo()

/*++

Routine Description:

    CServerInfo destructor

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                 None,
                 "CServerInfo::~CServerInfo",
                 "{%q}",
                 GetHostName()
                 ));

    CHECK_SERVER_INFO();

    //GlobalServerInfoDeAllocCount++;

    // unlink if we have a nested obj
    if ( m_ProxyLink ) {
        CServerInfo *pDerefObj;

        LockSerializedList(&GlobalServerInfoList);        
        pDerefObj = m_ProxyLink;
        m_ProxyLink = NULL;
        UnlockSerializedList(&GlobalServerInfoList);        

        if (pDerefObj) {
            pDerefObj->Dereference();
        }
    }


    RemoveFromSerializedList(&GlobalServerInfoList, &m_List);

    INET_ASSERT(m_ReferenceCount == 0);

    if (IsKeepAliveListInitialized()) {
        LockSerializedList(&m_KeepAliveList);
        while (!IsSerializedListEmpty(&m_KeepAliveList)) {
//dprintf("%#x ~S-I killing K-A %#x\n", GetCurrentThreadId(), HeadOfSerializedList(&m_KeepAliveList));

            LPVOID pEntry = SlDequeueHead(&m_KeepAliveList);

            INET_ASSERT(pEntry != NULL);

            if (pEntry != NULL) {

                ICSocket * pSocket = ContainingICSocket(pEntry);

//dprintf("~CServerInfo: destroying socket %#x\n", pSocket->GetSocket());
                pSocket->Destroy();
            }
        }
        UnlockSerializedList(&m_KeepAliveList);
        TerminateSerializedList(&m_KeepAliveList);
        TerminateSerializedList(&m_PipelinedList);
        //TerminateSerializedList(&m_Waiters);
    }

    DEBUG_LEAVE(0);
}


VOID
CServerInfo::Reference(
    VOID
    )

/*++

Routine Description:

    Increments the reference count for the CServerInfo

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::Reference",
                 "{%q}",
                 GetHostName()
                 ));

    CHECK_SERVER_INFO();
    INET_ASSERT(m_ReferenceCount > 0);

    InterlockedIncrement(&m_ReferenceCount);
//dprintf("CServerInfo %s - %d\n", GetHostName(), m_ReferenceCount);

    DEBUG_PRINT(SESSION,
                INFO,
                ("Reference count = %d\n",
                ReferenceCount()
                ));

    DEBUG_LEAVE(0);
}


BOOL
CServerInfo::Dereference(
    VOID
    )

/*++

Routine Description:

    Dereferences the SESSION_INFO. If the reference count goes to zero then this
    entry is deleted. If the reference count goes to 1 then the expiry timer is
    started

Arguments:

    None.

Return Value:

    BOOL
        TRUE    - entry was deleted

        FALSE   - entry was not deleted

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::Dereference",
                 "{%q}",
                 GetHostName()
                 ));

    CHECK_SERVER_INFO();
    INET_ASSERT(m_ReferenceCount > 0);

    //
    // we need to grab the list - we may be removing this entry or updating
    // the reference count and expiry fields which must be done atomically
    //

    LockSerializedList(&GlobalServerInfoList);

    LONG result = InterlockedDecrement(&m_ReferenceCount);
//dprintf("CServerInfo %s - %d\n", GetHostName(), m_ReferenceCount);

    DEBUG_PRINT(SESSION,
                INFO,
                ("Reference count = %d\n",
                ReferenceCount()
                ));

    BOOL deleted = FALSE;

    if (result == 0) {
        delete this;
        deleted = TRUE;
    } else if (result == 1) {

        //
        // start expiration proceedings...
        //

        SetExpiryTime();
    }

    UnlockSerializedList(&GlobalServerInfoList);

    DEBUG_LEAVE(deleted);

    return deleted;
}


DWORD    
CServerInfo::SetCachedProxyServerInfo(
    IN CServerInfo * pProxyServer,
    IN DWORD dwProxyVersion,
    IN BOOL fUseProxy,
    IN INTERNET_SCHEME HostScheme,
    IN INTERNET_PORT HostPort,
    IN INTERNET_SCHEME ProxyScheme,
    IN INTERNET_PORT ProxyPort
    )
/*++

Routine Description:

    If the Version information match up, copies
     the proxy information and links this server object
     to the appopriate proxy server object

    Assumes that this is called on successful use of the proxy
      object.

Arguments:

    None.

Return Value:

    DWORD
        ERROR_SUCCESS

        FALSE   - entry was not deleted

--*/


{
    DWORD error=ERROR_SUCCESS;

    LockSerializedList(&GlobalServerInfoList);            

    if ( dwProxyVersion != GlobalProxyVersionCount ) 
    {
        SetProxyScriptCached(FALSE);
        goto quit; // bail, we don't accept out of date additions to the cache
    }

    if ( m_ProxyLink )
    {
        if ( IsProxyScriptCached() && 
             HostScheme == m_HostScheme &&
             HostPort == m_HostPort &&
             fUseProxy )
        {
            if ( pProxyServer == m_ProxyLink ) {            
                INET_ASSERT(dwProxyVersion == GlobalProxyVersionCount);
                m_dwProxyVersion = dwProxyVersion; // we're now up to date
                goto quit; // match, no version or host changes
            }

            INET_ASSERT(pProxyServer != m_ProxyLink );            
        }
        //
        // unlink, because we have a new entry to save,
        //  and the previous entry is bad
        //
        m_ProxyLink->Dereference();
        m_ProxyLink = NULL;
    }

    //
    // Add new cached entry
    //

    SetProxyScriptCached(TRUE);

    m_HostScheme     = HostScheme;
    m_HostPort       = HostPort;

    m_dwProxyVersion = dwProxyVersion; // we're now up to date

    if ( fUseProxy )
    {
        INET_ASSERT(this != pProxyServer);

        m_ProxyLink = pProxyServer;
        m_ProxyLink->Reference();

        m_ProxyLink->m_HostScheme = ProxyScheme;
        m_ProxyLink->m_HostPort   = ProxyPort;

        switch (ProxyScheme)
        {
            case INTERNET_SCHEME_HTTP:
                m_ProxyLink->SetCernProxy();
                break;
            case INTERNET_SCHEME_SOCKS: 
                m_ProxyLink->SetSocksGateway();
                break;
            case INTERNET_SCHEME_FTP:
                m_ProxyLink->SetFTPProxy();
                break;
        }
    }

quit:

    UnlockSerializedList(&GlobalServerInfoList);        

    return error;
}

CServerInfo * 
CServerInfo::GetCachedProxyServerInfo(
    IN INTERNET_SCHEME HostScheme,
    IN INTERNET_PORT HostPort,
    OUT BOOL *pfCachedEntry
    )

/*++

Routine Description:

   Retrieves a cached server object, that indicates
    a probable proxy to use

   On Success, the return has an additional increment
    on its ref count, assumition that caller derefs

Arguments:

    None.

Return Value:

    CServerInfo *     
        NULL on failure

--*/

{
    CServerInfo *pProxyServer = NULL;

    LockSerializedList(&GlobalServerInfoList);        

    *pfCachedEntry = FALSE; 

    if ( IsProxyScriptCached() )
    {        
        //
        // Examine Version Count
        //

        if ( GlobalProxyVersionCount == m_dwProxyVersion &&
             HostScheme == m_HostScheme &&
             HostPort == m_HostPort
             )
        {
            *pfCachedEntry = TRUE;

            if ( m_ProxyLink ) {
                // matched cached entry
                m_ProxyLink->Reference();
                pProxyServer = m_ProxyLink;                    
            }
        }
        else
        {
            // version is expired, remove reference
            SetProxyScriptCached(FALSE);
            if ( m_ProxyLink ) {                
                m_ProxyLink->Dereference();
                m_ProxyLink = NULL;
            }
        }            
    }
        
    UnlockSerializedList(&GlobalServerInfoList);        
    return pProxyServer;
}

BOOL    
CServerInfo::CopyCachedProxyInfoToProxyMsg(
    IN OUT AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo
    )

/*++

Routine Description:

   Retrieves Cached Proxy info from object

Arguments:

    None.

Return Value:

    BOOL
        TRUE - sucess

--*/

{
    BOOL fSuccess = FALSE;

    // really only need to lock to proctect m_HostPort && m_HostScheme
    LockSerializedList(&GlobalServerInfoList);        

    pQueryForProxyInfo->SetUseProxy(FALSE);
    pQueryForProxyInfo->_lpszProxyHostName =  
        m_HostName.StringAddress() ? 
        NewString(m_HostName.StringAddress()) :
        NULL;

    if ( pQueryForProxyInfo->_lpszProxyHostName != NULL ) {
        // copy out cached entry to proxy message structure
        pQueryForProxyInfo->_nProxyHostPort        = m_HostPort;
        pQueryForProxyInfo->_tProxyScheme          = m_HostScheme;
        pQueryForProxyInfo->_bFreeProxyHostName    = TRUE;
        pQueryForProxyInfo->_dwProxyHostNameLength = 
            strlen((pQueryForProxyInfo)->_lpszProxyHostName);
        pQueryForProxyInfo->SetUseProxy(TRUE);
        fSuccess = TRUE; // success
    }

    UnlockSerializedList(&GlobalServerInfoList);        
    return fSuccess;
}



//
//DWORD
//CServerInfo::ResolveHostName(
//    IN BOOL bForce
//    )
//
///*++
//
//Routine Description:
//
//    Resolves the host name for a CServerInfo. If we already have a resolved
//    name for this info then it is freed. There can be only one thread updating
//    or using the host name info
//
//Arguments:
//
//    bForce  - TRUE if we need to force a re-resolution
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
//    DEBUG_ENTER((DBG_SESSION,
//                 Dword,
//                 "CServerInfo::ResolveHostName",
//                 "{%q} %B",
//                 GetHostName(),
//                 bForce
//                 ));
//
//    DWORD error = ERROR_SUCCESS;
//
//    if (bForce || IsAddressListEmpty(&m_AddressList)) {
//        EnterCriticalSection(&m_AddressListCritSec);
//
//        //
//        // BUGBUG - ideally, we want to test TTL here if bForce is TRUE to stop
//        //          multiple simultaneous forced requests re-resolving the same
//        //          info
//        //
//
//        if (bForce || IsAddressListEmpty(&m_AddressList)) {
//
//            //
//            // FreeAddressList() checks for an empty list
//            //
//
//            FreeAddressList(&m_AddressList);
//
//            //
//            // resolve the name & generate an address list. Since we don't know
//            // the port address we want to talk to right now, we will generate a
//            // list containing a default port number (0) which we must override
//            // when we really connect to the server
//            //
//
//            error = ::GetServiceAddress(GetHostName(),
//                                        NULL, // service name
//                                        NULL, // service guid
//                                        NS_DEFAULT,
//                                        INTERNET_INVALID_PORT_NUMBER,
//                                        0,    // protocol characteristics
//                                        &m_AddressList
//                                        );
//        }
//        LeaveCriticalSection(&m_AddressListCritSec);
//    }
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}


VOID
CServerInfo::UpdateConnectTime(
    IN DWORD dwConnectTime
    )

/*++

Routine Description:

    Calculates average connect time

Arguments:

    dwConnectTime   - current connect time

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::UpdateConnectTime",
                 "{%q} %d",
                 GetHostName(),
                 dwConnectTime
                 ));

    DWORD connectTime = m_ConnectTime;

    if (connectTime == (DWORD)-1) {
        connectTime = dwConnectTime;
    } else {
        connectTime = (connectTime + dwConnectTime) / 2;
    }
//dprintf("%s: connect time = %d, ave = %d\n", GetHostName(), dwConnectTime, connectTime);

    DEBUG_PRINT(SESSION,
                INFO,
                ("average connect time = %d mSec\n",
                connectTime
                ));

    InterlockedExchange((LPLONG)&m_ConnectTime, connectTime);

    DEBUG_LEAVE(0);
}


VOID
CServerInfo::UpdateRTT(
    IN DWORD dwRTT
    )

/*++

Routine Description:

    Calculates rolling average round-trip time

Arguments:

    dwRTT   - current round-trip time

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::UpdateRTT",
                 "{%q} %d",
                 GetHostName(),
                 dwRTT
                 ));

    DWORD RTT = m_RTT;

    if (RTT == 0) {
        RTT = dwRTT;
    } else {
        RTT = (RTT + dwRTT) / 2;
    }
//dprintf("%s: RTT = %d, ave = %d\n", GetHostName(), dwRTT, RTT);

    DEBUG_PRINT(SESSION,
                INFO,
                ("average round trip time = %d mSec\n",
                RTT
                ));

    InterlockedExchange((LPLONG)&m_RTT, RTT);

    DEBUG_LEAVE(0);
}


VOID
CServerInfo::UpdateDownloadRate(
    IN DWORD dwBytesPerSecond
    )

/*++

Routine Description:

    Calculates average download rate

Arguments:

    dwBytesPerSecond    - current download rate

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::UpdateDownloadRate",
                 "{%q} %d",
                 GetHostName(),
                 dwBytesPerSecond
                 ));

    DWORD downloadRate = m_DownloadRate;

    if (downloadRate == 0) {
        downloadRate = dwBytesPerSecond;
    } else {
        downloadRate = (downloadRate + dwBytesPerSecond) / 2;
    }

    DEBUG_PRINT(SESSION,
                INFO,
                ("average download rate = %d bytes/second\n",
                downloadRate
                ));

    InterlockedExchange((LPLONG)&m_DownloadRate, downloadRate);

    DEBUG_LEAVE(0);
}


VOID
CServerInfo::UpdateUploadRate(
    IN DWORD dwBytesPerSecond
    )

/*++

Routine Description:

    Calculates average upload rate

Arguments:

    dwBytesPerSecond    - current upload rate

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::UpdateUploadRate",
                 "{%q} %d",
                 GetHostName(),
                 dwBytesPerSecond
                 ));

    DWORD uploadRate = m_UploadRate;

    if (uploadRate == 0) {
        uploadRate = dwBytesPerSecond;
    } else {
        uploadRate = (uploadRate + dwBytesPerSecond) / 2;
    }

    DEBUG_PRINT(SESSION,
                INFO,
                ("average upload rate = %d bytes/second\n",
                uploadRate
                ));

    InterlockedExchange((LPLONG)&m_UploadRate, uploadRate);

    DEBUG_LEAVE(0);
}

//
//DWORD
//CServerInfo::GetConnection(
//    IN DWORD dwSocketFlags,
//    IN INTERNET_PORT nPort,
//    IN DWORD dwTimeout,
//    OUT ICSocket * * lplpSocket
//    )
//
///*++
//
//Routine Description:
//
//    Combines connection limiting and keep-alive list management. If keep-alive
//    connection requested and one is available, it is given out. If we can create
//    a connection, the connection count is increased (actually decremented) and
//    an OK-to-continue indication returned to the caller. If all connections are
//    currently in use, we wait for one to become available. If this is a sync
//    operation, we block waiting for the waiter event. If this is an async
//    operation, we return ERROR_IO_PENDING and pick up the request on a worker
//    thread when it becomes unblocked or times out proper
//
//Arguments:
//
//    dwSocketFlags   - flags identifying what type of connection we want:
//
//                        SF_SECURE       - we want https connection
//
//                        SF_KEEP_ALIVE   - we want persistent connection
//
//                        SF_NON_BLOCKING - we want non-blocking (async) socket
//
//    nPort           - required port
//
//    dwTimeout       - number of milliseconds we are willing to wait for
//                      connection to become available
//
//    lplpSocket      - returned pointer to ICSocket. ONLY used if the request is
//                      for a keep-alive socket and we had one available
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//                    Depending on *lplpSocket, we either returned the socket to
//                    use, or its okay to create a new connection
//
//                  ERROR_IO_PENDING
//                    Request will complete asynchronously
//
//        Failure - ERROR_INTERNET_TIMEOUT
//                    Failed to get connection in time allowed
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SESSION,
//                 Dword,
//                 "CServerInfo::GetConnection",
//                 "{%q} %#x, %d, %d, %#x",
//                 GetHostName(),
//                 dwSocketFlags,
//                 nPort,
//                 dwTimeout,
//                 lplpSocket
//                 ));
//
//    *lplpSocket = NULL;
//
//    DWORD error = DoFsm(new CFsm_GetConnection(dwSocketFlags,
//                                               nPort,
//                                               dwTimeout,
//                                               lplpSocket,
//                                               this
//                                               ));
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}


DWORD
CFsm_GetConnection::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    Runs next CFsm_GetConnection state

Arguments:

    Fsm - FSM controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
//dprintf("%#x: %s FSM %#x state %s\n", GetCurrentThreadId(), Fsm->MapType(), Fsm, Fsm->MapState());
    DEBUG_ENTER((DBG_SESSION,
                 Dword,
                 "CFsm_GetConnection::RunSM",
                 "%#x",
                 Fsm
                 ));

    CServerInfo * pServerInfo = (CServerInfo *)Fsm->GetContext();
    CFsm_GetConnection * stateMachine = (CFsm_GetConnection *)Fsm;
    DWORD error;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
        stateMachine->StartTimer();

        //
        // fall through
        //

    case FSM_STATE_CONTINUE:

#ifdef NEW_CONNECTION_SCHEME
    case FSM_STATE_ERROR:
#endif
        error = pServerInfo->GetConnection_Fsm(stateMachine);
        break;

#ifndef NEW_CONNECTION_SCHEME

    case FSM_STATE_ERROR:

        INET_ASSERT((Fsm->GetError() == ERROR_INTERNET_TIMEOUT)
                    || (Fsm->GetError() == ERROR_INTERNET_OPERATION_CANCELLED));

        pServerInfo->RemoveWaiter((DWORD_PTR)Fsm);
        error = Fsm->GetError();
        Fsm->SetDone();
//dprintf("%#x: FSM_STATE_ERROR - %d\n", GetCurrentThreadId(), error);
        break;

#endif

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}

#ifdef NEW_CONNECTION_SCHEME
//
//
//DWORD
//CServerInfo::GetConnection_Fsm(
//    IN CFsm_GetConnection * Fsm
//    )
//
///*++
//
//Routine Description:
//
//    Tries to get a connection of requested type for caller. If no connection is
//    available then one of the following happens:
//
//        * If there are available keep-alive connections of a different type then
//          one is closed and the caller allowed to create a new connection
//
//        * If this is an async request, the FSM is blocked and the thread returns
//          to the pool if a worker, or back to the app if an app thread
//
//        * If this is a sync request, we wait on an event for a conneciton to be
//          made available, or the connect timeout to elapse
//
//        * In the situation where we are being run out of connections (number of
//          active connections >= connection limit AND no activity has taken place
//          on any connection in the last dwLimitTimeout mSec (in FSM)) then we
//          allow a new connection to be created
//
//Arguments:
//
//    Fsm - get connection FSM
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//                    Depending on *lplpSocket, we either returned the socket to
//                    use, or its okay to create a new connection
//
//                  ERROR_IO_PENDING
//                    Request will complete asynchronously
//
//        Failure - ERROR_INTERNET_TIMEOUT
//                    Failed to get connection in time allowed
//
//                  ERROR_INTERNET_INTERNAL_ERROR
//                    Something unexpected happened
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SESSION,
//                 Dword,
//                 "CServerInfo::GetConnection_Fsm",
//                 "{%q [%d+%d/%d]} %#x(%#x, %d, %d, %d)",
//                 GetHostName(),
//                 AvailableConnections(),
//                 KeepAliveConnections(),
//                 ConnectionLimit(),
//                 Fsm,
//                 Fsm->m_dwSocketFlags,
//                 Fsm->m_nPort,
//                 Fsm->m_dwTimeout,
//                 Fsm->m_dwLimitTimeout
//                 ));
//
//    DEBUG_PRINT(SESSION,
//                INFO,
//                ("FSM %#x state %s elapsed %d\n",
//                Fsm,
//                Fsm->MapState(),
//                Fsm->GetElapsedTime()
//                ));
//
//    DPRINTF("%#x: FSM %#x state %s elapsed %d\n",
//            GetCurrentThreadId(),
//            Fsm,
//            Fsm->MapState(),
//            Fsm->GetElapsedTime()
//            );
//
//    PERF_ENTER(GetConnection);
//
//    CFsm_GetConnection & fsm = *Fsm;
//    DWORD error = fsm.GetError();
//    FSM_STATE state = fsm.GetState();
//    LPINTERNET_THREAD_INFO lpThreadInfo = fsm.GetThreadInfo();
//    ICSocket * pSocket = NULL;
//    BOOL bUnlockList = FALSE;
//    BOOL bKeepAliveWaiters;
//
//    *fsm.m_lplpSocket = NULL;
//
//    //
//    // FSM_STATE_ERROR processing. Typically a timeout has occurred. We now want
//    // to force a new connection instead of timing out
//    //
//
//    if (error != ERROR_SUCCESS) {
//        RemoveWaiter((DWORD)Fsm);
//    }
//
//    //
//    // timeout error is OK - we just try again until we get cancelled out or hit
//    // the retry limit. Any other error causes failure. We should not be getting
//    // ERROR_SUCCESS (test put here for defensive purposes only)
//    //
//
//    if ((state == FSM_STATE_ERROR)
//    && (error != ERROR_INTERNET_TIMEOUT)
//    && (error != ERROR_SUCCESS)) {
//
//        DPRINTF("%#x: FSM %#x FSM_STATE_ERROR, error = %d\n",
//                GetCurrentThreadId(),
//                Fsm,
//                error
//                );
//
//        DEBUG_PRINT(SESSION,
//                    ERROR,
//                    ("FSM %#x FSM_STATE_ERROR, error = %s (%d)\n",
//                    Fsm,
//                    InternetMapError(error),
//                    error
//                    ));
//
//        INET_ASSERT(error != ERROR_SUCCESS);
//
//        goto quit;
//    }
//
//    INET_ASSERT(lpThreadInfo != NULL);
//    INET_ASSERT(lpThreadInfo->hObjectMapped != NULL);
//
//    if ((lpThreadInfo == NULL) || (lpThreadInfo->hObjectMapped == NULL)) {
//        error = ERROR_INTERNET_INTERNAL_ERROR;
//        goto quit;
//    }
//
//    BOOL bAsyncRequest;
//
//    bAsyncRequest = lpThreadInfo->IsAsyncWorkerThread
//                    || ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->
//                        IsAsyncHandle();
//
//    //
//    // before we do anything, check if we've been run out of connections. If we
//    // don't do this check here and we are out, then we'll wait unnecessarily
//    // (assuming connections are not returned)
//    //
//
//    if ((state == FSM_STATE_INIT) && RunOutOfConnections()) {
//
//        DPRINTF("%#x: out of connections on first attempt: %d+%d/%d\n",
//                GetCurrentThreadId(),
//                AvailableConnections(),
//                KeepAliveConnections(),
//                ConnectionLimit()
//                );
//
//        DEBUG_PRINT(SESSION,
//                    ERROR,
//                    ("out of connections on first attempt: %d+%d/%d\n",
//                    AvailableConnections(),
//                    KeepAliveConnections(),
//                    ConnectionLimit()
//                    ));
//
//        error = ERROR_SUCCESS;
//        pSocket = NULL;
//        goto quit;
//    }
//
//try_again:
//
//    bUnlockList = TRUE;
//
//    //
//    // use m_Waiters to serialize access. N.B. - we will acquire m_KeepAliveList
//    // from within m_Waiters
//    //
//
//    LockSerializedList(&m_Waiters);
//    bKeepAliveWaiters = KeepAliveWaiters();
//    if (fsm.m_dwSocketFlags & SF_KEEP_ALIVE) {
//
//        //
//        // maintain requester order - if there are already waiters then queue
//        // this request, else try to satisfy the requester. HOWEVER, only check
//        // for existing requesters the FIRST time through. If we're here with
//        // FSM_STATE_CONTINUE then we've been unblocked and we can ignore any
//        // waiters that came after us
//        //
//
//        if ((state == FSM_STATE_CONTINUE) || !bKeepAliveWaiters) {
//
//            DEBUG_PRINT(SESSION,
//                        INFO,
//                        ("continuing or no current waiters for K-A connections\n"
//                        ));
//
//            while (pSocket = FindKeepAliveConnection(fsm.m_dwSocketFlags, fsm.m_nPort)) {
//                if (pSocket->IsReset()) {
//
//                    DPRINTF("%#x: ********* socket %#x is closed already\n",
//                            GetCurrentThreadId(),
//                            pSocket->GetSocket()
//                            );
//
//                    DEBUG_PRINT(SESSION,
//                                INFO,
//                                ("K-A connection %#x [%#x] is closed\n",
//                                pSocket,
//                                pSocket->GetSocket()
//                                ));
//
//                    pSocket->SetLinger(FALSE, 0);
//                    pSocket->Shutdown(2);
////dprintf("GetConnection: destroying reset socket %#x\n", pSocket->GetSocket());
//                    pSocket->Destroy();
//                    if (!UnlimitedConnections()) {
//                        ++m_ConnectionsAvailable;
//                    }
//                    CHECK_CONNECTION_COUNT();
//                } else {
//
//                    DPRINTF("%#x: *** matched %#x, %#x\n",
//                            GetCurrentThreadId(),
//                            pSocket->GetSocket(),
//                            pSocket->GetFlags()
//                            );
//
//                    break;
//                }
//            }
//            if (pSocket == NULL) {
//
//                DEBUG_PRINT(SESSION,
//                            INFO,
//                            ("no available K-A connections\n"
//                            ));
//
//                /*
//                //
//                // if all connections are in use as keep-alive connections then
//                // since we're here, we want a keep-alive connection that doesn't
//                // match the currently available keep-alive connections. Terminate
//                // the oldest keep-alive connection (at the head of the queue)
//                // and generate a new connection
//                //
//
//                LockSerializedList(&m_KeepAliveList);
//                if (ElementsOnSerializedList(&m_KeepAliveList) == m_ConnectionLimit) {
//                    pSocket = ContainingICSocket(SlDequeueHead(&m_KeepAliveList));
//                    pSocket->SetLinger(FALSE, 0);
//                    pSocket->Shutdown(2);
//                    pSocket->Destroy();
//                    if (!UnlimitedConnections()) {
//                        ++m_ConnectionsAvailable;
//                    }
//                    CHECK_CONNECTION_COUNT();
//                }
//                UnlockSerializedList(&m_KeepAliveList);
//                */
//            }
//        } else {
//
//            DEBUG_PRINT(SESSION,
//                        INFO,
//                        ("%d waiters for K-A connection to %q\n",
//                        ElementsOnSerializedList(&m_KeepAliveList),
//                        GetHostName()
//                        ));
//
//        }
//    }
//
//    //
//    // if we found a matching keep-alive connection or we are not limiting
//    // connections then we're done
//    //
//
//    if ((pSocket != NULL) || UnlimitedConnections()) {
//
//        INET_ASSERT(error == ERROR_SUCCESS);
//
//        error = ERROR_SUCCESS;
//        goto exit;
//    }
//
//    //
//    // no keep-alive connections matched, or there are already waiters for
//    // keep-alive connections
//    //
//
//    INET_ASSERT((AvailableConnections() >= 0)
//                && (AvailableConnections() <= ConnectionLimit()));
//
//    if (AvailableConnections() > 0) {
//
//        //
//        // can create a connection
//        //
//
//        DEBUG_PRINT(SESSION,
//                    INFO,
//                    ("%s: OK to create new connection: %d/%d\n",
//                    GetHostName(),
//                    AvailableConnections(),
//                    ConnectionLimit()
//                    ));
//
//        DPRINTF("%#x: *** %s: OK to create connection: %d/%d\n",
//                GetCurrentThreadId(),
//                GetHostName(),
//                AvailableConnections(),
//                ConnectionLimit()
//                );
//
//        INET_ASSERT(error == ERROR_SUCCESS);
//        INET_ASSERT(pSocket == NULL);
//
//        --m_ConnectionsAvailable;
//    //} else if (fsm.GetElapsedTime() > fsm.m_dwTimeout) {
//    //    error = ERROR_INTERNET_TIMEOUT;
//    } else {
//
//        //
//        // if there are keep-alive connections but no keep-alive waiters
//        // then either we don't want a keep-alive connection, or the ones
//        // available don't match our requirements.
//        // If we need a connection of a different type - e.g. SSL when all
//        // we have is non-SSL then close a connection & generate a new one.
//        // If we need a non-keep-alive connection then its okay to return
//        // a current keep-alive connection, the understanding being that the
//        // caller will not add Connection: Keep-Alive header (HTTP 1.0) or
//        // will add Connection: Close header (HTTP 1.1)
//        //
//
//        //
//        // BUGBUG - what about waiters for non-keep-alive connections?
//        //
//        // scenario - limit of 1 connection:
//        //
//        //  A. request for k-a
//        //      continue & create connection
//        //  B. request non-k-a
//        //      none available; wait
//        //  C. release k-a connection; unblock sync waiter B
//        //  D. request non-k-a
//        //      k-a available; return it; caller converts to non-k-a
//        //  E. unblocked waiter B request non-k-a
//        //      none available; wait
//        //
//        // If this situation continues, eventually B will time-out, whereas it
//        // could have had the connection taken by D. Request D is younger and
//        // therefore can afford to wait while B continues with the connection
//        //
//
//        BOOL fHaveConnection = FALSE;
//
//        if (!bKeepAliveWaiters) {
//            LockSerializedList(&m_KeepAliveList);
//            if (KeepAliveConnections() != 0) {
//                pSocket = ContainingICSocket(SlDequeueHead(&m_KeepAliveList));
//                fHaveConnection = TRUE;
//
//#define SOCK_FLAGS  (SF_ENCRYPT | SF_DECRYPT | SF_SECURE)
//
//                DWORD dwSocketTypeFlags = pSocket->GetFlags() & SOCK_FLAGS;
//                DWORD dwRequestTypeFlags = fsm.m_dwSocketFlags & SOCK_FLAGS;
//
//                if (dwSocketTypeFlags ^ dwRequestTypeFlags) {
//
//                    DEBUG_PRINT(SESSION,
//                                INFO,
//                                ("different socket types requested: %#x, %#x\n",
//                                fsm.m_dwSocketFlags,
//                                pSocket->GetFlags()
//                                ));
//
//                    DPRINTF("%#x: *** closing socket %#x: %#x vs. %#x\n",
//                            GetCurrentThreadId(),
//                            pSocket->GetSocket(),
//                            pSocket->GetFlags(),
//                            fsm.m_dwSocketFlags
//                            );
//
//                    pSocket->SetLinger(FALSE, 0);
//                    pSocket->Shutdown(2);
////dprintf("GetConnection: destroying different type socket %#x\n", pSocket->GetSocket());
//                    pSocket->Destroy();
//                    pSocket = NULL;
//                } else {
//
//                    DPRINTF("%#x: *** returning k-a connection %#x as non-k-a\n",
//                            GetCurrentThreadId(),
//                            pSocket->GetSocket()
//                            );
//
//                }
//                CHECK_CONNECTION_COUNT();
//            }
//            UnlockSerializedList(&m_KeepAliveList);
//            if (fHaveConnection) {
//                goto exit;
//            }
//        }
//
//        //
//        // about to wait for a connection. If it looks as though we're being run
//        // out of connections, then create a new one
//        //
//
//        if (RunOutOfConnections()) {
//            pSocket = NULL;
//            error = ERROR_SUCCESS;
//            goto exit;
//        }
//
//        DPRINTF("%#x: blocking %s FSM %#x state %s %d/%d elapsed: %d mSec\n",
//                GetCurrentThreadId(),
//                Fsm->MapType(),
//                Fsm,
//                Fsm->MapState(),
//                AvailableConnections(),
//                ConnectionLimit(),
//                Fsm->GetElapsedTime()
//                );
//
//        //
//        // we have to wait for a connection to become available. If we are an
//        // async request then we queue this FSM & return the thread to the pool
//        // or, if app thread, return pending indication to the app. If this is
//        // a sync request (in an app thread) then we block on an event waiting
//        // for a connection to become available
//        //
//
//        HANDLE hEvent = NULL;
//
//        if (!bAsyncRequest) {
//
//            //
//            // create unnamed, initially unsignalled, auto-reset event
//            //
//
//            hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
//            if (hEvent == NULL) {
//                error = GetLastError();
//                goto exit;
//            }
//        }
//
//        CConnectionWaiter * pWaiter;
//
//#if INET_DEBUG
//
//        for (pWaiter = (CConnectionWaiter *)HeadOfSerializedList(&m_Waiters);
//             pWaiter != (CConnectionWaiter *)SlSelf(&m_Waiters);
//             pWaiter = (CConnectionWaiter *)pWaiter->Next()) {
//
//            INET_ASSERT(pWaiter->Id() != (DWORD)(bAsyncRequest ? (DWORD)Fsm : lpThreadInfo->ThreadId));
//        }
//#endif
//
//        pWaiter = new CConnectionWaiter(&m_Waiters,
//                                        !bAsyncRequest,
//                                        (fsm.m_dwSocketFlags & SF_KEEP_ALIVE)
//                                            ? TRUE
//                                            : FALSE,
//                                        bAsyncRequest
//                                            ? (DWORD)Fsm
//                                            : lpThreadInfo->ThreadId,
//                                        hEvent);
//        if (pWaiter == NULL) {
//            error = ERROR_NOT_ENOUGH_MEMORY;
//            goto exit;
//        }
//        if (bAsyncRequest) {
//
//            //
//            // ensure that when the FSM is unblocked normally, the new state
//            // is STATE_CONTINUE
//            //
//
//            DWORD dwWaitTime = min(fsm.m_dwLimitTimeout, fsm.m_dwTimeout);
//
//            ////
//            //// make wait time elastic: we don't want to time out after the
//            //// connect timeout value elapses. We keep expanding the wait time
//            //// for a connection until we get one, or the request is cancelled
//            ////
//            //
//            //fsm.m_dwTimeout += dwWaitTime;
//            //if (fsm.m_dwTimeout == 0xffffffff) {
//            //    --fsm.m_dwTimeout;
//            //}
//            fsm.SetState(FSM_STATE_CONTINUE);
//            fsm.SetNextState(FSM_STATE_CONTINUE);
//            error = BlockWorkItem(Fsm, (DWORD)pWaiter, dwWaitTime);
//            if (error == ERROR_SUCCESS) {
//                error = ERROR_IO_PENDING;
//            }
//        } else {
//            UnlockSerializedList(&m_Waiters);
//            bUnlockList = FALSE;
//
//            DPRINTF("%#x: %s FSM %#x %s waiting %d mSec\n",
//                    GetCurrentThreadId(),
//                    Fsm->MapType(),
//                    Fsm,
//                    Fsm->MapState(),
//                    fsm.m_dwTimeout
//                    );
//
//            //DWORD dwWaitTime = fsm.m_dwTimeout - fsm.GetElapsedTime();
//            DWORD dwWaitTime = fsm.m_dwTimeout;
//
//            if ((int)dwWaitTime <= 0) {
//
//                DEBUG_PRINT(SESSION,
//                            ERROR,
//                            ("SYNC wait timed out (%d mSec)\n",
//                            fsm.m_dwTimeout
//                            ));
//
//                error = ERROR_INTERNET_TIMEOUT;
//            } else {
//
//                DEBUG_PRINT(SESSION,
//                            INFO,
//                            ("waiting %d mSec for SYNC event %#x\n",
//                            dwWaitTime,
//                            hEvent
//                            ));
//
//                //
//                // we'd better not be doing a sync wait if we are in the
//                // context of an app thread making an async request
//                //
//
//                INET_ASSERT(!lpThreadInfo->IsAsyncWorkerThread
//                            && !((INTERNET_HANDLE_OBJECT *)lpThreadInfo->
//                                hObjectMapped)->IsAsyncHandle());
//
//                //INET_ASSERT(dwWaitTime <= 60000);
//
//                DWORD dwDeltaWaitTime = min(dwWaitTime, fsm.m_dwLimitTimeout);
//                DWORD dwTimeStarted = GetTickCount();
//
//                do {
//
//                    DPRINTF("%#x: sync wait %d mSec\n",
//                            GetCurrentThreadId(),
//                            dwDeltaWaitTime
//                            );
//
//                    error = WaitForSingleObject(hEvent, dwDeltaWaitTime);
//                    if (error == STATUS_TIMEOUT) {
//                        if (RunOutOfConnections()) {
//
//                            DPRINTF("%#x: run out of connections\n",
//                                    GetCurrentThreadId()
//                                    );
//
//                            break;
//                        }
//                    }
//                } while (((GetTickCount() - dwTimeStarted) < dwWaitTime)
//                         && (error == STATUS_TIMEOUT));
//
//                DPRINTF("%#x: sync waiter unblocked - error = %d\n",
//                        GetCurrentThreadId(),
//                        error
//                        );
//
//            }
//            if (error == STATUS_TIMEOUT) {
//
//                DPRINTF("%#x: %s %d+%d/%d: timed out %#x (%s FSM %#x %s)\n",
//                        GetCurrentThreadId(),
//                        GetHostName(),
//                        AvailableConnections(),
//                        KeepAliveConnections(),
//                        ConnectionLimit(),
//                        GetCurrentThreadId(),
//                        Fsm->MapType(),
//                        Fsm,
//                        Fsm->MapState()
//                        );
//
//                RemoveWaiter(lpThreadInfo->ThreadId);
//                //bUnlockList = FALSE;
//                /*
//                LockSerializedList(&m_Waiters);
//                for (pWaiter = (CConnectionWaiter *)HeadOfSerializedList(&m_Waiters);
//                     pWaiter != (CConnectionWaiter *)SlSelf(&m_Waiters);
//                     pWaiter = (CConnectionWaiter *)pWaiter->Next()) {
//
//                    if (pWaiter->Id() == lpThreadInfo->ThreadId) {
//                        RemoveFromSerializedList(&m_Waiters, pWaiter->List());
//                        delete pWaiter;
//                        break;
//                    }
//                }
//                */
//                error = RunOutOfConnections()
//                        ? WAIT_OBJECT_0
//                        : ERROR_INTERNET_TIMEOUT;
//            //} else {
//            //    bUnlockList = FALSE;
//            }
//
//            BOOL bOk;
//
//            bOk = CloseHandle(hEvent);
//
//            INET_ASSERT(bOk);
//
//            if (error == WAIT_OBJECT_0) {
//
//                DPRINTF("%#x: sync requester trying again\n",
//                        GetCurrentThreadId()
//                        );
//
//                fsm.SetState(FSM_STATE_CONTINUE);
//                goto try_again;
//            }
//        }
//    }
//
//exit:
//
//    //
//    // if we are returning a (keep-alive) socket that has a different blocking
//    // mode from that requested, change it
//    //
//
//    if (pSocket != NULL) {
//        if ((pSocket->GetFlags() & SF_NON_BLOCKING)
//            ^ (fsm.m_dwSocketFlags & SF_NON_BLOCKING)) {
//
//            DEBUG_PRINT(SESSION,
//                        INFO,
//                        ("different blocking modes requested: %#x, %#x\n",
//                        fsm.m_dwSocketFlags,
//                        pSocket->GetFlags()
//                        ));
//
//            DPRINTF("%#x: *** changing socket %#x to %sBLOCKING\n",
//                    GetCurrentThreadId(),
//                    pSocket->GetSocket(),
//                    fsm.m_dwSocketFlags & SF_NON_BLOCKING
//                        ? "NON-"
//                        : ""
//                    );
//
//            pSocket->SetNonBlockingMode(fsm.m_dwSocketFlags
//                                        & SF_NON_BLOCKING);
//        }
//        *fsm.m_lplpSocket = pSocket;
//    }
//
//    if (bUnlockList) {
//        UnlockSerializedList(&m_Waiters);
//    }
//
//quit:
//
//    if (error != ERROR_IO_PENDING) {
//        fsm.SetDone();
//    }
//
//    DPRINTF("%#x: %s %d+%d/%d: get: %d, %#x, %d\n",
//            GetCurrentThreadId(),
//            GetHostName(),
//            AvailableConnections(),
//            KeepAliveConnections(),
//            ConnectionLimit(),
//            error,
//            pSocket ? pSocket->GetSocket() : 0,
//            ElementsOnSerializedList(&m_Waiters)
//            );
//
//    PERF_LEAVE(GetConnection);
//
//    DEBUG_LEAVE(error);
//
//    return error;
//}
//
//
//DWORD
//CServerInfo::ReleaseConnection(
//    IN ICSocket * lpSocket OPTIONAL
//    )
//
///*++
//
//Routine Description:
//
//    Returns a keep-alive connection to the pool, or allows another requester to
//    create a connection.
//
//    If we will break the connection limit then we assume we created an additional
//    connection when we were being starved, and we discard this one
//
//Arguments:
//
//    lpSocket    - pointer to ICSocket if we are returning a keep-alive connection
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
//    DPRINTF("%#x: rls %#x %d+%d/%d\n",
//            GetCurrentThreadId(),
//            lpSocket ? lpSocket->GetSocket() : 0,
//            AvailableConnections(),
//            KeepAliveConnections(),
//            ConnectionLimit()
//            );
//
//    //return ERROR_SUCCESS;
//    DEBUG_ENTER((DBG_SESSION,
//                 Dword,
//                 "CServerInfo::ReleaseConnection",
//                 "{%q [%d+%d/%d]} %#x [%#x]",
//                 GetHostName(),
//                 AvailableConnections(),
//                 KeepAliveConnections(),
//                 ConnectionLimit(),
//                 lpSocket,
//                 lpSocket ? lpSocket->GetSocket() : 0
//                 ));
//
//    PERF_ENTER(ReleaseConnection);
//
//    DWORD error = ERROR_SUCCESS;
//    BOOL bRelease = FALSE;
//
//    LockSerializedList(&m_Waiters);
//
//    //
//    // quite often (at least with catapult proxy based on IIS) the server may
//    // drop the connection even though it indicated it would keep it open. This
//    // typically happens on 304 (frequent) and 302 (less so) responses. If we
//    // determine the server has dropped the connection then throw it away and
//    // allow the app to create a new one
//    //
//
//    if (lpSocket != NULL) {
//        if (TotalAvailableConnections() >= ConnectionLimit()) {
//
//            DPRINTF("%#x: !!! too many K-A connections %d+%d/%d - discarding %#x\n",
//                    GetCurrentThreadId(),
//                    AvailableConnections(),
//                    KeepAliveConnections(),
//                    ConnectionLimit(),
//                    lpSocket->GetSocket()
//                    );
//
//            //
//            // trying to return keep-alive socket would overflow limit (if we
//            // are not already over). If we have less than the max. keep-alive
//            // connections, we will keep this one else close it
//            //
//
//            if (KeepAliveConnections() >= ConnectionLimit()) {
//
//                INET_ASSERT(KeepAliveConnections() == ConnectionLimit());
//
//                //
//                // BUGBUG - discarding k-a connection: it should be the oldest
//                //          (if oldest not this one)
//                //
//
//                DPRINTF("%#x: closing k-a %#x\n",
//                        GetCurrentThreadId(),
//                        lpSocket->GetSocket()
//                        );
//
//                lpSocket->Close();
//            } else {
//
//                INET_ASSERT(AvailableConnections() > 0);
//
//                if (AvailableConnections() > 0) {
//                    --m_ConnectionsAvailable;
//                }
//            }
//        }
//        if (lpSocket->IsClosed() || lpSocket->IsReset()) {
//
//            DEBUG_PRINT(SESSION,
//                        INFO,
//                        ("socket %#x already dead - throwing it out\n",
//                        lpSocket->GetSocket()
//                        ));
//
//            DPRINTF("%#x: socket %#x: already reset\n",
//                    GetCurrentThreadId(),
//                    lpSocket->GetSocket()
//                    );
//
////dprintf("ReleaseConnection: destroying already closed socket %#x\n", lpSocket->GetSocket());
//
//            BOOL bDestroyed = lpSocket->Dereference();
//
//            INET_ASSERT(bDestroyed);
//
//            lpSocket = NULL;
//        } else {
//
//            //
//            // if we are returning a keep-alive socket, put it in non-blocking
//            // mode if not already. Typically, Internet Explorer uses non-blocking
//            // sockets. In the infrequent cases where we want a blocking socket
//            // - mainly when doing java downloads - we will convert the socket
//            // to blocking mode when we get it from the pool
//            //
//
//            if (!lpSocket->IsNonBlocking()) {
//
//                DPRINTF("%#x: ***** WARNING: releasing BLOCKING k-a socket %#x\n",
//                        GetCurrentThreadId(),
//                        lpSocket->GetSocket()
//                        );
//
//                lpSocket->SetNonBlockingMode(TRUE);
//            }
//        }
//    }
//    if (lpSocket != NULL) {
//
//        DPRINTF("%#x: releasing K-A %#x (%d/%d)\n",
//                GetCurrentThreadId(),
//                lpSocket ? lpSocket->GetSocket() : 0,
//                AvailableConnections(),
//                ConnectionLimit()
//                );
//
//        INET_ASSERT(lpSocket->IsOpen());
//        INET_ASSERT(!lpSocket->IsOnList());
//        //INET_ASSERT(!lpSocket->IsReset());
//
//        lpSocket->SetKeepAlive();
//
//        DEBUG_PRINT(SESSION,
//                    INFO,
//                    ("releasing keep-alive socket %#x\n",
//                    lpSocket->GetSocket()
//                    ));
//
//        lpSocket->SetExpiryTime(GlobalKeepAliveSocketTimeout);
//
//        INET_ASSERT(!IsOnSerializedList(&m_KeepAliveList, lpSocket->List()));
//
//        InsertAtTailOfSerializedList(&m_KeepAliveList, lpSocket->List());
//
//        INET_ASSERT(UnlimitedConnections()
//                    ? TRUE
//                    : (KeepAliveConnections() <= ConnectionLimit()));
//
//        bRelease = TRUE;
//    } else {
//
//        DPRINTF("%#x: releasing connection (%d+%d/%d)\n",
//                GetCurrentThreadId(),
//                AvailableConnections(),
//                KeepAliveConnections(),
//                ConnectionLimit()
//                );
//
//        if (!UnlimitedConnections()) {
//            if (AvailableConnections() < ConnectionLimit()) {
//                ++m_ConnectionsAvailable;
//            } else {
//
//                DPRINTF("%#x: !!! not increasing avail cons (%d+%d) - at limit (%d)\n",
//                        GetCurrentThreadId(),
//                        AvailableConnections(),
//                        KeepAliveConnections(),
//                        ConnectionLimit()
//                        );
//
//            }
//        }
//
//        CHECK_CONNECTION_COUNT();
//
//        bRelease = TRUE;
//    }
//    if (bRelease && !UnlimitedConnections()) {
//
//        CHECK_CONNECTION_COUNT();
//
//        CConnectionWaiter * pWaiter;
//        BOOL bFreed = FALSE;
//
//        //
//        // loop here until we free a waiter or until there are no waiters left.
//        // The reason we do this is that we must free a waiter if there are any
//        // but the waiter corresponding to pWaiter may have been concurrently
//        // timed out and cannot be unblocked by us
//        //
//
//        do {
//            pWaiter = (CConnectionWaiter *)SlDequeueHead(&m_Waiters);
//            if (pWaiter != NULL) {
//
//                DEBUG_PRINT(SESSION,
//                            INFO,
//                            ("unblocking %s waiter %#x\n",
//                            pWaiter->IsSync() ? "Sync" : "Async",
//                            pWaiter->Id()
//                            ));
//
//                DPRINTF("%#x: Unblocking %s connection waiter %#x\n",
//                        GetCurrentThreadId(),
//                        pWaiter->IsSync() ? "Sync" : "Async",
//                        pWaiter->Id()
//                        );
//
//                if (pWaiter->IsSync()) {
//                    pWaiter->Signal();
//                    bFreed = TRUE;
//                } else {
//
//                    int n = (int)UnblockWorkItems(1, (DWORD)pWaiter, ERROR_SUCCESS);
//
//                    if (n >= 1) {
//
//                        //
//                        // should never be > 1
//                        //
//
//                        INET_ASSERT(n == 1);
//
//                        bFreed = TRUE;
//
//                        DPRINTF("%#x: unblocked waiting FSM %#x\n",
//                                GetCurrentThreadId(),
//                                pWaiter->Id()
//                                );
//
//                        DEBUG_PRINT(SESSION,
//                                    INFO,
//                                    ("unblocked waiting FSM %#x\n",
//                                    pWaiter->Id()
//                                    ));
//
//                    } else {
//
//                        //
//                        // should never be < 0
//                        //
//
//                        INET_ASSERT(n == 0);
//
//                        DPRINTF("%#x: ********* waiting FSM %#x NOT UNBLOCKED\n",
//                                GetCurrentThreadId(),
//                                pWaiter->Id()
//                                );
//
//                        DEBUG_PRINT(SESSION,
//                                    ERROR,
//                                    ("waiting FSM %#x not unblocked\n",
//                                    pWaiter->Id()
//                                    ));
//
//                    }
//                }
//                delete pWaiter;
//            } else {
//
//                DEBUG_PRINT(SESSION,
//                            INFO,
//                            ("no waiters\n"
//                            ));
//
//                DPRINTF("%#x: !!! NOT unblocking connection waiter\n",
//                        GetCurrentThreadId()
//                        );
//
//            }
//        } while ((pWaiter != NULL) && !bFreed);
//    } else {
//
//        DPRINTF("%#x: !!! NOT releasing or unlimited?\n",
//                GetCurrentThreadId()
//                );
//
//        DEBUG_PRINT(SESSION,
//                    INFO,
//                    ("bRelease = %B, UnlimitedConnections() = %B\n",
//                    bRelease,
//                    UnlimitedConnections()
//                    ));
//
//    }
//
//    if (TotalAvailableConnections() >= ConnectionLimit()) {
//        ResetLastActiveTime();
//    }
//
//    DEBUG_PRINT(SESSION,
//                INFO,
//                ("avail+k-a/limit = %d+%d/%d\n",
//                AvailableConnections(),
//                KeepAliveConnections(),
//                ConnectionLimit()
//                ));
//
//    UnlockSerializedList(&m_Waiters);
//
//    PERF_LEAVE(ReleaseConnection);
//
//    DEBUG_LEAVE(error);
//
//    DPRINTF("%#x: %s %d+%d/%d: rls %#x: %d, %d\n",
//            GetCurrentThreadId(),
//            GetHostName(),
//            AvailableConnections(),
//            KeepAliveConnections(),
//            ConnectionLimit(),
//            lpSocket ? lpSocket->GetSocket() : 0,
//            error,
//            ElementsOnSerializedList(&m_Waiters)
//            );
//
//    return error;
//}
//
#else


DWORD
CServerInfo::GetConnection_Fsm(
    IN CFsm_GetConnection * Fsm
    )

/*++

Routine Description:

    Tries to get a connection of requested type for caller. If no connection is
    available then one of the following happens:

        * If there are available keep-alive connections of a different type then
          one is closed and the caller allowed to create a new connection

        * If this is an async request, the FSM is blocked and the thread returns
          to the pool if a worker, or back to the app if an app thread

        * If this is a sync request, we wait on an event for a connection to be
          made available, or the connect timeout to elapse

Arguments:

    Fsm - get connection FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    Depending on *lplpSocket, we either returned the socket to
                    use, or its okay to create a new connection

                  ERROR_IO_PENDING
                    Request will complete asynchronously

        Failure - ERROR_INTERNET_TIMEOUT
                    Failed to get connection in time allowed

                  ERROR_INTERNET_INTERNAL_ERROR
                    Something unexpected happened

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 Dword,
                 "CServerInfo::GetConnection_Fsm",
                 "{%q [%d+%d/%d]} %#x(%#x, %d, %d)",
                 GetHostName(),
                 m_ConnectionsAvailable,
                 ElementsOnSerializedList(&m_KeepAliveList),
                 m_ConnectionLimit,
                 Fsm,
                 Fsm->m_dwSocketFlags,
                 Fsm->m_nPort,
                 Fsm->m_dwTimeout
                 ));

    PERF_ENTER(GetConnection);

    BOOL bFound = FALSE;
    DWORD error = ERROR_SUCCESS;
    CFsm_GetConnection & fsm = *Fsm;
    ICSocket * pSocket = NULL;
    LPINTERNET_THREAD_INFO lpThreadInfo = fsm.GetThreadInfo();
    HANDLE hEvent = NULL;
    BOOL bUnlockList = TRUE;
    BOOL bKeepAliveWaiters;

    INET_ASSERT(lpThreadInfo != NULL);
    INET_ASSERT(lpThreadInfo->hObjectMapped != NULL);
    INET_ASSERT(((HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->
                GetHandleType() == TypeHttpRequestHandle);

    if ((lpThreadInfo == NULL) || (lpThreadInfo->hObjectMapped == NULL)) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    BOOL bAsyncRequest;

    bAsyncRequest = lpThreadInfo->IsAsyncWorkerThread
                    || ((INTERNET_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->
                        IsAsyncHandle();

    *fsm.m_lplpSocket = NULL;

try_again:

    bUnlockList = TRUE;

    //
    // use m_Waiters to serialize access. N.B. - we will acquire m_KeepAliveList
    // from within m_Waiters
    //

    m_Waiters.Acquire();
    if (IsNewLimit()) {
        UpdateConnectionLimit();
    }
    bKeepAliveWaiters = KeepAliveWaiters();
    if (fsm.m_dwSocketFlags & SF_KEEP_ALIVE) {

        //
        // maintain requester order - if there are already waiters then queue
        // this request, else try to satisfy the requester. HOWEVER, only check
        // for existing requesters the FIRST time through. If we're here with
        // FSM_STATE_CONTINUE then we've been unblocked and we can ignore any
        // waiters that came after us
        //

        if ((fsm.GetState() == FSM_STATE_CONTINUE) || !bKeepAliveWaiters) {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("no current waiters for K-A connections\n"
                        ));

            while (pSocket = FindKeepAliveConnection(fsm.m_dwSocketFlags, fsm.m_nPort)) {
                if (pSocket->IsReset() || pSocket->HasExpired()) {

                    DPRINTF("%#x: %#x: ********* socket %#x is closed already\n",
                            GetCurrentThreadId(),
                            Fsm,
                            pSocket->GetSocket()
                            );

                    DEBUG_PRINT(SESSION,
                                INFO,
                                ("K-A connection %#x [%#x/%d] is reset (%B) or expired (%B)\n",
                                pSocket,
                                pSocket->GetSocket(),
                                pSocket->GetSourcePort(),
                                pSocket->IsReset(),
                                pSocket->HasExpired()
                                ));

                    pSocket->SetLinger(FALSE, 0);
                    pSocket->Shutdown(2);
//dprintf("GetConnection: destroying reset socket %#x\n", pSocket->GetSocket());
                    pSocket->Destroy();
                    pSocket = NULL;
                    if (!UnlimitedConnections()) {
                        ++m_ConnectionsAvailable;
                    }
                    CHECK_CONNECTION_COUNT();
                } else {

                    DPRINTF("%#x: %#x: *** matched %#x, %#x\n",
                            GetCurrentThreadId(),
                            Fsm,
                            pSocket->GetSocket(),
                            pSocket->GetFlags()
                            );

                    break;
                }
            }
            if (pSocket == NULL) {

                DEBUG_PRINT(SESSION,
                            INFO,
                            ("no available K-A connections\n"
                            ));

                /*
                //
                // if all connections are in use as keep-alive connections then
                // since we're here, we want a keep-alive connection that doesn't
                // match the currently available keep-alive connections. Terminate
                // the oldest keep-alive connection (at the head of the queue)
                // and generate a new connection
                //

                LockSerializedList(&m_KeepAliveList);
                if (ElementsOnSerializedList(&m_KeepAliveList) == m_ConnectionLimit) {
                    pSocket = ContainingICSocket(SlDequeueHead(&m_KeepAliveList));
                    pSocket->SetLinger(FALSE, 0);
                    pSocket->Shutdown(2);
                    pSocket->Destroy();
                    if (!UnlimitedConnections()) {
                        ++m_ConnectionsAvailable;
                    }
                    CHECK_CONNECTION_COUNT();
                }
                UnlockSerializedList(&m_KeepAliveList);
                */
            }
        } else {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("%d waiters for K-A connection to %q\n",
                        ElementsOnSerializedList(&m_KeepAliveList),
                        GetHostName()
                        ));

        }
    }

    //
    // if we found a matching keep-alive connection or we are not limiting
    // connections then we're done
    //

    if ((pSocket != NULL) || UnlimitedConnections()) {

        INET_ASSERT(error == ERROR_SUCCESS);

        goto exit;
    }

    //
    // no keep-alive connections matched, or there are already waiters for
    // keep-alive connections
    //

    INET_ASSERT(m_ConnectionsAvailable <= m_ConnectionLimit);

    if (m_ConnectionsAvailable > 0) {

        //
        // can create a connection
        //

        DEBUG_PRINT(SESSION,
                    INFO,
                    ("OK to create new connection\n"
                    ));

        DPRINTF("%#x: %#x: *** %s OK to create connection %d/%d\n",
                GetCurrentThreadId(),
                Fsm,
                GetHostName(),
                m_ConnectionsAvailable,
                m_ConnectionLimit
                );

        --m_ConnectionsAvailable;
    } else if (fsm.GetElapsedTime() > fsm.m_dwTimeout) {
        error = ERROR_INTERNET_TIMEOUT;
    } else {

        //
        // if there are keep-alive connections but no keep-alive waiters
        // then either we don't want a keep-alive connection, or the ones
        // available don't match our requirements.
        // If we need a connection of a different type - e.g. SSL when all
        // we have is non-SSL then close a connection & generate a new one.
        // If we need a non-keep-alive connection then its okay to return
        // a current keep-alive connection, the understanding being that the
        // caller will not add Connection: Keep-Alive header (HTTP 1.0) or
        // will add Connection: Close header (HTTP 1.1)
        //

        //
        // BUGBUG - what about waiters for non-keep-alive connections?
        //
        // scenario - limit of 1 connection:
        //
        //  A. request for k-a
        //      continue & create connection
        //  B. request non-k-a
        //      none available; wait
        //  C. release k-a connection; unblock sync waiter B
        //  D. request non-k-a
        //      k-a available; return it; caller converts to non-k-a
        //  E. unblocked waiter B request non-k-a
        //      none available; wait
        //
        // If this situation continues, eventually B will time-out, whereas it
        // could have had the connection taken by D. Request D is younger and
        // therefore can afford to wait while B continues with the connection
        //

        BOOL fHaveConnection = FALSE;

        if (!bKeepAliveWaiters || (fsm.GetState() == FSM_STATE_CONTINUE)) {
            LockSerializedList(&m_KeepAliveList);
            if (ElementsOnSerializedList(&m_KeepAliveList) != 0) {
                pSocket = ContainingICSocket(SlDequeueHead(&m_KeepAliveList));
                fHaveConnection = TRUE;

#define SOCK_FLAGS  (SF_ENCRYPT | SF_DECRYPT | SF_SECURE)

                DWORD dwSocketTypeFlags = pSocket->GetFlags() & SOCK_FLAGS;
                DWORD dwRequestTypeFlags = fsm.m_dwSocketFlags & SOCK_FLAGS;

                if ((dwSocketTypeFlags ^ dwRequestTypeFlags)
                    || (fsm.m_nPort != pSocket->GetPort())) {

                    DEBUG_PRINT(SESSION,
                                INFO,
                                ("different socket types (%#x, %#x) or ports (%d, %d) requested\n",
                                fsm.m_dwSocketFlags,
                                pSocket->GetFlags(),
                                fsm.m_nPort,
                                pSocket->GetPort()
                                ));

                    DPRINTF("%#x: %#x: *** closing socket %#x: %#x vs. %#x\n",
                            GetCurrentThreadId(),
                            Fsm,
                            pSocket->GetSocket(),
                            pSocket->GetFlags(),
                            fsm.m_dwSocketFlags
                            );

                    pSocket->SetLinger(FALSE, 0);
                    pSocket->Shutdown(2);
//dprintf("GetConnection: destroying different type socket %#x\n", pSocket->GetSocket());
                    pSocket->Destroy();
                    pSocket = NULL;
                    //if (!UnlimitedConnections()) {
                    //    ++m_ConnectionsAvailable;
                    //}
                } else {

                    DPRINTF("%#x: %#x: *** returning k-a connection %#x as non-k-a\n",
                            GetCurrentThreadId(),
                            Fsm,
                            pSocket->GetSocket()
                            );

                }
                CHECK_CONNECTION_COUNT();
            }
            UnlockSerializedList(&m_KeepAliveList);
            if (fHaveConnection) {
                goto exit;
            }
        }

        DPRINTF("%#x: %#x: blocking %s FSM %#x state %s %d/%d\n",
                GetCurrentThreadId(),
                Fsm,
                Fsm->MapType(),
                Fsm,
                Fsm->MapState(),
                m_ConnectionsAvailable,
                m_ConnectionLimit
                );

        //
        // we have to wait for a connection to become available. If we are an
        // async request then we queue this FSM & return the thread to the pool
        // or, if app thread, return pending indication to the app. If this is
        // a sync request (in an app thread) then we block on an event waiting
        // for a connection to become available
        //

        if (!bAsyncRequest) {

            //
            // create unnamed, initially unsignalled, auto-reset event
            //

            hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
            if (hEvent == NULL) {
                error = GetLastError();
                goto exit;
            }
        }

        CConnectionWaiter * pWaiter;

#if INET_DEBUG

        for (pWaiter = (CConnectionWaiter *)m_Waiters.Head();
             pWaiter != (CConnectionWaiter *)m_Waiters.Self();
             pWaiter = (CConnectionWaiter *)pWaiter->Next()) {

            INET_ASSERT(pWaiter->Id() != (DWORD_PTR)(bAsyncRequest ? (DWORD_PTR)Fsm : lpThreadInfo->ThreadId));
        }
#endif

        pWaiter = new CConnectionWaiter(&m_Waiters,
                                        !bAsyncRequest,
                                        (fsm.m_dwSocketFlags & SF_KEEP_ALIVE)
                                            ? TRUE
                                            : FALSE,
                                        bAsyncRequest
                                            ? (DWORD_PTR)Fsm
                                            : lpThreadInfo->ThreadId,
                                        hEvent,

                                        //
                                        // priority in request handle object
                                        // controls relative position in list
                                        // of waiters
                                        //

                                        ((HTTP_REQUEST_HANDLE_OBJECT *)
                                            lpThreadInfo->hObjectMapped)->
                                                GetPriority()
                                        );

        DPRINTF("%#x: %#x: new waiter %#x: as=%B, K-A=%B, id=%#x, hE=%#x, pri=%d\n",
                GetCurrentThreadId(),
                Fsm,
                pWaiter,
                bAsyncRequest,
                (fsm.m_dwSocketFlags & SF_KEEP_ALIVE)
                    ? TRUE
                    : FALSE,
                bAsyncRequest
                    ? (DWORD_PTR)Fsm
                    : lpThreadInfo->ThreadId,
                hEvent,
                ((HTTP_REQUEST_HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->
                    GetPriority()
                );

        if (pWaiter == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto exit;
        }
        if (bAsyncRequest) {

            //
            // ensure that when the FSM is unblocked normally, the new state
            // is STATE_CONTINUE
            //

            Fsm->SetState(FSM_STATE_CONTINUE);
            error = BlockWorkItem(Fsm,
                                  (DWORD_PTR)pWaiter,
                                  fsm.m_dwTimeout
                                  );
            if (error == ERROR_SUCCESS) {
                error = ERROR_IO_PENDING;
            }
        } else {
            m_Waiters.Release();
            bUnlockList = FALSE;

            DPRINTF("%#x: %#x: %s FSM %#x %s waiting %d msec\n",
                    GetCurrentThreadId(),
                    Fsm,
                    Fsm->MapType(),
                    Fsm,
                    Fsm->MapState(),
                    fsm.m_dwTimeout
                    );

            DWORD dwWaitTime = fsm.m_dwTimeout - fsm.GetElapsedTime();

            if ((int)dwWaitTime <= 0) {

                DEBUG_PRINT(SESSION,
                            ERROR,
                            ("SYNC wait timed out (%d mSec)\n",
                            fsm.m_dwTimeout
                            ));

                error = ERROR_INTERNET_TIMEOUT;
            } else {

                DEBUG_PRINT(SESSION,
                            INFO,
                            ("waiting %d mSec for SYNC event %#x\n",
                            dwWaitTime,
                            hEvent
                            ));

                //
                // we'd better not be doing a sync wait if we are in the
                // context of an app thread making an async request
                //

                INET_ASSERT(lpThreadInfo->IsAsyncWorkerThread
                            || !((INTERNET_HANDLE_OBJECT *)lpThreadInfo->
                                hObjectMapped)->IsAsyncHandle());

                //INET_ASSERT(dwWaitTime <= 60000);

                error = WaitForSingleObject(hEvent, dwWaitTime);

                DPRINTF("%#x: %#x: sync waiter unblocked - error = %d\n",
                        GetCurrentThreadId(),
                        Fsm,
                        error
                        );

            }
            if (error == STATUS_TIMEOUT) {

                DPRINTF("%#x: %#x: %s: %d+%d/%d: timed out %#x (%s FSM %#x %s)\n",
                        GetCurrentThreadId(),
                        Fsm,
                        GetHostName(),
                        m_ConnectionsAvailable,
                        ElementsOnSerializedList(&m_KeepAliveList),
                        m_ConnectionLimit,
                        GetCurrentThreadId(),
                        Fsm->MapType(),
                        Fsm,
                        Fsm->MapState()
                        );

                RemoveWaiter(lpThreadInfo->ThreadId);
                error = ERROR_INTERNET_TIMEOUT;
            }

            BOOL bOk;

            bOk = CloseHandle(hEvent);

            INET_ASSERT(bOk);

            if (error == WAIT_OBJECT_0) {

                DPRINTF("%#x: %#x: sync requester trying again\n",
                        GetCurrentThreadId(),
                        Fsm
                        );

                fsm.SetState(FSM_STATE_CONTINUE);
                goto try_again;
            }
        }
    }

exit:

    //
    // if we are returning a (keep-alive) socket that has a different blocking
    // mode from that requested, change it
    //

    if (pSocket != NULL) {
        if ((pSocket->GetFlags() & SF_NON_BLOCKING)
            ^ (fsm.m_dwSocketFlags & SF_NON_BLOCKING)) {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("different blocking modes requested: %#x, %#x\n",
                        fsm.m_dwSocketFlags,
                        pSocket->GetFlags()
                        ));

            DPRINTF("%#x: %#x: *** changing socket %#x to %sBLOCKING\n",
                    GetCurrentThreadId(),
                    Fsm,
                    pSocket->GetSocket(),
                    fsm.m_dwSocketFlags & SF_NON_BLOCKING ? "NON-" : ""
                    );

            if (!(GlobalRunningNovellClient32 && !GlobalNonBlockingClient32)) {
                pSocket->SetNonBlockingMode(fsm.m_dwSocketFlags & SF_NON_BLOCKING);
            }
        }
        *fsm.m_lplpSocket = pSocket;
    }

    if (bUnlockList) {
        m_Waiters.Release();
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DPRINTF("%#x: %#x: %s: %d+%d/%d: get: %d, %#x, %d\n",
            GetCurrentThreadId(),
            Fsm,
            GetHostName(),
            m_ConnectionsAvailable,
            ElementsOnSerializedList(&m_KeepAliveList),
            m_ConnectionLimit,
            error,
            pSocket ? pSocket->GetSocket() : 0,
            m_Waiters.Count()
            );

    PERF_LEAVE(GetConnection);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CServerInfo::ReleaseConnection(
    IN ICSocket * lpSocket OPTIONAL
    )

/*++

Routine Description:

    Returns a keep-alive connection to the pool, or allows another requester to
    create a connection

Arguments:

    lpSocket    - pointer to ICSocket if we are returning a keep-alive connection

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 Dword,
                 "CServerInfo::ReleaseConnection",
                 "{%q [%d+%d/%d]} %#x [%#x]",
                 GetHostName(),
                 AvailableConnections(),
                 KeepAliveConnections(),
                 ConnectionLimit(),
                 lpSocket,
                 lpSocket ? lpSocket->GetSocket() : 0
                 ));

    PERF_ENTER(ReleaseConnection);

    DWORD error = ERROR_SUCCESS;
    BOOL bRelease = FALSE;

    m_Waiters.Acquire();

    //
    // quite often (at least with catapult proxy based on IIS) the server may
    // drop the connection even though it indicated it would keep it open. This
    // typically happens on 304 (frequent) and 302 (less so) responses. If we
    // determine the server has dropped the connection then throw it away and
    // allow the app to create a new one
    //

    if (lpSocket != NULL) {
        if (lpSocket->IsClosed() || lpSocket->IsReset()) {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("socket %#x already dead - throwing it out\n",
                        lpSocket->GetSocket()
                        ));

            DPRINTF("%#x: socket %#x: already reset\n",
                    GetCurrentThreadId(),
                    lpSocket->GetSocket()
                    );

//dprintf("ReleaseConnection: destroying already closed socket %#x\n", lpSocket->GetSocket());
            BOOL bDestroyed = lpSocket->Dereference();

            INET_ASSERT(bDestroyed);

            lpSocket = NULL;
        } else {

            //
            // if we are returning a keep-alive socket, put it in non-blocking
            // mode if not already. Typically, Internet Explorer uses non-blocking
            // sockets. In the infrequent cases where we want a blocking socket
            // - mainly when doing java downloads - we will convert the socket
            // to blocking mode when we get it from the pool
            //

            if (!lpSocket->IsNonBlocking()) {

                DPRINTF("%#x: ***** WARNING: releasing BLOCKING k-a socket %#x\n",
                        GetCurrentThreadId(),
                        lpSocket->GetSocket()
                        );

                if (!(GlobalRunningNovellClient32 && !GlobalNonBlockingClient32)) {
                    lpSocket->SetNonBlockingMode(TRUE);
                }
            }
        }
    }
    if (lpSocket != NULL) {

        DPRINTF("%#x: releasing K-A %#x (%d+%d/%d)\n",
                GetCurrentThreadId(),
                lpSocket ? lpSocket->GetSocket() : 0,
                AvailableConnections(),
                KeepAliveConnections(),
                ConnectionLimit()
                );

        INET_ASSERT(lpSocket->IsOpen());
        INET_ASSERT(!lpSocket->IsOnList());
        //INET_ASSERT(!lpSocket->IsReset());

        lpSocket->SetKeepAlive();

        DEBUG_PRINT(SESSION,
                    INFO,
                    ("releasing keep-alive socket %#x\n",
                    lpSocket->GetSocket()
                    ));

        lpSocket->SetExpiryTime(GlobalKeepAliveSocketTimeout);

        INET_ASSERT(!IsOnSerializedList(&m_KeepAliveList, lpSocket->List()));

        InsertAtTailOfSerializedList(&m_KeepAliveList, lpSocket->List());

        INET_ASSERT(UnlimitedConnections()
            ? TRUE
            : (KeepAliveConnections() <= ConnectionLimit()));

        bRelease = TRUE;
    } else {

        DPRINTF("%#x: releasing connection (%d+%d/%d)\n",
                GetCurrentThreadId(),
                AvailableConnections(),
                KeepAliveConnections(),
                ConnectionLimit()
                );

        if (!UnlimitedConnections()) {
            ++m_ConnectionsAvailable;
        }

        CHECK_CONNECTION_COUNT();

        bRelease = TRUE;
    }
    if (bRelease && !UnlimitedConnections()) {

        CHECK_CONNECTION_COUNT();

        CConnectionWaiter * pWaiter = (CConnectionWaiter *)m_Waiters.RemoveHead();

        if (pWaiter != NULL) {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("unblocking %s waiter %#x, pri=%d\n",
                        pWaiter->IsSync() ? "SYNC" : "ASYNC",
                        pWaiter->Id(),
                        pWaiter->GetPriority()
                        ));

            DPRINTF("%#x: Unblocking %s connection waiter %#x, pri=%d\n",
                    GetCurrentThreadId(),
                    pWaiter->IsSync() ? "Sync" : "Async",
                    pWaiter->Id(),
                    pWaiter->GetPriority()
                    );

            if (pWaiter->IsSync()) {
                pWaiter->Signal();
            } else {

                DWORD n = UnblockWorkItems(1, (DWORD_PTR)pWaiter, ERROR_SUCCESS);

                //INET_ASSERT(n == 1);
            }
            delete pWaiter;
        } else {

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("no waiters\n"
                        ));

            DPRINTF("%#x: !!! NOT unblocking connection waiter\n",
                    GetCurrentThreadId()
                    );

        }
    } else {

        DPRINTF("%#x: !!! NOT releasing or unlimited?\n",
                GetCurrentThreadId()
                );

        DEBUG_PRINT(SESSION,
                    INFO,
                    ("bRelease = %B, UnlimitedConnections() = %B\n",
                    bRelease,
                    UnlimitedConnections()
                    ));

    }

    DEBUG_PRINT(SESSION,
                INFO,
                ("avail+k-a/limit = %d+%d/%d\n",
                AvailableConnections(),
                KeepAliveConnections(),
                ConnectionLimit()
                ));

    if (IsNewLimit()) {
        UpdateConnectionLimit();
    }

    m_Waiters.Release();

    PERF_LEAVE(ReleaseConnection);

    DEBUG_LEAVE(error);

    DPRINTF("%#x: %s: %d+%d/%d: rls %#x: %d, %d\n",
            GetCurrentThreadId(),
            GetHostName(),
            AvailableConnections(),
            KeepAliveConnections(),
            ConnectionLimit(),
            lpSocket ? lpSocket->GetSocket() : 0,
            error,
            m_Waiters.Count()
            );

    return error;
}

#endif // NEW_CONNECTION_SCHEME


VOID
CServerInfo::RemoveWaiter(
    IN DWORD_PTR dwId
    )

/*++

Routine Description:

    Removes a CConnectionWaiter corresponding to the FSM

Arguments:

    dwId    - waiter id to match

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::RemoveWaiter",
                 "%#x",
                 dwId
                 ));

    m_Waiters.Acquire();

    CConnectionWaiter * pWaiter;
    BOOL found = FALSE;

    for (pWaiter = (CConnectionWaiter *)m_Waiters.Head();
         pWaiter != (CConnectionWaiter *)m_Waiters.Self();
         pWaiter = (CConnectionWaiter *)pWaiter->Next()) {

        if (pWaiter->Id() == dwId) {
            m_Waiters.Remove((CPriorityListEntry *)pWaiter);
            delete pWaiter;
            found = TRUE;
            break;
        }
    }
    m_Waiters.Release();

    //INET_ASSERT(found);

    DEBUG_LEAVE(0);
}

//
// private CServerInfo methods
//


ICSocket *
CServerInfo::FindKeepAliveConnection(
    IN DWORD dwSocketFlags,
    IN INTERNET_PORT nPort
    )

/*++

Routine Description:

    Find a keep-alive connection with the requested attributes and port number

Arguments:

    dwSocketFlags   - socket type flags (e.g. SF_SECURE)

    nPort           - port to server

Return Value:

    ICSocket *

--*/

{
    DPRINTF("%#x: *** looking for K-A connection\n", GetCurrentThreadId());

    DEBUG_ENTER((DBG_SESSION,
                 Pointer,
                 "CServerInfo::FindKeepAliveConnection",
                 "{%q} %#x, %d",
                 GetHostName(),
                 dwSocketFlags,
                 nPort
                 ));

    ICSocket * pSocket = NULL;
    BOOL bFound = FALSE;

    //
    // don't check whether socket is non-blocking - we only really want to match
    // on secure/non-secure. Possible flags to check on are:
    //
    //  SF_ENCRYPT          - should be subsumed by SF_SECURE
    //  SF_DECRYPT          - should be subsumed by SF_SECURE
    //  SF_NON_BLOCKING     - this isn't criterion for match
    //  SF_CONNECTIONLESS   - not implemented?
    //  SF_AUTHORIZED       - must be set if authorized & in pool
    //  SF_SECURE           - opened for SSL/PCT if set
    //  SF_KEEP_ALIVE       - must be set
    //  SF_TUNNEL           - must be set if we're looking for a CONNECT tunnel to proxy
    //

    dwSocketFlags &= ~SF_NON_BLOCKING;

    LockSerializedList(&m_KeepAliveList);

    PLIST_ENTRY pEntry;

    for (pEntry = HeadOfSerializedList(&m_KeepAliveList);
         pEntry != (PLIST_ENTRY)SlSelf(&m_KeepAliveList);
         pEntry = pEntry->Flink) {

        pSocket = ContainingICSocket(pEntry);

        INET_ASSERT(pSocket->IsKeepAlive());

        //
        // We make sure the socket we request is the correct socket,
        //  Match() is a bit confusing and needs a bit of explaining,
        //  Match IS NOT AN EXACT MATCH, it mearly checks to make sure 
        //  that the requesting flags (dwSocketFlags) are found in the 
        //  socket flags.  So this can lead to a secure socket being returned
        //  on a non-secure open request, now realistically this doesn't happen
        //  because of the port number.  But in the case of tunnelling this may be
        //  an issue, so we add an additional check to make sure that we only
        //  get a tunneled socket to a proxy if we specifically request one.
        //

        if (pSocket->Match(dwSocketFlags)
        && (pSocket->GetPort() == nPort)
        &&  pSocket->MatchTunnelSemantics(dwSocketFlags)) {
            RemoveFromSerializedList(&m_KeepAliveList, pSocket->List());

            INET_ASSERT(!IsOnSerializedList(&m_KeepAliveList, pSocket->List()));

            bFound = TRUE;

            DEBUG_PRINT(SESSION,
                        INFO,
                        ("returning keep-alive socket %#x\n",
                        pSocket->GetSocket()
                        ));

            DPRINTF("%#x: *** %s keep-alive connection %#x (%d/%d)\n",
                    GetCurrentThreadId(),
                    GetHostName(),
                    pSocket->GetSocket(),
                    AvailableConnections(),
                    ConnectionLimit()
                    );

            break;
        }
    }
    UnlockSerializedList(&m_KeepAliveList);
    if (!bFound) {
        pSocket = NULL;
    }

    DEBUG_LEAVE(pSocket);

    return pSocket;
}


BOOL
CServerInfo::KeepAliveWaiters(
    VOID
    )

/*++

Routine Description:

    Determine if any of the waiters on the list are for keep-alive connections

Arguments:

    None.

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 Bool,
                 "CServerInfo::KeepAliveWaiters",
                 NULL
                 ));

    BOOL found = FALSE;
    CConnectionWaiter * pWaiter;

    m_Waiters.Acquire();
    for (pWaiter = (CConnectionWaiter *)m_Waiters.Head();
         pWaiter != (CConnectionWaiter *)m_Waiters.Self();
         pWaiter = (CConnectionWaiter *)pWaiter->Next()) {

        if (pWaiter->IsKeepAlive()) {
            found = TRUE;
            break;
        }
    }
    m_Waiters.Release();

    DEBUG_LEAVE(found);

    return found;
}

#ifdef NEW_CONNECTION_SCHEME
//
//
//BOOL
//CServerInfo::RunOutOfConnections(
//    VOID
//    )
//
///*++
//
//Routine Description:
//
//    Determines whether we have been run out of connections. Criteria for being
//    out of connections is: no available connections or keep-alive connections
//    and no recorded connection activity within GlobalConnectionInactiveTimeout
//
//Arguments:
//
//    None.
//
//Return Value:
//
//    BOOL
//        TRUE    - we are out of connections
//
//        FALSE   - not
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_SESSION,
//                 Bool,
//                 "CServerInfo::RunOutOfConnections",
//                 NULL
//                 ));
//
//    BOOL bOut = FALSE;
//
//    if ((TotalAvailableConnections() == 0)
//    && (GetLastActiveTime() != 0)
//    && !ConnectionActivity()) {
//
//        DPRINTF("%#x: >>>>>>>> Run out of connections. Last activity %d mSec ago. Create new\n",
//                GetCurrentThreadId(),
//                (GetLastActiveTime() == 0)
//                    ? -1
//                    : (GetTickCount() - GetLastActiveTime())
//                );
//
//        INET_ASSERT(!UnlimitedConnections());
//
//        DEBUG_PRINT(SESSION,
//                    WARNING,
//                    ("out of connections! Last activity %d mSec ago. Creating new\n",
//                    (GetLastActiveTime() == 0)
//                        ? -1
//                        : (GetTickCount() - GetLastActiveTime())
//                    ));
//
//        bOut = TRUE;
//    } else if (TotalAvailableConnections() == 0) {
//
//        DPRINTF("%#x: no connections, last activity %d mSec ago\n",
//                GetCurrentThreadId(),
//                (GetLastActiveTime() == 0)
//                    ? -1
//                    : (GetTickCount() - GetLastActiveTime())
//                );
//
//        DEBUG_PRINT(SESSION,
//                    INFO,
//                    ("no connections, last activity %d mSec ago\n",
//                    (GetLastActiveTime() == 0)
//                        ? -1
//                        : (GetTickCount() - GetLastActiveTime())
//                    ));
//
//    }
//
//    DEBUG_LEAVE(bOut);
//
//    return bOut;
//}
//
#endif // NEW_CONNECTION_SCHEME


VOID
CServerInfo::UpdateConnectionLimit(
    VOID
    )

/*++

Routine Description:

    Change connection limit to new limit

    Assumes: 1. Caller has acquired this object before calling this function

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::UpdateConnectionLimit",
                 "{%q: %d=>%d (%d+%d)}",
                 GetHostName(),
                 ConnectionLimit(),
                 GetNewLimit(),
                 AvailableConnections(),
                 KeepAliveConnections()
                 ));

    LONG difference = GetNewLimit() - ConnectionLimit();

    //
    // BUGBUG - only handling increases in limit for now
    //

    INET_ASSERT(difference > 0);

    if (difference > 0) {
        m_ConnectionsAvailable += difference;
    }
    m_ConnectionLimit = m_NewLimit;

    DEBUG_PRINT(SESSION,
                INFO,
                ("%q: new: %d+%d/%d\n",
                GetHostName(),
                AvailableConnections(),
                KeepAliveConnections(),
                ConnectionLimit()
                ));

    DEBUG_LEAVE(0);
}


VOID
CServerInfo::PurgeKeepAlives(
    IN DWORD dwForce
    )

/*++

Routine Description:

    Purges any timed-out keep-alive connections

Arguments:

    dwForce - force to apply when purging. Value can be:

                PKA_NO_FORCE    - only purge timed-out sockets or sockets in
                                  close-wait state (default)

                PKA_NOW         - purge all sockets

                PKA_AUTH_FAILED - purge sockets that have been marked as failing
                                  authentication

Return Value:

    None.

--*/

{
//dprintf("%#x PurgeKeepAlives(%d)\n", GetCurrentThreadId(), dwForce);
    DEBUG_ENTER((DBG_SESSION,
                 None,
                 "CServerInfo::PurgeKeepAlives",
                 "{%q [ref=%d, k-a=%d]} %s [%d]",
                 GetHostName(),
                 ReferenceCount(),
                 KeepAliveConnections(),
                 (dwForce == PKA_NO_FORCE) ? "NO_FORCE"
                 : (dwForce == PKA_NOW) ? "NOW"
                 : (dwForce == PKA_AUTH_FAILED) ? "AUTH_FAILED"
                 : "?",
                 dwForce
                 ));

    if (IsKeepAliveListInitialized()) {

        INET_ASSERT(ReferenceCount() >= 1);

        m_Waiters.Acquire();
        LockSerializedList(&m_KeepAliveList);

        PLIST_ENTRY last = (PLIST_ENTRY)SlSelf(&m_KeepAliveList);
        DWORD ticks = GetTickCount();

        for (PLIST_ENTRY pEntry = HeadOfSerializedList(&m_KeepAliveList);
            pEntry != (PLIST_ENTRY)SlSelf(&m_KeepAliveList);
            pEntry = last->Flink) {

            ICSocket * pSocket = ContainingICSocket(pEntry);
            BOOL bDelete;

            if (pSocket->IsReset()) {
//dprintf("%q: socket %#x/%d CLOSE-WAIT\n", GetHostName(), pSocket->GetSocket(), pSocket->GetSourcePort());
                bDelete = TRUE;
            } else if (dwForce == PKA_NO_FORCE) {
                bDelete = pSocket->HasExpired(ticks);
            } else if (dwForce == PKA_NOW) {
                bDelete = TRUE;
            } else if (dwForce == PKA_AUTH_FAILED) {
                bDelete = pSocket->IsAuthorized();
            }
            if (bDelete) {
//dprintf("%q: socket %#x/%d. Close-Wait=%B, Expired=%B, Now=%B, Auth=%B\n",
//        GetHostName(),
//        pSocket->GetSocket(),
//        pSocket->GetSourcePort(),
//        pSocket->IsReset(),
//        pSocket->HasExpired(ticks),
//        (dwForce == PKA_NOW),
//        (dwForce == PKA_AUTH_FAILED) && pSocket->IsAuthorized()
//        );

                DEBUG_PRINT(SESSION,
                            INFO,
                            ("purging keep-alive socket %#x/%d: Close-Wait=%B, Expired=%B, Now=%B, Auth=%B\n",
                            pSocket->GetSocket(),
                            pSocket->GetSourcePort(),
                            pSocket->IsReset(),
                            pSocket->HasExpired(ticks),
                            (dwForce == PKA_NOW),
                            (dwForce == PKA_AUTH_FAILED) && pSocket->IsAuthorized()
                            ));

                RemoveFromSerializedList(&m_KeepAliveList, pEntry);

                BOOL bDestroyed;

                bDestroyed = pSocket->Dereference();

                INET_ASSERT(bDestroyed);

                if (!UnlimitedConnections()) {
                    ++m_ConnectionsAvailable;

                    INET_ASSERT(m_ConnectionsAvailable <= m_ConnectionLimit);

                }
            } else {

                DEBUG_PRINT(SESSION,
                            INFO,
                            ("socket %#x/%d expires in %d mSec\n",
                            pSocket->GetSocket(),
                            pSocket->GetSourcePort(),
                            pSocket->GetExpiryTime() - ticks
                            ));

                last = pEntry;
            }
        }

        UnlockSerializedList(&m_KeepAliveList);
        m_Waiters.Release();
    }

    DEBUG_LEAVE(0);
}

//
// friend functions
//


CServerInfo *
ContainingServerInfo(
    IN LPVOID lpAddress
    )

/*++

Routine Description:

    Returns address of CServerInfo given address of m_List

Arguments:

    lpAddress   - address of m_List

Return Value:

    CServerInfo *

--*/

{
    return CONTAINING_RECORD(lpAddress, CServerInfo, m_List);
}
