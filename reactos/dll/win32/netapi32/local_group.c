/*
 * Copyright 2006 Robert Reif
 *
 * netapi32 local group functions
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

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "lmcons.h"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "lmerr.h"
#include "winreg.h"
#include "ntsecapi.h"
#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                NetLocalGroupAdd  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAdd(
    LPCWSTR servername,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %d %p %p) stub!\n", debugstr_w(servername), level, buf,
          parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupAddMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupAddMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDel  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDel(
    LPCWSTR servername,
    LPCWSTR groupname)
{
    FIXME("(%s %s) stub!\n", debugstr_w(servername), debugstr_w(groupname));
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMember  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMember(
    LPCWSTR servername,
    LPCWSTR groupname,
    PSID membersid)
{
    FIXME("(%s %s %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), membersid);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupDelMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupDelMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupEnum  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupEnum(
    LPCWSTR servername,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %d %p %d %p %p %p) stub!\n", debugstr_w(servername),
          level, bufptr, prefmaxlen, entriesread, totalentries, resumehandle);
    *entriesread = 0;
    *totalentries = 0;
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE* bufptr)
{
    FIXME("(%s %s %d %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, bufptr);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupGetMembers  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupGetMembers(
    LPCWSTR servername,
    LPCWSTR localgroupname,
    DWORD level,
    LPBYTE* bufptr,
    DWORD prefmaxlen,
    LPDWORD entriesread,
    LPDWORD totalentries,
    PDWORD_PTR resumehandle)
{
    FIXME("(%s %s %d %p %d, %p %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(localgroupname), level, bufptr, prefmaxlen, entriesread,
          totalentries, resumehandle);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetInfo  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetInfo(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    LPDWORD parm_err)
{
    FIXME("(%s %s %d %p %p) stub!\n", debugstr_w(servername),
          debugstr_w(groupname), level, buf, parm_err);
    return NERR_Success;
}

/************************************************************
 *                NetLocalGroupSetMember (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetLocalGroupSetMembers(
    LPCWSTR servername,
    LPCWSTR groupname,
    DWORD level,
    LPBYTE buf,
    DWORD totalentries)
{
    FIXME("(%s %s %d %p %d) stub!\n", debugstr_w(servername),
            debugstr_w(groupname), level, buf, totalentries);
    return NERR_Success;
}
