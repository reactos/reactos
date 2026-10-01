/*
 * TAPI32 Assisted Telephony
 *
 * Copyright 1999  Andreas Mohr
 * Copyright 2011  André Hentschel
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
#include "wingdi.h"
#include "winreg.h"
#include "objbase.h"
#include "tapi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(tapi);

/***********************************************************************
 *      tapiGetLocationInfoW (TAPI32.@)
 */
DWORD WINAPI tapiGetLocationInfoW(LPWSTR countrycode, LPWSTR citycode)
{
    HKEY hkey, hsubkey;
    DWORD currid;
    DWORD valsize;
    DWORD type;
    DWORD bufsize;
    BYTE buf[200];
    WCHAR szlockey[20];

    static const WCHAR currentidW[] = {'C','u','r','r','e','n','t','I','D',0};
    static const WCHAR locationW[]  = {'L','o','c','a','t','i','o','n','%','u',0};
    static const WCHAR areacodeW[]  = {'A','r','e','a','C','o','d','e',0};
    static const WCHAR countryW[]   = {'C','o','u','n','t','r','y',0};
    static const WCHAR fmtW[]       = {'%','u',0};

    static const WCHAR locations_keyW[] =
        {'S','o','f','t','w','a','r','e','\\','M','i','c','r','o','s','o','f','t','\\',
         'W','i','n','d','o','w','s','\\',
         'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
         'T','e','l','e','p','h','o','n','y','\\','L','o','c','a','t','i','o','n','s',0};

    if(RegOpenKeyW(HKEY_LOCAL_MACHINE, locations_keyW, &hkey) == ERROR_SUCCESS) {
        valsize = sizeof( DWORD);
        if(!RegQueryValueExW(hkey, currentidW, 0, &type, (LPBYTE) &currid, &valsize) &&
           type == REG_DWORD) {
            /* find a subkey called Location1, Location2... */
            swprintf( szlockey, ARRAY_SIZE(szlockey), locationW, currid);
            if( !RegOpenKeyW( hkey, szlockey, &hsubkey)) {
                if( citycode) {
                    bufsize=sizeof(buf);
                    if( !RegQueryValueExW( hsubkey, areacodeW, 0, &type, buf, &bufsize) &&
                        type == REG_SZ) {
                        lstrcpynW( citycode, (WCHAR *) buf, 8);
                    } else 
                        citycode[0] = '\0';
                }
                if( countrycode) {
                    bufsize=sizeof(buf);
                    if( !RegQueryValueExW( hsubkey, countryW, 0, &type, buf, &bufsize) &&
                        type == REG_DWORD)
                        swprintf( countrycode, 8, fmtW, *(LPDWORD) buf );
                    else
                        countrycode[0] = '\0';
                }
                TRACE("(%p \"%s\", %p \"%s\"): success.\n", countrycode, debugstr_w(countrycode),
                      citycode, debugstr_w(citycode));
                RegCloseKey( hkey);
                RegCloseKey( hsubkey);
                return 0; /* SUCCESS */
            }
        }
        RegCloseKey( hkey);
    }
    WARN("(%p, %p): failed (no telephony registry entries?).\n", countrycode, citycode);
    return TAPIERR_REQUESTFAILED;
}


/***********************************************************************
 *      tapiGetLocationInfoA (TAPI32.@)
 */
DWORD WINAPI tapiGetLocationInfoA(LPSTR countrycode, LPSTR citycode)
{
    DWORD ret, len;
    LPWSTR country, city;

    len = MultiByteToWideChar( CP_ACP, 0, countrycode, -1, NULL, 0 );
    country = HeapAlloc( GetProcessHeap(), 0, len );
    MultiByteToWideChar( CP_ACP, 0, countrycode, -1, country, len );

    len = MultiByteToWideChar( CP_ACP, 0, citycode, -1, NULL, 0 );
    city = HeapAlloc( GetProcessHeap(), 0, len );
    MultiByteToWideChar( CP_ACP, 0, citycode, -1, city, len );

    ret = tapiGetLocationInfoW(country, city);

    HeapFree( GetProcessHeap(), 0, city );
    HeapFree( GetProcessHeap(), 0, country );
    return ret;
}

/***********************************************************************
 *		tapiRequestMakeCall (TAPI32.@)
 */
DWORD WINAPI tapiRequestMakeCallA(LPCSTR lpszDestAddress, LPCSTR lpszAppName,
                                 LPCSTR lpszCalledParty, LPCSTR lpszComment)
{
    FIXME("(%s, %s, %s, %s): stub.\n", lpszDestAddress, lpszAppName, lpszCalledParty, lpszComment);
    return 0;
}
