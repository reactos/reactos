/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/route.h
 * PURPOSE:     Routing cache definitions
 */
#ifndef __ROUTE_H
#define __ROUTE_H

#include <neighbor.h>
#include <address.h>
#include <router.h>
#include <pool.h>
#include <arp.h>


/* Route Cache Node structure.
 * The primary purpose of the RCN is to cache selected source and
 * next-hop addresses. The routing cache is implemented as a binary
 * search tree to provide fast lookups when many RCNs are in the cache.
 */
typedef struct ROUTE_CACHE_NODE {
    struct ROUTE_CACHE_NODE *Parent; /* Pointer to parent */
    struct ROUTE_CACHE_NODE *Left;   /* Pointer to left child */
    struct ROUTE_CACHE_NODE *Right;  /* Pointer to right child */
    /* Memebers above this line must not be moved */
    ULONG RefCount;                  /* Reference count */
    UCHAR State;                     /* RCN state (RCN_STATE_*) */
    IP_ADDRESS Destination;          /* Destination address */
    PNET_TABLE_ENTRY NTE;            /* Preferred NTE */
    PNEIGHBOR_CACHE_ENTRY NCE;       /* Pointer to NCE for first hop (NULL if none) */
    UINT PathMTU;                    /* Path MTU to destination */
} ROUTE_CACHE_NODE, *PROUTE_CACHE_NODE;

/* RCN states */
#define RCN_STATE_PERMANENT 0x00 /* RCN is permanent (properly local) */
#define RCN_STATE_COMPUTED  0x01 /* RCN is computed */


#define IsExternalRCN(RCN) \
    (RCN == ExternalRCN)

#define IsInternalRCN(RCN) \
    (RCN != ExternalRCN)


NTSTATUS RouteStartup(
    VOID);

NTSTATUS RouteShutdown(
    VOID);

UINT RouteGetRouteToDestination(
    PIP_ADDRESS Destination,
    PNET_TABLE_ENTRY NTE,
    PROUTE_CACHE_NODE *RCN);

PROUTE_CACHE_NODE RouteAddRouteToDestination(
    PIP_ADDRESS Destination,
    PNET_TABLE_ENTRY NTE,
    PIP_INTERFACE IF,
    PNEIGHBOR_CACHE_ENTRY NCE);

VOID RouteRemoveRouteToDestination(
    PROUTE_CACHE_NODE RCN);

VOID RouteInvalidateNTE(
    PNET_TABLE_ENTRY NTE);

VOID RouteInvalidateNCE(
    PNEIGHBOR_CACHE_ENTRY NCE);

#endif /* __ROUTE_H */

/* EOF */
