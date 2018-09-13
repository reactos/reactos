/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetp.h

Abstract:

    Contains the Internet Gateway Service private functions proto type
    definitions.

Author:

    Madan Appiah  (madana)  11-Nov-1994

Environment:

    User Mode - Win32 - MIDL

Revision History:

--*/

#ifndef _INETP_
#define _INETP_

#if defined(__cplusplus)
extern "C" {
#endif

//
// manifests
//

//
// flags for close functions
//

#define CF_EXPEDITED_CLOSE  0x00000001

//
// define used to expire entries
//

#define ONE_HOUR_DELTA  (60 * 60 * (LONGLONG)10000000)

//
// define signature for LockRequestInfo structure
//
#define LOCK_REQUEST_SIGNATURE  0xAA55AA55

//
// types
//

typedef enum {
    TypeGenericHandle = 'HneG',
    TypeInternetHandle = 'tenI',
    TypeFtpConnectHandle = 'noCF',
    TypeFtpFindHandle = 'dnFF',
    TypeFtpFindHandleHtml = 'HnFF',
    TypeFtpFileHandle = 'liFF',
    TypeFtpFileHandleHtml = 'HlFF',
    TypeGopherConnectHandle = 'noCG',
    TypeGopherFindHandle = 'dnFG',
    TypeGopherFindHandleHtml = 'HnFG',
    TypeGopherFileHandle = 'liFG',
    TypeGopherFileHandleHtml = 'HlFG',
    TypeHttpConnectHandle = 'noCH',
    TypeHttpRequestHandle = 'qeRH',
    TypeFileRequestHandle = 'flRH',
    TypeWildHandle = 'dliW'
} HINTERNET_HANDLE_TYPE, *LPHINTERNET_HANDLE_TYPE;

typedef enum {
    HTML_STATE_INVALID,
    HTML_STATE_START,
    HTML_STATE_HEADER,
    HTML_STATE_WELCOME,
    HTML_STATE_DIR_HEADER,
    HTML_STATE_BODY,
    HTML_STATE_DIR_FOOTER,
    HTML_STATE_FOOTER,
#ifdef EXTENDED_ERROR_HTML
    HTML_STATE_END,
    HTML_STATE_ERROR_BODY
#else
    HTML_STATE_END
#endif
} HTML_STATE, *LPHTML_STATE;

typedef enum {
    READ_BUFFER_SIZE_INDEX,
    WRITE_BUFFER_SIZE_INDEX
} BUFFER_SIZE_INDEX;

typedef struct {

    DWORD   dwSignature;
    DWORD   dwSize;
    DWORD   dwCount;
    BOOL    fNoCacheLookup;
    BOOL    fNoDelete;
    HANDLE  hFile;
    LPSTR   UrlName;
    LPSTR   FileName;
    char    rgBuff[1];
}
LOCK_REQUEST_INFO, *LPLOCK_REQUEST_INFO;

//
// typedef virtual close function.
//

typedef BOOL ( *CLOSE_HANDLE_FUNC ) ( HINTERNET );
typedef BOOL ( *CONNECT_CLOSE_HANDLE_FUNC ) ( HINTERNET, DWORD );

//
// prototypes
//

BOOL
_InternetCloseHandle(
    IN HINTERNET hInternet
    );

DWORD
_InternetCloseHandleNoContext(
    IN HINTERNET hInternet
    );

//
// remote/RPC/object functions
//

DWORD
RIsHandleLocal(
    HINTERNET Handle,
    BOOL * IsLocalHandle,
    BOOL * IsAsyncHandle,
    HINTERNET_HANDLE_TYPE ExpectedHandleType
    );

DWORD
RGetHandleType(
    HINTERNET Handle,
    LPHINTERNET_HANDLE_TYPE HandleType
    );

DWORD
RSetHtmlHandleType(
    HINTERNET Handle
    );

DWORD
RSetHtmlState(
    HINTERNET Handle,
    HTML_STATE State
    );

DWORD
RGetHtmlState(
    HINTERNET Handle,
    LPHTML_STATE lpState
    );

DWORD
RSetUrl(
    HINTERNET Handle,
    LPSTR lpszUrl
    );

DWORD
RGetUrl(
    HINTERNET Handle,
    LPSTR* lpszUrl
    );

DWORD
RSetDirEntry(
    HINTERNET Handle,
    LPSTR lpszDirEntry
    );

DWORD
RGetDirEntry(
    HINTERNET Handle,
    LPSTR* lpszDirEntry
    );

DWORD
RSetParentHandle(
    HINTERNET hChild,
    HINTERNET hParent,
    BOOL DeleteWithChild
    );

DWORD
RGetParentHandle(
    HINTERNET hChild,
    LPHINTERNET lphParent
    );

DWORD
RGetContext(
    HINTERNET hInternet,
    DWORD_PTR *lpdwContext
    );

DWORD
RSetContext(
    HINTERNET hInternet,
    DWORD_PTR dwContext
    );

DWORD
RGetTimeout(
    HINTERNET hInternet,
    DWORD dwTimeoutOption,
    LPDWORD lpdwTimeoutValue
    );

DWORD
RSetTimeout(
    HINTERNET hInternet,
    DWORD dwTimeoutOption,
    DWORD dwTimeoutValue
    );

DWORD
RGetBufferSize(
    HINTERNET hInternet,
    DWORD dwBufferSizeOption,
    LPDWORD lpdwBufferSize
    );

DWORD
RSetBufferSize(
    HINTERNET hInternet,
    DWORD dwBufferSizeOption,
    DWORD dwBufferSize
    );

DWORD
RGetStatusCallback(
    IN HINTERNET Handle,
    OUT LPINTERNET_STATUS_CALLBACK lpStatusCallback
    );

DWORD
RExchangeStatusCallback(
    IN HINTERNET Handle,
    IN OUT LPINTERNET_STATUS_CALLBACK lpStatusCallback,
    IN BOOL fType
    );

DWORD
RAddAsyncRequest(
    IN HINTERNET Handle,
    BOOL fNoCallbackOK
    );

DWORD
RRemoveAsyncRequest(
    IN HINTERNET Handle
    );

DWORD
RMakeInternetConnectObjectHandle(
    HINTERNET ParentHandle,
    HINTERNET *ChildHandle,
    CONNECT_CLOSE_HANDLE_FUNC wCloseFunc,
    LPSTR lpszServerName,
    INTERNET_PORT nServerPort,
    LPSTR lpszUserName,
    LPSTR lpszPassword,
    DWORD ServiceType,
    DWORD dwFlags,
    DWORD_PTR dwContext
    );

HINTERNET
FindExistingConnectObject(
    IN HINTERNET hInternet,
    IN LPSTR lpHostName,
    IN INTERNET_PORT nPort,
    IN LPSTR lpszUserName,
    IN LPSTR lpszPassword,
    IN DWORD dwServiceType,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

INT
FlushExistingConnectObjects(
    IN HINTERNET hInternet
    );

DWORD
RMakeGfrFindObjectHandle(
    HINTERNET ParentHandle,
    HINTERNET *ChildHandle,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD_PTR dwContext
    );

DWORD
RMakeGfrFixedObjectHandle(
    HINTERNET ParentHandle,
    HINTERNET *ChildHandle,
    DWORD dwFixedType
    );

DWORD
RMakeGfrFileObjectHandle(
    HINTERNET ParentHandle,
    HINTERNET *ChildHandle,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD_PTR dwContext
    );

DWORD
RGetLocalHandle(
    HINTERNET Handle,
    HINTERNET *LocalHandle
    );

DWORD
RMakeHttpReqObjectHandle(
    HINTERNET ParentHandle,
    HINTERNET *ChildHandle,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD dwFlags,
    DWORD_PTR dwContext
    );

//
// FTP remote functions
//

DWORD
RMakeFtpFindObjectHandle(
    HINTERNET ParentHandle,
    HINTERNET *ChildHandle,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD_PTR dwContext
    );

DWORD
RMakeFtpFileObjectHandle(
    HINTERNET ParentHandle,
    HINTERNET *ChildHandle,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD_PTR dwContext
    );

//
// non-exported Internet subordinate functions
//

BOOL
FtpFindNextFileA(
    IN HINTERNET hFtpSession,
    OUT LPWIN32_FIND_DATA lpFindFileData
    );

BOOL
FtpReadFile(
    IN HINTERNET hFtpSession,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

BOOL
FtpWriteFile(
    IN HINTERNET hFtpSession,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    );

BOOL
FtpCloseFile(
    IN HINTERNET hFtpSession
    );

BOOL
GopherFindNextA(
    IN HINTERNET hGopherFind,
    OUT LPGOPHER_FIND_DATA lpFindFileData
    );

BOOL
GopherReadFile(
    IN HINTERNET hGopherFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

DWORD
HttpWriteData(
    IN HINTERNET hRequest,
    OUT LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten,
    IN DWORD dwSocketFlags
    );

DWORD
HttpReadData(
    IN HINTERNET hHttpRequest,
    OUT LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead,
    IN DWORD dwSocketFlags
    );

PUBLIC
DWORD
wHttpAddRequestHeaders(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );

DWORD
wFtpConnect(
    IN LPCSTR pszFtpSite,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR pszUsername,
    IN LPCSTR pszPassword,
    IN DWORD dwService,
    IN DWORD dwFlags,
    OUT LPHINTERNET lphInternet
    );

DWORD
wFtpMakeConnection(
    IN HINTERNET hFtpSession,
    IN LPCSTR pszUsername,
    IN LPCSTR pszPassword
    );

DWORD
wFtpDisconnect(
    IN HINTERNET hFtpSession,
    IN DWORD dwFlags
    );

DWORD
wFtpQueryDataAvailable(
    IN HINTERNET hFtpSession,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    );

DWORD
wGopherQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable
    );

DWORD
pHttpGetUrlInfo(
    IN HANDLE RequestHandle,
    IN LPBYTE Headers,
    IN DWORD HeadersLength,
    IN LPBYTE UrlBuf,
    IN OUT DWORD *UrlBufLen,
    IN BOOL ReloadFlagCheck
    );

DWORD
pFtpGetUrlString(
    IN LPSTR    lpszTargetName,
    IN LPSTR    lpszCWD,
    IN LPSTR    lpszObjectName,
    IN LPSTR    lpszExtension,
    IN DWORD    dwPort,
    OUT LPSTR   *lplpUrlName,
    OUT LPDWORD lpdwUrlLen
    );

DWORD
pFtpGetUrlInfo(
    IN HANDLE InternetConnectHandle,
    OUT LPSTR Url
    );

DWORD
pGopherGetUrlString(
    IN LPSTR   lpszTargetName,
    IN LPSTR   lpszCWD,
    IN LPSTR   lpszObjectName,
    IN LPSTR    lpszExtension,
    IN DWORD   dwPort,
    OUT LPSTR   *lplpUrlName,
    OUT LPDWORD lpdwUrlLen
    );

DWORD
pGfrGetUrlInfo(
    IN HANDLE InternetConnectHandle,
    OUT LPSTR Url
    );


DWORD
InbLocalEndCacheWrite(
    IN HINTERNET hFtpFile,
    IN LPSTR    lpszFileExtension,
    IN BOOL fNormal
    );

DWORD
InbGopherLocalEndCacheWrite(
    IN HINTERNET hGopherFile,
    IN LPSTR     lpszFileExtension,
    IN BOOL fNormal
    );

BOOL
GetCurrentSettingsVersion(
    LPDWORD lpdwVer
    );

BOOL
IncrementCurrentSettingsVersion(
    LPDWORD lpdwVer
    );


extern DWORD  GlobalSettingsVersion;
extern BOOL   GlobalSettingsLoaded;
extern const char   vszSyncMode[];
extern const char   vszInvalidFilenameChars[];


#if defined(__cplusplus)
}
#endif

#endif // _INETP_
 

