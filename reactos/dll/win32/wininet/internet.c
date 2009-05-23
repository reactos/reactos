/*
 * Wininet
 *
 * Copyright 1999 Corel Corporation
 * Copyright 2002 CodeWeavers Inc.
 * Copyright 2002 Jaco Greeff
 * Copyright 2002 TransGaming Technologies Inc.
 * Copyright 2004 Mike McCormack for CodeWeavers
 *
 * Ulrich Czekalla
 * Aric Stewart
 * David Hammerton
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

#include "config.h"
#include "wine/port.h"

#define MAXHOSTNAME 100 /* from http.c */

#if defined(__MINGW32__) || defined (_MSC_VER)
#include <ws2tcpip.h>
#endif

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#ifdef HAVE_SYS_POLL_H
# include <sys/poll.h>
#endif
#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <stdlib.h>
#include <ctype.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <assert.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "wininet.h"
#include "winineti.h"
#include "winnls.h"
#include "wine/debug.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "wine/exception.h"

#include "internet.h"
#include "resource.h"

#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

#define RESPONSE_TIMEOUT        30

typedef struct
{
    DWORD  dwError;
    CHAR   response[MAX_REPLY_LEN];
} WITHREADERROR, *LPWITHREADERROR;

static DWORD g_dwTlsErrIndex = TLS_OUT_OF_INDEXES;
static HMODULE WININET_hModule;

#define HANDLE_CHUNK_SIZE 0x10

static CRITICAL_SECTION WININET_cs;
static CRITICAL_SECTION_DEBUG WININET_cs_debug = 
{
    0, 0, &WININET_cs,
    { &WININET_cs_debug.ProcessLocksList, &WININET_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": WININET_cs") }
};
static CRITICAL_SECTION WININET_cs = { &WININET_cs_debug, -1, 0, 0, 0, 0 };

static LPWININETHANDLEHEADER *WININET_Handles;
static UINT WININET_dwNextHandle;
static UINT WININET_dwMaxHandles;

HINTERNET WININET_AllocHandle( LPWININETHANDLEHEADER info )
{
    LPWININETHANDLEHEADER *p;
    UINT handle = 0, num;

    list_init( &info->children );

    EnterCriticalSection( &WININET_cs );
    if( !WININET_dwMaxHandles )
    {
        num = HANDLE_CHUNK_SIZE;
        p = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, 
                   sizeof (*WININET_Handles)* num);
        if( !p )
            goto end;
        WININET_Handles = p;
        WININET_dwMaxHandles = num;
    }
    if( WININET_dwMaxHandles == WININET_dwNextHandle )
    {
        num = WININET_dwMaxHandles + HANDLE_CHUNK_SIZE;
        p = HeapReAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                   WININET_Handles, sizeof (*WININET_Handles)* num);
        if( !p )
            goto end;
        WININET_Handles = p;
        WININET_dwMaxHandles = num;
    }

    handle = WININET_dwNextHandle;
    if( WININET_Handles[handle] )
        ERR("handle isn't free but should be\n");
    WININET_Handles[handle] = WININET_AddRef( info );

    while( WININET_Handles[WININET_dwNextHandle] && 
           (WININET_dwNextHandle < WININET_dwMaxHandles ) )
        WININET_dwNextHandle++;
    
end:
    LeaveCriticalSection( &WININET_cs );

    return info->hInternet = (HINTERNET) (handle+1);
}

LPWININETHANDLEHEADER WININET_AddRef( LPWININETHANDLEHEADER info )
{
    ULONG refs = InterlockedIncrement(&info->refs);
    TRACE("%p -> refcount = %d\n", info, refs );
    return info;
}

LPWININETHANDLEHEADER WININET_GetObject( HINTERNET hinternet )
{
    LPWININETHANDLEHEADER info = NULL;
    UINT handle = (UINT) hinternet;

    EnterCriticalSection( &WININET_cs );

    if( (handle > 0) && ( handle <= WININET_dwMaxHandles ) && 
        WININET_Handles[handle-1] )
        info = WININET_AddRef( WININET_Handles[handle-1] );

    LeaveCriticalSection( &WININET_cs );

    TRACE("handle %d -> %p\n", handle, info);

    return info;
}

BOOL WININET_Release( LPWININETHANDLEHEADER info )
{
    ULONG refs = InterlockedDecrement(&info->refs);
    TRACE( "object %p refcount = %d\n", info, refs );
    if( !refs )
    {
        if ( info->vtbl->CloseConnection )
        {
            TRACE( "closing connection %p\n", info);
            info->vtbl->CloseConnection( info );
        }
        /* Don't send a callback if this is a session handle created with InternetOpenUrl */
        if ((info->htype != WH_HHTTPSESSION && info->htype != WH_HFTPSESSION)
            || !(info->dwInternalFlags & INET_OPENURL))
        {
            INTERNET_SendCallback(info, info->dwContext,
                                  INTERNET_STATUS_HANDLE_CLOSING, &info->hInternet,
                                  sizeof(HINTERNET));
        }
        TRACE( "destroying object %p\n", info);
        if ( info->htype != WH_HINIT )
            list_remove( &info->entry );
        info->vtbl->Destroy( info );
    }
    return TRUE;
}

BOOL WININET_FreeHandle( HINTERNET hinternet )
{
    BOOL ret = FALSE;
    UINT handle = (UINT) hinternet;
    LPWININETHANDLEHEADER info = NULL, child, next;

    EnterCriticalSection( &WININET_cs );

    if( (handle > 0) && ( handle <= WININET_dwMaxHandles ) )
    {
        handle--;
        if( WININET_Handles[handle] )
        {
            info = WININET_Handles[handle];
            TRACE( "destroying handle %d for object %p\n", handle+1, info);
            WININET_Handles[handle] = NULL;
            ret = TRUE;
        }
    }

    LeaveCriticalSection( &WININET_cs );

    /* As on native when the equivalent of WININET_Release is called, the handle
     * is already invalid, but if a new handle is created at this time it does
     * not yet get assigned the freed handle number */
    if( info )
    {
        /* Free all children as native does */
        LIST_FOR_EACH_ENTRY_SAFE( child, next, &info->children, WININETHANDLEHEADER, entry )
        {
            TRACE( "freeing child handle %d for parent handle %d\n",
                   (UINT)child->hInternet, handle+1);
            WININET_FreeHandle( child->hInternet );
        }
        WININET_Release( info );
    }

    EnterCriticalSection( &WININET_cs );

    if( WININET_dwNextHandle > handle && !WININET_Handles[handle] )
        WININET_dwNextHandle = handle;

    LeaveCriticalSection( &WININET_cs );

    return ret;
}

/***********************************************************************
 * DllMain [Internal] Initializes the internal 'WININET.DLL'.
 *
 * PARAMS
 *     hinstDLL    [I] handle to the DLL's instance
 *     fdwReason   [I]
 *     lpvReserved [I] reserved, must be NULL
 *
 * RETURNS
 *     Success: TRUE
 *     Failure: FALSE
 */

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:

            g_dwTlsErrIndex = TlsAlloc();

	    if (g_dwTlsErrIndex == TLS_OUT_OF_INDEXES)
		return FALSE;

            URLCacheContainers_CreateDefaults();

            WININET_hModule = hinstDLL;

        case DLL_THREAD_ATTACH:
	    break;

        case DLL_THREAD_DETACH:
	    if (g_dwTlsErrIndex != TLS_OUT_OF_INDEXES)
			{
				LPVOID lpwite = TlsGetValue(g_dwTlsErrIndex);
                                HeapFree(GetProcessHeap(), 0, lpwite);
			}
	    break;

        case DLL_PROCESS_DETACH:

	    URLCacheContainers_DeleteAll();

	    if (g_dwTlsErrIndex != TLS_OUT_OF_INDEXES)
	    {
	        HeapFree(GetProcessHeap(), 0, TlsGetValue(g_dwTlsErrIndex));
	        TlsFree(g_dwTlsErrIndex);
	    }
            break;
    }

    return TRUE;
}


/***********************************************************************
 *           InternetInitializeAutoProxyDll   (WININET.@)
 *
 * Setup the internal proxy
 *
 * PARAMETERS
 *     dwReserved
 *
 * RETURNS
 *     FALSE on failure
 *
 */
BOOL WINAPI InternetInitializeAutoProxyDll(DWORD dwReserved)
{
    FIXME("STUB\n");
    INTERNET_SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/***********************************************************************
 *           DetectAutoProxyUrl   (WININET.@)
 *
 * Auto detect the proxy url
 *
 * RETURNS
 *     FALSE on failure
 *
 */
BOOL WINAPI DetectAutoProxyUrl(LPSTR lpszAutoProxyUrl,
	DWORD dwAutoProxyUrlLength, DWORD dwDetectFlags)
{
    FIXME("STUB\n");
    INTERNET_SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/***********************************************************************
 *           INTERNET_ConfigureProxy
 *
 * FIXME:
 * The proxy may be specified in the form 'http=proxy.my.org'
 * Presumably that means there can be ftp=ftpproxy.my.org too.
 */
static BOOL INTERNET_ConfigureProxy( LPWININETAPPINFOW lpwai )
{
    HKEY key;
    DWORD type, len, enabled = 0;
    LPCSTR envproxy;
    static const WCHAR szInternetSettings[] =
        { 'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
          'W','i','n','d','o','w','s','\\','C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
          'I','n','t','e','r','n','e','t',' ','S','e','t','t','i','n','g','s',0 };
    static const WCHAR szProxyServer[] = { 'P','r','o','x','y','S','e','r','v','e','r', 0 };
    static const WCHAR szProxyEnable[] = { 'P','r','o','x','y','E','n','a','b','l','e', 0 };

    if (RegOpenKeyW( HKEY_CURRENT_USER, szInternetSettings, &key )) return FALSE;

    len = sizeof enabled;
    if (RegQueryValueExW( key, szProxyEnable, NULL, &type, (BYTE *)&enabled, &len ) || type != REG_DWORD)
        RegSetValueExW( key, szProxyEnable, 0, REG_DWORD, (BYTE *)&enabled, sizeof(REG_DWORD) );

    if (enabled)
    {
        TRACE("Proxy is enabled.\n");

        /* figure out how much memory the proxy setting takes */
        if (!RegQueryValueExW( key, szProxyServer, NULL, &type, NULL, &len ) && len && (type == REG_SZ))
        {
            LPWSTR szProxy, p;
            static const WCHAR szHttp[] = {'h','t','t','p','=',0};

            if (!(szProxy = HeapAlloc( GetProcessHeap(), 0, len )))
            {
                RegCloseKey( key );
                return FALSE;
            }
            RegQueryValueExW( key, szProxyServer, NULL, &type, (BYTE*)szProxy, &len );

            /* find the http proxy, and strip away everything else */
            p = strstrW( szProxy, szHttp );
            if (p)
            {
                p += lstrlenW( szHttp );
                lstrcpyW( szProxy, p );
            }
            p = strchrW( szProxy, ' ' );
            if (p) *p = 0;

            lpwai->dwAccessType = INTERNET_OPEN_TYPE_PROXY;
            lpwai->lpszProxy = szProxy;

            TRACE("http proxy = %s\n", debugstr_w(lpwai->lpszProxy));
        }
        else
            ERR("Couldn't read proxy server settings from registry.\n");
    }
    else if ((envproxy = getenv( "http_proxy" )))
    {
        WCHAR *envproxyW;

        len = MultiByteToWideChar( CP_UNIXCP, 0, envproxy, -1, NULL, 0 );
        if (!(envproxyW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR)))) return FALSE;
        MultiByteToWideChar( CP_UNIXCP, 0, envproxy, -1, envproxyW, len );

        lpwai->dwAccessType = INTERNET_OPEN_TYPE_PROXY;
        lpwai->lpszProxy = envproxyW;

        TRACE("http proxy (from environment) = %s\n", debugstr_w(lpwai->lpszProxy));
        enabled = 1;
    }
    if (!enabled)
    {
        TRACE("Proxy is not enabled.\n");
        lpwai->dwAccessType = INTERNET_OPEN_TYPE_DIRECT;
    }
    RegCloseKey( key );
    return (enabled > 0);
}

/***********************************************************************
 *           dump_INTERNET_FLAGS
 *
 * Helper function to TRACE the internet flags.
 *
 * RETURNS
 *    None
 *
 */
static void dump_INTERNET_FLAGS(DWORD dwFlags) 
{
#define FE(x) { x, #x }
    static const wininet_flag_info flag[] = {
        FE(INTERNET_FLAG_RELOAD),
        FE(INTERNET_FLAG_RAW_DATA),
        FE(INTERNET_FLAG_EXISTING_CONNECT),
        FE(INTERNET_FLAG_ASYNC),
        FE(INTERNET_FLAG_PASSIVE),
        FE(INTERNET_FLAG_NO_CACHE_WRITE),
        FE(INTERNET_FLAG_MAKE_PERSISTENT),
        FE(INTERNET_FLAG_FROM_CACHE),
        FE(INTERNET_FLAG_SECURE),
        FE(INTERNET_FLAG_KEEP_CONNECTION),
        FE(INTERNET_FLAG_NO_AUTO_REDIRECT),
        FE(INTERNET_FLAG_READ_PREFETCH),
        FE(INTERNET_FLAG_NO_COOKIES),
        FE(INTERNET_FLAG_NO_AUTH),
        FE(INTERNET_FLAG_CACHE_IF_NET_FAIL),
        FE(INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTP),
        FE(INTERNET_FLAG_IGNORE_REDIRECT_TO_HTTPS),
        FE(INTERNET_FLAG_IGNORE_CERT_DATE_INVALID),
        FE(INTERNET_FLAG_IGNORE_CERT_CN_INVALID),
        FE(INTERNET_FLAG_RESYNCHRONIZE),
        FE(INTERNET_FLAG_HYPERLINK),
        FE(INTERNET_FLAG_NO_UI),
        FE(INTERNET_FLAG_PRAGMA_NOCACHE),
        FE(INTERNET_FLAG_CACHE_ASYNC),
        FE(INTERNET_FLAG_FORMS_SUBMIT),
        FE(INTERNET_FLAG_NEED_FILE),
        FE(INTERNET_FLAG_TRANSFER_ASCII),
        FE(INTERNET_FLAG_TRANSFER_BINARY)
    };
#undef FE
    unsigned int i;

    for (i = 0; i < (sizeof(flag) / sizeof(flag[0])); i++) {
	if (flag[i].val & dwFlags) {
	    TRACE(" %s", flag[i].name);
	    dwFlags &= ~flag[i].val;
	}
    }	
    if (dwFlags)
        TRACE(" Unknown flags (%08x)\n", dwFlags);
    else
        TRACE("\n");
}

/***********************************************************************
 *           INTERNET_CloseHandle (internal)
 *
 * Close internet handle
 *
 */
static VOID APPINFO_Destroy(WININETHANDLEHEADER *hdr)
{
    LPWININETAPPINFOW lpwai = (LPWININETAPPINFOW) hdr;

    TRACE("%p\n",lpwai);

    HeapFree(GetProcessHeap(), 0, lpwai->lpszAgent);
    HeapFree(GetProcessHeap(), 0, lpwai->lpszProxy);
    HeapFree(GetProcessHeap(), 0, lpwai->lpszProxyBypass);
    HeapFree(GetProcessHeap(), 0, lpwai->lpszProxyUsername);
    HeapFree(GetProcessHeap(), 0, lpwai->lpszProxyPassword);
    HeapFree(GetProcessHeap(), 0, lpwai);
}

static DWORD APPINFO_QueryOption(WININETHANDLEHEADER *hdr, DWORD option, void *buffer, DWORD *size, BOOL unicode)
{
    LPWININETAPPINFOW ai = (LPWININETAPPINFOW)hdr;

    switch(option) {
    case INTERNET_OPTION_HANDLE_TYPE:
        TRACE("INTERNET_OPTION_HANDLE_TYPE\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *size = sizeof(DWORD);
        *(DWORD*)buffer = INTERNET_HANDLE_TYPE_INTERNET;
        return ERROR_SUCCESS;

    case INTERNET_OPTION_USER_AGENT: {
        DWORD bufsize;

        TRACE("INTERNET_OPTION_USER_AGENT\n");

        bufsize = *size;

        if (unicode) {
            *size = (strlenW(ai->lpszAgent) + 1) * sizeof(WCHAR);
            if(!buffer || bufsize < *size)
                return ERROR_INSUFFICIENT_BUFFER;

            strcpyW(buffer, ai->lpszAgent);
        }else {
            *size = WideCharToMultiByte(CP_ACP, 0, ai->lpszAgent, -1, NULL, 0, NULL, NULL);
            if(!buffer || bufsize < *size)
                return ERROR_INSUFFICIENT_BUFFER;

            WideCharToMultiByte(CP_ACP, 0, ai->lpszAgent, -1, buffer, *size, NULL, NULL);
        }

        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_PROXY:
        if (unicode) {
            INTERNET_PROXY_INFOW *pi = (INTERNET_PROXY_INFOW *)buffer;
            DWORD proxyBytesRequired = 0, proxyBypassBytesRequired = 0;
            LPWSTR proxy, proxy_bypass;

            if (ai->lpszProxy)
                proxyBytesRequired = (lstrlenW(ai->lpszProxy) + 1) * sizeof(WCHAR);
            if (ai->lpszProxyBypass)
                proxyBypassBytesRequired = (lstrlenW(ai->lpszProxyBypass) + 1) * sizeof(WCHAR);
            if (*size < sizeof(INTERNET_PROXY_INFOW) + proxyBytesRequired + proxyBypassBytesRequired)
            {
                *size = sizeof(INTERNET_PROXY_INFOW) + proxyBytesRequired + proxyBypassBytesRequired;
                return ERROR_INSUFFICIENT_BUFFER;
            }
            proxy = (LPWSTR)((LPBYTE)buffer + sizeof(INTERNET_PROXY_INFOW));
            proxy_bypass = (LPWSTR)((LPBYTE)buffer + sizeof(INTERNET_PROXY_INFOW) + proxyBytesRequired);

            pi->dwAccessType = ai->dwAccessType;
            pi->lpszProxy = NULL;
            pi->lpszProxyBypass = NULL;
            if (ai->lpszProxy) {
                lstrcpyW(proxy, ai->lpszProxy);
                pi->lpszProxy = proxy;
            }

            if (ai->lpszProxyBypass) {
                lstrcpyW(proxy_bypass, ai->lpszProxyBypass);
                pi->lpszProxyBypass = proxy_bypass;
            }

            *size = sizeof(INTERNET_PROXY_INFOW) + proxyBytesRequired + proxyBypassBytesRequired;
            return ERROR_SUCCESS;
        }else {
            INTERNET_PROXY_INFOA *pi = (INTERNET_PROXY_INFOA *)buffer;
            DWORD proxyBytesRequired = 0, proxyBypassBytesRequired = 0;
            LPSTR proxy, proxy_bypass;

            if (ai->lpszProxy)
                proxyBytesRequired = WideCharToMultiByte(CP_ACP, 0, ai->lpszProxy, -1, NULL, 0, NULL, NULL);
            if (ai->lpszProxyBypass)
                proxyBypassBytesRequired = WideCharToMultiByte(CP_ACP, 0, ai->lpszProxyBypass, -1,
                        NULL, 0, NULL, NULL);
            if (*size < sizeof(INTERNET_PROXY_INFOA) + proxyBytesRequired + proxyBypassBytesRequired)
            {
                *size = sizeof(INTERNET_PROXY_INFOA) + proxyBytesRequired + proxyBypassBytesRequired;
                return ERROR_INSUFFICIENT_BUFFER;
            }
            proxy = (LPSTR)((LPBYTE)buffer + sizeof(INTERNET_PROXY_INFOA));
            proxy_bypass = (LPSTR)((LPBYTE)buffer + sizeof(INTERNET_PROXY_INFOA) + proxyBytesRequired);

            pi->dwAccessType = ai->dwAccessType;
            pi->lpszProxy = NULL;
            pi->lpszProxyBypass = NULL;
            if (ai->lpszProxy) {
                WideCharToMultiByte(CP_ACP, 0, ai->lpszProxy, -1, proxy, proxyBytesRequired, NULL, NULL);
                pi->lpszProxy = proxy;
            }

            if (ai->lpszProxyBypass) {
                WideCharToMultiByte(CP_ACP, 0, ai->lpszProxyBypass, -1, proxy_bypass,
                        proxyBypassBytesRequired, NULL, NULL);
                pi->lpszProxyBypass = proxy_bypass;
            }

            *size = sizeof(INTERNET_PROXY_INFOA) + proxyBytesRequired + proxyBypassBytesRequired;
            return ERROR_SUCCESS;
        }
    }

    return INET_QueryOption(option, buffer, size, unicode);
}

static const HANDLEHEADERVtbl APPINFOVtbl = {
    APPINFO_Destroy,
    NULL,
    APPINFO_QueryOption,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};


/***********************************************************************
 *           InternetOpenW   (WININET.@)
 *
 * Per-application initialization of wininet
 *
 * RETURNS
 *    HINTERNET on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetOpenW(LPCWSTR lpszAgent, DWORD dwAccessType,
    LPCWSTR lpszProxy, LPCWSTR lpszProxyBypass, DWORD dwFlags)
{
    LPWININETAPPINFOW lpwai = NULL;
    HINTERNET handle = NULL;

    if (TRACE_ON(wininet)) {
#define FE(x) { x, #x }
	static const wininet_flag_info access_type[] = {
	    FE(INTERNET_OPEN_TYPE_PRECONFIG),
	    FE(INTERNET_OPEN_TYPE_DIRECT),
	    FE(INTERNET_OPEN_TYPE_PROXY),
	    FE(INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY)
	};
#undef FE
	DWORD i;
	const char *access_type_str = "Unknown";
	
	TRACE("(%s, %i, %s, %s, %i)\n", debugstr_w(lpszAgent), dwAccessType,
	      debugstr_w(lpszProxy), debugstr_w(lpszProxyBypass), dwFlags);
	for (i = 0; i < (sizeof(access_type) / sizeof(access_type[0])); i++) {
	    if (access_type[i].val == dwAccessType) {
		access_type_str = access_type[i].name;
		break;
	    }
	}
	TRACE("  access type : %s\n", access_type_str);
	TRACE("  flags       :");
	dump_INTERNET_FLAGS(dwFlags);
    }

    /* Clear any error information */
    INTERNET_SetLastError(0);

    lpwai = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WININETAPPINFOW));
    if (NULL == lpwai)
    {
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lend;
    }

    lpwai->hdr.htype = WH_HINIT;
    lpwai->hdr.vtbl = &APPINFOVtbl;
    lpwai->hdr.dwFlags = dwFlags;
    lpwai->hdr.refs = 1;
    lpwai->dwAccessType = dwAccessType;
    lpwai->lpszProxyUsername = NULL;
    lpwai->lpszProxyPassword = NULL;

    handle = WININET_AllocHandle( &lpwai->hdr );
    if( !handle )
    {
        HeapFree( GetProcessHeap(), 0, lpwai );
        INTERNET_SetLastError(ERROR_OUTOFMEMORY);
	goto lend;
    }

    if (NULL != lpszAgent)
    {
        lpwai->lpszAgent = HeapAlloc( GetProcessHeap(),0,
                                      (strlenW(lpszAgent)+1)*sizeof(WCHAR));
        if (lpwai->lpszAgent)
            lstrcpyW( lpwai->lpszAgent, lpszAgent );
    }
    if(dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG)
        INTERNET_ConfigureProxy( lpwai );
    else if (NULL != lpszProxy)
    {
        lpwai->lpszProxy = HeapAlloc( GetProcessHeap(), 0,
                                      (strlenW(lpszProxy)+1)*sizeof(WCHAR));
        if (lpwai->lpszProxy)
            lstrcpyW( lpwai->lpszProxy, lpszProxy );
    }

    if (NULL != lpszProxyBypass)
    {
        lpwai->lpszProxyBypass = HeapAlloc( GetProcessHeap(), 0,
                                     (strlenW(lpszProxyBypass)+1)*sizeof(WCHAR));
        if (lpwai->lpszProxyBypass)
            lstrcpyW( lpwai->lpszProxyBypass, lpszProxyBypass );
    }

lend:
    if( lpwai )
        WININET_Release( &lpwai->hdr );

    TRACE("returning %p\n", lpwai);

    return handle;
}


/***********************************************************************
 *           InternetOpenA   (WININET.@)
 *
 * Per-application initialization of wininet
 *
 * RETURNS
 *    HINTERNET on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetOpenA(LPCSTR lpszAgent, DWORD dwAccessType,
    LPCSTR lpszProxy, LPCSTR lpszProxyBypass, DWORD dwFlags)
{
    HINTERNET rc = NULL;
    INT len;
    WCHAR *szAgent = NULL, *szProxy = NULL, *szBypass = NULL;

    TRACE("(%s, 0x%08x, %s, %s, 0x%08x)\n", debugstr_a(lpszAgent),
       dwAccessType, debugstr_a(lpszProxy), debugstr_a(lpszProxyBypass), dwFlags);

    if( lpszAgent )
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszAgent, -1, NULL, 0);
        szAgent = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszAgent, -1, szAgent, len);
    }

    if( lpszProxy )
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszProxy, -1, NULL, 0);
        szProxy = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszProxy, -1, szProxy, len);
    }

    if( lpszProxyBypass )
    {
        len = MultiByteToWideChar(CP_ACP, 0, lpszProxyBypass, -1, NULL, 0);
        szBypass = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszProxyBypass, -1, szBypass, len);
    }

    rc = InternetOpenW(szAgent, dwAccessType, szProxy, szBypass, dwFlags);

    HeapFree(GetProcessHeap(), 0, szAgent);
    HeapFree(GetProcessHeap(), 0, szProxy);
    HeapFree(GetProcessHeap(), 0, szBypass);

    return rc;
}

/***********************************************************************
 *           InternetGetLastResponseInfoA (WININET.@)
 *
 * Return last wininet error description on the calling thread
 *
 * RETURNS
 *    TRUE on success of writing to buffer
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetLastResponseInfoA(LPDWORD lpdwError,
    LPSTR lpszBuffer, LPDWORD lpdwBufferLength)
{
    LPWITHREADERROR lpwite = TlsGetValue(g_dwTlsErrIndex);

    TRACE("\n");

    if (lpwite)
    {
        *lpdwError = lpwite->dwError;
        if (lpwite->dwError)
        {
            memcpy(lpszBuffer, lpwite->response, *lpdwBufferLength);
            *lpdwBufferLength = strlen(lpszBuffer);
        }
        else
            *lpdwBufferLength = 0;
    }
    else
    {
        *lpdwError = 0;
        *lpdwBufferLength = 0;
    }

    return TRUE;
}

/***********************************************************************
 *           InternetGetLastResponseInfoW (WININET.@)
 *
 * Return last wininet error description on the calling thread
 *
 * RETURNS
 *    TRUE on success of writing to buffer
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetGetLastResponseInfoW(LPDWORD lpdwError,
    LPWSTR lpszBuffer, LPDWORD lpdwBufferLength)
{
    LPWITHREADERROR lpwite = TlsGetValue(g_dwTlsErrIndex);

    TRACE("\n");

    if (lpwite)
    {
        *lpdwError = lpwite->dwError;
        if (lpwite->dwError)
        {
            memcpy(lpszBuffer, lpwite->response, *lpdwBufferLength);
            *lpdwBufferLength = lstrlenW(lpszBuffer);
        }
        else
            *lpdwBufferLength = 0;
    }
    else
    {
        *lpdwError = 0;
        *lpdwBufferLength = 0;
    }

    return TRUE;
}

/***********************************************************************
 *           InternetGetConnectedState (WININET.@)
 *
 * Return connected state
 *
 * RETURNS
 *    TRUE if connected
 *    if lpdwStatus is not null, return the status (off line,
 *    modem, lan...) in it.
 *    FALSE if not connected
 */
BOOL WINAPI InternetGetConnectedState(LPDWORD lpdwStatus, DWORD dwReserved)
{
    TRACE("(%p, 0x%08x)\n", lpdwStatus, dwReserved);

    if (lpdwStatus) {
	WARN("always returning LAN connection.\n");
	*lpdwStatus = INTERNET_CONNECTION_LAN;
    }
    return TRUE;
}


/***********************************************************************
 *           InternetGetConnectedStateExW (WININET.@)
 *
 * Return connected state
 *
 * PARAMS
 *
 * lpdwStatus         [O] Flags specifying the status of the internet connection.
 * lpszConnectionName [O] Pointer to buffer to receive the friendly name of the internet connection.
 * dwNameLen          [I] Size of the buffer, in characters.
 * dwReserved         [I] Reserved. Must be set to 0.
 *
 * RETURNS
 *    TRUE if connected
 *    if lpdwStatus is not null, return the status (off line,
 *    modem, lan...) in it.
 *    FALSE if not connected
 *
 * NOTES
 *   If the system has no available network connections, an empty string is
 *   stored in lpszConnectionName. If there is a LAN connection, a localized
 *   "LAN Connection" string is stored. Presumably, if only a dial-up
 *   connection is available then the name of the dial-up connection is
 *   returned. Why any application, other than the "Internet Settings" CPL,
 *   would want to use this function instead of the simpler InternetGetConnectedStateW
 *   function is beyond me.
 */
BOOL WINAPI InternetGetConnectedStateExW(LPDWORD lpdwStatus, LPWSTR lpszConnectionName,
                                         DWORD dwNameLen, DWORD dwReserved)
{
    TRACE("(%p, %p, %d, 0x%08x)\n", lpdwStatus, lpszConnectionName, dwNameLen, dwReserved);

    /* Must be zero */
    if(dwReserved)
	return FALSE;

    if (lpdwStatus) {
        WARN("always returning LAN connection.\n");
        *lpdwStatus = INTERNET_CONNECTION_LAN;
    }
    return LoadStringW(WININET_hModule, IDS_LANCONNECTION, lpszConnectionName, dwNameLen);
}


/***********************************************************************
 *           InternetGetConnectedStateExA (WININET.@)
 */
BOOL WINAPI InternetGetConnectedStateExA(LPDWORD lpdwStatus, LPSTR lpszConnectionName,
                                         DWORD dwNameLen, DWORD dwReserved)
{
    LPWSTR lpwszConnectionName = NULL;
    BOOL rc;

    TRACE("(%p, %p, %d, 0x%08x)\n", lpdwStatus, lpszConnectionName, dwNameLen, dwReserved);

    if (lpszConnectionName && dwNameLen > 0)
        lpwszConnectionName= HeapAlloc(GetProcessHeap(), 0, dwNameLen * sizeof(WCHAR));

    rc = InternetGetConnectedStateExW(lpdwStatus,lpwszConnectionName, dwNameLen,
                                      dwReserved);
    if (rc && lpwszConnectionName)
    {
        WideCharToMultiByte(CP_ACP,0,lpwszConnectionName,-1,lpszConnectionName,
                            dwNameLen, NULL, NULL);

        HeapFree(GetProcessHeap(),0,lpwszConnectionName);
    }

    return rc;
}


/***********************************************************************
 *           InternetConnectW (WININET.@)
 *
 * Open a ftp, gopher or http session
 *
 * RETURNS
 *    HINTERNET a session handle on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetConnectW(HINTERNET hInternet,
    LPCWSTR lpszServerName, INTERNET_PORT nServerPort,
    LPCWSTR lpszUserName, LPCWSTR lpszPassword,
    DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext)
{
    LPWININETAPPINFOW hIC;
    HINTERNET rc = NULL;

    TRACE("(%p, %s, %i, %s, %s, %i, %i, %lx)\n", hInternet, debugstr_w(lpszServerName),
	  nServerPort, debugstr_w(lpszUserName), debugstr_w(lpszPassword),
	  dwService, dwFlags, dwContext);

    if (!lpszServerName)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }

    /* Clear any error information */
    INTERNET_SetLastError(0);
    hIC = (LPWININETAPPINFOW) WININET_GetObject( hInternet );
    if ( (hIC == NULL) || (hIC->hdr.htype != WH_HINIT) )
    {
        INTERNET_SetLastError(ERROR_INVALID_HANDLE);
        goto lend;
    }

    switch (dwService)
    {
        case INTERNET_SERVICE_FTP:
            rc = FTP_Connect(hIC, lpszServerName, nServerPort,
            lpszUserName, lpszPassword, dwFlags, dwContext, 0);
            break;

        case INTERNET_SERVICE_HTTP:
	    rc = HTTP_Connect(hIC, lpszServerName, nServerPort,
            lpszUserName, lpszPassword, dwFlags, dwContext, 0);
            break;

        case INTERNET_SERVICE_GOPHER:
        default:
            break;
    }
lend:
    if( hIC )
        WININET_Release( &hIC->hdr );

    TRACE("returning %p\n", rc);
    return rc;
}


/***********************************************************************
 *           InternetConnectA (WININET.@)
 *
 * Open a ftp, gopher or http session
 *
 * RETURNS
 *    HINTERNET a session handle on success
 *    NULL on failure
 *
 */
HINTERNET WINAPI InternetConnectA(HINTERNET hInternet,
    LPCSTR lpszServerName, INTERNET_PORT nServerPort,
    LPCSTR lpszUserName, LPCSTR lpszPassword,
    DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext)
{
    HINTERNET rc = NULL;
    INT len = 0;
    LPWSTR szServerName = NULL;
    LPWSTR szUserName = NULL;
    LPWSTR szPassword = NULL;

    if (lpszServerName)
    {
	len = MultiByteToWideChar(CP_ACP, 0, lpszServerName, -1, NULL, 0);
        szServerName = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszServerName, -1, szServerName, len);
    }
    if (lpszUserName)
    {
	len = MultiByteToWideChar(CP_ACP, 0, lpszUserName, -1, NULL, 0);
        szUserName = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszUserName, -1, szUserName, len);
    }
    if (lpszPassword)
    {
	len = MultiByteToWideChar(CP_ACP, 0, lpszPassword, -1, NULL, 0);
        szPassword = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpszPassword, -1, szPassword, len);
    }


    rc = InternetConnectW(hInternet, szServerName, nServerPort,
        szUserName, szPassword, dwService, dwFlags, dwContext);

    HeapFree(GetProcessHeap(), 0, szServerName);
    HeapFree(GetProcessHeap(), 0, szUserName);
    HeapFree(GetProcessHeap(), 0, szPassword);
    return rc;
}


/***********************************************************************
 *           InternetFindNextFileA (WININET.@)
 *
 * Continues a file search from a previous call to FindFirstFile
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetFindNextFileA(HINTERNET hFind, LPVOID lpvFindData)
{
    BOOL ret;
    WIN32_FIND_DATAW fd;
    
    ret = InternetFindNextFileW(hFind, lpvFindData?&fd:NULL);
    if(lpvFindData)
        WININET_find_data_WtoA(&fd, (LPWIN32_FIND_DATAA)lpvFindData);
    return ret;
}

/***********************************************************************
 *           InternetFindNextFileW (WININET.@)
 *
 * Continues a file search from a previous call to FindFirstFile
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetFindNextFileW(HINTERNET hFind, LPVOID lpvFindData)
{
    WININETHANDLEHEADER *hdr;
    DWORD res;

    TRACE("\n");

    hdr = WININET_GetObject(hFind);
    if(!hdr) {
        WARN("Invalid handle\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(hdr->vtbl->FindNextFileW) {
        res = hdr->vtbl->FindNextFileW(hdr, lpvFindData);
    }else {
        WARN("Handle doesn't support NextFile\n");
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
    }

    WININET_Release(hdr);

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           InternetCloseHandle (WININET.@)
 *
 * Generic close handle function
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCloseHandle(HINTERNET hInternet)
{
    LPWININETHANDLEHEADER lpwh;
    
    TRACE("%p\n",hInternet);

    lpwh = WININET_GetObject( hInternet );
    if (NULL == lpwh)
    {
        INTERNET_SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    WININET_Release( lpwh );
    WININET_FreeHandle( hInternet );

    return TRUE;
}


/***********************************************************************
 *           ConvertUrlComponentValue (Internal)
 *
 * Helper function for InternetCrackUrlA
 *
 */
static void ConvertUrlComponentValue(LPSTR* lppszComponent, LPDWORD dwComponentLen,
                                     LPWSTR lpwszComponent, DWORD dwwComponentLen,
                                     LPCSTR lpszStart, LPCWSTR lpwszStart)
{
    TRACE("%p %d %p %d %p %p\n", *lppszComponent, *dwComponentLen, lpwszComponent, dwwComponentLen, lpszStart, lpwszStart);
    if (*dwComponentLen != 0)
    {
        DWORD nASCIILength=WideCharToMultiByte(CP_ACP,0,lpwszComponent,dwwComponentLen,NULL,0,NULL,NULL);
        if (*lppszComponent == NULL)
        {
            int nASCIIOffset=WideCharToMultiByte(CP_ACP,0,lpwszStart,lpwszComponent-lpwszStart,NULL,0,NULL,NULL);
            if (lpwszComponent)
                *lppszComponent = (LPSTR)lpszStart+nASCIIOffset;
            else
                *lppszComponent = NULL;
            *dwComponentLen = nASCIILength;
        }
        else
        {
            DWORD ncpylen = min((*dwComponentLen)-1, nASCIILength);
            WideCharToMultiByte(CP_ACP,0,lpwszComponent,dwwComponentLen,*lppszComponent,ncpylen+1,NULL,NULL);
            (*lppszComponent)[ncpylen]=0;
            *dwComponentLen = ncpylen;
        }
    }
}


/***********************************************************************
 *           InternetCrackUrlA (WININET.@)
 *
 * See InternetCrackUrlW.
 */
BOOL WINAPI InternetCrackUrlA(LPCSTR lpszUrl, DWORD dwUrlLength, DWORD dwFlags,
    LPURL_COMPONENTSA lpUrlComponents)
{
  DWORD nLength;
  URL_COMPONENTSW UCW;
  BOOL ret = FALSE;
  WCHAR *lpwszUrl, *hostname = NULL, *username = NULL, *password = NULL, *path = NULL,
        *scheme = NULL, *extra = NULL;

  TRACE("(%s %u %x %p)\n",
        lpszUrl ? debugstr_an(lpszUrl, dwUrlLength ? dwUrlLength : strlen(lpszUrl)) : "(null)",
        dwUrlLength, dwFlags, lpUrlComponents);

  if (!lpszUrl || !*lpszUrl || !lpUrlComponents ||
          lpUrlComponents->dwStructSize != sizeof(URL_COMPONENTSA))
  {
      INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
  }

  if(dwUrlLength<=0)
      dwUrlLength=-1;
  nLength=MultiByteToWideChar(CP_ACP,0,lpszUrl,dwUrlLength,NULL,0);

  /* if dwUrlLength=-1 then nLength includes null but length to 
       InternetCrackUrlW should not include it                  */
  if (dwUrlLength == -1) nLength--;

  lpwszUrl=HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WCHAR)*nLength);
  MultiByteToWideChar(CP_ACP,0,lpszUrl,dwUrlLength,lpwszUrl,nLength);

  memset(&UCW,0,sizeof(UCW));
  UCW.dwStructSize = sizeof(URL_COMPONENTSW);
  if (lpUrlComponents->dwHostNameLength)
  {
    UCW.dwHostNameLength = lpUrlComponents->dwHostNameLength;
    if (lpUrlComponents->lpszHostName)
    {
      hostname = HeapAlloc(GetProcessHeap(), 0, UCW.dwHostNameLength * sizeof(WCHAR));
      UCW.lpszHostName = hostname;
    }
  }
  if (lpUrlComponents->dwUserNameLength)
  {
    UCW.dwUserNameLength = lpUrlComponents->dwUserNameLength;
    if (lpUrlComponents->lpszUserName)
    {
      username = HeapAlloc(GetProcessHeap(), 0, UCW.dwUserNameLength * sizeof(WCHAR));
      UCW.lpszUserName = username;
    }
  }
  if (lpUrlComponents->dwPasswordLength)
  {
    UCW.dwPasswordLength = lpUrlComponents->dwPasswordLength;
    if (lpUrlComponents->lpszPassword)
    {
      password = HeapAlloc(GetProcessHeap(), 0, UCW.dwPasswordLength * sizeof(WCHAR));
      UCW.lpszPassword = password;
    }
  }
  if (lpUrlComponents->dwUrlPathLength)
  {
    UCW.dwUrlPathLength = lpUrlComponents->dwUrlPathLength;
    if (lpUrlComponents->lpszUrlPath)
    {
      path = HeapAlloc(GetProcessHeap(), 0, UCW.dwUrlPathLength * sizeof(WCHAR));
      UCW.lpszUrlPath = path;
    }
  }
  if (lpUrlComponents->dwSchemeLength)
  {
    UCW.dwSchemeLength = lpUrlComponents->dwSchemeLength;
    if (lpUrlComponents->lpszScheme)
    {
      scheme = HeapAlloc(GetProcessHeap(), 0, UCW.dwSchemeLength * sizeof(WCHAR));
      UCW.lpszScheme = scheme;
    }
  }
  if (lpUrlComponents->dwExtraInfoLength)
  {
    UCW.dwExtraInfoLength = lpUrlComponents->dwExtraInfoLength;
    if (lpUrlComponents->lpszExtraInfo)
    {
      extra = HeapAlloc(GetProcessHeap(), 0, UCW.dwExtraInfoLength * sizeof(WCHAR));
      UCW.lpszExtraInfo = extra;
    }
  }
  if ((ret = InternetCrackUrlW(lpwszUrl, nLength, dwFlags, &UCW)))
  {
    ConvertUrlComponentValue(&lpUrlComponents->lpszHostName, &lpUrlComponents->dwHostNameLength,
                             UCW.lpszHostName, UCW.dwHostNameLength, lpszUrl, lpwszUrl);
    ConvertUrlComponentValue(&lpUrlComponents->lpszUserName, &lpUrlComponents->dwUserNameLength,
                             UCW.lpszUserName, UCW.dwUserNameLength, lpszUrl, lpwszUrl);
    ConvertUrlComponentValue(&lpUrlComponents->lpszPassword, &lpUrlComponents->dwPasswordLength,
                             UCW.lpszPassword, UCW.dwPasswordLength, lpszUrl, lpwszUrl);
    ConvertUrlComponentValue(&lpUrlComponents->lpszUrlPath, &lpUrlComponents->dwUrlPathLength,
                             UCW.lpszUrlPath, UCW.dwUrlPathLength, lpszUrl, lpwszUrl);
    ConvertUrlComponentValue(&lpUrlComponents->lpszScheme, &lpUrlComponents->dwSchemeLength,
                             UCW.lpszScheme, UCW.dwSchemeLength, lpszUrl, lpwszUrl);
    ConvertUrlComponentValue(&lpUrlComponents->lpszExtraInfo, &lpUrlComponents->dwExtraInfoLength,
                             UCW.lpszExtraInfo, UCW.dwExtraInfoLength, lpszUrl, lpwszUrl);

    lpUrlComponents->nScheme = UCW.nScheme;
    lpUrlComponents->nPort = UCW.nPort;

    TRACE("%s: scheme(%s) host(%s) path(%s) extra(%s)\n", lpszUrl,
          debugstr_an(lpUrlComponents->lpszScheme, lpUrlComponents->dwSchemeLength),
          debugstr_an(lpUrlComponents->lpszHostName, lpUrlComponents->dwHostNameLength),
          debugstr_an(lpUrlComponents->lpszUrlPath, lpUrlComponents->dwUrlPathLength),
          debugstr_an(lpUrlComponents->lpszExtraInfo, lpUrlComponents->dwExtraInfoLength));
  }
  HeapFree(GetProcessHeap(), 0, lpwszUrl);
  HeapFree(GetProcessHeap(), 0, hostname);
  HeapFree(GetProcessHeap(), 0, username);
  HeapFree(GetProcessHeap(), 0, password);
  HeapFree(GetProcessHeap(), 0, path);
  HeapFree(GetProcessHeap(), 0, scheme);
  HeapFree(GetProcessHeap(), 0, extra);
  return ret;
}

static const WCHAR url_schemes[][7] =
{
    {'f','t','p',0},
    {'g','o','p','h','e','r',0},
    {'h','t','t','p',0},
    {'h','t','t','p','s',0},
    {'f','i','l','e',0},
    {'n','e','w','s',0},
    {'m','a','i','l','t','o',0},
    {'r','e','s',0},
};

/***********************************************************************
 *           GetInternetSchemeW (internal)
 *
 * Get scheme of url
 *
 * RETURNS
 *    scheme on success
 *    INTERNET_SCHEME_UNKNOWN on failure
 *
 */
static INTERNET_SCHEME GetInternetSchemeW(LPCWSTR lpszScheme, DWORD nMaxCmp)
{
    int i;

    TRACE("%s %d\n",debugstr_wn(lpszScheme, nMaxCmp), nMaxCmp);

    if(lpszScheme==NULL)
        return INTERNET_SCHEME_UNKNOWN;

    for (i = 0; i < sizeof(url_schemes)/sizeof(url_schemes[0]); i++)
        if (!strncmpW(lpszScheme, url_schemes[i], nMaxCmp))
            return INTERNET_SCHEME_FIRST + i;

    return INTERNET_SCHEME_UNKNOWN;
}

/***********************************************************************
 *           SetUrlComponentValueW (Internal)
 *
 * Helper function for InternetCrackUrlW
 *
 * PARAMS
 *     lppszComponent [O] Holds the returned string
 *     dwComponentLen [I] Holds the size of lppszComponent
 *                    [O] Holds the length of the string in lppszComponent without '\0'
 *     lpszStart      [I] Holds the string to copy from
 *     len            [I] Holds the length of lpszStart without '\0'
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
static BOOL SetUrlComponentValueW(LPWSTR* lppszComponent, LPDWORD dwComponentLen, LPCWSTR lpszStart, DWORD len)
{
    TRACE("%s (%d)\n", debugstr_wn(lpszStart,len), len);

    if ( (*dwComponentLen == 0) && (*lppszComponent == NULL) )
        return FALSE;

    if (*dwComponentLen != 0 || *lppszComponent == NULL)
    {
        if (*lppszComponent == NULL)
        {
            *lppszComponent = (LPWSTR)lpszStart;
            *dwComponentLen = len;
        }
        else
        {
            DWORD ncpylen = min((*dwComponentLen)-1, len);
            memcpy(*lppszComponent, lpszStart, ncpylen*sizeof(WCHAR));
            (*lppszComponent)[ncpylen] = '\0';
            *dwComponentLen = ncpylen;
        }
    }

    return TRUE;
}

/***********************************************************************
 *           InternetCrackUrlW   (WININET.@)
 *
 * Break up URL into its components
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 */
BOOL WINAPI InternetCrackUrlW(LPCWSTR lpszUrl_orig, DWORD dwUrlLength_orig, DWORD dwFlags,
                              LPURL_COMPONENTSW lpUC)
{
  /*
   * RFC 1808
   * <protocol>:[//<net_loc>][/path][;<params>][?<query>][#<fragment>]
   *
   */
    LPCWSTR lpszParam    = NULL;
    BOOL  bIsAbsolute = FALSE;
    LPCWSTR lpszap, lpszUrl = lpszUrl_orig;
    LPCWSTR lpszcp = NULL;
    LPWSTR  lpszUrl_decode = NULL;
    DWORD dwUrlLength = dwUrlLength_orig;

    TRACE("(%s %u %x %p)\n",
          lpszUrl ? debugstr_wn(lpszUrl, dwUrlLength ? dwUrlLength : strlenW(lpszUrl)) : "(null)",
          dwUrlLength, dwFlags, lpUC);

    if (!lpszUrl_orig || !*lpszUrl_orig || !lpUC)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    if (!dwUrlLength) dwUrlLength = strlenW(lpszUrl);

    if (dwFlags & ICU_DECODE)
    {
        WCHAR *url_tmp;
        DWORD len = dwUrlLength + 1;

        if (!(url_tmp = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
        {
            INTERNET_SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        memcpy(url_tmp, lpszUrl_orig, dwUrlLength * sizeof(WCHAR));
        url_tmp[dwUrlLength] = 0;
        if (!(lpszUrl_decode = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
        {
            HeapFree(GetProcessHeap(), 0, url_tmp);
            INTERNET_SetLastError(ERROR_OUTOFMEMORY);
            return FALSE;
        }
        if (InternetCanonicalizeUrlW(url_tmp, lpszUrl_decode, &len, ICU_DECODE | ICU_NO_ENCODE))
        {
            dwUrlLength = len;
            lpszUrl = lpszUrl_decode;
        }
        HeapFree(GetProcessHeap(), 0, url_tmp);
    }
    lpszap = lpszUrl;
    
    /* Determine if the URI is absolute. */
    while (lpszap - lpszUrl < dwUrlLength)
    {
        if (isalnumW(*lpszap))
        {
            lpszap++;
            continue;
        }
        if ((*lpszap == ':') && (lpszap - lpszUrl >= 2))
        {
            bIsAbsolute = TRUE;
            lpszcp = lpszap;
        }
        else
        {
            lpszcp = lpszUrl; /* Relative url */
        }

        break;
    }

    lpUC->nScheme = INTERNET_SCHEME_UNKNOWN;
    lpUC->nPort = INTERNET_INVALID_PORT_NUMBER;

    /* Parse <params> */
    if (!(lpszParam = memchrW(lpszap, ';', dwUrlLength - (lpszap - lpszUrl))))
        lpszParam = memchrW(lpszap, '?', dwUrlLength - (lpszap - lpszUrl));

    SetUrlComponentValueW(&lpUC->lpszExtraInfo, &lpUC->dwExtraInfoLength,
                          lpszParam, lpszParam ? dwUrlLength-(lpszParam-lpszUrl) : 0);

    if (bIsAbsolute) /* Parse <protocol>:[//<net_loc>] */
    {
        LPCWSTR lpszNetLoc;

        /* Get scheme first. */
        lpUC->nScheme = GetInternetSchemeW(lpszUrl, lpszcp - lpszUrl);
        SetUrlComponentValueW(&lpUC->lpszScheme, &lpUC->dwSchemeLength,
                                   lpszUrl, lpszcp - lpszUrl);

        /* Eat ':' in protocol. */
        lpszcp++;

        /* double slash indicates the net_loc portion is present */
        if ((lpszcp[0] == '/') && (lpszcp[1] == '/'))
        {
            lpszcp += 2;

            lpszNetLoc = memchrW(lpszcp, '/', dwUrlLength - (lpszcp - lpszUrl));
            if (lpszParam)
            {
                if (lpszNetLoc)
                    lpszNetLoc = min(lpszNetLoc, lpszParam);
                else
                    lpszNetLoc = lpszParam;
            }
            else if (!lpszNetLoc)
                lpszNetLoc = lpszcp + dwUrlLength-(lpszcp-lpszUrl);

            /* Parse net-loc */
            if (lpszNetLoc)
            {
                LPCWSTR lpszHost;
                LPCWSTR lpszPort;

                /* [<user>[<:password>]@]<host>[:<port>] */
                /* First find the user and password if they exist */

                lpszHost = memchrW(lpszcp, '@', dwUrlLength - (lpszcp - lpszUrl));
                if (lpszHost == NULL || lpszHost > lpszNetLoc)
                {
                    /* username and password not specified. */
                    SetUrlComponentValueW(&lpUC->lpszUserName, &lpUC->dwUserNameLength, NULL, 0);
                    SetUrlComponentValueW(&lpUC->lpszPassword, &lpUC->dwPasswordLength, NULL, 0);
                }
                else /* Parse out username and password */
                {
                    LPCWSTR lpszUser = lpszcp;
                    LPCWSTR lpszPasswd = lpszHost;

                    while (lpszcp < lpszHost)
                    {
                        if (*lpszcp == ':')
                            lpszPasswd = lpszcp;

                        lpszcp++;
                    }

                    SetUrlComponentValueW(&lpUC->lpszUserName, &lpUC->dwUserNameLength,
                                          lpszUser, lpszPasswd - lpszUser);

                    if (lpszPasswd != lpszHost)
                        lpszPasswd++;
                    SetUrlComponentValueW(&lpUC->lpszPassword, &lpUC->dwPasswordLength,
                                          lpszPasswd == lpszHost ? NULL : lpszPasswd,
                                          lpszHost - lpszPasswd);

                    lpszcp++; /* Advance to beginning of host */
                }

                /* Parse <host><:port> */

                lpszHost = lpszcp;
                lpszPort = lpszNetLoc;

                /* special case for res:// URLs: there is no port here, so the host is the
                   entire string up to the first '/' */
                if(lpUC->nScheme==INTERNET_SCHEME_RES)
                {
                    SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength,
                                          lpszHost, lpszPort - lpszHost);
                    lpszcp=lpszNetLoc;
                }
                else
                {
                    while (lpszcp < lpszNetLoc)
                    {
                        if (*lpszcp == ':')
                            lpszPort = lpszcp;

                        lpszcp++;
                    }

                    /* If the scheme is "file" and the host is just one letter, it's not a host */
                    if(lpUC->nScheme==INTERNET_SCHEME_FILE && (lpszPort-lpszHost)==1)
                    {
                        lpszcp=lpszHost;
                        SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength,
                                              NULL, 0);
                    }
                    else
                    {
                        SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength,
                                              lpszHost, lpszPort - lpszHost);
                        if (lpszPort != lpszNetLoc)
                            lpUC->nPort = atoiW(++lpszPort);
                        else switch (lpUC->nScheme)
                        {
                        case INTERNET_SCHEME_HTTP:
                            lpUC->nPort = INTERNET_DEFAULT_HTTP_PORT;
                            break;
                        case INTERNET_SCHEME_HTTPS:
                            lpUC->nPort = INTERNET_DEFAULT_HTTPS_PORT;
                            break;
                        case INTERNET_SCHEME_FTP:
                            lpUC->nPort = INTERNET_DEFAULT_FTP_PORT;
                            break;
                        case INTERNET_SCHEME_GOPHER:
                            lpUC->nPort = INTERNET_DEFAULT_GOPHER_PORT;
                            break;
                        default:
                            break;
                        }
                    }
                }
            }
        }
        else
        {
            SetUrlComponentValueW(&lpUC->lpszUserName, &lpUC->dwUserNameLength, NULL, 0);
            SetUrlComponentValueW(&lpUC->lpszPassword, &lpUC->dwPasswordLength, NULL, 0);
            SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength, NULL, 0);
        }
    }
    else
    {
        SetUrlComponentValueW(&lpUC->lpszScheme, &lpUC->dwSchemeLength, NULL, 0);
        SetUrlComponentValueW(&lpUC->lpszUserName, &lpUC->dwUserNameLength, NULL, 0);
        SetUrlComponentValueW(&lpUC->lpszPassword, &lpUC->dwPasswordLength, NULL, 0);
        SetUrlComponentValueW(&lpUC->lpszHostName, &lpUC->dwHostNameLength, NULL, 0);
    }

    /* Here lpszcp points to:
     *
     * <protocol>:[//<net_loc>][/path][;<params>][?<query>][#<fragment>]
     *                          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
     */
    if (lpszcp != 0 && lpszcp - lpszUrl < dwUrlLength && (!lpszParam || lpszcp < lpszParam))
    {
        INT len;

        /* Only truncate the parameter list if it's already been saved
         * in lpUC->lpszExtraInfo.
         */
        if (lpszParam && lpUC->dwExtraInfoLength && lpUC->lpszExtraInfo)
            len = lpszParam - lpszcp;
        else
        {
            /* Leave the parameter list in lpszUrlPath.  Strip off any trailing
             * newlines if necessary.
             */
            LPWSTR lpsznewline = memchrW(lpszcp, '\n', dwUrlLength - (lpszcp - lpszUrl));
            if (lpsznewline != NULL)
                len = lpsznewline - lpszcp;
            else
                len = dwUrlLength-(lpszcp-lpszUrl);
        }
        SetUrlComponentValueW(&lpUC->lpszUrlPath, &lpUC->dwUrlPathLength,
                                   lpszcp, len);
    }
    else
    {
        if (lpUC->lpszUrlPath && (lpUC->dwUrlPathLength > 0))
            lpUC->lpszUrlPath[0] = 0;
        lpUC->dwUrlPathLength = 0;
    }

    TRACE("%s: scheme(%s) host(%s) path(%s) extra(%s)\n", debugstr_wn(lpszUrl,dwUrlLength),
             debugstr_wn(lpUC->lpszScheme,lpUC->dwSchemeLength),
             debugstr_wn(lpUC->lpszHostName,lpUC->dwHostNameLength),
             debugstr_wn(lpUC->lpszUrlPath,lpUC->dwUrlPathLength),
             debugstr_wn(lpUC->lpszExtraInfo,lpUC->dwExtraInfoLength));

    HeapFree(GetProcessHeap(), 0, lpszUrl_decode );
    return TRUE;
}

/***********************************************************************
 *           InternetAttemptConnect (WININET.@)
 *
 * Attempt to make a connection to the internet
 *
 * RETURNS
 *    ERROR_SUCCESS on success
 *    Error value   on failure
 *
 */
DWORD WINAPI InternetAttemptConnect(DWORD dwReserved)
{
    FIXME("Stub\n");
    return ERROR_SUCCESS;
}


/***********************************************************************
 *           InternetCanonicalizeUrlA (WININET.@)
 *
 * Escape unsafe characters and spaces
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCanonicalizeUrlA(LPCSTR lpszUrl, LPSTR lpszBuffer,
	LPDWORD lpdwBufferLength, DWORD dwFlags)
{
    HRESULT hr;
    DWORD dwURLFlags = URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE;

    TRACE("(%s, %p, %p, 0x%08x) bufferlength: %d\n", debugstr_a(lpszUrl), lpszBuffer,
        lpdwBufferLength, lpdwBufferLength ? *lpdwBufferLength : -1, dwFlags);

    if(dwFlags & ICU_DECODE)
    {
        dwURLFlags |= URL_UNESCAPE;
        dwFlags &= ~ICU_DECODE;
    }

    if(dwFlags & ICU_ESCAPE)
    {
        dwURLFlags |= URL_UNESCAPE;
        dwFlags &= ~ICU_ESCAPE;
    }

    if(dwFlags & ICU_BROWSER_MODE)
    {
        dwURLFlags |= URL_BROWSER_MODE;
        dwFlags &= ~ICU_BROWSER_MODE;
    }

    if(dwFlags & ICU_NO_ENCODE)
    {
        /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
        dwURLFlags ^= URL_ESCAPE_UNSAFE;
        dwFlags &= ~ICU_NO_ENCODE;
    }

    if (dwFlags) FIXME("Unhandled flags 0x%08x\n", dwFlags);

    hr = UrlCanonicalizeA(lpszUrl, lpszBuffer, lpdwBufferLength, dwURLFlags);
    if (hr == E_POINTER) SetLastError(ERROR_INSUFFICIENT_BUFFER);
    if (hr == E_INVALIDARG) SetLastError(ERROR_INVALID_PARAMETER);

    return (hr == S_OK) ? TRUE : FALSE;
}

/***********************************************************************
 *           InternetCanonicalizeUrlW (WININET.@)
 *
 * Escape unsafe characters and spaces
 *
 * RETURNS
 *    TRUE on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetCanonicalizeUrlW(LPCWSTR lpszUrl, LPWSTR lpszBuffer,
    LPDWORD lpdwBufferLength, DWORD dwFlags)
{
    HRESULT hr;
    DWORD dwURLFlags = URL_WININET_COMPATIBILITY | URL_ESCAPE_UNSAFE;

    TRACE("(%s, %p, %p, 0x%08x) bufferlength: %d\n", debugstr_w(lpszUrl), lpszBuffer,
          lpdwBufferLength, dwFlags, lpdwBufferLength ? *lpdwBufferLength : -1);

    if(dwFlags & ICU_DECODE)
    {
        dwURLFlags |= URL_UNESCAPE;
        dwFlags &= ~ICU_DECODE;
    }

    if(dwFlags & ICU_ESCAPE)
    {
        dwURLFlags |= URL_UNESCAPE;
        dwFlags &= ~ICU_ESCAPE;
    }

    if(dwFlags & ICU_BROWSER_MODE)
    {
        dwURLFlags |= URL_BROWSER_MODE;
        dwFlags &= ~ICU_BROWSER_MODE;
    }

    if(dwFlags & ICU_NO_ENCODE)
    {
        /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
        dwURLFlags ^= URL_ESCAPE_UNSAFE;
        dwFlags &= ~ICU_NO_ENCODE;
    }

    if (dwFlags) FIXME("Unhandled flags 0x%08x\n", dwFlags);

    hr = UrlCanonicalizeW(lpszUrl, lpszBuffer, lpdwBufferLength, dwURLFlags);
    if (hr == E_POINTER) SetLastError(ERROR_INSUFFICIENT_BUFFER);
    if (hr == E_INVALIDARG) SetLastError(ERROR_INVALID_PARAMETER);

    return (hr == S_OK) ? TRUE : FALSE;
}

/* #################################################### */

static INTERNET_STATUS_CALLBACK set_status_callback(
    LPWININETHANDLEHEADER lpwh, INTERNET_STATUS_CALLBACK callback, BOOL unicode)
{
    INTERNET_STATUS_CALLBACK ret;

    if (unicode) lpwh->dwInternalFlags |= INET_CALLBACKW;
    else lpwh->dwInternalFlags &= ~INET_CALLBACKW;

    ret = lpwh->lpfnStatusCB;
    lpwh->lpfnStatusCB = callback;

    return ret;
}

/***********************************************************************
 *           InternetSetStatusCallbackA (WININET.@)
 *
 * Sets up a callback function which is called as progress is made
 * during an operation.
 *
 * RETURNS
 *    Previous callback or NULL 	on success
 *    INTERNET_INVALID_STATUS_CALLBACK  on failure
 *
 */
INTERNET_STATUS_CALLBACK WINAPI InternetSetStatusCallbackA(
	HINTERNET hInternet ,INTERNET_STATUS_CALLBACK lpfnIntCB)
{
    INTERNET_STATUS_CALLBACK retVal;
    LPWININETHANDLEHEADER lpwh;

    TRACE("%p\n", hInternet);

    if (!(lpwh = WININET_GetObject(hInternet)))
        return INTERNET_INVALID_STATUS_CALLBACK;

    retVal = set_status_callback(lpwh, lpfnIntCB, FALSE);

    WININET_Release( lpwh );
    return retVal;
}

/***********************************************************************
 *           InternetSetStatusCallbackW (WININET.@)
 *
 * Sets up a callback function which is called as progress is made
 * during an operation.
 *
 * RETURNS
 *    Previous callback or NULL 	on success
 *    INTERNET_INVALID_STATUS_CALLBACK  on failure
 *
 */
INTERNET_STATUS_CALLBACK WINAPI InternetSetStatusCallbackW(
	HINTERNET hInternet ,INTERNET_STATUS_CALLBACK lpfnIntCB)
{
    INTERNET_STATUS_CALLBACK retVal;
    LPWININETHANDLEHEADER lpwh;

    TRACE("%p\n", hInternet);

    if (!(lpwh = WININET_GetObject(hInternet)))
        return INTERNET_INVALID_STATUS_CALLBACK;

    retVal = set_status_callback(lpwh, lpfnIntCB, TRUE);

    WININET_Release( lpwh );
    return retVal;
}

/***********************************************************************
 *           InternetSetFilePointer (WININET.@)
 */
DWORD WINAPI InternetSetFilePointer(HINTERNET hFile, LONG lDistanceToMove,
    PVOID pReserved, DWORD dwMoveContext, DWORD_PTR dwContext)
{
    FIXME("stub\n");
    return FALSE;
}

/***********************************************************************
 *           InternetWriteFile (WININET.@)
 *
 * Write data to an open internet file
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetWriteFile(HINTERNET hFile, LPCVOID lpBuffer,
	DWORD dwNumOfBytesToWrite, LPDWORD lpdwNumOfBytesWritten)
{
    LPWININETHANDLEHEADER lpwh;
    BOOL retval = FALSE;

    TRACE("(%p %p %d %p)\n", hFile, lpBuffer, dwNumOfBytesToWrite, lpdwNumOfBytesWritten);

    lpwh = WININET_GetObject( hFile );
    if (!lpwh) {
        WARN("Invalid handle\n");
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(lpwh->vtbl->WriteFile) {
        retval = lpwh->vtbl->WriteFile(lpwh, lpBuffer, dwNumOfBytesToWrite, lpdwNumOfBytesWritten);
    }else {
        WARN("No Writefile method.\n");
        SetLastError(ERROR_INVALID_HANDLE);
        retval = FALSE;
    }

    WININET_Release( lpwh );

    return retval;
}


/***********************************************************************
 *           InternetReadFile (WININET.@)
 *
 * Read data from an open internet file
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetReadFile(HINTERNET hFile, LPVOID lpBuffer,
        DWORD dwNumOfBytesToRead, LPDWORD pdwNumOfBytesRead)
{
    LPWININETHANDLEHEADER hdr;
    DWORD res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;

    TRACE("%p %p %d %p\n", hFile, lpBuffer, dwNumOfBytesToRead, pdwNumOfBytesRead);

    hdr = WININET_GetObject(hFile);
    if (!hdr) {
        INTERNET_SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(hdr->vtbl->ReadFile)
        res = hdr->vtbl->ReadFile(hdr, lpBuffer, dwNumOfBytesToRead, pdwNumOfBytesRead);

    WININET_Release(hdr);

    TRACE("-- %s (%u) (bytes read: %d)\n", res == ERROR_SUCCESS ? "TRUE": "FALSE", res,
          pdwNumOfBytesRead ? *pdwNumOfBytesRead : -1);

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           InternetReadFileExA (WININET.@)
 *
 * Read data from an open internet file
 *
 * PARAMS
 *  hFile         [I] Handle returned by InternetOpenUrl or HttpOpenRequest.
 *  lpBuffersOut  [I/O] Buffer.
 *  dwFlags       [I] Flags. See notes.
 *  dwContext     [I] Context for callbacks.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 * NOTES
 *  The parameter dwFlags include zero or more of the following flags:
 *|IRF_ASYNC - Makes the call asynchronous.
 *|IRF_SYNC - Makes the call synchronous.
 *|IRF_USE_CONTEXT - Forces dwContext to be used.
 *|IRF_NO_WAIT - Don't block if the data is not available, just return what is available.
 *
 * However, in testing IRF_USE_CONTEXT seems to have no effect - dwContext isn't used.
 *
 * SEE
 *  InternetOpenUrlA(), HttpOpenRequestA()
 */
BOOL WINAPI InternetReadFileExA(HINTERNET hFile, LPINTERNET_BUFFERSA lpBuffersOut,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    LPWININETHANDLEHEADER hdr;
    DWORD res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;

    TRACE("(%p %p 0x%x 0x%lx)\n", hFile, lpBuffersOut, dwFlags, dwContext);

    hdr = WININET_GetObject(hFile);
    if (!hdr) {
        INTERNET_SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(hdr->vtbl->ReadFileExA)
        res = hdr->vtbl->ReadFileExA(hdr, lpBuffersOut, dwFlags, dwContext);

    WININET_Release(hdr);

    TRACE("-- %s (%u, bytes read: %d)\n", res == ERROR_SUCCESS ? "TRUE": "FALSE",
          res, lpBuffersOut->dwBufferLength);

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           InternetReadFileExW (WININET.@)
 * SEE
 *  InternetReadFileExA()
 */
BOOL WINAPI InternetReadFileExW(HINTERNET hFile, LPINTERNET_BUFFERSW lpBuffer,
	DWORD dwFlags, DWORD_PTR dwContext)
{
    LPWININETHANDLEHEADER hdr;
    DWORD res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;

    TRACE("(%p %p 0x%x 0x%lx)\n", hFile, lpBuffer, dwFlags, dwContext);

    hdr = WININET_GetObject(hFile);
    if (!hdr) {
        INTERNET_SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(hdr->vtbl->ReadFileExW)
        res = hdr->vtbl->ReadFileExW(hdr, lpBuffer, dwFlags, dwContext);

    WININET_Release(hdr);

    TRACE("-- %s (%u, bytes read: %d)\n", res == ERROR_SUCCESS ? "TRUE": "FALSE",
          res, lpBuffer->dwBufferLength);

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

DWORD INET_QueryOption(DWORD option, void *buffer, DWORD *size, BOOL unicode)
{
    static BOOL warn = TRUE;

    switch(option) {
    case INTERNET_OPTION_REQUEST_FLAGS:
        TRACE("INTERNET_OPTION_REQUEST_FLAGS\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *(ULONG*)buffer = 4;
        *size = sizeof(ULONG);

        return ERROR_SUCCESS;

    case INTERNET_OPTION_HTTP_VERSION:
        if (*size < sizeof(HTTP_VERSION_INFO))
            return ERROR_INSUFFICIENT_BUFFER;

        /*
         * Presently hardcoded to 1.1
         */
        ((HTTP_VERSION_INFO*)buffer)->dwMajorVersion = 1;
        ((HTTP_VERSION_INFO*)buffer)->dwMinorVersion = 1;
        *size = sizeof(HTTP_VERSION_INFO);

        return ERROR_SUCCESS;

    case INTERNET_OPTION_CONNECTED_STATE:

        if (warn) {
            FIXME("INTERNET_OPTION_CONNECTED_STATE: semi-stub\n");
            warn = FALSE;
        }
        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *(ULONG*)buffer = INTERNET_STATE_CONNECTED;
        *size = sizeof(ULONG);

        return ERROR_SUCCESS;

    case INTERNET_OPTION_PROXY: {
        WININETAPPINFOW ai;

        TRACE("Getting global proxy info\n");
        memset(&ai, 0, sizeof(WININETAPPINFOW));
        INTERNET_ConfigureProxy(&ai);

        return APPINFO_QueryOption(&ai.hdr, INTERNET_OPTION_PROXY, buffer, size, unicode); /* FIXME */
    }

    case INTERNET_OPTION_MAX_CONNS_PER_SERVER:
        TRACE("INTERNET_OPTION_MAX_CONNS_PER_SERVER\n");

        if (*size < sizeof(ULONG))
            return ERROR_INSUFFICIENT_BUFFER;

        *(ULONG*)buffer = 2;
        *size = sizeof(ULONG);

        return ERROR_SUCCESS;

    case INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER:
            TRACE("INTERNET_OPTION_MAX_CONNS_1_0_SERVER\n");

            if (*size < sizeof(ULONG))
                return ERROR_INSUFFICIENT_BUFFER;

            *(ULONG*)size = 4;
            *size = sizeof(ULONG);

            return ERROR_SUCCESS;

    case INTERNET_OPTION_SECURITY_FLAGS:
        FIXME("INTERNET_OPTION_SECURITY_FLAGS: Stub\n");
        return ERROR_SUCCESS;

    case INTERNET_OPTION_VERSION: {
        static const INTERNET_VERSION_INFO info = { 1, 2 };

        TRACE("INTERNET_OPTION_VERSION\n");

        if (*size < sizeof(INTERNET_VERSION_INFO))
            return ERROR_INSUFFICIENT_BUFFER;

        memcpy(buffer, &info, sizeof(info));
        *size = sizeof(info);

        return ERROR_SUCCESS;
    }

    case INTERNET_OPTION_PER_CONNECTION_OPTION: {
        INTERNET_PER_CONN_OPTION_LISTW *con = buffer;
        DWORD res = ERROR_SUCCESS, i;

        FIXME("INTERNET_OPTION_PER_CONNECTION_OPTION stub\n");

        if (*size < sizeof(INTERNET_PER_CONN_OPTION_LISTW))
            return ERROR_INSUFFICIENT_BUFFER;

        for (i = 0; i < con->dwOptionCount; i++) {
            INTERNET_PER_CONN_OPTIONW *option = con->pOptions + i;

            switch (option->dwOption) {
            case INTERNET_PER_CONN_FLAGS:
                option->Value.dwValue = PROXY_TYPE_DIRECT;
                break;

            case INTERNET_PER_CONN_PROXY_SERVER:
            case INTERNET_PER_CONN_PROXY_BYPASS:
            case INTERNET_PER_CONN_AUTOCONFIG_URL:
            case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
            case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
            case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
            case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME:
            case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
                FIXME("Unhandled dwOption %d\n", option->dwOption);
                memset(&option->Value, 0, sizeof(option->Value));
                break;

            default:
                FIXME("Unknown dwOption %d\n", option->dwOption);
                res = ERROR_INVALID_PARAMETER;
                break;
            }
        }

        return res;
    }
    }

    FIXME("Stub for %d\n", option);
    return ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
}

/***********************************************************************
 *           InternetQueryOptionW (WININET.@)
 *
 * Queries an options on the specified handle
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetQueryOptionW(HINTERNET hInternet, DWORD dwOption,
                                 LPVOID lpBuffer, LPDWORD lpdwBufferLength)
{
    LPWININETHANDLEHEADER hdr;
    DWORD res = ERROR_INVALID_HANDLE;

    TRACE("%p %d %p %p\n", hInternet, dwOption, lpBuffer, lpdwBufferLength);

    if(hInternet) {
        hdr = WININET_GetObject(hInternet);
        if (hdr) {
            res = hdr->vtbl->QueryOption(hdr, dwOption, lpBuffer, lpdwBufferLength, TRUE);
            WININET_Release(hdr);
        }
    }else {
        res = INET_QueryOption(dwOption, lpBuffer, lpdwBufferLength, TRUE);
    }

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}

/***********************************************************************
 *           InternetQueryOptionA (WININET.@)
 *
 * Queries an options on the specified handle
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetQueryOptionA(HINTERNET hInternet, DWORD dwOption,
                                 LPVOID lpBuffer, LPDWORD lpdwBufferLength)
{
    LPWININETHANDLEHEADER hdr;
    DWORD res = ERROR_INVALID_HANDLE;

    TRACE("%p %d %p %p\n", hInternet, dwOption, lpBuffer, lpdwBufferLength);

    if(hInternet) {
        hdr = WININET_GetObject(hInternet);
        if (hdr) {
            res = hdr->vtbl->QueryOption(hdr, dwOption, lpBuffer, lpdwBufferLength, FALSE);
            WININET_Release(hdr);
        }
    }else {
        res = INET_QueryOption(dwOption, lpBuffer, lpdwBufferLength, FALSE);
    }

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}


/***********************************************************************
 *           InternetSetOptionW (WININET.@)
 *
 * Sets an options on the specified handle
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetOptionW(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength)
{
    LPWININETHANDLEHEADER lpwhh;
    BOOL ret = TRUE;

    TRACE("(%p %d %p %d)\n", hInternet, dwOption, lpBuffer, dwBufferLength);

    lpwhh = (LPWININETHANDLEHEADER) WININET_GetObject( hInternet );
    if(lpwhh && lpwhh->vtbl->SetOption) {
        DWORD res;

        res = lpwhh->vtbl->SetOption(lpwhh, dwOption, lpBuffer, dwBufferLength);
        if(res != ERROR_INTERNET_INVALID_OPTION) {
            WININET_Release( lpwhh );

            if(res != ERROR_SUCCESS)
                SetLastError(res);

            return res == ERROR_SUCCESS;
        }
    }

    switch (dwOption)
    {
    case INTERNET_OPTION_CALLBACK:
      {
        if (!lpwhh)
        {
            INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
            return FALSE;
        }
        WININET_Release(lpwhh);
        INTERNET_SetLastError(ERROR_INTERNET_OPTION_NOT_SETTABLE);
        return FALSE;
      }
    case INTERNET_OPTION_HTTP_VERSION:
      {
        HTTP_VERSION_INFO* pVersion=(HTTP_VERSION_INFO*)lpBuffer;
        FIXME("Option INTERNET_OPTION_HTTP_VERSION(%d,%d): STUB\n",pVersion->dwMajorVersion,pVersion->dwMinorVersion);
      }
      break;
    case INTERNET_OPTION_ERROR_MASK:
      {
        ULONG flags = *(ULONG *)lpBuffer;
        FIXME("Option INTERNET_OPTION_ERROR_MASK(%d): STUB\n", flags);
      }
      break;
    case INTERNET_OPTION_CODEPAGE:
      {
        ULONG codepage = *(ULONG *)lpBuffer;
        FIXME("Option INTERNET_OPTION_CODEPAGE (%d): STUB\n", codepage);
      }
      break;
    case INTERNET_OPTION_REQUEST_PRIORITY:
      {
        ULONG priority = *(ULONG *)lpBuffer;
        FIXME("Option INTERNET_OPTION_REQUEST_PRIORITY (%d): STUB\n", priority);
      }
      break;
    case INTERNET_OPTION_CONNECT_TIMEOUT:
      {
        ULONG connecttimeout = *(ULONG *)lpBuffer;
        FIXME("Option INTERNET_OPTION_CONNECT_TIMEOUT (%d): STUB\n", connecttimeout);
      }
      break;
    case INTERNET_OPTION_DATA_RECEIVE_TIMEOUT:
      {
        ULONG receivetimeout = *(ULONG *)lpBuffer;
        FIXME("Option INTERNET_OPTION_DATA_RECEIVE_TIMEOUT (%d): STUB\n", receivetimeout);
      }
      break;
    case INTERNET_OPTION_MAX_CONNS_PER_SERVER:
      {
        ULONG conns = *(ULONG *)lpBuffer;
        FIXME("Option INTERNET_OPTION_MAX_CONNS_PER_SERVER (%d): STUB\n", conns);
      }
      break;
    case INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER:
      {
        ULONG conns = *(ULONG *)lpBuffer;
        FIXME("Option INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER (%d): STUB\n", conns);
      }
      break;
    case INTERNET_OPTION_RESET_URLCACHE_SESSION:
        FIXME("Option INTERNET_OPTION_RESET_URLCACHE_SESSION: STUB\n");
        break;
    case INTERNET_OPTION_END_BROWSER_SESSION:
        FIXME("Option INTERNET_OPTION_END_BROWSER_SESSION: STUB\n");
        break;
    case INTERNET_OPTION_CONNECTED_STATE:
        FIXME("Option INTERNET_OPTION_CONNECTED_STATE: STUB\n");
        break;
    case INTERNET_OPTION_DISABLE_PASSPORT_AUTH:
	TRACE("Option INTERNET_OPTION_DISABLE_PASSPORT_AUTH: harmless stub, since not enabled\n");
	break;
    case INTERNET_OPTION_SEND_TIMEOUT:
    case INTERNET_OPTION_RECEIVE_TIMEOUT:
    {
        ULONG timeout = *(ULONG *)lpBuffer;
        FIXME("INTERNET_OPTION_SEND/RECEIVE_TIMEOUT %d\n", timeout);
        break;
    }
    case INTERNET_OPTION_CONNECT_RETRIES:
    {
        ULONG retries = *(ULONG *)lpBuffer;
        FIXME("INTERNET_OPTION_CONNECT_RETRIES %d\n", retries);
        break;
    }
    case INTERNET_OPTION_CONTEXT_VALUE:
	 FIXME("Option INTERNET_OPTION_CONTEXT_VALUE; STUB\n");
	 break;
    case INTERNET_OPTION_SECURITY_FLAGS:
	 FIXME("Option INTERNET_OPTION_SECURITY_FLAGS; STUB\n");
	 break;
    case INTERNET_OPTION_DISABLE_AUTODIAL:
	 FIXME("Option INTERNET_OPTION_DISABLE_AUTODIAL; STUB\n");
	 break;
    case INTERNET_OPTION_HTTP_DECODING:
        FIXME("INTERNET_OPTION_HTTP_DECODING; STUB\n");
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_OPTION);
        ret = FALSE;
        break;
    case INTERNET_OPTION_COOKIES_3RD_PARTY:
        FIXME("INTERNET_OPTION_COOKIES_3RD_PARTY; STUB\n");
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_OPTION);
        ret = FALSE;
        break;
    case INTERNET_OPTION_SEND_UTF8_SERVERNAME_TO_PROXY:
        FIXME("INTERNET_OPTION_SEND_UTF8_SERVERNAME_TO_PROXY; STUB\n");
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_OPTION);
        ret = FALSE;
        break;
    case INTERNET_OPTION_CODEPAGE_PATH:
        FIXME("INTERNET_OPTION_CODEPAGE_PATH; STUB\n");
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_OPTION);
        ret = FALSE;
        break;
    case INTERNET_OPTION_CODEPAGE_EXTRA:
        FIXME("INTERNET_OPTION_CODEPAGE_EXTRA; STUB\n");
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_OPTION);
        ret = FALSE;
        break;
    case INTERNET_OPTION_IDN:
        FIXME("INTERNET_OPTION_IDN; STUB\n");
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_OPTION);
        ret = FALSE;
        break;
    default:
        FIXME("Option %d STUB\n",dwOption);
        INTERNET_SetLastError(ERROR_INTERNET_INVALID_OPTION);
        ret = FALSE;
        break;
    }

    if(lpwhh)
        WININET_Release( lpwhh );

    return ret;
}


/***********************************************************************
 *           InternetSetOptionA (WININET.@)
 *
 * Sets an options on the specified handle.
 *
 * RETURNS
 *    TRUE  on success
 *    FALSE on failure
 *
 */
BOOL WINAPI InternetSetOptionA(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength)
{
    LPVOID wbuffer;
    DWORD wlen;
    BOOL r;

    switch( dwOption )
    {
    case INTERNET_OPTION_CALLBACK:
        {
        LPWININETHANDLEHEADER lpwh;

        if (!(lpwh = WININET_GetObject(hInternet)))
        {
            INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
            return FALSE;
        }
        WININET_Release(lpwh);
        INTERNET_SetLastError(ERROR_INTERNET_OPTION_NOT_SETTABLE);
        return FALSE;
        }
    case INTERNET_OPTION_PROXY:
        {
        LPINTERNET_PROXY_INFOA pi = (LPINTERNET_PROXY_INFOA) lpBuffer;
        LPINTERNET_PROXY_INFOW piw;
        DWORD proxlen, prbylen;
        LPWSTR prox, prby;

        proxlen = MultiByteToWideChar( CP_ACP, 0, pi->lpszProxy, -1, NULL, 0);
        prbylen= MultiByteToWideChar( CP_ACP, 0, pi->lpszProxyBypass, -1, NULL, 0);
        wlen = sizeof(*piw) + proxlen + prbylen;
        wbuffer = HeapAlloc( GetProcessHeap(), 0, wlen*sizeof(WCHAR) );
        piw = (LPINTERNET_PROXY_INFOW) wbuffer;
        piw->dwAccessType = pi->dwAccessType;
        prox = (LPWSTR) &piw[1];
        prby = &prox[proxlen+1];
        MultiByteToWideChar( CP_ACP, 0, pi->lpszProxy, -1, prox, proxlen);
        MultiByteToWideChar( CP_ACP, 0, pi->lpszProxyBypass, -1, prby, prbylen);
        piw->lpszProxy = prox;
        piw->lpszProxyBypass = prby;
        }
        break;
    case INTERNET_OPTION_USER_AGENT:
    case INTERNET_OPTION_USERNAME:
    case INTERNET_OPTION_PASSWORD:
        wlen = MultiByteToWideChar( CP_ACP, 0, lpBuffer, dwBufferLength,
                                   NULL, 0 );
        wbuffer = HeapAlloc( GetProcessHeap(), 0, wlen*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, lpBuffer, dwBufferLength,
                                   wbuffer, wlen );
        break;
    default:
        wbuffer = lpBuffer;
        wlen = dwBufferLength;
    }

    r = InternetSetOptionW(hInternet,dwOption, wbuffer, wlen);

    if( lpBuffer != wbuffer )
        HeapFree( GetProcessHeap(), 0, wbuffer );

    return r;
}


/***********************************************************************
 *           InternetSetOptionExA (WININET.@)
 */
BOOL WINAPI InternetSetOptionExA(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwFlags)
{
    FIXME("Flags %08x ignored\n", dwFlags);
    return InternetSetOptionA( hInternet, dwOption, lpBuffer, dwBufferLength );
}

/***********************************************************************
 *           InternetSetOptionExW (WININET.@)
 */
BOOL WINAPI InternetSetOptionExW(HINTERNET hInternet, DWORD dwOption,
                           LPVOID lpBuffer, DWORD dwBufferLength, DWORD dwFlags)
{
    FIXME("Flags %08x ignored\n", dwFlags);
    if( dwFlags & ~ISO_VALID_FLAGS )
    {
        INTERNET_SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }
    return InternetSetOptionW( hInternet, dwOption, lpBuffer, dwBufferLength );
}

static const WCHAR WININET_wkday[7][4] =
    { { 'S','u','n', 0 }, { 'M','o','n', 0 }, { 'T','u','e', 0 }, { 'W','e','d', 0 },
      { 'T','h','u', 0 }, { 'F','r','i', 0 }, { 'S','a','t', 0 } };
static const WCHAR WININET_month[12][4] =
    { { 'J','a','n', 0 }, { 'F','e','b', 0 }, { 'M','a','r', 0 }, { 'A','p','r', 0 },
      { 'M','a','y', 0 }, { 'J','u','n', 0 }, { 'J','u','l', 0 }, { 'A','u','g', 0 },
      { 'S','e','p', 0 }, { 'O','c','t', 0 }, { 'N','o','v', 0 }, { 'D','e','c', 0 } };

/***********************************************************************
 *           InternetTimeFromSystemTimeA (WININET.@)
 */
BOOL WINAPI InternetTimeFromSystemTimeA( const SYSTEMTIME* time, DWORD format, LPSTR string, DWORD size )
{
    BOOL ret;
    WCHAR stringW[INTERNET_RFC1123_BUFSIZE];

    TRACE( "%p 0x%08x %p 0x%08x\n", time, format, string, size );

    if (!time || !string || format != INTERNET_RFC1123_FORMAT)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (size < INTERNET_RFC1123_BUFSIZE * sizeof(*string))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    ret = InternetTimeFromSystemTimeW( time, format, stringW, sizeof(stringW) );
    if (ret) WideCharToMultiByte( CP_ACP, 0, stringW, -1, string, size, NULL, NULL );

    return ret;
}

/***********************************************************************
 *           InternetTimeFromSystemTimeW (WININET.@)
 */
BOOL WINAPI InternetTimeFromSystemTimeW( const SYSTEMTIME* time, DWORD format, LPWSTR string, DWORD size )
{
    static const WCHAR date[] =
        { '%','s',',',' ','%','0','2','d',' ','%','s',' ','%','4','d',' ','%','0',
          '2','d',':','%','0','2','d',':','%','0','2','d',' ','G','M','T', 0 };

    TRACE( "%p 0x%08x %p 0x%08x\n", time, format, string, size );

    if (!time || !string || format != INTERNET_RFC1123_FORMAT)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (size < INTERNET_RFC1123_BUFSIZE * sizeof(*string))
    {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    sprintfW( string, date,
              WININET_wkday[time->wDayOfWeek],
              time->wDay,
              WININET_month[time->wMonth - 1],
              time->wYear,
              time->wHour,
              time->wMinute,
              time->wSecond );

    return TRUE;
}

/***********************************************************************
 *           InternetTimeToSystemTimeA (WININET.@)
 */
BOOL WINAPI InternetTimeToSystemTimeA( LPCSTR string, SYSTEMTIME* time, DWORD reserved )
{
    BOOL ret = FALSE;
    WCHAR *stringW;
    int len;

    TRACE( "%s %p 0x%08x\n", debugstr_a(string), time, reserved );

    len = MultiByteToWideChar( CP_ACP, 0, string, -1, NULL, 0 );
    stringW = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );

    if (stringW)
    {
        MultiByteToWideChar( CP_ACP, 0, string, -1, stringW, len );
        ret = InternetTimeToSystemTimeW( stringW, time, reserved );
        HeapFree( GetProcessHeap(), 0, stringW );
    }
    return ret;
}

/***********************************************************************
 *           InternetTimeToSystemTimeW (WININET.@)
 */
BOOL WINAPI InternetTimeToSystemTimeW( LPCWSTR string, SYSTEMTIME* time, DWORD reserved )
{
    unsigned int i;
    const WCHAR *s = string;
    WCHAR       *end;

    TRACE( "%s %p 0x%08x\n", debugstr_w(string), time, reserved );

    if (!string || !time) return FALSE;

    /* Windows does this too */
    GetSystemTime( time );

    /*  Convert an RFC1123 time such as 'Fri, 07 Jan 2005 12:06:35 GMT' into
     *  a SYSTEMTIME structure.
     */

    while (*s && !isalphaW( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wDayOfWeek = 7;

    for (i = 0; i < 7; i++)
    {
        if (toupperW( WININET_wkday[i][0] ) == toupperW( s[0] ) &&
            toupperW( WININET_wkday[i][1] ) == toupperW( s[1] ) &&
            toupperW( WININET_wkday[i][2] ) == toupperW( s[2] ) )
        {
            time->wDayOfWeek = i;
            break;
        }
    }

    if (time->wDayOfWeek > 6) return TRUE;
    while (*s && !isdigitW( *s )) s++;
    time->wDay = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isalphaW( *s )) s++;
    if (s[0] == '\0' || s[1] == '\0' || s[2] == '\0') return TRUE;
    time->wMonth = 0;

    for (i = 0; i < 12; i++)
    {
        if (toupperW( WININET_month[i][0]) == toupperW( s[0] ) &&
            toupperW( WININET_month[i][1]) == toupperW( s[1] ) &&
            toupperW( WININET_month[i][2]) == toupperW( s[2] ) )
        {
            time->wMonth = i + 1;
            break;
        }
    }
    if (time->wMonth == 0) return TRUE;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wYear = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wHour = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wMinute = strtolW( s, &end, 10 );
    s = end;

    while (*s && !isdigitW( *s )) s++;
    if (*s == '\0') return TRUE;
    time->wSecond = strtolW( s, &end, 10 );
    s = end;

    time->wMilliseconds = 0;
    return TRUE;
}

/***********************************************************************
 *	InternetCheckConnectionW (WININET.@)
 *
 * Pings a requested host to check internet connection
 *
 * RETURNS
 *   TRUE on success and FALSE on failure. If a failure then
 *   ERROR_NOT_CONNECTED is placed into GetLastError
 *
 */
BOOL WINAPI InternetCheckConnectionW( LPCWSTR lpszUrl, DWORD dwFlags, DWORD dwReserved )
{
/*
 * this is a kludge which runs the resident ping program and reads the output.
 *
 * Anyone have a better idea?
 */

  BOOL   rc = FALSE;
  static const CHAR ping[] = "ping -c 1 ";
  static const CHAR redirect[] = " >/dev/null 2>/dev/null";
  CHAR *command = NULL;
  WCHAR hostW[1024];
  DWORD len;
  INTERNET_PORT port;
  int status = -1;

  FIXME("\n");

  /*
   * Crack or set the Address
   */
  if (lpszUrl == NULL)
  {
     /*
      * According to the doc we are supposed to use the ip for the next
      * server in the WnInet internal server database. I have
      * no idea what that is or how to get it.
      *
      * So someone needs to implement this.
      */
     FIXME("Unimplemented with URL of NULL\n");
     return TRUE;
  }
  else
  {
     URL_COMPONENTSW components;

     ZeroMemory(&components,sizeof(URL_COMPONENTSW));
     components.lpszHostName = (LPWSTR)hostW;
     components.dwHostNameLength = 1024;

     if (!InternetCrackUrlW(lpszUrl,0,0,&components))
       goto End;

     TRACE("host name : %s\n",debugstr_w(components.lpszHostName));
     port = components.nPort;
     TRACE("port: %d\n", port);
  }

  if (dwFlags & FLAG_ICC_FORCE_CONNECTION)
  {
      struct sockaddr_in sin;
      int fd;

      if (!GetAddress(hostW, port, &sin))
          goto End;
      fd = socket(sin.sin_family, SOCK_STREAM, 0);
      if (fd != -1)
      {
          if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == 0)
              rc = TRUE;
          close(fd);
      }
  }
  else
  {
      /*
       * Build our ping command
       */
      len = WideCharToMultiByte(CP_UNIXCP, 0, hostW, -1, NULL, 0, NULL, NULL);
      command = HeapAlloc( GetProcessHeap(), 0, strlen(ping)+len+strlen(redirect) );
      strcpy(command,ping);
      WideCharToMultiByte(CP_UNIXCP, 0, hostW, -1, command+strlen(ping), len, NULL, NULL);
      strcat(command,redirect);

      TRACE("Ping command is : %s\n",command);

      status = system(command);

      TRACE("Ping returned a code of %i\n",status);

      /* Ping return code of 0 indicates success */
      if (status == 0)
         rc = TRUE;
  }

End:

  HeapFree( GetProcessHeap(), 0, command );
  if (rc == FALSE)
    INTERNET_SetLastError(ERROR_NOT_CONNECTED);

  return rc;
}


/***********************************************************************
 *	InternetCheckConnectionA (WININET.@)
 *
 * Pings a requested host to check internet connection
 *
 * RETURNS
 *   TRUE on success and FALSE on failure. If a failure then
 *   ERROR_NOT_CONNECTED is placed into GetLastError
 *
 */
BOOL WINAPI InternetCheckConnectionA(LPCSTR lpszUrl, DWORD dwFlags, DWORD dwReserved)
{
    WCHAR *szUrl;
    INT len;
    BOOL rc;

    len = MultiByteToWideChar(CP_ACP, 0, lpszUrl, -1, NULL, 0);
    if (!(szUrl = HeapAlloc(GetProcessHeap(), 0, len*sizeof(WCHAR))))
        return FALSE;
    MultiByteToWideChar(CP_ACP, 0, lpszUrl, -1, szUrl, len);
    rc = InternetCheckConnectionW(szUrl, dwFlags, dwReserved);
    HeapFree(GetProcessHeap(), 0, szUrl);
    
    return rc;
}


/**********************************************************
 *	INTERNET_InternetOpenUrlW (internal)
 *
 * Opens an URL
 *
 * RETURNS
 *   handle of connection or NULL on failure
 */
static HINTERNET INTERNET_InternetOpenUrlW(LPWININETAPPINFOW hIC, LPCWSTR lpszUrl,
    LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext)
{
    URL_COMPONENTSW urlComponents;
    WCHAR protocol[32], hostName[MAXHOSTNAME], userName[1024];
    WCHAR password[1024], path[2048], extra[1024];
    HINTERNET client = NULL, client1 = NULL;
    
    TRACE("(%p, %s, %s, %08x, %08x, %08lx)\n", hIC, debugstr_w(lpszUrl), debugstr_w(lpszHeaders),
	  dwHeadersLength, dwFlags, dwContext);
    
    urlComponents.dwStructSize = sizeof(URL_COMPONENTSW);
    urlComponents.lpszScheme = protocol;
    urlComponents.dwSchemeLength = 32;
    urlComponents.lpszHostName = hostName;
    urlComponents.dwHostNameLength = MAXHOSTNAME;
    urlComponents.lpszUserName = userName;
    urlComponents.dwUserNameLength = 1024;
    urlComponents.lpszPassword = password;
    urlComponents.dwPasswordLength = 1024;
    urlComponents.lpszUrlPath = path;
    urlComponents.dwUrlPathLength = 2048;
    urlComponents.lpszExtraInfo = extra;
    urlComponents.dwExtraInfoLength = 1024;
    if(!InternetCrackUrlW(lpszUrl, strlenW(lpszUrl), 0, &urlComponents))
	return NULL;
    switch(urlComponents.nScheme) {
    case INTERNET_SCHEME_FTP:
	if(urlComponents.nPort == 0)
	    urlComponents.nPort = INTERNET_DEFAULT_FTP_PORT;
	client = FTP_Connect(hIC, hostName, urlComponents.nPort,
			     userName, password, dwFlags, dwContext, INET_OPENURL);
	if(client == NULL)
	    break;
	client1 = FtpOpenFileW(client, path, GENERIC_READ, dwFlags, dwContext);
	if(client1 == NULL) {
	    InternetCloseHandle(client);
	    break;
	}
	break;
	
    case INTERNET_SCHEME_HTTP:
    case INTERNET_SCHEME_HTTPS: {
	static const WCHAR szStars[] = { '*','/','*', 0 };
	LPCWSTR accept[2] = { szStars, NULL };
	if(urlComponents.nPort == 0) {
	    if(urlComponents.nScheme == INTERNET_SCHEME_HTTP)
		urlComponents.nPort = INTERNET_DEFAULT_HTTP_PORT;
	    else
		urlComponents.nPort = INTERNET_DEFAULT_HTTPS_PORT;
	}
        if (urlComponents.nScheme == INTERNET_SCHEME_HTTPS) dwFlags |= INTERNET_FLAG_SECURE;

        /* FIXME: should use pointers, not handles, as handles are not thread-safe */
	client = HTTP_Connect(hIC, hostName, urlComponents.nPort,
			      userName, password, dwFlags, dwContext, INET_OPENURL);
	if(client == NULL)
	    break;

	if (urlComponents.dwExtraInfoLength) {
		WCHAR *path_extra;
		DWORD len = urlComponents.dwUrlPathLength + urlComponents.dwExtraInfoLength + 1;

		if (!(path_extra = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR))))
		{
			InternetCloseHandle(client);
			break;
		}
		strcpyW(path_extra, urlComponents.lpszUrlPath);
		strcatW(path_extra, urlComponents.lpszExtraInfo);
		client1 = HttpOpenRequestW(client, NULL, path_extra, NULL, NULL, accept, dwFlags, dwContext);
		HeapFree(GetProcessHeap(), 0, path_extra);
	}
	else
		client1 = HttpOpenRequestW(client, NULL, path, NULL, NULL, accept, dwFlags, dwContext);

	if(client1 == NULL) {
	    InternetCloseHandle(client);
	    break;
	}
	HttpAddRequestHeadersW(client1, lpszHeaders, dwHeadersLength, HTTP_ADDREQ_FLAG_ADD);
	if (!HttpSendRequestW(client1, NULL, 0, NULL, 0) &&
            GetLastError() != ERROR_IO_PENDING) {
	    InternetCloseHandle(client1);
	    client1 = NULL;
	    break;
	}
    }
    case INTERNET_SCHEME_GOPHER:
	/* gopher doesn't seem to be implemented in wine, but it's supposed
	 * to be supported by InternetOpenUrlA. */
    default:
        INTERNET_SetLastError(ERROR_INTERNET_UNRECOGNIZED_SCHEME);
	break;
    }

    TRACE(" %p <--\n", client1);
    
    return client1;
}

/**********************************************************
 *	InternetOpenUrlW (WININET.@)
 *
 * Opens an URL
 *
 * RETURNS
 *   handle of connection or NULL on failure
 */
static void AsyncInternetOpenUrlProc(WORKREQUEST *workRequest)
{
    struct WORKREQ_INTERNETOPENURLW const *req = &workRequest->u.InternetOpenUrlW;
    LPWININETAPPINFOW hIC = (LPWININETAPPINFOW) workRequest->hdr;

    TRACE("%p\n", hIC);

    INTERNET_InternetOpenUrlW(hIC, req->lpszUrl,
                              req->lpszHeaders, req->dwHeadersLength, req->dwFlags, req->dwContext);
    HeapFree(GetProcessHeap(), 0, req->lpszUrl);
    HeapFree(GetProcessHeap(), 0, req->lpszHeaders);
}

HINTERNET WINAPI InternetOpenUrlW(HINTERNET hInternet, LPCWSTR lpszUrl,
    LPCWSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext)
{
    HINTERNET ret = NULL;
    LPWININETAPPINFOW hIC = NULL;

    if (TRACE_ON(wininet)) {
	TRACE("(%p, %s, %s, %08x, %08x, %08lx)\n", hInternet, debugstr_w(lpszUrl), debugstr_w(lpszHeaders),
	      dwHeadersLength, dwFlags, dwContext);
	TRACE("  flags :");
	dump_INTERNET_FLAGS(dwFlags);
    }

    if (!lpszUrl)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        goto lend;
    }

    hIC = (LPWININETAPPINFOW) WININET_GetObject( hInternet );
    if (NULL == hIC ||  hIC->hdr.htype != WH_HINIT) {
	INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
 	goto lend;
    }
    
    if (hIC->hdr.dwFlags & INTERNET_FLAG_ASYNC) {
	WORKREQUEST workRequest;
	struct WORKREQ_INTERNETOPENURLW *req;

	workRequest.asyncproc = AsyncInternetOpenUrlProc;
	workRequest.hdr = WININET_AddRef( &hIC->hdr );
	req = &workRequest.u.InternetOpenUrlW;
	req->lpszUrl = WININET_strdupW(lpszUrl);
	if (lpszHeaders)
	    req->lpszHeaders = WININET_strdupW(lpszHeaders);
	else
	    req->lpszHeaders = 0;
	req->dwHeadersLength = dwHeadersLength;
	req->dwFlags = dwFlags;
	req->dwContext = dwContext;
	
	INTERNET_AsyncCall(&workRequest);
	/*
	 * This is from windows.
	 */
	INTERNET_SetLastError(ERROR_IO_PENDING);
    } else {
	ret = INTERNET_InternetOpenUrlW(hIC, lpszUrl, lpszHeaders, dwHeadersLength, dwFlags, dwContext);
    }
    
  lend:
    if( hIC )
        WININET_Release( &hIC->hdr );
    TRACE(" %p <--\n", ret);
    
    return ret;
}

/**********************************************************
 *	InternetOpenUrlA (WININET.@)
 *
 * Opens an URL
 *
 * RETURNS
 *   handle of connection or NULL on failure
 */
HINTERNET WINAPI InternetOpenUrlA(HINTERNET hInternet, LPCSTR lpszUrl,
    LPCSTR lpszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext)
{
    HINTERNET rc = NULL;

    INT lenUrl;
    INT lenHeaders = 0;
    LPWSTR szUrl = NULL;
    LPWSTR szHeaders = NULL;

    TRACE("\n");

    if(lpszUrl) {
        lenUrl = MultiByteToWideChar(CP_ACP, 0, lpszUrl, -1, NULL, 0 );
        szUrl = HeapAlloc(GetProcessHeap(), 0, lenUrl*sizeof(WCHAR));
        if(!szUrl)
            return NULL;
        MultiByteToWideChar(CP_ACP, 0, lpszUrl, -1, szUrl, lenUrl);
    }

    if(lpszHeaders) {
        lenHeaders = MultiByteToWideChar(CP_ACP, 0, lpszHeaders, dwHeadersLength, NULL, 0 );
        szHeaders = HeapAlloc(GetProcessHeap(), 0, lenHeaders*sizeof(WCHAR));
        if(!szHeaders) {
            HeapFree(GetProcessHeap(), 0, szUrl);
            return NULL;
        }
        MultiByteToWideChar(CP_ACP, 0, lpszHeaders, dwHeadersLength, szHeaders, lenHeaders);
    }
    
    rc = InternetOpenUrlW(hInternet, szUrl, szHeaders,
        lenHeaders, dwFlags, dwContext);

    HeapFree(GetProcessHeap(), 0, szUrl);
    HeapFree(GetProcessHeap(), 0, szHeaders);

    return rc;
}


static LPWITHREADERROR INTERNET_AllocThreadError(void)
{
    LPWITHREADERROR lpwite = HeapAlloc(GetProcessHeap(), 0, sizeof(*lpwite));

    if (lpwite)
    {
        lpwite->dwError = 0;
        lpwite->response[0] = '\0';
    }

    if (!TlsSetValue(g_dwTlsErrIndex, lpwite))
    {
        HeapFree(GetProcessHeap(), 0, lpwite);
        return NULL;
    }

    return lpwite;
}


/***********************************************************************
 *           INTERNET_SetLastError (internal)
 *
 * Set last thread specific error
 *
 * RETURNS
 *
 */
void INTERNET_SetLastError(DWORD dwError)
{
    LPWITHREADERROR lpwite = TlsGetValue(g_dwTlsErrIndex);

    if (!lpwite)
        lpwite = INTERNET_AllocThreadError();

    SetLastError(dwError);
    if(lpwite)
        lpwite->dwError = dwError;
}


/***********************************************************************
 *           INTERNET_GetLastError (internal)
 *
 * Get last thread specific error
 *
 * RETURNS
 *
 */
DWORD INTERNET_GetLastError(void)
{
    LPWITHREADERROR lpwite = TlsGetValue(g_dwTlsErrIndex);
    if (!lpwite) return 0;
    /* TlsGetValue clears last error, so set it again here */
    SetLastError(lpwite->dwError);
    return lpwite->dwError;
}


/***********************************************************************
 *           INTERNET_WorkerThreadFunc (internal)
 *
 * Worker thread execution function
 *
 * RETURNS
 *
 */
static DWORD CALLBACK INTERNET_WorkerThreadFunc(LPVOID lpvParam)
{
    LPWORKREQUEST lpRequest = lpvParam;
    WORKREQUEST workRequest;

    TRACE("\n");

    workRequest = *lpRequest;
    HeapFree(GetProcessHeap(), 0, lpRequest);

    workRequest.asyncproc(&workRequest);

    WININET_Release( workRequest.hdr );
    return TRUE;
}


/***********************************************************************
 *           INTERNET_AsyncCall (internal)
 *
 * Retrieves work request from queue
 *
 * RETURNS
 *
 */
BOOL INTERNET_AsyncCall(LPWORKREQUEST lpWorkRequest)
{
    BOOL bSuccess;
    LPWORKREQUEST lpNewRequest;

    TRACE("\n");

    lpNewRequest = HeapAlloc(GetProcessHeap(), 0, sizeof(WORKREQUEST));
    if (!lpNewRequest)
        return FALSE;

    *lpNewRequest = *lpWorkRequest;

    bSuccess = QueueUserWorkItem(INTERNET_WorkerThreadFunc, lpNewRequest, WT_EXECUTELONGFUNCTION);
    if (!bSuccess)
    {
        HeapFree(GetProcessHeap(), 0, lpNewRequest);
        INTERNET_SetLastError(ERROR_INTERNET_ASYNC_THREAD_FAILED);
    }

    return bSuccess;
}


/***********************************************************************
 *          INTERNET_GetResponseBuffer  (internal)
 *
 * RETURNS
 *
 */
LPSTR INTERNET_GetResponseBuffer(void)
{
    LPWITHREADERROR lpwite = TlsGetValue(g_dwTlsErrIndex);
    if (!lpwite)
        lpwite = INTERNET_AllocThreadError();
    TRACE("\n");
    return lpwite->response;
}

/***********************************************************************
 *           INTERNET_GetNextLine  (internal)
 *
 * Parse next line in directory string listing
 *
 * RETURNS
 *   Pointer to beginning of next line
 *   NULL on failure
 *
 */

LPSTR INTERNET_GetNextLine(INT nSocket, LPDWORD dwLen)
{
    struct timeval tv;
    fd_set infd;
    BOOL bSuccess = FALSE;
    INT nRecv = 0;
    LPSTR lpszBuffer = INTERNET_GetResponseBuffer();

    TRACE("\n");

    FD_ZERO(&infd);
    FD_SET(nSocket, &infd);
    tv.tv_sec=RESPONSE_TIMEOUT;
    tv.tv_usec=0;

    while (nRecv < MAX_REPLY_LEN)
    {
        if (select(nSocket+1,&infd,NULL,NULL,&tv) > 0)
        {
            if (recv(nSocket, &lpszBuffer[nRecv], 1, 0) <= 0)
            {
                INTERNET_SetLastError(ERROR_FTP_TRANSFER_IN_PROGRESS);
                goto lend;
            }

            if (lpszBuffer[nRecv] == '\n')
	    {
		bSuccess = TRUE;
                break;
	    }
            if (lpszBuffer[nRecv] != '\r')
                nRecv++;
        }
	else
	{
            INTERNET_SetLastError(ERROR_INTERNET_TIMEOUT);
            goto lend;
        }
    }

lend:
    if (bSuccess)
    {
        lpszBuffer[nRecv] = '\0';
	*dwLen = nRecv - 1;
        TRACE(":%d %s\n", nRecv, lpszBuffer);
        return lpszBuffer;
    }
    else
    {
        return NULL;
    }
}

/**********************************************************
 *	InternetQueryDataAvailable (WININET.@)
 *
 * Determines how much data is available to be read.
 *
 * RETURNS
 *   TRUE on success, FALSE if an error occurred. If
 *   INTERNET_FLAG_ASYNC was specified in InternetOpen, and
 *   no data is presently available, FALSE is returned with
 *   the last error ERROR_IO_PENDING; a callback with status
 *   INTERNET_STATUS_REQUEST_COMPLETE will be sent when more
 *   data is available.
 */
BOOL WINAPI InternetQueryDataAvailable( HINTERNET hFile,
                                LPDWORD lpdwNumberOfBytesAvailble,
                                DWORD dwFlags, DWORD_PTR dwContext)
{
    WININETHANDLEHEADER *hdr;
    DWORD res;

    TRACE("(%p %p %x %lx)\n", hFile, lpdwNumberOfBytesAvailble, dwFlags, dwContext);

    hdr = WININET_GetObject( hFile );
    if (!hdr) {
        INTERNET_SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(hdr->vtbl->QueryDataAvailable) {
        res = hdr->vtbl->QueryDataAvailable(hdr, lpdwNumberOfBytesAvailble, dwFlags, dwContext);
    }else {
        WARN("wrong handle\n");
        res = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
    }

    WININET_Release(hdr);

    if(res != ERROR_SUCCESS)
        SetLastError(res);
    return res == ERROR_SUCCESS;
}


/***********************************************************************
 *      InternetLockRequestFile (WININET.@)
 */
BOOL WINAPI InternetLockRequestFile( HINTERNET hInternet, HANDLE
*lphLockReqHandle)
{
    FIXME("STUB\n");
    return FALSE;
}

BOOL WINAPI InternetUnlockRequestFile( HANDLE hLockHandle)
{
    FIXME("STUB\n");
    return FALSE;
}


/***********************************************************************
 *      InternetAutodial (WININET.@)
 *
 * On windows this function is supposed to dial the default internet
 * connection. We don't want to have Wine dial out to the internet so
 * we return TRUE by default. It might be nice to check if we are connected.
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */
BOOL WINAPI InternetAutodial(DWORD dwFlags, HWND hwndParent)
{
    FIXME("STUB\n");

    /* Tell that we are connected to the internet. */
    return TRUE;
}

/***********************************************************************
 *      InternetAutodialHangup (WININET.@)
 *
 * Hangs up a connection made with InternetAutodial
 *
 * PARAM
 *    dwReserved
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */
BOOL WINAPI InternetAutodialHangup(DWORD dwReserved)
{
    FIXME("STUB\n");

    /* we didn't dial, we don't disconnect */
    return TRUE;
}

/***********************************************************************
 *      InternetCombineUrlA (WININET.@)
 *
 * Combine a base URL with a relative URL
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */

BOOL WINAPI InternetCombineUrlA(LPCSTR lpszBaseUrl, LPCSTR lpszRelativeUrl,
                                LPSTR lpszBuffer, LPDWORD lpdwBufferLength,
                                DWORD dwFlags)
{
    HRESULT hr=S_OK;

    TRACE("(%s, %s, %p, %p, 0x%08x)\n", debugstr_a(lpszBaseUrl), debugstr_a(lpszRelativeUrl), lpszBuffer, lpdwBufferLength, dwFlags);

    /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
    dwFlags ^= ICU_NO_ENCODE;
    hr=UrlCombineA(lpszBaseUrl,lpszRelativeUrl,lpszBuffer,lpdwBufferLength,dwFlags);

    return (hr==S_OK);
}

/***********************************************************************
 *      InternetCombineUrlW (WININET.@)
 *
 * Combine a base URL with a relative URL
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */

BOOL WINAPI InternetCombineUrlW(LPCWSTR lpszBaseUrl, LPCWSTR lpszRelativeUrl,
                                LPWSTR lpszBuffer, LPDWORD lpdwBufferLength,
                                DWORD dwFlags)
{
    HRESULT hr=S_OK;

    TRACE("(%s, %s, %p, %p, 0x%08x)\n", debugstr_w(lpszBaseUrl), debugstr_w(lpszRelativeUrl), lpszBuffer, lpdwBufferLength, dwFlags);

    /* Flip this bit to correspond to URL_ESCAPE_UNSAFE */
    dwFlags ^= ICU_NO_ENCODE;
    hr=UrlCombineW(lpszBaseUrl,lpszRelativeUrl,lpszBuffer,lpdwBufferLength,dwFlags);

    return (hr==S_OK);
}

/* max port num is 65535 => 5 digits */
#define MAX_WORD_DIGITS 5

#define URL_GET_COMP_LENGTH(url, component) ((url)->dw##component##Length ? \
    (url)->dw##component##Length : strlenW((url)->lpsz##component))
#define URL_GET_COMP_LENGTHA(url, component) ((url)->dw##component##Length ? \
    (url)->dw##component##Length : strlen((url)->lpsz##component))

static BOOL url_uses_default_port(INTERNET_SCHEME nScheme, INTERNET_PORT nPort)
{
    if ((nScheme == INTERNET_SCHEME_HTTP) &&
        (nPort == INTERNET_DEFAULT_HTTP_PORT))
        return TRUE;
    if ((nScheme == INTERNET_SCHEME_HTTPS) &&
        (nPort == INTERNET_DEFAULT_HTTPS_PORT))
        return TRUE;
    if ((nScheme == INTERNET_SCHEME_FTP) &&
        (nPort == INTERNET_DEFAULT_FTP_PORT))
        return TRUE;
    if ((nScheme == INTERNET_SCHEME_GOPHER) &&
        (nPort == INTERNET_DEFAULT_GOPHER_PORT))
        return TRUE;

    if (nPort == INTERNET_INVALID_PORT_NUMBER)
        return TRUE;

    return FALSE;
}

/* opaque urls do not fit into the standard url hierarchy and don't have
 * two following slashes */
static inline BOOL scheme_is_opaque(INTERNET_SCHEME nScheme)
{
    return (nScheme != INTERNET_SCHEME_FTP) &&
           (nScheme != INTERNET_SCHEME_GOPHER) &&
           (nScheme != INTERNET_SCHEME_HTTP) &&
           (nScheme != INTERNET_SCHEME_HTTPS) &&
           (nScheme != INTERNET_SCHEME_FILE);
}

static LPCWSTR INTERNET_GetSchemeString(INTERNET_SCHEME scheme)
{
    int index;
    if (scheme < INTERNET_SCHEME_FIRST)
        return NULL;
    index = scheme - INTERNET_SCHEME_FIRST;
    if (index >= sizeof(url_schemes)/sizeof(url_schemes[0]))
        return NULL;
    return (LPCWSTR)url_schemes[index];
}

/* we can calculate using ansi strings because we're just
 * calculating string length, not size
 */
static BOOL calc_url_length(LPURL_COMPONENTSW lpUrlComponents,
                            LPDWORD lpdwUrlLength)
{
    INTERNET_SCHEME nScheme;

    *lpdwUrlLength = 0;

    if (lpUrlComponents->lpszScheme)
    {
        DWORD dwLen = URL_GET_COMP_LENGTH(lpUrlComponents, Scheme);
        *lpdwUrlLength += dwLen;
        nScheme = GetInternetSchemeW(lpUrlComponents->lpszScheme, dwLen);
    }
    else
    {
        LPCWSTR scheme;

        nScheme = lpUrlComponents->nScheme;

        if (nScheme == INTERNET_SCHEME_DEFAULT)
            nScheme = INTERNET_SCHEME_HTTP;
        scheme = INTERNET_GetSchemeString(nScheme);
        *lpdwUrlLength += strlenW(scheme);
    }

    (*lpdwUrlLength)++; /* ':' */
    if (!scheme_is_opaque(nScheme) || lpUrlComponents->lpszHostName)
        *lpdwUrlLength += strlen("//");

    if (lpUrlComponents->lpszUserName)
    {
        *lpdwUrlLength += URL_GET_COMP_LENGTH(lpUrlComponents, UserName);
        *lpdwUrlLength += strlen("@");
    }
    else
    {
        if (lpUrlComponents->lpszPassword)
        {
            INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    if (lpUrlComponents->lpszPassword)
    {
        *lpdwUrlLength += strlen(":");
        *lpdwUrlLength += URL_GET_COMP_LENGTH(lpUrlComponents, Password);
    }

    if (lpUrlComponents->lpszHostName)
    {
        *lpdwUrlLength += URL_GET_COMP_LENGTH(lpUrlComponents, HostName);

        if (!url_uses_default_port(nScheme, lpUrlComponents->nPort))
        {
            char szPort[MAX_WORD_DIGITS+1];

            sprintf(szPort, "%d", lpUrlComponents->nPort);
            *lpdwUrlLength += strlen(szPort);
            *lpdwUrlLength += strlen(":");
        }

        if (lpUrlComponents->lpszUrlPath && *lpUrlComponents->lpszUrlPath != '/')
            (*lpdwUrlLength)++; /* '/' */
    }

    if (lpUrlComponents->lpszUrlPath)
        *lpdwUrlLength += URL_GET_COMP_LENGTH(lpUrlComponents, UrlPath);

    if (lpUrlComponents->lpszExtraInfo)
        *lpdwUrlLength += URL_GET_COMP_LENGTH(lpUrlComponents, ExtraInfo);

    return TRUE;
}

static void convert_urlcomp_atow(LPURL_COMPONENTSA lpUrlComponents, LPURL_COMPONENTSW urlCompW)
{
    INT len;

    ZeroMemory(urlCompW, sizeof(URL_COMPONENTSW));

    urlCompW->dwStructSize = sizeof(URL_COMPONENTSW);
    urlCompW->dwSchemeLength = lpUrlComponents->dwSchemeLength;
    urlCompW->nScheme = lpUrlComponents->nScheme;
    urlCompW->dwHostNameLength = lpUrlComponents->dwHostNameLength;
    urlCompW->nPort = lpUrlComponents->nPort;
    urlCompW->dwUserNameLength = lpUrlComponents->dwUserNameLength;
    urlCompW->dwPasswordLength = lpUrlComponents->dwPasswordLength;
    urlCompW->dwUrlPathLength = lpUrlComponents->dwUrlPathLength;
    urlCompW->dwExtraInfoLength = lpUrlComponents->dwExtraInfoLength;

    if (lpUrlComponents->lpszScheme)
    {
        len = URL_GET_COMP_LENGTHA(lpUrlComponents, Scheme) + 1;
        urlCompW->lpszScheme = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpUrlComponents->lpszScheme,
                            -1, urlCompW->lpszScheme, len);
    }

    if (lpUrlComponents->lpszHostName)
    {
        len = URL_GET_COMP_LENGTHA(lpUrlComponents, HostName) + 1;
        urlCompW->lpszHostName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpUrlComponents->lpszHostName,
                            -1, urlCompW->lpszHostName, len);
    }

    if (lpUrlComponents->lpszUserName)
    {
        len = URL_GET_COMP_LENGTHA(lpUrlComponents, UserName) + 1;
        urlCompW->lpszUserName = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpUrlComponents->lpszUserName,
                            -1, urlCompW->lpszUserName, len);
    }

    if (lpUrlComponents->lpszPassword)
    {
        len = URL_GET_COMP_LENGTHA(lpUrlComponents, Password) + 1;
        urlCompW->lpszPassword = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpUrlComponents->lpszPassword,
                            -1, urlCompW->lpszPassword, len);
    }

    if (lpUrlComponents->lpszUrlPath)
    {
        len = URL_GET_COMP_LENGTHA(lpUrlComponents, UrlPath) + 1;
        urlCompW->lpszUrlPath = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpUrlComponents->lpszUrlPath,
                            -1, urlCompW->lpszUrlPath, len);
    }

    if (lpUrlComponents->lpszExtraInfo)
    {
        len = URL_GET_COMP_LENGTHA(lpUrlComponents, ExtraInfo) + 1;
        urlCompW->lpszExtraInfo = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
        MultiByteToWideChar(CP_ACP, 0, lpUrlComponents->lpszExtraInfo,
                            -1, urlCompW->lpszExtraInfo, len);
    }
}

/***********************************************************************
 *      InternetCreateUrlA (WININET.@)
 *
 * See InternetCreateUrlW.
 */
BOOL WINAPI InternetCreateUrlA(LPURL_COMPONENTSA lpUrlComponents, DWORD dwFlags,
                               LPSTR lpszUrl, LPDWORD lpdwUrlLength)
{
    BOOL ret;
    LPWSTR urlW = NULL;
    URL_COMPONENTSW urlCompW;

    TRACE("(%p,%d,%p,%p)\n", lpUrlComponents, dwFlags, lpszUrl, lpdwUrlLength);

    if (!lpUrlComponents || lpUrlComponents->dwStructSize != sizeof(URL_COMPONENTSW) || !lpdwUrlLength)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    convert_urlcomp_atow(lpUrlComponents, &urlCompW);

    if (lpszUrl)
        urlW = HeapAlloc(GetProcessHeap(), 0, *lpdwUrlLength * sizeof(WCHAR));

    ret = InternetCreateUrlW(&urlCompW, dwFlags, urlW, lpdwUrlLength);

    if (!ret && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        *lpdwUrlLength /= sizeof(WCHAR);

    /* on success, lpdwUrlLength points to the size of urlW in WCHARS
    * minus one, so add one to leave room for NULL terminator
    */
    if (ret)
        WideCharToMultiByte(CP_ACP, 0, urlW, -1, lpszUrl, *lpdwUrlLength + 1, NULL, NULL);

    HeapFree(GetProcessHeap(), 0, urlCompW.lpszScheme);
    HeapFree(GetProcessHeap(), 0, urlCompW.lpszHostName);
    HeapFree(GetProcessHeap(), 0, urlCompW.lpszUserName);
    HeapFree(GetProcessHeap(), 0, urlCompW.lpszPassword);
    HeapFree(GetProcessHeap(), 0, urlCompW.lpszUrlPath);
    HeapFree(GetProcessHeap(), 0, urlCompW.lpszExtraInfo);
    HeapFree(GetProcessHeap(), 0, urlW);

    return ret;
}

/***********************************************************************
 *      InternetCreateUrlW (WININET.@)
 *
 * Creates a URL from its component parts.
 *
 * PARAMS
 *  lpUrlComponents [I] URL Components.
 *  dwFlags         [I] Flags. See notes.
 *  lpszUrl         [I] Buffer in which to store the created URL.
 *  lpdwUrlLength   [I/O] On input, the length of the buffer pointed to by
 *                        lpszUrl in characters. On output, the number of bytes
 *                        required to store the URL including terminator.
 *
 * NOTES
 *
 * The dwFlags parameter can be zero or more of the following:
 *|ICU_ESCAPE - Generates escape sequences for unsafe characters in the path and extra info of the URL.
 *
 * RETURNS
 *   TRUE on success
 *   FALSE on failure
 *
 */
BOOL WINAPI InternetCreateUrlW(LPURL_COMPONENTSW lpUrlComponents, DWORD dwFlags,
                               LPWSTR lpszUrl, LPDWORD lpdwUrlLength)
{
    DWORD dwLen;
    INTERNET_SCHEME nScheme;

    static const WCHAR slashSlashW[] = {'/','/'};
    static const WCHAR percentD[] = {'%','d',0};

    TRACE("(%p,%d,%p,%p)\n", lpUrlComponents, dwFlags, lpszUrl, lpdwUrlLength);

    if (!lpUrlComponents || lpUrlComponents->dwStructSize != sizeof(URL_COMPONENTSW) || !lpdwUrlLength)
    {
        INTERNET_SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!calc_url_length(lpUrlComponents, &dwLen))
        return FALSE;

    if (!lpszUrl || *lpdwUrlLength < dwLen)
    {
        *lpdwUrlLength = (dwLen + 1) * sizeof(WCHAR);
        INTERNET_SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return FALSE;
    }

    *lpdwUrlLength = dwLen;
    lpszUrl[0] = 0x00;

    dwLen = 0;

    if (lpUrlComponents->lpszScheme)
    {
        dwLen = URL_GET_COMP_LENGTH(lpUrlComponents, Scheme);
        memcpy(lpszUrl, lpUrlComponents->lpszScheme, dwLen * sizeof(WCHAR));
        lpszUrl += dwLen;

        nScheme = GetInternetSchemeW(lpUrlComponents->lpszScheme, dwLen);
    }
    else
    {
        LPCWSTR scheme;
        nScheme = lpUrlComponents->nScheme;

        if (nScheme == INTERNET_SCHEME_DEFAULT)
            nScheme = INTERNET_SCHEME_HTTP;

        scheme = INTERNET_GetSchemeString(nScheme);
        dwLen = strlenW(scheme);
        memcpy(lpszUrl, scheme, dwLen * sizeof(WCHAR));
        lpszUrl += dwLen;
    }

    /* all schemes are followed by at least a colon */
    *lpszUrl = ':';
    lpszUrl++;

    if (!scheme_is_opaque(nScheme) || lpUrlComponents->lpszHostName)
    {
        memcpy(lpszUrl, slashSlashW, sizeof(slashSlashW));
        lpszUrl += sizeof(slashSlashW)/sizeof(slashSlashW[0]);
    }

    if (lpUrlComponents->lpszUserName)
    {
        dwLen = URL_GET_COMP_LENGTH(lpUrlComponents, UserName);
        memcpy(lpszUrl, lpUrlComponents->lpszUserName, dwLen * sizeof(WCHAR));
        lpszUrl += dwLen;

        if (lpUrlComponents->lpszPassword)
        {
            *lpszUrl = ':';
            lpszUrl++;

            dwLen = URL_GET_COMP_LENGTH(lpUrlComponents, Password);
            memcpy(lpszUrl, lpUrlComponents->lpszPassword, dwLen * sizeof(WCHAR));
            lpszUrl += dwLen;
        }

        *lpszUrl = '@';
        lpszUrl++;
    }

    if (lpUrlComponents->lpszHostName)
    {
        dwLen = URL_GET_COMP_LENGTH(lpUrlComponents, HostName);
        memcpy(lpszUrl, lpUrlComponents->lpszHostName, dwLen * sizeof(WCHAR));
        lpszUrl += dwLen;

        if (!url_uses_default_port(nScheme, lpUrlComponents->nPort))
        {
            WCHAR szPort[MAX_WORD_DIGITS+1];

            sprintfW(szPort, percentD, lpUrlComponents->nPort);
            *lpszUrl = ':';
            lpszUrl++;
            dwLen = strlenW(szPort);
            memcpy(lpszUrl, szPort, dwLen * sizeof(WCHAR));
            lpszUrl += dwLen;
        }

        /* add slash between hostname and path if necessary */
        if (lpUrlComponents->lpszUrlPath && *lpUrlComponents->lpszUrlPath != '/')
        {
            *lpszUrl = '/';
            lpszUrl++;
        }
    }

    if (lpUrlComponents->lpszUrlPath)
    {
        dwLen = URL_GET_COMP_LENGTH(lpUrlComponents, UrlPath);
        memcpy(lpszUrl, lpUrlComponents->lpszUrlPath, dwLen * sizeof(WCHAR));
        lpszUrl += dwLen;
    }

    if (lpUrlComponents->lpszExtraInfo)
    {
        dwLen = URL_GET_COMP_LENGTH(lpUrlComponents, ExtraInfo);
        memcpy(lpszUrl, lpUrlComponents->lpszExtraInfo, dwLen * sizeof(WCHAR));
        lpszUrl += dwLen;
    }

    *lpszUrl = '\0';

    return TRUE;
}

/***********************************************************************
 *      InternetConfirmZoneCrossingA (WININET.@)
 *
 */
DWORD WINAPI InternetConfirmZoneCrossingA( HWND hWnd, LPSTR szUrlPrev, LPSTR szUrlNew, BOOL bPost )
{
    FIXME("(%p, %s, %s, %x) stub\n", hWnd, debugstr_a(szUrlPrev), debugstr_a(szUrlNew), bPost);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *      InternetConfirmZoneCrossingW (WININET.@)
 *
 */
DWORD WINAPI InternetConfirmZoneCrossingW( HWND hWnd, LPWSTR szUrlPrev, LPWSTR szUrlNew, BOOL bPost )
{
    FIXME("(%p, %s, %s, %x) stub\n", hWnd, debugstr_w(szUrlPrev), debugstr_w(szUrlNew), bPost);
    return ERROR_SUCCESS;
}

/***********************************************************************
 *      PrivacySetZonePreferenceW (WININET.@)
 */
DWORD WINAPI PrivacySetZonePreferenceW( DWORD zone, DWORD type, DWORD template, LPCWSTR preference )
{
    FIXME( "%x %x %x %s: stub\n", zone, type, template, debugstr_w(preference) );
    return 0;
}

/***********************************************************************
 *      PrivacyGetZonePreferenceW (WININET.@)
 */
DWORD WINAPI PrivacyGetZonePreferenceW( DWORD zone, DWORD type, LPDWORD template,
                                        LPWSTR preference, LPDWORD length )
{
    FIXME( "%x %x: stub\n", zone, type );
    return 0;
}

DWORD WINAPI InternetDialA( HWND hwndParent, LPSTR lpszConnectoid, DWORD dwFlags,
                            DWORD_PTR* lpdwConnection, DWORD dwReserved )
{
    FIXME("(%p, %p, 0x%08x, %p, 0x%08x) stub\n", hwndParent, lpszConnectoid, dwFlags,
          lpdwConnection, dwReserved);
    return ERROR_SUCCESS;
}

DWORD WINAPI InternetDialW( HWND hwndParent, LPWSTR lpszConnectoid, DWORD dwFlags,
                            DWORD_PTR* lpdwConnection, DWORD dwReserved )
{
    FIXME("(%p, %p, 0x%08x, %p, 0x%08x) stub\n", hwndParent, lpszConnectoid, dwFlags,
          lpdwConnection, dwReserved);
    return ERROR_SUCCESS;
}

BOOL WINAPI InternetGoOnlineA( LPSTR lpszURL, HWND hwndParent, DWORD dwReserved )
{
    FIXME("(%s, %p, 0x%08x) stub\n", debugstr_a(lpszURL), hwndParent, dwReserved);
    return TRUE;
}

BOOL WINAPI InternetGoOnlineW( LPWSTR lpszURL, HWND hwndParent, DWORD dwReserved )
{
    FIXME("(%s, %p, 0x%08x) stub\n", debugstr_w(lpszURL), hwndParent, dwReserved);
    return TRUE;
}

DWORD WINAPI InternetHangUp( DWORD_PTR dwConnection, DWORD dwReserved )
{
    FIXME("(0x%08lx, 0x%08x) stub\n", dwConnection, dwReserved);
    return ERROR_SUCCESS;
}

BOOL WINAPI CreateMD5SSOHash( PWSTR pszChallengeInfo, PWSTR pwszRealm, PWSTR pwszTarget,
                              PBYTE pbHexHash )
{
    FIXME("(%s, %s, %s, %p) stub\n", debugstr_w(pszChallengeInfo), debugstr_w(pwszRealm),
          debugstr_w(pwszTarget), pbHexHash);
    return FALSE;
}

BOOL WINAPI ResumeSuspendedDownload( HINTERNET hInternet, DWORD dwError )
{
    FIXME("(%p, 0x%08x) stub\n", hInternet, dwError);
    return FALSE;
}

BOOL WINAPI InternetQueryFortezzaStatus(DWORD *a, DWORD_PTR b)
{
    FIXME("(%p, %08lx) stub\n", a, b);
    return 0;
}
