/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2003  Internet Software Consortium.
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

/* $Id: stdlib.h,v 1.6 2007/06/19 23:47:23 tbox Exp $ */

#ifndef LWRES_STDLIB_H
#define LWRES_STDLIB_H 1

/*! \file lwres/stdlib.h */

#include <stdlib.h>

#include <lwres/lang.h>
#include <lwres/platform.h>

#ifdef LWRES_PLATFORM_NEEDSTRTOUL
#define strtoul lwres_strtoul
#endif

LWRES_LANG_BEGINDECLS

unsigned long lwres_strtoul(const char *, char **, int);

LWRES_LANG_ENDDECLS

#endif
