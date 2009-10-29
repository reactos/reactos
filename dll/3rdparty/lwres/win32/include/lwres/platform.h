/*
 * Copyright (C) 2004, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: platform.h,v 1.7 2007/06/18 23:47:52 tbox Exp $ */

#ifndef LWRES_PLATFORM_H
#define LWRES_PLATFORM_H 1

/*****
 ***** Platform-dependent defines.
 *****/

/***
 *** Network.
 ***/

/*
 * Define if this system needs the <netinet/in6.h> header file for IPv6.
 */
/*@LWRES_PLATFORM_NEEDNETINETIN6H@ */

/*
 * Define if this system needs the <netinet6/in6.h> header file for IPv6.
 */
/*@LWRES_PLATFORM_NEEDNETINET6IN6H@ */

/*
 * If sockaddrs on this system have an sa_len field, LWRES_PLATFORM_HAVESALEN
 * will be defined.
 */
/*@LWRES_PLATFORM_HAVESALEN@ */

/*
 * If this system has the IPv6 structure definitions, LWRES_PLATFORM_HAVEIPV6
 * will be defined.
 */
/*@LWRES_PLATFORM_HAVEIPV6@ */

/*
 * If this system is missing in6addr_any, LWRES_PLATFORM_NEEDIN6ADDRANY will
 * be defined.
 */
#define LWRES_PLATFORM_NEEDIN6ADDRANY

/*
 * If this system has in_addr6, rather than in6_addr,
 * LWRES_PLATFORM_HAVEINADDR6 will be defined.
 */
/*@LWRES_PLATFORM_HAVEINADDR6@ */

/*
 * Defined if unistd.h does not cause fd_set to be delared.
 */
/*@LWRES_PLATFORM_NEEDSYSSELECTH@ */

/*
 * Define some Macros
 */
#ifdef LIBLWRES_EXPORTS
#define LIBLWRES_EXTERNAL_DATA __declspec(dllexport)
#else
#define LIBLWRES_EXTERNAL_DATA __declspec(dllimport)
#endif

/*
 * Define the MAKE_NONBLOCKING Macro here since it can get used in
 * a number of places.
 */
#define MAKE_NONBLOCKING(sd, retval) \
do { \
	int _on = 1; \
	retval = ioctlsocket((SOCKET) sd, FIONBIO, &_on); \
} while (0)

/*
 * Need to define close here since lwres closes sockets and not files
 */
#undef  close
#define close closesocket

/*
 * Internal to liblwres.
 */
void InitSockets(void);

void DestroySockets(void);

#endif /* LWRES_PLATFORM_H */
