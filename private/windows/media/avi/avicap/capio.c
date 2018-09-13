/****************************************************************************
 *
 *   capio.c
 *
 *   i/o routines for video capture
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

//#define USE_AVIFILE 1
#define JMK_HACK_DONTWRITE TRUE

#define INC_OLE2
#pragma warning(disable:4103)
#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <msvideo.h>
#include <drawdib.h>
#include <mmreg.h>
#include <mmddk.h>

#include "ivideo32.h"
#include "mmdebug.h"

#ifdef USE_ACM
#include <msacm.h>
#endif

#include <avifmt.h>
#include "avicap.h"
#include "avicapi.h"
#include "time.h"

extern UINT GetSizeOfWaveFormat (LPWAVEFORMATEX lpwf);
STATICFN BOOL IndexVideo (LPCAPSTREAM lpcs, DWORD dwSize, BOOL bKeyFrame);
STATICFN BOOL IndexAudio (LPCAPSTREAM lpcs, DWORD dwSize);

#ifdef _DEBUG
    #define DSTATUS(lpcs, sz) statusUpdateStatus(lpcs, IDS_CAP_INFO, (LPTSTR) TEXT(sz))
#else
    #define DSTATUS(lpcs, sz)
#endif

#ifdef USE_AVIFILE
VOID WINAPI AVIPreloadFat (LPCAPSTREAM lpcs)
{
    return;
}

// Add an index entry for an audio buffer
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
//
STATICFN BOOL IndexAudio (LPCAPSTREAM lpcs, DWORD dwSize)
{
    ++lpcs->dwWaveChunkCount;
    return TRUE;
}

// Add an index entry for a video frame
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
//
STATICFN BOOL IndexVideo (LPCAPSTREAM lpcs, DWORD dwSize, BOOL bKeyFrame)
{
    ++lpcs->dwVideoChunkCount;
    return TRUE;
}

STATICFN void AVIFileCleanup(LPCAPSTREAM lpcs)
{
    if (lpcs->paudio)
        AVIStreamClose(lpcs->paudio), lpcs->paudio = NULL;
    if (lpcs->pvideo)
        AVIStreamClose(lpcs->pvideo), lpcs->pvideo = NULL;
    if (lpcs->pavifile)
        AVIFileClose(lpcs->pavifile), lpcs->pavifile = NULL;
}

/*
 * CapFileInit
 *
 *       Perform all initialization required to write a capture file.
 *
 *       We take a slightly strange approach: We don't write
 *       out the header until we're done capturing.  For now,
 *       we just seek 2K into the file, which is where all of
 *       the real data will go.
 *
 *       When we're done, we'll come back and write out the header,
 *       because then we'll know all of the values we need.
 *
 *      Also allocate and init the index.
 */

BOOL CapFileInit (
    LPCAPSTREAM lpcs)
{
    AVISTREAMINFOW si;
    LPBYTE ptr = NULL;
    UINT   cksize;
    RGBQUAD FAR * prgb;
    PALETTEENTRY aPalEntry[256];
    LPBITMAPINFO lpBitsInfoOut;    // Possibly compressed output format
    LONG lRet;
    UINT ii;

    // No special video format given -- use the default
    //
    lpBitsInfoOut = lpcs->CompVars.lpbiOut;
    if (lpcs->CompVars.hic == NULL)
        lpBitsInfoOut = lpcs->lpBitsInfo;

    // use avifile to access the data

    // create the avifile object, create a video and audio stream and
    // set the format for each stream.


    assert(lpcs->pavifile == NULL);

    /* if the capture file has not been set then error */
    if (!(*lpcs->achFile))
        goto error_exit;

    // !!! how to avoid truncating the file if already created ?

    lRet = AVIFileOpen(&lpcs->pavifile,
                    lpcs->achFile,
                    OF_WRITE | OF_CREATE,
                    NULL);
    if (lRet || !lpcs->pavifile)
        goto error_exit;

    // create video stream
    ZeroMemory (&si, sizeof(si));
    si.fccType = streamtypeVIDEO;

    if (lpcs->CompVars.hic)
        si.fccHandler = lpcs->CompVars.fccHandler;
    else
        si.fccHandler = lpBitsInfoOut->bmiHeader.biCompression;

    // A bit of history...
    // In VFW 1.0, we set fccHandler to 0 for BI_RLE8 formats
    // as a kludge to make Mplayer and Videdit play the files.
    // Just prior to 1.1 release, we found this broke Premiere,
    // so now (after AVICAP beta is on Compuserve), we change the
    // fccHandler to "MRLE".  Just ask Todd...
    // And now, at RC1, we change it again to "RLE ", Just ask Todd...
    if (si.fccHandler == BI_RLE8)
        si.fccHandler = mmioFOURCC('R', 'L', 'E', ' ');

    // !!!need to change this after capture
    si.dwScale = lpcs->sCapParms.dwRequestMicroSecPerFrame;



    si.dwRate = 1000000L;
    si.dwStart = 0L;
    si.dwQuality = (DWORD) -1L;         /* !!! ICQUALITY_DEFAULT */
    si.dwSampleSize = 0L;

    lRet = AVIFileCreateStream(lpcs->pavifile, &lpcs->pvideo, &si);
    if (lRet || !lpcs->pvideo)
        goto error_exit;

    // set format of video stream
    //  !!! dont write palette for full color?
    if (lpBitsInfoOut->bmiHeader.biBitCount > 8)
        lpBitsInfoOut->bmiHeader.biClrUsed = 0;

    // need to alloc a single block that we can fill with hdr + palette
    cksize = lpBitsInfoOut->bmiHeader.biSize
             + lpBitsInfoOut->bmiHeader.biClrUsed * sizeof(RGBQUAD);
    ptr = GlobalAllocPtr(GPTR, cksize);
    if (!ptr)
        goto error_exit;

    CopyMemory (ptr, (LPBYTE)&lpBitsInfoOut->bmiHeader,
                lpBitsInfoOut->bmiHeader.biSize);
    prgb = (RGBQUAD FAR *) &ptr[lpBitsInfoOut->bmiHeader.biSize];

    if (lpBitsInfoOut->bmiHeader.biClrUsed > 0) {
        // Get Palette info
        UINT nPalEntries = GetPaletteEntries(lpcs->hPalCurrent, 0,
                                             lpBitsInfoOut->bmiHeader.biClrUsed,
                                             aPalEntry);

        if (nPalEntries != lpBitsInfoOut->bmiHeader.biClrUsed)
            goto error_exit;

        for (ii = 0; ii < lpBitsInfoOut->bmiHeader.biClrUsed; ++ii)  {
            prgb[ii].rgbRed = aPalEntry[ii].peRed;
            prgb[ii].rgbGreen = aPalEntry[ii].peGreen;
            prgb[ii].rgbBlue = aPalEntry[ii].peBlue;
        }
    }
    if (AVIStreamSetFormat(lpcs->pvideo, 0, ptr, cksize))
        goto error_exit;

    GlobalFreePtr(ptr), ptr = NULL;


    // create audio stream if sound capture enabled
    if (lpcs->sCapParms.fCaptureAudio) {

         ZeroMemory (&si, sizeof(si));
         si.fccType = streamtypeAUDIO;
         si.fccHandler = 0L;
         si.dwScale = lpcs->lpWaveFormat->nBlockAlign;
         si.dwRate = lpcs->lpWaveFormat->nAvgBytesPerSec;
         si.dwStart = 0L;
         si.dwLength =  lpcs->dwWaveBytes / lpcs->lpWaveFormat->nBlockAlign;
         si.dwQuality = (DWORD)-1L;    /* !!! ICQUALITY_DEFAULT */
         si.dwSampleSize = lpcs->lpWaveFormat->nBlockAlign;

        lRet = AVIFileCreateStream(lpcs->pavifile, &lpcs->paudio, &si);
        if (lRet || !lpcs->paudio)
            goto error_exit;

        // write wave stream format

        cksize = GetSizeOfWaveFormat (lpcs->lpWaveFormat);

        if (AVIStreamSetFormat(lpcs->paudio, 0, lpcs->lpWaveFormat, cksize))
            goto error_exit;
    }

    // start streaming
    //
    // parameters are random for now, and are not used at all by current impl.
    // probably covered by above call but you never know
    //
    AVIStreamBeginStreaming(lpcs->pvideo, 0, 32000, 1000);
    if (lpcs->sCapParms.fCaptureAudio)
        AVIStreamBeginStreaming(lpcs->paudio, 0, 32000, 1000);

    // this is used for timing calcs, not just indexing
    //
    lpcs->dwVideoChunkCount = 0;
    lpcs->dwWaveChunkCount  = 0;

    // !!! write info chunks here

    return TRUE;

error_exit:
    if (ptr) {
        GlobalFreePtr(ptr); ptr = NULL;
    }
    AVIFileCleanup (lpcs);
    return FALSE;
}

/*
 * AVIFileFini
 *
 *       Write out the index, deallocate the index, and close the file.
 *
 */

BOOL AVIFileFini (LPCAPSTREAM lpcs, BOOL fWroteJunkChunks, BOOL fAbort)
{
    AVISTREAMINFOW si;

    DPF("AVICap32:    Start of AVIFileFini\n");

    AVIStreamEndStreaming(lpcs->pvideo);
    if (lpcs->sCapParms.fCaptureAudio)
        AVIStreamEndStreaming(lpcs->paudio);

    // if we got a good file, allow editing of it
    lpcs->fFileCaptured = !fAbort;

    // -----------------------------------------------------------
    // adjust audio & video streams to be the same length
    // -----------------------------------------------------------

   #if 0 // old technique - match video to audio unconditionally
    // share the captured frames out evenly over the captured audio

    if (lpcs->sCapParms.fCaptureAudio && lpcs->dwVideoChunkCount &&
            (lpcs->dwWaveBytes > 0)) {

         /* HACK HACK */
         /* Set rate that was captured based on length of audio data */

        lpcs->dwActualMicroSecPerFrame = (DWORD)
            MulDiv((LONG)lpcs->dwWaveBytes,
                     1000000,
                     (LONG)(lpcs->lpWaveFormat->nAvgBytesPerSec * lpcs->dwVideoChunkCount));
    } else {
        lpcs->dwActualMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;
    }
   #else // new technique for stream length munging
    //
    // Init a value in case we're not capturing audio
    //
    lpcs->dwActualMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;

    switch (lpcs->sCapParms.AVStreamMaster) {
        case AVSTREAMMASTER_NONE:
            lpcs->dwActualMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;
            break;

        case AVSTREAMMASTER_AUDIO:
        default:
            // VFW 1.0 and 1.1 ALWAYS munged frame rate to match audio
            // duration.
            if (lpcs->sCapParms.fCaptureAudio && lpcs->dwVideoChunkCount) {
                // Modify the video framerate based on audio duration
                lpcs->dwActualMicroSecPerFrame = (DWORD)
                    ((double)lpcs->dwWaveBytes * 1000000. /
                    ((double)lpcs->lpWaveFormat->nAvgBytesPerSec *
                    lpcs->dwVideoChunkCount + 0.5));
            }
            break;
    }
   #endif

    // ------------------------------------------------------------
    // write corrected stream timing back to the file
    // ------------------------------------------------------------

   #ifdef CHICAGO
    AVIStreamInfo (lpcs->pvideo, (LPAVISTREAMINFOA) &si, sizeof(si));
   #else
    AVIStreamInfo (lpcs->pvideo, &si, sizeof(si));
   #endif

    si.dwRate = 1000000L;
    si.dwScale = lpcs->dwActualMicroSecPerFrame;

    // no api for this- have to call the member directly!
    //
    lpcs->pvideo->lpVtbl->SetInfo(lpcs->pvideo, &si, sizeof(si));

    // Add the info chunks
    // This includes the capture card driver name, client app, date and time

    if (lpcs->lpInfoChunks) {
        LPBYTE  lpInfo;
        DWORD   cbData;

        lpInfo = lpcs->lpInfoChunks;
        while (lpInfo < (LPBYTE) lpcs->lpInfoChunks + lpcs->cbInfoChunks) {
            cbData = * (LPDWORD) (lpInfo + sizeof(DWORD));
            AVIFileWriteData (lpcs->pavifile,
                        (DWORD) * (LPDWORD) lpInfo,        // FOURCC
                        lpInfo + sizeof (DWORD) * 2,       // lpData
                        cbData);                           // cbData
            lpInfo += cbData + sizeof (DWORD) * 2;
        }
    }


    AVIFileCleanup(lpcs);

    return lpcs->fFileCaptured;
}

//
// Prepends dummy frame entries to the current valid video frame.
// Bumps the index, but does not actually trigger a write operation.
// nCount is a count of the number of frames to write
// Returns: TRUE on a successful write

BOOL WINAPI AVIWriteDummyFrames (
    LPCAPSTREAM lpcs,
    UINT        nCount,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    LONG lRet;

    lpcs->dwVideoChunkCount += nCount;

    lRet = AVIStreamWrite(lpcs->pvideo,
                    -1,             // current position
                    nCount,         // this many samples
                    NULL,           // no actual data
                    0,              // no data
                    0,              // not keyframe
                    NULL, NULL);    // no return of samples or bytes
    *lpbPending = FALSE;
    *lpuError = 0;
    if (lRet)
        *lpuError = IDS_CAP_FILE_WRITE_ERROR;
    return !(*lpuError);
}

// Writes compressed or uncompressed frames to the AVI file
// returns TRUE if no error, FALSE if end of file.

BOOL WINAPI AVIWriteVideoFrame (
    LPCAPSTREAM lpcs,
    LPBYTE      lpData,
    DWORD       dwBytesUsed,
    BOOL        fKeyFrame,
    UINT        uIndex,
    UINT        nDropped,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    LONG   lRet;

    lRet = AVIStreamWrite(lpcs->pvideo,     // write to video stream
                    -1,                     // next sample
                    1,                      // 1 sample only
                    lpData,                 // video buffer (no riff header)
                    dwBytesUsed,            // length of data
                    fKeyFrame ? AVIIF_KEYFRAME : 0,
                    NULL, NULL);    // no return of sample or byte count

    *lpbPending = FALSE;
    *lpuError = 0;
    if (lRet)
    {
        dprintf("AVIStreamWrite returned 0x%x", lRet);
        *lpuError = IDS_CAP_FILE_WRITE_ERROR;
    }
    else
    {
        ++lpcs->dwVideoChunkCount;
        if (nDropped)
            AVIWriteDummyFrames (lpcs, nDropped, lpuError, lpbPending);
    }
    return !(*lpuError);
}

BOOL WINAPI AVIWriteAudio (
    LPCAPSTREAM lpcs,
    LPWAVEHDR   lpWaveHdr,
    UINT        uIndex,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    LONG lRet;

    lRet = AVIStreamWrite(lpcs->paudio,
                -1,                 // next sample
                lpWaveHdr->dwBytesRecorded /
                    lpcs->lpWaveFormat->nBlockAlign,    // nr samples
                lpWaveHdr->lpData,
                lpWaveHdr->dwBytesRecorded,
                0,
                NULL,
                NULL);

    *lpbPending = FALSE;
    *lpuError = 0;
    if (lRet)
    {
        dprintf("AVIStreamWrite returned 0x%x", lRet);
        *lpuError = IDS_CAP_FILE_WRITE_ERROR;
    }
    else
        ++lpcs->dwWaveChunkCount;

    return !(*lpuError);
}

#else //---------------- ! using Avifile ----------------------------

// The following are anded with the size in the index
#define IS_AUDIO_CHUNK        0x80000000
#define IS_KEYFRAME_CHUNK     0x40000000
#define IS_DUMMY_CHUNK        0x20000000
#define IS_GRANULAR_CHUNK     0x10000000
#define INDEX_MASK  (IS_AUDIO_CHUNK | IS_KEYFRAME_CHUNK | IS_DUMMY_CHUNK | IS_GRANULAR_CHUNK)


// Add an index entry for a video frame
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
//
STATICFN BOOL IndexVideo (LPCAPSTREAM lpcs, DWORD dwSize, BOOL bKeyFrame)
{
    if (lpcs->dwIndex < lpcs->sCapParms.dwIndexSize) {
        *lpcs->lpdwIndexEntry = dwSize | (bKeyFrame ? IS_KEYFRAME_CHUNK : 0);
        ++lpcs->lpdwIndexEntry;
        ++lpcs->dwIndex;
        ++lpcs->dwVideoChunkCount;
        return TRUE;
    }
    dprintf("\n***WARNING*** Indexvideo space exhausted\n");
    return FALSE;
}

// Add an index entry for an audio buffer
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
//
STATICFN BOOL IndexAudio (LPCAPSTREAM lpcs, DWORD dwSize)
{
    if (lpcs->dwIndex < lpcs->sCapParms.dwIndexSize) {
       *lpcs->lpdwIndexEntry = dwSize | IS_AUDIO_CHUNK;
       ++lpcs->lpdwIndexEntry;
       ++lpcs->dwIndex;
       ++lpcs->dwWaveChunkCount;
       return TRUE;
    }
    dprintf("\n***WARNING*** Indexaudio space exhausted\n");
    return FALSE;
}

DWORD CalcWaveBufferSize (LPCAPSTREAM lpcs)
{
    DWORD dw;

    if (!lpcs->lpWaveFormat)
        return 0L;

    // at least .5 second
    dw = lpcs->lpWaveFormat->nAvgBytesPerSec / 2;
    if (lpcs->sCapParms.wChunkGranularity) {
        if (dw % lpcs->sCapParms.wChunkGranularity) {
            dw += lpcs->sCapParms.wChunkGranularity -
                dw % lpcs->sCapParms.wChunkGranularity;
        }
    }
    dw = max ((1024L * 16), dw);                // at least 16K
    dw -= sizeof(RIFF);

    dprintf("Wave buffer size = %ld", dw);
    return dw;
}

/*
 * AVIPreloadFat
 *
 *   Force FAT for this file to be loaded into the FAT cache
 *
 */

VOID WINAPI AVIPreloadFat (LPCAPSTREAM lpcs)
{
    DWORD dw;
   #ifdef CHICAGO
    DWORD dwPos;

    assert (lpcs->lpDropFrame);

    // save the current file pointer then seek to the end of the file
    //
    dwPos = SetFilePointer (lpcs->hFile, 0, NULL, FILE_CURRENT);
    dw = SetFilePointer (lpcs->hFile, 0, NULL, FILE_END);
    if ((dw == (DWORD)-1) || (dw < lpcs->dwBytesPerSector)) {
        // put the file pointer back to where it was
        SetFilePointer (lpcs->hFile, dwPos, NULL, FILE_BEGIN);
        return;
    }

    // read the last sector of the file, just to force
    // the fat for the file to be loaded
    //
    ReadFile (lpcs->hFile, lpcs->lpDropFrame, lpcs->dwBytesPerSector, &dw, NULL);

    // put the file pointer back to where it was
    //
    SetFilePointer (lpcs->hFile, dwPos, NULL, FILE_BEGIN);
   #else
    // Load all the FAT information.   On NT this is sufficient for FAT
    // files.  On NTFS partitiions there is no way we can read in all the
    // mapping information.
    GetFileSize(lpcs->hFile, &dw);
   #endif
}


#ifdef JMK_HACK_DONTWRITE
static BOOL bDontWrite;
#endif

// Write data to the capture file
// Returns: TRUE on a successful write
//
UINT NEAR PASCAL AVIWrite (
    LPCAPSTREAM lpcs,
    LPVOID      pbuf,
    DWORD       dwSize,
    UINT        uIndex, // index of header for this buffer, -1 for step capture
    UINT        uType,
    LPBOOL      lpbPending)
{
    DWORD dwWritten;
    DWORD dwGran;

    // the buffer must be sector aligned if using non-buffered IO
    // and the size must be at least word aligned
    // uIndex == -1 if this is a dummy frame write
    // uIndex == Index into alpVideoHdr OR index in alpWaveHdr based on uType
    //
    assert (!lpcs->fUsingNonBufferedIO || (!((DWORD)pbuf & (lpcs->dwBytesPerSector - 1))));
    assert (!(dwSize & 1));
    assert (dwSize);

    assert (*lpbPending == FALSE);

    // if we are doing non-buffered io, we need to pad each write
    // to an even multiple of sector size bytes, we do this by adding
    // a junk riff chunk into the write buffer after dwSize bytes
    //
    dwGran = lpcs->sCapParms.wChunkGranularity;
    if (lpcs->fUsingNonBufferedIO)
       dwGran = max (lpcs->dwBytesPerSector,
                (DWORD) lpcs->sCapParms.wChunkGranularity);

    assert (dwGran);

    if (dwSize % dwGran)
    {
        DWORD dwSizeT = dwGran - (dwSize % dwGran);
        LPRIFF priff = (LPRIFF)((LPBYTE)pbuf + dwSize + (dwSize & 1));

        if (dwSizeT < sizeof(RIFF))
            dwSizeT += dwGran;

        // add the junk riff chunk to the end of the buffer
        //
        priff->dwType = ckidAVIPADDING;
        priff->dwSize = dwSizeT - sizeof(RIFF);
        dwSize += dwSizeT;
    }

   #ifdef _DEBUG
    if (dwSize)
    {
        volatile BYTE bt;
        AuxDebugEx (8, DEBUGLINE "touch test of AviWrite buffer %08X\r\n", pbuf);
        bt = ((LPBYTE)pbuf)[dwSize-1];
    }

    // List all of the RIFF chunks within the block being written
    //
    dwWritten = 0;
    while (dwWritten < dwSize)
    {
        LPRIFF priff = (LPVOID)((LPBYTE)pbuf + dwWritten);
        AuxDebugEx (4, DEBUGLINE "RIFF=%.4s size=%08X\r\n",
                   &priff->dwType, priff->dwSize);
        dwWritten += priff->dwSize + sizeof(RIFF);
    }
   #endif


    // BUGBUG, Remove the following line when done performance testing
   #ifdef JMK_HACK_DONTWRITE
    if (bDontWrite)
        return 0;
   #endif

    if (lpcs->pAsync)
    {
        struct _avi_async * lpah = &lpcs->pAsync[lpcs->iLastAsync];
        UINT  iLastAsync;

        // set iLastAsync to point to what lpcs->iLastAsync
        // would be if we were to increment it.  If we end up
        // with an i/o that does not complete synchronously
        // we will then update lpcs->iLastAsync so that we can
        // remember to check for completion later
        //
        if ((iLastAsync = lpcs->iLastAsync+1) >= lpcs->iNumAsync)
            iLastAsync = 0;

        // is the async buffer that we are trying to use
        // already in use?
        //
        if (iLastAsync == lpcs->iNextAsync) {
	    AuxDebugEx(1, DEBUGLINE "async buffer already in use\r\n");
            return IDS_CAP_FILE_WRITE_ERROR;
	}
        assert (!lpah->uType);

        // init the async buffer with the info that we will need
        // to release the buffer when the io is complete
        //
        ZeroMemory (&lpah->ovl, sizeof(lpah->ovl));
        lpah->ovl.hEvent = lpcs->hCaptureEvent;
        lpah->ovl.Offset = lpcs->dwAsyncWriteOffset;
        // attempt an async write.  if WriteFile fails, we then
        // need to check if it's a real failure, or just an instance
        // of delayed completion.  if delayed completion, we fill out
        // the lpah structure so that we know what buffer to re-use
        // when the io finally completes.
        //
	if ( ! WriteFile (lpcs->hFile, pbuf, dwSize, &dwWritten, &lpah->ovl))
        {
            if (GetLastError() == ERROR_IO_PENDING)
            {

                // if we are passed a index of -1, that means that
                // this buffer is not associated with any entry in the
                // header array.  in this case, we must have the io complete
                // before we return from this function.
               //
               if (uIndex == (UINT)-1)
               {
                   while (1) {
                    if ( ! GetOverlappedResult (lpcs->hFile, &lpah->ovl, &dwWritten, TRUE))
                    {
                        AuxDebugEx (0, DEBUGLINE "WriteFile failed %d\r\n", GetLastError());
                        return IDS_CAP_FILE_WRITE_ERROR;
                    }
			// if Internal is non-zero, the io is still pending
	    // NOTE: this needs to be changed as soon as possible.
	    //	The current state of play means that GetOverlappedResult CAN return
	    //	something other than ERROR_IO_COMPLETE and the IO will still be
	    //	pending.  This causes us to make the wrong decision.
                        if (lpah->ovl.Internal != 0) {
			    Sleep(100);  // release some cycles
			}
			else
			    break;
                   }
                   //#pragma message(SQUAWK "get rid of this hack when GetOverlappedResult is fixed!")
                }
                else
                {
                    // io is begun, but not yet completed. so setup info in
                    // the pending io array so that we can check later for completion
                    //
                    *lpbPending = TRUE;
                    lpah->uType = uType;
                    lpah->uIndex = uIndex;
		    AuxDebugEx(2, DEBUGLINE "IOPending... iLastAsync was %d, will be %d, uIndex=%d, Event=%d\r\n",lpcs->iLastAsync , iLastAsync, uIndex, lpah->ovl.hEvent);
                    lpcs->iLastAsync = iLastAsync;
                }
            }
            else
            {
                AuxDebugEx (0, DEBUGLINE "WriteFile failed %d\r\n", GetLastError());
                return IDS_CAP_FILE_WRITE_ERROR;
	    }
	}

        // we get to here only when the io succeeds or is pending
        // so update the seek offset for use in the next write operation
        //
        lpcs->dwAsyncWriteOffset += dwSize;
    }
    else
    {
	// We are writing synchronously to the file
        if (!WriteFile (lpcs->hFile, pbuf, dwSize, &dwWritten, NULL) ||
            !(dwWritten == dwSize))
            return IDS_CAP_FILE_WRITE_ERROR;
    }

    return 0;
}

/*
 * CapFileInit
 *
 *       Perform all initialization required to write a capture file.
 *
 *       We take a slightly strange approach: We don't write
 *       out the header until we're done capturing.  For now,
 *       we just seek 2K into the file, which is where all of
 *       the real data will go.
 *
 *       When we're done, we'll come back and write out the header,
 *       because then we'll know all of the values we need.
 *
 *      Also allocate and init the index.
 */
BOOL CapFileInit (LPCAPSTREAM lpcs)
{
    LONG l;
    LPBITMAPINFO lpBitsInfoOut;    // Possibly compressed output format
    DWORD dwOpenFlags;

    // No special video format given -- use the default
    lpBitsInfoOut = lpcs->CompVars.lpbiOut;
    if (lpcs->CompVars.hic == NULL)
        lpBitsInfoOut = lpcs->lpBitsInfo;


    assert (lpcs->hmmio == NULL);   // Should never have a file handle on entry

    // if the capture file has not been set then set it now
    if (!(*lpcs->achFile))
         goto INIT_FILE_OPEN_ERROR;

    // Get the Bytes per sector for the drive
    {
        DWORD dwSectorsPerCluster;
        DWORD dwFreeClusters;
        DWORD dwClusters;
        TCHAR szFullPathName[MAX_PATH];
        LPTSTR pszFilePart;

        GetFullPathName (lpcs->achFile,
                NUMELMS (szFullPathName),
                szFullPathName,
                &pszFilePart);

        if (szFullPathName[1] == TEXT(':') && szFullPathName[2] == TEXT('\\')) {
            szFullPathName[3] = TEXT('\0');  // Terminate after "x:\"

            GetDiskFreeSpace (szFullPathName,
                   &dwSectorsPerCluster,
                   &lpcs->dwBytesPerSector,
                   &dwFreeClusters,
                   &dwClusters);
            AuxDebugEx (3, DEBUGLINE "BytesPerSector=%d\r\n",
                lpcs->dwBytesPerSector);
        }
        else {
            // This handles cases where we do not have a "x:\" filename
            // Principally this will be "\\server\name\path..."
            lpcs->dwBytesPerSector = DEFAULT_BYTESPERSECTOR;
            AuxDebugEx (3, DEBUGLINE "FullPath=%s\r\n", szFullPathName);
            AuxDebugEx (3, DEBUGLINE "GetFullPath failed, Forcing dwBytesPerSector to %d\r\n",DEFAULT_BYTESPERSECTOR);
        }

    // bytes per sector must be non-zero and a power of two
    //
    assert (lpcs->dwBytesPerSector);
    assert (!(lpcs->dwBytesPerSector & (lpcs->dwBytesPerSector-1)));
    }

   #ifdef ZERO_THE_FILE_FOR_TESTING
    {
    char c[64 * 1024];
    DWORD dwSize;
    DWORD dwBW;
    // Open the file just to zero it

    lpcs->hFile = CreateFile (lpcs->achFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (lpcs->hFile == INVALID_HANDLE_VALUE) {
        lpcs->hFile = 0;
        goto INIT_FILE_OPEN_ERROR;
    }

    ZeroMemory (c, sizeof(c));
    SetFilePointer (lpcs->hFile, 0, NULL, FILE_BEGIN);
    dwSize = GetFileSize (lpcs->hFile, NULL);

    while (SetFilePointer (lpcs->hFile, 0, NULL, FILE_CURRENT) < dwSize)
            WriteFile (lpcs->hFile, c, sizeof(c), &dwBW, NULL);
    }

    CloseHandle(lpcs->hFile);  // Close the "normal" open
   #endif

    // We can use non-buffered I/O if the ChunkGranularity is
    // a multiple of BytesPerSector.  Better check that wChunkGranularity
    // has indeed been set

    if (0 == lpcs->sCapParms.wChunkGranularity)
        lpcs->sCapParms.wChunkGranularity = lpcs->dwBytesPerSector;

    lpcs->fUsingNonBufferedIO =
            (lpcs->sCapParms.wChunkGranularity >= lpcs->dwBytesPerSector) &&
           ((lpcs->sCapParms.wChunkGranularity % lpcs->dwBytesPerSector) == 0) &&
            (lpcs->CompVars.hic == NULL) &&
            (!(lpcs->fCaptureFlags & CAP_fStepCapturingNow)) &&
            (!(lpcs->fCaptureFlags & CAP_fFrameCapturingNow));

    AuxDebugEx (3, DEBUGLINE "fUsingNonBufferedIO=%d\r\n", lpcs->fUsingNonBufferedIO);

    // setup CreateFile flags based on whether we are using
    // non-buffered io and/or overlapped io
    //
    dwOpenFlags = FILE_ATTRIBUTE_NORMAL;
    if (lpcs->fUsingNonBufferedIO)
    {
        dwOpenFlags |= FILE_FLAG_NO_BUFFERING;
       #ifdef CHICAGO
        #pragma message (SQUAWK "find a better way to set AsyncIO flag")
        if (GetProfileIntA ("Avicap32", "AsyncIO", FALSE))
       #else
            // give a way to override the async default option.
            if (!GetProfileIntA ("Avicap32", "AsyncIO", TRUE)) {
		AuxDebugEx (2, DEBUGLINE "NOT doing Async IO\r\n");
	    } else
       #endif
        {
            AuxDebugEx (3, DEBUGLINE "Doing Async IO\r\n");
            dwOpenFlags |= FILE_FLAG_OVERLAPPED;

            // We are requested to do async io.  Allocate an array
            // of async io headers and initialize the async io fields
            // in the CAPSTREAM structure
            //
            {
                UINT iNumAsync = NUMELMS(lpcs->alpVideoHdr) + NUMELMS(lpcs->alpWaveHdr) + 2;
		// This is quite a lot of buffers.  Perhaps we should limit
		// ourselves to lpcs->iNumVideo and lpcs->iNumAudio EXCEPT
		// these fields have not yet been set up.  We would need
		// to look in the cap stream structure to get the information.
		// It is simpler to assume the maximum numbers.
                lpcs->dwAsyncWriteOffset = lpcs->dwAVIHdrSize;
                lpcs->iNextAsync = lpcs->iLastAsync = 0;
                lpcs->pAsync = GlobalAllocPtr (GMEM_MOVEABLE | GMEM_ZEROINIT,
                                               sizeof(*lpcs->pAsync) * iNumAsync);
                if (lpcs->pAsync) {
                    lpcs->iNumAsync = iNumAsync;
                } else {
                    // cannot allocate the memory.  Go synchronous
                    dprintf("Failed to allocate async buffers");
                    dwOpenFlags &= ~(FILE_FLAG_OVERLAPPED);
		}
            }
        }
    }

    // Open the capture file, using Non Buffered I/O
    // if possible, given sector size, and buffer granularity
    //
    lpcs->hFile = CreateFile (lpcs->achFile,
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_ALWAYS,
                    dwOpenFlags,
                    NULL);

    if (lpcs->hFile == INVALID_HANDLE_VALUE) {
        lpcs->hFile = 0;
        goto INIT_FILE_OPEN_ERROR;
    }

    // BUGBUG, Remove the following line when done performance testing
   #ifdef JMK_HACK_DONTWRITE
    bDontWrite = GetProfileIntA("AVICAP32", "DontWrite", FALSE);
   #endif

    // Seek to a multiple of ChunkGranularity + AVIHEADERSIZE.
    // This is the point at which we'll start writing
    // Later, we'll come back and fill in the AVI header and index.

    // l is zero for standard wave and video formats
    l = (GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat) -
                sizeof (PCMWAVEFORMAT)) +
                (lpBitsInfoOut->bmiHeader.biSize -
                sizeof (BITMAPINFOHEADER));

    // (2K + size of wave and video stream headers) rounded to next 2K
    lpcs->dwAVIHdrSize = AVI_HEADERSIZE +
        (((lpcs->cbInfoChunks + l + lpcs->sCapParms.wChunkGranularity - 1)
        / lpcs->sCapParms.wChunkGranularity) * lpcs->sCapParms.wChunkGranularity);


    dprintf("AVIHdrSize = %ld", lpcs->dwAVIHdrSize);

    SetFilePointer (lpcs->hFile, lpcs->dwAVIHdrSize, NULL, FILE_BEGIN);
    if (lpcs->pAsync)
        lpcs->dwAsyncWriteOffset = lpcs->dwAVIHdrSize;

    // do all Index allocations
    if (!InitIndex (lpcs))
        CloseHandle (lpcs->hFile), lpcs->hFile = 0;

    lpcs->dwVideoChunkCount = 0;
    lpcs->dwWaveChunkCount = 0;

INIT_FILE_OPEN_ERROR:
    if (lpcs->hFile) {
	return(TRUE);
    }
    if (lpcs->pAsync) {
	GlobalFreePtr(lpcs->pAsync), lpcs->pAsync=NULL;
    }
    return (FALSE);
}

///////////////////////////////////////////////////////////////////////////
//  The index array is used to record the positions
//  of every chunk in the RIFF (avi) file.
//
//  what this array is:
//
//      each entry contains the size of the data
//      high order bits encode the type of data (audio / video)
//      and whether the video chunk is a key frame, dropped frame, etc.
///////////////////////////////////////////////////////////////////////////

// Allocate the index table
// Returns: TRUE if index can be allocated
//
BOOL InitIndex (LPCAPSTREAM lpcs)
{
    lpcs->dwIndex = 0;

    // we assume that we have not already allocated an index
    //
    assert (lpcs->lpdwIndexStart == NULL);

    // Limit index size between 1 minute at 30fps and 3 hours at 30fps
    lpcs->sCapParms.dwIndexSize = max (lpcs->sCapParms.dwIndexSize, 1800);
    lpcs->sCapParms.dwIndexSize = min (lpcs->sCapParms.dwIndexSize, 324000L);
    dprintf("Max Index Size = %ld", lpcs->sCapParms.dwIndexSize);

    if (lpcs->hIndex = GlobalAlloc (GMEM_MOVEABLE,
                lpcs->sCapParms.dwIndexSize * sizeof (DWORD))) {
        if (lpcs->lpdwIndexEntry =
            lpcs->lpdwIndexStart = (LPDWORD)GlobalLock (lpcs->hIndex))
            return TRUE;        // Success

        GlobalFree (lpcs->hIndex);
	lpcs->hIndex = NULL;
    }
    lpcs->lpdwIndexStart = NULL;
    return FALSE;
}

// Deallocate the index table
//
void FiniIndex (LPCAPSTREAM lpcs)
{
    if (lpcs->hIndex) {
        if (lpcs->lpdwIndexStart)
            GlobalUnlock (lpcs->hIndex);
        GlobalFree (lpcs->hIndex);
	lpcs->hIndex = NULL;
    }
    lpcs->lpdwIndexStart = NULL;
}



// Write out the index at the end of the capture file.
// The single frame capture methods do not append
// JunkChunks!  Audio chunks also now may have junk appended.
//
BOOL WriteIndex (LPCAPSTREAM lpcs, BOOL fJunkChunkWritten)
{
    BOOL  fChunkIsAudio;
    BOOL  fChunkIsKeyFrame;
    BOOL  fChunkIsDummy;
    BOOL  fChunkIsGranular;
    DWORD dwIndex;
    DWORD dw;
    DWORD dwJunk;
    DWORD off;
    AVIINDEXENTRY   avii;
    MMCKINFO    ck;
    LPDWORD lpdw;
    DWORD   dwGran;

    // determine which granularity (if any) to use
    // when calulating junk appended
    //
    dwGran = 0;
    if (fJunkChunkWritten)
    {
        dwGran = lpcs->sCapParms.wChunkGranularity;
        if (lpcs->fUsingNonBufferedIO)
           dwGran = max (lpcs->dwBytesPerSector, dwGran);
    }


    if (lpcs->dwIndex > lpcs->sCapParms.dwIndexSize)
        return TRUE;

    off        = lpcs->dwAVIHdrSize;

    ck.cksize  = 0;
    ck.ckid    = ckidAVINEWINDEX;
    ck.fccType = 0;

    if (mmioCreateChunk(lpcs->hmmio,&ck,0)) {
	dprintf("Failed to create chunk for index");
        return FALSE;
    }

    lpdw = lpcs->lpdwIndexStart;
    for (dwIndex= 0; dwIndex < lpcs->dwIndex; dwIndex++) {

        dw = *lpdw++;

        fChunkIsAudio      = (BOOL) ((dw & IS_AUDIO_CHUNK) != 0);
        fChunkIsKeyFrame   = (BOOL) ((dw & IS_KEYFRAME_CHUNK) != 0);
        fChunkIsDummy      = (BOOL) ((dw & IS_DUMMY_CHUNK) != 0);
        fChunkIsGranular   = (BOOL) ((dw & IS_GRANULAR_CHUNK) != 0);
        dw &= ~(INDEX_MASK);

        if (fChunkIsAudio) {
            avii.ckid         = MAKEAVICKID(cktypeWAVEbytes, 1);
            avii.dwFlags      = 0;
        } else {
            if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
                avii.ckid         = MAKEAVICKID(cktypeDIBcompressed, 0);
            else
                avii.ckid         = MAKEAVICKID(cktypeDIBbits, 0);
            avii.dwFlags      = fChunkIsKeyFrame ? AVIIF_KEYFRAME : 0;
        }
        avii.dwChunkLength    = dw;
        avii.dwChunkOffset    = off;

        if (mmioWrite(lpcs->hmmio, (LPVOID)&avii, sizeof(avii)) != sizeof(avii)) {
	    dprintf("Failed to write index chunk %d", dwIndex);
            return FALSE;
	}

        dw += sizeof (RIFF);
        // round to word boundary
        //
        dw += (dw & 1);
        off += dw;

        // If a Junk chunk was appended, move past it
        //
        if (fChunkIsGranular && dwGran && (off % dwGran)) {
            dwJunk = dwGran - (off % dwGran);

            if (dwJunk < sizeof (RIFF))
                off += dwGran;
            off += dwJunk;
        }
    }

    if (mmioAscend(lpcs->hmmio, &ck, 0)){
	dprintf("Failed to ascend at end of index writing");
        return FALSE;
    }

    return TRUE;
}

/*
 * AVIFileFini
 *
 *       Write out the index, deallocate the index, and close the file.
 *
 */
BOOL AVIFileFini (LPCAPSTREAM lpcs, BOOL fWroteJunkChunks, BOOL fAbort)
{
    MMCKINFO      ckRiff;
    MMCKINFO      ckList;
    MMCKINFO      ckStream;
    MMCKINFO      ck;
    UINT          ii;
    DWORD         dw;
    AVIStreamHeader        strhdr;
    DWORD         dwDataEnd;
    BOOL          fRet = TRUE;
    RGBQUAD       argbq[256];
    MainAVIHeader aviHdr;
    BOOL          fSound;
    LPBITMAPINFO  lpBitsInfoOut;    // Possibly compressed output format

    // No special video format given -- use the default
    //
    lpBitsInfoOut = lpcs->lpBitsInfo;
   #ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic != NULL)
        lpBitsInfoOut = lpcs->CompVars.lpbiOut;
   #endif

    // if the capture file has not been opened, we have nothing to do
    //
    if (lpcs->hFile == 0)
        return FALSE;

    // save off the current seek position.  this is the end of the capture
    // data.  then close the capture file,  we will do the final work
    // on the capture file using mmio & buffered io.
    //
    if (lpcs->pAsync)
        dwDataEnd = lpcs->dwAsyncWriteOffset;
    else
        dwDataEnd = SetFilePointer (lpcs->hFile, 0, NULL, FILE_CURRENT);

    CloseHandle (lpcs->hFile), lpcs->hFile = 0;

    // if we had allocated space for async buffers, free them now
    //
    if (lpcs->pAsync)
    {
        GlobalFreePtr (lpcs->pAsync);
        lpcs->pAsync = NULL;
        lpcs->iNextAsync = lpcs->iLastAsync = lpcs->iNumAsync = 0;
    }

    // if we are aborting capture, we are done
    lpcs->hmmio = mmioOpen(lpcs->achFile, NULL, MMIO_WRITE);
    assert (lpcs->hmmio != NULL);

    //
    if (fAbort)
        goto FileError;

    if (!lpcs->dwWaveBytes)
        fSound = FALSE;
    else
        fSound = lpcs->sCapParms.fCaptureAudio && (!(lpcs->fCaptureFlags & CAP_fFrameCapturingNow));

    // Seek to beginning of file, so we can write the header.
    mmioSeek(lpcs->hmmio, 0, SEEK_SET);

    DSTATUS(lpcs, "Writing AVI header");

    // Create RIFF chunk
    ckRiff.cksize = 0;
    ckRiff.fccType = formtypeAVI;
    if (mmioCreateChunk(lpcs->hmmio,&ckRiff,MMIO_CREATERIFF))
         goto FileError;

    // Create header list
    ckList.cksize = 0;
    ckList.fccType = listtypeAVIHEADER;
    if (mmioCreateChunk(lpcs->hmmio,&ckList,MMIO_CREATELIST))
         goto FileError;

    // Create AVI header chunk
    ck.cksize = sizeof(MainAVIHeader);
    ck.ckid = ckidAVIMAINHDR;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0))
         goto FileError;

    lpcs->dwAVIHdrPos = ck.dwDataOffset;

    // Calculate AVI header info
    //
    ZeroMemory (&aviHdr, sizeof(aviHdr));

    //
    // Set the stream lengths based on the Master stream
    //
   #if 0 // stream length calc with unconditional audio master
    aviHdr.dwMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;
    if (fSound && lpcs->dwVideoChunkCount) {
         /* HACK HACK */
         /* Set rate that was captured based on length of audio data */

         aviHdr.dwMicroSecPerFrame = (DWORD) MulDiv ((LONG)lpcs->dwWaveBytes,
                   1000000,
                   (LONG)(lpcs->lpWaveFormat->nAvgBytesPerSec * lpcs->dwVideoChunkCount));
    }
   #else

    // Init a value in case we're not capturing audio
    aviHdr.dwMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;

    switch (lpcs->sCapParms.AVStreamMaster) {
        case AVSTREAMMASTER_NONE:
            break;

        case AVSTREAMMASTER_AUDIO:
        default:
            // VFW 1.0 and 1.1 ALWAYS munged frame rate to match audio
            // duration.
            if (fSound && lpcs->sCapParms.fCaptureAudio && lpcs->dwVideoChunkCount) {
                // Modify the video framerate based on audio duration
                aviHdr.dwMicroSecPerFrame = (DWORD)
                    ((double)lpcs->dwWaveBytes * 1000000. /
                    ((double)lpcs->lpWaveFormat->nAvgBytesPerSec *
                    lpcs->dwVideoChunkCount + 0.5));
            }
            break;
    }
   #endif
    lpcs->dwActualMicroSecPerFrame = aviHdr.dwMicroSecPerFrame;

    aviHdr.dwMaxBytesPerSec = (DWORD) MulDiv (lpBitsInfoOut->bmiHeader.biSizeImage,
                                      1000000,
                                      lpcs->sCapParms.dwRequestMicroSecPerFrame) +
                                      (fSound ? lpcs->lpWaveFormat->nAvgBytesPerSec : 0);
    aviHdr.dwPaddingGranularity = 0L;
    aviHdr.dwFlags = AVIF_WASCAPTUREFILE | AVIF_HASINDEX;
    aviHdr.dwStreams = fSound ? 2 : 1;
    aviHdr.dwTotalFrames = lpcs->dwVideoChunkCount;
    aviHdr.dwInitialFrames = 0L;
    aviHdr.dwSuggestedBufferSize = 0L;
    aviHdr.dwWidth = lpBitsInfoOut->bmiHeader.biWidth;
    aviHdr.dwHeight = lpBitsInfoOut->bmiHeader.biHeight;

    aviHdr.dwReserved[0] = 0;
    aviHdr.dwReserved[1] = 0;
    aviHdr.dwReserved[2] = 0;
    aviHdr.dwReserved[3] = 0;
    //aviHdr.dwRate = 1000000L;
    //aviHdr.dwScale = aviHdr.dwMicroSecPerFrame;
    //aviHdr.dwStart = 0L;
    //aviHdr.dwLength = lpcs->dwVideoChunkCount;

    // Write AVI header info
    if (mmioWrite(lpcs->hmmio, (LPBYTE)&aviHdr, sizeof(aviHdr)) != sizeof(aviHdr) ||
        mmioAscend(lpcs->hmmio, &ck, 0))
        goto FileError;

    DSTATUS(lpcs, "Writing AVI Stream header");

    // Create stream header list
    ckStream.cksize = 0;
    ckStream.fccType = listtypeSTREAMHEADER;
    if (mmioCreateChunk(lpcs->hmmio,&ckStream,MMIO_CREATELIST))
        goto FileError;

    ZeroMemory (&strhdr, sizeof(strhdr));
    strhdr.fccType = streamtypeVIDEO;
    strhdr.fccHandler = lpBitsInfoOut->bmiHeader.biCompression;
   #ifdef NEW_COMPMAN
    if (lpcs->CompVars.hic)
        strhdr.fccHandler = lpcs->CompVars.fccHandler;
   #endif

    // A bit of history...
    // In VFW 1.0, we set fccHandler to 0 for BI_RLE8 formats
    // as a kludge to make Mplayer and Videdit play the files.
    // Just prior to 1.1 release, we found this broke Premiere,
    // so now (after AVICAP beta is on Compuserve), we change the
    // fccHandler to "MRLE".  Just ask Todd...
    // And now, at RC1, we change it again to "RLE ", Just ask Todd...
    if (strhdr.fccHandler == BI_RLE8)
        strhdr.fccHandler = mmioFOURCC('R', 'L', 'E', ' ');

    //strhdr.dwFlags = 0L;
   #ifdef NEW_COMPMAN
    //strhdr.wPriority = 0L;
    //strhdr.wLanguage = 0L;
   #else
    //strhdr.dwPriority = 0L;
   #endif

    //strhdr.dwInitialFrames = 0L;
    strhdr.dwScale = aviHdr.dwMicroSecPerFrame;
    strhdr.dwRate = 1000000L;
    //strhdr.dwStart = 0L;
    strhdr.dwLength = lpcs->dwVideoChunkCount;        /* Needs to get filled in! */
    strhdr.dwQuality = (DWORD) -1L;         /* !!! ICQUALITY_DEFAULT */
    //strhdr.dwSampleSize = 0L;

    //
    // Write stream header data
    //
    ck.ckid = ckidSTREAMHEADER;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0) ||
        mmioWrite(lpcs->hmmio, (LPBYTE)&strhdr, sizeof(strhdr)) != sizeof(strhdr) ||
        mmioAscend(lpcs->hmmio, &ck, 0))
        goto FileError;

    /*
    **  !!! dont write palette for full color?
    */
    if (lpBitsInfoOut->bmiHeader.biBitCount > 8)
        lpBitsInfoOut->bmiHeader.biClrUsed = 0;

    /* Create DIB header chunk */
    ck.cksize = lpBitsInfoOut->bmiHeader.biSize +
                           lpBitsInfoOut->bmiHeader.biClrUsed *
                           sizeof(RGBQUAD);
    ck.ckid = ckidSTREAMFORMAT;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0))
         goto FileError;

    /* Write DIB header data */
    if (mmioWrite(lpcs->hmmio, (LPBYTE)&lpBitsInfoOut->bmiHeader,
                               lpBitsInfoOut->bmiHeader.biSize) !=
             (LONG) lpBitsInfoOut->bmiHeader.biSize)
         goto FileError;

    if (lpBitsInfoOut->bmiHeader.biClrUsed > 0) {
        // Get Palette info
        if ((ii = GetPaletteEntries(lpcs->hPalCurrent, 0,
                                (UINT) lpBitsInfoOut->bmiHeader.biClrUsed,
                                (LPPALETTEENTRY) argbq)) !=
                    (UINT)lpBitsInfoOut->bmiHeader.biClrUsed)
            goto FileError;

	// Reorder the palette from PALETTEENTRY order to RGBQUAD order
	// by swapping the red and blue palette entries.
        //for (ii = 0; ii < lpBitsInfoOut->bmiHeader.biClrUsed; ++ii)
        while (ii--)
            SWAPTYPE(argbq[ii].rgbRed, argbq[ii].rgbBlue, BYTE);


        // Write Palette Info
        dw = sizeof(RGBQUAD) * lpBitsInfoOut->bmiHeader.biClrUsed;
        if (mmioWrite(lpcs->hmmio, (LPBYTE)argbq, dw) != (long)dw)
            goto FileError;
    }

    if (mmioAscend(lpcs->hmmio, &ck, 0))
         goto FileError;

    // ADD FOURCC stuff here!!! for Video stream

    // Ascend out of stream header
    if (mmioAscend(lpcs->hmmio, &ckStream, 0))
         goto FileError;

    /* If sound is enabled, then write WAVE header */
    if (fSound) {

         /* Create stream header list */
         ckStream.cksize = 0;
         ckStream.fccType = listtypeSTREAMHEADER;
         if (mmioCreateChunk(lpcs->hmmio,&ckStream,MMIO_CREATELIST))
             goto FileError;

         ZeroMemory (&strhdr, sizeof(strhdr));
         strhdr.fccType = streamtypeAUDIO;
         strhdr.fccHandler = 0L;
         strhdr.dwFlags = 0L;
        #ifdef NEW_COMPMAN
         strhdr.wPriority = 0L;
         strhdr.wLanguage = 0L;
        #else
         strhdr.dwPriority  = 0L;
        #endif
         strhdr.dwInitialFrames = 0L;
         strhdr.dwScale = lpcs->lpWaveFormat->nBlockAlign;
         strhdr.dwRate = lpcs->lpWaveFormat->nAvgBytesPerSec;
         strhdr.dwStart = 0L;
         strhdr.dwLength =  lpcs->dwWaveBytes /
                        lpcs->lpWaveFormat->nBlockAlign;
         strhdr.dwQuality = (DWORD)-1L;    /* !!! ICQUALITY_DEFAULT */
         strhdr.dwSampleSize = lpcs->lpWaveFormat->nBlockAlign;

         ck.ckid = ckidSTREAMHEADER;
         if (mmioCreateChunk(lpcs->hmmio,&ck,0) ||
             mmioWrite(lpcs->hmmio, (LPBYTE)&strhdr, sizeof(strhdr)) != sizeof(strhdr) ||
             mmioAscend(lpcs->hmmio, &ck, 0))
             goto FileError;

         ck.cksize = (LONG) GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat);
         ck.ckid = ckidSTREAMFORMAT;
         if (mmioCreateChunk(lpcs->hmmio,&ck,0) ||
             mmioWrite(lpcs->hmmio, (LPBYTE)lpcs->lpWaveFormat, ck.cksize) != (LONG) ck.cksize ||
             mmioAscend(lpcs->hmmio, &ck, 0))
             goto FileError;

         /* Ascend out of stream header */
         if (mmioAscend(lpcs->hmmio, &ckStream, 0))
             goto FileError;
    }

    // ADD FOURCC stuff here!!! for entire file
    DSTATUS(lpcs, "Writing Info chunks");
    if (lpcs->lpInfoChunks) {
        DSTATUS(lpcs, "Writing Info chunks");
        if (mmioWrite (lpcs->hmmio, lpcs->lpInfoChunks, lpcs->cbInfoChunks) !=
                lpcs->cbInfoChunks)
            goto FileError;
    }

    /* ascend from the Header list */
    if (mmioAscend(lpcs->hmmio, &ckList, 0))
         goto FileError;


    ck.ckid = ckidAVIPADDING;
    if (mmioCreateChunk(lpcs->hmmio,&ck,0))
         goto FileError;

    mmioSeek(lpcs->hmmio, lpcs->dwAVIHdrSize - 3 * sizeof(DWORD), SEEK_SET);

    if (mmioAscend(lpcs->hmmio, &ck, 0))
         goto FileError;

    DSTATUS(lpcs, "Writing Movie LIST");

    /* Start the movi list */
    ckList.cksize = 0;
    ckList.fccType = listtypeAVIMOVIE;
    if (mmioCreateChunk(lpcs->hmmio,&ckList,MMIO_CREATELIST))
         goto FileError;

    // Force the chunk to end on the next word boundary
    dprintf("IndexStartOffset = %8X\n", dwDataEnd);
    mmioSeek(lpcs->hmmio, dwDataEnd + (dwDataEnd & 1L), SEEK_SET);

    /* Ascend out of the movi list and the RIFF chunk so that */
    /* the sizes can be fixed */
    mmioAscend(lpcs->hmmio, &ckList, 0);

    /*
    ** Now write index out!
    */
    DSTATUS(lpcs, "Writing Index...");
    WriteIndex(lpcs, fWroteJunkChunks);

    lpcs->fFileCaptured = TRUE;     // we got a good file, allow editing of it
    goto Success;

FileError:
    lpcs->fFileCaptured = fRet = FALSE;      // bogus file - no editing allowed

Success:
    DSTATUS(lpcs, "Freeing Index...");
    FiniIndex (lpcs);
    mmioAscend(lpcs->hmmio, &ckRiff, 0);

    mmioSeek(lpcs->hmmio, 0, SEEK_END);

    mmioFlush(lpcs->hmmio, 0);

    // Close the file
    mmioClose(lpcs->hmmio, 0);
    lpcs->hmmio = NULL;

    return fRet;
}

//
// Prepends dummy frame entries to the current valid video frame.
// Bumps the index, but does not actually trigger a write operation.
// nCount is a count of the number of frames to write
// Returns: TRUE on a successful write

BOOL WINAPI AVIWriteDummyFrames (
    LPCAPSTREAM lpcs,
    UINT        nCount,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    DWORD  dwBytesToWrite;
    DWORD  dwType;
    LPRIFF priff;
    UINT   jj;

    *lpbPending = FALSE;
    *lpuError = 0;
    if ( ! nCount)
        return TRUE;

    // create a buffer full of dummy chunks to act as placeholders
    // for the dropped frames
    //
    dwType = MAKEAVICKID(cktypeDIBbits, 0);
    if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
        dwType = MAKEAVICKID(cktypeDIBcompressed, 0);

    // dont try to write more than 1 'sector' worth of dummy
    // frames
    //
    dwBytesToWrite = nCount * sizeof(RIFF);
    if (dwBytesToWrite > lpcs->dwBytesPerSector)
    {
#ifdef DEBUG
	UINT n = nCount;
#endif
        dwBytesToWrite = lpcs->dwBytesPerSector;
#ifdef DEBUG
        nCount = dwBytesToWrite / sizeof(RIFF);
	assert(nCount*sizeof(RIFF) == dwBytesToWrite);
	dprintf("Forced to reduce dummy frames from %d to %d", n, nCount);
#endif
    }

    // create index entries for the dummy chunks
    //
    for (jj = 0; jj < nCount-1; ++jj)
        IndexVideo (lpcs, IS_DUMMY_CHUNK, FALSE);
    IndexVideo (lpcs, IS_DUMMY_CHUNK | IS_GRANULAR_CHUNK, FALSE);

    // fill in the drop frame buffer with dummy frames
    //
    priff = (LPRIFF)lpcs->lpDropFrame;
    for (jj = 0; jj < nCount; ++jj, ++priff)
    {
        priff->dwSize  = 0;
        priff->dwType  = dwType;
    }

    //
    // cant use a single dummy frame buffer when we are doing async
    // write because we cant write 'n' dummy frames to the buffer
    // if it is currently already queued to an IO.
    //
    // perhaps several dummy frames?  1 frame, 2 frames, 3 frames, etc
    // create dynamically?
    //

    // write out the dummy frames
    //
    AuxDebugEx (3, DEBUGLINE "DummyFrames  Count=%d, ToWrite=%d\r\n",
                nCount, dwBytesToWrite);

    *lpuError = AVIWrite (lpcs,
                          lpcs->lpDropFrame,
                          dwBytesToWrite,
                          (UINT)-1,  // force sync completion
                          ASYNC_BUF_DROP,
                          lpbPending);
    return !(*lpuError);
}

// Writes compressed or uncompressed frames to the AVI file
// returns TRUE if no error, FALSE if end of file.
//
BOOL WINAPI AVIWriteVideoFrame (
    LPCAPSTREAM lpcs,
    LPBYTE      lpData,
    DWORD       dwBytesUsed,
    BOOL        fKeyFrame,
    UINT        uIndex,
    UINT        nDropped,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    DWORD  dwBytesToWrite;
    LPRIFF priff;

    *lpuError = 0;
    *lpbPending = FALSE;
    if (!IndexVideo (lpcs,
                dwBytesUsed | (nDropped ? 0 : IS_GRANULAR_CHUNK),
                fKeyFrame))
        return FALSE;

    // adjust the size field of the RIFF chunk that preceeds the
    // data to be written
    //
    priff = ((LPRIFF)lpData)-1;
    priff->dwSize = dwBytesUsed;
    dwBytesUsed += dwBytesUsed & 1;
    dwBytesToWrite = dwBytesUsed + sizeof(RIFF);

    if (nDropped)
    {
        UINT  jj;
        DWORD dwType;

        // determine the 'type' of the dummy chunks
        //
        //dwType = priff->dwType;
        dwType = MAKEAVICKID(cktypeDIBbits, 0);
        if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
            dwType = MAKEAVICKID(cktypeDIBcompressed, 0);

        // dont try to write more than 1 'sector' worth of dummy
        // frames
        //
        if (nDropped > (lpcs->dwBytesPerSector / sizeof(RIFF)))
            nDropped = lpcs->dwBytesPerSector / sizeof(RIFF);

        // create index entries for the dummy chunks
        //
        for (jj = 0; jj < nDropped-1; ++jj)
            IndexVideo (lpcs, IS_DUMMY_CHUNK, FALSE);

        IndexVideo (lpcs, IS_DUMMY_CHUNK | IS_GRANULAR_CHUNK, FALSE);

        // fill in the drop frame buffer with dummy frames
        //
        priff = (LPRIFF)(lpData + dwBytesToWrite - sizeof(RIFF));
        for (jj = 0; jj < nDropped; ++jj, ++priff)
        {
            priff->dwSize  = 0;
            priff->dwType  = dwType;
        }
        dwBytesToWrite += nDropped * sizeof(RIFF);
    }

    // AviWrite will write the data and create any trailing junk
    // that is necessary
    //

    // write out the chunk, video data, and possibly the junk chunk
    //
    AuxDebugEx (3, DEBUGLINE "Calling AVIWrite - Video=%8x dw=%8x\r\n",
                (LPBYTE)lpData - sizeof(RIFF), dwBytesToWrite);

    *lpuError = AVIWrite (lpcs,
                          (LPBYTE)lpData - sizeof(RIFF),
                          dwBytesToWrite,
                          uIndex,
                          ASYNC_BUF_VIDEO,
                          lpbPending);
    return !(*lpuError);
}

// New for Chicago, align audio buffers on wChunkGranularity boundaries!
//
BOOL WINAPI AVIWriteAudio (
    LPCAPSTREAM lpcs,
    LPWAVEHDR   lpwh,
    UINT        uIndex,
    LPUINT      lpuError,
    LPBOOL      lpbPending)
{
    DWORD  dwBytesToWrite;
    LPRIFF priff;

    *lpuError = 0;
    *lpbPending = FALSE;

    // change the dwSize field in the RIFF chunk
    priff = ((LPRIFF)lpwh->lpData) -1;
    priff->dwSize = lpwh->dwBytesRecorded;

    if ( ! IndexAudio (lpcs, lpwh->dwBytesRecorded | IS_GRANULAR_CHUNK))
        return FALSE;

    // update total bytes of wave audio recorded
    //
    lpcs->dwWaveBytes += lpwh->dwBytesRecorded;

    // pad the data to be written to a WORD (16 bit) boundary
    //
    lpwh->dwBytesRecorded += lpwh->dwBytesRecorded & 1;
    dwBytesToWrite = lpwh->dwBytesRecorded + sizeof(RIFF);

    // write out the chunk, audio data, and possibly the junk chunk
    AuxDebugEx (3, DEBUGLINE "Audio=%8x dw=%8x\r\n",
                lpwh->lpData - sizeof(RIFF), dwBytesToWrite);
    *lpuError = AVIWrite (lpcs,
                          lpwh->lpData - sizeof(RIFF),
                          dwBytesToWrite,
                          uIndex,
                          ASYNC_BUF_AUDIO,
                          lpbPending);
    return !(*lpuError);
}
    #endif  //---------------- USE_AVIFILE ----------------------------
