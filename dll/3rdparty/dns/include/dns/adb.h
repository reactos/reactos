/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: adb.h,v 1.85 2008/04/03 06:09:04 tbox Exp $ */

#ifndef DNS_ADB_H
#define DNS_ADB_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/adb.h
 *\brief
 * DNS Address Database
 *
 * This module implements an address database (ADB) for mapping a name
 * to an isc_sockaddr_t. It also provides statistical information on
 * how good that address might be.
 *
 * A client will pass in a dns_name_t, and the ADB will walk through
 * the rdataset looking up addresses associated with the name.  If it
 * is found on the internal lists, a structure is filled in with the
 * address information and stats for found addresses.
 *
 * If the name cannot be found on the internal lists, a new entry will
 * be created for a name if all the information needed can be found
 * in the zone table or cache.  This new address will then be returned.
 *
 * If a request must be made to remote servers to satisfy a name lookup,
 * this module will start fetches to try to complete these addresses.  When
 * at least one more completes, an event is sent to the caller.  If none of
 * them resolve before the fetch times out, an event indicating this is
 * sent instead.
 *
 * Records are stored internally until a timer expires. The timer is the
 * smaller of the TTL or signature validity period.
 *
 * Lameness is stored per <qname,qtype> tuple, and this data hangs off each
 * address field.  When an address is marked lame for a given tuple the address
 * will not be returned to a caller.
 *
 *
 * MP:
 *
 *\li	The ADB takes care of all necessary locking.
 *
 *\li	Only the task which initiated the name lookup can cancel the lookup.
 *
 *
 * Security:
 *
 *\li	None, since all data stored is required to be pre-filtered.
 *	(Cache needs to be sane, fetches return bounds-checked and sanity-
 *       checked data, caller passes a good dns_name_t for the zone, etc)
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/mem.h>
#include <isc/sockaddr.h>

#include <dns/types.h>
#include <dns/view.h>

ISC_LANG_BEGINDECLS

/***
 *** Magic number checks
 ***/

#define DNS_ADBFIND_MAGIC	  ISC_MAGIC('a','d','b','H')
#define DNS_ADBFIND_VALID(x)	  ISC_MAGIC_VALID(x, DNS_ADBFIND_MAGIC)
#define DNS_ADBADDRINFO_MAGIC	  ISC_MAGIC('a','d','A','I')
#define DNS_ADBADDRINFO_VALID(x)  ISC_MAGIC_VALID(x, DNS_ADBADDRINFO_MAGIC)


/***
 *** TYPES
 ***/

typedef struct dns_adbname		dns_adbname_t;

/*!
 *\brief
 * Represents a lookup for a single name.
 *
 * On return, the client can safely use "list", and can reorder the list.
 * Items may not be _deleted_ from this list, however, or added to it
 * other than by using the dns_adb_*() API.
 */
struct dns_adbfind {
	/* Public */
	unsigned int			magic;		/*%< RO: magic */
	dns_adbaddrinfolist_t		list;		/*%< RO: list of addrs */
	unsigned int			query_pending;	/*%< RO: partial list */
	unsigned int			partial_result;	/*%< RO: addrs missing */
	unsigned int			options;	/*%< RO: options */
	isc_result_t			result_v4;	/*%< RO: v4 result */
	isc_result_t			result_v6;	/*%< RO: v6 result */
	ISC_LINK(dns_adbfind_t)		publink;	/*%< RW: client use */

	/* Private */
	isc_mutex_t			lock;		/* locks all below */
	in_port_t			port;
	int				name_bucket;
	unsigned int			flags;
	dns_adbname_t		       *adbname;
	dns_adb_t		       *adb;
	isc_event_t			event;
	ISC_LINK(dns_adbfind_t)		plink;
};

/*
 * _INET:
 * _INET6:
 *	return addresses of that type.
 *
 * _EMPTYEVENT:
 *	Only schedule an event if no addresses are known.
 *	Must set _WANTEVENT for this to be meaningful.
 *
 * _WANTEVENT:
 *	An event is desired.  Check this bit in the returned find to see
 *	if one will actually be generated.
 *
 * _AVOIDFETCHES:
 *	If set, fetches will not be generated unless no addresses are
 *	available in any of the address families requested.
 *
 * _STARTATZONE:
 *	Fetches will start using the closest zone data or use the root servers.
 *	This is useful for reestablishing glue that has expired.
 *
 * _GLUEOK:
 * _HINTOK:
 *	Glue or hints are ok.  These are used when matching names already
 *	in the adb, and when dns databases are searched.
 *
 * _RETURNLAME:
 *	Return lame servers in a find, so that all addresses are returned.
 *
 * _LAMEPRUNED:
 *	At least one address was omitted from the list because it was lame.
 *	This bit will NEVER be set if _RETURNLAME is set in the createfind().
 */
/*% Return addresses of type INET. */
#define DNS_ADBFIND_INET		0x00000001
/*% Return addresses of type INET6. */
#define DNS_ADBFIND_INET6		0x00000002
#define DNS_ADBFIND_ADDRESSMASK		0x00000003
/*%
 *      Only schedule an event if no addresses are known.
 *      Must set _WANTEVENT for this to be meaningful.
 */
#define DNS_ADBFIND_EMPTYEVENT		0x00000004
/*%
 *	An event is desired.  Check this bit in the returned find to see
 *	if one will actually be generated.
 */
#define DNS_ADBFIND_WANTEVENT		0x00000008
/*%
 *	If set, fetches will not be generated unless no addresses are
 *	available in any of the address families requested.
 */
#define DNS_ADBFIND_AVOIDFETCHES	0x00000010
/*%
 *	Fetches will start using the closest zone data or use the root servers.
 *	This is useful for reestablishing glue that has expired.
 */
#define DNS_ADBFIND_STARTATZONE		0x00000020
/*%
 *	Glue or hints are ok.  These are used when matching names already
 *	in the adb, and when dns databases are searched.
 */
#define DNS_ADBFIND_GLUEOK		0x00000040
/*%
 *	Glue or hints are ok.  These are used when matching names already
 *	in the adb, and when dns databases are searched.
 */
#define DNS_ADBFIND_HINTOK		0x00000080
/*%
 *	Return lame servers in a find, so that all addresses are returned.
 */
#define DNS_ADBFIND_RETURNLAME		0x00000100
/*%
 *      Only schedule an event if no addresses are known.
 *      Must set _WANTEVENT for this to be meaningful.
 */
#define DNS_ADBFIND_LAMEPRUNED		0x00000200

/*%
 * The answers to queries come back as a list of these.
 */
struct dns_adbaddrinfo {
	unsigned int			magic;		/*%< private */

	isc_sockaddr_t			sockaddr;	/*%< [rw] */
	unsigned int			srtt;		/*%< [rw] microseconds */
	unsigned int			flags;		/*%< [rw] */
	dns_adbentry_t		       *entry;		/*%< private */
	ISC_LINK(dns_adbaddrinfo_t)	publink;
};

/*!<
 * The event sent to the caller task is just a plain old isc_event_t.  It
 * contains no data other than a simple status, passed in the "type" field
 * to indicate that another address resolved, or all partially resolved
 * addresses have failed to resolve.
 *
 * "sender" is the dns_adbfind_t used to issue this query.
 *
 * This is simply a standard event, with the "type" set to:
 *
 *\li	#DNS_EVENT_ADBMOREADDRESSES   -- another address resolved.
 *\li	#DNS_EVENT_ADBNOMOREADDRESSES -- all pending addresses failed,
 *					were canceled, or otherwise will
 *					not be usable.
 *\li	#DNS_EVENT_ADBCANCELED	     -- The request was canceled by a
 *					3rd party.
 *\li	#DNS_EVENT_ADBNAMEDELETED     -- The name was deleted, so this request
 *					was canceled.
 *
 * In each of these cases, the addresses returned by the initial call
 * to dns_adb_createfind() can still be used until they are no longer needed.
 */

/****
 **** FUNCTIONS
 ****/


isc_result_t
dns_adb_create(isc_mem_t *mem, dns_view_t *view, isc_timermgr_t *tmgr,
	       isc_taskmgr_t *taskmgr, dns_adb_t **newadb);
/*%<
 * Create a new ADB.
 *
 * Notes:
 *
 *\li	Generally, applications should not create an ADB directly, but
 *	should instead call dns_view_createresolver().
 *
 * Requires:
 *
 *\li	'mem' must be a valid memory context.
 *
 *\li	'view' be a pointer to a valid view.
 *
 *\li	'tmgr' be a pointer to a valid timer manager.
 *
 *\li	'taskmgr' be a pointer to a valid task manager.
 *
 *\li	'newadb' != NULL && '*newadb' == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS	after happiness.
 *\li	#ISC_R_NOMEMORY	after resource allocation failure.
 */

void
dns_adb_attach(dns_adb_t *adb, dns_adb_t **adbp);
/*%
 * Attach to an 'adb' to 'adbp'.
 *
 * Requires:
 *\li	'adb' to be a valid dns_adb_t, created via dns_adb_create().
 *\li	'adbp' to be a valid pointer to a *dns_adb_t which is initialized
 *	to NULL.
 */

void
dns_adb_detach(dns_adb_t **adb);
/*%
 * Delete the ADB. Sets *ADB to NULL. Cancels any outstanding requests.
 *
 * Requires:
 *
 *\li	'adb' be non-NULL and '*adb' be a valid dns_adb_t, created via
 *	dns_adb_create().
 */

void
dns_adb_whenshutdown(dns_adb_t *adb, isc_task_t *task, isc_event_t **eventp);
/*%
 * Send '*eventp' to 'task' when 'adb' has shutdown.
 *
 * Requires:
 *
 *\li	'*adb' is a valid dns_adb_t.
 *
 *\li	eventp != NULL && *eventp is a valid event.
 *
 * Ensures:
 *
 *\li	*eventp == NULL
 *
 *\li	The event's sender field is set to the value of adb when the event
 *	is sent.
 */

void
dns_adb_shutdown(dns_adb_t *adb);
/*%<
 * Shutdown 'adb'.
 *
 * Requires:
 *
 * \li	'*adb' is a valid dns_adb_t.
 */

isc_result_t
dns_adb_createfind(dns_adb_t *adb, isc_task_t *task, isc_taskaction_t action,
		   void *arg, dns_name_t *name, dns_name_t *qname,
		   dns_rdatatype_t qtype, unsigned int options,
		   isc_stdtime_t now, dns_name_t *target,
		   in_port_t port, dns_adbfind_t **find);
/*%<
 * Main interface for clients. The adb will look up the name given in
 * "name" and will build up a list of found addresses, and perhaps start
 * internal fetches to resolve names that are unknown currently.
 *
 * If other addresses resolve after this call completes, an event will
 * be sent to the <task, taskaction, arg> with the sender of that event
 * set to a pointer to the dns_adbfind_t returned by this function.
 *
 * If no events will be generated, the *find->result_v4 and/or result_v6
 * members may be examined for address lookup status.  The usual #ISC_R_SUCCESS,
 * #ISC_R_FAILURE, #DNS_R_NXDOMAIN, and #DNS_R_NXRRSET are returned, along with
 * #ISC_R_NOTFOUND meaning the ADB has not _yet_ found the values.  In this
 * latter case, retrying may produce more addresses.
 *
 * If events will be returned, the result_v[46] members are only valid
 * when that event is actually returned.
 *
 * The list of addresses returned is unordered.  The caller must impose
 * any ordering required.  The list will not contain "known bad" addresses,
 * however.  For instance, it will not return hosts that are known to be
 * lame for the zone in question.
 *
 * The caller cannot (directly) modify the contents of the address list's
 * fields other than the "link" field.  All values can be read at any
 * time, however.
 *
 * The "now" parameter is used only for determining which entries that
 * have a specific time to live or expire time should be removed from
 * the running database.  If specified as zero, the current time will
 * be retrieved and used.
 *
 * If 'target' is not NULL and 'name' is an alias (i.e. the name is
 * CNAME'd or DNAME'd to another name), then 'target' will be updated with
 * the domain name that 'name' is aliased to.
 *
 * All addresses returned will have the sockaddr's port set to 'port.'
 * The caller may change them directly in the dns_adbaddrinfo_t since
 * they are copies of the internal address only.
 *
 * XXXMLG  Document options, especially the flags which control how
 *         events are sent.
 *
 * Requires:
 *
 *\li	*adb be a valid isc_adb_t object.
 *
 *\li	If events are to be sent, *task be a valid task,
 *	and isc_taskaction_t != NULL.
 *
 *\li	*name is a valid dns_name_t.
 *
 *\li	qname != NULL and *qname be a valid dns_name_t.
 *
 *\li	target == NULL or target is a valid name with a buffer.
 *
 *\li	find != NULL && *find == NULL.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS	Addresses might have been returned, and events will be
 *			delivered for unresolved addresses.
 *\li	#ISC_R_NOMORE	Addresses might have been returned, but no events
 *			will ever be posted for this context.  This is only
 *			returned if task != NULL.
 *\li	#ISC_R_NOMEMORY	insufficient resources
 *\li	#DNS_R_ALIAS	'name' is an alias for another name.
 *
 * Calls, and returns error codes from:
 *
 *\li	isc_stdtime_get()
 *
 * Notes:
 *
 *\li	No internal reference to "name" exists after this function
 *	returns.
 */

void
dns_adb_cancelfind(dns_adbfind_t *find);
/*%<
 * Cancels the find, and sends the event off to the caller.
 *
 * It is an error to call dns_adb_cancelfind() on a find where
 * no event is wanted, or will ever be sent.
 *
 * Note:
 *
 *\li	It is possible that the real completion event was posted just
 *	before the dns_adb_cancelfind() call was made.  In this case,
 *	dns_adb_cancelfind() will do nothing.  The event callback needs
 *	to be prepared to find this situation (i.e. result is valid but
 *	the caller expects it to be canceled).
 *
 * Requires:
 *
 *\li	'find' be a valid dns_adbfind_t pointer.
 *
 *\li	events would have been posted to the task.  This can be checked
 *	with (find->options & DNS_ADBFIND_WANTEVENT).
 *
 * Ensures:
 *
 *\li	The event was posted to the task.
 */

void
dns_adb_destroyfind(dns_adbfind_t **find);
/*%<
 * Destroys the find reference.
 *
 * Note:
 *
 *\li	This can only be called after the event was delivered for a
 *	find.  Additionally, the event MUST have been freed via
 *	isc_event_free() BEFORE this function is called.
 *
 * Requires:
 *
 *\li	'find' != NULL and *find be valid dns_adbfind_t pointer.
 *
 * Ensures:
 *
 *\li	No "address found" events will be posted to the originating task
 *	after this function returns.
 */

void
dns_adb_dump(dns_adb_t *adb, FILE *f);
/*%<
 * This function is only used for debugging.  It will dump as much of the
 * state of the running system as possible.
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	f != NULL, and is a file open for writing.
 */

void
dns_adb_dumpfind(dns_adbfind_t *find, FILE *f);
/*%<
 * This function is only used for debugging.  Dump the data associated
 * with a find.
 *
 * Requires:
 *
 *\li	find is valid.
 *
 * \li	f != NULL, and is a file open for writing.
 */

isc_result_t
dns_adb_marklame(dns_adb_t *adb, dns_adbaddrinfo_t *addr, dns_name_t *qname,
		 dns_rdatatype_t type, isc_stdtime_t expire_time);
/*%<
 * Mark the given address as lame for the <qname,qtype>.  expire_time should
 * be set to the time when the entry should expire.  That is, if it is to
 * expire 10 minutes in the future, it should set it to (now + 10 * 60).
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 *
 *\li	qname be the qname used in the dns_adb_createfind() call.
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS		-- all is well.
 *\li	#ISC_R_NOMEMORY		-- could not mark address as lame.
 */

/*
 * A reasonable default for RTT adjustments
 */
#define DNS_ADB_RTTADJDEFAULT		7	/*%< default scale */
#define DNS_ADB_RTTADJREPLACE		0	/*%< replace with our rtt */
#define DNS_ADB_RTTADJAGE		10	/*%< age this rtt */

void
dns_adb_adjustsrtt(dns_adb_t *adb, dns_adbaddrinfo_t *addr,
		   unsigned int rtt, unsigned int factor);
/*%<
 * Mix the round trip time into the existing smoothed rtt.

 * The formula used
 * (where srtt is the existing rtt value, and rtt and factor are arguments to
 * this function):
 *
 *\code
 *	new_srtt = (old_srtt / 10 * factor) + (rtt / 10 * (10 - factor));
 *\endcode
 *
 * XXXRTH  Do we want to publish the formula?  What if we want to change how
 *         this works later on?  Recommend/require that the units are
 *	   microseconds?
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 *
 *\li	0 <= factor <= 10
 *
 * Note:
 *
 *\li	The srtt in addr will be updated to reflect the new global
 *	srtt value.  This may include changes made by others.
 */

void
dns_adb_changeflags(dns_adb_t *adb, dns_adbaddrinfo_t *addr,
		    unsigned int bits, unsigned int mask);
/*%
 * Change Flags.
 *
 * Set the flags as given by:
 *
 *\li	newflags = (oldflags & ~mask) | (bits & mask);
 *
 * Requires:
 *
 *\li	adb be valid.
 *
 *\li	addr be valid.
 */

isc_result_t
dns_adb_findaddrinfo(dns_adb_t *adb, isc_sockaddr_t *sa,
		     dns_adbaddrinfo_t **addrp, isc_stdtime_t now);
/*%<
 * Return a dns_adbaddrinfo_t that is associated with address 'sa'.
 *
 * Requires:
 *
 *\li	adb is valid.
 *
 *\li	sa is valid.
 *
 *\li	addrp != NULL && *addrp == NULL
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SHUTTINGDOWN
 */

void
dns_adb_freeaddrinfo(dns_adb_t *adb, dns_adbaddrinfo_t **addrp);
/*%<
 * Free a dns_adbaddrinfo_t allocated by dns_adb_findaddrinfo().
 *
 * Requires:
 *
 *\li	adb is valid.
 *
 *\li	*addrp is a valid dns_adbaddrinfo_t *.
 */

void
dns_adb_flush(dns_adb_t *adb);
/*%<
 * Flushes all cached data from the adb.
 *
 * Requires:
 *\li 	adb is valid.
 */

void
dns_adb_setadbsize(dns_adb_t *adb, isc_uint32_t size);
/*%<
 * Set a target memory size.  If memory usage exceeds the target
 * size entries will be removed before they would have expired on
 * a random basis.
 *
 * If 'size' is 0 then memory usage is unlimited.
 *
 * Requires:
 *\li	'adb' is valid.
 */

void
dns_adb_flushname(dns_adb_t *adb, dns_name_t *name);
/*%<
 * Flush 'name' from the adb cache.
 *
 * Requires:
 *\li	'adb' is valid.
 *\li	'name' is valid.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ADB_H */
