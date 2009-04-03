/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Sound Channel Process Functions - libao-driver
   Copyright (C) Matthew Chapman 2003
   Copyright (C) GuoJunBo guojunbo@ict.ac.cn 2003
   Copyright (C) Michael Gernoth mike@zerfleddert.de 2005

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
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <ao/ao.h>
#include <sys/time.h>

#define MAX_QUEUE	10
#define WAVEOUTBUF	16

int This->dsp_;
ao_device *o_device = NULL;
int default_driver;
int g_samplerate;
int This->channels;
BOOL This->dsp_bu = False;
static BOOL g_reopened;
static short g_samplewidth;

static struct audio_packet
{
	struct stream s;
	uint16 tick;
	uint8 index;
} packet_queue[MAX_QUEUE];
static unsigned int queue_hi, queue_lo;

BOOL
wave_out_open(void)
{
	ao_sample_format format;

	ao_initialize();
	default_driver = ao_default_driver_id();

	format.bits = 16;
	format.channels = 2;
	This->channels = 2;
	format.rate = 44100;
	g_samplerate = 44100;
	format.byte_format = AO_FMT_LITTLE;

	o_device = ao_open_live(default_driver, &format, NULL);
	if (o_device == NULL)
	{
		return False;
	}

	This->dsp_ = 0;
	queue_lo = queue_hi = 0;

	g_reopened = True;

	return True;
}

void
wave_out_close(void)
{
	/* Ack all remaining packets */
	while (queue_lo != queue_hi)
	{
		rdpsnd_send_completion(packet_queue[queue_lo].tick, packet_queue[queue_lo].index);
		free(packet_queue[queue_lo].s.data);
		queue_lo = (queue_lo + 1) % MAX_QUEUE;
	}

	if (o_device != NULL)
		ao_close(o_device);

	ao_shutdown();
}

BOOL
wave_out_format_supported(WAVEFORMATEX * pwfx)
{
	if (pwfx->wFormatTag != WAVE_FORMAT_PCM)
		return False;
	if ((pwfx->nChannels != 1) && (pwfx->nChannels != 2))
		return False;
	if ((pwfx->wBitsPerSample != 8) && (pwfx->wBitsPerSample != 16))
		return False;
	/* The only common denominator between libao output drivers is a sample-rate of
	   44100, we need to upsample 22050 to it */
	if ((pwfx->nSamplesPerSec != 44100) && (pwfx->nSamplesPerSec != 22050))
		return False;

	return True;
}

BOOL
wave_out_set_format(WAVEFORMATEX * pwfx)
{
	ao_sample_format format;

	format.bits = pwfx->wBitsPerSample;
	format.channels = pwfx->nChannels;
	This->channels = pwfx->nChannels;
	format.rate = 44100;
	g_samplerate = pwfx->nSamplesPerSec;
	format.byte_format = AO_FMT_LITTLE;

	g_samplewidth = pwfx->wBitsPerSample / 8;

	if (o_device != NULL)
		ao_close(o_device);

	o_device = ao_open_live(default_driver, &format, NULL);
	if (o_device == NULL)
	{
		return False;
	}

	g_reopened = True;

	return True;
}

void
wave_out_volume(uint16 left, uint16 right)
{
	warning("volume changes not supported with libao-output\n");
}

void
wave_out_write(STREAM s, uint16 tick, uint8 index)
{
	struct audio_packet *packet = &packet_queue[queue_hi];
	unsigned int next_hi = (queue_hi + 1) % MAX_QUEUE;

	if (next_hi == queue_lo)
	{
		error("No space to queue audio packet\n");
		return;
	}

	queue_hi = next_hi;

	packet->s = *s;
	packet->tick = tick;
	packet->index = index;
	packet->s.p += 4;

	/* we steal the data buffer from s, give it a new one */
	s->data = malloc(s->size);

	if (!This->dsp_bu)
		wave_out_play();
}

void
wave_out_play(void)
{
	struct audio_packet *packet;
	STREAM out;
	char outbuf[WAVEOUTBUF];
	int offset, len, i;
	static long prev_s, prev_us;
	unsigned int duration;
	struct timeval tv;
	int next_tick;

	if (g_reopened)
	{
		g_reopened = False;
		gettimeofday(&tv, NULL);
		prev_s = tv.tv_sec;
		prev_us = tv.tv_usec;
	}

	if (queue_lo == queue_hi)
	{
		This->dsp_bu = 0;
		return;
	}

	packet = &packet_queue[queue_lo];
	out = &packet->s;

	if (((queue_lo + 1) % MAX_QUEUE) != queue_hi)
	{
		next_tick = packet_queue[(queue_lo + 1) % MAX_QUEUE].tick;
	}
	else
	{
		next_tick = (packet->tick + 65535) % 65536;
	}

	len = 0;

	if (g_samplerate == 22050)
	{
		/* Resample to 44100 */
		for (i = 0; (i < ((WAVEOUTBUF / 4) * (3 - g_samplewidth))) && (out->p < out->end);
		     i++)
		{
			/* On a stereo-channel we must make sure that left and right
			   does not get mixed up, so we need to expand the sample-
			   data with channels in mind: 1234 -> 12123434
			   If we have a mono-channel, we can expand the data by simply
			   doubling the sample-data: 1234 -> 11223344 */
			if (This->channels == 2)
				offset = ((i * 2) - (i & 1)) * g_samplewidth;
			else
				offset = (i * 2) * g_samplewidth;

			memcpy(&outbuf[offset], out->p, g_samplewidth);
			memcpy(&outbuf[This->channels * g_samplewidth + offset], out->p, g_samplewidth);

			out->p += g_samplewidth;
			len += 2 * g_samplewidth;
		}
	}
	else
	{
		len = (WAVEOUTBUF > (out->end - out->p)) ? (out->end - out->p) : WAVEOUTBUF;
		memcpy(outbuf, out->p, len);
		out->p += len;
	}

	ao_play(o_device, outbuf, len);

	gettimeofday(&tv, NULL);

	duration = ((tv.tv_sec - prev_s) * 1000000 + (tv.tv_usec - prev_us)) / 1000;

	if (packet->tick > next_tick)
		next_tick += 65536;

	if ((out->p == out->end) || duration > next_tick - packet->tick + 500)
	{
		prev_s = tv.tv_sec;
		prev_us = tv.tv_usec;

		if (abs((next_tick - packet->tick) - duration) > 20)
		{
			DEBUG(("duration: %d, calc: %d, ", duration, next_tick - packet->tick));
			DEBUG(("last: %d, is: %d, should: %d\n", packet->tick,
			       (packet->tick + duration) % 65536, next_tick % 65536));
		}

		/* Until all drivers are using the windows sound-ticks, we need to
		   substract the 50 ticks added later by rdpsnd.c */
		rdpsnd_send_completion(((packet->tick + duration) % 65536) - 50, packet->index);
		free(out->data);
		queue_lo = (queue_lo + 1) % MAX_QUEUE;
	}

	This->dsp_bu = 1;
	return;
}
