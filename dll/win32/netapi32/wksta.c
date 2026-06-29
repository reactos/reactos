/* Copyright 2002 Andriy Palamarchuk
 * Copyright (c) 2003 Juan Lang
 *
 * netapi32 user functions
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

#include <lmwksta.h>
#include <lmjoin.h>

WINE_DEFAULT_DEBUG_CHANNEL(netapi32);

/************************************************************
 *                NETAPI_IsLocalComputer
 *
 * Checks whether the server name indicates local machine.
 */
DECLSPEC_HIDDEN BOOL NETAPI_IsLocalComputer( LMCSTR name )
{
    WCHAR buf[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(buf) / sizeof(buf[0]);
    BOOL ret;

    if (!name || !name[0]) return TRUE;

    ret = GetComputerNameW( buf,  &size );
    if (ret && name[0] == '\\' && name[1] == '\\') name += 2;
    return ret && !strcmpiW( name, buf );
}

static void wprint_mac(WCHAR* buffer, int len, const MIB_IFROW *ifRow)
{
    int i;
    unsigned char val;

    if (!buffer)
        return;
    if (len < 1)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    for (i = 0; i < ifRow->dwPhysAddrLen && 2 * i < len; i++)
    {
        val = ifRow->bPhysAddr[i];
        if ((val >>4) >9)
            buffer[2*i] = (WCHAR)((val >>4) + 'A' - 10);
        else
            buffer[2*i] = (WCHAR)((val >>4) + '0');
        if ((val & 0xf ) >9)
            buffer[2*i+1] = (WCHAR)((val & 0xf) + 'A' - 10);
        else
            buffer[2*i+1] = (WCHAR)((val & 0xf) + '0');
    }
    buffer[2*i]=0;
}

/* Theoretically this could be too short, except that MS defines
 * MAX_ADAPTER_NAME as 128, and MAX_INTERFACE_NAME_LEN as 256, and both
 * represent a count of WCHARs, so even with an extraordinarily long header
 * this will be plenty
 */
#define MAX_TRANSPORT_NAME MAX_INTERFACE_NAME_LEN
#define MAX_TRANSPORT_ADDR 13

#define NBT_TRANSPORT_NAME_HEADER "\\Device\\NetBT_Tcpip_"
#define UNKNOWN_TRANSPORT_NAME_HEADER "\\Device\\UnknownTransport_"

static void wprint_name(WCHAR *buffer, int len, ULONG transport,
 PMIB_IFROW ifRow)
{
    WCHAR *ptr1, *ptr2;
    const char *name;

    if (!buffer)
        return;
    if (!ifRow)
    {
        *buffer = '\0';
        return;
    }

    if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
        name = NBT_TRANSPORT_NAME_HEADER;
    else
        name = UNKNOWN_TRANSPORT_NAME_HEADER;

    for (ptr1 = buffer; *name && ptr1 < buffer + len; ptr1++, name++)
        *ptr1 = *name;
    for (ptr2 = ifRow->wszName; *ptr2 && ptr1 < buffer + len; ptr1++, ptr2++)
        *ptr1 = *ptr2;
    *ptr1 = '\0';
}

/***********************************************************************
 *                NetWkstaTransportEnum  (NETAPI32.@)
 */
 
struct WkstaTransportEnumData
{
    UCHAR          n_adapt;
    UCHAR          n_read;
    DWORD          prefmaxlen;
    LPBYTE        *pbuf;
    NET_API_STATUS ret;
};

/**********************************************************************/

static BOOL WkstaEnumAdaptersCallback(UCHAR totalLANAs, UCHAR lanaIndex,
 ULONG transport, const NetBIOSAdapterImpl *data, void *closure)
{
    BOOL ret;
    struct WkstaTransportEnumData *enumData = closure;

    if (enumData && enumData->pbuf)
    {
        if (lanaIndex == 0)
        {
            DWORD toAllocate;

            enumData->n_adapt = totalLANAs;
            enumData->n_read = 0;

            toAllocate = totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0)
             + MAX_TRANSPORT_NAME * sizeof(WCHAR) +
             MAX_TRANSPORT_ADDR * sizeof(WCHAR));
            if (enumData->prefmaxlen != MAX_PREFERRED_LENGTH)
                toAllocate = enumData->prefmaxlen;
            NetApiBufferAllocate(toAllocate, (LPVOID *)enumData->pbuf);
        }
        if (*(enumData->pbuf))
        {
            UCHAR spaceFor;

            if (enumData->prefmaxlen == MAX_PREFERRED_LENGTH)
                spaceFor = totalLANAs;
            else
                spaceFor = enumData->prefmaxlen /
                 (sizeof(WKSTA_TRANSPORT_INFO_0) + (MAX_TRANSPORT_NAME +
                 MAX_TRANSPORT_ADDR) * sizeof(WCHAR));
            if (enumData->n_read < spaceFor)
            {
                PWKSTA_TRANSPORT_INFO_0 ti;
                LMSTR transport_name, transport_addr;
                MIB_IFROW ifRow;

                ti = (PWKSTA_TRANSPORT_INFO_0)(*(enumData->pbuf) +
                 enumData->n_read * sizeof(WKSTA_TRANSPORT_INFO_0));
                transport_name = (LMSTR)(*(enumData->pbuf) +
                 totalLANAs * sizeof(WKSTA_TRANSPORT_INFO_0) +
                 enumData->n_read * MAX_TRANSPORT_NAME * sizeof(WCHAR));
                transport_addr = (LMSTR)(*(enumData->pbuf) +
                 totalLANAs * (sizeof(WKSTA_TRANSPORT_INFO_0) +
                 MAX_TRANSPORT_NAME * sizeof(WCHAR)) +
                 enumData->n_read * MAX_TRANSPORT_ADDR * sizeof(WCHAR));

                ifRow.dwIndex = data->ifIndex;
                GetIfEntry(&ifRow);
                ti->wkti0_quality_of_service = 0;
                ti->wkti0_number_of_vcs = 0;
                ti->wkti0_transport_name = transport_name;
                wprint_name(ti->wkti0_transport_name, MAX_TRANSPORT_NAME,
                 transport, &ifRow);
                ti->wkti0_transport_address = transport_addr;
                wprint_mac(ti->wkti0_transport_address, MAX_TRANSPORT_ADDR,
                 &ifRow);
                if (!memcmp(&transport, TRANSPORT_NBT, sizeof(ULONG)))
                    ti->wkti0_wan_ish = TRUE;
                else
                    ti->wkti0_wan_ish = FALSE;
                TRACE("%d of %d:ti at %p\n", lanaIndex, totalLANAs, ti);
                TRACE("transport_name at %p %s\n",
                 ti->wkti0_transport_name,
                 debugstr_w(ti->wkti0_transport_name));
                TRACE("transport_address at %p %s\n",
                 ti->wkti0_transport_address,
                 debugstr_w(ti->wkti0_transport_address));
                enumData->n_read++;
                enumData->ret = NERR_Success;
                ret = TRUE;
            }
            else
            {
                enumData->ret = ERROR_MORE_DATA;
                ret = FALSE;
            }
        }
        else
        {
            enumData->ret = ERROR_OUTOFMEMORY;
            ret = FALSE;
        }
    }
    else
        ret = FALSE;
    return ret;
}

/**********************************************************************/

NET_API_STATUS WINAPI 
NetWkstaTransportEnum(LMSTR ServerName, DWORD level, PBYTE* pbuf,
      DWORD prefmaxlen, LPDWORD read_entries,
      PDWORD total_entries, PDWORD hresume)
{
    NET_API_STATUS ret;

    TRACE(":%s, 0x%08x, %p, 0x%08x, %p, %p, %p\n", debugstr_w(ServerName), 
     level, pbuf, prefmaxlen, read_entries, total_entries,hresume);
    if (!NETAPI_IsLocalComputer(ServerName))
    {
        FIXME(":not implemented for non-local computers\n");
        ret = ERROR_INVALID_LEVEL;
    }
    else
    {
        if (hresume && *hresume)
        {
          FIXME(":resume handle not implemented\n");
          return ERROR_INVALID_LEVEL;
        }

        switch (level)
        {
            case 0: /* transport info */
            {
                ULONG allTransports;
                struct WkstaTransportEnumData enumData;

                if (NetBIOSNumAdapters() == 0)
                  return ERROR_NETWORK_UNREACHABLE;
                if (!read_entries)
                  return STATUS_ACCESS_VIOLATION;
                if (!total_entries || !pbuf)
                  return RPC_X_NULL_REF_POINTER;

                enumData.prefmaxlen = prefmaxlen;
                enumData.pbuf = pbuf;
                memcpy(&allTransports, ALL_TRANSPORTS, sizeof(ULONG));
                NetBIOSEnumAdapters(allTransports, WkstaEnumAdaptersCallback,
                 &enumData);
                *read_entries = enumData.n_read;
                *total_entries = enumData.n_adapt;
                if (hresume) *hresume= 0;
                ret = enumData.ret;
                break;
            }
            default:
                TRACE("Invalid level %d is specified\n", level);
                ret = ERROR_INVALID_LEVEL;
        }
    }
    return ret;
}


/************************************************************
 *                NetpGetComputerName  (NETAPI32.@)
 */
NET_API_STATUS WINAPI NetpGetComputerName(LPWSTR *Buffer)
{
    DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;

    TRACE("(%p)\n", Buffer);
    NetApiBufferAllocate(dwSize * sizeof(WCHAR), (LPVOID *) Buffer);
    if (GetComputerNameW(*Buffer,  &dwSize))
    {
        return NetApiBufferReallocate(
            *Buffer, (dwSize + 1) * sizeof(WCHAR),
            (LPVOID *) Buffer);
    }
    else
    {
        NetApiBufferFree(*Buffer);
        return ERROR_NOT_ENOUGH_MEMORY;
    }
}

NET_API_STATUS WINAPI I_NetNameCompare(LPVOID p1, LPWSTR wkgrp, LPWSTR comp,
 LPVOID p4, LPVOID p5)
{
    FIXME("(%p %s %s %p %p): stub\n", p1, debugstr_w(wkgrp), debugstr_w(comp),
     p4, p5);
    return ERROR_INVALID_PARAMETER;
}

NET_API_STATUS WINAPI I_NetNameValidate(LPVOID p1, LPWSTR wkgrp, LPVOID p3,
 LPVOID p4)
{
    FIXME("(%p %s %p %p): stub\n", p1, debugstr_w(wkgrp), p3, p4);
    return ERROR_INVALID_PARAMETER;
}
