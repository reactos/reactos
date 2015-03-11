/*
 * Copyright 2005 Paul Vriens
 *
 * netapi32 directory service functions
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

#include <dsrole.h>
#include <dsgetdc.h>

WINE_DEFAULT_DEBUG_CHANNEL(ds);

DWORD WINAPI DsGetDcNameW(LPCWSTR ComputerName, LPCWSTR AvoidDCName,
 GUID* DomainGuid, LPCWSTR SiteName, ULONG Flags,
 PDOMAIN_CONTROLLER_INFOW *DomainControllerInfo)
{
    FIXME("(%s, %s, %s, %s, %08x, %p): stub\n", debugstr_w(ComputerName),
     debugstr_w(AvoidDCName), debugstr_guid(DomainGuid),
     debugstr_w(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetDcNameA(LPCSTR ComputerName, LPCSTR AvoidDCName,
 GUID* DomainGuid, LPCSTR SiteName, ULONG Flags,
 PDOMAIN_CONTROLLER_INFOA *DomainControllerInfo)
{
    FIXME("(%s, %s, %s, %s, %08x, %p): stub\n", debugstr_a(ComputerName),
     debugstr_a(AvoidDCName), debugstr_guid(DomainGuid),
     debugstr_a(SiteName), Flags, DomainControllerInfo);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD WINAPI DsGetSiteNameW(LPCWSTR ComputerName, LPWSTR *SiteName)
{
    FIXME("(%s, %p): stub\n", debugstr_w(ComputerName), SiteName);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

/************************************************************
 *  DsRoleFreeMemory (NETAPI32.@)
 *
 * PARAMS
 *  Buffer [I] Pointer to the to-be-freed buffer.
 *
 * RETURNS
 *  Nothing
 */
VOID WINAPI DsRoleFreeMemory(PVOID Buffer)
{
    TRACE("(%p)\n", Buffer);
    HeapFree(GetProcessHeap(), 0, Buffer);
}
