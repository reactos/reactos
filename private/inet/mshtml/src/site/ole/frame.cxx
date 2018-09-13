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

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_FRAME_HXX_
#define X_FRAME_HXX_
#include "frame.hxx"
#endif

#ifndef X_FRAMESET_HXX_
#define X_FRAMESET_HXX_
#include "frameset.hxx"
#endif

#ifndef X_PROPS_HXX_
#define X_PROPS_HXX_
#include "props.hxx"
#endif

#define _cxx_
#include "frame.hdl"

MtDefine(CFrameElement, Elements, "CFrameElement")
MtDefine(CIFrameElement, Elements, "CIFrameElement")

const CElement::CLASSDESC CFrameElement::s_classdesc =
{
    {
        &CLSID_HTMLFrameElement,        // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NEVERSCROLL    |
        ELEMENTDESC_OLESITE,            // _dwFlags
        &IID_IHTMLFrameElement,         // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLFrameElement,  // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};

//+---------------------------------------------------------------------------
//
//  element creator used by parser
//
//----------------------------------------------------------------------------

HRESULT
CFrameElement::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElementResult)
{
    Assert(ppElementResult);

    *ppElementResult = new CFrameElement(pDoc);

    RRETURN ( (*ppElementResult) ? S_OK : E_OUTOFMEMORY);
}

//+---------------------------------------------------------------------------
//
//  Member: CFrameElement constructor
//
//----------------------------------------------------------------------------

CFrameElement::CFrameElement(CDoc *pDoc)
  : CFrameSite(ETAG_FRAME, pDoc)
{
}

//+----------------------------------------------------------------------------
//
// Member: CFrameElement:get_height
//
//-----------------------------------------------------------------------------
STDMETHODIMP CFrameElement::get_height(VARIANT * p)
{
    HRESULT hr = S_OK;

    if (p)
    {
        V_VT(p) = VT_I4;
        CLayout * pLayout = GetCurLayout();
        V_I4(p) = pLayout->GetHeight();
    }
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP CFrameElement::put_height(VARIANT p)
{
    RRETURN(SetErrorInfo(CTL_E_METHODNOTAPPLICABLE));
}

//+----------------------------------------------------------------------------
//
// Member: CFrameElement:get_width
//
//-----------------------------------------------------------------------------
STDMETHODIMP CFrameElement::get_width(VARIANT * p)
{
    HRESULT hr = S_OK;

    if (p)
    {
        V_VT(p) = VT_I4;
        CLayout * pLayout = GetCurLayout();
        V_I4(p) = pLayout->GetWidth();
    }
    RRETURN(SetErrorInfo(hr));
}

STDMETHODIMP CFrameElement::put_width(VARIANT p)
{
    RRETURN(SetErrorInfo(CTL_E_METHODNOTAPPLICABLE));
}
