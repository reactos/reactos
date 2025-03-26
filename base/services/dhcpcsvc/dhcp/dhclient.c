/*	$OpenBSD: dhclient.c,v 1.62 2004/12/05 18:35:51 deraadt Exp $	*/

/*
 * Copyright 2004 Henning Brauer <henning@openbsd.org>
 * Copyright (c) 1995, 1996, 1997, 1998, 1999
 * The Internet Software Consortium.    All rights reserved.
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
 *
 * This client was substantially modified and enhanced by Elliot Poger
 * for use on Linux while he was working on the MosquitoNet project at
 * Stanford.
 *
 * The current version owes much to Elliot's Linux enhancements, but
 * was substantially reorganized and partially rewritten by Ted Lemon
 * so as to use the same networking framework that the Internet Software
 * Consortium DHCP server uses.   Much system-specific configuration code
 * was moved into a shell script so that as support for more operating
 * systems is added, it will not be necessary to port and maintain
 * system-specific configuration code to these operating systems - instead,
 * the shell script can invoke the native tools to accomplish the same
 * purpose.
 */

#include <rosdhcp.h>

#define	PERIOD 0x2e
#define	hyphenchar(c) ((c) == 0x2d)
#define	bslashchar(c) ((c) == 0x5c)
#define	periodchar(c) ((c) == PERIOD)
#define	asterchar(c) ((c) == 0x2a)
#define	alphachar(c) (((c) >= 0x41 && (c) <= 0x5a) || \
	    ((c) >= 0x61 && (c) <= 0x7a))
#define	digitchar(c) ((c) >= 0x30 && (c) <= 0x39)

#define	borderchar(c) (alphachar(c) || digitchar(c))
#define	middlechar(c) (borderchar(c) || hyphenchar(c))
#define	domainchar(c) ((c) > 0x20 && (c) < 0x7f)

unsigned long debug_trace_level = 0; /* DEBUG_ULTRA */

char *path_dhclient_conf = _PATH_DHCLIENT_CONF;
char *path_dhclient_db = NULL;

int log_perror = 1;
int privfd;
//int nullfd = -1;

struct iaddr iaddr_broadcast = { 4, { 255, 255, 255, 255 } };
struct in_addr inaddr_any;
struct sockaddr_in sockaddr_broadcast;

/*
 * ASSERT_STATE() does nothing now; it used to be
 * assert (state_is == state_shouldbe).
 */
#define ASSERT_STATE(state_is, state_shouldbe) {}

#define TIME_MAX 2147483647

int		log_priority;
int		no_daemon;
int		unknown_ok = 1;
int		routefd;

void		 usage(void);
int		 check_option(struct client_lease *l, int option);
int		 ipv4addrs(char * buf);
int		 res_hnok(const char *dn);
char		*option_as_string(unsigned int code, unsigned char *data, int len);
int		 fork_privchld(int, int);
int              check_arp( struct interface_info *ip, struct client_lease *lp );

#define	ADVANCE(x, n) (x += ROUNDUP((n)->sa_len))

time_t	scripttime;


int
init_client(void)
{
    ApiInit();
    AdapterInit();

    tzset();

    memset(&sockaddr_broadcast, 0, sizeof(sockaddr_broadcast));
    sockaddr_broadcast.sin_family = AF_INET;
    sockaddr_broadcast.sin_port = htons(REMOTE_PORT);
    sockaddr_broadcast.sin_addr.s_addr = INADDR_BROADCAST;
    inaddr_any.s_addr = INADDR_ANY;
    bootp_packet_handler = do_packet;

    return 1; // TRUE
}

void
stop_client(void)
{
    // AdapterStop();
    // ApiFree();
    /* FIXME: Close pipe and kill pipe thread */
}

/* XXX Implement me */
int check_arp( struct interface_info *ip, struct client_lease *lp ) {
    return 1;
}

/*
 * Individual States:
 *
 * Each routine is called from the dhclient_state_machine() in one of
 * these conditions:
 * -> entering INIT state
 * -> recvpacket_flag == 0: timeout in this state
 * -> otherwise: received a packet in this state
 *
 * Return conditions as handled by dhclient_state_machine():
 * Returns 1, sendpacket_flag = 1: send packet, reset timer.
 * Returns 1, sendpacket_flag = 0: just reset the timer (wait for a milestone).
 * Returns 0: finish the nap which was interrupted for no good reason.
 *
 * Several per-interface variables are used to keep track of the process:
 *   active_lease: the lease that is being used on the interface
 *                 (null pointer if not configured yet).
 *   offered_leases: leases corresponding to DHCPOFFER messages that have
 *                   been sent to us by DHCP servers.
 *   acked_leases: leases corresponding to DHCPACK messages that have been
 *                 sent to us by DHCP servers.
 *   sendpacket: DHCP packet we're trying to send.
 *   destination: IP address to send sendpacket to
 * In addition, there are several relevant per-lease variables.
 *   T1_expiry, T2_expiry, lease_expiry: lease milestones
 * In the active lease, these control the process of renewing the lease;
 * In leases on the acked_leases list, this simply determines when we
 * can no longer legitimately use the lease.
 */

void
state_reboot(void *ipp)
{
	struct interface_info *ip = ipp;
	ULONG foo = (ULONG) GetTickCount();

	/* If we don't remember an active lease, go straight to INIT. */
	if (!ip->client->active || ip->client->active->is_bootp) {
		state_init(ip);
		return;
	}

	/* We are in the rebooting state. */
	ip->client->state = S_REBOOTING;

	/* make_request doesn't initialize xid because it normally comes
	   from the DHCPDISCOVER, but we haven't sent a DHCPDISCOVER,
	   so pick an xid now. */
	ip->client->xid = RtlRandom(&foo);

	/* Make a DHCPREQUEST packet, and set appropriate per-interface
	   flags. */
	make_request(ip, ip->client->active);
	ip->client->destination = iaddr_broadcast;
	time(&ip->client->first_sending);
	ip->client->interval = ip->client->config->initial_interval;

	/* Zap the medium list... */
	ip->client->medium = NULL;

	/* Send out the first DHCPREQUEST packet. */
	send_request(ip);
}

/*
 * Called when a lease has completely expired and we've
 * been unable to renew it.
 */
void
state_init(void *ipp)
{
	struct interface_info *ip = ipp;

	ASSERT_STATE(state, S_INIT);

	/* Make a DHCPDISCOVER packet, and set appropriate per-interface
	   flags. */
	make_discover(ip, ip->client->active);
	ip->client->xid = ip->client->packet.xid;
	ip->client->destination = iaddr_broadcast;
	ip->client->state = S_SELECTING;
	time(&ip->client->first_sending);
	ip->client->interval = ip->client->config->initial_interval;

	/* Add an immediate timeout to cause the first DHCPDISCOVER packet
	   to go out. */
	send_discover(ip);
}

/*
 * state_selecting is called when one or more DHCPOFFER packets
 * have been received and a configurable period of time has passed.
 */
void
state_selecting(void *ipp)
{
	struct interface_info *ip = ipp;
	struct client_lease *lp, *next, *picked;
        time_t cur_time;

	ASSERT_STATE(state, S_SELECTING);

        time(&cur_time);

	/* Cancel state_selecting and send_discover timeouts, since either
	   one could have got us here. */
	cancel_timeout(state_selecting, ip);
	cancel_timeout(send_discover, ip);

	/* We have received one or more DHCPOFFER packets.   Currently,
	   the only criterion by which we judge leases is whether or
	   not we get a response when we arp for them. */
	picked = NULL;
	for (lp = ip->client->offered_leases; lp; lp = next) {
		next = lp->next;

		/* Check to see if we got an ARPREPLY for the address
		   in this particular lease. */
		if (!picked) {
                    if( !check_arp(ip,lp) ) goto freeit;
                    picked = lp;
                    picked->next = NULL;
		} else {
freeit:
			free_client_lease(lp);
		}
	}
	ip->client->offered_leases = NULL;

	/* If we just tossed all the leases we were offered, go back
	   to square one. */
	if (!picked) {
		ip->client->state = S_INIT;
		state_init(ip);
		return;
	}

	/* If it was a BOOTREPLY, we can just take the address right now. */
	if (!picked->options[DHO_DHCP_MESSAGE_TYPE].len) {
		ip->client->new = picked;

		/* Make up some lease expiry times
		   XXX these should be configurable. */
		ip->client->new->expiry = cur_time + 12000;
		ip->client->new->renewal += cur_time + 8000;
		ip->client->new->rebind += cur_time + 10000;

		ip->client->state = S_REQUESTING;

		/* Bind to the address we received. */
		bind_lease(ip);
		return;
	}

	/* Go to the REQUESTING state. */
	ip->client->destination = iaddr_broadcast;
	ip->client->state = S_REQUESTING;
	ip->client->first_sending = cur_time;
	ip->client->interval = ip->client->config->initial_interval;

	/* Make a DHCPREQUEST packet from the lease we picked. */
	make_request(ip, picked);
	ip->client->xid = ip->client->packet.xid;

	/* Toss the lease we picked - we'll get it back in a DHCPACK. */
	free_client_lease(picked);

	/* Add an immediate timeout to send the first DHCPREQUEST packet. */
	send_request(ip);
}

/* state_requesting is called when we receive a DHCPACK message after
   having sent out one or more DHCPREQUEST packets. */

void
dhcpack(struct packet *packet)
{
	struct interface_info *ip = packet->interface;
	struct client_lease *lease;
        time_t cur_time;

        time(&cur_time);

	/* If we're not receptive to an offer right now, or if the offer
	   has an unrecognizable transaction id, then just drop it. */
	if (packet->interface->client->xid != packet->raw->xid ||
	    (packet->interface->hw_address.hlen != packet->raw->hlen) ||
	    (memcmp(packet->interface->hw_address.haddr,
	    packet->raw->chaddr, packet->raw->hlen)))
		return;

	if (ip->client->state != S_REBOOTING &&
	    ip->client->state != S_REQUESTING &&
	    ip->client->state != S_RENEWING &&
	    ip->client->state != S_REBINDING)
		return;

	note("DHCPACK from %s", piaddr(packet->client_addr));

	lease = packet_to_lease(packet);
	if (!lease) {
		note("packet_to_lease failed.");
		return;
	}

	ip->client->new = lease;

	/* Stop resending DHCPREQUEST. */
	cancel_timeout(send_request, ip);

	/* Figure out the lease time. */
	if (ip->client->new->options[DHO_DHCP_LEASE_TIME].data)
		ip->client->new->expiry = getULong(
		    ip->client->new->options[DHO_DHCP_LEASE_TIME].data);
	else
		ip->client->new->expiry = DHCP_DEFAULT_LEASE_TIME;
	/* A number that looks negative here is really just very large,
	   because the lease expiry offset is unsigned. */
	if (ip->client->new->expiry < 0)
		ip->client->new->expiry = TIME_MAX;
	/* XXX should be fixed by resetting the client state */
	if (ip->client->new->expiry < 60)
		ip->client->new->expiry = 60;

	/* Take the server-provided renewal time if there is one;
	   otherwise figure it out according to the spec. */
	if (ip->client->new->options[DHO_DHCP_RENEWAL_TIME].len)
		ip->client->new->renewal = getULong(
		    ip->client->new->options[DHO_DHCP_RENEWAL_TIME].data);
	else
		ip->client->new->renewal = ip->client->new->expiry / 2;

	/* Same deal with the rebind time. */
	if (ip->client->new->options[DHO_DHCP_REBINDING_TIME].len)
		ip->client->new->rebind = getULong(
		    ip->client->new->options[DHO_DHCP_REBINDING_TIME].data);
	else
		ip->client->new->rebind = ip->client->new->renewal +
		    ip->client->new->renewal / 2 + ip->client->new->renewal / 4;

#ifdef __REACTOS__
	ip->client->new->obtained = cur_time;
#endif
	ip->client->new->expiry += cur_time;
	/* Lease lengths can never be negative. */
	if (ip->client->new->expiry < cur_time)
		ip->client->new->expiry = TIME_MAX;
	ip->client->new->renewal += cur_time;
	if (ip->client->new->renewal < cur_time)
		ip->client->new->renewal = TIME_MAX;
	ip->client->new->rebind += cur_time;
	if (ip->client->new->rebind < cur_time)
		ip->client->new->rebind = TIME_MAX;

	bind_lease(ip);
}

void set_name_servers( PDHCP_ADAPTER Adapter, struct client_lease *new_lease ) {
    CHAR Buffer[200] = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
    HKEY RegKey;

    strcat(Buffer, Adapter->DhclientInfo.name);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Buffer, 0, KEY_WRITE, &RegKey ) != ERROR_SUCCESS)
        return;


    if( new_lease->options[DHO_DOMAIN_NAME_SERVERS].len ) {

        struct iaddr nameserver;
        char *nsbuf;
        int i, addrs =
            new_lease->options[DHO_DOMAIN_NAME_SERVERS].len / sizeof(ULONG);

        nsbuf = malloc( addrs * sizeof(IP_ADDRESS_STRING) );

        if( nsbuf) {
            nsbuf[0] = 0;
            for( i = 0; i < addrs; i++ ) {
                nameserver.len = sizeof(ULONG);
                memcpy( nameserver.iabuf,
                        new_lease->options[DHO_DOMAIN_NAME_SERVERS].data +
                        (i * sizeof(ULONG)), sizeof(ULONG) );
                strcat( nsbuf, piaddr(nameserver) );
                if( i != addrs-1 ) strcat( nsbuf, "," );
            }

            DH_DbgPrint(MID_TRACE,("Setting DhcpNameserver: %s\n", nsbuf));

            RegSetValueExA( RegKey, "DhcpNameServer", 0, REG_SZ,
                           (LPBYTE)nsbuf, strlen(nsbuf) + 1 );
            free( nsbuf );
        }

    } else {
            RegDeleteValueW( RegKey, L"DhcpNameServer" );
    }

    RegCloseKey( RegKey );

}

void
set_domain(PDHCP_ADAPTER Adapter,
           struct client_lease *new_lease)
{
    CHAR Buffer1[MAX_PATH] = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
    CHAR Buffer2[MAX_PATH] = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters";
    HKEY RegKey1, RegKey2;

    strcat(Buffer1, Adapter->DhclientInfo.name);

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Buffer1, 0, KEY_WRITE, &RegKey1 ) != ERROR_SUCCESS)
    {
        return;
    }

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Buffer2, 0, KEY_WRITE, &RegKey2 ) != ERROR_SUCCESS)
    {
        RegCloseKey(RegKey1);
        return;
    }

    if (new_lease->options[DHO_DOMAIN_NAME].len)
    {
        DH_DbgPrint(MID_TRACE, ("Setting DhcpDomain: %s\n", new_lease->options[DHO_DOMAIN_NAME].data));

        RegSetValueExA(RegKey1,
                       "DhcpDomain",
                       0,
                       REG_SZ,
                       (LPBYTE)new_lease->options[DHO_DOMAIN_NAME].data,
                       new_lease->options[DHO_DOMAIN_NAME].len);

        RegSetValueExA(RegKey2,
                       "DhcpDomain",
                       0,
                       REG_SZ,
                       (LPBYTE)new_lease->options[DHO_DOMAIN_NAME].data,
                       new_lease->options[DHO_DOMAIN_NAME].len);
    }
    else
    {
        RegDeleteValueW(RegKey1, L"DhcpDomain");
        RegDeleteValueW(RegKey2, L"DhcpDomain");
    }

    RegCloseKey(RegKey1);
    RegCloseKey(RegKey2);

}

void setup_adapter( PDHCP_ADAPTER Adapter, struct client_lease *new_lease ) {
    CHAR Buffer[200] = "SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters\\Interfaces\\";
    struct iaddr netmask;
    HKEY hkey;
    int i;
    DWORD dwEnableDHCP;

    strcat(Buffer, Adapter->DhclientInfo.name);
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, Buffer, 0, KEY_WRITE, &hkey) != ERROR_SUCCESS)
        hkey = NULL;


    if( Adapter->NteContext )
    {
        DeleteIPAddress( Adapter->NteContext );
        Adapter->NteContext = 0;
    }

    /* Set up our default router if we got one from the DHCP server */
    if( new_lease->options[DHO_SUBNET_MASK].len ) {
        NTSTATUS Status;

        memcpy( netmask.iabuf,
                new_lease->options[DHO_SUBNET_MASK].data,
                new_lease->options[DHO_SUBNET_MASK].len );
        Status = AddIPAddress
            ( *((ULONG*)new_lease->address.iabuf),
              *((ULONG*)netmask.iabuf),
              Adapter->IfMib.dwIndex,
              &Adapter->NteContext,
              &Adapter->NteInstance );
        if (hkey) {
            RegSetValueExA(hkey, "DhcpIPAddress", 0, REG_SZ, (LPBYTE)piaddr(new_lease->address), strlen(piaddr(new_lease->address))+1);
            Buffer[0] = '\0';
            for(i = 0; i < new_lease->options[DHO_SUBNET_MASK].len; i++)
            {
                sprintf(&Buffer[strlen(Buffer)], "%u", new_lease->options[DHO_SUBNET_MASK].data[i]);
                if (i + 1 < new_lease->options[DHO_SUBNET_MASK].len)
                    strcat(Buffer, ".");
            }
            RegSetValueExA(hkey, "DhcpSubnetMask", 0, REG_SZ, (LPBYTE)Buffer, strlen(Buffer)+1);
            dwEnableDHCP = 1;
            RegSetValueExA(hkey, "EnableDHCP", 0, REG_DWORD, (LPBYTE)&dwEnableDHCP, sizeof(DWORD));
        }

        if( !NT_SUCCESS(Status) )
            warning("AddIPAddress: %lx\n", Status);
    }

    if( new_lease->options[DHO_ROUTERS].len ) {
        NTSTATUS Status;

        Adapter->RouterMib.dwForwardDest = 0; /* Default route */
        Adapter->RouterMib.dwForwardMask = 0;
        Adapter->RouterMib.dwForwardMetric1 = 1;
        Adapter->RouterMib.dwForwardIfIndex = Adapter->IfMib.dwIndex;

        if( Adapter->RouterMib.dwForwardNextHop ) {
            /* If we set a default route before, delete it before continuing */
            DeleteIpForwardEntry( &Adapter->RouterMib );
        }

        Adapter->RouterMib.dwForwardNextHop =
            *((ULONG*)new_lease->options[DHO_ROUTERS].data);

        Status = CreateIpForwardEntry( &Adapter->RouterMib );

        if( !NT_SUCCESS(Status) )
            warning("CreateIpForwardEntry: %lx\n", Status);

        if (hkey) {
            Buffer[0] = '\0';
            for(i = 0; i < new_lease->options[DHO_ROUTERS].len; i++)
            {
                sprintf(&Buffer[strlen(Buffer)], "%u", new_lease->options[DHO_ROUTERS].data[i]);
                if (i + 1 < new_lease->options[DHO_ROUTERS].len)
                    strcat(Buffer, ".");
            }
            RegSetValueExA(hkey, "DhcpDefaultGateway", 0, REG_SZ, (LPBYTE)Buffer, strlen(Buffer)+1);
        }
    }

    if (hkey)
        RegCloseKey(hkey);
}


void
bind_lease(struct interface_info *ip)
{
    PDHCP_ADAPTER Adapter;
    struct client_lease *new_lease = ip->client->new;
    time_t cur_time;

    time(&cur_time);

    /* Remember the medium. */
    ip->client->new->medium = ip->client->medium;

    /* Replace the old active lease with the new one. */
    if (ip->client->active)
        free_client_lease(ip->client->active);
    ip->client->active = ip->client->new;
    ip->client->new = NULL;

    /* Set up a timeout to start the renewal process. */
    /* Timeout of zero means no timeout (some implementations seem to use
     * one day).
     */
    if( ip->client->active->renewal - cur_time )
        add_timeout(ip->client->active->renewal, state_bound, ip);

    note("bound to %s -- renewal in %ld seconds.",
         piaddr(ip->client->active->address),
         (long int)(ip->client->active->renewal - cur_time));

    ip->client->state = S_BOUND;

    Adapter = AdapterFindInfo( ip );

    if( Adapter )  setup_adapter( Adapter, new_lease );
    else {
        warning("Could not find adapter for info %p\n", ip);
        return;
    }
    set_name_servers( Adapter, new_lease );
    set_domain( Adapter, new_lease );
}

/*
 * state_bound is called when we've successfully bound to a particular
 * lease, but the renewal time on that lease has expired.   We are
 * expected to unicast a DHCPREQUEST to the server that gave us our
 * original lease.
 */
void
state_bound(void *ipp)
{
	struct interface_info *ip = ipp;

	ASSERT_STATE(state, S_BOUND);

	/* T1 has expired. */
	make_request(ip, ip->client->active);
	ip->client->xid = ip->client->packet.xid;

	if (ip->client->active->options[DHO_DHCP_SERVER_IDENTIFIER].len == 4) {
		memcpy(ip->client->destination.iabuf, ip->client->active->
		    options[DHO_DHCP_SERVER_IDENTIFIER].data, 4);
		ip->client->destination.len = 4;
	} else
		ip->client->destination = iaddr_broadcast;

	time(&ip->client->first_sending);
	ip->client->interval = ip->client->config->initial_interval;
	ip->client->state = S_RENEWING;

	/* Send the first packet immediately. */
	send_request(ip);
}

void
bootp(struct packet *packet)
{
	struct iaddrlist *ap;

	if (packet->raw->op != BOOTREPLY)
		return;

	/* If there's a reject list, make sure this packet's sender isn't
	   on it. */
	for (ap = packet->interface->client->config->reject_list;
	    ap; ap = ap->next) {
		if (addr_eq(packet->client_addr, ap->addr)) {
			note("BOOTREPLY from %s rejected.", piaddr(ap->addr));
			return;
		}
	}
	dhcpoffer(packet);
}

void
dhcp(struct packet *packet)
{
	struct iaddrlist *ap;
	void (*handler)(struct packet *);
	char *type;

	switch (packet->packet_type) {
	case DHCPOFFER:
		handler = dhcpoffer;
		type = "DHCPOFFER";
		break;
	case DHCPNAK:
		handler = dhcpnak;
		type = "DHCPNACK";
		break;
	case DHCPACK:
		handler = dhcpack;
		type = "DHCPACK";
		break;
	default:
		return;
	}

	/* If there's a reject list, make sure this packet's sender isn't
	   on it. */
	for (ap = packet->interface->client->config->reject_list;
	    ap; ap = ap->next) {
		if (addr_eq(packet->client_addr, ap->addr)) {
			note("%s from %s rejected.", type, piaddr(ap->addr));
			return;
		}
	}
	(*handler)(packet);
}

void
dhcpoffer(struct packet *packet)
{
	struct interface_info *ip = packet->interface;
	struct client_lease *lease, *lp;
	int i;
	int arp_timeout_needed = 0, stop_selecting;
	char *name = packet->options[DHO_DHCP_MESSAGE_TYPE].len ?
	    "DHCPOFFER" : "BOOTREPLY";
        time_t cur_time;

        time(&cur_time);

	/* If we're not receptive to an offer right now, or if the offer
	   has an unrecognizable transaction id, then just drop it. */
	if (ip->client->state != S_SELECTING ||
            packet->interface->client->xid != packet->raw->xid ||
            (packet->interface->hw_address.hlen != packet->raw->hlen) ||
	    (memcmp(packet->interface->hw_address.haddr,
	    packet->raw->chaddr, packet->raw->hlen)))
		return;

	note("%s from %s", name, piaddr(packet->client_addr));


	/* If this lease doesn't supply the minimum required parameters,
	   blow it off. */
	for (i = 0; ip->client->config->required_options[i]; i++) {
		if (!packet->options[ip->client->config->
		    required_options[i]].len) {
			note("%s isn't satisfactory.", name);
			return;
		}
	}

	/* If we've already seen this lease, don't record it again. */
	for (lease = ip->client->offered_leases;
	    lease; lease = lease->next) {
		if (lease->address.len == sizeof(packet->raw->yiaddr) &&
		    !memcmp(lease->address.iabuf,
		    &packet->raw->yiaddr, lease->address.len)) {
			debug("%s already seen.", name);
			return;
		}
	}

	lease = packet_to_lease(packet);
	if (!lease) {
		note("packet_to_lease failed.");
		return;
	}

	/* If this lease was acquired through a BOOTREPLY, record that
	   fact. */
	if (!packet->options[DHO_DHCP_MESSAGE_TYPE].len)
		lease->is_bootp = 1;

	/* Record the medium under which this lease was offered. */
	lease->medium = ip->client->medium;

	/* Send out an ARP Request for the offered IP address. */
        if( !check_arp( ip, lease ) ) {
            note("Arp check failed\n");
            return;
        }

	/* Figure out when we're supposed to stop selecting. */
	stop_selecting =
	    ip->client->first_sending + ip->client->config->select_interval;

	/* If this is the lease we asked for, put it at the head of the
	   list, and don't mess with the arp request timeout. */
	if (lease->address.len == ip->client->requested_address.len &&
	    !memcmp(lease->address.iabuf,
	    ip->client->requested_address.iabuf,
	    ip->client->requested_address.len)) {
		lease->next = ip->client->offered_leases;
		ip->client->offered_leases = lease;
	} else {
		/* If we already have an offer, and arping for this
		   offer would take us past the selection timeout,
		   then don't extend the timeout - just hope for the
		   best. */
		if (ip->client->offered_leases &&
		    (cur_time + arp_timeout_needed) > stop_selecting)
			arp_timeout_needed = 0;

		/* Put the lease at the end of the list. */
		lease->next = NULL;
		if (!ip->client->offered_leases)
			ip->client->offered_leases = lease;
		else {
			for (lp = ip->client->offered_leases; lp->next;
			    lp = lp->next)
				;	/* nothing */
			lp->next = lease;
		}
	}

	/* If we're supposed to stop selecting before we've had time
	   to wait for the ARPREPLY, add some delay to wait for
	   the ARPREPLY. */
	if (stop_selecting - cur_time < arp_timeout_needed)
		stop_selecting = cur_time + arp_timeout_needed;

	/* If the selecting interval has expired, go immediately to
	   state_selecting().  Otherwise, time out into
	   state_selecting at the select interval. */
	if (stop_selecting <= 0)
		state_selecting(ip);
	else {
		add_timeout(stop_selecting, state_selecting, ip);
		cancel_timeout(send_discover, ip);
	}
}

/* Allocate a client_lease structure and initialize it from the parameters
   in the specified packet. */

struct client_lease *
packet_to_lease(struct packet *packet)
{
	struct client_lease *lease;
	int i;

	lease = malloc(sizeof(struct client_lease));

	if (!lease) {
		warning("dhcpoffer: no memory to record lease.");
		return (NULL);
	}

	memset(lease, 0, sizeof(*lease));

	/* Copy the lease options. */
	for (i = 0; i < 256; i++) {
		if (packet->options[i].len) {
			lease->options[i].data =
			    malloc(packet->options[i].len + 1);
			if (!lease->options[i].data) {
				warning("dhcpoffer: no memory for option %d", i);
				free_client_lease(lease);
				return (NULL);
			} else {
				memcpy(lease->options[i].data,
				    packet->options[i].data,
				    packet->options[i].len);
				lease->options[i].len =
				    packet->options[i].len;
				lease->options[i].data[lease->options[i].len] =
				    0;
			}
			if (!check_option(lease,i)) {
				/* ignore a bogus lease offer */
				warning("Invalid lease option - ignoring offer");
				free_client_lease(lease);
				return (NULL);
			}
		}
	}

	lease->address.len = sizeof(packet->raw->yiaddr);
	memcpy(lease->address.iabuf, &packet->raw->yiaddr, lease->address.len);
#ifdef __REACTOS__
	lease->serveraddress.len = sizeof(packet->raw->siaddr);
	memcpy(lease->serveraddress.iabuf, &packet->raw->siaddr, lease->address.len);
#endif

	/* If the server name was filled out, copy it. */
	if ((!packet->options[DHO_DHCP_OPTION_OVERLOAD].len ||
	    !(packet->options[DHO_DHCP_OPTION_OVERLOAD].data[0] & 2)) &&
	    packet->raw->sname[0]) {
		lease->server_name = malloc(DHCP_SNAME_LEN + 1);
		if (!lease->server_name) {
			warning("dhcpoffer: no memory for server name.");
			free_client_lease(lease);
			return (NULL);
		}
		memcpy(lease->server_name, packet->raw->sname, DHCP_SNAME_LEN);
		lease->server_name[DHCP_SNAME_LEN]='\0';
		if (!res_hnok(lease->server_name) ) {
			warning("Bogus server name %s",  lease->server_name );
			free_client_lease(lease);
			return (NULL);
		}

	}

	/* Ditto for the filename. */
	if ((!packet->options[DHO_DHCP_OPTION_OVERLOAD].len ||
	    !(packet->options[DHO_DHCP_OPTION_OVERLOAD].data[0] & 1)) &&
	    packet->raw->file[0]) {
		/* Don't count on the NUL terminator. */
		lease->filename = malloc(DHCP_FILE_LEN + 1);
		if (!lease->filename) {
			warning("dhcpoffer: no memory for filename.");
			free_client_lease(lease);
			return (NULL);
		}
		memcpy(lease->filename, packet->raw->file, DHCP_FILE_LEN);
		lease->filename[DHCP_FILE_LEN]='\0';
	}
	return lease;
}

void
dhcpnak(struct packet *packet)
{
	struct interface_info *ip = packet->interface;

	/* If we're not receptive to an offer right now, or if the offer
	   has an unrecognizable transaction id, then just drop it. */
	if (packet->interface->client->xid != packet->raw->xid ||
	    (packet->interface->hw_address.hlen != packet->raw->hlen) ||
	    (memcmp(packet->interface->hw_address.haddr,
	    packet->raw->chaddr, packet->raw->hlen)))
		return;

	if (ip->client->state != S_REBOOTING &&
	    ip->client->state != S_REQUESTING &&
	    ip->client->state != S_RENEWING &&
	    ip->client->state != S_REBINDING)
		return;

	note("DHCPNAK from %s", piaddr(packet->client_addr));

	if (!ip->client->active) {
		note("DHCPNAK with no active lease.\n");
		return;
	}

	free_client_lease(ip->client->active);
	ip->client->active = NULL;

	/* Stop sending DHCPREQUEST packets... */
	cancel_timeout(send_request, ip);

	ip->client->state = S_INIT;
	state_init(ip);
}

/* Send out a DHCPDISCOVER packet, and set a timeout to send out another
   one after the right interval has expired.  If we don't get an offer by
   the time we reach the panic interval, call the panic function. */

void
send_discover(void *ipp)
{
	struct interface_info *ip = ipp;
	int interval, increase = 1;
        time_t cur_time;

        DH_DbgPrint(MID_TRACE,("Doing discover on interface %p\n",ip));

        time(&cur_time);

	/* Figure out how long it's been since we started transmitting. */
	interval = cur_time - ip->client->first_sending;

	/* If we're past the panic timeout, call the script and tell it
	   we haven't found anything for this interface yet. */
	if (interval > ip->client->config->timeout) {
		state_panic(ip);
        ip->client->first_sending = cur_time;
	}

	/* If we're selecting media, try the whole list before doing
	   the exponential backoff, but if we've already received an
	   offer, stop looping, because we obviously have it right. */
	if (!ip->client->offered_leases &&
	    ip->client->config->media) {
		int fail = 0;

		if (ip->client->medium) {
			ip->client->medium = ip->client->medium->next;
			increase = 0;
		}
		if (!ip->client->medium) {
			if (fail)
				error("No valid media types for %s!", ip->name);
			ip->client->medium = ip->client->config->media;
			increase = 1;
		}

		note("Trying medium \"%s\" %d", ip->client->medium->string,
		    increase);
                /* XXX Support other media types eventually */
	}

	/*
	 * If we're supposed to increase the interval, do so.  If it's
	 * currently zero (i.e., we haven't sent any packets yet), set
	 * it to one; otherwise, add to it a random number between zero
	 * and two times itself.  On average, this means that it will
	 * double with every transmission.
	 */
	if (increase) {
		if (!ip->client->interval)
			ip->client->interval =
			    ip->client->config->initial_interval;
		else {
			ip->client->interval += (rand() >> 2) %
			    (2 * ip->client->interval);
		}

		/* Don't backoff past cutoff. */
		if (ip->client->interval >
		    ip->client->config->backoff_cutoff)
			ip->client->interval =
				((ip->client->config->backoff_cutoff / 2)
				 + ((rand() >> 2) %
				    ip->client->config->backoff_cutoff));
	} else if (!ip->client->interval)
		ip->client->interval =
			ip->client->config->initial_interval;

	/* If the backoff would take us to the panic timeout, just use that
	   as the interval. */
	if (cur_time + ip->client->interval >
	    ip->client->first_sending + ip->client->config->timeout)
		ip->client->interval =
			(ip->client->first_sending +
			 ip->client->config->timeout) - cur_time + 1;

	/* Record the number of seconds since we started sending. */
	if (interval < 65536)
		ip->client->packet.secs = htons(interval);
	else
		ip->client->packet.secs = htons(65535);
	ip->client->secs = ip->client->packet.secs;

	note("DHCPDISCOVER on %s to %s port %d interval %ld",
	    ip->name, inet_ntoa(sockaddr_broadcast.sin_addr),
	    ntohs(sockaddr_broadcast.sin_port), (long int)ip->client->interval);

	/* Send out a packet. */
	(void)send_packet(ip, &ip->client->packet, ip->client->packet_length,
	    inaddr_any, &sockaddr_broadcast, NULL);

        DH_DbgPrint(MID_TRACE,("discover timeout: now %x -> then %x\n",
                               cur_time, cur_time + ip->client->interval));

	add_timeout(cur_time + ip->client->interval, send_discover, ip);
}

/*
 * state_panic gets called if we haven't received any offers in a preset
 * amount of time.   When this happens, we try to use existing leases
 * that haven't yet expired, and failing that, we call the client script
 * and hope it can do something.
 */
void
state_panic(void *ipp)
{
	struct interface_info *ip = ipp;
	uint16_t address_low;
	int i;
    IPAddr IpAddress;
    ULONG Buffer[20];
    ULONG BufferSize;
    DWORD ret;
    PDHCP_ADAPTER Adapter = AdapterFindInfo(ip);

	note("No DHCPOFFERS received.");

    if (!Adapter->NteContext)
    {
        /* Generate an automatic private address */
        DbgPrint("DHCPCSVC: Failed to receive a response from a DHCP server. An automatic private address will be assigned.\n");

        /* FIXME: The address generation code sucks */
        srand(0);

        for (;;)
        {
            address_low = rand();
            for (i = 0; i < ip->hw_address.hlen; i++)
                address_low += ip->hw_address.haddr[i];

            IpAddress = htonl(0xA9FE0000 | address_low);  // 169.254.X.X

            /* Send an ARP request to check if the IP address is already in use */
            BufferSize = sizeof(Buffer);
            ret = SendARP(IpAddress,
                          IpAddress,
                          Buffer,
                          &BufferSize);
            DH_DbgPrint(MID_TRACE,("DHCPCSVC: SendARP returned %lu\n", ret));
            if (ret != 0)
            {
                /* The IP address is not in use */
                DH_DbgPrint(MID_TRACE,("DHCPCSVC: Using automatic private address\n"));
                AddIPAddress(IpAddress,
                             htonl(0xFFFF0000), // 255.255.0.0
                             Adapter->IfMib.dwIndex,
                             &Adapter->NteContext,
                             &Adapter->NteInstance);
                return;
            }
        }
    }
}

void
send_request(void *ipp)
{
	struct interface_info *ip = ipp;
	struct sockaddr_in destination;
	struct in_addr from;
	int interval;
        time_t cur_time;

        time(&cur_time); 

	/* Figure out how long it's been since we started transmitting. */
	interval = cur_time - ip->client->first_sending;

	/* If we're in the INIT-REBOOT or REQUESTING state and we're
	   past the reboot timeout, go to INIT and see if we can
	   DISCOVER an address... */
	/* XXX In the INIT-REBOOT state, if we don't get an ACK, it
	   means either that we're on a network with no DHCP server,
	   or that our server is down.  In the latter case, assuming
	   that there is a backup DHCP server, DHCPDISCOVER will get
	   us a new address, but we could also have successfully
	   reused our old address.  In the former case, we're hosed
	   anyway.  This is not a win-prone situation. */
	if ((ip->client->state == S_REBOOTING ||
	    ip->client->state == S_REQUESTING) &&
	    interval > ip->client->config->reboot_timeout) {
		ip->client->state = S_INIT;
		cancel_timeout(send_request, ip);
		state_init(ip);
		return;
	}

	/* If we're in the reboot state, make sure the media is set up
	   correctly. */
	if (ip->client->state == S_REBOOTING &&
	    !ip->client->medium &&
	    ip->client->active->medium ) {
		/* If the medium we chose won't fly, go to INIT state. */
                /* XXX Nothing for now */

		/* Record the medium. */
		ip->client->medium = ip->client->active->medium;
	}

	/* If the lease has expired, relinquish the address and go back
	   to the INIT state. */
	if (ip->client->state != S_REQUESTING &&
	    cur_time > ip->client->active->expiry) {
            PDHCP_ADAPTER Adapter = AdapterFindInfo( ip );
            /* Run the client script with the new parameters. */
            /* No script actions necessary in the expiry case */
            /* Now do a preinit on the interface so that we can
               discover a new address. */

            if( Adapter )
            {
                DeleteIPAddress( Adapter->NteContext );
                Adapter->NteContext = 0;
            }

            ip->client->state = S_INIT;
            state_init(ip);
            return;
	}

	/* Do the exponential backoff... */
	if (!ip->client->interval)
		ip->client->interval = ip->client->config->initial_interval;
	else
		ip->client->interval += ((rand() >> 2) %
		    (2 * ip->client->interval));

	/* Don't backoff past cutoff. */
	if (ip->client->interval >
	    ip->client->config->backoff_cutoff)
		ip->client->interval =
		    ((ip->client->config->backoff_cutoff / 2) +
		    ((rand() >> 2) % ip->client->interval));

	/* If the backoff would take us to the expiry time, just set the
	   timeout to the expiry time. */
	if (ip->client->state != S_REQUESTING &&
	    cur_time + ip->client->interval >
	    ip->client->active->expiry)
		ip->client->interval =
		    ip->client->active->expiry - cur_time + 1;

	/* If the lease T2 time has elapsed, or if we're not yet bound,
	   broadcast the DHCPREQUEST rather than unicasting. */
	memset(&destination, 0, sizeof(destination));
	if (ip->client->state == S_REQUESTING ||
	    ip->client->state == S_REBOOTING ||
	    cur_time > ip->client->active->rebind)
		destination.sin_addr.s_addr = INADDR_BROADCAST;
	else
		memcpy(&destination.sin_addr.s_addr,
		    ip->client->destination.iabuf,
		    sizeof(destination.sin_addr.s_addr));
	destination.sin_port = htons(REMOTE_PORT);
	destination.sin_family = AF_INET;
//	destination.sin_len = sizeof(destination);

	if (ip->client->state != S_REQUESTING)
		memcpy(&from, ip->client->active->address.iabuf,
		    sizeof(from));
	else
		from.s_addr = INADDR_ANY;

	/* Record the number of seconds since we started sending. */
	if (ip->client->state == S_REQUESTING)
		ip->client->packet.secs = ip->client->secs;
	else {
		if (interval < 65536)
			ip->client->packet.secs = htons(interval);
		else
			ip->client->packet.secs = htons(65535);
	}

	note("DHCPREQUEST on %s to %s port %d", ip->name,
	    inet_ntoa(destination.sin_addr), ntohs(destination.sin_port));

	/* Send out a packet. */
	(void) send_packet(ip, &ip->client->packet, ip->client->packet_length,
	    from, &destination, NULL);

	add_timeout(cur_time + ip->client->interval, send_request, ip);
}

void
send_decline(void *ipp)
{
	struct interface_info *ip = ipp;

	note("DHCPDECLINE on %s to %s port %d", ip->name,
	    inet_ntoa(sockaddr_broadcast.sin_addr),
	    ntohs(sockaddr_broadcast.sin_port));

	/* Send out a packet. */
	(void) send_packet(ip, &ip->client->packet, ip->client->packet_length,
	    inaddr_any, &sockaddr_broadcast, NULL);
}

void
make_discover(struct interface_info *ip, struct client_lease *lease)
{
	unsigned char discover = DHCPDISCOVER;
	struct tree_cache *options[256];
	struct tree_cache option_elements[256];
	int i;
	ULONG foo = (ULONG) GetTickCount();

	memset(option_elements, 0, sizeof(option_elements));
	memset(options, 0, sizeof(options));
	memset(&ip->client->packet, 0, sizeof(ip->client->packet));

	/* Set DHCP_MESSAGE_TYPE to DHCPDISCOVER */
	i = DHO_DHCP_MESSAGE_TYPE;
	options[i] = &option_elements[i];
	options[i]->value = &discover;
	options[i]->len = sizeof(discover);
	options[i]->buf_size = sizeof(discover);
	options[i]->timeout = 0xFFFFFFFF;

	/* Request the options we want */
	i  = DHO_DHCP_PARAMETER_REQUEST_LIST;
	options[i] = &option_elements[i];
	options[i]->value = ip->client->config->requested_options;
	options[i]->len = ip->client->config->requested_option_count;
	options[i]->buf_size =
		ip->client->config->requested_option_count;
	options[i]->timeout = 0xFFFFFFFF;

	/* If we had an address, try to get it again. */
	if (lease) {
		ip->client->requested_address = lease->address;
		i = DHO_DHCP_REQUESTED_ADDRESS;
		options[i] = &option_elements[i];
		options[i]->value = lease->address.iabuf;
		options[i]->len = lease->address.len;
		options[i]->buf_size = lease->address.len;
		options[i]->timeout = 0xFFFFFFFF;
	} else
		ip->client->requested_address.len = 0;

	/* Send any options requested in the config file. */
	for (i = 0; i < 256; i++)
		if (!options[i] &&
		    ip->client->config->send_options[i].data) {
			options[i] = &option_elements[i];
			options[i]->value =
			    ip->client->config->send_options[i].data;
			options[i]->len =
			    ip->client->config->send_options[i].len;
			options[i]->buf_size =
			    ip->client->config->send_options[i].len;
			options[i]->timeout = 0xFFFFFFFF;
		}

	/* Set up the option buffer... */
	ip->client->packet_length = cons_options(NULL, &ip->client->packet, 0,
	    options);
	if (ip->client->packet_length < BOOTP_MIN_LEN)
		ip->client->packet_length = BOOTP_MIN_LEN;

	ip->client->packet.op = BOOTREQUEST;
	ip->client->packet.htype = ip->hw_address.htype;
	ip->client->packet.hlen = ip->hw_address.hlen;
	ip->client->packet.hops = 0;
	ip->client->packet.xid = RtlRandom(&foo);
	ip->client->packet.secs = 0; /* filled in by send_discover. */
	ip->client->packet.flags = 0;

	memset(&(ip->client->packet.ciaddr),
	    0, sizeof(ip->client->packet.ciaddr));
	memset(&(ip->client->packet.yiaddr),
	    0, sizeof(ip->client->packet.yiaddr));
	memset(&(ip->client->packet.siaddr),
	    0, sizeof(ip->client->packet.siaddr));
	memset(&(ip->client->packet.giaddr),
	    0, sizeof(ip->client->packet.giaddr));
	memcpy(ip->client->packet.chaddr,
	    ip->hw_address.haddr, ip->hw_address.hlen);
}


void
make_request(struct interface_info *ip, struct client_lease * lease)
{
	unsigned char request = DHCPREQUEST;
	struct tree_cache *options[256];
	struct tree_cache option_elements[256];
	int i;

	memset(options, 0, sizeof(options));
	memset(&ip->client->packet, 0, sizeof(ip->client->packet));

	/* Set DHCP_MESSAGE_TYPE to DHCPREQUEST */
	i = DHO_DHCP_MESSAGE_TYPE;
	options[i] = &option_elements[i];
	options[i]->value = &request;
	options[i]->len = sizeof(request);
	options[i]->buf_size = sizeof(request);
	options[i]->timeout = 0xFFFFFFFF;

	/* Request the options we want */
	i = DHO_DHCP_PARAMETER_REQUEST_LIST;
	options[i] = &option_elements[i];
	options[i]->value = ip->client->config->requested_options;
	options[i]->len = ip->client->config->requested_option_count;
	options[i]->buf_size =
		ip->client->config->requested_option_count;
	options[i]->timeout = 0xFFFFFFFF;

	/* If we are requesting an address that hasn't yet been assigned
	   to us, use the DHCP Requested Address option. */
	if (ip->client->state == S_REQUESTING) {
		/* Send back the server identifier... */
		i = DHO_DHCP_SERVER_IDENTIFIER;
		options[i] = &option_elements[i];
		options[i]->value = lease->options[i].data;
		options[i]->len = lease->options[i].len;
		options[i]->buf_size = lease->options[i].len;
		options[i]->timeout = 0xFFFFFFFF;
	}
	if (ip->client->state == S_REQUESTING ||
	    ip->client->state == S_REBOOTING) {
		ip->client->requested_address = lease->address;
		i = DHO_DHCP_REQUESTED_ADDRESS;
		options[i] = &option_elements[i];
		options[i]->value = lease->address.iabuf;
		options[i]->len = lease->address.len;
		options[i]->buf_size = lease->address.len;
		options[i]->timeout = 0xFFFFFFFF;
	} else
		ip->client->requested_address.len = 0;

	/* Send any options requested in the config file. */
	for (i = 0; i < 256; i++)
		if (!options[i] &&
		    ip->client->config->send_options[i].data) {
			options[i] = &option_elements[i];
			options[i]->value =
			    ip->client->config->send_options[i].data;
			options[i]->len =
			    ip->client->config->send_options[i].len;
			options[i]->buf_size =
			    ip->client->config->send_options[i].len;
			options[i]->timeout = 0xFFFFFFFF;
		}

	/* Set up the option buffer... */
	ip->client->packet_length = cons_options(NULL, &ip->client->packet, 0,
	    options);
	if (ip->client->packet_length < BOOTP_MIN_LEN)
		ip->client->packet_length = BOOTP_MIN_LEN;

	ip->client->packet.op = BOOTREQUEST;
	ip->client->packet.htype = ip->hw_address.htype;
	ip->client->packet.hlen = ip->hw_address.hlen;
	ip->client->packet.hops = 0;
	ip->client->packet.xid = ip->client->xid;
	ip->client->packet.secs = 0; /* Filled in by send_request. */

	/* If we own the address we're requesting, put it in ciaddr;
	   otherwise set ciaddr to zero. */
	if (ip->client->state == S_BOUND ||
	    ip->client->state == S_RENEWING ||
	    ip->client->state == S_REBINDING) {
		memcpy(&ip->client->packet.ciaddr,
		    lease->address.iabuf, lease->address.len);
		ip->client->packet.flags = 0;
	} else {
		memset(&ip->client->packet.ciaddr, 0,
		    sizeof(ip->client->packet.ciaddr));
		ip->client->packet.flags = 0;
	}

	memset(&ip->client->packet.yiaddr, 0,
	    sizeof(ip->client->packet.yiaddr));
	memset(&ip->client->packet.siaddr, 0,
	    sizeof(ip->client->packet.siaddr));
	memset(&ip->client->packet.giaddr, 0,
	    sizeof(ip->client->packet.giaddr));
	memcpy(ip->client->packet.chaddr,
	    ip->hw_address.haddr, ip->hw_address.hlen);
}

void
make_decline(struct interface_info *ip, struct client_lease *lease)
{
	struct tree_cache *options[256], message_type_tree;
	struct tree_cache requested_address_tree;
	struct tree_cache server_id_tree, client_id_tree;
	unsigned char decline = DHCPDECLINE;
	int i;

	memset(options, 0, sizeof(options));
	memset(&ip->client->packet, 0, sizeof(ip->client->packet));

	/* Set DHCP_MESSAGE_TYPE to DHCPDECLINE */
	i = DHO_DHCP_MESSAGE_TYPE;
	options[i] = &message_type_tree;
	options[i]->value = &decline;
	options[i]->len = sizeof(decline);
	options[i]->buf_size = sizeof(decline);
	options[i]->timeout = 0xFFFFFFFF;

	/* Send back the server identifier... */
	i = DHO_DHCP_SERVER_IDENTIFIER;
	options[i] = &server_id_tree;
	options[i]->value = lease->options[i].data;
	options[i]->len = lease->options[i].len;
	options[i]->buf_size = lease->options[i].len;
	options[i]->timeout = 0xFFFFFFFF;

	/* Send back the address we're declining. */
	i = DHO_DHCP_REQUESTED_ADDRESS;
	options[i] = &requested_address_tree;
	options[i]->value = lease->address.iabuf;
	options[i]->len = lease->address.len;
	options[i]->buf_size = lease->address.len;
	options[i]->timeout = 0xFFFFFFFF;

	/* Send the uid if the user supplied one. */
	i = DHO_DHCP_CLIENT_IDENTIFIER;
	if (ip->client->config->send_options[i].len) {
		options[i] = &client_id_tree;
		options[i]->value = ip->client->config->send_options[i].data;
		options[i]->len = ip->client->config->send_options[i].len;
		options[i]->buf_size = ip->client->config->send_options[i].len;
		options[i]->timeout = 0xFFFFFFFF;
	}


	/* Set up the option buffer... */
	ip->client->packet_length = cons_options(NULL, &ip->client->packet, 0,
	    options);
	if (ip->client->packet_length < BOOTP_MIN_LEN)
		ip->client->packet_length = BOOTP_MIN_LEN;

	ip->client->packet.op = BOOTREQUEST;
	ip->client->packet.htype = ip->hw_address.htype;
	ip->client->packet.hlen = ip->hw_address.hlen;
	ip->client->packet.hops = 0;
	ip->client->packet.xid = ip->client->xid;
	ip->client->packet.secs = 0; /* Filled in by send_request. */
	ip->client->packet.flags = 0;

	/* ciaddr must always be zero. */
	memset(&ip->client->packet.ciaddr, 0,
	    sizeof(ip->client->packet.ciaddr));
	memset(&ip->client->packet.yiaddr, 0,
	    sizeof(ip->client->packet.yiaddr));
	memset(&ip->client->packet.siaddr, 0,
	    sizeof(ip->client->packet.siaddr));
	memset(&ip->client->packet.giaddr, 0,
	    sizeof(ip->client->packet.giaddr));
	memcpy(ip->client->packet.chaddr,
	    ip->hw_address.haddr, ip->hw_address.hlen);
}

void
free_client_lease(struct client_lease *lease)
{
	int i;

	if (lease->server_name)
		free(lease->server_name);
	if (lease->filename)
		free(lease->filename);
	for (i = 0; i < 256; i++) {
		if (lease->options[i].len)
			free(lease->options[i].data);
	}
	free(lease);
}

FILE *leaseFile;

void
rewrite_client_leases(struct interface_info *ifi)
{
	struct client_lease *lp;

	if (!leaseFile) {
		leaseFile = fopen(path_dhclient_db, "w");
		if (!leaseFile)
			error("can't create %s", path_dhclient_db);
	} else {
		fflush(leaseFile);
		rewind(leaseFile);
	}

	for (lp = ifi->client->leases; lp; lp = lp->next)
		write_client_lease(ifi, lp, 1);
	if (ifi->client->active)
		write_client_lease(ifi, ifi->client->active, 1);

	fflush(leaseFile);
}

void
write_client_lease(struct interface_info *ip, struct client_lease *lease,
    int rewrite)
{
	static int leases_written;
	struct tm *t;
	int i;

	if (!rewrite) {
		if (leases_written++ > 20) {
			rewrite_client_leases(ip);
			leases_written = 0;
		}
	}

	/* If the lease came from the config file, we don't need to stash
	   a copy in the lease database. */
	if (lease->is_static)
		return;

	if (!leaseFile) {	/* XXX */
		leaseFile = fopen(path_dhclient_db, "w");
		if (!leaseFile) {
			error("can't create %s", path_dhclient_db);
                        return;
                }
	}

	fprintf(leaseFile, "lease {\n");
	if (lease->is_bootp)
		fprintf(leaseFile, "  bootp;\n");
	fprintf(leaseFile, "  interface \"%s\";\n", ip->name);
	fprintf(leaseFile, "  fixed-address %s;\n", piaddr(lease->address));
	if (lease->filename)
		fprintf(leaseFile, "  filename \"%s\";\n", lease->filename);
	if (lease->server_name)
		fprintf(leaseFile, "  server-name \"%s\";\n",
		    lease->server_name);
	if (lease->medium)
		fprintf(leaseFile, "  medium \"%s\";\n", lease->medium->string);
	for (i = 0; i < 256; i++)
		if (lease->options[i].len)
			fprintf(leaseFile, "  option %s %s;\n",
			    dhcp_options[i].name,
			    pretty_print_option(i, lease->options[i].data,
			    lease->options[i].len, 1, 1));

	t = gmtime(&lease->renewal);
        if (t)
	    fprintf(leaseFile, "  renew %d %d/%d/%d %02d:%02d:%02d;\n",
	        t->tm_wday, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
	        t->tm_hour, t->tm_min, t->tm_sec);
	t = gmtime(&lease->rebind);
        if (t)
	    fprintf(leaseFile, "  rebind %d %d/%d/%d %02d:%02d:%02d;\n",
	         t->tm_wday, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
	         t->tm_hour, t->tm_min, t->tm_sec);
	t = gmtime(&lease->expiry);
        if (t)
	    fprintf(leaseFile, "  expire %d %d/%d/%d %02d:%02d:%02d;\n",
	        t->tm_wday, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
	        t->tm_hour, t->tm_min, t->tm_sec);
	fprintf(leaseFile, "}\n");
	fflush(leaseFile);
}

void
priv_script_init(struct interface_info *ip, char *reason, char *medium)
{
	if (ip) {
            // XXX Do we need to do anything?
        }
}

void
priv_script_write_params(struct interface_info *ip, char *prefix, struct client_lease *lease)
{
	u_int8_t dbuf[1500];
	int i, len = 0;

#if 0
	script_set_env(ip->client, prefix, "ip_address",
	    piaddr(lease->address));
#endif

	if (lease->options[DHO_SUBNET_MASK].len &&
	    (lease->options[DHO_SUBNET_MASK].len <
	    sizeof(lease->address.iabuf))) {
		struct iaddr netmask, subnet, broadcast;

		memcpy(netmask.iabuf, lease->options[DHO_SUBNET_MASK].data,
		    lease->options[DHO_SUBNET_MASK].len);
		netmask.len = lease->options[DHO_SUBNET_MASK].len;

		subnet = subnet_number(lease->address, netmask);
		if (subnet.len) {
#if 0
			script_set_env(ip->client, prefix, "network_number",
			    piaddr(subnet));
#endif
			if (!lease->options[DHO_BROADCAST_ADDRESS].len) {
				broadcast = broadcast_addr(subnet, netmask);
				if (broadcast.len)
#if 0
					script_set_env(ip->client, prefix,
					    "broadcast_address",
					    piaddr(broadcast));
#else
                                ;
#endif
			}
		}
	}

#if 0
	if (lease->filename)
		script_set_env(ip->client, prefix, "filename", lease->filename);
	if (lease->server_name)
		script_set_env(ip->client, prefix, "server_name",
		    lease->server_name);
#endif

	for (i = 0; i < 256; i++) {
		u_int8_t *dp = NULL;

		if (ip->client->config->defaults[i].len) {
			if (lease->options[i].len) {
				switch (
				    ip->client->config->default_actions[i]) {
				case ACTION_DEFAULT:
					dp = lease->options[i].data;
					len = lease->options[i].len;
					break;
				case ACTION_SUPERSEDE:
supersede:
					dp = ip->client->
						config->defaults[i].data;
					len = ip->client->
						config->defaults[i].len;
					break;
				case ACTION_PREPEND:
					len = ip->client->
					    config->defaults[i].len +
					    lease->options[i].len;
					if (len >= sizeof(dbuf)) {
						warning("no space to %s %s",
						    "prepend option",
						    dhcp_options[i].name);
						goto supersede;
					}
					dp = dbuf;
					memcpy(dp,
						ip->client->
						config->defaults[i].data,
						ip->client->
						config->defaults[i].len);
					memcpy(dp + ip->client->
						config->defaults[i].len,
						lease->options[i].data,
						lease->options[i].len);
					dp[len] = '\0';
					break;
				case ACTION_APPEND:
					len = ip->client->
					    config->defaults[i].len +
					    lease->options[i].len + 1;
					if (len > sizeof(dbuf)) {
						warning("no space to %s %s",
						    "append option",
						    dhcp_options[i].name);
						goto supersede;
					}
					dp = dbuf;
					memcpy(dp,
						lease->options[i].data,
						lease->options[i].len);
					memcpy(dp + lease->options[i].len,
						ip->client->
						config->defaults[i].data,
						ip->client->
						config->defaults[i].len);
					dp[len-1] = '\0';
				}
			} else {
				dp = ip->client->
					config->defaults[i].data;
				len = ip->client->
					config->defaults[i].len;
			}
		} else if (lease->options[i].len) {
			len = lease->options[i].len;
			dp = lease->options[i].data;
		} else {
			len = 0;
		}
#if 0
		if (len) {
			char name[256];

			if (dhcp_option_ev_name(name, sizeof(name),
			    &dhcp_options[i]))
				script_set_env(ip->client, prefix, name,
				    pretty_print_option(i, dp, len, 0, 0));
		}
#endif
	}
#if 0
	snprintf(tbuf, sizeof(tbuf), "%d", (int)lease->expiry);
	script_set_env(ip->client, prefix, "expiry", tbuf);
#endif
}

int
dhcp_option_ev_name(char *buf, size_t buflen, struct dhcp_option *option)
{
	int i;

	for (i = 0; option->name[i]; i++) {
		if (i + 1 == buflen)
			return 0;
		if (option->name[i] == '-')
			buf[i] = '_';
		else
			buf[i] = option->name[i];
	}

	buf[i] = 0;
	return 1;
}

#if 0
void
go_daemon(void)
{
	static int state = 0;

	if (no_daemon || state)
		return;

	state = 1;

	/* Stop logging to stderr... */
	log_perror = 0;

	if (daemon(1, 0) == -1)
		error("daemon");

	/* we are chrooted, daemon(3) fails to open /dev/null */
	if (nullfd != -1) {
		dup2(nullfd, STDIN_FILENO);
		dup2(nullfd, STDOUT_FILENO);
		dup2(nullfd, STDERR_FILENO);
		close(nullfd);
		nullfd = -1;
	}
}
#endif

int
check_option(struct client_lease *l, int option)
{
	char *opbuf;
	char *sbuf;

	/* we use this, since this is what gets passed to dhclient-script */

	opbuf = pretty_print_option(option, l->options[option].data,
	    l->options[option].len, 0, 0);

	sbuf = option_as_string(option, l->options[option].data,
	    l->options[option].len);

	switch (option) {
	case DHO_SUBNET_MASK:
	case DHO_TIME_SERVERS:
	case DHO_NAME_SERVERS:
	case DHO_ROUTERS:
	case DHO_DOMAIN_NAME_SERVERS:
	case DHO_LOG_SERVERS:
	case DHO_COOKIE_SERVERS:
	case DHO_LPR_SERVERS:
	case DHO_IMPRESS_SERVERS:
	case DHO_RESOURCE_LOCATION_SERVERS:
	case DHO_SWAP_SERVER:
	case DHO_BROADCAST_ADDRESS:
	case DHO_NIS_SERVERS:
	case DHO_NTP_SERVERS:
	case DHO_NETBIOS_NAME_SERVERS:
	case DHO_NETBIOS_DD_SERVER:
	case DHO_FONT_SERVERS:
	case DHO_DHCP_SERVER_IDENTIFIER:
		if (!ipv4addrs(opbuf)) {
                        warning("Invalid IP address in option(%d): %s", option, opbuf);
			return (0);
		}
		return (1)  ;
	case DHO_HOST_NAME:
	case DHO_DOMAIN_NAME:
	case DHO_NIS_DOMAIN:
		if (!res_hnok(sbuf))
			warning("Bogus Host Name option %d: %s (%s)", option,
			    sbuf, opbuf);
		return (1);
	case DHO_PAD:
	case DHO_TIME_OFFSET:
	case DHO_BOOT_SIZE:
	case DHO_MERIT_DUMP:
	case DHO_ROOT_PATH:
	case DHO_EXTENSIONS_PATH:
	case DHO_IP_FORWARDING:
	case DHO_NON_LOCAL_SOURCE_ROUTING:
	case DHO_POLICY_FILTER:
	case DHO_MAX_DGRAM_REASSEMBLY:
	case DHO_DEFAULT_IP_TTL:
	case DHO_PATH_MTU_AGING_TIMEOUT:
	case DHO_PATH_MTU_PLATEAU_TABLE:
	case DHO_INTERFACE_MTU:
	case DHO_ALL_SUBNETS_LOCAL:
	case DHO_PERFORM_MASK_DISCOVERY:
	case DHO_MASK_SUPPLIER:
	case DHO_ROUTER_DISCOVERY:
	case DHO_ROUTER_SOLICITATION_ADDRESS:
	case DHO_STATIC_ROUTES:
	case DHO_TRAILER_ENCAPSULATION:
	case DHO_ARP_CACHE_TIMEOUT:
	case DHO_IEEE802_3_ENCAPSULATION:
	case DHO_DEFAULT_TCP_TTL:
	case DHO_TCP_KEEPALIVE_INTERVAL:
	case DHO_TCP_KEEPALIVE_GARBAGE:
	case DHO_VENDOR_ENCAPSULATED_OPTIONS:
	case DHO_NETBIOS_NODE_TYPE:
	case DHO_NETBIOS_SCOPE:
	case DHO_X_DISPLAY_MANAGER:
	case DHO_DHCP_REQUESTED_ADDRESS:
	case DHO_DHCP_LEASE_TIME:
	case DHO_DHCP_OPTION_OVERLOAD:
	case DHO_DHCP_MESSAGE_TYPE:
	case DHO_DHCP_PARAMETER_REQUEST_LIST:
	case DHO_DHCP_MESSAGE:
	case DHO_DHCP_MAX_MESSAGE_SIZE:
	case DHO_DHCP_RENEWAL_TIME:
	case DHO_DHCP_REBINDING_TIME:
	case DHO_DHCP_CLASS_IDENTIFIER:
	case DHO_DHCP_CLIENT_IDENTIFIER:
	case DHO_DHCP_USER_CLASS_ID:
	case DHO_END:
		return (1);
	default:
		warning("unknown dhcp option value 0x%x", option);
		return (unknown_ok);
	}
}

int
res_hnok(const char *dn)
{
	int pch = PERIOD, ch = *dn++;

	while (ch != '\0') {
		int nch = *dn++;

		if (periodchar(ch)) {
			;
		} else if (periodchar(pch)) {
			if (!borderchar(ch))
				return (0);
		} else if (periodchar(nch) || nch == '\0') {
			if (!borderchar(ch))
				return (0);
		} else {
			if (!middlechar(ch))
				return (0);
		}
		pch = ch, ch = nch;
	}
	return (1);
}

/* Does buf consist only of dotted decimal ipv4 addrs?
 * return how many if so,
 * otherwise, return 0
 */
int
ipv4addrs(char * buf)
{
    char *tmp;
    struct in_addr jnk;
    int i = 0;

    note("Input: %s", buf);

    do {
        tmp = strtok(buf, " ");
        note("got %s", tmp);
		if( tmp && inet_aton(tmp, &jnk) ) i++;
        buf = NULL;
    } while( tmp );

    return (i);
}


char *
option_as_string(unsigned int code, unsigned char *data, int len)
{
	static char optbuf[32768]; /* XXX */
	char *op = optbuf;
	int opleft = sizeof(optbuf);
	unsigned char *dp = data;

	if (code > 255)
		error("option_as_string: bad code %d", code);

	for (; dp < data + len; dp++) {
		if (!isascii(*dp) || !isprint(*dp)) {
			if (dp + 1 != data + len || *dp != 0) {
				_snprintf(op, opleft, "\\%03o", *dp);
				op += 4;
				opleft -= 4;
			}
		} else if (*dp == '"' || *dp == '\'' || *dp == '$' ||
		    *dp == '`' || *dp == '\\') {
			*op++ = '\\';
			*op++ = *dp;
			opleft -= 2;
		} else {
			*op++ = *dp;
			opleft--;
		}
	}
	if (opleft < 1)
		goto toobig;
	*op = 0;
	return optbuf;
toobig:
	warning("dhcp option too large");
	return "<error>";
}

