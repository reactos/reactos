//+---------------------------------------------------------------------
//
//   File:      eblock.cxx
//
//  Contents:   Div element class
//
//  Classes:    CDivElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_DIV_HXX_
#define X_DIV_HXX_
#include "div.hxx"
#endif

#ifndef X_E1D_HXX_
#define X_E1D_HXX_
#include "e1d.hxx"
#endif

#ifndef X_STYLE_HXX_
#define X_STYLE_HXX_
#include "style.hxx"
#endif

class CHtmTag;

#define _cxx_
#include "div.hdl"

interface IHTMLControlElement;

extern "C" const IID IID_IControl;

const CElement::CLASSDESC CDivElement::s_classdesc =
{
    {
        &CLSID_HTMLDivElement,               // _pclsid
        0,                                   // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                      // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                              // _pcpi
        0,                                   // _dwFlags
        &IID_IHTMLDivElement,                // _piidDispinterface
        &s_apHdlDescs,                       // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLDivElement,         // _pfnTearOff
    NULL,                                    // _pAccelsDesign
    NULL                                     // _pAccelsRun
};

HRESULT
CDivElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_DIV));
    Assert(ppElement);
    *ppElement = new CDivElement(pDoc);
    return *ppElement ? S_OK : E_OUTOFMEMORY;
}

#ifndef NO_DATABINDING

#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

const CDBindMethods *
CDivElement::GetDBindMethods()
{
    return &DBindMethodsTextRichRO;
}

//+----------------------------------------------------------------------------
//
//  Member:     CDivElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CDivElement::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    HRESULT hr;

    *ppv = NULL;

    // IE4 shipped the interface IHTMLControlElement with the same GUID as
    // IControl.  Unfortunately, IControl is a forms^3 interface, which is bad.
    // To resolve this problem Trident's GUID for IHTMLControlElement has
    // changed however, the old GUID remembered in the QI for CSite to return
    // IHTMLControlElement.  The only side affect is that using the old GUID
    // will not marshall the interface correctly only the new GUID has the
    // correct marshalling code.  So, the solution is that QI'ing for
    // IID_IControl or IID_IHTMLControlElement will return IHTMLControlElement.

    // For VB page designer we need to emulate IE4 behavior (fail the QI if not a site)
    if(iid == IID_IControl && Doc()->_fVB && !HasLayout())
        RRETURN(E_NOINTERFACE);

        if (iid == IID_IHTMLControlElement || iid == IID_IControl)
    {

        hr = CreateTearOffThunk(this,
                                s_apfnpdIHTMLControlElement,
                                NULL,
                                ppv,
                                (void *)s_ppropdescsInVtblOrderIHTMLControlElement);
        if (hr)
            RRETURN(hr);
    }
    else if (iid == IID_IHTMLTextContainer)
    {
        hr = CreateTearOffThunk(this,
                                (void *)this->s_apfnIHTMLTextContainer,
                                NULL,
                                ppv);
        if (hr)
            RRETURN(hr);
    }
/*
    else if (iid == IID_IHTMLDivPosition)
    {
        hr = CreateTearOffThunk(this, 
                                s_apfnpdIHTMLDivPosition, 
                                NULL, 
                                ppv,
                                (void*)s_ppropdescsInVtblOrderIHTMLDivPosition);
        if (hr)
            RRETURN(hr);
    }
*/
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}
#endif // ndef NO_DATABINDING
