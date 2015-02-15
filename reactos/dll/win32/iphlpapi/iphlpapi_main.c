/*
 * iphlpapi dll implementation
 *
 * Copyright (C) 2003 Juan Lang
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#define DEBUG

#include <config.h>
#include "iphlpapi_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(iphlpapi);

typedef struct _NAME_SERVER_LIST_CONTEXT {
    ULONG uSizeAvailable;
    ULONG uSizeRequired;
    PIP_PER_ADAPTER_INFO pData;
    UINT NumServers;
    IP_ADDR_STRING *pLastAddr;
} NAME_SERVER_LIST_CONTEXT, *PNAME_SERVER_LIST_CONTEXT;

BOOL WINAPI DllMain (HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
  switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
      DisableThreadLibraryCalls( hinstDLL );
      interfaceMapInit();
      break;

    case DLL_PROCESS_DETACH:
      interfaceMapFree();
      break;
  }
  return TRUE;
}

/******************************************************************
 *    AddIPAddress (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  Address [In]
 *  IpMask [In]
 *  IfIndex [In]
 *  NTEContext [In/Out]
 *  NTEInstance [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI AddIPAddress(IPAddr Address, IPMask Netmask, DWORD IfIndex, PULONG NteContext, PULONG NteInstance)
{
    return RtlNtStatusToDosError(addIPAddress(Address, Netmask, IfIndex, NteContext, NteInstance));
}

DWORD getInterfaceGatewayByIndex(DWORD index)
{
   DWORD ndx, retVal = 0, numRoutes = getNumRoutes();
   RouteTable *table = getRouteTable();
   if (!table) return 0;

    for (ndx = 0; ndx < numRoutes; ndx++)
    {
        if ((table->routes[ndx].ifIndex == (index)) && (table->routes[ndx].dest == 0))
            retVal = table->routes[ndx].gateway;
    }
    HeapFree(GetProcessHeap(), 0, table);
    return retVal;
}

/******************************************************************
 *    AllocateAndGetIfTableFromStack (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  ppIfTable [Out] -- pointer into which the MIB_IFTABLE is
 *   allocated and returned.
 *  bOrder [In] -- passed to GetIfTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS -- ERROR_INVALID_PARAMETER if ppIfTable is NULL, whatever
 *  GetIfTable returns otherwise
 *
 */
DWORD WINAPI AllocateAndGetIfTableFromStack(PMIB_IFTABLE *ppIfTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIfTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n", ppIfTable,
   (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppIfTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIfTable(*ppIfTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIfTable = (PMIB_IFTABLE)HeapAlloc(heap, flags, dwSize);
      ret = GetIfTable(*ppIfTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpAddrTableFromStack (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  ppIpAddrTable [Out]
 *  bOrder [In] -- passed to GetIpAddrTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI AllocateAndGetIpAddrTableFromStack(PMIB_IPADDRTABLE *ppIpAddrTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpAddrTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppIpAddrTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppIpAddrTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIpAddrTable(*ppIpAddrTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIpAddrTable = (PMIB_IPADDRTABLE)HeapAlloc(heap, flags, dwSize);
      ret = GetIpAddrTable(*ppIpAddrTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpForwardTableFromStack (IPHLPAPI.@)
 *
 *
 *  ppIpForwardTable [Out] -- pointer into which the MIB_IPFORWARDTABLE is
 *   allocated and returned.
 *  bOrder [In] -- passed to GetIfTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS -- ERROR_INVALID_PARAMETER if ppIfTable is NULL, whatever
 *  GetIpForwardTable returns otherwise
 *
 */
DWORD WINAPI AllocateAndGetIpForwardTableFromStack(PMIB_IPFORWARDTABLE *
 ppIpForwardTable, BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpForwardTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppIpForwardTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppIpForwardTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIpForwardTable(*ppIpForwardTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIpForwardTable = (PMIB_IPFORWARDTABLE)HeapAlloc(heap, flags, dwSize);
      ret = GetIpForwardTable(*ppIpForwardTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetIpNetTableFromStack (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  ppIpNetTable [Out]
 *  bOrder [In] -- passed to GetIpNetTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI AllocateAndGetIpNetTableFromStack(PMIB_IPNETTABLE *ppIpNetTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppIpNetTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppIpNetTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppIpNetTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetIpNetTable(*ppIpNetTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppIpNetTable = (PMIB_IPNETTABLE)HeapAlloc(heap, flags, dwSize);
      ret = GetIpNetTable(*ppIpNetTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetTcpTableFromStack (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  ppTcpTable [Out]
 *  bOrder [In] -- passed to GetTcpTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI AllocateAndGetTcpTableFromStack(PMIB_TCPTABLE *ppTcpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppTcpTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppTcpTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppTcpTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetTcpTable(*ppTcpTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppTcpTable = (PMIB_TCPTABLE)HeapAlloc(heap, flags, dwSize);
      ret = GetTcpTable(*ppTcpTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    AllocateAndGetUdpTableFromStack (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  ppUdpTable [Out]
 *  bOrder [In] -- passed to GetUdpTable to order the table
 *  heap [In] -- heap from which the table is allocated
 *  flags [In] -- flags to HeapAlloc
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI AllocateAndGetUdpTableFromStack(PMIB_UDPTABLE *ppUdpTable,
 BOOL bOrder, HANDLE heap, DWORD flags)
{
  DWORD ret;

  TRACE("ppUdpTable %p, bOrder %ld, heap 0x%08lx, flags 0x%08lx\n",
   ppUdpTable, (DWORD)bOrder, (DWORD)heap, flags);
  if (!ppUdpTable)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD dwSize = 0;

    ret = GetUdpTable(*ppUdpTable, &dwSize, bOrder);
    if (ret == ERROR_INSUFFICIENT_BUFFER) {
      *ppUdpTable = (PMIB_UDPTABLE)HeapAlloc(heap, flags, dwSize);
      ret = GetUdpTable(*ppUdpTable, &dwSize, bOrder);
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    CreateIpForwardEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pRoute [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI CreateIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
    return createIpForwardEntry( pRoute );
}


/******************************************************************
 *    CreateIpNetEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pArpEntry [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI CreateIpNetEntry(PMIB_IPNETROW pArpEntry)
{
  TRACE("pArpEntry %p\n", pArpEntry);
  /* could use SIOCSARP on systems that support it, not sure I want to */
  FIXME(":stub\n");
  return (DWORD) 0;
}


/******************************************************************
 *    CreateProxyArpEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  dwAddress [In]
 *  dwMask [In]
 *  dwIfIndex [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI CreateProxyArpEntry(DWORD dwAddress, DWORD dwMask, DWORD dwIfIndex)
{
  TRACE("dwAddress 0x%08lx, dwMask 0x%08lx, dwIfIndex 0x%08lx\n", dwAddress,
   dwMask, dwIfIndex);
  FIXME(":stub\n");
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    DeleteIPAddress (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  NTEContext [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI DeleteIPAddress(ULONG NTEContext)
{
  TRACE("NTEContext %ld\n", NTEContext);
  return RtlNtStatusToDosError(deleteIpAddress(NTEContext));
}


/******************************************************************
 *    DeleteIpForwardEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pRoute [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI DeleteIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
    return deleteIpForwardEntry( pRoute );
}


/******************************************************************
 *    DeleteIpNetEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pArpEntry [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI DeleteIpNetEntry(PMIB_IPNETROW pArpEntry)
{
  TRACE("pArpEntry %p\n", pArpEntry);
  /* could use SIOCDARP on systems that support it, not sure I want to */
  FIXME(":stub\n");
  return (DWORD) 0;
}


/******************************************************************
 *    DeleteProxyArpEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  dwAddress [In]
 *  dwMask [In]
 *  dwIfIndex [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI DeleteProxyArpEntry(DWORD dwAddress, DWORD dwMask, DWORD dwIfIndex)
{
  TRACE("dwAddress 0x%08lx, dwMask 0x%08lx, dwIfIndex 0x%08lx\n", dwAddress,
   dwMask, dwIfIndex);
  FIXME(":stub\n");
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}

/******************************************************************
 *    EnableRouter (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pHandle [In/Out]
 *  pOverlapped [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI EnableRouter(HANDLE * pHandle, OVERLAPPED * pOverlapped)
{
  TRACE("pHandle %p, pOverlapped %p\n", pHandle, pOverlapped);
  FIXME(":stub\n");
  /* could echo "1" > /proc/net/sys/net/ipv4/ip_forward, not sure I want to
     could map EACCESS to ERROR_ACCESS_DENIED, I suppose
     marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    FlushIpNetTable (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  dwIfIndex [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI FlushIpNetTable(DWORD dwIfIndex)
{
  TRACE("dwIfIndex 0x%08lx\n", dwIfIndex);
  FIXME(":stub\n");
  /* this flushes the arp cache of the given index
     marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    GetAdapterIndex (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  AdapterName [In/Out]
 *  IfIndex [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetAdapterIndex(LPWSTR AdapterName, PULONG IfIndex)
{
  TRACE("AdapterName %p, IfIndex %p\n", AdapterName, IfIndex);
  FIXME(":stub\n");
  /* marking Win2K+ functions not supported */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    GetAdaptersInfo (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pAdapterInfo [In/Out]
 *  pOutBufLen [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen)
{
  DWORD ret;
  BOOL dhcpEnabled;
  DWORD dhcpServer;

  TRACE("pAdapterInfo %p, pOutBufLen %p\n", pAdapterInfo, pOutBufLen);
  if (!pOutBufLen)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numNonLoopbackInterfaces = getNumNonLoopbackInterfaces();

    if (numNonLoopbackInterfaces > 0) {
      /* this calculation assumes only one address in the IP_ADDR_STRING lists.
         that's okay, because:
         - we don't get multiple addresses per adapter anyway
         - we don't know about per-adapter gateways
         - DHCP and WINS servers can have max one entry per list */
      ULONG size = sizeof(IP_ADAPTER_INFO) * numNonLoopbackInterfaces;

      if (!pAdapterInfo || *pOutBufLen < size) {
        *pOutBufLen = size;
        ret = ERROR_BUFFER_OVERFLOW;
      }
      else {
        InterfaceIndexTable *table = getNonLoopbackInterfaceIndexTable();

        if (table) {
          size = sizeof(IP_ADAPTER_INFO) * table->numIndexes;
          if (*pOutBufLen < size) {
            *pOutBufLen = size;
            ret = ERROR_INSUFFICIENT_BUFFER;
          }
          else {
            DWORD ndx;
            HKEY hKey;
            BOOL winsEnabled = FALSE;
            IP_ADDRESS_STRING primaryWINS, secondaryWINS;

            memset(pAdapterInfo, 0, size);
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
             "Software\\Wine\\Wine\\Config\\Network", 0, KEY_READ,
             &hKey) == ERROR_SUCCESS) {
              DWORD size = sizeof(primaryWINS.String);
              unsigned long addr;

              RegQueryValueExA(hKey, "WinsServer", NULL, NULL,
               (PBYTE)primaryWINS.String, &size);
              addr = inet_addr(primaryWINS.String);
              if (addr != INADDR_NONE && addr != INADDR_ANY)
                winsEnabled = TRUE;
              size = sizeof(secondaryWINS.String);
              RegQueryValueExA(hKey, "BackupWinsServer", NULL, NULL,
               (PBYTE)secondaryWINS.String, &size);
              addr = inet_addr(secondaryWINS.String);
              if (addr != INADDR_NONE && addr != INADDR_ANY)
                winsEnabled = TRUE;
              RegCloseKey(hKey);
            }
			TRACE("num of index is %lu\n", table->numIndexes);
            for (ndx = 0; ndx < table->numIndexes; ndx++) {
              PIP_ADAPTER_INFO ptr = &pAdapterInfo[ndx];
              DWORD addrLen = sizeof(ptr->Address), type;
              const char *ifname =
                  getInterfaceNameByIndex(table->indexes[ndx]);
              if (!ifname) {
                  ret = ERROR_OUTOFMEMORY;
                  break;
              }

              /* on Win98 this is left empty, but whatever */
              strncpy(ptr->AdapterName,ifname,sizeof(ptr->AdapterName));
              consumeInterfaceName(ifname);
              ptr->AdapterName[MAX_ADAPTER_NAME_LENGTH] = '\0';
              getInterfacePhysicalByIndex(table->indexes[ndx], &addrLen,
               ptr->Address, &type);
              /* MS defines address length and type as UINT in some places and
                 DWORD in others, **sigh**.  Don't want to assume that PUINT and
                 PDWORD are equiv (64-bit?) */
              ptr->AddressLength = addrLen;
              ptr->Type = type;
              ptr->Index = table->indexes[ndx];
              toIPAddressString(getInterfaceIPAddrByIndex(table->indexes[ndx]),
               ptr->IpAddressList.IpAddress.String);
              toIPAddressString(getInterfaceMaskByIndex(table->indexes[ndx]),
               ptr->IpAddressList.IpMask.String);
              ptr->IpAddressList.Context = ptr->Index;
              toIPAddressString(getInterfaceGatewayByIndex(table->indexes[ndx]),
               ptr->GatewayList.IpAddress.String);
              getDhcpInfoForAdapter(table->indexes[ndx], &dhcpEnabled,
                                    &dhcpServer, &ptr->LeaseObtained,
                                    &ptr->LeaseExpires);
              ptr->DhcpEnabled = (DWORD) dhcpEnabled;
              toIPAddressString(dhcpServer,
                                ptr->DhcpServer.IpAddress.String);
              if (winsEnabled) {
                ptr->HaveWins = TRUE;
                memcpy(ptr->PrimaryWinsServer.IpAddress.String,
                 primaryWINS.String, sizeof(primaryWINS.String));
                memcpy(ptr->SecondaryWinsServer.IpAddress.String,
                 secondaryWINS.String, sizeof(secondaryWINS.String));
              }
              if (ndx < table->numIndexes - 1)
                ptr->Next = &pAdapterInfo[ndx + 1];
              else
                ptr->Next = NULL;
            }
            ret = NO_ERROR;
          }
          free(table);
        }
        else
          ret = ERROR_OUTOFMEMORY;
      }
    }
    else
      ret = ERROR_NO_DATA;
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetBestInterface (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  dwDestAddr [In]
 *  pdwBestIfIndex [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetBestInterface(IPAddr dwDestAddr, PDWORD pdwBestIfIndex)
{
  DWORD ret;

  TRACE("dwDestAddr 0x%08lx, pdwBestIfIndex %p\n", dwDestAddr, pdwBestIfIndex);
  if (!pdwBestIfIndex)
    ret = ERROR_INVALID_PARAMETER;
  else {
    MIB_IPFORWARDROW ipRow;

    ret = GetBestRoute(dwDestAddr, 0, &ipRow);
    if (ret == ERROR_SUCCESS)
      *pdwBestIfIndex = ipRow.dwForwardIfIndex;
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetBestRoute (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  dwDestAddr [In]
 *  dwSourceAddr [In]
 *  OUT [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetBestRoute(DWORD dwDestAddr, DWORD dwSourceAddr, PMIB_IPFORWARDROW pBestRoute)
{
  PMIB_IPFORWARDTABLE table;
  DWORD ret;

  TRACE("dwDestAddr 0x%08lx, dwSourceAddr 0x%08lx, pBestRoute %p\n", dwDestAddr,
   dwSourceAddr, pBestRoute);
  if (!pBestRoute)
    return ERROR_INVALID_PARAMETER;

  AllocateAndGetIpForwardTableFromStack(&table, FALSE, GetProcessHeap(), 0);
  if (table) {
    DWORD ndx, matchedBits, matchedNdx = 0;

    for (ndx = 0, matchedBits = 0; ndx < table->dwNumEntries; ndx++) {
      if ((dwDestAddr & table->table[ndx].dwForwardMask) ==
       (table->table[ndx].dwForwardDest & table->table[ndx].dwForwardMask)) {
        DWORD numShifts, mask;

        for (numShifts = 0, mask = table->table[ndx].dwForwardMask;
         mask && !(mask & 1); mask >>= 1, numShifts++)
          ;
        if (numShifts > matchedBits) {
          matchedBits = numShifts;
          matchedNdx = ndx;
        }
      }
    }
    memcpy(pBestRoute, &table->table[matchedNdx], sizeof(MIB_IPFORWARDROW));
    HeapFree(GetProcessHeap(), 0, table);
    ret = ERROR_SUCCESS;
  }
  else
    ret = ERROR_OUTOFMEMORY;
  TRACE("returning %ld\n", ret);
  return ret;
}

/******************************************************************
 *    GetExtendedTcpTable (IPHLPAPI.@)
 *
 * Get the table of TCP endpoints available to the application.
 *
 * PARAMS
 *  pTcpTable [Out]    table struct with the filtered TCP endpoints available to application
 *  pdwSize   [In/Out] estimated size of the structure returned in pTcpTable, in bytes
 *  bOrder    [In]     whether to order the table
 *  ulAf	[in]	version of IP used by the TCP endpoints
 *  TableClass [in]	type of the TCP table structure from TCP_TABLE_CLASS
 *  Reserved [in]	reserved - this value must be zero
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: either ERROR_INSUFFICIENT_BUFFER or ERROR_INVALID_PARAMETER
 *
 * NOTES
 */

DWORD WINAPI GetExtendedTcpTable(PVOID pTcpTable, PDWORD pdwSize, BOOL bOrder, ULONG ulAf, TCP_TABLE_CLASS TableClass, ULONG Reserved)
{
	DWORD ret = NO_ERROR;

  if (TableClass == TCP_TABLE_OWNER_PID_ALL) {
    if (*pdwSize == 0) {
      *pdwSize = sizeof(MIB_TCPTABLE_OWNER_PID);
      return ERROR_INSUFFICIENT_BUFFER; 
    } else {
      ZeroMemory(pTcpTable, sizeof(MIB_TCPTABLE_OWNER_PID));
      return NO_ERROR;
    }
  }


    UNIMPLEMENTED;
    return ret;	
}


/******************************************************************
 *    GetFriendlyIfIndex (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  IfIndex [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetFriendlyIfIndex(DWORD IfIndex)
{
  /* windows doesn't validate these, either, just makes sure the top byte is
     cleared.  I assume my ifenum module never gives an index with the top
     byte set. */
  TRACE("returning %ld\n", IfIndex);
  return IfIndex;
}


/******************************************************************
 *    GetIcmpStatistics (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIcmpStatistics(PMIB_ICMP pStats)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getICMPStats(pStats);
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetIfEntry (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIfRow [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIfEntry(PMIB_IFROW pIfRow)
{
  DWORD ret;
  const char *name;

  TRACE("pIfRow %p\n", pIfRow);
  if (!pIfRow)
    return ERROR_INVALID_PARAMETER;

  name = getInterfaceNameByIndex(pIfRow->dwIndex);
  if (name) {
    ret = getInterfaceEntryByIndex(pIfRow->dwIndex, pIfRow);
    if (ret == NO_ERROR)
      ret = getInterfaceStatsByName(name, pIfRow);
    consumeInterfaceName(name);
  }
  else
    ret = ERROR_INVALID_DATA;
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IfTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((PMIB_IFROW)a)->dwIndex - ((PMIB_IFROW)b)->dwIndex;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIfTable (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIfTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIfTable %p, pdwSize %p, bOrder %ld\n", pdwSize, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numInterfaces = getNumInterfaces();
    ULONG size;
    TRACE("GetIfTable: numInterfaces = %d\n", (int)numInterfaces);
    size = sizeof(MIB_IFTABLE) + (numInterfaces - 1) * sizeof(MIB_IFROW);

    if (!pIfTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table = getInterfaceIndexTable();

      if (table) {
        size = sizeof(MIB_IFTABLE) + (table->numIndexes - 1) *
         sizeof(MIB_IFROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx;

          pIfTable->dwNumEntries = 0;
          for (ndx = 0; ndx < table->numIndexes; ndx++) {
            pIfTable->table[ndx].dwIndex = table->indexes[ndx];
            GetIfEntry(&pIfTable->table[ndx]);
            pIfTable->dwNumEntries++;
          }
          if (bOrder)
            qsort(pIfTable->table, pIfTable->dwNumEntries, sizeof(MIB_IFROW),
             IfTableSorter);
          ret = NO_ERROR;
        }
        free(table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetInterfaceInfo (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIfTable [In/Out]
 *  dwOutBufLen [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetInterfaceInfo(PIP_INTERFACE_INFO pIfTable, PULONG dwOutBufLen)
{
  DWORD ret;

  TRACE("pIfTable %p, dwOutBufLen %p\n", pIfTable, dwOutBufLen);
  if (!dwOutBufLen)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numNonLoopbackInterfaces = getNumNonLoopbackInterfaces();
    ULONG size;
    TRACE("numNonLoopbackInterfaces == 0x%x\n", numNonLoopbackInterfaces);
    size = sizeof(IP_INTERFACE_INFO) + (numNonLoopbackInterfaces) *
     sizeof(IP_ADAPTER_INDEX_MAP);

    if (!pIfTable || *dwOutBufLen < size) {
      *dwOutBufLen = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table = getNonLoopbackInterfaceIndexTable();

      if (table) {
        TRACE("table->numIndexes == 0x%x\n", table->numIndexes);
        size = sizeof(IP_INTERFACE_INFO) + (table->numIndexes) *
         sizeof(IP_ADAPTER_INDEX_MAP);
        if (*dwOutBufLen < size) {
          *dwOutBufLen = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx;

          pIfTable->NumAdapters = 0;
          for (ndx = 0; ndx < table->numIndexes; ndx++) {
            const char *walker, *name;
            WCHAR *assigner;

            pIfTable->Adapter[ndx].Index = table->indexes[ndx];
            name = getInterfaceNameByIndex(table->indexes[ndx]);
            for (walker = name, assigner = pIfTable->Adapter[ndx].Name;
             walker && *walker &&
             assigner - pIfTable->Adapter[ndx].Name < MAX_ADAPTER_NAME - 1;
             walker++, assigner++)
              *assigner = *walker;
            *assigner = 0;
            consumeInterfaceName(name);
            pIfTable->NumAdapters++;
          }
          ret = NO_ERROR;
        }
        free(table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IpAddrTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((PMIB_IPADDRROW)a)->dwAddr - ((PMIB_IPADDRROW)b)->dwAddr;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIpAddrTable (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIpAddrTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIpAddrTable %p, pdwSize %p, bOrder %ld\n", pIpAddrTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numInterfaces = getNumInterfaces();
    ULONG size = sizeof(MIB_IPADDRTABLE) + (numInterfaces - 1) *
     sizeof(MIB_IPADDRROW);

    if (!pIpAddrTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      InterfaceIndexTable *table = getInterfaceIndexTable();

      if (table) {
        size = sizeof(MIB_IPADDRTABLE) + (table->numIndexes - 1) *
         sizeof(MIB_IPADDRROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx, bcast;

          pIpAddrTable->dwNumEntries = 0;
          for (ndx = 0; ndx < table->numIndexes; ndx++) {
            pIpAddrTable->table[ndx].dwIndex = table->indexes[ndx];
            pIpAddrTable->table[ndx].dwAddr =
             getInterfaceIPAddrByIndex(table->indexes[ndx]);
            pIpAddrTable->table[ndx].dwMask =
             getInterfaceMaskByIndex(table->indexes[ndx]);
            /* the dwBCastAddr member isn't the broadcast address, it indicates
             * whether the interface uses the 1's broadcast address (1) or the
             * 0's broadcast address (0).
             */
            bcast = getInterfaceBCastAddrByIndex(table->indexes[ndx]);
            pIpAddrTable->table[ndx].dwBCastAddr =
             (bcast & pIpAddrTable->table[ndx].dwMask) ? 1 : 0;
            /* FIXME: hardcoded reasm size, not sure where to get it */
            pIpAddrTable->table[ndx].dwReasmSize = 65535;
            pIpAddrTable->table[ndx].unused1 = 0;
            pIpAddrTable->table[ndx].wType = 0; /* aka unused2 */
            pIpAddrTable->dwNumEntries++;
          }
          if (bOrder)
            qsort(pIpAddrTable->table, pIpAddrTable->dwNumEntries,
             sizeof(MIB_IPADDRROW), IpAddrTableSorter);
          ret = NO_ERROR;
        }
        free(table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IpForwardTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    PMIB_IPFORWARDROW rowA = (PMIB_IPFORWARDROW)a, rowB = (PMIB_IPFORWARDROW)b;

    ret = rowA->dwForwardDest - rowB->dwForwardDest;
    if (ret == 0) {
      ret = rowA->dwForwardProto - rowB->dwForwardProto;
      if (ret == 0) {
        ret = rowA->dwForwardPolicy - rowB->dwForwardPolicy;
        if (ret == 0)
          ret = rowA->dwForwardNextHop - rowB->dwForwardNextHop;
      }
    }
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIpForwardTable (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIpForwardTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIpForwardTable(PMIB_IPFORWARDTABLE pIpForwardTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pIpForwardTable %p, pdwSize %p, bOrder %ld\n", pIpForwardTable,
        pdwSize, (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numRoutes = getNumRoutes();
    ULONG sizeNeeded = sizeof(MIB_IPFORWARDTABLE) + (numRoutes - 1) *
     sizeof(MIB_IPFORWARDROW);

    if (!pIpForwardTable || *pdwSize < sizeNeeded) {
      *pdwSize = sizeNeeded;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      RouteTable *table = getRouteTable();
      if (table) {
        sizeNeeded = sizeof(MIB_IPFORWARDTABLE) + (table->numRoutes - 1) *
         sizeof(MIB_IPFORWARDROW);
        if (*pdwSize < sizeNeeded) {
          *pdwSize = sizeNeeded;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          DWORD ndx;

          pIpForwardTable->dwNumEntries = table->numRoutes;
          for (ndx = 0; ndx < numRoutes; ndx++) {
            pIpForwardTable->table[ndx].dwForwardIfIndex =
             table->routes[ndx].ifIndex;
            pIpForwardTable->table[ndx].dwForwardDest =
             table->routes[ndx].dest;
            pIpForwardTable->table[ndx].dwForwardMask =
             table->routes[ndx].mask;
            pIpForwardTable->table[ndx].dwForwardPolicy = 0;
            pIpForwardTable->table[ndx].dwForwardNextHop =
             table->routes[ndx].gateway;
            /* FIXME: this type is appropriate for local interfaces; may not
               always be appropriate */
            pIpForwardTable->table[ndx].dwForwardType = MIB_IPROUTE_TYPE_DIRECT;
            /* FIXME: other protos might be appropriate, e.g. the default route
               is typically set with MIB_IPPROTO_NETMGMT instead */
            pIpForwardTable->table[ndx].dwForwardProto = MIB_IPPROTO_LOCAL;
            /* punt on age and AS */
            pIpForwardTable->table[ndx].dwForwardAge = 0;
            pIpForwardTable->table[ndx].dwForwardNextHopAS = 0;
            pIpForwardTable->table[ndx].dwForwardMetric1 =
             table->routes[ndx].metric;
            /* rest of the metrics are 0.. */
            pIpForwardTable->table[ndx].dwForwardMetric2 = 0;
            pIpForwardTable->table[ndx].dwForwardMetric3 = 0;
            pIpForwardTable->table[ndx].dwForwardMetric4 = 0;
            pIpForwardTable->table[ndx].dwForwardMetric5 = 0;
          }
          if (bOrder)
            qsort(pIpForwardTable->table, pIpForwardTable->dwNumEntries,
             sizeof(MIB_IPFORWARDROW), IpForwardTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


static int IpNetTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b)
    ret = ((PMIB_IPNETROW)a)->dwAddr - ((PMIB_IPNETROW)b)->dwAddr;
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetIpNetTable (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pIpNetTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIpNetTable(PMIB_IPNETTABLE pIpNetTable, PULONG pdwSize, BOOL bOrder)
{
  DWORD ret = NO_ERROR;

  TRACE("pIpNetTable %p, pdwSize %p, bOrder %d\n", pIpNetTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumArpEntries();
    ULONG size = sizeof(MIB_IPNETTABLE);

    if (numEntries > 1)
      size += (numEntries - 1) * sizeof(MIB_IPNETROW);
    if (!pIpNetTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_IPNETTABLE table = getArpTable();
      if (table) {
        size = sizeof(MIB_IPNETTABLE);
        if (table->dwNumEntries > 1)
          size += (table->dwNumEntries - 1) * sizeof(MIB_IPNETROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          *pdwSize = size;
          memcpy(pIpNetTable, table, size);
          if (bOrder)
            qsort(pIpNetTable->table, pIpNetTable->dwNumEntries,
             sizeof(MIB_IPNETROW), IpNetTableSorter);
          ret = NO_ERROR;
        }
        HeapFree(GetProcessHeap(), 0, table);
      }
    }
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetIpStatistics (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIpStatistics(PMIB_IPSTATS pStats)
{
    return GetIpStatisticsEx(pStats, PF_INET);
}

/******************************************************************
 *    GetIpStatisticsEx (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *  dwFamily [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetIpStatisticsEx(PMIB_IPSTATS pStats, DWORD dwFamily)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getIPStats(pStats, dwFamily);
  TRACE("returning %ld\n", ret);
  return ret;
}

/******************************************************************
 *    GetNetworkParams (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pFixedInfo [In/Out]
 *  pOutBufLen [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetNetworkParams(PFIXED_INFO pFixedInfo, PULONG pOutBufLen)
{
  DWORD ret, size;
  LONG regReturn;
  HKEY hKey;
  PIPHLP_RES_INFO resInfo;

  TRACE("pFixedInfo %p, pOutBufLen %p\n", pFixedInfo, pOutBufLen);
  if (!pOutBufLen)
    return ERROR_INVALID_PARAMETER;

  resInfo = getResInfo();
  if (!resInfo)
    return ERROR_OUTOFMEMORY;

  size = sizeof(FIXED_INFO) + (resInfo->riCount > 1 ? (resInfo->riCount-1) *
   sizeof(IP_ADDR_STRING) : 0);
  if (!pFixedInfo || *pOutBufLen < size) {
    *pOutBufLen = size;
    disposeResInfo( resInfo );
    return ERROR_BUFFER_OVERFLOW;
  }

  memset(pFixedInfo, 0, size);
  size = sizeof(pFixedInfo->HostName);
  GetComputerNameExA(ComputerNameDnsHostname, pFixedInfo->HostName, &size);
  size = sizeof(pFixedInfo->DomainName);
  GetComputerNameExA(ComputerNameDnsDomain, pFixedInfo->DomainName, &size);

  TRACE("GetComputerNameExA: %s\n", pFixedInfo->DomainName);

  if (resInfo->riCount > 0) 
  {
    CopyMemory(&pFixedInfo->DnsServerList, resInfo->DnsList, sizeof(IP_ADDR_STRING));
    if (resInfo->riCount > 1)
    {
      IP_ADDR_STRING *pSrc = resInfo->DnsList->Next;
      IP_ADDR_STRING *pTarget = (struct _IP_ADDR_STRING*)((char*)pFixedInfo + sizeof(FIXED_INFO));

      pFixedInfo->DnsServerList.Next = pTarget;

      do
      {
        CopyMemory(pTarget, pSrc, sizeof(IP_ADDR_STRING));
        resInfo->riCount--;
        if (resInfo->riCount > 1)
        {
          pTarget->Next = (IP_ADDR_STRING*)((char*)pTarget + sizeof(IP_ADDR_STRING));
          pTarget = pTarget->Next;
          pSrc = pSrc->Next;
        }
        else
        {
          pTarget->Next = NULL;
          break;
        }
      }
      while(TRUE);
    }
    else
    {
      pFixedInfo->DnsServerList.Next = NULL;
    }
  }

  pFixedInfo->NodeType = HYBRID_NODETYPE;
  regReturn = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
   "SYSTEM\\CurrentControlSet\\Services\\VxD\\MSTCP", 0, KEY_READ, &hKey);
  if (regReturn != ERROR_SUCCESS)
    regReturn = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
     "SYSTEM\\CurrentControlSet\\Services\\NetBT\\Parameters", 0, KEY_READ,
     &hKey);
  if (regReturn == ERROR_SUCCESS)
  {
    DWORD size = sizeof(pFixedInfo->ScopeId);

    RegQueryValueExA(hKey, "ScopeID", NULL, NULL, (PBYTE)pFixedInfo->ScopeId, &size);
    RegCloseKey(hKey);
  }

  disposeResInfo( resInfo );
  /* FIXME: can check whether routing's enabled in /proc/sys/net/ipv4/ip_forward
     I suppose could also check for a listener on port 53 to set EnableDns */
  ret = NO_ERROR;
  TRACE("returning %ld\n", ret);

  return ret;
}


/******************************************************************
 *    GetNumberOfInterfaces (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pdwNumIf [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetNumberOfInterfaces(PDWORD pdwNumIf)
{
  DWORD ret;

  TRACE("pdwNumIf %p\n", pdwNumIf);
  if (!pdwNumIf)
    ret = ERROR_INVALID_PARAMETER;
  else {
    *pdwNumIf = getNumInterfaces();
    ret = NO_ERROR;
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetOwnerModuleFromTcpEntry (IPHLPAPI.@)
 *
 * Get data about the module that issued the context bind for a specific IPv4 TCP endpoint in a MIB table row
 *
 * PARAMS
 *  pTcpEntry [in]    pointer to a MIB_TCPROW_OWNER_MODULE structure
 *  Class [in]    	TCPIP_OWNER_MODULE_INFO_CLASS enumeration value
 *  Buffer [out]     	pointer a buffer containing a TCPIP_OWNER_MODULE_BASIC_INFO structure with the owner module data. 
 *  pdwSize [in, out]	estimated size of the structure returned in Buffer, in bytes
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: ERROR_INSUFFICIENT_BUFFER, ERROR_INVALID_PARAMETER, ERROR_NOT_ENOUGH_MEMORY
 * 	       ERROR_NOT_FOUND or ERROR_PARTIAL_COPY
 *
 * NOTES
 * The type of data returned in Buffer is indicated by the value of the Class parameter.
 */
DWORD WINAPI GetOwnerModuleFromTcpEntry( PMIB_TCPROW_OWNER_MODULE pTcpEntry, TCPIP_OWNER_MODULE_INFO_CLASS Class, PVOID Buffer, PDWORD pdwSize)
{
	DWORD ret = NO_ERROR;
	UNIMPLEMENTED;
	return ret;	
}


/******************************************************************
 *    GetPerAdapterInfo (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  IfIndex [In]
 *  pPerAdapterInfo [In/Out]
 *  pOutBufLen [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
static void CreateNameServerListEnumNamesFunc( PWCHAR Interface, PWCHAR Server, PVOID Data)
{
  IP_ADDR_STRING *pNext;
  PNAME_SERVER_LIST_CONTEXT Context = (PNAME_SERVER_LIST_CONTEXT)Data;

  if (!Context->NumServers)
  {
    if (Context->uSizeAvailable >= Context->uSizeRequired)
    {
      WideCharToMultiByte(CP_ACP, 0, Server, -1, Context->pData->DnsServerList.IpAddress.String, 16, NULL, NULL);
      Context->pData->DnsServerList.IpAddress.String[15] = '\0';
      Context->pLastAddr = &Context->pData->DnsServerList;
    }
  }
  else
  {
     Context->uSizeRequired += sizeof(IP_ADDR_STRING);
     if (Context->uSizeAvailable >= Context->uSizeRequired)
     {
         pNext = (IP_ADDR_STRING*)(((char*)Context->pLastAddr) + sizeof(IP_ADDR_STRING));
         WideCharToMultiByte(CP_ACP, 0, Server, -1, pNext->IpAddress.String, 16, NULL, NULL);
         pNext->IpAddress.String[15] = '\0';
         Context->pLastAddr->Next = pNext;
         Context->pLastAddr = pNext;
         pNext->Next = NULL;
     }
  }
  Context->NumServers++;
}

DWORD WINAPI GetPerAdapterInfo(ULONG IfIndex, PIP_PER_ADAPTER_INFO pPerAdapterInfo, PULONG pOutBufLen)
{
  HKEY hkey;
  DWORD dwSize = 0;
  const char *ifName;
  NAME_SERVER_LIST_CONTEXT Context;
  WCHAR keyname[200] = L"SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";

  if (!pOutBufLen)
    return ERROR_INVALID_PARAMETER;

  if (!pPerAdapterInfo || *pOutBufLen < sizeof(IP_PER_ADAPTER_INFO))
  {
    *pOutBufLen = sizeof(IP_PER_ADAPTER_INFO);
    return ERROR_BUFFER_OVERFLOW;
  }

  ifName = getInterfaceNameByIndex(IfIndex);
  if (!ifName)
    return ERROR_INVALID_PARAMETER;

  MultiByteToWideChar(CP_ACP, 0, ifName, -1, &keyname[62], sizeof(keyname)/sizeof(WCHAR) - 63);
  HeapFree(GetProcessHeap(), 0, (LPVOID)ifName);

  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, keyname, 0, KEY_READ, &hkey) != ERROR_SUCCESS)
  {
    return ERROR_NOT_SUPPORTED;
  }
  Context.NumServers = 0;
  Context.uSizeAvailable = *pOutBufLen;
  Context.uSizeRequired = sizeof(IP_PER_ADAPTER_INFO);
  Context.pData = pPerAdapterInfo;

  if (*pOutBufLen >= sizeof(IP_PER_ADAPTER_INFO))
    ZeroMemory(pPerAdapterInfo, sizeof(IP_PER_ADAPTER_INFO));

  EnumNameServers(hkey, &keyname[62], &Context, CreateNameServerListEnumNamesFunc);

  if (Context.uSizeRequired > Context.uSizeAvailable)
  {
    *pOutBufLen = Context.uSizeRequired;
    RegCloseKey(hkey);
    return ERROR_BUFFER_OVERFLOW;
  }

  if(RegQueryValueExW(hkey, L"NameServer", NULL, NULL, NULL, &dwSize) == ERROR_SUCCESS)
  {
    pPerAdapterInfo->AutoconfigActive = FALSE;
  }
  else
  {
    pPerAdapterInfo->AutoconfigActive = TRUE;
  }

  RegCloseKey(hkey);
  return NOERROR;
}


/******************************************************************
 *    GetRTTAndHopCount (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  DestIpAddress [In]
 *  HopCount [In/Out]
 *  MaxHops [In]
 *  RTT [In/Out]
 *
 * RETURNS
 *
 *  BOOL
 *
 */
BOOL WINAPI GetRTTAndHopCount(IPAddr DestIpAddress, PULONG HopCount, ULONG MaxHops, PULONG RTT)
{
  TRACE("DestIpAddress 0x%08lx, HopCount %p, MaxHops %ld, RTT %p\n",
   DestIpAddress, HopCount, MaxHops, RTT);
  FIXME(":stub\n");
  return (BOOL) 0;
}


/******************************************************************
 *    GetTcpStatisticsEx (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *  dwFamily [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetTcpStatisticsEx(PMIB_TCPSTATS pStats, DWORD dwFamily)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getTCPStats(pStats, dwFamily);
  TRACE("returning %ld\n", ret);
  return ret;
}

/******************************************************************
 *    GetTcpStatistics (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetTcpStatistics(PMIB_TCPSTATS pStats)
{
    return GetTcpStatisticsEx(pStats, PF_INET);
}


static int TcpTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    PMIB_TCPROW rowA = (PMIB_TCPROW)a, rowB = (PMIB_TCPROW)b;

    ret = rowA->dwLocalAddr - rowB->dwLocalAddr;
    if (ret == 0) {
      ret = rowA->dwLocalPort - rowB->dwLocalPort;
      if (ret == 0) {
        ret = rowA->dwRemoteAddr - rowB->dwRemoteAddr;
        if (ret == 0)
          ret = rowA->dwRemotePort - rowB->dwRemotePort;
      }
    }
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetTcpTable (IPHLPAPI.@)
 *
 * Get the table of active TCP connections.
 *
 * PARAMS
 *  pTcpTable [Out]    buffer for TCP connections table
 *  pdwSize   [In/Out] length of output buffer
 *  bOrder    [In]     whether to order the table
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * NOTES
 *  If pdwSize is less than required, the function will return 
 *  ERROR_INSUFFICIENT_BUFFER, and *pdwSize will be set to 
 *  the required byte size.
 *  If bOrder is true, the returned table will be sorted, first by
 *  local address and port number, then by remote address and port
 *  number.
 */
DWORD WINAPI GetTcpTable(PMIB_TCPTABLE pTcpTable, PDWORD pdwSize, BOOL bOrder)
{
  DWORD ret = ERROR_NO_DATA;

  TRACE("pTcpTable %p, pdwSize %p, bOrder %d\n", pTcpTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumTcpEntries();
    DWORD size = sizeof(MIB_TCPTABLE);

    if (numEntries > 1)
      size += (numEntries - 1) * sizeof(MIB_TCPROW);
    if (!pTcpTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_TCPTABLE pOurTcpTable = getTcpTable();
	  if (pOurTcpTable)
      {
        size = sizeof(MIB_TCPTABLE);
        if (pOurTcpTable->dwNumEntries > 1)
            size += (pOurTcpTable->dwNumEntries - 1) * sizeof(MIB_TCPROW);
          
        if (*pdwSize < size)
        {
            *pdwSize = size;

            ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            memcpy(pTcpTable, pOurTcpTable, size);
            
            if (bOrder)
                qsort(pTcpTable->table, pTcpTable->dwNumEntries,
                      sizeof(MIB_TCPROW), TcpTableSorter);
            
            ret = NO_ERROR;
        }
          
        free(pOurTcpTable);
	  }
   	}
  }
  TRACE("returning %d\n", ret);
  return ret;
}


/******************************************************************
 *    GetUdpStatisticsEx (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *  dwFamily [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetUdpStatisticsEx(PMIB_UDPSTATS pStats, DWORD dwFamily)
{
  DWORD ret;

  TRACE("pStats %p\n", pStats);
  ret = getUDPStats(pStats, dwFamily);
  TRACE("returning %ld\n", ret);
  return ret;
}

/******************************************************************
 *    GetUdpStatistics (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pStats [In/Out]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetUdpStatistics(PMIB_UDPSTATS pStats)
{
    return GetUdpStatisticsEx(pStats, PF_INET);
}


static int UdpTableSorter(const void *a, const void *b)
{
  int ret;

  if (a && b) {
    PMIB_UDPROW rowA = (PMIB_UDPROW)a, rowB = (PMIB_UDPROW)b;

    ret = rowA->dwLocalAddr - rowB->dwLocalAddr;
    if (ret == 0)
      ret = rowA->dwLocalPort - rowB->dwLocalPort;
  }
  else
    ret = 0;
  return ret;
}


/******************************************************************
 *    GetUdpTable (IPHLPAPI.@)
 *
 *
 * PARAMS
 *
 *  pUdpTable [In/Out]
 *  pdwSize [In/Out]
 *  bOrder [In]
 *
 * RETURNS
 *
 *  DWORD
 *
 */
DWORD WINAPI GetUdpTable(PMIB_UDPTABLE pUdpTable, PDWORD pdwSize, BOOL bOrder)
{
  DWORD ret;

  TRACE("pUdpTable %p, pdwSize %p, bOrder %ld\n", pUdpTable, pdwSize,
   (DWORD)bOrder);
  if (!pdwSize)
    ret = ERROR_INVALID_PARAMETER;
  else {
    DWORD numEntries = getNumUdpEntries();
    ULONG size = sizeof(MIB_UDPTABLE) + (numEntries - 1) * sizeof(MIB_UDPROW);

    if (!pUdpTable || *pdwSize < size) {
      *pdwSize = size;
      ret = ERROR_INSUFFICIENT_BUFFER;
    }
    else {
      PMIB_UDPTABLE table = getUdpTable();

      if (table) {
        size = sizeof(MIB_UDPTABLE) + (table->dwNumEntries - 1) *
         sizeof(MIB_UDPROW);
        if (*pdwSize < size) {
          *pdwSize = size;
          ret = ERROR_INSUFFICIENT_BUFFER;
        }
        else {
          memcpy(pUdpTable, table, size);
          if (bOrder)
            qsort(pUdpTable->table, pUdpTable->dwNumEntries,
             sizeof(MIB_UDPROW), UdpTableSorter);
          ret = NO_ERROR;
        }
        free(table);
      }
      else
        ret = ERROR_OUTOFMEMORY;
    }
  }
  TRACE("returning %ld\n", ret);
  return ret;
}


/******************************************************************
 *    GetUniDirectionalAdapterInfo (IPHLPAPI.@)
 *
 * This is a Win98-only function to get information on "unidirectional"
 * adapters.  Since this is pretty nonsensical in other contexts, it
 * never returns anything.
 *
 * PARAMS
 *  pIPIfInfo   [Out] buffer for adapter infos
 *  dwOutBufLen [Out] length of the output buffer
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI GetUniDirectionalAdapterInfo(PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS pIPIfInfo, PULONG dwOutBufLen)
{
  TRACE("pIPIfInfo %p, dwOutBufLen %p\n", pIPIfInfo, dwOutBufLen);
  /* a unidirectional adapter?? not bloody likely! */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    IpReleaseAddress (IPHLPAPI.@)
 *
 * Release an IP obtained through DHCP,
 *
 * PARAMS
 *  AdapterInfo [In] adapter to release IP address
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 */
DWORD WINAPI IpReleaseAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo)
{
  DWORD Status, Version = 0;

  if (!AdapterInfo || !AdapterInfo->Name)
      return ERROR_INVALID_PARAMETER;

  /* Maybe we should do this in DllMain */
  if (DhcpCApiInitialize(&Version) != ERROR_SUCCESS)
      return ERROR_PROC_NOT_FOUND;

  if (DhcpReleaseIpAddressLease(AdapterInfo->Index))
      Status = ERROR_SUCCESS;
  else
      Status = ERROR_PROC_NOT_FOUND;

  DhcpCApiCleanup();

  return Status;
}


/******************************************************************
 *    IpRenewAddress (IPHLPAPI.@)
 *
 * Renew an IP obtained through DHCP.
 *
 * PARAMS
 *  AdapterInfo [In] adapter to renew IP address
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI IpRenewAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo)
{
  DWORD Status, Version = 0;

  if (!AdapterInfo || !AdapterInfo->Name)
      return ERROR_INVALID_PARAMETER;

  /* Maybe we should do this in DllMain */
  if (DhcpCApiInitialize(&Version) != ERROR_SUCCESS)
      return ERROR_PROC_NOT_FOUND;

  if (DhcpRenewIpAddressLease(AdapterInfo->Index))
      Status = ERROR_SUCCESS;
  else
      Status = ERROR_PROC_NOT_FOUND;

  DhcpCApiCleanup();

  return Status;
}


/******************************************************************
 *    NotifyAddrChange (IPHLPAPI.@)
 *
 * Notify caller whenever the ip-interface map is changed.
 *
 * PARAMS
 *  Handle     [Out] handle usable in asynchronous notification
 *  overlapped [In]  overlapped structure that notifies the caller
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI NotifyAddrChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
  FIXME("(Handle %p, overlapped %p): stub\n", Handle, overlapped);
  if (Handle) *Handle = INVALID_HANDLE_VALUE;
  if (overlapped) ((IO_STATUS_BLOCK *) overlapped)->Status = STATUS_PENDING;
  return ERROR_IO_PENDING;
}


/******************************************************************
 *    NotifyRouteChange (IPHLPAPI.@)
 *
 * Notify caller whenever the ip routing table is changed.
 *
 * PARAMS
 *  Handle     [Out] handle usable in asynchronous notification
 *  overlapped [In]  overlapped structure that notifies the caller
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI NotifyRouteChange(PHANDLE Handle, LPOVERLAPPED overlapped)
{
  FIXME("(Handle %p, overlapped %p): stub\n", Handle, overlapped);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SendARP (IPHLPAPI.@)
 *
 * Send an ARP request.
 *
 * PARAMS
 *  DestIP     [In]     attempt to obtain this IP
 *  SrcIP      [In]     optional sender IP address
 *  pMacAddr   [Out]    buffer for the mac address
 *  PhyAddrLen [In/Out] length of the output buffer
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI SendARP(IPAddr DestIP, IPAddr SrcIP, PULONG pMacAddr, PULONG PhyAddrLen)
{
  FIXME("(DestIP 0x%08x, SrcIP 0x%08x, pMacAddr %p, PhyAddrLen %p): stub\n",
   DestIP, SrcIP, pMacAddr, PhyAddrLen);
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SetIfEntry (IPHLPAPI.@)
 *
 * Set the administrative status of an interface.
 *
 * PARAMS
 *  pIfRow [In] dwAdminStatus member specifies the new status.
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI SetIfEntry(PMIB_IFROW pIfRow)
{
  FIXME("(pIfRow %p): stub\n", pIfRow);
  /* this is supposed to set an interface administratively up or down.
     Could do SIOCSIFFLAGS and set/clear IFF_UP, but, not sure I want to, and
     this sort of down is indistinguishable from other sorts of down (e.g. no
     link). */
  return ERROR_NOT_SUPPORTED;
}


/******************************************************************
 *    SetIpForwardEntry (IPHLPAPI.@)
 *
 * Modify an existing route.
 *
 * PARAMS
 *  pRoute [In] route with the new information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 */
DWORD WINAPI SetIpForwardEntry(PMIB_IPFORWARDROW pRoute)
{
    return setIpForwardEntry( pRoute );
}


/******************************************************************
 *    SetIpNetEntry (IPHLPAPI.@)
 *
 * Modify an existing ARP entry.
 *
 * PARAMS
 *  pArpEntry [In] ARP entry with the new information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 */
DWORD WINAPI SetIpNetEntry(PMIB_IPNETROW pArpEntry)
{
  HANDLE tcpFile;
  NTSTATUS status;
  TCP_REQUEST_SET_INFORMATION_EX_ARP_ENTRY req =
      TCP_REQUEST_SET_INFORMATION_INIT;
  TDIEntityID id;
  DWORD returnSize;
  PMIB_IPNETROW arpBuff;

  if (!pArpEntry)
      return ERROR_INVALID_PARAMETER;

  if (!NT_SUCCESS(openTcpFile( &tcpFile, FILE_READ_DATA | FILE_WRITE_DATA )))
      return ERROR_NOT_SUPPORTED;

  if (!NT_SUCCESS(getNthIpEntity( tcpFile, pArpEntry->dwIndex, &id )))
  {
      closeTcpFile(tcpFile);
      return ERROR_INVALID_PARAMETER;
  }

  req.Req.ID.toi_class = INFO_CLASS_PROTOCOL;
  req.Req.ID.toi_type = INFO_TYPE_PROVIDER;
  req.Req.ID.toi_id = IP_MIB_ARPTABLE_ENTRY_ID;
  req.Req.ID.toi_entity.tei_instance = id.tei_instance;
  req.Req.ID.toi_entity.tei_entity = AT_ENTITY;
  req.Req.BufferSize = sizeof(MIB_IPNETROW);
  arpBuff = (PMIB_IPNETROW)&req.Req.Buffer[0];

  RtlCopyMemory(arpBuff, pArpEntry, sizeof(MIB_IPNETROW));

  status = DeviceIoControl( tcpFile,
                            IOCTL_TCP_SET_INFORMATION_EX,
                            &req,
                            sizeof(req),
                            NULL,
                            0,
                            &returnSize,
                            NULL );

  closeTcpFile(tcpFile);

  if (status)
     return NO_ERROR;
  else
     return ERROR_INVALID_PARAMETER;
}


/******************************************************************
 *    SetIpStatistics (IPHLPAPI.@)
 *
 * Toggle IP forwarding and det the default TTL value.
 *
 * PARAMS
 *  pIpStats [In] IP statistics with the new information
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetIpStatistics(PMIB_IPSTATS pIpStats)
{
  FIXME("(pIpStats %p): stub\n", pIpStats);
  return 0;
}


/******************************************************************
 *    SetIpTTL (IPHLPAPI.@)
 *
 * Set the default TTL value.
 *
 * PARAMS
 *  nTTL [In] new TTL value
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetIpTTL(UINT nTTL)
{
  FIXME("(nTTL %d): stub\n", nTTL);
  return 0;
}


/******************************************************************
 *    SetTcpEntry (IPHLPAPI.@)
 *
 * Set the state of a TCP connection.
 *
 * PARAMS
 *  pTcpRow [In] specifies connection with new state
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns NO_ERROR.
 */
DWORD WINAPI SetTcpEntry(PMIB_TCPROW pTcpRow)
{
  FIXME("(pTcpRow %p): stub\n", pTcpRow);
  return 0;
}


/******************************************************************
 *    UnenableRouter (IPHLPAPI.@)
 *
 * Decrement the IP-forwarding reference count. Turn off IP-forwarding
 * if it reaches zero.
 *
 * PARAMS
 *  pOverlapped     [In/Out] should be the same as in EnableRouter()
 *  lpdwEnableCount [Out]    optional, receives reference count
 *
 * RETURNS
 *  Success: NO_ERROR
 *  Failure: error code from winerror.h
 *
 * FIXME
 *  Stub, returns ERROR_NOT_SUPPORTED.
 */
DWORD WINAPI UnenableRouter(OVERLAPPED * pOverlapped, LPDWORD lpdwEnableCount)
{
  FIXME("(pOverlapped %p, lpdwEnableCount %p): stub\n", pOverlapped,
   lpdwEnableCount);
  return ERROR_NOT_SUPPORTED;
}

/*
 * @unimplemented
 */
DWORD WINAPI GetIpErrorString(IP_STATUS ErrorCode,PWCHAR Buffer,PDWORD Size)
{
    FIXME(":stub\n");
    return 0L;
}


/*
 * @unimplemented
 */
PIP_ADAPTER_ORDER_MAP WINAPI GetAdapterOrderMap(VOID)
{
    FIXME(":stub\n");
    return 0L;
}

/*
 * @implemented
 */
#if 0
DWORD WINAPI DECLSPEC_HOTPATCH GetAdaptersAddresses(ULONG Family,ULONG Flags,PVOID Reserved,PIP_ADAPTER_ADDRESSES pAdapterAddresses,PULONG pOutBufLen)
{
    InterfaceIndexTable *indexTable;
    IFInfo ifInfo;
    int i;
    ULONG ret, requiredSize = 0;
    PIP_ADAPTER_ADDRESSES currentAddress;
    PUCHAR currentLocation;
    HANDLE tcpFile;

    if (!pOutBufLen) return ERROR_INVALID_PARAMETER;
    if (Reserved) return ERROR_INVALID_PARAMETER;

    indexTable = getNonLoopbackInterfaceIndexTable(); //I think we want non-loopback here
    if (!indexTable)
        return ERROR_NOT_ENOUGH_MEMORY;

    ret = openTcpFile(&tcpFile, FILE_READ_DATA);
    if (!NT_SUCCESS(ret))
        return ERROR_NO_DATA;

    for (i = indexTable->numIndexes; i >= 0; i--)
    {
        if (NT_SUCCESS(getIPAddrEntryForIf(tcpFile,
                                           NULL,
                                           indexTable->indexes[i],
                                           &ifInfo)))
        {
            /* The whole struct */
            requiredSize += sizeof(IP_ADAPTER_ADDRESSES);

            /* Friendly name */
            if (!(Flags & GAA_FLAG_SKIP_FRIENDLY_NAME))
                requiredSize += strlen((char *)ifInfo.if_info.ent.if_descr) + 1; //FIXME

            /* Adapter name */
            requiredSize += strlen((char *)ifInfo.if_info.ent.if_descr) + 1;

            /* Unicast address */
            if (!(Flags & GAA_FLAG_SKIP_UNICAST))
                requiredSize += sizeof(IP_ADAPTER_UNICAST_ADDRESS);

            /* FIXME: Implement multicast, anycast, and dns server stuff */

            /* FIXME: Implement dns suffix and description */
            requiredSize += 2 * sizeof(WCHAR);

            /* We're only going to implement what's required for XP SP0 */
        }
    }

    if (*pOutBufLen < requiredSize)
    {
        *pOutBufLen = requiredSize;
        closeTcpFile(tcpFile);
        free(indexTable);
        return ERROR_BUFFER_OVERFLOW;
    }

    RtlZeroMemory(pAdapterAddresses, requiredSize);

    /* Let's set up the pointers */
    currentAddress = pAdapterAddresses;
    for (i = indexTable->numIndexes; i >= 0; i--)
    {
        if (NT_SUCCESS(getIPAddrEntryForIf(tcpFile,
                                           NULL,
                                           indexTable->indexes[i],
                                           &ifInfo)))
        {
            currentLocation = (PUCHAR)currentAddress + (ULONG_PTR)sizeof(IP_ADAPTER_ADDRESSES);

            /* FIXME: Friendly name */
            if (!(Flags & GAA_FLAG_SKIP_FRIENDLY_NAME))
            {
                currentAddress->FriendlyName = (PVOID)currentLocation;
                currentLocation += sizeof(WCHAR);
            }

            /* Adapter name */
            currentAddress->AdapterName = (PVOID)currentLocation;
            currentLocation += strlen((char *)ifInfo.if_info.ent.if_descr) + 1;

            /* Unicast address */
            if (!(Flags & GAA_FLAG_SKIP_UNICAST))
            {
                currentAddress->FirstUnicastAddress = (PVOID)currentLocation;
                currentLocation += sizeof(IP_ADAPTER_UNICAST_ADDRESS);
                currentAddress->FirstUnicastAddress->Address.lpSockaddr = (PVOID)currentLocation;
                currentLocation += sizeof(struct sockaddr);
            }

            /* FIXME: Implement multicast, anycast, and dns server stuff */

            /* FIXME: Implement dns suffix and description */
            currentAddress->DnsSuffix = (PVOID)currentLocation;
            currentLocation += sizeof(WCHAR);

            currentAddress->Description = (PVOID)currentLocation;
            currentLocation += sizeof(WCHAR);

            currentAddress->Next = (PVOID)currentLocation;

            /* We're only going to implement what's required for XP SP0 */

            currentAddress = currentAddress->Next;
        }
    }

    /* Terminate the last address correctly */
    if (currentAddress)
        currentAddress->Next = NULL;

    /* Now again, for real this time */

    currentAddress = pAdapterAddresses;
    for (i = indexTable->numIndexes; i >= 0; i--)
    {
        if (NT_SUCCESS(getIPAddrEntryForIf(tcpFile,
                                           NULL,
                                           indexTable->indexes[i],
                                           &ifInfo)))
        {
            /* Make sure we're not looping more than we hoped for */
            ASSERT(currentAddress);

            /* Alignment information */
            currentAddress->Length = sizeof(IP_ADAPTER_ADDRESSES);
            currentAddress->IfIndex = indexTable->indexes[i];

            /* Adapter name */
            strcpy(currentAddress->AdapterName, (char *)ifInfo.if_info.ent.if_descr);

            if (!(Flags & GAA_FLAG_SKIP_UNICAST))
            {
                currentAddress->FirstUnicastAddress->Length = sizeof(IP_ADAPTER_UNICAST_ADDRESS);
                currentAddress->FirstUnicastAddress->Flags = 0; //FIXME
                currentAddress->FirstUnicastAddress->Next = NULL; //FIXME: Support more than one address per adapter
                currentAddress->FirstUnicastAddress->Address.lpSockaddr->sa_family = AF_INET;
                memcpy(currentAddress->FirstUnicastAddress->Address.lpSockaddr->sa_data,
                       &ifInfo.ip_addr.iae_addr,
                       sizeof(ifInfo.ip_addr.iae_addr));
                currentAddress->FirstUnicastAddress->Address.iSockaddrLength = sizeof(ifInfo.ip_addr.iae_addr) + sizeof(USHORT);
                currentAddress->FirstUnicastAddress->PrefixOrigin = IpPrefixOriginOther; //FIXME
                currentAddress->FirstUnicastAddress->SuffixOrigin = IpPrefixOriginOther; //FIXME
                currentAddress->FirstUnicastAddress->DadState = IpDadStatePreferred; //FIXME
                currentAddress->FirstUnicastAddress->ValidLifetime = 0xFFFFFFFF; //FIXME
                currentAddress->FirstUnicastAddress->PreferredLifetime = 0xFFFFFFFF; //FIXME
                currentAddress->FirstUnicastAddress->LeaseLifetime = 0xFFFFFFFF; //FIXME
            }

            /* FIXME: Implement multicast, anycast, and dns server stuff */
            currentAddress->FirstAnycastAddress = NULL;
            currentAddress->FirstMulticastAddress = NULL;
            currentAddress->FirstDnsServerAddress = NULL;

            /* FIXME: Implement dns suffix, description, and friendly name */
            currentAddress->DnsSuffix[0] = UNICODE_NULL;
            currentAddress->Description[0] = UNICODE_NULL;
            currentAddress->FriendlyName[0] = UNICODE_NULL;

            /* Physical Address */
            memcpy(currentAddress->PhysicalAddress, ifInfo.if_info.ent.if_physaddr, ifInfo.if_info.ent.if_physaddrlen);
            currentAddress->PhysicalAddressLength = ifInfo.if_info.ent.if_physaddrlen;

            /* Flags */
            currentAddress->Flags = 0; //FIXME

            /* MTU */
            currentAddress->Mtu = ifInfo.if_info.ent.if_mtu;

            /* Interface type */
            currentAddress->IfType = ifInfo.if_info.ent.if_type;

            /* Operational status */
            currentAddress->OperStatus = ifInfo.if_info.ent.if_operstatus;

            /* We're only going to implement what's required for XP SP0 */

            /* Move to the next address */
            currentAddress = currentAddress->Next;
        }
    }

    closeTcpFile(tcpFile);
    free(indexTable);

    return NO_ERROR;
}
#endif

/*
 * @unimplemented
 */
BOOL WINAPI CancelIPChangeNotify(LPOVERLAPPED notifyOverlapped)
{
    FIXME(":stub\n");
    return 0L;
}

/*
 * @unimplemented
 */
DWORD WINAPI GetBestInterfaceEx(struct sockaddr *pDestAddr,PDWORD pdwBestIfIndex)
{
    FIXME(":stub\n");
    return 0L;
}

/*
 * @unimplemented
 */
DWORD WINAPI NhpAllocateAndGetInterfaceInfoFromStack(IP_INTERFACE_NAME_INFO **ppTable,PDWORD pdwCount,BOOL bOrder,HANDLE hHeap,DWORD dwFlags)
{
    FIXME(":stub\n");
    return 0L;
}

/*
 * @unimplemented
 */
DWORD WINAPI GetIcmpStatisticsEx(PMIB_ICMP_EX pStats,DWORD dwFamily)
{
    FIXME(":stub\n");
    return 0L;
}

DWORD WINAPI
SetIpForwardEntryToStack(PMIB_IPFORWARDROW pRoute)
{
    FIXME("SetIpForwardEntryToStack() stub\n");
    return 0L;
}

DWORD WINAPI
NhGetInterfaceNameFromDeviceGuid(DWORD dwUnknown1,
                                 DWORD dwUnknown2,
                                 DWORD dwUnknown3,
                                 DWORD dwUnknown4,
                                 DWORD dwUnknown5)
{
    FIXME("NhGetInterfaceNameFromDeviceGuid() stub\n");
    return 0L;
}
