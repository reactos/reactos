/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - Multipoint Communications Service
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

#include "rdesktop.h"

/* Parse an ASN.1 BER header */
static BOOL
ber_parse_header(STREAM s, int tagval, int *length)
{
	int tag, len;

	if (tagval > 0xff)
	{
		in_uint16_be(s, tag);
	}
	else
	{
	in_uint8(s, tag)}

	if (tag != tagval)
	{
		error("expected tag %d, got %d\n", tagval, tag);
		return False;
	}

	in_uint8(s, len);

	if (len & 0x80)
	{
		len &= ~0x80;
		*length = 0;
		while (len--)
			next_be(s, *length);
	}
	else
		*length = len;

	return s_check(s);
}

/* Output an ASN.1 BER header */
static void
ber_out_header(STREAM s, int tagval, int length)
{
	if (tagval > 0xff)
	{
		out_uint16_be(s, tagval);
	}
	else
	{
		out_uint8(s, tagval);
	}

	if (length >= 0x80)
	{
		out_uint8(s, 0x82);
		out_uint16_be(s, length);
	}
	else
		out_uint8(s, length);
}

/* Output an ASN.1 BER integer */
static void
ber_out_integer(STREAM s, int value)
{
	ber_out_header(s, BER_TAG_INTEGER, 2);
	out_uint16_be(s, value);
}

/* Output a DOMAIN_PARAMS structure (ASN.1 BER) */
static void
mcs_out_domain_params(STREAM s, int max_channels, int max_users, int max_tokens, int max_pdusize)
{
	ber_out_header(s, MCS_TAG_DOMAIN_PARAMS, 32);
	ber_out_integer(s, max_channels);
	ber_out_integer(s, max_users);
	ber_out_integer(s, max_tokens);
	ber_out_integer(s, 1);	/* num_priorities */
	ber_out_integer(s, 0);	/* min_throughput */
	ber_out_integer(s, 1);	/* max_height */
	ber_out_integer(s, max_pdusize);
	ber_out_integer(s, 2);	/* ver_protocol */
}

/* Parse a DOMAIN_PARAMS structure (ASN.1 BER) */
static BOOL
mcs_parse_domain_params(STREAM s)
{
	int length;

	ber_parse_header(s, MCS_TAG_DOMAIN_PARAMS, &length);
	in_uint8s(s, length);

	return s_check(s);
}

/* Send an MCS_CONNECT_INITIAL message (ASN.1 BER) */
static BOOL
mcs_send_connect_initial(RDPCLIENT * This, STREAM mcs_data)
{
	int datalen = (uint16)(mcs_data->end - mcs_data->data);
	int length = 9 + 3 * 34 + 4 + datalen;
	STREAM s;

	s = iso_init(This, length + 5);

	if(s == NULL)
		return False;

	ber_out_header(s, MCS_CONNECT_INITIAL, length);
	ber_out_header(s, BER_TAG_OCTET_STRING, 1);	/* calling domain */
	out_uint8(s, 1);
	ber_out_header(s, BER_TAG_OCTET_STRING, 1);	/* called domain */
	out_uint8(s, 1);

	ber_out_header(s, BER_TAG_BOOLEAN, 1);
	out_uint8(s, 0xff);	/* upward flag */

	mcs_out_domain_params(s, 34, 2, 0, 0xffff);	/* target params */
	mcs_out_domain_params(s, 1, 1, 1, 0x420);	/* min params */
	mcs_out_domain_params(s, 0xffff, 0xfc17, 0xffff, 0xffff);	/* max params */

	ber_out_header(s, BER_TAG_OCTET_STRING, datalen);
	out_uint8p(s, mcs_data->data, datalen);

	s_mark_end(s);
	return iso_send(This, s);
}

/* Expect a MCS_CONNECT_RESPONSE message (ASN.1 BER) */
static BOOL
mcs_recv_connect_response(RDPCLIENT * This, STREAM mcs_data)
{
	uint8 result;
	int length;
	STREAM s;

	s = iso_recv(This, NULL);
	if (s == NULL)
		return False;

	ber_parse_header(s, MCS_CONNECT_RESPONSE, &length);

	ber_parse_header(s, BER_TAG_RESULT, &length);
	in_uint8(s, result);
	if (result != 0)
	{
		error("MCS connect: %d\n", result);
		return False;
	}

	ber_parse_header(s, BER_TAG_INTEGER, &length);
	in_uint8s(s, length);	/* connect id */
	mcs_parse_domain_params(s);

	ber_parse_header(s, BER_TAG_OCTET_STRING, &length);

	sec_process_mcs_data(This, s);
	/*
	   if (length > mcs_data->size)
	   {
	   error("MCS data length %d, expected %d\n", length,
	   mcs_data->size);
	   length = mcs_data->size;
	   }

	   in_uint8a(s, mcs_data->data, length);
	   mcs_data->p = mcs_data->data;
	   mcs_data->end = mcs_data->data + length;
	 */
	return s_check_end(s);
}

/* Send an EDrq message (ASN.1 PER) */
static BOOL
mcs_send_edrq(RDPCLIENT * This)
{
	STREAM s;

	s = iso_init(This, 5);

	if(s == NULL)
		return False;

	out_uint8(s, (MCS_EDRQ << 2));
	out_uint16_be(s, 1);	/* height */
	out_uint16_be(s, 1);	/* interval */

	s_mark_end(s);
	return iso_send(This, s);
}

/* Send an AUrq message (ASN.1 PER) */
static BOOL
mcs_send_aurq(RDPCLIENT * This)
{
	STREAM s;

	s = iso_init(This, 1);

	if(s == NULL)
		return False;

	out_uint8(s, (MCS_AURQ << 2));

	s_mark_end(s);
	return iso_send(This, s);
}

/* Expect a AUcf message (ASN.1 PER) */
static BOOL
mcs_recv_aucf(RDPCLIENT * This, uint16 * mcs_userid)
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv(This, NULL);
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != MCS_AUCF)
	{
		error("expected AUcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0)
	{
		error("AUrq: %d\n", result);
		return False;
	}

	if (opcode & 2)
		in_uint16_be(s, *mcs_userid);

	return s_check_end(s);
}

/* Send a CJrq message (ASN.1 PER) */
static BOOL
mcs_send_cjrq(RDPCLIENT * This, uint16 chanid)
{
	STREAM s;

	DEBUG_RDP5(("Sending CJRQ for channel #%d\n", chanid));

	s = iso_init(This, 5);

	if(s == NULL)
		return False;

	out_uint8(s, (MCS_CJRQ << 2));
	out_uint16_be(s, This->mcs_userid);
	out_uint16_be(s, chanid);

	s_mark_end(s);
	return iso_send(This, s);
}

/* Expect a CJcf message (ASN.1 PER) */
static BOOL
mcs_recv_cjcf(RDPCLIENT * This)
{
	uint8 opcode, result;
	STREAM s;

	s = iso_recv(This, NULL);
	if (s == NULL)
		return False;

	in_uint8(s, opcode);
	if ((opcode >> 2) != MCS_CJCF)
	{
		error("expected CJcf, got %d\n", opcode);
		return False;
	}

	in_uint8(s, result);
	if (result != 0)
	{
		error("CJrq: %d\n", result);
		return False;
	}

	in_uint8s(s, 4);	/* mcs_userid, req_chanid */
	if (opcode & 2)
		in_uint8s(s, 2);	/* join_chanid */

	return s_check_end(s);
}

/* Initialise an MCS transport data packet */
STREAM
mcs_init(RDPCLIENT * This, int length)
{
	STREAM s;

	s = iso_init(This, length + 8);

	if(s == NULL)
		return NULL;

	s_push_layer(s, mcs_hdr, 8);

	return s;
}

/* Send an MCS transport data packet to a specific channel */
BOOL
mcs_send_to_channel(RDPCLIENT * This, STREAM s, uint16 channel)
{
	uint16 length;

	s_pop_layer(s, mcs_hdr);
	length = (uint16)(s->end - s->p - 8);
	length |= 0x8000;

	out_uint8(s, (MCS_SDRQ << 2));
	out_uint16_be(s, This->mcs_userid);
	out_uint16_be(s, channel);
	out_uint8(s, 0x70);	/* flags */
	out_uint16_be(s, length);

	return iso_send(This, s);
}

/* Send an MCS transport data packet to the global channel */
BOOL
mcs_send(RDPCLIENT * This, STREAM s)
{
	return mcs_send_to_channel(This, s, MCS_GLOBAL_CHANNEL);
}

/* Receive an MCS transport data packet */
STREAM
mcs_recv(RDPCLIENT * This, uint16 * channel, uint8 * rdpver)
{
	uint8 opcode, appid, length;
	STREAM s;

	s = iso_recv(This, rdpver);
	if (s == NULL)
		return NULL;
	if (rdpver != NULL)
		if (*rdpver != 3)
			return s;
	in_uint8(s, opcode);
	appid = opcode >> 2;
	if (appid != MCS_SDIN)
	{
		if (appid != MCS_DPUM)
		{
			error("expected data, got %d\n", opcode);
		}
		return NULL;
	}
	in_uint8s(s, 2);	/* userid */
	in_uint16_be(s, *channel);
	in_uint8s(s, 1);	/* flags */
	in_uint8(s, length);
	if (length & 0x80)
		in_uint8s(s, 1);	/* second byte of length */
	return s;
}

/* Establish a connection up to the MCS layer */
BOOL
mcs_connect(RDPCLIENT * This, char *server, char * cookie, STREAM mcs_data)
{
	unsigned int i;

	if (!iso_connect(This, server, cookie))
		return False;

	if (!mcs_send_connect_initial(This, mcs_data) || !mcs_recv_connect_response(This, mcs_data))
		goto error;

	if (!mcs_send_edrq(This) || !mcs_send_aurq(This))
		goto error;

	if (!mcs_recv_aucf(This, &This->mcs_userid))
		goto error;

	if (!mcs_send_cjrq(This, This->mcs_userid + MCS_USERCHANNEL_BASE) || !mcs_recv_cjcf(This))
		goto error;

	if (!mcs_send_cjrq(This, MCS_GLOBAL_CHANNEL) || !mcs_recv_cjcf(This))
		goto error;

	for (i = 0; i < This->num_channels; i++)
	{
		if (!mcs_send_cjrq(This, MCS_GLOBAL_CHANNEL + 1 + i) || !mcs_recv_cjcf(This))
			goto error;
	}
	return True;

	error:
	iso_disconnect(This);
	return False;
}

/* Establish a connection up to the MCS layer */
BOOL
mcs_reconnect(RDPCLIENT * This, char *server, char *cookie, STREAM mcs_data)
{
	unsigned int i;

	if (!iso_reconnect(This, server, cookie))
		return False;

	if (!mcs_send_connect_initial(This, mcs_data) || !mcs_recv_connect_response(This, mcs_data))
		goto error;

	if (!mcs_send_edrq(This) || !mcs_send_aurq(This))
		goto error;

	if (!mcs_recv_aucf(This, &This->mcs_userid))
		goto error;

	if (!mcs_send_cjrq(This, This->mcs_userid + MCS_USERCHANNEL_BASE) || !mcs_recv_cjcf(This))
		goto error;

	if (!mcs_send_cjrq(This, MCS_GLOBAL_CHANNEL) || !mcs_recv_cjcf(This))
		goto error;

	for (i = 0; i < This->num_channels; i++)
	{
		if (!mcs_send_cjrq(This, MCS_GLOBAL_CHANNEL + 1 + i) || !mcs_recv_cjcf(This))
			goto error;
	}
	return True;

      error:
	iso_disconnect(This);
	return False;
}

/* Disconnect from the MCS layer */
void
mcs_disconnect(RDPCLIENT * This)
{
	iso_disconnect(This);
}

/* reset the state of the mcs layer */
void
mcs_reset_state(RDPCLIENT * This)
{
	This->mcs_userid = 0;
	iso_reset_state(This);
}
