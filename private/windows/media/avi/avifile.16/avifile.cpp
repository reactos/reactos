/****************************************************************************
 *
 *  AVIFILE.C
 *
 *  routines for reading Standard AVI files
 *
 *  Copyright (c) 1992 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <storage.h>
#include <compman.h>
#include <compobj.h>
#include <avifmt.h>
#include "avifile.h"
#include "avifilei.h"
#include "avifile.rc"

#include "debug.h"

#undef GlobalFreePtr        // fix for C6
#define GlobalFreePtr(p)    GlobalFree(GlobalPtrHandle(p))

extern "C" STDAPI CalculateFileDataRate(PAVIFILE pf, LONG FAR *plMaxBytesPerSec);

//#undef StreamFromFOURCC
//#define StreamFromFOURCC(fcc) (UINT)(HIBYTE(LOWORD(fcc)) - (BYTE)'0')

BOOL AddToIndex(CAVIFile FAR * pfile, DWORD ckid, DWORD cksize, LONG off, DWORD dwFlags);

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

EXTERN_C void DecodeRle(LPBITMAPINFOHEADER lpbi, BYTE _huge *pb, BYTE _huge *prle);
EXTERN_C HINSTANCE ghMod;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#define comptypeNONE            mmioFOURCC('N','O','N','E')
#define comptypeRLE0            mmioFOURCC('R','L','E','0')
#define comptypeRLE             mmioFOURCC('R','L','E',' ')

#define WIDTHBYTES(i)       ((UINT)((i+31)&(~31))/8)
#define DIBWIDTHBYTES(lpbi) (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)(lpbi)->biBitCount)

LONG    lBufferSize = 0;
int     nBuffers = 0;

#define ckidSTREAMNAME         mmioFOURCC('s', 't', 'r', 'n')

/***************************************************************************
 ***************************************************************************/

EXTERN_C LONG FAR PASCAL shfileReadProc(HANDLE hsf, LONG lSeek, LONG lRead, LPVOID lpBuffer)
{
    if (shfileSeek((HSHFILE)hsf, lSeek, SEEK_SET) == -1)
        return -1;

    if (shfileRead((HSHFILE)hsf, (HPSTR)lpBuffer, lRead) != lRead)
        return -1;

    return lRead;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#define INDEX_WRITE_SIZE    32l*1024
#define INDEX_READ_SIZE     32l*1024

static BOOL WriteOutIndex(CAVIFile FAR *pfile, DWORD dwOffsetMovie)
{
    MMCKINFO ck;
    PAVIINDEXENTRY pIndex;
    LONG cnt = INDEX_WRITE_SIZE / sizeof(*pIndex);
    LONG l;
    BOOL f=FALSE;
#ifdef DEBUG
    DWORD time;
#endif

    /*
    ** Now write index out!
    */
    ck.ckid = ckidAVINEWINDEX;
    ck.cksize = sizeof(AVIINDEXENTRY) * pfile->px->nIndex;
    
    if (shfileCreateChunk(pfile->hshfile, &ck, 0))
        goto exit;

    DPF("Writing Index", time=timeGetTime());

    pIndex = (PAVIINDEXENTRY)GlobalAllocPtr(GHND,INDEX_WRITE_SIZE);

    if (pIndex == NULL)
        goto exit;

    for (l=0; l < pfile->px->nIndex; ) {

        cnt = IndexGetFileIndex(pfile->px, l, cnt, pIndex, -(LONG)dwOffsetMovie);

        if (cnt == 0)
            break;

        if (shfileWrite(pfile->hshfile, (HPSTR)pIndex,
             cnt * sizeof(AVIINDEXENTRY)) != cnt * sizeof(AVIINDEXENTRY))
             goto exit;

        l += cnt;

        DPF("!.");
    }

    DPF("!Done (%ldms)\n", timeGetTime()-time);

    if (shfileAscend(pfile->hshfile, &ck, 0))
        goto exit;

    f = TRUE;

exit:
    if (pIndex)
        GlobalFreePtr(pIndex);
    
    return f;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static BOOL ReadInIndex(CAVIFile FAR *pfile, DWORD size, DWORD dwOffsetMovie, BOOL fRle)
{
    PAVIINDEXENTRY pIndex;
    LONG cnt;
    LONG lIndexAdjust;
    BOOL f = FALSE;
#ifdef DEBUG
    DWORD time;
#endif

    pIndex = (PAVIINDEXENTRY)GlobalAllocPtr(GHND,INDEX_READ_SIZE);

    if (pIndex == NULL)
        goto exit;

    pfile->px = IndexCreate();

    if (pfile->px == 0)
        goto exit;

    DPF("Reading index.", time = timeGetTime());

    if (pfile->avihdr.dwFlags & AVIF_MUSTUSEINDEX)
        lIndexAdjust = dwOffsetMovie;
    else
        lIndexAdjust = -1;      // set when we read first index entry.

    while (size > 0) {

        cnt = min(INDEX_READ_SIZE, size);

        if (shfileRead(pfile->hshfile,(HPSTR)pIndex,cnt) != cnt)
            goto exit;

        size -= cnt;
        cnt /= sizeof(AVIINDEXENTRY);

        //
        // fix up the index
        //
        if (lIndexAdjust == -1) {
            lIndexAdjust = (LONG)(dwOffsetMovie + sizeof(DWORD)) -
                (LONG)pIndex->dwChunkOffset;
        }

        pfile->px = IndexAddFileIndex(pfile->px, pIndex, cnt, lIndexAdjust, fRle);

        if (pfile->px == NULL)
            goto exit;

        DPF("!.");
    }

    DPF("!Done (%ldms)\n", timeGetTime() - time);

    f = TRUE;

exit:
    if (pIndex)
        GlobalFreePtr(pIndex);

    return f;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT SaveChanges(CAVIFile FAR * pfile, BOOL fRelease)
{
    CAVIStream FAR *            pavi;
    int                         stream;
    MMCKINFO                    ck;
    MMCKINFO                    ckRIFF;
    MMCKINFO                    ckLIST;
    MMCKINFO                    ckStream;
    LONG                        lCur;
    HRESULT                     hr = AVIERR_OK;
    AVIStreamHeader             strhdr;

    // Clean up interleaving
    if (pfile->fInRecord) {
        if (pfile->px->nIndex > pfile->lRecordIndex + 1) {
	    AVIFileEndRecord((PAVIFILE) pfile);
	}

	// back out of last record....
        --pfile->px->nIndex;
	pfile->lWriteLoc -= 3 * sizeof(DWORD);
	shfileSeek(pfile->hshfile, pfile->lWriteLoc, SEEK_SET);
	pfile->fInRecord = FALSE;
    }
    
    // Go back and write out the header

    lCur = shfileSeek(pfile->hshfile, 0, SEEK_CUR);
    shfileSeek(pfile->hshfile, 0, SEEK_SET);

    /* Create RIFF chunk */
    ckRIFF.cksize = 0;
    ckRIFF.fccType = formtypeAVI;
    if (shfileCreateChunk(pfile->hshfile, &ckRIFF, MMIO_CREATERIFF)) {
	goto FileError;
    }

    /* Create header list */
    ckLIST.cksize = 0;
    ckLIST.fccType = listtypeAVIHEADER;
    if (shfileCreateChunk(pfile->hshfile, &ckLIST, MMIO_CREATELIST)) {
	goto FileError;
    }

    /* Create AVI header chunk */
    ck.cksize = sizeof(pfile->avihdr);
    ck.ckid = ckidAVIMAINHDR;
    if (shfileCreateChunk(pfile->hshfile, &ck, 0)) {
	goto FileError;
    }

    CalculateFileDataRate(&pfile->m_AVIFile, (LONG FAR *) &pfile->avihdr.dwMaxBytesPerSec);
    
    /* Write AVI header info */
    if (shfileWrite(pfile->hshfile,
		  (LPSTR)&pfile->avihdr,
		  sizeof(pfile->avihdr)) != sizeof(pfile->avihdr)) {
	goto FileError;
    }

    if (shfileAscend(pfile->hshfile, &ck, 0)) {
	goto FileError;
    }


#if 0
    for (l = 0;
	 l < muldiv32(pfile->avihdr.dwTotalFrames,
			     pfile->avihdr.dwMicroSecPerFrame,
			     1000000L);
         l++) {

	     for (stream = 0; stream < (int) pfile->avihdr.dwStreams; stream++) {
             }
    }
#endif
    
    for (stream = 0; stream < (int) pfile->avihdr.dwStreams; stream++) {
	pavi = pfile->ps[stream];
	
	/* Create stream header list */
	ckStream.cksize = 0;
	ckStream.fccType = listtypeSTREAMHEADER;
	if (shfileCreateChunk(pfile->hshfile,&ckStream,MMIO_CREATELIST)) {
	    goto FileError;
	}

	ck.ckid = ckidSTREAMHEADER;
	if (shfileCreateChunk(pfile->hshfile, &ck, 0)) {
	    goto FileError;
	}


	// !!! Make an AVIStreamHeader from the AVISTREAMINFO
	strhdr.fccType =                pavi->avistream.fccType;
	strhdr.fccHandler =             pavi->avistream.fccHandler;
	strhdr.dwFlags =                pavi->avistream.dwFlags;
	strhdr.wPriority =              pavi->avistream.wPriority;
	strhdr.wLanguage =              pavi->avistream.wLanguage;
	strhdr.dwRate =                 pavi->avistream.dwRate;
	strhdr.dwScale =                pavi->avistream.dwScale;
	strhdr.dwStart =                pavi->avistream.dwStart;
	strhdr.dwLength =               pavi->avistream.dwLength;
	strhdr.dwSuggestedBufferSize =  pavi->avistream.dwSuggestedBufferSize;
	strhdr.dwQuality =              pavi->avistream.dwQuality;
	strhdr.dwSampleSize =           pavi->avistream.dwSampleSize;
	strhdr.rcFrame =                pavi->avistream.rcFrame;
	strhdr.dwInitialFrames =        pavi->avistream.dwInitialFrames;
	
	if (shfileWrite(pfile->hshfile, (HPSTR) &strhdr, sizeof(strhdr)) !=
							    sizeof(strhdr)) {
	    goto FileError;
	}

	if (shfileAscend(pfile->hshfile, &ck, 0)) {
	    goto FileError;
	}


	ck.cksize = pavi->cbFormat;
	ck.ckid = ckidSTREAMFORMAT;
	
	if (shfileCreateChunk(pfile->hshfile, &ck, 0))
	    goto FileError;

	if (shfileWrite(pfile->hshfile, (HPSTR) pavi->lpFormat, ck.cksize) !=
			(LONG) ck.cksize)
	    goto FileError;

	if (shfileAscend(pfile->hshfile, &ck, 0))
	    goto FileError;

	if (pavi->avistream.szName[0]) {
	    ck.cksize = lstrlen(pavi->avistream.szName) + 1;
	    ck.ckid = ckidSTREAMNAME;

	    if (shfileCreateChunk(pfile->hshfile, &ck, 0))
		goto FileError;

	    if (shfileWrite(pfile->hshfile, (HPSTR) pavi->avistream.szName, ck.cksize) !=
			    (LONG) ck.cksize)
		goto FileError;

	    if (shfileAscend(pfile->hshfile, &ck, 0))
		goto FileError;
	}
	
	if (pavi->extra.cb) {
	    DPF2("Writing %ld bytes of extra stream data.\n", pavi->extra.cb);
	    if (shfileWrite(pfile->hshfile, (HPSTR) pavi->extra.lp, pavi->extra.cb) !=
			(LONG) pavi->extra.cb)
		goto FileError;
	}

	/* Ascend out of stream's header */
	if (shfileAscend(pfile->hshfile, &ckStream, 0)) {
	    goto FileError;
	}
    }

    /* ascend from the Header list */
    if (shfileAscend(pfile->hshfile, &ckLIST, 0)) {
	goto FileError;
    }

    /* Pad this header out so that the real data will start on a 2K 
    ** boundary by writing a JUNK chunk.
    */
    ck.ckid = ckidAVIPADDING;
    if (shfileCreateChunk(pfile->hshfile,&ck,0)) {
	goto FileError;
    }

    DPF("Data list start = %ld, current pos = %ld\n", pfile->lDataListStart, shfileSeek(pfile->hshfile, 0, SEEK_CUR));
      
    if (shfileSeek(pfile->hshfile, 0, SEEK_CUR) > pfile->lDataListStart) {
	// !!! Ack: we didn't leave enough space for the header.
	// !!! How can we avoid this?
	goto FileError;
    }
    
    shfileSeek(pfile->hshfile, pfile->lDataListStart, SEEK_SET);

    if (shfileAscend(pfile->hshfile, &ck, 0)) {
	goto FileError;
    }

    /* Start the 'movi' list, where all of the actual data will be. */
    ckLIST.cksize = 0;
    ckLIST.fccType = listtypeAVIMOVIE;
    if (shfileCreateChunk(pfile->hshfile, &ckLIST, MMIO_CREATELIST)) {
	goto FileError;
    }

    shfileSeek(pfile->hshfile, pfile->lWriteLoc, SEEK_SET);

    if (shfileAscend(pfile->hshfile, &ckLIST, 0))
        goto FileError;

    if (!WriteOutIndex(pfile, ckLIST.dwDataOffset))
        goto FileError;

    //
    // Write out any extra data around
    //
    if (pfile->extra.cb) {
	DPF2("Writing %ld bytes of extra file data.\n", pfile->extra.cb);
	
	if (shfileWrite(pfile->hshfile,
		      (HPSTR) pfile->extra.lp,
		      pfile->extra.cb) !=
		 (LONG) pfile->extra.cb)
	    goto FileError;
    }

FinishUp:
    if (shfileAscend(pfile->hshfile, &ckRIFF, 0))
	goto FileError;

    //
    // Always flush to be sure that the data really made it to the disk....
    //
    if (shfileFlush(pfile->hshfile, 0))
	goto FileError;
    
    return ResultFromScode(AVIERR_OK);

FileError:
    hr = ResultFromScode(AVIERR_FILEWRITE);
    goto FinishUp;
}    

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CAVIFile::CUnknownImpl::Release()
{
    CAVIFile FAR * pfile = m_pAVIFile;
    CAVIStream FAR * pavi;
    int iStream;

    DPF2("File   %lx: Usage--=%lx\n", (DWORD) (LPVOID) this, m_refs - 1);
    
    uUseCount--;
    if (!--m_refs) {
	if (pfile->fDirty) {
	    ++m_refs;
            SaveChanges(pfile, TRUE);
	    --m_refs;

	    // Unfortunately, it's too late to tell about any errors....
	}

	for (iStream = 0; iStream < (int)pfile->avihdr.dwStreams; iStream++) {
	    pavi = pfile->ps[iStream];

	    if (!pavi)
		continue;
	    
	    if (pavi->lpFormat)
		GlobalFreePtr(pavi->lpFormat);

	    if (pavi->lpData)
		GlobalFreePtr(pavi->lpData);

	    if (pavi->extra.lp) {
		DPF2("Freeing %ld bytes of extra stream data.\n", pavi->extra.cb);
		GlobalFreePtr(pavi->extra.lp);
	    }

	    if (pavi->paviBase)
		AVIStreamClose(pavi->paviBase);

	    delete pavi;
	}

	if (pfile->hshfile) {
	    shfileRelease(pfile->hshfile);
	    shfileClose(pfile->hshfile, 0);
	}

        if (pfile->px)
            FreeIndex(pfile->px);

	if (pfile->extra.lp) {
	    DPF2("Freeing %ld bytes of extra file data.\n", pfile->extra.cb);
	    GlobalFreePtr(pfile->extra.lp);
	}

	if (pfile->pb)
	    EndBuffered(pfile->pb);

	pfile->hshfile = NULL;

        pfile->px = NULL;
	delete pfile;
	return 0;
    } else {
	if (pfile->hshfile)
	    shfileRelease(pfile->hshfile);
    }
    
    return m_refs;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Save(
				   LPCSTR szFile,
				   AVICOMPRESSOPTIONS FAR *lpOptions,
				   AVISAVECALLBACK lpfnCallback)
{
    CAVIFile FAR * pfile = m_pAVIFile;
    HRESULT hr = ResultFromScode(AVIERR_OK);

    if (pfile->fDirty) {
        hr = SaveChanges(pfile, FALSE);
    }
    
    return hr;
}

#define SLASH(c)     ((c) == '/' || (c) == '\\')

/*--------------------------------------------------------------+
| FileName  - return a pointer to the filename part of szPath   |
|             with no preceding path.                           |
+--------------------------------------------------------------*/
LPSTR FAR FileName(LPCSTR lszPath)
{
    LPCSTR   lszCur;

    for (lszCur = lszPath + lstrlen(lszPath); lszCur > lszPath && !SLASH(*lszCur) && *lszCur != ':';)
	lszCur = AnsiPrev(lszPath, lszCur);
    if (lszCur == lszPath)
	return (LPSTR)lszCur;
    else
	return (LPSTR)(lszCur + 1);
}

BOOL IsRectBogus(LPRECT lprc, DWORD dwFrameWidth, DWORD dwFrameHeight,
		 LPBITMAPINFOHEADER lpbi)
{
    if (IsRectEmpty(lprc))
	return TRUE;

    if (lprc->right - lprc->left > (int) dwFrameWidth)
	return TRUE;

    if (lprc->bottom - lprc->top > (int) dwFrameHeight)
	return TRUE;

    // !!!! Check that rectangle matches lpbi?

    // We've run out of things to check, so it's OK....
    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Open(LPCSTR szFile, UINT mode)
{
    CAVIFile FAR * pfile = m_pAVIFile;
    CAVIStream FAR *        pavi;
    MMCKINFO        ck;
    MMCKINFO        ckRIFF;
    MMCKINFO        ckLIST;
    MMCKINFO        ckStream;
    DWORD           dwSize;
    BOOL            fRle=FALSE;
    LONG            l;
    int             iStream;
    int             i;
    HRESULT         hr = ResultFromScode(AVIERR_OK);
    IUnknown FAR *  pUnk;
    UINT            ui;
    AVIStreamHeader strhdr;
    char            ach[20];
    char            ach2[20];
    int             iStreamNumber;
#ifdef DEBUG
    DWORD time;
#endif

    if (pfile->achFile[0])
	return ResultFromScode(-1);

    pfile->mode = mode;
    _fstrcpy(pfile->achFile, szFile);

    // Assumptions about avilib.cpp:
    // We're assuming that if CREATE is set, WRITE is set too.
    // We're assuming that we'll always see READWRITE instead of just WRITE.

    // force the share flags to the 'correct' values
    if (mode & OF_READWRITE) {
	pfile->mode = (mode & ~(MMIO_SHAREMODE)) | OF_SHARE_EXCLUSIVE;
    } else {
	pfile->mode = (mode & ~(MMIO_SHAREMODE)) | OF_SHARE_DENY_WRITE;
    }

    // try to open the actual file
    // If the first attempt fails, no system error box, please.
    ui = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    
    pfile->hshfile = shfileOpen(pfile->achFile, NULL, pfile->mode);

    if (!pfile->hshfile && ((mode & MMIO_RWMODE) == OF_READ)) {
	// if the open fails, try again without the share flags.
	pfile->mode &= ~(MMIO_SHAREMODE);
	
	pfile->hshfile = shfileOpen(pfile->achFile, NULL, pfile->mode);
    }
    SetErrorMode(ui);
    
    if (!pfile->hshfile) {
	hr = ResultFromScode(AVIERR_FILEOPEN);
	goto error;
    }

    if (mode & OF_CREATE) {
        // make a empty index.
        pfile->px = IndexCreate();

	if (pfile->px == 0)
	    goto memerror;
	    
	pfile->lWriteLoc = 0;
	pfile->lHeaderSize = sizeof(MainAVIHeader) + 11 * sizeof(DWORD); 
	pfile->avihdr.dwFlags = AVIF_HASINDEX;
    } else {

	/* Read RIFF chunk */
	if (shfileDescend(pfile->hshfile, &ckRIFF, NULL, 0) != 0)
	    goto readerror;

	/*
	 * check for a 'QuickTime AVI' file, a QuickTime AVI file is a
	 * QuickTime public movie with a AVI file in the 'mdat' atom.
	 */
	if (ckRIFF.cksize == mmioFOURCC('m','d','a','t'))
	{
	    /*
	     * now the 'mdat' atom better be a RIFF/AVI or we cant handle it.
	     */
	    if (shfileDescend(pfile->hshfile, &ckRIFF, NULL, 0) != 0)
		goto formaterror;
	}

	if (ckRIFF.ckid != FOURCC_RIFF)
	    goto formaterror;

	if (ckRIFF.fccType != formtypeAVI)
	    goto formaterror;

	/* Read header list */
	ckLIST.fccType = listtypeAVIHEADER;
	if (FindChunkAndKeepExtras(&pfile->extra, pfile->hshfile, &ckLIST, &ckRIFF, MMIO_FINDLIST))
	    goto error;

	pfile->lHeaderSize = ckLIST.cksize + 8 * sizeof(DWORD);

	/* Read AVI header chunk */
	ck.ckid = ckidAVIMAINHDR;
	if (FindChunkAndKeepExtras(&pfile->extra, pfile->hshfile, &ck, &ckLIST, MMIO_FINDCHUNK))
	    goto error;

	dwSize = min(ck.cksize, sizeof(MainAVIHeader));

	/* Read AVI header info */
	if (shfileRead(pfile->hshfile, (HPSTR)&pfile->avihdr, dwSize) != (LONG)dwSize)
	    goto readerror;

	if (shfileAscend(pfile->hshfile, &ck, 0))
	    goto readerror;

	for (iStream = 0; iStream < (int)pfile->avihdr.dwStreams; iStream++) {
	    pfile->ps[iStream] = NULL;
	}
	
	/* Allocate stream data stuff, read streams */
	for (iStream = 0; iStream < (int)pfile->avihdr.dwStreams; ) {

	    if (shfileDescend(pfile->hshfile, &ckStream, &ckLIST, 0) != 0)
		goto readerror;

	    //
	    //  found a non-stream header skip
	    //
	    if (ckStream.fccType != listtypeSTREAMHEADER ||
		    ckStream.ckid != FOURCC_LIST) {
		if ((hr = ReadIntoExtra(&pfile->extra,
				  pfile->hshfile,
				  &ckStream)) != ResultFromScode(AVIERR_OK))
		    goto error;
		
		if (shfileAscend(pfile->hshfile, &ckStream, 0) != 0)
		    goto readerror;

		continue;
	    }

	    pfile->ps[iStream] = new FAR CAVIStream(NULL, &pUnk);
	    if (!pfile->ps[iStream])
		goto memerror;

	    pavi = pfile->ps[iStream];
	    pavi->pfile = pfile;
	    pavi->iStream = iStream;

	    //
	    //  walk every chunk in this stream header, until we are done.
	    //
	    while (shfileDescend(pfile->hshfile, &ck, &ckStream, 0) == 0) {
		switch (ck.ckid) {
                    case ckidSTREAMHEADER:
                        //
                        // set these to sane default's incase the file
                        // header is not big enough
                        //
                        // NOTE the stream rectangle is set to NULL, if
                        // this is a video stream it will be corrected
                        // when we proccess the format.
                        //
                        strhdr.dwQuality = (DWORD) ICQUALITY_DEFAULT;
                        SetRectEmpty(&strhdr.rcFrame);

			l = min(ck.cksize, sizeof(AVIStreamHeader));

			if (shfileRead(pfile->hshfile, (HPSTR)&strhdr, l) != l)
			    goto readerror;

			// !!! Copy fields from strhdr into
			pavi->avistream.fccType =       strhdr.fccType;
			pavi->avistream.fccHandler =    strhdr.fccHandler;
			pavi->avistream.dwFlags =       strhdr.dwFlags; //!!!
			pavi->avistream.dwCaps =        0; // !!!
			pavi->avistream.wPriority =	strhdr.wPriority;
			pavi->avistream.wLanguage =	strhdr.wLanguage;
			pavi->avistream.dwRate =        strhdr.dwRate;
			pavi->avistream.dwScale =       strhdr.dwScale;
			pavi->avistream.dwStart =       strhdr.dwStart;
			pavi->avistream.dwLength =      strhdr.dwLength;
			pavi->avistream.dwSuggestedBufferSize = strhdr.dwSuggestedBufferSize;
			pavi->avistream.dwInitialFrames = strhdr.dwInitialFrames;
			pavi->avistream.dwQuality =     strhdr.dwQuality;
			pavi->avistream.dwSampleSize =  strhdr.dwSampleSize;
			pavi->avistream.rcFrame =       strhdr.rcFrame;
			pavi->avistream.dwEditCount =   0;
			pavi->avistream.dwFormatChangeCount =   0;

			// Make up a stream name out of the filename, stream
			// type, and stream number.
			if (pavi->avistream.fccType == streamtypeVIDEO)
			    LoadString(ghMod, IDS_VIDEO, ach, sizeof(ach));
			else if (pavi->avistream.fccType == streamtypeAUDIO)
			    LoadString(ghMod, IDS_AUDIO, ach, sizeof(ach));
			else
			    wsprintf(ach, "'%4.4s'",
				(LPSTR)&(pavi->avistream.fccType));

			// figure out what # stream of this type this is....
			iStreamNumber = 1;
			for (i = 0; i < iStream; i++) {
			    if (pfile->ps[i]->avistream.fccType ==
					pavi->avistream.fccType)
				++iStreamNumber;
			}
			LoadString(ghMod, IDS_SSSTREAMD, ach2, sizeof(ach2));
			wsprintf(pavi->avistream.szName,
			    (LPSTR)ach2,
                            (LPSTR)FileName(pfile->achFile),
                            (LPSTR)ach,
			    iStreamNumber);
			
			break;

		    case ckidSTREAMFORMAT:

			if (pavi->lpFormat == NULL) {
			    pavi->cbFormat = ck.cksize;
			    pavi->lpFormat = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE,
							    ck.cksize);

			    if (pavi->lpFormat == NULL)
				goto memerror;

			    if (shfileRead(pfile->hshfile, (HPSTR) pavi->lpFormat, (LONG)ck.cksize) != (LONG)ck.cksize)
				goto readerror;

			    #define lpbi ((LPBITMAPINFOHEADER)pavi->lpFormat)

			    if (pavi->avistream.fccType != streamtypeVIDEO)
				break;

			    //
			    // make sure this is set
			    //
			    if (lpbi->biClrUsed == 0 && lpbi->biBitCount <= 8)
				lpbi->biClrUsed = (1 << (int)lpbi->biBitCount);

			    //
			    // fix up bogus stream rectangles.
			    //
			    if (IsRectBogus(&pavi->avistream.rcFrame,
					    pfile->avihdr.dwWidth,
					    pfile->avihdr.dwHeight,
					    lpbi)) {
				SetRect(&pavi->avistream.rcFrame, 0, 0,
				    (int)lpbi->biWidth, (int)lpbi->biHeight);
			    }

			    //
			    // make sure the biCompression is right for
			    // RLE files. stupid OLD files!
			    //
			    if (lpbi->biCompression == 0 && lpbi->biBitCount == 8) {
				if (pavi->avistream.fccHandler == comptypeRLE0 ||
				    pavi->avistream.fccHandler == comptypeRLE)
				    lpbi->biCompression = BI_RLE8;
			    }

			    if (pavi->avistream.fccHandler == comptypeNONE &&
				lpbi->biCompression == 0)
				pavi->avistream.fccHandler = comptypeDIB;

			    if (pavi->avistream.fccHandler == 0 &&
				lpbi->biCompression == 0)
				pavi->avistream.fccHandler = comptypeDIB;

			    // Assuming a DIB handler has RGB data will blow up
			    // if it has RLE data, and VidEdit et. al write out
			    // stupid confusing files like this.
			    //if (pavi->avistream.fccHandler == comptypeDIB)
                            //    lpbi->biCompression = BI_RGB;

                            if (lpbi->biCompression <= BI_RLE8)
                                fRle = TRUE;

			    #undef lpbi
			}
			break;

		    case ckidSTREAMHANDLERDATA:

			if (pavi->lpData == NULL) {
			    pavi->cbData = ck.cksize;
			    pavi->lpData = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE,
							  ck.cksize);

			    if (pavi->lpData == NULL)
				goto memerror;

			    if (shfileRead(pfile->hshfile, (HPSTR)pavi->lpData,
					 (LONG)ck.cksize) != (LONG)ck.cksize)
				goto readerror;
			}
			break;

		    case ckidSTREAMNAME:
			l = min((LONG) ck.cksize, sizeof(pavi->avistream.szName));
			
			if (shfileRead(pfile->hshfile,
				       (HPSTR)pavi->avistream.szName, l) != l)
			    goto readerror;
			break;
			

		    case ckidAVIPADDING:
		    case mmioFOURCC('p','a','d','d'):
			break;

		    default:
			if ((hr = ReadIntoExtra(&pavi->extra,
					  pfile->hshfile,
					  &ck)) != ResultFromScode(AVIERR_OK))
			    goto error;

			break;
		}

		if (shfileAscend(pfile->hshfile, &ck, 0) != 0)
		    goto readerror;
	    }

	    /* Ascend out of stream header */
	    if (shfileAscend(pfile->hshfile, &ckStream, 0) != 0)
		goto readerror;

	    if (pavi->avistream.fccType == 0)
		goto formaterror;

	    if (pavi->lpFormat == NULL)
		goto formaterror;

	    //
	    // make sure the sample size is set right
	    //
	    switch(pavi->avistream.fccType) {
		case streamtypeAUDIO:
		    /* Hack for backward compatibility with audio */
		    pavi->avistream.dwSampleSize =
			((LPWAVEFORMAT)pavi->lpFormat)->nBlockAlign;

		    // For audio, this number isn't useful when reading.
		    pavi->avistream.dwInitialFrames = 0;
		    // !!! We should let people read what the header says....
		    break;

		case streamtypeVIDEO:
		    // !!! But what if the samples are all the right size?
		    pavi->avistream.dwSampleSize = 0;
		    break;

		default:
		    // !!! ??? pavi->avistream.dwInitialFrames = 0;
		    // !!! ??? pavi->avistream.dwSampleSize = 0;
		    break;
	    }


	    l = sizeof(pavi->avistream.szName) - 1;
	    pavi->avistream.szName[l] = '\0';

	    // next stream
	    iStream++;
	}

	// Read extra data at end of header list....
	FindChunkAndKeepExtras(&pfile->extra, pfile->hshfile, &ck, &ckLIST, 0);
		
	if (shfileAscend(pfile->hshfile, &ckLIST, 0))
	    goto readerror;

	/* Find big data chunk */
	ckLIST.fccType = listtypeAVIMOVIE;
	if (FindChunkAndKeepExtras(&pfile->extra, pfile->hshfile, &ckLIST, &ckRIFF, MMIO_FINDLIST))
	    goto error;

	pfile->lDataListStart = ckLIST.dwDataOffset - 2 * sizeof(DWORD);

	if (shfileAscend(pfile->hshfile, &ckLIST, 0))
	    goto readerror;

	// Keep track of where data can be written
	pfile->lWriteLoc = ckLIST.dwDataOffset + ckLIST.cksize;

	//
	// read in or create a index, we only want the index entries for the
	// stream we are interested in!
        //
        ck.ckid = ckidAVINEWINDEX;
        if (FindChunkAndKeepExtras(&pfile->extra, pfile->hshfile, &ck, &ckRIFF, MMIO_FINDCHUNK) == 0 && ck.cksize != 0) {

            if (!ReadInIndex(pfile, ck.cksize, ckLIST.dwDataOffset, fRle))
                goto formaterror;

        } else {
	    /* Seek back to beginning of list, so we can scan */
            shfileSeek(pfile->hshfile, ckLIST.dwDataOffset + sizeof(DWORD), SEEK_SET);

            //!!! should we realy scan big files?
            //!!! or give a error

            pfile->px = IndexCreate();

            if (pfile->px == 0)
                goto formaterror;

            DPF("Scanning index", time = timeGetTime());

	    /* Scan through chunks... */
	    while (shfileDescend(pfile->hshfile, &ck, &ckLIST, 0) == 0) {

                AddToIndex(pfile,ck.ckid,ck.cksize,ck.dwDataOffset-8,0);

		/* Hack: don't ascend from LISTs */
		if (ck.ckid != FOURCC_LIST) {
		    if (shfileAscend(pfile->hshfile, &ck, 0) != 0)
			goto readerror;
                }

                if (pfile->px->nIndex % 512 == 0)
                    DPF("!.");
            }

            DPF("!Done (%ldms)\n", timeGetTime() - time);
        }

        if (pfile->px->nIndex == 0)
	    goto formaterror;

	// Read extra data at end of file....
	FindChunkAndKeepExtras(&pfile->extra, pfile->hshfile, &ck, &ckRIFF, 0);
	
        // shfileSetBuffer(pfile->hshfile, NULL, 0L, 0);

        //
        //  compute dwSuggestedBufferSize
        //
        if (pfile->avihdr.dwFlags & AVIF_ISINTERLEAVED) {

            LONG l;
            LONG lLen;

            pfile->avihdr.dwSuggestedBufferSize = 0;

            for (l=IndexFirst(pfile->px, STREAM_REC);
                 l != -1;
                 l = IndexNext(pfile->px, l, 0)) {

                lLen = IndexLength(pfile->px, l);

                if (pfile->avihdr.dwSuggestedBufferSize < (DWORD)lLen)
                    pfile->avihdr.dwSuggestedBufferSize = (DWORD)lLen;
            }
        }

	if ((pfile->avihdr.dwFlags & AVIF_ISINTERLEAVED) &&
                pfile->avihdr.dwInitialFrames) {

	    pfile->pb = InitBuffered((int) pfile->avihdr.dwInitialFrames * 2,
                                     pfile->avihdr.dwSuggestedBufferSize,
				     pfile->hshfile,
                                     pfile->px);
        }
        else /* if (pfile->avihdr.dwSuggestedBufferSize > 0 &&
                 pfile->avihdr.dwSuggestedBufferSize < 32l*1024) */ {

            int  nBuffers = GetProfileInt("avifile", "buffers", 5);

            pfile->pb = InitBuffered(nBuffers,
                                     min(pfile->avihdr.dwSuggestedBufferSize * 2, 32768L),
				     pfile->hshfile,
                                     pfile->px);
        }
    }

    return ResultFromScode(AVIERR_OK);

readerror:
    hr = ResultFromScode(AVIERR_FILEREAD);
    goto error;

memerror:
    hr = ResultFromScode(AVIERR_MEMORY);
    goto error;

formaterror:
    hr = ResultFromScode(AVIERR_BADFORMAT);
    
error:
    if (hr == ResultFromScode(AVIERR_OK))
	hr = ResultFromScode(AVIERR_ERROR);

    return hr;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::GetStream(PAVISTREAM FAR *ppavi, DWORD fccType, LONG lParam)
{
    CAVIFile FAR * pfile = m_pAVIFile;
    
    CAVIStream FAR *pavi;
    int             iStreamCur;
    int             iStreamWant;
    int             iStream;
    LONG            lLength;

    *ppavi = NULL;
    
    iStreamWant = (int)lParam;

    if (iStreamWant < 0 || iStreamWant >= (int)pfile->avihdr.dwStreams)
	return ResultFromScode(AVIERR_NODATA);

    /* Allocate stream data stuff, read streams */
    for (iStreamCur = -1, iStream = 0;
	    iStream < (int)pfile->avihdr.dwStreams;
	    iStream++) {
	if (fccType == 0 || pfile->ps[iStream]->avistream.fccType == fccType)
	    iStreamCur++;

	if (iStreamCur == iStreamWant)
	    break;
    }

    if (iStreamCur != iStreamWant)
	return ResultFromScode(AVIERR_NODATA);

    pavi = pfile->ps[iStream];

    if (pavi->fInit)
	goto returnnow;

    pavi->fInit = TRUE;
    
#if 0
    if ((pf->mode & (OF_WRITE | OF_READWRITE)) == 0) {
	pavi->hshfile = shfileOpen(pfile->achFile, NULL, MMIO_ALLOCBUF | pfile->mode);

	if (!pavi->hshfile)
	    goto error;
    } else
#endif
    pavi->hshfile = pfile->hshfile;

    AddRef();

    pavi->lPal = -4242;

    pavi->psx = MakeStreamIndex(pfile->px, iStream,
        (LONG)pavi->avistream.dwStart - pavi->avistream.dwInitialFrames,
        (LONG)pavi->avistream.dwSampleSize,
        pfile->hshfile, shfileReadProc, NULL);

    if (pavi->psx == NULL)
        return ResultFromScode(AVIERR_MEMORY);

    pavi->avistream.dwSuggestedBufferSize = pavi->psx->lMaxSampleSize;

    if (pavi->psx->lPalFrames == 0)
        pavi->avistream.dwFlags &= ~AVISF_VIDEO_PALCHANGES;
    else
        pavi->avistream.dwFlags |= AVISF_VIDEO_PALCHANGES;

    pavi->pb = pavi->pfile->pb;

    if (!pavi->pb) {

	lBufferSize = GetProfileInt("avifile", "buffersize", 0) * 1024L;
	nBuffers = GetProfileInt("avifile", "buffers", 0);
    
	if (lBufferSize && nBuffers && !(pavi->pfile->mode & OF_CREATE)) {
	    pavi->pb = InitBuffered(nBuffers, lBufferSize,
                                pavi->hshfile, NULL);
	}
    }

    //
    //  use ReadBuffered() to read data!
    //
    if (pavi->pb) {
        pavi->psx->hFile = (HANDLE)pavi->pb;
        pavi->psx->Read  = (STREAMIOPROC)BufferedRead;
    }

    lLength = pavi->psx->lEnd - pavi->psx->lStart;

    if (lLength != (LONG)pavi->avistream.dwLength +
                            (LONG)pavi->avistream.dwInitialFrames) {
#ifdef DEBUG
	DPF("Stream %d: Length is %ld, header says %ld.\n",
	    iStream, lLength,
	    pavi->avistream.dwLength + pavi->avistream.dwInitialFrames);
#endif
        //!!! should we correct the header!!!
    }

returnnow:
    pavi->m_AVIStream.QueryInterface(IID_IAVIStream, (LPVOID FAR *) ppavi);
    
    //
    // all done return success.
    //
    return ResultFromScode(AVIERR_OK); // success
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::CreateStream(
				   PAVISTREAM FAR *ppavi,
				   AVISTREAMINFO FAR *psi)
{
    CAVIFile FAR * pf = m_pAVIFile;
    CAVIStream FAR * pavi;
    int         iStream = (int) pf->avihdr.dwStreams;
    IUnknown FAR *  pUnk;

    // Check to see if the header will take up too much room!
    if (pf->lWriteLoc > 0) {
	if ((sizeof(AVIStreamHeader) + pf->lHeaderSize +
	     lstrlen(psi->szName) + 8 * sizeof(DWORD)) >
		    pf->lWriteLoc) {
	    DPF("Header will be too big with this new stream!\n");
	    return ResultFromScode(AVIERR_UNSUPPORTED);
	}
    }
    pf->lHeaderSize += sizeof(AVIStreamHeader) + 8 * sizeof(DWORD) +
		       lstrlen(psi->szName);

    if (iStream >= MAXSTREAMS) {
	DPF("Ack: Too many streams: we only support %ld.\n", (LONG) MAXSTREAMS);

	return ResultFromScode(AVIERR_UNSUPPORTED);
    }

    if ((pf->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    pf->ps[iStream] = new FAR CAVIStream(NULL, &pUnk);

    if (!pf->ps[iStream])
	return ResultFromScode(AVIERR_MEMORY);
    
    pavi = pf->ps[iStream];
    pavi->iStream = iStream;
    pavi->pfile = pf;
    pavi->avistream = *psi;
    pavi->avistream.dwLength = 0;       // no data initially
    pavi->avistream.dwSuggestedBufferSize = 0;
    pavi->hshfile = pf->hshfile;
    pavi->m_AVIStream.AddRef();
    AddRef();

    pavi->lpFormat = NULL;      // This will be set leater with a SetFormat
    pavi->cbFormat = 0;

    if (pavi->avistream.fccType == streamtypeAUDIO) {
	SetRectEmpty(&pavi->avistream.rcFrame);
    }

    pf->avihdr.dwStreams++;
    
    if (pavi->iStream == 0) {
	pavi->pfile->avihdr.dwMicroSecPerFrame =
		max(1000L, muldiv32(1000000L,
				     pavi->avistream.dwScale,
				     pavi->avistream.dwRate));
    }

    /* Make sure the width and height of the created file are right.... */
    pf->avihdr.dwWidth = max(pf->avihdr.dwWidth,
			     (DWORD) pavi->avistream.rcFrame.right);
    pf->avihdr.dwHeight = max(pf->avihdr.dwHeight,
			     (DWORD) pavi->avistream.rcFrame.bottom);

    // Only if interleaved?
    pf->avihdr.dwInitialFrames = max(pf->avihdr.dwInitialFrames,
                                     pavi->avistream.dwInitialFrames);

    *ppavi = &pavi->m_AVIStream;

    return ResultFromScode(AVIERR_OK);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#if 0
STDMETHODIMP CAVIFile::CAVIFileImpl::AddStream(
				PAVISTREAM pavi,
				PAVISTREAM FAR *ppaviNew)
{
    CAVIFile FAR * pf = m_pAVIFile;
    CAVIStream FAR * paviNew;
    int         iStream = (int) pf->avihdr.dwStreams;
    HRESULT     hr;
    IUnknown FAR *  pUnk;

    if ((pf->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    pfile->ps[iStream] = new FAR CAVIStream(NULL, &pUnk);

    // !!! error check?
    
    paviNew = pf->ps[iStream];
    paviNew->iStream = iStream;
    paviNew->pfile = pf;
    AVIStreamInfo(pavi, &paviNew->avistream, sizeof(paviNew->avistream));
    paviNew->hshfile = pf->hshfile;
    paviNew->m_AVIStream.AddRef();
    paviNew->paviBase = pavi;
    AVIStreamAddRef(pavi);

    paviNew->cbFormat = AVIStreamFormatSize(pavi, 0);
    paviNew->lpFormat = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, paviNew->cbFormat); 
    if (!paviNew->lpFormat) {
	AVIStreamClose((PAVISTREAM) pf->ps[iStream]);
	return ResultFromScode(AVIERR_MEMORY);
    }

    AVIStreamReadFormat(pavi, 0, paviNew->lpFormat, &paviNew->cbFormat);

    pf->avihdr.dwStreams++;
    
    AddRef();

    if (paviNew->iStream == 0) {
	pf->avihdr.dwMicroSecPerFrame =
		muldiv32(1000000L,
			 paviNew->avistream.dwScale,
			 paviNew->avistream.dwRate);
    }
    
    *ppaviNew = (PAVISTREAM) paviNew;
    
    return ResultFromScode(AVIERR_OK);
}
#endif

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::WriteData(
		       DWORD ckid,
		       LPVOID lpData,
		       LONG cbData)
{
    CAVIFile FAR *      pf = m_pAVIFile;

    // !!! Anything else we can check?
    if (lpData == NULL || cbData == 0)
	return ResultFromScode(AVIERR_BADPARAM);

    if ((pf->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    pf->fDirty = TRUE;

    return WriteExtra(&pf->extra, ckid, lpData, cbData);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::ReadData(
		      DWORD ckid,
		      LPVOID lpData,
		      LONG FAR *lpcbData)
{
    CAVIFile FAR * pf = m_pAVIFile;

    return ReadExtra(&pf->extra, ckid, lpData, lpcbData);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::EndRecord()
{
    CAVIFile FAR * pf = m_pAVIFile;

    if ((pf->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    pf->fDirty = TRUE;
    pf->avihdr.dwFlags |= AVIF_ISINTERLEAVED;

    if (pf->lWriteLoc == 0) {
	pf->lWriteLoc = (pf->lHeaderSize + 1024 + 2047) & ~(2047);
	pf->lDataListStart = pf->lWriteLoc - 3 * sizeof(DWORD);
	DPF("Writing first chunk at position %lu\n", pf->lWriteLoc);
    }

    shfileSeek(pf->hshfile, pf->lWriteLoc, SEEK_SET);
    
    if (pf->fInRecord) {
	// !!! Support padding here?
	
	if (shfileAscend(pf->hshfile, (MMCKINFO FAR *) &pf->ckRecord, 0))
	    return ResultFromScode(AVIERR_FILEWRITE);

        IndexSetLength(pf->px, pf->lRecordIndex, pf->ckRecord.cksize);

	//
	// Keep the main suggested buffer size as big as the biggest
	// record....
	//
	if (pf->ckRecord.cksize + 3 * sizeof(DWORD) >
					    pf->avihdr.dwSuggestedBufferSize)
	    pf->avihdr.dwSuggestedBufferSize = pf->ckRecord.cksize +
							   3 * sizeof(DWORD);
    }

    /* Start the next 'rec' list */
    pf->ckRecord.cksize = 0;
    pf->ckRecord.fccType = listtypeAVIRECORD;
    pf->fInRecord = TRUE;
    if (shfileCreateChunk(pf->hshfile, (MMCKINFO FAR *) &pf->ckRecord, MMIO_CREATELIST)) {
	return ResultFromScode(AVIERR_FILEWRITE);
    }

    pf->lWriteLoc = shfileSeek(pf->hshfile, 0, SEEK_CUR);
    
    pf->lRecordIndex = pf->px->nIndex;

    if (!AddToIndex(pf, pf->ckRecord.fccType, 0,
        pf->ckRecord.dwDataOffset - 2 * sizeof(DWORD), AVIIF_LIST)) {
        return ResultFromScode(AVIERR_MEMORY);
    }

    return ResultFromScode(AVIERR_OK);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Info(
		  AVIFILEINFO FAR * pfi,
		  LONG lSize)
{
    CAVIFile FAR * pf = m_pAVIFile;
    AVIFILEINFO     fi;
    char	ach[30];

    if (pfi == NULL)
	return ResultFromScode(AVIERR_BADPARAM);
	
    if (lSize < sizeof(fi))
	return ResultFromScode(AVIERR_BUFFERTOOSMALL);

    fi.dwMaxBytesPerSec =       pf->avihdr.dwMaxBytesPerSec;
    fi.dwFlags =                0;
    fi.dwStreams =              pf->avihdr.dwStreams;
    fi.dwSuggestedBufferSize =  pf->avihdr.dwSuggestedBufferSize;
    fi.dwWidth =                pf->avihdr.dwWidth;
    fi.dwHeight =               pf->avihdr.dwHeight;
    fi.dwScale =                pf->avihdr.dwMicroSecPerFrame;
    fi.dwRate =                 1000000L;
    fi.dwLength =               pf->avihdr.dwTotalFrames;
    fi.dwEditCount =            0;

    LoadString(ghMod, IDS_AVIFILE, ach, sizeof(ach));
    wsprintf(fi.szFileType, (LPSTR)ach);
    
    hmemcpy(pfi, &fi, sizeof(fi));

    return AVIERR_OK;
}


///////////////////////////////////////////////////////////////////////////
//
//  AVIFileClose()
//
//      close a AVIFile stream
//
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CAVIStream::CUnknownImpl::Release()
{
    CAVIStream FAR * pavi = m_pAVIStream;
	
    uUseCount--;

    if (m_refs < 20) {
	DPF2("Stream %lx: Usage--=%lx\n", (DWORD) (LPVOID) this, m_refs - 1);
    }

    shfileRelease(pavi->hshfile);
    
    if (!--m_refs) {
	if (pavi->hshfile != pavi->pfile->hshfile) {
	    shfileClose(pavi->hshfile, 0);
	    pavi->hshfile = 0;
	}

	if (pavi->pb && pavi->pb != pavi->pfile->pb) {
	    EndBuffered(pavi->pb);
	    pavi->pb = 0;
        }

        if (pavi->psx) {
            FreeStreamIndex(pavi->psx);
            pavi->psx = NULL;
        }

        pavi->fInit = FALSE;

	pavi->pfile->m_AVIFile.Release();
	return 0;
    }
    return m_refs;
}

/*      -       -       -       -       -       -       -       -       */

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

void CAVIStream::CAVIStreamImpl::ReadPalette(LONG lPos, LONG lPal, LPRGBQUAD prgb)
{
    CAVIStream FAR * pavi = m_pAVIStream;

    LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) pavi->lpFormat;
    LONG l;
    int i,n;

    static struct {
	BYTE                bFirstEntry;    /* first entry to change */
	BYTE                bNumEntries;    /* # entries to change (0 if 256) */
	WORD                wFlags;         /* Mostly to preserve alignment... */
	PALETTEENTRY        peNew[256];     /* New color specifications */
    } pc;

    DPF("Reading palette: lPos = %ld, lPal = %ld\n", lPos, lPal);

    if (lPal > lPos)
	lPal = 0;

    //
    //  get the palette colors in the initial format header
    //
    if (lPal <= 0) {
	hmemcpy(prgb,(LPBYTE)lpbi+(int)lpbi->biSize, lpbi->biClrUsed * sizeof(RGBQUAD));
	lPal = -1;
    }

    for (;;) {
	//
	//  search index forward for next palette change
	//
        l = StreamFindSample(pavi->psx, lPal+1, FIND_FORMAT|FIND_NEXT);

        if (l < 0 || l > lPos || l == lPal)
	    break;

	lPal = l;

	if (l <= (LONG) pavi->avistream.dwStart)
            continue;

        LONG off = StreamFindSample(pavi->psx, lPal, FIND_FORMAT|FIND_OFFSET);
        LONG len = StreamFindSample(pavi->psx, lPal, FIND_FORMAT|FIND_LENGTH);

#ifdef DEBUG
        DWORD adw[2];
        shfileSeek(pavi->hshfile, off-8, SEEK_SET);
        shfileRead(pavi->hshfile, (HPSTR)adw, sizeof(adw));
        Assert(TWOCCFromFOURCC(adw[0]) == cktypePALchange);
        Assert(adw[1] == (DWORD) len);
#endif

        if (len > (LONG)sizeof(AVIPALCHANGE) + (LONG)lpbi->biClrUsed * sizeof(PALETTEENTRY) * 2) {
	    DPF("Palette chunk obviously too large!\n");
	    break;
	}

	//
	//  read palchange from file and apply it
	//
        shfileSeek(pavi->hshfile, off, SEEK_SET);

        while (len >= sizeof(AVIPALCHANGE)) {

	    if (shfileRead(pavi->hshfile, (HPSTR)&pc, sizeof(AVIPALCHANGE)) !=
			sizeof(AVIPALCHANGE)) {
		DPF("Error reading palette change\n");
		break;
	    }

	    n = pc.bNumEntries == 0 ? 256 : (int)pc.bNumEntries;

	    if ((DWORD) n > lpbi->biClrUsed) {
		DPF("%d colors in palette change, only %lu in movie!\n", n, lpbi->biClrUsed);
		break;
            }

            if (pc.bFirstEntry + n > (int)lpbi->biClrUsed) {
		DPF("%d colors in palette change, only %lu in movie!\n", n, lpbi->biClrUsed);
		break;
	    }

	    if (shfileRead(pavi->hshfile,
			   (HPSTR)&pc.peNew,
			   n * sizeof(PALETTEENTRY)) !=
		    (LONG) n * sizeof(PALETTEENTRY)) {
		DPF("Error reading palette change entries\n");
		break;
	    }

	    for (i=0; i<n; i++) {
		pavi->argbq[pc.bFirstEntry+i].rgbRed       = pc.peNew[i].peRed;
		pavi->argbq[pc.bFirstEntry+i].rgbGreen     = pc.peNew[i].peGreen;
		pavi->argbq[pc.bFirstEntry+i].rgbBlue      = pc.peNew[i].peBlue;
		pavi->argbq[pc.bFirstEntry+i].rgbReserved  = 0;
	    }

            len -= n * sizeof(PALETTEENTRY) + sizeof(AVIPALCHANGE);
	}
    }
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::ReadFormat(LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    CAVIStream FAR * pavi = m_pAVIStream;
    
    LONG lPal;
    LPBITMAPINFOHEADER lpbi;

    if (lpcbFormat == NULL)
	return ResultFromScode(AVIERR_BADPARAM);

    if (lpFormat == NULL || *lpcbFormat == 0) {
	*lpcbFormat = pavi->cbFormat;
	return AVIERR_OK;
    }

    if (pavi->avistream.dwFlags & AVISF_VIDEO_PALCHANGES) {

        Assert(pavi->psx);

	//
	//  now go find the nearest palette change
	//
        lPal = StreamFindSample(pavi->psx, lPos, FIND_FORMAT|FIND_PREV);

        if (lPal < 0)
	    lPal = 0;

	if (lPal != pavi->lPal) {
	    ReadPalette(lPal, pavi->lPal, pavi->argbq);
	    pavi->lPal = lPal;
	}

	lpbi = (LPBITMAPINFOHEADER) pavi->lpFormat;

	hmemcpy(lpFormat, lpbi, min((LONG) lpbi->biSize, *lpcbFormat));

	if (*lpcbFormat > (LONG) lpbi->biSize) {
	    hmemcpy((LPBYTE)lpFormat + (int)lpbi->biSize, pavi->argbq,
		    min(lpbi->biClrUsed * sizeof(RGBQUAD),
			*lpcbFormat - lpbi->biSize));
	}
    }
    else {
	hmemcpy(lpFormat, pavi->lpFormat, min(*lpcbFormat, pavi->cbFormat));
    }

    if (*lpcbFormat < pavi->cbFormat) {
	*lpcbFormat = pavi->cbFormat;
	return ResultFromScode(AVIERR_BUFFERTOOSMALL);
    }

    *lpcbFormat = pavi->cbFormat;

    return AVIERR_OK;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Create(LONG lParam1, LONG lParam2)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Info(AVISTREAMINFO FAR * psi, LONG lSize)
{
    CAVIStream FAR * pavi = m_pAVIStream;
    
    if (psi == NULL)
	return ResultFromScode(AVIERR_BADPARAM);

    if (lSize < sizeof(pavi->avistream))
	return ResultFromScode(AVIERR_BUFFERTOOSMALL);

    hmemcpy(psi, &pavi->avistream, sizeof(pavi->avistream));

    return AVIERR_OK;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(LONG) CAVIStream::CAVIStreamImpl::FindSample(LONG lPos, LONG lFlags)
{
    CAVIStream FAR * pavi = m_pAVIStream;
    
    if (pavi->paviBase) {
	// If we haven't copied over the data yet, delegate.
        return AVIStreamFindSample(pavi->paviBase, lPos, lFlags);
    }

    if (lPos < (LONG)pavi->avistream.dwStart)
	return -1;

    if (lPos >= (LONG)(pavi->avistream.dwStart + pavi->avistream.dwLength))
        return -1;

    lPos = StreamFindSample(pavi->psx, lPos, (UINT)lFlags);

    return lPos < 0 ? -1 : lPos;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Read(
		 LONG       lStart,
		 LONG       lSamples,
		 LPVOID     lpBuffer,
		 LONG       cbBuffer,
		 LONG FAR * plBytes,
		 LONG FAR * plSamples)
{
    CAVIStream FAR *    pavi = m_pAVIStream;
    LONG                lBytes;

    if (pavi->paviBase) {
	// If we haven't copied over the data yet, delegate.
	return AVIStreamRead(pavi->paviBase, lStart, lSamples,
			     lpBuffer, cbBuffer, plBytes, plSamples);
    }

    Assert(pavi->psx);

    // !!! What if start too big? Length too long?

    if (lStart < (LONG) pavi->avistream.dwStart) {
	DPF("Read before start!\n");
	return ResultFromScode(AVIERR_BADPARAM);
    }

    // Handle one of the sillier aspects of AVI files:
    // Certain RLE-encoded files have their first frames split
    // up into lots of small pieces.  This code puts all of those
    // pieces back together again if necessary.

    if ((lStart == (LONG) pavi->avistream.dwStart) &&
		    (pavi->avistream.fccType == streamtypeVIDEO) &&
		    (pavi->avistream.dwInitialFrames > 0)) {

	LPBITMAPINFOHEADER  lpbi = (LPBITMAPINFOHEADER) pavi->lpFormat;
	LPVOID              lp;

	lStart -= (LONG) pavi->avistream.dwInitialFrames;
	lBytes = (DWORD)(WORD)DIBWIDTHBYTES(lpbi) * (DWORD)(WORD)lpbi->biHeight;

	//
	// a NULL buffer means return the size buffer needed to read
	// the given sample.
	//
        if (lpBuffer == NULL || cbBuffer == 0) {

	    if (plBytes)
                *plBytes = lBytes;

	    return AVIERR_OK;
	}

	if (cbBuffer < lBytes) {
	    if (plBytes)
		*plBytes = lBytes;
	    DPF("ReadFirst: Buffer is %ld bytes, needed %ld\n", cbBuffer, lBytes);
	    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
	}

	lp = GlobalAllocPtr(GMEM_MOVEABLE, lBytes);

	if (!lp)
	    return ResultFromScode(AVIERR_MEMORY);

        while (lStart <= (LONG)pavi->avistream.dwStart) {

            if (StreamRead(pavi->psx, lStart, 1, lp, lBytes) < 0) {
		GlobalFreePtr(lp);
                return ResultFromScode(AVIERR_FILEREAD);
	    }

	    // We probably shouldn't assume RLE here....
	    DecodeRle(lpbi, (BYTE _huge *) lpBuffer, (BYTE _huge *) lp);
	    lStart++;
	}

	GlobalFreePtr(lp);
        goto done;
    }

    //
    // do the read
    //
    lBytes = StreamRead(pavi->psx,lStart,lSamples,lpBuffer,cbBuffer);

    //
    // check for error
    //
    if (lBytes < 0) {

        if (plBytes)
            *plBytes = 0;

        if (plSamples)
            *plSamples = 0;

        if (cbBuffer == 0)
            return ResultFromScode(AVIERR_ERROR);

        //
        // the error may have been buffer too small, check this.
        //
        if (cbBuffer < pavi->psx->lSampleSize)
            return ResultFromScode(AVIERR_BUFFERTOOSMALL);

        lBytes = StreamFindSample(pavi->psx,lStart,FIND_PREV|FIND_LENGTH);

        if (cbBuffer < lBytes) {

            if (plBytes)
                *plBytes = lBytes;

            return ResultFromScode(AVIERR_BUFFERTOOSMALL);
        }
        else
            return ResultFromScode(AVIERR_FILEREAD);
    }

done:
    if (plBytes)
        *plBytes = lBytes;

    if (plSamples) {
        LONG lSampleSize = pavi->psx->lSampleSize;

        if (lSampleSize)
            *plSamples = lBytes / lSampleSize;
        else 
            *plSamples = 1;
    }

    return AVIERR_OK;
}

/**************************************************************************
* @doc  INTERNAL DRAWDIB
*
* @api BOOL | DibEq | This function compares two dibs.
*
* @parm LPBITMAPINFOHEADER lpbi1 | Pointer to one bitmap.
*       this DIB is assumed to have the colors after the BITMAPINFOHEADER
*
* @parm LPBITMAPINFOHEADER | lpbi2 | Pointer to second bitmap.
*       this DIB is assumed to have the colors after biSize bytes.
*
* @rdesc Returns TRUE if bitmaps are identical, FALSE otherwise.
*
**************************************************************************/
inline BOOL DibEq(LPBITMAPINFOHEADER lpbi1, LPBITMAPINFOHEADER lpbi2)
{
    return
        lpbi1->biCompression == lpbi2->biCompression   &&
        lpbi1->biSize        == lpbi2->biSize          &&
        lpbi1->biWidth       == lpbi2->biWidth         &&
        lpbi1->biHeight      == lpbi2->biHeight        &&
        lpbi1->biBitCount    == lpbi2->biBitCount;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::SetFormat(LONG lPos,LPVOID lpFormat,LONG cbFormat)
{
    CAVIStream FAR *    pavi = m_pAVIStream;
    LPBITMAPINFOHEADER  lpbi = (LPBITMAPINFOHEADER) lpFormat;
    int                 i;
    RGBQUAD FAR *       lprgb;
    struct {
	BYTE            bFirstEntry;    /* first entry to change */
	BYTE            bNumEntries;    /* # entries to change (0 if 256) */
	WORD            wFlags;         /* Mostly to preserve alignment... */
	PALETTEENTRY    pe[256];
    } s;


    //
    // Make sure the stream isn't read-only
    //
    if ((pavi->pfile->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    if (pavi->lpFormat == NULL) {
	// Check to see if the header will take up too much room!
	if (pavi->pfile->lWriteLoc > 0) {
	    if ((pavi->cbFormat + pavi->pfile->lHeaderSize + 2 * sizeof(DWORD)) >
			pavi->pfile->lWriteLoc) {
		DPF("Header will be too big with this format!\n");
		return ResultFromScode(AVIERR_UNSUPPORTED);
	    }
	}
	pavi->pfile->lHeaderSize += cbFormat + 2 * sizeof(DWORD);

	// This is a new stream, whose format hasn't been set.
	pavi->lpFormat = GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, cbFormat);
	if (!pavi->lpFormat) {
	    return ResultFromScode(AVIERR_MEMORY);
	}

	hmemcpy(pavi->lpFormat, lpFormat, cbFormat);
	pavi->cbFormat = cbFormat;

	if (pavi->avistream.fccType == streamtypeVIDEO) {

	    if (IsRectBogus(&pavi->avistream.rcFrame,
			    pavi->pfile->avihdr.dwWidth,
			    pavi->pfile->avihdr.dwHeight,
			    lpbi)) {
		DPF("Resetting stream rectangle....\n");
		SetRect(&pavi->avistream.rcFrame, 0, 0,
		    (int)lpbi->biWidth, (int)lpbi->biHeight);
	    }

	    if (lpbi->biClrUsed > 0) {
		// Get the right colors, so that we can detect palette changes
		hmemcpy(pavi->argbq,
			(LPBYTE) lpbi + lpbi->biSize,
			lpbi->biClrUsed * sizeof(RGBQUAD));
	    }

	    /* Make sure the width and height of the created file are right.... */
	    pavi->pfile->avihdr.dwWidth = max(pavi->pfile->avihdr.dwWidth,
				 (DWORD) pavi->avistream.rcFrame.right);
	    pavi->pfile->avihdr.dwHeight = max(pavi->pfile->avihdr.dwHeight,
				 (DWORD) pavi->avistream.rcFrame.bottom);
	} 

	return 0;
    }

    //
    // First, check if the format is actually different....
    //
    if (cbFormat == pavi->cbFormat &&
		(_fmemcmp(pavi->lpFormat, lpFormat, (int) cbFormat) == 0))
	return AVIERR_OK;

    //
    // We really only support format changes if they're palette changes...
    //
    if (pavi->avistream.fccType != streamtypeVIDEO) {	
	return ResultFromScode(AVIERR_UNSUPPORTED);
    }

    //
    // Can only currently set the palette at the end of the file
    //
    if (lPos < (LONG) (pavi->avistream.dwStart + pavi->avistream.dwLength))
	return ResultFromScode(AVIERR_UNSUPPORTED);

    //
    // We can only change the palette for things with palettes....
    //
    if (lpbi->biBitCount > 8 || lpbi->biClrUsed == 0)
	return ResultFromScode(AVIERR_UNSUPPORTED);

    //
    // Be sure only the palette is changing, nothing else....
    //
    if (cbFormat != pavi->cbFormat)
	return ResultFromScode(AVIERR_UNSUPPORTED);

    if (!DibEq((LPBITMAPINFOHEADER) lpFormat,
	       (LPBITMAPINFOHEADER) pavi->lpFormat))
	return ResultFromScode(AVIERR_UNSUPPORTED);

    // !!! Need to do here:
    // Get the correct palette for this point in the file, and check
    // that the new palette is in fact different.
    lprgb = (RGBQUAD FAR *) ((LPBYTE) lpbi + lpbi->biSize);

    if (_fmemcmp(pavi->argbq, lprgb, (UINT) lpbi->biClrUsed * sizeof(RGBQUAD)) == 0)
	return AVIERR_OK;

    //
    // Make the new format the current one....
    //
    hmemcpy(pavi->argbq, lprgb, lpbi->biClrUsed * sizeof(RGBQUAD));
    pavi->lPal = lPos;
    
    //
    // And be sure the stream is marked as having changes...
    //
    pavi->avistream.dwFlags |= AVISF_VIDEO_PALCHANGES;
    
    s.bFirstEntry = 0;
    s.bNumEntries = (BYTE) lpbi->biClrUsed;
    s.wFlags = 0;
    for (i = 0; i < (int) lpbi->biClrUsed; i++, lprgb++) {
	s.pe[i].peRed = lprgb->rgbRed;
	s.pe[i].peGreen = lprgb->rgbGreen;
	s.pe[i].peBlue = lprgb->rgbBlue;
    }
    
    // !!! Hack: use Write to write the palette change....

    return Write(lPos,
		 0,
		 &s,
		 sizeof(AVIPALCHANGE) + lpbi->biClrUsed * sizeof(PALETTEENTRY), 
		 AVIIF_NOTIME, NULL, NULL);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Write(LONG lStart,
					       LONG lSamples,
					       LPVOID lpData,
					       LONG cbData,
					       DWORD dwFlags,
					       LONG FAR *plSampWritten,
					       LONG FAR *plBytesWritten)
{
    CAVIStream FAR *        pavi = m_pAVIStream;
    MMCKINFO        ck;
    WORD            cktype;
    HRESULT	    hr;
    DWORD	    dwmsec;

    // !!! Idea: if it's audio-like data, and everything else matches the
    // last chunk written out, then merge the new data in with the old
    // data, rather than making a new chunk....
    
    if ((pavi->pfile->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    if (pavi->avistream.fccType == streamtypeAUDIO)
	cktype = aviTWOCC('w', 'b');
    else if (pavi->avistream.fccType == streamtypeVIDEO) {
	if (dwFlags & AVIIF_NOTIME)         
	    cktype = aviTWOCC('p', 'c');
	else {
	    LPBITMAPINFOHEADER lpbi = (LPBITMAPINFOHEADER) pavi->lpFormat;

	    if ((dwFlags & AVIIF_KEYFRAME) ||
		(lpbi->biCompression <= BI_RLE8 && cbData == (LONG) lpbi->biSizeImage))
		cktype = aviTWOCC('d', 'b');
	    else
		cktype = aviTWOCC('d', 'c');
	    // !!! 00dx ack!
	}
    } else {
        cktype = aviTWOCC('d', 'c');
    }
    
    ck.ckid = MAKEAVICKID(cktype, pavi->iStream);
    ck.cksize = cbData;

    if (lStart < 0)
	lStart = pavi->avistream.dwStart + pavi->avistream.dwLength;
    
    if (lStart > (LONG) (pavi->avistream.dwStart + pavi->avistream.dwLength)) {
	if (pavi->avistream.dwSampleSize == 0) {
	    // !!! stupid hack--insert lots of blank index entries....

	    while (lStart > (LONG) (pavi->avistream.dwStart + pavi->avistream.dwLength)) {
#if 1
		hr = Write(pavi->avistream.dwStart + pavi->avistream.dwLength,
			    1,
			    NULL,
			    0,
			    0,
			    NULL,
			    NULL);

		if (FAILED(hr))
		    return hr;
#else
		if (!AddToIndex(pavi->pfile, ck.ckid, 0, 0, 0))
		    return ResultFromScode(AVIERR_MEMORY);

		++pavi->avistream.dwLength;
		pavi->pfile->avihdr.dwFlags |= AVIF_MUSTUSEINDEX;
#endif
	    }
	} else 
	    return ResultFromScode(AVIERR_BADPARAM);
    }
    
    if (lStart < (LONG) (pavi->avistream.dwStart + pavi->avistream.dwLength))
	return ResultFromScode(AVIERR_UNSUPPORTED);
    
    pavi->pfile->fDirty = TRUE;

    if (pavi->pfile->lWriteLoc == 0) {
	pavi->pfile->lWriteLoc = (pavi->pfile->lHeaderSize + 1024 + 2047) & ~(2047);
	pavi->pfile->lDataListStart = pavi->pfile->lWriteLoc - 3 * sizeof(DWORD);
	DPF("Writing first chunk at position %lu\n", pavi->pfile->lWriteLoc);
    }

#if 0
    if ((lStart == (LONG) (pavi->avistream.dwStart + pavi->avistream.dwLength)) &&
	(pavi->avistream.fccType == streamtypeAUDIO) &&
	(pavi->pfile->lIndex > 0)) {
	    AVIINDEXENTRY   idx = pavi->pfile->pIndex[pavi->pfile->lIndex - 1];

	    if ((idx.ckid == ckid) &&
		(idx.dwChunkOffset +
		 2 * sizeof(DWORD) +
		 idx.dwChunkLength == lWriteLoc)) {

		// We could append to the previous chunk here....

	    }
    }
		
#endif
    
#ifdef DONTWRITEZEROLENGTH
    if (cbData == 0) {
	ck.dwDataOffset = 0;
	pavi->pfile->avihdr.dwFlags |= AVIF_MUSTUSEINDEX;
    } else
#endif
    {
	shfileSeek(pavi->hshfile, pavi->pfile->lWriteLoc, SEEK_SET);
	shfileCreateChunk(pavi->hshfile, &ck, 0);

	if (cbData) {
	    if (shfileWrite(pavi->hshfile, (HPSTR) lpData, cbData) != cbData)
		return ResultFromScode(AVIERR_FILEWRITE);
	}

	if (shfileAscend(pavi->hshfile, &ck, 0) != 0)
	    return ResultFromScode(AVIERR_FILEWRITE);
	
	pavi->pfile->lWriteLoc = shfileSeek(pavi->hshfile, 0, SEEK_CUR);
    }
    
    if (!AddToIndex(pavi->pfile, ck.ckid, cbData,
		    ck.dwDataOffset - 2 * sizeof(DWORD), dwFlags))
        return ResultFromScode(AVIERR_MEMORY);

    //
    // if we dont have a stream index now is a good time to make one.
    //
    if (pavi->psx == NULL) {

        pavi->psx = MakeStreamIndex(pavi->pfile->px, pavi->iStream,
            (LONG)pavi->avistream.dwStart - pavi->avistream.dwInitialFrames,
            (LONG)pavi->avistream.dwSampleSize,
            pavi->pfile->hshfile, shfileReadProc, NULL);

        //!!! what about pavi->pb

        if (!(dwFlags & AVIIF_NOTIME))
            pavi->psx->lEnd -= lSamples;    // correct for the decrement below
    }

    if (pavi->psx == NULL) {
        DPF("CAVIStream::Write no stream index!\n");
        return ResultFromScode(AVIERR_MEMORY);
    }

    if (!(dwFlags & AVIIF_NOTIME)) {
        pavi->avistream.dwLength += lSamples;

        if (pavi->psx)
            pavi->psx->lEnd += lSamples;
    }
    
    if (cbData > (LONG) pavi->avistream.dwSuggestedBufferSize)
	pavi->avistream.dwSuggestedBufferSize = cbData;

    if (cbData > (LONG) pavi->pfile->avihdr.dwSuggestedBufferSize)
	pavi->pfile->avihdr.dwSuggestedBufferSize = cbData;

    // Recalculate the overall file length....
    dwmsec = muldiv32(pavi->avistream.dwLength,
			     pavi->avistream.dwScale * 1000L,
			     pavi->avistream.dwRate);
    pavi->pfile->avihdr.dwTotalFrames =
	max(pavi->pfile->avihdr.dwTotalFrames,
	    (DWORD) muldiv32(dwmsec, 1000L,
			     pavi->pfile->avihdr.dwMicroSecPerFrame));
    // !!! The above calculation could easily overflow.
    // !!! NEEDS TO BE REORGANIZED!

    if (plBytesWritten)
	*plBytesWritten = cbData;
    
    if (plSampWritten)
	*plSampWritten = lSamples;
    
    return ResultFromScode(AVIERR_OK);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Delete(LONG lStart,LONG lSamples)
{
    CAVIStream FAR *      pavi = m_pAVIStream;

    if ((pavi->pfile->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    // go through and kill things from the index?
    
    // !!! what about keyframe boundaries?
	
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::ReadData(DWORD ckid, LPVOID lp, LONG FAR *lpcb)
{
    CAVIStream FAR *      pavi = m_pAVIStream;

    return ReadExtra(&pavi->extra, ckid, lp, lpcb);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::WriteData(DWORD ckid, LPVOID lp, LONG cb)
{
    CAVIStream FAR *      pavi = m_pAVIStream;

    // Check to see if the header will take up too much room!
    if (pavi->pfile->lWriteLoc > 0) {
	if ((cb + pavi->pfile->lHeaderSize + 2 * sizeof(DWORD)) >
		    pavi->pfile->lWriteLoc) {
	    DPF("Header will be too big with this extra data!\n");
	    return ResultFromScode(AVIERR_UNSUPPORTED);
	}
    }
    
    pavi->pfile->lHeaderSize += cb + 3 * sizeof(DWORD);

    if ((pavi->pfile->mode & (OF_WRITE | OF_READWRITE)) == 0)
	return ResultFromScode(AVIERR_READONLY);

    pavi->pfile->fDirty = TRUE;

    return WriteExtra(&pavi->extra, ckid, lp, cb);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

#if 0
STDMETHODIMP CAVIStream::CAVIStreamImpl::Clone(PAVISTREAM FAR * ppaviNew)
{
    CAVIStream FAR *      pavi = m_pAVIStream;

    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#endif


STDMETHODIMP CAVIStream::CStreamingImpl::Begin(LONG  lStart,	
					       LONG  lEnd,	
					       LONG  lRate)
{
    CAVIStream FAR *      pavi = m_pAVIStream;

    if (pavi->pb)
	BeginBufferedStreaming(pavi->pb, lRate > 0);
    
    return ResultFromScode(AVIERR_OK);
}


STDMETHODIMP CAVIStream::CStreamingImpl::End()
{
    CAVIStream FAR *      pavi = m_pAVIStream;

    if (pavi->pb)
	EndBufferedStreaming(pavi->pb);
    
    return ResultFromScode(AVIERR_OK);
}



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

static
BOOL AddToIndex(CAVIFile FAR * pfile, DWORD ckid, DWORD cksize, LONG off, DWORD dwFlags)
{
    PAVIINDEX px;
    AVIINDEXENTRY idx;

    idx.ckid          = ckid;
    idx.dwChunkOffset = off;
    idx.dwChunkLength = cksize;
    idx.dwFlags       = dwFlags;

    px = IndexAddFileIndex(pfile->px, &idx, 1, 0, FALSE);

    if (px == NULL)
        return FALSE;

    //
    // GlobalReAlloc may have moved our pointer, we need to patch all
    // places we use it!
    //
    if (px != pfile->px) {

        DPF("Index pointer has changed!\n");

        pfile->px = px;

        for (int i=0; i<(int)pfile->avihdr.dwStreams; i++) {

            CAVIStream FAR *ps = pfile->ps[i];

            if (ps->psx)
                ps->psx->px = px;
        }

        if (pfile->pb)
            pfile->pb->px = px;
    }

    return TRUE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Reserved1(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Reserved2(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Reserved3(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Reserved4(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIFile::CAVIFileImpl::Reserved5(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Reserved1(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Reserved2(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Reserved3(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Reserved4(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

STDMETHODIMP CAVIStream::CAVIStreamImpl::Reserved5(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}



CAVIFile::CMarshalImpl::CMarshalImpl(
	CAVIFile FAR*   pAVIFile)
{
	m_pAVIFile = pAVIFile;
}


STDMETHODIMP CAVIFile::CMarshalImpl::QueryInterface(
	const IID FAR&  iid,
	void FAR* FAR*  ppv)
{
	return m_pAVIFile->m_pUnknownOuter->QueryInterface(iid, ppv);
}

/*      -       -       -       -       -       -       -       -       */

STDMETHODIMP_(ULONG) CAVIFile::CMarshalImpl::AddRef()
{
	return m_pAVIFile->m_pUnknownOuter->AddRef();
}

/*      -       -       -       -       -       -       -       -       */

STDMETHODIMP_(ULONG) CAVIFile::CMarshalImpl::Release()
{
	return m_pAVIFile->m_pUnknownOuter->Release();
}

/*      -       -       -       -       -       -       -       -       */



// *** IMarshal methods ***
STDMETHODIMP CAVIFile::CMarshalImpl::GetUnmarshalClass (THIS_ REFIID riid, LPVOID pv, 
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPCLSID pCid)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk = &m_pAVIFile->m_Unknown;

    DPF("(F) UnMarshalClass called (context = %lx)\n", dwDestContext);
    
    if (dwDestContext != MSHCTX_LOCAL) {
	LPMARSHAL    pMarshal;
	
	DPF("Marshal context is %lu: delegating...\n", dwDestContext);

	hr = CoGetStandardMarshal(riid, NULL,
				  dwDestContext, pvDestContext,
				  mshlflags, &pMarshal);

	if (hr != NOERROR)
	    return hr;

	hr = pMarshal->GetUnmarshalClass(riid, pv,
					 dwDestContext, pvDestContext,
					 mshlflags, pCid);

	pMarshal->Release();

	return hr;
    }
    *pCid = CLSID_AVISimpleUnMarshal;
    return hr;
}

STDMETHODIMP CAVIFile::CMarshalImpl::GetMarshalSizeMax (THIS_ REFIID riid, LPVOID pv, 
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPDWORD pSize)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk = &m_pAVIFile->m_Unknown;

    if (dwDestContext != MSHCTX_LOCAL) {
	LPMARSHAL    pMarshal;
	
	hr = CoGetStandardMarshal(riid, NULL, dwDestContext, pvDestContext,
				  mshlflags, &pMarshal);

	if (hr != NOERROR)
	    return hr;

	hr = pMarshal->GetMarshalSizeMax(riid, pv,
					 dwDestContext, pvDestContext,
					 mshlflags, pSize);

	pMarshal->Release();

	return hr;
    }
    *pSize = sizeof(pUnk);
    
    return hr;
}

STDMETHODIMP CAVIFile::CMarshalImpl::MarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
		    LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk = &m_pAVIFile->m_Unknown;

    DPF("MarshalInterface (F) called\n");
    
    if (dwDestContext != MSHCTX_LOCAL) {
	LPMARSHAL    pMarshal;
	
	DPF("Marshal context is %lu: delegating...\n", dwDestContext);

	hr = CoGetStandardMarshal(riid, NULL,
				  dwDestContext, pvDestContext,
				  mshlflags, &pMarshal);

	if (hr != NOERROR)
	    return hr;

	hr = pMarshal->MarshalInterface(pStm, riid, pv,
					dwDestContext, pvDestContext,
					mshlflags);

	pMarshal->Release();

	return hr;
    }
    
    if ((riid != IID_IAVIStream && riid != IID_IAVIFile && riid != IID_IUnknown))
        return ResultFromScode(E_INVALIDARG);

    if ((hr = pStm->Write(&pUnk, sizeof(pUnk), NULL)) == NOERROR)
	AddRef();

    DPF("Returns %lx\n", (DWORD) (LPVOID) hr);
    return hr;
}

STDMETHODIMP CAVIFile::CMarshalImpl::UnmarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
		    LPVOID FAR* ppv)
{
    HRESULT hr = ResultFromScode(E_FAIL);

    DPF("(F) UnMarshalInterface called!!!\n");
    return hr;
}

STDMETHODIMP CAVIFile::CMarshalImpl::ReleaseMarshalData (THIS_ LPSTREAM pStm)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk;
    
    hr = pStm->Read(&pUnk,sizeof(pUnk),NULL);
    DPF("(F) ReleaseMarshalData\n");
    if (hr == NOERROR)
	pUnk->Release();

    return hr;
}

STDMETHODIMP CAVIFile::CMarshalImpl::DisconnectObject (THIS_ DWORD dwReserved)
{
    HRESULT hr = NOERROR;

    return hr;
}


CAVIStream::CMarshalImpl::CMarshalImpl(
	CAVIStream FAR* pAVIStream)
{
	m_pAVIStream = pAVIStream;
}


STDMETHODIMP CAVIStream::CMarshalImpl::QueryInterface(
	const IID FAR&  iid,
	void FAR* FAR*  ppv)
{
	return m_pAVIStream->m_pUnknownOuter->QueryInterface(iid, ppv);
}

/*      -       -       -       -       -       -       -       -       */

STDMETHODIMP_(ULONG) CAVIStream::CMarshalImpl::AddRef()
{
	return m_pAVIStream->m_pUnknownOuter->AddRef();
}

/*      -       -       -       -       -       -       -       -       */

STDMETHODIMP_(ULONG) CAVIStream::CMarshalImpl::Release()
{
	return m_pAVIStream->m_pUnknownOuter->Release();
}

/*      -       -       -       -       -       -       -       -       */



// *** IMarshal methods ***
STDMETHODIMP CAVIStream::CMarshalImpl::GetUnmarshalClass (THIS_ REFIID riid, LPVOID pv, 
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPCLSID pCid)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk = &m_pAVIStream->m_Unknown;

    DPF("(S) UnMarshalClass called (context = %lx)\n", dwDestContext);
    *pCid = CLSID_AVISimpleUnMarshal;
    return hr;
}

STDMETHODIMP CAVIStream::CMarshalImpl::GetMarshalSizeMax (THIS_ REFIID riid, LPVOID pv, 
		    DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags, LPDWORD pSize)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk = &m_pAVIStream->m_Unknown;

    *pSize = sizeof(pUnk);
    
    return hr;
}

STDMETHODIMP CAVIStream::CMarshalImpl::MarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
		    LPVOID pv, DWORD dwDestContext, LPVOID pvDestContext,
		    DWORD mshlflags)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk = &m_pAVIStream->m_Unknown;

    DPF("MarshalInterface (S) called\n");
    if ((riid != IID_IAVIStream && riid != IID_IAVIStream && riid != IID_IUnknown))
        return ResultFromScode(E_INVALIDARG);

    if ((hr = pStm->Write(&pUnk, sizeof(pUnk), NULL)) == NOERROR)
	AddRef();

    DPF("Returns %lx\n", (DWORD) (LPVOID) hr);
    return hr;
}

STDMETHODIMP CAVIStream::CMarshalImpl::UnmarshalInterface (THIS_ LPSTREAM pStm, REFIID riid,
		    LPVOID FAR* ppv)
{
    HRESULT hr = ResultFromScode(E_FAIL);

    DPF("(S) UnMarshalInterface called!!!\n");
    return hr;
}

STDMETHODIMP CAVIStream::CMarshalImpl::ReleaseMarshalData (THIS_ LPSTREAM pStm)
{
    HRESULT hr = NOERROR;
    IUnknown FAR * pUnk;
    
    hr = pStm->Read(&pUnk,sizeof(pUnk),NULL);
    DPF("(S) ReleaseMarshalData\n");
    if (hr == NOERROR)
	pUnk->Release();

    return hr;
}

STDMETHODIMP CAVIStream::CMarshalImpl::DisconnectObject (THIS_ DWORD dwReserved)
{
    HRESULT hr = NOERROR;

    return hr;
}



/***************************************************************************

  DecodeRle   - 'C' version

  Play back a RLE buffer into a DIB buffer

  returns
      none

 ***************************************************************************/

void DecodeRle(LPBITMAPINFOHEADER lpbi, BYTE _huge *pb, BYTE _huge *prle)
{
    BYTE    cnt;
    BYTE    b;
    WORD    x;
    WORD    dx,dy;
    WORD    wWidthBytes;

    #define RLE_ESCAPE  0
    #define RLE_EOL     0
    #define RLE_EOF     1
    #define RLE_JMP     2
    #define RLE_RUN     3

#if 0
#ifndef WIN32
    //
    // this uses ASM code found in RLEA.ASM
    //
    if (!(WinFlags & WF_CPU286))
        DecodeRle386(lpbi, pb, prle);
    else if (lpbi->biSizeImage < 65536l)
	DecodeRle286(lpbi, pb, prle);
    else
#endif
#endif
    {
	wWidthBytes = (WORD)lpbi->biWidth+3 & ~3;

	x = 0;

	for(;;)
	{
	    cnt = *prle++;
	    b   = *prle++;

	    if (cnt == RLE_ESCAPE)
	    {
		switch (b)
		{
		    case RLE_EOF:
			return;

		    case RLE_EOL:
			pb += wWidthBytes - x;
			x = 0;
			break;

		    case RLE_JMP:
			dx = (WORD)*prle++;
			dy = (WORD)*prle++;

			pb += (DWORD)wWidthBytes * dy + dx;
			x  += dx;

			break;

		    default:
			cnt = b;
			x  += cnt;
			while (cnt-- > 0)
			    *pb++ = *prle++;

			if (b & 1)
			    prle++;

			break;
		}
	    }
	    else
	    {
		x += cnt;

		while (cnt-- > 0)
		    *pb++ = b;
	    }
	}
    }
}
