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
#ifdef WITH_SSL
#include <sspi.h>
#include <schannel.h>
#endif

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
typedef struct
{
	CtxtHandle ssl_ctx;
	SecPkgContext_StreamSizes ssl_sizes;
	char *ssl_buf;
	char *extra_buf;
	size_t extra_len;
	char *peek_msg;
	char *peek_msg_mem;
	size_t peek_len;
	DWORD security_flags;
} netconn_t;
static char * g_ssl_server = NULL;
static RD_BOOL g_ssl_initialized = False;
static RD_BOOL cred_handle_initialized = False;
static RD_BOOL have_compat_cred_handle = False;
static SecHandle cred_handle, compat_cred_handle;
static netconn_t g_ssl1;
static netconn_t *g_ssl = NULL;
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

#ifdef WITH_SSL

RD_BOOL send_ssl_chunk(const void *msg, size_t size)
{
	SecBuffer bufs[4] = {
		{g_ssl->ssl_sizes.cbHeader, SECBUFFER_STREAM_HEADER, g_ssl->ssl_buf},
		{size,  SECBUFFER_DATA, g_ssl->ssl_buf+g_ssl->ssl_sizes.cbHeader},
		{g_ssl->ssl_sizes.cbTrailer, SECBUFFER_STREAM_TRAILER, g_ssl->ssl_buf+g_ssl->ssl_sizes.cbHeader+size},
		{0, SECBUFFER_EMPTY, NULL}
	};
	SecBufferDesc buf_desc = {SECBUFFER_VERSION, sizeof(bufs)/sizeof(*bufs), bufs};
	SECURITY_STATUS res;
	int tcp_res;

	memcpy(bufs[1].pvBuffer, msg, size);
	res = EncryptMessage(&g_ssl->ssl_ctx, 0, &buf_desc, 0);
	if (res != SEC_E_OK)
	{
		error("EncryptMessage failed: %d\n", res);
		return False;
	}

	tcp_res = send(g_sock, g_ssl->ssl_buf, bufs[0].cbBuffer+bufs[1].cbBuffer+bufs[2].cbBuffer, 0);
	if (tcp_res < 1)
	{
		error("send failed: %d (%s)\n", tcp_res, TCP_STRERROR);
		return False;
	}

	return True;
}

DWORD read_ssl_chunk(void *buf, SIZE_T buf_size, BOOL blocking, SIZE_T *ret_size, BOOL *eof)
{
	const SIZE_T ssl_buf_size = g_ssl->ssl_sizes.cbHeader+g_ssl->ssl_sizes.cbMaximumMessage+g_ssl->ssl_sizes.cbTrailer;
	SecBuffer bufs[4];
	SecBufferDesc buf_desc = {SECBUFFER_VERSION, sizeof(bufs)/sizeof(*bufs), bufs};
	SSIZE_T size, buf_len = 0;
	int i;
	SECURITY_STATUS res;

	//assert(conn->extra_len < ssl_buf_size);

	if (g_ssl->extra_len)
	{
		memcpy(g_ssl->ssl_buf, g_ssl->extra_buf, g_ssl->extra_len);
		buf_len = g_ssl->extra_len;
		g_ssl->extra_len = 0;
		xfree(g_ssl->extra_buf);
		g_ssl->extra_buf = NULL;
	}

	size = recv(g_sock, g_ssl->ssl_buf+buf_len, ssl_buf_size-buf_len, 0);
	if (size < 0)
	{
		if (!buf_len)
		{
			if (size == -1 && TCP_BLOCKS)
			{
				return WSAEWOULDBLOCK;
			}
			error("recv failed: %d (%s)\n", size, TCP_STRERROR);
			return -1;//ERROR_INTERNET_CONNECTION_ABORTED;
		}
	}
	else
	{
		buf_len += size;
	}

	if (!buf_len)
	{
		*eof = TRUE;
		*ret_size = 0;
		return ERROR_SUCCESS;
	}

	*eof = FALSE;

	do
	{
		memset(bufs, 0, sizeof(bufs));
		bufs[0].BufferType = SECBUFFER_DATA;
		bufs[0].cbBuffer = buf_len;
		bufs[0].pvBuffer = g_ssl->ssl_buf;

		res = DecryptMessage(&g_ssl->ssl_ctx, &buf_desc, 0, NULL);
		switch (res)
		{
		case SEC_E_OK:
			break;
		case SEC_I_CONTEXT_EXPIRED:
			*eof = TRUE;
			return ERROR_SUCCESS;
		case SEC_E_INCOMPLETE_MESSAGE:
			//assert(buf_len < ssl_buf_size);

			size = recv(g_sock, g_ssl->ssl_buf+buf_len, ssl_buf_size-buf_len, 0);
			if (size < 1)
			{
				if (size == -1 && TCP_BLOCKS)
				{
					/* FIXME: Optimize extra_buf usage. */
					g_ssl->extra_buf = xmalloc(buf_len);
					if (!g_ssl->extra_buf)
						return ERROR_NOT_ENOUGH_MEMORY;

					g_ssl->extra_len = buf_len;
					memcpy(g_ssl->extra_buf, g_ssl->ssl_buf, g_ssl->extra_len);
					return WSAEWOULDBLOCK;
				}

				error("recv failed: %d (%s)\n", size, TCP_STRERROR);
				return -1;//ERROR_INTERNET_CONNECTION_ABORTED;
			}

			buf_len += size;
			continue;
		default:
			error("DecryptMessage failed: %d\n", res);
			return -1;//ERROR_INTERNET_CONNECTION_ABORTED;
		}
	}
	while (res != SEC_E_OK);

	for (i=0; i < sizeof(bufs)/sizeof(*bufs); i++)
	{
		if (bufs[i].BufferType == SECBUFFER_DATA)
		{
			size = min(buf_size, bufs[i].cbBuffer);
			memcpy(buf, bufs[i].pvBuffer, size);
			if (size < bufs[i].cbBuffer)
			{
				//assert(!conn->peek_len);
				g_ssl->peek_msg_mem = g_ssl->peek_msg = xmalloc(bufs[i].cbBuffer - size);
				if (!g_ssl->peek_msg)
					return ERROR_NOT_ENOUGH_MEMORY;
				g_ssl->peek_len = bufs[i].cbBuffer-size;
				memcpy(g_ssl->peek_msg, (char*)bufs[i].pvBuffer+size, g_ssl->peek_len);
			}

			*ret_size = size;
		}
	}

	for (i=0; i < sizeof(bufs)/sizeof(*bufs); i++)
	{
		if (bufs[i].BufferType == SECBUFFER_EXTRA)
		{
			g_ssl->extra_buf = xmalloc(bufs[i].cbBuffer);
			if (!g_ssl->extra_buf)
				return ERROR_NOT_ENOUGH_MEMORY;

			g_ssl->extra_len = bufs[i].cbBuffer;
			memcpy(g_ssl->extra_buf, bufs[i].pvBuffer, g_ssl->extra_len);
		}
	}

	return ERROR_SUCCESS;
}
#endif /* WITH_SSL */
/* Send TCP transport data packet */
void
tcp_send(STREAM s)
{
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
			const BYTE *ptr = s->data + total;
			size_t chunk_size;

			sent = 0;

			while (length - total)
			{
				chunk_size = min(length - total, g_ssl->ssl_sizes.cbMaximumMessage);
				if (!send_ssl_chunk(ptr, chunk_size))
				{
#ifdef WITH_SCARD
					scard_unlock(SCARD_LOCK_TCP);
#endif

					//error("send_ssl_chunk: %d (%s)\n", sent, TCP_STRERROR);
					g_network_error = True;
					return;
				}

				sent += chunk_size;
				ptr += chunk_size;
				length -= chunk_size;
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

					error("send: %d (%s)\n", sent, TCP_STRERROR);
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
		if (!g_ssl)
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
			SIZE_T size = 0;
			BOOL eof;
			DWORD res;

			if (g_ssl->peek_msg)
			{
				size = min(length, g_ssl->peek_len);
				memcpy(s->end, g_ssl->peek_msg, size);
				g_ssl->peek_len -= size;
				g_ssl->peek_msg += size;
				s->end += size;

				if (!g_ssl->peek_len)
				{
					xfree(g_ssl->peek_msg_mem);
					g_ssl->peek_msg_mem = g_ssl->peek_msg = NULL;
				}

				return s;
			}

			do
			{
				res = read_ssl_chunk((BYTE*)s->end, length, TRUE, &size, &eof);
				if (res != ERROR_SUCCESS)
				{
					if (res == WSAEWOULDBLOCK)
					{
						if (size)
						{
 							res = ERROR_SUCCESS;
						}
					}
					else
					{
						error("read_ssl_chunk: %d (%s)\n", res, TCP_STRERROR);
						g_network_error = True;
						return NULL;
					}
					break;
				}
			}
			while (!size && !eof);
			rcvd = size;
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
					error("recv: %d (%s)\n", rcvd, TCP_STRERROR);
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

RD_BOOL
ensure_cred_handle(void)
{
	SECURITY_STATUS res = SEC_E_OK;

	if (!cred_handle_initialized)
	{
		SCHANNEL_CRED cred = {SCHANNEL_CRED_VERSION};
		SecPkgCred_SupportedProtocols prots;

		res = AcquireCredentialsHandleW(NULL, (WCHAR*)UNISP_NAME_W, SECPKG_CRED_OUTBOUND, NULL, &cred,
				NULL, NULL, &cred_handle, NULL);
		if (res == SEC_E_OK)
		{
			res = QueryCredentialsAttributesA(&cred_handle, SECPKG_ATTR_SUPPORTED_PROTOCOLS, &prots);
			if (res != SEC_E_OK || (prots.grbitProtocol & SP_PROT_TLS1_1PLUS_CLIENT))
			{
				cred.grbitEnabledProtocols = prots.grbitProtocol & ~SP_PROT_TLS1_1PLUS_CLIENT;
				res = AcquireCredentialsHandleW(NULL, (WCHAR*)UNISP_NAME_W, SECPKG_CRED_OUTBOUND, NULL, &cred,
						NULL, NULL, &compat_cred_handle, NULL);
				have_compat_cred_handle = res == SEC_E_OK;
			}
		}

		cred_handle_initialized = res == SEC_E_OK;
	}

	if (res != SEC_E_OK)
	{
		error("ensure_cred_handle failed: %ld\n", res);
		return False;
	}

	return True;
}

DWORD
ssl_handshake(RD_BOOL compat_mode)
{
	SecBuffer out_buf = {0, SECBUFFER_TOKEN, NULL}, in_bufs[2] = {{0, SECBUFFER_TOKEN}, {0, SECBUFFER_EMPTY}};
	SecBufferDesc out_desc = {SECBUFFER_VERSION, 1, &out_buf}, in_desc = {SECBUFFER_VERSION, 2, in_bufs};
	SecHandle *cred = &cred_handle;
	BYTE *read_buf;
	SIZE_T read_buf_size = 2048;
	ULONG attrs = 0;
	CtxtHandle ctx;
	SSIZE_T size;
	SECURITY_STATUS status;
	DWORD res = ERROR_SUCCESS;

	const DWORD isc_req_flags = ISC_REQ_ALLOCATE_MEMORY|ISC_REQ_USE_SESSION_KEY|ISC_REQ_CONFIDENTIALITY
		|ISC_REQ_SEQUENCE_DETECT|ISC_REQ_REPLAY_DETECT|ISC_REQ_MANUAL_CRED_VALIDATION;

	if (!ensure_cred_handle())
		return -1;

	if (compat_mode) {
		if (!have_compat_cred_handle)
			return -1;
		cred = &compat_cred_handle;
	}

	read_buf = xmalloc(read_buf_size);
	if (!read_buf)
		return ERROR_OUTOFMEMORY;

	if (!g_ssl_server)
		return -1;

	status = InitializeSecurityContextA(cred, NULL, g_ssl_server, isc_req_flags, 0, 0, NULL, 0,
			&ctx, &out_desc, &attrs, NULL);

	//assert(status != SEC_E_OK);

	while (status == SEC_I_CONTINUE_NEEDED || status == SEC_E_INCOMPLETE_MESSAGE)
	{
		if (out_buf.cbBuffer)
		{
			//assert(status == SEC_I_CONTINUE_NEEDED);

			size = send(g_sock, out_buf.pvBuffer, out_buf.cbBuffer, 0);
			if (size != out_buf.cbBuffer)
			{
				error("send failed: %d (%s)\n", size, TCP_STRERROR);
				status = -1;
				break;
			}

			FreeContextBuffer(out_buf.pvBuffer);
			out_buf.pvBuffer = NULL;
			out_buf.cbBuffer = 0;
		}

		if (status == SEC_I_CONTINUE_NEEDED)
		{
			//assert(in_bufs[1].cbBuffer < read_buf_size);

			memmove(read_buf, (BYTE*)in_bufs[0].pvBuffer+in_bufs[0].cbBuffer-in_bufs[1].cbBuffer, in_bufs[1].cbBuffer);
			in_bufs[0].cbBuffer = in_bufs[1].cbBuffer;

			in_bufs[1].BufferType = SECBUFFER_EMPTY;
			in_bufs[1].cbBuffer = 0;
			in_bufs[1].pvBuffer = NULL;
		}

		//assert(in_bufs[0].BufferType == SECBUFFER_TOKEN);
		//assert(in_bufs[1].BufferType == SECBUFFER_EMPTY);

		if (in_bufs[0].cbBuffer + 1024 > read_buf_size)
		{
			BYTE *new_read_buf = xrealloc(read_buf, read_buf_size + 1024);
			if (!new_read_buf)
			{
				status = E_OUTOFMEMORY;
				break;
			}

			in_bufs[0].pvBuffer = read_buf = new_read_buf;
			read_buf_size += 1024;
		}

		size = recv(g_sock, (char *)read_buf + in_bufs[0].cbBuffer, read_buf_size - in_bufs[0].cbBuffer, 0);
		if (size < 1)
		{
			error("recv failed: %d (%s)\n", size, TCP_STRERROR);
			res = -1;
			break;
		}

		in_bufs[0].cbBuffer += size;
		in_bufs[0].pvBuffer = read_buf;
		status = InitializeSecurityContextA(cred, &ctx, g_ssl_server,  isc_req_flags, 0, 0, &in_desc,
				0, NULL, &out_desc, &attrs, NULL);

		if (status == SEC_E_OK) {
			if (SecIsValidHandle(&g_ssl->ssl_ctx))
				DeleteSecurityContext(&g_ssl->ssl_ctx);
			g_ssl->ssl_ctx = ctx;

			if (in_bufs[1].BufferType == SECBUFFER_EXTRA)
			{
				//FIXME("SECBUFFER_EXTRA not supported\n");
			}

			status = QueryContextAttributesW(&ctx, SECPKG_ATTR_STREAM_SIZES, &g_ssl->ssl_sizes);
			if (status != SEC_E_OK)
			{
				//error("Can't determine ssl buffer sizes: %ld\n", status);
				break;
			}

			g_ssl->ssl_buf = xmalloc(g_ssl->ssl_sizes.cbHeader + g_ssl->ssl_sizes.cbMaximumMessage
					+ g_ssl->ssl_sizes.cbTrailer);
			if (!g_ssl->ssl_buf)
			{
				res = GetLastError();
				break;
			}
		}
	}

	xfree(read_buf);

	if (status != SEC_E_OK || res != ERROR_SUCCESS)
	{
		error("Failed to establish SSL connection: %08x (%u)\n", status, res);
		xfree(g_ssl->ssl_buf);
		g_ssl->ssl_buf = NULL;
		return res ? res : -1;
	}

	return ERROR_SUCCESS;
}

/* Establish a SSL/TLS 1.0 connection */
RD_BOOL
tcp_tls_connect(void)
{
	int err;
	char tcp_port_rdp_s[10];

	if (!g_ssl_initialized)
	{
		g_ssl = &g_ssl1;
		SecInvalidateHandle(&g_ssl->ssl_ctx);

		g_ssl_initialized = True;
	}

	snprintf(tcp_port_rdp_s, 10, "%d", g_tcp_port_rdp);
	if ((err = ssl_handshake(FALSE)) != 0)
	{
		goto fail;
	}

	return True;

      fail:
	g_ssl = NULL;
	return False;
}

/* Get public key from server of TLS 1.0 connection */
RD_BOOL
tcp_tls_get_server_pubkey(STREAM s)
{
	const CERT_CONTEXT *cert = NULL;
	SECURITY_STATUS status;

	s->data = s->p = NULL;
	s->size = 0;

	if (g_ssl == NULL)
		goto out;
	status = QueryContextAttributesW(&g_ssl->ssl_ctx, SECPKG_ATTR_REMOTE_CERT_CONTEXT, (void*)&cert);
	if (status != SEC_E_OK)
	{
		error("tcp_tls_get_server_pubkey: QueryContextAttributesW() failed %ld\n", status);
		goto out;
	}

	s->size = cert->cbCertEncoded;
	if (s->size < 1)
	{
		error("tcp_tls_get_server_pubkey: cert->cbCertEncoded = %ld\n", cert->cbCertEncoded);
		goto out;
	}

	s->data = s->p = (unsigned char *)xmalloc(s->size);
	memcpy(cert->pbCertEncoded, &s->p, s->size);
	s->p = s->data;
	s->end = s->p + s->size;

      out:
	if (cert)
		CertFreeCertificateContext(cert);
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

#ifdef WITH_SSL
	g_ssl_server = xmalloc(strlen(server)+1);
#endif /* WITH_SSL */

	return True;
}

/* Disconnect on the TCP layer */
void
tcp_disconnect(void)
{
#ifdef WITH_SSL
	if (g_ssl)
	{
		xfree(g_ssl->peek_msg_mem);
		g_ssl->peek_msg_mem = NULL;
		g_ssl->peek_msg = NULL;
		g_ssl->peek_len = 0;
		xfree(g_ssl->ssl_buf);
		g_ssl->ssl_buf = NULL;
		xfree(g_ssl->extra_buf);
		g_ssl->extra_buf = NULL;
		g_ssl->extra_len = 0;
		if (SecIsValidHandle(&g_ssl->ssl_ctx))
			DeleteSecurityContext(&g_ssl->ssl_ctx);
		if (cred_handle_initialized)
			FreeCredentialsHandle(&cred_handle);
		if (have_compat_cred_handle)
			FreeCredentialsHandle(&compat_cred_handle);
		if (g_ssl_server)
		{
			xfree(g_ssl_server);
			g_ssl_server = NULL;
		}
		g_ssl = NULL;
		g_ssl_initialized = False;
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
