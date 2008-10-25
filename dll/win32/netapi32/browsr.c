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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "lmcons.h"
#include "lmbrowsr.h"
#include "lmshare.h"

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
NET_API_STATUS WINAPI I_BrowserQueryEmulatedDomains(
    LPWSTR ServerName, PBROWSER_EMULATED_DOMAIN *EmulatedDomains,
    LPDWORD EntriesRead)
{
    return ERROR_NOT_SUPPORTED;
}
