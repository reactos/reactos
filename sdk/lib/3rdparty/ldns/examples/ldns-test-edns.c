/*
 * ldns-test-edns tries to get DNSKEY and RRSIG from an IP address.
 * This can be used to test if a DNS cache supports DNSSEC (caching RRSIGs),
 * i.e. for automatic configuration utilities or when you get a new DNS cache
 * from DHCP and wonder if your local validator could use that as a cache.
 *
 * (c) NLnet Labs 2010
 * See the file LICENSE for the license
 */

#include "config.h"
#include "errno.h"
#include <ldns/ldns.h>

/** print error details */
static int verb = 1;

static struct sockaddr_in6* cast_sockaddr_storage2sockaddr_in6(
		struct sockaddr_storage* s)
{
	return (struct sockaddr_in6*)s;
}

static struct sockaddr_in* cast_sockaddr_storage2sockaddr_in(
		struct sockaddr_storage* s)
{
	return (struct sockaddr_in*)s;
}

/** parse IP address */
static int
convert_addr(char* str, int p, struct sockaddr_storage* addr, socklen_t* len)
{
#ifdef AF_INET6
	if(strchr(str, ':')) {
		*len = (socklen_t)sizeof(struct sockaddr_in6);
		cast_sockaddr_storage2sockaddr_in6(addr)->sin6_family =
			AF_INET6;
		cast_sockaddr_storage2sockaddr_in6(addr)->sin6_port =
			htons((uint16_t)p);
		if(inet_pton(AF_INET6, str,
			&((struct sockaddr_in6*)addr)->sin6_addr) == 1)
			return 1;
	} else {
#endif
		*len = (socklen_t)sizeof(struct sockaddr_in);
#ifndef S_SPLINT_S
		cast_sockaddr_storage2sockaddr_in(addr)->sin_family =
			AF_INET;
#endif
		cast_sockaddr_storage2sockaddr_in(addr)->sin_port =
			htons((uint16_t)p);
		if(inet_pton(AF_INET, str,
			&((struct sockaddr_in*)addr)->sin_addr) == 1)
			return 1;
#ifdef AF_INET6
	}
#endif
	if(verb) printf("error: cannot parse IP address %s\n", str);
	return 0;
}

/** create a query to test */
static ldns_buffer*
make_query(const char* nm, int tp)
{
	/* with EDNS DO and CDFLAG */
	ldns_buffer* b = ldns_buffer_new(512);
	ldns_pkt* p;
	ldns_status s;
	if(!b) {
		if(verb) printf("error: out of memory\n");
		return NULL;
	}

	s = ldns_pkt_query_new_frm_str(&p, nm, tp, LDNS_RR_CLASS_IN,
		(uint16_t)(LDNS_RD|LDNS_CD));
	if(s != LDNS_STATUS_OK) {
		if(verb) printf("error: %s\n", ldns_get_errorstr_by_id(s));
		ldns_buffer_free(b);
		return NULL;
	}
	if(!p) {
		if(verb) printf("error: out of memory\n");
		ldns_buffer_free(b);
		return NULL;
	}

	ldns_pkt_set_edns_do(p, 1);
	ldns_pkt_set_edns_udp_size(p, 4096);
	ldns_pkt_set_id(p, ldns_get_random());
	if( (s=ldns_pkt2buffer_wire(b, p)) != LDNS_STATUS_OK) {
		if(verb) printf("error: %s\n", ldns_get_errorstr_by_id(s));
		ldns_pkt_free(p);
		ldns_buffer_free(b);
		return NULL;
	}
	ldns_pkt_free(p);

	return b;
}

/** try 3 times to get an EDNS reply from the server, exponential backoff */
static int
get_packet(struct sockaddr_storage* addr, socklen_t len, const char* nm,
	int tp, uint8_t **wire, size_t* wlen)
{
	struct timeval t;
	ldns_buffer* qbin;
	ldns_status s;
	int tries = 0;

	memset(&t, 0, sizeof(t));
	t.tv_usec = 100 * 1000; /* 100 milliseconds (then 200, 400, 800) */

	qbin = make_query(nm, tp);
	if(!qbin)
		return 0;
	while(tries < 4) {
		tries ++;
		s = ldns_udp_send(wire, qbin, addr, len, t, wlen);
		if(s != LDNS_STATUS_NETWORK_ERR) {
			break;
		}
		t.tv_usec *= 2;
		if(t.tv_usec > 1000*1000) {
			t.tv_usec -= 1000*1000;
			t.tv_sec += 1;
		}
	}
	ldns_buffer_free(qbin);
	if(tries == 4) {
		if(verb) printf("timeout\n");
		return 0;
	}
	if(s != LDNS_STATUS_OK) {
		if(verb) printf("error: %s\n", ldns_get_errorstr_by_id(s));
		return 0;
	}
	return 1;
}

/** test if type is present in returned packet */
static int
check_type_in_answer(ldns_pkt* p, int t)
{
	ldns_rr_list *l = ldns_pkt_rr_list_by_type(p, t, LDNS_SECTION_ANSWER);
	if(!l) {
		char* s = ldns_rr_type2str(t);
		if(verb) printf("no DNSSEC %s\n", s?s:"(out of memory)");
		LDNS_FREE(s);
		return 0;
	}
	ldns_rr_list_deep_free(l);
	return 1;
}

/** check the packet and make sure that EDNS and DO and the type and RRSIG */
static int
check_packet(uint8_t* wire, size_t len, int tp)
{
	ldns_pkt *p = NULL;
	ldns_status s;
	if( (s=ldns_wire2pkt(&p, wire, len)) != LDNS_STATUS_OK) {
		if(verb) printf("error: %s\n", ldns_get_errorstr_by_id(s));
		goto failed;
	}
	if(!p) {
		if(verb) printf("error: out of memory\n");
		goto failed;
	}

	/* does DNS work? */
	if(ldns_pkt_get_rcode(p) != LDNS_RCODE_NOERROR) {
		char* r = ldns_pkt_rcode2str(ldns_pkt_get_rcode(p));
		if(verb) printf("no answer, %s\n", r?r:"(out of memory)");
		LDNS_FREE(r);
		goto failed;
	}

	/* test EDNS0 presence, of OPT record */
	/* LDNS forgets during pkt parse, but we test the ARCOUNT;
	 * 0 additional means no EDNS(on the wire), and after parsing the
	 * same additional RRs as before means no EDNS OPT */
	if(LDNS_ARCOUNT(wire) == 0 ||
		ldns_pkt_arcount(p) == LDNS_ARCOUNT(wire)) {
		if(verb) printf("no EDNS\n");
		goto failed;
	}

	/* test if the type, RRSIG present */
	if(!check_type_in_answer(p, tp) ||
	   !check_type_in_answer(p, LDNS_RR_TYPE_RRSIG)) {
		goto failed;
	}
	
	LDNS_FREE(wire);
	ldns_pkt_free(p);
	return 1;
failed:
	LDNS_FREE(wire);
	ldns_pkt_free(p);
	return 0;
}

/** check EDNS at this IP and port */
static int
check_edns_ip(char* ip, int port, int info)
{
	struct sockaddr_storage addr;
	socklen_t len = 0;
	uint8_t* wire;
	size_t wlen;
	memset(&addr, 0, sizeof(addr));
	if(verb) printf("%s ", ip);
	if(!convert_addr(ip, port, &addr, &len))
		return 2;
	/* try to send 3 times to the IP address, test root key */
	if(!get_packet(&addr, len, ".", LDNS_RR_TYPE_DNSKEY, &wire, &wlen))
		return 2;
	if(!check_packet(wire, wlen, LDNS_RR_TYPE_DNSKEY))
		return 1;
	/* check support for caching type DS for chains of trust */
	if(!get_packet(&addr, len, "se.", LDNS_RR_TYPE_DS, &wire, &wlen))
		return 2;
	if(!check_packet(wire, wlen, LDNS_RR_TYPE_DS))
		return 1;
	if(verb) printf("OK\n");
	if(info) printf(" %s", ip);
	return 0;
}

int
main(int argc, char **argv)
{
	int i, r=0, info=0, ok=0;
#ifdef USE_WINSOCK
	WSADATA wsa_data;
	if(WSAStartup(MAKEWORD(2,2), &wsa_data) != 0) {
		printf("WSAStartup failed\n"); exit(1);
	}
#endif
	if (argc < 2 || strncmp(argv[1], "-h", 3) == 0) {
		printf("Usage: ldns-test-edns [-i] {ip address}\n");
		printf("Tests if the DNS cache at IP address supports EDNS.\n");
		printf("if it works, print IP address OK.\n");
		printf("-i: print IPs that are OK or print 'off'.\n");
		printf("exit value, last IP is 0:OK, 1:fail, 2:net error.\n");
		exit(1);
	}
	if(strcmp(argv[1], "-i") == 0) {
		info = 1;
		verb = 0;
	}

	for(i=1+info; i<argc; i++) {
		r = check_edns_ip(argv[i], LDNS_PORT, info);
		if(r == 0)
			ok++;
	}
	if(info && !ok)
		printf("off\n");
	return r;
}
