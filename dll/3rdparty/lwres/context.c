/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001, 2003  Internet Software Consortium.
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

/* $Id: context.c,v 1.50.332.2 2008/12/30 23:46:49 tbox Exp $ */

/*! \file context.c
   lwres_context_create() creates a #lwres_context_t structure for use in
   lightweight resolver operations. It holds a socket and other data
   needed for communicating with a resolver daemon. The new
   lwres_context_t is returned through contextp, a pointer to a
   lwres_context_t pointer. This lwres_context_t pointer must initially
   be NULL, and is modified to point to the newly created
   lwres_context_t.

   When the lightweight resolver needs to perform dynamic memory
   allocation, it will call malloc_function to allocate memory and
   free_function to free it. If malloc_function and free_function are
   NULL, memory is allocated using malloc and free. It is not
   permitted to have a NULL malloc_function and a non-NULL free_function
   or vice versa. arg is passed as the first parameter to the memory
   allocation functions. If malloc_function and free_function are NULL,
   arg is unused and should be passed as NULL.

   Once memory for the structure has been allocated, it is initialized
   using lwres_conf_init() and returned via *contextp.

   lwres_context_destroy() destroys a #lwres_context_t, closing its
   socket. contextp is a pointer to a pointer to the context that is to
   be destroyed. The pointer will be set to NULL when the context has
   been destroyed.

   The context holds a serial number that is used to identify resolver
   request packets and associate responses with the corresponding
   requests. This serial number is controlled using
   lwres_context_initserial() and lwres_context_nextserial().
   lwres_context_initserial() sets the serial number for context *ctx to
   serial. lwres_context_nextserial() increments the serial number and
   returns the previous value.

   Memory for a lightweight resolver context is allocated and freed using
   lwres_context_allocmem() and lwres_context_freemem(). These use
   whatever allocations were defined when the context was created with
   lwres_context_create(). lwres_context_allocmem() allocates len bytes
   of memory and if successful returns a pointer to the allocated
   storage. lwres_context_freemem() frees len bytes of space starting at
   location mem.

   lwres_context_sendrecv() performs I/O for the context ctx. Data are
   read and written from the context's socket. It writes data from
   sendbase -- typically a lightweight resolver query packet -- and waits
   for a reply which is copied to the receive buffer at recvbase. The
   number of bytes that were written to this receive buffer is returned
   in *recvd_len.

\section context_return Return Values

   lwres_context_create() returns #LWRES_R_NOMEMORY if memory for the
   struct lwres_context could not be allocated, #LWRES_R_SUCCESS
   otherwise.

   Successful calls to the memory allocator lwres_context_allocmem()
   return a pointer to the start of the allocated space. It returns NULL
   if memory could not be allocated.

   #LWRES_R_SUCCESS is returned when lwres_context_sendrecv() completes
   successfully. #LWRES_R_IOERROR is returned if an I/O error occurs and
   #LWRES_R_TIMEOUT is returned if lwres_context_sendrecv() times out
   waiting for a response.

\section context_see See Also

   lwres_conf_init(), malloc, free.
 */
#include <config.h>

#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <lwres/lwres.h>
#include <lwres/net.h>
#include <lwres/platform.h>

#ifdef LWRES_PLATFORM_NEEDSYSSELECTH
#include <sys/select.h>
#endif

#include "context_p.h"
#include "assert_p.h"

/*!
 * Some systems define the socket length argument as an int, some as size_t,
 * some as socklen_t.  The last is what the current POSIX standard mandates.
 * This definition is here so it can be portable but easily changed if needed.
 */
#ifndef LWRES_SOCKADDR_LEN_T
#define LWRES_SOCKADDR_LEN_T unsigned int
#endif

/*!
 * Make a socket nonblocking.
 */
#ifndef MAKE_NONBLOCKING
#define MAKE_NONBLOCKING(sd, retval) \
do { \
	retval = fcntl(sd, F_GETFL, 0); \
	if (retval != -1) { \
		retval |= O_NONBLOCK; \
		retval = fcntl(sd, F_SETFL, retval); \
	} \
} while (0)
#endif

LIBLWRES_EXTERNAL_DATA lwres_uint16_t lwres_udp_port = LWRES_UDP_PORT;
LIBLWRES_EXTERNAL_DATA const char *lwres_resolv_conf = LWRES_RESOLV_CONF;

static void *
lwres_malloc(void *, size_t);

static void
lwres_free(void *, void *, size_t);

/*!
 * lwres_result_t
 */
static lwres_result_t
context_connect(lwres_context_t *);

/*%
 * Creates a #lwres_context_t structure for use in
 *  lightweight resolver operations.
 */
lwres_result_t
lwres_context_create(lwres_context_t **contextp, void *arg,
		     lwres_malloc_t malloc_function,
		     lwres_free_t free_function,
		     unsigned int flags)
{
	lwres_context_t *ctx;

	REQUIRE(contextp != NULL && *contextp == NULL);

	/*
	 * If we were not given anything special to use, use our own
	 * functions.  These are just wrappers around malloc() and free().
	 */
	if (malloc_function == NULL || free_function == NULL) {
		REQUIRE(malloc_function == NULL);
		REQUIRE(free_function == NULL);
		malloc_function = lwres_malloc;
		free_function = lwres_free;
	}

	ctx = malloc_function(arg, sizeof(lwres_context_t));
	if (ctx == NULL)
		return (LWRES_R_NOMEMORY);

	/*
	 * Set up the context.
	 */
	ctx->malloc = malloc_function;
	ctx->free = free_function;
	ctx->arg = arg;
	ctx->sock = -1;

	ctx->timeout = LWRES_DEFAULT_TIMEOUT;
	ctx->serial = time(NULL); /* XXXMLG or BEW */

	ctx->use_ipv4 = 1;
	ctx->use_ipv6 = 1;
	if ((flags & (LWRES_CONTEXT_USEIPV4 | LWRES_CONTEXT_USEIPV6)) ==
	    LWRES_CONTEXT_USEIPV6) {
		ctx->use_ipv4 = 0;
	}
	if ((flags & (LWRES_CONTEXT_USEIPV4 | LWRES_CONTEXT_USEIPV6)) ==
	    LWRES_CONTEXT_USEIPV4) {
		ctx->use_ipv6 = 0;
	}

	/*
	 * Init resolv.conf bits.
	 */
	lwres_conf_init(ctx);

	*contextp = ctx;
	return (LWRES_R_SUCCESS);
}

/*%
Destroys a #lwres_context_t, closing its socket.
contextp is a pointer to a pointer to the context that is
to be destroyed. The pointer will be set to NULL
when the context has been destroyed.
 */
void
lwres_context_destroy(lwres_context_t **contextp) {
	lwres_context_t *ctx;

	REQUIRE(contextp != NULL && *contextp != NULL);

	ctx = *contextp;
	*contextp = NULL;

	if (ctx->sock != -1) {
#ifdef WIN32
		DestroySockets();
#endif
		(void)close(ctx->sock);
		ctx->sock = -1;
	}

	CTXFREE(ctx, sizeof(lwres_context_t));
}
/*% Increments the serial number and returns the previous value. */
lwres_uint32_t
lwres_context_nextserial(lwres_context_t *ctx) {
	REQUIRE(ctx != NULL);

	return (ctx->serial++);
}

/*% Sets the serial number for context *ctx to serial. */
void
lwres_context_initserial(lwres_context_t *ctx, lwres_uint32_t serial) {
	REQUIRE(ctx != NULL);

	ctx->serial = serial;
}

/*% Frees len bytes of space starting at location mem. */
void
lwres_context_freemem(lwres_context_t *ctx, void *mem, size_t len) {
	REQUIRE(mem != NULL);
	REQUIRE(len != 0U);

	CTXFREE(mem, len);
}

/*% Allocates len bytes of memory and if successful returns a pointer to the allocated storage. */
void *
lwres_context_allocmem(lwres_context_t *ctx, size_t len) {
	REQUIRE(len != 0U);

	return (CTXMALLOC(len));
}

static void *
lwres_malloc(void *arg, size_t len) {
	void *mem;

	UNUSED(arg);

	mem = malloc(len);
	if (mem == NULL)
		return (NULL);

	memset(mem, 0xe5, len);

	return (mem);
}

static void
lwres_free(void *arg, void *mem, size_t len) {
	UNUSED(arg);

	memset(mem, 0xa9, len);
	free(mem);
}

static lwres_result_t
context_connect(lwres_context_t *ctx) {
	int s;
	int ret;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	struct sockaddr *sa;
	LWRES_SOCKADDR_LEN_T salen;
	int domain;

	if (ctx->confdata.lwnext != 0) {
		memcpy(&ctx->address, &ctx->confdata.lwservers[0],
		       sizeof(lwres_addr_t));
		LWRES_LINK_INIT(&ctx->address, link);
	} else {
		/* The default is the IPv4 loopback address 127.0.0.1. */
		memset(&ctx->address, 0, sizeof(ctx->address));
		ctx->address.family = LWRES_ADDRTYPE_V4;
		ctx->address.length = 4;
		ctx->address.address[0] = 127;
		ctx->address.address[1] = 0;
		ctx->address.address[2] = 0;
		ctx->address.address[3] = 1;
	}

	if (ctx->address.family == LWRES_ADDRTYPE_V4) {
		memcpy(&sin.sin_addr, ctx->address.address,
		       sizeof(sin.sin_addr));
		sin.sin_port = htons(lwres_udp_port);
		sin.sin_family = AF_INET;
		sa = (struct sockaddr *)&sin;
		salen = sizeof(sin);
		domain = PF_INET;
	} else if (ctx->address.family == LWRES_ADDRTYPE_V6) {
		memcpy(&sin6.sin6_addr, ctx->address.address,
		       sizeof(sin6.sin6_addr));
		sin6.sin6_port = htons(lwres_udp_port);
		sin6.sin6_family = AF_INET6;
		sa = (struct sockaddr *)&sin6;
		salen = sizeof(sin6);
		domain = PF_INET6;
	} else
		return (LWRES_R_IOERROR);

#ifdef WIN32
	InitSockets();
#endif
	s = socket(domain, SOCK_DGRAM, IPPROTO_UDP);
	if (s < 0) {
#ifdef WIN32
		DestroySockets();
#endif
		return (LWRES_R_IOERROR);
	}

	ret = connect(s, sa, salen);
	if (ret != 0) {
#ifdef WIN32
		DestroySockets();
#endif
		(void)close(s);
		return (LWRES_R_IOERROR);
	}

	MAKE_NONBLOCKING(s, ret);
	if (ret < 0) {
#ifdef WIN32
		DestroySockets();
#endif
		(void)close(s);
		return (LWRES_R_IOERROR);
	}

	ctx->sock = s;

	return (LWRES_R_SUCCESS);
}

int
lwres_context_getsocket(lwres_context_t *ctx) {
	return (ctx->sock);
}

lwres_result_t
lwres_context_send(lwres_context_t *ctx,
		   void *sendbase, int sendlen) {
	int ret;
	lwres_result_t lwresult;

	if (ctx->sock == -1) {
		lwresult = context_connect(ctx);
		if (lwresult != LWRES_R_SUCCESS)
			return (lwresult);
	}

	ret = sendto(ctx->sock, sendbase, sendlen, 0, NULL, 0);
	if (ret < 0)
		return (LWRES_R_IOERROR);
	if (ret != sendlen)
		return (LWRES_R_IOERROR);

	return (LWRES_R_SUCCESS);
}

lwres_result_t
lwres_context_recv(lwres_context_t *ctx,
		   void *recvbase, int recvlen,
		   int *recvd_len)
{
	LWRES_SOCKADDR_LEN_T fromlen;
	struct sockaddr_in sin;
	struct sockaddr_in6 sin6;
	struct sockaddr *sa;
	int ret;

	if (ctx->address.family == LWRES_ADDRTYPE_V4) {
		sa = (struct sockaddr *)&sin;
		fromlen = sizeof(sin);
	} else {
		sa = (struct sockaddr *)&sin6;
		fromlen = sizeof(sin6);
	}

	/*
	 * The address of fromlen is cast to void * to shut up compiler
	 * warnings, namely on systems that have the sixth parameter
	 * prototyped as a signed int when LWRES_SOCKADDR_LEN_T is
	 * defined as unsigned.
	 */
	ret = recvfrom(ctx->sock, recvbase, recvlen, 0, sa, (void *)&fromlen);

	if (ret < 0)
		return (LWRES_R_IOERROR);

	if (ret == recvlen)
		return (LWRES_R_TOOLARGE);

	/*
	 * If we got something other than what we expect, have the caller
	 * wait for another packet.  This can happen if an old result
	 * comes in, or if someone is sending us random stuff.
	 */
	if (ctx->address.family == LWRES_ADDRTYPE_V4) {
		if (fromlen != sizeof(sin)
		    || memcmp(&sin.sin_addr, ctx->address.address,
			      sizeof(sin.sin_addr)) != 0
		    || sin.sin_port != htons(lwres_udp_port))
			return (LWRES_R_RETRY);
	} else {
		if (fromlen != sizeof(sin6)
		    || memcmp(&sin6.sin6_addr, ctx->address.address,
			      sizeof(sin6.sin6_addr)) != 0
		    || sin6.sin6_port != htons(lwres_udp_port))
			return (LWRES_R_RETRY);
	}

	if (recvd_len != NULL)
		*recvd_len = ret;

	return (LWRES_R_SUCCESS);
}

/*% performs I/O for the context ctx. */
lwres_result_t
lwres_context_sendrecv(lwres_context_t *ctx,
		       void *sendbase, int sendlen,
		       void *recvbase, int recvlen,
		       int *recvd_len)
{
	lwres_result_t result;
	int ret2;
	fd_set readfds;
	struct timeval timeout;

	/*
	 * Type of tv_sec is 32 bits long.
	 */
	if (ctx->timeout <= 0x7FFFFFFFU)
		timeout.tv_sec = (int)ctx->timeout;
	else
		timeout.tv_sec = 0x7FFFFFFF;

	timeout.tv_usec = 0;

	result = lwres_context_send(ctx, sendbase, sendlen);
	if (result != LWRES_R_SUCCESS)
		return (result);
 again:
	FD_ZERO(&readfds);
	FD_SET(ctx->sock, &readfds);
	ret2 = select(ctx->sock + 1, &readfds, NULL, NULL, &timeout);

	/*
	 * What happened with select?
	 */
	if (ret2 < 0)
		return (LWRES_R_IOERROR);
	if (ret2 == 0)
		return (LWRES_R_TIMEOUT);

	result = lwres_context_recv(ctx, recvbase, recvlen, recvd_len);
	if (result == LWRES_R_RETRY)
		goto again;

	return (result);
}
