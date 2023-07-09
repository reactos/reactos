/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    unsup.w

Abstract:

    No longer supported parts of wininet.w required to build product

Author:

    Richard L Firth (rfirth) 16-Aug-1995

Revision History:

    16-Aug-1995 rfirth
        Created

--*/

#define GATEWAY_INTERNET_ACCESS     2   // Internet via gateway

#define INTERNET_SERVICE_ARCHIE 4

#define INTERNET_HANDLE_TYPE_CONNECT_ARCHIE     14
#define INTERNET_HANDLE_TYPE_ARCHIE_FIND        15

#define INTERNET_OPTION_NAME_RES_THREAD         11
#define INTERNET_OPTION_GATEWAY_NAME            14
#define INTERNET_OPTION_ASYNC_REQUEST_COUNT     17
#define INTERNET_OPTION_MAXIMUM_WORKER_THREADS  18
#define INTERNET_OPTION_ASYNC_QUEUE_DEPTH       19
#define INTERNET_OPTION_WORKER_THREAD_TIMEOUT   20
#define INTERNET_OPTION_RECEIVE_ALL_MODE        25

//
// MIME
//

//
// parameters
//

#define MIME_OVERWRITE_EXISTING 0x00000001
#define MIME_FAIL_IF_EXISTING   0x00000002

#define MIME_ALL                ((DWORD)-1)
#define MIME_CONTENT_TYPE       1
#define MIME_EXTENSION          2
#define MIME_VIEWER             3

//
// types
//

typedef
BOOL
(CALLBACK * MIME_ENUMERATOR)(
    IN LPCTSTR lpszContentType,
    IN LPCTSTR lpszExtensions,
    IN LPCTSTR lpszViewer,
    IN LPCTSTR lpszViewerFriendlyName,
    IN LPCTSTR lpszCommandLine
    );

//
// prototypes
//

INTERNETAPI
BOOL
WINAPI
MimeCreateAssociation%(
    IN LPCTSTR% lpszContentType,
    IN LPCTSTR% lpszExtensions,
    IN LPCTSTR% lpszViewer,
    IN LPCTSTR% lpszViewerFriendlyName OPTIONAL,
    IN LPCTSTR% lpszCommandLine OPTIONAL,
    IN DWORD dwOptions
    );

INTERNETAPI
BOOL
WINAPI
MimeDeleteAssociation%(
    IN LPCTSTR% lpszContentType
    );

INTERNETAPI
BOOL
WINAPI
MimeGetAssociation%(
    IN DWORD dwFilterType,
    IN LPCTSTR% lpszFilter OPTIONAL,
    IN MIME_ENUMERATOR lpfnEnumerator
    );

//
// Archie
//

//
// manifests
//

#define ARCHIE_PRIORITY_LOW         32767
#define ARCHIE_PRIORITY_MEDIUM      0
#define ARCHIE_PRIORITY_HIGH        (-32767)

//
// Definitions of various string lengths
//

#define ARCHIE_MAX_HOST_TYPE_LENGTH 20
#define ARCHIE_MAX_HOST_NAME_LENGTH INTERNET_MAX_HOST_NAME_LENGTH
#define ARCHIE_MAX_HOST_ADDR_LENGTH 20
#define ARCHIE_MAX_USERNAME_LENGTH  30
#define ARCHIE_MAX_PASSWORD_LENGTH  30
#define ARCHIE_MAX_PATH_LENGTH      INTERNET_MAX_PATH_LENGTH

//
// structures/types
//

//
// ARCHIE_SEARCH_TYPE - Specifies the type of search to be performed. The
// cosntants are specific to Archie Protocol
//

typedef enum {
    ArchieExact                 = '=',
    ArchieRegexp                = 'R',
    ArchieExactOrRegexp         = 'r',
    ArchieSubstring             = 'S',
    ArchieExactOrSubstring      = 's',
    ArchieCaseSubstring         = 'C',  // substring with case sensitiveness
    ArchieExactOrCaseSubstring  = 'c'
} ARCHIE_SEARCH_TYPE;

//
// ARCHIE_TRANSFER_TYPE - Specifies the type of transfer for the file located
// using archie search
//

typedef enum {
    ArchieTransferUnknown       = 0x0,
    ArchieTransferBinary        = 0x1,
    ArchieTransferAscii         = 0x2
} ARCHIE_TRANSFER_TYPE;

//
// ARCHIE_ACCESS_METHOD - Specifies the type of access method used for
// accessing a file located by archie search
//

typedef enum {
    ArchieError                 = 0x0,
    ArchieAftp                  = 0x1,  // Anonymous FTP
    ArchieFtp                   = 0x2,  // FTP
    ArchieNfs                   = 0x4,  // NFS File System
    ArchieKnfs                  = 0x8,  // Kerberized NFS
    ArchiePfs                   = 0x10  // Andrew File System
} ARCHIE_ACCESS_METHOD;

//
// ARCHIE_FIND_DATA - Structure which stored the data found about a file that
// matches an archie query. It stores information about file name, attributes,
// location, size, and access method to be used for accessing the file. The
// path of the file instead of just the filename is stored
//

typedef struct {

    //
    // dwAttributes - attributes of the file
    //

    DWORD dwAttributes;

    //
    // dwSize - size of the file
    //

    DWORD dwSize;

    //
    // ftLastModificationTime - last when this file was modified
    //

    FILETIME ftLastFileModTime;

    //
    // ftLastHostModificationTime - last when file was modified
    //

    FILETIME ftLastHostModTime;

    //
    // TransferType - transfer type
    //

    ARCHIE_TRANSFER_TYPE TransferType;

    //
    // AccessMethodType - type of access
    //

    ARCHIE_ACCESS_METHOD AccessMethod;

    //
    // cHostType - type of host
    //

    TCHAR cHostType[ARCHIE_MAX_HOST_TYPE_LENGTH];

    //
    // cHostName - name of the host
    //

    TCHAR cHostName[ARCHIE_MAX_HOST_NAME_LENGTH];

    //
    // cHost - the host's Internet address
    //

    TCHAR cHostAddr[ARCHIE_MAX_HOST_ADDR_LENGTH];

    //
    // cFileName - path and name of the file
    //

    TCHAR cFileName[ARCHIE_MAX_PATH_LENGTH];

    //
    // cUserName - valid only for non-anonymous FTP access type
    //

    TCHAR cUserName[ARCHIE_MAX_USERNAME_LENGTH];

    //
    // cPassword - valid only for non-anonymous FTP access type
    //

    TCHAR cPassword[ARCHIE_MAX_PASSWORD_LENGTH];

} ARCHIE_FIND_DATA, *LPARCHIE_FIND_DATA;

//
// prototypes
//

INTERNETAPI
BOOL
WINAPI
InternetCancelAsyncRequest(
    IN DWORD dwAsyncId
    );

INTERNETAPI
HINTERNET
WINAPI
ArchieFindFirstFile%(
    IN HINTERNET hArchieSession,
    IN LPCTSTR% * lplpszHosts OPTIONAL,
    IN LPCTSTR% lpszSearchString,
    IN DWORD dwMaxHits,
    IN DWORD dwOffset,
    IN DWORD dwPriority,
    IN ARCHIE_SEARCH_TYPE SearchType,
    OUT LPARCHIE_FIND_DATA lpFindData,
    OUT LPDWORD lpdwNumberFound,
    IN DWORD_PTR dwContext
    );

//
// archie API errors
//

#define ERROR_ARCHIE_NONE_FOUND                 (INTERNET_ERROR_BASE + 24)
#define ERROR_ARCHIE_NETWORK                    (INTERNET_ERROR_BASE + 25)
#define ERROR_ARCHIE_ABORTED                    (INTERNET_ERROR_BASE + 26)

//
// MIME API errors
//

#define ERROR_MIME_UNKNOWN_CONTENT_TYPE         (INTERNET_ERROR_BASE + 39)

//
// structures/types
//

//
// FTP_FIND_DATA - same as WIN32_FIND_DATA, except has extra space for path
//

typedef struct _FTP_FIND_DATA% {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    TCHAR% cFileName[INTERNET_MAX_PATH_LENGTH];
    TCHAR% cAlternateFileName[14];
} FTP_FIND_DATA%, * LPFTP_FIND_DATA%;
