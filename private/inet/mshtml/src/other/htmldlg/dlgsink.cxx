//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1996
//
//  File:       dlgsink.cxx
//
//  Contents:   Implementation of the dlg sinks + extender
//
//  History:    06-25-96  AnandRa   Created
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#ifndef X_COMMIT_HXX_
#define X_COMMIT_HXX_
#include "commit.hxx"
#endif

#ifndef X_COREDISP_H_
#define X_COREDISP_H_
#include <coredisp.h>
#endif


DeclareTag(tagHTMLDlgExtenderMethods, "HTML Dialog Xtender", "Methods on the html dialog Xtender")

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgExtender::CHTMLDlgExtender
//
//  Synopsis:   constructor
//
//----------------------------------------------------------------------------

CHTMLDlgExtender::CHTMLDlgExtender(
    CHTMLDlg *      pDlg, 
    IHTMLElement *  pHTMLElement, 
    DISPID          dispid)
{
    HRESULT         hr = S_OK;
    CVariant        var;

    TraceTag((tagHTMLDlgExtenderMethods, "constructor"));

    _pDlg = pDlg;
    _pHTMLElement = pHTMLElement;
    _pHTMLElement->AddRef();
    _dispid = dispid;
    _ulRefs = 1;

    if (S_OK == hr)
        hr = THR_NOTRACE(GetDispProp(
                                    _pHTMLElement, 
                                    DISPID_A_VALUE, 
                                    g_lcidUserDefault, 
                                    &var, 
                                    NULL));

    _ExchangeValueBy = (S_OK == hr) ? EXCHANGEVALUEBY_VALUE : EXCHANGEVALUEBY_INNERTEXT;
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgExtender::~CHTMLDlgExtender
//
//  Synopsis:   destructor
//
//----------------------------------------------------------------------------

CHTMLDlgExtender::~CHTMLDlgExtender()
{
    TraceTag((tagHTMLDlgExtenderMethods, "destructor"));

    _pHTMLElement->Release();
}


//+---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgExtender::QueryInterface
//
//  Synopsis:   Per IUnknown.
//
//----------------------------------------------------------------------------

HRESULT
CHTMLDlgExtender::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IPropertyNotifySink)
    {
        *ppv = (IPropertyNotifySink *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgExtender::Value_PropPageToObject
//
//----------------------------------------------------------------------------

HRESULT
CHTMLDlgExtender::Value_PropPageToObject ()
{
    HRESULT     hr = S_OK;
    CVariant    var;
    CExcepInfo  ei;

    switch (_ExchangeValueBy)
    {

    case EXCHANGEVALUEBY_VALUE:

        hr = THR(::GetDispProp(
                _pHTMLElement,
                DISPID_A_VALUE,
                g_lcidUserDefault,
                &var,
                &ei));
        if (hr)
        {
            hr = THR(::GetDispProp(
                    _pHTMLElement,
                    DISPID_VALUE,
                    g_lcidUserDefault,
                    &var,
                    &ei));
            if (hr)
                goto Cleanup;
        }

        break;

    case EXCHANGEVALUEBY_INNERTEXT:

        hr = S_OK;
        goto Cleanup;
    }

    hr = THR(_pDlg->_pEngine->SetProperty(_dispid, &var));
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgExtender::Value_ObjectToPropPage
//
//----------------------------------------------------------------------------

HRESULT
CHTMLDlgExtender::Value_ObjectToPropPage ()
{
    HRESULT                 hr;
    CVariant                varValue;
    CVariant                varText;
    CExcepInfo              ei;
    IHTMLControlElement *   pControlElement = NULL;

    Assert (_pDlg && _pDlg->_pEngine);

    hr = THR(_pDlg->_pEngine->GetProperty(_dispid, &varValue));
    if (hr)
        goto Cleanup;

    switch (_ExchangeValueBy)
    {

    case EXCHANGEVALUEBY_VALUE:

        hr = THR(::SetDispProp(
                _pHTMLElement,
                DISPID_A_VALUE,
                g_lcidUserDefault,
                &varValue,
                &ei));
        if (hr)
        {
            hr = THR(::SetDispProp(
                _pHTMLElement,
                DISPID_VALUE,
                g_lcidUserDefault,
                &varValue,
                &ei));
            if (hr)
                goto Cleanup;
        }

        break;

    case EXCHANGEVALUEBY_INNERTEXT:

        hr = THR(VariantChangeTypeEx(&varText, &varValue, g_lcidUserDefault, 0, VT_BSTR));
        if (hr)
            goto Cleanup;

        hr = THR(_pHTMLElement->put_innerText(V_BSTR(&varText)));
        if (hr)
            goto Cleanup;

        break;

    default:
        Assert (0);
        break;
    }

Cleanup:
    ReleaseInterface (pControlElement);

    RRETURN (hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgExtender::OnChanged
//
//  Synopsis:   per IPropertyNotifySink.
//
//----------------------------------------------------------------------------

HRESULT
CHTMLDlgExtender::OnChanged(DISPID dispid)
{
    TraceTag((tagHTMLDlgExtenderMethods, "IPropertyNotifySink::OnChanged dispid:%d", dispid));

    //
    // Just look for the value property changing.  If this does
    // occur, then we have a dirty page.  Additionally, look to see
    // if this is occuring when the page is first being initialized.
    // Skip this prop change if so.
    //

    if ((DISPID_VALUE == dispid || DISPID_A_VALUE == dispid) && !_pDlg->_fInitializing)
    {
        _pDlg->OnPropertyChange(this);
    }
    
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CHTMLDlgExtender::OnRequestEdit
//
//  Synopsis:   per IPropertyNotifySink.
//
//----------------------------------------------------------------------------

HRESULT
CHTMLDlgExtender::OnRequestEdit(DISPID dispid)
{
    TraceTag((tagHTMLDlgExtenderMethods, "IPropertyNotifySink::OnRequestEdit dispid:%d", dispid));
    
    return S_OK;
}



//+---------------------------------------------------------------------------
//
//  Member:     CDlgDocPNS::QueryInterface
//
//  Synopsis:   Per IUnknown.
//
//----------------------------------------------------------------------------

IMPLEMENT_FORMS_SUBOBJECT_IUNKNOWN(CDlgDocPNS, CHTMLDlg, _PNS)

STDMETHODIMP
CDlgDocPNS::QueryInterface(REFIID iid, void ** ppv)
{
    if (iid == IID_IUnknown || iid == IID_IPropertyNotifySink)
    {
        *ppv = (IPropertyNotifySink *) this;
    }
    else
    {
        *ppv = NULL;
        RRETURN_NOTRACE(E_NOINTERFACE);
    }

    ((IUnknown *) *ppv)->AddRef();
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDlgDocPNS::OnChanged
//
//  Synopsis:   
//
//----------------------------------------------------------------------------

STDMETHODIMP CDlgDocPNS::OnChanged(DISPID dispid)
{

    RRETURN(_pDlg->OnPropertyChange(dispid, 0));
}
