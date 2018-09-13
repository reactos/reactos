/*--------------------------------------------------------------+
| audplay.c Simple routines to play audio using an AVIStream to |
| get data.  Uses global variables, so only one instance at a	|
| time.  (Usually, there's only one sound card, so this isn't	|
| so bad.							|
+--------------------------------------------------------------*/

#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <vfw.h>
#include "audplay.h"

/*--------------------------------------------------------------+
| ****************** AUDIO PLAYING SUPPORT ******************** |
+--------------------------------------------------------------*/

static	HWAVEOUT	shWaveOut = 0;	/* Current MCI device ID */
static	LONG		slBegin;
static	LONG		slCurrent;
static	LONG		slEnd;
static	BOOL		sfLooping;
static	BOOL		sfPlaying = FALSE;

#define MAX_AUDIO_BUFFERS	16
#define MIN_AUDIO_BUFFERS	2
#define AUDIO_BUFFER_SIZE	16384

static	UINT		swBuffers;	    // total # buffers
static	UINT		swBuffersOut;	    // buffers device has
static	UINT		swNextBuffer;	    // next buffer to fill
static	LPWAVEHDR	salpAudioBuf[MAX_AUDIO_BUFFERS];

static	PAVISTREAM	spavi;		    // stream we're playing
static	LONG		slSampleSize;	    // size of an audio sample

static	LONG		sdwBytesPerSec;
static	LONG		sdwSamplesPerSec;

#ifndef WIN32
extern LONG FAR PASCAL muldiv32(LONG, LONG, LONG);
#endif

/*---------------------------------------------------------------+
| aviaudioCloseDevice -- close the open audio device, if any.    |
+---------------------------------------------------------------*/
void NEAR aviaudioCloseDevice(void)
{
    UINT	w;

    if (shWaveOut) {
	while (swBuffers > 0) {
	    --swBuffers;
	    waveOutUnprepareHeader(shWaveOut, salpAudioBuf[swBuffers],
					sizeof(WAVEHDR));
	    GlobalFreePtr((LPBYTE) salpAudioBuf[swBuffers]);
	}
	
	w = waveOutClose(shWaveOut);

	// DPF("AudioCloseDevice: waveOutClose returns %u\n", w);
	shWaveOut = NULL;	
    }
}

/*--------------------------------------------------------------+
| aviaudioOpenDevice -- get ready to play waveform data.	|
+--------------------------------------------------------------*/
BOOL FAR aviaudioOpenDevice(HWND hwnd, PAVISTREAM pavi)
{
    UINT		w;
    LPVOID		lpFormat;
    LONG		cbFormat;
    AVISTREAMINFO	strhdr;

    if (!pavi)		// no wave data to play
	return FALSE;

    if (shWaveOut)	// already something playing
	return TRUE;

    spavi = pavi;

    AVIStreamInfo(pavi, &strhdr, sizeof(strhdr));

    slSampleSize = (LONG) strhdr.dwSampleSize;
    if (slSampleSize <= 0 || slSampleSize > AUDIO_BUFFER_SIZE)
	return FALSE;

    AVIStreamFormatSize(pavi, 0, &cbFormat);

    lpFormat = GlobalAllocPtr(GHND, cbFormat);
    if (!lpFormat)
	return FALSE;

    AVIStreamReadFormat(pavi, 0, lpFormat, &cbFormat);

    sdwSamplesPerSec = ((LPWAVEFORMAT) lpFormat)->nSamplesPerSec;
    sdwBytesPerSec = ((LPWAVEFORMAT) lpFormat)->nAvgBytesPerSec;

    w = waveOutOpen(&shWaveOut, WAVE_MAPPER, lpFormat,
			(DWORD) (UINT) hwnd, 0L, CALLBACK_WINDOW);

    //
    // Maybe we failed because someone is playing sound already.
    // Shut any sound off, and try once more before giving up.
    //
    if (w) {
	sndPlaySound(NULL, 0);
	w = waveOutOpen(&shWaveOut, WAVE_MAPPER, lpFormat,
			(DWORD) (UINT) hwnd, 0L, CALLBACK_WINDOW);
    }
		
//    DPF("waveOutOpen returns %u, shWaveOut = %u\n", w, shWaveOut);

    if (w != 0) {
	/* Show error message here? */
	
	return FALSE;
    }

    for (swBuffers = 0; swBuffers < MAX_AUDIO_BUFFERS; swBuffers++) {
	if (!(salpAudioBuf[swBuffers] =
		(LPWAVEHDR)GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE,
			(DWORD)(sizeof(WAVEHDR) + AUDIO_BUFFER_SIZE))))
	    break;
	salpAudioBuf[swBuffers]->dwFlags = WHDR_DONE;
	salpAudioBuf[swBuffers]->lpData = (LPBYTE) salpAudioBuf[swBuffers]
						    + sizeof(WAVEHDR);
	salpAudioBuf[swBuffers]->dwBufferLength = AUDIO_BUFFER_SIZE;
	if (!waveOutPrepareHeader(shWaveOut, salpAudioBuf[swBuffers],
					sizeof(WAVEHDR)))
	    continue;
	
	GlobalFreePtr((LPBYTE) salpAudioBuf[swBuffers]);
	break;
    }

    // DPF("Allocated %u %lu-byte buffers.\n", swBuffers, (DWORD) AUDIO_BUFFER_SIZE);

    if (swBuffers < MIN_AUDIO_BUFFERS) {
	aviaudioCloseDevice();
	return FALSE;
    }

    swBuffersOut = 0;
    swNextBuffer = 0;

    sfPlaying = FALSE;

    return TRUE;
}


//
// Return the time in milliseconds corresponding to the currently playing
// audio sample, or -1 if no audio is playing.
// WARNING: Some sound cards are pretty inaccurate!
//
LONG FAR aviaudioTime(void)
{
    MMTIME	mmtime;

    if (!sfPlaying)
	return -1;

    mmtime.wType = TIME_SAMPLES;

    waveOutGetPosition(shWaveOut, &mmtime, sizeof(mmtime));

    if (mmtime.wType == TIME_SAMPLES)
	return AVIStreamSampleToTime(spavi, slBegin)
		+ muldiv32(mmtime.u.sample, 1000, sdwSamplesPerSec);
    else if (mmtime.wType == TIME_BYTES)
	return AVIStreamSampleToTime(spavi, slBegin)
		+ muldiv32(mmtime.u.cb, 1000, sdwBytesPerSec);
    else
	return -1;
}


//
// Fill up any empty audio buffers and ship them out to the device.
//
BOOL NEAR aviaudioiFillBuffers(void)
{
    LONG		lRead;
    UINT		w;
    LONG		lSamplesToPlay;

    /* We're not playing, so do nothing. */
    if (!sfPlaying)
	return TRUE;

    // DPF3("%u/%u (%lu-%lu)\n", swBuffersOut, swBuffers, slCurrent, slEnd);

    while (swBuffersOut < swBuffers) {
	if (slCurrent >= slEnd) {
	    if (sfLooping) {
		/* Looping, so go to the beginning. */
		slCurrent = slBegin;
	    } else
		break;
	}

	/* Figure out how much data should go in this buffer */
	lSamplesToPlay = slEnd - slCurrent;
	if (lSamplesToPlay > AUDIO_BUFFER_SIZE / slSampleSize)
	    lSamplesToPlay = AUDIO_BUFFER_SIZE / slSampleSize;


	AVIStreamRead(spavi, slCurrent, lSamplesToPlay,
		      salpAudioBuf[swNextBuffer]->lpData,
		      AUDIO_BUFFER_SIZE,
		      &salpAudioBuf[swNextBuffer]->dwBufferLength,
		      &lRead);
	
	if (lRead != lSamplesToPlay) {
	    // DPF("Error from WAVE_READ\n");
	    return FALSE;
	}
	slCurrent += lRead;
	
	w = waveOutWrite(shWaveOut, salpAudioBuf[swNextBuffer],sizeof(WAVEHDR));
	
	if (w != 0) {
	    // DPF("Error from waveOutWrite\n");
	    return FALSE;
	}
	
	++swBuffersOut;
	++swNextBuffer;
	if (swNextBuffer >= swBuffers)
	    swNextBuffer = 0;
    }

    if (swBuffersOut == 0 && slCurrent >= slEnd)
	aviaudioStop();

    /* We've filled all of the buffers we can or want to. */
    return TRUE;
}

/*--------------------------------------------------------------+
| aviaudioPlay -- Play audio, starting at a given frame		|
|								|
+--------------------------------------------------------------*/
BOOL FAR aviaudioPlay(HWND hwnd, PAVISTREAM pavi, LONG lStart, LONG lEnd, BOOL fWait)
{
    if (!aviaudioOpenDevice(hwnd, pavi))
	return FALSE;

    if (lStart < 0)
	lStart = AVIStreamStart(pavi);

    if (lEnd < 0)
	lEnd = AVIStreamEnd(pavi);

    // DPF2("Audio play%s from %ld to %ld (samples)\n", ((LPSTR) (fWait ? " wait" : "")), lStart, lEnd);

    if (lStart >= lEnd)
	return TRUE;

    if (!sfPlaying) {

	//
	// We're beginning play, so pause until we've filled the buffers
	// for a seamless start
	//
	waveOutPause(shWaveOut);
	
	slBegin = lStart;
	slCurrent = lStart;
	slEnd = lEnd;
	sfPlaying = TRUE;
    } else {
	if (lStart > slEnd) {
	    // DPF("Gap in wave that is supposed to be played!\n");
	}
	slEnd = lEnd;
    }

//    sfLooping = fLoop;

    aviaudioiFillBuffers();

    //
    // Now unpause the audio and away it goes!
    //
    waveOutRestart(shWaveOut);

    //
    // Caller wants us not to return until play is finished
    //
    if (fWait) {
	while (swBuffersOut > 0)
	    Yield();
    }

    return TRUE;
}

/*--------------------------------------------------------------+
| aviaudioMessage -- handle wave messages received by		|
| window controlling audio playback.  When audio buffers are	|
| done, this routine calls aviaudioiFillBuffers to fill them	|
| up again.							|
+--------------------------------------------------------------*/
void FAR aviaudioMessage(HWND hwnd, unsigned msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == MM_WOM_DONE) {
	--swBuffersOut;
	aviaudioiFillBuffers();
    }
}


/*--------------------------------------------------------------+
| aviaudioStop -- stop playing, close the device.		|
+--------------------------------------------------------------*/
void FAR aviaudioStop(void)
{
    UINT	w;

    if (shWaveOut != 0) {

	w = waveOutReset(shWaveOut);

	sfPlaying = FALSE;
	
	// DPF("AudioStop: waveOutReset() returns %u \n", w);

	aviaudioCloseDevice();
    }
}
