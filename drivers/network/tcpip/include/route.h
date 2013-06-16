/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS TCP/IP protocol driver
 * FILE:        include/route.h
 * PURPOSE:     Routing cache definitions
 */

#pragma once

#include <neighbor.h>
#include <address.h>
#include <router.h>
#include <pool.h>
#include <info.h>
#include <arp.h>

PNEIGHBOR_CACHE_ENTRY RouteGetRouteToDestination(PIP_ADDRESS Destination);

/* EOF */
