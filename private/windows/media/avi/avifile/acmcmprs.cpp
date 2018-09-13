/****************************************************************************
 *
 *  ACMSTRM.C
 *
 *  routine for compressing audio with the ACM
 *
 *  Copyright (c) 1992 - 1995 Microsoft Corporation.  All Rights Reserved.
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
// Given an audio Stream (that is, essentially, a function that it can call
// to get audio samples), this presents the same sort of interface and allows
// other people to call it to get compressed audio.
//

#include <win32.h>
#include <vfw.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <mmreg.h>
#include <msacm.h>
#include "avifilei.h"	// uUseCount
#include "acmcmprs.h"
#include "avifile.rc"	// for resource ids
#include "debug.h"

EXTERN_C HINSTANCE ghMod;


#define WAVEFORMATSIZE(pwf) \
	((((LPWAVEFORMAT)(pwf))->wFormatTag == WAVE_FORMAT_PCM) ? \
		sizeof(PCMWAVEFORMAT) : \
		sizeof(WAVEFORMATEX) + ((LPWAVEFORMATEX)(pwf))->cbSize)


HRESULT CACMCmpStream::MakeInst(
	IUnknown FAR*	pUnknownOuter,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
{
    IUnknown FAR*	pUnknown;
    CACMCmpStream FAR* pAVIStream;
    HRESULT	hresult;

    pAVIStream = new FAR CACMCmpStream(pUnknownOuter, &pUnknown);
    if (!pAVIStream)
	return ResultFromScode(E_OUTOFMEMORY);
    hresult = pUnknown->QueryInterface(riid, ppv);
    if (FAILED(GetScode(hresult)))
	delete pAVIStream;
    return hresult;
}

/*	-	-	-	-	-	-	-	-	*/

CACMCmpStream::CACMCmpStream(
	IUnknown FAR*	pUnknownOuter,
	IUnknown FAR* FAR* ppUnknown)
{
    m_pavi = 0;
    m_hs = 0;
    m_lpFormat = 0;
    m_lpFormatC = 0;
    m_lpIn = 0;
    m_lpOut = 0;

    if (pUnknownOuter)
	m_pUnknownOuter = pUnknownOuter;
    else
	m_pUnknownOuter = this;
    *ppUnknown = this;
    m_refs = 0;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CACMCmpStream::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
    if (iid == IID_IUnknown)
	*ppv = (IUnknown FAR *) this;
    else if (iid == IID_IAVIStream)
	*ppv = (IAVIStream FAR *) this;
    else {
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
    }
    AddRef();
    return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CACMCmpStream::AddRef()
{
    uUseCount++;
    return ++m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

LONG CACMCmpStream::SetUpCompression()
{
    LONG	    	    lRet = AVIERR_OK;
    MMRESULT		err;


    // Get the initial format
    AVIStreamFormatSize(m_pavi, AVIStreamStart(m_pavi), &m_cbFormat);
    m_lpFormat = (LPWAVEFORMATEX) GlobalAllocPtr(GHND | GMEM_SHARE, m_cbFormat);
    if (!m_lpFormat) {
	lRet = AVIERR_MEMORY;
	goto exit;
    }
    AVIStreamReadFormat(m_pavi, AVIStreamStart(m_pavi), m_lpFormat, &m_cbFormat);

    if (m_lpFormatC != NULL) {
	// we already have the format, let's hope it works...

	// We could check if the format matches the original format....
	if (m_cbFormat == m_cbFormatC &&
		(_fmemcmp(m_lpFormat, m_lpFormatC, (int) m_cbFormat) == 0))
	    goto sameformat;
	
    } else if (m_lpFormat->wFormatTag != WAVE_FORMAT_PCM) {
	acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, (LPVOID)&m_cbFormatC);
	m_lpFormatC = (LPWAVEFORMATEX) GlobalAllocPtr(GHND | GMEM_SHARE, m_cbFormatC);
	if (!m_lpFormatC) {
	    lRet = AVIERR_MEMORY;
	    goto exit;
	}

	m_lpFormatC->wFormatTag = WAVE_FORMAT_PCM;
	
	if (acmFormatSuggest(NULL, m_lpFormat, m_lpFormatC, m_cbFormatC, 0L) != 0)
	    goto sameformat;
    } else {
sameformat:
	DPF("Leaving the format unchanged....\n");
	m_lpFormatC = m_lpFormat;
	m_cbFormatC = m_cbFormat;
	m_lpFormat = NULL;
	m_cbFormat = 0;

	m_hs = (HACMSTREAM) -1;
	
	goto exit;
    }

    ACMFORMATDETAILS	afdU;
    ACMFORMATTAGDETAILS	aftdU;
    ACMFORMATDETAILS	afdC;
    ACMFORMATTAGDETAILS	aftdC;

    afdU.cbStruct = sizeof(afdU);
    afdU.pwfx = m_lpFormat;
    afdU.cbwfx = m_cbFormat;
    afdU.fdwSupport = 0;
    afdU.dwFormatTag = m_lpFormat->wFormatTag;

    acmFormatDetails(NULL, &afdU, ACM_FORMATDETAILSF_FORMAT);

    aftdU.cbStruct = sizeof(aftdU);
    aftdU.dwFormatTag = m_lpFormat->wFormatTag;
    aftdU.fdwSupport = 0;

    acmFormatTagDetails(NULL,
			&aftdU, ACM_FORMATTAGDETAILSF_FORMATTAG);

    afdC.cbStruct = sizeof(afdC);
    afdC.pwfx = m_lpFormatC;
    afdC.cbwfx = m_cbFormatC;
    afdC.dwFormatTag = m_lpFormatC->wFormatTag;
    afdC.fdwSupport = 0;

    acmFormatDetails(NULL, &afdC, ACM_FORMATDETAILSF_FORMAT);

    aftdC.cbStruct = sizeof(aftdC);
    aftdC.dwFormatTag = m_lpFormatC->wFormatTag;
    aftdC.fdwSupport = 0;

    acmFormatTagDetails(NULL,
			&aftdC,
			ACM_FORMATTAGDETAILSF_FORMATTAG);

    DPF("Converting %s %s to %s %s\n", (LPSTR) &aftdU.szFormatTag, (LPSTR) &afdU.szFormat, (LPSTR) &aftdC.szFormatTag, (LPSTR) &afdC.szFormat);

    // Open the compressor they asked for...
    lRet = acmStreamOpen(&m_hs,		    // returned stream handle
			 NULL,		    // use any converter you want
			 m_lpFormat,	    // starting format
			 m_lpFormatC,	    // ending format
			 0L,		    // no filter
			 0L,		    // no callback
			 0L,		    // instance data for callback
			 ACM_STREAMOPENF_NONREALTIME);//emph. quality not speed


    // !!! translate error code

    if (!m_hs) {
	DPF("Unable to convert!\n");
#if 0
	TCHAR *		pachMessage;
	TCHAR		achTemp[128];
	static int	iEntered = 0;
	LPTSTR		aStrings[4];
// !!! This isn't defined in dev\inc\windows.h.  I don't understand why.
#ifndef FORMAT_MESSAGE_ARGUMENT_ARRAY
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0
#endif

	aStrings[0] = (LPTSTR) &aftdU.szFormatTag;
	aStrings[1] = (LPTSTR) &afdU.szFormat;
	aStrings[2] = (LPTSTR) &aftdC.szFormatTag;
	aStrings[3] = (LPTSTR) &afdC.szFormat;
	
	FormatMessage(FORMAT_MESSAGE_FROM_HMODULE |
		      FORMAT_MESSAGE_ALLOCATE_BUFFER |
		      FORMAT_MESSAGE_ARGUMENT_ARRAY,
		      (LPVOID) ghInst,
		      IDS_CNVTERR,
		      0,  // !!! GetSystemDefaultLanguage?
		      (LPTSTR) &pachMessage,
		      999,
		      (LPDWORD) &aStrings);  // !!! needs to be va_list for NT?
	
	LoadString(ghInst, IDS_ACMERR, (LPTSTR)achTemp, sizeof(achTemp)/sizeof(TCHAR));
	
	if (iEntered++ == 0)	
	    MessageBox(NULL, pachMessage, achTemp, MB_OK);
	iEntered--;

	LocalFree((HLOCAL) pachMessage);
#endif
	
	lRet = AVIERR_ERROR;
	goto exit;
    }

    // Fix avistream header
    m_avistream.dwSampleSize = m_lpFormatC->nBlockAlign;
    m_avistream.dwScale = m_lpFormatC->nBlockAlign;
    m_avistream.dwRate = m_lpFormatC->nAvgBytesPerSec;

    acmStreamSize(m_hs,
		  AVIStreamLength(m_pavi) * m_lpFormat->nBlockAlign,
		  (LPDWORD) &m_avistream.dwLength,
		  ACM_STREAMSIZEF_SOURCE);

    // !!! acmStreamSize rounds up here, do we need to compensate?
    // !!! should we round off/up here?
    m_avistream.dwLength /= m_lpFormatC->nBlockAlign;

    m_avistream.dwQuality = 0; // !!!

    m_cbIn = 4096; // !!!
    m_cbIn -= m_cbIn % m_lpFormat->nBlockAlign; // round down to block aligned

    acmStreamSize(m_hs,
		  m_cbIn,
		  (LPDWORD) &m_cbOut,
		  ACM_STREAMSIZEF_SOURCE);

    DPF("ACM conversion: input %ld bytes, output %ld bytes\n", m_cbIn, m_cbOut);

    m_lpIn = (BYTE _huge *) GlobalAllocPtr(GHND, m_cbIn);
    m_lpOut = (BYTE _huge *) GlobalAllocPtr(GHND, m_cbOut);

    if (!m_lpIn || !m_lpOut) {
	lRet = AVIERR_MEMORY;
	goto exit;
    }

    m_acm.cbStruct = sizeof(m_acm);
    m_acm.fdwStatus = 0;
    m_acm.dwUser = 0;
    m_acm.pbSrc = m_lpIn;
    m_acm.cbSrcLength = m_cbIn;
    m_acm.cbSrcLengthUsed = 0;
    m_acm.pbDst = m_lpOut;
    m_acm.cbDstLength = m_cbOut;
    m_acm.cbDstLengthUsed = 0;

    // !!! add in start, end flags for ACM....
    err = acmStreamPrepareHeader(m_hs, &m_acm, 0);

    if (err != 0) {
	DPF("acmStreamPrepareHeader returns %u\n", err);

	return AVIERR_COMPRESSOR;
    }

    m_dwPosIn = m_dwPosOut = 0;

    m_dwSamplesLeft = 0;

exit:
    if (lRet != AVIERR_OK) {
	// Don't release here!
    }

    return lRet;
}

/*	-	-	-	-	-	-	-	-	*/

//
//  ACM stream:
//
//  lParam1 should be a PAVISTREAM (an audio one!)
//
//  lParam2 should be an LPWAVEFORMAT for the format you want converted
//  to.
//
STDMETHODIMP CACMCmpStream::Create(LPARAM lParam1, LPARAM lParam2)
{
    PAVISTREAM		    pavi = (PAVISTREAM) lParam1;
    LPAVICOMPRESSOPTIONS    lpOpts = (LPAVICOMPRESSOPTIONS) lParam2;
    LPWAVEFORMAT	    lpwfNew = NULL;
    LONG	    	    lRet = AVIERR_OK;

    DPF("Creating ACM compression stream....\n");
    // Get the stream header for future reference....
    pavi->Info(&m_avistream, sizeof(m_avistream));
    m_avistream.fccHandler = 0;
    if (m_avistream.fccType != streamtypeAUDIO) {
	DPF("Stream isn't audio!\n");
	lRet = AVIERR_INTERNAL;
	goto exit;
    }

    if (acmGetVersion() < 0x02000000L) {
	DPF("Bad ACM version!\n");
	lRet = AVIERR_INTERNAL;
	goto exit;
    }

    if (lpOpts && lpOpts->lpFormat) {
	lpwfNew = (LPWAVEFORMAT) lpOpts->lpFormat;

	if (lpOpts->cbFormat < WAVEFORMATSIZE(lpwfNew)) {
	    DPF("Bad format size!\n");
	    lRet = AVIERR_INTERNAL;
	    goto exit;
	}
	
	m_cbFormatC = WAVEFORMATSIZE(lpwfNew);
	m_lpFormatC = (LPWAVEFORMATEX) GlobalAllocPtr(GMEM_MOVEABLE, m_cbFormatC);
	if (m_lpFormatC == NULL) {
	    DPF("Out of memory for format!\n");
	    lRet = AVIERR_MEMORY;
	    goto exit;
	}

	hmemcpy(m_lpFormatC, lpOpts->lpFormat, m_cbFormatC);
    } else {
	m_cbFormatC = 0;
	m_lpFormatC = NULL;
    }

    // Make sure the uncompressed stream doesn't go away without our
    // knowledge....
    AVIStreamAddRef(pavi);

    // Don't put this in the structure until we've done the AddRef....
    m_pavi = pavi;

exit:
    return ResultFromScode(lRet);
}

STDMETHODIMP_(ULONG) CACMCmpStream::Release()
{
    uUseCount--;

    if (--m_refs)
	return m_refs;

    if (m_lpFormat) {
	GlobalFreePtr(m_lpFormat);

	if (m_hs) {
	    m_acm.cbSrcLength = m_cbIn;
	    acmStreamUnprepareHeader(m_hs, &m_acm, 0);
	
	    acmStreamClose(m_hs, 0);
	}
    }

    if (m_lpIn) {
	GlobalFreePtr(m_lpIn);
    }

    if (m_lpOut) {
	GlobalFreePtr(m_lpOut);
    }

    if (m_pavi) {
	// Release our hold on the uncompressed stream....
	AVIStreamClose(m_pavi);
    }

    if (m_lpFormatC)
	GlobalFreePtr(m_lpFormatC);

    delete this;

    return 0;
}


STDMETHODIMP CACMCmpStream::Info(AVISTREAMINFOW FAR * psi, LONG lSize)
{
    if (m_hs == 0) {
	LONG	lRet;
	
	// !!! If they ask for info before writing or setting the
	// format, this will become a "read" stream!
	lRet = SetUpCompression();

	if (lRet != 0)
	    return ResultFromScode(lRet);
    }

    hmemcpy(psi, &m_avistream, min(lSize, sizeof(m_avistream)));

//    return sizeof(m_avistream);
    return 0;
}

STDMETHODIMP CACMCmpStream::ReadFormat(LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    LONG    lRet;

    if (m_hs == 0) {
	lRet = SetUpCompression();

	if (lRet != 0)
	    return ResultFromScode(lRet);
    }

    if (lpFormat)
	hmemcpy(lpFormat,
		m_lpFormatC,
		min(*lpcbFormat, (LONG) m_cbFormatC));

    *lpcbFormat = (LONG) m_cbFormatC;
    return 0;
}

STDMETHODIMP CACMCmpStream::Read(
                      LONG       lStart,
                      LONG       lSamples,
                      LPVOID     lpBuffer,
                      LONG       cbBuffer,
                      LONG FAR * plBytes,
                      LONG FAR * plSamples)
{
    LONG		lRet;

    MMRESULT		err;

    HRESULT		hr;

    if (plBytes)
	*plBytes = 0;
    if (plSamples)
	*plSamples = 0;

    if (m_hs == 0) {
	lRet = SetUpCompression();

	if (lRet != 0)
	    return ResultFromScode(lRet);
    }

    if (m_lpFormat == NULL) {
	// Just return original format....
	return AVIStreamRead(m_pavi, lStart, lSamples,
			     lpBuffer, cbBuffer, plBytes, plSamples);
    }

    if (lStart < 0 || lStart > (LONG) (m_avistream.dwStart + m_avistream.dwLength))
	return ResultFromScode(AVIERR_BADPARAM);

    if (lSamples == AVISTREAMREAD_CONVENIENT) {
	// If they didn't specify a number of samples, fill their buffer....
	lSamples = (cbBuffer ? cbBuffer : 32768L) / m_lpFormatC->nBlockAlign;
    }

    // Don't let anybody try to read past the end....
    if (lSamples + lStart >
		    (LONG) (m_avistream.dwStart + m_avistream.dwLength))
	lSamples = (LONG) (m_avistream.dwStart + m_avistream.dwLength) -
			       lStart;

    if (lSamples <= 0)
	return ResultFromScode(AVIERR_BADPARAM);

    if (lpBuffer) {
	LONG	lBytes;

	if (cbBuffer < lSamples * m_lpFormatC->nBlockAlign) {
	    DPF("Returning buffer too small\n");
	    if (plBytes)
		*plBytes = lSamples * m_lpFormatC->nBlockAlign;
	
	    return ResultFromScode(AVIERR_BUFFERTOOSMALL);
	}

	if (lStart < m_dwPosOut) {
	    // !!! return to beginning!
	    m_dwPosOut = 0;
	    m_dwPosIn = AVIStreamStart(m_pavi);
	    m_dwSamplesLeft = 0;
	}
	
	while (lSamples > 0) {
	    DPF("Want %ld samples at %ld  (PosOut=%ld, SamplesLeft=%ld)\n", lSamples, lStart, m_dwPosOut, m_dwSamplesLeft);
	    if (lStart >= m_dwPosOut + m_dwSamplesLeft) {
		// Throw away current converted buffer....
		m_dwPosOut += m_dwSamplesLeft;
		
		hr = AVIStreamRead(m_pavi,
			      m_dwPosIn, m_cbIn / m_lpFormat->nBlockAlign,
			      m_lpIn, m_cbIn, &lBytes, &lRet);

		if (lBytes != m_cbIn) {
		    DPF("AVIStreamRead: Asked for %lx bytes at %lx, got %lx!\n", m_cbIn, m_dwPosIn, lBytes);

		    if (lBytes < m_cbIn) {
			// Fill buffer with silence, and hope....
			BYTE _huge *hp;
			LONG	    cb;
			BYTE	    b;

			cb = (m_cbIn - lBytes);
			hp = (BYTE _huge *) m_lpIn + lBytes;

			if ((m_lpFormat->wFormatTag == WAVE_FORMAT_PCM) &&
					(m_lpFormat->wBitsPerSample == 8))
			    b = 0x80;
			else
			    b = 0;

			while (cb-- > 0)
			    *hp++ = b;

			// If we couldn't read anything, pretend we
			// really read enough.
			if (lBytes == 0 && m_dwPosIn >= AVIStreamLength(m_pavi)) {
			    lBytes = m_cbIn;
			    hr = NOERROR;
			}
		    }

		    // !!!
		    // lSampLen = lRet; // !!!
		    // lByteLen = lSampLen * m_lpFormat->nBlockAlign;
		}

		if (FAILED(GetScode(hr))) {
		    DPF("AVIStreamReadFailed! (start=%lx, len=%lx, err=%08lx)\n", m_dwPosIn, m_cbIn / m_lpFormat->nBlockAlign, hr);
		    return hr;
		}

		m_acm.cbSrcLength = lBytes;

		err = acmStreamConvert(m_hs, &m_acm, ACM_STREAMCONVERTF_BLOCKALIGN);

		if (err != 0) {
		    DPF("acmStreamConvert returns %u\n", err);

		    return ResultFromScode(AVIERR_COMPRESSOR);
		}

		DPF("Converted %lu of %lu bytes to %lu bytes (buffer size = %lu)\n", m_acm.cbSrcLengthUsed, m_acm.cbSrcLength, m_acm.cbDstLengthUsed, m_acm.cbDstLength);

		if (m_acm.cbSrcLengthUsed == 0) {
		    err = acmStreamConvert(m_hs, &m_acm, 0);

		    if (err != 0) {
			DPF("acmStreamConvert returns %u\n", err);

			return ResultFromScode(AVIERR_COMPRESSOR);
		    }

		    DPF("Converted (non-blockalign) %lu of %lu bytes to %lu bytes (buffer size = %lu)\n", m_acm.cbSrcLengthUsed, m_acm.cbSrcLength, m_acm.cbDstLengthUsed, m_acm.cbDstLength);
		}
		
		// Lie: say that the ACM returned a full block....
		// !!! acm.cbDstLengthUsed += m_lpFormatC->nBlockAlign - 1;
		// acm.cbDstLengthUsed -= acm.cbDstLengthUsed % m_lpFormatC->nBlockAlign;

		m_dwPosIn += m_acm.cbSrcLengthUsed / m_lpFormat->nBlockAlign;
		m_dwSamplesLeft = (m_acm.cbDstLengthUsed +  m_lpFormatC->nBlockAlign - 1) / m_lpFormatC->nBlockAlign;

		if (m_dwSamplesLeft == 0) {
		    // Instead, say that we got one block back, 0 bytes long.
		    DPF("ACM returned no data at all!  Ack!\n");
		    m_dwSamplesLeft = 1;
		}
		
		m_dwBytesMissing = m_dwSamplesLeft * m_lpFormatC->nBlockAlign - m_acm.cbDstLengthUsed;
	    }

	    if (lStart >= m_dwPosOut) {
		LONG		lSamplesRead;
		LONG		lBytesRead;
		
		lSamplesRead = min(m_dwSamplesLeft - (lStart - m_dwPosOut),
				   lSamples);

		lBytesRead = lSamplesRead * m_lpFormatC->nBlockAlign;

		if (m_dwBytesMissing &&
		    (lStart - m_dwPosOut + lSamplesRead == m_dwSamplesLeft)) {
		    DPF("Not copying %ld missing bytes....\n", m_dwBytesMissing);
		    lBytesRead -= m_dwBytesMissing;
		}
		
		DPF("Copying %ld samples... (%ld bytes)\n", lSamplesRead, lBytesRead);
		hmemcpy(lpBuffer,
			(BYTE _huge *) m_lpOut +
				(lStart - m_dwPosOut) * m_lpFormatC->nBlockAlign,
			lBytesRead);

		if (plBytes)
		    *plBytes += lBytesRead;

		if (plSamples)
		    *plSamples += lSamplesRead;

		lSamples -= lSamplesRead;
		lpBuffer = (BYTE _huge *) lpBuffer +
				   lSamplesRead * m_lpFormatC->nBlockAlign;
		lStart += lSamplesRead;
	    }
	}
    } else {
	// We always assume we could read whatever they asked for....
	if (plBytes)
	    *plBytes = lSamples * m_lpFormatC->nBlockAlign;

	if (plSamples)
	    *plSamples = lSamples;
    }
    return 0;
}

STDMETHODIMP_(LONG) CACMCmpStream::FindSample(LONG lPos, LONG lFlags)
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


STDMETHODIMP CACMCmpStream::SetFormat(LONG lPos,LPVOID lpFormat,LONG cbFormat)
{
    // !!! It should really be possible to use SetFormat & Write on this
    // stream.....
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CACMCmpStream::Write(LONG lStart,
				  LONG lSamples,
				  LPVOID lpBuffer,
				  LONG cbBuffer,
				  DWORD dwFlags,
				  LONG FAR *plSampWritten,
				  LONG FAR *plBytesWritten)
{
    // !!!
    // Maybe this is the place to decompress data and write it to the original
    // stream?
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CACMCmpStream::Delete(LONG lStart,LONG lSamples)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CACMCmpStream::ReadData(DWORD fcc, LPVOID lp, LONG FAR *lpcb)
{
    return AVIStreamReadData(m_pavi, fcc, lp, lpcb);
}

STDMETHODIMP CACMCmpStream::WriteData(DWORD fcc, LPVOID lp, LONG cb)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#if 0
STDMETHODIMP CACMCmpStream::Clone(PAVISTREAM FAR * ppaviNew)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#endif


#ifdef _WIN32
STDMETHODIMP CACMCmpStream::SetInfo(AVISTREAMINFOW FAR *lpInfo, LONG cbInfo)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#else
STDMETHODIMP CACMCmpStream::Reserved1(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CACMCmpStream::Reserved2(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CACMCmpStream::Reserved3(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CACMCmpStream::Reserved4(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CACMCmpStream::Reserved5(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}


#endif
