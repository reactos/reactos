/* 
 * ldns-notify.c - ldns-notify(8)
 * 
 * Copyright (c) 2001-2008, NLnet Labs, All right reserved
 *
 * See LICENSE for the license
 *
 * send a notify packet to a server
 */

#include "config.h"

/* ldns */
#include <ldns/ldns.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#include <errno.h>

static int verbose = 1;
static int max_num_retry = 15; /* times to try */

static void
usage(void)
{
	fprintf(stderr, "usage: ldns-notify [other options] -z zone <servers>\n");
	fprintf(stderr, "Ldns notify utility\n\n");
	fprintf(stderr, " Supported options:\n");
	fprintf(stderr, "\t-z zone\t\tThe zone\n");
	fprintf(stderr, "\t-I <address>\tsource address to query from\n");
	fprintf(stderr, "\t-s version\tSOA version number to include\n");
	fprintf(stderr, "\t-y <name:key[:algo]>\tspecify named base64 tsig key"
	                ", and optional an\n\t\t\t"
	                "algorithm (defaults to hmac-md5.sig-alg.reg.int)\n");
	fprintf(stderr, "\t-p port\t\tport to use to send to\n");
	fprintf(stderr, "\t-v\t\tPrint version information\n");
	fprintf(stderr, "\t-d\t\tPrint verbose debug information\n");
	fprintf(stderr, "\t-r num\t\tmax number of retries (%d)\n", 
		max_num_retry);
	fprintf(stderr, "\t-h\t\tPrint this help information\n\n");
	fprintf(stderr, "Report bugs to <ldns-team@nlnetlabs.nl>\n");
	exit(1);
}

static void
version(void)
{
        fprintf(stderr, "%s version %s\n", PACKAGE_NAME, PACKAGE_VERSION);
        fprintf(stderr, "Written by NLnet Labs.\n\n");
        fprintf(stderr,
                "Copyright (C) 2001-2008 NLnet Labs.  This is free software.\n"
                "There is NO warranty; not even for MERCHANTABILITY or FITNESS\n"
                "FOR A PARTICULAR PURPOSE.\n");
        exit(0);
}

static void
notify_host(int s, struct addrinfo* res, uint8_t* wire, size_t wiresize,
	const char* addrstr)
{
	int timeout_retry = 5; /* seconds */
	int num_retry = max_num_retry;
#ifndef S_SPLINT_S
	fd_set rfds;
#endif
	struct timeval tv;
	int retval = 0;
	ptrdiff_t received = 0;
	int got_ack = 0;
	socklen_t addrlen = 0;
	uint8_t replybuf[2048];
	ldns_status status;
	ldns_pkt* pkt = NULL;
	
	while(!got_ack) {
		/* send it */
		if(sendto(s, (void*)wire, wiresize, 0, 
			res->ai_addr, res->ai_addrlen) == -1) {
			printf("warning: send to %s failed: %s\n",
				addrstr, strerror(errno));
#ifndef USE_WINSOCK
			close(s);
#else
			closesocket(s);
#endif
			return;
		}

		/* wait for ACK packet */
#ifndef S_SPLINT_S
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		tv.tv_sec = timeout_retry; /* seconds */
#endif
		tv.tv_usec = 0; /* microseconds */
		retval = select(s + 1, &rfds, NULL, NULL, &tv);
		if (retval == -1) {
			printf("error waiting for reply from %s: %s\n",
				addrstr, strerror(errno));
#ifndef USE_WINSOCK
			close(s);
#else
			closesocket(s);
#endif
			return;
		}
		if(retval == 0) {
			num_retry--;
			if(num_retry == 0) {
				printf("error: failed to send notify to %s.\n",
					addrstr);
				exit(1);
			}
			printf("timeout (%d s) expired, retry notify to %s.\n",
				timeout_retry, addrstr);
		}
		if (retval == 1) {
			got_ack = 1;
		}
	}

	/* got reply */
	addrlen = res->ai_addrlen;
	received = recvfrom(s, (void*)replybuf, sizeof(replybuf), 0,
		res->ai_addr, &addrlen);
	res->ai_addrlen = addrlen;

#ifndef USE_WINSOCK
	close(s);
#else
	closesocket(s);
#endif
	if (received == -1) {
		printf("recv %s failed: %s\n", addrstr, strerror(errno));
		return;
	}

	/* check reply */
	status = ldns_wire2pkt(&pkt, replybuf, (size_t)received);
	if(status != LDNS_STATUS_OK) {
		ptrdiff_t i;
		printf("Could not parse reply packet: %s\n",
			ldns_get_errorstr_by_id(status));
		if (verbose > 1) {
			printf("hexdump of reply: ");
			for(i=0; i<received; i++)
				printf("%02x", (unsigned)replybuf[i]);
			printf("\n");
		}
		exit(1);
	}

	if(verbose) {
		ptrdiff_t i;
		printf("# reply from %s:\n", addrstr);
		ldns_pkt_print(stdout, pkt);
		if (verbose > 1) {
			printf("hexdump of reply: ");
			for(i=0; i<received; i++)
				printf("%02x", (unsigned)replybuf[i]);
			printf("\n");
		}
	}
	ldns_pkt_free(pkt);
}

int
main(int argc, char **argv)
{
	int c;
	int i;

	/* LDNS types */
	ldns_pkt *notify;
	ldns_rr *question;
	ldns_rdf *ldns_zone_name = NULL;
	ldns_status status;
	const char *zone_name = NULL;
	int include_soa = 0;
	uint32_t soa_version = 0;
	int do_hexdump = 1;
	uint8_t *wire = NULL;
	size_t wiresize = 0;
	const char *port = "53";
	char *tsig_sep;
	const char *tsig_name = NULL, *tsig_data = NULL, *tsig_algo = NULL;
	int error;
	struct addrinfo from_hints, *from0 = NULL;

	srandom(time(NULL) ^ getpid());

        while ((c = getopt(argc, argv, "vhdp:r:s:y:z:I:")) != -1) {
                switch (c) {
                case 'd':
			verbose++;
			break;
                case 'p':
			port = optarg;
			break;
                case 'r':
			max_num_retry = atoi(optarg);
			break;
                case 's':
			include_soa = 1;
			soa_version = (uint32_t)atoi(optarg);
			break;
                case 'y':
			if (!(tsig_sep = strchr(optarg, ':'))) {
				printf("TSIG argument is not in form "
					"<name:key[:algo]> %s\n", optarg);
				exit(1);
			}
			tsig_name = optarg;
			*tsig_sep++ = '\0';
			tsig_data = tsig_sep;
			if ((tsig_sep = strchr(tsig_sep, ':'))) {
				*tsig_sep++ = '\0';
				tsig_algo = tsig_sep;
			} else {
				tsig_algo = "hmac-md5.sig-alg.reg.int.";
			}
			/* With dig TSIG keys are also specified with -y,
			 * but format with drill is: -y <name:key[:algo]>
			 *             and with dig: -y [hmac:]name:key
			 *
			 * When we detect an unknown tsig algorithm in algo,
			 * but a known algorithm in name, we cane assume dig
			 * order was used.
			 *
			 * Following if statement is to anticipate and correct
			 * dig order
			 */
			if (strcasecmp(tsig_algo, "hmac-md5.sig-alg.reg.int")&&
			    strcasecmp(tsig_algo, "hmac-md5")                &&
			    strcasecmp(tsig_algo, "hmac-sha1")               &&
			    strcasecmp(tsig_algo, "hmac-sha256")             &&
			    strcasecmp(tsig_algo, "hmac-sha384")             &&
			    strcasecmp(tsig_algo, "hmac-sha512")             &&
			    ! (strcasecmp(tsig_name, "hmac-md5.sig-alg.reg.int")
			    && strcasecmp(tsig_name, "hmac-md5")
			    && strcasecmp(tsig_name, "hmac-sha1")
			    && strcasecmp(tsig_name, "hmac-sha256")
			    && strcasecmp(tsig_name, "hmac-sha384")
			    && strcasecmp(tsig_name, "hmac-sha512"))) {

				/* Roll options */
				const char *tmp_tsig_algo = tsig_name;
				tsig_name = tsig_data;
				tsig_data = tsig_algo;
				tsig_algo = tmp_tsig_algo;
			}	
			printf("Sign with name: %s, data: %s, algorithm: %s\n"
			      , tsig_name, tsig_data, tsig_algo);
			break;
                case 'z':
			zone_name = optarg;
			ldns_zone_name = ldns_dname_new_frm_str(zone_name);
			if(!ldns_zone_name) {
				printf("cannot parse zone name: %s\n", 
					zone_name);
				exit(1);
			}
                        break;
		case 'I':
			memset(&from_hints, 0, sizeof(from_hints));
			from_hints.ai_family = AF_UNSPEC;
			from_hints.ai_socktype = SOCK_DGRAM;
			from_hints.ai_protocol = IPPROTO_UDP;
			from_hints.ai_flags = AI_NUMERICHOST;
			error = getaddrinfo(optarg, 0, &from_hints, &from0);
			if (error) {
				printf("bad address: %s: %s\n", optarg,
					gai_strerror(error));
				exit(EXIT_FAILURE);
			}
			break;
		case 'v':
			version();
			/* fallthrough */
                case 'h':
                case '?':
                default:
                        usage();
                }
        }
        argc -= optind;
        argv += optind;

        if (argc == 0 || zone_name == NULL) {
                usage();
        }

	notify = ldns_pkt_new();
	question = ldns_rr_new();

	if (!notify || !question) {
		/* bail out */
		printf("error: cannot create ldns types\n");
		exit(1);
	}

	/* create the rr for inside the pkt */
	ldns_rr_set_class(question, LDNS_RR_CLASS_IN);
	ldns_rr_set_owner(question, ldns_zone_name);
	ldns_rr_set_type(question, LDNS_RR_TYPE_SOA);
	ldns_rr_set_question(question, true);
	ldns_pkt_set_opcode(notify, LDNS_PACKET_NOTIFY);
	ldns_pkt_push_rr(notify, LDNS_SECTION_QUESTION, question);
	ldns_pkt_set_aa(notify, true);
	ldns_pkt_set_random_id(notify);
	if(include_soa) {
		char buf[10240];
		ldns_rr *soa_rr=NULL;
		ldns_rdf *prev=NULL;
		snprintf(buf, sizeof(buf), "%s 3600 IN SOA . . %u 0 0 0 0",
			zone_name, (unsigned)soa_version);
		/*printf("Adding soa %s\n", buf);*/
		status = ldns_rr_new_frm_str(&soa_rr, buf, 3600, NULL, &prev);
		if(status != LDNS_STATUS_OK) {
			printf("Error adding SOA version: %s\n",
				ldns_get_errorstr_by_id(status));
		}
		ldns_pkt_push_rr(notify, LDNS_SECTION_ANSWER, soa_rr);
	}

	if(tsig_name && tsig_data) {
#ifdef HAVE_SSL
		status = ldns_pkt_tsig_sign(notify, tsig_name,
			tsig_data, 300, tsig_algo, NULL);
		if(status != LDNS_STATUS_OK) {
			printf("Error TSIG sign query: %s\n",
				ldns_get_errorstr_by_id(status));
		}
#else
	fprintf(stderr, "Warning: TSIG needs OpenSSL support, which has not been compiled in, TSIG skipped\n");
#endif
	}

	if(verbose) {
		printf("# Sending packet:\n");
		ldns_pkt_print(stdout, notify);

	}

	status = ldns_pkt2wire(&wire, notify, &wiresize);
	if (status) {
		printf("Error converting notify packet to hex: %s\n",
				ldns_get_errorstr_by_id(status));
	} else if(wiresize == 0) {
		printf("Error converting notify packet to hex.\n");
		exit(1);
	}
	if(do_hexdump && verbose > 1) {
		printf("Hexdump of notify packet:\n");
		for(i=0; i<(int)wiresize; i++)
			printf("%02x", (unsigned)wire[i]);
		printf("\n");
	}

	for(i=0; i<argc; i++)
	{
		struct addrinfo hints, *res0, *ai_res;
		int default_family = AF_UNSPEC;

		if(verbose)
			printf("# sending to %s\n", argv[i]);
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = default_family;
		/* if(strchr(argv[i], ':')) hints.ai_family = AF_INET6; */
		hints.ai_socktype = SOCK_DGRAM;
		hints.ai_protocol = IPPROTO_UDP;
		error = getaddrinfo(argv[i], port, &hints, &res0);
		if (error) {
			printf("skipping bad address: %s: %s\n", argv[i],
				gai_strerror(error));
			continue;
		}
		for (ai_res = res0; ai_res; ai_res = ai_res->ai_next) {
			int s;

			if (from0 && ai_res->ai_family != from0->ai_family)
				continue;

		        s = socket(ai_res->ai_family, ai_res->ai_socktype, 
				ai_res->ai_protocol);
			if(s == -1)
				continue;
			if (from0 && bind(s, from0->ai_addr, from0->ai_addrlen)) {
				perror("Could not bind to source IP");
				exit(EXIT_FAILURE);
			}
			/* send the notify */
			notify_host(s, ai_res, wire, wiresize, argv[i]);
		}
		freeaddrinfo(res0);
	}

	ldns_pkt_free(notify);
	free(wire);
        return 0;
}
