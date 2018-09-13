/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    connect.hxx

Abstract:

    Contains the client-side connect handle class

Author:

    Richard L Firth (rfirth) 03-Jan-1996

Revision History:

    03-Jan-1996 rfirth
        Created

--*/

extern LONG GlobalExistingConnectHandles;

//
// forward references
//

class CServerInfo;

//
// classes
//

/*++

Class Description:

    This class defines the INTERNET_FILE_HANDLE_OBJECT.   Which wraps the simple
      Win32 File handle.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the service handle value from
    the generic object handle.

--*/


class INTERNET_FILE_HANDLE_OBJECT : public INTERNET_HANDLE_OBJECT {

private:

    //
    // _hFileHandle - File Handle for access to R/W Win32 File I/O
    //

    HANDLE _hFileHandle;

    //
    // _Flags - the creation flags passed in to InternetConnect()
    //

    //
    // BUGBUG - combine with INTERNET_HANDLE_OBJECT::Flags
    //

    BOOL _Flags;

    //
    // _lpszFileName - the file name we care about.
    //

    LPSTR _lpszFileName;


public:

    INTERNET_FILE_HANDLE_OBJECT(
        INTERNET_HANDLE_OBJECT * INetObj,
        LPSTR lpszFileName,
        HANDLE hFileHandle,
        DWORD dwFlags,
        DWORD_PTR dwContext
        ) : INTERNET_HANDLE_OBJECT(INetObj)
    {
        _Context      = dwContext;
        _Flags        = dwFlags;
        _hFileHandle  = hFileHandle;
        _lpszFileName = (lpszFileName ? NewString(lpszFileName) : NULL );
    }

    HANDLE GetFileHandle()  {
        return _hFileHandle;
    }

    PSTR GetDataFileName() {
        return _lpszFileName;
    }

    virtual ~INTERNET_FILE_HANDLE_OBJECT(VOID) {
        if ( _hFileHandle )
        {
            CloseHandle(_hFileHandle);
            _hFileHandle = NULL;
        }

        if ( _lpszFileName ) {
            _lpszFileName = (LPSTR) FREE_MEMORY(_lpszFileName);
        }
    }


    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return TypeFileRequestHandle;
    }

};


/*++

Class Description:

    This class defines the INTERNET_CONNECT_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the service handle value from
    the generic object handle.

--*/

class INTERNET_CONNECT_HANDLE_OBJECT : public INTERNET_HANDLE_OBJECT {

private:

    //
    // _InternetConnectHandle
    //

    //
    // BUGBUG - from the days of RPC catapult proxy. No longer required
    //

    HINTERNET _InternetConnectHandle;

    //
    // _wCloseFunction - function which handles closing underlying connection
    // at protocol level
    //

    CONNECT_CLOSE_HANDLE_FUNC _wCloseFunction;

    //
    // _IsCopy - TRUE if this handle is part of a file/find/object handle
    //

    BOOL _IsCopy;

    //
    // _ServiceType - type of service
    //

    //
    // BUGBUG - superceded by INTERNET_SCHEME_TYPE
    //

    DWORD _ServiceType;

    //
    // _HandleType - type of handle
    //

    //
    // BUGBUG - superceded by HANDLE_OBJECT::ObjectType
    //

    HINTERNET_HANDLE_TYPE _HandleType;

    //
    // _Flags - the creation flags passed in to InternetConnect()
    //

    //
    // BUGBUG - combine with INTERNET_HANDLE_OBJECT::Flags
    //

    BOOL _Flags;

    //
    // _InUse - used when object created with INTERNET_FLAG_EXISTING_CONNECT.
    // If TRUE the handle is already in use by another request, else it is
    // available
    //

    BOOL _InUse;

protected:

    // cache related items

    BOOL    _fDeleteDataFile;
    LPSTR   _CacheCWD;  //Current working directory
    LPSTR   _CacheUrlName;
    XSTRING _xsPrimaryCacheKey;
    XSTRING _xsSecondaryCacheKey;
    LPSTR   _CacheFileName;
    //LPBYTE  _CacheHeaderInfo;
    DWORD   _CacheHeaderLength;
    BOOL    _CacheReadInProgress;   // BUGBUG should they be in flags field
    BOOL    _CacheWriteInProgress;
    HANDLE  _CacheFileHandle;
    HANDLE _CacheFileHandleRead;
    DWORD _RealCacheFileSize;
    DWORD _VirtualCacheFileSize;

#ifdef LAZY_WRITE
    LPBYTE  _CacheScratchBuf;
    DWORD   _CacheScratchBufLen;
    DWORD   _CacheScratchUsedLen;
#endif

    DWORD   _dwCacheFlags;   // RELOAD, NO_CACHE, MAKE_PERSISTENT
    DWORD   _dwStreamRefCount;
    HANDLE  _hCacheStream;
    DWORD   _dwCurrentStreamPosition;
    BOOL    _fFromCache;
    BOOL    _fCacheWriteDisabled;
    BOOL    _fIsHtmlFind;

    BOOL  _CacheCopy;
    BOOL  _CachePerUserItem;
    BOOL  _fForcedExpiry;
    BOOL  _fLazyUpdate;
    HANDLE  _hLockRequestInfo;

    LPSTR _OriginalUrl;

    VOID InitCacheVariables() {
        // Initialize   the Cache Variables
        _fDeleteDataFile = FALSE;
        _CacheCWD = NULL;
        _CacheUrlName = NULL;
        _CacheFileName = NULL;
        //_CacheHeaderInfo = NULL;
        _CacheHeaderLength = 0;
        _CacheReadInProgress = FALSE;
        _CacheWriteInProgress = FALSE;
        _CacheFileHandle = INVALID_HANDLE_VALUE;
        _CacheFileHandleRead = INVALID_HANDLE_VALUE;
        _RealCacheFileSize = 0;
        _VirtualCacheFileSize = 0;
        _CacheCopy = FALSE;

#ifdef LAZY_WRITE
        _CacheScratchBuf = NULL;
        _CacheScratchBufLen = 0;
        _CacheScratchUsedLen = 0;
#endif // LAZY_WRITE

        _dwCacheFlags = 0;
        _hCacheStream = NULL;    // ACHTUNG, this is a memory handle
        _dwStreamRefCount = 0;
        _dwCurrentStreamPosition = 0;
        _fFromCache = FALSE;
        _fCacheWriteDisabled = FALSE;
        _fIsHtmlFind = FALSE;

        _CachePerUserItem = 0;
        _fForcedExpiry = FALSE;
        _fLazyUpdate = FALSE;
        _hLockRequestInfo = NULL;

        _OriginalUrl = NULL;
    }

protected:

    //
    // an app can set/query the internal buffer sizes for e.g. FtpGetFile()
    //

    DWORD _ReadBufferSize;
    DWORD _WriteBufferSize;

    //
    // the following 3 pointers are for the string parameters passed to
    // InternetConnect()
    //

    ICSTRING _HostName;
    XSTRING _xsUser;
    XSTRING _xsPass;
    XSTRING _xsProxyUser;
    XSTRING _xsProxyPass;

    INTERNET_PORT _HostPort;

    //
    // _SchemeType - the actual scheme type we are using for this object (may be
    // different from original type if going via (CERN) proxy, e.g. HTTP => FTP)
    //

    INTERNET_SCHEME _SchemeType;

    //
    // _LastResponseInfo - in the case of an FTP URL handle object, we associate
    // the last response info so that we can pull out the server welcome message
    //

    LPSTR _LastResponseInfo;

    //
    // _LastResponseInfoLength - number of bytes in _LastResponseInfo. Only
    // meaningful if _LastResponseInfo not NULL
    //

    DWORD _LastResponseInfoLength;

    //
    // _bViaProxy - TRUE if the request was made via a proxy
    //

    BOOL _bViaProxy;

    //
    // _bNoHeaders - TRUE if we made the request via HTTP and no headers were
    // returned in the response, or the request was made via a protocol that
    // doesn't return headers
    //

    BOOL _bNoHeaders;

    //
    // _bNetFailed - TRUE if the net operation failed (used when we are returning
    // data from cache on net timeout)
    //

    BOOL _bNetFailed;

    //
    // Bits indicating whether or not the proxy or server username + password
    // on the request handle are valid; Any time a username/password is transferred
    // to the password cache it is invalidated for all internal calls to avoid mistaken
    // re-usage on redirect. Calls to InternetQueryInfo will return any username/password
    // found on the handle to support applications expecting this information to remain
    // valid.
    //
    struct FlagsStruct
    {
        BYTE fProxyUserPassValid;
        BYTE fServerUserPassValid;
        BYTE fReserved1;
        BYTE fReserved2;
    } _HandleFlags;

    //
    // _dwErrorMask - Indicates which special errors are allowed to be returned
    // via HttpSendRequest - currently ERROR_INTERNET_INSERT_CDROM can be returned
    // if first bit is set in this mask.
    //

    DWORD _dwErrorMask;

    //
    // _ServerInfo - pointer to global server information context
    //

    CServerInfo * _ServerInfo;

    //
    // _OriginServer - pointer to origin server information context
    //

    CServerInfo * _OriginServer;

public:

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_HANDLE_OBJECT * INetObj,
        LPTSTR lpszServerName,
        INTERNET_PORT nServerPort,
        LPTSTR lpszUsername OPTIONAL,
        LPTSTR lpszPassword OPTIONAL,
        DWORD dwService,
        DWORD dwFlags,
        DWORD_PTR dwContext
        );

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj
        );

    INTERNET_CONNECT_HANDLE_OBJECT(
        INTERNET_HANDLE_OBJECT * Parent,
        HINTERNET Child,
        CONNECT_CLOSE_HANDLE_FUNC wCloseFunc,
        LPTSTR lpszServerName,
        INTERNET_PORT nServerPort,
        LPTSTR lpszUsername OPTIONAL,
        LPTSTR lpszPassword OPTIONAL,
        DWORD ServiceType,
        DWORD dwFlags,
        DWORD_PTR dwContext
        );

    virtual ~INTERNET_CONNECT_HANDLE_OBJECT(VOID);

    virtual HINTERNET GetHandle(VOID);

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return _HandleType;
    }

    DWORD GetServiceType(VOID) {
        return _ServiceType;
    }

    VOID SetHandleType(DWORD dwServiceType) {
        switch (dwServiceType) {
        case INTERNET_SERVICE_FTP:
            _HandleType = TypeFtpConnectHandle;
            break;

        case INTERNET_SERVICE_GOPHER:
            _HandleType = TypeGopherConnectHandle;
            break;

        case INTERNET_SERVICE_HTTP:
            _HandleType = TypeHttpConnectHandle;
            break;
        }
    }

    DWORD GetBufferSize(IN DWORD SizeIndex) {
        switch (SizeIndex) {
        case INTERNET_OPTION_READ_BUFFER_SIZE:
            return _ReadBufferSize;

        case INTERNET_OPTION_WRITE_BUFFER_SIZE:
            return _WriteBufferSize;

        default:

            //
            // BUGBUG - global default
            //

            return (4 K);
        }
    }

    VOID SetBufferSize(IN DWORD SizeIndex, IN DWORD Size) {
        switch (SizeIndex) {
        case INTERNET_OPTION_READ_BUFFER_SIZE:
            _ReadBufferSize = Size;
            break;

        case INTERNET_OPTION_WRITE_BUFFER_SIZE:
            _WriteBufferSize = Size;
            break;
        }
    }

    DWORD GetFlags() {
        return _Flags;
    }

    void SetDeleteDataFile (void) {
        _fDeleteDataFile = TRUE;
    }

    BOOL IsInUse(VOID) const {

        //
        // by definition, a connect handle object created without
        // INTERNET_FLAG_EXISTING_CONNECT is in use
        //

        return (_Flags & INTERNET_FLAG_EXISTING_CONNECT) ? _InUse : TRUE;
    }

    VOID SetInUse(VOID) {
//dprintf("SetInUse() %#x\n", GetPseudoHandle());
        INET_ASSERT(_Flags & INTERNET_FLAG_EXISTING_CONNECT);
        INET_ASSERT(!_InUse);

        //
        // only handle type that should be coming through here
        //

        INET_ASSERT(GetHandleType() == TypeFtpConnectHandle);

        _InUse = TRUE;

        //
        // one less handle that can be flushed right now
        //

        if (InterlockedDecrement(&GlobalExistingConnectHandles) < 0) {

            INET_ASSERT(FALSE);

            GlobalExistingConnectHandles = 0;
        }
//dprintf("SetInUse: GlobalExistingConnectHandles = %d\n", GlobalExistingConnectHandles);
    }

    BOOL SetUnused(VOID) {
//dprintf("SetUnused() %#x\n", GetPseudoHandle());

        //INET_ASSERT(_Flags & INTERNET_FLAG_EXISTING_CONNECT);

        //
        // only handle type that should be coming through here
        //

        //INET_ASSERT(GetHandleType() == TypeFtpConnectHandle);

        if ((_Flags & INTERNET_FLAG_EXISTING_CONNECT) && _InUse) {
            _InUse = FALSE;

            //
            // this handle can be flushed if required
            //

            InterlockedIncrement(&GlobalExistingConnectHandles);
//dprintf("SetUnused: GlobalExistingConnectHandles = %d\n", GlobalExistingConnectHandles);
            return TRUE;
        } else {
            return FALSE;
        }
    }

    //
    // cache functions.
    //

    // creates a protocl specific URL based on targethost, CWD, objectname and an extension

    DWORD
    SetObjectName(
        IN LPSTR lpszObjectName,
        IN LPSTR lpszExtension,
        IN URLGEN_FUNC * procProtocolUrl
        );

    //
    // CWD, applies only to FTP
    //

    DWORD
    SetCurrentWorkingDirectory(
        IN LPSTR lpszCWD
        );

    //
    // CWD, applies only to FTP
    //

    DWORD
    GetCurrentWorkingDirectory(
        OUT LPSTR lpszCWD,
        OUT LPDWORD lpdwSize
        );

    VOID SetCacheFlags(DWORD dwFlags) {
        _dwCacheFlags = dwFlags;
    }

    DWORD GetCacheFlags() {
        return _dwCacheFlags;
    }


    VOID SetCacheWriteDisabled() {
        _fCacheWriteDisabled = TRUE;
    }

    BOOL IsCacheWriteDisabled() {
        return _fCacheWriteDisabled;
    }

    VOID SetFromCache() {
        _fFromCache = TRUE;
    }

    BOOL IsFromCache() {
        return _fFromCache;
    }

    //
    // returns info about the entry that is being reteived in a LocalAlloced
    // buffer. Caller is supposed to free the buffer whenever convenient
    //

    DWORD BeginCacheRetrieval( LPCACHE_ENTRY_INFO*);

    DWORD RecordCacheRetrieval (LPCACHE_ENTRY_INFO);

    DWORD
    ReadCache(
        OUT LPBYTE Buffer,
        IN DWORD BufferLen,
        OUT LPDWORD BytesRead
        );

    DWORD
    EndCacheRetrieval(
        VOID
        );

    DWORD
    GetCacheStream(
        LPBYTE lpBuffer,
        DWORD dwLen
        );

    DWORD
    ReleaseCacheStream(
        HANDLE hCacheStream
        );

    DWORD BeginCacheWrite(DWORD dwExpectedLength, LPCSTR lpszExtension, LPCSTR lpszFileName = NULL);

    DWORD
    WriteCache(
        IN LPBYTE Buffer,
        IN DWORD BufferLen
        );

    DWORD
    WriteToDisk(
        IN LPBYTE Buffer,
        IN DWORD BufferLen,
        OUT LPDWORD lpdwBytesWritten
        );

    DWORD EndCacheWrite(
        FILETIME* lpftExpireTime,
        FILETIME *lpftLastModifiedTime,
        FILETIME* lpftPostCheckTime,
        DWORD dwEntryType,
        DWORD dwHeaderLen,
        LPSTR lpHeaderInfo,
        LPSTR lpszFileExtension,
        BOOL fImage = FALSE
        );

    DWORD LazyUpdate(VOID);


    DWORD
    CacheGetUrlInfo(
        LPCACHE_ENTRY_INFO UrlInfo,
        LPDWORD lpdwBuffSize
        ) {
        return GetUrlCacheEntryInfo(_CacheUrlName, UrlInfo, lpdwBuffSize);
    }

    DWORD
    CacheSetUrlInfo(
        LPCACHE_ENTRY_INFO UrlInfo,
        DWORD dwFieldControl
        ) {
        return SetUrlCacheEntryInfo(_CacheUrlName, UrlInfo, dwFieldControl);
    }

    DWORD
    ExpireUrl(VOID){
        CACHE_ENTRY_INFO sCEI;
        GetCurrentGmtTime(&sCEI.ExpireTime);
        *(LONGLONG *)&(sCEI.ExpireTime) -= ONE_HOUR_DELTA;

        return SetUrlCacheEntryInfo(_CacheUrlName, &sCEI, CACHE_ENTRY_EXPTIME_FC);

    }

    DWORD
    ExpireUrl(LPSTR lpszUrl){
        CACHE_ENTRY_INFO sCEI;
        GetCurrentGmtTime(&sCEI.ExpireTime);
        *(LONGLONG *)&(sCEI.ExpireTime) -= ONE_HOUR_DELTA;

        return SetUrlCacheEntryInfo(lpszUrl, &sCEI, CACHE_ENTRY_EXPTIME_FC);

    }

    BOOL IsCacheReadInProgress(VOID) {
        return _CacheReadInProgress;
    }

    BOOL IsCacheWriteInProgress(VOID) {
        return _CacheWriteInProgress;
    }

#ifdef LAZY_WRITE

    LPBYTE GetCacheScratchBuf(LPDWORD Length, LPDWORD lpdwUsed);

    VOID FreeCacheScratchBuf( ) {
        if (_CacheScratchBuf) {
            FREE_MEMORY(_CacheScratchBuf);
            _CacheScratchBuf = NULL;
            _CacheScratchBufLen = _CacheScratchUsedLen = 0;
        }
    }

    VOID ResetScratchUseSize() {

        _CacheScratchUsedLen = 0;


    }

    BOOL CopyToScratch(LPBYTE lpBuff, DWORD dwSize) {

        if (_CacheScratchBuf && (dwSize <= (_CacheScratchBufLen -_CacheScratchUsedLen))) {

            memcpy((_CacheScratchBuf+_CacheScratchUsedLen), lpBuff, dwSize);

            _CacheScratchUsedLen += dwSize;

            return (TRUE);
        }
        return (FALSE);

    }
#endif // LAZY_WRITE


    VOID SetHostName(LPSTR HostName) {
        _HostName = HostName;
    }

    LPSTR GetHostName(VOID) {
        return _HostName.StringAddress();
    }

    LPSTR GetHostName(LPDWORD lpdwStringLength) {
        *lpdwStringLength = _HostName.StringLength();
        return _HostName.StringAddress();
    }

    void CopyHostName(LPSTR OutputString) {
        _HostName.CopyTo(OutputString);
    }

    BOOL GetUserAndPass (BOOL fProxy, LPSTR *pszUser, LPSTR *pszPass);

    LPSTR GetUserOrPass (BOOL fUser, BOOL fProxy) {
        XSTRING *xs = fUser?
            (fProxy? &_xsProxyUser : &_xsUser):
            (fProxy? &_xsProxyPass : &_xsPass);

        if (!xs->GetPtr() && GetHandleType() == TypeHttpRequestHandle) {
            INTERNET_CONNECT_HANDLE_OBJECT * pConnect =
                (INTERNET_CONNECT_HANDLE_OBJECT *) GetParent();
            return pConnect->GetUserOrPass (fUser, fProxy);
        } else {
            return xs->GetPtr();
        }
    }

    void SetUserOrPass (LPSTR lpszIn, BOOL fUser, BOOL fProxy ) {
        XSTRING *xs = fUser?
            (fProxy? &_xsProxyUser : &_xsUser):
            (fProxy? &_xsProxyPass : &_xsPass);
        xs->SetData(lpszIn);

        if (fProxy)
            _HandleFlags.fProxyUserPassValid = TRUE;
        else
            _HandleFlags.fServerUserPassValid = TRUE;
    }

    LPSTR GetServerName(VOID) {
        return _HostName.StringAddress();;
    }

    VOID FreeURL(VOID) {
        if (_CacheUrlName != NULL) {
            _CacheUrlName = (LPSTR)FREE_MEMORY(_CacheUrlName);

            INET_ASSERT(_CacheUrlName == NULL);

        }
    }

    LPSTR GetURL(VOID) {
        return _xsSecondaryCacheKey.GetPtr()?
            _xsPrimaryCacheKey.GetPtr() :  _CacheUrlName;
    }

    BOOL SetURL(LPSTR lpszUrl);

    BOOL SetURLPtr(LPSTR* ppszUrl);

    BOOL SetSecondaryCacheKey (LPSTR lpszKey);

    LPSTR GetSecondaryCacheKey (void) {
        return _xsSecondaryCacheKey.GetPtr();
    }

    void FreeSecondaryCacheKey (void);

    LPSTR GetCacheKey (void) {
        return _CacheUrlName;
    }

    VOID SetHostPort(INTERNET_PORT Port) {
        _HostPort = Port;
    }

    INTERNET_PORT GetHostPort(VOID) {
        return _HostPort;
    }

    INTERNET_SCHEME GetSchemeType(VOID) const {
        return (_SchemeType == INTERNET_SCHEME_DEFAULT)
            ? INTERNET_SCHEME_HTTP
            : _SchemeType;
    }

    VOID SetSchemeType(INTERNET_SCHEME SchemeType) {
        _SchemeType = SchemeType;
    }

    VOID SetSchemeType(LPSTR SchemeName, DWORD SchemeLength) {
        _SchemeType = MapUrlSchemeName(SchemeName, SchemeLength);
    }

    VOID SetConnectHandle(HINTERNET hInternet) {
        _InternetConnectHandle = hInternet;
    }

    PSTR GetDataFileName (void) const {
         return _CacheFileName;
    }

    VOID FreeCacheFileName(VOID) {
        if (_CacheFileName != NULL) {
            (void)FREE_MEMORY((HLOCAL)_CacheFileName);
            _CacheFileName = NULL;
        }
    }

    BOOL IsPerUserItem() {
        return (_CachePerUserItem != 0);
    }

    VOID SetPerUserItem(BOOL fFlag) {
        _CachePerUserItem = fFlag;
    }

    BOOL IsHtmlFind() {
        return (_fIsHtmlFind != 0);
    }

    VOID SetHtmlFind(BOOL fFlag) {
        _fIsHtmlFind = fFlag;
    }

    VOID SetForcedExpiry(BOOL fFlag) {

        _fForcedExpiry = fFlag;
    }

    BOOL IsForcedExpirySet() {

        return(_fForcedExpiry);
    }

    BOOL ExpireDependents();

    VOID AttachLastResponseInfo(VOID);

    VOID SetLastResponseInfo(LPSTR Buffer, DWORD Length) {

        INET_ASSERT(_LastResponseInfo == NULL);

        _LastResponseInfo = Buffer;
        _LastResponseInfoLength = Length;
    }

    LPSTR GetLastResponseInfo(LPDWORD lpdwLength) const {
        *lpdwLength = _LastResponseInfoLength;
        return _LastResponseInfo;
    }

    VOID FreeLastResponseInfo(VOID) {
        if (_LastResponseInfo != NULL) {
            _LastResponseInfo = (LPSTR)FREE_MEMORY((HLOCAL)_LastResponseInfo);

            INET_ASSERT(_LastResponseInfo == NULL);

        }
    }

    VOID SetViaProxy(BOOL bValue) {
        _bViaProxy = bValue;
    }

    BOOL IsViaProxy(VOID) const {
        return _bViaProxy;
    }

    VOID SetNoHeaders(BOOL bValue) {
        _bNoHeaders = bValue;
    }

    BOOL IsNoHeaders(VOID) const {
        return _bNoHeaders;
    }

    VOID SetNetFailed(VOID) {
        _bNetFailed = TRUE;
    }

    BOOL IsNetFailed(VOID) const {
        return _bNetFailed;
    }

    VOID SetLockRequestHandle(HANDLE hLockRequestInfo) {

        INET_ASSERT(_hLockRequestInfo==NULL);
        _hLockRequestInfo = hLockRequestInfo;

    }

    HANDLE GetLockRequestHandle() {

        return (_hLockRequestInfo);

    }

    CServerInfo * GetServerInfo(VOID) const {
        return _ServerInfo;
    }

    CServerInfo * GetOriginServer(VOID) const {
        return (_OriginServer != NULL) ? _OriginServer : _ServerInfo;
    }

    VOID
    SetOriginServer(
        IN CServerInfo * pServerInfo
        );

    VOID SetOriginServer(VOID) {
        SetOriginServer(_ServerInfo);
    }

    DWORD
    SetServerInfo(
        CServerInfo * pReferencedServerInfo
       )
    {
        if (_ServerInfo != NULL) {
            ::ReleaseServerInfo(_ServerInfo);
        }

        //
        // WARNING:: THIS ASSUMES a pre-referenced
        //   ServerInfo
        //
        
        _ServerInfo = pReferencedServerInfo;
        return ERROR_SUCCESS;
    }


    DWORD
    SetServerInfo(
        IN LPSTR lpszServerName,
        IN DWORD dwServerNameLength
        );

    DWORD
    SetServerInfo(
        IN BOOL bDoResolution,
        IN OPTIONAL BOOL fNtlm = FALSE
        ) {
        return SetServerInfo(GetSchemeType(), bDoResolution, fNtlm);
    }

    DWORD
    SetServerInfo(
        IN INTERNET_SCHEME tScheme,
        IN BOOL bDoResolution,
        IN OPTIONAL BOOL fNtlm = FALSE
        );

    VOID SetOriginalUrl(LPSTR lpszUrl) {
        if (_OriginalUrl != NULL) {
            _OriginalUrl = (LPSTR)FREE_MEMORY(_OriginalUrl);

            INET_ASSERT(_OriginalUrl == NULL);

        }
        if (lpszUrl != NULL) {
            _OriginalUrl = NewString(lpszUrl);

            INET_ASSERT(_OriginalUrl != NULL);

        }
    }

    LPSTR GetOriginalUrl(VOID) const {
        return _OriginalUrl;
    }

    VOID SetErrorMask(DWORD dwErrorMask) {
        _dwErrorMask = dwErrorMask;
    }

    DWORD GetErrorMask(VOID) {
        return _dwErrorMask;
    }


};
