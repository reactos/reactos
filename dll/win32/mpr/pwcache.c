/*
 * MPR Password Cache functions
 *
 * Copyright 1999 Ulrich Weigand
 * Copyright 2003,2004 Mike McCormack for CodeWeavers
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
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winnetwk.h"
#include "winreg.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mpr);

static const char mpr_key[] = "Software\\Wine\\Wine\\Mpr\\";

static inline BYTE hex( BYTE x )
{
    if( x <= 9 )
        return x + '0';
    return x + 'A' - 10;
}

static inline signed char ctox( CHAR x )
{
    if( ( x >= '0' ) && ( x <= '9' ) )
        return x - '0';
    if( ( x >= 'A' ) && ( x <= 'F' ) )
        return x - 'A' + 10;
    if( ( x >= 'a' ) && ( x <= 'f' ) )
        return x - 'a' + 10;
    return -1;
}

static LPSTR MPR_GetValueName( LPCSTR pbResource, WORD cbResource, BYTE nType )
{
    LPSTR name;
    DWORD  i;

    name = HeapAlloc( GetProcessHeap(), 0, 6+cbResource*2 );
    if( !name ) return NULL;

    sprintf( name, "X-%02X-", nType );
    for(i=0; i<cbResource; i++)
    {
        name[5+i*2]=hex((pbResource[i]&0xf0)>>4);
        name[6+i*2]=hex(pbResource[i]&0x0f);
    }
    name[5+i*2]=0;
    TRACE( "Value is %s\n", name );
    return name;
}


/**************************************************************************
 * WNetCachePassword [MPR.@]  Saves password in cache
 *
 * NOTES
 *	Only the parameter count is verified
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

    /* @@ Wine registry key: HKCU\Software\Wine\Wine\Mpr */
    r = RegCreateKeyA( HKEY_CURRENT_USER, mpr_key, &hkey );
    if( r )
        return WN_ACCESS_DENIED;

    valname = MPR_GetValueName( pbResource, cbResource, nType );
    if( valname )
    {
        r = RegSetValueExA( hkey, valname, 0, REG_BINARY, 
                            (LPBYTE)pbPassword, cbPassword );
        if( r )
            r = WN_CANCEL;
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
UINT WINAPI WNetRemoveCachedPassword(
      LPSTR pbResource,   /* [in] resource ID to delete */
      WORD cbResource,    /* [in] number of bytes in the resource ID */
      BYTE nType )        /* [in] Type of the resource to delete */
{
    HKEY hkey;
    DWORD r;
    LPSTR valname;

    WARN( "(%p(%s), %d, %d): totally insecure\n",
           pbResource, debugstr_a(pbResource), cbResource, nType );

    /* @@ Wine registry key: HKCU\Software\Wine\Wine\Mpr */
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

    WARN( "(%p(%s), %d, %p, %p, %d): totally insecure\n",
           pbResource, debugstr_a(pbResource), cbResource,
	   pbPassword, pcbPassword, nType );

    memset( pbPassword, 0, *pcbPassword);

    /* @@ Wine registry key: HKCU\Software\Wine\Wine\Mpr */
    r = RegCreateKeyA( HKEY_CURRENT_USER, mpr_key, &hkey );
    if( r )
        return WN_ACCESS_DENIED;

    valname = MPR_GetValueName( pbResource, cbResource, nType );
    if( valname )
    {
        sz = *pcbPassword;
        r = RegQueryValueExA( hkey, valname, 0, &type, (LPBYTE)pbPassword, &sz );
        *pcbPassword = sz;
        if( r )
            r = WN_CANCEL;
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
 *	The parameter count is verified
 * 
 *  This function is a huge security risk, as virii and such can use
 * it to grab all the passwords in the cache.  It's bad enough to 
 * store the passwords (insecurely).
 *
 *  bpPrefix and cbPrefix are used to filter the returned passwords
 *   the first cbPrefix bytes of the password resource identifier
 *   should match the same number of bytes in bpPrefix
 *
 * RETURNS
 *   Success: WN_SUCCESS   (even if no entries were enumerated)
 *   Failure: WN_ACCESS_DENIED, WN_BAD_PASSWORD, WN_BAD_VALUE,
 *             WN_NET_ERROR, WN_NOT_SUPPORTED, WN_OUT_OF_MEMORY
 */

UINT WINAPI WNetEnumCachedPasswords(
      LPSTR pbPrefix,  /* [in] prefix to filter cache entries */
      WORD cbPrefix,   /* [in] number of bytes in Prefix substring */
      BYTE nType,      /* [in] match the Type ID of the entry */
      ENUMPASSWORDPROC enumPasswordProc,  /* [in] callback function */
      DWORD param)     /* [in] parameter passed to enum function */
{
    HKEY hkey;
    DWORD r, type, val_sz, data_sz, i, j, size;
    PASSWORD_CACHE_ENTRY *entry;
    CHAR val[256], prefix[6];

    WARN( "(%s, %d, %d, %p, 0x%08x) totally insecure\n",
           debugstr_an(pbPrefix,cbPrefix), cbPrefix,
	   nType, enumPasswordProc, param );

    /* @@ Wine registry key: HKCU\Software\Wine\Wine\Mpr */
    r = RegCreateKeyA( HKEY_CURRENT_USER, mpr_key, &hkey );
    if( r )
        return WN_ACCESS_DENIED;

    sprintf(prefix, "X-%02X-", nType );

    for( i=0;  ; i++ )
    {
        val_sz  = sizeof val;
        data_sz = 0;
        type    = 0;
        val[0] = 0;
        r = RegEnumValueA( hkey, i, val, &val_sz, NULL, &type, NULL, &data_sz );
        if( r != ERROR_SUCCESS )
            break;
        if( type != REG_BINARY )
            continue;

        /* check the value is in the format we expect */
        if( val_sz < sizeof prefix )
            continue;
        if( memcmp( prefix, val, 5 ) )
            continue;

        /* decode the value */
        for(j=5; j<val_sz; j+=2 )
        {
            signed char hi = ctox( val[j] ), lo = ctox( val[j+1] );
            if( ( hi < 0 ) || ( lo < 0 ) )
                break;
            val[(j-5)/2] = (hi<<4) | lo;
        }

        /* find the decoded length */
        val_sz = (j - 5)/2;
        val[val_sz]=0;
        if( val_sz < cbPrefix )
            continue;

        /* check the prefix matches */
        if( memcmp(val, pbPrefix, cbPrefix) )
            continue;

        /* read the value data */
        size = offsetof( PASSWORD_CACHE_ENTRY, abResource[val_sz + data_sz] );
        entry = HeapAlloc( GetProcessHeap(), 0, size );
        memcpy( entry->abResource, val, val_sz );
        entry->cbEntry = size;
        entry->cbResource = val_sz;
        entry->cbPassword = data_sz;
        entry->iEntry = i;
        entry->nType = nType;
        size = sizeof val;
        r = RegEnumValueA( hkey, i, val, &size, NULL, &type,
                           &entry->abResource[val_sz], &data_sz );
        if( r == ERROR_SUCCESS )
            enumPasswordProc( entry, param );
        HeapFree( GetProcessHeap(), 0, entry );
    }

    RegCloseKey( hkey );

    return WN_SUCCESS;
}
