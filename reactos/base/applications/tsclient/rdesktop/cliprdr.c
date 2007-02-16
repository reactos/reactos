/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - Clipboard functions
   Copyright (C) Erik Forsberg <forsberg@cendio.se> 2003
   Copyright (C) Matthew Chapman 2003

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

#define CLIPRDR_CONNECT			1
#define CLIPRDR_FORMAT_ANNOUNCE		2
#define CLIPRDR_FORMAT_ACK		3
#define CLIPRDR_DATA_REQUEST		4
#define CLIPRDR_DATA_RESPONSE		5

#define CLIPRDR_REQUEST			0
#define CLIPRDR_RESPONSE		1
#define CLIPRDR_ERROR			2

static void
cliprdr_send_packet(RDPCLIENT * This, uint16 type, uint16 status, uint8 * data, uint32 length)
{
	STREAM s;

	DEBUG_CLIPBOARD(("CLIPRDR send: type=%d, status=%d, length=%d\n", type, status, length));

	s = channel_init(This, This->cliprdr.channel, length + 12);
	out_uint16_le(s, type);
	out_uint16_le(s, status);
	out_uint32_le(s, length);
	out_uint8p(s, data, length);
	out_uint32(s, 0);	/* pad? */
	s_mark_end(s);
	channel_send(This, s, This->cliprdr.channel);
}

/* Helper which announces our readiness to supply clipboard data
   in a single format (such as CF_TEXT) to the RDP side.
   To announce more than one format at a time, use
   cliprdr_send_native_format_announce.
 */
void
cliprdr_send_simple_native_format_announce(RDPCLIENT * This, uint32 format)
{
	uint8 buffer[36];

	DEBUG_CLIPBOARD(("cliprdr_send_simple_native_format_announce\n"));

	buf_out_uint32(buffer, format);
	memset(buffer + 4, 0, sizeof(buffer) - 4);	/* description */
	cliprdr_send_native_format_announce(This, buffer, sizeof(buffer));
}

/* Announces our readiness to supply clipboard data in multiple
   formats, each denoted by a 36-byte format descriptor of
   [ uint32 format + 32-byte description ].
 */
void
cliprdr_send_native_format_announce(RDPCLIENT * This, uint8 * formats_data, uint32 formats_data_length)
{
	DEBUG_CLIPBOARD(("cliprdr_send_native_format_announce\n"));

	cliprdr_send_packet(This, CLIPRDR_FORMAT_ANNOUNCE, CLIPRDR_REQUEST, formats_data,
			    formats_data_length);

	if (formats_data != This->cliprdr.last_formats)
	{
		if (This->cliprdr.last_formats)
			xfree(This->cliprdr.last_formats);

		This->cliprdr.last_formats = xmalloc(formats_data_length);
		memcpy(This->cliprdr.last_formats, formats_data, formats_data_length);
		This->cliprdr.last_formats_length = formats_data_length;
	}
}

void
cliprdr_send_data_request(RDPCLIENT * This, uint32 format)
{
	uint8 buffer[4];

	DEBUG_CLIPBOARD(("cliprdr_send_data_request\n"));
	buf_out_uint32(buffer, format);
	cliprdr_send_packet(This, CLIPRDR_DATA_REQUEST, CLIPRDR_REQUEST, buffer, sizeof(buffer));
}

void
cliprdr_send_data(RDPCLIENT * This, uint8 * data, uint32 length)
{
	DEBUG_CLIPBOARD(("cliprdr_send_data\n"));
	cliprdr_send_packet(This, CLIPRDR_DATA_RESPONSE, CLIPRDR_RESPONSE, data, length);
}

static void
cliprdr_process(RDPCLIENT * This, STREAM s)
{
	uint16 type, status;
	uint32 length, format;
	uint8 *data;

	in_uint16_le(s, type);
	in_uint16_le(s, status);
	in_uint32_le(s, length);
	data = s->p;

	DEBUG_CLIPBOARD(("CLIPRDR recv: type=%d, status=%d, length=%d\n", type, status, length));

	if (status == CLIPRDR_ERROR)
	{
		switch (type)
		{
			case CLIPRDR_FORMAT_ACK:
				/* FIXME: We seem to get this when we send an announce while the server is
				   still processing a paste. Try sending another announce. */
				cliprdr_send_native_format_announce(This, This->cliprdr.last_formats,
								    This->cliprdr.last_formats_length);
				break;
			case CLIPRDR_DATA_RESPONSE:
				ui_clip_request_failed(This);
				break;
			default:
				DEBUG_CLIPBOARD(("CLIPRDR error (type=%d)\n", type));
		}

		return;
	}

	switch (type)
	{
		case CLIPRDR_CONNECT:
			ui_clip_sync(This);
			break;
		case CLIPRDR_FORMAT_ANNOUNCE:
			ui_clip_format_announce(This, data, length);
			cliprdr_send_packet(This, CLIPRDR_FORMAT_ACK, CLIPRDR_RESPONSE, NULL, 0);
			return;
		case CLIPRDR_FORMAT_ACK:
			break;
		case CLIPRDR_DATA_REQUEST:
			in_uint32_le(s, format);
			ui_clip_request_data(This, format);
			break;
		case CLIPRDR_DATA_RESPONSE:
			ui_clip_handle_data(This, data, length);
			break;
		case 7:	/* TODO: W2K3 SP1 sends this on connect with a value of 1 */
			break;
		default:
			unimpl("CLIPRDR packet type %d\n", type);
	}
}

void
cliprdr_set_mode(RDPCLIENT * This, const char *optarg)
{
	ui_clip_set_mode(This, optarg);
}

BOOL
cliprdr_init(RDPCLIENT * This)
{
	This->cliprdr.channel =
		channel_register(This, "cliprdr",
				 CHANNEL_OPTION_INITIALIZED | CHANNEL_OPTION_ENCRYPT_RDP |
				 CHANNEL_OPTION_COMPRESS_RDP | CHANNEL_OPTION_SHOW_PROTOCOL,
				 cliprdr_process);
	return (This->cliprdr.channel != NULL);
}
