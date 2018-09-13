/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    http.cxx

Abstract:

    Contains methods for HTTP_REQUEST_HANDLE_OBJECT class

    Contents:
        RMakeHttpReqObjectHandle
        HTTP_REQUEST_HANDLE_OBJECT::HTTP_REQUEST_HANDLE_OBJECT
        HTTP_REQUEST_HANDLE_OBJECT::~HTTP_REQUEST_HANDLE_OBJECT
        HTTP_REQUEST_HANDLE_OBJECT::GetHandle
        HTTP_REQUEST_HANDLE_OBJECT::SetProxyName
        HTTP_REQUEST_HANDLE_OBJECT::GetProxyName
        HTTP_REQUEST_HANDLE_OBJECT::ReuseObject
        HTTP_REQUEST_HANDLE_OBJECT::ResetObject
        HTTP_REQUEST_HANDLE_OBJECT::UrlCacheUnlock
        HTTP_REQUEST_HANDLE_OBJECT::SetAuthenticated
        HTTP_REQUEST_HANDLE_OBJECT::IsAuthenticated

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

//
// functions
//


DWORD
RMakeHttpReqObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN CLOSE_HANDLE_FUNC wCloseFunc,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    C-callable wrapper for creating an HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    ParentHandle    - mapped address of parent (connect) handle

    ChildHandle     - IN: protocol-specific handle value associated with object
                        *** NOT USED FOR HTTP ***
                      OUT: mapped address of HTTP_REQUEST_HANDLE_OBJECT

    wCloseFunc      - address of protocol-specific function to be called when
                      object is closed
                        *** NOT USED FOR HTTP ***

    dwFlags         - app-supplied flags

    dwContext       - app-supplied context value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * hHttp;

    hHttp = new HTTP_REQUEST_HANDLE_OBJECT(
                    (INTERNET_CONNECT_HANDLE_OBJECT *)ParentHandle,
                    *ChildHandle,
                    wCloseFunc,
                    dwFlags,
                    dwContext
                    );
    if (hHttp != NULL) {
        error = hHttp->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            error = InternetIndicateStatusNewHandle((LPVOID)hHttp);

            //
            // ERROR_INTERNET_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_INTERNET_OPERATION_CANCELLED);

                hHttp = NULL;
            }
        } else {
            delete hHttp;
            hHttp = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ChildHandle = (HINTERNET)hHttp;

    return error;
}

//
// HTTP_REQUEST_HANDLE_OBJECT class implementation
//


HTTP_REQUEST_HANDLE_OBJECT::HTTP_REQUEST_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT * Parent,
    HINTERNET Child,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD dwFlags,
    DWORD_PTR dwContext
    ) : INTERNET_CONNECT_HANDLE_OBJECT(Parent)

/*++

Routine Description:

    Constructor for direct-to-net HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    Parent      - parent object

    Child       - IN: HTTPREQ structure pointer
                  OUT: pointer to created HTTP_REQUEST_HANDLE_OBJECT

    wCloseFunc  - address of function that closes/destroys HTTPREQ structure

    dwFlags     - open flags (e.g. INTERNET_FLAG_RELOAD)

    dwContext   - caller-supplied request context value

Return Value:

    None.

--*/

{
    _Context = dwContext;
    _Socket = NULL;
    _QueryBuffer = NULL;
    _QueryBufferLength = 0;
    _QueryOffset = 0;
    _QueryBytesAvailable = 0;
    _bKeepAliveConnection = FALSE;
    _bNoLongerKeepAlive = FALSE;
    _OpenFlags = dwFlags;
    _State = HttpRequestStateCreating;
    _RequestMethod = HTTP_METHOD_TYPE_UNKNOWN;
    _dwOptionalSaved = 0;
    _lpOptionalSaved = NULL;
    _fOptionalSaved = FALSE;
        _ResponseBuffer = NULL;
    _ResponseBufferLength = 0;
    ResetResponseVariables();
    _RequestHeaders.SetIsRequestHeaders(TRUE);
    _ResponseHeaders.SetIsRequestHeaders(FALSE);
    _fTalkingToSecureServerViaProxy = FALSE;
    _fRequestUsingProxy = FALSE;
    _bWantKeepAlive = FALSE;
    _bRefresh = FALSE;
    _RefreshHeader = NULL;
    _redirectCount = GlobalMaxHttpRedirects;
    _redirectCountedOut = FALSE;
    _fIgnoreOffline = FALSE;

    SetObjectType(TypeHttpRequestHandle);

    _pCacheEntryInfo = NULL;

    _pAuthCtx         = NULL;
    _pTunnelAuthCtx   = NULL;
    _pPWC             = NULL;
    _lpBlockingFilter = NULL;
    _dwCredPolicy     = 0xFFFFFFFF;

    _NoResetBits.Dword = 0;  // only here are we ever allowed to assign to Dword.

    SetDisableNTLMPreauth(GlobalDisableNTLMPreAuth);
    
    _ProxyHostName = NULL;
    _ProxyHostNameLength = NULL;
    _ProxyPort = INTERNET_INVALID_PORT_NUMBER;

    _SocksProxyHostName = NULL;
    _SocksProxyHostNameLength = NULL;
    _SocksProxyPort = INTERNET_INVALID_PORT_NUMBER;

    HttpFiltOpen(); // enumerate http filters if not already active

    _HaveReadFileExData = FALSE;
    memset(&_BuffersOut, 0, sizeof(_BuffersOut));
    _BuffersOut.dwStructSize = sizeof(_BuffersOut);
    _BuffersOut.lpvBuffer = (LPVOID)&_ReadFileExData;

    m_pSecurityInfo = NULL;

    SetPriority(0);

#ifdef RLF_TEST_CODE

    static long l = 0;
    SetPriority(l++);

#endif

    _RTT = 0;
    _CP = CP_ACP;

    if (_Status == ERROR_SUCCESS) {
        _Status = _RequestHeaders.GetError();
        if (_Status == ERROR_SUCCESS) {
            _Status = _ResponseHeaders.GetError();
        }
    }
}


HTTP_REQUEST_HANDLE_OBJECT::~HTTP_REQUEST_HANDLE_OBJECT(
    VOID
    )

/*++

Routine Description:

    Destructor for HTTP_REQUEST_HANDLE_OBJECT

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_OBJECTS,
                None,
                "~HTTP_REQUEST_HANDLE_OBJECT",
                "%#x",
                this
                ));

    //
    // close the socket (or free it to the pool if keep-alive)
    //

    //
    // Authentication Note:
    // The CloseConnection parameter to force the connection closed
    // is set if we received a challenge but didn't respond, otherwise
    // IIS will get confused when a subsequent request recycles the
    // socket from the keep-alive pool.
    //

    CloseConnection(GetAuthState() == AUTHSTATE_CHALLENGE);

    if (IsCacheWriteInProgress()) {
        LocalEndCacheWrite(IsEndOfFile());
    }

    if (IsCacheReadInProgress()) {

        INET_ASSERT (_pCacheEntryInfo);
        FREE_MEMORY (_pCacheEntryInfo);

        // Rest is cleaned up in INTERNET_CONNECT_HANDLE_OBJECT::EndCacheWrite

    } else {
        UrlCacheUnlock();
    }

    //
    // If there's an authentication context, unload the provider.
    //

    if (_pAuthCtx) {
        delete _pAuthCtx;
    }
    if (_pTunnelAuthCtx) {
        delete _pTunnelAuthCtx;
    }

    //
    // free the various buffers
    //

    FreeResponseBuffer();
    FreeQueryBuffer();
    SetProxyName(NULL,NULL,0);

    if (m_pSecurityInfo != NULL) {
        m_pSecurityInfo->Release();
    }

    DEBUG_LEAVE(0);
}


HINTERNET
HTTP_REQUEST_HANDLE_OBJECT::GetHandle(
    VOID
    )

/*++

Routine Description:

    Returns child handle value. NULL for HTTP

Arguments:

    None.

Return Value:

    HINTERNET
        NULL

--*/

{
    return NULL;
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::SetProxyName(
    IN LPSTR lpszProxyHostName,
    IN DWORD dwProxyHostNameLength,
    IN INTERNET_PORT ProxyPort
    )

/*++

Routine Description:

    Set proxy name in object. If already have name, free it. Don't set name if
    current pointer is input

Arguments:

    lpszProxyHostName       - pointer to proxy name to add

    dwProxyHostNameLength   - length of proxy name

    ProxyPort               - port

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::SetProxyName",
                 "{%q, %d, %d}%q, %d, %d",
                 _ProxyHostName,
                 _ProxyHostNameLength,
                 _ProxyPort,
                 lpszProxyHostName,
                 dwProxyHostNameLength,
                 ProxyPort
                 ));

    if (lpszProxyHostName != _ProxyHostName) {
        if (_ProxyHostName != NULL) {
            _ProxyHostName = (LPSTR)FREE_MEMORY(_ProxyHostName);

            INET_ASSERT(_ProxyHostName == NULL);

            SetOverrideProxyMode(FALSE);
        }
        if (lpszProxyHostName != NULL) {
            _ProxyHostName = NEW_STRING(lpszProxyHostName);
            if (_ProxyHostName == NULL) {
                dwProxyHostNameLength = 0;
            }
        }
        _ProxyHostNameLength = dwProxyHostNameLength;
        _ProxyPort = ProxyPort;
    } else if (lpszProxyHostName != NULL) {

        DEBUG_PRINT(HTTP,
                    WARNING,
                    ("!!! lpszProxyHostName == _ProxyHostName (%#x)\n",
                    lpszProxyHostName
                    ));

        INET_ASSERT(dwProxyHostNameLength == _ProxyHostNameLength);
        INET_ASSERT(ProxyPort == _ProxyPort);

    }

    DEBUG_LEAVE(0);
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::GetProxyName(
    OUT LPSTR* lplpszProxyHostName,
    OUT LPDWORD lpdwProxyHostNameLength,
    OUT LPINTERNET_PORT lpProxyPort
    )

/*++

Routine Description:

    Return address & length of proxy name plus proxy port

Arguments:

    lplpszProxyHostName     - returned address of name

    lpdwProxyHostNameLength - returned length of name

    lpProxyPort             - returned port

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::GetProxyName",
                 "{%q, %d, %d}%#x, %#x, %#x",
                 _ProxyHostName,
                 _ProxyHostNameLength,
                 _ProxyPort,
                 lplpszProxyHostName,
                 lpdwProxyHostNameLength,
                 lpProxyPort
                 ));

    *lplpszProxyHostName = _ProxyHostName;
    *lpdwProxyHostNameLength = _ProxyHostNameLength;
    *lpProxyPort = _ProxyPort;

    DEBUG_LEAVE(0);
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::ReuseObject(
    VOID
    )

/*++

Routine Description:

    Make the object re-usable: clear out any received data and headers and
    reset the state to open

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::ReuseObject",
                 NULL
                 ));

    _ResponseHeaders.FreeHeaders();
    FreeResponseBuffer();
    ResetResponseVariables();
    _ResponseHeaders.Initialize();
    _dwCurrentStreamPosition = 0;
    SetState(HttpRequestStateOpen);
    ResetEndOfFile();
    _ctChunkInfo.Reset();
    _QueryOffset = 0;
    _QueryBytesAvailable = 0;
    _dwQuerySetCookieHeader = 0;
    if (m_pSecurityInfo) {
        m_pSecurityInfo->Release();
    }
    m_pSecurityInfo = NULL;

    DEBUG_LEAVE(0);
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ResetObject(
    IN BOOL bForce,
    IN BOOL bFreeRequestHeaders
    )

/*++

Routine Description:

    This method is called when we we are clearing out a partially completed
    transaction, mainly for when we have determined that an if-modified-since
    request, or a response that has not invalidated the cache entry can be
    retrieved from cache (this is a speed issue)

    Abort the connection and clear out the response headers and response
    buffer; clear the response variables (all done by AbortConnection()).

    If bFreeRequestHeaders, clear out the request headers.

    Reinitialize the response headers. We do not reset the object state, but we
    do reset the end-of-file status

Arguments:

    bForce              - TRUE if connection is forced closed

    bFreeRequestHeaders - TRUE if request headers should be freed

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::ResetObject",
                 "%B, %B",
                 bForce,
                 bFreeRequestHeaders
                 ));

    DWORD error;

    error = AbortConnection(bForce);
    if (error == ERROR_SUCCESS) {
        if (bFreeRequestHeaders) {
            _RequestHeaders.FreeHeaders();
        }
        _ResponseHeaders.Initialize();
        ResetEndOfFile();
    }

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::UrlCacheUnlock(
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
    INET_ASSERT (!_CacheReadInProgress);

    if (_hCacheStream)
    {
        UnlockUrlCacheEntryStream (_hCacheStream, 0);
        _hCacheStream = NULL;
    }

    if (_pCacheEntryInfo)
    {

        if (_pCacheEntryInfo->CacheEntryType & SPARSE_CACHE_ENTRY)
        {

            //
            // We can't use the partial cache entry because it is
            // stale, so delete the data file we got from cache.
            //

            INET_ASSERT (_CacheFileHandle != INVALID_HANDLE_VALUE);
            CloseHandle (_CacheFileHandle);
            _CacheFileHandle = INVALID_HANDLE_VALUE;
            DeleteFile (_pCacheEntryInfo->lpszLocalFileName);
        }

        FREE_MEMORY (_pCacheEntryInfo);
        _pCacheEntryInfo = NULL;
    }
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::SetAuthenticated(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    SetAuthenticated    -

Return Value:

    None.

--*/

{
    if (!_Socket)
    {
        INET_ASSERT(FALSE);
    }
    else
    {
        _Socket->SetAuthenticated();
    }
}


BOOL
HTTP_REQUEST_HANDLE_OBJECT::IsAuthenticated(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    IsAuthenticated -

Return Value:

    BOOL

--*/

{
    return (_Socket ? _Socket->IsAuthenticated() : FALSE);
}
