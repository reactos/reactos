/*
 * Copyright (c) 2001
 *	Politecnico di Torino.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the Politecnico
 * di Torino, and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef __tcp_session
#define __tcp_session

#ifdef WIN32
#include "tme.h"
#endif

#ifdef __FreeBSD__

#ifdef _KERNEL
#include <net/tme/tme.h>
#else
#include <tme/tme.h>
#endif

#endif

#define UNKNOWN			0
#define SYN_RCV			1
#define SYN_ACK_RCV		2
#define ESTABLISHED		3
#define CLOSED_RST		4
#define FIN_CLN_RCV		5
#define FIN_SRV_RCV		6
#define CLOSED_FIN		7
#define ERROR_TCP		8
#define FIRST_IS_CLN	0
#define	FIRST_IS_SRV	0xffffffff
#define FIN_CLN			1
#define	FIN_SRV			2

#define MAX_WINDOW 65536

typedef struct __tcp_data
{
	struct timeval timestamp_block; /*DO NOT MOVE THIS VALUE*/
	struct timeval syn_timestamp;
	struct timeval last_timestamp;
	struct timeval syn_ack_timestamp;
	uint32 direction;
	uint32 seq_n_0_srv;
	uint32 seq_n_0_cln;
	uint32 ack_srv; /* acknowledge of (data sent by server) */
	uint32 ack_cln; /* acknowledge of (data sent by client) */
	uint32 status;
	uint32 pkts_cln_to_srv;
	uint32 pkts_srv_to_cln;
	uint32 bytes_srv_to_cln;
	uint32 bytes_cln_to_srv;
	uint32 close_state;
}
	 tcp_data;

#define FIN		1
#define	SYN		2
#define RST		4
#define PSH		8
#define ACK		16
#define URG		32

#define	TCP_SESSION						0x00000800
uint32 tcp_session(uint8 *block, uint32 pkt_size, TME_DATA *data, MEM_TYPE *mem_ex, uint8 *mem_data);

#endif