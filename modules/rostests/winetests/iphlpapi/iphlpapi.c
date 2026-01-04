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
 * an ANSI string on some versions of the IP helper API under Win9x.  This was
 * apparently an MS bug, it's corrected in later versions.
 *
 * The DomainName field of FIXED_INFO isn't NULL-terminated on Win98.
 */

#include <stdarg.h>
#include "winsock2.h"
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "ws2tcpip.h"
#include "windns.h"
#include "iphlpapi.h"
#include "icmpapi.h"
#include "iprtrmib.h"
#include "netioapi.h"
#include "wine/test.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef __REACTOS__
#include <versionhelpers.h>
const IN6_ADDR in6addr_loopback = {{ IN6ADDR_LOOPBACK_INIT }};
#endif

#define ICMP_MINLEN 8 /* copied from dlls/iphlpapi/ip_icmp.h file */

static HMODULE hLibrary = NULL;

static DWORD (WINAPI *pAllocateAndGetTcpExTableFromStack)(void**,BOOL,HANDLE,DWORD,DWORD);
static DWORD (WINAPI *pGetTcp6Table)(PMIB_TCP6TABLE,PDWORD,BOOL);
static DWORD (WINAPI *pGetUdp6Table)(PMIB_UDP6TABLE,PDWORD,BOOL);
static DWORD (WINAPI *pGetUnicastIpAddressEntry)(MIB_UNICASTIPADDRESS_ROW*);
static DWORD (WINAPI *pGetUnicastIpAddressTable)(ADDRESS_FAMILY,MIB_UNICASTIPADDRESS_TABLE**);
static DWORD (WINAPI *pGetExtendedTcpTable)(PVOID,PDWORD,BOOL,ULONG,TCP_TABLE_CLASS,ULONG);
static DWORD (WINAPI *pGetExtendedUdpTable)(PVOID,PDWORD,BOOL,ULONG,UDP_TABLE_CLASS,ULONG);
static DWORD (WINAPI *pCreateSortedAddressPairs)(const PSOCKADDR_IN6,ULONG,const PSOCKADDR_IN6,ULONG,ULONG,
                                                 PSOCKADDR_IN6_PAIR*,ULONG*);
static DWORD (WINAPI *pConvertLengthToIpv4Mask)(ULONG,ULONG*);
static DWORD (WINAPI *pParseNetworkString)(const WCHAR*,DWORD,NET_ADDRESS_INFO*,USHORT*,BYTE*);
static DWORD (WINAPI *pNotifyUnicastIpAddressChange)(ADDRESS_FAMILY, PUNICAST_IPADDRESS_CHANGE_CALLBACK,
                                                PVOID, BOOLEAN, HANDLE *);
static DWORD (WINAPI *pCancelMibChangeNotify2)(HANDLE);

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
static DWORD (WINAPI *pConvertInterfaceAliasToLuid)(const WCHAR*,NET_LUID*);
static DWORD (WINAPI *pConvertInterfaceLuidToAlias)(NET_LUID*,WCHAR*,SIZE_T);
static DWORD (WINAPI *pConvertGuidToStringA)(const GUID*,char*,DWORD);
static DWORD (WINAPI *pConvertGuidToStringW)(const GUID*,WCHAR*,DWORD);
static DWORD (WINAPI *pConvertStringToGuidW)(const WCHAR*,GUID*);
static DWORD (WINAPI *pGetIfEntry2)(PMIB_IF_ROW2);
static DWORD (WINAPI *pGetIfTable2)(PMIB_IF_TABLE2*);
static DWORD (WINAPI *pGetIfTable2Ex)(MIB_IF_TABLE_LEVEL,MIB_IF_TABLE2**);
static DWORD (WINAPI *pGetIpForwardTable2)(ADDRESS_FAMILY,MIB_IPFORWARD_TABLE2**);
static DWORD (WINAPI *pGetIpNetTable2)(ADDRESS_FAMILY,MIB_IPNET_TABLE2**);
static void (WINAPI *pFreeMibTable)(void*);
static DWORD (WINAPI *pConvertInterfaceGuidToLuid)(const GUID*,NET_LUID*);
static DWORD (WINAPI *pConvertInterfaceIndexToLuid)(NET_IFINDEX,NET_LUID*);
static DWORD (WINAPI *pConvertInterfaceLuidToGuid)(const NET_LUID*,GUID*);
static DWORD (WINAPI *pConvertInterfaceLuidToIndex)(const NET_LUID*,NET_IFINDEX*);
static DWORD (WINAPI *pConvertInterfaceLuidToNameA)(const NET_LUID*,char*,SIZE_T);
static DWORD (WINAPI *pConvertInterfaceLuidToNameW)(const NET_LUID*,WCHAR*,SIZE_T);
static DWORD (WINAPI *pConvertInterfaceNameToLuidA)(const char*,NET_LUID*);
static DWORD (WINAPI *pConvertInterfaceNameToLuidW)(const WCHAR*,NET_LUID*);
static NET_IF_COMPARTMENT_ID (WINAPI *pGetCurrentThreadCompartmentId)();
static char* (WINAPI *pif_indextoname)(NET_IFINDEX,char*);
static IF_INDEX (WINAPI *pif_nametoindex)(const char*);

#define ConvertInterfaceAliasToLuid    pConvertInterfaceAliasToLuid
#define ConvertInterfaceLuidToAlias    pConvertInterfaceLuidToAlias
#define ConvertGuidToStringA           pConvertGuidToStringA
#define ConvertGuidToStringW           pConvertGuidToStringW
#define ConvertStringToGuidW           pConvertStringToGuidW
#define GetIfEntry2                    pGetIfEntry2
#define GetIfTable2                    pGetIfTable2
#define GetIfTable2Ex                  pGetIfTable2Ex
#define GetIpForwardTable2             pGetIpForwardTable2
#define GetIpNetTable2                 pGetIpNetTable2
#define FreeMibTable                   pFreeMibTable
#define ConvertInterfaceGuidToLuid     pConvertInterfaceGuidToLuid
#define ConvertInterfaceIndexToLuid    pConvertInterfaceIndexToLuid
#define ConvertInterfaceLuidToGuid     pConvertInterfaceLuidToGuid
#define ConvertInterfaceLuidToIndex    pConvertInterfaceLuidToIndex
#define ConvertInterfaceLuidToNameA    pConvertInterfaceLuidToNameA
#define ConvertInterfaceLuidToNameW    pConvertInterfaceLuidToNameW
#define ConvertInterfaceNameToLuidA    pConvertInterfaceNameToLuidA
#define ConvertInterfaceNameToLuidW    pConvertInterfaceNameToLuidW
#define GetCurrentThreadCompartmentId  pGetCurrentThreadCompartmentId
#define if_indextoname                 pif_indextoname
#define if_nametoindex                 pif_nametoindex

/* Wine mistakes */
#define GetUnicastIpAddressTable       pGetUnicastIpAddressTable
#define ConvertLengthToIpv4Mask        pConvertLengthToIpv4Mask
#else
DWORD WINAPI ConvertGuidToStringA( const GUID *, char *, DWORD );
DWORD WINAPI ConvertGuidToStringW( const GUID *, WCHAR *, DWORD );
DWORD WINAPI ConvertStringToGuidW( const WCHAR *, GUID * );
#endif

static void loadIPHlpApi(void)
{
  hLibrary = LoadLibraryA("iphlpapi.dll");
  if (hLibrary) {
    pAllocateAndGetTcpExTableFromStack = (void *)GetProcAddress(hLibrary, "AllocateAndGetTcpExTableFromStack");
    pGetTcp6Table = (void *)GetProcAddress(hLibrary, "GetTcp6Table");
    pGetUdp6Table = (void *)GetProcAddress(hLibrary, "GetUdp6Table");
    pGetUnicastIpAddressEntry = (void *)GetProcAddress(hLibrary, "GetUnicastIpAddressEntry");
    pGetUnicastIpAddressTable = (void *)GetProcAddress(hLibrary, "GetUnicastIpAddressTable");
    pGetExtendedTcpTable = (void *)GetProcAddress(hLibrary, "GetExtendedTcpTable");
    pGetExtendedUdpTable = (void *)GetProcAddress(hLibrary, "GetExtendedUdpTable");
    pCreateSortedAddressPairs = (void *)GetProcAddress(hLibrary, "CreateSortedAddressPairs");
    pConvertLengthToIpv4Mask = (void *)GetProcAddress(hLibrary, "ConvertLengthToIpv4Mask");
    pParseNetworkString = (void *)GetProcAddress(hLibrary, "ParseNetworkString");
    pNotifyUnicastIpAddressChange = (void *)GetProcAddress(hLibrary, "NotifyUnicastIpAddressChange");
    pCancelMibChangeNotify2 = (void *)GetProcAddress(hLibrary, "CancelMibChangeNotify2");
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    ConvertInterfaceAliasToLuid = (void *)GetProcAddress(hLibrary, "ConvertInterfaceAliasToLuid");
    ConvertInterfaceLuidToAlias = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToAlias");
    ConvertGuidToStringA = (void *)GetProcAddress(hLibrary, "ConvertGuidToStringA");
    ConvertGuidToStringW = (void *)GetProcAddress(hLibrary, "ConvertGuidToStringW");
    ConvertStringToGuidW = (void *)GetProcAddress(hLibrary, "ConvertStringToGuidW");
    GetIfEntry2 = (void *)GetProcAddress(hLibrary, "GetIfEntry2");
    GetIfTable2 = (void *)GetProcAddress(hLibrary, "GetIfTable2");
    GetIfTable2Ex = (void *)GetProcAddress(hLibrary, "GetIfTable2Ex");
    GetIpForwardTable2 = (void *)GetProcAddress(hLibrary, "GetIpForwardTable2");
    GetIpNetTable2 = (void *)GetProcAddress(hLibrary, "GetIpNetTable2");
    FreeMibTable = (void *)GetProcAddress(hLibrary, "FreeMibTable");
    ConvertInterfaceGuidToLuid = (void *)GetProcAddress(hLibrary, "ConvertInterfaceGuidToLuid");
    ConvertInterfaceIndexToLuid = (void *)GetProcAddress(hLibrary, "ConvertInterfaceIndexToLuid");
    ConvertInterfaceLuidToGuid = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToGuid");
    ConvertInterfaceLuidToIndex = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToIndex");
    ConvertInterfaceLuidToNameA = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToNameA");
    ConvertInterfaceLuidToNameW = (void *)GetProcAddress(hLibrary, "ConvertInterfaceLuidToNameW");
    ConvertInterfaceNameToLuidA = (void *)GetProcAddress(hLibrary, "ConvertInterfaceNameToLuidA");
    ConvertInterfaceNameToLuidW = (void *)GetProcAddress(hLibrary, "ConvertInterfaceNameToLuidW");
    GetCurrentThreadCompartmentId = (void *)GetProcAddress(hLibrary, "GetCurrentThreadCompartmentId");
    if_indextoname = (void *)GetProcAddress(hLibrary, "if_indextoname");
    if_nametoindex = (void *)GetProcAddress(hLibrary, "if_nametoindex");

    /* If we can't get FreeMibTable(), use free(). */
    if (!FreeMibTable)
        FreeMibTable = (void (WINAPI *)(void *))free;
#endif
  }
}

static void freeIPHlpApi(void)
{
    FreeLibrary(hLibrary);
}

/* replacement for inet_ntoa */
static const char *ntoa( DWORD ip )
{
    static char buffers[4][16];
    static int i = -1;

    ip = htonl(ip);
    i = (i + 1) % ARRAY_SIZE(buffers);
    sprintf( buffers[i], "%lu.%lu.%lu.%lu", (ip >> 24) & 0xff, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff );
    return buffers[i];
}

static const char *ntoa6( IN6_ADDR *ip )
{
    static char buffers[4][40];
    static int i = -1;
    unsigned short *p = ip->u.Word;

    i = (i + 1) % ARRAY_SIZE(buffers);
    sprintf( buffers[i], "%x:%x:%x:%x:%x:%x:%x:%x",
             htons(p[0]), htons(p[1]), htons(p[2]), htons(p[3]), htons(p[4]), htons(p[5]), htons(p[6]), htons(p[7]) );
    return buffers[i];
}

static DWORD ipv4_addr( BYTE b1, BYTE b2, BYTE b3, BYTE b4 )
{
    return htonl( (b1 << 24) | (b2 << 16) | (b3 << 8) | b4 );
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
    DWORD apiReturn, numInterfaces;

    /* Crashes on Vista */
    if (0) {
        apiReturn = GetNumberOfInterfaces(NULL);
        if (apiReturn == ERROR_NOT_SUPPORTED)
            return;
        ok(apiReturn == ERROR_INVALID_PARAMETER,
           "GetNumberOfInterfaces(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
           apiReturn);
    }

    apiReturn = GetNumberOfInterfaces(&numInterfaces);
    ok(apiReturn == NO_ERROR,
       "GetNumberOfInterfaces returned %ld, expected 0\n", apiReturn);
}

static void testGetIfEntry(DWORD index)
{
    DWORD apiReturn;
    MIB_IFROW row;

    memset(&row, 0, sizeof(row));
    apiReturn = GetIfEntry(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
      skip("GetIfEntry is not supported\n");
      return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
     "GetIfEntry(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
     apiReturn);
    row.dwIndex = -1; /* hope that's always bogus! */
    apiReturn = GetIfEntry(&row);
    ok(apiReturn == ERROR_INVALID_DATA ||
     apiReturn == ERROR_FILE_NOT_FOUND /* Vista */,
     "GetIfEntry(bogus row) returned %ld, expected ERROR_INVALID_DATA or ERROR_FILE_NOT_FOUND\n",
     apiReturn);
    row.dwIndex = index;
    apiReturn = GetIfEntry(&row);
    ok(apiReturn == NO_ERROR, 
     "GetIfEntry returned %ld, expected NO_ERROR\n", apiReturn);
}

static void testGetIpAddrTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetIpAddrTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIpAddrTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpAddrTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIpAddrTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetIpAddrTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IPADDRTABLE buf = malloc(dwSize);

        apiReturn = GetIpAddrTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetIpAddrTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
           apiReturn);
        if (apiReturn == NO_ERROR && buf->dwNumEntries)
        {
            int i;
            testGetIfEntry(buf->table[0].dwIndex);
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                ok (buf->table[i].wType != 0, "Test[%d]: expected wType > 0\n", i);
                ok (buf->table[i].dwBCastAddr == 1, "Test[%d]: got %08lx\n", i, buf->table[i].dwBCastAddr);
                ok (buf->table[i].dwReasmSize == 0xffff, "Test[%d]: got %08lx\n", i, buf->table[i].dwReasmSize);
                trace("Entry[%d]: addr %s, dwIndex %lu, wType 0x%x\n", i,
                      ntoa(buf->table[i].dwAddr), buf->table[i].dwIndex, buf->table[i].wType);
            }
        }
        free(buf);
    }
}

static void testGetIfTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!GetIfEntry2)
        skip("Missing APIs!\n");
#endif
    apiReturn = GetIfTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIfTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIfTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIfTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetIfTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IFTABLE buf = malloc(dwSize);

        apiReturn = GetIfTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetIfTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n\n",
           apiReturn);

        if (apiReturn == NO_ERROR)
        {
            char descr[MAX_INTERFACE_NAME_LEN];
            WCHAR name[MAX_INTERFACE_NAME_LEN];
            DWORD i, index;

            if (winetest_debug > 1) trace( "interface table: %lu entries\n", buf->dwNumEntries );
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
            if (GetIfEntry2) {
#endif
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                MIB_IFROW *row = &buf->table[i];
                MIB_IF_ROW2 row2;
                GUID *guid;

                if (winetest_debug > 1)
                {
                    trace( "%lu: '%s' type %lu mtu %lu speed %lu\n",
                           row->dwIndex, debugstr_w(row->wszName), row->dwType, row->dwMtu, row->dwSpeed );
                    trace( "        in: bytes %lu upkts %lu nupkts %lu disc %lu err %lu unk %lu\n",
                           row->dwInOctets, row->dwInUcastPkts, row->dwInNUcastPkts,
                           row->dwInDiscards, row->dwInErrors, row->dwInUnknownProtos );
                    trace( "        out: bytes %lu upkts %lu nupkts %lu disc %lu err %lu\n",
                           row->dwOutOctets, row->dwOutUcastPkts, row->dwOutNUcastPkts,
                           row->dwOutDiscards, row->dwOutErrors );
                }
                apiReturn = GetAdapterIndex( row->wszName, &index );
                ok( !apiReturn, "got %ld\n", apiReturn );
                ok( index == row->dwIndex ||
                    broken( index != row->dwIndex && index ), /* Win8 can have identical guids for two different ifaces */
                    "got %ld vs %ld\n", index, row->dwIndex );
                memset( &row2, 0, sizeof(row2) );
                row2.InterfaceIndex = row->dwIndex;
                GetIfEntry2( &row2 );
                WideCharToMultiByte( CP_ACP, 0, row2.Description, -1, descr, sizeof(descr), NULL, NULL );
                ok( !strcmp( (char *)row->bDescr, descr ), "got %s vs %s\n", row->bDescr, descr );
                guid = &row2.InterfaceGuid;
#ifdef __REACTOS__
                _snwprintf( name, ARRAY_SIZE(name), L"\\DEVICE\\TCPIP_{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1],
                            guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5],
                            guid->Data4[6], guid->Data4[7]);
#else
                swprintf( name, ARRAY_SIZE(name), L"\\DEVICE\\TCPIP_{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                          guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1],
                          guid->Data4[2], guid->Data4[3], guid->Data4[4], guid->Data4[5],
                          guid->Data4[6], guid->Data4[7]);
#endif
                ok( !wcscmp( row->wszName, name ), "got %s vs %s\n", debugstr_w( row->wszName ), debugstr_w( name ) );
            }
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
            }
#endif
        }
        free(buf);
    }
}

static void testGetIpForwardTable(void)
{
    DWORD err, i, j;
    ULONG size = 0;
    MIB_IPFORWARDTABLE *buf;
    MIB_IPFORWARD_TABLE2 *table2;
    MIB_UNICASTIPADDRESS_TABLE *unicast;
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!GetIpForwardTable2 || !GetUnicastIpAddressTable || !ConvertLengthToIpv4Mask) {
        skip("Missing APIs!\n");
        return;
    }
#endif

    err = GetIpForwardTable( NULL, NULL, FALSE );
    ok( err == ERROR_INVALID_PARAMETER, "got %ld\n", err );

    err = GetIpForwardTable( NULL, &size, FALSE );
    ok( err == ERROR_INSUFFICIENT_BUFFER, "got %ld\n", err );

    buf = malloc( size );
    err = GetIpForwardTable( buf, &size, FALSE );
    ok( !err, "got %ld\n", err );

    err = GetIpForwardTable2( AF_INET, &table2 );
    ok( !err, "got %ld\n", err );
    ok( buf->dwNumEntries == table2->NumEntries, "got %ld vs %ld\n",
        buf->dwNumEntries, table2->NumEntries );

    err = GetUnicastIpAddressTable( AF_INET, &unicast );
    ok( !err, "got %ld\n", err );

    trace( "IP forward table: %lu entries\n", buf->dwNumEntries );
    for (i = 0; i < buf->dwNumEntries; i++)
    {
        MIB_IPFORWARDROW *row = buf->table + i;
        MIB_IPFORWARD_ROW2 *row2 = table2->Table + i;
        DWORD mask, next_hop;

        winetest_push_context( "%ld", i );

        trace( "dest %s mask %s gw %s if %lu type %lu proto %lu\n",
               ntoa( row->dwForwardDest ), ntoa( row->dwForwardMask ),
               ntoa( row->dwForwardNextHop ), row->dwForwardIfIndex,
               row->dwForwardType, row->dwForwardProto );
        ok( row->dwForwardDest == row2->DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr,
            "got %08lx vs %08lx\n", row->dwForwardDest, row2->DestinationPrefix.Prefix.Ipv4.sin_addr.s_addr );
        ConvertLengthToIpv4Mask( row2->DestinationPrefix.PrefixLength, &mask );
        ok( row->dwForwardMask == mask, "got %08lx vs %08lx\n", row->dwForwardMask, mask );
        ok( row->dwForwardPolicy == 0, "got %ld\n", row->dwForwardPolicy );

        next_hop = row2->NextHop.Ipv4.sin_addr.s_addr;
        if (!next_hop) /* for direct addresses, dwForwardNextHop is set to the address of the appropriate interface */
        {
            for (j = 0; j < unicast->NumEntries; j++)
            {
                if (unicast->Table[j].InterfaceLuid.Value == row2->InterfaceLuid.Value)
                {
                    next_hop = unicast->Table[j].Address.Ipv4.sin_addr.s_addr;
                    break;
                }
            }
        }
        ok( row->dwForwardNextHop == next_hop, "got %08lx vs %08lx\n", row->dwForwardNextHop, next_hop );

        ok( row->dwForwardIfIndex == row2->InterfaceIndex, "got %ld vs %ld\n", row->dwForwardIfIndex, row2->InterfaceIndex );
        if (!row2->NextHop.Ipv4.sin_addr.s_addr)
            ok( buf->table[i].dwForwardType == MIB_IPROUTE_TYPE_DIRECT, "got %ld\n", buf->table[i].dwForwardType );
        else
            ok( buf->table[i].dwForwardType == MIB_IPROUTE_TYPE_INDIRECT, "got %ld\n", buf->table[i].dwForwardType );
        ok( row->dwForwardProto == row2->Protocol, "got %ld vs %d\n", row->dwForwardProto, row2->Protocol );
        ok( row->dwForwardAge == row2->Age || row->dwForwardAge + 1 == row2->Age,
            "got %ld vs %ld\n", row->dwForwardAge, row2->Age );
        ok( row->dwForwardNextHopAS == 0, "got %08lx\n", row->dwForwardNextHopAS );
        /* FIXME: need to add the interface's metric from GetIpInterfaceTable() */
        ok( row->dwForwardMetric1 >= row2->Metric, "got %ld vs %ld\n", row->dwForwardMetric1, row2->Metric );
        ok( row->dwForwardMetric2 == 0, "got %ld\n", row->dwForwardMetric2 );
        ok( row->dwForwardMetric3 == 0, "got %ld\n", row->dwForwardMetric3 );
        ok( row->dwForwardMetric4 == 0, "got %ld\n", row->dwForwardMetric4 );
        ok( row->dwForwardMetric5 == 0, "got %ld\n", row->dwForwardMetric5 );

        winetest_pop_context();
    }
    FreeMibTable( unicast );
    FreeMibTable( table2 );
    free( buf );
}

static void testGetIpNetTable(void)
{
    DWORD apiReturn, ret, prev_idx;
    BOOL igmp3_found, ssdp_found;
    DWORD igmp3_addr, ssdp_addr;
    MIB_IPNET_TABLE2 *table2;
    ULONG dwSize = 0;
    unsigned int i;
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!GetIpNetTable2)
        skip("Missing APIs!\n");
#endif

    igmp3_addr = ipv4_addr( 224, 0, 0, 22 );
    ssdp_addr = ipv4_addr( 239, 255, 255, 250 );

    apiReturn = GetIpNetTable(NULL, NULL, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIpNetTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpNetTable(NULL, NULL, FALSE) returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIpNetTable(NULL, &dwSize, FALSE);
    ok(apiReturn == ERROR_NO_DATA || apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetIpNetTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_NO_DATA or ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_NO_DATA)
        ; /* empty ARP table's okay */
    else if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_IPNETTABLE buf = malloc(dwSize);

        memset(buf, 0xcc, dwSize);
        apiReturn = GetIpNetTable(buf, &dwSize, TRUE);
        ok((apiReturn == NO_ERROR && buf->dwNumEntries) || (apiReturn == ERROR_NO_DATA && !buf->dwNumEntries),
            "got apiReturn %lu, dwSize %lu, buf->dwNumEntries %lu.\n",
            apiReturn, dwSize, buf->dwNumEntries);

        if (apiReturn == NO_ERROR)
        {
            for (i = 0; i < buf->dwNumEntries - 1; ++i)
            {
                ok( buf->table[i].dwIndex <= buf->table[i + 1].dwIndex,
                    "Entries are not sorted by index, i %u.\n", i );
                if (buf->table[i].dwIndex == buf->table[i + 1].dwIndex)
                    ok(ntohl(buf->table[i].dwAddr) <= ntohl(buf->table[i + 1].dwAddr),
                       "Entries are not sorted by address, i %u.\n", i );
            }

            igmp3_found = ssdp_found = FALSE;
            prev_idx = ~0u;
            for (i = 0; i < buf->dwNumEntries; ++i)
            {
                if (buf->table[i].dwIndex != prev_idx)
                {
                    if (prev_idx != ~0u)
                    {
                        ok( igmp3_found, "%s not found, iface index %lu.\n", ntoa( igmp3_addr ), prev_idx);
                        ok( ssdp_found || broken(!ssdp_found) /* 239.255.255.250 is always present since Win10 */,
                            "%s not found.\n", ntoa( ssdp_addr ));
                    }
                    prev_idx = buf->table[i].dwIndex;
                    igmp3_found = ssdp_found = FALSE;
                }
                if (buf->table[i].dwAddr == igmp3_addr)
                    igmp3_found = TRUE;
                else if (buf->table[i].dwAddr == ssdp_addr)
                    ssdp_found = TRUE;
            }
#if defined(__REACTOS__)
            if (LOBYTE(LOWORD(GetVersion())) >= 6)
#endif
            ok( igmp3_found, "%s not found.\n", ntoa( igmp3_addr ));
            ok( ssdp_found || broken(!ssdp_found) /* 239.255.255.250 is always present since Win10 */,
                "%s not found.\n", ntoa( ssdp_addr ));

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
            if(GetIpNetTable2) {
#endif
            ret = GetIpNetTable2( AF_INET, &table2 );
            ok( !ret, "got ret %lu.\n", ret );
            for (i = 0; i < table2->NumEntries; ++i)
            {
                MIB_IPNET_ROW2 *row = &table2->Table[i];
                if (row->Address.Ipv4.sin_addr.s_addr == igmp3_addr
                    || row->Address.Ipv4.sin_addr.s_addr == ssdp_addr)
                {
                    ok( row->State == NlnsPermanent, "got state %d.\n", row->State );
                    ok( !row->IsRouter, "IsRouter is set.\n" );
                    ok( !row->IsUnreachable, "IsUnreachable is set.\n" );
                }
            }
            FreeMibTable( table2 );
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
            }
#endif
        }

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i, j;

            trace( "IP net table: %lu entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                trace( "%lu: idx %lu type %lu addr %s phys",
                       i, buf->table[i].dwIndex, buf->table[i].dwType, ntoa( buf->table[i].dwAddr ));
                for (j = 0; j < buf->table[i].dwPhysAddrLen; j++)
                    printf( " %02x", buf->table[i].bPhysAddr[j] );
                printf( "\n" );
            }
        }
        free(buf);
    }
}

static void testGetIcmpStatistics(void)
{
    DWORD apiReturn;
    MIB_ICMP stats;

    /* Crashes on Vista */
    if (0) {
        apiReturn = GetIcmpStatistics(NULL);
        if (apiReturn == ERROR_NOT_SUPPORTED)
            return;
        ok(apiReturn == ERROR_INVALID_PARAMETER,
           "GetIcmpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
           apiReturn);
    }

    apiReturn = GetIcmpStatistics(&stats);
    if (apiReturn == ERROR_NOT_SUPPORTED)
    {
        skip("GetIcmpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == NO_ERROR,
       "GetIcmpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "ICMP stats:          %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:          %8lu %8lu\n", stats.stats.icmpInStats.dwMsgs, stats.stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:        %8lu %8lu\n", stats.stats.icmpInStats.dwErrors, stats.stats.icmpOutStats.dwErrors );
        trace( "    dwDestUnreachs:  %8lu %8lu\n", stats.stats.icmpInStats.dwDestUnreachs, stats.stats.icmpOutStats.dwDestUnreachs );
        trace( "    dwTimeExcds:     %8lu %8lu\n", stats.stats.icmpInStats.dwTimeExcds, stats.stats.icmpOutStats.dwTimeExcds );
        trace( "    dwParmProbs:     %8lu %8lu\n", stats.stats.icmpInStats.dwParmProbs, stats.stats.icmpOutStats.dwParmProbs );
        trace( "    dwSrcQuenchs:    %8lu %8lu\n", stats.stats.icmpInStats.dwSrcQuenchs, stats.stats.icmpOutStats.dwSrcQuenchs );
        trace( "    dwRedirects:     %8lu %8lu\n", stats.stats.icmpInStats.dwRedirects, stats.stats.icmpOutStats.dwRedirects );
        trace( "    dwEchos:         %8lu %8lu\n", stats.stats.icmpInStats.dwEchos, stats.stats.icmpOutStats.dwEchos );
        trace( "    dwEchoReps:      %8lu %8lu\n", stats.stats.icmpInStats.dwEchoReps, stats.stats.icmpOutStats.dwEchoReps );
        trace( "    dwTimestamps:    %8lu %8lu\n", stats.stats.icmpInStats.dwTimestamps, stats.stats.icmpOutStats.dwTimestamps );
        trace( "    dwTimestampReps: %8lu %8lu\n", stats.stats.icmpInStats.dwTimestampReps, stats.stats.icmpOutStats.dwTimestampReps );
        trace( "    dwAddrMasks:     %8lu %8lu\n", stats.stats.icmpInStats.dwAddrMasks, stats.stats.icmpOutStats.dwAddrMasks );
        trace( "    dwAddrMaskReps:  %8lu %8lu\n", stats.stats.icmpInStats.dwAddrMaskReps, stats.stats.icmpOutStats.dwAddrMaskReps );
    }
}

static void testGetIpStatistics(void)
{
    DWORD apiReturn;
    MIB_IPSTATS stats;

    apiReturn = GetIpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetIpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetIpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
       "GetIpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP stats:\n" );
        trace( "    dwForwarding:      %lu\n", stats.dwForwarding );
        trace( "    dwDefaultTTL:      %lu\n", stats.dwDefaultTTL );
        trace( "    dwInReceives:      %lu\n", stats.dwInReceives );
        trace( "    dwInHdrErrors:     %lu\n", stats.dwInHdrErrors );
        trace( "    dwInAddrErrors:    %lu\n", stats.dwInAddrErrors );
        trace( "    dwForwDatagrams:   %lu\n", stats.dwForwDatagrams );
        trace( "    dwInUnknownProtos: %lu\n", stats.dwInUnknownProtos );
        trace( "    dwInDiscards:      %lu\n", stats.dwInDiscards );
        trace( "    dwInDelivers:      %lu\n", stats.dwInDelivers );
        trace( "    dwOutRequests:     %lu\n", stats.dwOutRequests );
        trace( "    dwRoutingDiscards: %lu\n", stats.dwRoutingDiscards );
        trace( "    dwOutDiscards:     %lu\n", stats.dwOutDiscards );
        trace( "    dwOutNoRoutes:     %lu\n", stats.dwOutNoRoutes );
        trace( "    dwReasmTimeout:    %lu\n", stats.dwReasmTimeout );
        trace( "    dwReasmReqds:      %lu\n", stats.dwReasmReqds );
        trace( "    dwReasmOks:        %lu\n", stats.dwReasmOks );
        trace( "    dwReasmFails:      %lu\n", stats.dwReasmFails );
        trace( "    dwFragOks:         %lu\n", stats.dwFragOks );
        trace( "    dwFragFails:       %lu\n", stats.dwFragFails );
        trace( "    dwFragCreates:     %lu\n", stats.dwFragCreates );
        trace( "    dwNumIf:           %lu\n", stats.dwNumIf );
        trace( "    dwNumAddr:         %lu\n", stats.dwNumAddr );
        trace( "    dwNumRoutes:       %lu\n", stats.dwNumRoutes );
    }
}

static void testGetTcpStatistics(void)
{
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    apiReturn = GetTcpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetTcpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetTcpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetTcpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
       "GetTcpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP stats:\n" );
        trace( "    dwRtoAlgorithm: %lu\n", stats.dwRtoAlgorithm );
        trace( "    dwRtoMin:       %lu\n", stats.dwRtoMin );
        trace( "    dwRtoMax:       %lu\n", stats.dwRtoMax );
        trace( "    dwMaxConn:      %lu\n", stats.dwMaxConn );
        trace( "    dwActiveOpens:  %lu\n", stats.dwActiveOpens );
        trace( "    dwPassiveOpens: %lu\n", stats.dwPassiveOpens );
        trace( "    dwAttemptFails: %lu\n", stats.dwAttemptFails );
        trace( "    dwEstabResets:  %lu\n", stats.dwEstabResets );
        trace( "    dwCurrEstab:    %lu\n", stats.dwCurrEstab );
        trace( "    dwInSegs:       %lu\n", stats.dwInSegs );
        trace( "    dwOutSegs:      %lu\n", stats.dwOutSegs );
        trace( "    dwRetransSegs:  %lu\n", stats.dwRetransSegs );
        trace( "    dwInErrs:       %lu\n", stats.dwInErrs );
        trace( "    dwOutRsts:      %lu\n", stats.dwOutRsts );
        trace( "    dwNumConns:     %lu\n", stats.dwNumConns );
    }
}

static void testGetUdpStatistics(void)
{
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    apiReturn = GetUdpStatistics(NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetUdpStatistics is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetUdpStatistics(NULL) returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetUdpStatistics(&stats);
    ok(apiReturn == NO_ERROR,
       "GetUdpStatistics returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP stats:\n" );
        trace( "    dwInDatagrams:  %lu\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %lu\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %lu\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %lu\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %lu\n", stats.dwNumAddrs );
    }
}

static void testGetIcmpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_ICMP_EX stats;

    /* Crashes on Vista */
    if (1) {
        apiReturn = GetIcmpStatisticsEx(NULL, AF_INET);
        ok(apiReturn == ERROR_INVALID_PARAMETER,
         "GetIcmpStatisticsEx(NULL, AF_INET) returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);
    }

    apiReturn = GetIcmpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIcmpStatisticsEx(&stats, AF_BAN) returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetIcmpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetIcmpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        INT i;
        trace( "ICMP IPv4 Ex stats:           %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:              %8lu %8lu\n", stats.icmpInStats.dwMsgs, stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:            %8lu %8lu\n", stats.icmpInStats.dwErrors, stats.icmpOutStats.dwErrors );
        for (i = 0; i < 256; i++)
            trace( "    rgdwTypeCount[%3i]: %8lu %8lu\n", i, stats.icmpInStats.rgdwTypeCount[i], stats.icmpOutStats.rgdwTypeCount[i] );
    }

    apiReturn = GetIcmpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetIcmpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        INT i;
        trace( "ICMP IPv6 Ex stats:           %8s %8s\n", "in", "out" );
        trace( "    dwMsgs:              %8lu %8lu\n", stats.icmpInStats.dwMsgs, stats.icmpOutStats.dwMsgs );
        trace( "    dwErrors:            %8lu %8lu\n", stats.icmpInStats.dwErrors, stats.icmpOutStats.dwErrors );
        for (i = 0; i < 256; i++)
            trace( "    rgdwTypeCount[%3i]: %8lu %8lu\n", i, stats.icmpInStats.rgdwTypeCount[i], stats.icmpOutStats.rgdwTypeCount[i] );
    }
}

static void testGetIpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_IPSTATS stats;

    apiReturn = GetIpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatisticsEx(NULL, AF_INET) returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetIpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetIpStatisticsEx(&stats, AF_BAN) returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetIpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetIpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP IPv4 Ex stats:\n" );
        trace( "    dwForwarding:      %lu\n", stats.dwForwarding );
        trace( "    dwDefaultTTL:      %lu\n", stats.dwDefaultTTL );
        trace( "    dwInReceives:      %lu\n", stats.dwInReceives );
        trace( "    dwInHdrErrors:     %lu\n", stats.dwInHdrErrors );
        trace( "    dwInAddrErrors:    %lu\n", stats.dwInAddrErrors );
        trace( "    dwForwDatagrams:   %lu\n", stats.dwForwDatagrams );
        trace( "    dwInUnknownProtos: %lu\n", stats.dwInUnknownProtos );
        trace( "    dwInDiscards:      %lu\n", stats.dwInDiscards );
        trace( "    dwInDelivers:      %lu\n", stats.dwInDelivers );
        trace( "    dwOutRequests:     %lu\n", stats.dwOutRequests );
        trace( "    dwRoutingDiscards: %lu\n", stats.dwRoutingDiscards );
        trace( "    dwOutDiscards:     %lu\n", stats.dwOutDiscards );
        trace( "    dwOutNoRoutes:     %lu\n", stats.dwOutNoRoutes );
        trace( "    dwReasmTimeout:    %lu\n", stats.dwReasmTimeout );
        trace( "    dwReasmReqds:      %lu\n", stats.dwReasmReqds );
        trace( "    dwReasmOks:        %lu\n", stats.dwReasmOks );
        trace( "    dwReasmFails:      %lu\n", stats.dwReasmFails );
        trace( "    dwFragOks:         %lu\n", stats.dwFragOks );
        trace( "    dwFragFails:       %lu\n", stats.dwFragFails );
        trace( "    dwFragCreates:     %lu\n", stats.dwFragCreates );
        trace( "    dwNumIf:           %lu\n", stats.dwNumIf );
        trace( "    dwNumAddr:         %lu\n", stats.dwNumAddr );
        trace( "    dwNumRoutes:       %lu\n", stats.dwNumRoutes );
    }

    apiReturn = GetIpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetIpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "IP IPv6 Ex stats:\n" );
        trace( "    dwForwarding:      %lu\n", stats.dwForwarding );
        trace( "    dwDefaultTTL:      %lu\n", stats.dwDefaultTTL );
        trace( "    dwInReceives:      %lu\n", stats.dwInReceives );
        trace( "    dwInHdrErrors:     %lu\n", stats.dwInHdrErrors );
        trace( "    dwInAddrErrors:    %lu\n", stats.dwInAddrErrors );
        trace( "    dwForwDatagrams:   %lu\n", stats.dwForwDatagrams );
        trace( "    dwInUnknownProtos: %lu\n", stats.dwInUnknownProtos );
        trace( "    dwInDiscards:      %lu\n", stats.dwInDiscards );
        trace( "    dwInDelivers:      %lu\n", stats.dwInDelivers );
        trace( "    dwOutRequests:     %lu\n", stats.dwOutRequests );
        trace( "    dwRoutingDiscards: %lu\n", stats.dwRoutingDiscards );
        trace( "    dwOutDiscards:     %lu\n", stats.dwOutDiscards );
        trace( "    dwOutNoRoutes:     %lu\n", stats.dwOutNoRoutes );
        trace( "    dwReasmTimeout:    %lu\n", stats.dwReasmTimeout );
        trace( "    dwReasmReqds:      %lu\n", stats.dwReasmReqds );
        trace( "    dwReasmOks:        %lu\n", stats.dwReasmOks );
        trace( "    dwReasmFails:      %lu\n", stats.dwReasmFails );
        trace( "    dwFragOks:         %lu\n", stats.dwFragOks );
        trace( "    dwFragFails:       %lu\n", stats.dwFragFails );
        trace( "    dwFragCreates:     %lu\n", stats.dwFragCreates );
        trace( "    dwNumIf:           %lu\n", stats.dwNumIf );
        trace( "    dwNumAddr:         %lu\n", stats.dwNumAddr );
        trace( "    dwNumRoutes:       %lu\n", stats.dwNumRoutes );
    }
}

static void testGetTcpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_TCPSTATS stats;

    apiReturn = GetTcpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetTcpStatisticsEx(NULL, AF_INET); returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetTcpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER || apiReturn == ERROR_NOT_SUPPORTED,
       "GetTcpStatisticsEx(&stats, AF_BAN) returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetTcpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetTcpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP IPv4 Ex stats:\n" );
        trace( "    dwRtoAlgorithm: %lu\n", stats.dwRtoAlgorithm );
        trace( "    dwRtoMin:       %lu\n", stats.dwRtoMin );
        trace( "    dwRtoMax:       %lu\n", stats.dwRtoMax );
        trace( "    dwMaxConn:      %lu\n", stats.dwMaxConn );
        trace( "    dwActiveOpens:  %lu\n", stats.dwActiveOpens );
        trace( "    dwPassiveOpens: %lu\n", stats.dwPassiveOpens );
        trace( "    dwAttemptFails: %lu\n", stats.dwAttemptFails );
        trace( "    dwEstabResets:  %lu\n", stats.dwEstabResets );
        trace( "    dwCurrEstab:    %lu\n", stats.dwCurrEstab );
        trace( "    dwInSegs:       %lu\n", stats.dwInSegs );
        trace( "    dwOutSegs:      %lu\n", stats.dwOutSegs );
        trace( "    dwRetransSegs:  %lu\n", stats.dwRetransSegs );
        trace( "    dwInErrs:       %lu\n", stats.dwInErrs );
        trace( "    dwOutRsts:      %lu\n", stats.dwOutRsts );
        trace( "    dwNumConns:     %lu\n", stats.dwNumConns );
    }

    apiReturn = GetTcpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetTcpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "TCP IPv6 Ex stats:\n" );
        trace( "    dwRtoAlgorithm: %lu\n", stats.dwRtoAlgorithm );
        trace( "    dwRtoMin:       %lu\n", stats.dwRtoMin );
        trace( "    dwRtoMax:       %lu\n", stats.dwRtoMax );
        trace( "    dwMaxConn:      %lu\n", stats.dwMaxConn );
        trace( "    dwActiveOpens:  %lu\n", stats.dwActiveOpens );
        trace( "    dwPassiveOpens: %lu\n", stats.dwPassiveOpens );
        trace( "    dwAttemptFails: %lu\n", stats.dwAttemptFails );
        trace( "    dwEstabResets:  %lu\n", stats.dwEstabResets );
        trace( "    dwCurrEstab:    %lu\n", stats.dwCurrEstab );
        trace( "    dwInSegs:       %lu\n", stats.dwInSegs );
        trace( "    dwOutSegs:      %lu\n", stats.dwOutSegs );
        trace( "    dwRetransSegs:  %lu\n", stats.dwRetransSegs );
        trace( "    dwInErrs:       %lu\n", stats.dwInErrs );
        trace( "    dwOutRsts:      %lu\n", stats.dwOutRsts );
        trace( "    dwNumConns:     %lu\n", stats.dwNumConns );
    }
}

static void testGetUdpStatisticsEx(void)
{
    DWORD apiReturn;
    MIB_UDPSTATS stats;

    apiReturn = GetUdpStatisticsEx(NULL, AF_INET);
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetUdpStatisticsEx(NULL, AF_INET); returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetUdpStatisticsEx(&stats, AF_BAN);
    ok(apiReturn == ERROR_INVALID_PARAMETER || apiReturn == ERROR_NOT_SUPPORTED,
       "GetUdpStatisticsEx(&stats, AF_BAN) returned %ld, expected ERROR_INVALID_PARAMETER\n", apiReturn);

    apiReturn = GetUdpStatisticsEx(&stats, AF_INET);
    ok(apiReturn == NO_ERROR, "GetUdpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP IPv4 Ex stats:\n" );
        trace( "    dwInDatagrams:  %lu\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %lu\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %lu\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %lu\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %lu\n", stats.dwNumAddrs );
    }

    apiReturn = GetUdpStatisticsEx(&stats, AF_INET6);
    ok(apiReturn == NO_ERROR || broken(apiReturn == ERROR_NOT_SUPPORTED),
       "GetUdpStatisticsEx returned %ld, expected NO_ERROR\n", apiReturn);
    if (apiReturn == NO_ERROR && winetest_debug > 1)
    {
        trace( "UDP IPv6 Ex stats:\n" );
        trace( "    dwInDatagrams:  %lu\n", stats.dwInDatagrams );
        trace( "    dwNoPorts:      %lu\n", stats.dwNoPorts );
        trace( "    dwInErrors:     %lu\n", stats.dwInErrors );
        trace( "    dwOutDatagrams: %lu\n", stats.dwOutDatagrams );
        trace( "    dwNumAddrs:     %lu\n", stats.dwNumAddrs );
    }
}

static void testGetTcpTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetTcpTable(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetTcpTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetTcpTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_TCPTABLE buf = malloc(dwSize);

        apiReturn = GetTcpTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetTcpTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i;
            trace( "TCP table: %lu entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
            {
                trace( "%lu: local %s:%u remote %s:%u state %lu\n", i,
                       ntoa(buf->table[i].dwLocalAddr), ntohs(buf->table[i].dwLocalPort),
                       ntoa(buf->table[i].dwRemoteAddr), ntohs(buf->table[i].dwRemotePort),
                       buf->table[i].dwState );
            }
        }
        free(buf);
    }
}

static void testGetUdpTable(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    apiReturn = GetUdpTable(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetUdpTable is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetUdpTable(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_UDPTABLE buf = malloc(dwSize);

        apiReturn = GetUdpTable(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetUdpTable(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i;
            trace( "UDP table: %lu entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
                trace( "%lu: %s:%u\n",
                       i, ntoa( buf->table[i].dwLocalAddr ), ntohs(buf->table[i].dwLocalPort) );
        }
        free(buf);
    }
}

static void testSetTcpEntry(void)
{
    DWORD ret;
    MIB_TCPROW row;

    memset(&row, 0, sizeof(row));
    if(0) /* This test crashes in OS >= VISTA */
    {
        ret = SetTcpEntry(NULL);
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu, expected %u\n", ret, ERROR_INVALID_PARAMETER);
    }

    ret = SetTcpEntry(&row);
    if (ret == ERROR_NETWORK_ACCESS_DENIED)
    {
        win_skip("SetTcpEntry failed with access error. Skipping test.\n");
        return;
    }
    todo_wine ok( ret == ERROR_INVALID_PARAMETER, "got %lu, expected %u\n", ret, ERROR_INVALID_PARAMETER);

    row.dwState = MIB_TCP_STATE_DELETE_TCB;
    ret = SetTcpEntry(&row);
    todo_wine ok( ret == ERROR_MR_MID_NOT_FOUND || broken(ret == ERROR_INVALID_PARAMETER),
       "got %lu, expected %u\n", ret, ERROR_MR_MID_NOT_FOUND);
}

static BOOL icmp_send_echo_test_apc_expect;
static void WINAPI icmp_send_echo_test_apc_xp(void *context)
{
    ok(icmp_send_echo_test_apc_expect, "Unexpected APC execution\n");
    ok(context == (void*)0xdeadc0de, "Wrong context: %p\n", context);
    icmp_send_echo_test_apc_expect = FALSE;
}

static void WINAPI icmp_send_echo_test_apc(void *context, IO_STATUS_BLOCK *io_status, ULONG reserved)
{
    icmp_send_echo_test_apc_xp(context);
    ok(io_status->Status == 0, "Got IO Status 0x%08lx\n", io_status->Status);
    ok(io_status->Information == sizeof(ICMP_ECHO_REPLY) + 32 /* sizeof(senddata) */,
        "Got IO Information %Iu\n", io_status->Information);
}

static void testIcmpSendEcho(void)
{
    /* The APC's signature is different pre-Vista */
    const PIO_APC_ROUTINE apc = broken(LOBYTE(LOWORD(GetVersion())) < 6)
                                ? (PIO_APC_ROUTINE)icmp_send_echo_test_apc_xp
                                : icmp_send_echo_test_apc;
    HANDLE icmp;
    char senddata[32], replydata[sizeof(senddata) + sizeof(ICMP_ECHO_REPLY)];
    char replydata2[sizeof(replydata) + sizeof(IO_STATUS_BLOCK)];
    DWORD ret, error, replysz = sizeof(replydata);
    IPAddr address;
    ICMP_ECHO_REPLY *reply;
    HANDLE event;
    INT i;

    memset(senddata, 0, sizeof(senddata));

    address = htonl(INADDR_LOOPBACK);
    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(INVALID_HANDLE_VALUE, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INVALID_HANDLE) /* <= 2003 */,
        "expected 87, got %ld\n", error);

    address = htonl(INADDR_LOOPBACK);
    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(INVALID_HANDLE_VALUE, NULL, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INVALID_HANDLE) /* <= 2003 */,
        "expected 87, got %ld\n", error);

    icmp = IcmpCreateFile();
    ok (icmp != INVALID_HANDLE_VALUE, "IcmpCreateFile failed unexpectedly with error %ld\n", GetLastError());

    address = 0;
    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_NETNAME
        || broken(error == IP_BAD_DESTINATION) /* <= 2003 */,
        "expected 1214, got %ld\n", error);

    address = htonl(INADDR_LOOPBACK);
    if (0) /* crashes in XP */
    {
        ret = IcmpSendEcho(icmp, address, NULL, sizeof(senddata), NULL, replydata, replysz, 1000);
        ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    }

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    if (!ret && error == ERROR_ACCESS_DENIED)
    {
        skip( "ICMP is not available.\n" );
        return;
    }
    ok (ret, "IcmpSendEcho failed unexpectedly with error %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, NULL, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, NULL, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER, "expected 87, got %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, 0, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, NULL, 0, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %ld\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata) - 1;
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %ld\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY);
    ret = IcmpSendEcho(icmp, address, senddata, 0, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %ld\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY) + ICMP_MINLEN;
    ret = IcmpSendEcho(icmp, address, senddata, ICMP_MINLEN, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (ret, "IcmpSendEcho failed unexpectedly with error %ld\n", error);

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY) + ICMP_MINLEN;
    ret = IcmpSendEcho(icmp, address, senddata, ICMP_MINLEN + 1, NULL, replydata, replysz, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, ICMP_MINLEN, NULL, replydata, replysz - 1, 1000);
    error = GetLastError();
    ok (!ret, "IcmpSendEcho succeeded unexpectedly\n");
    ok (error == IP_GENERAL_FAILURE
        || broken(error == IP_BUF_TOO_SMALL) /* <= 2003 */,
        "expected 11050, got %ld\n", error);

    /* in windows >= vista the timeout can't be invalid */
    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 0);
    error = GetLastError();
    if (!ret) ok(error == ERROR_INVALID_PARAMETER, "expected 87, got %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, -1);
    error = GetLastError();
    if (!ret) ok(error == ERROR_INVALID_PARAMETER, "expected 87, got %ld\n", error);

    /* real ping test */
    SetLastError(0xdeadbeef);
    address = htonl(INADDR_LOOPBACK);
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    if (!ret)
    {
        skip ("Failed to ping with error %ld, is lo interface down?.\n", error);
    }
    else if (winetest_debug > 1)
    {
        PICMP_ECHO_REPLY pong = (PICMP_ECHO_REPLY) replydata;
        trace ("send addr  : %s\n", ntoa(address));
        trace ("reply addr : %s\n", ntoa(pong->Address));
        trace ("reply size : %lu\n", replysz);
        trace ("roundtrip  : %lu ms\n", pong->RoundTripTime);
        trace ("status     : %lu\n", pong->Status);
        trace ("recv size  : %u\n", pong->DataSize);
        trace ("ttl        : %u\n", pong->Options.Ttl);
        trace ("flags      : 0x%x\n", pong->Options.Flags);
    }

    /* check reply data */
    SetLastError(0xdeadbeef);
    address = htonl(INADDR_LOOPBACK);
    for (i = 0; i < ARRAY_SIZE(senddata); i++) senddata[i] = i & 0xff;
    ret = IcmpSendEcho(icmp, address, senddata, sizeof(senddata), NULL, replydata, replysz, 1000);
    error = GetLastError();
    reply = (ICMP_ECHO_REPLY *)replydata;
    ok(ret, "IcmpSendEcho failed unexpectedly\n");
    ok(error == NO_ERROR, "Expect last error:0x%08x, got:0x%08lx\n", NO_ERROR, error);
    ok(INADDR_LOOPBACK == ntohl(reply->Address), "Address mismatch, expect:%s, got: %s\n", ntoa(INADDR_LOOPBACK),
       ntoa(reply->Address));
    ok(reply->Status == IP_SUCCESS, "Expect status:0x%08x, got:0x%08lx\n", IP_SUCCESS, reply->Status);
    ok(reply->DataSize == sizeof(senddata), "Got size:%d\n", reply->DataSize);
    ok(!memcmp(senddata, reply->Data, min(sizeof(senddata), reply->DataSize)), "Data mismatch\n");


    /*
     * IcmpSendEcho2
    */
    address = 0;
    replysz = sizeof(replydata2);
    memset(senddata, 0, sizeof(senddata));

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, replysz, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok(error == ERROR_INVALID_NETNAME
        || broken(error == IP_BAD_DESTINATION) /* <= 2003 */,
        "expected 1214, got %ld\n", error);

    event = CreateEventW(NULL, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEventW failed unexpectedly with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, event, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, replysz, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 returned success unexpectedly\n");
    ok(error == ERROR_INVALID_NETNAME
        || broken(error == ERROR_IO_PENDING) /* <= 2003 */,
        "Got last error: 0x%08lx\n", error);
    if (error == ERROR_IO_PENDING)
    {
        ret = WaitForSingleObjectEx(event, 2000, TRUE);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObjectEx failed unexpectedly with %lu\n", ret);
    }

    address = htonl(INADDR_LOOPBACK);
    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, sizeof(senddata), NULL, NULL, replysz, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok(error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_NOACCESS) /* <= 2003 */,
        "expected 87, got %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, event, NULL, NULL, address, senddata, sizeof(senddata), NULL, NULL, replysz, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok(error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_NOACCESS) /* <= 2003 */,
        "expected 87, got %ld\n", error);
    ok(WaitForSingleObjectEx(event, 0, TRUE) == WAIT_TIMEOUT, "Event was unexpectedly signalled.\n");

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, 0, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok(error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, event, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, 0, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok(error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %ld\n", error);
    ok(WaitForSingleObjectEx(event, 0, TRUE) == WAIT_TIMEOUT, "Event was unexpectedly signalled.\n");

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, sizeof(senddata), NULL, NULL, 0, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok(error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %ld\n", error);

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, event, NULL, NULL, address, senddata, sizeof(senddata), NULL, NULL, 0, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 succeeded unexpectedly\n");
    ok(error == ERROR_INVALID_PARAMETER
        || broken(error == ERROR_INSUFFICIENT_BUFFER) /* <= 2003 */,
        "expected 87, got %ld\n", error);
    ok(WaitForSingleObjectEx(event, 0, TRUE) == WAIT_TIMEOUT, "Event was unexpectedly signalled.\n");

    /* synchronous tests */
#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
    SetLastError(0xdeadbeef);
    address = htonl(INADDR_LOOPBACK);
    replysz = sizeof(ICMP_ECHO_REPLY) + sizeof(IO_STATUS_BLOCK);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, 0, NULL, replydata2, replysz, 1000);
    ok(ret, "IcmpSendEcho2 failed unexpectedly with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, NULL, 0, NULL, replydata2, replysz, 1000);
    ok(ret, "IcmpSendEcho2 failed unexpectedly with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, 0, NULL, replydata2, replysz, 1000);
    ok(ret, "IcmpSendEcho2 failed unexpectedly with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    replysz = sizeof(ICMP_ECHO_REPLY) + sizeof(IO_STATUS_BLOCK) + ICMP_MINLEN;
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, ICMP_MINLEN, NULL, replydata2, replysz, 1000);
    ok(ret, "IcmpSendEcho2 failed unexpectedly with error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata2);
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, replysz, 1000);
    if (!ret)
    {
        error = GetLastError();
        skip("Failed to ping with error %ld, is lo interface down?\n", error);
    }
    else if (winetest_debug > 1)
    {
        reply = (ICMP_ECHO_REPLY*)replydata2;
        trace("send addr  : %s\n", ntoa(address));
        trace("reply addr : %s\n", ntoa(reply->Address));
        trace("reply size : %lu\n", replysz);
        trace("roundtrip  : %lu ms\n", reply->RoundTripTime);
        trace("status     : %lu\n", reply->Status);
        trace("recv size  : %u\n", reply->DataSize);
        trace("ttl        : %u\n", reply->Options.Ttl);
        trace("flags      : 0x%x\n", reply->Options.Flags);
    }

    SetLastError(0xdeadbeef);
    for (i = 0; i < ARRAY_SIZE(senddata); i++) senddata[i] = i & 0xff;
    ret = IcmpSendEcho2(icmp, NULL, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, replysz, 1000);
    error = GetLastError();
    reply = (ICMP_ECHO_REPLY*)replydata2;
    ok(ret, "IcmpSendEcho2 failed unexpectedly\n");
    ok(error == NO_ERROR, "Expect last error: 0x%08x, got: 0x%08lx\n", NO_ERROR, error);
    ok(ntohl(reply->Address) == INADDR_LOOPBACK, "Address mismatch, expect: %s, got: %s\n", ntoa(INADDR_LOOPBACK),
       ntoa(reply->Address));
    ok(reply->Status == IP_SUCCESS, "Expect status: 0x%08x, got: 0x%08lx\n", IP_SUCCESS, reply->Status);
    ok(reply->DataSize == sizeof(senddata), "Got size: %d\n", reply->DataSize);
    ok(!memcmp(senddata, reply->Data, min(sizeof(senddata), reply->DataSize)), "Data mismatch\n");
#ifdef __REACTOS__
    }
#endif

    /* asynchronous tests with event */
    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata2);
    address = htonl(INADDR_LOOPBACK);
    memset(senddata, 0, sizeof(senddata));
    ret = IcmpSendEcho2(icmp, event, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, replysz, 1000);
    error = GetLastError();
    if (!ret && error != ERROR_IO_PENDING)
    {
        skip("Failed to ping with error %ld, is lo interface down?\n", error);
    }
    else
    {
        ok(!ret, "IcmpSendEcho2 returned success unexpectedly\n");
        ok(error == ERROR_IO_PENDING, "Expect last error: 0x%08x, got: 0x%08lx\n", ERROR_IO_PENDING, error);
        ret = WaitForSingleObjectEx(event, 2000, TRUE);
        ok(ret == WAIT_OBJECT_0, "WaitForSingleObjectEx failed unexpectedly with %lu\n", ret);
        reply = (ICMP_ECHO_REPLY*)replydata2;
        ok(ntohl(reply->Address) == INADDR_LOOPBACK, "Address mismatch, expect: %s, got: %s\n", ntoa(INADDR_LOOPBACK),
           ntoa(reply->Address));
#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
        ok(reply->Status == IP_SUCCESS, "Expect status: 0x%08x, got: 0x%08lx\n", IP_SUCCESS, reply->Status);
        ok(reply->DataSize == sizeof(senddata), "Got size: %d\n", reply->DataSize);
#ifdef __REACTOS__
    }
#endif
        if (winetest_debug > 1)
        {
            reply = (ICMP_ECHO_REPLY*)replydata2;
            trace("send addr  : %s\n", ntoa(address));
            trace("reply addr : %s\n", ntoa(reply->Address));
            trace("reply size : %lu\n", replysz);
            trace("roundtrip  : %lu ms\n", reply->RoundTripTime);
            trace("status     : %lu\n", reply->Status);
            trace("recv size  : %u\n", reply->DataSize);
            trace("ttl        : %u\n", reply->Options.Ttl);
            trace("flags      : 0x%x\n", reply->Options.Flags);
        }
    }

    SetLastError(0xdeadbeef);
    for (i = 0; i < ARRAY_SIZE(senddata); i++) senddata[i] = i & 0xff;
    ret = IcmpSendEcho2(icmp, event, NULL, NULL, address, senddata, sizeof(senddata), NULL, replydata2, replysz, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 returned success unexpectedly\n");
    ok(error == ERROR_IO_PENDING, "Expect last error: 0x%08x, got: 0x%08lx\n", ERROR_IO_PENDING, error);
    ret = WaitForSingleObjectEx(event, 2000, TRUE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObjectEx failed unexpectedly with %lu\n", ret);
    reply = (ICMP_ECHO_REPLY*)replydata2;
    ok(ntohl(reply->Address) == INADDR_LOOPBACK, "Address mismatch, expect: %s, got: %s\n", ntoa(INADDR_LOOPBACK),
       ntoa(reply->Address));
#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
    ok(reply->Status == IP_SUCCESS, "Expect status: 0x%08x, got: 0x%08lx\n", IP_SUCCESS, reply->Status);
    ok(reply->DataSize == sizeof(senddata), "Got size: %d\n", reply->DataSize);
#ifdef __REACTOS__
    }
#endif
    /* pre-Vista, reply->Data is an offset; otherwise it's a pointer, so hardcode the offset */
    ok(!memcmp(senddata, reply + 1, min(sizeof(senddata), reply->DataSize)), "Data mismatch\n");

    CloseHandle(event);

    /* asynchronous tests with APC */
    SetLastError(0xdeadbeef);
    replysz = sizeof(replydata2) + 10;
    address = htonl(INADDR_LOOPBACK);
    for (i = 0; i < ARRAY_SIZE(senddata); i++) senddata[i] = ~i & 0xff;
    icmp_send_echo_test_apc_expect = TRUE;
    /*
       NOTE: Supplying both event and apc has varying behavior across Windows versions, so not tested.
    */
#if defined(__REACTOS__) && defined(_MSC_VER)
    /* The call to IcmpSendEcho2() below with the invalid APC context causes
     * stack corruption on WS03 and ReactOS when compiled with MSVC but not GCC. */
    if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
    ret = IcmpSendEcho2(icmp, NULL, apc, (void*)0xdeadc0de, address, senddata, sizeof(senddata), NULL, replydata2, replysz, 1000);
    error = GetLastError();
    ok(!ret, "IcmpSendEcho2 returned success unexpectedly\n");
    ok(error == ERROR_IO_PENDING, "Expect last error: 0x%08x, got: 0x%08lx\n", ERROR_IO_PENDING, error);
    SleepEx(200, TRUE);
    SleepEx(0, TRUE);
    ok(icmp_send_echo_test_apc_expect == FALSE, "APC was not executed!\n");
    reply = (ICMP_ECHO_REPLY*)replydata2;
    ok(ntohl(reply->Address) == INADDR_LOOPBACK, "Address mismatch, expect: %s, got: %s\n", ntoa(INADDR_LOOPBACK),
       ntoa(reply->Address));
    ok(reply->Status == IP_SUCCESS, "Expect status: 0x%08x, got: 0x%08lx\n", IP_SUCCESS, reply->Status);
    ok(reply->DataSize == sizeof(senddata), "Got size: %d\n", reply->DataSize);
    /* pre-Vista, reply->Data is an offset; otherwise it's a pointer, so hardcode the offset */
    ok(!memcmp(senddata, reply + 1, min(sizeof(senddata), reply->DataSize)), "Data mismatch\n");
#if defined(__REACTOS__) && defined(_MSC_VER)
    }
#endif

    IcmpCloseHandle(icmp);
}

static void testIcmpParseReplies( void )
{
    ICMP_ECHO_REPLY reply = { 0 };
    DWORD ret;

    SetLastError( 0xdeadbeef );
    ret = IcmpParseReplies( &reply, sizeof(reply) );
    ok( ret == 0, "ret %ld\n", ret );
    ok( GetLastError() == 0, "gle %ld\n", GetLastError() );

    reply.Status = 12345;
    SetLastError( 0xdeadbeef );
    ret = IcmpParseReplies( &reply, sizeof(reply) );
    ok( ret == 0, "ret %ld\n", ret );
    ok( GetLastError() == 12345, "gle %ld\n", GetLastError() );
    ok( reply.Status == 12345, "status %ld\n", reply.Status );

    reply.Reserved = 1;
    SetLastError( 0xdeadbeef );
    ret = IcmpParseReplies( &reply, sizeof(reply) );
    ok( ret == 1, "ret %ld\n", ret );
    ok( GetLastError() == 0xdeadbeef, "gle %ld\n", GetLastError() );
    ok( reply.Status == 12345, "status %ld\n", reply.Status );
    ok( !reply.Reserved, "reserved %d\n", reply.Reserved );

#if defined(__REACTOS__) && defined(_MSC_VER)
    /* This crashes on WS03 when compiled with MSVC. It does work on ReactOS. */
    if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
    reply.Reserved = 3;
    SetLastError( 0xdeadbeef );
    ret = IcmpParseReplies( &reply, sizeof(reply) );
    ok( ret == 3, "ret %ld\n", ret );
    ok( GetLastError() == 0xdeadbeef, "gle %ld\n", GetLastError() );
    ok( reply.Status == 12345, "status %ld\n", reply.Status );
    ok( !reply.Reserved, "reserved %d\n", reply.Reserved );
#if defined(__REACTOS__) && defined(_MSC_VER)
    }
#endif
}

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
  testIcmpParseReplies();
}

static void testGetInterfaceInfo(void)
{
    DWORD apiReturn;
    ULONG len = 0, i;

    apiReturn = GetInterfaceInfo(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetInterfaceInfo is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetInterfaceInfo returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetInterfaceInfo(NULL, &len);
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetInterfaceInfo returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PIP_INTERFACE_INFO buf = malloc(len);

        apiReturn = GetInterfaceInfo(buf, &len);
        ok(apiReturn == NO_ERROR,
           "GetInterfaceInfo(buf, &dwSize) returned %ld, expected NO_ERROR\n",
           apiReturn);

        for (i = 0; i < buf->NumAdapters; i++)
        {
            MIB_IFROW row = { .dwIndex = buf->Adapter[i].Index };
            GetIfEntry( &row );
#ifdef __REACTOS__
            if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
            ok( !wcscmp( buf->Adapter[i].Name, row.wszName ), "got %s vs %s\n",
                debugstr_w( buf->Adapter[i].Name ), debugstr_w( row.wszName ) );
#ifdef __REACTOS__
            }
#endif
            ok( row.dwType != IF_TYPE_SOFTWARE_LOOPBACK, "got loopback\n" );
        }
        free(buf);
    }
}

static void testGetAdaptersInfo(void)
{
    IP_ADAPTER_INFO *ptr, *buf;
    NET_LUID luid;
    GUID guid;
    char name[ARRAY_SIZE(ptr->AdapterName)];
    DWORD err;
    ULONG len = 0;
    MIB_IFROW row;
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if(!ConvertInterfaceIndexToLuid || !ConvertInterfaceLuidToGuid) {
        skip("Missing APIs!\n");
        return;
    }
#endif

    err = GetAdaptersInfo( NULL, NULL );
    ok( err == ERROR_INVALID_PARAMETER, "got %ld\n", err );
    err = GetAdaptersInfo( NULL, &len );
    ok( err == ERROR_NO_DATA || err == ERROR_BUFFER_OVERFLOW, "got %ld\n", err );
    if (err == ERROR_NO_DATA) return;

    buf = malloc( len );
    err = GetAdaptersInfo( buf, &len );
    ok( !err, "got %ld\n", err );
    ptr = buf;
    while (ptr)
    {
        trace( "adapter '%s', address %s/%s gateway %s/%s\n", ptr->AdapterName,
               ptr->IpAddressList.IpAddress.String, ptr->IpAddressList.IpMask.String,
               ptr->GatewayList.IpAddress.String, ptr->GatewayList.IpMask.String );
        row.dwIndex = ptr->Index;
        GetIfEntry( &row );
        ConvertInterfaceIndexToLuid( ptr->Index, &luid );
        ConvertInterfaceLuidToGuid( &luid, &guid );
        sprintf( name, "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                 guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                 guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
                 guid.Data4[6], guid.Data4[7] );
        ok( !strcmp( ptr->AdapterName, name ), "expected '%s' got '%s'\n", ptr->AdapterName, name );
        ok( !strcmp( ptr->Description, (char *)row.bDescr ), "got %s vs %s\n", ptr->Description, (char *)row.bDescr );
        ok( ptr->AddressLength == row.dwPhysAddrLen, "got %d vs %ld\n", ptr->AddressLength, row.dwPhysAddrLen );
        ok( !memcmp(ptr->Address, row.bPhysAddr, ptr->AddressLength ), "mismatch\n" );
        ok( ptr->Type == row.dwType, "got %d vs %ld\n", ptr->Type, row.dwType );
        ok( ptr->Type != MIB_IF_TYPE_LOOPBACK, "shouldn't get loopback\n" );
        ok( ptr->IpAddressList.IpAddress.String[0], "A valid IP address must be present\n" );
        ok( ptr->IpAddressList.IpMask.String[0], "A valid mask must be present\n" );
        ok( ptr->GatewayList.IpAddress.String[0], "A valid IP address must be present\n" );
        ok( ptr->GatewayList.IpMask.String[0], "A valid mask must be present\n" );
        ptr = ptr->Next;
    }
    free( buf );
}

static void testGetNetworkParams(void)
{
    DWORD apiReturn;
    ULONG len = 0;

    apiReturn = GetNetworkParams(NULL, NULL);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetNetworkParams is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INVALID_PARAMETER,
       "GetNetworkParams returned %ld, expected ERROR_INVALID_PARAMETER\n",
       apiReturn);
    apiReturn = GetNetworkParams(NULL, &len);
    ok(apiReturn == ERROR_BUFFER_OVERFLOW,
       "GetNetworkParams returned %ld, expected ERROR_BUFFER_OVERFLOW\n",
       apiReturn);
    if (apiReturn == ERROR_BUFFER_OVERFLOW) {
        PFIXED_INFO buf = malloc(len);

        apiReturn = GetNetworkParams(buf, &len);
        ok(apiReturn == NO_ERROR,
           "GetNetworkParams(buf, &dwSize) returned %ld, expected NO_ERROR\n",
           apiReturn);
        free(buf);
    }
}

static void testGetBestInterface(void)
{
    DWORD apiReturn;
    DWORD bestIfIndex;

    apiReturn = GetBestInterface( INADDR_ANY, &bestIfIndex );
    trace( "GetBestInterface([0.0.0.0], {%lu}) = %lu\n", bestIfIndex, apiReturn );
    if (apiReturn == ERROR_NOT_SUPPORTED)
    {
        skip( "GetBestInterface is not supported\n" );
        return;
    }

    apiReturn = GetBestInterface( INADDR_LOOPBACK, NULL );
    ok( apiReturn == ERROR_INVALID_PARAMETER,
        "GetBestInterface([127.0.0.1], NULL) returned %lu, expected %d\n",
        apiReturn, ERROR_INVALID_PARAMETER );

    apiReturn = GetBestInterface( INADDR_LOOPBACK, &bestIfIndex );
    ok( apiReturn == NO_ERROR,
        "GetBestInterface([127.0.0.1], {%lu}) returned %lu, expected %d\n",
        bestIfIndex, apiReturn, NO_ERROR );
}

static void testGetBestInterfaceEx(void)
{
    DWORD apiReturn;
    DWORD bestIfIndex = 0;
    struct sockaddr_in destAddr;

    memset(&destAddr, 0, sizeof(struct sockaddr_in));
    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.S_un.S_addr = INADDR_ANY;
    apiReturn = GetBestInterfaceEx( (struct sockaddr *)&destAddr, &bestIfIndex );
    trace( "GetBestInterfaceEx([0.0.0.0], {%lu}) = %lu\n", bestIfIndex, apiReturn );
    if (apiReturn == ERROR_NOT_SUPPORTED)
    {
        skip( "GetBestInterfaceEx not supported\n" );
        return;
    }

    apiReturn = GetBestInterfaceEx( NULL, NULL );
    ok( apiReturn == ERROR_INVALID_PARAMETER,
        "GetBestInterfaceEx(NULL, NULL) returned %lu, expected %d\n",
        apiReturn, ERROR_INVALID_PARAMETER );

    apiReturn = GetBestInterfaceEx( NULL, &bestIfIndex );
    ok( apiReturn == ERROR_INVALID_PARAMETER,
        "GetBestInterfaceEx(NULL, {%lu}) returned %lu, expected %d\n",
        bestIfIndex, apiReturn, ERROR_INVALID_PARAMETER );

    memset(&destAddr, 0, sizeof(struct sockaddr_in));
    apiReturn = GetBestInterfaceEx( (struct sockaddr *)&destAddr, NULL );
    ok( apiReturn == ERROR_INVALID_PARAMETER,
        "GetBestInterfaceEx(<AF_UNSPEC>, NULL) returned %lu, expected %d\n",
        apiReturn, ERROR_INVALID_PARAMETER );

    memset(&destAddr, -1, sizeof(struct sockaddr_in));
    apiReturn = GetBestInterfaceEx( (struct sockaddr *)&destAddr, NULL );
    ok( apiReturn == ERROR_INVALID_PARAMETER,
        "GetBestInterfaceEx(<INVALID>, NULL) returned %lu, expected %d\n",
        apiReturn, ERROR_INVALID_PARAMETER );

    memset(&destAddr, 0, sizeof(struct sockaddr_in));
    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.S_un.S_addr = INADDR_LOOPBACK;
    apiReturn = GetBestInterfaceEx( (struct sockaddr *)&destAddr, NULL );
    ok( apiReturn == ERROR_INVALID_PARAMETER,
        "GetBestInterfaceEx([127.0.0.1], NULL) returned %lu, expected %d\n",
        apiReturn, ERROR_INVALID_PARAMETER );

    memset(&destAddr, 0, sizeof(struct sockaddr_in));
    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.S_un.S_addr = INADDR_LOOPBACK;
    apiReturn = GetBestInterfaceEx( (struct sockaddr *)&destAddr, &bestIfIndex );
    ok( apiReturn == NO_ERROR,
        "GetBestInterfaceEx([127.0.0.1], {%lu}) returned %lu, expected %d\n",
        bestIfIndex, apiReturn, ERROR_INVALID_PARAMETER );
}

static void testGetBestRoute(void)
{
    DWORD apiReturn;
    MIB_IPFORWARDROW bestRoute;

    apiReturn = GetBestRoute( INADDR_ANY, 0, &bestRoute );
    trace( "GetBestRoute([0.0.0.0], 0, [...]) = %lu\n", apiReturn );
    if (apiReturn == ERROR_NOT_SUPPORTED)
    {
        skip( "GetBestRoute is not supported\n" );
        return;
    }

    apiReturn = GetBestRoute( INADDR_ANY, 0, NULL );
    ok( apiReturn == ERROR_INVALID_PARAMETER,
        "GetBestRoute([0.0.0.0], 0, NULL) returned %lu, expected %d\n",
        apiReturn, ERROR_INVALID_PARAMETER );

    apiReturn = GetBestRoute( INADDR_LOOPBACK, 0, &bestRoute );
    ok( apiReturn == NO_ERROR,
        "GetBestRoute([127.0.0.1], 0, NULL) returned %lu, expected %d\n",
        apiReturn, NO_ERROR );
}

/*
still-to-be-tested 98-onward functions:
IpReleaseAddress
IpRenewAddress
*/
static DWORD CALLBACK testWin98Functions(void *p)
{
  testGetInterfaceInfo();
  testGetAdaptersInfo();
  testGetNetworkParams();
  testGetBestInterface();
  testGetBestInterfaceEx();
  testGetBestRoute();
  return 0;
}

static void testGetPerAdapterInfo(void)
{
    DWORD ret, needed;
    void *buffer;

    ret = GetPerAdapterInfo(1, NULL, NULL);
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu instead of ERROR_INVALID_PARAMETER\n", ret );
    needed = 0xdeadbeef;
    ret = GetPerAdapterInfo(1, NULL, &needed);
    if (ret == ERROR_NO_DATA) return;  /* no such adapter */
    ok( ret == ERROR_BUFFER_OVERFLOW, "got %lu instead of ERROR_BUFFER_OVERFLOW\n", ret );
    ok( needed != 0xdeadbeef, "needed not set\n" );
    buffer = malloc( needed );
    ret = GetPerAdapterInfo(1, buffer, &needed);
    ok( ret == NO_ERROR, "got %lu instead of NO_ERROR\n", ret );
    free( buffer );
}

static void testNotifyAddrChange(void)
{
    DWORD ret, bytes;
    OVERLAPPED overlapped;
    HANDLE handle;
    BOOL success;

#ifdef __REACTOS__
    if (IsReactOS()) {
        skip("FIXME: testNotifyAddrChange() hangs on ReactOS! (works on Windows)\n");
        return;
    }
#endif
    handle = NULL;
    ZeroMemory(&overlapped, sizeof(overlapped));
    ret = NotifyAddrChange(&handle, &overlapped);
    ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %ld, expected ERROR_IO_PENDING\n", ret);
    ret = GetLastError();
    ok(ret == ERROR_IO_PENDING, "GetLastError returned %ld, expected ERROR_IO_PENDING\n", ret);
    success = CancelIPChangeNotify(&overlapped);
    ok(success == TRUE, "CancelIPChangeNotify returned FALSE, expected TRUE\n");
    success = GetOverlappedResult( handle, &overlapped, &bytes, TRUE );
    ok( !success && GetLastError() == ERROR_OPERATION_ABORTED, "got bret %d, err %lu.\n", success, GetLastError() );

    ZeroMemory(&overlapped, sizeof(overlapped));
    success = CancelIPChangeNotify(&overlapped);
    ok(success == FALSE, "CancelIPChangeNotify returned TRUE, expected FALSE\n");

    handle = NULL;
    ZeroMemory(&overlapped, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    ret = NotifyAddrChange(&handle, &overlapped);
    ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %ld, expected ERROR_IO_PENDING\n", ret);
    ok(handle != INVALID_HANDLE_VALUE, "NotifyAddrChange returned invalid file handle\n");
    success = GetOverlappedResult(handle, &overlapped, &bytes, FALSE);
    ok(success == FALSE, "GetOverlappedResult returned TRUE, expected FALSE\n");
    ret = GetLastError();
    ok(ret == ERROR_IO_INCOMPLETE, "GetLastError returned %ld, expected ERROR_IO_INCOMPLETE\n", ret);
    success = CancelIPChangeNotify(&overlapped);
    ok(success == TRUE, "CancelIPChangeNotify returned FALSE, expected TRUE\n");
    success = GetOverlappedResult( handle, &overlapped, &bytes, TRUE );
    ok( !success && GetLastError() == ERROR_OPERATION_ABORTED, "got bret %d, err %lu.\n", success, GetLastError() );

    if (winetest_interactive)
    {
        handle = NULL;
        ZeroMemory(&overlapped, sizeof(overlapped));
        overlapped.hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
        trace("Testing asynchronous ipv4 address change notification. Please "
              "change the ipv4 address of one of your network interfaces\n");
        ret = NotifyAddrChange(&handle, &overlapped);
        ok(ret == ERROR_IO_PENDING, "NotifyAddrChange returned %ld, expected NO_ERROR\n", ret);
        success = GetOverlappedResult(handle, &overlapped, &bytes, TRUE);
        ok(success == TRUE, "GetOverlappedResult returned FALSE, expected TRUE\n");
    }

    /* test synchronous functionality */
    if (winetest_interactive)
    {
        trace("Testing synchronous ipv4 address change notification. Please "
              "change the ipv4 address of one of your network interfaces\n");
        ret = NotifyAddrChange(NULL, NULL);
        ok(ret == NO_ERROR, "NotifyAddrChange returned %ld, expected NO_ERROR\n", ret);
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
    BOOL dns_eligible_found = FALSE;
    ULONG ret, size, osize, i;
    IP_ADAPTER_ADDRESSES *aa, *ptr;
    IP_ADAPTER_UNICAST_ADDRESS *ua;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!ConvertInterfaceLuidToGuid)
        skip("Missing APIs!\n");
#endif
    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %lu\n", ret);

    /* size should be ignored and overwritten if buffer is NULL */
    size = 0x7fffffff;
    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, NULL, &size);
    ok(ret == ERROR_BUFFER_OVERFLOW, "expected ERROR_BUFFER_OVERFLOW, got %lu\n", ret);
    if (ret != ERROR_BUFFER_OVERFLOW) return;

    /* GAA_FLAG_SKIP_FRIENDLY_NAME is ignored */
    osize = 0x7fffffff;
    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, NULL, &osize);
    ok(ret == ERROR_BUFFER_OVERFLOW, "expected ERROR_BUFFER_OVERFLOW, got %lu\n", ret);
#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) >= 6)
#endif
    ok(osize == size, "expected %ld, got %ld\n", size, osize);

    ptr = malloc(size);
    ret = GetAdaptersAddresses(AF_UNSPEC, 0, NULL, ptr, &size);
    ok(!ret, "expected ERROR_SUCCESS got %lu\n", ret);
    free(ptr);

    /* higher size must not be changed to lower size */
    size *= 2;
    osize = size;
    ptr = malloc(osize);
    ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, ptr, &osize);
    while (ret == ERROR_BUFFER_OVERFLOW)
    {
        size = osize * 2;
        osize = size;
        ptr = realloc(ptr, osize);
        ret = GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_FRIENDLY_NAME, NULL, ptr, &osize);
    }
    ok(!ret, "expected ERROR_SUCCESS got %lu\n", ret);
    ok(osize == size, "expected %ld, got %ld\n", size, osize);

    for (aa = ptr; !ret && aa; aa = aa->Next)
    {
        char temp[128], buf[39];
        IP_ADAPTER_PREFIX *prefix;
        DWORD status;
        GUID guid;

        ok(aa->Length == sizeof(IP_ADAPTER_ADDRESSES_LH) ||
           aa->Length == sizeof(IP_ADAPTER_ADDRESSES_XP),
           "Unknown structure size of %lu bytes\n", aa->Length);
        ok(aa->DnsSuffix != NULL, "DnsSuffix is not a valid pointer\n");
        ok(aa->Description != NULL, "Description is not a valid pointer\n");
        ok(aa->FriendlyName != NULL, "FriendlyName is not a valid pointer\n");

        for (i = 0; i < aa->PhysicalAddressLength; i++)
            sprintf(temp + i * 3, "%02X-", aa->PhysicalAddress[i]);
        temp[i ? i * 3 - 1 : 0] = '\0';
        trace("idx %lu name %s %s dns %s descr %s phys %s mtu %lu flags %08lx type %lu\n",
              aa->IfIndex, aa->AdapterName,
              wine_dbgstr_w(aa->FriendlyName), wine_dbgstr_w(aa->DnsSuffix),
              wine_dbgstr_w(aa->Description), temp, aa->Mtu, aa->Flags, aa->IfType );
        ua = aa->FirstUnicastAddress;
        while (ua)
        {
            ok(ua->Length == sizeof(IP_ADAPTER_UNICAST_ADDRESS_LH) ||
               ua->Length == sizeof(IP_ADAPTER_UNICAST_ADDRESS_XP),
               "Unknown structure size of %lu bytes\n", ua->Length);
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
            trace("  flags %08lx origin %u/%u state %u lifetime %lu/%lu/%lu prefix %u\n",
                  ua->Flags, ua->PrefixOrigin, ua->SuffixOrigin, ua->DadState,
                  ua->ValidLifetime, ua->PreferredLifetime, ua->LeaseLifetime,
                  ua->Length < sizeof(IP_ADAPTER_UNICAST_ADDRESS_LH) ? 0 : ua->OnLinkPrefixLength);

            if (ua->Flags & IP_ADAPTER_ADDRESS_DNS_ELIGIBLE)
                dns_eligible_found = TRUE;

#ifdef __REACTOS__
            if (LOBYTE(LOWORD(GetVersion())) >= 6) {
#endif
            if(ua->Address.lpSockaddr->sa_family == AF_INET)
                ok(aa->Ipv4Enabled == TRUE, "expected Ipv4Enabled flag to be set in interface %ls\n", aa->FriendlyName);
            else if(ua->Address.lpSockaddr->sa_family == AF_INET6)
                ok(aa->Ipv6Enabled == TRUE, "expected Ipv6Enabled flag to be set in interface %ls\n", aa->FriendlyName);
#ifdef __REACTOS__
            }
#endif

            ua = ua->Next;
        }
        for (i = 0, temp[0] = '\0'; i < ARRAY_SIZE(aa->ZoneIndices); i++)
            sprintf(temp + strlen(temp), "%ld ", aa->ZoneIndices[i]);
        trace("status %u index %lu zone %s\n", aa->OperStatus, aa->Ipv6IfIndex, temp );
        prefix = aa->FirstPrefix;
        while (prefix)
        {
            trace( "  prefix %u/%lu flags %08lx\n", prefix->Address.iSockaddrLength,
                   prefix->PrefixLength, prefix->Flags );
            prefix = prefix->Next;
        }

        if (aa->Length < sizeof(IP_ADAPTER_ADDRESSES_LH)) continue;
        trace("speed %s/%s metrics %lu/%lu guid %s type %u/%u\n",
              wine_dbgstr_longlong(aa->TransmitLinkSpeed),
              wine_dbgstr_longlong(aa->ReceiveLinkSpeed),
              aa->Ipv4Metric, aa->Ipv6Metric, wine_dbgstr_guid((GUID*) &aa->NetworkGuid),
              aa->ConnectionType, aa->TunnelType);

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
        if (ConvertInterfaceLuidToGuid) {
#endif
        status = ConvertInterfaceLuidToGuid(&aa->Luid, &guid);
        ok(!status, "got %lu\n", status);
        sprintf(buf, "{%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}",
                guid.Data1, guid.Data2, guid.Data3, guid.Data4[0], guid.Data4[1],
                guid.Data4[2], guid.Data4[3], guid.Data4[4], guid.Data4[5],
                guid.Data4[6], guid.Data4[7]);
        ok(!strcasecmp(aa->AdapterName, buf), "expected '%s' got '%s'\n", aa->AdapterName, buf);
#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
        }
#endif
    }
    ok(dns_eligible_found, "Did not find any dns eligible addresses.\n");
    free(ptr);
}

static DWORD get_extended_tcp_table( ULONG family, TCP_TABLE_CLASS class, void **table )
{
    DWORD ret, size = 0;

    *table = NULL;
    ret = pGetExtendedTcpTable( NULL, &size, TRUE, family, class, 0 );
    if (ret != ERROR_INSUFFICIENT_BUFFER) return ret;

    *table = malloc( size );
    ret = pGetExtendedTcpTable( *table, &size, TRUE, family, class, 0 );
    while (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        *table = realloc( *table, size );
        ret = pGetExtendedTcpTable( *table, &size, TRUE, family, class, 0 );
    }
    return ret;
}

static void test_GetExtendedTcpTable(void)
{
    DWORD ret;
    MIB_TCPTABLE *table;
    MIB_TCPTABLE_OWNER_PID *table_pid;
    MIB_TCPTABLE_OWNER_MODULE *table_module;

    if (!pGetExtendedTcpTable)
    {
        win_skip("GetExtendedTcpTable not available\n");
        return;
    }
    ret = pGetExtendedTcpTable( NULL, NULL, TRUE, AF_INET, TCP_TABLE_BASIC_ALL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    ret = get_extended_tcp_table( AF_INET, TCP_TABLE_BASIC_ALL, (void **)&table );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table );

    ret = get_extended_tcp_table( AF_INET, TCP_TABLE_BASIC_LISTENER, (void **)&table );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table );

    ret = get_extended_tcp_table( AF_INET, TCP_TABLE_OWNER_PID_ALL, (void **)&table_pid );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table_pid );

    ret = get_extended_tcp_table( AF_INET, TCP_TABLE_OWNER_PID_LISTENER, (void **)&table_pid );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table_pid );

    ret = get_extended_tcp_table( AF_INET, TCP_TABLE_OWNER_MODULE_ALL, (void **)&table_module );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table_module );

    ret = get_extended_tcp_table( AF_INET, TCP_TABLE_OWNER_MODULE_LISTENER, (void **)&table_module );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table_module );
}

/* Test that the TCP_TABLE_OWNER_PID_ALL table contains an entry for a socket
   we make, and associates it with our process. */
static void test_GetExtendedTcpTable_owner( int family )
{
    SOCKET sock;
    int port;
    DWORD i, ret;
    void *raw_table = NULL;

#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) < 6) {
        skip("This test is invalid for this NT version.\n");
        return;
    }
#endif
    winetest_push_context( "%s", family == AF_INET ? "AF_INET" : "AF_INET6" );

    sock = socket( family, SOCK_STREAM, IPPROTO_TCP );
    ok( sock != INVALID_SOCKET, "socket error %d\n", WSAGetLastError() );

    if (family == AF_INET)
    {
        struct sockaddr_in addr = { 0 };
        int addr_len = sizeof(addr);

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
        addr.sin_port = 0;

        ret = bind( sock, (struct sockaddr *)&addr, addr_len );
        ok( !ret, "bind error %d\n", WSAGetLastError() );
        ret = getsockname( sock, (struct sockaddr *)&addr, &addr_len );
        ok( !ret, "getsockname error %d\n", WSAGetLastError() );

        port = addr.sin_port;
    }
    else
    {
        struct sockaddr_in6 addr = { 0 };
        int addr_len = sizeof(addr);

        addr.sin6_family = AF_INET6;
        addr.sin6_addr = in6addr_loopback;
        addr.sin6_port = 0;

        ret = bind( sock, (struct sockaddr *)&addr, addr_len );
        ok( !ret, "bind error %d\n", WSAGetLastError() );
        ret = getsockname( sock, (struct sockaddr *)&addr, &addr_len );
        ok( !ret, "getsockname error %d\n", WSAGetLastError() );

        port = addr.sin6_port;
    }

    listen( sock, 1 );

    ret = get_extended_tcp_table( family, TCP_TABLE_OWNER_PID_ALL, &raw_table );
    if (ret != ERROR_SUCCESS)
    {
        skip( "error %lu getting TCP table\n", ret );
        goto done;
    }

    if (family == AF_INET)
    {
        MIB_TCPTABLE_OWNER_PID *table = raw_table;
        BOOL found_it = FALSE;
        for (i = 0; i < table->dwNumEntries; i++)
        {
            MIB_TCPROW_OWNER_PID *row = &table->table[i];
            if (row->dwLocalPort == port && row->dwLocalAddr == htonl( INADDR_LOOPBACK ))
            {
                ok( row->dwState == MIB_TCP_STATE_LISTEN, "unexpected socket state %ld\n", row->dwState );
                ok( row->dwOwningPid == GetCurrentProcessId(), "unexpected socket owner %04lx\n", row->dwOwningPid );
                found_it = TRUE;
                break;
            }
        }
        ok( found_it, "no table entry for socket\n" );
    }
    else
    {
        MIB_TCP6TABLE_OWNER_PID *table = raw_table;
        BOOL found_it = FALSE;
        for (i = 0; i < table->dwNumEntries; i++)
        {
            MIB_TCP6ROW_OWNER_PID *row = &table->table[i];
            if (row->dwLocalPort == port && IN6_IS_ADDR_LOOPBACK( (IN6_ADDR*)&row->ucLocalAddr ))
            {
                ok( row->dwState == MIB_TCP_STATE_LISTEN, "unexpected socket state %ld\n", row->dwState );
                ok( row->dwOwningPid == GetCurrentProcessId(), "unexpected socket owner %04lx\n", row->dwOwningPid );
                found_it = TRUE;
                break;
            }
        }
        ok( found_it, "no table entry for socket\n" );
    }

done:
    closesocket( sock );
    free( raw_table );

    winetest_pop_context();
}

static void test_AllocateAndGetTcpExTableFromStack(void)
{
    DWORD ret;
    MIB_TCPTABLE_OWNER_PID *table_ex = NULL;

    if (!pAllocateAndGetTcpExTableFromStack)
    {
        win_skip("AllocateAndGetTcpExTableFromStack not available\n");
        return;
    }

    if (0)
    {
        /* crashes on native */
        ret = pAllocateAndGetTcpExTableFromStack( NULL, FALSE, INVALID_HANDLE_VALUE, 0, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
        ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, INVALID_HANDLE_VALUE, 0, AF_INET );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
        ret = pAllocateAndGetTcpExTableFromStack( NULL, FALSE, GetProcessHeap(), 0, AF_INET );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
    }

    ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, GetProcessHeap(), 0, 0 );
    ok( ret == ERROR_INVALID_PARAMETER || broken(ret == ERROR_NOT_SUPPORTED) /* win2k */, "got %lu\n", ret );

    ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, GetProcessHeap(), 0, AF_INET );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );

    if (ret == NO_ERROR && winetest_debug > 1)
    {
        DWORD i;
        trace( "AllocateAndGetTcpExTableFromStack table: %lu entries\n", table_ex->dwNumEntries );
        for (i = 0; i < table_ex->dwNumEntries; i++)
        {
          char remote_ip[16];

          strcpy(remote_ip, ntoa(table_ex->table[i].dwRemoteAddr));
          trace( "%lu: local %s:%u remote %s:%u state %lu pid %lu\n", i,
                 ntoa(table_ex->table[i].dwLocalAddr), ntohs(table_ex->table[i].dwLocalPort),
                 remote_ip, ntohs(table_ex->table[i].dwRemotePort),
                 table_ex->table[i].dwState, table_ex->table[i].dwOwningPid );
        }
    }
    HeapFree(GetProcessHeap(), 0, table_ex);

    ret = pAllocateAndGetTcpExTableFromStack( (void **)&table_ex, FALSE, GetProcessHeap(), 0, AF_INET6 );
    ok( ret == ERROR_NOT_SUPPORTED, "got %lu\n", ret );
}

static DWORD get_extended_udp_table( ULONG family, UDP_TABLE_CLASS class, void **table )
{
    DWORD ret, size = 0;

    *table = NULL;
    ret = pGetExtendedUdpTable( NULL, &size, TRUE, family, class, 0 );
    if (ret != ERROR_INSUFFICIENT_BUFFER) return ret;

    *table = malloc( size );
    ret = pGetExtendedUdpTable( *table, &size, TRUE, family, class, 0 );
    while (ret == ERROR_INSUFFICIENT_BUFFER)
    {
        *table = realloc( *table, size );
        ret = pGetExtendedUdpTable( *table, &size, TRUE, family, class, 0 );
    }
    return ret;
}

static void test_GetExtendedUdpTable(void)
{
    DWORD ret;
    MIB_UDPTABLE *table;
    MIB_UDPTABLE_OWNER_PID *table_pid;
    MIB_UDPTABLE_OWNER_MODULE *table_module;

    if (!pGetExtendedUdpTable)
    {
        win_skip("GetExtendedUdpTable not available\n");
        return;
    }
    ret = pGetExtendedUdpTable( NULL, NULL, TRUE, AF_INET, UDP_TABLE_BASIC, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    ret = get_extended_udp_table( AF_INET, UDP_TABLE_BASIC, (void **)&table );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table );

    ret = get_extended_udp_table( AF_INET, UDP_TABLE_OWNER_PID, (void **)&table_pid );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table_pid );

    ret = get_extended_udp_table( AF_INET, UDP_TABLE_OWNER_MODULE, (void **)&table_module );
    ok( ret == ERROR_SUCCESS, "got %lu\n", ret );
    free( table_module );
}

/* Test that the UDP_TABLE_OWNER_PID table contains an entry for a socket we
   make, and associates it with our process. */
static void test_GetExtendedUdpTable_owner( int family )
{
    SOCKET sock;
    int port;
    DWORD i, ret;
    void *raw_table = NULL;

#ifdef __REACTOS__
    if (LOBYTE(LOWORD(GetVersion())) < 6) {
        skip("This test is invalid for this NT version.\n");
        return;
    }
#endif
    winetest_push_context( "%s", family == AF_INET ? "AF_INET" : "AF_INET6" );

    sock = socket( family, SOCK_DGRAM, IPPROTO_UDP );
    ok( sock != INVALID_SOCKET, "socket error %d\n", WSAGetLastError() );

    if (family == AF_INET)
    {
        struct sockaddr_in addr = { 0 };
        int addr_len = sizeof(addr);

        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
        addr.sin_port = 0;

        ret = bind( sock, (struct sockaddr *)&addr, addr_len );
        ok( !ret, "bind error %d\n", WSAGetLastError() );
        ret = getsockname( sock, (struct sockaddr *)&addr, &addr_len );
        ok( !ret, "getsockname error %d\n", WSAGetLastError() );

        port = addr.sin_port;
    }
    else
    {
        struct sockaddr_in6 addr = { 0 };
        int addr_len = sizeof(addr);

        addr.sin6_family = AF_INET6;
        addr.sin6_addr = in6addr_loopback;
        addr.sin6_port = 0;

        ret = bind( sock, (struct sockaddr *)&addr, addr_len );
        ok( !ret, "bind error %d\n", WSAGetLastError() );
        ret = getsockname( sock, (struct sockaddr *)&addr, &addr_len );
        ok( !ret, "getsockname error %d\n", WSAGetLastError() );

        port = addr.sin6_port;
    }

    ret = get_extended_udp_table( family, UDP_TABLE_OWNER_PID, &raw_table );
    if (ret != ERROR_SUCCESS)
    {
        skip( "error %lu getting UDP table\n", ret );
        goto done;
    }

    if (family == AF_INET)
    {
        MIB_UDPTABLE_OWNER_PID *table = raw_table;
        BOOL found_it = FALSE;
        for (i = 0; i < table->dwNumEntries; i++)
        {
            MIB_UDPROW_OWNER_PID *row = &table->table[i];
            if (row->dwLocalPort == port && row->dwLocalAddr == htonl( INADDR_LOOPBACK ))
            {
                ok( row->dwOwningPid == GetCurrentProcessId(), "unexpected socket owner %04lx\n", row->dwOwningPid );
                found_it = TRUE;
                break;
            }
        }
        ok( found_it, "no table entry for socket\n" );
    }
    else
    {
        MIB_UDP6TABLE_OWNER_PID *table = raw_table;
        BOOL found_it = FALSE;
        for (i = 0; i < table->dwNumEntries; i++)
        {
            MIB_UDP6ROW_OWNER_PID *row = &table->table[i];
            if (row->dwLocalPort == port && IN6_IS_ADDR_LOOPBACK( (IN6_ADDR*)&row->ucLocalAddr ))
            {
                ok( row->dwOwningPid == GetCurrentProcessId(), "unexpected socket owner %04lx\n", row->dwOwningPid );
                found_it = TRUE;
                break;
            }
        }
        ok( found_it, "no table entry for socket\n" );
    }

done:
    closesocket( sock );
    free( raw_table );

    winetest_pop_context();
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
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
    ok( pair_count == 0xdeadbeef, "got %lu\n", pair_count );

    pair = (SOCKADDR_IN6_PAIR *)0xdeadbeef;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, NULL, 1, 0, &pair, &pair_count );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
    ok( pair == (SOCKADDR_IN6_PAIR *)0xdeadbeef, "got %p\n", pair );
    ok( pair_count == 0xdeadbeef, "got %lu\n", pair_count );

    pair = NULL;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 1, 0, &pair, &pair_count );
    ok( ret == NO_ERROR, "got %lu\n", ret );
    ok( pair != NULL, "pair not set\n" );
    ok( pair_count >= 1, "got %lu\n", pair_count );
    ok( pair[0].SourceAddress != NULL, "src address not set\n" );
    ok( pair[0].DestinationAddress != NULL, "dst address not set\n" );
    FreeMibTable( pair );

    dst[1].sin6_family = AF_INET6;
    dst[1].sin6_addr.u.Word[5] = 0xffff;
    dst[1].sin6_addr.u.Word[6] = 0x0404;
    dst[1].sin6_addr.u.Word[7] = 0x0808;

    pair = NULL;
    pair_count = 0xdeadbeef;
    ret = pCreateSortedAddressPairs( NULL, 0, dst, 2, 0, &pair, &pair_count );
    ok( ret == NO_ERROR, "got %lu\n", ret );
    ok( pair != NULL, "pair not set\n" );
    ok( pair_count >= 2, "got %lu\n", pair_count );
    ok( pair[0].SourceAddress != NULL, "src address not set\n" );
    ok( pair[0].DestinationAddress != NULL, "dst address not set\n" );
    ok( pair[1].SourceAddress != NULL, "src address not set\n" );
    ok( pair[1].DestinationAddress != NULL, "dst address not set\n" );
    FreeMibTable( pair );
}

static IP_ADAPTER_ADDRESSES *get_adapters( ULONG flags )
{
    ULONG err, size = 4096;
    IP_ADAPTER_ADDRESSES *tmp, *ret;

    if (!(ret = malloc( size ))) return NULL;
    err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    while (err == ERROR_BUFFER_OVERFLOW)
    {
        if (!(tmp = realloc( ret, size ))) break;
        ret = tmp;
        err = GetAdaptersAddresses( AF_UNSPEC, flags, NULL, ret, &size );
    }
    if (err == ERROR_SUCCESS) return ret;
    free( ret );
    return NULL;
}

static DWORD get_interface_index(void)
{
    DWORD ret = 0;
    IP_ADAPTER_ADDRESSES *buf, *aa;

    buf = get_adapters( 0 );
    if (!buf) return 0;

    for (aa = buf; aa; aa = aa->Next)
    {
        if (aa->IfType == IF_TYPE_ETHERNET_CSMACD)
        {
            ret = aa->IfIndex;
            break;
        }
    }
    free( buf );
    return ret;
}

static void convert_luid_to_name( NET_LUID *luid, WCHAR *expect_nameW, int len )
{
    struct
    {
        const WCHAR *prefix;
        DWORD type;
    } prefixes[] =
    {
        { L"other", IF_TYPE_OTHER },
        { L"ethernet", IF_TYPE_ETHERNET_CSMACD },
        { L"tokenring", IF_TYPE_ISO88025_TOKENRING },
        { L"ppp", IF_TYPE_PPP },
        { L"loopback", IF_TYPE_SOFTWARE_LOOPBACK },
        { L"atm", IF_TYPE_ATM },
        { L"wireless", IF_TYPE_IEEE80211 },
        { L"tunnel", IF_TYPE_TUNNEL },
        { L"ieee1394", IF_TYPE_IEEE1394 }
    };
    DWORD i;
    const WCHAR *prefix = NULL;

    for (i = 0; i < ARRAY_SIZE(prefixes); i++)
    {
        if (prefixes[i].type == luid->Info.IfType)
        {
            prefix = prefixes[i].prefix;
            break;
        }
    }
#ifdef __REACTOS__
    if (prefix)
        _snwprintf( expect_nameW, len, L"%s_%d", prefix, luid->Info.NetLuidIndex );
    else
        _snwprintf( expect_nameW, len, L"iftype%d_%d", luid->Info.IfType, luid->Info.NetLuidIndex );
#else
    if (prefix)
        swprintf( expect_nameW, len, L"%s_%d", prefix, luid->Info.NetLuidIndex );
    else
        swprintf( expect_nameW, len, L"iftype%d_%d", luid->Info.IfType, luid->Info.NetLuidIndex );
#endif
}

static void test_interface_identifier_conversion(void)
{
    DWORD ret, i;
    NET_LUID luid;
    GUID guid;
    SIZE_T len;
    WCHAR nameW[IF_MAX_STRING_SIZE + 1];
    WCHAR alias[IF_MAX_STRING_SIZE + 1];
    WCHAR expect_nameW[IF_MAX_STRING_SIZE + 1];
    char nameA[IF_MAX_STRING_SIZE + 1], *name;
    char expect_nameA[IF_MAX_STRING_SIZE + 1];
    NET_IFINDEX index;
    MIB_IF_TABLE2 *table;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!ConvertInterfaceIndexToLuid || !ConvertInterfaceLuidToIndex ||
        !ConvertInterfaceLuidToGuid || !ConvertInterfaceGuidToLuid ||
        !ConvertInterfaceLuidToNameW || !ConvertInterfaceLuidToNameA ||
        !ConvertInterfaceNameToLuidW || !ConvertInterfaceNameToLuidA ||
        !pConvertInterfaceAliasToLuid || !pConvertInterfaceLuidToAlias ||
        !GetIfTable2 || !if_nametoindex || !if_indextoname) {
        skip("Missing APIs!\n");
        return;
    }
#endif
    ret = GetIfTable2( &table );
    ok( !ret, "got %ld\n", ret );

    for (i = 0; i < table->NumEntries; i++)
    {
        MIB_IF_ROW2 *row = table->Table + i;

        /* ConvertInterfaceIndexToLuid */
        memset( &luid, 0xff, sizeof(luid) );
        ret = ConvertInterfaceIndexToLuid( 0, &luid );
        ok( ret == ERROR_FILE_NOT_FOUND, "got %lu\n", ret );
        ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
        ok( !luid.Info.NetLuidIndex, "got %u\n", luid.Info.NetLuidIndex );
        ok( !luid.Info.IfType, "got %u\n", luid.Info.IfType );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceIndexToLuid( row->InterfaceIndex, &luid );
        ok( !ret, "got %lu\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceLuidToIndex */
        ret = ConvertInterfaceLuidToIndex( &luid, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

        ret = ConvertInterfaceLuidToIndex( &luid, &index );
        ok( !ret, "got %lu\n", ret );
        ok( index == row->InterfaceIndex, "mismatch\n" );

        /* ConvertInterfaceLuidToGuid */
        memset( &guid, 0xff, sizeof(guid) );
        ret = ConvertInterfaceLuidToGuid( NULL, &guid );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
        ok( guid.Data1 == 0xffffffff, "got %s\n", debugstr_guid(&guid) );

        ret = ConvertInterfaceLuidToGuid( &luid, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

        memset( &guid, 0, sizeof(guid) );
        ret = ConvertInterfaceLuidToGuid( &luid, &guid );
        ok( !ret, "got %lu\n", ret );
        ok( IsEqualGUID( &guid, &row->InterfaceGuid ), "mismatch\n" );

        /* ConvertInterfaceGuidToLuid */
        luid.Info.NetLuidIndex = 1;
        ret = ConvertInterfaceGuidToLuid( NULL, &luid );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
        ok( luid.Info.NetLuidIndex == 1, "got %u\n", luid.Info.NetLuidIndex );

        ret = ConvertInterfaceGuidToLuid( &guid, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceGuidToLuid( &guid, &luid );
        ok( !ret, "got %lu\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value ||
            broken( luid.Value != row->InterfaceLuid.Value), /* Win8 can have identical guids for two different ifaces */
            "mismatch\n" );
        if (luid.Value != row->InterfaceLuid.Value) continue;

        /* ConvertInterfaceLuidToNameW */
        ret = ConvertInterfaceLuidToNameW( &luid, NULL, 0 );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

        ret = ConvertInterfaceLuidToNameW( &luid, nameW, 0 );
        ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %lu\n", ret );

        nameW[0] = 0;
        len = ARRAY_SIZE(nameW);
        ret = ConvertInterfaceLuidToNameW( &luid, nameW, len );
        ok( !ret, "got %lu\n", ret );
        convert_luid_to_name( &luid, expect_nameW, len );
        ok( !wcscmp( nameW, expect_nameW ), "got %s vs %s\n", debugstr_w( nameW ), debugstr_w( expect_nameW ) );

        /* ConvertInterfaceLuidToNameA */
        ret = ConvertInterfaceLuidToNameA( &luid, NULL, 0 );
        ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %lu\n", ret );

        ret = ConvertInterfaceLuidToNameA( &luid, nameA, 0 );
        ok( ret == ERROR_NOT_ENOUGH_MEMORY, "got %lu\n", ret );

        nameA[0] = 0;
        len = ARRAY_SIZE(nameA);
        ret = ConvertInterfaceLuidToNameA( &luid, nameA, len );
        ok( !ret, "got %lu\n", ret );
        ok( nameA[0], "name not set\n" );

        /* ConvertInterfaceNameToLuidW */
        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidW( NULL, &luid );
        ok( ret == ERROR_INVALID_NAME, "got %lu\n", ret );
        ok( !luid.Info.Reserved, "got %x\n", luid.Info.Reserved );
        ok( luid.Info.NetLuidIndex != 0xdead, "index not set\n" );
        ok( !luid.Info.IfType, "got %u\n", luid.Info.IfType );

        ret = ConvertInterfaceNameToLuidW( nameW, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidW( nameW, &luid );
        ok( !ret, "got %lu\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceNameToLuidA */
        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidA( NULL, &luid );
        ok( ret == ERROR_INVALID_NAME, "got %lu\n", ret );
#if defined(__REACTOS__) && defined(_MSC_VER)
        ok( luid.Info.Reserved == 0xffdead, "reserved set\n" );
        ok( luid.Info.NetLuidIndex == 0xffdead, "index set\n" );
#else
        ok( luid.Info.Reserved == 0xdead, "reserved set\n" );
        ok( luid.Info.NetLuidIndex == 0xdead, "index set\n" );
#endif
        ok( luid.Info.IfType == 0xdead, "type set\n" );

        ret = ConvertInterfaceNameToLuidA( nameA, NULL );
        ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

        luid.Info.Reserved = luid.Info.NetLuidIndex = luid.Info.IfType = 0xdead;
        ret = ConvertInterfaceNameToLuidA( nameA, &luid );
        ok( !ret, "got %lu\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceAliasToLuid */
        ret = ConvertInterfaceAliasToLuid( row->Alias, &luid );
        ok( !ret, "got %lu\n", ret );
        ok( luid.Value == row->InterfaceLuid.Value, "mismatch\n" );

        /* ConvertInterfaceLuidToAlias */
        ret = ConvertInterfaceLuidToAlias( &row->InterfaceLuid, alias, ARRAY_SIZE(alias) );
        ok( !ret, "got %lu\n", ret );
        ok( !wcscmp( alias, row->Alias ), "got %s vs %s\n", wine_dbgstr_w( alias ), wine_dbgstr_w( row->Alias ) );

        index = if_nametoindex( nameA );
        ok( index == row->InterfaceIndex, "Got index %lu for %s, expected %lu\n", index, nameA, row->InterfaceIndex );
        /* Wargaming.net Game Center passes a GUID-like string. */
        index = if_nametoindex( "{00000001-0000-0000-0000-000000000000}" );
        ok( !index, "Got unexpected index %lu\n", index );
        index = if_nametoindex( wine_dbgstr_guid( &guid ) );
        ok( !index, "Got unexpected index %lu for input %s\n", index, wine_dbgstr_guid( &guid ) );

        /* if_indextoname */
        nameA[0] = 0;
        name = if_indextoname( row->InterfaceIndex, nameA );
        ConvertInterfaceLuidToNameA( &row->InterfaceLuid, expect_nameA, ARRAY_SIZE(expect_nameA) );
        ok( name == nameA, "mismatch\n" );
        ok( !strcmp( nameA, expect_nameA ), "mismatch\n" );
    }
    FreeMibTable( table );
}

static void test_interface_identifier_conversion_failure(void)
{
    DWORD ret;
    WCHAR nameW[IF_MAX_STRING_SIZE + 1];
    char nameA[IF_MAX_STRING_SIZE + 1], *name;
    NET_IFINDEX index;
    NET_LUID luid;
    GUID guid;
    static const GUID guid_zero;
    static const GUID guid_ones = { 0xffffffffUL, 0xffff, 0xffff, { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!ConvertInterfaceIndexToLuid || !ConvertInterfaceLuidToIndex ||
        !ConvertInterfaceLuidToGuid || !ConvertInterfaceGuidToLuid ||
        !ConvertInterfaceLuidToNameW || !ConvertInterfaceLuidToNameA ||
        !ConvertInterfaceNameToLuidW || !ConvertInterfaceNameToLuidA ||
        !if_nametoindex || !if_indextoname) {
        skip("Missing APIs!\n");
        return;
    }
#endif
    /* ConvertInterfaceIndexToLuid */
    ret = ConvertInterfaceIndexToLuid( 0, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", ret );

    ret = ConvertInterfaceIndexToLuid( -1, &luid );
    ok( ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %lu\n", ret );

    /* ConvertInterfaceLuidToIndex */
    ret = ConvertInterfaceLuidToIndex( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", ret );

    ret = ConvertInterfaceLuidToIndex( NULL, &index );
    ok( ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", ret );

    luid.Value = -1;
    index = -1;
    ret = ConvertInterfaceLuidToIndex( &luid, &index );
    ok( ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %lu\n", ret );
    ok( index == 0, "index shall be zero (got %lu)\n", index );

    /* ConvertInterfaceLuidToGuid */
    ret = ConvertInterfaceLuidToGuid( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", ret );

    luid.Value = -1;
    memcpy( &guid, &guid_ones, sizeof(guid) );
    ret = ConvertInterfaceLuidToGuid( &luid, &guid );
    ok( ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %lu\n", ret );
    ok( memcmp( &guid, &guid_zero, sizeof(guid) ) == 0, "guid shall be nil\n" );

    /* ConvertInterfaceGuidToLuid */
    ret = ConvertInterfaceGuidToLuid( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", ret );

    /* ConvertInterfaceLuidToNameW */
    ret = ConvertInterfaceLuidToNameW( NULL, NULL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", ret );

    memset( nameW, 0, sizeof(nameW) );
    ret = ConvertInterfaceLuidToNameW( NULL, nameW, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %lu\n", ret );
    ok( !nameW[0], "nameW shall not change\n" );

    /* ConvertInterfaceLuidToNameA */
    ret = ConvertInterfaceLuidToNameA( NULL, NULL, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    memset( nameA, 0, sizeof(nameA) );
    ret = ConvertInterfaceLuidToNameA( NULL, nameA, 0 );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
    ok( !nameA[0], "nameA shall not change\n" );

    /* ConvertInterfaceNameToLuidW */
    ret = ConvertInterfaceNameToLuidW( NULL, NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    /* ConvertInterfaceNameToLuidA */
    ret = ConvertInterfaceNameToLuidA( NULL, NULL );
    ok( ret == ERROR_INVALID_NAME, "got %lu\n", ret );

    /* if_nametoindex */
    index = if_nametoindex( NULL );
    ok( !index, "Got unexpected index %lu\n", index );

    /* if_indextoname */
    name = if_indextoname( 0, NULL );
    ok( name == NULL, "expected NULL, got %s\n", name );

    name = if_indextoname( 0, nameA );
    ok( name == NULL, "expected NULL, got %p\n", name );

    name = if_indextoname( ~0u, nameA );
    ok( name == NULL, "expected NULL, got %p\n", name );
}

static void test_GetIfEntry2(void)
{
    DWORD ret;
    MIB_IF_ROW2 row;
    NET_IFINDEX index;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!GetIfEntry2) {
        skip("Missing APIs!\n");
        return;
    }
#endif
    if (!(index = get_interface_index()))
    {
        skip( "no suitable interface found\n" );
        return;
    }

    ret = GetIfEntry2( NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    memset( &row, 0, sizeof(row) );
    ret = GetIfEntry2( &row );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = index;
    ret = GetIfEntry2( &row );
    ok( ret == NO_ERROR, "got %lu\n", ret );
    ok( row.InterfaceIndex == index, "got %lu\n", index );
}

static void test_GetIfTable2(void)
{
    DWORD ret;
    MIB_IF_TABLE2 *table;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!GetIfTable2) {
        skip("Missing APIs!\n");
        return;
    }
#endif
    table = NULL;
    ret = GetIfTable2( &table );
    ok( ret == NO_ERROR, "got %lu\n", ret );
    ok( table != NULL, "table not set\n" );
    FreeMibTable( table );
}

static void test_GetIfTable2Ex(void)
{
    DWORD ret;
    MIB_IF_TABLE2 *table;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!GetIfTable2Ex) {
        skip("Missing APIs!\n");
        return;
    }
#endif
    table = NULL;
    ret = GetIfTable2Ex( MibIfTableNormal, &table );
    ok( ret == NO_ERROR, "got %lu\n", ret );
    ok( table != NULL, "table not set\n" );
    FreeMibTable( table );

    table = NULL;
    ret = GetIfTable2Ex( MibIfTableRaw, &table );
    ok( ret == NO_ERROR, "got %lu\n", ret );
    ok( table != NULL, "table not set\n" );
    FreeMibTable( table );

    table = NULL;
    ret = GetIfTable2Ex( MibIfTableNormalWithoutStatistics, &table );
    ok( ret == NO_ERROR || broken(ret == ERROR_INVALID_PARAMETER), "got %lu\n", ret );
    ok( table != NULL || broken(!table), "table not set\n" );
    FreeMibTable( table );

    table = NULL;
    ret = GetIfTable2Ex( 3, &table );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );
    ok( !table, "table should not be set\n" );
    FreeMibTable( table );
}

static void test_GetUnicastIpAddressEntry(void)
{
    IP_ADAPTER_ADDRESSES *aa, *ptr;
    MIB_UNICASTIPADDRESS_ROW row;
    DWORD ret;

    if (!pGetUnicastIpAddressEntry)
    {
        win_skip( "GetUnicastIpAddressEntry not available\n" );
        return;
    }

    ret = pGetUnicastIpAddressEntry( NULL );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    memset( &row, 0, sizeof(row) );
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    memset( &row, 0, sizeof(row) );
    row.Address.Ipv4.sin_family = AF_INET;
    row.Address.Ipv4.sin_port = 0;
    row.Address.Ipv4.sin_addr.S_un.S_addr = 0x01020304;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_FILE_NOT_FOUND, "got %lu\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = 123;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = get_interface_index();
    row.Address.Ipv4.sin_family = AF_INET;
    row.Address.Ipv4.sin_port = 0;
    row.Address.Ipv4.sin_addr.S_un.S_addr = 0x01020304;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_NOT_FOUND, "got %lu\n", ret );

    memset( &row, 0, sizeof(row) );
    row.InterfaceIndex = 123;
    row.Address.Ipv4.sin_family = AF_INET;
    row.Address.Ipv4.sin_port = 0;
    row.Address.Ipv4.sin_addr.S_un.S_addr = 0x01020304;
    ret = pGetUnicastIpAddressEntry( &row );
    ok( ret == ERROR_FILE_NOT_FOUND, "got %lu\n", ret );

    ptr = get_adapters( GAA_FLAG_INCLUDE_ALL_INTERFACES );
    ok(ptr != NULL, "can't get adapters\n");

    for (aa = ptr; !ret && aa; aa = aa->Next)
    {
        IP_ADAPTER_UNICAST_ADDRESS *ua;

        ua = aa->FirstUnicastAddress;
        while (ua)
        {
            /* test with luid */
            memset( &row, 0, sizeof(row) );
            memcpy(&row.InterfaceLuid, &aa->Luid, sizeof(aa->Luid));
            memcpy(&row.Address, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
            ret = pGetUnicastIpAddressEntry( &row );
            ok( ret == NO_ERROR, "got %lu\n", ret );

            /* test with index */
            memset( &row, 0, sizeof(row) );
            row.InterfaceIndex = aa->IfIndex;
            memcpy(&row.Address, ua->Address.lpSockaddr, ua->Address.iSockaddrLength);
            ret = pGetUnicastIpAddressEntry( &row );
            ok( ret == NO_ERROR, "got %lu\n", ret );
            if (ret == NO_ERROR)
            {
                ok(row.InterfaceLuid.Info.Reserved == aa->Luid.Info.Reserved, "Expected %d, got %d\n",
                    aa->Luid.Info.Reserved, row.InterfaceLuid.Info.Reserved);
                ok(row.InterfaceLuid.Info.NetLuidIndex == aa->Luid.Info.NetLuidIndex, "Expected %d, got %d\n",
                    aa->Luid.Info.NetLuidIndex, row.InterfaceLuid.Info.NetLuidIndex);
                ok(row.InterfaceLuid.Info.IfType == aa->Luid.Info.IfType, "Expected %d, got %d\n",
                    aa->Luid.Info.IfType, row.InterfaceLuid.Info.IfType);
                ok(row.InterfaceIndex == aa->IfIndex, "Expected %ld, got %ld\n",
                    aa->IfIndex, row.InterfaceIndex);
                ok(row.PrefixOrigin == ua->PrefixOrigin, "Expected %d, got %d\n",
                    ua->PrefixOrigin, row.PrefixOrigin);
                ok(row.SuffixOrigin == ua->SuffixOrigin, "Expected %d, got %d\n",
                    ua->SuffixOrigin, row.SuffixOrigin);
                ok(row.ValidLifetime == ua->ValidLifetime, "Expected %ld, got %ld\n",
                    ua->ValidLifetime, row.ValidLifetime);
                ok(row.PreferredLifetime == ua->PreferredLifetime, "Expected %ld, got %ld\n",
                    ua->PreferredLifetime, row.PreferredLifetime);
                ok(row.OnLinkPrefixLength == ua->OnLinkPrefixLength, "Expected %d, got %d\n",
                    ua->OnLinkPrefixLength, row.OnLinkPrefixLength);
                ok(row.SkipAsSource == 0, "Expected 0, got %d\n", row.SkipAsSource);
                ok(row.DadState == ua->DadState, "Expected %d, got %d\n", ua->DadState, row.DadState);
                if (row.Address.si_family == AF_INET6)
                    ok(row.ScopeId.Value == row.Address.Ipv6.sin6_scope_id, "Expected %ld, got %ld\n",
                        row.Address.Ipv6.sin6_scope_id, row.ScopeId.Value);
                ok(row.CreationTimeStamp.QuadPart, "CreationTimeStamp is 0\n");
            }
            ua = ua->Next;
        }
    }
    free(ptr);
}

static void test_GetUnicastIpAddressTable(void)
{
    MIB_UNICASTIPADDRESS_TABLE *table;
    DWORD ret;
    ULONG i;

    if (!pGetUnicastIpAddressTable)
    {
        win_skip( "GetUnicastIpAddressTable not available\n" );
        return;
    }

    ret = pGetUnicastIpAddressTable(AF_UNSPEC, NULL);
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    ret = pGetUnicastIpAddressTable(AF_BAN, &table);
    ok( ret == ERROR_INVALID_PARAMETER, "got %lu\n", ret );

    ret = pGetUnicastIpAddressTable(AF_INET, &table);
    ok( ret == NO_ERROR, "got %lu\n", ret );
    trace("GetUnicastIpAddressTable(AF_INET): NumEntries %lu\n", table->NumEntries);
    FreeMibTable( table );

    ret = pGetUnicastIpAddressTable(AF_INET6, &table);
    ok( ret == NO_ERROR, "got %lu\n", ret );
    trace("GetUnicastIpAddressTable(AF_INET6): NumEntries %lu\n", table->NumEntries);
    FreeMibTable( table );

    ret = pGetUnicastIpAddressTable(AF_UNSPEC, &table);
    ok( ret == NO_ERROR, "got %lu\n", ret );
    trace("GetUnicastIpAddressTable(AF_UNSPEC): NumEntries %lu\n", table->NumEntries);
    for (i = 0; i < table->NumEntries && winetest_debug > 1; i++)
    {
        trace("Index %lu:\n", i);
        trace("Address.si_family:               %u\n", table->Table[i].Address.si_family);
        trace("InterfaceLuid.Info.Reserved:     %u\n", table->Table[i].InterfaceLuid.Info.Reserved);
        trace("InterfaceLuid.Info.NetLuidIndex: %u\n", table->Table[i].InterfaceLuid.Info.NetLuidIndex);
        trace("InterfaceLuid.Info.IfType:       %u\n", table->Table[i].InterfaceLuid.Info.IfType);
        trace("InterfaceIndex:                  %lu\n", table->Table[i].InterfaceIndex);
        trace("PrefixOrigin:                    %u\n", table->Table[i].PrefixOrigin);
        trace("SuffixOrigin:                    %u\n", table->Table[i].SuffixOrigin);
        trace("ValidLifetime:                   %lu seconds\n", table->Table[i].ValidLifetime);
        trace("PreferredLifetime:               %lu seconds\n", table->Table[i].PreferredLifetime);
        trace("OnLinkPrefixLength:              %u\n", table->Table[i].OnLinkPrefixLength);
        trace("SkipAsSource:                    %u\n", table->Table[i].SkipAsSource);
        trace("DadState:                        %u\n", table->Table[i].DadState);
        trace("ScopeId.Value:                   %lu\n", table->Table[i].ScopeId.Value);
        trace("CreationTimeStamp:               %08lx%08lx\n", table->Table[i].CreationTimeStamp.HighPart, table->Table[i].CreationTimeStamp.LowPart);
    }

    FreeMibTable( table );
}

static void test_ConvertLengthToIpv4Mask(void)
{
    DWORD ret;
    DWORD n;
    ULONG mask;
    ULONG expected;

    if (!pConvertLengthToIpv4Mask)
    {
        win_skip( "ConvertLengthToIpv4Mask not available\n" );
        return;
    }

    for (n = 0; n <= 32; n++)
    {
        mask = 0xdeadbeef;
        if (n > 0)
            expected = htonl( ~0u << (32 - n) );
        else
            expected = 0;

        ret = pConvertLengthToIpv4Mask( n, &mask );
        ok( ret == NO_ERROR, "ConvertLengthToIpv4Mask returned 0x%08lx, expected 0x%08x\n", ret, NO_ERROR );
        ok( mask == expected, "ConvertLengthToIpv4Mask mask value 0x%08lx, expected 0x%08lx\n", mask, expected );
    }

    /* Testing for out of range. In this case both mask and return are changed to indicate error. */
    mask = 0xdeadbeef;
    ret = pConvertLengthToIpv4Mask( 33, &mask );
    ok( ret == ERROR_INVALID_PARAMETER, "ConvertLengthToIpv4Mask returned 0x%08lx, expected 0x%08x\n", ret, ERROR_INVALID_PARAMETER );
    ok( mask == INADDR_NONE, "ConvertLengthToIpv4Mask mask value 0x%08lx, expected 0x%08x\n", mask, INADDR_NONE );
}

static void test_GetTcp6Table(void)
{
    DWORD ret;
    ULONG size = 0;
    PMIB_TCP6TABLE buf;

    if (!pGetTcp6Table)
    {
        win_skip("GetTcp6Table not available\n");
        return;
    }

    ret = pGetTcp6Table(NULL, &size, FALSE);
    if (ret == ERROR_NOT_SUPPORTED)
    {
        skip("GetTcp6Table is not supported\n");
        return;
    }
    ok(ret == ERROR_INSUFFICIENT_BUFFER,
       "GetTcp6Table(NULL, &size, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n", ret);
    if (ret != ERROR_INSUFFICIENT_BUFFER) return;

    buf = malloc(size);

    ret = pGetTcp6Table(buf, &size, FALSE);
    ok(ret == NO_ERROR,
       "GetTcp6Table(buf, &size, FALSE) returned %ld, expected NO_ERROR\n", ret);

    if (ret == NO_ERROR && winetest_debug > 1)
    {
        DWORD i;
        trace("TCP6 table: %lu entries\n", buf->dwNumEntries);
        for (i = 0; i < buf->dwNumEntries; i++)
        {
            trace("%lu: local %s%%%u:%u remote %s%%%u:%u state %u\n", i,
                  ntoa6(&buf->table[i].LocalAddr), ntohs(buf->table[i].dwLocalScopeId),
                  ntohs(buf->table[i].dwLocalPort), ntoa6(&buf->table[i].RemoteAddr),
                  ntohs(buf->table[i].dwRemoteScopeId), ntohs(buf->table[i].dwRemotePort),
                  buf->table[i].State);
        }
    }

    free(buf);
}

static void test_GetUdp6Table(void)
{
    DWORD apiReturn;
    ULONG dwSize = 0;

    if (!pGetUdp6Table) {
        win_skip("GetUdp6Table not available\n");
        return;
    }

    apiReturn = pGetUdp6Table(NULL, &dwSize, FALSE);
    if (apiReturn == ERROR_NOT_SUPPORTED) {
        skip("GetUdp6Table is not supported\n");
        return;
    }
    ok(apiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetUdp6Table(NULL, &dwSize, FALSE) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
       apiReturn);
    if (apiReturn == ERROR_INSUFFICIENT_BUFFER) {
        PMIB_UDP6TABLE buf = malloc(dwSize);

        apiReturn = pGetUdp6Table(buf, &dwSize, FALSE);
        ok(apiReturn == NO_ERROR,
           "GetUdp6Table(buf, &dwSize, FALSE) returned %ld, expected NO_ERROR\n",
           apiReturn);

        if (apiReturn == NO_ERROR && winetest_debug > 1)
        {
            DWORD i;
            trace( "UDP6 table: %lu entries\n", buf->dwNumEntries );
            for (i = 0; i < buf->dwNumEntries; i++)
                trace( "%lu: %s%%%u:%u\n",
                       i, ntoa6(&buf->table[i].dwLocalAddr), ntohs(buf->table[i].dwLocalScopeId), ntohs(buf->table[i].dwLocalPort) );
        }
        free(buf);
    }
}

static void test_ParseNetworkString(void)
{
    struct
    {
        char str[32];
        IN_ADDR addr;
        DWORD ret;
    }
    ipv4_address_tests[] =
    {
        {"1.2.3.4",               {{{1, 2, 3, 4}}}},
#if defined(__REACTOS__) && defined(_MSC_VER) && _MSC_VER < 1930
        {"1.2.3.4a",              {0}, ERROR_INVALID_PARAMETER},
        {"1.2.3.0x4a",            {0}, ERROR_INVALID_PARAMETER},
        {"1.2.3",                 {0}, ERROR_INVALID_PARAMETER},
        {"a1.2.3.4",              {0}, ERROR_INVALID_PARAMETER},
        {"0xdeadbeef",            {0}, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:22",            {0}, ERROR_INVALID_PARAMETER},
        {"::1",                   {0}, ERROR_INVALID_PARAMETER},
        {"winehq.org",            {0}, ERROR_INVALID_PARAMETER},
#else
        {"1.2.3.4a",              {}, ERROR_INVALID_PARAMETER},
        {"1.2.3.0x4a",            {}, ERROR_INVALID_PARAMETER},
        {"1.2.3",                 {}, ERROR_INVALID_PARAMETER},
        {"a1.2.3.4",              {}, ERROR_INVALID_PARAMETER},
        {"0xdeadbeef",            {}, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:22",            {}, ERROR_INVALID_PARAMETER},
        {"::1",                   {}, ERROR_INVALID_PARAMETER},
        {"winehq.org",            {}, ERROR_INVALID_PARAMETER},
#endif
    };
    struct
    {
        char str[32];
        IN_ADDR addr;
        DWORD port;
        DWORD ret;
    }
    ipv4_service_tests[] =
    {
        {"1.2.3.4:22",            {{{1, 2, 3, 4}}}, 22},
#if defined(__REACTOS__) && defined(_MSC_VER) && _MSC_VER < 1930
        {"winehq.org:22",         {0}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4",               {0}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:0",             {0}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:65536",         {0}, 0, ERROR_INVALID_PARAMETER},
#else
        {"winehq.org:22",         {}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4",               {}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:0",             {}, 0, ERROR_INVALID_PARAMETER},
        {"1.2.3.4:65536",         {}, 0, ERROR_INVALID_PARAMETER},
#endif
    };
    WCHAR wstr[IP6_ADDRESS_STRING_BUFFER_LENGTH] = {'1','2','7','.','0','.','0','.','1',':','2','2',0};
    NET_ADDRESS_INFO info;
    USHORT port;
    BYTE prefix_len;
    DWORD ret;
    int i;

    if (!pParseNetworkString)
    {
        win_skip("ParseNetworkString not available\n");
        return;
    }

    ret = pParseNetworkString(wstr, -1, NULL, NULL, NULL);
    ok(ret == ERROR_SUCCESS, "expected success, got %ld\n", ret);

    ret = pParseNetworkString(NULL, NET_STRING_IPV4_SERVICE, &info, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    for (i = 0; i < ARRAY_SIZE(ipv4_address_tests); i++)
    {
        MultiByteToWideChar(CP_ACP, 0, ipv4_address_tests[i].str, sizeof(ipv4_address_tests[i].str),
                            wstr, ARRAY_SIZE(wstr));
        memset(&info, 0x99, sizeof(info));
        port = 0x9999;
        prefix_len = 0x99;

        ret = pParseNetworkString(wstr, NET_STRING_IPV4_ADDRESS, &info, &port, &prefix_len);

        ok(ret == ipv4_address_tests[i].ret,
           "%s gave error %ld\n", ipv4_address_tests[i].str, ret);
        ok(info.Format == ret ? NET_ADDRESS_FORMAT_UNSPECIFIED : NET_ADDRESS_IPV4,
           "%s gave format %d\n", ipv4_address_tests[i].str, info.Format);
        ok(info.Ipv4Address.sin_addr.S_un.S_addr == (ret ? 0x99999999 : ipv4_address_tests[i].addr.S_un.S_addr),
           "%s gave address %d.%d.%d.%d\n", ipv4_address_tests[i].str,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b1, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b2,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b3, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b4);
        ok(info.Ipv4Address.sin_port == (ret ? 0x9999 : 0),
           "%s gave port %d\n", ipv4_service_tests[i].str, ntohs(info.Ipv4Address.sin_port));
        ok(port == (ret ? 0x9999 : 0),
           "%s gave port %d\n", ipv4_service_tests[i].str, port);
        ok(prefix_len == (ret ? 0x99 : 255),
           "%s gave prefix length %d\n", ipv4_service_tests[i].str, prefix_len);
    }

    for (i = 0; i < ARRAY_SIZE(ipv4_service_tests); i++)
    {
        MultiByteToWideChar(CP_ACP, 0, ipv4_service_tests[i].str, sizeof(ipv4_service_tests[i].str),
                            wstr, ARRAY_SIZE(wstr));
        memset(&info, 0x99, sizeof(info));
        port = 0x9999;
        prefix_len = 0x99;

        ret = pParseNetworkString(wstr, NET_STRING_IPV4_SERVICE, &info, &port, &prefix_len);

        ok(ret == ipv4_service_tests[i].ret,
           "%s gave error %ld\n", ipv4_service_tests[i].str, ret);
        ok(info.Format == ret ? NET_ADDRESS_FORMAT_UNSPECIFIED : NET_ADDRESS_IPV4,
           "%s gave format %d\n", ipv4_address_tests[i].str, info.Format);
        ok(info.Ipv4Address.sin_addr.S_un.S_addr == (ret ? 0x99999999 : ipv4_service_tests[i].addr.S_un.S_addr),
           "%s gave address %d.%d.%d.%d\n", ipv4_service_tests[i].str,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b1, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b2,
           info.Ipv4Address.sin_addr.S_un.S_un_b.s_b3, info.Ipv4Address.sin_addr.S_un.S_un_b.s_b4);
        ok(ntohs(info.Ipv4Address.sin_port) == (ret ? 0x9999 : ipv4_service_tests[i].port),
           "%s gave port %d\n", ipv4_service_tests[i].str, ntohs(info.Ipv4Address.sin_port));
        ok(port == (ret ? 0x9999 : ipv4_service_tests[i].port),
           "%s gave port %d\n", ipv4_service_tests[i].str, port);
        ok(prefix_len == (ret ? 0x99 : 255),
           "%s gave prefix length %d\n", ipv4_service_tests[i].str, prefix_len);
    }
}

static void WINAPI test_ipaddtess_change_callback(PVOID context, PMIB_UNICASTIPADDRESS_ROW row,
                                                 MIB_NOTIFICATION_TYPE notification_type)
{
    BOOL *callback_called = context;

    *callback_called = TRUE;

    ok(notification_type == MibInitialNotification, "Unexpected notification_type %#x.\n",
            notification_type);
    ok(!row, "Unexpected row %p.\n", row);
}

static void test_NotifyUnicastIpAddressChange(void)
{
    BOOL callback_called;
    HANDLE handle;
    DWORD ret;

    if (!pNotifyUnicastIpAddressChange)
    {
        win_skip("NotifyUnicastIpAddressChange not available.\n");
        return;
    }

    callback_called = FALSE;
    ret = pNotifyUnicastIpAddressChange(AF_INET, test_ipaddtess_change_callback,
            &callback_called, TRUE, &handle);
    ok(ret == NO_ERROR, "Unexpected ret %#lx.\n", ret);
    ok(callback_called, "Callback was not called.\n");

    ret = pCancelMibChangeNotify2(handle);
    ok(ret == NO_ERROR, "Unexpected ret %#lx.\n", ret);
    ok(!CloseHandle(handle), "CloseHandle() succeeded.\n");
}

static void test_ConvertGuidToString( void )
{
    DWORD err;
    char bufA[39];
    WCHAR bufW[39];
    GUID guid = { 0xa, 0xb, 0xc, { 0xd, 0, 0xe, 0xf } }, guid2;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!ConvertGuidToStringA || !ConvertGuidToStringW) {
        skip("Missing APIs!\n");
        return;
    }
#endif
    err = ConvertGuidToStringA( &guid, bufA, 38 );
    ok( err, "got %ld\n", err );
    err = ConvertGuidToStringA( &guid, bufA, 39 );
    ok( !err, "got %ld\n", err );
    ok( !strcmp( bufA, "{0000000A-000B-000C-0D00-0E0F00000000}" ), "got %s\n", bufA );

    err = ConvertGuidToStringW( &guid, bufW, 38 );
    ok( err, "got %ld\n", err );
    err = ConvertGuidToStringW( &guid, bufW, 39 );
    ok( !err, "got %ld\n", err );
    ok( !wcscmp( bufW, L"{0000000A-000B-000C-0D00-0E0F00000000}" ), "got %s\n", debugstr_w( bufW ) );

    err = ConvertStringToGuidW( bufW, &guid2 );
    ok( !err, "got %ld\n", err );
    ok( IsEqualGUID( &guid, &guid2 ), "guid mismatch\n" );

    err = ConvertStringToGuidW( L"foo", &guid2 );
    ok( err == ERROR_INVALID_PARAMETER, "got %ld\n", err );
}

static void test_compartments(void)
{
    NET_IF_COMPARTMENT_ID id;

#if defined(__REACTOS__) && (DLL_EXPORT_VERSION < _WIN32_WINNT_VISTA)
    if (!GetCurrentThreadCompartmentId) {
        skip("Missing APIs!\n");
        return;
    }
#endif
    id = GetCurrentThreadCompartmentId();
    ok(id == NET_IF_COMPARTMENT_ID_PRIMARY, "got %u\n", id);
}

START_TEST(iphlpapi)
{
  WSADATA wsa_data;
  WSAStartup(MAKEWORD(2, 2), &wsa_data);

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
    test_GetExtendedTcpTable_owner(AF_INET);
    test_GetExtendedTcpTable_owner(AF_INET6);
    test_GetExtendedUdpTable();
    test_GetExtendedUdpTable_owner(AF_INET);
    test_GetExtendedUdpTable_owner(AF_INET6);
    test_AllocateAndGetTcpExTableFromStack();
    test_CreateSortedAddressPairs();
    test_interface_identifier_conversion();
    test_interface_identifier_conversion_failure();
    test_GetIfEntry2();
    test_GetIfTable2();
    test_GetIfTable2Ex();
    test_GetUnicastIpAddressEntry();
    test_GetUnicastIpAddressTable();
    test_ConvertLengthToIpv4Mask();
    test_GetTcp6Table();
    test_GetUdp6Table();
    test_ParseNetworkString();
    test_NotifyUnicastIpAddressChange();
    test_ConvertGuidToString();
    test_compartments();
    freeIPHlpApi();
  }

  WSACleanup();
}
