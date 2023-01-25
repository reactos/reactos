/*
 * ldns-testns. Light-weight DNS daemon, gives canned replies.
 *
 * Tiny dns server, that responds with specially crafted replies
 * to requests. For testing dns software.
 *
 * (c) NLnet Labs, 2005 - 2008
 * See the file LICENSE for the license
 */

/*
 * This program is a debugging aid. It can is not efficient, especially
 * with a long config file, but it can give any reply to any query.
 * This can help the developer pre-script replies for queries.
 *
 * It listens to IP4 UDP and TCP by default.
 * You can specify a packet RR by RR with header flags to return.
 *
 * Missing features:
 *		- matching content different from reply content.
 *		- find way to adjust mangled packets?
 */

/*
	The data file format is as follows:
	
	; comment.
	; a number of entries, these are processed first to last.
	; a line based format.

	$ORIGIN origin
	$TTL default_ttl

	ENTRY_BEGIN
	; first give MATCH lines, that say what queries are matched
	; by this entry.
	; 'opcode' makes the query match the opcode from the reply
	; if you leave it out, any opcode matches this entry.
	; 'qtype' makes the query match the qtype from the reply
	; 'qname' makes the query match the qname from the reply
	; 'serial=1023' makes the query match if ixfr serial is 1023. 
	MATCH [opcode] [qtype] [qname] [serial=<value>]
	MATCH [UDP|TCP]
	MATCH ...
	; Then the REPLY header is specified.
	REPLY opcode, rcode or flags.
		(opcode)  QUERY IQUERY STATUS NOTIFY UPDATE
		(rcode)   NOERROR FORMERR SERVFAIL NXDOMAIN NOTIMPL YXDOMAIN
		 		YXRRSET NXRRSET NOTAUTH NOTZONE
		(flags)   QR AA TC RD CD RA AD
	REPLY ...
	; any additional actions to do.
	; 'copy_id' copies the ID from the query to the answer.
	ADJUST copy_id
	; 'sleep=10' sleeps for 10 seconds before giving the answer (TCP is open)
	ADJUST [sleep=<num>]    ; sleep before giving any reply
	ADJUST [packet_sleep=<num>]  ; sleep before this packet in sequence
	SECTION QUESTION
	<RRs, one per line>    ; the RRcount is determined automatically.
	SECTION ANSWER
	<RRs, one per line>
	SECTION AUTHORITY
	<RRs, one per line>
	SECTION ADDITIONAL
	<RRs, one per line>
	EXTRA_PACKET		; follow with SECTION, REPLY for more packets.
	HEX_ANSWER_BEGIN	; follow with hex data
				; this replaces any answer packet constructed
				; with the SECTION keywords (only SECTION QUERY
				; is used to match queries). If the data cannot
				; be parsed, ADJUST rules for the answer packet
				; are ignored
	HEX_ANSWER_END
	ENTRY_END
*/

/* Example data file:
$ORIGIN nlnetlabs.nl
$TTL 3600

ENTRY_BEGIN
MATCH qname
REPLY NOERROR
ADJUST copy_id
SECTION QUESTION
www.nlnetlabs.nl.	IN	A
SECTION ANSWER
www.nlnetlabs.nl.	IN	A	195.169.215.155
SECTION AUTHORITY
nlnetlabs.nl.		IN	NS	www.nlnetlabs.nl.
ENTRY_END

ENTRY_BEGIN
MATCH qname
REPLY NOERROR
ADJUST copy_id
SECTION QUESTION
www2.nlnetlabs.nl.	IN	A
HEX_ANSWER_BEGIN
; 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19
;-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
 00 bf 81 80 00 01 00 01 00 02 00 02 03 77 77 77 0b 6b 61 6e	;	   1-  20
 61 72 69 65 70 69 65 74 03 63 6f 6d 00 00 01 00 01 03 77 77	;	  21-  40
 77 0b 6b 61 6e 61 72 69 65 70 69 65 74 03 63 6f 6d 00 00 01	;	  41-  60
 00 01 00 01 50 8b 00 04 52 5e ed 32 0b 6b 61 6e 61 72 69 65	;	  61-  80
 70 69 65 74 03 63 6f 6d 00 00 02 00 01 00 01 50 8b 00 11 03	;	  81- 100
 6e 73 31 08 68 65 78 6f 6e 2d 69 73 02 6e 6c 00 0b 6b 61 6e	;	 101- 120
 61 72 69 65 70 69 65 74 03 63 6f 6d 00 00 02 00 01 00 01 50	;	 121- 140
 8b 00 11 03 6e 73 32 08 68 65 78 6f 6e 2d 69 73 02 6e 6c 00	;	 141- 160
 03 6e 73 31 08 68 65 78 6f 6e 2d 69 73 02 6e 6c 00 00 01 00	;	 161- 180
 01 00 00 46 53 00 04 52 5e ed 02 03 6e 73 32 08 68 65 78 6f	;	 181- 200
 6e 2d 69 73 02 6e 6c 00 00 01 00 01 00 00 46 53 00 04 d4 cc	;	 201- 220
 db 5b
HEX_ANSWER_END
ENTRY_END


*/

struct sockaddr_storage;
#include "config.h"
#include <ldns/ldns.h>
#include "ldns-testpkts.h"

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_TIME_H
#include <time.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_NETINET_UDP_H
#include <netinet/udp.h>
#endif
#ifdef HAVE_NETINET_IGMP_H
#include <netinet/igmp.h>
#endif
#include <errno.h>
#include <signal.h>

#ifdef HAVE_TARGETCONDITIONALS_H
#include <TargetConditionals.h>
#endif

#if defined(TARGET_OS_TV) || defined(TARGET_OS_WATCH)
#undef HAVE_FORK
#endif

#define INBUF_SIZE 4096         /* max size for incoming queries */
#define DEFAULT_PORT 53		/* default if no -p port is specified */
#define CONN_BACKLOG 256	/* connections queued up for tcp */
static const char* prog_name = "ldns-testns";
static FILE* logfile = 0;
static int do_verbose = 0;

static void usage(void)
{
	printf("Usage: %s [options] <datafile>\n", prog_name);
	printf("  -r	listens on random port. Port number is printed.\n");
	printf("  -p	listens on the specified port, default %d.\n", DEFAULT_PORT);
	printf("  -f	forks given number extra instances, default none.\n");
	printf("  -v	more verbose, prints queries, answers and matching.\n");
	printf("  -6	listen on IP6 any address, instead of IP4 any address.\n");
	printf("The program answers queries with canned replies from the datafile.\n");
	exit(EXIT_FAILURE);
}

static void log_msg(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	vfprintf(logfile, msg, args);
	fflush(logfile);
	va_end(args);
}

static void error(const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	fprintf(logfile, "%s error: ", prog_name);
	vfprintf(logfile, msg, args);
	fprintf(logfile, "\n");
	fflush(logfile);
	va_end(args);
	exit(EXIT_FAILURE);
}

void verbose(int lvl, const char* msg, ...) ATTR_FORMAT(printf, 2, 3);
void verbose(int ATTR_UNUSED(lvl), const char* msg, ...)
{
	va_list args;
	va_start(args, msg);
	if(do_verbose)
		vfprintf(logfile, msg, args);
	fflush(logfile);
	va_end(args);
}

static int bind_port(int sock, int port, int fam)
{
    struct sockaddr_in addr;
#if defined(AF_INET6) && defined(HAVE_GETADDRINFO)
    if(fam == AF_INET6) {
    	struct sockaddr_in6 addr6;
	memset(&addr6, 0, sizeof(addr6));
	addr6.sin6_family = AF_INET6;
    	addr6.sin6_port = (in_port_t)htons((uint16_t)port);
#  if HAVE_DECL_IN6ADDR_ANY
	addr6.sin6_addr = in6addr_any;
#  else
	memset(&addr6.sin6_addr, 0, sizeof(addr6.sin6_addr));
#  endif
    	return bind(sock, (struct sockaddr *)&addr6, (socklen_t) sizeof(addr6));
    }
#endif

#ifndef S_SPLINT_S
    addr.sin_family = AF_INET;
#endif
    addr.sin_port = (in_port_t)htons((uint16_t)port);
    addr.sin_addr.s_addr = INADDR_ANY;
    return bind(sock, (struct sockaddr *)&addr, (socklen_t) sizeof(addr));
}

struct handle_udp_userdata {
	int udp_sock;
	struct sockaddr_storage addr_him;
	socklen_t hislen;
};
static void
send_udp(uint8_t* buf, size_t len, void* data)
{
	struct handle_udp_userdata *userdata = (struct handle_udp_userdata*)data;
	/* udp send reply */
	ptrdiff_t nb;
	nb = sendto(userdata->udp_sock, (void*)buf, len, 0, 
		(struct sockaddr*)&userdata->addr_him, userdata->hislen);
	if(nb == -1)
		log_msg("sendto(): %s\n", strerror(errno));
	else if((size_t)nb != len)
		log_msg("sendto(): only sent %d of %d octets.\n", 
			(int)nb, (int)len);
}

static void
handle_udp(int udp_sock, struct entry* entries, int *count)
{
	ptrdiff_t nb;
	uint8_t inbuf[INBUF_SIZE];
	struct handle_udp_userdata userdata;
	userdata.udp_sock = udp_sock;

	userdata.hislen = (socklen_t)sizeof(userdata.addr_him);
	/* udp recv */
	nb = recvfrom(udp_sock, (void*)inbuf, INBUF_SIZE, 0, 
		(struct sockaddr*)&userdata.addr_him, &userdata.hislen);
	if (nb < 1) {
#ifndef USE_WINSOCK
		log_msg("recvfrom(): %s\n", strerror(errno));
#else
		if(WSAGetLastError() != WSAEINPROGRESS &&
			WSAGetLastError() != WSAECONNRESET &&
			WSAGetLastError()!= WSAEWOULDBLOCK)
			log_msg("recvfrom(): %d\n", WSAGetLastError());
#endif
		return;
	}
	handle_query(inbuf, nb, entries, count, transport_udp, send_udp, 
		&userdata, do_verbose?logfile:0);
}

static int
read_n_bytes(int sock, uint8_t* buf, size_t sz)
{
	size_t count = 0;
	while(count < sz) {
		ptrdiff_t nb = recv(sock, (void*)(buf+count), sz-count, 0);
		if(nb < 0) {
			log_msg("recv(): %s\n", strerror(errno));
			return -1;
		} else if(nb == 0) {
			log_msg("recv: remote end closed the channel\n");
			return sz-count;
		}
		count += nb;
	}
	return 0;
}

static void
write_n_bytes(int sock, uint8_t* buf, size_t sz)
{
	size_t count = 0;
	while(count < sz) {
		ptrdiff_t nb = send(sock, (void*)(buf+count), sz-count, 0);
		if(nb < 0) {
			log_msg("send(): %s\n", strerror(errno));
			return;
		}
		count += nb;
	}
}

struct handle_tcp_userdata {
	int s;
};
static void
send_tcp(uint8_t* buf, size_t len, void* data)
{
	struct handle_tcp_userdata *userdata = (struct handle_tcp_userdata*)data;
	uint16_t tcplen;
	/* tcp send reply */
	tcplen = htons(len);
	write_n_bytes(userdata->s, (uint8_t*)&tcplen, sizeof(tcplen));
	write_n_bytes(userdata->s, buf, len);
}

static void
handle_tcp(int tcp_sock, struct entry* entries, int *count)
{
	int s;
	struct sockaddr_storage addr_him;
	socklen_t hislen;
	uint8_t inbuf[INBUF_SIZE];
	uint16_t tcplen = 0;
	struct handle_tcp_userdata userdata;

	/* accept */
	hislen = (socklen_t)sizeof(addr_him);
	if((s = accept(tcp_sock, (struct sockaddr*)&addr_him, &hislen)) < 0) {
		log_msg("accept(): %s\n", strerror(errno));
		return;
	}
	userdata.s = s;

	while(1) {
		/* tcp recv */
		if (read_n_bytes(s, (uint8_t*)&tcplen, sizeof(tcplen))) {
#ifndef USE_WINSOCK
			close(s);
#else
			closesocket(s);
#endif
			return;
		}
		tcplen = ntohs(tcplen);
		if(tcplen >= INBUF_SIZE) {
			log_msg("query %d bytes too large, buffer %d bytes.\n",
				tcplen, INBUF_SIZE);
#ifndef USE_WINSOCK
			close(s);
#else
			closesocket(s);
#endif
			return;
		}
		if (read_n_bytes(s, inbuf, tcplen)) {
#ifndef USE_WINSOCK
			close(s);
#else
			closesocket(s);
#endif
			return;
		}

		handle_query(inbuf, (ptrdiff_t) tcplen, entries, count, transport_tcp, 
			send_tcp, &userdata, do_verbose?logfile:0);

		/* another query straight away? */
		if(1) {
			fd_set rset;
			struct timeval tv;
			int ret;
			FD_ZERO(&rset);
			FD_SET(s, &rset);
			tv.tv_sec = 0;
			tv.tv_usec = 100*1000;
			ret = select(s+1, &rset, NULL, NULL, &tv);
			if(ret < 0) {
				error("select(): %s\n", strerror(errno));
			}
			if(ret == 0) {
				/* timeout */
				break;
			}
		}
	}
#ifndef USE_WINSOCK
	close(s);
#else
	closesocket(s);
#endif

}

/** shared by the service and main routine (forked and threaded) */
static int udp_sock, tcp_sock;
static struct entry* entries;

/** 
 * Test DNS server service, uses global udpsock, tcpsock, reply entries 
 * The signature is kept void so the function can be used as a thread function.
 */
static void
service(void)
{
	fd_set rset, wset, eset;
	int count;
	int maxfd;

	/* service */
	count = 0;
	while (1) {
#ifndef S_SPLINT_S
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		FD_ZERO(&eset);
		FD_SET(udp_sock, &rset);
		FD_SET(tcp_sock, &rset);
#endif
		maxfd = udp_sock;
		if(tcp_sock > maxfd)
			maxfd = tcp_sock;
		if(select(maxfd+1, &rset, &wset, &eset, NULL) < 0) {
			error("select(): %s\n", strerror(errno));
		}
		if(FD_ISSET(udp_sock, &rset)) {
			handle_udp(udp_sock, entries, &count);
		}
		if(FD_ISSET(tcp_sock, &rset)) {
			handle_tcp(tcp_sock, entries, &count);
		}
	}
}

static void
forkit(int number)
{
	int i;
	for(i=0; i<number; i++)
	{
#if !defined(HAVE_FORK) || !defined(HAVE_FORK_AVAILABLE)
#ifndef USE_WINSOCK
		log_msg("fork() not available.\n");
		exit(1);
#else /* USE_WINSOCK */
		DWORD tid;
		HANDLE id = CreateThread(NULL, 0, 
			(LPTHREAD_START_ROUTINE)service, NULL,
			0, &tid);
		if(id == NULL) {
			log_msg("error CreateThread: %d\n", GetLastError());
			return;
		}
		log_msg("thread id: %d\n", (int)tid);
#endif /* USE_WINSOCK */
#else /* HAVE_FORK */
		pid_t pid = fork();
		if(pid == (pid_t) -1) {
			log_msg("error forking: %s\n", strerror(errno));
			return;
		}
		if(pid == 0)
			return; /* child starts serving */
		log_msg("forked pid: %d\n", (int)pid);
#endif /* HAVE_FORK */
	}
}

int
main(int argc, char **argv)
{
	/* arguments */
	int c;
	int port = DEFAULT_PORT;
	const char* datafile;
	int forknum = 0;

	/* network */
	int fam = AF_INET;
	bool random_port_success;

#ifdef USE_WINSOCK
	WSADATA wsa_data;
#endif
	
	/* parse arguments */
	srandom(time(NULL) ^ getpid());
	logfile = stdout;
	prog_name = argv[0];
	log_msg("%s: start\n", prog_name);
	while((c = getopt(argc, argv, "6f:p:rv")) != -1) {
		switch(c) {
		case '6':
#ifdef AF_INET6
			fam = AF_INET6;
#else
			log_msg("cannot -6: no IP6 available\n");
			exit(1);
#endif
			break;
		case 'r':
                	port = 0;
                	break;
		case 'f':
                	forknum = atoi(optarg);
			if(forknum < 1)
				error("invalid forkno %s, give number", optarg);
                	break;
		case 'p':
			port = atoi(optarg);
			if (port < 1) {
				error("Invalid port %s, use a number.", optarg);
			}
			break;
		case 'v':
			do_verbose++;
			break;
		default:
			usage();
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if(argc == 0 || argc > 1)
		usage();
	
	datafile = argv[0];
	log_msg("Reading datafile %s\n", datafile);
	entries = read_datafile(datafile, 0);

#ifdef SIGPIPE
        (void)signal(SIGPIPE, SIG_IGN);
#endif
#ifdef USE_WINSOCK
	if(WSAStartup(MAKEWORD(2,2), &wsa_data) != 0)
		error("WSAStartup failed\n");
#endif
	
	if((udp_sock = socket(fam, SOCK_DGRAM, 0)) < 0) {
		error("udp socket(): %s\n", strerror(errno));
	}
	if((tcp_sock = socket(fam, SOCK_STREAM, 0)) < 0) {
		error("tcp socket(): %s\n", strerror(errno));
	}
	c = 1;
	if(setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, (void*)&c, (socklen_t) sizeof(int)) < 0) {
		error("setsockopt(SO_REUSEADDR): %s\n", strerror(errno));
	}

	/* bind ip4 */
	if (port > 0) {
		if (bind_port(udp_sock, port, fam)) {
			error("cannot bind(): %s\n", strerror(errno));
		}
		if (bind_port(tcp_sock, port, fam)) {
			error("cannot bind(): %s\n", strerror(errno));
		}
		if (listen(tcp_sock, CONN_BACKLOG) < 0) {
			error("listen(): %s\n", strerror(errno));
		}
	} else {
		random_port_success = false;
		while (!random_port_success) {
			port = (random() % 64510) + 1025;
			log_msg("trying to bind to port %d\n", port);
			random_port_success = true;
			if (bind_port(udp_sock, port, fam)) {
#ifdef EADDRINUSE
				if (errno != EADDRINUSE) {
#elif defined(USE_WINSOCK)
				if (WSAGetLastError() != WSAEADDRINUSE) {
#else
				if (1) {
#endif
					perror("bind()");
					return -1;
				} else {
					random_port_success = false;
				}
			}
			if (random_port_success) {
				if (bind_port(tcp_sock, port, fam)) {
#ifdef EADDRINUSE
					if (errno != EADDRINUSE) {
#elif defined(USE_WINSOCK)
					if (WSAGetLastError()!=WSAEADDRINUSE){
#else
					if (1) {
#endif
						perror("bind()");
						return -1;
					} else {
						random_port_success = false;
					}
				}
			}
			if (random_port_success) {
				if (listen(tcp_sock, CONN_BACKLOG) < 0) {
					error("listen(): %s\n", strerror(errno));
				}
			}
		
		}
	}
	log_msg("Listening on port %d\n", port);

	/* forky! */
	if(forknum > 0)
		forkit(forknum);

	service();

        return 0;
}
