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

/* $Id: timer.h,v 1.9 2007/06/19 23:47:17 tbox Exp $ */

#ifndef DNS_TIMER_H
#define DNS_TIMER_H 1

/*! \file dns/timer.h */

/***
 ***	Imports
 ***/

#include <isc/buffer.h>
#include <isc/lang.h>

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_timer_setidle(isc_timer_t *timer, unsigned int maxtime,
		  unsigned int idletime, isc_boolean_t purge);
/*%<
 * Convenience function for setting up simple, one-second-granularity
 * idle timers as used by zone transfers.
 * \brief
 * Set the timer 'timer' to go off after 'idletime' seconds of inactivity,
 * or after 'maxtime' at the very latest.  Events are purged iff
 * 'purge' is ISC_TRUE.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_TIMER_H */
