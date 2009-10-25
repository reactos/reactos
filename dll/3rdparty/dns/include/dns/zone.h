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

/* $Id: zone.h,v 1.160.50.4 2009/01/29 22:40:35 jinmei Exp $ */

#ifndef DNS_ZONE_H
#define DNS_ZONE_H 1

/*! \file dns/zone.h */

/***
 ***	Imports
 ***/

#include <stdio.h>

#include <isc/formatcheck.h>
#include <isc/lang.h>
#include <isc/rwlock.h>

#include <dns/masterdump.h>
#include <dns/rdatastruct.h>
#include <dns/types.h>

typedef enum {
	dns_zone_none,
	dns_zone_master,
	dns_zone_slave,
	dns_zone_stub
} dns_zonetype_t;

#define DNS_ZONEOPT_SERVERS	  0x00000001U	/*%< perform server checks */
#define DNS_ZONEOPT_PARENTS	  0x00000002U	/*%< perform parent checks */
#define DNS_ZONEOPT_CHILDREN	  0x00000004U	/*%< perform child checks */
#define DNS_ZONEOPT_NOTIFY	  0x00000008U	/*%< perform NOTIFY */
#define DNS_ZONEOPT_MANYERRORS	  0x00000010U	/*%< return many errors on load */
#define DNS_ZONEOPT_IXFRFROMDIFFS 0x00000020U	/*%< calculate differences */
#define DNS_ZONEOPT_NOMERGE	  0x00000040U	/*%< don't merge journal */
#define DNS_ZONEOPT_CHECKNS	  0x00000080U	/*%< check if NS's are addresses */
#define DNS_ZONEOPT_FATALNS	  0x00000100U	/*%< DNS_ZONEOPT_CHECKNS is fatal */
#define DNS_ZONEOPT_MULTIMASTER	  0x00000200U	/*%< this zone has multiple masters */
#define DNS_ZONEOPT_USEALTXFRSRC  0x00000400U	/*%< use alternate transfer sources */
#define DNS_ZONEOPT_CHECKNAMES	  0x00000800U	/*%< check-names */
#define DNS_ZONEOPT_CHECKNAMESFAIL 0x00001000U	/*%< fatal check-name failures */
#define DNS_ZONEOPT_CHECKWILDCARD 0x00002000U	/*%< check for internal wildcards */
#define DNS_ZONEOPT_CHECKMX	  0x00004000U	/*%< check-mx */
#define DNS_ZONEOPT_CHECKMXFAIL   0x00008000U	/*%< fatal check-mx failures */
#define DNS_ZONEOPT_CHECKINTEGRITY 0x00010000U	/*%< perform integrity checks */
#define DNS_ZONEOPT_CHECKSIBLING  0x00020000U	/*%< perform sibling glue checks */
#define DNS_ZONEOPT_NOCHECKNS	  0x00040000U	/*%< disable IN NS address checks */
#define DNS_ZONEOPT_WARNMXCNAME	  0x00080000U	/*%< warn on MX CNAME check */
#define DNS_ZONEOPT_IGNOREMXCNAME 0x00100000U	/*%< ignore MX CNAME check */
#define DNS_ZONEOPT_WARNSRVCNAME  0x00200000U	/*%< warn on SRV CNAME check */
#define DNS_ZONEOPT_IGNORESRVCNAME 0x00400000U	/*%< ignore SRV CNAME check */
#define DNS_ZONEOPT_UPDATECHECKKSK 0x00800000U	/*%< check dnskey KSK flag */
#define DNS_ZONEOPT_TRYTCPREFRESH 0x01000000U	/*%< try tcp refresh on udp failure */
#define DNS_ZONEOPT_NOTIFYTOSOA	  0x02000000U	/*%< Notify the SOA MNAME */
#define DNS_ZONEOPT_NSEC3TESTZONE 0x04000000U	/*%< nsec3-test-zone */

#ifndef NOMINUM_PUBLIC
/*
 * Nominum specific options build down.
 */
#define DNS_ZONEOPT_NOTIFYFORWARD 0x80000000U	/* forward notify to master */
#endif /* NOMINUM_PUBLIC */

#ifndef DNS_ZONE_MINREFRESH
#define DNS_ZONE_MINREFRESH		    300	/*%< 5 minutes */
#endif
#ifndef DNS_ZONE_MAXREFRESH
#define DNS_ZONE_MAXREFRESH		2419200	/*%< 4 weeks */
#endif
#ifndef DNS_ZONE_DEFAULTREFRESH
#define DNS_ZONE_DEFAULTREFRESH		   3600	/*%< 1 hour */
#endif
#ifndef DNS_ZONE_MINRETRY
#define DNS_ZONE_MINRETRY		    300	/*%< 5 minutes */
#endif
#ifndef DNS_ZONE_MAXRETRY
#define DNS_ZONE_MAXRETRY		1209600	/*%< 2 weeks */
#endif
#ifndef DNS_ZONE_DEFAULTRETRY
#define DNS_ZONE_DEFAULTRETRY		     60	/*%< 1 minute, subject to
						   exponential backoff */
#endif

#define DNS_ZONESTATE_XFERRUNNING	1
#define DNS_ZONESTATE_XFERDEFERRED	2
#define DNS_ZONESTATE_SOAQUERY		3
#define DNS_ZONESTATE_ANY		4

ISC_LANG_BEGINDECLS

/***
 ***	Functions
 ***/

isc_result_t
dns_zone_create(dns_zone_t **zonep, isc_mem_t *mctx);
/*%<
 *	Creates a new empty zone and attach '*zonep' to it.
 *
 * Requires:
 *\li	'zonep' to point to a NULL pointer.
 *\li	'mctx' to be a valid memory context.
 *
 * Ensures:
 *\li	'*zonep' refers to a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_UNEXPECTED
 */

void
dns_zone_setclass(dns_zone_t *zone, dns_rdataclass_t rdclass);
/*%<
 *	Sets the class of a zone.  This operation can only be performed
 *	once on a zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	dns_zone_setclass() not to have been called since the zone was
 *	created.
 *\li	'rdclass' != dns_rdataclass_none.
 */

dns_rdataclass_t
dns_zone_getclass(dns_zone_t *zone);
/*%<
 *	Returns the current zone class.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_uint32_t
dns_zone_getserial(dns_zone_t *zone);
/*%<
 *	Returns the current serial number of the zone.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_settype(dns_zone_t *zone, dns_zonetype_t type);
/*%<
 *	Sets the zone type. This operation can only be performed once on
 *	a zone.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 *\li	dns_zone_settype() not to have been called since the zone was
 *	created.
 *\li	'type' != dns_zone_none
 */

void
dns_zone_setview(dns_zone_t *zone, dns_view_t *view);
/*%<
 *	Associate the zone with a view.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

dns_view_t *
dns_zone_getview(dns_zone_t *zone);
/*%<
 *	Returns the zone's associated view.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setorigin(dns_zone_t *zone, const dns_name_t *origin);
/*%<
 *	Sets the zones origin to 'origin'.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'origin' to be non NULL.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li 	#ISC_R_NOMEMORY
 */

dns_name_t *
dns_zone_getorigin(dns_zone_t *zone);
/*%<
 *	Returns the value of the origin.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setfile(dns_zone_t *zone, const char *file);

isc_result_t
dns_zone_setfile2(dns_zone_t *zone, const char *file,
		  dns_masterformat_t format);
/*%<
 *    Sets the name of the master file in the format of 'format' from which
 *    the zone loads its database to 'file'.
 *
 *    For zones that have no associated master file, 'file' will be NULL.
 *
 *	For zones with persistent databases, the file name
 *	setting is ignored.
 *
 *    dns_zone_setfile() is a backward-compatible form of
 *    dns_zone_setfile2(), which always specifies the
 *    dns_masterformat_text (RFC1035) format.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SUCCESS
 */

const char *
dns_zone_getfile(dns_zone_t *zone);
/*%<
 * 	Gets the name of the zone's master file, if any.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *\li	Pointer to null-terminated file name, or NULL.
 */

isc_result_t
dns_zone_load(dns_zone_t *zone);

isc_result_t
dns_zone_loadnew(dns_zone_t *zone);
/*%<
 *	Cause the database to be loaded from its backing store.
 *	Confirm that the minimum requirements for the zone type are
 *	met, otherwise DNS_R_BADZONE is returned.
 *
 *	dns_zone_loadnew() only loads zones that are not yet loaded.
 *	dns_zone_load() also loads zones that are already loaded and
 *	and whose master file has changed since the last load.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_UNEXPECTED
 *\li	#ISC_R_SUCCESS
 *\li	DNS_R_CONTINUE	  Incremental load has been queued.
 *\li	DNS_R_UPTODATE	  The zone has already been loaded based on
 *			  file system timestamps.
 *\li	DNS_R_BADZONE
 *\li	Any result value from dns_db_load().
 */

void
dns_zone_attach(dns_zone_t *source, dns_zone_t **target);
/*%<
 *	Attach '*target' to 'source' incrementing its external
 * 	reference count.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_zone_detach(dns_zone_t **zonep);
/*%<
 *	Detach from a zone decrementing its external reference count.
 *	If this was the last external reference to the zone it will be
 * 	shut down and eventually freed.
 *
 * Require:
 *\li	'zonep' to point to a valid zone.
 */

void
dns_zone_iattach(dns_zone_t *source, dns_zone_t **target);
/*%<
 *	Attach '*target' to 'source' incrementing its internal
 * 	reference count.  This is intended for use by operations
 * 	such as zone transfers that need to prevent the zone
 * 	object from being freed but not from shutting down.
 *
 * Require:
 *\li	The caller is running in the context of the zone's task.
 *\li	'zone' to be a valid zone.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_zone_idetach(dns_zone_t **zonep);
/*%<
 *	Detach from a zone decrementing its internal reference count.
 *	If there are no more internal or external references to the
 * 	zone, it will be freed.
 *
 * Require:
 *\li	The caller is running in the context of the zone's task.
 *\li	'zonep' to point to a valid zone.
 */

void
dns_zone_setflag(dns_zone_t *zone, unsigned int flags, isc_boolean_t value);
/*%<
 *	Sets ('value' == 'ISC_TRUE') / clears ('value' == 'IS_FALSE')
 *	zone flags.  Valid flag bits are DNS_ZONE_F_*.
 *
 * Requires
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_getdb(dns_zone_t *zone, dns_db_t **dbp);
/*%<
 * 	Attach '*dbp' to the database to if it exists otherwise
 *	return DNS_R_NOTLOADED.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'dbp' to be != NULL && '*dbp' == NULL.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	DNS_R_NOTLOADED
 */

isc_result_t
dns_zone_setdbtype(dns_zone_t *zone,
		   unsigned int dbargc, const char * const *dbargv);
/*%<
 *	Sets the database type to dbargv[0] and database arguments
 *	to subsequent dbargv elements.
 *	'db_type' is not checked to see if it is a valid database type.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'database' to be non NULL.
 *\li	'dbargc' to be >= 1
 *\li	'dbargv' to point to dbargc NULL-terminated strings
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SUCCESS
 */

isc_result_t
dns_zone_getdbtype(dns_zone_t *zone, char ***argv, isc_mem_t *mctx);
/*%<
 *	Returns the current dbtype.  isc_mem_free() should be used
 * 	to free 'argv' after use.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'argv' to be non NULL and *argv to be NULL.
 *\li	'mctx' to be valid.
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SUCCESS
 */

void
dns_zone_markdirty(dns_zone_t *zone);
/*%<
 *	Mark a zone as 'dirty'.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_expire(dns_zone_t *zone);
/*%<
 *	Mark the zone as expired.  If the zone requires dumping cause it to
 *	be initiated.  Set the refresh and retry intervals to there default
 *	values and unload the zone.
 *
 * Require
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_refresh(dns_zone_t *zone);
/*%<
 *	Initiate zone up to date checks.  The zone must already be being
 *	managed.
 *
 * Require
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_flush(dns_zone_t *zone);
/*%<
 *	Write the zone to database if there are uncommitted changes.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_dump(dns_zone_t *zone);
/*%<
 *	Write the zone to database.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_dumptostream(dns_zone_t *zone, FILE *fd);

isc_result_t
dns_zone_dumptostream2(dns_zone_t *zone, FILE *fd, dns_masterformat_t format,
		       const dns_master_style_t *style);
/*%<
 *    Write the zone to stream 'fd' in the specified 'format'.
 *    If the 'format' is dns_masterformat_text (RFC1035), 'style' also
 *    specifies the file style (e.g., &dns_master_style_default).
 *
 *    dns_zone_dumptostream() is a backward-compatible form of
 *    dns_zone_dumptostream2(), which always uses the dns_masterformat_text
 *    format and the dns_master_style_default style.
 *
 *    Note that dns_zone_dumptostream2() is the most flexible form.  It
 *    can also provide the functionality of dns_zone_fulldumptostream().
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'fd' to be a stream open for writing.
 */

isc_result_t
dns_zone_fulldumptostream(dns_zone_t *zone, FILE *fd);
/*%<
 *	The same as dns_zone_dumptostream, but dumps the zone with
 *	different dump settings (dns_master_style_full).
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'fd' to be a stream open for writing.
 */

void
dns_zone_maintenance(dns_zone_t *zone);
/*%<
 *	Perform regular maintenance on the zone.  This is called as a
 *	result of a zone being managed.
 *
 * Require
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setmasters(dns_zone_t *zone, const isc_sockaddr_t *masters,
		    isc_uint32_t count);
isc_result_t
dns_zone_setmasterswithkeys(dns_zone_t *zone,
			    const isc_sockaddr_t *masters,
			    dns_name_t **keynames,
			    isc_uint32_t count);
/*%<
 *	Set the list of master servers for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'masters' array of isc_sockaddr_t with port set or NULL.
 *\li	'count' the number of masters.
 *\li      'keynames' array of dns_name_t's for tsig keys or NULL.
 *
 *  \li    dns_zone_setmasters() is just a wrapper to setmasterswithkeys(),
 *      passing NULL in the keynames field.
 *
 * \li	If 'masters' is NULL then 'count' must be zero.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 *\li      Any result dns_name_dup() can return, if keynames!=NULL
 */

isc_result_t
dns_zone_setalsonotify(dns_zone_t *zone, const isc_sockaddr_t *notify,
		       isc_uint32_t count);
/*%<
 *	Set the list of additional servers to be notified when
 *	a zone changes.	 To clear the list use 'count = 0'.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'notify' to be non-NULL if count != 0.
 *\li	'count' to be the number of notifiees.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

void
dns_zone_unload(dns_zone_t *zone);
/*%<
 *	detach the database from the zone structure.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setoption(dns_zone_t *zone, unsigned int option, isc_boolean_t value);
/*%<
 *	Set given options on ('value' == ISC_TRUE) or off ('value' ==
 *	#ISC_FALSE).
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

unsigned int
dns_zone_getoptions(dns_zone_t *zone);
/*%<
 *	Returns the current zone options.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setminrefreshtime(dns_zone_t *zone, isc_uint32_t val);
/*%<
 *	Set the minimum refresh time.
 *
 * Requires:
 *\li	'zone' is valid.
 *\li	val > 0.
 */

void
dns_zone_setmaxrefreshtime(dns_zone_t *zone, isc_uint32_t val);
/*%<
 *	Set the maximum refresh time.
 *
 * Requires:
 *\li	'zone' is valid.
 *\li	val > 0.
 */

void
dns_zone_setminretrytime(dns_zone_t *zone, isc_uint32_t val);
/*%<
 *	Set the minimum retry time.
 *
 * Requires:
 *\li	'zone' is valid.
 *\li	val > 0.
 */

void
dns_zone_setmaxretrytime(dns_zone_t *zone, isc_uint32_t val);
/*%<
 *	Set the maximum retry time.
 *
 * Requires:
 *\li	'zone' is valid.
 *	val > 0.
 */

isc_result_t
dns_zone_setxfrsource4(dns_zone_t *zone, const isc_sockaddr_t *xfrsource);
isc_result_t
dns_zone_setaltxfrsource4(dns_zone_t *zone,
			  const isc_sockaddr_t *xfrsource);
/*%<
 * 	Set the source address to be used in IPv4 zone transfers.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'xfrsource' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getxfrsource4(dns_zone_t *zone);
isc_sockaddr_t *
dns_zone_getaltxfrsource4(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setxfrsource4
 *	call, or the default of inaddr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setxfrsource6(dns_zone_t *zone, const isc_sockaddr_t *xfrsource);
isc_result_t
dns_zone_setaltxfrsource6(dns_zone_t *zone,
			  const isc_sockaddr_t *xfrsource);
/*%<
 * 	Set the source address to be used in IPv6 zone transfers.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'xfrsource' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getxfrsource6(dns_zone_t *zone);
isc_sockaddr_t *
dns_zone_getaltxfrsource6(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setxfrsource6
 *	call, or the default of in6addr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setnotifysrc4(dns_zone_t *zone, const isc_sockaddr_t *notifysrc);
/*%<
 * 	Set the source address to be used with IPv4 NOTIFY messages.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'notifysrc' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getnotifysrc4(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setnotifysrc4
 *	call, or the default of inaddr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setnotifysrc6(dns_zone_t *zone, const isc_sockaddr_t *notifysrc);
/*%<
 * 	Set the source address to be used with IPv6 NOTIFY messages.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'notifysrc' to contain the address.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 */

isc_sockaddr_t *
dns_zone_getnotifysrc6(dns_zone_t *zone);
/*%<
 *	Returns the source address set by a previous dns_zone_setnotifysrc6
 *	call, or the default of in6addr_any, port 0.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setnotifyacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the notify acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be a valid acl.
 */

void
dns_zone_setqueryacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the query acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be a valid acl.
 */

void
dns_zone_setqueryonacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the query-on acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be a valid acl.
 */

void
dns_zone_setupdateacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the update acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be valid acl.
 */

void
dns_zone_setforwardacl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the forward unsigned updates acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be valid acl.
 */

void
dns_zone_setxfracl(dns_zone_t *zone, dns_acl_t *acl);
/*%<
 *	Sets the transfer acl list for the zone.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'acl' to be valid acl.
 */

dns_acl_t *
dns_zone_getnotifyacl(dns_zone_t *zone);
/*%<
 * 	Returns the current notify acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getqueryacl(dns_zone_t *zone);
/*%<
 * 	Returns the current query acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getqueryonacl(dns_zone_t *zone);
/*%<
 * 	Returns the current query-on acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getupdateacl(dns_zone_t *zone);
/*%<
 * 	Returns the current update acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getforwardacl(dns_zone_t *zone);
/*%<
 * 	Returns the current forward unsigned updates acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

dns_acl_t *
dns_zone_getxfracl(dns_zone_t *zone);
/*%<
 * 	Returns the current transfer acl or NULL.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	acl a pointer to the acl.
 *\li	NULL
 */

void
dns_zone_clearupdateacl(dns_zone_t *zone);
/*%<
 *	Clear the current update acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearforwardacl(dns_zone_t *zone);
/*%<
 *	Clear the current forward unsigned updates acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearnotifyacl(dns_zone_t *zone);
/*%<
 *	Clear the current notify acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearqueryacl(dns_zone_t *zone);
/*%<
 *	Clear the current query acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearqueryonacl(dns_zone_t *zone);
/*%<
 *	Clear the current query-on acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_clearxfracl(dns_zone_t *zone);
/*%<
 *	Clear the current transfer acl.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

isc_boolean_t
dns_zone_getupdatedisabled(dns_zone_t *zone);
/*%<
 * Return update disabled.
 * Transient unless called when running in isc_task_exclusive() mode.
 */

void
dns_zone_setupdatedisabled(dns_zone_t *zone, isc_boolean_t state);
/*%<
 * Set update disabled.
 * Should only be called only when running in isc_task_exclusive() mode.
 * Failure to do so may result in updates being committed after the
 * call has been made.
 */

isc_boolean_t
dns_zone_getzeronosoattl(dns_zone_t *zone);
/*%<
 * Return zero-no-soa-ttl status.
 */

void
dns_zone_setzeronosoattl(dns_zone_t *zone, isc_boolean_t state);
/*%<
 * Set zero-no-soa-ttl status.
 */

void
dns_zone_setchecknames(dns_zone_t *zone, dns_severity_t severity);
/*%<
 * 	Set the severity of name checking when loading a zone.
 *
 * Require:
 * \li     'zone' to be a valid zone.
 */

dns_severity_t
dns_zone_getchecknames(dns_zone_t *zone);
/*%<
 *	Return the current severity of name checking.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 */

void
dns_zone_setjournalsize(dns_zone_t *zone, isc_int32_t size);
/*%<
 *	Sets the journal size for the zone.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_int32_t
dns_zone_getjournalsize(dns_zone_t *zone);
/*%<
 *	Return the journal size as set with a previous call to
 *	dns_zone_setjournalsize().
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_notifyreceive(dns_zone_t *zone, isc_sockaddr_t *from,
		       dns_message_t *msg);
/*%<
 *	Tell the zone that it has received a NOTIFY message from another
 *	server.  This may cause some zone maintenance activity to occur.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 *\li	'*from' to contain the address of the server from which 'msg'
 *		was received.
 *\li	'msg' a message with opcode NOTIFY and qr clear.
 *
 * Returns:
 *\li	DNS_R_REFUSED
 *\li	DNS_R_NOTIMP
 *\li	DNS_R_FORMERR
 *\li	DNS_R_SUCCESS
 */

void
dns_zone_setmaxxfrin(dns_zone_t *zone, isc_uint32_t maxxfrin);
/*%<
 * Set the maximum time (in seconds) that a zone transfer in (AXFR/IXFR)
 * of this zone will use before being aborted.
 *
 * Requires:
 * \li	'zone' to be valid initialised zone.
 */

isc_uint32_t
dns_zone_getmaxxfrin(dns_zone_t *zone);
/*%<
 * Returns the maximum transfer time for this zone.  This will be
 * either the value set by the last call to dns_zone_setmaxxfrin() or
 * the default value of 1 hour.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

void
dns_zone_setmaxxfrout(dns_zone_t *zone, isc_uint32_t maxxfrout);
/*%<
 * Set the maximum time (in seconds) that a zone transfer out (AXFR/IXFR)
 * of this zone will use before being aborted.
 *
 * Requires:
 * \li	'zone' to be valid initialised zone.
 */

isc_uint32_t
dns_zone_getmaxxfrout(dns_zone_t *zone);
/*%<
 * Returns the maximum transfer time for this zone.  This will be
 * either the value set by the last call to dns_zone_setmaxxfrout() or
 * the default value of 1 hour.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

isc_result_t
dns_zone_setjournal(dns_zone_t *zone, const char *journal);
/*%<
 * Sets the filename used for journaling updates / IXFR transfers.
 * The default journal name is set by dns_zone_setfile() to be
 * "file.jnl".  If 'journal' is NULL, the zone will have no
 * journal name.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMEMORY
 */

char *
dns_zone_getjournal(dns_zone_t *zone);
/*%<
 * Returns the journal name associated with this zone.
 * If no journal has been set this will be NULL.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

dns_zonetype_t
dns_zone_gettype(dns_zone_t *zone);
/*%<
 * Returns the type of the zone (master/slave/etc.)
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 */

void
dns_zone_settask(dns_zone_t *zone, isc_task_t *task);
/*%<
 * Give a zone a task to work with.  Any current task will be detached.
 *
 * Requires:
 *\li	'zone' to be valid.
 *\li	'task' to be valid.
 */

void
dns_zone_gettask(dns_zone_t *zone, isc_task_t **target);
/*%<
 * Attach '*target' to the zone's task.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *\li	'zone' to have a task.
 *\li	'target' to be != NULL && '*target' == NULL.
 */

void
dns_zone_notify(dns_zone_t *zone);
/*%<
 * Generate notify events for this zone.
 *
 * Requires:
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zone_replacedb(dns_zone_t *zone, dns_db_t *db, isc_boolean_t dump);
/*%<
 * Replace the database of "zone" with a new database "db".
 *
 * If "dump" is ISC_TRUE, then the new zone contents are dumped
 * into to the zone's master file for persistence.  When replacing
 * a zone database by one just loaded from a master file, set
 * "dump" to ISC_FALSE to avoid a redundant redump of the data just
 * loaded.  Otherwise, it should be set to ISC_TRUE.
 *
 * If the "diff-on-reload" option is enabled in the configuration file,
 * the differences between the old and the new database are added to the
 * journal file, and the master file dump is postponed.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	DNS_R_SUCCESS
 * \li	DNS_R_BADZONE	zone failed basic consistency checks:
 *			* a single SOA must exist
 *			* some NS records must exist.
 *	Others
 */

isc_uint32_t
dns_zone_getidlein(dns_zone_t *zone);
/*%<
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	number of seconds of idle time before we abort the transfer in.
 */

void
dns_zone_setidlein(dns_zone_t *zone, isc_uint32_t idlein);
/*%<
 * \li	Set the idle timeout for transfer the.
 * \li	Zero set the default value, 1 hour.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

isc_uint32_t
dns_zone_getidleout(dns_zone_t *zone);
/*%<
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	number of seconds of idle time before we abort a transfer out.
 */

void
dns_zone_setidleout(dns_zone_t *zone, isc_uint32_t idleout);
/*%<
 * \li	Set the idle timeout for transfers out.
 * \li	Zero set the default value, 1 hour.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_getssutable(dns_zone_t *zone, dns_ssutable_t **table);
/*%<
 * Get the simple-secure-update policy table.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setssutable(dns_zone_t *zone, dns_ssutable_t *table);
/*%<
 * Set / clear the simple-secure-update policy table.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

isc_mem_t *
dns_zone_getmctx(dns_zone_t *zone);
/*%<
 * Get the memory context of a zone.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

dns_zonemgr_t *
dns_zone_getmgr(dns_zone_t *zone);
/*%<
 *	If 'zone' is managed return the zone manager otherwise NULL.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setsigvalidityinterval(dns_zone_t *zone, isc_uint32_t interval);
/*%<
 * Set the zone's RRSIG validity interval.  This is the length of time
 * for which DNSSEC signatures created as a result of dynamic updates
 * to secure zones will remain valid, in seconds.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

isc_uint32_t
dns_zone_getsigvalidityinterval(dns_zone_t *zone);
/*%<
 * Get the zone's RRSIG validity interval.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setsigresigninginterval(dns_zone_t *zone, isc_uint32_t interval);
/*%<
 * Set the zone's RRSIG re-signing interval.  A dynamic zone's RRSIG's
 * will be re-signed 'interval' amount of time before they expire.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

isc_uint32_t
dns_zone_getsigresigninginterval(dns_zone_t *zone);
/*%<
 * Get the zone's RRSIG re-signing interval.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 */

void
dns_zone_setnotifytype(dns_zone_t *zone, dns_notifytype_t notifytype);
/*%<
 * Sets zone notify method to "notifytype"
 */

isc_result_t
dns_zone_forwardupdate(dns_zone_t *zone, dns_message_t *msg,
		       dns_updatecallback_t callback, void *callback_arg);
/*%<
 * Forward 'msg' to each master in turn until we get an answer or we
 * have exhausted the list of masters. 'callback' will be called with
 * ISC_R_SUCCESS if we get an answer and the returned message will be
 * passed as 'answer_message', otherwise a non ISC_R_SUCCESS result code
 * will be passed and answer_message will be NULL.  The callback function
 * is responsible for destroying 'answer_message'.
 *		(callback)(callback_arg, result, answer_message);
 *
 * Require:
 *\li	'zone' to be valid
 *\li	'msg' to be valid.
 *\li	'callback' to be non NULL.
 * Returns:
 *\li	#ISC_R_SUCCESS if the message has been forwarded,
 *\li	#ISC_R_NOMEMORY
 *\li	Others
 */

isc_result_t
dns_zone_next(dns_zone_t *zone, dns_zone_t **next);
/*%<
 * Find the next zone in the list of managed zones.
 *
 * Requires:
 *\li	'zone' to be valid
 *\li	The zone manager for the indicated zone MUST be locked
 *	by the caller.  This is not checked.
 *\li	'next' be non-NULL, and '*next' be NULL.
 *
 * Ensures:
 *\li	'next' points to a valid zone (result ISC_R_SUCCESS) or to NULL
 *	(result ISC_R_NOMORE).
 */



isc_result_t
dns_zone_first(dns_zonemgr_t *zmgr, dns_zone_t **first);
/*%<
 * Find the first zone in the list of managed zones.
 *
 * Requires:
 *\li	'zonemgr' to be valid
 *\li	The zone manager for the indicated zone MUST be locked
 *	by the caller.  This is not checked.
 *\li	'first' be non-NULL, and '*first' be NULL
 *
 * Ensures:
 *\li	'first' points to a valid zone (result ISC_R_SUCCESS) or to NULL
 *	(result ISC_R_NOMORE).
 */

isc_result_t
dns_zone_setkeydirectory(dns_zone_t *zone, const char *directory);
/*%<
 *	Sets the name of the directory where private keys used for
 *	online signing of dynamic zones are found.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *
 * Returns:
 *\li	#ISC_R_NOMEMORY
 *\li	#ISC_R_SUCCESS
 */

const char *
dns_zone_getkeydirectory(dns_zone_t *zone);
/*%<
 * 	Gets the name of the directory where private keys used for
 *	online signing of dynamic zones are found.
 *
 * Requires:
 *\li	'zone' to be valid initialised zone.
 *
 * Returns:
 *	Pointer to null-terminated file name, or NULL.
 */


isc_result_t
dns_zonemgr_create(isc_mem_t *mctx, isc_taskmgr_t *taskmgr,
		   isc_timermgr_t *timermgr, isc_socketmgr_t *socketmgr,
		   dns_zonemgr_t **zmgrp);
/*%<
 * Create a zone manager.
 *
 * Requires:
 *\li	'mctx' to be a valid memory context.
 *\li	'taskmgr' to be a valid task manager.
 *\li	'timermgr' to be a valid timer manager.
 *\li	'zmgrp'	to point to a NULL pointer.
 */

isc_result_t
dns_zonemgr_managezone(dns_zonemgr_t *zmgr, dns_zone_t *zone);
/*%<
 *	Bring the zone under control of a zone manager.
 *
 * Require:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'zone' to be a valid zone.
 */

isc_result_t
dns_zonemgr_forcemaint(dns_zonemgr_t *zmgr);
/*%<
 * Force zone maintenance of all zones managed by 'zmgr' at its
 * earliest convenience.
 */

void
dns_zonemgr_resumexfrs(dns_zonemgr_t *zmgr);
/*%<
 * Attempt to start any stalled zone transfers.
 */

void
dns_zonemgr_shutdown(dns_zonemgr_t *zmgr);
/*%<
 *	Shut down the zone manager.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_attach(dns_zonemgr_t *source, dns_zonemgr_t **target);
/*%<
 *	Attach '*target' to 'source' incrementing its external
 * 	reference count.
 *
 * Require:
 *\li	'zone' to be a valid zone.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_zonemgr_detach(dns_zonemgr_t **zmgrp);
/*%<
 *	 Detach from a zone manager.
 *
 * Requires:
 *\li	'*zmgrp' is a valid, non-NULL zone manager pointer.
 *
 * Ensures:
 *\li	'*zmgrp' is NULL.
 */

void
dns_zonemgr_releasezone(dns_zonemgr_t *zmgr, dns_zone_t *zone);
/*%<
 *	Release 'zone' from the managed by 'zmgr'.  'zmgr' is implicitly
 *	detached from 'zone'.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'zone' to be a valid zone.
 *\li	'zmgr' == 'zone->zmgr'
 *
 * Ensures:
 *\li	'zone->zmgr' == NULL;
 */

void
dns_zonemgr_settransfersin(dns_zonemgr_t *zmgr, isc_uint32_t value);
/*%<
 *	Set the maximum number of simultaneous transfers in allowed by
 *	the zone manager.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

isc_uint32_t
dns_zonemgr_getttransfersin(dns_zonemgr_t *zmgr);
/*%<
 *	Return the maximum number of simultaneous transfers in allowed.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_settransfersperns(dns_zonemgr_t *zmgr, isc_uint32_t value);
/*%<
 *	Set the number of zone transfers allowed per nameserver.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager
 */

isc_uint32_t
dns_zonemgr_getttransfersperns(dns_zonemgr_t *zmgr);
/*%<
 *	Return the number of transfers allowed per nameserver.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_setiolimit(dns_zonemgr_t *zmgr, isc_uint32_t iolimit);
/*%<
 *	Set the number of simultaneous file descriptors available for
 *	reading and writing masterfiles.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'iolimit' to be positive.
 */

isc_uint32_t
dns_zonemgr_getiolimit(dns_zonemgr_t *zmgr);
/*%<
 *	Get the number of simultaneous file descriptors available for
 *	reading and writing masterfiles.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

void
dns_zonemgr_setserialqueryrate(dns_zonemgr_t *zmgr, unsigned int value);
/*%<
 *	Set the number of SOA queries sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager
 */

unsigned int
dns_zonemgr_getserialqueryrate(dns_zonemgr_t *zmgr);
/*%<
 *	Return the number of SOA queries sent per second.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 */

unsigned int
dns_zonemgr_getcount(dns_zonemgr_t *zmgr, int state);
/*%<
 *	Returns the number of zones in the specified state.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'state' to be a valid DNS_ZONESTATE_ constant.
 */

void
dns_zonemgr_unreachableadd(dns_zonemgr_t *zmgr, isc_sockaddr_t *remote,
			   isc_sockaddr_t *local, isc_time_t *now);
/*%<
 *	Add the pair of addresses to the unreachable cache.
 *
 * Requires:
 *\li	'zmgr' to be a valid zone manager.
 *\li	'remote' to be a valid sockaddr.
 *\li	'local' to be a valid sockaddr.
 */

void
dns_zone_forcereload(dns_zone_t *zone);
/*%<
 *      Force a reload of specified zone.
 *
 * Requires:
 *\li      'zone' to be a valid zone.
 */

isc_boolean_t
dns_zone_isforced(dns_zone_t *zone);
/*%<
 *      Check if the zone is waiting a forced reload.
 *
 * Requires:
 * \li     'zone' to be a valid zone.
 */

isc_result_t
dns_zone_setstatistics(dns_zone_t *zone, isc_boolean_t on);
/*%<
 * This function is obsoleted by dns_zone_setrequeststats().
 */

isc_uint64_t *
dns_zone_getstatscounters(dns_zone_t *zone);
/*%<
 * This function is obsoleted by dns_zone_getrequeststats().
 */

void
dns_zone_setstats(dns_zone_t *zone, isc_stats_t *stats);
/*%<
 * Set a general zone-maintenance statistics set 'stats' for 'zone'.  This
 * function is expected to be called only on zone creation (when necessary).
 * Once installed, it cannot be removed or replaced.  Also, there is no
 * interface to get the installed stats from the zone; the caller must keep the
 * stats to reference (e.g. dump) it later.
 *
 * Requires:
 * \li	'zone' to be a valid zone and does not have a statistics set already
 *	installed.
 *
 *\li	stats is a valid statistics supporting zone statistics counters
 *	(see dns/stats.h).
 */

void
dns_zone_setrequeststats(dns_zone_t *zone, isc_stats_t *stats);
/*%<
 * Set an additional statistics set to zone.  It is attached in the zone
 * but is not counted in the zone module; only the caller updates the counters.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 *\li	stats is a valid statistics.
 */

isc_stats_t *
dns_zone_getrequeststats(dns_zone_t *zone);
/*%<
 * Get the additional statistics for zone, if one is installed.
 *
 * Requires:
 * \li	'zone' to be a valid zone.
 *
 * Returns:
 * \li	when available, a pointer to the statistics set installed in zone;
 *	otherwise NULL.
 */

void
dns_zone_dialup(dns_zone_t *zone);
/*%<
 * Perform dialup-time maintenance on 'zone'.
 */

void
dns_zone_setdialup(dns_zone_t *zone, dns_dialuptype_t dialup);
/*%<
 * Set the dialup type of 'zone' to 'dialup'.
 *
 * Requires:
 * \li	'zone' to be valid initialised zone.
 *\li	'dialup' to be a valid dialup type.
 */

void
dns_zone_log(dns_zone_t *zone, int level, const char *msg, ...)
	ISC_FORMAT_PRINTF(3, 4);
/*%<
 * Log the message 'msg...' at 'level', including text that identifies
 * the message as applying to 'zone'.
 */

void
dns_zone_logc(dns_zone_t *zone, isc_logcategory_t *category, int level,
	      const char *msg, ...) ISC_FORMAT_PRINTF(4, 5);
/*%<
 * Log the message 'msg...' at 'level', including text that identifies
 * the message as applying to 'zone'.
 */

void
dns_zone_name(dns_zone_t *zone, char *buf, size_t len);
/*%<
 * Return the name of the zone with class and view.
 *
 * Requires:
 *\li	'zone' to be valid.
 *\li	'buf' to be non NULL.
 */

isc_result_t
dns_zone_checknames(dns_zone_t *zone, dns_name_t *name, dns_rdata_t *rdata);
/*%<
 * Check if this record meets the check-names policy.
 *
 * Requires:
 *	'zone' to be valid.
 *	'name' to be valid.
 *	'rdata' to be valid.
 *
 * Returns:
 *	DNS_R_SUCCESS		passed checks.
 *	DNS_R_BADOWNERNAME	failed ownername checks.
 *	DNS_R_BADNAME		failed rdata checks.
 */

void
dns_zone_setacache(dns_zone_t *zone, dns_acache_t *acache);
/*%<
 *	Associate the zone with an additional cache.
 *
 * Require:
 *	'zone' to be a valid zone.
 *	'acache' to be a non NULL pointer.
 *
 * Ensures:
 *	'zone' will have a reference to 'acache'
 */

void
dns_zone_setcheckmx(dns_zone_t *zone, dns_checkmxfunc_t checkmx);
/*%<
 *	Set the post load integrity callback function 'checkmx'.
 *	'checkmx' will be called if the MX is not within the zone.
 *
 * Require:
 *	'zone' to be a valid zone.
 */

void
dns_zone_setchecksrv(dns_zone_t *zone, dns_checkmxfunc_t checksrv);
/*%<
 *	Set the post load integrity callback function 'checksrv'.
 *	'checksrv' will be called if the SRV TARGET is not within the zone.
 *
 * Require:
 *	'zone' to be a valid zone.
 */

void
dns_zone_setcheckns(dns_zone_t *zone, dns_checknsfunc_t checkns);
/*%<
 *	Set the post load integrity callback function 'checkmx'.
 *	'checkmx' will be called if the MX is not within the zone.
 *
 * Require:
 *	'zone' to be a valid zone.
 */

void
dns_zone_setnotifydelay(dns_zone_t *zone, isc_uint32_t delay);
/*%<
 * Set the minimum delay between sets of notify messages.
 *
 * Requires:
 *	'zone' to be valid.
 */

isc_uint32_t
dns_zone_getnotifydelay(dns_zone_t *zone);
/*%<
 * Get the minimum delay between sets of notify messages.
 *
 * Requires:
 *	'zone' to be valid.
 */

void
dns_zone_setisself(dns_zone_t *zone, dns_isselffunc_t isself, void *arg);
/*%<
 * Set the isself callback function and argument.
 *
 * isc_boolean_t
 * isself(dns_view_t *myview, dns_tsigkey_t *mykey, isc_netaddr_t *srcaddr,
 *	  isc_netaddr_t *destaddr, dns_rdataclass_t rdclass, void *arg);
 *
 * 'isself' returns ISC_TRUE if a non-recursive query from 'srcaddr' to
 * 'destaddr' with optional key 'mykey' for class 'rdclass' would be
 * delivered to 'myview'.
 */

void
dns_zone_setnodes(dns_zone_t *zone, isc_uint32_t nodes);
/*%<
 * Set the number of nodes that will be checked per quantum.
 */

void
dns_zone_setsignatures(dns_zone_t *zone, isc_uint32_t signatures);
/*%<
 * Set the number of signatures that will be generated per quantum.
 */

isc_result_t
dns_zone_signwithkey(dns_zone_t *zone, dns_secalg_t algorithm,
		     isc_uint16_t keyid, isc_boolean_t delete);
/*%<
 * Initiate/resume signing of the entire zone with the zone DNSKEY(s)
 * that match the given algorithm and keyid.
 */

isc_result_t
dns_zone_addnsec3chain(dns_zone_t *zone, dns_rdata_nsec3param_t *nsec3param);
/*%<
 * Incrementally add a NSEC3 chain that corresponds to 'nsec3param'.
 */

void
dns_zone_setprivatetype(dns_zone_t *zone, dns_rdatatype_t type);
dns_rdatatype_t
dns_zone_getprivatetype(dns_zone_t *zone);
/*
 * Get/Set the private record type.  It is expected that these interfaces
 * will not be permanent.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_ZONE_H */
