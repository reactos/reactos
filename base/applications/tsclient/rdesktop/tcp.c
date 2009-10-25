/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - TCP layer
   Copyright (C) Matthew Chapman 1999-2005

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#if 0 // FIXME: header mess
#include <unistd.h>		/* select read write close */
#include <sys/socket.h>		/* socket connect setsockopt */
#include <sys/time.h>		/* timeval */
#include <netdb.h>		/* gethostbyname */
#include <netinet/in.h>		/* sockaddr_in */
#include <netinet/tcp.h>	/* TCP_NODELAY */
#include <arpa/inet.h>		/* inet_addr */
#include <errno.h>		/* errno */
#endif

#include "rdesktop.h"
#include <winsock2.h>

typedef int socklen_t;

#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif

/* Initialise TCP transport data packet */
STREAM
tcp_init(RDPCLIENT * This, uint32 maxlen)
{
	if (maxlen > This->tcp.out.size)
	{
		void * p;

		p = realloc(This->tcp.out.data, maxlen);

		if (p == NULL)
		{
			This->disconnect_reason = 262;
			return NULL;
		}

		This->tcp.out.data = (uint8 *)p;
		This->tcp.out.size = maxlen;
	}

	This->tcp.out.p = This->tcp.out.data;
	This->tcp.out.end = This->tcp.out.data + This->tcp.out.size;
	return &This->tcp.out;
}

/* Send TCP transport data packet */
BOOL
tcp_send(RDPCLIENT * This, STREAM s)
{
	int length = (int)(s->end - s->data);
	int total = 0;
	DWORD sent;

	OVERLAPPED overlapped;
	memset(&overlapped, 0, sizeof(overlapped));

	while (total < length)
	{
		WriteFile((HANDLE)This->tcp.sock, s->data + total, length - total, NULL, &overlapped);

		switch(WaitForSingleObjectEx((HANDLE)This->tcp.sock, INFINITE, TRUE))
		{
			/* Success */
		case WAIT_OBJECT_0:
			break;

			/* Timeout or error */
		case WAIT_TIMEOUT:
		default:
			This->disconnect_reason = 772;

			/* Aborted, must disconnect ASAP */
		case WAIT_IO_COMPLETION:
			CancelIo((HANDLE)This->tcp.sock);
			break;
		}

		/* Wait for completion. We could hang here, but we shouldn't */
		if(!GetOverlappedResult((HANDLE)This->tcp.sock, &overlapped, &sent, TRUE))
			return False;

		total += sent;
	}

	return True;
}

/* Receive a message on the TCP layer */
STREAM
tcp_recv(RDPCLIENT * This, STREAM s, uint32 length)
{
	unsigned int new_length, end_offset, p_offset;
	DWORD rcvd = 0;

	if (s == NULL)
	{
		/* read into "new" stream */
		if (length > This->tcp.in.size)
		{
			void * p = realloc(This->tcp.in.data, length);

			if(p == NULL)
			{
				This->disconnect_reason = 262;
				return NULL;
			}

			This->tcp.in.data = (uint8 *) p;
			This->tcp.in.size = length;
		}
		This->tcp.in.end = This->tcp.in.p = This->tcp.in.data;
		s = &This->tcp.in;
	}
	else
	{
		/* append to existing stream */
		new_length = (unsigned int)(s->end - s->data) + length;
		if (new_length > s->size)
		{
			void * p = realloc(s->data, new_length);

			if(p == NULL)
			{
				This->disconnect_reason = 262;
				return NULL;
			}

			p_offset = (unsigned int)(s->p - s->data);
			end_offset = (unsigned int)(s->end - s->data);
			s->data = (uint8 *) p;
			s->size = new_length;
			s->p = s->data + p_offset;
			s->end = s->data + end_offset;
		}
	}

	while (length > 0)
	{
		OVERLAPPED overlapped;
		memset(&overlapped, 0, sizeof(overlapped));

		if (!ui_select(This, This->tcp.sock))
			/* User quit */
			return NULL;

		ReadFile((HANDLE)This->tcp.sock, s->end, length, NULL, &overlapped);

		switch(WaitForSingleObjectEx((HANDLE)This->tcp.sock, INFINITE, TRUE))
		{
			/* Success */
		case WAIT_OBJECT_0:
			break;

			/* Timeout or error */
		case WAIT_TIMEOUT:
		default:
			This->disconnect_reason = 1028;

			/* Aborted, must disconnect ASAP */
		case WAIT_IO_COMPLETION:
			CancelIo((HANDLE)This->tcp.sock);
			break;
		}

		/* Wait for completion. We could hang here, but we shouldn't */
		if(!GetOverlappedResult((HANDLE)This->tcp.sock, &overlapped, &rcvd, TRUE))
			return False;

		if (rcvd == 0)
		{
			error("Connection closed\n");
			This->disconnect_reason = 2308;
			return NULL;
		}

		s->end += rcvd;
		length -= rcvd;
	}

	return s;
}

/* Establish a connection on the TCP layer */
BOOL
tcp_connect(RDPCLIENT * This, char *server)
{
	int true_value = 1;

#ifdef IPv6

	int n;
	struct addrinfo hints, *res, *ressave;
	char tcp_port_rdp_s[10];

	snprintf(tcp_port_rdp_s, 10, "%d", This->tcp_port_rdp);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((n = getaddrinfo(server, tcp_port_rdp_s, &hints, &res)))
	{
		error("getaddrinfo: %s\n", gai_strerror(n));
		return False;
	}

	ressave = res;
	This->tcp.sock = -1;
	while (res)
	{
		This->tcp.sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (!(This->tcp.sock < 0))
		{
			if (connect(This->tcp.sock, res->ai_addr, res->ai_addrlen) == 0)
				break;
			close(This->tcp.sock);
			This->tcp.sock = -1;
		}
		res = res->ai_next;
	}
	freeaddrinfo(ressave);

	if (This->tcp.sock == -1)
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
		This->disconnect_reason = 260;
		return False;
	}

	if ((This->tcp.sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		// error("socket: %s\n", strerror(errno)); // EOF
		return False;
	}

	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(This->tcp_port_rdp);

	// TODO: apply connection timeout here

	if (connect(This->tcp.sock, (struct sockaddr *) &servaddr, sizeof(struct sockaddr)) < 0)
	{
		// error("connect: %s\n", strerror(errno)); // EOF
		This->disconnect_reason = 516;
		closesocket(This->tcp.sock);
		return False;
	}

#endif /* IPv6 */

	setsockopt(This->tcp.sock, IPPROTO_TCP, TCP_NODELAY, (void *) &true_value, sizeof(true_value));

	This->tcp.in.size = 4096;
	This->tcp.in.data = (uint8 *) malloc(This->tcp.in.size);

	if(This->tcp.in.data == NULL)
	{
		This->disconnect_reason = 262;
		return False;
	}

	This->tcp.out.size = 4096;
	This->tcp.out.data = (uint8 *) malloc(This->tcp.out.size);

	if(This->tcp.out.data == NULL)
	{
		This->disconnect_reason = 262;
		return False;
	}

	return True;
}

/* Disconnect on the TCP layer */
BOOL
tcp_disconnect(RDPCLIENT * This)
{
	closesocket(This->tcp.sock);
	return True;
}

wchar_t *
tcp_get_address(RDPCLIENT * This)
{
#if 0
	static char ipaddr[32];
	struct sockaddr_in sockaddr;
	socklen_t len = sizeof(sockaddr);
	if (getsockname(This->tcp.sock, (struct sockaddr *) &sockaddr, &len) == 0)
	{
		unsigned char *ip = (unsigned char *) &sockaddr.sin_addr;
		sprintf(ipaddr, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	}
	else
		strcpy(ipaddr, "127.0.0.1");
	return ipaddr;
#endif
	return NULL; // TODO
}

/* reset the state of the tcp layer */
/* Support for Session Directory */
void
tcp_reset_state(RDPCLIENT * This)
{
	This->tcp.sock = -1;		/* reset socket */

	/* Clear the incoming stream */
	if (This->tcp.in.data != NULL)
		free(This->tcp.in.data);
	This->tcp.in.p = NULL;
	This->tcp.in.end = NULL;
	This->tcp.in.data = NULL;
	This->tcp.in.size = 0;
	This->tcp.in.iso_hdr = NULL;
	This->tcp.in.mcs_hdr = NULL;
	This->tcp.in.sec_hdr = NULL;
	This->tcp.in.rdp_hdr = NULL;
	This->tcp.in.channel_hdr = NULL;

	/* Clear the outgoing stream */
	if (This->tcp.out.data != NULL)
		free(This->tcp.out.data);
	This->tcp.out.p = NULL;
	This->tcp.out.end = NULL;
	This->tcp.out.data = NULL;
	This->tcp.out.size = 0;
	This->tcp.out.iso_hdr = NULL;
	This->tcp.out.mcs_hdr = NULL;
	This->tcp.out.sec_hdr = NULL;
	This->tcp.out.rdp_hdr = NULL;
	This->tcp.out.channel_hdr = NULL;
}
