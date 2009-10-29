/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: context.h,v 1.21.332.2 2008/12/30 23:46:49 tbox Exp $ */

#ifndef LWRES_CONTEXT_H
#define LWRES_CONTEXT_H 1

/*! \file lwres/context.h */

#include <stddef.h>

#include <lwres/lang.h>
#include <lwres/int.h>
#include <lwres/result.h>

/*!
 * Used to set various options such as timeout, authentication, etc
 */
typedef struct lwres_context lwres_context_t;

LWRES_LANG_BEGINDECLS

typedef void *(*lwres_malloc_t)(void *arg, size_t length);
typedef void (*lwres_free_t)(void *arg, void *mem, size_t length);

/*
 * XXXMLG
 *
 * Make the server reload /etc/resolv.conf periodically.
 *
 * Make the server do sortlist/searchlist.
 *
 * Client side can disable the search/sortlist processing.
 *
 * Use an array of addresses/masks and searchlist for client-side, and
 * if added to the client disable the processing on the server.
 *
 * Share /etc/resolv.conf data between contexts.
 */

/*!
 * _SERVERMODE
 *	Don't allocate and connect a socket to the server, since the
 *	caller _is_ a server.
 *
 * _USEIPV4, _USEIPV6
 *	Use IPv4 and IPv6 transactions with remote servers, respectively.
 *	For backward compatibility, regard both flags as being set when both
 *	are cleared.
 */
#define LWRES_CONTEXT_SERVERMODE	0x00000001U
#define LWRES_CONTEXT_USEIPV4		0x00000002U
#define LWRES_CONTEXT_USEIPV6		0x00000004U

lwres_result_t
lwres_context_create(lwres_context_t **contextp, void *arg,
		     lwres_malloc_t malloc_function,
		     lwres_free_t free_function,
		     unsigned int flags);
/**<
 * Allocate a lwres context.  This is used in all lwres calls.
 *
 * Memory management can be replaced here by passing in two functions.
 * If one is non-NULL, they must both be non-NULL.  "arg" is passed to
 * these functions.
 *
 * Contexts are not thread safe.  Document at the top of the file.
 * XXXMLG
 *
 * If they are NULL, the standard malloc() and free() will be used.
 *
 *\pre	contextp != NULL && contextp == NULL.
 *
 *\return	Returns 0 on success, non-zero on failure.
 */

void
lwres_context_destroy(lwres_context_t **contextp);
/**<
 * Frees all memory associated with a lwres context.
 *
 *\pre	contextp != NULL && contextp == NULL.
 */

lwres_uint32_t
lwres_context_nextserial(lwres_context_t *ctx);
/**<
 * XXXMLG Document
 */

void
lwres_context_initserial(lwres_context_t *ctx, lwres_uint32_t serial);

void
lwres_context_freemem(lwres_context_t *ctx, void *mem, size_t len);

void *
lwres_context_allocmem(lwres_context_t *ctx, size_t len);

int
lwres_context_getsocket(lwres_context_t *ctx);

lwres_result_t
lwres_context_send(lwres_context_t *ctx,
		   void *sendbase, int sendlen);

lwres_result_t
lwres_context_recv(lwres_context_t *ctx,
		   void *recvbase, int recvlen,
		   int *recvd_len);

lwres_result_t
lwres_context_sendrecv(lwres_context_t *ctx,
		       void *sendbase, int sendlen,
		       void *recvbase, int recvlen,
		       int *recvd_len);

LWRES_LANG_ENDDECLS

#endif /* LWRES_CONTEXT_H */

