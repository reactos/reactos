/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    proxysup.hxx

Abstract:

    Contains class definition for proxy server and proxy bypass list

    Contents:
        PROXY_BYPASS_LIST_ENTRY
        PROXY_BYPASS_LIST
        PROXY_SERVER_LIST_ENTRY
        PROXY_SERVER_LIST
        PROXY_INFO
        PROXY_STATE
        BAD_PROXY_LIST_ENTRY
        BAD_PROXY_LIST

Author:

    Richard L Firth (rfirth) 03-Feb-1996

Revision History:

    03-Feb-1996 rfirth
        Created

    29-Nov-1996 arthurbi
        Added support for external Auto-Proxy Dlls

--*/

//
// defintions
//

#define MAX_BAD_PROXY_ENTRIES            20
#define BAD_PROXY_EXPIRES_TIME           (30 * 60 * (LONGLONG)10000000) // 30 minutes


//
// prototypes
//

BOOL
IsLocalMacro(
    IN LPSTR lpszMetaName,
    IN DWORD dwMetaNameLength
    );



//
// class implementations
//

class CServerInfo;
class  PROXY_INFO_GLOBAL;
extern PROXY_INFO_GLOBAL GlobalProxyInfo;


//
// BAD_PROXY_LIST_ENTRY - represents a proxy that is has failed
//   while being used, for some reason, such as DNS or connection failure.
//

class BAD_PROXY_LIST_ENTRY {

public:

    //
    // _ftLastFailed - Last time this proxy was accessed and failed
    //

    FILETIME _ftLastFailed;

    //
    // _fEntryUsed  - Since these objects are initalily part of an array
    //      this boolean tells whether its actually a valid entry
    //

    BOOL     _fEntryUsed;

    //
    // _lpszProxyName - Name of the Proxy that is bad
    //

    LPSTR    _lpszProxyName;

    //
    // _ipProxyPort - Port number of the proxy
    //

    INTERNET_PORT _ipProxyPort;

    //
    // destructor ...
    //

    ~BAD_PROXY_LIST_ENTRY()
    {
        if ( _fEntryUsed && _lpszProxyName )
        {
            FREE_MEMORY(_lpszProxyName);
        }
    }


    BOOL
    IsEntryExpired( FILETIME *pCurrentGmtTime )
    {
        LONGLONG ElapsedTime;

        ElapsedTime = (*(LONGLONG *)pCurrentGmtTime) - (*(LONGLONG *)&_ftLastFailed);

        if( ElapsedTime > BAD_PROXY_EXPIRES_TIME )
        {
            return TRUE;
        }

        return FALSE;
    }

    BOOL
    IsMatch( LPSTR lpszProxyName, INTERNET_PORT ipProxyPort )
    {
        if ( _lpszProxyName &&
             _ipProxyPort == ipProxyPort &&
             lstrcmpi(lpszProxyName, _lpszProxyName ) == 0 )
        {
            return TRUE;
        }

        return FALSE;
    }

    DWORD
    SetEntry( LPSTR lpszProxyName, INTERNET_PORT ipProxyPort, FILETIME ftLastFailed )
    {
        if ( _fEntryUsed )
        {
            INET_ASSERT(_lpszProxyName );
            FREE_MEMORY(_lpszProxyName );
            _fEntryUsed = FALSE;
        }

        _lpszProxyName =
            NewString(lpszProxyName);        

        if ( ! _lpszProxyName )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }

        _ipProxyPort  = ipProxyPort;
        _ftLastFailed = ftLastFailed ;
        _fEntryUsed   = TRUE;

        return ERROR_SUCCESS;
    }

};


//
// BAD_PROXY_LIST - maintains a list of proxies that have are done, or not working
//   properly to satisfy requests.
//

class BAD_PROXY_LIST {

private:

    //
    // rgBadEntries - Array of proxies that have failed.
    //

    BAD_PROXY_LIST_ENTRY rgBadEntries[MAX_BAD_PROXY_ENTRIES];

    //
    // _dwEntries - Number of entries 
    //

    DWORD _dwEntries;

public:

    BAD_PROXY_LIST()
    {
        _dwEntries = 0;
        for ( DWORD i = 0; i < ARRAY_ELEMENTS(rgBadEntries); i++ )
        {
            rgBadEntries[i]._fEntryUsed = FALSE;
        }
    }

    VOID
    CheckForExpiredEntries(VOID)
    {
        FILETIME ftCurTime;

        if ( _dwEntries == 0 ) {
            return;
        }

        GetCurrentGmtTime(&ftCurTime);

        for ( DWORD i = 0; i < ARRAY_ELEMENTS(rgBadEntries); i++ )
        {
            if (   rgBadEntries[i]._fEntryUsed &&
                   rgBadEntries[i].IsEntryExpired(&ftCurTime) )
            {
                _dwEntries--;
                rgBadEntries[i]._fEntryUsed = FALSE;
                UPDATE_GLOBAL_PROXY_VERSION();        
            }
        }
    }


    DWORD
    AddEntry( LPSTR lpszProxyHost, INTERNET_PORT ipProxyPort)
    {
        DWORD i;
        FILETIME ftCurTime;

        GetCurrentGmtTime(&ftCurTime);

        UPDATE_GLOBAL_PROXY_VERSION();

        //
        // BUGBUG [arthurbi] this should be combined into one loop.
        //

        //
        // Check to see if the new entry is already in the array,
        //  if so use it.
        //

        for ( i = 0; i < ARRAY_ELEMENTS(rgBadEntries); i++ )
        {
            if (   rgBadEntries[i]._fEntryUsed &&
                   rgBadEntries[i].IsMatch(lpszProxyHost, ipProxyPort) )
            {
                rgBadEntries[i]._ftLastFailed = ftCurTime;
                return ERROR_SUCCESS;
            }
        }

        //
        // Check to see if there is an unused entry, or an
        //  entry that has expired.
        //

        for ( i = 0; i < ARRAY_ELEMENTS(rgBadEntries); i++ )
        {
            if ( ! rgBadEntries[i]._fEntryUsed )
            {
                break;
            }
            
            if ( rgBadEntries[i].IsEntryExpired(&ftCurTime) )
            {
                _dwEntries --;
                break;
            }
        }

        //
        // If we haven't found an array entry we can use,
        //  Find the oldest entry in the array.
        //

        if ( i == ARRAY_ELEMENTS(rgBadEntries) )
        {
            DWORD dwOldestIndex = 0;
            FILETIME ftOldest = rgBadEntries[0]._ftLastFailed;

            for ( i = 0; i < ARRAY_ELEMENTS(rgBadEntries); i++ )
            {
                INET_ASSERT(rgBadEntries[i]._fEntryUsed);

                if ( (*(LONGLONG *)&rgBadEntries[i]._ftLastFailed) < (*(LONGLONG *)&ftOldest) )
                {
                    ftOldest = rgBadEntries[i]._ftLastFailed;
                    dwOldestIndex = i;
                    _dwEntries --;

                }
            }

            i = dwOldestIndex;
        }

        INET_ASSERT(i != ARRAY_ELEMENTS(rgBadEntries));
        _dwEntries ++;

        return rgBadEntries[i].SetEntry(lpszProxyHost, ipProxyPort, ftCurTime);

    }


    BOOL
    IsBadProxyName(LPSTR lpszProxyHost, INTERNET_PORT ipProxyPort)
    {
        DWORD i;
        FILETIME ftCurTime;

        //
        // Try to find this proxy in our "black list" of bad
        //   proxys.  If we find return TRUE.
        //

        GetCurrentGmtTime(&ftCurTime);

        for ( i = 0; i < ARRAY_ELEMENTS(rgBadEntries); i++ )
        {
            if (   rgBadEntries[i]._fEntryUsed &&
                   rgBadEntries[i].IsMatch(lpszProxyHost, ipProxyPort) &&
                 ! rgBadEntries[i].IsEntryExpired(&ftCurTime) )
            {
                return TRUE;
            }
        }

        return FALSE;
    }

    VOID
    ClearList(
        VOID
        )
    {
        DWORD i;

        for ( i = 0; i < ARRAY_ELEMENTS(rgBadEntries); i++ )
        {            
            rgBadEntries[i]._fEntryUsed = FALSE;
        }
        _dwEntries = 0;
    }
};

//
// PROXY_BYPASS_LIST_ENTRY - describes a server or address range that should
// not go via proxy
//

class PROXY_BYPASS_LIST_ENTRY {

friend class PROXY_BYPASS_LIST;

private:

    //
    // _List - there may be a variable number of bypass entries
    //

    LIST_ENTRY _List;

    //
    // _Scheme - scheme to be bypassed if significant, else 0
    //

    INTERNET_SCHEME _Scheme;

    //
    // _Name - the name of the server to bypass the proxy. May contain wildcard
    // characters (*)
    //

    ICSTRING _Name;

    //
    // _Port - port number if significant, else 0
    //

    INTERNET_PORT _Port;

    //
    // _LocalSemantics - TRUE if we match local addresses (non-IP address names
    // not containing periods)
    //

    BOOL _LocalSemantics;

public:

    PROXY_BYPASS_LIST_ENTRY(
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort
        ) {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "PROXY_BYPASS_LIST_ENTRY",
                    "%s, %q, %d, %d",
                    InternetMapScheme(tScheme),
                    lpszHostName,
                    dwHostNameLength,
                    nPort
                    ));

        InitializeListHead(&_List);
        _Scheme = tScheme;
        if (IsLocalMacro(lpszHostName, dwHostNameLength)) {

            DEBUG_PRINT(PROXY,
                        INFO,
                        ("<local>\n"
                        ));

            _Name = NULL;
            _LocalSemantics = TRUE;
        } else {
            _Name.MakeCopy(lpszHostName, dwHostNameLength);
            _Name.MakeLowerCase();
            _LocalSemantics = FALSE;
        }
        _Port = nPort;

        DEBUG_LEAVE(0);
    }

    ~PROXY_BYPASS_LIST_ENTRY() {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "~PROXY_BYPASS_LIST_ENTRY",
                    _Name.StringAddress()
                    ));

        DEBUG_LEAVE(0);
    }

    BOOL IsLocal(VOID) const {
        return _LocalSemantics;
    }

    BOOL
    WriteEntry(
        OUT LPSTR lpszBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );
};

//
// PROXY_BYPASS_LIST - contains the list of proxy bypass entries
//

class PROXY_BYPASS_LIST {

private:

    //
    // _List - serialized list of PROXY_BYPASS_LIST_ENTRY objects
    //

    SERIALIZED_LIST _List;

    //
    // _Error - errors stored here from initialization
    //

    DWORD _Error;

    //
    // _lpszBypassString - unparsed input string that we're passed,
    //     sometime when I have time this should take over as our stored structure
    //

    LPSTR _lpszBypassString;

public:

    PROXY_BYPASS_LIST(LPCSTR BypassList) {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "PROXY_BYPASS_LIST",
                    "%q",
                    BypassList
                    ));

        InitializeSerializedList(&_List);
        if (BypassList == NULL) {
            BypassList = "";
        }

        _Error = ERROR_SUCCESS;
        _lpszBypassString = NewString(BypassList);
        if ( _lpszBypassString == NULL ) {
            _Error = ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( _Error == ERROR_SUCCESS ) {
            _Error = AddList(_lpszBypassString);
        }

        DEBUG_LEAVE(0);
    }

    ~PROXY_BYPASS_LIST() {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "~PROXY_BYPASS_LIST",
                    NULL
                    ));

        LockSerializedList(&_List);

        while (!IsSerializedListEmpty(&_List)) {

            //
            // remove the PROXY_BYPASS_LIST_ENTRY at the head of the serialized
            // list
            //

            LPVOID entry = SlDequeueHead(&_List);

            //
            // entry should not be NULL - IsSerializedListEmpty() told us we
            // could expect something
            //

            INET_ASSERT(entry != NULL);

            //
            // get the address of the object (should be the same as entry) and
            // delete it
            //

            delete CONTAINING_RECORD(entry, PROXY_BYPASS_LIST_ENTRY, _List);
        }

        if ( _lpszBypassString ) {
            FREE_MEMORY(_lpszBypassString);
        }

        UnlockSerializedList(&_List);

        TerminateSerializedList(&_List);

        DEBUG_LEAVE(0);
    }

    DWORD
    AddList(
        IN LPSTR lpszList
        );

    BOOL
    Find(
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort
        );

    DWORD
    Add(
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort
        );

    BOOL
    IsBypassed(
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort
        );

    // This function just checks the bypass list.
    BOOL
    IsHostInBypassList(
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort,
        IN BOOL isAddress
        );


    DWORD GetError(VOID) const {
        return _Error;
    }

    LPSTR CopyString(VOID) {    
        return (_lpszBypassString != NULL) ? NewString(_lpszBypassString) : NULL;
    }

    VOID
    GetList(
        OUT LPSTR * lplpszList,
        IN DWORD dwBufferLength,
        IN OUT LPDWORD lpdwRequiredLength
        );
};

//
// PROXY_SERVER_LIST_ENTRY - describes a proxy server and the scheme that uses
// it
//

class PROXY_SERVER_LIST_ENTRY {

friend class PROXY_SERVER_LIST;

private:

    //
    // _List - there may be a variable number of proxies
    //

    LIST_ENTRY _List;

    //
    // _Protocol - which protocol scheme this proxy information is for (e.g.
    // INTERNET_SCHEME_FTP in ftp=http://proxy:80)
    //

    INTERNET_SCHEME _Protocol;

    //
    // _Scheme - which protocol scheme we use to get the data (e.g.
    // INTERNET_SCHEME_HTTP in ftp=http://proxy:80)
    //

    INTERNET_SCHEME _Scheme;

    //
    // _ProxyName - the name of the proxy server we use for this scheme
    //

    ICSTRING _ProxyName;

    //
    // _ProxyPort - port number at proxy host
    //

    INTERNET_PORT _ProxyPort;

    //
    // _AddressList - the list of socket addresses for this proxy. Keeping this
    // information here allows us not have to resolve the proxy name each time
    // if DNS caching has been disabled
    //

    //ADDRESS_INFO_LIST _AddressList;

public:

    PROXY_SERVER_LIST_ENTRY(
        IN INTERNET_SCHEME tProtocol,
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort
        ) {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "PROXY_SERVER_LIST_ENTRY",
                    "%s, %s, %q, %d, %d",
                    InternetMapScheme(tProtocol),
                    InternetMapScheme(tScheme),
                    lpszHostName,
                    dwHostNameLength,
                    nPort
                    ));

        InitializeListHead(&_List);
        _Protocol = tProtocol;
        _Scheme = tScheme;
        _ProxyName.MakeCopy(lpszHostName, dwHostNameLength);
        _ProxyPort = nPort;
        //InitializeAddressList(&_AddressList);

        DEBUG_LEAVE(0);
    }

    ~PROXY_SERVER_LIST_ENTRY() {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "~PROXY_SERVER_LIST_ENTRY",
                    _ProxyName.StringAddress()
                    ));

        //FreeAddressList(&_AddressList);

        DEBUG_LEAVE(0);
    }

    //DWORD ResolveAddress(VOID) {
    //    return GetServiceAddress(_ProxyName.StringAddress(),
    //                             NULL,
    //                             NULL,
    //                             NS_DEFAULT,
    //                             _ProxyPort,
    //                             0,
    //                             &_AddressList
    //                             );
    //}

    BOOL
    WriteEntry(
        OUT LPSTR lpszBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );
};

//
// PROXY_SERVER_LIST - need one of these for each PROXY_INFO list. This class
// contains the proxy list and performs all operations on it
//

class PROXY_SERVER_LIST {

private:

    //
    // _List - serialized list of PROXY_SERVER_LIST_ENTRY objects
    //

    SERIALIZED_LIST _List;

    //
    // _Error - errors stored here from initialization
    //

    DWORD _Error;

    //
    // _lpszServerListString - A copy of the input string, sometime this should be 
    //    shared so that other structures use the same strings
    //

    LPSTR _lpszServerListString;

public:

    PROXY_SERVER_LIST(LPCSTR ServerList) {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "PROXY_SERVER_LIST",
                    "%q",
                    ServerList
                    ));

        _lpszServerListString = NULL;
        _Error = ERROR_SUCCESS;

        InitializeSerializedList(&_List);
        if (ServerList == NULL) {
            ServerList = "";
        }

        _lpszServerListString = NewString(ServerList);
        if ( _lpszServerListString == NULL ) {
            _Error = ERROR_NOT_ENOUGH_MEMORY;
        }

        if ( _Error == ERROR_SUCCESS ) {
            _Error = AddList(_lpszServerListString);
        }

        DEBUG_LEAVE(0);
    }

    ~PROXY_SERVER_LIST() {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "~PROXY_SERVER_LIST",
                    NULL
                    ));

        LockSerializedList(&_List);

        while (!IsSerializedListEmpty(&_List)) {

            //
            // remove the PROXY_SERVER_LIST_ENTRY at the head of the serialized
            // list
            //

            LPVOID entry = SlDequeueHead(&_List);

            //
            // entry should not be NULL - IsSerializedListEmpty() told us we
            // could expect something
            //

            INET_ASSERT(entry != NULL);

            //
            // get the address of the object (should be the same as entry) and
            // delete it
            //

            delete CONTAINING_RECORD(entry, PROXY_SERVER_LIST_ENTRY, _List);
        }

        if ( _lpszServerListString) {
            FREE_MEMORY(_lpszServerListString);
        }

        UnlockSerializedList(&_List);

        TerminateSerializedList(&_List);

        DEBUG_LEAVE(0);
    }

    DWORD
    AddList(
        IN LPSTR lpszList
        );

    BOOL
    Find(
        IN INTERNET_SCHEME tScheme
        );

    DWORD
    Add(
        IN INTERNET_SCHEME tProtocol,
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort
        );

    INTERNET_SCHEME
    ProxyScheme(
        IN INTERNET_SCHEME tProtocol
        );

    BOOL
    GetProxyHostName(
        IN INTERNET_SCHEME tProtocol,
        IN OUT LPINTERNET_SCHEME lptScheme,
        OUT LPSTR * lplpszHostName,
        OUT LPBOOL lpbFreeHostName,
        OUT LPDWORD lpdwHostNameLength,
        OUT LPINTERNET_PORT lpHostPort
        );

    DWORD
    AddToBypassList(
        IN PROXY_BYPASS_LIST * lpBypassList
        );

    DWORD GetError(VOID) const {
        return _Error;
    }

    LPSTR CopyString(VOID) {    
        return (_lpszServerListString != NULL ) ? NewString(_lpszServerListString) : NULL;
    }

    VOID
    GetList(
        OUT LPSTR * lplpszList,
        IN DWORD dwBufferLength,
        IN OUT LPDWORD lpdwRequiredLength
        );
};


//
// PROXY_INFO - maintains both PROXY_SERVER_LIST and PROXY_BYPASS_LIST,
//   provides base class for overall Proxy Management
//

class PROXY_INFO {

private:

    //
    // _ProxyServerList - pointer to list of proxy servers. We create this from
    // free store because all other handles derived from INTERNET_HANDLE_OBJECT
    // don't need to unnecessarily allocate a SERIALIZED_LIST (& therefore a
    // CRITICAL_SECTION)
    //

    PROXY_SERVER_LIST * _ProxyServerList;

    //
    // _ProxyBypassList - pointer to list of targets which will not go via
    // proxy. From free store because of same reason as PROXY_SERVER_LIST
    //

    PROXY_BYPASS_LIST * _ProxyBypassList;

    //
    // _Error - errors returned from creating proxy server & bypass lists
    //

    DWORD _Error;

    //
    // _Lock - acquire this before accessing the proxy info
    //

    RESOURCE_LOCK _Lock;

    //
    // _fDisableDirect - Disable direct connections on failure of proxy connections
    //

    BOOL _fDisableDirect;

    //
    // _fModifiedInProcess - TRUE if the proxy info is set to something other than the
    // registry contents
    //

    BOOL _fModifiedInProcess;

    //
    // _dwSettingsVersion - passed in settings version used to track changes
    //   made while we're off in processing. 
    //

    DWORD _dwSettingsVersion;

protected:

    //
    // _BadProxyList - Keep a listing of what proxies recently failed, so
    //   we can prevent reusing them.
    //

    BAD_PROXY_LIST _BadProxyList;

    //
    // IsGlobal() - TRUE if this is a global object used as a
    //   standard proxy info object.
    //

    BOOL IsGlobal(VOID) const {
        return ((this == (PROXY_INFO *) &GlobalProxyInfo) ? TRUE : FALSE );
    }

    BOOL IsDisableDirect(VOID) const {
        return _fDisableDirect;
    }

    VOID SetDisableDirect(BOOL fDisableDirect) {
        _fDisableDirect = fDisableDirect;
    }

    INTERNET_SCHEME
    ProxyScheme(
        IN INTERNET_SCHEME tProtocol
        ) {
        if (_ProxyServerList != NULL) {
            return _ProxyServerList->ProxyScheme(tProtocol);
        }
        return INTERNET_SCHEME_UNKNOWN;
    }

    BOOL
    GetProxyHostName(
        IN INTERNET_SCHEME tProtocol,
        IN OUT LPINTERNET_SCHEME lptScheme,
        OUT LPSTR * lplpszHostName,
        OUT LPBOOL lpbFreeHostName,
        OUT LPDWORD lpdwHostNameLength,
        OUT LPINTERNET_PORT lpHostPort
        ) {
        if (_ProxyServerList != NULL) {
            return _ProxyServerList->GetProxyHostName(tProtocol,
                                                      lptScheme,
                                                      lplpszHostName,
                                                      lpbFreeHostName,
                                                      lpdwHostNameLength,
                                                      lpHostPort
                                                      );
        }
        return FALSE;
    }

    BOOL
    IsBypassed(
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength,
        IN INTERNET_PORT nPort
        ) {

        //
        // Prevent proxing of the loop-address, to allow us to work with 
        //   Pointcast, local Web servers that run on the client machine
        //

        if (lpszHostName && 
            dwHostNameLength == (sizeof("127.0.0.1")-1) &&
            strncmp( lpszHostName, "127.0.0.1", sizeof("127.0.0.1")-1 ) == 0 ) 
        {
            return TRUE;
        }

        if (_ProxyBypassList != NULL) {
            return _ProxyBypassList->IsBypassed(tScheme,
                                                lpszHostName,
                                                dwHostNameLength,
                                                nPort
                                                );
        }
        return FALSE;
    }

    BOOL
    IsHostInBypassList(
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength
        ) {
        if (_ProxyBypassList != NULL) {
            return _ProxyBypassList->IsHostInBypassList(INTERNET_SCHEME_DEFAULT,
                                                        lpszHostName,
                                                        dwHostNameLength,
                                                        INTERNET_INVALID_PORT_NUMBER,
                                                        FALSE
                                                        );
        }
        return FALSE;
    }

    VOID
    CleanOutLists(
        VOID
        );

public:

    PROXY_INFO() {

        DEBUG_ENTER((DBG_OBJECTS,
            None,
            "PROXY_INFO::PROXY_INFO",
            ""
            ));


        DEBUG_LEAVE(0);
    }


    ~PROXY_INFO() {
        if (_Lock.IsInitialized()) {
            TerminateProxySettings();
        }
    }

    VOID Lock(BOOL bExclusiveMode) {
        _Lock.Acquire(bExclusiveMode);
    }

    VOID Unlock(VOID) {
        _Lock.Release();
    }

    DWORD GetError(VOID) const {
        return _Error;
    }

    BOOL IsModifiedInProcess(VOID) const {
        return _fModifiedInProcess;
    }

    DWORD
    GetProxyStringInfo(
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    BOOL
    RedoSendRequest(
        IN OUT LPDWORD lpdwError,
        IN AUTO_PROXY_ASYNC_MSG *pQueryForProxyInfo,
        IN CServerInfo *pOriginServer,
        IN CServerInfo *pProxyServer
        );


    //
    // Virtual Interface APIs
    // 
    //   InitializeProxySettings
    //   TerminateProxySettings - destroys our internal vars
    //   SetProxySettings - update proxy settings
    //   GetProxySettings - get current proxy settings
    //   RefreshProxySettings - force proxy settings to be reprocessed (auto-proxy, auto-detect)
    //   QueryProxySettings - ask proxy objects for proxy info
    //   IsProxySettingsConfigured
    //

    virtual 
    VOID 
    InitializeProxySettings(
        VOID
        );

    virtual
    VOID 
    TerminateProxySettings(
        VOID
        );
    
    virtual 
    DWORD 
    SetProxySettings(
        IN LPINTERNET_PROXY_INFO_EX  lpProxySettings,
        IN BOOL fModifiedInProcess
        );

    virtual 
    DWORD 
    GetProxySettings(
        OUT LPINTERNET_PROXY_INFO_EX  lpProxySettings,
        IN BOOL fCheckVersion
        );

    virtual
    DWORD
    RefreshProxySettings(
        IN BOOL fForceRefresh
        );

    virtual
    DWORD
    QueryProxySettings(
        IN OUT AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
        );

    virtual BOOL IsProxySettingsConfigured(VOID) {
        return (_ProxyServerList != NULL) ? TRUE : FALSE;
    }



};


//
// PROXY_INFO_GLOBAL - global proxy information for settings
//

class PROXY_INFO_GLOBAL : public PROXY_INFO {

private:

    //
    // _AutoProxyList - pointer to list of DLL that can handle changing
    //   proxy information based on connection URL, or downloaded local
    //   server information.
    //

    AUTO_PROXY_DLLS  * _AutoProxyList;

    //
    // _dwProxyFlags - set to control the various proxy modes that we support
    //

    DWORD _dwProxyFlags;

    //
    // _dwConnectionName - set to new connection name associated with this proxy info
    //

    LPSTR _lpszConnectionName;

    //
    // _fRefreshDisabled - TRUE at initalization, we're not allowed
    //    to refresh until InternetOpen
    //

    BOOL _fRefreshDisabled;

    //
    // _fQueuedRefresh - A refresh has been queued up, 
    //      ready to take place after InternetOpen
    //

    BOOL _fQueuedRefresh;

public:

    PROXY_INFO_GLOBAL() {
        _AutoProxyList      = NULL;
        _lpszConnectionName = NULL;
        _dwProxyFlags       = 0;
        _fQueuedRefresh     = FALSE;
        _fRefreshDisabled   = TRUE;
    }

    ~PROXY_INFO_GLOBAL() {
    }

    VOID
    FreeAutoProxyInfo(
        VOID
        )
    {
        if ( _AutoProxyList &&
             _AutoProxyList->IsAutoProxy() )
        {
            _AutoProxyList->FreeAutoProxyInfo();
        }
    }


    VOID SetAbortHandle(HINTERNET hInternet)
    {
        if ( _AutoProxyList &&
             _AutoProxyList->IsAutoProxy() )
        {
            _AutoProxyList->SetAbortHandle(hInternet);
        }
        else
        {
            INET_ASSERT(FALSE);
        }
    }

    VOID AbortAutoProxy(VOID) 
    {
        if ( _AutoProxyList &&
             _AutoProxyList->IsAutoProxy() )
        {
            _AutoProxyList->AbortAutoProxy();
        }
    }

    BOOL IsAutoProxy(VOID) {
        if ( _AutoProxyList &&
             _AutoProxyList->IsAutoProxy() )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }

    BOOL
    GetAutoProxyThreadSettings(
        OUT LPINTERNET_PROXY_INFO_EX  lpProxySettings
        )
    {
        if ( _AutoProxyList ) 
        {
            return _AutoProxyList->GetAutoProxyThreadSettings(lpProxySettings);
        }

        //else
        return FALSE;
    }


    BOOL
    SetAutoProxyThreadSettings(
        OUT LPINTERNET_PROXY_INFO_EX  lpProxySettings
        )
    {
        if ( _AutoProxyList ) 
        {
            return _AutoProxyList->SetAutoProxyThreadSettings(lpProxySettings);
        }

        //else
        return FALSE;
    }


    DWORD
    DoThreadProcessing(
        LPVOID lpAutoProxyObject
        )
    {
        BOOL fReturn;

        INET_ASSERT(lpAutoProxyObject == (LPVOID) _AutoProxyList);
        INET_ASSERT(IsGlobal());

        return (_AutoProxyList->DoThreadProcessing());
    }


    // This function determines if a host bypasses the proxy.
    // Will use autoproxy if it is enabled, else it will just
    // consult the bypass list.
    BOOL
    HostBypassesProxy(
        IN INTERNET_SCHEME tScheme,
        IN LPSTR lpszHostName,
        IN DWORD dwHostNameLength
        );

    VOID
    ClearBadProxyList(
        VOID
        )
    {
        Lock(TRUE);

        _BadProxyList.ClearList();

        Unlock();
    }

    VOID
    CheckForExpiredEntries(
        VOID
        )
    {
        Lock(TRUE);

        _BadProxyList.CheckForExpiredEntries();

        Unlock();
    }


    BOOL IsRefreshDisabled(VOID) {
        return (_fRefreshDisabled);
    }

    VOID SetRefreshDisabled(BOOL fDisable) {
        _fRefreshDisabled = fDisable;
    }

    VOID QueueRefresh(VOID) {
        _fQueuedRefresh = TRUE;
    }

    VOID
    ReleaseQueuedRefresh(
        VOID
        );

    // Virtual Interface APIs
    // 
    //   TerminateProxySettings - destroys our internal vars
    //   SetProxySettings - update proxy settings
    //   GetProxySettings - get current proxy settings
    //   RefreshProxySettings - force proxy settings to be reprocessed (auto-proxy, auto-detect)
    //   QueryProxySettings - ask proxy objects for proxy info
    //   IsProxySettingsConfigured
    //

    VOID
    TerminateProxySettings(
        VOID
        );
    
    DWORD 
    SetProxySettings(
        IN LPINTERNET_PROXY_INFO_EX  lpProxySettings,
        IN BOOL fModifiedInProcess
        );
    
    DWORD 
    GetProxySettings(
        OUT LPINTERNET_PROXY_INFO_EX  lpProxySettings,
        IN BOOL fCheckVersion
        );
    
    DWORD
    RefreshProxySettings(
        IN BOOL fForceRefresh
        );
    
    DWORD
    QueryProxySettings(
        IN OUT AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
        );

    BOOL IsProxySettingsConfigured(VOID) {
        return (IsAutoProxy() || PROXY_INFO::IsProxySettingsConfigured());
    }

    LPSTR GetConnectionName(VOID) {
        return _lpszConnectionName;
    }


};


//
// PROXY_INFO_GLOBAL_WRAPPER - wraps the global proxy information 
//   so that we can avoid using, auto-proxy and auto-detect 
//

class PROXY_INFO_GLOBAL_WRAPPER : public PROXY_INFO { 

public:

    PROXY_INFO_GLOBAL_WRAPPER() {
    }

    ~PROXY_INFO_GLOBAL_WRAPPER() {

        DEBUG_ENTER((DBG_OBJECTS,
            None,
            "PROXY_INFO_GLOBAL_WRAPPER::~PROXY_INFO_GLOBAL_WRAPPER",
            ""
            ));

        DEBUG_LEAVE(0);
    }

    //
    // Virtual Interface APIs
    // 

    VOID 
    InitializeProxySettings(
        VOID
        )
    {
    }


    VOID
    TerminateProxySettings(
        VOID
        ) 
    {
    }    

    
    DWORD 
    SetProxySettings(
        IN LPINTERNET_PROXY_INFO_EX  lpProxySettings,
        IN BOOL fModifiedInProcess
        )
    {
        return ERROR_SUCCESS; // do nothing
    }
    
    DWORD 
    GetProxySettings(
        OUT LPINTERNET_PROXY_INFO_EX  lpProxySettings,
        IN BOOL fCheckVersion
        )
    {
       return ERROR_SUCCESS; // do nothing
    }
    
    DWORD
    RefreshProxySettings(
        IN BOOL fForceRefresh
        )
    {
       return ERROR_SUCCESS; // do nothing
    }
    
    DWORD
    QueryProxySettings(
        IN OUT AUTO_PROXY_ASYNC_MSG **ppQueryForProxyInfo
        )
    {
        return GlobalProxyInfo.PROXY_INFO::QueryProxySettings(ppQueryForProxyInfo);
    }


    BOOL IsProxySettingsConfigured(VOID) {
        return GlobalProxyInfo.PROXY_INFO::IsProxySettingsConfigured();
    }
};



