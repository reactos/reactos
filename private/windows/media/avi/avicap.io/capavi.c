/****************************************************************************
 *
 *   capavi.c
 *
 *   Main video capture module.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992 - 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#define INC_OLE2
#pragma warning(disable:4103)
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <vfw.h>
#include <mmreg.h>
#include <mmddk.h>

#include "ivideo32.h"
#include "mmdebug.h"

#ifdef USE_ACM
#include <msacm.h>
#endif

#include "avicapi.h"
#include "time.h"

#define JMK_HACK_TIMERS    TRUE

#ifdef JMK_HACK_TIMERS
 #define _INC_MMTIMERS_CODE_ TRUE
 #define CLIPBOARDLOGSIZE 1000

 #ifndef MAKEFOURCC
  #define MAKEFOURCC(a,b,c,d) ((DWORD)(a) | ((DWORD)(b) << 8) | ((DWORD)(c) << 16) | ((DWORD)(d) << 24))
 #endif

 #define RIFFTYPE(dw) (((dw & 0xFF) << 24) | ((dw & 0xFF00) << 8) | ((dw & 0xFF0000) >> 8) | ((dw & 0xFF000000) >> 24))

 #include "mmtimers.h"

typedef struct _timerstuff {
     DWORD dwFrameTickTime;	// What we think the current frame time should be
     DWORD dwFrameStampTime;	// Stamped in the VIDEOHDR
     DWORD dwTimeWritten;       // Time WriteFile called
     DWORD dwTimeToWrite;       // Time WriteFile returned
     WORD  nFramesAppended;	// Accumulated appended dropped frames
     WORD  nDummyFrames;	// frames calc'ed as dropped
     DWORD dwVideoChunkCount;   // current 'frame'
     WORD  nAudioIndex;         // next audio buffer
     WORD  nVideoIndex;         // next video buffer
     BOOL  bPending;
     WORD  nSleepCount;
     DWORD dwSleepBegin;
     DWORD dwSleepEnd;
     };

 STATICDT PCTIMER  pctWriteBase;
 STATICDT struct _timerstuff * pCurTimerStuff;
 STATICDT struct _timerstuff * pTimerStuff;
 STATICDT HGLOBAL  hMemTimers;

 STATICDT struct _timerriff {
     FOURCC   fccRIFF;       // 'RIFF'
     DWORD    cbTotal;       // total (inclusive) size of riff data
     FOURCC   fccJMKD;       // 'JMKD' data type identifier
     DWORD    fccVCHD;       // 'VCHD' capture data header
     DWORD    cbVCHD;        // sizeof vchd data
     struct _vchd {
         DWORD            nPrio;
         DWORD            dwFramesCaptured;
         DWORD            dwFramesDropped;
         DWORD            dwDropFramesAppended;
         DWORD            dwDropFramesNotAppended;
         DWORD            dwTimerFrequency;
         DWORD            dwSpare[2];
         CAPTUREPARMS     cap;
         BITMAPINFOHEADER bmih;
         DWORD            nMaxVideoBuffers;
         struct _thkvideohdr {
             VIDEOHDR vh;
             LPBYTE   p32Buff;
             DWORD    p16Alloc;
             DWORD    dwMemHandle;
             DWORD    dwReserved;
             }            atvh[64];
         }    vchd;
     DWORD    fccChunk;      // chunk data type tag
     DWORD    cbChunk;       // non-inclusive size of chunk data
     } * pTimerRiff;

 STATICDT UINT nTimerIndex;
 STATICDT UINT nSleepCount;
#endif

#ifdef _DEBUG
    #define DSTATUS(lpcs, sz) statusUpdateStatus(lpcs, IDS_CAP_INFO, (LPTSTR) TEXT(sz))
#else
    #define DSTATUS(lpcs, sz)
#endif

// Allocate memory on a sector boundary

LPVOID FAR PASCAL AllocSectorAlignedMem (DWORD dwRequest, DWORD dwAlign)
{
    LPVOID pbuf;

    dwRequest = (DWORD) ROUNDUPTOSECTORSIZE (dwRequest, dwAlign) + dwAlign;      // round up to next page boundary

    pbuf = VirtualAlloc (NULL, dwRequest,
                        MEM_COMMIT | MEM_RESERVE,
                        PAGE_READWRITE);
    AuxDebugEx(4, DEBUGLINE "Allocated %d bytes of sector aligned memory at %8x\r\n", dwRequest, pbuf);
    return pbuf;
}

void FAR PASCAL FreeSectorAlignedMem (LPVOID pbuf)
{
   // the pointer we free had better be aligned on at least a 256 byte
   // boundary
   //
   assert (!((DWORD_PTR)pbuf & 255));
   VirtualFree ((LPVOID)((DWORD_PTR)pbuf & ~255), 0, MEM_RELEASE);
}

#define ONEMEG (1024L * 1024L)

DWORDLONG GetFreePhysicalMemory(void)
{
    MEMORYSTATUSEX ms;
    
	ms.dwLength = sizeof(ms);

    GlobalMemoryStatusEx(&ms);

    if (ms.ullTotalPhys > 8L * ONEMEG)
        return ms.ullTotalPhys - ONEMEG * 4;

    return(ms.ullTotalPhys /2);
}

// ****************************************************************
// ******************** Audio Buffer Control **********************
// ****************************************************************

// Audio buffers are always allocated under the presumption that
// audio capture may be enabled at any time.
// AVIAudioInit must be matched with AVIAudioFini (both only called once)
// AVIAudioPrepare must be matched with AVIAudioUnPrepare
//      (which may be called multiple times to enable and disable audio)


// AVI AudioInit - Allocate and initialize buffers for audio capture.
//                 This routine is also used by MCI capture.
//                 Returns: 0 on success, otherwise an error code.

UINT AVIAudioInit (LPCAPSTREAM lpcs)
{
    int		i;
    LPVOID      pHdr;
    LPVOID      p;

    if (lpcs->sCapParms.wNumAudioRequested == 0)
        lpcs->sCapParms.wNumAudioRequested = DEF_WAVE_BUFFERS;

    // .5 second of audio per buffer (or 10K, whichever is larger)
    if (lpcs->sCapParms.dwAudioBufferSize == 0)
        lpcs->dwWaveSize = CalcWaveBufferSize (lpcs);
    else {
        lpcs->dwWaveSize = 0;
        if (lpcs->lpWaveFormat)
            lpcs->dwWaveSize = lpcs->sCapParms.dwAudioBufferSize;
    }

    // Alloc the wave memory
    for(i = 0; i < (int)lpcs->sCapParms.wNumAudioRequested; i++) {

        pHdr = GlobalAllocPtr(GPTR, sizeof(WAVEHDR));

        if (pHdr == NULL)
            break;

        lpcs->alpWaveHdr[i] = pHdr;

        p = AllocSectorAlignedMem( sizeof(RIFF) + lpcs->dwWaveSize, lpcs->dwBytesPerSector);
        if (p == NULL) {
            GlobalFreePtr (pHdr);
            lpcs->alpWaveHdr[i] = NULL;
            break;
        }

        lpcs->alpWaveHdr[i]->lpData          = (LPBYTE)p + sizeof(RIFF);
        lpcs->alpWaveHdr[i]->dwBufferLength  = lpcs->dwWaveSize;
        lpcs->alpWaveHdr[i]->dwBytesRecorded = 0;
        lpcs->alpWaveHdr[i]->dwUser          = 0;
        lpcs->alpWaveHdr[i]->dwFlags         = 0;
        lpcs->alpWaveHdr[i]->dwLoops         = 0;

        ((LPRIFF)p)->dwType = MAKEAVICKID(cktypeWAVEbytes, 1);
        ((LPRIFF)p)->dwSize = lpcs->dwWaveSize;
    }

    lpcs->iNumAudio = i;

    return ((lpcs->iNumAudio == 0) ? IDS_CAP_WAVE_ALLOC_ERROR : 0);
}


//
// AVI AudioFini    - UnPrepares headers
//                      This routine is also used by MCI capture.
//                      Returns: 0 on success, otherwise an error code.

UINT AVIAudioFini (LPCAPSTREAM lpcs)
{
    int ii;

    /* free headers and data */
    for (ii=0; ii < MAX_WAVE_BUFFERS; ++ii) {
        if (lpcs->alpWaveHdr[ii]) {
            if (lpcs->alpWaveHdr[ii]->lpData)
                FreeSectorAlignedMem((LPBYTE)lpcs->alpWaveHdr[ii]->lpData - sizeof (RIFF));
            GlobalFreePtr(lpcs->alpWaveHdr[ii]);
            lpcs->alpWaveHdr[ii] = NULL;
        }
    }

    return 0;
}

//
// AVI AudioPrepare - Opens the wave device and adds the buffers
//                    Prepares headers and adds buffers to the device
//                    This routine is also used by MCI capture.
//                    Returns: 0 on success, otherwise an error code.

UINT AVIAudioPrepare (LPCAPSTREAM lpcs)
{
    UINT uiError;
    int  ii;

    /* See if we can open that format for input */

    // register event callback to avoid polling

    uiError = waveInOpen(&lpcs->hWaveIn,
        WAVE_MAPPER, lpcs->lpWaveFormat,
        (DWORD_PTR) lpcs->hCaptureEvent,  0, CALLBACK_EVENT );

    if (uiError != MMSYSERR_NOERROR)
        return IDS_CAP_WAVE_OPEN_ERROR;

    lpcs->fAudioYield = FALSE; // ACM is separate thread, don't yield
    lpcs->fAudioBreak = FALSE;

    DPF("AudioYield = %d", lpcs->fAudioYield);

    for (ii = 0; ii < (int)lpcs->sCapParms.wNumAudioRequested; ++ii) {

        if (waveInPrepareHeader (lpcs->hWaveIn, lpcs->alpWaveHdr[ii],
                                 sizeof(WAVEHDR)))
            return IDS_CAP_WAVE_ALLOC_ERROR;

        if (waveInAddBuffer (lpcs->hWaveIn, lpcs->alpWaveHdr[ii],
                             sizeof(WAVEHDR)))
            return IDS_CAP_WAVE_ALLOC_ERROR;
	AuxDebugEx(3, DEBUGLINE "Added wave buffer %d (%8x)\r\n", ii, lpcs->alpWaveHdr[ii]);
    }

    lpcs->iNextWave = 0;        // current wave
    lpcs->dwWaveBytes = 0L;     // number of wave bytes
    lpcs->dwWaveChunkCount = 0; // number of wave frames

    return 0;
}

//
// AVI AudioUnPrepare - UnPrepares headers and closes the wave device.
//                      This routine is also used by MCI capture.
//                      Returns: 0 on success, otherwise an error code.

UINT AVIAudioUnPrepare (LPCAPSTREAM lpcs)
{
    int ii;

    if (lpcs->hWaveIn)
    {
        waveInReset(lpcs->hWaveIn);

        // unprepare any headers that have been prepared
        //
        for (ii=0; ii < lpcs->iNumAudio; ++ii)
            if (lpcs->alpWaveHdr[ii] &&
                (lpcs->alpWaveHdr[ii]->dwFlags & WHDR_PREPARED))
                waveInUnprepareHeader (lpcs->hWaveIn,
                                       lpcs->alpWaveHdr[ii],
                                       sizeof(WAVEHDR));

        waveInClose(lpcs->hWaveIn);
        lpcs->hWaveIn = NULL;
    }

    return 0;
}

// ****************************************************************
// ******************** Video Buffer Control **********************
// ****************************************************************

#if defined CHICAGO

// Win95 capavi code
// AVIVideoInit -  Allocates, and initialize buffers for video capture.
//                 This routine is also used by MCI capture.
//                 Returns: 0 on success, otherwise an error code.

UINT AVIVideoInit (LPCAPSTREAM lpcs)
{
    UINT           iMaxVideo;
    DWORD          mmr;
    LPTHKVIDEOHDR  ptvh;
    UINT           ii;
    DWORD          cbVideo;

    lpcs->iNumVideo = 0;
    lpcs->iNextVideo = 0;
    lpcs->dwVideoChunkCount = 0;
    lpcs->dwFramesDropped = 0;
    lpcs->fBuffersOnHardware = FALSE;

    // When performing MCI step capture, buffer array is not used
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

    cbVideo = ROUNDUPTOSECTORSIZE (lpcs->lpBitsInfo->bmiHeader.biSizeImage
                                   + sizeof(RIFF),
                                   lpcs->dwBytesPerSector)
              + lpcs->dwBytesPerSector;

    // If the user hasn't specified the number of video buffers to use,
    // assume the minimum

    if (lpcs->sCapParms.wNumVideoRequested == 0) {
        iMaxVideo = lpcs->sCapParms.wNumVideoRequested = MIN_VIDEO_BUFFERS;
	lpcs->fCaptureFlags |= CAP_fDefaultVideoBuffers;
    } else {
	// use the number of video buffers that the user requested
	// or the maximum that will fit in memory.
	//
	iMaxVideo = min (MAX_VIDEO_BUFFERS, lpcs->sCapParms.wNumVideoRequested);
    }

    if (iMaxVideo > 1)
    {
        DWORDLONG dwFreeMem;
        DWORDLONG dwUserRequests;
        DWORDLONG dwAudioMem;

        // How much actual free physical memory exists?
        dwFreeMem = GetFreePhysicalMemory();
        dwAudioMem = lpcs->dwWaveSize * lpcs->sCapParms.wNumAudioRequested;

        #define FOREVER_FREE 32768L   // Always keep this free for swap space

        // How much memory will be used if we allocate per the request?
        //
        dwUserRequests = dwAudioMem
                         + cbVideo * iMaxVideo
                         + FOREVER_FREE;

        // If request is greater than available memory, force fewer buffers
        //
        if (dwUserRequests > dwFreeMem)
        {
            if (dwFreeMem > dwAudioMem)
                dwFreeMem -= dwAudioMem;
            iMaxVideo = (int)(((dwFreeMem * 8) / 10) / cbVideo);
            iMaxVideo = min (MAX_VIDEO_BUFFERS, iMaxVideo);
            dprintf("iMaxVideo = %d\n", iMaxVideo);
        }
    }

    mmr = vidxAllocHeaders(lpcs->hVideoIn, iMaxVideo, &ptvh);
    if (mmr != MMSYSERR_NOERROR)
        return IDS_CAP_VIDEO_ALLOC_ERROR;

    AuxDebugEx (3, DEBUGLINE "vidxAllocHdrs returned ptvh=%X\r\n", ptvh);
    AuxDebugDump (8, ptvh, sizeof(*ptvh) * iMaxVideo);

    for (ii = 0; ii < iMaxVideo; ++ii)
    {
        LPVIDEOHDR pvh = NULL;
        LPRIFF     priff;

        // in chicago we let the thunk layer allocate memory
        // so that we can be assured that the memory can be easily
        // thunked.
        //
        // the pointer will be rounded up to a sector size boundary
        //
        mmr = vidxAllocBuffer (lpcs->hVideoIn, ii, &ptvh, cbVideo);
        if ((mmr != MMSYSERR_NOERROR) || (ptvh == NULL))
            break;

        lpcs->alpVideoHdr[ii] = pvh = &ptvh->vh;

        // vidxAllocBuffer actually returns a couple of extra fields
        // after the video header. the first of these holds the
        // linear address of the buffer.
        //
        priff = (LPVOID) ROUNDUPTOSECTORSIZE (ptvh->p32Buff, lpcs->dwBytesPerSector);

       #ifdef DEBUG
        {
        LPBYTE pb = (LPVOID)ptvh->p32Buff;
        AuxDebugEx (4, DEBUGLINE "buffer[%d] at %x linear. Doing touch test\r\n",
                    ii, ptvh->p32Buff);
        pb[0] = 0;
        pb[cbVideo-1] = 0;
        }
       #endif

        // write the riff header for this chunk.
        //
	priff->dwType = MAKEAVICKID(cktypeDIBbits, 0);
        if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
            priff->dwType = MAKEAVICKID(cktypeDIBcompressed, 0);
        priff->dwSize = lpcs->lpBitsInfo->bmiHeader.biSizeImage;

        // init the video header
        //
        pvh->lpData = (LPVOID)(priff + 1);
        pvh->dwBufferLength  = priff->dwSize;
        pvh->dwBytesUsed     = 0;
        pvh->dwTimeCaptured  = 0;
        pvh->dwUser          = 0;
        pvh->dwFlags         = 0;

        AuxDebugEx (4, DEBUGLINE "lpVideoHdr[%d]==%X\r\n", ii, lpcs->alpVideoHdr[ii]);
        AuxDebugDump (8, lpcs->alpVideoHdr[ii], sizeof(*ptvh));
    }
    lpcs->iNumVideo = ii;
    lpcs->cbVideoAllocation = cbVideo;

    dprintf("cbVideo = %ld \n", cbVideo);
    dprintf("iNumVideo Allocated = %d \n", lpcs->iNumVideo);
    return lpcs->iNumVideo ? 0 : IDS_CAP_VIDEO_ALLOC_ERROR;
}

//
// AVIVideoPrepare -  Prepares headers and adds buffers to the device
//                    This routine is also used by MCI capture.
//                    Returns: 0 on success, otherwise an error code.

UINT AVIVideoPrepare (LPCAPSTREAM lpcs)
{
    int ii;

    // When performing MCI step capture, buffer array is not used
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

   #ifdef JMK_HACK_CHECKHDR
    {
    LPTHKVIDEOHDR lptvh = (LPVOID)lpcs->alpVideoHdr[0];

    if (HIWORD(lptvh->vh.lpData) != HIWORD(lptvh->p32Buff))
        {
        AuxDebugEx (0, DEBUGLINE "before stream init: hdr trouble\r\n");

        AuxDebugEx (0, DEBUGLINE "iNext=%d, ptvh=%X\r\n", lpcs->iNextVideo, lptvh);
        AuxDebugDump (0, lptvh, sizeof(*lptvh));
        AuxDebugEx (0, DEBUGLINE "alpVideoHdrs=%X\r\n", lpcs->alpVideoHdr);
        AuxDebugDump (0, lpcs->alpVideoHdr, sizeof(lpcs->alpVideoHdr[0]) * 8);

        INLINE_BREAK;
        return IDS_CAP_VIDEO_OPEN_ERROR;
        }
    }
   #endif

    // Open the video stream, setting the capture rate
    //
    if (videoStreamInit(lpcs->hVideoIn,
                        lpcs->sCapParms.dwRequestMicroSecPerFrame,
                        lpcs->hRing0CapEvt,
                        0,
                        CALLBACK_EVENT))
    {
        dprintf("cant open video device!\n");
        return IDS_CAP_VIDEO_OPEN_ERROR;
    }

   #ifdef JMK_HACK_CHECKHDR
    {
    LPTHKVIDEOHDR lptvh = (LPVOID)lpcs->alpVideoHdr[0];

    if (HIWORD(lptvh->vh.lpData) != HIWORD(lptvh->p32Buff))
        {
        AuxDebugEx (0, DEBUGLINE "after stream init: hdr trouble\r\n");

        AuxDebugEx (0, DEBUGLINE "iNext=%d, ptvh=%X\r\n", lpcs->iNextVideo, lptvh);
        AuxDebugDump (0, lptvh, sizeof(*lptvh));
        AuxDebugEx (0, DEBUGLINE "alpVideoHdrs=%X\r\n", lpcs->alpVideoHdr);
        AuxDebugDump (0, lpcs->alpVideoHdr, sizeof(lpcs->alpVideoHdr[0]) * 8);

        INLINE_BREAK;
        return IDS_CAP_VIDEO_OPEN_ERROR;
        }
    }
   #endif

    // Prepare (lock) the buffers, and give them to the device
    //
    for (ii = 0; ii < lpcs->iNumVideo; ++ii)
    {
        if (vidxAddBuffer (lpcs->hVideoIn,
                           lpcs->alpVideoHdr[ii],
                           sizeof(VIDEOHDR)))
        {
            lpcs->iNumVideo = ii;
            dprintf("**** could only prepare %d Video buffers!\n", lpcs->iNumVideo);
            break;
        }
    }

   #ifdef JMK_HACK_CHECKHDR
    {
    LPTHKVIDEOHDR lptvh = (LPVOID)lpcs->alpVideoHdr[0];

    if (IsBadWritePtr (lptvh, sizeof(*lptvh)) ||
        HIWORD(lptvh->vh.lpData) != HIWORD(lptvh->p16Alloc))
        {
        AuxDebugEx (0, DEBUGLINE "after add buffers: hdr trouble\r\n");

        AuxDebugEx (0, DEBUGLINE "iNext=%d, ptvh=%X\r\n", lpcs->iNextVideo, lptvh);
        AuxDebugDump (0, lptvh, sizeof(*lptvh));
        AuxDebugEx (0, DEBUGLINE "alpVideoHdrs=%X\r\n", lpcs->alpVideoHdr);
        AuxDebugDump (0, lpcs->alpVideoHdr, sizeof(lpcs->alpVideoHdr[0]) * 8);

        INLINE_BREAK;
        return IDS_CAP_VIDEO_OPEN_ERROR;
        }
    }
   #endif

    return 0;
}

#else // code below is !CHICAGO

// this structure is used to keep track of memory allocation
// for video buffers used in capture.  it is allocated when
// allocating a videohdr would be called for
//
typedef struct _cap_videohdr {
    VIDEOHDR vh;
    LPBYTE   pAlloc;      // address of allocated buffer
    DWORD    dwMemIdent;  // identity of allocation (used in Chicago)
    DWORD    dwReserved;  // used in chicago
    BOOL     bHwBuffer;   // TRUE if buffer is allocated using videoStreamAllocBuffer
} CAPVIDEOHDR, FAR *LPCAPVIDEOHDR;

// AVIVideoInit -  Allocates, and initialize buffers for video capture.
//                 This routine is also used by MCI capture.
//                 Returns: 0 on success, otherwise an error code.

UINT AVIVideoInit (LPCAPSTREAM lpcs)
{
    int            iMaxVideo;
    int		   ii;
    LPCAPVIDEOHDR  pcvh;
    LPVOID         pbuf;
    DWORD          cbVideo;
    BOOL	   fAllowHardwareBuffers;


//#define SINGLEHEADERBLOCK

    lpcs->iNumVideo = 0;
    lpcs->iNextVideo = 0;
    lpcs->dwVideoChunkCount = 0;
    lpcs->dwFramesDropped = 0;
    lpcs->fBuffersOnHardware = FALSE;
    fAllowHardwareBuffers = GetProfileIntA ("Avicap32", "AllowHardwareBuffers", TRUE);

    // When performing MCI step capture, buffer array is not used
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

    cbVideo = (DWORD) ROUNDUPTOSECTORSIZE (lpcs->lpBitsInfo->bmiHeader.biSizeImage
                                   + sizeof(RIFF),
                                   lpcs->dwBytesPerSector)
              + lpcs->dwBytesPerSector;

    // If the user hasn't specified the number of video buffers to use,
    // assume the minimum

    if (lpcs->sCapParms.wNumVideoRequested == 0) {
	UINT cDefaultVideoBuffers = GetProfileIntA ("Avicap32", "nVideoBuffers", MIN_VIDEO_BUFFERS);
	cDefaultVideoBuffers = min(MAX_VIDEO_BUFFERS, max(MIN_VIDEO_BUFFERS, cDefaultVideoBuffers));
        iMaxVideo = lpcs->sCapParms.wNumVideoRequested = cDefaultVideoBuffers;
	lpcs->fCaptureFlags |= CAP_fDefaultVideoBuffers;
    } else {
	// use the number of video buffers that the user requested
	// or the maximum that will fit in memory.
	//
	iMaxVideo = min (MAX_VIDEO_BUFFERS, lpcs->sCapParms.wNumVideoRequested);
    }

    // Post VFW 1.1a, see if the driver can allocate memory
    //
   #ifdef ALLOW_HW_BUFFERS
    if (fAllowHardwareBuffers && (videoStreamAllocBuffer (lpcs->hVideoIn, (LPVOID *) &pbuf, cbVideo)
        == DV_ERR_OK))
    {
	DWORD dwRet;
	dprintf("Allocated test h/w buffer at address %8x, size %d bytes", pbuf, cbVideo);
        lpcs->fBuffersOnHardware = TRUE;
        dwRet = videoStreamFreeBuffer (lpcs->hVideoIn, pbuf);

	dprintf("Freed test h/w buffer at address %8x, retcode 0x%x", pbuf, dwRet);
    }
    else
   #endif
    {
        DWORDLONG dwFreeMem;
        DWORDLONG dwUserRequests;
        DWORDLONG dwAudioMem;

        lpcs->fBuffersOnHardware = FALSE;

        // How much actual free physical memory exists?
        dwFreeMem = GetFreePhysicalMemory();
        dwAudioMem = lpcs->dwWaveSize * lpcs->sCapParms.wNumAudioRequested;

        #define FOREVER_FREE 32768L   // Always keep this free for swap space

        // How much memory will be used if we allocate per the request?
        //
        dwUserRequests = dwAudioMem
                         + cbVideo * iMaxVideo
                         + FOREVER_FREE;

        // If request is greater than available memory, force fewer buffers
        //
        if (dwUserRequests > dwFreeMem)
        {
            if (dwFreeMem > dwAudioMem)
                dwFreeMem -= dwAudioMem;
            iMaxVideo = (int)(((dwFreeMem * 8) / 10) / cbVideo);
            iMaxVideo = min (MAX_VIDEO_BUFFERS, iMaxVideo);
            dprintf("iMaxVideo = %d\n", iMaxVideo);
        }
    }

#ifdef SINGLEHEADERBLOCK
    pcvh = GlobalAllocPtr (GMEM_MOVEABLE, iMaxVideo * sizeof(CAPVIDEOHDR));
    // note: pcvh is freed by referencing through alpVideoHdr[0]
    if ( ! pcvh)
        return IDS_CAP_VIDEO_ALLOC_ERROR;

    AuxDebugEx (3, DEBUGLINE "allocated video headers pcvh=%X\r\n", pcvh);
#endif

    // Set up the buffers presuming fixed size DIBs and Junk chunks
    // These will be modified later if the device provides compressed data

    for (ii = 0; ii < iMaxVideo; ++ii)
    {
        LPVIDEOHDR pvh = NULL;
        LPRIFF     priff;

#ifndef SINGLEHEADERBLOCK
        pcvh = (LPCAPVIDEOHDR)GlobalAllocPtr(GMEM_MOVEABLE, sizeof(CAPVIDEOHDR));
        if (pcvh== NULL)
            break;
        lpcs->alpVideoHdr[ii] = (LPVIDEOHDR)pcvh;
        ZeroMemory(pcvh, sizeof (CAPVIDEOHDR));
#endif		

       #ifdef ALLOW_HW_BUFFERS
        //
        // for the first buffer, always try to allocate on hardware,
	// NO.  If we are not to use hardware buffers, then do not use them.
        // if that fails, grab virtual memory for the buffer.
        // for all but the first buffer, we use whatever worked for
        // the first buffer, and if that fails.  we stop allocating buffers
        //
        if (lpcs->fBuffersOnHardware)
        {
	    MMRESULT mmr;
            pbuf = NULL;
            mmr = videoStreamAllocBuffer (lpcs->hVideoIn, (LPVOID) &pbuf, cbVideo);
            if ((mmr != MMSYSERR_NOERROR) || (pbuf == NULL))
            {
                if (0 == ii)
                    break;  // nothing allocated

	        dprintf("Failed to allocate hardware buffer %d, rc=0x%x", ii, mmr);

		// if the user did not ask for a specific number of buffers,
		// or the hardware is set up to work with ONLY hardware
		// allocated buffers, take what we've got and work with that.
		if ((lpcs->fCaptureFlags & CAP_fDefaultVideoBuffers)
		  || (GetProfileIntA ("Avicap32", "HardwareBuffersOnly", FALSE)))
		{
		    break;
		}

                lpcs->fBuffersOnHardware = FALSE;
		// use normal memory for the remaining video buffers.
                pbuf = AllocSectorAlignedMem (cbVideo, lpcs->dwBytesPerSector);
            }
            else {
                lpcs->fBuffersOnHardware = TRUE;
		dprintf("Allocated hardware buffer %d at address %8x", ii, pbuf);
	    }
        }
        else
            pbuf = AllocSectorAlignedMem (cbVideo, lpcs->dwBytesPerSector);

       #else ! dont allow hw buffers

        pbuf = AllocSectorAlignedMem (cbVideo, lpcs->dwBytesPerSector);

       #endif // ALLOW_HW_BUFFERS

        if (pbuf == NULL) {
#ifndef SINGLEHEADERBLOCK
	    GlobalFreePtr(pcvh);
	    lpcs->alpVideoHdr[ii] = NULL;
#endif		
            break;
	}

        // save the original allocation pointer to the buffer
        // in the extra fields of the capture header. also remember
        // whether we got the buffer from the driver or not
        //
#ifndef SINGLEHEADERBLOCK
        pcvh->pAlloc = pbuf;
        pcvh->bHwBuffer = lpcs->fBuffersOnHardware;
        lpcs->alpVideoHdr[ii] = pvh = &pcvh->vh;
#else
        pcvh[ii].pAlloc = pbuf;
        pcvh[ii].bHwBuffer = lpcs->fBuffersOnHardware;
        lpcs->alpVideoHdr[ii] = pvh = &pcvh[ii].vh;
#endif		
        priff = (LPVOID) ROUNDUPTOSECTORSIZE (pbuf, lpcs->dwBytesPerSector);

        // write the riff header for this frame
        //
	priff->dwType = MAKEAVICKID(cktypeDIBbits, 0);
        if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
            priff->dwType = MAKEAVICKID(cktypeDIBcompressed, 0);
        priff->dwSize = lpcs->lpBitsInfo->bmiHeader.biSizeImage;

        // fill in the video hdr for this frame
        //
        pvh->lpData          = (LPVOID)(priff + 1);
        pvh->dwBufferLength  = priff->dwSize;
        pvh->dwBytesUsed     = 0;
        pvh->dwTimeCaptured  = 0;
        pvh->dwUser          = 0;
        pvh->dwFlags         = 0;

        AuxDebugEx (4, DEBUGLINE "lpVideoHdr[%d]==%X\r\n", ii, lpcs->alpVideoHdr[ii]);
        AuxDebugDump (8, lpcs->alpVideoHdr[ii], sizeof(*pcvh));
    }
    lpcs->iNumVideo = ii;
    lpcs->cbVideoAllocation = cbVideo;

    // if we did not create even a single buffer, free the headers
    //

#ifdef SINGLEHEADERBLOCK
    if ( ! lpcs->iNumVideo)
        GlobalFreePtr (pcvh);
#else
    // we allocate video headers as we proceed.  There is nothing to free
#endif

   #ifdef ALLOW_HW_BUFFERS
    if (lpcs->fBuffersOnHardware)
        dprintf("HARDWARE iNumVideo Allocated = %d \n", lpcs->iNumVideo);
    else
   #endif
        dprintf("HIGH iNumVideo Allocated = %d \n", lpcs->iNumVideo);

    return lpcs->iNumVideo ? 0 : IDS_CAP_VIDEO_ALLOC_ERROR;
}

void CALLBACK
VideoCallback(
    HVIDEO hvideo,
    UINT msg,
    DWORD_PTR dwInstance,
    DWORD_PTR lParam1,
    DWORD_PTR lParam2
)
{
    LPCAPSTREAM lpcs = (LPCAPSTREAM) dwInstance;

    if (lpcs && lpcs->hCaptureEvent) {
        SetEvent(lpcs->hCaptureEvent);
    } else {
	AuxDebugEx(1, DEBUGLINE "VideoCallback with NO instance data\r\n");
    }
}

//
// AVIVideoPrepare -  Prepares headers and adds buffers to the device
//                    This routine is also used by MCI capture.
//                    Returns: 0 on success, otherwise an error code.
UINT AVIVideoPrepare (LPCAPSTREAM lpcs)
{
    MMRESULT mmr;
    int      ii;

    // When performing MCI step capture, buffer array is not used
    //
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

    // Open the video stream, setting the capture rate
    //
    mmr = videoStreamInit (lpcs->hVideoIn,
                           lpcs->sCapParms.dwRequestMicroSecPerFrame,
                           (DWORD_PTR) VideoCallback,
                           (DWORD_PTR) lpcs,
                           CALLBACK_FUNCTION);
    if (mmr) {
        dprintf("cannot open video device!  Error is %d\n", mmr);
        return IDS_CAP_VIDEO_OPEN_ERROR;
    }

    // Prepare (lock) the buffers, and give them to the device
    //
    for (ii = 0; ii < lpcs->iNumVideo; ++ii)
    {
        mmr = videoStreamPrepareHeader (lpcs->hVideoIn,
                                        lpcs->alpVideoHdr[ii],
                                        sizeof(VIDEOHDR));
        if (mmr)
        {
            lpcs->iNumVideo = ii;
            dprintf("**** could only prepare %d Video buffers!\n", lpcs->iNumVideo);
            break;
        }

        mmr = videoStreamAddBuffer (lpcs->hVideoIn,
                                    lpcs->alpVideoHdr[ii],
                                    sizeof(VIDEOHDR));
        if (mmr)
             return IDS_CAP_VIDEO_ALLOC_ERROR;
	AuxDebugEx(3, DEBUGLINE "Added video buffer %d (%8x)\r\n", ii, lpcs->alpVideoHdr[ii]);
    }
    return 0;
}

#endif // not chicago

//
// AVI VideoUnPrepare - UnPrepares headers, frees memory, and
//                      resets the video in device.
//                      This routine is also used by MCI capture.
//                      Returns: 0 on success, otherwise an error code.

UINT AVIVideoUnPrepare (LPCAPSTREAM lpcs)
{
    // When performing MCI step capture, buffer array is not used
    //
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

    // Reset the buffers so they can be freed
    //
    if (lpcs->hVideoIn) {
        videoStreamReset(lpcs->hVideoIn);

        // unprepare headers
        // Unlock and free headers and data

       #if defined CHICAGO
        vidxFreeHeaders (lpcs->hVideoIn);
        ZeroMemory (lpcs->alpVideoHdr, sizeof(lpcs->alpVideoHdr));
       #else
        {
            int ii;
#ifdef SINGLEHEADERBLOCK
            LPCAPVIDEOHDR pcvhAll = (LPVOID)lpcs->alpVideoHdr[0];
#endif

            for (ii = 0; ii < lpcs->iNumVideo; ++ii)
            {
                LPCAPVIDEOHDR pcvh = (LPVOID)lpcs->alpVideoHdr[ii];
                if (pcvh)
                {
                    if (pcvh->vh.dwFlags & VHDR_PREPARED)
                        videoStreamUnprepareHeader (lpcs->hVideoIn,
                                                    &pcvh->vh,
                                                    sizeof(VIDEOHDR));

                    if (pcvh->pAlloc) {
                     #ifdef ALLOW_HW_BUFFERS
			if (pcvh->bHwBuffer)
			{
			    dprintf("Freeing hardware buffer %d at address %8x", ii, pcvh->pAlloc);
                            videoStreamFreeBuffer (lpcs->hVideoIn, (LPVOID)pcvh->pAlloc);
			}
			else
                     #endif
			{
			dprintf("Freeing video buffer %d at address %8x", ii, pcvh->pAlloc);
			if (lpcs->fBuffersOnHardware) DebugBreak();

                        FreeSectorAlignedMem (pcvh->pAlloc);
			}
                    } else {
			dprintf("NO buffer allocated for index %d", ii);
		    }

#ifndef SINGLEHEADERBLOCK
		    GlobalFreePtr(pcvh);
#endif
		    lpcs->alpVideoHdr[ii] = NULL;
                } else {
		    dprintf("NO video header for index %d", ii);
		}
	    }

#ifdef SINGLEHEADERBLOCK
            // free the array of video headers
            //
            if (pcvhAll) {
                GlobalFreePtr (pcvhAll);
	    }
#endif
	}
       #endif
        // Shut down the video stream
        videoStreamFini(lpcs->hVideoIn);
    }

    return 0;
}

/*
 *  AVI Fini    - undo the mess that AVIInit did.
 *
 */
void AVIFini(LPCAPSTREAM lpcs)
{
    AuxDebugEx (2, "AVIFini(%08x)\r\n", lpcs);

    if (lpcs->lpDropFrame) {
        FreeSectorAlignedMem (lpcs->lpDropFrame), lpcs->lpDropFrame = NULL;
    }

    AVIVideoUnPrepare (lpcs);           // Free the video device and buffers
    AVIAudioUnPrepare (lpcs);           // Free the audio device
    AVIAudioFini (lpcs);                // Free the audio buffers

    if (lpcs->hCaptureEvent) {
        CloseHandle (lpcs->hCaptureEvent), lpcs->hCaptureEvent = NULL;
    }

    if (lpcs->heSyncWrite) {
        CloseHandle (lpcs->heSyncWrite), lpcs->heSyncWrite = NULL;
    }

    if (lpcs->hCompletionPort) {
        CloseHandle (lpcs->hCompletionPort), lpcs->hCompletionPort = NULL;
    }

    if (hmodKernel) {
        pfnCreateIoCompletionPort = NULL;
        pfnGetQueuedCompletionStatus = NULL;
        FreeLibrary(hmodKernel);
        hmodKernel = 0;
    }

    AuxDebugEx (2, "AVIFini(...) exits\r\n");
}

//
// AVI Init
//     This routine does all the non-File initalization for AVICapture.
//     Returns: 0 on success, Error string value on failure.
//

UINT AVIInit (LPCAPSTREAM lpcs)
{
    UINT         wError = 0;    // Success
    LPBITMAPINFO lpBitsInfoOut;    // Possibly compressed output format

    // Allocate a DropFrame buffer
    if (lpcs->lpDropFrame == NULL) {
        assert (lpcs->dwBytesPerSector);
        lpcs->lpDropFrame = AllocSectorAlignedMem (lpcs->dwBytesPerSector, lpcs->dwBytesPerSector);
    }

    /* No special video format given -- use the default */
   #ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic == NULL)
        lpBitsInfoOut = lpcs->lpBitsInfo;
    else
        lpBitsInfoOut = lpcs->CompVars.lpbiOut;
   #else
    lpBitsInfoOut = lpcs->lpBitsInfo;
   #endif

    // -------------------------------------------------------
    // figure out buffer sizes
    // -------------------------------------------------------

    // Init all pointers to NULL
    ZeroMemory (lpcs->alpVideoHdr, sizeof(lpcs->alpVideoHdr));
    ZeroMemory (lpcs->alpWaveHdr, sizeof(lpcs->alpWaveHdr));

    // -------------------------------------------------------
    //                    Init Sound
    // -------------------------------------------------------

    if (lpcs->sCapParms.fCaptureAudio) {
        if ((DWORD)(wError = AVIAudioInit (lpcs))) {
            dprintf("can't init audio buffers!\n");
            goto AVIInitFailed;
        }
    }

    // -------------------------------------------------------
    //                    Init Video
    // -------------------------------------------------------

    if ((DWORD)(wError = AVIVideoInit (lpcs))) {
        dprintf("AVIVideoInitFailed (no buffers alloc'd)!\n");
        goto AVIInitFailed;
    }

    // --------------------------------------------------------------
    //  Prepare audio buffers (lock em down) and give them to the device
    // --------------------------------------------------------------

    if (lpcs->sCapParms.fCaptureAudio) {
        if ((DWORD)(wError = AVIAudioPrepare (lpcs))) {
            dprintf("can't prepare audio buffers!\n");
            goto AVIInitFailed;
        }
    }

    // --------------------------------------------------------------
    //  Prepare video buffers (lock em down) and give them to the device
    // --------------------------------------------------------------

    if ((DWORD)(wError = AVIVideoPrepare (lpcs))) {
        dprintf("can't prepare video buffers!\n");
        goto AVIInitFailed;
    }

    // -------------------------------------------------------
    //   all done, return success
    // -------------------------------------------------------

    return (0);            // SUCCESS !

    // -------------------------------------------------------
    //   we got a error, return string ID of error message
    // -------------------------------------------------------
AVIInitFailed:
    AVIFini(lpcs);      // Shutdown everything
    return wError;
}

// Maintains info chunks which are written to the AVI header
//
BOOL FAR PASCAL SetInfoChunk(LPCAPSTREAM lpcs, LPCAPINFOCHUNK lpcic)
{
    DWORD       ckid   = lpcic->fccInfoID;
    LPVOID      lpData = lpcic->lpData;
    LONG        cbData = lpcic->cbData;
    LPBYTE      lp;
    LPBYTE      lpw;
    LPBYTE      lpEnd;
    LPBYTE      lpNext;
    LONG        cbSizeThis;
    BOOL        fOK = FALSE;

    // Delete all info chunks?
    if (ckid == 0) {
        if (lpcs->lpInfoChunks) {
            GlobalFreePtr (lpcs->lpInfoChunks);
            lpcs->lpInfoChunks = NULL;
            lpcs->cbInfoChunks = 0;
        }
        return TRUE;
    }

    // Try removing an entry if it already exists...
    // Also used if lpData is NULL to just remove an entry
    // note: lpw and lpEnd are LPRIFF values... except the code is written
    // to use them as pointers to an array of DWORD values. (yuk)
    //
    lpw   = (LPBYTE)lpcs->lpInfoChunks;           // always points at fcc
    lpEnd = (LPBYTE)lpcs->lpInfoChunks + lpcs->cbInfoChunks;
    while (lpw < lpEnd) {
        cbSizeThis = ((DWORD UNALIGNED FAR *)lpw)[1];
        cbSizeThis += cbSizeThis & 1;           // force WORD (16 bit) alignment

	// Point lpNext at the next RIFF block
        lpNext = lpw + cbSizeThis + sizeof (DWORD) * 2;

	// If this info chunk is the same as that passed in... we can delete the
	// existing information
        if ((*(DWORD UNALIGNED FAR *) lpw) == ckid) {
            lpcs->cbInfoChunks -= cbSizeThis + sizeof (DWORD) * 2;
	    // could have coded: lpcs->cbInfoChunks -= lpNext - lpw;
	    // the next line should always be true...
            if (lpNext <= lpEnd) {
                if (lpEnd - lpNext)
                    CopyMemory (lpw, lpNext, lpEnd - lpNext);
                if (lpcs->cbInfoChunks) {
                    lpcs->lpInfoChunks = (LPBYTE) GlobalReAllocPtr( // shrink it
                        lpcs->lpInfoChunks,
                        lpcs->cbInfoChunks,
                        GMEM_MOVEABLE);
                }
                else {
                    if (lpcs->lpInfoChunks)
                        GlobalFreePtr (lpcs->lpInfoChunks);
                    lpcs->lpInfoChunks = NULL;
                }
                fOK = TRUE;
            }
            break;
        }
        else
            lpw = lpNext;
    }

    if (lpData == NULL || cbData == 0)         // Only deleting, get out
        return fOK;

    // Add a new entry
    cbData += cbData & 1;               // force WORD (16 bit) alignment
    cbData += sizeof(RIFF);             // add sizeof RIFF
    if (lpcs->lpInfoChunks)
        lp = GlobalReAllocPtr(lpcs->lpInfoChunks, lpcs->cbInfoChunks + cbData, GMEM_MOVEABLE);
    else
        lp = GlobalAllocPtr(GMEM_MOVEABLE, cbData);

    if (!lp)
        return FALSE;

    // Save the pointer in our status block
    lpcs->lpInfoChunks = lp;

    // build RIFF chunk in block
    //
    ((LPRIFF)(lp + lpcs->cbInfoChunks))->dwType = ckid;
    ((LPRIFF)(lp + lpcs->cbInfoChunks))->dwSize = lpcic->cbData;

    CopyMemory (lp + lpcs->cbInfoChunks + sizeof(RIFF),
                lpData,
                cbData - sizeof(RIFF));

    // Update the length of the info chunk
    lpcs->cbInfoChunks += cbData;

    return TRUE;
}

/*+ ProcessNextVideoBuffer
 *
 *-===============================================================*/

STATICFN BOOL _inline ProcessNextVideoBuffer (
    LPCAPSTREAM  lpcs,
    BOOL         fStopping,
    LPUINT       lpuError,
    LPVIDEOHDR * plpvhDraw,
    LPBOOL       lpbPending)
{
    LPVIDEOHDR lpvh;

    *lpuError = 0;
    *plpvhDraw = NULL;
    *lpbPending = FALSE;

    lpvh = lpcs->alpVideoHdr[lpcs->iNextVideo];
    if (!(lpvh->dwFlags & VHDR_DONE)) {
        return fStopping;
    }

   #if defined CHICAGO
    {
    LPTHKVIDEOHDR lptvh = (LPVOID)lpvh;

    #ifdef JMK_HACK_CHECKHDR
     if (IsBadWritePtr (lptvh, sizeof(*lptvh)) ||
        HIWORD(lptvh->vh.lpData) != HIWORD(lptvh->p16Alloc))
        {
        OutputDebugStringA(DEBUGLINE "trouble with video hdr\r\n");

        AuxDebugEx (0, DEBUGLINE "iNext=%d, ptvh=%X\r\n", lpcs->iNextVideo, lptvh);
        AuxDebugDump (0, lptvh, sizeof(*lptvh));
        AuxDebugEx (0, DEBUGLINE "alpVideoHdrs=%X\r\n", lpcs->alpVideoHdr);
        AuxDebugDump (0, lpcs->alpVideoHdr, sizeof(lpcs->alpVideoHdr[0]) * 8);

        INLINE_BREAK;

        return TRUE;
        }
    #endif

    // Swap the linear pointer back (was swapped in vidxAddBuffer)
    //
    lptvh->vh.lpData = (LPVOID)(ROUNDUPTOSECTORSIZE(lptvh->p32Buff, lpcs->dwBytesPerSector) + sizeof(RIFF));
    }
   #endif

    if (lpvh->dwBytesUsed)
    {
        DWORD  dwTime;
        DWORD  dwBytesUsed = lpvh->dwBytesUsed;
        BOOL   fKeyFrame = lpvh->dwFlags & VHDR_KEYFRAME;
        LPVOID lpData = lpvh->lpData;

        // get expected time for this frame in milliseconds
        //
        dwTime = MulDiv (lpcs->dwVideoChunkCount + 1,
                         lpcs->sCapParms.dwRequestMicroSecPerFrame,
                         1000);

       #ifdef NEW_COMPMAN
        //
        // We are automatically compressing during capture, so
        // compress the frame before we pass it on to be written
        //
        if (lpcs->CompVars.hic)
        {
            LPRIFF priff;

            dwBytesUsed = 0;
            lpData = ICSeqCompressFrame(&lpcs->CompVars, 0,
                                        lpvh->lpData,
                                        &fKeyFrame,
                                        &dwBytesUsed);

            priff = ((LPRIFF)lpData) -1;
            priff->dwType = MAKEAVICKID(cktypeDIBbits, 0);
            priff->dwSize = dwBytesUsed;
        }
       #endif // NEW_COMPMAN

        // do video stream callback for this frame
        //
        if (lpcs->CallbackOnVideoStream)
            lpcs->CallbackOnVideoStream (lpcs->hwnd, lpvh);
        lpvh->dwFlags &= ~VHDR_DONE;

        // if we are not capturing to disk, just increment
        // the 'chunk count' (i.e. frame count?) and go on
        // otherwise we want to queue the frame up to write
        // here
        //
        if ( ! (lpcs->fCaptureFlags & CAP_fCapturingToDisk))
        {
            // Warning: Kludge to create frame chunk count when net capture
            // follows.
            ++lpcs->dwVideoChunkCount;
        }
        else
        {
            int nAppendDummyFrames = 0;

            // if the expected time for this frame is less than the
            // timestamp for the frame.  we may have dropped some frames
            // before this frame.
            //
            if (lpcs->dwVideoChunkCount && (dwTime < lpvh->dwTimeCaptured))
            {
                int  nDropCount;
                BOOL bPending;

                // calculate how many frames have been dropped.
                // NOTE: this number may be zero if dwTimeCaptured is just
                // a little bit late.
                //
                nDropCount = MulDiv(lpvh->dwTimeCaptured - dwTime,
                                    1000,
                                    lpcs->sCapParms.dwRequestMicroSecPerFrame);

               #ifdef JMK_HACK_TIMERS
                if (pTimerRiff)
                    pTimerRiff->vchd.dwDropFramesNotAppended += nDropCount;
               #endif

                // If any frames have been dropped, write them out before
                // we get back to writing the current frame.
                //
                if (nDropCount > 0)
                {
                    AuxDebugEx(2,"*****Adding %d to the dropcount\r\n", nDropCount);
                    lpcs->dwFramesDropped += nDropCount;
                    if (! AVIWriteDummyFrames (lpcs, nDropCount, lpuError, &bPending))
                        fStopping = TRUE;
                }
            }

           #ifdef JMK_HACK_TIMERS
	    if (pTimerRiff) {
	        if (nTimerIndex == CLIPBOARDLOGSIZE)
		    nTimerIndex = 0;
	
// nTimerIndex will be OK	if ((nTimerIndex < CLIPBOARDLOGSIZE) && pTimerStuff)
		if (pTimerStuff)
		{
	
		    pCurTimerStuff = &pTimerStuff[nTimerIndex];
                    ++nTimerIndex;

		    pCurTimerStuff->nFramesAppended = 0;
		    pCurTimerStuff->nDummyFrames  = (WORD)lpcs->dwFramesDropped;
		    pCurTimerStuff->dwFrameTickTime = dwTime;
		    pCurTimerStuff->dwFrameStampTime = lpvh->dwTimeCaptured;
		    pCurTimerStuff->dwVideoChunkCount = lpcs->dwVideoChunkCount;
                    pCurTimerStuff->dwTimeWritten = pcDeltaTicks(&pctWriteBase);
		    pCurTimerStuff->dwTimeToWrite = 0;
		    pCurTimerStuff->nVideoIndex = (WORD)lpcs->iNextVideo;
		    pCurTimerStuff->nAudioIndex = (WORD)lpcs->iNextWave;
		}
	    } // fClipboardLogging
           #endif // JMK_HACK_TIMERS

           // look ahead for dummy frames and try to
           // append them to the current frame
           //
           nAppendDummyFrames = 0;

           #define LOOKAHEAD_FOR_DUMMYS 1
           #ifdef LOOKAHEAD_FOR_DUMMYS
           {
            int        iNext;
            LPVIDEOHDR lpvhNext;
            iNext = lpcs->iNextVideo+1;
            if (iNext >= lpcs->iNumVideo)
                iNext = 0;

            // is the next frame done already?  if so
            // we can append any dropped frames to the end of
            // this frame before we write it out
            //
            lpvhNext = lpcs->alpVideoHdr[iNext];
            if (lpvhNext->dwFlags & VHDR_DONE)
            {
		// Recalculate the current time, which may have
		// changed if dummy frames were inserted above
		dwTime = MulDiv (lpcs->dwVideoChunkCount + 1,
                         lpcs->sCapParms.dwRequestMicroSecPerFrame,
                         1000);
	
                nAppendDummyFrames =
                    MulDiv (lpvhNext->dwTimeCaptured - dwTime,
                            1000,
                            lpcs->sCapParms.dwRequestMicroSecPerFrame);

                if ((--nAppendDummyFrames) < 0)
                    nAppendDummyFrames = 0;
		else {
		    AuxDebugEx(3, DEBUGLINE "Appending %d dummy frames", nAppendDummyFrames);
		}

                AuxDebugEx(1,"*****Adding %d to the dropcount in lookahead mode\r\n", nAppendDummyFrames);
                lpcs->dwFramesDropped += nAppendDummyFrames;

               #ifdef JMK_HACK_TIMERS
                if (pTimerRiff) {
                    pTimerRiff->vchd.dwDropFramesAppended += nAppendDummyFrames;
		    pCurTimerStuff->nFramesAppended = (WORD)nAppendDummyFrames;
		}
               #endif
            }
           }
           #endif

            if ( ! AVIWriteVideoFrame (lpcs,
                                       lpData,
                                       dwBytesUsed,
                                       fKeyFrame,
                                       lpcs->iNextVideo,
                                       nAppendDummyFrames,
                                       lpuError, lpbPending))
                fStopping = TRUE;

           #ifdef JMK_HACK_TIMERS
            if (pCurTimerStuff)
            {
                pCurTimerStuff->dwTimeToWrite = pcDeltaTicks(&pctWriteBase);
                pCurTimerStuff->bPending = (BOOL) *lpbPending;
            }
           #endif
        }

    }

    // return lpvh to the caller so that the frame can be
    // drawn (time permitting)
    //
    *plpvhDraw = lpvh;

    // increment the next Video buffer pointer
    //
    if (++lpcs->iNextVideo >= lpcs->iNumVideo)
        lpcs->iNextVideo = 0;

    return fStopping;
}

/*+ ProcessAudioBuffers
 *
 *-===============================================================*/

STATICFN BOOL _inline ProcessAudioBuffers (
    LPCAPSTREAM lpcs,
    BOOL        fStopping,
    LPUINT      lpuError)
{
    int       iLastWave;
    UINT      ii;
    LPWAVEHDR lpwh;

    *lpuError = 0;
    assert (lpcs->sCapParms.fCaptureAudio);

    // if all buffers are done, we have broke audio.
    //
    iLastWave = lpcs->iNextWave == 0 ? lpcs->iNumAudio -1 : lpcs->iNextWave-1;
    if (!fStopping && lpcs->alpWaveHdr[iLastWave]->dwFlags & WHDR_DONE)
        lpcs->fAudioBreak = TRUE;

    // process all done buffers, but no more than iNumAudio at one
    // pass (to avoid getting stuck here forever)
    //
    for (ii = 0; ii < (UINT)lpcs->iNumAudio; ++ii)
    {
        BOOL bPending;

        // if the next buffer is not done, break out of the loop
        // and return to the caller
        //
        lpwh = lpcs->alpWaveHdr[lpcs->iNextWave];
        if (!(lpwh->dwFlags & WHDR_DONE))
            break;
        lpwh->dwFlags &= ~WHDR_DONE;

        // is there any data in the buffer ?
        // if so, do wave stream callback, then write the
        // buffer
        //
        bPending = FALSE;
        if (lpwh->dwBytesRecorded)
        {
            if (lpcs->CallbackOnWaveStream)
               lpcs->CallbackOnWaveStream (lpcs->hwnd, lpwh);

            if ( ! (lpcs->fCaptureFlags & CAP_fCapturingToDisk))
            {
                lpcs->dwWaveChunkCount++;
                lpcs->dwWaveBytes += lpwh->dwBytesRecorded;
            }
            else
            {
                // write the audio buffer, bPending will be true
                // if the write will complete asynchronously
                //
                if ( ! AVIWriteAudio (lpcs, lpwh, lpcs->iNextWave,
                                      lpuError, &bPending))
                    fStopping = TRUE;
            }
        }

        // if we are not writing async, we can put the buffer
        // back on the wave driver's queue now
        //
        if ( ! bPending)
        {
            lpwh->dwBytesRecorded = 0;
	    AuxDebugEx(3, DEBUGLINE "Calling waveInAddBuffer for address %8x", lpwh);
            if (waveInAddBuffer(lpcs->hWaveIn, lpwh, sizeof(WAVEHDR)))
            {
                fStopping = TRUE;
                *lpuError = IDS_CAP_WAVE_ADD_ERROR;
            }
        }

        // increment the next wave buffer pointer
        //
        if (++lpcs->iNextWave >= lpcs->iNumAudio)
            lpcs->iNextWave = 0;
    }

    return fStopping;
}

/*+ ProcessAsyncIOBuffers
 *
 *-===============================================================*/

STATICFN BOOL _inline ProcessAsyncIOBuffers (
    LPCAPSTREAM lpcs,
    BOOL        fStopping,
    LPUINT      lpuError)
{
    UINT      ii;
    struct _avi_async * lpah;

    // if there are no async buffer headers, there is nothing to do!
    //
    *lpuError = 0;
    assert (lpcs->pAsync);

    //
    // process all done buffers, stopping when there are no more outstanding
    // iNextAsync can never go past iLastAsync.
    //
    while(lpcs->iNextAsync != lpcs->iLastAsync)
    {
        DWORD dwUsed;

        // if this async header has never been used,
        // we are done
        //
        lpah = &lpcs->pAsync[lpcs->iNextAsync];
        assert (lpah->uType);

        AuxDebugEx (2, DEBUGLINE "processing async io buffer %d off=%x\r\n",
                    lpcs->iNextAsync, lpah->ovl.Offset);

        // if the next buffer is not done, or failed break
        // out of the loop
        //
        // if the io on this block has already completed (because the IO
        // completed out of order) queue it to the device without waiting
        // otherwise get the next completion status.  If a block has
        // completed, and it is the block at the head of the async queue,
        // then it can be passed straight back to the device queue.  If the
        // completed block is not the one we are expecting, we mark the IO
        // as complete, then return.  Thought..call GetQueuedCompletionStatus
        // in a loop, until there are no more blocks pending.  This way we
        // might get to complete the block we want on this call to
        // ProcessAsyncIOBuffers.
        //
        if (lpah->uType & ASYNCIOPENDING) {
            DWORD dwWritten;
            DWORD key;
            LPOVERLAPPED povl;
            BOOL fResult =
            pfnGetQueuedCompletionStatus(lpcs->hCompletionPort,
                                        &dwWritten,
                                        &key,
                                        &povl,
                                        0);
            if (fResult) {
                // we dequeued a block.  Did we dequeue the one we wanted?
                ((struct _avi_async *)povl)->uType &= ~ASYNCIOPENDING;
                if ((PVOID)povl == (PVOID)lpah) {
                    // this is the one we wanted
                    // fall through and add back to the device queue
                    AuxDebugEx(2,"Dequeued the block we wanted at %8x\r\n", lpah);
                } else {
                    // the io block completed out of order.
                    // Clear the io pending flag and return.
                    AuxDebugEx(1,"Dequeued out of order at %8x\r\n", povl->hEvent);
                    break;
                }
            } else {
                if (povl) {
                    // a failed io operation
                    *lpuError = IDS_CAP_FILE_WRITE_ERROR;
		    AuxDebugEx(1, DEBUGLINE "A failed IO operation (GQCS)\r\n");
                    fStopping = TRUE;
                } else {
                    // nothing completed
		    AuxDebugEx(3, DEBUGLINE "Nothing completed on call to GQCS\r\n");
                    break;
                }
            }
        } else {
            // IO is already complete for this block
        }

        // the buffer is done, so now we need to queue the wave/video
        // buffer back to the wave/video driver
        //

	assert (!(lpah->uType & ASYNCIOPENDING));
        switch (lpah->uType)
        {
            case ASYNC_BUF_VIDEO:
            {
                LPVIDEOHDR lpvh = lpcs->alpVideoHdr[lpah->uIndex];
               #if defined CHICAGO
                if (vidxAddBuffer(lpcs->hVideoIn, lpvh, sizeof (VIDEOHDR)))
               #else
		AuxDebugEx(3, DEBUGLINE "Queueing video buffer lpvh=%x (index %d)\r\n", lpvh, lpah->uIndex);
                if (videoStreamAddBuffer(lpcs->hVideoIn, lpvh, sizeof (VIDEOHDR)))
               #endif
                {
                    fStopping = TRUE;
                    *lpuError = IDS_CAP_VIDEO_ADD_ERROR;
                }
            }
            break;

            case ASYNC_BUF_AUDIO:
            {
                LPWAVEHDR lpwh = lpcs->alpWaveHdr[lpah->uIndex];
                lpwh->dwBytesRecorded = 0;
		AuxDebugEx(3, DEBUGLINE "Queueing audio buffer lpwh=%x (index %d)\r\n", lpwh, lpah->uIndex);
                if (waveInAddBuffer (lpcs->hWaveIn, lpwh, sizeof(WAVEHDR)))
                {
                    fStopping = TRUE;
                    *lpuError = IDS_CAP_WAVE_ADD_ERROR;
                }
            }
            break;

            //case ASYNC_BUF_DROP:
            //{
            //}
            //break;
        }

	// mark the overlapped header structure as vacant
        lpah->uType = 0;
        lpah->uIndex = 0;

        // increment to the next async io buffer
        //
        if (++lpcs->iNextAsync  >= lpcs->iNumAsync)
            lpcs->iNextAsync = 0;  // wrapped...
    }

    return fStopping;
}

/*+ ShowCompletionStatus
 *
 *-===============================================================*/

STATICFN void ShowCompletionStatus (
    LPCAPSTREAM lpcs,
    BOOL        fCapturedOK)
{
    // Notify if there was an error while recording
    //
    if ( ! fCapturedOK)
        errorUpdateError (lpcs, IDS_CAP_RECORDING_ERROR);

    // put up completion message on status line
    //
    if (lpcs->fCaptureFlags & CAP_fCapturingToDisk)
    {
        DWORD dw;

        // The muldiv32 doesn't give 0 if numerator is zero
        dw = 0;
        if (lpcs->dwVideoChunkCount)
            dw = muldiv32(lpcs->dwVideoChunkCount,1000000,lpcs->dwTimeElapsedMS);

        if (lpcs->sCapParms.fCaptureAudio)
        {
            // "Captured %d.%03d sec.  %ld frames (%ld dropped) (%d.%03d fps).  %ld audio bytes (%d.%03d sps)"
            statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOAUDIO,
                  (UINT)(lpcs->dwTimeElapsedMS/1000),
                  (UINT)(lpcs->dwTimeElapsedMS%1000),
                  lpcs->dwVideoChunkCount,
                  lpcs->dwFramesDropped,
                  (UINT)(dw / 1000),
                  (UINT)(dw % 1000),
                  lpcs->dwWaveBytes,
                  (UINT) lpcs->lpWaveFormat->nSamplesPerSec / 1000,
                  (UINT) lpcs->lpWaveFormat->nSamplesPerSec % 1000);
        }
        else
        {
            // "Captured %d.%03d sec.  %ld frames (%ld dropped) (%d.%03d fps)."
            statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOONLY,
                  (UINT)(lpcs->dwTimeElapsedMS/1000),
                  (UINT)(lpcs->dwTimeElapsedMS%1000),
                  lpcs->dwVideoChunkCount,
                  lpcs->dwFramesDropped,
                  (UINT)(dw / 1000),
                  (UINT)(dw % 1000));
        }
    } // endif capturing to disk (no warnings or errors if to net)

    // if capture was successful, warn the user about various abnormal
    // conditions.
    //
    if (fCapturedOK)
    {
        if (lpcs->dwVideoChunkCount == 0)
        {
            // No frames captured, warn user that interrupts are probably not enabled.
            errorUpdateError (lpcs, IDS_CAP_NO_FRAME_CAP_ERROR);
        }
        else if (lpcs->sCapParms.fCaptureAudio && lpcs->dwWaveBytes == 0)
        {
            // No audio captured, warn user audio card is hosed
            errorUpdateError (lpcs, IDS_CAP_NO_AUDIO_CAP_ERROR);
        }
        else if (lpcs->sCapParms.fCaptureAudio && lpcs->fAudioBreak)
        {
            // some of the audio was dropped
            if(lpcs->CompVars.hic) {
		errorUpdateError (lpcs, IDS_CAP_AUDIO_DROP_COMPERROR);
	    } else {
		errorUpdateError (lpcs, IDS_CAP_AUDIO_DROP_ERROR);
	    }
        }
        else if (lpcs->fCaptureFlags & CAP_fCapturingToDisk)
        {
            DWORD dwPctDropped;

            assert (lpcs->dwVideoChunkCount);
            dwPctDropped = 100 * lpcs->dwFramesDropped / lpcs->dwVideoChunkCount;
            //
            // dropped > 10% (default) of the frames
            //
            if (dwPctDropped > lpcs->sCapParms.wPercentDropForError)
                errorUpdateError (lpcs, IDS_CAP_STAT_FRAMESDROPPED,
                      lpcs->dwFramesDropped,
                      lpcs->dwVideoChunkCount,
                      (UINT)(muldiv32(lpcs->dwFramesDropped,10000,lpcs->dwVideoChunkCount)/100),
                      (UINT)(muldiv32(lpcs->dwFramesDropped,10000,lpcs->dwVideoChunkCount)%100)/10
                      );
        }
    }
}

/*
 *  AVI Capture
 *      This is the main streaming capture loop for both audio and
 * video.  It will first init all buffers and drivers and then go into a
 * loop checking for buffers to be filled.  When a buffer is filled then
 * the data for it is written out.
 * Afterwards it cleans up after itself (frees buffers etc...)
 * Returns: 0 on success, else error code
 */
void FAR PASCAL _LOADDS AVICapture1(LPCAPSTREAM lpcs)
{
    BOOL        fCapturedOK = TRUE;
    BOOL        fStopping;         // True when finishing capture
    BOOL        fStopped;          // True if driver notified to stop
    TCHAR       ach[128];
    TCHAR       achMsg[128];
    UINT        wError;         // Error String ID
    LPVIDEOHDR  lpVidHdr;
    LPWAVEHDR   lpWaveHdr;
    DWORD       dwTimeStarted;  // When did we start in milliseconds
    DWORD       dwTimeStopped;
    DWORD       dwTimeToStop;   // Lesser of MCI capture time or frame limit
    BOOL        fTryToPaint = FALSE;
    BOOL        fTryToPaintAgain = FALSE;
    HDC         hdc;
    HPALETTE    hpalT;
    HCURSOR     hOldCursor;
    RECT        rcDrawRect;
    CAPINFOCHUNK cic;
    DWORD       dwOldPrio;
    BOOL        bVideoWritePending;
    LPVIDEOHDR  lpvhDraw;

    lpcs->fCaptureFlags |= CAP_fCapturingNow;
    // we should Assert that CAP_fCapturingNow is already turned on

    lpcs->dwReturn = DV_ERR_OK;

    hOldCursor = SetCursor(lpcs->hWaitCursor);

    statusUpdateStatus(lpcs, IDS_CAP_BEGIN);  // Always the first message

    // If not 1 Meg. free, give it up!!!
    if (GetFreePhysicalMemory () < (1024L * 1024L)) {
        errorUpdateError (lpcs, IDS_CAP_OUTOFMEM);
        goto EarlyExit;
    }

    statusUpdateStatus(lpcs, IDS_CAP_STAT_CAP_INIT);

    // Try painting the DIB only if Live window
    fTryToPaintAgain = fTryToPaint = lpcs->fLiveWindow;

    if (fTryToPaint) {
        hdc = GetDC(lpcs->hwnd);
        SetWindowOrgEx(hdc, lpcs->ptScroll.x, lpcs->ptScroll.y, NULL);
        hpalT = DrawDibGetPalette (lpcs->hdd);
        if (hpalT)
            hpalT = SelectPalette( hdc, hpalT, FALSE);
        RealizePalette(hdc);
        if (lpcs->fScale)
            GetClientRect (lpcs->hwnd, &rcDrawRect);
        else
            SetRect (&rcDrawRect, 0, 0, lpcs->dxBits, lpcs->dyBits);
    }

    // -------------------------------------------------------
    //   When should capture stop?
    // -------------------------------------------------------

    // If using MCI, capture for the shorter of the MCI period,
    // or the capture limit

    if (lpcs->sCapParms.fLimitEnabled)
        dwTimeToStop = (DWORD) ((DWORD) 1000 * lpcs->sCapParms.wTimeLimit);
    else
        dwTimeToStop = (DWORD) -1L; // very large

    if (lpcs->sCapParms.fMCIControl) {
        DWORD dwTime;

        // if MCI stop time not given, use lpcs->sCapParms.wTimeLimit
        if (lpcs->sCapParms.dwMCIStopTime == lpcs->sCapParms.dwMCIStartTime)
                    lpcs->sCapParms.dwMCIStopTime = lpcs->sCapParms.dwMCIStartTime +
                    (DWORD) ((DWORD)1000 * lpcs->sCapParms.wTimeLimit);

        dwTime = lpcs->sCapParms.dwMCIStopTime - lpcs->sCapParms.dwMCIStartTime;

        if (lpcs->sCapParms.fLimitEnabled)
            dwTimeToStop = min (dwTime, dwTimeToStop);
        else
            dwTimeToStop = dwTime;
    }

    //
    // never ever try to capture more than the index size!
    //
    if (lpcs->fCaptureFlags & CAP_fCapturingToDisk)
    {
        DWORD dwTime = MulDiv (lpcs->sCapParms.dwIndexSize,
                               lpcs->sCapParms.dwRequestMicroSecPerFrame,
                               1000l);

        dwTimeToStop = min (dwTime, dwTimeToStop);
    }

    // if doing MCI capture, initialize MCI device.  if init fails
    // go straight to the exit code
    //
    if (lpcs->sCapParms.fMCIControl)
    {
        if ( ! MCIDeviceOpen (lpcs) ||
             ! MCIDeviceSetPosition (lpcs, lpcs->sCapParms.dwMCIStartTime))
        {
            fCapturedOK = FALSE;
            errorUpdateError (lpcs, IDS_CAP_MCI_CONTROL_ERROR);
            statusUpdateStatus(lpcs, 0);    // Clear status
            goto EarlyExit;
        }
    }

    //
    // If we're compressing while capturing, warm up the compressor
    //
   #ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic)
    {
        if ( ! ICSeqCompressFrameStart (&lpcs->CompVars, lpcs->lpBitsInfo))
        {
            // !!! We're in trouble here!
            dprintf("ICSeqCompressFrameStart failed !!!\n");
            errorUpdateError (lpcs, IDS_CAP_COMPRESSOR_ERROR);
            goto EarlyExit;
        }

	// HACK WARNING !!!
        // Kludge, offset the lpBitsOut ptr
        // Compman allocates the compress buffer too large by
        // 2048 + 16 so we will still have room
	// By stepping on 8 bytes we give ourselves room for a RIFF header
        //
        ((LPBYTE)lpcs->CompVars.lpBitsOut) += 8;

        assert(lpcs->CompVars.lpbiOut != NULL);
    }
   #endif

    // -------------------------------------------------------
    //  Open the output file
    // -------------------------------------------------------

    if (lpcs->fCaptureFlags & CAP_fCapturingToDisk) {
	if (!CapFileInit(lpcs))
	{
	    errorUpdateError (lpcs, IDS_CAP_FILE_OPEN_ERROR);
	    goto EarlyExit;
	}
    } else {
        AuxDebugEx (3, DEBUGLINE "Setting dwBytesPerSector to %d\r\n",DEFAULT_BYTESPERSECTOR);
        lpcs->dwBytesPerSector=DEFAULT_BYTESPERSECTOR;
    }

   #ifdef JMK_HACK_TIMERS
    // Allocate memory for logging capture results to the clipboard if requested
    if (GetProfileIntA ("Avicap32", "ClipboardLogging", FALSE))
    {
        AuxDebugEx (2, DEBUGLINE "ClipboardLogging Enabled\r\n");
        InitPerformanceCounters();
        pcBegin(), pctWriteBase = pc.base;

	hMemTimers = GlobalAlloc(GHND | GMEM_ZEROINIT,
                             sizeof(struct _timerriff) +
                             sizeof(struct _timerstuff) * CLIPBOARDLOGSIZE);

	if (hMemTimers && ((DWORD_PTR)(pTimerRiff = GlobalLock (hMemTimers))))
	    ;
	else if (hMemTimers)
	{
	    GlobalFree(hMemTimers);
	    pTimerRiff = 0;
	    pTimerStuff = 0;
	    hMemTimers = 0;
	}
	nTimerIndex = 0;
	nSleepCount = 0;
    }  // if ClipboardLogging
   #endif  // JMK_HACK_TIMERS

    // Make sure the parent has been repainted
    //
    UpdateWindow(lpcs->hwnd);

    //
    // call AVIInit() to get all the capture memory we will need
    //

    wError = IDS_CAP_AVI_INIT_ERROR;
    lpcs->hCaptureEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (lpcs->hCaptureEvent)
    {
       #ifdef CHICAGO
        lpcs->hRing0CapEvt = OpenVxDHandle (lpcs->hCaptureEvent);
        if ( ! lpcs->hRing0CapEvt)
            CloseHandle (lpcs->hCaptureEvent), lpcs->hCaptureEvent = NULL;
        else
       #endif
            wError = AVIInit(lpcs);

    }

    // if avifile init failed, cleanup and return error.
    //
    if (wError)
    {
        // Error in initalization - return
        //
        errorUpdateError (lpcs, wError);
        AVIFini(lpcs);
        AVIFileFini(lpcs, TRUE, TRUE);
        statusUpdateStatus(lpcs, 0);    // Clear status
        goto EarlyExit;
    }

    // Click OK to capture string (must follow AVIInit)
    //
    LoadString(lpcs->hInst, IDS_CAP_SEQ_MSGSTART, ach, NUMELMS(ach));
    wsprintf(achMsg, ach, (LPBYTE)lpcs->achFile);

    // clear status
    //
    statusUpdateStatus(lpcs, 0);

    // -------------------------------------------------------
    //   Ready to go, make the user click OK?
    // -------------------------------------------------------

    if (lpcs->sCapParms.fMakeUserHitOKToCapture && (lpcs->fCaptureFlags & CAP_fCapturingToDisk))
    {
        UINT idBtn;

        idBtn = MessageBox (lpcs->hwnd, achMsg, TEXT(""),
                            MB_OKCANCEL | MB_ICONEXCLAMATION);

        if (idBtn == IDCANCEL)
        {
            AVIFini(lpcs);
            AVIFileFini (lpcs, TRUE, TRUE);
            statusUpdateStatus (lpcs, 0);
            goto EarlyExit;
        }
    }

    // update the status, so the user knows how to stop
    //
    statusUpdateStatus(lpcs, IDS_CAP_SEQ_MSGSTOP);
    UpdateWindow(lpcs->hwnd);

    // this should be an ASSERT.  After all, we turned the flag on at the
    // top of the routine
    //lpcs->fCaptureFlags |= CAP_fCapturingNow;

    // query async key states to 'reset' them to current values
    //
    GetAsyncKeyState(lpcs->sCapParms.vKeyAbort);
    GetAsyncKeyState(VK_ESCAPE);
    GetAsyncKeyState(VK_LBUTTON);
    GetAsyncKeyState(VK_RBUTTON);

    // Insert the digitization time
    // strings written to the file should be ascii, since this is
    // an ascii file format.
    //
    //
    // no point in pulling in the whole C runtime just to get a silly
    // timestamp, so we just cook the system time into ascii right here.
    //
    {
    SYSTEMTIME time;
    // Note: both szDay and szMonth are explicitly null-terminated by virtue
    // of being C strings ... "xxx"
    static char szDay[] = "Sun\0Mon\0Tue\0Wed\0Thu\0Fri\0Sat";
    #define DAYLENGTH (sizeof(szDay)/7)
    static char szMonth[] = "Jan\0Feb\0Mar\0Apr\0May\0Jun\0Jul\0Aug\0Sep\0Oct\0Nov\0Dec";
    #define MONTHLENGTH (sizeof(szMonth)/12)
    char sz[30];

    GetLocalTime (&time);
    // note: GetLocalTime returns months in range 1-12
    //                    returns days in range 0-6

    //example: Fri Apr 29  8:25:12 1994
    wsprintfA(sz, "%s %s %2d %2d:%02d:%02d %4d",
              szDay + time.wDayOfWeek * DAYLENGTH,
              szMonth-MONTHLENGTH + time.wMonth * MONTHLENGTH,
              time.wDay, time.wHour, time.wMinute, time.wSecond, time.wYear);

    cic.fccInfoID = mmioFOURCC ('I','D','I','T');
    cic.lpData = sz;
    cic.cbData = 25;		  // WARNING: this length is static.
    SetInfoChunk (lpcs, &cic);
    }

    // -------------------------------------------------------
    //   Start MCI, Audio, and video streams
    // -------------------------------------------------------

    // Callback will preroll, then return on frame accurate postion
    // The 1 indicates recording is about to start
    // Callback can return FALSE to exit without capturing
    //
    if (lpcs->CallbackOnControl &&
        !lpcs->CallbackOnControl(lpcs->hwnd, CONTROLCALLBACK_PREROLL))
    {
        AVIFini(lpcs);
        AVIFileFini(lpcs, TRUE, TRUE);
        statusUpdateStatus(lpcs, 0);
        goto EarlyExit;
    }

    dwOldPrio = GetThreadPriority(GetCurrentThread());
    if (dwOldPrio != THREAD_PRIORITY_HIGHEST)
        SetThreadPriority (GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

   #ifdef JMK_HACK_TIMERS
    if (pTimerRiff)
    {
	UINT ii;

        pTimerRiff->fccRIFF = RIFFTYPE('RIFF'); //MAKEFOURCC('R','I','F','F');
	pTimerRiff->cbTotal = sizeof(struct _timerriff) - 8 +
	    		  sizeof(struct _timerstuff) * CLIPBOARDLOGSIZE;
        pTimerRiff->fccJMKD = RIFFTYPE('JMKD'); //MAKEFOURCC('J','M','K','D');
        pTimerRiff->fccVCHD = RIFFTYPE('VCHD'); //MAKEFOURCC('V','C','H','D');
	
	pTimerRiff->cbVCHD  = sizeof(struct _vchd);
	pTimerRiff->vchd.nPrio = GetThreadPriority(GetCurrentThread());
	pTimerRiff->vchd.bmih = lpcs->lpBitsInfo->bmiHeader;
	pTimerRiff->vchd.cap  = lpcs->sCapParms;
	pTimerRiff->vchd.dwDropFramesAppended = 0;
	pTimerRiff->vchd.dwDropFramesNotAppended = 0;
        pTimerRiff->vchd.dwTimerFrequency = pcGetTickRate();
	
	for (ii = 0; ii < NUMELMS(pTimerRiff->vchd.atvh); ++ii)
	{
	    if (lpcs->alpVideoHdr[ii])
            {
	        struct _thkvideohdr * ptvh = (LPVOID)lpcs->alpVideoHdr[ii];
               #ifndef CHICAGO
                assert (sizeof(CAPVIDEOHDR) == sizeof(*ptvh));
               #endif
                pTimerRiff->vchd.atvh[ii] = *ptvh;
                pTimerRiff->vchd.nMaxVideoBuffers = ii;
            }
        }
	
        pTimerRiff->fccChunk = RIFFTYPE('VCAP'); //MAKEFOURCC('V','C','A','P');
	pTimerRiff->cbChunk = pTimerRiff->cbTotal - sizeof(*pTimerRiff);
	
	pTimerStuff = (LPVOID)(pTimerRiff + 1);
	pCurTimerStuff = &pTimerStuff[0];
    }  // fClipboardLogging
   #endif  // JMK_HACK_TIMERS

    // make sure that the fat is loaded before we begin capturing
    //
    AVIPreloadFat (lpcs);

    // start the MCI device
    //
    if (lpcs->sCapParms.fMCIControl)
        MCIDevicePlay (lpcs);

    dwTimeStarted = timeGetTime();

    // start audio & video streams
    //
    if (lpcs->sCapParms.fCaptureAudio)
        waveInStart(lpcs->hWaveIn);
    videoStreamStart(lpcs->hVideoIn);

    // -------------------------------------------------------
    //   MAIN CAPTURE LOOP
    // -------------------------------------------------------

    fCapturedOK=TRUE;
    fStopping = FALSE;    // TRUE when we need to stop
    fStopped = FALSE;     // TRUE if drivers notified we have stopped
    lpcs->dwTimeElapsedMS = 0;

    assert (lpcs->iNextVideo == 0);
    if (lpcs->sCapParms.fCaptureAudio) {
	assert (lpcs->iNextWave == 0);
	lpWaveHdr = lpcs->alpWaveHdr[lpcs->iNextWave];
	// lpWaveHdr is only interesting when we capture audio
    }

    lpVidHdr = lpcs->alpVideoHdr[lpcs->iNextVideo];

    DPF("Start of main capture loop");

    for (;;)
    {

        // The INTEL driver uses the GetError message to
        // process buffers, so call it often...
        // FIX JAYBO        videoStreamGetError (lpcs->hVideoIn, &dwStreamError, &dwDriverDropCount);


        // if there are no buffers to process, we either wait
        // or leave the loop forever (depending on whether we expect
        // more buffers to be done in the future)
        //
        if (!(lpVidHdr->dwFlags & VHDR_DONE) &&
            !(lpcs->sCapParms.fCaptureAudio
	        && (lpWaveHdr->dwFlags & WHDR_DONE)))
        {
            if (fStopped)
                break;

           #ifdef JMK_HACK_TIMERS
            if (pCurTimerStuff)
            {
               pCurTimerStuff->nSleepCount = ++nSleepCount;
               pCurTimerStuff->dwSleepBegin = pcGetTicks();
            }
           #endif

	    AuxDebugEx(2,DEBUGLINE "***** Waiting for something interesting to happen while capturing\r\n");
            WaitForSingleObject (lpcs->hCaptureEvent, 300);

           #ifdef JMK_HACK_TIMERS
            if (pCurTimerStuff)
	    {
               pCurTimerStuff->dwSleepEnd = pcGetTicks();
	    }
           #endif
        }

        // What time is it?
        lpcs->dwTimeElapsedMS = timeGetTime() - dwTimeStarted;

        // -------------------------------------------------------
        //        Is video buffer ready to be written?
        // -------------------------------------------------------

        if ((DWORD)(fStopping = ProcessNextVideoBuffer (lpcs,
                                                fStopping,
                                                &wError,
                                                &lpvhDraw,  // captured frame to draw if time permits
                                                &bVideoWritePending))) // TRUE if Write pending on lpvhDraw
        {
            AuxDebugEx (1, DEBUGLINE "ProcessVideo stopping\r\n");
            if (wError)
            {
                AuxDebugEx (1, DEBUGLINE "ProcessVideo return error %d\r\n", wError);
                errorUpdateError (lpcs, wError);
                fCapturedOK = FALSE;
                break;
            }
        }
        lpVidHdr = lpcs->alpVideoHdr[lpcs->iNextVideo];

        // if there is still more time, (or at least every 100 frames)
        // show status if we're not ending the capture
        //
        if (!fStopping &&
            (lpcs->fCaptureFlags & CAP_fCapturingToDisk) &&
            (!(lpVidHdr->dwFlags & VHDR_DONE) ||
              (lpcs->dwVideoChunkCount && (lpcs->dwVideoChunkCount % 100 == 0))))
        {
            // Captured %ld frames (Dropped %ld) %d.%03d sec. Hit Escape to Stop
            //
            statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOCURRENT,
                               lpcs->dwVideoChunkCount,
                               lpcs->dwFramesDropped,
                               (UINT)(lpcs->dwTimeElapsedMS/1000),
                               (UINT)(lpcs->dwTimeElapsedMS%1000));
        }

        // If the yield callback returns FALSE, abort
        //
        if (lpcs->CallbackOnYield && !lpcs->CallbackOnYield (lpcs->hwnd))
            fStopping = TRUE;

       #if 0 // this is a 16 bit ism??
        // Don't do peekMessage yield for ACM
        if (lpcs->sCapParms.fYield) {
            MSG msg;

            if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
                // Kludge to get rid of timers from lpcs->hwnd
                if (msg.message == WM_TIMER && msg.hwnd == lpcs->hwnd)
                    ;
                else {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            }
        }
       #endif

        // Outside routine is handling when to stop
        // The CONTROLCALLBACK_CAPTURING indicates we're asking when to stop
        //
        if (lpcs->CallbackOnControl &&
            !lpcs->CallbackOnControl (lpcs->hwnd, CONTROLCALLBACK_CAPTURING))
            fStopping = TRUE;

        // -------------------------------------------------------
        //        Is audio buffer ready to be written?
        // -------------------------------------------------------

        if (lpcs->sCapParms.fCaptureAudio) {
            if ((DWORD)(fStopping = ProcessAudioBuffers (lpcs, fStopping, &wError)))
            {
                AuxDebugEx (1, DEBUGLINE "ProcessAudio stopping\r\n");
                if (wError)
                {
                    AuxDebugEx (1, DEBUGLINE "ProcessAudio return error %d\r\n", wError);
                    errorUpdateError (lpcs, wError);
                    fCapturedOK = FALSE;
                    break;
                }
            }
	    lpWaveHdr = lpcs->alpWaveHdr[lpcs->iNextWave];
	}

        // if we are not writing the frame async, we can put the video buffer
        // back on the video driver's queue now
        //
        if (lpvhDraw)
        {

            // if the next video header is not ready yet, and
            // we have no outstanding io buffers (in async mode) draw
            // the current one
            //
            if ( !(lpVidHdr->dwFlags & VHDR_DONE) &&
                (!lpcs->pAsync || 
                 (lpcs->iNextAsync+2 >= lpcs->iLastAsync)) &&
                lpvhDraw->dwBytesUsed)
            {
                AuxDebugEx (4, DEBUGLINE "time enough to draw!\r\n");
                if (fTryToPaintAgain &&
                    lpcs->dwVideoChunkCount &&
                    lpvhDraw->dwFlags & VHDR_KEYFRAME)
                {
                    fTryToPaintAgain = DrawDibDraw(lpcs->hdd, hdc,
                            0, 0,
                            rcDrawRect.right - rcDrawRect.left,
                            rcDrawRect.bottom - rcDrawRect.top,
                            /*lpcs->dxBits, lpcs->dyBits, */
                            (LPBITMAPINFOHEADER)lpcs->lpBitsInfo,
                            lpvhDraw->lpData, 0, 0, -1, -1,
                            DDF_SAME_HDC | DDF_SAME_DIB | DDF_SAME_SIZE);
                }
            }

            // if there is not a write pending for the draw frame
            // put it back into the video drivers queue now
            //
            if ( ! bVideoWritePending)
            {
		AuxDebugEx(3, DEBUGLINE "Queueing video buffer, lpvh=%8x", lpvhDraw);

                // return the emptied buffer to the que
                //
               #if defined CHICAGO
                if (vidxAddBuffer(lpcs->hVideoIn, lpvhDraw, sizeof (VIDEOHDR)))
               #else

                if (videoStreamAddBuffer(lpcs->hVideoIn, lpvhDraw, sizeof (VIDEOHDR)))
               #endif
                {
                    AuxDebugEx (2, DEBUGLINE "Failed to Queue Video buffer %08x\r\n", lpvhDraw);
                    errorUpdateError (lpcs, IDS_CAP_VIDEO_ADD_ERROR);
                    fCapturedOK = FALSE;
                    fStopping = TRUE;
                    break;
                }
            }
        }

        // ------------------------------------------------------------
        //        Any completed I/O buffers?
        // ------------------------------------------------------------

        if (lpcs->pAsync)
            if ((DWORD)(fStopping = ProcessAsyncIOBuffers (lpcs, fStopping, &wError)))
            {
                if (wError)
                {
                    errorUpdateError (lpcs, wError);
                    fCapturedOK = FALSE;
                    break;
                }
            }

        // -------------------------------------------------------
        //        is there any reason to stop?
        // -------------------------------------------------------

        if (!fStopping)
        {
            if (lpcs->sCapParms.vKeyAbort &&
                (GetAsyncKeyState(lpcs->sCapParms.vKeyAbort & 0x00ff) & 0x0001))
            {
                BOOL fT = TRUE;
                if (lpcs->sCapParms.vKeyAbort & 0x8000)  // Ctrl?
                    fT = fT && (GetAsyncKeyState(VK_CONTROL) & 0x8000);
                if (lpcs->sCapParms.vKeyAbort & 0x4000)  // Shift?
                    fT = fT && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
                fStopping = fT;      // User aborts
            }
            if (lpcs->sCapParms.fAbortLeftMouse && (GetAsyncKeyState(VK_LBUTTON) & 0x0001))
                fStopping = TRUE;      // User aborts
            if (lpcs->sCapParms.fAbortRightMouse && (GetAsyncKeyState(VK_RBUTTON) & 0x0001))
                fStopping = TRUE;      // User aborts
            if ((lpcs->fCaptureFlags & CAP_fAbortCapture) || (lpcs->fCaptureFlags & CAP_fStopCapture))
                fStopping = TRUE;          // Somebody above wants us to quit
            if (lpcs->dwTimeElapsedMS > dwTimeToStop)
                fStopping = TRUE;      // all done

           #ifdef DEBUG
            if (fStopping)
               AuxDebugEx (1, DEBUGLINE "user stop\r\n");
           #endif
        }

        // -------------------------------------------------------
        //        Tell all the devices to stop
        // -------------------------------------------------------

        if (fStopping)
        {
            if ( ! fStopped)
            {
                fStopped = TRUE;
                DSTATUS(lpcs, "Stopping....");

                if (lpcs->sCapParms.fCaptureAudio)
                {
                    DSTATUS(lpcs, "Stopping Audio");
                    waveInStop(lpcs->hWaveIn);
                }

                DSTATUS(lpcs, "Stopping Video");
                videoStreamStop(lpcs->hVideoIn);         // Stop everybody

                dwTimeStopped = timeGetTime ();

                if (lpcs->sCapParms.fMCIControl)
                {
                    DSTATUS(lpcs, "Stopping MCI");
                    MCIDevicePause (lpcs);
                }
                DSTATUS(lpcs, "Stopped");

                // Force cursor back to hourglass
                //
                SetCursor(lpcs->hWaitCursor);
            }

            // "Finished capture, now writing frame %ld"
            //
            if (fCapturedOK)
                statusUpdateStatus(lpcs, IDS_CAP_STAT_CAP_FINI, lpcs->dwVideoChunkCount);
            else
            {
                statusUpdateStatus(lpcs, IDS_CAP_RECORDING_ERROR2);
                break;
            }

	    // Wait for all the async IO to complete ??
	    //
        }

    // -------------------------------------------------------
    //   END OF MAIN CAPTURE LOOP
    // -------------------------------------------------------
    }

    DPF("End of main capture loop");

    // eat any keys that have been pressed
    //
    while(GetKey(FALSE))
        ;

    // flush stuff to disk, close everything etc.
    //
    AVIFini(lpcs);
    AVIFileFini(lpcs, TRUE, !fCapturedOK);

    // This is the corrected capture duration, based on audio samples
    lpcs->dwTimeElapsedMS = lpcs->dwActualMicroSecPerFrame *
                            lpcs->dwVideoChunkCount / 1000;

    // update the status line with information about the completed
    // capture, or with erroor information
    //
    ShowCompletionStatus (lpcs, fCapturedOK);

EarlyExit:

    //
    // If we we're compressing while capturing, close it down
    //
   #ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic) {
        // Kludge, reset the lpBitsOut pointer
        if (lpcs->CompVars.lpBitsOut)
            ((LPBYTE) lpcs->CompVars.lpBitsOut) -= 8;
        ICSeqCompressFrameEnd(&lpcs->CompVars);
    }
   #endif

    if (fTryToPaint) {
        if (hpalT)
             SelectPalette(hdc, hpalT, FALSE);
        ReleaseDC (lpcs->hwnd, hdc);
    }

    if (lpcs->sCapParms.fMCIControl)
        MCIDeviceClose (lpcs);

    // Let the user see where capture stopped
    if ((!lpcs->fLiveWindow) && (!lpcs->fOverlayWindow))
        videoFrame( lpcs->hVideoIn, &lpcs->VidHdr );
    InvalidateRect( lpcs->hwnd, NULL, TRUE);

    SetThreadPriority (GetCurrentThread(), dwOldPrio);
    SetCursor(hOldCursor);

    lpcs->fCapFileExists = (lpcs->dwReturn == DV_ERR_OK);
    lpcs->fCaptureFlags &= ~CAP_fCapturingNow;

    statusUpdateStatus(lpcs, IDS_CAP_END);      // Always the last message

   #ifdef JMK_HACK_TIMERS
    if (pTimerRiff)
    {
        UINT    ii;
	UINT	kk;
        LPSTR   psz;
        HGLOBAL hMem;

        kk = (lpcs->dwVideoChunkCount >= CLIPBOARDLOGSIZE) ?
			CLIPBOARDLOGSIZE : nTimerIndex;

        hMem = GlobalAlloc (GHND, (16 * 5 + 2) * kk + 80);
	
        if (hMem && ((DWORD_PTR)(psz = GlobalLock (hMem))))
        {
            pTimerRiff->vchd.dwFramesCaptured = lpcs->dwVideoChunkCount;
            pTimerRiff->vchd.dwFramesDropped = lpcs->dwFramesDropped;

            pTimerRiff->cbTotal = sizeof(struct _timerriff) - 8 +
                                  sizeof(struct _timerstuff) * nTimerIndex;
            pTimerRiff->cbChunk = pTimerRiff->cbTotal - sizeof(*pTimerRiff);

            lstrcpyA(psz, "Slot#, VideoIndex, ExpectedTime, DriverTime, AccumulatedDummyFrames, CurrentAppendedDummies");
            for (ii = 0; ii < kk; ++ii)
            {
                psz += lstrlenA(psz);
                wsprintfA(psz, "\r\n%d, %ld, %ld, %ld, %d, %d",
			  ii,
			  pTimerStuff[ii].dwVideoChunkCount,
                          pTimerStuff[ii].dwFrameTickTime,
                          pTimerStuff[ii].dwFrameStampTime,
                          pTimerStuff[ii].nDummyFrames,
			  pTimerStuff[ii].nFramesAppended
                          );
            }

            GlobalUnlock (hMem);
            GlobalUnlock (hMemTimers);

            if (OpenClipboard (lpcs->hwnd))
            {
                EmptyClipboard ();
                SetClipboardData (CF_RIFF, hMemTimers);
                SetClipboardData (CF_TEXT, hMem);
                CloseClipboard ();
            }
            else
            {
                GlobalFree (hMem);
                GlobalFree (hMemTimers);
            }
        }
        else
        {
            // Failed to allocate or lock hMem.  Cleanup.
            //
            if (hMem)
                GlobalFree(hMem);

            // Free off the timer block.  (We have not set the
            // clipboard data.)
            //
            if (hMemTimers)
            {
                GlobalUnlock(hMemTimers);
                GlobalFree(hMemTimers);
            }
        }

        hMemTimers = NULL;
        pTimerRiff = NULL;
	pTimerStuff = NULL;
	pCurTimerStuff = NULL;
    }
   #endif

    return;
}


// Returns TRUE if the capture task was created, or
// capture completed OK.

BOOL AVICapture (LPCAPSTREAM lpcs)
{
    CAPINFOCHUNK cic;
    void (WINAPI _LOADDS * pfnCapture) (LPCAPSTREAM lpcs);

    if (lpcs->fCaptureFlags & CAP_fCapturingNow) {

	AuxDebugEx(4, DEBUGLINE "rejecting capture as previous capture still running\r\n");
        return IDS_CAP_VIDEO_OPEN_ERROR;
    }

    // if there is a previous capture thread, wait for it to finish and
    // clean it up
    // -it has set fCapturingNow to FALSE, so it will end 'soon' !
    if (lpcs->hThreadCapture) {
	AuxDebugEx(4, DEBUGLINE "Starting capture while previous capture thread still active\r\n");
        WaitForSingleObject(lpcs->hThreadCapture, INFINITE);

	CloseHandle(lpcs->hThreadCapture);
	lpcs->hThreadCapture = NULL;
    }

    // Turn off the STOP and ABORT capture bits
    lpcs->fCaptureFlags &= ~(CAP_fStopCapture | CAP_fAbortCapture);
    lpcs->dwReturn      = 0;

#if DONT_CLEAR_SMPTE_JAYBO
    // Prior to Win95, we always cleared out old SMPTE chunks,
    // but since Adobe may have created a chunk manually, don't
    // zap existing chunks.
    cic.fccInfoID = mmioFOURCC ('I','S','M','T');
    cic.lpData = NULL;
    cic.cbData = 0;
    SetInfoChunk (lpcs, &cic);
#endif

    // And get ready to write a SMPTE info chunk
    if (lpcs->sCapParms.fMCIControl) {
        // create SMPTE string
	CHAR szSMPTE[40];  // must write ansi
        TimeMSToSMPTE (lpcs->sCapParms.dwMCIStartTime, szSMPTE);
        cic.lpData = szSMPTE;
        cic.cbData = lstrlenA(szSMPTE) + 1;
        cic.fccInfoID = mmioFOURCC ('I','S','M','T');
        SetInfoChunk (lpcs, &cic);
    }

    // set pfnCapture to point to the capture function of choice.
    // Use an MCI device to do step capture capture???
    // assume No MCI device, just a normal streaming capture
    //
    pfnCapture = AVICapture1;
    if (lpcs->sCapParms.fStepMCIDevice && lpcs->sCapParms.fMCIControl)
        pfnCapture = MCIStepCapture;

    // if the fYield flag is true, create a thread to do the
    // capture loop.  otherwise do the capture loop inline
    //
    if (lpcs->sCapParms.fYield)
    {
        DWORD tid;

        lpcs->fCaptureFlags |= CAP_fCapturingNow;
	// future operations on this thread are now locked out.
	// we must turn this flag off if the thread creation fails

        lpcs->hThreadCapture = CreateThread (NULL,
                                             0,
                                             (LPTHREAD_START_ROUTINE) pfnCapture,
                                             lpcs,
                                             0,
                                             &tid);


        // if thread creation failed, turn off the capturing flag
        //
        if ( ! lpcs->hThreadCapture) {
	    AuxDebugEx(1,"Failed to create capture thread");
            lpcs->fCaptureFlags &= ~CAP_fCapturingNow;
	}

        return (lpcs->hThreadCapture != NULL);
    }
    else
    {
        pfnCapture (lpcs);
        return (0 == lpcs->dwReturn);
    }
}
