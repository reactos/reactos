//+----------------------------------------------------------------------------
//
//  HTML property pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:      siteobj.cpp
//
//  Contents:  CSite implementation
//
//  History:   22-Jan-97 EricB      Created.
//
//-----------------------------------------------------------------------------
#include "pch.h"
#include "SiteObj.h"
#pragma hdrstop

//+----------------------------------------------------------------------------
//
//  Method:     CSite::CSite
//
//  Synopsis:  constructor
//
//-----------------------------------------------------------------------------
CSite::CSite(HWND hWnd, LPPROPVIEW pView) :
    m_cRef(0),
    m_pObj(NULL),
    m_pIOleObject(NULL),
    m_pIOleIPObject(NULL),
    m_pIOleDocView(NULL),
	m_pIOleCommandTarget(NULL),
    m_pClientSite(NULL),
    m_pAdviseSink(NULL),
    m_IPSite(NULL),
    m_pDocSite(NULL),
	m_pDocHostUIHandler(NULL),
	m_pDocHostShowUi(NULL),
    m_pTypeInfo(NULL),
    m_pEventSink(NULL),
    m_cCookieArrayElements(0)
{
    m_hWnd = hWnd;
    m_pView = pView;

#ifdef _DEBUG
    strcpy(szClass, "CSite");
#endif
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::~CSite
//
//  Synopsis:  destructor
//
//-----------------------------------------------------------------------------
CSite::~CSite()
{
    ASSERT(m_cRef == 0);

    // Object pointers cleaned up in Close.

    // We delete our own interfaces since we control them
    DeleteInterfaceImp(m_pDocSite);
    DeleteInterfaceImp(m_IPSite);
    DeleteInterfaceImp(m_pAdviseSink);
    DeleteInterfaceImp(m_pClientSite);
	DeleteInterfaceImp(m_pDocHostUIHandler);
	DeleteInterfaceImp(m_pDocHostShowUi);
    DeleteInterfaceImp(m_pEventSink);
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::IUnknown::QueryInterface
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSite::QueryInterface(REFIID riid, void **ppv)
{
    *ppv = NULL;

    if (IID_IServiceProvider == riid)
	{
        *ppv = this;
	}

    if (IID_IOleClientSite == riid)
	{
        *ppv = m_pClientSite;
	}

    if (IID_IAdviseSink == riid)
	{
        *ppv = m_pAdviseSink;
	}

    if (IID_IOleWindow == riid || IID_IOleInPlaceSite == riid)
	{
        *ppv = m_IPSite;
	}

    if (IID_IOleDocumentSite == riid)
	{
        *ppv = m_pDocSite;
	}

    if (IID_IDocHostUIHandler == riid)
    {
        *ppv = m_pDocHostUIHandler;
    }

    if (IID_IDocHostShowUI == riid)
    {
        *ppv = m_pDocHostShowUi;
    }

    if (NULL != *ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

	// Try the frame instead
	return m_pView->QueryInterface(riid, ppv);
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::IUnknown::AddRef
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CSite::AddRef(void)
{
    return ++m_cRef;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::IUnknown::Release
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CSite::Release(void)
{
    ASSERT(m_cRef > 0);

    if (0 != --m_cRef)
    {
        return m_cRef;
    }

    delete this;

    return 0;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::IServiceProvider::QueryService
//
//  BUGBUG: is this still needed????
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSite::QueryService(REFGUID sid, REFIID iid, LPVOID * ppv)
{
    HRESULT hr = S_OK;

    *ppv = NULL;
    hr = E_NOINTERFACE;

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::Create
//
//  Synopsis:  Asks the site to instantiate the Trident object.
//
//-----------------------------------------------------------------------------
BOOL
CSite::Create(LPCTSTR pszUrl)
{
    HRESULT   hr;

    // Create the site's interface implementations which Trident will call
    m_pClientSite = new CClientSite(this);
    m_pAdviseSink = new CAdviseSink(this);
    m_IPSite = new CInPlaceSite(this);
    m_pDocSite = new CDocSite(this);
    m_pDocHostUIHandler = new CDocHostUiHandler(this);
    m_pDocHostShowUi = new CDosHostShowUi(this);
    m_pEventSink = new CInputEventSink(this);

    if (NULL == m_pClientSite || NULL == m_pAdviseSink ||
        NULL == m_IPSite || NULL == m_pDocSite  ||
        NULL == m_pDocHostUIHandler || m_pEventSink == NULL)
    {
        TRACE(TEXT("CSite::Create: sub-object allocation failed.\n"));
        return FALSE;
    }

    // Create Trident
    //
    hr = CoCreateInstance(CLSID_HTMLDocument, NULL, CLSCTX_INPROC_SERVER,
                          IID_IUnknown, (void **)&m_pObj);

    if (FAILED(hr) || NULL == m_pObj)
    {
        TRACE(TEXT("CSite::Create: CoCreateInstance(CLSID_HTMLDocument) failed with error 0x%x"), hr);
        return FALSE;
    }

    // Initialise the object we just created. Set ClientSite, the AdviseSink,
    // etc.

    // We need an IOleObject most of the time, so get one here.
    //
    hr = m_pObj->QueryInterface(IID_IOleObject, (void **)&m_pIOleObject);
    if (FAILED(hr))
    {
        TRACE(TEXT("CSite::Create: QI for IID_IOleObject failed with error 0x%x"), hr);
        return FALSE;
    }

    // SetClientSite is critical for DocObjects

    m_pIOleObject->SetClientSite(m_pClientSite);

    DWORD dw;

    m_pIOleObject->Advise(m_pAdviseSink, &dw);

    if (FAILED(Load(pszUrl)))
    {
		return FALSE;
    }

    //if (GetTypeLib() == TRUE)
    //{
        // BUGBUG: unconditionally connecting the event sink.
        //ConnectSink();
    //}
    ConnectSink();
    
    // Put the object in the running state

    OleRun(m_pIOleObject);

    return TRUE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::Close
//
//  Synopsis:  frees alls the object pointers
//
//-----------------------------------------------------------------------------
void
CSite::Close(void)
{
    ReleaseInterface(m_pTypeInfo);
    DisconnectSink();

    //OnInPlaceDeactivate releases this pointer.
    if (NULL != m_pIOleIPObject)
	{
        m_pIOleIPObject->InPlaceDeactivate();
	}

    ReleaseInterface(m_pIOleDocView);
	ReleaseInterface(m_pIOleCommandTarget);
    ReleaseInterface(m_pObj);
    if (NULL != m_pIOleObject)
    {
        m_pIOleObject->Close(OLECLOSE_NOSAVE);
        ReleaseInterface(m_pIOleObject);
    }
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::Activate
//
//  Synopsis:  Activates a verb on the mshtml object living in the site.
//
//-----------------------------------------------------------------------------
void
CSite::Activate(LONG iVerb)
{
    DECLAREWAITCURSOR;
    RECT        rc;
            
    SetWaitCursor();

    GetClientRect(m_hWnd, &rc);
    TRACE(TEXT("%d, %d, %d, %d"), rc.left, rc.top, rc.right, rc.bottom);
    m_pIOleObject->DoVerb(iVerb, NULL, m_pClientSite, 0, m_hWnd, &rc);

    ResetWaitCursor();
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::UpdateObjectRects
//
//  Synopsis:  Informs the site that the client area window was resized and
//              that the site needs to also tell the DocObject of the resize.
//
//-----------------------------------------------------------------------------
void
CSite::UpdateObjectRects(void)
{
    if (NULL != m_pIOleDocView)
	{
		RECT    rc;
        
	    GetClientRect(m_hWnd, &rc);
        TRACE(TEXT("%d, %d, %d, %d"), rc.left, rc.top, rc.right, rc.bottom);
		m_pIOleDocView->SetRect(&rc);
	}
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::Load
//
//  Synopsis:  Loads the URL provided using IPersistMoniker or IPersistFile.
//              If no path was provided it simply does an InitNew.
//
//-----------------------------------------------------------------------------
HRESULT
CSite::Load(LPCTSTR ptszUrl)
{
    HRESULT   hr = S_OK;

    if (ptszUrl != NULL && *ptszUrl != 0)
    {
#ifdef UNICODE
        LPCWSTR pszURL = ptszUrl;
#else
        OLECHAR  szName[256];
        MultiByteToWideChar(CP_ACP, 0, ptszUrl, -1, szName, 256);
        LPCWSTR pszURL = szName;
#endif
        // Path has been provided so check should we use IPersisMoniker
        // or IPersistFile?
        //
        if (memcmp(ptszUrl, TEXT("file:"), 5 * sizeof(TCHAR)) == 0 ||
            memcmp(ptszUrl, TEXT("http:"), 5 * sizeof(TCHAR)) == 0)
        {
            // Ask the system for a URL Moniker
            IMoniker* pUrlMk;

            hr = CreateURLMoniker(NULL, (LPWSTR)pszURL, &pUrlMk);

            if (FAILED(hr))
            {
                TRACE(TEXT("Load: CreateURLMoniker failed: 0x%x\n"), hr);
                return hr;
            }

            IBindCtx * pBCtx;

            hr = CreateBindCtx(0, &pBCtx);

            if (FAILED(hr))
            {
                TRACE(TEXT("Load: CreateBindCtx failed: 0x%x\n"), hr);
                ReleaseInterface(pUrlMk);
                return hr;
            }

#define DOASYNCRONOUS
#ifdef DOASYNCRONOUS

            //
            // An IPersistMoniker load is asyncronous, which means that we
            // need to detect the document load event if we want to do post-
            // load processing. The advantage of this type of load is that the
            // document.URL property will return the actual URL of the loaded
            // page.
            //
            IPersistMoniker * pPMk;

            hr = m_pObj->QueryInterface(IID_IPersistMoniker, (void **)&pPMk);

            if (FAILED(hr))
            {
                TRACE(TEXT("Load: BindToStorage failed: 0x%x\n"), hr);
                ReleaseInterface(pBCtx);
                ReleaseInterface(pUrlMk);
                return hr;
            }

            hr = pPMk->Load(FALSE, pUrlMk, pBCtx, STGM_READ);

            ReleaseInterface(pPMk);
#else
            //
            // An IPersistStreamInit load is syncronous. The disadvantage is
            // that the document object won't know the URL of the loaded page.
            //
            IStream * pStm;

            hr = pUrlMk->BindToStorage(pBCtx, NULL, IID_IStream, (void**)&pStm);

            if (FAILED(hr))
            {
                TRACE(TEXT("Load: BindToStorage failed: 0x%x\n"), hr);
                ReleaseInterface(pBCtx);
                ReleaseInterface(pUrlMk);
                return hr;
            }

            IPersistStreamInit * pPStrmInit;

            hr = m_pObj->QueryInterface(IID_IPersistStreamInit,
                                        (void **)&pPStrmInit);

            if (FAILED(hr))
            {
                TRACE(TEXT("Load: QI on IID_IPersistStreamInit failed: 0x%x\n"),
                      hr);
                ReleaseInterface(pBCtx);
                ReleaseInterface(pUrlMk);
                ReleaseInterface(pStm);
                return hr;
            }

            hr = pPStrmInit->Load(pStm);

            ReleaseInterface(pStm);
            ReleaseInterface(pPStrmInit);
#endif
            if (FAILED(hr))
            {
                TRACE(TEXT("Load: Load failed: 0x%x\n"), hr);
            }
            ReleaseInterface(pBCtx);
            ReleaseInterface(pUrlMk);
        }
        else
        {
            IPersistFile*  pPFile;
            hr = m_pObj->QueryInterface(IID_IPersistFile, (void **) &pPFile);
            if (SUCCEEDED(hr))
            {
                // Call Load on the IPersistFile
                hr = pPFile->Load((LPWSTR)pszURL, 0);
                ReleaseInterface(pPFile);
            }
        }
    }
    else
    {
        // No path provided so just do an InitNew on the Stream
        IPersistStreamInit * pPStm;

        hr = m_pObj->QueryInterface(IID_IPersistStreamInit, (void **)&pPStm);

        if (SUCCEEDED(hr))
        {
            hr = pPStm->InitNew();
            ReleaseInterface(pPStm);
        }
    }

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::GetTypeLib
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
BOOL
CSite::GetTypeLib(void)
{
#if 0   // Currently not needed.
    HRESULT hr;
    LPTYPELIB pLib;
    WCHAR path[MAX_PATH+1024];

    ASSERT(m_pTypeInfo == NULL);
    ASSERT(m_pIOleObject != NULL);

    // BUGBUG hard-coded type lib location
    // Get type lib from exe
    VERIFY(GetModuleFileNameW(NULL, path, MAX_PATH+1024));
    //wcscat(path, L"\\4");
    TRACE(TEXT("GetTypeLib: Module name is %S\n"), path);

    hr = LoadTypeLib(path, &pLib);
    //hr = LoadRegTypeLib();

    if (FAILED(hr))
    {
        TRACE(TEXT("LoadTypeLib failed: 0x%X\n"), hr);
        return FALSE;
    }

    ASSERT(pLib != NULL);
    hr = pLib->GetTypeInfoOfGuid(DIID_HTMLInputTextEvents, &m_pTypeInfo);

    if (FAILED(hr))
    {
        TRACE(TEXT("GetTypeInfoOfGuid failed: 0x%X\n"), hr);
    }

    ReleaseInterface(pLib);

    return TRUE;
#endif
    return E_NOTIMPL;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::ConnectSink
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
BOOL
CSite::ConnectSink()
{
    ASSERT(m_pObj != NULL);     // No object to talk to
    ASSERT(m_cCookieArrayElements == 0);    // Advise already set up?

    if (m_pObj == NULL || m_cCookieArrayElements != 0)
    {
        return FALSE;
    }

#ifdef NOTYET
    LPCONNECTIONPOINTCONTAINER  pContainer;
    LPCONNECTIONPOINT pConnection;
    HRESULT hr;

    IHTMLDocument2 * pHTMLDocument;

    hr = m_pObj->QueryInterface(IID_IHTMLDocument2, (void **)&pHTMLDocument);
    if (FAILED(hr))
    {
        TRACE(TEXT("ConnectSink QI on IID_IHTMLDocument2 failed with error 0x%x\n"), hr);
        return FALSE;
    }

    IHTMLElementCollection * pElCol;

    hr = pHTMLDocument->get_all(&pElCol);

    pHTMLDocument->Release();

    if (FAILED(hr))
    {
        TRACE(TEXT("ConnectSink: get_forms failed with error 0x%x\n"), hr);
        return FALSE;
    }

    long lLength;

    hr = pElCol->get_length(&lLength);

    if (FAILED(hr))
    {
        TRACE(TEXT("ConnectSink: pElCol->get_length failed with error 0x%x\n"), hr);
        return FALSE;
    }

    TRACE(TEXT("ConnectSink: pElCol->get_length returned %d\n"), lLength);

    VARIANT varNull, varIndex, varDisp;
    VariantInit(&varNull);
    VariantInit(&varIndex);
    VariantInit(&varDisp);
    V_VT(&varNull) = VT_EMPTY;
    V_VT(&varIndex) = VT_INT;

    for (int i = 0; i < lLength; i++)
    {
        V_INT(&varIndex) = i;

        hr = pElCol->item(varIndex, varNull, &varDisp);

        if (FAILED(hr))
        {
            TRACE(TEXT("ConnectSink: pElCol->item failed with error 0x%x\n"), hr);
            pElCol->Release();
            return FALSE;
        }

        if (varDisp.pdispVal != NULL)
        {
            IHTMLElement * pElem = (IHTMLElement *)varDisp.pdispVal;
            BSTR bstrClass;

            hr = pElem->get_className(&bstrClass);
            if (FAILED(hr))
            {
                TRACE(TEXT("ConnectSink: get_className failed with error 0x%x\n"), hr);
                pElem->Release();
                pElCol->Release();
                return FALSE;
            }

#ifdef _DEBUG
            BSTR bstrTag, bstrID;
            // What is ID???
            hr = pElem->get_id(&bstrID);
            if (FAILED(hr))
            {
                TRACE(TEXT("ConnectSink: get_id failed with error 0x%x\n"), hr);
                pElem->Release();
                pElCol->Release();
                return FALSE;
            }
            hr = pElem->get_tagName(&bstrTag);
            if (FAILED(hr))
            {
                TRACE(TEXT("ConnectSink: get_tagName failed with error 0x%x\n"), hr);
                pElem->Release();
                pElCol->Release();
                return FALSE;
            }
#endif // _DEBUG

            if (bstrClass == NULL)
            {
                bstrClass = SysAllocString(L"<null>");
            }

#ifdef _DEBUG
            TRACE(TEXT("Found element %S %S with class %S\n"), bstrTag, bstrID, bstrClass);

            SysFreeString(bstrID);
            SysFreeString(bstrTag);
#endif // _DEBUG

            //
            // Compare the class attribute to see if it is an input control
            // that we want to monitor.
            // BUGBUG: move literal to a string resource.
            //
            if (wcsicmp(bstrClass, L"PageInput") == 0)
            {
                hr = pElem->QueryInterface(IID_IConnectionPointContainer,
                                           (void**)&pContainer);
                if (FAILED(hr))
                {
                    TRACE(TEXT("ConnectSink QI on ConnectionPointContainer failed: %X\n"), hr);
                    pElem->Release();
                    SysFreeString(bstrClass);
                    continue;
                }
                pElem->Release();
                ASSERT(pContainer);

                hr = pContainer->FindConnectionPoint(DIID_HTMLInputTextEvents,
                                                     &pConnection);
                if (FAILED(hr))
                {
                    TRACE(TEXT("ConnectSink FindConnectionPoint failed: %X\n"), hr);

                    // Not a text control, see which it is.
                    //
                    IEnumConnectionPoints * pEnum;
                    hr = pContainer->EnumConnectionPoints(&pEnum);
                    pContainer->Release();
                    if (FAILED(hr))
                    {
                        TRACE(TEXT("EnumConnectionPoints failed: 0x%x\n"), hr);
                        SysFreeString(bstrClass);
                        continue;
                    }
                    ULONG cFetched = 0;
                    do
                    {
                        hr = pEnum->Next(1, &pConnection, &cFetched);
                        if (cFetched > 0)
                        {
                            IID iid;
                            hr = pConnection->GetConnectionInterface(&iid);
                            if (FAILED(hr))
                            {
                                pConnection->Release();
                                pEnum->Release();
                                SysFreeString(bstrClass);
                                TRACE(TEXT("GetConnectionInterface failed: 0x%x\n"),
                                      hr);
                                continue;
                            }
#ifdef _DEBUG
                            LPOLESTR pstrIID;
                            if (SUCCEEDED(StringFromIID(iid, &pstrIID)))
                            {
                                TRACE(TEXT("Connection IID: %S\n"), pstrIID);
                                CoTaskMemFree(pstrIID);
                            }
#endif // _DEBUG
                            if (iid == DIID_HTMLTextEditEvents ||
                                iid == DIID_HTMLInputImageEvents ||
                                iid == DIID_HTMLSelectElementEvents ||
                                iid == DIID_HTMLInputTxtBaseEvents ||
                                iid == DIID_HTMLTextAreaEvents ||
                                iid == DIID_HTMLInputButtonEvents ||
                                iid == DIID_HTMLCheckboxElementEvents ||
                                iid == DIID_HTMLRadioElementEvents ||
                                iid == DIID_HTMLAreaEvents ||
                                iid == DIID_HTMLMarqueeEvents ||
                                iid == DIID_HTMLInputTxtBaseEvents)
                            {
                                pEnum->Release();
                                goto DoAdvise;
                            }
#ifdef _DEBUG
                            //
                            // These are elements that probably don't need to
                            // be monitored.
                            //
                            if (iid == DIID_HTMLLabelEvents ||
                                iid == DIID_HTMLAnchorEvents ||
                                iid == DIID_HTMLImgEvents ||
                                iid == DIID_HTMLImgBaseEvents ||
                                iid == DIID_HTMLFormElementEvents)
                            {
                                TRACE(TEXT("DispInterface ignored\n"));
                            }
#endif // _DEBUG
                            pConnection->Release();
                        }
                    } while (SUCCEEDED(hr) && cFetched > 0);
                    pEnum->Release();
                    SysFreeString(bstrClass);
                    TRACE(TEXT("FindConnection failed: 0x%x\n"), hr);
                    continue;
                }
                pContainer->Release();
DoAdvise:
                ASSERT(m_pEventSink);
                DWORD dwCookie = 0;

                hr = pConnection->Advise((LPDISPATCH)m_pEventSink, &dwCookie);

                pConnection->Release();

                if (FAILED(hr) || dwCookie == 0)
                {
                    TRACE(TEXT("ConnectSink Advise failed: %X\n"), hr);
                    SysFreeString(bstrClass);
                    continue;
                }
                TRACE(TEXT("pConnection->Advise successful, cookie is 0x%x\n"),
                      dwCookie);

                m_rgCookie[m_cCookieArrayElements++] = dwCookie;
            }
            else
            {
                pElem->Release();
            }

            SysFreeString(bstrClass);
        }
        else
        {
            TRACE(TEXT("ConnectSink: item fetch failed\n"));
        }
    }

    pElCol->Release();

    return(m_cCookieArrayElements > 0);

#endif // NOTYET

    return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSite::DisconnectSink
//
//  Synopsis:
//
//-----------------------------------------------------------------------------
BOOL
CSite::DisconnectSink()
{
    ASSERT(m_pIOleObject != NULL);

//    if (m_pIOleObject == NULL || m_cCookieArrayElements == 0)
        return FALSE;

    LPCONNECTIONPOINTCONTAINER  pContainer;
    LPCONNECTIONPOINT pConnection;
    HRESULT hr;

    hr = m_pIOleObject->QueryInterface(IID_IConnectionPointContainer,
                                       (void**)&pContainer);

    if (FAILED(hr))
    {
        TRACE(TEXT("DisconnectSink() QI on ConnectionPointContainer failed: %X\n"), hr);
        return FALSE;
    }

    ASSERT(pContainer);
    hr = pContainer->FindConnectionPoint(DIID_HTMLInputTextElementEvents,
                                         &pConnection);

    if (FAILED(hr))
    {
        ReleaseInterface(pContainer);
        TRACE(TEXT("DisconnectSink() FindConnection failed: %X\n"), hr);
        return FALSE;
    }

    // Establish the connection with our interface
    ASSERT(pConnection != NULL);

    ASSERT(m_pEventSink);
    hr = pConnection->Unadvise(m_rgCookie[--m_cCookieArrayElements]);

    if (FAILED(hr))
    {
        TRACE(TEXT("Unadvise failed: %X\n"), hr);
    }

    ReleaseInterface(pConnection);
    ReleaseInterface(pContainer);

    return (hr == S_OK);
}
