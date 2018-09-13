/******************************************************************************

   Copyright (C) Microsoft Corporation 1991-1992. All rights reserved.

   Title:   avisound.c - Code for playing audio in AVI files.

*****************************************************************************/
#include "graphic.h"

#define AUDIO_PANIC 10
static UINT nAudioPanic;

//
// redefine StreamFromFOURCC to only handle 0-9 streams!
//
#undef StreamFromFOURCC
#define StreamFromFOURCC(fcc) (UINT)(HIBYTE(LOWORD(fcc)) - (BYTE)'0')

void FAR PASCAL _LOADDS mciaviWaveOutFunc(HWAVEOUT hWaveOut, UINT wMsg,
		    DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

#ifndef _WIN32
#define GetDS() (HGLOBAL)HIWORD((DWORD)(LPVOID)&ghModule)
#endif //WIN16

#ifdef REMOTESTEAL
#if 0 // COMMENTARY
    Wave devices can only be used by one task at a time.  When there are
    multiple processes involved we need to have some way of passing the
    wave device around between processes.  Because each process executes
    asynchronously we choose an asynchronous method of passing the wave
    device.

    When we start playing an attempt is made to open the wave device.  If
    successful we write our playback window handle to a known (volatile)
    area of the registry.

    When we stop playing and release the wave device we clear the registry.

    If we cannot open a wave device, we inspect the registry to see if
    another process has it open.  If YES, then we post a WM_AUDIO_OFF message
    to the registered window handle, passing our playback window handle as
    a parameter.  We then continue to play silently.

    Receiving WM_AUDIO_OFF:
	if we are playing with a wave device we stop temporarily, post a
	message to the task wanting the wave device along with our window
	handle (so that the wave device can be returned to us).

    When we have finished with the wave device we check hwndLostAudio to
    see if there is a window we should tell to pick up the wave device.
    If so, we post WM_AUDIO_ON to that window.

#endif// COMMENTARY

// hwndLostAudio is set to the window handle of the device from which we
//	stole the audio device.  It is passed as a parameter on the
//	WM_AUDIO_ON message to tell us to whom we should return the
//	wave device when we have finished with it.

// hwndWantAudio contains transitory state.  It is set on receipt of a
//	WM_AUDIO_OFF message to tell us the window that wants to grab the
//	wave device.  If we are playing, we temporarily stop which causes
//	the wave device to be released.  Immediately the wave device is
//	released we post a message to hwndWantAudio and reset the variable.
//	If we receive a WM_AUDIO_OFF message while we are not playing (which
//	can happen due to asynchronous execution) hwndWantAudio is not set.

HWND hwndLostAudio=0;  // Window handle of the window who gave up audio
HWND hwndWantAudio=0;  // Window handle of the window which wants the audio

HKEY	hkey = 0;  // Handle to area of registry for inter process steal

#define REMOTEWAVEHWND  TEXT("RemoteWaveWindow")
#define WAVEOWNER       TEXT("WaveOwner")

TCHAR szRemoteWaveHwnd[] = REMOTEWAVEHWND;
TCHAR szWaveOwner[] = WAVEOWNER;
#include <profile.key>
#endif

/******************************************************************************
*****************************************************************************/

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | SetUpAudio | set up wave device and associated data
 *
 * @parm NPMCIGRAPHIC | npMCI | near ptr to the instance data
 *
 * @parm BOOL | fPlaying |
 *
 ***************************************************************************/

DWORD FAR PASCAL SetUpAudio(NPMCIGRAPHIC npMCI, BOOL fPlaying)
{
    UINT	w;
    DWORD	dw;
    LPWAVEHDR   lpWaveHdr;
    STREAMINFO *psi;

    if (npMCI->nAudioStreams == 0) {
	npMCI->wABs = 0;
	npMCI->wABOptimal = 0;
	return 0L;
    }

    nAudioPanic = mmGetProfileInt(szIni, TEXT("AudioPanic"), AUDIO_PANIC);

    psi = SI(npMCI->nAudioStream);
    Assert(psi->sh.fccType == streamtypeAUDIO);
    Assert(psi->cbFormat);
    Assert(psi->lpFormat);

    if (!npMCI->pWF) {
        npMCI->pWF = (NPWAVEFORMAT)LocalAlloc(LPTR, (UINT)psi->cbFormat);

	if (!npMCI->pWF) {
	    return MCIERR_OUT_OF_MEMORY;
	}
    }

    hmemcpy(npMCI->pWF,psi->lpFormat,psi->cbFormat);

    npMCI->wEarlyAudio = (UINT)psi->sh.dwInitialFrames;
    DPF1(("Setting up audio... wEarlyAudio==%d\n", npMCI->wEarlyAudio));
    npMCI->dwAudioLength = psi->sh.dwLength * psi->sh.dwSampleSize;

    if (npMCI->dwAudioLength < 1000L) {
        DPF(("AudioLength is bogus"));
        npMCI->dwAudioLength = muldiv32((npMCI->pWF->nAvgBytesPerSec + 100) *
                npMCI->lFrames,npMCI->dwMicroSecPerFrame,1000000L);
    }

    if (!fPlaying) {
	// We are not actually playing.  Check to see if we will be able
	// to handle the format of the wave data.
        if (IsNTWOW()) {
            /* See if we can cope with this wave format. */
            w = waveOutOpen(&npMCI->hWave, (UINT)WAVE_MAPPER,
        		(const LPWAVEFORMATEX) npMCI->pWF,
        		(DWORD_PTR) NULL,
        		(DWORD_PTR) (LPMCIGRAPHIC) npMCI,
                        (DWORD) WAVE_FORMAT_QUERY);
	    if (WAVERR_BADFORMAT == w)
		return(MCIERR_WAVE_OUTPUTSUNSUITABLE);	// Try 16 bit codec
	    else
		return(0);  // We might be able to cope with this format
        }
    }

    //
    // choose the audio playback method depending on how we are going to
    // receive audio data from the file.
    //
    switch (npMCI->wPlaybackAlg) {
        case MCIAVI_ALG_HARDDISK:
	case MCIAVI_ALG_AUDIOONLY:

            if (!npMCI->pf && !npMCI->hmmioAudio) {
                MMIOINFO            mmioInfo;

                _fmemset(&mmioInfo, 0, sizeof(MMIOINFO));
                mmioInfo.htask = (HANDLE) npMCI->hCallingTask; //ntmmsystem bug, should be threadid which is dword
                npMCI->hmmioAudio = mmioOpen(npMCI->szFilename, &mmioInfo,
                                        MMIO_READ | MMIO_DENYWRITE);

                if (npMCI->hmmioAudio == NULL)
                    npMCI->hmmioAudio = mmioOpen(npMCI->szFilename, &mmioInfo,
                                        MMIO_READ);

                if (!npMCI->hmmioAudio) {
		    Assert(0);
		    return MCIERR_DRIVER_INTERNAL;
		}
            }

            // !!! We use four 1/2 second buffers.  This is arbitrary.
            npMCI->wABs = 4;
            npMCI->wABOptimal = 0;
            npMCI->dwABSize = npMCI->pWF->nAvgBytesPerSec / 2;
	    break;

        case MCIAVI_ALG_CDROM:
            //!!!! we need to tune this!!!!
            // !!! We use four 1/4 second buffers.  This is arbitrary.
            npMCI->wABs = mmGetProfileInt(szIni, TEXT("CDAudioBuffers"), 4);
            npMCI->wABOptimal = 0;
            npMCI->dwABSize = npMCI->pWF->nAvgBytesPerSec /
			      mmGetProfileInt(szIni, TEXT("CDAudioBufSize"), 4);
            break;

        case MCIAVI_ALG_INTERLEAVED:
#define BUFMOD	    4096  	// had better be a power of 2!
            /* Fix up some values based on the header information */
            dw = muldiv32(npMCI->dwMicroSecPerFrame,
                    npMCI->pWF->nAvgBytesPerSec,1000000L);

            npMCI->dwABSize = (dw + BUFMOD - 1) & ~(BUFMOD - 1L);

            npMCI->wABs = npMCI->wEarlyAudio + 2 + (WORD) npMCI->dwBufferedVideo;

            /* Soundblaster hack: waveoutdone only accurate to 2K. */

            //!!!!!!!!!! is this right.

            if (npMCI->dwMicroSecPerFrame) {
                npMCI->wABOptimal = npMCI->wABs -
                        (UINT) (muldiv32(BUFMOD, 1, muldiv32(npMCI->dwMicroSecPerFrame,
                        npMCI->pWF->nAvgBytesPerSec,1000000L)));
            } else {
                npMCI->wABOptimal = 0;
            }

            //!!! hack so we can do burst reading, up to 1sec
            //npMCI->wABs += (int)muldiv32(1000000l, 1, npMCI->dwMicroSecPerFrame);

	    // !!!!!!
	    // !!!!!!
	    npMCI->wABOptimal = 0;
	    npMCI->wABs = (UINT) muldivru32(npMCI->wABs, dw, npMCI->dwABSize) + 1;
	
            DPF2(("Using %u audio buffers of %lu bytes, of which %u should be full.\n", npMCI->wABs, npMCI->dwABSize, npMCI->wABOptimal));
            break;

        default:
            Assert(0);
            return 0L;
    }

    npMCI->dwABSize -= npMCI->dwABSize % npMCI->pWF->nBlockAlign;

    if (!fPlaying) {
	return 0L;
    }

    /* This code adjusts the wave format block to play
    ** the audio at the correct speed to match the frame rate.
    */

    npMCI->pWF->nSamplesPerSec = muldiv32(npMCI->pWF->nSamplesPerSec,
					    npMCI->dwMicroSecPerFrame,
					    npMCI->dwPlayMicroSecPerFrame);

    npMCI->pWF->nAvgBytesPerSec = muldiv32(npMCI->pWF->nAvgBytesPerSec,
					    npMCI->dwMicroSecPerFrame,
					    npMCI->dwPlayMicroSecPerFrame);

    if (npMCI->pWF->wFormatTag == WAVE_FORMAT_PCM) {
	/* Make sure this is exactly right... */
	npMCI->pWF->nAvgBytesPerSec =
            npMCI->pWF->nSamplesPerSec * npMCI->pWF->nBlockAlign;
    }

    /* Kill any currently playing sound */
    sndPlaySound(NULL, 0);

    DPF2(("Opening wave device....\n"));
    /* Try to open a wave device. */
    w = waveOutOpen(&npMCI->hWave, (UINT)WAVE_MAPPER,
		(const LPWAVEFORMATEX) npMCI->pWF,
		(DWORD_PTR) &mciaviWaveOutFunc,
		(DWORD_PTR) (LPMCIGRAPHIC) npMCI,
                (DWORD)CALLBACK_FUNCTION);

    if (w) {
	DPF(("Unable to open wave device.\n"));

	npMCI->hWave = NULL;
        return w == WAVERR_BADFORMAT ?
			    MCIERR_WAVE_OUTPUTSUNSUITABLE :
			    MCIERR_WAVE_OUTPUTSINUSE;
    }

    // We now have the wave device, so turn off the flag that says we
    // lost it.
    npMCI->dwFlags &= ~MCIAVI_LOSTAUDIO;

#ifndef _WIN32 // No need to lock it on NT - although we could with Virtual mem
              // functions
    //
    // page lock our DS so our wave callback function can
    // touch it without worry. see mciaviWaveOutFunc()
    //
    GlobalPageLock(GetDS());
#endif //WIN16

    /* Pause the wave output device, so it won't start playing
    ** when we're loading up the buffers.
    */
    if (waveOutPause(npMCI->hWave) != 0) {
	DPF(("Error from waveOutPause!\n"));
	return MCIERR_DRIVER_INTERNAL;
    }

    if (npMCI->dwFlags & MCIAVI_VOLUMESET) {
	InternalSetVolume(npMCI, npMCI->dwVolume);
    } else {
	// must be done on worker thread
	InternalGetVolume(npMCI);
    }

#ifdef DEBUG
    Assert(!(npMCI->lpAudio));
#endif
    npMCI->lpAudio = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE,
		    npMCI->wABs * (npMCI->dwABSize + sizeof(WAVEHDR)));

    if (!npMCI->lpAudio) {
        // The wave device will be released when CleanUpAudio is called
	return MCIERR_OUT_OF_MEMORY;
    }

    npMCI->dwAudioPlayed = 0L;
    npMCI->wNextAB = 0;
    npMCI->dwUsedThisAB = 0;

    /* Allocate and prepare our buffers */
    for (w = 0; w < npMCI->wABs; w++) {
	lpWaveHdr = (LPWAVEHDR) (npMCI->lpAudio + (w * sizeof(WAVEHDR)));

	lpWaveHdr->lpData = (HPSTR) npMCI->lpAudio +
				    npMCI->wABs * sizeof(WAVEHDR) +
				    w * npMCI->dwABSize;
	lpWaveHdr->dwBufferLength = npMCI->dwABSize;
	lpWaveHdr->dwBytesRecorded = 0L;
	lpWaveHdr->dwUser = 0L;
	lpWaveHdr->dwFlags = 0L;
	lpWaveHdr->dwLoops = 0L;
	lpWaveHdr->lpNext = 0L;
	lpWaveHdr->reserved = 0;
    }

    for (w = 0; w < npMCI->wABs; w++) {
	lpWaveHdr = (LPWAVEHDR) (npMCI->lpAudio + (w * sizeof(WAVEHDR)));

	if (waveOutPrepareHeader(npMCI->hWave, lpWaveHdr, sizeof(WAVEHDR))
			!= 0) {
	    return MCIERR_OUT_OF_MEMORY;
	}
	lpWaveHdr->dwFlags |= WHDR_DONE;
    }

#ifdef REMOTESTEAL
    // Set a registry key so that other processes know who has the
    // wave device... if it fails we do not bother to try and
    // recover.  It only means that the sound will not follow the
    // active video
    {
	TCHAR achName[100];
	DWORD dwRet;
	DWORD dwDisposition;

	lstrcpy(achName, KEYNAME);
	lstrcat(achName, szWaveOwner);

	EnterList();
        if (hkey || (ERROR_SUCCESS == (dwRet = RegCreateKeyEx(ROOTKEY, achName, 0,
			    TEXT("VFW"), REG_OPTION_VOLATILE, KEY_ALL_ACCESS,
			    NULL, &hkey, &dwDisposition)))) {

	    // Write both out process id and the handle of the event
	    // that needs to be signalled.  Any remote process out there
	    // that want to steal our wave device will duplicate the handle
	    // to the event, for which our process id is required.
	    DWORD pid = GetCurrentProcessId();
	    RegSetValueEx(hkey, szRemoteWaveHwnd, 0, REG_DWORD,
			    (CONST BYTE *)&npMCI->hwndDefault, sizeof(npMCI->hwndDefault));
	}
	LeaveList();
    }
#endif

    return 0L;
}

DWORD FAR PASCAL CleanUpAudio(NPMCIGRAPHIC npMCI)
{
    UINT	w;

    /* Clear flags relating to playing audio */
    npMCI->dwFlags &= ~(MCIAVI_WAVEPAUSED | MCIAVI_LOSEAUDIO);

    if (npMCI->lpAudio) {
        waveOutRestart(npMCI->hWave); // just in case we are paused
	waveOutReset(npMCI->hWave);

	for (w = 0; w < npMCI->wABs; w++) {
	    LPWAVEHDR	lpWaveHdr;

	    lpWaveHdr = (LPWAVEHDR) (npMCI->lpAudio
					    + (w * sizeof(WAVEHDR)));

#ifndef _WIN32
	    //don't touch prepared data
	    lpWaveHdr->lpData = npMCI->lpAudio
				    + npMCI->wABs * sizeof(WAVEHDR)
				    + w * npMCI->dwABSize;
	    lpWaveHdr->dwBufferLength = npMCI->dwABSize;
#endif

	    /* Do we need to check for an error from this? */
	    waveOutUnprepareHeader(npMCI->hWave, lpWaveHdr,
						    sizeof(WAVEHDR));
	}
	GlobalFreePtr(npMCI->lpAudio);
	npMCI->lpAudio = NULL;

	Assert(npMCI->wABFull == 0);
    }

    DPF2(("Closing wave device.\n"));
#ifdef REMOTESTEAL
    EnterList()
    if (hkey) {
        RegDeleteValue(hkey, szRemoteWaveHwnd);
    }
    LeaveList()
#endif
    waveOutClose(npMCI->hWave);
    npMCI->hWave = 0;

#ifdef REMOTESTEAL
    // If we are being asked to give the audio to someone else...
    // tell them to pick it up, and tell them that we want it back.
    // We do this here rather than in GiveWaveDevice as GiveWaveDevice
    // may not be called.
    if (hwndWantAudio) {
	if (IsWindow(hwndWantAudio)) {
	    PostMessage(hwndWantAudio, WM_AUDIO_ON, (WPARAM)npMCI->hwndDefault, 0);
	}
	hwndWantAudio = 0;
    }
#endif // REMOTESTEAL

#ifndef _WIN32
    GlobalPageUnlock(GetDS());
#endif //WIN16

    return 0L;
}

BOOL NEAR PASCAL WaitForFreeAudioBuffer(NPMCIGRAPHIC npMCI, BOOL FAR *lpfHurry)
{
    LPWAVEHDR   lpWaveHdr;

    lpWaveHdr = (LPWAVEHDR) (npMCI->lpAudio
				+ (npMCI->wNextAB * sizeof(WAVEHDR)));

    /* Use the number of full audio buffers to decide if we're behind. */
    if (npMCI->wABFull < npMCI->wABOptimal) {
        *lpfHurry = TRUE;
    }

    /* If all of the audio buffers are full, we have to wait. */
    if (npMCI->wABFull == npMCI->wABs) {

        DWORD time = timeGetTime();

        #define AUDIO_WAIT_TIMEOUT 2000

        DOUT2("waiting for audio buffer.");

        // we better not wait if the device is not playing!
        Assert(!(npMCI->dwFlags & MCIAVI_WAVEPAUSED));

#ifdef XDEBUG
        GetAsyncKeyState(VK_ESCAPE);
        GetAsyncKeyState(VK_F2);
        GetAsyncKeyState(VK_F3);
        GetAsyncKeyState(VK_F4);
#endif
        while (npMCI->wABFull == npMCI->wABs) {

            if (npMCI->dwFlags & MCIAVI_STOP)
                return FALSE;

            aviTaskCheckRequests(npMCI);

            //
            //  the "Fahrenheit VA Audio Wave Driver" may get confused
            //  if you call waveOutPause() and waveOutRestart() alot
            //  and it will stay paused no matter what you do, it has
            //  all our buffers and it still does not make any sound
            //  you can call waveOutRestart() until you are blue in
            //  the face, it will do nothing.
            //
            //  so this is why this routine can time out, after waiting
            //  2 seconds or so we just toss all the audio in the buffers
            //  and start over.
            //
            if (timeGetTime() - time > AUDIO_WAIT_TIMEOUT) {
                DOUT("Gave up waiting, reseting wave device\n");
                gfUseGetPosition = 0;
                // Can no longer rely on waveOutGetPosition returning
                // the right value.
                waveOutReset(npMCI->hWave);
                break;
            }

#ifdef XDEBUG
            if (GetAsyncKeyState(VK_ESCAPE) & 0x0001) {
                DPF(("STOPPED WAITING! wABFull = %d, wABs = %d\n", npMCI->wABFull,npMCI->wABs));
                return FALSE;
            }

            if (GetAsyncKeyState(VK_F2) & 0x0001) {
                DOUT("Trying waveOutRestart\n");
                waveOutRestart(npMCI->hWave);
            }

            if (GetAsyncKeyState(VK_F3) & 0x0001) {
                DOUT("Trying waveOutReset\n");
                gfUseGetPosition = 0;
                // Can no longer rely on waveOutGetPosition returning
                // the right value.
                waveOutReset(npMCI->hWave);
            }

            if (GetAsyncKeyState(VK_F4) & 0x0001) {

                int i,n;

                for (i=n=0; i<(int)npMCI->wABs; i++) {

                    if (((LPWAVEHDR)npMCI->lpAudio)[i].dwFlags & WHDR_DONE) {
                        DPF(("Buffer #%d is done!\n", i));
                        n++;
                    }
                    else {
                        DPF(("Buffer #%d is not done\n", i));
                    }
                }

                if (n > 0)
                    DPF(("%d buffers are done but our callback did not get called!\n", n));
            }
#endif
        }

        DOUT2("done\n");
    }

    /* Debugging check that wave has finished playing--should never happen */
    Assert(lpWaveHdr->dwFlags & WHDR_DONE);

#ifndef _WIN32
// don't touch prepared data
    lpWaveHdr->lpData = npMCI->lpAudio +
				npMCI->wABs * sizeof(WAVEHDR) +
				npMCI->wNextAB * npMCI->dwABSize;
#endif

    return TRUE;
}

#ifndef _WIN32
#pragma optimize("", off)
#endif

BOOL NEAR PASCAL ReadSomeAudio(NPMCIGRAPHIC npMCI, BYTE _huge * lpAudio,
				DWORD dwStart, DWORD FAR * pdwLength)
{
    DWORD	dwIndex = 0;
    DWORD	ckidAudio;
    DWORD	dwAudioPos = 0L;
    AVIINDEXENTRY far * lpIndexEntry;

    Assert(npMCI->wPlaybackAlg == MCIAVI_ALG_HARDDISK ||
           npMCI->wPlaybackAlg == MCIAVI_ALG_AUDIOONLY);

    Assert(npMCI->hpIndex);

    /*
    ** Figure out what type of chunk we're looking for,
    */
    ckidAudio = MAKEAVICKID(cktypeWAVEbytes, npMCI->nAudioStream);

    lpIndexEntry = (AVIINDEXENTRY FAR *) npMCI->hpIndex;

    for (dwIndex = 0; dwIndex < npMCI->macIndex;
                dwIndex++, ++((AVIINDEXENTRY _huge *) lpIndexEntry)) {

	if (lpIndexEntry->ckid != ckidAudio)
	    continue;
	
	if (dwAudioPos + lpIndexEntry->dwChunkLength > dwStart) {
	    DWORD	dwLengthNow;
	    DWORD	dwSeekTo;
	
	    dwLengthNow = lpIndexEntry->dwChunkLength;
	    dwSeekTo = lpIndexEntry->dwChunkOffset + 8;
	
	    if (dwAudioPos + dwLengthNow > dwStart + *pdwLength) {
		/* Attempted optimization: If we've already read some
		** data, and we can't read the next whole chunk, let's
		** leave it for later.
		*/
		if (dwAudioPos > dwStart && (!(npMCI->dwFlags & MCIAVI_REVERSE)))
		    break;
		dwLengthNow = dwStart + *pdwLength - dwAudioPos;
	    }
	
	    if (dwAudioPos < dwStart) {
		dwLengthNow -= (dwStart - dwAudioPos);
		dwSeekTo += (dwStart - dwAudioPos);
	    }

            mmioSeek(npMCI->hmmioAudio, dwSeekTo, SEEK_SET);

	    if (mmioRead(npMCI->hmmioAudio, lpAudio, dwLengthNow)
			    != (LONG) dwLengthNow) {
		DPF(("Error reading audio data (%lx bytes at %lx)\n", dwLengthNow, dwSeekTo));
		return FALSE;
	    }
	    lpAudio += dwLengthNow;	
	}
	
	dwAudioPos += lpIndexEntry->dwChunkLength;
	
	if (dwAudioPos >= dwStart + *pdwLength)
	    return TRUE;
    }

    if (dwAudioPos < dwStart)
	*pdwLength = 0;	    // return FALSE?
    else
	*pdwLength = dwAudioPos - dwStart;

    return TRUE;
}
#ifndef _WIN32
#pragma optimize("", on)
#endif
	
BOOL NEAR PASCAL ReverseWaveBuffer(NPMCIGRAPHIC npMCI, LPWAVEHDR lpWaveHdr)
{
    DWORD   dwLeft = lpWaveHdr->dwBufferLength;
    BYTE _huge *hp1;
    BYTE _huge *hp2;
    DWORD   dwBlock = npMCI->pWF->nBlockAlign;
    BYTE    bTemp;
    DWORD   dw;

    Assert(npMCI->dwFlags & MCIAVI_REVERSE);
    Assert(npMCI->wPlaybackAlg == MCIAVI_ALG_HARDDISK ||
	   npMCI->wPlaybackAlg == MCIAVI_ALG_AUDIOONLY);

    /* This routine doesn't like it when the data doesn't end on a
    ** block boundary, so make it so.  This should never happen.
    */
    Assert((dwLeft % dwBlock) == 0);
    dwLeft -= dwLeft % dwBlock;

    hp1 = lpWaveHdr->lpData;
    hp2 = ((HPSTR) lpWaveHdr->lpData) + (dwLeft - dwBlock);

    while ((LONG) dwLeft > (LONG) dwBlock) {
	for (dw = 0; dw < dwBlock; dw++) {
	    bTemp = *hp1;
	    *hp1++ = *hp2;
	    *hp2++ = bTemp;
	}
	hp2 -= dwBlock * 2;
	dwLeft -= dwBlock * 2;
    }

    return TRUE;
}

void FAR PASCAL BuildVolumeTable(NPMCIGRAPHIC npMCI)
{
    int	    vol;
    int     i;

    if (!npMCI->pWF || npMCI->pWF->wFormatTag != WAVE_FORMAT_PCM)
        return;

    if (((NPPCMWAVEFORMAT) npMCI->pWF)->wBitsPerSample != 8)
        return;

    vol = (LOWORD(npMCI->dwVolume) + HIWORD(npMCI->dwVolume)) / 2;
    vol = (int) (((LONG) vol * 256) / 500);

    if (!npMCI->pVolumeTable)
        npMCI->pVolumeTable = (void *)LocalAlloc(LPTR, 256);

    if (!npMCI->pVolumeTable)
        return;

    for (i = 0; i < 256; i++) {
        npMCI->pVolumeTable[i] = (BYTE) min(255, max(0,
                (int) ((((LONG) (i - 128) * vol) / 256) + 128)));
    }
}

BOOL NEAR PASCAL AdjustVolume(NPMCIGRAPHIC npMCI, LPWAVEHDR lpWaveHdr)
{
    DWORD   dwLeft = lpWaveHdr->dwBufferLength;
    BYTE FAR *pb;

    if (npMCI->pWF->wFormatTag != WAVE_FORMAT_PCM)
	return FALSE;

    if (!npMCI->pVolumeTable)
        return FALSE;

    if (((NPPCMWAVEFORMAT)npMCI->pWF)->wBitsPerSample != 8)
        return FALSE;

    pb = lpWaveHdr->lpData;

#ifndef _WIN32
    if (OFFSETOF(pb) + dwLeft > 64l*1024) {
	while (dwLeft--) {
            *pb = npMCI->pVolumeTable[*pb];
            ((BYTE _huge *)pb)++;
	}
    }
    else {
        while ((int)dwLeft--)
            *pb++ = npMCI->pVolumeTable[*pb];
    }
#else
    while ((int)dwLeft--)
        *pb++ = npMCI->pVolumeTable[*pb];
#endif

    return TRUE;
}

BOOL NEAR PASCAL PlaySomeAudio(NPMCIGRAPHIC npMCI, LPWAVEHDR lpWaveHdr)
{
    if (npMCI->pVolumeTable)
        AdjustVolume(npMCI, lpWaveHdr);

    lpWaveHdr->dwFlags &= ~WHDR_DONE;

    /* If we're playing and we've used all of our audio buffers, pause the
    ** wave device until we can fill more of them up.
    **
    ** we need to be carefull not to do this on the last frame!!!
    */
    if ((npMCI->wTaskState == TASKPLAYING) &&
        !(npMCI->dwFlags & MCIAVI_WAVEPAUSED) &&
        (npMCI->wABFull == 0 || npMCI->nAudioBehind > nAudioPanic)) {

        if (npMCI->wABFull > 0) {
            DPF(("Audio panic stop\n"));
        } else {
            DPF(("Audio queue empty; pausing wave device\n"));
        }

        //
        // some audio cards dont like starving it confuses them
        // it is kind of rude any way.  we are going to cause a audio break
        // anyway so if we lose a little bit of audio (a few frames or so)
        // no one will even notice (any worse than the audio break)
        //
        if (npMCI->wABFull <= 1) {
            DOUT("Trying audio hack!\n");
            gfUseGetPosition = 0;
            // Can no longer rely on waveOutGetPosition returning
            // the right value.
            waveOutReset(npMCI->hWave);
        }

        ++npMCI->dwAudioBreaks;
        waveOutPause(npMCI->hWave);

        ICDrawStop(npMCI->hicDraw);
	npMCI->dwFlags |= MCIAVI_WAVEPAUSED;
    }


    if (waveOutWrite(npMCI->hWave, lpWaveHdr, sizeof(WAVEHDR)) != 0) {
        DPF(("Error from waveOutWrite!\n"));
	npMCI->dwTaskError = MCIERR_AVI_AUDIOERROR;

        // if this fails, you can end up looping indefinitely in
        // PlayRecordAudio in the while(cksize) loop - cksize is never
        // decremented because the current audio buffer stays full.
        // Need to throw this stuff away or we lock up.
        npMCI->dwUsedThisAB = 0;


	return FALSE;
    } else {
#ifdef _WIN32
	InterlockedIncrement(&npMCI->wABFull);
#else
	++npMCI->wABFull;
#endif

	/* Use the next wave buffer next time */
	++npMCI->wNextAB;
	if (npMCI->wNextAB == npMCI->wABs)
	    npMCI->wNextAB = 0;
	
	npMCI->dwUsedThisAB = 0;
    }

    if (npMCI->wABFull < min(npMCI->wABOptimal, npMCI->wABFull/2))
        npMCI->nAudioBehind++;
    else
        npMCI->nAudioBehind=0;

    /* If we paused the wave device to let ourselves catch up, and
    ** we've caught up enough, restart the device.
    */
    if ((npMCI->dwFlags & MCIAVI_WAVEPAUSED) &&
        npMCI->wTaskState == TASKPLAYING &&
        npMCI->wABFull == npMCI->wABs) {

        DPF2(("restarting wave device\n"));
        waveOutRestart(npMCI->hWave);

	ICDrawStart(npMCI->hicDraw);
        npMCI->dwFlags &= ~(MCIAVI_WAVEPAUSED);
        npMCI->nAudioBehind = 0;
    }

    return TRUE;
}

/* Play the current record's audio */
BOOL NEAR PASCAL PlayRecordAudio(NPMCIGRAPHIC npMCI, BOOL FAR *pfHurryUp,
				    BOOL FAR *pfPlayedAudio)
{
    LPWAVEHDR	lpWaveHdr;
    FOURCC	ckid;
    DWORD	cksize;
    LPSTR	lpSave;
    BYTE _huge *lpData;
    BOOL	fRet = TRUE;
////BOOL        fSilence;
    LONG        len;
    DWORD       dwBytesThisChunk;

    Assert(npMCI->wPlaybackAlg == MCIAVI_ALG_INTERLEAVED);

    lpSave = npMCI->lp;

    *pfPlayedAudio = FALSE;

    /* Remember!
    **
    ** In the new file format, things shouldn't necessarily need to
    ** be ordered with the wave stuff always first.
    */

    len = (LONG)npMCI->dwThisRecordSize;

    while (len > 3 * sizeof(DWORD)) {

	/* Look at the next chunk */
	ckid = GET_DWORD();
        cksize = GET_DWORD();

        lpData = npMCI->lp;

        len -= ((cksize + 1) & ~1) + 8;
        SKIP_BYTES((cksize + 1) & ~1);

        if (StreamFromFOURCC(ckid) != (UINT)npMCI->nAudioStream)
            continue;

	*pfPlayedAudio = TRUE;

	while (cksize) {
	    dwBytesThisChunk = cksize;

	    lpWaveHdr = ((LPWAVEHDR)npMCI->lpAudio) + npMCI->wNextAB;

	    if (!npMCI->dwUsedThisAB) {
		if (!WaitForFreeAudioBuffer(npMCI, pfHurryUp))
		    /* We had to stop waiting--the stop flag was probably set. */
		    goto exit;
	    }

	    if (dwBytesThisChunk > npMCI->dwABSize - npMCI->dwUsedThisAB) {
		// DPF(("Audio Record is too big!\n"));
		dwBytesThisChunk = npMCI->dwABSize - npMCI->dwUsedThisAB;
	    }

	    hmemcpy((BYTE _huge *)lpWaveHdr->lpData + npMCI->dwUsedThisAB,
		    lpData, dwBytesThisChunk);

	    cksize -= dwBytesThisChunk;
	    lpData += dwBytesThisChunk;
	    npMCI->dwUsedThisAB += dwBytesThisChunk;
	
	    if (npMCI->dwUsedThisAB == npMCI->dwABSize) {
		lpWaveHdr->dwBufferLength = npMCI->dwUsedThisAB;

		fRet = PlaySomeAudio(npMCI, lpWaveHdr);
	    }
	}

    }

    if ((*pfPlayedAudio == FALSE) && (npMCI->dwUsedThisAB)) {

	// this is interleaved. therefore if we have some audio waiting,
	// and there was no audio in this record, it's because we've
	// used up all the audio -> so play remaining audio or
	// it will never be queued.

	lpWaveHdr = ((LPWAVEHDR)npMCI->lpAudio) + npMCI->wNextAB;
	lpWaveHdr->dwBufferLength = npMCI->dwUsedThisAB;
	fRet = PlaySomeAudio(npMCI, lpWaveHdr);
    }

    /* Use the number of full audio buffers to decide if we're behind. */
    if (npMCI->wABFull >= npMCI->wABOptimal) {
         *pfHurryUp = FALSE;
    }

exit:
    npMCI->lp = lpSave;

    return fRet;
}

/* For "preload audio" or "random access audio" modes, do what needs
** to be done to keep our buffers full.
*/
BOOL NEAR PASCAL KeepPlayingAudio(NPMCIGRAPHIC npMCI)
{
    LPWAVEHDR	lpWaveHdr;
    DWORD	dwBytesTotal = 0L;
    LONG        lNewAudioPos;
////BOOL        fFirstTime = TRUE;

    Assert(npMCI->wPlaybackAlg == MCIAVI_ALG_HARDDISK ||
	   npMCI->wPlaybackAlg == MCIAVI_ALG_AUDIOONLY);

PlayMore:
    lpWaveHdr = ((LPWAVEHDR)npMCI->lpAudio) + npMCI->wNextAB;

    if (npMCI->dwFlags & MCIAVI_REVERSE) {
	lNewAudioPos = npMCI->dwAudioPos - npMCI->dwABSize;
	if (lNewAudioPos < 0)
	    lNewAudioPos = 0;
	dwBytesTotal = npMCI->dwAudioPos - lNewAudioPos;
    } else {
	lNewAudioPos = npMCI->dwAudioPos + npMCI->dwABSize;
	if (lNewAudioPos > (LONG) npMCI->dwAudioLength)
	    lNewAudioPos = npMCI->dwAudioLength;
	dwBytesTotal = lNewAudioPos - npMCI->dwAudioPos;
    }

    if (dwBytesTotal == 0) {

        if (npMCI->dwFlags & MCIAVI_WAVEPAUSED) {

            DOUT("no more audio to play, restarting wave device\n");

            waveOutRestart(npMCI->hWave);
            ICDrawStart(npMCI->hicDraw);
            npMCI->dwFlags &= ~(MCIAVI_WAVEPAUSED);
            npMCI->nAudioBehind = 0;
        }

        return TRUE;
    }

    /* If all of the audio buffers are full, we have nothing to do */
    if (npMCI->wABFull == npMCI->wABs)
	return TRUE;

#if 0
    //!!!! Should we be yielding at all in here?
    //!!! NO NO! not if updating!!!!
    if (!fFirstTime) {
	aviTaskYield();
    }
    fFirstTime = FALSE;
#endif

    if (npMCI->dwFlags & MCIAVI_REVERSE)
	npMCI->dwAudioPos = lNewAudioPos;

#ifdef USEAVIFILE
    if (npMCI->pf) {
	LONG	    lPos;
	LONG	    lLength;

        lPos = npMCI->dwAudioPos / SH(npMCI->nAudioStream).dwSampleSize;
	lLength = dwBytesTotal / SH(npMCI->nAudioStream).dwSampleSize;

        AVIStreamRead(SI(npMCI->nAudioStream)->ps,
		      lPos, lLength,
		      lpWaveHdr->lpData,
		      npMCI->dwABSize,
		      NULL, NULL);
    }
    else
#endif
    {
	if (!ReadSomeAudio(npMCI, lpWaveHdr->lpData,
			npMCI->dwAudioPos,
			&dwBytesTotal))
	    return FALSE;
		
	if (dwBytesTotal == 0)
		return TRUE;
    }

    if (!(npMCI->dwFlags & MCIAVI_REVERSE))
	npMCI->dwAudioPos += dwBytesTotal;

    lpWaveHdr->dwBufferLength = dwBytesTotal;

    if (npMCI->dwFlags & MCIAVI_REVERSE) {
	ReverseWaveBuffer(npMCI, lpWaveHdr);
    }

    if (!PlaySomeAudio(npMCI, lpWaveHdr))
	return FALSE;

//  return TRUE;

    goto PlayMore;
}


#ifdef USENONINTFROMCD
/* Play the current chunk's audio */
BOOL NEAR PASCAL HandleAudioChunk(NPMCIGRAPHIC npMCI)
{
    LPWAVEHDR	lpWaveHdr;
    FOURCC	ckid;
    DWORD	cksize;
    BYTE _huge *lpData;
    BOOL	fRet = TRUE;
    BOOL	fSilence;
    DWORD	dwBytesTotal = 0L;
    DWORD       dwBytesThisChunk;
    DWORD       dwBytesThisBuffer;
    BOOL        fHurryUp;

    Assert(npMCI->wPlaybackAlg == MCIAVI_ALG_CDROM);

    while ((DWORD) (npMCI->lp - npMCI->lpBuffer)
            < npMCI->dwThisRecordSize - 3 * sizeof(DWORD)) {

	/* Look at the next chunk */
	ckid = GET_DWORD();
	cksize = GET_DWORD();

	lpData = npMCI->lp;
	SKIP_BYTES(cksize + (cksize & 1));

	fSilence = (TWOCCFromFOURCC(ckid) == cktypeWAVEsilence);

	if (fSilence) {
	    if (cksize != sizeof(DWORD)) {
		DPF(("Wave silence chunk of bad length!\n"));
		fRet = FALSE;
		npMCI->dwTaskError = MCIERR_INVALID_FILE;
		goto exit;
	    }
	    dwBytesThisChunk = PEEK_DWORD();
	} else {
	    dwBytesThisChunk = cksize;
	}

	while (dwBytesThisChunk > 0) {
	    lpWaveHdr = ((LPWAVEHDR)npMCI->lpAudio) + npMCI->wNextAB;

	    if (!WaitForFreeAudioBuffer(npMCI, &fHurryUp))
		/* We had to stop waiting--the stop flag was probably set. */
		goto exit;
	
	    dwBytesThisBuffer = min(dwBytesThisChunk,
			    npMCI->dwABSize - npMCI->dwUsedThisAB);

	    if (!fSilence) {
		/* Move the data into the buffer */
		hmemcpy((BYTE _huge *) lpWaveHdr->lpData + npMCI->dwUsedThisAB,
			lpData,
			dwBytesThisBuffer);
		lpData += dwBytesThisBuffer;
	    } else {
		/* Fill the buffer with silence */
		/* This isn't right for 16-bit! */
#ifndef _WIN32
    #pragma message("WAVE silence chunks don't work right now.")
#endif
	//      fmemfill((BYTE _huge *)lpWaveHdr->lpData + npMCI->dwUsedThisAB,
	//				dwBytesThisBuffer, 0x80);
	    }
	
	    dwBytesThisChunk -= dwBytesThisBuffer;
	    npMCI->dwUsedThisAB += dwBytesThisBuffer;

//	    if (npMCI->dwUsedThisAB == npMCI->dwABSize) {
		lpWaveHdr->dwBufferLength = npMCI->dwUsedThisAB;

		fRet = PlaySomeAudio(npMCI, lpWaveHdr);
//	    }
	}
    }

exit:
    return fRet;
}
#endif


/******************************************************************************
*****************************************************************************/


// We need to talk to a worker thread for another AVI device
extern DWORD mciaviTaskRequest(NPMCIGRAPHIC npMCI, UINT message,
				DWORD dwFlags, LPARAM lParam,
				DWORD dwCallback, BOOL bDelayedComplete);

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | StealWaveDevice | steal the audio device from another
 * instance of MCIAVI.
 *
 * @parm NPMCIGRAPHIC | npMCI | near ptr to the instance data
 *
 ***************************************************************************/

BOOL FAR PASCAL StealWaveDevice(NPMCIGRAPHIC npMCI)
{
#ifdef STEALWAVE
    NPMCIGRAPHIC np;
    Assert(npMCI->hWave == NULL);

#if defined(_WIN32) && defined(DEBUG)
    if (mmGetProfileInt(szIni, TEXT("NoStealing"), 0)) {
        return(FALSE);
    }
#endif

    DPF2(("StealWaveDevice '%ls' hTask=%04X\n", (LPSTR)npMCI->szFilename, npMCI->hTask));
#if 0
    if (npMCI->dwFlags & MCIAVI_SEEKING) {
	DPF0(("Do we really want to get a wave device for seeking?\n"));
    }
#endif
    EnterList();

    //
    //  walk the list of open MCIAVI instances and find one that
    //  could give up the wave device
    //
    for (np=npMCIList; np; np = np->npMCINext) {

        if (np->hWave) {
	    DWORD dwRetRequest;
            DPF2(("**** Stealing the wave device from '%ls'  (hTask=%4X).\n", (LPSTR)np->szFilename, np->hTask));

	    // Requesting the current wave device owner to release ownership.
	    // By sending the request directly to the worker thread we avoid
	    // some of the problems we might have if we had to talk to the
	    // winproc thread.  This operation is synchronous.  We do not
	    // want the request to complete until the wave device has been
	    // released.  If we sent to the window, then the winproc thread
	    // would have to send a synchronous request to its worker.  This
	    // is too tedious, and we do not have (at time of writing) the
	    // mechanisms for supporting this way of working.

	    LeaveList();
            dwRetRequest = mciaviTaskRequest(np, AVI_WAVESTEAL, 0, (LPARAM) 0, 0, FALSE);
	    if (dwRetRequest == 0) {
		return TRUE;
	    } else {
		return(FALSE);
	    }
        }
    }

    DPF2(("StealWaveDevice can't find a local device to steal\n"));
#ifdef REMOTESTEAL
    // Read the registry key of the task with the wave device.
    // If it fails we do not bother to try and
    // recover.  It only means that the sound will not follow the
    // active video
    {
	TCHAR achName[100];
	DWORD dwRet;
	DWORD dwType;
	HWND hwndRemote;
	DWORD cbData = sizeof(hwndRemote);

	lstrcpy(achName, KEYNAME);
	lstrcat(achName, szWaveOwner);

        if (hkey || (ERROR_SUCCESS == (dwRet = RegOpenKeyEx(ROOTKEY, achName,
			    0, KEY_ALL_ACCESS, &hkey)))) {

	    dwRet = RegQueryValueEx(hkey, szRemoteWaveHwnd, 0, &dwType,
			    (LPBYTE)&hwndRemote, &cbData);
	    if (dwRet == ERROR_SUCCESS) {
		DPF2(("Posting WM_AUDIO_OFF to hwnd %x\n", hwndRemote));
		PostMessage(hwndRemote, WM_AUDIO_OFF, (WPARAM)npMCI->hwndDefault, 0);
	    } else {
		// Could not read wave owner stuff from registry
	    }
	} else {
    	    // No wave owner stored in registry
	}
    }
#endif // REMOTESTEAL
    LeaveList();
#endif // STEALWAVE

    return FALSE;
}

/***************************************************************************
 *
 * @doc INTERNAL MCIAVI
 *
 * @api BOOL | GiveWaveDevice | give away the audio device
 * instance of MCIAVI.
 *
 * @parm NPMCIGRAPHIC | npMCI | near ptr to the instance data
 *
 ***************************************************************************/
BOOL FAR PASCAL GiveWaveDevice(NPMCIGRAPHIC npMCI)
{
#ifdef STEALWAVE
    extern NPMCIGRAPHIC npMCIList; // in graphic.c
    NPMCIGRAPHIC np;

    Assert(npMCI->hWave == NULL);
    DPF2(("GiveWaveDevice '%ls' hTask=%04X\n", (LPTSTR)npMCI->szFilename, npMCI->hTask));

    if (hwndLostAudio) {
	BOOL fRet = FALSE;
	if (IsWindow(hwndLostAudio)) {
	    DPF3(("Posting AUDIO_ON message to %x\n", hwndLostAudio));
	    fRet = PostMessage(hwndLostAudio, WM_AUDIO_ON, 0, 0);
	} else {
	    DPF3(("Lost audio window %x is no longer valid\n", hwndLostAudio));
	}
	hwndLostAudio = 0;
	if (fRet) return TRUE;
    }

    EnterList();

    //
    //  walk the list of open MCIAVI instances and find one that
    //  wants a wave device.  Then hint to that task that it can
    //  try and grab the wave device back.
    //  MORE inter process stuff required.
    //
    for (np=npMCIList; np; np = np->npMCINext) {

        if ((np->dwFlags & MCIAVI_LOSTAUDIO)
            && (!TestNTFlags(npMCI, NTF_CLOSING))
           )
        {
            DPF2(("**** Giving wave device to '%ls'   hTask=%04X \n", (LPSTR)np->szFilename, np->hTask));

            Assert(np!=npMCI);

            LeaveList();

            mciaviTaskRequest(np, AVI_WAVERETURN, 0, (LPARAM) 0, 0, FALSE);
            return TRUE;
        }
    }

    LeaveList();
#endif // STEALWAVE
    return FALSE;
}



#ifndef _WIN32
#pragma alloc_text(FIX, mciaviWaveOutFunc)
#pragma optimize("", off)
#endif

void FAR PASCAL _LOADDS mciaviWaveOutFunc(HWAVEOUT hWaveOut, UINT wMsg,
		    DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    NPMCIGRAPHIC npMCI;
    LPWAVEHDR    lpwh;

#ifndef _WIN32
    _asm _emit 0x66  ; pushad
    _asm _emit 0x60
#endif

    npMCI = (NPMCIGRAPHIC) dwInstance;
    lpwh = (LPWAVEHDR) dwParam1;

    switch(wMsg) {
	case MM_WOM_DONE:
	
#ifdef _WIN32
	    InterlockedDecrement(&npMCI->wABFull);
#else
            npMCI->wABFull--;
#endif
            npMCI->dwAudioPlayed += lpwh->dwBufferLength;
#ifdef USE_PERFORMANCE_TIMING
	    GETTIME(npMCI->dwTimingStart);
#else
	    npMCI->dwTimingStart = timeGetTime();
#endif
	    break;
    }

#ifndef _WIN32
    _asm _emit 0x66  ; popad
    _asm _emit 0x61
#endif
}

#ifndef _WIN32
#pragma optimize("", off)
#endif

