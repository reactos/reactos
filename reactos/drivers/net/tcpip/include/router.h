/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/router.h
 * PURPOSE:     IP routing definitions
 */
#ifndef __ROUTER_H
#define __ROUTER_H

#include <neighbor.h>


/* Forward Information Base Entry */
typedef struct _FIB_ENTRY {
    LIST_ENTRY ListEntry;         /* Entry on list */
    ULONG RefCount;               /* Reference count */
    PIP_ADDRESS NetworkAddress;   /* Address of network */
    PIP_ADDRESS Netmask;          /* Netmask of network */
    PNET_TABLE_ENTRY NTE;         /* Pointer to NTE to use */
    PNEIGHBOR_CACHE_ENTRY Router; /* Pointer to NCE of router to use */
    UINT Metric;                  /* Cost of this route */
} FIB_ENTRY, *PFIB_ENTRY;


PNET_TABLE_ENTRY RouterFindBestNTE(
    PIP_INTERFACE Interface,
    PIP_ADDRESS Destination);

PIP_INTERFACE RouterFindOnLinkInterface(
    PIP_ADDRESS Address,
    PNET_TABLE_ENTRY NTE);

PFIB_ENTRY RouterAddRoute(
    PIP_ADDRESS NetworkAddress,
    PIP_ADDRESS Netmask,
    PNET_TABLE_ENTRY NTE,
    PNEIGHBOR_CACHE_ENTRY Router,
    UINT Metric);

PNEIGHBOR_CACHE_ENTRY RouterGetRoute(
    PIP_ADDRESS Destination,
    PNET_TABLE_ENTRY NTE);

VOID RouterRemoveRoute(
    PFIB_ENTRY FIBE);

PFIB_ENTRY RouterCreateRouteIPv4(
    IPv4_RAW_ADDRESS NetworkAddress,
    IPv4_RAW_ADDRESS Netmask,
    IPv4_RAW_ADDRESS RouterAddress,
    PNET_TABLE_ENTRY NTE,
    UINT Metric);

NTSTATUS RouterStartup(
    VOID);

NTSTATUS RouterShutdown(
    VOID);

#endif /* __ROUTER_H */

/* EOF */
