/* ipstats.h
 * Copyright (C) 2003 Juan Lang
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * This module implements functions shared by DLLs that need to get network-
 * related statistics.  It's meant to hide some platform-specificisms, and
 * share code that was previously duplicated.
 */
#ifndef WINE_IPSTATS_H_
#define WINE_IPSTATS_H_

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "iprtrmib.h"

/* Fills in entry's interface stats, using name to find them.
 * Returns ERROR_INVALID_PARAMETER if name or entry is NULL, NO_ERROR otherwise.
 */
DWORD getInterfaceStatsByName(const char *name, PMIB_IFROW entry);

/* Ditto above by index. */
DWORD getInterfaceStatsByIndex(DWORD index, PMIB_IFROW entry);

/* Gets ICMP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getICMPStats(MIB_ICMP *stats);

/* Gets IP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getIPStats(PMIB_IPSTATS stats, DWORD family);

/* Gets TCP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getTCPStats(MIB_TCPSTATS *stats, DWORD family);

/* Gets UDP statistics into stats.  Returns ERROR_INVALID_PARAMETER if stats is
 * NULL, NO_ERROR otherwise.
 */
DWORD getUDPStats(MIB_UDPSTATS *stats, DWORD family);

/* Route table functions */

DWORD getNumRoutes(void);

/* Minimalist route entry, only has the fields I can actually fill in.  How
 * these map to the different windows route data structures is up to you.
 */
typedef struct _RouteEntry {
  DWORD dest;
  DWORD mask;
  DWORD gateway;
  DWORD ifIndex;
  DWORD metric;
} RouteEntry;

typedef struct _RouteTable {
  DWORD numRoutes;
  RouteEntry routes[1];
} RouteTable;

/* Allocates and returns to you the route table, or NULL if it can't allocate
 * enough memory.  free() the returned table.
 */
RouteTable *getRouteTable(void);

/* Returns the number of entries in the arp table. */
DWORD getNumArpEntries(void);

/* Allocates and returns to you the arp table, or NULL if it can't allocate
 * enough memory.  free() the returned table.
 */
PMIB_IPNETTABLE getArpTable(void);

/* Returns the number of entries in the UDP state table. */
DWORD getNumUdpEntries(void);

/* Allocates and returns to you the UDP state table, or NULL if it can't
 * allocate enough memory.  free() the returned table.
 */
PMIB_UDPTABLE getUdpTable(void);

/* Returns the number of entries in the TCP state table. */
DWORD getNumTcpEntries(void);

/* Allocates and returns to you the TCP state table, or NULL if it can't
 * allocate enough memory.  free() the returned table.
 */
PMIB_TCPTABLE getTcpTable(void);

#endif /* ndef WINE_IPSTATS_H_ */
