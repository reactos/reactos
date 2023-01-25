/*
 * Verify or create TLS authentication with DANE (RFC6698)
 *
 * (c) NLnetLabs 2012
 *
 * See the file LICENSE for the license.
 *
 * wish list:
 * - nicer reporting (tracing of evaluation process)
 * - verbosity levels
 * - STARTTLS support
 */

#include "config.h"
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#include <sys/time.h>
#include <errno.h>
#include <fcntl.h>

#include <ldns/ldns.h>

#ifdef USE_DANE
#ifdef HAVE_SSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#ifndef IPPROTO_SCTP
#define IPPROTO_SCTP 132
#endif

#define LDNS_ERR(code, msg) do { if (code != LDNS_STATUS_OK) \
					ldns_err(msg, code); } while (false)
#define MEMERR(msg) do { fprintf(stderr, "memory error in %s\n", msg); \
			 exit(EXIT_FAILURE); } while (false)
#define BUFSIZE 16384

/* Exit status on a PKIX validated connection but without TLSA records
 * when the -T option was given:
 */
#define NO_TLSAS_EXIT_STATUS 2

/* int verbosity = 3; */

static void
print_usage(const char* progname)
{
#ifdef USE_DANE_VERIFY
	printf("Usage: %s [OPTIONS] verify <name> <port>\n", progname);
	printf("   or: %s [OPTIONS] -t <tlsafile> verify\n", progname);
	printf("\n\tVerify the TLS connection at <name>:<port> or"
	       "\n\tuse TLSA record(s) from <tlsafile> to verify the\n"
			"\tTLS service they reference.\n");
	printf("\n   or: %s [OPTIONS] create <name> <port> [<usage> "
#else
	printf("Usage: %s [OPTIONS] create <name> <port> [<usage> "
#endif
			"[<selector> [<type>]]]\n", progname);
	printf("\n\tUse the TLS connection(s) to <name> <port> "
			"to create the TLSA\n\t"
			"resource record(s) that would "
			"authenticate the connection.\n");
	printf("\n\t<usage>"
			"\t\t0 | PKIX-TA  : CA constraint\n"
			"\t\t\t1 | PKIX-EE  : Service certificate constraint\n"
			"\t\t\t2 | DANE-TA  : Trust anchor assertion\n"
			"\t\t\t3 | DANE-EE  : Domain-issued certificate "
			"(default)\n");
	printf("\n\t<selector>"
			"\t0 | Cert     : Full certificate\n"
			"\t\t\t1 | SPKI     : SubjectPublicKeyInfo "
			"(default)\n");
	printf("\n\t<type>"
			"\t\t0 | Full     : No hash used\n"
			"\t\t\t1 | SHA2-256 : SHA-256 (default)\n"
			"\t\t\t2 | SHA2-512 : SHA-512\n");

	printf("OPTIONS:\n");
	printf("\t-h\t\tshow this text\n");
	printf("\t-4\t\tTLS connect IPv4 only\n");
	printf("\t-6\t\tTLS connect IPv6 only\n");
	printf("\t-r <address>\t"
	       "use resolver at <address> instead of local resolver\n");
	printf("\t-a <address>\t"
	       "don't resolve <name>, but connect to <address>(es)\n");
	printf("\t-b\t\t"
	       "print \"<name>. TYPE52 \\#<size> <hex data>\" form\n"
	      );
	printf("\t-c <certfile>\t"
	       "verify or create TLSA records for the\n"
	       "\t\t\tcertificate (chain) in <certfile>\n"
	      );
	printf("\t-d\t\tassume DNSSEC validity even when insecure or bogus\n");
	printf("\t-f <CAfile>\tuse CAfile to validate\n");
#if HAVE_DANE_CA_FILE
	printf("\t\t\tDefault is %s\n", LDNS_DANE_CA_FILE);
#endif
	printf("\t-i\t\tinteract after connecting\n");
	printf("\t-k <keyfile>\t"
	       "use DNSKEY/DS rr(s) in <keyfile> to validate TLSAs\n"
	       "\t\t\twhen signature chasing (i.e. -S)\n"
	      );
	printf("\t\t\tDefault is %s\n", LDNS_TRUST_ANCHOR_FILE);
	printf("\t-n\t\tdo *not* verify server name in certificate\n");
	printf("\t-o <offset>\t"
	       "select <offset>th certificate from the end of\n"
	       "\t\t\tthe validation chain. -1 means self-signed at end\n"
	      );
	printf("\t-p <CApath>\t"
	       "use certificates in the <CApath> directory to validate\n"
	      );
#if HAVE_DANE_CA_PATH
	printf("\t\t\tDefaults is %s\n", LDNS_DANE_CA_PATH);
#endif
	printf("\t-s\t\tassume PKIX validity\n");
	printf("\t-S\t\tChase signature(s) to a known key\n");
	printf("\t-t <tlsafile>\tdo not use DNS, "
	       "but read TLSA record(s) from <tlsafile>\n"
	      );
	printf("\t-T\t\tReturn exit status 2 for PKIX validated connections\n"
	       "\t\t\twithout (secure) TLSA records(s)\n");
	printf("\t-u\t\tuse UDP transport instead of TCP\n");
	printf("\t-v\t\tshow version and exit\n");
	/* printf("\t-V [0-5]\tset verbosity level (default 3)\n"); */
	exit(EXIT_SUCCESS);
}

static int
dane_int_within_range(const char* arg, int max, const char* name)
{
	char* endptr; /* utility var for strtol usage */
	int val = strtol(arg, &endptr, 10);

	if ((val < 0 || val > max)
			|| (errno != 0 && val == 0) /* out of range */
			|| endptr == arg            /* no digits */
			|| *endptr != '\0'          /* more chars */
			) {
		fprintf(stderr, "<%s> should be in range [0-%d]\n", name, max);
		exit(EXIT_FAILURE);
	}
	return val;
}

struct dane_param_choice_struct {
	const char* name;
	int number;
};
typedef struct dane_param_choice_struct dane_param_choice;

dane_param_choice dane_certificate_usage_table[] = {
	{ "PKIX-TA"				,   0 },
	{ "CA constraint"			,   0 },
	{ "CA-constraint"			,   0 },
	{ "PKIX-EE"				,   1 },
	{ "Service certificate constraint"	,   1 },
	{ "Service-certificate-constraint"	,   1 },
	{ "DANE-TA"				,   2 },
	{ "Trust anchor assertion"		,   2 },
	{ "Trust-anchor-assertion"		,   2 },
	{ "anchor"				,   2 },
	{ "DANE-EE"				,   3 },
	{ "Domain-issued certificate"		,   3 },
	{ "Domain-issued-certificate"		,   3 },
	{ "PrivCert"				, 255 },
	{ NULL, -1 }
};

dane_param_choice dane_selector_table[] = {
	{ "Cert"		,   0 },
	{ "Full certificate"	,   0 },
	{ "Full-certificate"	,   0 },
	{ "certificate"		,   0 },
	{ "SPKI"		,   1 },
	{ "SubjectPublicKeyInfo",   1 },
	{ "PublicKey"		,   1 },
	{ "pubkey"		,   1 },
	{ "key"			,   1 },
	{ "PrivSel"		, 255 },
	{ NULL, -1 }
};

dane_param_choice dane_matching_type_table[] = {
	{ "Full"		,   0 },
	{ "no-hash-used"	,   0 },
	{ "no hash used"	,   0 },
	{ "SHA2-256"		,   1 },
	{ "sha256"		,   1 },
	{ "sha-256"		,   1 },
	{ "SHA2-512"		,   2 },
	{ "sha512"		,   2 },
	{ "sha-512"		,   2 },
	{ "PrivMatch"		, 255 },
	{ NULL, -1 }
};

static int
dane_int_within_range_table(const char* arg, int max, const char* name,
		dane_param_choice table[])
{
	dane_param_choice* t;

	if (*arg) {
		for (t = table; t->name; t++) {
			if (strncasecmp(arg, t->name, strlen(arg)) == 0) {
				return t->number;
			}
		}
	}
	return dane_int_within_range(arg, max, name);
}

static void
ssl_err(const char* s)
{
	fprintf(stderr, "error: %s\n", s);
	ERR_print_errors_fp(stderr);
	exit(EXIT_FAILURE);
}

static void
ldns_err(const char* s, ldns_status err)
{
	if (err == LDNS_STATUS_SSL_ERR) {
		ssl_err(s);
	} else {
		fprintf(stderr, "%s: %s\n", s, ldns_get_errorstr_by_id(err));
		exit(EXIT_FAILURE);
	}
}

static ldns_status
ssl_connect_and_get_cert_chain(
		X509** cert, STACK_OF(X509)** extra_certs,
	       	SSL* ssl, const char* name_str,
		ldns_rdf* address, uint16_t port,
		ldns_dane_transport transport)
{
	struct sockaddr_storage *a = NULL;
	size_t a_len = 0;
	int sock;
	int r;

	assert(cert != NULL);
	assert(extra_certs != NULL);

	a = ldns_rdf2native_sockaddr_storage(address, port, &a_len);
	switch (transport) {
	case LDNS_DANE_TRANSPORT_TCP:

		sock = socket((int)((struct sockaddr*)a)->sa_family,
				SOCK_STREAM, IPPROTO_TCP);
		break;

	case LDNS_DANE_TRANSPORT_UDP:

		sock = socket((int)((struct sockaddr*)a)->sa_family,
				SOCK_DGRAM, IPPROTO_UDP);
		break;

	case LDNS_DANE_TRANSPORT_SCTP:

		sock = socket((int)((struct sockaddr*)a)->sa_family,
				SOCK_STREAM, IPPROTO_SCTP);
		break;

	default:
		LDNS_FREE(a);
		return LDNS_STATUS_DANE_UNKNOWN_TRANSPORT;
	}
	if (sock == -1) {
		LDNS_FREE(a);
		return LDNS_STATUS_NETWORK_ERR;
	}
	if (connect(sock, (struct sockaddr*)a, (socklen_t)a_len) == -1) {
		LDNS_FREE(a);
		return LDNS_STATUS_NETWORK_ERR;
	}
	LDNS_FREE(a);
	if (! SSL_clear(ssl)) {
		close(sock);
		fprintf(stderr, "SSL_clear\n");
		return LDNS_STATUS_SSL_ERR;
	}
#ifdef SSL_CTRL_SET_TLSEXT_HOSTNAME
	(void) SSL_set_tlsext_host_name(ssl, name_str);
#endif
	SSL_set_connect_state(ssl);
	(void) SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY);
	if (! SSL_set_fd(ssl, sock)) {
		close(sock);
		fprintf(stderr, "SSL_set_fd\n");
		return LDNS_STATUS_SSL_ERR;
	}
	for (;;) {
		ERR_clear_error();
		if ((r = SSL_do_handshake(ssl)) == 1) {
			break;
		}
		r = SSL_get_error(ssl, r);
		if (r != SSL_ERROR_WANT_READ && r != SSL_ERROR_WANT_WRITE) {
			fprintf(stderr, "handshaking SSL_get_error: %d\n", r);
			return LDNS_STATUS_SSL_ERR;
		}
	}
	*cert = SSL_get_peer_certificate(ssl);
	*extra_certs = SSL_get_peer_cert_chain(ssl);

	return LDNS_STATUS_OK;
}


#ifdef USE_DANE_VERIFY
static void
ssl_interact(SSL* ssl)
{
	fd_set rfds;
	int maxfd;
	int sock;
	int r;

	char buf[BUFSIZE];
	char* bufptr;
	int to_write;
	int written;

	sock = SSL_get_fd(ssl);
	if (sock == -1) {
		return;
	}
	maxfd = (STDIN_FILENO > sock ? STDIN_FILENO : sock) + 1;
	for (;;) {
#ifndef S_SPLINT_S
		FD_ZERO(&rfds);
#endif /* splint */
		FD_SET(sock, &rfds);
		FD_SET(STDIN_FILENO, &rfds);

		r = select(maxfd, &rfds, NULL, NULL, NULL);
		if (r == -1) {
			perror("select");
			break;
		}
		if (FD_ISSET(sock, &rfds)) {
			to_write = SSL_read(ssl, buf, BUFSIZE);
			if (to_write <= 0) {
				r = SSL_get_error(ssl, to_write);
				if (r != SSL_ERROR_ZERO_RETURN) {
					fprintf(stderr,
						"reading SSL_get_error:"
						" %d\n", r);
				}
				break;
			}
			bufptr = buf;
			while (to_write > 0) {
				written = (int) fwrite(bufptr, 1, 
						(size_t) to_write, stdout);
				if (written == 0) {
					perror("fwrite");
					break;
				}
				to_write -= written;
				bufptr += written;
			}
		} /* if (FD_ISSET(sock, &rfds)) */

		if (FD_ISSET(STDIN_FILENO, &rfds)) {
			to_write = (int) read(STDIN_FILENO, buf, BUFSIZE - 1);
			if (to_write <= 0) {
				if (to_write == -1) {
					perror("read");
				}
				break;
			}
			if (buf[to_write - 1] == '\n') {
				buf[to_write - 1] = '\r';
				buf[to_write    ] = '\n';
				to_write += 1;
			}
			bufptr = buf;
			while (to_write > 0) {
				written = SSL_write(ssl, bufptr, to_write);
				if (written <= 0) {
					r = SSL_get_error(ssl, to_write);
					if (r != SSL_ERROR_ZERO_RETURN) {
						fprintf(stderr,
							"writing SSL_get_error"
							": %d\n", r);
					}
					break;
				}
				to_write -= written;
				bufptr += written;
			}
		} /* if (FD_ISSET(STDIN_FILENO, &rfds)) */

	} /* for (;;) */
}
#endif /* USE_DANE_VERIFY */


static ldns_rr_list*
rr_list_filter_rr_type(ldns_rr_list* l, ldns_rr_type t)
{
	size_t i;
	ldns_rr* rr;
	ldns_rr_list* r = ldns_rr_list_new();

	if (r == NULL) {
		return r;
	}
	for (i = 0; i < ldns_rr_list_rr_count(l); i++) {
		rr = ldns_rr_list_rr(l, i);
		if (ldns_rr_get_type(rr) == t) {
			if (! ldns_rr_list_push_rr(r, rr)) {
				ldns_rr_list_free(r);
				return NULL;
			}
		}
	}
	return r;
}


/* Return a copy of the list of tlsa records where the usage types
 * "CA constraint" are replaced with "Trust anchor assertion" and the usage
 * types "Service certificate constraint" are replaced with 
 * "Domain-issued certificate".
 *
 * This to check what would happen if PKIX validation was successful always.
 */
static ldns_rr_list*
dane_no_pkix_transform(const ldns_rr_list* tlas)
{
	size_t i;
	ldns_rr* rr;
	ldns_rr* new_rr;
	ldns_rdf* rdf;
	ldns_rr_list* r = ldns_rr_list_new();

	if (r == NULL) {
		return r;
	}
	for (i = 0; i < ldns_rr_list_rr_count(tlas); i++) {
		rr = ldns_rr_list_rr(tlas, i);
		if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_TLSA) {

			new_rr = ldns_rr_clone(rr);
			if (!new_rr) {
				ldns_rr_list_deep_free(r);
				return NULL;
			}
			switch(ldns_rdf2native_int8(ldns_rr_rdf(new_rr, 0))) {

			case LDNS_TLSA_USAGE_CA_CONSTRAINT:

				rdf = ldns_native2rdf_int8(LDNS_RDF_TYPE_INT8,
			(uint8_t) LDNS_TLSA_USAGE_TRUST_ANCHOR_ASSERTION);
				if (! rdf) {
					ldns_rr_free(new_rr);
					ldns_rr_list_deep_free(r);
					return NULL;
				}
				(void) ldns_rr_set_rdf(new_rr, rdf, 0);
				break;


			case LDNS_TLSA_USAGE_SERVICE_CERTIFICATE_CONSTRAINT:

				rdf = ldns_native2rdf_int8(LDNS_RDF_TYPE_INT8,
			(uint8_t) LDNS_TLSA_USAGE_DOMAIN_ISSUED_CERTIFICATE);
				if (! rdf) {
					ldns_rr_free(new_rr);
					ldns_rr_list_deep_free(r);
					return NULL;
				}
				(void) ldns_rr_set_rdf(new_rr, rdf, 0);
				break;


			default:
				break;
			}
			if (! ldns_rr_list_push_rr(r, new_rr)) {
				ldns_rr_free(new_rr);
				ldns_rr_list_deep_free(r);
				return NULL;
			}
		}
	}
	return r;
}

static void
print_rr_as_TYPEXXX(FILE* out, ldns_rr* rr)
{
	size_t i, sz;
	ldns_status s;
	ldns_buffer* buf = ldns_buffer_new(LDNS_MAX_PACKETLEN);
	char* str;

	ldns_buffer_clear(buf);
	s = ldns_rdf2buffer_str_dname(buf, ldns_rr_owner(rr));
	LDNS_ERR(s, "could not ldns_rdf2buffer_str_dname");
	ldns_buffer_printf(buf, "\t%d", ldns_rr_ttl(rr));
	ldns_buffer_printf(buf, "\t");
	s = ldns_rr_class2buffer_str(buf, ldns_rr_get_class(rr));
	LDNS_ERR(s, "could not ldns_rr_class2buffer_str");
	ldns_buffer_printf(buf, "\tTYPE%d", ldns_rr_get_type(rr));
	sz = 0;
	for (i = 0; i < ldns_rr_rd_count(rr); i++) {
		 sz += ldns_rdf_size(ldns_rr_rdf(rr, i));
	}
	ldns_buffer_printf(buf, "\t\\# %d ", sz);
	for (i = 0; i < ldns_rr_rd_count(rr); i++) {
		s = ldns_rdf2buffer_str_hex(buf, ldns_rr_rdf(rr, i));
		LDNS_ERR(s, "could not ldns_rdf2buffer_str_hex");
	}
	str = ldns_buffer_export2str(buf);
	ldns_buffer_free(buf);
	fprintf(out, "%s\n", str);
	LDNS_FREE(str);
}

static void
print_rr_list_as_TYPEXXX(FILE* out, ldns_rr_list* l)
{
	size_t i;

	for (i = 0; i < ldns_rr_list_rr_count(l); i++) {
		print_rr_as_TYPEXXX(out, ldns_rr_list_rr(l, i));
	}
}

static ldns_status
read_key_file(const char *filename, ldns_rr_list *keys)
{
	ldns_status status = LDNS_STATUS_ERR;
	ldns_rr *rr;
	FILE *fp;
	uint32_t my_ttl = 0;
	ldns_rdf *my_origin = NULL;
	ldns_rdf *my_prev = NULL;
	int line_nr;

	if (!(fp = fopen(filename, "r"))) {
		return LDNS_STATUS_FILE_ERR;
	}
	while (!feof(fp)) {
		status = ldns_rr_new_frm_fp_l(&rr, fp, &my_ttl, &my_origin,
				&my_prev, &line_nr);

		if (status == LDNS_STATUS_OK) {

			if (   ldns_rr_get_type(rr) == LDNS_RR_TYPE_DS
			    || ldns_rr_get_type(rr) == LDNS_RR_TYPE_DNSKEY)

				ldns_rr_list_push_rr(keys, rr);

		} else if (   status == LDNS_STATUS_SYNTAX_EMPTY
		           || status == LDNS_STATUS_SYNTAX_TTL
		           || status == LDNS_STATUS_SYNTAX_ORIGIN
		           || status == LDNS_STATUS_SYNTAX_INCLUDE)

			status = LDNS_STATUS_OK;
		else
			break;
	}
	fclose(fp);
	return status;
}


static ldns_status
dane_setup_resolver(ldns_resolver** res, ldns_rdf* nameserver_addr,
		ldns_rr_list* keys, bool dnssec_off)
{
	ldns_status s = LDNS_STATUS_OK;

	assert(res != NULL);

	if (nameserver_addr) {
		*res = ldns_resolver_new();
		if (*res) {
			s = ldns_resolver_push_nameserver(*res, nameserver_addr);
		} else {
			s = LDNS_STATUS_MEM_ERR;
		}
	} else {
	        s = ldns_resolver_new_frm_file(res, NULL);
	}
	if (s == LDNS_STATUS_OK) {
		ldns_resolver_set_dnssec(*res, ! dnssec_off);

		if (keys && ldns_rr_list_rr_count(keys) > 0) {
			/* anchors must trigger signature chasing */
			ldns_resolver_set_dnssec_anchors(*res, keys);
			ldns_resolver_set_dnssec_cd(*res, true);
		}
	}
	return s;
}


static ldns_status
dane_query(ldns_rr_list** rrs, ldns_resolver* r,
		ldns_rdf *name, ldns_rr_type t, ldns_rr_class c,
		bool insecure_is_ok)
{
	ldns_pkt* p = NULL;
	ldns_rr_list* keys = NULL;
	ldns_rr_list* rrsigs  = NULL;
	ldns_rdf* signame = NULL;
	ldns_status s;

	assert(rrs != NULL);

	p = ldns_resolver_query(r, name, t, c, LDNS_RD);
	if (! p) {
		return LDNS_STATUS_MEM_ERR;
	}
	*rrs = ldns_pkt_rr_list_by_type(p, t, LDNS_SECTION_ANSWER);

	if (! ldns_resolver_dnssec(r)) { /* DNSSEC explicitly disabled,
					    anything goes */
		ldns_pkt_free(p);
		return LDNS_STATUS_OK;
	}
	if (ldns_rr_list_rr_count(*rrs) == 0) { /* assert(*rrs == NULL) */

		if (ldns_pkt_get_rcode(p) == LDNS_RCODE_SERVFAIL) {

			ldns_pkt_free(p);
			return LDNS_STATUS_DANE_BOGUS;
		} else {
			ldns_pkt_free(p);
			return LDNS_STATUS_OK;
		}
	}
	/* We have answers and we have dnssec. */

	if (! ldns_pkt_cd(p)) { /* we act as stub resolver (no sigchase) */

		if (! ldns_pkt_ad(p)) { /* Not secure */

			goto insecure;
		}
		ldns_pkt_free(p);
		return LDNS_STATUS_OK;
	}

	/* sigchase */

	/* TODO: handle cname reference check */

	rrsigs = ldns_pkt_rr_list_by_type(p,
			LDNS_RR_TYPE_RRSIG,
			LDNS_SECTION_ANSWER);

	if (! rrsigs || ldns_rr_list_rr_count(rrsigs) == 0) {
		goto insecure;
	}

	signame = ldns_rr_rrsig_signame(ldns_rr_list_rr(rrsigs, 0));
	if (! signame) {
		s = LDNS_STATUS_ERR;
		goto error;
	}
	/* First try with the keys we already have */
	s = ldns_verify(*rrs, rrsigs, ldns_resolver_dnssec_anchors(r), NULL);
	if (s == LDNS_STATUS_OK) {
		goto cleanup;
	}
	/* Fetch the necessary keys and recheck */
	keys = ldns_fetch_valid_domain_keys(r, signame,
			ldns_resolver_dnssec_anchors(r), &s);

	if (s != LDNS_STATUS_OK) {
		goto error;
	}
	if (ldns_rr_list_rr_count(keys) == 0) { /* An insecure island */
		goto insecure;
	}
	s = ldns_verify(*rrs, rrsigs, keys, NULL);
	switch (s) {
	case LDNS_STATUS_CRYPTO_BOGUS: goto bogus;
	case LDNS_STATUS_OK          : goto cleanup;
	default                      : break;
	}
insecure:
	s = LDNS_STATUS_DANE_INSECURE;
bogus:
	if (! insecure_is_ok) {
error:
		ldns_rr_list_deep_free(*rrs);
		*rrs = ldns_rr_list_new();
	}
cleanup:
	if (keys) {
		ldns_rr_list_deep_free(keys);
	}
	if (rrsigs) {
		ldns_rr_list_deep_free(rrsigs);
	}
	ldns_pkt_free(p);
	return s;
}


static ldns_rr_list*
dane_lookup_addresses(ldns_resolver* res, ldns_rdf* dname,
		int ai_family)
{
	ldns_status s;
	ldns_rr_list *as = NULL;
	ldns_rr_list *aaas = NULL;
	ldns_rr_list *r = ldns_rr_list_new();

	if (r == NULL) {
		MEMERR("ldns_rr_list_new");
	}
	if (ai_family == AF_UNSPEC || ai_family == AF_INET) {

		s = dane_query(&as, res,
				dname, LDNS_RR_TYPE_A, LDNS_RR_CLASS_IN,
			       	true);

		if (s == LDNS_STATUS_DANE_INSECURE &&
			ldns_rr_list_rr_count(as) > 0) {
			fprintf(stderr, "Warning! Insecure IPv4 addresses. "
					"Continuing with them...\n");

		} else if (s == LDNS_STATUS_DANE_BOGUS ||
				LDNS_STATUS_CRYPTO_BOGUS == s) {
			fprintf(stderr, "Warning! Bogus IPv4 addresses. "
					"Discarding...\n");
			ldns_rr_list_deep_free(as);
			as = ldns_rr_list_new();

		} else if (s != LDNS_STATUS_OK) {
			LDNS_ERR(s, "dane_query");

		}
		if (! ldns_rr_list_push_rr_list(r, as)) {
			MEMERR("ldns_rr_list_push_rr_list");
		}
	}
	if (ai_family == AF_UNSPEC || ai_family == AF_INET6) {

		s = dane_query(&aaas, res,
				dname, LDNS_RR_TYPE_AAAA, LDNS_RR_CLASS_IN,
			       	true);

		if (s == LDNS_STATUS_DANE_INSECURE &&
			ldns_rr_list_rr_count(aaas) > 0) {
			fprintf(stderr, "Warning! Insecure IPv6 addresses. "
					"Continuing with them...\n");

		} else if (s == LDNS_STATUS_DANE_BOGUS ||
				LDNS_STATUS_CRYPTO_BOGUS == s) {
			fprintf(stderr, "Warning! Bogus IPv6 addresses. "
					"Discarding...\n");
			ldns_rr_list_deep_free(aaas);
			aaas = ldns_rr_list_new();

		} else if (s != LDNS_STATUS_OK) {
			LDNS_ERR(s, "dane_query");

		}
		if (! ldns_rr_list_push_rr_list(r, aaas)) {
			MEMERR("ldns_rr_list_push_rr_list");
		}
	}
	return r;
}

static ldns_status
dane_read_tlsas_from_file(ldns_rr_list** tlsas,
		char* filename, ldns_rdf* origin)
{
	FILE* fp = NULL;
	ldns_rr* rr = NULL;
	ldns_rdf *my_origin = NULL;
	ldns_rdf *my_prev = NULL;
	ldns_rdf *origin_lc = NULL;
	int line_nr;
	ldns_status s = LDNS_STATUS_MEM_ERR;

	assert(tlsas != NULL);
	assert(filename != NULL);

	if (strcmp(filename, "-") == 0) {
		fp = stdin;
	} else {
		fp = fopen(filename, "r");
		if (!fp) {
			fprintf(stderr, "Unable to open %s: %s\n",
					filename, strerror(errno));
			exit(EXIT_FAILURE);
		}
	}
	if (origin) {
		my_origin = ldns_rdf_clone(origin);
		if (! my_origin) {
			goto error;
		}
		my_prev   = ldns_rdf_clone(origin);
		if (! my_prev) {
			goto error;
		}
		origin_lc = ldns_rdf_clone(origin);
		if (! origin_lc) {
			goto error;
		}
		ldns_dname2canonical(origin_lc);
	}
	*tlsas = ldns_rr_list_new();
	if (! *tlsas) {
		goto error;
	}
	while (! feof(fp)) {
		s = ldns_rr_new_frm_fp_l(&rr, fp, NULL,
				&my_origin, &my_prev, &line_nr);
		if (s != LDNS_STATUS_OK) {
			goto error;
		}
		if (ldns_rr_get_type(rr) == LDNS_RR_TYPE_TLSA) {
			ldns_dname2canonical(ldns_rr_owner(rr));
			if (! origin || ldns_dname_compare(ldns_rr_owner(rr),
						origin_lc) == 0) {
				if (ldns_rr_list_push_rr(*tlsas, rr)) {
					continue;
				} else {
					s = LDNS_STATUS_MEM_ERR;
					goto error;
				}
			}
		}
		ldns_rr_free(rr);
	}

	ldns_rdf_deep_free(origin_lc);
	ldns_rdf_deep_free(my_prev);
	ldns_rdf_deep_free(my_origin);
	fclose(fp);

	return LDNS_STATUS_OK;

error:
	if (*tlsas) {
		ldns_rr_list_deep_free(*tlsas);
		*tlsas = NULL;
	}
	if (origin_lc) {
		ldns_rdf_deep_free(origin_lc);
	}
	if (my_prev) {
		ldns_rdf_deep_free(my_prev);
	}
	if (my_origin) {
		ldns_rdf_deep_free(my_origin);
	}
	if (fp && fp != stdin) {
		fclose(fp);
	}
	return s;
}

static bool
dane_wildcard_label_cmp(uint8_t iw, const char* w, uint8_t il, const char* l)
{
	if (iw == 0) { /* End of match label */
		if (il == 0) { /* And end in the to be matched label */
			return true;
		}
		return false;
	}
	do {
		if (*w == '*') {
			if (iw == 1) { /* '*' is the last match char,
					  remainder matches wildcard */
				return true;
			}
			while (il > 0) { /* more to match? */

				if (w[1] == *l) { /* Char after '*' matches.
						   * Recursion for backtracking
						   */
					if (dane_wildcard_label_cmp(
								iw - 1, w + 1,
								il    , l)) {
						return true;
					}
				}
				l += 1;
				il -= 1;
			}
		}
		/* Skip up till next wildcard (if possible) */
		while (il > 0 && iw > 0 && *w != '*' && *w == *l) {
			w += 1;
			l += 1;
			il -= 1;
			iw -= 1;
		}
	} while (iw > 0 && *w == '*' &&  /* More to match a next wildcard? */
			(il > 0 || iw == 1));

	return iw == 0 && il == 0;
}

static bool
dane_label_matches_label(ldns_rdf* w, ldns_rdf* l)
{
	uint8_t iw;
	uint8_t il;

	iw = ldns_rdf_data(w)[0];
	il = ldns_rdf_data(l)[0];
	return dane_wildcard_label_cmp(
			iw, (const char*)ldns_rdf_data(w) + 1,
			il, (const char*)ldns_rdf_data(l) + 1);
}

static bool
dane_name_matches_server_name(const char* name_str, ldns_rdf* server_name)
{
	ldns_rdf* name;
	uint8_t nn, ns, i;
	ldns_rdf* ln;
	ldns_rdf* ls;
	
	name = ldns_dname_new_frm_str((const char*)name_str);
	if (! name) {
		LDNS_ERR(LDNS_STATUS_ERR, "ldns_dname_new_frm_str");
	}
	nn = ldns_dname_label_count(name);
	ns = ldns_dname_label_count(server_name);
	if (nn != ns) {
		ldns_rdf_free(name);
		return false;
	}
	ldns_dname2canonical(name);
	for (i = 0; i < nn; i++) {
		ln = ldns_dname_label(name, i);
		if (! ln) {
			return false;
		}
		ls = ldns_dname_label(server_name, i);
		if (! ls) {
			ldns_rdf_free(ln);
			return false;
		}
		if (! dane_label_matches_label(ln, ls)) {
			ldns_rdf_free(ln);
			ldns_rdf_free(ls);
			return false;
		}
		ldns_rdf_free(ln);
		ldns_rdf_free(ls);
	}
	return true;
}

static bool
dane_X509_any_subject_alt_name_matches_server_name(
		X509 *cert, ldns_rdf* server_name)
{
	GENERAL_NAMES* names;
	GENERAL_NAME*  name;
	unsigned char* subject_alt_name_str = NULL;
	int i, n;

	names = X509_get_ext_d2i(cert, NID_subject_alt_name, 0, 0 );
	if (! names) { /* No subjectAltName extension */
		return false;
	}
	n = sk_GENERAL_NAME_num(names);
	for (i = 0; i < n; i++) {
		name = sk_GENERAL_NAME_value(names, i);
		if (name->type == GEN_DNS) {
			(void) ASN1_STRING_to_UTF8(&subject_alt_name_str,
					name->d.dNSName);
			if (subject_alt_name_str) {
				if (dane_name_matches_server_name((char*)
							subject_alt_name_str,
							server_name)) {
					OPENSSL_free(subject_alt_name_str);
					return true;
				}
				OPENSSL_free(subject_alt_name_str);
			}
		}
	}
	/* sk_GENERAL_NAMES_pop_free(names, sk_GENERAL_NAME_free); */
	return false;
}

static bool
dane_X509_subject_name_matches_server_name(X509 *cert, ldns_rdf* server_name)
{
	X509_NAME* subject_name;
	int i;
	X509_NAME_ENTRY* entry;
	ASN1_STRING* entry_data;
	unsigned char* subject_name_str = NULL;
	bool r;
 
	subject_name = X509_get_subject_name(cert);
	if (! subject_name ) {
		ssl_err("could not X509_get_subject_name");
	}
	i = X509_NAME_get_index_by_NID(subject_name, NID_commonName, -1);
	entry = X509_NAME_get_entry(subject_name, i);
	entry_data = X509_NAME_ENTRY_get_data(entry);
	(void) ASN1_STRING_to_UTF8(&subject_name_str, entry_data);
	if (subject_name_str) {
		r = dane_name_matches_server_name(
				(char*)subject_name_str, server_name);
		OPENSSL_free(subject_name_str);
		return r;
	} else {
		return false;
	}
}

static bool
dane_verify_server_name(X509* cert, ldns_rdf* server_name)
{
	ldns_rdf* server_name_lc;
	bool r;
	server_name_lc = ldns_rdf_clone(server_name);
	if (! server_name_lc) {
		LDNS_ERR(LDNS_STATUS_MEM_ERR, "ldns_rdf_clone");
	}
	ldns_dname2canonical(server_name_lc);
	r = dane_X509_any_subject_alt_name_matches_server_name(
			cert, server_name_lc) || 
	    dane_X509_subject_name_matches_server_name(
			cert, server_name_lc);
	ldns_rdf_free(server_name_lc);
	return r;
}

static void
dane_create(ldns_rr_list* tlsas, ldns_rdf* tlsa_owner,
		ldns_tlsa_certificate_usage certificate_usage, int offset,
		ldns_tlsa_selector          selector,
		ldns_tlsa_matching_type     matching_type,
		X509* cert, STACK_OF(X509)* extra_certs,
		X509_STORE* validate_store,
		bool verify_server_name, ldns_rdf* name)
{
	ldns_status s;
	X509* selected_cert;
	ldns_rr* tlsa_rr;

	if (verify_server_name && ! dane_verify_server_name(cert, name)) {
		fprintf(stderr, "The certificate does not match the "
				"server name\n");
		exit(EXIT_FAILURE);
	}

	s = ldns_dane_select_certificate(&selected_cert,
			cert, extra_certs, validate_store,
			certificate_usage, offset);
	LDNS_ERR(s, "could not select certificate");

	s = ldns_dane_create_tlsa_rr(&tlsa_rr,
			certificate_usage, selector, matching_type,
			selected_cert);
	LDNS_ERR(s, "could not create tlsa rr");

	ldns_rr_set_owner(tlsa_rr, ldns_rdf_clone(tlsa_owner));
			     
	if (! ldns_rr_list_contains_rr(tlsas, tlsa_rr)) {
		if (! ldns_rr_list_push_rr(tlsas, tlsa_rr)) {
			MEMERR("ldns_rr_list_push_rr");
		}
	}
}

#if defined(USE_DANE_VERIFY) && ( OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL) )
static bool
dane_verify(ldns_rr_list* tlsas, ldns_rdf* address,
		X509* cert, STACK_OF(X509)* extra_certs,
		X509_STORE* validate_store,
		bool verify_server_name, ldns_rdf* name,
		bool assume_pkix_validity)
{
	ldns_status s;
	char* address_str = NULL;

	s = ldns_dane_verify(tlsas, cert, extra_certs, validate_store);
	if (address) {
		address_str = ldns_rdf2str(address);
		fprintf(stdout, "%s", address_str ? address_str : "<address>");
		free(address_str);
	} else {
		X509_NAME_print_ex_fp(stdout,
				X509_get_subject_name(cert), 0, 0);
	}
	if (s == LDNS_STATUS_OK) {
		if (verify_server_name &&
				! dane_verify_server_name(cert, name)) {

			fprintf(stdout, " did not dane-validate, because:"
					" the certificate name did not match"
					" the server name\n");
			return false;
		}
		fprintf(stdout, " dane-validated successfully\n");
		return true;
	} else if (assume_pkix_validity &&
			s == LDNS_STATUS_DANE_PKIX_DID_NOT_VALIDATE) {
		fprintf(stdout, " dane-validated successfully,"
				" because PKIX is assumed valid\n");
		return true;
	}
	fprintf(stdout, " did not dane-validate, because: %s\n",
			ldns_get_errorstr_by_id(s));
	return false;
}
#endif /* defined(USE_DANE_VERIFY) && OPENSSL_VERSION_NUMBER < 0x10100000 */

#if OPENSSL_VERSION_NUMBER >= 0x10100000  && ! defined(HAVE_LIBRESSL)
static int _ldns_tls_verify_always_ok(int ok, X509_STORE_CTX *ctx)
{
	(void)ok;
	(void)ctx;
	return 1;
}
#endif

/**
 * Return either an A or AAAA rdf, based on the given
 * string. If it it not a valid ip address, return null.
 *
 * Caller receives ownership of returned rdf (if not null),
 * and must free it.
 */
static inline ldns_rdf* rdf_addr_frm_str(const char* str) {
	ldns_rdf *a = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_A, str);
	if (!a) {
		a = ldns_rdf_new_frm_str(LDNS_RDF_TYPE_AAAA, str);
	}
	return a;
}


int
main(int argc, char* const* argv)
{
	int c;
	enum { UNDETERMINED, VERIFY, CREATE } mode = UNDETERMINED;

	ldns_status   s;
	size_t        i;

#if OPENSSL_VERSION_NUMBER >= 0x10100000 && ! defined(HAVE_LIBRESSL)
	size_t        j, usable_tlsas = 0;
# ifdef USE_DANE_VERIFY
	X509_STORE_CTX *store_ctx = NULL;
# endif /* USE_DANE_VERIFY */
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000 */

	bool print_tlsa_as_type52   = false;
	bool assume_dnssec_validity = false;
	bool assume_pkix_validity   = false;
	bool verify_server_name     = true;
	bool interact               = false;

#if HAVE_DANE_CA_FILE
	const char* CAfile    = LDNS_DANE_CA_FILE;
#else
	const char* CAfile    = NULL;
#endif
#if HAVE_DANE_CA_PATH
	const char* CApath    = LDNS_DANE_CA_PATH;
#else
	const char* CApath    = NULL;
#endif
	char* cert_file = NULL;
	X509* cert                  = NULL;
	STACK_OF(X509)* extra_certs = NULL;
	
	ldns_rr_list*  keys = ldns_rr_list_new();
	size_t        nkeys = 0;
	bool    do_sigchase = false;

	ldns_rr_list* addresses = ldns_rr_list_new();
	ldns_rr*      address_rr;
	ldns_rdf*     address;

	int           ai_family = AF_UNSPEC;
	int           transport = LDNS_DANE_TRANSPORT_TCP;

	char*         name_str = NULL;	/* suppress uninitialized warning */
	ldns_rdf*     name;
	uint16_t      port = 0;		/* suppress uninitialized warning */

	ldns_resolver* res            = NULL;
	ldns_rdf*      nameserver_rdf = NULL;
	ldns_rdf*      tlsa_owner     = NULL;
	char*          tlsa_owner_str = NULL;
	ldns_rr_list*  tlsas          = NULL;
	char*          tlsas_file     = NULL;

	/* For extracting service port and transport from tla_owner. */
	ldns_rdf*      port_rdf       = NULL;
	char*          port_str       = NULL;
	ldns_rdf*      transport_rdf  = NULL;
	char*          transport_str  = NULL;

	ldns_rr_list*  originals      = NULL; /* original tlsas (before
					       * transform), but also used
					       * as temporary.
					       */

	ldns_tlsa_certificate_usage certificate_usage = 666;
	int                         offset            =  -1;
	ldns_tlsa_selector          selector          = 666;
	ldns_tlsa_matching_type     matching_type     = 666;
	

	X509_STORE *store  = NULL;

	SSL_CTX* ctx = NULL;
	SSL*     ssl = NULL;

	int  no_tlsas_exit_status  = EXIT_SUCCESS;
	int           exit_success = EXIT_SUCCESS; 

	bool success = true;

	if (! keys || ! addresses) {
		MEMERR("ldns_rr_list_new");
	}
	while((c = getopt(argc, argv, "46a:bc:df:hik:no:p:r:sSt:TuvV:")) != -1){
		switch(c) {
		case 'h':
			print_usage("ldns-dane");
			break;
		case '4':
			ai_family = AF_INET;
			break;
		case '6':
			ai_family = AF_INET6;
			break;
		case 'r':
			if (nameserver_rdf) {
				fprintf(stderr, "Can only specify -r once\n");
				exit(EXIT_FAILURE);
			}
			nameserver_rdf = rdf_addr_frm_str(optarg);
			if (!nameserver_rdf) {
				fprintf(stderr,
				        "Could not interpret address %s\n",
				        optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 'a':
			s = ldns_str2rdf_a(&address, optarg);
			if (s == LDNS_STATUS_OK) {
				address_rr = ldns_rr_new_frm_type(
						LDNS_RR_TYPE_A);
			} else {
				s = ldns_str2rdf_aaaa(&address, optarg);
				if (s == LDNS_STATUS_OK) {
					address_rr = ldns_rr_new_frm_type(
							LDNS_RR_TYPE_AAAA);
				} else {
					fprintf(stderr,
						"Could not interpret address "
						"%s\n",
						optarg);
					exit(EXIT_FAILURE);
				}
			}
			(void) ldns_rr_a_set_address(address_rr, address);
			for (i = 0; i < ldns_rr_list_rr_count(addresses); i++){
				if (ldns_rdf_compare(address,
				     ldns_rr_a_address(
				      ldns_rr_list_rr(addresses, i))) == 0) {
					break;
				}
			}
			if (i >= ldns_rr_list_rr_count(addresses)) {
				if (! ldns_rr_list_push_rr(addresses,
							address_rr)) {
					MEMERR("ldns_rr_list_push_rr");
				}
			}
			break;
		case 'b':
			print_tlsa_as_type52 = true;
			/* TODO: do it with output formats... maybe... */
			break;
		case 'c':
			cert_file = optarg; /* checking in SSL stuff below */
			break;
		case 'd':
			assume_dnssec_validity = true;
			break;
		case 'f':
			CAfile = optarg;
			break;
		case 'i':
			interact = true;
			break;
		case 'k':
			s = read_key_file(optarg, keys);
			if (s == LDNS_STATUS_FILE_ERR) {
				fprintf(stderr, "Error opening %s: %s\n",
						optarg, strerror(errno));
			}
			LDNS_ERR(s, "Could not parse key file");
			if (ldns_rr_list_rr_count(keys) == nkeys) {
				fprintf(stderr, "No keys found in file"
						" %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			nkeys = ldns_rr_list_rr_count(keys);
			break;
		case 'n':
			verify_server_name = false;
			break;
		case 'o':
			offset = atoi(optarg); /* todo check if all numeric */
			break;
		case 'p':
			CApath = optarg;
			break;
		case 's':
			assume_pkix_validity = true;
			break;
		case 'S':
			do_sigchase = true;
			break;
		case 't':
			tlsas_file = optarg;
			break;
		case 'T':
			no_tlsas_exit_status = NO_TLSAS_EXIT_STATUS;
			break;
		case 'u':
			transport = LDNS_DANE_TRANSPORT_UDP;
			break;
		case 'v':
			printf("ldns-dane version %s (ldns version %s)\n",
					LDNS_VERSION, ldns_version());
			exit(EXIT_SUCCESS);
			break;
/*		case 'V':
			verbosity = atoi(optarg);
			break;
 */
		}
	}

	/* Filter out given IPv4 addresses when -6 was given, 
	 * and IPv6 addresses when -4 was given.
	 */
	if (ldns_rr_list_rr_count(addresses) > 0 &&
			ai_family != AF_UNSPEC) {
		originals = addresses;
		addresses = rr_list_filter_rr_type(originals,
				(ai_family == AF_INET
				 ? LDNS_RR_TYPE_A : LDNS_RR_TYPE_AAAA));
		ldns_rr_list_free(originals);
		if (addresses == NULL) {
			MEMERR("rr_list_filter_rr_type");
		}
		if (ldns_rr_list_rr_count(addresses) == 0) {
			fprintf(stderr,
				"No addresses of the specified type remain\n");
			exit(EXIT_FAILURE);
		}
	}

	if (do_sigchase) {
		if (nkeys == 0) {
			(void) read_key_file(LDNS_TRUST_ANCHOR_FILE, keys);
			nkeys = ldns_rr_list_rr_count(keys);

			if (nkeys == 0) {
				fprintf(stderr, "Unable to chase "
						"signature without keys.\n");
				exit(EXIT_FAILURE);
			}
		}
	} else {
		keys = NULL;
	}

	argc -= optind;
	argv += optind;

	if (argc == 0) {

		print_usage("ldns-dane");
	}
	if (strncasecmp(*argv, "create", strlen(*argv)) == 0) {

		mode = CREATE;
		argc--;
		argv++;

#ifdef USE_DANE_VERIFY
	} else if (strncasecmp(*argv, "verify", strlen(*argv)) == 0) {

		mode = VERIFY;
		argc--;
		argv++;

	} else {
		fprintf(stderr, "Specify create or verify mode\n");
#else
	} else {
		fprintf(stderr, "Specify create mode\n");
#endif
		exit(EXIT_FAILURE);
	}

#ifndef USE_DANE_VERIFY
	(void)transport_str;
	(void)transport_rdf;
	(void)port_str;
	(void)port_rdf;
	(void)interact;
#else
	if (mode == VERIFY && argc == 0) {

		if (! tlsas_file) {
			fprintf(stderr, "ERROR! Nothing given to verify\n");
			exit(EXIT_FAILURE);
		}
		s = dane_read_tlsas_from_file(&tlsas, tlsas_file, NULL);
		LDNS_ERR(s, "could not read tlsas from file");

		/* extract port, transport and hostname from TLSA owner name */

		if (ldns_rr_list_rr_count(tlsas) == 0) {

			fprintf(stderr, "ERROR! No TLSA records to extract "
					"service port, transport and hostname"
					"\n");
			exit(EXIT_FAILURE);
		}
		tlsa_owner = ldns_rr_list_owner(tlsas);
		if (ldns_dname_label_count(tlsa_owner) < 2) {
			fprintf(stderr, "ERROR! To few labels in TLSA owner\n");
			exit(EXIT_FAILURE);
		}
		do {
			s = LDNS_STATUS_MEM_ERR;
			port_rdf = ldns_dname_label(tlsa_owner, 0);
			if (! port_rdf) {
				break;
			}
			port_str = ldns_rdf2str(port_rdf);
			if (! port_str) {
				break;
			}
			if (*port_str != '_') {
				fprintf(stderr, "ERROR! Badly formatted "
						"service port label in the "
						"TLSA owner name\n");
				exit(EXIT_FAILURE);
			}
			if (port_str[strlen(port_str) - 1] == '.') {
				port_str[strlen(port_str) - 1] = '\000';
			}
			port = (uint16_t) dane_int_within_range(
					port_str + 1, 65535, "port");
			s = LDNS_STATUS_OK;
		} while (false);
		LDNS_ERR(s, "could not extract service port from TLSA owner");

		do {
			s = LDNS_STATUS_MEM_ERR;
			transport_rdf = ldns_dname_label(tlsa_owner, 1);
			if (! transport_rdf) {
				break;
			}
			transport_str = ldns_rdf2str(transport_rdf);
			if (! transport_str) {
				break;
			}
			if (transport_str[strlen(transport_str) - 1] == '.') {
				transport_str[strlen(transport_str) - 1] =
					'\000';
			}
			if (strcmp(transport_str, "_tcp") == 0) {

				transport = LDNS_DANE_TRANSPORT_TCP;

			} else if (strcmp(transport_str, "_udp") == 0) {

				transport = LDNS_DANE_TRANSPORT_UDP;

			} else if (strcmp(transport_str, "_sctp") == 0) {

				transport = LDNS_DANE_TRANSPORT_SCTP;

			} else {
				fprintf(stderr, "ERROR! Badly formatted "
						"transport label in the "
						"TLSA owner name\n");
				exit(EXIT_FAILURE);
			}
			s = LDNS_STATUS_OK;
			break;
		} while(false);
		LDNS_ERR(s, "could not extract transport from TLSA owner");

		tlsa_owner_str = ldns_rdf2str(tlsa_owner);
		if (! tlsa_owner_str) {
			MEMERR("ldns_rdf2str");
		}
		name = ldns_dname_clone_from(tlsa_owner, 2);
		if (! name) {
			MEMERR("ldns_dname_clone_from");
		}
		name_str = ldns_rdf2str(name);
		if (! name_str) {
			MEMERR("ldns_rdf2str");
		}


	} else 
#endif /* USE_DANE_VERIFY */
		if (argc < 2) {

		print_usage("ldns-dane");

	} else {
		name_str = *argv++; argc--;
		s = ldns_str2rdf_dname(&name, name_str);
		LDNS_ERR(s, "could not ldns_str2rdf_dname");

		port = (uint16_t)dane_int_within_range(*argv++, 65535, "port");
		--argc;

		s = ldns_dane_create_tlsa_owner(&tlsa_owner,
				name, port, transport);
		LDNS_ERR(s, "could not create TLSA owner name");
		tlsa_owner_str = ldns_rdf2str(tlsa_owner);
		if (! tlsa_owner_str) {
			MEMERR("ldns_rdf2str");
		}
	}

	switch (mode) {
	case VERIFY:
		if (argc > 0) {

			print_usage("ldns-dane");
		}
		if (tlsas_file) {

			s = dane_read_tlsas_from_file(&tlsas, tlsas_file,
					tlsa_owner);
			LDNS_ERR(s, "could not read tlas from file");
		} else {
			/* lookup tlsas */
			s = dane_setup_resolver(&res, nameserver_rdf,
			                        keys, assume_dnssec_validity);
			LDNS_ERR(s, "could not dane_setup_resolver");
			s = dane_query(&tlsas, res, tlsa_owner,
					LDNS_RR_TYPE_TLSA, LDNS_RR_CLASS_IN,
					false);
			ldns_resolver_free(res);
		}

		if (s == LDNS_STATUS_DANE_INSECURE) {

			fprintf(stderr, "Warning! TLSA records for %s "
				"were found, but were insecure.\n"
				"PKIX validation without DANE will be "
				"performed. If you wish to perform DANE\n"
				"even though the RR's are insecure, "
				"use the -d option.\n", tlsa_owner_str);

			exit_success = no_tlsas_exit_status;

		} else if (s != LDNS_STATUS_OK) {

			ldns_err("dane_query", s);

		} else if (ldns_rr_list_rr_count(tlsas) == 0) {

			fprintf(stderr, "Warning! No TLSA records for %s "
				"were found.\n"
				"PKIX validation without DANE will be "
				"performed.\n", ldns_rdf2str(tlsa_owner));

			exit_success = no_tlsas_exit_status;

		} else if (assume_pkix_validity) { /* number of  tlsa's > 0 */
			
			/* transform type "CA constraint" to "Trust anchor
			 * assertion" and "Service Certificate Constraint"
			 * to "Domain Issues Certificate"
			 */
			originals = tlsas;
			tlsas = dane_no_pkix_transform(originals);
		}

		break;

	case CREATE:
		if (argc > 0) {
			certificate_usage = dane_int_within_range_table(
					*argv++, 3, "certificate usage",
					dane_certificate_usage_table);
			argc--;
		} else {
			certificate_usage = LDNS_TLSA_USAGE_DANE_EE;
		}
		if (argc > 0) {
			selector = dane_int_within_range_table(
					*argv++, 1, "selector",
					dane_selector_table);
			argc--;
		} else {
			selector = LDNS_TLSA_SELECTOR_SPKI;
		}
		if (argc > 0) {
			matching_type = dane_int_within_range_table(
					*argv++, 2, "matching type",
					dane_matching_type_table);

			argc--;
		} else {
			matching_type = LDNS_TLSA_MATCHING_TYPE_SHA2_256;
		}
		if (argc > 0) {

			print_usage("ldns-dane");
		}
		if ((certificate_usage == LDNS_TLSA_USAGE_CA_CONSTRAINT ||
		     certificate_usage ==
			     LDNS_TLSA_USAGE_SERVICE_CERTIFICATE_CONSTRAINT) &&
		     ! CAfile && ! CApath && ! assume_pkix_validity) {

			fprintf(stderr,
				"When using the \"CA constraint\" or "
			        "\"Service certificate constraint\",\n"
				"-f <CAfile> and/or -p <CApath> options "
				"must be given to perform PKIX validation.\n\n"
				"PKIX validation may be turned off "
				"with the -s option. Note that with\n"
				"\"CA constraint\" the verification process "
				"should then end with a self-signed\n"
				"certificate which must be present "
				"in the server certificate chain.\n\n");

			exit(EXIT_FAILURE);
		}
		tlsas = ldns_rr_list_new();
		break;
	default:
		fprintf(stderr, "Unreachable code\n");
		assert(0);
	}

#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL)
	/* ssl initialize */
	SSL_load_error_strings();
	SSL_library_init();
#endif

	/* ssl load validation store */
	if (! assume_pkix_validity || CAfile || CApath) {
		store = X509_STORE_new();
		if (! store) {
			ssl_err("could not X509_STORE_new");
		}
		if ((CAfile || CApath) && X509_STORE_load_locations(
					store, CAfile, CApath) != 1) {
			ssl_err("error loading CA certificates");
		}
	}

#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL)
	ctx =  SSL_CTX_new(SSLv23_client_method());
#else
	ctx =  SSL_CTX_new(TLS_client_method());
	if (ctx && SSL_CTX_dane_enable(ctx) <= 0) {
		ssl_err("could not SSL_CTX_dane_enable");
	}

	/* Use TLSv1.0 or above for connection. */
	long flags = 0;
# ifdef SSL_OP_NO_SSLv2
	flags |= SSL_OP_NO_SSLv2;
# endif
# ifdef SSL_OP_NO_SSLv3
	flags |= SSL_OP_NO_SSLv3;
# endif
# ifdef SSL_OP_NO_COMPRESSION
	flags |= SSL_OP_NO_COMPRESSION;
# endif
	SSL_CTX_set_options(ctx, flags);

	if (CAfile || CApath) {
		if (!SSL_CTX_load_verify_locations(ctx, CAfile, CApath))
			ssl_err("could not set verify locations\n");

	} else if (!SSL_CTX_set_default_verify_paths(ctx))
		ssl_err("could not set default verify paths\n");
#endif
	if (! ctx) {
		ssl_err("could not SSL_CTX_new");
	}
	if (cert_file &&
	    SSL_CTX_use_certificate_chain_file(ctx, cert_file) != 1) {
		ssl_err("error loading certificate");
	}

	if (cert_file) { /* ssl load certificate */

		ssl = SSL_new(ctx);
		if (! ssl) {
			ssl_err("could not SSL_new");
		}
		cert = SSL_get_certificate(ssl);
		if (! cert) {
			ssl_err("could not SSL_get_certificate");
		}
#ifndef SSL_CTX_get_extra_chain_certs
#ifndef S_SPLINT_S
		extra_certs = ctx->extra_certs;
#endif /* splint */
#else
		if(!SSL_CTX_get_extra_chain_certs(ctx, &extra_certs)) {
			ssl_err("could not SSL_CTX_get_extra_chain_certs");
		}
#endif
		switch (mode) {
		case CREATE: dane_create(tlsas, tlsa_owner, certificate_usage,
					     offset, selector, matching_type,
					     cert, extra_certs, store,
					     verify_server_name, name);
			     break;
#ifdef USE_DANE_VERIFY
#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL)
		case VERIFY: if (! dane_verify(tlsas, NULL,
			                       cert, extra_certs, store,
					       verify_server_name, name,
					       assume_pkix_validity)) {
				     success = false;
			     }
			     break;
#else /* OPENSSL_VERSION_NUMBER < 0x10100000 */
		case VERIFY: 
			     usable_tlsas = 0;
			     SSL_set_connect_state(ssl);
			     if (SSL_dane_enable(ssl, name_str) <= 0) {
			     	ssl_err("could not SSL_dane_enable");
			     }
			     if (!verify_server_name) {
			     	SSL_dane_set_flags(ssl, DANE_FLAG_NO_DANE_EE_NAMECHECKS);
			     }
			     for (j = 0; j < ldns_rr_list_rr_count(tlsas); j++) {
			     	int ret;
			     	ldns_rr *tlsa_rr = ldns_rr_list_rr(tlsas, j);

			     	if (ldns_rr_get_type(tlsa_rr) != LDNS_RR_TYPE_TLSA) {
			     		fprintf(stderr, "Skipping non TLSA RR: ");
			     		ldns_rr_print(stderr, tlsa_rr);
			     		fprintf(stderr, "\n");
			     		continue;
			     	}
			     	if (ldns_rr_rd_count(tlsa_rr) != 4) {
			     		fprintf(stderr, "Skipping TLSA with wrong rdata RR: ");
			     		ldns_rr_print(stderr, tlsa_rr);
			     		fprintf(stderr, "\n");
			     		continue;
			     	}
			     	ret = SSL_dane_tlsa_add(ssl,
			     			ldns_rdf2native_int8(ldns_rr_rdf(tlsa_rr, 0)),
			     			ldns_rdf2native_int8(ldns_rr_rdf(tlsa_rr, 1)),
			     			ldns_rdf2native_int8(ldns_rr_rdf(tlsa_rr, 2)),
			     			ldns_rdf_data(ldns_rr_rdf(tlsa_rr, 3)),
			     			ldns_rdf_size(ldns_rr_rdf(tlsa_rr, 3)));
			     	if (ret < 0) {
			     		ssl_err("could not SSL_dane_tlsa_add");
			     	}
			     	if (ret == 0) {
			     		fprintf(stderr, "Skipping unusable TLSA RR: ");
			     		ldns_rr_print(stderr, tlsa_rr);
			     		fprintf(stderr, "\n");
			     		continue;
			     	}
			     	usable_tlsas += 1;
			     }
			     if (!usable_tlsas) {
			     	fprintf(stderr, "No usable TLSA records were found.\n"
			     			"PKIX validation without DANE will be performed.\n");
				exit_success = no_tlsas_exit_status;
			     }
			     if (!(store_ctx = X509_STORE_CTX_new())) {
				     ssl_err("could not SSL_new");
			     }
			     if (!X509_STORE_CTX_init(store_ctx, store, cert, extra_certs)) {
				     ssl_err("could not X509_STORE_CTX_init");
			     }
			     X509_STORE_CTX_set_default(store_ctx,
					     SSL_is_server(ssl) ? "ssl_client" : "ssl_server");
			     X509_VERIFY_PARAM_set1(X509_STORE_CTX_get0_param(store_ctx),
					     SSL_get0_param(ssl));
			     X509_STORE_CTX_set0_dane(store_ctx, SSL_get0_dane(ssl));
			     X509_NAME_print_ex_fp(stdout,
					     X509_get_subject_name(cert), 0, 0);
			     if (X509_verify_cert(store_ctx)) {
				fprintf(stdout, " %s-validated successfully\n",
						  usable_tlsas 
						? "dane" : "PKIX");
			     } else {
				fprintf(stdout, " did not dane-validate, because: %s\n",
						X509_verify_cert_error_string(
							X509_STORE_CTX_get_error(store_ctx)));
				success = false;
			     }
			     if (store_ctx) {
				     X509_STORE_CTX_free(store_ctx);
			     }
			     break;
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000 */
#endif /* ifdef USE_DANE_VERIFY */
		default:     break; /* suppress warning */
		}
		SSL_free(ssl);

	} else {/* No certificate file given, creation/validation via TLS. */

		/* We need addresses to connect to */
		if (ldns_rr_list_rr_count(addresses) == 0) {
			s = dane_setup_resolver(&res, nameserver_rdf,
			                        keys, assume_dnssec_validity);
			LDNS_ERR(s, "could not dane_setup_resolver");
			ldns_rr_list_free(addresses);
			addresses =dane_lookup_addresses(res, name, ai_family);
			ldns_resolver_free(res);
		}
		if (ldns_rr_list_rr_count(addresses) == 0) {
			fprintf(stderr, "No addresses for %s\n", name_str);
			exit(EXIT_FAILURE);
		}

		/* for all addresses, setup SSL and retrieve certificates */
		for (i = 0; i < ldns_rr_list_rr_count(addresses); i++) {

			ssl = SSL_new(ctx);
			if (! ssl) {
				ssl_err("could not SSL_new");
			}
			address = ldns_rr_a_address(
					ldns_rr_list_rr(addresses, i));
			assert(address != NULL);
#if OPENSSL_VERSION_NUMBER >= 0x10100000  && ! defined(HAVE_LIBRESSL)
			if (mode == VERIFY) {
				usable_tlsas = 0;
				if (SSL_dane_enable(ssl, name_str) <= 0) {
					ssl_err("could not SSL_dane_enable");
				}
				if (!verify_server_name) {
					SSL_dane_set_flags(ssl, DANE_FLAG_NO_DANE_EE_NAMECHECKS);
				}
				for (j = 0; j < ldns_rr_list_rr_count(tlsas); j++) {
					int ret;
					ldns_rr *tlsa_rr = ldns_rr_list_rr(tlsas, j);

					if (ldns_rr_get_type(tlsa_rr) != LDNS_RR_TYPE_TLSA) {
						fprintf(stderr, "Skipping non TLSA RR: ");
						ldns_rr_print(stderr, tlsa_rr);
						fprintf(stderr, "\n");
						continue;
					}
					if (ldns_rr_rd_count(tlsa_rr) != 4) {
						fprintf(stderr, "Skipping TLSA with wrong rdata RR: ");
						ldns_rr_print(stderr, tlsa_rr);
						fprintf(stderr, "\n");
						continue;
					}
					ret = SSL_dane_tlsa_add(ssl,
							ldns_rdf2native_int8(ldns_rr_rdf(tlsa_rr, 0)) | (assume_pkix_validity ? 2 : 0),
							ldns_rdf2native_int8(ldns_rr_rdf(tlsa_rr, 1)),
							ldns_rdf2native_int8(ldns_rr_rdf(tlsa_rr, 2)),
							ldns_rdf_data(ldns_rr_rdf(tlsa_rr, 3)),
							ldns_rdf_size(ldns_rr_rdf(tlsa_rr, 3)));
					if (ret < 0) {
						ssl_err("could not SSL_dane_tlsa_add");
					}
					if (ret == 0) {
						fprintf(stderr, "Skipping unusable TLSA RR: ");
						ldns_rr_print(stderr, tlsa_rr);
						fprintf(stderr, "\n");
						continue;
					}
					usable_tlsas += 1;
				}
				if (!usable_tlsas) {
					fprintf(stderr, "No usable TLSA records were found.\n"
							"PKIX validation without DANE will be performed.\n");

					exit_success = no_tlsas_exit_status;
					if (assume_pkix_validity)
						SSL_set_verify(ssl, SSL_VERIFY_PEER, _ldns_tls_verify_always_ok);
				}
			}
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000 */
			s = ssl_connect_and_get_cert_chain(&cert, &extra_certs,
					ssl, name_str, address,port, transport);
			if (s == LDNS_STATUS_NETWORK_ERR) {
				fprintf(stderr, "Could not connect to ");
				ldns_rdf_print(stderr, address);
				fprintf(stderr, " %d\n", (int) port);

				/* All addresses should succeed */
				success = false;
				continue;
			}
			LDNS_ERR(s, "could not get cert chain from ssl");
#if OPENSSL_VERSION_NUMBER >= 0x10100000 && ! defined(HAVE_LIBRESSL)

			if (mode == VERIFY) {
				char *address_str = ldns_rdf2str(address);
				long verify_result = SSL_get_verify_result(ssl);

				fprintf(stdout, "%s", address_str ? address_str : "<address>");
				free(address_str);

				if (verify_result == X509_V_OK) {
					fprintf(stdout, " %s-validated successfully\n",
							  usable_tlsas 
							? "dane" : "PKIX");
				} else {
					fprintf(stdout, " did not dane-validate, because: %s\n",
							X509_verify_cert_error_string(verify_result));
					success = false;
				}
			}
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000 */
			switch (mode) {
			case CREATE: dane_create(tlsas, tlsa_owner,
						     certificate_usage, offset,
						     selector, matching_type,
						     cert, extra_certs, store,
						     verify_server_name, name);
				     break;

#ifdef USE_DANE_VERIFY
			case VERIFY:
#if OPENSSL_VERSION_NUMBER < 0x10100000 || defined(HAVE_LIBRESSL)
				     if (! dane_verify(tlsas, address,
						cert, extra_certs, store,
						verify_server_name, name,
						assume_pkix_validity)) {
					success = false;

				     }
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000 */
				     if (success && interact) {
					     ssl_interact(ssl);
				     }
				     break;
#endif /* USE_DANE_VERIFY */

			default:     break; /* suppress warning */
			}
			(void)SSL_shutdown(ssl);
			SSL_free(ssl);
		} /* end for all addresses */
	} /* end No certification file */

	if (mode == CREATE) {
		if (print_tlsa_as_type52) {
			print_rr_list_as_TYPEXXX(stdout, tlsas);
		} else {
			ldns_rr_list_print(stdout, tlsas);
		}
	}
	ldns_rr_list_deep_free(tlsas);

	/* cleanup */
	SSL_CTX_free(ctx);

	if (nameserver_rdf) {
		ldns_rdf_deep_free(nameserver_rdf);
	}
	if (store) {
		X509_STORE_free(store);
	}
	if (tlsa_owner_str) {
		LDNS_FREE(tlsa_owner_str);
	}
	if (tlsa_owner) {
		ldns_rdf_free(tlsa_owner);
	}
	if (addresses) {
		ldns_rr_list_deep_free(addresses);
	}
	if (success) {
		exit(exit_success);
	} else {
		exit(EXIT_FAILURE);
	}
}
#else  /* HAVE_SSL */

int
main(int argc, char **argv)
{
	fprintf(stderr, "ldns-dane needs OpenSSL support, "
			"which has not been compiled in\n");
	return 1;
}
#endif /* HAVE_SSL */

#else  /* USE_DANE */

int
main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	fprintf(stderr, "dane support was disabled with this build of ldns, "
			"and has not been compiled in\n");
	return 1;
}

#endif /* USE_DANE */
