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

/* $Id: types.h,v 1.130.50.3 2009/01/29 22:40:35 jinmei Exp $ */

#ifndef DNS_TYPES_H
#define DNS_TYPES_H 1

/*! \file dns/types.h
 * \brief
 * Including this file gives you type declarations suitable for use in
 * .h files, which lets us avoid circular type reference problems.
 * \brief
 * To actually use a type or get declarations of its methods, you must
 * include the appropriate .h file too.
 */

#include <isc/types.h>

typedef struct dns_acache			dns_acache_t;
typedef struct dns_acacheentry			dns_acacheentry_t;
typedef struct dns_acachestats			dns_acachestats_t;
typedef struct dns_acl 				dns_acl_t;
typedef struct dns_aclelement 			dns_aclelement_t;
typedef struct dns_aclenv			dns_aclenv_t;
typedef struct dns_adb				dns_adb_t;
typedef struct dns_adbaddrinfo			dns_adbaddrinfo_t;
typedef ISC_LIST(dns_adbaddrinfo_t)		dns_adbaddrinfolist_t;
typedef struct dns_adbentry			dns_adbentry_t;
typedef struct dns_adbfind			dns_adbfind_t;
typedef ISC_LIST(dns_adbfind_t)			dns_adbfindlist_t;
typedef struct dns_byaddr			dns_byaddr_t;
typedef struct dns_cache			dns_cache_t;
typedef isc_uint16_t				dns_cert_t;
typedef struct dns_compress			dns_compress_t;
typedef struct dns_db				dns_db_t;
typedef struct dns_dbimplementation		dns_dbimplementation_t;
typedef struct dns_dbiterator			dns_dbiterator_t;
typedef void					dns_dbload_t;
typedef void					dns_dbnode_t;
typedef struct dns_dbtable			dns_dbtable_t;
typedef void					dns_dbversion_t;
typedef struct dns_dlzimplementation		dns_dlzimplementation_t;
typedef struct dns_dlzdb			dns_dlzdb_t;
typedef struct dns_sdlzimplementation		dns_sdlzimplementation_t;
typedef struct dns_decompress			dns_decompress_t;
typedef struct dns_dispatch			dns_dispatch_t;
typedef struct dns_dispatchevent		dns_dispatchevent_t;
typedef struct dns_dispatchlist			dns_dispatchlist_t;
typedef struct dns_dispatchmgr			dns_dispatchmgr_t;
typedef struct dns_dispentry			dns_dispentry_t;
typedef struct dns_dumpctx			dns_dumpctx_t;
typedef struct dns_fetch			dns_fetch_t;
typedef struct dns_fixedname			dns_fixedname_t;
typedef struct dns_forwarders			dns_forwarders_t;
typedef struct dns_fwdtable			dns_fwdtable_t;
typedef struct dns_iptable			dns_iptable_t;
typedef isc_uint32_t				dns_iterations_t;
typedef isc_uint16_t				dns_keyflags_t;
typedef struct dns_keynode			dns_keynode_t;
typedef struct dns_keytable			dns_keytable_t;
typedef isc_uint16_t				dns_keytag_t;
typedef struct dns_loadctx			dns_loadctx_t;
typedef struct dns_loadmgr			dns_loadmgr_t;
typedef struct dns_message			dns_message_t;
typedef isc_uint16_t				dns_messageid_t;
typedef isc_region_t				dns_label_t;
typedef struct dns_lookup			dns_lookup_t;
typedef struct dns_name				dns_name_t;
typedef ISC_LIST(dns_name_t)			dns_namelist_t;
typedef isc_uint16_t				dns_opcode_t;
typedef unsigned char				dns_offsets_t[128];
typedef struct dns_order			dns_order_t;
typedef struct dns_peer				dns_peer_t;
typedef struct dns_peerlist			dns_peerlist_t;
typedef struct dns_portlist			dns_portlist_t;
typedef struct dns_rbt				dns_rbt_t;
typedef isc_uint16_t				dns_rcode_t;
typedef struct dns_rdata			dns_rdata_t;
typedef struct dns_rdatacallbacks		dns_rdatacallbacks_t;
typedef isc_uint16_t				dns_rdataclass_t;
typedef struct dns_rdatalist			dns_rdatalist_t;
typedef struct dns_rdataset			dns_rdataset_t;
typedef ISC_LIST(dns_rdataset_t)		dns_rdatasetlist_t;
typedef struct dns_rdatasetiter			dns_rdatasetiter_t;
typedef isc_uint16_t				dns_rdatatype_t;
typedef struct dns_request			dns_request_t;
typedef struct dns_requestmgr			dns_requestmgr_t;
typedef struct dns_resolver			dns_resolver_t;
typedef struct dns_sdbimplementation		dns_sdbimplementation_t;
typedef isc_uint8_t				dns_secalg_t;
typedef isc_uint8_t				dns_secproto_t;
typedef struct dns_signature			dns_signature_t;
typedef struct dns_ssurule			dns_ssurule_t;
typedef struct dns_ssutable			dns_ssutable_t;
typedef struct dns_stats			dns_stats_t;
typedef isc_uint32_t				dns_rdatastatstype_t;
typedef struct dns_tkeyctx			dns_tkeyctx_t;
typedef isc_uint16_t				dns_trust_t;
typedef struct dns_tsig_keyring			dns_tsig_keyring_t;
typedef struct dns_tsigkey			dns_tsigkey_t;
typedef isc_uint32_t				dns_ttl_t;
typedef struct dns_validator			dns_validator_t;
typedef struct dns_view				dns_view_t;
typedef ISC_LIST(dns_view_t)			dns_viewlist_t;
typedef struct dns_zone				dns_zone_t;
typedef ISC_LIST(dns_zone_t)			dns_zonelist_t;
typedef struct dns_zonemgr			dns_zonemgr_t;
typedef struct dns_zt				dns_zt_t;

/*
 * If we are not using GSSAPI, define the types we use as opaque types here.
 */
#ifndef GSSAPI
typedef struct not_defined_gss_cred_id *gss_cred_id_t;
typedef struct not_defined_gss_ctx *gss_ctx_id_t;
#endif
typedef struct dst_gssapi_signverifyctx dst_gssapi_signverifyctx_t;

typedef enum {
	dns_hash_sha1 = 1
} dns_hash_t;

typedef enum {
	dns_fwdpolicy_none = 0,
	dns_fwdpolicy_first = 1,
	dns_fwdpolicy_only = 2
} dns_fwdpolicy_t;

typedef enum {
	dns_namereln_none = 0,
	dns_namereln_contains = 1,
	dns_namereln_subdomain = 2,
	dns_namereln_equal = 3,
	dns_namereln_commonancestor = 4
} dns_namereln_t;

typedef enum {
	dns_one_answer, dns_many_answers
} dns_transfer_format_t;

typedef enum {
	dns_dbtype_zone = 0, dns_dbtype_cache = 1, dns_dbtype_stub = 3
} dns_dbtype_t;

typedef enum {
	dns_notifytype_no = 0,
	dns_notifytype_yes = 1,
	dns_notifytype_explicit = 2,
	dns_notifytype_masteronly = 3
} dns_notifytype_t;

typedef enum {
	dns_dialuptype_no = 0,
	dns_dialuptype_yes = 1,
	dns_dialuptype_notify = 2,
	dns_dialuptype_notifypassive = 3,
	dns_dialuptype_refresh = 4,
	dns_dialuptype_passive = 5
} dns_dialuptype_t;

typedef enum {
	dns_masterformat_none = 0,
	dns_masterformat_text = 1,
	dns_masterformat_raw = 2
} dns_masterformat_t;

/*
 * These are generated by gen.c.
 */
#include <dns/enumtype.h>	/* Provides dns_rdatatype_t. */
#include <dns/enumclass.h>	/* Provides dns_rdataclass_t. */

/*%
 * rcodes.
 */
enum {
	/*
	 * Standard rcodes.
	 */
	dns_rcode_noerror = 0,
#define dns_rcode_noerror		((dns_rcode_t)dns_rcode_noerror)
	dns_rcode_formerr = 1,
#define dns_rcode_formerr		((dns_rcode_t)dns_rcode_formerr)
	dns_rcode_servfail = 2,
#define dns_rcode_servfail		((dns_rcode_t)dns_rcode_servfail)
	dns_rcode_nxdomain = 3,
#define dns_rcode_nxdomain		((dns_rcode_t)dns_rcode_nxdomain)
	dns_rcode_notimp = 4,
#define dns_rcode_notimp		((dns_rcode_t)dns_rcode_notimp)
	dns_rcode_refused = 5,
#define dns_rcode_refused		((dns_rcode_t)dns_rcode_refused)
	dns_rcode_yxdomain = 6,
#define dns_rcode_yxdomain		((dns_rcode_t)dns_rcode_yxdomain)
	dns_rcode_yxrrset = 7,
#define dns_rcode_yxrrset		((dns_rcode_t)dns_rcode_yxrrset)
	dns_rcode_nxrrset = 8,
#define dns_rcode_nxrrset		((dns_rcode_t)dns_rcode_nxrrset)
	dns_rcode_notauth = 9,
#define dns_rcode_notauth		((dns_rcode_t)dns_rcode_notauth)
	dns_rcode_notzone = 10,
#define dns_rcode_notzone		((dns_rcode_t)dns_rcode_notzone)
	/*
	 * Extended rcodes.
	 */
	dns_rcode_badvers = 16
#define dns_rcode_badvers		((dns_rcode_t)dns_rcode_badvers)
};

/*%
 * TSIG errors.
 */
enum {
	dns_tsigerror_badsig = 16,
	dns_tsigerror_badkey = 17,
	dns_tsigerror_badtime = 18,
	dns_tsigerror_badmode = 19,
	dns_tsigerror_badname = 20,
	dns_tsigerror_badalg = 21,
	dns_tsigerror_badtrunc = 22
};

/*%
 * Opcodes.
 */
enum {
	dns_opcode_query = 0,
#define dns_opcode_query		((dns_opcode_t)dns_opcode_query)
	dns_opcode_iquery = 1,
#define dns_opcode_iquery		((dns_opcode_t)dns_opcode_iquery)
	dns_opcode_status = 2,
#define dns_opcode_status		((dns_opcode_t)dns_opcode_status)
	dns_opcode_notify = 4,
#define dns_opcode_notify		((dns_opcode_t)dns_opcode_notify)
	dns_opcode_update = 5		/* dynamic update */
#define dns_opcode_update		((dns_opcode_t)dns_opcode_update)
};

/*%
 * Trust levels.  Must be kept in sync with trustnames[] in masterdump.c.
 */
enum {
	/* Sentinel value; no data should have this trust level. */
	dns_trust_none = 0,
#define dns_trust_none			((dns_trust_t)dns_trust_none)

	/*% Subject to DNSSEC validation but has not yet been validated */
	dns_trust_pending = 1,
#define dns_trust_pending		((dns_trust_t)dns_trust_pending)

	/*% Received in the additional section of a response. */
	dns_trust_additional = 2,
#define dns_trust_additional		((dns_trust_t)dns_trust_additional)

	/* Received in a referral response. */
	dns_trust_glue = 3,
#define dns_trust_glue			((dns_trust_t)dns_trust_glue)

	/* Answer from a non-authoritative server */
	dns_trust_answer = 4,
#define dns_trust_answer		((dns_trust_t)dns_trust_answer)

	/*  Received in the authority section as part of an
	    authoritative response */
	dns_trust_authauthority = 5,
#define dns_trust_authauthority		((dns_trust_t)dns_trust_authauthority)

	/* Answer from an authoritative server */
	dns_trust_authanswer = 6,
#define dns_trust_authanswer		((dns_trust_t)dns_trust_authanswer)

	/* Successfully DNSSEC validated */
	dns_trust_secure = 7,
#define dns_trust_secure		((dns_trust_t)dns_trust_secure)

	/* This server is authoritative */
	dns_trust_ultimate = 8
#define dns_trust_ultimate		((dns_trust_t)dns_trust_ultimate)
};

/*%
 * Name checking severities.
 */
typedef enum {
	dns_severity_ignore,
	dns_severity_warn,
	dns_severity_fail
} dns_severity_t;

/*
 * Functions.
 */
typedef void
(*dns_dumpdonefunc_t)(void *, isc_result_t);

typedef void
(*dns_loaddonefunc_t)(void *, isc_result_t);

typedef isc_result_t
(*dns_addrdatasetfunc_t)(void *, dns_name_t *, dns_rdataset_t *);

typedef isc_result_t
(*dns_additionaldatafunc_t)(void *, dns_name_t *, dns_rdatatype_t);

typedef isc_result_t
(*dns_digestfunc_t)(void *, isc_region_t *);

typedef void
(*dns_xfrindone_t)(dns_zone_t *, isc_result_t);

typedef void
(*dns_updatecallback_t)(void *, isc_result_t, dns_message_t *);

typedef int
(*dns_rdatasetorderfunc_t)(const dns_rdata_t *, const void *);

typedef isc_boolean_t
(*dns_checkmxfunc_t)(dns_zone_t *, dns_name_t *, dns_name_t *);

typedef isc_boolean_t
(*dns_checksrvfunc_t)(dns_zone_t *, dns_name_t *, dns_name_t *);

typedef isc_boolean_t
(*dns_checknsfunc_t)(dns_zone_t *, dns_name_t *, dns_name_t *,
		     dns_rdataset_t *, dns_rdataset_t *);

typedef isc_boolean_t
(*dns_isselffunc_t)(dns_view_t *, dns_tsigkey_t *, isc_sockaddr_t *,
		    isc_sockaddr_t *, dns_rdataclass_t, void *);

#endif /* DNS_TYPES_H */
