/*
 * Digital video MCI Wine Driver
 *
 * Copyright 1999, 2000 Eric POUECH
 * Copyright 2003 Dmitry Timoshkov
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

#include "private_mciavi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciavi);

static BOOL MCIAVI_GetInfoAudio(WINE_MCIAVI* wma, const MMCKINFO* mmckList, MMCKINFO *mmckStream)
{
    MMCKINFO	mmckInfo;

    TRACE("ash.fccType=%s\n",			debugstr_fourcc(wma->ash_audio.fccType));
    if (wma->ash_audio.fccHandler) /* not all streams specify a handler */
        TRACE("ash.fccHandler=%s\n",		debugstr_fourcc(wma->ash_audio.fccHandler));
    else
        TRACE("ash.fccHandler=0, no handler specified\n");
    TRACE("ash.dwFlags=%ld\n", 			wma->ash_audio.dwFlags);
    TRACE("ash.wPriority=%d\n", 		wma->ash_audio.wPriority);
    TRACE("ash.wLanguage=%d\n", 		wma->ash_audio.wLanguage);
    TRACE("ash.dwInitialFrames=%ld\n", 		wma->ash_audio.dwInitialFrames);
    TRACE("ash.dwScale=%ld\n", 			wma->ash_audio.dwScale);
    TRACE("ash.dwRate=%ld\n", 			wma->ash_audio.dwRate);
    TRACE("ash.dwStart=%ld\n", 			wma->ash_audio.dwStart);
    TRACE("ash.dwLength=%ld\n", 		wma->ash_audio.dwLength);
    TRACE("ash.dwSuggestedBufferSize=%ld\n", 	wma->ash_audio.dwSuggestedBufferSize);
    TRACE("ash.dwQuality=%ld\n", 		wma->ash_audio.dwQuality);
    TRACE("ash.dwSampleSize=%ld\n", 		wma->ash_audio.dwSampleSize);
    TRACE("ash.rcFrame=(%d,%d,%d,%d)\n", 	wma->ash_audio.rcFrame.top, wma->ash_audio.rcFrame.left,
	  wma->ash_audio.rcFrame.bottom, wma->ash_audio.rcFrame.right);

    /* rewind to the start of the stream */
    mmioAscend(wma->hFile, mmckStream, 0);

    mmckInfo.ckid = ckidSTREAMFORMAT;
    if (mmioDescend(wma->hFile, &mmckInfo, mmckList, MMIO_FINDCHUNK) != 0) {
       WARN("Can't find 'strf' chunk\n");
	return FALSE;
    }
    if (mmckInfo.cksize < sizeof(WAVEFORMAT)) {
	WARN("Size of strf chunk (%ld) < audio format struct\n", mmckInfo.cksize);
	return FALSE;
    }
    wma->lpWaveFormat = malloc(mmckInfo.cksize);
    if (!wma->lpWaveFormat) {
	WARN("Can't alloc WaveFormat\n");
	return FALSE;
    }

    mmioRead(wma->hFile, (LPSTR)wma->lpWaveFormat, mmckInfo.cksize);

    TRACE("waveFormat.wFormatTag=%d\n",		wma->lpWaveFormat->wFormatTag);
    TRACE("waveFormat.nChannels=%d\n", 		wma->lpWaveFormat->nChannels);
    TRACE("waveFormat.nSamplesPerSec=%ld\n",	wma->lpWaveFormat->nSamplesPerSec);
    TRACE("waveFormat.nAvgBytesPerSec=%ld\n",	wma->lpWaveFormat->nAvgBytesPerSec);
    TRACE("waveFormat.nBlockAlign=%d\n",	wma->lpWaveFormat->nBlockAlign);
    TRACE("waveFormat.wBitsPerSample=%d\n",	wma->lpWaveFormat->wBitsPerSample);
    if (mmckInfo.cksize >= sizeof(WAVEFORMATEX))
	TRACE("waveFormat.cbSize=%d\n", 		wma->lpWaveFormat->cbSize);

    return TRUE;
}

static BOOL MCIAVI_GetInfoVideo(WINE_MCIAVI* wma, const MMCKINFO* mmckList, MMCKINFO* mmckStream)
{
    MMCKINFO	mmckInfo;

    TRACE("ash.fccType=%s\n",			debugstr_fourcc(wma->ash_video.fccType));
    TRACE("ash.fccHandler=%s\n",		debugstr_fourcc(wma->ash_video.fccHandler));
    TRACE("ash.dwFlags=%ld\n", 			wma->ash_video.dwFlags);
    TRACE("ash.wPriority=%d\n", 		wma->ash_video.wPriority);
    TRACE("ash.wLanguage=%d\n", 		wma->ash_video.wLanguage);
    TRACE("ash.dwInitialFrames=%ld\n", 		wma->ash_video.dwInitialFrames);
    TRACE("ash.dwScale=%ld\n", 			wma->ash_video.dwScale);
    TRACE("ash.dwRate=%ld\n", 			wma->ash_video.dwRate);
    TRACE("ash.dwStart=%ld\n", 			wma->ash_video.dwStart);
    TRACE("ash.dwLength=%ld\n", 		wma->ash_video.dwLength);
    TRACE("ash.dwSuggestedBufferSize=%ld\n", 	wma->ash_video.dwSuggestedBufferSize);
    TRACE("ash.dwQuality=%ld\n", 		wma->ash_video.dwQuality);
    TRACE("ash.dwSampleSize=%ld\n", 		wma->ash_video.dwSampleSize);
    TRACE("ash.rcFrame=(%d,%d,%d,%d)\n", 	wma->ash_video.rcFrame.top, wma->ash_video.rcFrame.left,
	  wma->ash_video.rcFrame.bottom, wma->ash_video.rcFrame.right);

    /* rewind to the start of the stream */
    mmioAscend(wma->hFile, mmckStream, 0);

    mmckInfo.ckid = ckidSTREAMFORMAT;
    if (mmioDescend(wma->hFile, &mmckInfo, mmckList, MMIO_FINDCHUNK) != 0) {
       WARN("Can't find 'strf' chunk\n");
	return FALSE;
    }

    wma->inbih = malloc(mmckInfo.cksize);
    if (!wma->inbih) {
	WARN("Can't alloc input BIH\n");
	return FALSE;
    }

    mmioRead(wma->hFile, (LPSTR)wma->inbih, mmckInfo.cksize);

    TRACE("bih.biSize=%ld\n", 		wma->inbih->biSize);
    TRACE("bih.biWidth=%ld\n", 		wma->inbih->biWidth);
    TRACE("bih.biHeight=%ld\n", 	wma->inbih->biHeight);
    TRACE("bih.biPlanes=%d\n", 		wma->inbih->biPlanes);
    TRACE("bih.biBitCount=%d\n", 	wma->inbih->biBitCount);
    TRACE("bih.biCompression=%lx\n", 	wma->inbih->biCompression);
    TRACE("bih.biSizeImage=%ld\n", 	wma->inbih->biSizeImage);
    TRACE("bih.biXPelsPerMeter=%ld\n", 	wma->inbih->biXPelsPerMeter);
    TRACE("bih.biYPelsPerMeter=%ld\n", 	wma->inbih->biYPelsPerMeter);
    TRACE("bih.biClrUsed=%ld\n", 	wma->inbih->biClrUsed);
    TRACE("bih.biClrImportant=%ld\n", 	wma->inbih->biClrImportant);

    SetRect(&wma->source, 0, 0, wma->inbih->biWidth, wma->inbih->biHeight);
    wma->dest = wma->source;

    return TRUE;
}

struct AviListBuild {
    DWORD	numVideoFrames;
    DWORD	numAudioAllocated;
    DWORD	numAudioBlocks;
    DWORD	inVideoSize;
    DWORD	inAudioSize;
};

static BOOL	MCIAVI_AddFrame(WINE_MCIAVI* wma, LPMMCKINFO mmck,
				struct AviListBuild* alb)
{
    const BYTE *p;
    DWORD stream_n;
    DWORD twocc;

    if (mmck->ckid == ckidAVIPADDING) return TRUE;

    p = (const BYTE *)&mmck->ckid;

    if (!isxdigit(p[0]) || !isxdigit(p[1]))
    {
        WARN("wrongly encoded stream #\n");
        return FALSE;
    }

    stream_n = (p[0] <= '9') ? (p[0] - '0') : (tolower(p[0]) - 'a' + 10);
    stream_n <<= 4;
    stream_n |= (p[1] <= '9') ? (p[1] - '0') : (tolower(p[1]) - 'a' + 10);

    TRACE("ckid %4.4s (stream #%ld)\n", (LPSTR)&mmck->ckid, stream_n);

    /* Some (rare?) AVI files have video streams name XXYY where XX = stream number and YY = TWOCC
     * of the last 2 characters of the biCompression member of the BITMAPINFOHEADER structure.
     * Ex: fccHandler = IV32 & biCompression = IV32 => stream name = XX32
     *     fccHandler = MSVC & biCompression = CRAM => stream name = XXAM
     * Another possibility is that these TWOCC are simply ignored.
     * Default to cktypeDIBcompressed when this case happens.
     */
    twocc = TWOCCFromFOURCC(mmck->ckid);
    if (twocc == TWOCCFromFOURCC(wma->inbih->biCompression))
        twocc = cktypeDIBcompressed;
    /* Also detect some chunks that seem to be used by Indeo videos where the chunk is named
     * after the codec. */
    else if (twocc == LOWORD(wma->ash_video.fccHandler))
        twocc = cktypeDIBcompressed;
    switch (twocc) {
    case cktypeDIBbits:
    case cktypeDIBcompressed:
    case cktypePALchange:
        if (stream_n != wma->video_stream_n)
        {
            TRACE("data belongs to another video stream #%ld\n", stream_n);
            return FALSE;
        }

	TRACE("Adding video frame[%ld]: %ld bytes\n",
	      alb->numVideoFrames, mmck->cksize);

	if (alb->numVideoFrames < wma->dwPlayableVideoFrames) {
	    wma->lpVideoIndex[alb->numVideoFrames].dwOffset = mmck->dwDataOffset;
	    wma->lpVideoIndex[alb->numVideoFrames].dwSize = mmck->cksize;
	    if (alb->inVideoSize < mmck->cksize)
		alb->inVideoSize = mmck->cksize;
	    alb->numVideoFrames++;
	} else {
	    WARN("Too many video frames\n");
	}
	break;
    case cktypeWAVEbytes:
        if (stream_n != wma->audio_stream_n)
        {
            TRACE("data belongs to another audio stream #%ld\n", stream_n);
            return FALSE;
        }

	TRACE("Adding audio frame[%ld]: %ld bytes\n",
	      alb->numAudioBlocks, mmck->cksize);
	if (wma->lpWaveFormat) {
	    if (alb->numAudioBlocks >= alb->numAudioAllocated) {
                DWORD newsize = alb->numAudioAllocated + 32;
                struct MMIOPos* newindex;

                newindex = realloc(wma->lpAudioIndex, newsize * sizeof(struct MMIOPos));
                if (!newindex) return FALSE;
                alb->numAudioAllocated = newsize;
                wma->lpAudioIndex = newindex;
	    }
	    wma->lpAudioIndex[alb->numAudioBlocks].dwOffset = mmck->dwDataOffset;
	    wma->lpAudioIndex[alb->numAudioBlocks].dwSize = mmck->cksize;
	    if (alb->inAudioSize < mmck->cksize)
		alb->inAudioSize = mmck->cksize;
	    alb->numAudioBlocks++;
	} else {
	    WARN("Wave chunk without wave format... discarding\n");
	}
	break;
    default:
        WARN("Unknown frame type %4.4s\n", (LPSTR)&mmck->ckid);
	break;
    }
    return TRUE;
}

BOOL MCIAVI_GetInfo(WINE_MCIAVI* wma)
{
    MMCKINFO		ckMainRIFF;
    MMCKINFO		mmckHead;
    MMCKINFO		mmckList;
    MMCKINFO		mmckInfo;
    AVIStreamHeader strh;
    struct AviListBuild alb;
    DWORD stream_n;

    if (mmioDescend(wma->hFile, &ckMainRIFF, NULL, 0) != 0) {
	WARN("Can't find 'RIFF' chunk\n");
	return FALSE;
    }

    if ((ckMainRIFF.ckid != FOURCC_RIFF) || (ckMainRIFF.fccType != formtypeAVI)) {
	WARN("Can't find 'AVI ' chunk\n");
	return FALSE;
    }

    mmckHead.fccType = listtypeAVIHEADER;
    if (mmioDescend(wma->hFile, &mmckHead, &ckMainRIFF, MMIO_FINDLIST) != 0) {
	WARN("Can't find 'hdrl' list\n");
	return FALSE;
    }

    mmckInfo.ckid = ckidAVIMAINHDR;
    if (mmioDescend(wma->hFile, &mmckInfo, &mmckHead, MMIO_FINDCHUNK) != 0) {
	WARN("Can't find 'avih' chunk\n");
	return FALSE;
    }

    mmioRead(wma->hFile, (LPSTR)&wma->mah, sizeof(wma->mah));

    TRACE("mah.dwMicroSecPerFrame=%ld\n", 	wma->mah.dwMicroSecPerFrame);
    TRACE("mah.dwMaxBytesPerSec=%ld\n", 	wma->mah.dwMaxBytesPerSec);
    TRACE("mah.dwPaddingGranularity=%ld\n", 	wma->mah.dwPaddingGranularity);
    TRACE("mah.dwFlags=%ld\n", 			wma->mah.dwFlags);
    TRACE("mah.dwTotalFrames=%ld\n", 		wma->mah.dwTotalFrames);
    TRACE("mah.dwInitialFrames=%ld\n", 		wma->mah.dwInitialFrames);
    TRACE("mah.dwStreams=%ld\n", 		wma->mah.dwStreams);
    TRACE("mah.dwSuggestedBufferSize=%ld\n",	wma->mah.dwSuggestedBufferSize);
    TRACE("mah.dwWidth=%ld\n", 			wma->mah.dwWidth);
    TRACE("mah.dwHeight=%ld\n", 		wma->mah.dwHeight);

    mmioAscend(wma->hFile, &mmckInfo, 0);

    TRACE("Start of streams\n");
    wma->video_stream_n = 0;
    wma->audio_stream_n = 0;

    for (stream_n = 0; stream_n < wma->mah.dwStreams; stream_n++)
    {
        MMCKINFO mmckStream;

        mmckList.fccType = listtypeSTREAMHEADER;
        if (mmioDescend(wma->hFile, &mmckList, &mmckHead, MMIO_FINDLIST) != 0)
            break;

        mmckStream.ckid = ckidSTREAMHEADER;
        if (mmioDescend(wma->hFile, &mmckStream, &mmckList, MMIO_FINDCHUNK) != 0)
        {
            WARN("Can't find 'strh' chunk\n");
            continue;
        }

        mmioRead(wma->hFile, (LPSTR)&strh, sizeof(strh));

        TRACE("Stream #%ld fccType %4.4s\n", stream_n, (LPSTR)&strh.fccType);

        if (strh.fccType == streamtypeVIDEO)
        {
            TRACE("found video stream\n");
            if (wma->inbih)
                WARN("ignoring another video stream\n");
            else
            {
                wma->ash_video = strh;

                if (!MCIAVI_GetInfoVideo(wma, &mmckList, &mmckStream))
                    return FALSE;
                wma->video_stream_n = stream_n;
                wma->dwSet |= 4;
            }
        }
        else if (strh.fccType == streamtypeAUDIO)
        {
            TRACE("found audio stream\n");
            if (wma->lpWaveFormat)
                WARN("ignoring another audio stream\n");
            else
            {
                wma->ash_audio = strh;

                if (!MCIAVI_GetInfoAudio(wma, &mmckList, &mmckStream))
                    return FALSE;
                wma->audio_stream_n = stream_n;
                wma->dwSet |= 3;
            }
        }
        else
            TRACE("Unsupported stream type %4.4s\n", (LPSTR)&strh.fccType);

        mmioAscend(wma->hFile, &mmckList, 0);
    }

    TRACE("End of streams\n");

    mmioAscend(wma->hFile, &mmckHead, 0);

    /* no need to read optional JUNK chunk */

    mmckList.fccType = listtypeAVIMOVIE;
    if (mmioDescend(wma->hFile, &mmckList, &ckMainRIFF, MMIO_FINDLIST) != 0) {
	WARN("Can't find 'movi' list\n");
	return FALSE;
    }

    wma->dwPlayableVideoFrames = wma->mah.dwTotalFrames;
    wma->lpVideoIndex = calloc(wma->dwPlayableVideoFrames, sizeof(struct MMIOPos));
    if (!wma->lpVideoIndex) {
	WARN("Can't alloc video index array\n");
	return FALSE;
    }
    wma->dwPlayableAudioBlocks = 0;
    wma->lpAudioIndex = NULL;

    alb.numAudioBlocks = alb.numVideoFrames = 0;
    alb.inVideoSize = alb.inAudioSize = 0;
    alb.numAudioAllocated = 0;

    while (mmioDescend(wma->hFile, &mmckInfo, &mmckList, 0) == 0) {
	if (mmckInfo.fccType == listtypeAVIRECORD) {
	    MMCKINFO	tmp;

	    while (mmioDescend(wma->hFile, &tmp, &mmckInfo, 0) == 0) {
		MCIAVI_AddFrame(wma, &tmp, &alb);
		mmioAscend(wma->hFile, &tmp, 0);
	    }
	} else {
	    MCIAVI_AddFrame(wma, &mmckInfo, &alb);
	}

	mmioAscend(wma->hFile, &mmckInfo, 0);
    }

#ifdef __REACTOS__
    /* Empty file */
    if (alb.numVideoFrames == 0) {
        WARN("NumVideoFrames: %u, Empty or possibly corrupt video file");
        return FALSE;
    }
#endif

    if (alb.numVideoFrames != wma->dwPlayableVideoFrames) {
	WARN("AVI header says %ld frames, we found %ld video frames, reducing playable frames\n",
	     wma->dwPlayableVideoFrames, alb.numVideoFrames);
	wma->dwPlayableVideoFrames = alb.numVideoFrames;
    }
    wma->dwPlayableAudioBlocks = alb.numAudioBlocks;

    if (alb.inVideoSize > wma->ash_video.dwSuggestedBufferSize) {
	WARN("inVideoSize=%ld suggestedSize=%ld\n", alb.inVideoSize, wma->ash_video.dwSuggestedBufferSize);
	wma->ash_video.dwSuggestedBufferSize = alb.inVideoSize;
    }
    if (alb.inAudioSize > wma->ash_audio.dwSuggestedBufferSize) {
	WARN("inAudioSize=%ld suggestedSize=%ld\n", alb.inAudioSize, wma->ash_audio.dwSuggestedBufferSize);
	wma->ash_audio.dwSuggestedBufferSize = alb.inAudioSize;
    }

    wma->indata = malloc(wma->ash_video.dwSuggestedBufferSize);
    if (!wma->indata) {
	WARN("Can't alloc input buffer\n");
	return FALSE;
    }

    return TRUE;
}

BOOL    MCIAVI_OpenVideo(WINE_MCIAVI* wma)
{
    HDC hDC;
    DWORD	outSize;
    FOURCC	fcc = wma->ash_video.fccHandler;

    TRACE("fcc %4.4s\n", (LPSTR)&fcc);

    wma->dwCachedFrame = -1;

    /* get the right handle */
    if (fcc == mmioFOURCC('C','R','A','M')) fcc = mmioFOURCC('M','S','V','C');

    /* try to get a decompressor for that type */
    wma->hic = ICLocate(ICTYPE_VIDEO, fcc, wma->inbih, NULL, ICMODE_DECOMPRESS);
    if (!wma->hic) {
        /* check for builtin DIB compressions */
        fcc = wma->inbih->biCompression;
        if ((fcc == mmioFOURCC('D','I','B',' ')) ||
            (fcc == mmioFOURCC('R','L','E',' ')) ||
            (fcc == BI_RGB) || (fcc == BI_RLE8) ||
            (fcc == BI_RLE4) || (fcc == BI_BITFIELDS))
            goto paint_frame;

	WARN("Can't locate codec for the file\n");
	return FALSE;
    }

    outSize = sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD);

    wma->outbih = malloc(outSize);
    if (!wma->outbih) {
	WARN("Can't alloc output BIH\n");
	return FALSE;
    }
    if (!ICGetDisplayFormat(wma->hic, wma->inbih, wma->outbih, 0, 0, 0)) {
	WARN("Can't open decompressor\n");
	return FALSE;
    }

    TRACE("bih.biSize=%ld\n", 		wma->outbih->biSize);
    TRACE("bih.biWidth=%ld\n", 		wma->outbih->biWidth);
    TRACE("bih.biHeight=%ld\n", 	wma->outbih->biHeight);
    TRACE("bih.biPlanes=%d\n", 		wma->outbih->biPlanes);
    TRACE("bih.biBitCount=%d\n", 	wma->outbih->biBitCount);
    TRACE("bih.biCompression=%lx\n", 	wma->outbih->biCompression);
    TRACE("bih.biSizeImage=%ld\n", 	wma->outbih->biSizeImage);
    TRACE("bih.biXPelsPerMeter=%ld\n", 	wma->outbih->biXPelsPerMeter);
    TRACE("bih.biYPelsPerMeter=%ld\n", 	wma->outbih->biYPelsPerMeter);
    TRACE("bih.biClrUsed=%ld\n", 	wma->outbih->biClrUsed);
    TRACE("bih.biClrImportant=%ld\n", 	wma->outbih->biClrImportant);

    wma->outdata = malloc(wma->outbih->biSizeImage);
    if (!wma->outdata) {
	WARN("Can't alloc output buffer\n");
	return FALSE;
    }

    if (ICSendMessage(wma->hic, ICM_DECOMPRESS_BEGIN,
		      (DWORD_PTR)wma->inbih, (DWORD_PTR)wma->outbih) != ICERR_OK) {
	WARN("Can't begin decompression\n");
	return FALSE;
    }

paint_frame:
    if (!(wma->hdd = DrawDibOpen()))
    {
	ERR("DrawDibOpen() failed.\n");
	free(wma->outdata);
	wma->outdata = NULL;
	return FALSE;
    }

    hDC = wma->hWndPaint ? GetDC(wma->hWndPaint) : 0;
    if (hDC)
    {
        MCIAVI_PaintFrame(wma, hDC);
        ReleaseDC(wma->hWndPaint, hDC);
    }
    return TRUE;
}

static void CALLBACK MCIAVI_waveCallback(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                                        DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
    WINE_MCIAVI *wma = MCIAVI_mciGetOpenDev(dwInstance);

    if (!wma) return;

    EnterCriticalSection(&wma->cs);

    switch (uMsg) {
    case WOM_OPEN:
    case WOM_CLOSE:
	break;
    case WOM_DONE:
	InterlockedIncrement(&wma->dwEventCount);
	TRACE("Returning waveHdr=%Ix\n", dwParam1);
	SetEvent(wma->hEvent);
	break;
    default:
	ERR("Unknown uMsg=%d\n", uMsg);
    }

    LeaveCriticalSection(&wma->cs);
}

DWORD MCIAVI_OpenAudio(WINE_MCIAVI* wma, unsigned* nHdr, LPWAVEHDR* pWaveHdr)
{
    DWORD	dwRet;
    LPWAVEHDR	waveHdr;
    unsigned	i;

    dwRet = waveOutOpen((HWAVEOUT *)&wma->hWave, WAVE_MAPPER, wma->lpWaveFormat,
                       (DWORD_PTR)MCIAVI_waveCallback, wma->wDevID, CALLBACK_FUNCTION);
    if (dwRet != 0) {
	TRACE("Can't open low level audio device %ld\n", dwRet);
	dwRet = MCIERR_DEVICE_OPEN;
	wma->hWave = 0;
	goto cleanUp;
    }

    /* FIXME: should set up a heuristic to compute the number of wave headers
     * to be used...
     */
    *nHdr = 7;
    waveHdr = calloc(*nHdr, sizeof(WAVEHDR) + wma->ash_audio.dwSuggestedBufferSize);
    if (!waveHdr) {
	TRACE("Can't alloc wave headers\n");
	dwRet = MCIERR_DEVICE_OPEN;
	goto cleanUp;
    }

    for (i = 0; i < *nHdr; i++) {
	/* other fields are zero:ed on allocation */
	waveHdr[i].lpData = (char*)waveHdr +
	    *nHdr * sizeof(WAVEHDR) + i * wma->ash_audio.dwSuggestedBufferSize;
	waveHdr[i].dwBufferLength = wma->ash_audio.dwSuggestedBufferSize;
	if (waveOutPrepareHeader(wma->hWave, &waveHdr[i], sizeof(WAVEHDR))) {
	    dwRet = MCIERR_INTERNAL;
	    goto cleanUp;
	}
    }

    if (wma->dwCurrVideoFrame != 0 && wma->lpWaveFormat) {
	FIXME("Should recompute dwCurrAudioBlock, except unsynchronized sound & video\n");
    }
    wma->dwCurrAudioBlock = 0;

    wma->hEvent = CreateEventW(NULL, FALSE, FALSE, NULL);
    wma->dwEventCount = *nHdr - 1;
    *pWaveHdr = waveHdr;
 cleanUp:
    return dwRet;
}

void MCIAVI_PlayAudioBlocks(WINE_MCIAVI* wma, unsigned nHdr, LPWAVEHDR waveHdr)
{
    if (!wma->lpAudioIndex) 
        return;
    TRACE("%ld (ec=%lu)\n", wma->lpAudioIndex[wma->dwCurrAudioBlock].dwOffset, wma->dwEventCount);

    /* push as many blocks as possible => audio gets priority */
    while (wma->dwStatus != MCI_MODE_STOP && wma->dwStatus != MCI_MODE_NOT_READY &&
	   wma->dwCurrAudioBlock < wma->dwPlayableAudioBlocks) {
	unsigned	whidx = wma->dwCurrAudioBlock % nHdr;

	ResetEvent(wma->hEvent);
	if (InterlockedDecrement(&wma->dwEventCount) < 0 ||
	    !wma->lpAudioIndex[wma->dwCurrAudioBlock].dwOffset)
        {
            InterlockedIncrement(&wma->dwEventCount);
	    break;
        }

	mmioSeek(wma->hFile, wma->lpAudioIndex[wma->dwCurrAudioBlock].dwOffset, SEEK_SET);
	mmioRead(wma->hFile, waveHdr[whidx].lpData, wma->lpAudioIndex[wma->dwCurrAudioBlock].dwSize);

	waveHdr[whidx].dwFlags &= ~WHDR_DONE;
	waveHdr[whidx].dwBufferLength = wma->lpAudioIndex[wma->dwCurrAudioBlock].dwSize;
	waveOutWrite(wma->hWave, &waveHdr[whidx], sizeof(WAVEHDR));
	wma->dwCurrAudioBlock++;
    }
}

double MCIAVI_PaintFrame(WINE_MCIAVI* wma, HDC hDC)
{
    void* 		pBitmapData;
    LPBITMAPINFO	pBitmapInfo;
    DWORD		frame;

    if (!hDC || !wma->inbih)
	return 0;

    if (wma->dwCurrVideoFrame >= wma->dwPlayableVideoFrames)
        frame = wma->dwPlayableVideoFrames - 1;
    else
        frame = wma->dwCurrVideoFrame;

    TRACE("Painting frame %lu (cached %lu)\n", frame, wma->dwCachedFrame);

    if (frame != wma->dwCachedFrame)
    {
#ifdef __REACTOS__
        if (wma->dwCurrVideoFrame >= wma->dwPlayableVideoFrames) {
            ERR("Invalid frame requested. Current : %u Total Playable %u\n", wma->dwCurrVideoFrame, wma->dwPlayableVideoFrames);
            return 0;
        }
#endif
        if (!wma->lpVideoIndex[frame].dwOffset)
	    return 0;

        if (wma->lpVideoIndex[frame].dwSize)
        {
            mmioSeek(wma->hFile, wma->lpVideoIndex[frame].dwOffset, SEEK_SET);
            mmioRead(wma->hFile, wma->indata, wma->lpVideoIndex[frame].dwSize);

            wma->inbih->biSizeImage = wma->lpVideoIndex[frame].dwSize;

            if (wma->hic && ICDecompress(wma->hic, 0, wma->inbih, wma->indata,
                                         wma->outbih, wma->outdata) != ICERR_OK)
            {
                WARN("Decompression error\n");
                return 0;
            }
        }

        wma->dwCachedFrame = frame;
    }

    if (wma->hic) {
        pBitmapData = wma->outdata;
        pBitmapInfo = (LPBITMAPINFO)wma->outbih;
    } else {
        pBitmapData = wma->indata;
        pBitmapInfo = (LPBITMAPINFO)wma->inbih;
    }

    DrawDibDraw(wma->hdd, hDC, wma->dest.left, wma->dest.top, wma->dest.right - wma->dest.left, wma->dest.bottom - wma->dest.top,
                &pBitmapInfo->bmiHeader, pBitmapData,
                wma->source.left, wma->source.top, wma->source.right - wma->source.left, wma->source.bottom - wma->source.top, 0);

    return (wma->ash_video.dwScale / (double)wma->ash_video.dwRate) * 1000000;
}
