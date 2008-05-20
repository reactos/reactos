/*
 * Copyright (C) 2007 Francois Gouget
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

#ifndef __WINE_WINHTTP_H
#define __WINE_WINHTTP_H

#define WINHTTPAPI
#define BOOLAPI WINHTTPAPI BOOL WINAPI


typedef LPVOID HINTERNET;
typedef HINTERNET *LPHINTERNET;

#define INTERNET_DEFAULT_PORT           0
#define INTERNET_DEFAULT_HTTP_PORT      80
#define INTERNET_DEFAULT_HTTPS_PORT     443
typedef WORD INTERNET_PORT;
typedef INTERNET_PORT *LPINTERNET_PORT;

#define INTERNET_SCHEME_HTTP            1
#define INTERNET_SCHEME_HTTPS           2
typedef int INTERNET_SCHEME, *LPINTERNET_SCHEME;

/* flags for WinHttpOpen */
#define WINHTTP_FLAG_ASYNC                  0x10000000

/* flags for WinHttpOpenRequest */
#define WINHTTP_FLAG_ESCAPE_PERCENT         0x00000004
#define WINHTTP_FLAG_NULL_CODEPAGE          0x00000008
#define WINHTTP_FLAG_ESCAPE_DISABLE         0x00000040
#define WINHTTP_FLAG_ESCAPE_DISABLE_QUERY   0x00000080
#define WINHTTP_FLAG_BYPASS_PROXY_CACHE     0x00000100
#define WINHTTP_FLAG_REFRESH                WINHTTP_FLAG_BYPASS_PROXY_CACHE
#define WINHTTP_FLAG_SECURE                 0x00800000

#define WINHTTP_ACCESS_TYPE_DEFAULT_PROXY   0
#define WINHTTP_ACCESS_TYPE_NO_PROXY        1
#define WINHTTP_ACCESS_TYPE_NAMED_PROXY     3

#define WINHTTP_NO_PROXY_NAME               NULL
#define WINHTTP_NO_PROXY_BYPASS             NULL

#define WINHTTP_NO_REFERER                  NULL
#define WINHTTP_DEFAULT_ACCEPT_TYPES        NULL

#define WINHTTP_ERROR_BASE                  12000

#define ERROR_WINHTTP_AUTODETECTION_FAILED     (WINHTTP_ERROR_BASE + 180)

typedef struct
{
    DWORD   dwStructSize;
    LPWSTR  lpszScheme;
    DWORD   dwSchemeLength;
    INTERNET_SCHEME nScheme;
    LPWSTR  lpszHostName;
    DWORD   dwHostNameLength;
    INTERNET_PORT nPort;
    LPWSTR  lpszUserName;
    DWORD   dwUserNameLength;
    LPWSTR  lpszPassword;
    DWORD   dwPasswordLength;
    LPWSTR  lpszUrlPath;
    DWORD   dwUrlPathLength;
    LPWSTR  lpszExtraInfo;
    DWORD   dwExtraInfoLength;
} URL_COMPONENTS, *LPURL_COMPONENTS;
typedef URL_COMPONENTS URL_COMPONENTSW;
typedef LPURL_COMPONENTS LPURL_COMPONENTSW;

typedef struct
{
    DWORD_PTR dwResult;
    DWORD dwError;
} WINHTTP_ASYNC_RESULT, *LPWINHTTP_ASYNC_RESULT;

typedef struct
{
    FILETIME ftExpiry;
    FILETIME ftStart;
    LPWSTR lpszSubjectInfo;
    LPWSTR lpszIssuerInfo;
    LPWSTR lpszProtocolName;
    LPWSTR lpszSignatureAlgName;
    LPWSTR lpszEncryptionAlgName;
    DWORD dwKeySize;
} WINHTTP_CERTIFICATE_INFO;

typedef struct
{
    DWORD dwAccessType;
    LPCWSTR lpszProxy;
    LPCWSTR lpszProxyBypass;
} WINHTTP_PROXY_INFO, *LPWINHTTP_PROXY_INFO;
typedef WINHTTP_PROXY_INFO WINHTTP_PROXY_INFOW;
typedef LPWINHTTP_PROXY_INFO LPWINHTTP_PROXY_INFOW;

typedef struct
{
    BOOL   fAutoDetect;
    LPWSTR lpszAutoConfigUrl;
    LPWSTR lpszProxy;
    LPWSTR lpszProxyBypass;
} WINHTTP_CURRENT_USER_IE_PROXY_CONFIG;

typedef VOID (CALLBACK *WINHTTP_STATUS_CALLBACK)(HINTERNET,DWORD_PTR,DWORD,LPVOID,DWORD);

typedef struct
{
    DWORD dwFlags;
    DWORD dwAutoDetectFlags;
    LPCWSTR lpszAutoConfigUrl;
    LPVOID lpvReserved;
    DWORD dwReserved;
    BOOL fAutoLogonIfChallenged;
} WINHTTP_AUTOPROXY_OPTIONS;

typedef struct
{
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
} HTTP_VERSION_INFO, *LPHTTP_VERSION_INFO;


#ifdef __cplusplus
extern "C" {
#endif

BOOL        WINAPI WinHttpAddRequestHeaders(HINTERNET,LPCWSTR,DWORD,DWORD);
BOOL        WINAPI WinHttpDetectAutoProxyConfigUrl(DWORD,LPWSTR*);
BOOL        WINAPI WinHttpCheckPlatform(void);
BOOL        WINAPI WinHttpCloseHandle(HINTERNET);
HINTERNET   WINAPI WinHttpConnect(HINTERNET,LPCWSTR,INTERNET_PORT,DWORD);
BOOL        WINAPI WinHttpCrackUrl(LPCWSTR,DWORD,DWORD,LPURL_COMPONENTS);
BOOL        WINAPI WinHttpCreateUrl(LPURL_COMPONENTS,DWORD,LPWSTR,LPDWORD);
BOOL        WINAPI WinHttpGetDefaultProxyConfiguration(WINHTTP_PROXY_INFO*);
BOOL        WINAPI WinHttpGetIEProxyConfigForCurrentUser(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* config);
BOOL        WINAPI WinHttpGetProxyForUrl(HINTERNET,LPCWSTR,WINHTTP_AUTOPROXY_OPTIONS*,WINHTTP_PROXY_INFO*);
HINTERNET   WINAPI WinHttpOpen(LPCWSTR,DWORD,LPCWSTR,LPCWSTR,DWORD);
HINTERNET   WINAPI WinHttpOpenRequest(HINTERNET,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR,DWORD);
BOOL        WINAPI WinHttpQueryAuthParams(HINTERNET,DWORD,LPVOID*);
BOOL        WINAPI WinHttpQueryAuthSchemes(HINTERNET,LPDWORD,LPDWORD,LPDWORD);
BOOL        WINAPI WinHttpQueryDataAvailable(HINTERNET,LPDWORD);
BOOL        WINAPI WinHttpQueryHeaders(HINTERNET,DWORD,LPCWSTR,LPVOID,LPDWORD,LPDWORD);
BOOL        WINAPI WinHttpReadData(HINTERNET,LPVOID,DWORD,LPDWORD);
BOOL        WINAPI WinHttpReceiveResponse(HINTERNET,LPVOID);
BOOL        WINAPI WinHttpSendRequest(HINTERNET,LPCWSTR,DWORD,LPVOID,DWORD,DWORD,DWORD_PTR);
BOOL        WINAPI WinHttpSetDefaultProxyConfiguration(WINHTTP_PROXY_INFO*);
BOOL        WINAPI WinHttpSetCredentials(HINTERNET,DWORD,DWORD,LPCWSTR,LPCWSTR,LPVOID);
BOOL        WINAPI WinHttpSetOption(HINTERNET,DWORD,LPVOID,DWORD);
WINHTTP_STATUS_CALLBACK WINAPI WinHttpSetStatusCallback(HINTERNET,WINHTTP_STATUS_CALLBACK,DWORD,DWORD_PTR);
BOOL        WINAPI WinHttpSetTimeouts(HINTERNET,int,int,int,int);
BOOL        WINAPI WinHttpTimeFromSystemTime(CONST SYSTEMTIME *,LPWSTR);
BOOL        WINAPI WinHttpTimeToSystemTime(LPCWSTR,SYSTEMTIME*);
BOOL        WINAPI WinHttpWriteData(HINTERNET,LPCVOID,DWORD,LPDWORD);

#ifdef __cplusplus
}
#endif

#endif  /* __WINE_WINHTTP_H */
