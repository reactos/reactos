/*
 * ldns-signzone signs a zone file
 * 
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"
#include <stdlib.h>
#include <unistd.h>

#include <errno.h>

#include <time.h>

#include <ldns/ldns.h>
#include <ldns/keys.h>

#ifdef HAVE_SSL
#include <openssl/conf.h>
#include <openssl/engine.h>
#endif /* HAVE_SSL */

#define MAX_FILENAME_LEN 250
int verbosity = 1;

static void
usage(FILE *fp, const char *prog) {
	fprintf(fp, "%s [OPTIONS] <domain name>\n", prog);
	fprintf(fp, "  prints the NSEC3 hash of the given domain name\n");
	fprintf(fp, "-a [algorithm] hashing algorithm\n");
	fprintf(fp, "-t [number] number of hash iterations\n");
	fprintf(fp, "-s [string] salt\n");
}

int
main(int argc, char *argv[])
{
	ldns_rdf *dname, *hashed_dname;
	uint8_t nsec3_algorithm = 1;
	size_t nsec3_iterations_cmd = 1;
	uint16_t nsec3_iterations = 1;
	uint8_t nsec3_salt_length = 0;
	uint8_t *nsec3_salt = NULL;
	
	char *prog = _strdup(argv[0]);

	int c;
	while ((c = getopt(argc, argv, "a:s:t:")) != -1) {
		switch (c) {
		case 'a':
			nsec3_algorithm = (uint8_t) atoi(optarg);
			break;
		case 's':
			if (strlen(optarg) % 2 != 0) {
				fprintf(stderr, "Salt value is not valid hex data, not a multiple of 2 characters\n");
				exit(EXIT_FAILURE);
			}
			if (strlen(optarg) > 512) {
				fprintf(stderr, "Salt too long\n");
				exit(EXIT_FAILURE);
			}
			if (nsec3_salt) LDNS_FREE(nsec3_salt);
			nsec3_salt_length = (uint8_t) (strlen(optarg) / 2);
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
		dname = ldns_dname_new_frm_str(argv[0]);
		if (!dname) {
			free(prog);
			if (nsec3_salt) free(nsec3_salt);
			fprintf(stderr,
			        "Error: unable to parse domain name\n");
			return EXIT_FAILURE;
		}
		hashed_dname = ldns_nsec3_hash_name(dname,
		                                    nsec3_algorithm,
		                                    nsec3_iterations,
		                                    nsec3_salt_length,
		                                    nsec3_salt);
		if (!hashed_dname) {
			free(prog);
			if (nsec3_salt) free(nsec3_salt);
			fprintf(stderr,
			        "Error creating NSEC3 hash\n");
			return EXIT_FAILURE;
		}
		ldns_rdf_print(stdout, hashed_dname);
		printf("\n");
		ldns_rdf_deep_free(dname);
		ldns_rdf_deep_free(hashed_dname);
	}

	if (nsec3_salt) {
		free(nsec3_salt);
	}

	free(prog);
	
	return EXIT_SUCCESS;
}
