/**************************************************************************
*
*    proto_tftm.c -- Etherboot Multicast TFTP 
*    Written 2003-2003 by Timothy Legge <tlegge@rogers.com>
*
*    This program is free software; you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation; either version 2 of the License, or
*    (at your option) any later version.
*
*    This program is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software
*    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
*    This code is based on the DOWNLOAD_PROTO_TFTM section of 
*    Etherboot 5.3 core/nic.c and:
*    
*    Anselm Martin Hoffmeister's previous proto_tftm.c multicast work
*    Eric Biederman's proto_slam.c
*
*    $Revision: 1.1 $
*    $Author: oliverschwartz $
*    $Date: 2004/03/21 20:38:41 $
*
*    REVISION HISTORY:
*    ================
*    09-07-2003	timlegge	Release Version, Capable of Multicast Booting
*    08-30-2003	timlegge	Initial version, Assumes consecutive blocks
*
*    Indent Options: indent -kr -i8
***************************************************************************/

#ifdef DOWNLOAD_PROTO_TFTM
#include "etherboot.h"
#include "nic.h"

//#define TFTM_DEBUG
#ifdef TFTM_DEBUG
#define debug(x) printf x
#else
#define debug(x)
#endif
struct tftm_info {
	in_addr server_ip;
	in_addr multicast_ip;
	in_addr local_ip;
	uint16_t server_port;
	uint16_t multicast_port;
	uint16_t local_port;
	int (*fnc) (unsigned char *, unsigned int, unsigned int, int);
	int sent_nack;
	const char *name;	/* Filename */
};

struct tftm_state {
	unsigned long block_size;
	unsigned long total_bytes;
	unsigned long total_packets;
	char ismaster;
	unsigned long received_packets;
	unsigned char *image;
	unsigned char *bitmap;
	char recvd_oack;
} state;

#define TFTM_PORT 1758
#define TFTM_MIN_PACKET 1024


int opt_get_multicast(struct tftp_t *tr, unsigned short *len,
		      unsigned long *filesize, struct tftm_info *info);

static int await_tftm(int ival, void *ptr, unsigned short ptype __unused,
		      struct iphdr *ip, struct udphdr *udp)
{
	struct tftm_info *info = ptr;

	/* Check for Unicast data being received */
	if (ip->dest.s_addr == arptable[ARP_CLIENT].ipaddr.s_addr) {
		if (!udp) {
			return 0;
		}
		if (arptable[ARP_CLIENT].ipaddr.s_addr != ip->dest.s_addr)
			return 0;
		if (ntohs(udp->dest) != ival)
			return 0;

		return 1;	/* Unicast Data Received */
	}

	/* Also check for Multicast data being received */
	if ((ip->dest.s_addr == info->multicast_ip.s_addr) &&
	    (ntohs(udp->dest) == info->multicast_port) &&
	    (nic.packetlen >= ETH_HLEN + sizeof(struct iphdr) +
	     sizeof(struct udphdr))) {
		return 1;	/* Multicast data received */
	}
	return 0;
}

int proto_tftm(struct tftm_info *info)
{
	int retry = 0;
	static unsigned short iport = 2000;
	unsigned short oport = 0;
	unsigned short len, block = 0, prevblock = 0;
	struct tftp_t *tr;
	struct tftpreq_t tp;
	unsigned long filesize = 0;

	state.image = 0;
	state.bitmap = 0;

	rx_qdrain();

	/* Warning: the following assumes the layout of bootp_t.
	   But that's fixed by the IP, UDP and BOOTP specs. */

	/* Send a tftm-request to the server */
	tp.opcode = htons(TFTP_RRQ);	/* Const for "\0x0" "\0x1" =^= ReadReQuest */
	len =
	    sizeof(tp.ip) + sizeof(tp.udp) + sizeof(tp.opcode) +
	    sprintf((char *) tp.u.rrq,
		    "%s%coctet%cmulticast%c%cblksize%c%d%ctsize%c",
		    info->name, 0, 0, 0, 0, 0, TFTM_MIN_PACKET, 0, 0) + 1;

	if (!udp_transmit(arptable[ARP_SERVER].ipaddr.s_addr, ++iport,
			  TFTM_PORT, len, &tp))
		return (0);

	/* loop to listen for packets and to receive the file */
	for (;;) {
		long timeout;
#ifdef	CONGESTED
		timeout =
		    rfc2131_sleep_interval(block ? TFTP_REXMT : TIMEOUT,
					   retry);
#else
		timeout = rfc2131_sleep_interval(TIMEOUT, retry);
#endif
		/* Calls the await_reply function in nic.c which in turn calls
		   await_tftm (1st parameter) as above */
		if (!await_reply(await_tftm, iport, info, timeout)) {
			if (!block && retry++ < MAX_TFTP_RETRIES) {	/* maybe initial request was lost */
				if (!udp_transmit
				    (arptable[ARP_SERVER].ipaddr.s_addr,
				     ++iport, TFTM_PORT, len, &tp))
					return (0);
				continue;
			}
#ifdef	CONGESTED
			if (block && ((retry += TFTP_REXMT) < TFTP_TIMEOUT)) {	/* we resend our last ack */
#ifdef	MDEBUG
				printf("<REXMT>\n");
#endif
				debug(("Timed out receiving file"));
				len =
				    sizeof(tp.ip) + sizeof(tp.udp) +
				    sizeof(tp.opcode) +
				    sprintf((char *) tp.u.rrq,
					    "%s%coctet%cmulticast%c%cblksize%c%d%ctsize%c",
					    info->name, 0, 0, 0, 0, 0,
					    TFTM_MIN_PACKET, 0, 0) + 1;

				udp_transmit
				    (arptable[ARP_SERVER].ipaddr.s_addr,
				     ++iport, TFTM_PORT, len, &tp);
					continue;
			}
#endif
			break;	/* timeout */
		}

		tr = (struct tftp_t *) &nic.packet[ETH_HLEN];

		if (tr->opcode == ntohs(TFTP_ERROR)) {
			printf("TFTP error %d (%s)\n",
			       ntohs(tr->u.err.errcode), tr->u.err.errmsg);
			break;
		}

		if (tr->opcode == ntohs(TFTP_OACK)) {
			int i =
			    opt_get_multicast(tr, &len, &filesize, info);

			if (i == 0 || (i != 7 && !state.recvd_oack)) {	/* Multicast unsupported */
				/* Transmit an error message to the server to end the transmission */
				printf
				    ("TFTM-Server doesn't understand options [blksize tsize multicast]\n");
				tp.opcode = htons(TFTP_ERROR);
				tp.u.err.errcode = 8;
				/*
				 *      Warning: the following assumes the layout of bootp_t.
				 *      But that's fixed by the IP, UDP and BOOTP specs.
				 */
				len =
				    sizeof(tp.ip) + sizeof(tp.udp) +
				    sizeof(tp.opcode) +
				    sizeof(tp.u.err.errcode) +
				    /*
				     *      Normally bad form to omit the format string, but in this case
				     *      the string we are copying from is fixed. sprintf is just being
				     *      used as a strcpy and strlen.
				     */
				    sprintf((char *) tp.u.err.errmsg,
					    "RFC2090 error") + 1;
				udp_transmit(arptable[ARP_SERVER].ipaddr.
					     s_addr, iport,
					     ntohs(tr->udp.src), len, &tp);
				block = tp.u.ack.block = 0;	/* this ensures, that */
				/* the packet does not get */
				/* processed as data! */
				return (0);
			} else {
				unsigned long bitmap_len;
				/* */
				if (!state.recvd_oack) {

					state.total_packets =
					    1 + (filesize -
						 (filesize %
						  state.block_size)) /
					    state.block_size;
					bitmap_len =
					    (state.total_packets + 7) / 8;
					if (!state.image) {
						state.bitmap =
						    allot(bitmap_len);
						state.image =
						    allot(filesize);

						if ((unsigned long) state.
						    image < 1024 * 1024) {
							printf
							    ("ALERT: tftp filesize to large for available memory\n");
							return 0;
						}
						memset(state.bitmap, 0,
						       bitmap_len);
					}
					/* If I'm running over multicast join the multicast group */
					join_group(IGMP_SERVER,
						   info->multicast_ip.
						   s_addr);
				}
				state.recvd_oack = 1;
			}



		} else if (tr->opcode == htons(TFTP_DATA)) {
			unsigned long data_len;
			unsigned char *data;
			struct udphdr *udp;
			udp =
			    (struct udphdr *) &nic.packet[ETH_HLEN +
							  sizeof(struct
								 iphdr)];
			len =
			    ntohs(tr->udp.len) - sizeof(struct udphdr) - 4;
			data =
			    nic.packet + ETH_HLEN + sizeof(struct iphdr) +
			    sizeof(struct udphdr) + 4;

			if (len > TFTM_MIN_PACKET)	/* shouldn't happen */
				continue;	/* ignore it */

			block = ntohs(tp.u.ack.block = tr->u.data.block);

			if (block > state.total_packets) {
				printf("ALERT: Invalid packet number\n");
				continue;
			}

			/* Compute the expected data length */
			if (block != state.total_packets) {
				data_len = state.block_size;
			} else {
				data_len = filesize % state.block_size;
			}
			/* If the packet size is wrong drop the packet and then continue */
			if (ntohs(udp->len) !=
			    (data_len + (data - (unsigned char *) udp))) {
				printf
				    ("ALERT: udp packet is not the correct size: %d\n",
				     block);
				continue;
			}
			if (nic.packetlen < data_len + (data - nic.packet)) {
				printf
				    ("ALERT: Ethernet packet shorter than data_len: %d\n",
				     block);
				continue;
			}

			if (data_len > state.block_size) {
				data_len = state.block_size;
			}
			if (((state.
			      bitmap[block >> 3] >> (block & 7)) & 1) ==
			    0) {
				/* Non duplicate packet */
				state.bitmap[block >> 3] |=
				    (1 << (block & 7));
				memcpy(state.image +
				       ((block - 1) * state.block_size),
				       data, data_len);
				state.received_packets++;
			} else {

/*				printf("<DUP>\n"); */
			}
		}

		else {		/* neither TFTP_OACK, TFTP_DATA nor TFTP_ERROR */
			break;
		}

		if (state.received_packets <= state.total_packets) {
			unsigned long b;
			unsigned long len;
			unsigned long max;
			int value;
			int last;

			/* Compute the last bit and store an inverted trailer */
			max = state.total_packets + 1;
			value =
			    ((state.
			      bitmap[(max - 1) >> 3] >> ((max -
							  1) & 7)) & 1);
			value = !value;
			state.bitmap[max >> 3] &= ~(1 << (max & 7));
			state.bitmap[max >> 3] |= value << (max & 7);

			len = 0;
			last = 0;	/* Start with the received packets */
			for (b = 1; b <= max; b++) {
				value =
				    (state.bitmap[b >> 3] >> (b & 7)) & 1;

				if (value == 0) {
					tp.u.ack.block = htons(b - 1);	/* Acknowledge the previous block */
					break;
				}
			}
		}
		if (state.ismaster) {
			tp.opcode = htons(TFTP_ACK);
			oport = ntohs(tr->udp.src);
			udp_transmit(arptable[ARP_SERVER].ipaddr.s_addr, iport, oport, TFTP_MIN_PACKET, &tp);	/* ack */
		}
		if (state.received_packets == state.total_packets) {
			/* If the client is finished and not the master,
			 * ack the last packet */
			if (!state.ismaster) {
				tp.opcode = htons(TFTP_ACK);
				/* Ack Last packet to end xfer */
				tp.u.ack.block = htons(state.total_packets);
				oport = ntohs(tr->udp.src);
				udp_transmit(arptable[ARP_SERVER].ipaddr.s_addr, iport, oport, TFTP_MIN_PACKET, &tp);	/* ack */
			}
			/* We are done get out */
			forget(state.bitmap);
			break;
		}

		if ((unsigned short) (block - prevblock) != 1) {
			/* Retransmission or OACK, don't process via callback
			 * and don't change the value of prevblock.  */
			continue;
		}

		prevblock = block;
		retry = 0;	/* It's the right place to zero the timer? */

	}
	/* Leave the multicast group */
	leave_group(IGMP_SERVER);
	return info->fnc(state.image, 1, filesize, 1);
}

int url_tftm(const char *name,
	     int (*fnc) (unsigned char *, unsigned int, unsigned int, int))
{

	int ret;
	struct tftm_info info;

	/* Set the defaults */
	info.server_ip.s_addr = arptable[ARP_SERVER].ipaddr.s_addr;
	info.server_port = TFTM_PORT;
	info.local_ip.s_addr = arptable[ARP_CLIENT].ipaddr.s_addr;
	info.local_port = TFTM_PORT;	/* Does not matter. So take tftm port too. */
	info.multicast_ip.s_addr = info.local_ip.s_addr;
	info.multicast_port = TFTM_PORT;
	info.fnc = fnc;
	state.ismaster = 0;
	info.name = name;

	state.block_size = 0;
	state.total_bytes = 0;
	state.total_packets = 0;
	state.received_packets = 0;
	state.image = 0;
	state.bitmap = 0;
	state.recvd_oack = 0;

	if (name[0] != '/') {
		/* server ip given, so use it */
		name += inet_aton(info.name, &info.server_ip);
		/* No way to specify another port for now */
	}
	if (name[0] != '/') {
		printf("Bad tftm-URI: [%s]\n", info.name);
		return 0;
	}

	ret = proto_tftm(&info);

	return ret;
}

/******************************
* Parse the multicast options
*******************************/
int opt_get_multicast(struct tftp_t *tr, unsigned short *len,
		      unsigned long *filesize, struct tftm_info *info)
{
	const char *p = tr->u.oack.data, *e = 0;
	int i = 0;
	*len = ntohs(tr->udp.len) - sizeof(struct udphdr) - 2;
	if (*len > TFTM_MIN_PACKET)
		return -1;
	e = p + *len;

	while (*p != '\0' && p < e) {
		if (!strcasecmp("tsize", p)) {
			p += 6;
			if ((*filesize = strtoul(p, &p, 10)) > 0)
				i |= 4;
			debug(("\n"));
			debug(("tsize=%d\n", *filesize));
			while (p < e && *p)
				p++;
			if (p < e)
				p++;
		} else if (!strcasecmp("blksize", p)) {
			i |= 2;
			p += 8;
			state.block_size = strtoul(p, &p, 10);
			if (state.block_size != TFTM_MIN_PACKET) {
				printf
				    ("TFTM-Server rejected required transfer blocksize %d\n",
				     TFTM_MIN_PACKET);
				return 0;
			}
			debug(("blksize=%d\n", state.block_size));
			while (p < e && *p)
				p++;
			if (p < e)
				p++;
		} else if (!strncmp(p, "multicast", 10)) {
			i |= 1;
			p += 10;
			debug(("multicast options: %s\n", p));
			p += 1 + inet_aton(p, &info->multicast_ip);
			debug(("multicast ip = %@\n", info->multicast_ip));
			info->multicast_port = strtoul(p, &p, 10);
			++p;
			debug(("multicast port = %d\n",
			       info->multicast_port));
			state.ismaster = (*p == '1' ? 1 : 0);
			debug(("multicast ismaster = %d\n",
			       state.ismaster));
			while (p < e && *p)
				p++;
			if (p < e)
				p++;
		}
	}
	if (p > e)
		return 0;
	return i;
}
#endif				/* DOWNLOAD_PROTO_TFTP */
