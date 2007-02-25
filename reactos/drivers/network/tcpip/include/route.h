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
#include <info.h>
#include <arp.h>

PNEIGHBOR_CACHE_ENTRY RouteGetRouteToDestination(PIP_ADDRESS Destination);

#endif /* __ROUTE_H */

/* EOF */
