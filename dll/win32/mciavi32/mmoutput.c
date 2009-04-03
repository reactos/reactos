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

    TRACE("ash.fccType='%c%c%c%c'\n", 		LOBYTE(LOWORD(wma->ash_audio.fccType)),
	                                        HIBYTE(LOWORD(wma->ash_audio.fccType)),
	                                        LOBYTE(HIWORD(wma->ash_audio.fccType)),
	                                        HIBYTE(HIWORD(wma->ash_audio.fccType)));
    if (wma->ash_audio.fccHandler) /* not all streams specify a handler */
        TRACE("ash.fccHandler='%c%c%c%c'\n",	LOBYTE(LOWORD(wma->ash_audio.fccHandler)),
	                                        HIBYTE(LOWORD(wma->ash_audio.fccHandler)),
	                                        LOBYTE(HIWORD(wma->ash_audio.fccHandler)),
	                                        HIBYTE(HIWORD(wma->ash_audio.fccHandler)));
    else
        TRACE("ash.fccHandler=0, no handler specified\n");
    TRACE("ash.dwFlags=%d\n", 			wma->ash_audio.dwFlags);
    TRACE("ash.wPriority=%d\n", 		wma->ash_audio.wPriority);
    TRACE("ash.wLanguage=%d\n", 		wma->ash_audio.wLanguage);
    TRACE("ash.dwInitialFrames=%d\n", 		wma->ash_audio.dwInitialFrames);
    TRACE("ash.dwScale=%d\n", 			wma->ash_audio.dwScale);
    TRACE("ash.dwRate=%d\n", 			wma->ash_audio.dwRate);
    TRACE("ash.dwStart=%d\n", 			wma->ash_audio.dwStart);
    TRACE("ash.dwLength=%d\n", 		wma->ash_audio.dwLength);
    TRACE("ash.dwSuggestedBufferSize=%d\n", 	wma->ash_audio.dwSuggestedBufferSize);
    TRACE("ash.dwQuality=%d\n", 		wma->ash_audio.dwQuality);
    TRACE("ash.dwSampleSize=%d\n", 		wma->ash_audio.dwSampleSize);
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
	WARN("Size of strf chunk (%d) < audio format struct\n", mmckInfo.cksize);
	return FALSE;
    }
    wma->lpWaveFormat = HeapAlloc(GetProcessHeap(), 0, mmckInfo.cksize);
    if (!wma->lpWaveFormat) {
	WARN("Can't alloc WaveFormat\n");
	return FALSE;
    }

    mmioRead(wma->hFile, (LPSTR)wma->lpWaveFormat, mmckInfo.cksize);

    TRACE("waveFormat.wFormatTag=%d\n",		wma->lpWaveFormat->wFormatTag);
    TRACE("waveFormat.nChannels=%d\n", 		wma->lpWaveFormat->nChannels);
    TRACE("waveFormat.nSamplesPerSec=%d\n",	wma->lpWaveFormat->nSamplesPerSec);
    TRACE("waveFormat.nAvgBytesPerSec=%d\n",	wma->lpWaveFormat->nAvgBytesPerSec);
    TRACE("waveFormat.nBlockAlign=%d\n",	wma->lpWaveFormat->nBlockAlign);
    TRACE("waveFormat.wBitsPerSample=%d\n",	wma->lpWaveFormat->wBitsPerSample);
    if (mmckInfo.cksize >= sizeof(WAVEFORMATEX))
	TRACE("waveFormat.cbSize=%d\n", 		wma->lpWaveFormat->cbSize);

    return TRUE;
}

static BOOL MCIAVI_GetInfoVideo(WINE_MCIAVI* wma, const MMCKINFO* mmckList, MMCKINFO* mmckStream)
{
    MMCKINFO	mmckInfo;

    TRACE("ash.fccType='%c%c%c%c'\n", 		LOBYTE(LOWORD(wma->ash_video.fccType)),
	                                        HIBYTE(LOWORD(wma->ash_video.fccType)),
	                                        LOBYTE(HIWORD(wma->ash_video.fccType)),
	                                        HIBYTE(HIWORD(wma->ash_video.fccType)));
    TRACE("ash.fccHandler='%c%c%c%c'\n",	LOBYTE(LOWORD(wma->ash_video.fccHandler)),
	                                        HIBYTE(LOWORD(wma->ash_video.fccHandler)),
	                                        LOBYTE(HIWORD(wma->ash_video.fccHandler)),
	                                        HIBYTE(HIWORD(wma->ash_video.fccHandler)));
    TRACE("ash.dwFlags=%d\n", 			wma->ash_video.dwFlags);
    TRACE("ash.wPriority=%d\n", 		wma->ash_video.wPriority);
    TRACE("ash.wLanguage=%d\n", 		wma->ash_video.wLanguage);
    TRACE("ash.dwInitialFrames=%d\n", 		wma->ash_video.dwInitialFrames);
    TRACE("ash.dwScale=%d\n", 			wma->ash_video.dwScale);
    TRACE("ash.dwRate=%d\n", 			wma->ash_video.dwRate);
    TRACE("ash.dwStart=%d\n", 			wma->ash_video.dwStart);
    TRACE("ash.dwLength=%d\n", 		wma->ash_video.dwLength);
    TRACE("ash.dwSuggestedBufferSize=%d\n", 	wma->ash_video.dwSuggestedBufferSize);
    TRACE("ash.dwQuality=%d\n", 		wma->ash_video.dwQuality);
    TRACE("ash.dwSampleSize=%d\n", 		wma->ash_video.dwSampleSize);
    TRACE("ash.rcFrame=(%d,%d,%d,%d)\n", 	wma->ash_video.rcFrame.top, wma->ash_video.rcFrame.left,
	  wma->ash_video.rcFrame.bottom, wma->ash_video.rcFrame.right);

    /* rewind to the start of the stream */
    mmioAscend(wma->hFile, mmckStream, 0);

    mmckInfo.ckid = ckidSTREAMFORMAT;
    if (mmioDescend(wma->hFile, &mmckInfo, mmckList, MMIO_FINDCHUNK) != 0) {
       WARN("Can't find 'strf' chunk\n");
	return FALSE;
    }

    wma->inbih = HeapAlloc(GetProcessHeap(), 0, mmckInfo.cksize);
    if (!wma->inbih) {
	WARN("Can't alloc input BIH\n");
	return FALSE;
    }

    mmioRead(wma->hFile, (LPSTR)wma->inbih, mmckInfo.cksize);

    TRACE("bih.biSize=%d\n", 		wma->inbih->biSize);
    TRACE("bih.biWidth=%d\n", 		wma->inbih->biWidth);
    TRACE("bih.biHeight=%d\n", 	wma->inbih->biHeight);
    TRACE("bih.biPlanes=%d\n", 		wma->inbih->biPlanes);
    TRACE("bih.biBitCount=%d\n", 	wma->inbih->biBitCount);
    TRACE("bih.biCompression=%x\n", 	wma->inbih->biCompression);
    TRACE("bih.biSizeImage=%d\n", 	wma->inbih->biSizeImage);
    TRACE("bih.biXPelsPerMeter=%d\n", 	wma->inbih->biXPelsPerMeter);
    TRACE("bih.biYPelsPerMeter=%d\n", 	wma->inbih->biYPelsPerMeter);
    TRACE("bih.biClrUsed=%d\n", 	wma->inbih->biClrUsed);
    TRACE("bih.biClrImportant=%d\n", 	wma->inbih->biClrImportant);

    wma->source.left = 0;
    wma->source.top = 0;
    wma->source.right = wma->inbih->biWidth;
    wma->source.bottom = wma->inbih->biHeight;

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

    TRACE("ckid %4.4s (stream #%d)\n", (LPSTR)&mmck->ckid, stream_n);

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
    
    switch (twocc) {
    case cktypeDIBbits:
    case cktypeDIBcompressed:
    case cktypePALchange:
        if (stream_n != wma->video_stream_n)
        {
            TRACE("data belongs to another video stream #%d\n", stream_n);
            return FALSE;
        }

	TRACE("Adding video frame[%d]: %d bytes\n",
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
            TRACE("data belongs to another audio stream #%d\n", stream_n);
            return FALSE;
        }

	TRACE("Adding audio frame[%d]: %d bytes\n",
	      alb->numAudioBlocks, mmck->cksize);
	if (wma->lpWaveFormat) {
	    if (alb->numAudioBlocks >= alb->numAudioAllocated) {
		alb->numAudioAllocated += 32;
		if (!wma->lpAudioIndex)
		    wma->lpAudioIndex = HeapAlloc(GetProcessHeap(), 0,
						  alb->numAudioAllocated * sizeof(struct MMIOPos));
		else
		    wma->lpAudioIndex = HeapReAlloc(GetProcessHeap(), 0, wma->lpAudioIndex,
						    alb->numAudioAllocated * sizeof(struct MMIOPos));
		if (!wma->lpAudioIndex) return FALSE;
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

    TRACE("mah.dwMicroSecPerFrame=%d\n", 	wma->mah.dwMicroSecPerFrame);
    TRACE("mah.dwMaxBytesPerSec=%d\n", 	wma->mah.dwMaxBytesPerSec);
    TRACE("mah.dwPaddingGranularity=%d\n", 	wma->mah.dwPaddingGranularity);
    TRACE("mah.dwFlags=%d\n", 			wma->mah.dwFlags);
    TRACE("mah.dwTotalFrames=%d\n", 		wma->mah.dwTotalFrames);
    TRACE("mah.dwInitialFrames=%d\n", 		wma->mah.dwInitialFrames);
    TRACE("mah.dwStreams=%d\n", 		wma->mah.dwStreams);
    TRACE("mah.dwSuggestedBufferSize=%d\n",	wma->mah.dwSuggestedBufferSize);
    TRACE("mah.dwWidth=%d\n", 			wma->mah.dwWidth);
    TRACE("mah.dwHeight=%d\n", 		wma->mah.dwHeight);

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

        TRACE("Stream #%d fccType %4.4s\n", stream_n, (LPSTR)&strh.fccType);

        if (strh.fccType == streamtypeVIDEO)
        {
            TRACE("found video stream\n");
            if (wma->inbih)
                WARN("ignoring another video stream\n");
            else
            {
                wma->ash_audio = strh;

                if (!MCIAVI_GetInfoVideo(wma, &mmckList, &mmckStream))
                    return FALSE;
                wma->video_stream_n = stream_n;
            }
        }
        else if (strh.fccType == streamtypeAUDIO)
        {
            TRACE("found audio stream\n");
            if (wma->lpWaveFormat)
                WARN("ignoring another audio stream\n");
            else
            {
                wma->ash_video = strh;

                if (!MCIAVI_GetInfoAudio(wma, &mmckList, &mmckStream))
                    return FALSE;
                wma->audio_stream_n = stream_n;
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
    wma->lpVideoIndex = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
				  wma->dwPlayableVideoFrames * sizeof(struct MMIOPos));
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
    if (alb.numVideoFrames != wma->dwPlayableVideoFrames) {
	WARN("Found %d video frames (/%d), reducing playable frames\n",
	     alb.numVideoFrames, wma->dwPlayableVideoFrames);
	wma->dwPlayableVideoFrames = alb.numVideoFrames;
    }
    wma->dwPlayableAudioBlocks = alb.numAudioBlocks;

    if (alb.inVideoSize > wma->ash_video.dwSuggestedBufferSize) {
	WARN("inVideoSize=%d suggestedSize=%d\n", alb.inVideoSize, wma->ash_video.dwSuggestedBufferSize);
	wma->ash_video.dwSuggestedBufferSize = alb.inVideoSize;
    }
    if (alb.inAudioSize > wma->ash_audio.dwSuggestedBufferSize) {
	WARN("inAudioSize=%d suggestedSize=%d\n", alb.inAudioSize, wma->ash_audio.dwSuggestedBufferSize);
	wma->ash_audio.dwSuggestedBufferSize = alb.inAudioSize;
    }

    wma->indata = HeapAlloc(GetProcessHeap(), 0, wma->ash_video.dwSuggestedBufferSize);
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

    wma->outbih = HeapAlloc(GetProcessHeap(), 0, outSize);
    if (!wma->outbih) {
	WARN("Can't alloc output BIH\n");
	return FALSE;
    }
    if (!ICGetDisplayFormat(wma->hic, wma->inbih, wma->outbih, 0, 0, 0)) {
	WARN("Can't open decompressor\n");
	return FALSE;
    }

    TRACE("bih.biSize=%d\n", 		wma->outbih->biSize);
    TRACE("bih.biWidth=%d\n", 		wma->outbih->biWidth);
    TRACE("bih.biHeight=%d\n", 	wma->outbih->biHeight);
    TRACE("bih.biPlanes=%d\n", 		wma->outbih->biPlanes);
    TRACE("bih.biBitCount=%d\n", 	wma->outbih->biBitCount);
    TRACE("bih.biCompression=%x\n", 	wma->outbih->biCompression);
    TRACE("bih.biSizeImage=%d\n", 	wma->outbih->biSizeImage);
    TRACE("bih.biXPelsPerMeter=%d\n", 	wma->outbih->biXPelsPerMeter);
    TRACE("bih.biYPelsPerMeter=%d\n", 	wma->outbih->biYPelsPerMeter);
    TRACE("bih.biClrUsed=%d\n", 	wma->outbih->biClrUsed);
    TRACE("bih.biClrImportant=%d\n", 	wma->outbih->biClrImportant);

    wma->outdata = HeapAlloc(GetProcessHeap(), 0, wma->outbih->biSizeImage);
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
	TRACE("Returning waveHdr=%lx\n", dwParam1);
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
	TRACE("Can't open low level audio device %d\n", dwRet);
	dwRet = MCIERR_DEVICE_OPEN;
	wma->hWave = 0;
	goto cleanUp;
    }

    /* FIXME: should set up a heuristic to compute the number of wave headers
     * to be used...
     */
    *nHdr = 7;
    waveHdr = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,
			*nHdr * (sizeof(WAVEHDR) + wma->ash_audio.dwSuggestedBufferSize));
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
    TRACE("%d (ec=%u)\n", wma->lpAudioIndex[wma->dwCurrAudioBlock].dwOffset, wma->dwEventCount);

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

LRESULT MCIAVI_PaintFrame(WINE_MCIAVI* wma, HDC hDC)
{
    void* 		pBitmapData;
    LPBITMAPINFO	pBitmapInfo;

    if (!hDC || !wma->inbih)
	return TRUE;

    TRACE("Painting frame %u (cached %u)\n", wma->dwCurrVideoFrame, wma->dwCachedFrame);

    if (wma->dwCurrVideoFrame != wma->dwCachedFrame)
    {
        if (!wma->lpVideoIndex[wma->dwCurrVideoFrame].dwOffset)
	    return FALSE;

        if (wma->lpVideoIndex[wma->dwCurrVideoFrame].dwSize)
        {
            mmioSeek(wma->hFile, wma->lpVideoIndex[wma->dwCurrVideoFrame].dwOffset, SEEK_SET);
            mmioRead(wma->hFile, wma->indata, wma->lpVideoIndex[wma->dwCurrVideoFrame].dwSize);

            wma->inbih->biSizeImage = wma->lpVideoIndex[wma->dwCurrVideoFrame].dwSize;

            if (wma->hic && ICDecompress(wma->hic, 0, wma->inbih, wma->indata,
                                         wma->outbih, wma->outdata) != ICERR_OK)
            {
                WARN("Decompression error\n");
                return FALSE;
            }
        }

        wma->dwCachedFrame = wma->dwCurrVideoFrame;
    }

    if (wma->hic) {
        pBitmapData = wma->outdata;
        pBitmapInfo = (LPBITMAPINFO)wma->outbih;
    } else {
        pBitmapData = wma->indata;
        pBitmapInfo = (LPBITMAPINFO)wma->inbih;
    }

    StretchDIBits(hDC,
                  wma->dest.left, wma->dest.top,
                  wma->dest.right - wma->dest.left, wma->dest.bottom - wma->dest.top,
                  wma->source.left, wma->source.top,
                  wma->source.right - wma->source.left, wma->source.bottom - wma->source.top,
                  pBitmapData, pBitmapInfo, DIB_RGB_COLORS, SRCCOPY);

    return TRUE;
}
