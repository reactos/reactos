/*
 * MPR Network Provider Services functions
 *
 * Copyright 1999 Ulrich Weigand
 * Copyright 2004 Mike McCormack for CodeWeavers Inc.
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "netspi.h"
#include "wine/debug.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpr);

#include "wine/unicode.h"

#include "mprres.h"

/***********************************************************************
 *         NPS_ProxyPasswordDialog
 */
static INT_PTR WINAPI NPS_ProxyPasswordDialog(
    HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    HWND hitem;
    LPAUTHDLGSTRUCTA lpAuthDlgStruct;

    if( uMsg == WM_INITDIALOG )
    {
        TRACE("WM_INITDIALOG (%08lx)\n", lParam);

        /* save the parameter list */
        lpAuthDlgStruct = (LPAUTHDLGSTRUCTA) lParam;
        SetWindowLongPtrW( hdlg, GWLP_USERDATA, lParam );

        if( lpAuthDlgStruct->lpExplainText )
        {
            hitem = GetDlgItem( hdlg, IDC_EXPLAIN );
            SetWindowTextA( hitem, lpAuthDlgStruct->lpExplainText );
        }

        /* extract the Realm from the proxy response and show it */
        if( lpAuthDlgStruct->lpResource )
        {
            hitem = GetDlgItem( hdlg, IDC_REALM );
            SetWindowTextA( hitem, lpAuthDlgStruct->lpResource );
        }

        return TRUE;
    }

    lpAuthDlgStruct = (LPAUTHDLGSTRUCTA) GetWindowLongPtrW( hdlg, GWLP_USERDATA );

    switch( uMsg )
    {
    case WM_COMMAND:
        if( wParam == IDOK )
        {
            hitem = GetDlgItem( hdlg, IDC_USERNAME );
            if( hitem )
                GetWindowTextA( hitem, lpAuthDlgStruct->lpUsername, lpAuthDlgStruct->cbUsername );

            hitem = GetDlgItem( hdlg, IDC_PASSWORD );
            if( hitem )
                GetWindowTextA( hitem, lpAuthDlgStruct->lpPassword, lpAuthDlgStruct->cbPassword );

            EndDialog( hdlg, WN_SUCCESS );
            return TRUE;
        }
        if( wParam == IDCANCEL )
        {
            EndDialog( hdlg, WN_CANCEL );
            return TRUE;
        }
        break;
    }
    return FALSE;
}

/*****************************************************************
 *  NPSAuthenticationDialogA [MPR.@]
 */
DWORD WINAPI NPSAuthenticationDialogA( LPAUTHDLGSTRUCTA lpAuthDlgStruct )
{
    HMODULE hwininet = GetModuleHandleA( "mpr.dll" );

    TRACE("%p\n", lpAuthDlgStruct);

    if( !lpAuthDlgStruct )
        return WN_BAD_POINTER;
    if( lpAuthDlgStruct->cbStructure < sizeof *lpAuthDlgStruct )
        return WN_BAD_POINTER;

    TRACE("%s %s %s\n",lpAuthDlgStruct->lpResource,
          lpAuthDlgStruct->lpOUTitle, lpAuthDlgStruct->lpExplainText);

    return DialogBoxParamW( hwininet, MAKEINTRESOURCEW( IDD_PROXYDLG ),
             lpAuthDlgStruct->hwndOwner, NPS_ProxyPasswordDialog, 
             (LPARAM) lpAuthDlgStruct );
}

/*****************************************************************
 *  NPSGetProviderHandleA [MPR.@]
 */
DWORD WINAPI NPSGetProviderHandleA( PHPROVIDER phProvider )
{
    FIXME( "(%p): stub\n", phProvider );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetProviderNameA [MPR.@]
 */
DWORD WINAPI NPSGetProviderNameA( HPROVIDER hProvider, LPCSTR *lpszProviderName )
{
    FIXME( "(%p, %p): stub\n", hProvider, lpszProviderName );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSGetSectionNameA [MPR.@]
 */
DWORD WINAPI NPSGetSectionNameA( HPROVIDER hProvider, LPCSTR *lpszSectionName )
{
    FIXME( "(%p, %p): stub\n", hProvider, lpszSectionName );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSSetExtendedErrorA [MPR.@]
 */
DWORD WINAPI NPSSetExtendedErrorA( DWORD NetSpecificError, LPSTR lpExtendedErrorText )
{
    FIXME( "(%08x, %s): stub\n", NetSpecificError, debugstr_a(lpExtendedErrorText) );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSSetCustomTextA [MPR.@]
 */
VOID WINAPI NPSSetCustomTextA( LPSTR lpCustomErrorText )
{
    FIXME( "(%s): stub\n", debugstr_a(lpCustomErrorText) );
}

/*****************************************************************
 *  NPSCopyStringA [MPR.@]
 */
DWORD WINAPI NPSCopyStringA( LPCSTR lpString, LPVOID lpBuffer, LPDWORD lpdwBufferSize )
{
    FIXME( "(%s, %p, %p): stub\n", debugstr_a(lpString), lpBuffer, lpdwBufferSize );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSDeviceGetNumberA [MPR.@]
 */
DWORD WINAPI NPSDeviceGetNumberA( LPSTR lpLocalName, LPDWORD lpdwNumber, LPDWORD lpdwType )
{
    FIXME( "(%s, %p, %p): stub\n", debugstr_a(lpLocalName), lpdwNumber, lpdwType );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSDeviceGetStringA [MPR.@]
 */
DWORD WINAPI NPSDeviceGetStringA( DWORD dwNumber, DWORD dwType, LPSTR lpLocalName, LPDWORD lpdwBufferSize )
{
    FIXME( "(%d, %d, %p, %p): stub\n", dwNumber, dwType, lpLocalName, lpdwBufferSize );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSNotifyRegisterA [MPR.@]
 */
DWORD WINAPI NPSNotifyRegisterA( enum NOTIFYTYPE NotifyType, NOTIFYCALLBACK pfNotifyCallBack )
{
    FIXME( "(%d, %p): stub\n", NotifyType, pfNotifyCallBack );
    return WN_NOT_SUPPORTED;
}

/*****************************************************************
 *  NPSNotifyGetContextA [MPR.@]
 */
LPVOID WINAPI NPSNotifyGetContextA( NOTIFYCALLBACK pfNotifyCallBack )
{
    FIXME( "(%p): stub\n", pfNotifyCallBack );
    return NULL;
}

/*****************************************************************
 *  PwdGetPasswordStatusA [MPR.@]
 */
DWORD WINAPI PwdGetPasswordStatusA( LPCSTR lpProvider, DWORD dwIndex, LPDWORD status )
{
    FIXME("%s %d %p\n", debugstr_a(lpProvider), dwIndex, status );
    *status = 0;
    return WN_SUCCESS;
}

/*****************************************************************
 *  PwdGetPasswordStatusA [MPR.@]
 */
DWORD WINAPI PwdGetPasswordStatusW( LPCWSTR lpProvider, DWORD dwIndex, LPDWORD status )
{
    FIXME("%s %d %p\n", debugstr_w(lpProvider), dwIndex, status );
    *status = 0;
    return WN_SUCCESS;
}

/*****************************************************************
 *  PwdSetPasswordStatusA [MPR.@]
 */
DWORD WINAPI PwdSetPasswordStatusA( LPCSTR lpProvider, DWORD dwIndex, DWORD status )
{
    FIXME("%s %d %d\n", debugstr_a(lpProvider), dwIndex, status );
    return WN_SUCCESS;
}

/*****************************************************************
 *  PwdSetPasswordStatusW [MPR.@]
 */
DWORD WINAPI PwdSetPasswordStatusW( LPCWSTR lpProvider, DWORD dwIndex, DWORD status )
{
    FIXME("%s %d %d\n", debugstr_w(lpProvider), dwIndex, status );
    return WN_SUCCESS;
}

typedef struct _CHANGEPWDINFOA {
    LPSTR lpUsername;
    LPSTR lpPassword;
    DWORD cbPassword;
} CHANGEPWDINFOA, *LPCHANGEPWDINFOA;

typedef struct _CHANGEPWDINFOW {
    LPWSTR lpUsername;
    LPWSTR lpPassword;
    DWORD cbPassword;
} CHANGEPWDINFOW, *LPCHANGEPWDINFOW;

/*****************************************************************
 *  PwdChangePasswordA [MPR.@]
 */
DWORD WINAPI PwdChangePasswordA( LPCSTR lpProvider, HWND hWnd, DWORD flags, LPCHANGEPWDINFOA info )
{
    FIXME("%s %p %x %p\n", debugstr_a(lpProvider), hWnd, flags, info );
    return WN_SUCCESS;
}

/*****************************************************************
 *  PwdChangePasswordA [MPR.@]
 */
DWORD WINAPI PwdChangePasswordW( LPCWSTR lpProvider, HWND hWnd, DWORD flags, LPCHANGEPWDINFOW info )
{
    FIXME("%s %p %x %p\n", debugstr_w(lpProvider), hWnd, flags, info );
    return WN_SUCCESS;
}
