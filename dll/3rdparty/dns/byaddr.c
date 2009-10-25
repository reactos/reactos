/*
 * Copyright (C) 2004, 2005, 2007  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: byaddr.c,v 1.39 2007/06/19 23:47:16 tbox Exp $ */

/*! \file */

#include <config.h>

#include <isc/mem.h>
#include <isc/netaddr.h>
#include <isc/print.h>
#include <isc/string.h>		/* Required for HP/UX (and others?) */
#include <isc/task.h>
#include <isc/util.h>

#include <dns/byaddr.h>
#include <dns/db.h>
#include <dns/events.h>
#include <dns/lookup.h>
#include <dns/rdata.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/resolver.h>
#include <dns/result.h>
#include <dns/view.h>

/*
 * XXXRTH  We could use a static event...
 */

struct dns_byaddr {
	/* Unlocked. */
	unsigned int		magic;
	isc_mem_t *		mctx;
	isc_mutex_t		lock;
	dns_fixedname_t		name;
	/* Locked by lock. */
	unsigned int		options;
	dns_lookup_t *		lookup;
	isc_task_t *		task;
	dns_byaddrevent_t *	event;
	isc_boolean_t		canceled;
};

#define BYADDR_MAGIC			ISC_MAGIC('B', 'y', 'A', 'd')
#define VALID_BYADDR(b)			ISC_MAGIC_VALID(b, BYADDR_MAGIC)

#define MAX_RESTARTS 16

static char hex_digits[] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

isc_result_t
dns_byaddr_createptrname(isc_netaddr_t *address, isc_boolean_t nibble,
			 dns_name_t *name)
{
	/*
	 * We dropped bitstring labels, so all lookups will use nibbles.
	 */
	UNUSED(nibble);

	return (dns_byaddr_createptrname2(address,
					  DNS_BYADDROPT_IPV6INT, name));
}

isc_result_t
dns_byaddr_createptrname2(isc_netaddr_t *address, unsigned int options,
			  dns_name_t *name)
{
	char textname[128];
	unsigned char *bytes;
	int i;
	char *cp;
	isc_buffer_t buffer;
	unsigned int len;

	REQUIRE(address != NULL);

	/*
	 * We create the text representation and then convert to a
	 * dns_name_t.  This is not maximally efficient, but it keeps all
	 * of the knowledge of wire format in the dns_name_ routines.
	 */

	bytes = (unsigned char *)(&address->type);
	if (address->family == AF_INET) {
		(void)snprintf(textname, sizeof(textname),
			       "%u.%u.%u.%u.in-addr.arpa.",
			       (bytes[3] & 0xff),
			       (bytes[2] & 0xff),
			       (bytes[1] & 0xff),
			       (bytes[0] & 0xff));
	} else if (address->family == AF_INET6) {
		cp = textname;
		for (i = 15; i >= 0; i--) {
			*cp++ = hex_digits[bytes[i] & 0x0f];
			*cp++ = '.';
			*cp++ = hex_digits[(bytes[i] >> 4) & 0x0f];
			*cp++ = '.';
		}
		if ((options & DNS_BYADDROPT_IPV6INT) != 0)
			strcpy(cp, "ip6.int.");
		else
			strcpy(cp, "ip6.arpa.");
	} else
		return (ISC_R_NOTIMPLEMENTED);

	len = (unsigned int)strlen(textname);
	isc_buffer_init(&buffer, textname, len);
	isc_buffer_add(&buffer, len);
	return (dns_name_fromtext(name, &buffer, dns_rootname,
				  ISC_FALSE, NULL));
}

static inline isc_result_t
copy_ptr_targets(dns_byaddr_t *byaddr, dns_rdataset_t *rdataset) {
	isc_result_t result;
	dns_name_t *name;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	/*
	 * The caller must be holding the byaddr's lock.
	 */

	result = dns_rdataset_first(rdataset);
	while (result == ISC_R_SUCCESS) {
		dns_rdata_ptr_t ptr;
		dns_rdataset_current(rdataset, &rdata);
		result = dns_rdata_tostruct(&rdata, &ptr, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);
		name = isc_mem_get(byaddr->mctx, sizeof(*name));
		if (name == NULL) {
			dns_rdata_freestruct(&ptr);
			return (ISC_R_NOMEMORY);
		}
		dns_name_init(name, NULL);
		result = dns_name_dup(&ptr.ptr, byaddr->mctx, name);
		dns_rdata_freestruct(&ptr);
		if (result != ISC_R_SUCCESS) {
			isc_mem_put(byaddr->mctx, name, sizeof(*name));
			return (ISC_R_NOMEMORY);
		}
		ISC_LIST_APPEND(byaddr->event->names, name, link);
		dns_rdata_reset(&rdata);
		result = dns_rdataset_next(rdataset);
	}
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;

	return (result);
}

static void
lookup_done(isc_task_t *task, isc_event_t *event) {
	dns_byaddr_t *byaddr = event->ev_arg;
	dns_lookupevent_t *levent;
	isc_result_t result;

	REQUIRE(event->ev_type == DNS_EVENT_LOOKUPDONE);
	REQUIRE(VALID_BYADDR(byaddr));
	REQUIRE(byaddr->task == task);

	UNUSED(task);

	levent = (dns_lookupevent_t *)event;

	if (levent->result == ISC_R_SUCCESS) {
		result = copy_ptr_targets(byaddr, levent->rdataset);
		byaddr->event->result = result;
	} else
		byaddr->event->result = levent->result;
	isc_event_free(&event);
	isc_task_sendanddetach(&byaddr->task, (isc_event_t **)&byaddr->event);
}

static void
bevent_destroy(isc_event_t *event) {
	dns_byaddrevent_t *bevent;
	dns_name_t *name, *next_name;
	isc_mem_t *mctx;

	REQUIRE(event->ev_type == DNS_EVENT_BYADDRDONE);
	mctx = event->ev_destroy_arg;
	bevent = (dns_byaddrevent_t *)event;

	for (name = ISC_LIST_HEAD(bevent->names);
	     name != NULL;
	     name = next_name) {
		next_name = ISC_LIST_NEXT(name, link);
		ISC_LIST_UNLINK(bevent->names, name, link);
		dns_name_free(name, mctx);
		isc_mem_put(mctx, name, sizeof(*name));
	}
	isc_mem_put(mctx, event, event->ev_size);
}

isc_result_t
dns_byaddr_create(isc_mem_t *mctx, isc_netaddr_t *address, dns_view_t *view,
		  unsigned int options, isc_task_t *task,
		  isc_taskaction_t action, void *arg, dns_byaddr_t **byaddrp)
{
	isc_result_t result;
	dns_byaddr_t *byaddr;
	isc_event_t *ievent;

	byaddr = isc_mem_get(mctx, sizeof(*byaddr));
	if (byaddr == NULL)
		return (ISC_R_NOMEMORY);
	byaddr->mctx = mctx;
	byaddr->options = options;

	byaddr->event = isc_mem_get(mctx, sizeof(*byaddr->event));
	if (byaddr->event == NULL) {
		result = ISC_R_NOMEMORY;
		goto cleanup_byaddr;
	}
	ISC_EVENT_INIT(byaddr->event, sizeof(*byaddr->event), 0, NULL,
		       DNS_EVENT_BYADDRDONE, action, arg, byaddr,
		       bevent_destroy, mctx);
	byaddr->event->result = ISC_R_FAILURE;
	ISC_LIST_INIT(byaddr->event->names);

	byaddr->task = NULL;
	isc_task_attach(task, &byaddr->task);

	result = isc_mutex_init(&byaddr->lock);
	if (result != ISC_R_SUCCESS)
		goto cleanup_event;

	dns_fixedname_init(&byaddr->name);

	result = dns_byaddr_createptrname2(address, options,
					   dns_fixedname_name(&byaddr->name));
	if (result != ISC_R_SUCCESS)
		goto cleanup_lock;

	byaddr->lookup = NULL;
	result = dns_lookup_create(mctx, dns_fixedname_name(&byaddr->name),
				   dns_rdatatype_ptr, view, 0, task,
				   lookup_done, byaddr, &byaddr->lookup);
	if (result != ISC_R_SUCCESS)
		goto cleanup_lock;

	byaddr->canceled = ISC_FALSE;
	byaddr->magic = BYADDR_MAGIC;

	*byaddrp = byaddr;

	return (ISC_R_SUCCESS);

 cleanup_lock:
	DESTROYLOCK(&byaddr->lock);

 cleanup_event:
	ievent = (isc_event_t *)byaddr->event;
	isc_event_free(&ievent);
	byaddr->event = NULL;

	isc_task_detach(&byaddr->task);

 cleanup_byaddr:
	isc_mem_put(mctx, byaddr, sizeof(*byaddr));

	return (result);
}

void
dns_byaddr_cancel(dns_byaddr_t *byaddr) {
	REQUIRE(VALID_BYADDR(byaddr));

	LOCK(&byaddr->lock);

	if (!byaddr->canceled) {
		byaddr->canceled = ISC_TRUE;
		if (byaddr->lookup != NULL)
			dns_lookup_cancel(byaddr->lookup);
	}

	UNLOCK(&byaddr->lock);
}

void
dns_byaddr_destroy(dns_byaddr_t **byaddrp) {
	dns_byaddr_t *byaddr;

	REQUIRE(byaddrp != NULL);
	byaddr = *byaddrp;
	REQUIRE(VALID_BYADDR(byaddr));
	REQUIRE(byaddr->event == NULL);
	REQUIRE(byaddr->task == NULL);
	dns_lookup_destroy(&byaddr->lookup);

	DESTROYLOCK(&byaddr->lock);
	byaddr->magic = 0;
	isc_mem_put(byaddr->mctx, byaddr, sizeof(*byaddr));

	*byaddrp = NULL;
}
