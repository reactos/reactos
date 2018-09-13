/****************************************************************************
 *
 *  AVIMEM.C
 *
 *  routine for putting a stream interface on top of data in memory
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
#include <compman.h>
#include <compobj.h>
#include <avifmt.h>
#include "avifile.h"
#include "avifilei.h"
#include "avimem.h"

#define WIDTHBYTES(i)       ((UINT)((i+31)&(~31))/8)
#define DIBWIDTHBYTES(lpbi) (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)(lpbi)->biBitCount)

STDAPI AVIMakeStreamFromClipboard(UINT cfFormat, HANDLE hGlobal, PAVISTREAM FAR *ppstream)
{
    CAVIMemStream FAR*	pAVIStream;
    HRESULT		hr;
    LPVOID		lp;

    if (cfFormat != CF_DIB && cfFormat != CF_WAVE)
	return ResultFromScode(AVIERR_UNSUPPORTED);
    
    pAVIStream = new FAR CAVIMemStream();
    if (!pAVIStream)
	return ResultFromScode(E_OUTOFMEMORY);

    lp = GlobalAllocPtr(GMEM_MOVEABLE, GlobalSize(hGlobal));
    if (!lp)
	return ResultFromScode(E_OUTOFMEMORY);

    hmemcpy(lp, GlobalLock(hGlobal), GlobalSize(hGlobal));
	
    pAVIStream->Create((LONG) cfFormat, (LONG) lp);
    
    hr = pAVIStream->QueryInterface(IID_IAVIStream, (LPVOID FAR *) ppstream);
    if (FAILED(GetScode(hr)))
	delete pAVIStream;
    return hr;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIMemStream::CAVIMemStream()
{
    m_lpData = NULL;
    m_lpMemory = NULL;
    m_lpFormat = NULL;
    m_refs = 0;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIMemStream::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
    if (iid == IID_IUnknown)
	*ppv = this;
    else if (iid == IID_IAVIStream)
	*ppv = this;
    else
	return ResultFromScode(E_NOINTERFACE);
    AddRef();
    return AVIERR_OK;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIMemStream::AddRef()
{
    uUseCount++;
    return ++m_refs;
}


/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIMemStream::Create(LONG lParam1, LONG lParam2)
{
    UINT    cfFormat = (UINT) lParam1;
    m_lpMemory = (LPVOID) lParam2;

    if (cfFormat == CF_DIB) {
	LPBITMAPINFOHEADER lpbi;
	
	m_lpFormat = m_lpMemory;
	lpbi = (LPBITMAPINFOHEADER) m_lpFormat;

	if (lpbi->biSizeImage == 0) {
	    if (lpbi->biCompression = BI_RGB) {
		lpbi->biSizeImage = DIBWIDTHBYTES(lpbi) *
				    lpbi->biHeight;
	    }
	}

	_fmemset(&m_avistream, 0, sizeof(m_avistream));
	m_avistream.fccType = streamtypeVIDEO;
	m_avistream.fccHandler = 0;
	m_avistream.dwStart = 0;
	m_avistream.dwLength = 1;
	m_avistream.dwScale = 1;
	m_avistream.dwRate = 15;
	m_avistream.dwSampleSize = 0;
	SetRect(&m_avistream.rcFrame, 0, 0,
		(int) lpbi->biWidth,
		(int) lpbi->biHeight);
	
	m_cbFormat = lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);
	m_lpData = (LPBYTE) m_lpMemory + m_cbFormat;
	m_cbData = lpbi->biSizeImage;
    } else if (cfFormat == CF_WAVE) {
	DWORD _huge * lpdw;
	LPWAVEFORMAT lpwf;
#define ckidWAVEFORMAT          mmioFOURCC('f', 'm', 't', ' ')
#define ckidWAVEDATA	        mmioFOURCC('d', 'a', 't', 'a')

	lpdw = (DWORD _huge *) ((LPBYTE) m_lpMemory + 12);

	while (*lpdw != ckidWAVEFORMAT)
	    lpdw = (DWORD _huge *)
		   (((BYTE _huge *) lpdw) + lpdw[1] + sizeof(DWORD) * 2);
	
	m_lpFormat = (LPBYTE) (lpdw + 2);
	m_cbFormat = lpdw[1];

	do {
	    lpdw = (DWORD _huge *)
		   (((BYTE _huge *) lpdw) + lpdw[1] + sizeof(DWORD) * 2);
	} while (*lpdw != ckidWAVEDATA);
	
	m_lpData = (LPBYTE) (lpdw + 2);
	m_cbData = lpdw[1];

	lpwf = (LPWAVEFORMAT) m_lpFormat;
	
	_fmemset(&m_avistream, 0, sizeof(m_avistream));
	m_avistream.fccType = streamtypeAUDIO;
	m_avistream.fccHandler = 0;
	m_avistream.dwStart = 0;
	m_avistream.dwSampleSize = lpwf->nBlockAlign;
	m_avistream.dwLength = m_cbData / m_avistream.dwSampleSize;
	m_avistream.dwScale = lpwf->nBlockAlign;
	m_avistream.dwRate = lpwf->nAvgBytesPerSec;
    }

    return 0;
}

STDMETHODIMP_(ULONG) CAVIMemStream::Release()
{
    uUseCount--;
    if (!--m_refs) {
	if (m_lpMemory) {
	    GlobalFreePtr(m_lpMemory);
	}

	delete this;
	return 0;
    }

    return m_refs;
}


STDMETHODIMP CAVIMemStream::Info(AVISTREAMINFO FAR * psi, LONG lSize)
{
    hmemcpy(psi, &m_avistream, min(lSize, sizeof(m_avistream)));
    
//    return sizeof(m_avistream);
    return ResultFromScode(0);
}

STDMETHODIMP CAVIMemStream::ReadFormat(LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    if (lpFormat) {
	hmemcpy(lpFormat, m_lpFormat, min(*lpcbFormat, (LONG) m_cbFormat));
    }
    
    *lpcbFormat = m_cbFormat;
    return AVIERR_OK;
}

STDMETHODIMP CAVIMemStream::Read(
                      LONG       lStart,
                      LONG       lSamples,
                      LPVOID     lpBuffer,
                      LONG       cbBuffer,
                      LONG FAR * plBytes,
                      LONG FAR * plSamples)
{
    // !!! CONVENIENT?
    if (lStart + lSamples > (LONG) m_avistream.dwLength)
	lSamples = (LONG) m_avistream.dwLength - lStart;
	
    if (lSamples == 0 || lStart >= (LONG) m_avistream.dwLength) {
	if (plBytes)
	    *plBytes = 0;
	if (plSamples)
	    *plSamples = 0;
    }
    
    if (m_avistream.dwSampleSize) {
	if (lSamples > 0)
	    lSamples = min(lSamples, cbBuffer / (LONG) m_avistream.dwSampleSize);
	else
	    lSamples = cbBuffer / m_avistream.dwSampleSize;

	if (plBytes)
	    *plBytes = lSamples * m_avistream.dwSampleSize;

	if (plSamples)
	    *plSamples = lSamples;

	hmemcpy(lpBuffer,
		(BYTE _huge *) m_lpData + lStart * m_avistream.dwSampleSize,
		lSamples * m_avistream.dwSampleSize);
	
	if (cbBuffer < (LONG) m_avistream.dwSampleSize)
	    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
    } else {
	if (plBytes)
	    *plBytes = m_cbData;

	if (plSamples)
	    *plSamples = 1;
	if (lpBuffer) {
	    hmemcpy(lpBuffer, m_lpData, min(cbBuffer, m_cbData));

	    if (cbBuffer < m_cbData)
		return ResultFromScode(AVIERR_BUFFERTOOSMALL);
	}
    }
    
    return AVIERR_OK;
}

STDMETHODIMP_(LONG) CAVIMemStream::FindSample(LONG lPos, LONG lFlags)
{
    if (lFlags & FIND_FORMAT) {
	if (lFlags & FIND_PREV)
	    return 0;
	else {
	    if (lPos > 0)
		return -1;
	    else
		return 0;   
	}
    }
    return lPos;
}


STDMETHODIMP CAVIMemStream::SetFormat(LONG lPos,LPVOID lpFormat,LONG cbFormat)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::Write(LONG lStart,
						     LONG lSamples,
						     LPVOID lpBuffer,
						     LONG cbBuffer,
						     DWORD dwFlags,
						     LONG FAR *plSampWritten,
						     LONG FAR *plBytesWritten)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::Delete(LONG lStart,LONG lSamples)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::ReadData(DWORD fcc, LPVOID lp, LONG FAR *lpcb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::WriteData(DWORD fcc, LPVOID lp, LONG cb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#if 0
STDMETHODIMP CAVIMemStream::Clone(PAVISTREAM FAR * ppaviNew)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#endif


STDMETHODIMP CAVIMemStream::Reserved1(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::Reserved2(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::Reserved3(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::Reserved4(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CAVIMemStream::Reserved5(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
