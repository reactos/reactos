/*
 * MPR WNet functions
 *
 * Copyright 1999 Ulrich Weigand
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winnetwk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpr);


/*
 * Browsing Functions
 */

/*********************************************************************
 * WNetOpenEnumA [MPR.@]
 */
DWORD WINAPI WNetOpenEnumA( DWORD dwScope, DWORD dwType, DWORD dwUsage,
                            LPNETRESOURCEA lpNet, LPHANDLE lphEnum )
{
    FIXME( "(%08lX, %08lX, %08lX, %p, %p): stub\n",
	    dwScope, dwType, dwUsage, lpNet, lphEnum );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetOpenEnumW [MPR.@]
 */
DWORD WINAPI WNetOpenEnumW( DWORD dwScope, DWORD dwType, DWORD dwUsage,
                            LPNETRESOURCEW lpNet, LPHANDLE lphEnum )
{
   FIXME( "(%08lX, %08lX, %08lX, %p, %p): stub\n",
          dwScope, dwType, dwUsage, lpNet, lphEnum );

   SetLastError(WN_NO_NETWORK);
   return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetEnumResourceA [MPR.@]
 */
DWORD WINAPI WNetEnumResourceA( HANDLE hEnum, LPDWORD lpcCount,
                                LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
	    hEnum, lpcCount, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetEnumResourceW [MPR.@]
 */
DWORD WINAPI WNetEnumResourceW( HANDLE hEnum, LPDWORD lpcCount,
                                LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
	    hEnum, lpcCount, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetCloseEnum [MPR.@]
 */
DWORD WINAPI WNetCloseEnum( HANDLE hEnum )
{
    FIXME( "(%p): stub\n", hEnum );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceInformationA [MPR.@]
 */
DWORD WINAPI WNetGetResourceInformationA( LPNETRESOURCEA lpNetResource,
                                          LPVOID lpBuffer, LPDWORD cbBuffer,
                                          LPSTR *lplpSystem )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceInformationW [MPR.@]
 */
DWORD WINAPI WNetGetResourceInformationW( LPNETRESOURCEW lpNetResource,
                                          LPVOID lpBuffer, LPDWORD cbBuffer,
                                          LPWSTR *lplpSystem )
{
    FIXME( "(%p, %p, %p, %p): stub\n",
           lpNetResource, lpBuffer, cbBuffer, lplpSystem );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceParentA [MPR.@]
 */
DWORD WINAPI WNetGetResourceParentA( LPNETRESOURCEA lpNetResource,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p): stub\n",
           lpNetResource, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetGetResourceParentW [MPR.@]
 */
DWORD WINAPI WNetGetResourceParentW( LPNETRESOURCEW lpNetResource,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%p, %p, %p): stub\n",
           lpNetResource, lpBuffer, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}



/*
 * Connection Functions
 */

/*********************************************************************
 *  WNetAddConnectionA [MPR.@]
 */
DWORD WINAPI WNetAddConnectionA( LPCSTR lpRemoteName, LPCSTR lpPassword,
                                 LPCSTR lpLocalName )
{
    FIXME( "(%s, %p, %s): stub\n",
           debugstr_a(lpRemoteName), lpPassword, debugstr_a(lpLocalName) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetAddConnectionW [MPR.@]
 */
DWORD WINAPI WNetAddConnectionW( LPCWSTR lpRemoteName, LPCWSTR lpPassword,
                                 LPCWSTR lpLocalName )
{
    FIXME( "(%s, %p, %s): stub\n",
           debugstr_w(lpRemoteName), lpPassword, debugstr_w(lpLocalName) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetAddConnection2A [MPR.@]
 */
DWORD WINAPI WNetAddConnection2A( LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %s, 0x%08lX): stub\n",
           lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection2W [MPR.@]
 */
DWORD WINAPI WNetAddConnection2W( LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %s, 0x%08lX): stub\n",
           lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection3A [MPR.@]
 */
DWORD WINAPI WNetAddConnection3A( HWND hwndOwner, LPNETRESOURCEA lpNetResource,
                                  LPCSTR lpPassword, LPCSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetAddConnection3W [MPR.@]
 */
DWORD WINAPI WNetAddConnection3W( HWND hwndOwner, LPNETRESOURCEW lpNetResource,
                                  LPCWSTR lpPassword, LPCWSTR lpUserID,
                                  DWORD dwFlags )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetUseConnectionA [MPR.@]
 */
DWORD WINAPI WNetUseConnectionA( HWND hwndOwner, LPNETRESOURCEA lpNetResource,
                                 LPCSTR lpPassword, LPCSTR lpUserID, DWORD dwFlags,
                                 LPSTR lpAccessName, LPDWORD lpBufferSize,
                                 LPDWORD lpResult )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX, %s, %p, %p), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_a(lpUserID), dwFlags,
           debugstr_a(lpAccessName), lpBufferSize, lpResult );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetUseConnectionW [MPR.@]
 */
DWORD WINAPI WNetUseConnectionW( HWND hwndOwner, LPNETRESOURCEW lpNetResource,
                                 LPCWSTR lpPassword, LPCWSTR lpUserID, DWORD dwFlags,
                                 LPWSTR lpAccessName, LPDWORD lpBufferSize,
                                 LPDWORD lpResult )
{
    FIXME( "(%p, %p, %p, %s, 0x%08lX, %s, %p, %p), stub\n",
           hwndOwner, lpNetResource, lpPassword, debugstr_w(lpUserID), dwFlags,
           debugstr_w(lpAccessName), lpBufferSize, lpResult );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 *  WNetCancelConnectionA [MPR.@]
 */
DWORD WINAPI WNetCancelConnectionA( LPCSTR lpName, BOOL fForce )
{
    FIXME( "(%s, %d), stub\n", debugstr_a(lpName), fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnectionW [MPR.@]
 */
DWORD WINAPI WNetCancelConnectionW( LPCWSTR lpName, BOOL fForce )
{
    FIXME( "(%s, %d), stub\n", debugstr_w(lpName), fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnection2A [MPR.@]
 */
DWORD WINAPI WNetCancelConnection2A( LPCSTR lpName, DWORD dwFlags, BOOL fForce )
{
    FIXME( "(%s, %08lX, %d), stub\n", debugstr_a(lpName), dwFlags, fForce );

    return WN_SUCCESS;
}

/*********************************************************************
 *  WNetCancelConnection2W [MPR.@]
 */
DWORD WINAPI WNetCancelConnection2W( LPCWSTR lpName, DWORD dwFlags, BOOL fForce )
{
    FIXME( "(%s, %08lX, %d), stub\n", debugstr_w(lpName), dwFlags, fForce );

    return WN_SUCCESS;
}

/*****************************************************************
 *  WNetRestoreConnectionA [MPR.@]
 */
DWORD WINAPI WNetRestoreConnectionA( HWND hwndOwner, LPSTR lpszDevice )
{
    FIXME( "(%p, %s), stub\n", hwndOwner, debugstr_a(lpszDevice) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetRestoreConnectionW [MPR.@]
 */
DWORD WINAPI WNetRestoreConnectionW( HWND hwndOwner, LPWSTR lpszDevice )
{
    FIXME( "(%p, %s), stub\n", hwndOwner, debugstr_w(lpszDevice) );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetGetConnectionA [MPR.@]
 *
 * RETURNS
 * - WN_BAD_LOCALNAME     lpLocalName makes no sense
 * - WN_NOT_CONNECTED     drive is a local drive
 * - WN_MORE_DATA         buffer isn't big enough
 * - WN_SUCCESS           success (net path in buffer)
 *
 */
DWORD WINAPI WNetGetConnectionA( LPCSTR lpLocalName,
                                 LPSTR lpRemoteName, LPDWORD lpBufferSize )
{
    char label[40];

    TRACE( "local %s\n", lpLocalName );
    if (lpLocalName[1] == ':')
    {
        switch(GetDriveTypeA(lpLocalName))
        {
        case DRIVE_REMOTE:
            if (!GetVolumeInformationA( lpLocalName, label, sizeof(label),
                                        NULL, NULL, NULL, NULL, 0 )) label[0] = 0;
            if (strlen(label) + 1 > *lpBufferSize)
            {
                *lpBufferSize = strlen(label) + 1;
                return WN_MORE_DATA;
            }
            strcpy( lpRemoteName, label );
            *lpBufferSize = strlen(lpRemoteName) + 1;
            return WN_SUCCESS;
	case DRIVE_REMOVABLE:
	case DRIVE_FIXED:
	case DRIVE_CDROM:
	  TRACE("file is local\n");
	  return WN_NOT_CONNECTED;
	default:
	    return WN_BAD_LOCALNAME;
        }
    }
    return WN_BAD_LOCALNAME;
}

/**************************************************************************
 * WNetGetConnectionW [MPR.@]
 */
DWORD WINAPI WNetGetConnectionW( LPCWSTR lpLocalName,
                                 LPWSTR lpRemoteName, LPDWORD lpBufferSize )
{
    CHAR  buf[200];
    DWORD ret, x = sizeof(buf);
    INT len = WideCharToMultiByte( CP_ACP, 0, lpLocalName, -1, NULL, 0, NULL, NULL );
    LPSTR lnA = HeapAlloc( GetProcessHeap(), 0, len );

    WideCharToMultiByte( CP_ACP, 0, lpLocalName, -1, lnA, len, NULL, NULL );
    ret = WNetGetConnectionA( lnA, buf, &x );
    HeapFree( GetProcessHeap(), 0, lnA );
    if (ret == WN_SUCCESS)
    {
        x = MultiByteToWideChar( CP_ACP, 0, buf, -1, NULL, 0 );
        if (x > *lpBufferSize)
        {
            *lpBufferSize = x;
            return WN_MORE_DATA;
        }
        *lpBufferSize = MultiByteToWideChar( CP_ACP, 0, buf, -1, lpRemoteName, *lpBufferSize );
    }
    return ret;
}

/**************************************************************************
 * WNetSetConnectionA [MPR.@]
 */
DWORD WINAPI WNetSetConnectionA( LPCSTR lpName, DWORD dwProperty,
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08lX, %p): stub\n", debugstr_a(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/**************************************************************************
 * WNetSetConnectionW [MPR.@]
 */
DWORD WINAPI WNetSetConnectionW( LPCWSTR lpName, DWORD dwProperty,
                                 LPVOID pvValue )
{
    FIXME( "(%s, %08lX, %p): stub\n", debugstr_w(lpName), dwProperty, pvValue );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUniversalNameA [MPR.@]
 */
DWORD WINAPI WNetGetUniversalNameA ( LPCSTR lpLocalPath, DWORD dwInfoLevel,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%s, 0x%08lX, %p, %p): stub\n",
           debugstr_a(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 * WNetGetUniversalNameW [MPR.@]
 */
DWORD WINAPI WNetGetUniversalNameW ( LPCWSTR lpLocalPath, DWORD dwInfoLevel,
                                     LPVOID lpBuffer, LPDWORD lpBufferSize )
{
    FIXME( "(%s, 0x%08lX, %p, %p): stub\n",
           debugstr_w(lpLocalPath), dwInfoLevel, lpBuffer, lpBufferSize);

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}



/*
 * Other Functions
 */

/**************************************************************************
 * WNetGetUserA [MPR.@]
 *
 * FIXME: we should not return ourselves, but the owner of the drive lpName
 */
DWORD WINAPI WNetGetUserA( LPCSTR lpName, LPSTR lpUserID, LPDWORD lpBufferSize )
{
    if (GetUserNameA( lpUserID, lpBufferSize )) return WN_SUCCESS;
    return GetLastError();
}

/*****************************************************************
 * WNetGetUserW [MPR.@]
 */
DWORD WINAPI WNetGetUserW( LPCWSTR lpName, LPWSTR lpUserID, LPDWORD lpBufferSize )
{
    FIXME( "(%s, %p, %p): mostly stub\n",
           debugstr_w(lpName), lpUserID, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*********************************************************************
 * WNetConnectionDialog [MPR.@]
 */
DWORD WINAPI WNetConnectionDialog( HWND hwnd, DWORD dwType )
{
    FIXME( "(%p, %08lX): stub\n", hwnd, dwType );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetConnectionDialog1A [MPR.@]
 */
DWORD WINAPI WNetConnectionDialog1A( LPCONNECTDLGSTRUCTA lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetConnectionDialog1W [MPR.@]
 */
DWORD WINAPI WNetConnectionDialog1W( LPCONNECTDLGSTRUCTW lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog [MPR.@]
 */
DWORD WINAPI WNetDisconnectDialog( HWND hwnd, DWORD dwType )
{
    FIXME( "(%p, %08lX): stub\n", hwnd, dwType );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog1A [MPR.@]
 */
DWORD WINAPI WNetDisconnectDialog1A( LPDISCDLGSTRUCTA lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetDisconnectDialog1W [MPR.@]
 */
DWORD WINAPI WNetDisconnectDialog1W( LPDISCDLGSTRUCTW lpConnDlgStruct )
{
    FIXME( "(%p): stub\n", lpConnDlgStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetLastErrorA [MPR.@]
 */
DWORD WINAPI WNetGetLastErrorA( LPDWORD lpError,
                                LPSTR lpErrorBuf, DWORD nErrorBufSize,
                                LPSTR lpNameBuf, DWORD nNameBufSize )
{
    FIXME( "(%p, %p, %ld, %p, %ld): stub\n",
           lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetLastErrorW [MPR.@]
 */
DWORD WINAPI WNetGetLastErrorW( LPDWORD lpError,
                                LPWSTR lpErrorBuf, DWORD nErrorBufSize,
                         LPWSTR lpNameBuf, DWORD nNameBufSize )
{
    FIXME( "(%p, %p, %ld, %p, %ld): stub\n",
           lpError, lpErrorBuf, nErrorBufSize, lpNameBuf, nNameBufSize );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetNetworkInformationA [MPR.@]
 */
DWORD WINAPI WNetGetNetworkInformationA( LPCSTR lpProvider,
                                         LPNETINFOSTRUCT lpNetInfoStruct )
{
    FIXME( "(%s, %p): stub\n", debugstr_a(lpProvider), lpNetInfoStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*********************************************************************
 * WNetGetNetworkInformationW [MPR.@]
 */
DWORD WINAPI WNetGetNetworkInformationW( LPCWSTR lpProvider,
                                         LPNETINFOSTRUCT lpNetInfoStruct )
{
    FIXME( "(%s, %p): stub\n", debugstr_w(lpProvider), lpNetInfoStruct );

    SetLastError(WN_NO_NETWORK);
    return ERROR_NO_NETWORK;
}

/*****************************************************************
 *  WNetGetProviderNameA [MPR.@]
 */
DWORD WINAPI WNetGetProviderNameA( DWORD dwNetType,
                                   LPSTR lpProvider, LPDWORD lpBufferSize )
{
    FIXME( "(%ld, %p, %p): stub\n", dwNetType, lpProvider, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetGetProviderNameW [MPR.@]
 */
DWORD WINAPI WNetGetProviderNameW( DWORD dwNetType,
                                   LPWSTR lpProvider, LPDWORD lpBufferSize )
{
    FIXME( "(%ld, %p, %p): stub\n", dwNetType, lpProvider, lpBufferSize );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}
