//+---------------------------------------------------------------------
//
//   File:      txtelems.cxx
//
//  Contents:   Element class
//
//  Classes:    CElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_TXTELEMS_HXX_
#define X_TXTELEMS_HXX_
#include "txtelems.hxx"
#endif

#ifndef X_HTMLDLG_HXX_
#define X_HTMLDLG_HXX_
#include "htmldlg.hxx"
#endif

#define _cxx_
#include "textelem.hdl"

MtDefine(CTextElement, Elements, "CTextElement")

const CElement::CLASSDESC CTextElement::s_classdesc =
{
    {
        &CLSID_HTMLTextElement,             // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLTextElement,              // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLTextElement,         // _pfnTearOff

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


HRESULT CTextElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);
    *ppElementResult = new CTextElement(pht->GetTag(), pDoc);
    return *ppElementResult ? S_OK : E_OUTOFMEMORY;
}


