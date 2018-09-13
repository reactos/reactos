// avifps.cpp - proxy and stub code for IAVIFile & IAVIStream
//
//
//  Copyright (c) 1993 Microsoft Corporation.  All Rights Reserved.
//
// History:
//  Created by DavidMay		6/19/93
//
//
// What's in this file:
//
//  Code to enable "standard marshalling" of the IAVIFile and IAVIStream
//  interfaces, consisting of the following classes:
//
//  CPSFactory, derived from IPSFactory:
//	Proxy/stub factory, called from DllGetClassObject to create
//	the other classes.
//
//  CPrxAVIStream, derived from IAVIStream:
//	This class serves as a stand-in for the interface in the app that's
//	calling it.  Uses RPC to communicate with....
//
//  CStubAVIStream, derived from IRpcStubBuffer:
//	This class in the called app receives requests from the proxy
//	and forwards them to the actual implementation of the IAVIStream.
//
//  CPrxAVIFile & CStubAVIFile, just like the stream versions.
//
//
//  Also included:
//  The function TaskHasExistingProxies can be used by an application
//  before exiting to check whether any of its objects are being used
//  by other applications.  This is done by keeping track of what active
//  stubs exist within a given task context.
//

#include <win32.h>
#pragma warning(disable:4355)
#include "avifile.h"
#include "avifps.h"
#include "debug.h"

#ifndef WIN32
typedef POINT POINTS;
#endif

// when thunking between 16-bit ansi and 32-bit unicode apps, the
// AVISTREAMINFO and AVIFILEINFO structures are different. What is transmitted
// is a common format that includes space for both unicode and ansi strings.
// ansi apps will not send or use the unicode strings. unicode apps will send
// both.
typedef struct _PS_STREAMINFO {
    DWORD		fccType;
    DWORD               fccHandler;
    DWORD               dwFlags;        /* Contains AVITF_* flags */
    DWORD		dwCaps;
    WORD		wPriority;
    WORD		wLanguage;
    DWORD               dwScale;
    DWORD               dwRate; /* dwRate / dwScale == samples/second */
    DWORD               dwStart;
    DWORD               dwLength; /* In units above... */
    DWORD		dwInitialFrames;
    DWORD               dwSuggestedBufferSize;
    DWORD               dwQuality;
    DWORD               dwSampleSize;
    POINTS		ptFrameTopLeft;
    POINTS		ptFrameBottomRight;
    DWORD		dwEditCount;
    DWORD		dwFormatChangeCount;
    char		szName[64];
    DWORD		bHasUnicode;
    WCHAR		szUnicodeName[64];
} PS_STREAMINFO, FAR * LPPS_STREAMINFO;

typedef struct _PS_FILEINFO {
    DWORD		dwMaxBytesPerSec;	// max. transfer rate
    DWORD		dwFlags;		// the ever-present flags
    DWORD		dwCaps;
    DWORD		dwStreams;
    DWORD		dwSuggestedBufferSize;

    DWORD		dwWidth;
    DWORD		dwHeight;

    DWORD		dwScale;	
    DWORD		dwRate;	/* dwRate / dwScale == samples/second */
    DWORD		dwLength;

    DWORD		dwEditCount;

    char		szFileType[64];		// descriptive string for file type?
    DWORD		bHasUnicode;
    WCHAR		szUnicodeType[64];	
} PS_FILEINFO, FAR * LPPS_FILEINFO;



#ifndef WIN32
//
// These constants are defined in the 32-bit UUID.LIB, but not
// in any 16-bit LIB.  They are stolen here from the .IDL files
// in the TYPES project.
//
extern "C" {
const IID IID_IRpcStubBuffer = {0xD5F56AFC,0x593b,0x101A,{0xB5,0x69,0x08,0x00,0x2B,0x2D,0xBF,0x7A}};
const IID IID_IRpcProxyBuffer = {0xD5F56A34,0x593b,0x101A,{0xB5,0x69,0x08,0x00,0x2B,0x2D,0xBF,0x7A}};
const IID IID_IPSFactoryBuffer = {0xD5F569D0,0x593b,0x101A,{0xB5,0x69,0x08,0x00,0x2B,0x2D,0xBF,0x7A}};
}
#endif

// functions for proxy/stub usage tracking; see end of this file.
void UnregisterStubUsage(void);
void RegisterStubUsage(void);
extern "C" BOOL FAR TaskHasExistingProxies(void);



#if 0	// this function is actually in classobj.cpp,
// but if this were a separate proxy/stub DLL, it would look like this.
STDAPI DllGetClassObject(const CLSID FAR&	rclsid,
			 const IID FAR&	riid,
			 void FAR* FAR*	ppv)
{
    HRESULT	hresult;

    DPF("DllGetClassObject\n");

    if (rclsid == CLSID_AVIStreamPS) {
	return (*ppv = (LPVOID)new CPSFactory()) != NULL
		? NOERROR : ResultFromScode(E_OUTOFMEMORY);
    } else {
	return ResultFromScode(E_UNEXPECTED);
    }
}
#endif

/*
 *	IMPLEMENTATION of CPSFactory
 *
 *
 *  Note: This Factory supports proxies and stubs for two separate
 *  interfaces, IID_IAVIFile and IID_IAVIStream.
 */

CPSFactory::CPSFactory(void)
{
    m_refs = 1;
}



// controlling unknown for PSFactory
STDMETHODIMP CPSFactory::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (iid == IID_IUnknown || iid == IID_IPSFactoryBuffer)
    {
	*ppv = this;
	++m_refs;
	return NOERROR;
    }
    else
    {
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) CPSFactory::AddRef(void)
{
    return ++m_refs;
}

STDMETHODIMP_(ULONG) CPSFactory::Release(void)
{
    if (--m_refs == 0)
    {
	delete this;
	return 0;
    }

    return m_refs;
}


// create proxy for given interface
STDMETHODIMP CPSFactory::CreateProxy(IUnknown FAR* pUnkOuter, REFIID iid,
	IRpcProxyBuffer FAR* FAR* ppProxy, void FAR* FAR* ppv)
{
    IRpcProxyBuffer FAR* pProxy;
    HRESULT     hresult;

    *ppProxy = NULL;
    *ppv = NULL;

    if (pUnkOuter == NULL)
	return ResultFromScode(E_INVALIDARG);

    if (iid == IID_IAVIStream) {
	if ((pProxy = CPrxAVIStream::Create(pUnkOuter)) == NULL)
	    return ResultFromScode(E_OUTOFMEMORY);
    } else if (iid == IID_IAVIFile) {
	if ((pProxy = CPrxAVIFile::Create(pUnkOuter)) == NULL)
	    return ResultFromScode(E_OUTOFMEMORY);
    } else
	return ResultFromScode(E_NOINTERFACE);

    hresult = pProxy->QueryInterface(iid, ppv);

    if (hresult == NOERROR)
	*ppProxy = pProxy;			// transfer ref to caller
    else
	pProxy->Release();			// free proxy just created

    return hresult;
}



// create stub for given interface
STDMETHODIMP CPSFactory::CreateStub(REFIID iid, IUnknown FAR* pUnkServer, IRpcStubBuffer FAR* FAR* ppStub)
{
    if (iid == IID_IAVIStream) {
	return CStubAVIStream::Create(pUnkServer, ppStub);
    } else if (iid == IID_IAVIFile) {
	return CStubAVIFile::Create(pUnkServer, ppStub);
    } else
	return ResultFromScode(E_NOINTERFACE);
}





/*
 *  IMPLEMENTATION of CPrxAVIStream
 *
 */


// create unconnected CPrxAVIStream; return controlling IProxy/IUnknokwn FAR*
IRpcProxyBuffer FAR* CPrxAVIStream::Create(IUnknown FAR* pUnkOuter)
{
    CPrxAVIStream FAR* pPrxAVIStream;

    if ((pPrxAVIStream = new CPrxAVIStream(pUnkOuter)) == NULL)
	return NULL;

    return &pPrxAVIStream->m_Proxy;
}


CPrxAVIStream::CPrxAVIStream(IUnknown FAR* pUnkOuter) : m_Proxy(this)
{
    // NOTE: could assert here since we should always be aggregated
    if (pUnkOuter == NULL)
	pUnkOuter = &m_Proxy;

    m_refs = 1;
    m_pUnkOuter = pUnkOuter;
    m_pRpcChannelBuffer = NULL;
    m_sh.fccType = 0;

    DPF("PrxStream %lx: Usage++=%lx\n", (DWORD) (LPVOID) this, 1L);
}


CPrxAVIStream::~CPrxAVIStream(void)
{
    m_Proxy.Disconnect();
}
		

// Methods for controlling unknown
STDMETHODIMP CPrxAVIStream::CProxyImpl::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (iid == IID_IUnknown || iid == IID_IRpcProxyBuffer)
        *ppv = (void FAR *)this;
    else if (iid ==  IID_IAVIStream)
        *ppv = (void FAR *)m_pPrxAVIStream;
    else {
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
    }

    // simplest way to be correct: always addref the pointer we will return;
    // easy since all interfaces here are derived from IUnknown.
    ((IUnknown FAR*) *ppv)->AddRef();

    return NOERROR;
}

STDMETHODIMP_(ULONG) CPrxAVIStream::CProxyImpl::AddRef(void)
{
    return ++m_pPrxAVIStream->m_refs;
}

STDMETHODIMP_(ULONG) CPrxAVIStream::CProxyImpl::Release(void)
{
    if (--m_pPrxAVIStream->m_refs == 0)
    {
	delete m_pPrxAVIStream;
	return 0;
    }
    return m_pPrxAVIStream->m_refs;
}


// connect proxy to channel given
STDMETHODIMP CPrxAVIStream::CProxyImpl::Connect(IRpcChannelBuffer FAR* pRpcChannelBuffer)
{
    if (m_pPrxAVIStream->m_pRpcChannelBuffer != NULL)
	return ResultFromScode(E_UNEXPECTED);

    if (pRpcChannelBuffer == NULL)
	return ResultFromScode(E_INVALIDARG);

    (m_pPrxAVIStream->m_pRpcChannelBuffer = pRpcChannelBuffer)->AddRef();
    return NOERROR;
}


// disconnect proxy from any current channel
STDMETHODIMP_(void) CPrxAVIStream::CProxyImpl::Disconnect(void)
{
    if (m_pPrxAVIStream->m_pRpcChannelBuffer)
    {
	m_pPrxAVIStream->m_pRpcChannelBuffer->Release();
	m_pPrxAVIStream->m_pRpcChannelBuffer = NULL;
    }
}



// IUnknown methods for external interface(s); always delegate
STDMETHODIMP CPrxAVIStream::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    return m_pUnkOuter->QueryInterface(iid, ppv);
}

STDMETHODIMP_(ULONG) CPrxAVIStream::AddRef(void)
{
    DPF("PrxStream %lx: Usage++=%lx\n", (DWORD) (LPVOID) this, m_refs + 1);
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CPrxAVIStream::Release(void)
{
    DPF("PrxStream %lx: Usage--=%lx\n", (DWORD) (LPVOID) this, m_refs - 1);
    return m_pUnkOuter->Release();
}



// IAVIStream interface methods

STDMETHODIMP CPrxAVIStream::Create(LONG lParam1, LONG lParam2)
{
    return ResultFromScode(E_NOTIMPL);
}

#ifdef WIN32
STDMETHODIMP CPrxAVIStream::Info(AVISTREAMINFOW FAR * psi, LONG lSize)
#else
STDMETHODIMP CPrxAVIStream::Info(AVISTREAMINFO FAR * psi, LONG lSize)
#endif
{
    HRESULT hrMarshal;
    HRESULT hrMethod = NOERROR;
    IRpcChannelBuffer FAR* pChannel = m_pRpcChannelBuffer;
    RPCOLEMESSAGE Message;

    _fmemset(&Message, 0, sizeof(Message));

    if (pChannel == NULL)
	return ResultFromScode(RPC_E_CONNECTION_TERMINATED);

    //
    // NOTE: We take advantage here of the fact that we assume the
    // stream is read-only and not being changed on the other end!
    //
    // To avoid some intertask calls, we assume that the result
    // of the Info() method will not change.
    //
    if (m_sh.fccType == 0) {

	// we might be talking to 16 or 32-bit stub, so we need to
	// exchange a common (superset) format and pick out the bits we need.

	// format in: lSize
	// format out: PS_STREAMINFO, hrMethod
	Message.cbBuffer = sizeof(lSize);
	Message.iMethod = IAVISTREAM_Info;
	
	if ((hrMarshal = pChannel->GetBuffer(&Message, IID_IAVIStream)) != NOERROR)
	    goto ErrExit;

	((DWORD FAR *)Message.Buffer)[0] = sizeof(PS_STREAMINFO);
	
	if ((hrMarshal = pChannel->SendReceive(&Message,(ULONG*) &hrMethod)) != NOERROR) {
	    ;
ErrExit:
	    return PropagateResult(hrMarshal, RPC_E_CLIENT_CANTMARSHAL_DATA);
	}

	hrMethod = ((HRESULT FAR *)Message.Buffer)[0];
	PS_STREAMINFO FAR * psinfo = (PS_STREAMINFO FAR *)
			((LPBYTE)Message.Buffer + sizeof(HRESULT));

	// get the bits we want
	m_sh.fccType 	= psinfo->fccType;
	m_sh.fccHandler = psinfo->fccHandler;
	m_sh.dwFlags 	= psinfo->dwFlags;        /* Contains AVITF_* flags */
	m_sh.dwCaps 	= psinfo->dwCaps;
	m_sh.wPriority 	= psinfo->wPriority;
	m_sh.wLanguage 	= psinfo->wLanguage;
	m_sh.dwScale 	= psinfo->dwScale;
	m_sh.dwRate 	= psinfo->dwRate; /* dwRate / dwScale == samples/second */
	m_sh.dwStart 	= psinfo->dwStart;
	m_sh.dwLength 	= psinfo->dwLength; /* In units above... */
	m_sh.dwInitialFrames = psinfo->dwInitialFrames;
	m_sh.dwSuggestedBufferSize = psinfo->dwSuggestedBufferSize;
	m_sh.dwQuality 	= psinfo->dwQuality;
	m_sh.dwSampleSize = psinfo->dwSampleSize;

	// RECTs are different sizes, so use POINTS (WORD point)
	m_sh.rcFrame.top = psinfo->ptFrameTopLeft.y;	
	m_sh.rcFrame.left = psinfo->ptFrameTopLeft.x;
	m_sh.rcFrame.bottom = psinfo->ptFrameBottomRight.y;	
	m_sh.rcFrame.right = psinfo->ptFrameBottomRight.x;

    	m_sh.dwEditCount = psinfo->dwEditCount;
    	m_sh.dwFormatChangeCount = psinfo->dwFormatChangeCount;

#ifdef WIN32	
	// use unicode if we've been sent it
	if (psinfo->bHasUnicode) {
	    _fmemcpy(m_sh.szName, psinfo->szUnicodeName, sizeof(m_sh.szName));
	} else {
	    // need ansi->unicode thunk
	    MultiByteToWideChar(
	    	CP_ACP, 0,
		psinfo->szName,
	     	-1,
		m_sh.szName,
		NUMELMS(m_sh.szName));
	}
#else
    	// we only use the ansi which is always sent
	_fmemcpy(m_sh.szName, psinfo->szName, sizeof(m_sh.szName));
#endif

	pChannel->FreeBuffer(&Message);
    }

    _fmemcpy(psi, &m_sh, min((int) lSize, sizeof(m_sh)));

    return hrMethod;
}


STDMETHODIMP_(LONG) CPrxAVIStream::FindSample(LONG lPos, LONG lFlags)
{
    HRESULT hrMarshal;
    HRESULT hrMethod;
    IRpcChannelBuffer FAR* pChannel = m_pRpcChannelBuffer;
    LONG    lResult;

    if (pChannel == NULL)
	return -1; // !!! ResultFromScode(RPC_E_CONNECTION_TERMINATED);

    RPCOLEMESSAGE Message;

    _fmemset(&Message, 0, sizeof(Message));

    // format in: lPos, lFlags
    // format out: hrMethod, lResult
    Message.cbBuffer = sizeof(lPos) + sizeof(lFlags);
    Message.iMethod = IAVISTREAM_FindSample;

    if ((hrMarshal = pChannel->GetBuffer(&Message, IID_IAVIStream)) != NOERROR)
	goto ErrExit;

    ((DWORD FAR *)Message.Buffer)[0] = lPos;
    ((DWORD FAR *)Message.Buffer)[1] = lFlags;

    if ((hrMarshal = pChannel->SendReceive(&Message, (ULONG*) &hrMethod)) != NOERROR) {
	goto ErrExit;
    }

    hrMethod = ((HRESULT FAR *)Message.Buffer)[0];
    lResult = ((LONG FAR *)Message.Buffer)[1];

    pChannel->FreeBuffer(&Message);

    DPF("Proxy: FindSample (%ld) returns (%ld)\n", lPos, lResult);
    return lResult; // !!! hrMethod;

ErrExit:
    DPF("Proxy: FindSample returning error...\n");
    return -1; // !!! PropagateResult(hrMarshal, RPC_E_CLIENT_CANTMARSHAL_DATA);
}


STDMETHODIMP CPrxAVIStream::ReadFormat(LONG lPos, LPVOID lpFormat, LONG FAR *lpcbFormat)
{
    HRESULT hrMarshal;
    HRESULT hrMethod;
    IRpcChannelBuffer FAR* pChannel = m_pRpcChannelBuffer;

    if (pChannel == NULL)
	return ResultFromScode(RPC_E_CONNECTION_TERMINATED);

    // check that size is 0 if pointer is null
    if (lpFormat == NULL) {
	*lpcbFormat = 0;
    }

    // format in: dw, *lpcbFormat
    // format out: hrMethod, *lpcbFormat, format data

    RPCOLEMESSAGE Message;

    _fmemset(&Message, 0, sizeof(Message));

    Message.cbBuffer = sizeof(lPos) + sizeof(*lpcbFormat);
    Message.iMethod = IAVISTREAM_ReadFormat;

    if ((hrMarshal = pChannel->GetBuffer(&Message, IID_IAVIStream)) != NOERROR)
	goto ErrExit;

    ((DWORD FAR *)Message.Buffer)[0] = lPos;
    ((DWORD FAR *)Message.Buffer)[1] = lpFormat ? *lpcbFormat : 0;

    if ((hrMarshal = pChannel->SendReceive(&Message, (ULONG*) &hrMethod)) != NOERROR) {
	goto ErrExit;
    }

    hrMethod = ((HRESULT FAR *)Message.Buffer)[0];

    if (lpFormat && *lpcbFormat)
	hmemcpy(lpFormat, (LPBYTE) Message.Buffer + 2*sizeof(DWORD),
		min(*lpcbFormat, (long) ((DWORD FAR *) Message.Buffer)[1]));

    // write the size last, so we don't copy more than user's buffer
    *lpcbFormat = ((DWORD FAR *)Message.Buffer)[1];

    pChannel->FreeBuffer(&Message);

    return hrMethod;

ErrExit:
    return PropagateResult(hrMarshal, RPC_E_CLIENT_CANTMARSHAL_DATA);
}


STDMETHODIMP CPrxAVIStream::Read(
                 LONG       lStart,
                 LONG       lSamples,
                 LPVOID     lpBuffer,
                 LONG       cbBuffer,
                 LONG FAR * plBytes,
                 LONG FAR * plSamples)
{
    HRESULT hrMarshal;
    HRESULT hrMethod;
    IRpcChannelBuffer FAR* pChannel = m_pRpcChannelBuffer;
    LONG    lTemp;

    if (pChannel == NULL)
	return ResultFromScode(RPC_E_CONNECTION_TERMINATED);

    if (lpBuffer == NULL)
	cbBuffer = 0;

    // format on input: lPos, lLength, cb
    // format on output: hresult, samples, cb, frame
    RPCOLEMESSAGE Message;

    _fmemset(&Message, 0, sizeof(Message));

    Message.cbBuffer = sizeof(lStart) + sizeof(lSamples) + sizeof(cbBuffer);
    Message.iMethod = IAVISTREAM_Read;

    if ((hrMarshal = pChannel->GetBuffer(&Message, IID_IAVIStream)) != NOERROR)
	goto ErrExit;

    ((DWORD FAR *)Message.Buffer)[0] = lStart;
    ((DWORD FAR *)Message.Buffer)[1] = lSamples;
    ((DWORD FAR *)Message.Buffer)[2] = lpBuffer ? cbBuffer : 0;

    if ((hrMarshal = pChannel->SendReceive(&Message, (ULONG*) &hrMethod)) != NOERROR) {
	goto ErrExit;
    }

    hrMethod = ((HRESULT FAR *)Message.Buffer)[0];

    lTemp = ((DWORD FAR *)Message.Buffer)[1];
    if (plBytes)
	*plBytes = lTemp;

    if (plSamples)
	*plSamples = ((DWORD FAR *)Message.Buffer)[2];

    if (lpBuffer && lTemp)
	hmemcpy(lpBuffer, (LPBYTE) Message.Buffer + 3*sizeof(DWORD), lTemp);

    pChannel->FreeBuffer(&Message);

    return hrMethod;

ErrExit:
    return PropagateResult(hrMarshal, RPC_E_CLIENT_CANTMARSHAL_DATA);
}

//
// All of the writing-related messages are not remoted....
//
STDMETHODIMP CPrxAVIStream::SetFormat(LONG lPos,LPVOID lpFormat,LONG cbFormat)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::Write(LONG lStart,
				  LONG lSamples,
				  LPVOID lpData,
				  LONG cbData,
				  DWORD dwFlags,
				  LONG FAR *plSampWritten,
				  LONG FAR *plBytesWritten)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::Delete(LONG lStart,LONG lSamples)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::ReadData(DWORD ckid, LPVOID lp, LONG FAR *lpcb)
{
    // !!! This should really be remoted!
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::WriteData(DWORD ckid, LPVOID lp, LONG cb)
{
    return ResultFromScode(E_NOTIMPL);
}


#ifdef WIN32
STDMETHODIMP CPrxAVIStream::SetInfo(AVISTREAMINFOW FAR *lpInfo, LONG cbInfo)
{
    return ResultFromScode(E_NOTIMPL);
}

#else
STDMETHODIMP CPrxAVIStream::Reserved1(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::Reserved2(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::Reserved3(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::Reserved4(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIStream::Reserved5(void)
{
    return ResultFromScode(E_NOTIMPL);
}

#endif


/*
 *  IMPLEMENTATION of CStubAVIStream
 *	
 */

// create connected interface stub
HRESULT CStubAVIStream::Create(IUnknown FAR* pUnkObject, IRpcStubBuffer FAR* FAR* ppStub)
{
    CStubAVIStream FAR* pStubAVIStream;

    *ppStub = NULL;

    if ((pStubAVIStream = new CStubAVIStream()) == NULL)
	return ResultFromScode(E_OUTOFMEMORY);

    HRESULT hresult;
    if ((hresult = pStubAVIStream->Connect(pUnkObject)) != NOERROR)
    {
	pStubAVIStream->Release();
	return hresult;
    }

    *ppStub = pStubAVIStream;
    return NOERROR;
}


CStubAVIStream::CStubAVIStream(void)
{
    m_refs	 = 1; /// !!! ??? 0
    DPF("StubStream %lx: Usage++=%lx  (C)\n", (DWORD) (LPVOID) this, 1L);
    m_pAVIStream = NULL;
    RegisterStubUsage();
}


CStubAVIStream::~CStubAVIStream(void)
{
    UnregisterStubUsage();
    Disconnect();
}


// controling unknown methods for interface stub
STDMETHODIMP CStubAVIStream::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{

    if (iid == IID_IUnknown || iid == IID_IRpcStubBuffer)
    {
	*ppv = this;
	DPF("StubStream %lx: Usage++=%lx  (QI)\n", (DWORD) (LPVOID) this, m_refs + 1);
	++m_refs;
	return NOERROR;
    }
    else
    {
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) CStubAVIStream::AddRef(void)
{
    DPF("StubStream %lx: Usage++=%lx\n", (DWORD) (LPVOID) this, m_refs + 1);
    return ++m_refs;
}

STDMETHODIMP_(ULONG) CStubAVIStream::Release(void)
{
    DPF("StubStream %lx: Usage--=%lx\n", (DWORD) (LPVOID) this, m_refs - 1);
    if (--m_refs == 0)
    {
	if (m_pAVIStream) {
	    DPF("Releasing stream in funny place!\n");
	    m_pAVIStream->Release();
	    m_pAVIStream = NULL;
	}
	delete this;
	return 0;
    }

    return m_refs;
}


// connect interface stub to server object
STDMETHODIMP CStubAVIStream::Connect(IUnknown FAR* pUnkObj)
{
    HRESULT	hr;

    if (m_pAVIStream)
	// call Disconnect first
	return ResultFromScode(E_UNEXPECTED);

    if (pUnkObj == NULL)
	return ResultFromScode(E_INVALIDARG);
		
    // NOTE: QI ensures out param is zero if error
    hr = pUnkObj->QueryInterface(IID_IAVIStream, (LPVOID FAR*)&m_pAVIStream);

    DPF("CStubAVIStream::Connect: Result = %lx, stream = %lx\n", (DWORD) (LPVOID) hr, (DWORD) m_pAVIStream);
    return hr;
}


// disconnect interface stub from server objec
STDMETHODIMP_(void) CStubAVIStream::Disconnect(void)
{
    DPF("CStubAVIStream::Disconnect\n");
    if (m_pAVIStream) {
	DPF("Disconnect: Releasing stream\n");
	m_pAVIStream->Release();
	m_pAVIStream = NULL;
    }
}


// remove method call
STDMETHODIMP CStubAVIStream::Invoke
	(RPCOLEMESSAGE FAR *pMessage, IRpcChannelBuffer FAR *pChannel)
{
    HRESULT     hresult;
    HRESULT	hrMethod;

    DPF("!AVISTREAM: Invoke: ");

    if (!m_pAVIStream) {
	DPF("!No stream!\n");
	return ResultFromScode(RPC_E_UNEXPECTED);
    }

#if 0
    if (iid != IID_IAVIStream) {
	DPF("!Wrong interface\n");

	return ResultFromScode(RPC_E_UNEXPECTED);
    }
#endif

    switch (pMessage->iMethod)
    {
	case IAVISTREAM_Info:
	    // format on input: lSize
	    // format on output: hresult, PS_STREAMINFO
	{
	    DWORD lSize;
#ifdef WIN32
	    AVISTREAMINFOW si;
#else
	    AVISTREAMINFO si;
#endif

	    DPF("!Info\n");

	    // need to send a common ansi/unicode version with both strings
	    PS_STREAMINFO psinfo;
	    hrMethod = m_pAVIStream->Info(&si, sizeof(si));

	    // copy all members
	    psinfo.fccType	= si.fccType;
	    psinfo.fccHandler	= si.fccHandler;
	    psinfo.dwFlags	= si.dwFlags;        /* Contains AVITF_* flags */
	    psinfo.dwCaps	= si.dwCaps;
	    psinfo.wPriority	= si.wPriority;
	    psinfo.wLanguage	= si.wLanguage;
	    psinfo.dwScale	= si.dwScale;
	    psinfo.dwRate	= si.dwRate; /* dwRate / dwScale == samples/second */
	    psinfo.dwStart	= si.dwStart;
	    psinfo.dwLength	= si.dwLength; /* In units above... */
	    psinfo.dwInitialFrames	= si.dwInitialFrames;
	    psinfo.dwSuggestedBufferSize	= si.dwSuggestedBufferSize;
	    psinfo.dwQuality	= si.dwQuality;
	    psinfo.dwSampleSize	= si.dwSampleSize;
	    psinfo.dwEditCount	= si.dwEditCount;
	    psinfo.dwFormatChangeCount	= si.dwFormatChangeCount;

	    // RECT is different size, so use POINTS
	    psinfo.ptFrameTopLeft.x = (short) si.rcFrame.left;
	    psinfo.ptFrameTopLeft.y = (short) si.rcFrame.top;
	    psinfo.ptFrameBottomRight.x = (short) si.rcFrame.right;
	    psinfo.ptFrameBottomRight.y = (short) si.rcFrame.bottom;

#ifdef WIN32	
	    // send both UNICODE and ansi
	    hmemcpy(psinfo.szUnicodeName, si.szName, sizeof(psinfo.szUnicodeName));
	    psinfo.bHasUnicode = TRUE;
	    WideCharToMultiByte(CP_ACP, 0,
	    	si.szName,
		-1,
		psinfo.szName,
		NUMELMS(psinfo.szName),
		NULL, NULL);
#else
    	    // just send ansi version for 16-bit stub
	    psinfo.bHasUnicode = FALSE;
	    hmemcpy(psinfo.szName, si.szName, sizeof(si.szName));
#endif

	    lSize = ((DWORD FAR *)pMessage->Buffer)[0];


	    pMessage->cbBuffer = lSize + sizeof(hrMethod);
	
	    if ((hresult = pChannel->GetBuffer(pMessage, IID_IAVIStream)) != NOERROR)
		return PropagateResult(hresult, RPC_E_SERVER_CANTUNMARSHAL_DATA);

	    ((HRESULT FAR *)pMessage->Buffer)[0] = hrMethod;

	    hmemcpy((LPBYTE) pMessage->Buffer + sizeof(hrMethod),
		    &psinfo,
		    lSize);
		
	    return NOERROR;
	}

	case IAVISTREAM_FindSample:
	    // format on input: lPos, lFlags
	    // format on output: hResult, lResult
	{
	    LONG lPos, lFlags, lResult;

	    lPos = ((DWORD FAR *)pMessage->Buffer)[0];
	    lFlags = ((DWORD FAR *)pMessage->Buffer)[1];

	    DPF("!FindSample (%ld)\n", lPos);
	
	    lResult = m_pAVIStream->FindSample(lPos, lFlags);

	    hrMethod = 0; // !!!

	    pMessage->cbBuffer = sizeof(lResult) + sizeof(hrMethod);
	
	    if ((hresult = pChannel->GetBuffer(pMessage, IID_IAVIStream)) != NOERROR)
		return PropagateResult(hresult, RPC_E_SERVER_CANTUNMARSHAL_DATA);

	    ((HRESULT FAR *)pMessage->Buffer)[0] = hrMethod;
	    ((DWORD FAR *)pMessage->Buffer)[1] = lResult;

	    return NOERROR;
	}

	case IAVISTREAM_ReadFormat:
	    // format on input: lPos, cbFormat
	    // format on output: hresult, cbFormat, format
	{

	    LONG cbIn;
	    LONG cb;
	    DWORD lPos;
	    LPVOID lp;

	    lPos = ((DWORD FAR *)pMessage->Buffer)[0];
	    cb = cbIn = ((DWORD FAR *)pMessage->Buffer)[1];
	
	    DPF("!ReadFormat (%ld)\n", lPos);
	
	    pMessage->cbBuffer = sizeof(cbIn) + cbIn + sizeof(hrMethod);
	
	    if ((hresult = pChannel->GetBuffer(pMessage, IID_IAVIStream)) != NOERROR)
		return PropagateResult(hresult, RPC_E_SERVER_CANTUNMARSHAL_DATA);

	    lp = cbIn ? (LPBYTE) pMessage->Buffer + 2 * sizeof(DWORD) : NULL;
	
	    hrMethod = m_pAVIStream->ReadFormat(lPos, lp, &cb);

	    ((HRESULT FAR *)pMessage->Buffer)[0] = hrMethod;
	    ((DWORD FAR *)pMessage->Buffer)[1] = cb;
	    pMessage->cbBuffer = sizeof(cbIn) + sizeof(hrMethod) +
				 ((cb && cbIn) ? cb : 0);

	    return NOERROR;
	}

	case IAVISTREAM_Read:
	    // format on input: lPos, lSamples, cb
	    // format on output: hresult, samples, cb, frame
	{

	    LONG cb;
	    LONG lPos, lSamples;
	    LPVOID lp;


	    lPos = ((DWORD FAR *)pMessage->Buffer)[0];
	    lSamples = ((DWORD FAR *)pMessage->Buffer)[1];
	    cb = ((DWORD FAR *)pMessage->Buffer)[2];
	
	    DPF("!Read (%ld, %ld) ", lPos, lSamples);

	    pMessage->cbBuffer = 3 * sizeof(DWORD) + cb;
	
	    if ((hresult = pChannel->GetBuffer(pMessage, IID_IAVIStream)) != NOERROR)
		return PropagateResult(hresult, RPC_E_SERVER_CANTUNMARSHAL_DATA);

	    lp = cb ? (LPBYTE) pMessage->Buffer + 3 * sizeof(DWORD) : NULL;

            DPF("! %ld bytes ");

	    hrMethod = m_pAVIStream->Read(lPos, lSamples, lp, cb, &cb, &lSamples);

            DPF("! -> %ld bytes\n");

	    ((HRESULT FAR *)pMessage->Buffer)[0] = hrMethod;
	    ((DWORD FAR *)pMessage->Buffer)[1] = cb;
	    ((DWORD FAR *)pMessage->Buffer)[2] = lSamples;

	    return NOERROR;
	}

	default:
	    // unknown method
	
	    DPF("!Unknown method (%d)\n", pMessage->iMethod);

	    return ResultFromScode(RPC_E_UNEXPECTED);
    }
}


// return TRUE if we support given interface
STDMETHODIMP_(IRpcStubBuffer FAR *) CStubAVIStream::IsIIDSupported(REFIID iid)
{
    // if we are connected, we have already checked for this interface;
    // if we are not connected, it doesn't matter.
    return iid == IID_IAVIStream ? (IRpcStubBuffer *) this : 0;
}


// returns number of refs we have to object
STDMETHODIMP_(ULONG) CStubAVIStream::CountRefs(void)
{
    // return 1 if connected; 0 if not.
    return m_pAVIStream != NULL;
}

STDMETHODIMP CStubAVIStream::DebugServerQueryInterface(LPVOID FAR *ppv)
{
    *ppv = m_pAVIStream;

    if (!m_pAVIStream) {
	DPF("!No stream!\n");
	return ResultFromScode(E_UNEXPECTED);
    }

    return NOERROR;
}

STDMETHODIMP_(void) CStubAVIStream::DebugServerRelease(LPVOID pv)
{


}




/*
 *  IMPLEMENTATION of CPrxAVIFile
 *
 */


// create unconnected CPrxAVIFile; return controlling IProxy/IUnknokwn FAR*
IRpcProxyBuffer FAR* CPrxAVIFile::Create(IUnknown FAR* pUnkOuter)
{
    CPrxAVIFile FAR* pPrxAVIFile;

    if ((pPrxAVIFile = new CPrxAVIFile(pUnkOuter)) == NULL)
		return NULL;

    return &pPrxAVIFile->m_Proxy;
}


CPrxAVIFile::CPrxAVIFile(IUnknown FAR* pUnkOuter) : m_Proxy(this)
{
    // NOTE: could assert here since we should always be aggregated
    if (pUnkOuter == NULL)
	pUnkOuter = &m_Proxy;

    m_refs = 1;
    m_pUnkOuter = pUnkOuter;
    m_pRpcChannelBuffer = NULL;
    m_fi.dwStreams = 0;
}


CPrxAVIFile::~CPrxAVIFile(void)
{
    m_Proxy.Disconnect();
}


// Methods for controlling unknown
STDMETHODIMP CPrxAVIFile::CProxyImpl::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    if (iid == IID_IUnknown || iid == IID_IRpcProxyBuffer)
	*ppv = (void FAR *)this;
    else if (iid ==  IID_IAVIFile)
	*ppv = (void FAR *)m_pPrxAVIFile;
    else
    {
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
    }

    // simplest way to be correct: always addref the pointer we will return;
    // easy since all interfaces here are derived from IUnknown.
    ((IUnknown FAR*) *ppv)->AddRef();
    return NOERROR;
}

STDMETHODIMP_(ULONG) CPrxAVIFile::CProxyImpl::AddRef(void)
{
    return ++m_pPrxAVIFile->m_refs;
}

STDMETHODIMP_(ULONG) CPrxAVIFile::CProxyImpl::Release(void)
{
    if (--m_pPrxAVIFile->m_refs == 0)
    {
	delete m_pPrxAVIFile;
	return 0;
    }
    return m_pPrxAVIFile->m_refs;
}


// connect proxy to channel given
STDMETHODIMP CPrxAVIFile::CProxyImpl::Connect(IRpcChannelBuffer FAR* pChannelChannelBuffer)
{
    if (m_pPrxAVIFile->m_pRpcChannelBuffer != NULL)
	return ResultFromScode(E_UNEXPECTED);

    if (pChannelChannelBuffer == NULL)
	return ResultFromScode(E_INVALIDARG);

    (m_pPrxAVIFile->m_pRpcChannelBuffer = pChannelChannelBuffer)->AddRef();
    return NOERROR;
}


// disconnect proxy from any current channel
STDMETHODIMP_(void) CPrxAVIFile::CProxyImpl::Disconnect(void)
{
    if (m_pPrxAVIFile->m_pRpcChannelBuffer)
    {
	m_pPrxAVIFile->m_pRpcChannelBuffer->Release();
	m_pPrxAVIFile->m_pRpcChannelBuffer = NULL;
    }
}



// IUnknown methods for external interface(s); always delegate
STDMETHODIMP CPrxAVIFile::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{
    return m_pUnkOuter->QueryInterface(iid, ppv);
}

STDMETHODIMP_(ULONG) CPrxAVIFile::AddRef(void)
{
    return m_pUnkOuter->AddRef();
}

STDMETHODIMP_(ULONG) CPrxAVIFile::Release(void)
{
    return m_pUnkOuter->Release();
}



// IAVIFile interface methods

#ifdef WIN32
STDMETHODIMP CPrxAVIFile::Info(AVIFILEINFOW FAR * psi, LONG lSize)
#else
STDMETHODIMP CPrxAVIFile::Info(AVIFILEINFO FAR * psi, LONG lSize)
#endif
{
    HRESULT hrMarshal;
    HRESULT hrMethod = NOERROR;
    IRpcChannelBuffer FAR* pChannel = m_pRpcChannelBuffer;

    if (pChannel == NULL)
	return ResultFromScode(RPC_E_CONNECTION_TERMINATED);

    if (m_fi.dwStreams == 0) {
	RPCOLEMESSAGE Message;

	_fmemset(&Message, 0, sizeof(Message));

	// format in: lSize
	// format out: hrMethod, PS_FILEINFO
	Message.cbBuffer = sizeof(lSize);
	Message.iMethod = IAVIFILE_Info;
	
	if ((hrMarshal = pChannel->GetBuffer(&Message, IID_IAVIFile)) != NOERROR)
	    goto ErrExit;

	((DWORD FAR *)Message.Buffer)[0] = sizeof(PS_FILEINFO);
	
	if ((hrMarshal = pChannel->SendReceive(&Message, (ULONG*) &hrMethod)) != NOERROR) {
	    ;
ErrExit:
	    return PropagateResult(hrMarshal, RPC_E_CLIENT_CANTMARSHAL_DATA);
	}

	hrMethod = ((HRESULT FAR *)Message.Buffer)[0];

	PS_FILEINFO FAR * psinfo = (PS_FILEINFO FAR *)
			((LPBYTE)Message.Buffer + sizeof(HRESULT));

	// get the bits we want
	m_fi.dwMaxBytesPerSec	= psinfo->dwMaxBytesPerSec;	// max. transfer rate
	m_fi.dwFlags	= psinfo->dwFlags;		// the ever-present flags
	m_fi.dwCaps	= psinfo->dwCaps;
	m_fi.dwStreams	= psinfo->dwStreams;
	m_fi.dwSuggestedBufferSize = psinfo->dwSuggestedBufferSize;
	m_fi.dwWidth	= psinfo->dwWidth;
	m_fi.dwHeight	= psinfo->dwHeight;
	m_fi.dwScale	= psinfo->dwScale;	
	m_fi.dwRate	= psinfo->dwRate;	/* dwRate / dwScale == samples/second */
	m_fi.dwLength	= psinfo->dwLength;
	m_fi.dwEditCount = psinfo->dwEditCount;


#ifdef WIN32	
	// use unicode if we've been sent it
	if (psinfo->bHasUnicode) {
	    _fmemcpy(m_fi.szFileType,
	    	psinfo->szUnicodeType, sizeof(m_fi.szFileType));
	} else {
	    // need ansi->unicode thunk
	    MultiByteToWideChar(
	    	CP_ACP, 0,
		psinfo->szFileType,
	     	-1,
		m_fi.szFileType,
		NUMELMS(m_fi.szFileType));
	}
#else
    	// we only use the ansi which is always sent
	_fmemcpy(m_fi.szFileType, psinfo->szFileType, sizeof(m_fi.szFileType));
#endif

	pChannel->FreeBuffer(&Message);
    }

    _fmemcpy(psi, &m_fi, min((int) lSize, sizeof(m_fi)));

    return hrMethod;

}

#ifndef WIN32
STDMETHODIMP CPrxAVIFile::Open(LPCTSTR szFile, UINT mode)
{
    return ResultFromScode(E_NOTIMPL);
}
#endif

STDMETHODIMP CPrxAVIFile::GetStream(PAVISTREAM FAR * ppStream,
				     DWORD fccType,
                                     LONG lParam)
{
    HRESULT hrMarshal;
    HRESULT hrMethod = NOERROR;
    IRpcChannelBuffer FAR* pChannel = m_pRpcChannelBuffer;

    if (pChannel == NULL)
	return ResultFromScode(RPC_E_CONNECTION_TERMINATED);

    RPCOLEMESSAGE Message;

    _fmemset(&Message, 0, sizeof(Message));

    // format in: fccType lParam
    // format out: returned interface (marshalled)
    Message.cbBuffer = sizeof(fccType) + sizeof(lParam);
    Message.iMethod = IAVIFILE_GetStream;

    if ((hrMarshal = pChannel->GetBuffer(&Message, IID_IAVIFile)) != NOERROR)
	goto ErrExit;

    ((DWORD FAR *)Message.Buffer)[0] = fccType;
    ((DWORD FAR *)Message.Buffer)[1] = lParam;

    if ((hrMarshal = pChannel->SendReceive(&Message, (ULONG*) &hrMethod)) != NOERROR) {
	;
ErrExit:
	return PropagateResult(hrMarshal, RPC_E_CLIENT_CANTMARSHAL_DATA);
    }

    hrMethod = ((HRESULT FAR *)Message.Buffer)[0];

    if (hrMethod == NOERROR) {
	HGLOBAL	    h;
	LPSTREAM    pstm;

	h = GlobalAlloc(GHND, Message.cbBuffer - sizeof(hrMethod));

	hmemcpy(GlobalLock(h),
		(LPBYTE) Message.Buffer + sizeof(hrMethod),
		Message.cbBuffer - sizeof(hrMethod));

	CreateStreamOnHGlobal(h, FALSE, &pstm);
	
	CoUnmarshalInterface(pstm, IID_IAVIStream, (LPVOID FAR *) ppStream);

	pstm->Release();

	pChannel->FreeBuffer(&Message);
    }

    return hrMethod;
}

STDMETHODIMP CPrxAVIFile::CreateStream(
                                     PAVISTREAM FAR * ppStream,
#ifdef WIN32
                                     AVISTREAMINFOW FAR * psi)
#else
                                     AVISTREAMINFO FAR * psi)
#endif
{
    return ResultFromScode(E_NOTIMPL);
}

#ifndef WIN32
STDMETHODIMP CPrxAVIFile::Save(
                                     LPCTSTR szFile,
                                     AVICOMPRESSOPTIONS FAR *lpOptions,
                                     AVISAVECALLBACK lpfnCallback)
{

    return ResultFromScode(E_NOTIMPL);
}
#endif


STDMETHODIMP CPrxAVIFile::ReadData(DWORD ckid, LPVOID lp, LONG FAR *lpcb)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIFile::WriteData(DWORD ckid, LPVOID lp, LONG cb)
{
    return ResultFromScode(E_NOTIMPL);
}

STDMETHODIMP CPrxAVIFile::EndRecord()
{
    return ResultFromScode(E_NOTIMPL);
}

#ifdef WIN32
STDMETHODIMP CPrxAVIFile::DeleteStream(DWORD fccType, LONG lParam)
{
    return ResultFromScode(AVIERR_UNSUPPORTED);
}
#else
STDMETHODIMP CPrxAVIFile::Reserved1(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIFile::Reserved2(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIFile::Reserved3(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIFile::Reserved4(void)
{
    return ResultFromScode(E_NOTIMPL);
}


STDMETHODIMP CPrxAVIFile::Reserved5(void)
{
    return ResultFromScode(E_NOTIMPL);
}

#endif


/*
 *  IMPLEMENTATION of CStubAVIFile
 *	
 */

// create connected interface stub
HRESULT CStubAVIFile::Create(IUnknown FAR* pUnkObject, IRpcStubBuffer FAR* FAR* ppStub)
{
	CStubAVIFile FAR* pStubAVIFile;

	*ppStub = NULL;

    if ((pStubAVIFile = new CStubAVIFile()) == NULL)
		return ResultFromScode(E_OUTOFMEMORY);

	HRESULT hresult;
	if ((hresult = pStubAVIFile->Connect(pUnkObject)) != NOERROR)
	{
		pStubAVIFile->Release();
		return hresult;
	}

	*ppStub = pStubAVIFile;
	return NOERROR;
}


CStubAVIFile::CStubAVIFile(void)
{
    m_refs = 1;
    m_pAVIFile = NULL;
    RegisterStubUsage();
}


CStubAVIFile::~CStubAVIFile(void)
{
    UnregisterStubUsage();
    Disconnect();
}


// controling unknown methods for interface stub
STDMETHODIMP CStubAVIFile::QueryInterface(REFIID iid, LPVOID FAR* ppv)
{

    if (iid == IID_IUnknown || iid == IID_IRpcStubBuffer)
    {
	*ppv = this;
	++m_refs;
	return NOERROR;
    }
    else
    {
	*ppv = NULL;
	return ResultFromScode(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG) CStubAVIFile::AddRef(void)
{
	return ++m_refs;
}

STDMETHODIMP_(ULONG) CStubAVIFile::Release(void)
{
	if (--m_refs == 0)
	{
		delete this;
		return 0;
	}

	return m_refs;
}


// connect interface stub to server object
STDMETHODIMP CStubAVIFile::Connect(IUnknown FAR* pUnkObj)
{
	if (m_pAVIFile)
		// call Disconnect first
		return ResultFromScode(E_UNEXPECTED);

	if (pUnkObj == NULL)
		return ResultFromScode(E_INVALIDARG);
		
	// NOTE: QI ensures out param is zero if error
	return pUnkObj->QueryInterface(IID_IAVIFile, (LPVOID FAR*)&m_pAVIFile);
}


// disconnect interface stub from server objec
STDMETHODIMP_(void) CStubAVIFile::Disconnect(void)
{
	if (m_pAVIFile) {
		m_pAVIFile->Release();
		m_pAVIFile = NULL;
	}
}


// remove method call
STDMETHODIMP CStubAVIFile::Invoke
	(RPCOLEMESSAGE FAR *pMessage, IRpcChannelBuffer FAR *pChannel)
{
    HRESULT     hresult;
    HRESULT		hrMethod;
	
    if (!m_pAVIFile)
	return ResultFromScode(RPC_E_UNEXPECTED);

#if 0
    if (iid != IID_IAVIFile)
	return ResultFromScode(RPC_E_UNEXPECTED);
#endif

    switch (pMessage->iMethod)
    {
	case IAVIFILE_Info:
	    // format on input: lSize
	    // format on output: hresult, AVIFILEINFO
	{
	    DWORD lSize;
#ifdef WIN32
	    AVIFILEINFOW si;
#else
	    AVIFILEINFO si;
#endif
	    PS_FILEINFO psinfo;
	    hrMethod = m_pAVIFile->Info(&si, sizeof(si));

	    // copy all members
	    psinfo.dwMaxBytesPerSec	= si.dwMaxBytesPerSec;
	    psinfo.dwFlags	= si.dwFlags;
	    psinfo.dwCaps	= si.dwCaps;
	    psinfo.dwStreams	= si.dwStreams;
	    psinfo.dwSuggestedBufferSize	= si.dwSuggestedBufferSize;
	    psinfo.dwWidth	= si.dwWidth;
	    psinfo.dwHeight	= si.dwHeight;
	    psinfo.dwScale	= si.dwScale;	
	    psinfo.dwRate	= si.dwRate;
	    psinfo.dwLength	= si.dwLength;
	    psinfo.dwEditCount	= si.dwEditCount;

#ifdef WIN32	
	    // send both UNICODE and ansi
	    hmemcpy(psinfo.szUnicodeType, si.szFileType, NUMELMS(psinfo.szFileType));
	    psinfo.bHasUnicode = TRUE;
	    WideCharToMultiByte(CP_ACP, 0,
	    	si.szFileType,
		-1,
		psinfo.szFileType,
		NUMELMS(psinfo.szFileType),
		NULL, NULL);
#else
    	    // just send ansi version for 16-bit stub
	    psinfo.bHasUnicode = FALSE;
	    hmemcpy(psinfo.szFileType, si.szFileType, sizeof(si.szFileType));
#endif

	    lSize = ((DWORD FAR *)pMessage->Buffer)[0];

	    pMessage->cbBuffer = lSize + sizeof(hrMethod);
	
	    if ((hresult = pChannel->GetBuffer(pMessage, IID_IAVIStream)) != NOERROR)
		return PropagateResult(hresult, RPC_E_SERVER_CANTUNMARSHAL_DATA);

	    ((HRESULT FAR *)pMessage->Buffer)[0] = hrMethod;

	    hmemcpy((LPBYTE) pMessage->Buffer + sizeof(hrMethod),
		    &psinfo,
		    lSize);


	    return NOERROR;
	}

	case IAVIFILE_GetStream:
	    // format on input: fccType, lParam
	    // format on output: marshalled IAVIStream pointer
	{
	    DWORD	    lParam, fccType;
	    PAVISTREAM	    ps;
	    HGLOBAL	    h;
	    DWORD	    dwDestCtx = 0;
	    LPVOID	    pvDestCtx = NULL;
	    DWORD	    cb;
	    LPSTREAM	    pstm;
	
	    fccType = ((DWORD FAR *)pMessage->Buffer)[0];
	    lParam = ((DWORD FAR *)pMessage->Buffer)[1];
	
	    hrMethod = m_pAVIFile->GetStream(&ps, fccType, lParam);

	    if (hrMethod == NOERROR) {

		pChannel->GetDestCtx(&dwDestCtx, &pvDestCtx);

#ifdef WIN32
		cb = 0;
		CoGetMarshalSizeMax(&cb, IID_IAVIStream, ps,
				    dwDestCtx, pvDestCtx, MSHLFLAGS_NORMAL);
#else
		cb = 800; // !!!!!!!
#endif

		h = GlobalAlloc(GHND, cb);

		CreateStreamOnHGlobal(h, FALSE, &pstm);

		CoMarshalInterface(pstm, IID_IAVIStream, ps,
				   dwDestCtx, pvDestCtx, MSHLFLAGS_NORMAL);

		pstm->Release();
	    } else
		cb = 0;

	    pMessage->cbBuffer = cb + sizeof(hrMethod);
	
	    if ((hresult = pChannel->GetBuffer(pMessage, IID_IAVIStream)) != NOERROR)
		return PropagateResult(hresult, RPC_E_SERVER_CANTUNMARSHAL_DATA);

	    ((HRESULT FAR *)pMessage->Buffer)[0] = hrMethod;

	    if (cb) {
		hmemcpy((LPBYTE) pMessage->Buffer + sizeof(hrMethod),
			GlobalLock(h), cb);

		GlobalFree(h);
	    }

	    return NOERROR;

	}


	default:
		// unknown method
		return ResultFromScode(RPC_E_UNEXPECTED);
	}
}


// return TRUE if we support given interface
STDMETHODIMP_(IRpcStubBuffer FAR *) CStubAVIFile::IsIIDSupported(REFIID iid)
{
	// if we are connected, we have already checked for this interface;
	// if we are not connected, it doesn't matter.
	return iid == IID_IAVIFile ? (IRpcStubBuffer *) this : 0;
}


// returns number of refs we have to object
STDMETHODIMP_(ULONG) CStubAVIFile::CountRefs(void)
{
	// return 1 if connected; 0 if not.
	return m_pAVIFile != NULL;
}



STDMETHODIMP CStubAVIFile::DebugServerQueryInterface(LPVOID FAR *ppv)
{
    *ppv = m_pAVIFile;

    if (!m_pAVIFile) {
	DPF("!No File!\n");
	return ResultFromScode(E_UNEXPECTED);
    }

    return NOERROR;
}

STDMETHODIMP_(void) CStubAVIFile::DebugServerRelease(LPVOID pv)
{


}



//
// The following functions exist to allow an application to determine
// if another application is using any of its objects.
//
// !!!!!!   I don't know if this really works.
//

#define MAXTASKCACHE	64
HTASK	ahtaskUsed[MAXTASKCACHE];
int	aiRefCount[MAXTASKCACHE];

void RegisterStubUsage(void)
{
    HTASK htask = GetCurrentTask();
    int i;

    for (i = 0; i < MAXTASKCACHE; i++) {
	if (ahtaskUsed[i] == htask) {
	    ++aiRefCount[i];
	    return;
	}
    }

    for (i = 0; i < MAXTASKCACHE; i++) {
	if (ahtaskUsed[i] == NULL) {
	    ahtaskUsed[i] = htask;
	    aiRefCount[i] = 1;
	    return;
	}
    }

    DPF("Ack: Proxy cache full!\n");
}

void UnregisterStubUsage(void)
{
    HTASK htask = GetCurrentTask();
    int i;

    for (i = 0; i < MAXTASKCACHE; i++) {
	if (ahtaskUsed[i] == htask) {
	    if (--aiRefCount[i] <= 0) {
		ahtaskUsed[i] = NULL;
		aiRefCount[i] = 0;
	    }
	    return;
	}
    }

    DPF("Ack: Proxy not in cache!\n");
}

BOOL FAR TaskHasExistingProxies(void)
{
    HTASK htask = GetCurrentTask();
    int i;

    for (i = 0; i < MAXTASKCACHE; i++) {
	if (ahtaskUsed[i] == htask) {
	    return TRUE;
	}
    }

    return FALSE;
}

