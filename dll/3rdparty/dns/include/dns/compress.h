/*
 * Copyright (C) 2004-2007, 2009  Internet Systems Consortium, Inc. ("ISC")
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

/* $Id: compress.h,v 1.40.332.2 2009/01/18 23:47:41 tbox Exp $ */

#ifndef DNS_COMPRESS_H
#define DNS_COMPRESS_H 1

#include <isc/lang.h>
#include <isc/region.h>

#include <dns/types.h>

ISC_LANG_BEGINDECLS

#define DNS_COMPRESS_NONE		0x00	/*%< no compression */
#define DNS_COMPRESS_GLOBAL14		0x01	/*%< "normal" compression. */
#define DNS_COMPRESS_ALL		0x01	/*%< all compression. */
#define DNS_COMPRESS_CASESENSITIVE	0x02	/*%< case sensitive compression. */

/*! \file dns/compress.h
 *	Direct manipulation of the structures is strongly discouraged.
 */

#define DNS_COMPRESS_TABLESIZE 64
#define DNS_COMPRESS_INITIALNODES 16

typedef struct dns_compressnode dns_compressnode_t;

struct dns_compressnode {
	isc_region_t		r;
	isc_uint16_t		offset;
	isc_uint16_t		count;
	isc_uint8_t		labels;
	dns_compressnode_t	*next;
};

struct dns_compress {
	unsigned int		magic;		/*%< Magic number. */
	unsigned int		allowed;	/*%< Allowed methods. */
	int			edns;		/*%< Edns version or -1. */
	/*% Global compression table. */
	dns_compressnode_t	*table[DNS_COMPRESS_TABLESIZE];
	/*% Preallocated nodes for the table. */
	dns_compressnode_t	initialnodes[DNS_COMPRESS_INITIALNODES];
	isc_uint16_t		count;		/*%< Number of nodes. */
	isc_mem_t		*mctx;		/*%< Memory context. */
};

typedef enum {
	DNS_DECOMPRESS_ANY,			/*%< Any compression */
	DNS_DECOMPRESS_STRICT,			/*%< Allowed compression */
	DNS_DECOMPRESS_NONE			/*%< No compression */
} dns_decompresstype_t;

struct dns_decompress {
	unsigned int		magic;		/*%< Magic number. */
	unsigned int		allowed;	/*%< Allowed methods. */
	int			edns;		/*%< Edns version or -1. */
	dns_decompresstype_t	type;		/*%< Strict checking */
};

isc_result_t
dns_compress_init(dns_compress_t *cctx, int edns, isc_mem_t *mctx);
/*%<
 *	Initialise the compression context structure pointed to by 'cctx'.
 *
 *	Requires:
 *	\li	'cctx' is a valid dns_compress_t structure.
 *	\li	'mctx' is an initialized memory context.
 *	Ensures:
 *	\li	cctx->global is initialized.
 *
 *	Returns:
 *	\li	#ISC_R_SUCCESS
 *	\li	failures from dns_rbt_create()
 */

void
dns_compress_invalidate(dns_compress_t *cctx);

/*%<
 *	Invalidate the compression structure pointed to by cctx.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 */

void
dns_compress_setmethods(dns_compress_t *cctx, unsigned int allowed);

/*%<
 *	Sets allowed compression methods.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 */

unsigned int
dns_compress_getmethods(dns_compress_t *cctx);

/*%<
 *	Gets allowed compression methods.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *
 *	Returns:
 *\li		allowed compression bitmap.
 */

void
dns_compress_setsensitive(dns_compress_t *cctx, isc_boolean_t sensitive);

/*
 *	Preserve the case of compressed domain names.
 *
 *	Requires:
 *		'cctx' to be initialized.
 */

isc_boolean_t
dns_compress_getsensitive(dns_compress_t *cctx);
/*
 *	Return whether case is to be preserved when compressing
 *	domain names.
 *
 *	Requires:
 *		'cctx' to be initialized.
 */

int
dns_compress_getedns(dns_compress_t *cctx);

/*%<
 *	Gets edns value.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *
 *	Returns:
 *\li		-1 .. 255
 */

isc_boolean_t
dns_compress_findglobal(dns_compress_t *cctx, const dns_name_t *name,
			dns_name_t *prefix, isc_uint16_t *offset);
/*%<
 *	Finds longest possible match of 'name' in the global compression table.
 *
 *	Requires:
 *\li		'cctx' to be initialized.
 *\li		'name' to be a absolute name.
 *\li		'prefix' to be initialized.
 *\li		'offset' to point to an isc_uint16_t.
 *
 *	Ensures:
 *\li		'prefix' and 'offset' are valid if ISC_TRUE is 	returned.
 *
 *	Returns:
 *\li		#ISC_TRUE / #ISC_FALSE
 */

void
dns_compress_add(dns_compress_t *cctx, const dns_name_t *name,
		 const dns_name_t *prefix, isc_uint16_t offset);
/*%<
 *	Add compression pointers for 'name' to the compression table,
 *	not replacing existing pointers.
 *
 *	Requires:
 *\li		'cctx' initialized
 *
 *\li		'name' must be initialized and absolute, and must remain
 *		valid until the message compression is complete.
 *
 *\li		'prefix' must be a prefix returned by
 *		dns_compress_findglobal(), or the same as 'name'.
 */

void
dns_compress_rollback(dns_compress_t *cctx, isc_uint16_t offset);

/*%<
 *	Remove any compression pointers from global table >= offset.
 *
 *	Requires:
 *\li		'cctx' is initialized.
 */

void
dns_decompress_init(dns_decompress_t *dctx, int edns,
		    dns_decompresstype_t type);

/*%<
 *	Initializes 'dctx'.
 *	Records 'edns' and 'type' into the structure.
 *
 *	Requires:
 *\li		'dctx' to be a valid pointer.
 */

void
dns_decompress_invalidate(dns_decompress_t *dctx);

/*%<
 *	Invalidates 'dctx'.
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

void
dns_decompress_setmethods(dns_decompress_t *dctx, unsigned int allowed);

/*%<
 *	Sets 'dctx->allowed' to 'allowed'.
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

unsigned int
dns_decompress_getmethods(dns_decompress_t *dctx);

/*%<
 *	Returns 'dctx->allowed'
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

int
dns_decompress_edns(dns_decompress_t *dctx);

/*%<
 *	Returns 'dctx->edns'
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

dns_decompresstype_t
dns_decompress_type(dns_decompress_t *dctx);

/*%<
 *	Returns 'dctx->type'
 *
 *	Requires:
 *\li		'dctx' to be initialized
 */

ISC_LANG_ENDDECLS

#endif /* DNS_COMPRESS_H */
