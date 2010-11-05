/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/dhcpcapi/dhcpcapi.c
 * PURPOSE:         Client API for DHCP
 * COPYRIGHT:       Copyright 2005 Art Yerkes <ayerkes@speakeasy.net>
 */

#include <rosdhcp.h>

#define NDEBUG
#include <debug.h>

static HANDLE PipeHandle = INVALID_HANDLE_VALUE;

DWORD APIENTRY DhcpCApiInitialize(LPDWORD Version) {
    DWORD PipeMode;

    /* Wait for the pipe to be available */
    if (WaitNamedPipeW(DHCP_PIPE_NAME, NMPWAIT_USE_DEFAULT_WAIT))
    {
        /* It's available, let's try to open it */
        PipeHandle = CreateFileW(DHCP_PIPE_NAME,
                                 GENERIC_READ | GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL);

        /* Check if we succeeded in opening the pipe */
        if (PipeHandle == INVALID_HANDLE_VALUE)
        {
            /* We didn't */
            return GetLastError();
        }
        else
        {
            /* Change the pipe into message mode */
            PipeMode = PIPE_READMODE_MESSAGE; 
            if (!SetNamedPipeHandleState(PipeHandle, &PipeMode, NULL, NULL))
            {
                /* Mode change failed */
                CloseHandle(PipeHandle);
                PipeHandle = INVALID_HANDLE_VALUE;
                return GetLastError();
            }
            else
            {
                /* We're good to go */
                *Version = 2;
                return NO_ERROR;
            }
        }
    }
    else
    {
        /* No good, we failed */
        return GetLastError();
    }
}

VOID APIENTRY DhcpCApiCleanup() {
    CloseHandle(PipeHandle);
    PipeHandle = INVALID_HANDLE_VALUE;
}

DWORD APIENTRY DhcpQueryHWInfo( DWORD AdapterIndex,
                                     PDWORD MediaType,
                                     PDWORD Mtu,
                                     PDWORD Speed ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqQueryHWInfo;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

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

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqLeaseIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    return Reply.Reply;
}

DWORD APIENTRY DhcpReleaseIpAddressLease( DWORD AdapterIndex ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqReleaseIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    return Reply.Reply;
}

DWORD APIENTRY DhcpRenewIpAddressLease( DWORD AdapterIndex ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqRenewIpAddress;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

    return Reply.Reply;
}

DWORD APIENTRY DhcpStaticRefreshParams( DWORD AdapterIndex,
                                             DWORD Address,
                                             DWORD Netmask ) {
    COMM_DHCP_REQ Req;
    COMM_DHCP_REPLY Reply;
    DWORD BytesRead;
    BOOL Result;

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqStaticRefreshParams;
    Req.AdapterIndex = AdapterIndex;
    Req.Body.StaticRefreshParams.IPAddress = Address;
    Req.Body.StaticRefreshParams.Netmask = Netmask;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);
    if (!Result)
    {
        /* Pipe transaction failed */
        return 0;
    }

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
    DbgPrint("DHCPCSVC: DhcpNotifyConfigChange not implemented yet\n");
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

    ASSERT(PipeHandle != INVALID_HANDLE_VALUE);

    Req.Type = DhcpReqGetAdapterInfo;
    Req.AdapterIndex = AdapterIndex;

    Result = TransactNamedPipe(PipeHandle,
                               &Req, sizeof(Req),
                               &Reply, sizeof(Reply),
                               &BytesRead, NULL);

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
