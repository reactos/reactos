/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000, 2001, 2003  Internet Software Consortium.
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

/*! \file */
/*
 * $Id: ssu.c,v 1.34 2008/01/18 23:46:58 tbox Exp $
 * Principal Author: Brian Wellington
 */

#include <config.h>

#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/result.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/fixedname.h>
#include <dns/name.h>
#include <dns/ssu.h>

#include <dst/gssapi.h>

#define SSUTABLEMAGIC		ISC_MAGIC('S', 'S', 'U', 'T')
#define VALID_SSUTABLE(table)	ISC_MAGIC_VALID(table, SSUTABLEMAGIC)

#define SSURULEMAGIC		ISC_MAGIC('S', 'S', 'U', 'R')
#define VALID_SSURULE(table)	ISC_MAGIC_VALID(table, SSURULEMAGIC)

struct dns_ssurule {
	unsigned int magic;
	isc_boolean_t grant;	/*%< is this a grant or a deny? */
	unsigned int matchtype;	/*%< which type of pattern match? */
	dns_name_t *identity;	/*%< the identity to match */
	dns_name_t *name;	/*%< the name being updated */
	unsigned int ntypes;	/*%< number of data types covered */
	dns_rdatatype_t *types;	/*%< the data types.  Can include ANY, */
				/*%< defaults to all but SIG,SOA,NS if NULL */
	ISC_LINK(dns_ssurule_t) link;
};

struct dns_ssutable {
	unsigned int magic;
	isc_mem_t *mctx;
	unsigned int references;
	isc_mutex_t lock;
	ISC_LIST(dns_ssurule_t) rules;
};

isc_result_t
dns_ssutable_create(isc_mem_t *mctx, dns_ssutable_t **tablep) {
	isc_result_t result;
	dns_ssutable_t *table;

	REQUIRE(tablep != NULL && *tablep == NULL);
	REQUIRE(mctx != NULL);

	table = isc_mem_get(mctx, sizeof(dns_ssutable_t));
	if (table == NULL)
		return (ISC_R_NOMEMORY);
	result = isc_mutex_init(&table->lock);
	if (result != ISC_R_SUCCESS) {
		isc_mem_put(mctx, table, sizeof(dns_ssutable_t));
		return (result);
	}
	table->references = 1;
	table->mctx = mctx;
	ISC_LIST_INIT(table->rules);
	table->magic = SSUTABLEMAGIC;
	*tablep = table;
	return (ISC_R_SUCCESS);
}

static inline void
destroy(dns_ssutable_t *table) {
	isc_mem_t *mctx;

	REQUIRE(VALID_SSUTABLE(table));

	mctx = table->mctx;
	while (!ISC_LIST_EMPTY(table->rules)) {
		dns_ssurule_t *rule = ISC_LIST_HEAD(table->rules);
		if (rule->identity != NULL) {
			dns_name_free(rule->identity, mctx);
			isc_mem_put(mctx, rule->identity, sizeof(dns_name_t));
		}
		if (rule->name != NULL) {
			dns_name_free(rule->name, mctx);
			isc_mem_put(mctx, rule->name, sizeof(dns_name_t));
		}
		if (rule->types != NULL)
			isc_mem_put(mctx, rule->types,
				    rule->ntypes * sizeof(dns_rdatatype_t));
		ISC_LIST_UNLINK(table->rules, rule, link);
		rule->magic = 0;
		isc_mem_put(mctx, rule, sizeof(dns_ssurule_t));
	}
	DESTROYLOCK(&table->lock);
	table->magic = 0;
	isc_mem_put(mctx, table, sizeof(dns_ssutable_t));
}

void
dns_ssutable_attach(dns_ssutable_t *source, dns_ssutable_t **targetp) {
	REQUIRE(VALID_SSUTABLE(source));
	REQUIRE(targetp != NULL && *targetp == NULL);

	LOCK(&source->lock);

	INSIST(source->references > 0);
	source->references++;
	INSIST(source->references != 0);

	UNLOCK(&source->lock);

	*targetp = source;
}

void
dns_ssutable_detach(dns_ssutable_t **tablep) {
	dns_ssutable_t *table;
	isc_boolean_t done = ISC_FALSE;

	REQUIRE(tablep != NULL);
	table = *tablep;
	REQUIRE(VALID_SSUTABLE(table));

	LOCK(&table->lock);

	INSIST(table->references > 0);
	if (--table->references == 0)
		done = ISC_TRUE;
	UNLOCK(&table->lock);

	*tablep = NULL;

	if (done)
		destroy(table);
}

isc_result_t
dns_ssutable_addrule(dns_ssutable_t *table, isc_boolean_t grant,
		     dns_name_t *identity, unsigned int matchtype,
		     dns_name_t *name, unsigned int ntypes,
		     dns_rdatatype_t *types)
{
	dns_ssurule_t *rule;
	isc_mem_t *mctx;
	isc_result_t result;

	REQUIRE(VALID_SSUTABLE(table));
	REQUIRE(dns_name_isabsolute(identity));
	REQUIRE(dns_name_isabsolute(name));
	REQUIRE(matchtype <= DNS_SSUMATCHTYPE_MAX);
	if (matchtype == DNS_SSUMATCHTYPE_WILDCARD)
		REQUIRE(dns_name_iswildcard(name));
	if (ntypes > 0)
		REQUIRE(types != NULL);

	mctx = table->mctx;
	rule = isc_mem_get(mctx, sizeof(dns_ssurule_t));
	if (rule == NULL)
		return (ISC_R_NOMEMORY);

	rule->identity = NULL;
	rule->name = NULL;
	rule->types = NULL;

	rule->grant = grant;

	rule->identity = isc_mem_get(mctx, sizeof(dns_name_t));
	if (rule->identity == NULL) {
		result = ISC_R_NOMEMORY;
		goto failure;
	}
	dns_name_init(rule->identity, NULL);
	result = dns_name_dup(identity, mctx, rule->identity);
	if (result != ISC_R_SUCCESS)
		goto failure;

	rule->name = isc_mem_get(mctx, sizeof(dns_name_t));
	if (rule->name == NULL) {
		result = ISC_R_NOMEMORY;
		goto failure;
	}
	dns_name_init(rule->name, NULL);
	result = dns_name_dup(name, mctx, rule->name);
	if (result != ISC_R_SUCCESS)
		goto failure;

	rule->matchtype = matchtype;

	rule->ntypes = ntypes;
	if (ntypes > 0) {
		rule->types = isc_mem_get(mctx,
					  ntypes * sizeof(dns_rdatatype_t));
		if (rule->types == NULL) {
			result = ISC_R_NOMEMORY;
			goto failure;
		}
		memcpy(rule->types, types, ntypes * sizeof(dns_rdatatype_t));
	} else
		rule->types = NULL;

	rule->magic = SSURULEMAGIC;
	ISC_LIST_INITANDAPPEND(table->rules, rule, link);

	return (ISC_R_SUCCESS);

 failure:
	if (rule->identity != NULL) {
		if (dns_name_dynamic(rule->identity))
			dns_name_free(rule->identity, mctx);
		isc_mem_put(mctx, rule->identity, sizeof(dns_name_t));
	}
	if (rule->name != NULL) {
		if (dns_name_dynamic(rule->name))
			dns_name_free(rule->name, mctx);
		isc_mem_put(mctx, rule->name, sizeof(dns_name_t));
	}
	if (rule->types != NULL)
		isc_mem_put(mctx, rule->types,
			    ntypes * sizeof(dns_rdatatype_t));
	isc_mem_put(mctx, rule, sizeof(dns_ssurule_t));

	return (result);
}

static inline isc_boolean_t
isusertype(dns_rdatatype_t type) {
	return (ISC_TF(type != dns_rdatatype_ns &&
		       type != dns_rdatatype_soa &&
		       type != dns_rdatatype_rrsig));
}

static void
reverse_from_address(dns_name_t *tcpself, isc_netaddr_t *tcpaddr) {
	char buf[16 * 4 + sizeof("IP6.ARPA.")];
	isc_result_t result;
	unsigned char *ap;
	isc_buffer_t b;
	unsigned long l;

	switch (tcpaddr->family) {
	case AF_INET:
		l = ntohl(tcpaddr->type.in.s_addr);
		result = isc_string_printf(buf, sizeof(buf),
					   "%lu.%lu.%lu.%lu.IN-ADDR.ARPA.",
					   (l >> 0) & 0xff, (l >> 8) & 0xff,
					   (l >> 16) & 0xff, (l >> 24) & 0xff);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		break;
	case AF_INET6:
		ap = tcpaddr->type.in6.s6_addr;
		result = isc_string_printf(buf, sizeof(buf),
					   "%x.%x.%x.%x.%x.%x.%x.%x."
					   "%x.%x.%x.%x.%x.%x.%x.%x."
					   "%x.%x.%x.%x.%x.%x.%x.%x."
					   "%x.%x.%x.%x.%x.%x.%x.%x."
					   "IP6.ARPA.",
					   ap[15] & 0x0f, (ap[15] >> 4) & 0x0f,
					   ap[14] & 0x0f, (ap[14] >> 4) & 0x0f,
					   ap[13] & 0x0f, (ap[13] >> 4) & 0x0f,
					   ap[12] & 0x0f, (ap[12] >> 4) & 0x0f,
					   ap[11] & 0x0f, (ap[11] >> 4) & 0x0f,
					   ap[10] & 0x0f, (ap[10] >> 4) & 0x0f,
					   ap[9] & 0x0f, (ap[9] >> 4) & 0x0f,
					   ap[8] & 0x0f, (ap[8] >> 4) & 0x0f,
					   ap[7] & 0x0f, (ap[7] >> 4) & 0x0f,
					   ap[6] & 0x0f, (ap[6] >> 4) & 0x0f,
					   ap[5] & 0x0f, (ap[5] >> 4) & 0x0f,
					   ap[4] & 0x0f, (ap[4] >> 4) & 0x0f,
					   ap[3] & 0x0f, (ap[3] >> 4) & 0x0f,
					   ap[2] & 0x0f, (ap[2] >> 4) & 0x0f,
					   ap[1] & 0x0f, (ap[1] >> 4) & 0x0f,
					   ap[0] & 0x0f, (ap[0] >> 4) & 0x0f);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		break;
	default:
		INSIST(0);
	}
	isc_buffer_init(&b, buf, strlen(buf));
	isc_buffer_add(&b, strlen(buf));
	result = dns_name_fromtext(tcpself, &b, dns_rootname, 0, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

static void
stf_from_address(dns_name_t *stfself, isc_netaddr_t *tcpaddr) {
	char buf[sizeof("X.X.X.X.Y.Y.Y.Y.2.0.0.2.IP6.ARPA.")];
	isc_result_t result;
	unsigned char *ap;
	isc_buffer_t b;
	unsigned long l;

	switch(tcpaddr->family) {
	case AF_INET:
		l = ntohl(tcpaddr->type.in.s_addr);
		result = isc_string_printf(buf, sizeof(buf),
					   "%lx.%lx.%lx.%lx.%lx.%lx.%lx.%lx"
					   "2.0.0.2.IP6.ARPA.",
					   l & 0xf, (l >> 4) & 0xf,
					   (l >> 8) & 0xf, (l >> 12) & 0xf,
					   (l >> 16) & 0xf, (l >> 20) & 0xf,
					   (l >> 24) & 0xf, (l >> 28) & 0xf);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		break;
	case AF_INET6:
		ap = tcpaddr->type.in6.s6_addr;
		result = isc_string_printf(buf, sizeof(buf),
					   "%x.%x.%x.%x.%x.%x.%x.%x."
					   "%x.%x.%x.%x.IP6.ARPA.",
					   ap[5] & 0x0f, (ap[5] >> 4) & 0x0f,
					   ap[4] & 0x0f, (ap[4] >> 4) & 0x0f,
					   ap[3] & 0x0f, (ap[3] >> 4) & 0x0f,
					   ap[2] & 0x0f, (ap[2] >> 4) & 0x0f,
					   ap[1] & 0x0f, (ap[1] >> 4) & 0x0f,
					   ap[0] & 0x0f, (ap[0] >> 4) & 0x0f);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		break;
	default:
		INSIST(0);
	}
	isc_buffer_init(&b, buf, strlen(buf));
	isc_buffer_add(&b, strlen(buf));
	result = dns_name_fromtext(stfself, &b, dns_rootname, 0, NULL);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
}

isc_boolean_t
dns_ssutable_checkrules(dns_ssutable_t *table, dns_name_t *signer,
			dns_name_t *name, isc_netaddr_t *tcpaddr,
			dns_rdatatype_t type)
{
	dns_ssurule_t *rule;
	unsigned int i;
	dns_fixedname_t fixed;
	dns_name_t *wildcard;
	dns_name_t *tcpself;
	dns_name_t *stfself;
	isc_result_t result;

	REQUIRE(VALID_SSUTABLE(table));
	REQUIRE(signer == NULL || dns_name_isabsolute(signer));
	REQUIRE(dns_name_isabsolute(name));

	if (signer == NULL && tcpaddr == NULL)
		return (ISC_FALSE);

	for (rule = ISC_LIST_HEAD(table->rules);
	     rule != NULL;
	     rule = ISC_LIST_NEXT(rule, link))
	{
		switch (rule->matchtype) {
		case DNS_SSUMATCHTYPE_NAME:
		case DNS_SSUMATCHTYPE_SUBDOMAIN:
		case DNS_SSUMATCHTYPE_WILDCARD:
		case DNS_SSUMATCHTYPE_SELF:
		case DNS_SSUMATCHTYPE_SELFSUB:
		case DNS_SSUMATCHTYPE_SELFWILD:
			if (signer == NULL)
				continue;
			if (dns_name_iswildcard(rule->identity)) {
				if (!dns_name_matcheswildcard(signer,
							      rule->identity))
					continue;
			} else {
				if (!dns_name_equal(signer, rule->identity))
					continue;
			}
			break;
		case DNS_SSUMATCHTYPE_SELFKRB5:
		case DNS_SSUMATCHTYPE_SELFMS:
		case DNS_SSUMATCHTYPE_SUBDOMAINKRB5:
		case DNS_SSUMATCHTYPE_SUBDOMAINMS:
			if (signer == NULL)
				continue;
			break;
		case DNS_SSUMATCHTYPE_TCPSELF:
		case DNS_SSUMATCHTYPE_6TO4SELF:
			if (tcpaddr == NULL)
				continue;
			break;
		}

		switch (rule->matchtype) {
		case DNS_SSUMATCHTYPE_NAME:
			if (!dns_name_equal(name, rule->name))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SUBDOMAIN:
			if (!dns_name_issubdomain(name, rule->name))
				continue;
			break;
		case DNS_SSUMATCHTYPE_WILDCARD:
			if (!dns_name_matcheswildcard(name, rule->name))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SELF:
			if (!dns_name_equal(signer, name))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SELFSUB:
			if (!dns_name_issubdomain(name, signer))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SELFWILD:
			dns_fixedname_init(&fixed);
			wildcard = dns_fixedname_name(&fixed);
			result = dns_name_concatenate(dns_wildcardname, signer,
						      wildcard, NULL);
			if (result != ISC_R_SUCCESS)
				continue;
			if (!dns_name_matcheswildcard(name, wildcard))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SELFKRB5:
			if (!dst_gssapi_identitymatchesrealmkrb5(signer, name,
							       rule->identity))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SELFMS:
			if (!dst_gssapi_identitymatchesrealmms(signer, name,
							       rule->identity))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SUBDOMAINKRB5:
			if (!dns_name_issubdomain(name, rule->name))
				continue;
			if (!dst_gssapi_identitymatchesrealmkrb5(signer, NULL,
							       rule->identity))
				continue;
			break;
		case DNS_SSUMATCHTYPE_SUBDOMAINMS:
			if (!dns_name_issubdomain(name, rule->name))
				continue;
			if (!dst_gssapi_identitymatchesrealmms(signer, NULL,
							       rule->identity))
				continue;
			break;
		case DNS_SSUMATCHTYPE_TCPSELF:
			dns_fixedname_init(&fixed);
			tcpself = dns_fixedname_name(&fixed);
			reverse_from_address(tcpself, tcpaddr);
			if (dns_name_iswildcard(rule->identity)) {
				if (!dns_name_matcheswildcard(tcpself,
							      rule->identity))
					continue;
			} else {
				if (!dns_name_equal(tcpself, rule->identity))
					continue;
			}
			if (!dns_name_equal(tcpself, name))
				continue;
			break;
		case DNS_SSUMATCHTYPE_6TO4SELF:
			dns_fixedname_init(&fixed);
			stfself = dns_fixedname_name(&fixed);
			stf_from_address(stfself, tcpaddr);
			if (dns_name_iswildcard(rule->identity)) {
				if (!dns_name_matcheswildcard(stfself,
							      rule->identity))
					continue;
			} else {
				if (!dns_name_equal(stfself, rule->identity))
					continue;
			}
			if (!dns_name_equal(stfself, name))
				continue;
			break;
		}

		if (rule->ntypes == 0) {
			if (!isusertype(type))
				continue;
		} else {
			for (i = 0; i < rule->ntypes; i++) {
				if (rule->types[i] == dns_rdatatype_any ||
				    rule->types[i] == type)
					break;
			}
			if (i == rule->ntypes)
				continue;
		}
		return (rule->grant);
	}

	return (ISC_FALSE);
}

isc_boolean_t
dns_ssurule_isgrant(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->grant);
}

dns_name_t *
dns_ssurule_identity(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->identity);
}

unsigned int
dns_ssurule_matchtype(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->matchtype);
}

dns_name_t *
dns_ssurule_name(const dns_ssurule_t *rule) {
	REQUIRE(VALID_SSURULE(rule));
	return (rule->name);
}

unsigned int
dns_ssurule_types(const dns_ssurule_t *rule, dns_rdatatype_t **types) {
	REQUIRE(VALID_SSURULE(rule));
	REQUIRE(types != NULL && *types != NULL);
	*types = rule->types;
	return (rule->ntypes);
}

isc_result_t
dns_ssutable_firstrule(const dns_ssutable_t *table, dns_ssurule_t **rule) {
	REQUIRE(VALID_SSUTABLE(table));
	REQUIRE(rule != NULL && *rule == NULL);
	*rule = ISC_LIST_HEAD(table->rules);
	return (*rule != NULL ? ISC_R_SUCCESS : ISC_R_NOMORE);
}

isc_result_t
dns_ssutable_nextrule(dns_ssurule_t *rule, dns_ssurule_t **nextrule) {
	REQUIRE(VALID_SSURULE(rule));
	REQUIRE(nextrule != NULL && *nextrule == NULL);
	*nextrule = ISC_LIST_NEXT(rule, link);
	return (*nextrule != NULL ? ISC_R_SUCCESS : ISC_R_NOMORE);
}
