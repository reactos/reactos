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

/* $Id: master.h,v 1.51 2008/04/02 02:37:42 marka Exp $ */

#ifndef DNS_MASTER_H
#define DNS_MASTER_H 1

/*! \file dns/master.h */

/***
 ***	Imports
 ***/

#include <stdio.h>

#include <isc/lang.h>

#include <dns/types.h>

/*
 * Flags to be passed in the 'options' argument in the functions below.
 */
#define	DNS_MASTER_AGETTL 	0x00000001	/*%< Age the ttl based on $DATE. */
#define DNS_MASTER_MANYERRORS 	0x00000002	/*%< Continue processing on errors. */
#define DNS_MASTER_NOINCLUDE 	0x00000004	/*%< Disallow $INCLUDE directives. */
#define DNS_MASTER_ZONE 	0x00000008	/*%< Loading a zone master file. */
#define DNS_MASTER_HINT 	0x00000010	/*%< Loading a hint master file. */
#define DNS_MASTER_SLAVE 	0x00000020	/*%< Loading a slave master file. */
#define DNS_MASTER_CHECKNS 	0x00000040	/*%<
						 * Check NS records to see
						 * if they are an address
						 */
#define DNS_MASTER_FATALNS 	0x00000080	/*%<
						 * Treat DNS_MASTER_CHECKNS
						 * matches as fatal
						 */
#define DNS_MASTER_CHECKNAMES   0x00000100
#define DNS_MASTER_CHECKNAMESFAIL 0x00000200
#define DNS_MASTER_CHECKWILDCARD 0x00000400	/* Check for internal wildcards. */
#define DNS_MASTER_CHECKMX	0x00000800
#define DNS_MASTER_CHECKMXFAIL	0x00001000

#define DNS_MASTER_RESIGN	0x00002000

ISC_LANG_BEGINDECLS

/*
 * Structures that implement the "raw" format for master dump.
 * These are provided for a reference purpose only; in the actual
 * encoding, we directly read/write each field so that the encoded data
 * is always "packed", regardless of the hardware architecture.
 */
#define DNS_RAWFORMAT_VERSION 0

/* Common header */
typedef struct {
	isc_uint32_t		format;		/* must be
						 * dns_masterformat_raw */
	isc_uint32_t		version;	/* compatibility for future
						 * extensions */
	isc_uint32_t		dumptime;	/* timestamp on creation
						 * (currently unused)
						 */
} dns_masterrawheader_t;

/* The structure for each RRset */
typedef struct {
	isc_uint32_t		totallen;	/* length of the data for this
						 * RRset, including the
						 * "header" part */
	dns_rdataclass_t	rdclass;	/* 16-bit class */
	dns_rdatatype_t		type;		/* 16-bit type */
	dns_rdatatype_t		covers;		/* same as type */
	dns_ttl_t		ttl;		/* 32-bit TTL */
	isc_uint32_t		nrdata;		/* number of RRs in this set */
	/* followed by encoded owner name, and then rdata */
} dns_masterrawrdataset_t;

/***
 ***	Function
 ***/

isc_result_t
dns_master_loadfile(const char *master_file,
		    dns_name_t *top,
		    dns_name_t *origin,
		    dns_rdataclass_t zclass,
		    unsigned int options,
		    dns_rdatacallbacks_t *callbacks,
		    isc_mem_t *mctx);

isc_result_t
dns_master_loadfile2(const char *master_file,
		     dns_name_t *top,
		     dns_name_t *origin,
		     dns_rdataclass_t zclass,
		     unsigned int options,
		     dns_rdatacallbacks_t *callbacks,
		     isc_mem_t *mctx,
		     dns_masterformat_t format);

isc_result_t
dns_master_loadfile3(const char *master_file,
		     dns_name_t *top,
		     dns_name_t *origin,
		     dns_rdataclass_t zclass,
		     unsigned int options,
		     isc_uint32_t resign,
		     dns_rdatacallbacks_t *callbacks,
		     isc_mem_t *mctx,
		     dns_masterformat_t format);

isc_result_t
dns_master_loadstream(FILE *stream,
		      dns_name_t *top,
		      dns_name_t *origin,
		      dns_rdataclass_t zclass,
		      unsigned int options,
		      dns_rdatacallbacks_t *callbacks,
		      isc_mem_t *mctx);

isc_result_t
dns_master_loadbuffer(isc_buffer_t *buffer,
		      dns_name_t *top,
		      dns_name_t *origin,
		      dns_rdataclass_t zclass,
		      unsigned int options,
		      dns_rdatacallbacks_t *callbacks,
		      isc_mem_t *mctx);

isc_result_t
dns_master_loadlexer(isc_lex_t *lex,
		     dns_name_t *top,
		     dns_name_t *origin,
		     dns_rdataclass_t zclass,
		     unsigned int options,
		     dns_rdatacallbacks_t *callbacks,
		     isc_mem_t *mctx);

isc_result_t
dns_master_loadfileinc(const char *master_file,
		       dns_name_t *top,
		       dns_name_t *origin,
		       dns_rdataclass_t zclass,
		       unsigned int options,
		       dns_rdatacallbacks_t *callbacks,
		       isc_task_t *task,
		       dns_loaddonefunc_t done, void *done_arg,
		       dns_loadctx_t **ctxp, isc_mem_t *mctx);

isc_result_t
dns_master_loadfileinc2(const char *master_file,
			dns_name_t *top,
			dns_name_t *origin,
			dns_rdataclass_t zclass,
			unsigned int options,
			dns_rdatacallbacks_t *callbacks,
			isc_task_t *task,
			dns_loaddonefunc_t done, void *done_arg,
			dns_loadctx_t **ctxp, isc_mem_t *mctx,
			dns_masterformat_t format);

isc_result_t
dns_master_loadfileinc3(const char *master_file,
			dns_name_t *top,
			dns_name_t *origin,
			dns_rdataclass_t zclass,
			unsigned int options,
			isc_uint32_t resign,
			dns_rdatacallbacks_t *callbacks,
			isc_task_t *task,
			dns_loaddonefunc_t done, void *done_arg,
			dns_loadctx_t **ctxp, isc_mem_t *mctx,
			dns_masterformat_t format);

isc_result_t
dns_master_loadstreaminc(FILE *stream,
			 dns_name_t *top,
			 dns_name_t *origin,
			 dns_rdataclass_t zclass,
			 unsigned int options,
			 dns_rdatacallbacks_t *callbacks,
			 isc_task_t *task,
			 dns_loaddonefunc_t done, void *done_arg,
			 dns_loadctx_t **ctxp, isc_mem_t *mctx);

isc_result_t
dns_master_loadbufferinc(isc_buffer_t *buffer,
			 dns_name_t *top,
			 dns_name_t *origin,
			 dns_rdataclass_t zclass,
			 unsigned int options,
			 dns_rdatacallbacks_t *callbacks,
			 isc_task_t *task,
			 dns_loaddonefunc_t done, void *done_arg,
			 dns_loadctx_t **ctxp, isc_mem_t *mctx);

isc_result_t
dns_master_loadlexerinc(isc_lex_t *lex,
			dns_name_t *top,
			dns_name_t *origin,
			dns_rdataclass_t zclass,
			unsigned int options,
			dns_rdatacallbacks_t *callbacks,
			isc_task_t *task,
			dns_loaddonefunc_t done, void *done_arg,
			dns_loadctx_t **ctxp, isc_mem_t *mctx);

/*%<
 * Loads a RFC1305 master file from a file, stream, buffer, or existing
 * lexer into rdatasets and then calls 'callbacks->commit' to commit the
 * rdatasets.  Rdata memory belongs to dns_master_load and will be
 * reused / released when the callback completes.  dns_load_master will
 * abort if callbacks->commit returns any value other than ISC_R_SUCCESS.
 *
 * If 'DNS_MASTER_AGETTL' is set and the master file contains one or more
 * $DATE directives, the TTLs of the data will be aged accordingly.
 *
 * 'callbacks->commit' is assumed to call 'callbacks->error' or
 * 'callbacks->warn' to generate any error messages required.
 *
 * 'done' is called with 'done_arg' and a result code when the loading
 * is completed or has failed.  If the initial setup fails 'done' is
 * not called.
 *
 * 'resign' the number of seconds before a RRSIG expires that it should
 * be re-signed.  0 is used if not provided.
 *
 * Requires:
 *\li	'master_file' points to a valid string.
 *\li	'lexer' points to a valid lexer.
 *\li	'top' points to a valid name.
 *\li	'origin' points to a valid name.
 *\li	'callbacks->commit' points to a valid function.
 *\li	'callbacks->error' points to a valid function.
 *\li	'callbacks->warn' points to a valid function.
 *\li	'mctx' points to a valid memory context.
 *\li	'task' and 'done' to be valid.
 *\li	'lmgr' to be valid.
 *\li	'ctxp != NULL && ctxp == NULL'.
 *
 * Returns:
 *\li	ISC_R_SUCCESS upon successfully loading the master file.
 *\li	ISC_R_SEENINCLUDE upon successfully loading the master file with
 *		a $INCLUDE statement.
 *\li	ISC_R_NOMEMORY out of memory.
 *\li	ISC_R_UNEXPECTEDEND expected to be able to read a input token and
 *		there was not one.
 *\li	ISC_R_UNEXPECTED
 *\li	DNS_R_NOOWNER failed to specify a ownername.
 *\li	DNS_R_NOTTL failed to specify a ttl.
 *\li	DNS_R_BADCLASS record class did not match zone class.
 *\li	DNS_R_CONTINUE load still in progress (dns_master_load*inc() only).
 *\li	Any dns_rdata_fromtext() error code.
 *\li	Any error code from callbacks->commit().
 */

void
dns_loadctx_detach(dns_loadctx_t **ctxp);
/*%<
 * Detach from the load context.
 *
 * Requires:
 *\li	'*ctxp' to be valid.
 *
 * Ensures:
 *\li	'*ctxp == NULL'
 */

void
dns_loadctx_attach(dns_loadctx_t *source, dns_loadctx_t **target);
/*%<
 * Attach to the load context.
 *
 * Requires:
 *\li	'source' to be valid.
 *\li	'target != NULL && *target == NULL'.
 */

void
dns_loadctx_cancel(dns_loadctx_t *ctx);
/*%<
 * Cancel loading the zone file associated with this load context.
 *
 * Requires:
 *\li	'ctx' to be valid
 */

ISC_LANG_ENDDECLS

#endif /* DNS_MASTER_H */
