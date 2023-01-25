/*
 * ldnsd. Light-weight DNS daemon
 *
 * Tiny dns server to show how a real one could be built.
 *
 * (c) NLnet Labs, 2005
 * See the file LICENSE for the license
 */

#include "config.h"
#include <ldns/ldns.h>

#ifdef HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
#endif
#ifdef HAVE_ARPA_INET_H
#  include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#  include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_UDP_H
#  include <netinet/udp.h>
#endif
#ifdef HAVE_NETINET_IGMP_H
#  include <netinet/igmp.h>
#endif

#include <errno.h>

#define INBUF_SIZE 4096

static void usage(FILE *output)
{
	fprintf(output, "Usage: ldnsd <address> <port> <zone> <zonefile>\n");
	fprintf(output, "Listens on the specified port and answers queries for the given zone\n");
	fprintf(output, "This is NOT a full-fledged authoritative nameserver!\n");
}

static int udp_bind(int sock, int port, const char *my_address)
{
    struct sockaddr_in addr;
    in_addr_t maddr = INADDR_ANY;

    if (my_address) {
#ifdef AF_INET6
        if (inet_pton(AF_INET6, my_address, &maddr) < 1) {
#else
	if (0) {
#endif
            if (inet_pton(AF_INET, my_address, &maddr) < 1) {
                return -2;
            }
        }
    }

#ifndef S_SPLINT_S
    addr.sin_family = AF_INET;
#endif
    addr.sin_port = (in_port_t) htons((uint16_t)port);
    addr.sin_addr.s_addr = maddr;
    return bind(sock, (struct sockaddr *)&addr, (socklen_t) sizeof(addr));
}

/* this will probably be moved to a better place in the library itself */
static ldns_rr_list *
get_rrset(const ldns_zone *zone, const ldns_rdf *owner_name, const ldns_rr_type qtype, const ldns_rr_class qclass)
{
	uint16_t i;
	ldns_rr_list *rrlist = ldns_rr_list_new();
	ldns_rr *cur_rr;
	if (!zone || !owner_name) {
		fprintf(stderr, "Warning: get_rrset called with NULL zone or owner name\n");
		return rrlist;
	}
	
	for (i = 0; i < ldns_zone_rr_count(zone); i++) {
		cur_rr = ldns_rr_list_rr(ldns_zone_rrs(zone), i);
		if (ldns_dname_compare(ldns_rr_owner(cur_rr), owner_name) == 0 &&
		    ldns_rr_get_class(cur_rr) == qclass &&
		    ldns_rr_get_type(cur_rr) == qtype
		   ) {
			ldns_rr_list_push_rr(rrlist, ldns_rr_clone(cur_rr));
		}
	}
	
	printf("Found rrset of %u rrs\n", (unsigned int) ldns_rr_list_rr_count(rrlist));
	
	return rrlist;
}

int
main(int argc, char **argv)
{
	/* arguments */
	int port;
	const char *zone_file;

	/* network */
	int sock;
	ptrdiff_t nb;
	struct sockaddr addr_me;
	struct sockaddr addr_him;
	socklen_t hislen = (socklen_t) sizeof(addr_him);
	uint8_t inbuf[INBUF_SIZE];
	uint8_t *outbuf;

	/* dns */
	ldns_status status;
	ldns_pkt *query_pkt;
	ldns_pkt *answer_pkt;
	size_t answer_size;
	ldns_rr *query_rr;
	ldns_rr_list *answer_qr;
	ldns_rr_list *answer_an;
	ldns_rr_list *answer_ns;
	ldns_rr_list *answer_ad;
	ldns_rdf *origin = NULL;
	
	/* zone */
	ldns_zone *zone;
	int line_nr;
	FILE *zone_fp;
	
	/* use this to listen on specified interfaces later? */
	char *my_address = NULL;
		
	if (argc < 5) {
		usage(stderr);
		exit(EXIT_FAILURE);
	} else {
	    my_address = argv[1];
		port = atoi(argv[2]);
		if (port < 1) {
			usage(stderr);
			exit(EXIT_FAILURE);
		}
		if (ldns_str2rdf_dname(&origin, argv[3]) != LDNS_STATUS_OK) {
			fprintf(stderr, "Bad origin, not a correct domain name\n");
			usage(stderr);
			exit(EXIT_FAILURE);
		}
		zone_file = argv[4];
	}
	
	printf("Reading zone file %s\n", zone_file);
	zone_fp = fopen(zone_file, "r");
	if (!zone_fp) {
		fprintf(stderr, "Unable to open %s: %s\n", zone_file, strerror(errno));
		exit(EXIT_FAILURE);
	}
	
	line_nr = 0;
	status = ldns_zone_new_frm_fp_l(&zone, zone_fp, origin, 0, LDNS_RR_CLASS_IN, &line_nr);

	if (status != LDNS_STATUS_OK) {
		printf("Zone reader failed, aborting\n");
		exit(EXIT_FAILURE);
	} else {
		printf("Read %u resource records in zone file\n", (unsigned int) ldns_zone_rr_count(zone));
	}
	fclose(zone_fp);

	printf("Listening on port %d\n", port);
	sock =  socket(AF_INET, SOCK_DGRAM, 0);
	if (sock < 0) {
		fprintf(stderr, "%s: socket(): %s\n", argv[0], strerror(errno));
		exit(1);
	}
	memset(&addr_me, 0, sizeof(addr_me));

	/* bind: try all ports in that range */
	if (udp_bind(sock, port, my_address)) {
		fprintf(stderr, "%s: cannot bind(): %s\n", argv[0], strerror(errno));
		exit(errno);
	}

	/* Done. Now receive */
	while (1) {
		nb = recvfrom(sock, (void*)inbuf, INBUF_SIZE, 0, 
			&addr_him, &hislen);
		if (nb < 1) {
			fprintf(stderr, "%s: recvfrom(): %s\n",
			argv[0], strerror(errno));
			exit(1);
		}

		/*
		show(inbuf, nb, nn, hp, sp, ip, bp);
		*/
		printf("Got query of %u bytes\n", (unsigned int) nb);
		status = ldns_wire2pkt(&query_pkt, inbuf, (size_t) nb);
		if (status != LDNS_STATUS_OK) {
			printf("Got bad packet: %s\n", ldns_get_errorstr_by_id(status));
		} else {
			ldns_pkt_print(stdout, query_pkt);
		}

		query_rr = ldns_rr_list_rr(ldns_pkt_question(query_pkt), 0);
		printf("QUERY RR: \n");
		ldns_rr_print(stdout, query_rr);
		
		answer_qr = ldns_rr_list_new();
		ldns_rr_list_push_rr(answer_qr, ldns_rr_clone(query_rr));

		answer_an = get_rrset(zone, ldns_rr_owner(query_rr), ldns_rr_get_type(query_rr), ldns_rr_get_class(query_rr));
		answer_pkt = ldns_pkt_new();
		answer_ns = ldns_rr_list_new();
		answer_ad = ldns_rr_list_new();
		
		ldns_pkt_set_qr(answer_pkt, 1);
		ldns_pkt_set_aa(answer_pkt, 1);
		ldns_pkt_set_id(answer_pkt, ldns_pkt_id(query_pkt));

		ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_QUESTION, answer_qr);
		ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_ANSWER, answer_an);
		ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_AUTHORITY, answer_ns);
		ldns_pkt_push_rr_list(answer_pkt, LDNS_SECTION_ADDITIONAL, answer_ad);

		status = ldns_pkt2wire(&outbuf, answer_pkt, &answer_size);
		
		printf("Answer packet size: %u bytes.\n", (unsigned int) answer_size);
		if (status != LDNS_STATUS_OK) {
			printf("Error creating answer: %s\n", ldns_get_errorstr_by_id(status));
		} else {
			(void) sendto(sock, (void*)outbuf, answer_size, 0, 
				&addr_him, hislen);
		}
		
		ldns_pkt_free(query_pkt);
		ldns_pkt_free(answer_pkt);
		LDNS_FREE(outbuf);
		ldns_rr_list_free(answer_qr);
		ldns_rr_list_free(answer_an);
		ldns_rr_list_free(answer_ns);
		ldns_rr_list_free(answer_ad);
	}
	
	/* No cleanup because of the infinite loop
	 *
	 * ldns_rdf_deep_free(origin);
	 * ldns_zone_deep_free(zone);
	 * return 0;
	 */
}
