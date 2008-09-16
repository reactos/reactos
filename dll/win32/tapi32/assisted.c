/*
 * TAPI32 Assisted Telephony
 *
 * Copyright 1999  Andreas Mohr
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
 *		tapiGetLocationInfo (TAPI32.@)
 */
DWORD WINAPI tapiGetLocationInfoA(LPSTR lpszCountryCode, LPSTR lpszCityCode)
{
    HKEY hkey, hsubkey;
    DWORD currid;
    DWORD valsize;
    DWORD type;
    DWORD bufsize;
    BYTE buf[100];
    char szlockey[20];
    if(!RegOpenKeyA(HKEY_LOCAL_MACHINE,
           "Software\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Locations",
           &hkey) != ERROR_SUCCESS) { 
        valsize = sizeof( DWORD);
        if(!RegQueryValueExA(hkey, "CurrentID", 0, &type, (LPBYTE) &currid,
                    &valsize) && type == REG_DWORD) {
            /* find a subkey called Location1, Location2... */
            sprintf( szlockey, "Location%u", currid); 
            if( !RegOpenKeyA( hkey, szlockey, &hsubkey)) {
                if( lpszCityCode) {
                    bufsize=sizeof(buf);
                    if( !RegQueryValueExA( hsubkey, "AreaCode", 0, &type, buf,
                                &bufsize) && type == REG_SZ) {
			lstrcpynA( lpszCityCode, (char *) buf, 8);
                    } else 
                        lpszCityCode[0] = '\0';
                }
                if( lpszCountryCode) {
                    bufsize=sizeof(buf);
                    if( !RegQueryValueExA( hsubkey, "Country", 0, &type, buf,
                                &bufsize) && type == REG_DWORD)
                        snprintf( lpszCountryCode, 8, "%u", *(LPDWORD) buf );
                    else
                        lpszCountryCode[0] = '\0';
                }
                TRACE("(%p \"%s\", %p \"%s\"): success.\n",
                        lpszCountryCode, debugstr_a(lpszCountryCode),
                        lpszCityCode, debugstr_a(lpszCityCode));
                RegCloseKey( hkey);
                RegCloseKey( hsubkey);
                return 0; /* SUCCESS */
            }
        }
        RegCloseKey( hkey);
    }
    WARN("(%p, %p): failed (no telephony registry entries?).\n",
            lpszCountryCode, lpszCityCode);
    return TAPIERR_REQUESTFAILED;
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
