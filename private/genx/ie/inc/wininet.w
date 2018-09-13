/*++

Copyright (c) 1995-1999  Microsoft Corporation

Module Name:

    wininet.h

Abstract:

    Contains manifests, macros, types and prototypes for Microsoft Windows
    Internet Extensions

--*/


#if !defined(_WININET_)
#define _WININET_

;begin_internal
#if !defined(_WININETEX_)
#define _WININETEX_
;end_internal

/*
 * Set up Structure Packing to be 4 bytes
 * for all wininet structures
 */
#include <pshpack4.h>



;begin_both

#if defined(__cplusplus)
extern "C" {
#endif

;end_both

#if !defined(_WINX32_)
#define INTERNETAPI DECLSPEC_IMPORT
#define URLCACHEAPI DECLSPEC_IMPORT
#else
#define INTERNETAPI
#define URLCACHEAPI
#endif

#define BOOLAPI INTERNETAPI BOOL WINAPI

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

;begin_internal
#define MAX_CACHE_ENTRY_INFO_SIZE       4096
;end_internal

//
// maximum field lengths (arbitrary)
//

#define INTERNET_MAX_HOST_NAME_LENGTH   256
#define INTERNET_MAX_USER_NAME_LENGTH   128
#define INTERNET_MAX_PASSWORD_LENGTH    128
#define INTERNET_MAX_PORT_NUMBER_LENGTH 5           // INTERNET_PORT is unsigned short
#define INTERNET_MAX_PORT_NUMBER_VALUE  65535       // maximum unsigned short value
#define INTERNET_MAX_PATH_LENGTH        2048
#define INTERNET_MAX_SCHEME_LENGTH      32          // longest protocol name length
#define INTERNET_MAX_URL_LENGTH         (INTERNET_MAX_SCHEME_LENGTH \
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

#define INTERNET_REQFLAG_FROM_CACHE     0x00000001  // response came from cache
#define INTERNET_REQFLAG_ASYNC          0x00000002  // request was made asynchronously
#define INTERNET_REQFLAG_VIA_PROXY      0x00000004  // request was made via a proxy
#define INTERNET_REQFLAG_NO_HEADERS     0x00000008  // orginal response contained no headers
#define INTERNET_REQFLAG_PASSIVE        0x00000010  // FTP: passive-mode connection
#define INTERNET_REQFLAG_CACHE_WRITE_DISABLED 0x00000040  // HTTPS: this request not cacheable
#define INTERNET_REQFLAG_NET_TIMEOUT    0x00000080  // w/ _FROM_CACHE: net request timed out

//
// flags common to open functions (not InternetOpen()):
//

#define INTERNET_FLAG_RELOAD            0x80000000  // retrieve the original item

//
// flags for InternetOpenUrl():
//

#define INTERNET_FLAG_RAW_DATA          0x40000000  // FTP/gopher find: receive the item as raw (structured) data
#define INTERNET_FLAG_EXISTING_CONNECT  0x20000000  // FTP: use existing InternetConnect handle for server if possible

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
#define INTERNET_FLAG_FROM_CACHE        0x01000000  // use offline semantics
#define INTERNET_FLAG_OFFLINE           INTERNET_FLAG_FROM_CACHE

//
// additional flags
//

#define INTERNET_FLAG_SECURE            0x00800000  // use PCT/SSL if applicable (HTTP)
#define INTERNET_FLAG_KEEP_CONNECTION   0x00400000  // use keep-alive semantics
#define INTERNET_FLAG_NO_AUTO_REDIRECT  0x00200000  // don't handle redirections automatically
#define INTERNET_FLAG_READ_PREFETCH     0x00100000  // do background read prefetch
#define INTERNET_FLAG_NO_COOKIES        0x00080000  // no automatic cookie handling
#define INTERNET_FLAG_NO_AUTH           0x00040000  // no automatic authentication handling
;begin_internal
#define INTERNET_FLAG_UNUSED_1          0x00020000
;end_internal
#define INTERNET_FLAG_CACHE_IF_NET_FAIL 0x00010000  // return cache file if net request fails

//
// Security Ignore Flags, Allow HttpOpenRequest to overide
//  Secure Channel (SSL/PCT) failures of the following types.
//

#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   0x00008000 // ex: https:// to http://
#define INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  0x00004000 // ex: http:// to https://
#define INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  0x00002000 // expired X509 Cert.
#define INTERNET_FLAG_IGNORE_CERT_CN_INVALID    0x00001000 // bad common name in X509 Cert.

//
// more caching flags
//

#define INTERNET_FLAG_RESYNCHRONIZE     0x00000800  // asking wininet to update an item if it is newer
#define INTERNET_FLAG_HYPERLINK         0x00000400  // asking wininet to do hyperlinking semantic which works right for scripts
#define INTERNET_FLAG_NO_UI             0x00000200  // no cookie popup
#define INTERNET_FLAG_PRAGMA_NOCACHE    0x00000100  // asking wininet to add "pragma: no-cache"
#define INTERNET_FLAG_CACHE_ASYNC       0x00000080  // ok to perform lazy cache-write
#define INTERNET_FLAG_FORMS_SUBMIT      0x00000040  // this is a forms submit
#define INTERNET_FLAG_FWD_BACK          0x00000020  // fwd-back button op 
;begin_internal
;end_internal
#define INTERNET_FLAG_NEED_FILE         0x00000010  // need a file for this request
#define INTERNET_FLAG_MUST_CACHE_REQUEST INTERNET_FLAG_NEED_FILE
;begin_internal
#define INTERNET_FLAG_BGUPDATE          0x00000008
#define INTERNET_FLAG_UNUSED_4          0x00000004
;end_internal

//
// flags for FTP
//

#define INTERNET_FLAG_TRANSFER_ASCII    FTP_TRANSFER_TYPE_ASCII     // 0x00000001
#define INTERNET_FLAG_TRANSFER_BINARY   FTP_TRANSFER_TYPE_BINARY    // 0x00000002

//
// flags field masks
//

#define SECURITY_INTERNET_MASK  (INTERNET_FLAG_IGNORE_CERT_CN_INVALID    |  \
                                 INTERNET_FLAG_IGNORE_CERT_DATE_INVALID  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS  |  \
                                 INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP   )

#define INTERNET_FLAGS_MASK     (INTERNET_FLAG_RELOAD               \
                                | INTERNET_FLAG_RAW_DATA            \
                                | INTERNET_FLAG_EXISTING_CONNECT    \
                                | INTERNET_FLAG_ASYNC               \
                                | INTERNET_FLAG_PASSIVE             \
                                | INTERNET_FLAG_NO_CACHE_WRITE      \
                                | INTERNET_FLAG_MAKE_PERSISTENT     \
                                | INTERNET_FLAG_FROM_CACHE          \
                                | INTERNET_FLAG_SECURE              \
                                | INTERNET_FLAG_KEEP_CONNECTION     \
                                | INTERNET_FLAG_NO_AUTO_REDIRECT    \
                                | INTERNET_FLAG_READ_PREFETCH       \
                                | INTERNET_FLAG_NO_COOKIES          \
                                | INTERNET_FLAG_NO_AUTH             \
                                | INTERNET_FLAG_CACHE_IF_NET_FAIL   \
                                | SECURITY_INTERNET_MASK            \
                                | INTERNET_FLAG_RESYNCHRONIZE       \
                                | INTERNET_FLAG_HYPERLINK           \
                                | INTERNET_FLAG_NO_UI               \
                                | INTERNET_FLAG_PRAGMA_NOCACHE      \
                                | INTERNET_FLAG_CACHE_ASYNC         \
                                | INTERNET_FLAG_FORMS_SUBMIT        \
                                | INTERNET_FLAG_NEED_FILE           \
                                | INTERNET_FLAG_TRANSFER_BINARY     \
                                | INTERNET_FLAG_TRANSFER_ASCII      \
                                | INTERNET_FLAG_FWD_BACK            \
                                | INTERNET_FLAG_BGUPDATE            \
                                )

#define INTERNET_ERROR_MASK_INSERT_CDROM                    0x1
#define INTERNET_ERROR_MASK_COMBINED_SEC_CERT               0x2
#define INTERNET_ERROR_MASK_NEED_MSN_SSPI_PKG               0X4
#define INTERNET_ERROR_MASK_LOGIN_FAILURE_DISPLAY_ENTITY_BODY 0x8

#define INTERNET_OPTIONS_MASK   (~INTERNET_FLAGS_MASK)

//
// common per-API flags (new APIs)
//

#define WININET_API_FLAG_ASYNC          0x00000001  // force async operation
#define WININET_API_FLAG_SYNC           0x00000004  // force sync operation
#define WININET_API_FLAG_USE_CONTEXT    0x00000008  // use value supplied in dwContext (even if 0)

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
    INTERNET_SCHEME_JAVASCRIPT,
    INTERNET_SCHEME_VBSCRIPT,
    INTERNET_SCHEME_FIRST = INTERNET_SCHEME_FTP,
    INTERNET_SCHEME_LAST = INTERNET_SCHEME_VBSCRIPT
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

;begin_internal

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
// INTERNET_DIAGNOSTIC_SOCKET_INFO - info about the socket in use
// (diagnostic purposes only, hence internal)
//

typedef struct {
    DWORD_PTR Socket;
    DWORD     SourcePort;
    DWORD     DestPort;
    DWORD     Flags;
} INTERNET_DIAGNOSTIC_SOCKET_INFO, * LPINTERNET_DIAGNOSTIC_SOCKET_INFO;

//
// INTERNET_DIAGNOSTIC_SOCKET_INFO.Flags definitions
//

#define IDSI_FLAG_KEEP_ALIVE    0x00000001  // set if from keep-alive pool
#define IDSI_FLAG_SECURE        0x00000002  // set if secure connection
#define IDSI_FLAG_PROXY         0x00000004  // set if using proxy
#define IDSI_FLAG_TUNNEL        0x00000008  // set if tunnelling through proxy

;end_internal

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
// INTERNET_PER_CONN_OPTION_LIST - set per-connection options such as proxy
// and autoconfig info
//
// Set and queried using Internet[Set|Query]Option with 
// INTERNET_OPTION_PER_CONNECTION_OPTION
//

typedef struct {
    DWORD   dwOption;            // option to be queried or set
    union {                     
        DWORD    dwValue;        // dword value for the option 
        LPTSTR%  pszValue;       // pointer to string value for the option         
        FILETIME ftValue;        // file-time value for the option
    } Value;    
} INTERNET_PER_CONN_OPTION%, * LPINTERNET_PER_CONN_OPTION%;

typedef struct {
    DWORD   dwSize;             // size of the INTERNET_PER_CONN_OPTION_LIST struct 
    LPTSTR% pszConnection;      // connection name to set/query options 
    DWORD   dwOptionCount;      // number of options to set/query 
    DWORD   dwOptionError;      // on error, which option failed 
    LPINTERNET_PER_CONN_OPTION%  pOptions;
                                // array of options to set/query 
} INTERNET_PER_CONN_OPTION_LIST%, * LPINTERNET_PER_CONN_OPTION_LIST%;

//
// Options used in INTERNET_PER_CONN_OPTON struct
//
#define INTERNET_PER_CONN_FLAGS                         1
#define INTERNET_PER_CONN_PROXY_SERVER                  2
#define INTERNET_PER_CONN_PROXY_BYPASS                  3
#define INTERNET_PER_CONN_AUTOCONFIG_URL                4
#define INTERNET_PER_CONN_AUTODISCOVERY_FLAGS           5
;begin_internal
#define INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL      6
#define INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS  7
#define INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME   8
#define INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL    9
;end_internal

//
// PER_CONN_FLAGS
//
#define PROXY_TYPE_DIRECT                               0x00000001   // direct to net
#define PROXY_TYPE_PROXY                                0x00000002   // via named proxy
#define PROXY_TYPE_AUTO_PROXY_URL                       0x00000004   // autoproxy URL
#define PROXY_TYPE_AUTO_DETECT                          0x00000008   // use autoproxy detection

//
// PER_CONN_AUTODISCOVERY_FLAGS
//
#define AUTO_PROXY_FLAG_USER_SET                        0x00000001   // user changed this setting
#define AUTO_PROXY_FLAG_ALWAYS_DETECT                   0x00000002   // force detection even when its not needed
#define AUTO_PROXY_FLAG_DETECTION_RUN                   0x00000004   // detection has been run
#define AUTO_PROXY_FLAG_MIGRATED                        0x00000008   // migration has just been done 
#define AUTO_PROXY_FLAG_DONT_CACHE_PROXY_RESULT         0x00000010   // don't cache result of host=proxy name
#define AUTO_PROXY_FLAG_CACHE_INIT_RUN                  0x00000020   // don't initalize and run unless URL expired
#define AUTO_PROXY_FLAG_DETECTION_SUSPECT               0x00000040   // if we're on a LAN & Modem, with only one IP, bad?!?

//
// INTERNET_VERSION_INFO - version information returned via
// InternetQueryOption(..., INTERNET_OPTION_VERSION, ...)
//

typedef struct {
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
} INTERNET_VERSION_INFO, * LPINTERNET_VERSION_INFO;

//
// HTTP_VERSION_INFO - query or set global HTTP version (1.0 or 1.1)
//

typedef struct {
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
} HTTP_VERSION_INFO, * LPHTTP_VERSION_INFO;

//
// INTERNET_CONNECTED_INFO - information used to set the global connected state
//

typedef struct {

    //
    // dwConnectedState - new connected/disconnected state.
    // See INTERNET_STATE_CONNECTED, etc.
    //

    DWORD dwConnectedState;

    //
    // dwFlags - flags controlling connected->disconnected (or disconnected->
    // connected) transition. See below
    //

    DWORD dwFlags;
} INTERNET_CONNECTED_INFO, * LPINTERNET_CONNECTED_INFO;

;begin_internal

#define INTERNET_ONLINE_OFFLINE_INFO    INTERNET_CONNECTED_INFO
#define LPINTERNET_ONLINE_OFFLINE_INFO  LPINTERNET_CONNECTED_INFO
#define dwOfflineState                  dwConnectedState

;end_internal

//
// flags for INTERNET_CONNECTED_INFO dwFlags
//

//
// ISO_FORCE_DISCONNECTED - if set when putting Wininet into disconnected mode,
// all outstanding requests will be aborted with a cancelled error
//

#define ISO_FORCE_DISCONNECTED  0x00000001

;begin_internal

#define ISO_FORCE_OFFLINE       ISO_FORCE_DISCONNECTED

;end_internal

//
// URL_COMPONENTS - the constituent parts of an URL. Used in InternetCrackUrl()
// and InternetCreateUrl()
//
// For InternetCrackUrl(), if a pointer field and its corresponding length field
// are both 0 then that component is not returned. If the pointer field is NULL
// but the length field is not zero, then both the pointer and length fields are
// returned if both pointer and corresponding length fields are non-zero then
// the pointer field points to a buffer where the component is copied. The
// component may be un-escaped, depending on dwFlags
//
// For InternetCreateUrl(), the pointer fields should be NULL if the component
// is not required. If the corresponding length field is zero then the pointer
// field is the address of a zero-terminated string. If the length field is not
// zero then it is the string length of the corresponding pointer field
//

#pragma warning( disable : 4121 )   // disable alignment warning

typedef struct {
    DWORD   dwStructSize;       // size of this structure. Used in version check
    LPTSTR% lpszScheme;         // pointer to scheme name
    DWORD   dwSchemeLength;     // length of scheme name
    INTERNET_SCHEME nScheme;    // enumerated scheme type (if known)
    LPTSTR% lpszHostName;       // pointer to host name
    DWORD   dwHostNameLength;   // length of host name
    INTERNET_PORT nPort;        // converted port number
    LPTSTR% lpszUserName;       // pointer to user name
    DWORD   dwUserNameLength;   // length of user name
    LPTSTR% lpszPassword;       // pointer to password
    DWORD   dwPasswordLength;   // length of password
    LPTSTR% lpszUrlPath;        // pointer to URL-path
    DWORD   dwUrlPathLength;    // length of URL-path
    LPTSTR% lpszExtraInfo;      // pointer to extra information (e.g. ?foo or #foo)
    DWORD   dwExtraInfoLength;  // length of extra information
} URL_COMPONENTS%, * LPURL_COMPONENTS%;

#pragma warning( default : 4121 )   // restore alignment warning

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

;begin_internal

#ifdef __WINCRYPT_H__
#ifdef ALGIDDEF
//
// INTERNET_SECURITY_INFO - contains information about certificate
// and encryption settings for a connection.
//

#define INTERNET_SECURITY_INFO_DEFINED

typedef struct {

    //
    // dwSize - Size of INTERNET_SECURITY_INFO structure.
    //

    DWORD dwSize;


    //
    // pCertificate - Cert context pointing to leaf of certificate chain.
    //

    PCCERT_CONTEXT pCertificate;

    //
    // Start SecPkgContext_ConnectionInfo
    // The following members must match those
    // of the SecPkgContext_ConnectionInfo
    // sspi structure (schnlsp.h)
    //


    //
    // dwProtocol - Protocol that this connection was made with
    //  (PCT, SSL2, SSL3, etc)
    //

    DWORD dwProtocol;

    //
    // aiCipher - Cipher that this connection as made with
    //

    ALG_ID aiCipher;

    //
    // dwCipherStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwCipherStrength;

    //
    // aiHash - Hash that this connection as made with
    //

    ALG_ID aiHash;

    //
    // dwHashStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwHashStrength;

    //
    // aiExch - Key Exchange type that this connection as made with
    //

    ALG_ID aiExch;

    //
    // dwExchStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwExchStrength;


} INTERNET_SECURITY_INFO, * LPINTERNET_SECURITY_INFO;


typedef struct {
    //
    // dwSize - size of INTERNET_SECURITY_CONNECTION_INFO
    //
    DWORD dwSize;

    // fSecure - Is this a secure connection.
    BOOL fSecure;

    //
    // dwProtocol - Protocol that this connection was made with
    //  (PCT, SSL2, SSL3, etc)
    //

    DWORD dwProtocol;

    //
    // aiCipher - Cipher that this connection as made with
    //

    ALG_ID aiCipher;

    //
    // dwCipherStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwCipherStrength;

    //
    // aiHash - Hash that this connection as made with
    //

    ALG_ID aiHash;

    //
    // dwHashStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwHashStrength;

    //
    // aiExch - Key Exchange type that this connection as made with
    //

    ALG_ID aiExch;

    //
    // dwExchStrength - Strength (in bits) that this connection
    //  was made with;
    //

    DWORD dwExchStrength;

} INTERNET_SECURITY_CONNECTION_INFO , * LPINTERNET_SECURITY_CONNECTION_INFO;


INTERNETAPI
BOOL
WINAPI
InternetAlgIdToString%(
    IN ALG_ID                         ai,
    IN LPTSTR%                        lpstr,
    IN OUT LPDWORD                    lpdwBufferLength,
    IN DWORD                          dwReserved
    );

INTERNETAPI
BOOL
WINAPI
InternetSecurityProtocolToString%(
    IN DWORD                          dwProtocol,
    IN LPTSTR%                        lpstr,
    IN OUT LPDWORD                    lpdwBufferLength,
    IN DWORD                          dwReserved
    );

#endif // ALGIDDEF
#endif // __WINCRYPT_H__

#ifdef INTERNET_SECURITY_INFO_DEFINED

INTERNETAPI
DWORD
WINAPI
ShowSecurityInfo(
    IN HWND                            hWndParent,
    IN LPINTERNET_SECURITY_INFO        pSecurityInfo
    );
#endif // INTERNET_SECURITY_INFO_DEFINED



INTERNETAPI
DWORD
WINAPI
ShowX509EncodedCertificate(
    IN HWND    hWndParent,
    IN LPBYTE  lpCert,
    IN DWORD   cbCert
    );

INTERNETAPI
DWORD
WINAPI
ShowClientAuthCerts(
    IN HWND hWndParent
    );

INTERNETAPI
DWORD
WINAPI
ParseX509EncodedCertificateForListBoxEntry(
    IN LPBYTE  lpCert,
    IN DWORD   cbCert,
    OUT LPSTR  lpszListBoxEntry,
    IN LPDWORD lpdwListBoxEntry
    );

//
// This is a private API for Trident.  It displays
// security info based on a URL
//

INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURLA(
    IN       LPSTR    lpszURL,
    IN       HWND     hwndParent
    );

INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURLW(
    IN       LPWSTR    lpszURL,
    IN       HWND     hwndParent
    );

#ifdef UNICODE
#define InternetShowSecurityInfoByURL  InternetShowSecurityInfoByURLW
#else
#ifdef _WINX32_
#define InternetShowSecurityInfoByURL  InternetShowSecurityInfoByURLA
#else
INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURL(
    IN       LPSTR    lpszURL,
    IN       HWND     hwndParent
    );
#endif // _WINX32_
#endif // !UNICODE

//Fortezza related exports. not public

// The commands that InternetFortezzaCommand supports.

typedef enum {
    FORTCMD_LOGON                   = 1,
    FORTCMD_LOGOFF                  = 2,
    FORTCMD_CHG_PERSONALITY         = 3,
} FORTCMD;

    
INTERNETAPI
BOOL
WINAPI
InternetFortezzaCommand(DWORD dwCommand, HWND hwnd, DWORD_PTR dwReserved);


typedef enum {
    FORTSTAT_INSTALLED          = 0x00000001,
    FORTSTAT_LOGGEDON           = 0x00000002,
}   FORTSTAT ; 

INTERNETAPI
BOOL
WINAPI
InternetQueryFortezzaStatus(DWORD *pdwStatus, DWORD_PTR dwReserved);


;end_internal

//
// INTERNET_BUFFERS - combines headers and data. May be chained for e.g. file
// upload or scatter/gather operations. For chunked read/write, lpcszHeader
// contains the chunked-ext
//

typedef struct _INTERNET_BUFFERS% {
    DWORD dwStructSize;                 // used for API versioning. Set to sizeof(INTERNET_BUFFERS)
    struct _INTERNET_BUFFERS% * Next;   // chain of buffers
    LPCTSTR% lpcszHeader;               // pointer to headers (may be NULL)
    DWORD dwHeadersLength;              // length of headers if not NULL
    DWORD dwHeadersTotal;               // size of headers if not enough buffer
    LPVOID lpvBuffer;                   // pointer to data buffer (may be NULL)
    DWORD dwBufferLength;               // length of data buffer if not NULL
    DWORD dwBufferTotal;                // total size of chunk, or content-length if not chunked
    DWORD dwOffsetLow;                  // used for read-ranges (only used in HttpSendRequest2)
    DWORD dwOffsetHigh;
} INTERNET_BUFFERS%, * LPINTERNET_BUFFERS%;

//
// prototypes
//

BOOLAPI
InternetTimeFromSystemTimeA(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format
    OUT LPSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    );

BOOLAPI
InternetTimeFromSystemTimeW(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format
    OUT LPWSTR lpszTime,        // output string buffer
    IN  DWORD cbTime            // output buffer size
    );

#ifdef UNICODE
#define InternetTimeFromSystemTime  InternetTimeFromSystemTimeW
#else
#ifdef _WINX32_
#define InternetTimeFromSystemTime  InternetTimeFromSystemTimeA
#else
BOOLAPI
InternetTimeFromSystemTime(
    IN  CONST SYSTEMTIME *pst,  // input GMT time
    IN  DWORD dwRFC,            // RFC format
    OUT LPSTR lpszTime,         // output string buffer
    IN  DWORD cbTime            // output buffer size
    );
#endif // _WINX32_
#endif // !UNICODE

//
// constants for InternetTimeFromSystemTime
//

#define INTERNET_RFC1123_FORMAT     0
#define INTERNET_RFC1123_BUFSIZE   30

BOOLAPI
InternetTimeToSystemTimeA(
    IN  LPCSTR lpszTime,         // NULL terminated string
    OUT SYSTEMTIME *pst,         // output in GMT time
    IN  DWORD dwReserved
    );

BOOLAPI
InternetTimeToSystemTimeW(
    IN  LPCWSTR lpszTime,        // NULL terminated string
    OUT SYSTEMTIME *pst,         // output in GMT time
    IN  DWORD dwReserved
    );

#ifdef UNICODE
#define InternetTimeToSystemTime  InternetTimeToSystemTimeW
#else
#ifdef _WINX32_
#define InternetTimeToSystemTime  InternetTimeToSystemTimeA
#else
BOOLAPI
InternetTimeToSystemTime(
    IN  LPCSTR lpszTime,         // NULL terminated string
    OUT SYSTEMTIME *pst,         // output in GMT time
    IN  DWORD dwReserved
    );
#endif // _WINX32_
#endif // !UNICODE

;begin_internal

BOOLAPI
InternetDebugGetLocalTime(
    OUT SYSTEMTIME * pstLocalTime,
    OUT DWORD      * pdwReserved
    );

;end_internal

BOOLAPI
InternetCrackUrl%(
    IN LPCTSTR% lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTS% lpUrlComponents
    );

BOOLAPI
InternetCreateUrl%(
    IN LPURL_COMPONENTS% lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPTSTR% lpszUrl,
    IN OUT LPDWORD lpdwUrlLength
    );

BOOLAPI
InternetCanonicalizeUrl%(
    IN LPCTSTR% lpszUrl,
    OUT LPTSTR% lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );

BOOLAPI
InternetCombineUrl%(
    IN LPCTSTR% lpszBaseUrl,
    IN LPCTSTR% lpszRelativeUrl,
    OUT LPTSTR% lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );

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
#define ICU_ENCODE_PERCENT      0x00001000      // Encode any percent (ASCII25)
        // signs encountered, default is to not encode percent.

INTERNETAPI
HINTERNET
WINAPI
InternetOpen%(
    IN LPCTSTR% lpszAgent,
    IN DWORD dwAccessType,
    IN LPCTSTR% lpszProxy OPTIONAL,
    IN LPCTSTR% lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    );

//
// access types for InternetOpen()
//

#define INTERNET_OPEN_TYPE_PRECONFIG                    0   // use registry configuration
#define INTERNET_OPEN_TYPE_DIRECT                       1   // direct to net
#define INTERNET_OPEN_TYPE_PROXY                        3   // via named proxy
#define INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY  4   // prevent using java/script/INS

//
// old names for access types
//

#define PRE_CONFIG_INTERNET_ACCESS  INTERNET_OPEN_TYPE_PRECONFIG
#define LOCAL_INTERNET_ACCESS       INTERNET_OPEN_TYPE_DIRECT
#define CERN_PROXY_INTERNET_ACCESS  INTERNET_OPEN_TYPE_PROXY

BOOLAPI
InternetCloseHandle(
    IN HINTERNET hInternet
    );

INTERNETAPI
HINTERNET
WINAPI
InternetConnect%(
    IN HINTERNET hInternet,
    IN LPCTSTR% lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCTSTR% lpszUserName OPTIONAL,
    IN LPCTSTR% lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

//
// service types for InternetConnect()
//

;begin_internal
#define INTERNET_SERVICE_URL    0
;end_internal
#define INTERNET_SERVICE_FTP    1
#define INTERNET_SERVICE_GOPHER 2
#define INTERNET_SERVICE_HTTP   3

;begin_internal
//
// InternetConnectUrl() - a macro which allows you to specify an URL instead of
// the component parts to InternetConnect(). If any API which uses the returned
// connect handle specifies a NULL path then the URL-path part of the URL
// specified in InternetConnectUrl() will be used
//

#define InternetConnectUrl(hInternet, lpszUrl, dwFlags, dwContext) \
    InternetConnect(hInternet,                      \
                    lpszUrl,                        \
                    INTERNET_INVALID_PORT_NUMBER,   \
                    NULL,                           \
                    NULL,                           \
                    INTERNET_SERVICE_URL,           \
                    dwFlags,                        \
                    dwContext                       \
                    )
;end_internal

INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrl%(
    IN HINTERNET hInternet,
    IN LPCTSTR% lpszUrl,
    IN LPCTSTR% lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
InternetReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    );

INTERNETAPI
BOOL
WINAPI
InternetReadFileEx%(
    IN HINTERNET hFile,
    OUT LPINTERNET_BUFFERS% lpBuffersOut,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

//
// flags for InternetReadFileEx()
//

#define IRF_ASYNC       WININET_API_FLAG_ASYNC
#define IRF_SYNC        WININET_API_FLAG_SYNC
#define IRF_USE_CONTEXT WININET_API_FLAG_USE_CONTEXT
#define IRF_NO_WAIT     0x00000008

INTERNETAPI
DWORD
WINAPI
InternetSetFilePointer(
    IN HINTERNET hFile,
    IN LONG  lDistanceToMove,
    IN PVOID pReserved,
    IN DWORD dwMoveMethod,
    IN DWORD_PTR dwContext
    );

BOOLAPI
InternetWriteFile(
    IN HINTERNET hFile,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    );

;begin_internal

INTERNETAPI
BOOL
WINAPI
InternetWriteFileEx%(
    IN HINTERNET hFile,
    IN LPINTERNET_BUFFERS% lpBuffersIn,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

;end_internal

BOOLAPI
InternetQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
InternetFindNextFile%(
    IN HINTERNET hFind,
    OUT LPVOID lpvFindData
    );

BOOLAPI
InternetQueryOption%(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );

BOOLAPI
InternetSetOption%(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    );

BOOLAPI
InternetSetOptionEx%(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags
    );

BOOLAPI
InternetLockRequestFile(
    IN  HINTERNET hInternet,
    OUT HANDLE * lphLockRequestInfo
    );

BOOLAPI
InternetUnlockRequestFile(
    IN HANDLE hLockRequestInfo
    );

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
;begin_internal
#define INTERNET_OPTION_CONTEXT_VALUE_OLD       10
;end_internal
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
#define INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT 32
#define INTERNET_OPTION_DATAFILE_NAME           33
#define INTERNET_OPTION_URL                     34
#define INTERNET_OPTION_SECURITY_CERTIFICATE    35
#define INTERNET_OPTION_SECURITY_KEY_BITNESS    36
#define INTERNET_OPTION_REFRESH                 37
#define INTERNET_OPTION_PROXY                   38
#define INTERNET_OPTION_SETTINGS_CHANGED        39
#define INTERNET_OPTION_VERSION                 40
#define INTERNET_OPTION_USER_AGENT              41
#define INTERNET_OPTION_END_BROWSER_SESSION     42
#define INTERNET_OPTION_PROXY_USERNAME          43
#define INTERNET_OPTION_PROXY_PASSWORD          44
#define INTERNET_OPTION_CONTEXT_VALUE           45
#define INTERNET_OPTION_CONNECT_LIMIT           46
#define INTERNET_OPTION_SECURITY_SELECT_CLIENT_CERT 47
#define INTERNET_OPTION_POLICY                  48
#define INTERNET_OPTION_DISCONNECTED_TIMEOUT    49
#define INTERNET_OPTION_CONNECTED_STATE         50
#define INTERNET_OPTION_IDLE_STATE              51
#define INTERNET_OPTION_OFFLINE_SEMANTICS       52
#define INTERNET_OPTION_SECONDARY_CACHE_KEY     53
#define INTERNET_OPTION_CALLBACK_FILTER         54
#define INTERNET_OPTION_CONNECT_TIME            55
#define INTERNET_OPTION_SEND_THROUGHPUT         56
#define INTERNET_OPTION_RECEIVE_THROUGHPUT      57
#define INTERNET_OPTION_REQUEST_PRIORITY        58
#define INTERNET_OPTION_HTTP_VERSION            59
#define INTERNET_OPTION_RESET_URLCACHE_SESSION  60
;begin_internal
#define INTERNET_OPTION_NET_SPEED               61
;end_internal
#define INTERNET_OPTION_ERROR_MASK              62
#define INTERNET_OPTION_FROM_CACHE_TIMEOUT      63
#define INTERNET_OPTION_BYPASS_EDITED_ENTRY     64
;begin_internal
// Pass in pointer to INTERNET_SECURITY_CONNECTION_INFO to be filled in.
#define INTERNET_OPTION_SECURITY_CONNECTION_INFO  66
#define INTERNET_OPTION_DIAGNOSTIC_SOCKET_INFO  67
;end_internal
#define INTERNET_OPTION_CODEPAGE                68
#define INTERNET_OPTION_CACHE_TIMESTAMPS        69
#define INTERNET_OPTION_DISABLE_AUTODIAL        70
;begin_internal
#define INTERNET_OPTION_DETECT_POST_SEND        71
#define INTERNET_OPTION_DISABLE_NTLM_PREAUTH    72
;end_internal
#define INTERNET_OPTION_MAX_CONNS_PER_SERVER     73
#define INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER 74
#define INTERNET_OPTION_PER_CONNECTION_OPTION   75
#define INTERNET_OPTION_DIGEST_AUTH_UNLOAD             76
#define INTERNET_OPTION_IGNORE_OFFLINE           77

#define INTERNET_FIRST_OPTION                   INTERNET_OPTION_CALLBACK
#define INTERNET_LAST_OPTION                    INTERNET_OPTION_IGNORE_OFFLINE
;begin_internal
#define INTERNET_LAST_OPTION_INTERNAL           INTERNET_OPTION_IGNORE_OFFLINE
;end_internal


;begin_internal

#define INTERNET_OPTION_OFFLINE_TIMEOUT INTERNET_OPTION_DISCONNECTED_TIMEOUT
#define INTERNET_OPTION_LINE_STATE      INTERNET_OPTION_CONNECTED_STATE

;end_internal

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
#define INTERNET_HANDLE_TYPE_FILE_REQUEST       14

//
// values for INTERNET_OPTION_SECURITY_FLAGS
//

// query only
#define SECURITY_FLAG_SECURE                    0x00000001 // can query only
#define SECURITY_FLAG_STRENGTH_WEAK             0x10000000
#define SECURITY_FLAG_STRENGTH_MEDIUM           0x40000000
#define SECURITY_FLAG_STRENGTH_STRONG           0x20000000
#define SECURITY_FLAG_UNKNOWNBIT                0x80000000
#define SECURITY_FLAG_FORTEZZA                  0x08000000
#define SECURITY_FLAG_NORMALBITNESS             SECURITY_FLAG_STRENGTH_WEAK



// The following are unused
#define SECURITY_FLAG_SSL                       0x00000002
#define SECURITY_FLAG_SSL3                      0x00000004
#define SECURITY_FLAG_PCT                       0x00000008
#define SECURITY_FLAG_PCT4                      0x00000010
#define SECURITY_FLAG_IETFSSL4                  0x00000020

// The following are for backwards compatability only.
#define SECURITY_FLAG_40BIT                     SECURITY_FLAG_STRENGTH_WEAK
#define SECURITY_FLAG_128BIT                    SECURITY_FLAG_STRENGTH_STRONG
#define SECURITY_FLAG_56BIT                     SECURITY_FLAG_STRENGTH_MEDIUM

// setable flags
#define SECURITY_FLAG_IGNORE_REVOCATION         0x00000080
#define SECURITY_FLAG_IGNORE_UNKNOWN_CA         0x00000100
#define SECURITY_FLAG_IGNORE_WRONG_USAGE        0x00000200

#define SECURITY_FLAG_IGNORE_CERT_CN_INVALID    INTERNET_FLAG_IGNORE_CERT_CN_INVALID
#define SECURITY_FLAG_IGNORE_CERT_DATE_INVALID  INTERNET_FLAG_IGNORE_CERT_DATE_INVALID


#define SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTPS  INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS
#define SECURITY_FLAG_IGNORE_REDIRECT_TO_HTTP   INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP



#define SECURITY_SET_MASK       (SECURITY_FLAG_IGNORE_REVOCATION |\
                                 SECURITY_FLAG_IGNORE_UNKNOWN_CA |\
                                 SECURITY_FLAG_IGNORE_CERT_CN_INVALID |\
                                 SECURITY_FLAG_IGNORE_CERT_DATE_INVALID |\
                                 SECURITY_FLAG_IGNORE_WRONG_USAGE)



BOOLAPI
InternetGetLastResponseInfo%(
    OUT LPDWORD lpdwError,
    OUT LPTSTR% lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );

//
// callback function for InternetSetStatusCallback
//

typedef
VOID
(CALLBACK * INTERNET_STATUS_CALLBACK)(
    IN HINTERNET hInternet,
    IN DWORD_PTR dwContext,
    IN DWORD dwInternetStatus,
    IN LPVOID lpvStatusInformation OPTIONAL,
    IN DWORD dwStatusInformationLength
    );

typedef INTERNET_STATUS_CALLBACK * LPINTERNET_STATUS_CALLBACK;

INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallbackA(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    );

INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallbackW(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    );

#ifdef UNICODE
#define InternetSetStatusCallback  InternetSetStatusCallbackW
#else
#ifdef _WINX32_
#define InternetSetStatusCallback  InternetSetStatusCallbackA
#else
INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallback(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    );
#endif // _WINX32_
#endif // !UNICODE

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
#define INTERNET_STATUS_DETECTING_PROXY         80
#define INTERNET_STATUS_REQUEST_COMPLETE        100
#define INTERNET_STATUS_REDIRECT                110
#define INTERNET_STATUS_INTERMEDIATE_RESPONSE   120
#define INTERNET_STATUS_USER_INPUT_REQUIRED     140
#define INTERNET_STATUS_STATE_CHANGE            200

//
// the following can be indicated in a state change notification:
//

#define INTERNET_STATE_CONNECTED                0x00000001  // connected state (mutually exclusive with disconnected)
#define INTERNET_STATE_DISCONNECTED             0x00000002  // disconnected from network
#define INTERNET_STATE_DISCONNECTED_BY_USER     0x00000010  // disconnected by user request
#define INTERNET_STATE_IDLE                     0x00000100  // no network requests being made (by Wininet)
#define INTERNET_STATE_BUSY                     0x00000200  // network requests being made (by Wininet)

;begin_internal

#define INTERNET_STATE_ONLINE       INTERNET_STATE_CONNECTED
#define INTERNET_STATE_OFFLINE      INTERNET_STATE_DISCONNECTED
#define INTERNET_STATE_OFFLINE_USER INTERNET_STATE_DISCONNECTED_BY_USER
#define INTERNET_LINE_STATE_MASK    (INTERNET_STATE_ONLINE | INTERNET_STATE_OFFLINE)
#define INTERNET_BUSY_STATE_MASK    (INTERNET_STATE_IDLE | INTERNET_STATE_BUSY)

;end_internal

;begin_internal

//
// the following are used with InternetSetOption(..., INTERNET_OPTION_CALLBACK_FILTER, ...)
// to filter out unrequired callbacks. INTERNET_STATUS_REQUEST_COMPLETE cannot
// be filtered out
//

#define INTERNET_STATUS_FILTER_RESOLVING        0x00000001
#define INTERNET_STATUS_FILTER_RESOLVED         0x00000002
#define INTERNET_STATUS_FILTER_CONNECTING       0x00000004
#define INTERNET_STATUS_FILTER_CONNECTED        0x00000008
#define INTERNET_STATUS_FILTER_SENDING          0x00000010
#define INTERNET_STATUS_FILTER_SENT             0x00000020
#define INTERNET_STATUS_FILTER_RECEIVING        0x00000040
#define INTERNET_STATUS_FILTER_RECEIVED         0x00000080
#define INTERNET_STATUS_FILTER_CLOSING          0x00000100
#define INTERNET_STATUS_FILTER_CLOSED           0x00000200
#define INTERNET_STATUS_FILTER_HANDLE_CREATED   0x00000400
#define INTERNET_STATUS_FILTER_HANDLE_CLOSING   0x00000800
#define INTERNET_STATUS_FILTER_PREFETCH         0x00001000
#define INTERNET_STATUS_FILTER_REDIRECT         0x00002000
#define INTERNET_STATUS_FILTER_STATE_CHANGE     0x00004000

;end_internal

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
FtpFindFirstFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszSearchFile OPTIONAL,
    OUT LPWIN32_FIND_DATA% lpFindFileData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
FtpGetFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszRemoteFile,
    IN LPCTSTR% lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
FtpPutFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszLocalFile,
    IN LPCTSTR% lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
FtpGetFileEx(
    IN HINTERNET hFtpSession,
    IN LPCSTR lpszRemoteFile,
    IN LPCWSTR lpszNewFile,
    IN BOOL fFailIfExists,
    IN DWORD dwFlagsAndAttributes,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI    
FtpPutFileEx(
    IN HINTERNET hFtpSession,
    IN LPCWSTR lpszLocalFile,
    IN LPCSTR lpszNewRemoteFile,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
FtpDeleteFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszFileName
    );

BOOLAPI
FtpRenameFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszExisting,
    IN LPCTSTR% lpszNew
    );

INTERNETAPI
HINTERNET
WINAPI
FtpOpenFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszFileName,
    IN DWORD dwAccess,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
FtpCreateDirectory%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszDirectory
    );

BOOLAPI
FtpRemoveDirectory%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszDirectory
    );

BOOLAPI
FtpSetCurrentDirectory%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszDirectory
    );

BOOLAPI
FtpGetCurrentDirectory%(
    IN HINTERNET hConnect,
    OUT LPTSTR% lpszCurrentDirectory,
    IN OUT LPDWORD lpdwCurrentDirectory
    );

BOOLAPI
FtpCommand%(
    IN HINTERNET hConnect,
    IN BOOL fExpectResponse,
    IN DWORD dwFlags,
    IN LPCTSTR% lpszCommand,
    IN DWORD_PTR dwContext,
    OUT HINTERNET *phFtpCommand OPTIONAL
    );

INTERNETAPI
DWORD
WINAPI
FtpGetFileSize(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwFileSizeHigh OPTIONAL
    );



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
    TCHAR% DisplayString[MAX_GOPHER_DISPLAY_TEXT + 1];
    DWORD GopherType;   // GOPHER_TYPE_, if known
    DWORD SizeLow;
    DWORD SizeHigh;
    FILETIME LastModificationTime;
    TCHAR% Locator[MAX_GOPHER_LOCATOR_LENGTH + 1];
} GOPHER_FIND_DATA%, * LPGOPHER_FIND_DATA%;

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

BOOLAPI
GopherCreateLocator%(
    IN LPCTSTR% lpszHost,
    IN INTERNET_PORT nServerPort,
    IN LPCTSTR% lpszDisplayString OPTIONAL,
    IN LPCTSTR% lpszSelectorString OPTIONAL,
    IN DWORD dwGopherType,
    OUT LPTSTR% lpszLocator OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    );

BOOLAPI
GopherGetLocatorType%(
    IN LPCTSTR% lpszLocator,
    OUT LPDWORD lpdwGopherType
    );

INTERNETAPI
HINTERNET
WINAPI
GopherFindFirstFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszLocator OPTIONAL,
    IN LPCTSTR% lpszSearchString OPTIONAL,
    OUT LPGOPHER_FIND_DATA% lpFindData OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

INTERNETAPI
HINTERNET
WINAPI
GopherOpenFile%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszLocator,
    IN LPCTSTR% lpszView OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

typedef
BOOL
(CALLBACK * GOPHER_ATTRIBUTE_ENUMERATOR)(
    LPGOPHER_ATTRIBUTE_TYPE lpAttributeInfo,
    DWORD dwError
    );

BOOLAPI
GopherGetAttribute%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszLocator,
    IN LPCTSTR% lpszAttributeName OPTIONAL,
    OUT LPBYTE lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwCharactersReturned,
    IN GOPHER_ATTRIBUTE_ENUMERATOR lpfnEnumerator OPTIONAL,
    IN DWORD_PTR dwContext
    );

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

#define HTTP_VERSIONA           "HTTP/1.0"
#define HTTP_VERSIONW           L"HTTP/1.0"

#ifdef UNICODE
#define HTTP_VERSION            HTTP_VERSIONW
#else
#define HTTP_VERSION            HTTP_VERSIONA
#endif

//
// HttpQueryInfo info levels. Generally, there is one info level
// for each potential RFC822/HTTP/MIME header that an HTTP server
// may send as part of a request response.
//
// The HTTP_QUERY_RAW_HEADERS info level is provided for clients
// that choose to perform their own header parsing.
//

;begin_internal

//
// Note that adding any HTTP_QUERY_* codes here must be followed
//   by an equivlent line in wininet\http\hashgen\hashgen.cpp
//   please see that file for further information regarding
//   the addition of new HTTP headers
//

;end_internal


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
#define HTTP_QUERY_REFRESH                      46
#define HTTP_QUERY_CONTENT_DISPOSITION          47

//
// HTTP 1.1 defined headers
//

#define HTTP_QUERY_AGE                          48
#define HTTP_QUERY_CACHE_CONTROL                49
#define HTTP_QUERY_CONTENT_BASE                 50
#define HTTP_QUERY_CONTENT_LOCATION             51
#define HTTP_QUERY_CONTENT_MD5                  52
#define HTTP_QUERY_CONTENT_RANGE                53
#define HTTP_QUERY_ETAG                         54
#define HTTP_QUERY_HOST                         55
#define HTTP_QUERY_IF_MATCH                     56
#define HTTP_QUERY_IF_NONE_MATCH                57
#define HTTP_QUERY_IF_RANGE                     58
#define HTTP_QUERY_IF_UNMODIFIED_SINCE          59
#define HTTP_QUERY_MAX_FORWARDS                 60
#define HTTP_QUERY_PROXY_AUTHORIZATION          61
#define HTTP_QUERY_RANGE                        62
#define HTTP_QUERY_TRANSFER_ENCODING            63
#define HTTP_QUERY_UPGRADE                      64
#define HTTP_QUERY_VARY                         65
#define HTTP_QUERY_VIA                          66
#define HTTP_QUERY_WARNING                      67
#define HTTP_QUERY_EXPECT                       68
#define HTTP_QUERY_PROXY_CONNECTION             69
#define HTTP_QUERY_UNLESS_MODIFIED_SINCE        70


;begin_internal

// These are not part of HTTP 1.1 yet. We will propose these to the
// HTTP extensions working group. These are required for the client-caps support
// we are doing in conjuntion with IIS.

;end_internal

#define HTTP_QUERY_ECHO_REQUEST                 71
#define HTTP_QUERY_ECHO_REPLY                   72

// These are the set of headers that should be added back to a request when
// re-doing a request after a RETRY_WITH response.
#define HTTP_QUERY_ECHO_HEADERS                 73
#define HTTP_QUERY_ECHO_HEADERS_CRLF            74

#define HTTP_QUERY_MAX                          74

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

#define HTTP_STATUS_CONTINUE            100 // OK to continue with request
#define HTTP_STATUS_SWITCH_PROTOCOLS    101 // server has switched protocols in upgrade header

#define HTTP_STATUS_OK                  200 // request completed
#define HTTP_STATUS_CREATED             201 // object created, reason = new URI
#define HTTP_STATUS_ACCEPTED            202 // async completion (TBS)
#define HTTP_STATUS_PARTIAL             203 // partial completion
#define HTTP_STATUS_NO_CONTENT          204 // no info to return
#define HTTP_STATUS_RESET_CONTENT       205 // request completed, but clear form
#define HTTP_STATUS_PARTIAL_CONTENT     206 // partial GET furfilled

#define HTTP_STATUS_AMBIGUOUS           300 // server couldn't decide what to return
#define HTTP_STATUS_MOVED               301 // object permanently moved
#define HTTP_STATUS_REDIRECT            302 // object temporarily moved
#define HTTP_STATUS_REDIRECT_METHOD     303 // redirection w/ new access method
#define HTTP_STATUS_NOT_MODIFIED        304 // if-modified-since was not modified
#define HTTP_STATUS_USE_PROXY           305 // redirection to proxy, location header specifies proxy to use
#define HTTP_STATUS_REDIRECT_KEEP_VERB  307 // HTTP/1.1: keep same verb

#define HTTP_STATUS_BAD_REQUEST         400 // invalid syntax
#define HTTP_STATUS_DENIED              401 // access denied
#define HTTP_STATUS_PAYMENT_REQ         402 // payment required
#define HTTP_STATUS_FORBIDDEN           403 // request forbidden
#define HTTP_STATUS_NOT_FOUND           404 // object not found
#define HTTP_STATUS_BAD_METHOD          405 // method is not allowed
#define HTTP_STATUS_NONE_ACCEPTABLE     406 // no response acceptable to client found
#define HTTP_STATUS_PROXY_AUTH_REQ      407 // proxy authentication required
#define HTTP_STATUS_REQUEST_TIMEOUT     408 // server timed out waiting for request
#define HTTP_STATUS_CONFLICT            409 // user should resubmit with more info
#define HTTP_STATUS_GONE                410 // the resource is no longer available
#define HTTP_STATUS_LENGTH_REQUIRED     411 // the server refused to accept request w/o a length
#define HTTP_STATUS_PRECOND_FAILED      412 // precondition given in request failed
#define HTTP_STATUS_REQUEST_TOO_LARGE   413 // request entity was too large
#define HTTP_STATUS_URI_TOO_LONG        414 // request URI too long
#define HTTP_STATUS_UNSUPPORTED_MEDIA   415 // unsupported media type
#define HTTP_STATUS_RETRY_WITH          449 // retry after doing the appropriate action.

#define HTTP_STATUS_SERVER_ERROR        500 // internal server error
#define HTTP_STATUS_NOT_SUPPORTED       501 // required not supported
#define HTTP_STATUS_BAD_GATEWAY         502 // error response received from gateway
#define HTTP_STATUS_SERVICE_UNAVAIL     503 // temporarily overloaded
#define HTTP_STATUS_GATEWAY_TIMEOUT     504 // timed out waiting for gateway
#define HTTP_STATUS_VERSION_NOT_SUP     505 // HTTP version not supported

#define HTTP_STATUS_FIRST               HTTP_STATUS_CONTINUE
#define HTTP_STATUS_LAST                HTTP_STATUS_VERSION_NOT_SUP

//
// prototypes
//

INTERNETAPI
HINTERNET
WINAPI
HttpOpenRequest%(
    IN HINTERNET hConnect,
    IN LPCTSTR% lpszVerb,
    IN LPCTSTR% lpszObjectName,
    IN LPCTSTR% lpszVersion,
    IN LPCTSTR% lpszReferrer OPTIONAL,
    IN LPCTSTR% FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
HttpAddRequestHeaders%(
    IN HINTERNET hRequest,
    IN LPCTSTR% lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwModifiers
    );

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

BOOLAPI
HttpSendRequest%(
    IN HINTERNET hRequest,
    IN LPCTSTR% lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    );

INTERNETAPI
BOOL
WINAPI
HttpSendRequestEx%(
    IN HINTERNET hRequest,
    IN LPINTERNET_BUFFERS% lpBuffersIn OPTIONAL,
    OUT LPINTERNET_BUFFERS% lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

//
// flags for HttpSendRequestEx(), HttpEndRequest()
//

#define HSR_ASYNC       WININET_API_FLAG_ASYNC          // force async
#define HSR_SYNC        WININET_API_FLAG_SYNC           // force sync
#define HSR_USE_CONTEXT WININET_API_FLAG_USE_CONTEXT    // use dwContext value
#define HSR_INITIATE    0x00000008                      // iterative operation (completed by HttpEndRequest)
#define HSR_DOWNLOAD    0x00000010                      // download to file
#define HSR_CHUNKED     0x00000020                      // operation is send of chunked data

INTERNETAPI
BOOL
WINAPI
HttpEndRequest%(
    IN HINTERNET hRequest,
    OUT LPINTERNET_BUFFERS% lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    );

BOOLAPI
HttpQueryInfo%(
    IN HINTERNET hRequest,
    IN DWORD dwInfoLevel,
    IN OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex OPTIONAL
    );

//
// Cookie APIs
//

;begin_internal
typedef struct _INTERNET_COOKIE {
    DWORD cbSize;
    LPSTR pszName;
    LPSTR pszData;
    LPSTR pszDomain;
    LPSTR pszPath;
    FILETIME *pftExpires;
    DWORD dwFlags;
    LPSTR pszUrl;
} INTERNET_COOKIE, *PINTERNET_COOKIE;

#define INTERNET_COOKIE_IS_SECURE   0x01
#define INTERNET_COOKIE_IS_SESSION  0x02

;end_internal

BOOLAPI
InternetSetCookie%(
    IN LPCTSTR% lpszUrl,
    IN LPCTSTR% lpszCookieName,
    IN LPCTSTR% lpszCookieData
    );

BOOLAPI
InternetGetCookie%(
    IN LPCTSTR% lpszUrl,
    IN LPCTSTR% lpszCookieName,
    OUT LPTSTR% lpCookieData,
    IN OUT LPDWORD lpdwSize
    );

//
// offline browsing
//

INTERNETAPI
DWORD
WINAPI
InternetAttemptConnect(
    IN DWORD dwReserved
    );

BOOLAPI
InternetCheckConnection%(
    IN LPCTSTR% lpszUrl,
    IN DWORD dwFlags,
    IN DWORD dwReserved
    );

;begin_internal
//
// DAV Detection
//
BOOLAPI
HttpCheckDavCompliance%(
    IN LPCTSTR% lpszUrl,
    IN LPCTSTR% lpszComplianceToken,
    IN OUT LPBOOL lpfFound,
    IN HWND hWnd,
    IN LPVOID lpvReserved
    );

BOOLAPI
HttpCheckCachedDavStatus%(
    IN LPCTSTR% lpszUrl,
    IN OUT LPDWORD lpdwStatus
    );

BOOLAPI
HttpCheckDavCollection%(
    IN LPCTSTR% lpszUrl,
    IN OUT LPBOOL lpfFound,
    IN HWND hWnd,
    IN LPVOID lpvReserved
    );

// DAV detection defines
#define DAV_LEVEL1_STATUS               0x00000001
#define DAV_COLLECTION_STATUS           0x00004000
#define DAV_DETECTION_REQUIRED          0x00008000
;end_internal
    

#define FLAG_ICC_FORCE_CONNECTION       0x00000001

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
#define FLAGS_ERROR_UI_SERIALIZE_DIALOGS        0x10

//
// If SERIALIZE_DIALOGS flag set, client should implement thread-safe non-blocking callback...
//

DWORD InternetAuthNotifyCallback
(
    DWORD_PTR       dwContext,    // as passed to InternetErrorDlg
    DWORD           dwReturn,     // error code: success, resend, or cancel
    LPVOID          lpReserved    // reserved: will be set to null
);
typedef DWORD (CALLBACK * PFN_AUTH_NOTIFY) (DWORD_PTR, DWORD, LPVOID);

//
// ... and last parameter of InternetErrorDlg should point to...
//

typedef struct
{
    DWORD            cbStruct;    // size of this structure
    DWORD            dwOptions;   // reserved: must set to 0
    PFN_AUTH_NOTIFY  pfnNotify;   // notification callback to retry InternetErrorDlg
    DWORD_PTR        dwContext;   // context to pass to to notification function
}
    INTERNET_AUTH_NOTIFY_DATA;


INTERNETAPI
BOOL
WINAPI
ResumeSuspendedDownload(
    IN HINTERNET hRequest,
    IN DWORD dwResultCode
    );

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
InternetConfirmZoneCrossingA(
    IN HWND hWnd,
    IN LPSTR szUrlPrev,
    IN LPSTR szUrlNew,
    IN BOOL bPost
    );

INTERNETAPI
DWORD
WINAPI
InternetConfirmZoneCrossingW(
    IN HWND hWnd,
    IN LPWSTR szUrlPrev,
    IN LPWSTR szUrlNew,
    IN BOOL bPost
    );

#ifdef UNICODE
#define InternetConfirmZoneCrossing  InternetConfirmZoneCrossingW
#else
#ifdef _WINX32_
#define InternetConfirmZoneCrossing  InternetConfirmZoneCrossingA
#else
INTERNETAPI
DWORD
WINAPI
InternetConfirmZoneCrossing(
    IN HWND hWnd,
    IN LPSTR szUrlPrev,
    IN LPSTR szUrlNew,
    IN BOOL bPost
    );
#endif // _WINX32_
#endif // !UNICODE

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
#define ERROR_INTERNET_DIALOG_PENDING           (INTERNET_ERROR_BASE + 49)
#define ERROR_INTERNET_RETRY_DIALOG             (INTERNET_ERROR_BASE + 50)
;begin_internal
#define ERROR_INTERNET_NO_NEW_CONTAINERS        (INTERNET_ERROR_BASE + 51)
;end_internal
#define ERROR_INTERNET_HTTPS_HTTP_SUBMIT_REDIR  (INTERNET_ERROR_BASE + 52)
#define ERROR_INTERNET_INSERT_CDROM             (INTERNET_ERROR_BASE + 53)
#define ERROR_INTERNET_FORTEZZA_LOGIN_NEEDED    (INTERNET_ERROR_BASE + 54)
#define ERROR_INTERNET_SEC_CERT_ERRORS          (INTERNET_ERROR_BASE + 55)
#define ERROR_INTERNET_SEC_CERT_NO_REV          (INTERNET_ERROR_BASE + 56)
#define ERROR_INTERNET_SEC_CERT_REV_FAILED      (INTERNET_ERROR_BASE + 57)

//
// FTP API errors
//

#define ERROR_FTP_TRANSFER_IN_PROGRESS          (INTERNET_ERROR_BASE + 110)
#define ERROR_FTP_DROPPED                       (INTERNET_ERROR_BASE + 111)
#define ERROR_FTP_NO_PASSIVE_MODE               (INTERNET_ERROR_BASE + 112)

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
#define ERROR_HTTP_NOT_REDIRECTED               (INTERNET_ERROR_BASE + 160)
#define ERROR_HTTP_COOKIE_NEEDS_CONFIRMATION    (INTERNET_ERROR_BASE + 161)
#define ERROR_HTTP_COOKIE_DECLINED              (INTERNET_ERROR_BASE + 162)
#define ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION  (INTERNET_ERROR_BASE + 168)

//
// additional Internet API error codes
//

#define ERROR_INTERNET_SECURITY_CHANNEL_ERROR   (INTERNET_ERROR_BASE + 157)
#define ERROR_INTERNET_UNABLE_TO_CACHE_FILE     (INTERNET_ERROR_BASE + 158)
#define ERROR_INTERNET_TCPIP_NOT_INSTALLED      (INTERNET_ERROR_BASE + 159)
#define ERROR_INTERNET_DISCONNECTED             (INTERNET_ERROR_BASE + 163)
#define ERROR_INTERNET_SERVER_UNREACHABLE       (INTERNET_ERROR_BASE + 164)
#define ERROR_INTERNET_PROXY_SERVER_UNREACHABLE (INTERNET_ERROR_BASE + 165)

#define ERROR_INTERNET_BAD_AUTO_PROXY_SCRIPT    (INTERNET_ERROR_BASE + 166)
#define ERROR_INTERNET_UNABLE_TO_DOWNLOAD_SCRIPT (INTERNET_ERROR_BASE + 167)
#define ERROR_INTERNET_SEC_INVALID_CERT         (INTERNET_ERROR_BASE + 169)
#define ERROR_INTERNET_SEC_CERT_REVOKED         (INTERNET_ERROR_BASE + 170)

// InternetAutodial specific errors

#define ERROR_INTERNET_FAILED_DUETOSECURITYCHECK  (INTERNET_ERROR_BASE + 171)
#define ERROR_INTERNET_NOT_INITIALIZED          (INTERNET_ERROR_BASE + 172)
#define ERROR_INTERNET_NEED_MSN_SSPI_PKG          (INTERNET_ERROR_BASE + 173)
#define ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY   (INTERNET_ERROR_BASE + 174)


#define INTERNET_ERROR_LAST                     ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY

;begin_internal

#define ERROR_INTERNET_OFFLINE  ERROR_INTERNET_DISCONNECTED

//
// internal error codes that are used to communicate specific information inside
// of Wininet but which are meaningless at the interface
//

#define INTERNET_INTERNAL_ERROR_BASE            (INTERNET_ERROR_BASE + 900)

#define ERROR_INTERNET_INTERNAL_SOCKET_ERROR    (INTERNET_INTERNAL_ERROR_BASE + 1)
#define ERROR_INTERNET_CONNECTION_AVAILABLE     (INTERNET_INTERNAL_ERROR_BASE + 2)
#define ERROR_INTERNET_NO_KNOWN_SERVERS         (INTERNET_INTERNAL_ERROR_BASE + 3)
#define ERROR_INTERNET_PING_FAILED              (INTERNET_INTERNAL_ERROR_BASE + 4)
#define ERROR_INTERNET_NO_PING_SUPPORT          (INTERNET_INTERNAL_ERROR_BASE + 5)
#define ERROR_INTERNET_CACHE_SUCCESS            (INTERNET_INTERNAL_ERROR_BASE + 6)
;end_internal

//#endif // !defined(_WINERROR_)

//
// URLCACHE APIs
//

//
// datatype definitions.
//

//
// cache entry type flags.
//

#define NORMAL_CACHE_ENTRY              0x00000001
#define STICKY_CACHE_ENTRY              0x00000004
#define EDITED_CACHE_ENTRY              0x00000008
#define TRACK_OFFLINE_CACHE_ENTRY       0x00000010
#define TRACK_ONLINE_CACHE_ENTRY        0x00000020
#define SPARSE_CACHE_ENTRY              0x00010000
#define COOKIE_CACHE_ENTRY              0x00100000
#define URLHISTORY_CACHE_ENTRY          0x00200000

;begin_internal
#define HTTP_1_1_CACHE_ENTRY            0x00000040
#define STATIC_CACHE_ENTRY              0x00000080
#define MUST_REVALIDATE_CACHE_ENTRY     0x00000100
#define PENDING_DELETE_CACHE_ENTRY      0x00400000
#define OTHER_USER_CACHE_ENTRY          0x00800000
#define POST_RESPONSE_CACHE_ENTRY       0x04000000
#define INSTALLED_CACHE_ENTRY           0x10000000
#define POST_CHECK_CACHE_ENTRY          0x20000000

// We include some entry types even if app doesn't specifically ask for them.
#define INCLUDE_BY_DEFAULT_CACHE_ENTRY \
    (HTTP_1_1_CACHE_ENTRY | MUST_REVALIDATE_CACHE_ENTRY)

;end_internal


#define URLCACHE_FIND_DEFAULT_FILTER    NORMAL_CACHE_ENTRY             \
                                    |   COOKIE_CACHE_ENTRY             \
                                    |   URLHISTORY_CACHE_ENTRY         \
                                    |   TRACK_OFFLINE_CACHE_ENTRY      \
                                    |   TRACK_ONLINE_CACHE_ENTRY       \
                                    |   STICKY_CACHE_ENTRY



//
// INTERNET_CACHE_ENTRY_INFO -
//

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)

typedef struct _INTERNET_CACHE_ENTRY_INFO% {
    DWORD dwStructSize;         // version of cache system.
    LPTSTR% lpszSourceUrlName;    // embedded pointer to the URL name string.
    LPTSTR% lpszLocalFileName;  // embedded pointer to the local file name.
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
    LPTSTR% lpHeaderInfo;        // embedded pointer to the header info.
    DWORD dwHeaderInfoSize;     // size of the above header.
    LPTSTR% lpszFileExtension;  // File extension used to retrive the urldata as a file.
        union {                     // Exemption delta from last access time.
                DWORD dwReserved;
                DWORD dwExemptDelta;
    };                          // Exemption delta from last access
} INTERNET_CACHE_ENTRY_INFO%, * LPINTERNET_CACHE_ENTRY_INFO%;

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

typedef struct _INTERNET_CACHE_TIMESTAMPS {
    FILETIME    ftExpires;
    FILETIME    ftLastModified;
} INTERNET_CACHE_TIMESTAMPS, *LPINTERNET_CACHE_TIMESTAMPS;



//
// Cache Group
//
typedef LONGLONG GROUPID;


//
// Cache Group Flags
//
#define CACHEGROUP_ATTRIBUTE_GET_ALL        0xffffffff
#define CACHEGROUP_ATTRIBUTE_BASIC          0x00000001
#define CACHEGROUP_ATTRIBUTE_FLAG           0x00000002
#define CACHEGROUP_ATTRIBUTE_TYPE           0x00000004
#define CACHEGROUP_ATTRIBUTE_QUOTA          0x00000008
#define CACHEGROUP_ATTRIBUTE_GROUPNAME      0x00000010
#define CACHEGROUP_ATTRIBUTE_STORAGE        0x00000020

#define CACHEGROUP_FLAG_NONPURGEABLE        0x00000001
#define CACHEGROUP_FLAG_GIDONLY             0x00000004

#define CACHEGROUP_FLAG_FLUSHURL_ONDELETE   0x00000002

#define CACHEGROUP_SEARCH_ALL               0x00000000
#define CACHEGROUP_SEARCH_BYURL             0x00000001

#define CACHEGROUP_TYPE_INVALID             0x00000001


//
// updatable cache group fields
//
#define CACHEGROUP_READWRITE_MASK                   \
            CACHEGROUP_ATTRIBUTE_TYPE               \
        |   CACHEGROUP_ATTRIBUTE_QUOTA              \
        |   CACHEGROUP_ATTRIBUTE_GROUPNAME          \
        |   CACHEGROUP_ATTRIBUTE_STORAGE

//
// INTERNET_CACHE_GROUP_INFO
//

#define  GROUPNAME_MAX_LENGTH       120
#define  GROUP_OWNER_STORAGE_SIZE   4
typedef struct _INTERNET_CACHE_GROUP_INFO% {
    DWORD           dwGroupSize;
    DWORD           dwGroupFlags;
    DWORD           dwGroupType;
    DWORD           dwDiskUsage;    // in KB
    DWORD           dwDiskQuota;    // in KB
    DWORD           dwOwnerStorage[GROUP_OWNER_STORAGE_SIZE];
    TCHAR%          szGroupName[GROUPNAME_MAX_LENGTH];
} INTERNET_CACHE_GROUP_INFO%, * LPINTERNET_CACHE_GROUP_INFO%;


;begin_internal

//
// Well known sticky group ID
//
#define CACHEGROUP_ID_BUILTIN_STICKY       0x1000000000000007

//
// INTERNET_CACHE_CONFIG_PATH_ENTRY
//

typedef struct _INTERNET_CACHE_CONFIG_PATH_ENTRY% {
    TCHAR% CachePath[MAX_PATH];
    DWORD dwCacheSize;  // in KBytes
} INTERNET_CACHE_CONFIG_PATH_ENTRY%, * LPINTERNET_CACHE_CONFIG_PATH_ENTRY%;

//
// INTERNET_CACHE_CONFIG_INFO
//

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201)

typedef struct _INTERNET_CACHE_CONFIG_INFO% {
    DWORD dwStructSize;
    DWORD dwContainer;
    DWORD dwQuota;
    DWORD dwReserved4;
    BOOL  fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    union 
    { 
        struct 
        {
            TCHAR% CachePath[MAX_PATH];
            DWORD dwCacheSize;
        };
        INTERNET_CACHE_CONFIG_PATH_ENTRY% CachePaths[ANYSIZE_ARRAY];
    };
    DWORD dwNormalUsage;
    DWORD dwExemptUsage;
} INTERNET_CACHE_CONFIG_INFO%, * LPINTERNET_CACHE_CONFIG_INFO%;

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif


BOOLAPI
IsUrlCacheEntryExpired%(
    IN      LPCTSTR%        lpszUrlName,
    IN      DWORD           dwFlags,
    IN OUT  FILETIME*       pftLastModified
    );


;end_internal

//
// Cache APIs
//

BOOLAPI
CreateUrlCacheEntry%(
    IN LPCTSTR% lpszUrlName,
    IN DWORD dwExpectedFileSize,
    IN LPCTSTR% lpszFileExtension,
    OUT LPTSTR% lpszFileName,
    IN DWORD dwReserved
    );

#ifndef USE_FIXED_COMMIT_URL_CACHE_ENTRY
// Temporary state of affairs until we reconcile our apis.

// Why are we doing this? HeaderInfo _should_ be string data.
// However, one group is passing binary data instead. For the
// unicode api, we've decided to disallow this, but this
// brings up an inconsistency between the u and a apis, which
// is undesirable.

// For Beta 1, we'll go with this behaviour, but in future releases
// we want to make these apis consistent.

BOOLAPI
CommitUrlCacheEntryA(
    IN LPCSTR lpszUrlName,
    IN LPCSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPBYTE lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCSTR lpszFileExtension,
    IN LPCSTR lpszOriginalUrl
    );
BOOLAPI
CommitUrlCacheEntryW(
    IN LPCWSTR lpszUrlName,
    IN LPCWSTR lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPWSTR lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCWSTR lpszFileExtension,
    IN LPCWSTR lpszOriginalUrl
    );

#ifdef UNICODE
#define CommitUrlCacheEntry CommitUrlCacheEntryW
#else
#define CommitUrlCacheEntry CommitUrlCacheEntryA
#endif

#else
CommitUrlCacheEntry%(
    IN LPCTSTR% lpszUrlName,
    IN LPCTSTR% lpszLocalFileName,
    IN FILETIME ExpireTime,
    IN FILETIME LastModifiedTime,
    IN DWORD CacheEntryType,
    IN LPCTSTR% lpHeaderInfo,
    IN DWORD dwHeaderSize,
    IN LPCTSTR% lpszFileExtension,
    IN LPCTSTR% lpszOriginalUrl
    );
#endif

BOOLAPI
RetrieveUrlCacheEntryFile%(
    IN LPCTSTR%  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFO% lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN DWORD dwReserved
    );

BOOLAPI
UnlockUrlCacheEntryFileA(
    IN LPCSTR lpszUrlName,
    IN DWORD dwReserved
    );

BOOLAPI
UnlockUrlCacheEntryFileW(
    IN LPCWSTR lpszUrlName,
    IN DWORD dwReserved
    );

    

#ifdef UNICODE
#define UnlockUrlCacheEntryFile  UnlockUrlCacheEntryFileW
#else
#ifdef _WINX32_
#define UnlockUrlCacheEntryFile  UnlockUrlCacheEntryFileA
#else
BOOLAPI
UnlockUrlCacheEntryFile(
    IN LPCSTR lpszUrlName,
    IN DWORD dwReserved
    );
#endif // _WINX32_
#endif // !UNICODE

INTERNETAPI
HANDLE
WINAPI
RetrieveUrlCacheEntryStream%(
    IN LPCTSTR%  lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFO% lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize,
    IN BOOL fRandomRead,
    IN DWORD dwReserved
    );

BOOLAPI
ReadUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD dwLocation,
    IN OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwLen,
    IN DWORD Reserved
    );

BOOLAPI
UnlockUrlCacheEntryStream(
    IN HANDLE hUrlCacheStream,
    IN DWORD Reserved
    );


BOOLAPI
GetUrlCacheEntryInfo%(
    IN LPCTSTR% lpszUrlName,
    OUT LPINTERNET_CACHE_ENTRY_INFO% lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufferSize
    );


URLCACHEAPI
HANDLE
WINAPI
FindFirstUrlCacheGroup(
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwFilter,
    IN      LPVOID                          lpSearchCondition,
    IN      DWORD                           dwSearchCondition,
    OUT     GROUPID*                        lpGroupId,
    IN OUT  LPVOID                          lpReserved
    );

URLCACHEAPI
BOOL
WINAPI
FindNextUrlCacheGroup(
    IN HANDLE                               hFind,
    OUT GROUPID*                            lpGroupId,
    IN OUT  LPVOID                          lpReserved
    );


URLCACHEAPI
BOOL
WINAPI
GetUrlCacheGroupAttribute%(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    OUT     LPINTERNET_CACHE_GROUP_INFO%    lpGroupInfo,
    IN OUT  LPDWORD                         lpdwGroupInfo,
    IN OUT  LPVOID                          lpReserved
    );

URLCACHEAPI
BOOL
WINAPI
SetUrlCacheGroupAttribute%(
    IN      GROUPID                         gid,
    IN      DWORD                           dwFlags,
    IN      DWORD                           dwAttributes,
    IN      LPINTERNET_CACHE_GROUP_INFO%    lpGroupInfo,
    IN OUT  LPVOID                          lpReserved
    );


INTERNETAPI
GROUPID
WINAPI
CreateUrlCacheGroup(
    IN      DWORD                           dwFlags,
    IN      LPVOID                          lpReserved
    );

BOOLAPI
DeleteUrlCacheGroup(
    IN      GROUPID                         GroupId,
    IN      DWORD                           dwFlags,
    IN      LPVOID                          lpReserved
    );

;begin_internal
#define INTERNET_CACHE_FLAG_ALLOW_COLLISIONS     0x00000100
#define INTERNET_CACHE_FLAG_INSTALLED_ENTRY      0x00000200
#define INTERNET_CACHE_FLAG_ENTRY_OR_MAPPING     0x00000400
#define INTERNET_CACHE_FLAG_ADD_FILENAME_ONLY    0x00000800
#define INTERNET_CACHE_FLAG_GET_STRUCT_ONLY      0x00001000
;end_internal

BOOLAPI
GetUrlCacheEntryInfoEx%(
    IN LPCTSTR% lpszUrl,
    OUT LPINTERNET_CACHE_ENTRY_INFO% lpCacheEntryInfo,
    IN OUT LPDWORD lpdwCacheEntryInfoBufSize,
    OUT LPTSTR%      lpszReserved,  // must pass null
    IN OUT LPDWORD lpdwReserved,  // must pass null
    LPVOID         lpReserved,    // must pass null
    DWORD          dwFlags        // reserved
    );

#define CACHE_ENTRY_ATTRIBUTE_FC    0x00000004
#define CACHE_ENTRY_HITRATE_FC      0x00000010
#define CACHE_ENTRY_MODTIME_FC      0x00000040
#define CACHE_ENTRY_EXPTIME_FC      0x00000080
#define CACHE_ENTRY_ACCTIME_FC      0x00000100
#define CACHE_ENTRY_SYNCTIME_FC     0x00000200
#define CACHE_ENTRY_HEADERINFO_FC   0x00000400
#define CACHE_ENTRY_EXEMPT_DELTA_FC 0x00000800

;begin_internal
#define CACHE_ENTRY_MODIFY_DATA_FC  0x80000000
;end_internal

BOOLAPI
SetUrlCacheEntryInfo%(
    IN LPCTSTR% lpszUrlName,
    IN LPINTERNET_CACHE_ENTRY_INFO% lpCacheEntryInfo,
    IN DWORD dwFieldControl
    );

//
// Cache Group Functions
//


INTERNETAPI
GROUPID
WINAPI
CreateUrlCacheGroup(
    IN DWORD  dwFlags,
    IN LPVOID lpReserved  // must pass NULL
    );

BOOLAPI
DeleteUrlCacheGroup(
    IN  GROUPID GroupId,
    IN  DWORD   dwFlags,       // must pass 0
    IN  LPVOID  lpReserved     // must pass NULL
    );

// Flags for SetUrlCacheEntryGroup
#define INTERNET_CACHE_GROUP_ADD      0
#define INTERNET_CACHE_GROUP_REMOVE   1

BOOLAPI
SetUrlCacheEntryGroupA(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    );

BOOLAPI
SetUrlCacheEntryGroupW(
    IN LPCWSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    );

#ifdef UNICODE
#define SetUrlCacheEntryGroup  SetUrlCacheEntryGroupW
#else
#ifdef _WINX32_
#define SetUrlCacheEntryGroup  SetUrlCacheEntryGroupA
#else
BOOLAPI
SetUrlCacheEntryGroup(
    IN LPCSTR   lpszUrlName,
    IN DWORD    dwFlags,
    IN GROUPID  GroupId,
    IN LPBYTE   pbGroupAttributes, // must pass NULL
    IN DWORD    cbGroupAttributes, // must pass 0
    IN LPVOID   lpReserved         // must pass NULL
    );
#endif // _WINX32_
#endif // !UNICODE

INTERNETAPI
HANDLE
WINAPI
FindFirstUrlCacheEntryEx%(
    IN     LPCTSTR%    lpszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    LPINTERNET_CACHE_ENTRY_INFO% lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    );

BOOLAPI
FindNextUrlCacheEntryEx%(
    IN     HANDLE    hEnumHandle,
    OUT    LPINTERNET_CACHE_ENTRY_INFO% lpFirstCacheEntryInfo,
    IN OUT LPDWORD   lpdwFirstCacheEntryInfoBufferSize,
    OUT    LPVOID    lpReserved,     // must pass NULL
    IN OUT LPDWORD   pcbReserved2,   // must pass NULL
    IN     LPVOID    lpReserved3     // must pass NULL
    );

INTERNETAPI
HANDLE
WINAPI
FindFirstUrlCacheEntry%(
    IN LPCTSTR% lpszUrlSearchPattern,
    OUT LPINTERNET_CACHE_ENTRY_INFO% lpFirstCacheEntryInfo,
    IN OUT LPDWORD lpdwFirstCacheEntryInfoBufferSize
    );

BOOLAPI
FindNextUrlCacheEntry%(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_ENTRY_INFO% lpNextCacheEntryInfo,
    IN OUT LPDWORD lpdwNextCacheEntryInfoBufferSize
    );

;begin_internal

// Flags for CreateContainer

#define INTERNET_CACHE_CONTAINER_NOSUBDIRS (0x1)
#define INTERNET_CACHE_CONTAINER_AUTODELETE (0x2)
#define INTERNET_CACHE_CONTAINER_RESERVED1 (0x4)
#define INTERNET_CACHE_CONTAINER_NODESKTOPINIT (0x8)
#define INTERNET_CACHE_CONTAINER_MAP_ENABLED (0x10)

BOOLAPI
CreateUrlCacheContainer%(
     IN LPCTSTR% Name,
     IN LPCTSTR% lpCachePrefix,
     LPCTSTR% lpszCachePath,
     IN DWORD KBCacheLimit,
     IN DWORD dwContainerType,
     IN DWORD dwOptions,
     IN OUT LPVOID pvBuffer,
     IN OUT LPDWORD cbBuffer
     );

BOOLAPI
DeleteUrlCacheContainer%(
     IN LPCTSTR% Name,
     IN DWORD dwOptions
     );

//
// INTERNET_CACHE_ENTRY_INFO -
//


typedef struct _INTERNET_CACHE_CONTAINER_INFO% {
    DWORD dwCacheVersion;       // version of software
    LPTSTR% lpszName;             // embedded pointer to the container name string.
    LPTSTR% lpszCachePrefix;      // embedded pointer to the container URL prefix
    LPTSTR% lpszVolumeLabel;      // embedded pointer to the container volume label if any.
    LPTSTR% lpszVolumeTitle;      // embedded pointer to the container volume title if any.
} INTERNET_CACHE_CONTAINER_INFO%, * LPINTERNET_CACHE_CONTAINER_INFO%;

//  FindFirstContainer options
#define CACHE_FIND_CONTAINER_RETURN_NOCHANGE (0x1)

INTERNETAPI
HANDLE
WINAPI
FindFirstUrlCacheContainer%(
    IN OUT LPDWORD pdwModified,
    OUT LPINTERNET_CACHE_CONTAINER_INFO% lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );

BOOLAPI
FindNextUrlCacheContainer%(
    IN HANDLE hEnumHandle,
    OUT LPINTERNET_CACHE_CONTAINER_INFO% lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize
    );

;end_internal

BOOLAPI
FindCloseUrlCache(
    IN HANDLE hEnumHandle
    );

BOOLAPI
DeleteUrlCacheEntryA(
    IN LPCSTR lpszUrlName
    );

BOOLAPI
DeleteUrlCacheEntryW(
    IN LPCWSTR lpszUrlName
    );

#ifdef UNICODE
#define DeleteUrlCacheEntry  DeleteUrlCacheEntryW
#else
#ifdef _WINX32_
#define DeleteUrlCacheEntry  DeleteUrlCacheEntryA
#else
BOOLAPI
DeleteUrlCacheEntry(
    IN LPCSTR lpszUrlName
    );
#endif // _WINX32_
#endif // !UNICODE

;begin_internal


typedef enum {
    WININET_SYNC_MODE_NEVER=0,
    WININET_SYNC_MODE_ON_EXPIRY, // bogus
    WININET_SYNC_MODE_ONCE_PER_SESSION,
    WININET_SYNC_MODE_ALWAYS,
    WININET_SYNC_MODE_AUTOMATIC,
    WININET_SYNC_MODE_DEFAULT = WININET_SYNC_MODE_AUTOMATIC
} WININET_SYNC_MODE;


BOOLAPI
FreeUrlCacheSpace%(
    IN LPCTSTR% lpszCachePath,
    IN DWORD dwSize,
    IN DWORD dwFilter
    );

//
// config APIs.
//

#define CACHE_CONFIG_FORCE_CLEANUP_FC           0x00000020
#define CACHE_CONFIG_DISK_CACHE_PATHS_FC        0x00000040
#define CACHE_CONFIG_SYNC_MODE_FC               0x00000080
#define CACHE_CONFIG_CONTENT_PATHS_FC           0x00000100
#define CACHE_CONFIG_COOKIES_PATHS_FC           0x00000200
#define CACHE_CONFIG_HISTORY_PATHS_FC           0x00000400
#define CACHE_CONFIG_QUOTA_FC                   0x00000800
#define CACHE_CONFIG_USER_MODE_FC               0x00001000
#define CACHE_CONFIG_CONTENT_USAGE_FC           0x00002000
#define CACHE_CONFIG_STICKY_CONTENT_USAGE_FC    0x00004000

BOOLAPI
GetUrlCacheConfigInfo%(
    OUT LPINTERNET_CACHE_CONFIG_INFO% lpCacheConfigInfo,
    IN OUT LPDWORD lpdwCacheConfigInfoBufferSize,
    IN DWORD dwFieldControl
    );

BOOLAPI
SetUrlCacheConfigInfo%(
    IN LPINTERNET_CACHE_CONFIG_INFO% lpCacheConfigInfo,
    IN DWORD dwFieldControl
    );

INTERNETAPI
DWORD
WINAPI
RunOnceUrlCache(
        HWND    hwnd,
        HINSTANCE hinst,
        LPSTR   lpszCmd,
        int     nCmdShow);

INTERNETAPI
DWORD
WINAPI
DeleteIE3Cache(
        HWND    hwnd,
        HINSTANCE hinst,
        LPSTR   lpszCmd,
        int     nCmdShow);

BOOLAPI
UpdateUrlCacheContentPath(LPSTR szNewPath);

// Cache header data defines.

#define CACHE_HEADER_DATA_CURRENT_SETTINGS_VERSION   0
#define CACHE_HEADER_DATA_CONLIST_CHANGE_COUNT       1
#define CACHE_HEADER_DATA_COOKIE_CHANGE_COUNT        2

;begin_internal

#define CACHE_HEADER_DATA_NOTIFICATION_HWND          3
#define CACHE_HEADER_DATA_NOTIFICATION_MESG          4
#define CACHE_HEADER_DATA_ROOTGROUP_OFFSET           5
#define CACHE_HEADER_DATA_GID_LOW                    6
#define CACHE_HEADER_DATA_GID_HIGH                   7

// beta logging stats
#define CACHE_HEADER_DATA_CACHE_NOT_EXPIRED         8
#define CACHE_HEADER_DATA_CACHE_NOT_MODIFIED        9
#define CACHE_HEADER_DATA_CACHE_MODIFIED            10
#define CACHE_HEADER_DATA_CACHE_RESUMED             11
#define CACHE_HEADER_DATA_CACHE_NOT_RESUMED         12
#define CACHE_HEADER_DATA_CACHE_MISS                13
#define CACHE_HEADER_DATA_DOWNLOAD_PARTIAL          14
#define CACHE_HEADER_DATA_DOWNLOAD_ABORTED          15
#define CACHE_HEADER_DATA_DOWNLOAD_CACHED           16
#define CACHE_HEADER_DATA_DOWNLOAD_NOT_CACHED       17
#define CACHE_HEADER_DATA_DOWNLOAD_NO_FILE          18
#define CACHE_HEADER_DATA_DOWNLOAD_FILE_NEEDED      19
#define CACHE_HEADER_DATA_DOWNLOAD_FILE_NOT_NEEDED  20

// retail data
#define CACHE_HEADER_DATA_NOTIFICATION_FILTER       21
#define CACHE_HEADER_DATA_ROOT_LEAK_OFFSET          22

// more beta logging stats
#define CACHE_HEADER_DATA_SYNCSTATE_IMAGE           23
#define CACHE_HEADER_DATA_SYNCSTATE_VOLATILE        24
#define CACHE_HEADER_DATA_SYNCSTATE_IMAGE_STATIC    25
#define CACHE_HEADER_DATA_SYNCSTATE_STATIC_VOLATILE 26

// retail data
#define CACHE_HEADER_DATA_ROOT_GROUPLIST_OFFSET     27 // offset to group list
#define CACHE_HEADER_DATA_ROOT_FIXUP_OFFSET         28 // offset to fixup list
#define CACHE_HEADER_DATA_ROOT_FIXUP_COUNT          29 // num of fixup items
#define CACHE_HEADER_DATA_ROOT_FIXUP_TRIGGER        30 // threshhold to fix up
#define CACHE_HEADER_DATA_HIGH_VERSION_STRING       31 // highest entry ver


#define CACHE_HEADER_DATA_LAST                      31

// options for cache notification filter
#define CACHE_NOTIFY_ADD_URL                        0x00000001
#define CACHE_NOTIFY_DELETE_URL                     0x00000002
#define CACHE_NOTIFY_UPDATE_URL                     0x00000004
#define CACHE_NOTIFY_DELETE_ALL                     0x00000008
#define CACHE_NOTIFY_URL_SET_STICKY                 0x00000010
#define CACHE_NOTIFY_URL_UNSET_STICKY               0x00000020
#define CACHE_NOTIFY_SET_ONLINE                     0x00000100
#define CACHE_NOTIFY_SET_OFFLINE                    0x00000200

#define CACHE_NOTIFY_FILTER_CHANGED                 0x10000000

BOOL
RegisterUrlCacheNotification(
    IN  HWND        hWnd,
    IN  UINT        uMsg,
    IN  GROUPID     gid,
    IN  DWORD       dwOpsFilter,
    IN  DWORD       dwReserved
    );


;end_internal

BOOL
GetUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData);

BOOL
SetUrlCacheHeaderData(IN DWORD nIdx, IN  DWORD  dwData);

BOOL
IncrementUrlCacheHeaderData(IN DWORD nIdx, OUT LPDWORD lpdwData);

BOOL
LoadUrlCacheContent();

BOOL
GetUrlCacheContainerInfo%(
    IN LPTSTR% lpszUrlName,
    OUT LPINTERNET_CACHE_CONTAINER_INFO% lpContainerInfo,
    IN OUT LPDWORD lpdwContainerInfoBufferSize,
    IN DWORD dwOptions
    );

;end_internal

;begin_both

//
// Autodial APIs
//

INTERNETAPI
DWORD
WINAPI
InternetDialA(
    IN HWND     hwndParent,
    IN LPSTR   lpszConnectoid,
    IN DWORD    dwFlags,
    OUT DWORD_PTR *lpdwConnection,
    IN DWORD    dwReserved
    );

INTERNETAPI
DWORD
WINAPI
InternetDialW(
    IN HWND     hwndParent,
    IN LPWSTR   lpszConnectoid,
    IN DWORD    dwFlags,
    OUT DWORD_PTR *lpdwConnection,
    IN DWORD    dwReserved
    );

#ifdef UNICODE
#define InternetDial  InternetDialW
#else
#ifdef _WINX32_
#define InternetDial  InternetDialA
#else
INTERNETAPI
DWORD
WINAPI
InternetDial(
    IN HWND     hwndParent,
    IN LPSTR   lpszConnectoid,
    IN DWORD    dwFlags,
    OUT LPDWORD lpdwConnection,
    IN DWORD    dwReserved
    );
#endif // _WINX32_
#endif // !UNICODE

// Flags for InternetDial - must not conflict with InternetAutodial flags
//                          as they are valid here also.
#define INTERNET_DIAL_FORCE_PROMPT     0x2000
#define INTERNET_DIAL_SHOW_OFFLINE     0x4000
#define INTERNET_DIAL_UNATTENDED       0x8000

INTERNETAPI
DWORD
WINAPI
InternetHangUp(
    IN DWORD    dwConnection,
    IN DWORD    dwReserved);

#define INTERENT_GOONLINE_REFRESH 0x00000001
#define INTERENT_GOONLINE_MASK 0x00000001

INTERNETAPI
BOOL
WINAPI
InternetGoOnlineA(
    IN LPSTR   lpszURL,
    IN HWND     hwndParent,
    IN DWORD    dwFlags
    );

INTERNETAPI
BOOL
WINAPI
InternetGoOnlineW(
    IN LPWSTR   lpszURL,
    IN HWND     hwndParent,
    IN DWORD    dwFlags
    );

#ifdef UNICODE
#define InternetGoOnline  InternetGoOnlineW
#else
#ifdef _WINX32_
#define InternetGoOnline  InternetGoOnlineA
#else
INTERNETAPI
BOOL
WINAPI
InternetGoOnline(
    IN LPSTR   lpszURL,
    IN HWND     hwndParent,
    IN DWORD    dwFlags
    );
#endif // _WINX32_
#endif // !UNICODE

INTERNETAPI
BOOL
WINAPI
InternetAutodial(
    IN DWORD    dwFlags,
    IN HWND     hwndParent);

// Flags for InternetAutodial
#define INTERNET_AUTODIAL_FORCE_ONLINE          1
#define INTERNET_AUTODIAL_FORCE_UNATTENDED      2
#define INTERNET_AUTODIAL_FAILIFSECURITYCHECK   4

#define INTERNET_AUTODIAL_FLAGS_MASK (INTERNET_AUTODIAL_FORCE_ONLINE | INTERNET_AUTODIAL_FORCE_UNATTENDED | INTERNET_AUTODIAL_FAILIFSECURITYCHECK)
INTERNETAPI
BOOL
WINAPI
InternetAutodialHangup(
    IN DWORD    dwReserved);

INTERNETAPI
BOOL
WINAPI
InternetGetConnectedState(
    OUT LPDWORD  lpdwFlags,
    IN DWORD    dwReserved);

INTERNETAPI
BOOL
WINAPI
InternetGetConnectedStateExA(
    OUT LPDWORD lpdwFlags,
    OUT LPSTR  lpszConnectionName,
    IN DWORD    dwNameLen,
    IN DWORD    dwReserved
    );

INTERNETAPI
BOOL
WINAPI
InternetGetConnectedStateExW(
    OUT LPDWORD lpdwFlags,
    OUT LPWSTR  lpszConnectionName,
    IN DWORD    dwNameLen,
    IN DWORD    dwReserved
    );

#ifdef UNUSED
INTERNETAPI
BOOL
WINAPI
InternetInitializeAutoProxyDll(
    DWORD dwReserved
    );
#endif
    
#ifdef UNICODE
#define InternetGetConnectedStateEx  InternetGetConnectedStateExW
#else
#ifdef _WINX32_
#define InternetGetConnectedStateEx  InternetGetConnectedStateExA
#else
INTERNETAPI
BOOL
WINAPI
InternetGetConnectedStateEx(
    OUT LPDWORD lpdwFlags,
    IN LPSTR  lpszConnectionName,
    IN DWORD    dwNameLen,
    IN DWORD    dwReserved
    );
#endif // _WINX32_
#endif // !UNICODE

// Flags for InternetGetConnectedState and Ex
#define INTERNET_CONNECTION_MODEM           0x01
#define INTERNET_CONNECTION_LAN             0x02
#define INTERNET_CONNECTION_PROXY           0x04
#define INTERNET_CONNECTION_MODEM_BUSY      0x08  /* no longer used */
#define INTERNET_RAS_INSTALLED              0x10
#define INTERNET_CONNECTION_OFFLINE         0x20
#define INTERNET_CONNECTION_CONFIGURED      0x40

//
// Custom dial handler functions
//

// Custom dial handler prototype
typedef DWORD (CALLBACK * PFN_DIAL_HANDLER) (HWND, LPCSTR, DWORD, LPDWORD);

// Flags for custom dial handler
#define INTERNET_CUSTOMDIAL_CONNECT         0
#define INTERNET_CUSTOMDIAL_UNATTENDED      1
#define INTERNET_CUSTOMDIAL_DISCONNECT      2
#define INTERNET_CUSTOMDIAL_SHOWOFFLINE     4

// Custom dial handler supported functionality flags
#define INTERNET_CUSTOMDIAL_SAFE_FOR_UNATTENDED 1
#define INTERNET_CUSTOMDIAL_WILL_SUPPLY_STATE   2
#define INTERNET_CUSTOMDIAL_CAN_HANGUP          4

INTERNETAPI
BOOL
WINAPI
InternetSetDialStateA(
    IN LPCSTR lpszConnectoid,
    IN DWORD    dwState,
    IN DWORD    dwReserved
    );

INTERNETAPI
BOOL
WINAPI
InternetSetDialStateW(
    IN LPCWSTR lpszConnectoid,
    IN DWORD    dwState,
    IN DWORD    dwReserved
    );

#ifdef UNICODE
#define InternetSetDialState  InternetSetDialStateW
#else
#ifdef _WINX32_
#define InternetSetDialState  InternetSetDialStateA
#else
INTERNETAPI
BOOL
WINAPI
InternetSetDialState(
    IN LPCSTR lpszConnectoid,
    IN DWORD    dwState,
    IN DWORD    dwReserved
    );
#endif // _WINX32_
#endif // !UNICODE

// States for InternetSetDialState
#define INTERNET_DIALSTATE_DISCONNECTED     1

;end_both

;begin_internal
// Registry entries used by the dialing code
// All of these entries are in:
// HKCU\software\microsoft\windows\current version\internet settings

#define REGSTR_DIAL_AUTOCONNECT     "AutoConnect"

;end_internal

;begin_internal
// Wininet events

#define REGSTR_PATH_INETEVENTS      "Software\\microsoft\\windows\\currentversion\\internet settings\\Events"

#define INETEVT_RAS_CONNECT         0x00000001
#define INETEVT_RAS_DISCONNECT      0x00000002
#define INETEVT_ONLINE              0x00000004
#define INETEVT_OFFLINE             0x00000008
#define INETEVT_LOGON               0x00000010
#define INETEVT_LOGOFF              0x00000020

INTERNETAPI
DWORD
WINAPI
InternetDispatchEvent(
    IN DWORD dwEvent,
    IN LPWSTR pwsEventDesc,
    IN DWORD dwEventData);

;end_internal

;begin_internal

// Used by security manager.

INTERNETAPI
BOOL
WINAPI
IsHostInProxyBypassList(
    IN INTERNET_SCHEME tScheme,
    IN LPCSTR   lpszHost,
    IN DWORD    cchHost);

// Used by Shell to determine if anyone has loaded wininet yet
// Shell code calls OpenMutex with this name and if no mutex is
// obtained, we know that no copy of wininet has been loaded yet

#define WININET_STARTUP_MUTEX "WininetStartupMutex"

;end_internal

;begin_internal
BOOL DoConnectoidsExist(void); // Returns TRUE if any RAS connectoids exist and FALSE otherwise

BOOL GetDiskInfoA(
    IN      PSTR pszPath, 
    IN OUT  PDWORD pdwClusterSize, 
    IN OUT  PDWORDLONG pdlAvail, 
    IN OUT  PDWORDLONG pdlTotal);

typedef BOOL (*CACHE_OPERATOR)(INTERNET_CACHE_ENTRY_INFO* pcei, PDWORD pcbcei, PVOID pOpData);

BOOL PerformOperationOverUrlCacheA(
    IN     PCSTR     pszUrlSearchPattern,
    IN     DWORD     dwFlags,
    IN     DWORD     dwFilter,
    IN     GROUPID   GroupId,
    OUT    PVOID     pReserved1,
    IN OUT PDWORD    pdwReserved2,
    IN     PVOID     pReserved3,
    IN       CACHE_OPERATOR op, 
    IN OUT PVOID     pOperatorData
    );

BOOL IsProfilesCapable();

;end_internal


;begin_internal

//  in cookimp.cxx and cookexp.cxx
BOOL ImportCookieFile%( IN LPCTSTR% szFilename );
BOOL ExportCookieFile%( IN LPCTSTR% szFilename, BOOL fAppend);

;end_internal


;begin_both

#if defined(__cplusplus)
}
#endif

;end_both

/*
 * Return packing to whatever it was before we
 * entered this file
 */
#include <poppack.h>

;begin_internal
#endif // !define(_WININETEX_)
;end_internal

#endif // !defined(_WININET_)
