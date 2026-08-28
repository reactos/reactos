/*
 * Sample MIDI Wine Driver for Linux
 *
 * Copyright 1994 Martin Ayotte
 * Copyright 1999 Eric Pouech
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

/* TODO:
 *      + implement it correctly
 *      + finish asynchronous commands
 *      + better implement non waiting command (without the MCI_WAIT flag).
 */

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wingdi.h"
#include "winuser.h"
#include "wownt32.h"
#include "mmddk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mcimidi);

#define MIDI_NOTEOFF             0x80
#define MIDI_NOTEON              0x90

typedef struct {
    DWORD		dwFirst;		/* offset in file of track */
    DWORD		dwLast;			/* number of bytes in file of track */
    DWORD		dwIndex;		/* current index in file (dwFirst <= dwIndex < dwLast) */
    DWORD		dwLength;		/* number of pulses in this track */
    DWORD		dwEventPulse;		/* current pulse # (event) pointed by dwIndex */
    DWORD		dwEventData;		/* current data    (event) pointed by dwIndex */
    WORD		wEventLength;		/* current length  (event) pointed by dwIndex */
    WORD		wStatus : 1,		/* 1 : playing, 0 : done */
	                wTrackNr : 7,
	                wLastCommand : 8;	/* last MIDI command on track */
} MCI_MIDITRACK;

typedef struct tagWINE_MCIMIDI {
    UINT		wDevID;			/* the MCI one */
    HMIDI		hMidi;
    int			nUseCount;          	/* Incremented for each shared open          */
    HANDLE 		hCallback;         	/* Callback handle for pending notification  */
    HANDLE		hThread;		/* Player thread                             */
    HMMIO		hFile;	            	/* mmio file handle open as Element          */
    LPWSTR		lpstrElementName;       /* Name of file (if any)                     */
    LPWSTR		lpstrCopyright;
    LPWSTR		lpstrName;
    WORD		wPort;			/* the WINMM device unit */
    WORD		dwStatus;		/* one from MCI_MODE_xxxx */
    DWORD		dwMciTimeFormat;	/* One of the supported MCI_FORMAT_xxxx */
    WORD		wFormat;		/* Format of MIDI hFile (0, 1 or 2) */
    WORD		nTracks;		/* Number of tracks in hFile */
    WORD		nDivision;		/* Number of division in hFile PPQN or SMPTE */
    WORD		wStartedPlaying;
    DWORD		dwTempo;		/* Tempo (# of 1/4 note per second */
    MCI_MIDITRACK*     	tracks;			/* Content of each track */
    DWORD		dwPulse;
    DWORD		dwPositionMS;
    DWORD		dwEndMS;
    DWORD		dwStartTicks;
} WINE_MCIMIDI;

/*======================================================================*
 *                  	    MCI MIDI implementation			*
 *======================================================================*/

static DWORD mmr2mci(DWORD ret)
{
    switch (ret) {
    case MMSYSERR_ALLOCATED:
	return			MCIERR_SEQ_PORT_INUSE;
    case MMSYSERR_NOMEM:
	return			MCIERR_OUT_OF_MEMORY;
    case MMSYSERR_BADDEVICEID:	/* wine*.drv disabled */
      return			MCIERR_SEQ_PORT_NONEXISTENT;
    case MIDIERR_INVALIDSETUP:	/* from midimap.dll without snd-seq module  */
	return			MCIERR_SEQ_PORT_MAPNODEVICE;
    default:
	return ret;
    }
}

static DWORD MIDI_mciResume(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/**************************************************************************
 * 				MIDI_drvOpen			[internal]
 */
static	DWORD	MIDI_drvOpen(LPCWSTR str, LPMCI_OPEN_DRIVER_PARMSW modp)
{
    WINE_MCIMIDI*	wmm;

    if (!modp) return 0xFFFFFFFF;

    wmm = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIMIDI));

    if (!wmm)
	return 0;

    wmm->wDevID = modp->wDeviceID;
    mciSetDriverData(wmm->wDevID, (DWORD_PTR)wmm);
    modp->wCustomCommandTable = MCI_NO_COMMAND_TABLE;
    modp->wType = MCI_DEVTYPE_SEQUENCER;
    return modp->wDeviceID;
}

/**************************************************************************
 * 				MCIMIDI_drvClose		[internal]
 */
static	DWORD	MIDI_drvClose(DWORD dwDevID)
{
    WINE_MCIMIDI*  wmm = (WINE_MCIMIDI*)mciGetDriverData(dwDevID);

    if (wmm) {
	mciSetDriverData(dwDevID, 0);
	HeapFree(GetProcessHeap(), 0, wmm);
	return 1;
    }
    return (dwDevID == 0xFFFFFFFF) ? 1 : 0;
}

/**************************************************************************
 * 				MIDI_mciGetOpenDev		[internal]
 */
static WINE_MCIMIDI*  MIDI_mciGetOpenDev(MCIDEVICEID wDevID, UINT wMsg)
{
    WINE_MCIMIDI*	wmm = (WINE_MCIMIDI*)mciGetDriverData(wDevID);

    if (wmm == NULL || ((wmm->nUseCount == 0) ^ (wMsg == MCI_OPEN_DRIVER))) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wmm;
}

/**************************************************************************
 *				MIDI_mciNotify			[internal]
 *
 * Notifications in MCI work like a 1-element queue.
 * Each new notification request supersedes the previous one.
 * This affects Play and Record; other commands are immediate.
 */
static void MIDI_mciNotify(DWORD_PTR hWndCallBack, WINE_MCIMIDI* wmm, UINT wStatus)
{
    /* We simply save one parameter by not passing the wDevID local
     * to the command.  They are the same (via mciGetDriverData).
     */
    MCIDEVICEID wDevID = wmm->wDevID;
    HANDLE old = InterlockedExchangePointer(&wmm->hCallback, NULL);
    if (old) mciDriverNotify(old, wDevID, MCI_NOTIFY_SUPERSEDED);
    mciDriverNotify(HWND_32(LOWORD(hWndCallBack)), wDevID, wStatus);
}

/**************************************************************************
 * 				MIDI_mciReadByte		[internal]
 */
static DWORD MIDI_mciReadByte(WINE_MCIMIDI* wmm, BYTE *lpbyt)
{
    DWORD	ret = 0;

    if (mmioRead(wmm->hFile, (HPSTR)lpbyt, sizeof(BYTE)) != sizeof(BYTE)) {
	WARN("Error reading wmm=%p\n", wmm);
	ret = MCIERR_INVALID_FILE;
    }

    return ret;
}

/**************************************************************************
 * 				MIDI_mciReadWord		[internal]
 */
static DWORD MIDI_mciReadWord(WINE_MCIMIDI* wmm, LPWORD lpw)
{
    BYTE	hibyte, lobyte;
    DWORD	ret = MCIERR_INVALID_FILE;

    if (MIDI_mciReadByte(wmm, &hibyte) == 0 &&
	MIDI_mciReadByte(wmm, &lobyte) == 0) {
	*lpw = ((WORD)hibyte << 8) + lobyte;
	ret = 0;
    }
    return ret;
}

/**************************************************************************
 * 				MIDI_mciReadLong		[internal]
 */
static DWORD MIDI_mciReadLong(WINE_MCIMIDI* wmm, LPDWORD lpdw)
{
    WORD	hiword, loword;
    DWORD	ret = MCIERR_INVALID_FILE;

    if (MIDI_mciReadWord(wmm, &hiword) == 0 &&
	MIDI_mciReadWord(wmm, &loword) == 0) {
	*lpdw = MAKELONG(loword, hiword);
	ret = 0;
    }
    return ret;
}

/**************************************************************************
 *  				MIDI_mciReadVaryLen		[internal]
 */
static WORD MIDI_mciReadVaryLen(WINE_MCIMIDI* wmm, LPDWORD lpdw)
{
    BYTE	byte;
    DWORD	value = 0;
    WORD	len = 0;

    do {
	if (MIDI_mciReadByte(wmm, &byte) != 0) {
            *lpdw = 0;
	    return 0;
	}
	value = (value << 7) + (byte & 0x7F);
	len++;
    } while (byte & 0x80);
    *lpdw = value;
    return len;
}

/**************************************************************************
 * 				MIDI_mciReadNextEvent		[internal]
 */
static DWORD	MIDI_mciReadNextEvent(WINE_MCIMIDI* wmm, MCI_MIDITRACK* mmt)
{
    BYTE	b1, b2 = 0, b3;
    WORD	hw = 0;
    DWORD	evtPulse;
    DWORD	evtLength;
    DWORD	tmp;

    if (mmioSeek(wmm->hFile, mmt->dwIndex, SEEK_SET) != mmt->dwIndex) {
	WARN("Can't seek at %08lX\n", mmt->dwIndex);
	return MCIERR_INVALID_FILE;
    }
    evtLength = MIDI_mciReadVaryLen(wmm, &evtPulse) + 1;	/* > 0 */
    MIDI_mciReadByte(wmm, &b1);
    switch (b1) {
    case 0xF0:
    case 0xF7:
	evtLength += MIDI_mciReadVaryLen(wmm, &tmp);
	evtLength += tmp;
	break;
    case 0xFF:
	MIDI_mciReadByte(wmm, &b2);	evtLength++;

	evtLength += MIDI_mciReadVaryLen(wmm, &tmp);
	if (evtLength >= 0x10000u) {
	    /* this limitation shouldn't be a problem */
	    WARN("Ouch !! Implementation limitation to 64k bytes for a MIDI event is overflowed\n");
	    hw = 0xFFFF;
	} else {
	    hw = LOWORD(evtLength);
	}
	evtLength += tmp;
	break;
    default:
	if (b1 & 0x80) { /* use running status ? */
	    mmt->wLastCommand = b1;
	    MIDI_mciReadByte(wmm, &b2);	evtLength++;
	} else {
	    b2 = b1;
	    b1 = mmt->wLastCommand;
	}
	switch ((b1 >> 4) & 0x07) {
	case 0:	case 1:	case 2: case 3: case 6:
	    MIDI_mciReadByte(wmm, &b3);	evtLength++;
	    hw = b3;
	    break;
	case 4:	case 5:
	    break;
	case 7:
	    WARN("Strange indeed b1=0x%02x\n", b1);
	}
	break;
    }
    if (mmt->dwIndex + evtLength > mmt->dwLast)
	return MCIERR_INTERNAL;

    mmt->dwEventPulse += evtPulse;
    mmt->dwEventData   = (hw << 16) + (b2 << 8) + b1;
    mmt->wEventLength  = evtLength;

    /*
      TRACE("[%u] => pulse=%08x(%08x), data=%08x, length=%u\n",
      mmt->wTrackNr, mmt->dwEventPulse, evtPulse,
      mmt->dwEventData, mmt->wEventLength);
    */
    return 0;
}

/**************************************************************************
 * 				MIDI_mciReadMTrk		[internal]
 */
static DWORD MIDI_mciReadMTrk(WINE_MCIMIDI* wmm, MCI_MIDITRACK* mmt)
{
    DWORD		toberead;
    FOURCC		fourcc;

    if (mmioRead(wmm->hFile, (HPSTR)&fourcc, sizeof(FOURCC)) != sizeof(FOURCC)) {
	return MCIERR_INVALID_FILE;
    }

    if (fourcc != mmioFOURCC('M', 'T', 'r', 'k')) {
	WARN("Can't synchronize on 'MTrk' !\n");
	return MCIERR_INVALID_FILE;
    }

    if (MIDI_mciReadLong(wmm, &toberead) != 0) {
	return MCIERR_INVALID_FILE;
    }
    mmt->dwFirst = mmioSeek(wmm->hFile, 0, SEEK_CUR); /* >= 0 */
    mmt->dwLast = mmt->dwFirst + toberead;

    /* compute # of pulses in this track */
    mmt->dwIndex = mmt->dwFirst;
    mmt->dwEventPulse = 0;

    while (MIDI_mciReadNextEvent(wmm, mmt) == 0 && LOWORD(mmt->dwEventData) != 0x2FFF) {
	char	buf[1024];
	WORD	len;

	mmt->dwIndex += mmt->wEventLength;

	switch (LOWORD(mmt->dwEventData)) {
	case 0x02FF:
	case 0x03FF:
	    len = mmt->wEventLength - HIWORD(mmt->dwEventData);
	    if (len >= sizeof(buf)) {
		WARN("Buffer for text is too small (%u are needed)\n", len);
		len = sizeof(buf) - 1;
	    }
            if (mmioRead(wmm->hFile, buf, len) == len) {
		buf[len] = 0;	/* end string in case */
		switch (HIBYTE(LOWORD(mmt->dwEventData))) {
		case 0x02:
		    if (wmm->lpstrCopyright) {
			WARN("Two copyright notices (%s|%s)\n", debugstr_w(wmm->lpstrCopyright), buf);
			HeapFree(GetProcessHeap(), 0, wmm->lpstrCopyright);
		    }
		    len = MultiByteToWideChar( CP_ACP, 0, buf, -1, NULL, 0 );
		    wmm->lpstrCopyright = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
		    MultiByteToWideChar( CP_ACP, 0, buf, -1, wmm->lpstrCopyright, len );
		    break;
		case 0x03:
		    if (wmm->lpstrName) {
			WARN("Two names (%s|%s)\n", debugstr_w(wmm->lpstrName), buf);
			HeapFree(GetProcessHeap(), 0, wmm->lpstrName);
		    } /* last name or name from last track wins */
		    len = MultiByteToWideChar( CP_ACP, 0, buf, -1, NULL, 0 );
		    wmm->lpstrName = HeapAlloc( GetProcessHeap(), 0, len * sizeof(WCHAR) );
		    MultiByteToWideChar( CP_ACP, 0, buf, -1, wmm->lpstrName, len );
		    break;
		}
	    }
	    break;
	}
    }
    mmt->dwLength = mmt->dwEventPulse;

    TRACE("Track %u has %lu bytes and %lu pulses\n", mmt->wTrackNr, toberead, mmt->dwLength);

    /* reset track data */
    mmt->wStatus = 1;	/* ok, playing */
    mmt->dwIndex = mmt->dwFirst;
    mmt->dwEventPulse = 0;

    if (mmioSeek(wmm->hFile, 0, SEEK_CUR) != mmt->dwLast) {
	WARN("Ouch, out of sync seek=%lu track=%lu\n",
	     mmioSeek(wmm->hFile, 0, SEEK_CUR), mmt->dwLast);
	/* position at end of this track, to be ready to read next track */
	mmioSeek(wmm->hFile, mmt->dwLast, SEEK_SET);
    }

    return 0;
}

/**************************************************************************
 * 				MIDI_mciReadMThd		[internal]
 */
static DWORD MIDI_mciReadMThd(WINE_MCIMIDI* wmm, DWORD dwOffset)
{
    DWORD	toberead;
    FOURCC	fourcc;
    WORD	nt;

    TRACE("(%p, %08lX);\n", wmm, dwOffset);

    if (mmioSeek(wmm->hFile, dwOffset, SEEK_SET) != dwOffset) {
	WARN("Can't seek at %08lX begin of 'MThd'\n", dwOffset);
	return MCIERR_INVALID_FILE;
    }
    if (mmioRead(wmm->hFile, (HPSTR)&fourcc, sizeof(FOURCC)) != sizeof(FOURCC))
	return MCIERR_INVALID_FILE;

    if (fourcc != mmioFOURCC('M', 'T', 'h', 'd')) {
	WARN("Can't synchronize on 'MThd' !\n");
	return MCIERR_INVALID_FILE;
    }

    if (MIDI_mciReadLong(wmm, &toberead) != 0 || toberead < 3 * sizeof(WORD))
	return MCIERR_INVALID_FILE;

    if (MIDI_mciReadWord(wmm, &wmm->wFormat) != 0 ||
	MIDI_mciReadWord(wmm, &wmm->nTracks) != 0 ||
	MIDI_mciReadWord(wmm, &wmm->nDivision) != 0) {
	return MCIERR_INVALID_FILE;
    }

    TRACE("toberead=0x%08lX, wFormat=0x%04X nTracks=0x%04X nDivision=0x%04X\n",
	  toberead, wmm->wFormat, wmm->nTracks, wmm->nDivision);

    /* MS doc says that the MIDI MCI time format must be put by default to the format
     * stored in the MIDI file...
     */
    if (wmm->nDivision > 0x8000) {
	/* eric.pouech@lemel.fr 98/11
	 * I did not check this very code (pulses are expressed as SMPTE sub-frames).
	 * In about 40 MB of MIDI files I have, none was SMPTE based...
	 * I'm just wondering if this is widely used :-). So, if someone has one of
	 * these files, I'd like to know about it.
	 */
	FIXME("Handling SMPTE time in MIDI files has not been tested\n"
	      "Please report to comp.emulators.ms-windows.wine with MIDI file !\n");

	switch (HIBYTE(wmm->nDivision)) {
	case 0xE8:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_24;	break;	/* -24 */
	case 0xE7:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_25;	break;	/* -25 */
	case 0xE3:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_30DROP;	break;	/* -29 */ /* is the MCI constant correct ? */
	case 0xE2:	wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_30;	break;	/* -30 */
	default:
	    WARN("Unsupported number of frames %d\n", -(char)HIBYTE(wmm->nDivision));
	    return MCIERR_INVALID_FILE;
	}
	switch (LOBYTE(wmm->nDivision)) {
	case 4:	/* MIDI Time Code */
	case 8:
	case 10:
	case 80: /* SMPTE bit resolution */
	case 100:
	default:
	    WARN("Unsupported number of sub-frames %d\n", LOBYTE(wmm->nDivision));
	    return MCIERR_INVALID_FILE;
	}
    } else if (wmm->nDivision == 0) {
	WARN("Number of division is 0, can't support that !!\n");
	return MCIERR_INVALID_FILE;
    } else {
	wmm->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
    }

    switch (wmm->wFormat) {
    case 0:
	if (wmm->nTracks != 1) {
	    WARN("Got type 0 file whose number of track is not 1. Setting it to 1\n");
	    wmm->nTracks = 1;
	}
	break;
    case 1:
    case 2:
	break;
    default:
	WARN("Handling MIDI files which format = %d is not (yet) supported\n"
	     "Please report with MIDI file !\n", wmm->wFormat);
	return MCIERR_INVALID_FILE;
    }

    if (wmm->nTracks > 0x80) {
	/* wTrackNr is 7 bits only */
	FIXME("Truncating MIDI file with %u tracks\n", wmm->nTracks);
	wmm->nTracks = 0x80;
    }

    if ((wmm->tracks = HeapAlloc(GetProcessHeap(), 0, sizeof(MCI_MIDITRACK) * wmm->nTracks)) == NULL) {
	return MCIERR_OUT_OF_MEMORY;
    }

    toberead -= 3 * sizeof(WORD);
    if (toberead > 0) {
	TRACE("Size of MThd > 6, skipping %ld extra bytes\n", toberead);
	mmioSeek(wmm->hFile, toberead, SEEK_CUR);
    }

    for (nt = 0; nt < wmm->nTracks; nt++) {
	wmm->tracks[nt].wTrackNr = nt;
	if (MIDI_mciReadMTrk(wmm, &wmm->tracks[nt]) != 0) {
	    WARN("Can't read 'MTrk' header\n");
	    return MCIERR_INVALID_FILE;
	}
    }

    wmm->dwTempo = 500000;

    return 0;
}

/**************************************************************************
 * 			MIDI_ConvertPulseToMS			[internal]
 */
static	DWORD	MIDI_ConvertPulseToMS(WINE_MCIMIDI* wmm, DWORD pulse)
{
    DWORD	ret = 0;

    /* FIXME: this function may return false values since the tempo (wmm->dwTempo)
     * may change during file playing
     */
    if (wmm->nDivision == 0) {
	FIXME("Shouldn't happen. wmm->nDivision = 0\n");
    } else if (wmm->nDivision > 0x8000) { /* SMPTE, unchecked FIXME? */
	int	nf = -(char)HIBYTE(wmm->nDivision);	/* number of frames     */
	int	nsf = LOBYTE(wmm->nDivision);		/* number of sub-frames */
	ret = (pulse * 1000) / (nf * nsf);
    } else {
	ret = (DWORD)((double)pulse * ((double)wmm->dwTempo / 1000) /
		      (double)wmm->nDivision);
    }

    /*
      TRACE("pulse=%u tempo=%u division=%u=0x%04x => ms=%u\n",
      pulse, wmm->dwTempo, wmm->nDivision, wmm->nDivision, ret);
    */

    return ret;
}

#define TIME_MS_IN_ONE_HOUR	(60*60*1000)
#define TIME_MS_IN_ONE_MINUTE	(60*1000)
#define TIME_MS_IN_ONE_SECOND	(1000)

/**************************************************************************
 * 			MIDI_ConvertTimeFormatToMS		[internal]
 */
static	DWORD	MIDI_ConvertTimeFormatToMS(WINE_MCIMIDI* wmm, DWORD val)
{
    DWORD	ret = 0;

    switch (wmm->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = val;
	break;
    case MCI_FORMAT_SMPTE_24:
	ret =
	    (HIBYTE(HIWORD(val)) * 125) / 3 +             LOBYTE(HIWORD(val)) * TIME_MS_IN_ONE_SECOND +
	    HIBYTE(LOWORD(val)) * TIME_MS_IN_ONE_MINUTE + LOBYTE(LOWORD(val)) * TIME_MS_IN_ONE_HOUR;
	break;
    case MCI_FORMAT_SMPTE_25:
	ret =
	    HIBYTE(HIWORD(val)) * 40 + 		  	  LOBYTE(HIWORD(val)) * TIME_MS_IN_ONE_SECOND +
	    HIBYTE(LOWORD(val)) * TIME_MS_IN_ONE_MINUTE + LOBYTE(LOWORD(val)) * TIME_MS_IN_ONE_HOUR;
	break;
    case MCI_FORMAT_SMPTE_30:
	ret =
	    (HIBYTE(HIWORD(val)) * 100) / 3 + 		  LOBYTE(HIWORD(val)) * TIME_MS_IN_ONE_SECOND +
	    HIBYTE(LOWORD(val)) * TIME_MS_IN_ONE_MINUTE + LOBYTE(LOWORD(val)) * TIME_MS_IN_ONE_HOUR;
	break;
    default:
	WARN("Bad time format %lu!\n", wmm->dwMciTimeFormat);
    }
    /*
      TRACE("val=%u=0x%08x [tf=%u] => ret=%u\n", val, val, wmm->dwMciTimeFormat, ret);
    */
    return ret;
}

/**************************************************************************
 * 			MIDI_ConvertMSToTimeFormat		[internal]
 */
static	DWORD	MIDI_ConvertMSToTimeFormat(WINE_MCIMIDI* wmm, DWORD _val)
{
    DWORD	ret = 0, val = _val;
    DWORD	h, m, s, f;

    switch (wmm->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = val;
	break;
    case MCI_FORMAT_SMPTE_24:
    case MCI_FORMAT_SMPTE_25:
    case MCI_FORMAT_SMPTE_30:
	h = val / TIME_MS_IN_ONE_HOUR;
	m = (val -= h * TIME_MS_IN_ONE_HOUR)   / TIME_MS_IN_ONE_MINUTE;
	s = (val -= m * TIME_MS_IN_ONE_MINUTE) / TIME_MS_IN_ONE_SECOND;
	switch (wmm->dwMciTimeFormat) {
	case MCI_FORMAT_SMPTE_24:
	    /* one frame is 1000/24 val long, 1000/24 == 125/3 */
	    f = (val * 3) / 125; 	val -= (f * 125) / 3;
	    break;
	case MCI_FORMAT_SMPTE_25:
	    /* one frame is 1000/25 ms long, 1000/25 == 40 */
	    f = val / 40; 		val -= f * 40;
	    break;
	case MCI_FORMAT_SMPTE_30:
	    /* one frame is 1000/30 ms long, 1000/30 == 100/3 */
	    f = (val * 3) / 100; 	val -= (f * 100) / 3;
	    break;
	default:
	    FIXME("There must be some bad bad programmer\n");
	    f = 0;
	}
	/* val contains the number of ms which cannot make a complete frame */
	/* FIXME: is this correct ? programs seem to be happy with that */
	ret = (f << 24) | (s << 16) | (m << 8) | (h << 0);
	break;
    default:
	WARN("Bad time format %lu!\n", wmm->dwMciTimeFormat);
    }
    /*
      TRACE("val=%u [tf=%u] => ret=%u=0x%08x\n", _val, wmm->dwMciTimeFormat, ret, ret);
    */
    return ret;
}

/**************************************************************************
 * 			MIDI_GetMThdLengthMS			[internal]
 */
static	DWORD	MIDI_GetMThdLengthMS(WINE_MCIMIDI* wmm)
{
    WORD	nt;
    DWORD	ret = 0;

    for (nt = 0; nt < wmm->nTracks; nt++) {
	if (wmm->wFormat == 2) {
	    ret += wmm->tracks[nt].dwLength;
	} else if (wmm->tracks[nt].dwLength > ret) {
	    ret = wmm->tracks[nt].dwLength;
	}
    }
    /* FIXME: this is wrong if there is a tempo change inside the file */
    return MIDI_ConvertPulseToMS(wmm, ret);
}

/**************************************************************************
 * 				MIDI_mciOpen			[internal]
 */
static DWORD MIDI_mciOpen(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_OPEN_PARMSW lpParms)
{
    DWORD		dwRet = 0;

    TRACE("(%d, %08lX, %p)\n", wmm->wDevID, dwFlags, lpParms);

    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;
    if (dwFlags & MCI_OPEN_SHAREABLE)
	return MCIERR_HARDWARE;

    if (wmm->nUseCount > 0) {
	/* The driver is already opened on this channel
	 * MIDI sequencer cannot be shared
	 */
	return MCIERR_DEVICE_OPEN;
    }
    wmm->nUseCount++;

    wmm->hFile = 0;
    wmm->hMidi = 0;
    wmm->wPort = MIDI_MAPPER;
    wmm->lpstrElementName = NULL;

    TRACE("wDevID=%d (lpParams->wDeviceID=%d)\n", wmm->wDevID, lpParms->wDeviceID);
    /*	lpParms->wDeviceID = wDevID;*/

    if (dwFlags & MCI_OPEN_ELEMENT) {
	TRACE("MCI_OPEN_ELEMENT %s!\n", debugstr_w(lpParms->lpstrElementName));
        if (lpParms->lpstrElementName && lpParms->lpstrElementName[0]) {
	    wmm->hFile = mmioOpenW((LPWSTR)lpParms->lpstrElementName, NULL,
				   MMIO_ALLOCBUF | MMIO_READ | MMIO_DENYWRITE);
	    if (wmm->hFile == 0) {
		WARN("Can't find file %s!\n", debugstr_w(lpParms->lpstrElementName));
		wmm->nUseCount--;
		return MCIERR_FILE_NOT_FOUND;
	    }
            wmm->lpstrElementName = HeapAlloc(GetProcessHeap(), 0, 
                                              (lstrlenW(lpParms->lpstrElementName) + 1) * sizeof(WCHAR));
            lstrcpyW(wmm->lpstrElementName, lpParms->lpstrElementName);
	}
    }
    TRACE("hFile=%p\n", wmm->hFile);

    wmm->lpstrCopyright = NULL;
    wmm->lpstrName = NULL;

    wmm->dwStatus = MCI_MODE_NOT_READY;	/* while loading file contents */
    /* spec says it should be the default format from the MIDI file... */
    wmm->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;

    if (wmm->hFile != 0) {
	MMCKINFO	ckMainRIFF;
	MMCKINFO	mmckInfo;
	DWORD		dwOffset = 0;

	if (mmioDescend(wmm->hFile, &ckMainRIFF, NULL, 0) != 0) {
	    dwRet = MCIERR_INVALID_FILE;
	} else {
	    TRACE("ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX\n",
		  (LPSTR)&ckMainRIFF.ckid, (LPSTR)&ckMainRIFF.fccType, ckMainRIFF.cksize);

	    if (ckMainRIFF.ckid == FOURCC_RIFF && ckMainRIFF.fccType == mmioFOURCC('R', 'M', 'I', 'D')) {
		mmckInfo.ckid = mmioFOURCC('d', 'a', 't', 'a');
		if (mmioDescend(wmm->hFile, &mmckInfo, &ckMainRIFF, MMIO_FINDCHUNK) == 0) {
		    TRACE("... is a 'RMID' file\n");
		    dwOffset = mmckInfo.dwDataOffset;
		} else {
		    dwRet = MCIERR_INVALID_FILE;
		}
	    }
	    if (dwRet == 0 && MIDI_mciReadMThd(wmm, dwOffset) != 0) {
		WARN("Can't read 'MThd' header\n");
		dwRet = MCIERR_INVALID_FILE;
	    }
	}
    } else {
	TRACE("hFile==0, setting #tracks to 0; is this correct ?\n");
	wmm->nTracks = 0;
	wmm->wFormat = 0;
	wmm->nDivision = 1;
    }
    if (dwRet != 0) {
	wmm->nUseCount--;
	if (wmm->hFile != 0)
	    mmioClose(wmm->hFile, 0);
	wmm->hFile = 0;
	HeapFree(GetProcessHeap(), 0, wmm->tracks);
	HeapFree(GetProcessHeap(), 0, wmm->lpstrElementName);
	HeapFree(GetProcessHeap(), 0, wmm->lpstrCopyright);
	HeapFree(GetProcessHeap(), 0, wmm->lpstrName);
    } else {
	wmm->dwPositionMS = 0;
	wmm->dwStatus = MCI_MODE_STOP;
	if (dwFlags & MCI_NOTIFY)
	    MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    }
    return dwRet;
}

/**************************************************************************
 * 				MIDI_mciStop			[internal]
 */
static DWORD MIDI_mciStop(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet = 0;

    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (wmm->dwStatus != MCI_MODE_STOP) {
	HANDLE old = InterlockedExchangePointer(&wmm->hCallback, NULL);
	if (old) mciDriverNotify(old, wmm->wDevID, MCI_NOTIFY_ABORTED);
    }

    if (wmm->dwStatus != MCI_MODE_STOP) {
	int	oldstat = wmm->dwStatus;

	wmm->dwStatus = MCI_MODE_NOT_READY;
	if (oldstat == MCI_MODE_PAUSE)
	    dwRet = midiOutReset((HMIDIOUT)wmm->hMidi);

	if (wmm->hThread)
	    WaitForSingleObject(wmm->hThread, INFINITE);
    }

    /* sanity reset */
    wmm->dwStatus = MCI_MODE_STOP;

    if ((dwFlags & MCI_NOTIFY) && lpParms && MMSYSERR_NOERROR==dwRet)
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    return dwRet;
}

/**************************************************************************
 * 				MIDI_mciClose			[internal]
 */
static DWORD MIDI_mciClose(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{

    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (wmm->dwStatus != MCI_MODE_STOP) {
	/* mciStop handles MCI_NOTIFY_ABORTED */
	MIDI_mciStop(wmm, MCI_WAIT, lpParms);
    }

    wmm->nUseCount--;
    if (wmm->nUseCount == 0) {
	if (wmm->hFile != 0) {
	    mmioClose(wmm->hFile, 0);
	    wmm->hFile = 0;
	    TRACE("hFile closed !\n");
	}
	if (wmm->hThread) {
	    CloseHandle(wmm->hThread);
	    wmm->hThread = 0;
	}
	HeapFree(GetProcessHeap(), 0, wmm->tracks);
	HeapFree(GetProcessHeap(), 0, wmm->lpstrElementName);
	HeapFree(GetProcessHeap(), 0, wmm->lpstrCopyright);
	HeapFree(GetProcessHeap(), 0, wmm->lpstrName);
    } else {
	TRACE("Shouldn't happen... nUseCount=%d\n", wmm->nUseCount);
	return MCIERR_INTERNAL;
    }

    if ((dwFlags & MCI_NOTIFY) && lpParms) {
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    }
    return 0;
}

/**************************************************************************
 * 				MIDI_mciFindNextEvent		[internal]
 */
static MCI_MIDITRACK*	MIDI_mciFindNextEvent(WINE_MCIMIDI* wmm, LPDWORD hiPulse)
{
    WORD		cnt, nt;
    MCI_MIDITRACK*	mmt;

    *hiPulse = 0xFFFFFFFFul;
    cnt = 0xFFFFu;
    for (nt = 0; nt < wmm->nTracks; nt++) {
	mmt = &wmm->tracks[nt];

	if (mmt->wStatus == 0)
	    continue;
	if (mmt->dwEventPulse < *hiPulse) {
	    *hiPulse = mmt->dwEventPulse;
	    cnt = nt;
	}
    }
    return (cnt == 0xFFFFu) ? 0 /* no more event on all tracks */
	: &wmm->tracks[cnt];
}

/**************************************************************************
 * 				MIDI_player			[internal]
 */
static DWORD MIDI_player(WINE_MCIMIDI* wmm, DWORD dwFlags)
{
    DWORD		dwRet;
    WORD		doPlay, nt;
    MCI_MIDITRACK*	mmt;
    DWORD		hiPulse, dwStartMS = wmm->dwPositionMS;
    HANDLE		oldcb = NULL;

    /* init tracks */
    for (nt = 0; nt < wmm->nTracks; nt++) {
	mmt = &wmm->tracks[nt];

	mmt->wStatus = 1;	/* ok, playing */
	mmt->dwIndex = mmt->dwFirst;
	if (wmm->wFormat == 2 && nt > 0) {
	    mmt->dwEventPulse = wmm->tracks[nt - 1].dwLength;
	} else {
	    mmt->dwEventPulse = 0;
	}
	MIDI_mciReadNextEvent(wmm, mmt); /* FIXME == 0 */
    }

    dwRet = midiOutOpen((LPHMIDIOUT)&wmm->hMidi, wmm->wPort, 0L, 0L, CALLBACK_NULL);
    if (dwRet != MMSYSERR_NOERROR) {
	return mmr2mci(dwRet);
    }

    wmm->dwPulse = 0;
    wmm->dwTempo = 500000;
    wmm->dwPositionMS = 0;
    wmm->wStartedPlaying = FALSE;

    while (wmm->dwStatus != MCI_MODE_STOP && wmm->dwStatus != MCI_MODE_NOT_READY) {
	/* it seems that in case of multi-threading, gcc is optimizing just a little bit
	 * too much. Tell gcc not to optimize status value using volatile.
	 */
	while (((volatile WINE_MCIMIDI*)wmm)->dwStatus == MCI_MODE_PAUSE);

	doPlay = (wmm->dwPositionMS >= dwStartMS && wmm->dwPositionMS <= wmm->dwEndMS);

	TRACE("wmm->dwStatus=%d, doPlay=%c\n", wmm->dwStatus, doPlay ? 'T' : 'F');

	if ((mmt = MIDI_mciFindNextEvent(wmm, &hiPulse)) == NULL)
	    break;  /* no more event on tracks */

	/* if starting playing, then set StartTicks to the value it would have had
	 * if play had started at position 0
	 */
	if (doPlay && !wmm->wStartedPlaying) {
	    wmm->dwStartTicks = GetTickCount() - MIDI_ConvertPulseToMS(wmm, wmm->dwPulse);
	    wmm->wStartedPlaying = TRUE;
	    TRACE("Setting dwStartTicks to %lu\n", wmm->dwStartTicks);
	}

	if (hiPulse > wmm->dwPulse) {
	    wmm->dwPositionMS += MIDI_ConvertPulseToMS(wmm, hiPulse - wmm->dwPulse);
	    if (doPlay) {
		DWORD	togo = wmm->dwStartTicks + wmm->dwPositionMS;
		DWORD	tc = GetTickCount();

		TRACE("Pulses hi=0x%08lx <> cur=0x%08lx\n", hiPulse, wmm->dwPulse);
		TRACE("Wait until %lu => %lu ms\n",
		      tc - wmm->dwStartTicks, togo - wmm->dwStartTicks);
		if (tc < togo)
		    Sleep(togo - tc);
	    }
	    wmm->dwPulse = hiPulse;
	}

	switch (LOBYTE(LOWORD(mmt->dwEventData))) {
	case 0xF0:
	case 0xF7:	/* sysex events */
	    {
		FIXME("Not handling SysEx events (yet)\n");
	    }
	    break;
	case 0xFF:
	    /* position after meta data header */
	    mmioSeek(wmm->hFile, mmt->dwIndex + HIWORD(mmt->dwEventData), SEEK_SET);
	    switch (HIBYTE(LOWORD(mmt->dwEventData))) {
	    case 0x00: /* 16-bit sequence number */
		if (TRACE_ON(mcimidi)) {
		    WORD	twd;

		    MIDI_mciReadWord(wmm, &twd);	/* == 0 */
		    TRACE("Got sequence number %u\n", twd);
		}
		break;
	    case 0x01: /* any text */
	    case 0x02: /* Copyright Message text */
	    case 0x03: /* Sequence/Track Name text */
	    case 0x04: /* Instrument Name text */
	    case 0x05: /* Lyric text */
	    case 0x06: /* Marker text */
	    case 0x07: /* Cue-point text */
		if (TRACE_ON(mcimidi)) {
		    char	buf[1024];
		    WORD	len = mmt->wEventLength - HIWORD(mmt->dwEventData);
		    static const char* const	info[8] = {"", "Text", "Copyright", "Seq/Trk name",
							   "Instrument", "Lyric", "Marker", "Cue-point"};
		    WORD	idx = HIBYTE(LOWORD(mmt->dwEventData));

		    if (len >= sizeof(buf)) {
			WARN("Buffer for text is too small (%u are needed)\n", len);
			len = sizeof(buf) - 1;
		    }
                    if (mmioRead(wmm->hFile, buf, len) == len) {
			buf[len] = 0;	/* end string in case */
			TRACE("%s => \"%s\"\n", (idx < 8 ) ? info[idx] : "", buf);
		    } else {
			WARN("Couldn't read data for %s\n", (idx < 8) ? info[idx] : "");
		    }
		}
		break;
	    case 0x20:
		/* MIDI channel (cc) */
		if (FIXME_ON(mcimidi)) {
		    BYTE	bt;

		    MIDI_mciReadByte(wmm, &bt);	/* == 0 */
		    FIXME("NIY: MIDI channel=%u, track=%u\n", bt, mmt->wTrackNr);
		}
		break;
	    case 0x21:
		/* MIDI port (pp) */
		if (FIXME_ON(mcimidi)) {
		    BYTE	bt;

		    MIDI_mciReadByte(wmm, &bt);	/* == 0 */
		    FIXME("NIY: MIDI port=%u, track=%u\n", bt, mmt->wTrackNr);
		}
		break;
	    case 0x2F: /* end of track */
		mmt->wStatus = 0;
		break;
	    case 0x51:/* set tempo */
		/* Tempo is expressed in -seconds per midi quarter note
		 * for format 1 MIDI files, this can only be present on track #0
		 */
		if (mmt->wTrackNr != 0 && wmm->wFormat == 1) {
		    WARN("For format #1 MIDI files, tempo can only be changed on track #0 (%u)\n", mmt->wTrackNr);
		} else {
		    BYTE	tbt;
		    DWORD	value = 0;

		    MIDI_mciReadByte(wmm, &tbt);	value  = ((DWORD)tbt) << 16;
		    MIDI_mciReadByte(wmm, &tbt);	value |= ((DWORD)tbt) << 8;
		    MIDI_mciReadByte(wmm, &tbt);	value |= ((DWORD)tbt) << 0;
		    TRACE("Setting tempo to %ld (BPM=%ld)\n", wmm->dwTempo, (value) ? (60000000 / value) : 0);
		    wmm->dwTempo = value;
		}
		break;
	    case 0x54: /* (hour) (min) (second) (frame) (fractional-frame) - SMPTE track start */
		if (mmt->wTrackNr != 0 && wmm->wFormat == 1) {
		    WARN("For format #1 MIDI files, SMPTE track start can only be expressed on track #0 (%u)\n", mmt->wTrackNr);
		} if (mmt->dwEventPulse != 0) {
		    WARN("SMPTE track start can only be expressed at start of track (%lu)\n", mmt->dwEventPulse);
		} else {
		    BYTE	h, m, s, f, ff;

		    MIDI_mciReadByte(wmm, &h);
		    MIDI_mciReadByte(wmm, &m);
		    MIDI_mciReadByte(wmm, &s);
		    MIDI_mciReadByte(wmm, &f);
		    MIDI_mciReadByte(wmm, &ff);
		    FIXME("NIY: SMPTE track start %u:%u:%u %u.%u\n", h, m, s, f, ff);
		}
		break;
	    case 0x58: /* file rhythm */
		if (TRACE_ON(mcimidi)) {
		    BYTE	num, den, cpmc, _32npqn;

		    MIDI_mciReadByte(wmm, &num);
		    MIDI_mciReadByte(wmm, &den);		/* to notate e.g. 6/8 */
		    MIDI_mciReadByte(wmm, &cpmc);		/* number of MIDI clocks per metronome click */
		    MIDI_mciReadByte(wmm, &_32npqn);		/* number of notated 32nd notes per MIDI quarter note */

		    TRACE("%u/%u, clock per metronome click=%u, 32nd notes by 1/4 note=%u\n", num, 1 << den, cpmc, _32npqn);
		}
		break;
	    case 0x59: /* key signature */
		if (TRACE_ON(mcimidi)) {
		    BYTE	sf, mm;

		    MIDI_mciReadByte(wmm, &sf);
		    MIDI_mciReadByte(wmm, &mm);

		    if (sf >= 0x80) 	TRACE("%d flats\n", -(char)sf);
		    else if (sf > 0) 	TRACE("%d sharps\n", (char)sf);
		    else 		TRACE("Key of C\n");
		    TRACE("Mode: %s\n", (mm == 0) ? "major" : "minor");
		}
		break;
	    default:
		WARN("Unknown MIDI meta event %02x. Skipping...\n", HIBYTE(LOWORD(mmt->dwEventData)));
		break;
	    }
	    break;
	default:
	    if (doPlay) {
		dwRet = midiOutShortMsg((HMIDIOUT)wmm->hMidi, mmt->dwEventData);
	    } else {
		switch (LOBYTE(LOWORD(mmt->dwEventData)) & 0xF0) {
		case MIDI_NOTEON:
		case MIDI_NOTEOFF:
		    dwRet = 0;
		    break;
		default:
		    dwRet = midiOutShortMsg((HMIDIOUT)wmm->hMidi, mmt->dwEventData);
		}
	    }
	}
	mmt->dwIndex += mmt->wEventLength;
	if (mmt->dwIndex < mmt->dwFirst || mmt->dwIndex >= mmt->dwLast) {
	    mmt->wStatus = 0;
	}
	if (mmt->wStatus) {
	    MIDI_mciReadNextEvent(wmm, mmt);
	}
    }

    midiOutReset((HMIDIOUT)wmm->hMidi);

    dwRet = midiOutClose((HMIDIOUT)wmm->hMidi);

    if (dwFlags & MCI_NOTIFY)
	oldcb = InterlockedExchangePointer(&wmm->hCallback, NULL);

    wmm->dwStatus = MCI_MODE_STOP;

    /* Let the potentially asynchronous commands support FAILURE notification. */
    if (oldcb) mciDriverNotify(oldcb, wmm->wDevID,
	dwRet ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL);
    return mmr2mci(dwRet);
}

static DWORD CALLBACK MIDI_Starter(void *ptr)
{
    WINE_MCIMIDI* wmm = ptr;
    return MIDI_player(wmm, MCI_NOTIFY);
}

static DWORD ensurePlayerThread(WINE_MCIMIDI* wmm)
{
    if (1) {
	DWORD dwRet;

	switch (wmm->dwStatus) {
	default:
	    return MCIERR_NONAPPLICABLE_FUNCTION;
	case MCI_MODE_PAUSE:
	    return MIDI_mciResume(wmm, 0, NULL);
	case MCI_MODE_PLAY:
	    /* the player was not stopped, use it */
	    return 0;
	case MCI_MODE_STOP:
	    break;
	}
	wmm->dwStatus = MCI_MODE_PLAY;
	if (wmm->hThread) {
	    WaitForSingleObject(wmm->hThread, INFINITE);
	    CloseHandle(wmm->hThread);
	    wmm->hThread = 0;
	}
	wmm->hThread = CreateThread(NULL, 0, MIDI_Starter, wmm, 0, NULL);
	if (!wmm->hThread) {
	    dwRet = MCIERR_OUT_OF_MEMORY;
	} else {
	    SetThreadPriority(wmm->hThread, THREAD_PRIORITY_TIME_CRITICAL);
	    dwRet = 0;
	}
	if (dwRet)
	    wmm->dwStatus = MCI_MODE_STOP;
	return dwRet;
    }
}

/**************************************************************************
 * 				MIDI_mciPlay			[internal]
 */
static DWORD MIDI_mciPlay(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_PLAY_PARMS lpParms)
{
    DWORD		dwStartMS, dwEndMS;
    DWORD		dwRet;
    HANDLE		oldcb;

    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (wmm->hFile == 0) {
	WARN("Can't play: no file %s!\n", debugstr_w(wmm->lpstrElementName));
	return MCIERR_FILE_NOT_FOUND;
    }

    if (lpParms && (dwFlags & MCI_TO)) {
	dwEndMS = MIDI_ConvertTimeFormatToMS(wmm, lpParms->dwTo);
	/* FIXME: if (dwEndMS > length) return MCIERR_OUTOFRANGE; */
    } else {
	dwEndMS = 0xFFFFFFFFul; /* FIXME: dwEndMS = length; */
    }
    if (lpParms && (dwFlags & MCI_FROM)) {
	dwStartMS = MIDI_ConvertTimeFormatToMS(wmm, lpParms->dwFrom);
    } else {
	dwStartMS = wmm->dwPositionMS;
    }
    if (dwEndMS < dwStartMS)
	return MCIERR_OUTOFRANGE;

    if (dwFlags & MCI_FROM) {
	/* Stop with MCI_NOTIFY_ABORTED and set new position. */
	MIDI_mciStop(wmm, MCI_WAIT, NULL);
	wmm->dwPositionMS = dwStartMS;
    } /* else use existing player. */
    if (wmm->dwEndMS != dwEndMS) {
	oldcb = InterlockedExchangePointer(&wmm->hCallback, NULL);
	if (oldcb) mciDriverNotify(oldcb, wmm->wDevID, MCI_NOTIFY_ABORTED);
	wmm->dwEndMS = dwEndMS;
    }

    TRACE("Playing from %lu to %lu\n", dwStartMS, dwEndMS);

    if ((dwFlags & MCI_NOTIFY) && lpParms) {
	oldcb = InterlockedExchangePointer(&wmm->hCallback, HWND_32(LOWORD(lpParms->dwCallback)));
	if (oldcb) mciDriverNotify(oldcb, wmm->wDevID, MCI_NOTIFY_SUPERSEDED);
    }

    dwRet = ensurePlayerThread(wmm);

    if (!dwRet && (dwFlags & MCI_WAIT)) {
	WaitForSingleObject(wmm->hThread, INFINITE);
	GetExitCodeThread(wmm->hThread, &dwRet);
	/* STATUS_PENDING cannot happen. It folds onto MCIERR_UNRECOGNIZED_KEYWORD */
    }
    /* The player thread performs notification at exit. */
    return dwRet;
}

/**************************************************************************
 * 				MIDI_mciPause			[internal]
 */
static DWORD MIDI_mciPause(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (wmm->dwStatus == MCI_MODE_PLAY) {
	/* stop all notes */
	unsigned chn;
	for (chn = 0; chn < 16; chn++)
	    midiOutShortMsg((HMIDIOUT)(wmm->hMidi), 0x78B0 | chn);
	wmm->dwStatus = MCI_MODE_PAUSE;
    }
    if ((dwFlags & MCI_NOTIFY) && lpParms)
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MIDI_mciResume			[internal]
 */
static DWORD MIDI_mciResume(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (wmm->dwStatus == MCI_MODE_PAUSE) {
	wmm->wStartedPlaying = FALSE;
	wmm->dwStatus = MCI_MODE_PLAY;
    }
    if ((dwFlags & MCI_NOTIFY) && lpParms)
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MIDI_mciSet			[internal]
 */
static DWORD MIDI_mciSet(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_SEQ_SET_PARMS lpParms)
{
    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE("MCI_FORMAT_MILLISECONDS !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
	    break;
	case MCI_FORMAT_SMPTE_24:
	    TRACE("MCI_FORMAT_SMPTE_24 !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_24;
	    break;
	case MCI_FORMAT_SMPTE_25:
	    TRACE("MCI_FORMAT_SMPTE_25 !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_25;
	    break;
	case MCI_FORMAT_SMPTE_30:
	    TRACE("MCI_FORMAT_SMPTE_30 !\n");
	    wmm->dwMciTimeFormat = MCI_FORMAT_SMPTE_30;
	    break;
	default:
	    WARN("Bad time format %lu!\n", lpParms->dwTimeFormat);
	    return MCIERR_BAD_TIME_FORMAT;
	}
    }
    if (dwFlags & MCI_SET_VIDEO) {
	TRACE("No support for video !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_OPEN) {
	TRACE("No support for door open !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_DOOR_CLOSED) {
	TRACE("No support for door close !\n");
	return MCIERR_UNSUPPORTED_FUNCTION;
    }
    if (dwFlags & MCI_SET_AUDIO) {
	if (dwFlags & MCI_SET_ON) {
	    TRACE("MCI_SET_ON audio !\n");
	} else if (dwFlags & MCI_SET_OFF) {
	    TRACE("MCI_SET_OFF audio !\n");
	} else {
	    WARN("MCI_SET_AUDIO without SET_ON or SET_OFF\n");
	    return MCIERR_BAD_INTEGER;
	}

	switch (lpParms->dwAudio)
        {
        case MCI_SET_AUDIO_ALL:         TRACE("MCI_SET_AUDIO_ALL !\n"); break;
        case MCI_SET_AUDIO_LEFT:        TRACE("MCI_SET_AUDIO_LEFT !\n"); break;
        case MCI_SET_AUDIO_RIGHT:       TRACE("MCI_SET_AUDIO_RIGHT !\n"); break;
        default:                        WARN("Unknown audio channel %lu\n", lpParms->dwAudio); break;
        }
    }

    if (dwFlags & MCI_SEQ_SET_MASTER)
	TRACE("MCI_SEQ_SET_MASTER !\n");
    if (dwFlags & MCI_SEQ_SET_SLAVE)
	TRACE("MCI_SEQ_SET_SLAVE !\n");
    if (dwFlags & MCI_SEQ_SET_OFFSET)
	TRACE("MCI_SEQ_SET_OFFSET !\n");
    if (dwFlags & MCI_SEQ_SET_PORT) {
	TRACE("MCI_SEQ_SET_PORT = %ld\n", lpParms->dwPort);
	if ((UINT16)lpParms->dwPort != (UINT16)MIDI_MAPPER &&
	    (UINT16)lpParms->dwPort >= midiOutGetNumDevs())
	    /* FIXME: input/output port distinction? */
	    return MCIERR_SEQ_PORT_NONEXISTENT;
	/* FIXME: Native manages to swap the device while playing! */
	wmm->wPort = lpParms->dwPort;
    }
    if (dwFlags & MCI_SEQ_SET_TEMPO)
	TRACE("MCI_SEQ_SET_TEMPO !\n");
    if (dwFlags & MCI_NOTIFY)
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 * 				MIDI_mciStatus			[internal]
 */
static DWORD MIDI_mciStatus(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    DWORD		ret = 0;

    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    /* FIXME in Format 2 */
	    lpParms->dwReturn = 1;
	    TRACE("MCI_STATUS_CURRENT_TRACK => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if ((dwFlags & MCI_TRACK) && wmm->wFormat == 2) {
		if (lpParms->dwTrack >= wmm->nTracks)
		    return MCIERR_OUTOFRANGE;
		/* FIXME: this is wrong if there is a tempo change inside the file */
		lpParms->dwReturn = MIDI_ConvertPulseToMS(wmm, wmm->tracks[lpParms->dwTrack].dwLength);
	    } else {
		lpParms->dwReturn = MIDI_GetMThdLengthMS(wmm);
	    }
	    lpParms->dwReturn = MIDI_ConvertMSToTimeFormat(wmm, lpParms->dwReturn);
	    /* FIXME: ret = MCI_COLONIZED4_RETURN if SMPTE */
	    TRACE("MCI_STATUS_LENGTH => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
	    TRACE("MCI_STATUS_MODE => %u\n", wmm->dwStatus);
 	    lpParms->dwReturn = MAKEMCIRESOURCE(wmm->dwStatus, wmm->dwStatus);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE("MCI_STATUS_MEDIA_PRESENT => TRUE\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    lpParms->dwReturn = (wmm->wFormat == 2) ? wmm->nTracks : 1;
	    TRACE("MCI_STATUS_NUMBER_OF_TRACKS => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_POSITION:
	    /* FIXME: check MCI_TRACK == 1 if set */
	    lpParms->dwReturn = MIDI_ConvertMSToTimeFormat(wmm,
							   (dwFlags & MCI_STATUS_START) ? 0 : wmm->dwPositionMS);
	    /* FIXME: ret = MCI_COLONIZED4_RETURN if SMPTE */
	    TRACE("MCI_STATUS_POSITION %s => %Iu\n",
		  (dwFlags & MCI_STATUS_START) ? "start" : "current", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    lpParms->dwReturn = (wmm->dwStatus == MCI_MODE_NOT_READY) ?
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    TRACE("MCI_STATUS_READY = %u\n", LOWORD(lpParms->dwReturn));
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(wmm->dwMciTimeFormat, MCI_FORMAT_RETURN_BASE + wmm->dwMciTimeFormat);
	    TRACE("MCI_STATUS_TIME_FORMAT => %u\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_SEQ_STATUS_DIVTYPE:
	    TRACE("MCI_SEQ_STATUS_DIVTYPE !\n");
	    if (wmm->nDivision > 0x8000) {
		switch (HIBYTE(wmm->nDivision)) {
		case 0xE8:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_24;	break;	/* -24 */
		case 0xE7:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_25;	break;	/* -25 */
		case 0xE3:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_30DROP;	break;	/* -29 */ /* is the MCI constant correct ? */
		case 0xE2:	lpParms->dwReturn = MCI_SEQ_DIV_SMPTE_30;	break;	/* -30 */
		default:	FIXME("There is a bad bad programmer\n");
		}
	    } else {
		lpParms->dwReturn = MCI_SEQ_DIV_PPQN;
	    }
	    lpParms->dwReturn = MAKEMCIRESOURCE(lpParms->dwReturn,lpParms->dwReturn);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_SEQ_STATUS_MASTER:
	    TRACE("MCI_SEQ_STATUS_MASTER !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_SEQ_NONE, MCI_SEQ_NONE_S);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_SEQ_STATUS_SLAVE:
	    TRACE("MCI_SEQ_STATUS_SLAVE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_SEQ_FILE, MCI_SEQ_FILE_S);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_SEQ_STATUS_OFFSET:
	    TRACE("MCI_SEQ_STATUS_OFFSET !\n");
	    lpParms->dwReturn = 0;
	    break;
	case MCI_SEQ_STATUS_PORT:
	    if (wmm->wPort != (UINT16)MIDI_MAPPER)
		lpParms->dwReturn = wmm->wPort;
	    else {
		lpParms->dwReturn = MAKEMCIRESOURCE(MIDI_MAPPER, MCI_SEQ_MAPPER_S);
		ret = MCI_RESOURCE_RETURNED;
	    }
	    TRACE("MCI_SEQ_STATUS_PORT (%u) => %d\n", wmm->wDevID, wmm->wPort);
	    break;
	case MCI_SEQ_STATUS_TEMPO:
	    TRACE("MCI_SEQ_STATUS_TEMPO !\n");
	    lpParms->dwReturn = wmm->dwTempo;
	    break;
	default:
	    FIXME("Unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNSUPPORTED_FUNCTION;
	}
    } else {
	return MCIERR_MISSING_PARAMETER;
    }
    if ((dwFlags & MCI_NOTIFY) && HRESULT_CODE(ret)==0)
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				MIDI_mciGetDevCaps		[internal]
 */
static DWORD MIDI_mciGetDevCaps(WINE_MCIMIDI* wmm, DWORD dwFlags,
				LPMCI_GETDEVCAPS_PARMS lpParms)
{
    DWORD		ret;

    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    TRACE("MCI_GETDEVCAPS_DEVICE_TYPE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_SEQUENCER, MCI_DEVTYPE_SEQUENCER);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    TRACE("MCI_GETDEVCAPS_HAS_AUDIO !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    TRACE("MCI_GETDEVCAPS_HAS_VIDEO !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    TRACE("MCI_GETDEVCAPS_USES_FILES !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    TRACE("MCI_GETDEVCAPS_COMPOUND_DEVICE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    TRACE("MCI_GETDEVCAPS_CAN_EJECT !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    TRACE("MCI_GETDEVCAPS_CAN_PLAY !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    TRACE("MCI_GETDEVCAPS_CAN_RECORD !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    TRACE("MCI_GETDEVCAPS_CAN_SAVE !\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	default:
	    FIXME("Unknown capability (%08lx) !\n", lpParms->dwItem);
	    return MCIERR_UNRECOGNIZED_COMMAND;
	}
    } else {
	WARN("No GetDevCaps-Item !\n");
	return MCIERR_UNRECOGNIZED_COMMAND;
    }
    if ((dwFlags & MCI_NOTIFY) && HRESULT_CODE(ret)==0)
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				MIDI_mciInfo			[internal]
 */
static DWORD MIDI_mciInfo(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_INFO_PARMSW lpParms)
{
    LPCWSTR		str = 0;
    DWORD		ret = 0;

    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (lpParms == NULL || lpParms->lpstrReturn == NULL)
	return MCIERR_NULL_PARAMETER_BLOCK;

    TRACE("buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);

    switch (dwFlags & ~(MCI_WAIT|MCI_NOTIFY)) {
    case MCI_INFO_PRODUCT:      str = L"Wine's MIDI sequencer"; break;
    case MCI_INFO_FILE:         str = wmm->lpstrElementName; break;
    case MCI_INFO_COPYRIGHT:    str = wmm->lpstrCopyright; break;
    case MCI_INFO_NAME:         str = wmm->lpstrName; break;
    default:
	WARN("Don't know this info command (%lu)\n", dwFlags);
	return MCIERR_MISSING_PARAMETER; /* not MCIERR_FLAGS_... */
    }
    if (!ret) {
	if (lpParms->dwRetSize) {
	    /* FIXME? Since NT, mciwave, mciseq and mcicda set dwRetSize
	     *        to the number of characters written, excluding \0. */
            lstrcpynW(lpParms->lpstrReturn, str ? str : L"", lpParms->dwRetSize);
	} else ret = MCIERR_PARAM_OVERFLOW;
    }
    if (MMSYSERR_NOERROR==ret && (dwFlags & MCI_NOTIFY))
	MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				MIDI_mciSeek			[internal]
 */
static DWORD MIDI_mciSeek(WINE_MCIMIDI* wmm, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    DWORD		position;

    TRACE("(%d, %08lX, %p);\n", wmm->wDevID, dwFlags, lpParms);

    if (lpParms == NULL) 	return MCIERR_NULL_PARAMETER_BLOCK;

    position = dwFlags & (MCI_SEEK_TO_START|MCI_SEEK_TO_END|MCI_TO);
    if (!position)		return MCIERR_MISSING_PARAMETER;
    if (position&(position-1))	return MCIERR_FLAGS_NOT_COMPATIBLE;

    MIDI_mciStop(wmm, MCI_WAIT, 0);

    if (dwFlags & MCI_TO) { /* FIXME: compare with length */
        wmm->dwPositionMS = MIDI_ConvertTimeFormatToMS(wmm, lpParms->dwTo);
    } else if (dwFlags & MCI_SEEK_TO_START) {
        wmm->dwPositionMS = 0;
    } else {
        wmm->dwPositionMS = 0xFFFFFFFF; /* FIXME */
    }

    TRACE("Seeking to position=%lu ms\n", wmm->dwPositionMS);

    if (dwFlags & MCI_NOTIFY)
        MIDI_mciNotify(lpParms->dwCallback, wmm, MCI_NOTIFY_SUCCESSFUL);

    return 0;
}

/*======================================================================*
 *                  	    MIDI entry points 				*
 *======================================================================*/

/**************************************************************************
 * 				DriverProc (MCISEQ.@)
 */
LRESULT CALLBACK MCIMIDI_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                    LPARAM dwParam1, LPARAM dwParam2)
{
    WINE_MCIMIDI*	wmm;
    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "Sample Midi Driver !", "OSS Driver", MB_OK); return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    case DRV_OPEN:		return MIDI_drvOpen((LPCWSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSW)dwParam2);
    case DRV_CLOSE:		return MIDI_drvClose(dwDevID);
    }
    if ((wMsg < DRV_MCI_FIRST) || (wMsg > DRV_MCI_LAST)) {
	TRACE("Sending msg %04x to default driver proc\n", wMsg);
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }

    wmm = MIDI_mciGetOpenDev(dwDevID, wMsg);
    if (wmm == NULL)		return MCIERR_INVALID_DEVICE_ID;

    switch (wMsg) {
    case MCI_OPEN_DRIVER:	return MIDI_mciOpen      (wmm, dwParam1, (LPMCI_OPEN_PARMSW)     dwParam2);
    case MCI_CLOSE_DRIVER:	return MIDI_mciClose     (wmm, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_PLAY:		return MIDI_mciPlay      (wmm, dwParam1, (LPMCI_PLAY_PARMS)      dwParam2);
    case MCI_STOP:		return MIDI_mciStop      (wmm, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_SET:		return MIDI_mciSet       (wmm, dwParam1, (LPMCI_SEQ_SET_PARMS)   dwParam2);
    case MCI_PAUSE:		return MIDI_mciPause     (wmm, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_RESUME:		return MIDI_mciResume    (wmm, dwParam1, (LPMCI_GENERIC_PARMS)   dwParam2);
    case MCI_STATUS:		return MIDI_mciStatus    (wmm, dwParam1, (LPMCI_STATUS_PARMS)    dwParam2);
    case MCI_GETDEVCAPS:	return MIDI_mciGetDevCaps(wmm, dwParam1, (LPMCI_GETDEVCAPS_PARMS)dwParam2);
    case MCI_INFO:		return MIDI_mciInfo      (wmm, dwParam1, (LPMCI_INFO_PARMSW)     dwParam2);
    case MCI_SEEK:		return MIDI_mciSeek      (wmm, dwParam1, (LPMCI_SEEK_PARMS)      dwParam2);
    case MCI_OPEN:
    case MCI_CLOSE:
	FIXME("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	/* fall through */
    default:
	TRACE("Unsupported command [0x%x]\n", wMsg);
        return MCIERR_UNSUPPORTED_FUNCTION; /* Win9x: MCIERR_UNRECOGNIZED_COMMAND */
    }
}
