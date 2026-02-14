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
DhcpNotifyConfigChange(
    _In_ LPWSTR ServerName,
    _In_ LPWSTR AdapterName,
    _In_ BOOL NewIpAddress,
    _In_ DWORD IpIndex,
    _In_ DWORD IpAddress,
    _In_ DWORD SubnetMask,
    _In_ INT DhcpAction);

DWORD
APIENTRY
DhcpQueryHWInfo(
    _In_ DWORD AdapterIndex,
    _Out_ PDWORD MediaType,
    _Out_ PDWORD Mtu,
    _Out_ PDWORD Speed);

DWORD
APIENTRY
DhcpReleaseParameters(
    _In_ PWSTR AdapterName);

DWORD APIENTRY DhcpStaticRefreshParams( DWORD AdapterIndex,
                                             DWORD Address,
                                             DWORD Netmask );

#ifdef __cplusplus
}
#endif

#endif/*_DHCPCAPI_H*/
