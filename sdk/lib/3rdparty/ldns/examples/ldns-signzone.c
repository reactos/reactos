/*
 * ldns-signzone signs a zone file
 * 
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include <stdio.h>

#include "config.h"

#ifdef HAVE_SSL

#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include <time.h>

#include <ldns/ldns.h>
#include <ldns/keys.h>

#include <openssl/conf.h>
#ifndef OPENSSL_NO_ENGINE
#include <openssl/engine.h>
#endif
#include <openssl/err.h>

#define MAX_FILENAME_LEN 250

char *prog;
int verbosity = 1;

static void
usage(FILE *fp, const char *prog) {
	fprintf(fp, "%s [OPTIONS] zonefile key [key [key]]\n", prog);
	fprintf(fp, "  signs the zone with the given key(s)\n");
	fprintf(fp, "  -b\t\tuse layout in signed zone and print comments DNSSEC records\n");
	fprintf(fp, "  -d\t\tused keys are not added to the zone\n");
	fprintf(fp, "  -e <date>\texpiration date\n");
	fprintf(fp, "  -f <file>\toutput zone to file (default <name>.signed)\n");
	fprintf(fp, "  -i <date>\tinception date\n");
	fprintf(fp, "  -o <domain>\torigin for the zone\n");
	fprintf(fp, "  -u\t\tset SOA serial to the number of seconds since 1-1-1970\n");
	fprintf(fp, "  -v\t\tprint version and exit\n");
	fprintf(fp, "  -z <[scheme:]hash>\tAdd ZONEMD resource record\n");
	fprintf(fp, "\t\t<scheme> should be \"simple\" (or 1)\n");
	fprintf(fp, "\t\t<hash> should be \"sha384\" or \"sha512\" (or 1 or 2)\n");
	fprintf(fp, "\t\tthis option can be given more than once\n");
	fprintf(fp, "  -Z\t\tAllow ZONEMDs to be added without signing\n");
	fprintf(fp, "  -A\t\tsign DNSKEY with all keys instead of minimal\n");
	fprintf(fp, "  -U\t\tSign with every unique algorithm in the provided keys\n");
#ifndef OPENSSL_NO_ENGINE
	fprintf(fp, "  -E <name>\tuse <name> as the crypto engine for signing\n");
	fprintf(fp, "           \tThis can have a lot of extra options, see the manual page for more info\n");
	fprintf(fp, "  -k <algorithm>,<key>\tuse `key' with `algorithm' from engine as ZSK\n");
	fprintf(fp, "  -K <algorithm>,<key>\tuse `key' with `algorithm' from engine as KSK\n");
#endif
	fprintf(fp, "  -n\t\tuse NSEC3 instead of NSEC.\n");
	fprintf(fp, "\t\tIf you use NSEC3, you can specify the following extra options:\n");
	fprintf(fp, "\t\t-a [algorithm] hashing algorithm\n");
	fprintf(fp, "\t\t-t [number] number of hash iterations\n");
	fprintf(fp, "\t\t-s [string] salt\n");
	fprintf(fp, "\t\t-p set the opt-out flag on all nsec3 rrs\n");
	fprintf(fp, "\n");
	fprintf(fp, "  keys must be specified by their base name (usually K<name>+<alg>+<id>),\n");
	fprintf(fp, "  i.e. WITHOUT the .private extension.\n");
	fprintf(fp, "  If the public part of the key is not present in the zone, the DNSKEY RR\n");
	fprintf(fp, "  will be read from the file called <base name>.key. If that does not exist,\n");
	fprintf(fp, "  a default DNSKEY will be generated from the private key and added to the zone.\n");
	fprintf(fp, "  A date can be a timestamp (seconds since the epoch), or of\n  the form <YYYYMMdd[hhmmss]>\n");
#ifndef OPENSSL_NO_ENGINE
	fprintf(fp, "  For -k or -K, the algorithm can be specified as an integer or a symbolic name:" );

#define __LIST(x) fprintf ( fp, " %3d: %-15s", LDNS_SIGN_ ## x, # x )

	fprintf ( fp, "\n " );
	__LIST ( RSAMD5 );
#ifdef USE_DSA
	__LIST ( DSA );
#endif
	__LIST ( RSASHA1 );
	fprintf ( fp, "\n " );
#ifdef USE_DSA
	__LIST ( DSA_NSEC3 );
#endif
	__LIST ( RSASHA1_NSEC3 );
	__LIST ( RSASHA256 );
	fprintf ( fp, "\n " );
	__LIST ( RSASHA512 );
	__LIST ( ECC_GOST );
	__LIST ( ECDSAP256SHA256 );
	fprintf ( fp, "\n " );
	__LIST ( ECDSAP384SHA384 );

#ifdef USE_ED25519
	__LIST ( ED25519 );
#endif

#ifdef USE_ED448
	__LIST ( ED448 );
#endif
	fprintf ( fp, "\n" );

#undef __LIST
#endif
}

static void check_tm(struct tm tm)
{
	if (tm.tm_year < 70) {
		fprintf(stderr, "You cannot specify dates before 1970\n");
		exit(EXIT_FAILURE);
	}
	if (tm.tm_mon < 0 || tm.tm_mon > 11) {
		fprintf(stderr, "The month must be in the range 1 to 12\n");
		exit(EXIT_FAILURE);
	}
	if (tm.tm_mday < 1 || tm.tm_mday > 31) {
		fprintf(stderr, "The day must be in the range 1 to 31\n");
		exit(EXIT_FAILURE);
	}
	
	if (tm.tm_hour < 0 || tm.tm_hour > 23) {
		fprintf(stderr, "The hour must be in the range 0-23\n");
		exit(EXIT_FAILURE);
	}

	if (tm.tm_min < 0 || tm.tm_min > 59) {
		fprintf(stderr, "The minute must be in the range 0-59\n");
		exit(EXIT_FAILURE);
	}

	if (tm.tm_sec < 0 || tm.tm_sec > 59) {
		fprintf(stderr, "The second must be in the range 0-59\n");
		exit(EXIT_FAILURE);
	}

}

/*
 * if the ttls are different, make them equal
 * if one of the ttls equals LDNS_DEFAULT_TTL, that one is changed
 * otherwise, rr2 will get the ttl of rr1
 * 
 * prints a warning if a non-default TTL is changed
 */
static void
equalize_ttls(ldns_rr *rr1, ldns_rr *rr2, uint32_t default_ttl)
{
	uint32_t ttl1, ttl2;
	
	ttl1 = ldns_rr_ttl(rr1);
	ttl2 = ldns_rr_ttl(rr2);
	
	if (ttl1 != ttl2) {
		if (ttl1 == default_ttl) {
			ldns_rr_set_ttl(rr1, ttl2);
		} else if (ttl2 == default_ttl) {
			ldns_rr_set_ttl(rr2, ttl1);
		} else {
			ldns_rr_set_ttl(rr2, ttl1);
			fprintf(stderr, 
			        "warning: changing non-default TTL %u to %u\n",
			        (unsigned int) ttl2, (unsigned int)  ttl1);
		}
	}
}

static void
equalize_ttls_rr_list(ldns_rr_list *rr_list, ldns_rr *rr, uint32_t default_ttl)
{
	size_t i;
	ldns_rr *cur_rr;
	
	for (i = 0; i < ldns_rr_list_rr_count(rr_list); i++) {
		cur_rr = ldns_rr_list_rr(rr_list, i);
		if (ldns_rr_compare_no_rdata(cur_rr, rr) == 0) {
			equalize_ttls(cur_rr, rr, default_ttl);
		}
	}
}

static ldns_rr *
find_key_in_zone(ldns_rr *pubkey_gen, ldns_zone *zone) {
	size_t key_i;
	ldns_rr *pubkey;
	
	for (key_i = 0;
		key_i < ldns_rr_list_rr_count(ldns_zone_rrs(zone));
		key_i++) {
		pubkey = ldns_rr_list_rr(ldns_zone_rrs(zone), key_i);
		if (ldns_rr_get_type(pubkey) == LDNS_RR_TYPE_DNSKEY &&
			(ldns_calc_keytag(pubkey)
				==
				ldns_calc_keytag(pubkey_gen) ||
					 /* KSK has gen-keytag + 1 */
					 ldns_calc_keytag(pubkey)
					 ==
					 ldns_calc_keytag(pubkey_gen) + 1) 
			   ) {
				if (verbosity >= 2) {
					fprintf(stderr, "Found it in the zone!\n");
				}
				return pubkey;
		}
	}
	return NULL;
}

static ldns_rr *
find_key_in_file(const char *keyfile_name_base, ldns_key* ATTR_UNUSED(key),
	uint32_t zone_ttl)
{
	char *keyfile_name;
	FILE *keyfile;
	int line_nr;
	uint32_t default_ttl = zone_ttl;

	ldns_rr *pubkey = NULL;
	keyfile_name = LDNS_XMALLOC(char,
	                            strlen(keyfile_name_base) + 5);
	snprintf(keyfile_name,
		 strlen(keyfile_name_base) + 5,
		 "%s.key",
		 keyfile_name_base);
	if (verbosity >= 2) {
		fprintf(stderr, "Trying to read %s\n", keyfile_name);
	}
	keyfile = fopen(keyfile_name, "r");
	line_nr = 0;
	if (keyfile) {
		if (ldns_rr_new_frm_fp_l(&pubkey,
					 keyfile,
					 &default_ttl,
					 NULL,
					 NULL,
					 &line_nr) ==
		    LDNS_STATUS_OK) {
			if (verbosity >= 2) {
				printf("Key found in file: %s\n", keyfile_name);
			}
		}
		fclose(keyfile);
	}
	LDNS_FREE(keyfile_name);
	return pubkey;
}

/* this function tries to find the specified keys either in the zone that
 * has been read, or in a <basename>.key file. If the key is not found,
 * a public key is generated, and it is assumed the key is a ZSK
 * 
 * if add_keys is true; the DNSKEYs are added to the zone prior to signing
 * if it is false, they are not added.
 * Even if keys are not added, the function is still needed, to check
 * whether keys of which we only have key data are KSKs or ZSKS
 */
static void
find_or_create_pubkey(const char *keyfile_name_base, ldns_key *key, ldns_zone *orig_zone, bool add_keys, uint32_t default_ttl) {
	ldns_rr *pubkey_gen, *pubkey;
	int key_in_zone;
	
	if (default_ttl == LDNS_DEFAULT_TTL) {
		default_ttl = ldns_rr_ttl(ldns_zone_soa(orig_zone));
	}

	if (!ldns_key_pubkey_owner(key)) {
		ldns_key_set_pubkey_owner(key, ldns_rdf_clone(ldns_rr_owner(ldns_zone_soa(orig_zone))));
	}

	/* find the public key in the zone, or in a
	 * separate file
	 * we 'generate' one anyway, 
	 * then match that to any present in the zone,
	 * if it matches, we drop our own. If not,
	 * we try to see if there is a .key file present.
	 * If not, we use our own generated one, with
	 * some default values 
	 *
	 * Even if -d (do-not-add-keys) is specified, 
	 * we still need to do this, because we need
	 * to have any key flags that are set this way
	 */
	pubkey_gen = ldns_key2rr(key);
	ldns_rr_set_ttl(pubkey_gen, default_ttl);

	if (verbosity >= 2) {
		fprintf(stderr,
			   "Looking for key with keytag %u or %u\n",
			   (unsigned int) ldns_calc_keytag(pubkey_gen),
			   (unsigned int) ldns_calc_keytag(pubkey_gen)+1
			   );
	}

	pubkey = find_key_in_zone(pubkey_gen, orig_zone);
	key_in_zone = 1;
	if (!pubkey) {
		key_in_zone = 0;
		/* it was not in the zone, try to read a .key file */
		pubkey = find_key_in_file(keyfile_name_base, key, default_ttl);
		if (!pubkey && !(ldns_key_flags(key) & LDNS_KEY_SEP_KEY)) {
			/* maybe it is a ksk? */
			ldns_key_set_keytag(key, ldns_key_keytag(key) + 1);
			pubkey = find_key_in_file(keyfile_name_base, key, default_ttl);
			if (!pubkey) {
				/* ok, no file, set back to ZSK */
				ldns_key_set_keytag(key, ldns_key_keytag(key) - 1);
			}
		}
		if(pubkey && ldns_dname_compare(ldns_rr_owner(pubkey), ldns_rr_owner(ldns_zone_soa(orig_zone))) != 0) {
			fprintf(stderr, "Error %s.key has wrong name: %s\n",
				keyfile_name_base, ldns_rdf2str(ldns_rr_owner(pubkey)));
			exit(EXIT_FAILURE); /* leak rdf2str, but we exit */
		}
	}
	
	if (!pubkey) {
		/* okay, no public key found,
		   just use our generated one */
		pubkey = pubkey_gen;
		if (verbosity >= 2) {
			fprintf(stderr, "Not in zone, no .key file, generating ZSK DNSKEY from private key data\n");
		}
	} else {
		ldns_rr_free(pubkey_gen);
	}
	ldns_key_set_flags(key, ldns_rdf2native_int16(ldns_rr_rdf(pubkey, 0)));
	ldns_key_set_keytag(key, ldns_calc_keytag(pubkey));
	
	if (add_keys && !key_in_zone) {
		equalize_ttls_rr_list(ldns_zone_rrs(orig_zone), pubkey, default_ttl);
		ldns_zone_push_rr(orig_zone, pubkey);
	}
}

#ifndef OPENSSL_NO_ENGINE
/*
 * For keys coming from the engine (-k or -K), parse algorithm specification.
 */
static enum ldns_enum_signing_algorithm
parse_algspec ( const char * const p )
{
	if ( p == NULL )
		return 0;

	if ( isdigit ( (const unsigned char)*p ) ) {
		const char *nptr = NULL;
		const long id = strtol ( p, (char **) &nptr, 10 );
		return id > 0 && nptr != NULL && *nptr == ',' ? id : 0;
	}

#define __MATCH(x)							\
	if ( !memcmp ( # x, p, sizeof ( # x ) - 1 )			\
	     && p [ sizeof ( # x ) - 1 ] == ',' ) {			\
		return LDNS_SIGN_ ## x;					\
	}

	__MATCH ( RSAMD5 );
	__MATCH ( RSASHA1 );
#ifdef USE_DSA
	__MATCH ( DSA );
#endif
	__MATCH ( RSASHA1_NSEC3 );
	__MATCH ( RSASHA256 );
	__MATCH ( RSASHA512 );
#ifdef USE_DSA
	__MATCH ( DSA_NSEC3 );
#endif
	__MATCH ( ECC_GOST );
	__MATCH ( ECDSAP256SHA256 );
	__MATCH ( ECDSAP384SHA384 );

#ifdef USE_ED25519
	__MATCH ( ED25519 );
#endif

#ifdef USE_ED448
	__MATCH ( ED448 );
#endif

#undef __MATCH

	return 0;
}

/*
 * For keys coming from the engine (-k or -K), parse key specification
 * in the form of <algorithm>,<key-id>. No whitespace is allowed
 * between <algorithm> and the comma, and between the comma and
 * <key-id>. <key-id> format is specific to the engine at hand, i.e.
 * it can be the old OpenSC syntax or a PKCS #11 URI as defined in RFC 7512
 * and (partially) supported by OpenSC (as of 20180312).
 */
static const char *
parse_keyspec ( const char * const p,
		enum ldns_enum_signing_algorithm * const algorithm,
		const char ** const id )
{
	const char * const comma = strchr ( p, ',' );

	if ( comma == NULL || !(*algorithm = parse_algspec ( p )) )
		return NULL;
	return comma [ 1 ] ? *id = comma + 1 : NULL;
}

/*
 * Load a key from the engine.
 */
static ldns_key *
load_key ( const char * const p, ENGINE * const e )
{
	enum ldns_enum_signing_algorithm alg = 0;
	const char *id = NULL;
	ldns_status status = LDNS_STATUS_ERR;
	ldns_key *key = NULL;

	/* Parse key specification. */
	if ( parse_keyspec ( p, &alg, &id ) == NULL ) {
		fprintf ( stderr,
			  "Failed to parse key specification `%s'.\n",
			  p );
		usage ( stderr, prog );
		exit ( EXIT_FAILURE );
	}

	/* Validate that the algorithm can be used for signing. */
	switch ( alg ) {
	case LDNS_SIGN_RSAMD5:
	case LDNS_SIGN_RSASHA1:
	case LDNS_SIGN_RSASHA1_NSEC3:
	case LDNS_SIGN_RSASHA256:
	case LDNS_SIGN_RSASHA512:
#ifdef USE_DSA
	case LDNS_SIGN_DSA:
	case LDNS_SIGN_DSA_NSEC3:
#endif
	case LDNS_SIGN_ECC_GOST:
#ifdef USE_ECDSA
	case LDNS_SIGN_ECDSAP256SHA256:
	case LDNS_SIGN_ECDSAP384SHA384:
#endif
		break;
	default:
		fprintf ( stderr,
			  "Algorithm %d cannot be used for signing.\n",
			  alg );
		usage ( stderr, prog );
		exit ( EXIT_FAILURE );
	}

	printf ( "Engine key id: %s, algo %d\n", id, alg );

	/* Attempt to load the key from the engine. */
	status = ldns_key_new_frm_engine (
			&key, e, (char *) id, (ldns_algorithm)alg );
	if ( status != LDNS_STATUS_OK ) {
		ERR_print_errors_fp ( stderr );
		exit ( EXIT_FAILURE );
	}

	return key;
}

/*
 * For keys coming from the engine (-k or -K), set key parameters
 * and determine whether the key is listed in the zone file.
 */
static void
post_process_engine_key ( ldns_key_list * const keys,
			  ldns_key * const key,
			  ldns_zone * const zone,
			  const bool add_keys,
			  const uint32_t ttl,
			  const uint32_t inception,
			  const uint32_t expiration )
{
	if ( key == NULL ) return;

	if ( expiration ) ldns_key_set_expiration ( key, expiration );

	if ( inception ) ldns_key_set_inception ( key, inception );

	ldns_key_list_push_key ( keys, key );
	find_or_create_pubkey ( "", key, zone, add_keys, ttl );
}

/*
 * Initialize OpenSSL, for versions 1.1 and newer. 
 */
static ENGINE *
init_openssl_engine ( const char * const id )
{
	ENGINE *e = NULL;

#ifdef HAVE_ERR_LOAD_CRYPTO_STRINGS
        ERR_load_crypto_strings();
#endif
#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL) || !defined(HAVE_OPENSSL_INIT_CRYPTO)
        OpenSSL_add_all_algorithms();
#else
	if ( !OPENSSL_init_crypto ( OPENSSL_INIT_LOAD_CONFIG, NULL ) ) {
		fprintf ( stderr, "OPENSSL_init_crypto(3) failed.\n" );
		ERR_print_errors_fp ( stderr );
		exit ( EXIT_FAILURE );
	}
#endif

	if ( (e = ENGINE_by_id ( id )) == NULL ) {
		fprintf ( stderr, "ENGINE_by_id(3) failed.\n" );
		ERR_print_errors_fp ( stderr );
		exit ( EXIT_FAILURE );
	}

	if (  !ENGINE_set_default_DSA ( e ) ) {
		fprintf ( stderr, "ENGINE_set_default_DSA(3) failed.\n" );
		ERR_print_errors_fp ( stderr );
		exit ( EXIT_FAILURE );
	}

	if (  !ENGINE_set_default_RSA ( e ) ) {
		fprintf ( stderr, "ENGINE_set_default_RSA(3) failed.\n" );
		ERR_print_errors_fp ( stderr );
		exit ( EXIT_FAILURE );
	}

	return e;
}

/*
 * De-initialize OpenSSL, for versions 1.1 and newer.
 *
 * All of that is not strictly necessary because the process exits
 * anyway, however, when an engine is used, this is the only hope
 * of letting the engine's driver know that the program terminates
 * (for the fear that the driver's reference counting may go awry, etc.)
 * Still, there is no guarantee that this function helps...
 */
static void
shutdown_openssl ( ENGINE * const e )
{
	if ( e != NULL ) {
#ifdef HAVE_ENGINE_FREE
		ENGINE_free ( e );
#endif
#ifdef HAVE_ENGINE_CLEANUP
		ENGINE_cleanup ();
#endif
	}

#ifdef HAVE_CONF_MODULES_UNLOAD
	CONF_modules_unload ( 1 );
#endif
#ifdef HAVE_EVP_CLEANUP
	EVP_cleanup ();
#endif
#ifdef HAVE_CRYPTO_CLEANUP_ALL_EX_DATA
	CRYPTO_cleanup_all_ex_data ();
#endif
#ifdef HAVE_ERR_FREE_STRINGS
	ERR_free_strings ();
#endif
}
#endif

int str2zonemd_signflag(const char *str, const char **reason)
{
	char *colon;

	static const char *reasons[] = {
	      "Unknown <scheme>, should be \"simple\""
	    , "Syntax error in <hash>, should be \"sha384\" or \"sha512\""
	    , "Unknown <hash>, should be \"sha384\" or \"sha512\""
	};

	if (!str)
		return LDNS_STATUS_NULL;

	if ((colon = strchr(str, ':'))) {
		if ((colon - str != 1 || str[0] != '1')
		&&  (colon - str != 6 || strncasecmp(str, "simple", 6))) {
			if (reason) *reason = reasons[0];
			return 0;
		}

		if (strchr(colon + 1, ':')) {
			if (reason) *reason = reasons[1];
			return 0;
		}
		return str2zonemd_signflag(colon + 1, reason);
	}
	if (!strcasecmp(str, "1") || !strcasecmp(str, "sha384"))
		return LDNS_SIGN_WITH_ZONEMD_SIMPLE_SHA384;
	if (!strcasecmp(str, "2") || !strcasecmp(str, "sha512"))
		return LDNS_SIGN_WITH_ZONEMD_SIMPLE_SHA512;

	if (reason) *reason = reasons[2];
	return 0;
}

int
main(int argc, char *argv[])
{
	const char *zonefile_name;
	FILE *zonefile = NULL;
	int line_nr = 0;
	int c;
	int argi;
#ifndef OPENSSL_NO_ENGINE
	ENGINE *engine = NULL;
#endif
	ldns_zone *orig_zone;
	ldns_rr_list *orig_rrs = NULL;
	ldns_rr *orig_soa = NULL;
	ldns_dnssec_zone *signed_zone;

	char *keyfile_name_base;
	char *keyfile_name = NULL;
	FILE *keyfile = NULL;
	ldns_key *key = NULL;
#ifndef OPENSSL_NO_ENGINE
	ldns_key *eng_ksk = NULL; /* KSK specified with -K */
	ldns_key *eng_zsk = NULL; /* ZSK specified with -k */
#endif
	ldns_key_list *keys;
	ldns_status s;
	size_t i;
	ldns_rr_list *added_rrs;

	char *outputfile_name = NULL;
	FILE *outputfile;
	
	bool use_nsec3 = false;
	int signflags = 0;
	bool unixtime_serial = false;

	/* Add the given keys to the zone if they are not yet present */
	bool add_keys = true;
	uint8_t nsec3_algorithm = 1;
	uint8_t nsec3_flags = 0;
	size_t nsec3_iterations_cmd = 1;
	uint16_t nsec3_iterations = 1;
	uint8_t nsec3_salt_length = 0;
	uint8_t *nsec3_salt = NULL;
	
	/* we need to know the origin before reading ksk's,
	 * so keep an array of filenames until we know it
	 */
	struct tm tm;
	uint32_t inception;
	uint32_t expiration;
	ldns_rdf *origin = NULL;
	uint32_t ttl = LDNS_DEFAULT_TTL;
	ldns_rr_class class = LDNS_RR_CLASS_IN;	
	
	ldns_status result;

	ldns_output_format_storage fmt_st;
	ldns_output_format* fmt = ldns_output_format_init(&fmt_st);

	/* For parson zone digest parameters */
	int flag;
	const char *reason = NULL;

	prog = _strdup(argv[0]);
	inception = 0;
	expiration = 0;
	
	keys = ldns_key_list_new();

	while ((c = getopt(argc, argv, "a:bde:f:i:k:no:ps:t:uvz:ZAUE:K:")) != -1) {
		switch (c) {
		case 'a':
			nsec3_algorithm = (uint8_t) atoi(optarg);
			if (nsec3_algorithm != 1) {
				fprintf(stderr, "Bad NSEC3 algorithm, only RSASHA1 allowed\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'b':
			ldns_output_format_set(fmt, LDNS_COMMENT_FLAGS
						  | LDNS_COMMENT_LAYOUT      
						  | LDNS_COMMENT_NSEC3_CHAIN
						  | LDNS_COMMENT_BUBBLEBABBLE);
			break;
		case 'd':
			add_keys = false;
			break;
		case 'e':
			/* try to parse YYYYMMDD first,
			 * if that doesn't work, it
			 * should be a timestamp (seconds since epoch)
			 */
			memset(&tm, 0, sizeof(tm));

			if (strlen(optarg) == 8 &&
			    sscanf(optarg, "%4d%2d%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday)
			    ) {
			   	tm.tm_year -= 1900;
			   	tm.tm_mon--;
			   	check_tm(tm);
				expiration = 
					(uint32_t) ldns_mktime_from_utc(&tm);
			} else if (strlen(optarg) == 14 &&
					 sscanf(optarg, "%4d%2d%2d%2d%2d%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec)
					 ) {
			   	tm.tm_year -= 1900;
			   	tm.tm_mon--;
			   	check_tm(tm);
				expiration = 
					(uint32_t) ldns_mktime_from_utc(&tm);
			} else {
				expiration = (uint32_t) atol(optarg);
			}
			break;
		case 'f':
			outputfile_name = LDNS_XMALLOC(char, MAX_FILENAME_LEN + 1);
			strncpy(outputfile_name, optarg, MAX_FILENAME_LEN);
			break;
		case 'i':
			memset(&tm, 0, sizeof(tm));

			if (strlen(optarg) == 8 &&
			    sscanf(optarg, "%4d%2d%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday)
			    ) {
			   	tm.tm_year -= 1900;
			   	tm.tm_mon--;
			   	check_tm(tm);
				inception = 
					(uint32_t) ldns_mktime_from_utc(&tm);
			} else if (strlen(optarg) == 14 &&
					 sscanf(optarg, "%4d%2d%2d%2d%2d%2d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec)
					 ) {
			   	tm.tm_year -= 1900;
			   	tm.tm_mon--;
			   	check_tm(tm);
				inception = 
					(uint32_t) ldns_mktime_from_utc(&tm);
			} else {
				inception = (uint32_t) atol(optarg);
			}
			break;
		case 'n':
			use_nsec3 = true;
			break;
		case 'o':
			if (ldns_str2rdf_dname(&origin, optarg) != LDNS_STATUS_OK) {
				fprintf(stderr, "Bad origin, not a correct domain name\n");
				usage(stderr, prog);
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			nsec3_flags = nsec3_flags | LDNS_NSEC3_VARS_OPTOUT_MASK;
			break;
		case 'u':
			unixtime_serial = true;
			break;
		case 'v':
			printf("zone signer version %s (ldns version %s)\n", LDNS_VERSION, ldns_version());
			exit(EXIT_SUCCESS);
			break;
		case 'z':
			flag = str2zonemd_signflag(optarg, &reason);
			if (flag)
				signflags |= flag;
			else {
				fprintf( stderr
				       , "%s\nwith zone digest parameters:"
				         " \"%s\"\n"
				       , reason, optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'Z':
			signflags |= LDNS_SIGN_NO_KEYS_NO_NSECS;
			break;
		case 'A':
			signflags |= LDNS_SIGN_DNSKEY_WITH_ZSK;
			break;
		case 'E':
#ifndef OPENSSL_NO_ENGINE
			engine = init_openssl_engine ( optarg );
			break;
#else
			/* fallthrough */
#endif
		case 'k':
#ifndef OPENSSL_NO_ENGINE
			eng_zsk = load_key ( optarg, engine );
			break;
#else
			/* fallthrough */
#endif
		case 'K':
#ifndef OPENSSL_NO_ENGINE
			eng_ksk = load_key ( optarg, engine );
			/* I apologize for that, there is no API. */
			eng_ksk -> _extra.dnssec.flags |= LDNS_KEY_SEP_KEY;
#else
			fprintf(stderr, "%s compiled without engine support\n"
			              , prog);
			exit(EXIT_FAILURE);
#endif
			break;
		case 'U':
			signflags |= LDNS_SIGN_WITH_ALL_ALGORITHMS;
			break;
		case 's':
			if (strlen(optarg) % 2 != 0) {
				fprintf(stderr, "Salt value is not valid hex data, not a multiple of 2 characters\n");
				exit(EXIT_FAILURE);
			}
			nsec3_salt_length = (uint8_t) strlen(optarg) / 2;
			nsec3_salt = LDNS_XMALLOC(uint8_t, nsec3_salt_length);
			for (c = 0; c < (int) strlen(optarg); c += 2) {
				if (isxdigit((int) optarg[c]) && isxdigit((int) optarg[c+1])) {
					nsec3_salt[c/2] = (uint8_t) ldns_hexdigit_to_int(optarg[c]) * 16 +
						ldns_hexdigit_to_int(optarg[c+1]);
				} else {
					fprintf(stderr, "Salt value is not valid hex data.\n");
					exit(EXIT_FAILURE);
				}
			}

			break;
		case 't':
			nsec3_iterations_cmd = (size_t) atol(optarg);
			if (nsec3_iterations_cmd > LDNS_NSEC3_MAX_ITERATIONS) {
				fprintf(stderr, "Iterations count can not exceed %u, quitting\n", LDNS_NSEC3_MAX_ITERATIONS);
				exit(EXIT_FAILURE);
			}
			nsec3_iterations = (uint16_t) nsec3_iterations_cmd;
			break;
		default:
			usage(stderr, prog);
			exit(EXIT_SUCCESS);
		}
	}
	
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		printf("Error: not enough arguments\n");
		usage(stdout, prog);
		exit(EXIT_FAILURE);
	} else {
		zonefile_name = argv[0];
	}

	/* read zonefile first to find origin if not specified */
	
	if (strncmp(zonefile_name, "-", 2) == 0) {
		s = ldns_zone_new_frm_fp_l(&orig_zone,
					   stdin,
					   origin,
					   ttl,
					   class,
					   &line_nr);
			if (s != LDNS_STATUS_OK) {
				fprintf(stderr, "Zone not read, error: %s at stdin line %d\n", 
					   ldns_get_errorstr_by_id(s),
					   line_nr);
				exit(EXIT_FAILURE);
			} else {
				orig_soa = ldns_zone_soa(orig_zone);
				if (!orig_soa) {
					fprintf(stderr,
						   "Error reading zonefile: missing SOA record\n");
					exit(EXIT_FAILURE);
				}
				orig_rrs = ldns_zone_rrs(orig_zone);
				if (!orig_rrs) {
					fprintf(stderr,
						   "Error reading zonefile: no resource records\n");
					exit(EXIT_FAILURE);
				}
			}
	} else {
		zonefile = fopen(zonefile_name, "r");
		
		if (!zonefile) {
			fprintf(stderr,
				   "Error: unable to read %s (%s)\n",
				   zonefile_name,
				   strerror(errno));
			exit(EXIT_FAILURE);
		} else {
			s = ldns_zone_new_frm_fp_l(&orig_zone,
			                           zonefile,
			                           origin,
			                           ttl,
			                           class,
			                           &line_nr);
			if (s != LDNS_STATUS_OK) {
				fprintf(stderr, "Zone not read, error: %s at %s line %d\n", 
					   ldns_get_errorstr_by_id(s), 
					   zonefile_name, line_nr);
				exit(EXIT_FAILURE);
			} else {
				orig_soa = ldns_zone_soa(orig_zone);
				if (!orig_soa) {
					fprintf(stderr,
						   "Error reading zonefile: missing SOA record\n");
					exit(EXIT_FAILURE);
				}
				orig_rrs = ldns_zone_rrs(orig_zone);
				if (!orig_rrs) {
					fprintf(stderr,
						   "Error reading zonefile: no resource records\n");
					exit(EXIT_FAILURE);
				}
			}
			fclose(zonefile);
		}
	}

	/* read the ZSKs */
	argi = 1;
	while (argi < argc) {
		keyfile_name_base = argv[argi];
		keyfile_name = LDNS_XMALLOC(char, strlen(keyfile_name_base) + 9);
		snprintf(keyfile_name,
			    strlen(keyfile_name_base) + 9,
			    "%s.private",
			    keyfile_name_base);
		keyfile = fopen(keyfile_name, "r");
		line_nr = 0;
		if (!keyfile) {
			fprintf(stderr,
				   "Error: unable to read %s: %s\n",
				   keyfile_name,
				   strerror(errno));
		} else {
			s = ldns_key_new_frm_fp_l(&key, keyfile, &line_nr);
			fclose(keyfile);
			if (s == LDNS_STATUS_OK) {
				/* set times in key? they will end up
				   in the rrsigs
				*/
				if (expiration != 0) {
					ldns_key_set_expiration(key, expiration);
				}
				if (inception != 0) {
					ldns_key_set_inception(key, inception);
				}

				LDNS_FREE(keyfile_name);
				
				ldns_key_list_push_key(keys, key);
			} else {
				fprintf(stderr, "Error reading key from %s at line %d: %s\n", argv[argi], line_nr, ldns_get_errorstr_by_id(s));
			}
		}
		/* and, if not unset by -p, find or create the corresponding DNSKEY record */
		if (key) {
			find_or_create_pubkey(keyfile_name_base, key,
			                      orig_zone, add_keys, ttl);
		}
		argi++;
	}

#ifndef OPENSSL_NO_ENGINE
       /*
	* The user may have loaded a KSK and a ZSK from the engine.
	* Since these keys carry no meta-information which is
	* relevant to DNS (origin, TTL, etc), and because that
	* information becomes known only after the command line
	* and the zone file are parsed completely, the program
	* needs to post-process these keys before they become usable.
	*/

	/* The engine's KSK. */
	post_process_engine_key ( keys,
				  eng_ksk,
				  orig_zone,
				  add_keys,
				  ttl,
				  inception,
				  expiration );

	/* The engine's ZSK. */
	post_process_engine_key ( keys,
				  eng_zsk,
				  orig_zone,
				  add_keys,
				  ttl,
				  inception,
				  expiration );
#endif
	if (ldns_key_list_key_count(keys) < 1 
	&&  !(signflags & LDNS_SIGN_NO_KEYS_NO_NSECS)) {
			
		fprintf(stderr, "Error: no keys to sign with. Aborting.\n\n");
		usage(stderr, prog);
		exit(EXIT_FAILURE);
	}

	signed_zone = ldns_dnssec_zone_new();
	if (unixtime_serial) {
		ldns_rr_soa_increment_func_int(ldns_zone_soa(orig_zone),
			ldns_soa_serial_unixtime, 0);
	}
	if (ldns_dnssec_zone_add_rr(signed_zone, ldns_zone_soa(orig_zone)) !=
	    LDNS_STATUS_OK) {
		fprintf(stderr,
		  "Error adding SOA to dnssec zone, skipping record\n");
	}
	
	for (i = 0;
	     i < ldns_rr_list_rr_count(ldns_zone_rrs(orig_zone));
	     i++) {
		if (ldns_dnssec_zone_add_rr(signed_zone, 
		         ldns_rr_list_rr(ldns_zone_rrs(orig_zone), 
		         i)) !=
		    LDNS_STATUS_OK) {
			fprintf(stderr,
			        "Error adding RR to dnssec zone");
			fprintf(stderr, ", skipping record:\n");
			ldns_rr_print(stderr, 
			  ldns_rr_list_rr(ldns_zone_rrs(orig_zone), i));
		}
	}
	/* list to store newly created rrs, so we can free them later */
	added_rrs = ldns_rr_list_new();

	if (use_nsec3) {
		if (verbosity < 1)
			; /* pass */

		else if (nsec3_iterations > 500)
			fprintf(stderr, "Warning! NSEC3 iterations larger than "
			    "500 may cause validating resolvers to return "
			    "SERVFAIL!\n"
			    "See: https://datatracker.ietf.org/doc/html/"
			    "draft-hardaker-dnsop-nsec3-guidance-03#section-4\n");

		else if (nsec3_iterations > 100)
			fprintf(stderr, "Warning! NSEC3 iterations larger than "
			    "100 may cause validating resolvers to return "
			    "insecure responses!\n"
			    "See: https://datatracker.ietf.org/doc/html/"
			    "draft-hardaker-dnsop-nsec3-guidance-03#section-4\n");

		result = ldns_dnssec_zone_sign_nsec3_flg_mkmap(signed_zone,
			added_rrs,
			keys,
			ldns_dnssec_default_replace_signatures,
			NULL,
			nsec3_algorithm,
			nsec3_flags,
			nsec3_iterations,
			nsec3_salt_length,
			nsec3_salt,
			signflags,
			&fmt_st.hashmap);
	} else {
		result = ldns_dnssec_zone_sign_flg(signed_zone,
				added_rrs,
				keys,
				ldns_dnssec_default_replace_signatures,
				NULL,
				signflags);
	}
	if (result != LDNS_STATUS_OK) {
		fprintf(stderr, "Error signing zone: %s\n",
			   ldns_get_errorstr_by_id(result));
	}

	if (!outputfile_name) {
		outputfile_name = LDNS_XMALLOC(char, MAX_FILENAME_LEN);
		snprintf(outputfile_name, MAX_FILENAME_LEN, "%s.signed", zonefile_name);
	}

	if (signed_zone) {
		if (strncmp(outputfile_name, "-", 2) == 0) {
			ldns_dnssec_zone_print(stdout, signed_zone);
		} else {
			outputfile = fopen(outputfile_name, "w");
			if (!outputfile) {
				fprintf(stderr, "Unable to open %s for writing: %s\n",
					   outputfile_name, strerror(errno));
			} else {
				ldns_dnssec_zone_print_fmt(
						outputfile, fmt, signed_zone);
				fclose(outputfile);
			}
		}
	} else {
		fprintf(stderr, "Error signing zone.\n");

#ifdef HAVE_SSL
		if (ERR_peek_error()) {
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(HAVE_LIBRESSL)
#ifdef HAVE_ERR_LOAD_CRYPTO_STRINGS
			ERR_load_crypto_strings();
#endif
#endif
			ERR_print_errors_fp(stderr);
#if OPENSSL_VERSION_NUMBER < 0x10100000L || defined(HAVE_LIBRESSL)
#ifdef HAVE_ERR_FREE_STRINGS
			ERR_free_strings ();
#endif
#endif
		}
#endif
		exit(EXIT_FAILURE);
	}
	
	ldns_key_list_free(keys);
	/* since the ldns_rr records are pointed to in both the ldns_zone
	 * and the ldns_dnssec_zone, we can either deep_free the
	 * dnssec_zone and 'shallow' free the original zone and added
	 * records, or the other way around
	 */
	ldns_dnssec_zone_free(signed_zone);
	ldns_zone_deep_free(orig_zone);
	ldns_rr_list_deep_free(added_rrs);
	ldns_rdf_deep_free(origin);
	LDNS_FREE(outputfile_name);

#ifndef OPENSSL_NO_ENGINE
	shutdown_openssl ( engine );
#else
#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL)
#ifdef HAVE_CRYPTO_CLEANUP_ALL_EX_DATA
	CRYPTO_cleanup_all_ex_data ();
#endif
#endif
#endif

	free(prog);
	exit(EXIT_SUCCESS);
}

#else /* !HAVE_SSL */
int
main(int argc __attribute__((unused)),
     char **argv __attribute__((unused)))
{
       fprintf(stderr, "ldns-signzone needs OpenSSL support, which has not been compiled in\n");
       return 1;
}
#endif /* HAVE_SSL */
