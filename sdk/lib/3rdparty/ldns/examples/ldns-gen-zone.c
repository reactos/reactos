/*
 * Reads a zone file from disk and prints it to stdout, one RR per line.
 * Adds artificial DS records and RRs.
 * For the purpose of generating a test zone file
 *
 * (c) SIDN 2010/2011 - Marco Davids/Miek Gieben
 *
 * See the LICENSE file for the license
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <ldns/ldns.h>
#include <errno.h>

#define NUM_DS 4                /* maximum of 4 DS records per delegation */
#define ALGO 8			/* Algorithm to use for fake DS records - RSASHA256 - RFC5702  */
#define DIGESTTYPE 2		/* Digest type to use for fake DS records - SHA-256 - RFC 4509 */


/**
 * Usage function.
 *
 */
static void
usage(FILE *fp, char *prog) {
        fprintf(fp, "\n\nUsage: %s [-hsv] [-ap NUM] [-o ORIGIN] [<zonefile>]\n", prog);
        fprintf(fp, "\tReads a zonefile and add some artificial NS RRsets and DS records.\n");
        fprintf(fp, "\tIf no zonefile is given, the zone is read from stdin.\n");
        fprintf(fp, "\t-a <NUM> add NUM artificial delegations (NS RRSets) to output.\n");
        fprintf(fp, "\t-p <NUM> add NUM percent of DS RRset's to the NS RRsets (1-%d RR's per DS RRset).\n", NUM_DS);
        fprintf(fp, "\t-o ORIGIN sets an $ORIGIN, which can be handy if the one in the zonefile is set to @.\n");
        fprintf(fp, "\t-s if input zone file is already sorted and canonicalized (ie all lowercase),\n\t   use this option to speed things up while inserting DS records.\n");
        fprintf(fp, "\t-h show this text.\n");
        fprintf(fp, "\t-v shows the version and exits.\n");
        fprintf(fp, "\nif no file is given standard input is read.\n\n");
}

/**
 * Insert the DS records, return the amount added.
 *
 */
static int
insert_ds(ldns_rdf *dsowner, uint32_t ttl)
{
        int d, dsrand;
        int keytag = 0;
        char *dsownerstr;
        char digeststr[70];

        /**
         * Average the amount of DS records per delegation a little.
         */
        dsrand = 1+rand() % NUM_DS;
        for(d = 0; d < dsrand; d++) {
                keytag = 1+rand() % 65535;
                /**
                 * Dynamic hashes method below is still too slow... 20% slower than a fixed string...
                 *
                 * We assume RAND_MAX is 32 bit, http://www.gnu.org/s/libc/manual/html_node/ISO-Random.html
                 * 2147483647 or 0x7FFFFFFF
                 */
                snprintf(digeststr, 65,
                        "%08x%08x%08x%08x%08x%08x%08x%08x",
                        (unsigned) rand()%RAND_MAX, (unsigned) rand()%RAND_MAX, (unsigned) rand()%RAND_MAX,
                        (unsigned) rand()%RAND_MAX, (unsigned) rand()%RAND_MAX, (unsigned) rand()%RAND_MAX,
                        (unsigned) rand()%RAND_MAX, (unsigned) rand()%RAND_MAX);
                dsownerstr = ldns_rdf2str(dsowner);
                fprintf(stdout, "%s\t%u\tIN\tDS\t%d %d %d %s\n", dsownerstr, (unsigned) ttl, keytag, ALGO, DIGESTTYPE, digeststr);
        }
        return dsrand;
}

int
main(int argc, char **argv) {
        char *filename, *rrstr, *ownerstr;
        const char *classtypestr1 = "IN NS ns1.example.com.";
        const char *classtypestr2 = "IN NS ns2.example.com.";
        const size_t classtypelen = strlen(classtypestr1);
        /* Simply because this was developed by SIDN and we don't use xn-- for .nl :-) */
        const char *punystr = "xn--fake-rr";
        const size_t punylen = strlen(punystr);
        size_t rrstrlen, ownerlen;
        FILE *fp;
        int c, nsrand;
        uint32_t ttl;
        int counta,countd,countr;
        ldns_zone *z;
        ldns_rdf *origin = NULL;
        int line_nr = 0;
        int addrrs = 0;
        int dsperc = 0;
        bool canonicalize = true;
        bool sort = true;
        bool do_ds = false;
        ldns_status s;
        size_t i;
        ldns_rr_list *rrset_list;
        ldns_rdf *owner;
        ldns_rr_type cur_rr_type;
        ldns_rr *cur_rr;
        ldns_status status;

        counta = countd = countr = 0;

        /**
         * Set some random seed.
         */
        srand((unsigned int)time(NULL));

        /**
         * Commandline options.
         */
        while ((c = getopt(argc, argv, "a:p:shvo:")) != -1) {
                switch (c) {
        	    case 'a':
                        addrrs = atoi(optarg);
                        if (addrrs <= 0) {
                                fprintf(stderr, "error\n");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'o':
                        origin = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_DNAME, optarg);
                        if (!origin) {
                                fprintf(stderr, "error: creating origin from -o %s failed.\n", optarg);
                                exit(EXIT_FAILURE);
                        }
                        break;
                case 'p':
                        dsperc = atoi(optarg);
                        if (dsperc < 0 || dsperc > 100) {
                                fprintf(stderr, "error: percentage of signed delegations must be between [0-100].\n");
                                exit(EXIT_FAILURE);
                        }
                        do_ds = true;
                        break;
                case 's':
                        sort = false;
                        canonicalize = false;
                        break;
                case 'h':
                        usage(stdout, argv[0]);
                        exit(EXIT_SUCCESS);
                case 'v':
                        fprintf(stdout, "ldns-gen-zone version %s (ldns version %s)\n", LDNS_VERSION, ldns_version());
                        exit(EXIT_SUCCESS);
                default:
                        fprintf(stderr, "\nTry -h for more information.\n\n");
                        exit(EXIT_FAILURE);
	        }
        }
        argc -= optind;
        argv += optind;

        /**
         * Read zone.
         */
        if (argc == 0) {
                fp = stdin;
        } else {
                filename = argv[0];
                fp = fopen(filename, "r");
                if (!fp) {
	                fprintf(stderr, "Unable to open %s: %s\n", filename, strerror (errno));
                	exit(EXIT_FAILURE);
	        }
        }
        s = ldns_zone_new_frm_fp_l(&z, fp, origin, 0, LDNS_RR_CLASS_IN, &line_nr);
        if (s != LDNS_STATUS_OK) {
                fprintf(stderr, "%s at line %d\n", ldns_get_errorstr_by_id(s), line_nr);
                exit(EXIT_FAILURE);
        }
        if (!ldns_zone_soa(z)) {
                fprintf(stderr, "No zone data seen\n");
                exit(EXIT_FAILURE);
        }

        ttl = ldns_rr_ttl(ldns_zone_soa(z));
        if (!origin) {
                origin = ldns_rr_owner(ldns_zone_soa(z));
                // Check for root (.) origin here TODO(MG)
        }
        ownerstr = ldns_rdf2str(origin);
        if (!ownerstr) {
                fprintf(stderr, "ldns_rdf2str(origin) failed\n");
                exit(EXIT_FAILURE);
        }
        ownerlen = strlen(ownerstr);

        ldns_rr_print(stdout, ldns_zone_soa(z));

        if (addrrs > 0) {
                while (addrrs > counta) {
                        counta++;
                        rrstrlen = punylen + ownerlen + classtypelen + 4;
                        rrstrlen *= 2; /* estimate */
                        rrstr = (char*)malloc(rrstrlen);
                        if (!rrstr) {
                                fprintf(stderr, "malloc() failed: Out of memory\n");
                                exit(EXIT_FAILURE);
                        }
                        (void)snprintf(rrstr, rrstrlen, "%s%d.%s %u %s", punystr, counta,
                                ownerstr, (unsigned) ttl, classtypestr1);
                        status = ldns_rr_new_frm_str(&cur_rr, rrstr, 0, NULL, NULL);
                        if (status == LDNS_STATUS_OK) {
                                ldns_rr_print(stdout, cur_rr);
                                ldns_rr_free(cur_rr);
                        } else {
                                fprintf(stderr, "ldns_rr_new_frm_str() failed\n");
                                exit(EXIT_FAILURE);
                        }

                        (void)snprintf(rrstr, rrstrlen, "%s%d.%s %u %s", punystr, counta,
                                ownerstr, (unsigned) ttl, classtypestr2);
                        status = ldns_rr_new_frm_str(&cur_rr, rrstr, 0, NULL, NULL);
                        if (status == LDNS_STATUS_OK) {
                                ldns_rr_print(stdout, cur_rr);
                        } else {
                                fprintf(stderr, "ldns_rr_new_frm_str() failed\n");
                                exit(EXIT_FAILURE);
                        }

                        free(rrstr);

                        /* may we add a DS record as well? */
                        if (do_ds) {
                                /*
                                 * Per definition this may not be the same as the origin, so no
                                 * check required same for NS check - so the only thing left is some
                                 * randomization.
                                 */
                                nsrand = rand() % 100;
                                if (nsrand < dsperc) {
                                        owner = ldns_rr_owner(cur_rr);
                                        ttl = ldns_rr_ttl(cur_rr);
                                        countd += insert_ds(owner, ttl);
                                }
                        }
                        ldns_rr_free(cur_rr);
                }
        }

        if (!do_ds) {
                ldns_rr_list_print(stdout, ldns_zone_rrs(z));
        } else {
                /*
                * We use dns_rr_list_pop_rrset and that requires a sorted list weird things may happen
                * if the -s option was used on unsorted, non-canonicalized input
                */
                if (canonicalize) {
                        ldns_rr2canonical(ldns_zone_soa(z));
                        for (i = 0; i < ldns_rr_list_rr_count(ldns_zone_rrs(z)); i++) {
                                ldns_rr2canonical(ldns_rr_list_rr(ldns_zone_rrs(z), i));
                        }
                }

                if (sort) {
                        ldns_zone_sort(z);
                }

                /* Work on a per RRset basis for DS records - weird things will happen if the -s option
                 * was used in combination with an unsorted zone file
                 */
                while((rrset_list = ldns_rr_list_pop_rrset(ldns_zone_rrs(z)))) {
                        owner = ldns_rr_list_owner(rrset_list);
                        cur_rr_type = ldns_rr_list_type(rrset_list);
                        /**
                         * Print them...
                         */
                        cur_rr = ldns_rr_list_pop_rr(rrset_list);
                        while (cur_rr) {
                                ttl = ldns_rr_ttl(cur_rr);
                                fprintf(stdout, "%s", ldns_rr2str(cur_rr));
                                cur_rr = ldns_rr_list_pop_rr(rrset_list);
                        }
                        /*
                         * And all the way at the end a DS record if
                         * we are dealing with an NS rrset
                         */
                        nsrand = rand() % 100;
                        if (nsrand == 0) {
                                nsrand = 100;
                        }

                        if ((cur_rr_type == LDNS_RR_TYPE_NS) &&
                                (ldns_rdf_compare(owner, origin) != 0) && (nsrand < dsperc)) {
                                /**
                                 * No DS records for the $ORIGIN, only for delegations, obey dsperc.
                                 */
                                countr++;
                                countd += insert_ds(owner, ttl);
                        }
                        ldns_rr_list_free(rrset_list);
                        ldns_rdf_free(owner);
                }
        }

        /**
         * And done...
         */
        fclose(fp);
        fprintf(stdout, ";; Added %d DS records (percentage was %d) to %d NS RRset's (from input-zone: %d, from added: %d)\n;; lines in original input-zone: %d\n",
                countd, dsperc, counta + countr, countr, counta, line_nr);
        exit(EXIT_SUCCESS);
}
