/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001  Internet Software Consortium.
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

/* $Id: lwres_noop.c,v 1.19 2007/06/19 23:47:22 tbox Exp $ */

/*! \file */

/**
 *    These are low-level routines for creating and parsing lightweight
 *    resolver no-op request and response messages.
 * 
 *    The no-op message is analogous to a ping packet: a packet is sent to
 *    the resolver daemon and is simply echoed back. The opcode is intended
 *    to allow a client to determine if the server is operational or not.
 * 
 *    There are four main functions for the no-op opcode. One render
 *    function converts a no-op request structure -- lwres_nooprequest_t --
 *    to the lighweight resolver's canonical format. It is complemented by a
 *    parse function that converts a packet in this canonical format to a
 *    no-op request structure. Another render function converts the no-op
 *    response structure -- lwres_noopresponse_t to the canonical format.
 *    This is complemented by a parse function which converts a packet in
 *    canonical format to a no-op response structure.
 * 
 *    These structures are defined in \link lwres.h <lwres/lwres.h.> \endlink They are shown below.
 * 
 * \code
 * #define LWRES_OPCODE_NOOP       0x00000000U
 * 
 * typedef struct {
 *         lwres_uint16_t  datalength;
 *         unsigned char   *data;
 * } lwres_nooprequest_t;
 * 
 * typedef struct {
 *         lwres_uint16_t  datalength;
 *         unsigned char   *data;
 * } lwres_noopresponse_t;
 * \endcode
 * 
 *    Although the structures have different types, they are identical. This
 *    is because the no-op opcode simply echos whatever data was sent: the
 *    response is therefore identical to the request.
 * 
 *    lwres_nooprequest_render() uses resolver context ctx to convert no-op
 *    request structure req to canonical format. The packet header structure
 *    pkt is initialised and transferred to buffer b. The contents of *req
 *    are then appended to the buffer in canonical format.
 *    lwres_noopresponse_render() performs the same task, except it converts
 *    a no-op response structure lwres_noopresponse_t to the lightweight
 *    resolver's canonical format.
 * 
 *    lwres_nooprequest_parse() uses context ctx to convert the contents of
 *    packet pkt to a lwres_nooprequest_t structure. Buffer b provides space
 *    to be used for storing this structure. When the function succeeds, the
 *    resulting lwres_nooprequest_t is made available through *structp.
 *    lwres_noopresponse_parse() offers the same semantics as
 *    lwres_nooprequest_parse() except it yields a lwres_noopresponse_t
 *    structure.
 * 
 *    lwres_noopresponse_free() and lwres_nooprequest_free() release the
 *    memory in resolver context ctx that was allocated to the
 *    lwres_noopresponse_t or lwres_nooprequest_t structures referenced via
 *    structp.
 * 
 * \section lwres_noop_return Return Values
 * 
 *    The no-op opcode functions lwres_nooprequest_render(),
 *    lwres_noopresponse_render() lwres_nooprequest_parse() and
 *    lwres_noopresponse_parse() all return #LWRES_R_SUCCESS on success. They
 *    return #LWRES_R_NOMEMORY if memory allocation fails.
 *    #LWRES_R_UNEXPECTEDEND is returned if the available space in the buffer
 *    b is too small to accommodate the packet header or the
 *    lwres_nooprequest_t and lwres_noopresponse_t structures.
 *    lwres_nooprequest_parse() and lwres_noopresponse_parse() will return
 *    #LWRES_R_UNEXPECTEDEND if the buffer is not empty after decoding the
 *    received packet. These functions will return #LWRES_R_FAILURE if
 *    pktflags in the packet header structure #lwres_lwpacket_t indicate that
 *    the packet is not a response to an earlier query.
 * 
 * \section lwres_noop_see See Also
 * 
 *    lwpacket.c
 */

#include <config.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <lwres/lwbuffer.h>
#include <lwres/lwpacket.h>
#include <lwres/lwres.h>
#include <lwres/result.h>

#include "context_p.h"
#include "assert_p.h"

/*% Uses resolver context ctx to convert no-op request structure req to canonical format. */
lwres_result_t
lwres_nooprequest_render(lwres_context_t *ctx, lwres_nooprequest_t *req,
			 lwres_lwpacket_t *pkt, lwres_buffer_t *b)
{
	unsigned char *buf;
	size_t buflen;
	int ret;
	size_t payload_length;

	REQUIRE(ctx != NULL);
	REQUIRE(req != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);

	payload_length = sizeof(lwres_uint16_t) + req->datalength;

	buflen = LWRES_LWPACKET_LENGTH + payload_length;
	buf = CTXMALLOC(buflen);
	if (buf == NULL)
		return (LWRES_R_NOMEMORY);
	lwres_buffer_init(b, buf, buflen);

	pkt->length = buflen;
	pkt->version = LWRES_LWPACKETVERSION_0;
	pkt->pktflags &= ~LWRES_LWPACKETFLAG_RESPONSE;
	pkt->opcode = LWRES_OPCODE_NOOP;
	pkt->result = 0;
	pkt->authtype = 0;
	pkt->authlength = 0;

	ret = lwres_lwpacket_renderheader(b, pkt);
	if (ret != LWRES_R_SUCCESS) {
		lwres_buffer_invalidate(b);
		CTXFREE(buf, buflen);
		return (ret);
	}

	INSIST(SPACE_OK(b, payload_length));

	/*
	 * Put the length and the data.  We know this will fit because we
	 * just checked for it.
	 */
	lwres_buffer_putuint16(b, req->datalength);
	lwres_buffer_putmem(b, req->data, req->datalength);

	INSIST(LWRES_BUFFER_AVAILABLECOUNT(b) == 0);

	return (LWRES_R_SUCCESS);
}

/*% Converts a no-op response structure lwres_noopresponse_t to the lightweight resolver's canonical format. */

lwres_result_t
lwres_noopresponse_render(lwres_context_t *ctx, lwres_noopresponse_t *req,
			  lwres_lwpacket_t *pkt, lwres_buffer_t *b)
{
	unsigned char *buf;
	size_t buflen;
	int ret;
	size_t payload_length;

	REQUIRE(ctx != NULL);
	REQUIRE(req != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);

	payload_length = sizeof(lwres_uint16_t) + req->datalength;

	buflen = LWRES_LWPACKET_LENGTH + payload_length;
	buf = CTXMALLOC(buflen);
	if (buf == NULL)
		return (LWRES_R_NOMEMORY);
	lwres_buffer_init(b, buf, buflen);

	pkt->length = buflen;
	pkt->version = LWRES_LWPACKETVERSION_0;
	pkt->pktflags |= LWRES_LWPACKETFLAG_RESPONSE;
	pkt->opcode = LWRES_OPCODE_NOOP;
	pkt->authtype = 0;
	pkt->authlength = 0;

	ret = lwres_lwpacket_renderheader(b, pkt);
	if (ret != LWRES_R_SUCCESS) {
		lwres_buffer_invalidate(b);
		CTXFREE(buf, buflen);
		return (ret);
	}

	INSIST(SPACE_OK(b, payload_length));

	/*
	 * Put the length and the data.  We know this will fit because we
	 * just checked for it.
	 */
	lwres_buffer_putuint16(b, req->datalength);
	lwres_buffer_putmem(b, req->data, req->datalength);

	INSIST(LWRES_BUFFER_AVAILABLECOUNT(b) == 0);

	return (LWRES_R_SUCCESS);
}

/*% Uses context ctx to convert the contents of packet pkt to a lwres_nooprequest_t structure. */
lwres_result_t
lwres_nooprequest_parse(lwres_context_t *ctx, lwres_buffer_t *b,
			lwres_lwpacket_t *pkt, lwres_nooprequest_t **structp)
{
	int ret;
	lwres_nooprequest_t *req;

	REQUIRE(ctx != NULL);
	REQUIRE(b != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	if ((pkt->pktflags & LWRES_LWPACKETFLAG_RESPONSE) != 0)
		return (LWRES_R_FAILURE);

	req = CTXMALLOC(sizeof(lwres_nooprequest_t));
	if (req == NULL)
		return (LWRES_R_NOMEMORY);

	if (!SPACE_REMAINING(b, sizeof(lwres_uint16_t))) {
		ret = LWRES_R_UNEXPECTEDEND;
		goto out;
	}
	req->datalength = lwres_buffer_getuint16(b);

	if (!SPACE_REMAINING(b, req->datalength)) {
		ret = LWRES_R_UNEXPECTEDEND;
		goto out;
	}
	req->data = b->base + b->current;
	lwres_buffer_forward(b, req->datalength);

	if (LWRES_BUFFER_REMAINING(b) != 0) {
		ret = LWRES_R_TRAILINGDATA;
		goto out;
	}

	/* success! */
	*structp = req;
	return (LWRES_R_SUCCESS);

	/* Error return */
 out:
	CTXFREE(req, sizeof(lwres_nooprequest_t));
	return (ret);
}

/*% Offers the same semantics as lwres_nooprequest_parse() except it yields a lwres_noopresponse_t structure. */
lwres_result_t
lwres_noopresponse_parse(lwres_context_t *ctx, lwres_buffer_t *b,
			 lwres_lwpacket_t *pkt, lwres_noopresponse_t **structp)
{
	int ret;
	lwres_noopresponse_t *req;

	REQUIRE(ctx != NULL);
	REQUIRE(b != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	if ((pkt->pktflags & LWRES_LWPACKETFLAG_RESPONSE) == 0)
		return (LWRES_R_FAILURE);

	req = CTXMALLOC(sizeof(lwres_noopresponse_t));
	if (req == NULL)
		return (LWRES_R_NOMEMORY);

	if (!SPACE_REMAINING(b, sizeof(lwres_uint16_t))) {
		ret = LWRES_R_UNEXPECTEDEND;
		goto out;
	}
	req->datalength = lwres_buffer_getuint16(b);

	if (!SPACE_REMAINING(b, req->datalength)) {
		ret = LWRES_R_UNEXPECTEDEND;
		goto out;
	}
	req->data = b->base + b->current;

	lwres_buffer_forward(b, req->datalength);
	if (LWRES_BUFFER_REMAINING(b) != 0) {
		ret = LWRES_R_TRAILINGDATA;
		goto out;
	}

	/* success! */
	*structp = req;
	return (LWRES_R_SUCCESS);

	/* Error return */
 out:
	CTXFREE(req, sizeof(lwres_noopresponse_t));
	return (ret);
}

/*% Release the memory in resolver context ctx. */
void
lwres_noopresponse_free(lwres_context_t *ctx, lwres_noopresponse_t **structp)
{
	lwres_noopresponse_t *noop;

	REQUIRE(ctx != NULL);
	REQUIRE(structp != NULL && *structp != NULL);

	noop = *structp;
	*structp = NULL;

	CTXFREE(noop, sizeof(lwres_noopresponse_t));
}

/*% Release the memory in resolver context ctx. */
void
lwres_nooprequest_free(lwres_context_t *ctx, lwres_nooprequest_t **structp)
{
	lwres_nooprequest_t *noop;

	REQUIRE(ctx != NULL);
	REQUIRE(structp != NULL && *structp != NULL);

	noop = *structp;
	*structp = NULL;

	CTXFREE(noop, sizeof(lwres_nooprequest_t));
}
