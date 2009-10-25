/*
 * Copyright (C) 2004, 2005, 2007-2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: stats.c,v 1.16.118.2 2009/01/29 23:47:44 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/stats.h>
#include <isc/util.h>

#include <dns/opcode.h>
#include <dns/rdatatype.h>
#include <dns/stats.h>

#define DNS_STATS_MAGIC			ISC_MAGIC('D', 's', 't', 't')
#define DNS_STATS_VALID(x)		ISC_MAGIC_VALID(x, DNS_STATS_MAGIC)

/*%
 * Statistics types.
 */
typedef enum {
	dns_statstype_general = 0,
	dns_statstype_rdtype = 1,
	dns_statstype_rdataset = 2,
	dns_statstype_opcode = 3
} dns_statstype_t;

/*%
 * It doesn't make sense to have 2^16 counters for all possible types since
 * most of them won't be used.  We have counters for the first 256 types and
 * those explicitly supported in the rdata implementation.
 * XXXJT: this introduces tight coupling with the rdata implementation.
 * Ideally, we should have rdata handle this type of details.
 */
enum {
	/* For 0-255, we use the rdtype value as counter indices */
	rdtypecounter_dlv = 256,	/* for dns_rdatatype_dlv */
	rdtypecounter_others = 257,	/* anything else */
	rdtypecounter_max = 258,
	/* The following are used for rdataset */
	rdtypenxcounter_max = rdtypecounter_max * 2,
	rdtypecounter_nxdomain = rdtypenxcounter_max,
	rdatasettypecounter_max = rdtypecounter_nxdomain + 1
};

struct dns_stats {
	/*% Unlocked */
	unsigned int	magic;
	dns_statstype_t	type;
	isc_mem_t	*mctx;
	isc_mutex_t	lock;
	isc_stats_t	*counters;

	/*%  Locked by lock */
	unsigned int	references;
};

typedef struct rdatadumparg {
	dns_rdatatypestats_dumper_t	fn;
	void				*arg;
} rdatadumparg_t;

typedef struct opcodedumparg {
	dns_opcodestats_dumper_t	fn;
	void				*arg;
} opcodedumparg_t;

void
dns_stats_attach(dns_stats_t *stats, dns_stats_t **statsp) {
	REQUIRE(DNS_STATS_VALID(stats));
	REQUIRE(statsp != NULL && *statsp == NULL);

	LOCK(&stats->lock);
	stats->references++;
	UNLOCK(&stats->lock);

	*statsp = stats;
}

void
dns_stats_detach(dns_stats_t **statsp) {
	dns_stats_t *stats;

	REQUIRE(statsp != NULL && DNS_STATS_VALID(*statsp));

	stats = *statsp;
	*statsp = NULL;

	LOCK(&stats->lock);
	stats->references--;
	UNLOCK(&stats->lock);

	if (stats->references == 0) {
		isc_stats_detach(&stats->counters);
		DESTROYLOCK(&stats->lock);
		isc_mem_putanddetach(&stats->mctx, stats, sizeof(*stats));
	}
}

/*%
 * Create methods
 */
static isc_result_t
create_stats(isc_mem_t *mctx, dns_statstype_t	type, int ncounters,
	     dns_stats_t **statsp)
{
	dns_stats_t *stats;
	isc_result_t result;

	stats = isc_mem_get(mctx, sizeof(*stats));
	if (stats == NULL)
		return (ISC_R_NOMEMORY);

	stats->counters = NULL;
	stats->references = 1;

	result = isc_mutex_init(&stats->lock);
	if (result != ISC_R_SUCCESS)
		goto clean_stats;

	result = isc_stats_create(mctx, &stats->counters, ncounters);
	if (result != ISC_R_SUCCESS)
		goto clean_mutex;

	stats->magic = DNS_STATS_MAGIC;
	stats->type = type;
	stats->mctx = NULL;
	isc_mem_attach(mctx, &stats->mctx);
	*statsp = stats;

	return (ISC_R_SUCCESS);

  clean_mutex:
	DESTROYLOCK(&stats->lock);
  clean_stats:
	isc_mem_put(mctx, stats, sizeof(*stats));

	return (result);
}

isc_result_t
dns_generalstats_create(isc_mem_t *mctx, dns_stats_t **statsp, int ncounters) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_general, ncounters, statsp));
}

isc_result_t
dns_rdatatypestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_rdtype, rdtypecounter_max,
			     statsp));
}

isc_result_t
dns_rdatasetstats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_rdataset,
			     (rdtypecounter_max * 2) + 1, statsp));
}

isc_result_t
dns_opcodestats_create(isc_mem_t *mctx, dns_stats_t **statsp) {
	REQUIRE(statsp != NULL && *statsp == NULL);

	return (create_stats(mctx, dns_statstype_opcode, 16, statsp));
}

/*%
 * Increment/Decrement methods
 */
void
dns_generalstats_increment(dns_stats_t *stats, isc_statscounter_t counter) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);

	isc_stats_increment(stats->counters, counter);
}

void
dns_rdatatypestats_increment(dns_stats_t *stats, dns_rdatatype_t type) {
	int counter;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rdtype);

	if (type == dns_rdatatype_dlv)
		counter = rdtypecounter_dlv;
	else if (type > dns_rdatatype_any)
		counter = rdtypecounter_others;
	else
		counter = (int)type;

	isc_stats_increment(stats->counters, (isc_statscounter_t)counter);
}

static inline void
update_rdatasetstats(dns_stats_t *stats, dns_rdatastatstype_t rrsettype,
		     isc_boolean_t increment)
{
	int counter;
	dns_rdatatype_t rdtype;

	if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
	     DNS_RDATASTATSTYPE_ATTR_NXDOMAIN) != 0) {
		counter = rdtypecounter_nxdomain;
	} else {
		rdtype = DNS_RDATASTATSTYPE_BASE(rrsettype);
		if (rdtype == dns_rdatatype_dlv)
			counter = (int)rdtypecounter_dlv;
		else if (rdtype > dns_rdatatype_any)
			counter = (int)rdtypecounter_others;
		else
			counter = (int)rdtype;

		if ((DNS_RDATASTATSTYPE_ATTR(rrsettype) &
		     DNS_RDATASTATSTYPE_ATTR_NXRRSET) != 0)
			counter += rdtypecounter_max;
	}

	if (increment)
		isc_stats_increment(stats->counters, counter);
	else
		isc_stats_decrement(stats->counters, counter);
}

void
dns_rdatasetstats_increment(dns_stats_t *stats, dns_rdatastatstype_t rrsettype)
{
	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	update_rdatasetstats(stats, rrsettype, ISC_TRUE);
}

void
dns_rdatasetstats_decrement(dns_stats_t *stats, dns_rdatastatstype_t rrsettype)
{
	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	update_rdatasetstats(stats, rrsettype, ISC_FALSE);
}
void
dns_opcodestats_increment(dns_stats_t *stats, dns_opcode_t code) {
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_opcode);

	isc_stats_increment(stats->counters, (isc_statscounter_t)code);
}

/*%
 * Dump methods
 */
void
dns_generalstats_dump(dns_stats_t *stats, dns_generalstats_dumper_t dump_fn,
		      void *arg, unsigned int options)
{
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_general);

	isc_stats_dump(stats->counters, (isc_stats_dumper_t)dump_fn,
		       arg, options);
}

static void
dump_rdentry(int rdcounter, isc_uint64_t value, dns_rdatastatstype_t attributes,
	     dns_rdatatypestats_dumper_t dump_fn, void * arg)
{
	dns_rdatatype_t rdtype = dns_rdatatype_none; /* sentinel */
	dns_rdatastatstype_t type;

	if (rdcounter == rdtypecounter_others)
		attributes |= DNS_RDATASTATSTYPE_ATTR_OTHERTYPE;
	else {
		if (rdcounter == rdtypecounter_dlv)
			rdtype = dns_rdatatype_dlv;
		else
			rdtype = (dns_rdatatype_t)rdcounter;
	}
	type = DNS_RDATASTATSTYPE_VALUE((dns_rdatastatstype_t)rdtype,
					attributes);
	dump_fn(type, value, arg);
}

static void
rdatatype_dumpcb(isc_statscounter_t counter, isc_uint64_t value, void *arg) {
	rdatadumparg_t *rdatadumparg = arg;

	dump_rdentry(counter, value, 0, rdatadumparg->fn, rdatadumparg->arg);
}

void
dns_rdatatypestats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
			void *arg0, unsigned int options)
{
	rdatadumparg_t arg;
	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_rdtype);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, rdatatype_dumpcb, &arg, options);
}

static void
rdataset_dumpcb(isc_statscounter_t counter, isc_uint64_t value, void *arg) {
	rdatadumparg_t *rdatadumparg = arg;

	if (counter < rdtypecounter_max) {
		dump_rdentry(counter, value, 0, rdatadumparg->fn,
			     rdatadumparg->arg);
	} else if (counter < rdtypenxcounter_max) {
		dump_rdentry(counter - rdtypecounter_max, value,
			     DNS_RDATASTATSTYPE_ATTR_NXRRSET,
			     rdatadumparg->fn, rdatadumparg->arg);
	} else {
		dump_rdentry(0, value, DNS_RDATASTATSTYPE_ATTR_NXDOMAIN,
			     rdatadumparg->fn, rdatadumparg->arg);
	}
}

void
dns_rdatasetstats_dump(dns_stats_t *stats, dns_rdatatypestats_dumper_t dump_fn,
		       void *arg0, unsigned int options)
{
	rdatadumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) &&
		stats->type == dns_statstype_rdataset);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, rdataset_dumpcb, &arg, options);
}

static void
opcode_dumpcb(isc_statscounter_t counter, isc_uint64_t value, void *arg) {
	opcodedumparg_t *opcodearg = arg;

	opcodearg->fn((dns_opcode_t)counter, value, opcodearg->arg);
}

void
dns_opcodestats_dump(dns_stats_t *stats, dns_opcodestats_dumper_t dump_fn,
		     void *arg0, unsigned int options)
{
	opcodedumparg_t arg;

	REQUIRE(DNS_STATS_VALID(stats) && stats->type == dns_statstype_opcode);

	arg.fn = dump_fn;
	arg.arg = arg0;
	isc_stats_dump(stats->counters, opcode_dumpcb, &arg, options);
}

/***
 *** Obsolete variables and functions follow:
 ***/
LIBDNS_EXTERNAL_DATA const char *dns_statscounter_names[DNS_STATS_NCOUNTERS] =
	{
	"success",
	"referral",
	"nxrrset",
	"nxdomain",
	"recursion",
	"failure",
	"duplicate",
	"dropped"
	};

isc_result_t
dns_stats_alloccounters(isc_mem_t *mctx, isc_uint64_t **ctrp) {
	int i;
	isc_uint64_t *p =
		isc_mem_get(mctx, DNS_STATS_NCOUNTERS * sizeof(isc_uint64_t));
	if (p == NULL)
		return (ISC_R_NOMEMORY);
	for (i = 0; i < DNS_STATS_NCOUNTERS; i++)
		p[i] = 0;
	*ctrp = p;
	return (ISC_R_SUCCESS);
}

void
dns_stats_freecounters(isc_mem_t *mctx, isc_uint64_t **ctrp) {
	isc_mem_put(mctx, *ctrp, DNS_STATS_NCOUNTERS * sizeof(isc_uint64_t));
	*ctrp = NULL;
}
