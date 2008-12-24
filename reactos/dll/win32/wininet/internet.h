/*
 * Wininet
 *
 * Copyright 1999 Corel Corporation
 *
 * Ulrich Czekalla
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _WINE_INTERNET_H_
#define _WINE_INTERNET_H_

#ifndef __WINE_CONFIG_H
# error You must include config.h to use this header
#endif

#include "wine/unicode.h"
#include "wine/list.h"

#include <time.h>
#ifdef HAVE_NETDB_H
# include <netdb.h>
#endif
#ifdef HAVE_NETINET_IN_H
# include <sys/types.h>
# include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif

#if defined(__MINGW32__) || defined (_MSC_VER)
#include "ws2tcpip.h"
#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif
#else
#define closesocket close
#define ioctlsocket ioctl
#endif /* __MINGW32__ */

/* used for netconnection.c stuff */
typedef struct
{
    BOOL useSSL;
    int socketFD;
    void *ssl_s;
    char *peek_msg;
    char *peek_msg_mem;
    size_t peek_len;
} WININET_NETCONNECTION;

static inline LPWSTR WININET_strdupW( LPCWSTR str )
{
    LPWSTR ret = HeapAlloc( GetProcessHeap(), 0, (strlenW(str) + 1)*sizeof(WCHAR) );
    if (ret) strcpyW( ret, str );
    return ret;
}

static inline LPWSTR WININET_strdup_AtoW( LPCSTR str )
{
    int len = MultiByteToWideChar( CP_ACP, 0, str, -1, NULL, 0);
    LPWSTR ret = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
    if (ret)
        MultiByteToWideChar( CP_ACP, 0, str, -1, ret, len);
    return ret;
}

static inline LPSTR WININET_strdup_WtoA( LPCWSTR str )
{
    int len = WideCharToMultiByte( CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
    LPSTR ret = HeapAlloc( GetProcessHeap(), 0, len );
    if (ret)
        WideCharToMultiByte( CP_ACP, 0, str, -1, ret, len, NULL, NULL);
    return ret;
}

static inline void WININET_find_data_WtoA(LPWIN32_FIND_DATAW dataW, LPWIN32_FIND_DATAA dataA)
{
    dataA->dwFileAttributes = dataW->dwFileAttributes;
    dataA->ftCreationTime   = dataW->ftCreationTime;
    dataA->ftLastAccessTime = dataW->ftLastAccessTime;
    dataA->ftLastWriteTime  = dataW->ftLastWriteTime;
    dataA->nFileSizeHigh    = dataW->nFileSizeHigh;
    dataA->nFileSizeLow     = dataW->nFileSizeLow;
    dataA->dwReserved0      = dataW->dwReserved0;
    dataA->dwReserved1      = dataW->dwReserved1;
    WideCharToMultiByte(CP_ACP, 0, dataW->cFileName, -1, 
        dataA->cFileName, sizeof(dataA->cFileName),
        NULL, NULL);
    WideCharToMultiByte(CP_ACP, 0, dataW->cAlternateFileName, -1, 
        dataA->cAlternateFileName, sizeof(dataA->cAlternateFileName),
        NULL, NULL);
}

typedef enum
{
    WH_HINIT = INTERNET_HANDLE_TYPE_INTERNET,
    WH_HFTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_FTP,
    WH_HGOPHERSESSION = INTERNET_HANDLE_TYPE_CONNECT_GOPHER,
    WH_HHTTPSESSION = INTERNET_HANDLE_TYPE_CONNECT_HTTP,
    WH_HFILE = INTERNET_HANDLE_TYPE_FTP_FILE,
    WH_HFTPFINDNEXT = INTERNET_HANDLE_TYPE_FTP_FIND,
    WH_HHTTPREQ = INTERNET_HANDLE_TYPE_HTTP_REQUEST,
} WH_TYPE;

#define INET_OPENURL 0x0001
#define INET_CALLBACKW 0x0002

typedef struct _WININETHANDLEHEADER WININETHANDLEHEADER, *LPWININETHANDLEHEADER;

typedef struct {
    void (*Destroy)(WININETHANDLEHEADER*);
    void (*CloseConnection)(WININETHANDLEHEADER*);
    DWORD (*QueryOption)(WININETHANDLEHEADER*,DWORD,void*,DWORD*,BOOL);
    DWORD (*SetOption)(WININETHANDLEHEADER*,DWORD,void*,DWORD);
    DWORD (*ReadFile)(WININETHANDLEHEADER*,void*,DWORD,DWORD*);
    DWORD (*ReadFileExA)(WININETHANDLEHEADER*,INTERNET_BUFFERSA*,DWORD,DWORD_PTR);
    BOOL (*WriteFile)(WININETHANDLEHEADER*,const void*,DWORD,DWORD*);
    DWORD (*QueryDataAvailable)(WININETHANDLEHEADER*,DWORD*,DWORD,DWORD_PTR);
    DWORD (*FindNextFileW)(WININETHANDLEHEADER*,void*);
} HANDLEHEADERVtbl;

struct _WININETHANDLEHEADER
{
    WH_TYPE htype;
    const HANDLEHEADERVtbl *vtbl;
    HINTERNET hInternet;
    DWORD  dwFlags;
    DWORD_PTR dwContext;
    DWORD  dwError;
    DWORD  dwInternalFlags;
    LONG   refs;
    INTERNET_STATUS_CALLBACK lpfnStatusCB;
    struct list entry;
    struct list children;
};


typedef struct
{
    WININETHANDLEHEADER hdr;
    LPWSTR  lpszAgent;
    LPWSTR  lpszProxy;
    LPWSTR  lpszProxyBypass;
    LPWSTR  lpszProxyUsername;
    LPWSTR  lpszProxyPassword;
    DWORD   dwAccessType;
} WININETAPPINFOW, *LPWININETAPPINFOW;


typedef struct
{
    WININETHANDLEHEADER hdr;
    WININETAPPINFOW *lpAppInfo;
    LPWSTR  lpszHostName; /* the final destination of the request */
    LPWSTR  lpszServerName; /* the name of the server we directly connect to */
    LPWSTR  lpszUserName;
    LPWSTR  lpszPassword;
    INTERNET_PORT nHostPort; /* the final destination port of the request */
    INTERNET_PORT nServerPort; /* the port of the server we directly connect to */
    struct sockaddr_in socketAddress;
} WININETHTTPSESSIONW, *LPWININETHTTPSESSIONW;

#define HDR_ISREQUEST		0x0001
#define HDR_COMMADELIMITED	0x0002
#define HDR_SEMIDELIMITED	0x0004

typedef struct
{
    LPWSTR lpszField;
    LPWSTR lpszValue;
    WORD wFlags;
    WORD wCount;
} HTTPHEADERW, *LPHTTPHEADERW;


struct HttpAuthInfo;

typedef struct
{
    WININETHANDLEHEADER hdr;
    WININETHTTPSESSIONW *lpHttpSession;
    LPWSTR lpszPath;
    LPWSTR lpszVerb;
    LPWSTR lpszRawHeaders;
    WININET_NETCONNECTION netConnection;
    LPWSTR lpszVersion;
    LPWSTR lpszStatusText;
    DWORD dwContentLength; /* total number of bytes to be read */
    DWORD dwContentRead; /* bytes of the content read so far */
    HTTPHEADERW *pCustHeaders;
    DWORD nCustHeaders;
    HANDLE hCacheFile;
    LPWSTR lpszCacheFile;
    struct HttpAuthInfo *pAuthInfo;
    struct HttpAuthInfo *pProxyAuthInfo;
} WININETHTTPREQW, *LPWININETHTTPREQW;



struct WORKREQ_FTPPUTFILEW
{
    LPWSTR lpszLocalFile;
    LPWSTR lpszNewRemoteFile;
    DWORD  dwFlags;
    DWORD_PTR dwContext;
};

struct WORKREQ_FTPSETCURRENTDIRECTORYW
{
    LPWSTR lpszDirectory;
};

struct WORKREQ_FTPCREATEDIRECTORYW
{
    LPWSTR lpszDirectory;
};

struct WORKREQ_FTPFINDFIRSTFILEW
{
    LPWSTR lpszSearchFile;
    LPWIN32_FIND_DATAW lpFindFileData;
    DWORD  dwFlags;
    DWORD_PTR dwContext;
};

struct WORKREQ_FTPGETCURRENTDIRECTORYW
{
    LPWSTR lpszDirectory;
    DWORD *lpdwDirectory;
};

struct WORKREQ_FTPOPENFILEW
{
    LPWSTR lpszFilename;
    DWORD  dwAccess;
    DWORD  dwFlags;
    DWORD_PTR dwContext;
};

struct WORKREQ_FTPGETFILEW
{
    LPWSTR lpszRemoteFile;
    LPWSTR lpszNewFile;
    BOOL   fFailIfExists;
    DWORD  dwLocalFlagsAttribute;
    DWORD  dwFlags;
    DWORD_PTR dwContext;
};

struct WORKREQ_FTPDELETEFILEW
{
    LPWSTR lpszFilename;
};

struct WORKREQ_FTPREMOVEDIRECTORYW
{
    LPWSTR lpszDirectory;
};

struct WORKREQ_FTPRENAMEFILEW
{
    LPWSTR lpszSrcFile;
    LPWSTR lpszDestFile;
};

struct WORKREQ_FTPFINDNEXTW
{
    LPWIN32_FIND_DATAW lpFindFileData;
};

struct WORKREQ_HTTPSENDREQUESTW
{
    LPWSTR lpszHeader;
    DWORD  dwHeaderLength;
    LPVOID lpOptional;
    DWORD  dwOptionalLength;
    DWORD  dwContentLength;
    BOOL   bEndRequest;
};

struct WORKREQ_SENDCALLBACK
{
    DWORD_PTR dwContext;
    DWORD     dwInternetStatus;
    LPVOID    lpvStatusInfo;
    DWORD     dwStatusInfoLength;
};

struct WORKREQ_INTERNETOPENURLW
{
    HINTERNET hInternet;
    LPWSTR     lpszUrl;
    LPWSTR     lpszHeaders;
    DWORD     dwHeadersLength;
    DWORD     dwFlags;
    DWORD_PTR dwContext;
};

struct WORKREQ_INTERNETREADFILEEXA
{
    LPINTERNET_BUFFERSA lpBuffersOut;
};

typedef struct WORKREQ
{
    void (*asyncproc)(struct WORKREQ*);
    WININETHANDLEHEADER *hdr;

    union {
        struct WORKREQ_FTPPUTFILEW              FtpPutFileW;
        struct WORKREQ_FTPSETCURRENTDIRECTORYW  FtpSetCurrentDirectoryW;
        struct WORKREQ_FTPCREATEDIRECTORYW      FtpCreateDirectoryW;
        struct WORKREQ_FTPFINDFIRSTFILEW        FtpFindFirstFileW;
        struct WORKREQ_FTPGETCURRENTDIRECTORYW  FtpGetCurrentDirectoryW;
        struct WORKREQ_FTPOPENFILEW             FtpOpenFileW;
        struct WORKREQ_FTPGETFILEW              FtpGetFileW;
        struct WORKREQ_FTPDELETEFILEW           FtpDeleteFileW;
        struct WORKREQ_FTPREMOVEDIRECTORYW      FtpRemoveDirectoryW;
        struct WORKREQ_FTPRENAMEFILEW           FtpRenameFileW;
        struct WORKREQ_FTPFINDNEXTW             FtpFindNextW;
        struct WORKREQ_HTTPSENDREQUESTW         HttpSendRequestW;
        struct WORKREQ_SENDCALLBACK             SendCallback;
	struct WORKREQ_INTERNETOPENURLW         InternetOpenUrlW;
        struct WORKREQ_INTERNETREADFILEEXA      InternetReadFileExA;
    } u;

} WORKREQUEST, *LPWORKREQUEST;

HINTERNET WININET_AllocHandle( LPWININETHANDLEHEADER info );
LPWININETHANDLEHEADER WININET_GetObject( HINTERNET hinternet );
LPWININETHANDLEHEADER WININET_AddRef( LPWININETHANDLEHEADER info );
BOOL WININET_Release( LPWININETHANDLEHEADER info );
BOOL WININET_FreeHandle( HINTERNET hinternet );

DWORD INET_QueryOption(DWORD,void*,DWORD*,BOOL);

time_t ConvertTimeString(LPCWSTR asctime);

HINTERNET FTP_Connect(LPWININETAPPINFOW hIC, LPCWSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
	LPCWSTR lpszPassword, DWORD dwFlags, DWORD_PTR dwContext,
	DWORD dwInternalFlags);

HINTERNET HTTP_Connect(LPWININETAPPINFOW hIC, LPCWSTR lpszServerName,
	INTERNET_PORT nServerPort, LPCWSTR lpszUserName,
	LPCWSTR lpszPassword, DWORD dwFlags, DWORD_PTR dwContext,
	DWORD dwInternalFlags);

BOOL GetAddress(LPCWSTR lpszServerName, INTERNET_PORT nServerPort,
	struct sockaddr_in *psa);

void INTERNET_SetLastError(DWORD dwError);
DWORD INTERNET_GetLastError(void);
BOOL INTERNET_AsyncCall(LPWORKREQUEST lpWorkRequest);
LPSTR INTERNET_GetResponseBuffer(void);
LPSTR INTERNET_GetNextLine(INT nSocket, LPDWORD dwLen);

BOOLAPI HTTP_HttpSendRequestW(LPWININETHTTPREQW lpwhr, LPCWSTR lpszHeaders,
	DWORD dwHeaderLength, LPVOID lpOptional, DWORD dwOptionalLength,
	DWORD dwContentLength, BOOL bEndRequest);
INTERNETAPI HINTERNET WINAPI HTTP_HttpOpenRequestW(LPWININETHTTPSESSIONW lpwhs,
	LPCWSTR lpszVerb, LPCWSTR lpszObjectName, LPCWSTR lpszVersion,
	LPCWSTR lpszReferrer , LPCWSTR *lpszAcceptTypes,
	DWORD dwFlags, DWORD_PTR dwContext);
BOOL HTTP_FinishedReading(LPWININETHTTPREQW lpwhr);

VOID SendAsyncCallback(LPWININETHANDLEHEADER hdr, DWORD_PTR dwContext,
                       DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                       DWORD dwStatusInfoLength);

VOID INTERNET_SendCallback(LPWININETHANDLEHEADER hdr, DWORD_PTR dwContext,
                           DWORD dwInternetStatus, LPVOID lpvStatusInfo,
                           DWORD dwStatusInfoLength);

LPHTTPHEADERW HTTP_GetHeader(LPWININETHTTPREQW lpwhr, LPCWSTR header);

BOOL NETCON_connected(WININET_NETCONNECTION *connection);
BOOL NETCON_init(WININET_NETCONNECTION *connnection, BOOL useSSL);
BOOL NETCON_create(WININET_NETCONNECTION *connection, int domain,
	      int type, int protocol);
BOOL NETCON_close(WININET_NETCONNECTION *connection);
BOOL NETCON_connect(WININET_NETCONNECTION *connection, const struct sockaddr *serv_addr,
		    unsigned int addrlen);
BOOL NETCON_secure_connect(WININET_NETCONNECTION *connection, LPCWSTR hostname);
BOOL NETCON_send(WININET_NETCONNECTION *connection, const void *msg, size_t len, int flags,
		int *sent /* out */);
BOOL NETCON_recv(WININET_NETCONNECTION *connection, void *buf, size_t len, int flags,
		int *recvd /* out */);
BOOL NETCON_query_data_available(WININET_NETCONNECTION *connection, DWORD *available);
BOOL NETCON_getNextLine(WININET_NETCONNECTION *connection, LPSTR lpszBuffer, LPDWORD dwBuffer);
LPCVOID NETCON_GetCert(WININET_NETCONNECTION *connection);
DWORD NETCON_set_timeout(WININET_NETCONNECTION *connection, BOOL send, int value);

extern void URLCacheContainers_CreateDefaults(void);
extern void URLCacheContainers_DeleteAll(void);

#define MAX_REPLY_LEN	 	0x5B4

/* Used for debugging - maybe need to be shared in the Wine debugging code ? */
typedef struct
{
    DWORD val;
    const char* name;
} wininet_flag_info;

#endif /* _WINE_INTERNET_H_ */
