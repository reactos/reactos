/*
 * MPR Password Cache functions
 *
 * Copyright 1999 Ulrich Weigand
 * Copyright 2003 Mike McCormack for CodeWeavers
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

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnetwk.h"
#include "winreg.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpr);

static const char mpr_key[] = "Software\\Wine\\Wine\\Mpr\\";

static LPSTR MPR_GetValueName( LPSTR pbResource, WORD cbResource, BYTE nType )
{
    LPSTR name;
    DWORD  i, x = 0;

    /* just a hash so the value name doesn't get too large */
    for( i=0; i<cbResource; i++ )
        x = ((x<<7) | (x >> 25)) ^ toupper(pbResource[i]);

    name = HeapAlloc( GetProcessHeap(), 0, 0x10 );
    if( name )
        sprintf( name, "I-%08lX-%02X", x, nType );
    TRACE( "Value is %s\n", name );
    return name;
}

/**************************************************************************
 * WNetCachePassword [MPR.@]  Saves password in cache
 *
 * NOTES
 *	only the parameter count is verifyed
 *
 *	---- everything below this line might be wrong (js) -----
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WN_ACCESS_DENIED, WN_BAD_PASSWORD, WN_BADVALUE, WN_NET_ERROR,
 *             WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetCachePassword(
    LPSTR pbResource, /* [in] Name of workgroup, computer, or resource */
    WORD cbResource,  /* [in] Size of name */
    LPSTR pbPassword, /* [in] Buffer containing password */
    WORD cbPassword,  /* [in] Size of password */
    BYTE nType,       /* [in] Type of password to cache */
    WORD x)

{
    HKEY hkey;
    DWORD r;
    LPSTR valname;

    WARN( "(%p(%s), %d, %p(%s), %d, %d, 0x%08x): totally insecure\n",
           pbResource, debugstr_a(pbResource), cbResource,
	   pbPassword, debugstr_a(pbPassword), cbPassword,
	   nType, x );

    r = RegCreateKeyA( HKEY_CURRENT_USER, mpr_key, &hkey );
    if( r )
        return WN_ACCESS_DENIED;

    valname = MPR_GetValueName( pbResource, cbResource, nType );
    if( valname )
    {
        r = RegSetValueExA( hkey, valname, 0, REG_BINARY, 
                            pbPassword, cbPassword );
        if( r )
            r = WN_ACCESS_DENIED;
        else
            r = WN_SUCCESS;
        HeapFree( GetProcessHeap(), 0, valname );
    }
    else
        r = WN_OUT_OF_MEMORY;

    RegCloseKey( hkey );

    return r;
}

/*****************************************************************
 *  WNetRemoveCachedPassword [MPR.@]
 */
UINT WINAPI WNetRemoveCachedPassword( LPSTR pbResource, WORD cbResource,
                                      BYTE nType )
{
    HKEY hkey;
    DWORD r;
    LPSTR valname;

    WARN( "(%p(%s), %d, %d): totally insecure\n",
           pbResource, debugstr_a(pbResource), cbResource, nType );

    r = RegCreateKeyA( HKEY_CURRENT_USER, mpr_key, &hkey );
    if( r )
        return WN_ACCESS_DENIED;

    valname = MPR_GetValueName( pbResource, cbResource, nType );
    if( valname )
    {
        r = RegDeleteValueA( hkey, valname );
        if( r )
            r = WN_ACCESS_DENIED;
        else
            r = WN_SUCCESS;
        HeapFree( GetProcessHeap(), 0, valname );
    }
    else
        r = WN_OUT_OF_MEMORY;

    return r;
}

/*****************************************************************
 * WNetGetCachedPassword [MPR.@]  Retrieves password from cache
 *
 * NOTES
 *  the stub seems to be wrong:
 *	arg1:	ptr	0x40xxxxxx -> (no string)
 *	arg2:	len	36
 *	arg3:	ptr	0x40xxxxxx -> (no string)
 *	arg4:	ptr	0x40xxxxxx -> 0xc8
 *	arg5:	type?	4
 *
 *	---- everything below this line might be wrong (js) -----
 * RETURNS
 *    Success: WN_SUCCESS
 *    Failure: WN_ACCESS_DENIED, WN_BAD_PASSWORD, WN_BAD_VALUE,
 *             WN_NET_ERROR, WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */
DWORD WINAPI WNetGetCachedPassword(
    LPSTR pbResource,   /* [in]  Name of workgroup, computer, or resource */
    WORD cbResource,    /* [in]  Size of name */
    LPSTR pbPassword,   /* [out] Buffer to receive password */
    LPWORD pcbPassword, /* [out] Receives size of password */
    BYTE nType)         /* [in]  Type of password to retrieve */
{
    HKEY hkey;
    DWORD r, type = 0, sz;
    LPSTR valname;

    WARN( "(%p(%s), %d, %p, %p, %d): stub\n",
           pbResource, debugstr_a(pbResource), cbResource,
	   pbPassword, pcbPassword, nType );

    r = RegCreateKeyA( HKEY_CURRENT_USER, mpr_key, &hkey );
    if( r )
        return WN_ACCESS_DENIED;

    valname = MPR_GetValueName( pbResource, cbResource, nType );
    if( valname )
    {
        sz = *pcbPassword;
        r = RegQueryValueExA( hkey, valname, 0, &type, pbPassword, &sz );
        *pcbPassword = sz;
        if( r )
            r = WN_ACCESS_DENIED;
        else
            r = WN_SUCCESS;
        HeapFree( GetProcessHeap(), 0, valname );
    }
    else
        r = WN_OUT_OF_MEMORY;

    return r;
}

/*******************************************************************
 * WNetEnumCachedPasswords [MPR.@]
 *
 * NOTES
 *	the parameter count is verifyed
 * 
 *  This function is a huge security risk, as virii and such can use
 * it to grab all the passwords in the cache.  It's bad enough to 
 * store the passwords (insecurely).
 *
 *  observed values:
 *	arg1	ptr	0x40xxxxxx -> (no string)
 *	arg2	int	32
 *	arg3	type?	4
 *	arg4	enumPasswordProc (verifyed)
 *	arg5	ptr	0x40xxxxxx -> 0x0
 *
 *	---- everything below this line might be wrong (js) -----
 */

UINT WINAPI WNetEnumCachedPasswords( LPSTR pbPrefix, WORD cbPrefix,
                                     BYTE nType, ENUMPASSWORDPROC enumPasswordProc, DWORD x)
{
    WARN( "(%p(%s), %d, %d, %p, 0x%08lx): don't implement this\n",
           pbPrefix, debugstr_a(pbPrefix), cbPrefix,
	   nType, enumPasswordProc, x );

    return WN_NOT_SUPPORTED;
}
