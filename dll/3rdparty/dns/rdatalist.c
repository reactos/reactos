/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: rdatalist.c,v 1.36 2008/09/24 02:46:22 marka Exp $ */

/*! \file */

#include <config.h>

#include <stddef.h>

#include <isc/util.h>

#include <dns/name.h>
#include <dns/nsec3.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>

#include "rdatalist_p.h"

static dns_rdatasetmethods_t methods = {
	isc__rdatalist_disassociate,
	isc__rdatalist_first,
	isc__rdatalist_next,
	isc__rdatalist_current,
	isc__rdatalist_clone,
	isc__rdatalist_count,
	isc__rdatalist_addnoqname,
	isc__rdatalist_getnoqname,
	isc__rdatalist_addclosest,
	isc__rdatalist_getclosest,
	NULL,
	NULL,
	NULL
};

void
dns_rdatalist_init(dns_rdatalist_t *rdatalist) {

	/*
	 * Initialize rdatalist.
	 */

	rdatalist->rdclass = 0;
	rdatalist->type = 0;
	rdatalist->covers = 0;
	rdatalist->ttl = 0;
	ISC_LIST_INIT(rdatalist->rdata);
	ISC_LINK_INIT(rdatalist, link);
}

isc_result_t
dns_rdatalist_tordataset(dns_rdatalist_t *rdatalist,
			 dns_rdataset_t *rdataset)
{
	/*
	 * Make 'rdataset' refer to the rdata in 'rdatalist'.
	 */

	REQUIRE(rdatalist != NULL);
	REQUIRE(DNS_RDATASET_VALID(rdataset));
	REQUIRE(! dns_rdataset_isassociated(rdataset));

	rdataset->methods = &methods;
	rdataset->rdclass = rdatalist->rdclass;
	rdataset->type = rdatalist->type;
	rdataset->covers = rdatalist->covers;
	rdataset->ttl = rdatalist->ttl;
	rdataset->trust = 0;
	rdataset->private1 = rdatalist;
	rdataset->private2 = NULL;
	rdataset->private3 = NULL;
	rdataset->privateuint4 = 0;
	rdataset->private5 = NULL;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_rdatalist_fromrdataset(dns_rdataset_t *rdataset,
			   dns_rdatalist_t **rdatalist)
{
	REQUIRE(rdatalist != NULL && rdataset != NULL);
	*rdatalist = rdataset->private1;

	return (ISC_R_SUCCESS);
}

void
isc__rdatalist_disassociate(dns_rdataset_t *rdataset) {
	UNUSED(rdataset);
}

isc_result_t
isc__rdatalist_first(dns_rdataset_t *rdataset) {
	dns_rdatalist_t *rdatalist;

	rdatalist = rdataset->private1;
	rdataset->private2 = ISC_LIST_HEAD(rdatalist->rdata);

	if (rdataset->private2 == NULL)
		return (ISC_R_NOMORE);

	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_next(dns_rdataset_t *rdataset) {
	dns_rdata_t *rdata;

	rdata = rdataset->private2;
	if (rdata == NULL)
		return (ISC_R_NOMORE);

	rdataset->private2 = ISC_LIST_NEXT(rdata, link);

	if (rdataset->private2 == NULL)
		return (ISC_R_NOMORE);

	return (ISC_R_SUCCESS);
}

void
isc__rdatalist_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata) {
	dns_rdata_t *list_rdata;

	list_rdata = rdataset->private2;
	INSIST(list_rdata != NULL);

	dns_rdata_clone(list_rdata, rdata);
}

void
isc__rdatalist_clone(dns_rdataset_t *source, dns_rdataset_t *target) {
	*target = *source;

	/*
	 * Reset iterator state.
	 */
	target->private2 = NULL;
}

unsigned int
isc__rdatalist_count(dns_rdataset_t *rdataset) {
	dns_rdatalist_t *rdatalist;
	dns_rdata_t *rdata;
	unsigned int count;

	rdatalist = rdataset->private1;

	count = 0;
	for (rdata = ISC_LIST_HEAD(rdatalist->rdata);
	     rdata != NULL;
	     rdata = ISC_LIST_NEXT(rdata, link))
		count++;

	return (count);
}

isc_result_t
isc__rdatalist_addnoqname(dns_rdataset_t *rdataset, dns_name_t *name) {
	dns_rdataset_t *neg = NULL;
	dns_rdataset_t *negsig = NULL;
	dns_rdataset_t *rdset;
	dns_ttl_t ttl;

	for (rdset = ISC_LIST_HEAD(name->list);
	     rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->rdclass != rdataset->rdclass)
			continue;
		if (rdset->type == dns_rdatatype_nsec ||
		    rdset->type == dns_rdatatype_nsec3)
			neg = rdset;
	}
	if (neg == NULL)
		return (ISC_R_NOTFOUND);

	for (rdset = ISC_LIST_HEAD(name->list);
	     rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->type == dns_rdatatype_rrsig &&
		    rdset->covers == neg->type)
			negsig = rdset;
	}

	if (negsig == NULL)
		return (ISC_R_NOTFOUND);
	/*
	 * Minimise ttl.
	 */
	ttl = rdataset->ttl;
	if (neg->ttl < ttl)
		ttl = neg->ttl;
	if (negsig->ttl < ttl)
		ttl = negsig->ttl;
	rdataset->ttl = neg->ttl = negsig->ttl = ttl;
	rdataset->attributes |= DNS_RDATASETATTR_NOQNAME;
	rdataset->private6 = name;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig)
{
	dns_rdataclass_t rdclass = rdataset->rdclass;
	dns_rdataset_t *tneg = NULL;
	dns_rdataset_t *tnegsig = NULL;
	dns_name_t *noqname = rdataset->private6;

	REQUIRE((rdataset->attributes & DNS_RDATASETATTR_NOQNAME) != 0);
	(void)dns_name_dynamic(noqname);	/* Sanity Check. */

	for (rdataset = ISC_LIST_HEAD(noqname->list);
	     rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->rdclass != rdclass)
			continue;
		if (rdataset->type == dns_rdatatype_nsec ||
		    rdataset->type == dns_rdatatype_nsec3)
			tneg = rdataset;
	}
	if (tneg == NULL)
		return (ISC_R_NOTFOUND);

	for (rdataset = ISC_LIST_HEAD(noqname->list);
	     rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->type == dns_rdatatype_rrsig &&
		    rdataset->covers == tneg->type)
			tnegsig = rdataset;
	}
	if (tnegsig == NULL)
		return (ISC_R_NOTFOUND);

	dns_name_clone(noqname, name);
	dns_rdataset_clone(tneg, neg);
	dns_rdataset_clone(tnegsig, negsig);
	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_addclosest(dns_rdataset_t *rdataset, dns_name_t *name) {
	dns_rdataset_t *neg = NULL;
	dns_rdataset_t *negsig = NULL;
	dns_rdataset_t *rdset;
	dns_ttl_t ttl;

	for (rdset = ISC_LIST_HEAD(name->list);
	     rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->rdclass != rdataset->rdclass)
			continue;
		if (rdset->type == dns_rdatatype_nsec ||
		    rdset->type == dns_rdatatype_nsec3)
			neg = rdset;
	}
	if (neg == NULL)
		return (ISC_R_NOTFOUND);

	for (rdset = ISC_LIST_HEAD(name->list);
	     rdset != NULL;
	     rdset = ISC_LIST_NEXT(rdset, link))
	{
		if (rdset->type == dns_rdatatype_rrsig &&
		    rdset->covers == neg->type)
			negsig = rdset;
	}

	if (negsig == NULL)
		return (ISC_R_NOTFOUND);
	/*
	 * Minimise ttl.
	 */
	ttl = rdataset->ttl;
	if (neg->ttl < ttl)
		ttl = neg->ttl;
	if (negsig->ttl < ttl)
		ttl = negsig->ttl;
	rdataset->ttl = neg->ttl = negsig->ttl = ttl;
	rdataset->attributes |= DNS_RDATASETATTR_CLOSEST;
	rdataset->private7 = name;
	return (ISC_R_SUCCESS);
}

isc_result_t
isc__rdatalist_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
			  dns_rdataset_t *neg, dns_rdataset_t *negsig)
{
	dns_rdataclass_t rdclass = rdataset->rdclass;
	dns_rdataset_t *tneg = NULL;
	dns_rdataset_t *tnegsig = NULL;
	dns_name_t *closest = rdataset->private7;

	REQUIRE((rdataset->attributes & DNS_RDATASETATTR_CLOSEST) != 0);
	(void)dns_name_dynamic(closest);	/* Sanity Check. */

	for (rdataset = ISC_LIST_HEAD(closest->list);
	     rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->rdclass != rdclass)
			continue;
		if (rdataset->type == dns_rdatatype_nsec ||
		    rdataset->type == dns_rdatatype_nsec3)
			tneg = rdataset;
	}
	if (tneg == NULL)
		return (ISC_R_NOTFOUND);

	for (rdataset = ISC_LIST_HEAD(closest->list);
	     rdataset != NULL;
	     rdataset = ISC_LIST_NEXT(rdataset, link))
	{
		if (rdataset->type == dns_rdatatype_rrsig &&
		    rdataset->covers == tneg->type)
			tnegsig = rdataset;
	}
	if (tnegsig == NULL)
		return (ISC_R_NOTFOUND);

	dns_name_clone(closest, name);
	dns_rdataset_clone(tneg, neg);
	dns_rdataset_clone(tnegsig, negsig);
	return (ISC_R_SUCCESS);
}
