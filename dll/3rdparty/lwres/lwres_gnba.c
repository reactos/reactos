/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2002  Internet Software Consortium.
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

/* $Id: lwres_gnba.c,v 1.28 2007/09/24 17:18:25 each Exp $ */

/*! \file lwres_gnba.c
   These are low-level routines for creating and parsing lightweight
   resolver address-to-name lookup request and response messages.

   There are four main functions for the getnamebyaddr opcode. One
 render function converts a getnamebyaddr request structure --
   lwres_gnbarequest_t -- to the lightweight resolver's canonical
   format. It is complemented by a parse function that converts a
   packet in this canonical format to a getnamebyaddr request
   structure. Another render function converts the getnamebyaddr
   response structure -- lwres_gnbaresponse_t to the canonical format.
   This is complemented by a parse function which converts a packet in
   canonical format to a getnamebyaddr response structure.       

   These structures are defined in \link lwres.h <lwres/lwres.h.>\endlink They are shown
   below.

\code
#define LWRES_OPCODE_GETNAMEBYADDR      0x00010002U

typedef struct {
        lwres_uint32_t  flags;
        lwres_addr_t    addr;
} lwres_gnbarequest_t;

typedef struct {
        lwres_uint32_t  flags;
        lwres_uint16_t  naliases;
        char           *realname;
        char          **aliases;
        lwres_uint16_t  realnamelen;
        lwres_uint16_t *aliaslen;
        void           *base;
        size_t          baselen;
} lwres_gnbaresponse_t;
\endcode

   lwres_gnbarequest_render() uses resolver context ctx to convert
   getnamebyaddr request structure req to canonical format. The packet
   header structure pkt is initialised and transferred to buffer b.
   The contents of *req are then appended to the buffer in canonical
   format. lwres_gnbaresponse_render() performs the same task, except
   it converts a getnamebyaddr response structure lwres_gnbaresponse_t
   to the lightweight resolver's canonical format.

   lwres_gnbarequest_parse() uses context ctx to convert the contents
   of packet pkt to a lwres_gnbarequest_t structure. Buffer b provides
   space to be used for storing this structure. When the function
   succeeds, the resulting lwres_gnbarequest_t is made available
   through *structp. lwres_gnbaresponse_parse() offers the same   
semantics as lwres_gnbarequest_parse() except it yields a    
   lwres_gnbaresponse_t structure.

   lwres_gnbaresponse_free() and lwres_gnbarequest_free() release the
   memory in resolver context ctx that was allocated to the     
   lwres_gnbaresponse_t or lwres_gnbarequest_t structures referenced  
   via structp. Any memory associated with ancillary buffers and      
   strings for those structures is also discarded.

\section lwres_gbna_return Return Values

   The getnamebyaddr opcode functions lwres_gnbarequest_render(),
   lwres_gnbaresponse_render() lwres_gnbarequest_parse() and
   lwres_gnbaresponse_parse() all return #LWRES_R_SUCCESS on success.
   They return #LWRES_R_NOMEMORY if memory allocation fails.
   #LWRES_R_UNEXPECTEDEND is returned if the available space in the
   buffer b is too small to accommodate the packet header or the
   lwres_gnbarequest_t and lwres_gnbaresponse_t structures.
   lwres_gnbarequest_parse() and lwres_gnbaresponse_parse() will
   return #LWRES_R_UNEXPECTEDEND if the buffer is not empty after
   decoding the received packet. These functions will return
   #LWRES_R_FAILURE if pktflags in the packet header structure
   #lwres_lwpacket_t indicate that the packet is not a response to an
   earlier query.

\section lwres_gbna_see See Also

   \link lwpacket.c lwres_packet\endlink

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

/*% Uses resolver context ctx to convert getnamebyaddr request structure req to canonical format. */
lwres_result_t
lwres_gnbarequest_render(lwres_context_t *ctx, lwres_gnbarequest_t *req,
			 lwres_lwpacket_t *pkt, lwres_buffer_t *b)
{
	unsigned char *buf;
	size_t buflen;
	int ret;
	size_t payload_length;

	REQUIRE(ctx != NULL);
	REQUIRE(req != NULL);
	REQUIRE(req->addr.family != 0);
	REQUIRE(req->addr.length != 0);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);

	payload_length = 4 + 4 + 2 + + req->addr.length;

	buflen = LWRES_LWPACKET_LENGTH + payload_length;
	buf = CTXMALLOC(buflen);
	if (buf == NULL)
		return (LWRES_R_NOMEMORY);
	lwres_buffer_init(b, buf, buflen);

	pkt->length = buflen;
	pkt->version = LWRES_LWPACKETVERSION_0;
	pkt->pktflags &= ~LWRES_LWPACKETFLAG_RESPONSE;
	pkt->opcode = LWRES_OPCODE_GETNAMEBYADDR;
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
	lwres_buffer_putuint32(b, req->flags);
	lwres_buffer_putuint32(b, req->addr.family);
	lwres_buffer_putuint16(b, req->addr.length);
	lwres_buffer_putmem(b, (unsigned char *)req->addr.address,
			    req->addr.length);

	INSIST(LWRES_BUFFER_AVAILABLECOUNT(b) == 0);

	return (LWRES_R_SUCCESS);
}

/*% Converts a getnamebyaddr response structure lwres_gnbaresponse_t to the lightweight resolver's canonical format. */
lwres_result_t
lwres_gnbaresponse_render(lwres_context_t *ctx, lwres_gnbaresponse_t *req,
			  lwres_lwpacket_t *pkt, lwres_buffer_t *b)
{
	unsigned char *buf;
	size_t buflen;
	int ret;
	size_t payload_length;
	lwres_uint16_t datalen;
	int x;

	REQUIRE(ctx != NULL);
	REQUIRE(req != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);

	/*
	 * Calculate packet size.
	 */
	payload_length = 4;			       /* flags */
	payload_length += 2;			       /* naliases */
	payload_length += 2 + req->realnamelen + 1;    /* real name encoding */
	for (x = 0; x < req->naliases; x++)	       /* each alias */
		payload_length += 2 + req->aliaslen[x] + 1;

	buflen = LWRES_LWPACKET_LENGTH + payload_length;
	buf = CTXMALLOC(buflen);
	if (buf == NULL)
		return (LWRES_R_NOMEMORY);
	lwres_buffer_init(b, buf, buflen);

	pkt->length = buflen;
	pkt->version = LWRES_LWPACKETVERSION_0;
	pkt->pktflags |= LWRES_LWPACKETFLAG_RESPONSE;
	pkt->opcode = LWRES_OPCODE_GETNAMEBYADDR;
	pkt->authtype = 0;
	pkt->authlength = 0;

	ret = lwres_lwpacket_renderheader(b, pkt);
	if (ret != LWRES_R_SUCCESS) {
		lwres_buffer_invalidate(b);
		CTXFREE(buf, buflen);
		return (ret);
	}

	INSIST(SPACE_OK(b, payload_length));
	lwres_buffer_putuint32(b, req->flags);

	/* encode naliases */
	lwres_buffer_putuint16(b, req->naliases);

	/* encode the real name */
	datalen = req->realnamelen;
	lwres_buffer_putuint16(b, datalen);
	lwres_buffer_putmem(b, (unsigned char *)req->realname, datalen);
	lwres_buffer_putuint8(b, 0);

	/* encode the aliases */
	for (x = 0; x < req->naliases; x++) {
		datalen = req->aliaslen[x];
		lwres_buffer_putuint16(b, datalen);
		lwres_buffer_putmem(b, (unsigned char *)req->aliases[x],
				    datalen);
		lwres_buffer_putuint8(b, 0);
	}

	INSIST(LWRES_BUFFER_AVAILABLECOUNT(b) == 0);

	return (LWRES_R_SUCCESS);
}

/*% Uses context ctx to convert the contents of packet pkt to a lwres_gnbarequest_t structure. */
lwres_result_t
lwres_gnbarequest_parse(lwres_context_t *ctx, lwres_buffer_t *b,
			lwres_lwpacket_t *pkt, lwres_gnbarequest_t **structp)
{
	int ret;
	lwres_gnbarequest_t *gnba;

	REQUIRE(ctx != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	if ((pkt->pktflags & LWRES_LWPACKETFLAG_RESPONSE) != 0)
		return (LWRES_R_FAILURE);

	if (!SPACE_REMAINING(b, 4))
		return (LWRES_R_UNEXPECTEDEND);

	gnba = CTXMALLOC(sizeof(lwres_gnbarequest_t));
	if (gnba == NULL)
		return (LWRES_R_NOMEMORY);

	gnba->flags = lwres_buffer_getuint32(b);

	ret = lwres_addr_parse(b, &gnba->addr);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	if (LWRES_BUFFER_REMAINING(b) != 0) {
		ret = LWRES_R_TRAILINGDATA;
		goto out;
	}

	*structp = gnba;
	return (LWRES_R_SUCCESS);

 out:
	if (gnba != NULL)
		lwres_gnbarequest_free(ctx, &gnba);

	return (ret);
}

/*% Offers the same semantics as lwres_gnbarequest_parse() except it yields a lwres_gnbaresponse_t structure. */

lwres_result_t
lwres_gnbaresponse_parse(lwres_context_t *ctx, lwres_buffer_t *b,
			 lwres_lwpacket_t *pkt, lwres_gnbaresponse_t **structp)
{
	int ret;
	unsigned int x;
	lwres_uint32_t flags;
	lwres_uint16_t naliases;
	lwres_gnbaresponse_t *gnba;

	REQUIRE(ctx != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	gnba = NULL;

	if ((pkt->pktflags & LWRES_LWPACKETFLAG_RESPONSE) == 0)
		return (LWRES_R_FAILURE);

	/*
	 * Pull off flags & naliases
	 */
	if (!SPACE_REMAINING(b, 4 + 2))
		return (LWRES_R_UNEXPECTEDEND);
	flags = lwres_buffer_getuint32(b);
	naliases = lwres_buffer_getuint16(b);

	gnba = CTXMALLOC(sizeof(lwres_gnbaresponse_t));
	if (gnba == NULL)
		return (LWRES_R_NOMEMORY);
	gnba->base = NULL;
	gnba->aliases = NULL;
	gnba->aliaslen = NULL;

	gnba->flags = flags;
	gnba->naliases = naliases;

	if (naliases > 0) {
		gnba->aliases = CTXMALLOC(sizeof(char *) * naliases);
		if (gnba->aliases == NULL) {
			ret = LWRES_R_NOMEMORY;
			goto out;
		}

		gnba->aliaslen = CTXMALLOC(sizeof(lwres_uint16_t) * naliases);
		if (gnba->aliaslen == NULL) {
			ret = LWRES_R_NOMEMORY;
			goto out;
		}
	}

	/*
	 * Now, pull off the real name.
	 */
	ret = lwres_string_parse(b, &gnba->realname, &gnba->realnamelen);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	/*
	 * Parse off the aliases.
	 */
	for (x = 0; x < gnba->naliases; x++) {
		ret = lwres_string_parse(b, &gnba->aliases[x],
					 &gnba->aliaslen[x]);
		if (ret != LWRES_R_SUCCESS)
			goto out;
	}

	if (LWRES_BUFFER_REMAINING(b) != 0) {
		ret = LWRES_R_TRAILINGDATA;
		goto out;
	}

	*structp = gnba;
	return (LWRES_R_SUCCESS);

 out:
	if (gnba != NULL) {
		if (gnba->aliases != NULL)
			CTXFREE(gnba->aliases, sizeof(char *) * naliases);
		if (gnba->aliaslen != NULL)
			CTXFREE(gnba->aliaslen,
				sizeof(lwres_uint16_t) * naliases);
		CTXFREE(gnba, sizeof(lwres_gnbaresponse_t));
	}

	return (ret);
}

/*% Release the memory in resolver context ctx that was allocated to the lwres_gnbarequest_t. */
void
lwres_gnbarequest_free(lwres_context_t *ctx, lwres_gnbarequest_t **structp)
{
	lwres_gnbarequest_t *gnba;

	REQUIRE(ctx != NULL);
	REQUIRE(structp != NULL && *structp != NULL);

	gnba = *structp;
	*structp = NULL;

	CTXFREE(gnba, sizeof(lwres_gnbarequest_t));
}

/*% Release the memory in resolver context ctx that was allocated to the lwres_gnbaresponse_t. */
void
lwres_gnbaresponse_free(lwres_context_t *ctx, lwres_gnbaresponse_t **structp)
{
	lwres_gnbaresponse_t *gnba;

	REQUIRE(ctx != NULL);
	REQUIRE(structp != NULL && *structp != NULL);

	gnba = *structp;
	*structp = NULL;

	if (gnba->naliases > 0) {
		CTXFREE(gnba->aliases, sizeof(char *) * gnba->naliases);
		CTXFREE(gnba->aliaslen,
			sizeof(lwres_uint16_t) * gnba->naliases);
	}
	if (gnba->base != NULL)
		CTXFREE(gnba->base, gnba->baselen);
	CTXFREE(gnba, sizeof(lwres_gnbaresponse_t));
}
