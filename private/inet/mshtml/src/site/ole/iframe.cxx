//+---------------------------------------------------------------------
//
//   File:      frame.cxx
//
//  Contents:   frame tag implementation
//
//  Classes:    CFrameSite, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_QI_IMPL_H_
#define X_QI_IMPL_H_
#include "qi_impl.h"
#endif

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifdef WIN16
#ifndef X_EXDISP_H_
#define X_EXDISP_H_
#include <exdisp.h>
#endif
#endif

#define _cxx_
#include "iframe.hdl"

const CElement::CLASSDESC CIFrameElement::s_classdesc =
{
    {
        &CLSID_HTMLIFrame,              // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NEVERSCROLL    |
        ELEMENTDESC_OLESITE,            // _dwFlags
        &IID_IHTMLIFrameElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLIFrameElement, // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};

//+---------------------------------------------------------------------------
//
//  element creator used by parser
//
//----------------------------------------------------------------------------

HRESULT
CIFrameElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);

    *ppElementResult = new CIFrameElement(pDoc);

    RRETURN ( (*ppElementResult) ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member: CIFrameElement constructor
//
//----------------------------------------------------------------------------

CIFrameElement::CIFrameElement(CDoc *pDoc)
  : CFrameSite(ETAG_IFRAME, pDoc)
{
}


//+----------------------------------------------------------------------------
//
//  Member:     CIFrameElement::PrivateQueryInterface, IUnknown
//
//  Synopsis:   Private unknown QI.
//
//-----------------------------------------------------------------------------

HRESULT
CIFrameElement::PrivateQueryInterface ( REFIID iid, void ** ppv )
{
    *ppv = NULL;
    switch(iid.Data1)
    {
        QI_INHERITS2(this, IUnknown, IHTMLIFrameElement)
        QI_HTML_TEAROFF(this, IHTMLIFrameElement, NULL)
        QI_HTML_TEAROFF(this, IHTMLIFrameElement2, NULL)
        QI_TEAROFF_DISPEX(this, NULL)
        default:
            RRETURN(THR_NOTRACE(super::PrivateQueryInterface(iid, ppv)));
    }

    if (!*ppv)
        RRETURN(E_NOINTERFACE);

    ((IUnknown *)*ppv)->AddRef();

    return S_OK;
}


//+----------------------------------------------------------------------------
//
// Member: CIFrameElement::ApplyDefaultFormat
//
//-----------------------------------------------------------------------------
HRESULT
CIFrameElement::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    long    cxHSpace = GetAAhspace();
    long    cyVSpace = GetAAvspace();

    if (cxHSpace || cyVSpace)
    {

        pCFI->PrepareFancyFormat();

        pCFI->_ff()._fHasMargins = TRUE;

        if (cxHSpace)
        {
            pCFI->_ff()._cuvMarginLeft.SetValue(cxHSpace, CUnitValue::UNIT_PIXELS);
            pCFI->_ff()._cuvMarginRight.SetValue(cxHSpace, CUnitValue::UNIT_PIXELS);
        }

        if (cyVSpace)
        {
            pCFI->_ff()._cuvMarginTop.SetValue(cyVSpace, CUnitValue::UNIT_PIXELS);
            pCFI->_ff()._cuvMarginBottom.SetValue(cyVSpace, CUnitValue::UNIT_PIXELS);
        }

        pCFI->UnprepareForDebug();
    }

    RRETURN(super::ApplyDefaultFormat(pCFI));
}

//+---------------------------------------------------------------------------
//
//  Member:     CIFrameElement::Save
//
//  Synopsis:   called twice: for opening <NOFRAMES> and for </NOFRAMES>.
//
//----------------------------------------------------------------------------

HRESULT
CIFrameElement::Save(CStreamWriteBuff * pStreamWrBuff, BOOL fEnd)
{
    HRESULT hr;

    hr = THR(super::Save(pStreamWrBuff, fEnd));
    if (hr)
        goto Cleanup;

    if (!fEnd && !pStreamWrBuff->TestFlag(WBF_SAVE_PLAINTEXT))
    {
        DWORD dwOldFlags = pStreamWrBuff->ClearFlags(WBF_ENTITYREF);

        pStreamWrBuff->SetFlags(WBF_KEEP_BREAKS | WBF_NO_WRAP);

        if (_cstrContents.Length())
        {
            hr = THR(pStreamWrBuff->Write(_cstrContents));
            if (hr)
                goto Cleanup;
        }

        pStreamWrBuff->RestoreFlags(dwOldFlags);
    }

Cleanup:

    RRETURN1(hr, S_FALSE);
}


