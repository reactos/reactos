/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 1999-2002  Internet Software Consortium.
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

/* $Id: masterdump.h,v 1.42 2008/09/24 02:46:23 marka Exp $ */

#ifndef DNS_MASTERDUMP_H
#define DNS_MASTERDUMP_H 1

/*! \file dns/masterdump.h */

/***
 ***	Imports
 ***/

#include <stdio.h>

#include <isc/lang.h>

#include <dns/types.h>

/***
 *** Types
 ***/

typedef struct dns_master_style dns_master_style_t;

/***
 *** Definitions
 ***/

/*
 * Flags affecting master file formatting.  Flags 0x0000FFFF
 * define the formatting of the rdata part and are defined in
 * rdata.h.
 */

/*% Omit the owner name when possible. */
#define	DNS_STYLEFLAG_OMIT_OWNER        0x00010000U

/*%
 * Omit the TTL when possible.  If DNS_STYLEFLAG_TTL is
 * also set, this means no TTLs are ever printed
 * because $TTL directives are generated before every
 * change in the TTL.  In this case, no columns need to
 * be reserved for the TTL.  Master files generated with
 * these options will be rejected by BIND 4.x because it
 * does not recognize the $TTL directive.
 *
 * If DNS_STYLEFLAG_TTL is not also set, the TTL will be
 * omitted when it is equal to the previous TTL.
 * This is correct according to RFC1035, but the
 * TTLs may be silently misinterpreted by older
 * versions of BIND which use the SOA MINTTL as a
 * default TTL value.
 */
#define	DNS_STYLEFLAG_OMIT_TTL		0x00020000U

/*% Omit the class when possible. */
#define	DNS_STYLEFLAG_OMIT_CLASS	0x00040000U

/*% Output $TTL directives. */
#define	DNS_STYLEFLAG_TTL		0x00080000U

/*%
 * Output $ORIGIN directives and print owner names relative to
 * the origin when possible.
 */
#define	DNS_STYLEFLAG_REL_OWNER		0x00100000U

/*% Print domain names in RR data in relative form when possible.
   For this to take effect, DNS_STYLEFLAG_REL_OWNER must also be set. */
#define	DNS_STYLEFLAG_REL_DATA		0x00200000U

/*% Print the trust level of each rdataset. */
#define	DNS_STYLEFLAG_TRUST		0x00400000U

/*% Print negative caching entries. */
#define	DNS_STYLEFLAG_NCACHE		0x00800000U

/*% Never print the TTL. */
#define	DNS_STYLEFLAG_NO_TTL		0x01000000U

/*% Never print the CLASS. */
#define	DNS_STYLEFLAG_NO_CLASS		0x02000000U

/*% Report re-signing time. */
#define	DNS_STYLEFLAG_RESIGN		0x04000000U

ISC_LANG_BEGINDECLS

/***
 ***	Constants
 ***/

/*%
 * The default master file style.
 *
 * This uses $TTL directives to avoid the need to dedicate a
 * tab stop for the TTL.  The class is only printed for the first
 * rrset in the file and shares a tab stop with the RR type.
 */
LIBDNS_EXTERNAL_DATA extern const dns_master_style_t dns_master_style_default;

/*%
 * A master file style that dumps zones to a very generic format easily
 * imported/checked with external tools.
 */
LIBDNS_EXTERNAL_DATA extern const dns_master_style_t dns_master_style_full;

/*%
 * A master file style that prints explicit TTL values on each
 * record line, never using $TTL statements.  The TTL has a tab
 * stop of its own, but the class and type share one.
 */
LIBDNS_EXTERNAL_DATA extern const dns_master_style_t
					dns_master_style_explicitttl;

/*%
 * A master style format designed for cache files.  It prints explicit TTL
 * values on each record line and never uses $ORIGIN or relative names.
 */
LIBDNS_EXTERNAL_DATA extern const dns_master_style_t dns_master_style_cache;

/*%
 * A master style that prints name, ttl, class, type, and value on
 * every line.  Similar to explicitttl above, but more verbose.
 * Intended for generating master files which can be easily parsed
 * by perl scripts and similar applications.
 */
LIBDNS_EXTERNAL_DATA extern const dns_master_style_t dns_master_style_simple;

/*%
 * The style used for debugging, "dig" output, etc.
 */
LIBDNS_EXTERNAL_DATA extern const dns_master_style_t dns_master_style_debug;

/***
 ***	Functions
 ***/

void
dns_dumpctx_attach(dns_dumpctx_t *source, dns_dumpctx_t **target);
/*%<
 * Attach to a dump context.
 *
 * Require:
 *\li	'source' to be valid.
 *\li	'target' to be non NULL and '*target' to be NULL.
 */

void
dns_dumpctx_detach(dns_dumpctx_t **dctxp);
/*%<
 * Detach from a dump context.
 *
 * Require:
 *\li	'dctxp' to point to a valid dump context.
 *
 * Ensures:
 *\li	'*dctxp' is NULL.
 */

void
dns_dumpctx_cancel(dns_dumpctx_t *dctx);
/*%<
 * Cancel a in progress dump.
 *
 * Require:
 *\li	'dctx' to be valid.
 */

dns_dbversion_t *
dns_dumpctx_version(dns_dumpctx_t *dctx);
/*%<
 * Return the version handle (if any) of the database being dumped.
 *
 * Require:
 *\li	'dctx' to be valid.
 */

dns_db_t *
dns_dumpctx_db(dns_dumpctx_t *dctx);
/*%<
 * Return the database being dumped.
 *
 * Require:
 *\li	'dctx' to be valid.
 */


/*@{*/
isc_result_t
dns_master_dumptostreaminc(isc_mem_t *mctx, dns_db_t *db,
			   dns_dbversion_t *version,
			   const dns_master_style_t *style, FILE *f,
			   isc_task_t *task, dns_dumpdonefunc_t done,
			   void *done_arg, dns_dumpctx_t **dctxp);

isc_result_t
dns_master_dumptostream(isc_mem_t *mctx, dns_db_t *db,
			dns_dbversion_t *version,
			const dns_master_style_t *style, FILE *f);

isc_result_t
dns_master_dumptostream2(isc_mem_t *mctx, dns_db_t *db,
			 dns_dbversion_t *version,
			 const dns_master_style_t *style,
			 dns_masterformat_t format, FILE *f);
/*%<
 * Dump the database 'db' to the steam 'f' in the specified format by
 * 'format'.  If the format is dns_masterformat_text (the RFC1035 format),
 * 'style' specifies the file style (e.g., &dns_master_style_default).
 *
 * dns_master_dumptostream() is an old form of dns_master_dumptostream2(),
 * which always specifies the dns_masterformat_text format.
 *
 * Temporary dynamic memory may be allocated from 'mctx'.
 *
 * Require:
 *\li	'task' to be valid.
 *\li	'done' to be non NULL.
 *\li	'dctxp' to be non NULL && '*dctxp' to be NULL.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_CONTINUE	dns_master_dumptostreaminc() only.
 *\li	ISC_R_NOMEMORY
 *\li	Any database or rrset iterator error.
 *\li	Any dns_rdata_totext() error code.
 */
/*@}*/

/*@{*/
isc_result_t
dns_master_dumpinc(isc_mem_t *mctx, dns_db_t *db, dns_dbversion_t *version,
		   const dns_master_style_t *style, const char *filename,
		   isc_task_t *task, dns_dumpdonefunc_t done, void *done_arg,
		   dns_dumpctx_t **dctxp);

isc_result_t
dns_master_dumpinc2(isc_mem_t *mctx, dns_db_t *db, dns_dbversion_t *version,
		    const dns_master_style_t *style, const char *filename,
		    isc_task_t *task, dns_dumpdonefunc_t done, void *done_arg,			    dns_dumpctx_t **dctxp, dns_masterformat_t format);

isc_result_t
dns_master_dump(isc_mem_t *mctx, dns_db_t *db,
		dns_dbversion_t *version,
		const dns_master_style_t *style, const char *filename);

isc_result_t
dns_master_dump2(isc_mem_t *mctx, dns_db_t *db,
		 dns_dbversion_t *version,
		 const dns_master_style_t *style, const char *filename,
		 dns_masterformat_t format);

/*%<
 * Dump the database 'db' to the file 'filename' in the specified format by
 * 'format'.  If the format is dns_masterformat_text (the RFC1035 format),
 * 'style' specifies the file style (e.g., &dns_master_style_default).
 *
 * dns_master_dumpinc() and dns_master_dump() are old forms of _dumpinc2()
 * and _dump2(), respectively, which always specify the dns_masterformat_text
 * format.
 *
 * Temporary dynamic memory may be allocated from 'mctx'.
 *
 * Returns:
 *\li	ISC_R_SUCCESS
 *\li	ISC_R_CONTINUE	dns_master_dumpinc() only.
 *\li	ISC_R_NOMEMORY
 *\li	Any database or rrset iterator error.
 *\li	Any dns_rdata_totext() error code.
 */
/*@}*/

isc_result_t
dns_master_rdatasettotext(dns_name_t *owner_name,
			  dns_rdataset_t *rdataset,
			  const dns_master_style_t *style,
			  isc_buffer_t *target);
/*%<
 * Convert 'rdataset' to text format, storing the result in 'target'.
 *
 * Notes:
 *\li	The rdata cursor position will be changed.
 *
 * Requires:
 *\li	'rdataset' is a valid non-question rdataset.
 *
 *\li	'rdataset' is not empty.
 */

isc_result_t
dns_master_questiontotext(dns_name_t *owner_name,
			  dns_rdataset_t *rdataset,
			  const dns_master_style_t *style,
			  isc_buffer_t *target);

isc_result_t
dns_master_dumpnodetostream(isc_mem_t *mctx, dns_db_t *db,
			    dns_dbversion_t *version,
			    dns_dbnode_t *node, dns_name_t *name,
			    const dns_master_style_t *style,
			    FILE *f);

isc_result_t
dns_master_dumpnode(isc_mem_t *mctx, dns_db_t *db, dns_dbversion_t *version,
		    dns_dbnode_t *node, dns_name_t *name,
		    const dns_master_style_t *style, const char *filename);

isc_result_t
dns_master_stylecreate(dns_master_style_t **style, unsigned int flags,
		       unsigned int ttl_column, unsigned int class_column,
		       unsigned int type_column, unsigned int rdata_column,
		       unsigned int line_length, unsigned int tab_width,
		       isc_mem_t *mctx);

void
dns_master_styledestroy(dns_master_style_t **style, isc_mem_t *mctx);

const char *
dns_trust_totext(dns_trust_t trust);

ISC_LANG_ENDDECLS

#endif /* DNS_MASTERDUMP_H */
