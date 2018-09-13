/*
 * @(#)Peer.cxx 1.0 2/27/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * implementation of xtag peer for XML data islands
 * 
 */


#include "core.hxx"
#pragma hdrstop

#include "peer.hxx"
#include "xmldom.hxx"
#include "core/com/bstr.hxx"
#include "islandshared.hxx"

// BUGBUG remove when not needed
#if DBG == 1
#include "core/io/stringbufferoutputstream.hxx"
#endif


//379E501F-B231-11d1-ADC1-00805FC752D8
extern "C" const CLSID CLSID_XMLIslandPeer = 
{0x379e501f,0xb231,0x11d1,{0xad,0xc1,0x00,0x80,0x5f,0xc7,0x52,0xd8}};

CXMLIslandPeer::EVENTINFO CXMLIslandPeer::s_eventinfoTable[] =
{
    { L"ondataavailable", PEEREVENT_ONDATAAVAILABLE }
};

CXMLIslandPeer::CXMLIslandPeer(DSODocument* pDoc)
{
    _pPropChange = NULL;
    _pDocWrapper = new DOMDocumentWrapper(pDoc);
    _pDocWrapper->Release(); // smartpointer.
    _dwAdviseCookie = 0;
    _pDocEvents = NULL;
    _pEBOM = NULL;
    _pCPNodeList = NULL;
    _pDSO = NULL;
}

CXMLIslandPeer::~CXMLIslandPeer()
{
    // Unadvise on the ConnectionPoint
    HookUpToConnection(FALSE);
    // Let go of the event helper
    if (_pDocEvents)
        _pDocEvents->Release();

    // Detach from the event which notifies us of the src attribute changing
    if (_pPropChange)
    {
        AttachEventHelper(_HTMLElement, _pPropChange, L"onpropertychange", FALSE);
        _pPropChange->Release();
    }

    _PeerSite = null;
    _pDocWrapper = null;
    
    if (_pEBOM)
        _pEBOM->Release();

    ReleaseCPNODEList(_pCPNodeList);
}

///////////////////////////////////////////////////////////////////////////////
// CXMLIslandPeer::QueryInterface - over-ride QI
//
HRESULT 
STDMETHODCALLTYPE 
CXMLIslandPeer::QueryInterface(REFIID riid, LPVOID *ppvObject)
{
    HRESULT hr = S_OK;
    STACK_ENTRY_IUNKNOWN(this);

    if (ppvObject == NULL)
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    *ppvObject = NULL;

    TRY
    {
        hr = super::QueryInterface(riid,ppvObject);
        if (hr != E_NOINTERFACE)
            goto Cleanup;

        if (IsEqualIID(riid, IID_IDispatch) ||
            IsEqualIID(riid, IID_IDispatchEx))
        {
            *ppvObject = static_cast<IDispatchEx *>(this);
            AddRef();

        }
        else if (IsEqualIID(riid, IID_IConnectionPointContainer))
        {
            // BUGBUG - is this all still needed now that CXMLIslandPeer
            // is it's own controlling IUnknown ?
            CXMLConnectionPtContainer *pCPC = new_ne CXMLConnectionPtContainer(
                IID_IPropertyNotifySink, 
                static_cast<IElementBehavior *>(this),  // set this object as the pUnkOuter
                &_pCPNodeList, 
                getWrapped()->getSpinLockCPC());

            if (NULL != pCPC)
            {
                *ppvObject = (LPVOID) pCPC;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }
        else if (IsEqualIID(riid, IID_DataSource))
        {
            if (!_pDSO)
            {
                _pDSO = new XMLDSO;  // is created with refcount = 0
                if (!_pDSO)
                {
                    hr = E_OUTOFMEMORY;
                    goto Cleanup;
                }
            
                _pDSO->GetAttributesFromElement(_HTMLElement);
                if (_PeerSite)
                {
                    _pDSO->setSite(_PeerSite); 
                    _pDSO->setInterfaceSafetyOptions(IID_IUnknown, INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA);
                }
                _pDSO->AddDocument(getWrapped());
            }

            Assert(!!_pDSO);
            hr = _pDSO->QueryInterface(riid, ppvObject);
        }
        else
        {
            hr = _pDocWrapper->QueryInterface(riid, ppvObject);
        }
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY

Cleanup:
    return hr;
}

HRESULT STDMETHODCALLTYPE 
CXMLIslandPeer::GetDispID( 
    /* [in] */ BSTR bstrName,
    /* [in] */ DWORD grfdex,
    /* [out] */ DISPID __RPC_FAR *pid)
{
    HRESULT hr;

    // bug 64435 - if this is a XML island peer document, then we need to return
    // DISP_E_UNKNOWNNAME if bstrName = "readyState" so that the peer readystate
    // is returned by Trident as a string instead of as a numeric value so that
    // out peer is consistent with all the other HTML elements.
    static const WCHAR* pszReadyState = L"readyState";
    if (::StrCmpW(bstrName,pszReadyState) == 0) 
    {
        return DISP_E_UNKNOWNNAME;
    }
    hr = getDispatch()->GetDispID(bstrName, grfdex, pid);
    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  CXMLIslandPeer::Init
//  method of IElementBehavior interface
//
STDMETHODIMP
CXMLIslandPeer::Init(
    IElementBehaviorSite *pPeerSite)
{
    STACK_ENTRY_IUNKNOWN(this);
    HRESULT hr = S_OK;

    TRY
    {
        _PeerSite = pPeerSite; // _reference::operator=

        if (SUCCEEDED(hr = _PeerSite->GetElement(&_HTMLElement)))
        {
            // Set the security information.  (setSite now works correctly in this case)
            getWrapped()->setSite(_PeerSite); 
            getWrapped()->setInterfaceSafetyOptions(IID_IUnknown, INTERFACESAFE_FOR_UNTRUSTED_DATA, INTERFACESAFE_FOR_UNTRUSTED_DATA);

            hr = RegisterBehaviourEvents();
            
        }
        else
        {
            goto Exit;
        }

        if (SUCCEEDED(hr))
        {
            hr = HookUpToConnection(TRUE);
        }
    }
    CATCH
    {
        // BUGBUG something besides E_FAIL?
        hr = E_FAIL;
    }
    ENDTRY


Exit:
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CXMLIslandPeer::Notify
//  method of IElementBehavior interface
//
STDMETHODIMP
CXMLIslandPeer::Notify(
    LONG event, 
    VARIANT * pVar)
{
    HRESULT hr = S_OK;

    Assert(_PeerSite != null);
    Assert(_HTMLElement != null);

    switch (event)
    {

        // BEHAVIOREVENT_CONTENTREADY is sent immediately after the peer's end tag is parsed
        // and at any subsequent time that the content of the tag is modified
        case BEHAVIOREVENT_CONTENTREADY:
        {
            hr = InitXMLIslandPeer();
            break;
        }

        // BEHAVIOREVENT_DOCUMENTREADY is sent when the entire document has been parsed
        // BUGBUG do we need to handle this event or not?
        case BEHAVIOREVENT_DOCUMENTREADY:
        {
            break;
        }

    }

    return hr;
}


///////////////////////////////////////////////////////////////////////////////
//  CXMLIslandPeer::GetParsedText
//

HRESULT
CXMLIslandPeer::GetParsedText(BSTR *pbstrParsedText)
{
    const static GUID SID_SElementBehaviorMisc = {0x3050f632,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
    const static GUID CGID_ElementBehaviorMisc = {0x3050f633,0x98b5,0x11cf, {0xbb,0x82,0x00,0xaa,0x00,0xbd,0xce,0x0b}};
    const static int CMDID_ELEMENTBEHAVIORMISC_PARSEDTEXT = 1;

    HRESULT hr;
    IServiceProvider *pIServiceProvider = NULL;
    IOleCommandTarget *pIOleCommandTarget = NULL;
    VARIANT varParsedText;
    VariantInit(&varParsedText);

    Assert (pbstrParsedText);

    hr = _PeerSite->QueryInterface(IID_IServiceProvider, (void**)&pIServiceProvider);
    if (FAILED(hr))
        goto Exit;

    hr = pIServiceProvider->QueryService(SID_SElementBehaviorMisc, IID_IOleCommandTarget, (void**)&pIOleCommandTarget);
    if (FAILED(hr))
        goto Exit;

    hr = pIOleCommandTarget->Exec(
            &CGID_ElementBehaviorMisc, CMDID_ELEMENTBEHAVIORMISC_PARSEDTEXT, 0, NULL, &varParsedText);
    if (hr)
        goto Exit;

    if (V_VT(&varParsedText) != VT_BSTR)
    {
        VariantClear(&varParsedText);
        hr = E_UNEXPECTED;
        goto Exit;
    }

    *pbstrParsedText = V_BSTR(&varParsedText);

Exit:
    release(&pIServiceProvider);
    release(&pIOleCommandTarget);
    // do not do VariantClear(&varParsedText) since we are returning the BSTR
    return hr;
}

///////////////////////////////////////////////////////////////////////////////
//  CXMLIslandPeer::InitXMLIslandPeer
//
//  initializes CXMLIslandPeer object
//
HRESULT
CXMLIslandPeer::InitXMLIslandPeer()
{
    HRESULT hr;
    STACK_ENTRY_IUNKNOWN(this);
    BSTR bstrXML = null;
    VARIANT vtSrcAttr;
    IStream *pIStream = NULL;
    BSTR bstrSRC = ::SysAllocString(L"SRC");
    VARIANT varExpandoValue;
    VARIANT_BOOL result;

    TRY
    {

        // First attach our expando to the tag
        // This is the VARIANT we are going to attach        
        varExpandoValue.pdispVal = static_cast<IDispatchEx *>(_pDocWrapper);
        varExpandoValue.vt = VT_DISPATCH;
    
        hr = SetExpandoProperty(
            _HTMLElement, 
            String::newString("XMLDocument"),
            &varExpandoValue);

        Assert(SUCCEEDED(hr) && "Failed to add expando property !");

        // VariantClear(&varExpandoValue); -- not needed since we didn't do an addref.

        if (NULL == bstrSRC && SUCCEEDED(hr))
        {
            hr = E_OUTOFMEMORY;
        }

        // Get Trident to tell us when the SRC attribute changes
        if (SUCCEEDED(hr))
            hr = RegisterForElementEvents(_HTMLElement, &_pPropChange, (CElementEventSink *)this);

        if (FAILED(hr))
        {
            goto Exit;
        }

        VariantInit(&vtSrcAttr);

        hr = _HTMLElement->getAttribute(bstrSRC, 0, &vtSrcAttr);
        ::SysFreeString(bstrSRC);
    
        // If we don't have a SRC attribute, we need to get what's between the tags
        if (FAILED(hr) || (vtSrcAttr.vt != VT_BSTR) || (V_BSTR(&vtSrcAttr) == NULL))
        {
            // BUGBUG - this get_innerHTML call can be removed when
            // MSHTML fixes their side.
            hr = _HTMLElement->get_innerHTML(&bstrXML);
        
            if (NULL == bstrXML || 0 == *bstrXML)
            {
                // try the new method
                hr = GetParsedText(&bstrXML);
            }
            if (NULL == bstrXML)
                hr = E_FAIL;
        
            if (FAILED(hr))
                goto Exit;
        }

        // We had better have either a SRC attribute or some XML
        Assert (bstrXML != NULL || V_BSTR(&vtSrcAttr) != NULL && "Must have either a SRC attribute or some XML");

        // Do we have a SRC attribute ?
        if (V_VT(&vtSrcAttr) == VT_BSTR && V_BSTR(&vtSrcAttr) != NULL)
        {
            // This relative URL will be resolved using the baseURL we provided on the 
            // document in Init.
            // Have to call the wrapper instead of the document directly
            // because it's the wrapper that handles dom locks
            hr = _pDocWrapper->load(vtSrcAttr, &result);
        }
        else if (bstrXML != NULL)
        {
            // Have to call the wrapper instead of the document directly
            // because it's the wrapper that handles dom locks
            hr = _pDocWrapper->loadXML(bstrXML, &result);
        }


#ifdef LETSLEAVETHISOUT
#if DBG == 1
        // debug code to enable looking at parsed doc tree in debugger
        StringBuffer * buf = StringBuffer::newStringBuffer(4096);
        Stream * pStreamOut = StringStream::newStringStream(buf);
        pStreamOut->getIStream(&pIStream);
        OutputHelper * pOutputHelper = getWrapped()->createOutput(pIStream);
        getWrapped()->save(pOutputHelper);
        String * pStringOut = buf->toString();

        // BUGBUG add debug code to dump document
        //FileOutputStream* out = FileOutputStream::newFileOutputStream(String::newString(_T("dumpTree.xml")));
        //dumpTree(getWrapped(), PrintStream::newPrintStream(out, true), String::emptyString());
#endif
#endif // LETSLEAVETHISOUT

    }
    CATCH
    {
        // BUGBUG something besides E_FAIL?
        hr = E_FAIL;
        goto Exit;
    }
    ENDTRY


Exit:
    VariantClear(&vtSrcAttr);
    ::SysFreeString(bstrXML);
    release(&pIStream);
    return hr;
}

HRESULT 
CXMLIslandPeer::HookUpToConnection(BOOL fAdvise)
{
    HRESULT hr;
    IConnectionPointContainer *pCPC;
    IConnectionPoint *pCP;

    if (SUCCEEDED(hr = getWrapped()->QueryInterface(IID_IConnectionPointContainer, (LPVOID *)&pCPC)))
    {
        hr = pCPC->FindConnectionPoint(DIID_XMLDOMDocumentEvents, &pCP);
        pCPC->Release(); // Don't need you any more
        
        if (SUCCEEDED(hr))
        {
            if (fAdvise)
            {
                if (!_pDocEvents)
                {
                    _pDocEvents = new_ne CDocEventReceiver((CDocEventSink *)this);
                    Assert(_pDocEvents && "Unable to create doc event handler");

                    if (!_pDocEvents)
                        return E_OUTOFMEMORY;

                }
                hr = pCP->Advise((IUnknown *)_pDocEvents, &_dwAdviseCookie);

            }
            else
            {
                Assert(_dwAdviseCookie && "Unadvising on a bad cookie");
                hr = pCP->Unadvise(_dwAdviseCookie);
#if DBG == 1
                _dwAdviseCookie = 0;
#endif
            }
            pCP->Release();
        }
    }
    return hr;
}


HRESULT 
CXMLIslandPeer::RegisterBehaviourEvents()
{
    UINT i;
    HRESULT hr;
    EVENTINFO *pEventWalker;

    Assert (_PeerSite && "We don't have a Peer site to register events against!");
    Assert (sizeof(s_eventinfoTable) / sizeof(EVENTINFO) == PEEREVENT_NUMEVENTS && "PEEREVENT_NUMEVENTS is out of sync");

    if (SUCCEEDED(hr = _PeerSite->QueryInterface(IID_IElementBehaviorSiteOM, (LPVOID *)&_pEBOM)))
    {
        for (i = 0; i < PEEREVENT_NUMEVENTS && SUCCEEDED(hr); i++)
        {
            pEventWalker = &s_eventinfoTable[i];
            BSTR bstrEventname = SysAllocString(pEventWalker->rgwchEventName);
            
            if (NULL != bstrEventname)
            {
                hr = _pEBOM->RegisterEvent( 
                        bstrEventname,
                        0, 
                        &_rglEventCookies[pEventWalker->lCookieNumber]);

                SysFreeString(bstrEventname);
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }

            Assert (SUCCEEDED(hr) && "Unexpected problem while registering events");
        }
    }

    Assert (_pEBOM && "Couldn't get an ElementBehaviourOM pointer - no events for you my lad !");
    
    return hr;
}

HRESULT 
CXMLIslandPeer::onReadyStateChange()
{
// BUGBUG - commenting out this line fixes bug 66769, but this means the 
// loop is not safe to changes in the list that may happen during OnChanged.  
// We should eventually copy the ppnsConnector references into a local
// array while inside BusyLock then release the BusyLock and zip through the 
// array and call OnChanged.
//    BusyLock bl(getWrapped()->getSpinLockCPC()); // bug 66769

    PCPNODE pNode = _pCPNodeList;
    HRESULT hr = S_OK;
    
    while (pNode && SUCCEEDED(hr))
    {
        Assert (pNode->cpt != CP_Invalid && "Invalid ConnectionPoint node");

        // We only fire through IPropertyNotifySink
        // If we couldn't get IPropertyNotifySink, it's useless to us anyway
        if (CP_PropertyNotifySink == pNode->cpt)
        {
            hr = pNode->ppnsConnector->OnChanged(DISPID_READYSTATE);
        }
        pNode = pNode->pNext;
    }

    return hr;
}

HRESULT 
CXMLIslandPeer::onDataAvailable()
{
    Assert (_pEBOM && "We don't have an EBOM pointer !");

    if (!_pEBOM)
        return S_OK;

    return _pEBOM->FireEvent(_rglEventCookies[PEEREVENT_ONDATAAVAILABLE], NULL);
}


HRESULT 
CXMLIslandPeer::onSrcChange()
{
    STACK_ENTRY_IUNKNOWN(this);

    VARIANT varAttrib;
    VARIANT varAttribBstr;
    BSTR bstrSrc;
    HRESULT hr;

    VariantInit(&varAttrib);
    VariantInit(&varAttribBstr);
    
    hr = GetExpandoProperty(
        _HTMLElement, 
        String::newString("src"),
        &varAttrib);
    
    if (SUCCEEDED(hr))
    {
        if (varAttrib.vt != VT_BSTR)
        {
            if (SUCCEEDED(hr = VariantChangeTypeEx(&varAttribBstr, &varAttrib, GetThreadLocale(), 0, VT_BSTR)))
            {
                bstrSrc = varAttribBstr.bstrVal;
            }
            else
            {
                bstrSrc = NULL;
            }
        }
        else
        {
            bstrSrc = varAttrib.bstrVal;
        }
    }        
    TRY
    {
        if (SUCCEEDED(hr))
        {
            // Let's re-use all the great IXMLDOMDocument wrapper stuff so we
            // get the right locking behavior.
            VARIANT_BOOL result;
            VARIANT vURL;
            vURL.vt = VT_BSTR;
            V_BSTR(&vURL) = bstrSrc;
            hr = _pDocWrapper->load(vURL, &result);
        }
    }
    CATCH
    {
        hr = ERESULT;
    }
    ENDTRY
    
    VariantClear(&varAttrib);
    
    if (varAttribBstr.vt != VT_EMPTY)
        VariantClear(&varAttribBstr);
    
    return hr;
}

/// End of file ///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

