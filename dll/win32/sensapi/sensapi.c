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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "sensevts.h"
#include "sensapi.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(sensapi);

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
