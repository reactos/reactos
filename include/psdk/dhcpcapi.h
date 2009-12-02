#ifndef _DHCPCAPI_H
#define _DHCPCAPI_H

#include <windows.h>
#include <accctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

    VOID WINAPI DhcpLeaseIpAddress( ULONG AdapterIndex );
    VOID WINAPI DhcpReleaseIpAddressLease( ULONG AdapterIndex );
    VOID WINAPI DhcpStaticRefreshParams
    ( ULONG AdapterIndex, ULONG IpAddress, ULONG NetMask );

#ifdef __cplusplus
}
#endif

#endif/*_DHCPCAPI_H*/
