#ifndef _DHCPCAPI_H
#define _DHCPCAPI_H
#if __GNUC__ >= 3
#pragma GCC system_header
#endif

#include <windows.h>
#include <accctrl.h>

#ifdef __cplusplus
extern "C" {
#endif

    VOID STDCALL DhcpLeaseIpAddress( ULONG AdapterIndex );
    VOID STDCALL DhcpReleaseIpAddressLease( ULONG AdapterIndex );
    VOID STDCALL DhcpStaticRefreshParams
    ( ULONG AdapterIndex, ULONG IpAddress, ULONG NetMask );

#ifdef __cplusplus
}
#endif

#endif/*_DHCPCAPI_H*/
