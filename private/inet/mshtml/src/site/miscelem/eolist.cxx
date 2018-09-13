//+---------------------------------------------------------------------
//
//   File:      eolist.cxx
//
//  Contents:   Ordered List Element class
//
//  Classes:    COListElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EOLIST_HXX_
#define X_EOLIST_HXX_
#include "eolist.hxx"
#endif

#define _cxx_
#include "olist.hdl"

EXTERN_C const ENUMDESC s_enumdescTYPE;

const CElement::CLASSDESC COListElement::s_classdesc =
{
    {
        &CLSID_HTMLOListElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLOListElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLOListElement,      // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


//+------------------------------------------------------------------------
//
//  Member:     COListElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
COListElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_HTML_TEAROFF(this, IHTMLOListElement, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}


HRESULT COListElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(pht->Is(ETAG_OL));
    Assert(ppElementResult);
    *ppElementResult = new COListElement(pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}


HRESULT
COListElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr;

    hr = super::ApplyDefaultFormat(pCFI);
    if (hr)
        goto Cleanup;

    pCFI->PrepareParaFormat();
    pCFI->_pf()._lNumberingStart = GetAAstart();
    pCFI->_pf()._cListing.SetStyle( FilterHtmlListType(pCFI->_pff->_ListType, 0) );
    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     FilterHtmlListType()
//
//  Returns:    Return the perferred htmlListType for ordered lists.
//
//------------------------------------------------------------------------

styleListStyleType
COListElement::FilterHtmlListType(  styleListStyleType type, WORD wLevel )
{
    return ( styleListStyleTypeNotSet != type) ? type : styleListStyleTypeDecimal;
}


//+-----------------------------------------------------------------------
//
//  Member:     OnPropertyChange()
//
//  Note  :    Trap the change to start attribute to inval the index caches
//
//------------------------------------------------------------------------
HRESULT
COListElement::OnPropertyChange(DISPID dispid, DWORD dwFlags)
{
    HRESULT hr;

    if (dispid == DISPID_COListElement_start)
    {
        UpdateVersion();
    }
    hr = THR( super::OnPropertyChange( dispid, dwFlags ) );

    RRETURN( hr );
}
