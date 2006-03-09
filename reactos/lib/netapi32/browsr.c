/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * netapi32 browser functions
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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "lmcons.h"
#include "lmbrowsr.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                I_BrowserSetNetlogonState  (NETAPI32.@)
 */
NET_API_STATUS WINAPI I_BrowserSetNetlogonState(
    LPWSTR ServerName, LPWSTR DomainName, LPWSTR EmulatedServerName,
    DWORD Role)
{
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                I_BrowserQueryEmulatedDomains  (NETAPI32.@)
 */
NET_API_STATUS WINAPI I_BrowserQueryEmulatedDomains(LPWSTR ServerName,PBYTE* blah,PDWORD EntriesRead)
//NET_API_STATUS I_BrowserQueryEmulatedDomains(
//    LPWSTR ServerName, PBYTE *EmulatedDomains
//    PDWORD EntriesRead)
{
    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 *                NetShareEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareEnum( LPWSTR servername, DWORD level, LPBYTE* bufptr,
  DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("%s %ld %p %ld %p %p %p\n", debugstr_w(servername), level, bufptr,
          prefmaxlen, entriesread, totalentries, resume_handle);
    return ERROR_NOT_SUPPORTED;
}
