/*
 * Copyright (C) 2004, 2005, 2007-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2003  Internet Software Consortium.
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

/* $Id: diff.c,v 1.18.50.2 2009/01/05 23:47:22 tbox Exp $ */

/*! \file */

#include <config.h>

#include <stdlib.h>

#include <isc/buffer.h>
#include <isc/file.h>
#include <isc/mem.h>
#include <isc/string.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/diff.h>
#include <dns/log.h>
#include <dns/rdataclass.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/rdatatype.h>
#include <dns/result.h>

#define CHECK(op) \
	do { result = (op);					\
		if (result != ISC_R_SUCCESS) goto failure;	\
	} while (0)

#define DIFF_COMMON_LOGARGS \
	dns_lctx, DNS_LOGCATEGORY_GENERAL, DNS_LOGMODULE_DIFF

static dns_rdatatype_t
rdata_covers(dns_rdata_t *rdata) {
	return (rdata->type == dns_rdatatype_rrsig ?
		dns_rdata_covers(rdata) : 0);
}

isc_result_t
dns_difftuple_create(isc_mem_t *mctx,
		     dns_diffop_t op, dns_name_t *name, dns_ttl_t ttl,
		     dns_rdata_t *rdata, dns_difftuple_t **tp)
{
	dns_difftuple_t *t;
	unsigned int size;
	unsigned char *datap;

	REQUIRE(tp != NULL && *tp == NULL);

	/*
	 * Create a new tuple.  The variable-size wire-format name data and
	 * rdata immediately follow the dns_difftuple_t structure
	 * in memory.
	 */
	size = sizeof(*t) + name->length + rdata->length;
	t = isc_mem_allocate(mctx, size);
	if (t == NULL)
		return (ISC_R_NOMEMORY);
	t->mctx = mctx;
	t->op = op;

	datap = (unsigned char *)(t + 1);

	memcpy(datap, name->ndata, name->length);
	dns_name_init(&t->name, NULL);
	dns_name_clone(name, &t->name);
	t->name.ndata = datap;
	datap += name->length;

	t->ttl = ttl;

	memcpy(datap, rdata->data, rdata->length);
	dns_rdata_init(&t->rdata);
	dns_rdata_clone(rdata, &t->rdata);
	t->rdata.data = datap;
	datap += rdata->length;

	ISC_LINK_INIT(&t->rdata, link);
	ISC_LINK_INIT(t, link);
	t->magic = DNS_DIFFTUPLE_MAGIC;

	INSIST(datap == (unsigned char *)t + size);

	*tp = t;
	return (ISC_R_SUCCESS);
}

void
dns_difftuple_free(dns_difftuple_t **tp) {
	dns_difftuple_t *t = *tp;
	REQUIRE(DNS_DIFFTUPLE_VALID(t));
	dns_name_invalidate(&t->name);
	t->magic = 0;
	isc_mem_free(t->mctx, t);
	*tp = NULL;
}

isc_result_t
dns_difftuple_copy(dns_difftuple_t *orig, dns_difftuple_t **copyp) {
	return (dns_difftuple_create(orig->mctx, orig->op, &orig->name,
				     orig->ttl, &orig->rdata, copyp));
}

void
dns_diff_init(isc_mem_t *mctx, dns_diff_t *diff) {
	diff->mctx = mctx;
	diff->resign = 0;
	ISC_LIST_INIT(diff->tuples);
	diff->magic = DNS_DIFF_MAGIC;
}

void
dns_diff_clear(dns_diff_t *diff) {
	dns_difftuple_t *t;
	REQUIRE(DNS_DIFF_VALID(diff));
	while ((t = ISC_LIST_HEAD(diff->tuples)) != NULL) {
		ISC_LIST_UNLINK(diff->tuples, t, link);
		dns_difftuple_free(&t);
	}
	ENSURE(ISC_LIST_EMPTY(diff->tuples));
}

void
dns_diff_append(dns_diff_t *diff, dns_difftuple_t **tuplep)
{
	ISC_LIST_APPEND(diff->tuples, *tuplep, link);
	*tuplep = NULL;
}

/* XXX this is O(N) */

void
dns_diff_appendminimal(dns_diff_t *diff, dns_difftuple_t **tuplep)
{
	dns_difftuple_t *ot, *next_ot;

	REQUIRE(DNS_DIFF_VALID(diff));
	REQUIRE(DNS_DIFFTUPLE_VALID(*tuplep));

	/*
	 * Look for an existing tuple with the same owner name,
	 * rdata, and TTL.   If we are doing an addition and find a
	 * deletion or vice versa, remove both the old and the
	 * new tuple since they cancel each other out (assuming
	 * that we never delete nonexistent data or add existing
	 * data).
	 *
	 * If we find an old update of the same kind as
	 * the one we are doing, there must be a programming
	 * error.  We report it but try to continue anyway.
	 */
	for (ot = ISC_LIST_HEAD(diff->tuples); ot != NULL;
	     ot = next_ot)
	{
		next_ot = ISC_LIST_NEXT(ot, link);
		if (dns_name_equal(&ot->name, &(*tuplep)->name) &&
		    dns_rdata_compare(&ot->rdata, &(*tuplep)->rdata) == 0 &&
		    ot->ttl == (*tuplep)->ttl)
		{
			ISC_LIST_UNLINK(diff->tuples, ot, link);
			if ((*tuplep)->op == ot->op) {
				UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "unexpected non-minimal diff");
			} else {
				dns_difftuple_free(tuplep);
			}
			dns_difftuple_free(&ot);
			break;
		}
	}

	if (*tuplep != NULL) {
		ISC_LIST_APPEND(diff->tuples, *tuplep, link);
		*tuplep = NULL;
	}

	ENSURE(*tuplep == NULL);
}

static isc_stdtime_t
setresign(dns_rdataset_t *modified, isc_uint32_t delta) {
	dns_rdata_t rdata = DNS_RDATA_INIT;
	dns_rdata_rrsig_t sig;
	isc_stdtime_t when;
	isc_result_t result;

	result = dns_rdataset_first(modified);
	INSIST(result == ISC_R_SUCCESS);
	dns_rdataset_current(modified, &rdata);
	(void)dns_rdata_tostruct(&rdata, &sig, NULL);
	if ((rdata.flags & DNS_RDATA_OFFLINE) != 0)
		when = 0;
	else
		when = sig.timeexpire - delta;
	dns_rdata_reset(&rdata);

	result = dns_rdataset_next(modified);
	while (result == ISC_R_SUCCESS) {
		dns_rdataset_current(modified, &rdata);
		(void)dns_rdata_tostruct(&rdata, &sig, NULL);
		if ((rdata.flags & DNS_RDATA_OFFLINE) != 0) {
			goto next_rr;
		}
		if (when == 0 || sig.timeexpire - delta < when)
			when = sig.timeexpire - delta;
 next_rr:
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(modified);
	}
	INSIST(result == ISC_R_NOMORE);
	return (when);
}

static isc_result_t
diff_apply(dns_diff_t *diff, dns_db_t *db, dns_dbversion_t *ver,
	   isc_boolean_t warn)
{
	dns_difftuple_t *t;
	dns_dbnode_t *node = NULL;
	isc_result_t result;
	char namebuf[DNS_NAME_FORMATSIZE];
	char typebuf[DNS_RDATATYPE_FORMATSIZE];
	char classbuf[DNS_RDATACLASS_FORMATSIZE];

	REQUIRE(DNS_DIFF_VALID(diff));
	REQUIRE(DNS_DB_VALID(db));

	t = ISC_LIST_HEAD(diff->tuples);
	while (t != NULL) {
		dns_name_t *name;

		INSIST(node == NULL);
		name = &t->name;
		/*
		 * Find the node.
		 * We create the node if it does not exist.
		 * This will cause an empty node to be created if the diff
		 * contains a deletion of an RR at a nonexistent name,
		 * but such diffs should never be created in the first
		 * place.
		 */

		while (t != NULL && dns_name_equal(&t->name, name)) {
			dns_rdatatype_t type, covers;
			dns_diffop_t op;
			dns_rdatalist_t rdl;
			dns_rdataset_t rds;
			dns_rdataset_t ardataset;
			dns_rdataset_t *modified = NULL;
			isc_boolean_t offline;

			op = t->op;
			type = t->rdata.type;
			covers = rdata_covers(&t->rdata);

			/*
			 * Collect a contiguous set of updates with
			 * the same operation (add/delete) and RR type
			 * into a single rdatalist so that the
			 * database rrset merging/subtraction code
			 * can work more efficiently than if each
			 * RR were merged into / subtracted from
			 * the database separately.
			 *
			 * This is done by linking rdata structures from the
			 * diff into "rdatalist".  This uses the rdata link
			 * field, not the diff link field, so the structure
			 * of the diff itself is not affected.
			 */

			rdl.type = type;
			rdl.covers = covers;
			rdl.rdclass = t->rdata.rdclass;
			rdl.ttl = t->ttl;
			ISC_LIST_INIT(rdl.rdata);
			ISC_LINK_INIT(&rdl, link);

			node = NULL;
			if (type != dns_rdatatype_nsec3 &&
			    covers != dns_rdatatype_nsec3)
				CHECK(dns_db_findnode(db, name, ISC_TRUE,
						      &node));
			else
				CHECK(dns_db_findnsec3node(db, name, ISC_TRUE,
							   &node));

			offline = ISC_FALSE;
			while (t != NULL &&
			       dns_name_equal(&t->name, name) &&
			       t->op == op &&
			       t->rdata.type == type &&
			       rdata_covers(&t->rdata) == covers)
			{
				dns_name_format(name, namebuf, sizeof(namebuf));
				dns_rdatatype_format(t->rdata.type, typebuf,
						     sizeof(typebuf));
				dns_rdataclass_format(t->rdata.rdclass,
						      classbuf,
						      sizeof(classbuf));
				if (t->ttl != rdl.ttl && warn)
					isc_log_write(DIFF_COMMON_LOGARGS,
						ISC_LOG_WARNING,
						"'%s/%s/%s': TTL differs in "
						"rdataset, adjusting "
						"%lu -> %lu",
						namebuf, typebuf, classbuf,
						(unsigned long) t->ttl,
						(unsigned long) rdl.ttl);
				if (t->rdata.flags & DNS_RDATA_OFFLINE)
					offline = ISC_TRUE;
				ISC_LIST_APPEND(rdl.rdata, &t->rdata, link);
				t = ISC_LIST_NEXT(t, link);
			}

			/*
			 * Convert the rdatalist into a rdataset.
			 */
			dns_rdataset_init(&rds);
			CHECK(dns_rdatalist_tordataset(&rdl, &rds));
			if (rds.type == dns_rdatatype_rrsig)
				switch (op) {
				case DNS_DIFFOP_ADDRESIGN:
				case DNS_DIFFOP_DELRESIGN:
					modified = &ardataset;
					dns_rdataset_init(modified);
					break;
				default:
					break;
				}
			rds.trust = dns_trust_ultimate;

			/*
			 * Merge the rdataset into the database.
			 */
			switch (op) {
			case DNS_DIFFOP_ADD:
			case DNS_DIFFOP_ADDRESIGN:
				result = dns_db_addrdataset(db, node, ver,
							    0, &rds,
							    DNS_DBADD_MERGE|
							    DNS_DBADD_EXACT|
							    DNS_DBADD_EXACTTTL,
							    modified);
				break;
			case DNS_DIFFOP_DEL:
			case DNS_DIFFOP_DELRESIGN:
				result = dns_db_subtractrdataset(db, node, ver,
							       &rds,
							       DNS_DBSUB_EXACT,
							       modified);
				break;
			default:
				INSIST(0);
			}

			if (result == ISC_R_SUCCESS) {
				if (modified != NULL) {
					isc_stdtime_t resign;
					resign = setresign(modified,
							   diff->resign);
					dns_db_setsigningtime(db, modified,
							      resign);
				}
			} else if (result == DNS_R_UNCHANGED) {
				/*
				 * This will not happen when executing a
				 * dynamic update, because that code will
				 * generate strictly minimal diffs.
				 * It may happen when receiving an IXFR
				 * from a server that is not as careful.
				 * Issue a warning and continue.
				 */
				if (warn)
					isc_log_write(DIFF_COMMON_LOGARGS,
						      ISC_LOG_WARNING,
						      "update with no effect");
			} else if (result == DNS_R_NXRRSET) {
				/*
				 * OK.
				 */
			} else {
				if (modified != NULL &&
				    dns_rdataset_isassociated(modified))
					dns_rdataset_disassociate(modified);
				CHECK(result);
			}
			dns_db_detachnode(db, &node);
			if (modified != NULL &&
			    dns_rdataset_isassociated(modified))
				dns_rdataset_disassociate(modified);
		}
	}
	return (ISC_R_SUCCESS);

 failure:
	if (node != NULL)
		dns_db_detachnode(db, &node);
	return (result);
}

isc_result_t
dns_diff_apply(dns_diff_t *diff, dns_db_t *db, dns_dbversion_t *ver) {
	return (diff_apply(diff, db, ver, ISC_TRUE));
}

isc_result_t
dns_diff_applysilently(dns_diff_t *diff, dns_db_t *db, dns_dbversion_t *ver) {
	return (diff_apply(diff, db, ver, ISC_FALSE));
}

/* XXX this duplicates lots of code in diff_apply(). */

isc_result_t
dns_diff_load(dns_diff_t *diff, dns_addrdatasetfunc_t addfunc,
	      void *add_private)
{
	dns_difftuple_t *t;
	isc_result_t result;

	REQUIRE(DNS_DIFF_VALID(diff));

	t = ISC_LIST_HEAD(diff->tuples);
	while (t != NULL) {
		dns_name_t *name;

		name = &t->name;
		while (t != NULL && dns_name_equal(&t->name, name)) {
			dns_rdatatype_t type, covers;
			dns_diffop_t op;
			dns_rdatalist_t rdl;
			dns_rdataset_t rds;

			op = t->op;
			type = t->rdata.type;
			covers = rdata_covers(&t->rdata);

			rdl.type = type;
			rdl.covers = covers;
			rdl.rdclass = t->rdata.rdclass;
			rdl.ttl = t->ttl;
			ISC_LIST_INIT(rdl.rdata);
			ISC_LINK_INIT(&rdl, link);

			while (t != NULL && dns_name_equal(&t->name, name) &&
			       t->op == op && t->rdata.type == type &&
			       rdata_covers(&t->rdata) == covers)
			{
				ISC_LIST_APPEND(rdl.rdata, &t->rdata, link);
				t = ISC_LIST_NEXT(t, link);
			}

			/*
			 * Convert the rdatalist into a rdataset.
			 */
			dns_rdataset_init(&rds);
			CHECK(dns_rdatalist_tordataset(&rdl, &rds));
			rds.trust = dns_trust_ultimate;

			INSIST(op == DNS_DIFFOP_ADD);
			result = (*addfunc)(add_private, name, &rds);
			if (result == DNS_R_UNCHANGED) {
				isc_log_write(DIFF_COMMON_LOGARGS,
					      ISC_LOG_WARNING,
					      "update with no effect");
			} else if (result == ISC_R_SUCCESS ||
				   result == DNS_R_NXRRSET) {
				/*
				 * OK.
				 */
			} else {
				CHECK(result);
			}
		}
	}
	result = ISC_R_SUCCESS;
 failure:
	return (result);
}

/*
 * XXX uses qsort(); a merge sort would be more natural for lists,
 * and perhaps safer wrt thread stack overflow.
 */
isc_result_t
dns_diff_sort(dns_diff_t *diff, dns_diff_compare_func *compare) {
	unsigned int length = 0;
	unsigned int i;
	dns_difftuple_t **v;
	dns_difftuple_t *p;
	REQUIRE(DNS_DIFF_VALID(diff));

	for (p = ISC_LIST_HEAD(diff->tuples);
	     p != NULL;
	     p = ISC_LIST_NEXT(p, link))
		length++;
	if (length == 0)
		return (ISC_R_SUCCESS);
	v = isc_mem_get(diff->mctx, length * sizeof(dns_difftuple_t *));
	if (v == NULL)
		return (ISC_R_NOMEMORY);
	i = 0;
	for (i = 0; i < length; i++) {
		p = ISC_LIST_HEAD(diff->tuples);
		v[i] = p;
		ISC_LIST_UNLINK(diff->tuples, p, link);
	}
	INSIST(ISC_LIST_HEAD(diff->tuples) == NULL);
	qsort(v, length, sizeof(v[0]), compare);
	for (i = 0; i < length; i++) {
		ISC_LIST_APPEND(diff->tuples, v[i], link);
	}
	isc_mem_put(diff->mctx, v, length * sizeof(dns_difftuple_t *));
	return (ISC_R_SUCCESS);
}


/*
 * Create an rdataset containing the single RR of the given
 * tuple.  The caller must allocate the rdata, rdataset and
 * an rdatalist structure for it to refer to.
 */

static isc_result_t
diff_tuple_tordataset(dns_difftuple_t *t, dns_rdata_t *rdata,
		      dns_rdatalist_t *rdl, dns_rdataset_t *rds)
{
	REQUIRE(DNS_DIFFTUPLE_VALID(t));
	REQUIRE(rdl != NULL);
	REQUIRE(rds != NULL);

	rdl->type = t->rdata.type;
	rdl->rdclass = t->rdata.rdclass;
	rdl->ttl = t->ttl;
	ISC_LIST_INIT(rdl->rdata);
	ISC_LINK_INIT(rdl, link);
	dns_rdataset_init(rds);
	ISC_LINK_INIT(rdata, link);
	dns_rdata_clone(&t->rdata, rdata);
	ISC_LIST_APPEND(rdl->rdata, rdata, link);
	return (dns_rdatalist_tordataset(rdl, rds));
}

isc_result_t
dns_diff_print(dns_diff_t *diff, FILE *file) {
	isc_result_t result;
	dns_difftuple_t *t;
	char *mem = NULL;
	unsigned int size = 2048;
	const char *op = NULL;

	REQUIRE(DNS_DIFF_VALID(diff));

	mem = isc_mem_get(diff->mctx, size);
	if (mem == NULL)
		return (ISC_R_NOMEMORY);

	for (t = ISC_LIST_HEAD(diff->tuples); t != NULL;
	     t = ISC_LIST_NEXT(t, link))
	{
		isc_buffer_t buf;
		isc_region_t r;

		dns_rdatalist_t rdl;
		dns_rdataset_t rds;
		dns_rdata_t rd = DNS_RDATA_INIT;

		result = diff_tuple_tordataset(t, &rd, &rdl, &rds);
		if (result != ISC_R_SUCCESS) {
			UNEXPECTED_ERROR(__FILE__, __LINE__,
					 "diff_tuple_tordataset failed: %s",
					 dns_result_totext(result));
			result =  ISC_R_UNEXPECTED;
			goto cleanup;
		}
 again:
		isc_buffer_init(&buf, mem, size);
		result = dns_rdataset_totext(&rds, &t->name,
					     ISC_FALSE, ISC_FALSE, &buf);

		if (result == ISC_R_NOSPACE) {
			isc_mem_put(diff->mctx, mem, size);
			size += 1024;
			mem = isc_mem_get(diff->mctx, size);
			if (mem == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}
			goto again;
		}

		if (result != ISC_R_SUCCESS)
			goto cleanup;
		/*
		 * Get rid of final newline.
		 */
		INSIST(buf.used >= 1 &&
		       ((char *) buf.base)[buf.used-1] == '\n');
		buf.used--;

		isc_buffer_usedregion(&buf, &r);
		switch (t->op) {
		case DNS_DIFFOP_EXISTS: op = "exists"; break;
		case DNS_DIFFOP_ADD: op = "add"; break;
		case DNS_DIFFOP_DEL: op = "del"; break;
		case DNS_DIFFOP_ADDRESIGN: op = "add re-sign"; break;
		case DNS_DIFFOP_DELRESIGN: op = "del re-sign"; break;
		}
		if (file != NULL)
			fprintf(file, "%s %.*s\n", op, (int) r.length,
				(char *) r.base);
		else
			isc_log_write(DIFF_COMMON_LOGARGS, ISC_LOG_DEBUG(7),
				      "%s %.*s", op, (int) r.length,
				      (char *) r.base);
	}
	result = ISC_R_SUCCESS;
 cleanup:
	if (mem != NULL)
		isc_mem_put(diff->mctx, mem, size);
	return (result);
}
