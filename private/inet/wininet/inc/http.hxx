/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    http.hxx

Abstract:

    Contains the client-side HTTP handle class

Author:

    Richard L Firth (rfirth) 03-Jan-1996

Revision History:

    03-Jan-1996 rfirth
        Created

--*/

//
// prototype for glue to urlmon security manager
//

DWORD GetCredPolicy   (LPSTR pszUrl);
DWORD GetCookiePolicy (LPCSTR pszUrl, BOOL fIsSessionCookie);
VOID  SetStopWarning  (LPCSTR pszUrl, DWORD dwPolicy, BOOL fIsSessionCookie);

//
// manifests
//

#define INITIAL_HEADERS_COUNT           16
#define HEADERS_INCREMENT               4

#define INVALID_HEADER_INDEX            0xff
#define INVALID_HEADER_SLOT             0xFFFFFFFF

#define HTTPREQ_STATE_ANYTHING_OK       0x8000  // for debug purposes
#define HTTPREQ_STATE_CLOSE_OK          0x4000
#define HTTPREQ_STATE_ADD_OK            0x2000
#define HTTPREQ_STATE_SEND_OK           0x1000
#define HTTPREQ_STATE_READ_OK           0x0800
#define HTTPREQ_STATE_QUERY_REQUEST_OK  0x0400
#define HTTPREQ_STATE_QUERY_RESPONSE_OK 0x0200
#define HTTPREQ_STATE_REUSE_OK          0x0100
#define HTTPREQ_STATE_WRITE_OK          0x0010

//
// macros
//

#define IS_VALID_HTTP_STATE(p, api, ref) \
    (p)->CheckState(HTTPREQ_STATE_ ## api ## _OK)

#define IsValidHttpState(api) \
    CheckState(HTTPREQ_STATE_ ## api ## _OK)

//
// STRESS_BUG_DEBUG - used to catch a specific stress fault,
//   where we have a corrupted crit sec.
//


#define CLEAR_DEBUG_CRIT(x)
#define IS_DEBUG_CRIT_OK(x)

//
// types
//

typedef enum {
    HTTP_HEADER_TYPE_UNKNOWN = 0,
    HTTP_HEADER_TYPE_ACCEPT
} HTTP_HEADER_TYPE;

typedef enum {
    HTTP_METHOD_TYPE_UNKNOWN = -1,
    HTTP_METHOD_TYPE_FIRST = 0,
    HTTP_METHOD_TYPE_GET= HTTP_METHOD_TYPE_FIRST,
    HTTP_METHOD_TYPE_HEAD,
    HTTP_METHOD_TYPE_POST,
    HTTP_METHOD_TYPE_PUT,
    HTTP_METHOD_TYPE_PROPFIND,
    HTTP_METHOD_TYPE_PROPPATCH,
    HTTP_METHOD_TYPE_LOCK,
    HTTP_METHOD_TYPE_UNLOCK,
    HTTP_METHOD_TYPE_COPY,
    HTTP_METHOD_TYPE_MOVE,
    HTTP_METHOD_TYPE_MKCOL,
    HTTP_METHOD_TYPE_CONNECT,
    HTTP_METHOD_TYPE_DELETE,
    HTTP_METHOD_TYPE_LINK,
    HTTP_METHOD_TYPE_UNLINK,
    HTTP_METHOD_TYPE_BMOVE,
    HTTP_METHOD_TYPE_BCOPY,
    HTTP_METHOD_TYPE_BPROPFIND,
    HTTP_METHOD_TYPE_BPROPPATCH,
    HTTP_METHOD_TYPE_BDELETE,
    HTTP_METHOD_TYPE_SUBSCRIBE,
    HTTP_METHOD_TYPE_UNSUBSCRIBE,
    HTTP_METHOD_TYPE_NOTIFY,
    HTTP_METHOD_TYPE_POLL, 
    HTTP_METHOD_TYPE_CHECKIN,
    HTTP_METHOD_TYPE_CHECKOUT,
    HTTP_METHOD_TYPE_INVOKE,
    HTTP_METHOD_TYPE_SEARCH,
    HTTP_METHOD_TYPE_PIN,
    HTTP_METHOD_TYPE_MPOST,
    HTTP_METHOD_TYPE_LAST = HTTP_METHOD_TYPE_MPOST
} HTTP_METHOD_TYPE;

typedef enum {

    //
    // The request handle is in the process of being created
    //

    HttpRequestStateCreating    = 0 | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The request handle is open, but the request has not been sent to the
    // server
    //

    HttpRequestStateOpen        = 1 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ADD_OK
                                    | HTTPREQ_STATE_SEND_OK
                                    | HTTPREQ_STATE_READ_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_QUERY_RESPONSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The request has been sent to the server, but the response headers have
    // not been received
    //

    HttpRequestStateRequest     = 2 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_WRITE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The response headers are being received, but not yet completed
    //

    HttpRequestStateResponse    = 3 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_WRITE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The response headers have been received, and there is object data
    // available to read
    //

    //
    // QFE 3576: It's possible that we're init'ing a new request,
    //           but the previous request hasn't been drained yet.
    //           Since we know it will always be drained, the lowest
    //           impact change is to allow headers to be replaced
    //           if we're in a state with potential data left to receive.
 
    HttpRequestStateObjectData  = 4 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ADD_OK
                                    | HTTPREQ_STATE_READ_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_QUERY_RESPONSE_OK
                                    | HTTPREQ_STATE_REUSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,
  

    //
    // A fatal error occurred
    //

    HttpRequestStateError       = 5 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK,

    //
    // The request is closing
    //

    HttpRequestStateClosing     = 6 | HTTPREQ_STATE_ANYTHING_OK,

    //
    // the data has been drained from the request object and it can be re-used
    //

    HttpRequestStateReopen      = 7 | HTTPREQ_STATE_CLOSE_OK
                                    | HTTPREQ_STATE_ADD_OK
                                    | HTTPREQ_STATE_READ_OK
                                    | HTTPREQ_STATE_QUERY_REQUEST_OK
                                    | HTTPREQ_STATE_QUERY_RESPONSE_OK
                                    | HTTPREQ_STATE_REUSE_OK
                                    | HTTPREQ_STATE_ANYTHING_OK

} HTTPREQ_STATE, FAR * LPHTTPREQ_STATE;

//
// general prototypes
//

HTTP_METHOD_TYPE
MapHttpRequestMethod(
    IN LPCSTR lpszVerb
    );

DWORD
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod,
    OUT LPCSTR * lplpcszName
    );

DWORD
CreateEscapedUrlPath(
    IN LPSTR lpszUrlPath,
    OUT LPSTR * lplpszEncodedUrlPath
    );

#if INET_DEBUG

LPSTR
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod
    );

#endif

//
// forward references
//

class CFsm_HttpSendRequest;
class CFsm_MakeConnection;
class CFsm_OpenConnection;
class CFsm_OpenProxyTunnel;
class CFsm_SendRequest;
class CFsm_ReceiveResponse;
class CFsm_HttpReadData;
class CFsm_HttpWriteData;
class CFsm_ReadData;
class CFsm_HttpQueryAvailable;
class CFsm_DrainResponse;
class CFsm_Redirect;
class CFsm_ReadLoop;

//
// classes
//

//
// HEADER_STRING - extension of ICSTRING so we can perform hashing on strings.
// Note that we only care about the header value
//

class HEADER_STRING : public ICSTRING {

private:

    DWORD m_Hash;

public:

    HEADER_STRING() {
        SetHash(0);
    }

    ~HEADER_STRING() {
    }

    VOID SetHash(DWORD dwHash) {
        m_Hash = dwHash;
    }

    VOID SetNextKnownIndex(BYTE bNextKnown) {
        _Union.Bytes.ExtraByte1 = bNextKnown;
    }

    BYTE * GetNextKnownIndexPtr() {
        return &_Union.Bytes.ExtraByte1;
    }

    DWORD GetNextKnownIndex() {
        return ((DWORD)_Union.Bytes.ExtraByte1);
    }

    DWORD GetHash(VOID) {
        return m_Hash;
    }

    VOID CreateHash(LPSTR lpszBase);

    int HashStrnicmp(LPSTR lpBase, LPSTR lpszString, DWORD dwLength, DWORD dwHash)
    {
        int RetVal = -1;

        if ((m_Hash == 0) || (m_Hash == dwHash))
        {
            if ( Strnicmp(lpBase, lpszString, dwLength) == 0 )
            {

                LPSTR value;

                value = StringAddress(lpBase) + dwLength;

                //
                // the input string could be a substring of a different header
                //

                if (*value == ':')
                {
                    RetVal = 0; // success, we have a match here!
                }
           }
        }

        return RetVal;
    }

    HEADER_STRING & operator=(LPSTR String) {
        SetHash(0);
        return (HEADER_STRING &)ICSTRING::operator=(String);
    }
};

//
// HTTP_HEADERS - array of pointers to general HTTP header strings. Headers are
// stored without line termination. _HeadersLength maintains the cumulative
// amount of buffer space required to store all the headers. Accounts for the
// missing line termination sequence
//

#define HTTP_HEADER_SIGNATURE   0x64616548  // "Head"

class HTTP_HEADERS {

public:

    //
    // _bKnownHeaders - array of bytes which index into lpHeaders
    //   the 0 column is an error catcher, so all indexes need to be biased by 1 (++'ed)
    //

    BYTE _bKnownHeaders[HTTP_QUERY_MAX+2];

private:



#if INET_DEBUG

    DWORD _Signature;

#endif

    //
    // _lpHeaders - (growable) array of pointers to headers added by app
    //

    HEADER_STRING * _lpHeaders;

    //
    // _TotalSlots - number of pointers in (i.e. size of) _lpHeaders
    //

    DWORD _TotalSlots;

    //
    // _NextOpenSlot - offset where the next array is open for allocation
    //

    DWORD _NextOpenSlot;

    //
    // _FreeSlots - number of available headers in _lpHeaders
    //

    DWORD _FreeSlots;

    //
    // _HeadersLength - the amount of buffer space required to store the headers.
    // For each header, includes +2 for the line termination that must be added
    // when the header buffer is generated, or when the headers are queried
    //

    DWORD _HeadersLength;

    //
    // _IsRequestHeaders - TRUE if this HTTP_HEADERS object describes the
    // request headers
    //

    BOOL _IsRequestHeaders;

    //
    // _lpszVerb etc. - in the case of request headers, we maintain these
    // pointers and corresponding lengths to make modifying the request line
    // easier in the case of a redirect
    //
    // N.B. The pointers are just offsets into the request line AND MUST NOT BE
    // FREED
    //

    LPSTR _lpszVerb;
    DWORD _dwVerbLength;
    LPSTR _lpszObjectName;
    DWORD _dwObjectNameLength;
    LPSTR _lpszVersion;
    DWORD _dwVersionLength;

    DWORD _RequestVersionMajor;
    DWORD _RequestVersionMinor;

    //
    // _Error - status code if error
    //

    DWORD _Error;

    //
    // _CritSec - acquire this when accessing header structure - stops multiple
    // threads clashing while modifying headers
    //

    CRITICAL_SECTION _CritSec;

    //
    // private methods
    //

    DWORD
    AllocateHeaders(
        IN DWORD dwNumberOfHeaders
        );

public:

    HTTP_HEADERS() {

#if INET_DEBUG

        _Signature = HTTP_HEADER_SIGNATURE;

#endif

        CLEAR_DEBUG_CRIT(_szCritSecBefore);
        CLEAR_DEBUG_CRIT(_szCritSecAfter);
        InitializeCriticalSection(&_CritSec);
        Initialize();
    }

    ~HTTP_HEADERS() {

        DEBUG_ENTER((DBG_OBJECTS,
                    None,
                    "~HTTP_HEADERS",
                    "%#x",
                    this
                    ));

#if INET_DEBUG

        INET_ASSERT(_Signature == HTTP_HEADER_SIGNATURE);

#endif

        FreeHeaders();
        DeleteCriticalSection(&_CritSec);

        DEBUG_LEAVE(0);

    }

    VOID Initialize(VOID) {
        _lpHeaders = NULL;
        _TotalSlots = 0;
        _FreeSlots = 0;
        _HeadersLength = 0;
        _lpszVerb = NULL;
        _dwVerbLength = 0;
        _lpszObjectName = NULL;
        _dwObjectNameLength = 0;
        _lpszVersion = NULL;
        _dwVersionLength = 0;
        _RequestVersionMajor = 0;
        _RequestVersionMinor = 0;
        _NextOpenSlot = 0;
        memset((void *) _bKnownHeaders, INVALID_HEADER_SLOT, ARRAY_ELEMENTS(_bKnownHeaders));
        _Error = AllocateHeaders(INITIAL_HEADERS_COUNT);
    }

    BOOL IsHeaderPresent(DWORD dwQueryIndex) const {
        return (_bKnownHeaders[dwQueryIndex] != INVALID_HEADER_INDEX) ? TRUE : FALSE ;
    }

    VOID LockHeaders(VOID) {
        EnterCriticalSection(&_CritSec);
    }

    VOID UnlockHeaders(VOID) {
        LeaveCriticalSection(&_CritSec);
    }

    VOID
    FreeHeaders(
        VOID
        );

    VOID
    CopyHeaders(
        IN OUT LPSTR * lpBuffer,
        IN LPSTR lpszObjectName,
        IN DWORD dwObjectNameLength
        );

#ifdef COMPRESSED_HEADERS

    VOID
    CopyCompressedHeaders(
        IN OUT LPSTR * lpBuffer
        );

#endif //COMPRESSED_HEADERS

    HEADER_STRING *
    FASTCALL
    FindFreeSlot(
        DWORD* piSlot
        );

    HEADER_STRING *
    GetSlot(
        DWORD iSlot
        )
    {
        return &_lpHeaders[iSlot];
    }

    LPSTR GetHeaderPointer (LPBYTE pbBase, DWORD iSlot)
    {
        INET_ASSERT (iSlot < _TotalSlots);
        return _lpHeaders[iSlot].StringAddress((LPSTR) pbBase);
    }

    VOID
    ShrinkHeader(
        LPBYTE pbBase,
        DWORD  iSlot,
        DWORD  dwOldQueryIndex,
        DWORD  dwNewQueryIndex,
        DWORD  cbNewSize
        );

    DWORD
    inline
    FastFind(
        IN DWORD  dwQueryIndex,
        IN DWORD  dwIndex
        );

    DWORD
    inline
    FastNukeFind(
        IN DWORD  dwQueryIndex,
        IN DWORD  dwIndex,
        OUT BYTE **lplpbPrevIndex
        );

    DWORD
    inline
    SlowFind(
        IN LPSTR lpBase,
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN DWORD dwIndex,
        IN DWORD dwHash,
        OUT DWORD *lpdwQueryIndex,
        OUT BYTE  **lplpbPrevIndex
        );

    BYTE
    inline
    FastAdd(
        IN DWORD  dwQueryIndex,
        IN DWORD  dwSlot
        );

    BOOL
    inline
    HeaderMatch(
        IN DWORD dwHash,
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        OUT DWORD *lpdwQueryIndex
        );

    VOID
    RemoveAllByIndex(
        IN DWORD dwQueryIndex
        );

    VOID RemoveHeader(IN DWORD dwIndex, IN DWORD dwQueryIndex, IN BYTE *pbPrevByte) {

        INET_ASSERT(dwIndex < _TotalSlots);
        INET_ASSERT(dwIndex != 0);
        // INET_ASSERT(_HeadersLength > 2);
        INET_ASSERT(_lpHeaders[dwIndex].StringLength() > 2);
        INET_ASSERT(_FreeSlots <= _TotalSlots);

        //
        // remove the length of the header + 2 for CR-LF from the total headers
        // length
        //

        if (_HeadersLength) {
            _HeadersLength -= _lpHeaders[dwIndex].StringLength()
                            + (sizeof("\r\n") - 1)
                            ;
        }

        //
        // Update the cached known headers, if this is one.
        //

        if ( dwQueryIndex < INVALID_HEADER_INDEX )
        {
            *pbPrevByte = (BYTE) _lpHeaders[dwIndex].GetNextKnownIndex();
        }

        //
        // set the header string to NULL. Frees the header buffer
        //

        _lpHeaders[dwIndex] = (LPSTR)NULL;

        //
        // we have freed a slot in the headers array
        //

        if ( _FreeSlots == 0 )
        {
            _NextOpenSlot = dwIndex;
        }

        ++_FreeSlots;

        INET_ASSERT(_FreeSlots <= _TotalSlots);

    }

    DWORD GetSlotsInUse (void) {
        return _TotalSlots - _FreeSlots;
    }

    DWORD HeadersLength(VOID) const {
        return _HeadersLength;
    }

    DWORD ObjectNameLength(VOID) const {
        return _dwObjectNameLength;
    }

    LPSTR ObjectName(VOID) const {
        return _lpszObjectName;
    }

    DWORD
    AddHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    AddHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    ReplaceHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    ReplaceHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        );

    DWORD
    FindHeader(
        IN LPSTR lpBase,
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN DWORD dwModifiers,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    FindHeader(
        IN LPSTR lpBase,
        IN DWORD dwQueryIndex,
        IN DWORD dwModifiers,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    FastFindHeader(
        IN LPSTR lpBase,
        IN DWORD dwQueryIndex,
        OUT LPVOID *lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT DWORD dwIndex
        );


    DWORD
    QueryRawHeaders(
        IN LPSTR lpBase,
        IN BOOL bCrLfTerminated,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    QueryFilteredRawHeaders(
        IN LPSTR lpBase,
        IN LPSTR *lplpFilterList,
        IN DWORD cListElements,
        IN BOOL  fExclude,
        IN BOOL  fSkipVerb,
        IN BOOL bCrLfTerminated,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    AddRequest(
        IN LPSTR lpszVerb,
        IN LPSTR lpszObjectName,
        IN LPSTR lpszVersion
        );

    DWORD
    ModifyRequest(
        IN HTTP_METHOD_TYPE tMethod,
        IN LPSTR lpszObjectName,
        IN DWORD dwObjectNameLength,
        IN LPSTR lpszVersion OPTIONAL,
        IN DWORD dwVersionLength
        );

    HEADER_STRING * GetFirstHeader(VOID) const {

        INET_ASSERT(_lpHeaders != NULL);

        return _lpHeaders;
    }

    HEADER_STRING * GetEmptyHeader(VOID) const {
        for (DWORD i = 0; i < _TotalSlots; ++i) {
            if (_lpHeaders[i].HaveString() && _lpHeaders[i].StringLength() == 0) {
                return &_lpHeaders[i];
            }
        }
        return NULL;
    }

    VOID SetIsRequestHeaders(BOOL bRequestHeaders) {
        _IsRequestHeaders = bRequestHeaders;
    }

    DWORD GetError(VOID) const {
        return _Error;
    }

    LPSTR GetVerb(LPDWORD lpdwVerbLength) const {
        *lpdwVerbLength = _dwVerbLength;
        return _lpszVerb;
    }

    //VOID SetRequestVersion(DWORD dwMajor, DWORD dwMinor) {
    //    _RequestVersionMajor = dwMajor;
    //    _RequestVersionMinor = dwMinor;
    //}

    VOID
    SetRequestVersion(
        VOID
        );

    DWORD MajorVersion(VOID) const {
        return _RequestVersionMajor;
    }

    DWORD MinorVersion(VOID) const {
        return _RequestVersionMinor;
    }
};

// class HTTP_HEADER_PARSER
//
// Retrieves HTTP headers from string containing server response headers.
//

class HTTP_HEADER_PARSER : public HTTP_HEADERS
{
public:
    HTTP_HEADER_PARSER(
        LPSTR lpszHeaders,
        DWORD cbHeaders
        );

    HTTP_HEADER_PARSER()
        : HTTP_HEADERS() {}

    DWORD
    ParseHeaders(
        IN LPSTR lpHeaderBase,
        IN DWORD dwBufferLength,
        IN BOOL fEof,
        IN OUT DWORD *lpdwBufferLengthScanned,
        OUT LPBOOL pfFoundCompleteLine,
        OUT LPBOOL pfFoundEndOfHeaders
        );

    BOOL
    ParseStatusLine(
        IN LPSTR lpHeaderBase,
        IN DWORD dwBufferLength,
        IN BOOL fEof,
        IN OUT DWORD *lpdwBufferLengthScanned,
        OUT DWORD *lpdwStatusCode,
        OUT DWORD *lpdwMajorVersion,
        OUT DWORD *lpdwMinorVersion
        );

};


//
// flags for AddHeader
//

#define CLEAN_HEADER                   0x00000001                  // if set, header should be cleaned up
#define ADD_HEADER_IF_NEW              HTTP_ADDREQ_FLAG_ADD_IF_NEW // only add the header if it doesn't already exist
#define ADD_HEADER                     HTTP_ADDREQ_FLAG_ADD        // if replacing and header not found, add it
#define COALESCE_HEADER_WITH_COMMA     HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA   // headers of the same name will be coalesced
#define COALESCE_HEADER_WITH_SEMICOLON HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON   // headers of the same name will be coalesced
#define REPLACE_HEADER                 HTTP_ADDREQ_FLAG_REPLACE    // not currently used internally

/*++

Class Description:

    This class defines the HTTP_REQUEST_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the service handle value from
        the generic object handle.

--*/

class HTTP_REQUEST_HANDLE_OBJECT : public INTERNET_CONNECT_HANDLE_OBJECT {

private:

    LIST_ENTRY m_PipelineList;

    //
    // m_lPriority - relative priority used to determine which request gets the
    // next available connection
    //

    LONG m_lPriority;

    //
    // _Socket - this is the socket we are using for this request. It may be a
    // pre-existing keep-alive connection or a new connection (not necessarily
    // keep-alive)
    //

    ICSocket * _Socket;

    //
    // _bKeepAliveConnection - if TRUE, _Socket is keep-alive, else we must
    // really close it
    //

    BOOL _bKeepAliveConnection;

    //
    // _bNoLongerKeepAlive - if this ever gets set to TRUE its because we began
    // with a keep-alive connection which reverted to non-keep-alive when the
    // server responded with no "(Proxy-)Connection: Keep-Alive" header
    //

    BOOL _bNoLongerKeepAlive;

    //
    // _QueryBuffer - buffer used to query socket data available
    //

    LPVOID _QueryBuffer;

    //
    // _QueryBufferLength - length of _QueryBuffer
    //

    DWORD _QueryBufferLength;

    //
    // _QueryOffset - offset of next read from _QueryBuffer
    //

    DWORD _QueryOffset;

    //
    // _QueryBytesAvailable - number of bytes we think are available for this
    // socket in the query buffer
    //

    DWORD _QueryBytesAvailable;

    //
    // _OpenFlags - flags specified in HttpOpenRequest()
    //

    DWORD _OpenFlags;

    //
    // _State - the HTTP request/response state
    //

    HTTPREQ_STATE _State;

    //
    // HTTP request information
    //

    //
    // _RequestHeaders - collection of request headers, including the request
    // line
    //

    HTTP_HEADERS _RequestHeaders;

    //
    // _RequestMethod - (known) method used to make HTTP request
    //

    HTTP_METHOD_TYPE _RequestMethod;

    //
    // Values for optional data saved offin handle when in negotiate stage.
    //

    DWORD   _dwOptionalSaved;

    LPVOID  _lpOptionalSaved;

    BOOL    _fOptionalSaved;

    //
    // HTTP response information
    //

    //
    // _ResponseHeaders - collection of response headers, including the status
    // line
    //

    HTTP_HEADER_PARSER _ResponseHeaders;

    //
    // In the case of response headers, remember slot number for
    // Content-Length and Content-Range in case we need to fix up
    // a 206 partial content response.
    //

    DWORD _iSlotContentLength;
    DWORD _iSlotContentRange;

    //
    // _StatusCode - return status from the server
    //

    DWORD _StatusCode;

    //
    // _ResponseBuffer - pointer to the buffer containing part or all of
    // response, starting with headers (if >= HTTP/1.0, i.e. if IsUpLevel)
    //

    LPBYTE _ResponseBuffer;

    //
    // _ResponseBufferLength - length of _ResponseBuffer
    //

    DWORD _ResponseBufferLength;

    //
    // _BytesReceived - number of bytes received into _ResponseBuffer
    //

    DWORD _BytesReceived;

    //
    // _ResponseScanned - amount of response buffers scanned for eof headers
    //

    DWORD _ResponseScanned;

    //
    // _ResponseBufferDataReadyToRead - special length of response buffer,
    //  set if we've parsed it from a chunk-transfer stream, this will be
    //  the correct length
    //

    DWORD _ResponseBufferDataReadyToRead;

    //
    // _DataOffset - the offset in _ResponseBuffer at which the response data
    // starts (data after headers)
    //

    DWORD _DataOffset;

    //
    // _BytesRemaining - number of _ContentLength bytes remaining to be read by
    // application
    //

    DWORD _BytesRemaining;

    //
    // _ContentLength - as parsed from the response headers
    //

    DWORD _ContentLength;

    //
    // _BytesInSocket - if content-length, the number of bytes we have yet to
    // receive from the socket
    //

    DWORD _BytesInSocket;

    //
    // time stamps parsed from response
    //

    FILETIME _ftLastModified;
    FILETIME _ftExpires;
    FILETIME _ftPostCheck;

    //
    // cache lookup struct
    //

    CACHE_ENTRY_INFOEX* _pCacheEntryInfo;

    //
    // _ctChunkInfo - Chunk Info for tracking a chunk stream.
    //

    CHUNK_TRANSFER _ctChunkInfo;

    //
    // _fTalkingToSecureServerViaProxy - We're talking SSL, but
    //      actually we're connected through a proxy so things
    //      need to be carefully watched.  Don't send a Proxy
    //      Username and Password to the the secure sever.
    //

    BOOL   _fTalkingToSecureServerViaProxy;

    //
    // _fRequestUsingProxy - TRUE if we're actually using the proxy
    //  to reach the server. Needed to keep track of whether
    //  we're using the proxy or not.
    //

    BOOL   _fRequestUsingProxy;

    //
    // _bWantKeepAlive - TRUE if we want a keep-alive connection
    //

    BOOL _bWantKeepAlive;

    //
    // _bRefresh - TRUE if the response contains a "Refresh" header
    //

    BOOL _bRefresh;

    //
    // _RefreshHeader - value from the "Refresh" header, if it exists
    //

    HEADER_STRING _RefreshHeader;

    //
    // _dwQuerySetCookieHeader - Passed to HttpQueryInfo to track what
    //                              cookie header we're parsing.
    //

    DWORD _dwQuerySetCookieHeader;

    //
    // _Union - get at flags individually, or as DWORD so they can be zapped
    //

    union {

        //
        // Flags - several bits of information gleaned from the response, such
        // as whether the server responded with a "connection: keep-alive", etc.
        //

        struct {
            DWORD Eof               : 1;    // we have received all response
            DWORD DownLevel         : 1;    // response is HTTP/0.9
            DWORD UpLevel           : 1;    // response is HTTP/1.0 or greater
            DWORD Http1_1Response   : 1;    // response is HTTP/1.1 or greater
            DWORD MustRevalidate    : 1;    // response contains cache-control: must-revalidate
            DWORD KeepAlive         : 1;    // response contains keep-alive header
            DWORD ConnCloseResponse : 1;    // response contains connection: close header
            DWORD PersistServer     : 1;    // have persistent connection to server
            DWORD PersistProxy      : 1;    // have persistent connection to proxy
            DWORD Data              : 1;    // set if we have got to the data part
            DWORD ContentLength     : 1;    // set if we have parsed Content-Length
            DWORD ChunkEncoding     : 1;    // set if we have parsed a Transfer-Encoding: chunked
            DWORD AutoSync          : 1;    // set when synchronizing due to sync-mode
            DWORD BadNSServer       : 1;    // set when server is bogus NS 1.1
            DWORD ConnCloseChecked  : 1;    // set when we have checked for Connection: Close
            DWORD ConnCloseReq      : 1;    // set if Connection: Close in request headers
            DWORD ProxyConnCloseReq : 1;    // set if Proxy-Connection: Close in request headers
            //DWORD CookieUI          : 1;    // set if we're doing Cookie UI
        } Flags;

        //
        // Dword - used in initialization and ReuseObject
        //

        DWORD Dword;

    } _Union;

    //
    // Filter, authentication related members.
    //

    AUTHCTX* _pAuthCtx;         // authentication context
    AUTHCTX* _pTunnelAuthCtx;   // context for nested request
    PWC*     _pPWC;             // PWC* for Basic, Digest authctxt
    LPVOID   _lpBlockingFilter; // filter that requested UI
    DWORD    _dwCredPolicy;     // zone policy on use of credentials

    union {

        struct {
            // AuthState: none, negotiate, challenge, or need tunnel.
            DWORD AuthState               : 2;

            // TRUE if KA socket should be flushed with password cache.
            DWORD IsAuthorized            : 1;

            // TRUE if request has been processed once.
            DWORD FirstSendProcessed      : 1;

            // TRUE if client added header that implies no caching.
            DWORD CacheReadDisabled       : 1;

            // TRUE if this handle should ignore general proxy settings,
            // and use the proxy found in this object
            DWORD OverrideProxyMode       : 1;

            // TRUE, if we are using a proxy to create a tunnel.
            DWORD IsTunnel                : 1;

            // TRUE, if the object was originally "/"
            DWORD IsObjectRoot           : 1;

            // TRUE, if a method body is to be transmitted.
            DWORD MethodBody             : 1;

            // TRUE, if NTLM preauth is to be disabled.
            DWORD DisableNTLMPreauth     : 1;

        } Flags;

        //
        // Dword - used in initialization ONLY, do NOT use ELSEWHERE !
        //

        DWORD Dword;

    } _NoResetBits;

    //
    // Proxy, and Socks Proxy Information, used to decide if need to go through
    //  a proxy, a socks proxy, or no proxy at all (NULLs).
    //

    LPSTR _ProxyHostName;
    DWORD _ProxyHostNameLength;
    INTERNET_PORT _ProxyPort;

    LPSTR _SocksProxyHostName;
    DWORD _SocksProxyHostNameLength;
    INTERNET_PORT _SocksProxyPort;

    //
    // _ProxySchemeType - Used to determine the proxy scheme, in proxy override mode.
    //

    //INTERNET_SCHEME _ProxySchemeType;

    //
    // InternetReadFileEx data
    //
#ifndef unix
    DWORD _ReadFileExData;
#else
    BYTE  _ReadFileExData;
#endif /* unix */
    BOOL _HaveReadFileExData;
    INTERNET_BUFFERS _BuffersOut;

    //
    //  _redirectCount - The number of times we've redirected on this request
    //

    DWORD _redirectCount;

    //
    //  _redirectCountedOut - TRUE if we redirected too many times.
    //

    BOOL  _redirectCountedOut;

    //
    //   _AddCRLFToPost - TRUE if we need to add a CRLF to the POST.
    //

    BOOL _AddCRLFToPOST;

    //
    // info we used to keep in the secure socket object
    //

    SECURITY_CACHE_LIST_ENTRY *m_pSecurityInfo;

    //
    // _RTT - round-trip time for this request
    //

    DWORD _RTT;

    //
    // _CP - code page to use for char width conversions
    //

    UINT _CP;


    //
    // _fIgnore Offline - ignore global offline (enable loopback)
    //
    BOOL    _fIgnoreOffline;

    //
    // private response functions
    //

    PRIVATE
    BOOL
    FindEndOfHeader(
        IN OUT LPSTR * lpszResponse,
        IN LPSTR lpszEnd,
        OUT LPDWORD lpdwHeaderLength
        );

    PRIVATE
    VOID
    CheckForWellKnownHeader(
        IN LPSTR lpszHeader,
        IN DWORD dwHeaderLength,
        IN DWORD iSlot
        );

    PRIVATE
    VOID
    CheckWellKnownHeaders(
        VOID
        );

    HANDLE GetDownloadFileReadHandle (VOID) {

        if (_CacheFileHandleRead == INVALID_HANDLE_VALUE) {
            _CacheFileHandleRead = CreateFile( _CacheFileName, GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL, NULL);
        }
        return _CacheFileHandleRead;
    }


    VOID ZapFlags(VOID) {

        //
        // clear out all bits
        //

        _Union.Dword = 0;
    }

public:

    HTTP_REQUEST_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT *Parent,
        HINTERNET Child,
        CLOSE_HANDLE_FUNC wCloseFunc,
        DWORD dwFlags,
        DWORD_PTR dwContext
        );

    virtual ~HTTP_REQUEST_HANDLE_OBJECT(VOID);

    virtual HINTERNET GetHandle(VOID);

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return TypeHttpRequestHandle;
    }

    //
    // request headers functions
    //

    DWORD
    AddRequest(
        IN LPSTR lpszVerb,
        IN LPSTR lpszObjectName,
        IN LPSTR lpszVersion
        ) {
        return _RequestHeaders.AddRequest(lpszVerb,
                                          lpszObjectName,
                                          lpszVersion
                                          );
    }

    DWORD
    ModifyRequest(
        IN HTTP_METHOD_TYPE tMethod,
        IN LPSTR lpszObjectName,
        IN DWORD dwObjectNameLength,
        IN LPSTR lpszVersion OPTIONAL,
        IN DWORD dwVersionLength
        ) {

        DWORD error;

        error = _RequestHeaders.ModifyRequest(tMethod,
                                              lpszObjectName,
                                              dwObjectNameLength,
                                              lpszVersion,
                                              dwVersionLength
                                              );
        if (error == ERROR_SUCCESS) {
            SetMethodType(tMethod);
        }
        return error;
    }

    VOID SetMethodType(IN LPCSTR lpszVerb) {
        _RequestMethod = MapHttpRequestMethod(lpszVerb);
    }

    VOID SetMethodType(IN HTTP_METHOD_TYPE tMethod) {
        _RequestMethod = tMethod;
    }

    HTTP_METHOD_TYPE GetMethodType(VOID) const {
        return _RequestMethod;
    }

    LPSTR
    CreateRequestBuffer(
        OUT LPDWORD lpdwRequestLength,
        IN LPVOID lpOptional,
        IN DWORD dwOptionalLength,
        IN BOOL bExtraCrLf,
        IN DWORD dwMaxPacketLength,
        OUT LPBOOL lpbCombinedData
        );


    DWORD
    AddRequestHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.AddHeader(lpszHeaderName,
                                         dwHeaderNameLength,
                                         lpszHeaderValue,
                                         dwHeaderValueLength,
                                         dwIndex,
                                         dwFlags
                                         );
    }

    DWORD
    AddRequestHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.AddHeader(dwQueryIndex,
                                         lpszHeaderValue,
                                         dwHeaderValueLength,
                                         dwIndex,
                                         dwFlags
                                         );
    }



    DWORD
    ReplaceRequestHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.ReplaceHeader(lpszHeaderName,
                                             dwHeaderNameLength,
                                             lpszHeaderValue,
                                             dwHeaderValueLength,
                                             dwIndex,
                                             dwFlags
                                             );
    }

    DWORD
    ReplaceRequestHeader(
        IN DWORD dwQueryIndex,
        IN LPSTR lpszHeaderValue,
        IN DWORD dwHeaderValueLength,
        IN DWORD dwIndex,
        IN DWORD dwFlags
        ) {
        return _RequestHeaders.ReplaceHeader(dwQueryIndex,
                                             lpszHeaderValue,
                                             dwHeaderValueLength,
                                             dwIndex,
                                             dwFlags
                                             );
    }


    DWORD
    QueryRequestHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    QueryRequestHeader(
        IN DWORD dwQueryIndex,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        );



    //
    // response headers functions
    //

    DWORD
    AddInternalResponseHeader(
        IN DWORD dwHeaderIndex,
        IN LPSTR lpszHeader,
        IN DWORD dwHeaderLength
        );

    DWORD
    UpdateResponseHeaders(
        IN OUT LPBOOL lpbEof
        );

    DWORD
    CreateResponseHeaders(
        IN OUT LPSTR* ppszBuffer,
        IN     DWORD  dwBufferLength
        );

    DWORD
    FindResponseHeader(
        IN LPSTR lpszHeaderName,
        OUT LPSTR lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    QueryResponseVersionDword(
        IN OUT LPDWORD lpdwVersionMajor,
        IN OUT LPDWORD lpdwVersionMinor
        );

    DWORD
    QueryResponseVersion(
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    DWORD
    QueryStatusCode(
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers
        );

    DWORD
    QueryStatusText(
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );


    DWORD
    FastQueryResponseHeader(
        IN DWORD dwQueryIndex,
        OUT LPVOID *lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT DWORD dwIndex
        )
    {
        return _ResponseHeaders.FastFindHeader(
                                    (LPSTR)_ResponseBuffer,
                                    dwQueryIndex,
                                    lplpBuffer,
                                    lpdwBufferLength,
                                    dwIndex
                                    );
    }


    DWORD
    FastQueryRequestHeader(
        IN DWORD dwQueryIndex,
        OUT LPVOID *lplpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT DWORD dwIndex
        )
    {
        return _RequestHeaders.FastFindHeader(
                                    (LPSTR)_ResponseBuffer,
                                    dwQueryIndex,
                                    lplpBuffer,
                                    lpdwBufferLength,
                                    dwIndex
                                    );
    }

    DWORD
    QueryResponseHeader(
        IN LPSTR lpszHeaderName,
        IN DWORD dwHeaderNameLength,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        )

    {
        //
        // this is the slow manner for finding a header, avoid doing this by calling
        //   the faster method
        //

        return _ResponseHeaders.FindHeader((LPSTR)_ResponseBuffer,
                                            lpszHeaderName,
                                            dwHeaderNameLength,
                                            dwModifiers,
                                            lpBuffer,
                                            lpdwBufferLength,
                                            lpdwIndex
                                            );
    }

    DWORD
    QueryResponseHeader(
        IN DWORD dwQueryIndex,
        IN LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN DWORD dwModifiers,
        IN OUT LPDWORD lpdwIndex
        )
    {
        return _ResponseHeaders.FindHeader((LPSTR)_ResponseBuffer,
                                            dwQueryIndex,
                                            dwModifiers,
                                            lpBuffer,
                                            lpdwBufferLength,
                                            lpdwIndex
                                            );
    }


    BOOL IsResponseHeaderPresent(DWORD dwQueryIndex) const {
        return _ResponseHeaders.IsHeaderPresent(dwQueryIndex);
    }

    BOOL IsRequestHeaderPresent(DWORD dwQueryIndex) const {
        return _RequestHeaders.IsHeaderPresent(dwQueryIndex);
    }

    DWORD
    QueryRawResponseHeaders(
        IN BOOL bCrLfTerminated,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
        );

    HEADER_STRING * GetStatusLine(VOID) const {

        //
        // _StatusLine is just a reference for the first response header
        //

        return _ResponseHeaders.GetFirstHeader();
    }

    //
    // general headers methods
    //

    DWORD
    QueryInfo(
        IN DWORD dwInfoLevel,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength,
        IN OUT LPDWORD lpdwIndex
        );

    DWORD
    QueryFilteredRawResponseHeaders(
        LPSTR   *lplpFilterList,
        DWORD   cListElements,
        BOOL    fExclude,
        BOOL    fSkipVerb,
        BOOL    bCrLfTerminated,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
    )
    {
       return (_ResponseHeaders.QueryFilteredRawHeaders(
           (LPSTR)_ResponseBuffer,
           lplpFilterList,
           cListElements,
           fExclude,
           fSkipVerb,
           bCrLfTerminated,
           lpBuffer,
           lpdwBufferLength));

    };

    DWORD
    QueryRequestHeadersWithEcho(
        BOOL    bCrLfTerminated,
        OUT LPVOID lpBuffer,
        IN OUT LPDWORD lpdwBufferLength
    );



    //
    // connection-oriented methods
    //

    DWORD
    InitBeginSendRequest(
        IN LPCSTR lpszHeaders OPTIONAL,
        IN DWORD dwHeadersLength,
        IN LPVOID *lplpOptional,
        IN LPDWORD lpdwOptionalLength,
        IN DWORD dwOptionalLengthTotal,
        OUT LPBOOL pfGoneOffline
        );

    DWORD
    QueueAsyncSendRequest(
        IN LPVOID lpOptional OPTIONAL,
        IN DWORD dwOptionalLength,
        IN AR_TYPE arRequest,
        IN FARPROC lpfpAsyncCallback
        );


    DWORD
    HttpBeginSendRequest(
        IN LPVOID lpOptional OPTIONAL,
        IN DWORD dwOptionalLength
        );

    DWORD
    HttpEndSendRequest(
        VOID
        );

    DWORD
    SockConnect(
        IN LPSTR TargetServer,
        IN INTERNET_PORT TcpipPort,
        IN LPSTR SocksHostName,
        IN INTERNET_PORT SocksPort
        );

    DWORD
    OpenConnection(
        IN BOOL NoKeepAlive
        );

    DWORD
    OpenConnection_Fsm(
        IN CFsm_OpenConnection * Fsm
        );

    DWORD
    CloseConnection(
        IN BOOL ForceClosed
        );

    VOID
    ReleaseConnection(
        IN BOOL bClose,
        IN BOOL bIndicate,
        IN BOOL bDelete
        );

    DWORD
    AbortConnection(
        IN BOOL bForce
        );

    DWORD
    OpenProxyTunnel(
        VOID
        );

    DWORD
    OpenProxyTunnel_Fsm(
        IN CFsm_OpenProxyTunnel * Fsm
        );

    DWORD
    CloneResponseBuffer(
        IN HTTP_REQUEST_HANDLE_OBJECT *pChildRequestObj
        );

    DWORD
    HttpReadData_Fsm(
        IN CFsm_HttpReadData * Fsm
        );

    DWORD
    HttpWriteData_Fsm(
        IN CFsm_HttpWriteData * Fsm
        );

    DWORD
    ReadResponse(
        VOID
        );

    DWORD
    ReadData(
        OUT LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToRead,
        OUT LPDWORD lpdwNumberOfBytesRead,
        IN BOOL fNoAsync,
        IN DWORD dwSocketFlags
        );

    DWORD
    ReadData_Fsm(
        IN CFsm_ReadData * Fsm
        );

    DWORD
    WriteData(
        OUT LPVOID lpBuffer,
        IN DWORD dwNumberOfBytesToWrite,
        OUT LPDWORD lpdwNumberOfBytesWritten
        );

    DWORD
    QueryDataAvailable(
        OUT LPDWORD lpdwNumberOfBytesAvailable
        );

    DWORD
    QueryAvailable_Fsm(
        IN CFsm_HttpQueryAvailable * Fsm
        );

    DWORD
    DrainResponse(
        OUT LPBOOL lpbDrained
        );

    DWORD
    DrainResponse_Fsm(
        IN CFsm_DrainResponse * Fsm
        );

    DWORD
    LocalEndCacheWrite(
        IN BOOL fNormal
        );

    VOID
    GetTimeStampsForCache(
        OUT LPFILETIME lpftExpiryTime,
        OUT LPFILETIME lpftLastModTime,
        OUT LPFILETIME lpftPostCheckTime,
        OUT LPBOOL lpfHasExpiry,
        OUT LPBOOL lpfHasLastModTime,
        OUT LPBOOL lpfHasPostCheck
        );

    BOOL IsPartialResponseCacheable (void);

    DWORD
    Redirect(
        IN HTTP_METHOD_TYPE tMethod,
        IN BOOL fRedirectToProxy
        );

    DWORD
    Redirect_Fsm(
        IN CFsm_Redirect * Fsm
        );

    DWORD
    BuildProxyMessage(
        IN CFsm_HttpSendRequest * Fsm,
        AUTO_PROXY_ASYNC_MSG * pProxyMsg,
        IN OUT URL_COMPONENTS * pUrlComponents
        );

    DWORD
    QueryProxySettings(
        IN CFsm_HttpSendRequest * Fsm,
        INTERNET_HANDLE_OBJECT * pInternet,
        IN OUT URL_COMPONENTS * pUrlComponents
        );

    DWORD
    CheckForCachedProxySettings(
        IN AUTO_PROXY_ASYNC_MSG *pProxyMsg,
        OUT CServerInfo **ppProxyServerInfo
        );

    DWORD
    ProcessProxySettings(
        IN CFsm_HttpSendRequest * Fsm,
        IN INTERNET_CONNECT_HANDLE_OBJECT * pConnect,
        IN OUT URL_COMPONENTS * pUrlComponents,
        OUT LPSTR * lplpszRequestObject,
        OUT DWORD * lpdwRequestObjectSize
        );

    DWORD
    UpdateRequestInfo(
        IN CFsm_HttpSendRequest * Fsm,
        IN LPSTR lpszObject,
        IN DWORD dwcbObject,
        IN OUT URL_COMPONENTS * pUrlComponents,
        IN OUT CServerInfo **ppProxyServerInfo
        );

    DWORD
    UpdateProxyInfo(
        IN CFsm_HttpSendRequest * Fsm,
        IN BOOL fCallback
        );

    BOOL
    FindConnCloseRequestHeader(
        IN DWORD dwHeaderIndex
        );

    VOID
    RemoveAllRequestHeadersByName(
        IN DWORD dwQueryIndex
        );

    //
    // response buffer/data methods
    //

    BOOL IsBufferedData(VOID) {

        BOOL fIsBufferedData;

        //INET_ASSERT(IsData());

        fIsBufferedData = (_DataOffset < _BytesReceived) ? TRUE : FALSE;

        if ( fIsBufferedData &&
             IsChunkEncoding() &&
             _ResponseBufferDataReadyToRead == 0 )
        {
            fIsBufferedData = FALSE;
        }

        return (fIsBufferedData);
    }

    DWORD BufferedDataLength(VOID) {
        return (DWORD)(_BytesReceived - _DataOffset);
    }

    DWORD BufferDataAvailToRead(VOID) {
        return ( IsChunkEncoding() ) ? _ResponseBufferDataReadyToRead : BufferedDataLength() ;
    }

    VOID ReduceDataAvailToRead(DWORD dwReduceBy)
    {
        if ( IsChunkEncoding() )
        {
            _ResponseBufferDataReadyToRead -= dwReduceBy;
        }
        //else
        //{
            _DataOffset += dwReduceBy;
        //}
    }

    LPVOID BufferedDataStart(VOID) {
        return (LPVOID)(_ResponseBuffer + _DataOffset);
    }

    VOID SetCookieQuery(DWORD QueryIndex) {
        _dwQuerySetCookieHeader = QueryIndex;
    }

    DWORD GetCookieQuery(VOID) const {
        return _dwQuerySetCookieHeader;
    }


    VOID SetContentLength(DWORD ContentLength) {
        _ContentLength = ContentLength;
    }

    DWORD GetContentLength(VOID) const {
        return _ContentLength;
    }

    DWORD GetBytesInSocket(VOID) const {
        return _BytesInSocket;
    }

    DWORD GetBytesRemaining(VOID) const {
        return _BytesRemaining;
    }

    DWORD GetStatusCode(VOID) const {
        return _StatusCode;
    }

    VOID SetRefreshHeader(LPSTR Header, DWORD Length) {
        _RefreshHeader.MakeCopy(Header, Length);
    }

    HEADER_STRING & GetRefreshHeader(VOID) {
        return _RefreshHeader;
    }

    VOID FreeResponseBuffer(VOID) {
        if (_ResponseBuffer != NULL) {
            _ResponseBuffer = (LPBYTE)FREE_MEMORY((HLOCAL)_ResponseBuffer);

            INET_ASSERT(_ResponseBuffer == NULL);

        }
        _ResponseBufferLength = 0;
        _BytesReceived = 0;
        _DataOffset = 0;
    }

    VOID FreeQueryBuffer(VOID) {
        if (_QueryBuffer != NULL) {
            _QueryBuffer = (LPVOID)FREE_MEMORY((HLOCAL)_QueryBuffer);

            INET_ASSERT(_QueryBuffer == NULL);

            _QueryBuffer = NULL;
            _QueryBufferLength = 0;
            _QueryOffset = 0;
            _QueryBytesAvailable = 0;
        }
    }

    BOOL HaveQueryData(VOID) {
        return (_QueryBytesAvailable != 0) ? TRUE : FALSE;
    }

    DWORD CopyQueriedData(LPVOID lpBuffer, DWORD dwBufferLength) {

        INET_ASSERT(lpBuffer != NULL);
        INET_ASSERT(dwBufferLength != 0);

        DWORD len = min(_QueryBytesAvailable, dwBufferLength);

        if (len != 0) {
            memcpy(lpBuffer,
                   (LPVOID)((LPBYTE)_QueryBuffer + _QueryOffset),
                   len
                   );
            _QueryOffset += len;
            _QueryBytesAvailable -= len;
        }
        return len;
    }

    VOID ResetResponseVariables(VOID) {

        _StatusCode = 0;
        _BytesReceived = 0;
        _ResponseScanned = 0;
        _iSlotContentLength = 0;
        _iSlotContentRange = 0;
        _ResponseBufferDataReadyToRead = 0;
        _DataOffset = 0;
        _BytesRemaining = 0;
        _ContentLength = 0;
        _BytesInSocket = 0;
        ZapFlags();
        _ftLastModified.dwLowDateTime = 0;
        _ftLastModified.dwHighDateTime = 0;
        _ftExpires.dwLowDateTime = 0;
        _ftExpires.dwHighDateTime = 0;
        _ftPostCheck.dwLowDateTime = 0;
        _ftPostCheck.dwHighDateTime = 0;
    }

    //
    // flags methods
    //

    VOID SetEof(BOOL Value) {
        _Union.Flags.Eof = Value ? 1 : 0;
    }

    BOOL IsEof(VOID) const {
        return _Union.Flags.Eof;
    }

    VOID SetDownLevel(BOOL Value) {
        _Union.Flags.DownLevel = Value ? 1 : 0;
    }

    BOOL IsDownLevel(VOID) const {
        return _Union.Flags.DownLevel;
    }

    BOOL IsRequestHttp1_1() {
        return (_RequestHeaders.MajorVersion() > 1)
                ? TRUE
                : (((_RequestHeaders.MajorVersion() == 1)
                    && (_RequestHeaders.MinorVersion() >= 1))
                    ? TRUE
                    : FALSE);
    }

    BOOL IsRequestHttp1_0() {
        return ((_RequestHeaders.MajorVersion() == 1)
                    && (_RequestHeaders.MinorVersion() == 0));
    }

    VOID SetResponseHttp1_1(BOOL Value) {
        _Union.Flags.Http1_1Response = Value ? 1 : 0;
    }

    BOOL IsResponseHttp1_1(VOID) const {
        return _Union.Flags.Http1_1Response;
    }

    VOID SetMustRevalidate (VOID) {
        _Union.Flags.MustRevalidate = 1;
    }

    BOOL IsMustRevalidate (VOID) const {
        return _Union.Flags.MustRevalidate;
    }

    VOID SetUpLevel(BOOL Value) {
        _Union.Flags.UpLevel = Value ? 1 : 0;
    }

    BOOL IsUpLevel(VOID) const {
        return _Union.Flags.UpLevel;
    }

    VOID SetKeepAlive(BOOL Value) {
        _Union.Flags.KeepAlive = Value ? 1 : 0;
    }

    BOOL IsKeepAlive(VOID) const {
        return _Union.Flags.KeepAlive;
    }

    VOID SetConnCloseResponse(BOOL Value) {
        _Union.Flags.ConnCloseResponse = Value ? 1 : 0;
    }

    BOOL IsConnCloseResponse(VOID) const {
        return _Union.Flags.ConnCloseResponse;
    }

    VOID SetData(BOOL Value) {
        _Union.Flags.Data = Value ? 1 : 0;
    }

    BOOL IsData(VOID) const {
        return _Union.Flags.Data;
    }

    VOID SetOpenFlags(DWORD OpenFlags) {
        _OpenFlags = OpenFlags;
    }

    DWORD GetOpenFlags(VOID) {
        return _OpenFlags;
    }

    //VOID SetCookieUI(BOOL Value)
    //{
    //    _Union.Flags.CookieUI = (Value) ? TRUE : FALSE;
    //}

    //BOOL IsCookieUI() const
    //{
    //    return _Union.Flags.CookieUI;
    //}

    VOID SetHaveChunkEncoding(BOOL Value) {
        _Union.Flags.ChunkEncoding = Value ? 1 : 0;
    }

    BOOL IsChunkEncoding(VOID) const {
        return _Union.Flags.ChunkEncoding;
    }

    VOID SetHaveContentLength(BOOL Value) {
        _Union.Flags.ContentLength = Value ? 1 : 0;
    }

    BOOL IsContentLength(VOID) const {
        return _Union.Flags.ContentLength;
    }

    VOID SetAutoSync(VOID) {
        _Union.Flags.AutoSync = 1;
    }

    BOOL IsAutoSync(VOID) {
        return _Union.Flags.AutoSync == 1;
    }

    VOID SetBadNSServer(BOOL Value) {
        _Union.Flags.BadNSServer = Value ? 1 : 0;
    }

    BOOL IsBadNSServer(VOID) const {
        return _Union.Flags.BadNSServer ? TRUE : FALSE;
    }

    VOID SetCheckedConnCloseRequest(BOOL bProxy, BOOL bFound) {
        _Union.Flags.ConnCloseChecked = 1;
        if (bProxy) {
            _Union.Flags.ProxyConnCloseReq = bFound ? 1 : 0;
        } else {
            _Union.Flags.ConnCloseReq = bFound ? 1 : 0;
        }
    }

    BOOL CheckedConnCloseRequest(VOID) {
        return (_Union.Flags.ConnCloseChecked == 1) ? TRUE : FALSE;
    }

    BOOL IsConnCloseRequest(BOOL bProxyHeader) {
        return bProxyHeader
                    ? (_Union.Flags.ProxyConnCloseReq == 1)
                    : (_Union.Flags.ConnCloseReq == 1);
    }

    VOID
    SetBadNSReceiveTimeout(
        VOID
        );

    BOOL IsChunkedEncodingFinished(VOID)  {
        return _ctChunkInfo.IsFinished();
    }

    VOID CheckClientRequestHeaders (VOID);

    VOID SetFirstSendProcessed() {
        _NoResetBits.Flags.FirstSendProcessed = 1;
    }

    BOOL IsFirstSendProcessed() const {
        return _NoResetBits.Flags.FirstSendProcessed;
    }

    VOID SetObjectRoot() {
        _NoResetBits.Flags.IsObjectRoot = 1;
    }

    BOOL IsObjectRoot() {
        return _NoResetBits.Flags.IsObjectRoot;
    }

    VOID SetOverrideProxyMode(BOOL Value) {
        _NoResetBits.Flags.OverrideProxyMode = (Value ? TRUE : FALSE );
    }

    BOOL IsOverrideProxyMode() const {
        return _NoResetBits.Flags.OverrideProxyMode;
    }

    VOID SetCacheReadDisabled (VOID) {
        _NoResetBits.Flags.CacheReadDisabled = 1;
    }

    BOOL IsCacheReadDisabled (VOID) const {
        return _NoResetBits.Flags.CacheReadDisabled;
    }

    VOID SetWantKeepAlive(BOOL Value) {
        _bWantKeepAlive = Value;
    }

    BOOL IsWantKeepAlive(VOID) const {
        return _bWantKeepAlive;
    }

    VOID SetRefresh(BOOL Value) {
        _bRefresh = Value;
    }

    BOOL IsRefresh(VOID) const {
        return _bRefresh;
    }

    VOID SetAddCRLF(BOOL Value) {
        _AddCRLFToPOST = Value;
    }

    //
    // Bit to distinguish nested request for establishing a tunnel.
    //

    VOID SetTunnel (VOID) {
        _NoResetBits.Flags.IsTunnel = 1;
    }

    BOOL IsTunnel(VOID) const {
        return (BOOL) _NoResetBits.Flags.IsTunnel;
    }

    //
    // secure (socket) methods
    //

    VOID SetSecureFlags(DWORD Flags) {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->SetSecureFlags(Flags);
        }
    }

    DWORD GetSecureFlags(VOID) {
        if(m_pSecurityInfo)
        {
            return m_pSecurityInfo->GetSecureFlags();
        }
        return 0;
    }


    VOID LockHeaders(VOID) {
        //EnterCriticalSection(&_CritSec);
        _RequestHeaders.LockHeaders();
    }

    VOID UnlockHeaders(VOID) {
        //LeaveCriticalSection(&_CritSec);
        _RequestHeaders.UnlockHeaders();
    }

    VOID SetIgnoreOffline(VOID) {
        _fIgnoreOffline = TRUE;
    }


    //
    // GetCertChainList (and)
    //   SetCertChainList -
    //  Sets and Gets Client Authentication Cert Chains.
    //

    CERT_CONTEXT_ARRAY* GetCertContextArray(VOID) {
        if(m_pSecurityInfo)
        {
            return m_pSecurityInfo->GetCertContextArray();
        }
        return NULL;
    }

    VOID SetCertContextArray(CERT_CONTEXT_ARRAY* pNewCertContextArray) {
        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->SetCertContextArray(pNewCertContextArray);
        }
    }

    //
    // function to get SSL Certificate Information
    //

    DWORD GetSecurityInfo(LPINTERNET_SECURITY_INFO pInfo) {

        if(m_pSecurityInfo)
        {
            m_pSecurityInfo->CopyOut(*pInfo);

            return ERROR_SUCCESS;
        }
        else
        {
            return ERROR_INTERNET_INTERNAL_ERROR;
        }
    }

    //
    // authentication related methods
    //

    VOID SetAuthCtx (AUTHCTX *pAuthCtx) {
        _pAuthCtx = pAuthCtx;
    }

    AUTHCTX* GetAuthCtx (VOID) {
        return _pAuthCtx;
    }

    VOID SetPWC (PWC *pPWC) {
        _pPWC = pPWC;
    }

    PWC* GetPWC (VOID) {
        return _pPWC;
    }

    AUTHCTX* GetTunnelAuthCtx (VOID) {
        return _pTunnelAuthCtx;
    }

    DWORD GetCredPolicy (VOID) {
        if (_dwCredPolicy == 0xFFFFFFFF) {
            _dwCredPolicy = ::GetCredPolicy (GetURL());
        }
        return _dwCredPolicy;
    }


    DWORD GetAuthState (void) {
        return _NoResetBits.Flags.AuthState;
    }

    void SetAuthState (DWORD dw) {
        INET_ASSERT( dw <= AUTHSTATE_LAST );
        _NoResetBits.Flags.AuthState = dw;
    }

    void SetAuthorized (void) {
        _NoResetBits.Flags.IsAuthorized = 1;
    }

    BOOL IsAuthorized (void) {
        return _NoResetBits.Flags.IsAuthorized;
    }

    void SetMethodBody (void) {
        _NoResetBits.Flags.MethodBody = 1;
    }

    BOOL IsMethodBody (void) {
        return _NoResetBits.Flags.MethodBody;
    }

    void SetDisableNTLMPreauth (BOOL fVal) {
        _NoResetBits.Flags.DisableNTLMPreauth = (DWORD) (fVal ? TRUE : FALSE);
    }

    BOOL IsDisableNTLMPreauth (void) {
        return _NoResetBits.Flags.DisableNTLMPreauth;
    }


    LPVOID GetBlockingFilter (void) {
        return _lpBlockingFilter;
    }

    void SetBlockingFilter (LPVOID lpFilter) {
        _lpBlockingFilter = lpFilter;
    }

    //
    // proxy methods
    //

    BOOL IsTalkingToSecureServerViaProxy(VOID) const {
        return _fTalkingToSecureServerViaProxy;
    }

    VOID SetIsTalkingToSecureServerViaProxy(BOOL fTalkingToSecureServerViaProxy) {
        _fTalkingToSecureServerViaProxy = fTalkingToSecureServerViaProxy;
    }

    VOID SetRequestUsingProxy(BOOL fRequestUsingProxy) {
        _fRequestUsingProxy = fRequestUsingProxy;
    }


    BOOL IsRequestUsingProxy(VOID) const {
        return _fRequestUsingProxy;
    }

    VOID
    GetProxyName(
        OUT LPSTR* lplpszProxyHostName,
        OUT LPDWORD lpdwProxyHostNameLength,
        OUT LPINTERNET_PORT lpProxyPort
        );

    VOID
    SetProxyName(
        IN LPSTR lpszProxyHostName,
        IN DWORD dwProxyHostNameLength,
        IN INTERNET_PORT ProxyPort
        );

    VOID
    GetSocksProxyName(
        LPSTR *lplpszProxyHostName,
        DWORD *lpdwProxyHostNameLength,
        LPINTERNET_PORT lpProxyPort
        )
    {
        *lplpszProxyHostName     = _SocksProxyHostName;
        *lpdwProxyHostNameLength = _SocksProxyHostNameLength;
        *lpProxyPort             = _SocksProxyPort;

    }


    VOID
    SetSocksProxyName(
        LPSTR lpszProxyHostName,
        DWORD dwProxyHostNameLength,
        INTERNET_PORT ProxyPort
        )
    {
        _SocksProxyHostName          = lpszProxyHostName;
        _SocksProxyHostNameLength    = dwProxyHostNameLength;
        _SocksProxyPort              = ProxyPort;

    }

    VOID ClearPersistentConnection (VOID) {
        _Union.Flags.PersistProxy  = 0;
        _Union.Flags.PersistServer = 0;
    }


    VOID SetPersistentConnection (BOOL fProxy) {
        if (fProxy) {
            _Union.Flags.PersistProxy  = 1;
        } else {
            _Union.Flags.PersistServer = 1;
        }
    }

    BOOL IsPersistentConnection (BOOL fProxy) {
        return fProxy ?
            _Union.Flags.PersistProxy : _Union.Flags.PersistServer;
    }

    //
    // functions to get/set state
    //

    VOID SetState(HTTPREQ_STATE NewState) {

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("SetState(): current state %#x [%s], new state %#x [%s]\n",
                    _State,
                    InternetMapHttpState(_State),
                    NewState,
                    InternetMapHttpState(NewState)
                    ));

        _State = NewState;
    }

    BOOL CheckState(DWORD Flag) {

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("CheckState(): current state %#x [%s], checking %#x [%s] - %B\n",
                    _State,
                    InternetMapHttpState(_State),
                    Flag,
                    InternetMapHttpStateFlag(Flag),
                    (_State & Flag) ? TRUE : FALSE
                    ));

        return (_State & Flag) ? TRUE : FALSE;
    }

    VOID
    ReuseObject(
        VOID
        );

    DWORD
    ResetObject(
        IN BOOL bForce,
        IN BOOL bFreeRequestHeaders
        );

    DWORD SetStreamPointer (LONG lDistanceToMove, DWORD dwMoveMethod);

    BOOL AttemptReadFromFile (LPVOID lpBuf, DWORD cbBuf, DWORD *pcbRead);

    VOID AdvanceReadPosition (DWORD cbRead)
    {
        _dwCurrentStreamPosition += cbRead;
        if (IsKeepAlive() && IsContentLength())
        {
            _BytesRemaining -= cbRead;
            INET_ASSERT (_BytesRemaining + _dwCurrentStreamPosition
                == _ContentLength);
        }
    }

    BOOL IsReadRequest (void)
    {
        return (IsCacheWriteInProgress() &&
            _dwCurrentStreamPosition != _VirtualCacheFileSize);
    }


    DWORD
    ReadLoop_Fsm(
        IN CFsm_ReadLoop * Fsm
        );

    DWORD WriteResponseBufferToCache (VOID);
    DWORD WriteQueryBufferToCache (VOID);

    VOID SetNoLongerKeepAlive(VOID) {
        _bNoLongerKeepAlive = TRUE;
    }

    BOOL IsNoLongerKeepAlive(VOID) const {
        return _bNoLongerKeepAlive;
    }

    //
    // send.cxx methods
    //

    DWORD
    HttpSendRequest_Start(
        IN CFsm_HttpSendRequest * Fsm
        );

    DWORD
    HttpSendRequest_Finish(
        IN CFsm_HttpSendRequest * Fsm
        );

    DWORD
    MakeConnection_Fsm(
        IN CFsm_MakeConnection * Fsm
        );

    DWORD
    SendRequest_Fsm(
        IN CFsm_SendRequest * Fsm
        );

    DWORD
    ReceiveResponse_Fsm(
        IN CFsm_ReceiveResponse * Fsm
        );

    BOOL
    FCanWriteToCache(
        VOID
        );

    BOOL
    FAddIfModifiedSinceHeader(
        IN LPCACHE_ENTRY_INFO lpCei
        );

    BOOL AddHeaderIfEtagFound (IN LPCACHE_ENTRY_INFO lpCei);

    DWORD
    FHttpBeginCacheRetrieval(
        IN BOOL bReset,
        IN BOOL bOffline,
        IN BOOL bNoRetrieveIfExist = FALSE 
        );

    DWORD UrlCacheRetrieve (BOOL fOffline)
    {
        INET_ASSERT (!_hCacheStream && !_pCacheEntryInfo);
        return ::UrlCacheRetrieve
            (_CacheUrlName, fOffline, &_hCacheStream, &_pCacheEntryInfo);
    }

    VOID UrlCacheUnlock (VOID);

    BOOL FHttpBeginCacheWrite(VOID);

    DWORD ResumePartialDownload (VOID);

    DWORD
    GetFromCachePreNetIO(
        VOID
        );

    DWORD
    GetFromCachePostNetIO(
        IN DWORD statusCode,
        IN BOOL fVariation = FALSE
        );

    DWORD
    AddTimestampsFromCacheToResponseHeaders(
        IN LPCACHE_ENTRY_INFO lpCacheEntryInfo
        );

    DWORD
    AddTimeHeader(
        IN FILETIME fTime,
        IN DWORD dwHeaderIndex
        );


    LPINTERNET_BUFFERS SetReadFileEx(VOID) {
        _BuffersOut.lpvBuffer = (LPVOID)&_ReadFileExData;

        //
        // receive ONE byte
        //

        _BuffersOut.dwBufferLength = 1;
        return &_BuffersOut;
    }

    VOID SetReadFileExData(VOID) {
        _HaveReadFileExData = TRUE;
    }

    VOID ResetReadFileExData(VOID) {
        _HaveReadFileExData = FALSE;
    }

    BOOL HaveReadFileExData(VOID) {
        return _HaveReadFileExData;
    }

    BYTE GetReadFileExData(VOID) {
        ResetReadFileExData();
        return (BYTE)_ReadFileExData;
    }

    //
    // cookie.cxx methods
    //

    int
    CreateCookieHeaderIfNeeded(
        VOID
        );

    DWORD
    ExtractSetCookieHeaders(
        LPDWORD lpdwHeaderIndex
        );

    LPCACHE_ENTRY_INFO GetCacheEntryInfo(VOID){
        return _pCacheEntryInfo;
    }

    BOOL IsInCache(VOID) {
        return _pCacheEntryInfo != NULL;
    }

    BOOL CanRetrieveFromCache(BOOL fCheckCache = TRUE) {
        return ((fCheckCache && IsInCache()) || !fCheckCache)
            && !IsAutoSync()
            && !(_OpenFlags
                 & (INTERNET_FLAG_RELOAD | INTERNET_FLAG_RESYNCHRONIZE));
    }

    //
    // priority methods
    //

    LONG GetPriority(VOID) const {
        return m_lPriority;
    }

    VOID SetPriority(LONG lPriority) {
        m_lPriority = lPriority;
    }

    //
    // Round Trip Time methods
    //

    VOID StartRTT(VOID) {
        _RTT = GetTickCount();
    }

    VOID UpdateRTT(VOID) {
        _RTT = GetTickCount() - _RTT;

        CServerInfo * pServerInfo = GetOriginServer();

        if (pServerInfo != NULL) {
            pServerInfo->UpdateRTT(_RTT);
        }
    }

    DWORD GetRTT(VOID) const {
        return _RTT;
    }

    VOID SetCodePage(UINT dwCodePage) {
        _CP = dwCodePage;
    }

    UINT GetCodePage(VOID) const {
        return _CP;
    }

    VOID SetAuthenticated();

    BOOL IsAuthenticated();

    //
    // diagnostic info
    //

    SOCKET GetSocket(VOID) {
        return (_Socket != NULL) ? _Socket->GetSocket() : INVALID_SOCKET;
    }

    DWORD GetSourcePort(VOID) {
        return (_Socket != NULL) ? _Socket->GetSourcePort() : 0;
    }

    DWORD GetDestPort(VOID) {
        return (_Socket != NULL) ? _Socket->GetPort() : 0;
    }

    BOOL FromKeepAlivePool(VOID) const {
        return _bKeepAliveConnection;
    }

    BOOL IsSecure(VOID) {
        return (_Socket != NULL) ? _Socket->IsSecure() : FALSE;
    }

    BOOL IsBlockedOnUserInput(VOID) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("HTTP_REQUEST_HANDLE_OBJECT::IsBlockedOnUserInput() = %B\n",
                    _dwUiBlocked
                    ));

        return (BOOL) _dwUiBlocked;
    }

    VOID BlockOnUserInput(DWORD_PTR dwBlockId) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("HTTP_REQUEST_HANDLE_OBJECT::BlockOnUserInput()\n"
                    ));

        INET_ASSERT(!_dwUiBlocked);
        _dwUiBlocked       = TRUE;
        _dwBlockId         = dwBlockId;
    }

    DWORD_PTR GetBlockId(VOID) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("HTTP_REQUEST_HANDLE_OBJECT::GetBlockId() = %u\n",
                    _dwBlockId
                    ));

        INET_ASSERT(_dwUiBlocked);
        return _dwBlockId;
    }


    VOID UnBlockOnUserInput(VOID) {

        DEBUG_PRINT(THRDINFO,
                    INFO,
                    ("HTTP_REQUEST_HANDLE_OBJECT::UnBlockedOnUserInput()\n"
                    ));

        INET_ASSERT(_dwUiBlocked);
        _dwUiBlocked        = FALSE;
    }
};

//
// prototypes
//

BOOL
IsCorrectUser(
    IN LPSTR lpszHeaderInfo,
    IN DWORD dwHeaderSize
    );
