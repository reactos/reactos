/*
 * revoke sets the revoke bit of a public key.
 *
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"

#include <ldns/ldns.h>
#ifdef HAVE_SSL
#include <openssl/ssl.h>
#endif /* HAVE_SSL */

#include <errno.h>

static void
usage(FILE *fp, char *prog) {
	fprintf(fp, "%s [-n] keyfile\n", prog);
	fprintf(fp, "  Revokes a key\n");
	fprintf(fp, "Options:\n");
	fprintf(fp, "  -n: do not write to file but to stdout\n");
}

int
main(int argc, char *argv[])
{
	FILE *keyfp;
	char *keyname;
	ldns_rr *k;
	uint16_t flags;
	char *program = argv[0];
	int nofile = 0;
	ldns_rdf *origin = NULL;
	ldns_status result;

	argv++, argc--;
	while (argc && argv[0][0] == '-') {
		if (strcmp(argv[0], "-n") == 0) {
			nofile=1;
		}
		else {
			usage(stderr, program);
			exit(EXIT_FAILURE);
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

	result = ldns_rr_new_frm_fp(&k, keyfp, 0, &origin, NULL);
	/* what does this while loop do? */
	while (result == LDNS_STATUS_SYNTAX_ORIGIN) {
		result = ldns_rr_new_frm_fp(&k, keyfp, 0, &origin, NULL);
	}
	if (result != LDNS_STATUS_OK) {
		fprintf(stderr, "Could not read public key from file %s: %s\n", keyname, ldns_get_errorstr_by_id(result));
		exit(EXIT_FAILURE);
	}
	fclose(keyfp);

	flags = ldns_read_uint16(ldns_rdf_data(ldns_rr_dnskey_flags(k)));
	flags |= LDNS_KEY_REVOKE_KEY;

	if (!ldns_rr_dnskey_set_flags(k,
		ldns_native2rdf_int16(LDNS_RDF_TYPE_INT16, flags)))
	{
		fprintf(stderr, "Revocation failed\n");
		exit(EXIT_FAILURE);
	}

	/* print the public key RR to .key */

	if (nofile)
		ldns_rr_print(stdout,k);
	else {
		keyfp = fopen(keyname, "w");
		if (!keyfp) {
			fprintf(stderr, "Unable to open %s: %s\n", keyname,
				strerror(errno));
			exit(EXIT_FAILURE);
		} else {
			ldns_rr_print(keyfp, k);
			fclose(keyfp);
			fprintf(stdout, "DNSKEY revoked\n");
		}
	}

	free(keyname);
	ldns_rr_free(k);

        exit(EXIT_SUCCESS);
}
