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

#ifndef _WINE_WININETI_H_
#define _WINE_WININETI_H_

/* FIXME: #include <iedial.h> */
#include <schannel.h>
#include <sspi.h>

typedef struct _INTERNET_CACHE_CONFIG_PATH_ENTRYA
{
    CHAR CachePath[MAX_PATH];
    DWORD dwCacheSize;
} INTERNET_CACHE_CONFIG_PATH_ENTRYA, *LPINTERNET_CACHE_CONFIG_PATH_ENTRYA;

typedef struct _INTERNET_CACHE_CONFIG_PATH_ENTRYW
{
    WCHAR CachePath[MAX_PATH];
    DWORD dwCacheSize;
} INTERNET_CACHE_CONFIG_PATH_ENTRYW, *LPINTERNET_CACHE_CONFIG_PATH_ENTRYW;

DECL_WINELIB_TYPE_AW(INTERNET_CACHE_CONFIG_PATH_ENTRY)
DECL_WINELIB_TYPE_AW(LPINTERNET_CACHE_CONFIG_PATH_ENTRY)

typedef struct _INTERNET_CACHE_CONFIG_INFOA
{
    DWORD dwStructSize;
    DWORD dwContainer;
    DWORD dwQuota;
    DWORD dwReserved4;
    BOOL fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    __C89_NAMELESS union
    {
        __C89_NAMELESS struct
        {
            CHAR CachePath[MAX_PATH];
            DWORD dwCacheSize;
        } __C89_NAMELESSSTRUCTNAME;
        INTERNET_CACHE_CONFIG_PATH_ENTRYA CachePaths[ANYSIZE_ARRAY];
    } __C89_NAMELESSUNIONNAME;
    DWORD dwNormalUsage;
    DWORD dwExemptUsage;
} INTERNET_CACHE_CONFIG_INFOA, *LPINTERNET_CACHE_CONFIG_INFOA;

typedef struct _INTERNET_CACHE_CONFIG_INFOW
{
    DWORD dwStructSize;
    DWORD dwContainer;
    DWORD dwQuota;
    DWORD dwReserved4;
    BOOL  fPerUser;
    DWORD dwSyncMode;
    DWORD dwNumCachePaths;
    __C89_NAMELESS union
    {
        __C89_NAMELESS struct
        {
            WCHAR CachePath[MAX_PATH];
            DWORD dwCacheSize;
        } __C89_NAMELESSSTRUCTNAME;
        INTERNET_CACHE_CONFIG_PATH_ENTRYW CachePaths[ANYSIZE_ARRAY];
    } __C89_NAMELESSUNIONNAME;
    DWORD dwNormalUsage;
    DWORD dwExemptUsage;
} INTERNET_CACHE_CONFIG_INFOW, *LPINTERNET_CACHE_CONFIG_INFOW;

DECL_WINELIB_TYPE_AW(INTERNET_CACHE_CONFIG_INFO)
DECL_WINELIB_TYPE_AW(LPINTERNET_CACHE_CONFIG_INFO)

typedef enum {
    WININET_SYNC_MODE_NEVER = 0,
    WININET_SYNC_MODE_ON_EXPIRY,
    WININET_SYNC_MODE_ONCE_PER_SESSION,
    WININET_SYNC_MODE_ALWAYS,
    WININET_SYNC_MODE_AUTOMATIC,
    WININET_SYNC_MODE_DEFAULT = WININET_SYNC_MODE_AUTOMATIC
} WININET_SYNC_MODE;

/* Flags for GetUrlCacheConfigInfoA/W and SetUrlCacheConfigInfoA/W */
#define CACHE_CONFIG_FORCE_CLEANUP_FC        0x00000020
#define CACHE_CONFIG_DISK_CACHE_PATHS_FC     0x00000040
#define CACHE_CONFIG_SYNC_MODE_FC            0x00000080
#define CACHE_CONFIG_CONTENT_PATHS_FC        0x00000100
#define CACHE_CONFIG_COOKIES_PATHS_FC        0x00000200
#define CACHE_CONFIG_HISTORY_PATHS_FC        0x00000400
#define CACHE_CONFIG_QUOTA_FC                0x00000800
#define CACHE_CONFIG_USER_MODE_FC            0x00001000
#define CACHE_CONFIG_CONTENT_USAGE_FC        0x00002000
#define CACHE_CONFIG_STICKY_CONTENT_USAGE_FC 0x00004000

#define ERROR_INTERNET_NO_NEW_CONTAINERS            (INTERNET_ERROR_BASE + 51)
#define ERROR_INTERNET_SOURCE_PORT_IN_USE           (INTERNET_ERROR_BASE + 58)
#define ERROR_INTERNET_INSECURE_FALLBACK_REQUIRED   (INTERNET_ERROR_BASE + 59)
#define ERROR_INTERNET_PROXY_ALERT                  (INTERNET_ERROR_BASE + 61)
#define ERROR_INTERNET_NO_CM_CONNECTION             (INTERNET_ERROR_BASE + 80)
#define ERROR_HTTP_PUSH_STATUS_CODE_NOT_SUPPORTED   (INTERNET_ERROR_BASE + 147)
#define ERROR_HTTP_PUSH_RETRY_NOT_SUPPORTED         (INTERNET_ERROR_BASE + 148)
#define ERROR_HTTP_PUSH_ENABLE_FAILED               (INTERNET_ERROR_BASE + 149)

#define ERROR_INTERNET_OFFLINE                      ERROR_INTERNET_DISCONNECTED

#ifdef __cplusplus
extern "C" {
#endif

DWORD       WINAPI DeleteIE3Cache(HWND,HINSTANCE,LPSTR,int);
BOOL        WINAPI GetDiskInfoA(PCSTR,PDWORD,PDWORDLONG,PDWORDLONG);
BOOL        WINAPI GetUrlCacheConfigInfoA(LPINTERNET_CACHE_CONFIG_INFOA,LPDWORD,DWORD);
BOOL        WINAPI GetUrlCacheConfigInfoW(LPINTERNET_CACHE_CONFIG_INFOW,LPDWORD,DWORD);
#define     GetUrlCacheConfigInfo WINELIB_NAME_AW(GetUrlCacheConfigInfo)
BOOL        WINAPI IncrementUrlCacheHeaderData(DWORD,LPDWORD);
BOOL        WINAPI InternetQueryFortezzaStatus(DWORD*,DWORD_PTR);
BOOL        WINAPI IsUrlCacheEntryExpiredA(LPCSTR,DWORD,FILETIME*);
BOOL        WINAPI IsUrlCacheEntryExpiredW(LPCWSTR,DWORD,FILETIME*);
#define     IsUrlCacheEntryExpired WINELIB_NAME_AW(IsUrlCacheEntryExpired)
BOOL        WINAPI SetUrlCacheConfigInfoA(LPINTERNET_CACHE_CONFIG_INFOA,DWORD);
BOOL        WINAPI SetUrlCacheConfigInfoW(LPINTERNET_CACHE_CONFIG_INFOW,DWORD);
#define     SetUrlCacheConfigInfo WINELIB_NAME_AW(SetUrlCacheConfigInfo)
BOOL        WINAPI InternetGetSecurityInfoByURLA(LPSTR,PCCERT_CHAIN_CONTEXT*,DWORD*);
BOOL        WINAPI InternetGetSecurityInfoByURLW(LPCWSTR,PCCERT_CHAIN_CONTEXT*,DWORD*);
#define     InternetGetSecurityInfoByURL WINELIB_NAME_AW(InternetGetSecurityInfoByURL)

#ifdef __cplusplus
}
#endif

#endif /* _WINE_WININETI_H_ */
