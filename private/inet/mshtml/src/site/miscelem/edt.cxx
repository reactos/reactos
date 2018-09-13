//+---------------------------------------------------------------------
//
//   File:      edd.cxx
//
//  Contents:   DT element class
//
//  Classes:    CDTElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EDT_HXX_
#define X_EDT_HXX_
#include "edt.hxx"
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
#include "dt.hdl"

MtDefine(CDTElement, Elements, "CDTElement")

const CElement::CLASSDESC CDTElement::s_classdesc =
{
    {
        &CLSID_HTMLDTElement,               // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLDTElement,                // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLDTElement,         // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CDTElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_DT));

    Assert(ppElement);
    *ppElement = new CDTElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}

HRESULT 
CDTElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;

    pCFI->PrepareParaFormat();
    pCFI->_pf()._cListing.SetType( CListing::DEFINITION );
    pCFI->UnprepareForDebug();

    pCFI->PrepareFancyFormat();
    pCFI->_ff()._cuvSpaceBefore.SetValue(0, CUnitValue::UNIT_POINT);
    pCFI->UnprepareForDebug();

    pCFI->PrepareFancyFormat();
    pCFI->_ff()._cuvSpaceAfter.SetValue(0, CUnitValue::UNIT_POINT);
    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

Cleanup:
    RRETURN(hr);
}

