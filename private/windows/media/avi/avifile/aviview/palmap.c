/****************************************************************************
 *
 *  PALMAP.C
 *
 *  Stream handler to map to a palette.
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

#include <windows.h>
#include <windowsx.h>
#include <win32.h>
#include <mmsystem.h>
#include <vfw.h>
#include "dibmap.h"
#include "palmap.h"


LONG FAR PASCAL AVIStreamMakePalette(PAVISTREAM pavi, LONG lSkip,
			      HPALETTE FAR *lphpal,
			      LPBYTE lp16to8,
			      int nColors)
{
    LPHISTOGRAM lpHist = NULL;
    LONG	l, lEnd;
    LONG	lRet = AVIERR_OK;
    PGETFRAME	pgf = NULL;

    if (!pavi || !lphpal || nColors < 2 || nColors > 256)
	return AVIERR_BADPARAM;

    if (lSkip < 1)
	lSkip = 1;

    lpHist = InitHistogram(NULL);
    if (!lpHist)
	return AVIERR_MEMORY;

    pgf = AVIStreamGetFrameOpen(pavi, NULL);

    l = AVIStreamStart(pavi);
    lEnd = l + AVIStreamLength(pavi);
    for (l = AVIStreamStart(pavi), lEnd = l + AVIStreamLength(pavi);
	    l < lEnd;
	    l += lSkip) {
	LPBITMAPINFOHEADER lpbi;

	lpbi = AVIStreamGetFrame(pgf, l);

	if (!lpbi) {
	    lRet = AVIERR_INTERNAL;
	    goto error;
	}
	
	DibHistogram(lpbi, NULL, 0, 0, -1, -1, lpHist);
    }

    *lphpal = HistogramPalette(lpHist, lp16to8, nColors);

    if (!*lphpal)
	lRet = AVIERR_MEMORY;

error:
    if (pgf)
	AVIStreamGetFrameClose(pgf);

    if (lpHist)
	FreeHistogram(lpHist);

    return lRet;
}



typedef struct {
    IAVIStreamVtbl FAR *	lpVtbl;

    ULONG			ulRefCount;

    //
    // instance data
    //
    PAVISTREAM	    pavi;
    PGETFRAME	    pgf;
    AVISTREAMINFO avistream;
    HPALETTE	    hpal;
    LPBYTE	    lp16to8;
    LONG	    lLastFrame;
    HANDLE	    hdibLast;
} PALMAPSTREAM, FAR*PPALMAPSTREAM;

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE PalMapStreamQueryInterface(PAVISTREAM ps, REFIID riid, LPVOID FAR* ppvObj);
HRESULT STDMETHODCALLTYPE PalMapStreamCreate       (PAVISTREAM ps, LONG lParam1, LONG lParam2);
ULONG   STDMETHODCALLTYPE PalMapStreamAddRef       (PAVISTREAM ps);
ULONG   STDMETHODCALLTYPE PalMapStreamRelease      (PAVISTREAM ps);
HRESULT STDMETHODCALLTYPE PalMapStreamInfo         (PAVISTREAM ps, AVISTREAMINFO FAR * psi, LONG lSize);
LONG    STDMETHODCALLTYPE PalMapStreamFindKeyFrame (PAVISTREAM ps, LONG lPos, LONG lFlags);
HRESULT STDMETHODCALLTYPE PalMapStreamReadFormat   (PAVISTREAM ps, LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat);
HRESULT STDMETHODCALLTYPE PalMapStreamSetFormat    (PAVISTREAM ps, LONG lPos, LPVOID lpFormat, LONG cbFormat);
HRESULT STDMETHODCALLTYPE PalMapStreamRead         (PAVISTREAM ps, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, LONG FAR * plBytes,LONG FAR * plSamples);
HRESULT STDMETHODCALLTYPE PalMapStreamWrite        (PAVISTREAM ps, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten);
HRESULT STDMETHODCALLTYPE PalMapStreamDelete       (PAVISTREAM ps, LONG lStart, LONG lSamples);
HRESULT STDMETHODCALLTYPE PalMapStreamReadData     (PAVISTREAM ps, DWORD fcc, LPVOID lp,LONG FAR *lpcb);
HRESULT STDMETHODCALLTYPE PalMapStreamWriteData    (PAVISTREAM ps, DWORD fcc, LPVOID lp,LONG cb);

IAVIStreamVtbl PalMapStreamHandler = {
    PalMapStreamQueryInterface,
    PalMapStreamAddRef,
    PalMapStreamRelease,
    PalMapStreamCreate,
    PalMapStreamInfo,
    PalMapStreamFindKeyFrame,
    PalMapStreamReadFormat,
    PalMapStreamSetFormat,
    PalMapStreamRead,
    PalMapStreamWrite,
    PalMapStreamDelete,
    PalMapStreamReadData,
    PalMapStreamWriteData
};

LONG FAR PASCAL AVICreateMappedStream(PAVISTREAM FAR *ppsMapped,
			       PAVISTREAM ps,
			       int nColors)
{
    PPALMAPSTREAM pavi;

    pavi = (PPALMAPSTREAM) GlobalAllocPtr(GHND, sizeof(PALMAPSTREAM));
    if (pavi == NULL)
        return AVIERR_MEMORY;

    pavi->lpVtbl = &PalMapStreamHandler;

    (pavi->lpVtbl->Create)((PAVISTREAM) pavi, (LONG) ps, nColors);
    // !!! error check

    *ppsMapped = (PAVISTREAM) pavi;

    return AVIERR_OK;


}

///////////////////////////////////////////////////////////////////////////
//
//  PalMapStreamOpen()
//
//  open a single stream of a particular type from a AVI file.
//
//  params:
//      szFile      - PAVISTREAM
//      fccType     - must be streamtypeVIDEO
//      lParam	    - nColors
//
//  returns:
//      a PAVISTREAM for the specifed stream or NULL.
//
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE PalMapStreamCreate(PAVISTREAM ps, LONG lParam1, LONG lParam2)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;

    LONG		lRet = AVIERR_OK;

    if (AVIStreamAddRef((PAVISTREAM) lParam1) != AVIERR_OK)
	return ResultFromScode(AVIERR_FILEOPEN);

    pavi->pavi = (PAVISTREAM) lParam1;

    AVIStreamInfo(pavi->pavi, &pavi->avistream, sizeof(pavi->avistream));

    if (pavi->avistream.fccType != streamtypeVIDEO) {
	lRet = AVIERR_INTERNAL;
	goto error;
    }

    pavi->pgf = AVIStreamGetFrameOpen(pavi->pavi, NULL);

    if (!pavi->pgf) {
	lRet = AVIERR_INTERNAL;
	goto error;
    }

    pavi->avistream.fccHandler = 0;

    if (lParam2 < 2 || lParam2 > 256)
	lParam2 = 256;

    pavi->lp16to8 = GlobalAllocPtr(GMEM_MOVEABLE, 32768L);
    if (!pavi->lp16to8) {
	lRet = AVIERR_MEMORY;
	goto error;
    }

    lRet = AVIStreamMakePalette(pavi->pavi,
				AVIStreamLength(pavi->pavi) / 30,
				&pavi->hpal, pavi->lp16to8,
				(int) lParam2);

    pavi->lLastFrame = -1;

error:
    return ResultFromScode(lRet);
}

///////////////////////////////////////////////////////////////////////////
//
//  PalMapStreamAddRef()
//
//      increase the reference count of the stream
//
///////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE PalMapStreamAddRef(PAVISTREAM ps)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;

    return ++pavi->ulRefCount;
}

///////////////////////////////////////////////////////////////////////////
//
//  PalMapStreamRelease()
//
//      close a PalMapStream stream
//
///////////////////////////////////////////////////////////////////////////

ULONG STDMETHODCALLTYPE PalMapStreamRelease(PAVISTREAM ps)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;

    if (--pavi->ulRefCount)
	return pavi->ulRefCount;

    if (pavi->pgf)
	AVIStreamGetFrameClose(pavi->pgf);
    pavi->pgf = 0;

    if (pavi->pavi)
	AVIStreamClose(pavi->pavi);
    pavi->pavi = 0;

    if (pavi->lp16to8) {
	GlobalFreePtr(pavi->lp16to8);
	pavi->lp16to8 = 0;
    }

    if (pavi->hpal) {
	DeleteObject(pavi->hpal);
	pavi->hpal = 0;
    }

    if (pavi->hdibLast) {
	GlobalFree(pavi->hdibLast);
	pavi->hdibLast = 0;
    }

    GlobalFreePtr(pavi);

    return 0;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE PalMapStreamReadFormat(PAVISTREAM ps, LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;
    LPBITMAPINFOHEADER  lpbi;
    LONG		lSize;

    PalMapStreamRead(ps, lPos, 1, NULL, 0, NULL, NULL);

    if (pavi->hdibLast == 0)
	return ResultFromScode(AVIERR_INTERNAL);

    lpbi = (LPBITMAPINFOHEADER) GlobalLock(pavi->hdibLast);	

    lSize = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

    if (lpFormat)
	hmemcpy(lpFormat, lpbi, min(*lpcbFormat, lSize));

    *lpcbFormat = lSize;

    return 0;
}

LONG STDMETHODCALLTYPE PalMapStreamFindKeyFrame(PAVISTREAM ps, LONG lPos, LONG lFlags)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;

    return lPos;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE PalMapStreamInfo(PAVISTREAM ps, AVISTREAMINFO FAR * psi, LONG lSize)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;

    if (psi)
	hmemcpy(psi, &pavi->avistream, min(lSize, sizeof(pavi->avistream)));

    return 0;
}


///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE PalMapStreamRead(PAVISTREAM   ps,
                 LONG       lStart,
                 LONG       lSamples,
                 LPVOID     lpBuffer,
                 LONG       cbBuffer,
                 LONG FAR * plBytes,
                 LONG FAR * plSamples)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;

    LPBITMAPINFOHEADER  lpbi;
    LPVOID		lp;

    if (lStart != pavi->lLastFrame) {
	pavi->lLastFrame = -1;
	lpbi = AVIStreamGetFrame(pavi->pgf, lStart);

	if (!lpbi)
	    goto ReadNothing;

	if (pavi->hdibLast) {
	    GlobalFree(pavi->hdibLast);
	    pavi->hdibLast = 0;
	}

	pavi->hdibLast = DibReduce(lpbi, NULL, pavi->hpal, pavi->lp16to8);
	pavi->lLastFrame = lStart;
    }

    lpbi = (LPBITMAPINFOHEADER) GlobalLock(pavi->hdibLast);	
    //
    // a NULL buffer means return the size buffer needed to read
    // the given sample.
    //
    lp = (LPBYTE) lpbi + lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);
	
    if (plBytes)
	*plBytes = lpbi->biSizeImage;
	
    if (plSamples)
	*plSamples = 1;

    if (lpBuffer) {
	if (cbBuffer >= (LONG) lpbi->biSizeImage)
	    hmemcpy(lpBuffer, lp, lpbi->biSizeImage);
	else
	    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
    }

    return 0;

ReadNothing:
    if (plBytes)
	*plBytes = 0;

    if (plSamples)
	*plSamples = 0;

    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
}



//
//
//   Extra unimplemented functions.....
//
//
//
HRESULT STDMETHODCALLTYPE PalMapStreamQueryInterface(PAVISTREAM ps, REFIID riid, LPVOID FAR* ppvObj)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE PalMapStreamReadData     (PAVISTREAM ps, DWORD fcc, LPVOID lp, LONG FAR *lpcb)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE PalMapStreamSetFormat    (PAVISTREAM ps, LONG lPos, LPVOID lpFormat, LONG cbFormat)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE PalMapStreamWriteData    (PAVISTREAM ps, DWORD fcc, LPVOID lp, LONG cb)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE PalMapStreamWrite        (PAVISTREAM ps, LONG lStart, LONG lSamples, LPVOID lpBuffer, LONG cbBuffer, DWORD dwFlags, LONG FAR *plSampWritten, LONG FAR *plBytesWritten)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

HRESULT STDMETHODCALLTYPE PalMapStreamDelete       (PAVISTREAM ps, LONG lStart, LONG lSamples)
{
    PPALMAPSTREAM pavi = (PPALMAPSTREAM) ps;
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
