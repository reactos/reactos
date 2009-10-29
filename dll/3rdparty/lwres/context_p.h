/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: context_p.h,v 1.17.332.2 2008/12/30 23:46:49 tbox Exp $ */

#ifndef LWRES_CONTEXT_P_H
#define LWRES_CONTEXT_P_H 1

/*! \file */

/*@{*/
/**
 * Helper functions, assuming the context is always called "ctx" in
 * the scope these functions are called from.
 */
#define CTXMALLOC(len)		ctx->malloc(ctx->arg, (len))
#define CTXFREE(addr, len)	ctx->free(ctx->arg, (addr), (len))
/*@}*/

#define LWRES_DEFAULT_TIMEOUT	120	/* 120 seconds for a reply */

/**
 * Not all the attributes here are actually settable by the application at
 * this time.
 */
struct lwres_context {
	unsigned int		timeout;	/*%< time to wait for reply */
	lwres_uint32_t		serial;		/*%< serial number state */

	/*
	 * For network I/O.
	 */
	int			sock;		/*%< socket to send on */
	lwres_addr_t		address;	/*%< address to send to */
	int			use_ipv4;	/*%< use IPv4 transaction */
	int			use_ipv6;	/*%< use IPv6 transaction */

	/*@{*/
	/*
	 * Function pointers for allocating memory.
	 */
	lwres_malloc_t		malloc;
	lwres_free_t		free;
	void		       *arg;
	/*@}*/

	/*%
	 * resolv.conf-like data
	 */
	lwres_conf_t		confdata;
};

#endif /* LWRES_CONTEXT_P_H */
