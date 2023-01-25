/*
 * read a zone from disk and split it up:
 *
 * zone: SOA a b c d e f g h i j k l 
 * becomes:
 * zone1: SOA a b c d e f
 * zone2: SOA f g h i k l
 *
 * ldns-catzone removes the last name and put
 * the zone back together.
 *
 * This way you can incremental sign a zone
 *
 * See the file LICENSE for the license
 */

#include "config.h"
#include <errno.h>
#include <ldns/ldns.h>

#define DEFAULT_SPLIT 	1000
#define FILE_SIZE 	255
#define SPLIT_MAX 	999 
#define NO_SPLIT 	0
#define INTENT_TO_SPLIT 1
#define SPLIT_NOW	2

static void
usage(FILE *f, char *progname)
{
		fprintf(f, "Usage: %s [OPTIONS] <zonefile> [keys]\n", progname);
		fprintf(f, "  Cut a zone file into pieces, each part is put in a file\n");
		fprintf(f, "  named: '<zonefile>.NNN'. Where NNN is a integer ranging 000 to 999.\n");
		fprintf(f, "  If key files are given they are inserted in each part.\n");
		fprintf(f, "  The original SOA is also included in each part, making them correct DNS\n");
		fprintf(f, "  (mini) zones.\n");
		fprintf(f, "  This utility can be used to parallel sign a large zone.\n");
		fprintf(f, "  To make it work the original zone needs to be canonical ordered.\n");
		fprintf(f, "\nOPTIONS:\n");
		fprintf(f, "  -n NUMBER\tsplit after this many RRs\n");
		fprintf(f, "  -o ORIGIN\tuse this as initial origin, for zones starting with @\n");
		fprintf(f, "  -z\t\tsort the zone prior to splitting. The current ldns zone\n");
		fprintf(f, "  \t\timplementation makes this unusable for large zones.\n");
		fprintf(f, "  -v\t\tshow version number and exit\n");
}


/* key the keys from the cmd line */
static ldns_rr_list *
open_keyfiles(char **files, uint16_t filec) 
{
	uint16_t i;
	ldns_rr_list *pubkeys;
	ldns_rr *k;
	FILE *kfp;

 	pubkeys = ldns_rr_list_new();
	
	for (i = 0; i < filec; i++) {
		if (!(kfp = fopen(files[i], "r"))) {
			fprintf(stderr, "Error opening key file %s: %s\n", files[i], strerror(errno));
			return NULL;
		}
		if (ldns_rr_new_frm_fp(&k, kfp, NULL, NULL, NULL) != LDNS_STATUS_OK) {
			fprintf(stderr, "Error parsing the key file %s: %s\n", files[i], strerror(errno));
			ldns_rr_list_deep_free(pubkeys);
			return NULL;
		}
		fclose(kfp);
		ldns_rr_list_push_rr(pubkeys, k);
	}
	return pubkeys;
}

/* open a new zone file with the correct suffix */
static FILE *
open_newfile(char *basename, ldns_zone *z, size_t counter, ldns_rr_list *keys)
{
	char filename[FILE_SIZE];
	FILE *fp;

	if (counter > SPLIT_MAX)  {
		fprintf(stderr, "Maximum split count reached %u\n", (unsigned int) counter);
		return NULL;
	}

	snprintf(filename, FILE_SIZE, "%s.%03u", basename, (unsigned int) counter);

	if (!(fp = fopen(filename, "w"))) {
		fprintf(stderr, "Cannot open zone %s: %s\n", filename, strerror(errno));
		return NULL;
	} else {
		fprintf(stderr, "%s\n", filename);
	}
	ldns_rr_print(fp, ldns_zone_soa(z));
	if (keys) {
		ldns_rr_list_print(fp, keys);

	}
	return fp;
}

int
main(int argc, char **argv)
{
	char *progname;
	FILE *fp;
	ldns_zone *z;
	ldns_rr_list *zrrs;
	ldns_rdf *lastname;
	int c; 
	int line_nr;
	size_t split;
	size_t i;
	int splitting;
	int compare;
	size_t file_counter;
	ldns_rdf *origin;
	ldns_rdf *current_rdf;
	ldns_rr *current_rr;
	ldns_rr_list *last_rrset;
	ldns_rr_list *pubkeys;
	bool sort;
	ldns_status s;

	progname = _strdup(argv[0]);
	split = 0;
	splitting = NO_SPLIT; 
	file_counter = 0;
	lastname = NULL;
	origin = NULL;
	last_rrset = ldns_rr_list_new();
	sort = false;

	while ((c = getopt(argc, argv, "n:o:zv")) != -1) {
		switch(c) {
			case 'n':
				split = (size_t)atoi(optarg);
				if (split == 0) {
					fprintf(stderr, "-n want a integer\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'o':
				origin = ldns_dname_new_frm_str(_strdup(optarg));
				if (!origin) {
					fprintf(stderr, "Cannot convert the origin %s to a domainname\n", optarg);
					exit(EXIT_FAILURE);
				}
				break;
			case 'v':
				printf("zone file splitter version %s (ldns version %s)\n", LDNS_VERSION, ldns_version());
				exit(EXIT_SUCCESS);
				break;
			case 'z':
				sort = true;
				break;
			default:
				fprintf(stderr, "Unrecognized option\n");
				usage(stdout, progname);
				exit(EXIT_FAILURE);
		}
	}
	if (split == 0) {
		split = DEFAULT_SPLIT;
	}
	
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		usage(stdout, progname);
		exit(EXIT_FAILURE);
	}

	if (!(fp = fopen(argv[0], "r"))) {
		fprintf(stderr, "Unable to open %s: %s\n", argv[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* get the keys */
	pubkeys = open_keyfiles(argv + 1, (uint16_t) argc - 1);
	
	/* suck in the entire zone ... */
	if (!origin) {
		origin = ldns_dname_new_frm_str(".");
	}
	
	s = ldns_zone_new_frm_fp_l(&z, fp, origin, 0, LDNS_RR_CLASS_IN, &line_nr);
	fclose(fp);

	if (s != LDNS_STATUS_OK) {
		fprintf(stderr, "Zone file %s could not be parsed correctly: %s at line %d\n", 
				argv[0],
				ldns_get_errorstr_by_id(s),
				line_nr);
		exit(EXIT_FAILURE);
	}
	/* these kind of things can kill you... */
	if (sort) {
		ldns_zone_sort(z);
	}

	zrrs = ldns_zone_rrs(z);
	if (ldns_rr_list_rr_count(zrrs) / split > SPLIT_MAX) {
		fprintf(stderr, "The zone is too large for the used -n value: %u\n", (unsigned int) split);
		exit(EXIT_FAILURE);
	}
	
	
	/* Setup */
	if (!(fp = open_newfile(argv[0], z, file_counter, pubkeys))) {
			exit(EXIT_FAILURE);
	}

	for(i = 0; i < ldns_rr_list_rr_count(zrrs); i++) {
	
		current_rr = ldns_rr_list_rr(zrrs, i);
		current_rdf = ldns_rr_owner(current_rr);

		compare = ldns_dname_compare(current_rdf, lastname);

		if (compare == 0) {
			ldns_rr_list_push_rr(last_rrset, current_rr);
		} 

		if (i > 0 && (i % split) == 0) {
			splitting = INTENT_TO_SPLIT;
		}

		if (splitting == INTENT_TO_SPLIT) { 
			if (compare != 0) {
				splitting = SPLIT_NOW;
			} 
		}

		if (splitting == SPLIT_NOW) {
			fclose(fp);

			lastname = NULL;
			splitting = NO_SPLIT;
			file_counter++;
			if (!(fp = open_newfile(argv[0], z, file_counter, pubkeys))) {
				exit(EXIT_FAILURE);
			}

			/* insert the last RRset in the new file */
			ldns_rr_list_print(fp, last_rrset);

			/* print the current rr */
			ldns_rr_print(fp, current_rr); 

			/* remove them */
			ldns_rr_list_free(last_rrset); 
			last_rrset = ldns_rr_list_new();
			/* add the current RR */
			ldns_rr_list_push_rr(last_rrset, current_rr);
			continue;
		}
		if (splitting == NO_SPLIT || splitting == INTENT_TO_SPLIT) {
			ldns_rr_print(fp, current_rr); 
		}
		if (compare != 0) {
			/* remove them and then add the current one */
			ldns_rr_list_free(last_rrset); 
			last_rrset = ldns_rr_list_new();
			ldns_rr_list_push_rr(last_rrset, current_rr);
		}
		lastname = current_rdf;
	}
	fclose(fp); 
        exit(EXIT_SUCCESS);
}
