/*
* 
* Copyright (c) 1998,1999 Microsoft Corporation. All rights reserved.
* EXEMPT: copyright change only, no build required
* 
*/
// XMLHTTP.h : Declaration of the CXMLHTTP

#ifndef _XML_XML_HTTP_H_
#define _XML_XML_HTTP_H_

#include <wininet.h>
#include <winineti.h>

DEFINE_CLASS(CXMLHTTP);

extern HRESULT STDMETHODCALLTYPE CreateXMLHTTPRequest(REFIID iid, void **ppvObj);

typedef INTERNETAPI INTERNET_STATUS_CALLBACK InternetSetStatusCallbackFunc(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
);

class XMLHTTPWrapper : public _dispatchexport<CXMLHTTP, IXMLHttpRequest, &LIBID_MSXML, ORD_MSXML, &IID_IXMLHttpRequest>
{
public: 
    
    XMLHTTPWrapper(CXMLHTTP * p) : 
        _dispatchexport<CXMLHTTP, IXMLHttpRequest, &LIBID_MSXML, ORD_MSXML, &IID_IXMLHttpRequest>(p)
        {}

    ~XMLHTTPWrapper() {}

    // IXMLHttpRequest methods
	STDMETHOD(open)					(BSTR bstrMethod, BSTR bstrUrl, VARIANT fAsync,
									 VARIANT bstrUser, VARIANT  bstrPassword);

	STDMETHOD(setRequestHeader)		(BSTR bstrHeader, BSTR bstrValue);
	STDMETHOD(getResponseHeader)	(BSTR bstrHeader, BSTR * pbstrValue);

	STDMETHOD(send)					(VARIANT bstrBody);
	STDMETHOD(abort)				();

	STDMETHOD(get_status)			(long * plStatus);
	STDMETHOD(get_statusText)		(BSTR * pbstrStatus);

	STDMETHOD(getAllResponseHeaders)(BSTR * pbstrHeaders);

	STDMETHOD(get_responseXML)		(IDispatch ** ppBody);
	STDMETHOD(get_responseText)		(BSTR * pbstrBody);
	STDMETHOD(get_responseBody)		(VARIANT * pvarBody);
	STDMETHOD(get_responseStream)	(VARIANT * pvarBody);

	STDMETHOD(get_readyState)		(long * plState);
	STDMETHOD(put_onreadystatechange)	(IDispatch * pReadyStateSink);
};


struct HttpContext
{
	HttpContext(CXMLHTTP * p)
	{
		pXmlHttp = p;
		::InitializeCriticalSection(&CriticalSection);
		hHTTP = NULL;
	}

	~HttpContext()
	{
		DeleteCriticalSection(&CriticalSection);
	}

	void Lock()
	{
		EnterCriticalSection(&CriticalSection);
	}
	
	void UnLock()
	{
		LeaveCriticalSection(&CriticalSection);
	}
	
	CXMLHTTP *			pXmlHttp;
	CRITICAL_SECTION	CriticalSection;
	HINTERNET			hHTTP;

};


/////////////////////////////////////////////////////////////////////////////
// CXMLHTTP
class CXMLHTTP : public CSafeControl
{
	// Friend functions
	friend void InternetStatusCallBack(
				    HINTERNET	hInternet,
				    DWORD       dwContext,
				    DWORD		dwInternetStatus,
				    LPVOID		lpvStatusInfo,
				    DWORD		dwStatusInfoLen);

	friend LRESULT  WndProcMain(
	                HWND    hwnd,
	                UINT    msg,
	                WPARAM  wParam,
	                LPARAM  lParam);

    DECLARE_CLASS_MEMBERS_NOQI_INTERFACE(CXMLHTTP, CSafeControl);
public:

	CXMLHTTP();
	~CXMLHTTP();

    //---------------------------------------------------------------
    // IUnknown override.
    //---------------------------------------------------------------
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void ** ppvObject);

    public: ULONG _release();

    virtual void finalize()
    {
        super::finalize();
    }

// IXMLHttpRequest
public:
	STDMETHOD(open)					(BSTR bstrMethod, BSTR bstrUrl, VARIANT varAsync,
									 VARIANT bstrUser, VARIANT  bstrPassword);

	STDMETHOD(setRequestHeader)		(BSTR bstrHeader, BSTR bstrValue);
	STDMETHOD(getResponseHeader)	(BSTR bstrHeader, BSTR * pbstrValue);

	STDMETHOD(send)					(VARIANT bstrBody, BOOL fResend);
	STDMETHOD(abort)				();

	STDMETHOD(get_status)			(long * plStatus);
	STDMETHOD(get_statusText)		(BSTR * pbstrStatus);

	STDMETHOD(getAllResponseHeaders)(BSTR * pbstrHeaders);

	STDMETHOD(get_responseXML)		(IDispatch ** ppBody);
	STDMETHOD(get_responseText)		(BSTR * pbstrBody);
	STDMETHOD(get_responseBody)		(VARIANT * pvarBody);
	STDMETHOD(get_responseStream)	(VARIANT * pvarBody);

	STDMETHOD(get_readyState)		(long * plState);
	STDMETHOD(put_onreadystatechange)	(IDispatch * pReadyStateSink);

private:
	enum State
	{
		CREATED		= 0,
		OPENED		= 1,
		SENDING		= 2,
		SENT		= 3,
		RECEIVING	= 4,
		RESPONSE	= 5
	};
	
	// Helpers
    HRESULT CreateStreamOnResponse(IStream ** ppStm);
	HRESULT LoadViaPersistStream(IDispatch * pDispatch);
	HRESULT SetRequestBody(VARIANT varBody);
	HRESULT SetRequiredRequestHeaders();
	HRESULT SetRequestHeader(char * szHeader, char * szValue, BOOL fReplace = FALSE);
	
	void SetState(State eState);
	HRESULT SetTargetUrl(char * szUrl);
	BOOL Authenticate();
	HRESULT AccessAllowed();
	HRESULT GetContentType();
    HRESULT	ReadResponse();

	HRESULT CreateXMLDocument();

	HRESULT	GetWindow(HWND& hwnd);

	HRESULT	CrackUrl(BSTR bstrUrl, INTERNET_SCHEME& isScheme,
					 char ** pszHostname, INTERNET_PORT& ipPort,
					 char ** pszPath);

	void Initialize();
	void ReleaseResources();
    void Reset(bool cleanupall = false);
    void SetErrorInfo(HRESULT hr, DWORD dwLastError);

    DWORD       m_cbAvail;
    DWORD       m_cbRead;

	State		m_eState;

	// Internet handles
    HINSTANCE   m_hWininet;     // WININET DLL Handle
	HINTERNET	m_hInet;		// Handle from InternetOpen
	HINTERNET	m_hSession;		// Handle from InternetConnect
	HINTERNET	m_hHTTP;		// Handle from HttpOpenRequest

	// Buffers and counters
	BSTR		m_bstrTargetUrl;
	char *		m_szRequestBuffer;
	ULONG		m_cbRequestBody;
	char *		m_szResponseBuffer;
	ULONG		m_cbResponseBuffer;
	ULONG		m_cbResponseBody;
    InternetSetStatusCallbackFunc * m_pfnInternetSetStatusCallback;

	// XML document interfaces
    IXMLDOMDocument *	m_pXMLDoc;
    IXMLParser *		m_pXMLParser;

	// Events
    IDispatch *         m_pReadyStateSink;
    HWND            m_hWnd;

	// User agent
	char *		m_szUserAgent;
	
	// Flags
	BOOL		m_fUseridAndPassword;
	BOOL		m_fXMLResponse;
	BOOL		m_fChunkedRequest;
	BOOL		m_fContentDetermined;
	BOOL		m_fXDomainUrlChecked;

	// Asynchronous members
	BOOL		m_fAsync;
	DWORD		m_dwStatus;
	HttpContext * m_pContext;
	
    RMutex      m_pMutex;
};

#endif
