/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Protocol services - Virtual channels
   Copyright (C) Erik Forsberg <forsberg@cendio.se> 2003
   Copyright (C) Matthew Chapman 2003-2005

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

// FIXME: header mess
#if 0
#define CHANNEL_CHUNK_LENGTH		1600
#define CHANNEL_FLAG_FIRST		0x01
#define CHANNEL_FLAG_LAST		0x02
#define CHANNEL_FLAG_SHOW_PROTOCOL	0x10
#endif

/* FIXME: We should use the information in TAG_SRV_CHANNELS to map RDP5
   channels to MCS channels.

   The format of TAG_SRV_CHANNELS seems to be

   global_channel_no (uint16le)
   number_of_other_channels (uint16le)
   ..followed by uint16les for the other channels.
*/

VCHANNEL *
channel_register(RDPCLIENT * This, char *name, uint32 flags, void (*callback) (RDPCLIENT *, STREAM))
{
	VCHANNEL *channel;

	if (!This->use_rdp5)
		return NULL;

	if (This->num_channels >= MAX_CHANNELS)
	{
		error("Channel table full, increase MAX_CHANNELS\n");
		return NULL;
	}

	channel = &This->channels[This->num_channels];
	channel->mcs_id = MCS_GLOBAL_CHANNEL + 1 + This->num_channels;
	strncpy(channel->name, name, 8);
	channel->flags = flags;
	channel->process = callback;
	This->num_channels++;
	return channel;
}

STREAM
channel_init(RDPCLIENT * This, VCHANNEL * channel, uint32 length)
{
	STREAM s;

	s = sec_init(This, This->encryption ? SEC_ENCRYPT : 0, length + 8);
	s_push_layer(s, channel_hdr, 8);
	return s;
}

void
channel_send(RDPCLIENT * This, STREAM s, VCHANNEL * channel)
{
	uint32 length, flags;
	uint32 thislength, remaining;
	uint8 *data;

	/* first fragment sent in-place */
	s_pop_layer(s, channel_hdr);
	length = s->end - s->p - sizeof(CHANNEL_PDU_HEADER);

	DEBUG_CHANNEL(("channel_send, length = %d\n", length));

	thislength = MIN(length, CHANNEL_CHUNK_LENGTH);
/* Note: In the original clipboard implementation, this number was
   1592, not 1600. However, I don't remember the reason and 1600 seems
   to work so.. This applies only to *this* length, not the length of
   continuation or ending packets. */
	remaining = length - thislength;
	flags = (remaining == 0) ? CHANNEL_FLAG_FIRST | CHANNEL_FLAG_LAST : CHANNEL_FLAG_FIRST;
	if (channel->flags & CHANNEL_OPTION_SHOW_PROTOCOL)
		flags |= CHANNEL_FLAG_SHOW_PROTOCOL;

	out_uint32_le(s, length);
	out_uint32_le(s, flags);
	data = s->end = s->p + thislength;
	DEBUG_CHANNEL(("Sending %d bytes with FLAG_FIRST\n", thislength));
	sec_send_to_channel(This, s, This->encryption ? SEC_ENCRYPT : 0, channel->mcs_id);

	/* subsequent segments copied (otherwise would have to generate headers backwards) */
	while (remaining > 0)
	{
		thislength = MIN(remaining, CHANNEL_CHUNK_LENGTH);
		remaining -= thislength;
		flags = (remaining == 0) ? CHANNEL_FLAG_LAST : 0;
		if (channel->flags & CHANNEL_OPTION_SHOW_PROTOCOL)
			flags |= CHANNEL_FLAG_SHOW_PROTOCOL;

		DEBUG_CHANNEL(("Sending %d bytes with flags %d\n", thislength, flags));

		s = sec_init(This, This->encryption ? SEC_ENCRYPT : 0, thislength + 8);
		out_uint32_le(s, length);
		out_uint32_le(s, flags);
		out_uint8p(s, data, thislength);
		s_mark_end(s);
		sec_send_to_channel(This, s, This->encryption ? SEC_ENCRYPT : 0, channel->mcs_id);

		data += thislength;
	}
}

void
channel_process(RDPCLIENT * This, STREAM s, uint16 mcs_channel)
{
	uint32 length, flags;
	uint32 thislength;
	VCHANNEL *channel = NULL;
	unsigned int i;
	STREAM in;

	for (i = 0; i < This->num_channels; i++)
	{
		channel = &This->channels[i];
		if (channel->mcs_id == mcs_channel)
			break;
	}

	if (i >= This->num_channels)
		return;

	in_uint32_le(s, length);
	in_uint32_le(s, flags);
	if ((flags & CHANNEL_FLAG_FIRST) && (flags & CHANNEL_FLAG_LAST))
	{
		/* single fragment - pass straight up */
		channel->process(This, s);
	}
	else
	{
		/* add fragment to defragmentation buffer */
		in = &channel->in;
		if (flags & CHANNEL_FLAG_FIRST)
		{
			if (length > in->size)
			{
				in->data = (uint8 *) xrealloc(in->data, length);
				in->size = length;
			}
			in->p = in->data;
		}

		thislength = MIN(s->end - s->p, in->data + in->size - in->p);
		memcpy(in->p, s->p, thislength);
		in->p += thislength;

		if (flags & CHANNEL_FLAG_LAST)
		{
			in->end = in->p;
			in->p = in->data;
			channel->process(This, in);
		}
	}
}
