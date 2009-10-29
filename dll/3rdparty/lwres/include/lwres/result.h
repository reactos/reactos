/*
 * Copyright (C) 2004-2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: result.h,v 1.21 2007/06/19 23:47:23 tbox Exp $ */

#ifndef LWRES_RESULT_H
#define LWRES_RESULT_H 1

/*! \file lwres/result.h */

typedef unsigned int lwres_result_t;

#define LWRES_R_SUCCESS			0
#define LWRES_R_NOMEMORY		1
#define LWRES_R_TIMEOUT			2
#define LWRES_R_NOTFOUND		3
#define LWRES_R_UNEXPECTEDEND		4	/* unexpected end of input */
#define LWRES_R_FAILURE			5	/* generic failure */
#define LWRES_R_IOERROR			6
#define LWRES_R_NOTIMPLEMENTED		7
#define LWRES_R_UNEXPECTED		8
#define LWRES_R_TRAILINGDATA		9
#define LWRES_R_INCOMPLETE		10
#define LWRES_R_RETRY			11
#define LWRES_R_TYPENOTFOUND		12
#define LWRES_R_TOOLARGE		13

#endif /* LWRES_RESULT_H */
