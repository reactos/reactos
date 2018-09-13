//+---------------------------------------------------------------------
//
//   File:      eulist.cxx
//
//  Contents:   Ordered List Element class
//
//  Classes:    CUListElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EULIST_HXX_
#define X_EULIST_HXX_
#include "eulist.hxx"
#endif

#define _cxx_
#include "ulist.hdl"

const CElement::CLASSDESC CUListElement::s_classdesc =
{
    {
        &CLSID_HTMLUListElement,            // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLUListElement,             // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLUListElement,      // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


//+------------------------------------------------------------------------
//
//  Member:     CUListElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-------------------------------------------------------------------------

HRESULT
CUListElement::PrivateQueryInterface(REFIID iid, void ** ppv)
{
    HRESULT hr;

    *ppv = NULL;

    if IID_HTML_TEAROFF(this, IHTMLUListElement, NULL)
    else
    {
        RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    (*(IUnknown **)ppv)->AddRef();
    return S_OK;
}


HRESULT CUListElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(pht->Is(ETAG_UL) || pht->Is(ETAG_MENU) || pht->Is(ETAG_DIR));
    Assert(ppElementResult);
    *ppElementResult = new CUListElement(pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}

HRESULT
CUListElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr;

    hr = super::ApplyDefaultFormat(pCFI);
    if (hr)
        goto Cleanup;

    pCFI->PrepareParaFormat();
    pCFI->_pf()._cListing.SetStyle( FilterHtmlListType( pCFI->_pff->_ListType,
        (WORD)pCFI->_pf()._cListing.GetLevel() ) );
    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------
//
//  Member:     FilterHtmlListType()
//
//  Returns:    Return the perferred htmlListType for unordered lists.
//
//------------------------------------------------------------------------

styleListStyleType
CUListElement::FilterHtmlListType(  styleListStyleType type, WORD wLevel )
{
    return ( styleListStyleTypeNotSet != type ? type :
            ((wLevel == 1) ? styleListStyleTypeDisc :
             (wLevel == 2) ? styleListStyleTypeCircle : styleListStyleTypeSquare) );
}

