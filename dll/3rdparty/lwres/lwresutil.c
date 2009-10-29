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

/* $Id: lwresutil.c,v 1.34 2007/06/19 23:47:22 tbox Exp $ */

/*! \file */

/**
 *    lwres_string_parse() retrieves a DNS-encoded string starting the
 *    current pointer of lightweight resolver buffer b: i.e. b->current.
 *    When the function returns, the address of the first byte of the
 *    encoded string is returned via *c and the length of that string is
 *    given by *len. The buffer's current pointer is advanced to point at
 *    the character following the string length, the encoded string, and
 *    the trailing NULL character.
 * 
 *    lwres_addr_parse() extracts an address from the buffer b. The
 *    buffer's current pointer b->current is presumed to point at an
 *    encoded address: the address preceded by a 32-bit protocol family
 *    identifier and a 16-bit length field. The encoded address is copied
 *    to addr->address and addr->length indicates the size in bytes of
 *    the address that was copied. b->current is advanced to point at the
 *  next byte of available data in the buffer following the encoded
 *    address.
 * 
 *    lwres_getaddrsbyname() and lwres_getnamebyaddr() use the
 *    lwres_gnbaresponse_t structure defined below:
 * 
 * \code
 * typedef struct {
 *         lwres_uint32_t          flags;
 *         lwres_uint16_t          naliases;
 *         lwres_uint16_t          naddrs;
 *         char                   *realname;
 *         char                  **aliases;
 *         lwres_uint16_t          realnamelen;
 *         lwres_uint16_t         *aliaslen;
 *         lwres_addrlist_t        addrs;
 *         void                   *base;
 *         size_t                  baselen;
 * } lwres_gabnresponse_t;
 * \endcode
 * 
 *    The contents of this structure are not manipulated directly but
 *    they are controlled through the \link lwres_gabn.c lwres_gabn*\endlink functions.  
 * 
 *    The lightweight resolver uses lwres_getaddrsbyname() to perform
 *    foward lookups. Hostname name is looked up using the resolver
 *    context ctx for memory allocation. addrtypes is a bitmask      
 *    indicating which type of addresses are to be looked up. Current
 *    values for this bitmask are #LWRES_ADDRTYPE_V4 for IPv4 addresses
 *    and #LWRES_ADDRTYPE_V6 for IPv6 addresses. Results of the lookup are
 *    returned in *structp.
 * 
 *    lwres_getnamebyaddr() performs reverse lookups. Resolver context  
 *    ctx is used for memory allocation. The address type is indicated by
 *    addrtype: #LWRES_ADDRTYPE_V4 or #LWRES_ADDRTYPE_V6. The address to be
 *    looked up is given by addr and its length is addrlen bytes. The    
 *    result of the function call is made available through *structp.   
 * 
 * \section lwresutil_return Return Values
 * 
 *    Successful calls to lwres_string_parse() and lwres_addr_parse()
 *    return #LWRES_R_SUCCESS. Both functions return #LWRES_R_FAILURE if 
 *    the buffer is corrupt or #LWRES_R_UNEXPECTEDEND if the buffer has   
 *    less space than expected for the components of the encoded string
 *    or address.
 * 
 * lwres_getaddrsbyname() returns #LWRES_R_SUCCESS on success and it
 *    returns #LWRES_R_NOTFOUND if the hostname name could not be found.
 * 
 *    #LWRES_R_SUCCESS is returned by a successful call to
 *    lwres_getnamebyaddr().
 * 
 *    Both lwres_getaddrsbyname() and lwres_getnamebyaddr() return
 *    #LWRES_R_NOMEMORY when memory allocation requests fail and
 *    #LWRES_R_UNEXPECTEDEND if the buffers used for sending queries and
 *    receiving replies are too small.     
 * 
 * \section lwresutil_see See Also
 * 
 *    lwbuffer.c, lwres_gabn.c
 */

#include <config.h>

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <lwres/lwbuffer.h>
#include <lwres/lwres.h>
#include <lwres/result.h>

#include "assert_p.h"
#include "context_p.h"

/*% Parse data. */
/*!
 * Requires:
 *
 *	The "current" pointer in "b" points to encoded raw data.
 *
 * Ensures:
 *
 *	The address of the first byte of the data is returned via "p",
 *	and the length is returned via "len".  If NULL, they are not
 *	set.
 *
 *	On return, the current pointer of "b" will point to the character
 *	following the data length and the data.
 *
 */
lwres_result_t
lwres_data_parse(lwres_buffer_t *b, unsigned char **p, lwres_uint16_t *len)
{
	lwres_uint16_t datalen;
	unsigned char *data;

	REQUIRE(b != NULL);

	/*
	 * Pull off the length (2 bytes)
	 */
	if (!SPACE_REMAINING(b, 2))
		return (LWRES_R_UNEXPECTEDEND);
	datalen = lwres_buffer_getuint16(b);

	/*
	 * Set the pointer to this string to the right place, then
	 * advance the buffer pointer.
	 */
	if (!SPACE_REMAINING(b, datalen))
		return (LWRES_R_UNEXPECTEDEND);
	data = b->base + b->current;
	lwres_buffer_forward(b, datalen);

	if (len != NULL)
		*len = datalen;
	if (p != NULL)
		*p = data;

	return (LWRES_R_SUCCESS);
}

/*% Retrieves a DNS-encoded string. */
/*!
 * Requires:
 *
 *	The "current" pointer in "b" point to an encoded string.
 *
 * Ensures:
 *
 *	The address of the first byte of the string is returned via "c",
 *	and the length is returned via "len".  If NULL, they are not
 *	set.
 *
 *	On return, the current pointer of "b" will point to the character
 *	following the string length, the string, and the trailing NULL.
 *
 */
lwres_result_t
lwres_string_parse(lwres_buffer_t *b, char **c, lwres_uint16_t *len)
{
	lwres_uint16_t datalen;
	char *string;

	REQUIRE(b != NULL);

	/*
	 * Pull off the length (2 bytes)
	 */
	if (!SPACE_REMAINING(b, 2))
		return (LWRES_R_UNEXPECTEDEND);
	datalen = lwres_buffer_getuint16(b);

	/*
	 * Set the pointer to this string to the right place, then
	 * advance the buffer pointer.
	 */
	if (!SPACE_REMAINING(b, datalen))
		return (LWRES_R_UNEXPECTEDEND);
	string = (char *)b->base + b->current;
	lwres_buffer_forward(b, datalen);

	/*
	 * Skip the "must be zero" byte.
	 */
	if (!SPACE_REMAINING(b, 1))
		return (LWRES_R_UNEXPECTEDEND);
	if (0 != lwres_buffer_getuint8(b))
		return (LWRES_R_FAILURE);

	if (len != NULL)
		*len = datalen;
	if (c != NULL)
		*c = string;

	return (LWRES_R_SUCCESS);
}

/*% Extracts an address from the buffer b. */
lwres_result_t
lwres_addr_parse(lwres_buffer_t *b, lwres_addr_t *addr)
{
	REQUIRE(addr != NULL);

	if (!SPACE_REMAINING(b, 6))
		return (LWRES_R_UNEXPECTEDEND);

	addr->family = lwres_buffer_getuint32(b);
	addr->length = lwres_buffer_getuint16(b);

	if (!SPACE_REMAINING(b, addr->length))
		return (LWRES_R_UNEXPECTEDEND);
	if (addr->length > LWRES_ADDR_MAXLEN)
		return (LWRES_R_FAILURE);

	lwres_buffer_getmem(b, addr->address, addr->length);

	return (LWRES_R_SUCCESS);
}

/*% Used to perform forward lookups. */
lwres_result_t
lwres_getaddrsbyname(lwres_context_t *ctx, const char *name,
		     lwres_uint32_t addrtypes, lwres_gabnresponse_t **structp)
{
	lwres_gabnrequest_t request;
	lwres_gabnresponse_t *response;
	int ret;
	int recvlen;
	lwres_buffer_t b_in, b_out;
	lwres_lwpacket_t pkt;
	lwres_uint32_t serial;
	char *buffer;
	char target_name[1024];
	unsigned int target_length;

	REQUIRE(ctx != NULL);
	REQUIRE(name != NULL);
	REQUIRE(addrtypes != 0);
	REQUIRE(structp != NULL && *structp == NULL);

	b_in.base = NULL;
	b_out.base = NULL;
	response = NULL;
	buffer = NULL;
	serial = lwres_context_nextserial(ctx);

	buffer = CTXMALLOC(LWRES_RECVLENGTH);
	if (buffer == NULL) {
		ret = LWRES_R_NOMEMORY;
		goto out;
	}

	target_length = strlen(name);
	if (target_length >= sizeof(target_name))
		return (LWRES_R_FAILURE);
	strcpy(target_name, name); /* strcpy is safe */

	/*
	 * Set up our request and render it to a buffer.
	 */
	request.flags = 0;
	request.addrtypes = addrtypes;
	request.name = target_name;
	request.namelen = target_length;
	pkt.pktflags = 0;
	pkt.serial = serial;
	pkt.result = 0;
	pkt.recvlength = LWRES_RECVLENGTH;

 again:
	ret = lwres_gabnrequest_render(ctx, &request, &pkt, &b_out);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	ret = lwres_context_sendrecv(ctx, b_out.base, b_out.length, buffer,
				     LWRES_RECVLENGTH, &recvlen);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	lwres_buffer_init(&b_in, buffer, recvlen);
	b_in.used = recvlen;

	/*
	 * Parse the packet header.
	 */
	ret = lwres_lwpacket_parseheader(&b_in, &pkt);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	/*
	 * Sanity check.
	 */
	if (pkt.serial != serial)
		goto again;
	if (pkt.opcode != LWRES_OPCODE_GETADDRSBYNAME)
		goto again;

	/*
	 * Free what we've transmitted
	 */
	CTXFREE(b_out.base, b_out.length);
	b_out.base = NULL;
	b_out.length = 0;

	if (pkt.result != LWRES_R_SUCCESS) {
		ret = pkt.result;
		goto out;
	}

	/*
	 * Parse the response.
	 */
	ret = lwres_gabnresponse_parse(ctx, &b_in, &pkt, &response);
	if (ret != LWRES_R_SUCCESS)
		goto out;
	response->base = buffer;
	response->baselen = LWRES_RECVLENGTH;
	buffer = NULL; /* don't free this below */

	*structp = response;
	return (LWRES_R_SUCCESS);

 out:
	if (b_out.base != NULL)
		CTXFREE(b_out.base, b_out.length);
	if (buffer != NULL)
		CTXFREE(buffer, LWRES_RECVLENGTH);
	if (response != NULL)
		lwres_gabnresponse_free(ctx, &response);

	return (ret);
}


/*% Used to perform reverse lookups. */
lwres_result_t
lwres_getnamebyaddr(lwres_context_t *ctx, lwres_uint32_t addrtype,
		    lwres_uint16_t addrlen, const unsigned char *addr,
		    lwres_gnbaresponse_t **structp)
{
	lwres_gnbarequest_t request;
	lwres_gnbaresponse_t *response;
	int ret;
	int recvlen;
	lwres_buffer_t b_in, b_out;
	lwres_lwpacket_t pkt;
	lwres_uint32_t serial;
	char *buffer;

	REQUIRE(ctx != NULL);
	REQUIRE(addrtype != 0);
	REQUIRE(addrlen != 0);
	REQUIRE(addr != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	b_in.base = NULL;
	b_out.base = NULL;
	response = NULL;
	buffer = NULL;
	serial = lwres_context_nextserial(ctx);

	buffer = CTXMALLOC(LWRES_RECVLENGTH);
	if (buffer == NULL) {
		ret = LWRES_R_NOMEMORY;
		goto out;
	}

	/*
	 * Set up our request and render it to a buffer.
	 */
	request.flags = 0;
	request.addr.family = addrtype;
	request.addr.length = addrlen;
	memcpy(request.addr.address, addr, addrlen);
	pkt.pktflags = 0;
	pkt.serial = serial;
	pkt.result = 0;
	pkt.recvlength = LWRES_RECVLENGTH;

 again:
	ret = lwres_gnbarequest_render(ctx, &request, &pkt, &b_out);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	ret = lwres_context_sendrecv(ctx, b_out.base, b_out.length, buffer,
				     LWRES_RECVLENGTH, &recvlen);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	lwres_buffer_init(&b_in, buffer, recvlen);
	b_in.used = recvlen;

	/*
	 * Parse the packet header.
	 */
	ret = lwres_lwpacket_parseheader(&b_in, &pkt);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	/*
	 * Sanity check.
	 */
	if (pkt.serial != serial)
		goto again;
	if (pkt.opcode != LWRES_OPCODE_GETNAMEBYADDR)
		goto again;

	/*
	 * Free what we've transmitted
	 */
	CTXFREE(b_out.base, b_out.length);
	b_out.base = NULL;
	b_out.length = 0;

	if (pkt.result != LWRES_R_SUCCESS) {
		ret = pkt.result;
		goto out;
	}

	/*
	 * Parse the response.
	 */
	ret = lwres_gnbaresponse_parse(ctx, &b_in, &pkt, &response);
	if (ret != LWRES_R_SUCCESS)
		goto out;
	response->base = buffer;
	response->baselen = LWRES_RECVLENGTH;
	buffer = NULL; /* don't free this below */

	*structp = response;
	return (LWRES_R_SUCCESS);

 out:
	if (b_out.base != NULL)
		CTXFREE(b_out.base, b_out.length);
	if (buffer != NULL)
		CTXFREE(buffer, LWRES_RECVLENGTH);
	if (response != NULL)
		lwres_gnbaresponse_free(ctx, &response);

	return (ret);
}

/*% Get rdata by name. */
lwres_result_t
lwres_getrdatabyname(lwres_context_t *ctx, const char *name,
		     lwres_uint16_t rdclass, lwres_uint16_t rdtype,
		     lwres_uint32_t flags, lwres_grbnresponse_t **structp)
{
	int ret;
	int recvlen;
	lwres_buffer_t b_in, b_out;
	lwres_lwpacket_t pkt;
	lwres_uint32_t serial;
	char *buffer;
	lwres_grbnrequest_t request;
	lwres_grbnresponse_t *response;
	char target_name[1024];
	unsigned int target_length;

	REQUIRE(ctx != NULL);
	REQUIRE(name != NULL);
	REQUIRE(structp != NULL && *structp == NULL);

	b_in.base = NULL;
	b_out.base = NULL;
	response = NULL;
	buffer = NULL;
	serial = lwres_context_nextserial(ctx);

	buffer = CTXMALLOC(LWRES_RECVLENGTH);
	if (buffer == NULL) {
		ret = LWRES_R_NOMEMORY;
		goto out;
	}

	target_length = strlen(name);
	if (target_length >= sizeof(target_name))
		return (LWRES_R_FAILURE);
	strcpy(target_name, name); /* strcpy is safe */

	/*
	 * Set up our request and render it to a buffer.
	 */
	request.rdclass = rdclass;
	request.rdtype = rdtype;
	request.flags = flags;
	request.name = target_name;
	request.namelen = target_length;
	pkt.pktflags = 0;
	pkt.serial = serial;
	pkt.result = 0;
	pkt.recvlength = LWRES_RECVLENGTH;

 again:
	ret = lwres_grbnrequest_render(ctx, &request, &pkt, &b_out);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	ret = lwres_context_sendrecv(ctx, b_out.base, b_out.length, buffer,
				     LWRES_RECVLENGTH, &recvlen);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	lwres_buffer_init(&b_in, buffer, recvlen);
	b_in.used = recvlen;

	/*
	 * Parse the packet header.
	 */
	ret = lwres_lwpacket_parseheader(&b_in, &pkt);
	if (ret != LWRES_R_SUCCESS)
		goto out;

	/*
	 * Sanity check.
	 */
	if (pkt.serial != serial)
		goto again;
	if (pkt.opcode != LWRES_OPCODE_GETRDATABYNAME)
		goto again;

	/*
	 * Free what we've transmitted
	 */
	CTXFREE(b_out.base, b_out.length);
	b_out.base = NULL;
	b_out.length = 0;

	if (pkt.result != LWRES_R_SUCCESS) {
		ret = pkt.result;
		goto out;
	}

	/*
	 * Parse the response.
	 */
	ret = lwres_grbnresponse_parse(ctx, &b_in, &pkt, &response);
	if (ret != LWRES_R_SUCCESS)
		goto out;
	response->base = buffer;
	response->baselen = LWRES_RECVLENGTH;
	buffer = NULL; /* don't free this below */

	*structp = response;
	return (LWRES_R_SUCCESS);

 out:
	if (b_out.base != NULL)
		CTXFREE(b_out.base, b_out.length);
	if (buffer != NULL)
		CTXFREE(buffer, LWRES_RECVLENGTH);
	if (response != NULL)
		lwres_grbnresponse_free(ctx, &response);

	return (ret);
}
