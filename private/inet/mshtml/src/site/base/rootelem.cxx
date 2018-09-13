//+---------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       rootelem.cxx
//
//  Contents:   Implementation of CRootElement
//
//  Classes:    CRootElement
//
//----------------------------------------------------------------------------

#include <headers.hxx>

#ifndef X_SWITCHES_HXX_
#define X_SWITCHES_HXX_
#include "switches.hxx"
#endif

#ifndef X_ROOTELEM_HXX
#define X_ROOTELEM_HXX
#include "rootelem.hxx"
#endif

#ifndef X_CODEPAGE_H_
#define X_CODEPAGE_H_
#include "codepage.h"
#endif

MtDefine(CRootElement, Elements, "CRootElement")


//////////////
//  Globals //
//////////////

const CElement::CLASSDESC CRootElement::s_classdesc =
{
    {
        NULL,                   // _pclsid
        0,                      // _idrBase
#ifndef NO_PROPERTY_PAGE
        0,                      // _apClsidPages
#endif // NO_PROPERTY_PAGE
        NULL,                   // _pcpi
        ELEMENTDESC_NOLAYOUT,   // _dwFlags
        NULL,                   // _piidDispinterface
        NULL
    },
    NULL,

    NULL,                       // _paccelsDesign
    NULL                        // _paccelsRun
};

void
CRootElement::Notify(CNotification *pNF)
{
    NOTIFYTYPE              ntype = pNF->Type();

    CMarkup *               pMarkup = GetMarkup();

    super::Notify(pNF);

    switch (ntype)
    {
    case NTYPE_SET_CODEPAGE:
        //
        // Directly switch the codepage (do not call SwitchCodePage)
        //
        {
            CDoc * pDoc = Doc();
            ULONG ulData;
            UINT WindowsCodePageFromCodePage( CODEPAGE cp );
        
            pNF->Data(&ulData);

            pDoc->_codepage = CODEPAGE(ulData);
            pDoc->_codepageFamily = WindowsCodePageFromCodePage(pDoc->_codepage);
        }
        break;

    case NTYPE_CLEAR_FORMAT_CACHES:
        GetFirstBranch()->VoidCachedInfo();
        break;

    case NTYPE_END_PARSE:
        pMarkup->SetLoaded(TRUE);

        break;
    }
    return;
}

HRESULT
CRootElement::ComputeFormats(CFormatInfo * pCFI, CTreeNode * pNodeTarget)
{
    SwitchesBegTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    CDoc *       pDoc = Doc();
    THREADSTATE *pts = GetThreadState();
    CColorValue  cv;
    COLORREF     cr;
    HRESULT      hr = S_OK;

    Assert(pCFI);
    Assert(SameScope( this, pNodeTarget));
    Assert(pCFI->_eExtraValues != ComputeFormatsType_Normal || (pNodeTarget->_iCF == -1 && pNodeTarget->_iPF == -1 && pNodeTarget->_iFF == -1));
    AssertSz(!TLS(fInInitAttrBag), "Trying to compute formats during InitAttrBag! This is bogus and must be corrected!");

    pCFI->Reset();
    pCFI->_pNodeContext = pNodeTarget;

    //
    // Setup Char Format
    //

    if (pDoc->_icfDefault < 0)
    {
        hr = THR(pDoc->CacheDefaultCharFormat());
        if (hr)
            goto Cleanup;
    }

    pCFI->_icfSrc = pDoc->_icfDefault;
    pCFI->_pcfSrc = pCFI->_pcf = pDoc->_pcfDefault;

    //
    // Setup Para Format
    //

    pCFI->_ipfSrc = pts->_ipfDefault;
    pCFI->_ppfSrc = pCFI->_ppf = pts->_ppfDefault;

    //
    // Setup Fancy Format
    //

    pCFI->_iffSrc = pts->_iffDefault;
    pCFI->_pffSrc = pCFI->_pff = pts->_pffDefault;
    
    cv = pDoc->GetAAbgColor();

    if (cv.IsDefined())
        cr = cv.GetColorRef();
    else
        cr = pDoc->_pOptionSettings->crBack();
    
    if (   !pCFI->_pff->_ccvBackColor.IsDefined() 
        ||  pCFI->_pff->_ccvBackColor.GetColorRef() != cr)
    {
        pCFI->PrepareFancyFormat();
        pCFI->_ff()._ccvBackColor = cr;
        pCFI->UnprepareForDebug();
    }

    Assert(pCFI->_pff->_ccvBackColor.IsDefined());

    if(pCFI->_eExtraValues == ComputeFormatsType_Normal)
    {
        hr = THR(pNodeTarget->CacheNewFormats(pCFI));
        if (hr)
            goto Cleanup;

        // If the doc codepage is Hebrew visual order, set the flag.
        pDoc->_fVisualOrder = (pDoc->GetCodePage() == CP_ISO_8859_8);
    }

Cleanup:

    SwitchesEndTimer(SWITCHES_TIMER_COMPUTEFORMATS);

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootElement::YieldCurrency
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CRootElement::YieldCurrency(CElement *pElemNew)
{
    return super::YieldCurrency( pElemNew );
}

//+---------------------------------------------------------------------------
//
//  Member:     CRootElement::YieldUI
//
//  Synopsis:
//
//----------------------------------------------------------------------------

void
CRootElement::YieldUI(CElement *pElemNew)
{
    // Note: We call Doc()->RemoveUI() if an embedded control
    // calls IOIPF::SetBorderSpace or IOIPF::SetMenu with non-null
    // values.

    Doc()->ShowUIActiveBorder(FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CRootElement::BecomeUIActive
//
//  Synopsis:
//
//----------------------------------------------------------------------------

HRESULT
CRootElement::BecomeUIActive()
{
    HRESULT hr = S_OK;

    // Nothing to do?

    // if the doc is not currently UIActive but a UIActive site exists, allow the
    // doc to go into UIACTIVE state.
    // CHROME
    // If Chrome hosted there is no valid HWND so CServer::GetFocus() is used
    // in place of ::GetFocus()
    if (!Doc()->IsChromeHosted())
    {
        if (Doc()->_pElemUIActive == this && GetFocus() == Doc()->InPlace()->_hwnd &&
            Doc()->State() >= OS_UIACTIVE)
            return S_OK;
    }
    else
    {
        if (Doc()->_pElemUIActive == this && Doc()->GetFocus() &&
            Doc()->State() >= OS_UIACTIVE)
            return S_OK;
    }

    // Tell the document that we are now the UI active site.
    // This will deactivate the current UI active site.

    hr = THR(Doc()->SetUIActiveElement(this));
    if (hr || Doc()->State() < OS_UIACTIVE)
        goto Cleanup;

    if (!Doc()->_pInPlace->_fDeactivating)
    {
        // We're now the UI active object, so tell the frame that.

        IGNORE_HR(Doc()->SetActiveObject());

#ifndef NO_OLEUI
        // Get our menus and toolbars up.

        IGNORE_HR(Doc()->InstallUI(FALSE));

        // If appropriate, show our grab handles.

        if (!Doc()->_fMsoDocMode &&
                (Doc()->GetAmbientBool(DISPID_AMBIENT_SHOWHATCHING, TRUE) ||
                Doc()->GetAmbientBool(DISPID_AMBIENT_SHOWGRABHANDLES, TRUE)))
        {
            Doc()->ShowUIActiveBorder(TRUE);
        }
#endif // NO_OLEUI
    }

Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------
//
//  Method:     CRootElement::QueryStatusUndoRedo
//
//  Synopsis:   Helper function for QueryStatus(). Check if in our current
//              state we suport these commands.
//
//--------------------------------------------------------------------

#ifndef NO_EDIT
HRESULT
CRootElement::QueryStatusUndoRedo(
        BOOL fUndo,
        MSOCMD * pcmd,
        MSOCMDTEXT * pcmdtext)
{
    BSTR        bstr = NULL;
    HRESULT     hr;

    // Get the Undo/Redo state.
    if (fUndo)
        hr = THR_NOTRACE(Doc()->_pUndoMgr->GetLastUndoDescription(&bstr));
    else
        hr = THR_NOTRACE(Doc()->_pUndoMgr->GetLastRedoDescription(&bstr));

    // Return the command state.
    pcmd->cmdf = hr ? MSOCMDSTATE_DISABLED : MSOCMDSTATE_UP;

    // Return the command text if requested.
    if (pcmdtext && pcmdtext->cmdtextf == MSOCMDTEXTF_NAME)
    {


        // BUGBUG - This code needs to be supported on the MAC. (rodc)
#if !defined(_MAC)
        if (hr)
        {
            pcmdtext->cwActual = LoadString(
                    GetResourceHInst(),
                    fUndo ? IDS_CANTUNDO : IDS_CANTREDO,
                    pcmdtext->rgwz,
                    pcmdtext->cwBuf);
        }
        else
        {
            hr = Format(
                    0,
                    pcmdtext->rgwz,
                    pcmdtext->cwBuf,
                    MAKEINTRESOURCE(fUndo ? IDS_UNDO : IDS_REDO),
                    bstr);
            if (!hr)
                pcmdtext->cwActual = _tcslen(pcmdtext->rgwz);
        }
#endif
    }

    if (bstr)
        FormsFreeString(bstr);

    return S_OK;
}
#endif // NO_EDIT

