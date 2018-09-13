//+---------------------------------------------------------------------
//
//   File:      inputbtn.cxx
//
//  Contents:   InputBtn element class, etc..
//
//  Classes:    CButton, etc..
//
//------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FORMKRNL_HXX_
#define X_FORMKRNL_HXX_
#include "formkrnl.hxx"
#endif

#ifndef X_TPOINTER_HXX_
#define X_TPOINTER_HXX_
#include "tpointer.hxx"
#endif

#ifndef X_POSTDATA_HXX_
#define X_POSTDATA_HXX_
#include "postdata.hxx"
#endif

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X_ROOTELEM_HXX_
#define X_ROOTELEM_HXX_
#include "rootelem.hxx"
#endif

#ifndef X_LAYOUT_HXX_
#define X_LAYOUT_HXX_
#include "layout.hxx"
#endif

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_BTNHLPER_HXX_
#define X_BTNHLPER_HXX_
#include "btnhlper.hxx"
#endif

#ifndef X_INPUTLYT_HXX_
#define X_INPUTLYT_HXX_
#include "inputlyt.hxx"
#endif

#ifndef X_STRBUF_HXX_
#define X_STRBUF_HXX_
#include "strbuf.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_IRANGE_HXX_
#define X_IRANGE_HXX_
#include "irange.hxx"
#endif

#ifndef X_EFORM_HXX_
#define X_EFORM_HXX_
#include "eform.hxx"
#endif

#ifndef X_MSHTMHST_H_
#define X_MSHTMHST_H_
#include <mshtmhst.h>
#endif

#ifndef X_BTNLYT_H_
#define X_BTNLYT_H_
#include <btnlyt.hxx>
#endif

#define _cxx_
#include "inputbtn.hdl"

MtDefine(CButton, Elements, "CButton")

IMPLEMENT_LAYOUT_FNS(CButton, CButtonLayout)

CElement::CLASSDESC CButton::s_classdescButtonReset =
{
    {
        &CLSID_HTMLButtonElement,       // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_CANCEL |            // _dwFlags, button/reset is a cancel button
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLButtonElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLButtonElement,   // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};

CElement::CLASSDESC CButton::s_classdescButtonSubmit =
{
    {
        &CLSID_HTMLButtonElement,       // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_DEFAULT |           // _dwFlags, button/submit is a default button
        ELEMENTDESC_NOTIFYENDPARSE |
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLButtonElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLButtonElement,   // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};

CElement::CLASSDESC CButton::s_classdescTagButton =
{
    {
        &CLSID_HTMLButtonElement,       // _pclsid
        0,                              // _idrBase
#ifndef NO_PROPERTY_PAGE
        s_apclsidPages,                 // _apClsidPages
#endif // NO_PROPERTY_PAGE
        s_acpi,                         // _pcpi
        ELEMENTDESC_NOTIFYENDPARSE |    // _dwFlags
        ELEMENTDESC_NOANCESTORCLICK |
        ELEMENTDESC_HASDEFDESCENT,
        &IID_IHTMLButtonElement,        // _piidDispinterface
        &s_apHdlDescs,                  // _apHdlDesc
    },
    (void *)s_apfnpdIHTMLButtonElement,   // _pfnTearOff
    NULL,                               // _pAccelsDesign
    NULL                                // _pAccelsRun
};

const CBase::CLASSDESC *
CButton::GetClassDesc() const
{
    switch (GetAAtype())
    {
        case htmlInputReset:
            return (CBase::CLASSDESC *)&s_classdescButtonReset;

        case htmlInputSubmit:
            return (CBase::CLASSDESC *)&s_classdescButtonSubmit;

        default:
            return (CBase::CLASSDESC *)&s_classdescTagButton;
    }
}

#ifndef NO_DATABINDING
#ifndef X_ELEMDB_HXX_
#define X_ELEMDB_HXX_
#include "elemdb.hxx"
#endif

const CDBindMethods *
CButton::GetDBindMethods()
{
    return &DBindMethodsTextRichRO;
}
#endif

HRESULT
CButton::CreateElement(CHtmTag *pht,
                              CDoc *pDoc, CElement **ppElement)
{
    Assert(ppElement);

    *ppElement = new CButton(pht->GetTag(), pDoc);

    RRETURN ( (*ppElement) ? S_OK : E_OUTOFMEMORY);
}

HRESULT
CButton::Init2(CInit2Context * pContext)
{
    HRESULT     hr = S_OK;
    htmlInput   type = GetAAtype();

    if (type!=htmlInputReset  && type!=htmlInputSubmit)
    {
        SetAAtype(htmlInputButton);
    }

    hr = THR(super::Init2(pContext));
    if (!OK(hr))
        goto Cleanup;

Cleanup:

    RRETURN1(hr, S_INCOMPLETE);
}



 //+----------------------------------------------------------------------------
//
//  Method:     GetSubmitInfo
//
//  Synopsis:   returns the submit info string if checked
//              (name && value pair)
//
//  Returns:    S_OK if successful
//              S_FALSE if not applicable for current element
//
//-----------------------------------------------------------------------------
HRESULT
CButton::GetSubmitInfo(CPostData * pSubmitData)
{
    CStr            cstrValue;
    const TCHAR *   pchName = GetAAsubmitname();
    HRESULT         hr      = GetValueHelper(&cstrValue);
    if (hr)
        goto Cleanup;


    hr = THR(pSubmitData->AppendNameValuePair(pchName, cstrValue, Doc()));

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CButton::GetEnabled
//
//  Synopsis:   return not disabled
//
//----------------------------------------------------------------------------

STDMETHODIMP
CButton::GetEnabled(VARIANT_BOOL * pfEnabled)
{
    if (!pfEnabled)
        RRETURN(E_INVALIDARG);

    *pfEnabled = !GetAAdisabled();
    return S_OK;
}

DWORD
CButton::GetBorderInfo(CDocInfo * pdci, CBorderInfo *pborderinfo, BOOL fAll)
{
    pborderinfo->abStyles[BORDER_TOP]    =
    pborderinfo->abStyles[BORDER_RIGHT]  =
    pborderinfo->abStyles[BORDER_BOTTOM] =
    pborderinfo->abStyles[BORDER_LEFT]   = BTN_PRESSED(_wBtnStatus)
                                                ? fmBorderStyleSunken
                                                : fmBorderStyleRaised;

    BOOL    nBorders = CElement::GetBorderInfo( pdci, pborderinfo, fAll );

    pborderinfo->aiWidths[BORDER_TOP]    += 1;  // xyFlat space
    pborderinfo->aiWidths[BORDER_RIGHT]  += 1;  // xyFlat space
    pborderinfo->aiWidths[BORDER_BOTTOM] += 1;  // xyFlat space
    pborderinfo->aiWidths[BORDER_LEFT]   += 1;  // xyFlat space

    return nBorders;
}


void
CButton::Notify(CNotification *pNF)
{
    super::Notify(pNF);
    switch (pNF->Type())
    {
    case NTYPE_ELEMENT_SETFOCUS:
        _wBtnStatus = BTN_SETSTATUS(_wBtnStatus, FLAG_HASFOCUS);
        break;

    case NTYPE_ELEMENT_ENTERTREE:
        // BUGBUG: is there any better way to do this?
        Layout()->GetDisplay()->SetWordWrap(FALSE);
        if (GetAAtype() == htmlInputSubmit)
        {
            SetDefaultElem();
        }
        break;

    case NTYPE_ELEMENT_EXITTREE_1:
        if (GetAAtype() == htmlInputSubmit)
        {
            if( pNF->DataAsDWORD() & EXITTREE_DESTROY )
            {
                Doc()->_pElemDefault = NULL;
            }
            else
            {
                SetDefaultElem(TRUE);
            }
        }
        break;
    }
}


HRESULT
CButton::YieldCurrency(CElement *pElemNew)
{
    HRESULT hr;

    hr = THR(super::YieldCurrency(pElemNew));
    if (hr)
        goto Cleanup;

    _wBtnStatus = BTN_RESSTATUS(_wBtnStatus, FLAG_HASFOCUS);

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CButton::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CButton::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    RRETURN1(Layout()->GetFocusShape(lSubDivision, pdci, ppShape), S_FALSE);
}



HRESULT BUGCALL
CButton::HandleMessage(CMessage * pMessage)
{
    HRESULT hr = S_FALSE;

    if (!CanHandleMessage())
        goto Cleanup;

    if (!IsEditable(TRUE))
    {
        if (!IsEnabled())
        {
            goto Cleanup;
        }

        hr = BtnHandleMessage(pMessage);
        if (hr == S_FALSE)
        {
            hr = super::HandleMessage(pMessage);
        }
    }
    else
    {
        if (pMessage->message == WM_CONTEXTMENU)
        {
            hr = THR(OnContextMenu(
                    (short)LOWORD(pMessage->lParam),
                    (short)HIWORD(pMessage->lParam),
                    CONTEXT_MENU_CONTROL));
        }
        if (hr == S_FALSE)
            hr = super::HandleMessage(pMessage);
    }

Cleanup:
    RRETURN1(hr, S_FALSE);
}

HRESULT
CButton::DoClick(CMessage * pMessage, CTreeNode *pNodeContext, BOOL fFromLabel)
{
    HRESULT hr = S_OK;

    if (!IsEditable(TRUE))
    {
        hr = super::DoClick(pMessage, pNodeContext, fFromLabel);
    }
    RRETURN1(hr, S_FALSE);
}


HRESULT
CButton::ClickAction(CMessage * pMessage)
{
    HRESULT         hr = S_OK;
    CFormElement *  pForm;

    pForm = GetParentForm();
    if (pForm)
    {
        switch (GetAAtype())
        {
            case htmlInputReset:
                pForm->DoReset(TRUE);
                break;

            case htmlInputSubmit:
                pForm->DoSubmit(this, TRUE);
                break;
        }
    }
    RRETURN1(hr, S_FALSE);
}

HRESULT
CButton::ApplyDefaultFormat(CFormatInfo *pCFI)
{
    HRESULT hr = S_OK;
    LOGFONT lf;
    CUnitValue uvBorder;
    DWORD dwRawValue;
    int i;
    CDoc * pDoc = Doc();

    pCFI->PrepareCharFormat();
    pCFI->PrepareFancyFormat();

    // Set default color and let super override it with the use style
    pCFI->_cf()._ccvTextColor.SetSysColor(COLOR_BTNTEXT);
    pCFI->_ff()._ccvBackColor.SetSysColor(COLOR_BTNFACE);

    // our intrinsics shouldn't inherit the cursor property. they have a 'default'
    pCFI->_cf()._bCursorIdx = styleCursorAuto;

#ifdef  NEVER
    if (pDoc->IsPrintDoc())
    {
        CODEPAGESETTINGS * pCS = pDoc->_pCodepageSettings;

        CODEPAGE cp = pDoc->GetCodePage();
        // Thai does not have a fixed pitch font. Leave it as proportional
        if(cp != CP_THAI)
        {
            pCFI->_cf()._bPitchAndFamily = FIXED_PITCH;
            // pCFI->_cf._fBumpSizeDown = TRUE;         // not necessary?
            pCFI->_cf()._latmFaceName = pCS->latmFixedFontFace;
        }
        pCFI->_cf()._bCharSet = pCS->bCharSet;
    }
    else
#endif
    {
        DefaultFontInfoFromCodePage( pDoc->GetCodePage(), &lf );
        pCFI->_cf()._bCharSet = lf.lfCharSet;
        pCFI->_cf().SetFaceName( lf.lfFaceName);
    }
    pCFI->_cf()._fNarrow = IsNarrowCharSet(pCFI->_cf()._bCharSet);
    pCFI->_cf()._fBold = FALSE;
    pCFI->_cf()._wWeight = 400;
    pCFI->_cf()._yHeight = 200; // 10 * 20 twips NS compatibility
    // pCFI->_cf._yHeight = 160; // 8 * 20 twips IE3 compatibility
    pCFI->_bBlockAlign     = htmlBlockAlignCenter;

    // Border info
    uvBorder.SetValue( 2, CUnitValue::UNIT_PIXELS );
    dwRawValue = uvBorder.GetRawValue();

    pCFI->_ff()._ccvBorderColorLight.SetSysColor(COLOR_BTNFACE);
    pCFI->_ff()._ccvBorderColorDark.SetSysColor(COLOR_3DDKSHADOW);
    pCFI->_ff()._ccvBorderColorHilight.SetSysColor(COLOR_BTNHIGHLIGHT);
    pCFI->_ff()._ccvBorderColorShadow.SetSysColor(COLOR_BTNSHADOW);

    if (pCFI->_cf()._fVisibilityHidden || pCFI->_cf()._fDisplayNone)
        _fButtonWasHidden = TRUE;

    for ( i = BORDER_TOP; i <= BORDER_LEFT; i++ )
        pCFI->_ff()._cuvBorderWidths[i].SetRawValue( dwRawValue );

    pCFI->_ff()._bBorderSoftEdges = TRUE;
    // End Border info

    pCFI->UnprepareForDebug();

    hr = THR(super::ApplyDefaultFormat(pCFI));
    if(hr)
        goto Cleanup;

    pCFI->PrepareCharFormat();

    pCFI->_cf().SetHeightInNonscalingTwips( pCFI->_pcf->_yHeight );

    pCFI->UnprepareForDebug();

Cleanup:
    RRETURN(hr);
}

HRESULT
CButton::put_status(VARIANT status)
{
    switch(status.vt)
    {
    case VT_NULL:
        _vStatus.vt = VT_NULL;
        break;
    case VT_BOOL:
        _vStatus.vt = VT_BOOL;
        V_BOOL(&_vStatus) = V_BOOL(&status);
        break;
    default:
        _vStatus.vt = VT_BOOL;
        V_BOOL(&_vStatus) = VB_TRUE;
    }

    Verify(S_OK==OnPropertyChange(DISPID_CButton_status, 0));

    RRETURN(S_OK);
}

HRESULT
CButton::get_status(VARIANT * pStatus)
{
    HRESULT hr = S_OK;

    if ( !pStatus )
    {
        hr = E_INVALIDARG;
        goto Cleanup;
    }

    if (_vStatus.vt==VT_NULL)
    {
        pStatus->vt = VT_NULL;
    }
    else
    {
        pStatus->vt = VT_BOOL;
        V_BOOL(pStatus) = V_BOOL(&_vStatus);
    }
Cleanup:
    RRETURN(SetErrorInfo(hr));
}

HRESULT
CButton::GetValueHelper(CStr *pstr)
{
    HRESULT hr = S_OK;
    BSTR    bStrValue;

    hr = THR(GetText(&bStrValue, WBF_NO_WRAP|WBF_NO_TAG_FOR_CONTEXT));
    if (hr)
        goto Cleanup;

    Assert(pstr);
    hr = pstr->SetBSTR(bStrValue);
    FormsFreeString(bStrValue);

Cleanup:
    RRETURN(hr);
}

HRESULT
CButton::SetValueHelper(CStr *pstr)
{
    HRESULT hr = S_OK;
    CDoc *  pDoc = Doc();

    // HACK: we would like to have version numbers on each
    // individual tree, but we don't know the full impact of that
    // instead, we will reset the contents version after this change
    // to make sure that the linebreaker doesn't restart during parsing.
    // We only do this if the contents haven't changed, not the 
    // structure of the tree.

    //
    // There is now a version number on markups.  Now, we only have to use
    // exclusively.  No one should use the doc version anymore.  And
    // then we will be able to get rid of this hack
    //
    
    long lDocContentsVersionBefore = pDoc->GetDocContentsVersion();
    long lDocTreeVersionBefore = pDoc->GetDocTreeVersion();

    Assert( pstr );

    hr = THR( Inject( Inside, TRUE, *pstr, pstr->Length() ) );

    if (lDocTreeVersionBefore == pDoc->GetDocTreeVersion() )
        pDoc->__lDocContentsVersion = lDocContentsVersionBefore;

    RRETURN(hr);
}


//+---------------------------------------------------------------------------
//
//  Member:     CButton::createTextRange
//
//----------------------------------------------------------------------------

HRESULT
CButton::createTextRange( IHTMLTxtRange * * ppDisp )
{
    HRESULT hr = S_OK;

    hr = THR( EnsureInMarkup() );
    
    if (hr)
        goto Cleanup;

    hr = THR( GetMarkup()->createTextRange( ppDisp, this ) );
    
    if (hr)
        goto Cleanup;

Cleanup:

    RRETURN( SetErrorInfo( hr ) );
}


