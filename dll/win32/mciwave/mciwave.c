/*
 * Wine Driver for MCI wave forms
 *
 * Copyright 	1994 Martin Ayotte
 *		1999,2000,2005 Eric Pouech
 *              2000 Francois Jacques
 *		2009 Jörg Höhle
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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "mmddk.h"
#include "wownt32.h"
#include "digitalv.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciwave);

typedef struct {
    UINT			wDevID;
    HANDLE			hWave;
    int				nUseCount;	/* Incremented for each shared open */
    HMMIO			hFile;  	/* mmio file handle open as Element */
    MCIDEVICEID			wNotifyDeviceID;	/* MCI device ID with a pending notification */
    HANDLE			hCallback;	/* Callback handle for pending notification */
    LPWSTR			lpFileName;	/* Name of file (if any)                     */
    WAVEFORMATEX		wfxRef;
    LPWAVEFORMATEX		lpWaveFormat;	/* Points to wfxRef until set by OPEN or RECORD */
    BOOL			fInput;		/* FALSE = Output, TRUE = Input */
    WORD			wInput;		/* wave input device */
    WORD			wOutput;	/* wave output device */
    volatile WORD		dwStatus;	/* one from MCI_MODE_xxxx */
    DWORD			dwMciTimeFormat;/* One of the supported MCI_FORMAT_xxxx */
    DWORD			dwPosition;	/* position in bytes in chunk */
    HANDLE			hEvent;		/* for synchronization */
    LONG			dwEventCount;	/* for synchronization */
    MMCKINFO                   	ckMainRIFF;     /* main RIFF chunk */
    MMCKINFO                   	ckWaveData;     /* data chunk */
} WINE_MCIWAVE;

/* ===================================================================
 * ===================================================================
 * FIXME: should be using the new mmThreadXXXX functions from WINMM
 * instead of those
 * it would require to add a wine internal flag to mmThreadCreate
 * in order to pass a 32 bit function instead of a 16 bit one
 * ===================================================================
 * =================================================================== */

typedef DWORD (*async_cmd)(MCIDEVICEID wDevID, DWORD_PTR dwFlags, DWORD_PTR pmt, HANDLE evt);

struct SCA {
    async_cmd   cmd;
    HANDLE      evt;
    UINT 	wDevID;
    DWORD_PTR   dwParam1;
    DWORD_PTR   dwParam2;
};

/**************************************************************************
 * 				MCI_SCAStarter			[internal]
 */
static DWORD CALLBACK	MCI_SCAStarter(LPVOID arg)
{
    struct SCA*	sca = (struct SCA*)arg;
    DWORD		ret;

    TRACE("In thread before async command (%08x,%08Ix,%08Ix)\n",
	  sca->wDevID, sca->dwParam1, sca->dwParam2);
    ret = sca->cmd(sca->wDevID, sca->dwParam1 | MCI_WAIT, sca->dwParam2, sca->evt);
    TRACE("In thread after async command (%08x,%08Ix,%08Ix)\n",
	  sca->wDevID, sca->dwParam1, sca->dwParam2);
    free(sca);
    return ret;
}

/**************************************************************************
 * 				MCI_SendCommandAsync		[internal]
 */
static	DWORD MCI_SendCommandAsync(UINT wDevID, async_cmd cmd, DWORD_PTR dwParam1,
				   DWORD_PTR dwParam2, UINT size)
{
    HANDLE handles[2];
    struct SCA* sca = malloc(sizeof(struct SCA) + size);

    if (sca == 0)
	return MCIERR_OUT_OF_MEMORY;

    sca->wDevID   = wDevID;
    sca->cmd      = cmd;
    sca->dwParam1 = dwParam1;

    if (size && dwParam2) {
	sca->dwParam2 = (DWORD_PTR)sca + sizeof(struct SCA);
	/* copy structure passed by program in dwParam2 to be sure
	 * we can still use it whatever the program does
	 */
	memcpy((LPVOID)sca->dwParam2, (LPVOID)dwParam2, size);
    } else {
	sca->dwParam2 = dwParam2;
    }

    if ((sca->evt = handles[1] = CreateEventW(NULL, FALSE, FALSE, NULL)) == NULL ||
        (handles[0] = CreateThread(NULL, 0, MCI_SCAStarter, sca, 0, NULL)) == 0) {
	WARN("Couldn't allocate thread for async command handling, sending synchronously\n");
        if (handles[1]) CloseHandle(handles[1]);
        sca->evt = NULL;
        return MCI_SCAStarter(sca);
    }

    SetThreadPriority(handles[0], THREAD_PRIORITY_TIME_CRITICAL);
    /* wait until either:
     * - the thread has finished (handles[0], likely an error)
     * - init phase of async command is done (handles[1])
     */
    WaitForMultipleObjects(2, handles, FALSE, INFINITE);
    CloseHandle(handles[0]);
    CloseHandle(handles[1]);
    return 0;
}

/*======================================================================*
 *                  	    MCI WAVE implementation			*
 *======================================================================*/

static DWORD WAVE_mciResume(UINT wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms);

/**************************************************************************
 * 				MCIWAVE_drvOpen			[internal]
 */
static LRESULT WAVE_drvOpen(LPCWSTR str, LPMCI_OPEN_DRIVER_PARMSW modp)
{
    WINE_MCIWAVE*	wmw;

    if (modp == NULL) return 0xFFFFFFFF;

    wmw = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WINE_MCIWAVE));

    if (!wmw)
	return 0;

    wmw->wDevID = modp->wDeviceID;
    mciSetDriverData(wmw->wDevID, (DWORD_PTR)wmw);
    modp->wCustomCommandTable = MCI_NO_COMMAND_TABLE;
    modp->wType = MCI_DEVTYPE_WAVEFORM_AUDIO;

    wmw->wfxRef.wFormatTag     	= WAVE_FORMAT_PCM;
    wmw->wfxRef.nChannels       = 1;      /* MONO */
    wmw->wfxRef.nSamplesPerSec  = 11025;
    wmw->wfxRef.nAvgBytesPerSec = 11025;
    wmw->wfxRef.nBlockAlign     = 1;
    wmw->wfxRef.wBitsPerSample  = 8;
    wmw->wfxRef.cbSize          = 0;      /* don't care */

    return modp->wDeviceID;
}

/**************************************************************************
 * 				MCIWAVE_drvClose		[internal]
 */
static LRESULT WAVE_drvClose(MCIDEVICEID dwDevID)
{
    WINE_MCIWAVE*  wmw = (WINE_MCIWAVE*)mciGetDriverData(dwDevID);

    if (wmw) {
	free(wmw);
	mciSetDriverData(dwDevID, 0);
	return 1;
    }
    return (dwDevID == 0xFFFFFFFF) ? 1 : 0;
}

/**************************************************************************
 * 				WAVE_mciGetOpenDev		[internal]
 */
static WINE_MCIWAVE *WAVE_mciGetOpenDev(MCIDEVICEID wDevID)
{
    WINE_MCIWAVE*	wmw = (WINE_MCIWAVE*)mciGetDriverData(wDevID);

    if (wmw == NULL || wmw->nUseCount == 0) {
	WARN("Invalid wDevID=%u\n", wDevID);
	return 0;
    }
    return wmw;
}

/**************************************************************************
 *				WAVE_mciNotify			[internal]
 *
 * Notifications in MCI work like a 1-element queue.
 * Each new notification request supersedes the previous one.
 * This affects Play and Record; other commands are immediate.
 */
static void WAVE_mciNotify(DWORD_PTR hWndCallBack, WINE_MCIWAVE* wmw, UINT wStatus)
{
    /* We simply save one parameter by not passing the wDevID local
     * to the command.  They are the same (via mciGetDriverData).
     */
    MCIDEVICEID wDevID = wmw->wNotifyDeviceID;
    HANDLE old = InterlockedExchangePointer(&wmw->hCallback, NULL);
    if (old) mciDriverNotify(old, wDevID, MCI_NOTIFY_SUPERSEDED);
    mciDriverNotify(HWND_32(LOWORD(hWndCallBack)), wDevID, wStatus);
}

/**************************************************************************
 * 				WAVE_ConvertByteToTimeFormat	[internal]
 */
static	DWORD 	WAVE_ConvertByteToTimeFormat(WINE_MCIWAVE* wmw, DWORD val)
{
    DWORD	   ret = 0;

    switch (wmw->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = MulDiv(val,1000,wmw->lpWaveFormat->nAvgBytesPerSec);
	break;
    case MCI_FORMAT_BYTES:
	ret = val;
	break;
    case MCI_FORMAT_SAMPLES:
	ret = MulDiv(val,wmw->lpWaveFormat->nSamplesPerSec,wmw->lpWaveFormat->nAvgBytesPerSec);
	break;
    default:
	WARN("Bad time format %lu!\n", wmw->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wmw->dwMciTimeFormat, ret);
    return ret;
}

/**************************************************************************
 * 				WAVE_ConvertTimeFormatToByte	[internal]
 */
static	DWORD 	WAVE_ConvertTimeFormatToByte(WINE_MCIWAVE* wmw, DWORD val)
{
    DWORD	ret = 0;

    switch (wmw->dwMciTimeFormat) {
    case MCI_FORMAT_MILLISECONDS:
	ret = MulDiv(val,wmw->lpWaveFormat->nAvgBytesPerSec,1000);
	if (ret > wmw->ckWaveData.cksize &&
	    val == WAVE_ConvertByteToTimeFormat(wmw, wmw->ckWaveData.cksize))
	    ret = wmw->ckWaveData.cksize;
	break;
    case MCI_FORMAT_BYTES:
	ret = val;
	break;
    case MCI_FORMAT_SAMPLES:
	ret = MulDiv(val,wmw->lpWaveFormat->nAvgBytesPerSec,wmw->lpWaveFormat->nSamplesPerSec);
	break;
    default:
	WARN("Bad time format %lu!\n", wmw->dwMciTimeFormat);
    }
    TRACE("val=%lu=0x%08lx [tf=%lu] => ret=%lu\n", val, val, wmw->dwMciTimeFormat, ret);
    return ret;
}

/**************************************************************************
 * 			WAVE_mciReadFmt	                        [internal]
 */
static	DWORD WAVE_mciReadFmt(WINE_MCIWAVE* wmw, const MMCKINFO* pckMainRIFF)
{
    MMCKINFO	mmckInfo;
    LONG	r;
    LPWAVEFORMATEX pwfx;

    mmckInfo.ckid = mmioFOURCC('f', 'm', 't', ' ');
    if (mmioDescend(wmw->hFile, &mmckInfo, pckMainRIFF, MMIO_FINDCHUNK) != 0)
	return MCIERR_INVALID_FILE;
    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX\n",
	  (LPSTR)&mmckInfo.ckid, (LPSTR)&mmckInfo.fccType, mmckInfo.cksize);

    pwfx = malloc(mmckInfo.cksize);
    if (!pwfx) return MCIERR_OUT_OF_MEMORY;

    r = mmioRead(wmw->hFile, (HPSTR)pwfx, mmckInfo.cksize);
    if (r < 0 || r < sizeof(PCMWAVEFORMAT)) {
	free(pwfx);
	return MCIERR_INVALID_FILE;
    }
    TRACE("wFormatTag=%04X !\n",   pwfx->wFormatTag);
    TRACE("nChannels=%d\n",        pwfx->nChannels);
    TRACE("nSamplesPerSec=%ld\n",   pwfx->nSamplesPerSec);
    TRACE("nAvgBytesPerSec=%ld\n",  pwfx->nAvgBytesPerSec);
    TRACE("nBlockAlign=%d\n",      pwfx->nBlockAlign);
    TRACE("wBitsPerSample=%u !\n", pwfx->wBitsPerSample);
    if (r >= sizeof(WAVEFORMATEX))
	TRACE("cbSize=%u !\n",     pwfx->cbSize);
    if ((pwfx->wFormatTag != WAVE_FORMAT_PCM)
	&& (r < sizeof(WAVEFORMATEX) || (r < sizeof(WAVEFORMATEX) + pwfx->cbSize))) {
	free(pwfx);
	return MCIERR_INVALID_FILE;
    }
    wmw->lpWaveFormat = pwfx;

    mmioAscend(wmw->hFile, &mmckInfo, 0);
    wmw->ckWaveData.ckid = mmioFOURCC('d', 'a', 't', 'a');
    if (mmioDescend(wmw->hFile, &wmw->ckWaveData, pckMainRIFF, MMIO_FINDCHUNK) != 0) {
	TRACE("can't find data chunk\n");
	return MCIERR_INVALID_FILE;
    }
    TRACE("Chunk Found ckid=%.4s fccType=%.4s cksize=%08lX\n",
	  (LPSTR)&wmw->ckWaveData.ckid, (LPSTR)&wmw->ckWaveData.fccType, wmw->ckWaveData.cksize);
    return 0;
}

/**************************************************************************
 * 			WAVE_mciDefaultFmt			[internal]
 *
 * wmw->lpWaveFormat points to the default wave format at wmw->wfxRef
 * until either Open File or Record.  It becomes immutable afterwards,
 * i.e. Set wave format or channels etc. is subsequently refused.
 */
static void WAVE_mciDefaultFmt(WINE_MCIWAVE* wmw)
{
    wmw->lpWaveFormat = &wmw->wfxRef;
    wmw->lpWaveFormat->wFormatTag = WAVE_FORMAT_PCM;
    wmw->lpWaveFormat->nChannels = 1;
    wmw->lpWaveFormat->nSamplesPerSec = 11025;
    wmw->lpWaveFormat->nAvgBytesPerSec = 11025;
    wmw->lpWaveFormat->nBlockAlign = 1;
    wmw->lpWaveFormat->wBitsPerSample = 8;
    wmw->lpWaveFormat->cbSize = 0;
}

/**************************************************************************
 * 			WAVE_mciCreateRIFFSkeleton              [internal]
 */
static DWORD WAVE_mciCreateRIFFSkeleton(WINE_MCIWAVE* wmw)
{
   MMCKINFO     ckWaveFormat;
   LPMMCKINFO   lpckRIFF     = &(wmw->ckMainRIFF);
   LPMMCKINFO   lpckWaveData = &(wmw->ckWaveData);

   lpckRIFF->ckid    = FOURCC_RIFF;
   lpckRIFF->fccType = mmioFOURCC('W', 'A', 'V', 'E');
   lpckRIFF->cksize  = 0;

   if (MMSYSERR_NOERROR != mmioCreateChunk(wmw->hFile, lpckRIFF, MMIO_CREATERIFF))
	goto err;

   ckWaveFormat.fccType = 0;
   ckWaveFormat.ckid    = mmioFOURCC('f', 'm', 't', ' ');
   ckWaveFormat.cksize  = sizeof(PCMWAVEFORMAT);

   /* Set wave format accepts PCM only, however open an
    * existing ADPCM file, record into it and the MCI will
    * happily save back in that format. */
   if (wmw->lpWaveFormat->wFormatTag == WAVE_FORMAT_PCM) {
	if (wmw->lpWaveFormat->nBlockAlign !=
	    wmw->lpWaveFormat->nChannels * wmw->lpWaveFormat->wBitsPerSample/8) {
	    WORD size = wmw->lpWaveFormat->nChannels *
		wmw->lpWaveFormat->wBitsPerSample/8;
	    WARN("Incorrect nBlockAlign (%d), setting it to %d\n",
		wmw->lpWaveFormat->nBlockAlign, size);
	    wmw->lpWaveFormat->nBlockAlign = size;
	}
	if (wmw->lpWaveFormat->nAvgBytesPerSec !=
	    wmw->lpWaveFormat->nSamplesPerSec * wmw->lpWaveFormat->nBlockAlign) {
	    DWORD speed = wmw->lpWaveFormat->nSamplesPerSec *
		wmw->lpWaveFormat->nBlockAlign;
	    WARN("Incorrect nAvgBytesPerSec (%ld), setting it to %ld\n",
		wmw->lpWaveFormat->nAvgBytesPerSec, speed);
	    wmw->lpWaveFormat->nAvgBytesPerSec = speed;
	}
   }
   if (wmw->lpWaveFormat == &wmw->wfxRef) {
	LPWAVEFORMATEX pwfx = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WAVEFORMATEX));
	if (!pwfx) return MCIERR_OUT_OF_MEMORY;
	/* Set wave format accepts PCM only so the size is known. */
	assert(wmw->wfxRef.wFormatTag == WAVE_FORMAT_PCM);
	*pwfx = wmw->wfxRef;
	wmw->lpWaveFormat = pwfx;
   }

   if (MMSYSERR_NOERROR != mmioCreateChunk(wmw->hFile, &ckWaveFormat, 0))
	goto err;

   if (-1 == mmioWrite(wmw->hFile, (HPCSTR)wmw->lpWaveFormat, (WAVE_FORMAT_PCM==wmw->lpWaveFormat->wFormatTag)
	? sizeof(PCMWAVEFORMAT) : sizeof(WAVEFORMATEX)+wmw->lpWaveFormat->cbSize))
	goto err;

   if (MMSYSERR_NOERROR != mmioAscend(wmw->hFile, &ckWaveFormat, 0))
	goto err;

   lpckWaveData->cksize  = 0;
   lpckWaveData->fccType = 0;
   lpckWaveData->ckid    = mmioFOURCC('d', 'a', 't', 'a');

   /* create data chunk */
   if (MMSYSERR_NOERROR != mmioCreateChunk(wmw->hFile, lpckWaveData, 0))
	goto err;

   return 0;

err:
   /* mciClose takes care of wmw->lpWaveFormat. */
   return MCIERR_INVALID_FILE;
}

static DWORD create_tmp_file(HMMIO* hFile, LPWSTR* pszTmpFileName)
{
    WCHAR       szTmpPath[MAX_PATH];
    WCHAR       szPrefix[4];
    DWORD       dwRet = MMSYSERR_NOERROR;

    szPrefix[0] = 'M';
    szPrefix[1] = 'C';
    szPrefix[2] = 'I';
    szPrefix[3] = '\0';

    if (!GetTempPathW(ARRAY_SIZE(szTmpPath), szTmpPath)) {
        WARN("can't retrieve temp path!\n");
        *pszTmpFileName = NULL;
        return MCIERR_FILE_NOT_FOUND;
    }

    *pszTmpFileName = calloc(1, MAX_PATH * sizeof(WCHAR));
    if (!GetTempFileNameW(szTmpPath, szPrefix, 0, *pszTmpFileName)) {
        WARN("can't retrieve temp file name!\n");
        free(*pszTmpFileName);
        return MCIERR_FILE_NOT_FOUND;
    }

    TRACE("%s!\n", debugstr_w(*pszTmpFileName));

    if (*pszTmpFileName && (*pszTmpFileName)[0]) {

        *hFile = mmioOpenW(*pszTmpFileName, NULL,
                           MMIO_ALLOCBUF | MMIO_READWRITE | MMIO_CREATE);

        if (*hFile == 0) {
            WARN("can't create file=%s!\n", debugstr_w(*pszTmpFileName));
            /* temporary file could not be created. clean filename. */
            free(*pszTmpFileName);
            dwRet = MCIERR_FILE_NOT_FOUND;
        }
    }
    return dwRet;
}

static LRESULT WAVE_mciOpenFile(WINE_MCIWAVE* wmw, LPCWSTR filename)
{
    LRESULT dwRet = MMSYSERR_NOERROR;
    LPWSTR fn;

    fn = wcsdup(filename);
    if (!fn) return MCIERR_OUT_OF_MEMORY;
    free(wmw->lpFileName);
    wmw->lpFileName = fn;

    if (filename[0]) {
        /* FIXME : what should be done if wmw->hFile is already != 0, or the driver is playin' */
        TRACE("MCI_OPEN_ELEMENT %s!\n", debugstr_w(filename));

        wmw->hFile = mmioOpenW((LPWSTR)filename, NULL,
                               MMIO_ALLOCBUF | MMIO_DENYWRITE | MMIO_READ);

        if (wmw->hFile == 0) {
            WARN("can't find file=%s!\n", debugstr_w(filename));
            dwRet = MCIERR_FILE_NOT_FOUND;
        }
        else
        {
            LPMMCKINFO          lpckMainRIFF = &wmw->ckMainRIFF;

            /* make sure we're at the beginning of the file */
            mmioSeek(wmw->hFile, 0, SEEK_SET);

            /* first reading of this file. read the waveformat chunk */
            if (mmioDescend(wmw->hFile, lpckMainRIFF, NULL, 0) != 0) {
                dwRet = MCIERR_INVALID_FILE;
            } else {
                TRACE("ParentChunk ckid=%.4s fccType=%.4s cksize=%08lX\n",
                      (LPSTR)&(lpckMainRIFF->ckid),
                      (LPSTR) &(lpckMainRIFF->fccType),
                      (lpckMainRIFF->cksize));

                if ((lpckMainRIFF->ckid != FOURCC_RIFF) ||
                    lpckMainRIFF->fccType != mmioFOURCC('W', 'A', 'V', 'E')) {
                    dwRet = MCIERR_INVALID_FILE;
                } else {
                    dwRet = WAVE_mciReadFmt(wmw, lpckMainRIFF);
                }
            }
        }
    }
    return dwRet;
}

/**************************************************************************
 * 			WAVE_mciOpen	                        [internal]
 */
static LRESULT WAVE_mciOpen(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_WAVE_OPEN_PARMSW lpOpenParms)
{
    DWORD		dwRet = 0;
    WINE_MCIWAVE*	wmw = (WINE_MCIWAVE*)mciGetDriverData(wDevID);

    TRACE("(%04X, %08lX, %p)\n", wDevID, dwFlags, lpOpenParms);
    if (lpOpenParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL) 		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_OPEN_SHAREABLE)
	return MCIERR_UNSUPPORTED_FUNCTION;

    if (wmw->nUseCount > 0) {
	/* The driver is already opened on this channel
	 * Wave driver cannot be shared
	 */
	return MCIERR_DEVICE_OPEN;
    }

    wmw->nUseCount++;

    wmw->wInput = wmw->wOutput = WAVE_MAPPER;
    wmw->fInput = FALSE;
    wmw->hWave = 0;
    wmw->dwStatus = MCI_MODE_NOT_READY;
    wmw->hFile = 0;
    wmw->lpFileName = NULL; /* will be set by WAVE_mciOpenFile */
    wmw->hCallback = NULL;
    WAVE_mciDefaultFmt(wmw);

    TRACE("wDevID=%04X (lpParams->wDeviceID=%08X)\n", wDevID, lpOpenParms->wDeviceID);
    /* Logs show the native winmm calls us with 0 still in lpOpenParms.wDeviceID */
    wmw->wNotifyDeviceID = wDevID;

    if (dwFlags & MCI_OPEN_ELEMENT) {
	if (dwFlags & MCI_OPEN_ELEMENT_ID) {
	    /* could it be that (DWORD)lpOpenParms->lpstrElementName
	     * contains the hFile value ?
	     */
	    dwRet = MCIERR_UNRECOGNIZED_COMMAND;
	} else {
            dwRet = WAVE_mciOpenFile(wmw, lpOpenParms->lpstrElementName);
	}
    }
    TRACE("hFile=%p\n", wmw->hFile);

    if (dwRet == 0) {
	wmw->dwPosition = 0;

	wmw->dwStatus = MCI_MODE_STOP;

	if (dwFlags & MCI_NOTIFY)
	    WAVE_mciNotify(lpOpenParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);
    } else {
	wmw->nUseCount--;
	if (wmw->hFile != 0)
	    mmioClose(wmw->hFile, 0);
	wmw->hFile = 0;
	free(wmw->lpFileName);
	wmw->lpFileName = NULL;
    }
    return dwRet;
}

/**************************************************************************
 *                               WAVE_mciCue             [internal]
 */
static DWORD WAVE_mciCue(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    /* Tests on systems without sound drivers show that Cue, like
     * Record and Play, opens winmm, returning MCIERR_WAVE_xyPUTSUNSUITABLE.
     * The first Cue Notify does not immediately return the
     * notification, as if a player or recorder thread is started.
     * PAUSE mode is reported when successful, but this mode is
     * different from the normal Pause, because a) Pause then returns
     * NONAPPLICABLE_FUNCTION instead of 0 and b) Set Channels etc. is
     * still accepted, returning the original notification as ABORTED.
     * I.e. Cue allows subsequent format changes, unlike Record or
     * Open file, closes winmm if the format changes and stops this
     * thread.
     * Wine creates one player or recorder thread per async. Play or
     * Record command.  Notification behaviour suggests that MS-W*
     * reuses a single thread to improve response times.  Having Cue
     * start this thread early helps to improve Play/Record's initial
     * response time.  In effect, Cue is a performance hint, which
     * justifies our almost no-op implementation.
     */

    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (wmw->dwStatus != MCI_MODE_STOP) return MCIERR_NONAPPLICABLE_FUNCTION;

    if ((dwFlags & MCI_NOTIFY) && lpParms)
	WAVE_mciNotify(lpParms->dwCallback,wmw,MCI_NOTIFY_SUCCESSFUL);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				WAVE_mciStop			[internal]
 */
static DWORD WAVE_mciStop(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD 		dwRet = 0;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (wmw->dwStatus != MCI_MODE_STOP) {
	HANDLE old = InterlockedExchangePointer(&wmw->hCallback, NULL);
	if (old) mciDriverNotify(old, wDevID, MCI_NOTIFY_ABORTED);
    }

    /* wait for playback thread (if any) to exit before processing further */
    switch (wmw->dwStatus) {
    case MCI_MODE_PAUSE:
    case MCI_MODE_PLAY:
    case MCI_MODE_RECORD:
        wmw->dwStatus = MCI_MODE_NOT_READY;
        dwRet = (wmw->fInput) ? waveInReset(wmw->hWave) : waveOutReset(wmw->hWave);
	while (wmw->dwStatus != MCI_MODE_STOP)
	    Sleep(10);
	break;
    }

    /* sanity resets */
    wmw->dwStatus = MCI_MODE_STOP;

    if ((dwFlags & MCI_NOTIFY) && lpParms && MMSYSERR_NOERROR==dwRet)
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);

    return dwRet;
}

/**************************************************************************
 *				WAVE_mciClose		[internal]
 */
static DWORD WAVE_mciClose(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD		dwRet = 0;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (wmw->dwStatus != MCI_MODE_STOP) {
        /* mciStop handles MCI_NOTIFY_ABORTED */
	dwRet = WAVE_mciStop(wDevID, MCI_WAIT, lpParms);
    }

    wmw->nUseCount--;

    if (wmw->nUseCount == 0) {
	if (wmw->hFile != 0) {
	    mmioClose(wmw->hFile, 0);
	    wmw->hFile = 0;
	}
    }

    if (wmw->lpWaveFormat != &wmw->wfxRef)
	free(wmw->lpWaveFormat);
    wmw->lpWaveFormat = &wmw->wfxRef;
    free(wmw->lpFileName);
    wmw->lpFileName = NULL;

    if ((dwFlags & MCI_NOTIFY) && lpParms) {
	WAVE_mciNotify(lpParms->dwCallback, wmw,
	    (dwRet == 0) ? MCI_NOTIFY_SUCCESSFUL : MCI_NOTIFY_FAILURE);
    }

    return 0;
}

/**************************************************************************
 * 				WAVE_mciPlayCallback		[internal]
 */
static	void	CALLBACK WAVE_mciPlayCallback(HWAVEOUT hwo, UINT uMsg,
					      DWORD_PTR dwInstance,
					      LPARAM dwParam1, LPARAM dwParam2)
{
    WINE_MCIWAVE*	wmw = (WINE_MCIWAVE*)dwInstance;

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	break;
    case WOM_DONE:
	InterlockedIncrement(&wmw->dwEventCount);
	TRACE("Returning waveHdr=%Ix\n", dwParam1);
	SetEvent(wmw->hEvent);
	break;
    default:
	ERR("Unknown uMsg=%d\n", uMsg);
    }
}

/******************************************************************
 *			WAVE_mciPlayWaitDone		[internal]
 */
static void WAVE_mciPlayWaitDone(WINE_MCIWAVE* wmw)
{
    for (;;) {
	ResetEvent(wmw->hEvent);
	if (InterlockedDecrement(&wmw->dwEventCount) >= 0) {
	    break;
	}
	InterlockedIncrement(&wmw->dwEventCount);

	WaitForSingleObject(wmw->hEvent, INFINITE);
    }
}

/**************************************************************************
 * 				WAVE_mciPlay		[internal]
 */
static DWORD WAVE_mciPlay(MCIDEVICEID wDevID, DWORD_PTR dwFlags, DWORD_PTR pmt, HANDLE hEvent)
{
    LPMCI_PLAY_PARMS    lpParms = (void*)pmt;
    DWORD		end;
    LONG		bufsize, count, left;
    DWORD		dwRet;
    LPWAVEHDR		waveHdr = NULL;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    HANDLE		oldcb;
    int			whidx;

    TRACE("(%u, %08IX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    if (wmw->hFile == 0) {
	WARN("Can't play: no file=%s!\n", debugstr_w(wmw->lpFileName));
	return MCIERR_FILE_NOT_FOUND;
    }

    if (wmw->dwStatus == MCI_MODE_PAUSE && !wmw->fInput && !(dwFlags & (MCI_FROM | MCI_TO))) {
	/* FIXME: notification is different with Resume than Play */
	return WAVE_mciResume(wDevID, dwFlags, (LPMCI_GENERIC_PARMS)lpParms);
    }

    /** This function will be called again by a thread when async is used.
     * We have to set MCI_MODE_PLAY before we do this so that the app can spin
     * on MCI_STATUS, so we have to allow it here if we're not going to start this thread.
     */
    if ( !(wmw->dwStatus == MCI_MODE_STOP) &&
	!((wmw->dwStatus == MCI_MODE_PLAY) && (dwFlags & MCI_WAIT) && !wmw->hWave)) {
	/* FIXME: Check FROM/TO parameters first. */
	/* FIXME: Play; Play [notify|wait] must hook into the running player. */
	dwRet = WAVE_mciStop(wDevID, MCI_WAIT, NULL);
	if (dwRet) return dwRet;
    }

    if (wmw->lpWaveFormat->wFormatTag == WAVE_FORMAT_PCM) {
        if (wmw->lpWaveFormat->nBlockAlign !=
            wmw->lpWaveFormat->nChannels * wmw->lpWaveFormat->wBitsPerSample/8) {
            WARN("Incorrect nBlockAlign (%d), setting it to %d\n",
                wmw->lpWaveFormat->nBlockAlign,
                wmw->lpWaveFormat->nChannels *
                 wmw->lpWaveFormat->wBitsPerSample/8);
            wmw->lpWaveFormat->nBlockAlign =
                wmw->lpWaveFormat->nChannels *
                wmw->lpWaveFormat->wBitsPerSample/8;
        }
        if (wmw->lpWaveFormat->nAvgBytesPerSec !=
            wmw->lpWaveFormat->nSamplesPerSec * wmw->lpWaveFormat->nBlockAlign) {
            WARN("Incorrect nAvgBytesPerSec (%ld), setting it to %ld\n",
                wmw->lpWaveFormat->nAvgBytesPerSec,
                wmw->lpWaveFormat->nSamplesPerSec *
                 wmw->lpWaveFormat->nBlockAlign);
            wmw->lpWaveFormat->nAvgBytesPerSec =
                wmw->lpWaveFormat->nSamplesPerSec *
                wmw->lpWaveFormat->nBlockAlign;
        }
    }

    end = wmw->ckWaveData.cksize;
    if (dwFlags & MCI_TO) {
	DWORD position = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwTo);
	if (position > end)		return MCIERR_OUTOFRANGE;
	end = position;
    }
    if (dwFlags & MCI_FROM) {
	DWORD position = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwFrom);
	if (position > end)		return MCIERR_OUTOFRANGE;
	/* Seek rounds down, so do we. */
	position /= wmw->lpWaveFormat->nBlockAlign;
	position *= wmw->lpWaveFormat->nBlockAlign;
	wmw->dwPosition = position;
    }
    if (end < wmw->dwPosition) return MCIERR_OUTOFRANGE;
    left = end - wmw->dwPosition;
    if (0==left) return MMSYSERR_NOERROR; /* FIXME: NOTIFY */

    wmw->fInput = FALSE; /* FIXME: waveInOpen may have been called. */
    wmw->dwStatus = MCI_MODE_PLAY;

    if (!(dwFlags & MCI_WAIT)) {
	return MCI_SendCommandAsync(wDevID, WAVE_mciPlay, dwFlags,
				    (DWORD_PTR)lpParms, sizeof(MCI_PLAY_PARMS));
    }

    TRACE("Playing from byte=%lu to byte=%lu\n", wmw->dwPosition, end);

    oldcb = InterlockedExchangePointer(&wmw->hCallback,
	(dwFlags & MCI_NOTIFY) ? HWND_32(LOWORD(lpParms->dwCallback)) : NULL);
    if (oldcb) mciDriverNotify(oldcb, wDevID, MCI_NOTIFY_ABORTED);
    oldcb = NULL;

#define	WAVE_ALIGN_ON_BLOCK(wmw,v) \
((((v) + (wmw)->lpWaveFormat->nBlockAlign - 1) / (wmw)->lpWaveFormat->nBlockAlign) * (wmw)->lpWaveFormat->nBlockAlign)

    /* go back to beginning of chunk plus the requested position */
    /* FIXME: I'm not sure this is correct, notably because some data linked to
     * the decompression state machine will not be correctly initialized.
     * try it this way (other way would be to decompress from 0 up to dwPosition
     * and to start sending to hWave when dwPosition is reached)
     */
    mmioSeek(wmw->hFile, wmw->ckWaveData.dwDataOffset + wmw->dwPosition, SEEK_SET); /* >= 0 */

    dwRet = waveOutOpen((HWAVEOUT *)&wmw->hWave, wmw->wOutput, wmw->lpWaveFormat,
			(DWORD_PTR)WAVE_mciPlayCallback, (DWORD_PTR)wmw, CALLBACK_FUNCTION);

    if (dwRet != 0) {
	TRACE("Can't open low level audio device %ld\n", dwRet);
	dwRet = MCIERR_DEVICE_OPEN;
	wmw->hWave = 0;
	goto cleanUp;
    }

    /* make it so that 3 buffers per second are needed */
    bufsize = WAVE_ALIGN_ON_BLOCK(wmw, wmw->lpWaveFormat->nAvgBytesPerSec / 3);

    waveHdr = malloc(2 * sizeof(WAVEHDR) + 2 * bufsize);
    waveHdr[0].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR);
    waveHdr[1].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR) + bufsize;
    waveHdr[0].dwUser         = waveHdr[1].dwUser         = 0L;
    waveHdr[0].dwLoops        = waveHdr[1].dwLoops        = 0L;
    waveHdr[0].dwFlags        = waveHdr[1].dwFlags        = 0L;
    waveHdr[0].dwBufferLength = waveHdr[1].dwBufferLength = bufsize;
    if (waveOutPrepareHeader(wmw->hWave, &waveHdr[0], sizeof(WAVEHDR)) ||
	waveOutPrepareHeader(wmw->hWave, &waveHdr[1], sizeof(WAVEHDR))) {
	dwRet = MCIERR_INTERNAL;
	goto cleanUp;
    }

    whidx = 0;
    wmw->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    if (!wmw->hEvent) {
	dwRet = MCIERR_OUT_OF_MEMORY;
	goto cleanUp;
    }
    wmw->dwEventCount = 1L; /* for first buffer */

    TRACE("Playing (normalized) from byte=%lu for %lu bytes\n", wmw->dwPosition, left);
    if (hEvent) SetEvent(hEvent);

    /* FIXME: this doesn't work if wmw->dwPosition != 0 */
    while (left > 0 && wmw->dwStatus != MCI_MODE_STOP && wmw->dwStatus != MCI_MODE_NOT_READY) {
	count = mmioRead(wmw->hFile, waveHdr[whidx].lpData, min(bufsize, left));
	TRACE("mmioRead bufsize=%ld count=%ld\n", bufsize, count);
	if (count < 1)
	    break;
	/* count is always <= bufsize, so this is correct regarding the
	 * waveOutPrepareHeader function
	 */
	waveHdr[whidx].dwBufferLength = count;
	waveHdr[whidx].dwFlags &= ~WHDR_DONE;
	TRACE("before WODM_WRITE lpWaveHdr=%p dwBufferLength=%lu\n",
	      &waveHdr[whidx], waveHdr[whidx].dwBufferLength);
	dwRet = waveOutWrite(wmw->hWave, &waveHdr[whidx], sizeof(WAVEHDR));
	if (dwRet) {
	    ERR("Aborting play loop, WODM_WRITE error %ld\n", dwRet);
	    dwRet = MCIERR_HARDWARE;
	    break;
	}
	left -= count;
	wmw->dwPosition += count;
	TRACE("after WODM_WRITE dwPosition=%lu\n", wmw->dwPosition);
	/* InterlockedDecrement if and only if waveOutWrite is successful */
	WAVE_mciPlayWaitDone(wmw);
	whidx ^= 1;
    }

    WAVE_mciPlayWaitDone(wmw); /* to balance first buffer */

    /* just to get rid of some race conditions between play, stop and pause */
    waveOutReset(wmw->hWave);

    waveOutUnprepareHeader(wmw->hWave, &waveHdr[0], sizeof(WAVEHDR));
    waveOutUnprepareHeader(wmw->hWave, &waveHdr[1], sizeof(WAVEHDR));

cleanUp:
    if (dwFlags & MCI_NOTIFY)
	oldcb = InterlockedExchangePointer(&wmw->hCallback, NULL);

    free(waveHdr);

    if (wmw->hWave) {
	waveOutClose(wmw->hWave);
	wmw->hWave = 0;
    }
    CloseHandle(wmw->hEvent);
    wmw->hEvent = NULL;

    wmw->dwStatus = MCI_MODE_STOP;

    /* Let the potentially asynchronous commands support FAILURE notification. */
    if (oldcb) mciDriverNotify(oldcb, wDevID,
	dwRet ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL);

    return dwRet;
}

/**************************************************************************
 * 				WAVE_mciRecordCallback		[internal]
 */
static	void	CALLBACK WAVE_mciRecordCallback(HWAVEOUT hwo, UINT uMsg,
                                                DWORD_PTR dwInstance,
                                                LPARAM dwParam1, LPARAM dwParam2)
{
    WINE_MCIWAVE*	wmw = (WINE_MCIWAVE*)dwInstance;
    LPWAVEHDR           lpWaveHdr;
    LONG                count = 0;

    switch (uMsg) {
    case WIM_OPEN:
    case WIM_CLOSE:
	break;
    case WIM_DATA:
	lpWaveHdr = (LPWAVEHDR) dwParam1;

	InterlockedIncrement(&wmw->dwEventCount);

	count = mmioWrite(wmw->hFile, lpWaveHdr->lpData, lpWaveHdr->dwBytesRecorded);

	lpWaveHdr->dwFlags &= ~WHDR_DONE;
        if (count > 0)
            wmw->dwPosition  += count;
        /* else error reporting ?? */
        if (wmw->dwStatus == MCI_MODE_RECORD)
        {
           /* Only queue up another buffer if we are recording.  We could receive this
              message also when waveInReset() is called, since it notifies on all wave
              buffers that are outstanding.  Queueing up more sometimes causes waveInClose
              to fail. */
           waveInAddBuffer(wmw->hWave, lpWaveHdr, sizeof(*lpWaveHdr));
           TRACE("after mmioWrite dwPosition=%lu\n", wmw->dwPosition);
        }

	SetEvent(wmw->hEvent);
	break;
    default:
	ERR("Unknown uMsg=%d\n", uMsg);
    }
}

/******************************************************************
 *			WAVE_mciRecordWaitDone		[internal]
 */
static void WAVE_mciRecordWaitDone(WINE_MCIWAVE* wmw)
{
    for (;;) {
	ResetEvent(wmw->hEvent);
	if (InterlockedDecrement(&wmw->dwEventCount) >= 0) {
	    break;
	}
	InterlockedIncrement(&wmw->dwEventCount);

	WaitForSingleObject(wmw->hEvent, INFINITE);
    }
}

/**************************************************************************
 * 				WAVE_mciRecord			[internal]
 */
static DWORD WAVE_mciRecord(MCIDEVICEID wDevID, DWORD_PTR dwFlags, DWORD_PTR pmt, HANDLE hEvent)
{
    LPMCI_RECORD_PARMS  lpParms = (void*)pmt;
    DWORD		end;
    DWORD		dwRet = MMSYSERR_NOERROR;
    LONG		bufsize;
    LPWAVEHDR		waveHdr = NULL;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    HANDLE		oldcb;

    TRACE("(%u, %08IX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;

    if (wmw->dwStatus == MCI_MODE_PAUSE && wmw->fInput) {
        /* FIXME: parameters (start/end) in lpParams may not be used */
        return WAVE_mciResume(wDevID, dwFlags, (LPMCI_GENERIC_PARMS)lpParms);
    }

    /** This function will be called again by a thread when async is used.
     * We have to set MCI_MODE_RECORD before we do this so that the app can spin
     * on MCI_STATUS, so we have to allow it here if we're not going to start this thread.
     */
    if ( !(wmw->dwStatus == MCI_MODE_STOP) &&
	!((wmw->dwStatus == MCI_MODE_RECORD) && (dwFlags & MCI_WAIT) && !wmw->hWave)) {
	return MCIERR_INTERNAL;
    }

    wmw->fInput = TRUE; /* FIXME: waveOutOpen may have been called. */
    wmw->dwStatus = MCI_MODE_RECORD;

    if (!(dwFlags & MCI_WAIT)) {
	return MCI_SendCommandAsync(wDevID, WAVE_mciRecord, dwFlags,
				    (DWORD_PTR)lpParms, sizeof(MCI_RECORD_PARMS));
    }

    /* FIXME: we only re-create the RIFF structure from an existing file (if any)
     * we don't modify the wave part of an existing file (ie. we always erase an
     * existing content, we don't overwrite)
     */
    free(wmw->lpFileName);
    dwRet = create_tmp_file(&wmw->hFile, (WCHAR**)&wmw->lpFileName);
    if (dwRet != 0) return dwRet;

    /* new RIFF file, lpWaveFormat now valid */
    dwRet = WAVE_mciCreateRIFFSkeleton(wmw);
    if (dwRet != 0) return dwRet;

    if (dwFlags & MCI_TO) {
	end = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwTo);
    } else end = 0xFFFFFFFF;
    if (dwFlags & MCI_FROM) {
	DWORD position = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwFrom);
	if (wmw->ckWaveData.cksize < position)	return MCIERR_OUTOFRANGE;
	/* Seek rounds down, so do we. */
	position /= wmw->lpWaveFormat->nBlockAlign;
	position *= wmw->lpWaveFormat->nBlockAlign;
	wmw->dwPosition = position;
    }
    if (end==wmw->dwPosition) return MMSYSERR_NOERROR; /* FIXME: NOTIFY */

    TRACE("Recording from byte=%lu to byte=%lu\n", wmw->dwPosition, end);

    oldcb = InterlockedExchangePointer(&wmw->hCallback,
	(dwFlags & MCI_NOTIFY) ? HWND_32(LOWORD(lpParms->dwCallback)) : NULL);
    if (oldcb) mciDriverNotify(oldcb, wDevID, MCI_NOTIFY_ABORTED);
    oldcb = NULL;

#define	WAVE_ALIGN_ON_BLOCK(wmw,v) \
((((v) + (wmw)->lpWaveFormat->nBlockAlign - 1) / (wmw)->lpWaveFormat->nBlockAlign) * (wmw)->lpWaveFormat->nBlockAlign)

    wmw->ckWaveData.cksize = WAVE_ALIGN_ON_BLOCK(wmw, wmw->ckWaveData.cksize);

    /* Go back to the beginning of the chunk plus the requested position */
    /* FIXME: I'm not sure this is correct, notably because some data linked to
     * the decompression state machine will not be correctly initialized.
     * Try it this way (other way would be to decompress from 0 up to dwPosition
     * and to start sending to hWave when dwPosition is reached).
     */
    mmioSeek(wmw->hFile, wmw->ckWaveData.dwDataOffset + wmw->dwPosition, SEEK_SET); /* >= 0 */

    dwRet = waveInOpen((HWAVEIN*)&wmw->hWave, wmw->wInput, wmw->lpWaveFormat,
			(DWORD_PTR)WAVE_mciRecordCallback, (DWORD_PTR)wmw, CALLBACK_FUNCTION);

    if (dwRet != MMSYSERR_NOERROR) {
	TRACE("Can't open low level audio device %ld\n", dwRet);
	dwRet = MCIERR_DEVICE_OPEN;
	wmw->hWave = 0;
	goto cleanUp;
    }

    /* make it so that 3 buffers per second are needed */
    bufsize = WAVE_ALIGN_ON_BLOCK(wmw, wmw->lpWaveFormat->nAvgBytesPerSec / 3);

    waveHdr = malloc(2 * sizeof(WAVEHDR) + 2 * bufsize);
    waveHdr[0].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR);
    waveHdr[1].lpData = (char*)waveHdr + 2 * sizeof(WAVEHDR) + bufsize;
    waveHdr[0].dwUser         = waveHdr[1].dwUser         = 0L;
    waveHdr[0].dwLoops        = waveHdr[1].dwLoops        = 0L;
    waveHdr[0].dwFlags        = waveHdr[1].dwFlags        = 0L;
    waveHdr[0].dwBufferLength = waveHdr[1].dwBufferLength = bufsize;

    if (waveInPrepareHeader(wmw->hWave, &waveHdr[0], sizeof(WAVEHDR)) ||
	waveInPrepareHeader(wmw->hWave, &waveHdr[1], sizeof(WAVEHDR))) {
	dwRet = MCIERR_INTERNAL;
	goto cleanUp;
    }

    if (waveInAddBuffer(wmw->hWave, &waveHdr[0], sizeof(WAVEHDR)) ||
	waveInAddBuffer(wmw->hWave, &waveHdr[1], sizeof(WAVEHDR))) {
	dwRet = MCIERR_INTERNAL;
	goto cleanUp;
    }

    wmw->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wmw->dwEventCount = 1L; /* for first buffer */

    TRACE("Recording (normalized) from byte=%lu for %lu bytes\n", wmw->dwPosition, end - wmw->dwPosition);

    waveInStart(wmw->hWave);

    if (hEvent) SetEvent(hEvent);

    while (wmw->dwPosition < end && wmw->dwStatus != MCI_MODE_STOP && wmw->dwStatus != MCI_MODE_NOT_READY) {
	WAVE_mciRecordWaitDone(wmw);
    }
    /* Grab callback before another thread kicks in after we change dwStatus. */
    if (dwFlags & MCI_NOTIFY) {
	oldcb = InterlockedExchangePointer(&wmw->hCallback, NULL);
	dwFlags &= ~MCI_NOTIFY;
    }
    /* needed so that the callback above won't add again the buffers returned by the reset */
    wmw->dwStatus = MCI_MODE_STOP;

    waveInReset(wmw->hWave);

    waveInUnprepareHeader(wmw->hWave, &waveHdr[0], sizeof(WAVEHDR));
    waveInUnprepareHeader(wmw->hWave, &waveHdr[1], sizeof(WAVEHDR));

    dwRet = 0;

cleanUp:
    if (dwFlags & MCI_NOTIFY)
	oldcb = InterlockedExchangePointer(&wmw->hCallback, NULL);

    free(waveHdr);

    if (wmw->hWave) {
	waveInClose(wmw->hWave);
	wmw->hWave = 0;
    }
    CloseHandle(wmw->hEvent);

    wmw->dwStatus = MCI_MODE_STOP;

    if (oldcb) mciDriverNotify(oldcb, wDevID,
	dwRet ? MCI_NOTIFY_FAILURE : MCI_NOTIFY_SUCCESSFUL);

    return dwRet;

}

/**************************************************************************
 * 				WAVE_mciPause			[internal]
 */
static DWORD WAVE_mciPause(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    DWORD 		dwRet;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;

    switch (wmw->dwStatus) {
    case MCI_MODE_PLAY:
	dwRet = waveOutPause(wmw->hWave);
	if (dwRet==MMSYSERR_NOERROR) wmw->dwStatus = MCI_MODE_PAUSE;
	else { /* When playthread was not started yet, winmm not opened, error 5 MMSYSERR_INVALHANDLE */
	    ERR("waveOutPause error %ld\n",dwRet);
	    dwRet = MCIERR_INTERNAL;
	}
	break;
    case MCI_MODE_RECORD:
	dwRet = waveInStop(wmw->hWave);
	if (dwRet==MMSYSERR_NOERROR) wmw->dwStatus = MCI_MODE_PAUSE;
	else {
	    ERR("waveInStop error %ld\n",dwRet);
	    dwRet = MCIERR_INTERNAL;
	}
	break;
    case MCI_MODE_PAUSE:
	dwRet = MMSYSERR_NOERROR;
	break;
    default:
	dwRet = MCIERR_NONAPPLICABLE_FUNCTION;
    }
    if (MMSYSERR_NOERROR==dwRet && (dwFlags & MCI_NOTIFY) && lpParms)
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);
    return dwRet;
}

/**************************************************************************
 * 				WAVE_mciResume			[internal]
 */
static DWORD WAVE_mciResume(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_GENERIC_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		dwRet;

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;

    switch (wmw->dwStatus) {
    case MCI_MODE_PAUSE:
	/* Only update dwStatus if wave* succeeds and will exchange buffers buffers. */
	if (wmw->fInput) {
	    dwRet = waveInStart(wmw->hWave);
	    if (dwRet==MMSYSERR_NOERROR) wmw->dwStatus = MCI_MODE_RECORD;
	    else {
		ERR("waveInStart error %ld\n",dwRet);
		dwRet = MCIERR_INTERNAL;
	    }
	} else {
	    dwRet = waveOutRestart(wmw->hWave);
	    if (dwRet==MMSYSERR_NOERROR) wmw->dwStatus = MCI_MODE_PLAY;
	    else {
		ERR("waveOutRestart error %ld\n",dwRet);
		dwRet = MCIERR_INTERNAL;
	    }
	}
	break;
    case MCI_MODE_PLAY:
    case MCI_MODE_RECORD:
	dwRet = MMSYSERR_NOERROR;
	break;
    default:
	dwRet = MCIERR_NONAPPLICABLE_FUNCTION;
    }
    if (MMSYSERR_NOERROR==dwRet && (dwFlags & MCI_NOTIFY) && lpParms)
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);
    return dwRet;
}

/**************************************************************************
 * 				WAVE_mciSeek			[internal]
 */
static DWORD WAVE_mciSeek(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_SEEK_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		position, dwRet;

    TRACE("(%04X, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;

    position = dwFlags & (MCI_SEEK_TO_START|MCI_SEEK_TO_END|MCI_TO);
    if (!position)		return MCIERR_MISSING_PARAMETER;
    if (position&(position-1))	return MCIERR_FLAGS_NOT_COMPATIBLE;

    /* Stop sends MCI_NOTIFY_ABORTED when needed */
    dwRet = WAVE_mciStop(wDevID, MCI_WAIT, 0);
    if (dwRet != MMSYSERR_NOERROR) return dwRet;

    if (dwFlags & MCI_TO) {
	position = WAVE_ConvertTimeFormatToByte(wmw, lpParms->dwTo);
	if (position > wmw->ckWaveData.cksize)
	    return MCIERR_OUTOFRANGE;
    } else if (dwFlags & MCI_SEEK_TO_START) {
	position = 0;
    } else {
	position = wmw->ckWaveData.cksize;
    }
    /* Seek rounds down, unless at end */
    if (position != wmw->ckWaveData.cksize) {
	position /= wmw->lpWaveFormat->nBlockAlign;
	position *= wmw->lpWaveFormat->nBlockAlign;
    }
    wmw->dwPosition = position;
    TRACE("Seeking to position=%lu bytes\n", position);

    if (dwFlags & MCI_NOTIFY)
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);

    return MMSYSERR_NOERROR;
}

/**************************************************************************
 * 				WAVE_mciSet			[internal]
 */
static DWORD WAVE_mciSet(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_WAVE_SET_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_SET_TIME_FORMAT) {
	switch (lpParms->dwTimeFormat) {
	case MCI_FORMAT_MILLISECONDS:
	    TRACE("MCI_FORMAT_MILLISECONDS !\n");
	    wmw->dwMciTimeFormat = MCI_FORMAT_MILLISECONDS;
	    break;
	case MCI_FORMAT_BYTES:
	    TRACE("MCI_FORMAT_BYTES !\n");
	    wmw->dwMciTimeFormat = MCI_FORMAT_BYTES;
	    break;
	case MCI_FORMAT_SAMPLES:
	    TRACE("MCI_FORMAT_SAMPLES !\n");
	    wmw->dwMciTimeFormat = MCI_FORMAT_SAMPLES;
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
    if (dwFlags & MCI_WAVE_INPUT) {
	TRACE("MCI_WAVE_INPUT = %d\n", lpParms->wInput);
	if (lpParms->wInput >= waveInGetNumDevs())
	    return MCIERR_OUTOFRANGE;
	if (wmw->wInput != (WORD)lpParms->wInput)
	    WAVE_mciStop(wDevID, MCI_WAIT, NULL);
	wmw->wInput = lpParms->wInput;
    }
    if (dwFlags & MCI_WAVE_OUTPUT) {
	TRACE("MCI_WAVE_OUTPUT = %d\n", lpParms->wOutput);
	if (lpParms->wOutput >= waveOutGetNumDevs())
	    return MCIERR_OUTOFRANGE;
	if (wmw->wOutput != (WORD)lpParms->wOutput)
	    WAVE_mciStop(wDevID, MCI_WAIT, NULL);
	wmw->wOutput = lpParms->wOutput;
    }
    if (dwFlags & MCI_WAVE_SET_ANYINPUT) {
	TRACE("MCI_WAVE_SET_ANYINPUT\n");
	if (wmw->wInput != (WORD)lpParms->wInput)
	    WAVE_mciStop(wDevID, MCI_WAIT, NULL);
	wmw->wInput = WAVE_MAPPER;
    }
    if (dwFlags & MCI_WAVE_SET_ANYOUTPUT) {
	TRACE("MCI_WAVE_SET_ANYOUTPUT\n");
	if (wmw->wOutput != (WORD)lpParms->wOutput)
	    WAVE_mciStop(wDevID, MCI_WAIT, NULL);
	wmw->wOutput = WAVE_MAPPER;
    }
    /* Set wave format parameters is refused after Open or Record.*/
    if (dwFlags & MCI_WAVE_SET_FORMATTAG) {
	TRACE("MCI_WAVE_SET_FORMATTAG = %d\n", lpParms->wFormatTag);
	if (wmw->lpWaveFormat != &wmw->wfxRef) return MCIERR_NONAPPLICABLE_FUNCTION;
	if (lpParms->wFormatTag != WAVE_FORMAT_PCM)
	    return MCIERR_OUTOFRANGE;
    }
    if (dwFlags & MCI_WAVE_SET_AVGBYTESPERSEC) {
	if (wmw->lpWaveFormat != &wmw->wfxRef) return MCIERR_NONAPPLICABLE_FUNCTION;
	wmw->wfxRef.nAvgBytesPerSec = lpParms->nAvgBytesPerSec;
	TRACE("MCI_WAVE_SET_AVGBYTESPERSEC = %ld\n", wmw->wfxRef.nAvgBytesPerSec);
    }
    if (dwFlags & MCI_WAVE_SET_BITSPERSAMPLE) {
	if (wmw->lpWaveFormat != &wmw->wfxRef) return MCIERR_NONAPPLICABLE_FUNCTION;
	wmw->wfxRef.wBitsPerSample = lpParms->wBitsPerSample;
	TRACE("MCI_WAVE_SET_BITSPERSAMPLE = %d\n", wmw->wfxRef.wBitsPerSample);
    }
    if (dwFlags & MCI_WAVE_SET_BLOCKALIGN) {
	if (wmw->lpWaveFormat != &wmw->wfxRef) return MCIERR_NONAPPLICABLE_FUNCTION;
	wmw->wfxRef.nBlockAlign = lpParms->nBlockAlign;
	TRACE("MCI_WAVE_SET_BLOCKALIGN = %d\n", wmw->wfxRef.nBlockAlign);
    }
    if (dwFlags & MCI_WAVE_SET_CHANNELS) {
	if (wmw->lpWaveFormat != &wmw->wfxRef) return MCIERR_NONAPPLICABLE_FUNCTION;
	wmw->wfxRef.nChannels = lpParms->nChannels;
	TRACE("MCI_WAVE_SET_CHANNELS = %d\n", wmw->wfxRef.nChannels);
    }
    if (dwFlags & MCI_WAVE_SET_SAMPLESPERSEC) {
	if (wmw->lpWaveFormat != &wmw->wfxRef) return MCIERR_NONAPPLICABLE_FUNCTION;
	wmw->wfxRef.nSamplesPerSec = lpParms->nSamplesPerSec;
	TRACE("MCI_WAVE_SET_SAMPLESPERSEC = %ld\n", wmw->wfxRef.nSamplesPerSec);
    }
    if (dwFlags & MCI_NOTIFY)
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);
    return 0;
}

/**************************************************************************
 *				WAVE_mciSave		[internal]
 */
static DWORD WAVE_mciSave(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_SAVE_PARMSW lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		ret = MCIERR_FILE_NOT_SAVED, tmpRet;

    TRACE("%d, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw     == NULL)	return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_WAIT)
    {
    	FIXME("MCI_WAIT not implemented\n");
    }
    WAVE_mciStop(wDevID, 0, NULL);

    ret = mmioAscend(wmw->hFile, &wmw->ckWaveData, 0);
    ret = mmioAscend(wmw->hFile, &wmw->ckMainRIFF, 0);

    ret = mmioClose(wmw->hFile, 0);
    wmw->hFile = 0;

    /*
      If the destination file already exists, it has to be overwritten.  (Behaviour
      verified in Windows (2000)).  If it doesn't overwrite, it is breaking one of
      my applications.  We are making use of mmioRename, which WILL NOT overwrite
      the destination file (which is what Windows does, also verified in Win2K)
      So, lets delete the destination file before calling mmioRename.  If the
      destination file DOESN'T exist, the delete will fail silently.  Let's also be
      careful not to lose our previous error code.
    */
    tmpRet = GetLastError();
    DeleteFileW (lpParms->lpfilename);
    SetLastError(tmpRet);

    /* FIXME: Open file.wav; Save; must not rename the original file.
     * Nor must Save a.wav; Save b.wav rename a. */
    if (0 == mmioRenameW(wmw->lpFileName, lpParms->lpfilename, 0, 0 )) {
	ret = MMSYSERR_NOERROR;
    }

    if (MMSYSERR_NOERROR==ret && (dwFlags & MCI_NOTIFY))
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);

    if (ret == MMSYSERR_NOERROR)
        ret = WAVE_mciOpenFile(wmw, lpParms->lpfilename);

    return ret;
}

/**************************************************************************
 * 				WAVE_mciStatus		[internal]
 */
static DWORD WAVE_mciStatus(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_STATUS_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		ret = 0;

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);
    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;
    if (!(dwFlags & MCI_STATUS_ITEM))	return MCIERR_MISSING_PARAMETER;

    if (dwFlags & MCI_STATUS_ITEM) {
	switch (lpParms->dwItem) {
	case MCI_STATUS_CURRENT_TRACK:
	    lpParms->dwReturn = 1;
            TRACE("MCI_STATUS_CURRENT_TRACK => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_LENGTH:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = WAVE_ConvertByteToTimeFormat(wmw, wmw->ckWaveData.cksize);
            TRACE("MCI_STATUS_LENGTH => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_MODE:
	    TRACE("MCI_STATUS_MODE => %u\n", wmw->dwStatus);
	    lpParms->dwReturn = MAKEMCIRESOURCE(wmw->dwStatus, wmw->dwStatus);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_MEDIA_PRESENT:
	    TRACE("MCI_STATUS_MEDIA_PRESENT => TRUE!\n");
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_NUMBER_OF_TRACKS:
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = 1;
            TRACE("MCI_STATUS_NUMBER_OF_TRACKS => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_STATUS_POSITION:
	    if (!wmw->hFile) {
		lpParms->dwReturn = 0;
		return MCIERR_UNSUPPORTED_FUNCTION;
	    }
	    /* only one track in file is currently handled, so don't take care of MCI_TRACK flag */
	    lpParms->dwReturn = WAVE_ConvertByteToTimeFormat(wmw,
							     (dwFlags & MCI_STATUS_START) ? 0 : wmw->dwPosition);
            TRACE("MCI_STATUS_POSITION %s => %Iu\n",
		  (dwFlags & MCI_STATUS_START) ? "start" : "current", lpParms->dwReturn);
	    break;
	case MCI_STATUS_READY:
	    lpParms->dwReturn = (wmw->dwStatus == MCI_MODE_NOT_READY) ?
		MAKEMCIRESOURCE(FALSE, MCI_FALSE) : MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    TRACE("MCI_STATUS_READY => %u!\n", LOWORD(lpParms->dwReturn));
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_STATUS_TIME_FORMAT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(wmw->dwMciTimeFormat, MCI_FORMAT_RETURN_BASE + wmw->dwMciTimeFormat);
            TRACE("MCI_STATUS_TIME_FORMAT => %Iu\n", lpParms->dwReturn);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_WAVE_INPUT:
	    if (wmw->wInput != (WORD)WAVE_MAPPER)
		lpParms->dwReturn = wmw->wInput;
	    else {
		lpParms->dwReturn = MAKEMCIRESOURCE(WAVE_MAPPER, WAVE_MAPPER_S);
		ret = MCI_RESOURCE_RETURNED;
	    }
	    TRACE("MCI_WAVE_INPUT => %d\n", (signed)wmw->wInput);
	    break;
	case MCI_WAVE_OUTPUT:
	    if (wmw->wOutput != (WORD)WAVE_MAPPER)
		lpParms->dwReturn = wmw->wOutput;
	    else {
		lpParms->dwReturn = MAKEMCIRESOURCE(WAVE_MAPPER, WAVE_MAPPER_S);
		ret = MCI_RESOURCE_RETURNED;
	    }
	    TRACE("MCI_WAVE_OUTPUT => %d\n", (signed)wmw->wOutput);
	    break;
	/* It is always ok to query wave format parameters,
	 * except on auto-open yield MCIERR_UNSUPPORTED_FUNCTION. */
	case MCI_WAVE_STATUS_FORMATTAG:
	    if (wmw->lpWaveFormat->wFormatTag != WAVE_FORMAT_PCM)
		lpParms->dwReturn = wmw->lpWaveFormat->wFormatTag;
	    else {
		lpParms->dwReturn = MAKEMCIRESOURCE(WAVE_FORMAT_PCM, WAVE_FORMAT_PCM_S);
		ret = MCI_RESOURCE_RETURNED;
	    }
	    TRACE("MCI_WAVE_FORMATTAG => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_AVGBYTESPERSEC:
	    lpParms->dwReturn = wmw->lpWaveFormat->nAvgBytesPerSec;
	    TRACE("MCI_WAVE_STATUS_AVGBYTESPERSEC => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_BITSPERSAMPLE:
	    lpParms->dwReturn = wmw->lpWaveFormat->wBitsPerSample;
	    TRACE("MCI_WAVE_STATUS_BITSPERSAMPLE => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_BLOCKALIGN:
	    lpParms->dwReturn = wmw->lpWaveFormat->nBlockAlign;
	    TRACE("MCI_WAVE_STATUS_BLOCKALIGN => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_CHANNELS:
	    lpParms->dwReturn = wmw->lpWaveFormat->nChannels;
	    TRACE("MCI_WAVE_STATUS_CHANNELS => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_SAMPLESPERSEC:
	    lpParms->dwReturn = wmw->lpWaveFormat->nSamplesPerSec;
	    TRACE("MCI_WAVE_STATUS_SAMPLESPERSEC => %Iu\n", lpParms->dwReturn);
	    break;
	case MCI_WAVE_STATUS_LEVEL:
	    TRACE("MCI_WAVE_STATUS_LEVEL !\n");
	    lpParms->dwReturn = 0xAAAA5555;
	    break;
	default:
            WARN("unknown command %08lX !\n", lpParms->dwItem);
	    return MCIERR_UNSUPPORTED_FUNCTION;
	}
    }
    if ((dwFlags & MCI_NOTIFY) && HRESULT_CODE(ret)==0)
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				WAVE_mciGetDevCaps		[internal]
 */
static DWORD WAVE_mciGetDevCaps(MCIDEVICEID wDevID, DWORD dwFlags,
				LPMCI_GETDEVCAPS_PARMS lpParms)
{
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);
    DWORD		ret = 0;

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (lpParms == NULL)	return MCIERR_NULL_PARAMETER_BLOCK;
    if (wmw == NULL)		return MCIERR_INVALID_DEVICE_ID;

    if (dwFlags & MCI_GETDEVCAPS_ITEM) {
	switch(lpParms->dwItem) {
	case MCI_GETDEVCAPS_DEVICE_TYPE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(MCI_DEVTYPE_WAVEFORM_AUDIO, MCI_DEVTYPE_WAVEFORM_AUDIO);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_AUDIO:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_HAS_VIDEO:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_USES_FILES:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_COMPOUND_DEVICE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_RECORD:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_EJECT:
	    lpParms->dwReturn = MAKEMCIRESOURCE(FALSE, MCI_FALSE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_PLAY:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_GETDEVCAPS_CAN_SAVE:
	    lpParms->dwReturn = MAKEMCIRESOURCE(TRUE, MCI_TRUE);
	    ret = MCI_RESOURCE_RETURNED;
	    break;
	case MCI_WAVE_GETDEVCAPS_INPUTS:
	    lpParms->dwReturn = waveInGetNumDevs();
	    break;
	case MCI_WAVE_GETDEVCAPS_OUTPUTS:
	    lpParms->dwReturn = waveOutGetNumDevs();
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
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				WAVE_mciInfo			[internal]
 */
static DWORD WAVE_mciInfo(MCIDEVICEID wDevID, DWORD dwFlags, LPMCI_INFO_PARMSW lpParms)
{
    DWORD		ret = 0;
    LPCWSTR		str = 0;
    WINE_MCIWAVE*	wmw = WAVE_mciGetOpenDev(wDevID);

    TRACE("(%u, %08lX, %p);\n", wDevID, dwFlags, lpParms);

    if (!lpParms || !lpParms->lpstrReturn)
	return MCIERR_NULL_PARAMETER_BLOCK;

    TRACE("buf=%p, len=%lu\n", lpParms->lpstrReturn, lpParms->dwRetSize);

    if (wmw == NULL) {
	ret = MCIERR_INVALID_DEVICE_ID;
    } else {
	switch (dwFlags & ~(MCI_WAIT|MCI_NOTIFY)) {
        case MCI_INFO_PRODUCT: str = L"Wine's audio player"; break;
	case MCI_INFO_FILE:    str = wmw->lpFileName; break;
        case MCI_WAVE_INPUT:   str = L"Wine Wave In"; break;
        case MCI_WAVE_OUTPUT:  str = L"Wine Wave Out"; break;
	default:
            WARN("Don't know this info command (%lu)\n", dwFlags);
	    ret = MCIERR_UNRECOGNIZED_KEYWORD;
	}
    }
    if (!ret) {
	if (lpParms->dwRetSize) {
	    /* FIXME? Since NT, mciwave, mciseq and mcicda set dwRetSize
	     *        to the number of characters written, excluding \0. */
            lstrcpynW(lpParms->lpstrReturn, str ? str : L"", lpParms->dwRetSize);
	} else ret = MCIERR_PARAM_OVERFLOW;
    }
    if (MMSYSERR_NOERROR==ret && (dwFlags & MCI_NOTIFY))
	WAVE_mciNotify(lpParms->dwCallback, wmw, MCI_NOTIFY_SUCCESSFUL);
    return ret;
}

/**************************************************************************
 * 				DriverProc (MCIWAVE.@)
 */
LRESULT CALLBACK MCIWAVE_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                    LPARAM dwParam1, LPARAM dwParam2)
{
    TRACE("(%08IX, %p, %08X, %08IX, %08IX)\n",
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    switch (wMsg) {
    case DRV_LOAD:		return 1;
    case DRV_FREE:		return 1;
    case DRV_OPEN:		return WAVE_drvOpen((LPCWSTR)dwParam1, (LPMCI_OPEN_DRIVER_PARMSW)dwParam2);
    case DRV_CLOSE:		return WAVE_drvClose(dwDevID);
    case DRV_ENABLE:		return 1;
    case DRV_DISABLE:		return 1;
    case DRV_QUERYCONFIGURE:	return 1;
    case DRV_CONFIGURE:		MessageBoxA(0, "MCI waveaudio Driver !", "Wine Driver", MB_OK);	return 1;
    case DRV_INSTALL:		return DRVCNF_RESTART;
    case DRV_REMOVE:		return DRVCNF_RESTART;
    }

    if (dwDevID == 0xFFFFFFFF) return MCIERR_UNSUPPORTED_FUNCTION;

    switch (wMsg) {
    case MCI_OPEN_DRIVER:	return WAVE_mciOpen      (dwDevID, dwParam1, (LPMCI_WAVE_OPEN_PARMSW)  dwParam2);
    case MCI_CLOSE_DRIVER:	return WAVE_mciClose     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_CUE:		return WAVE_mciCue       (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_PLAY:		return WAVE_mciPlay      (dwDevID, dwParam1, dwParam2, NULL);
    case MCI_RECORD:		return WAVE_mciRecord    (dwDevID, dwParam1, dwParam2, NULL);
    case MCI_STOP:		return WAVE_mciStop      (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_SET:		return WAVE_mciSet       (dwDevID, dwParam1, (LPMCI_WAVE_SET_PARMS)    dwParam2);
    case MCI_PAUSE:		return WAVE_mciPause     (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_RESUME:		return WAVE_mciResume    (dwDevID, dwParam1, (LPMCI_GENERIC_PARMS)     dwParam2);
    case MCI_STATUS:		return WAVE_mciStatus    (dwDevID, dwParam1, (LPMCI_STATUS_PARMS)      dwParam2);
    case MCI_GETDEVCAPS:	return WAVE_mciGetDevCaps(dwDevID, dwParam1, (LPMCI_GETDEVCAPS_PARMS)  dwParam2);
    case MCI_INFO:		return WAVE_mciInfo      (dwDevID, dwParam1, (LPMCI_INFO_PARMSW)       dwParam2);
    case MCI_SEEK:		return WAVE_mciSeek      (dwDevID, dwParam1, (LPMCI_SEEK_PARMS)        dwParam2);
    case MCI_SAVE:		return WAVE_mciSave	 (dwDevID, dwParam1, (LPMCI_SAVE_PARMSW)       dwParam2);
	/* commands that should be supported */
    case MCI_LOAD:
    case MCI_FREEZE:
    case MCI_PUT:
    case MCI_REALIZE:
    case MCI_UNFREEZE:
    case MCI_UPDATE:
    case MCI_WHERE:
    case MCI_STEP:
    case MCI_SPIN:
    case MCI_ESCAPE:
    case MCI_COPY:
    case MCI_CUT:
    case MCI_DELETE:
    case MCI_PASTE:
	FIXME("Unsupported command [%u]\n", wMsg);
	break;
    case MCI_WINDOW:
	TRACE("Unsupported command [%u]\n", wMsg);
	break;
	/* option which can be silenced */
    case MCI_CONFIGURE:
	return 0;
    case MCI_OPEN:
    case MCI_CLOSE:
	ERR("Shouldn't receive a MCI_OPEN or CLOSE message\n");
	break;
    default:
	FIXME("is probably wrong msg [%u]\n", wMsg);
	return DefDriverProc(dwDevID, hDriv, wMsg, dwParam1, dwParam2);
    }
    return MCIERR_UNRECOGNIZED_COMMAND;
}
