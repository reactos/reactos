/*
 * Copyright (C) 2004, 2005, 2007, 2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
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

/* $Id: rootns.c,v 1.36 2008/09/24 02:46:22 marka Exp $ */

/*! \file */

#include <config.h>

#include <isc/buffer.h>
#include <isc/string.h>		/* Required for HP/UX (and others?) */
#include <isc/util.h>

#include <dns/callbacks.h>
#include <dns/db.h>
#include <dns/dbiterator.h>
#include <dns/fixedname.h>
#include <dns/log.h>
#include <dns/master.h>
#include <dns/rdata.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatasetiter.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>
#include <dns/rootns.h>
#include <dns/view.h>

static char root_ns[] =
";\n"
"; Internet Root Nameservers\n"
";\n"
"$TTL 518400\n"
".                       518400  IN      NS      A.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      B.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      C.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      D.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      E.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      F.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      G.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      H.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      I.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      J.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      K.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      L.ROOT-SERVERS.NET.\n"
".                       518400  IN      NS      M.ROOT-SERVERS.NET.\n"
"A.ROOT-SERVERS.NET.     3600000 IN      A       198.41.0.4\n"
"A.ROOT-SERVERS.NET.     3600000 IN      AAAA    2001:503:BA3E::2:30\n"
"B.ROOT-SERVERS.NET.     3600000 IN      A       192.228.79.201\n"
"C.ROOT-SERVERS.NET.     3600000 IN      A       192.33.4.12\n"
"D.ROOT-SERVERS.NET.     3600000 IN      A       128.8.10.90\n"
"E.ROOT-SERVERS.NET.     3600000 IN      A       192.203.230.10\n"
"F.ROOT-SERVERS.NET.     3600000 IN      A       192.5.5.241\n"
"F.ROOT-SERVERS.NET.     3600000 IN      AAAA    2001:500:2F::F\n"
"G.ROOT-SERVERS.NET.     3600000 IN      A       192.112.36.4\n"
"H.ROOT-SERVERS.NET.     3600000 IN      A       128.63.2.53\n"
"H.ROOT-SERVERS.NET.     3600000 IN      AAAA    2001:500:1::803F:235\n"
"I.ROOT-SERVERS.NET.     3600000 IN      A       192.36.148.17\n"
"J.ROOT-SERVERS.NET.     3600000 IN      A       192.58.128.30\n"
"J.ROOT-SERVERS.NET.     3600000 IN      AAAA    2001:503:C27::2:30\n"
"K.ROOT-SERVERS.NET.     3600000 IN      A       193.0.14.129\n"
"K.ROOT-SERVERS.NET.     3600000 IN      AAAA    2001:7FD::1\n"
"L.ROOT-SERVERS.NET.     3600000 IN      A       199.7.83.42\n"
"M.ROOT-SERVERS.NET.     3600000 IN      A       202.12.27.33\n"
"M.ROOT-SERVERS.NET.     3600000 IN      AAAA    2001:DC3::35\n";

static isc_result_t
in_rootns(dns_rdataset_t *rootns, dns_name_t *name) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_ns_t ns;

	if (!dns_rdataset_isassociated(rootns))
		return (ISC_R_NOTFOUND);

	result = dns_rdataset_first(rootns);
	while (result == ISC_R_SUCCESS) {
		dns_rdataset_current(rootns, &rdata);
		result = dns_rdata_tostruct(&rdata, &ns, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);
		if (dns_name_compare(name, &ns.name) == 0)
			return (ISC_R_SUCCESS);
		result = dns_rdataset_next(rootns);
		dns_rdata_reset(&rdata);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_NOTFOUND;
	return (result);
}

static isc_result_t
check_node(dns_rdataset_t *rootns, dns_name_t *name,
	   dns_rdatasetiter_t *rdsiter) {
	isc_result_t result;
	dns_rdataset_t rdataset;

	dns_rdataset_init(&rdataset);
	result = dns_rdatasetiter_first(rdsiter);
	while (result == ISC_R_SUCCESS) {
		dns_rdatasetiter_current(rdsiter, &rdataset);
		switch (rdataset.type) {
		case dns_rdatatype_a:
		case dns_rdatatype_aaaa:
			result = in_rootns(rootns, name);
			if (result != ISC_R_SUCCESS)
				goto cleanup;
			break;
		case dns_rdatatype_ns:
			if (dns_name_compare(name, dns_rootname) == 0)
				break;
			/*FALLTHROUGH*/
		default:
			result = ISC_R_FAILURE;
			goto cleanup;
		}
		dns_rdataset_disassociate(&rdataset);
		result = dns_rdatasetiter_next(rdsiter);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
 cleanup:
	if (dns_rdataset_isassociated(&rdataset))
		dns_rdataset_disassociate(&rdataset);
	return (result);
}

static isc_result_t
check_hints(dns_db_t *db) {
	isc_result_t result;
	dns_rdataset_t rootns;
	dns_dbiterator_t *dbiter = NULL;
	dns_dbnode_t *node = NULL;
	isc_stdtime_t now;
	dns_fixedname_t fixname;
	dns_name_t *name;
	dns_rdatasetiter_t *rdsiter = NULL;

	isc_stdtime_get(&now);

	dns_fixedname_init(&fixname);
	name = dns_fixedname_name(&fixname);

	dns_rdataset_init(&rootns);
	(void)dns_db_find(db, dns_rootname, NULL, dns_rdatatype_ns, 0,
			  now, NULL, name, &rootns, NULL);
	result = dns_db_createiterator(db, 0, &dbiter);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_dbiterator_first(dbiter);
	while (result == ISC_R_SUCCESS) {
		result = dns_dbiterator_current(dbiter, &node, name);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		result = dns_db_allrdatasets(db, node, NULL, now, &rdsiter);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		result = check_node(&rootns, name, rdsiter);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		dns_rdatasetiter_destroy(&rdsiter);
		dns_db_detachnode(db, &node);
		result = dns_dbiterator_next(dbiter);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

 cleanup:
	if (dns_rdataset_isassociated(&rootns))
		dns_rdataset_disassociate(&rootns);
	if (rdsiter != NULL)
		dns_rdatasetiter_destroy(&rdsiter);
	if (node != NULL)
		dns_db_detachnode(db, &node);
	if (dbiter != NULL)
		dns_dbiterator_destroy(&dbiter);
	return (result);
}

isc_result_t
dns_rootns_create(isc_mem_t *mctx, dns_rdataclass_t rdclass,
		  const char *filename, dns_db_t **target)
{
	isc_result_t result, eresult;
	isc_buffer_t source;
	size_t len;
	dns_rdatacallbacks_t callbacks;
	dns_db_t *db = NULL;

	REQUIRE(target != NULL && *target == NULL);

	result = dns_db_create(mctx, "rbt", dns_rootname, dns_dbtype_zone,
			       rdclass, 0, NULL, &db);
	if (result != ISC_R_SUCCESS)
		return (result);

	dns_rdatacallbacks_init(&callbacks);

	len = strlen(root_ns);
	isc_buffer_init(&source, root_ns, len);
	isc_buffer_add(&source, len);

	result = dns_db_beginload(db, &callbacks.add,
				  &callbacks.add_private);
	if (result != ISC_R_SUCCESS)
		return (result);
	if (filename != NULL) {
		/*
		 * Load the hints from the specified filename.
		 */
		result = dns_master_loadfile(filename, &db->origin,
					     &db->origin, db->rdclass,
					     DNS_MASTER_HINT,
					     &callbacks, db->mctx);
	} else if (rdclass == dns_rdataclass_in) {
		/*
		 * Default to using the Internet root servers.
		 */
		result = dns_master_loadbuffer(&source, &db->origin,
					       &db->origin, db->rdclass,
					       DNS_MASTER_HINT,
					       &callbacks, db->mctx);
	} else
		result = ISC_R_NOTFOUND;
	eresult = dns_db_endload(db, &callbacks.add_private);
	if (result == ISC_R_SUCCESS || result == DNS_R_SEENINCLUDE)
		result = eresult;
	if (result != ISC_R_SUCCESS && result != DNS_R_SEENINCLUDE)
		goto db_detach;
	if (check_hints(db) != ISC_R_SUCCESS)
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_HINTS, ISC_LOG_WARNING,
			      "extra data in root hints '%s'",
			      (filename != NULL) ? filename : "<BUILT-IN>");
	*target = db;
	return (ISC_R_SUCCESS);

 db_detach:
	dns_db_detach(&db);

	return (result);
}

static void
report(dns_view_t *view, dns_name_t *name, isc_boolean_t missing,
       dns_rdata_t *rdata)
{
	const char *viewname = "", *sep = "";
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char databuf[sizeof("xxxx:xxxx:xxxx:xxxx:xxxx:xxxx:123.123.123.123")];
	isc_buffer_t buffer;
	isc_result_t result;

	if (strcmp(view->name, "_bind") != 0 &&
	    strcmp(view->name, "_default") != 0) {
		viewname = view->name;
		sep = ": view ";
	}

	dns_name_format(name, namebuf, sizeof(namebuf));
	dns_rdatatype_format(rdata->type, typebuf, sizeof(typebuf));
	isc_buffer_init(&buffer, databuf, sizeof(databuf) - 1);
	result = dns_rdata_totext(rdata, NULL, &buffer);
	RUNTIME_CHECK(result == ISC_R_SUCCESS);
	databuf[isc_buffer_usedlength(&buffer)] = '\0';

	if (missing)
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_HINTS, ISC_LOG_WARNING,
			      "checkhints%s%s: %s/%s (%s) missing from hints",
			      sep, viewname, namebuf, typebuf, databuf);
	else
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_HINTS, ISC_LOG_WARNING,
			      "checkhints%s%s: %s/%s (%s) extra record "
			      "in hints", sep, viewname, namebuf, typebuf,
			      databuf);
}

static isc_boolean_t
inrrset(dns_rdataset_t *rrset, dns_rdata_t *rdata) {
	isc_result_t result;
	dns_rdata_t current = DNS_RDATA_INIT;

	result = dns_rdataset_first(rrset);
	while (result == ISC_R_SUCCESS) {
		dns_rdataset_current(rrset, &current);
		if (dns_rdata_compare(rdata, &current) == 0)
			return (ISC_TRUE);
		dns_rdata_reset(&current);
		result = dns_rdataset_next(rrset);
	}
	return (ISC_FALSE);
}

/*
 * Check that the address RRsets match.
 *
 * Note we don't complain about missing glue records.
 */

static void
check_address_records(dns_view_t *view, dns_db_t *hints, dns_db_t *db,
		      dns_name_t *name, isc_stdtime_t now)
{
	isc_result_t hresult, rresult, result;
	dns_rdataset_t hintrrset, rootrrset;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_name_t *foundname;
	dns_fixedname_t fixed;

	dns_rdataset_init(&hintrrset);
	dns_rdataset_init(&rootrrset);
	dns_fixedname_init(&fixed);
	foundname = dns_fixedname_name(&fixed);

	hresult = dns_db_find(hints, name, NULL, dns_rdatatype_a, 0,
			      now, NULL, foundname, &hintrrset, NULL);
	rresult = dns_db_find(db, name, NULL, dns_rdatatype_a,
			      DNS_DBFIND_GLUEOK, now, NULL, foundname,
			      &rootrrset, NULL);
	if (hresult == ISC_R_SUCCESS &&
	    (rresult == ISC_R_SUCCESS || rresult == DNS_R_GLUE)) {
		result = dns_rdataset_first(&rootrrset);
		while (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&rdata);
			dns_rdataset_current(&rootrrset, &rdata);
			if (!inrrset(&hintrrset, &rdata))
				report(view, name, ISC_TRUE, &rdata);
			result = dns_rdataset_next(&rootrrset);
		}
		result = dns_rdataset_first(&hintrrset);
		while (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&rdata);
			dns_rdataset_current(&hintrrset, &rdata);
			if (!inrrset(&rootrrset, &rdata))
				report(view, name, ISC_FALSE, &rdata);
			result = dns_rdataset_next(&hintrrset);
		}
	}
	if (hresult == ISC_R_NOTFOUND &&
	    (rresult == ISC_R_SUCCESS || rresult == DNS_R_GLUE)) {
		result = dns_rdataset_first(&rootrrset);
		while (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&rdata);
			dns_rdataset_current(&rootrrset, &rdata);
			report(view, name, ISC_TRUE, &rdata);
			result = dns_rdataset_next(&rootrrset);
		}
	}
	if (dns_rdataset_isassociated(&rootrrset))
		dns_rdataset_disassociate(&rootrrset);
	if (dns_rdataset_isassociated(&hintrrset))
		dns_rdataset_disassociate(&hintrrset);

	/*
	 * Check AAAA records.
	 */
	hresult = dns_db_find(hints, name, NULL, dns_rdatatype_aaaa, 0,
			      now, NULL, foundname, &hintrrset, NULL);
	rresult = dns_db_find(db, name, NULL, dns_rdatatype_aaaa,
			      DNS_DBFIND_GLUEOK, now, NULL, foundname,
			      &rootrrset, NULL);
	if (hresult == ISC_R_SUCCESS &&
	    (rresult == ISC_R_SUCCESS || rresult == DNS_R_GLUE)) {
		result = dns_rdataset_first(&rootrrset);
		while (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&rdata);
			dns_rdataset_current(&rootrrset, &rdata);
			if (!inrrset(&hintrrset, &rdata))
				report(view, name, ISC_TRUE, &rdata);
			dns_rdata_reset(&rdata);
			result = dns_rdataset_next(&rootrrset);
		}
		result = dns_rdataset_first(&hintrrset);
		while (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&rdata);
			dns_rdataset_current(&hintrrset, &rdata);
			if (!inrrset(&rootrrset, &rdata))
				report(view, name, ISC_FALSE, &rdata);
			dns_rdata_reset(&rdata);
			result = dns_rdataset_next(&hintrrset);
		}
	}
	if (hresult == ISC_R_NOTFOUND &&
	    (rresult == ISC_R_SUCCESS || rresult == DNS_R_GLUE)) {
		result = dns_rdataset_first(&rootrrset);
		while (result == ISC_R_SUCCESS) {
			dns_rdata_reset(&rdata);
			dns_rdataset_current(&rootrrset, &rdata);
			report(view, name, ISC_TRUE, &rdata);
			dns_rdata_reset(&rdata);
			result = dns_rdataset_next(&rootrrset);
		}
	}
	if (dns_rdataset_isassociated(&rootrrset))
		dns_rdataset_disassociate(&rootrrset);
	if (dns_rdataset_isassociated(&hintrrset))
		dns_rdataset_disassociate(&hintrrset);
}

void
dns_root_checkhints(dns_view_t *view, dns_db_t *hints, dns_db_t *db) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_ns_t ns;
	dns_rdataset_t hintns, rootns;
	const char *viewname = "", *sep = "";
	isc_stdtime_t now;
	dns_name_t *name;
	dns_fixedname_t fixed;

	REQUIRE(hints != NULL);
	REQUIRE(db != NULL);
	REQUIRE(view != NULL);

	isc_stdtime_get(&now);

	if (strcmp(view->name, "_bind") != 0 &&
	    strcmp(view->name, "_default") != 0) {
		viewname = view->name;
		sep = ": view ";
	}

	dns_rdataset_init(&hintns);
	dns_rdataset_init(&rootns);
	dns_fixedname_init(&fixed);
	name = dns_fixedname_name(&fixed);

	result = dns_db_find(hints, dns_rootname, NULL, dns_rdatatype_ns, 0,
			     now, NULL, name, &hintns, NULL);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_HINTS, ISC_LOG_WARNING,
			      "checkhints%s%s: unable to get root NS rrset "
			      "from hints: %s", sep, viewname,
			      dns_result_totext(result));
		goto cleanup;
	}

	result = dns_db_find(db, dns_rootname, NULL, dns_rdatatype_ns, 0,
			     now, NULL, name, &rootns, NULL);
	if (result != ISC_R_SUCCESS) {
		isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_HINTS, ISC_LOG_WARNING,
			      "checkhints%s%s: unable to get root NS rrset "
			      "from cache: %s", sep, viewname,
			      dns_result_totext(result));
		goto cleanup;
	}

	/*
	 * Look for missing root NS names.
	 */
	result = dns_rdataset_first(&rootns);
	while (result == ISC_R_SUCCESS) {
		dns_rdataset_current(&rootns, &rdata);
		result = dns_rdata_tostruct(&rdata, &ns, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		result = in_rootns(&hintns, &ns.name);
		if (result != ISC_R_SUCCESS) {
			char namebuf[DNS_NAME_FORMATSIZE];
			/* missing from hints */
			dns_name_format(&ns.name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_HINTS, ISC_LOG_WARNING,
				      "checkhints%s%s: unable to find root "
				      "NS '%s' in hints", sep, viewname,
				      namebuf);
		} else
			check_address_records(view, hints, db, &ns.name, now);
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(&rootns);
	}
	if (result != ISC_R_NOMORE) {
		goto cleanup;
	}

	/*
	 * Look for extra root NS names.
	 */
	result = dns_rdataset_first(&hintns);
	while (result == ISC_R_SUCCESS) {
		dns_rdataset_current(&hintns, &rdata);
		result = dns_rdata_tostruct(&rdata, &ns, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		result = in_rootns(&rootns, &ns.name);
		if (result != ISC_R_SUCCESS) {
			char namebuf[DNS_NAME_FORMATSIZE];
			/* extra entry in hints */
			dns_name_format(&ns.name, namebuf, sizeof(namebuf));
			isc_log_write(dns_lctx, DNS_LOGCATEGORY_GENERAL,
				      DNS_LOGMODULE_HINTS, ISC_LOG_WARNING,
				      "checkhints%s%s: extra NS '%s' in hints",
				      sep, viewname, namebuf);
		}
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(&hintns);
	}
	if (result != ISC_R_NOMORE) {
		goto cleanup;
	}

 cleanup:
	if (dns_rdataset_isassociated(&rootns))
		dns_rdataset_disassociate(&rootns);
	if (dns_rdataset_isassociated(&hintns))
		dns_rdataset_disassociate(&hintns);
}
