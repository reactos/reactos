/*
 * Portions Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Portions Copyright (C) 2000-2002  Internet Software Consortium.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * Portions Copyright (C) 1995-2000 by Network Associates, Inc.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND ISC AND NETWORK ASSOCIATES DISCLAIMS
 * ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
 * IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/* $Id: dst_parse.h,v 1.11 2008/05/15 00:50:26 each Exp $ */

/*! \file */
#ifndef DST_DST_PARSE_H
#define DST_DST_PARSE_H 1

#include <isc/lang.h>

#include <dst/dst.h>

#define MAJOR_VERSION		1
#define MINOR_VERSION		2

#define MAXFIELDSIZE		512
#define MAXFIELDS		12

#define TAG_SHIFT		4
#define TAG_ALG(tag)		((unsigned int)(tag) >> TAG_SHIFT)
#define TAG(alg, off)		(((alg) << TAG_SHIFT) + (off))

/* These are used by both RSA-MD5 and RSA-SHA1 */
#define RSA_NTAGS		11
#define TAG_RSA_MODULUS		((DST_ALG_RSAMD5 << TAG_SHIFT) + 0)
#define TAG_RSA_PUBLICEXPONENT	((DST_ALG_RSAMD5 << TAG_SHIFT) + 1)
#define TAG_RSA_PRIVATEEXPONENT	((DST_ALG_RSAMD5 << TAG_SHIFT) + 2)
#define TAG_RSA_PRIME1		((DST_ALG_RSAMD5 << TAG_SHIFT) + 3)
#define TAG_RSA_PRIME2		((DST_ALG_RSAMD5 << TAG_SHIFT) + 4)
#define TAG_RSA_EXPONENT1	((DST_ALG_RSAMD5 << TAG_SHIFT) + 5)
#define TAG_RSA_EXPONENT2	((DST_ALG_RSAMD5 << TAG_SHIFT) + 6)
#define TAG_RSA_COEFFICIENT	((DST_ALG_RSAMD5 << TAG_SHIFT) + 7)
#define TAG_RSA_ENGINE		((DST_ALG_RSAMD5 << TAG_SHIFT) + 8)
#define TAG_RSA_LABEL		((DST_ALG_RSAMD5 << TAG_SHIFT) + 9)
#define TAG_RSA_PIN		((DST_ALG_RSAMD5 << TAG_SHIFT) + 10)

#define DH_NTAGS		4
#define TAG_DH_PRIME		((DST_ALG_DH << TAG_SHIFT) + 0)
#define TAG_DH_GENERATOR	((DST_ALG_DH << TAG_SHIFT) + 1)
#define TAG_DH_PRIVATE		((DST_ALG_DH << TAG_SHIFT) + 2)
#define TAG_DH_PUBLIC		((DST_ALG_DH << TAG_SHIFT) + 3)

#define DSA_NTAGS		5
#define TAG_DSA_PRIME		((DST_ALG_DSA << TAG_SHIFT) + 0)
#define TAG_DSA_SUBPRIME	((DST_ALG_DSA << TAG_SHIFT) + 1)
#define TAG_DSA_BASE		((DST_ALG_DSA << TAG_SHIFT) + 2)
#define TAG_DSA_PRIVATE		((DST_ALG_DSA << TAG_SHIFT) + 3)
#define TAG_DSA_PUBLIC		((DST_ALG_DSA << TAG_SHIFT) + 4)

#define OLD_HMACMD5_NTAGS	1
#define HMACMD5_NTAGS		2
#define TAG_HMACMD5_KEY		((DST_ALG_HMACMD5 << TAG_SHIFT) + 0)
#define TAG_HMACMD5_BITS	((DST_ALG_HMACMD5 << TAG_SHIFT) + 1)

#define HMACSHA1_NTAGS		2
#define TAG_HMACSHA1_KEY	((DST_ALG_HMACSHA1 << TAG_SHIFT) + 0)
#define TAG_HMACSHA1_BITS	((DST_ALG_HMACSHA1 << TAG_SHIFT) + 1)

#define HMACSHA224_NTAGS	2
#define TAG_HMACSHA224_KEY	((DST_ALG_HMACSHA224 << TAG_SHIFT) + 0)
#define TAG_HMACSHA224_BITS	((DST_ALG_HMACSHA224 << TAG_SHIFT) + 1)

#define HMACSHA256_NTAGS	2
#define TAG_HMACSHA256_KEY	((DST_ALG_HMACSHA256 << TAG_SHIFT) + 0)
#define TAG_HMACSHA256_BITS	((DST_ALG_HMACSHA256 << TAG_SHIFT) + 1)

#define HMACSHA384_NTAGS	2
#define TAG_HMACSHA384_KEY	((DST_ALG_HMACSHA384 << TAG_SHIFT) + 0)
#define TAG_HMACSHA384_BITS	((DST_ALG_HMACSHA384 << TAG_SHIFT) + 1)

#define HMACSHA512_NTAGS	2
#define TAG_HMACSHA512_KEY	((DST_ALG_HMACSHA512 << TAG_SHIFT) + 0)
#define TAG_HMACSHA512_BITS	((DST_ALG_HMACSHA512 << TAG_SHIFT) + 1)

struct dst_private_element {
	unsigned short tag;
	unsigned short length;
	unsigned char *data;
};

typedef struct dst_private_element dst_private_element_t;

struct dst_private {
	unsigned short nelements;
	dst_private_element_t elements[MAXFIELDS];
};

typedef struct dst_private dst_private_t;

ISC_LANG_BEGINDECLS

void
dst__privstruct_free(dst_private_t *priv, isc_mem_t *mctx);

int
dst__privstruct_parse(dst_key_t *key, unsigned int alg, isc_lex_t *lex,
		      isc_mem_t *mctx, dst_private_t *priv);

int
dst__privstruct_writefile(const dst_key_t *key, const dst_private_t *priv,
			  const char *directory);

ISC_LANG_ENDDECLS

#endif /* DST_DST_PARSE_H */
