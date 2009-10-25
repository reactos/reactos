/*
 * Portions Copyright (C) 2005-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 1999-2001  Internet Software Consortium.
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

/*
 * Copyright (C) 2002 Stichting NLnet, Netherlands, stichting@nlnet.nl.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND STICHTING NLNET
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * STICHTING NLNET BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * The development of Dynamically Loadable Zones (DLZ) for Bind 9 was
 * conceived and contributed by Rob Butler.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ROB BUTLER
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * ROB BUTLER BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
 * OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
 * USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: dlz.h,v 1.7.332.2 2009/01/18 23:47:41 tbox Exp $ */

/*! \file dns/dlz.h */

#ifndef DLZ_H
#define DLZ_H 1

/*****
 ***** Module Info
 *****/

/*
 * DLZ Interface
 *
 * The DLZ interface allows zones to be looked up using a driver instead of
 * Bind's default in memory zone table.
 *
 *
 * Reliability:
 *	No anticipated impact.
 *
 * Resources:
 *
 * Security:
 *	No anticipated impact.
 *
 * Standards:
 *	None.
 */

/*****
 ***** Imports
 *****/

#include <dns/name.h>
#include <dns/types.h>
#include <dns/view.h>

#include <isc/lang.h>

ISC_LANG_BEGINDECLS

/***
 *** Types
 ***/

#define DNS_DLZ_MAGIC		ISC_MAGIC('D','L','Z','D')
#define DNS_DLZ_VALID(dlz)	ISC_MAGIC_VALID(dlz, DNS_DLZ_MAGIC)

typedef isc_result_t
(*dns_dlzallowzonexfr_t)(void *driverarg, void *dbdata, isc_mem_t *mctx,
			 dns_rdataclass_t rdclass, dns_name_t *name,
			 isc_sockaddr_t *clientaddr,
			 dns_db_t **dbp);

/*%<
 * Method prototype.  Drivers implementing the DLZ interface MUST
 * supply an allow zone transfer method.  This method is called when
 * the DNS server is performing a zone transfer query.  The driver's
 * method should return ISC_R_SUCCESS and a database pointer to the
 * name server if the zone is supported by the database, and zone
 * transfer is allowed.  Otherwise it will return ISC_R_NOTFOUND if
 * the zone is not supported by the database, or ISC_R_NOPERM if zone
 * transfers are not allowed.  If an error occurs it should return a
 * result code indicating the type of error.
 */

typedef isc_result_t
(*dns_dlzcreate_t)(isc_mem_t *mctx, const char *dlzname, unsigned int argc,
		   char *argv[], void *driverarg, void **dbdata);

/*%<
 * Method prototype.  Drivers implementing the DLZ interface MUST
 * supply a create method.  This method is called when the DNS server
 * is starting up and creating drivers for use later.
 */

typedef void
(*dns_dlzdestroy_t)(void *driverarg, void **dbdata);

/*%<
 * Method prototype.  Drivers implementing the DLZ interface MUST
 * supply a destroy method.  This method is called when the DNS server
 * is shutting down and no longer needs the driver.
 */

typedef isc_result_t
(*dns_dlzfindzone_t)(void *driverarg, void *dbdata, isc_mem_t *mctx,
		     dns_rdataclass_t rdclass, dns_name_t *name,
		     dns_db_t **dbp);

/*%<

 * Method prototype.  Drivers implementing the DLZ interface MUST
 * supply a find zone method.  This method is called when the DNS
 * server is performing a query.  The find zone method will be called
 * with the longest possible name first, and continue to be called
 * with successively shorter domain names, until any of the following
 * occur:
 *
 * \li	1) a match is found, and the function returns (ISC_R_SUCCESS)
 *
 * \li	2) a problem occurs, and the functions returns anything other
 *	   than (ISC_R_NOTFOUND)
 * \li	3) we run out of domain name labels. I.E. we have tried the
 *	   shortest domain name
 * \li	4) the number of labels in the domain name is less than
 *	   min_labels for dns_dlzfindzone
 *
 * The driver's find zone method should return ISC_R_SUCCESS and a
 * database pointer to the name server if the zone is supported by the
 * database.  Otherwise it will return ISC_R_NOTFOUND, and a null
 * pointer if the zone is not supported.  If an error occurs it should
 * return a result code indicating the type of error.
 */

/*% the methods supplied by a DLZ driver */
typedef struct dns_dlzmethods {
	dns_dlzcreate_t		create;
	dns_dlzdestroy_t	destroy;
	dns_dlzfindzone_t	findzone;
	dns_dlzallowzonexfr_t	allowzonexfr;
} dns_dlzmethods_t;

/*% information about a DLZ driver */
struct dns_dlzimplementation {
	const char				*name;
	const dns_dlzmethods_t			*methods;
	isc_mem_t				*mctx;
	void					*driverarg;
	ISC_LINK(dns_dlzimplementation_t)	link;
};

/*% an instance of a DLZ driver */
struct dns_dlzdb {
	unsigned int		magic;
	isc_mem_t		*mctx;
	dns_dlzimplementation_t	*implementation;
	void			*dbdata;
};


/***
 *** Method declarations
 ***/

isc_result_t
dns_dlzallowzonexfr(dns_view_t *view, dns_name_t *name,
		    isc_sockaddr_t *clientaddr, dns_db_t **dbp);

/*%<
 * This method is called when the DNS server is performing a zone
 * transfer query.  It will call the DLZ driver's allow zone transfer
 * method.
 */

isc_result_t
dns_dlzcreate(isc_mem_t *mctx, const char *dlzname,
	      const char *drivername, unsigned int argc,
	      char *argv[], dns_dlzdb_t **dbp);

/*%<
 * This method is called when the DNS server is starting up and
 * creating drivers for use later.  It will search the DLZ driver list
 * for 'drivername' and return a DLZ driver via dbp if a match is
 * found.  If the DLZ driver supplies a create method, this function
 * will call it.
 */

void
dns_dlzdestroy(dns_dlzdb_t **dbp);

/*%<
 * This method is called when the DNS server is shutting down and no
 * longer needs the driver.  If the DLZ driver supplies a destroy
 * methods, this function will call it.
 */

isc_result_t
dns_dlzfindzone(dns_view_t *view, dns_name_t *name,
		unsigned int minlabels, dns_db_t **dbp);

/*%<
 * This method is called when the DNS server is performing a query.
 * It will call the DLZ driver's find zone method.
 */

isc_result_t
dns_dlzregister(const char *drivername, const dns_dlzmethods_t *methods,
		 void *driverarg, isc_mem_t *mctx,
		dns_dlzimplementation_t **dlzimp);

/*%<
 * Register a dynamically loadable zones (DLZ) driver for the database
 * type 'drivername', implemented by the functions in '*methods'.
 *
 * dlzimp must point to a NULL dlz_implementation_t pointer.  That is,
 * dlzimp != NULL && *dlzimp == NULL.  It will be assigned a value that
 * will later be used to identify the driver when deregistering it.
 */

isc_result_t
dns_dlzstrtoargv(isc_mem_t *mctx, char *s, unsigned int *argcp, char ***argvp);

/*%<
 * This method is called when the name server is starting up to parse
 * the DLZ driver command line from named.conf.  Basically it splits
 * up a string into and argc / argv.  The primary difference of this
 * method is items between braces { } are considered only 1 word.  for
 * example the command line "this is { one grouped phrase } and this
 * isn't" would be parsed into:
 *
 * \li	argv[0]: "this"
 * \li	argv[1]: "is"
 * \li	argv{2]: " one grouped phrase "
 * \li	argv[3]: "and"
 * \li	argv[4]: "this"
 * \li	argv{5}: "isn't"
 *
 * braces should NOT be nested, more than one grouping in the command
 * line is allowed.  Notice, argv[2] has an extra space at the
 * beginning and end.  Extra spaces are not stripped between a
 * grouping.  You can do so in your driver if needed, or be sure not
 * to put extra spaces before / after the braces.
 */

void
dns_dlzunregister(dns_dlzimplementation_t **dlzimp);

/*%<
 * Removes the dlz driver from the list of registered dlz drivers.
 * There must be no active dlz drivers of this type when this function
 * is called.
 */

ISC_LANG_ENDDECLS

#endif /* DLZ_H */
