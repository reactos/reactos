/****************************************************************************
 *
 *  FAKEFILE.C
 *
 *  routines for simulating the IAVIFile interface from a bunch of streams
 *
 *  Copyright (c) 1992  - 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  You have a royalty-free right to use, modify, reproduce and
 *  distribute the Sample Files (and/or any modified version) in
 *  any way you find useful, provided that you agree that
 *  Microsoft has no warranty obligations or liability for any
 *  Sample Application Files which are modified.
 *
 ***************************************************************************/

#include <win32.h>
#include <vfw.h>
#include "fakefile.h"
#include "debug.h"

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

/*	-	-	-	-	-	-	-	-	*/

/**************************************************************************
* @doc EXTERNAL AVIMakeFileFromStreams
*
* @api HRESULT | AVIMakeFileFromStreams | Constructs an AVIFile interface
*	pointer out of separate streams. If <f AVIFileGetStream>
*	is called with the returned file interface pointer, it will
*  return the specified
*	streams.
*
* @parm PAVIFILE FAR * | ppfile | Specifies a pointer to the location
*       used to return the new file interface pointer.
*
* @parm int | nStreams | Specifies the number of streams in
*       the array of stream interface pointers referenced by
*       <p papStreams>.
*
* @parm PAVISTREAM FAR * | papStreams | Specifies a pointer to
*       an array of stream interface pointers.
*
* @comm Use <f AVIFileRelease> to close the file. This function is
*       useful for putting streams onto the Clipboard.
*
* @rdesc Returns zero if successful; otherwise it returns an error code.
*
* @xref <f AVIFileClose> <f AVIFileGetStream>
*
*************************************************************************/
STDAPI AVIMakeFileFromStreams(PAVIFILE FAR *	ppfile,
			       int		nStreams,
			       PAVISTREAM FAR *	papStreams)
{
    CFakeFile FAR*	pAVIFile;

    pAVIFile = new FAR CFakeFile(nStreams, papStreams);
    if (!pAVIFile)
	return ResultFromScode(E_OUTOFMEMORY);

    *ppfile = (PAVIFILE) (LPVOID) pAVIFile;

    AVIFileAddRef((PAVIFILE) (LPVOID) pAVIFile);

    return AVIERR_OK;
}

/*	-	-	-	-	-	-	-	-	*/

CFakeFile::CFakeFile(int nStreams, PAVISTREAM FAR * papStreams)
{
    int		    i;
    AVISTREAMINFOW  si;
    DWORD	    dwLength;

    m_pUnknownOuter = this;
    m_refs = 0;

    _fmemset(&avihdr, 0, sizeof(avihdr));
    aps = 0;

    avihdr.dwStreams = nStreams;

    if (nStreams > 0) {
	aps = (PAVISTREAM NEAR *) LocalAlloc(LPTR, nStreams * sizeof(PAVISTREAM));


	// make sure none of the streams go away without our consent
	for (i = 0; i < nStreams; i++) {
	    aps[i] = papStreams[i];
	    AVIStreamAddRef(aps[i]);
	    // !!! should error check here, to make sure streams are valid
	
	    aps[i]->Info(&si, sizeof(si));

	    if (i == 0) {
		avihdr.dwScale  = si.dwScale;
		avihdr.dwRate   = si.dwRate;
		if ((avihdr.dwScale == 0 || avihdr.dwRate == 0) ||
			    muldiv32(1000000, si.dwScale, si.dwRate) < 10000) {
		    avihdr.dwScale = 1;
		    avihdr.dwRate = 100;
		}
	    }


	    dwLength = muldiv32(AVIStreamSampleToTime(aps[i], si.dwLength),
				avihdr.dwRate,
				avihdr.dwScale * 1000);
			
	    avihdr.dwLength = max(avihdr.dwLength, dwLength);
	    avihdr.dwWidth  = max((DWORD) si.rcFrame.right, avihdr.dwWidth);
	    avihdr.dwHeight  = max((DWORD) si.rcFrame.bottom, avihdr.dwHeight);

	    avihdr.dwWidth  = max((DWORD) si.rcFrame.right, avihdr.dwWidth);
	    avihdr.dwHeight  = max((DWORD) si.rcFrame.bottom, avihdr.dwHeight);
	}
    }
}

STDMETHODIMP CFakeFile::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
    if (iid == IID_IUnknown)
	*ppv = this;
    else if (iid == IID_IAVIFile)
	*ppv = this;
    else {
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
    }

    AddRef();
    return AVIERR_OK;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CFakeFile::AddRef()
{
    DPF("Fake   %p: Usage++=%lx\n", (DWORD_PTR) (LPVOID) this, m_refs + 1);

    return ++m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

#ifndef _WIN32
STDMETHODIMP CFakeFile::Open(LPCTSTR szFile, UINT mode)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#endif

STDMETHODIMP CFakeFile::GetStream(PAVISTREAM FAR * ppavi,
				DWORD fccType,
				LONG lParam)
{
    HRESULT		    hr;
    int			    i;

    if (fccType == 0) {
	// just return nth stream
	if (lParam < (LONG) avihdr.dwStreams) {
	    *ppavi = aps[lParam];
	    AVIStreamAddRef(*ppavi);
	    return AVIERR_OK;
	} else {
	    *ppavi = NULL;
	    return ResultFromScode(AVIERR_UNSUPPORTED);
	}
    }

    // otherwise loop through and find the one we want...
    for (i = 0; i < (int) avihdr.dwStreams; i++) {
	AVISTREAMINFO	strhdr;
	
	hr = AVIStreamInfo(aps[i], &strhdr, sizeof(strhdr));

	if (strhdr.fccType == fccType) {
	    if (lParam == 0) {
		*ppavi = aps[i];
		AVIStreamAddRef(*ppavi);
		return AVIERR_OK;
	    }

	    --lParam;
	}
    }

    // !!!
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#ifndef _WIN32
STDMETHODIMP CFakeFile::Save(LPCTSTR szFile,
				   AVICOMPRESSOPTIONS FAR *lpOptions,
				   AVISAVECALLBACK lpfnCallback)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#endif

STDMETHODIMP CFakeFile::CreateStream(PAVISTREAM FAR *ppstream,
		       AVISTREAMINFOW FAR *psi)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

#if 0
STDMETHODIMP CFakeFile::AddStream(PAVISTREAM pstream,
		       PAVISTREAM FAR *ppstreamNew)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#endif

STDMETHODIMP CFakeFile::WriteData(DWORD ckid,
		       LPVOID lpData,
		       LONG cbData)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CFakeFile::ReadData(DWORD ckid,
		      LPVOID lpData,
		      LONG FAR *lpcbData)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CFakeFile::EndRecord(void)
{
    return ResultFromScode(AVIERR_OK);
}

#ifdef _WIN32
STDMETHODIMP CFakeFile::DeleteStream(
				DWORD fccType,
				LONG lParam)
{
    // !!!
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#endif

STDMETHODIMP CFakeFile::Info(
		  AVIFILEINFOW FAR * pfi,
		  LONG lSize)
{
    hmemcpy(pfi, &avihdr, min(lSize,sizeof(avihdr)));
//    return sizeof(avihdr);
    return 0;
}



STDMETHODIMP_(ULONG) CFakeFile::Release()
{
    int		i;

    DPF("Fake   %p: Usage--=%lx\n", (DWORD_PTR) (LPVOID) this, m_refs - 1);

    if (!--m_refs) {
	LONG lRet = AVIERR_OK;

	if (aps) {
	    // Release our hold on the sub-streams
	    for (i = 0; i < (int) avihdr.dwStreams; i++) {
		AVIStreamClose(aps[i]);
	    }

	    LocalFree((HLOCAL) aps);
	}

	delete this;
	return 0;
    }
    return m_refs;
}


#ifndef _WIN32
STDMETHODIMP CFakeFile::Reserved1(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CFakeFile::Reserved2(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CFakeFile::Reserved3(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CFakeFile::Reserved4(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}

STDMETHODIMP CFakeFile::Reserved5(void)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#endif
