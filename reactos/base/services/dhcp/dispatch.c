/*	$OpenBSD: dispatch.c,v 1.31 2004/09/21 04:07:03 david Exp $	*/

/*
 * Copyright 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 1995, 1996, 1997, 1998, 1999
 * The Internet Software Consortium.   All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon <mellon@fugue.com> in cooperation with Vixie
 * Enterprises.  To learn more about the Internet Software Consortium,
 * see ``http://www.vix.com/isc''.  To learn more about Vixie
 * Enterprises, see ``http://www.vix.com''.
 */

#include "rosdhcp.h"
#include "dhcpd.h"
//#include <sys/ioctl.h>

//#include <net/if_media.h>
//#include <ifaddrs.h>
//#include <poll.h>

struct protocol *protocols = NULL;
struct timeout *timeouts = NULL;
static struct timeout *free_timeouts = NULL;
static int interfaces_invalidated = FALSE;
void (*bootp_packet_handler)(struct interface_info *,
                             struct dhcp_packet *, int, unsigned int,
                             struct iaddr, struct hardware *);

static int interface_status(struct interface_info *ifinfo);

/*
 * Use getifaddrs() to get a list of all the attached interfaces.  For
 * each interface that's of type INET and not the loopback interface,
 * register that interface with the network I/O software, figure out
 * what subnet it's on, and add it to the list of interfaces.
 */
void
discover_interfaces(struct interface_info *iface)
{
    PDHCP_ADAPTER Adapter = AdapterFindInfo( iface );

    if_register_receive(iface);
    if_register_send(iface);

    if( Adapter->DhclientState.state != S_STATIC ) {
        add_protocol(iface->name, iface->rfdesc, got_one, iface);
	iface->client->state = S_INIT;
	state_reboot(iface);
    }
}

void
reinitialize_interfaces(void)
{
    interfaces_invalidated = 1;
}

/*
 * Wait for packets to come in using poll().  When a packet comes in,
 * call receive_packet to receive the packet and possibly strip hardware
 * addressing information from it, and then call through the
 * bootp_packet_handler hook to try to do something with it.
 */
void
dispatch(void)
{
    int count, i, to_msec, nfds = 0;
    struct protocol *l;
    fd_set fds;
    time_t howlong;
    struct timeval timeval;

    ApiLock();

    for (l = protocols; l; l = l->next)
        nfds++;

    FD_ZERO(&fds);

    do {
        /*
         * Call any expired timeouts, and then if there's still
         * a timeout registered, time out the select call then.
         */
    another:
        if (timeouts) {
            struct timeout *t;

            if (timeouts->when <= cur_time) {
                t = timeouts;
                timeouts = timeouts->next;
                (*(t->func))(t->what);
                t->next = free_timeouts;
                free_timeouts = t;
                goto another;
            }

            /*
             * Figure timeout in milliseconds, and check for
             * potential overflow, so we can cram into an
             * int for poll, while not polling with a
             * negative timeout and blocking indefinitely.
             */
            howlong = timeouts->when - cur_time;
            if (howlong > INT_MAX / 1000)
                howlong = INT_MAX / 1000;
            to_msec = howlong * 1000;
        } else
            to_msec = -1;

        /* Set up the descriptors to be polled. */
        for (i = 0, l = protocols; l; l = l->next) {
            struct interface_info *ip = l->local;

            if (ip && (l->handler != got_one || !ip->dead)) {
                FD_SET(l->fd, &fds);
                i++;
            }
        }

        if (i == 0) {
            /* No interfaces for now, set the select timeout reasonably so
             * we can recover from that condition later. */
            timeval.tv_sec = 5;
            timeval.tv_usec = 0;
        } else {
            /* Wait for a packet or a timeout... XXX */
            timeval.tv_sec = to_msec / 1000;
            timeval.tv_usec = (to_msec % 1000) * 1000;
        }

        ApiUnlock();

        count = select(nfds, &fds, NULL, NULL, &timeval);

        DH_DbgPrint(MID_TRACE,("Select: %d\n", count));

        /* Review poll output */
        for (i = 0, l = protocols; l; l = l->next) {
            struct interface_info *ip = l->local;

            if (ip && (l->handler != got_one || !ip->dead)) {
                DH_DbgPrint
                    (MID_TRACE,
                     ("set(%d) -> %s\n",
                      l->fd, FD_ISSET(l->fd, &fds) ? "true" : "false"));
                i++;
            }
        }


        ApiLock();

        /* Not likely to be transitory... */
        if (count == SOCKET_ERROR) {
            if (errno == EAGAIN || errno == EINTR) {
                time(&cur_time);
                continue;
            } else {
                error("poll: %s", strerror(errno));
                break;
            }
        }

        /* Get the current time... */
        time(&cur_time);

        i = 0;
        for (l = protocols; l; l = l->next) {
            struct interface_info *ip;
            ip = l->local;
            if (FD_ISSET(l->fd, &fds)) {
                if (ip && (l->handler != got_one ||
                           !ip->dead)) {
                    DH_DbgPrint(MID_TRACE,("Handling %x\n", l));
                    (*(l->handler))(l);
                    if (interfaces_invalidated)
                        break;
                }
                i++;
            }
            interfaces_invalidated = 0;
        }
    } while (1);

    ApiUnlock(); /* Not reached currently */
}

void
got_one(struct protocol *l)
{
    struct sockaddr_in from;
    struct hardware hfrom;
    struct iaddr ifrom;
    ssize_t result;
    union {
        /*
         * Packet input buffer.  Must be as large as largest
         * possible MTU.
         */
        unsigned char packbuf[4095];
        struct dhcp_packet packet;
    } u;
    struct interface_info *ip = l->local;

    if ((result = receive_packet(ip, u.packbuf, sizeof(u), &from,
                                 &hfrom)) == -1) {
        warning("receive_packet failed on %s: %s", ip->name,
                strerror(errno));
        ip->errors++;
        if ((!interface_status(ip)) ||
            (ip->noifmedia && ip->errors > 20)) {
            /* our interface has gone away. */
            warning("Interface %s no longer appears valid.",
                    ip->name);
            ip->dead = 1;
            interfaces_invalidated = 1;
            close(l->fd);
            remove_protocol(l);
            free(ip);
        }
        return;
    }
    if (result == 0)
        return;

    if (bootp_packet_handler) {
        ifrom.len = 4;
        memcpy(ifrom.iabuf, &from.sin_addr, ifrom.len);

        (*bootp_packet_handler)(ip, &u.packet, result,
                                from.sin_port, ifrom, &hfrom);
    }
}

#if 0
int
interface_status(struct interface_info *ifinfo)
{
    char *ifname = ifinfo->name;
    int ifsock = ifinfo->rfdesc;
    struct ifreq ifr;
    struct ifmediareq ifmr;

    /* get interface flags */
    memset(&ifr, 0, sizeof(ifr));
    strlcpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));
    if (ioctl(ifsock, SIOCGIFFLAGS, &ifr) < 0) {
        syslog(LOG_ERR, "ioctl(SIOCGIFFLAGS) on %s: %m", ifname);
        goto inactive;
    }

    /*
     * if one of UP and RUNNING flags is dropped,
     * the interface is not active.
     */
    if ((ifr.ifr_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
        goto inactive;

    /* Next, check carrier on the interface, if possible */
    if (ifinfo->noifmedia)
        goto active;
    memset(&ifmr, 0, sizeof(ifmr));
    strlcpy(ifmr.ifm_name, ifname, sizeof(ifmr.ifm_name));
    if (ioctl(ifsock, SIOCGIFMEDIA, (caddr_t)&ifmr) < 0) {
        if (errno != EINVAL) {
            syslog(LOG_DEBUG, "ioctl(SIOCGIFMEDIA) on %s: %m",
                   ifname);

            ifinfo->noifmedia = 1;
            goto active;
        }
        /*
         * EINVAL (or ENOTTY) simply means that the interface
         * does not support the SIOCGIFMEDIA ioctl. We regard it alive.
         */
        ifinfo->noifmedia = 1;
        goto active;
    }
    if (ifmr.ifm_status & IFM_AVALID) {
        switch (ifmr.ifm_active & IFM_NMASK) {
        case IFM_ETHER:
            if (ifmr.ifm_status & IFM_ACTIVE)
                goto active;
            else
                goto inactive;
            break;
        default:
            goto inactive;
        }
    }
inactive:
    return (0);
active:
    return (1);
}
#else
int
interface_status(struct interface_info *ifinfo)
{
    return (1);
}
#endif

void
add_timeout(time_t when, void (*where)(void *), void *what)
{
    struct timeout *t, *q;

    DH_DbgPrint(MID_TRACE,("Adding timeout %x %p %x\n", when, where, what));
    /* See if this timeout supersedes an existing timeout. */
    t = NULL;
    for (q = timeouts; q; q = q->next) {
        if (q->func == where && q->what == what) {
            if (t)
                t->next = q->next;
            else
                timeouts = q->next;
            break;
        }
        t = q;
    }

    /* If we didn't supersede a timeout, allocate a timeout
       structure now. */
    if (!q) {
        if (free_timeouts) {
            q = free_timeouts;
            free_timeouts = q->next;
            q->func = where;
            q->what = what;
        } else {
            q = malloc(sizeof(struct timeout));
            if (!q)
                error("Can't allocate timeout structure!");
            q->func = where;
            q->what = what;
        }
    }

    q->when = when;

    /* Now sort this timeout into the timeout list. */

    /* Beginning of list? */
    if (!timeouts || timeouts->when > q->when) {
        q->next = timeouts;
        timeouts = q;
        return;
    }

    /* Middle of list? */
    for (t = timeouts; t->next; t = t->next) {
        if (t->next->when > q->when) {
            q->next = t->next;
            t->next = q;
            return;
        }
    }

    /* End of list. */
    t->next = q;
    q->next = NULL;
}

void
cancel_timeout(void (*where)(void *), void *what)
{
    struct timeout *t, *q;

    /* Look for this timeout on the list, and unlink it if we find it. */
    t = NULL;
    for (q = timeouts; q; q = q->next) {
        if (q->func == where && q->what == what) {
            if (t)
                t->next = q->next;
            else
                timeouts = q->next;
            break;
        }
        t = q;
    }

    /* If we found the timeout, put it on the free list. */
    if (q) {
        q->next = free_timeouts;
        free_timeouts = q;
    }
}

/* Add a protocol to the list of protocols... */
void
add_protocol(char *name, int fd, void (*handler)(struct protocol *),
             void *local)
{
    struct protocol *p;

    p = malloc(sizeof(*p));
    if (!p)
        error("can't allocate protocol struct for %s", name);

    p->fd = fd;
    p->handler = handler;
    p->local = local;
    p->next = protocols;
    protocols = p;
}

void
remove_protocol(struct protocol *proto)
{
    struct protocol *p, *next, *prev;

    prev = NULL;
    for (p = protocols; p; p = next) {
        next = p->next;
        if (p == proto) {
            if (prev)
                prev->next = p->next;
            else
                protocols = p->next;
            free(p);
        }
    }
}

struct protocol *
find_protocol_by_adapter(struct interface_info *info)
{
    struct protocol *p;

    for( p = protocols; p; p = p->next ) {
        if( p->local == (void *)info ) return p;
    }

    return NULL;
}

int
interface_link_status(char *ifname)
{
#if 0
    struct ifmediareq ifmr;
    int sock;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
        error("Can't create socket");

    memset(&ifmr, 0, sizeof(ifmr));
    strlcpy(ifmr.ifm_name, ifname, sizeof(ifmr.ifm_name));
    if (ioctl(sock, SIOCGIFMEDIA, (caddr_t)&ifmr) == -1) {
        /* EINVAL -> link state unknown. treat as active */
        if (errno != EINVAL)
            syslog(LOG_DEBUG, "ioctl(SIOCGIFMEDIA) on %s: %m",
                   ifname);
        close(sock);
        return (1);
    }
    close(sock);

    if (ifmr.ifm_status & IFM_AVALID) {
        if ((ifmr.ifm_active & IFM_NMASK) == IFM_ETHER) {
            if (ifmr.ifm_status & IFM_ACTIVE)
                return (1);
            else
                return (0);
        }
    }
#endif
    return (1);
}
