/*
 * read a zone file from disk and prints it, one RR per line
 *
 * (c) NLnetLabs 2005-2008
 *
 * See the file LICENSE for the license
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>

#include <ldns/ldns.h>
#include <ldns/host2str.h>

#include <errno.h>

static void print_usage(const char* progname)
{
	printf("Usage: %s [OPTIONS] <zonefile>\n", progname);
	printf("\tReads the zonefile and prints it.\n");
	printf("\tThe RR count of the zone is printed to stderr.\n");
	printf("\t-0 zeroize timestamps and signature in RRSIG records.\n");
	printf("\t-b include Bubble Babble encoding of DS's.\n");
	printf("\t-c canonicalize all rrs in the zone.\n");
	printf("\t-d only show DNSSEC data from the zone\n");
	printf("\t-e <rr type>\n");
	printf("\t\tDo not print RRs of the given <rr type>.\n");
	printf("\t\tThis option may be given multiple times.\n");
	printf("\t\t-e is not meant to be used together with -E.\n");
	printf("\t-E <rr type>\n");
	printf("\t\tPrint only RRs of the given <rr type>.\n");
	printf("\t\tThis option may be given multiple times.\n");
	printf("\t\t-E is not meant to be used together with -e.\n");
	printf("\t-h show this text\n");
	printf("\t-n do not print the SOA record\n");
	printf("\t-p prepend SOA serial with spaces so"
		" it takes exactly ten characters.\n");
	printf("\t-s strip DNSSEC data from the zone\n");
	printf("\t-S [[+|-]<number> | YYYYMMDDxx | "
			" unixtime ]\n"
		"\t\tSet serial number to <number> or,"
			" when preceded by a sign,\n"
		"\t\toffset the existing number with "
			"<number>.  With YYYYMMDDxx\n"
		"\t\tthe serial is formatted as a datecounter"
			", and with unixtime as\n"
		"\t\tthe number of seconds since 1-1-1970."
			"  However, on serial\n"
		"\t\tnumber decrease, +1 is used in stead"
			".  (implies -s)\n");
	printf("\t-u <rr type>\n");
	printf("\t\tMark <rr type> for printing in unknown type format.\n");
	printf("\t\tThis option may be given multiple times.\n");
	printf("\t\t-u is not meant to be used together with -U.\n");
	printf("\t-U <rr type>\n");
	printf("\t\tMark <rr type> for not printing in unknown type format.\n");
	printf("\t\tThis option may be given multiple times.\n");
	printf(
	"\t\tThe first occurrence of the -U option marks all RR types for"
	"\n\t\tprinting in unknown type format except for the given <rr type>."
	"\n\t\tSubsequent -U options will clear the mark for those <rr type>s"
	"\n\t\ttoo, so that only the given <rr type>s will be printed in the"
	"\n\t\tpresentation format specific for those <rr type>s.\n");
	printf("\t\t-U is not meant to be used together with -u.\n");
	printf("\t-v shows the version and exits\n");
	printf("\t-z sort the zone (implies -c).\n");
	printf("\nif no file is given standard input is read\n");
	exit(EXIT_SUCCESS);
}

static void exclude_type(ldns_rdf **show_types, ldns_rr_type t)
{
	ldns_status s;

	assert(show_types != NULL);

	if (! *show_types && LDNS_STATUS_OK !=
			(s = ldns_rdf_bitmap_known_rr_types(show_types)))
		goto fail;

	s =  ldns_nsec_bitmap_clear_type(*show_types, t);
	if (s == LDNS_STATUS_OK)
		return;
fail:
	fprintf(stderr, "Cannot exclude rr type %s: %s\n"
	              , ldns_rr_descript(t)->_name
		      , ldns_get_errorstr_by_id(s));
	exit(EXIT_FAILURE);
}

static void include_type(ldns_rdf **show_types, ldns_rr_type t)
{
	ldns_status s;

	assert(show_types != NULL);

	if (! *show_types && LDNS_STATUS_OK !=
			(s = ldns_rdf_bitmap_known_rr_types_space(show_types)))
		goto fail;

	s =  ldns_nsec_bitmap_set_type(*show_types, t);
	if (s == LDNS_STATUS_OK)
		return;
fail:
	fprintf(stderr, "Cannot exclude all rr types except %s: %s\n"
	              , ldns_rr_descript(t)->_name
		      , ldns_get_errorstr_by_id(s));
	exit(EXIT_FAILURE);
}

int
main(int argc, char **argv)
{
	char *filename;
	FILE *fp;
	ldns_zone *z;
	int line_nr = 0;
	int c;
	bool canonicalize = false;
	bool sort = false;
	bool print_soa = true;
	ldns_status s;
	size_t i;
	ldns_rr_list *stripped_list;
	ldns_rr *cur_rr;
	ldns_output_format_storage fmt_storage;
	ldns_output_format* fmt = ldns_output_format_init(&fmt_storage);
	ldns_rdf *show_types = NULL;

	ldns_soa_serial_increment_func_t soa_serial_increment_func = NULL;
	int soa_serial_increment_func_data = 0;

        while ((c = getopt(argc, argv, "0bcde:E:hnpsS:u:U:vz")) != -1) {
                switch(c) {
			case '0':
				fmt->flags |= LDNS_FMT_ZEROIZE_RRSIGS;
				break;
			case 'b':
				fmt->flags |= 
					( LDNS_COMMENT_BUBBLEBABBLE |
					  LDNS_COMMENT_FLAGS        );
				break;
                	case 'c':
                		canonicalize = true;
                		break;
                	case 'd':
				include_type(&show_types, LDNS_RR_TYPE_RRSIG);
				include_type(&show_types, LDNS_RR_TYPE_NSEC);
				include_type(&show_types, LDNS_RR_TYPE_NSEC3);
				break;
			case 'e':
				exclude_type(&show_types, 
					ldns_get_rr_type_by_name(optarg));
				break;
			case 'E':
				include_type(&show_types, 
					ldns_get_rr_type_by_name(optarg));
				break;
			case 'h':
				print_usage("ldns-read-zone");
				break;
			case 'n':
				print_soa = false;
				break;
			case 'p':
				fmt->flags |= LDNS_FMT_PAD_SOA_SERIAL;
				break;
			case 's':
			case 'S':
				exclude_type(&show_types, LDNS_RR_TYPE_RRSIG);
				exclude_type(&show_types, LDNS_RR_TYPE_NSEC);
				exclude_type(&show_types, LDNS_RR_TYPE_NSEC3);
				if (c == 's') break;
				if (*optarg == '+' || *optarg == '-') {
					soa_serial_increment_func_data =
						atoi(optarg);
					soa_serial_increment_func =
						ldns_soa_serial_increment_by;
				} else if (! strtok(optarg, "0123456789")) {
					soa_serial_increment_func_data =
						atoi(optarg);
					soa_serial_increment_func =
						ldns_soa_serial_identity;
				} else if (!strcasecmp(optarg, "YYYYMMDDxx")){
					soa_serial_increment_func =
						ldns_soa_serial_datecounter;
				} else if (!strcasecmp(optarg, "unixtime")){
					soa_serial_increment_func =
						ldns_soa_serial_unixtime;
				} else {
					fprintf(stderr, "-S expects a number "
						"optionally preceded by a "
						"+ or - sign to indicate an "
						"offset, or the text YYYYMM"
						"DDxx or unixtime\n");
					exit(EXIT_FAILURE);
				}
				break;
			case 'u':
				s = ldns_output_format_set_type(fmt,
					ldns_get_rr_type_by_name(optarg));
				if (s != LDNS_STATUS_OK) {
					fprintf( stderr
					       , "Cannot set rr type %s "
					         "in output format to "
						 "print as unknown type: %s\n"
					       , ldns_rr_descript(
					       ldns_get_rr_type_by_name(optarg)
						       )->_name
					       , ldns_get_errorstr_by_id(s)
					       );
					exit(EXIT_FAILURE);
				}
				break;
			case 'U':
				s = ldns_output_format_clear_type(fmt,
					ldns_get_rr_type_by_name(optarg));
				if (s != LDNS_STATUS_OK) {
					fprintf( stderr
					       , "Cannot set rr type %s "
					         "in output format to not "
						 "print as unknown type: %s\n"
					       , ldns_rr_descript(
					       ldns_get_rr_type_by_name(optarg)
						       )->_name
					       , ldns_get_errorstr_by_id(s)
					       );
					exit(EXIT_FAILURE);
				}
				break;
			case 'v':
				printf("read zone version %s (ldns version %s)\n", LDNS_VERSION, ldns_version());
				exit(EXIT_SUCCESS);
				break;
                        case 'z':
                		canonicalize = true;
                                sort = true;
                                break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc == 0) {
		fp = stdin;
	} else {
		filename = argv[0];

		fp = fopen(filename, "r");
		if (!fp) {
			fprintf(stderr, "Unable to open %s: %s\n", filename, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	
	s = ldns_zone_new_frm_fp_l(&z, fp, NULL, 0, LDNS_RR_CLASS_IN, &line_nr);

	fclose(fp);
	if (s != LDNS_STATUS_OK) {
		fprintf(stderr, "%s at line %d\n", 
				ldns_get_errorstr_by_id(s),
				line_nr);
                exit(EXIT_FAILURE);
	}

	if (show_types) {
		if (print_soa)
			print_soa = ldns_nsec_bitmap_covers_type(show_types,
					LDNS_RR_TYPE_SOA);
		stripped_list = ldns_rr_list_new();
		while ((cur_rr = ldns_rr_list_pop_rr(ldns_zone_rrs(z))))
			if (ldns_nsec_bitmap_covers_type(show_types,
						ldns_rr_get_type(cur_rr)))
				ldns_rr_list_push_rr(stripped_list, cur_rr);
			else
				ldns_rr_free(cur_rr);
		ldns_rr_list_free(ldns_zone_rrs(z));
		ldns_zone_set_rrs(z, stripped_list);
	}

	if (canonicalize) {
		ldns_rr2canonical(ldns_zone_soa(z));
		for (i = 0; i < ldns_rr_list_rr_count(ldns_zone_rrs(z)); i++) {
			ldns_rr2canonical(ldns_rr_list_rr(ldns_zone_rrs(z), i));
		}
	}
	if (sort) {
		ldns_zone_sort(z);
	}

	if (print_soa && ldns_zone_soa(z)) {
		if (soa_serial_increment_func) {
			ldns_rr_soa_increment_func_int(
					ldns_zone_soa(z)
				, soa_serial_increment_func
				, soa_serial_increment_func_data
				);
		}
		ldns_rr_print_fmt(stdout, fmt, ldns_zone_soa(z));
	}
	ldns_rr_list_print_fmt(stdout, fmt, ldns_zone_rrs(z));

	ldns_zone_deep_free(z);

        exit(EXIT_SUCCESS);
}
