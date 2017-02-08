/*
 * Copyright 2002 Andriy Palamarchuk
 *
 * netapi32 access functions
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

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 * NetGroupAdd  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupAdd(IN LPCWSTR servername,
            IN DWORD level,
            IN LPBYTE buf,
            OUT LPDWORD parm_err)
{
    FIXME("(%s, %d, %p, %p) stub!\n", debugstr_w(servername),
          level, buf, parm_err);
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupAddUser  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupAddUser(IN LPCWSTR servername,
                IN LPCWSTR groupname,
                IN LPCWSTR username)
{
    FIXME("(%s, %s, %s) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), debugstr_w(username));
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupDel  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupDel(IN LPCWSTR servername,
            IN LPCWSTR groupname)
{
    FIXME("(%s, %s) stub!\n", debugstr_w(servername),
          debugstr_w(groupname));
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupDelUser  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupDelUser(IN LPCWSTR servername,
                IN LPCWSTR groupname,
                IN LPCWSTR username)
{
    FIXME("(%s, %s, %s) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), debugstr_w(username));
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupEnum  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupEnum(IN LPCWSTR servername,
             IN DWORD level,
             OUT LPBYTE *bufptr,
             IN DWORD prefmaxlen,
             OUT LPDWORD entriesread,
             OUT LPDWORD totalentries,
             IN OUT PDWORD_PTR resume_handle)
{
    FIXME("(%s, %d, %p, %d, %p, %p, %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resume_handle);
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupGetInfo  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupGetInfo(IN LPCWSTR servername,
                IN LPCWSTR groupname,
                IN DWORD level,
                OUT LPBYTE *bufptr)
{
    FIXME("(%s, %s, %d, %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, bufptr);
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupGetUsers  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupGetUsers(IN LPCWSTR servername,
                 IN LPCWSTR groupname,
                 IN DWORD level,
                 OUT LPBYTE *bufptr,
                 IN DWORD prefmaxlen,
                 OUT LPDWORD entriesread,
                 OUT LPDWORD totalentries,
                 IN OUT PDWORD_PTR resume_handle)
{
    FIXME("(%s, %s, %d, %p, %d, %p, %p, %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, bufptr, prefmaxlen, entriesread,
          totalentries, resume_handle);
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupSetInfo  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupSetInfo(IN LPCWSTR servername,
                IN LPCWSTR groupname,
                IN DWORD level,
                IN LPBYTE buf,
                OUT LPDWORD parm_err)
{
    FIXME("(%s, %s, %d, %p, %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, parm_err);
    return ERROR_ACCESS_DENIED;
}


/************************************************************
 * NetGroupSetUsers  (NETAPI32.@)
 *
 */
NET_API_STATUS
WINAPI
NetGroupSetUsers(IN LPCWSTR servername,
                 IN LPCWSTR groupname,
                 IN DWORD level,
                 IN LPBYTE buf,
                 IN DWORD totalentries)
{
    FIXME("(%s, %s, %d, %p, %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return ERROR_ACCESS_DENIED;
}

/* EOF */
