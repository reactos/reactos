/* WINE iphlpapi.h
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef WINE_IPHLPAPI_H__
#define WINE_IPHLPAPI_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <iprtrmib.h>
#include <ipexport.h>
#include <iptypes.h>

DWORD
STDCALL
NhpAllocateAndGetInterfaceInfoFromStack(
    OUT IP_INTERFACE_NAME_INFO **ppTable,
    OUT PDWORD                 pdwCount,
    IN BOOL                    bOrder,
    IN HANDLE                  hHeap,
    IN DWORD                   dwFlags
    );
DWORD STDCALL AllocateAndGetIfTableFromStack(PMIB_IFTABLE *ppIfTable,
 BOOL bOrder, HANDLE heap, DWORD flags);
DWORD STDCALL AllocateAndGetIpAddrTableFromStack(PMIB_IPADDRTABLE *ppIpAddrTable,
 BOOL bOrder, HANDLE heap, DWORD flags);
DWORD STDCALL AllocateAndGetIpForwardTableFromStack(PMIB_IPFORWARDTABLE *
 ppIpForwardTable, BOOL bOrder, HANDLE heap, DWORD flags);
DWORD STDCALL AllocateAndGetIpNetTableFromStack(PMIB_IPNETTABLE *ppIpNetTable,
 BOOL bOrder, HANDLE heap, DWORD flags);
DWORD STDCALL AllocateAndGetTcpTableFromStack(PMIB_TCPTABLE *ppTcpTable,
 BOOL bOrder, HANDLE heap, DWORD flags);
DWORD STDCALL AllocateAndGetUdpTableFromStack(PMIB_UDPTABLE *ppUdpTable,
 BOOL bOrder, HANDLE heap, DWORD flags);

DWORD STDCALL GetNumberOfInterfaces(PDWORD pdwNumIf);

DWORD STDCALL GetIfEntry(PMIB_IFROW pIfRow);

DWORD STDCALL GetIfTable(PMIB_IFTABLE pIfTable, PULONG pdwSize, BOOL bOrder);

DWORD STDCALL GetIpAddrTable(PMIB_IPADDRTABLE pIpAddrTable, PULONG pdwSize,
 BOOL bOrder);

DWORD STDCALL GetIpNetTable(PMIB_IPNETTABLE pIpNetTable, PULONG pdwSize,
 BOOL bOrder);

DWORD STDCALL GetIpForwardTable(PMIB_IPFORWARDTABLE pIpForwardTable,
 PULONG pdwSize, BOOL bOrder);

DWORD STDCALL GetTcpTable(PMIB_TCPTABLE pTcpTable, PDWORD pdwSize, BOOL bOrder);

DWORD STDCALL GetUdpTable(PMIB_UDPTABLE pUdpTable, PDWORD pdwSize, BOOL bOrder);

DWORD STDCALL GetIpStatistics(PMIB_IPSTATS pStats);

DWORD STDCALL GetIpStatisticsEx(PMIB_IPSTATS pStats, DWORD dwFamily);

DWORD STDCALL GetIcmpStatistics(PMIB_ICMP pStats);

DWORD STDCALL GetTcpStatistics(PMIB_TCPSTATS pStats);

DWORD STDCALL GetTcpStatisticsEx(PMIB_TCPSTATS pStats, DWORD dwFamily);

DWORD STDCALL GetUdpStatistics(PMIB_UDPSTATS pStats);

DWORD STDCALL GetUdpStatisticsEx(PMIB_UDPSTATS pStats, DWORD dwFamily);

DWORD STDCALL SetIfEntry(PMIB_IFROW pIfRow);

DWORD STDCALL CreateIpForwardEntry(PMIB_IPFORWARDROW pRoute);

DWORD STDCALL SetIpForwardEntry(PMIB_IPFORWARDROW pRoute);

DWORD STDCALL DeleteIpForwardEntry(PMIB_IPFORWARDROW pRoute);

DWORD STDCALL SetIpStatistics(PMIB_IPSTATS pIpStats);

DWORD STDCALL SetIpTTL(UINT nTTL);

DWORD STDCALL CreateIpNetEntry(PMIB_IPNETROW pArpEntry);

DWORD STDCALL SetIpNetEntry(PMIB_IPNETROW pArpEntry);

DWORD STDCALL DeleteIpNetEntry(PMIB_IPNETROW pArpEntry);

DWORD STDCALL FlushIpNetTable(DWORD dwIfIndex);

DWORD STDCALL CreateProxyArpEntry(DWORD dwAddress, DWORD dwMask,
 DWORD dwIfIndex);

DWORD STDCALL DeleteProxyArpEntry(DWORD dwAddress, DWORD dwMask,
 DWORD dwIfIndex);

DWORD STDCALL SetTcpEntry(PMIB_TCPROW pTcpRow);

DWORD STDCALL GetInterfaceInfo(PIP_INTERFACE_INFO pIfTable, PULONG dwOutBufLen);

DWORD STDCALL GetUniDirectionalAdapterInfo(
 PIP_UNIDIRECTIONAL_ADAPTER_ADDRESS pIPIfInfo, PULONG dwOutBufLen);

DWORD STDCALL GetBestInterface(IPAddr dwDestAddr, PDWORD pdwBestIfIndex);

DWORD STDCALL GetBestRoute(DWORD dwDestAddr, DWORD dwSourceAddr,
 PMIB_IPFORWARDROW   pBestRoute);

DWORD STDCALL NotifyAddrChange(PHANDLE Handle, LPOVERLAPPED overlapped);

DWORD STDCALL NotifyRouteChange(PHANDLE Handle, LPOVERLAPPED overlapped);

DWORD STDCALL GetAdapterIndex(LPWSTR AdapterName,PULONG IfIndex);

DWORD STDCALL AddIPAddress(IPAddr Address, IPMask IpMask, DWORD IfIndex,
 PULONG NTEContext, PULONG NTEInstance);

DWORD STDCALL DeleteIPAddress(ULONG NTEContext);

DWORD STDCALL GetNetworkParams(PFIXED_INFO pFixedInfo, PULONG pOutBufLen);

DWORD STDCALL GetAdaptersInfo(PIP_ADAPTER_INFO pAdapterInfo, PULONG pOutBufLen);

DWORD STDCALL GetPerAdapterInfo(ULONG IfIndex,
 PIP_PER_ADAPTER_INFO pPerAdapterInfo, PULONG pOutBufLen);

DWORD STDCALL IpReleaseAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo);

DWORD STDCALL IpRenewAddress(PIP_ADAPTER_INDEX_MAP AdapterInfo);

DWORD STDCALL SendARP(IPAddr DestIP, IPAddr SrcIP, PULONG pMacAddr,
 PULONG  PhyAddrLen);

BOOL STDCALL GetRTTAndHopCount(IPAddr DestIpAddress, PULONG HopCount,
 ULONG  MaxHops, PULONG RTT);

DWORD STDCALL GetFriendlyIfIndex(DWORD IfIndex);

DWORD STDCALL EnableRouter(HANDLE* pHandle, OVERLAPPED* pOverlapped);

DWORD STDCALL UnenableRouter(OVERLAPPED* pOverlapped, LPDWORD lpdwEnableCount);

DWORD STDCALL GetIcmpStatisticsEx(PMIB_ICMP_EX pStats,DWORD dwFamily);

DWORD STDCALL NhpAllocateAndGetInterfaceInfoFromStack(IP_INTERFACE_NAME_INFO **ppTable,PDWORD pdwCount,BOOL bOrder,HANDLE hHeap,DWORD dwFlags);

DWORD STDCALL GetBestInterfaceEx(struct sockaddr *pDestAddr,PDWORD pdwBestIfIndex);

BOOL STDCALL CancelIPChangeNotify(LPOVERLAPPED notifyOverlapped);

PIP_ADAPTER_ORDER_MAP STDCALL GetAdapterOrderMap(VOID);

DWORD STDCALL GetAdaptersAddresses(ULONG Family,DWORD Flags,PVOID Reserved,PIP_ADAPTER_ADDRESSES pAdapterAddresses,PULONG pOutBufLen);

DWORD STDCALL DisableMediaSense(HANDLE *pHandle,OVERLAPPED *pOverLapped);

DWORD STDCALL RestoreMediaSense(OVERLAPPED* pOverlapped,LPDWORD lpdwEnableCount);

DWORD STDCALL GetIpErrorString(IP_STATUS ErrorCode,PWCHAR Buffer,PDWORD Size);

#ifdef __cplusplus
}
#endif

#endif /* WINE_IPHLPAPI_H__ */
