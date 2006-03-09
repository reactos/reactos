/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * Net API buffer calls
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
#include "wine/port.h"

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "lmcons.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "winerror.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                NetApiBufferFree  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetApiBufferFree(LPVOID Buffer)
{
    FIXME("(%p)\n", Buffer);
    return NERR_Success;
}

/************************************************************
 *                NetUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI
NetUserGetInfo(LPCWSTR servername, LPCWSTR username, DWORD level,
               LPBYTE* bufptr)
{
    FIXME("(%s, %s, %ld, %p)\n", debugstr_w(servername), debugstr_w(username),
          level, bufptr);
    return NERR_Success;
}

/************************************************************
 *                NetWkstaUserGetInfo  (NETAPI32.@)
 */
NET_API_STATUS NetWkstaUserGetInfo(LPWSTR reserved, DWORD level,
                                          LPBYTE* bufptr)
{
    return NERR_Success;
}
