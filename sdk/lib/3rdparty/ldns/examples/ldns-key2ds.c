/*
 * key2ds transforms a public key into its DS
 * It (currently) prints out the public key
 *
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"

#include <ldns/ldns.h>

#include <errno.h>

static void
usage(FILE *fp, char *prog) {
	fprintf(fp, "%s [-fn] [-1|-2] keyfile\n", prog);
	fprintf(fp, "  Generate a DS RR from the DNSKEYS in keyfile\n");
	fprintf(fp, "  The following file will be created: ");
	fprintf(fp, "K<name>+<alg>+<id>.ds\n");
	fprintf(fp, "  The base name (K<name>+<alg>+<id> will be printed to stdout\n");
	fprintf(fp, "Options:\n");
	fprintf(fp, "  -f: ignore SEP flag (i.e. make DS records for any key)\n");
	fprintf(fp, "  -n: do not write DS records to file(s) but to stdout\n");
	fprintf(fp, "  (default) use similar hash to the key algorithm.\n");
	fprintf(fp, "  -1: use SHA1 for the DS hash\n");
	fprintf(fp, "  -2: use SHA256 for the DS hash\n");
#ifdef USE_GOST
	fprintf(fp, "  -g: use GOST for the DS hash\n");
#endif
#ifdef USE_ECDSA
	fprintf(fp, "  -4: use SHA384 for the DS hash\n");
#endif
}

static int
is_suitable_dnskey(ldns_rr *rr, int sep_only)
{
	if (!rr || ldns_rr_get_type(rr) != LDNS_RR_TYPE_DNSKEY) {
		return 0;
	}
	return !sep_only ||
	        (ldns_rdf2native_int16(ldns_rr_dnskey_flags(rr)) &
	        LDNS_KEY_SEP_KEY);
}

static ldns_hash
suitable_hash(ldns_signing_algorithm algorithm)
{
	switch (algorithm) {
	case LDNS_SIGN_RSASHA256:
	case LDNS_SIGN_RSASHA512:
		return LDNS_SHA256;
	case LDNS_SIGN_ECC_GOST:
#ifdef USE_GOST
		return LDNS_HASH_GOST;
#else
		return LDNS_SHA256;
#endif
#ifdef USE_ECDSA
	case LDNS_SIGN_ECDSAP256SHA256:
		return LDNS_SHA256;
	case LDNS_SIGN_ECDSAP384SHA384:
		return LDNS_SHA384;
#endif
#ifdef USE_ED25519
	case LDNS_SIGN_ED25519:
		return LDNS_SHA256;
#endif
#ifdef USE_ED448
	case LDNS_SIGN_ED448:
		return LDNS_SHA256;
#endif
	default: break;
	}
	return LDNS_SHA1;
}

int
main(int argc, char *argv[])
{
	FILE *keyfp, *dsfp;
	char *keyname;
	char *dsname;
	char *owner;
	ldns_rr *k, *ds;
	ldns_signing_algorithm alg;
	ldns_hash h;
	int similar_hash=1;
	char *program = argv[0];
	int nofile = 0;
	ldns_rdf *origin = NULL;
	ldns_status result = LDNS_STATUS_OK;
	int sep_only = 1;

	h = LDNS_SHA1;

	argv++, argc--;
	while (argc && argv[0][0] == '-') {
		if (strcmp(argv[0], "-1") == 0) {
			h = LDNS_SHA1;
			similar_hash = 0;
		}
		if (strcmp(argv[0], "-2") == 0) {
			h = LDNS_SHA256;
			similar_hash = 0;
		}
#ifdef USE_GOST
		if (strcmp(argv[0], "-g") == 0) {
			if(!ldns_key_EVP_load_gost_id()) {
				fprintf(stderr, "error: libcrypto does not provide GOST\n");
				exit(EXIT_FAILURE);
			}
			h = LDNS_HASH_GOST;
			similar_hash = 0;
		}
#endif
#ifdef USE_ECDSA
		if (strcmp(argv[0], "-4") == 0) {
			h = LDNS_SHA384;
			similar_hash = 0;
		}
#endif
		if (strcmp(argv[0], "-f") == 0) {
			sep_only = 0;
		}
		if (strcmp(argv[0], "-n") == 0) {
			nofile=1;
		}
		argv++, argc--;
	}

	if (argc != 1) {
		usage(stderr, program);
		exit(EXIT_FAILURE);
	}
	keyname = _strdup(argv[0]);

	keyfp = fopen(keyname, "r");
	if (!keyfp) {
		fprintf(stderr, "Failed to open public key file %s: %s\n", keyname,
				strerror(errno));
		exit(EXIT_FAILURE);
	}

	while (result == LDNS_STATUS_OK) {
		result = ldns_rr_new_frm_fp(&k, keyfp, 0, &origin, NULL);
		while (result == LDNS_STATUS_SYNTAX_ORIGIN ||
			result == LDNS_STATUS_SYNTAX_TTL ||
			(result == LDNS_STATUS_OK && !is_suitable_dnskey(k, sep_only))
		) {
			if (result == LDNS_STATUS_OK) {
				ldns_rr_free(k);
			}
			result = ldns_rr_new_frm_fp(&k, keyfp, 0, &origin, NULL);
		}
		if (result == LDNS_STATUS_SYNTAX_EMPTY) {
			/* we're done */
			break;
		}
		if (result != LDNS_STATUS_OK) {
			fprintf(stderr, "Could not read public key from file %s: %s\n", keyname, ldns_get_errorstr_by_id(result));
			exit(EXIT_FAILURE);
		}

		owner = ldns_rdf2str(ldns_rr_owner(k));
		alg = ldns_rdf2native_int8(ldns_rr_dnskey_algorithm(k));
		if(similar_hash)
			h = suitable_hash(alg);

		ds = ldns_key_rr2ds(k, h);
		if (!ds) {
			fprintf(stderr, "Conversion to a DS RR failed\n");
			ldns_rr_free(k);
			free(owner);
			exit(EXIT_FAILURE);
		}

		/* print the public key RR to .key */
		dsname = LDNS_XMALLOC(char, strlen(owner) + 16);
		snprintf(dsname, strlen(owner) + 15, "K%s+%03u+%05u.ds", owner, alg, (unsigned int) ldns_calc_keytag(k));

		if (nofile)
			ldns_rr_print(stdout,ds);
		else {
			dsfp = fopen(dsname, "w");
			if (!dsfp) {
				fprintf(stderr, "Unable to open %s: %s\n", dsname, strerror(errno));
				exit(EXIT_FAILURE);
			} else {
				ldns_rr_print(dsfp, ds);
				fclose(dsfp);
				fprintf(stdout, "K%s+%03u+%05u\n", owner, alg, (unsigned int) ldns_calc_keytag(k)); 
			}
		}

		ldns_rr_free(ds);
		ldns_rr_free(k);
		free(owner);
		LDNS_FREE(dsname);
	}
	fclose(keyfp);
	free(keyname);
	exit(EXIT_SUCCESS);
}
