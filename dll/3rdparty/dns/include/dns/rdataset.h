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

/* $Id: rdataset.h,v 1.65.50.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_RDATASET_H
#define DNS_RDATASET_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/rdataset.h
 * \brief
 * A DNS rdataset is a handle that can be associated with a collection of
 * rdata all having a common owner name, class, and type.
 *
 * The dns_rdataset_t type is like a "virtual class".  To actually use
 * rdatasets, an implementation of the method suite (e.g. "slabbed rdata") is
 * required.
 *
 * XXX &lt;more&gt; XXX
 *
 * MP:
 *\li	Clients of this module must impose any required synchronization.
 *
 * Reliability:
 *\li	No anticipated impact.
 *
 * Resources:
 *\li	TBS
 *
 * Security:
 *\li	No anticipated impact.
 *
 * Standards:
 *\li	None.
 */

#include <isc/lang.h>
#include <isc/magic.h>
#include <isc/stdtime.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

typedef enum {
	dns_rdatasetadditional_fromauth,
	dns_rdatasetadditional_fromcache,
	dns_rdatasetadditional_fromglue
} dns_rdatasetadditional_t;

typedef struct dns_rdatasetmethods {
	void			(*disassociate)(dns_rdataset_t *rdataset);
	isc_result_t		(*first)(dns_rdataset_t *rdataset);
	isc_result_t		(*next)(dns_rdataset_t *rdataset);
	void			(*current)(dns_rdataset_t *rdataset,
					   dns_rdata_t *rdata);
	void			(*clone)(dns_rdataset_t *source,
					 dns_rdataset_t *target);
	unsigned int		(*count)(dns_rdataset_t *rdataset);
	isc_result_t		(*addnoqname)(dns_rdataset_t *rdataset,
					      dns_name_t *name);
	isc_result_t		(*getnoqname)(dns_rdataset_t *rdataset,
					      dns_name_t *name,
					      dns_rdataset_t *neg,
					      dns_rdataset_t *negsig);
	isc_result_t		(*addclosest)(dns_rdataset_t *rdataset,
					      dns_name_t *name);
	isc_result_t		(*getclosest)(dns_rdataset_t *rdataset,
					      dns_name_t *name,
					      dns_rdataset_t *neg,
					      dns_rdataset_t *negsig);
	isc_result_t		(*getadditional)(dns_rdataset_t *rdataset,
						 dns_rdatasetadditional_t type,
						 dns_rdatatype_t qtype,
						 dns_acache_t *acache,
						 dns_zone_t **zonep,
						 dns_db_t **dbp,
						 dns_dbversion_t **versionp,
						 dns_dbnode_t **nodep,
						 dns_name_t *fname,
						 dns_message_t *msg,
						 isc_stdtime_t now);
	isc_result_t		(*setadditional)(dns_rdataset_t *rdataset,
						 dns_rdatasetadditional_t type,
						 dns_rdatatype_t qtype,
						 dns_acache_t *acache,
						 dns_zone_t *zone,
						 dns_db_t *db,
						 dns_dbversion_t *version,
						 dns_dbnode_t *node,
						 dns_name_t *fname);
	isc_result_t		(*putadditional)(dns_acache_t *acache,
						 dns_rdataset_t *rdataset,
						 dns_rdatasetadditional_t type,
						 dns_rdatatype_t qtype);
} dns_rdatasetmethods_t;

#define DNS_RDATASET_MAGIC	       ISC_MAGIC('D','N','S','R')
#define DNS_RDATASET_VALID(set)	       ISC_MAGIC_VALID(set, DNS_RDATASET_MAGIC)

/*%
 * Direct use of this structure by clients is strongly discouraged, except
 * for the 'link' field which may be used however the client wishes.  The
 * 'private', 'current', and 'index' fields MUST NOT be changed by clients.
 * rdataset implementations may change any of the fields.
 */
struct dns_rdataset {
	unsigned int			magic;		/* XXX ? */
	dns_rdatasetmethods_t *		methods;
	ISC_LINK(dns_rdataset_t)	link;
	/*
	 * XXX do we need these, or should they be retrieved by methods?
	 * Leaning towards the latter, since they are not frequently required
	 * once you have the rdataset.
	 */
	dns_rdataclass_t		rdclass;
	dns_rdatatype_t			type;
	dns_ttl_t			ttl;
	dns_trust_t			trust;
	dns_rdatatype_t			covers;
	/*
	 * attributes
	 */
	unsigned int			attributes;
	/*%
	 * the counter provides the starting point in the "cyclic" order.
	 * The value ISC_UINT32_MAX has a special meaning of "picking up a
	 * random value." in order to take care of databases that do not
	 * increment the counter.
	 */
	isc_uint32_t			count;
	/*
	 * This RRSIG RRset should be re-generated around this time.
	 * Only valid if DNS_RDATASETATTR_RESIGN is set in attributes.
	 */
	isc_stdtime_t			resign;
	/*@{*/
	/*%
	 * These are for use by the rdataset implementation, and MUST NOT
	 * be changed by clients.
	 */
	void *				private1;
	void *				private2;
	void *				private3;
	unsigned int			privateuint4;
	void *				private5;
	void *				private6;
	void *				private7;
	/*@}*/

};

/*!
 * \def DNS_RDATASETATTR_RENDERED
 *	Used by message.c to indicate that the rdataset was rendered.
 *
 * \def DNS_RDATASETATTR_TTLADJUSTED
 *	Used by message.c to indicate that the rdataset's rdata had differing
 *	TTL values, and the rdataset->ttl holds the smallest.
 *
 * \def DNS_RDATASETATTR_LOADORDER
 *	Output the RRset in load order.
 */

#define DNS_RDATASETATTR_QUESTION	0x00000001
#define DNS_RDATASETATTR_RENDERED	0x00000002	/*%< Used by message.c */
#define DNS_RDATASETATTR_ANSWERED	0x00000004	/*%< Used by server. */
#define DNS_RDATASETATTR_CACHE		0x00000008	/*%< Used by resolver. */
#define DNS_RDATASETATTR_ANSWER		0x00000010	/*%< Used by resolver. */
#define DNS_RDATASETATTR_ANSWERSIG	0x00000020	/*%< Used by resolver. */
#define DNS_RDATASETATTR_EXTERNAL	0x00000040	/*%< Used by resolver. */
#define DNS_RDATASETATTR_NCACHE		0x00000080	/*%< Used by resolver. */
#define DNS_RDATASETATTR_CHAINING	0x00000100	/*%< Used by resolver. */
#define DNS_RDATASETATTR_TTLADJUSTED	0x00000200	/*%< Used by message.c */
#define DNS_RDATASETATTR_FIXEDORDER	0x00000400
#define DNS_RDATASETATTR_RANDOMIZE	0x00000800
#define DNS_RDATASETATTR_CHASE		0x00001000	/*%< Used by resolver. */
#define DNS_RDATASETATTR_NXDOMAIN	0x00002000
#define DNS_RDATASETATTR_NOQNAME	0x00004000
#define DNS_RDATASETATTR_CHECKNAMES	0x00008000	/*%< Used by resolver. */
#define DNS_RDATASETATTR_REQUIREDGLUE	0x00010000
#define DNS_RDATASETATTR_LOADORDER	0x00020000
#define DNS_RDATASETATTR_RESIGN		0x00040000
#define DNS_RDATASETATTR_CLOSEST	0x00080000
#define DNS_RDATASETATTR_OPTOUT		0x00100000	/*%< OPTOUT proof */

/*%
 * _OMITDNSSEC:
 * 	Omit DNSSEC records when rendering ncache records.
 */
#define DNS_RDATASETTOWIRE_OMITDNSSEC	0x0001

void
dns_rdataset_init(dns_rdataset_t *rdataset);
/*%<
 * Make 'rdataset' a valid, disassociated rdataset.
 *
 * Requires:
 *\li	'rdataset' is not NULL.
 *
 * Ensures:
 *\li	'rdataset' is a valid, disassociated rdataset.
 */

void
dns_rdataset_invalidate(dns_rdataset_t *rdataset);
/*%<
 * Invalidate 'rdataset'.
 *
 * Requires:
 *\li	'rdataset' is a valid, disassociated rdataset.
 *
 * Ensures:
 *\li	If assertion checking is enabled, future attempts to use 'rdataset'
 *	without initializing it will cause an assertion failure.
 */

void
dns_rdataset_disassociate(dns_rdataset_t *rdataset);
/*%<
 * Disassociate 'rdataset' from its rdata, allowing it to be reused.
 *
 * Notes:
 *\li	The client must ensure it has no references to rdata in the rdataset
 *	before disassociating.
 *
 * Requires:
 *\li	'rdataset' is a valid, associated rdataset.
 *
 * Ensures:
 *\li	'rdataset' is a valid, disassociated rdataset.
 */

isc_boolean_t
dns_rdataset_isassociated(dns_rdataset_t *rdataset);
/*%<
 * Is 'rdataset' associated?
 *
 * Requires:
 *\li	'rdataset' is a valid rdataset.
 *
 * Returns:
 *\li	#ISC_TRUE			'rdataset' is associated.
 *\li	#ISC_FALSE			'rdataset' is not associated.
 */

void
dns_rdataset_makequestion(dns_rdataset_t *rdataset, dns_rdataclass_t rdclass,
			  dns_rdatatype_t type);
/*%<
 * Make 'rdataset' a valid, associated, question rdataset, with a
 * question class of 'rdclass' and type 'type'.
 *
 * Notes:
 *\li	Question rdatasets have a class and type, but no rdata.
 *
 * Requires:
 *\li	'rdataset' is a valid, disassociated rdataset.
 *
 * Ensures:
 *\li	'rdataset' is a valid, associated, question rdataset.
 */

void
dns_rdataset_clone(dns_rdataset_t *source, dns_rdataset_t *target);
/*%<
 * Make 'target' refer to the same rdataset as 'source'.
 *
 * Requires:
 *\li	'source' is a valid, associated rdataset.
 *
 *\li	'target' is a valid, dissociated rdataset.
 *
 * Ensures:
 *\li	'target' references the same rdataset as 'source'.
 */

unsigned int
dns_rdataset_count(dns_rdataset_t *rdataset);
/*%<
 * Return the number of records in 'rdataset'.
 *
 * Requires:
 *\li	'rdataset' is a valid, associated rdataset.
 *
 * Returns:
 *\li	The number of records in 'rdataset'.
 */

isc_result_t
dns_rdataset_first(dns_rdataset_t *rdataset);
/*%<
 * Move the rdata cursor to the first rdata in the rdataset (if any).
 *
 * Requires:
 *\li	'rdataset' is a valid, associated rdataset.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no rdata in the set.
 */

isc_result_t
dns_rdataset_next(dns_rdataset_t *rdataset);
/*%<
 * Move the rdata cursor to the next rdata in the rdataset (if any).
 *
 * Requires:
 *\li	'rdataset' is a valid, associated rdataset.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	#ISC_R_NOMORE			There are no more rdata in the set.
 */

void
dns_rdataset_current(dns_rdataset_t *rdataset, dns_rdata_t *rdata);
/*%<
 * Make 'rdata' refer to the current rdata.
 *
 * Notes:
 *
 *\li	The data returned in 'rdata' is valid for the life of the
 *	rdataset; in particular, subsequent changes in the cursor position
 *	do not invalidate 'rdata'.
 *
 * Requires:
 *\li	'rdataset' is a valid, associated rdataset.
 *
 *\li	The rdata cursor of 'rdataset' is at a valid location (i.e. the
 *	result of last call to a cursor movement command was ISC_R_SUCCESS).
 *
 * Ensures:
 *\li	'rdata' refers to the rdata at the rdata cursor location of
 *\li	'rdataset'.
 */

isc_result_t
dns_rdataset_totext(dns_rdataset_t *rdataset,
		    dns_name_t *owner_name,
		    isc_boolean_t omit_final_dot,
		    isc_boolean_t question,
		    isc_buffer_t *target);
/*%<
 * Convert 'rdataset' to text format, storing the result in 'target'.
 *
 * Notes:
 *\li	The rdata cursor position will be changed.
 *
 *\li	The 'question' flag should normally be #ISC_FALSE.  If it is
 *	#ISC_TRUE, the TTL and rdata fields are not printed.  This is
 *	for use when printing an rdata representing a question section.
 *
 *\li	This interface is deprecated; use dns_master_rdatasettottext()
 * 	and/or dns_master_questiontotext() instead.
 *
 * Requires:
 *\li	'rdataset' is a valid rdataset.
 *
 *\li	'rdataset' is not empty.
 */

isc_result_t
dns_rdataset_towire(dns_rdataset_t *rdataset,
		    dns_name_t *owner_name,
		    dns_compress_t *cctx,
		    isc_buffer_t *target,
		    unsigned int options,
		    unsigned int *countp);
/*%<
 * Convert 'rdataset' to wire format, compressing names as specified
 * in 'cctx', and storing the result in 'target'.
 *
 * Notes:
 *\li	The rdata cursor position will be changed.
 *
 *\li	The number of RRs added to target will be added to *countp.
 *
 * Requires:
 *\li	'rdataset' is a valid rdataset.
 *
 *\li	'rdataset' is not empty.
 *
 *\li	'countp' is a valid pointer.
 *
 * Ensures:
 *\li	On a return of ISC_R_SUCCESS, 'target' contains a wire format
 *	for the data contained in 'rdataset'.  Any error return leaves
 *	the buffer unchanged.
 *
 *\li	*countp has been incremented by the number of RRs added to
 *	target.
 *
 * Returns:
 *\li	#ISC_R_SUCCESS		- all ok
 *\li	#ISC_R_NOSPACE		- 'target' doesn't have enough room
 *
 *\li	Any error returned by dns_rdata_towire(), dns_rdataset_next(),
 *	dns_name_towire().
 */

isc_result_t
dns_rdataset_towiresorted(dns_rdataset_t *rdataset,
			  const dns_name_t *owner_name,
			  dns_compress_t *cctx,
			  isc_buffer_t *target,
			  dns_rdatasetorderfunc_t order,
			  const void *order_arg,
			  unsigned int options,
			  unsigned int *countp);
/*%<
 * Like dns_rdataset_towire(), but sorting the rdatasets according to
 * the integer value returned by 'order' when called with the rdataset
 * and 'order_arg' as arguments.
 *
 * Requires:
 *\li	All the requirements of dns_rdataset_towire(), and
 *	that order_arg is NULL if and only if order is NULL.
 */

isc_result_t
dns_rdataset_towirepartial(dns_rdataset_t *rdataset,
			   const dns_name_t *owner_name,
			   dns_compress_t *cctx,
			   isc_buffer_t *target,
			   dns_rdatasetorderfunc_t order,
			   const void *order_arg,
			   unsigned int options,
			   unsigned int *countp,
			   void **state);
/*%<
 * Like dns_rdataset_towiresorted() except that a partial rdataset
 * may be written.
 *
 * Requires:
 *\li	All the requirements of dns_rdataset_towiresorted().
 *	If 'state' is non NULL then the current position in the
 *	rdataset will be remembered if the rdataset in not
 *	completely written and should be passed on on subsequent
 *	calls (NOT CURRENTLY IMPLEMENTED).
 *
 * Returns:
 *\li	#ISC_R_SUCCESS if all of the records were written.
 *\li	#ISC_R_NOSPACE if unable to fit in all of the records. *countp
 *		      will be updated to reflect the number of records
 *		      written.
 */

isc_result_t
dns_rdataset_additionaldata(dns_rdataset_t *rdataset,
			    dns_additionaldatafunc_t add, void *arg);
/*%<
 * For each rdata in rdataset, call 'add' for each name and type in the
 * rdata which is subject to additional section processing.
 *
 * Requires:
 *
 *\li	'rdataset' is a valid, non-question rdataset.
 *
 *\li	'add' is a valid dns_additionaldatafunc_t
 *
 * Ensures:
 *
 *\li	If successful, dns_rdata_additionaldata() will have been called for
 *	each rdata in 'rdataset'.
 *
 *\li	If a call to dns_rdata_additionaldata() is not successful, the
 *	result returned will be the result of dns_rdataset_additionaldata().
 *
 * Returns:
 *
 *\li	#ISC_R_SUCCESS
 *
 *\li	Any error that dns_rdata_additionaldata() can return.
 */

isc_result_t
dns_rdataset_getnoqname(dns_rdataset_t *rdataset, dns_name_t *name,
			dns_rdataset_t *neg, dns_rdataset_t *negsig);
/*%<
 * Return the noqname proof for this record.
 *
 * Requires:
 *\li	'rdataset' to be valid and #DNS_RDATASETATTR_NOQNAME to be set.
 *\li	'name' to be valid.
 *\li	'neg' and 'negsig' to be valid and not associated.
 */

isc_result_t
dns_rdataset_addnoqname(dns_rdataset_t *rdataset, dns_name_t *name);
/*%<
 * Associate a noqname proof with this record.
 * Sets #DNS_RDATASETATTR_NOQNAME if successful.
 * Adjusts the 'rdataset->ttl' to minimum of the 'rdataset->ttl' and
 * the 'nsec'/'nsec3' and 'rrsig(nsec)'/'rrsig(nsec3)' ttl.
 *
 * Requires:
 *\li	'rdataset' to be valid and #DNS_RDATASETATTR_NOQNAME to be set.
 *\li	'name' to be valid and have NSEC or NSEC3 and associated RRSIG
 *	 rdatasets.
 */

isc_result_t
dns_rdataset_getclosest(dns_rdataset_t *rdataset, dns_name_t *name,
			dns_rdataset_t *nsec, dns_rdataset_t *nsecsig);
/*%<
 * Return the closest encloser for this record.
 *
 * Requires:
 *\li	'rdataset' to be valid and #DNS_RDATASETATTR_CLOSEST to be set.
 *\li	'name' to be valid.
 *\li	'nsec' and 'nsecsig' to be valid and not associated.
 */

isc_result_t
dns_rdataset_addclosest(dns_rdataset_t *rdataset, dns_name_t *name);
/*%<
 * Associate a closest encloset proof with this record.
 * Sets #DNS_RDATASETATTR_CLOSEST if successful.
 * Adjusts the 'rdataset->ttl' to minimum of the 'rdataset->ttl' and
 * the 'nsec' and 'rrsig(nsec)' ttl.
 *
 * Requires:
 *\li	'rdataset' to be valid and #DNS_RDATASETATTR_CLOSEST to be set.
 *\li	'name' to be valid and have NSEC3 and RRSIG(NSEC3) rdatasets.
 */

isc_result_t
dns_rdataset_getadditional(dns_rdataset_t *rdataset,
			   dns_rdatasetadditional_t type,
			   dns_rdatatype_t qtype,
			   dns_acache_t *acache,
			   dns_zone_t **zonep,
			   dns_db_t **dbp,
			   dns_dbversion_t **versionp,
			   dns_dbnode_t **nodep,
			   dns_name_t *fname,
			   dns_message_t *msg,
			   isc_stdtime_t now);
/*%<
 * Get cached additional information from the DB node for a particular
 * 'rdataset.'  'type' is one of dns_rdatasetadditional_fromauth,
 * dns_rdatasetadditional_fromcache, and dns_rdatasetadditional_fromglue,
 * which specifies the origin of the information.  'qtype' is intended to
 * be used for specifying a particular rdata type in the cached information.
 *
 * Requires:
 * \li	'rdataset' is a valid rdataset.
 * \li	'acache' can be NULL, in which case this function will simply return
 * 	ISC_R_FAILURE.
 * \li	For the other pointers, see dns_acache_getentry().
 *
 * Ensures:
 * \li	See dns_acache_getentry().
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_FAILURE	- additional information caching is not supported.
 * \li	#ISC_R_NOTFOUND	- the corresponding DB node has not cached additional
 *			  information for 'rdataset.'
 * \li	Any error that dns_acache_getentry() can return.
 */

isc_result_t
dns_rdataset_setadditional(dns_rdataset_t *rdataset,
			   dns_rdatasetadditional_t type,
			   dns_rdatatype_t qtype,
			   dns_acache_t *acache,
			   dns_zone_t *zone,
			   dns_db_t *db,
			   dns_dbversion_t *version,
			   dns_dbnode_t *node,
			   dns_name_t *fname);
/*%<
 * Set cached additional information to the DB node for a particular
 * 'rdataset.'  See dns_rdataset_getadditional for the semantics of 'type'
 * and 'qtype'.
 *
 * Requires:
 * \li	'rdataset' is a valid rdataset.
 * \li	'acache' can be NULL, in which case this function will simply return
 *	ISC_R_FAILURE.
 * \li	For the other pointers, see dns_acache_setentry().
 *
 * Ensures:
 * \li	See dns_acache_setentry().
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_FAILURE	- additional information caching is not supported.
 * \li	#ISC_R_NOMEMORY
 * \li	Any error that dns_acache_setentry() can return.
 */

isc_result_t
dns_rdataset_putadditional(dns_acache_t *acache,
			   dns_rdataset_t *rdataset,
			   dns_rdatasetadditional_t type,
			   dns_rdatatype_t qtype);
/*%<
 * Discard cached additional information stored in the DB node for a particular
 * 'rdataset.'  See dns_rdataset_getadditional for the semantics of 'type'
 * and 'qtype'.
 *
 * Requires:
 * \li	'rdataset' is a valid rdataset.
 * \li	'acache' can be NULL, in which case this function will simply return
 *	ISC_R_FAILURE.
 *
 * Ensures:
 * \li	See dns_acache_cancelentry().
 *
 * Returns:
 * \li	#ISC_R_SUCCESS
 * \li	#ISC_R_FAILURE	- additional information caching is not supported.
 * \li	#ISC_R_NOTFOUND	- the corresponding DB node has not cached additional
 *			  information for 'rdataset.'
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RDATASET_H */
