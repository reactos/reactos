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
#include <time.h>
#include <dhcp/rosdhcp_public.h>

#define DHCP_TIMEOUT 1000

DWORD APIENTRY DhcpCApiInitialize(LPDWORD Version) {
    *Version = 2;
    return 0;
}

VOID APIENTRY DhcpCApiCleanup() {
}

DWORD APIENTRY DhcpQueryHWInfo( DWORD AdapterIndex,
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

DWORD APIENTRY DhcpLeaseIpAddress( DWORD AdapterIndex ) {   
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

DWORD APIENTRY DhcpReleaseIpAddressLease( DWORD AdapterIndex ) {
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

DWORD APIENTRY DhcpRenewIpAddressLease( DWORD AdapterIndex ) {
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

DWORD APIENTRY DhcpStaticRefreshParams( DWORD AdapterIndex, 
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

/*++
 * @name DhcpRosGetAdapterInfo
 * @implemented ReactOS only
 *
 * Get DHCP info for an adapter
 *
 * @param AdapterIndex
 *        Index of the adapter (iphlpapi-style) for which info is
 *        requested
 *
 * @param DhcpEnabled
 *        Returns whether DHCP is enabled for the adapter
 *
 * @param DhcpServer
 *        Returns DHCP server IP address (255.255.255.255 if no
 *        server reached yet), in network byte order
 *
 * @param LeaseObtained
 *        Returns time at which the lease was obtained
 *
 * @param LeaseExpires
 *        Returns time at which the lease will expire
 *
 * @return non-zero on success
 *
 * @remarks This is a ReactOS-only routine
 *
 *--*/
DWORD APIENTRY DhcpRosGetAdapterInfo( DWORD AdapterIndex,
                                      PBOOL DhcpEnabled,
                                      PDWORD DhcpServer,
                                      time_t *LeaseObtained,
                                      time_t *LeaseExpires )
{
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    Req.Type = DhcpReqGetAdapterInfo;
    Req.AdapterIndex = AdapterIndex;

    Result = CallNamedPipe
        ( DHCP_PIPE_NAME, &Req, sizeof(Req), &Reply, sizeof(Reply),
          &BytesRead, DHCP_TIMEOUT );

    if ( 0 != Result && 0 != Reply.Reply ) {
        *DhcpEnabled = Reply.GetAdapterInfo.DhcpEnabled;
    } else {
        *DhcpEnabled = FALSE;
    }
    if ( *DhcpEnabled ) {
        *DhcpServer = Reply.GetAdapterInfo.DhcpServer;
        *LeaseObtained = Reply.GetAdapterInfo.LeaseObtained;
        *LeaseExpires = Reply.GetAdapterInfo.LeaseExpires;
    } else {
        *DhcpServer = INADDR_NONE;
        *LeaseObtained = 0;
        *LeaseExpires = 0;
    }

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
