/*
 * $Id: duration.c 4518 2011-02-24 15:39:09Z matthijs $
 *
 * Copyright (c) 2009 NLNet Labs. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/**
 *
 * This file is copied from the OpenDNSSEC source repository
 * and only slightly adapted to make it fit.
 */

/**
 *
 * Durations.
 */

#include <ldns/config.h>
#include <ldns/duration.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


/**
 * Create a new 'instant' duration.
 *
 */
ldns_duration_type*
ldns_duration_create(void)
{
    ldns_duration_type* duration;

    duration = malloc(sizeof(ldns_duration_type));
    if (!duration) {
        return NULL;
    }
    duration->years = 0;
    duration->months = 0;
    duration->weeks = 0;
    duration->days = 0;
    duration->hours = 0;
    duration->minutes = 0;
    duration->seconds = 0;
    return duration;
}


/**
 * Compare durations.
 *
 */
int
ldns_duration_compare(const ldns_duration_type* d1, const ldns_duration_type* d2)
{
    if (!d1 && !d2) {
        return 0;
    }
    if (!d1 || !d2) {
        return d1?-1:1;
    }

    if (d1->years != d2->years) {
        return (int) (d1->years - d2->years);
    }
    if (d1->months != d2->months) {
        return (int) (d1->months - d2->months);
    }
    if (d1->weeks != d2->weeks) {
        return (int) (d1->weeks - d2->weeks);
    }
    if (d1->days != d2->days) {
        return (int) (d1->days - d2->days);
    }
    if (d1->hours != d2->hours) {
        return (int) (d1->hours - d2->hours);
    }
    if (d1->minutes != d2->minutes) {
        return (int) (d1->minutes - d2->minutes);
    }
    if (d1->seconds != d2->seconds) {
        return (int) (d1->seconds - d2->seconds);
    }

    return 0;
}


/**
 * Create a duration from string.
 *
 */
ldns_duration_type*
ldns_duration_create_from_string(const char* str)
{
    ldns_duration_type* duration = ldns_duration_create();
    char* P, *X, *T, *W;
    int not_weeks = 0;

    if (!duration) {
        return NULL;
    }
    if (!str) {
        return duration;
    }

    P = strchr(str, 'P');
    if (!P) {
	ldns_duration_cleanup(duration);
        return NULL;
    }

    T = strchr(str, 'T');
    X = strchr(str, 'Y');
    if (X) {
        duration->years = (time_t) atoi(str+1);
        str = X;
        not_weeks = 1;
    }
    X = strchr(str, 'M');
    if (X && (!T || (size_t) (X-P) < (size_t) (T-P))) {
        duration->months = (time_t) atoi(str+1);
        str = X;
        not_weeks = 1;
    }
    X = strchr(str, 'D');
    if (X) {
        duration->days = (time_t) atoi(str+1);
        str = X;
        not_weeks = 1;
    }
    if (T) {
        str = T;
        not_weeks = 1;
    }
    X = strchr(str, 'H');
    if (X && T) {
        duration->hours = (time_t) atoi(str+1);
        str = X;
        not_weeks = 1;
    }
    X = strrchr(str, 'M');
    if (X && T && (size_t) (X-P) > (size_t) (T-P)) {
        duration->minutes = (time_t) atoi(str+1);
        str = X;
        not_weeks = 1;
    }
    X = strchr(str, 'S');
    if (X && T) {
        duration->seconds = (time_t) atoi(str+1);
        str = X;
        not_weeks = 1;
    }

    W = strchr(str, 'W');
    if (W) {
        if (not_weeks) {
            ldns_duration_cleanup(duration);
            return NULL;
        } else {
            duration->weeks = (time_t) atoi(str+1);
        }
    }
    return duration;
}


/**
 * Helper func for ldns_duration2string below. If t > 0,
 * scan print t and c on buf, forwarding buf. Return 0 on success.
 */
static inline int dur_scan_print(char **buf, char *eob, char c, time_t t)
{
	if (t > 0) {
		int r = snprintf(*buf, eob - *buf, "%u%c", (unsigned)t, c);
		if (r < 0 || (*buf += r) >= eob)
			return -1;
	}
	return 0;
}

/**
 * Convert a duration to a string.
 *
 */
char*
ldns_duration2string(const ldns_duration_type* d)
{
 	/* Max string size should be 7 * 40 + 3 on a 127 bits machine
	 * So 300 (< 273) is more than enough.
	 */
	char buf[300] = "P0D", *eob = buf + sizeof(buf), *p = buf + 1;

	if (!d)
		return NULL;

	if (dur_scan_print(&p, eob, 'Y', d->years)
	||  dur_scan_print(&p, eob, 'M', d->months)
	||  dur_scan_print(&p, eob, 'W', d->weeks)
	||  dur_scan_print(&p, eob, 'D', d->days))
		return NULL;

	if (d->hours || d->minutes || d->seconds) {
		if (p > (eob - 2))
			return NULL; /* Error; no space left on buf for 'T' */

		*p++ = 'T'; *p = 0;
		if (dur_scan_print(&p, eob, 'H', d->hours)
		||  dur_scan_print(&p, eob, 'M', d->minutes)
		||  dur_scan_print(&p, eob, 'S', d->seconds))
			return NULL;
	}
	return _strdup(buf);
}


/**
 * Convert a duration to a time.
 *
 */
time_t
ldns_duration2time(const ldns_duration_type* duration)
{
    time_t period = 0;

    if (duration) {
        period += (duration->seconds);
        period += (duration->minutes)*60;
        period += (duration->hours)*3600;
        period += (duration->days)*86400;
        period += (duration->weeks)*86400*7;
        period += (duration->months)*86400*31;
        period += (duration->years)*86400*365;

        /* [TODO] calculate correct number of days in this month/year */
	/*
        if (duration->months || duration->years) {
        }
	*/
    }
    return period;
}


/**
 * Clean up duration.
 *
 */
void
ldns_duration_cleanup(ldns_duration_type* duration)
{
    if (!duration) {
        return;
    }
    free(duration);
    return;
}
