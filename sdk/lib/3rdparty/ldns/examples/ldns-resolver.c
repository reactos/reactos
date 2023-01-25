/*
 * ldns-resolver tries to create a resolver structure from /dev/urandom
 * this is only useful to test the library for robustness with input data
 *
 * (c) NLnet Labs 2006 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"
#include "errno.h"

#include <ldns/ldns.h>

int
main(int argc, char **argv) {

	ldns_resolver *r;
	int line = 1;
	FILE *rand;
	ldns_status s;

	if (argc != 2 || strncmp(argv[1], "-h", 3) == 0) {
		printf("Usage: ldns-resolver <file>\n");
		printf("Tries to create a stub resolver structure from the given file.\n");
		exit(EXIT_FAILURE);
	}
	
	if (!(rand = fopen(argv[1], "r"))) {
		printf("Error opening %s: %s\n", argv[1], strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("Trying to read from %s\n", argv[1]);
	s = ldns_resolver_new_frm_fp_l(&r, rand, &line);
	if (s != LDNS_STATUS_OK) {
		printf("Failed: %s at line %d\n", ldns_get_errorstr_by_id(s), line);
		exit(EXIT_FAILURE);
	} else {
		printf("Success\n");
		ldns_resolver_print(stdout, r);
		ldns_resolver_deep_free(r);
	}

	fclose(rand);

	return EXIT_SUCCESS;
}
