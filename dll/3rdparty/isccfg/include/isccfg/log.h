/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2001  Internet Software Consortium.
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

/* $Id: log.h,v 1.12.332.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef ISCCFG_LOG_H
#define ISCCFG_LOG_H 1

/*! \file isccfg/log.h */

#include <isc/lang.h>
#include <isc/log.h>

LIBISCCFG_EXTERNAL_DATA extern isc_logcategory_t cfg_categories[];
LIBISCCFG_EXTERNAL_DATA extern isc_logmodule_t cfg_modules[];

#define CFG_LOGCATEGORY_CONFIG	(&cfg_categories[0])

#define CFG_LOGMODULE_PARSER	(&cfg_modules[0])

ISC_LANG_BEGINDECLS

void
cfg_log_init(isc_log_t *lctx);
/*%<
 * Make the libisccfg categories and modules available for use with the
 * ISC logging library.
 *
 * Requires:
 *\li	lctx is a valid logging context.
 *
 *\li	cfg_log_init() is called only once.
 *
 * Ensures:
 * \li	The categories and modules defined above are available for
 * 	use by isc_log_usechannnel() and isc_log_write().
 */

ISC_LANG_ENDDECLS

#endif /* ISCCFG_LOG_H */
