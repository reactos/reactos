/* -*- c-basic-offset: 8 -*-
   rdesktop: A Remote Desktop Protocol client.
   Sound Channel Process Functions - SGI/IRIX
   Copyright (C) Matthew Chapman 2003
   Copyright (C) GuoJunBo guojunbo@ict.ac.cn 2003
   Copyright (C) Jeremy Meng void.foo@gmail.com 2004, 2005

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
#include <errno.h>
#include <dmedia/audio.h>

/* #define IRIX_DEBUG 1 */

#define IRIX_MAX_VOL     65535

#define MAX_QUEUE	10

int This->dsp_;
ALconfig audioconfig;
ALport output_port;

BOOL This->dsp_bu = False;
static BOOL g_swapaudio;
static int g_snd_rate;
static BOOL g_swapaudio;
static int width = AL_SAMPLE_16;

double min_volume, max_volume, volume_range;
int resource, maxFillable;
int combinedFrameSize;

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
	ALparamInfo pinfo;

#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_open: begin\n");
#endif

	if (alGetParamInfo(AL_DEFAULT_OUTPUT, AL_GAIN, &pinfo) < 0)
	{
		fprintf(stderr, "wave_out_open: alGetParamInfo failed: %s\n",
			alGetErrorString(oserror()));
	}
	min_volume = alFixedToDouble(pinfo.min.ll);
	max_volume = alFixedToDouble(pinfo.max.ll);
	volume_range = (max_volume - min_volume);
#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_open: minvol = %lf, maxvol= %lf, range = %lf.\n",
		min_volume, max_volume, volume_range);
#endif

	queue_lo = queue_hi = 0;

	audioconfig = alNewConfig();
	if (audioconfig == (ALconfig) 0)
	{
		fprintf(stderr, "wave_out_open: alNewConfig failed: %s\n",
			alGetErrorString(oserror()));
		return False;
	}

	output_port = alOpenPort("rdpsnd", "w", 0);
	if (output_port == (ALport) 0)
	{
		fprintf(stderr, "wave_out_open: alOpenPort failed: %s\n",
			alGetErrorString(oserror()));
		return False;
	}

#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_open: returning\n");
#endif
	return True;
}

void
wave_out_close(void)
{
	/* Ack all remaining packets */
#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_close: begin\n");
#endif

	while (queue_lo != queue_hi)
	{
		rdpsnd_send_completion(packet_queue[queue_lo].tick, packet_queue[queue_lo].index);
		free(packet_queue[queue_lo].s.data);
		queue_lo = (queue_lo + 1) % MAX_QUEUE;
	}
	alDiscardFrames(output_port, 0);

	alClosePort(output_port);
	alFreeConfig(audioconfig);
#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_close: returning\n");
#endif
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
	int channels;
	int frameSize, channelCount;
	ALpv params;

#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_set_format: init...\n");
#endif

	g_swapaudio = False;
	if (pwfx->wBitsPerSample == 8)
		width = AL_SAMPLE_8;
	else if (pwfx->wBitsPerSample == 16)
	{
		width = AL_SAMPLE_16;
		/* Do we need to swap the 16bit values? (Are we BigEndian) */
#if (defined(B_ENDIAN))
		g_swapaudio = 1;
#else
		g_swapaudio = 0;
#endif
	}

	/* Limited support to configure an opened audio port in IRIX.  The
	   number of channels is a static setting and can not be changed after
	   a port is opened.  So if the number of channels remains the same, we
	   can configure other settings; otherwise we have to reopen the audio
	   port, using same config. */

	channels = pwfx->nChannels;
	g_snd_rate = pwfx->nSamplesPerSec;

	alSetSampFmt(audioconfig, AL_SAMPFMT_TWOSCOMP);
	alSetWidth(audioconfig, width);
	if (channels != alGetChannels(audioconfig))
	{
		alClosePort(output_port);
		alSetChannels(audioconfig, channels);
		output_port = alOpenPort("rdpsnd", "w", audioconfig);

		if (output_port == (ALport) 0)
		{
			fprintf(stderr, "wave_out_set_format: alOpenPort failed: %s\n",
				alGetErrorString(oserror()));
			return False;
		}

	}

	resource = alGetResource(output_port);
	maxFillable = alGetFillable(output_port);
	channelCount = alGetChannels(audioconfig);
	frameSize = alGetWidth(audioconfig);

	if (frameSize == 0 || channelCount == 0)
	{
		fprintf(stderr, "wave_out_set_format: bad frameSize or channelCount\n");
		return False;
	}
	combinedFrameSize = frameSize * channelCount;

	params.param = AL_RATE;
	params.value.ll = (long long) g_snd_rate << 32;

	if (alSetParams(resource, &params, 1) < 0)
	{
		fprintf(stderr, "wave_set_format: alSetParams failed: %s\n",
			alGetErrorString(oserror()));
		return False;
	}
	if (params.sizeOut < 0)
	{
		fprintf(stderr, "wave_set_format: invalid rate %d\n", g_snd_rate);
		return False;
	}

#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_set_format: returning...\n");
#endif
	return True;
}

void
wave_out_volume(uint16 left, uint16 right)
{
	double gainleft, gainright;
	ALpv pv[1];
	ALfixed gain[8];

#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_volume: begin\n");
	fprintf(stderr, "left='%d', right='%d'\n", left, right);
#endif

	gainleft = (double) left / IRIX_MAX_VOL;
	gainright = (double) right / IRIX_MAX_VOL;

	gain[0] = alDoubleToFixed(min_volume + gainleft * volume_range);
	gain[1] = alDoubleToFixed(min_volume + gainright * volume_range);

	pv[0].param = AL_GAIN;
	pv[0].value.ptr = gain;
	pv[0].sizeIn = 8;
	if (alSetParams(AL_DEFAULT_OUTPUT, pv, 1) < 0)
	{
		fprintf(stderr, "wave_out_volume: alSetParams failed: %s\n",
			alGetErrorString(oserror()));
		return;
	}

#if (defined(IRIX_DEBUG))
	fprintf(stderr, "wave_out_volume: returning\n");
#endif
}

void
wave_out_write(STREAM s, uint16 tick, uint8 index)
{
	struct audio_packet *packet = &packet_queue[queue_hi];
	unsigned int next_hi = (queue_hi + 1) % MAX_QUEUE;

	if (next_hi == queue_lo)
	{
		fprintf(stderr, "No space to queue audio packet\n");
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
	ssize_t len;
	unsigned int i;
	uint8 swap;
	STREAM out;
	static BOOL swapped = False;
	int gf;

	while (1)
	{
		if (queue_lo == queue_hi)
		{
			This->dsp_bu = False;
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

		len = out->end - out->p;

		alWriteFrames(output_port, out->p, len / combinedFrameSize);

		out->p += len;
		if (out->p == out->end)
		{
			gf = alGetFilled(output_port);
			if (gf < (4 * maxFillable / 10))
			{
				rdpsnd_send_completion(packet->tick, packet->index);
				free(out->data);
				queue_lo = (queue_lo + 1) % MAX_QUEUE;
				swapped = False;
			}
			else
			{
#if (defined(IRIX_DEBUG))
/*  				fprintf(stderr,"Busy playing...\n"); */
#endif
				This->dsp_bu = True;
				usleep(10);
				return;
			}
		}
	}
}
