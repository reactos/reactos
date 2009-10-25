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

/* $Id: tcpmsg.c,v 1.31 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/mem.h>
#include <isc/task.h>
#include <isc/util.h>

#include <dns/events.h>
#include <dns/result.h>
#include <dns/tcpmsg.h>

#ifdef TCPMSG_DEBUG
#include <stdio.h>		/* Required for printf. */
#define XDEBUG(x) printf x
#else
#define XDEBUG(x)
#endif

#define TCPMSG_MAGIC		ISC_MAGIC('T', 'C', 'P', 'm')
#define VALID_TCPMSG(foo)	ISC_MAGIC_VALID(foo, TCPMSG_MAGIC)

static void recv_length(isc_task_t *, isc_event_t *);
static void recv_message(isc_task_t *, isc_event_t *);


static void
recv_length(isc_task_t *task, isc_event_t *ev_in) {
	isc_socketevent_t *ev = (isc_socketevent_t *)ev_in;
	isc_event_t *dev;
	dns_tcpmsg_t *tcpmsg = ev_in->ev_arg;
	isc_region_t region;
	isc_result_t result;

	INSIST(VALID_TCPMSG(tcpmsg));

	dev = &tcpmsg->event;
	tcpmsg->address = ev->address;

	if (ev->result != ISC_R_SUCCESS) {
		tcpmsg->result = ev->result;
		goto send_and_free;
	}

	/*
	 * Success.
	 */
	tcpmsg->size = ntohs(tcpmsg->size);
	if (tcpmsg->size == 0) {
		tcpmsg->result = ISC_R_UNEXPECTEDEND;
		goto send_and_free;
	}
	if (tcpmsg->size > tcpmsg->maxsize) {
		tcpmsg->result = ISC_R_RANGE;
		goto send_and_free;
	}

	region.base = isc_mem_get(tcpmsg->mctx, tcpmsg->size);
	region.length = tcpmsg->size;
	if (region.base == NULL) {
		tcpmsg->result = ISC_R_NOMEMORY;
		goto send_and_free;
	}
	XDEBUG(("Allocated %d bytes\n", tcpmsg->size));

	isc_buffer_init(&tcpmsg->buffer, region.base, region.length);
	result = isc_socket_recv(tcpmsg->sock, &region, 0,
				 task, recv_message, tcpmsg);
	if (result != ISC_R_SUCCESS) {
		tcpmsg->result = result;
		goto send_and_free;
	}

	isc_event_free(&ev_in);
	return;

 send_and_free:
	isc_task_send(tcpmsg->task, &dev);
	tcpmsg->task = NULL;
	isc_event_free(&ev_in);
	return;
}

static void
recv_message(isc_task_t *task, isc_event_t *ev_in) {
	isc_socketevent_t *ev = (isc_socketevent_t *)ev_in;
	isc_event_t *dev;
	dns_tcpmsg_t *tcpmsg = ev_in->ev_arg;

	(void)task;

	INSIST(VALID_TCPMSG(tcpmsg));

	dev = &tcpmsg->event;
	tcpmsg->address = ev->address;

	if (ev->result != ISC_R_SUCCESS) {
		tcpmsg->result = ev->result;
		goto send_and_free;
	}

	tcpmsg->result = ISC_R_SUCCESS;
	isc_buffer_add(&tcpmsg->buffer, ev->n);

	XDEBUG(("Received %d bytes (of %d)\n", ev->n, tcpmsg->size));

 send_and_free:
	isc_task_send(tcpmsg->task, &dev);
	tcpmsg->task = NULL;
	isc_event_free(&ev_in);
}

void
dns_tcpmsg_init(isc_mem_t *mctx, isc_socket_t *sock, dns_tcpmsg_t *tcpmsg) {
	REQUIRE(mctx != NULL);
	REQUIRE(sock != NULL);
	REQUIRE(tcpmsg != NULL);

	tcpmsg->magic = TCPMSG_MAGIC;
	tcpmsg->size = 0;
	tcpmsg->buffer.base = NULL;
	tcpmsg->buffer.length = 0;
	tcpmsg->maxsize = 65535;		/* Largest message possible. */
	tcpmsg->mctx = mctx;
	tcpmsg->sock = sock;
	tcpmsg->task = NULL;			/* None yet. */
	tcpmsg->result = ISC_R_UNEXPECTED;	/* None yet. */
	/*
	 * Should probably initialize the event here, but it can wait.
	 */
}


void
dns_tcpmsg_setmaxsize(dns_tcpmsg_t *tcpmsg, unsigned int maxsize) {
	REQUIRE(VALID_TCPMSG(tcpmsg));
	REQUIRE(maxsize < 65536);

	tcpmsg->maxsize = maxsize;
}


isc_result_t
dns_tcpmsg_readmessage(dns_tcpmsg_t *tcpmsg,
		       isc_task_t *task, isc_taskaction_t action, void *arg)
{
	isc_result_t result;
	isc_region_t region;

	REQUIRE(VALID_TCPMSG(tcpmsg));
	REQUIRE(task != NULL);
	REQUIRE(tcpmsg->task == NULL);  /* not currently in use */

	if (tcpmsg->buffer.base != NULL) {
		isc_mem_put(tcpmsg->mctx, tcpmsg->buffer.base,
			    tcpmsg->buffer.length);
		tcpmsg->buffer.base = NULL;
		tcpmsg->buffer.length = 0;
	}

	tcpmsg->task = task;
	tcpmsg->action = action;
	tcpmsg->arg = arg;
	tcpmsg->result = ISC_R_UNEXPECTED;  /* unknown right now */

	ISC_EVENT_INIT(&tcpmsg->event, sizeof(isc_event_t), 0, 0,
		       DNS_EVENT_TCPMSG, action, arg, tcpmsg,
		       NULL, NULL);

	region.base = (unsigned char *)&tcpmsg->size;
	region.length = 2;  /* isc_uint16_t */
	result = isc_socket_recv(tcpmsg->sock, &region, 0,
				 tcpmsg->task, recv_length, tcpmsg);

	if (result != ISC_R_SUCCESS)
		tcpmsg->task = NULL;

	return (result);
}

void
dns_tcpmsg_cancelread(dns_tcpmsg_t *tcpmsg) {
	REQUIRE(VALID_TCPMSG(tcpmsg));

	isc_socket_cancel(tcpmsg->sock, NULL, ISC_SOCKCANCEL_RECV);
}

void
dns_tcpmsg_keepbuffer(dns_tcpmsg_t *tcpmsg, isc_buffer_t *buffer) {
	REQUIRE(VALID_TCPMSG(tcpmsg));
	REQUIRE(buffer != NULL);

	*buffer = tcpmsg->buffer;
	tcpmsg->buffer.base = NULL;
	tcpmsg->buffer.length = 0;
}

#if 0
void
dns_tcpmsg_freebuffer(dns_tcpmsg_t *tcpmsg) {
	REQUIRE(VALID_TCPMSG(tcpmsg));

	if (tcpmsg->buffer.base == NULL)
		return;

	isc_mem_put(tcpmsg->mctx, tcpmsg->buffer.base, tcpmsg->buffer.length);
	tcpmsg->buffer.base = NULL;
	tcpmsg->buffer.length = 0;
}
#endif

void
dns_tcpmsg_invalidate(dns_tcpmsg_t *tcpmsg) {
	REQUIRE(VALID_TCPMSG(tcpmsg));

	tcpmsg->magic = 0;

	if (tcpmsg->buffer.base != NULL) {
		isc_mem_put(tcpmsg->mctx, tcpmsg->buffer.base,
			    tcpmsg->buffer.length);
		tcpmsg->buffer.base = NULL;
		tcpmsg->buffer.length = 0;
	}
}
