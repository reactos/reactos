/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/dhcpcapi/dhcpcapi.c
 * PURPOSE:         Client API for DHCP
 * COPYRIGHT:       Copyright 2005 Art Yerkes <ayerkes@speakeasy.net>
 */

#include <winsock2.h>
#include <dhcpcsdk.h>
#include <time.h>
#include <dhcp/rosdhcp_public.h>

#define NDEBUG
#include <debug.h>

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

    Result = CallNamedPipeW
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

    Result = CallNamedPipeW
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

    Result = CallNamedPipeW
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

    Result = CallNamedPipeW
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

    Result = CallNamedPipeW
        ( DHCP_PIPE_NAME, &Req, sizeof(Req), &Reply, sizeof(Reply),
          &BytesRead, DHCP_TIMEOUT );

    return Reply.Reply;
}

/*!
 * Set new TCP/IP parameters and notify DHCP client service of this
 *
 * \param[in] ServerName
 *        NULL for local machine
 *
 * \param[in] AdapterName
 *        IPHLPAPI name of adapter to change
 *
 * \param[in] NewIpAddress
 *        TRUE if IP address changes
 *
 * \param[in] IpAddress
 *        New IP address (network byte order)
 *
 * \param[in] SubnetMask
 *        New subnet mask (network byte order)
 *
 * \param[in] DhcpAction
 *        0 - don't modify
 *        1 - enable DHCP
 *        2 - disable DHCP
 *
 * \return non-zero on success
 *
 * \remarks Undocumented by Microsoft
 */
DWORD APIENTRY
DhcpNotifyConfigChange(LPWSTR ServerName,
                       LPWSTR AdapterName,
                       BOOL NewIpAddress,
                       DWORD IpIndex,
                       DWORD IpAddress,
                       DWORD SubnetMask,
                       int DhcpAction)
{
    DPRINT1("DhcpNotifyConfigChange not implemented yet\n");
    return 0;
}

/*!
 * Get DHCP info for an adapter
 *
 * \param[in] AdapterIndex
 *        Index of the adapter (iphlpapi-style) for which info is
 *        requested
 *
 * \param[out] DhcpEnabled
 *        Returns whether DHCP is enabled for the adapter
 *
 * \param[out] DhcpServer
 *        Returns DHCP server IP address (255.255.255.255 if no
 *        server reached yet), in network byte order
 *
 * \param[out] LeaseObtained
 *        Returns time at which the lease was obtained
 *
 * \param[out] LeaseExpires
 *        Returns time at which the lease will expire
 *
 * \return non-zero on success
 *
 * \remarks This is a ReactOS-only routine
 */
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

    Result = CallNamedPipeW
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

INT WINAPI
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
