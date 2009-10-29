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

/* $Id: lwres_grbn.c,v 1.10 2007/06/19 23:47:22 tbox Exp $ */

/*! \file lwres_grbn.c

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

/*% Thread-save equivalent to \link lwres_gabn.c lwres_gabn* \endlink routines. */
lwres_result_t
lwres_grbnrequest_render(lwres_context_t *ctx, lwres_grbnrequest_t *req,
			 lwres_lwpacket_t *pkt, lwres_buffer_t *b)
{
	unsigned char *buf;
	size_t buflen;
	int ret;
	size_t payload_length;
	lwres_uint16_t datalen;

	REQUIRE(ctx != NULL);
	REQUIRE(req != NULL);
	REQUIRE(req->name != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);

	datalen = strlen(req->name);

	payload_length = 4 + 2 + 2 + 2 + req->namelen + 1;

	buflen = LWRES_LWPACKET_LENGTH + payload_length;
	buf = CTXMALLOC(buflen);
	if (buf == NULL)
		return (LWRES_R_NOMEMORY);

	lwres_buffer_init(b, buf, buflen);

	pkt->length = buflen;
	pkt->version = LWRES_LWPACKETVERSION_0;
	pkt->pktflags &= ~LWRES_LWPACKETFLAG_RESPONSE;
	pkt->opcode = LWRES_OPCODE_GETRDATABYNAME;
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
	 * Flags.
	 */
	lwres_buffer_putuint32(b, req->flags);

	/*
	 * Class.
	 */
	lwres_buffer_putuint16(b, req->rdclass);

	/*
	 * Type.
	 */
	lwres_buffer_putuint16(b, req->rdtype);

	/*
	 * Put the length and the data.  We know this will fit because we
	 * just checked for it.
	 */
	lwres_buffer_putuint16(b, datalen);
	lwres_buffer_putmem(b, (unsigned char *)req->name, datalen);
	lwres_buffer_putuint8(b, 0); /* trailing NUL */

	INSIST(LWRES_BUFFER_AVAILABLECOUNT(b) == 0);

	return (LWRES_R_SUCCESS);
}

/*% Thread-save equivalent to \link lwres_gabn.c lwres_gabn* \endlink routines. */
lwres_result_t
lwres_grbnresponse_render(lwres_context_t *ctx, lwres_grbnresponse_t *req,
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

	/* flags, class, type, ttl, nrdatas, nsigs */
	payload_length = 4 + 2 + 2 + 4 + 2 + 2;
	/* real name encoding */
	payload_length += 2 + req->realnamelen + 1;
	/* each rr */
	for (x = 0; x < req->nrdatas; x++)
		payload_length += 2 + req->rdatalen[x];
	for (x = 0; x < req->nsigs; x++)
		payload_length += 2 + req->siglen[x];

	buflen = LWRES_LWPACKET_LENGTH + payload_length;
	buf = CTXMALLOC(buflen);
	if (buf == NULL)
		return (LWRES_R_NOMEMORY);
	lwres_buffer_init(b, buf, buflen);

	pkt->length = buflen;
	pkt->version = LWRES_LWPACKETVERSION_0;
	pkt->pktflags |= LWRES_LWPACKETFLAG_RESPONSE;
	pkt->opcode = LWRES_OPCODE_GETRDATABYNAME;
	pkt->authtype = 0;
	pkt->authlength = 0;

	ret = lwres_lwpacket_renderheader(b, pkt);
	if (ret != LWRES_R_SUCCESS) {
		lwres_buffer_invalidate(b);
		CTXFREE(buf, buflen);
		return (ret);
	}

	/*
	 * Check space needed here.
	 */
	INSIST(SPACE_OK(b, payload_length));

	/* Flags. */
	lwres_buffer_putuint32(b, req->flags);

	/* encode class, type, ttl, and nrdatas */
	lwres_buffer_putuint16(b, req->rdclass);
	lwres_buffer_putuint16(b, req->rdtype);
	lwres_buffer_putuint32(b, req->ttl);
	lwres_buffer_putuint16(b, req->nrdatas);
	lwres_buffer_putuint16(b, req->nsigs);

	/* encode the real name */
	datalen = req->realnamelen;
	lwres_buffer_putuint16(b, datalen);
	lwres_buffer_putmem(b, (unsigned char *)req->realname, datalen);
	lwres_buffer_putuint8(b, 0);

	/* encode the rdatas */
	for (x = 0; x < req->nrdatas; x++) {
		datalen = req->rdatalen[x];
		lwres_buffer_putuint16(b, datalen);
		lwres_buffer_putmem(b, req->rdatas[x], datalen);
	}

	/* encode the signatures */
	for (x = 0; x < req->nsigs; x++) {
		datalen = req->siglen[x];
		lwres_buffer_putuint16(b, datalen);
		lwres_buffer_putmem(b, req->sigs[x], datalen);
	}

	INSIST(LWRES_BUFFER_AVAILABLECOUNT(b) == 0);
	INSIST(LWRES_BUFFER_USEDCOUNT(b) == pkt->length);

	return (LWRES_R_SUCCESS);
}

/*% Thread-save equivalent to \link lwres_gabn.c lwres_gabn* \endlink routines. */
lwres_result_t
lwres_grbnrequest_parse(lwres_context_t *ctx, lwres_buffer_t *b,
			lwres_lwpacket_t *pkt, lwres_grbnrequest_t **structp)
{
	int ret;
	char *name;
	lwres_grbnrequest_t *grbn;
	lwres_uint32_t flags;
	lwres_uint16_t rdclass, rdtype;
	lwres_uint16_t namelen;

	REQUIRE(ctx != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	if ((pkt->pktflags & LWRES_LWPACKETFLAG_RESPONSE) != 0)
		return (LWRES_R_FAILURE);

	if (!SPACE_REMAINING(b, 4 + 2 + 2))
		return (LWRES_R_UNEXPECTEDEND);

	/*
	 * Pull off the flags, class, and type.
	 */
	flags = lwres_buffer_getuint32(b);
	rdclass = lwres_buffer_getuint16(b);
	rdtype = lwres_buffer_getuint16(b);

	/*
	 * Pull off the name itself
	 */
	ret = lwres_string_parse(b, &name, &namelen);
	if (ret != LWRES_R_SUCCESS)
		return (ret);

	if (LWRES_BUFFER_REMAINING(b) != 0)
		return (LWRES_R_TRAILINGDATA);

	grbn = CTXMALLOC(sizeof(lwres_grbnrequest_t));
	if (grbn == NULL)
		return (LWRES_R_NOMEMORY);

	grbn->flags = flags;
	grbn->rdclass = rdclass;
	grbn->rdtype = rdtype;
	grbn->name = name;
	grbn->namelen = namelen;

	*structp = grbn;
	return (LWRES_R_SUCCESS);
}

/*% Thread-save equivalent to \link lwres_gabn.c lwres_gabn* \endlink routines. */
lwres_result_t
lwres_grbnresponse_parse(lwres_context_t *ctx, lwres_buffer_t *b,
			lwres_lwpacket_t *pkt, lwres_grbnresponse_t **structp)
{
	lwres_result_t ret;
	unsigned int x;
	lwres_uint32_t flags;
	lwres_uint16_t rdclass, rdtype;
	lwres_uint32_t ttl;
	lwres_uint16_t nrdatas, nsigs;
	lwres_grbnresponse_t *grbn;

	REQUIRE(ctx != NULL);
	REQUIRE(pkt != NULL);
	REQUIRE(b != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	grbn = NULL;

	if ((pkt->pktflags & LWRES_LWPACKETFLAG_RESPONSE) == 0)
		return (LWRES_R_FAILURE);

	/*
	 * Pull off the flags, class, type, ttl, nrdatas, and nsigs
	 */
	if (!SPACE_REMAINING(b, 4 + 2 + 2 + 4 + 2 + 2))
		return (LWRES_R_UNEXPECTEDEND);
	flags = lwres_buffer_getuint32(b);
	rdclass = lwres_buffer_getuint16(b);
	rdtype = lwres_buffer_getuint16(b);
	ttl = lwres_buffer_getuint32(b);
	nrdatas = lwres_buffer_getuint16(b);
	nsigs = lwres_buffer_getuint16(b);

	/*
	 * Pull off the name itself
	 */

	grbn = CTXMALLOC(sizeof(lwres_grbnresponse_t));
	if (grbn == NULL)
		return (LWRES_R_NOMEMORY);
	grbn->rdatas = NULL;
	grbn->rdatalen = NULL;
	grbn->sigs = NULL;
	grbn->siglen = NULL;
	grbn->base = NULL;

	grbn->flags = flags;
	grbn->rdclass = rdclass;
	grbn->rdtype = rdtype;
	grbn->ttl = ttl;
	grbn->nrdatas = nrdatas;
	grbn->nsigs = nsigs;

	if (nrdatas > 0) {
		grbn->rdatas = CTXMALLOC(sizeof(char *) * nrdatas);
		if (grbn->rdatas == NULL) {
			ret = LWRES_R_NOMEMORY;
			goto out;
		}

		grbn->rdatalen = CTXMALLOC(sizeof(lwres_uint16_t) * nrdatas);
		if (grbn->rdatalen == NULL) {
			ret = LWRES_R_NOMEMORY;
			goto out;
		}
	}

	if (nsigs > 0) {
		grbn->sigs = CTXMALLOC(sizeof(char *) * nsigs);
		if (grbn->sigs == NULL) {
			ret = LWRES_R_NOMEMORY;
			goto out;
		}

		grbn->siglen = CTXMALLOC(sizeof(lwres_uint16_t) * nsigs);
		if (grbn->siglen == NULL) {
			ret = LWRES_R_NOMEMORY;
			goto out;
		}
	}

	/*
	 * Now, pull off the real name.
	 */
	ret = lwres_string_parse(b, &grbn->realname, &grbn->realnamelen);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	/*
	 * Parse off the rdatas.
	 */
	for (x = 0; x < grbn->nrdatas; x++) {
		ret = lwres_data_parse(b, &grbn->rdatas[x],
					 &grbn->rdatalen[x]);
		if (ret != LWRES_R_SUCCESS)
			goto out;
	}

	/*
	 * Parse off the signatures.
	 */
	for (x = 0; x < grbn->nsigs; x++) {
		ret = lwres_data_parse(b, &grbn->sigs[x], &grbn->siglen[x]);
		if (ret != LWRES_R_SUCCESS)
			goto out;
	}

	if (LWRES_BUFFER_REMAINING(b) != 0) {
		ret = LWRES_R_TRAILINGDATA;
		goto out;
	}

	*structp = grbn;
	return (LWRES_R_SUCCESS);

 out:
	if (grbn != NULL) {
		if (grbn->rdatas != NULL)
			CTXFREE(grbn->rdatas, sizeof(char *) * nrdatas);
		if (grbn->rdatalen != NULL)
			CTXFREE(grbn->rdatalen,
				sizeof(lwres_uint16_t) * nrdatas);
		if (grbn->sigs != NULL)
			CTXFREE(grbn->sigs, sizeof(char *) * nsigs);
		if (grbn->siglen != NULL)
			CTXFREE(grbn->siglen, sizeof(lwres_uint16_t) * nsigs);
		CTXFREE(grbn, sizeof(lwres_grbnresponse_t));
	}

	return (ret);
}

/*% Thread-save equivalent to \link lwres_gabn.c lwres_gabn* \endlink routines. */
void
lwres_grbnrequest_free(lwres_context_t *ctx, lwres_grbnrequest_t **structp)
{
	lwres_grbnrequest_t *grbn;

	REQUIRE(ctx != NULL);
	REQUIRE(structp != NULL && *structp != NULL);

	grbn = *structp;
	*structp = NULL;

	CTXFREE(grbn, sizeof(lwres_grbnrequest_t));
}

/*% Thread-save equivalent to \link lwres_gabn.c lwres_gabn* \endlink routines. */
void
lwres_grbnresponse_free(lwres_context_t *ctx, lwres_grbnresponse_t **structp)
{
	lwres_grbnresponse_t *grbn;

	REQUIRE(ctx != NULL);
	REQUIRE(structp != NULL && *structp != NULL);

	grbn = *structp;
	*structp = NULL;

	if (grbn->nrdatas > 0) {
		CTXFREE(grbn->rdatas, sizeof(char *) * grbn->nrdatas);
		CTXFREE(grbn->rdatalen,
			sizeof(lwres_uint16_t) * grbn->nrdatas);
	}
	if (grbn->nsigs > 0) {
		CTXFREE(grbn->sigs, sizeof(char *) * grbn->nsigs);
		CTXFREE(grbn->siglen, sizeof(lwres_uint16_t) * grbn->nsigs);
	}
	if (grbn->base != NULL)
		CTXFREE(grbn->base, grbn->baselen);
	CTXFREE(grbn, sizeof(lwres_grbnresponse_t));
}
