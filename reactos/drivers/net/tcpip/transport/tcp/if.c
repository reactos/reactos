/*
 * Copyright (c) 1997-1998 University of Utah and the Flux Group.
 * All rights reserved.
 * 
 * This file is part of the Flux OSKit.  The OSKit is free software, also known
 * as "open source;" you can redistribute it and/or modify it under the terms
 * of the GNU General Public License (GPL), version 2, as published by the Free
 * Software Foundation (FSF).  To explore alternate licensing terms, contact
 * the University of Utah at csl-dist@cs.utah.edu or +1-801-585-3271.
 * 
 * The OSKit is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GPL for more details.  You should have
 * received a copy of the GPL along with the OSKit; see the file COPYING.  If
 * not, write to the FSF, 59 Temple Place #330, Boston, MA 02111-1307, USA.
 */

#include <roscfg.h>
#include <limits.h>
#include <tcpip.h>
#include <tcp.h>
#include <pool.h>
#include <route.h>
#include <address.h>
#include <datagram.h>
#include <checksum.h>
#include <routines.h>
#include <neighbor.h>
#include <oskittcp.h>

#if 0
#include <sys/param.h>
#include <sys/kernel.h>
#include <oskit/c/assert.h>
#include <net/if.h>
#endif

#include <oskittcp.h>

int if_index = 0;
struct ifaddr **ifnet_addrs;

int	ifqmaxlen = OSK_IFQ_MAXLEN;
struct	ifnet *ifnet;

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */
void
ifinit()
{
}

void
if_attach(ifp)
	struct ifnet *ifp;
{
    KeBugCheck( 0xface );
}

struct ifnet *
ifunit(char *name)
{
	return 0;
}

int ifa_iffind(addr, ifaddr, type)
	struct sockaddr *addr;
	struct ifaddr *ifaddr;
	int type;
{
    PNEIGHBOR_CACHE_ENTRY NCE;
    IP_ADDRESS Destination;
    NTSTATUS Status;
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;

    TI_DbgPrint(MID_TRACE,("called for type %d\n", type));

    if( !addr || !ifaddr ) {
	TI_DbgPrint(MID_TRACE,("no addr or no ifaddr (%x %x)\n", 
			       addr, ifaddr));
	return OSK_EINVAL;
    }

    Destination.Type = IP_ADDRESS_V4;
    Destination.Address.IPv4Address = addr_in->sin_addr.s_addr;

    NCE = RouterGetRoute(&Destination, NULL);

    if( !NCE || !NCE->Interface ) {
	TI_DbgPrint(MID_TRACE,("no neighbor cache or no interface (%x %x)\n",
			       NCE, NCE->Interface));
	return OSK_EADDRNOTAVAIL;
    }

    /* XXX - Point-to-point interfaces not supported yet */
    memset(&ifaddr->ifa_dstaddr, 0, sizeof( struct sockaddr ) );
    
    addr_in->sin_family = PF_INET;
    addr_in = (struct sockaddr_in *)&ifaddr->ifa_addr;
    Status = GetInterfaceIPv4Address( NCE->Interface,
				      type,
				      &addr_in->sin_addr.s_addr );

    if( !NT_SUCCESS(Status) )
	addr_in->sin_addr.s_addr = 0;
    
    ifaddr->ifa_flags = 0; /* XXX what goes here? */
    ifaddr->ifa_refcnt = 0; /* Anachronistic */
    ifaddr->ifa_metric = 1; /* We can get it like in ninfo.c, if we want */
    ifaddr->ifa_mtu = NCE->Interface->MTU;
    
    TI_DbgPrint(MID_TRACE,("status in iffind: %x\n", Status));

    return NT_SUCCESS(Status) ? 0 : OSK_EADDRNOTAVAIL;
}

/*
 * Find an interface on a specific network.  If many, choice
 * is most specific found.
 */
int ifa_ifwithnet(addr, ifaddr)
	struct sockaddr *addr;
	struct ifaddr *ifaddr;
{
    return ifa_iffind(addr, ifaddr, ADE_UNICAST);
}

/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
ifa_ifwithdstaddr(addr, ifaddr)
	register struct sockaddr *addr;
	register struct ifaddr *ifaddr;
{
    return ifa_iffind(addr, ifaddr, ADE_POINTOPOINT);
}

/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
int ifa_ifwithaddr(addr, ifaddr)
    struct sockaddr *addr;
    struct ifaddr *ifaddr;
{
    int error = ifa_ifwithnet( addr, ifaddr );
    struct sockaddr_in *addr_in = (struct sockaddr_in *)addr;
    struct sockaddr_in *faddr_in = (struct sockaddr_in *)ifaddr->ifa_addr;
    if( error != 0 ) return error;
    else return 
	     (faddr_in->sin_addr.s_addr == addr_in->sin_addr.s_addr) ?
	     0 : OSK_EADDRNOTAVAIL;
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
void
if_slowtimo(arg)
	void *arg;
{
#if 0
	register struct ifnet *ifp;
	int s = splimp();

	for (ifp = ifnet; ifp; ifp = ifp->if_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp->if_unit);
	}
	splx(s);
	timeout(if_slowtimo, (void *)0, hz / IFNET_SLOWHZ);
#endif
}
