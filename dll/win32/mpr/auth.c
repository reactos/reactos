/*
 * MPR Authentication and Logon functions
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnetwk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpr);


/*****************************************************************
 *  WNetLogoffA [MPR.@]
 */
DWORD WINAPI WNetLogoffA( LPCSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %p): stub\n", debugstr_a(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogoffW [MPR.@]
 */
DWORD WINAPI WNetLogoffW( LPCWSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %p): stub\n", debugstr_w(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogonA [MPR.@]
 */
DWORD WINAPI WNetLogonA( LPCSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %p): stub\n", debugstr_a(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetLogonW [MPR.@]
 */
DWORD WINAPI WNetLogonW( LPCWSTR lpProvider, HWND hwndOwner )
{
    FIXME( "(%s, %p): stub\n", debugstr_w(lpProvider), hwndOwner );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetVerifyPasswordA [MPR.@]
 */
DWORD WINAPI WNetVerifyPasswordA( LPCSTR lpszPassword, BOOL *pfMatch )
{
    FIXME( "(%p, %p): stub\n", lpszPassword, pfMatch );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}

/*****************************************************************
 *  WNetVerifyPasswordW [MPR.@]
 */
DWORD WINAPI WNetVerifyPasswordW( LPCWSTR lpszPassword, BOOL *pfMatch )
{
    FIXME( "(%p, %p): stub\n", lpszPassword, pfMatch );

    SetLastError(WN_NO_NETWORK);
    return WN_NO_NETWORK;
}
