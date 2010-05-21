/*
 * PROJECT:     ReactOS Networking
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/iphlpapi/dhcp_reactos.c
 * PURPOSE:     DHCP helper functions for ReactOS
 * COPYRIGHT:   Copyright 2006 Ge van Geldorp <gvg@reactos.org>
 */

#include "iphlpapi_private.h"
#include "dhcp.h"
#include "dhcpcsdk.h"
#include "dhcpcapi.h"
#include <assert.h>

#define NDEBUG
#include "debug.h"

DWORD APIENTRY DhcpRosGetAdapterInfo(DWORD AdapterIndex,
                                     PBOOL DhcpEnabled,
                                     PDWORD DhcpServer,
                                     time_t *LeaseObtained,
                                     time_t *LeaseExpires);

DWORD getDhcpInfoForAdapter(DWORD AdapterIndex,
                            PBOOL DhcpEnabled,
                            PDWORD DhcpServer,
                            time_t *LeaseObtained,
                            time_t *LeaseExpires)
{
    DWORD Status, Version = 0;

    Status = DhcpCApiInitialize(&Version);
    if (Status == ERROR_NOT_READY)
    {
        /* The DHCP server isn't running yet */
        *DhcpEnabled = FALSE;
        *DhcpServer = htonl(INADDR_NONE);
        *LeaseObtained = 0;
        *LeaseExpires = 0;
        return ERROR_SUCCESS;
    }
    else if (Status != ERROR_SUCCESS)
    {
        return Status;
    }

    Status = DhcpRosGetAdapterInfo(AdapterIndex, DhcpEnabled, DhcpServer,
                                   LeaseObtained, LeaseExpires);

    DhcpCApiCleanup();

    return Status;
}
