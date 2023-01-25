#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include "ppport.h"

#include "ldns/ldns.h"
#include "ldns/error.h"
#include "ldns/rr.h"
#include "ldns/keys.h"
#include "ldns/dname.h"
#include "ldns/host2str.h"
#include "ldns/rdata.h"
#include "ldns/rbtree.h"
#include "ldns/resolver.h"
#include "ldns/packet.h"
#include "ldns/dnssec.h"

#include "ldns/dnssec_zone.h"
#include "ldns/dnssec_verify.h"
#include "ldns/dnssec_sign.h"
#include "ldns/rr_functions.h"

#if LDNS_REVISION < ((1<<16)|(6<<8)|(17))
  #define LDNS_RDF_TYPE_HIP LDNS_RDF_TYPE_TSIG
#endif

#include "const-c.inc"

typedef ldns_zone *          DNS__LDNS__Zone;
typedef ldns_rr_list *       DNS__LDNS__RRList;
typedef ldns_rr *            DNS__LDNS__RR;
typedef ldns_rr *            DNS__LDNS__RR__Opt;
typedef ldns_rdf *           DNS__LDNS__RData;
typedef ldns_rdf *           DNS__LDNS__RData__Opt;
typedef ldns_dnssec_zone *   DNS__LDNS__DNSSecZone;
typedef ldns_dnssec_rrsets * DNS__LDNS__DNSSecRRSets;
typedef ldns_dnssec_rrs *    DNS__LDNS__DNSSecRRs;
typedef ldns_dnssec_name *   DNS__LDNS__DNSSecName;
typedef ldns_rbtree_t *      DNS__LDNS__RBTree;
typedef ldns_rbnode_t *      DNS__LDNS__RBNode;
typedef ldns_resolver *      DNS__LDNS__Resolver;
typedef ldns_pkt *           DNS__LDNS__Packet;
typedef ldns_key *           DNS__LDNS__Key;
typedef ldns_key_list *      DNS__LDNS__KeyList;
typedef ldns_dnssec_data_chain * DNS__LDNS__DNSSecDataChain;
typedef ldns_dnssec_trust_tree * DNS__LDNS__DNSSecTrustTree;
typedef const char *         Mortal_PV;

typedef ldns_pkt_opcode   LDNS_Pkt_Opcode;
typedef ldns_pkt_rcode    LDNS_Pkt_Rcode;
typedef ldns_pkt_section  LDNS_Pkt_Section;
typedef ldns_pkt_type     LDNS_Pkt_Type;
typedef ldns_rr_type      LDNS_RR_Type;
typedef ldns_rr_class     LDNS_RR_Class;
typedef ldns_rdf_type     LDNS_RDF_Type;
typedef ldns_hash         LDNS_Hash;
typedef ldns_status       LDNS_Status;
typedef ldns_signing_algorithm LDNS_Signing_Algorithm;

/* callback function used by the signing methods */
int sign_policy(ldns_rr *sig, void *n) {
    return *(uint16_t*)n;
}

/* utility methods */
void add_cloned_rrs_to_list(ldns_rr_list * list, ldns_rr_list * add) {
    size_t count;
    size_t i;

    count = ldns_rr_list_rr_count(add);

    for(i = 0; i < count; i++) {
        ldns_rr_list_push_rr(list, ldns_rr_clone(ldns_rr_list_rr(add, i)));
    }
}


#if LDNS_REVISION < ((1<<16)|(6<<8)|(12))
ldns_dnssec_trust_tree *ldns_dnssec_derive_trust_tree_time(
                ldns_dnssec_data_chain *data_chain, 
                ldns_rr *rr, time_t check_time);
ldns_rr_list *ldns_fetch_valid_domain_keys_time(const ldns_resolver * res,
                const ldns_rdf * domain, const ldns_rr_list * keys,
                time_t check_time, ldns_status *status);
ldns_rr_list *ldns_validate_domain_dnskey_time(
                const ldns_resolver *res, const ldns_rdf *domain, 
                const ldns_rr_list *keys, time_t check_time);
ldns_rr_list *ldns_validate_domain_ds_time(
                const ldns_resolver *res, const ldns_rdf *domain, 
                const ldns_rr_list * keys, time_t check_time);
ldns_status ldns_verify_rrsig_keylist_time(
                ldns_rr_list *rrset, ldns_rr *rrsig, 
                const ldns_rr_list *keys, time_t check_time,
                ldns_rr_list *good_keys);
ldns_status ldns_verify_trusted_time(
                ldns_resolver *res, ldns_rr_list *rrset, 
                ldns_rr_list *rrsigs, time_t check_time,
                ldns_rr_list *validating_keys);
ldns_status ldns_verify_rrsig_time(
                ldns_rr_list *rrset, ldns_rr *rrsig, 
                ldns_rr *key, time_t check_time);
ldns_status ldns_verify_time(ldns_rr_list *rrset,
                                    ldns_rr_list *rrsig,
                                    const ldns_rr_list *keys,
                                    time_t check_time,
                                    ldns_rr_list *good_keys);   

ldns_dnssec_trust_tree *ldns_dnssec_derive_trust_tree_time(
                ldns_dnssec_data_chain *data_chain, 
                ldns_rr *rr, time_t check_time) {
    Perl_croak(aTHX_ "function ldns_dnssec_derive_trust_tree_time is not implemented in this version of ldns");
}

ldns_rr_list *ldns_fetch_valid_domain_keys_time(const ldns_resolver * res,
                const ldns_rdf * domain, const ldns_rr_list * keys,
                time_t check_time, ldns_status *status) {
    Perl_croak(aTHX_ "function ldns_fetch_valid_domain_keys_time is not implemented in this version of ldns");
}

ldns_rr_list *ldns_validate_domain_dnskey_time(
                const ldns_resolver *res, const ldns_rdf *domain, 
                const ldns_rr_list *keys, time_t check_time) {
    Perl_croak(aTHX_ "function ldns_validate_domain_dnskey_time is not implemented in this version of ldns");
}

ldns_rr_list *ldns_validate_domain_ds_time(
                const ldns_resolver *res, const ldns_rdf *domain, 
                const ldns_rr_list * keys, time_t check_time) {
    Perl_croak(aTHX_ "function ldns_validate_domain_ds_time is not implemented in this version of ldns");
}

ldns_status ldns_verify_rrsig_keylist_time(
                ldns_rr_list *rrset, ldns_rr *rrsig, 
                const ldns_rr_list *keys, time_t check_time,
                ldns_rr_list *good_keys) {
    Perl_croak(aTHX_ "function ldns_verify_rrsig_keylist_time is not implemented in this version of ldns");
}

ldns_status ldns_verify_trusted_time(
                ldns_resolver *res, ldns_rr_list *rrset, 
                ldns_rr_list *rrsigs, time_t check_time,
                ldns_rr_list *validating_keys) {
    Perl_croak(aTHX_ "function ldns_verify_trusted_time is not implemented in this version of ldns");
}

ldns_status ldns_verify_rrsig_time(
                ldns_rr_list *rrset, ldns_rr *rrsig, 
                ldns_rr *key, time_t check_time) {
    Perl_croak(aTHX_ "function ldns_verify_rrsig_time is not implemented in this version of ldns");
}

ldns_status ldns_verify_time(ldns_rr_list *rrset,
                                    ldns_rr_list *rrsig,
                                    const ldns_rr_list *keys,
                                    time_t check_time,
                                    ldns_rr_list *good_keys) {
    Perl_croak(aTHX_ "function ldns_verify_time is not implemented in this version of ldns");
}

#endif


MODULE = DNS::LDNS           PACKAGE = DNS::LDNS

INCLUDE: const-xs.inc

const char *
ldns_get_errorstr_by_id(s)
	LDNS_Status s
	ALIAS:
	errorstr_by_id = 1

Mortal_PV
ldns_rr_type2str(type)
	LDNS_RR_Type type;
	ALIAS:
	rr_type2str = 1

Mortal_PV
ldns_rr_class2str(class)
	LDNS_RR_Class class;
	ALIAS:
	rr_class2str = 1

Mortal_PV
ldns_pkt_opcode2str(opcode)
	LDNS_Pkt_Opcode opcode;
	ALIAS:
	pkt_opcode2str = 1

Mortal_PV
ldns_pkt_rcode2str(rcode)
	LDNS_Pkt_Rcode rcode;
	ALIAS:
	pkt_rcode2str = 1

LDNS_RR_Type
ldns_get_rr_type_by_name(name)
	char * name;
	ALIAS:
	rr_type_by_name = 1

LDNS_RR_Class
ldns_get_rr_class_by_name(name)
	char * name;
	ALIAS:
	rr_class_by_name = 1

DNS__LDNS__RR
ldns_dnssec_create_nsec(from, to, nsec_type)
	DNS__LDNS__DNSSecName from;
	DNS__LDNS__DNSSecName to;
	LDNS_RR_Type nsec_type;
	ALIAS:
	dnssec_create_nsec = 1

DNS__LDNS__RR
dnssec_create_nsec3(from, to, zone_name, algorithm, flags, iterations, salt)
	DNS__LDNS__DNSSecName from;
	DNS__LDNS__DNSSecName to;
	DNS__LDNS__RData zone_name;
	uint8_t algorithm;
	uint8_t flags;
	uint16_t iterations;
	char * salt;
	CODE:
	RETVAL = ldns_dnssec_create_nsec3(from, to, zone_name, algorithm, flags, iterations, strlen(salt), (uint8_t*)salt);
	OUTPUT:
	RETVAL

DNS__LDNS__RR
ldns_create_nsec(current, next, rrs)
	DNS__LDNS__RData current;
	DNS__LDNS__RData next;
	DNS__LDNS__RRList rrs;
	ALIAS:
	create_nsec = 1

DNS__LDNS__RR
create_nsec3(cur_owner, cur_zone, rrs, algorithm, flags, iterations, salt, emptynonterminal)
	DNS__LDNS__RData cur_owner;
	DNS__LDNS__RData cur_zone;
	DNS__LDNS__RRList rrs;
	uint8_t algorithm;
	uint8_t flags;
	uint16_t iterations;
	char * salt;
	bool emptynonterminal;
	CODE:
	RETVAL = ldns_create_nsec3(cur_owner, cur_zone, rrs, algorithm, 
	    flags, iterations, strlen(salt), (uint8_t*)salt, emptynonterminal);
	OUTPUT:
	RETVAL

LDNS_Signing_Algorithm
ldns_get_signing_algorithm_by_name(name)
	const char * name;
	ALIAS:
	signing_algorithm_by_name = 1

int
ldns_key_algo_supported(algorithm)
	int algorithm;
	ALIAS:
	key_algorithm_supported = 1

DNS__LDNS__RR
ldns_read_anchor_file(filename)
	char * filename;
	ALIAS:
	read_anchor_file = 1

MODULE = DNS::LDNS           PACKAGE = DNS::LDNS::GC

void
ldns_zone_deep_free(zone)
	DNS__LDNS__Zone zone;
	ALIAS:
	_zone_deep_free = 1

void
ldns_rr_list_deep_free(list)
	DNS__LDNS__RRList list;
	ALIAS:
	_rrlist_deep_free = 1

void
ldns_rr_free(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rr_free = 1

void
ldns_rdf_deep_free(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	_rdata_deep_free = 1

void
ldns_dnssec_zone_deep_free(zone)
	DNS__LDNS__DNSSecZone zone;
	ALIAS:
	_dnssec_zone_deep_free = 1

void
ldns_dnssec_name_deep_free(name)
	DNS__LDNS__DNSSecName name;
	ALIAS:
	_dnssec_name_deep_free = 1

void
ldns_resolver_deep_free(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	_resolver_deep_free = 1

void
ldns_pkt_free(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_packet_free = 1

void
ldns_key_deep_free(key)
	DNS__LDNS__Key key;
	ALIAS:
	_key_deep_free = 1

void
ldns_key_list_free(keylist)
	DNS__LDNS__KeyList keylist;
	ALIAS:
	_keylist_free = 1

void
ldns_dnssec_data_chain_deep_free(chain)
	DNS__LDNS__DNSSecDataChain chain;
	ALIAS:
	_dnssec_datachain_deep_free = 1

void
ldns_dnssec_trust_tree_free(tree)
	DNS__LDNS__DNSSecTrustTree tree;
	ALIAS:
	_dnssec_trusttree_free = 1

MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::Zone

PROTOTYPES: ENABLE

DNS__LDNS__Zone
ldns_zone_new()
	ALIAS:
	_new = 1

DNS__LDNS__Zone
_new_from_file(fp, origin, ttl, c, s, line_nr)
	FILE*         fp;
	DNS__LDNS__RData__Opt origin;
	uint32_t      ttl;
	LDNS_RR_Class c;
	LDNS_Status   s;
	int           line_nr;
        PREINIT:
            ldns_zone *z;
        CODE:
        if (ttl == 0) { ttl = 0; }
	RETVAL = NULL;
	s = ldns_zone_new_frm_fp_l(&z, fp, origin, ttl, c, &line_nr);

	if (s == LDNS_STATUS_OK) {
	    RETVAL = z;
	}

        OUTPUT:
        RETVAL
	s
	line_nr

void
print(zone, fp)
	DNS__LDNS__Zone zone;
	FILE* fp;
	CODE:
	ldns_zone_print(fp, zone);

void
canonicalize(zone)
	DNS__LDNS__Zone zone;
	PREINIT:
	    ldns_rr_list *list;
	    size_t count;
	    size_t i;
	CODE:
	list = ldns_zone_rrs(zone);
	count = ldns_rr_list_rr_count(list);

	ldns_rr2canonical(ldns_zone_soa(zone));
	for (i = 0; i < ldns_rr_list_rr_count(ldns_zone_rrs(zone)); i++) {
	    ldns_rr2canonical(ldns_rr_list_rr(ldns_zone_rrs(zone), i));
	}

void
ldns_zone_sort(zone)
	DNS__LDNS__Zone zone;
	ALIAS:
	sort = 1

DNS__LDNS__RR
ldns_zone_soa(zone)
	DNS__LDNS__Zone zone;
	ALIAS:
	_soa = 1

void
ldns_zone_set_soa(zone, soa)
	DNS__LDNS__Zone zone;
	DNS__LDNS__RR soa;
	ALIAS:
	_set_soa = 1

DNS__LDNS__RRList
ldns_zone_rrs(zone)
	DNS__LDNS__Zone zone;
	ALIAS:
	_rrs = 1

void
ldns_zone_set_rrs(zone, list)
	DNS__LDNS__Zone zone;
	DNS__LDNS__RRList list;
	ALIAS:
	_set_rrs = 1

DNS__LDNS__Zone
ldns_zone_sign(zone, keylist)
	DNS__LDNS__Zone zone;
	DNS__LDNS__KeyList keylist;
	ALIAS:
	sign = 1

DNS__LDNS__Zone
sign_nsec3(zone, keylist, algorithm, flags, iterations, salt)
	DNS__LDNS__Zone zone;
	DNS__LDNS__KeyList keylist;
	uint8_t algorithm;
	uint8_t flags;
	uint16_t iterations;
	unsigned char * salt;
	CODE:
	RETVAL = ldns_zone_sign_nsec3(zone, keylist, algorithm, flags, iterations, strlen(salt), (uint8_t*)salt);
	OUTPUT:
	RETVAL


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::RRList

PROTOTYPES: ENABLE

DNS__LDNS__RRList
ldns_rr_list_new()
	ALIAS:
	_new = 1

DNS__LDNS__RRList
_new_hosts_from_file(fp, line_nr)
	FILE * fp;
	int line_nr;
	CODE:
	RETVAL = ldns_get_rr_list_hosts_frm_fp_l(fp, &line_nr);
	OUTPUT:
	RETVAL

DNS__LDNS__RRList
ldns_rr_list_clone(list)
	DNS__LDNS__RRList list;
	ALIAS:
	clone = 1

void
print(list, fp)
	DNS__LDNS__RRList list;
	FILE* fp;
	CODE:
	ldns_rr_list_print(fp, list);

Mortal_PV
ldns_rr_list2str(list)
	DNS__LDNS__RRList list;
	ALIAS:
	to_string = 1

DNS__LDNS__RR
ldns_rr_list_rr(list, i)
	DNS__LDNS__RRList list;
	size_t i;
	ALIAS:
	_rr = 1

DNS__LDNS__RR
ldns_rr_list_pop_rr(list)
	DNS__LDNS__RRList list;
	ALIAS:
	pop = 1

bool
ldns_rr_list_push_rr(list, rr)
	DNS__LDNS__RRList list;
	DNS__LDNS__RR rr;
	ALIAS:
	_push = 1

size_t
ldns_rr_list_rr_count(list)
	DNS__LDNS__RRList list;
	ALIAS:
	rr_count = 1

int
ldns_rr_list_compare(list, otherlist)
	DNS__LDNS__RRList list;
	DNS__LDNS__RRList otherlist;
	ALIAS:
	compare = 1

DNS__LDNS__RRList
ldns_rr_list_subtype_by_rdf(list, rdf, pos)
	DNS__LDNS__RRList list;
	DNS__LDNS__RData rdf;
	size_t pos;
	ALIAS:
	subtype_by_rdata = 1

DNS__LDNS__RRList
ldns_rr_list_pop_rrset(list)
	DNS__LDNS__RRList list;
	ALIAS:
	pop_rrset = 1

bool
ldns_is_rrset(list)
	DNS__LDNS__RRList list;
	ALIAS:
	is_rrset = 1

bool
ldns_rr_list_contains_rr(list, rr)
	DNS__LDNS__RRList list;
	DNS__LDNS__RR rr;
	ALIAS:
	contains_rr = 1

DNS__LDNS__RRList
ldns_rr_list_pop_rr_list(list, count)
	DNS__LDNS__RRList list;
	size_t count;
	ALIAS:
	pop_list = 1

bool
_push_list(list, otherlist)
	DNS__LDNS__RRList list;
	DNS__LDNS__RRList otherlist;
	PREINIT:
	    bool ret;
	CODE:
	ret = ldns_rr_list_push_rr_list(list, otherlist);
	if (ret) {
	    ldns_rr_list_free(otherlist);
	}
	OUTPUT:
	RETVAL

LDNS_Status
_verify_rrsig_keylist(rrset, rrsig, keys, good_keys)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RR rrsig;
	DNS__LDNS__RRList keys;
	DNS__LDNS__RRList good_keys;
	PREINIT:
	    DNS__LDNS__RRList gk;
	CODE:
	gk = ldns_rr_list_new();
	RETVAL = ldns_verify_rrsig_keylist(rrset, rrsig, keys, good_keys);
	add_cloned_rrs_to_list(good_keys, gk);
	ldns_rr_list_free(gk);
	OUTPUT:
	RETVAL

LDNS_Status
_verify_rrsig_keylist_time(rrset, rrsig, keys, check_time, good_keys)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RR rrsig;
	DNS__LDNS__RRList keys;
	time_t check_time;
	DNS__LDNS__RRList good_keys;
	PREINIT:
	    DNS__LDNS__RRList gk;
	CODE:
	gk = ldns_rr_list_new();
	RETVAL = ldns_verify_rrsig_keylist_time(
	    rrset, rrsig, keys, check_time, good_keys);
	add_cloned_rrs_to_list(good_keys, gk);
	ldns_rr_list_free(gk);
	OUTPUT:
	RETVAL

LDNS_Status
_verify_rrsig_keylist_notime(rrset, rrsig, keys, good_keys)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RR rrsig;
	DNS__LDNS__RRList keys;
	DNS__LDNS__RRList good_keys;
	PREINIT:
	    DNS__LDNS__RRList gk;
	CODE:
	gk = ldns_rr_list_new();
	RETVAL = ldns_verify_rrsig_keylist_notime(rrset, rrsig, keys, NULL);
	add_cloned_rrs_to_list(good_keys, gk);
	ldns_rr_list_free(gk);
	OUTPUT:
	RETVAL

LDNS_Status
ldns_verify_rrsig(rrset, rrsig, key)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RR rrsig;
	DNS__LDNS__RR key;
	ALIAS:
	_verify_rrsig = 1

LDNS_Status
ldns_verify_rrsig_time(rrset, rrsig, key, check_time)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RR rrsig;
	DNS__LDNS__RR key;
	time_t check_time;
	ALIAS:
	_verify_rrsig_time = 1

LDNS_Status
_verify(rrset, rrsig, keys, good_keys)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RRList rrsig;
	DNS__LDNS__RRList keys;
	DNS__LDNS__RRList good_keys;
	PREINIT:
	    DNS__LDNS__RRList gk;
	CODE:
	gk = ldns_rr_list_new();
	RETVAL = ldns_verify(rrset, rrsig, keys, gk);
	add_cloned_rrs_to_list(good_keys, gk);
	ldns_rr_list_free(gk);
	OUTPUT:
	RETVAL

LDNS_Status
_verify_time(rrset, rrsig, keys, check_time, good_keys)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RRList rrsig;
	DNS__LDNS__RRList keys;
	time_t check_time;
	DNS__LDNS__RRList good_keys;
	PREINIT:
	    DNS__LDNS__RRList gk;
	CODE:
	gk = ldns_rr_list_new();
	RETVAL = ldns_verify_time(rrset, rrsig, keys, check_time, gk);
	add_cloned_rrs_to_list(good_keys, gk);
	ldns_rr_list_free(gk);
	OUTPUT:
	RETVAL

LDNS_Status
_verify_notime(rrset, rrsig, keys, good_keys)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RRList rrsig;
	DNS__LDNS__RRList keys;
	DNS__LDNS__RRList good_keys;
	PREINIT:
	    DNS__LDNS__RRList gk;
	CODE:
	gk = ldns_rr_list_new();
	RETVAL = ldns_verify_notime(rrset, rrsig, keys, gk);
	add_cloned_rrs_to_list(good_keys, gk);
	ldns_rr_list_free(gk);
	OUTPUT:
	RETVAL

DNS__LDNS__RR
ldns_create_empty_rrsig(rrset, current_key)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__Key current_key;
	ALIAS:
	create_empty_rrsig = 1

DNS__LDNS__RRList
ldns_sign_public(rrset, keys)
	DNS__LDNS__RRList rrset;
	DNS__LDNS__KeyList keys;
	ALIAS:
	sign_public = 1

void
ldns_rr_list_sort(list)
	DNS__LDNS__RRList list;
	ALIAS:
	sort = 1

void
ldns_rr_list_sort_nsec3(list)
	DNS__LDNS__RRList list;
	ALIAS:
	sort_nsec3 = 1

void
ldns_rr_list2canonical(list)
	DNS__LDNS__RRList list;
	ALIAS:
	canonicalize = 1

DNS__LDNS__RR
ldns_dnssec_get_dnskey_for_rrsig(rr, rrlist)
	DNS__LDNS__RR rr;
	DNS__LDNS__RRList rrlist;
	ALIAS:
	_get_dnskey_for_rrsig = 1

DNS__LDNS__RR
ldns_dnssec_get_rrsig_for_name_and_type(name, type, rrsigs)
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	DNS__LDNS__RRList rrsigs;
	ALIAS:
	_get_rrsig_for_name_and_type = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::RR

PROTOTYPES: ENABLE

DNS__LDNS__RR
ldns_rr_new()
	ALIAS:
	_new = 1

DNS__LDNS__RR
ldns_rr_new_frm_type(t)
	LDNS_RR_Type t;
	ALIAS:
	_new_from_type = 1

DNS__LDNS__RR
_new_from_str(str, default_ttl, origin, prev, s)
	const char* str;
	uint32_t default_ttl;
	DNS__LDNS__RData__Opt origin;
	DNS__LDNS__RData__Opt prev;
	LDNS_Status s;
	PREINIT:
	    DNS__LDNS__RR rr = NULL;
	    ldns_rdf *pclone = NULL;
	CODE:

	if (prev != NULL) {
	    pclone = ldns_rdf_clone(prev);
        }

	s = ldns_rr_new_frm_str(&rr, str, default_ttl, origin, &prev);
	if (prev != NULL) {
	    prev = pclone;
	}

	if (s == LDNS_STATUS_OK) {
	    RETVAL = rr;
	}
	OUTPUT:
	RETVAL
	s
	prev

DNS__LDNS__RR
_new_from_file(fp, default_ttl, origin, prev, s, line_nr)
	FILE*         fp;
	uint32_t      default_ttl;
	DNS__LDNS__RData__Opt origin;
	DNS__LDNS__RData__Opt prev;
	LDNS_Status   s;
	int           line_nr;
        PREINIT:
            ldns_rr *rr;
	    ldns_rdf *oclone = NULL;
	    ldns_rdf *pclone = NULL;
        CODE:

	/* Must clone origin and prev because new_frm_fp_l may change 
 	   them and may not (we do not know for certain). The perl layer 
	   will take care of freeing the old structs. */
	if (origin != NULL) {
	    oclone = ldns_rdf_clone(origin);
        }
	if (prev != NULL) {
	    pclone = ldns_rdf_clone(prev);
        }

	RETVAL = NULL;
	s = ldns_rr_new_frm_fp_l(&rr, fp, &default_ttl, &oclone, &pclone, 
	    &line_nr);

	/* Replace the input origin with our new clone. The perl layer will
	   take care of freeing it later. */
	if (origin != NULL) {
	    origin = oclone;
	}
	if (prev != NULL) {
	    prev = pclone;
	}

	if (s == LDNS_STATUS_OK) {
	    RETVAL = rr;
	}

        OUTPUT:
        RETVAL
	s
	line_nr
        default_ttl
        origin
	prev

DNS__LDNS__RR
ldns_rr_clone(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	clone = 1

void
ldns_rr_set_owner(rr, owner)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData owner;
	ALIAS:
	_set_owner = 1

void
ldns_rr_set_ttl(rr, ttl)
	DNS__LDNS__RR rr;
	uint32_t ttl;
	ALIAS:
	set_ttl = 1

void
ldns_rr_set_type(rr, type)
	DNS__LDNS__RR rr;
	LDNS_RR_Type type;
	ALIAS:
	set_type = 1

void
ldns_rr_set_class(rr, class)
	DNS__LDNS__RR rr;
	LDNS_RR_Class class;
	ALIAS:
	set_class = 1

void
print(rr, fp)
	DNS__LDNS__RR rr;
	FILE* fp;
	CODE:
	ldns_rr_print(fp, rr);

Mortal_PV
ldns_rr2str(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	to_string = 1

int
ldns_rr_compare(rr, otherrr)
	DNS__LDNS__RR rr;
	DNS__LDNS__RR otherrr;
	ALIAS:
	compare = 1

int
ldns_rr_compare_no_rdata(rr, otherrr)
	DNS__LDNS__RR rr;
	DNS__LDNS__RR otherrr;
	ALIAS:
	compare_no_rdata = 1

int
ldns_rr_compare_ds(rr, otherrr)
	DNS__LDNS__RR rr;
	DNS__LDNS__RR otherrr;
	ALIAS:
	compare_ds = 1

int
compare_dname(rr, otherrr)
	DNS__LDNS__RR rr;
	DNS__LDNS__RR otherrr;
	CODE:
	RETVAL = ldns_dname_compare(
	    ldns_rr_owner(rr), ldns_rr_owner(otherrr));
	OUTPUT:
	RETVAL

DNS__LDNS__RData
ldns_rr_owner(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_owner = 1

size_t
ldns_rr_rd_count(rr);
	DNS__LDNS__RR rr;
	ALIAS:
	rd_count = 1

DNS__LDNS__RData
ldns_rr_rdf(rr, i)
	DNS__LDNS__RR rr;
	size_t i;
	ALIAS:
	_rdata = 1

DNS__LDNS__RData
ldns_rr_set_rdf(rr, rdf, i)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	size_t i;
	ALIAS:
	_set_rdata = 1

uint32_t
ldns_rr_ttl(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	ttl = 1

LDNS_RR_Class
ldns_rr_get_class(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	class = 1

LDNS_RR_Type
ldns_rr_get_type(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	type = 1

DNS__LDNS__RData
ldns_rr_pop_rdf(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	pop_rdata = 1

bool
ldns_rr_push_rdf(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_push_rdata = 1

DNS__LDNS__RData
ldns_rr_rrsig_typecovered(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_typecovered = 1

bool
ldns_rr_rrsig_set_typecovered(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_typecovered = 1	 

DNS__LDNS__RData
ldns_rr_rrsig_algorithm(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_algorithm = 1

bool
ldns_rr_rrsig_set_algorithm(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_algorithm = 1

DNS__LDNS__RData
ldns_rr_rrsig_expiration(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_expiration = 1

bool
ldns_rr_rrsig_set_expiration(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_expiration = 1

DNS__LDNS__RData
ldns_rr_rrsig_inception(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_inception = 1

bool
ldns_rr_rrsig_set_inception(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_inception = 1

DNS__LDNS__RData
ldns_rr_rrsig_keytag(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_keytag = 1

bool
ldns_rr_rrsig_set_keytag(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_keytag = 1

DNS__LDNS__RData
ldns_rr_rrsig_sig(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_sig = 1

bool
ldns_rr_rrsig_set_sig(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_sig = 1

DNS__LDNS__RData
ldns_rr_rrsig_labels(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_labels = 1

bool
ldns_rr_rrsig_set_labels(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_labels = 1

DNS__LDNS__RData
ldns_rr_rrsig_origttl(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_origttl = 1

bool
ldns_rr_rrsig_set_origttl(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_origttl = 1

DNS__LDNS__RData
ldns_rr_rrsig_signame(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_rrsig_signame = 1

bool
ldns_rr_rrsig_set_signame(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_rrsig_set_signame = 1

DNS__LDNS__RData
ldns_rr_dnskey_algorithm(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_dnskey_algorithm = 1

bool
ldns_rr_dnskey_set_algorithm(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_dnskey_set_algorithm = 1

DNS__LDNS__RData
ldns_rr_dnskey_flags(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_dnskey_flags = 1

bool
ldns_rr_dnskey_set_flags(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_dnskey_set_flags = 1

DNS__LDNS__RData
ldns_rr_dnskey_protocol(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_dnskey_protocol = 1

bool
ldns_rr_dnskey_set_protocol(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_dnskey_set_protocol = 1

DNS__LDNS__RData
ldns_rr_dnskey_key(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	_dnskey_key = 1

bool
ldns_rr_dnskey_set_key(rr, rdf)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData rdf;
	ALIAS:
	_dnskey_set_key = 1

size_t
ldns_rr_dnskey_key_size(rr)
	DNS__LDNS__RR rr;
	ALIAS:
	dnskey_key_size = 1

uint16_t
ldns_calc_keytag(key)
	DNS__LDNS__RR key;
	ALIAS:
	calc_keytag = 1

DNS__LDNS__RData
ldns_nsec3_hash_name_frm_nsec3(rr, name)
	DNS__LDNS__RR rr;
	DNS__LDNS__RData name;
	ALIAS:
	_hash_name_from_nsec3 = 1

DNS__LDNS__RData
_nsec3_hash_name(name, algorithm, iterations, salt)
	DNS__LDNS__RData name;
	uint8_t algorithm;
	uint16_t iterations;
	char * salt;
	CODE:
	RETVAL = ldns_nsec3_hash_name(name, algorithm, iterations, 
	    strlen(salt), (uint8_t *)salt);
	OUTPUT:
	RETVAL

LDNS_Status
ldns_dnssec_verify_denial(rr, nsecs, rrsigs)
	DNS__LDNS__RR rr;
	DNS__LDNS__RRList nsecs;
	DNS__LDNS__RRList rrsigs;
	ALIAS:
	_verify_denial = 1

LDNS_Status
ldns_dnssec_verify_denial_nsec3(rr, nsecs, rrsigs, packet_rcode, packet_qtype, packet_nodata)
	DNS__LDNS__RR rr;
	DNS__LDNS__RRList nsecs;
	DNS__LDNS__RRList rrsigs;
	LDNS_Pkt_Rcode packet_rcode;
	LDNS_RR_Type packet_qtype;
	signed char packet_nodata;
	ALIAS:
	_verify_denial_nsec3 = 1

DNS__LDNS__RR
_verify_denial_nsec3_match(rr, nsecs, rrsigs, packet_rcode, packet_qtype, packet_nodata, status)
	DNS__LDNS__RR rr;
	DNS__LDNS__RRList nsecs;
	DNS__LDNS__RRList rrsigs;
	LDNS_Pkt_Rcode packet_rcode;
	LDNS_RR_Type packet_qtype;
	signed char packet_nodata;
	LDNS_Status status;
	PREINIT:
	    ldns_rr ** match;
	CODE:
	RETVAL = NULL;
	status = ldns_dnssec_verify_denial_nsec3_match(rr, nsecs, rrsigs, 
	    packet_rcode, packet_qtype, packet_nodata, match);
	if (status == LDNS_STATUS_OK) {
	    RETVAL = *match;
	}
	OUTPUT:
	status
	RETVAL

void
nsec3_add_param_rdfs(rr, algorithm, flags, iterations, salt)
	DNS__LDNS__RR rr;
	uint8_t algorithm;
	uint8_t flags;
	uint16_t iterations;
	char * salt;
	CODE:
	ldns_nsec3_add_param_rdfs(rr, algorithm, flags, iterations, strlen(salt), (uint8_t*)salt);

uint8_t
ldns_nsec3_algorithm(nsec3)
	DNS__LDNS__RR nsec3;
	ALIAS:
	nsec3_algorithm = 1

uint8_t
ldns_nsec3_flags(nsec3)
	DNS__LDNS__RR nsec3;
	ALIAS:
	nsec3_flags = 1

bool
ldns_nsec3_optout(nsec3)
	DNS__LDNS__RR nsec3;
	ALIAS:
	nsec3_optout = 1

uint16_t
ldns_nsec3_iterations(nsec3)
	DNS__LDNS__RR nsec3;
	ALIAS:
	nsec3_iterations = 1

DNS__LDNS__RData
ldns_nsec3_next_owner(nsec3)
	DNS__LDNS__RR nsec3;
	ALIAS:
	_nsec3_next_owner = 1

DNS__LDNS__RData
ldns_nsec3_bitmap(nsec3)
	DNS__LDNS__RR nsec3;
	ALIAS:
	_nsec3_bitmap = 1

DNS__LDNS__RData
ldns_nsec3_salt(nsec3)
	DNS__LDNS__RR nsec3;
	ALIAS:
	_nsec3_salt = 1

DNS__LDNS__RR
ldns_key_rr2ds(key, hash)
        DNS__LDNS__RR key;
	LDNS_Hash hash;
	ALIAS:
	key_to_ds = 1

bool
ldns_rr_is_question(rr)
        DNS__LDNS__RR rr;
	ALIAS:
	is_question = 1

uint8_t
ldns_rr_label_count(rr)
        DNS__LDNS__RR rr;
	ALIAS:
	label_count = 1

MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::RData

PROTOTYPES: ENABLE

DNS__LDNS__RData
ldns_rdf_new_frm_str(type, str)
	LDNS_RDF_Type type;
	const char *str;
	ALIAS:
	_new = 1

DNS__LDNS__RData
ldns_rdf_clone(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	clone = 1

Mortal_PV
ldns_rdf2str(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	to_string = 1

void
print(rdf, fp)
	DNS__LDNS__RData rdf;
	FILE* fp;
	CODE:
	ldns_rdf_print(fp, rdf);

LDNS_RDF_Type
ldns_rdf_get_type(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	type = 1

void
ldns_rdf_set_type(rdf, type)
	DNS__LDNS__RData rdf;
	LDNS_RDF_Type type
	ALIAS:
	set_type = 1

int
ldns_rdf_compare(rd1, rd2)
	DNS__LDNS__RData rd1;
	DNS__LDNS__RData rd2;
	ALIAS:
	compare = 1

DNS__LDNS__RData
ldns_rdf_address_reverse(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	address_reverse = 1

uint8_t
ldns_dname_label_count(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	label_count = 1

DNS__LDNS__RData
ldns_dname_label(rdf, labelpos)
	DNS__LDNS__RData rdf;
	uint8_t labelpos;
	ALIAS:
	label = 1

int
ldns_dname_is_wildcard(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	is_wildcard = 1

int
ldns_dname_match_wildcard(rdf, wildcard)
	DNS__LDNS__RData rdf;
	DNS__LDNS__RData wildcard;
	ALIAS:
	matches_wildcard = 1

signed char
ldns_dname_is_subdomain(rdf, parent)
	DNS__LDNS__RData rdf;
	DNS__LDNS__RData parent;
	ALIAS:
	is_subdomain = 1

DNS__LDNS__RData
ldns_dname_left_chop(rdf)
	DNS__LDNS__RData rdf
	ALIAS:
	left_chop = 1

LDNS_Status
ldns_dname_cat(rdata, otherrd)
	DNS__LDNS__RData rdata;
	DNS__LDNS__RData otherrd;
	ALIAS:
	_cat = 1

int
ldns_dname_compare(dname, otherdname)
	DNS__LDNS__RData dname;
	DNS__LDNS__RData otherdname;
	ALIAS:
	compare = 1

LDNS_RR_Type
ldns_rdf2rr_type(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	to_rr_type = 1

DNS__LDNS__RData
ldns_dname_reverse(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	dname_reverse = 1

void
ldns_dname2canonical(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	dname2canonical = 1

time_t
ldns_rdf2native_time_t(rdf)
	DNS__LDNS__RData rdf;
	ALIAS:
	to_unix_time = 1
	2native_time_t = 2


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::DNSSecZone

PROTOTYPES: ENABLE

DNS__LDNS__DNSSecZone
ldns_dnssec_zone_new()
	ALIAS:
	_new = 1

DNS__LDNS__DNSSecZone
_new_from_file(fp, origin, ttl, c, s, line_nr)
	FILE*         fp;
	DNS__LDNS__RData__Opt origin;
	uint32_t      ttl;
	LDNS_RR_Class c;
	LDNS_Status   s;
	int           line_nr;
        PREINIT:
            ldns_dnssec_zone *z;
        CODE:
	RETVAL = NULL;
#if LDNS_REVISION < ((1<<16)|(6<<8)|(13))
	Perl_croak(aTHX_ "function ldns_dnssec_zone_new_frm_fp_l is not implemented in this version of ldns");
#else
	s = ldns_dnssec_zone_new_frm_fp_l(&z, fp, origin, ttl, c, &line_nr);
#endif

	if (s == LDNS_STATUS_OK) {
	    RETVAL = z;
	}

        OUTPUT:
        RETVAL
	s
	line_nr

LDNS_Status
create_from_zone(dnssec_zone, zone)
	DNS__LDNS__DNSSecZone dnssec_zone;
	DNS__LDNS__Zone zone;
	PREINIT:
	    size_t i;
            ldns_rr *cur_rr;
            ldns_status status;
	    ldns_rr_list *failed_nsec3s;
	    ldns_rr_list *failed_nsec3_rrsigs;
            ldns_status result = LDNS_STATUS_OK;
	CODE:
	failed_nsec3s = ldns_rr_list_new();
        failed_nsec3_rrsigs = ldns_rr_list_new();

        status = ldns_dnssec_zone_add_rr(dnssec_zone, 
	             ldns_rr_clone(ldns_zone_soa(zone)));
	if (result == LDNS_STATUS_OK) {
	    result = status;
        }

        for (i = 0; i < ldns_rr_list_rr_count(ldns_zone_rrs(zone)); i++) {
            cur_rr = ldns_rr_list_rr(ldns_zone_rrs(zone), i);
            status = ldns_dnssec_zone_add_rr(dnssec_zone, 
                         ldns_rr_clone(cur_rr));
            if (status != LDNS_STATUS_OK) {
                if (LDNS_STATUS_DNSSEC_NSEC3_ORIGINAL_NOT_FOUND == status) {
                    if (ldns_rr_get_type(cur_rr) == LDNS_RR_TYPE_RRSIG
                        && ldns_rdf2rr_type(ldns_rr_rrsig_typecovered(cur_rr))
                        == LDNS_RR_TYPE_NSEC3) {
                        ldns_rr_list_push_rr(failed_nsec3_rrsigs, cur_rr);
                    } else {
                        ldns_rr_list_push_rr(failed_nsec3s, cur_rr);
                    }
                }
		if (result == LDNS_STATUS_OK) {
		    result = status;
                }
            }
        }

        if (ldns_rr_list_rr_count(failed_nsec3s) > 0) {
            (void) ldns_dnssec_zone_add_empty_nonterminals(dnssec_zone);
            for (i = 0; i < ldns_rr_list_rr_count(failed_nsec3s); i++) {
                cur_rr = ldns_rr_list_rr(failed_nsec3s, i);
                status = ldns_dnssec_zone_add_rr(dnssec_zone, 
                             ldns_rr_clone(cur_rr));
		if (result == LDNS_STATUS_OK) {
		    result = status;
                }
            }
            for (i = 0; i < ldns_rr_list_rr_count(failed_nsec3_rrsigs); i++) {
                cur_rr = ldns_rr_list_rr(failed_nsec3_rrsigs, i);
                status = ldns_dnssec_zone_add_rr(dnssec_zone, 
                             ldns_rr_clone(cur_rr));
		if (result == LDNS_STATUS_OK) {
		    result = status;
                }
            }
        }

        ldns_rr_list_free(failed_nsec3_rrsigs);
        ldns_rr_list_free(failed_nsec3s);
        RETVAL = result;
	OUTPUT:
	RETVAL

void
print(zone, fp)
	DNS__LDNS__DNSSecZone zone;
	FILE* fp;
	CODE:
	ldns_dnssec_zone_print(fp, zone);

LDNS_Status
ldns_dnssec_zone_add_rr(zone, rr)
	DNS__LDNS__DNSSecZone zone;
	DNS__LDNS__RR	 rr;
	ALIAS:
	_add_rr = 1

LDNS_Status
ldns_dnssec_zone_add_empty_nonterminals(zone)
	DNS__LDNS__DNSSecZone zone;
	ALIAS:
	_add_empty_nonterminals = 1

LDNS_Status
ldns_dnssec_zone_mark_glue(zone)
	DNS__LDNS__DNSSecZone zone;
	ALIAS:
	_mark_glue = 1

DNS__LDNS__DNSSecName
_soa(zone)
	DNS__LDNS__DNSSecZone zone;
	CODE:
	RETVAL = zone->soa;
	OUTPUT:
	RETVAL

DNS__LDNS__RBTree
_names(zone)
	DNS__LDNS__DNSSecZone zone;
	CODE:
	RETVAL = zone->names;
	OUTPUT:
	RETVAL

DNS__LDNS__DNSSecRRSets
ldns_dnssec_zone_find_rrset(zone, rdf, type)
	DNS__LDNS__DNSSecZone zone;
	DNS__LDNS__RData rdf;
	LDNS_RR_Type type;
	ALIAS:
	_find_rrset = 1

LDNS_Status
_sign(zone, key_list, policy, flags)
	DNS__LDNS__DNSSecZone zone;
	DNS__LDNS__KeyList key_list;
	uint16_t policy;
	int flags;
	PREINIT:
	    ldns_rr_list * new_rrs;
	CODE:
	new_rrs = ldns_rr_list_new();
	RETVAL = ldns_dnssec_zone_sign_flg(zone, new_rrs, key_list, 
	    sign_policy, (void*)&policy, flags);
	ldns_rr_list_free(new_rrs);
	OUTPUT:
	RETVAL

LDNS_Status
_sign_nsec3(zone, key_list, policy, algorithm, flags, iterations, salt, signflags)
	DNS__LDNS__DNSSecZone zone;
	DNS__LDNS__KeyList key_list;
	uint16_t policy;
	uint8_t algorithm;
	uint8_t flags;
	uint16_t iterations;
	char * salt;
	int signflags;
	PREINIT:
	     ldns_rr_list * new_rrs;
	CODE:
	new_rrs = ldns_rr_list_new();
	RETVAL = ldns_dnssec_zone_sign_nsec3_flg(zone, new_rrs, key_list, 
	    sign_policy, (void*)&policy, algorithm, flags, iterations, 
	    strlen(salt), (uint8_t*)salt, signflags);
	ldns_rr_list_free(new_rrs);
	OUTPUT:
	RETVAL

LDNS_Status
create_nsecs(zone)
	DNS__LDNS__DNSSecZone zone;
	PREINIT:
	    ldns_rr_list * new_rrs;
	CODE:
	new_rrs = ldns_rr_list_new();
	RETVAL = ldns_dnssec_zone_create_nsecs(zone, new_rrs);
	ldns_rr_list_free(new_rrs);
	OUTPUT:
	RETVAL

LDNS_Status
create_nsec3s(zone, algorithm, flags, iterations, salt)
	DNS__LDNS__DNSSecZone zone;
	uint8_t algorithm;
	uint8_t flags;
	uint8_t iterations;
	char * salt;
	PREINIT:
	    ldns_rr_list * new_rrs;
	CODE:
	new_rrs = ldns_rr_list_new();
	RETVAL = ldns_dnssec_zone_create_nsec3s(zone, new_rrs, algorithm,
	    flags, iterations, strlen(salt), (uint8_t*)salt);
	ldns_rr_list_free(new_rrs);
	OUTPUT:
	RETVAL

LDNS_Status
create_rrsigs(zone, key_list, policy, flags)
	DNS__LDNS__DNSSecZone zone;
	DNS__LDNS__KeyList key_list;
	uint16_t policy;
	int flags;
	PREINIT:
	     ldns_rr_list * new_rrs;
	CODE:
	new_rrs = ldns_rr_list_new();
	RETVAL = ldns_dnssec_zone_create_rrsigs_flg(zone, new_rrs, key_list, 
	    sign_policy, (void*)&policy, flags);
	ldns_rr_list_free(new_rrs);
	OUTPUT:
	RETVAL


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::DNSSecRRSets

DNS__LDNS__DNSSecRRs
_rrs(rrsets)
	DNS__LDNS__DNSSecRRSets rrsets;
	CODE:
	RETVAL = rrsets->rrs;
	OUTPUT:
	RETVAL

DNS__LDNS__DNSSecRRs
_signatures(rrsets)
	DNS__LDNS__DNSSecRRSets rrsets;
	CODE:
	RETVAL = rrsets->signatures;
	OUTPUT:
	RETVAL

bool
ldns_dnssec_rrsets_contains_type(rrsets, type)
	DNS__LDNS__DNSSecRRSets rrsets;
	LDNS_RR_Type type;
	ALIAS:
	contains_type = 1

LDNS_RR_Type
ldns_dnssec_rrsets_type(rrsets)
	DNS__LDNS__DNSSecRRSets rrsets;
	ALIAS:
	type = 1

LDNS_Status
ldns_dnssec_rrsets_set_type(rrsets, type)
	DNS__LDNS__DNSSecRRSets rrsets;
	LDNS_RR_Type type;
	ALIAS:
	_set_type = 1

DNS__LDNS__DNSSecRRSets
_next(rrsets)
	DNS__LDNS__DNSSecRRSets rrsets;
	CODE:
	RETVAL = rrsets->next;
	OUTPUT:
	RETVAL

LDNS_Status
ldns_dnssec_rrsets_add_rr(rrsets, rr)
	DNS__LDNS__DNSSecRRSets rrsets;
	DNS__LDNS__RR rr;
	ALIAS:
	_add_rr = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::DNSSecRRs

DNS__LDNS__DNSSecRRs
ldns_dnssec_rrs_new()
	ALIAS:
	_new = 1

DNS__LDNS__RR
_rr(rrs)
	DNS__LDNS__DNSSecRRs rrs;
	CODE:
	RETVAL = rrs->rr;
	OUTPUT:
	RETVAL

DNS__LDNS__DNSSecRRs
_next(rrs)
	DNS__LDNS__DNSSecRRs rrs;
	CODE:
	RETVAL = rrs->next;
	OUTPUT:
	RETVAL

LDNS_Status
ldns_dnssec_rrs_add_rr(rrs, rr)
	DNS__LDNS__DNSSecRRs rrs;
	DNS__LDNS__RR rr;
	ALIAS:
	_add_rr = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::DNSSecName

DNS__LDNS__DNSSecName
ldns_dnssec_name_new()
	ALIAS:
	_new = 1

DNS__LDNS__RData
ldns_dnssec_name_name(name)
	DNS__LDNS__DNSSecName name;
	ALIAS:
	_name = 1

bool
ldns_dnssec_name_is_glue(name)
	DNS__LDNS__DNSSecName name;
	ALIAS:
	is_glue = 1

DNS__LDNS__DNSSecRRSets
_rrsets(name)
	DNS__LDNS__DNSSecName name;
	CODE:
	RETVAL = name->rrsets;
	OUTPUT:
	RETVAL

DNS__LDNS__RR
_nsec(name)
	DNS__LDNS__DNSSecName name;
	CODE:
	RETVAL = name->nsec;
	OUTPUT:
	RETVAL

DNS__LDNS__RData
_hashed_name(name)
	DNS__LDNS__DNSSecName name;
	CODE:
	RETVAL = name->hashed_name;
	OUTPUT:
	RETVAL

DNS__LDNS__DNSSecRRs
_nsec_signatures(name)
	DNS__LDNS__DNSSecName name;
	CODE:
	RETVAL = name->nsec_signatures;
	OUTPUT:
	RETVAL

void
ldns_dnssec_name_set_name(name, dname)
	DNS__LDNS__DNSSecName name;
	DNS__LDNS__RData dname;
	ALIAS:
	_set_name = 1

void
ldns_dnssec_name_set_nsec(name, nsec)
	DNS__LDNS__DNSSecName name;
	DNS__LDNS__RR nsec;
	ALIAS:
	_set_nsec = 1

int
ldns_dnssec_name_cmp(a, b)
	DNS__LDNS__DNSSecName a;
	DNS__LDNS__DNSSecName b;
	ALIAS:
	compare = 1

LDNS_Status
ldns_dnssec_name_add_rr(name, rr)
	DNS__LDNS__DNSSecName name;
	DNS__LDNS__RR rr;
	ALIAS:
	_add_rr = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::RBTree

DNS__LDNS__RBNode
ldns_rbtree_first(tree)
	DNS__LDNS__RBTree tree;
	ALIAS:
	_first = 1

DNS__LDNS__RBNode
ldns_rbtree_last(tree)
	DNS__LDNS__RBTree tree;
	ALIAS:
	_last = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::RBNode

DNS__LDNS__RBNode
ldns_rbtree_next(node)
	DNS__LDNS__RBNode node;
	ALIAS:
	_next = 1

DNS__LDNS__RBNode
ldns_rbtree_previous(node)
	DNS__LDNS__RBNode node;
	ALIAS:
	_previous = 1

DNS__LDNS__RBNode
ldns_dnssec_name_node_next_nonglue(node)
	DNS__LDNS__RBNode node;
	ALIAS:
	_next_nonglue = 1

bool
is_null(node)
	DNS__LDNS__RBNode node;
	CODE:
	RETVAL = (node == LDNS_RBTREE_NULL);
	OUTPUT:
	RETVAL

DNS__LDNS__DNSSecName
_name(node)
	DNS__LDNS__RBNode node;
	CODE:
	RETVAL = (ldns_dnssec_name*)node->data;
	OUTPUT:
	RETVAL


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::Resolver

DNS__LDNS__Resolver
_new_from_file(fp, s)
	FILE* fp;
	LDNS_Status s;
        PREINIT:
            ldns_resolver *r;
	CODE:
	RETVAL = NULL;
	s = ldns_resolver_new_frm_fp(&r, fp);
	if (s == LDNS_STATUS_OK) {
	    RETVAL = r;
	}
	OUTPUT:
	RETVAL
	s

DNS__LDNS__Resolver
ldns_resolver_new()
	ALIAS:
	_new = 1

bool
ldns_resolver_dnssec(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	dnssec = 1

void
ldns_resolver_set_dnssec(resolver, d)
	DNS__LDNS__Resolver resolver;
	bool d;
	ALIAS:
	set_dnssec = 1

bool
ldns_resolver_dnssec_cd(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	dnssec_cd = 1

void
ldns_resolver_set_dnssec_cd(resolver, d)
	DNS__LDNS__Resolver resolver;
	bool d;
	ALIAS:
	set_dnssec_cd = 1

uint16_t
ldns_resolver_port(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	port = 1

void
ldns_resolver_set_port(resolver, port)
	DNS__LDNS__Resolver resolver;
	uint16_t port;
	ALIAS:
	set_port = 1

bool
ldns_resolver_recursive(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	recursive = 1

void
ldns_resolver_set_recursive(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_recursive = 1

bool
ldns_resolver_debug(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	debug = 1

void
ldns_resolver_set_debug(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_debug = 1

uint8_t
ldns_resolver_retry(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	retry = 1

void
ldns_resolver_set_retry(resolver, re)
	DNS__LDNS__Resolver resolver;
	uint8_t re;
	ALIAS:
	set_retry = 1

uint8_t
ldns_resolver_retrans(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	retrans = 1

void
ldns_resolver_set_retrans(resolver, re)
	DNS__LDNS__Resolver resolver;
	uint8_t re;
	ALIAS:
	set_retrans = 1

bool
ldns_resolver_fallback(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	fallback = 1

void
ldns_resolver_set_fallback(resolver, f)
	DNS__LDNS__Resolver resolver;
	bool f;
	ALIAS:
	set_fallback = 1

uint8_t
ldns_resolver_ip6(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	ip6 = 1

void
ldns_resolver_set_ip6(resolver, i)
	DNS__LDNS__Resolver resolver;
	uint8_t i;
	ALIAS:
	set_ip6 = 1

uint16_t
ldns_resolver_edns_udp_size(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	edns_udp_size = 1

void
ldns_resolver_set_edns_udp_size(resolver, s)
	DNS__LDNS__Resolver resolver;
	uint16_t s;
	ALIAS:
	set_edns_udp_size = 1

bool
ldns_resolver_usevc(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	usevc = 1

void
ldns_resolver_set_usevc(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_usevc = 1

bool
ldns_resolver_fail(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	fail = 1

void
ldns_resolver_set_fail(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_fail = 1

bool
ldns_resolver_defnames(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	defnames = 1

void
ldns_resolver_set_defnames(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_defnames = 1

bool
ldns_resolver_dnsrch(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	dnsrch = 1

void
ldns_resolver_set_dnsrch(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_dnsrch = 1

bool
ldns_resolver_igntc(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	igntc = 1

void
ldns_resolver_set_igntc(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_igntc = 1

bool
ldns_resolver_random(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	random = 1

void
ldns_resolver_set_random(resolver, b)
	DNS__LDNS__Resolver resolver;
	bool b;
	ALIAS:
	set_random = 1

bool
ldns_resolver_trusted_key(resolver, keys, trusted_key)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RRList keys;
	DNS__LDNS__RRList trusted_key;
	ALIAS:
	trusted_key = 1

DNS__LDNS__RRList
ldns_resolver_dnssec_anchors(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	_dnssec_anchors = 1

void
ldns_resolver_set_dnssec_anchors(resolver, list)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RRList list;
	ALIAS:
	_set_dnssec_anchors = 1

void
ldns_resolver_push_dnssec_anchor(resolver, rr)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RR rr;
	ALIAS:
	_push_dnssec_anchor = 1

DNS__LDNS__RData
ldns_resolver_domain(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	_domain = 1

void
ldns_resolver_set_domain(resolver, rd)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData rd;
	ALIAS:
	_set_domain = 1

AV *
_nameservers(resolver)
	DNS__LDNS__Resolver resolver;
	PREINIT:
	    ldns_rdf** list;
	    AV * result;
	    int i;
	    SV * elem;
	CODE:
	result = (AV *)sv_2mortal((SV *)newAV());
	list = ldns_resolver_nameservers(resolver);

	/* FIXME: Make a typemap for this ? */	
	for (i = 0; i < ldns_resolver_nameserver_count(resolver); i++) {
	    elem = newSVpv(0, 0);
	    sv_setref_pv(elem, "LDNS::RData", list[i]);
	    av_push(result, elem);
	}
	RETVAL = result;
	OUTPUT:
	RETVAL

size_t
ldns_resolver_nameserver_count(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	nameserver_count = 1

LDNS_Status
ldns_resolver_push_nameserver(resolver, n)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData n;
	ALIAS:
	_push_nameserver = 1

DNS__LDNS__RData
ldns_resolver_pop_nameserver(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	_pop_nameserver = 1

void
ldns_resolver_nameservers_randomize(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	nameservers_randomize = 1

const char*
ldns_resolver_tsig_keyname(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	tsig_keyname = 1

void
ldns_resolver_set_tsig_keyname(resolver, tsig_keyname)
	DNS__LDNS__Resolver resolver;
	char* tsig_keyname;
	ALIAS:
	set_tsig_keyname = 1

const char*
ldns_resolver_tsig_algorithm(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	tsig_algorithm = 1

void
ldns_resolver_set_tsig_algorithm(resolver, tsig_algorithm)
	DNS__LDNS__Resolver resolver;
	char* tsig_algorithm;
	ALIAS:
	set_tsig_algorithm = 1

const char*
ldns_resolver_tsig_keydata(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	tsig_keydata = 1

void
ldns_resolver_set_tsig_keydata(resolver, tsig_keydata)
	DNS__LDNS__Resolver resolver;
	char* tsig_keydata;
	ALIAS:
	set_tsig_keydata = 1

size_t
ldns_resolver_searchlist_count(resolver)
	DNS__LDNS__Resolver resolver;
	ALIAS:
	searchlist_count = 1

void
ldns_resolver_push_searchlist(resolver, rd)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData rd;
	ALIAS:
	_push_searchlist = 1

AV *
_searchlist(resolver)
	DNS__LDNS__Resolver resolver;
	PREINIT:
	    ldns_rdf** list;
	    AV * result;
	    int i;
	    SV * elem;
	CODE:
	result = (AV *)sv_2mortal((SV *)newAV());
	list = ldns_resolver_searchlist(resolver);

	/* FIXME: Make a typemap for this ? */	
	for (i = 0; i < ldns_resolver_searchlist_count(resolver); i++) {
	    elem = newSVpv(0, 0);
	    sv_setref_pv(elem, "LDNS::RData", list[i]);
	    av_push(result, elem);
	}
	RETVAL = result;
	OUTPUT:
	RETVAL

size_t
ldns_resolver_nameserver_rtt(resolver, pos)
	DNS__LDNS__Resolver resolver;
	size_t pos;
	ALIAS:
	nameserver_rtt = 1

void
ldns_resolver_set_nameserver_rtt(resolver, pos, val)
	DNS__LDNS__Resolver resolver;
	size_t pos;
	size_t val;
	ALIAS:
	set_nameserver_rtt = 1

AV *
_timeout(resolver)
	DNS__LDNS__Resolver resolver;
	PREINIT:
	    struct timeval t;
	    AV * result;
	CODE:
	t = ldns_resolver_timeout(resolver);
	result = (AV *)sv_2mortal((SV *)newAV());
	av_push(result, newSVuv(t.tv_sec));
	av_push(result, newSVuv(t.tv_usec));
	RETVAL = result;
	OUTPUT:
	RETVAL

void
set_timeout(resolver, sec, usec)
	DNS__LDNS__Resolver resolver;
	uint32_t sec;
	uint32_t usec;
	PREINIT:
	    struct timeval t;
	CODE:
	t.tv_sec = sec;
	t.tv_usec = usec;
	ldns_resolver_set_timeout(resolver, t);

void
_set_rtt(resolver, rtt)
	DNS__LDNS__Resolver resolver;
	AV * rtt;
	PREINIT:
	    size_t *buff;
	    int i;
	    SV** elem;
	CODE:
	buff = malloc(sizeof(size_t)*(av_len(rtt)+1));
	for (i = 0; i <= av_len(rtt); i++) {
	    elem = av_fetch(rtt, i, 0);
	    buff[i] = SvUV(*elem);
	}
	ldns_resolver_set_rtt(resolver, buff);

AV *
_rtt(resolver)
	DNS__LDNS__Resolver resolver;
	PREINIT:
	    int i;
	    size_t *rtt;
	    AV * result;
	CODE:
	result = (AV *)sv_2mortal((SV *)newAV());
	rtt = ldns_resolver_rtt(resolver);

	for (i = 0; i < ldns_resolver_nameserver_count(resolver); i++) {
	    av_push(result, newSVuv(rtt[i]));
	}
	RETVAL = result;
	OUTPUT:
	RETVAL

DNS__LDNS__RRList
ldns_validate_domain_ds(resolver, domain, keys)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData domain;
	DNS__LDNS__RRList keys;
	ALIAS:
	validate_domain_ds = 1

DNS__LDNS__RRList
ldns_validate_domain_ds_time(resolver, domain, keys, check_time)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData domain;
	DNS__LDNS__RRList keys;
	time_t check_time;
	ALIAS:
	validate_domain_ds_time = 1

DNS__LDNS__RRList
ldns_validate_domain_dnskey(resolver, domain, keys)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData domain;
	DNS__LDNS__RRList keys;
	ALIAS:
	validate_domain_dnskey = 1

DNS__LDNS__RRList
ldns_validate_domain_dnskey_time(resolver, domain, keys, check_time)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData domain;
	DNS__LDNS__RRList keys;
	time_t check_time;
	ALIAS:
	validate_domain_dnskey_time = 1

LDNS_Status
ldns_verify_trusted(resolver, rrset, rrsigs, validating_keys)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RRList rrsigs;
	DNS__LDNS__RRList validating_keys;
	ALIAS:
	_verify_trusted = 1

LDNS_Status
ldns_verify_trusted_time(resolver, rrset, rrsigs, check_time, validating_keys)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RRList rrset;
	DNS__LDNS__RRList rrsigs;
	time_t check_time;
	DNS__LDNS__RRList validating_keys;
	ALIAS:
	_verify_trusted_time = 1

DNS__LDNS__RRList
_fetch_valid_domain_keys(resolver, domain, keys, s)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData domain;
	DNS__LDNS__RRList keys;
	LDNS_Status s;
        PREINIT:
            DNS__LDNS__RRList trusted;
	    DNS__LDNS__RRList ret;
	    size_t i;
	CODE:
	RETVAL = NULL;
	trusted = ldns_fetch_valid_domain_keys(resolver, domain, keys, &s);
	if (s == LDNS_STATUS_OK) {
	    RETVAL = ldns_rr_list_clone(trusted);
	    ldns_rr_list_free(trusted);
	}
	OUTPUT:
	RETVAL
	s

DNS__LDNS__RRList
_fetch_valid_domain_keys_time(resolver, domain, keys, check_time, s)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData domain;
	DNS__LDNS__RRList keys;
	time_t check_time;
	LDNS_Status s;
        PREINIT:
            DNS__LDNS__RRList trusted;
	    DNS__LDNS__RRList ret;
	    size_t i;
	CODE:
	RETVAL = NULL;
	trusted = ldns_fetch_valid_domain_keys_time(
	    resolver, domain, keys, check_time, &s);
	if (s == LDNS_STATUS_OK) {
	    RETVAL = ldns_rr_list_clone(trusted);
	    ldns_rr_list_free(trusted);
	}
	OUTPUT:
	RETVAL
	s

DNS__LDNS__Packet
ldns_resolver_query(resolver, name, type, class, flags)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	LDNS_RR_Class class;
	uint16_t flags;
	ALIAS:
	query = 1

DNS__LDNS__Packet
_send(resolver, name, type, class, flags, s)
        DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	LDNS_RR_Class class;
	uint16_t flags;
	LDNS_Status s;
	PREINIT:
	    DNS__LDNS__Packet packet;
	CODE:
	s = ldns_resolver_send(&packet, resolver, name, type, class, flags);
	if (s == LDNS_STATUS_OK) {
	    RETVAL = packet;
	}
	OUTPUT:
	RETVAL
	s

DNS__LDNS__Packet
_send_pkt(resolver, packet, s)
        DNS__LDNS__Resolver resolver;
	DNS__LDNS__Packet packet;
	LDNS_Status s;
	PREINIT:
	    DNS__LDNS__Packet answer;
	CODE:
	s = ldns_resolver_send_pkt(&answer, resolver, packet);
	if (s == LDNS_STATUS_OK) {
	    RETVAL = answer;
	}
	OUTPUT:
	RETVAL
	s

DNS__LDNS__Packet
_prepare_query_pkt(resolver, name, type, class, flags, s)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	LDNS_RR_Class class;
	uint16_t flags;
	LDNS_Status s;
	PREINIT:
	    DNS__LDNS__Packet packet;
	CODE:
	s = ldns_resolver_prepare_query_pkt(&packet, resolver, name, type, class, flags);
	if (s == LDNS_STATUS_OK) {
	    RETVAL = packet;
	}
	OUTPUT:
	RETVAL
	s

DNS__LDNS__Packet
ldns_resolver_search(resolver, name, type, class, flags)
	DNS__LDNS__Resolver resolver;
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	LDNS_RR_Class class;
	uint16_t flags;
	ALIAS:
	search = 1

DNS__LDNS__DNSSecDataChain
build_data_chain(res, qflags, data_set, pkt, orig_rr)
	DNS__LDNS__Resolver res;
	uint16_t qflags;
	DNS__LDNS__RRList data_set;
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RR__Opt orig_rr;
	CODE:
	RETVAL = ldns_dnssec_build_data_chain(res, qflags, data_set, pkt, orig_rr);
	OUTPUT:
	RETVAL

DNS__LDNS__RRList
ldns_get_rr_list_addr_by_name(res, name, class, flags)
	DNS__LDNS__Resolver res;
	DNS__LDNS__RData name;
	LDNS_RR_Class class;
	uint16_t flags;
	ALIAS:
	get_rr_list_addr_by_name = 1

DNS__LDNS__RRList
ldns_get_rr_list_name_by_addr(res, addr, class, flags)
	DNS__LDNS__Resolver res;
	DNS__LDNS__RData addr;
	LDNS_RR_Class class;
	uint16_t flags;
	ALIAS:
	get_rr_list_addr_by_addr = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::Packet

Mortal_PV
ldns_pkt2str(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	to_string = 1

DNS__LDNS__RRList
ldns_pkt_question(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_question = 1

void
ldns_pkt_set_question(pkt, l)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RRList l;
	ALIAS:
	_set_question = 1

DNS__LDNS__RRList
ldns_pkt_answer(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_answer = 1

void
ldns_pkt_set_answer(pkt, l)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RRList l;
	ALIAS:
	_set_answer = 1

DNS__LDNS__RRList
ldns_pkt_authority(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_authority = 1

void
ldns_pkt_set_authority(pkt, l)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RRList l;
	ALIAS:
	_set_authority = 1

DNS__LDNS__RRList
ldns_pkt_additional(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_additional = 1

void
ldns_pkt_set_additional(pkt, l)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RRList l;
	ALIAS:
	_set_additional = 1

DNS__LDNS__RRList
ldns_pkt_all(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	all = 1

DNS__LDNS__RRList
ldns_pkt_all_noquestion(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	all_noquestion = 1

signed char
ldns_pkt_qr(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	qr = 1

void
ldns_pkt_set_qr(pkt, b)
	DNS__LDNS__Packet pkt;
	signed char b;
	ALIAS:
	set_qr = 1

signed char
ldns_pkt_aa(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	aa = 1

void
ldns_pkt_set_aa(pkt, b)
	DNS__LDNS__Packet pkt;
	signed char b;
	ALIAS:
	set_aa = 1

signed char
ldns_pkt_tc(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	tc = 1

void
ldns_pkt_set_tc(pkt, b)
	DNS__LDNS__Packet pkt;
	signed char b;
	ALIAS:
	set_tc = 1

signed char
ldns_pkt_rd(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	rd = 1

void
ldns_pkt_set_rd(pkt, b)
	DNS__LDNS__Packet pkt;
	signed char b;
	ALIAS:
	set_rd = 1

bool
ldns_pkt_cd(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	cd = 1

void
ldns_pkt_set_cd(pkt, b)
	DNS__LDNS__Packet pkt;
	signed char b;
	ALIAS:
	set_cd = 1

signed char
ldns_pkt_ra(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	ra = 1

void
ldns_pkt_set_ra(pkt, b)
	DNS__LDNS__Packet pkt;
	signed char b;
	ALIAS:
	set_ra = 1

signed char
ldns_pkt_ad(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	ad = 1

void
ldns_pkt_set_ad(pkt, b)
	DNS__LDNS__Packet pkt;
	signed char b;
	ALIAS:
	set_ad = 1

uint16_t
ldns_pkt_id(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	id = 1

void
ldns_pkt_set_id(pkt, id)
	DNS__LDNS__Packet pkt;
	uint16_t id;
	ALIAS:
	set_id = 1

void
ldns_pkt_set_random_id(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	set_random_id = 1

uint16_t
ldns_pkt_qdcount(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	qdcount = 1

uint16_t
ldns_pkt_ancount(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	ancount = 1

uint16_t
ldns_pkt_nscount(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	nscount = 1

uint16_t
ldns_pkt_arcount(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	arcount = 1

LDNS_Pkt_Opcode
ldns_pkt_get_opcode(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	opcode = 1

void
ldns_pkt_set_opcode(pkt, c)
	DNS__LDNS__Packet pkt;
	LDNS_Pkt_Opcode c;
	ALIAS:
	set_opcode = 1

uint8_t
ldns_pkt_get_rcode(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	rcode = 1

void
ldns_pkt_set_rcode(pkt, r)
	DNS__LDNS__Packet pkt;
	uint8_t r;
	ALIAS:
	set_rcode = 1

size_t
ldns_pkt_size(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	size = 1

uint32_t
ldns_pkt_querytime(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	querytime = 1

void
ldns_pkt_set_querytime(pkt, t)
	DNS__LDNS__Packet pkt;
	uint32_t t;
	ALIAS:
	set_querytime = 1

DNS__LDNS__RData
ldns_pkt_answerfrom(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_answerfrom = 1

AV *
_timestamp(pkt)
	DNS__LDNS__Packet pkt;
	PREINIT:
	    struct timeval t;
	    AV * result;
	CODE:
	t = ldns_pkt_timestamp(pkt);
	result = (AV *)sv_2mortal((SV *)newAV());
	av_push(result, newSVuv(t.tv_sec));
	av_push(result, newSVuv(t.tv_usec));
	RETVAL = result;
	OUTPUT:
	RETVAL

void
set_timestamp(pkt, sec, usec)
	DNS__LDNS__Packet pkt;
	uint32_t sec;
	uint32_t usec;
	PREINIT:
	    struct timeval t;
	CODE:
	t.tv_sec = sec;
	t.tv_usec = usec;
	ldns_pkt_set_timestamp(pkt, t);

void
ldns_pkt_set_answerfrom(pkt, a)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RData a;
	ALIAS:
	_set_answerfrom = 1

bool
ldns_pkt_set_flags(pkt, f)
	DNS__LDNS__Packet pkt;
	uint16_t f;
	ALIAS:
	set_flags = 1

DNS__LDNS__RRList
ldns_pkt_rr_list_by_name(pkt, name, sec)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RData name;
	LDNS_Pkt_Section sec;
	ALIAS:
	rr_list_by_name = 1

DNS__LDNS__RRList
ldns_pkt_rr_list_by_type(pkt, type, sec)
	DNS__LDNS__Packet pkt;
	LDNS_RR_Type type;
	LDNS_Pkt_Section sec;
	ALIAS:
	rr_list_by_type = 1

DNS__LDNS__RRList
ldns_pkt_rr_list_by_name_and_type(pkt, name, type, sec)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	LDNS_Pkt_Section sec;
	ALIAS:
	rr_list_by_name_and_type = 1

bool
ldns_pkt_rr(pkt, sec, rr)
	DNS__LDNS__Packet pkt;
	LDNS_Pkt_Section sec;
	DNS__LDNS__RR rr;
	ALIAS:
	rr = 1

bool
ldns_pkt_push_rr(pkt, sec, rr)
	DNS__LDNS__Packet pkt;
	LDNS_Pkt_Section sec;
	DNS__LDNS__RR rr;
	ALIAS:
	_push_rr = 1

bool
ldns_pkt_safe_push_rr(pkt, sec, rr)
	DNS__LDNS__Packet pkt;
	LDNS_Pkt_Section sec;
	DNS__LDNS__RR rr;
	ALIAS:
	_safe_push_rr = 1

uint16_t
ldns_pkt_section_count(pkt, sec)
	DNS__LDNS__Packet pkt;
	LDNS_Pkt_Section sec;
	ALIAS:
	section_count = 1

signed char
ldns_pkt_empty(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	empty = 1

DNS__LDNS__RR
ldns_pkt_tsig(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_tsig = 1

void
ldns_pkt_set_tsig(pkt, rr)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RR rr;
	ALIAS:
	_set_tsig = 1

DNS__LDNS__Packet
ldns_pkt_clone(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	clone = 1

LDNS_Pkt_Type
ldns_pkt_reply_type(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	reply_type = 1

DNS__LDNS__Packet
ldns_pkt_new()
	ALIAS:
	_new = 1

DNS__LDNS__Packet
ldns_pkt_query_new(name, type, class, flags)
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	LDNS_RR_Class class;
	uint16_t flags;
	ALIAS:
	_query_new = 1

DNS__LDNS__RRList
ldns_dnssec_pkt_get_rrsigs_for_name_and_type(pkt, name, type)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RData name;
	LDNS_RR_Type type;
	ALIAS:
	get_rrsigs_for_name_and_type = 1

DNS__LDNS__RRList
ldns_dnssec_pkt_get_rrsigs_for_type(pkt, type)
	DNS__LDNS__Packet pkt;
	LDNS_RR_Type type;
	ALIAS:
	get_rrsigs_for_type = 1

uint16_t
ldns_pkt_edns_udp_size(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	edns_udp_size = 1

void
ldns_pkt_set_edns_udp_size(pkt, s)
	DNS__LDNS__Packet pkt;
	uint16_t s;
	ALIAS:
	set_edns_udp_size = 1

uint8_t
ldns_pkt_edns_extended_rcode(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	edns_extended_rcode = 1

void
ldns_pkt_set_edns_extended_rcode(pkt, c)
	DNS__LDNS__Packet pkt;
	uint8_t c;
	ALIAS:
	set_edns_extended_rcode = 1

uint8_t
ldns_pkt_edns_version(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	edns_version = 1

void
ldns_pkt_set_edns_version(pkt, v)
	DNS__LDNS__Packet pkt;
	uint8_t v;
	ALIAS:
	set_edns_version = 1

uint16_t
ldns_pkt_edns_z(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	edns_z = 1

void
ldns_pkt_set_edns_z(pkt, z)
	DNS__LDNS__Packet pkt;
	uint16_t z;
	ALIAS:
	set_edns_z = 1

signed char
ldns_pkt_edns_do(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	edns_do = 1

DNS__LDNS__RData
ldns_pkt_edns_data(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	_edns_data = 1

void
ldns_pkt_set_edns_data(pkt, data)
	DNS__LDNS__Packet pkt;
	DNS__LDNS__RData data;
	ALIAS:
	_set_edns_data = 1

void
ldns_pkt_set_edns_do(pkt, val)
	DNS__LDNS__Packet pkt;
	signed char val;
	ALIAS:
	set_edns_do = 1

bool
ldns_pkt_edns(pkt)
	DNS__LDNS__Packet pkt;
	ALIAS:
	edns = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::Key

DNS__LDNS__Key
_new_from_file(fp, line_nr, s)
	FILE*         fp;
	int           line_nr;
	LDNS_Status   s;
        PREINIT:
            ldns_key *key;
        CODE:
	RETVAL = NULL;
	s = ldns_key_new_frm_fp_l(&key, fp, &line_nr);

	if (s == LDNS_STATUS_OK) {
	    RETVAL = key;
	}
        OUTPUT:
        RETVAL
	s
	line_nr

DNS__LDNS__Key
ldns_key_new()
	ALIAS:
	_new = 1

void
print(key, fp)
	DNS__LDNS__Key key;
	FILE* fp;
	CODE:
	ldns_key_print(fp, key);

Mortal_PV
ldns_key2str(key)
	DNS__LDNS__Key key;
	ALIAS:
	to_string = 1

void
ldns_key_set_algorithm(key, algorithm)
	DNS__LDNS__Key key;
	LDNS_Signing_Algorithm algorithm;
	ALIAS:
	set_algorithm = 1

LDNS_Signing_Algorithm
ldns_key_algorithm(key)
	DNS__LDNS__Key key;
	ALIAS:
	algorithm = 1

void
ldns_key_set_flags(key, flags)
	DNS__LDNS__Key key;
	uint16_t flags;
	ALIAS:
	set_flags = 1

uint16_t
ldns_key_flags(key)
	DNS__LDNS__Key key;
	ALIAS:
	flags = 1

void
ldns_key_set_hmac_key(key, hmac)
	DNS__LDNS__Key key;
	unsigned char* hmac;
	ALIAS:
	set_hmac_key = 1

unsigned char *
ldns_key_hmac_key(key)
	DNS__LDNS__Key key;
	ALIAS:
	hmac_key = 1

void
ldns_key_set_hmac_size(key, size)
	DNS__LDNS__Key key;
	size_t size;
	ALIAS:
	set_hmac_size = 1

size_t
ldns_key_hmac_size(key)
	DNS__LDNS__Key key;
	ALIAS:
	hmac_size = 1

void
ldns_key_set_origttl(key, t)
	DNS__LDNS__Key key;
	uint32_t t;
	ALIAS:
	set_origttl = 1

uint32_t
ldns_key_origttl(key)
	DNS__LDNS__Key key;
	ALIAS:
	origttl = 1

void
ldns_key_set_inception(key, i)
	DNS__LDNS__Key key;
	uint32_t i;
	ALIAS:
	set_inception = 1

uint32_t
ldns_key_inception(key)
	DNS__LDNS__Key key;
	ALIAS:
	inception = 1

void
ldns_key_set_expiration(key, e)
	DNS__LDNS__Key key;
	uint32_t e;
	ALIAS:
	set_expiration = 1

uint32_t
ldns_key_expiration(key)
	DNS__LDNS__Key key;
	ALIAS:
	expiration = 1

void
ldns_key_set_pubkey_owner(key, r)
	DNS__LDNS__Key key;
	DNS__LDNS__RData r;
	ALIAS:
	_set_pubkey_owner = 1

DNS__LDNS__RData
ldns_key_pubkey_owner(key)
	DNS__LDNS__Key key;
	ALIAS:
	_pubkey_owner = 1

void
ldns_key_set_keytag(key, tag)
	DNS__LDNS__Key key;
	uint16_t tag;
	ALIAS:
	set_keytag = 1

uint16_t
ldns_key_keytag(key)
	DNS__LDNS__Key key;
	ALIAS:
	keytag = 1

void
ldns_key_set_use(key, v)
	DNS__LDNS__Key key;
	signed char v;
	ALIAS:
	set_use = 1

signed char
ldns_key_use(key)
	DNS__LDNS__Key key;
	ALIAS:
	use = 1

char *
ldns_key_get_file_base_name(key)
	DNS__LDNS__Key key;
	ALIAS:
	get_file_base_name = 1

DNS__LDNS__RR
ldns_key2rr(key)
	DNS__LDNS__Key key;
	ALIAS:
	to_rr = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::KeyList

DNS__LDNS__KeyList
ldns_key_list_new()
	ALIAS:
	_new = 1

void
ldns_key_list_set_use(keys, v)
	DNS__LDNS__KeyList keys;
	bool v;
	ALIAS:
	set_use = 1

DNS__LDNS__Key
ldns_key_list_pop_key(keylist)
	DNS__LDNS__KeyList keylist;
	ALIAS:
	pop = 1

void
ldns_key_list_push_key(keylist, key)
	DNS__LDNS__KeyList keylist;
	DNS__LDNS__Key key;
	ALIAS:
	_push = 1

size_t
ldns_key_list_key_count(keylist)
	DNS__LDNS__KeyList keylist;
	ALIAS:
	count = 1

DNS__LDNS__Key
ldns_key_list_key(keylist, nr)
	DNS__LDNS__KeyList keylist;
	size_t nr;
	ALIAS:
	_key = 1


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::DNSSecDataChain

DNS__LDNS__DNSSecDataChain
ldns_dnssec_data_chain_new()
	ALIAS:
	_new = 1

void
print(chain, fp)
	DNS__LDNS__DNSSecDataChain chain;
	FILE* fp;
	CODE:
	ldns_dnssec_data_chain_print(fp, chain);

DNS__LDNS__DNSSecTrustTree
ldns_dnssec_derive_trust_tree(chain, rr)
	DNS__LDNS__DNSSecDataChain chain;
	DNS__LDNS__RR rr;
	ALIAS:
	_derive_trust_tree = 1

DNS__LDNS__DNSSecTrustTree
ldns_dnssec_derive_trust_tree_time(chain, rr, check_time)
	DNS__LDNS__DNSSecDataChain chain;
	DNS__LDNS__RR rr;
	time_t check_time;
	ALIAS:
	_derive_trust_tree_time = 1

DNS__LDNS__RRList
_rrset(chain)
	DNS__LDNS__DNSSecDataChain chain;
	CODE:
	RETVAL = chain->rrset;
	OUTPUT:
	RETVAL

DNS__LDNS__RRList
_signatures(chain)
	DNS__LDNS__DNSSecDataChain chain;
	CODE:
	RETVAL = chain->signatures;
	OUTPUT:
	RETVAL

LDNS_RR_Type
parent_type(chain)
	DNS__LDNS__DNSSecDataChain chain;
	CODE:
	RETVAL = chain->parent_type;
	OUTPUT:
	RETVAL

DNS__LDNS__DNSSecDataChain
_parent(chain)
	DNS__LDNS__DNSSecDataChain chain;
	CODE:
	RETVAL = chain->parent;
	OUTPUT:
	RETVAL

LDNS_Pkt_Rcode
packet_rcode(chain)
	DNS__LDNS__DNSSecDataChain chain;
	CODE:
	RETVAL = chain->packet_rcode;
	OUTPUT:
	RETVAL

LDNS_RR_Type
packet_qtype(chain)
	DNS__LDNS__DNSSecDataChain chain;
	CODE:
	RETVAL = chain->packet_qtype;
	OUTPUT:
	RETVAL

signed char
packet_nodata(chain)
	DNS__LDNS__DNSSecDataChain chain;
	CODE:
	RETVAL = chain->packet_nodata;
	OUTPUT:
	RETVAL


MODULE = DNS::LDNS		PACKAGE = DNS::LDNS::DNSSecTrustTree

DNS__LDNS__DNSSecTrustTree
ldns_dnssec_trust_tree_new()
	ALIAS:
	_new = 1

void
print(tree, fp, tabs, extended)
	DNS__LDNS__DNSSecTrustTree tree;
	FILE* fp;
	size_t tabs;
	bool extended;
	CODE:
	ldns_dnssec_trust_tree_print(fp, tree, tabs, extended);

size_t
ldns_dnssec_trust_tree_depth(tree)
	DNS__LDNS__DNSSecTrustTree tree;
	ALIAS:
	depth = 1

LDNS_Status
ldns_dnssec_trust_tree_add_parent(tree, parent, signature, parent_status)
	DNS__LDNS__DNSSecTrustTree tree;
	DNS__LDNS__DNSSecTrustTree parent;
	DNS__LDNS__RR signature;
	LDNS_Status parent_status;
	ALIAS:
	_add_parent = 1

LDNS_Status
ldns_dnssec_trust_tree_contains_keys(tree, trusted_keys)
	DNS__LDNS__DNSSecTrustTree tree;
	DNS__LDNS__RRList trusted_keys;
	ALIAS:
	_contains_keys = 1

DNS__LDNS__RR
_rr(tree)
	DNS__LDNS__DNSSecTrustTree tree;
	CODE:
	RETVAL = tree->rr;
	OUTPUT:
	RETVAL

DNS__LDNS__RRList
_rrset(tree)
	DNS__LDNS__DNSSecTrustTree tree;
	CODE:
	RETVAL = tree->rrset;
	OUTPUT:
	RETVAL

DNS__LDNS__DNSSecTrustTree
_parent(tree, i)
	DNS__LDNS__DNSSecTrustTree tree;
	size_t i;
	CODE:
	RETVAL = tree->parents[i];
	OUTPUT:
	RETVAL

LDNS_Status
_parent_status(tree, i)
	DNS__LDNS__DNSSecTrustTree tree;
	size_t i;
	CODE:
	RETVAL = tree->parent_status[i];
	OUTPUT:
	RETVAL

DNS__LDNS__RR
_parent_signature(tree, i)
	DNS__LDNS__DNSSecTrustTree tree;
	size_t i;
	CODE:
	RETVAL = tree->parent_signature[i];
	OUTPUT:
	RETVAL

size_t
parent_count(tree)
	DNS__LDNS__DNSSecTrustTree tree;
	CODE:
	RETVAL = tree->parent_count;
	OUTPUT:
	RETVAL
