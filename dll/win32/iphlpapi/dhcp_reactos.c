/*
 * PROJECT:     ReactOS Networking
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/iphlpapi/dhcp_reactos.c
 * PURPOSE:     DHCP helper functions for ReactOS
 * COPYRIGHT:   Copyright 2006 Ge van Geldorp <gvg@reactos.org>
 */

#include "iphlpapi_private.h"
#include "dhcp.h"
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
    return DhcpRosGetAdapterInfo(AdapterIndex, DhcpEnabled, DhcpServer,
                                 LeaseObtained, LeaseExpires);
}
