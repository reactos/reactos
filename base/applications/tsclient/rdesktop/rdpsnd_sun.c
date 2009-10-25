/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Sound Channel Process Functions - Sun
   Copyright (C) Matthew Chapman 2003
   Copyright (C) GuoJunBo guojunbo@ict.ac.cn 2003
   Copyright (C) Michael Gernoth mike@zerfleddert.de 2003

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
#include <sys/ioctl.h>
#include <sys/audioio.h>

#if (defined(sun) && (defined(__svr4__) || defined(__SVR4)))
#include <stropts.h>
#endif

#define MAX_QUEUE	10

int This->dsp_;
BOOL This->dsp_bu = False;
static BOOL g_reopened;
static BOOL g_swapaudio;
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
	char *dsp_dev = getenv("AUDIODEV");

	if (dsp_dev == NULL)
	{
		dsp_dev = xstrdup("/dev/audio");
	}

	if ((This->dsp_ = open(dsp_dev, O_WRONLY | O_NONBLOCK)) == -1)
	{
		perror(dsp_dev);
		return False;
	}

	/* Non-blocking so that user interface is responsive */
	fcntl(This->dsp_, F_SETFL, fcntl(This->dsp_, F_GETFL) | O_NONBLOCK);

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

#if defined I_FLUSH && defined FLUSHW
	/* Flush the audiobuffer */
	ioctl(This->dsp_, I_FLUSH, FLUSHW);
#endif
#if defined AUDIO_FLUSH
	ioctl(This->dsp_, AUDIO_FLUSH, NULL);
#endif
	close(This->dsp_);
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

	return True;
}

BOOL
wave_out_set_format(WAVEFORMATEX * pwfx)
{
	audio_info_t info;

	ioctl(This->dsp_, AUDIO_DRAIN, 0);
	g_swapaudio = False;
	AUDIO_INITINFO(&info);


	if (pwfx->wBitsPerSample == 8)
	{
		info.play.encoding = AUDIO_ENCODING_LINEAR8;
	}
	else if (pwfx->wBitsPerSample == 16)
	{
		info.play.encoding = AUDIO_ENCODING_LINEAR;
		/* Do we need to swap the 16bit values? (Are we BigEndian) */
#ifdef B_ENDIAN
		g_swapaudio = 1;
#else
		g_swapaudio = 0;
#endif
	}

	g_samplewidth = pwfx->wBitsPerSample / 8;

	if (pwfx->nChannels == 1)
	{
		info.play.channels = 1;
	}
	else if (pwfx->nChannels == 2)
	{
		info.play.channels = 2;
		g_samplewidth *= 2;
	}

	info.play.sample_rate = pwfx->nSamplesPerSec;
	info.play.precision = pwfx->wBitsPerSample;
	info.play.samples = 0;
	info.play.eof = 0;
	info.play.error = 0;
	g_reopened = True;

	if (ioctl(This->dsp_, AUDIO_SETINFO, &info) == -1)
	{
		perror("AUDIO_SETINFO");
		close(This->dsp_);
		return False;
	}

	return True;
}

void
wave_out_volume(uint16 left, uint16 right)
{
	audio_info_t info;
	uint balance;
	uint volume;

	AUDIO_INITINFO(&info);

	volume = (left > right) ? left : right;

	if (volume / AUDIO_MID_BALANCE != 0)
	{
		balance =
			AUDIO_MID_BALANCE - (left / (volume / AUDIO_MID_BALANCE)) +
			(right / (volume / AUDIO_MID_BALANCE));
	}
	else
	{
		balance = AUDIO_MID_BALANCE;
	}

	info.play.gain = volume / (65536 / AUDIO_MAX_GAIN);
	info.play.balance = balance;

	if (ioctl(This->dsp_, AUDIO_SETINFO, &info) == -1)
	{
		perror("AUDIO_SETINFO");
		return;
	}
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
	audio_info_t info;
	ssize_t len;
	unsigned int i;
	uint8 swap;
	STREAM out;
	static BOOL swapped = False;
	static BOOL sentcompletion = True;
	static uint32 samplecnt = 0;
	static uint32 numsamples;

	while (1)
	{
		if (g_reopened)
		{
			/* Device was just (re)openend */
			samplecnt = 0;
			swapped = False;
			sentcompletion = True;
			g_reopened = False;
		}

		if (queue_lo == queue_hi)
		{
			This->dsp_bu = 0;
			return;
		}

		packet = &packet_queue[queue_lo];
		out = &packet->s;

		/* Swap the current packet, but only once */
		if (g_swapaudio && !swapped)
		{
			for (i = 0; i < out->end - out->p; i += 2)
			{
				swap = *(out->p + i);
				*(out->p + i) = *(out->p + i + 1);
				*(out->p + i + 1) = swap;
			}
			swapped = True;
		}

		if (sentcompletion)
		{
			sentcompletion = False;
			numsamples = (out->end - out->p) / g_samplewidth;
		}

		len = 0;

		if (out->end != out->p)
		{
			len = write(This->dsp_, out->p, out->end - out->p);
			if (len == -1)
			{
				if (errno != EWOULDBLOCK)
					perror("write audio");
				This->dsp_bu = 1;
				return;
			}
		}

		out->p += len;
		if (out->p == out->end)
		{
			if (ioctl(This->dsp_, AUDIO_GETINFO, &info) == -1)
			{
				perror("AUDIO_GETINFO");
				return;
			}

			/* Ack the packet, if we have played at least 70% */
			if (info.play.samples >= samplecnt + ((numsamples * 7) / 10))
			{
				samplecnt += numsamples;
				rdpsnd_send_completion(packet->tick, packet->index);
				free(out->data);
				queue_lo = (queue_lo + 1) % MAX_QUEUE;
				swapped = False;
				sentcompletion = True;
			}
			else
			{
				This->dsp_bu = 1;
				return;
			}
		}
	}
}
