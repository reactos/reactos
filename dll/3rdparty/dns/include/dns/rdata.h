/*
 * Copyright (C) 2004-2009  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1998-2003  Internet Software Consortium.
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

/* $Id: rdata.h,v 1.70.120.3 2009/02/16 00:29:27 marka Exp $ */

#ifndef DNS_RDATA_H
#define DNS_RDATA_H 1

/*****
 ***** Module Info
 *****/

/*! \file dns/rdata.h
 * \brief
 * Provides facilities for manipulating DNS rdata, including conversions to
 * and from wire format and text format.
 *
 * Given the large amount of rdata possible in a nameserver, it was important
 * to come up with a very efficient way of storing rdata, but at the same
 * time allow it to be manipulated.
 *
 * The decision was to store rdata in uncompressed wire format,
 * and not to make it a fully abstracted object; i.e. certain parts of the
 * server know rdata is stored that way.  This saves a lot of memory, and
 * makes adding rdata to messages easy.  Having much of the server know
 * the representation would be perilous, and we certainly don't want each
 * user of rdata to be manipulating such a low-level structure.  This is
 * where the rdata module comes in.  The module allows rdata handles to be
 * created and attached to uncompressed wire format regions.  All rdata
 * operations and conversions are done through these handles.
 *
 * Implementation Notes:
 *
 *\li	The routines in this module are expected to be synthesized by the
 *	build process from a set of source files, one per rdata type.  For
 *	portability, it's probably best that the building be done by a C
 *	program.  Adding a new rdata type will be a simple matter of adding
 *	a file to a directory and rebuilding the server.  *All* knowledge of
 *	the format of a particular rdata type is in this file.
 *
 * MP:
 *\li	Clients of this module must impose any required synchronization.
 *
 * Reliability:
 *\li	This module deals with low-level byte streams.  Errors in any of
 *	the functions are likely to crash the server or corrupt memory.
 *
 *\li	Rdata is typed, and the caller must know what type of rdata it has.
 *	A caller that gets this wrong could crash the server.
 *
 *\li	The fromstruct() and tostruct() routines use a void * pointer to
 *	represent the structure.  The caller must ensure that it passes a
 *	pointer to the appropriate type, or the server could crash or memory
 *	could be corrupted.
 *
 * Resources:
 *\li	None.
 *
 * Security:
 *
 *\li	*** WARNING ***
 *	dns_rdata_fromwire() deals with raw network data.  An error in
 *	this routine could result in the failure or hijacking of the server.
 *
 * Standards:
 *\li	RFC1035
 *\li	Draft EDNS0 (0)
 *\li	Draft EDNS1 (0)
 *\li	Draft Binary Labels (2)
 *\li	Draft Local Compression (1)
 *\li	Various RFCs for particular types; these will be documented in the
 *	 sources files of the types.
 *
 */

/***
 *** Imports
 ***/

#include <isc/lang.h>

#include <dns/types.h>
#include <dns/name.h>

ISC_LANG_BEGINDECLS


/***
 *** Types
 ***/

/*%
 ***** An 'rdata' is a handle to a binary region.  The handle has an RR
 ***** class and type, and the data in the binary region is in the format
 ***** of the given class and type.
 *****/
/*%
 * Clients are strongly discouraged from using this type directly, with
 * the exception of the 'link' field which may be used directly for whatever
 * purpose the client desires.
 */
struct dns_rdata {
	unsigned char *			data;
	unsigned int			length;
	dns_rdataclass_t		rdclass;
	dns_rdatatype_t			type;
	unsigned int			flags;
	ISC_LINK(dns_rdata_t)		link;
};

#define DNS_RDATA_INIT { NULL, 0, 0, 0, 0, {(void*)(-1), (void *)(-1)}}

#define DNS_RDATA_UPDATE	0x0001		/*%< update pseudo record. */
#define DNS_RDATA_OFFLINE	0x0002		/*%< RRSIG has a offline key. */

/*
 * Flags affecting rdata formatting style.  Flags 0xFFFF0000
 * are used by masterfile-level formatting and defined elsewhere.
 * See additional comments at dns_rdata_tofmttext().
 */

/*% Split the rdata into multiple lines to try to keep it
 within the "width". */
#define DNS_STYLEFLAG_MULTILINE		0x00000001U

/*% Output explanatory comments. */
#define DNS_STYLEFLAG_COMMENT		0x00000002U

#define DNS_RDATA_DOWNCASE		DNS_NAME_DOWNCASE
#define DNS_RDATA_CHECKNAMES		DNS_NAME_CHECKNAMES
#define DNS_RDATA_CHECKNAMESFAIL	DNS_NAME_CHECKNAMESFAIL
#define DNS_RDATA_CHECKREVERSE		DNS_NAME_CHECKREVERSE
#define DNS_RDATA_CHECKMX		DNS_NAME_CHECKMX
#define DNS_RDATA_CHECKMXFAIL		DNS_NAME_CHECKMXFAIL

/***
 *** Initialization
 ***/

void
dns_rdata_init(dns_rdata_t *rdata);
/*%<
 * Make 'rdata' empty.
 *
 * Requires:
 *	'rdata' is a valid rdata (i.e. not NULL, points to a struct dns_rdata)
 */

void
dns_rdata_reset(dns_rdata_t *rdata);
/*%<
 * Make 'rdata' empty.
 *
 * Requires:
 *\li	'rdata' is a previously initialized rdata and is not linked.
 */

void
dns_rdata_clone(const dns_rdata_t *src, dns_rdata_t *target);
/*%<
 * Clone 'target' from 'src'.
 *
 * Requires:
 *\li	'src' to be initialized.
 *\li	'target' to be initialized.
 */

/***
 *** Comparisons
 ***/

int
dns_rdata_compare(const dns_rdata_t *rdata1, const dns_rdata_t *rdata2);
/*%<
 * Determine the relative ordering under the DNSSEC order relation of
 * 'rdata1' and 'rdata2'.
 *
 * Requires:
 *
 *\li	'rdata1' is a valid, non-empty rdata
 *
 *\li	'rdata2' is a valid, non-empty rdata
 *
 * Returns:
 *\li	< 0		'rdata1' is less than 'rdata2'
 *\li	0		'rdata1' is equal to 'rdata2'
 *\li	> 0		'rdata1' is greater than 'rdata2'
 */

/***
 *** Conversions
 ***/

void
dns_rdata_fromregion(dns_rdata_t *rdata, dns_rdataclass_t rdclass,
		     dns_rdatatype_t type, isc_region_t *r);
/*%<
 * Make 'rdata' refer to region 'r'.
 *
 * Requires:
 *
 *\li	The data in 'r' is properly formatted for whatever type it is.
 */

void
dns_rdata_toregion(const dns_rdata_t *rdata, isc_region_t *r);
/*%<
 * Make 'r' refer to 'rdata'.
 */

isc_result_t
dns_rdata_fromwire(dns_rdata_t *rdata, dns_rdataclass_t rdclass,
		   dns_rdatatype_t type, isc_buffer_t *source,
		   dns_decompress_t *dctx, unsigned int options,
		   isc_buffer_t *target);
/*%<
 * Copy the possibly-compressed rdata at source into the target region.
 *
 * Notes:
 *\li	Name decompression policy is controlled by 'dctx'.
 *
 *	'options'
 *\li	DNS_RDATA_DOWNCASE	downcase domain names when they are copied
 *				into target.
 *
 * Requires:
 *
 *\li	'rdclass' and 'type' are valid.
 *
 *\li	'source' is a valid buffer, and the active region of 'source'
 *	references the rdata to be processed.
 *
 *\li	'target' is a valid buffer.
 *
 *\li	'dctx' is a valid decompression context.
 *
 * Ensures,
 *	if result is success:
 *	\li 	If 'rdata' is not NULL, it is attached to the target.
 *	\li	The conditions dns_name_fromwire() ensures for names hold
 *		for all names in the rdata.
 *	\li	The current location in source is advanced, and the used space
 *		in target is updated.
 *
 * Result:
 *\li	Success
 *\li	Any non-success status from dns_name_fromwire()
 *\li	Various 'Bad Form' class failures depending on class and type
 *\li	Bad Form: Input too short
 *\li	Resource Limit: Not enough space
 */

isc_result_t
dns_rdata_towire(dns_rdata_t *rdata, dns_compress_t *cctx,
		 isc_buffer_t *target);
/*%<
 * Convert 'rdata' into wire format, compressing it as specified by the
 * compression context 'cctx', and storing the result in 'target'.
 *
 * Notes:
 *\li	If the compression context allows global compression, then the
 *	global compression table may be updated.
 *
 * Requires:
 *\li	'rdata' is a valid, non-empty rdata
 *
 *\li	target is a valid buffer
 *
 *\li	Any offsets specified in a global compression table are valid
 *	for target.
 *
 * Ensures,
 *	if the result is success:
 *	\li	The used space in target is updated.
 *
 * Returns:
 *\li	Success
 *\li	Any non-success status from dns_name_towire()
 *\li	Resource Limit: Not enough space
 */

isc_result_t
dns_rdata_fromtext(dns_rdata_t *rdata, dns_rdataclass_t rdclass,
		   dns_rdatatype_t type, isc_lex_t *lexer, dns_name_t *origin,
		   unsigned int options, isc_mem_t *mctx,
		   isc_buffer_t *target, dns_rdatacallbacks_t *callbacks);
/*%<
 * Convert the textual representation of a DNS rdata into uncompressed wire
 * form stored in the target region.  Tokens constituting the text of the rdata
 * are taken from 'lexer'.
 *
 * Notes:
 *\li	Relative domain names in the rdata will have 'origin' appended to them.
 *	A NULL origin implies "origin == dns_rootname".
 *
 *
 *	'options'
 *\li	DNS_RDATA_DOWNCASE	downcase domain names when they are copied
 *				into target.
 *\li	DNS_RDATA_CHECKNAMES 	perform checknames checks.
 *\li	DNS_RDATA_CHECKNAMESFAIL fail if the checknames check fail.  If
 *				not set a warning will be issued.
 *\li	DNS_RDATA_CHECKREVERSE  this should set if the owner name ends
 *				in IP6.ARPA, IP6.INT or IN-ADDR.ARPA.
 *
 * Requires:
 *
 *\li	'rdclass' and 'type' are valid.
 *
 *\li	'lexer' is a valid isc_lex_t.
 *
 *\li	'mctx' is a valid isc_mem_t.
 *
 *\li	'target' is a valid region.
 *
 *\li	'origin' if non NULL it must be absolute.
 *
 *\li	'callbacks' to be NULL or callbacks->warn and callbacks->error be
 *	initialized.
 *
 * Ensures,
 *	if result is success:
 *\li	 	If 'rdata' is not NULL, it is attached to the target.

 *\li		The conditions dns_name_fromtext() ensures for names hold
 *		for all names in the rdata.

 *\li		The used space in target is updated.
 *
 * Result:
 *\li	Success
 *\li	Translated result codes from isc_lex_gettoken
 *\li	Various 'Bad Form' class failures depending on class and type
 *\li	Bad Form: Input too short
 *\li	Resource Limit: Not enough space
 *\li	Resource Limit: Not enough memory
 */

isc_result_t
dns_rdata_totext(dns_rdata_t *rdata, dns_name_t *origin, isc_buffer_t *target);
/*%<
 * Convert 'rdata' into text format, storing the result in 'target'.
 * The text will consist of a single line, with fields separated by
 * single spaces.
 *
 * Notes:
 *\li	If 'origin' is not NULL, then any names in the rdata that are
 *	subdomains of 'origin' will be made relative it.
 *
 *\li	XXX Do we *really* want to support 'origin'?  I'm inclined towards "no"
 *	at the moment.
 *
 * Requires:
 *
 *\li	'rdata' is a valid, non-empty rdata
 *
 *\li	'origin' is NULL, or is a valid name
 *
 *\li	'target' is a valid text buffer
 *
 * Ensures,
 *	if the result is success:
 *
 *	\li	The used space in target is updated.
 *
 * Returns:
 *\li	Success
 *\li	Any non-success status from dns_name_totext()
 *\li	Resource Limit: Not enough space
 */

isc_result_t
dns_rdata_tofmttext(dns_rdata_t *rdata, dns_name_t *origin, unsigned int flags,
		    unsigned int width, const char *linebreak,
		    isc_buffer_t *target);
/*%<
 * Like dns_rdata_totext, but do formatted output suitable for
 * database dumps.  This is intended for use by dns_db_dump();
 * library users are discouraged from calling it directly.
 *
 * If (flags & #DNS_STYLEFLAG_MULTILINE) != 0, attempt to stay
 * within 'width' by breaking the text into multiple lines.
 * The string 'linebreak' is inserted between lines, and parentheses
 * are added when necessary.  Because RRs contain unbreakable elements
 * such as domain names whose length is variable, unpredictable, and
 * potentially large, there is no guarantee that the lines will
 * not exceed 'width' anyway.
 *
 * If (flags & #DNS_STYLEFLAG_MULTILINE) == 0, the rdata is always
 * printed as a single line, and no parentheses are used.
 * The 'width' and 'linebreak' arguments are ignored.
 *
 * If (flags & #DNS_STYLEFLAG_COMMENT) != 0, output explanatory
 * comments next to things like the SOA timer fields.  Some
 * comments (e.g., the SOA ones) are only printed when multiline
 * output is selected.
 */

isc_result_t
dns_rdata_fromstruct(dns_rdata_t *rdata, dns_rdataclass_t rdclass,
		     dns_rdatatype_t type, void *source, isc_buffer_t *target);
/*%<
 * Convert the C structure representation of an rdata into uncompressed wire
 * format in 'target'.
 *
 * XXX  Should we have a 'size' parameter as a sanity check on target?
 *
 * Requires:
 *
 *\li	'rdclass' and 'type' are valid.
 *
 *\li	'source' points to a valid C struct for the class and type.
 *
 *\li	'target' is a valid buffer.
 *
 *\li	All structure pointers to memory blocks should be NULL if their
 *	corresponding length values are zero.
 *
 * Ensures,
 *	if result is success:
 *	\li 	If 'rdata' is not NULL, it is attached to the target.
 *
 *	\li	The used space in 'target' is updated.
 *
 * Result:
 *\li	Success
 *\li	Various 'Bad Form' class failures depending on class and type
 *\li	Resource Limit: Not enough space
 */

isc_result_t
dns_rdata_tostruct(dns_rdata_t *rdata, void *target, isc_mem_t *mctx);
/*%<
 * Convert an rdata into its C structure representation.
 *
 * If 'mctx' is NULL then 'rdata' must persist while 'target' is being used.
 *
 * If 'mctx' is non NULL then memory will be allocated if required.
 *
 * Requires:
 *
 *\li	'rdata' is a valid, non-empty rdata.
 *
 *\li	'target' to point to a valid pointer for the type and class.
 *
 * Result:
 *\li	Success
 *\li	Resource Limit: Not enough memory
 */

void
dns_rdata_freestruct(void *source);
/*%<
 * Free dynamic memory attached to 'source' (if any).
 *
 * Requires:
 *
 *\li	'source' to point to the structure previously filled in by
 *	dns_rdata_tostruct().
 */

isc_boolean_t
dns_rdatatype_ismeta(dns_rdatatype_t type);
/*%<
 * Return true iff the rdata type 'type' is a meta-type
 * like ANY or AXFR.
 */

isc_boolean_t
dns_rdatatype_issingleton(dns_rdatatype_t type);
/*%<
 * Return true iff the rdata type 'type' is a singleton type,
 * like CNAME or SOA.
 *
 * Requires:
 * \li	'type' is a valid rdata type.
 *
 */

isc_boolean_t
dns_rdataclass_ismeta(dns_rdataclass_t rdclass);
/*%<
 * Return true iff the rdata class 'rdclass' is a meta-class
 * like ANY or NONE.
 */

isc_boolean_t
dns_rdatatype_isdnssec(dns_rdatatype_t type);
/*%<
 * Return true iff 'type' is one of the DNSSEC
 * rdata types that may exist alongside a CNAME record.
 *
 * Requires:
 * \li	'type' is a valid rdata type.
 */

isc_boolean_t
dns_rdatatype_iszonecutauth(dns_rdatatype_t type);
/*%<
 * Return true iff rdata of type 'type' is considered authoritative
 * data (not glue) in the NSEC chain when it occurs in the parent zone
 * at a zone cut.
 *
 * Requires:
 * \li	'type' is a valid rdata type.
 *
 */

isc_boolean_t
dns_rdatatype_isknown(dns_rdatatype_t type);
/*%<
 * Return true iff the rdata type 'type' is known.
 *
 * Requires:
 * \li	'type' is a valid rdata type.
 *
 */


isc_result_t
dns_rdata_additionaldata(dns_rdata_t *rdata, dns_additionaldatafunc_t add,
			 void *arg);
/*%<
 * Call 'add' for each name and type from 'rdata' which is subject to
 * additional section processing.
 *
 * Requires:
 *
 *\li	'rdata' is a valid, non-empty rdata.
 *
 *\li	'add' is a valid dns_additionalfunc_t.
 *
 * Ensures:
 *
 *\li	If successful, then add() will have been called for each name
 *	and type subject to additional section processing.
 *
 *\li	If add() returns something other than #ISC_R_SUCCESS, that result
 *	will be returned as the result of dns_rdata_additionaldata().
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Many other results are possible if not successful.
 */

isc_result_t
dns_rdata_digest(dns_rdata_t *rdata, dns_digestfunc_t digest, void *arg);
/*%<
 * Send 'rdata' in DNSSEC canonical form to 'digest'.
 *
 * Note:
 *\li	'digest' may be called more than once by dns_rdata_digest().  The
 *	concatenation of all the regions, in the order they were given
 *	to 'digest', will be the DNSSEC canonical form of 'rdata'.
 *
 * Requires:
 *
 *\li	'rdata' is a valid, non-empty rdata.
 *
 *\li	'digest' is a valid dns_digestfunc_t.
 *
 * Ensures:
 *
 *\li	If successful, then all of the rdata's data has been sent, in
 *	DNSSEC canonical form, to 'digest'.
 *
 *\li	If digest() returns something other than ISC_R_SUCCESS, that result
 *	will be returned as the result of dns_rdata_digest().
 *
 * Returns:
 *
 *\li	ISC_R_SUCCESS
 *
 *\li	Many other results are possible if not successful.
 */

isc_boolean_t
dns_rdatatype_questiononly(dns_rdatatype_t type);
/*%<
 * Return true iff rdata of type 'type' can only appear in the question
 * section of a properly formatted message.
 *
 * Requires:
 * \li	'type' is a valid rdata type.
 *
 */

isc_boolean_t
dns_rdatatype_notquestion(dns_rdatatype_t type);
/*%<
 * Return true iff rdata of type 'type' can not appear in the question
 * section of a properly formatted message.
 *
 * Requires:
 * \li	'type' is a valid rdata type.
 *
 */

isc_boolean_t
dns_rdatatype_atparent(dns_rdatatype_t type);
/*%<
 * Return true iff rdata of type 'type' should appear at the parent of
 * a zone cut.
 *
 * Requires:
 * \li	'type' is a valid rdata type.
 *
 */

unsigned int
dns_rdatatype_attributes(dns_rdatatype_t rdtype);
/*%<
 * Return attributes for the given type.
 *
 * Requires:
 *\li	'rdtype' are known.
 *
 * Returns:
 *\li	a bitmask consisting of the following flags.
 */

/*% only one may exist for a name */
#define DNS_RDATATYPEATTR_SINGLETON		0x00000001U
/*% requires no other data be present */
#define DNS_RDATATYPEATTR_EXCLUSIVE		0x00000002U
/*% Is a meta type */
#define DNS_RDATATYPEATTR_META			0x00000004U
/*% Is a DNSSEC type, like RRSIG or NSEC */
#define DNS_RDATATYPEATTR_DNSSEC		0x00000008U
/*% Is a zone cut authority type */
#define DNS_RDATATYPEATTR_ZONECUTAUTH		0x00000010U
/*% Is reserved (unusable) */
#define DNS_RDATATYPEATTR_RESERVED		0x00000020U
/*% Is an unknown type */
#define DNS_RDATATYPEATTR_UNKNOWN		0x00000040U
/*% Is META, and can only be in a question section */
#define DNS_RDATATYPEATTR_QUESTIONONLY		0x00000080U
/*% is META, and can NOT be in a question section */
#define DNS_RDATATYPEATTR_NOTQUESTION		0x00000100U
/*% Is present at zone cuts in the parent, not the child */
#define DNS_RDATATYPEATTR_ATPARENT		0x00000200U

dns_rdatatype_t
dns_rdata_covers(dns_rdata_t *rdata);
/*%<
 * Return the rdatatype that this type covers.
 *
 * Requires:
 *\li	'rdata' is a valid, non-empty rdata.
 *
 *\li	'rdata' is a type that covers other rdata types.
 *
 * Returns:
 *\li	The type covered.
 */

isc_boolean_t
dns_rdata_checkowner(dns_name_t* name, dns_rdataclass_t rdclass,
		     dns_rdatatype_t type, isc_boolean_t wildcard);
/*
 * Returns whether this is a valid ownername for this <type,class>.
 * If wildcard is true allow the first label to be a wildcard if
 * appropriate.
 *
 * Requires:
 *	'name' is a valid name.
 */

isc_boolean_t
dns_rdata_checknames(dns_rdata_t *rdata, dns_name_t *owner, dns_name_t *bad);
/*
 * Returns whether 'rdata' contains valid domain names.  The checks are
 * sensitive to the owner name.
 *
 * If 'bad' is non-NULL and a domain name fails the check the
 * the offending name will be return in 'bad' by cloning from
 * the 'rdata' contents.
 *
 * Requires:
 *	'rdata' to be valid.
 *	'owner' to be valid.
 *	'bad'	to be NULL or valid.
 */

ISC_LANG_ENDDECLS

#endif /* DNS_RDATA_H */
