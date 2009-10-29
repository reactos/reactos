/*
 * Copyright (C) 2004, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001, 2003  Internet Software Consortium.
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

/* $Id: print_p.h,v 1.4 2007/06/19 23:47:22 tbox Exp $ */

#ifndef LWRES_PRINT_P_H
#define LWRES_PRINT_P_H 1

/***
 *** Imports
 ***/

#include <lwres/lang.h>
#include <lwres/platform.h>

/*
 * This block allows lib/lwres/print.c to be cleanly compiled even if
 * the platform does not need it.  The standard Makefile will still
 * not compile print.c or archive print.o, so this is just to make test
 * compilation ("make print.o") easier.
 */
#if !defined(LWRES_PLATFORM_NEEDVSNPRINTF) && defined(LWRES__PRINT_SOURCE)
#define LWRES_PLATFORM_NEEDVSNPRINTF
#endif

#if !defined(LWRES_PLATFORM_NEEDSPRINTF) && defined(LWRES__PRINT_SOURCE)
#define LWRES_PLATFORM_NEEDSPRINTF
#endif

/***
 *** Macros.
 ***/

#ifdef __GNUC__
#define LWRES_FORMAT_PRINTF(fmt, args) \
        __attribute__((__format__(__printf__, fmt, args)))
#else
#define LWRES_FORMAT_PRINTF(fmt, args)
#endif

/***
 *** Functions
 ***/

#ifdef LWRES_PLATFORM_NEEDVSNPRINTF
#include <stdarg.h>
#include <stddef.h>
#endif

LWRES_LANG_BEGINDECLS

#ifdef LWRES_PLATFORM_NEEDVSNPRINTF
int
lwres__print_vsnprintf(char *str, size_t size, const char *format, va_list ap)
     LWRES_FORMAT_PRINTF(3, 0);
#define vsnprintf lwres__print_vsnprintf

int
lwres__print_snprintf(char *str, size_t size, const char *format, ...)
     LWRES_FORMAT_PRINTF(3, 4);
#define snprintf lwres__print_snprintf
#endif /* LWRES_PLATFORM_NEEDVSNPRINTF */

#ifdef LWRES_PLATFORM_NEEDSPRINTF
int
lwres__print_sprintf(char *str, const char *format, ...) LWRES_FORMAT_PRINTF(2, 3);
#define sprintf lwres__print_sprintf
#endif

LWRES_LANG_ENDDECLS

#endif /* LWRES_PRINT_P_H */
