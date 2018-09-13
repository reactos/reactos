
/*	-	-	-	-	-	-	-	-	*/

/*
**	Copyright (C) Microsoft Corporation 1993-1995. All rights reserved.
*/

/*	-	-	-	-	-	-	-	-	*/

#include <win32.h>
#include <vfw.h>
#include "avifilei.h"
#include "debug.h"

/*	-	-	-	-	-	-	-	-	*/

UINT	uUseCount;
BOOL	fLocked;

/*	-	-	-	-	-	-	-	-	*/

STDAPI DllCanUnloadNow(
	void)
{
	DPF("DllCanUnloadNow: Usage = %u\n", uUseCount);
	return ResultFromScode((fLocked || uUseCount) ? S_FALSE : S_OK);
}

/*	-	-	-	-	-	-	-	-	*/

HRESULT CAVIFile::Create(
	IUnknown FAR*	pUnknownOuter,
	const IID FAR&	riid,
	void FAR* FAR*	ppv)
{
	IUnknown FAR*	pUnknown;
	CAVIFile FAR*	pAVIFile;
	HRESULT	hresult;

	pAVIFile = new FAR CAVIFile(pUnknownOuter, &pUnknown);
	if (!pAVIFile)
		return ResultFromScode(E_OUTOFMEMORY);
	hresult = pUnknown->QueryInterface(riid, ppv);
	if (FAILED(GetScode(hresult)))
		delete pAVIFile;
	return hresult;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIFile::CAVIFile(
	IUnknown FAR*	pUnknownOuter,
	IUnknown FAR* FAR*	ppUnknown) :
	m_Unknown(this),
	m_PersistS(this),
	m_PersistF(this),
	m_AVIFile(this)
#ifdef CUSTOMMARSHAL
	, m_Marshal(this)
#endif
{
	hshfile = 0;
	achFile[0] = TEXT('\0');
	fInRecord = FALSE;
	lWriteLoc = 0;
	fDirty = 0;
	extra.lp = 0;
	extra.cb = 0;
	_fmemset(&avihdr, 0, sizeof(avihdr));
	avihdr.dwStreams = 0;
	lHeaderSize = 0;
        px = NULL;
	pb = NULL;

	if (pUnknownOuter) {
	    DPF("(F) Being aggregated!\n");
	    m_pUnknownOuter = pUnknownOuter;
	} else
	    m_pUnknownOuter = &m_Unknown;
	*ppUnknown = &m_Unknown;

#ifdef WIN32
	InitializeCriticalSection(&m_critsec);
#endif
}

/*	-	-	-	-	-	-	-	-	*/

CAVIFile::CUnknownImpl::CUnknownImpl(
	CAVIFile FAR*	pAVIFile)
{
	m_pAVIFile = pAVIFile;
	m_refs = 0;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFile::CUnknownImpl::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	if (iid == IID_IUnknown
#ifdef CUSTOMMARSHAL
		 || iid == CLSID_AVISimpleUnMarshal
#endif
	)
		*ppv = &m_pAVIFile->m_Unknown;
	else if (iid == IID_IAVIFile)
		*ppv = &m_pAVIFile->m_AVIFile;
#if 0
	else if (iid == IID_IPersistStorage)
		*ppv = &m_pAVIFile->m_PersistS;
#endif
	else if (iid == IID_IPersistFile)
		*ppv = &m_pAVIFile->m_PersistF;
#ifdef CUSTOMMARSHAL
	else if (iid == IID_IMarshal) {
		DPF("(F) QueryInterface (IMarshal)\n");
		*ppv = &m_pAVIFile->m_Marshal;
	}
#endif
	else {
		*ppv = NULL;
		return ResultFromScode(E_NOINTERFACE);
	}
	AddRef();
	return AVIERR_OK;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFile::CUnknownImpl::AddRef()
{
	DPF2("File   %p: Usage++=%lx\n", (DWORD_PTR) (LPVOID) this, m_refs + 1);

	if (m_pAVIFile->hshfile)
	    shfileAddRef(m_pAVIFile->hshfile);
	uUseCount++;
	return ++m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIFile::CAVIFileImpl::CAVIFileImpl(
	CAVIFile FAR*	pAVIFile)
{
	m_pAVIFile = pAVIFile;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIFile::CAVIFileImpl::~CAVIFileImpl()
{
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIFile::CAVIFileImpl::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	return m_pAVIFile->m_pUnknownOuter->QueryInterface(iid, ppv);
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFile::CAVIFileImpl::AddRef()
{
	return m_pAVIFile->m_pUnknownOuter->AddRef();
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIFile::CAVIFileImpl::Release()
{
	return m_pAVIFile->m_pUnknownOuter->Release();
}

/*	-	-	-	-	-	-	-	-	*/


/*	-	-	-	-	-	-	-	-	*/

CAVIStream::CAVIStream(
	IUnknown FAR*	pUnknownOuter,
	IUnknown FAR* FAR*	ppUnknown) :
	m_Unknown(this),
	m_AVIStream(this),
#ifdef CUSTOMMARSHAL
	m_Marshal(this),
#endif
	m_Streaming(this)
{
	paviBase = NULL;
	hshfile = 0;
	lpFormat = NULL;
	cbFormat = 0;
	lpData = NULL;
	cbData = 0;
	extra.lp = NULL;
        extra.cb = 0;
        psx = NULL;
	lPal = 0;
	pb = NULL;
	fInit = FALSE;

	if (pUnknownOuter) {
	    DPF("(S) Being aggregated!\n");
	    m_pUnknownOuter = pUnknownOuter;
	}
	else
		m_pUnknownOuter = &m_Unknown;
	*ppUnknown = &m_Unknown;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIStream::~CAVIStream()
{
    if (lpFormat)
	GlobalFreePtr(lpFormat);

    if (lpData)
	GlobalFreePtr(lpData);

    if (extra.lp) {
	DPF2("Freeing %ld bytes of extra stream data.\n", extra.cb);
	GlobalFreePtr(extra.lp);
    }

    if (paviBase)
	AVIStreamClose(paviBase);
}

/*	-	-	-	-	-	-	-	-	*/

CAVIStream::CUnknownImpl::CUnknownImpl(
	CAVIStream FAR*	pAVIStream)
{
	m_pAVIStream = pAVIStream;
	m_refs = 0;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIStream::CUnknownImpl::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	if (iid == IID_IUnknown
#ifdef CUSTOMMARSHAL
		|| iid == CLSID_AVISimpleUnMarshal
#endif
	)
		*ppv = &m_pAVIStream->m_Unknown;
	else if (iid == IID_IAVIStream)
		*ppv = &m_pAVIStream->m_AVIStream;
	else if (iid == IID_IAVIStreaming)
		*ppv = &m_pAVIStream->m_Streaming;
#ifdef CUSTOMMARSHAL
	else if (iid == IID_IMarshal) {
		DPF("(S) QueryInterface (IMarshal)\n");
		*ppv = &m_pAVIStream->m_Marshal;
	}
#endif
	else {
		*ppv = NULL;
		return ResultFromScode(E_NOINTERFACE);
	}
	AddRef();
	return NULL;
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIStream::CUnknownImpl::AddRef()
{
	uUseCount++;
	if (m_pAVIStream->hshfile)
	    shfileAddRef(m_pAVIStream->hshfile);
	if (m_refs < 20) {
	    DPF2("Stream %p: Usage++=%lx\n", (DWORD_PTR) (LPVOID) this, m_refs + 1);
	}
	
	return ++m_refs;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIStream::CAVIStreamImpl::CAVIStreamImpl(
	CAVIStream FAR*	pAVIStream)
{
	m_pAVIStream = pAVIStream;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIStream::CAVIStreamImpl::~CAVIStreamImpl()
{
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIStream::CAVIStreamImpl::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	return m_pAVIStream->m_pUnknownOuter->QueryInterface(iid, ppv);
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIStream::CAVIStreamImpl::AddRef()
{
	return m_pAVIStream->m_pUnknownOuter->AddRef();
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIStream::CAVIStreamImpl::Release()
{
	return m_pAVIStream->m_pUnknownOuter->Release();
}

/*	-	-	-	-	-	-	-	-	*/

CAVIStream::CStreamingImpl::CStreamingImpl(
	CAVIStream   FAR*	pAVIStream)
{
	m_pAVIStream = pAVIStream;
}

/*	-	-	-	-	-	-	-	-	*/

CAVIStream::CStreamingImpl::~CStreamingImpl()
{
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP CAVIStream::CStreamingImpl::QueryInterface(
	const IID FAR&	iid,
	void FAR* FAR*	ppv)
{
	return m_pAVIStream->m_pUnknownOuter->QueryInterface(iid, ppv);
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIStream::CStreamingImpl::AddRef()
{
	return m_pAVIStream->m_pUnknownOuter->AddRef();
}

/*	-	-	-	-	-	-	-	-	*/

STDMETHODIMP_(ULONG) CAVIStream::CStreamingImpl::Release()
{
	return m_pAVIStream->m_pUnknownOuter->Release();
}

/*	-	-	-	-	-	-	-	-	*/
