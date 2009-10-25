/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2001  Internet Software Consortium.
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

/* $Id: lib.c,v 1.14 2007/06/19 23:47:17 tbox Exp $ */

/*! \file */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>

#include <isc/once.h>
#include <isc/msgs.h>
#include <isc/lib.h>

/***
 *** Globals
 ***/

LIBISC_EXTERNAL_DATA isc_msgcat_t *		isc_msgcat = NULL;


/***
 *** Private
 ***/

static isc_once_t		msgcat_once = ISC_ONCE_INIT;


/***
 *** Functions
 ***/

static void
open_msgcat(void) {
	isc_msgcat_open("libisc.cat", &isc_msgcat);
}

void
isc_lib_initmsgcat(void) {
	isc_result_t result;

	/*!
	 * Initialize the ISC library's message catalog, isc_msgcat, if it
	 * has not already been initialized.
	 */

	result = isc_once_do(&msgcat_once, open_msgcat);
	if (result != ISC_R_SUCCESS) {
		/*
		 * Normally we'd use RUNTIME_CHECK() or FATAL_ERROR(), but
		 * we can't do that here, since they might call us!
		 * (Note that the catalog might be open anyway, so we might
		 * as well try to  provide an internationalized message.)
		 */
		fprintf(stderr, "%s:%d: %s: isc_once_do() %s.\n",
			__FILE__, __LINE__,
			isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				       ISC_MSG_FATALERROR, "fatal error"),
			isc_msgcat_get(isc_msgcat, ISC_MSGSET_GENERAL,
				       ISC_MSG_FAILED, "failed"));
		abort();
	}
}
