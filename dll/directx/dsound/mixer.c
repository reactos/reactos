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

#include <assert.h>
#include <stdarg.h>
#include <math.h>	/* Insomnia - pow() function */

#define NONAMELESSSTRUCT
#define NONAMELESSUNION
#include "windef.h"
#include "winbase.h"
#include "mmsystem.h"
#include "winreg.h"
#include "winternl.h"
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
	TRACE("(%p)\n",dsb);

	/* calculate the 10ms write lead */
	dsb->writelead = (dsb->freq / 100) * dsb->pwfx->nBlockAlign;
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
	DirectSoundDevice * device = dsb->dsound->device;
        INT fl,fr;

        if (dsb->pwfx->wBitsPerSample == 8)  {
                if (device->pwfx->wBitsPerSample == 8 &&
                    device->pwfx->nChannels == dsb->pwfx->nChannels) {
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

        if (device->pwfx->nChannels == 2) {
                if (device->pwfx->wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        *(obuf + 1) = cvtS16toU8(fr);
                        return;
                }
                if (device->pwfx->wBitsPerSample == 16) {
                        *((INT16 *)obuf) = fl;
                        *(((INT16 *)obuf) + 1) = fr;
                        return;
                }
        }
        if (device->pwfx->nChannels == 1) {
                fl = (fl + fr) >> 1;
                if (device->pwfx->wBitsPerSample == 8) {
                        *obuf = cvtS16toU8(fl);
                        return;
                }
                if (device->pwfx->wBitsPerSample == 16) {
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
	INT	oAdvance = dsb->dsound->device->pwfx->nBlockAlign;

	ibp = dsb->buffer->memory + dsb->buf_mixpos;
	obp = buf;

	TRACE("(%p, %p, %p), buf_mixpos=%ld\n", dsb, ibp, obp, dsb->buf_mixpos);
	/* Check for the best case */
	if ((dsb->freq == dsb->dsound->device->pwfx->nSamplesPerSec) &&
	    (dsb->pwfx->wBitsPerSample == dsb->dsound->device->pwfx->wBitsPerSample) &&
	    (dsb->pwfx->nChannels == dsb->dsound->device->pwfx->nChannels)) {
	        INT bytesleft = dsb->buflen - dsb->buf_mixpos;
		TRACE("(%p) Best case\n", dsb);
	    	if (len <= bytesleft )
			CopyMemory(obp, ibp, len);
		else { /* wrap */
			CopyMemory(obp, ibp, bytesleft);
			CopyMemory(obp + bytesleft, dsb->buffer->memory, len - bytesleft);
		}
		return len;
	}

	/* Check for same sample rate */
	if (dsb->freq == dsb->dsound->device->pwfx->nSamplesPerSec) {
		TRACE("(%p) Same sample rate %ld = primary %ld\n", dsb,
			dsb->freq, dsb->dsound->device->pwfx->nSamplesPerSec);
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
	   dsb, dsb->freq, dsb->dsound->device->pwfx->nSamplesPerSec); */

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

	switch (dsb->dsound->device->pwfx->wBitsPerSample) {
	case 8:
		/* 8-bit WAV is unsigned, but we need to operate */
		/* on signed data for this to work properly */
		switch (dsb->dsound->device->pwfx->nChannels) {
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
			FIXME("doesn't support %d channels\n", dsb->dsound->device->pwfx->nChannels);
			break;
		}
		break;
	case 16:
		/* 16-bit WAV is signed -- much better */
		switch (dsb->dsound->device->pwfx->nChannels) {
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
			FIXME("doesn't support %d channels\n", dsb->dsound->device->pwfx->nChannels);
			break;
		}
		break;
	default:
		FIXME("doesn't support %d bit samples\n", dsb->dsound->device->pwfx->wBitsPerSample);
		break;
	}
}

static LPBYTE DSOUND_tmpbuffer(DirectSoundDevice *device, DWORD len)
{
    TRACE("(%p,%ld)\n", device, len);

    if (len > device->tmp_buffer_len) {
        if (device->tmp_buffer)
            device->tmp_buffer = HeapReAlloc(GetProcessHeap(), 0, device->tmp_buffer, len);
        else
            device->tmp_buffer = HeapAlloc(GetProcessHeap(), 0, len);

        device->tmp_buffer_len = len;
    }

    return device->tmp_buffer;
}

static DWORD DSOUND_MixInBuffer(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD fraglen)
{
	INT	i, len, ilen, field, todo;
	BYTE	*buf, *ibuf;

	TRACE("(%p,%ld,%ld)\n",dsb,writepos,fraglen);

	len = fraglen;
	if (!(dsb->playflags & DSBPLAY_LOOPING)) {
		int secondary_remainder = dsb->buflen - dsb->buf_mixpos;
		int adjusted_remainder = MulDiv(dsb->dsound->device->pwfx->nAvgBytesPerSec, secondary_remainder, dsb->nAvgBytesPerSec);
		assert(adjusted_remainder >= 0);
		TRACE("secondary_remainder = %d, adjusted_remainder = %d, len = %d\n", secondary_remainder, adjusted_remainder, len);
		if (adjusted_remainder < len) {
			TRACE("clipping len to remainder of secondary buffer\n");
			len = adjusted_remainder;
		}
		if (len == 0)
			return 0;
	}

	if (len % dsb->dsound->device->pwfx->nBlockAlign) {
		INT nBlockAlign = dsb->dsound->device->pwfx->nBlockAlign;
		ERR("length not a multiple of block size, len = %d, block size = %d\n", len, nBlockAlign);
		len = (len / nBlockAlign) * nBlockAlign;	/* data alignment */
	}

	if ((buf = ibuf = DSOUND_tmpbuffer(dsb->dsound->device, len)) == NULL)
		return 0;

	TRACE("MixInBuffer (%p) len = %d, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		DSOUND_MixerVol(dsb, ibuf, len);

	if (dsb->dsound->device->pwfx->wBitsPerSample == 8) {
		BYTE	*obuf = dsb->dsound->device->buffer + writepos;

		if ((writepos + len) <= dsb->dsound->device->buflen)
			todo = len;
		else
			todo = dsb->dsound->device->buflen - writepos;

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
			obuf = dsb->dsound->device->buffer;

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
		obufs = (INT16 *)(dsb->dsound->device->buffer + writepos);

		if ((writepos + len) <= dsb->dsound->device->buflen)
			todo = len / 2;
		else
			todo = (dsb->dsound->device->buflen - writepos) / 2;

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
			obufs = (INT16 *)dsb->dsound->device->buffer;

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
		} else if (dsb->buf_mixpos > dsb->buflen) {
			ERR("Mixpos (%lu) past buflen (%lu), capping...\n", dsb->buf_mixpos, dsb->buflen);
			dsb->buf_mixpos = dsb->buflen;
		}
	}

	return len;
}

static void DSOUND_PhaseCancel(IDirectSoundBufferImpl *dsb, DWORD writepos, DWORD len)
{
	INT     ilen, field;
	UINT    i, todo;
	BYTE	*buf, *ibuf;

	TRACE("(%p,%ld,%ld)\n",dsb,writepos,len);

	if (len % dsb->dsound->device->pwfx->nBlockAlign) {
		INT nBlockAlign = dsb->dsound->device->pwfx->nBlockAlign;
		ERR("length not a multiple of block size, len = %ld, block size = %d\n", len, nBlockAlign);
		len = (len / nBlockAlign) * nBlockAlign;	/* data alignment */
	}

	if ((buf = ibuf = DSOUND_tmpbuffer(dsb->dsound->device, len)) == NULL)
		return;

	TRACE("PhaseCancel (%p) len = %ld, dest = %ld\n", dsb, len, writepos);

	ilen = DSOUND_MixerNorm(dsb, ibuf, len);
	if ((dsb->dsbd.dwFlags & DSBCAPS_CTRLPAN) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRLVOLUME) ||
	    (dsb->dsbd.dwFlags & DSBCAPS_CTRL3D))
		DSOUND_MixerVol(dsb, ibuf, len);

	/* subtract instead of add, to phase out premixed data */
	if (dsb->dsound->device->pwfx->wBitsPerSample == 8) {
		BYTE	*obuf = dsb->dsound->device->buffer + writepos;

		if ((writepos + len) <= dsb->dsound->device->buflen)
			todo = len;
		else
			todo = dsb->dsound->device->buflen - writepos;

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
			obuf = dsb->dsound->device->buffer;

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
		obufs = (INT16 *)(dsb->dsound->device->buffer + writepos);

		if ((writepos + len) <= dsb->dsound->device->buflen)
			todo = len / 2;
		else
			todo = (dsb->dsound->device->buflen - writepos) / 2;

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
			obufs = (INT16 *)dsb->dsound->device->buffer;

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
	INT	oAdvance = dsb->dsound->device->pwfx->nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? dsb->dsound->device->buflen : 0) +
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
		nlen *= dsb->dsound->device->pwfx->nBlockAlign;
		writepos =
			((dsb->primary_mixpos < nlen) ? dsb->dsound->device->buflen : 0) +
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
	INT	oAdvance = dsb->dsound->device->pwfx->nBlockAlign;
	/* determine amount of premixed data to cancel */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
#endif

	WARN("(%p, %ld), buf_mixpos=%ld\n", dsb, buf_writepos, dsb->buf_mixpos);
	/* since this is not implemented yet, just cancel *ALL* prebuffering for now
	 * (which is faster anyway when there's only a single secondary buffer) */
	dsb->dsound->device->need_remix = TRUE;
}

void DSOUND_ForceRemix(IDirectSoundBufferImpl *dsb)
{
	TRACE("(%p)\n",dsb);
	EnterCriticalSection(&dsb->lock);
	if (dsb->state == STATE_PLAYING)
		dsb->dsound->device->need_remix = TRUE;
	LeaveCriticalSection(&dsb->lock);
}

static DWORD DSOUND_MixOne(IDirectSoundBufferImpl *dsb, DWORD playpos, DWORD writepos, DWORD mixlen)
{
	DWORD len, slen;
	/* determine this buffer's write position */
	DWORD buf_writepos = DSOUND_CalcPlayPosition(dsb, writepos, writepos);
	/* determine how much already-mixed data exists */
	DWORD buf_done =
		((dsb->buf_mixpos < buf_writepos) ? dsb->buflen : 0) +
		dsb->buf_mixpos - buf_writepos;
	DWORD primary_done =
		((dsb->primary_mixpos < writepos) ? dsb->dsound->device->buflen : 0) +
		dsb->primary_mixpos - writepos;
	DWORD adv_done =
		((dsb->dsound->device->mixpos < writepos) ? dsb->dsound->device->buflen : 0) +
		dsb->dsound->device->mixpos - writepos;
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
				              dsb->dsound->device->pwfx->nBlockAlign;
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

	if (len < dsb->dsound->device->fraglen) {
		/* smaller than a fragment, wait until it gets larger
		 * before we take the mixing overhead */
		TRACE("mixlen not worth it, deferring mixing\n");
		still_behind = 1;
		goto post_mix;
	}

	/* ok, we know how much to mix, let's go */
	still_behind = (adv_done > primary_done);
	while (len) {
		slen = dsb->dsound->device->buflen - dsb->primary_mixpos;
		if (slen > len) slen = len;
		slen = DSOUND_MixInBuffer(dsb, dsb->primary_mixpos, slen);

		if ((dsb->primary_mixpos < dsb->dsound->device->mixpos) &&
		    (dsb->primary_mixpos + slen >= dsb->dsound->device->mixpos))
			still_behind = FALSE;

		dsb->primary_mixpos += slen; len -= slen;
		dsb->primary_mixpos %= dsb->dsound->device->buflen;

		if ((dsb->state == STATE_STOPPED) || !slen) break;
	}
	TRACE("new primary_mixpos=%ld, primary_advbase=%ld\n", dsb->primary_mixpos, dsb->dsound->device->mixpos);
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
		dsb->need_remix = FALSE;
		DSOUND_CheckEvent(dsb, buf_left);
	}

	/* return how far we think the primary buffer can
	 * advance its underrun detector...*/
	if (still_behind) return 0;
	if ((mixlen - len) < primary_done) return 0;
	slen = ((dsb->primary_mixpos < dsb->dsound->device->mixpos) ?
		dsb->dsound->device->buflen : 0) + dsb->primary_mixpos -
		dsb->dsound->device->mixpos;
	if (slen > mixlen) {
		/* the primary_done and still_behind checks above should have worked */
		FIXME("problem with advancement calculation (advlen=%ld > mixlen=%ld)\n", slen, mixlen);
		slen = 0;
	}
	return slen;
}

static DWORD DSOUND_MixToPrimary(DirectSoundDevice *device, DWORD playpos, DWORD writepos, DWORD mixlen, BOOL recover)
{
	INT			i, len, maxlen = 0;
	IDirectSoundBufferImpl	*dsb;

	TRACE("(%ld,%ld,%ld,%d)\n", playpos, writepos, mixlen, recover);
	for (i = 0; i < device->nrofbuffers; i++) {
		dsb = device->buffers[i];

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

static void DSOUND_MixReset(DirectSoundDevice *device, DWORD writepos)
{
	INT			i;
	IDirectSoundBufferImpl	*dsb;
	int nfiller;

	TRACE("(%p,%ld)\n", device, writepos);

	/* the sound of silence */
	nfiller = device->pwfx->wBitsPerSample == 8 ? 128 : 0;

	/* reset all buffer mix positions */
	for (i = 0; i < device->nrofbuffers; i++) {
		dsb = device->buffers[i];

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
	if (device->mixpos < writepos) {
		FillMemory(device->buffer + writepos, device->buflen - writepos, nfiller);
		FillMemory(device->buffer, device->mixpos, nfiller);
	} else {
		FillMemory(device->buffer + writepos, device->mixpos - writepos, nfiller);
	}

	/* reset primary mix position */
	device->mixpos = writepos;
}

static void DSOUND_CheckReset(DirectSoundDevice *device, DWORD writepos)
{
	TRACE("(%p,%ld)\n",device,writepos);
	if (device->need_remix) {
		DSOUND_MixReset(device, writepos);
		device->need_remix = FALSE;
		/* maximize Half-Life performance */
		device->prebuf = ds_snd_queue_min;
		device->precount = 0;
	} else {
		device->precount++;
		if (device->precount >= 4) {
			if (device->prebuf < ds_snd_queue_max)
				device->prebuf++;
			device->precount = 0;
		}
	}
	TRACE("premix adjust: %d\n", device->prebuf);
}

void DSOUND_WaveQueue(DirectSoundDevice *device, DWORD mixq)
{
	TRACE("(%p,%ld)\n", device, mixq);
	if (mixq + device->pwqueue > ds_hel_queue) mixq = ds_hel_queue - device->pwqueue;
	TRACE("queueing %ld buffers, starting at %d\n", mixq, device->pwwrite);
	for (; mixq; mixq--) {
		waveOutWrite(device->hwo, device->pwave[device->pwwrite], sizeof(WAVEHDR));
		device->pwwrite++;
		if (device->pwwrite >= DS_HEL_FRAGS) device->pwwrite = 0;
		device->pwqueue++;
	}
}

/* #define SYNC_CALLBACK */

void DSOUND_PerformMix(DirectSoundDevice *device)
{
	int nfiller;
	BOOL forced;
	HRESULT hres;

	TRACE("(%p)\n", device);

	/* the sound of silence */
	nfiller = device->pwfx->wBitsPerSample == 8 ? 128 : 0;

	/* whether the primary is forced to play even without secondary buffers */
	forced = ((device->state == STATE_PLAYING) || (device->state == STATE_STARTING));

	if (device->priolevel != DSSCL_WRITEPRIMARY) {
		BOOL paused = ((device->state == STATE_STOPPED) || (device->state == STATE_STARTING));
		/* FIXME: document variables */
 		DWORD playpos, writepos, inq, maxq, frag;
 		if (device->hwbuf) {
			hres = IDsDriverBuffer_GetPosition(device->hwbuf, &playpos, &writepos);
			if (hres) {
			    WARN("IDsDriverBuffer_GetPosition failed\n");
			    return;
			}
			/* Well, we *could* do Just-In-Time mixing using the writepos,
			 * but that's a little bit ambitious and unnecessary... */
			/* rather add our safety margin to the writepos, if we're playing */
			if (!paused) {
				writepos += device->writelead;
				writepos %= device->buflen;
			} else writepos = playpos;
		} else {
 			playpos = device->pwplay * device->fraglen;
 			writepos = playpos;
 			if (!paused) {
	 			writepos += ds_hel_margin * device->fraglen;
 				writepos %= device->buflen;
	 		}
		}
		TRACE("primary playpos=%ld, writepos=%ld, clrpos=%ld, mixpos=%ld, buflen=%ld\n",
		      playpos,writepos,device->playpos,device->mixpos,device->buflen);
		assert(device->playpos < device->buflen);
		/* wipe out just-played sound data */
		if (playpos < device->playpos) {
			FillMemory(device->buffer + device->playpos, device->buflen - device->playpos, nfiller);
			FillMemory(device->buffer, playpos, nfiller);
		} else {
			FillMemory(device->buffer + device->playpos, playpos - device->playpos, nfiller);
		}
		device->playpos = playpos;

		EnterCriticalSection(&(device->mixlock));

		/* reset mixing if necessary */
		DSOUND_CheckReset(device, writepos);

		/* check how much prebuffering is left */
		inq = device->mixpos;
		if (inq < writepos)
			inq += device->buflen;
		inq -= writepos;

		/* find the maximum we can prebuffer */
		if (!paused) {
			maxq = playpos;
			if (maxq < writepos)
				maxq += device->buflen;
			maxq -= writepos;
		} else maxq = device->buflen;

		/* clip maxq to device->prebuf */
		frag = device->prebuf * device->fraglen;
		if (maxq > frag) maxq = frag;

		/* check for consistency */
		if (inq > maxq) {
			/* the playback position must have passed our last
			 * mixed position, i.e. it's an underrun, or we have
			 * nothing more to play */
			TRACE("reached end of mixed data (inq=%ld, maxq=%ld)\n", inq, maxq);
			inq = 0;
			/* stop the playback now, to allow buffers to refill */
			if (device->state == STATE_PLAYING) {
				device->state = STATE_STARTING;
			}
			else if (device->state == STATE_STOPPING) {
				device->state = STATE_STOPPED;
			}
			else {
				/* how can we have an underrun if we aren't playing? */
				WARN("unexpected primary state (%ld)\n", device->state);
			}
#ifdef SYNC_CALLBACK
			/* DSOUND_callback may need this lock */
			LeaveCriticalSection(&(device->mixlock));
#endif
			if (DSOUND_PrimaryStop(device) != DS_OK)
				WARN("DSOUND_PrimaryStop failed\n");
#ifdef SYNC_CALLBACK
			EnterCriticalSection(&(device->mixlock));
#endif
			if (device->hwbuf) {
				/* the Stop is supposed to reset play position to beginning of buffer */
				/* unfortunately, OSS is not able to do so, so get current pointer */
				hres = IDsDriverBuffer_GetPosition(device->hwbuf, &playpos, NULL);
				if (hres) {
					LeaveCriticalSection(&(device->mixlock));
					WARN("IDsDriverBuffer_GetPosition failed\n");
					return;
				}
			} else {
	 			playpos = device->pwplay * device->fraglen;
			}
			writepos = playpos;
			device->playpos = playpos;
			device->mixpos = writepos;
			inq = 0;
			maxq = device->buflen;
			if (maxq > frag) maxq = frag;
			FillMemory(device->buffer, device->buflen, nfiller);
			paused = TRUE;
		}

		/* do the mixing */
		frag = DSOUND_MixToPrimary(device, playpos, writepos, maxq, paused);
		if (forced) frag = maxq - inq;
		device->mixpos += frag;
		device->mixpos %= device->buflen;

		if (frag) {
			/* buffers have been filled, restart playback */
			if (device->state == STATE_STARTING) {
				device->state = STATE_PLAYING;
			}
			else if (device->state == STATE_STOPPED) {
				/* the dsound is supposed to play if there's something to play
				 * even if it is reported as stopped, so don't let this confuse you */
				device->state = STATE_STOPPING;
			}
			LeaveCriticalSection(&(device->mixlock));
			if (paused) {
				if (DSOUND_PrimaryPlay(device) != DS_OK)
					WARN("DSOUND_PrimaryPlay failed\n");
				else
					TRACE("starting playback\n");
			}
		}
		else
			LeaveCriticalSection(&(device->mixlock));
	} else {
		/* in the DSSCL_WRITEPRIMARY mode, the app is totally in charge... */
		if (device->state == STATE_STARTING) {
			if (DSOUND_PrimaryPlay(device) != DS_OK)
				WARN("DSOUND_PrimaryPlay failed\n");
			else
				device->state = STATE_PLAYING;
		}
		else if (device->state == STATE_STOPPING) {
			if (DSOUND_PrimaryStop(device) != DS_OK)
				WARN("DSOUND_PrimaryStop failed\n");
			else
				device->state = STATE_STOPPED;
		}
	}
}

void CALLBACK DSOUND_timer(UINT timerID, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        DirectSoundDevice * device = (DirectSoundDevice*)dwUser;
	DWORD start_time =  GetTickCount();
        DWORD end_time;
	TRACE("(%d,%d,0x%lx,0x%lx,0x%lx)\n",timerID,msg,dwUser,dw1,dw2);
        TRACE("entering at %ld\n", start_time);

	if (DSOUND_renderer[device->drvdesc.dnDevNode] != device) {
		ERR("dsound died without killing us?\n");
		timeKillEvent(timerID);
		timeEndPeriod(DS_TIME_RES);
		return;
	}

	RtlAcquireResourceShared(&(device->buffer_list_lock), TRUE);

	if (device->ref)
		DSOUND_PerformMix(device);

	RtlReleaseResource(&(device->buffer_list_lock));

	end_time = GetTickCount();
	TRACE("completed processing at %ld, duration = %ld\n", end_time, end_time - start_time);
}

void CALLBACK DSOUND_callback(HWAVEOUT hwo, UINT msg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
        DirectSoundDevice * device = (DirectSoundDevice*)dwUser;
	TRACE("(%p,%x,%lx,%lx,%lx)\n",hwo,msg,dwUser,dw1,dw2);
	TRACE("entering at %ld, msg=%08x(%s)\n", GetTickCount(), msg, 
		msg==MM_WOM_DONE ? "MM_WOM_DONE" : msg==MM_WOM_CLOSE ? "MM_WOM_CLOSE" : 
		msg==MM_WOM_OPEN ? "MM_WOM_OPEN" : "UNKNOWN");
	if (msg == MM_WOM_DONE) {
		DWORD inq, mixq, fraglen, buflen, pwplay, playpos, mixpos;
		if (device->pwqueue == (DWORD)-1) {
			TRACE("completed due to reset\n");
			return;
		}
/* it could be a bad idea to enter critical section here... if there's lock contention,
 * the resulting scheduling delays might obstruct the winmm player thread */
#ifdef SYNC_CALLBACK
		EnterCriticalSection(&(device->mixlock));
#endif
		/* retrieve current values */
		fraglen = device->fraglen;
		buflen = device->buflen;
		pwplay = device->pwplay;
		playpos = pwplay * fraglen;
		mixpos = device->mixpos;
		/* check remaining mixed data */
		inq = ((mixpos < playpos) ? buflen : 0) + mixpos - playpos;
		mixq = inq / fraglen;
		if ((inq - (mixq * fraglen)) > 0) mixq++;
		/* complete the playing buffer */
		TRACE("done playing primary pos=%ld\n", playpos);
		pwplay++;
		if (pwplay >= DS_HEL_FRAGS) pwplay = 0;
		/* write new values */
		device->pwplay = pwplay;
		device->pwqueue--;
		/* queue new buffer if we have data for it */
		if (inq>1) DSOUND_WaveQueue(device, inq-1);
#ifdef SYNC_CALLBACK
		LeaveCriticalSection(&(device->mixlock));
#endif
	}
	TRACE("completed\n");
}
