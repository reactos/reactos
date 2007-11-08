/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - ISO layer
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

#include <precomp.h>

/* Send a self-contained ISO PDU */
static void
iso_send_msg(uint8 code)
{
	STREAM s;

	s = tcp_init(11);

	out_uint8(s, 3);	/* version */
	out_uint8(s, 0);	/* reserved */
	out_uint16_be(s, 11);	/* length */

	out_uint8(s, 6);	/* hdrlen */
	out_uint8(s, code);
	out_uint16(s, 0);	/* dst_ref */
	out_uint16(s, 0);	/* src_ref */
	out_uint8(s, 0);	/* class */

	s_mark_end(s);
	tcp_send(s);
}

static void
iso_send_connection_request(char *username)
{
	STREAM s;
	int length = 30 + strlen(username);

	s = tcp_init(length);

	out_uint8(s, 3);	/* version */
	out_uint8(s, 0);	/* reserved */
	out_uint16_be(s, length);	/* length */

	out_uint8(s, length - 5);	/* hdrlen */
	out_uint8(s, ISO_PDU_CR);
	out_uint16(s, 0);	/* dst_ref */
	out_uint16(s, 0);	/* src_ref */
	out_uint8(s, 0);	/* class */

	out_uint8p(s, "Cookie: mstshash=", strlen("Cookie: mstshash="));
	out_uint8p(s, username, strlen(username));

	out_uint8(s, 0x0d);	/* Unknown */
	out_uint8(s, 0x0a);	/* Unknown */

	s_mark_end(s);
	tcp_send(s);
}

/* Receive a message on the ISO layer, return code */
static STREAM
iso_recv_msg(uint8 * code, uint8 * rdpver)
{
	STREAM s;
	uint16 length;
	uint8 version;

	s = tcp_recv(NULL, 4);
	if (s == NULL)
		return NULL;
	in_uint8(s, version);
	if (rdpver != NULL)
		*rdpver = version;
	if (version == 3)
	{
		in_uint8s(s, 1);	/* pad */
		in_uint16_be(s, length);
	}
	else
	{
		in_uint8(s, length);
		if (length & 0x80)
		{
			length &= ~0x80;
			next_be(s, length);
		}
	}
	s = tcp_recv(s, length - 4);
	if (s == NULL)
		return NULL;
	if (version != 3)
		return s;
	in_uint8s(s, 1);	/* hdrlen */
	in_uint8(s, *code);
	if (*code == ISO_PDU_DT)
	{
		in_uint8s(s, 1);	/* eot */
		return s;
	}
	in_uint8s(s, 5);	/* dst_ref, src_ref, class */
	return s;
}

/* Initialise ISO transport data packet */
STREAM
iso_init(int length)
{
	STREAM s;

	s = tcp_init(length + 7);
	s_push_layer(s, iso_hdr, 7);

	return s;
}

/* Send an ISO data PDU */
void
iso_send(STREAM s)
{
	uint16 length;

	s_pop_layer(s, iso_hdr);
	length = s->end - s->p;

	out_uint8(s, 3);	/* version */
	out_uint8(s, 0);	/* reserved */
	out_uint16_be(s, length);

	out_uint8(s, 2);	/* hdrlen */
	out_uint8(s, ISO_PDU_DT);	/* code */
	out_uint8(s, 0x80);	/* eot */

	tcp_send(s);
}

/* Receive ISO transport data packet */
STREAM
iso_recv(uint8 * rdpver)
{
	STREAM s;
	uint8 code = 0;

	s = iso_recv_msg(&code, rdpver);
	if (s == NULL)
		return NULL;
	if (rdpver != NULL)
		if (*rdpver != 3)
			return s;
	if (code != ISO_PDU_DT)
	{
		error("expected DT, got 0x%x\n", code);
		return NULL;
	}
	return s;
}

/* Establish a connection up to the ISO layer */
BOOL
iso_connect(char *server, char *username)
{
	uint8 code = 0;

	if (!tcp_connect(server))
		return False;

	iso_send_connection_request(username);

	if (iso_recv_msg(&code, NULL) == NULL)
		return False;

	if (code != ISO_PDU_CC)
	{
		error("expected CC, got 0x%x\n", code);
		tcp_disconnect();
		return False;
	}

	return True;
}

/* Establish a reconnection up to the ISO layer */
BOOL
iso_reconnect(char *server)
{
	uint8 code = 0;

	if (!tcp_connect(server))
		return False;

	iso_send_msg(ISO_PDU_CR);

	if (iso_recv_msg(&code, NULL) == NULL)
		return False;

	if (code != ISO_PDU_CC)
	{
		error("expected CC, got 0x%x\n", code);
		tcp_disconnect();
		return False;
	}

	return True;
}

/* Disconnect from the ISO layer */
void
iso_disconnect(void)
{
	iso_send_msg(ISO_PDU_DR);
	tcp_disconnect();
}

/* reset the state to support reconnecting */
void
iso_reset_state(void)
{
	tcp_reset_state();
}
