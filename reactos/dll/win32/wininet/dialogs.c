/*
 * Wininet
 *
 * Copyright 2003 Mike McCormack for CodeWeavers Inc.
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "wininet.h"
#include "winnetwk.h"
#include "wine/debug.h"
#include "winerror.h"
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"

#include "internet.h"

#include "wine/unicode.h"

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(wininet);

struct WININET_ErrorDlgParams
{
    HWND       hWnd;
    HINTERNET  hRequest;
    DWORD      dwError;
    DWORD      dwFlags;
    LPVOID*    lppvData;
};

/***********************************************************************
 *         WININET_GetProxyServer
 *
 *  Determine the name of the proxy server the request is using
 */
static BOOL WININET_GetProxyServer( HINTERNET hRequest, LPWSTR szBuf, DWORD sz )
{
    LPWININETHTTPREQW lpwhr;
    LPWININETHTTPSESSIONW lpwhs = NULL;
    LPWININETAPPINFOW hIC = NULL;
    LPWSTR p;

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hRequest );
    if (NULL == lpwhr)
	return FALSE;

    lpwhs = lpwhr->lpHttpSession;
    if (NULL == lpwhs)
	return FALSE;

    hIC = lpwhs->lpAppInfo;
    if (NULL == hIC)
	return FALSE;

    lstrcpynW(szBuf, hIC->lpszProxy, sz);

    /* FIXME: perhaps it would be better to use InternetCrackUrl here */
    p = strchrW(szBuf, ':');
    if (p)
        *p = 0;

    return TRUE;
}

/***********************************************************************
 *         WININET_GetAuthRealm
 *
 *  Determine the name of the (basic) Authentication realm
 */
static BOOL WININET_GetAuthRealm( HINTERNET hRequest, LPWSTR szBuf, DWORD sz )
{
    LPWSTR p, q;
    DWORD index;
    static const WCHAR szRealm[] = { 'r','e','a','l','m','=',0 };

    /* extract the Realm from the proxy response and show it */
    index = 0;
    if( !HttpQueryInfoW( hRequest, HTTP_QUERY_PROXY_AUTHENTICATE,
                         szBuf, &sz, &index) )
        return FALSE;

    /*
     * FIXME: maybe we should check that we're
     * dealing with 'Basic' Authentication
     */
    p = strchrW( szBuf, ' ' );
    if( !p || strncmpW( p+1, szRealm, strlenW(szRealm) ) )
    {
        ERR("proxy response wrong? (%s)\n", debugstr_w(szBuf));
        return FALSE;
    }


    /* remove quotes */
    p += 7;
    if( *p == '"' )
    {
        p++;
        q = strrchrW( p, '"' );
        if( q )
            *q = 0;
    }
    strcpyW( szBuf, p );

    return TRUE;
}

/***********************************************************************
 *         WININET_GetSetPassword
 */
static BOOL WININET_GetSetPassword( HWND hdlg, LPCWSTR szServer, 
                                    LPCWSTR szRealm, BOOL bSet )
{
    WCHAR szResource[0x80], szUserPass[0x40];
    LPWSTR p;
    HWND hUserItem, hPassItem;
    DWORD r, dwMagic = 19;
    UINT r_len, u_len;
    WORD sz;
    static const WCHAR szColon[] = { ':',0 };
    static const WCHAR szbs[] = { '/', 0 };

    hUserItem = GetDlgItem( hdlg, IDC_USERNAME );
    hPassItem = GetDlgItem( hdlg, IDC_PASSWORD );

    /* now try fetch the username and password */
    lstrcpyW( szResource, szServer);
    lstrcatW( szResource, szbs);
    lstrcatW( szResource, szRealm);

    /*
     * WNetCachePassword is only concerned with the length
     * of the data stored (which we tell it) and it does
     * not use strlen() internally so we can add WCHAR data
     * instead of ASCII data and get it back the same way.
     */
    if( bSet )
    {
        szUserPass[0] = 0;
        GetWindowTextW( hUserItem, szUserPass, 
                        (sizeof szUserPass-1)/sizeof(WCHAR) );
        lstrcatW(szUserPass, szColon);
        u_len = strlenW( szUserPass );
        GetWindowTextW( hPassItem, szUserPass+u_len, 
                        (sizeof szUserPass)/sizeof(WCHAR)-u_len );

        r_len = (strlenW( szResource ) + 1)*sizeof(WCHAR);
        u_len = (strlenW( szUserPass ) + 1)*sizeof(WCHAR);
        r = WNetCachePassword( (CHAR*)szResource, r_len,
                               (CHAR*)szUserPass, u_len, dwMagic, 0 );

        return ( r == WN_SUCCESS );
    }

    sz = sizeof szUserPass;
    r_len = (strlenW( szResource ) + 1)*sizeof(WCHAR);
    r = WNetGetCachedPassword( (CHAR*)szResource, r_len,
                               (CHAR*)szUserPass, &sz, dwMagic );
    if( r != WN_SUCCESS )
        return FALSE;

    p = strchrW( szUserPass, ':' );
    if( p )
    {
        *p = 0;
        SetWindowTextW( hUserItem, szUserPass );
        SetWindowTextW( hPassItem, p+1 );
    }

    return TRUE;
}

/***********************************************************************
 *         WININET_SetProxyAuthorization
 */
static BOOL WININET_SetProxyAuthorization( HINTERNET hRequest,
                                         LPWSTR username, LPWSTR password )
{
    LPWININETHTTPREQW lpwhr;
    LPWININETHTTPSESSIONW lpwhs;
    LPWININETAPPINFOW hIC;
    LPWSTR p;

    lpwhr = (LPWININETHTTPREQW) WININET_GetObject( hRequest );
    if( !lpwhr )
	return FALSE;
        
    lpwhs = lpwhr->lpHttpSession;
    if (NULL == lpwhs ||  lpwhs->hdr.htype != WH_HHTTPSESSION)
    {
        INTERNET_SetLastError(ERROR_INTERNET_INCORRECT_HANDLE_TYPE);
	return FALSE;
    }

    hIC = lpwhs->lpAppInfo;

    p = HeapAlloc( GetProcessHeap(), 0, (strlenW( username ) + 1)*sizeof(WCHAR) );
    if( !p )
        return FALSE;
    
    lstrcpyW( p, username );
    hIC->lpszProxyUsername = p;

    p = HeapAlloc( GetProcessHeap(), 0, (strlenW( password ) + 1)*sizeof(WCHAR) );
    if( !p )
        return FALSE;
    
    lstrcpyW( p, password );
    hIC->lpszProxyPassword = p;

    return TRUE;
}

/***********************************************************************
 *         WININET_ProxyPasswordDialog
 */
static INT_PTR WINAPI WININET_ProxyPasswordDialog(
    HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HWND hitem;
    struct WININET_ErrorDlgParams *params;
    WCHAR szRealm[0x80], szServer[0x80];

    if( uMsg == WM_INITDIALOG )
    {
        TRACE("WM_INITDIALOG (%08lx)\n", lParam);

        /* save the parameter list */
        params = (struct WININET_ErrorDlgParams*) lParam;
        SetWindowLongPtrW( hdlg, GWLP_USERDATA, lParam );

        /* extract the Realm from the proxy response and show it */
        if( WININET_GetAuthRealm( params->hRequest,
                                  szRealm, sizeof szRealm/sizeof(WCHAR)) )
        {
            hitem = GetDlgItem( hdlg, IDC_REALM );
            SetWindowTextW( hitem, szRealm );
        }

        /* extract the name of the proxy server */
        if( WININET_GetProxyServer( params->hRequest, 
                                    szServer, sizeof szServer/sizeof(WCHAR)) )
        {
            hitem = GetDlgItem( hdlg, IDC_PROXY );
            SetWindowTextW( hitem, szServer );
        }

        WININET_GetSetPassword( hdlg, szServer, szRealm, FALSE );

        return TRUE;
    }

    params = (struct WININET_ErrorDlgParams*)
                 GetWindowLongPtrW( hdlg, GWLP_USERDATA );

    switch( uMsg )
    {
    case WM_COMMAND:
        if( wParam == IDOK )
        {
            WCHAR username[0x20], password[0x20];

            username[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_USERNAME );
            if( hitem )
                GetWindowTextW( hitem, username, sizeof username/sizeof(WCHAR) );
            
            password[0] = 0;
            hitem = GetDlgItem( hdlg, IDC_PASSWORD );
            if( hitem )
                GetWindowTextW( hitem, password, sizeof password/sizeof(WCHAR) );

            hitem = GetDlgItem( hdlg, IDC_SAVEPASSWORD );
            if( hitem &&
                SendMessageW( hitem, BM_GETSTATE, 0, 0 ) &&
                WININET_GetAuthRealm( params->hRequest,
                                  szRealm, sizeof szRealm/sizeof(WCHAR)) &&
                WININET_GetProxyServer( params->hRequest, 
                                    szServer, sizeof szServer/sizeof(WCHAR)) )
            {
                WININET_GetSetPassword( hdlg, szServer, szRealm, TRUE );
            }
            WININET_SetProxyAuthorization( params->hRequest, username, password );

            EndDialog( hdlg, ERROR_INTERNET_FORCE_RETRY );
            return TRUE;
        }
        if( wParam == IDCANCEL )
        {
            EndDialog( hdlg, 0 );
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/***********************************************************************
 *         WININET_GetConnectionStatus
 */
static INT WININET_GetConnectionStatus( HINTERNET hRequest )
{
    WCHAR szStatus[0x20];
    DWORD sz, index, dwStatus;

    TRACE("%p\n", hRequest );

    sz = sizeof szStatus;
    index = 0;
    if( !HttpQueryInfoW( hRequest, HTTP_QUERY_STATUS_CODE,
                    szStatus, &sz, &index))
        return -1;
    dwStatus = atoiW( szStatus );

    TRACE("request %p status = %d\n", hRequest, dwStatus );

    return dwStatus;
}


/***********************************************************************
 *         InternetErrorDlg
 */
DWORD WINAPI InternetErrorDlg(HWND hWnd, HINTERNET hRequest,
                 DWORD dwError, DWORD dwFlags, LPVOID* lppvData)
{
    struct WININET_ErrorDlgParams params;
    HMODULE hwininet = GetModuleHandleA( "wininet.dll" );
    INT dwStatus;

    TRACE("%p %p %d %08x %p\n", hWnd, hRequest, dwError, dwFlags, lppvData);

    params.hWnd = hWnd;
    params.hRequest = hRequest;
    params.dwError = dwError;
    params.dwFlags = dwFlags;
    params.lppvData = lppvData;

    switch( dwError )
    {
    case ERROR_SUCCESS:
        if( !(dwFlags & FLAGS_ERROR_UI_FILTER_FOR_ERRORS ) )
            return 0;
        dwStatus = WININET_GetConnectionStatus( hRequest );
        if( HTTP_STATUS_PROXY_AUTH_REQ != dwStatus )
            return ERROR_SUCCESS;
        return DialogBoxParamW( hwininet, MAKEINTRESOURCEW( IDD_PROXYDLG ),
                    hWnd, WININET_ProxyPasswordDialog, (LPARAM) &params );

    case ERROR_INTERNET_INCORRECT_PASSWORD:
        return DialogBoxParamW( hwininet, MAKEINTRESOURCEW( IDD_PROXYDLG ),
                    hWnd, WININET_ProxyPasswordDialog, (LPARAM) &params );

    case ERROR_INTERNET_HTTP_TO_HTTPS_ON_REDIR:
    case ERROR_INTERNET_INVALID_CA:
    case ERROR_INTERNET_POST_IS_NON_SECURE:
    case ERROR_INTERNET_SEC_CERT_CN_INVALID:
    case ERROR_INTERNET_SEC_CERT_DATE_INVALID:
        FIXME("Need to display dialog for error %d\n", dwError);
        return ERROR_SUCCESS;
    }
    return ERROR_INVALID_PARAMETER;
}
