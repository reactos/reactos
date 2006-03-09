/* $Id: dllmain.c 12852 2005-01-06 13:58:04Z mf $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/dhcpcapi/dhcpcapi.c
 * PURPOSE:         Client API for DHCP
 * PROGRAMMER:      arty (ayerkes@speakeasy.net)
 * UPDATE HISTORY:
 *                  Created 12/04/2005
 */

#include <roscfg.h>
#include <winsock2.h>
#include <dhcpcsdk.h>
#include <dhcp/rosdhcp_public.h>

#define DHCP_TIMEOUT 1000

#define EXPORT __declspec(dllexport) WINAPI

DWORD EXPORT DhcpCApiInitialize(LPDWORD Version) {
    *Version = 2;
    return 0;
}

VOID EXPORT DhcpCApiCleanup() {
}

DWORD EXPORT DhcpQueryHWInfo( DWORD AdapterIndex,
                                     PDWORD MediaType, 
                                     PDWORD Mtu, 
                                     PDWORD Speed ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    Req.Type = DhcpReqQueryHWInfo;
    Req.AdapterIndex = AdapterIndex;

    Result = CallNamedPipe
        ( DHCP_PIPE_NAME, &Req, sizeof(Req), &Reply, sizeof(Reply),
          &BytesRead, DHCP_TIMEOUT );

    if( !Reply.Reply ) return 0;
    else {
        *MediaType = Reply.QueryHWInfo.MediaType;
        *Mtu = Reply.QueryHWInfo.Mtu;
        *Speed = Reply.QueryHWInfo.Speed;
        return 1;
    }
}

DWORD EXPORT DhcpLeaseIpAddress( DWORD AdapterIndex ) {   
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    Req.Type = DhcpReqLeaseIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = CallNamedPipe
        ( DHCP_PIPE_NAME, &Req, sizeof(Req), &Reply, sizeof(Reply),
          &BytesRead, DHCP_TIMEOUT );

    return Reply.Reply;
}

DWORD EXPORT DhcpReleaseIpAddressLease( DWORD AdapterIndex ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    Req.Type = DhcpReqReleaseIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = CallNamedPipe
        ( DHCP_PIPE_NAME, &Req, sizeof(Req), &Reply, sizeof(Reply),
          &BytesRead, DHCP_TIMEOUT );

    return Reply.Reply;
}

DWORD EXPORT DhcpRenewIpAddressLease( DWORD AdapterIndex ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    Req.Type = DhcpReqRenewIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = CallNamedPipe
        ( DHCP_PIPE_NAME, &Req, sizeof(Req), &Reply, sizeof(Reply),
          &BytesRead, DHCP_TIMEOUT );

    return Reply.Reply;
}

DWORD EXPORT DhcpStaticRefreshParams( DWORD AdapterIndex, 
                                             DWORD Address, 
                                             DWORD Netmask ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    Req.Type = DhcpReqStaticRefreshParams;
    Req.AdapterIndex = AdapterIndex;
    Req.Body.StaticRefreshParams.IPAddress = Address;
    Req.Body.StaticRefreshParams.Netmask = Netmask;

    Result = CallNamedPipe
        ( DHCP_PIPE_NAME, &Req, sizeof(Req), &Reply, sizeof(Reply),
          &BytesRead, DHCP_TIMEOUT );

    return Reply.Reply;
}

INT STDCALL
DllMain(PVOID hinstDll,
	ULONG dwReason,
	PVOID reserved)
{
   switch (dwReason)
     {
     case DLL_PROCESS_ATTACH:
	DisableThreadLibraryCalls(hinstDll);
	break;

     case DLL_PROCESS_DETACH:
	break;
     }

   return TRUE;
}

/* EOF */
