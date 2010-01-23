/*
 * Implementation of System Event Notification Service Library (sensapi.dll)
 *
 * Copyright 2005 Steven Edwards for CodeWeavers
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
 *
 * Notes:
 * The System Event Notification Service reports the status of network
 * connections. For Wine we just report that we are always connected.
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "ole2.h"
#include "sensevts.h"
#include "sensapi.h"

#include "uuids.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sensapi);

static HMODULE SENSAPI_hModule;

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("%p,%x,%p\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
        {
            DisableThreadLibraryCalls(hinstDLL);
            SENSAPI_hModule = hinstDLL;
            break;
        }
        case DLL_PROCESS_DETACH:
        {
            break;
        }
    }

    return TRUE;
}

BOOL WINAPI IsDestinationReachableA(LPCSTR lpszDestination, LPQOCINFO lpQOCInfo)
{
    FIXME("%s,%p\n", lpszDestination, lpQOCInfo);
    return TRUE;
}
BOOL WINAPI IsDestinationReachableW(LPCWSTR lpszDestination, LPQOCINFO lpQOCInfo)
{
    FIXME("%s,%p\n", debugstr_w(lpszDestination), lpQOCInfo);
    return TRUE;
}

BOOL WINAPI IsNetworkAlive(LPDWORD lpdwFlags)
{
    TRACE("yes, using LAN type network.\n");
    if (lpdwFlags)
       *lpdwFlags = NETWORK_ALIVE_LAN;
    return TRUE;
}
