/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: lookup.c,v 1.21 2007/06/18 23:47:40 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/string.h>		/* Required for HP/UX (and others?) */
#include <isc/task.h>
#include <isc/util.h>

#include <dns/db.h>
#include <dns/events.h>
#include <dns/lookup.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/view.h>

struct dns_lookup {
	/* Unlocked. */
	unsigned int		magic;
	isc_mem_t *		mctx;
	isc_mutex_t		lock;
	dns_rdatatype_t		type;
	dns_fixedname_t		name;
	/* Locked by lock. */
	unsigned int		options;
	isc_task_t *		task;
	dns_view_t *		view;
	dns_lookupevent_t *	event;
	dns_fetch_t *		fetch;
	unsigned int		restarts;
	isc_boolean_t		canceled;
	dns_rdataset_t		rdataset;
	dns_rdataset_t		sigrdataset;
};

#define LOOKUP_MAGIC			ISC_MAGIC('l', 'o', 'o', 'k')
#define VALID_LOOKUP(l)			ISC_MAGIC_VALID((l), LOOKUP_MAGIC)

#define MAX_RESTARTS 16

static void lookup_find(dns_lookup_t *lookup, dns_fetchevent_t *event);

static void
fetch_done(isc_task_t *task, isc_event_t *event) {
	dns_lookup_t *lookup = event->ev_arg;
	dns_fetchevent_t *fevent;

	UNUSED(task);
	REQUIRE(event->ev_type == DNS_EVENT_FETCHDONE);
	REQUIRE(VALID_LOOKUP(lookup));
	REQUIRE(lookup->task == task);
	fevent = (dns_fetchevent_t *)event;
	REQUIRE(fevent->fetch == lookup->fetch);

	lookup_find(lookup, fevent);
}

static inline isc_result_t
start_fetch(dns_lookup_t *lookup) {
	isc_result_t result;

	/*
	 * The caller must be holding the lookup's lock.
	 */

	REQUIRE(lookup->fetch == NULL);

	result = dns_resolver_createfetch(lookup->view->resolver,
					  dns_fixedname_name(&lookup->name),
					  lookup->type,
					  NULL, NULL, NULL, 0,
					  lookup->task, fetch_done, lookup,
					  &lookup->rdataset,
					  &lookup->sigrdataset,
					  &lookup->fetch);

	return (result);
}

static isc_result_t
build_event(dns_lookup_t *lookup) {
	dns_name_t *name = NULL;
	dns_rdataset_t *rdataset = NULL;
	dns_rdataset_t *sigrdataset = NULL;
	isc_result_t result;

	name = isc_mem_get(lookup->mctx, sizeof(dns_name_t));
	if (name == NULL) {
		result = ISC_R_NOMEMORY;
		goto fail;
	}
	dns_name_init(name, NULL);
	result = dns_name_dup(dns_fixedname_name(&lookup->name),
			      lookup->mctx, name);
	if (result != ISC_R_SUCCESS)
		goto fail;

	if (dns_rdataset_isassociated(&lookup->rdataset)) {
		rdataset = isc_mem_get(lookup->mctx, sizeof(dns_rdataset_t));
		if (rdataset == NULL) {
			result = ISC_R_NOMEMORY;
			goto fail;
		}
		dns_rdataset_init(rdataset);
		dns_rdataset_clone(&lookup->rdataset, rdataset);
	}

	if (dns_rdataset_isassociated(&lookup->sigrdataset)) {
		sigrdataset = isc_mem_get(lookup->mctx,
					  sizeof(dns_rdataset_t));
		if (sigrdataset == NULL) {
			result = ISC_R_NOMEMORY;
			goto fail;
		}
		dns_rdataset_init(sigrdataset);
		dns_rdataset_clone(&lookup->sigrdataset, sigrdataset);
	}

	lookup->event->name = name;
	lookup->event->rdataset = rdataset;
	lookup->event->sigrdataset = sigrdataset;

	return (ISC_R_SUCCESS);

 fail:
	if (name != NULL) {
		if (dns_name_dynamic(name))
			dns_name_free(name, lookup->mctx);
		isc_mem_put(lookup->mctx, name, sizeof(dns_name_t));
	}
	if (rdataset != NULL) {
		if (dns_rdataset_isassociated(rdataset))
			dns_rdataset_disassociate(rdataset);
		isc_mem_put(lookup->mctx, rdataset, sizeof(dns_rdataset_t));
	}
	return (result);
}

static isc_result_t
view_find(dns_lookup_t *lookup, dns_name_t *foundname) {
	isc_result_t result;
	dns_name_t *name = dns_fixedname_name(&lookup->name);
	dns_rdatatype_t type;

	if (lookup->type == dns_rdatatype_rrsig)
		type = dns_rdatatype_any;
	else
		type = lookup->type;

	result = dns_view_find(lookup->view, name, type, 0, 0, ISC_FALSE,
			       &lookup->event->db, &lookup->event->node,
			       foundname, &lookup->rdataset,
			       &lookup->sigrdataset);
	return (result);
}

static void
lookup_find(dns_lookup_t *lookup, dns_fetchevent_t *event) {
	isc_result_t result;
	isc_boolean_t want_restart;
	isc_boolean_t send_event;
	dns_name_t *name, *fname, *prefix;
	dns_fixedname_t foundname, fixed;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	unsigned int nlabels;
	int order;
	dns_namereln_t namereln;
	dns_rdata_cname_t cname;
	dns_rdata_dname_t dname;

	REQUIRE(VALID_LOOKUP(lookup));

	LOCK(&lookup->lock);

	result = ISC_R_SUCCESS;
	name = dns_fixedname_name(&lookup->name);

	do {
		lookup->restarts++;
		want_restart = ISC_FALSE;
		send_event = ISC_TRUE;

		if (event == NULL && !lookup->canceled) {
			dns_fixedname_init(&foundname);
			fname = dns_fixedname_name(&foundname);
			INSIST(!dns_rdataset_isassociated(&lookup->rdataset));
			INSIST(!dns_rdataset_isassociated
						(&lookup->sigrdataset));
			/*
			 * If we have restarted then clear the old node.				 */
			if  (lookup->event->node != NULL) {
				INSIST(lookup->event->db != NULL);
				dns_db_detachnode(lookup->event->db,
						 &lookup->event->node);
			}
			if (lookup->event->db != NULL)
				dns_db_detach(&lookup->event->db);
			result = view_find(lookup, fname);
			if (result == ISC_R_NOTFOUND) {
				/*
				 * We don't know anything about the name.
				 * Launch a fetch.
				 */
				if  (lookup->event->node != NULL) {
					INSIST(lookup->event->db != NULL);
					dns_db_detachnode(lookup->event->db,
							 &lookup->event->node);
				}
				if (lookup->event->db != NULL)
					dns_db_detach(&lookup->event->db);
				result = start_fetch(lookup);
				if (result == ISC_R_SUCCESS)
					send_event = ISC_FALSE;
				goto done;
			}
		} else if (event != NULL) {
			result = event->result;
			fname = dns_fixedname_name(&event->foundname);
			dns_resolver_destroyfetch(&lookup->fetch);
			INSIST(event->rdataset == &lookup->rdataset);
			INSIST(event->sigrdataset == &lookup->sigrdataset);
		} else
			fname = NULL;	/* Silence compiler warning. */

		/*
		 * If we've been canceled, forget about the result.
		 */
		if (lookup->canceled)
			result = ISC_R_CANCELED;

		switch (result) {
		case ISC_R_SUCCESS:
			result = build_event(lookup);
			if (event == NULL)
				break;
			if (event->db != NULL)
				dns_db_attach(event->db, &lookup->event->db);
			if (event->node != NULL)
				dns_db_attachnode(lookup->event->db,
						  event->node,
						  &lookup->event->node);
			break;
		case DNS_R_CNAME:
			/*
			 * Copy the CNAME's target into the lookup's
			 * query name and start over.
			 */
			result = dns_rdataset_first(&lookup->rdataset);
			if (result != ISC_R_SUCCESS)
				break;
			dns_rdataset_current(&lookup->rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &cname, NULL);
			dns_rdata_reset(&rdata);
			if (result != ISC_R_SUCCESS)
				break;
			result = dns_name_copy(&cname.cname, name, NULL);
			dns_rdata_freestruct(&cname);
			if (result == ISC_R_SUCCESS) {
				want_restart = ISC_TRUE;
				send_event = ISC_FALSE;
			}
			break;
		case DNS_R_DNAME:
			namereln = dns_name_fullcompare(name, fname, &order,
							&nlabels);
			INSIST(namereln == dns_namereln_subdomain);
			/*
			 * Get the target name of the DNAME.
			 */
			result = dns_rdataset_first(&lookup->rdataset);
			if (result != ISC_R_SUCCESS)
				break;
			dns_rdataset_current(&lookup->rdataset, &rdata);
			result = dns_rdata_tostruct(&rdata, &dname, NULL);
			dns_rdata_reset(&rdata);
			if (result != ISC_R_SUCCESS)
				break;
			/*
			 * Construct the new query name and start over.
			 */
			dns_fixedname_init(&fixed);
			prefix = dns_fixedname_name(&fixed);
			dns_name_split(name, nlabels, prefix, NULL);
			result = dns_name_concatenate(prefix, &dname.dname,
						      name, NULL);
			dns_rdata_freestruct(&dname);
			if (result == ISC_R_SUCCESS) {
				want_restart = ISC_TRUE;
				send_event = ISC_FALSE;
			}
			break;
		default:
			send_event = ISC_TRUE;
		}

		if (dns_rdataset_isassociated(&lookup->rdataset))
			dns_rdataset_disassociate(&lookup->rdataset);
		if (dns_rdataset_isassociated(&lookup->sigrdataset))
			dns_rdataset_disassociate(&lookup->sigrdataset);

	done:
		if (event != NULL) {
			if (event->node != NULL)
				dns_db_detachnode(event->db, &event->node);
			if (event->db != NULL)
				dns_db_detach(&event->db);
			isc_event_free(ISC_EVENT_PTR(&event));
		}

		/*
		 * Limit the number of restarts.
		 */
		if (want_restart && lookup->restarts == MAX_RESTARTS) {
			want_restart = ISC_FALSE;
			result = ISC_R_QUOTA;
			send_event = ISC_TRUE;
		}

	} while (want_restart);

	if (send_event) {
		lookup->event->result = result;
		lookup->event->ev_sender = lookup;
		isc_task_sendanddetach(&lookup->task,
				       (isc_event_t **)&lookup->event);
		dns_view_detach(&lookup->view);
	}

	UNLOCK(&lookup->lock);
}

static void
levent_destroy(isc_event_t *event) {
	dns_lookupevent_t *levent;
	isc_mem_t *mctx;
 
	REQUIRE(event->ev_type == DNS_EVENT_LOOKUPDONE);
	mctx = event->ev_destroy_arg;
	levent = (dns_lookupevent_t *)event;

	if (levent->name != NULL) {
		if (dns_name_dynamic(levent->name))
			dns_name_free(levent->name, mctx);
		isc_mem_put(mctx, levent->name, sizeof(dns_name_t));
	}
	if (levent->rdataset != NULL) {
		dns_rdataset_disassociate(levent->rdataset);
		isc_mem_put(mctx, levent->rdataset, sizeof(dns_rdataset_t));
	}
	if (levent->sigrdataset != NULL) {
		dns_rdataset_disassociate(levent->sigrdataset);
		isc_mem_put(mctx, levent->sigrdataset, sizeof(dns_rdataset_t));
	}
	if (levent->node != NULL)
		dns_db_detachnode(levent->db, &levent->node);
	if (levent->db != NULL)
		dns_db_detach(&levent->db);
	isc_mem_put(mctx, event, event->ev_size);
}

isc_result_t
dns_lookup_create(isc_mem_t *mctx, dns_name_t *name, dns_rdatatype_t type,
		  dns_view_t *view, unsigned int options, isc_task_t *task,
		  isc_taskaction_t action, void *arg, dns_lookup_t **lookupp)
{
	isc_result_t result;
	dns_lookup_t *lookup;
	isc_event_t *ievent;

	lookup = isc_mem_get(mctx, sizeof(*lookup));
	if (lookup == NULL)
		return (ISC_R_NOMEMORY);
	lookup->mctx = mctx;
	lookup->options = options;

	ievent = isc_event_allocate(mctx, lookup, DNS_EVENT_LOOKUPDONE,
				    action, arg, sizeof(*lookup->event));
	if (ievent == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_lookup;
	}
	lookup->event = (dns_lookupevent_t *)ievent;
	lookup->event->ev_destroy = levent_destroy;
	lookup->event->ev_destroy_arg = mctx;
	lookup->event->result = ISC_R_FAILURE;
	lookup->event->name = NULL;
	lookup->event->rdataset = NULL;
	lookup->event->sigrdataset = NULL;
	lookup->event->db = NULL;
	lookup->event->node = NULL;

	lookup->task = NULL;
	isc_task_attach(task, &lookup->task);

	result = isc_mutex_init(&lookup->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_event;

	dns_fixedname_init(&lookup->name);

	result = dns_name_copy(name, dns_fixedname_name(&lookup->name), NULL);
	if (result != ISC_R_SUCCESS)
		goto cleanup_lock;

	lookup->type = type;
	lookup->view = NULL;
	dns_view_attach(view, &lookup->view);
	lookup->fetch = NULL;
	lookup->restarts = 0;
	lookup->canceled = ISC_FALSE;
	dns_rdataset_init(&lookup->rdataset);
	dns_rdataset_init(&lookup->sigrdataset);
	lookup->magic = LOOKUP_MAGIC;

	*lookupp = lookup;

	lookup_find(lookup, NULL);

	return (ISC_R_SUCCESS);

 cleanup_lock:
	DESTROYLOCK(&lookup->lock);

 cleanup_event:
	ievent = (isc_event_t *)lookup->event;
	isc_event_free(&ievent);
	lookup->event = NULL;

	isc_task_detach(&lookup->task);

 cleanup_lookup:
	isc_mem_put(mctx, lookup, sizeof(*lookup));

	return (result);
}

void
dns_lookup_cancel(dns_lookup_t *lookup) {
	REQUIRE(VALID_LOOKUP(lookup));

	LOCK(&lookup->lock);

	if (!lookup->canceled) {
		lookup->canceled = ISC_TRUE;
		if (lookup->fetch != NULL) {
			INSIST(lookup->view != NULL);
			dns_resolver_cancelfetch(lookup->fetch);
		}
	}

	UNLOCK(&lookup->lock);
}

void
dns_lookup_destroy(dns_lookup_t **lookupp) {
	dns_lookup_t *lookup;

	REQUIRE(lookupp != NULL);
	lookup = *lookupp;
	REQUIRE(VALID_LOOKUP(lookup));
	REQUIRE(lookup->event == NULL);
	REQUIRE(lookup->task == NULL);
	REQUIRE(lookup->view == NULL);
	if (dns_rdataset_isassociated(&lookup->rdataset))
		dns_rdataset_disassociate(&lookup->rdataset);
	if (dns_rdataset_isassociated(&lookup->sigrdataset))
		dns_rdataset_disassociate(&lookup->sigrdataset);

	DESTROYLOCK(&lookup->lock);
	lookup->magic = 0;
	isc_mem_put(lookup->mctx, lookup, sizeof(*lookup));

	*lookupp = NULL;
}
