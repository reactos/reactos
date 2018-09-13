//+---------------------------------------------------------------------
//
//   File:      unknown.cxx
//
//  Contents:   Element class
//
//  Classes:    CUnknownElement
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_CONNECT_HXX_
#define X_CONNECT_HXX_
#include "connect.hxx"
#endif

#ifndef X_MSHTMLRC_H_
#define X_MSHTMLRC_H_
#include "mshtmlrc.h"
#endif

#ifndef X_ELEMENT_HXX_
#define X_ELEMENT_HXX_
#include "element.hxx"
#endif

#ifndef X_CFPF_HXX_
#define X_CFPF_HXX_
#include "cfpf.hxx"
#endif

#ifndef X_HTMTAGS_HXX_
#define X_HTMTAGS_HXX_
#include "htmtags.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X__DOC_H_
#define X__DOC_H_
#include "_doc.h"
#endif

#define _cxx_
#include "unknown.hdl"

MtDefine(CUnknownElement, Elements, "CUnknownElement")

//+----------------------------------------------------------------------------
//
//  Class:      CUnknownElement
//
//-----------------------------------------------------------------------------
             
const CElement::CLASSDESC CUnknownElement::s_classdesc =
{
    {
        &CLSID_HTMLUnknownElement,          // _pclsid
        0,                                  // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                     // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                             // _pcpi
        ELEMENTDESC_NOLAYOUT,               // _dwFlags
        &IID_IHTMLUnknownElement,           // _piidDispinterface
        &s_apHdlDescs,                      // _apHdlDesc
    },
    (void *)s_apfnIHTMLUnknownElement,      //_apfnTearOff

    NULL,                                   // _pAccelsDesign
    NULL                                    // _pAccelsRun
};


//+-----------------------------------------------------------
//
//  Class: CUnknownElement
//
//------------------------------------------------------------

CUnknownElement::CUnknownElement (CHtmTag *pht, CDoc *pDoc)
  : CElement(ETAG_UNKNOWN, pDoc)
{
#ifndef VSTUDIO7
    const TCHAR *pchTagName;
    HRESULT     hr;
    
    if (pht->GetTag() == ETAG_UNKNOWN)
        pchTagName = pht->GetPch();
    else
        pchTagName = NameFromEtag(pht->GetTag());

    if (pchTagName && pht->IsEnd() && *pchTagName != _T('/'))
    {
        // If this is an end tag and there is no slash in the tag name,
        // add one now.
        hr = THR(_cstrTagName.Set(_T("/")));
        if (hr)
            goto Error;
    }
    else
    {
        hr = THR(_cstrTagName.Set(_T("")));
        if (hr)
            goto Error;
    }
    
    if (pchTagName)
    {
        hr = THR(_cstrTagName.Append(pchTagName));
        if (hr)
            goto Error;
    }

    CharUpper(_cstrTagName);

Error:
    return;
#endif //VSTUDIO7
}

#ifdef VSTUDIO7
//+------------------------------------------------------------------------
//
//  Member:     CElement::InternalSetTagName
//
//  Synopsis:   Sets the tag and scope as attributes of an element.
//
//-------------------------------------------------------------------------
HRESULT
CUnknownElement::InternalSetTagName(CHtmTag *pht)
{
    HRESULT     hr;
    LPCTSTR     pchTagName;
    CStr        cstrTagName;
    
    if (pht->GetTag() == ETAG_UNKNOWN)
        pchTagName = pht->GetPch();
    else
        pchTagName = NameFromEtag(pht->GetTag());

    if (pchTagName && pht->IsEnd() && *pchTagName != _T('/'))
    {
        // If this is an end tag and there is no slash in the tag name,
        // add one now.
        hr = THR(cstrTagName.Set(_T("/")));
        if (hr)
            goto Error;
    }
    else
    {
        hr = THR(cstrTagName.Set(_T("")));
        if (hr)
            goto Error;
    }
    
    if (pchTagName)
    {
        hr = THR(cstrTagName.Append(pchTagName));
        if (hr)
            goto Error;
    }

    CharUpper(cstrTagName);
    
    hr = SetAAtagName(cstrTagName);
    if (hr)
        goto Error;
    
Error:
    RRETURN(hr);
}
#endif //VSTUDIO7

HRESULT CUnknownElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);
    *ppElement = new CUnknownElement(pht, pDoc);
    return *ppElement ? S_OK : E_OUTOFMEMORY;
}

//+------------------------------------------------------------------------
//
//  Method:     CUnknownElement::Init2
//
//-------------------------------------------------------------------------

HRESULT
CUnknownElement::Init2(CInit2Context * pContext)
{
    HRESULT     hr;

    hr = THR(super::Init2(pContext));
    if (hr)
        goto Cleanup;

#ifdef VSTUDIO7
    if (pContext && pContext->_pht)
    {
        hr = THR(InternalSetTagName(pContext->_pht));
    }
#endif

Cleanup:
    RRETURN (hr);
}
