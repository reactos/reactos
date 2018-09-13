/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:

    wininet.h

Abstract:

    Contains manifests, macros, types and prototypes for Microsoft Windows
    Internet Extensions

--*/

#if !defined(_WININET_)
#define _WININET_

/*
 * Set up Structure Packing to be 4 bytes
 * for all wininet structures
 */

#pragma pack(push, wininet, 4)

#if defined(__cplusplus)
extern "C" {
#endif

#if !defined(_WINX32_)
#define INTERNETAPI DECLSPEC_IMPORT
#else
#define INTERNETAPI
#endif

//
// internet types
//

typedef LPVOID HINTERNET;
typedef HINTERNET * LPHINTERNET;

typedef WORD INTERNET_PORT;
typedef INTERNET_PORT * LPINTERNET_PORT;

//
// Internet APIs
//

//
// manifests
//

#define INTERNET_INVALID_PORT_NUMBER    0           // use the protocol-specific default

#define INTERNET_DEFAULT_FTP_PORT       21          // default for FTP servers
#define INTERNET_DEFAULT_GOPHER_PORT    70          //    "     "  gopher "
#define INTERNET_DEFAULT_HTTP_PORT      80          //    "     "  HTTP   "
#define INTERNET_DEFAULT_HTTPS_PORT     443         //    "     "  HTTPS  "
#define INTERNET_DEFAULT_SOCKS_PORT     1080        // default for SOCKS firewall servers.

#define MAX_CACHE_ENTRY_INFO_SIZE       4096

//
// maximum field lengths (arbitrary)
//

#define INTERNET_MAX_HOST_NAME_LENGTH   256
#define INTERNET_MAX_USER_NAME_LENGTH   128
#define INTERNET_MAX_PASSWORD_LENGTH    128
#define INTERNET_MAX_PORT_NUMBER_LENGTH 5           // INTERNET_PORT is unsigned short
#define INTERNET_MAX_PORT_NUMBER_VALUE  65535       // maximum unsigned short value
#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_PROTOCOL_NAME      "gopher"    // longest protocol name
#define INTERNET_MAX_URL_LENGTH         ((sizeof(INTERNET_MAX_PROTOCOL_NAME) - 1) \
                                        + sizeof("://") \
                                        + INTERNET_MAX_PATH_LENGTH)

//
// values returned by InternetQueryOption() with INTERNET_OPTION_KEEP_CONNECTION:
//

#define INTERNET_KEEP_ALIVE_UNKNOWN     ((DWORD)-1)
#define INTERNET_KEEP_ALIVE_ENABLED     1
#define INTERNET_KEEP_ALIVE_DISABLED    0

//
// flags returned by InternetQueryOption() with INTERNET_OPTION_REQUEST_FLAGS
//

#define INTERNET_REQFLAG_FROM_CACHE     0x00000001
#define INTERNET_REQFLAG_ASYNC          0x00000002

//
// flags common to open functions (not InternetOpen()):
//

#define INTERNET_FLAG_RELOAD            0x80000000  // retrieve the original item

//
// flags for InternetOpenUrl():
//

#define INTERNET_FLAG_RAW_DATA          0x40000000  // receive the item as raw data
#define INTERNET_FLAG_EXISTING_CONNECT  0x20000000  // do not create new connection object

//
// flags for InternetOpen():
//

#define INTERNET_FLAG_ASYNC             0x10000000  // this request is asynchronous (where supported)

//
// protocol-specific flags:
//

#define INTERNET_FLAG_PASSIVE           0x08000000  // used for FTP connections

//
// additional cache flags
//

#define INTERNET_FLAG_NO_CACHE_WRITE    0x04000000  // don't write this item to the cache
#define INTERNET_FLAG_DONT_CACHE        INTERNET_FLAG_NO_CACHE_WRITE

#define INTERNET_FLAG_MAKE_PERSISTENT   0x02000000  // make this item persistent in cache

#define INTERNET_FLAG_OFFLINE           0x01000000  // use offline semantics

//
// additional flags
//

#define INTERNET_FLAG_SECURE            0x00800000  // use PCT/SSL if applicable (HTTP)

#define INTERNET_FLAG_KEEP_CONNECTION   0x00400000  // use keep-alive semantics

#define INTERNET_FLAG_NO_AUTO_REDIRECT  0x00200000  // don't handle redirections automatically

#define INTERNET_FLAG_READ_PREFETCH     0x00100000  // do background read prefetch

#define INTERNET_FLAG_NO_COOKIES        0x00080000  // no automatic cookie handling

#define INTERNET_FLAG_NO_AUTH           0x00040000  // no automatic authentication handling

//
// Security Ignore Flags, Allow HttpOpenRequest to overide
//  Secure Channel (SSL/PCT) failures of the following types.
//

#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   0x00008000 // ex: https:// to http://

#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  0x00004000 // ex: http:// to https://

#define INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  0x00002000 // expired X509 Cert.

#define INTERNET_FLAG_IGNORE_CERT_CN_INVALID    0x00001000 // bad common name in X509 Cert.

//more caching flags
#define INTERNET_FLAG_MUST_CACHE_REQUEST        0x00000010 // fails if unable to cache request
#define INTERNET_FLAG_RESYNCHRONIZE             0x00000800 // asking wininet to update an item if it is newer
#define INTERNET_FLAG_HYPERLINK                 0x00000400 // asking wininet to
                                                           //do hyperlinking semantic which works right for scripts
#define INTERNET_FLAG_NO_UI                     0x00000200

//
// flags for FTP
//

#define INTERNET_FLAG_TRANSFER_ASCII    FTP_TRANSFER_TYPE_ASCII
#define INTERNET_FLAG_TRANSFER_BINARY   FTP_TRANSFER_TYPE_BINARY

//
// flags field masks
//

#define SECURITY_INTERNET_MASK  (INTERNET_FLAG_IGNORE_CERT_CN_INVALID    |  \
                                 INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   )

#define SECURITY_SET_MASK       SECURITY_INTERNET_MASK

#define INTERNET_FLAGS_MASK     (INTERNET_FLAG_RELOAD               \
                                | INTERNET_FLAG_RAW_DATA            \
                                | INTERNET_FLAG_EXISTING_CONNECT    \
                                | INTERNET_FLAG_ASYNC               \
                                | INTERNET_FLAG_PASSIVE             \
                                | INTERNET_FLAG_NO_CACHE_WRITE      \
                                | INTERNET_FLAG_MAKE_PERSISTENT     \
                                | INTERNET_FLAG_OFFLINE             \
                                | INTERNET_FLAG_SECURE              \
                                | INTERNET_FLAG_KEEP_CONNECTION     \
                                | INTERNET_FLAG_NO_AUTO_REDIRECT    \
                                | INTERNET_FLAG_READ_PREFETCH       \
                                | INTERNET_FLAG_NO_COOKIES          \
                                | INTERNET_FLAG_NO_AUTH             \
                                | SECURITY_INTERNET_MASK            \
                                | INTERNET_FLAG_TRANSFER_ASCII      \
                                | INTERNET_FLAG_TRANSFER_BINARY     \
                                | INTERNET_FLAG_RESYNCHRONIZE       \
                                | INTERNET_FLAG_MUST_CACHE_REQUEST  \
                                | INTERNET_FLAG_HYPERLINK           \
                                | INTERNET_FLAG_NO_UI               \
                                )

#define INTERNET_OPTIONS_MASK   (~INTERNET_FLAGS_MASK)

//
// INTERNET_NO_CALLBACK - if this value is presented as the dwContext parameter
// then no call-backs will be made for that API
//

#define INTERNET_NO_CALLBACK            0

//
// structures/types
//

//
// INTERNET_SCHEME - enumerated URL scheme type
//

typedef enum {
    INTERNET_SCHEME_PARTIAL = -2,
    INTERNET_SCHEME_UNKNOWN = -1,
    INTERNET_SCHEME_DEFAULT = 0,
    INTERNET_SCHEME_FTP,
    INTERNET_SCHEME_GOPHER,
    INTERNET_SCHEME_HTTP,
    INTERNET_SCHEME_HTTPS,
    INTERNET_SCHEME_FILE,
    INTERNET_SCHEME_NEWS,
    INTERNET_SCHEME_MAILTO,
    INTERNET_SCHEME_SOCKS,
    INTERNET_SCHEME_FIRST = INTERNET_SCHEME_FTP,
    INTERNET_SCHEME_LAST = INTERNET_SCHEME_SOCKS
} INTERNET_SCHEME, * LPINTERNET_SCHEME;

//
// INTERNET_ASYNC_RESULT - this structure is returned to the application via
// the callback with INTERNET_STATUS_REQUEST_COMPLETE. It is not sufficient to
// just return the result of the async operation. If the API failed then the
// app cannot call GetLastError() because the thread context will be incorrect.
// Both the value returned by the async API and any resultant error code are
// made available. The app need not check dwError if dwResult indicates that
// the API succeeded (in this case dwError will be ERROR_SUCCESS)
//

typedef struct {

    //
    // dwResult - the HINTERNET, DWORD or BOOL return code from an async API
    //

    DWORD dwResult;

    //
    // dwError - the error code if the API failed
    //

    DWORD dwError;
} INTERNET_ASYNC_RESULT, * LPINTERNET_ASYNC_RESULT;

//
// INTERNET_PREFETCH_STATUS -
//

typedef struct {

    //
    // dwStatus - status of download. See INTERNET_PREFETCH_ flags
    //

    DWORD dwStatus;

    //
    // dwSize - size of file downloaded so far
    //

    DWORD dwSize;
} INTERNET_PREFETCH_STATUS, * LPINTERNET_PREFETCH_STATUS;

//
// INTERNET_PREFETCH_STATUS - dwStatus values
//

#define INTERNET_PREFETCH_PROGRESS  0
#define INTERNET_PREFETCH_COMPLETE  1
#define INTERNET_PREFETCH_ABORTED   2

//
// INTERNET_PROXY_INFO - structure supplied with INTERNET_OPTION_PROXY to get/
// set proxy information on a InternetOpen() handle
//

typedef struct {

    //
    // dwAccessType - INTERNET_OPEN_TYPE_DIRECT, INTERNET_OPEN_TYPE_PROXY, or
    // INTERNET_OPEN_TYPE_PRECONFIG (set only)
    //

    DWORD dwAccessType;

    //
    // lpszProxy - proxy server list
    //

    LPCTSTR lpszProxy;

    //
    // lpszProxyBypass - proxy bypass list
    //

    LPCTSTR lpszProxyBypass;
} INTERNET_PROXY_INFO, * LPINTERNET_PROXY_INFO;

//
// INTERNET_VERSION_INFO - version information returned via
// InternetQueryOption(..., INTERNET_OPTION_VERSION, ...)
//

typedef struct {
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
} INTERNET_VERSION_INFO, * LPINTERNET_VERSION_INFO;

//
// URL_COMPONENTS - the constituent parts of an URL. Used in InternetCrackUrl()
// and InternetCreateUrl()
//
// For InternetCrackUrl(), if a pointer field and its corresponding length field
// are both 0 then that component is not returned; If the pointer field is NULL
// but the length field is not zero, then both the pointer and length fields are
// returned; if both pointer and corresponding length fields are non-zero then
// the pointer field points to a buffer where the component is copied. The
// component may be un-escaped, depending on dwFlags
//
// For InternetCreateUrl(), the pointer fields should be NULL if the component
// is not required. If the corresponding length field is zero then the pointer
// field is the address of a zero-terminated string. If the length field is not
// zero then it is the string length of the corresponding pointer field
//

typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPSTR   lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    INTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPSTR   lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    INTERNET_PORT nPort;        // converted port number
    LPSTR   lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPSTR   lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPSTR   lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPSTR   lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} URL_COMPONENTSA, * LPURL_COMPONENTSA;
typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPWSTR  lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    INTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPWSTR  lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    INTERNET_PORT nPort;        // converted port number
    LPWSTR  lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPWSTR  lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPWSTR  lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPWSTR  lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} URL_COMPONENTSW, * LPURL_COMPONENTSW;
#ifdef UNICODE
typedef URL_COMPONENTSW URL_COMPONENTS;
typedef LPURL_COMPONENTSW LPURL_COMPONENTS;
#else
typedef URL_COMPONENTSA URL_COMPONENTS;
typedef LPURL_COMPONENTSA LPURL_COMPONENTS;
#endif // UNICODE

//
// INTERNET_CERTIFICATE_INFO lpBuffer - contains the certificate returned from
// the server
//

typedef struct {

    //
    // ftExpiry - date the certificate expires.
    //

    FILETIME ftExpiry;

    //
    // ftStart - date the certificate becomes valid.
    //

    FILETIME ftStart;

    //
    // lpszSubjectInfo - the name of organization, site, and server
    //   the cert. was issued for.
    //

    LPTSTR lpszSubjectInfo;

    //
    // lpszIssuerInfo - the name of orgainzation, site, and server
    //   the cert was issues by.
    //

    LPTSTR lpszIssuerInfo;

    //
    // lpszProtocolName - the name of the protocol used to provide the secure
    //   connection.
    //

    LPTSTR lpszProtocolName;

    //
    // lpszSignatureAlgName - the name of the algorithm used for signing
    //  the certificate.
    //

    LPTSTR lpszSignatureAlgName;

    //
    // lpszEncryptionAlgName - the name of the algorithm used for
    //  doing encryption over the secure channel (SSL/PCT) connection.
    //

    LPTSTR lpszEncryptionAlgName;

    //
    // dwKeySize - size of the key.
    //

    DWORD dwKeySize;

} INTERNET_CERTIFICATE_INFO, * LPINTERNET_CERTIFICATE_INFO;

//
// prototypes
//

INTERNETAPI
BOOL
WINAPI
InternetTimeFromSystemTime(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format
    OUT LPSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    );

//
// constants for InternetTimeFromSystemTime
//

#define INTERNET_RFC1123_FORMAT     0
#define INTERNET_RFC1123_BUFSIZE   30

INTERNETAPI
BOOL
WINAPI
InternetTimeToSystemTime(
    IN  LPCSTR lpszTime,         // NULL terminated string
    OUT SYSTEMTIME *pst,         // output in GMT time
    IN  DWORD dwReserved
    );

INTERNETAPI
BOOL
WINAPI
InternetDebugGetLocalTime(
    OUT SYSTEMTIME * pstLocalTime,
    OUT DWORD      * pdwReserved
);

INTERNETAPI
BOOL
WINAPI
InternetCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSA lpUrlComponents
    );
INTERNETAPI
BOOL
WINAPI
InternetCrackUrlW(
    IN LPCWSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW lpUrlComponents
    );
#ifdef UNICODE
#define InternetCrackUrl  InternetCrackUrlW
#else
#define InternetCrackUrl  InternetCrackUrlA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetCreateUrlA(
    IN LPURL_COMPONENTSA lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPSTR lpszUrl,
    IN OUT LPDWORD lpdwUrlLength
    );
INTERNETAPI
BOOL
WINAPI
InternetCreateUrlW(
    IN LPURL_COMPONENTSW lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPWSTR lpszUrl,
    IN OUT LPDWORD lpdwUrlLength
    );
#ifdef UNICODE
#define InternetCreateUrl  InternetCreateUrlW
#else
#define InternetCreateUrl  InternetCreateUrlA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetCanonicalizeUrlA(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );
INTERNETAPI
BOOL
WINAPI
InternetCanonicalizeUrlW(
    IN LPCWSTR lpszUrl,
    OUT LPWSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );
#ifdef UNICODE
#define InternetCanonicalizeUrl  InternetCanonicalizeUrlW
#else
#define InternetCanonicalizeUrl  InternetCanonicalizeUrlA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetCombineUrlA(
    IN LPCSTR lpszBaseUrl,
    IN LPCSTR lpszRelativeUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );
INTERNETAPI
BOOL
WINAPI
InternetCombineUrlW(
    IN LPCWSTR lpszBaseUrl,
    IN LPCWSTR lpszRelativeUrl,
    OUT LPWSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );
#ifdef UNICODE
#define InternetCombineUrl  InternetCombineUrlW
#else
#define InternetCombineUrl  InternetCombineUrlA
#endif // !UNICODE

//
// flags for InternetCrackUrl() and InternetCreateUrl()
//

#define ICU_ESCAPE      0x80000000  // (un)escape URL characters
#define ICU_USERNAME    0x40000000  // use internal username & password

//
// flags for InternetCanonicalizeUrl() and InternetCombineUrl()
//

#define ICU_NO_ENCODE   0x20000000  // Don't convert unsafe characters to escape sequence
#define ICU_DECODE      0x10000000  // Convert %XX escape sequences to characters
#define ICU_NO_META     0x08000000  // Don't convert .. etc. meta path sequences
#define ICU_ENCODE_SPACES_ONLY 0x04000000  // Encode spaces only
#define ICU_BROWSER_MODE 0x02000000 // Special encode/decode rules for browser

INTERNETAPI
HINTERNET
WINAPI
InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );
INTERNETAPI
HINTERNET
WINAPI
InternetOpenW(
    IN LPCWSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCWSTR lpszProxy OPTIONAL,
    IN LPCWSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );
#ifdef UNICODE
#define InternetOpen  InternetOpenW
#else
#define InternetOpen  InternetOpenA
#endif // !UNICODE

//
// access types for InternetOpen()
//

#define INTERNET_OPEN_TYPE_PRECONFIG    0   // use registry configuration
#define INTERNET_OPEN_TYPE_DIRECT       1   // direct to net
#define INTERNET_OPEN_TYPE_PROXY        3   // via named proxy

#define PRE_CONFIG_INTERNET_ACCESS  INTERNET_OPEN_TYPE_PRECONFIG
#define LOCAL_INTERNET_ACCESS       INTERNET_OPEN_TYPE_DIRECT
#define GATEWAY_INTERNET_ACCESS     2   // Internet via gateway
#define CERN_PROXY_INTERNET_ACCESS  INTERNET_OPEN_TYPE_PROXY

INTERNETAPI
BOOL
WINAPI
InternetCloseHandle(
    IN HINTERNET hInternet
    );

INTERNETAPI
HINTERNET
WINAPI
InternetConnectA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszUserName OPTIONAL,
    IN LPCSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
HINTERNET
WINAPI
InternetConnectW(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR lpszUserName OPTIONAL,
    IN LPCWSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define InternetConnect  InternetConnectW
#else
#define InternetConnect  InternetConnectA
#endif // !UNICODE

//
// service types for InternetConnect()
//

#define INTERNET_SERVICE_FTP    1
#define INTERNET_SERVICE_GOPHER 2
#define INTERNET_SERVICE_HTTP   3

INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlW(
    IN HINTERNET hInternet,
    IN LPCWSTR lpszUrl,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define InternetOpenUrl  InternetOpenUrlW
#else
#define InternetOpenUrl  InternetOpenUrlA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

INTERNETAPI
DWORD
WINAPI
InternetSetFilePointer(
    IN HINTERNET hFile,
    IN LONG  lDistanceToMove,
    IN PVOID pReserved,
    IN DWORD dwMoveMethod,
    IN DWORD dwContext
    );

INTERNETAPI
BOOL
WINAPI
InternetWriteFile(
    IN HINTERNET hFile,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    );

INTERNETAPI
BOOL
WINAPI
InternetQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );

INTERNETAPI
BOOL
WINAPI
InternetFindNextFileA(
    IN HINTERNET hFind,
    OUT LPVOID lpvFindData
    );
INTERNETAPI
BOOL
WINAPI
InternetFindNextFileW(
    IN HINTERNET hFind,
    OUT LPVOID lpvFindData
    );
#ifdef UNICODE
#define InternetFindNextFile  InternetFindNextFileW
#else
#define InternetFindNextFile  InternetFindNextFileA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
INTERNETAPI
BOOL
WINAPI
InternetQueryOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
#ifdef UNICODE
#define InternetQueryOption  InternetQueryOptionW
#else
#define InternetQueryOption  InternetQueryOptionA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );
INTERNETAPI
BOOL
WINAPI
InternetSetOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );
#ifdef UNICODE
#define InternetSetOption  InternetSetOptionW
#else
#define InternetSetOption  InternetSetOptionA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetSetOptionExA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags
    );
INTERNETAPI
BOOL
WINAPI
InternetSetOptionExW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags
    );
#ifdef UNICODE
#define InternetSetOptionEx  InternetSetOptionExW
#else
#define InternetSetOptionEx  InternetSetOptionExA
#endif // !UNICODE

//
// flags for InternetSetOptionEx()
//

#define ISO_GLOBAL      0x00000001  // modify option globally
#define ISO_REGISTRY    0x00000002  // write option to registry (where applicable)

#define ISO_VALID_FLAGS (ISO_GLOBAL | ISO_REGISTRY)

//
// options manifests for Internet{Query|Set}Option
//

#define INTERNET_OPTION_CALLBACK                1
#define INTERNET_OPTION_CONNECT_TIMEOUT         2
#define INTERNET_OPTION_CONNECT_RETRIES         3
#define INTERNET_OPTION_CONNECT_BACKOFF         4
#define INTERNET_OPTION_SEND_TIMEOUT            5
#define INTERNET_OPTION_CONTROL_SEND_TIMEOUT    INTERNET_OPTION_SEND_TIMEOUT
#define INTERNET_OPTION_RECEIVE_TIMEOUT         6
#define INTERNET_OPTION_CONTROL_RECEIVE_TIMEOUT INTERNET_OPTION_RECEIVE_TIMEOUT
#define INTERNET_OPTION_DATA_SEND_TIMEOUT       7
#define INTERNET_OPTION_DATA_RECEIVE_TIMEOUT    8
#define INTERNET_OPTION_HANDLE_TYPE             9
#define INTERNET_OPTION_CONTEXT_VALUE           10
#define INTERNET_OPTION_LISTEN_TIMEOUT          11
#define INTERNET_OPTION_READ_BUFFER_SIZE        12
#define INTERNET_OPTION_WRITE_BUFFER_SIZE       13

#define INTERNET_OPTION_ASYNC_ID                15
#define INTERNET_OPTION_ASYNC_PRIORITY          16

#define INTERNET_OPTION_PARENT_HANDLE           21
#define INTERNET_OPTION_KEEP_CONNECTION         22
#define INTERNET_OPTION_REQUEST_FLAGS           23
#define INTERNET_OPTION_EXTENDED_ERROR          24

#define INTERNET_OPTION_OFFLINE_MODE            26
#define INTERNET_OPTION_CACHE_STREAM_HANDLE     27
#define INTERNET_OPTION_USERNAME                28
#define INTERNET_OPTION_PASSWORD                29
#define INTERNET_OPTION_ASYNC                   30
#define INTERNET_OPTION_SECURITY_FLAGS          31
#define INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT    32
#define INTERNET_OPTION_DATAFILE_NAME           33
#define INTERNET_OPTION_URL                     34
#define INTERNET_OPTION_SECURITY_CERTIFICATE    35
#define INTERNET_OPTION_SECURITY_KEY_BITNESS    36
#define INTERNET_OPTION_REFRESH                 37
#define INTERNET_OPTION_PROXY                   38
#define INTERNET_OPTION_SETTINGS_CHANGED        39
#define INTERNET_OPTION_VERSION                 40
#define INTERNET_OPTION_USER_AGENT              41

#define INTERNET_FIRST_OPTION                   INTERNET_OPTION_CALLBACK
#define INTERNET_LAST_OPTION                    INTERNET_OPTION_USER_AGENT

//
// values for INTERNET_OPTION_PRIORITY
//

#define INTERNET_PRIORITY_FOREGROUND            1000

//
// handle types
//

#define INTERNET_HANDLE_TYPE_INTERNET           1
#define INTERNET_HANDLE_TYPE_CONNECT_FTP        2
#define INTERNET_HANDLE_TYPE_CONNECT_GOPHER     3
#define INTERNET_HANDLE_TYPE_CONNECT_HTTP       4
#define INTERNET_HANDLE_TYPE_FTP_FIND           5
#define INTERNET_HANDLE_TYPE_FTP_FIND_HTML      6
#define INTERNET_HANDLE_TYPE_FTP_FILE           7
#define INTERNET_HANDLE_TYPE_FTP_FILE_HTML      8
#define INTERNET_HANDLE_TYPE_GOPHER_FIND        9
#define INTERNET_HANDLE_TYPE_GOPHER_FIND_HTML   10
#define INTERNET_HANDLE_TYPE_GOPHER_FILE        11
#define INTERNET_HANDLE_TYPE_GOPHER_FILE_HTML   12
#define INTERNET_HANDLE_TYPE_HTTP_REQUEST       13

//
// values for INTERNET_OPTION_SECURITY_FLAGS
//

#define SECURITY_FLAG_SECURE                    0x00000001 // can query only
#define SECURITY_FLAG_SSL                       0x00000002
#define SECURITY_FLAG_SSL3                      0x00000004
#define SECURITY_FLAG_PCT                       0x00000008
#define SECURITY_FLAG_PCT4                      0x00000010
#define SECURITY_FLAG_IETFSSL4                  0x00000020

#define SECURITY_FLAG_40BIT                     0x10000000
#define SECURITY_FLAG_128BIT                    0x20000000
#define SECURITY_FLAG_56BIT                     0x40000000
#define SECURITY_FLAG_UNKNOWNBIT                0x80000000
#define SECURITY_FLAG_NORMALBITNESS             SECURITY_FLAG_40BIT

#define SECURITY_FLAG_IGNORE_CERT_CN_INVALID    INTERNET_FLAG_IGNORE_CERT_CN_INVALID
#define SECURITY_FLAG_IGNORE_CERT_DATE_INVALID  INTERNET_FLAG_IGNORE_CERT_DATE_INVALID
#define SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS  INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
#define SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP   INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP

INTERNETAPI
BOOL
WINAPI
InternetGetLastResponseInfoA(
    OUT LPDWORD lpdwError,
    OUT LPSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
INTERNETAPI
BOOL
WINAPI
InternetGetLastResponseInfoW(
    OUT LPDWORD lpdwError,
    OUT LPWSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
#ifdef UNICODE
#define InternetGetLastResponseInfo  InternetGetLastResponseInfoW
#else
#define InternetGetLastResponseInfo  InternetGetLastResponseInfoA
#endif // !UNICODE

//
// callback function for InternetSetStatusCallback
//

typedef
VOID
(CALLBACK * INTERNET_STATUS_CALLBACK)(
    IN HINTERNET hInternet,
    IN DWORD dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
    );

typedef INTERNET_STATUS_CALLBACK * LPINTERNET_STATUS_CALLBACK;

INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallback(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    );

//
// status manifests for Internet status callback
//

#define INTERNET_STATUS_RESOLVING_NAME          10
#define INTERNET_STATUS_NAME_RESOLVED           11
#define INTERNET_STATUS_CONNECTING_TO_SERVER    20
#define INTERNET_STATUS_CONNECTED_TO_SERVER     21
#define INTERNET_STATUS_SENDING_REQUEST         30
#define INTERNET_STATUS_REQUEST_SENT            31
#define INTERNET_STATUS_RECEIVING_RESPONSE      40
#define INTERNET_STATUS_RESPONSE_RECEIVED       41
#define INTERNET_STATUS_CTL_RESPONSE_RECEIVED   42
#define INTERNET_STATUS_PREFETCH                43
#define INTERNET_STATUS_CLOSING_CONNECTION      50
#define INTERNET_STATUS_CONNECTION_CLOSED       51
#define INTERNET_STATUS_HANDLE_CREATED          60
#define INTERNET_STATUS_HANDLE_CLOSING          70
#define INTERNET_STATUS_REQUEST_COMPLETE        100
#define INTERNET_STATUS_REDIRECT                110

//
// if the following value is returned by InternetSetStatusCallback, then
// probably an invalid (non-code) address was supplied for the callback
//

#define INTERNET_INVALID_STATUS_CALLBACK        ((INTERNET_STATUS_CALLBACK)(-1L))

//
// FTP
//

//
// manifests
//

#define FTP_TRANSFER_TYPE_UNKNOWN   0x00000000
#define FTP_TRANSFER_TYPE_ASCII     0x00000001
#define FTP_TRANSFER_TYPE_BINARY    0x00000002

#define FTP_TRANSFER_TYPE_MASK      (FTP_TRANSFER_TYPE_ASCII | FTP_TRANSFER_TYPE_BINARY)

//
// prototypes
//

INTERNETAPI
HINTERNET
WINAPI
FtpFindFirstFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATA lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
HINTERNET
WINAPI
FtpFindFirstFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATA lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define FtpFindFirstFile  FtpFindFirstFileW
#else
#define FtpFindFirstFile  FtpFindFirstFileA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpGetFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszRemoteFile,
    IN LPCSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
BOOL
WINAPI
FtpGetFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszRemoteFile,
    IN LPCWSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define FtpGetFile  FtpGetFileW
#else
#define FtpGetFile  FtpGetFileA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpPutFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszLocalFile,
    IN LPCSTR lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
BOOL
WINAPI
FtpPutFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszLocalFile,
    IN LPCWSTR lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define FtpPutFile  FtpPutFileW
#else
#define FtpPutFile  FtpPutFileA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpDeleteFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszFileName
    );
INTERNETAPI
BOOL
WINAPI
FtpDeleteFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszFileName
    );
#ifdef UNICODE
#define FtpDeleteFile  FtpDeleteFileW
#else
#define FtpDeleteFile  FtpDeleteFileA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpRenameFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszExisting,
    IN LPCSTR lpszNew
    );
INTERNETAPI
BOOL
WINAPI
FtpRenameFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszExisting,
    IN LPCWSTR lpszNew
    );
#ifdef UNICODE
#define FtpRenameFile  FtpRenameFileW
#else
#define FtpRenameFile  FtpRenameFileA
#endif // !UNICODE

INTERNETAPI
HINTERNET
WINAPI
FtpOpenFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
HINTERNET
WINAPI
FtpOpenFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define FtpOpenFile  FtpOpenFileW
#else
#define FtpOpenFile  FtpOpenFileA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpCreateDirectoryA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszDirectory
    );
INTERNETAPI
BOOL
WINAPI
FtpCreateDirectoryW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszDirectory
    );
#ifdef UNICODE
#define FtpCreateDirectory  FtpCreateDirectoryW
#else
#define FtpCreateDirectory  FtpCreateDirectoryA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpRemoveDirectoryA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszDirectory
    );
INTERNETAPI
BOOL
WINAPI
FtpRemoveDirectoryW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszDirectory
    );
#ifdef UNICODE
#define FtpRemoveDirectory  FtpRemoveDirectoryW
#else
#define FtpRemoveDirectory  FtpRemoveDirectoryA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpSetCurrentDirectoryA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszDirectory
    );
INTERNETAPI
BOOL
WINAPI
FtpSetCurrentDirectoryW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszDirectory
    );
#ifdef UNICODE
#define FtpSetCurrentDirectory  FtpSetCurrentDirectoryW
#else
#define FtpSetCurrentDirectory  FtpSetCurrentDirectoryA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpGetCurrentDirectoryA(
    IN HINTERNET hConnect,
    OUT LPSTR lpszCurrentDirectory,
    IN OUT LPDWORD lpdwCurrentDirectory
    );
INTERNETAPI
BOOL
WINAPI
FtpGetCurrentDirectoryW(
    IN HINTERNET hConnect,
    OUT LPWSTR lpszCurrentDirectory,
    IN OUT LPDWORD lpdwCurrentDirectory
    );
#ifdef UNICODE
#define FtpGetCurrentDirectory  FtpGetCurrentDirectoryW
#else
#define FtpGetCurrentDirectory  FtpGetCurrentDirectoryA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
FtpCommandA(
    IN HINTERNET hConnect,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCSTR lpszCommand,
    IN DWORD dwContext
    );
INTERNETAPI
BOOL
WINAPI
FtpCommandW(
    IN HINTERNET hConnect,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCWSTR lpszCommand,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define FtpCommand  FtpCommandW
#else
#define FtpCommand  FtpCommandA
#endif // !UNICODE

//
// Gopher
//

//
// manifests
//

//
// string field lengths (in characters, not bytes)
//

#define MAX_GOPHER_DISPLAY_TEXT     128
#define MAX_GOPHER_SELECTOR_TEXT    256
#define MAX_GOPHER_HOST_NAME        INTERNET_MAX_HOST_NAME_LENGTH
#define MAX_GOPHER_LOCATOR_LENGTH   (1                                  \
                                    + MAX_GOPHER_DISPLAY_TEXT           \
                                    + 1                                 \
                                    + MAX_GOPHER_SELECTOR_TEXT          \
                                    + 1                                 \
                                    + MAX_GOPHER_HOST_NAME              \
                                    + 1                                 \
                                    + INTERNET_MAX_PORT_NUMBER_LENGTH   \
                                    + 1                                 \
                                    + 1                                 \
                                    + 2                                 \
                                    )

//
// structures/types
//

//
// GOPHER_FIND_DATA - returns the results of a GopherFindFirstFile()/
// InternetFindNextFile() request
//

typedef struct {
    CHAR   DisplayString[MAX_GOPHER_DISPLAY_TEXT + 1];
    DWORD GopherType;   // GOPHER_TYPE_, if known
    DWORD SizeLow;
    DWORD SizeHigh;
    FILETIME LastModificationTime;
    CHAR   Locator[MAX_GOPHER_LOCATOR_LENGTH + 1];
} GOPHER_FIND_DATAA, * LPGOPHER_FIND_DATAA;
typedef struct {
    WCHAR  DisplayString[MAX_GOPHER_DISPLAY_TEXT + 1];
    DWORD GopherType;   // GOPHER_TYPE_, if known
    DWORD SizeLow;
    DWORD SizeHigh;
    FILETIME LastModificationTime;
    WCHAR  Locator[MAX_GOPHER_LOCATOR_LENGTH + 1];
} GOPHER_FIND_DATAW, * LPGOPHER_FIND_DATAW;
#ifdef UNICODE
typedef GOPHER_FIND_DATAW GOPHER_FIND_DATA;
typedef LPGOPHER_FIND_DATAW LPGOPHER_FIND_DATA;
#else
typedef GOPHER_FIND_DATAA GOPHER_FIND_DATA;
typedef LPGOPHER_FIND_DATAA LPGOPHER_FIND_DATA;
#endif // UNICODE

//
// manifests for GopherType
//

#define GOPHER_TYPE_TEXT_FILE       0x00000001
#define GOPHER_TYPE_DIRECTORY       0x00000002
#define GOPHER_TYPE_CSO             0x00000004
#define GOPHER_TYPE_ERROR           0x00000008
#define GOPHER_TYPE_MAC_BINHEX      0x00000010
#define GOPHER_TYPE_DOS_ARCHIVE     0x00000020
#define GOPHER_TYPE_UNIX_UUENCODED  0x00000040
#define GOPHER_TYPE_INDEX_SERVER    0x00000080
#define GOPHER_TYPE_TELNET          0x00000100
#define GOPHER_TYPE_BINARY          0x00000200
#define GOPHER_TYPE_REDUNDANT       0x00000400
#define GOPHER_TYPE_TN3270          0x00000800
#define GOPHER_TYPE_GIF             0x00001000
#define GOPHER_TYPE_IMAGE           0x00002000
#define GOPHER_TYPE_BITMAP          0x00004000
#define GOPHER_TYPE_MOVIE           0x00008000
#define GOPHER_TYPE_SOUND           0x00010000
#define GOPHER_TYPE_HTML            0x00020000
#define GOPHER_TYPE_PDF             0x00040000
#define GOPHER_TYPE_CALENDAR        0x00080000
#define GOPHER_TYPE_INLINE          0x00100000
#define GOPHER_TYPE_UNKNOWN         0x20000000
#define GOPHER_TYPE_ASK             0x40000000
#define GOPHER_TYPE_GOPHER_PLUS     0x80000000

//
// gopher type macros
//

#define IS_GOPHER_FILE(type)            (BOOL)(((type) & GOPHER_TYPE_FILE_MASK) ? TRUE : FALSE)
#define IS_GOPHER_DIRECTORY(type)       (BOOL)(((type) & GOPHER_TYPE_DIRECTORY) ? TRUE : FALSE)
#define IS_GOPHER_PHONE_SERVER(type)    (BOOL)(((type) & GOPHER_TYPE_CSO) ? TRUE : FALSE)
#define IS_GOPHER_ERROR(type)           (BOOL)(((type) & GOPHER_TYPE_ERROR) ? TRUE : FALSE)
#define IS_GOPHER_INDEX_SERVER(type)    (BOOL)(((type) & GOPHER_TYPE_INDEX_SERVER) ? TRUE : FALSE)
#define IS_GOPHER_TELNET_SESSION(type)  (BOOL)(((type) & GOPHER_TYPE_TELNET) ? TRUE : FALSE)
#define IS_GOPHER_BACKUP_SERVER(type)   (BOOL)(((type) & GOPHER_TYPE_REDUNDANT) ? TRUE : FALSE)
#define IS_GOPHER_TN3270_SESSION(type)  (BOOL)(((type) & GOPHER_TYPE_TN3270) ? TRUE : FALSE)
#define IS_GOPHER_ASK(type)             (BOOL)(((type) & GOPHER_TYPE_ASK) ? TRUE : FALSE)
#define IS_GOPHER_PLUS(type)            (BOOL)(((type) & GOPHER_TYPE_GOPHER_PLUS) ? TRUE : FALSE)

#define IS_GOPHER_TYPE_KNOWN(type)      (BOOL)(((type) & GOPHER_TYPE_UNKNOWN) ? FALSE : TRUE)

//
// GOPHER_TYPE_FILE_MASK - use this to determine if a locator identifies a
// (known) file type
//

#define GOPHER_TYPE_FILE_MASK       (GOPHER_TYPE_TEXT_FILE          \
                                    | GOPHER_TYPE_MAC_BINHEX        \
                                    | GOPHER_TYPE_DOS_ARCHIVE       \
                                    | GOPHER_TYPE_UNIX_UUENCODED    \
                                    | GOPHER_TYPE_BINARY            \
                                    | GOPHER_TYPE_GIF               \
                                    | GOPHER_TYPE_IMAGE             \
                                    | GOPHER_TYPE_BITMAP            \
                                    | GOPHER_TYPE_MOVIE             \
                                    | GOPHER_TYPE_SOUND             \
                                    | GOPHER_TYPE_HTML              \
                                    | GOPHER_TYPE_PDF               \
                                    | GOPHER_TYPE_CALENDAR          \
                                    | GOPHER_TYPE_INLINE            \
                                    )

//
// structured gopher attributes (as defined in gopher+ protocol document)
//

typedef struct {
    LPCTSTR Comment;
    LPCTSTR EmailAddress;
} GOPHER_ADMIN_ATTRIBUTE_TYPE, * LPGOPHER_ADMIN_ATTRIBUTE_TYPE;

typedef struct {
    FILETIME DateAndTime;
} GOPHER_MOD_DATE_ATTRIBUTE_TYPE, * LPGOPHER_MOD_DATE_ATTRIBUTE_TYPE;

typedef struct {
    DWORD Ttl;
} GOPHER_TTL_ATTRIBUTE_TYPE, * LPGOPHER_TTL_ATTRIBUTE_TYPE;

typedef struct {
    INT Score;
} GOPHER_SCORE_ATTRIBUTE_TYPE, * LPGOPHER_SCORE_ATTRIBUTE_TYPE;

typedef struct {
    INT LowerBound;
    INT UpperBound;
} GOPHER_SCORE_RANGE_ATTRIBUTE_TYPE, * LPGOPHER_SCORE_RANGE_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR Site;
} GOPHER_SITE_ATTRIBUTE_TYPE, * LPGOPHER_SITE_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR Organization;
} GOPHER_ORGANIZATION_ATTRIBUTE_TYPE, * LPGOPHER_ORGANIZATION_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR Location;
} GOPHER_LOCATION_ATTRIBUTE_TYPE, * LPGOPHER_LOCATION_ATTRIBUTE_TYPE;

typedef struct {
    INT DegreesNorth;
    INT MinutesNorth;
    INT SecondsNorth;
    INT DegreesEast;
    INT MinutesEast;
    INT SecondsEast;
} GOPHER_GEOGRAPHICAL_LOCATION_ATTRIBUTE_TYPE, * LPGOPHER_GEOGRAPHICAL_LOCATION_ATTRIBUTE_TYPE;

typedef struct {
    INT Zone;
} GOPHER_TIMEZONE_ATTRIBUTE_TYPE, * LPGOPHER_TIMEZONE_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR Provider;
} GOPHER_PROVIDER_ATTRIBUTE_TYPE, * LPGOPHER_PROVIDER_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR Version;
} GOPHER_VERSION_ATTRIBUTE_TYPE, * LPGOPHER_VERSION_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR ShortAbstract;
    LPCTSTR AbstractFile;
} GOPHER_ABSTRACT_ATTRIBUTE_TYPE, * LPGOPHER_ABSTRACT_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR ContentType;
    LPCTSTR Language;
    DWORD Size;
} GOPHER_VIEW_ATTRIBUTE_TYPE, * LPGOPHER_VIEW_ATTRIBUTE_TYPE;

typedef struct {
    BOOL TreeWalk;
} GOPHER_VERONICA_ATTRIBUTE_TYPE, * LPGOPHER_VERONICA_ATTRIBUTE_TYPE;

typedef struct {
    LPCTSTR QuestionType;
    LPCTSTR QuestionText;
} GOPHER_ASK_ATTRIBUTE_TYPE, * LPGOPHER_ASK_ATTRIBUTE_TYPE;

//
// GOPHER_UNKNOWN_ATTRIBUTE_TYPE - this is returned if we retrieve an attribute
// that is not specified in the current gopher/gopher+ documentation. It is up
// to the application to parse the information
//

typedef struct {
    LPCTSTR Text;
} GOPHER_UNKNOWN_ATTRIBUTE_TYPE, * LPGOPHER_UNKNOWN_ATTRIBUTE_TYPE;

//
// GOPHER_ATTRIBUTE_TYPE - returned in the user's buffer when an enumerated
// GopherGetAttribute call is made
//

typedef struct {
    DWORD CategoryId;   // e.g. GOPHER_CATEGORY_ID_ADMIN
    DWORD AttributeId;  // e.g. GOPHER_ATTRIBUTE_ID_ADMIN
    union {
        GOPHER_ADMIN_ATTRIBUTE_TYPE Admin;
        GOPHER_MOD_DATE_ATTRIBUTE_TYPE ModDate;
        GOPHER_TTL_ATTRIBUTE_TYPE Ttl;
        GOPHER_SCORE_ATTRIBUTE_TYPE Score;
        GOPHER_SCORE_RANGE_ATTRIBUTE_TYPE ScoreRange;
        GOPHER_SITE_ATTRIBUTE_TYPE Site;
        GOPHER_ORGANIZATION_ATTRIBUTE_TYPE Organization;
        GOPHER_LOCATION_ATTRIBUTE_TYPE Location;
        GOPHER_GEOGRAPHICAL_LOCATION_ATTRIBUTE_TYPE GeographicalLocation;
        GOPHER_TIMEZONE_ATTRIBUTE_TYPE TimeZone;
        GOPHER_PROVIDER_ATTRIBUTE_TYPE Provider;
        GOPHER_VERSION_ATTRIBUTE_TYPE Version;
        GOPHER_ABSTRACT_ATTRIBUTE_TYPE Abstract;
        GOPHER_VIEW_ATTRIBUTE_TYPE View;
        GOPHER_VERONICA_ATTRIBUTE_TYPE Veronica;
        GOPHER_ASK_ATTRIBUTE_TYPE Ask;
        GOPHER_UNKNOWN_ATTRIBUTE_TYPE Unknown;
    } AttributeType;
} GOPHER_ATTRIBUTE_TYPE, * LPGOPHER_ATTRIBUTE_TYPE;

#define MAX_GOPHER_CATEGORY_NAME    128     // arbitrary
#define MAX_GOPHER_ATTRIBUTE_NAME   128     //     "
#define MIN_GOPHER_ATTRIBUTE_LENGTH 256     //     "

//
// known gopher attribute categories. See below for ordinals
//

#define GOPHER_INFO_CATEGORY        TEXT("+INFO")
#define GOPHER_ADMIN_CATEGORY       TEXT("+ADMIN")
#define GOPHER_VIEWS_CATEGORY       TEXT("+VIEWS")
#define GOPHER_ABSTRACT_CATEGORY    TEXT("+ABSTRACT")
#define GOPHER_VERONICA_CATEGORY    TEXT("+VERONICA")

//
// known gopher attributes. These are the attribute names as defined in the
// gopher+ protocol document
//

#define GOPHER_ADMIN_ATTRIBUTE      TEXT("Admin")
#define GOPHER_MOD_DATE_ATTRIBUTE   TEXT("Mod-Date")
#define GOPHER_TTL_ATTRIBUTE        TEXT("TTL")
#define GOPHER_SCORE_ATTRIBUTE      TEXT("Score")
#define GOPHER_RANGE_ATTRIBUTE      TEXT("Score-range")
#define GOPHER_SITE_ATTRIBUTE       TEXT("Site")
#define GOPHER_ORG_ATTRIBUTE        TEXT("Org")
#define GOPHER_LOCATION_ATTRIBUTE   TEXT("Loc")
#define GOPHER_GEOG_ATTRIBUTE       TEXT("Geog")
#define GOPHER_TIMEZONE_ATTRIBUTE   TEXT("TZ")
#define GOPHER_PROVIDER_ATTRIBUTE   TEXT("Provider")
#define GOPHER_VERSION_ATTRIBUTE    TEXT("Version")
#define GOPHER_ABSTRACT_ATTRIBUTE   TEXT("Abstract")
#define GOPHER_VIEW_ATTRIBUTE       TEXT("View")
#define GOPHER_TREEWALK_ATTRIBUTE   TEXT("treewalk")

//
// identifiers for attribute strings
//

#define GOPHER_ATTRIBUTE_ID_BASE        0xabcccc00

#define GOPHER_CATEGORY_ID_ALL          (GOPHER_ATTRIBUTE_ID_BASE + 1)

#define GOPHER_CATEGORY_ID_INFO         (GOPHER_ATTRIBUTE_ID_BASE + 2)
#define GOPHER_CATEGORY_ID_ADMIN        (GOPHER_ATTRIBUTE_ID_BASE + 3)
#define GOPHER_CATEGORY_ID_VIEWS        (GOPHER_ATTRIBUTE_ID_BASE + 4)
#define GOPHER_CATEGORY_ID_ABSTRACT     (GOPHER_ATTRIBUTE_ID_BASE + 5)
#define GOPHER_CATEGORY_ID_VERONICA     (GOPHER_ATTRIBUTE_ID_BASE + 6)
#define GOPHER_CATEGORY_ID_ASK          (GOPHER_ATTRIBUTE_ID_BASE + 7)

#define GOPHER_CATEGORY_ID_UNKNOWN      (GOPHER_ATTRIBUTE_ID_BASE + 8)

#define GOPHER_ATTRIBUTE_ID_ALL         (GOPHER_ATTRIBUTE_ID_BASE + 9)

#define GOPHER_ATTRIBUTE_ID_ADMIN       (GOPHER_ATTRIBUTE_ID_BASE + 10)
#define GOPHER_ATTRIBUTE_ID_MOD_DATE    (GOPHER_ATTRIBUTE_ID_BASE + 11)
#define GOPHER_ATTRIBUTE_ID_TTL         (GOPHER_ATTRIBUTE_ID_BASE + 12)
#define GOPHER_ATTRIBUTE_ID_SCORE       (GOPHER_ATTRIBUTE_ID_BASE + 13)
#define GOPHER_ATTRIBUTE_ID_RANGE       (GOPHER_ATTRIBUTE_ID_BASE + 14)
#define GOPHER_ATTRIBUTE_ID_SITE        (GOPHER_ATTRIBUTE_ID_BASE + 15)
#define GOPHER_ATTRIBUTE_ID_ORG         (GOPHER_ATTRIBUTE_ID_BASE + 16)
#define GOPHER_ATTRIBUTE_ID_LOCATION    (GOPHER_ATTRIBUTE_ID_BASE + 17)
#define GOPHER_ATTRIBUTE_ID_GEOG        (GOPHER_ATTRIBUTE_ID_BASE + 18)
#define GOPHER_ATTRIBUTE_ID_TIMEZONE    (GOPHER_ATTRIBUTE_ID_BASE + 19)
#define GOPHER_ATTRIBUTE_ID_PROVIDER    (GOPHER_ATTRIBUTE_ID_BASE + 20)
#define GOPHER_ATTRIBUTE_ID_VERSION     (GOPHER_ATTRIBUTE_ID_BASE + 21)
#define GOPHER_ATTRIBUTE_ID_ABSTRACT    (GOPHER_ATTRIBUTE_ID_BASE + 22)
#define GOPHER_ATTRIBUTE_ID_VIEW        (GOPHER_ATTRIBUTE_ID_BASE + 23)
#define GOPHER_ATTRIBUTE_ID_TREEWALK    (GOPHER_ATTRIBUTE_ID_BASE + 24)

#define GOPHER_ATTRIBUTE_ID_UNKNOWN     (GOPHER_ATTRIBUTE_ID_BASE + 25)

//
// prototypes
//

INTERNETAPI
BOOL
WINAPI
GopherCreateLocatorA(
    IN LPCSTR lpszHost,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszDisplayString OPTIONAL,
    IN LPCSTR lpszSelectorString OPTIONAL,
    IN DWORD dwGopherType,
    OUT LPSTR lpszLocator OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
INTERNETAPI
BOOL
WINAPI
GopherCreateLocatorW(
    IN LPCWSTR lpszHost,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR lpszDisplayString OPTIONAL,
    IN LPCWSTR lpszSelectorString OPTIONAL,
    IN DWORD dwGopherType,
    OUT LPWSTR lpszLocator OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );
#ifdef UNICODE
#define GopherCreateLocator  GopherCreateLocatorW
#else
#define GopherCreateLocator  GopherCreateLocatorA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
GopherGetLocatorTypeA(
    IN LPCSTR lpszLocator,
    OUT LPDWORD lpdwGopherType
    );
INTERNETAPI
BOOL
WINAPI
GopherGetLocatorTypeW(
    IN LPCWSTR lpszLocator,
    OUT LPDWORD lpdwGopherType
    );
#ifdef UNICODE
#define GopherGetLocatorType  GopherGetLocatorTypeW
#else
#define GopherGetLocatorType  GopherGetLocatorTypeA
#endif // !UNICODE

INTERNETAPI
HINTERNET
WINAPI
GopherFindFirstFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszLocator OPTIONAL,
    IN LPCSTR lpszSearchString OPTIONAL,
    OUT LPGOPHER_FIND_DATAA lpFindData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
HINTERNET
WINAPI
GopherFindFirstFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszLocator OPTIONAL,
    IN LPCWSTR lpszSearchString OPTIONAL,
    OUT LPGOPHER_FIND_DATAW lpFindData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define GopherFindFirstFile  GopherFindFirstFileW
#else
#define GopherFindFirstFile  GopherFindFirstFileA
#endif // !UNICODE

INTERNETAPI
HINTERNET
WINAPI
GopherOpenFileA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszView OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
HINTERNET
WINAPI
GopherOpenFileW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszLocator,
    IN LPCWSTR lpszView OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define GopherOpenFile  GopherOpenFileW
#else
#define GopherOpenFile  GopherOpenFileA
#endif // !UNICODE

typedef
BOOL
(CALLBACK * GOPHER_ATTRIBUTE_ENUMERATOR)(
    LPGOPHER_ATTRIBUTE_TYPE lpAttributeInfo,
    DWORD dwError
    );

INTERNETAPI
BOOL
WINAPI
GopherGetAttributeA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszLocator,
    IN LPCSTR lpszAttributeName OPTIONAL,
    OUT LPBYTE lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwCharactersReturned,
    IN GOPHER_ATTRIBUTE_ENUMERATOR lpfnEnumerator OPTIONAL,
    IN DWORD dwContext
    );
INTERNETAPI
BOOL
WINAPI
GopherGetAttributeW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszLocator,
    IN LPCWSTR lpszAttributeName OPTIONAL,
    OUT LPBYTE lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwCharactersReturned,
    IN GOPHER_ATTRIBUTE_ENUMERATOR lpfnEnumerator OPTIONAL,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define GopherGetAttribute  GopherGetAttributeW
#else
#define GopherGetAttribute  GopherGetAttributeA
#endif // !UNICODE

//
// HTTP
//

//
// manifests
//

//
// the default major/minor HTTP version numbers
//

#define HTTP_MAJOR_VERSION      1
#define HTTP_MINOR_VERSION      0

#define HTTP_VERSION            TEXT("HTTP/1.0")

//
// HttpQueryInfo info levels. Generally, there is one info level
// for each potential RFC822/HTTP/MIME header that an HTTP server
// may send as part of a request response.
//
// The HTTP_QUERY_RAW_HEADERS info level is provided for clients
// that choose to perform their own header parsing.
//

#define HTTP_QUERY_MIME_VERSION                 0
#define HTTP_QUERY_CONTENT_TYPE                 1
#define HTTP_QUERY_CONTENT_TRANSFER_ENCODING    2
#define HTTP_QUERY_CONTENT_ID                   3
#define HTTP_QUERY_CONTENT_DESCRIPTION          4
#define HTTP_QUERY_CONTENT_LENGTH               5
#define HTTP_QUERY_CONTENT_LANGUAGE             6
#define HTTP_QUERY_ALLOW                        7
#define HTTP_QUERY_PUBLIC                       8
#define HTTP_QUERY_DATE                         9
#define HTTP_QUERY_EXPIRES                      10
#define HTTP_QUERY_LAST_MODIFIED                11
#define HTTP_QUERY_MESSAGE_ID                   12
#define HTTP_QUERY_URI                          13
#define HTTP_QUERY_DERIVED_FROM                 14
#define HTTP_QUERY_COST                         15
#define HTTP_QUERY_LINK                         16
#define HTTP_QUERY_PRAGMA                       17
#define HTTP_QUERY_VERSION                      18  // special: part of status line
#define HTTP_QUERY_STATUS_CODE                  19  // special: part of status line
#define HTTP_QUERY_STATUS_TEXT                  20  // special: part of status line
#define HTTP_QUERY_RAW_HEADERS                  21  // special: all headers as ASCIIZ
#define HTTP_QUERY_RAW_HEADERS_CRLF             22  // special: all headers
#define HTTP_QUERY_CONNECTION                   23
#define HTTP_QUERY_ACCEPT                       24
#define HTTP_QUERY_ACCEPT_CHARSET               25
#define HTTP_QUERY_ACCEPT_ENCODING              26
#define HTTP_QUERY_ACCEPT_LANGUAGE              27
#define HTTP_QUERY_AUTHORIZATION                28
#define HTTP_QUERY_CONTENT_ENCODING             29
#define HTTP_QUERY_FORWARDED                    30
#define HTTP_QUERY_FROM                         31
#define HTTP_QUERY_IF_MODIFIED_SINCE            32
#define HTTP_QUERY_LOCATION                     33
#define HTTP_QUERY_ORIG_URI                     34
#define HTTP_QUERY_REFERER                      35
#define HTTP_QUERY_RETRY_AFTER                  36
#define HTTP_QUERY_SERVER                       37
#define HTTP_QUERY_TITLE                        38
#define HTTP_QUERY_USER_AGENT                   39
#define HTTP_QUERY_WWW_AUTHENTICATE             40
#define HTTP_QUERY_PROXY_AUTHENTICATE           41
#define HTTP_QUERY_ACCEPT_RANGES                42
#define HTTP_QUERY_SET_COOKIE                   43
#define HTTP_QUERY_COOKIE                       44
#define HTTP_QUERY_REQUEST_METHOD               45  // special: GET/POST etc.

#define HTTP_QUERY_MAX                          45

//
// HTTP_QUERY_CUSTOM - if this special value is supplied as the dwInfoLevel
// parameter of HttpQueryInfo() then the lpBuffer parameter contains the name
// of the header we are to query
//

#define HTTP_QUERY_CUSTOM                       65535

//
// HTTP_QUERY_FLAG_REQUEST_HEADERS - if this bit is set in the dwInfoLevel
// parameter of HttpQueryInfo() then the request headers will be queried for the
// request information
//

#define HTTP_QUERY_FLAG_REQUEST_HEADERS         0x80000000

//
// HTTP_QUERY_FLAG_SYSTEMTIME - if this bit is set in the dwInfoLevel parameter
// of HttpQueryInfo() AND the header being queried contains date information,
// e.g. the "Expires:" header then lpBuffer will contain a SYSTEMTIME structure
// containing the date and time information converted from the header string
//

#define HTTP_QUERY_FLAG_SYSTEMTIME              0x40000000

//
// HTTP_QUERY_FLAG_NUMBER - if this bit is set in the dwInfoLevel parameter of
// HttpQueryInfo(), then the value of the header will be converted to a number
// before being returned to the caller, if applicable
//

#define HTTP_QUERY_FLAG_NUMBER                  0x20000000

//
// HTTP_QUERY_FLAG_COALESCE - combine the values from several headers of the
// same name into the output buffer
//

#define HTTP_QUERY_FLAG_COALESCE                0x10000000

#define HTTP_QUERY_MODIFIER_FLAGS_MASK          (HTTP_QUERY_FLAG_REQUEST_HEADERS    \
                                                | HTTP_QUERY_FLAG_SYSTEMTIME        \
                                                | HTTP_QUERY_FLAG_NUMBER            \
                                                | HTTP_QUERY_FLAG_COALESCE          \
                                                )

#define HTTP_QUERY_HEADER_MASK                  (~HTTP_QUERY_MODIFIER_FLAGS_MASK)

//
// HTTP Response Status Codes:
//

#define HTTP_STATUS_OK              200     // request completed
#define HTTP_STATUS_CREATED         201     // object created, reason = new URI
#define HTTP_STATUS_ACCEPTED        202     // async completion (TBS)
#define HTTP_STATUS_PARTIAL         203     // partial completion
#define HTTP_STATUS_NO_CONTENT      204     // no info to return

#define HTTP_STATUS_AMBIGUOUS       300     // server couldn't decide what to return
#define HTTP_STATUS_MOVED           301     // object permanently moved
#define HTTP_STATUS_REDIRECT        302     // object temporarily moved
#define HTTP_STATUS_REDIRECT_METHOD 303     // redirection w/ new access method
#define HTTP_STATUS_NOT_MODIFIED    304     // if-modified-since was not modified

#define HTTP_STATUS_BAD_REQUEST     400     // invalid syntax
#define HTTP_STATUS_DENIED          401     // access denied
#define HTTP_STATUS_PAYMENT_REQ     402     // payment required
#define HTTP_STATUS_FORBIDDEN       403     // request forbidden
#define HTTP_STATUS_NOT_FOUND       404     // object not found
#define HTTP_STATUS_BAD_METHOD      405     // method is not allowed
#define HTTP_STATUS_NONE_ACCEPTABLE 406     // no response acceptable to client found
#define HTTP_STATUS_PROXY_AUTH_REQ  407     // proxy authentication required
#define HTTP_STATUS_REQUEST_TIMEOUT 408     // server timed out waiting for request
#define HTTP_STATUS_CONFLICT        409     // user should resubmit with more info
#define HTTP_STATUS_GONE            410     // the resource is no longer available
#define HTTP_STATUS_AUTH_REFUSED    411     // couldn't authorize client

#define HTTP_STATUS_SERVER_ERROR    500     // internal server error
#define HTTP_STATUS_NOT_SUPPORTED   501     // required not supported
#define HTTP_STATUS_BAD_GATEWAY     502     // error response received from gateway
#define HTTP_STATUS_SERVICE_UNAVAIL 503     // temporarily overloaded
#define HTTP_STATUS_GATEWAY_TIMEOUT 504     // timed out waiting for gateway

//
// prototypes
//

INTERNETAPI
HINTERNET
WINAPI
HttpOpenRequestA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszVerb,
    IN LPCSTR lpszObjectName,
    IN LPCSTR lpszVersion,
    IN LPCSTR lpszReferrer OPTIONAL,
    IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
INTERNETAPI
HINTERNET
WINAPI
HttpOpenRequestW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD dwContext
    );
#ifdef UNICODE
#define HttpOpenRequest  HttpOpenRequestW
#else
#define HttpOpenRequest  HttpOpenRequestA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
HttpAddRequestHeadersA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );
INTERNETAPI
BOOL
WINAPI
HttpAddRequestHeadersW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );
#ifdef UNICODE
#define HttpAddRequestHeaders  HttpAddRequestHeadersW
#else
#define HttpAddRequestHeaders  HttpAddRequestHeadersA
#endif // !UNICODE

//
// values for dwModifiers parameter of HttpAddRequestHeaders()
//

#define HTTP_ADDREQ_INDEX_MASK      0x0000FFFF
#define HTTP_ADDREQ_FLAGS_MASK      0xFFFF0000

//
// HTTP_ADDREQ_FLAG_ADD_IF_NEW - the header will only be added if it doesn't
// already exist
//

#define HTTP_ADDREQ_FLAG_ADD_IF_NEW 0x10000000

//
// HTTP_ADDREQ_FLAG_ADD - if HTTP_ADDREQ_FLAG_REPLACE is set but the header is
// not found then if this flag is set, the header is added anyway, so long as
// there is a valid header-value
//

#define HTTP_ADDREQ_FLAG_ADD        0x20000000

//
// HTTP_ADDREQ_FLAG_COALESCE - coalesce headers with same name. e.g.
// "Accept: text/*" and "Accept: audio/*" with this flag results in a single
// header: "Accept: text/*, audio/*"
//

//
// HTTP_ADDREQ_FLAG_COALESCE - coalesce headers with same name. e.g.
// "Accept: text/*" and "Accept: audio/*" with this flag results in a single
// header: "Accept: text/*, audio/*"
//

#define HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA       0x40000000

#define HTTP_ADDREQ_FLAG_COALESCE_WITH_SEMICOLON   0x01000000

#define HTTP_ADDREQ_FLAG_COALESCE                  HTTP_ADDREQ_FLAG_COALESCE_WITH_COMMA

//
// HTTP_ADDREQ_FLAG_REPLACE - replaces the specified header. Only one header can
// be supplied in the buffer. If the header to be replaced is not the first
// in a list of headers with the same name, then the relative index should be
// supplied in the low 8 bits of the dwModifiers parameter. If the header-value
// part is missing, then the header is removed
//

#define HTTP_ADDREQ_FLAG_REPLACE    0x80000000

INTERNETAPI
BOOL
WINAPI
HttpSendRequestA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );
INTERNETAPI
BOOL
WINAPI
HttpSendRequestW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );
#ifdef UNICODE
#define HttpSendRequest  HttpSendRequestW
#else
#define HttpSendRequest  HttpSendRequestA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
HttpQueryInfoA(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );
INTERNETAPI
BOOL
WINAPI
HttpQueryInfoW(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );
#ifdef UNICODE
#define HttpQueryInfo  HttpQueryInfoW
#else
#define HttpQueryInfo  HttpQueryInfoA
#endif // !UNICODE

//
// Cookie APIs
//

INTERNETAPI
BOOL
WINAPI
InternetSetCookieA(
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszCookieName,
    IN LPCSTR lpszCookieData
    );
INTERNETAPI
BOOL
WINAPI
InternetSetCookieW(
    IN LPCSTR lpszUrl,
    IN LPCWSTR lpszCookieName,
    IN LPCWSTR lpszCookieData
    );
#ifdef UNICODE
#define InternetSetCookie  InternetSetCookieW
#else
#define InternetSetCookie  InternetSetCookieA
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetGetCookieA(
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszCookieName,
    OUT LPSTR lpCookieData,
    IN OUT LPDWORD lpdwSize
    );
INTERNETAPI
BOOL
WINAPI
InternetGetCookieW(
    IN LPCSTR lpszUrl,
    IN LPCWSTR lpszCookieName,
    OUT LPWSTR lpCookieData,
    IN OUT LPDWORD lpdwSize
    );
#ifdef UNICODE
#define InternetGetCookie  InternetGetCookieW
#else
#define InternetGetCookie  InternetGetCookieA
#endif // !UNICODE

//
// offline browsing
//

INTERNETAPI
DWORD
WINAPI
InternetAttemptConnect(
    DWORD dwReserved
    );

//
// Internet UI
//

//
// InternetErrorDlg - Provides UI for certain Errors.
//

#define FLAGS_ERROR_UI_FILTER_FOR_ERRORS        0x01
#define FLAGS_ERROR_UI_FLAGS_CHANGE_OPTIONS     0x02
#define FLAGS_ERROR_UI_FLAGS_GENERATE_DATA      0x04
#define FLAGS_ERROR_UI_FLAGS_NO_UI              0x08

INTERNETAPI
DWORD
WINAPI
InternetErrorDlg(
     IN HWND hWnd,
     IN OUT HINTERNET hRequest,
     IN DWORD dwError,
     IN DWORD dwFlags,
     IN OUT LPVOID * lppvData
     );

INTERNETAPI
DWORD
WINAPI
InternetConfirmZoneCrossing(
     IN HWND hWnd,
     IN LPSTR szUrlPrev,
     IN LPSTR szUrlNew,
     IN BOOL bPost
     );

//#if !defined(_WINERROR_)

//
// Internet API error returns
//

#define INTERNET_ERROR_BASE                     12000

#define ERROR_INTERNET_OUT_OF_HANDLES           (INTERNET_ERROR_BASE + 1)
#define ERROR_INTERNET_TIMEOUT                  (INTERNET_ERROR_BASE + 2)
#define ERROR_INTERNET_EXTENDED_ERROR           (INTERNET_ERROR_BASE + 3)
#define ERROR_INTERNET_INTERNAL_ERROR           (INTERNET_ERROR_BASE + 4)
#define ERROR_INTERNET_INVALID_URL              (INTERNET_ERROR_BASE + 5)
#define ERROR_INTERNET_UNRECOGNIZED_SCHEME      (INTERNET_ERROR_BASE + 6)
#define ERROR_INTERNET_NAME_NOT_RESOLVED        (INTERNET_ERROR_BASE + 7)
#define ERROR_INTERNET_PROTOCOL_NOT_FOUND       (INTERNET_ERROR_BASE + 8)
#define ERROR_INTERNET_INVALID_OPTION           (INTERNET_ERROR_BASE + 9)
#define ERROR_INTERNET_BAD_OPTION_LENGTH        (INTERNET_ERROR_BASE + 10)
#define ERROR_INTERNET_OPTION_NOT_SETTABLE      (INTERNET_ERROR_BASE + 11)
#define ERROR_INTERNET_SHUTDOWN                 (INTERNET_ERROR_BASE + 12)
#define ERROR_INTERNET_INCORRECT_USER_NAME      (INTERNET_ERROR_BASE + 13)
#define ERROR_INTERNET_INCORRECT_PASSWORD       (INTERNET_ERROR_BASE + 14)
#define ERROR_INTERNET_LOGIN_FAILURE            (INTERNET_ERROR_BASE + 15)
#define ERROR_INTERNET_INVALID_OPERATION        (INTERNET_ERROR_BASE + 16)
#define ERROR_INTERNET_OPERATION_CANCELLED      (INTERNET_ERROR_BASE + 17)
#define ERROR_INTERNET_INCORRECT_HANDLE_TYPE    (INTERNET_ERROR_BASE + 18)
#define ERROR_INTERNET_INCORRECT_HANDLE_STATE   (INTERNET_ERROR_BASE + 19)
#define ERROR_INTERNET_NOT_PROXY_REQUEST        (INTERNET_ERROR_BASE + 20)
#define ERROR_INTERNET_REGISTRY_VALUE_NOT_FOUND (INTERNET_ERROR_BASE + 21)
#define ERROR_INTERNET_BAD_REGISTRY_PARAMETER   (INTERNET_ERROR_BASE + 22)
#define ERROR_INTERNET_NO_DIRECT_ACCESS         (INTERNET_ERROR_BASE + 23)
#define ERROR_INTERNET_NO_CONTEXT               (INTERNET_ERROR_BASE + 24)
#define ERROR_INTERNET_NO_CALLBACK              (INTERNET_ERROR_BASE + 25)
#define ERROR_INTERNET_REQUEST_PENDING          (INTERNET_ERROR_BASE + 26)
#define ERROR_INTERNET_INCORRECT_FORMAT         (INTERNET_ERROR_BASE + 27)
#define ERROR_INTERNET_ITEM_NOT_FOUND           (INTERNET_ERROR_BASE + 28)
#define ERROR_INTERNET_CANNOT_CONNECT           (INTERNET_ERROR_BASE + 29)
#define ERROR_INTERNET_CONNECTION_ABORTED       (INTERNET_ERROR_BASE + 30)
#define ERROR_INTERNET_CONNECTION_RESET         (INTERNET_ERROR_BASE + 31)
#define ERROR_INTERNET_FORCE_RETRY              (INTERNET_ERROR_BASE + 32)
#define ERROR_INTERNET_INVALID_PROXY_REQUEST    (INTERNET_ERROR_BASE + 33)
#define ERROR_INTERNET_NEED_UI                  (INTERNET_ERROR_BASE + 34)

#define ERROR_INTERNET_HANDLE_EXISTS            (INTERNET_ERROR_BASE + 36)
#define ERROR_INTERNET_SEC_CERT_DATE_INVALID    (INTERNET_ERROR_BASE + 37)
#define ERROR_INTERNET_SEC_CERT_CN_INVALID      (INTERNET_ERROR_BASE + 38)
#define ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR   (INTERNET_ERROR_BASE + 39)
#define ERROR_INTERNET_HTTPS_TO_HTTP_ON_REDIR   (INTERNET_ERROR_BASE + 40)
#define ERROR_INTERNET_MIXED_SECURITY           (INTERNET_ERROR_BASE + 41)
#define ERROR_INTERNET_CHG_POST_IS_NON_SECURE   (INTERNET_ERROR_BASE + 42)
#define ERROR_INTERNET_POST_IS_NON_SECURE       (INTERNET_ERROR_BASE + 43)
#define ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED  (INTERNET_ERROR_BASE + 44)
#define ERROR_INTERNET_INVALID_CA               (INTERNET_ERROR_BASE + 45)
#define ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP    (INTERNET_ERROR_BASE + 46)
#define ERROR_INTERNET_ASYNC_THREAD_FAILED      (INTERNET_ERROR_BASE + 47)
#define ERROR_INTERNET_REDIRECT_SCHEME_CHANGE   (INTERNET_ERROR_BASE + 48)

//
// FTP API errors
//

#define ERROR_FTP_TRANSFER_IN_PROGRESS          (INTERNET_ERROR_BASE + 110)
#define ERROR_FTP_DROPPED                       (INTERNET_ERROR_BASE + 111)

//
// gopher API errors
//

#define ERROR_GOPHER_PROTOCOL_ERROR             (INTERNET_ERROR_BASE + 130)
#define ERROR_GOPHER_NOT_FILE                   (INTERNET_ERROR_BASE + 131)
#define ERROR_GOPHER_DATA_ERROR                 (INTERNET_ERROR_BASE + 132)
#define ERROR_GOPHER_END_OF_DATA                (INTERNET_ERROR_BASE + 133)
#define ERROR_GOPHER_INVALID_LOCATOR            (INTERNET_ERROR_BASE + 134)
#define ERROR_GOPHER_INCORRECT_LOCATOR_TYPE     (INTERNET_ERROR_BASE + 135)
#define ERROR_GOPHER_NOT_GOPHER_PLUS            (INTERNET_ERROR_BASE + 136)
#define ERROR_GOPHER_ATTRIBUTE_NOT_FOUND        (INTERNET_ERROR_BASE + 137)
#define ERROR_GOPHER_UNKNOWN_LOCATOR            (INTERNET_ERROR_BASE + 138)

//
// HTTP API errors
//

#define ERROR_HTTP_HEADER_NOT_FOUND             (INTERNET_ERROR_BASE + 150)
#define ERROR_HTTP_DOWNLEVEL_SERVER             (INTERNET_ERROR_BASE + 151)
#define ERROR_HTTP_INVALID_SERVER_RESPONSE      (INTERNET_ERROR_BASE + 152)
#define ERROR_HTTP_INVALID_HEADER               (INTERNET_ERROR_BASE + 153)
#define ERROR_HTTP_INVALID_QUERY_REQUEST        (INTERNET_ERROR_BASE + 154)
#define ERROR_HTTP_HEADER_ALREADY_EXISTS        (INTERNET_ERROR_BASE + 155)
#define ERROR_HTTP_REDIRECT_FAILED              (INTERNET_ERROR_BASE + 156)
#define ERROR_HTTP_NOT_REDIRECTED               (INTERNET_ERROR_BASE + 160) // BUGBUG

#define ERROR_INTERNET_SECURITY_CHANNEL_ERROR   (INTERNET_ERROR_BASE + 157) // BUGBUG
#define ERROR_INTERNET_UNABLE_TO_CACHE_FILE     (INTERNET_ERROR_BASE + 158) // BUGBUG
#define ERROR_INTERNET_TCPIP_NOT_INSTALLED      (INTERNET_ERROR_BASE + 159) // BUGBUG

#define INTERNET_ERROR_LAST                     ERROR_INTERNET_TCPIP_NOT_INSTALLED

//#endif // !defined(_WINERROR_)

//
// URLCACHE APIs
//

#if !defined(_WINX32_) && !defined(_URLCACHEAPI_)
#define URLCACHEAPI DECLSPEC_IMPORT
#else
#define URLCACHEAPI
#endif

//
// datatype definitions.
//

//
// cache entry type flags.
//

#define NORMAL_CACHE_ENTRY      0x00000001
#define STABLE_CACHE_ENTRY      0x00000002
#define STICKY_CACHE_ENTRY      0x00000004

#define SPARSE_CACHE_ENTRY      0x00010000
#define OCX_CACHE_ENTRY         0x00020000

#define COOKIE_CACHE_ENTRY      0x00100000
#define URLHISTORY_CACHE_ENTRY  0x00200000

//
// INTERNET_CACHE_ENTRY_INFO -
//

typedef struct _INTERNET_CACHE_ENTRY_INFOA {
    DWORD dwStructSize;         // version of cache system. 
    LPSTR lpszSourceUrlName;    // embedded pointer to the URL name string.
    LPSTR   lpszLocalFileName;  // embedded pointer to the local file name.
    DWORD CacheEntryType;       // cache type bit mask.
    DWORD dwUseCount;           // current users count of the cache entry.
    DWORD dwHitRate;            // num of times the cache entry was retrieved.
    DWORD dwSizeLow;            // low DWORD of the file size.
    DWORD dwSizeHigh;           // high DWORD of the file size.
    FILETIME LastModifiedTime;  // last modified time of the file in GMT format.
    FILETIME ExpireTime;        // expire time of the file in GMT format
    FILETIME LastAccessTime;    // last accessed time in GMT format
    FILETIME LastSyncTime;      // last time the URL was synchronized
                                // with the source
    LPBYTE lpHeaderInfo;        // embedded pointer to the header info.
    DWORD dwHeaderInfoSize;     // size of the above header.
    LPSTR   lpszFileExtension;  // File extension used to retrive the urldata as a file.
    DWORD dwReserved;           // reserved for future use.
} INTERNET_CACHE_ENTRY_INFOA, * LPINTERNET_CACHE_ENTRY_INFOA;
typedef struct _INTERNET_CACHE_ENTRY_INFOW {
    DWORD dwStructSize;         // version of cache system. ?? do we need this for all entries?
    LPSTR lpszSourceUrlName;    // embedded pointer to the URL name string.
    LPWSTR  lpszLocalFileName;  // embedded pointer to the local file name.
    DWORD CacheEntryType;       // cache type bit mask.
    DWORD dwUseCount;           // current users count of the cache entry.
    DWORD dwHitRate;            // num of times the cache entry was retrieved.
    DWORD dwSizeLow;            // low DWORD of the file size.
    DWORD dwSizeHigh;           // high DWORD of the file size.
    FILETIME LastModifiedTime;  // last modified time of the file in GMT format.
    FILETIME ExpireTime;        // expire time of the file in GMT format
    FILETIME LastAccessTime;    // last accessed time in GMT format
    FILETIME LastSyncTime;      // last time the URL was synchronized
                                // with the source
    LPBYTE lpHeaderInfo;        // embedded pointer to the header info.
    DWORD dwHeaderInfoSize;     // size of the above header.
    LPWSTR  lpszFileExtension;  // File extension used to retrive the urldata as a file.
    DWORD dwReserved;           // reserved for future use.
} INTERNET_CACHE_ENTRY_INFOW, * LPINTERNET_CACHE_ENTRY_INFOW;
#ifdef UNICODE
typedef INTERNET_CACHE_ENTRY_INFOW INTERNET_CACHE_ENTRY_INFO;
typedef LPINTERNET_CACHE_ENTRY_INFOW LPINTERNET_CACHE_ENTRY_INFO;
#else
typedef INTERNET_CACHE_ENTRY_INFOA INTERNET_CACHE_ENTRY_INFO;
typedef LPINTERNET_CACHE_ENTRY_INFOA LPINTERNET_CACHE_ENTRY_INFO;
#endif // UNICODE

//
// Cache APIs
//

URLCACHEAPI
BOOL
WINAPI
CreateUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPSTR lpszFileName,
    IN DWORD dwReserved
    );
URLCACHEAPI
BOOL
WINAPI
CreateUrlCacheEntryW(
    IN LPCSTR lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCSTR lpszFileExtension,
    OUT LPWSTR lpszFileName,
    IN DWORD dwReserved
    );
#ifdef UNICODE
#define CreateUrlCacheEntry  CreateUrlCacheEntryW
#else
#define CreateUrlCacheEntry  CreateUrlCacheEntryA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN DWORD dwReserved
    );
URLCACHEAPI
BOOL
WINAPI
CommitUrlCacheEntryW(
    IN LPCSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCWSTR lpszFileExtension,
    IN DWORD dwReserved
    );
#ifdef UNICODE
#define CommitUrlCacheEntry  CommitUrlCacheEntryW
#else
#define CommitUrlCacheEntry  CommitUrlCacheEntryA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
RetrieveUrlCacheEntryFileA(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    );
URLCACHEAPI
BOOL
WINAPI
RetrieveUrlCacheEntryFileW(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    );
#ifdef UNICODE
#define RetrieveUrlCacheEntryFile  RetrieveUrlCacheEntryFileW
#else
#define RetrieveUrlCacheEntryFile  RetrieveUrlCacheEntryFileA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
UnlockUrlCacheEntryFile(
    IN LPCSTR lpszUrlName,
    IN DWORD dwReserved
    );

URLCACHEAPI
HANDLE
WINAPI
RetrieveUrlCacheEntryStreamA(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    );
URLCACHEAPI
HANDLE
WINAPI
RetrieveUrlCacheEntryStreamW(
    IN LPCSTR  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    );
#ifdef UNICODE
#define RetrieveUrlCacheEntryStream  RetrieveUrlCacheEntryStreamW
#else
#define RetrieveUrlCacheEntryStream  RetrieveUrlCacheEntryStreamA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
ReadUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD dwLocation,
    IN OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwLen,
    IN DWORD Reserved
    );

URLCACHEAPI
BOOL
WINAPI
UnlockUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD Reserved
    );

URLCACHEAPI
BOOL
WINAPI
GetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );
URLCACHEAPI
BOOL
WINAPI
GetUrlCacheEntryInfoW(
    IN LPCSTR lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );
#ifdef UNICODE
#define GetUrlCacheEntryInfo  GetUrlCacheEntryInfoW
#else
#define GetUrlCacheEntryInfo  GetUrlCacheEntryInfoA
#endif // !UNICODE

#define CACHE_ENTRY_ATTRIBUTE_FC    0x00000004
#define CACHE_ENTRY_HITRATE_FC      0x00000010
#define CACHE_ENTRY_MODTIME_FC      0x00000040
#define CACHE_ENTRY_EXPTIME_FC      0x00000080
#define CACHE_ENTRY_ACCTIME_FC      0x00000100
#define CACHE_ENTRY_SYNCTIME_FC     0x00000200
#define CACHE_ENTRY_HEADERINFO_FC   0x00000400

URLCACHEAPI
BOOL
WINAPI
SetUrlCacheEntryInfoA(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOA lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );
URLCACHEAPI
BOOL
WINAPI
SetUrlCacheEntryInfoW(
    IN LPCSTR lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFOW lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );
#ifdef UNICODE
#define SetUrlCacheEntryInfo  SetUrlCacheEntryInfoW
#else
#define SetUrlCacheEntryInfo  SetUrlCacheEntryInfoA
#endif // !UNICODE

URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheEntryA(
    IN LPCSTR lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    );
URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheEntryW(
    IN LPCSTR lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    );
#ifdef UNICODE
#define FindFirstUrlCacheEntry  FindFirstUrlCacheEntryW
#else
#define FindFirstUrlCacheEntry  FindFirstUrlCacheEntryA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheEntryA(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_ENTRY_INFOA lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    );
URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheEntryW(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_ENTRY_INFOW lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    );
#ifdef UNICODE
#define FindNextUrlCacheEntry  FindNextUrlCacheEntryW
#else
#define FindNextUrlCacheEntry  FindNextUrlCacheEntryA
#endif // !UNICODE

URLCACHEAPI
BOOL
WINAPI
FindCloseUrlCache(
    IN HANDLE hEnumHandle
    );

URLCACHEAPI
BOOL
WINAPI
DeleteUrlCacheEntry(
    IN LPCSTR lpszUrlName
    );

#if defined(__cplusplus)
}
#endif

/*
 * Return packing to whatever it was before we
 * entered this file
 */
#pragma pack(pop, wininet)

#endif // !defined(_WININET_)
