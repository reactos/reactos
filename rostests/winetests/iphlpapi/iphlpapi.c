/*
 * iphlpapi dll test
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/*
 * Some observations that an automated test can't produce:
 * An adapter index is a key for an adapter.  That is, if an index is returned
 * from one API, that same index may be used successfully in another API, as
 * long as the adapter remains present.
 * If the adapter is removed and reinserted, however, the index may change (and
 * indeed it does change on Win2K).
 *
 * The Name field of the IP_ADAPTER_INDEX_MAP entries returned by
 * GetInterfaceInfo is declared as a wide string, but the bytes are actually
 * an ASCII string on some versions of the IP helper API under Win9x.  This was
 * apparently an MS bug, it's corrected in later versions.
 *
 * The DomainName field of FIXED_INFO isn't NULL-terminated on Win98.
 */

#include <stdarg.h>
#include "winsock2.h"
#include "windef.h"
#include "winbase.h"
#include "iphlpapi.h"
#include "iprtrmib.h"
#include "wine/test.h"
#include <stdio.h>
#include <stdlib.h>

#undef htonl
#undef htons
#undef ntohl
#undef ntohs

#define htonl(l) ((u_long)(l))
#define htons(s) ((u_short)(s))
#define ntohl(l) ((u_long)(l))
#define ntohs(s) ((u_short)(s))

static HMODULE hLibrary = NULL;

typedef DWORD (WINAPI *GetNumberOfInterfacesFunc)(PDWORD);
typedef DWORD (WINAPI *GetIpAddrTableFunc)(PMIB_IPADDRTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetIfEntryFunc)(PMIB_IFROW);
typedef DWORD (WINAPI *GetFriendlyIfIndexFunc)(DWORD);
typedef DWORD (WINAPI *GetIfTableFunc)(PMIB_IFTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetIpForwardTableFunc)(PMIB_IPFORWARDTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetIpNetTableFunc)(PMIB_IPNETTABLE,PULONG,BOOL);
typedef DWORD (WINAPI *GetInterfaceInfoFunc)(PIP_INTERFACE_INFO,PULONG);
typedef DWORD (WINAPI *GetAdaptersInfoFunc)(PIP_ADAPTER_INFO,PULONG);
typedef DWORD (WINAPI *GetNetworkParamsFunc)(PFIXED_INFO,PULONG);
typedef DWORD (WINAPI *GetIcmpStatisticsFunc)(PMIB_ICMP);
typedef DWORD (WINAPI *GetIpStatisticsFunc)(PMIB_IPSTATS);
typedef DWORD (WINAPI *GetTcpStatisticsFunc)(PMIB_TCPSTATS);
typedef DWORD (WINAPI *GetUdpStatisticsFunc)(PMIB_UDPSTATS);
typedef DWORD (WINAPI *GetTcpTableFunc)(PMIB_TCPTABLE,PDWORD,BOOL);
typedef DWORD (WINAPI *GetUdpTableFunc)(PMIB_UDPTABLE,PDWORD,BOOL);
typedef DWORD (WINAPI *GetPerAdapterInfoFunc)(ULONG,PIP_PER_ADAPTER_INFO,PULONG);
typedef DWORD (WINAPI *GetAdaptersAddressesFunc)(ULONG,ULONG,PVOID,PIP_ADAPTER_ADDRESSES,PULONG);

static GetNumberOfInterfacesFunc gGetNumberOfInterfaces = NULL;
static GetIpAddrTableFunc gGetIpAddrTable = NULL;
static GetIfEntryFunc gGetIfEntry = NULL;
static GetFriendlyIfIndexFunc gGetFriendlyIfIndex = NULL;
static GetIfTableFunc gGetIfTable = NULL;
static GetIpForwardTableFunc gGetIpForwardTable = NULL;
static GetIpNetTableFunc gGetIpNetTable = NULL;
static GetInterfaceInfoFunc gGetInterfaceInfo = NULL;
static GetAdaptersInfoFunc gGetAdaptersInfo = NULL;
static GetNetworkParamsFunc gGetNetworkParams = NULL;
static GetIcmpStatisticsFunc gGetIcmpStatistics = NULL;
static GetIpStatisticsFunc gGetIpStatistics = NULL;
static GetTcpStatisticsFunc gGetTcpStatistics = NULL;
static GetUdpStatisticsFunc gGetUdpStatistics = NULL;
static GetTcpTableFunc gGetTcpTable = NULL;
static GetUdpTableFunc gGetUdpTable = NULL;
static GetPerAdapterInfoFunc gGetPerAdapterInfo = NULL;
static GetAdaptersAddressesFunc gGetAdaptersAddresses = NULL;

static void loadIPHlpApi(void)
{
  hLibrary = LoadLibraryA("iphlpapi.dll");
  if (hLibrary) {
    gGetNumberOfInterfaces = (GetNumberOfInterfacesFunc)GetProcAddress(
     hLibrary, "GetNumberOfInterfaces");
    gGetIpAddrTable = (GetIpAddrTableFunc)GetProcAddress(
     hLibrary, "GetIpAddrTable");
    gGetIfEntry = (GetIfEntryFunc)GetProcAddress(
     hLibrary, "GetIfEntry");
    gGetFriendlyIfIndex = (GetFriendlyIfIndexFunc)GetProcAddress(
     hLibrary, "GetFriendlyIfIndex");
    gGetIfTable = (GetIfTableFunc)GetProcAddress(
     hLibrary, "GetIfTable");
    gGetIpForwardTable = (GetIpForwardTableFunc)GetProcAddress(
     hLibrary, "GetIpForwardTable");
    gGetIpNetTable = (GetIpNetTableFunc)GetProcAddress(
     hLibrary, "GetIpNetTable");
    gGetInterfaceInfo = (GetInterfaceInfoFunc)GetProcAddress(
     hLibrary, "GetInterfaceInfo");
    gGetAdaptersInfo = (GetAdaptersInfoFunc)GetProcAddress(
     hLibrary, "GetAdaptersInfo");
    gGetNetworkParams = (GetNetworkParamsFunc)GetProcAddress(
     hLibrary, "GetNetworkParams");
    gGetIcmpStatistics = (GetIcmpStatisticsFunc)GetProcAddress(
     hLibrary, "GetIcmpStatistics");
    gGetIpStatistics = (GetIpStatisticsFunc)GetProcAddress(
     hLibrary, "GetIpStatistics");
    gGetTcpStatistics = (GetTcpStatisticsFunc)GetProcAddress(
     hLibrary, "GetTcpStatistics");
    gGetUdpStatistics = (GetUdpStatisticsFunc)GetProcAddress(
     hLibrary, "GetUdpStatistics");
    gGetTcpTable = (GetTcpTableFunc)GetProcAddress(
     hLibrary, "GetTcpTable");
    gGetUdpTable = (GetUdpTableFunc)GetProcAddress(
     hLibrary, "GetUdpTable");
    gGetPerAdapterInfo = (GetPerAdapterInfoFunc)GetProcAddress(hLibrary, "GetPerAdapterInfo");
    gGetAdaptersAddresses = (GetAdaptersAddressesFunc)GetProcAddress(hLibrary, "GetAdaptersAddresses");
  }
}

static void freeIPHlpApi(void)
{
  if (hLibrary) {
    gGetNumberOfInterfaces = NULL;
    gGetIpAddrTable = NULL;
    gGetIfEntry = NULL;
    gGetFriendlyIfIndex = NULL;
    gGetIfTable = NULL;
    gGetIpForwardTable = NULL;
    gGetIpNetTable = NULL;
    gGetInterfaceInfo = NULL;
    gGetAdaptersInfo = NULL;
    gGetNetworkParams = NULL;
    gGetIcmpStatistics = NULL;
    gGetIpStatistics = NULL;
    gGetTcpStatistics = NULL;
    gGetUdpStatistics = NULL;
    gGetTcpTable = NULL;
    gGetUdpTable = NULL;
    FreeLibrary(hLibrary);
    hLibrary = NULL;
  }
}

/* replacement for inet_ntoa */
static const char *ntoa( DWORD ip )
{
    static char buffer[40];

    ip = htonl(ip);
    sprintf( buffer, "%u.%u.%u.%u", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff );
    return buffer;
}

/*
still-to-be-tested 98-only functions:
GetUniDirectionalAdapterInfo
*/
static void testWin98OnlyFunctions(void)
{
}

static void testGetNumberOfInterfaces(void)
{
  if (gGetNumberOfInterfaces) {
    DWORD apiReturn, numInterfaces;

    /* Crashes on Vista */
    if (0) {
      apiReturn = gGetNumberOfInterfaces(NULL), numInterfaces;
      if (apiReturn == ERROR_NOT_SUPPORTED)
        return;
      ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetNumberOfInterfaces(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    }

    apiReturn = gGetNumberOfInterfaces(&numInterfaces);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetNumberOfInterfaces is not supported\n");
      return;
    }
    ok(apiReturn == NO_ERROR,
     "GetNumberOfInterfaces returned %d, expected 0\n", apiReturn);
  }
}

static void testGetIfEntry(DWORD index)
{
  if (gGetIfEntry) {
    DWORD apiReturn;
    MIB_IFROW row;

    memset(&row, 0, sizeof(row));
    apiReturn = gGetIfEntry(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIfEntry is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfEntry(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    row.dwIndex = -1; /* hope that's always bogus! */
    apiReturn = gGetIfEntry(&row);
    ok(apiReturn == ERROR_INVALID_DATA ||
     apiReturn == ERROR_FILE_NOT_FOUND /* Vista */,
     "GetIfEntry(bogus row) returned %d, expected ERROR_INVALID_DATA or ERROR_FILE_NOT_FOUND\n",
     apiReturn);
    row.dwIndex = index;
    apiReturn = gGetIfEntry(&row);
    ok(apiReturn == NO_ERROR, 
     "GetIfEntry returned %d, expected NO_ERROR\n", apiReturn);
  }
}

static void testGetIpAddrTable(void)
{
  if (gGetIpAddrTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIpAddrTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpAddrTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpAddrTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpAddrTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpAddrTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPADDRTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = gGetIpAddrTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIpAddrTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
       apiReturn);
      if (apiReturn == NO_ERROR && buf->dwNumEntries)
        testGetIfEntry(buf->table[0].dwIndex);
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetIfTable(void)
{
  if (gGetIfTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIfTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIfTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIfTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIfTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IFTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = gGetIfTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIfTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n\n",
       apiReturn);

      if (apiReturn == NO_ERROR && winetest_debug > 1)
      {
          DWORD i, j;
          char name[MAX_INTERFACE_NAME_LEN];

          trace( "interface table: %u entries\n", buf->dwNumEntries );
          for (i = 0; i < buf->dwNumEntries; i++)
          {
              MIB_IFROW *row = &buf->table[i];
              WideCharToMultiByte( CP_ACP, 0, row->wszName, -1, name, MAX_INTERFACE_NAME_LEN, NULL, NULL );
              trace( "%u: '%s' type %u mtu %u speed %u phys",
                     row->dwIndex, name, row->dwType, row->dwMtu, row->dwSpeed );
              for (j = 0; j < row->dwPhysAddrLen; j++)
                  printf( " %02x", row->bPhysAddr[j] );
              printf( "\n" );
              trace( "        in: bytes %u upkts %u nupkts %u disc %u err %u unk %u\n",
                     row->dwInOctets, row->dwInUcastPkts, row->dwInNUcastPkts,
                     row->dwInDiscards, row->dwInErrors, row->dwInUnknownProtos );
              trace( "        out: bytes %u upkts %u nupkts %u disc %u err %u\n",
                     row->dwOutOctets, row->dwOutUcastPkts, row->dwOutNUcastPkts,
                     row->dwOutDiscards, row->dwOutErrors );
          }
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetIpForwardTable(void)
{
  if (gGetIpForwardTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIpForwardTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpForwardTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpForwardTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpForwardTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpForwardTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPFORWARDTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = gGetIpForwardTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIpForwardTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
       apiReturn);

      if (apiReturn == NO_ERROR && winetest_debug > 1)
      {
          DWORD i;

          trace( "IP forward table: %u entries\n", buf->dwNumEntries );
          for (i = 0; i < buf->dwNumEntries; i++)
          {
              char buffer[40];
              sprintf( buffer, "dest %s", ntoa( buf->table[i].dwForwardDest ));
              sprintf( buffer + strlen(buffer), " mask %s", ntoa( buf->table[i].dwForwardMask ));
              trace( "%u: %s gw %s if %u type %u\n", i, buffer,
                     ntoa( buf->table[i].dwForwardNextHop ),
                     buf->table[i].dwForwardIfIndex, buf->table[i].dwForwardType );
          }
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetIpNetTable(void)
{
  if (gGetIpNetTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetIpNetTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpNetTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpNetTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpNetTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpNetTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_NO_DATA or ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_NO_DATA)
      ; /* empty ARP table's okay */
    else if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPNETTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = gGetIpNetTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR ||
         apiReturn == ERROR_NO_DATA, /* empty ARP table's okay */
       "GetIpNetTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
       apiReturn);

      if (apiReturn == NO_ERROR && winetest_debug > 1)
      {
          DWORD i, j;

          trace( "IP net table: %u entries\n", buf->dwNumEntries );
          for (i = 0; i < buf->dwNumEntries; i++)
          {
              trace( "%u: idx %u type %u addr %s phys",
                     i, buf->table[i].dwIndex, buf->table[i].dwType, ntoa( buf->table[i].dwAddr ));
              for (j = 0; j < buf->table[i].dwPhysAddrLen; j++)
                  printf( " %02x", buf->table[i].bPhysAddr[j] );
              printf( "\n" );
          }
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetIcmpStatistics(void)
{
  if (gGetIcmpStatistics) {
    DWORD apiReturn;
    MIB_ICMP stats;

    /* Crashes on Vista */
    if (0) {
      apiReturn = gGetIcmpStatistics(NULL);
      if (apiReturn == ERROR_NOT_SUPPORTED)
        return;
      ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIcmpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    }

    apiReturn = gGetIcmpStatistics(&stats);
    if (apiReturn == ERROR_NOT_SUPPORTED)
    {
      skip("GetIcmpStatistics is not supported\n");
      return;
    }
    ok(apiReturn == NO_ERROR,
     "GetIcmpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "ICMP stats:          %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:          %8u %8u\n", stats.stats.icmpInStats.dwMsgs, stats.stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:        %8u %8u\n", stats.stats.icmpInStats.dwErrors, stats.stats.icmpOutStats.dwErrors );
        trace( "    dwDestUnreachs:  %8u %8u\n", stats.stats.icmpInStats.dwDestUnreachs, stats.stats.icmpOutStats.dwDestUnreachs );
        trace( "    dwTimeExcds:     %8u %8u\n", stats.stats.icmpInStats.dwTimeExcds, stats.stats.icmpOutStats.dwTimeExcds );
        trace( "    dwParmProbs:     %8u %8u\n", stats.stats.icmpInStats.dwParmProbs, stats.stats.icmpOutStats.dwParmProbs );
        trace( "    dwSrcQuenchs:    %8u %8u\n", stats.stats.icmpInStats.dwSrcQuenchs, stats.stats.icmpOutStats.dwSrcQuenchs );
        trace( "    dwRedirects:     %8u %8u\n", stats.stats.icmpInStats.dwRedirects, stats.stats.icmpOutStats.dwRedirects );
        trace( "    dwEchos:         %8u %8u\n", stats.stats.icmpInStats.dwEchos, stats.stats.icmpOutStats.dwEchos );
        trace( "    dwEchoReps:      %8u %8u\n", stats.stats.icmpInStats.dwEchoReps, stats.stats.icmpOutStats.dwEchoReps );
        trace( "    dwTimestamps:    %8u %8u\n", stats.stats.icmpInStats.dwTimestamps, stats.stats.icmpOutStats.dwTimestamps );
        trace( "    dwTimestampReps: %8u %8u\n", stats.stats.icmpInStats.dwTimestampReps, stats.stats.icmpOutStats.dwTimestampReps );
        trace( "    dwAddrMasks:     %8u %8u\n", stats.stats.icmpInStats.dwAddrMasks, stats.stats.icmpOutStats.dwAddrMasks );
        trace( "    dwAddrMaskReps:  %8u %8u\n", stats.stats.icmpInStats.dwAddrMaskReps, stats.stats.icmpOutStats.dwAddrMaskReps );
    }
  }
}

static void testGetIpStatistics(void)
{
  if (gGetIpStatistics) {
    DWORD apiReturn;
    MIB_IPSTATS stats;

    apiReturn = gGetIpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpStatistics is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetIpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
      "GetIpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP stats:\n" );
        trace( "    dwForwarding:      %u\n", stats.dwForwarding );
        trace( "    dwDefaultTTL:      %u\n", stats.dwDefaultTTL );
        trace( "    dwInReceives:      %u\n", stats.dwInReceives );
        trace( "    dwInHdrErrors:     %u\n", stats.dwInHdrErrors );
        trace( "    dwInAddrErrors:    %u\n", stats.dwInAddrErrors );
        trace( "    dwForwDatagrams:   %u\n", stats.dwForwDatagrams );
        trace( "    dwInUnknownProtos: %u\n", stats.dwInUnknownProtos );
        trace( "    dwInDiscards:      %u\n", stats.dwInDiscards );
        trace( "    dwInDelivers:      %u\n", stats.dwInDelivers );
        trace( "    dwOutRequests:     %u\n", stats.dwOutRequests );
        trace( "    dwRoutingDiscards: %u\n", stats.dwRoutingDiscards );
        trace( "    dwOutDiscards:     %u\n", stats.dwOutDiscards );
        trace( "    dwOutNoRoutes:     %u\n", stats.dwOutNoRoutes );
        trace( "    dwReasmTimeout:    %u\n", stats.dwReasmTimeout );
        trace( "    dwReasmReqds:      %u\n", stats.dwReasmReqds );
        trace( "    dwReasmOks:        %u\n", stats.dwReasmOks );
        trace( "    dwReasmFails:      %u\n", stats.dwReasmFails );
        trace( "    dwFragOks:         %u\n", stats.dwFragOks );
        trace( "    dwFragFails:       %u\n", stats.dwFragFails );
        trace( "    dwFragCreates:     %u\n", stats.dwFragCreates );
        trace( "    dwNumIf:           %u\n", stats.dwNumIf );
        trace( "    dwNumAddr:         %u\n", stats.dwNumAddr );
        trace( "    dwNumRoutes:       %u\n", stats.dwNumRoutes );
    }
  }
}

static void testGetTcpStatistics(void)
{
  if (gGetTcpStatistics) {
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    apiReturn = gGetTcpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetTcpStatistics is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetTcpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetTcpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
      "GetTcpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP stats:\n" );
        trace( "    dwRtoAlgorithm: %u\n", stats.dwRtoAlgorithm );
        trace( "    dwRtoMin:       %u\n", stats.dwRtoMin );
        trace( "    dwRtoMax:       %u\n", stats.dwRtoMax );
        trace( "    dwMaxConn:      %u\n", stats.dwMaxConn );
        trace( "    dwActiveOpens:  %u\n", stats.dwActiveOpens );
        trace( "    dwPassiveOpens: %u\n", stats.dwPassiveOpens );
        trace( "    dwAttemptFails: %u\n", stats.dwAttemptFails );
        trace( "    dwEstabResets:  %u\n", stats.dwEstabResets );
        trace( "    dwCurrEstab:    %u\n", stats.dwCurrEstab );
        trace( "    dwInSegs:       %u\n", stats.dwInSegs );
        trace( "    dwOutSegs:      %u\n", stats.dwOutSegs );
        trace( "    dwRetransSegs:  %u\n", stats.dwRetransSegs );
        trace( "    dwInErrs:       %u\n", stats.dwInErrs );
        trace( "    dwOutRsts:      %u\n", stats.dwOutRsts );
        trace( "    dwNumConns:     %u\n", stats.dwNumConns );
    }
  }
}

static void testGetUdpStatistics(void)
{
  if (gGetUdpStatistics) {
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    apiReturn = gGetUdpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetUdpStatistics is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetUdpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetUdpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
     "GetUdpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP stats:\n" );
        trace( "    dwInDatagrams:  %u\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %u\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %u\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %u\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %u\n", stats.dwNumAddrs );
    }
  }
}

static void testGetTcpTable(void)
{
  if (gGetTcpTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetTcpTable(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetTcpTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER ||
       broken(apiReturn == ERROR_NO_DATA), /* win95 */
     "GetTcpTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_TCPTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = gGetTcpTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetTcpTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
       apiReturn);

      if (apiReturn == NO_ERROR && winetest_debug > 1)
      {
          DWORD i;
          trace( "TCP table: %u entries\n", buf->dwNumEntries );
          for (i = 0; i < buf->dwNumEntries; i++)
          {
              char buffer[40];
              sprintf( buffer, "local %s:%u",
                       ntoa(buf->table[i].dwLocalAddr), ntohs(buf->table[i].dwLocalPort) );
              trace( "%u: %s remote %s:%u state %u\n",
                     i, buffer, ntoa( buf->table[i].dwRemoteAddr ),
                     ntohs(buf->table[i].dwRemotePort), buf->table[i].dwState );
          }
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetUdpTable(void)
{
  if (gGetUdpTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = gGetUdpTable(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetUdpTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetUdpTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_UDPTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = gGetUdpTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetUdpTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
       apiReturn);

      if (apiReturn == NO_ERROR && winetest_debug > 1)
      {
          DWORD i;
          trace( "UDP table: %u entries\n", buf->dwNumEntries );
          for (i = 0; i < buf->dwNumEntries; i++)
              trace( "%u: %s:%u\n",
                     i, ntoa( buf->table[i].dwLocalAddr ), ntohs(buf->table[i].dwLocalPort) );
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

/*
still-to-be-tested NT4-onward functions:
CreateIpForwardEntry
DeleteIpForwardEntry
CreateIpNetEntry
DeleteIpNetEntry
GetFriendlyIfIndex
GetRTTAndHopCount
SetIfEntry
SetIpForwardEntry
SetIpNetEntry
SetIpStatistics
SetIpTTL
SetTcpEntry
*/
static void testWinNT4Functions(void)
{
  testGetNumberOfInterfaces();
  testGetIpAddrTable();
  testGetIfTable();
  testGetIpForwardTable();
  testGetIpNetTable();
  testGetIcmpStatistics();
  testGetIpStatistics();
  testGetTcpStatistics();
  testGetUdpStatistics();
  testGetTcpTable();
  testGetUdpTable();
}

static void testGetInterfaceInfo(void)
{
  if (gGetInterfaceInfo) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = gGetInterfaceInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetInterfaceInfo is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetInterfaceInfo returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetInterfaceInfo(NULL, &len);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetInterfaceInfo returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PIP_INTERFACE_INFO buf = HeapAlloc(GetProcessHeap(), 0, len);

      apiReturn = gGetInterfaceInfo(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetInterfaceInfo(buf, &dwSize) returned %d, expected NO_ERROR\n",
       apiReturn);
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetAdaptersInfo(void)
{
  if (gGetAdaptersInfo) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = gGetAdaptersInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetAdaptersInfo is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetAdaptersInfo returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetAdaptersInfo(NULL, &len);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_BUFFER_OVERFLOW,
     "GetAdaptersInfo returned %d, expected ERROR_NO_DATA or ERROR_BUFFER_OVERFLOW\n",
     apiReturn);
    if (apiReturn == ERROR_NO_DATA)
      ; /* no adapter's, that's okay */
    else if (apiReturn == ERROR_BUFFER_OVERFLOW) {
      PIP_ADAPTER_INFO buf = HeapAlloc(GetProcessHeap(), 0, len);

      apiReturn = gGetAdaptersInfo(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetAdaptersInfo(buf, &dwSize) returned %d, expected NO_ERROR\n",
       apiReturn);
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetNetworkParams(void)
{
  if (gGetNetworkParams) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = gGetNetworkParams(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetNetworkParams is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetNetworkParams returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = gGetNetworkParams(NULL, &len);
    ok(apiReturn == ERROR_BUFFER_OVERFLOW,
     "GetNetworkParams returned %d, expected ERROR_BUFFER_OVERFLOW\n",
     apiReturn);
    if (apiReturn == ERROR_BUFFER_OVERFLOW) {
      PFIXED_INFO buf = HeapAlloc(GetProcessHeap(), 0, len);

      apiReturn = gGetNetworkParams(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetNetworkParams(buf, &dwSize) returned %d, expected NO_ERROR\n",
       apiReturn);
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

/*
still-to-be-tested 98-onward functions:
GetBestInterface
GetBestRoute
IpReleaseAddress
IpRenewAddress
*/
static void testWin98Functions(void)
{
  testGetInterfaceInfo();
  testGetAdaptersInfo();
  testGetNetworkParams();
}

static void testGetPerAdapterInfo(void)
{
    DWORD ret, needed;
    void *buffer;

    if (!gGetPerAdapterInfo) return;
    ret = gGetPerAdapterInfo(1, NULL, NULL);
    if (ret == ERROR_NOT_SUPPORTED) {
      skip("GetPerAdapterInfo is not supported\n");
      return;
    }
    ok( ret == ERROR_INVALID_PARAMETER, "got %u instead of ERROR_INVALID_PARAMETER\n", ret );
    needed = 0xdeadbeef;
    ret = gGetPerAdapterInfo(1, NULL, &needed);
    if (ret == ERROR_NO_DATA) return;  /* no such adapter */
    ok( ret == ERROR_BUFFER_OVERFLOW, "got %u instead of ERROR_BUFFER_OVERFLOW\n", ret );
    ok( needed != 0xdeadbeef, "needed not set\n" );
    buffer = HeapAlloc( GetProcessHeap(), 0, needed );
    ret = gGetPerAdapterInfo(1, buffer, &needed);
    ok( ret == NO_ERROR, "got %u instead of NO_ERROR\n", ret );
    HeapFree( GetProcessHeap(), 0, buffer );
}

/*
still-to-be-tested 2K-onward functions:
AddIPAddress
CreateProxyArpEntry
DeleteIPAddress
DeleteProxyArpEntry
EnableRouter
FlushIpNetTable
GetAdapterIndex
NotifyAddrChange
NotifyRouteChange
SendARP
UnenableRouter
*/
static void testWin2KFunctions(void)
{
    testGetPerAdapterInfo();
}

static void test_GetAdaptersAddresses(void)
{
    ULONG ret, size;
    IP_ADAPTER_ADDRESSES *aa;
    IP_ADAPTER_UNICAST_ADDRESS *ua;

    if (!gGetAdaptersAddresses)
    {
        win_skip("GetAdaptersAddresses not present\n");
        return;
    }

    ret = gGetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", ret);

    ret = gGetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    ok(ret == ERROR_BUFFER_OVERFLOW, "expected ERROR_BUFFER_OVERFLOW, got %u\n", ret);
    if (ret != ERROR_BUFFER_OVERFLOW) return;

    aa = HeapAlloc(GetProcessHeap(), 0, size);
    ret = gGetAdaptersAddresses(AF_UNSPEC, 0, NULL, aa, &size);
    ok(!ret, "expected ERROR_SUCCESS got %u\n", ret);

    while (!ret && winetest_debug > 1 && aa)
    {
        trace("Length:                %u\n", aa->Length);
        trace("IfIndex:               %u\n", aa->IfIndex);
        trace("Next:                  %p\n", aa->Next);
        trace("AdapterName:           %s\n", aa->AdapterName);
        trace("FirstUnicastAddress:   %p\n", aa->FirstUnicastAddress);
        ua = aa->FirstUnicastAddress;
        while (ua)
        {
            trace("\tLength:                  %u\n", ua->Length);
            trace("\tFlags:                   0x%08x\n", ua->Flags);
            trace("\tNext:                    %p\n", ua->Next);
            trace("\tAddress.lpSockaddr:      %p\n", ua->Address.lpSockaddr);
            trace("\tAddress.iSockaddrLength: %d\n", ua->Address.iSockaddrLength);
            trace("\tPrefixOrigin:            %u\n", ua->PrefixOrigin);
            trace("\tSuffixOrigin:            %u\n", ua->SuffixOrigin);
            trace("\tDadState:                %u\n", ua->DadState);
            trace("\tValidLifetime:           0x%08x\n", ua->ValidLifetime);
            trace("\tPreferredLifetime:       0x%08x\n", ua->PreferredLifetime);
            trace("\tLeaseLifetime:           0x%08x\n", ua->LeaseLifetime);
            trace("\n");
            ua = ua->Next;
        }
        trace("FirstAnycastAddress:   %p\n", aa->FirstAnycastAddress);
        trace("FirstMulticastAddress: %p\n", aa->FirstMulticastAddress);
        trace("FirstDnsServerAddress: %p\n", aa->FirstDnsServerAddress);
        trace("DnsSuffix:             %p\n", aa->DnsSuffix);
        trace("Description:           %p\n", aa->Description);
        trace("FriendlyName:          %p\n", aa->FriendlyName);
        trace("PhysicalAddress:       %02x\n", aa->PhysicalAddress[0]);
        trace("PhysicalAddressLength: %u\n", aa->PhysicalAddressLength);
        trace("Flags:                 0x%08x\n", aa->Flags);
        trace("Mtu:                   %u\n", aa->Mtu);
        trace("IfType:                %u\n", aa->IfType);
        trace("OperStatus:            %u\n", aa->OperStatus);
        trace("\n");
        aa = aa->Next;
    }
    HeapFree(GetProcessHeap(), 0, aa);
}

START_TEST(iphlpapi)
{

  loadIPHlpApi();
  if (hLibrary) {
    testWin98OnlyFunctions();
    testWinNT4Functions();
    testWin98Functions();
    testWin2KFunctions();
    test_GetAdaptersAddresses();
    freeIPHlpApi();
  }
}
