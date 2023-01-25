/*
 * ldns-rrsig prints out the inception and expiration dates in a more readable 
 * way than the normal RRSIG presentation format
 *
 * for a particularly domain
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"

#include <ldns/ldns.h>

static int
usage(FILE *fp, char *prog) {
	fprintf(fp, "%s domain [type]\n", prog);
	fprintf(fp, "  print out the inception and expiration dates\n");
	fprintf(fp, "  in a more human readable form\n");
	fprintf(fp, "  <type>\tquery for RRSIG(<type>), defaults to SOA\n");
	return 0;
}

int
main(int argc, char *argv[])
{
	ldns_resolver *res;
	ldns_resolver *localres;
	ldns_rdf *domain;
	ldns_pkt *p;
	ldns_rr_list *rrsig;
	ldns_rr_list *rrsig_type;
	ldns_rr_list *ns;
	ldns_rr_list *ns_ip;
	uint8_t i, j;
	ldns_rr_type t;
	const char * type_name;
	struct tm incep, expir;
	char incep_buf[26];
	char expir_buf[26];
	ldns_status s;
	time_t now = time(NULL);
	
	p = NULL;
	rrsig = NULL;
	rrsig_type = NULL;
	domain = NULL;

	/* option parsing */
	
	if (argc < 2) {
		usage(stdout, argv[0]);
		exit(EXIT_FAILURE);
	} else {
		/* create a rdf from the command line arg */
		domain = ldns_dname_new_frm_str(argv[1]);
		if (!domain) {
			usage(stdout, argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (argc == 3) {
		/* optional type arg */
		type_name = argv[2];
		t = ldns_rdf2rr_type(
			ldns_rdf_new_frm_str(LDNS_RDF_TYPE_TYPE, type_name));
		if (t == 0) {
			fprintf(stderr, " *** %s is not a valid RR type\n", type_name);
			exit(EXIT_FAILURE);
		}
	} else {
		t = LDNS_RR_TYPE_SOA; 
		type_name = "SOA";
	}

	/* create a new resolver from /etc/resolv.conf */
	s = ldns_resolver_new_frm_file(&localres, NULL);
	if (s != LDNS_STATUS_OK) {
		exit(EXIT_FAILURE);
	}

	/* first get the nameserver of the domain in question */
	p = ldns_resolver_query(localres, domain, LDNS_RR_TYPE_NS,
				LDNS_RR_CLASS_IN, LDNS_RD);
	if (!p) {
		fprintf(stderr," *** Could not find any nameserver for %s", argv[1]);
		ldns_resolver_deep_free(localres);
		exit(EXIT_FAILURE);
	}
	ns = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_NS, LDNS_SECTION_ANSWER);

	if (!ns) {
		fprintf(stderr," *** Could not find any nameserver for %s", argv[1]);
		ldns_pkt_free(p);
		ldns_resolver_deep_free(localres);
		exit(EXIT_FAILURE);
	}

	/* use our local resolver to resolv the names in the for usage in our
	 * new resolver */
	res = ldns_resolver_new();
	if (!res) {
		ldns_pkt_free(p);
		ldns_resolver_deep_free(localres);
		ldns_rr_list_deep_free(ns);
		exit(EXIT_FAILURE);
	}
	for(i = 0; i < ldns_rr_list_rr_count(ns); i++) {
		ns_ip = ldns_get_rr_list_addr_by_name(localres,
			ldns_rr_ns_nsdname(ldns_rr_list_rr(ns, i)),
			LDNS_RR_CLASS_IN, LDNS_RD);
		/* add these to new resolver */
		for(j = 0; j < ldns_rr_list_rr_count(ns_ip); j++) {
			if (ldns_resolver_push_nameserver(res,
				ldns_rr_a_address(ldns_rr_list_rr(ns_ip, j))) != LDNS_STATUS_OK) {
				printf("Error adding nameserver to resolver\n");
                		ldns_pkt_free(p);
                		ldns_resolver_deep_free(res);
                                ldns_resolver_deep_free(localres);
                		ldns_rr_list_deep_free(ns);
				exit(EXIT_FAILURE);
			}
		}
       		ldns_rr_list_deep_free(ns_ip);

	}

	/* enable DNSSEC */
	ldns_resolver_set_dnssec(res, true);
	/* also set CD, we want EVERYTHING! */
	ldns_resolver_set_dnssec_cd(res, true);

	/* use the resolver to send it a query for the soa
	 * records of the domain given on the command line
	 */
	ldns_pkt_free(p);
	p = ldns_resolver_query(res, domain, LDNS_RR_TYPE_RRSIG, LDNS_RR_CLASS_IN, LDNS_RD);

	ldns_rdf_deep_free(domain);
	
        if (!p)  {
		ldns_resolver_deep_free(localres);
    		ldns_rr_list_deep_free(ns);
		exit(EXIT_FAILURE);
        } else {
		/* retrieve the RRSIG records from the answer section of that
		 * packet
		 */
		rrsig = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_RRSIG, LDNS_SECTION_ANSWER);
		if (!rrsig) {
			fprintf(stderr, 
					" *** invalid answer name %s after RRSIG query for %s\n",
					argv[1], argv[1]);
                        ldns_pkt_free(p);
                        ldns_resolver_deep_free(res);
        		ldns_rr_list_deep_free(ns);
			exit(EXIT_FAILURE);
		} else {
			rrsig_type = ldns_rr_list_new();

			for(i = 0; i < ldns_rr_list_rr_count(rrsig); i++) {
				if (ldns_rdf2rr_type(
					ldns_rr_rrsig_typecovered(
					ldns_rr_list_rr(rrsig, i))) == t) {
					ldns_rr_list_push_rr(rrsig_type,
						ldns_rr_list_rr(rrsig, i));
				}
			}
			if (ldns_rr_list_rr_count(rrsig_type) == 0) {
				fprintf(stderr, " *** No RRSIG(%s) type found\n",
					type_name);
                		ldns_resolver_deep_free(localres);
                		ldns_resolver_deep_free(res);
                		ldns_pkt_free(p);
                		ldns_rr_list_deep_free(ns);
                		ldns_rr_list_free(rrsig);
                		ldns_rr_list_deep_free(rrsig_type);
				exit(EXIT_FAILURE);
			}
			
			for(i = 0; i < ldns_rr_list_rr_count(rrsig_type); i++) {
				memset(&incep, 0, sizeof(incep));
				if (ldns_serial_arithmetics_gmtime_r(
						ldns_rdf2native_time_t(
						ldns_rr_rrsig_inception(
						ldns_rr_list_rr(rrsig_type, i))),
					       	now, &incep
					)
				    && asctime_r(&incep, incep_buf)) {
					incep_buf[24] = '\0';
				} else {
					incep_buf[0] = '\0';
				}
				memset(&expir, 0, sizeof(expir));
				if (ldns_serial_arithmetics_gmtime_r(
						ldns_rdf2native_time_t(
						ldns_rr_rrsig_expiration(
						ldns_rr_list_rr(rrsig_type, i))),
					       	now, &expir
					)
				    && asctime_r(&expir, expir_buf)) {
					expir_buf[24] = '\0';
				} else {
					expir_buf[0] = '\0';
				}

				fprintf(stdout, "%s RRSIG(%s):  %s - %s\n",
					argv[1], type_name, incep_buf, expir_buf);
			}
                        ldns_rr_list_free(rrsig);
                        ldns_rr_list_deep_free(rrsig_type);
		}
        }
        ldns_pkt_free(p);
        ldns_resolver_deep_free(localres);
        ldns_resolver_deep_free(res);
        ldns_rr_list_deep_free(ns);
        return 0;
}
