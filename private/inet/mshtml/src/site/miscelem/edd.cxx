//+---------------------------------------------------------------------
//
//   File:      edd.cxx
//
//  Contents:   DD element class
//
//  Classes:    CDDElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EDD_HXX_
#define X_EDD_HXX_
#include "edd.hxx"
#endif

#ifndef X_ELIST_HXX_
#define X_ELIST_HXX_
#include "elist.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#define _cxx_
#include "dd.hdl"

MtDefine(CDDElement, Elements, "CDDElement")

const CElement::CLASSDESC CDDElement::s_classdesc =
{
    {
        &CLSID_HTMLDDElement,               // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLDDElement,                // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLDDElement,         // apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CDDElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_DD));
    Assert(ppElement);
    *ppElement = new CDDElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}

HRESULT
CDDElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;

    // If we're under a DL, we indent the whole paragraph, otherwise,
    // just the first line.
    if (GetFirstBranch()->Ancestor(ETAG_DL))
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._fHasMargins = TRUE;
        if (!pCFI->_ppf->HasRTL(TRUE))
        {
            pCFI->_ff()._cuvMarginLeft.SetPoints( LIST_INDENT_POINTS );
        }
        else
        {
            pCFI->_ff()._cuvMarginRight.SetPoints( LIST_INDENT_POINTS );
        }
        pCFI->UnprepareForDebug();
    }
    else
    {
        pCFI->PrepareParaFormat();
        pCFI->_pf()._fFirstLineIndentForDD = TRUE;
        pCFI->UnprepareForDebug();
    }

    // Restart leveling. This means that nested DLs under this DD will
    // not indent anymore. This is all for Netscape compatibility.
    pCFI->PrepareParaFormat();
    pCFI->_pf()._cListing.SetLevel(0);
    pCFI->_pf()._cListing.SetType(CListing::DEFINITION);
    pCFI->UnprepareForDebug();

    pCFI->PrepareFancyFormat();
    pCFI->_ff()._cuvSpaceBefore.SetPoints(0);
    pCFI->_ff()._cuvSpaceAfter.SetPoints(0);
    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}
