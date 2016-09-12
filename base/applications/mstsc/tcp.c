/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - TCP layer
   Copyright (C) Matthew Chapman <matthewc.unsw.edu.au> 1999-2008
   Copyright 2005-2011 Peter Astrand <astrand@cendio.se> for Cendio AB
   Copyright 2012-2013 Henrik Andersson <hean01@cendio.se> for Cendio AB

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "precomp.h"

#ifdef _WIN32
#define socklen_t int
#define TCP_CLOSE(_sck) closesocket(_sck)
#define TCP_STRERROR "tcp error"
#define TCP_SLEEP(_n) Sleep(_n)
#define TCP_BLOCKS (WSAGetLastError() == WSAEWOULDBLOCK)
#else /* _WIN32 */
#define TCP_CLOSE(_sck) close(_sck)
#define TCP_STRERROR strerror(errno)
#define TCP_SLEEP(_n) sleep(_n)
#define TCP_BLOCKS (errno == EWOULDBLOCK)
#endif /* _WIN32 */

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

#ifdef WITH_SCARD
#define STREAM_COUNT 8
#else
#define STREAM_COUNT 1
#endif

#ifdef WITH_SSL
static RD_BOOL g_ssl_initialized = False;
static SSL *g_ssl = NULL;
static SSL_CTX *g_ssl_ctx = NULL;
#endif /* WITH_SSL */
static int g_sock;
static struct stream g_in;
static struct stream g_out[STREAM_COUNT];
int g_tcp_port_rdp = TCP_PORT_RDP;
extern RD_BOOL g_user_quit;
extern RD_BOOL g_network_error;
extern RD_BOOL g_reconnect_loop;

/* Initialise TCP transport data packet */
STREAM
tcp_init(uint32 maxlen)
{
	static int cur_stream_id = 0;
	STREAM result = NULL;

#ifdef WITH_SCARD
	scard_lock(SCARD_LOCK_TCP);
#endif
	result = &g_out[cur_stream_id];
	cur_stream_id = (cur_stream_id + 1) % STREAM_COUNT;

	if (maxlen > result->size)
	{
		result->data = (uint8 *) xrealloc(result->data, maxlen);
		result->size = maxlen;
	}

	result->p = result->data;
	result->end = result->data + result->size;
#ifdef WITH_SCARD
	scard_unlock(SCARD_LOCK_TCP);
#endif
	return result;
}

/* Send TCP transport data packet */
void
tcp_send(STREAM s)
{
#ifdef WITH_SSL
	int ssl_err;
#endif /* WITH_SSL */
	int length = s->end - s->data;
	int sent, total = 0;

	if (g_network_error == True)
		return;

#ifdef WITH_SCARD
	scard_lock(SCARD_LOCK_TCP);
#endif
	while (total < length)
	{
#ifdef WITH_SSL
		if (g_ssl)
		{
			sent = SSL_write(g_ssl, s->data + total, length - total);
			if (sent <= 0)
			{
				ssl_err = SSL_get_error(g_ssl, sent);
				if (sent < 0 && (ssl_err == SSL_ERROR_WANT_READ ||
						 ssl_err == SSL_ERROR_WANT_WRITE))
				{
					TCP_SLEEP(0);
					sent = 0;
				}
				else
				{
#ifdef WITH_SCARD
					scard_unlock(SCARD_LOCK_TCP);
#endif

					error("SSL_write: %d (%s)\n", ssl_err, TCP_STRERROR);
					g_network_error = True;
					return;
				}
			}
		}
		else
		{
#endif /* WITH_SSL */
			sent = send(g_sock, (const char *)s->data + total, length - total, 0);
			if (sent <= 0)
			{
				if (sent == -1 && TCP_BLOCKS)
				{
					TCP_SLEEP(0);
					sent = 0;
				}
				else
				{
#ifdef WITH_SCARD
					scard_unlock(SCARD_LOCK_TCP);
#endif

					error("send: %s\n", TCP_STRERROR);
					g_network_error = True;
					return;
				}
			}
#ifdef WITH_SSL
		}
#endif /* WITH_SSL */
		total += sent;
	}
#ifdef WITH_SCARD
	scard_unlock(SCARD_LOCK_TCP);
#endif
}

/* Receive a message on the TCP layer */
STREAM
tcp_recv(STREAM s, uint32 length)
{
	uint32 new_length, end_offset, p_offset;
	int rcvd = 0;
#ifdef WITH_SSL
	int ssl_err;
#endif /* WITH_SSL */

	if (g_network_error == True)
		return NULL;

	if (s == NULL)
	{
		/* read into "new" stream */
		if (length > g_in.size)
		{
			g_in.data = (uint8 *) xrealloc(g_in.data, length);
			g_in.size = length;
		}
		g_in.end = g_in.p = g_in.data;
		s = &g_in;
	}
	else
	{
		/* append to existing stream */
		new_length = (s->end - s->data) + length;
		if (new_length > s->size)
		{
			p_offset = s->p - s->data;
			end_offset = s->end - s->data;
			s->data = (uint8 *) xrealloc(s->data, new_length);
			s->size = new_length;
			s->p = s->data + p_offset;
			s->end = s->data + end_offset;
		}
	}

	while (length > 0)
	{
#ifdef WITH_SSL
		if (!g_ssl || SSL_pending(g_ssl) <= 0)
#endif /* WITH_SSL */
		{
			if (!ui_select(g_sock))
			{
				/* User quit */
				g_user_quit = True;
				return NULL;
			}
		}

#ifdef WITH_SSL
		if (g_ssl)
		{
			rcvd = SSL_read(g_ssl, s->end, length);
			ssl_err = SSL_get_error(g_ssl, rcvd);

			if (ssl_err == SSL_ERROR_SSL)
			{
				if (SSL_get_shutdown(g_ssl) & SSL_RECEIVED_SHUTDOWN)
				{
					error("Remote peer initiated ssl shutdown.\n");
					return NULL;
				}

				ERR_print_errors_fp(stdout);
				g_network_error = True;
				return NULL;
			}

			if (ssl_err == SSL_ERROR_WANT_READ || ssl_err == SSL_ERROR_WANT_WRITE)
			{
				rcvd = 0;
			}
			else if (ssl_err != SSL_ERROR_NONE)
			{
				error("SSL_read: %d (%s)\n", ssl_err, TCP_STRERROR);
				g_network_error = True;
				return NULL;
			}

		}
		else
		{
#endif /* WITH_SSL */
			rcvd = recv(g_sock, (char *)s->end, length, 0);
			if (rcvd < 0)
			{
				if (rcvd == -1 && TCP_BLOCKS)
				{
					rcvd = 0;
				}
				else
				{
					error("recv: %s\n", TCP_STRERROR);
					g_network_error = True;
					return NULL;
				}
			}
			else if (rcvd == 0)
			{
				error("Connection closed\n");
				return NULL;
			}
#ifdef WITH_SSL
		}
#endif /* WITH_SSL */

		s->end += rcvd;
		length -= rcvd;
	}

	return s;
}

#ifdef WITH_SSL
/* Establish a SSL/TLS 1.0 connection */
RD_BOOL
tcp_tls_connect(void)
{
	int err;
	long options;

	if (!g_ssl_initialized)
	{
		SSL_load_error_strings();
		SSL_library_init();
		g_ssl_initialized = True;
	}

	/* create process context */
	if (g_ssl_ctx == NULL)
	{
		g_ssl_ctx = SSL_CTX_new(TLSv1_client_method());
		if (g_ssl_ctx == NULL)
		{
			error("tcp_tls_connect: SSL_CTX_new() failed to create TLS v1.0 context\n");
			goto fail;
		}

		options = 0;
#ifdef SSL_OP_NO_COMPRESSION
		options |= SSL_OP_NO_COMPRESSION;
#endif // __SSL_OP_NO_COMPRESSION
		options |= SSL_OP_DONT_INSERT_EMPTY_FRAGMENTS;
		SSL_CTX_set_options(g_ssl_ctx, options);
	}

	/* free old connection */
	if (g_ssl)
		SSL_free(g_ssl);

	/* create new ssl connection */
	g_ssl = SSL_new(g_ssl_ctx);
	if (g_ssl == NULL)
	{
		error("tcp_tls_connect: SSL_new() failed\n");
		goto fail;
	}

	if (SSL_set_fd(g_ssl, g_sock) < 1)
	{
		error("tcp_tls_connect: SSL_set_fd() failed\n");
		goto fail;
	}

	do
	{
		err = SSL_connect(g_ssl);
	}
	while (SSL_get_error(g_ssl, err) == SSL_ERROR_WANT_READ);

	if (err < 0)
	{
		ERR_print_errors_fp(stdout);
		goto fail;
	}

	return True;

      fail:
	if (g_ssl)
		SSL_free(g_ssl);
	if (g_ssl_ctx)
		SSL_CTX_free(g_ssl_ctx);

	g_ssl = NULL;
	g_ssl_ctx = NULL;
	return False;
}

/* Get public key from server of TLS 1.0 connection */
RD_BOOL
tcp_tls_get_server_pubkey(STREAM s)
{
	X509 *cert = NULL;
	EVP_PKEY *pkey = NULL;

	s->data = s->p = NULL;
	s->size = 0;

	if (g_ssl == NULL)
		goto out;

	cert = SSL_get_peer_certificate(g_ssl);
	if (cert == NULL)
	{
		error("tcp_tls_get_server_pubkey: SSL_get_peer_certificate() failed\n");
		goto out;
	}

	pkey = X509_get_pubkey(cert);
	if (pkey == NULL)
	{
		error("tcp_tls_get_server_pubkey: X509_get_pubkey() failed\n");
		goto out;
	}

	s->size = i2d_PublicKey(pkey, NULL);
	if (s->size < 1)
	{
		error("tcp_tls_get_server_pubkey: i2d_PublicKey() failed\n");
		goto out;
	}

	s->data = s->p = xmalloc(s->size);
	i2d_PublicKey(pkey, &s->p);
	s->p = s->data;
	s->end = s->p + s->size;

      out:
	if (cert)
		X509_free(cert);
	if (pkey)
		EVP_PKEY_free(pkey);
	return (s->size != 0);
}
#endif /* WITH_SSL */

/* Establish a connection on the TCP layer */
RD_BOOL
tcp_connect(char *server)
{
	socklen_t option_len;
	uint32 option_value;
	int i;

#ifdef IPv6

	int n;
	struct addrinfo hints, *res, *ressave;
	char tcp_port_rdp_s[10];

	snprintf(tcp_port_rdp_s, 10, "%d", g_tcp_port_rdp);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((n = getaddrinfo(server, tcp_port_rdp_s, &hints, &res)))
	{
		error("getaddrinfo: %s\n", gai_strerror(n));
		return False;
	}

	ressave = res;
	g_sock = -1;
	while (res)
	{
		g_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (!(g_sock < 0))
		{
			if (connect(g_sock, res->ai_addr, res->ai_addrlen) == 0)
				break;
			TCP_CLOSE(g_sock);
			g_sock = -1;
		}
		res = res->ai_next;
	}
	freeaddrinfo(ressave);

	if (g_sock == -1)
	{
		error("%s: unable to connect\n", server);
		return False;
	}

#else /* no IPv6 support */

	struct hostent *nslookup;
	struct sockaddr_in servaddr;

	if ((nslookup = gethostbyname(server)) != NULL)
	{
		memcpy(&servaddr.sin_addr, nslookup->h_addr, sizeof(servaddr.sin_addr));
	}
	else if ((servaddr.sin_addr.s_addr = inet_addr(server)) == INADDR_NONE)
	{
		error("%s: unable to resolve host\n", server);
		return False;
	}

	if ((g_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		error("socket: %s\n", TCP_STRERROR);
		return False;
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons((uint16) g_tcp_port_rdp);

	if (connect(g_sock, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) < 0)
	{
		if (!g_reconnect_loop)
			error("connect: %s\n", TCP_STRERROR);

		TCP_CLOSE(g_sock);
		g_sock = -1;
		return False;
	}

#endif /* IPv6 */

	option_value = 1;
	option_len = sizeof(option_value);
	setsockopt(g_sock, IPPROTO_TCP, TCP_NODELAY, (void *) &option_value, option_len);
	/* receive buffer must be a least 16 K */
	if (getsockopt(g_sock, SOL_SOCKET, SO_RCVBUF, (void *) &option_value, &option_len) == 0)
	{
		if (option_value < (1024 * 16))
		{
			option_value = 1024 * 16;
			option_len = sizeof(option_value);
			setsockopt(g_sock, SOL_SOCKET, SO_RCVBUF, (void *) &option_value,
				   option_len);
		}
	}

	g_in.size = 4096;
	g_in.data = (uint8 *) xmalloc(g_in.size);

	for (i = 0; i < STREAM_COUNT; i++)
	{
		g_out[i].size = 4096;
		g_out[i].data = (uint8 *) xmalloc(g_out[i].size);
	}

	return True;
}

/* Disconnect on the TCP layer */
void
tcp_disconnect(void)
{
#ifdef WITH_SSL
	if (g_ssl)
	{
		if (!g_network_error)
			(void) SSL_shutdown(g_ssl);
		SSL_free(g_ssl);
		g_ssl = NULL;
		SSL_CTX_free(g_ssl_ctx);
		g_ssl_ctx = NULL;
	}
#endif /* WITH_SSL */
	TCP_CLOSE(g_sock);
	g_sock = -1;
}

char *
tcp_get_address()
{
	static char ipaddr[32];
	struct sockaddr_in sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (getsockname(g_sock, (struct sockaddr *) &sockaddr, &len) == 0)
	{
		uint8 *ip = (uint8 *) & sockaddr.sin_addr;
		sprintf(ipaddr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	}
	else
		strcpy(ipaddr, "127.0.0.1");
	return ipaddr;
}

RD_BOOL
tcp_is_connected()
{
	struct sockaddr_in sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (getpeername(g_sock, (struct sockaddr *) &sockaddr, &len))
		return True;
	return False;
}

/* reset the state of the tcp layer */
/* Support for Session Directory */
void
tcp_reset_state(void)
{
	int i;

	/* Clear the incoming stream */
	if (g_in.data != NULL)
		xfree(g_in.data);
	g_in.p = NULL;
	g_in.end = NULL;
	g_in.data = NULL;
	g_in.size = 0;
	g_in.iso_hdr = NULL;
	g_in.mcs_hdr = NULL;
	g_in.sec_hdr = NULL;
	g_in.rdp_hdr = NULL;
	g_in.channel_hdr = NULL;

	/* Clear the outgoing stream(s) */
	for (i = 0; i < STREAM_COUNT; i++)
	{
		if (g_out[i].data != NULL)
			xfree(g_out[i].data);
		g_out[i].p = NULL;
		g_out[i].end = NULL;
		g_out[i].data = NULL;
		g_out[i].size = 0;
		g_out[i].iso_hdr = NULL;
		g_out[i].mcs_hdr = NULL;
		g_out[i].sec_hdr = NULL;
		g_out[i].rdp_hdr = NULL;
		g_out[i].channel_hdr = NULL;
	}
}
