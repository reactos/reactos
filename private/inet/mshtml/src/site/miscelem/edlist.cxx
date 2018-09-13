//+---------------------------------------------------------------------
//
//   File:      edlist.cxx
//
//  Contents:   Ordered List Element class
//
//  Classes:    CDListElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EDLIST_HXX_
#define X_EDLIST_HXX_
#include "edlist.hxx"
#endif

#define _cxx_
#include "dlist.hdl"

const CElement::CLASSDESC CDListElement::s_classdesc =
{
    {
        &CLSID_HTMLDListElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLDListElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLDListElement,      // _pfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


//+------------------------------------------------------------------------
//
//  Member:     CDListElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CDListElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_HTML_TEAROFF(this, IHTMLDListElement, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}


HRESULT CDListElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(pht->Is(ETAG_DL));
    Assert(ppElementResult);
    *ppElementResult = new CDListElement(pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}

