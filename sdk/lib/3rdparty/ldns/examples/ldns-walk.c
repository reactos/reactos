/*
 * ldns-walk uses educated guesses and NSEC data to retrieve the
 * contents of a dnssec signed zone
 *
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

#include "config.h"

#include <ldns/ldns.h>

int verbosity = 0;

static int
usage(FILE *fp, char *prog) {
	fprintf(fp, "%s [options] domain\n", prog);
	fprintf(fp, "  print out the owner names for domain and the record types for those names\n");
	fprintf(fp, "OPTIONS:\n");
	fprintf(fp, "-4\t\tonly use IPv4\n");
	fprintf(fp, "-6\t\tonly use IPv6\n");
	fprintf(fp, "-f\t\tfull; get all rrsets instead of only a list of names and types\n");
	fprintf(fp, "-s <name>\t\tStart from this name\n");
	fprintf(fp, "-v <verbosity>\t\tVerbosity level [1-5]\n");
	fprintf(fp, "-version\tShow version and exit\n");
	fprintf(fp, "@<nameserver>\t\tUse this nameserver\n");
	return 0;
}

static ldns_rdf *
create_dname_plus_1(ldns_rdf *dname)
{
	uint8_t *wire;
	ldns_rdf *newdname;
	uint8_t labellen;
	size_t pos;
	ldns_status status;
	size_t i;
	
	ldns_dname2canonical(dname);
	labellen = ldns_rdf_data(dname)[0];
	if (verbosity >= 3) {
                printf("Create +e for ");
                ldns_rdf_print(stdout, dname);
                printf("\n");
	}
	if (labellen < 63) {
		wire = malloc(ldns_rdf_size(dname) + 1);
		if (!wire) {
			fprintf(stderr, "Malloc error: out of memory?\n");
			exit(127);
		}
		wire[0] = labellen + 1;
		memcpy(&wire[1], ldns_rdf_data(dname) + 1, labellen);
		memcpy(&wire[labellen+1], ldns_rdf_data(dname) + labellen, ldns_rdf_size(dname) - labellen);
		wire[labellen+1] = (uint8_t) '\000';
		pos = 0;
		status = ldns_wire2dname(&newdname, wire, ldns_rdf_size(dname) + 1, &pos);
		free(wire);
	} else {
		wire = malloc(ldns_rdf_size(dname));
		if (!wire) {
			fprintf(stderr, "Malloc error: out of memory?\n");
			exit(127);
		}
		wire[0] = labellen;
		memcpy(&wire[1], ldns_rdf_data(dname) + 1, labellen);
		memcpy(&wire[labellen], ldns_rdf_data(dname) + labellen, ldns_rdf_size(dname) - labellen);
		i = labellen;
		while (wire[i] == 255) {
			if (i == 0) {
				printf("Error, don't know how to add 1 to a label with maximum length and all values on 255\n");
				exit(9);
			} else {
				i--;
			}
		}
		wire[i] = wire[i] + 1;
		pos = 0;
		status = ldns_wire2dname(&newdname, wire, ldns_rdf_size(dname) + 1, &pos);
		free(wire);
	}
	if (verbosity >= 3) {
		printf("result: ");
		ldns_rdf_print(stdout, newdname);
		printf("\n");
	}

	if (status != LDNS_STATUS_OK) {
	  printf("Error: %s\n", ldns_get_errorstr_by_id(status));
	  exit(10);
        }
	
	return newdname;
}

static ldns_rdf *
create_plus_1_dname(ldns_rdf *dname)
{
	ldns_rdf *label;
	ldns_status status;
	
	if (verbosity >= 3) {
	  printf("Creating n+e for: ");
	  ldns_rdf_print(stdout, dname);
	  printf("\n");
        }
	
	ldns_dname2canonical(dname);
	status = ldns_str2rdf_dname(&label, "\\000");
	if (status != LDNS_STATUS_OK) {
		printf("error creating \\000 dname: %s\n\n", ldns_get_errorstr_by_id(status));
		exit(2);
	}
	status = ldns_dname_cat(label, dname);
	if (status != LDNS_STATUS_OK) {
		printf("error catting \\000 dname: %s\n\n", ldns_get_errorstr_by_id(status));
		exit(3);
	}
	return label;
}

static void 
query_type_bitmaps(ldns_resolver *res, 
                   uint16_t res_flags,
                   const ldns_rdf *name,
                   const ldns_rdf *rdf)
{
	/* Note: this code is duplicated in higher.c in 
	 * ldns_nsec_type_check() function
	 */
	uint8_t window_block_nr;
	uint8_t bitmap_length;
	uint16_t type;
	uint16_t pos = 0;
	uint16_t bit_pos;
	uint8_t *data = ldns_rdf_data(rdf);
	
	ldns_pkt *answer_pkt;
	char *errstr;
	
	if (verbosity >= 3) {
		printf("Getting Resource Records covered by NSEC at ");
		ldns_rdf_print(stdout, name);
		printf("\n");
	}
	
	while(pos < ldns_rdf_size(rdf)) {
		window_block_nr = data[pos];
		bitmap_length = data[pos + 1];
		pos += 2;
		
		for (bit_pos = 0; bit_pos < (bitmap_length) * 8; bit_pos++) {
			if (ldns_get_bit(&data[pos], bit_pos)) {
				type = 256 * (uint16_t) window_block_nr + bit_pos;
				/* skip nsec and rrsig */
				if (type != LDNS_RR_TYPE_NSEC &&
				    type != LDNS_RR_TYPE_RRSIG) {
				    if (verbosity >=  3) {
						printf("querying for:\n");
						ldns_rdf_print(stdout, name);
						printf(" type %u\n", (unsigned int) type);
					}
					answer_pkt = ldns_resolver_query(res, name, type,
					                                 LDNS_RR_CLASS_IN,
					                                 res_flags);
					if (answer_pkt) {
						if (verbosity >= 5) {
							ldns_pkt_print(stdout, answer_pkt);
						}
						/* hmm, this does not give us the right records
						 * when asking for type NS above the delegation
						 * (or, in fact, when the delegated zone is 
						 * served by this server either)
						 * do we need to special case NS like NSEC?
						 * or can we fix the query or the answer reading?
						 * ...
						 */
						ldns_rr_list_print(stdout,
						                   ldns_pkt_answer(answer_pkt));
						ldns_pkt_free(answer_pkt);
					} else {
						printf("Query error, bailing out\n");
						printf("Failed at ");
						ldns_rdf_print(stdout, name);
						errstr = ldns_rr_type2str(type);
						printf(" %s\n", errstr);
						free(errstr);
						exit(1);
					}
				}
			}
		}
		pos += (uint16_t) bitmap_length;
	}
}


int
main(int argc, char *argv[])
{
	ldns_status status;

	ldns_resolver *res;
	ldns_rdf *domain = NULL;
	ldns_pkt *p;
	ldns_rr *soa;
	ldns_rr_list *rrlist;
	ldns_rr_list *rrlist2;
	ldns_rr_list *nsec_sigs = NULL;
	ldns_rdf *soa_p1;
	ldns_rdf *next_dname;
	ldns_rdf *last_dname;
	ldns_rdf *last_dname_p;
	ldns_rdf *startpoint = NULL;
	ldns_rr *nsec_rr = NULL;
	const char* arg_domain = NULL;
	int full = 0;

	char *serv = NULL;
	ldns_rdf *serv_rdf;
	ldns_resolver *cmdline_res;
	ldns_rr_list *cmdline_rr_list;
	ldns_rdf *cmdline_dname;

	uint8_t fam = LDNS_RESOLV_INETANY;
	int result = 0;
	int i;
	char *arg_end_ptr = NULL;
	size_t j;

	p = NULL;
	rrlist = NULL;
	rrlist2 = NULL;
	soa = NULL;
	domain = NULL;
	
	if (argc < 2) {
		usage(stdout, argv[0]);
		exit(EXIT_FAILURE);
	} else {
		for (i = 1; i < argc; i++) {
			if (strncmp(argv[i], "-4", 3) == 0) {
                        	if (fam != LDNS_RESOLV_INETANY) {
                                	fprintf(stderr, "You can only specify one of -4 or -6\n");
                                	exit(1);
                        	}
                        	fam = LDNS_RESOLV_INET;
			} else if (strncmp(argv[i], "-6", 3) == 0) {
                        	if (fam != LDNS_RESOLV_INETANY) {
                                	fprintf(stderr, "You can only specify one of -4 or -6\n");
                                	exit(1);
                        	}
                        	fam = LDNS_RESOLV_INET6;
			} else if (strncmp(argv[i], "-f", 3) == 0) {
				full = true;
			} else if (strncmp(argv[i], "-s", 3) == 0) {
				if (i + 1 < argc) {
					if (ldns_str2rdf_dname(&startpoint, argv[i + 1]) != LDNS_STATUS_OK) {
						printf("Bad start point name: %s\n", argv[i + 1]);
						exit(1);
					}
				} else {
					printf("Missing argument for -s\n");
					exit(1);
				}
				i++;
			} else if (strncmp(argv[i], "-v", 3) == 0) {
				if (i + 1 < argc) {
					verbosity = strtol(argv[i+1], &arg_end_ptr, 10);
					if (*arg_end_ptr != '\0') {
						printf("Bad argument for -v: %s\n", argv[i+1]);
						exit(1);
					}
				} else {
					printf("Missing argument for -v\n");
					exit(1);
				}
				i++;
			} else if (strcmp("-version", argv[i]) == 0) {
				printf("dns zone walker, version %s (ldns version %s)\n", LDNS_VERSION, ldns_version());
				goto exit;
			} else {
                        	if (argv[i][0] == '@') {
					if (strlen(argv[i]) == 1) {
						if (i + 1 < argc) {
							serv = argv[i + 1];
							i++;
						} else {
							printf("Missing argument for -s\n");
							exit(1);
						}
					} else {
						serv = argv[i] + 1;
					}
                        	} else {
					if (i < argc) {
						if (!domain) {
							/* create a rdf from the command line arg */
							arg_domain = argv[i];
							domain = ldns_dname_new_frm_str(arg_domain);
							if (!domain) {
								usage(stdout, argv[0]);
								exit(1);
							}
						} else {
							printf("One domain at a time please\n");
							exit(1);
						}
					} else {
						printf("No domain given to walk\n");
						exit(1);
					}
				}
			}
		}
	}
	if (!domain) {
		printf("Missing argument\n");
		exit(1);
	}


	/* create a new resolver from /etc/resolv.conf */
	if(!serv) {
		if (ldns_resolver_new_frm_file(&res, NULL) != LDNS_STATUS_OK) {
			fprintf(stderr, "%s", "Could not create resolver obj");
			result = EXIT_FAILURE;
			goto exit;
		}
	} else {
		res = ldns_resolver_new();
		if (!res || strlen(serv) <= 0) {
			result = EXIT_FAILURE;
			goto exit;
		}
		/* add the nameserver */
		serv_rdf = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_A, serv);
        	if (!serv_rdf) {
			/* maybe ip6 */
			serv_rdf = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_AAAA, serv);
		}
		if (!serv_rdf) {
			/* try to resolv the name if possible */
			status = ldns_resolver_new_frm_file(&cmdline_res, NULL);
			
			if (status != LDNS_STATUS_OK) {
				fprintf(stderr, "%s", "@server ip could not be converted");
				result = EXIT_FAILURE;
				goto exit;
			}

			cmdline_dname = ldns_dname_new_frm_str(serv);
			cmdline_rr_list = ldns_get_rr_list_addr_by_name(
						cmdline_res, 
						cmdline_dname,
						LDNS_RR_CLASS_IN,
						0);
			ldns_rdf_deep_free(cmdline_dname);
                        ldns_resolver_deep_free(cmdline_res);
			if (!cmdline_rr_list) {
				fprintf(stderr, "%s %s", "could not find any address for the name: ", serv);
				result = EXIT_FAILURE;
				goto exit;
			} else {
				if (ldns_resolver_push_nameserver_rr_list(
						res, 
						cmdline_rr_list
					) != LDNS_STATUS_OK) {
					fprintf(stderr, "%s", "pushing nameserver");
					result = EXIT_FAILURE;
					ldns_rr_list_deep_free(cmdline_rr_list);
					goto exit;
				}
                                ldns_rr_list_deep_free(cmdline_rr_list);
			}
		} else {
			if (ldns_resolver_push_nameserver(res, serv_rdf) != LDNS_STATUS_OK) {
				fprintf(stderr, "%s", "pushing nameserver");
				result = EXIT_FAILURE;
				goto exit;
			} else {
				ldns_rdf_deep_free(serv_rdf);
			}
		}

	}

	ldns_resolver_set_dnssec(res, true);
	ldns_resolver_set_dnssec_cd(res, true);
	ldns_resolver_set_ip6(res, fam);	

	if (!res) {
		exit(2);
	}

	/* use the resolver to send it a query for the soa
	 * records of the domain given on the command line
	 */
	if (verbosity >= 3) {
		printf("\nQuerying for: ");
		ldns_rdf_print(stdout, domain);
		printf("\n");
	}
	p = ldns_resolver_query(res, domain, LDNS_RR_TYPE_SOA, LDNS_RR_CLASS_IN, LDNS_RD);
	soa = NULL;
	if (verbosity >= 5) {
		if (p) {
			ldns_pkt_print(stdout, p);
		} else {
			fprintf(stdout, "No Packet Received from ldns_resolver_query()\n");
		}
	}

        if (!p)  {
		exit(3);
        } else {
		/* retrieve the MX records from the answer section of that
		 * packet
		 */
		rrlist = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_SOA, LDNS_SECTION_ANSWER);
		if (!rrlist || ldns_rr_list_rr_count(rrlist) != 1) {
			if (rrlist) {
				printf(" *** > 1 SOA: %u\n", (unsigned int) ldns_rr_list_rr_count(rrlist));
			} else {
				printf(" *** No rrlist...\b");
			}
			/* TODO: conversion memory */
			fprintf(stderr, 
					" *** invalid answer name after SOA query for %s\n",
					arg_domain);
			ldns_pkt_print(stdout, p);
			ldns_pkt_free(p);
			ldns_resolver_deep_free(res);
			exit(4);
		} else {
			soa = ldns_rr_clone(ldns_rr_list_rr(rrlist, 0));
			ldns_rr_list_deep_free(rrlist);
			rrlist = NULL;
			/* check if zone contains DNSSEC data */
			rrlist = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_RRSIG, LDNS_SECTION_ANSWER);
			if (!rrlist) {
				printf("No DNSSEC data received; either the zone is not secured or you should query it directly (with @nameserver)\n");
				ldns_pkt_free(p);
				ldns_resolver_deep_free(res);
				exit(5);
			}
			ldns_rr_list_deep_free(rrlist);
		}
        }

	/* add \001 to soa */
	status = ldns_str2rdf_dname(&soa_p1, "\\000");
	if (status != LDNS_STATUS_OK) {
		printf("error: %s\n", ldns_get_errorstr_by_id(status));
	}
	if (!soa) {
		printf("Error getting SOA\n");
		exit(1);
	}

	if (startpoint) {
		last_dname = startpoint;
		last_dname_p = create_dname_plus_1(last_dname);
	} else {
		last_dname = ldns_rdf_clone(domain);
		if (ldns_dname_cat(soa_p1, last_dname) != LDNS_STATUS_OK) {
			printf("Error concatenating dnames\n");
			exit(EXIT_FAILURE);
		}
		last_dname_p = ldns_rdf_clone(soa_p1);
	}

	if (!full) {
		ldns_rdf_print(stdout, ldns_rr_owner(soa));
		printf("\t");
	}
	
	next_dname = NULL;
	while (!next_dname || ldns_rdf_compare(next_dname, domain) != 0) {
		if (p) {
			ldns_pkt_free(p);
			p = NULL;
		}
		if (verbosity >= 4) {
			printf("Querying for: ");
			ldns_rdf_print(stdout, last_dname_p);
			printf("\n");
		}
		p = ldns_resolver_query(res, last_dname_p, LDNS_RR_TYPE_DS, LDNS_RR_CLASS_IN, LDNS_RD);
		if (verbosity >= 5) {
			if (p) {
				ldns_pkt_print(stdout, p);
			} else {
				fprintf(stdout, "No Packet Received from ldns_resolver_query()\n");
			}
		}

		if (next_dname) {
			ldns_rdf_deep_free(next_dname);
			ldns_rr_free(nsec_rr);
			next_dname = NULL;
			nsec_rr = NULL;
		}

		if (!p)  {
			fprintf(stderr, "Error trying to resolve: ");
			ldns_rdf_print(stderr, last_dname_p);
			fprintf(stderr, "\n");
			while (!p) {
				if (verbosity >= 3) {
					printf("Querying for: ");
					ldns_rdf_print(stdout, last_dname_p);
					printf("\n");
				}
				p = ldns_resolver_query(res, last_dname_p, LDNS_RR_TYPE_DS, LDNS_RR_CLASS_IN, LDNS_RD);
				/* TODO: make a general option for this (something like ignore_rtt)? */
				for (j = 0; j < ldns_resolver_nameserver_count(res); j++) {
					if (ldns_resolver_nameserver_rtt(res, j) != 0) {
						ldns_resolver_set_nameserver_rtt(res, j, LDNS_RESOLV_RTT_MIN);
					}
				}
				if (verbosity >= 5) {
					if (p) {
						ldns_pkt_print(stdout, p);
					} else {
						fprintf(stdout, "No Packet Received from ldns_resolver_query()\n");
					}
				}
			}
		}

		/* if the current name is an empty non-terminal, bind returns
		 * SERVFAIL on the plus1-query...
		 * so requery with only the last dname
		 */
		if (ldns_pkt_get_rcode(p) == LDNS_RCODE_SERVFAIL) {
			ldns_pkt_free(p);
			p = NULL;
			if (verbosity >= 3) {
				printf("Querying for: ");
				ldns_rdf_print(stdout, last_dname);
				printf("\n");
			}
			p = ldns_resolver_query(res, last_dname, LDNS_RR_TYPE_DS, LDNS_RR_CLASS_IN, LDNS_RD);
			if (verbosity >= 5) {
				if (p) {
					ldns_pkt_print(stdout, p);
				} else {
					fprintf(stdout, "No Packet Received from ldns_resolver_query()\n");
				}
			}

			if (!p) {
				exit(51);
			}
			rrlist = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_NSEC, LDNS_SECTION_AUTHORITY);
			rrlist2 = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_NSEC, LDNS_SECTION_ANSWER);
		} else {
			rrlist = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_NSEC, LDNS_SECTION_AUTHORITY);
			rrlist2 = ldns_pkt_rr_list_by_type(p, LDNS_RR_TYPE_NSEC, LDNS_SECTION_ANSWER);
		}
	if (rrlist && rrlist2) {
		ldns_rr_list_cat(rrlist, rrlist2);
	} else if (rrlist2) {
		rrlist = rrlist2;
	}

	if (!rrlist || ldns_rr_list_rr_count(rrlist) < 1) {
		if (!rrlist) {
			fflush(stdout);
			fprintf(stderr, "Zone does not seem to be DNSSEC secured,"
			                "or it uses NSEC3.\n");
			fflush(stderr);
			goto exit;
		}
	} else {
		/* find correct nsec */
		next_dname = NULL;
		for (j = 0; j < ldns_rr_list_rr_count(rrlist); j++) {
			if (ldns_nsec_covers_name(ldns_rr_list_rr(rrlist, j), last_dname_p)) {
				if (verbosity >= 3) {
					printf("The domain name: ");
					ldns_rdf_print(stdout, last_dname_p);
					printf("\nis covered by NSEC: ");
					ldns_rr_print(stdout, ldns_rr_list_rr(rrlist, j));
				}
				next_dname = ldns_rdf_clone(ldns_rr_rdf(ldns_rr_list_rr(rrlist, j), 0));
				nsec_rr = ldns_rr_clone(ldns_rr_list_rr(rrlist, j));
				nsec_sigs = ldns_dnssec_pkt_get_rrsigs_for_name_and_type(p, ldns_rr_owner(nsec_rr), LDNS_RR_TYPE_NSEC);
			} else {
				if (verbosity >= 4) {
					printf("\n");
					ldns_rdf_print(stdout, last_dname_p);
					printf("\nNOT covered by NSEC: ");
					ldns_rr_print(stdout, ldns_rr_list_rr(rrlist, j));
					printf("\n");
				}
			}
		}
		if (!next_dname) {
			printf("Error no nsec for ");
			ldns_rdf_print(stdout, last_dname);
			printf("\n");
			exit(1);
		}
		ldns_rr_list_deep_free(rrlist);
	}
	if (!next_dname) {
		/* apparently the zone also has prepended data (i.e. a.example and www.a.example, 
 		 * The www comes after the a but before a\\000, so we need to make another name (\\000.a)
		 */
		if (last_dname_p) {
			ldns_rdf_deep_free(last_dname_p);
		}
		last_dname_p = create_plus_1_dname(last_dname);
	} else {
		if (last_dname) {
			if (ldns_rdf_compare(last_dname, next_dname) == 0) {
				printf("\n\nNext dname is the same as current, this would loop forever. This is a problem that usually occurs when walking through a caching forwarder. Try using the authoritative nameserver to walk (with @nameserver).\n");
				exit(2);
			}
			ldns_rdf_deep_free(last_dname);
		}
		last_dname = ldns_rdf_clone(next_dname);
		if (last_dname_p) {
			ldns_rdf_deep_free(last_dname_p);
		}
		last_dname_p = create_dname_plus_1(last_dname);
		if (!full) {
			ldns_rdf_print(stdout, ldns_rr_owner(nsec_rr));
			printf(" ");
			ldns_rdf_print(stdout, ldns_rr_rdf(nsec_rr, 1));
			printf("\n");
		} else {
			/* ok, so now we know all the types present at this name,
			 * query for those one by one (...)
			 */
			query_type_bitmaps(res, LDNS_RD, ldns_rr_owner(nsec_rr),
			                   ldns_rr_rdf(nsec_rr, 1));
			/* print this nsec and its signatures too */
			ldns_rr_print(stdout, nsec_rr);
			if (nsec_sigs) {
				ldns_rr_list_print(stdout, nsec_sigs);
				ldns_rr_list_free(nsec_sigs);
				nsec_sigs = NULL;
			}
		}
	}

	}

	ldns_rdf_deep_free(domain);
	ldns_rdf_deep_free(soa_p1);
	ldns_rdf_deep_free(last_dname_p);
	ldns_rdf_deep_free(last_dname);
	ldns_rdf_deep_free(next_dname);
	ldns_rr_free(nsec_rr);
	ldns_pkt_free(p);

	ldns_rr_free(soa);

	printf("\n\n");
	ldns_resolver_deep_free(res);

	exit:
	return result;
}
