//+---------------------------------------------------------------------
//
//   File:      ebr.cxx
//
//  Contents:   BR element class
//
//  Classes:    CBRElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_EBR_HXX_
#define X_EBR_HXX_
#include "ebr.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include <strbuf.hxx>
#endif

#define _cxx_
#include "br.hdl"

MtDefine(CBRElement, Elements, "CBRElement")

const CElement::CLASSDESC CBRElement::s_classdesc =
{
    {
        &CLSID_HTMLBRElement,               // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLBRElement,                // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLBRElement,         // _pfnTearOff
    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};

HRESULT
CBRElement::CreateElement(
    CHtmTag *pht,
    CDoc *pDoc,
    CElement **ppElement)
{
    Assert(pht->Is(ETAG_BR));

    Assert(ppElement);
    *ppElement = new CBRElement(pDoc);
    return *ppElement ? S_OK: E_OUTOFMEMORY;
}


HRESULT
CBRElement::Save(
    CStreamWriteBuff * pStreamWrBuff,
    BOOL fEnd )
{
    HRESULT hr;

    hr = THR( super::Save(pStreamWrBuff, fEnd) );
    if (hr)
        goto Cleanup;

    if (fEnd && pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        hr = THR( pStreamWrBuff->NewLine() );
    }

Cleanup:

    RRETURN(hr);
}
