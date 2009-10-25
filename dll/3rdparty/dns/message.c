/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2003  Internet Software Consortium.
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

/* $Id: message.c,v 1.245.50.2 2009/01/18 23:47:40 tbox Exp $ */

/*! \file */

/***
 *** Imports
 ***/

#include <config.h>
#include <ctype.h>

#include <isc/buffer.h>
#include <isc/mem.h>
#include <isc/print.h>
#include <isc/string.h>		/* Required for HP/UX (and others?) */
#include <isc/util.h>

#include <dns/dnssec.h>
#include <dns/keyvalues.h>
#include <dns/log.h>
#include <dns/masterdump.h>
#include <dns/message.h>
#include <dns/opcode.h>
#include <dns/rdata.h>
#include <dns/rdatalist.h>
#include <dns/rdataset.h>
#include <dns/rdatastruct.h>
#include <dns/result.h>
#include <dns/tsig.h>
#include <dns/view.h>

#ifdef SKAN_MSG_DEBUG
static void
hexdump(const char *msg, const char *msg2, void *base, size_t len) {
	unsigned char *p;
	unsigned int cnt;

	p = base;
	cnt = 0;

	printf("*** %s [%s] (%u bytes @ %p)\n", msg, msg2, len, base);

	while (cnt < len) {
		if (cnt % 16 == 0)
			printf("%p: ", p);
		else if (cnt % 8 == 0)
			printf(" |");
		printf(" %02x %c", *p, (isprint(*p) ? *p : ' '));
		p++;
		cnt++;

		if (cnt % 16 == 0)
			printf("\n");
	}

	if (cnt % 16 != 0)
		printf("\n");
}
#endif

#define DNS_MESSAGE_OPCODE_MASK		0x7800U
#define DNS_MESSAGE_OPCODE_SHIFT	11
#define DNS_MESSAGE_RCODE_MASK		0x000fU
#define DNS_MESSAGE_FLAG_MASK		0x8ff0U
#define DNS_MESSAGE_EDNSRCODE_MASK	0xff000000U
#define DNS_MESSAGE_EDNSRCODE_SHIFT	24
#define DNS_MESSAGE_EDNSVERSION_MASK	0x00ff0000U
#define DNS_MESSAGE_EDNSVERSION_SHIFT	16

#define VALID_NAMED_SECTION(s)  (((s) > DNS_SECTION_ANY) \
				 && ((s) < DNS_SECTION_MAX))
#define VALID_SECTION(s)	(((s) >= DNS_SECTION_ANY) \
				 && ((s) < DNS_SECTION_MAX))
#define ADD_STRING(b, s)	{if (strlen(s) >= \
				   isc_buffer_availablelength(b)) \
				       return(ISC_R_NOSPACE); else \
				       isc_buffer_putstr(b, s);}
#define VALID_PSEUDOSECTION(s)	(((s) >= DNS_PSEUDOSECTION_ANY) \
				 && ((s) < DNS_PSEUDOSECTION_MAX))

#define OPTOUT(x) (((x)->attributes & DNS_RDATASETATTR_OPTOUT) != 0)

/*%
 * This is the size of each individual scratchpad buffer, and the numbers
 * of various block allocations used within the server.
 * XXXMLG These should come from a config setting.
 */
#define SCRATCHPAD_SIZE		512
#define NAME_COUNT		  8
#define OFFSET_COUNT		  4
#define RDATA_COUNT		  8
#define RDATALIST_COUNT		  8
#define RDATASET_COUNT		 RDATALIST_COUNT

/*%
 * Text representation of the different items, for message_totext
 * functions.
 */
static const char *sectiontext[] = {
	"QUESTION",
	"ANSWER",
	"AUTHORITY",
	"ADDITIONAL"
};

static const char *updsectiontext[] = {
	"ZONE",
	"PREREQUISITE",
	"UPDATE",
	"ADDITIONAL"
};

static const char *opcodetext[] = {
	"QUERY",
	"IQUERY",
	"STATUS",
	"RESERVED3",
	"NOTIFY",
	"UPDATE",
	"RESERVED6",
	"RESERVED7",
	"RESERVED8",
	"RESERVED9",
	"RESERVED10",
	"RESERVED11",
	"RESERVED12",
	"RESERVED13",
	"RESERVED14",
	"RESERVED15"
};

static const char *rcodetext[] = {
	"NOERROR",
	"FORMERR",
	"SERVFAIL",
	"NXDOMAIN",
	"NOTIMP",
	"REFUSED",
	"YXDOMAIN",
	"YXRRSET",
	"NXRRSET",
	"NOTAUTH",
	"NOTZONE",
	"RESERVED11",
	"RESERVED12",
	"RESERVED13",
	"RESERVED14",
	"RESERVED15",
	"BADVERS"
};


/*%
 * "helper" type, which consists of a block of some type, and is linkable.
 * For it to work, sizeof(dns_msgblock_t) must be a multiple of the pointer
 * size, or the allocated elements will not be aligned correctly.
 */
struct dns_msgblock {
	unsigned int			count;
	unsigned int			remaining;
	ISC_LINK(dns_msgblock_t)	link;
}; /* dynamically sized */

static inline dns_msgblock_t *
msgblock_allocate(isc_mem_t *, unsigned int, unsigned int);

#define msgblock_get(block, type) \
	((type *)msgblock_internalget(block, sizeof(type)))

static inline void *
msgblock_internalget(dns_msgblock_t *, unsigned int);

static inline void
msgblock_reset(dns_msgblock_t *);

static inline void
msgblock_free(isc_mem_t *, dns_msgblock_t *, unsigned int);

/*
 * Allocate a new dns_msgblock_t, and return a pointer to it.  If no memory
 * is free, return NULL.
 */
static inline dns_msgblock_t *
msgblock_allocate(isc_mem_t *mctx, unsigned int sizeof_type,
		  unsigned int count)
{
	dns_msgblock_t *block;
	unsigned int length;

	length = sizeof(dns_msgblock_t) + (sizeof_type * count);

	block = isc_mem_get(mctx, length);
	if (block == NULL)
		return (NULL);

	block->count = count;
	block->remaining = count;

	ISC_LINK_INIT(block, link);

	return (block);
}

/*
 * Return an element from the msgblock.  If no more are available, return
 * NULL.
 */
static inline void *
msgblock_internalget(dns_msgblock_t *block, unsigned int sizeof_type) {
	void *ptr;

	if (block == NULL || block->remaining == 0)
		return (NULL);

	block->remaining--;

	ptr = (((unsigned char *)block)
	       + sizeof(dns_msgblock_t)
	       + (sizeof_type * block->remaining));

	return (ptr);
}

static inline void
msgblock_reset(dns_msgblock_t *block) {
	block->remaining = block->count;
}

/*
 * Release memory associated with a message block.
 */
static inline void
msgblock_free(isc_mem_t *mctx, dns_msgblock_t *block, unsigned int sizeof_type)
{
	unsigned int length;

	length = sizeof(dns_msgblock_t) + (sizeof_type * block->count);

	isc_mem_put(mctx, block, length);
}

/*
 * Allocate a new dynamic buffer, and attach it to this message as the
 * "current" buffer.  (which is always the last on the list, for our
 * uses)
 */
static inline isc_result_t
newbuffer(dns_message_t *msg, unsigned int size) {
	isc_result_t result;
	isc_buffer_t *dynbuf;

	dynbuf = NULL;
	result = isc_buffer_allocate(msg->mctx, &dynbuf, size);
	if (result != ISC_R_SUCCESS)
		return (ISC_R_NOMEMORY);

	ISC_LIST_APPEND(msg->scratchpad, dynbuf, link);
	return (ISC_R_SUCCESS);
}

static inline isc_buffer_t *
currentbuffer(dns_message_t *msg) {
	isc_buffer_t *dynbuf;

	dynbuf = ISC_LIST_TAIL(msg->scratchpad);
	INSIST(dynbuf != NULL);

	return (dynbuf);
}

static inline void
releaserdata(dns_message_t *msg, dns_rdata_t *rdata) {
	ISC_LIST_PREPEND(msg->freerdata, rdata, link);
}

static inline dns_rdata_t *
newrdata(dns_message_t *msg) {
	dns_msgblock_t *msgblock;
	dns_rdata_t *rdata;

	rdata = ISC_LIST_HEAD(msg->freerdata);
	if (rdata != NULL) {
		ISC_LIST_UNLINK(msg->freerdata, rdata, link);
		return (rdata);
	}

	msgblock = ISC_LIST_TAIL(msg->rdatas);
	rdata = msgblock_get(msgblock, dns_rdata_t);
	if (rdata == NULL) {
		msgblock = msgblock_allocate(msg->mctx, sizeof(dns_rdata_t),
					     RDATA_COUNT);
		if (msgblock == NULL)
			return (NULL);

		ISC_LIST_APPEND(msg->rdatas, msgblock, link);

		rdata = msgblock_get(msgblock, dns_rdata_t);
	}

	dns_rdata_init(rdata);
	return (rdata);
}

static inline void
releaserdatalist(dns_message_t *msg, dns_rdatalist_t *rdatalist) {
	ISC_LIST_PREPEND(msg->freerdatalist, rdatalist, link);
}

static inline dns_rdatalist_t *
newrdatalist(dns_message_t *msg) {
	dns_msgblock_t *msgblock;
	dns_rdatalist_t *rdatalist;

	rdatalist = ISC_LIST_HEAD(msg->freerdatalist);
	if (rdatalist != NULL) {
		ISC_LIST_UNLINK(msg->freerdatalist, rdatalist, link);
		return (rdatalist);
	}

	msgblock = ISC_LIST_TAIL(msg->rdatalists);
	rdatalist = msgblock_get(msgblock, dns_rdatalist_t);
	if (rdatalist == NULL) {
		msgblock = msgblock_allocate(msg->mctx,
					     sizeof(dns_rdatalist_t),
					     RDATALIST_COUNT);
		if (msgblock == NULL)
			return (NULL);

		ISC_LIST_APPEND(msg->rdatalists, msgblock, link);

		rdatalist = msgblock_get(msgblock, dns_rdatalist_t);
	}

	return (rdatalist);
}

static inline dns_offsets_t *
newoffsets(dns_message_t *msg) {
	dns_msgblock_t *msgblock;
	dns_offsets_t *offsets;

	msgblock = ISC_LIST_TAIL(msg->offsets);
	offsets = msgblock_get(msgblock, dns_offsets_t);
	if (offsets == NULL) {
		msgblock = msgblock_allocate(msg->mctx,
					     sizeof(dns_offsets_t),
					     OFFSET_COUNT);
		if (msgblock == NULL)
			return (NULL);

		ISC_LIST_APPEND(msg->offsets, msgblock, link);

		offsets = msgblock_get(msgblock, dns_offsets_t);
	}

	return (offsets);
}

static inline void
msginitheader(dns_message_t *m) {
	m->id = 0;
	m->flags = 0;
	m->rcode = 0;
	m->opcode = 0;
	m->rdclass = 0;
}

static inline void
msginitprivate(dns_message_t *m) {
	unsigned int i;

	for (i = 0; i < DNS_SECTION_MAX; i++) {
		m->cursors[i] = NULL;
		m->counts[i] = 0;
	}
	m->opt = NULL;
	m->sig0 = NULL;
	m->sig0name = NULL;
	m->tsig = NULL;
	m->tsigname = NULL;
	m->state = DNS_SECTION_ANY;  /* indicate nothing parsed or rendered */
	m->opt_reserved = 0;
	m->sig_reserved = 0;
	m->reserved = 0;
	m->buffer = NULL;
}

static inline void
msginittsig(dns_message_t *m) {
	m->tsigstatus = dns_rcode_noerror;
	m->querytsigstatus = dns_rcode_noerror;
	m->tsigkey = NULL;
	m->tsigctx = NULL;
	m->sigstart = -1;
	m->sig0key = NULL;
	m->sig0status = dns_rcode_noerror;
	m->timeadjust = 0;
}

/*
 * Init elements to default state.  Used both when allocating a new element
 * and when resetting one.
 */
static inline void
msginit(dns_message_t *m) {
	msginitheader(m);
	msginitprivate(m);
	msginittsig(m);
	m->header_ok = 0;
	m->question_ok = 0;
	m->tcp_continuation = 0;
	m->verified_sig = 0;
	m->verify_attempted = 0;
	m->order = NULL;
	m->order_arg = NULL;
	m->query.base = NULL;
	m->query.length = 0;
	m->free_query = 0;
	m->saved.base = NULL;
	m->saved.length = 0;
	m->free_saved = 0;
	m->querytsig = NULL;
}

static inline void
msgresetnames(dns_message_t *msg, unsigned int first_section) {
	unsigned int i;
	dns_name_t *name, *next_name;
	dns_rdataset_t *rds, *next_rds;

	/*
	 * Clean up name lists by calling the rdataset disassociate function.
	 */
	for (i = first_section; i < DNS_SECTION_MAX; i++) {
		name = ISC_LIST_HEAD(msg->sections[i]);
		while (name != NULL) {
			next_name = ISC_LIST_NEXT(name, link);
			ISC_LIST_UNLINK(msg->sections[i], name, link);

			rds = ISC_LIST_HEAD(name->list);
			while (rds != NULL) {
				next_rds = ISC_LIST_NEXT(rds, link);
				ISC_LIST_UNLINK(name->list, rds, link);

				INSIST(dns_rdataset_isassociated(rds));
				dns_rdataset_disassociate(rds);
				isc_mempool_put(msg->rdspool, rds);
				rds = next_rds;
			}
			if (dns_name_dynamic(name))
				dns_name_free(name, msg->mctx);
			isc_mempool_put(msg->namepool, name);
			name = next_name;
		}
	}
}

static void
msgresetopt(dns_message_t *msg)
{
	if (msg->opt != NULL) {
		if (msg->opt_reserved > 0) {
			dns_message_renderrelease(msg, msg->opt_reserved);
			msg->opt_reserved = 0;
		}
		INSIST(dns_rdataset_isassociated(msg->opt));
		dns_rdataset_disassociate(msg->opt);
		isc_mempool_put(msg->rdspool, msg->opt);
		msg->opt = NULL;
	}
}

static void
msgresetsigs(dns_message_t *msg, isc_boolean_t replying) {
	if (msg->sig_reserved > 0) {
		dns_message_renderrelease(msg, msg->sig_reserved);
		msg->sig_reserved = 0;
	}
	if (msg->tsig != NULL) {
		INSIST(dns_rdataset_isassociated(msg->tsig));
		INSIST(msg->namepool != NULL);
		if (replying) {
			INSIST(msg->querytsig == NULL);
			msg->querytsig = msg->tsig;
		} else {
			dns_rdataset_disassociate(msg->tsig);
			isc_mempool_put(msg->rdspool, msg->tsig);
			if (msg->querytsig != NULL) {
				dns_rdataset_disassociate(msg->querytsig);
				isc_mempool_put(msg->rdspool, msg->querytsig);
			}
		}
		if (dns_name_dynamic(msg->tsigname))
			dns_name_free(msg->tsigname, msg->mctx);
		isc_mempool_put(msg->namepool, msg->tsigname);
		msg->tsig = NULL;
		msg->tsigname = NULL;
	} else if (msg->querytsig != NULL && !replying) {
		dns_rdataset_disassociate(msg->querytsig);
		isc_mempool_put(msg->rdspool, msg->querytsig);
		msg->querytsig = NULL;
	}
	if (msg->sig0 != NULL) {
		INSIST(dns_rdataset_isassociated(msg->sig0));
		dns_rdataset_disassociate(msg->sig0);
		isc_mempool_put(msg->rdspool, msg->sig0);
		if (msg->sig0name != NULL) {
			if (dns_name_dynamic(msg->sig0name))
				dns_name_free(msg->sig0name, msg->mctx);
			isc_mempool_put(msg->namepool, msg->sig0name);
		}
		msg->sig0 = NULL;
		msg->sig0name = NULL;
	}
}

/*
 * Free all but one (or everything) for this message.  This is used by
 * both dns_message_reset() and dns_message_destroy().
 */
static void
msgreset(dns_message_t *msg, isc_boolean_t everything) {
	dns_msgblock_t *msgblock, *next_msgblock;
	isc_buffer_t *dynbuf, *next_dynbuf;
	dns_rdata_t *rdata;
	dns_rdatalist_t *rdatalist;

	msgresetnames(msg, 0);
	msgresetopt(msg);
	msgresetsigs(msg, ISC_FALSE);

	/*
	 * Clean up linked lists.
	 */

	/*
	 * Run through the free lists, and just unlink anything found there.
	 * The memory isn't lost since these are part of message blocks we
	 * have allocated.
	 */
	rdata = ISC_LIST_HEAD(msg->freerdata);
	while (rdata != NULL) {
		ISC_LIST_UNLINK(msg->freerdata, rdata, link);
		rdata = ISC_LIST_HEAD(msg->freerdata);
	}
	rdatalist = ISC_LIST_HEAD(msg->freerdatalist);
	while (rdatalist != NULL) {
		ISC_LIST_UNLINK(msg->freerdatalist, rdatalist, link);
		rdatalist = ISC_LIST_HEAD(msg->freerdatalist);
	}

	dynbuf = ISC_LIST_HEAD(msg->scratchpad);
	INSIST(dynbuf != NULL);
	if (!everything) {
		isc_buffer_clear(dynbuf);
		dynbuf = ISC_LIST_NEXT(dynbuf, link);
	}
	while (dynbuf != NULL) {
		next_dynbuf = ISC_LIST_NEXT(dynbuf, link);
		ISC_LIST_UNLINK(msg->scratchpad, dynbuf, link);
		isc_buffer_free(&dynbuf);
		dynbuf = next_dynbuf;
	}

	msgblock = ISC_LIST_HEAD(msg->rdatas);
	if (!everything && msgblock != NULL) {
		msgblock_reset(msgblock);
		msgblock = ISC_LIST_NEXT(msgblock, link);
	}
	while (msgblock != NULL) {
		next_msgblock = ISC_LIST_NEXT(msgblock, link);
		ISC_LIST_UNLINK(msg->rdatas, msgblock, link);
		msgblock_free(msg->mctx, msgblock, sizeof(dns_rdata_t));
		msgblock = next_msgblock;
	}

	/*
	 * rdatalists could be empty.
	 */

	msgblock = ISC_LIST_HEAD(msg->rdatalists);
	if (!everything && msgblock != NULL) {
		msgblock_reset(msgblock);
		msgblock = ISC_LIST_NEXT(msgblock, link);
	}
	while (msgblock != NULL) {
		next_msgblock = ISC_LIST_NEXT(msgblock, link);
		ISC_LIST_UNLINK(msg->rdatalists, msgblock, link);
		msgblock_free(msg->mctx, msgblock, sizeof(dns_rdatalist_t));
		msgblock = next_msgblock;
	}

	msgblock = ISC_LIST_HEAD(msg->offsets);
	if (!everything && msgblock != NULL) {
		msgblock_reset(msgblock);
		msgblock = ISC_LIST_NEXT(msgblock, link);
	}
	while (msgblock != NULL) {
		next_msgblock = ISC_LIST_NEXT(msgblock, link);
		ISC_LIST_UNLINK(msg->offsets, msgblock, link);
		msgblock_free(msg->mctx, msgblock, sizeof(dns_offsets_t));
		msgblock = next_msgblock;
	}

	if (msg->tsigkey != NULL) {
		dns_tsigkey_detach(&msg->tsigkey);
		msg->tsigkey = NULL;
	}

	if (msg->tsigctx != NULL)
		dst_context_destroy(&msg->tsigctx);

	if (msg->query.base != NULL) {
		if (msg->free_query != 0)
			isc_mem_put(msg->mctx, msg->query.base,
				    msg->query.length);
		msg->query.base = NULL;
		msg->query.length = 0;
	}

	if (msg->saved.base != NULL) {
		if (msg->free_saved != 0)
			isc_mem_put(msg->mctx, msg->saved.base,
				    msg->saved.length);
		msg->saved.base = NULL;
		msg->saved.length = 0;
	}

	/*
	 * cleanup the buffer cleanup list
	 */
	dynbuf = ISC_LIST_HEAD(msg->cleanup);
	while (dynbuf != NULL) {
		next_dynbuf = ISC_LIST_NEXT(dynbuf, link);
		ISC_LIST_UNLINK(msg->cleanup, dynbuf, link);
		isc_buffer_free(&dynbuf);
		dynbuf = next_dynbuf;
	}

	/*
	 * Set other bits to normal default values.
	 */
	if (!everything)
		msginit(msg);

	ENSURE(isc_mempool_getallocated(msg->namepool) == 0);
	ENSURE(isc_mempool_getallocated(msg->rdspool) == 0);
}

static unsigned int
spacefortsig(dns_tsigkey_t *key, int otherlen) {
	isc_region_t r1, r2;
	unsigned int x;
	isc_result_t result;

	/*
	 * The space required for an TSIG record is:
	 *
	 *	n1 bytes for the name
	 *	2 bytes for the type
	 *	2 bytes for the class
	 *	4 bytes for the ttl
	 *	2 bytes for the rdlength
	 *	n2 bytes for the algorithm name
	 *	6 bytes for the time signed
	 *	2 bytes for the fudge
	 *	2 bytes for the MAC size
	 *	x bytes for the MAC
	 *	2 bytes for the original id
	 *	2 bytes for the error
	 *	2 bytes for the other data length
	 *	y bytes for the other data (at most)
	 * ---------------------------------
	 *     26 + n1 + n2 + x + y bytes
	 */

	dns_name_toregion(&key->name, &r1);
	dns_name_toregion(key->algorithm, &r2);
	if (key->key == NULL)
		x = 0;
	else {
		result = dst_key_sigsize(key->key, &x);
		if (result != ISC_R_SUCCESS)
			x = 0;
	}
	return (26 + r1.length + r2.length + x + otherlen);
}

isc_result_t
dns_message_create(isc_mem_t *mctx, unsigned int intent, dns_message_t **msgp)
{
	dns_message_t *m;
	isc_result_t result;
	isc_buffer_t *dynbuf;
	unsigned int i;

	REQUIRE(mctx != NULL);
	REQUIRE(msgp != NULL);
	REQUIRE(*msgp == NULL);
	REQUIRE(intent == DNS_MESSAGE_INTENTPARSE
		|| intent == DNS_MESSAGE_INTENTRENDER);

	m = isc_mem_get(mctx, sizeof(dns_message_t));
	if (m == NULL)
		return (ISC_R_NOMEMORY);

	/*
	 * No allocations until further notice.  Just initialize all lists
	 * and other members that are freed in the cleanup phase here.
	 */

	m->magic = DNS_MESSAGE_MAGIC;
	m->from_to_wire = intent;
	msginit(m);

	for (i = 0; i < DNS_SECTION_MAX; i++)
		ISC_LIST_INIT(m->sections[i]);
	m->mctx = mctx;

	ISC_LIST_INIT(m->scratchpad);
	ISC_LIST_INIT(m->cleanup);
	m->namepool = NULL;
	m->rdspool = NULL;
	ISC_LIST_INIT(m->rdatas);
	ISC_LIST_INIT(m->rdatalists);
	ISC_LIST_INIT(m->offsets);
	ISC_LIST_INIT(m->freerdata);
	ISC_LIST_INIT(m->freerdatalist);

	/*
	 * Ok, it is safe to allocate (and then "goto cleanup" if failure)
	 */

	result = isc_mempool_create(m->mctx, sizeof(dns_name_t), &m->namepool);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	isc_mempool_setfreemax(m->namepool, NAME_COUNT);
	isc_mempool_setname(m->namepool, "msg:names");

	result = isc_mempool_create(m->mctx, sizeof(dns_rdataset_t),
				    &m->rdspool);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	isc_mempool_setfreemax(m->rdspool, NAME_COUNT);
	isc_mempool_setname(m->rdspool, "msg:rdataset");

	dynbuf = NULL;
	result = isc_buffer_allocate(mctx, &dynbuf, SCRATCHPAD_SIZE);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	ISC_LIST_APPEND(m->scratchpad, dynbuf, link);

	m->cctx = NULL;

	*msgp = m;
	return (ISC_R_SUCCESS);

	/*
	 * Cleanup for error returns.
	 */
 cleanup:
	dynbuf = ISC_LIST_HEAD(m->scratchpad);
	if (dynbuf != NULL) {
		ISC_LIST_UNLINK(m->scratchpad, dynbuf, link);
		isc_buffer_free(&dynbuf);
	}
	if (m->namepool != NULL)
		isc_mempool_destroy(&m->namepool);
	if (m->rdspool != NULL)
		isc_mempool_destroy(&m->rdspool);
	m->magic = 0;
	isc_mem_put(mctx, m, sizeof(dns_message_t));

	return (ISC_R_NOMEMORY);
}

void
dns_message_reset(dns_message_t *msg, unsigned int intent) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(intent == DNS_MESSAGE_INTENTPARSE
		|| intent == DNS_MESSAGE_INTENTRENDER);

	msgreset(msg, ISC_FALSE);
	msg->from_to_wire = intent;
}

void
dns_message_destroy(dns_message_t **msgp) {
	dns_message_t *msg;

	REQUIRE(msgp != NULL);
	REQUIRE(DNS_MESSAGE_VALID(*msgp));

	msg = *msgp;
	*msgp = NULL;

	msgreset(msg, ISC_TRUE);
	isc_mempool_destroy(&msg->namepool);
	isc_mempool_destroy(&msg->rdspool);
	msg->magic = 0;
	isc_mem_put(msg->mctx, msg, sizeof(dns_message_t));
}

static isc_result_t
findname(dns_name_t **foundname, dns_name_t *target,
	 dns_namelist_t *section)
{
	dns_name_t *curr;

	for (curr = ISC_LIST_TAIL(*section);
	     curr != NULL;
	     curr = ISC_LIST_PREV(curr, link)) {
		if (dns_name_equal(curr, target)) {
			if (foundname != NULL)
				*foundname = curr;
			return (ISC_R_SUCCESS);
		}
	}

	return (ISC_R_NOTFOUND);
}

isc_result_t
dns_message_find(dns_name_t *name, dns_rdataclass_t rdclass,
		 dns_rdatatype_t type, dns_rdatatype_t covers,
		 dns_rdataset_t **rdataset)
{
	dns_rdataset_t *curr;

	if (rdataset != NULL) {
		REQUIRE(*rdataset == NULL);
	}

	for (curr = ISC_LIST_TAIL(name->list);
	     curr != NULL;
	     curr = ISC_LIST_PREV(curr, link)) {
		if (curr->rdclass == rdclass &&
		    curr->type == type && curr->covers == covers) {
			if (rdataset != NULL)
				*rdataset = curr;
			return (ISC_R_SUCCESS);
		}
	}

	return (ISC_R_NOTFOUND);
}

isc_result_t
dns_message_findtype(dns_name_t *name, dns_rdatatype_t type,
		     dns_rdatatype_t covers, dns_rdataset_t **rdataset)
{
	dns_rdataset_t *curr;

	REQUIRE(name != NULL);
	if (rdataset != NULL) {
		REQUIRE(*rdataset == NULL);
	}

	for (curr = ISC_LIST_TAIL(name->list);
	     curr != NULL;
	     curr = ISC_LIST_PREV(curr, link)) {
		if (curr->type == type && curr->covers == covers) {
			if (rdataset != NULL)
				*rdataset = curr;
			return (ISC_R_SUCCESS);
		}
	}

	return (ISC_R_NOTFOUND);
}

/*
 * Read a name from buffer "source".
 */
static isc_result_t
getname(dns_name_t *name, isc_buffer_t *source, dns_message_t *msg,
	dns_decompress_t *dctx)
{
	isc_buffer_t *scratch;
	isc_result_t result;
	unsigned int tries;

	scratch = currentbuffer(msg);

	/*
	 * First try:  use current buffer.
	 * Second try:  allocate a new buffer and use that.
	 */
	tries = 0;
	while (tries < 2) {
		result = dns_name_fromwire(name, source, dctx, ISC_FALSE,
					   scratch);

		if (result == ISC_R_NOSPACE) {
			tries++;

			result = newbuffer(msg, SCRATCHPAD_SIZE);
			if (result != ISC_R_SUCCESS)
				return (result);

			scratch = currentbuffer(msg);
			dns_name_reset(name);
		} else {
			return (result);
		}
	}

	INSIST(0);  /* Cannot get here... */
	return (ISC_R_UNEXPECTED);
}

static isc_result_t
getrdata(isc_buffer_t *source, dns_message_t *msg, dns_decompress_t *dctx,
	 dns_rdataclass_t rdclass, dns_rdatatype_t rdtype,
	 unsigned int rdatalen, dns_rdata_t *rdata)
{
	isc_buffer_t *scratch;
	isc_result_t result;
	unsigned int tries;
	unsigned int trysize;

	scratch = currentbuffer(msg);

	isc_buffer_setactive(source, rdatalen);

	/*
	 * First try:  use current buffer.
	 * Second try:  allocate a new buffer of size
	 *     max(SCRATCHPAD_SIZE, 2 * compressed_rdatalen)
	 *     (the data will fit if it was not more than 50% compressed)
	 * Subsequent tries: double buffer size on each try.
	 */
	tries = 0;
	trysize = 0;
	/* XXX possibly change this to a while (tries < 2) loop */
	for (;;) {
		result = dns_rdata_fromwire(rdata, rdclass, rdtype,
					    source, dctx, 0,
					    scratch);

		if (result == ISC_R_NOSPACE) {
			if (tries == 0) {
				trysize = 2 * rdatalen;
				if (trysize < SCRATCHPAD_SIZE)
					trysize = SCRATCHPAD_SIZE;
			} else {
				INSIST(trysize != 0);
				if (trysize >= 65535)
					return (ISC_R_NOSPACE);
					/* XXX DNS_R_RRTOOLONG? */
				trysize *= 2;
			}
			tries++;
			result = newbuffer(msg, trysize);
			if (result != ISC_R_SUCCESS)
				return (result);

			scratch = currentbuffer(msg);
		} else {
			return (result);
		}
	}
}

#define DO_FORMERR					\
	do {						\
		if (best_effort)			\
			seen_problem = ISC_TRUE;	\
		else {					\
			result = DNS_R_FORMERR;		\
			goto cleanup;			\
		}					\
	} while (0)

static isc_result_t
getquestions(isc_buffer_t *source, dns_message_t *msg, dns_decompress_t *dctx,
	     unsigned int options)
{
	isc_region_t r;
	unsigned int count;
	dns_name_t *name;
	dns_name_t *name2;
	dns_offsets_t *offsets;
	dns_rdataset_t *rdataset;
	dns_rdatalist_t *rdatalist;
	isc_result_t result;
	dns_rdatatype_t rdtype;
	dns_rdataclass_t rdclass;
	dns_namelist_t *section;
	isc_boolean_t free_name;
	isc_boolean_t best_effort;
	isc_boolean_t seen_problem;

	section = &msg->sections[DNS_SECTION_QUESTION];

	best_effort = ISC_TF(options & DNS_MESSAGEPARSE_BESTEFFORT);
	seen_problem = ISC_FALSE;

	name = NULL;
	rdataset = NULL;
	rdatalist = NULL;

	for (count = 0; count < msg->counts[DNS_SECTION_QUESTION]; count++) {
		name = isc_mempool_get(msg->namepool);
		if (name == NULL)
			return (ISC_R_NOMEMORY);
		free_name = ISC_TRUE;

		offsets = newoffsets(msg);
		if (offsets == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		dns_name_init(name, *offsets);

		/*
		 * Parse the name out of this packet.
		 */
		isc_buffer_remainingregion(source, &r);
		isc_buffer_setactive(source, r.length);
		result = getname(name, source, msg, dctx);
		if (result != ISC_R_SUCCESS)
			goto cleanup;

		/*
		 * Run through the section, looking to see if this name
		 * is already there.  If it is found, put back the allocated
		 * name since we no longer need it, and set our name pointer
		 * to point to the name we found.
		 */
		result = findname(&name2, name, section);

		/*
		 * If it is the first name in the section, accept it.
		 *
		 * If it is not, but is not the same as the name already
		 * in the question section, append to the section.  Note that
		 * here in the question section this is illegal, so return
		 * FORMERR.  In the future, check the opcode to see if
		 * this should be legal or not.  In either case we no longer
		 * need this name pointer.
		 */
		if (result != ISC_R_SUCCESS) {
			if (!ISC_LIST_EMPTY(*section))
				DO_FORMERR;
			ISC_LIST_APPEND(*section, name, link);
			free_name = ISC_FALSE;
		} else {
			isc_mempool_put(msg->namepool, name);
			name = name2;
			name2 = NULL;
			free_name = ISC_FALSE;
		}

		/*
		 * Get type and class.
		 */
		isc_buffer_remainingregion(source, &r);
		if (r.length < 4) {
			result = ISC_R_UNEXPECTEDEND;
			goto cleanup;
		}
		rdtype = isc_buffer_getuint16(source);
		rdclass = isc_buffer_getuint16(source);

		/*
		 * If this class is different than the one we already read,
		 * this is an error.
		 */
		if (msg->state == DNS_SECTION_ANY) {
			msg->state = DNS_SECTION_QUESTION;
			msg->rdclass = rdclass;
		} else if (msg->rdclass != rdclass)
			DO_FORMERR;

		/*
		 * Can't ask the same question twice.
		 */
		result = dns_message_find(name, rdclass, rdtype, 0, NULL);
		if (result == ISC_R_SUCCESS)
			DO_FORMERR;

		/*
		 * Allocate a new rdatalist.
		 */
		rdatalist = newrdatalist(msg);
		if (rdatalist == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		rdataset =  isc_mempool_get(msg->rdspool);
		if (rdataset == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}

		/*
		 * Convert rdatalist to rdataset, and attach the latter to
		 * the name.
		 */
		rdatalist->type = rdtype;
		rdatalist->covers = 0;
		rdatalist->rdclass = rdclass;
		rdatalist->ttl = 0;
		ISC_LIST_INIT(rdatalist->rdata);

		dns_rdataset_init(rdataset);
		result = dns_rdatalist_tordataset(rdatalist, rdataset);
		if (result != ISC_R_SUCCESS)
			goto cleanup;

		rdataset->attributes |= DNS_RDATASETATTR_QUESTION;

		ISC_LIST_APPEND(name->list, rdataset, link);
		rdataset = NULL;
	}

	if (seen_problem)
		return (DNS_R_RECOVERABLE);
	return (ISC_R_SUCCESS);

 cleanup:
	if (rdataset != NULL) {
		INSIST(!dns_rdataset_isassociated(rdataset));
		isc_mempool_put(msg->rdspool, rdataset);
	}
#if 0
	if (rdatalist != NULL)
		isc_mempool_put(msg->rdlpool, rdatalist);
#endif
	if (free_name)
		isc_mempool_put(msg->namepool, name);

	return (result);
}

static isc_boolean_t
update(dns_section_t section, dns_rdataclass_t rdclass) {
	if (section == DNS_SECTION_PREREQUISITE)
		return (ISC_TF(rdclass == dns_rdataclass_any ||
			       rdclass == dns_rdataclass_none));
	if (section == DNS_SECTION_UPDATE)
		return (ISC_TF(rdclass == dns_rdataclass_any));
	return (ISC_FALSE);
}

static isc_result_t
getsection(isc_buffer_t *source, dns_message_t *msg, dns_decompress_t *dctx,
	   dns_section_t sectionid, unsigned int options)
{
	isc_region_t r;
	unsigned int count, rdatalen;
	dns_name_t *name;
	dns_name_t *name2;
	dns_offsets_t *offsets;
	dns_rdataset_t *rdataset;
	dns_rdatalist_t *rdatalist;
	isc_result_t result;
	dns_rdatatype_t rdtype, covers;
	dns_rdataclass_t rdclass;
	dns_rdata_t *rdata;
	dns_ttl_t ttl;
	dns_namelist_t *section;
	isc_boolean_t free_name, free_rdataset;
	isc_boolean_t preserve_order, best_effort, seen_problem;
	isc_boolean_t issigzero;

	preserve_order = ISC_TF(options & DNS_MESSAGEPARSE_PRESERVEORDER);
	best_effort = ISC_TF(options & DNS_MESSAGEPARSE_BESTEFFORT);
	seen_problem = ISC_FALSE;

	for (count = 0; count < msg->counts[sectionid]; count++) {
		int recstart = source->current;
		isc_boolean_t skip_name_search, skip_type_search;

		section = &msg->sections[sectionid];

		skip_name_search = ISC_FALSE;
		skip_type_search = ISC_FALSE;
		free_name = ISC_FALSE;
		free_rdataset = ISC_FALSE;

		name = isc_mempool_get(msg->namepool);
		if (name == NULL)
			return (ISC_R_NOMEMORY);
		free_name = ISC_TRUE;

		offsets = newoffsets(msg);
		if (offsets == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		dns_name_init(name, *offsets);

		/*
		 * Parse the name out of this packet.
		 */
		isc_buffer_remainingregion(source, &r);
		isc_buffer_setactive(source, r.length);
		result = getname(name, source, msg, dctx);
		if (result != ISC_R_SUCCESS)
			goto cleanup;

		/*
		 * Get type, class, ttl, and rdatalen.  Verify that at least
		 * rdatalen bytes remain.  (Some of this is deferred to
		 * later.)
		 */
		isc_buffer_remainingregion(source, &r);
		if (r.length < 2 + 2 + 4 + 2) {
			result = ISC_R_UNEXPECTEDEND;
			goto cleanup;
		}
		rdtype = isc_buffer_getuint16(source);
		rdclass = isc_buffer_getuint16(source);

		/*
		 * If there was no question section, we may not yet have
		 * established a class.  Do so now.
		 */
		if (msg->state == DNS_SECTION_ANY &&
		    rdtype != dns_rdatatype_opt &&	/* class is UDP SIZE */
		    rdtype != dns_rdatatype_tsig &&	/* class is ANY */
		    rdtype != dns_rdatatype_tkey) {	/* class is undefined */
			msg->rdclass = rdclass;
			msg->state = DNS_SECTION_QUESTION;
		}

		/*
		 * If this class is different than the one in the question
		 * section, bail.
		 */
		if (msg->opcode != dns_opcode_update
		    && rdtype != dns_rdatatype_tsig
		    && rdtype != dns_rdatatype_opt
		    && rdtype != dns_rdatatype_dnskey /* in a TKEY query */
		    && rdtype != dns_rdatatype_sig /* SIG(0) */
		    && rdtype != dns_rdatatype_tkey /* Win2000 TKEY */
		    && msg->rdclass != dns_rdataclass_any
		    && msg->rdclass != rdclass)
			DO_FORMERR;

		/*
		 * Special type handling for TSIG, OPT, and TKEY.
		 */
		if (rdtype == dns_rdatatype_tsig) {
			/*
			 * If it is a tsig, verify that it is in the
			 * additional data section.
			 */
			if (sectionid != DNS_SECTION_ADDITIONAL ||
			    rdclass != dns_rdataclass_any ||
			    count != msg->counts[sectionid]  - 1)
				DO_FORMERR;
			msg->sigstart = recstart;
			skip_name_search = ISC_TRUE;
			skip_type_search = ISC_TRUE;
		} else if (rdtype == dns_rdatatype_opt) {
			/*
			 * The name of an OPT record must be ".", it
			 * must be in the additional data section, and
			 * it must be the first OPT we've seen.
			 */
			if (!dns_name_equal(dns_rootname, name) ||
			    msg->opt != NULL)
				DO_FORMERR;
			skip_name_search = ISC_TRUE;
			skip_type_search = ISC_TRUE;
		} else if (rdtype == dns_rdatatype_tkey) {
			/*
			 * A TKEY must be in the additional section if this
			 * is a query, and the answer section if this is a
			 * response.  Unless it's a Win2000 client.
			 *
			 * Its class is ignored.
			 */
			dns_section_t tkeysection;

			if ((msg->flags & DNS_MESSAGEFLAG_QR) == 0)
				tkeysection = DNS_SECTION_ADDITIONAL;
			else
				tkeysection = DNS_SECTION_ANSWER;
			if (sectionid != tkeysection &&
			    sectionid != DNS_SECTION_ANSWER)
				DO_FORMERR;
		}

		/*
		 * ... now get ttl and rdatalen, and check buffer.
		 */
		ttl = isc_buffer_getuint32(source);
		rdatalen = isc_buffer_getuint16(source);
		r.length -= (2 + 2 + 4 + 2);
		if (r.length < rdatalen) {
			result = ISC_R_UNEXPECTEDEND;
			goto cleanup;
		}

		/*
		 * Read the rdata from the wire format.  Interpret the
		 * rdata according to its actual class, even if it had a
		 * DynDNS meta-class in the packet (unless this is a TSIG).
		 * Then put the meta-class back into the finished rdata.
		 */
		rdata = newrdata(msg);
		if (rdata == NULL) {
			result = ISC_R_NOMEMORY;
			goto cleanup;
		}
		if (msg->opcode == dns_opcode_update &&
		    update(sectionid, rdclass)) {
			if (rdatalen != 0) {
				result = DNS_R_FORMERR;
				goto cleanup;
			}
			/*
			 * When the rdata is empty, the data pointer is
			 * never dereferenced, but it must still be non-NULL.
			 * Casting 1 rather than "" avoids warnings about
			 * discarding the const attribute of a string,
			 * for compilers that would warn about such things.
			 */
			rdata->data = (unsigned char *)1;
			rdata->length = 0;
			rdata->rdclass = rdclass;
			rdata->type = rdtype;
			rdata->flags = DNS_RDATA_UPDATE;
			result = ISC_R_SUCCESS;
		} else if (rdclass == dns_rdataclass_none &&
			   msg->opcode == dns_opcode_update &&
			   sectionid == DNS_SECTION_UPDATE) {
			result = getrdata(source, msg, dctx, msg->rdclass,
					  rdtype, rdatalen, rdata);
		} else
			result = getrdata(source, msg, dctx, rdclass,
					  rdtype, rdatalen, rdata);
		if (result != ISC_R_SUCCESS)
			goto cleanup;
		rdata->rdclass = rdclass;
		issigzero = ISC_FALSE;
		if (rdtype == dns_rdatatype_rrsig  &&
		    rdata->flags == 0) {
			covers = dns_rdata_covers(rdata);
			if (covers == 0)
				DO_FORMERR;
		} else if (rdtype == dns_rdatatype_sig /* SIG(0) */ &&
			   rdata->flags == 0) {
			covers = dns_rdata_covers(rdata);
			if (covers == 0) {
				if (sectionid != DNS_SECTION_ADDITIONAL ||
				    count != msg->counts[sectionid]  - 1)
					DO_FORMERR;
				msg->sigstart = recstart;
				skip_name_search = ISC_TRUE;
				skip_type_search = ISC_TRUE;
				issigzero = ISC_TRUE;
			}
		} else
			covers = 0;

		/*
		 * If we are doing a dynamic update or this is a meta-type,
		 * don't bother searching for a name, just append this one
		 * to the end of the message.
		 */
		if (preserve_order || msg->opcode == dns_opcode_update ||
		    skip_name_search) {
			if (rdtype != dns_rdatatype_opt &&
			    rdtype != dns_rdatatype_tsig &&
			    !issigzero)
			{
				ISC_LIST_APPEND(*section, name, link);
				free_name = ISC_FALSE;
			}
		} else {
			/*
			 * Run through the section, looking to see if this name
			 * is already there.  If it is found, put back the
			 * allocated name since we no longer need it, and set
			 * our name pointer to point to the name we found.
			 */
			result = findname(&name2, name, section);

			/*
			 * If it is a new name, append to the section.
			 */
			if (result == ISC_R_SUCCESS) {
				isc_mempool_put(msg->namepool, name);
				name = name2;
			} else {
				ISC_LIST_APPEND(*section, name, link);
			}
			free_name = ISC_FALSE;
		}

		/*
		 * Search name for the particular type and class.
		 * Skip this stage if in update mode or this is a meta-type.
		 */
		if (preserve_order || msg->opcode == dns_opcode_update ||
		    skip_type_search)
			result = ISC_R_NOTFOUND;
		else {
			/*
			 * If this is a type that can only occur in
			 * the question section, fail.
			 */
			if (dns_rdatatype_questiononly(rdtype))
				DO_FORMERR;

			rdataset = NULL;
			result = dns_message_find(name, rdclass, rdtype,
						   covers, &rdataset);
		}

		/*
		 * If we found an rdataset that matches, we need to
		 * append this rdata to that set.  If we did not, we need
		 * to create a new rdatalist, store the important bits there,
		 * convert it to an rdataset, and link the latter to the name.
		 * Yuck.  When appending, make certain that the type isn't
		 * a singleton type, such as SOA or CNAME.
		 *
		 * Note that this check will be bypassed when preserving order,
		 * the opcode is an update, or the type search is skipped.
		 */
		if (result == ISC_R_SUCCESS) {
			if (dns_rdatatype_issingleton(rdtype))
				DO_FORMERR;
		}

		if (result == ISC_R_NOTFOUND) {
			rdataset = isc_mempool_get(msg->rdspool);
			if (rdataset == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}
			free_rdataset = ISC_TRUE;

			rdatalist = newrdatalist(msg);
			if (rdatalist == NULL) {
				result = ISC_R_NOMEMORY;
				goto cleanup;
			}

			rdatalist->type = rdtype;
			rdatalist->covers = covers;
			rdatalist->rdclass = rdclass;
			rdatalist->ttl = ttl;
			ISC_LIST_INIT(rdatalist->rdata);

			dns_rdataset_init(rdataset);
			RUNTIME_CHECK(dns_rdatalist_tordataset(rdatalist,
							       rdataset)
				      == ISC_R_SUCCESS);

			if (rdtype != dns_rdatatype_opt &&
			    rdtype != dns_rdatatype_tsig &&
			    !issigzero)
			{
				ISC_LIST_APPEND(name->list, rdataset, link);
				free_rdataset = ISC_FALSE;
			}
		}

		/*
		 * Minimize TTLs.
		 *
		 * Section 5.2 of RFC2181 says we should drop
		 * nonauthoritative rrsets where the TTLs differ, but we
		 * currently treat them the as if they were authoritative and
		 * minimize them.
		 */
		if (ttl != rdataset->ttl) {
			rdataset->attributes |= DNS_RDATASETATTR_TTLADJUSTED;
			if (ttl < rdataset->ttl)
				rdataset->ttl = ttl;
		}

		/* Append this rdata to the rdataset. */
		dns_rdatalist_fromrdataset(rdataset, &rdatalist);
		ISC_LIST_APPEND(rdatalist->rdata, rdata, link);

		/*
		 * If this is an OPT record, remember it.  Also, set
		 * the extended rcode.  Note that msg->opt will only be set
		 * if best-effort parsing is enabled.
		 */
		if (rdtype == dns_rdatatype_opt && msg->opt == NULL) {
			dns_rcode_t ercode;

			msg->opt = rdataset;
			rdataset = NULL;
			free_rdataset = ISC_FALSE;
			ercode = (dns_rcode_t)
				((msg->opt->ttl & DNS_MESSAGE_EDNSRCODE_MASK)
				 >> 20);
			msg->rcode |= ercode;
			isc_mempool_put(msg->namepool, name);
			free_name = ISC_FALSE;
		}

		/*
		 * If this is an SIG(0) or TSIG record, remember it.  Note
		 * that msg->sig0 or msg->tsig will only be set if best-effort
		 * parsing is enabled.
		 */
		if (issigzero && msg->sig0 == NULL) {
			msg->sig0 = rdataset;
			msg->sig0name = name;
			rdataset = NULL;
			free_rdataset = ISC_FALSE;
			free_name = ISC_FALSE;
		} else if (rdtype == dns_rdatatype_tsig && msg->tsig == NULL) {
			msg->tsig = rdataset;
			msg->tsigname = name;
			rdataset = NULL;
			free_rdataset = ISC_FALSE;
			free_name = ISC_FALSE;
		}

		if (seen_problem) {
			if (free_name)
				isc_mempool_put(msg->namepool, name);
			if (free_rdataset)
				isc_mempool_put(msg->rdspool, rdataset);
			free_name = free_rdataset = ISC_FALSE;
		}
		INSIST(free_name == ISC_FALSE);
		INSIST(free_rdataset == ISC_FALSE);
	}

	if (seen_problem)
		return (DNS_R_RECOVERABLE);
	return (ISC_R_SUCCESS);

 cleanup:
	if (free_name)
		isc_mempool_put(msg->namepool, name);
	if (free_rdataset)
		isc_mempool_put(msg->rdspool, rdataset);

	return (result);
}

isc_result_t
dns_message_parse(dns_message_t *msg, isc_buffer_t *source,
		  unsigned int options)
{
	isc_region_t r;
	dns_decompress_t dctx;
	isc_result_t ret;
	isc_uint16_t tmpflags;
	isc_buffer_t origsource;
	isc_boolean_t seen_problem;
	isc_boolean_t ignore_tc;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(source != NULL);
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTPARSE);

	seen_problem = ISC_FALSE;
	ignore_tc = ISC_TF(options & DNS_MESSAGEPARSE_IGNORETRUNCATION);

	origsource = *source;

	msg->header_ok = 0;
	msg->question_ok = 0;

	isc_buffer_remainingregion(source, &r);
	if (r.length < DNS_MESSAGE_HEADERLEN)
		return (ISC_R_UNEXPECTEDEND);

	msg->id = isc_buffer_getuint16(source);
	tmpflags = isc_buffer_getuint16(source);
	msg->opcode = ((tmpflags & DNS_MESSAGE_OPCODE_MASK)
		       >> DNS_MESSAGE_OPCODE_SHIFT);
	msg->rcode = (dns_rcode_t)(tmpflags & DNS_MESSAGE_RCODE_MASK);
	msg->flags = (tmpflags & DNS_MESSAGE_FLAG_MASK);
	msg->counts[DNS_SECTION_QUESTION] = isc_buffer_getuint16(source);
	msg->counts[DNS_SECTION_ANSWER] = isc_buffer_getuint16(source);
	msg->counts[DNS_SECTION_AUTHORITY] = isc_buffer_getuint16(source);
	msg->counts[DNS_SECTION_ADDITIONAL] = isc_buffer_getuint16(source);

	msg->header_ok = 1;

	/*
	 * -1 means no EDNS.
	 */
	dns_decompress_init(&dctx, -1, DNS_DECOMPRESS_ANY);

	dns_decompress_setmethods(&dctx, DNS_COMPRESS_GLOBAL14);

	ret = getquestions(source, msg, &dctx, options);
	if (ret == ISC_R_UNEXPECTEDEND && ignore_tc)
		goto truncated;
	if (ret == DNS_R_RECOVERABLE) {
		seen_problem = ISC_TRUE;
		ret = ISC_R_SUCCESS;
	}
	if (ret != ISC_R_SUCCESS)
		return (ret);
	msg->question_ok = 1;

	ret = getsection(source, msg, &dctx, DNS_SECTION_ANSWER, options);
	if (ret == ISC_R_UNEXPECTEDEND && ignore_tc)
		goto truncated;
	if (ret == DNS_R_RECOVERABLE) {
		seen_problem = ISC_TRUE;
		ret = ISC_R_SUCCESS;
	}
	if (ret != ISC_R_SUCCESS)
		return (ret);

	ret = getsection(source, msg, &dctx, DNS_SECTION_AUTHORITY, options);
	if (ret == ISC_R_UNEXPECTEDEND && ignore_tc)
		goto truncated;
	if (ret == DNS_R_RECOVERABLE) {
		seen_problem = ISC_TRUE;
		ret = ISC_R_SUCCESS;
	}
	if (ret != ISC_R_SUCCESS)
		return (ret);

	ret = getsection(source, msg, &dctx, DNS_SECTION_ADDITIONAL, options);
	if (ret == ISC_R_UNEXPECTEDEND && ignore_tc)
		goto truncated;
	if (ret == DNS_R_RECOVERABLE) {
		seen_problem = ISC_TRUE;
		ret = ISC_R_SUCCESS;
	}
	if (ret != ISC_R_SUCCESS)
		return (ret);

	isc_buffer_remainingregion(source, &r);
	if (r.length != 0) {
		isc_log_write(dns_lctx, ISC_LOGCATEGORY_GENERAL,
			      DNS_LOGMODULE_MESSAGE, ISC_LOG_DEBUG(3),
			      "message has %u byte(s) of trailing garbage",
			      r.length);
	}

 truncated:
	if ((options & DNS_MESSAGEPARSE_CLONEBUFFER) == 0)
		isc_buffer_usedregion(&origsource, &msg->saved);
	else {
		msg->saved.length = isc_buffer_usedlength(&origsource);
		msg->saved.base = isc_mem_get(msg->mctx, msg->saved.length);
		if (msg->saved.base == NULL)
			return (ISC_R_NOMEMORY);
		memcpy(msg->saved.base, isc_buffer_base(&origsource),
		       msg->saved.length);
		msg->free_saved = 1;
	}

	if (ret == ISC_R_UNEXPECTEDEND && ignore_tc)
		return (DNS_R_RECOVERABLE);
	if (seen_problem == ISC_TRUE)
		return (DNS_R_RECOVERABLE);
	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_renderbegin(dns_message_t *msg, dns_compress_t *cctx,
			isc_buffer_t *buffer)
{
	isc_region_t r;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(buffer != NULL);
	REQUIRE(msg->buffer == NULL);
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTRENDER);

	msg->cctx = cctx;

	/*
	 * Erase the contents of this buffer.
	 */
	isc_buffer_clear(buffer);

	/*
	 * Make certain there is enough for at least the header in this
	 * buffer.
	 */
	isc_buffer_availableregion(buffer, &r);
	if (r.length < DNS_MESSAGE_HEADERLEN)
		return (ISC_R_NOSPACE);

	if (r.length < msg->reserved)
		return (ISC_R_NOSPACE);

	/*
	 * Reserve enough space for the header in this buffer.
	 */
	isc_buffer_add(buffer, DNS_MESSAGE_HEADERLEN);

	msg->buffer = buffer;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_renderchangebuffer(dns_message_t *msg, isc_buffer_t *buffer) {
	isc_region_t r, rn;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(buffer != NULL);
	REQUIRE(msg->buffer != NULL);

	/*
	 * Ensure that the new buffer is empty, and has enough space to
	 * hold the current contents.
	 */
	isc_buffer_clear(buffer);

	isc_buffer_availableregion(buffer, &rn);
	isc_buffer_usedregion(msg->buffer, &r);
	REQUIRE(rn.length > r.length);

	/*
	 * Copy the contents from the old to the new buffer.
	 */
	isc_buffer_add(buffer, r.length);
	memcpy(rn.base, r.base, r.length);

	msg->buffer = buffer;

	return (ISC_R_SUCCESS);
}

void
dns_message_renderrelease(dns_message_t *msg, unsigned int space) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(space <= msg->reserved);

	msg->reserved -= space;
}

isc_result_t
dns_message_renderreserve(dns_message_t *msg, unsigned int space) {
	isc_region_t r;

	REQUIRE(DNS_MESSAGE_VALID(msg));

	if (msg->buffer != NULL) {
		isc_buffer_availableregion(msg->buffer, &r);
		if (r.length < (space + msg->reserved))
			return (ISC_R_NOSPACE);
	}

	msg->reserved += space;

	return (ISC_R_SUCCESS);
}

static inline isc_boolean_t
wrong_priority(dns_rdataset_t *rds, int pass, dns_rdatatype_t preferred_glue) {
	int pass_needed;

	/*
	 * If we are not rendering class IN, this ordering is bogus.
	 */
	if (rds->rdclass != dns_rdataclass_in)
		return (ISC_FALSE);

	switch (rds->type) {
	case dns_rdatatype_a:
	case dns_rdatatype_aaaa:
		if (preferred_glue == rds->type)
			pass_needed = 4;
		else
			pass_needed = 3;
		break;
	case dns_rdatatype_rrsig:
	case dns_rdatatype_dnskey:
		pass_needed = 2;
		break;
	default:
		pass_needed = 1;
	}

	if (pass_needed >= pass)
		return (ISC_FALSE);

	return (ISC_TRUE);
}

isc_result_t
dns_message_rendersection(dns_message_t *msg, dns_section_t sectionid,
			  unsigned int options)
{
	dns_namelist_t *section;
	dns_name_t *name, *next_name;
	dns_rdataset_t *rdataset, *next_rdataset;
	unsigned int count, total;
	isc_result_t result;
	isc_buffer_t st; /* for rollbacks */
	int pass;
	isc_boolean_t partial = ISC_FALSE;
	unsigned int rd_options;
	dns_rdatatype_t preferred_glue = 0;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(msg->buffer != NULL);
	REQUIRE(VALID_NAMED_SECTION(sectionid));

	section = &msg->sections[sectionid];

	if ((sectionid == DNS_SECTION_ADDITIONAL)
	    && (options & DNS_MESSAGERENDER_ORDERED) == 0) {
		if ((options & DNS_MESSAGERENDER_PREFER_A) != 0) {
			preferred_glue = dns_rdatatype_a;
			pass = 4;
		} else if ((options & DNS_MESSAGERENDER_PREFER_AAAA) != 0) {
			preferred_glue = dns_rdatatype_aaaa;
			pass = 4;
		} else
			pass = 3;
	} else
		pass = 1;

	if ((options & DNS_MESSAGERENDER_OMITDNSSEC) == 0)
		rd_options = 0;
	else
		rd_options = DNS_RDATASETTOWIRE_OMITDNSSEC;

	/*
	 * Shrink the space in the buffer by the reserved amount.
	 */
	msg->buffer->length -= msg->reserved;

	total = 0;
	if (msg->reserved == 0 && (options & DNS_MESSAGERENDER_PARTIAL) != 0)
		partial = ISC_TRUE;

	/*
	 * Render required glue first.  Set TC if it won't fit.
	 */
	name = ISC_LIST_HEAD(*section);
	if (name != NULL) {
		rdataset = ISC_LIST_HEAD(name->list);
		if (rdataset != NULL &&
		    (rdataset->attributes & DNS_RDATASETATTR_REQUIREDGLUE) != 0 &&
		    (rdataset->attributes & DNS_RDATASETATTR_RENDERED) == 0) {
			const void *order_arg = msg->order_arg;
			st = *(msg->buffer);
			count = 0;
			if (partial)
				result = dns_rdataset_towirepartial(rdataset,
								    name,
								    msg->cctx,
								    msg->buffer,
								    msg->order,
								    order_arg,
								    rd_options,
								    &count,
								    NULL);
			else
				result = dns_rdataset_towiresorted(rdataset,
								   name,
								   msg->cctx,
								   msg->buffer,
								   msg->order,
								   order_arg,
								   rd_options,
								   &count);
			total += count;
			if (partial && result == ISC_R_NOSPACE) {
				msg->flags |= DNS_MESSAGEFLAG_TC;
				msg->buffer->length += msg->reserved;
				msg->counts[sectionid] += total;
				return (result);
			}
			if (result != ISC_R_SUCCESS) {
				INSIST(st.used < 65536);
				dns_compress_rollback(msg->cctx,
						      (isc_uint16_t)st.used);
				*(msg->buffer) = st;  /* rollback */
				msg->buffer->length += msg->reserved;
				msg->counts[sectionid] += total;
				return (result);
			}
			rdataset->attributes |= DNS_RDATASETATTR_RENDERED;
		}
	}

	do {
		name = ISC_LIST_HEAD(*section);
		if (name == NULL) {
			msg->buffer->length += msg->reserved;
			msg->counts[sectionid] += total;
			return (ISC_R_SUCCESS);
		}

		while (name != NULL) {
			next_name = ISC_LIST_NEXT(name, link);

			rdataset = ISC_LIST_HEAD(name->list);
			while (rdataset != NULL) {
				next_rdataset = ISC_LIST_NEXT(rdataset, link);

				if ((rdataset->attributes &
				     DNS_RDATASETATTR_RENDERED) != 0)
					goto next;

				if (((options & DNS_MESSAGERENDER_ORDERED)
				     == 0)
				    && (sectionid == DNS_SECTION_ADDITIONAL)
				    && wrong_priority(rdataset, pass,
						      preferred_glue))
					goto next;

				st = *(msg->buffer);

				count = 0;
				if (partial)
					result = dns_rdataset_towirepartial(
							  rdataset,
							  name,
							  msg->cctx,
							  msg->buffer,
							  msg->order,
							  msg->order_arg,
							  rd_options,
							  &count,
							  NULL);
				else
					result = dns_rdataset_towiresorted(
							  rdataset,
							  name,
							  msg->cctx,
							  msg->buffer,
							  msg->order,
							  msg->order_arg,
							  rd_options,
							  &count);

				total += count;

				/*
				 * If out of space, record stats on what we
				 * rendered so far, and return that status.
				 *
				 * XXXMLG Need to change this when
				 * dns_rdataset_towire() can render partial
				 * sets starting at some arbitrary point in the
				 * set.  This will include setting a bit in the
				 * rdataset to indicate that a partial
				 * rendering was done, and some state saved
				 * somewhere (probably in the message struct)
				 * to indicate where to continue from.
				 */
				if (partial && result == ISC_R_NOSPACE) {
					msg->buffer->length += msg->reserved;
					msg->counts[sectionid] += total;
					return (result);
				}
				if (result != ISC_R_SUCCESS) {
					INSIST(st.used < 65536);
					dns_compress_rollback(msg->cctx,
							(isc_uint16_t)st.used);
					*(msg->buffer) = st;  /* rollback */
					msg->buffer->length += msg->reserved;
					msg->counts[sectionid] += total;
					return (result);
				}

				/*
				 * If we have rendered non-validated data,
				 * ensure that the AD bit is not set.
				 */
				if (rdataset->trust != dns_trust_secure &&
				    (sectionid == DNS_SECTION_ANSWER ||
				     sectionid == DNS_SECTION_AUTHORITY))
					msg->flags &= ~DNS_MESSAGEFLAG_AD;
				if (OPTOUT(rdataset))
					msg->flags &= ~DNS_MESSAGEFLAG_AD;

				rdataset->attributes |=
					DNS_RDATASETATTR_RENDERED;

			next:
				rdataset = next_rdataset;
			}

			name = next_name;
		}
	} while (--pass != 0);

	msg->buffer->length += msg->reserved;
	msg->counts[sectionid] += total;

	return (ISC_R_SUCCESS);
}

void
dns_message_renderheader(dns_message_t *msg, isc_buffer_t *target) {
	isc_uint16_t tmp;
	isc_region_t r;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(target != NULL);

	isc_buffer_availableregion(target, &r);
	REQUIRE(r.length >= DNS_MESSAGE_HEADERLEN);

	isc_buffer_putuint16(target, msg->id);

	tmp = ((msg->opcode << DNS_MESSAGE_OPCODE_SHIFT)
	       & DNS_MESSAGE_OPCODE_MASK);
	tmp |= (msg->rcode & DNS_MESSAGE_RCODE_MASK);
	tmp |= (msg->flags & DNS_MESSAGE_FLAG_MASK);

	INSIST(msg->counts[DNS_SECTION_QUESTION]  < 65536 &&
	       msg->counts[DNS_SECTION_ANSWER]    < 65536 &&
	       msg->counts[DNS_SECTION_AUTHORITY] < 65536 &&
	       msg->counts[DNS_SECTION_ADDITIONAL] < 65536);

	isc_buffer_putuint16(target, tmp);
	isc_buffer_putuint16(target,
			    (isc_uint16_t)msg->counts[DNS_SECTION_QUESTION]);
	isc_buffer_putuint16(target,
			    (isc_uint16_t)msg->counts[DNS_SECTION_ANSWER]);
	isc_buffer_putuint16(target,
			    (isc_uint16_t)msg->counts[DNS_SECTION_AUTHORITY]);
	isc_buffer_putuint16(target,
			    (isc_uint16_t)msg->counts[DNS_SECTION_ADDITIONAL]);
}

isc_result_t
dns_message_renderend(dns_message_t *msg) {
	isc_buffer_t tmpbuf;
	isc_region_t r;
	int result;
	unsigned int count;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(msg->buffer != NULL);

	if ((msg->rcode & ~DNS_MESSAGE_RCODE_MASK) != 0 && msg->opt == NULL) {
		/*
		 * We have an extended rcode but are not using EDNS.
		 */
		return (DNS_R_FORMERR);
	}

	/*
	 * If we've got an OPT record, render it.
	 */
	if (msg->opt != NULL) {
		dns_message_renderrelease(msg, msg->opt_reserved);
		msg->opt_reserved = 0;
		/*
		 * Set the extended rcode.
		 */
		msg->opt->ttl &= ~DNS_MESSAGE_EDNSRCODE_MASK;
		msg->opt->ttl |= ((msg->rcode << 20) &
				  DNS_MESSAGE_EDNSRCODE_MASK);
		/*
		 * Render.
		 */
		count = 0;
		result = dns_rdataset_towire(msg->opt, dns_rootname,
					     msg->cctx, msg->buffer, 0,
					     &count);
		msg->counts[DNS_SECTION_ADDITIONAL] += count;
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	/*
	 * If we're adding a TSIG or SIG(0) to a truncated message,
	 * clear all rdatasets from the message except for the question
	 * before adding the TSIG or SIG(0).  If the question doesn't fit,
	 * don't include it.
	 */
	if ((msg->tsigkey != NULL || msg->sig0key != NULL) &&
	    (msg->flags & DNS_MESSAGEFLAG_TC) != 0)
	{
		isc_buffer_t *buf;

		msgresetnames(msg, DNS_SECTION_ANSWER);
		buf = msg->buffer;
		dns_message_renderreset(msg);
		msg->buffer = buf;
		isc_buffer_clear(msg->buffer);
		isc_buffer_add(msg->buffer, DNS_MESSAGE_HEADERLEN);
		dns_compress_rollback(msg->cctx, 0);
		result = dns_message_rendersection(msg, DNS_SECTION_QUESTION,
						   0);
		if (result != ISC_R_SUCCESS && result != ISC_R_NOSPACE)
			return (result);
	}

	/*
	 * If we're adding a TSIG record, generate and render it.
	 */
	if (msg->tsigkey != NULL) {
		dns_message_renderrelease(msg, msg->sig_reserved);
		msg->sig_reserved = 0;
		result = dns_tsig_sign(msg);
		if (result != ISC_R_SUCCESS)
			return (result);
		count = 0;
		result = dns_rdataset_towire(msg->tsig, msg->tsigname,
					     msg->cctx, msg->buffer, 0,
					     &count);
		msg->counts[DNS_SECTION_ADDITIONAL] += count;
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	/*
	 * If we're adding a SIG(0) record, generate and render it.
	 */
	if (msg->sig0key != NULL) {
		dns_message_renderrelease(msg, msg->sig_reserved);
		msg->sig_reserved = 0;
		result = dns_dnssec_signmessage(msg, msg->sig0key);
		if (result != ISC_R_SUCCESS)
			return (result);
		count = 0;
		/*
		 * Note: dns_rootname is used here, not msg->sig0name, since
		 * the owner name of a SIG(0) is irrelevant, and will not
		 * be set in a message being rendered.
		 */
		result = dns_rdataset_towire(msg->sig0, dns_rootname,
					     msg->cctx, msg->buffer, 0,
					     &count);
		msg->counts[DNS_SECTION_ADDITIONAL] += count;
		if (result != ISC_R_SUCCESS)
			return (result);
	}

	isc_buffer_usedregion(msg->buffer, &r);
	isc_buffer_init(&tmpbuf, r.base, r.length);

	dns_message_renderheader(msg, &tmpbuf);

	msg->buffer = NULL;  /* forget about this buffer only on success XXX */

	return (ISC_R_SUCCESS);
}

void
dns_message_renderreset(dns_message_t *msg) {
	unsigned int i;
	dns_name_t *name;
	dns_rdataset_t *rds;

	/*
	 * Reset the message so that it may be rendered again.
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTRENDER);

	msg->buffer = NULL;

	for (i = 0; i < DNS_SECTION_MAX; i++) {
		msg->cursors[i] = NULL;
		msg->counts[i] = 0;
		for (name = ISC_LIST_HEAD(msg->sections[i]);
		     name != NULL;
		     name = ISC_LIST_NEXT(name, link)) {
			for (rds = ISC_LIST_HEAD(name->list);
			     rds != NULL;
			     rds = ISC_LIST_NEXT(rds, link)) {
				rds->attributes &= ~DNS_RDATASETATTR_RENDERED;
			}
		}
	}
	if (msg->tsigname != NULL)
		dns_message_puttempname(msg, &msg->tsigname);
	if (msg->tsig != NULL) {
		dns_rdataset_disassociate(msg->tsig);
		dns_message_puttemprdataset(msg, &msg->tsig);
	}
	if (msg->sig0 != NULL) {
		dns_rdataset_disassociate(msg->sig0);
		dns_message_puttemprdataset(msg, &msg->sig0);
	}
}

isc_result_t
dns_message_firstname(dns_message_t *msg, dns_section_t section) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(VALID_NAMED_SECTION(section));

	msg->cursors[section] = ISC_LIST_HEAD(msg->sections[section]);

	if (msg->cursors[section] == NULL)
		return (ISC_R_NOMORE);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_nextname(dns_message_t *msg, dns_section_t section) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(VALID_NAMED_SECTION(section));
	REQUIRE(msg->cursors[section] != NULL);

	msg->cursors[section] = ISC_LIST_NEXT(msg->cursors[section], link);

	if (msg->cursors[section] == NULL)
		return (ISC_R_NOMORE);

	return (ISC_R_SUCCESS);
}

void
dns_message_currentname(dns_message_t *msg, dns_section_t section,
			dns_name_t **name)
{
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(VALID_NAMED_SECTION(section));
	REQUIRE(name != NULL && *name == NULL);
	REQUIRE(msg->cursors[section] != NULL);

	*name = msg->cursors[section];
}

isc_result_t
dns_message_findname(dns_message_t *msg, dns_section_t section,
		     dns_name_t *target, dns_rdatatype_t type,
		     dns_rdatatype_t covers, dns_name_t **name,
		     dns_rdataset_t **rdataset)
{
	dns_name_t *foundname;
	isc_result_t result;

	/*
	 * XXX These requirements are probably too intensive, especially
	 * where things can be NULL, but as they are they ensure that if
	 * something is NON-NULL, indicating that the caller expects it
	 * to be filled in, that we can in fact fill it in.
	 */
	REQUIRE(msg != NULL);
	REQUIRE(VALID_SECTION(section));
	REQUIRE(target != NULL);
	if (name != NULL)
		REQUIRE(*name == NULL);
	if (type == dns_rdatatype_any) {
		REQUIRE(rdataset == NULL);
	} else {
		if (rdataset != NULL)
			REQUIRE(*rdataset == NULL);
	}

	result = findname(&foundname, target,
			  &msg->sections[section]);

	if (result == ISC_R_NOTFOUND)
		return (DNS_R_NXDOMAIN);
	else if (result != ISC_R_SUCCESS)
		return (result);

	if (name != NULL)
		*name = foundname;

	/*
	 * And now look for the type.
	 */
	if (type == dns_rdatatype_any)
		return (ISC_R_SUCCESS);

	result = dns_message_findtype(foundname, type, covers, rdataset);
	if (result == ISC_R_NOTFOUND)
		return (DNS_R_NXRRSET);

	return (result);
}

void
dns_message_movename(dns_message_t *msg, dns_name_t *name,
		     dns_section_t fromsection,
		     dns_section_t tosection)
{
	REQUIRE(msg != NULL);
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTRENDER);
	REQUIRE(name != NULL);
	REQUIRE(VALID_NAMED_SECTION(fromsection));
	REQUIRE(VALID_NAMED_SECTION(tosection));

	/*
	 * Unlink the name from the old section
	 */
	ISC_LIST_UNLINK(msg->sections[fromsection], name, link);
	ISC_LIST_APPEND(msg->sections[tosection], name, link);
}

void
dns_message_addname(dns_message_t *msg, dns_name_t *name,
		    dns_section_t section)
{
	REQUIRE(msg != NULL);
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTRENDER);
	REQUIRE(name != NULL);
	REQUIRE(VALID_NAMED_SECTION(section));

	ISC_LIST_APPEND(msg->sections[section], name, link);
}

void
dns_message_removename(dns_message_t *msg, dns_name_t *name,
		       dns_section_t section)
{
	REQUIRE(msg != NULL);
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTRENDER);
	REQUIRE(name != NULL);
	REQUIRE(VALID_NAMED_SECTION(section));

	ISC_LIST_UNLINK(msg->sections[section], name, link);
}

isc_result_t
dns_message_gettempname(dns_message_t *msg, dns_name_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item == NULL);

	*item = isc_mempool_get(msg->namepool);
	if (*item == NULL)
		return (ISC_R_NOMEMORY);
	dns_name_init(*item, NULL);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_gettempoffsets(dns_message_t *msg, dns_offsets_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item == NULL);

	*item = newoffsets(msg);
	if (*item == NULL)
		return (ISC_R_NOMEMORY);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_gettemprdata(dns_message_t *msg, dns_rdata_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item == NULL);

	*item = newrdata(msg);
	if (*item == NULL)
		return (ISC_R_NOMEMORY);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_gettemprdataset(dns_message_t *msg, dns_rdataset_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item == NULL);

	*item = isc_mempool_get(msg->rdspool);
	if (*item == NULL)
		return (ISC_R_NOMEMORY);

	dns_rdataset_init(*item);

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_gettemprdatalist(dns_message_t *msg, dns_rdatalist_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item == NULL);

	*item = newrdatalist(msg);
	if (*item == NULL)
		return (ISC_R_NOMEMORY);

	return (ISC_R_SUCCESS);
}

void
dns_message_puttempname(dns_message_t *msg, dns_name_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item != NULL);

	if (dns_name_dynamic(*item))
		dns_name_free(*item, msg->mctx);
	isc_mempool_put(msg->namepool, *item);
	*item = NULL;
}

void
dns_message_puttemprdata(dns_message_t *msg, dns_rdata_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item != NULL);

	releaserdata(msg, *item);
	*item = NULL;
}

void
dns_message_puttemprdataset(dns_message_t *msg, dns_rdataset_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item != NULL);

	REQUIRE(!dns_rdataset_isassociated(*item));
	isc_mempool_put(msg->rdspool, *item);
	*item = NULL;
}

void
dns_message_puttemprdatalist(dns_message_t *msg, dns_rdatalist_t **item) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(item != NULL && *item != NULL);

	releaserdatalist(msg, *item);
	*item = NULL;
}

isc_result_t
dns_message_peekheader(isc_buffer_t *source, dns_messageid_t *idp,
		       unsigned int *flagsp)
{
	isc_region_t r;
	isc_buffer_t buffer;
	dns_messageid_t id;
	unsigned int flags;

	REQUIRE(source != NULL);

	buffer = *source;

	isc_buffer_remainingregion(&buffer, &r);
	if (r.length < DNS_MESSAGE_HEADERLEN)
		return (ISC_R_UNEXPECTEDEND);

	id = isc_buffer_getuint16(&buffer);
	flags = isc_buffer_getuint16(&buffer);
	flags &= DNS_MESSAGE_FLAG_MASK;

	if (flagsp != NULL)
		*flagsp = flags;
	if (idp != NULL)
		*idp = id;

	return (ISC_R_SUCCESS);
}

isc_result_t
dns_message_reply(dns_message_t *msg, isc_boolean_t want_question_section) {
	unsigned int first_section;
	isc_result_t result;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE((msg->flags & DNS_MESSAGEFLAG_QR) == 0);

	if (!msg->header_ok)
		return (DNS_R_FORMERR);
	if (msg->opcode != dns_opcode_query &&
	    msg->opcode != dns_opcode_notify)
		want_question_section = ISC_FALSE;
	if (want_question_section) {
		if (!msg->question_ok)
			return (DNS_R_FORMERR);
		first_section = DNS_SECTION_ANSWER;
	} else
		first_section = DNS_SECTION_QUESTION;
	msg->from_to_wire = DNS_MESSAGE_INTENTRENDER;
	msgresetnames(msg, first_section);
	msgresetopt(msg);
	msgresetsigs(msg, ISC_TRUE);
	msginitprivate(msg);
	/*
	 * We now clear most flags and then set QR, ensuring that the
	 * reply's flags will be in a reasonable state.
	 */
	msg->flags &= DNS_MESSAGE_REPLYPRESERVE;
	msg->flags |= DNS_MESSAGEFLAG_QR;

	/*
	 * This saves the query TSIG status, if the query was signed, and
	 * reserves space in the reply for the TSIG.
	 */
	if (msg->tsigkey != NULL) {
		unsigned int otherlen = 0;
		msg->querytsigstatus = msg->tsigstatus;
		msg->tsigstatus = dns_rcode_noerror;
		if (msg->querytsigstatus == dns_tsigerror_badtime)
			otherlen = 6;
		msg->sig_reserved = spacefortsig(msg->tsigkey, otherlen);
		result = dns_message_renderreserve(msg, msg->sig_reserved);
		if (result != ISC_R_SUCCESS) {
			msg->sig_reserved = 0;
			return (result);
		}
	}
	if (msg->saved.base != NULL) {
		msg->query.base = msg->saved.base;
		msg->query.length = msg->saved.length;
		msg->free_query = msg->free_saved;
		msg->saved.base = NULL;
		msg->saved.length = 0;
		msg->free_saved = 0;
	}

	return (ISC_R_SUCCESS);
}

dns_rdataset_t *
dns_message_getopt(dns_message_t *msg) {

	/*
	 * Get the OPT record for 'msg'.
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));

	return (msg->opt);
}

isc_result_t
dns_message_setopt(dns_message_t *msg, dns_rdataset_t *opt) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	/*
	 * Set the OPT record for 'msg'.
	 */

	/*
	 * The space required for an OPT record is:
	 *
	 *	1 byte for the name
	 *	2 bytes for the type
	 *	2 bytes for the class
	 *	4 bytes for the ttl
	 *	2 bytes for the rdata length
	 * ---------------------------------
	 *     11 bytes
	 *
	 * plus the length of the rdata.
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(opt->type == dns_rdatatype_opt);
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTRENDER);
	REQUIRE(msg->state == DNS_SECTION_ANY);

	msgresetopt(msg);

	result = dns_rdataset_first(opt);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	dns_rdataset_current(opt, &rdata);
	msg->opt_reserved = 11 + rdata.length;
	result = dns_message_renderreserve(msg, msg->opt_reserved);
	if (result != ISC_R_SUCCESS) {
		msg->opt_reserved = 0;
		goto cleanup;
	}

	msg->opt = opt;

	return (ISC_R_SUCCESS);

 cleanup:
	dns_message_puttemprdataset(msg, &opt);
	return (result);

}

dns_rdataset_t *
dns_message_gettsig(dns_message_t *msg, dns_name_t **owner) {

	/*
	 * Get the TSIG record and owner for 'msg'.
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(owner == NULL || *owner == NULL);

	if (owner != NULL)
		*owner = msg->tsigname;
	return (msg->tsig);
}

isc_result_t
dns_message_settsigkey(dns_message_t *msg, dns_tsigkey_t *key) {
	isc_result_t result;

	/*
	 * Set the TSIG key for 'msg'
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(msg->state == DNS_SECTION_ANY);

	if (key == NULL && msg->tsigkey != NULL) {
		if (msg->sig_reserved != 0) {
			dns_message_renderrelease(msg, msg->sig_reserved);
			msg->sig_reserved = 0;
		}
		dns_tsigkey_detach(&msg->tsigkey);
	}
	if (key != NULL) {
		REQUIRE(msg->tsigkey == NULL && msg->sig0key == NULL);
		dns_tsigkey_attach(key, &msg->tsigkey);
		if (msg->from_to_wire == DNS_MESSAGE_INTENTRENDER) {
			msg->sig_reserved = spacefortsig(msg->tsigkey, 0);
			result = dns_message_renderreserve(msg,
							   msg->sig_reserved);
			if (result != ISC_R_SUCCESS) {
				dns_tsigkey_detach(&msg->tsigkey);
				msg->sig_reserved = 0;
				return (result);
			}
		}
	}
	return (ISC_R_SUCCESS);
}

dns_tsigkey_t *
dns_message_gettsigkey(dns_message_t *msg) {

	/*
	 * Get the TSIG key for 'msg'
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));

	return (msg->tsigkey);
}

isc_result_t
dns_message_setquerytsig(dns_message_t *msg, isc_buffer_t *querytsig) {
	dns_rdata_t *rdata = NULL;
	dns_rdatalist_t *list = NULL;
	dns_rdataset_t *set = NULL;
	isc_buffer_t *buf = NULL;
	isc_region_t r;
	isc_result_t result;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(msg->querytsig == NULL);

	if (querytsig == NULL)
		return (ISC_R_SUCCESS);

	result = dns_message_gettemprdata(msg, &rdata);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	result = dns_message_gettemprdatalist(msg, &list);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	result = dns_message_gettemprdataset(msg, &set);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	isc_buffer_usedregion(querytsig, &r);
	result = isc_buffer_allocate(msg->mctx, &buf, r.length);
	if (result != ISC_R_SUCCESS)
		goto cleanup;
	isc_buffer_putmem(buf, r.base, r.length);
	isc_buffer_usedregion(buf, &r);
	dns_rdata_init(rdata);
	dns_rdata_fromregion(rdata, dns_rdataclass_any, dns_rdatatype_tsig, &r);
	dns_message_takebuffer(msg, &buf);
	ISC_LIST_INIT(list->rdata);
	ISC_LIST_APPEND(list->rdata, rdata, link);
	result = dns_rdatalist_tordataset(list, set);
	if (result != ISC_R_SUCCESS)
		goto cleanup;

	msg->querytsig = set;

	return (result);

 cleanup:
	if (rdata != NULL)
		dns_message_puttemprdata(msg, &rdata);
	if (list != NULL)
		dns_message_puttemprdatalist(msg, &list);
	if (set != NULL)
		dns_message_puttemprdataset(msg, &set);
	return (ISC_R_NOMEMORY);
}

isc_result_t
dns_message_getquerytsig(dns_message_t *msg, isc_mem_t *mctx,
			 isc_buffer_t **querytsig) {
	isc_result_t result;
	dns_rdata_t rdata = DNS_RDATA_INIT;
	isc_region_t r;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(mctx != NULL);
	REQUIRE(querytsig != NULL && *querytsig == NULL);

	if (msg->tsig == NULL)
		return (ISC_R_SUCCESS);

	result = dns_rdataset_first(msg->tsig);
	if (result != ISC_R_SUCCESS)
		return (result);
	dns_rdataset_current(msg->tsig, &rdata);
	dns_rdata_toregion(&rdata, &r);

	result = isc_buffer_allocate(mctx, querytsig, r.length);
	if (result != ISC_R_SUCCESS)
		return (result);
	isc_buffer_putmem(*querytsig, r.base, r.length);
	return (ISC_R_SUCCESS);
}

dns_rdataset_t *
dns_message_getsig0(dns_message_t *msg, dns_name_t **owner) {

	/*
	 * Get the SIG(0) record for 'msg'.
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(owner == NULL || *owner == NULL);

	if (msg->sig0 != NULL && owner != NULL) {
		/* If dns_message_getsig0 is called on a rendered message
		 * after the SIG(0) has been applied, we need to return the
		 * root name, not NULL.
		 */
		if (msg->sig0name == NULL)
			*owner = dns_rootname;
		else
			*owner = msg->sig0name;
	}
	return (msg->sig0);
}

isc_result_t
dns_message_setsig0key(dns_message_t *msg, dst_key_t *key) {
	isc_region_t r;
	unsigned int x;
	isc_result_t result;

	/*
	 * Set the SIG(0) key for 'msg'
	 */

	/*
	 * The space required for an SIG(0) record is:
	 *
	 *	1 byte for the name
	 *	2 bytes for the type
	 *	2 bytes for the class
	 *	4 bytes for the ttl
	 *	2 bytes for the type covered
	 *	1 byte for the algorithm
	 *	1 bytes for the labels
	 *	4 bytes for the original ttl
	 *	4 bytes for the signature expiration
	 *	4 bytes for the signature inception
	 *	2 bytes for the key tag
	 *	n bytes for the signer's name
	 *	x bytes for the signature
	 * ---------------------------------
	 *     27 + n + x bytes
	 */
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTRENDER);
	REQUIRE(msg->state == DNS_SECTION_ANY);

	if (key != NULL) {
		REQUIRE(msg->sig0key == NULL && msg->tsigkey == NULL);
		dns_name_toregion(dst_key_name(key), &r);
		result = dst_key_sigsize(key, &x);
		if (result != ISC_R_SUCCESS) {
			msg->sig_reserved = 0;
			return (result);
		}
		msg->sig_reserved = 27 + r.length + x;
		result = dns_message_renderreserve(msg, msg->sig_reserved);
		if (result != ISC_R_SUCCESS) {
			msg->sig_reserved = 0;
			return (result);
		}
		msg->sig0key = key;
	}
	return (ISC_R_SUCCESS);
}

dst_key_t *
dns_message_getsig0key(dns_message_t *msg) {

	/*
	 * Get the SIG(0) key for 'msg'
	 */

	REQUIRE(DNS_MESSAGE_VALID(msg));

	return (msg->sig0key);
}

void
dns_message_takebuffer(dns_message_t *msg, isc_buffer_t **buffer) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(buffer != NULL);
	REQUIRE(ISC_BUFFER_VALID(*buffer));

	ISC_LIST_APPEND(msg->cleanup, *buffer, link);
	*buffer = NULL;
}

isc_result_t
dns_message_signer(dns_message_t *msg, dns_name_t *signer) {
	isc_result_t result = ISC_R_SUCCESS;
	dns_rdata_t rdata = DNS_RDATA_INIT;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(signer != NULL);
	REQUIRE(msg->from_to_wire == DNS_MESSAGE_INTENTPARSE);

	if (msg->tsig == NULL && msg->sig0 == NULL)
		return (ISC_R_NOTFOUND);

	if (msg->verify_attempted == 0)
		return (DNS_R_NOTVERIFIEDYET);

	if (!dns_name_hasbuffer(signer)) {
		isc_buffer_t *dynbuf = NULL;
		result = isc_buffer_allocate(msg->mctx, &dynbuf, 512);
		if (result != ISC_R_SUCCESS)
			return (result);
		dns_name_setbuffer(signer, dynbuf);
		dns_message_takebuffer(msg, &dynbuf);
	}

	if (msg->sig0 != NULL) {
		dns_rdata_sig_t sig;

		result = dns_rdataset_first(msg->sig0);
		INSIST(result == ISC_R_SUCCESS);
		dns_rdataset_current(msg->sig0, &rdata);

		result = dns_rdata_tostruct(&rdata, &sig, NULL);
		if (result != ISC_R_SUCCESS)
			return (result);

		if (msg->verified_sig && msg->sig0status == dns_rcode_noerror)
			result = ISC_R_SUCCESS;
		else
			result = DNS_R_SIGINVALID;
		dns_name_clone(&sig.signer, signer);
		dns_rdata_freestruct(&sig);
	} else {
		dns_name_t *identity;
		dns_rdata_any_tsig_t tsig;

		result = dns_rdataset_first(msg->tsig);
		INSIST(result == ISC_R_SUCCESS);
		dns_rdataset_current(msg->tsig, &rdata);

		result = dns_rdata_tostruct(&rdata, &tsig, NULL);
		if (msg->tsigstatus != dns_rcode_noerror)
			result = DNS_R_TSIGVERIFYFAILURE;
		else if (tsig.error != dns_rcode_noerror)
			result = DNS_R_TSIGERRORSET;
		else
			result = ISC_R_SUCCESS;
		dns_rdata_freestruct(&tsig);

		if (msg->tsigkey == NULL) {
			/*
			 * If msg->tsigstatus & tsig.error are both
			 * dns_rcode_noerror, the message must have been
			 * verified, which means msg->tsigkey will be
			 * non-NULL.
			 */
			INSIST(result != ISC_R_SUCCESS);
		} else {
			identity = dns_tsigkey_identity(msg->tsigkey);
			if (identity == NULL) {
				if (result == ISC_R_SUCCESS)
					result = DNS_R_NOIDENTITY;
				identity = &msg->tsigkey->name;
			}
			dns_name_clone(identity, signer);
		}
	}

	return (result);
}

void
dns_message_resetsig(dns_message_t *msg) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	msg->verified_sig = 0;
	msg->verify_attempted = 0;
	msg->tsigstatus = dns_rcode_noerror;
	msg->sig0status = dns_rcode_noerror;
	msg->timeadjust = 0;
	if (msg->tsigkey != NULL) {
		dns_tsigkey_detach(&msg->tsigkey);
		msg->tsigkey = NULL;
	}
}

isc_result_t
dns_message_rechecksig(dns_message_t *msg, dns_view_t *view) {
	dns_message_resetsig(msg);
	return (dns_message_checksig(msg, view));
}

#ifdef SKAN_MSG_DEBUG
void
dns_message_dumpsig(dns_message_t *msg, char *txt1) {
	dns_rdata_t querytsigrdata = DNS_RDATA_INIT;
	dns_rdata_any_tsig_t querytsig;
	isc_result_t result;

	if (msg->tsig != NULL) {
		result = dns_rdataset_first(msg->tsig);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		dns_rdataset_current(msg->tsig, &querytsigrdata);
		result = dns_rdata_tostruct(&querytsigrdata, &querytsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		hexdump(txt1, "TSIG", querytsig.signature,
			querytsig.siglen);
	}

	if (msg->querytsig != NULL) {
		result = dns_rdataset_first(msg->querytsig);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		dns_rdataset_current(msg->querytsig, &querytsigrdata);
		result = dns_rdata_tostruct(&querytsigrdata, &querytsig, NULL);
		RUNTIME_CHECK(result == ISC_R_SUCCESS);
		hexdump(txt1, "QUERYTSIG", querytsig.signature,
			querytsig.siglen);
	}
}
#endif

isc_result_t
dns_message_checksig(dns_message_t *msg, dns_view_t *view) {
	isc_buffer_t b, msgb;

	REQUIRE(DNS_MESSAGE_VALID(msg));

	if (msg->tsigkey == NULL && msg->tsig == NULL && msg->sig0 == NULL)
		return (ISC_R_SUCCESS);

	INSIST(msg->saved.base != NULL);
	isc_buffer_init(&msgb, msg->saved.base, msg->saved.length);
	isc_buffer_add(&msgb, msg->saved.length);
	if (msg->tsigkey != NULL || msg->tsig != NULL) {
#ifdef SKAN_MSG_DEBUG
		dns_message_dumpsig(msg, "dns_message_checksig#1");
#endif
		if (view != NULL)
			return (dns_view_checksig(view, &msgb, msg));
		else
			return (dns_tsig_verify(&msgb, msg, NULL, NULL));
	} else {
		dns_rdata_t rdata = DNS_RDATA_INIT;
		dns_rdata_sig_t sig;
		dns_rdataset_t keyset;
		isc_result_t result;

		result = dns_rdataset_first(msg->sig0);
		INSIST(result == ISC_R_SUCCESS);
		dns_rdataset_current(msg->sig0, &rdata);

		/*
		 * This can occur when the message is a dynamic update, since
		 * the rdata length checking is relaxed.  This should not
		 * happen in a well-formed message, since the SIG(0) is only
		 * looked for in the additional section, and the dynamic update
		 * meta-records are in the prerequisite and update sections.
		 */
		if (rdata.length == 0)
			return (ISC_R_UNEXPECTEDEND);

		result = dns_rdata_tostruct(&rdata, &sig, msg->mctx);
		if (result != ISC_R_SUCCESS)
			return (result);

		dns_rdataset_init(&keyset);
		if (view == NULL)
			return (DNS_R_KEYUNAUTHORIZED);
		result = dns_view_simplefind(view, &sig.signer,
					     dns_rdatatype_key /* SIG(0) */,
					     0, 0, ISC_FALSE, &keyset, NULL);

		if (result != ISC_R_SUCCESS) {
			/* XXXBEW Should possibly create a fetch here */
			result = DNS_R_KEYUNAUTHORIZED;
			goto freesig;
		} else if (keyset.trust < dns_trust_secure) {
			/* XXXBEW Should call a validator here */
			result = DNS_R_KEYUNAUTHORIZED;
			goto freesig;
		}
		result = dns_rdataset_first(&keyset);
		INSIST(result == ISC_R_SUCCESS);
		for (;
		     result == ISC_R_SUCCESS;
		     result = dns_rdataset_next(&keyset))
		{
			dst_key_t *key = NULL;

			dns_rdata_reset(&rdata);
			dns_rdataset_current(&keyset, &rdata);
			isc_buffer_init(&b, rdata.data, rdata.length);
			isc_buffer_add(&b, rdata.length);

			result = dst_key_fromdns(&sig.signer, rdata.rdclass,
						 &b, view->mctx, &key);
			if (result != ISC_R_SUCCESS)
				continue;
			if (dst_key_alg(key) != sig.algorithm ||
			    dst_key_id(key) != sig.keyid ||
			    !(dst_key_proto(key) == DNS_KEYPROTO_DNSSEC ||
			      dst_key_proto(key) == DNS_KEYPROTO_ANY))
			{
				dst_key_free(&key);
				continue;
			}
			result = dns_dnssec_verifymessage(&msgb, msg, key);
			dst_key_free(&key);
			if (result == ISC_R_SUCCESS)
				break;
		}
		if (result == ISC_R_NOMORE)
			result = DNS_R_KEYUNAUTHORIZED;

 freesig:
		if (dns_rdataset_isassociated(&keyset))
			dns_rdataset_disassociate(&keyset);
		dns_rdata_freestruct(&sig);
		return (result);
	}
}

isc_result_t
dns_message_sectiontotext(dns_message_t *msg, dns_section_t section,
			  const dns_master_style_t *style,
			  dns_messagetextflag_t flags,
			  isc_buffer_t *target) {
	dns_name_t *name, empty_name;
	dns_rdataset_t *rdataset;
	isc_result_t result;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(target != NULL);
	REQUIRE(VALID_SECTION(section));

	if (ISC_LIST_EMPTY(msg->sections[section]))
		return (ISC_R_SUCCESS);

	if ((flags & DNS_MESSAGETEXTFLAG_NOCOMMENTS) == 0) {
		ADD_STRING(target, ";; ");
		if (msg->opcode != dns_opcode_update) {
			ADD_STRING(target, sectiontext[section]);
		} else {
			ADD_STRING(target, updsectiontext[section]);
		}
		ADD_STRING(target, " SECTION:\n");
	}

	dns_name_init(&empty_name, NULL);
	result = dns_message_firstname(msg, section);
	if (result != ISC_R_SUCCESS) {
		return (result);
	}
	do {
		name = NULL;
		dns_message_currentname(msg, section, &name);
		for (rdataset = ISC_LIST_HEAD(name->list);
		     rdataset != NULL;
		     rdataset = ISC_LIST_NEXT(rdataset, link)) {
			if (section == DNS_SECTION_QUESTION) {
				ADD_STRING(target, ";");
				result = dns_master_questiontotext(name,
								   rdataset,
								   style,
								   target);
			} else {
				result = dns_master_rdatasettotext(name,
								   rdataset,
								   style,
								   target);
			}
			if (result != ISC_R_SUCCESS)
				return (result);
		}
		result = dns_message_nextname(msg, section);
	} while (result == ISC_R_SUCCESS);
	if ((flags & DNS_MESSAGETEXTFLAG_NOHEADERS) == 0 &&
	    (flags & DNS_MESSAGETEXTFLAG_NOCOMMENTS) == 0)
		ADD_STRING(target, "\n");
	if (result == ISC_R_NOMORE)
		result = ISC_R_SUCCESS;
	return (result);
}

isc_result_t
dns_message_pseudosectiontotext(dns_message_t *msg,
				dns_pseudosection_t section,
				const dns_master_style_t *style,
				dns_messagetextflag_t flags,
				isc_buffer_t *target) {
	dns_rdataset_t *ps = NULL;
	dns_name_t *name = NULL;
	isc_result_t result;
	char buf[sizeof("1234567890")];
	isc_uint32_t mbz;
	dns_rdata_t rdata;
	isc_buffer_t optbuf;
	isc_uint16_t optcode, optlen;
	unsigned char *optdata;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(target != NULL);
	REQUIRE(VALID_PSEUDOSECTION(section));

	switch (section) {
	case DNS_PSEUDOSECTION_OPT:
		ps = dns_message_getopt(msg);
		if (ps == NULL)
			return (ISC_R_SUCCESS);
		if ((flags & DNS_MESSAGETEXTFLAG_NOCOMMENTS) == 0)
			ADD_STRING(target, ";; OPT PSEUDOSECTION:\n");
		ADD_STRING(target, "; EDNS: version: ");
		snprintf(buf, sizeof(buf), "%u",
			 (unsigned int)((ps->ttl & 0x00ff0000) >> 16));
		ADD_STRING(target, buf);
		ADD_STRING(target, ", flags:");
		if ((ps->ttl & DNS_MESSAGEEXTFLAG_DO) != 0)
			ADD_STRING(target, " do");
		mbz = ps->ttl & ~DNS_MESSAGEEXTFLAG_DO & 0xffff;
		if (mbz != 0) {
			ADD_STRING(target, "; MBZ: ");
			snprintf(buf, sizeof(buf), "%.4x ", mbz);
			ADD_STRING(target, buf);
			ADD_STRING(target, ", udp: ");
		} else
			ADD_STRING(target, "; udp: ");
		snprintf(buf, sizeof(buf), "%u\n", (unsigned int)ps->rdclass);
		ADD_STRING(target, buf);

		result = dns_rdataset_first(ps);
		if (result != ISC_R_SUCCESS)
			return (ISC_R_SUCCESS);

		/* Print EDNS info, if any */
		dns_rdata_init(&rdata);
		dns_rdataset_current(ps, &rdata);
		if (rdata.length < 4)
			return (ISC_R_SUCCESS);

		isc_buffer_init(&optbuf, rdata.data, rdata.length);
		isc_buffer_add(&optbuf, rdata.length);
		optcode = isc_buffer_getuint16(&optbuf);
		optlen = isc_buffer_getuint16(&optbuf);

		if (optcode == DNS_OPT_NSID) {
			ADD_STRING(target, "; NSID");
		} else {
			ADD_STRING(target, "; OPT=");
			sprintf(buf, "%u", optcode);
			ADD_STRING(target, buf);
		}

		if (optlen != 0) {
			int i;
			ADD_STRING(target, ": ");

			optdata = rdata.data + 4;
			for (i = 0; i < optlen; i++) {
				sprintf(buf, "%02x ", optdata[i]);
				ADD_STRING(target, buf);
			}
			for (i = 0; i < optlen; i++) {
				ADD_STRING(target, " (");
				if (isprint(optdata[i]))
					isc_buffer_putmem(target, &optdata[i],
							  1);
				else
					isc_buffer_putstr(target, ".");
				ADD_STRING(target, ")");
			}
		}
		ADD_STRING(target, "\n");
		return (ISC_R_SUCCESS);
	case DNS_PSEUDOSECTION_TSIG:
		ps = dns_message_gettsig(msg, &name);
		if (ps == NULL)
			return (ISC_R_SUCCESS);
		if ((flags & DNS_MESSAGETEXTFLAG_NOCOMMENTS) == 0)
			ADD_STRING(target, ";; TSIG PSEUDOSECTION:\n");
		result = dns_master_rdatasettotext(name, ps, style, target);
		if ((flags & DNS_MESSAGETEXTFLAG_NOHEADERS) == 0 &&
		    (flags & DNS_MESSAGETEXTFLAG_NOCOMMENTS) == 0)
			ADD_STRING(target, "\n");
		return (result);
	case DNS_PSEUDOSECTION_SIG0:
		ps = dns_message_getsig0(msg, &name);
		if (ps == NULL)
			return (ISC_R_SUCCESS);
		if ((flags & DNS_MESSAGETEXTFLAG_NOCOMMENTS) == 0)
			ADD_STRING(target, ";; SIG0 PSEUDOSECTION:\n");
		result = dns_master_rdatasettotext(name, ps, style, target);
		if ((flags & DNS_MESSAGETEXTFLAG_NOHEADERS) == 0 &&
		    (flags & DNS_MESSAGETEXTFLAG_NOCOMMENTS) == 0)
			ADD_STRING(target, "\n");
		return (result);
	}
	return (ISC_R_UNEXPECTED);
}

isc_result_t
dns_message_totext(dns_message_t *msg, const dns_master_style_t *style,
		   dns_messagetextflag_t flags, isc_buffer_t *target) {
	char buf[sizeof("1234567890")];
	isc_result_t result;

	REQUIRE(DNS_MESSAGE_VALID(msg));
	REQUIRE(target != NULL);

	if ((flags & DNS_MESSAGETEXTFLAG_NOHEADERS) == 0) {
		ADD_STRING(target, ";; ->>HEADER<<- opcode: ");
		ADD_STRING(target, opcodetext[msg->opcode]);
		ADD_STRING(target, ", status: ");
		if (msg->rcode < (sizeof(rcodetext)/sizeof(rcodetext[0]))) {
			ADD_STRING(target, rcodetext[msg->rcode]);
		} else {
			snprintf(buf, sizeof(buf), "%4u", msg->rcode);
			ADD_STRING(target, buf);
		}
		ADD_STRING(target, ", id: ");
		snprintf(buf, sizeof(buf), "%6u", msg->id);
		ADD_STRING(target, buf);
		ADD_STRING(target, "\n;; flags: ");
		if ((msg->flags & DNS_MESSAGEFLAG_QR) != 0)
			ADD_STRING(target, "qr ");
		if ((msg->flags & DNS_MESSAGEFLAG_AA) != 0)
			ADD_STRING(target, "aa ");
		if ((msg->flags & DNS_MESSAGEFLAG_TC) != 0)
			ADD_STRING(target, "tc ");
		if ((msg->flags & DNS_MESSAGEFLAG_RD) != 0)
			ADD_STRING(target, "rd ");
		if ((msg->flags & DNS_MESSAGEFLAG_RA) != 0)
			ADD_STRING(target, "ra ");
		if ((msg->flags & DNS_MESSAGEFLAG_AD) != 0)
			ADD_STRING(target, "ad ");
		if ((msg->flags & DNS_MESSAGEFLAG_CD) != 0)
			ADD_STRING(target, "cd ");
		if (msg->opcode != dns_opcode_update) {
			ADD_STRING(target, "; QUESTION: ");
		} else {
			ADD_STRING(target, "; ZONE: ");
		}
		snprintf(buf, sizeof(buf), "%1u",
			 msg->counts[DNS_SECTION_QUESTION]);
		ADD_STRING(target, buf);
		if (msg->opcode != dns_opcode_update) {
			ADD_STRING(target, ", ANSWER: ");
		} else {
			ADD_STRING(target, ", PREREQ: ");
		}
		snprintf(buf, sizeof(buf), "%1u",
			 msg->counts[DNS_SECTION_ANSWER]);
		ADD_STRING(target, buf);
		if (msg->opcode != dns_opcode_update) {
			ADD_STRING(target, ", AUTHORITY: ");
		} else {
			ADD_STRING(target, ", UPDATE: ");
		}
		snprintf(buf, sizeof(buf), "%1u",
			msg->counts[DNS_SECTION_AUTHORITY]);
		ADD_STRING(target, buf);
		ADD_STRING(target, ", ADDITIONAL: ");
		snprintf(buf, sizeof(buf), "%1u",
			msg->counts[DNS_SECTION_ADDITIONAL]);
		ADD_STRING(target, buf);
		ADD_STRING(target, "\n");
	}
	result = dns_message_pseudosectiontotext(msg,
						 DNS_PSEUDOSECTION_OPT,
						 style, flags, target);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_message_sectiontotext(msg, DNS_SECTION_QUESTION,
					   style, flags, target);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = dns_message_sectiontotext(msg, DNS_SECTION_ANSWER,
					   style, flags, target);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = dns_message_sectiontotext(msg, DNS_SECTION_AUTHORITY,
					   style, flags, target);
	if (result != ISC_R_SUCCESS)
		return (result);
	result = dns_message_sectiontotext(msg, DNS_SECTION_ADDITIONAL,
					   style, flags, target);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_message_pseudosectiontotext(msg,
						 DNS_PSEUDOSECTION_TSIG,
						 style, flags, target);
	if (result != ISC_R_SUCCESS)
		return (result);

	result = dns_message_pseudosectiontotext(msg,
						 DNS_PSEUDOSECTION_SIG0,
						 style, flags, target);
	if (result != ISC_R_SUCCESS)
		return (result);

	return (ISC_R_SUCCESS);
}

isc_region_t *
dns_message_getrawmessage(dns_message_t *msg) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	return (&msg->saved);
}

void
dns_message_setsortorder(dns_message_t *msg, dns_rdatasetorderfunc_t order,
			 const void *order_arg)
{
	REQUIRE(DNS_MESSAGE_VALID(msg));
	msg->order = order;
	msg->order_arg = order_arg;
}

void
dns_message_settimeadjust(dns_message_t *msg, int timeadjust) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	msg->timeadjust = timeadjust;
}

int
dns_message_gettimeadjust(dns_message_t *msg) {
	REQUIRE(DNS_MESSAGE_VALID(msg));
	return (msg->timeadjust);
}

isc_result_t
dns_opcode_totext(dns_opcode_t opcode, isc_buffer_t *target) {

	REQUIRE(opcode < 16);

	if (isc_buffer_availablelength(target) < strlen(opcodetext[opcode]))
		return (ISC_R_NOSPACE);
	isc_buffer_putstr(target, opcodetext[opcode]);
	return (ISC_R_SUCCESS);
}
