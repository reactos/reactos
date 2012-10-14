#ifndef _DHCPCAPI_H
#define _DHCPCAPI_H

#ifdef __cplusplus
extern "C" {
#endif

DWORD APIENTRY DhcpLeaseIpAddress( DWORD AdapterIndex );
DWORD APIENTRY DhcpQueryHWInfo( DWORD AdapterIndex,
                                     PDWORD MediaType,
                                     PDWORD Mtu,
                                     PDWORD Speed );
DWORD APIENTRY DhcpReleaseIpAddressLease( DWORD AdapterIndex );
DWORD APIENTRY DhcpRenewIpAddressLease( DWORD AdapterIndex );
DWORD APIENTRY DhcpStaticRefreshParams( DWORD AdapterIndex,
                                             DWORD Address,
                                             DWORD Netmask );
DWORD APIENTRY
DhcpNotifyConfigChange(LPWSTR ServerName,
                       LPWSTR AdapterName,
                       BOOL NewIpAddress,
                       DWORD IpIndex,
                       DWORD IpAddress,
                       DWORD SubnetMask,
                       int DhcpAction);
DWORD APIENTRY DhcpRosGetAdapterInfo( DWORD AdapterIndex,
                                      PBOOL DhcpEnabled,
                                      PDWORD DhcpServer,
                                      time_t *LeaseObtained,
                                      time_t *LeaseExpires );

#ifdef __cplusplus
}
#endif

#endif/*_DHCPCAPI_H*/
