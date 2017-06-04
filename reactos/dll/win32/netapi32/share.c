/* Copyright 2006 Paul Vriens
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

#include "netapi32.h"

WINE_DEFAULT_DEBUG_CHANNEL(share);

/************************************************************
 *                NetFileEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetFileEnum(
    LPWSTR ServerName, LPWSTR BasePath, LPWSTR UserName,
    DWORD Level, LPBYTE* BufPtr, DWORD PrefMaxLen,
    LPDWORD EntriesRead, LPDWORD TotalEntries, PDWORD_PTR ResumeHandle)
{
    FIXME("(%s, %s, %s, %u): stub\n", debugstr_w(ServerName), debugstr_w(BasePath),
        debugstr_w(UserName), Level);
    return ERROR_NOT_SUPPORTED;
}
