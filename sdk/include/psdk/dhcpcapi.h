#ifndef _DHCPCAPI_H
#define _DHCPCAPI_H

#ifdef __cplusplus
extern "C" {
#endif

DWORD
APIENTRY
DhcpAcquireParameters(
    _In_ PWSTR AdapterName);

DWORD
APIENTRY
DhcpEnumClasses(
    _In_ DWORD Unknown1,
    _In_ PWSTR AdapterName,
    _In_ DWORD Unknown3,
    _In_ DWORD Unknown4);

DWORD
APIENTRY
DhcpHandlePnPEvent(
    _In_ DWORD Unknown1,
    _In_ DWORD Unknown2,
    _In_ LPWSTR AdapterName,
    _In_ DWORD Unknown4,
    _In_ DWORD Unknown5);

DWORD
APIENTRY
DhcpReleaseParameters(
    _In_ PWSTR AdapterName);

DWORD APIENTRY DhcpQueryHWInfo( DWORD AdapterIndex,
                                     PDWORD MediaType,
                                     PDWORD Mtu,
                                     PDWORD Speed );
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

#ifdef __cplusplus
}
#endif

#endif/*_DHCPCAPI_H*/
