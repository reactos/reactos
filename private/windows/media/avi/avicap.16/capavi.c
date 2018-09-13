/****************************************************************************
 *
 *   capavi.c
 *
 *   Main video capture module.
 *
 *   Microsoft Video for Windows Sample Capture Class
 *
 *   Copyright (c) 1992, 1993 Microsoft Corporation.  All Rights Reserved.
 *
 *    You have a royalty-free right to use, modify, reproduce and
 *    distribute the Sample Files (and/or any modified version) in
 *    any way you find useful, provided that you agree that
 *    Microsoft has no warranty obligations or liability for any
 *    Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>         // for _fmemset
#include <msvideo.h>
#include <drawdib.h>
#include <mmreg.h>
#include <mmddk.h>
#include <msacm.h>
#include <avifmt.h>
#include "avicap.h"
#include "avicapi.h"
#include "time.h"

time_t      ltime;

extern void NEAR PASCAL MemCopy(LPVOID, LPVOID, DWORD); // in memcopy.asm
extern WORD FAR PASCAL SmartDrv(char chDrive, WORD w);
extern WORD GetSizeOfWaveFormat (LPWAVEFORMATEX lpwf);

/* dialog function prototype */
LONG FAR PASCAL _export capseqDlgProc(HWND hwnd, unsigned msg, WORD wParam, LONG lParam);

#ifdef _DEBUG
    #define DSTATUS(lpcs, sz) statusUpdateStatus(lpcs, IDS_CAP_INFO, (LPSTR) sz)
#else
    #define DSTATUS(lpcs, sz)
#endif

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

// The following are anded with the size in the index
#define IS_AUDIO_CHUNK        0x80000000
#define IS_KEYFRAME_CHUNK     0x40000000
#define IS_DUMMY_CHUNK        0x20000000
#define IS_LAST_DUMMY_CHUNK   0x10000000
#define INDEX_MASK  (IS_AUDIO_CHUNK | IS_KEYFRAME_CHUNK | IS_DUMMY_CHUNK | IS_LAST_DUMMY_CHUNK)


// Allocate the index table
// Returns: TRUE if index can be allocated
BOOL InitIndex (LPCAPSTREAM lpcs)
{
    lpcs->dwIndex = 0;

    WinAssert (lpcs->lpdwIndexStart == NULL);

    // Limit index size between 1 minute at 30fps and 3 hours at 30fps
    lpcs->sCapParms.dwIndexSize = max (lpcs->sCapParms.dwIndexSize, 1800);
    lpcs->sCapParms.dwIndexSize = min (lpcs->sCapParms.dwIndexSize, 324000L);
    dprintf("Max Index Size = %ld \n", lpcs->sCapParms.dwIndexSize);

    if (lpcs->hIndex = GlobalAlloc (GMEM_MOVEABLE,
                lpcs->sCapParms.dwIndexSize * sizeof (DWORD))) {
        if (lpcs->lpdwIndexEntry =
                lpcs->lpdwIndexStart =
                (DWORD _huge *)GlobalLock (lpcs->hIndex)) {
            GlobalPageLock (lpcs->hIndex);
            return TRUE;        // Success
        }
        GlobalFree (lpcs->hIndex);
    }
    lpcs->hIndex = NULL;
    lpcs->lpdwIndexStart = NULL;
    return FALSE;
}

// Deallocate the index table
void FiniIndex (LPCAPSTREAM lpcs)
{
    if (lpcs->hIndex) {
        GlobalPageUnlock (lpcs->hIndex);
        if (lpcs->lpdwIndexStart)
            GlobalUnlock (lpcs->hIndex);
        GlobalFree (lpcs->hIndex);
    }
    lpcs->hIndex = NULL;
    lpcs->lpdwIndexStart = NULL;
}


// Add an index entry for a video frame
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
BOOL IndexVideo (LPCAPSTREAM lpcs, DWORD dwSize, BOOL bKeyFrame)
{
    BOOL fOK = lpcs->dwIndex < lpcs->sCapParms.dwIndexSize;

    if (fOK) {
        *lpcs->lpdwIndexEntry++ = dwSize | (bKeyFrame ? IS_KEYFRAME_CHUNK : 0);
        lpcs->dwIndex++;
        lpcs->dwVideoChunkCount++;
    }
    return (fOK);
}


// Add an index entry for an audio buffer
// dwSize is the size of data ONLY, not including the chunk or junk
// Returns: TRUE if index space is not exhausted
BOOL IndexAudio (LPCAPSTREAM lpcs, DWORD dwSize)
{
    BOOL fOK = lpcs->dwIndex < lpcs->sCapParms.dwIndexSize;

    if (fOK) {
        *lpcs->lpdwIndexEntry++ = dwSize | IS_AUDIO_CHUNK;
        lpcs->dwIndex++;
        lpcs->dwWaveChunkCount++;
    }
    return (fOK);
}


// Write out the index at the end of the capture file.
// The single frame capture methods do not append
// JunkChunks!  Audio chunks do not have junk appended.
BOOL WriteIndex (LPCAPSTREAM lpcs, BOOL fJunkChunkWritten)
{
    BOOL  fChunkIsAudio;
    BOOL  fChunkIsKeyFrame;
    BOOL  fChunkIsDummy;
    BOOL  fChunkIsLastDummy;
    DWORD dwIndex;
    DWORD dw;
    DWORD dwDummySize;
    DWORD dwJunk;
    DWORD off;
    AVIINDEXENTRY   avii;
    MMCKINFO    ck;
    DWORD _huge *lpdw;

    if (lpcs->dwIndex > lpcs->sCapParms.dwIndexSize)
        return TRUE;

    off        = lpcs->dwAVIHdrSize;

    ck.cksize  = 0;
    ck.ckid    = ckidAVINEWINDEX;
    ck.fccType = 0;

    if (mmioCreateChunk(lpcs->hmmio,&ck,0))
        return FALSE;

    lpdw = lpcs->lpdwIndexStart;
    for (dwIndex= 0; dwIndex< lpcs->dwIndex; dwIndex++) {

        dw = *lpdw++;

        fChunkIsAudio      = (BOOL) ((dw & IS_AUDIO_CHUNK) != 0);
        fChunkIsKeyFrame   = (BOOL) ((dw & IS_KEYFRAME_CHUNK) != 0);
        fChunkIsDummy      = (BOOL) ((dw & IS_DUMMY_CHUNK) != 0);
        fChunkIsLastDummy  = (BOOL) ((dw & IS_LAST_DUMMY_CHUNK) != 0);
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

        if (mmioWrite(lpcs->hmmio, (LPVOID)&avii, sizeof(avii)) != sizeof(avii))
            return FALSE;

        dw += sizeof (RIFF);
        off += dw;

        // ooh, getting messy. We know that dummy chunks come in a group
        // (1 or more) and are always terminated by a IS_LAST_DUMMY_CHUNK flag.
        // only the last one gets junk append to round out to 2K
        if (fChunkIsDummy) {
            dwDummySize += sizeof(RIFF);
            if (!fChunkIsLastDummy)
                continue;
            else
                dw = dwDummySize;   // total size of all dummy entries in group
        }
        else
            dwDummySize = 0;

        if (fJunkChunkWritten & !fChunkIsAudio) {
           // If a Junk chunk was appended, move past it
           if (dw % lpcs->sCapParms.wChunkGranularity) {
                dwJunk = lpcs->sCapParms.wChunkGranularity - (dw % lpcs->sCapParms.wChunkGranularity);

                if (dwJunk < sizeof (RIFF))
                    off += lpcs->sCapParms.wChunkGranularity + dwJunk;
                else
                    off += dwJunk;
           }
        }

         if (off & 1)
             off++;
    }

    if (mmioAscend(lpcs->hmmio, &ck, 0))
        return FALSE;

    return TRUE;
}



// Allocate DOS memory for faster disk writes
LPVOID NEAR PASCAL AllocDosMem (DWORD dw)
{
    HANDLE h;

    if (h = LOWORD (GlobalDosAlloc(dw)))
        return (GlobalLock (h));
    return NULL;
}

// General purpose memory allocator
LPVOID NEAR PASCAL AllocMem (DWORD dw, BOOL fUseDOSMemory)
{
#if 0
    if (fUseDOSMemory)
        return AllocDosMem(dw);
#endif

    return GlobalAllocPtr (GMEM_MOVEABLE, dw);
}

void NEAR PASCAL FreeMem(LPVOID p)
{
    GlobalFreePtr(p);
}


#pragma optimize ("", off)

DWORD GetFreePhysicalMemory(void)
{
    DWORD   adw[ 0x30 / sizeof(DWORD) ];
    WORD    fFail;

    //
    //  if standard mode just ask KERNEL how much memory is free
    //
    //  if enhanced mode, call DPMI and find out how much *real*
    //  memory is free.
    //
    if (GetWinFlags() & WF_STANDARD)
    {
        return GetFreeSpace(0);
    }
    else _asm
    {
        mov     ax, 0500h
        push    ss
        pop     es
        lea     di, word ptr adw
        int     31h
        sbb     ax, ax
        mov     fFail, ax
    }

    if (fFail)
        return (0l);

    return (adw[2] * 4096);
}
#pragma optimize ("", on)

/*
 *  CalcWaveBufferSize   - Figure out how large to make the wave buffers
 *    a. At least .5 seconds
 *    b. But not less than 10K, (else capture frmae rate suffers)
 *    c. A multiple of lpcs->sCapParms.wChunkGranularity
 */
DWORD CalcWaveBufferSize (LPCAPSTREAM lpcs)
{
    DWORD dw;

    if (!lpcs-> lpWaveFormat)
        return 0L;

    // at least .5 second
    dw = (DWORD) lpcs->lpWaveFormat->nChannels *
         (DWORD) lpcs->lpWaveFormat->nSamplesPerSec *
         (lpcs->lpWaveFormat->wBitsPerSample / 8) / 2L;
    dw -= dw % lpcs->sCapParms.wChunkGranularity;
    dw = max ((1024L * 10), dw);                // at least 10K

//    dprintf("Wave buffer size = %ld \n", dw);
    return dw;
}

static BOOL IsWaveInDeviceMapped(HWAVEIN hWaveIn)
{
    DWORD err;
    DWORD dw;

    err = waveInMessage(hWaveIn,
        WIDM_MAPPER_STATUS,
        WAVEIN_MAPPER_STATUS_MAPPED,
        (DWORD)(LPVOID)&dw);

    return err == 0 && dw != 0;
}

// ****************************************************************
// ******************** Capture File Routines *********************
// ****************************************************************


/*
 * AVIFileInit
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
BOOL AVIFileInit(LPCAPSTREAM lpcs)
{
#define TEMP_BUFF_SIZE  128
    LONG l;
    char ach[TEMP_BUFF_SIZE];
    LPBITMAPINFO lpBitsInfoOut;    // Possibly compressed output format

    /* No special video format given -- use the default */
    if (lpcs->CompVars.hic == NULL)
	lpBitsInfoOut = lpcs->lpBitsInfo;
    else
	lpBitsInfoOut = lpcs->CompVars.lpbiOut;

    WinAssert (lpcs->hmmio == NULL);   // Should never have a file handle on entry

    /* if the capture file has not been set then set it now */
    if (!(*lpcs->achFile)){
//       if (!fileSetCapFile())
             goto INIT_FILE_OPEN_ERROR;
    }

    /* we have a capture file, open it and set it up */
    lpcs->hmmio = mmioOpen(lpcs->achFile, NULL, MMIO_WRITE);
    if (!lpcs->hmmio) {
         /* try and create */
         lpcs->hmmio = mmioOpen(lpcs->achFile, NULL, MMIO_CREATE | MMIO_WRITE);
         if (!lpcs->hmmio) {
             goto INIT_FILE_OPEN_ERROR;
         }
    }

    /* pre-read the file */
    l = mmioSeek( lpcs->hmmio, 0L, SEEK_END );
    while( l > 0 ) {
         l = mmioSeek( lpcs->hmmio, -min(l, 50000L), SEEK_CUR );
        mmioRead( lpcs->hmmio, ach, sizeof(ach) );
    }

    /* Seek to 2K (or multiple of 2K), where we're going to write our data.
    ** later, we'll come back and fill in the file.
    */

    // l is zero for standard wave and video formats
    l = (GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat) -
                sizeof (PCMWAVEFORMAT)) +
                (lpBitsInfoOut->bmiHeader.biSize -
                sizeof (BITMAPINFOHEADER));

    // (2K + size of wave and video stream headers) rounded to next 2K
    lpcs->dwAVIHdrSize = AVI_HEADERSIZE +
        (((lpcs->cbInfoChunks + l + lpcs->sCapParms.wChunkGranularity - 1)
        / lpcs->sCapParms.wChunkGranularity) * lpcs->sCapParms.wChunkGranularity);


    dprintf("AVIHdrSize = %ld \n", lpcs->dwAVIHdrSize);
    mmioSeek(lpcs->hmmio, lpcs->dwAVIHdrSize, SEEK_SET);

    if (!InitIndex (lpcs))           // do all Index allocations
        mmioClose (lpcs->hmmio, 0);

    lpcs->dwVideoChunkCount = 0;
    lpcs->dwWaveChunkCount = 0;

INIT_FILE_OPEN_ERROR:
    return (lpcs->hmmio != NULL);
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
    int           i;
    DWORD         dw;
    AVIStreamHeader        strhdr;
    DWORD         dwDataEnd;
    BOOL        fRet = TRUE;
    RGBQUAD     argbq[256];
    MainAVIHeader   aviHdr;
    BOOL        fSound = lpcs->sCapParms.fCaptureAudio;
    LPBITMAPINFO lpBitsInfoOut;    // Possibly compressed output format

    /* No special video format given -- use the default */
    if (lpcs->CompVars.hic == NULL)
	lpBitsInfoOut = lpcs->lpBitsInfo;
    else
	lpBitsInfoOut = lpcs->CompVars.lpbiOut;

    if (lpcs->hmmio == NULL)  // This can be called even though never opened
        return FALSE;

    if (fAbort)
        goto FileError;

    if (!lpcs->dwWaveBytes)
        fSound = FALSE;

    dwDataEnd = mmioSeek(lpcs->hmmio, 0, SEEK_CUR);

    /* Seek to beginning of file, so we can write the header. */
    mmioSeek(lpcs->hmmio, 0, SEEK_SET);

    DSTATUS(lpcs, "Writing AVI header");

    /* Create RIFF chunk */
    ckRiff.cksize = 0;
    ckRiff.fccType = formtypeAVI;
    if(mmioCreateChunk(lpcs->hmmio,&ckRiff,MMIO_CREATERIFF)) {
         goto FileError;
    }

    /* Create header list */
    ckList.cksize = 0;
    ckList.fccType = listtypeAVIHEADER;
    if(mmioCreateChunk(lpcs->hmmio,&ckList,MMIO_CREATELIST)) {
         goto FileError;
    }

    /* Create AVI header chunk */
    ck.cksize = sizeof(MainAVIHeader);
    ck.ckid = ckidAVIMAINHDR;
    if(mmioCreateChunk(lpcs->hmmio,&ck,0)) {
         goto FileError;
    }

    lpcs->dwAVIHdrPos = ck.dwDataOffset;

    /* Calculate AVI header info */
    _fmemset(&aviHdr, 0, sizeof(aviHdr));
    if (fSound && lpcs->dwVideoChunkCount) {
         /* HACK HACK */
         /* Set rate that was captured based on length of audio data */

         aviHdr.dwMicroSecPerFrame = (DWORD)
                ((double)lpcs->dwWaveBytes * 1000000. /
                ((double)lpcs->lpWaveFormat->nAvgBytesPerSec *
                lpcs->dwVideoChunkCount + 0.5));
    } else {
         aviHdr.dwMicroSecPerFrame = lpcs->sCapParms.dwRequestMicroSecPerFrame;
    }
    lpcs->dwActualMicroSecPerFrame = aviHdr.dwMicroSecPerFrame;

    aviHdr.dwMaxBytesPerSec = (DWORD) muldiv32 (lpBitsInfoOut->bmiHeader.biSizeImage,
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

//  The following were set for all versions before Chicago Beta2
//  They are now listed as reserved...
//    aviHdr.dwRate = 1000000L;
//    aviHdr.dwScale = aviHdr.dwMicroSecPerFrame;
//    aviHdr.dwStart = 0L;
//    aviHdr.dwLength = lpcs->dwVideoChunkCount;

    /* Write AVI header info */
    if(mmioWrite(lpcs->hmmio, (LPSTR)&aviHdr, sizeof(aviHdr)) !=
             sizeof(aviHdr)) {
         goto FileError;
    }

    if(mmioAscend(lpcs->hmmio, &ck, 0)) {
         goto FileError;
    }

    DSTATUS(lpcs, "Writing AVI Stream header");

    /* Create stream header list */
    ckStream.cksize = 0;
    ckStream.fccType = listtypeSTREAMHEADER;
    if(mmioCreateChunk(lpcs->hmmio,&ckStream,MMIO_CREATELIST)) {
         goto FileError;
    }

    _fmemset(&strhdr, 0, sizeof(strhdr));
    strhdr.fccType = streamtypeVIDEO;
    if (lpcs->CompVars.hic)
        strhdr.fccHandler = lpcs->CompVars.fccHandler;
    else
        strhdr.fccHandler = lpBitsInfoOut->bmiHeader.biCompression;

    // A bit of history...
    // In VFW 1.0, we set fccHandler to 0 for BI_RLE8 formats
    // as a kludge to make Mplayer and Videdit play the files.
    // Just prior to 1.1 release, we found this broke Premiere,
    // so now (after AVICAP beta is on Compuserve), we change the
    // fccHandler to "MRLE".  Just ask Todd...
    // And now, at RC1, we change it again to "RLE ", Just ask Todd...
    if (strhdr.fccHandler == BI_RLE8)
        strhdr.fccHandler = mmioFOURCC('R', 'L', 'E', ' ');

    strhdr.dwFlags = 0L;
    strhdr.wPriority = 0L;
    strhdr.wLanguage = 0L;
    strhdr.dwInitialFrames = 0L;
    strhdr.dwScale = aviHdr.dwMicroSecPerFrame;
    strhdr.dwRate = 1000000L;
    strhdr.dwStart = 0L;
    strhdr.dwLength = lpcs->dwVideoChunkCount;        /* Needs to get filled in! */
    strhdr.dwQuality = (DWORD) -1L;         /* !!! ICQUALITY_DEFAULT */
    strhdr.dwSampleSize = 0L;

    ck.ckid = ckidSTREAMHEADER;
    if(mmioCreateChunk(lpcs->hmmio,&ck,0)) {
         goto FileError;
    }

    /* Write stream header data */
    if(mmioWrite(lpcs->hmmio, (LPSTR)&strhdr, sizeof(strhdr)) != sizeof(strhdr)) {
         goto FileError;
    }

    if(mmioAscend(lpcs->hmmio, &ck, 0)) {
         goto FileError;
    }

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
    if(mmioCreateChunk(lpcs->hmmio,&ck,0)) {
         goto FileError;
    }

    /* Write DIB header data */
    if(mmioWrite(lpcs->hmmio, (LPSTR)&lpBitsInfoOut->bmiHeader,
                               lpBitsInfoOut->bmiHeader.biSize) !=
             (LONG) lpBitsInfoOut->bmiHeader.biSize) {
         goto FileError;
    }

    if (lpBitsInfoOut->bmiHeader.biClrUsed > 0) {
        /* Get Palette info */
        if(GetPaletteEntries(lpcs->hPalCurrent, 0,
                                (WORD) lpBitsInfoOut->bmiHeader.biClrUsed,
                                (LPPALETTEENTRY) argbq) !=
                    (WORD)lpBitsInfoOut->bmiHeader.biClrUsed) {
            goto FileError;
        }

        for(i = 0; i < (int) lpBitsInfoOut->bmiHeader.biClrUsed; i++)
            SWAP(argbq[i].rgbRed, argbq[i].rgbBlue);

        /* Write Palette Info */
        dw = sizeof(RGBQUAD) * lpBitsInfoOut->bmiHeader.biClrUsed;
        if (mmioWrite(lpcs->hmmio, (LPSTR)argbq, dw) != (long)dw) {
            goto FileError;
        }
    }

    if(mmioAscend(lpcs->hmmio, &ck, 0)) {
         goto FileError;
    }

    // ADD FOURCC stuff here!!! for Video stream

    /* Ascend out of stream header */
    if(mmioAscend(lpcs->hmmio, &ckStream, 0)) {
         goto FileError;
    }

    /* If sound is enabled, then write WAVE header */
    if(fSound) {

         /* Create stream header list */
         ckStream.cksize = 0;
         ckStream.fccType = listtypeSTREAMHEADER;
         if(mmioCreateChunk(lpcs->hmmio,&ckStream,MMIO_CREATELIST)) {
             goto FileError;
         }

         _fmemset(&strhdr, 0, sizeof(strhdr));
         strhdr.fccType = streamtypeAUDIO;
         strhdr.fccHandler = 0L;
         strhdr.dwFlags = 0L;
         strhdr.wPriority = 0L;
         strhdr.wLanguage = 0L;
         strhdr.dwInitialFrames = 0L;
         strhdr.dwScale = lpcs->lpWaveFormat->nBlockAlign;
         strhdr.dwRate = lpcs->lpWaveFormat->nAvgBytesPerSec;
         strhdr.dwStart = 0L;
         strhdr.dwLength =  lpcs->dwWaveBytes /
                        lpcs->lpWaveFormat->nBlockAlign;
         strhdr.dwQuality = (DWORD)-1L;    /* !!! ICQUALITY_DEFAULT */
         strhdr.dwSampleSize = lpcs->lpWaveFormat->nBlockAlign;

         ck.ckid = ckidSTREAMHEADER;
         if(mmioCreateChunk(lpcs->hmmio,&ck,0)) {
             goto FileError;
         }

         if(mmioWrite(lpcs->hmmio, (LPSTR)&strhdr, sizeof(strhdr)) != sizeof(strhdr)) {
             goto FileError;
         }

         if(mmioAscend(lpcs->hmmio, &ck, 0)) {
             goto FileError;
         }

         ck.cksize = (LONG) GetSizeOfWaveFormat ((LPWAVEFORMATEX) lpcs->lpWaveFormat);
         ck.ckid = ckidSTREAMFORMAT;
         if(mmioCreateChunk(lpcs->hmmio,&ck,0)) {
             goto FileError;
         }

         /* Write WAVE header info */
         if(mmioWrite(lpcs->hmmio, (LPSTR)lpcs->lpWaveFormat, ck.cksize) != (LONG) ck.cksize) {
             goto FileError;
         }

         if(mmioAscend(lpcs->hmmio, &ck, 0)) {
             goto FileError;
         }

         /* Ascend out of stream header */
         if(mmioAscend(lpcs->hmmio, &ckStream, 0)) {
             goto FileError;
         }

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
    if(mmioAscend(lpcs->hmmio, &ckList, 0)) {
         goto FileError;
    }


    ck.ckid = ckidAVIPADDING;
    if(mmioCreateChunk(lpcs->hmmio,&ck,0)) {
         goto FileError;
    }

    mmioSeek(lpcs->hmmio, lpcs->dwAVIHdrSize - 3 * sizeof(DWORD), SEEK_SET);

    if(mmioAscend(lpcs->hmmio, &ck, 0)) {
         goto FileError;
    }

    DSTATUS(lpcs, "Writing Movie LIST");

    /* Start the movi list */
    ckList.cksize = 0;
    ckList.fccType = listtypeAVIMOVIE;
    if(mmioCreateChunk(lpcs->hmmio,&ckList,MMIO_CREATELIST)) {
         goto FileError;
    }

    // Force the chunk to end on the next word boundary
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

    /* Close the file */
    mmioClose(lpcs->hmmio, 0);
    lpcs->hmmio = NULL;

    return fRet;
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

WORD AVIAudioInit (LPCAPSTREAM lpcs)
{
    int         i;
    LPVOID        p;

    if (lpcs->sCapParms.wNumAudioRequested == 0)
        lpcs->sCapParms.wNumAudioRequested = DEF_WAVE_BUFFERS;

    // Alloc the wave memory
    for(i = 0; i < (int)lpcs->sCapParms.wNumAudioRequested; i++) {

        p = AllocMem(sizeof(WAVEHDR) + lpcs->dwWaveSize, FALSE /* DOSMem */);

        if (p == NULL)
            break;

        lpcs->alpWaveHdr[i] = p;
        lpcs->alpWaveHdr[i]->lpData          = (LPBYTE)p
                                               + sizeof(WAVEHDR) + sizeof(RIFF);
        lpcs->alpWaveHdr[i]->dwBufferLength  = lpcs->dwWaveSize - sizeof(RIFF);
        lpcs->alpWaveHdr[i]->dwBytesRecorded = 0;
        lpcs->alpWaveHdr[i]->dwUser          = 0;
        lpcs->alpWaveHdr[i]->dwFlags         = 0;
        lpcs->alpWaveHdr[i]->dwLoops         = 0;

        /* Set Chunk ID, Size in buffer */
        p = (LPBYTE)p + sizeof(WAVEHDR);

        ((LPRIFF)p)->dwType = MAKEAVICKID(cktypeWAVEbytes, 1);
        ((LPRIFF)p)->dwSize = lpcs->dwWaveSize - sizeof(RIFF);
    }

    lpcs->iNumAudio = i;

    return ((lpcs->iNumAudio == 0) ? IDS_CAP_WAVE_ALLOC_ERROR : 0);
}


//
// AVI AudioFini    - UnPrepares headers and resets the wave device.
//                      This routine is also used by MCI capture.
//                      Returns: 0 on success, otherwise an error code.

WORD AVIAudioFini (LPCAPSTREAM lpcs)
{
    int i;

    /* free headers and data */
    for(i=0; i < MAX_WAVE_BUFFERS; i++) {
        if (lpcs->alpWaveHdr[i]) {
            FreeMem(lpcs->alpWaveHdr[i]);
            lpcs->alpWaveHdr[i] = NULL;
        }
    }

    return 0;
}


//
// AVI AudioPrepare - Opens the wave device and adds the buffers
//                    Prepares headers and adds buffers to the device
//                    This routine is also used by MCI capture.
//                    Returns: 0 on success, otherwise an error code.

WORD AVIAudioPrepare (LPCAPSTREAM lpcs, HWND hWndCallback)
{
    UINT        uiError;
    int i;

    /* See if we can open that format for input */

    uiError = waveInOpen((LPHWAVEIN)&lpcs->hWaveIn,
        (UINT)WAVE_MAPPER, lpcs->lpWaveFormat,
        (DWORD) hWndCallback, 0L,
        (hWndCallback ? CALLBACK_WINDOW : 0L));

    if (uiError != MMSYSERR_NOERROR)
        return IDS_CAP_WAVE_OPEN_ERROR;

    lpcs->fAudioYield = IsWaveInDeviceMapped(lpcs->hWaveIn);
    lpcs->fAudioBreak = FALSE;
    DPF("AVICap:    AudioYield = %d \n", lpcs->fAudioYield);

    for(i = 0; i < (int)lpcs->sCapParms.wNumAudioRequested; i++) {
        if (waveInPrepareHeader(lpcs->hWaveIn, lpcs->alpWaveHdr[i],
                sizeof(WAVEHDR)))
            return IDS_CAP_WAVE_ALLOC_ERROR;

        if (waveInAddBuffer(lpcs->hWaveIn, lpcs->alpWaveHdr[i],
                sizeof(WAVEHDR)))
            return IDS_CAP_WAVE_ALLOC_ERROR;
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

WORD AVIAudioUnPrepare (LPCAPSTREAM lpcs)
{
    int i;

    if (lpcs->hWaveIn) {
        waveInReset(lpcs->hWaveIn);

        /* unprepare headers by unlocking them */
        for(i=0; i < lpcs->iNumAudio; i++) {
            if (lpcs->alpWaveHdr[i]) {
                if (lpcs->alpWaveHdr[i]->dwFlags & WHDR_PREPARED)
                    waveInUnprepareHeader(lpcs->hWaveIn, lpcs->alpWaveHdr[i],
                                sizeof(WAVEHDR));
            }
        }

        waveInClose(lpcs->hWaveIn);
        lpcs->hWaveIn = NULL;
    }
    return 0;
}

// ****************************************************************
// ******************** Video Buffer Control **********************
// ****************************************************************

// AVIVideoInit -  Allocates, and initialize buffers for video capture.
//                 This routine is also used by MCI capture.
//                 Returns: 0 on success, otherwise an error code.

WORD AVIVideoInit (LPCAPSTREAM lpcs)
{
    int         iMaxVideo;
    DWORD       dwFreeMem;
    DWORD       dwUserRequests;
    DWORD       dwAudioMem;
    int         i;
    LPVOID      p;

    lpcs->iNextVideo = 0;
    lpcs->dwVideoChunkCount = 0;
    lpcs->dwFramesDropped = 0;

    // When performing MCI step capture, buffer array is not used
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

    // If the user hasn't specified the number of video buffers to use,
    // assume the minimum

    if (lpcs->sCapParms.wNumVideoRequested == 0)
        lpcs->sCapParms.wNumVideoRequested = MIN_VIDEO_BUFFERS;

    iMaxVideo = min (MAX_VIDEO_BUFFERS, lpcs->sCapParms.wNumVideoRequested);

    // Post VFW 1.1a, see if the driver can allocate memory
    if (videoStreamAllocHdrAndBuffer (lpcs->hVideoIn,
                (LPVIDEOHDR FAR *) &p, (DWORD) sizeof(VIDEOHDR) + lpcs->dwVideoSize)
                        == DV_ERR_OK) {
        lpcs-> fBuffersOnHardware = TRUE;
        videoStreamFreeHdrAndBuffer (lpcs->hVideoIn, (LPVIDEOHDR) p);
    }
    else {
        lpcs-> fBuffersOnHardware = FALSE;

        // How much actual free physical memory exists?
        dwFreeMem = GetFreePhysicalMemory();

        dwAudioMem = lpcs->dwWaveSize * lpcs->sCapParms.wNumAudioRequested;

#define FOREVER_FREE 32768L   // Always keep this free for swap space

        // How much memory will be used if we allocate per the request?
        dwUserRequests = dwAudioMem +
                     lpcs->dwVideoSize * iMaxVideo +
                     FOREVER_FREE;

        // If request is greater than available memory, force fewer buffers
        if (dwUserRequests > dwFreeMem) {
            if (dwFreeMem > dwAudioMem)
                dwFreeMem -= dwAudioMem;
            iMaxVideo = (int)(((dwFreeMem * 8) / 10) / lpcs->dwVideoSize);
            iMaxVideo = min (MAX_VIDEO_BUFFERS, iMaxVideo);
            dprintf("iMaxVideo = %d\n", iMaxVideo);
        }
    } // endif not allocating buffers from hardware

    // Set up the buffers presuming fixed size DIBs and Junk chunks
    // These will be modified later if the device provides compressed data

    for (i=0; i < iMaxVideo; i++) {

        if (lpcs-> fBuffersOnHardware)
            videoStreamAllocHdrAndBuffer (lpcs->hVideoIn,
                (LPVIDEOHDR FAR *) &p, sizeof(VIDEOHDR) + lpcs->dwVideoSize);
        else
            p = AllocMem(sizeof(VIDEOHDR) + lpcs->dwVideoSize, lpcs->sCapParms.fUsingDOSMemory /* DOSMem */);

        if (p == NULL)
            break;

        lpcs->alpVideoHdr[i] = p;
        lpcs->alpVideoHdr[i]->lpData          = (LPBYTE)p + sizeof(VIDEOHDR) + sizeof(RIFF);
        lpcs->alpVideoHdr[i]->dwBufferLength  = lpcs->lpBitsInfo->bmiHeader.biSizeImage;
        lpcs->alpVideoHdr[i]->dwBytesUsed     = 0;
        lpcs->alpVideoHdr[i]->dwTimeCaptured  = 0;
        lpcs->alpVideoHdr[i]->dwUser          = 0;
        // Buffers on hardware are marked prepared during allocation!
        if (!lpcs-> fBuffersOnHardware)
            lpcs->alpVideoHdr[i]->dwFlags     = 0;

        p = (LPBYTE)p + sizeof(VIDEOHDR);

        if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
             ((LPRIFF)p)->dwType = MAKEAVICKID(cktypeDIBcompressed, 0);
        else
             ((LPRIFF)p)->dwType = MAKEAVICKID(cktypeDIBbits, 0);
         ((LPRIFF)p)->dwSize = lpcs->lpBitsInfo->bmiHeader.biSizeImage;

         if(lpcs->dwVideoJunkSize) {
             p = ((BYTE huge *)p) + ((LPRIFF)p)->dwSize + sizeof(RIFF);

             ((LPRIFF)p)->dwType = ckidAVIPADDING;;
             ((LPRIFF)p)->dwSize = lpcs->dwVideoJunkSize;
         }
    }

    lpcs->iNumVideo = i;

    if (lpcs-> fBuffersOnHardware)
        dprintf("HARDWARE iNumVideo Allocated = %d \n", lpcs->iNumVideo);
    else if (lpcs->sCapParms.fUsingDOSMemory)
        dprintf("DOS iNumVideo Allocated = %d \n", lpcs->iNumVideo);
    else
        dprintf("HIGH iNumVideo Allocated = %d \n", lpcs->iNumVideo);

    return ((lpcs->iNumVideo == 0) ? IDS_CAP_VIDEO_ALLOC_ERROR : 0);
}

//
// AVIVideoPrepare -  Prepares headers and adds buffers to the device
//                    This routine is also used by MCI capture.
//                    Returns: 0 on success, otherwise an error code.
WORD AVIVideoPrepare (LPCAPSTREAM lpcs)
{
    int i;

    // When performing MCI step capture, buffer array is not used
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

    // Open the video stream, setting the capture rate
    if (videoStreamInit(lpcs->hVideoIn,
                lpcs->sCapParms.dwRequestMicroSecPerFrame,
                0L, 0L, 0L )) {
        dprintf("cant open video device!\n");
        return IDS_CAP_VIDEO_OPEN_ERROR;
    }

    // Prepare (lock) the buffers, and give them to the device
    for (i=0; i < lpcs->iNumVideo; i++) {
        // If the buffers are on the hardware, don't Prepare them
        if (!lpcs-> fBuffersOnHardware) {
            if (videoStreamPrepareHeader (lpcs->hVideoIn,
                        lpcs->alpVideoHdr[i], sizeof(VIDEOHDR))) {
                lpcs->iNumVideo = i;
                dprintf("**** could only prepare %d Video!\n", lpcs->iNumVideo);
                break;
            }
        }

        if (videoStreamAddBuffer(lpcs->hVideoIn, lpcs->alpVideoHdr[i], sizeof(VIDEOHDR)))
             return IDS_CAP_VIDEO_ALLOC_ERROR;
    }
    return 0;
}

//
// AVI VideoUnPrepare - UnPrepares headers, frees memory, and
//                      resets the video in device.
//                      This routine is also used by MCI capture.
//                      Returns: 0 on success, otherwise an error code.

WORD AVIVideoUnPrepare (LPCAPSTREAM lpcs)
{
    int i;

    // When performing MCI step capture, buffer array is not used
    if (lpcs->sCapParms.fStepMCIDevice)
        return 0;

    /* Reset the buffers so they can be freed */
    if (lpcs->hVideoIn) {
        videoStreamReset(lpcs->hVideoIn);

        /* unprepare headers */
        /* Unlock and free headers and data */

        for(i = 0; i < MAX_VIDEO_BUFFERS; i++) {
            if (lpcs->alpVideoHdr[i]) {
                if (!lpcs-> fBuffersOnHardware) {
                    if (lpcs->alpVideoHdr[i]->dwFlags & VHDR_PREPARED)
                        videoStreamUnprepareHeader(lpcs->hVideoIn,
                            lpcs->alpVideoHdr[i],sizeof(VIDEOHDR));

                    FreeMem(lpcs->alpVideoHdr[i]);
                }
                else
                    videoStreamFreeHdrAndBuffer(lpcs->hVideoIn, lpcs->alpVideoHdr[i]);
                lpcs->alpVideoHdr[i] = NULL;
            }
        }
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
    if (lpcs->lpDOSWriteBuffer) {
        FreeMem(lpcs->lpDOSWriteBuffer);
        lpcs->lpDOSWriteBuffer = NULL;
    }

    AVIVideoUnPrepare (lpcs);           // Free the video device and buffers

    AVIAudioUnPrepare (lpcs);           // Free the audio device
    AVIAudioFini (lpcs);                // Free the audio buffers
}

//
// AVI Init
//     This routine does all the non-File initalization for AVICapture.
//     Returns: 0 on success, Error string value on failure.
//

WORD AVIInit (LPCAPSTREAM lpcs)
{
    WORD         wError = 0;    // Success
    int          i;
    LPBITMAPINFO lpBitsInfoOut;    // Possibly compressed output format

    /* No special video format given -- use the default */
    if (lpcs->CompVars.hic == NULL)
	lpBitsInfoOut = lpcs->lpBitsInfo;
    else
	lpBitsInfoOut = lpcs->CompVars.lpbiOut;

    // -------------------------------------------------------
    // figure out buffer sizes
    // -------------------------------------------------------

    // Init all pointers to NULL
    for(i = 0; i < MAX_VIDEO_BUFFERS; i++)
        lpcs->alpVideoHdr[i] = NULL;

    for(i = 0; i < MAX_WAVE_BUFFERS; i++)
        lpcs->alpWaveHdr[i] = NULL;

    // .5 second of audio per buffer (or 10K, whichever is larger)
    if (lpcs->sCapParms.dwAudioBufferSize == 0)
        lpcs->dwWaveSize =  CalcWaveBufferSize (lpcs);
    else {
        if (!lpcs-> lpWaveFormat)
            lpcs->dwWaveSize = 0;
        else
            lpcs->dwWaveSize = lpcs->sCapParms.dwAudioBufferSize;
    }
    /* Set video buffer size to Image size
        (normally dx * dy * (depth / 8)) + sizeof(RIFF) */
    lpcs->dwVideoSize = lpcs->lpBitsInfo->bmiHeader.biSizeImage + sizeof(RIFF);
    lpcs->fVideoDataIsCompressed = (lpBitsInfoOut->bmiHeader.biCompression
                != BI_RGB);

    /* Pad out to multiple of lpcs->sCapParms.wChunkGranularity (2K) size */
    // Calc dwVideoJunkSize

    if (lpcs->dwVideoJunkSize = lpcs->sCapParms.wChunkGranularity - (lpcs->dwVideoSize % lpcs->sCapParms.wChunkGranularity)) {
         if (lpcs->dwVideoJunkSize < sizeof(RIFF))
             lpcs->dwVideoJunkSize += lpcs->sCapParms.wChunkGranularity;

         lpcs->dwVideoSize += lpcs->dwVideoJunkSize;

         lpcs->dwVideoJunkSize -= sizeof(RIFF);
    } else {
         lpcs->dwVideoJunkSize = 0L;
    }

    // -------------------------------------------------------
    //                    DOS copy buffer
    // -------------------------------------------------------

    lpcs->dwDOSBufferSize = max (lpcs->dwWaveSize, lpcs->dwVideoSize);

#if 0
    // Only get a DOS copy buffer if we're not trying to get DOS video buffers
    if (!lpcs->sCapParms.fUsingDOSMemory) {
        lpcs->lpDOSWriteBuffer = AllocDosMem(lpcs->dwDOSBufferSize);

        if (lpcs->lpDOSWriteBuffer) {
            dprintf("Allocated DOS write buffer (%ld bytes).\n", lpcs->dwDOSBufferSize);
        } else {
            dprintf("Unable to allocate DOS write buffer.\n");
        }
    }
#endif

    // -------------------------------------------------------
    //                    Init Sound
    // -------------------------------------------------------

    if (lpcs->sCapParms.fCaptureAudio) {
        if (wError = AVIAudioInit (lpcs)) {
            dprintf("can't init audio buffers!\n");
            goto AVIInitFailed;
        }
    }

    // -------------------------------------------------------
    //                    Init Video
    // -------------------------------------------------------

    if (wError = AVIVideoInit (lpcs)) {
        dprintf("AVIVideoInitFailed (no buffers alloc'd)!\n");
        goto AVIInitFailed;
    }

    // --------------------------------------------------------------
    //  Prepare audio buffers (lock em down) and give them to the device
    // --------------------------------------------------------------

    if (lpcs->sCapParms.fCaptureAudio) {
        if (wError = AVIAudioPrepare (lpcs, NULL)) {
            dprintf("can't prepare audio buffers!\n");
            goto AVIInitFailed;
        }
    }

    // --------------------------------------------------------------
    //  Prepare video buffers (lock em down) and give them to the device
    // --------------------------------------------------------------

    if (wError = AVIVideoPrepare (lpcs)) {
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

// Write data to the capture file
// Returns: TRUE on a successful write
BOOL NEAR PASCAL AVIWrite(LPCAPSTREAM lpcs, LPVOID p, DWORD dwSize)
{
    if (lpcs->lpDOSWriteBuffer) {
        MemCopy(lpcs->lpDOSWriteBuffer, p, dwSize);
        p = lpcs->lpDOSWriteBuffer;
    }

    return mmioWrite(lpcs->hmmio, p, (long)dwSize) == (long)dwSize;
}

//
// Writes dummy frames which on playback just repeat the previous frame
// nCount is a count of the number of frames to write
// Returns: TRUE on a successful write
BOOL AVIWriteDummyFrames (LPCAPSTREAM lpcs, int nCount)
{
    DWORD dwBytesToWrite;
    DWORD dwJunkSize;
    LPRIFF p;
    int j;

    p = (LPRIFF) lpcs->DropFrame;
    for (j = 0; j < nCount; j++) {
        // The index includes info on if this is a dummy chunk,
        // AND if this is the last dummy chunk in a sequence
        IndexVideo (lpcs, IS_DUMMY_CHUNK |
                ((j == nCount - 1) ? IS_LAST_DUMMY_CHUNK : 0), FALSE);
        if (lpcs->lpBitsInfo->bmiHeader.biCompression == BI_RLE8)
            p->dwType = MAKEAVICKID(cktypeDIBcompressed, 0);
        else
            p->dwType  = MAKEAVICKID(cktypeDIBbits, 0);
        p->dwSize  = 0;
        p++;
    }

    dwBytesToWrite = nCount * sizeof(RIFF);

    /* Pad out to multiple of lpcs->sCapParms.wChunkGranularity (2K) size */

    if (dwJunkSize = (dwBytesToWrite % lpcs->sCapParms.wChunkGranularity)) {
       dwJunkSize = lpcs->sCapParms.wChunkGranularity - dwJunkSize;
       if (dwJunkSize < sizeof(RIFF))
            dwJunkSize += lpcs->sCapParms.wChunkGranularity;

        dwBytesToWrite += dwJunkSize;

        dwJunkSize -= sizeof(RIFF);
    } else {
        dwJunkSize = 0L;
    }

    // Now create a new junk chunk at the end of the compressed data
    if(dwJunkSize) {
        p->dwType = ckidAVIPADDING;
        p->dwSize = dwJunkSize;
    }

    /* write out the dummy frames, and possibly the junk chunk */
    return (AVIWrite(lpcs, lpcs->DropFrame, dwBytesToWrite));
}

// Writes compressed or uncompressed frames to the AVI file
// returns TRUE if no error, FALSE if end of file.

BOOL AVIWriteVideoFrame (LPCAPSTREAM lpcs, LPVIDEOHDR lpVidHdr)
{
    DWORD dwBytesToWrite;
    DWORD dwJunkSize;
    LPVOID p;
    LPVOID lpData;

    // If the device compresses the data, calculate new junk chunk
    // and fix the RIFF header

    //
    // We are automatically compressing during capture, so
    // first compress the frame.
    //
    if (lpcs->CompVars.hic) {
        DWORD       dwBytesUsed = 0;	// don't force a data rate
        BOOL        fKeyFrame;

        lpData = ICSeqCompressFrame(&lpcs->CompVars, 0,
            lpVidHdr->lpData, &fKeyFrame, &dwBytesUsed);

        ((RIFF FAR*)lpData)[-1].dwType = MAKEAVICKID(cktypeDIBbits, 0);
        ((RIFF FAR*)lpData)[-1].dwSize = dwBytesUsed;

        if (fKeyFrame)
            lpVidHdr->dwFlags |= VHDR_KEYFRAME;
        else
            lpVidHdr->dwFlags &= ~VHDR_KEYFRAME;

        lpVidHdr->dwBytesUsed = dwBytesUsed;
    }
    else {
        lpData = lpVidHdr->lpData;
    }

    if (lpcs->fVideoDataIsCompressed) {       // ie. if not BI_RGB

        // change the dwSize field in the RIFF chunk
        *((LPDWORD)((BYTE _huge *)lpVidHdr->lpData - sizeof(DWORD)))
                = lpVidHdr->dwBytesUsed;

         // Make sure that the JUNK chunk starts on a WORD boundary
         if (lpVidHdr->dwBytesUsed & 1)
             ++lpVidHdr->dwBytesUsed;

        dwBytesToWrite = lpVidHdr->dwBytesUsed + sizeof(RIFF);

        /* Pad out to multiple of lpcs->sCapParms.wChunkGranularity (2K) size */

        if (dwJunkSize = (dwBytesToWrite % lpcs->sCapParms.wChunkGranularity)) {
             dwJunkSize = lpcs->sCapParms.wChunkGranularity - dwJunkSize;
             if (dwJunkSize < sizeof(RIFF))
                dwJunkSize += lpcs->sCapParms.wChunkGranularity;

            dwBytesToWrite += dwJunkSize;

             // Now create a new junk chunk at the end of the compressed data
             p = (BYTE huge *)lpVidHdr->lpData + lpVidHdr->dwBytesUsed;

             ((LPRIFF)p)->dwType = ckidAVIPADDING;
             ((LPRIFF)p)->dwSize = dwJunkSize - sizeof(RIFF);
        }
    } // endif compressed data
    else {
        dwBytesToWrite = lpcs->dwVideoSize;
    } // endif not compressed data

    /* write out the chunk, video data, and possibly the junk chunk */
    return (AVIWrite(lpcs, (LPBYTE)lpData - sizeof(RIFF), dwBytesToWrite));
}

//
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
    lpw   = (LPBYTE)lpcs->lpInfoChunks;           // always points at fcc
    lpEnd = (LPBYTE)lpcs->lpInfoChunks + lpcs->cbInfoChunks;
    while (lpw < lpEnd) {
        cbSizeThis = ((LPDWORD)lpw)[1];
        cbSizeThis += cbSizeThis & 1;           // force WORD alignment
        lpNext = lpw + cbSizeThis + sizeof (DWORD) * 2;
        if ((*(LPDWORD) lpw) == ckid) {
            lpcs->cbInfoChunks -= cbSizeThis + sizeof (DWORD) * 2;
            if (lpNext <= lpEnd) {
                if (lpEnd - lpNext)
                    hmemcpy(lpw, lpNext, lpEnd - lpNext);
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
    cbData += cbData & 1;               // force WORD alignment
    cbData += sizeof(DWORD) * 2;        // add sizeof 2 FOURCCs
    if (lpcs->lpInfoChunks) {
	lp = (LPBYTE) GlobalReAllocPtr(lpcs->lpInfoChunks, lpcs->cbInfoChunks + cbData, GMEM_MOVEABLE);
    } else {
	lp = (LPBYTE) GlobalAllocPtr(GMEM_MOVEABLE, cbData);
    }

    if (!lp)
	return FALSE;

    // build RIFF chunk in block
    ((DWORD FAR *) (lp + lpcs->cbInfoChunks))[0] = ckid;
    ((DWORD FAR *) (lp + lpcs->cbInfoChunks))[1] = lpcic->cbData;

    hmemcpy(lp + lpcs->cbInfoChunks + sizeof(DWORD) * 2,
	    lpData,
	    cbData - sizeof(DWORD) * 2);
    lpcs->lpInfoChunks = lp;
    lpcs->cbInfoChunks += cbData;

    return TRUE;
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
void FAR PASCAL _loadds AVICapture1(LPCAPSTREAM lpcs)
{
    BOOL        fOK = TRUE;
    BOOL        fT;
    BOOL        fVideoBuffersInDOSMem;
    BOOL        fStopping;         // True when finishing capture
    BOOL        fStopped;          // True if driver notified to stop
    DWORD       dw;
    char        ach[128];
    char        achMsg[128];
    WORD        w;
    WORD        wError;         // Error String ID
    DWORD       dwDriverDropCount;
    WORD        wSmartDrv;
    LPVIDEOHDR  lpVidHdr;
    LPWAVEHDR   lpWaveHdr;
    DWORD       dwTimeStarted;  // When did we start in milliseconds
    DWORD       dwTimeStopped;
    DWORD       dwTimeToStop;   // Lesser of MCI capture time or frame limit
    BOOL        fTryToPaint = FALSE;
    HDC         hdc;
    HPALETTE    hpalT;
    HCURSOR     hOldCursor;
    RECT        rcDrawRect;
    DWORD       dwStreamError;
    CAPINFOCHUNK cic;

    lpcs-> dwReturn = DV_ERR_OK;

    hOldCursor = SetCursor(lpcs->hWaitCursor);

    statusUpdateStatus(lpcs, IDS_CAP_BEGIN);  // Always the first message

    // If not 1 Meg. free, give it up!!!
    if (GetFreePhysicalMemory () < (1024L * 1024L)) {
        errorUpdateError (lpcs, IDS_CAP_OUTOFMEM);
        lpcs-> dwReturn = IDS_CAP_OUTOFMEM;
        goto EarlyExit;
    }

    statusUpdateStatus(lpcs, IDS_CAP_STAT_CAP_INIT);

    // Try painting the DIB only if Live window
    fTryToPaint = lpcs->fLiveWindow;

    if (fTryToPaint) {
        hdc = GetDC(lpcs->hwnd);
        SetWindowOrg(hdc, lpcs->ptScroll.x, lpcs->ptScroll.y);
        hpalT = DrawDibGetPalette (lpcs->hdd);
        if (hpalT)
            hpalT = SelectPalette( hdc, hpalT, FALSE);
        RealizePalette(hdc);
        if (lpcs-> fScale)
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
        // if MCI stop time not given, use lpcs->sCapParms.wTimeLimit
        if (lpcs->sCapParms.dwMCIStopTime == lpcs->sCapParms.dwMCIStartTime)
                    lpcs->sCapParms.dwMCIStopTime = lpcs->sCapParms.dwMCIStartTime +
                    (DWORD) ((DWORD)1000 * lpcs->sCapParms.wTimeLimit);

        dw = lpcs->sCapParms.dwMCIStopTime - lpcs->sCapParms.dwMCIStartTime;

        if (lpcs->sCapParms.fLimitEnabled)
            dwTimeToStop = min (dw, dwTimeToStop);
        else
            dwTimeToStop = dw;
    }

    //
    // never ever try to capture more than the index size!
    //
    if (lpcs->fCapturingToDisk) {
        dw = muldiv32(lpcs->sCapParms.dwIndexSize,
                lpcs->sCapParms.dwRequestMicroSecPerFrame,
                1000l);

        dwTimeToStop = min (dw, dwTimeToStop);
    }

    if (lpcs->sCapParms.fMCIControl) {
        fOK = FALSE;            // Assume the worst
        if (MCIDeviceOpen (lpcs)) {
            if (MCIDeviceSetPosition (lpcs, lpcs->sCapParms.dwMCIStartTime))
                fOK = TRUE;
        }
        if (!fOK) {
            errorUpdateError (lpcs, IDS_CAP_MCI_CONTROL_ERROR);
            statusUpdateStatus(lpcs, NULL);    // Clear status
            lpcs-> dwReturn = IDS_CAP_MCI_CONTROL_ERROR;
            goto EarlyExit;
        }
    }

    //
    // If we're compressing while capturing, warm up the compressor
    //
    if (lpcs->CompVars.hic) {
        if (ICSeqCompressFrameStart(&lpcs->CompVars, lpcs->lpBitsInfo) == NULL) {

	    // !!! We're in trouble here!
            dprintf("ICSeqCompressFrameStart failed !!!\n");
            lpcs-> dwReturn = IDS_CAP_COMPRESSOR_ERROR;
            errorUpdateError (lpcs, IDS_CAP_COMPRESSOR_ERROR);
            goto EarlyExit;
        }
        // Kludge, offset the lpBitsOut ptr
        // Compman allocates the compress buffer too large by
        // 2048 + 16 so we will still have room
        ((LPBYTE) lpcs->CompVars.lpBitsOut) += 8;
    }

    // No compression desired
    if (!lpcs->CompVars.hic)
	WinAssert(lpcs->CompVars.lpbiOut == NULL);

    // -------------------------------------------------------
    //  Open the output file
    // -------------------------------------------------------

    if (lpcs->fCapturingToDisk) {
        if (!AVIFileInit(lpcs)) {
            lpcs-> dwReturn = IDS_CAP_FILE_OPEN_ERROR;
            errorUpdateError (lpcs, IDS_CAP_FILE_OPEN_ERROR);
            goto EarlyExit;
        }
    }

    /* Make sure the parent has been repainted */
    UpdateWindow(lpcs->hwnd);

    //
    // call AVIInit() to get all the capture memory we will need
    //

    // Don't use DOS memory if capturing to Net
    fVideoBuffersInDOSMem = lpcs->sCapParms.fUsingDOSMemory;

    wError = AVIInit(lpcs);

    if (wError && fVideoBuffersInDOSMem) {
        lpcs->sCapParms.fUsingDOSMemory = FALSE;
        wError = AVIInit(lpcs);
    }

    if (wError) {
        /* Error in initalization - return */
        errorUpdateError (lpcs, wError);
        AVIFini(lpcs);
        AVIFileFini(lpcs, TRUE /* fWroteJunkChunks */, TRUE /* fAbort */);
        statusUpdateStatus(lpcs, NULL);    // Clear status
        lpcs-> dwReturn = wError;
        goto EarlyExit;
    }

    /* Click OK to capture string (must follow AVIInit) */
    LoadString(lpcs->hInst, IDS_CAP_SEQ_MSGSTART, ach, sizeof(ach));
    wsprintf(achMsg, ach, (LPSTR)lpcs->achFile);

    statusUpdateStatus(lpcs, NULL);

    // -------------------------------------------------------
    //   Ready to go, make the user click OK?
    // -------------------------------------------------------

    if (lpcs->sCapParms.fMakeUserHitOKToCapture && lpcs->fCapturingToDisk) {
	w = MessageBox(lpcs->hwnd, achMsg, "", MB_OKCANCEL | MB_ICONEXCLAMATION);
        if (w == IDCANCEL) {
            /* clean-up and get out */
            AVIFini(lpcs);
            AVIFileFini(lpcs, TRUE /* fWroteJunkChunks */, TRUE /* fAbort */);
            statusUpdateStatus(lpcs, NULL);    // Clear status
            goto EarlyExit;
        }
    } // endif forcing user to hit OK

    /* update the status, so the user knows how to stop */
    statusUpdateStatus(lpcs, IDS_CAP_SEQ_MSGSTOP);
    UpdateWindow(lpcs->hwnd);

    lpcs-> fCapturingNow = TRUE;

    GetAsyncKeyState(lpcs->sCapParms.vKeyAbort);
    GetAsyncKeyState(VK_ESCAPE);
    GetAsyncKeyState(VK_LBUTTON);
    GetAsyncKeyState(VK_RBUTTON);

    if (lpcs->sCapParms.fDisableWriteCache)
        wSmartDrv = SmartDrv(lpcs->achFile[0], (WORD)-1);  // turn all off....

    // Insert the digitization time
    cic.fccInfoID = mmioFOURCC ('I','D','I','T');
    time (&ltime);
    cic.lpData = (LPSTR) ctime(&ltime);
    cic.cbData  = 26;
    SetInfoChunk (lpcs, &cic);


    // -------------------------------------------------------
    //   Start MCI, Audio, and video streams
    // -------------------------------------------------------

    if (lpcs-> CallbackOnControl) {
        // Callback will preroll, then return on frame accurate postion
        // The 1 indicates recording is about to start
        // Callback can return FALSE to exit without capturing
        if (!((*(lpcs->CallbackOnControl)) (lpcs->hwnd, CONTROLCALLBACK_PREROLL ))) {
            /* clean-up and get out */
            AVIFini(lpcs);
            AVIFileFini(lpcs, TRUE /* fWroteJunkChunks */, TRUE /* fAbort */);
            statusUpdateStatus(lpcs, NULL);    // Clear status
            goto EarlyExit;
        }
    }

    if (lpcs->sCapParms.fMCIControl)
        MCIDevicePlay (lpcs);

    dwTimeStarted = timeGetTime();

    if(lpcs->sCapParms.fCaptureAudio)
        waveInStart(lpcs->hWaveIn);

    videoStreamStart(lpcs->hVideoIn);

    // -------------------------------------------------------
    //   MAIN CAPTURE LOOP
    // -------------------------------------------------------

    fOK=TRUE;
    fStopping = FALSE;    // TRUE when we need to stop
    fStopped = FALSE;     // TRUE if drivers notified we have stopped
    lpcs->dwTimeElapsedMS = 0;

    lpVidHdr = lpcs->alpVideoHdr[lpcs->iNextVideo];
    lpWaveHdr = lpcs->alpWaveHdr[lpcs->iNextWave];

    for (;;) {

        // The INTEL driver uses the GetError message to
        // process buffers, so call it often...
        videoStreamGetError (lpcs->hVideoIn, &dwStreamError, &dwDriverDropCount);

        // What time is it?
        lpcs->dwTimeElapsedMS = timeGetTime() - dwTimeStarted;

        // -------------------------------------------------------
        //        Is video buffer ready to be written?
        // -------------------------------------------------------
        if ((lpVidHdr->dwFlags & VHDR_DONE)) {
            if (lpVidHdr-> dwBytesUsed) {
                // Current time in milliseconds
                dw = muldiv32 ((lpcs->dwVideoChunkCount + 1),
                                lpcs->sCapParms.dwRequestMicroSecPerFrame, 1000);
                if (lpcs->CallbackOnVideoStream)
                    (*(lpcs->CallbackOnVideoStream)) (lpcs->hwnd, lpVidHdr);

                if (lpcs-> fCapturingToDisk) {
                    if (lpcs->dwVideoChunkCount &&
                                (dw < lpVidHdr->dwTimeCaptured)) {
                        // Has the capture device skipped frames?
                        // w = # of frames skipped
                        w = (WORD) muldiv32 ((lpVidHdr-> dwTimeCaptured - dw),
                                1000,
                                lpcs->sCapParms.dwRequestMicroSecPerFrame);
                        w = min (w, (sizeof (lpcs->DropFrame) / sizeof (RIFF) - sizeof (RIFF) ) );
                        lpcs->dwFramesDropped+= w;
                        fOK = AVIWriteDummyFrames (lpcs, w);

                        if (!fOK)
                            fStopping = TRUE;
                    } // end if writing dummy frames

                    if (!AVIWriteVideoFrame (lpcs, lpVidHdr)) {
                        fOK = FALSE;
                        fStopping = TRUE;
                        // "ERROR: Could not write to file."
                        errorUpdateError(lpcs, IDS_CAP_FILE_WRITE_ERROR);
                    }
                    else {
                        if (!IndexVideo(lpcs, lpVidHdr-> dwBytesUsed,
                                (BOOL) (lpVidHdr->dwFlags & VHDR_KEYFRAME)))
                            fStopping = TRUE;
                    }
                } // endif fCapturingToDisk
                // Warning: Kludge to create frame chunk count when net capture
                // follows.
                else
                    lpcs->dwVideoChunkCount++;

                // -------------------------------------------------------
                //         if we have *nothing* to do paint or show status.
                // -------------------------------------------------------
                w = (lpcs->iNextVideo + 1) % lpcs->iNumVideo;
                if (!(lpcs->alpVideoHdr[w]-> dwFlags & VHDR_DONE)) {
                    if (fTryToPaint && lpcs->dwVideoChunkCount &&
                                lpVidHdr-> dwFlags & VHDR_KEYFRAME) {
                        fTryToPaint = DrawDibDraw(lpcs->hdd, hdc,
                                0, 0,
                                rcDrawRect.right - rcDrawRect.left,
                                rcDrawRect.bottom - rcDrawRect.top,
                                /*lpcs->dxBits, lpcs->dyBits, */
                                (LPBITMAPINFOHEADER)lpcs->lpBitsInfo,
                                lpVidHdr-> lpData, 0, 0, -1, -1,
                                DDF_SAME_HDC | DDF_SAME_DIB | DDF_SAME_SIZE);
                    }
                }
                // if there is still more time, (or at least every 100 frames)
                // show status if we're not ending the capture
                if ((!fStopping) && (lpcs-> fCapturingToDisk) &&
                        ((lpcs->dwVideoChunkCount && (lpcs->dwVideoChunkCount % 100 == 0)) ||
                        (!(lpcs->alpVideoHdr[w]-> dwFlags & VHDR_DONE)) ) ) {

                    // "Captured %ld frames (Dropped %ld) %d.%03d sec. Hit Escape to Stop"
                    statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOCURRENT,
                            lpcs->dwVideoChunkCount, lpcs->dwFramesDropped,
                            (int)(lpcs-> dwTimeElapsedMS/1000), (int)(lpcs-> dwTimeElapsedMS%1000)
                            );
                } // endif next buffer not ready
            } // endif any bytes used in the buffer

             /* return the emptied buffer to the que */
            lpVidHdr->dwFlags &= ~VHDR_DONE;
            if (videoStreamAddBuffer(lpcs->hVideoIn,
                        lpVidHdr, sizeof (VIDEOHDR))) {
                fOK = FALSE;
                fStopping = TRUE;
                // "ERROR: Could not re-add buffer."
                  errorUpdateError (lpcs, IDS_CAP_VIDEO_ADD_ERROR);
            }

            /* increment the next Video buffer pointer */
            if (++lpcs->iNextVideo >= lpcs->iNumVideo)
                lpcs->iNextVideo = 0;

            lpVidHdr = lpcs->alpVideoHdr[lpcs->iNextVideo];
        }

        if (lpcs-> CallbackOnYield) {
            // If the yield callback returns FALSE, abort
            if (!((*(lpcs->CallbackOnYield)) (lpcs->hwnd)))
                fStopping = TRUE;
        }

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

        if (lpcs-> CallbackOnControl) {
            // Outside routine is handling when to stop
            // The CONTROLCALLBACK_CAPTURING indicates we're asking when to stop
            if (!((*(lpcs->CallbackOnControl)) (lpcs->hwnd, CONTROLCALLBACK_CAPTURING )))
                fStopping = TRUE;
        }

        // -------------------------------------------------------
        //        Is audio buffer ready to be written?
        // -------------------------------------------------------
        if (lpcs->sCapParms.fCaptureAudio) {
            int iLastWave;

            //
            // we may need to yield for audio to get converted.
            //
            if (lpcs->fAudioYield)
                Yield();

            //
            // if all buffers are done, we have broke audio.
            //
            iLastWave = lpcs->iNextWave == 0 ?
                        lpcs->iNumAudio -1 : lpcs->iNextWave-1;

            if (!fStopping &&
                    lpcs->alpWaveHdr[iLastWave]->dwFlags & WHDR_DONE)
                lpcs->fAudioBreak = TRUE;

            w = lpcs->iNumAudio; // don't get stuck here forever...
            while (w && fOK && (lpWaveHdr-> dwFlags & WHDR_DONE)) {
                w--;
                if (lpWaveHdr-> dwBytesRecorded) {
                    /* Chunk info is included in the wave data */
                    /* Reset Chunk Size in buffer */
                    ((LPRIFF)(lpWaveHdr->lpData))[-1].dwSize =
                                lpWaveHdr-> dwBytesRecorded;
                    if (lpcs-> CallbackOnWaveStream) {
                        (*(lpcs->CallbackOnWaveStream)) (lpcs->hwnd, lpWaveHdr);
                    }
                    if (lpcs-> fCapturingToDisk) {
                        if(!AVIWrite (lpcs, lpWaveHdr-> lpData - sizeof(RIFF),
                                (lpWaveHdr-> dwBytesRecorded +
                                sizeof (RIFF) + 1) & ~1L)) {
                            fOK = FALSE;
                            fStopping = TRUE;
                            errorUpdateError (lpcs, IDS_CAP_FILE_WRITE_ERROR);
                         } else {
                            if (IndexAudio (lpcs, lpWaveHdr-> dwBytesRecorded))
                                lpcs->dwWaveBytes += lpWaveHdr-> dwBytesRecorded;
                            else
                                fStopping = TRUE;
                         }
                    } // endif capturing to disk
                    // Warning: Kludge to create wave chunk count when net capture
                    // follows.
                    else {
                        lpcs->dwWaveChunkCount++;
                        lpcs->dwWaveBytes += lpWaveHdr-> dwBytesRecorded;
                    }
                } // endif dwBytesRecorded

                lpWaveHdr-> dwBytesRecorded = 0;
                lpWaveHdr-> dwFlags &= ~WHDR_DONE;

                 /* return the emptied buffer to the que */
                if(waveInAddBuffer(lpcs->hWaveIn, lpWaveHdr, sizeof(WAVEHDR))) {
                    fOK = FALSE;
                    fStopping = TRUE;
                    errorUpdateError(lpcs, IDS_CAP_WAVE_ADD_ERROR);
                }

                /* increment the next wave buffer pointer */
                if(++lpcs->iNextWave >= lpcs->iNumAudio)
                    lpcs->iNextWave = 0;

                lpWaveHdr = lpcs->alpWaveHdr[lpcs->iNextWave];

            } // endwhile buffer available
         } // endif sound enabled

        // -------------------------------------------------------
        //        is there any reason to stop?
        // -------------------------------------------------------
        if (lpcs->sCapParms.vKeyAbort) {
            if (GetAsyncKeyState(lpcs->sCapParms.vKeyAbort & 0x00ff) & 0x0001) {
                fT = TRUE;
                if (lpcs->sCapParms.vKeyAbort & 0x8000)  // Ctrl?
                    fT = fT && (GetAsyncKeyState(VK_CONTROL) & 0x8000);
                if (lpcs->sCapParms.vKeyAbort & 0x4000)  // Shift?
                    fT = fT && (GetAsyncKeyState(VK_SHIFT) & 0x8000);
                fStopping = fT;      // User aborts
            }
        }
        if (lpcs->sCapParms.fAbortLeftMouse)
            if (GetAsyncKeyState(VK_LBUTTON) & 0x0001)
                fStopping = TRUE;      // User aborts
        if (lpcs->sCapParms.fAbortRightMouse)
            if (GetAsyncKeyState(VK_RBUTTON) & 0x0001)
                fStopping = TRUE;      // User aborts
        if (lpcs-> fAbortCapture || lpcs-> fStopCapture)
            fStopping = TRUE;          // Somebody above wants us to quit

        if (lpcs-> dwTimeElapsedMS > dwTimeToStop)
            fStopping = TRUE;      // all done

        // -------------------------------------------------------
        //        Quit only when we have stopped, and
        //      no more buffers are pending from any device.
        // -------------------------------------------------------
        if (fStopped) {
            if (!(lpVidHdr-> dwFlags & VHDR_DONE)) {
                if (lpcs->sCapParms.fCaptureAudio) {
                   if (!(lpWaveHdr-> dwFlags & WHDR_DONE))
                        break;
                }
                else
                    break;
            }
        }

        // -------------------------------------------------------
        //        Tell all the devices to stop
        // -------------------------------------------------------
        if (fStopping && !fStopped) {
            fStopped = TRUE;

            DSTATUS(lpcs, "Stopping....");

            if(lpcs->sCapParms.fCaptureAudio) {
                DSTATUS(lpcs, "Stopping Audio");
                waveInStop(lpcs->hWaveIn);
            }

            DSTATUS(lpcs, "Stopping Video");
            videoStreamStop(lpcs->hVideoIn);         // Stop everybody

            dwTimeStopped = timeGetTime ();

            if (lpcs->sCapParms.fMCIControl) {
                DSTATUS(lpcs, "Stopping MCI");
                MCIDevicePause (lpcs);
            }
            DSTATUS(lpcs, "Stopped");

            SetCursor(lpcs->hWaitCursor);  // Force cursor back to hourglass

        }


        if (fStopping) {
            // "Finished capture, now writing frame %ld"
            if (fOK) {
                statusUpdateStatus(lpcs, IDS_CAP_STAT_CAP_FINI, lpcs->dwVideoChunkCount);
            }
            else {               // Exit if problems
                statusUpdateStatus(lpcs, IDS_CAP_RECORDING_ERROR2);
                break;
            }
        }

    } // end of forever

    // -------------------------------------------------------
    //   END OF MAIN CAPTURE LOOP
    // -------------------------------------------------------

    if (lpcs->sCapParms.fDisableWriteCache)
        SmartDrv(lpcs->achFile[0], wSmartDrv);  // turn Smartdrive back on

    /* eat any keys that have been pressed */
    while(GetKey(FALSE))
        ;

    AVIFini(lpcs);  // does the Reset, and frees all buffers
    AVIFileFini(lpcs, TRUE /* fWroteJunkChunks */, FALSE /* fAbort */);

    // This is the corrected capture duration, based on audio samples
    lpcs->dwTimeElapsedMS = lpcs->dwActualMicroSecPerFrame *
                lpcs->dwVideoChunkCount / 1000;

    /* Notify if there was an error while recording */

    if(!fOK) {
        errorUpdateError (lpcs, IDS_CAP_RECORDING_ERROR);
    }


    if (lpcs-> fCapturingToDisk) {
        if (lpcs->dwVideoChunkCount)
            dw = muldiv32(lpcs->dwVideoChunkCount,1000000,lpcs-> dwTimeElapsedMS);
        else
            dw = 0;     // The muldiv32 doesn't give 0 if numerator is zero

        if(lpcs->sCapParms.fCaptureAudio) {
            // "Captured %d.%03d sec.  %ld frames (%ld dropped) (%d.%03d fps).  %ld audio bytes (%d.%03d sps)"
            statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOAUDIO,
                  (WORD)(lpcs-> dwTimeElapsedMS/1000),
                  (WORD)(lpcs-> dwTimeElapsedMS%1000),
                  lpcs->dwVideoChunkCount,
                  lpcs->dwFramesDropped,
                  (WORD)(dw / 1000),
                  (WORD)(dw % 1000),
                  lpcs->dwWaveBytes,
                  (WORD) lpcs->lpWaveFormat->nSamplesPerSec / 1000,
                  (WORD) lpcs->lpWaveFormat->nSamplesPerSec % 1000);
        } else {
            // "Captured %d.%03d sec.  %ld frames (%ld dropped) (%d.%03d fps)."
            statusUpdateStatus(lpcs, IDS_CAP_STAT_VIDEOONLY,
                  (WORD)(lpcs-> dwTimeElapsedMS/1000),
                  (WORD)(lpcs-> dwTimeElapsedMS%1000),
                  lpcs->dwVideoChunkCount,
                  lpcs->dwFramesDropped,
                  (WORD)(dw / 1000),
                  (WORD)(dw % 1000));
        }
    } // endif capturing to disk (no warnings or errors if to net)

    // No frames captured, warn user that interrupts are probably not enabled.
    if (fOK && (lpcs->dwVideoChunkCount == 0)) {
        errorUpdateError (lpcs, IDS_CAP_NO_FRAME_CAP_ERROR);
    }
    // No audio captured, (but enabled), warn user audio card is hosed
    else if (fOK && lpcs->sCapParms.fCaptureAudio && (lpcs->dwWaveBytes == 0)) {
        errorUpdateError (lpcs, IDS_CAP_NO_AUDIO_CAP_ERROR);
    }
    // Audio underrun, inform user
    else if (fOK && lpcs->sCapParms.fCaptureAudio && lpcs->fAudioBreak) {
        errorUpdateError (lpcs, IDS_CAP_AUDIO_DROP_ERROR);
    }

    // If frames dropped, or changed capture rate, warn the user
    else if (fOK && lpcs->dwVideoChunkCount && lpcs->fCapturingToDisk) {

        // Warn user if dropped > 10% (default) of the frames
        if ((DWORD)100 * lpcs->dwFramesDropped / lpcs->dwVideoChunkCount >
                    lpcs-> sCapParms.wPercentDropForError) {

            // "%ld of %ld frames (%d.%03d\%) dropped during capture."
            errorUpdateError (lpcs, IDS_CAP_STAT_FRAMESDROPPED,
                  lpcs->dwFramesDropped,
                  lpcs->dwVideoChunkCount,
                  (WORD)(muldiv32(lpcs->dwFramesDropped,10000,lpcs->dwVideoChunkCount)/100),
                  (WORD)(muldiv32(lpcs->dwFramesDropped,10000,lpcs->dwVideoChunkCount)%100)
                  );
        }
    }

EarlyExit:

    //
    // If we were compressing while capturing, close it down
    //
    if (lpcs->CompVars.hic) {
        // Kludge, reset the lpBitsOut pointer
        if (lpcs->CompVars.lpBitsOut)
            ((LPBYTE) lpcs->CompVars.lpBitsOut) -= 8;
	ICSeqCompressFrameEnd(&lpcs->CompVars);
    }

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

    SetCursor(hOldCursor);

    lpcs->fCapFileExists = (lpcs-> dwReturn == DV_ERR_OK);
    lpcs->fCapturingNow = FALSE;

    statusUpdateStatus(lpcs, IDS_CAP_END);      // Always the last message

    return;
}


// Returns TRUE if the capture task was created, or
// capture completed OK.

BOOL AVICapture (LPCAPSTREAM lpcs)
{
    WORD w;
    CAPINFOCHUNK cic;
    char szSMPTE[40];

    if (lpcs-> fCapturingNow)
        return IDS_CAP_VIDEO_OPEN_ERROR;

    lpcs-> fStopCapture  = FALSE;
    lpcs-> fAbortCapture = FALSE;
    lpcs-> hTaskCapture  = NULL;
    lpcs-> dwReturn      = 0;

    // Clear any SMPTE info chunk
    cic.fccInfoID = mmioFOURCC ('I','S','M','T');
    cic.lpData = NULL;
    cic.cbData = 0;
    SetInfoChunk (lpcs, &cic);

#if 1
    // And get ready to write a SMPTE info chunk
    if (lpcs->sCapParms.fMCIControl) {
        // create SMPTE string
        TimeMSToSMPTE (lpcs->sCapParms.dwMCIStartTime, (LPSTR) szSMPTE);
        cic.lpData = szSMPTE;
        cic.cbData = lstrlen (szSMPTE) + 1;
        SetInfoChunk (lpcs, &cic);
    }
#endif

    // Use an MCI device to do step capture capture???
    if (lpcs->sCapParms.fStepMCIDevice && lpcs->sCapParms.fMCIControl) {
        if (lpcs->sCapParms.fYield) {
            w = (WORD) mmTaskCreate((LPTASKCALLBACK) MCIStepCapture,
                        &lpcs->hTaskCapture, (DWORD) lpcs);
            // if task creation failed, turn off the capturing flag
            if (w != 0)
                lpcs->fCapturingNow = FALSE;
            return ((BOOL) !w);
        }
        else  {
            MCIStepCapture (lpcs);
            return ((BOOL) !lpcs->dwReturn);
        }
    }

    // No MCI device, just a normal streaming capture
    else if (lpcs->sCapParms.fYield) {
        w = (WORD) mmTaskCreate((LPTASKCALLBACK) AVICapture1,
                &lpcs->hTaskCapture, (DWORD) lpcs);
        // if task creation failed, turn off the capturing flag
        if (w != 0)
            lpcs->fCapturingNow = FALSE;
        return ((BOOL) !w);
    }
    else  {
        AVICapture1 (lpcs);
        return ((BOOL) !lpcs->dwReturn);
    }
}





