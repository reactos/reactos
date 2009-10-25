/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: sdb.h,v 1.21.332.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_SDB_H
#define DNS_SDB_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/sdb.h
 * \brief
 * Simple database API.
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>

#include <dns/types.h>

/***
 *** Types
 ***/

/*%
 * A simple database.  This is an opaque type.
 */
typedef struct dns_sdb dns_sdb_t;

/*%
 * A simple database lookup in progress.  This is an opaque type.
 */
typedef struct dns_sdblookup dns_sdblookup_t;

/*%
 * A simple database traversal in progress.  This is an opaque type.
 */
typedef struct dns_sdballnodes dns_sdballnodes_t;

typedef isc_result_t
(*dns_sdblookupfunc_t)(const char *zone, const char *name, void *dbdata,
		       dns_sdblookup_t *);

typedef isc_result_t
(*dns_sdbauthorityfunc_t)(const char *zone, void *dbdata, dns_sdblookup_t *);

typedef isc_result_t
(*dns_sdballnodesfunc_t)(const char *zone, void *dbdata,
			 dns_sdballnodes_t *allnodes);

typedef isc_result_t
(*dns_sdbcreatefunc_t)(const char *zone, int argc, char **argv,
		       void *driverdata, void **dbdata);

typedef void
(*dns_sdbdestroyfunc_t)(const char *zone, void *driverdata, void **dbdata);


typedef struct dns_sdbmethods {
	dns_sdblookupfunc_t	lookup;
	dns_sdbauthorityfunc_t	authority;
	dns_sdballnodesfunc_t	allnodes;
	dns_sdbcreatefunc_t	create;
	dns_sdbdestroyfunc_t	destroy;
} dns_sdbmethods_t;

/***
 *** Functions
 ***/

ISC_LANG_BEGINDECLS

#define DNS_SDBFLAG_RELATIVEOWNER 0x00000001U
#define DNS_SDBFLAG_RELATIVERDATA 0x00000002U
#define DNS_SDBFLAG_THREADSAFE 0x00000004U

isc_result_t
dns_sdb_register(const char *drivername, const dns_sdbmethods_t *methods,
		 void *driverdata, unsigned int flags, isc_mem_t *mctx,
		 dns_sdbimplementation_t **sdbimp);
/*%<
 * Register a simple database driver for the database type 'drivername',
 * implemented by the functions in '*methods'.
 *
 * sdbimp must point to a NULL dns_sdbimplementation_t pointer.  That is,
 * sdbimp != NULL && *sdbimp == NULL.  It will be assigned a value that
 * will later be used to identify the driver when deregistering it.
 *
 * The name server will perform lookups in the database by calling the
 * function 'lookup', passing it a printable zone name 'zone', a printable
 * domain name 'name', and a copy of the argument 'dbdata' that
 * was potentially returned by the create function.  The 'dns_sdblookup_t'
 * argument to 'lookup' and 'authority' is an opaque pointer to be passed to
 * ns_sdb_putrr().
 *
 * The lookup function returns the lookup results to the name server
 * by calling ns_sdb_putrr() once for each record found.  On success,
 * the return value of the lookup function should be ISC_R_SUCCESS.
 * If the domain name 'name' does not exist, the lookup function should
 * ISC_R_NOTFOUND.  Any other return value is treated as an error.
 *
 * Lookups at the zone apex will cause the server to also call the
 * function 'authority' (if non-NULL), which must provide an SOA record
 * and NS records for the zone by calling ns_sdb_putrr() once for each of
 * these records.  The 'authority' function may be NULL if invoking
 * the 'lookup' function on the zone apex will return SOA and NS records.
 *
 * The allnodes function, if non-NULL, fills in an opaque structure to be
 * used by a database iterator.  This allows the zone to be transferred.
 * This may use a considerable amount of memory for large zones, and the
 * zone transfer may not be fully RFC1035 compliant if the zone is
 * frequently changed.
 *
 * The create function will be called for each zone configured
 * into the name server using this database type.  It can be used
 * to create a "database object" containing zone specific data,
 * which can make use of the database arguments specified in the
 * name server configuration.
 *
 * The destroy function will be called to free the database object
 * when its zone is destroyed.
 *
 * The create and destroy functions may be NULL.
 *
 * If flags includes DNS_SDBFLAG_RELATIVEOWNER, the lookup and authority
 * functions will be called with relative names rather than absolute names.
 * The string "@" represents the zone apex in this case.
 *
 * If flags includes DNS_SDBFLAG_RELATIVERDATA, the rdata strings may
 * include relative names.  Otherwise, all names in the rdata string must
 * be absolute.  Be aware that if relative names are allowed, any
 * absolute names must contain a trailing dot.
 *
 * If flags includes DNS_SDBFLAG_THREADSAFE, the driver must be able to
 * handle multiple lookups in parallel.  Otherwise, calls into the driver
 * are serialized.
 */

void
dns_sdb_unregister(dns_sdbimplementation_t **sdbimp);
/*%<
 * Removes the simple database driver from the list of registered database
 * types.  There must be no active databases of this type when this function
 * is called.
 */

/*% See dns_sdb_putradata() */
isc_result_t
dns_sdb_putrr(dns_sdblookup_t *lookup, const char *type, dns_ttl_t ttl,
	      const char *data);
isc_result_t
dns_sdb_putrdata(dns_sdblookup_t *lookup, dns_rdatatype_t type, dns_ttl_t ttl,
		 const unsigned char *rdata, unsigned int rdlen);
/*%<
 * Add a single resource record to the lookup structure to be
 * returned in the query response.  dns_sdb_putrr() takes the
 * resource record in master file text format as a null-terminated
 * string, and dns_sdb_putrdata() takes the raw RDATA in
 * uncompressed wire format.
 */

/*% See dns_sdb_putnamerdata() */
isc_result_t
dns_sdb_putnamedrr(dns_sdballnodes_t *allnodes, const char *name,
		   const char *type, dns_ttl_t ttl, const char *data);
isc_result_t
dns_sdb_putnamedrdata(dns_sdballnodes_t *allnodes, const char *name,
		      dns_rdatatype_t type, dns_ttl_t ttl,
		      const void *rdata, unsigned int rdlen);
/*%<
 * Add a single resource record to the allnodes structure to be
 * included in a zone transfer response, in text or wire
 * format as above.
 */

isc_result_t
dns_sdb_putsoa(dns_sdblookup_t *lookup, const char *mname, const char *rname,
	       isc_uint32_t serial);
/*%<
 * This function may optionally be called from the 'authority' callback
 * to simplify construction of the SOA record for 'zone'.  It will
 * provide a SOA listing 'mname' as as the master server and 'rname' as
 * the responsible person mailbox.  It is the responsibility of the
 * driver to increment the serial number between responses if necessary.
 * All other SOA fields will have reasonable default values.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_SDB_H */
