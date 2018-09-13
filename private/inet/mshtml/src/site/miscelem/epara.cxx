//+---------------------------------------------------------------------
//
//   File:      epara.cxx
//
//  Contents:   Para element class
//
//  Classes:    CParaElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EPARA_HXX_
#define X_EPARA_HXX_
#include "epara.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#define _cxx_
#include "para.hdl"

const CElement::CLASSDESC CParaElement::s_classdesc =
{
    {
        &CLSID_HTMLParaElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        0,                                  // _dwFlags
        &IID_IHTMLParaElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLParaElement,       // _apfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CParaElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(pht->Is(ETAG_P));

    Assert(ppElement);
    *ppElement = new CParaElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}

HRESULT
CParaElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;

    hr = super::Save(pStreamWrBuff, fEnd);
    if (hr)
        goto Cleanup;
    
    if (fEnd && pStreamWrBuff->TestFlag(WBF_FORMATTED_PLAINTEXT))
    {
        // Double space in plaintext mode after <P> elements
        hr = pStreamWrBuff->NewLine();
        if (hr)
            goto Cleanup;
    }

Cleanup:
    RRETURN(hr);
}

       
