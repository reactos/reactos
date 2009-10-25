/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: tcpmsg.h,v 1.22 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_TCPMSG_H
#define DNS_TCPMSG_H 1

/*! \file dns/tcpmsg.h */

#include <isc/buffer.h>
#include <isc/lang.h>
#include <isc/socket.h>

typedef struct dns_tcpmsg {
	/* private (don't touch!) */
	unsigned int		magic;
	isc_uint16_t		size;
	isc_buffer_t		buffer;
	unsigned int		maxsize;
	isc_mem_t	       *mctx;
	isc_socket_t	       *sock;
	isc_task_t	       *task;
	isc_taskaction_t	action;
	void		       *arg;
	isc_event_t		event;
	/* public (read-only) */
	isc_result_t		result;
	isc_sockaddr_t		address;
} dns_tcpmsg_t;

ISC_LANG_BEGINDECLS

void
dns_tcpmsg_init(isc_mem_t *mctx, isc_socket_t *sock, dns_tcpmsg_t *tcpmsg);
/*%<
 * Associate a tcp message state with a given memory context and
 * TCP socket.
 *
 * Requires:
 *
 *\li	"mctx" and "sock" be non-NULL and valid types.
 *
 *\li	"sock" be a read/write TCP socket.
 *
 *\li	"tcpmsg" be non-NULL and an uninitialized or invalidated structure.
 *
 * Ensures:
 *
 *\li	"tcpmsg" is a valid structure.
 */

void
dns_tcpmsg_setmaxsize(dns_tcpmsg_t *tcpmsg, unsigned int maxsize);
/*%<
 * Set the maximum packet size to "maxsize"
 *
 * Requires:
 *
 *\li	"tcpmsg" be valid.
 *
 *\li	512 <= "maxsize" <= 65536
 */

isc_result_t
dns_tcpmsg_readmessage(dns_tcpmsg_t *tcpmsg,
		       isc_task_t *task, isc_taskaction_t action, void *arg);
/*%<
 * Schedule an event to be delivered when a DNS message is readable, or
 * when an error occurs on the socket.
 *
 * Requires:
 *
 *\li	"tcpmsg" be valid.
 *
 *\li	"task", "taskaction", and "arg" be valid.
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS		-- no error
 *\li	Anything that the isc_socket_recv() call can return.  XXXMLG
 *
 * Notes:
 *
 *\li	The event delivered is a fully generic event.  It will contain no
 *	actual data.  The sender will be a pointer to the dns_tcpmsg_t.
 *	The result code inside that structure should be checked to see
 *	what the final result was.
 */

void
dns_tcpmsg_cancelread(dns_tcpmsg_t *tcpmsg);
/*%<
 * Cancel a readmessage() call.  The event will still be posted with a
 * CANCELED result code.
 *
 * Requires:
 *
 *\li	"tcpmsg" be valid.
 */

void
dns_tcpmsg_keepbuffer(dns_tcpmsg_t *tcpmsg, isc_buffer_t *buffer);
/*%<
 * If a dns buffer is to be kept between calls, this function marks the
 * internal state-machine buffer as invalid, and copies all the contents
 * of the state into "buffer".
 *
 * Requires:
 *
 *\li	"tcpmsg" be valid.
 *
 *\li	"buffer" be non-NULL.
 */

void
dns_tcpmsg_invalidate(dns_tcpmsg_t *tcpmsg);
/*%<
 * Clean up all allocated state, and invalidate the structure.
 *
 * Requires:
 *
 *\li	"tcpmsg" be valid.
 *
 * Ensures:
 *
 *\li	"tcpmsg" is invalidated and disassociated with all memory contexts,
 *	sockets, etc.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TCPMSG_H */
