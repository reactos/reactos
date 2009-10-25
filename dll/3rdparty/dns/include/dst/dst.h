/*
 * Copyright (C) 2004-2008  Internet Systems Consortium, Inc. ("ISC")
 * Copyright (C) 2000-2002  Internet Software Consortium.
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

/* $Id: dst.h,v 1.12 2008/09/24 02:46:23 marka Exp $ */

#ifndef DST_DST_H
#define DST_DST_H 1

/*! \file dst/dst.h */

#include <isc/lang.h>

#include <dns/types.h>

#include <dst/gssapi.h>

ISC_LANG_BEGINDECLS

/***
 *** Types
 ***/

/*%
 * The dst_key structure is opaque.  Applications should use the accessor
 * functions provided to retrieve key attributes.  If an application needs
 * to set attributes, new accessor functions will be written.
 */

typedef struct dst_key		dst_key_t;
typedef struct dst_context 	dst_context_t;

/* DST algorithm codes */
#define DST_ALG_UNKNOWN		0
#define DST_ALG_RSAMD5		1
#define DST_ALG_RSA		DST_ALG_RSAMD5	/*%< backwards compatibility */
#define DST_ALG_DH		2
#define DST_ALG_DSA		3
#define DST_ALG_ECC		4
#define DST_ALG_RSASHA1		5
#define DST_ALG_NSEC3DSA	6
#define DST_ALG_NSEC3RSASHA1	7
#define DST_ALG_HMACMD5		157
#define DST_ALG_GSSAPI		160
#define DST_ALG_HMACSHA1	161	/* XXXMPA */
#define DST_ALG_HMACSHA224	162	/* XXXMPA */
#define DST_ALG_HMACSHA256	163	/* XXXMPA */
#define DST_ALG_HMACSHA384	164	/* XXXMPA */
#define DST_ALG_HMACSHA512	165	/* XXXMPA */
#define DST_ALG_PRIVATE		254
#define DST_ALG_EXPAND		255
#define DST_MAX_ALGS		255

/*% A buffer of this size is large enough to hold any key */
#define DST_KEY_MAXSIZE		1280

/*%
 * A buffer of this size is large enough to hold the textual representation
 * of any key
 */
#define DST_KEY_MAXTEXTSIZE	2048

/*% 'Type' for dst_read_key() */
#define DST_TYPE_KEY		0x1000000	/* KEY key */
#define DST_TYPE_PRIVATE	0x2000000
#define DST_TYPE_PUBLIC		0x4000000

/***
 *** Functions
 ***/

isc_result_t
dst_lib_init(isc_mem_t *mctx, isc_entropy_t *ectx, unsigned int eflags);
/*%<
 * Initializes the DST subsystem.
 *
 * Requires:
 * \li 	"mctx" is a valid memory context
 * \li	"ectx" is a valid entropy context
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOMEMORY
 *
 * Ensures:
 * \li	DST is properly initialized.
 */

void
dst_lib_destroy(void);
/*%<
 * Releases all resources allocated by DST.
 */

isc_boolean_t
dst_algorithm_supported(unsigned int alg);
/*%<
 * Checks that a given algorithm is supported by DST.
 *
 * Returns:
 * \li	ISC_TRUE
 * \li	ISC_FALSE
 */

isc_result_t
dst_context_create(dst_key_t *key, isc_mem_t *mctx, dst_context_t **dctxp);
/*%<
 * Creates a context to be used for a sign or verify operation.
 *
 * Requires:
 * \li	"key" is a valid key.
 * \li	"mctx" is a valid memory context.
 * \li	dctxp != NULL && *dctxp == NULL
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	ISC_R_NOMEMORY
 *
 * Ensures:
 * \li	*dctxp will contain a usable context.
 */

void
dst_context_destroy(dst_context_t **dctxp);
/*%<
 * Destroys all memory associated with a context.
 *
 * Requires:
 * \li	*dctxp != NULL && *dctxp == NULL
 *
 * Ensures:
 * \li	*dctxp == NULL
 */

isc_result_t
dst_context_adddata(dst_context_t *dctx, const isc_region_t *data);
/*%<
 * Incrementally adds data to the context to be used in a sign or verify
 * operation.
 *
 * Requires:
 * \li	"dctx" is a valid context
 * \li	"data" is a valid region
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	DST_R_SIGNFAILURE
 * \li	all other errors indicate failure
 */

isc_result_t
dst_context_sign(dst_context_t *dctx, isc_buffer_t *sig);
/*%<
 * Computes a signature using the data and key stored in the context.
 *
 * Requires:
 * \li	"dctx" is a valid context.
 * \li	"sig" is a valid buffer.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	DST_R_VERIFYFAILURE
 * \li	all other errors indicate failure
 *
 * Ensures:
 * \li	"sig" will contain the signature
 */

isc_result_t
dst_context_verify(dst_context_t *dctx, isc_region_t *sig);
/*%<
 * Verifies the signature using the data and key stored in the context.
 *
 * Requires:
 * \li	"dctx" is a valid context.
 * \li	"sig" is a valid region.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	all other errors indicate failure
 *
 * Ensures:
 * \li	"sig" will contain the signature
 */

isc_result_t
dst_key_computesecret(const dst_key_t *pub, const dst_key_t *priv,
		      isc_buffer_t *secret);
/*%<
 * Computes a shared secret from two (Diffie-Hellman) keys.
 *
 * Requires:
 * \li	"pub" is a valid key that can be used to derive a shared secret
 * \li	"priv" is a valid private key that can be used to derive a shared secret
 * \li	"secret" is a valid buffer
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 * \li	If successful, secret will contain the derived shared secret.
 */

isc_result_t
dst_key_fromfile(dns_name_t *name, dns_keytag_t id, unsigned int alg, int type,
		 const char *directory, isc_mem_t *mctx, dst_key_t **keyp);
/*%<
 * Reads a key from permanent storage.  The key can either be a public or
 * private key, and is specified by name, algorithm, and id.  If a private key
 * is specified, the public key must also be present.  If directory is NULL,
 * the current directory is assumed.
 *
 * Requires:
 * \li	"name" is a valid absolute dns name.
 * \li	"id" is a valid key tag identifier.
 * \li	"alg" is a supported key algorithm.
 * \li	"type" is DST_TYPE_PUBLIC, DST_TYPE_PRIVATE, or the bitwise union.
 *		  DST_TYPE_KEY look for a KEY record otherwise DNSKEY
 * \li	"mctx" is a valid memory context.
 * \li	"keyp" is not NULL and "*keyp" is NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 * \li	If successful, *keyp will contain a valid key.
 */

isc_result_t
dst_key_fromnamedfile(const char *filename, int type, isc_mem_t *mctx,
		      dst_key_t **keyp);
/*%<
 * Reads a key from permanent storage.  The key can either be a public or
 * key, and is specified by filename.  If a private key is specified, the
 * public key must also be present.
 *
 * Requires:
 * \li	"filename" is not NULL
 * \li	"type" is DST_TYPE_PUBLIC, DST_TYPE_PRIVATE, or the bitwise union
 *		  DST_TYPE_KEY look for a KEY record otherwise DNSKEY
 * \li	"mctx" is a valid memory context
 * \li	"keyp" is not NULL and "*keyp" is NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 * \li	If successful, *keyp will contain a valid key.
 */


isc_result_t
dst_key_read_public(const char *filename, int type,
		    isc_mem_t *mctx, dst_key_t **keyp);
/*%<
 * Reads a public key from permanent storage.  The key must be a public key.
 *
 * Requires:
 * \li	"filename" is not NULL
 * \li	"type" is DST_TYPE_KEY look for a KEY record otherwise DNSKEY
 * \li	"mctx" is a valid memory context
 * \li	"keyp" is not NULL and "*keyp" is NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	DST_R_BADKEYTYPE if the key type is not the expected one
 * \li	ISC_R_UNEXPECTEDTOKEN if the file can not be parsed as a public key
 * \li	any other result indicates failure
 *
 * Ensures:
 * \li	If successful, *keyp will contain a valid key.
 */

isc_result_t
dst_key_tofile(const dst_key_t *key, int type, const char *directory);
/*%<
 * Writes a key to permanent storage.  The key can either be a public or
 * private key.  Public keys are written in DNS format and private keys
 * are written as a set of base64 encoded values.  If directory is NULL,
 * the current directory is assumed.
 *
 * Requires:
 * \li	"key" is a valid key.
 * \li	"type" is DST_TYPE_PUBLIC, DST_TYPE_PRIVATE, or the bitwise union
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	any other result indicates failure
 */

isc_result_t
dst_key_fromdns(dns_name_t *name, dns_rdataclass_t rdclass,
		isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp);
/*%<
 * Converts a DNS KEY record into a DST key.
 *
 * Requires:
 * \li	"name" is a valid absolute dns name.
 * \li	"source" is a valid buffer.  There must be at least 4 bytes available.
 * \li	"mctx" is a valid memory context.
 * \li	"keyp" is not NULL and "*keyp" is NULL.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 * \li	If successful, *keyp will contain a valid key, and the consumed
 *	pointer in data will be advanced.
 */

isc_result_t
dst_key_todns(const dst_key_t *key, isc_buffer_t *target);
/*%<
 * Converts a DST key into a DNS KEY record.
 *
 * Requires:
 * \li	"key" is a valid key.
 * \li	"target" is a valid buffer.  There must be at least 4 bytes unused.
 *
 * Returns:
 * \li	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 * \li	If successful, the used pointer in 'target' is advanced by at least 4.
 */

isc_result_t
dst_key_frombuffer(dns_name_t *name, unsigned int alg,
		   unsigned int flags, unsigned int protocol,
		   dns_rdataclass_t rdclass,
		   isc_buffer_t *source, isc_mem_t *mctx, dst_key_t **keyp);
/*%<
 * Converts a buffer containing DNS KEY RDATA into a DST key.
 *
 * Requires:
 *\li	"name" is a valid absolute dns name.
 *\li	"alg" is a supported key algorithm.
 *\li	"source" is a valid buffer.
 *\li	"mctx" is a valid memory context.
 *\li	"keyp" is not NULL and "*keyp" is NULL.
 *
 * Returns:
 *\li 	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 *\li	If successful, *keyp will contain a valid key, and the consumed
 *	pointer in source will be advanced.
 */

isc_result_t
dst_key_tobuffer(const dst_key_t *key, isc_buffer_t *target);
/*%<
 * Converts a DST key into DNS KEY RDATA format.
 *
 * Requires:
 *\li	"key" is a valid key.
 *\li	"target" is a valid buffer.
 *
 * Returns:
 *\li 	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 *\li	If successful, the used pointer in 'target' is advanced.
 */

isc_result_t
dst_key_privatefrombuffer(dst_key_t *key, isc_buffer_t *buffer);
/*%<
 * Converts a public key into a private key, reading the private key
 * information from the buffer.  The buffer should contain the same data
 * as the .private key file would.
 *
 * Requires:
 *\li	"key" is a valid public key.
 *\li	"buffer" is not NULL.
 *
 * Returns:
 *\li 	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 *\li	If successful, key will contain a valid private key.
 */

gss_ctx_id_t
dst_key_getgssctx(const dst_key_t *key);
/*%<
 * Returns the opaque key data.
 * Be cautions when using this value unless you know what you are doing.
 *
 * Requires:
 *\li	"key" is not NULL.
 *
 * Returns:
 *\li	gssctx key data, possibly NULL.
 */

isc_result_t
dst_key_fromgssapi(dns_name_t *name, gss_ctx_id_t gssctx, isc_mem_t *mctx,
		   dst_key_t **keyp);
/*%<
 * Converts a GSSAPI opaque context id into a DST key.
 *
 * Requires:
 *\li	"name" is a valid absolute dns name.
 *\li	"gssctx" is a GSSAPI context id.
 *\li	"mctx" is a valid memory context.
 *\li	"keyp" is not NULL and "*keyp" is NULL.
 *
 * Returns:
 *\li 	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 *\li	If successful, *keyp will contain a valid key and be responsible for
 *	the context id.
 */

isc_result_t
dst_key_fromlabel(dns_name_t *name, int alg, unsigned int flags,
		  unsigned int protocol, dns_rdataclass_t rdclass,
		  const char *engine, const char *label, const char *pin,
		  isc_mem_t *mctx, dst_key_t **keyp);

isc_result_t
dst_key_generate(dns_name_t *name, unsigned int alg,
		 unsigned int bits, unsigned int param,
		 unsigned int flags, unsigned int protocol,
		 dns_rdataclass_t rdclass,
		 isc_mem_t *mctx, dst_key_t **keyp);
/*%<
 * Generate a DST key (or keypair) with the supplied parameters.  The
 * interpretation of the "param" field depends on the algorithm:
 * \code
 * 	RSA:	exponent
 * 		0	use exponent 3
 * 		!0	use Fermat4 (2^16 + 1)
 * 	DH:	generator
 * 		0	default - use well known prime if bits == 768 or 1024,
 * 			otherwise use 2 as the generator.
 * 		!0	use this value as the generator.
 * 	DSA:	unused
 * 	HMACMD5: entropy
 *		0	default - require good entropy
 *		!0	lack of good entropy is ok
 *\endcode
 *
 * Requires:
 *\li	"name" is a valid absolute dns name.
 *\li	"keyp" is not NULL and "*keyp" is NULL.
 *
 * Returns:
 *\li 	ISC_R_SUCCESS
 * \li	any other result indicates failure
 *
 * Ensures:
 *\li	If successful, *keyp will contain a valid key.
 */

isc_boolean_t
dst_key_compare(const dst_key_t *key1, const dst_key_t *key2);
/*%<
 * Compares two DST keys.
 *
 * Requires:
 *\li	"key1" is a valid key.
 *\li	"key2" is a valid key.
 *
 * Returns:
 *\li 	ISC_TRUE
 * \li	ISC_FALSE
 */

isc_boolean_t
dst_key_paramcompare(const dst_key_t *key1, const dst_key_t *key2);
/*%<
 * Compares the parameters of two DST keys.  This is used to determine if
 * two (Diffie-Hellman) keys can be used to derive a shared secret.
 *
 * Requires:
 *\li	"key1" is a valid key.
 *\li	"key2" is a valid key.
 *
 * Returns:
 *\li 	ISC_TRUE
 * \li	ISC_FALSE
 */

void
dst_key_free(dst_key_t **keyp);
/*%<
 * Release all memory associated with the key.
 *
 * Requires:
 *\li	"keyp" is not NULL and "*keyp" is a valid key.
 *
 * Ensures:
 *\li	All memory associated with "*keyp" will be freed.
 *\li	*keyp == NULL
 */

/*%<
 * Accessor functions to obtain key fields.
 *
 * Require:
 *\li	"key" is a valid key.
 */
dns_name_t *
dst_key_name(const dst_key_t *key);

unsigned int
dst_key_size(const dst_key_t *key);

unsigned int
dst_key_proto(const dst_key_t *key);

unsigned int
dst_key_alg(const dst_key_t *key);

isc_uint32_t
dst_key_flags(const dst_key_t *key);

dns_keytag_t
dst_key_id(const dst_key_t *key);

dns_rdataclass_t
dst_key_class(const dst_key_t *key);

isc_boolean_t
dst_key_isprivate(const dst_key_t *key);

isc_boolean_t
dst_key_iszonekey(const dst_key_t *key);

isc_boolean_t
dst_key_isnullkey(const dst_key_t *key);

isc_result_t
dst_key_buildfilename(const dst_key_t *key, int type,
		      const char *directory, isc_buffer_t *out);
/*%<
 * Generates the filename used by dst to store the specified key.
 * If directory is NULL, the current directory is assumed.
 *
 * Requires:
 *\li	"key" is a valid key
 *\li	"type" is either DST_TYPE_PUBLIC, DST_TYPE_PRIVATE, or 0 for no suffix.
 *\li	"out" is a valid buffer
 *
 * Ensures:
 *\li	the file name will be written to "out", and the used pointer will
 *		be advanced.
 */

isc_result_t
dst_key_sigsize(const dst_key_t *key, unsigned int *n);
/*%<
 * Computes the size of a signature generated by the given key.
 *
 * Requires:
 *\li	"key" is a valid key.
 *\li	"n" is not NULL
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	DST_R_UNSUPPORTEDALG
 *
 * Ensures:
 *\li	"n" stores the size of a generated signature
 */

isc_result_t
dst_key_secretsize(const dst_key_t *key, unsigned int *n);
/*%<
 * Computes the size of a shared secret generated by the given key.
 *
 * Requires:
 *\li	"key" is a valid key.
 *\li	"n" is not NULL
 *
 * Returns:
 *\li	#ISC_R_SUCCESS
 *\li	DST_R_UNSUPPORTEDALG
 *
 * Ensures:
 *\li	"n" stores the size of a generated shared secret
 */

isc_uint16_t
dst_region_computeid(const isc_region_t *source, unsigned int alg);
/*%<
 * Computes the key id of the key stored in the provided region with the
 * given algorithm.
 *
 * Requires:
 *\li	"source" contains a valid, non-NULL region.
 *
 * Returns:
 *\li 	the key id
 */

isc_uint16_t
dst_key_getbits(const dst_key_t *key);
/*
 * Get the number of digest bits required (0 == MAX).
 *
 * Requires:
 *	"key" is a valid key.
 */

void
dst_key_setbits(dst_key_t *key, isc_uint16_t bits);
/*
 * Set the number of digest bits required (0 == MAX).
 *
 * Requires:
 *	"key" is a valid key.
 */

ISC_LANG_ENDDECLS

#endif /* DST_DST_H */
