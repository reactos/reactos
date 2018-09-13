/*
 * @(#)events.cxx 1.0 7/13/98
 * 
* Copyright (c) 1997 - 1999 Microsoft Corporation. All rights reserved. *
 * IDispatch object for receiving events from the tree
 * 
 */


#include "core.hxx"
#pragma hdrstop

#include <xmldomdid.h>
#include "events.hxx"

CEventReceiverBase::CEventReceiverBase(REFIID iid) : _iid(iid), _cRef(1)
{
}


CEventReceiverBase::~CEventReceiverBase()
{
}

STDMETHODIMP 
CEventReceiverBase::QueryInterface(
    REFIID riid, 
    void **ppv)
{
    if (NULL == ppv)
        return E_POINTER;
    
    *ppv = NULL;
    
    if ( IID_IDispatch == riid || _iid == riid )
    {
        *ppv = this;
    }

    else if ( IID_IUnknown == riid )
    {
        *ppv = (IUnknown*)(IDispatch*)this;
    }
    
    if (*ppv)
    {
        AddRef();
        return S_OK;
    }
    else
    {
        return E_NOINTERFACE;
    }
}


STDMETHODIMP_(ULONG) 
CEventReceiverBase::AddRef()
{
    return _cRef++;
}
    
STDMETHODIMP_(ULONG) 
CEventReceiverBase::Release()
{
    _cRef--;
    
    if (0 == _cRef)
    {
        delete this;
        return 0;
    }
    
    return _cRef;
}


STDMETHODIMP 
CDocEventReceiver::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS  *pDispParams,
    VARIANT  *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    Assert(_pSink && "Attempting to fire events to a NULL Sink");

    HRESULT hr;

    switch (dispIdMember)
    {
    case DISPID_XMLDOMEVENT_ONREADYSTATECHANGE:
        hr = _pSink->onReadyStateChange();
        break;

    case DISPID_XMLDOMEVENT_ONDATAAVAILABLE:
        hr = _pSink->onDataAvailable();
        break;

    default:
        Assert(FALSE && "Unhandled event - CEventReceiver and CEventSink need to be updated !");
        hr = DISP_E_MEMBERNOTFOUND;
        break;
    }

    return hr;
}

CPropChangeReceiver::CPropChangeReceiver(CElementEventSink *pSink, IHTMLWindow2 *pHTMLWindow) :
    CEventReceiver<CElementEventSink>(IID_IUnknown, pSink)
{
    _pHTMLWindow = pHTMLWindow;
    
    if (pHTMLWindow)
        pHTMLWindow->AddRef();
}

CPropChangeReceiver::~CPropChangeReceiver()
{
    if (_pHTMLWindow)
        _pHTMLWindow->Release();
}



STDMETHODIMP 
CPropChangeReceiver::Invoke(
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS  *pDispParams,
    VARIANT  *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr)
{
    Assert(_pSink && "Attempting to fire events to a NULL Sink");

    HRESULT hr;
    IHTMLEventObj *pEvent;

    hr = _pHTMLWindow->get_event(&pEvent);

    if (SUCCEEDED(hr))
    {
        Assert (pEvent && "NULL pEvent despite SUCCESS from Trident");
        // Get the attribute
        IHTMLEventObj2 *pEvent2;

        hr = pEvent->QueryInterface(IID_IHTMLEventObj2, (LPVOID *)&pEvent2);

        if (SUCCEEDED(hr))
        {
            BSTR bstrPropName = NULL;
            hr = pEvent2->get_propertyName(&bstrPropName);
            pEvent2->Release();

            if (SUCCEEDED(hr))
            {
                // StrCmpNIW - included in IE 4 shlwapi
                if (0 == StrCmpNIW(bstrPropName, L"src", 3))
                {
                    // Yep, SRC is changing
                    hr = _pSink->onSrcChange();

                }
            }
            
            SysFreeString(bstrPropName);
        }
        pEvent->Release();

    }

    return hr;
}


HRESULT 
AttachEventHelper(
    IHTMLElement *pElement,
    IDispatch *pdispHandler,
    LPWSTR pwszEventName,
    BOOL fAttach)
{
    Assert (pdispHandler && "Must pass in valid IDispatch");

    HRESULT hr;
    IHTMLElement2 *pElem2;

    VARIANT_BOOL vbSuccess;

    if (SUCCEEDED(hr = pElement->QueryInterface(IID_IHTMLElement2, (LPVOID *)&pElem2)))
    {
        BSTR bstrEvent = SysAllocString(pwszEventName);
        Assert(bstrEvent && "Failed to allocate BSTR");
    
        if (fAttach)
        {
            hr = pElem2->attachEvent(bstrEvent, pdispHandler, &vbSuccess);
            if (SUCCEEDED(hr) && VARIANT_FALSE == vbSuccess)
                hr = E_FAIL;
        }
        else
        {
            hr = pElem2->detachEvent(bstrEvent, pdispHandler);
        }
    
    
        SysFreeString(bstrEvent);
        pElem2->Release();
    }

    return hr;
}


HRESULT 
RegisterForElementEvents(
    IHTMLElement *pElem, 
    CPropChangeReceiver **ppEventDispObj, 
    CElementEventSink *pEventSink)
{
    Assert(pElem && "Must provide a valid element pointer");
    Assert(ppEventDispObj && "Must provide a valid pointer");
    Assert(pEventSink && "Must provide a valid pointer");

    *ppEventDispObj = NULL;

    IHTMLDocument2 *pDoc;
    IHTMLWindow2 *pWindow;
    IDispatch *pdisp;
    HRESULT hr;
    
    hr = pElem->get_document(&pdisp);
    if (SUCCEEDED(hr))
    {
        Assert (pdisp && "NULL IDispatch despite SUCCESS from Trident");
        hr = pdisp->QueryInterface(IID_IHTMLDocument2, (LPVOID *)&pDoc);
        pdisp->Release();
        
        if (SUCCEEDED(hr))
        {
            Assert (pDoc && "NULL pDoc despite SUCCESS from Trident");

            hr = pDoc->get_parentWindow(&pWindow);
            pDoc->Release();
            
            if (SUCCEEDED(hr))
            {
                Assert (pWindow && "NULL pWindow despite SUCCESS from Trident");

                *ppEventDispObj = new_ne CPropChangeReceiver(pEventSink, pWindow);
                pWindow->Release();
            }
        }
    }
    
    // Preserve the HRESULT in case something failed above
    if (!(*ppEventDispObj) )
        return E_OUTOFMEMORY;

    return AttachEventHelper(pElem,(IDispatch *)*ppEventDispObj, L"onpropertychange", TRUE);
}

