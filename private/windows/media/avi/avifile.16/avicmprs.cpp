/****************************************************************************
 *
 *  AVICMPRS.C
 *
 *  routine for compressing AVI files...
 *
 *      AVISave()
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


//
// What this file does:
//
// Given an AVI Stream (that is, essentially, a function that it can call
// to get video frames), this presents the same sort of interface and allows
// other people to call it to get compressed frames.
//

#include <win32.h>
#include <compobj.h>
#include <compman.h>
#include <avifmt.h>
#include "avifile.h"
#include "avifilei.h"
#include "avicmprs.h"
#include "debug.h"

#define ALIGNULONG(i)     ((i+3)&(~3))                  /* ULONG aligned ! */
#define WIDTHBYTES(i)     ((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)
#define DIBPTR(lpbi) ((LPBYTE)(lpbi) + \
            (int)(lpbi)->biSize + \
            (int)(lpbi)->biClrUsed * sizeof(RGBQUAD) )

void CAVICmpStream::ResetInst(void)
{
    lFrameCurrent = -1;
    lLastKeyFrame = 0;
    dwQualityLast = ICQUALITY_HIGH;
    dwSaved = 0;
}

/*	-	-	-	-	-	-	-	-	*/

HRESULT CAVICmpStream::Create(
	IUnknown FAR*	pUnknownOuter,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
{
	IUnknown FAR*	pUnknown;
	CAVICmpStream FAR*	pAVIStream;
	HRESULT	hresult;

	pAVIStream = new FAR CAVICmpStream(pUnknownOuter, &pUnknown);
	if (!pAVIStream)
		return ResultFromScode(E_OUTOFMEMORY);
	hresult = pUnknown->QueryInterface(riid, ppv);
	if (FAILED(GetScode(hresult)))
		delete pAVIStream;
	return hresult;
}

/*	-	-	-	-	-	-	-	-	*/

CAVICmpStream::CAVICmpStream(
	IUnknown FAR*	pUnknownOuter,
	IUnknown FAR* FAR*	ppUnknown) :
	m_Unknown(this),
	m_AVIStream(this)
{
	// !!! clear extra junk!
	pavi = 0;
	pgf = 0;
	hic = 0;
	lpbiC = 0;
	lpbiU = 0;
	lpFormat = 0;
	cbFormat = 0;
	lpFormatOrig = 0;
	cbFormatOrig = 0;
	lpHandler = 0;
	cbHandler = 0;
	
	if (pUnknownOuter)
		m_pUnknownOuter = pUnknownOuter;
	else
		m_pUnknownOuter = &m_Unknown;
	*ppUnknown = &m_Unknown;
}

/*	-	-	-	-	-	-	-	-	*/

CAVICmpStream::CUnknownImpl::CUnknownImpl(
	CAVICmpStream FAR*	pAVIStream)
{
	m_pAVIStream = pAVIStream;
	m_refs = 0;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVICmpStream::CUnknownImpl::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	if (iid == IID_IUnknown)
		*ppv = &m_pAVIStream->m_Unknown;
	else if (iid == IID_IAVIStream)
		*ppv = &m_pAVIStream->m_AVIStream;
	else
		return ResultFromScode(E_NOINTERFACE);
	AddRef();
	return AVIERR_OK;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVICmpStream::CUnknownImpl::AddRef()
{
	uUseCount++;
	return ++m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

CAVICmpStream::CAVICmpStreamImpl::CAVICmpStreamImpl(
	CAVICmpStream FAR*	pAVIStream)
{
	m_pAVIStream = pAVIStream;
}

/*	-	-	-	-	-	-	-	-	*/

CAVICmpStream::CAVICmpStreamImpl::~CAVICmpStreamImpl()
{
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	return m_pAVIStream->m_pUnknownOuter->QueryInterface(iid, ppv);
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVICmpStream::CAVICmpStreamImpl::AddRef()
{
	return m_pAVIStream->m_pUnknownOuter->AddRef();
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVICmpStream::CAVICmpStreamImpl::Release()
{
	return m_pAVIStream->m_pUnknownOuter->Release();
}

/*	-	-	-	-	-	-	-	-	*/

HRESULT CAVICmpStream::SetUpCompression()
{
    LONG		lRet = AVIERR_OK;
    LPBITMAPINFOHEADER  lpbi;
    CAVICmpStream FAR * pinst = this;	// for convenience....
    DWORD		dw;
    
    pinst->pgf = AVIStreamGetFrameOpen(pinst->pavi, NULL);
    if (!pinst->pgf) {
	// !!! we couldn't decompress the stream!
	lRet = AVIERR_INTERNAL;
	goto exit;
    }

    if (pinst->avistream.fccHandler == comptypeDIB)
	goto exit;
    
    lpbi = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pinst->pgf, 0);

    if (lpbi == NULL) {
	lRet = AVIERR_INTERNAL;
	goto exit;
    }
    
    /*
    ** get the size requied to hold the format.
    */
    dw = ICCompressGetFormatSize(pinst->hic, lpbi);
    if ((LONG) dw < sizeof(BITMAPINFOHEADER))
	goto ic_error;

    pinst->cbFormat = dw;
    pinst->lpFormat = (LPBITMAPINFOHEADER) GlobalAllocPtr(GHND | GMEM_SHARE, pinst->cbFormat);
    if (!pinst->lpFormat) {
	lRet = AVIERR_MEMORY;
	goto exit;
    }

    /*
    ** get the compressed format from the compressor.
    */
    dw = ICCompressGetFormat(pinst->hic, lpbi, pinst->lpFormat);
    if ((LONG) dw < 0)
	goto ic_error;
    
    pinst->avistream.rcFrame.right = pinst->avistream.rcFrame.left +
					  (int) pinst->lpFormat->biWidth;
    pinst->avistream.rcFrame.bottom = pinst->avistream.rcFrame.top +
					  (int) pinst->lpFormat->biHeight;
    
    dw = ICCompressBegin(pinst->hic, lpbi, pinst->lpFormat);
    
    if (dw != ICERR_OK)
	goto ic_error;

    /*
    ** allocate buffer to hold compressed data.
    */
    dw = ICCompressGetSize(pinst->hic, lpbi, pinst->lpFormat);

    pinst->lpbiC = (LPBITMAPINFOHEADER)
	GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, pinst->cbFormat + dw);
    
    if (!pinst->lpbiC) {
	lRet = AVIERR_MEMORY;
	goto exit;
    }

    hmemcpy((LPVOID)pinst->lpbiC, pinst->lpFormat, pinst->cbFormat);

    pinst->lpC = (LPSTR) pinst->lpbiC + pinst->lpbiC->biSize +
				pinst->lpbiC->biClrUsed * sizeof(RGBQUAD);
	
    //
    //  check for temporal compress, and alocate a previous
    //  DIB buffer if needed
    //
    if (pinst->dwKeyFrameEvery != 1) {
	pinst->lpbiU = (LPBITMAPINFOHEADER)
	    GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE,
		    sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

	if (!pinst->lpbiU) {
	    lRet = AVIERR_MEMORY;
	    goto exit;
	}

	dw = ICDecompressGetFormat(pinst->hic, pinst->lpFormat, pinst->lpbiU);

	if ((LONG) dw < 0)
	    goto ic_error;

	if (pinst->lpbiU->biSizeImage == 0)
	    pinst->lpbiU->biSizeImage = pinst->lpbiU->biHeight *
					    DIBWIDTHBYTES(*pinst->lpbiU);

	pinst->lpbiU = (LPBITMAPINFOHEADER)
	    GlobalReAllocPtr(pinst->lpbiU,
		pinst->lpbiU->biSize +
			pinst->lpbiU->biClrUsed * sizeof(RGBQUAD) +
			pinst->lpbiU->biSizeImage,
		GMEM_MOVEABLE | GMEM_SHARE);

	if (!pinst->lpbiU) {
	    lRet = AVIERR_MEMORY;
	    goto exit;
	}

	pinst->lpU = (LPSTR) pinst->lpbiU + pinst->lpbiU->biSize +
				pinst->lpbiU->biClrUsed * sizeof(RGBQUAD);
	
	dw = ICDecompressBegin(pinst->hic, pinst->lpFormat, pinst->lpbiU);

	if (dw != ICERR_OK)
	    goto ic_error;
    }

    // !!! We really should check if the new stream has palette changes....
    
exit:
    if (lRet != AVIERR_OK)
	// Clean up before returning...
	;
    
    return ResultFromScode(lRet);
    
ic_error:
    if (dw == ICERR_BADFORMAT)
	lRet = AVIERR_BADFORMAT;
    else if (dw == ICERR_MEMORY)
	lRet = AVIERR_MEMORY;
    else
	lRet = AVIERR_INTERNAL;
    goto exit;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Create(LONG lParam1, LONG lParam2)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;
    ICINFO	    icinfo;
    AVICOMPRESSOPTIONS FAR *lpOpt = (AVICOMPRESSOPTIONS FAR *)lParam2;
    LONG	    lRet = AVIERR_OK;

    // The AVI Stream that we're compressing is passsed in in the <szFile>
    // parameter.
    pinst->pavi = (PAVISTREAM)lParam1;

    // Make sure the uncompressed stream doesn't go away without our
    // knowledge....
    AVIStreamAddRef(pinst->pavi);
    // !!! how can we check if pinst->pavi is valid?
    
    // Get the stream header for future reference....    
    AVIStreamInfo(pinst->pavi, &pinst->avistream, sizeof(pinst->avistream));

    pinst->ResetInst();

    if (!lpOpt || (lpOpt->fccHandler == comptypeDIB)) {
	pinst->avistream.fccHandler = comptypeDIB;
	lRet = AVIERR_OK;
	goto exit;
    }
    
    pinst->avistream.fccHandler = lpOpt->fccHandler;

    // Open the compressor they asked for in the options structure...
    pinst->hic = ICOpen(ICTYPE_VIDEO, lpOpt->fccHandler, ICMODE_COMPRESS);
    
    if (!pinst->hic) {
	lRet = AVIERR_NOCOMPRESSOR;
	goto exit;
    }
    
    if (lpOpt->cbParms) {
	ICSetState(pinst->hic, lpOpt->lpParms, lpOpt->cbParms);
    }

    pinst->avistream.dwQuality = lpOpt->dwQuality;

    if (pinst->avistream.dwQuality == ICQUALITY_DEFAULT) {
	pinst->avistream.dwQuality = ICGetDefaultQuality(pinst->hic);
    }

    /*
    **  get info about this compressor
    */
    ICGetInfo(pinst->hic,&icinfo,sizeof(icinfo));

    pinst->dwICFlags = icinfo.dwFlags;

    if (lpOpt->dwFlags & AVICOMPRESSF_KEYFRAMES)
        pinst->dwKeyFrameEvery = lpOpt->dwKeyFrameEvery;
    else
        pinst->dwKeyFrameEvery = 1;

    if (!(icinfo.dwFlags & VIDCF_TEMPORAL))
	pinst->dwKeyFrameEvery = 1;     // compressor doesn't do temporal

    
    if (lpOpt->dwFlags & AVICOMPRESSF_DATARATE)
	pinst->dwMaxSize = muldiv32(lpOpt->dwBytesPerSecond,
				pinst->avistream.dwScale,
				pinst->avistream.dwRate);
    else
	pinst->dwMaxSize = 0;


    {
	ICCOMPRESSFRAMES    iccf;
	DWORD		    dw;


	iccf.lpbiOutput = pinst->lpbiC;
	iccf.lOutput = 0;

	iccf.lpbiInput = pinst->lpbiU;
	iccf.lInput = 0;

	iccf.lStartFrame = 0;
	iccf.lFrameCount = (LONG) pinst->avistream.dwLength;

	iccf.lQuality = (LONG) pinst->avistream.dwQuality;
	iccf.lDataRate = (LONG) lpOpt->dwBytesPerSecond;

	iccf.lKeyRate = (LONG) pinst->dwKeyFrameEvery;

	iccf.dwRate = pinst->avistream.dwRate;
	iccf.dwScale = pinst->avistream.dwScale;

	iccf.dwOverheadPerFrame = 0;
	iccf.dwReserved2 = 0;
	iccf.GetData = NULL;
	iccf.PutData = NULL;

	dw = ICSendMessage(pinst->hic,
		      ICM_COMPRESS_FRAMES_INFO,
		      (DWORD) (LPVOID) &iccf,
		      sizeof(iccf));

	// If they support this message, don't give
	// warning for data rate!
	if (dw == ICERR_OK) {
	    DPF("Compressor supports COMPRESSFRAMESINFO\n");
	    // !!! fDataRateChanged = TRUE;
	}

#ifdef STATUSCALLBACKS
	ICSetStatusProc(pinst->hic,
			0,
			pinst,
			CompressStatusProc);
#endif
    }


exit:
    if (lRet != AVIERR_OK)
	// Clean up before returning...
	;
    
    return ResultFromScode(lRet);
}

STDMETHODIMP_(ULONG) CAVICmpStream::CUnknownImpl::Release()
{
    CAVICmpStream FAR * pinst = m_pAVIStream;
    
    uUseCount--;
    if (!--m_refs) {
	if (pinst->hic) {
	    ICCompressEnd(pinst->hic);

	    if (pinst->dwKeyFrameEvery != 1 && pinst->lpbiU)
		ICDecompressEnd(pinst->hic);

	    if (pinst->lpbiU)
		GlobalFreePtr((LPVOID) pinst->lpbiU);

	    if (pinst->lpbiC)
		GlobalFreePtr((LPVOID) pinst->lpbiC);

	    ICClose(pinst->hic);
	}

	if (pinst->pgf) {
	    AVIStreamGetFrameClose(pinst->pgf);
	    pinst->pgf = 0;
	}

	if (pinst->pavi) {
	    // Release our hold on the uncompressed stream....
	    AVIStreamClose(pinst->pavi);
	}

	if (pinst->lpFormat)
	    GlobalFreePtr(pinst->lpFormat);

	if (pinst->lpFormatOrig)
	    GlobalFreePtr(pinst->lpFormatOrig);

	delete pinst;
	return 0;
    }

    return m_refs;
}


STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Info(AVISTREAMINFO FAR * psi, LONG lSize)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;
    
    hmemcpy(psi, &pinst->avistream, min(lSize, sizeof(pinst->avistream)));
    
//    return sizeof(pinst->avistream);
    return ResultFromScode(0);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::ReadFormat(LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;

    LPBITMAPINFOHEADER	lpbi;

    if (!pinst->pgf) {
	HRESULT	    hr;
	hr = pinst->SetUpCompression();

	if (hr != NOERROR)
	    return hr;
    }
    
    lpbi = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pinst->pgf, lPos);

    if (!lpbi)
	return ResultFromScode(AVIERR_MEMORY);
    
    if (pinst->hic == 0) {
	pinst->cbFormat = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

	if (lpFormat)
	    hmemcpy(lpFormat, lpbi, min(*lpcbFormat, (LONG) pinst->cbFormat));
    } else {
	if (lpFormat) {
	    hmemcpy(lpFormat, pinst->lpFormat, min(*lpcbFormat, (LONG) pinst->cbFormat));

	    if (pinst->lpFormat->biClrUsed > 0) {
		// Make sure we have the right colors!
		// !!! This is bad--We may need to restart the compressor...
		hmemcpy((LPBYTE) lpFormat + pinst->lpFormat->biSize,
			(LPBYTE) lpbi + lpbi->biSize,
			pinst->lpFormat->biClrUsed * sizeof(RGBQUAD));
	    }
	}
    }
    
    *lpcbFormat = pinst->cbFormat;
    return AVIERR_OK;
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Read(
                      LONG       lStart,
                      LONG       lSamples,
                      LPVOID     lpBuffer,
                      LONG       cbBuffer,
                      LONG FAR * plBytes,
                      LONG FAR * plSamples)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;
    LPBITMAPINFOHEADER	lpbi;
    LONG		lRet;

    if (!pinst->pgf) {
	HRESULT	    hr;
	hr = pinst->SetUpCompression();

	if (hr != NOERROR)
	    return hr;
    }
    
    if (pinst->hic == 0) {
	lpbi = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pinst->pgf, lStart);

	if (!lpbi)
	    return ResultFromScode(AVIERR_MEMORY);

	if (plBytes)
	    *plBytes = lpbi->biSizeImage;

	if ((LONG) lpbi->biSizeImage > cbBuffer)
	    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
	
	if (lpBuffer)
	    hmemcpy(lpBuffer, DIBPTR(lpbi), min((DWORD) cbBuffer, lpbi->biSizeImage));

	if (plSamples)
	    *plSamples = 1;

	return AVIERR_OK;
    }
    
    if (lStart < pinst->lFrameCurrent)
	pinst->ResetInst();

    while (pinst->lFrameCurrent < lStart) {
	++pinst->lFrameCurrent;
    
	lpbi = (LPBITMAPINFOHEADER) AVIStreamGetFrame(pinst->pgf, pinst->lFrameCurrent);

	if (lpbi == NULL) {
	    pinst->ResetInst();	// Make sure we don't assume anything
	    return ResultFromScode(AVIERR_INTERNAL);
	}
	
	// !!! Check if format has changed!

	lRet = pinst->ICCrunch(lpbi, DIBPTR(lpbi));
	if (lRet != AVIERR_OK) {
	    pinst->ResetInst();	// Make sure we don't assume anything
	    return ResultFromScode(AVIERR_INTERNAL);    // !!! error < 0.
	}
    }

    if (plBytes)
	*plBytes = pinst->lpbiC->biSizeImage;

    if ((LONG) pinst->lpbiC->biSizeImage > cbBuffer)
	return ResultFromScode(AVIERR_BUFFERTOOSMALL);
	
    if (lpBuffer)
	hmemcpy(lpBuffer, pinst->lpC,
		min((DWORD) cbBuffer, pinst->lpbiC->biSizeImage));

    if (plSamples)
	*plSamples = 1;
    
    return AVIERR_OK;
}

STDMETHODIMP_(LONG) CAVICmpStream::CAVICmpStreamImpl::FindSample(LONG lPos, LONG lFlags)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;
    if (lFlags & FIND_KEY) {
	if (pinst->hic == 0)
	    return lPos;
	    
	if (lFlags & FIND_PREV) {
	    /* If the frame they're asking about isn't the one we have,
	    ** we have to go actually do the work and find out.
	    */
	    if (lPos < pinst->lLastKeyFrame || lPos > pinst->lFrameCurrent)
		Read(lPos, 1, NULL, 0, NULL, NULL);

	    return pinst->lLastKeyFrame;
	} else {
	    return -1; // !!! Find Next KeyFrame
	}
    }
    if (lFlags & FIND_ANY) {
	return lPos;
    }
    if (lFlags & FIND_FORMAT) {
	// !!! This is wrong in the case where we're compressing something
	// with a palette change and the compressor preserves it....
	if (lFlags & FIND_PREV)
	    return 0;
	else
	    return -1;
    }

    return -1;

}

/////////////////////////////////////////////////////////////////////////////
//
//  ICCrunch()
//
//  crunch a frame and make it fit into the specifed size, by varing the
//  quality.  the suplied quality is the upper bound.
//
//  if the compressor can crunch, then let it crunch
//
//  if the compressor does quality, then vary the quality
//
//  if the compressor does not do quality, then the caller gets what
//  ever it will do.
//
//
//  The frame to be compressed is passed in in lpbi.
//
//  The compressed frame can be found in the lpC member variable....
//
/////////////////////////////////////////////////////////////////////////////

LONG CAVICmpStream::ICCrunch(LPBITMAPINFOHEADER lpbi, LPVOID lp)
{
    DWORD   dw;
    DWORD   dwFlags;
    DWORD   dwSize;
    DWORD   ckid;
    DWORD   dwQuality = avistream.dwQuality;
    DWORD   dwQualityMin;
    DWORD   dwQualityMax;
    DWORD   dwMaxSizeThisFrame;
    DWORD   dwSizeMin;
    DWORD   dwSizeMax;
    BOOL    fKeyFrame=FALSE;
    BOOL    fCrunch;            /* are we crunching? */
    BOOL    fFirst=TRUE;

    dwMaxSizeThisFrame = dwMaxSize;
    
    if (lFrameCurrent == 0 || (dwKeyFrameEvery != 0 &&
	    lFrameCurrent - lLastKeyFrame >= (long)dwKeyFrameEvery)) {
        fKeyFrame = TRUE;
    }

    //
    //  give the key frames more space, and take some away from the
    //  non key frames.
    //
    //  give the key frame two shares, assuming we have more frames to
    //  go around.
    //
    if (dwKeyFrameEvery > 0) {
	if (lFrameCurrent == 0) {
	    dwMaxSizeThisFrame = 0xffffff;
	} else if (fKeyFrame) {
            dwMaxSizeThisFrame = dwMaxSizeThisFrame + dwSaved;
	    dwSaved = 0;
        } else {
	    DWORD	dwTakeAway;

	    dwTakeAway = dwMaxSizeThisFrame / dwKeyFrameEvery;
	    if (dwSaved > dwMaxSizeThisFrame)
		dwTakeAway = 0;
	    
	    /* If we're padding, take away a multiple of 2K. */
	    if (fPad) {
		if (dwMaxSizeThisFrame > dwTakeAway + 2048)
		    dwTakeAway += 2047;
		dwTakeAway -= dwTakeAway % 2048;
	    }
            dwMaxSizeThisFrame -= dwTakeAway; 
	    dwSaved += dwTakeAway;

	    if (!fPad) {
		/* Try to give a little extra space to each frame */
		dwMaxSizeThisFrame += dwSaved / dwKeyFrameEvery;
		dwSaved -= dwSaved / dwKeyFrameEvery;
	    }
        }
    } else {
        // the only key frame is frame zero
        if (lFrameCurrent == 0)
            dwMaxSizeThisFrame = 0xffffff;
	else {
	    /* Give each frame whatever extra there is.... */
	    dwMaxSizeThisFrame += dwSaved;
	    dwSaved = 0;
	}
    }

    //
    //  if the device supports crunching or does not do quality we dont
    //  crunch.
    //
    fCrunch = dwMaxSizeThisFrame > 0 && !(dwICFlags & VIDCF_CRUNCH) &&
         (dwICFlags & VIDCF_QUALITY);

////if (lFrameCurrent > 0 && fCrunch)
////    dwQuality = dwQualityLast;

    DPF("ICCrunch: Frame %ld, Quality = %ld, MaxSize = %ld\n", lFrameCurrent, avistream.dwQuality, dwMaxSizeThisFrame);

    dwQualityMin = 0;
    dwQualityMax = dwQuality;

    dwSizeMin = 0;
    dwSizeMax = dwMaxSizeThisFrame;

    for (;;) {
        ckid = 0L;
        dwFlags = fKeyFrame ? AVIIF_KEYFRAME : 0;

        //
        //  compress the frame
        //
        dw = ICCompress(hic,
                0,              // flags
                lpbiC,          // ouput format
		lpC,            // output data
                lpbi,           // format of frame to compress
                lp,	        // frame data to compress
                &ckid,          // ckid for data in AVI file
                &dwFlags,       // flags in the AVI index.
                lFrameCurrent,  // frame number of seq.
                dwMaxSizeThisFrame,	// reqested size in bytes. (if non zero)
                dwQuality,	// quality value
                fKeyFrame ? NULL : lpbiU,
                fKeyFrame ? NULL : lpU);

        if (dw != ICERR_OK)
            break;

        dwSize = lpbiC->biSizeImage;

	DPF("                     Quality = %ld, Size = %ld, %c\n", dwQuality, dwSize, (dwFlags & AVIIF_KEYFRAME) ? 'K' : ' ');

        //
        // if the device can't crunch (does not do it it self, or does not do
        // quality) then we are done.
        //
        if (!fCrunch)
            break;

        //
        //  we are crunching, see if the frame fit.
        //
        if (dwSize <= dwMaxSizeThisFrame) {
            dwQualityMin = dwQuality;
            dwSizeMin = dwSize;

            //
            //  when the quality gets too close bail out.
            //
            if (dwQualityMax - dwQualityMin <= 10)
                break;

            //
            //  if we get within 512 bytes it is good enough
            //
            if ((LONG) (dwMaxSizeThisFrame - dwSize) <= (LONG) min(512L, dwMaxSizeThisFrame / 8L))
                break;

            //
            // if the first try, (with the user specifed quality) made it
            // then use it.  otherwise we need to search.
            //
            if (fFirst)
                break;
        }
        else {
	    //
	    //  when the quality gets too close bail out.
	    //
	    if (dwQualityMax - dwQualityMin <= 1)
		break;

            dwQualityMax = dwQuality;
            dwSizeMax = dwSize;
        }

        if (fFirst && dwQuality != dwQualityLast)
            dwQuality = dwQualityLast;
        else
            dwQuality = (dwQualityMin + dwQualityMax) / 2;

#if 0
            //
            // make a guess based on how close we are now.
            //
            dwQuality = dwQualityMin + muldiv32(dwQualityMax-dwQualityMin,
                        dwMaxSizeThisFrame-dwSizeMin,dwSizeMax-dwSizeMin);
#endif
        fFirst = FALSE;
    }

#if 0
    /* If this wasn't the first frame, save up any extra space for later */
    if (dwSize < dwMaxSizeThisFrame && lFrameCurrent > 0) {
	dwSaved += dwMaxSizeThisFrame - dwSize;
	if (fPad) {
	    dwSaved -= ((dwMaxSizeThisFrame - dwSize) % 2048L);
	}

	// HACK: limit this, so it doesn't get too big!!!
	if (dwSaved > 32768L)
	    dwSaved = 32768L;
	if (dwSaved > dwMaxSizeThisFrame * 5)
	    dwSaved = dwMaxSizeThisFrame * 5;
    }
#endif

    if (dw != ICERR_OK) {
        if (dw == ICERR_BADFORMAT)
	    return AVIERR_BADFORMAT;
        else
	    return AVIERR_INTERNAL;
    }

    if (dwFlags & AVIIF_KEYFRAME) {
        lLastKeyFrame = lFrameCurrent;
    }

    //
    // remember the quality that worked, it will be the best guess next time.
    //
    dwQualityLast = dwQuality;

    //
    //  decompress the image into the offscreen buffer, for use next time.
    //
    if (dwKeyFrameEvery != 1 && lpbiU) {
        dw = ICDecompress(hic, 0,
            lpbiC,lpC,
            lpbiU,lpU);

	// !!! error check?
    }

    //
    // return the dwFlags and ckid, by stuffing them in the stream info.
    //
    m_ckid = ckid;
    m_dwFlags = dwFlags;

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

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::SetFormat(LONG lPos,LPVOID lpFormat,LONG cbFormat)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;

    LONG		lRet = AVIERR_OK;
    HRESULT		hr;
    LPBITMAPINFOHEADER  lpbi = (LPBITMAPINFOHEADER) lpFormat;
    DWORD		dw;
    
    if (pinst->pgf)
	return ResultFromScode(AVIERR_UNSUPPORTED);
	
    if (lpbi->biCompression != BI_RGB)
	return ResultFromScode(AVIERR_UNSUPPORTED);
    
    if (pinst->avistream.fccHandler == comptypeDIB)
	goto exit;

    if (pinst->lpFormatOrig) {
	if ((cbFormat = pinst->cbFormatOrig) &&
	    (_fmemcmp(pinst->lpFormatOrig, lpFormat, (int) cbFormat) == 0))
	    return AVIERR_OK;

	DPF("AVICmprs: SetFormat when format already set!\n");
    }
    
    //
    // Can only currently set the palette at the end of the file
    //
    if (lPos < (LONG) (pinst->avistream.dwStart + pinst->avistream.dwLength))
	return ResultFromScode(AVIERR_UNSUPPORTED);

    if (pinst->lpFormatOrig) {
	//
	// We can only change the palette for things with palettes....
	//
	if (lpbi->biBitCount > 8 || lpbi->biClrUsed == 0)
	    return ResultFromScode(AVIERR_UNSUPPORTED);

	//
	// Be sure only the palette is changing, nothing else....
	//
	if (cbFormat != pinst->cbFormatOrig)
	    return ResultFromScode(AVIERR_UNSUPPORTED);

	if (!DibEq((LPBITMAPINFOHEADER) lpFormat,
		   (LPBITMAPINFOHEADER) pinst->lpFormatOrig))
	    return ResultFromScode(AVIERR_UNSUPPORTED);

	dw = ICCompressGetFormat(pinst->hic, lpFormat, pinst->lpFormat);
	if ((LONG) dw < 0)
	    goto ic_error;
    
	ICCompressEnd(pinst->hic);
	dw = ICCompressBegin(pinst->hic, lpFormat, pinst->lpFormat);

	if (dw != ICERR_OK)
	    goto ic_error;

	
	if (pinst->dwKeyFrameEvery != 1 && pinst->lpbiU) {
	    ICDecompressEnd(pinst->hic);

	    dw = ICDecompressGetFormat(pinst->hic, pinst->lpFormat, pinst->lpbiU);

	    if ((LONG) dw < 0)
		goto ic_error;

	    dw = ICDecompressBegin(pinst->hic, pinst->lpFormat, pinst->lpbiU);

	    if (dw != ICERR_OK)
		goto ic_error;
	}

	goto setformatandexit;
    }


    pinst->lpFormatOrig = (LPBITMAPINFOHEADER)
	GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, cbFormat);
    pinst->cbFormatOrig = cbFormat;
    
    if (!pinst->lpFormatOrig) {
	lRet = AVIERR_MEMORY;
	goto exit;
    }

    hmemcpy(pinst->lpFormatOrig, lpFormat, cbFormat);
    
    /*
    ** get the size requied to hold the format.
    */
    dw = ICCompressGetFormatSize(pinst->hic, lpFormat);
    if ((LONG) dw < sizeof(BITMAPINFOHEADER))
	goto ic_error;

    pinst->cbFormat = dw;
    pinst->lpFormat = (LPBITMAPINFOHEADER) GlobalAllocPtr(GHND | GMEM_SHARE, pinst->cbFormat);
    if (!pinst->lpFormat) {
	lRet = AVIERR_MEMORY;
	goto exit;
    }

    /*
    ** get the compressed format from the compressor.
    */
    dw = ICCompressGetFormat(pinst->hic, lpFormat, pinst->lpFormat);
    if ((LONG) dw < 0)
	goto ic_error;
    
    pinst->avistream.rcFrame.right = pinst->avistream.rcFrame.left +
					  (int) pinst->lpFormat->biWidth;
    pinst->avistream.rcFrame.bottom = pinst->avistream.rcFrame.top +
					  (int) pinst->lpFormat->biHeight;
    
    dw = ICCompressBegin(pinst->hic, lpFormat, pinst->lpFormat);
    
    if (dw != ICERR_OK)
	goto ic_error;

    /*
    ** allocate buffer to hold compressed data.
    */
    dw = ICCompressGetSize(pinst->hic, lpFormat, pinst->lpFormat);

    pinst->lpbiC = (LPBITMAPINFOHEADER)
	GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE, pinst->cbFormat + dw);
    
    if (!pinst->lpbiC) {
	lRet = AVIERR_MEMORY;
	goto exit;
    }

    hmemcpy((LPVOID)pinst->lpbiC, pinst->lpFormat, pinst->cbFormat);

    pinst->lpC = (LPSTR) pinst->lpbiC + pinst->lpbiC->biSize +
				pinst->lpbiC->biClrUsed * sizeof(RGBQUAD);
	
    //
    //  check for temporal compress, and alocate a previous
    //  DIB buffer if needed
    //
    if (pinst->dwKeyFrameEvery != 1) {
	pinst->lpbiU = (LPBITMAPINFOHEADER)
	    GlobalAllocPtr(GMEM_MOVEABLE | GMEM_SHARE,
		    sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));

	if (!pinst->lpbiU) {
	    lRet = AVIERR_MEMORY;
	    goto exit;
	}

	dw = ICDecompressGetFormat(pinst->hic, pinst->lpFormat, pinst->lpbiU);

	if ((LONG) dw < 0)
	    goto ic_error;

	if (pinst->lpbiU->biSizeImage == 0)
	    pinst->lpbiU->biSizeImage = pinst->lpbiU->biHeight *
					    DIBWIDTHBYTES(*pinst->lpbiU);

	pinst->lpbiU = (LPBITMAPINFOHEADER)
	    GlobalReAllocPtr(pinst->lpbiU,
		pinst->lpbiU->biSize +
			pinst->lpbiU->biClrUsed * sizeof(RGBQUAD) +
			pinst->lpbiU->biSizeImage,
		GMEM_MOVEABLE | GMEM_SHARE);

	if (!pinst->lpbiU) {
	    lRet = AVIERR_MEMORY;
	    goto exit;
	}

	pinst->lpU = (LPSTR) pinst->lpbiU + pinst->lpbiU->biSize +
				pinst->lpbiU->biClrUsed * sizeof(RGBQUAD);
	
	dw = ICDecompressBegin(pinst->hic, pinst->lpFormat, pinst->lpbiU);

	if (dw != ICERR_OK)
	    goto ic_error;
    }

setformatandexit:
    hr = AVIStreamSetFormat(pinst->pavi, lPos,
			    pinst->lpFormat, pinst->cbFormat);

    if (hr != NOERROR)
	return hr;

exit:
    if (lRet != AVIERR_OK)
	// Clean up before returning...
	;
    
    return ResultFromScode(lRet);
    
ic_error:
    if (dw == ICERR_BADFORMAT)
	lRet = AVIERR_BADFORMAT;
    else if (dw == ICERR_MEMORY)
	lRet = AVIERR_MEMORY;
    else
	lRet = AVIERR_INTERNAL;
    goto exit;
} 

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Write(LONG lStart,
						     LONG lSamples,
						     LPVOID lpBuffer,
						     LONG cbBuffer,
						     DWORD dwFlags,
						     LONG FAR *plSampWritten,
						     LONG FAR *plBytesWritten)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;

    LONG		lRet;
    
    if (pinst->pgf)
	return ResultFromScode(AVIERR_UNSUPPORTED);
	
    if (lStart < (LONG) (pinst->avistream.dwStart + pinst->avistream.dwLength))
	return ResultFromScode(AVIERR_UNSUPPORTED);
    
    if (lSamples > 1)
	return ResultFromScode(AVIERR_UNSUPPORTED);

    pinst->lFrameCurrent = lStart;

    if (pinst->avistream.fccHandler == comptypeDIB) {
	dwFlags |= AVIIF_KEYFRAME;
    } else {
	lRet = pinst->ICCrunch(pinst->lpFormatOrig, lpBuffer);
	if (lRet != AVIERR_OK)
	    return ResultFromScode(lRet);
	lpBuffer = pinst->lpC;
	cbBuffer = pinst->lpbiC->biSizeImage;
	dwFlags = pinst->lLastKeyFrame == lStart ? AVIIF_KEYFRAME : 0;
    }

    return AVIStreamWrite(pinst->pavi,
			  lStart,
			  lSamples,
			  lpBuffer,
			  cbBuffer,
			  dwFlags,
			  plSampWritten,
			  plBytesWritten);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Delete(LONG lStart,LONG lSamples)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;

    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::ReadData(DWORD fcc, LPVOID lp, LONG FAR *lpcb)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;

    // Don't pass through 'strd' data!
    if (fcc == ckidSTREAMHANDLERDATA) {
	if (pinst->cbHandler) {
	    hmemcpy(lp, pinst->lpHandler, min(*lpcb, pinst->cbHandler));
	}
	*lpcb = pinst->cbHandler;
	return AVIERR_OK;
    }
    
    return AVIStreamReadData(pinst->pavi, fcc, lp, lpcb);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::WriteData(DWORD fcc, LPVOID lp, LONG cb)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;

    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#if 0
STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Clone(PAVISTREAM FAR * ppaviNew)
{
    CAVICmpStream FAR * pinst = m_pAVIStream;

    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#endif


STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Reserved1(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Reserved2(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Reserved3(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Reserved4(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVICmpStream::CAVICmpStreamImpl::Reserved5(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
