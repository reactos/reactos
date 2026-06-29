/*
 * PROJECT:     ReactOS Networking
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        lib/iphlpapi/dhcp_reactos.c
 * PURPOSE:     DHCP helper functions for ReactOS
 * COPYRIGHT:   Copyright 2006 Ge van Geldorp <gvg@reactos.org>
 */

#ifndef WINE_DHCP_H_
#define WINE_DHCP_H_

DWORD
getDhcpInfoForAdapter(
    DWORD AdapterIndex,
    PIP_ADAPTER_INFO ptr);

#endif /* ndef WINE_DHCP_H_ */
