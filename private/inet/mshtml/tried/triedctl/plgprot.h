/*

    File: PlgProt.h

    Copyright (c) 1997-1999 Microsoft Corporation.  All Rights Reserved.

    Abstract:
		DHTMLEd Pluggable Protocol

    History:
        06/26/97    Cgomes - ported from trident
		03/20/98	Vank   - ported from VID/htmed

*/
#if !defined __INC_PLGPROT_H__
#define __INC_PLGPRO_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "resource.h"
#include "dhtmled.h"
#include "proxyframe.h"
#include "private.h"

EXTERN_C const CLSID CLSID_DHTMLEdProtocolInfo;
EXTERN_C const CLSID CLSID_DHTMLEdProtocol;

#define BIND_ASYNC 1

#define ExpectedExpr(expr)		\
		_ASSERTE((expr));			\
		if(!(expr))					\
			{ return E_UNEXPECTED; }

#define ExpectedPtr(ptr)		\
		_ASSERTE(ptr != NULL);			\
		if(ptr == NULL)					\
			{ return E_UNEXPECTED; }

#define InitParam(param)				\
		if(param != NULL)				\
			{ *param = NULL; }

#define IfNullRet(param) 				\
		_ASSERTE(param != NULL);		\
		if(param == NULL)				\
			{ return E_INVALIDARG; }

#define IfNullGo(param) 				\
		_ASSERTE(param != NULL);		\
		if(param == NULL)				\
			{ goto ONERROR; }

#define IfFailGo(hr)							\
		_ASSERTE(SUCCEEDED(hr));				\
		if(FAILED(hr))							\
			goto ONERROR;

#define _IfFailGo(hr)							\
		if(FAILED(hr))							\
			goto ONERROR;

#define IfFailRet(hr)						\
		_ASSERTE(SUCCEEDED(hr));		\
		if(FAILED(hr)) 					\
			{ return hr; }

#define IfNullPtrGo(ptr)							\
		_ASSERTE(ptr != NULL);					\
		if(ptr == NULL)							\
		{ hr = E_POINTER; goto ONERROR;}

#define AtlCreateInstance(ClassName, iid, ppUnk) \
	{ \
		CComObject<ClassName> *pObject = NULL; \
		if(SUCCEEDED(CComObject<ClassName>::CreateInstance(&pObject)) && \
			pObject != NULL) \
		{ \
			if(FAILED(pObject->GetUnknown()->QueryInterface(iid, (void**) ppUnk))) \
			{ \
				*ppUnk = NULL; \
			} \
		} \
	}

#define dimensionof(a)  (sizeof(a)/sizeof(*(a)))


//////////////////////////////////////////////////////////////////////////////
//
// DHTMLEd ProtocolInfo class
//

class ATL_NO_VTABLE CDHTMLEdProtocolInfo :
	public CComObjectRootEx<CComMultiThreadModel>,
	public IClassFactory,
	public IInternetProtocolInfo,
	public IProtocolInfoConnector
{
public:

//DECLARE_POLY_AGGREGATABLE(CDHTMLEdProtocolInfo)
//DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CDHTMLEdProtocolInfo)
	COM_INTERFACE_ENTRY(IClassFactory)
	COM_INTERFACE_ENTRY(IInternetProtocolInfo)
	COM_INTERFACE_ENTRY(IProtocolInfoConnector)
END_COM_MAP()

//
//  IClassFactory methods
//
    STDMETHODIMP CreateInstance(IUnknown * pUnkOuter, REFIID riid, void **ppvObject);
    STDMETHODIMP RemoteCreateInstance( REFIID riid, IUnknown ** ppvObject);
    STDMETHODIMP LockServer(BOOL fLock);
    STDMETHODIMP RemoteLockServer(BOOL fLock);
//
//  IInternetProtocolInfo methods
//
    STDMETHODIMP CombineUrl(LPCWSTR     pwzBaseUrl,
                            LPCWSTR     pwzRelativeUrl,
                            DWORD       dwFlags,
                            LPWSTR      pwzResult,
                            DWORD       cchResult,
                            DWORD *     pcchResult,
                            DWORD       dwReserved);
    STDMETHODIMP CompareUrl(LPCWSTR     pwzUrl1,
                            LPCWSTR     pwzUrl2,
                            DWORD       dwFlags);
    STDMETHODIMP ParseUrl(LPCWSTR     pwzUrl,
                          PARSEACTION ParseAction,
                          DWORD       dwFlags,
                          LPWSTR      pwzResult,
                          DWORD       cchResult,
                          DWORD *     pcchResult,
                          DWORD       dwReserved);
    STDMETHODIMP QueryInfo(LPCWSTR         pwzUrl,
                           QUERYOPTION     QueryOption,
                           DWORD           dwQueryFlags,
                           LPVOID          pBuffer,
                           DWORD           cbBuffer,
                           DWORD *         pcbBuf,
                           DWORD           dwReserved);

	// IProtocolInfoConnector methods
	STDMETHODIMP SetProxyFrame ( SIZE_T* vpProxyFrame );
//
//  Data members
//
private:
	BOOL					m_fZombied:1;
	CProxyFrame*			m_pProxyFrame;
	IProtocolInfoConnector*	m_piProtocolConIntf;

//
//  constructor
//
public:
	CDHTMLEdProtocolInfo();
	~CDHTMLEdProtocolInfo();
	void Zombie();

#if defined _DEBUG_ADDREF_RELEASE
public:
	ULONG InternalAddRef()
	{
		ATLTRACE(_T("CDHTMLEdProtocolInfo Ref %d>\n"), m_dwRef+1);
		_ASSERTE(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
		ATLTRACE(_T("CDHTMLEdProtocolInfo Ref %d<\n"), m_dwRef-1);
		return _ThreadModel::Decrement(&m_dwRef);
	}
#endif

};

//////////////////////////////////////////////////////////////////////////////
//
// DHTMLEd Protocol class
//

class ATL_NO_VTABLE CDHTMLEdProtocol :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDHTMLEdProtocol, &CLSID_DHTMLEdProtocol>,
	public IInternetProtocol,
	public IProtocolInfoConnector
{
public:

DECLARE_POLY_AGGREGATABLE(CDHTMLEdProtocol)
DECLARE_GET_CONTROLLING_UNKNOWN()

BEGIN_COM_MAP(CDHTMLEdProtocol)
	COM_INTERFACE_ENTRY(IInternetProtocol)
	COM_INTERFACE_ENTRY(IProtocolInfoConnector)
END_COM_MAP()

//
//  IInternetProtocol methods
//
    STDMETHOD(LockRequest)(DWORD dwOptions);
    STDMETHOD(Read)(void *pv,ULONG cb,ULONG *pcbRead);
    STDMETHOD(Seek)(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER *plibNewPosition);
    STDMETHOD(UnlockRequest)();

//
//  IInternetProtocolRoot methods
//
    STDMETHOD(Abort)(HRESULT hrReason,DWORD dwOptions);
    STDMETHOD(Continue)(PROTOCOLDATA *pStateInfo);
    STDMETHOD(Resume)();
    STDMETHOD(Start)(LPCWSTR szUrl, IInternetProtocolSink *pProtSink, IInternetBindInfo *pOIBindInfo, DWORD grfSTI, HANDLE_PTR dwReserved);
    STDMETHOD(Suspend)();
    STDMETHOD(Terminate)(DWORD dwOptions);

	// IProtocolInfoConnector methods
	STDMETHODIMP SetProxyFrame ( SIZE_T* vpProxyFrame );
//
//  Data members
//
private:
    CComPtr<IInternetProtocolSink> 	m_srpSink; 			// The protocol sink
    CComPtr<IInternetBindInfo>     	m_srpBindInfo; 		// The Bind info
	CComPtr<IStream>				m_srpStream;		// Buffer Stream

	CComBSTR		m_bstrBaseURL;			// BaseURL of buffer
    DWORD 			m_bscf;
	DWORD			m_grfBindF;
	DWORD			m_grfSTI;
	BINDINFO		m_bindinfo;
	BOOL			m_fAborted:1;
	BOOL 			m_fZombied:1;
	CProxyFrame*	m_pProxyFrame;

//
//  constructor
//
public:
	CDHTMLEdProtocol();
	~CDHTMLEdProtocol();
	void Zombie();

//
//  Method members
//
private:
	HRESULT ParseAndBind();
	void 	ReportData(ULONG cb);

#if defined _DEBUG_ADDREF_RELEASE
public:
	ULONG InternalAddRef()
	{
		ATLTRACE(_T("CDHTMLEdProtocol Ref %d>\n"), m_dwRef+1);
		_ASSERTE(m_dwRef != -1L);
		return _ThreadModel::Increment(&m_dwRef);
	}
	ULONG InternalRelease()
	{
		ATLTRACE(_T("CDHTMLEdProtocol Ref %d<\n"), m_dwRef-1);
		return _ThreadModel::Decrement(&m_dwRef);
	}
#endif
};

#endif __INC_PLGPRO_H__

/* end of file*/
