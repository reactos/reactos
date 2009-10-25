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

/* $Id: dnssec.h,v 1.32.332.4 2009/06/08 23:47:00 tbox Exp $ */

#ifndef DNS_DNSSEC_H
#define DNS_DNSSEC_H 1

/*! \file dns/dnssec.h */

#include <isc/lang.h>
#include <isc/stdtime.h>

#include <dns/types.h>

#include <dst/dst.h>

ISC_LANG_BEGINDECLS

isc_result_t
dns_dnssec_keyfromrdata(dns_name_t *name, dns_rdata_t *rdata, isc_mem_t *mctx,
			dst_key_t **key);
/*%<
 *	Creates a DST key from a DNS record.  Basically a wrapper around
 *	dst_key_fromdns().
 *
 *	Requires:
 *\li		'name' is not NULL
 *\li		'rdata' is not NULL
 *\li		'mctx' is not NULL
 *\li		'key' is not NULL
 *\li		'*key' is NULL
 *
 *	Returns:
 *\li		#ISC_R_SUCCESS
 *\li		#ISC_R_NOMEMORY
 *\li		DST_R_INVALIDPUBLICKEY
 *\li		various errors from dns_name_totext
 */

isc_result_t
dns_dnssec_sign(dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		isc_stdtime_t *inception, isc_stdtime_t *expire,
		isc_mem_t *mctx, isc_buffer_t *buffer, dns_rdata_t *sigrdata);
/*%<
 *	Generates a SIG record covering this rdataset.  This has no effect
 *	on existing SIG records.
 *
 *	Requires:
 *\li		'name' (the owner name of the record) is a valid name
 *\li		'set' is a valid rdataset
 *\li		'key' is a valid key
 *\li		'inception' is not NULL
 *\li		'expire' is not NULL
 *\li		'mctx' is not NULL
 *\li		'buffer' is not NULL
 *\li		'sigrdata' is not NULL
 *
 *	Returns:
 *\li		#ISC_R_SUCCESS
 *\li		#ISC_R_NOMEMORY
 *\li		#ISC_R_NOSPACE
 *\li		#DNS_R_INVALIDTIME - the expiration is before the inception
 *\li		#DNS_R_KEYUNAUTHORIZED - the key cannot sign this data (either
 *			it is not a zone key or its flags prevent
 *			authentication)
 *\li		DST_R_*
 */

isc_result_t
dns_dnssec_verify(dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		  isc_boolean_t ignoretime, isc_mem_t *mctx,
		  dns_rdata_t *sigrdata);

isc_result_t
dns_dnssec_verify2(dns_name_t *name, dns_rdataset_t *set, dst_key_t *key,
		   isc_boolean_t ignoretime, isc_mem_t *mctx,
		   dns_rdata_t *sigrdata, dns_name_t *wild);
/*%<
 *	Verifies the SIG record covering this rdataset signed by a specific
 *	key.  This does not determine if the key's owner is authorized to
 *	sign this record, as this requires a resolver or database.
 *	If 'ignoretime' is ISC_TRUE, temporal validity will not be checked.
 *
 *	Requires:
 *\li		'name' (the owner name of the record) is a valid name
 *\li		'set' is a valid rdataset
 *\li		'key' is a valid key
 *\li		'mctx' is not NULL
 *\li		'sigrdata' is a valid rdata containing a SIG record
 *\li		'wild' if non-NULL then is a valid and has a buffer.
 *
 *	Returns:
 *\li		#ISC_R_SUCCESS
 *\li		#ISC_R_NOMEMORY
 *\li		#DNS_R_FROMWILDCARD - the signature is valid and is from
 *			a wildcard expansion.  dns_dnssec_verify2() only.
 *			'wild' contains the name of the wildcard if non-NULL.
 *\li		#DNS_R_SIGINVALID - the signature fails to verify
 *\li		#DNS_R_SIGEXPIRED - the signature has expired
 *\li		#DNS_R_SIGFUTURE - the signature's validity period has not begun
 *\li		#DNS_R_KEYUNAUTHORIZED - the key cannot sign this data (either
 *			it is not a zone key or its flags prevent
 *			authentication)
 *\li		DST_R_*
 */

/*@{*/
isc_result_t
dns_dnssec_findzonekeys(dns_db_t *db, dns_dbversion_t *ver, dns_dbnode_t *node,
			dns_name_t *name, isc_mem_t *mctx,
			unsigned int maxkeys, dst_key_t **keys,
			unsigned int *nkeys);
isc_result_t
dns_dnssec_findzonekeys2(dns_db_t *db, dns_dbversion_t *ver,
			 dns_dbnode_t *node, dns_name_t *name,
			 const char *directory, isc_mem_t *mctx,
			 unsigned int maxkeys, dst_key_t **keys,
			 unsigned int *nkeys);
/*%<
 * 	Finds a set of zone keys.
 * 	XXX temporary - this should be handled in dns_zone_t.
 */
/*@}*/

isc_result_t
dns_dnssec_signmessage(dns_message_t *msg, dst_key_t *key);
/*%<
 *	Signs a message with a SIG(0) record.  This is implicitly called by
 *	dns_message_renderend() if msg->sig0key is not NULL.
 *
 *	Requires:
 *\li		'msg' is a valid message
 *\li		'key' is a valid key that can be used for signing
 *
 *	Returns:
 *\li		#ISC_R_SUCCESS
 *\li		#ISC_R_NOMEMORY
 *\li		DST_R_*
 */

isc_result_t
dns_dnssec_verifymessage(isc_buffer_t *source, dns_message_t *msg,
			 dst_key_t *key);
/*%<
 *	Verifies a message signed by a SIG(0) record.  This is not
 *	called implicitly by dns_message_parse().  If dns_message_signer()
 *	is called before dns_dnssec_verifymessage(), it will return
 *	#DNS_R_NOTVERIFIEDYET.  dns_dnssec_verifymessage() will set
 *	the verified_sig0 flag in msg if the verify succeeds, and
 *	the sig0status field otherwise.
 *
 *	Requires:
 *\li		'source' is a valid buffer containing the unparsed message
 *\li		'msg' is a valid message
 *\li		'key' is a valid key
 *
 *	Returns:
 *\li		#ISC_R_SUCCESS
 *\li		#ISC_R_NOMEMORY
 *\li		#ISC_R_NOTFOUND - no SIG(0) was found
 *\li		#DNS_R_SIGINVALID - the SIG record is not well-formed or
 *				   was not generated by the key.
 *\li		DST_R_*
 */

ISC_LANG_ENDDECLS

#endif /* DNS_DNSSEC_H */
