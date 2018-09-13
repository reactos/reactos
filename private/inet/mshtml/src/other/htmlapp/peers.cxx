
#include "headers.hxx"

#ifndef X_APP_HXX_
#define X_APP_HXX_
#include "app.hxx"
#endif

#ifndef X_PEERS_HXX_
#define X_PEERS_HXX_
#include "peers.hxx"
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include "coredisp.h"
#endif

#ifndef X_FUNCSIG_HXX_
#define X_FUNCSIG_HXX_
#include "funcsig.hxx"
#endif

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_DISPEX_H_
#define X_DISPEX_H_
#include <dispex.h>
#endif

#ifndef X_DOWNLOAD_HXX_
#define X_DOWNLOAD_HXX_
#include "download.hxx"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#define _cxx_
#include "htmlapp.hdl"

MtDefine(HTA, Behaviors, "HTA")
MtDefine(CAppBehavior, HTA, "CAppBehavior")

const CBase::CLASSDESC CAppBehavior::s_classdesc =
{
    &CLSID_HTMLAppBehavior,         // _pclsid
    0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
    NULL,                           // _apClsidPages
#endif // NO_PROPERTY_PAGE
    NULL,                           // _pcpi
    0,                              // _dwFlags
    &IID_IHTMLAppBehavior,          // _piidDispinterface
    &s_apHdlDescs,                  // _apHdlDesc
};

//+-------------------------------------------------------------------------
//
//  Method:     CBaseBehavior::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------
STDMETHODIMP
CBaseBehavior::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    if (!ppv)
        return E_POINTER;

    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_INHERITS(this, IElementBehavior)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }
    else
        RRETURN(super::PrivateQueryInterface(iid, ppv));
}

//+-------------------------------------------------------------------------
//
//  Method:     CBaseBehavior::Init
//
//  Synopsis:   Per IElementBehavior
//
//--------------------------------------------------------------------------
STDMETHODIMP
CBaseBehavior::Init(IElementBehaviorSite *pSite)
{
    HRESULT hr = E_INVALIDARG;

    if (pSite != NULL)
    {
        _pSite = pSite;
        _pSite->AddRef();
        hr = S_OK;
    }
    return hr;  
}

//+-------------------------------------------------------------------------
//
//  Method:     CBaseBehavior::Notify
//
//  Synopsis:   Per IElementBehavior
//
//--------------------------------------------------------------------------
STDMETHODIMP CBaseBehavior::Notify(LONG lNotify, VARIANT * pVarNotify)
{
    RRETURN(S_OK);
}



//+-------------------------------------------------------------------------
//
//  Method:     CBaseBehavior::CBaseBehavior
//              CBaseBehavior::~CBaseBehavior
//
//  Synopsis:   Constructor/Destructor for app behavior.
//
//--------------------------------------------------------------------------
CAppBehavior::CAppBehavior()
{
    // Load the commandline into the attribute array.
    SetAAcommandLine(theApp.cmdLine()); 
}

CAppBehavior::~CAppBehavior()
{
    if (_pBitsCtx)
    {
        _pBitsCtx->SetProgSink(NULL);
        _pBitsCtx->Disconnect();
        _pBitsCtx->Release();
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CBaseBehavior::QueryInterface
//
//  Synopsis:   Per IUnknown
//
//--------------------------------------------------------------------------
STDMETHODIMP
CAppBehavior::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    *ppv = NULL;

    switch (iid.Data1)
    {
    QI_TEAROFF_DISPEX(this, NULL)
    QI_HTML_TEAROFF(this, IHTMLAppBehavior, NULL)
    QI_HTML_TEAROFF(this, IHTMLAppBehavior2, NULL)
    }

    if (*ppv)
    {
        ((IUnknown *)*ppv)->AddRef();
        return S_OK;
    }

    RRETURN (super::PrivateQueryInterface(iid, ppv));
}

//+-------------------------------------------------------------------------
//
//  Method:     CAppBehavior::Init
//
//  Synopsis:   Per IElementBehavior
//
//  Notes:      Initialization function for APPLICATION tag
//--------------------------------------------------------------------------
STDMETHODIMP
CAppBehavior::Init(IElementBehaviorSite *pSite)
{
    HRESULT hr = super::Init(pSite);
    IHTMLElement *pElement = NULL;
    CDoc * pDoc;
    const TCHAR * pchUrl;

    if (_pSite)
    {
        hr = _pSite->GetElement(&pElement);
        if (SUCCEEDED(hr))
        {
            // Build up the attribute array...
            if (GetPropDescArray())
            {
                const PROPERTYDESC * const *    ppPropDescs = NULL;

                VARIANT attrValue;
                VariantInit(&attrValue);
            
                for (ppPropDescs = GetPropDescArray(); *ppPropDescs; ppPropDescs++)
                {
                    if ((*ppPropDescs)->pstrName)
                    {
                        hr = GetAttrValue(pElement, (*ppPropDescs)->pstrName, &attrValue);
                        if (SUCCEEDED(hr))
                        {
                            (*ppPropDescs)->HandleLoadFromHTMLString(this, attrValue.bstrVal);
                            VariantClear(&attrValue);
                        }
                    }
                }
            }
            ReleaseInterface(pElement);
        }
    }

    pchUrl = GetAAicon();
    if (!pchUrl || !*pchUrl)
        goto Cleanup;

    hr = THR(theApp._Client._pUnk->QueryInterface(
            CLSID_HTMLDocument,
            (void **) &pDoc));
    if (hr)
        goto Cleanup;

    // Notify the app that we are starting an async property download so it
    // can ignore the IDM_PARSECOMPLETE exec command if it comes in before 
    // this download is complete.
    theApp.Wait(TRUE);

    Assert(!_pBitsCtx);

    // Get the bits context
    hr = THR(pDoc->NewDwnCtx(DWNCTX_FILE, GetAAicon(),
                NULL, (CDwnCtx **) &_pBitsCtx));        // BUGBUG (lmollico): should we pass pElement?
    if (hr || !_pBitsCtx)
    {
        theApp.Wait(FALSE);
        goto Cleanup;
    }

    // if bits already got, call the callback
    if (_pBitsCtx->GetState() & (DWNLOAD_COMPLETE | DWNLOAD_ERROR | DWNLOAD_STOPPED))
        OnDwnChan();
    else    // register the callback
    {
        _pBitsCtx->SetProgSink(pDoc->GetProgSink());
        _pBitsCtx->SetCallback(OnDwnChanCallback, this);
        _pBitsCtx->SelectChanges(DWNCHG_COMPLETE, 0, TRUE);
    }

    return S_OK;
Cleanup:
    theApp.SetAttributes(this);
    return S_OK;
}


void CAppBehavior::OnDwnChan()
{
    HRESULT hr;
    ULONG ulState = _pBitsCtx->GetState();

    if (ulState & DWNLOAD_COMPLETE)
    {
        TCHAR * pchFile;

        hr = _pBitsCtx->GetFile(&pchFile);
        if (hr)
            goto Cleanup;

        SetAAicon(pchFile);     // BUGBUG (lmollico): we should use a private member string instead
        MemFreeString(pchFile);
    }

Cleanup:
    if (ulState & (DWNLOAD_COMPLETE | DWNLOAD_STOPPED | DWNLOAD_ERROR))
    {
        _pBitsCtx->SetProgSink(NULL);
        theApp.Wait(FALSE);
        theApp.SetAttributes(this);
    }
}


//+-------------------------------------------------------------------------
//
//  Method:     CAppBehavior::InvokeEx
//
//  Synopsis:   Per IDispatchEx
//
//  Notes:      Override of CBase implementation.  Filters out all dispids
//              except those generated by this behavior.
//--------------------------------------------------------------------------
STDMETHODIMP
CAppBehavior::InvokeEx(DISPID id, LCID lcid, WORD wFlags,
                      DISPPARAMS *pdp, VARIANT *pvarRes,
                      EXCEPINFO *pei, IServiceProvider *pspCaller)
{
    if (id == DISPID_CAppBehavior_commandLine)
        RRETURN(super::InvokeEx(id, lcid, wFlags, pdp, pvarRes, pei, pspCaller));
    else
        RRETURN_NOTRACE(DISP_E_MEMBERNOTFOUND);
}

//+-------------------------------------------------------------------------
//
//  Method:     CAppBehavior::GetDispID
//
//  Synopsis:   Per IDispatchEx
//
//  Notes:      Override of CBase implementation.  Filters out all properties
//              except those generated by this behavior.
//--------------------------------------------------------------------------
STDMETHODIMP
CAppBehavior::GetDispID(BSTR bstrName, DWORD grfdex, DISPID *pid)
{
    if (!StrCmpIC(bstrName, TEXT("commandLine")))
        return DISP_E_UNKNOWNNAME;
    else    
        RRETURN(super::GetDispID(bstrName, grfdex, pid));
}

//+-------------------------------------------------------------------------
//
//  Method:     CAppBehavior::GetStyles
//
//  Synopsis:   Returns window styles reflecting values specified in APPLICATION
//              tag attributes (or default values if not specified).
//
//--------------------------------------------------------------------------
DWORD
CAppBehavior::GetStyles()
{
    DWORD dwStyle = WS_OVERLAPPED | 
                    GetAAcaption() | 
                    GetAAsysMenu() | 
                    GetAAborder() | 
                    GetAAminimizeButton() | 
                    GetAAmaximizeButton();

    if (GetAAcaption() == HTMLCaptionFlagNo || GetAAborder() == HTMLBorderNone)
    {
        // The only way to get a captionless or borderless window is to turn 
        // off the caption and everything that goes along with it.  It must
        // also be a POPUP window, but we morph it later.
        dwStyle &= ~(WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);
    }

    return dwStyle;
}

//+-------------------------------------------------------------------------
//
//  Method:     CAppBehavior::GetExtendedStyles
//
//  Synopsis:   Returns extended window styles reflecting values specified in
//              APPLICATION tag attributes (or default values if not specified).
//
//--------------------------------------------------------------------------
DWORD CAppBehavior::GetExtendedStyles()
{
    DWORD dwStyleEx = GetAAborderStyle();
    return dwStyleEx;
}

//+---------------------------------------------------------------------------
//
//  Member:     GetAttrValue
//
//  Synopsis:   Given an element and attribute name, retrieves the current
//              value of that element.
//
//----------------------------------------------------------------------------
STDMETHODIMP
GetAttrValue(IHTMLElement *pElement, const TCHAR * pchAttrName, VARIANT *pVarOut)
{
    VARIANT attrName;
    HRESULT hr = S_OK;

    if (!pElement || !pVarOut)
        return E_POINTER;
        
    V_VT(&attrName) = VT_BSTR;
    V_BSTR(&attrName) = SysAllocString(pchAttrName);
    if ( !V_BSTR(&attrName) )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }
    hr = pElement->getAttribute(attrName.bstrVal, 0, pVarOut);
    VariantClear(&attrName);

    // If this attribute doesn't exist, clear the variant and return a failure code.
    if (V_VT(pVarOut) == VT_NULL)
    {
        VariantClear(pVarOut);
        hr = E_FAIL;
    }
Cleanup:        
    return hr;
}


