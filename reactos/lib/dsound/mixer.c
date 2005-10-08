/*  			DirectSound
 *
 * Copyright 1998 Marcus Meissner
 * Copyright 1998 Rob Riggs
 * Copyright 2000-2002 TransGaming Technologies, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <math.h>	/* Insomnia - pow() function */

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"
#include "mmsystem.h"
#include "winreg.h"
#include "winternl.h"
#include "mmddk.h"
#include "wine/windef16.h"
#include "wine/debug.h"
#include "dsound.h"
#include "dsdriver.h"
#include "dsound_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(dsound);

void DSOUND_RecalcVolPan(PDSVOLUMEPAN volpan)
{
	double temp;
	TRACE("(%p)\n",volpan);

	TRACE("Vol=%ld Pan=%ld\n", volpan->lVolume, volpan->lPan);
	/* the AmpFactors are expressed in 16.16 fixed point */
	volpan->dwVolAmpFactor = (ULONG) (pow(2.0, volpan->lVolume / 600.0) * 0xffff);
	/* FIXME: dwPan{Left|Right}AmpFactor */

	/* FIXME: use calculated vol and pan ampfactors */
	temp = (double) (volpan->lVolume - (volpan->lPan > 0 ? volpan->lPan : 0));
	volpan->dwTotalLeftAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 0xffff);
	temp = (double) (volpan->lVolume + (volpan->lPan < 0 ? volpan->lPan : 0));
	volpan->dwTotalRightAmpFactor = (ULONG) (pow(2.0, temp / 600.0) * 0xffff);

	TRACE("left = %lx, right = %lx\n", volpan->dwTotalLeftAmpFactor, volpan->dwTotalRightAmpFactor);
}

void DSOUND_AmpFactorToVolPan(PDSVOLUMEPAN volpan)
{
    double left,right;
    TRACE("(%p)\n",volpan);

    TRACE("left=%lx, right=%lx\n",volpan->dwTotalLeftAmpFactor,volpan->dwTotalRightAmpFactor);
    if (volpan->dwTotalLeftAmpFactor==0)
        left=-10000;
    else
        left=600 * log(((double)volpan->dwTotalLeftAmpFactor) / 0xffff) / log(2);
    if (volpan->dwTotalRightAmpFactor==0)
        right=-10000;
    else
        right=600 * log(((double)volpan->dwTotalRightAmpFactor) / 0xffff) / log(2);
    if (left<right)
    {
        volpan->lVolume=right;
        volpan->dwVolAmpFactor=volpan->dwTotalRightAmpFactor;
    }
    else
    {
        volpan->lVolume=left;
        volpan->dwVolAmpFactor=volpan->dwTotalLeftAmpFactor;
    }
    if (volpan->lVolume < -10000)
        volpan->lVolume=-10000;
    volpan->lPan=right-left;
    if (volpan->lPan < -10000)
        volpan->lPan=-10000;

    TRACE("Vol=%ld Pan=%ld\n", volpan->lVolume, volpan->lPan);
}

void DSOUND_RecalcFormat(IDirectSoundBufferImpl *dsb)
{
	DWORD sw;
	TRACE("(%p)\n",dsb);

	sw = dsb->pwfx->nChannels * (dsb->pwfx->wBitsPerSample / 8);
	/* calculate the 10ms write lead */
	dsb->writelead = (dsb->freq / 100) * sw;
}

void DSOUND_CheckEvent(IDirectSoundBufferImpl *dsb, int len)
{
	int			i;
	DWORD			offset;
	LPDSBPOSITIONNOTIFY	event;
	TRACE("(%p,%d)\n",dsb,len);

	if (dsb->nrofnotifies == 0)
		return;

	TRACE("(%p) buflen = %ld, playpos = %ld, len = %d\n",
		dsb, dsb->buflen, dsb->playpos, len);
	for (i = 0; i < dsb->nrofnotifies ; i++) {
		event = dsb->notifies + i;
		offset = event->dwOffset;
		TRACE("checking %d, position %ld, event = %p\n",
			i, offset, event->hEventNotify);
		/* DSBPN_OFFSETSTOP has to be the last element. So this is */
		/* OK. [Inside DirectX, p274] */
		/*  */
		/* This also means we can't sort the entries by offset, */
		/* because DSBPN_OFFSETSTOP == -1 */
		if (offset == DSBPN_OFFSETSTOP) {
			if (dsb->state == STATE_STOPPED) {
				SetEvent(event->hEventNotify);
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				return;
			} else
				return;
		}
		if ((dsb->playpos + len) >= dsb->buflen) {
			if ((offset < ((dsb->playpos + len) % dsb->buflen)) ||
			    (offset >= dsb->playpos)) {
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		} else {
			if ((offset >= dsb->playpos) && (offset < (dsb->playpos + len))) {
				TRACE("signalled event %p (%d)\n", event->hEventNotify, i);
				SetEvent(event->hEventNotify);
			}
		}
	}
}

/* WAV format info can be found at:
 *
 *    http://www.cwi.nl/ftp/audio/AudioFormats.part2
 *    ftp://ftp.cwi.nl/pub/audio/RIFF-format
 *
 * Import points to remember:
 *    8-bit WAV is unsigned
 *    16-bit WAV is signed
 */
 /* Use the same formulas as pcmconverter.c */
static inline INT16 cvtU8toS16(BYTE b)
{
    return (short)((b+(b << 8))-32768);
}

static inline BYTE cvtS16toU8(INT16 s)
{
    return (s >> 8) ^ (unsigned char)0x80;
}

static inline void cp_fields(const IDirectSoundBufferImpl *dsb, BYTE *ibuf, BYTE *obuf )
{
        INT fl,fr;

        if (dsb->pwfx->wBitsPerSample == 8)  {
                if (dsb->dsound->pwfx->wBitsPerSample == 8 &&
                    dsb->dsound->pwfx->nChannels == dsb->pwfx->nChannels) {
                        /* avoid needless 8->16->8 conversion */
                        *obuf=*ibuf;
                        if (dsb->pwfx->nChannels==2)
                                *(obuf+1)=*(ibuf+1);
                        return;
                }
                fl = cvtU8toS16(*ibuf);
                fr = (dsb->pwfx->nChannels==2 ? cvtU8toS16(*(ibuf + 1)) : fl);
        } else {
                fl = *((INT16 *)ibuf);
                fr = (dsb->pwfx->nChannels==2 ? *(((INT16 *)ibuf) + 1)  : fl);
        }

        if (dsb->dsound->pwfx->nChannels == 2) {
                if (dsb->dsound->pwfx->wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        *(obuf + 1) = cvtS16toU8(fr);
                        return;
                }
                if (dsb->dsound->pwfx->wBitsPerSample == 16) {
                        *((INT16 *)obuf) = fl;
                        *(((INT16 *)obuf) + 1) = fr;
                        return;
                }
        }
        if (dsb->dsound->pwfx->nChannels == 1) {
                fl = (fl + fr) >> 1;
                if (dsb->dsound->pwfx->wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        return;
                }
                if (dsb->dsound->pwfx->wBitsPerSample == 16) {
                        *((INT16 *)obuf) = fl;
                        return;
                }
        }
}

/* Now with PerfectPitch (tm) technology */
static INT DSOUND_MixerNorm(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i, size, ipos, ilen;
	BYTE	*ibp, *obp;
	INT	iAdvance = dsb->pwfx->nBlockAlign;
	INT	oAdvance = dsb->dsound->pwfx->nBlockAlign;

	ibp = dsb->buffer->memory + dsb->buf_mixpos;
	obp = buf;

	TRACE("(%p, %p, %p), buf_mixpos=%ld\n", dsb, ibp, obp, dsb->buf_mixpos);
	/* Check for the best case */
	if ((dsb->freq == dsb->dsound->pwfx->nSamplesPerSec) &&
	    (dsb->pwfx->wBitsPerSample == dsb->dsound->pwfx->wBitsPerSample) &&
	    (dsb->pwfx->nChannels == dsb->dsound->pwfx->nChannels)) {
	        DWORD bytesleft = dsb->buflen - dsb->buf_mixpos;
		TRACE("(%p) Best case\n", dsb);
	    	if (len <= bytesleft )
			memcpy(obp, ibp, len);
		else { /* wrap */
			memcpy(obp, ibp, bytesleft );
			memcpy(obp + bytesleft, dsb->buffer->memory, len - bytesleft);
		}
		return len;
	}

	/* Check for same sample rate */
	if (dsb->freq == dsb->dsound->pwfx->nSamplesPerSec) {
		TRACE("(%p) Same sample rate %ld = primary %ld\n", dsb,
			dsb->freq, dsb->dsound->pwfx->nSamplesPerSec);
		ilen = 0;
		for (i = 0; i < len; i += oAdvance) {
			cp_fields(dsb, ibp, obp );
			ibp += iAdvance;
			ilen += iAdvance;
			obp += oAdvance;
			if (ibp >= (BYTE *)(dsb->buffer->memory + dsb->buflen))
				ibp = dsb->buffer->memory;	/* wrap */
		}
		return (ilen);
	}

	/* Mix in different sample rates */
	/* */
	/* New PerfectPitch(tm) Technology (c) 1998 Rob Riggs */
	/* Patent Pending :-] */

	/* Patent enhancements (c) 2000 Ove Kåven,
	 * TransGaming Technologies Inc. */

	/* FIXME("(%p) Adjusting frequency: %ld -> %ld (need optimization)\n",
	   dsb, dsb->freq, dsb->dsound->pwfx->nSamplesPerSec); */

	size = len / oAdvance;
	ilen = 0;
	ipos = dsb->buf_mixpos;
	for (i = 0; i < size; i++) {
                cp_fields(dsb, (dsb->buffer->memory + ipos), obp);
		obp += oAdvance;
		dsb->freqAcc += dsb->freqAdjust;
		if (dsb->freqAcc >= (1<<DSOUND_FREQSHIFT)) {
			ULONG adv = (dsb->freqAcc>>DSOUND_FREQSHIFT) * iAdvance;
			dsb->freqAcc &= (1<<DSOUND_FREQSHIFT)-1;
			ipos += adv; ilen += adv;
			ipos %= dsb->buflen;
		}
	}
	return ilen;
}

static void DSOUND_MixerVol(IDirectSoundBufferImpl *dsb, BYTE *buf, INT len)
{
	INT	i;
	BYTE	*bpc = buf;
	INT16	*bps = (INT16 *) buf;

	TRACE("(%p,%p,%d)\n",dsb,buf,len);
	TRACE("left = %lx, right = %lx\n", dsb->cvolpan.dwTotalLeftAmpFactor, 
		dsb->cvolpan.dwTotalRightAmpFactor);

	if ((!(dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) || (dsb->cvolpan.lPan == 0)) &&
	    (!(dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) || (dsb->cvolpan.lVolume == 0)) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		return;		/* Nothing to do */

	/* If we end up with some bozo coder using panning or 3D sound */
	/* with a mono primary buffer, it could sound very weird using */
	/* this method. Oh well, tough patooties. */

	switch (dsb->dsound->pwfx->wBitsPerSample) {
	case 8:
		/* 8-bit WAV is unsigned, but we need to operate */
		/* on signed data for this to work properly */
		switch (dsb->dsound->pwfx->nChannels) {
		case 1:
			for (i = 0; i < len; i++) {
				INT val = *bpc - 128;
				val = (val * dsb->cvolpan.dwTotalLeftAmpFactor) >> 16;
				*bpc = val + 128;
				bpc++;
			}
			break;
		case 2:
			for (i = 0; i < len; i+=2) {
				INT val = *bpc - 128;
				val = (val * dsb->cvolpan.dwTotalLeftAmpFactor) >> 16;
				*bpc++ = val + 128;
				val = *bpc - 128;
				val = (val * dsb->cvolpan.dwTotalRightAmpFactor) >> 16;
				*bpc = val + 128;
				bpc++;
			}
			break;
		default:
			FIXME("doesn't support %d channels\n", dsb->dsound->pwfx->nChannels);
			break;
		}
		break;
	case 16:
		/* 16-bit WAV is signed -- much better */
		switch (dsb->dsound->pwfx->nChannels) {
		case 1:
			for (i = 0; i < len; i += 2) {
				*bps = (*bps * dsb->cvolpan.dwTotalLeftAmpFactor) >> 16;
				bps++;
			}
			break;
		case 2:
			for (i = 0; i < len; i += 4) {
				*bps = (*bps * dsb->cvolpan.dwTotalLeftAmpFactor) >> 16;
				bps++;
				*bps = (*bps * dsb->cvolpan.dwTotalRightAmpFactor) >> 16;
				bps++;
			}
			break;
		default:
			FIXME("doesn't support %d channels\n", dsb->dsound->pwfx->nChannels);
			break;
		}
		break;
	default:
		FIXME("doesn't support %d bit samples\n", dsb->dsound->pwfx->wBitsPerSample);
		break;
	}
}

static void *tmp_buffer;
static size_t tmp_buffer_len = 0;

static void *DSOUND_tmpbuffer(size_t len)
{
  if (len>tmp_buffer_len) {
    void *new_buffer = realloc(tmp_buffer, len);
    if (new_buffer) {
      tmp_buffer = new_buffer;
      tmp_buffer_len = len;
    }
    return new_buffer;
  }
  return tmp_buffer;
}

static DWORD DSOUND_MixInBuffer(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD fraglen)
{
	INT	i, len, ilen, temp, field, nBlockAlign, todo;
	BYTE	*buf, *ibuf;

	TRACE("(%p,%ld,%ld)\n",dsb,writepos,fraglen);

	len = fraglen;
	if (!(dsb->playflags & DSBPLAY_LOOPING)) {
		temp = MulDiv(dsb->dsound->pwfx->nAvgBytesPerSec, dsb->buflen,
			dsb->nAvgBytesPerSec) -
		       MulDiv(dsb->dsound->pwfx->nAvgBytesPerSec, dsb->buf_mixpos,
			dsb->nAvgBytesPerSec);
		len = (len > temp) ? temp : len;
	}
	nBlockAlign = dsb->dsound->pwfx->nBlockAlign;
	len = len / nBlockAlign * nBlockAlign;	/* data alignment */

	if (len == 0) {
		/* This should only happen if we aren't looping and temp < nBlockAlign */
		return 0;
	}

	/* Been seeing segfaults in malloc() for some reason... */
	TRACE("allocating buffer (size = %d)\n", len);
	if ((buf = ibuf = (BYTE *) DSOUND_tmpbuffer(len)) == NULL)
		return 0;

	TRACE("MixInBuffer (%p) len = %d, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		DSOUND_MixerVol(dsb, ibuf, len);

	if (dsb->dsound->pwfx->wBitsPerSample == 8) {
		BYTE	*obuf = dsb->dsound->buffer + writepos;

		if ((writepos + len) <= dsb->dsound->buflen)
			todo = len;
		else
			todo = dsb->dsound->buflen - writepos;

		for (i = 0; i < todo; i++) {
			/* 8-bit WAV is unsigned */
			field = (*ibuf++ - 128);
			field += (*obuf - 128);
			if (field > 127) field = 127;
			else if (field < -128) field = -128;
			*obuf++ = field + 128;
		}
 
		if (todo < len) {
			todo = len - todo;
			obuf = dsb->dsound->buffer;

			for (i = 0; i < todo; i++) {
				/* 8-bit WAV is unsigned */
				field = (*ibuf++ - 128);
				field += (*obuf - 128);
				if (field > 127) field = 127;
				else if (field < -128) field = -128;
				*obuf++ = field + 128;
			}
		}
        } else {
		INT16	*ibufs, *obufs;

		ibufs = (INT16 *) ibuf;
		obufs = (INT16 *)(dsb->dsound->buffer + writepos);

		if ((writepos + len) <= dsb->dsound->buflen)
			todo = len / 2;
		else
			todo = (dsb->dsound->buflen - writepos) / 2;

		for (i = 0; i < todo; i++) {
			/* 16-bit WAV is signed */
			field = *ibufs++;
			field += *obufs;
			if (field > 32767) field = 32767;
			else if (field < -32768) field = -32768;
			*obufs++ = field;
		}

		if (todo < (len / 2)) {
			todo = (len / 2) - todo;
			obufs = (INT16 *)dsb->dsound->buffer;

			for (i = 0; i < todo; i++) {
				/* 16-bit WAV is signed */
				field = *ibufs++;
				field += *obufs;
				if (field > 32767) field = 32767;
				else if (field < -32768) field = -32768;
				*obufs++ = field;
			}
		}
        }

	if (dsb->leadin && (dsb->startpos > dsb->buf_mixpos) && (dsb->startpos <= dsb->buf_mixpos + ilen)) {
		/* HACK... leadin should be reset when the PLAY position reaches the startpos,
		 * not the MIX position... but if the sound buffer is bigger than our prebuffering
		 * (which must be the case for the streaming buffers that need this hack anyway)
		 * plus DS_HEL_MARGIN or equivalent, then this ought to work anyway. */
		dsb->leadin = FALSE;
	}

	dsb->buf_mixpos += ilen;

	if (dsb->buf_mixpos >= dsb->buflen) {
		if (dsb->playflags & DSBPLAY_LOOPING) {
			/* wrap */
			dsb->buf_mixpos %= dsb->buflen;
			if (dsb->leadin && (dsb->startpos <= dsb->buf_mixpos))
				dsb->leadin = FALSE; /* HACK: see above */
		}
	}

	return len;
}

static void DSOUND_PhaseCancel(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD len)
{
	INT     ilen, field, nBlockAlign;
	UINT    i, todo;
	BYTE	*buf, *ibuf;

	TRACE("(%p,%ld,%ld)\n",dsb,writepos,len);

	nBlockAlign = dsb->dsound->pwfx->nBlockAlign;
	len = len / nBlockAlign * nBlockAlign;  /* data alignment */

	TRACE("allocating buffer (size = %ld)\n", len);
	if ((buf = ibuf = (BYTE *) DSOUND_tmpbuffer(len)) == NULL)
		return;

	TRACE("PhaseCancel (%p) len = %ld, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		DSOUND_MixerVol(dsb, ibuf, len);

	/* subtract instead of add, to phase out premixed data */
	if (dsb->dsound->pwfx->wBitsPerSample == 8) {
		BYTE	*obuf = dsb->dsound->buffer + writepos;

		if ((writepos + len) <= dsb->dsound->buflen)
			todo = len;
		else
			todo = dsb->dsound->buflen - writepos;

		for (i = 0; i < todo; i++) {
			/* 8-bit WAV is unsigned */
			field = (*ibuf++ - 128);
			field -= (*obuf - 128);
			if (field > 127) field = 127;
			else if (field < -128) field = -128;
			*obuf++ = field + 128;
		}
 
		if (todo < len) {
			todo = len - todo;
			obuf = dsb->dsound->buffer;

			for (i = 0; i < todo; i++) {
				/* 8-bit WAV is unsigned */
				field = (*ibuf++ - 128);
				field -= (*obuf - 128);
				if (field > 127) field = 127;
				else if (field < -128) field = -128;
				*obuf++ = field + 128;
			}
		}
        } else {
		INT16	*ibufs, *obufs;

		ibufs = (INT16 *) ibuf;
		obufs = (INT16 *)(dsb->dsound->buffer + writepos);

		if ((writepos + len) <= dsb->dsound->buflen)
			todo = len / 2;
		else
			todo = (dsb->dsound->buflen - writepos) / 2;

		for (i = 0; i < todo; i++) {
			/* 16-bit WAV is signed */
			field = *ibufs++;
			field -= *obufs;
			if (field > 32767) field = 32767;
			else if (field < -32768) field = -32768;
			*obufs++ = field;
		}

		if (todo < (len / 2)) {
			todo = (len / 2) - todo;
			obufs = (INT16 *)dsb->dsound->buffer;

			for (i = 0; i < todo; i++) {
				/* 16-bit WAV is signed */
				field = *ibufs++;
				field -= *obufs;
				if (field > 32767) field = 32767;
				else if (field < -32768) field = -32768;
				*obufs++ = field;
			}
		}
        }
}

static void DSOUND_MixCancel(IDirectSoundBufferImpl *dsb, DWORD writepos, BOOL cancel)
{
	DWORD   size, flen, len, npos, nlen;
	INT	iAdvance = dsb->pwfx->nBlockAlign;
	INT	oAdvance = dsb->dsound->pwfx->nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? dsb->dsound->buflen : 0) +
		dsb->primary_mixpos - writepos;

	TRACE("(%p, %ld), buf_mixpos=%ld\n", dsb, writepos, dsb->buf_mixpos);

	/* backtrack the mix position */
	size = primary_done / oAdvance;
	flen = size * dsb->freqAdjust;
	len = (flen >> DSOUND_FREQSHIFT) * iAdvance;
	flen &= (1<<DSOUND_FREQSHIFT)-1;
	while (dsb->freqAcc < flen) {
		len += iAdvance;
		dsb->freqAcc += 1<<DSOUND_FREQSHIFT;
	}
	len %= dsb->buflen;
	npos = ((dsb->buf_mixpos < len) ? dsb->buflen : 0) +
		dsb->buf_mixpos - len;
	if (dsb->leadin && (dsb->startpos > npos) && (dsb->startpos <= npos + len)) {
		/* stop backtracking at startpos */
		npos = dsb->startpos;
		len = ((dsb->buf_mixpos < npos) ? dsb->buflen : 0) +
			dsb->buf_mixpos - npos;
		flen = dsb->freqAcc;
		nlen = len / dsb->pwfx->nBlockAlign;
		nlen = ((nlen << DSOUND_FREQSHIFT) + flen) / dsb->freqAdjust;
		nlen *= dsb->dsound->pwfx->nBlockAlign;
		writepos =
			((dsb->primary_mixpos < nlen) ? dsb->dsound->buflen : 0) +
			dsb->primary_mixpos - nlen;
	}

	dsb->freqAcc -= flen;
	dsb->buf_mixpos = npos;
	dsb->primary_mixpos = writepos;

	TRACE("new buf_mixpos=%ld, primary_mixpos=%ld (len=%ld)\n",
	      dsb->buf_mixpos, dsb->primary_mixpos, len);

	if (cancel) DSOUND_PhaseCancel(dsb, writepos, len);
}

void DSOUND_MixCancelAt(IDirectSoundBufferImpl *dsb, DWORD buf_writepos)
{
#if 0
	DWORD   i, size, flen, len, npos, nlen;
	INT	iAdvance = dsb->pwfx->nBlockAlign;
	INT	oAdvance = dsb->dsound->pwfx->nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
#endif

	WARN("(%p, %ld), buf_mixpos=%ld\n", dsb, buf_writepos, dsb->buf_mixpos);
	/* since this is not implemented yet, just cancel *ALL* prebuffering for now
	 * (which is faster anyway when there's only a single secondary buffer) */
	dsb->dsound->need_remix = TRUE;
}

void DSOUND_ForceRemix(IDirectSoundBufferImpl *dsb)
{
	TRACE("(%p)\n",dsb);
	EnterCriticalSection(&dsb->lock);
	if (dsb->state == STATE_PLAYING) {
#if 0 /* this may not be quite reliable yet */
		dsb->need_remix = TRUE;
#else
		dsb->dsound->need_remix = TRUE;
#endif
	}
	LeaveCriticalSection(&dsb->lock);
}

static DWORD DSOUND_MixOne(IDirectSoundBufferImpl *dsb, DWORD playpos, DWORD writepos, DWORD mixlen)
{
	DWORD len, slen;
	/* determine this buffer's write position */
	DWORD buf_writepos = DSOUND_CalcPlayPosition(dsb, dsb->state & dsb->dsound->state, writepos,
						     writepos, dsb->primary_mixpos, dsb->buf_mixpos);
	/* determine how much already-mixed data exists */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? dsb->dsound->buflen : 0) +
		dsb->primary_mixpos - writepos;
	DWORD adv_done =
		((dsb->dsound->mixpos < writepos) ? dsb->dsound->buflen : 0) +
		dsb->dsound->mixpos - writepos;
	DWORD played =
		((buf_writepos < dsb->playpos) ? dsb->buflen : 0) +
		buf_writepos - dsb->playpos;
	DWORD buf_left = dsb->buflen - buf_writepos;
	int still_behind;

	TRACE("(%p,%ld,%ld,%ld)\n",dsb,playpos,writepos,mixlen);
	TRACE("buf_writepos=%ld, primary_writepos=%ld\n", buf_writepos, writepos);
	TRACE("buf_done=%ld, primary_done=%ld\n", buf_done, primary_done);
	TRACE("buf_mixpos=%ld, primary_mixpos=%ld, mixlen=%ld\n", dsb->buf_mixpos, dsb->primary_mixpos,
	      mixlen);
	TRACE("looping=%ld, startpos=%ld, leadin=%ld\n", dsb->playflags, dsb->startpos, dsb->leadin);

	/* check for notification positions */
	if (dsb->dsbd.dwFlags & DSBCAPS_CTRLPOSITIONNOTIFY &&
	    dsb->state != STATE_STARTING) {
		DSOUND_CheckEvent(dsb, played);
	}

	/* save write position for non-GETCURRENTPOSITION2... */
	dsb->playpos = buf_writepos;

	/* check whether CalcPlayPosition detected a mixing underrun */
	if ((buf_done == 0) && (dsb->primary_mixpos != writepos)) {
		/* it did, but did we have more to play? */
		if ((dsb->playflags & DSBPLAY_LOOPING) ||
		    (dsb->buf_mixpos < dsb->buflen)) {
			/* yes, have to recover */
			ERR("underrun on sound buffer %p\n", dsb);
			TRACE("recovering from underrun: primary_mixpos=%ld\n", writepos);
		}
		dsb->primary_mixpos = writepos;
		primary_done = 0;
	}
	/* determine how far ahead we should mix */
	if (((dsb->playflags & DSBPLAY_LOOPING) ||
	     (dsb->leadin && (dsb->probably_valid_to != 0))) &&
	    !(dsb->dsbd.dwFlags & DSBCAPS_STATIC)) {
		/* if this is a streaming buffer, it typically means that
		 * we should defer mixing past probably_valid_to as long
		 * as we can, to avoid unnecessary remixing */
		/* the heavy-looking calculations shouldn't be that bad,
		 * as any game isn't likely to be have more than 1 or 2
		 * streaming buffers in use at any time anyway... */
		DWORD probably_valid_left =
			(dsb->probably_valid_to == (DWORD)-1) ? dsb->buflen :
			((dsb->probably_valid_to < buf_writepos) ? dsb->buflen : 0) +
			dsb->probably_valid_to - buf_writepos;
		/* check for leadin condition */
		if ((probably_valid_left == 0) &&
		    (dsb->probably_valid_to == dsb->startpos) &&
		    dsb->leadin)
			probably_valid_left = dsb->buflen;
		TRACE("streaming buffer probably_valid_to=%ld, probably_valid_left=%ld\n",
		      dsb->probably_valid_to, probably_valid_left);
		/* check whether the app's time is already up */
		if (probably_valid_left < dsb->writelead) {
			WARN("probably_valid_to now within writelead, possible streaming underrun\n");
			/* once we pass the point of no return,
			 * no reason to hold back anymore */
			dsb->probably_valid_to = (DWORD)-1;
			/* we just have to go ahead and mix what we have,
			 * there's no telling what the app is thinking anyway */
		} else {
			/* adjust for our frequency and our sample size */
			probably_valid_left = MulDiv(probably_valid_left,
						     1 << DSOUND_FREQSHIFT,
						     dsb->pwfx->nBlockAlign * dsb->freqAdjust) *
				              dsb->dsound->pwfx->nBlockAlign;
			/* check whether to clip mix_len */
			if (probably_valid_left < mixlen) {
				TRACE("clipping to probably_valid_left=%ld\n", probably_valid_left);
				mixlen = probably_valid_left;
			}
		}
	}
	/* cut mixlen with what's already been mixed */
	if (mixlen < primary_done) {
		/* huh? and still CalcPlayPosition didn't
		 * detect an underrun? */
		FIXME("problem with underrun detection (mixlen=%ld < primary_done=%ld)\n", mixlen, primary_done);
		return 0;
	}
	len = mixlen - primary_done;
	TRACE("remaining mixlen=%ld\n", len);

	if (len < dsb->dsound->fraglen) {
		/* smaller than a fragment, wait until it gets larger
		 * before we take the mixing overhead */
		TRACE("mixlen not worth it, deferring mixing\n");
		still_behind = 1;
		goto post_mix;
	}

	/* ok, we know how much to mix, let's go */
	still_behind = (adv_done > primary_done);
	while (len) {
		slen = dsb->dsound->buflen - dsb->primary_mixpos;
		if (slen > len) slen = len;
		slen = DSOUND_MixInBuffer(dsb, dsb->primary_mixpos, slen);

		if ((dsb->primary_mixpos < dsb->dsound->mixpos) &&
		    (dsb->primary_mixpos + slen >= dsb->dsound->mixpos))
			still_behind = FALSE;

		dsb->primary_mixpos += slen; len -= slen;
		dsb->primary_mixpos %= dsb->dsound->buflen;

		if ((dsb->state == STATE_STOPPED) || !slen) break;
	}
	TRACE("new primary_mixpos=%ld, primary_advbase=%ld\n", dsb->primary_mixpos, dsb->dsound->mixpos);
	TRACE("mixed data len=%ld, still_behind=%d\n", mixlen-len, still_behind);

post_mix:
	/* check if buffer should be considered complete */
	if (buf_left < dsb->writelead &&
	    !(dsb->playflags & DSBPLAY_LOOPING)) {
		dsb->state = STATE_STOPPED;
		dsb->playpos = 0;
		dsb->last_playpos = 0;
		dsb->buf_mixpos = 0;
		dsb->leadin = FALSE;
		DSOUND_CheckEvent(dsb, buf_left);
	}

	/* return how far we think the primary buffer can
	 * advance its underrun detector...*/
	if (still_behind) return 0;
	if ((mixlen - len) < primary_done) return 0;
	slen = ((dsb->primary_mixpos < dsb->dsound->mixpos) ?
		dsb->dsound->buflen : 0) + dsb->primary_mixpos -
		dsb->dsound->mixpos;
	if (slen > mixlen) {
		/* the primary_done and still_behind checks above should have worked */
		FIXME("problem with advancement calculation (advlen=%ld > mixlen=%ld)\n", slen, mixlen);
		slen = 0;
	}
	return slen;
}

static DWORD DSOUND_MixToPrimary(IDirectSoundImpl *dsound, DWORD playpos, DWORD writepos, DWORD mixlen, BOOL recover)
{
	INT			i, len, maxlen = 0;
	IDirectSoundBufferImpl	*dsb;

	TRACE("(%ld,%ld,%ld,%d)\n", playpos, writepos, mixlen, recover);
	for (i = 0; i < dsound->nrofbuffers; i++) {
		dsb = dsound->buffers[i];

		if (dsb->buflen && dsb->state && !dsb->hwbuf) {
			TRACE("Checking %p, mixlen=%ld\n", dsb, mixlen);
			EnterCriticalSection(&(dsb->lock));
			if (dsb->state == STATE_STOPPING) {
				DSOUND_MixCancel(dsb, writepos, TRUE);
				dsb->state = STATE_STOPPED;
				DSOUND_CheckEvent(dsb, 0);
			} else {
				if ((dsb->state == STATE_STARTING) || recover) {
					dsb->primary_mixpos = writepos;
					dsb->cvolpan = dsb->volpan;
					dsb->need_remix = FALSE;
				}
				else if (dsb->need_remix) {
					DSOUND_MixCancel(dsb, writepos, TRUE);
					dsb->cvolpan = dsb->volpan;
					dsb->need_remix = FALSE;
				}
				len = DSOUND_MixOne(dsb, playpos, writepos, mixlen);
				if (dsb->state == STATE_STARTING)
					dsb->state = STATE_PLAYING;
				maxlen = (len > maxlen) ? len : maxlen;
			}
			LeaveCriticalSection(&(dsb->lock));
		}
	}

	return maxlen;
}

static void DSOUND_MixReset(IDirectSoundImpl *dsound, DWORD writepos)
{
	INT			i;
	IDirectSoundBufferImpl	*dsb;
	int nfiller;

	TRACE("(%ld)\n", writepos);

	/* the sound of silence */
	nfiller = dsound->pwfx->wBitsPerSample == 8 ? 128 : 0;

	/* reset all buffer mix positions */
	for (i = 0; i < dsound->nrofbuffers; i++) {
		dsb = dsound->buffers[i];

		if (dsb->buflen && dsb->state && !dsb->hwbuf) {
			TRACE("Resetting %p\n", dsb);
			EnterCriticalSection(&(dsb->lock));
			if (dsb->state == STATE_STOPPING) {
				dsb->state = STATE_STOPPED;
			}
			else if (dsb->state == STATE_STARTING) {
				/* nothing */
			} else {
				DSOUND_MixCancel(dsb, writepos, FALSE);
				dsb->cvolpan = dsb->volpan;
				dsb->need_remix = FALSE;
			}
			LeaveCriticalSection(&(dsb->lock));
		}
	}

	/* wipe out premixed data */
	if (dsound->mixpos < writepos) {
		memset(dsound->buffer + writepos, nfiller, dsound->buflen - writepos);
		memset(dsound->buffer, nfiller, dsound->mixpos);
	} else {
		memset(dsound->buffer + writepos, nfiller, dsound->mixpos - writepos);
	}

	/* reset primary mix position */
	dsound->mixpos = writepos;
}

static void DSOUND_CheckReset(IDirectSoundImpl *dsound, DWORD writepos)
{
	TRACE("(%p,%ld)\n",dsound,writepos);
	if (dsound->need_remix) {
		DSOUND_MixReset(dsound, writepos);
		dsound->need_remix = FALSE;
		/* maximize Half-Life performance */
		dsound->prebuf = ds_snd_queue_min;
		dsound->precount = 0;
	} else {
		dsound->precount++;
		if (dsound->precount >= 4) {
			if (dsound->prebuf < ds_snd_queue_max)
				dsound->prebuf++;
			dsound->precount = 0;
		}
	}
	TRACE("premix adjust: %d\n", dsound->prebuf);
}

void DSOUND_WaveQueue(IDirectSoundImpl *dsound, DWORD mixq)
{
	TRACE("(%p,%ld)\n",dsound,mixq);
	if (mixq + dsound->pwqueue > ds_hel_queue) mixq = ds_hel_queue - dsound->pwqueue;
	TRACE("queueing %ld buffers, starting at %d\n", mixq, dsound->pwwrite);
	for (; mixq; mixq--) {
		waveOutWrite(dsound->hwo, dsound->pwave[dsound->pwwrite], sizeof(WAVEHDR));
		dsound->pwwrite++;
		if (dsound->pwwrite >= DS_HEL_FRAGS) dsound->pwwrite = 0;
		dsound->pwqueue++;
	}
}

/* #define SYNC_CALLBACK */

void DSOUND_PerformMix(IDirectSoundImpl *dsound)
{
	int nfiller;
	BOOL forced;
	HRESULT hres;

	TRACE("(%p)\n", dsound);

	/* the sound of silence */
	nfiller = dsound->pwfx->wBitsPerSample == 8 ? 128 : 0;

	/* whether the primary is forced to play even without secondary buffers */
	forced = ((dsound->state == STATE_PLAYING) || (dsound->state == STATE_STARTING));

	if (dsound->priolevel != DSSCL_WRITEPRIMARY) {
		BOOL paused = ((dsound->state == STATE_STOPPED) || (dsound->state == STATE_STARTING));
		/* FIXME: document variables */
 		DWORD playpos, writepos, inq, maxq, frag;
 		if (dsound->hwbuf) {
			hres = IDsDriverBuffer_GetPosition(dsound->hwbuf, &playpos, &writepos);
			if (hres) {
			    WARN("IDsDriverBuffer_GetPosition failed\n");
			    return;
			}
			/* Well, we *could* do Just-In-Time mixing using the writepos,
			 * but that's a little bit ambitious and unnecessary... */
			/* rather add our safety margin to the writepos, if we're playing */
			if (!paused) {
				writepos += dsound->writelead;
				writepos %= dsound->buflen;
			} else writepos = playpos;
		} else {
 			playpos = dsound->pwplay * dsound->fraglen;
 			writepos = playpos;
 			if (!paused) {
	 			writepos += ds_hel_margin * dsound->fraglen;
 				writepos %= dsound->buflen;
	 		}
		}
		TRACE("primary playpos=%ld, writepos=%ld, clrpos=%ld, mixpos=%ld, buflen=%ld\n",
		      playpos,writepos,dsound->playpos,dsound->mixpos,dsound->buflen);
		assert(dsound->playpos < dsound->buflen);
		/* wipe out just-played sound data */
		if (playpos < dsound->playpos) {
			memset(dsound->buffer + dsound->playpos, nfiller, dsound->buflen - dsound->playpos);
			memset(dsound->buffer, nfiller, playpos);
		} else {
			memset(dsound->buffer + dsound->playpos, nfiller, playpos - dsound->playpos);
		}
		dsound->playpos = playpos;

		EnterCriticalSection(&(dsound->mixlock));

		/* reset mixing if necessary */
		DSOUND_CheckReset(dsound, writepos);

		/* check how much prebuffering is left */
		inq = dsound->mixpos;
		if (inq < writepos)
			inq += dsound->buflen;
		inq -= writepos;

		/* find the maximum we can prebuffer */
		if (!paused) {
			maxq = playpos;
			if (maxq < writepos)
				maxq += dsound->buflen;
			maxq -= writepos;
		} else maxq = dsound->buflen;

		/* clip maxq to dsound->prebuf */
		frag = dsound->prebuf * dsound->fraglen;
		if (maxq > frag) maxq = frag;

		/* check for consistency */
		if (inq > maxq) {
			/* the playback position must have passed our last
			 * mixed position, i.e. it's an underrun, or we have
			 * nothing more to play */
			TRACE("reached end of mixed data (inq=%ld, maxq=%ld)\n", inq, maxq);
			inq = 0;
			/* stop the playback now, to allow buffers to refill */
			if (dsound->state == STATE_PLAYING) {
				dsound->state = STATE_STARTING;
			}
			else if (dsound->state == STATE_STOPPING) {
				dsound->state = STATE_STOPPED;
			}
			else {
				/* how can we have an underrun if we aren't playing? */
				WARN("unexpected primary state (%ld)\n", dsound->state);
			}
#ifdef SYNC_CALLBACK
			/* DSOUND_callback may need this lock */
			LeaveCriticalSection(&(dsound->mixlock));
#endif
			if (DSOUND_PrimaryStop(dsound) != DS_OK)
				WARN("DSOUND_PrimaryStop failed\n");
#ifdef SYNC_CALLBACK
			EnterCriticalSection(&(dsound->mixlock));
#endif
			if (dsound->hwbuf) {
				/* the Stop is supposed to reset play position to beginning of buffer */
				/* unfortunately, OSS is not able to do so, so get current pointer */
				hres = IDsDriverBuffer_GetPosition(dsound->hwbuf, &playpos, NULL);
				if (hres) {
					LeaveCriticalSection(&(dsound->mixlock));
					WARN("IDsDriverBuffer_GetPosition failed\n");
					return;
				}
			} else {
	 			playpos = dsound->pwplay * dsound->fraglen;
			}
			writepos = playpos;
			dsound->playpos = playpos;
			dsound->mixpos = writepos;
			inq = 0;
			maxq = dsound->buflen;
			if (maxq > frag) maxq = frag;
			memset(dsound->buffer, nfiller, dsound->buflen);
			paused = TRUE;
		}

		/* do the mixing */
		frag = DSOUND_MixToPrimary(dsound, playpos, writepos, maxq, paused);
		if (forced) frag = maxq - inq;
		dsound->mixpos += frag;
		dsound->mixpos %= dsound->buflen;

		if (frag) {
			/* buffers have been filled, restart playback */
			if (dsound->state == STATE_STARTING) {
				dsound->state = STATE_PLAYING;
			}
			else if (dsound->state == STATE_STOPPED) {
				/* the dsound is supposed to play if there's something to play
				 * even if it is reported as stopped, so don't let this confuse you */
				dsound->state = STATE_STOPPING;
			}
			LeaveCriticalSection(&(dsound->mixlock));
			if (paused) {
				if (DSOUND_PrimaryPlay(dsound) != DS_OK)
					WARN("DSOUND_PrimaryPlay failed\n");
				else
					TRACE("starting playback\n");
			}
		}
		else
			LeaveCriticalSection(&(dsound->mixlock));
	} else {
		/* in the DSSCL_WRITEPRIMARY mode, the app is totally in charge... */
		if (dsound->state == STATE_STARTING) {
			if (DSOUND_PrimaryPlay(dsound) != DS_OK)
				WARN("DSOUND_PrimaryPlay failed\n");
			else
				dsound->state = STATE_PLAYING;
		}
		else if (dsound->state == STATE_STOPPING) {
			if (DSOUND_PrimaryStop(dsound) != DS_OK)
				WARN("DSOUND_PrimaryStop failed\n");
			else
				dsound->state = STATE_STOPPED;
		}
	}
}

void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        IDirectSoundImpl* This = (IDirectSoundImpl*)dwUser;
	DWORD start_time =  GetTickCount();
        DWORD end_time;
	TRACE("(%d,%d,0x%lx,0x%lx,0x%lx)\n",timerID,msg,dwUser,dw1,dw2);
        TRACE("entering at %ld\n", start_time);

	if (dsound != This) {
		ERR("dsound died without killing us?\n");
		timeKillEvent(timerID);
		timeEndPeriod(DS_TIME_RES);
		return;
	}

	RtlAcquireResourceShared(&(This->buffer_list_lock), TRUE);

	if (This->ref)
		DSOUND_PerformMix(This);

	RtlReleaseResource(&(This->buffer_list_lock));

	end_time = GetTickCount();
	TRACE("completed processing at %ld, duration = %ld\n", end_time, end_time - start_time);
}

void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        IDirectSoundImpl* This = (IDirectSoundImpl*)dwUser;
	TRACE("(%p,%x,%lx,%lx,%lx)\n",hwo,msg,dwUser,dw1,dw2);
	TRACE("entering at %ld, msg=%08x(%s)\n", GetTickCount(), msg, 
		msg==MM_WOM_DONE ? "MM_WOM_DONE" : msg==MM_WOM_CLOSE ? "MM_WOM_CLOSE" : 
		msg==MM_WOM_OPEN ? "MM_WOM_OPEN" : "UNKNOWN");
	if (msg == MM_WOM_DONE) {
		DWORD inq, mixq, fraglen, buflen, pwplay, playpos, mixpos;
		if (This->pwqueue == (DWORD)-1) {
			TRACE("completed due to reset\n");
			return;
		}
/* it could be a bad idea to enter critical section here... if there's lock contention,
 * the resulting scheduling delays might obstruct the winmm player thread */
#ifdef SYNC_CALLBACK
		EnterCriticalSection(&(This->mixlock));
#endif
		/* retrieve current values */
		fraglen = This->fraglen;
		buflen = This->buflen;
		pwplay = This->pwplay;
		playpos = pwplay * fraglen;
		mixpos = This->mixpos;
		/* check remaining mixed data */
		inq = ((mixpos < playpos) ? buflen : 0) + mixpos - playpos;
		mixq = inq / fraglen;
		if ((inq - (mixq * fraglen)) > 0) mixq++;
		/* complete the playing buffer */
		TRACE("done playing primary pos=%ld\n", playpos);
		pwplay++;
		if (pwplay >= DS_HEL_FRAGS) pwplay = 0;
		/* write new values */
		This->pwplay = pwplay;
		This->pwqueue--;
		/* queue new buffer if we have data for it */
		if (inq>1) DSOUND_WaveQueue(This, inq-1);
#ifdef SYNC_CALLBACK
		LeaveCriticalSection(&(This->mixlock));
#endif
	}
	TRACE("completed\n");
}
