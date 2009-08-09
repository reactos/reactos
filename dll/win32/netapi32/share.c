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

#include "wine/debug.h"
#include "lm.h"
#include "winerror.h"

WINE_DEFAULT_DEBUG_CHANNEL(share);

/************************************************************
 * NetSessionEnum  (NETAPI32.@)
 *
 * PARAMS
 *   servername    [I]   Pointer to a string with the name of the server
 *   UncClientName [I]   Pointer to a string with the name of the session
 *   username      [I]   Pointer to a string with the name of the user
 *   level         [I]   Data information level
 *   bufptr        [O]   Buffer to the data
 *   prefmaxlen    [I]   Preferred maximum length of the data
 *   entriesread   [O]   Pointer to the number of entries enumerated
 *   totalentries  [O]   Pointer to the possible number of entries
 *   resume_handle [I/O] Pointer to a handle for subsequent searches
 *
 * RETURNS
 *   If successful, the function returns NERR_Success
 *   On failure it returns:
 *     ERROR_ACCESS_DENIED         User has no access to the requested information
 *     ERROR_INVALID_LEVEL         Value of 'level' is not correct
 *     ERROR_INVALID_PARAMETER     Wrong parameter
 *     ERROR_MORE_DATA             Need a larger buffer
 *     ERROR_NOT_ENOUGH_MEMORY     Not enough memory
 *     NERR_ClientNameNotFound     A session does not exist on a given computer
 *     NERR_InvalidComputer        Invalid computer name
 *     NERR_UserNotFound           User name could not be found.
 */
NET_API_STATUS WINAPI NetSessionEnum(LMSTR servername, LMSTR UncClientName,
    LMSTR username, DWORD level, LPBYTE* bufptr, DWORD prefmaxlen, LPDWORD entriesread,
    LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("Stub (%s %s %s %d %p %d %p %p %p)\n", debugstr_w(servername),
        debugstr_w(UncClientName), debugstr_w(username),
        level, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);

    return NERR_Success;
}

/************************************************************
 * NetShareEnum  (NETAPI32.@)
 *
 * PARAMS
 *   servername    [I]   Pointer to a string with the name of the server
 *   level         [I]   Data information level
 *   bufptr        [O]   Buffer to the data
 *   prefmaxlen    [I]   Preferred maximum length of the data
 *   entriesread   [O]   Pointer to the number of entries enumerated
 *   totalentries  [O]   Pointer to the possible number of entries
 *   resume_handle [I/O] Pointer to a handle for subsequent searches
 *
 * RETURNS
 *   If successful, the function returns NERR_Success
 *   On failure it returns a system error code (FIXME: find out which)
 *
 */
NET_API_STATUS WINAPI NetShareEnum( LMSTR servername, DWORD level, LPBYTE* bufptr,
    DWORD prefmaxlen, LPDWORD entriesread, LPDWORD totalentries, LPDWORD resume_handle)
{
    FIXME("Stub (%s %d %p %d %p %p %p)\n", debugstr_w(servername), level, bufptr,
        prefmaxlen, entriesread, totalentries, resume_handle);

    return ERROR_NOT_SUPPORTED;
}

/************************************************************
 * NetShareDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareDel(LMSTR servername, LMSTR netname, DWORD reserved)
{
    FIXME("Stub (%s %s %d)\n", debugstr_w(servername), debugstr_w(netname), reserved);
    return NERR_Success;
}

/************************************************************
 * NetShareGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareGetInfo(LMSTR servername, LMSTR netname,
    DWORD level, LPBYTE *bufptr)
{
    FIXME("Stub (%s %s %d %p)\n", debugstr_w(servername),
        debugstr_w(netname),level, bufptr);
    return NERR_NetNameNotFound;
}

/************************************************************
 * NetShareAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetShareAdd(LMSTR servername,
    DWORD level, LPBYTE buf, LPDWORD parm_err)
{
    FIXME("Stub (%s %d %p %p)\n", debugstr_w(servername), level, buf, parm_err);
    return ERROR_NOT_SUPPORTED;
}
