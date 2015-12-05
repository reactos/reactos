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
#include "ws2tcpip.h"
#include "iphlpapi.h"
#include "iprtrmib.h"
#include "netioapi.h"
#include "wine/test.h"
#include <stdio.h>
#include <stdlib.h>

#define ICMP_MINLEN 8 /* copied from dlls/iphlpapi/ip_icmp.h file */

static HMODULE hLibrary = NULL;

static DWORD (WINAPI *pGetNumberOfInterfaces)(PDWORD);
static DWORD (WINAPI *pGetIpAddrTable)(PMIB_IPADDRTABLE,PULONG,BOOL);
static DWORD (WINAPI *pGetIfEntry)(PMIB_IFROW);
static DWORD (WINAPI *pGetIfEntry2)(PMIB_IF_ROW2);
static DWORD (WINAPI *pGetFriendlyIfIndex)(DWORD);
static DWORD (WINAPI *pGetIfTable)(PMIB_IFTABLE,PULONG,BOOL);
static DWORD (WINAPI *pGetIfTable2)(PMIB_IF_TABLE2*);
static DWORD (WINAPI *pGetIpForwardTable)(PMIB_IPFORWARDTABLE,PULONG,BOOL);
static DWORD (WINAPI *pGetIpNetTable)(PMIB_IPNETTABLE,PULONG,BOOL);
static DWORD (WINAPI *pGetInterfaceInfo)(PIP_INTERFACE_INFO,PULONG);
static DWORD (WINAPI *pGetAdaptersInfo)(PIP_ADAPTER_INFO,PULONG);
static DWORD (WINAPI *pGetNetworkParams)(PFIXED_INFO,PULONG);
static DWORD (WINAPI *pGetIcmpStatistics)(PMIB_ICMP);
static DWORD (WINAPI *pGetIpStatistics)(PMIB_IPSTATS);
static DWORD (WINAPI *pGetTcpStatistics)(PMIB_TCPSTATS);
static DWORD (WINAPI *pGetUdpStatistics)(PMIB_UDPSTATS);
static DWORD (WINAPI *pGetIcmpStatisticsEx)(PMIB_ICMP_EX,DWORD);
static DWORD (WINAPI *pGetIpStatisticsEx)(PMIB_IPSTATS,DWORD);
static DWORD (WINAPI *pGetTcpStatisticsEx)(PMIB_TCPSTATS,DWORD);
static DWORD (WINAPI *pGetUdpStatisticsEx)(PMIB_UDPSTATS,DWORD);
static DWORD (WINAPI *pGetTcpTable)(PMIB_TCPTABLE,PDWORD,BOOL);
static DWORD (WINAPI *pGetUdpTable)(PMIB_UDPTABLE,PDWORD,BOOL);
static DWORD (WINAPI *pGetPerAdapterInfo)(ULONG,PIP_PER_ADAPTER_INFO,PULONG);
static DWORD (WINAPI *pGetAdaptersAddresses)(ULONG,ULONG,PVOID,PIP_ADAPTER_ADDRESSES,PULONG);
static DWORD (WINAPI *pNotifyAddrChange)(PHANDLE,LPOVERLAPPED);
static BOOL  (WINAPI *pCancelIPChangeNotify)(LPOVERLAPPED);
static DWORD (WINAPI *pGetExtendedTcpTable)(PVOID,PDWORD,BOOL,ULONG,TCP_TABLE_CLASS,ULONG);
static DWORD (WINAPI *pGetExtendedUdpTable)(PVOID,PDWORD,BOOL,ULONG,UDP_TABLE_CLASS,ULONG);
static DWORD (WINAPI *pSetTcpEntry)(PMIB_TCPROW);
static HANDLE(WINAPI *pIcmpCreateFile)(VOID);
static DWORD (WINAPI *pIcmpSendEcho)(HANDLE,IPAddr,LPVOID,WORD,PIP_OPTION_INFORMATION,LPVOID,DWORD,DWORD);
static DWORD (WINAPI *pCreateSortedAddressPairs)(const PSOCKADDR_IN6,ULONG,const PSOCKADDR_IN6,ULONG,ULONG,
                                                 PSOCKADDR_IN6_PAIR*,ULONG*);
static void (WINAPI *pFreeMibTable)(void*);
static DWORD (WINAPI *pConvertInterfaceGuidToLuid)(const GUID*,NET_LUID*);
static DWORD (WINAPI *pConvertInterfaceIndexToLuid)(NET_IFINDEX,NET_LUID*);
static DWORD (WINAPI *pConvertInterfaceLuidToGuid)(const NET_LUID*,GUID*);
static DWORD (WINAPI *pConvertInterfaceLuidToIndex)(const NET_LUID*,NET_IFINDEX*);
static DWORD (WINAPI *pConvertInterfaceLuidToNameW)(const NET_LUID*,WCHAR*,SIZE_T);
static DWORD (WINAPI *pConvertInterfaceLuidToNameA)(const NET_LUID*,char*,SIZE_T);
static DWORD (WINAPI *pConvertInterfaceNameToLuidA)(const char*,NET_LUID*);
static DWORD (WINAPI *pConvertInterfaceNameToLuidW)(const WCHAR*,NET_LUID*);

static void loadIPHlpApi(void)
{
  hLibrary = LoadLibraryA("iphlpapi.dll");
  if (hLibrary) {
    pGetNumberOfInterfaces = (void *)GetProcAddress(hLibrary, "GetNumberOfInterfaces");
    pGetIpAddrTable = (void *)GetProcAddress(hLibrary, "GetIpAddrTable");
    pGetIfEntry = (void *)GetProcAddress(hLibrary, "GetIfEntry");
    pGetIfEntry2 = (void *)GetProcAddress(hLibrary, "GetIfEntry2");
    pGetFriendlyIfIndex = (void *)GetProcAddress(hLibrary, "GetFriendlyIfIndex");
    pGetIfTable = (void *)GetProcAddress(hLibrary, "GetIfTable");
    pGetIfTable2 = (void *)GetProcAddress(hLibrary, "GetIfTable2");
    pGetIpForwardTable = (void *)GetProcAddress(hLibrary, "GetIpForwardTable");
    pGetIpNetTable = (void *)GetProcAddress(hLibrary, "GetIpNetTable");
    pGetInterfaceInfo = (void *)GetProcAddress(hLibrary, "GetInterfaceInfo");
    pGetAdaptersInfo = (void *)GetProcAddress(hLibrary, "GetAdaptersInfo");
    pGetNetworkParams = (void *)GetProcAddress(hLibrary, "GetNetworkParams");
    pGetIcmpStatistics = (void *)GetProcAddress(hLibrary, "GetIcmpStatistics");
    pGetIpStatistics = (void *)GetProcAddress(hLibrary, "GetIpStatistics");
    pGetTcpStatistics = (void *)GetProcAddress(hLibrary, "GetTcpStatistics");
    pGetUdpStatistics = (void *)GetProcAddress(hLibrary, "GetUdpStatistics");
    pGetIcmpStatisticsEx = (void *)GetProcAddress(hLibrary, "GetIcmpStatisticsEx");
    pGetIpStatisticsEx = (void *)GetProcAddress(hLibrary, "GetIpStatisticsEx");
    pGetTcpStatisticsEx = (void *)GetProcAddress(hLibrary, "GetTcpStatisticsEx");
    pGetUdpStatisticsEx = (void *)GetProcAddress(hLibrary, "GetUdpStatisticsEx");
    pGetTcpTable = (void *)GetProcAddress(hLibrary, "GetTcpTable");
    pGetUdpTable = (void *)GetProcAddress(hLibrary, "GetUdpTable");
    pGetPerAdapterInfo = (void *)GetProcAddress(hLibrary, "GetPerAdapterInfo");
    pGetAdaptersAddresses = (void *)GetProcAddress(hLibrary, "GetAdaptersAddresses");
    pNotifyAddrChange = (void *)GetProcAddress(hLibrary, "NotifyAddrChange");
    pCancelIPChangeNotify = (void *)GetProcAddress(hLibrary, "CancelIPChangeNotify");
    pGetExtendedTcpTable = (void *)GetProcAddress(hLibrary, "GetExtendedTcpTable");
    pGetExtendedUdpTable = (void *)GetProcAddress(hLibrary, "GetExtendedUdpTable");
    pSetTcpEntry = (void *)GetProcAddress(hLibrary, "SetTcpEntry");
    pIcmpCreateFile = (void *)GetProcAddress(hLibrary, "IcmpCreateFile");
    pIcmpSendEcho = (void *)GetProcAddress(hLibrary, "IcmpSendEcho");
    pCreateSortedAddressPairs = (void *)GetProcAddress(hLibrary, "CreateSortedAddressPairs");
    pFreeMibTable = (void *)GetProcAddress(hLibrary, "FreeMibTable");
    pConvertInterfaceGuidToLuid = (void *)GetProcAddress(hLibrary, "ConvertInterfaceGuidToLuid");
    pConvertInterfaceIndexToLuid = (void *)GetProcAddress(hLibrary, "ConvertInterfaceIndexToLuid");
    pConvertInterfaceLuidToGuid = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToGuid");
    pConvertInterfaceLuidToIndex = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToIndex");
    pConvertInterfaceLuidToNameA = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToNameA");
    pConvertInterfaceLuidToNameW = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToNameW");
    pConvertInterfaceNameToLuidA = (void *)GetProcAddress(hLibrary, "ConvertInterfaceNameToLuidA");
    pConvertInterfaceNameToLuidW = (void *)GetProcAddress(hLibrary, "ConvertInterfaceNameToLuidW");
  }
}

static void freeIPHlpApi(void)
{
    FreeLibrary(hLibrary);
}

/* replacement for inet_ntoa */
static const char *ntoa( DWORD ip )
{
    static char buffer[40];

    ip = htonl(ip);
    sprintf( buffer, "%u.%u.%u.%u", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff );
    return buffer;
}

static inline const char* debugstr_longlong(ULONGLONG ll)
{
    static char string[17];
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(string, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll);
    else
        sprintf(string, "%lx", (unsigned long)ll);
    return string;
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
  if (pGetNumberOfInterfaces) {
    DWORD apiReturn, numInterfaces;

    /* Crashes on Vista */
    if (0) {
      apiReturn = pGetNumberOfInterfaces(NULL);
      if (apiReturn == ERROR_NOT_SUPPORTED)
        return;
      ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetNumberOfInterfaces(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    }

    apiReturn = pGetNumberOfInterfaces(&numInterfaces);
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
  if (pGetIfEntry) {
    DWORD apiReturn;
    MIB_IFROW row;

    memset(&row, 0, sizeof(row));
    apiReturn = pGetIfEntry(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIfEntry is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfEntry(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    row.dwIndex = -1; /* hope that's always bogus! */
    apiReturn = pGetIfEntry(&row);
    ok(apiReturn == ERROR_INVALID_DATA ||
     apiReturn == ERROR_FILE_NOT_FOUND /* Vista */,
     "GetIfEntry(bogus row) returned %d, expected ERROR_INVALID_DATA or ERROR_FILE_NOT_FOUND\n",
     apiReturn);
    row.dwIndex = index;
    apiReturn = pGetIfEntry(&row);
    ok(apiReturn == NO_ERROR, 
     "GetIfEntry returned %d, expected NO_ERROR\n", apiReturn);
  }
}

static void testGetIpAddrTable(void)
{
  if (pGetIpAddrTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = pGetIpAddrTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpAddrTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpAddrTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetIpAddrTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpAddrTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPADDRTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = pGetIpAddrTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIpAddrTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
       apiReturn);
      if (apiReturn == NO_ERROR && buf->dwNumEntries)
      {
        int i;
        testGetIfEntry(buf->table[0].dwIndex);
        for (i = 0; i < buf->dwNumEntries; i++)
        {
          ok (buf->table[i].wType != 0, "Test[%d]: expected wType > 0\n", i);
          trace("Entry[%d]: addr %s, dwIndex %u, wType 0x%x\n", i,
                ntoa(buf->table[i].dwAddr), buf->table[i].dwIndex, buf->table[i].wType);
        }
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetIfTable(void)
{
  if (pGetIfTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = pGetIfTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIfTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetIfTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIfTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IFTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = pGetIfTable(buf, &dwSize, FALSE);
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
  if (pGetIpForwardTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = pGetIpForwardTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpForwardTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpForwardTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetIpForwardTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpForwardTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPFORWARDTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = pGetIpForwardTable(buf, &dwSize, FALSE);
      ok(apiReturn == NO_ERROR,
       "GetIpForwardTable(buf, &dwSize, FALSE) returned %d, expected NO_ERROR\n",
       apiReturn);

      if (apiReturn == NO_ERROR && winetest_debug > 1)
      {
          DWORD i;

          trace( "IP forward table: %u entries\n", buf->dwNumEntries );
          for (i = 0; i < buf->dwNumEntries; i++)
          {
              char buffer[100];
              sprintf( buffer, "dest %s", ntoa( buf->table[i].dwForwardDest ));
              sprintf( buffer + strlen(buffer), " mask %s", ntoa( buf->table[i].dwForwardMask ));
              trace( "%u: %s gw %s if %u type %u\n", i, buffer,
                     ntoa( buf->table[i].dwForwardNextHop ),
                     buf->table[i].dwForwardIfIndex, U1(buf->table[i]).dwForwardType );
          }
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetIpNetTable(void)
{
  if (pGetIpNetTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = pGetIpNetTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpNetTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpNetTable(NULL, NULL, FALSE) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetIpNetTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetIpNetTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_NO_DATA or ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_NO_DATA)
      ; /* empty ARP table's okay */
    else if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_IPNETTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = pGetIpNetTable(buf, &dwSize, FALSE);
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
                     i, buf->table[i].dwIndex, U(buf->table[i]).dwType, ntoa( buf->table[i].dwAddr ));
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
  if (pGetIcmpStatistics) {
    DWORD apiReturn;
    MIB_ICMP stats;

    /* Crashes on Vista */
    if (0) {
      apiReturn = pGetIcmpStatistics(NULL);
      if (apiReturn == ERROR_NOT_SUPPORTED)
        return;
      ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIcmpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    }

    apiReturn = pGetIcmpStatistics(&stats);
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
  if (pGetIpStatistics) {
    DWORD apiReturn;
    MIB_IPSTATS stats;

    apiReturn = pGetIpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIpStatistics is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetIpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
      "GetIpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP stats:\n" );
        trace( "    dwForwarding:      %u\n", U(stats).dwForwarding );
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
  if (pGetTcpStatistics) {
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    apiReturn = pGetTcpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetTcpStatistics is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetTcpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetTcpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
      "GetTcpStatistics returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP stats:\n" );
        trace( "    dwRtoAlgorithm: %u\n", U(stats).dwRtoAlgorithm );
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
  if (pGetUdpStatistics) {
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    apiReturn = pGetUdpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetUdpStatistics is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetUdpStatistics(NULL) returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetUdpStatistics(&stats);
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

static void testGetIcmpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_ICMP_EX stats;

    if (!pGetIcmpStatisticsEx)
    {
        win_skip( "GetIcmpStatisticsEx not available\n" );
        return;
    }

    /* Crashes on Vista */
    if (1) {
        apiReturn = pGetIcmpStatisticsEx(NULL, AF_INET);
        ok(apiReturn == ERROR_INVALID_PARAMETER,
         "GetIcmpStatisticsEx(NULL, AF_INET) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);
    }

    apiReturn = pGetIcmpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIcmpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = pGetIcmpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetIcmpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        INT i;
        trace( "ICMP IPv4 Ex stats:           %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:              %8u %8u\n", stats.icmpInStats.dwMsgs, stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:            %8u %8u\n", stats.icmpInStats.dwErrors, stats.icmpOutStats.dwErrors );
        for (i = 0; i < 256; i++)
            trace( "    rgdwTypeCount[%3i]: %8u %8u\n", i, stats.icmpInStats.rgdwTypeCount[i], stats.icmpOutStats.rgdwTypeCount[i] );
    }

    apiReturn = pGetIcmpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetIcmpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        INT i;
        trace( "ICMP IPv6 Ex stats:           %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:              %8u %8u\n", stats.icmpInStats.dwMsgs, stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:            %8u %8u\n", stats.icmpInStats.dwErrors, stats.icmpOutStats.dwErrors );
        for (i = 0; i < 256; i++)
            trace( "    rgdwTypeCount[%3i]: %8u %8u\n", i, stats.icmpInStats.rgdwTypeCount[i], stats.icmpOutStats.rgdwTypeCount[i] );
    }
}

static void testGetIpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_IPSTATS stats;

    if (!pGetIpStatisticsEx)
    {
        win_skip( "GetIpStatisticsEx not available\n" );
        return;
    }

    apiReturn = pGetIpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatisticsEx(NULL, AF_INET) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = pGetIpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = pGetIpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetIpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP IPv4 Ex stats:\n" );
        trace( "    dwForwarding:      %u\n", U(stats).dwForwarding );
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

    apiReturn = pGetIpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetIpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP IPv6 Ex stats:\n" );
        trace( "    dwForwarding:      %u\n", U(stats).dwForwarding );
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

static void testGetTcpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    if (!pGetTcpStatisticsEx)
    {
        win_skip( "GetTcpStatisticsEx not available\n" );
        return;
    }

    apiReturn = pGetTcpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetTcpStatisticsEx(NULL, AF_INET); returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = pGetTcpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER || apiReturn == ERROR_NOT_SUPPORTED,
       "GetTcpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = pGetTcpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetTcpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP IPv4 Ex stats:\n" );
        trace( "    dwRtoAlgorithm: %u\n", U(stats).dwRtoAlgorithm );
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

    apiReturn = pGetTcpStatisticsEx(&stats, AF_INET6);
    todo_wine ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
                 "GetTcpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP IPv6 Ex stats:\n" );
        trace( "    dwRtoAlgorithm: %u\n", U(stats).dwRtoAlgorithm );
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

static void testGetUdpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    if (!pGetUdpStatisticsEx)
    {
        win_skip( "GetUdpStatisticsEx not available\n" );
        return;
    }

    apiReturn = pGetUdpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetUdpStatisticsEx(NULL, AF_INET); returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = pGetUdpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER || apiReturn == ERROR_NOT_SUPPORTED,
       "GetUdpStatisticsEx(&stats, AF_BAN) returned %d, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = pGetUdpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetUdpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP IPv4 Ex stats:\n" );
        trace( "    dwInDatagrams:  %u\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %u\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %u\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %u\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %u\n", stats.dwNumAddrs );
    }

    apiReturn = pGetUdpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetUdpStatisticsEx returned %d, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP IPv6 Ex stats:\n" );
        trace( "    dwInDatagrams:  %u\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %u\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %u\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %u\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %u\n", stats.dwNumAddrs );
    }
}

static void testGetTcpTable(void)
{
  if (pGetTcpTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = pGetTcpTable(NULL, &dwSize, FALSE);
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

      apiReturn = pGetTcpTable(buf, &dwSize, FALSE);
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
                     ntohs(buf->table[i].dwRemotePort), U(buf->table[i]).dwState );
          }
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetUdpTable(void)
{
  if (pGetUdpTable) {
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = pGetUdpTable(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetUdpTable is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetUdpTable(NULL, &dwSize, FALSE) returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PMIB_UDPTABLE buf = HeapAlloc(GetProcessHeap(), 0, dwSize);

      apiReturn = pGetUdpTable(buf, &dwSize, FALSE);
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

static void testSetTcpEntry(void)
{
    DWORD ret;
    MIB_TCPROW row;

    memset(&row, 0, sizeof(row));
    if(0) /* This test crashes in OS >= VISTA */
    {
        ret = pSetTcpEntry(NULL);
        ok( ret == ERROR_INVALID_PARAMETER, "got %u, expected %u\n", ret, ERROR_INVALID_PARAMETER);
    }

    ret = pSetTcpEntry(&row);
    if (ret == ERROR_NETWORK_ACCESS_DENIED)
    {
        win_skip("SetTcpEntry failed with access error. Skipping test.\n");
        return;
    }
    todo_wine ok( ret == ERROR_INVALID_PARAMETER, "got %u, expected %u\n", ret, ERROR_INVALID_PARAMETER);

    U(row).dwState = MIB_TCP_STATE_DELETE_TCB;
    ret = pSetTcpEntry(&row);
    todo_wine ok( ret == ERROR_MR_MID_NOT_FOUND || broken(ret == ERROR_INVALID_PARAMETER),
       "got %u, expected %u\n", ret, ERROR_MR_MID_NOT_FOUND);
}

static void testIcmpSendEcho(void)
{
    HANDLE icmp;
    char senddata[32], replydata[sizeof(senddata) + sizeof(ICMP_ECHO_REPLY)];
    DWORD ret, error, replysz = sizeof(replydata);
    IPAddr address;

    if (!pIcmpSendEcho || !pIcmpCreateFile)
    {
        win_skip( "IcmpSendEcho or IcmpCreateFile not available\n" );
        return;
    }
    memset(senddata, 0, sizeof(senddata));

    address = htonl(INADDR_LOOPBACK);
    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(INVALID_HANDLE_VALUE, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
todo_wine
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INVALID_HANDLE) /* <= 2003 */,
        "expected 87, got %d\n", error);

    icmp = pIcmpCreateFile();
    if (icmp == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        if (error == ERROR_ACCESS_DENIED)
        {
            skip ("ICMP is not available.\n");
            return;
        }
    }
    ok (icmp != INVALID_HANDLE_VALUE, "IcmpCreateFile failed unexpectedly with error %d\n", GetLastError());

    address = 0;
    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_NETNAME
        || broken(error == IP_BAD_DESTINATION) /* <= 2003 */,
        "expected 1214, got %d\n", error);

    address = htonl(INADDR_LOOPBACK);
    if (0) /* crashes in XP */
    {
        ret = pIcmpSendEcho(icmp, address, NULL, sizeof(senddata), NULL, replydata, replysz, 1000);
        ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    }

    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, senddata, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, NULL, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    if (0) /* crashes in wine, remove IF when fixed */
    {
    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, NULL, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER, "expected 87, got %d\n", error);
    }

    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, 0, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
todo_wine
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %d\n", error);

    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, NULL, 0, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
todo_wine
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %d\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata) - 1;
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    todo_wine {
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %d\n", error);
    }

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY);
    ret = pIcmpSendEcho(icmp, address, senddata, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
todo_wine
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY) + ICMP_MINLEN;
    ret = pIcmpSendEcho(icmp, address, senddata, ICMP_MINLEN, NULL, replydata, replysz, 1000);
    error = GetLastError();
todo_wine
    ok (ret, "IcmpSendEcho failed unexpectedly with error %d\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY) + ICMP_MINLEN;
    ret = pIcmpSendEcho(icmp, address, senddata, ICMP_MINLEN + 1, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
todo_wine
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %d\n", error);

    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, senddata, ICMP_MINLEN, NULL, replydata, replysz - 1, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
todo_wine
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %d\n", error);

    /* in windows >= vista the timeout can't be invalid */
    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata);
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 0);
    error = GetLastError();
    if (!ret) ok(error == ERROR_INVALID_PARAMETER, "expected 87, got %d\n", error);

    SetLastError(0xdeadbeef);
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, -1);
    error = GetLastError();
    if (!ret) ok(error == ERROR_INVALID_PARAMETER, "expected 87, got %d\n", error);

    /* real ping test */
    SetLastError(0xdeadbeef);
    address = htonl(INADDR_LOOPBACK);
    ret = pIcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    if (ret)
    {
        PICMP_ECHO_REPLY pong = (PICMP_ECHO_REPLY) replydata;
        trace ("send addr  : %s\n", ntoa(address));
        trace ("reply addr : %s\n", ntoa(pong->Address));
        trace ("reply size : %u\n", replysz);
        trace ("roundtrip  : %u ms\n", pong->RoundTripTime);
        trace ("status     : %u\n", pong->Status);
        trace ("recv size  : %u\n", pong->DataSize);
        trace ("ttl        : %u\n", pong->Options.Ttl);
        trace ("flags      : 0x%x\n", pong->Options.Flags);
    }
    else
    {
        skip ("Failed to ping with error %d, is lo interface down?.\n", error);
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
  testGetIcmpStatisticsEx();
  testGetIpStatisticsEx();
  testGetTcpStatisticsEx();
  testGetUdpStatisticsEx();
  testGetTcpTable();
  testGetUdpTable();
  testSetTcpEntry();
  testIcmpSendEcho();
}

static void testGetInterfaceInfo(void)
{
  if (pGetInterfaceInfo) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = pGetInterfaceInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetInterfaceInfo is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetInterfaceInfo returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetInterfaceInfo(NULL, &len);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
     "GetInterfaceInfo returned %d, expected ERROR_INSUFFICIENT_BUFFER\n",
     apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
      PIP_INTERFACE_INFO buf = HeapAlloc(GetProcessHeap(), 0, len);

      apiReturn = pGetInterfaceInfo(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetInterfaceInfo(buf, &dwSize) returned %d, expected NO_ERROR\n",
       apiReturn);
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetAdaptersInfo(void)
{
  if (pGetAdaptersInfo) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = pGetAdaptersInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetAdaptersInfo is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetAdaptersInfo returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetAdaptersInfo(NULL, &len);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_BUFFER_OVERFLOW,
     "GetAdaptersInfo returned %d, expected ERROR_NO_DATA or ERROR_BUFFER_OVERFLOW\n",
     apiReturn);
    if (apiReturn == ERROR_NO_DATA)
      ; /* no adapter's, that's okay */
    else if (apiReturn == ERROR_BUFFER_OVERFLOW) {
      PIP_ADAPTER_INFO ptr, buf = HeapAlloc(GetProcessHeap(), 0, len);

      apiReturn = pGetAdaptersInfo(buf, &len);
      ok(apiReturn == NO_ERROR,
       "GetAdaptersInfo(buf, &dwSize) returned %d, expected NO_ERROR\n",
       apiReturn);
      ptr = buf;
      while (ptr) {
        ok(ptr->IpAddressList.IpAddress.String[0], "A valid IP must be present\n");
        ok(ptr->IpAddressList.IpMask.String[0], "A valid mask must be present\n");
        trace("Adapter '%s', IP %s, Mask %s\n", ptr->AdapterName,
              ptr->IpAddressList.IpAddress.String, ptr->IpAddressList.IpMask.String);
        ptr = ptr->Next;
      }
      HeapFree(GetProcessHeap(), 0, buf);
    }
  }
}

static void testGetNetworkParams(void)
{
  if (pGetNetworkParams) {
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = pGetNetworkParams(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetNetworkParams is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetNetworkParams returned %d, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    apiReturn = pGetNetworkParams(NULL, &len);
    ok(apiReturn == ERROR_BUFFER_OVERFLOW,
     "GetNetworkParams returned %d, expected ERROR_BUFFER_OVERFLOW\n",
     apiReturn);
    if (apiReturn == ERROR_BUFFER_OVERFLOW) {
      PFIXED_INFO buf = HeapAlloc(GetProcessHeap(), 0, len);

      apiReturn = pGetNetworkParams(buf, &len);
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
static DWORD CALLBACK testWin98Functions(void *p)
{
  testGetInterfaceInfo();
  testGetAdaptersInfo();
  testGetNetworkParams();
  return 0;
}

static void testGetPerAdapterInfo(void)
{
    DWORD ret, needed;
    void *buffer;

    if (!pGetPerAdapterInfo) return;
    ret = pGetPerAdapterInfo(1, NULL, NULL);
    if (ret == ERROR_NOT_SUPPORTED) {
      skip("GetPerAdapterInfo is not supported\n");
      return;
    }
    ok( ret == ERROR_INVALID_PARAMETER, "got %u instead of ERROR_INVALID_PARAMETER\n", ret );
    needed = 0xdeadbeef;
    ret = pGetPerAdapterInfo(1, NULL, &needed);
    if (ret == ERROR_NO_DATA) return;  /* no such adapter */
    ok( ret == ERROR_BUFFER_OVERFLOW, "got %u instead of ERROR_BUFFER_OVERFLOW\n", ret );
    ok( needed != 0xdeadbeef, "needed not set\n" );
    buffer = HeapAlloc( GetProcessHeap(), 0, needed );
    ret = pGetPerAdapterInfo(1, buffer, &needed);
    ok( ret == NO_ERROR, "got %u instead of NO_ERROR\n", ret );
    HeapFree( GetProcessHeap(), 0, buffer );
}

static void testNotifyAddrChange(void)
{
    DWORD ret, bytes;
    OVERLAPPED overlapped;
    HANDLE handle;
    BOOL success;

    if (!pNotifyAddrChange)
    {
        win_skip("NotifyAddrChange not present\n");
        return;
    }
    if (!pCancelIPChangeNotify)
    {
        win_skip("CancelIPChangeNotify not present\n");
        return;
    }

    handle = NULL;
    ZeroMemory(&overlapped, sizeof(overlapped));
    ret = pNotifyAddrChange(&handle, &overlapped);
    if (ret == ERROR_NOT_SUPPORTED)
    {
        win_skip("NotifyAddrChange is not supported\n");
        return;
    }
    ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %d, expected ERROR_IO_PENDING\n", ret);
    ret = GetLastError();
    todo_wine ok(ret == ERROR_IO_PENDING, "GetLastError returned %d, expected ERROR_IO_PENDING\n", ret);
    success = pCancelIPChangeNotify(&overlapped);
    todo_wine ok(success == TRUE, "CancelIPChangeNotify returned FALSE, expected TRUE\n");

    ZeroMemory(&overlapped, sizeof(overlapped));
    success = pCancelIPChangeNotify(&overlapped);
    ok(success == FALSE, "CancelIPChangeNotify returned TRUE, expected FALSE\n");

    handle = NULL;
    ZeroMemory(&overlapped, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    ret = pNotifyAddrChange(&handle, &overlapped);
    ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %d, expected ERROR_IO_PENDING\n", ret);
    todo_wine ok(handle != INVALID_HANDLE_VALUE, "NotifyAddrChange returned invalid file handle\n");
    success = GetOverlappedResult(handle, &overlapped, &bytes, FALSE);
    ok(success == FALSE, "GetOverlappedResult returned TRUE, expected FALSE\n");
    ret = GetLastError();
    ok(ret == ERROR_IO_INCOMPLETE, "GetLastError returned %d, expected ERROR_IO_INCOMPLETE\n", ret);
    success = pCancelIPChangeNotify(&overlapped);
    todo_wine ok(success == TRUE, "CancelIPChangeNotify returned FALSE, expected TRUE\n");

    if (winetest_interactive)
    {
        handle = NULL;
        ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        trace("Testing asynchronous ipv4 address change notification. Please "
              "change the ipv4 address of one of your network interfaces\n");
        ret = pNotifyAddrChange(&handle, &overlapped);
        ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %d, expected NO_ERROR\n", ret);
        success = GetOverlappedResult(handle, &overlapped, &bytes, TRUE);
        ok(success == TRUE, "GetOverlappedResult returned FALSE, expected TRUE\n");
    }

    /* test synchronous functionality */
    if (winetest_interactive)
    {
        trace("Testing synchronous ipv4 address change notification. Please "
              "change the ipv4 address of one of your network interfaces\n");
        ret = pNotifyAddrChange(NULL, NULL);
        todo_wine ok(ret == NO_ERROR, "NotifyAddrChange returned %d, expected NO_ERROR\n", ret);
    }
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
NotifyRouteChange + CancelIPChangeNotify
SendARP
UnenableRouter
*/
static void testWin2KFunctions(void)
{
    testGetPerAdapterInfo();
    testNotifyAddrChange();
}

static void test_GetAdaptersAddresses(void)
{
    ULONG ret, size, osize, i;
    IP_ADAPTER_ADDRESSES *aa, *ptr;
    IP_ADAPTER_UNICAST_ADDRESS *ua;

    if (!pGetAdaptersAddresses)
    {
        win_skip("GetAdaptersAddresses not present\n");
        return;
    }

    ret = pGetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", ret);

    /* size should be ignored and overwritten if buffer is NULL */
    size = 0x7fffffff;
    ret = pGetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    ok(ret == ERROR_BUFFER_OVERFLOW, "expected ERROR_BUFFER_OVERFLOW, got %u\n", ret);
    if (ret != ERROR_BUFFER_OVERFLOW) return;

    ptr = HeapAlloc(GetProcessHeap(), 0, size);
    ret = pGetAdaptersAddresses(AF_UNSPEC, 0, NULL, ptr, &size);
    ok(!ret, "expected ERROR_SUCCESS got %u\n", ret);
    HeapFree(GetProcessHeap(), 0, ptr);

    /* higher size must not be changed to lower size */
    size *= 2;
    osize = size;
    ptr = HeapAlloc(GetProcessHeap(), 0, osize);
    ret = pGetAdaptersAddresses(AF_UNSPEC, 0, NULL, ptr, &osize);
    ok(!ret, "expected ERROR_SUCCESS got %u\n", ret);
    ok(osize == size, "expected %d, got %d\n", size, osize);

    for (aa = ptr; !ret && aa; aa = aa->Next)
    {
        char temp[128];

        ok(S(U(*aa)).Length == sizeof(IP_ADAPTER_ADDRESSES_LH) ||
           S(U(*aa)).Length == sizeof(IP_ADAPTER_ADDRESSES_XP),
           "Unknown structure size of %u bytes\n", S(U(*aa)).Length);
        ok(aa->DnsSuffix != NULL, "DnsSuffix is not a valid pointer\n");
        ok(aa->Description != NULL, "Description is not a valid pointer\n");
        ok(aa->FriendlyName != NULL, "FriendlyName is not a valid pointer\n");

        trace("\n");
        trace("Length:                %u\n", S(U(*aa)).Length);
        trace("IfIndex:               %u\n", S(U(*aa)).IfIndex);
        trace("Next:                  %p\n", aa->Next);
        trace("AdapterName:           %s\n", aa->AdapterName);
        trace("FirstUnicastAddress:   %p\n", aa->FirstUnicastAddress);
        ua = aa->FirstUnicastAddress;
        while (ua)
        {
            ok(ua->PrefixOrigin != IpPrefixOriginOther,
               "bad address config value %d\n", ua->PrefixOrigin);
            ok(ua->SuffixOrigin != IpSuffixOriginOther,
               "bad address config value %d\n", ua->PrefixOrigin);
            /* Address configured manually or from DHCP server? */
            if (ua->PrefixOrigin == IpPrefixOriginManual ||
                ua->PrefixOrigin == IpPrefixOriginDhcp)
            {
                ok(ua->ValidLifetime, "expected non-zero value\n");
                ok(ua->PreferredLifetime, "expected non-zero value\n");
                ok(ua->LeaseLifetime, "expected non-zero\n");
            }
            /* Is the address ok in the network (not duplicated)? */
            ok(ua->DadState != IpDadStateInvalid && ua->DadState != IpDadStateDuplicate,
               "bad address duplication value %d\n", ua->DadState);
            trace("\tLength:                  %u\n", S(U(*ua)).Length);
            trace("\tFlags:                   0x%08x\n", S(U(*ua)).Flags);
            trace("\tNext:                    %p\n", ua->Next);
            trace("\tAddress.lpSockaddr:      %p\n", ua->Address.lpSockaddr);
            trace("\tAddress.iSockaddrLength: %d\n", ua->Address.iSockaddrLength);
            trace("\tPrefixOrigin:            %u\n", ua->PrefixOrigin);
            trace("\tSuffixOrigin:            %u\n", ua->SuffixOrigin);
            trace("\tDadState:                %u\n", ua->DadState);
            trace("\tValidLifetime:           %u seconds\n", ua->ValidLifetime);
            trace("\tPreferredLifetime:       %u seconds\n", ua->PreferredLifetime);
            trace("\tLeaseLifetime:           %u seconds\n", ua->LeaseLifetime);
            trace("\n");
            ua = ua->Next;
        }
        trace("FirstAnycastAddress:   %p\n", aa->FirstAnycastAddress);
        trace("FirstMulticastAddress: %p\n", aa->FirstMulticastAddress);
        trace("FirstDnsServerAddress: %p\n", aa->FirstDnsServerAddress);
        trace("DnsSuffix:             %s %p\n", wine_dbgstr_w(aa->DnsSuffix), aa->DnsSuffix);
        trace("Description:           %s %p\n", wine_dbgstr_w(aa->Description), aa->Description);
        trace("FriendlyName:          %s %p\n", wine_dbgstr_w(aa->FriendlyName), aa->FriendlyName);
        trace("PhysicalAddressLength: %u\n", aa->PhysicalAddressLength);
        for (i = 0; i < aa->PhysicalAddressLength; i++)
            sprintf(temp + i * 3, "%02X-", aa->PhysicalAddress[i]);
        temp[i ? i * 3 - 1 : 0] = '\0';
        trace("PhysicalAddress:       %s\n", temp);
        trace("Flags:                 0x%08x\n", aa->Flags);
        trace("Mtu:                   %u\n", aa->Mtu);
        trace("IfType:                %u\n", aa->IfType);
        trace("OperStatus:            %u\n", aa->OperStatus);
        trace("Ipv6IfIndex:           %u\n", aa->Ipv6IfIndex);
        for (i = 0, temp[0] = '\0'; i < sizeof(aa->ZoneIndices) / sizeof(aa->ZoneIndices[0]); i++)
            sprintf(temp + strlen(temp), "%d ", aa->ZoneIndices[i]);
        trace("ZoneIndices:           %s\n", temp);
        trace("FirstPrefix:           %p\n", aa->FirstPrefix);

        if (S(U(*aa)).Length < sizeof(IP_ADAPTER_ADDRESSES_LH)) continue;
#ifndef __REACTOS__
        trace("TransmitLinkSpeed:     %s\n", debugstr_longlong(aa->TransmitLinkSpeed));
        trace("ReceiveLinkSpeed:      %s\n", debugstr_longlong(aa->ReceiveLinkSpeed));
        trace("FirstWinsServerAddress:%p\n", aa->FirstWinsServerAddress);
        trace("FirstGatewayAddress:   %p\n", aa->FirstGatewayAddress);
        trace("Ipv4Metric:            %u\n", aa->Ipv4Metric);
        trace("Ipv6Metric:            %u\n", aa->Ipv6Metric);
        trace("Luid:                  %p\n", &aa->Luid);
        trace("Dhcpv4Server:          %p\n", &aa->Dhcpv4Server);
        trace("CompartmentId:         %u\n", aa->CompartmentId);
        trace("NetworkGuid:           %s\n", wine_dbgstr_guid((GUID*) &aa->NetworkGuid));
        trace("ConnectionType:        %u\n", aa->ConnectionType);
        trace("TunnelType:            %u\n", aa->TunnelType);
        trace("Dhcpv6Server:          %p\n", &aa->Dhcpv6Server);
        trace("Dhcpv6ClientDuidLength:%u\n", aa->Dhcpv6ClientDuidLength);
        trace("Dhcpv6ClientDuid:      %p\n", aa->Dhcpv6ClientDuid);
        trace("Dhcpv6Iaid:            %u\n", aa->Dhcpv6Iaid);
        trace("FirstDnsSuffix:        %p\n", aa->FirstDnsSuffix);
        trace("\n");
#endif
    }
    HeapFree(GetProcessHeap(), 0, ptr);
}

static void test_GetExtendedTcpTable(void)
{
    DWORD ret, size;
    MIB_TCPTABLE *table;
    MIB_TCPTABLE_OWNER_PID *table_pid;
    MIB_TCPTABLE_OWNER_MODULE *table_module;

    if (!pGetExtendedTcpTable)
    {
        win_skip("GetExtendedTcpTable not available\n");
        return;
    }
    ret = pGetExtendedTcpTable( NULL, NULL, TRUE, AF_INET, TCP_TABLE_BASIC_ALL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_BASIC_ALL, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table, &size, TRUE, AF_INET, TCP_TABLE_BASIC_ALL, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_BASIC_LISTENER, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table, &size, TRUE, AF_INET, TCP_TABLE_BASIC_LISTENER, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_pid = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_pid, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_ALL, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_pid );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_pid = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_pid, &size, TRUE, AF_INET, TCP_TABLE_OWNER_PID_LISTENER, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_pid );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_module = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_module, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_ALL, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_module );

    size = 0;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_module = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedTcpTable( table_module, &size, TRUE, AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_module );
}

static void test_GetExtendedUdpTable(void)
{
    DWORD ret, size;
    MIB_UDPTABLE *table;
    MIB_UDPTABLE_OWNER_PID *table_pid;
    MIB_UDPTABLE_OWNER_MODULE *table_module;

    if (!pGetExtendedUdpTable)
    {
        win_skip("GetExtendedUdpTable not available\n");
        return;
    }
    ret = pGetExtendedUdpTable( NULL, NULL, TRUE, AF_INET, UDP_TABLE_BASIC, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    size = 0;
    ret = pGetExtendedUdpTable( NULL, &size, TRUE, AF_INET, UDP_TABLE_BASIC, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedUdpTable( table, &size, TRUE, AF_INET, UDP_TABLE_BASIC, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table );

    size = 0;
    ret = pGetExtendedUdpTable( NULL, &size, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_pid = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedUdpTable( table_pid, &size, TRUE, AF_INET, UDP_TABLE_OWNER_PID, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_pid );

    size = 0;
    ret = pGetExtendedUdpTable( NULL, &size, TRUE, AF_INET, UDP_TABLE_OWNER_MODULE, 0 );
    ok( ret == ERROR_INSUFFICIENT_BUFFER, "got %u\n", ret );

    table_module = HeapAlloc( GetProcessHeap(), 0, size );
    ret = pGetExtendedUdpTable( table_module, &size, TRUE, AF_INET, UDP_TABLE_OWNER_MODULE, 0 );
    ok( ret == ERROR_SUCCESS, "got %u\n", ret );
    HeapFree( GetProcessHeap(), 0, table_module );
}

static void test_CreateSortedAddressPairs(void)
{
    SOCKADDR_IN6 dst[2];
    SOCKADDR_IN6_PAIR *pair;
    ULONG pair_count;
    DWORD ret;

    if (!pCreateSortedAddressPairs)
    {
        win_skip( "CreateSortedAddressPairs not available\n" );
        return;
    }

    memset( dst, 0, sizeof(dst) );
    dst[0].sin6_family = AF_INET6;
    dst[0].sin6_addr.u.Word[5] = 0xffff;
    dst[0].sin6_addr.u.Word[6] = 0x0808;
    dst[0].sin6_addr.u.Word[7] = 0x0808;

    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 1, 0, NULL, &pair_count );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    ok( pair_count == 0xdeadbeef, "got %u\n", pair_count );

    pair = (SOCKADDR_IN6_PAIR *)0xdeadbeef;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, NULL, 1, 0, &pair, &pair_count );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    ok( pair == (SOCKADDR_IN6_PAIR *)0xdeadbeef, "got %p\n", pair );
    ok( pair_count == 0xdeadbeef, "got %u\n", pair_count );

    pair = NULL;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 1, 0, &pair, &pair_count );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( pair != NULL, "pair not set\n" );
    ok( pair_count >= 1, "got %u\n", pair_count );
    ok( pair[0].SourceAddress != NULL, "src address not set\n" );
    ok( pair[0].DestinationAddress != NULL, "dst address not set\n" );
    pFreeMibTable( pair );

    dst[1].sin6_family = AF_INET6;
    dst[1].sin6_addr.u.Word[5] = 0xffff;
    dst[1].sin6_addr.u.Word[6] = 0x0404;
    dst[1].sin6_addr.u.Word[7] = 0x0808;

    pair = NULL;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 2, 0, &pair, &pair_count );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( pair != NULL, "pair not set\n" );
    ok( pair_count >= 2, "got %u\n", pair_count );
    ok( pair[0].SourceAddress != NULL, "src address not set\n" );
    ok( pair[0].DestinationAddress != NULL, "dst address not set\n" );
    ok( pair[1].SourceAddress != NULL, "src address not set\n" );
    ok( pair[1].DestinationAddress != NULL, "dst address not set\n" );
    pFreeMibTable( pair );
}

static DWORD get_interface_index(void)
{
    DWORD size = 0, ret = 0;
    IP_ADAPTER_ADDRESSES *buf, *aa;

    if (pGetAdaptersAddresses( AF_UNSPEC, 0, NULL, NULL, &size ) != ERROR_BUFFER_OVERFLOW)
        return 0;

    buf = HeapAlloc( GetProcessHeap(), 0, size );
    pGetAdaptersAddresses( AF_UNSPEC, 0, NULL, buf, &size );
    for (aa = buf; aa; aa = aa->Next)
    {
        if (aa->IfType == IF_TYPE_ETHERNET_CSMACD)
        {
            ret = aa->IfIndex;
            break;
        }
    }
    HeapFree( GetProcessHeap(), 0, buf );
    return ret;
}

static void test_interface_identifier_conversion(void)
{
    DWORD ret;
    NET_LUID luid;
    GUID guid;
    SIZE_T len;
    WCHAR nameW[IF_MAX_STRING_SIZE + 1];
    char nameA[IF_MAX_STRING_SIZE + 1];
    NET_IFINDEX index;

    if (!pConvertInterfaceIndexToLuid)
    {
        win_skip( "ConvertInterfaceIndexToLuid not available\n" );
        return;
    }
    if (!(index = get_interface_index()))
    {
        skip( "no suitable interface found\n" );
        return;
    }

    /* ConvertInterfaceIndexToLuid */
    ret = pConvertInterfaceIndexToLuid( 0, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &luid, 0xff, sizeof(luid) );
    ret = pConvertInterfaceIndexToLuid( 0, &luid );
    ok( ret == ERROR_FILE_NOT_FOUND, "got %u\n", ret );
    ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
    ok( !luid.Info.NetLuidIndex, "got %u\n", luid.Info.NetLuidIndex );
    ok( !luid.Info.IfType, "got %u\n", luid.Info.IfType );

    luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
    ret = pConvertInterfaceIndexToLuid( index, &luid );
    ok( !ret, "got %u\n", ret );
    ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
    ok( luid.Info.NetLuidIndex != 0xdead, "index not set\n" );
    ok( luid.Info.IfType == IF_TYPE_ETHERNET_CSMACD, "got %u\n", luid.Info.IfType );

    /* ConvertInterfaceLuidToIndex */
    ret = pConvertInterfaceLuidToIndex( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToIndex( NULL, &index );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToIndex( &luid, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToIndex( &luid, &index );
    ok( !ret, "got %u\n", ret );

    /* ConvertInterfaceLuidToGuid */
    ret = pConvertInterfaceLuidToGuid( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &guid, 0xff, sizeof(guid) );
    ret = pConvertInterfaceLuidToGuid( NULL, &guid );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    ok( guid.Data1 == 0xffffffff, "got %x\n", guid.Data1 );

    ret = pConvertInterfaceLuidToGuid( &luid, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &guid, 0, sizeof(guid) );
    ret = pConvertInterfaceLuidToGuid( &luid, &guid );
    ok( !ret, "got %u\n", ret );
    ok( guid.Data1, "got %x\n", guid.Data1 );

    /* ConvertInterfaceGuidToLuid */
    ret = pConvertInterfaceGuidToLuid( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    luid.Info.NetLuidIndex = 1;
    ret = pConvertInterfaceGuidToLuid( NULL, &luid );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );
    ok( luid.Info.NetLuidIndex == 1, "got %u\n", luid.Info.NetLuidIndex );

    ret = pConvertInterfaceGuidToLuid( &guid, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
    ret = pConvertInterfaceGuidToLuid( &guid, &luid );
    ok( !ret, "got %u\n", ret );
    ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
    ok( luid.Info.NetLuidIndex != 0xdead, "index not set\n" );
    ok( luid.Info.IfType == IF_TYPE_ETHERNET_CSMACD, "got %u\n", luid.Info.IfType );

    /* ConvertInterfaceLuidToNameW */
    ret = pConvertInterfaceLuidToNameW( NULL, NULL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToNameW( &luid, NULL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToNameW( NULL, nameW, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToNameW( &luid, nameW, 0 );
    ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %u\n", ret );

    nameW[0] = 0;
    len = sizeof(nameW)/sizeof(nameW[0]);
    ret = pConvertInterfaceLuidToNameW( &luid, nameW, len );
    ok( !ret, "got %u\n", ret );
    ok( nameW[0], "name not set\n" );

    /* ConvertInterfaceLuidToNameA */
    ret = pConvertInterfaceLuidToNameA( NULL, NULL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToNameA( &luid, NULL, 0 );
    ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %u\n", ret );

    ret = pConvertInterfaceLuidToNameA( NULL, nameA, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    ret = pConvertInterfaceLuidToNameA( &luid, nameA, 0 );
    ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %u\n", ret );

    nameA[0] = 0;
    len = sizeof(nameA)/sizeof(nameA[0]);
    ret = pConvertInterfaceLuidToNameA( &luid, nameA, len );
    ok( !ret, "got %u\n", ret );
    ok( nameA[0], "name not set\n" );

    /* ConvertInterfaceNameToLuidW */
    ret = pConvertInterfaceNameToLuidW( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
    ret = pConvertInterfaceNameToLuidW( NULL, &luid );
    ok( ret == ERROR_INVALID_NAME, "got %u\n", ret );
    ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
    ok( luid.Info.NetLuidIndex != 0xdead, "index not set\n" );
    ok( !luid.Info.IfType, "got %u\n", luid.Info.IfType );

    ret = pConvertInterfaceNameToLuidW( nameW, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
    ret = pConvertInterfaceNameToLuidW( nameW, &luid );
    ok( !ret, "got %u\n", ret );
    ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
    ok( luid.Info.NetLuidIndex != 0xdead, "index not set\n" );
    ok( luid.Info.IfType == IF_TYPE_ETHERNET_CSMACD, "got %u\n", luid.Info.IfType );

    /* ConvertInterfaceNameToLuidA */
    ret = pConvertInterfaceNameToLuidA( NULL, NULL );
    ok( ret == ERROR_INVALID_NAME, "got %u\n", ret );

    luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
    ret = pConvertInterfaceNameToLuidA( NULL, &luid );
    ok( ret == ERROR_INVALID_NAME, "got %u\n", ret );
    ok( luid.Info.Reserved == 0xdead, "reserved set\n" );
    ok( luid.Info.NetLuidIndex == 0xdead, "index set\n" );
    ok( luid.Info.IfType == 0xdead, "type set\n" );

    ret = pConvertInterfaceNameToLuidA( nameA, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
    ret = pConvertInterfaceNameToLuidA( nameA, &luid );
    ok( !ret, "got %u\n", ret );
    ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
    ok( luid.Info.NetLuidIndex != 0xdead, "index not set\n" );
    ok( luid.Info.IfType == IF_TYPE_ETHERNET_CSMACD, "got %u\n", luid.Info.IfType );
}

static void test_GetIfEntry2(void)
{
    DWORD ret;
    MIB_IF_ROW2 row;
    NET_IFINDEX index;

    if (!pGetIfEntry2)
    {
        win_skip( "GetIfEntry2 not available\n" );
        return;
    }
    if (!(index = get_interface_index()))
    {
        skip( "no suitable interface found\n" );
        return;
    }

    ret = pGetIfEntry2( NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    ret = pGetIfEntry2( &row );
    ok( ret == ERROR_INVALID_PARAMETER, "got %u\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = index;
    ret = pGetIfEntry2( &row );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( row.InterfaceIndex == index, "got %u\n", index );
}

static void test_GetIfTable2(void)
{
    DWORD ret;
    MIB_IF_TABLE2 *table;

    if (!pGetIfTable2)
    {
        win_skip( "GetIfTable2 not available\n" );
        return;
    }

    table = NULL;
    ret = pGetIfTable2( &table );
    ok( ret == NO_ERROR, "got %u\n", ret );
    ok( table != NULL, "table not set\n" );
    pFreeMibTable( table );
}

START_TEST(iphlpapi)
{

  loadIPHlpApi();
  if (hLibrary) {
    HANDLE thread;

    testWin98OnlyFunctions();
    testWinNT4Functions();

    /* run testGetXXXX in two threads at once to make sure we don't crash in that case */
    thread = CreateThread(NULL, 0, testWin98Functions, NULL, 0, NULL);
    testWin98Functions(NULL);
    WaitForSingleObject(thread, INFINITE);

    testWin2KFunctions();
    test_GetAdaptersAddresses();
    test_GetExtendedTcpTable();
    test_GetExtendedUdpTable();
    test_CreateSortedAddressPairs();
    test_interface_identifier_conversion();
    test_GetIfEntry2();
    test_GetIfTable2();
    freeIPHlpApi();
  }
}
