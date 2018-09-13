/*

    File: PlgProt.cpp

    Copyright (c) 1997-1999 Microsoft Corporation.  All Rights Reserved.

    Abstract:

    History:
        06/26/97    Cgomes - ported from Trident
		03/20/98	Vank   - ported from VID/htmed

	This pluggable protocol handler allows the control to override
	URL combining, parsing the security URL, and loading data.

	The control implements a property for the BaseURL which is set
	properly by default, but can be overridden by the user.  To make
	this work we override CombineURL.

	To assure that the control is safe when hosted by IE but powerful
	when hosted by VB, we override ParseURL(PARSE_SECURITY_URL) and
	return a URL representing the zone of the outermost hosting Trident,
	or the path to the drive where the DLL is installed if Trident is
	not our host.  This correctly handles cases where we're hosted on
	an intranet page, which is hosted in an internet page, which is
	hosted in an intranet page, etc.  The topmost container's security
	zone is the one returned to IE.
	
	Finally, the control is in charge of saying what data is to be loaded.
	This can be set from a file, a URL, or a BSTR.

	NOTE:
	TSDK had the unusual requirement of having to be able to register
	even when it could not run.  WinINet and UrlMon were dynamically
	loaded when the control was instantiated.  This is clearly not
	necessary when we're a part of IE5, so this code has been disabled
	using the define LATE_BIND_URLMON_WININET.

*/
#include "stdafx.h"
#include <wininet.h>
#include "plgprot.h"
#include "dhtmledit.h"


//////////////////////////////////////////////////////////////////////////////
//
//  DHTMLEd Protocol Implementaion
//
CDHTMLEdProtocolInfo::CDHTMLEdProtocolInfo()
{
	ATLTRACE(_T("CDHTMLEdProtocolInfo::CDHTMLEdProtocolInfo\n"));

	m_fZombied			= FALSE;
	m_pProxyFrame		= NULL;
	m_piProtocolConIntf	= NULL;
}


CDHTMLEdProtocolInfo::~CDHTMLEdProtocolInfo()
{
	ATLTRACE(_T("CDHTMLEdProtocolInfo::~CDHTMLEdProtocolInfo\n"));

	Zombie();
}


void CDHTMLEdProtocolInfo::Zombie()
{
	m_fZombied = TRUE;
	if ( NULL != m_piProtocolConIntf )
	{
		m_piProtocolConIntf->Release ();
		m_piProtocolConIntf = NULL;
	}
}


//////////////////////////////////////////////////////////////////////////////
//
//  IClassFactory Implementation
//

STDMETHODIMP CDHTMLEdProtocolInfo::CreateInstance
(
	IUnknown* 	/*pUnkOuter*/,
	REFIID 	   	riid,
	void**		ppvObject
)
{
	ExpectedExpr((!m_fZombied));
	InitParam(ppvObject);
	IfNullRet(ppvObject);

	HRESULT hr;

	// Only support creating the DHTMLEdProtocol object

	AtlCreateInstance(CDHTMLEdProtocol, riid, ppvObject);
	_ASSERTE(*ppvObject != NULL);

	if(*ppvObject == NULL)
		return E_NOINTERFACE;
	else
	{
		hr = (reinterpret_cast<IUnknown*>(*ppvObject)->QueryInterface) ( IID_IProtocolInfoConnector, (LPVOID*) &m_piProtocolConIntf );
		_ASSERTE ( SUCCEEDED ( hr ) && m_piProtocolConIntf );
		if ( SUCCEEDED ( hr ) && m_piProtocolConIntf )
		{
			if ( NULL != m_pProxyFrame )
			{
				m_piProtocolConIntf->SetProxyFrame ( (SIZE_T*)m_pProxyFrame );
			}
		}
		return NOERROR;
	}
}


STDMETHODIMP CDHTMLEdProtocolInfo::RemoteCreateInstance
(
	REFIID 		/*riid*/,
	IUnknown** 	/*ppvObject*/
)
{
	ExpectedExpr((!m_fZombied));
	ATLTRACENOTIMPL(_T("RemoteCreateInstance"));
}


STDMETHODIMP CDHTMLEdProtocolInfo::LockServer(BOOL /*fLock*/)
{
	ExpectedExpr((!m_fZombied));
	ATLTRACE(_T("CDHTMLEdProtocolInfo::LockServer\n"));

	return NOERROR;
}


STDMETHODIMP CDHTMLEdProtocolInfo::RemoteLockServer(BOOL /*fLock*/)
{
	ExpectedExpr((!m_fZombied));
	ATLTRACENOTIMPL(_T("RemoteLockServer"));
}


//////////////////////////////////////////////////////////////////////////////
//
//  IInternetProtocolInfo Implementation
//


//	Override the BaseURL
//
STDMETHODIMP CDHTMLEdProtocolInfo::CombineUrl
(
    LPCWSTR     pwzBaseURL,
    LPCWSTR     pwzRelativeURL,
    DWORD       /*dwFlags*/,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       /*dwReserved*/
)
{
	_ASSERTE ( m_pProxyFrame );

	CComBSTR bstrBaseURL;
#ifdef LATE_BIND_URLMON_WININET
	PFNCoInternetCombineUrl pfnCoInternetCombineUrl = m_pProxyFrame->m_pfnCoInternetCombineUrl;
#endif // LATE_BIND_URLMON_WININET

	ExpectedExpr((!m_fZombied));
	InitParam(pcchResult);
	IfNullGo(pwzBaseURL);
	IfNullGo(pwzRelativeURL);
	IfNullGo(pwzResult);
	IfNullGo(pcchResult);

	ATLTRACE(_T("CDHTMLEdProtocolInfo::CombineUrl(%ls,%ls)\n"), pwzBaseURL, pwzRelativeURL);

	HRESULT hr;
	_ASSERTE ( m_pProxyFrame );
	IfNullGo(m_pProxyFrame);

	// GetBaseURL returns the value of the control's BaseURL property.  The pwzBaseURL parameter is ignored.
	hr = m_pProxyFrame->GetBaseURL(bstrBaseURL);
	_IfFailGo(hr);

	// Handle case where return buffer is too small
	*pcchResult  = bstrBaseURL.Length () + 1;
	if(*pcchResult > cchResult)
	{
		return S_FALSE;
	}

	// combine with our base url
#ifdef LATE_BIND_URLMON_WININET
	_ASSERTE ( pfnCoInternetCombineUrl );
    hr = (*pfnCoInternetCombineUrl) ( bstrBaseURL, pwzRelativeURL, ICU_ESCAPE, pwzResult, cchResult, pcchResult, 0 );
#else
    hr = CoInternetCombineUrl ( bstrBaseURL, pwzRelativeURL, ICU_ESCAPE, pwzResult, cchResult, pcchResult, 0 );
#endif // LATE_BIND_URLMON_WININET

	IfFailGo(hr);

	if ( S_OK == hr )
		ATLTRACE(_T("CDHTMLEdProtocolInfo::CombinUrl to %ls\n"), pwzResult);

	return hr;

ONERROR:
	return INET_E_DEFAULT_ACTION;
}


STDMETHODIMP CDHTMLEdProtocolInfo::CompareUrl
(
    LPCWSTR     /*pwzUrl1*/,
    LPCWSTR     /*pwzUrl2*/,
    DWORD       /*dwFlags*/
)
{
	ExpectedExpr((!m_fZombied));
    return E_NOTIMPL;
}


//	Override the security URL.  See comments at top of file.
//
STDMETHODIMP CDHTMLEdProtocolInfo::ParseUrl
(
    LPCWSTR     pwzURL,
    PARSEACTION ParseAction,
    DWORD       /*dwFlags*/,
    LPWSTR      pwzResult,
    DWORD       cchResult,
    DWORD *     pcchResult,
    DWORD       /*dwReserved*/
)
{
	ExpectedExpr((!m_fZombied));
	IfNullRet(pwzURL);
	InitParam(pcchResult);

	ATLTRACE(_T("CDHTMLEdProtocolInfo::ParseUrl(%d, %ls)\n"), (int)ParseAction, pwzURL);

	HRESULT hr;

	switch(ParseAction)
	{
		case PARSE_SECURITY_URL:
		{
			_ASSERTE(m_pProxyFrame != NULL);

			if(m_pProxyFrame != NULL)
			{
				CComBSTR bstrSecurityURL;

				hr = m_pProxyFrame->GetSecurityURL(bstrSecurityURL);

				if(SUCCEEDED(hr))
				{
					// set out param
					*pcchResult = bstrSecurityURL.Length () + 1;

					if(*pcchResult <= cchResult)
					{
						// copy result
						wcscpy(pwzResult, bstrSecurityURL);

						ATLTRACE(_T("CDHTMLEdProtocolInfo::ParseUrl(%ls)\n"), pwzResult);

						return NOERROR;
					}
					else
						return S_FALSE; // buffer too small
				}
			}
		}
		break;

		case PARSE_CANONICALIZE:
		case PARSE_FRIENDLY:
		case PARSE_DOCUMENT:
		case PARSE_PATH_FROM_URL:
		case PARSE_URL_FROM_PATH:
		case PARSE_ROOTDOCUMENT:
		case PARSE_ANCHOR:
		case PARSE_ENCODE:
		case PARSE_DECODE:
		case PARSE_MIME:
		case PARSE_SERVER:
		case PARSE_SCHEMA:
		case PARSE_SITE:
		case PARSE_DOMAIN:
		case PARSE_LOCATION:
		case PARSE_SECURITY_DOMAIN:
		default:
    		return INET_E_DEFAULT_ACTION;
	}

    return INET_E_DEFAULT_ACTION;
}

STDMETHODIMP CDHTMLEdProtocolInfo::QueryInfo
(
    LPCWSTR         /*pwzURL*/,
    QUERYOPTION     QueryOption,
    DWORD           /*dwQueryFlags*/,
    LPVOID          pBuffer,
    DWORD           /*cbBuffer*/,
    DWORD *         pcbBuf,
    DWORD           /*dwReserved*/
)
{
	ExpectedExpr((!m_fZombied));
	InitParam(pcbBuf);
	IfNullRet(pBuffer);
	IfNullRet(pcbBuf);

	switch(QueryOption)
	{
		case QUERY_CONTENT_TYPE:
		case QUERY_EXPIRATION_DATE:
		case QUERY_TIME_OF_LAST_CHANGE:
		case QUERY_CONTENT_ENCODING:
		case QUERY_REFRESH:
		case QUERY_RECOMBINE:
		case QUERY_CAN_NAVIGATE:
		default:
			break;
	}

    return INET_E_DEFAULT_ACTION;
}


//	This type of strong coupling should normally be avoided, but it was
//	fast, simple, and safe in this case.
//
STDMETHODIMP CDHTMLEdProtocolInfo::SetProxyFrame ( SIZE_T* vpProxyFrame )
{
	m_pProxyFrame = (CProxyFrame*)vpProxyFrame;
	if ( NULL != m_piProtocolConIntf )
	{
		m_piProtocolConIntf->SetProxyFrame ( vpProxyFrame );
	}
	return S_OK;
}


STDMETHODIMP CDHTMLEdProtocol::SetProxyFrame ( SIZE_T* vpProxyFrame )
{
	m_pProxyFrame = (CProxyFrame*)vpProxyFrame;
	return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
//  DHTMLEd Protocol Implementaion
//


CDHTMLEdProtocol::CDHTMLEdProtocol()
{
	ATLTRACE(_T("CDHTMLEdProtocol::CDHTMLEdProtocol\n"));

	m_fZombied = FALSE;
	m_fAborted = FALSE;
    m_bscf = BSCF_FIRSTDATANOTIFICATION;
	m_pProxyFrame = NULL;

}


CDHTMLEdProtocol::~CDHTMLEdProtocol()
{
	ATLTRACE(_T("CDHTMLEdProtocol::~CDHTMLEdProtocol\n"));
	Zombie();
}


void CDHTMLEdProtocol::Zombie()
{
	m_fZombied = TRUE;
	m_srpSink.Release();
	m_srpBindInfo.Release();
	m_srpStream.Release();
	m_bstrBaseURL.Empty();
}


/*

    HRESULT ParseAndBind

    Description:
		Gets the stream from the control and begins returning data to IE.

*/
HRESULT CDHTMLEdProtocol::ParseAndBind()
{
	HRESULT hr;
	STATSTG sstg = {0};

	_ASSERTE(m_bstrBaseURL != NULL);
	_ASSERTE(m_srpStream == NULL);

	hr = m_pProxyFrame->GetFilteredStream(&m_srpStream);

	IfFailGo(hr);
	IfNullPtrGo(m_srpStream);

	// Read in size of the stream

	hr = m_srpStream->Stat(&sstg, STATFLAG_NONAME);
	IfFailGo(hr);

// fall through

ONERROR:

	if(!m_fAborted)
	{
		// Report Data to sink
		if(m_srpSink != NULL)
		{
			DWORD bscf = m_bscf | BSCF_DATAFULLYAVAILABLE | BSCF_LASTDATANOTIFICATION;

			// Specify mime / type as HTML
			m_srpSink->ReportProgress(BINDSTATUS_MIMETYPEAVAILABLE, L"text/html");

			// Report size of data
			ATLTRACE(_T("CDHTMLEdProtocol::ParseAndBind(%d bytes)\n"), sstg.cbSize.LowPart);
			m_srpSink->ReportData(bscf, sstg.cbSize.LowPart, sstg.cbSize.LowPart);

			// Report result should be called only when all data have been read by consumer
			// IE4 accepts ReportResult() here while IE5 does not. This is because IE4 queues
			// the report while IE5 executes it immediatetely, terminating the VID protocol.
			// See VID bug #18128 for additional details.
			//if(m_srpSink != NULL)
			//{
			//	m_srpSink->ReportResult(hr, 0, 0);  DO NOT DO THIS!
			//}
		}
	}
    return hr;
}


/*

    void ReportData

    Description:
        Report to sink data is fully available

*/
void CDHTMLEdProtocol::ReportData(ULONG cb)
{
    m_bscf |= BSCF_LASTDATANOTIFICATION | BSCF_DATAFULLYAVAILABLE;

	if(m_srpSink != NULL)
	{
    	m_srpSink->ReportData(m_bscf, cb, cb);
	}
}


//////////////////////////////////////////////////////////////////////////////
//
//  IInternetProtocol Implementation
//


STDMETHODIMP CDHTMLEdProtocol::LockRequest(DWORD /*dwOptions*/)
{
	ExpectedExpr((!m_fZombied));
    return S_OK;
}


STDMETHODIMP CDHTMLEdProtocol::Read(void *pv, ULONG cb, ULONG *pcbRead)
{
	ATLTRACE(_T("CDHTMLEdProtocol::Read(%ls) %d bytes\n"), m_bstrBaseURL ? m_bstrBaseURL : L"(null)", cb);

	if(m_fZombied)
		return S_FALSE;

	_ASSERTE(m_srpStream != NULL);
	if(m_srpStream == NULL)
		return INET_E_DOWNLOAD_FAILURE;

	HRESULT hr;

	hr = m_srpStream->Read(pv, cb, pcbRead);
	_ASSERTE(SUCCEEDED(hr));

	if(FAILED(hr))
		return INET_E_DOWNLOAD_FAILURE;

	ATLTRACE(_T("CDHTMLEdProtocol::Read returning hr=%08X %d bytes read\n"), ((*pcbRead) ? hr : S_FALSE), *pcbRead);

	if(*pcbRead)
		return hr;
	else
	{
		// Tell the sink that I am done reading.
		m_srpSink->ReportResult(S_FALSE, 0, 0);
		return S_FALSE;
	}
}


STDMETHODIMP CDHTMLEdProtocol::Seek
(
    LARGE_INTEGER 	dlibMove,
    DWORD 			dwOrigin,
    ULARGE_INTEGER 	*plibNewPosition
)
{
	ATLTRACE(_T("CDHTMLEdProtocol::Seek(%ls)\n"), m_bstrBaseURL);

	ExpectedExpr((!m_fZombied));
	ExpectedPtr(m_srpStream);

	HRESULT hr;

	// Do the seek

    hr = m_srpStream->Seek(dlibMove, dwOrigin, plibNewPosition);
	IfFailRet(hr);

	return hr;
}


STDMETHODIMP CDHTMLEdProtocol::UnlockRequest()
{
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
//  IInternetProtocolRoot Implementation
//


STDMETHODIMP CDHTMLEdProtocol::Start
(
    LPCWSTR 				pwzURL,
    IInternetProtocolSink 	*pSink,
    IInternetBindInfo 		*pBindInfo,
    DWORD 					grfSTI,
    HANDLE_PTR				/*dwReserved*/
)
{
	ATLTRACE(_T("CDHTMLEdProtocol::Start(%ls)\n"), pwzURL);
	_ASSERTE ( m_pProxyFrame );

#ifdef LATE_BIND_URLMON_WININET
	PFNCoInternetParseUrl pfnCoInternetParseUrl = m_pProxyFrame->m_pfnCoInternetParseUrl;
#endif // LATE_BIND_URLMON_WININET

	ExpectedExpr((!m_fZombied));
	IfNullRet(pwzURL);
	IfNullRet(pBindInfo);
	IfNullRet(pSink);

    HRESULT         hr;
    WCHAR           wch[INTERNET_MAX_URL_LENGTH];
    DWORD           dwSize;

	_ASSERTE(m_srpSink == NULL);
	_ASSERTE(m_bstrBaseURL == NULL);

    if( !(grfSTI & PI_PARSE_URL))
    {
		m_srpSink.Release();
		m_srpSink = pSink;

		m_srpBindInfo.Release();
		m_srpBindInfo = pBindInfo;
    }

    m_bindinfo.cbSize = sizeof(BINDINFO);
    hr = pBindInfo->GetBindInfo(&m_grfBindF, &m_bindinfo);
	IfFailGo(hr);

	ATLTRACE(_T("CDHTMLEdProtocol::BINDF                    =%08X\n"), 	m_grfBindF);
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.szExtraInfo     =%ls\n"), 	m_bindinfo.szExtraInfo ? m_bindinfo.szExtraInfo : L"(null)");
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.grfBindInfoF    =%08X\n"), 	m_bindinfo.grfBindInfoF);
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.dwBindVerb      =%08X\n"), 	m_bindinfo.dwBindVerb);
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.szCustomVerb    =%ls\n"),   	m_bindinfo.szCustomVerb ? m_bindinfo.szCustomVerb : L"(null)");
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.cbstgmedData    =%08X\n"), 	m_bindinfo.cbstgmedData);
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.dwOptions       =%08X\n"), 	m_bindinfo.dwOptions);
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.dwOptionsFlags  =%08X\n"), 	m_bindinfo.dwOptionsFlags);
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.dwCodePage      =%08X\n"), 	m_bindinfo.dwCodePage);
	ATLTRACE(_T("CDHTMLEdProtocol::BindInfo.dwReserved      =%08X\n"), 	m_bindinfo.dwReserved);

    //
    // First get the basic url.  Unescape it first.
    //

#ifdef LATE_BIND_URLMON_WININET
	_ASSERTE ( pfnCoInternetParseUrl );
    hr = (*pfnCoInternetParseUrl)( pwzURL, PARSE_ENCODE, 0, wch, dimensionof(wch), &dwSize, 0 );
#else
    hr = CoInternetParseUrl ( pwzURL, PARSE_ENCODE, 0, wch, dimensionof(wch), &dwSize, 0 );
#endif // LATE_BIND_URLMON_WININET

	IfFailGo(hr);

	m_bstrBaseURL = wch;
	IfNullPtrGo(m_bstrBaseURL.m_str);

    //
    // Now append any extra data if needed.
    //

    if (m_bindinfo.szExtraInfo)
    {
		m_bstrBaseURL.Append(m_bindinfo.szExtraInfo);
		IfNullPtrGo(m_bstrBaseURL.m_str);
    }

    m_grfSTI = grfSTI;

    //
    // If forced to go async, return E_PENDING now, and
    // perform binding when we get the Continue.
    //

    if (grfSTI & PI_FORCE_ASYNC)
    {
        PROTOCOLDATA    protdata;

        hr = E_PENDING;
        protdata.grfFlags = PI_FORCE_ASYNC;
        protdata.dwState = BIND_ASYNC;
        protdata.pData = NULL;
        protdata.cbData = 0;

        m_srpSink->Switch(&protdata);
    }
    else
    {
        hr = ParseAndBind();
		IfFailGo(hr);
    }

	return hr;

ONERROR:
    return hr;
}


STDMETHODIMP CDHTMLEdProtocol::Continue(PROTOCOLDATA *pStateInfoIn)
{
	ATLTRACE(_T("CDHTMLEdProtocol::Continue(%ls)\n"), m_bstrBaseURL);

	ExpectedExpr((!m_fZombied));
	IfNullRet(pStateInfoIn);

    HRESULT hr = E_FAIL;

	_ASSERTE(pStateInfoIn->pData != NULL);
	_ASSERTE(pStateInfoIn->cbData != 0);
	_ASSERTE(pStateInfoIn->dwState == (DWORD) BIND_ASYNC);

    if(pStateInfoIn->dwState == BIND_ASYNC)
    {
        hr =  ParseAndBind();
    }

    return hr;
}


STDMETHODIMP CDHTMLEdProtocol::Abort(HRESULT /*hrReason*/, DWORD /*dwOptions*/)
{
	ATLTRACE(_T("CDHTMLEdProtocol::Abort(%ls)\n"), m_bstrBaseURL);

	ExpectedExpr((!m_fZombied));

    m_fAborted = TRUE;

	ExpectedPtr(m_srpSink);

    return m_srpSink->ReportResult(E_ABORT, 0, 0);
}


STDMETHODIMP CDHTMLEdProtocol::Terminate(DWORD dwOptions)
{
	ATLTRACE(_T("CDHTMLEdProtocol::Terminate(%08X, %ls)\n"), dwOptions, m_bstrBaseURL);

	ExpectedExpr((!m_fZombied));

    if (m_bindinfo.stgmedData.tymed != TYMED_NULL)
    {
        ::ReleaseStgMedium(&(m_bindinfo.stgmedData));
        m_bindinfo.stgmedData.tymed = TYMED_NULL;
    }

    if (m_bindinfo.szExtraInfo)
    {
        ::CoTaskMemFree(m_bindinfo.szExtraInfo);
        m_bindinfo.szExtraInfo = NULL;
    }

	Zombie();

    return NOERROR;
}


STDMETHODIMP CDHTMLEdProtocol::Suspend()
{
	ATLTRACE(_T("CDHTMLEdProtocol::Suspend(%ls)\n"), m_bstrBaseURL);

	ExpectedExpr((!m_fZombied));
    return E_NOTIMPL;
}


STDMETHODIMP CDHTMLEdProtocol::Resume()
{
	ATLTRACE(_T("CDHTMLEdProtocol::Resume(%ls)\n"), m_bstrBaseURL);

	ExpectedExpr((!m_fZombied));
    return E_NOTIMPL;
}

/* end of file */
