//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       inputlyt.cxx
//
//  Contents:   Implementation of layout class for <INPUT> controls.
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_RECT_HXX_
#define X_RECT_HXX_
#include "rect.hxx"
#endif

#ifndef X_SIZE_HXX_
#define X_SIZE_HXX_
#include "size.hxx"
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

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X_INTL_HXX_
#define X_INTL_HXX_
#include "intl.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X_INPUTTXT_HXX_
#define X_INPUTTXT_HXX_
#include "inputtxt.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif


MtDefine(CInputLayout,       Layout, "CInputLayout")
MtDefine(CInputTextLayout,   Layout, "CInputTextLayout")
MtDefine(CInputFileLayout,   Layout, "CInputFileLayout")
MtDefine(CInputButtonLayout, Layout, "CInputButtonLayout")

extern void DrawTextSelectionForRect(HDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor);


const CLayout::LAYOUTDESC CInputLayout::s_layoutdesc =
{
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_FLOWLAYOUT,      // _dwFlags
};

const CLayout::LAYOUTDESC CInputFileLayout::s_layoutdesc =
{
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_NOTALTERINSET    |
    LAYOUTDESC_NEVEROPAQUE      |
    LAYOUTDESC_FLOWLAYOUT,      // _dwFlags
};

const CLayout::LAYOUTDESC CInputButtonLayout::s_layoutdesc =
{
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};


HRESULT
CInputTextLayout::OnTextChange(void)
{
    CInput * pInput  = DYNCAST(CInput, ElementOwner());

    if (!pInput->IsEditable(TRUE))
        pInput->_fTextChanged = TRUE;

    if (pInput->_fFiredValuePropChange)
    {
        pInput->_fFiredValuePropChange = FALSE;
    }
    else
    {
        pInput->OnPropertyChange(DISPID_CInput_value, 0); // value change
    }

    return S_OK;
}

// JS OnSelection event
HRESULT
CInputLayout::OnSelectionChange(void)
{
    DYNCAST(CInput, ElementOwner())->Fire_onselect();  //JS event
    return S_OK;
}



CInputTextLayout::CInputTextLayout(CElement * pElementLayout)
                                : super(pElementLayout)
{
}

//+------------------------------------------------------------------------
//
//  Member:     CInputTextLayout::PreDrag
//
//  Synopsis:   Prevent dragging text out of Password control
//
//-------------------------------------------------------------------------

HRESULT
CInputTextLayout::PreDrag(
    DWORD           dwKeyState,
    IDataObject **  ppDO,
    IDropSource **  ppDS)
{
    HRESULT hr;

    if (DYNCAST(CInput, ElementOwner())->GetType() == htmlInputPassword)
        hr = S_FALSE;
    else
        hr = super::PreDrag(dwKeyState, ppDO, ppDS);
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CInputFileLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CRect           rc;
    CRectShape *    pShape;
    CBorderInfo     biButton;
    CInput *        pInputFile = DYNCAST(CInput, ElementOwner());
    HRESULT         hr = S_FALSE;
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);

    *ppShape = NULL;

    if (!pInputFile->_fButtonHasFocus)
        goto Cleanup;

    GetRect(&rc, COORDSYS_CONTENT);
    if (rc.IsEmpty())
        goto Cleanup;

    pShape = new CRectShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    if(!fRightToLeft)
        rc.left = rc.right - _sizeButton.cx;
    else
        rc.right = rc.left + _sizeButton.cx;

    ComputeInputFileBorderInfo(pdci, biButton);

    rc.top     = rc.top + biButton.aiWidths[BORDER_TOP];
    rc.left    = rc.left + biButton.aiWidths[BORDER_LEFT];
    rc.bottom  = rc.bottom - biButton.aiWidths[BORDER_BOTTOM];
    rc.right   = rc.right - biButton.aiWidths[BORDER_RIGHT];

    rc.InflateRect(-1, -1);

    pShape->_rect = rc;
    *ppShape = pShape;
    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}


//+---------------------------------------------------------------------------
//
//  Member:     CInputButtonLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CInputButtonLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CRect           rc;
    CBorderInfo     bi;
    CRectShape *    pShape;
    HRESULT         hr = S_FALSE;

    *ppShape = NULL;

    ElementOwner()->GetBorderInfo(pdci, &bi);
    GetRect(&rc, COORDSYS_CONTENT);
    if (rc.IsEmpty())
        goto Cleanup;

    pShape = new CRectShape;
    if (!pShape)
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    }

    // -1/+1 to exclude xflat border
    pShape->_rect = rc;
    pShape->_rect.top     += bi.aiWidths[BORDER_TOP] - 1;
    pShape->_rect.left    += bi.aiWidths[BORDER_LEFT] -1;
    pShape->_rect.bottom  -= bi.aiWidths[BORDER_BOTTOM] + 1;
    pShape->_rect.right   -= bi.aiWidths[BORDER_RIGHT] + 1;
    pShape->_rect.InflateRect(-1, -1);
    *ppShape = pShape;
    hr = S_OK;

Cleanup:
    RRETURN1(hr, S_FALSE);
}

void
CInputButtonLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    super::DrawClient( prcBounds,
                       prcRedraw,
                       pDispSurface,
                       pDispNode,
                       cookie,
                       pClientData,
                       dwFlags);

    // (bug 49150) Has the button just appeared? Should it be the default element
    CInput * pButton = DYNCAST(CInput, ElementOwner());
    const CCharFormat *pCF = GetFirstBranch()->GetCharFormat();
    Assert(pButton && pCF);

    if (pButton->GetBtnWasHidden() && pButton->GetType() == htmlInputSubmit
        && !pCF->IsDisplayNone() && !pCF->IsVisibilityHidden())
    {
        pButton->SetDefaultElem();
        pButton->SetBtnWasHidden( FALSE );
    }
}

void CInputButtonLayout::DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    CDoc *          pDoc = Doc();
    CBorderInfo     bi;
    BOOL            fDefaultAndCurrent = pDoc && ElementOwner()->_fDefault
                                    && ElementOwner()->IsEnabled()
                                    && pDoc->HasFocus();

    Verify(ElementOwner()->GetBorderInfo(pDI, &bi, TRUE));

    // draw default if necessary
    bi.acrColors[BORDER_TOP][1]
        = bi.acrColors[BORDER_RIGHT][1]
        = bi.acrColors[BORDER_BOTTOM][1]
        = bi.acrColors[BORDER_LEFT][1]
        = fDefaultAndCurrent ?
            RGB(0,0,0)
            : ElementOwner()->GetInheritedBackgroundColor();
    bi.xyFlat = pDI->DocPixelsFromWindowX(fDefaultAndCurrent ? -1 : 1,
                                                FALSE);

    bi.aiWidths[BORDER_TOP]    = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_TOP], FALSE);
    bi.aiWidths[BORDER_RIGHT]  = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_RIGHT], FALSE);
    bi.aiWidths[BORDER_BOTTOM] = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM], FALSE);
    bi.aiWidths[BORDER_LEFT]   = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT], FALSE);
    ::DrawBorder(pDI, (RECT *)prcBounds, &bi);
}


CBtnHelper * CInputButtonLayout::GetBtnHelper()
{
    CElement * pElement = ElementOwner();
    Assert(pElement);
    CInput * pButton = DYNCAST(CInput, pElement);
    return pButton->GetBtnHelper();
}

#define TEXT_INSET_DEFAULT_TOP      1
#define TEXT_INSET_DEFAULT_BOTTOM   1
#define TEXT_INSET_DEFAULT_RIGHT    1
#define TEXT_INSET_DEFAULT_LEFT     1

htmlInput
CInputLayout::GetType() const
{
    return DYNCAST(CInput, ElementOwner())->GetType();
}

BOOL
CInputLayout::GetMultiLine() const
{
    return IsTypeMultiline(GetType());
}

BOOL 
CInputLayout::GetAutoSize() const
{
    switch(GetType())
    {
    case htmlInputButton:
    case htmlInputSubmit:
    case htmlInputReset:
        return TRUE;
    default:
        return FALSE;
    }
}

HRESULT
CInputLayout::GetFontSize(CCalcInfo * pci, SIZE * psizeFontForShortStr, SIZE * psizeFontForLongStr)
{
    HRESULT hr = S_OK;
    CCcs    * pccs;
    CBaseCcs *pBaseCcs;
    const CCharFormat *pCF;

    pCF = GetFirstBranch()->GetCharFormat();

    pccs = fc().GetCcs(pci->_hdc, pci, pCF);
    pBaseCcs = pccs ? pccs->GetBaseCcs() : NULL;
    if (!pccs || !pBaseCcs || !pci || !pci->_hdc || !pBaseCcs->_hfont)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    // WE DON'T need externalleading here any more.

    psizeFontForShortStr->cx = pBaseCcs->_xMaxCharWidth;
    psizeFontForLongStr->cx =  pBaseCcs->_xAveCharWidth;
    psizeFontForShortStr->cy = psizeFontForLongStr->cy = pBaseCcs->_yHeight;
    pccs->Release();

Cleanup:
    RRETURN(hr);
}

DWORD
CInputButtonLayout::CalcSizeHelper(
    CCalcInfo * pci,
    SIZE *      psize)
{
    DWORD       grfReturn=0;
    SIZEMODE    smModeSave;
	unsigned	fMinmaxValidSave;

    BOOL fMinMax     = pci->_smMode == SIZEMODE_MMWIDTH ||
                       pci->_smMode == SIZEMODE_MINWIDTH;

    CUnitValue  uvWidth = ElementOwner()->GetFirstBranch()->GetCascadedwidth();
    CUnitValue  uvHeight= ElementOwner()->GetFirstBranch()->GetCascadedheight();
    BOOL        fWidthNotSet    = uvWidth.IsNullOrEnum();
    BOOL        fHeightNotSet   = uvHeight.IsNullOrEnum();
    SIZE        sizeStyle;
    CBorderInfo bInfo;
    SIZE        sizeFontForShortStr, sizeFontForLongStr;
    long        lBorderWY = pci->DocPixelsFromWindowY(bInfo.aiWidths[BORDER_TOP]                                
        + bInfo.aiWidths[BORDER_BOTTOM]);

    _fContentsAffectSize = fWidthNotSet || fHeightNotSet;

    ElementOwner()->GetBorderInfo(pci, &bInfo, FALSE);
    GetFontSize(pci, &sizeFontForShortStr, &sizeFontForLongStr);

    if (!fWidthNotSet)
    {
        psize->cx = (!fMinMax || !PercentWidth()
                        ? uvWidth.XGetPixelValue(pci, pci->_sizeParent.cx,
                            GetFirstBranch()->GetFontHeightInTwips(&uvWidth) )
                        : 0);
    }
    else
    {
        psize->cx = sizeFontForLongStr.cx
                        + pci->DocPixelsFromWindowX(bInfo.aiWidths[BORDER_LEFT]
                                                + bInfo.aiWidths[BORDER_RIGHT]);

    }

    if (!fHeightNotSet)
    {
        psize->cy = (!fMinMax || !PercentHeight()
                        ? uvHeight.YGetPixelValue(pci, pci->_sizeParent.cy,
                          GetFirstBranch()->GetFontHeightInTwips(&uvHeight))
                        : 0);
    }
    else
    {
        psize->cy = lBorderWY + 1;
    }

    sizeStyle = *psize;

    smModeSave = pci->_smMode;
    fMinmaxValidSave = _fMinMaxValid;
    pci->_smMode = SIZEMODE_MMWIDTH;

    CalcTextSize(pci, psize);
    psize->cy = _dp.GetHeight();

    if (psize->cy > sizeFontForLongStr.cy)
    {
        // should include border without default borders
        psize->cy = psize->cy + lBorderWY - pci->DocPixelsFromWindowY(2);
    }

    if (smModeSave != SIZEMODE_MMWIDTH)
    {
        _fMinMaxValid = fMinmaxValidSave;
        pci->_smMode = smModeSave;
        _fSizeThis = FALSE;
    }

    if (fWidthNotSet)
    {
        sizeStyle.cx = psize->cx;
    }

    if (fHeightNotSet)
    {
        sizeStyle.cy = psize->cy;
    }

    GetBtnHelper()->AdjustInsetForSize( pci,
                                        psize,
                                        &sizeStyle,
                                        &bInfo,
                                        &sizeFontForShortStr,
                                        GetDisplay()->GetCaret(),
                                        fWidthNotSet,
                                        fHeightNotSet,
                                        GetDisplay()->IsRTL());
    
    *psize = sizeStyle;

    return grfReturn;
}

//+------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::GetMinSize
//
//  Synopsis:   Get minimum size of the input file control
//
//              the min size of the input file controls should be
//              the default browse button size + 0 char wide input box
//
//-------------------------------------------------------------------------
void
CInputFileLayout::GetMinSize(SIZE * pSize, CCalcInfo * pci)
{
    pSize->cx = pSize->cy = 0;
    AdjustSizeForBorder(pSize, pci, TRUE);
}

//+-------------------------------------------------------------------------
//
//  Method:     CInputTextLayout::CalcSizeHelper
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CInputTextLayout::CalcSizeHelper(
    CCalcInfo * pci,
    SIZE *      psize)
{
    DWORD           grfReturn       = (pci->_grfLayout & LAYOUT_FORCE);
    CTreeNode *     pNode           = GetFirstBranch();
    CUnitValue      uvWidth         = pNode->GetCascadedwidth();
    CUnitValue      uvHeight        = pNode->GetCascadedheight();
    BOOL            fMinMax         = (     pci->_smMode == SIZEMODE_MMWIDTH
                                        ||  pci->_smMode == SIZEMODE_MINWIDTH );
    SIZE            sizeMin;
    BOOL            fWidthNotSet    = uvWidth.IsNullOrEnum();
    BOOL            fHeightNotSet   = uvHeight.IsNullOrEnum();

    GetMinSize(&sizeMin, pci);

    if (fWidthNotSet || fHeightNotSet)
    {
        long    rgPadding[PADDING_MAX];
        SIZE    sizeFontForShortStr;
        SIZE    sizeFontForLongStr;
        BOOL    fPrinting   = Doc()->IsPrintDoc();
        int     charX       = DYNCAST(CInput, ElementOwner())->GetAAsize();
        int     charY       = 1;
        
        Assert(charX > 0);

        GetDisplay()->GetPadding(pci, rgPadding, fMinMax);
        GetFontSize(pci, &sizeFontForShortStr, &sizeFontForLongStr);

        Assert(sizeFontForShortStr.cx && sizeFontForShortStr.cy && sizeFontForLongStr.cx && sizeFontForLongStr.cy);

        psize->cx = (charX -1) * sizeFontForLongStr.cx
                    + sizeFontForShortStr.cx
                    + rgPadding[PADDING_LEFT]
                    + rgPadding[PADDING_RIGHT]
                    + pci->DocPixelsFromWindowX(fPrinting ? 2:0);
        psize->cy = charY * sizeFontForLongStr.cy
                    + rgPadding[PADDING_TOP]
                    + rgPadding[PADDING_BOTTOM]
                    + pci->DocPixelsFromWindowY(fPrinting ? 2:0);

        // for textboxes, the border and scrollbars go outside
        AdjustSizeForBorder(psize, pci, TRUE);
    }

    if (!fWidthNotSet)
    {
        psize->cx = (!fMinMax || !PercentWidth()
                        ? uvWidth.XGetPixelValue(pci,
                                               pci->_sizeParent.cx,
                                               pNode->GetFontHeightInTwips(&uvWidth) )
                        : 0);
        if (psize->cx < sizeMin.cx)
        {
            psize->cx = sizeMin.cx;
        }
    }
    if (!fHeightNotSet)
    {
        psize->cy = (!fMinMax || !PercentHeight()
                        ? uvHeight.YGetPixelValue(pci,
                                        pci->_sizeParent.cy,
                                        pNode->GetFontHeightInTwips(&uvHeight))
                        : 0);
        if (psize->cy < sizeMin.cy)
        {
            psize->cy = sizeMin.cy;
        }
    }

    return grfReturn;
}


//+-------------------------------------------------------------------------
//
//  Method:     CInputLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CInputLayout::CalcSize(
    CCalcInfo * pci,
    SIZE *      psize,
    SIZE *      psizeDefault)
{
    Assert(ElementOwner());
    CSaveCalcInfo   sci(pci, this);
    CTreeNode *     pNode       = GetFirstBranch();
    CSize           sizeOriginal;
    DWORD           grfReturn;
    BOOL            fRecalcText = FALSE;
    BOOL            fNormalMode = (     pci->_smMode == SIZEMODE_NATURAL
                                    ||  pci->_smMode == SIZEMODE_FULLSIZE);
    BOOL            fWidthChanged;
    BOOL            fHeightChanged;
    htmlInput       typeInput   = GetType();
    BOOL            fIsButton   =   typeInput == htmlInputButton
                                ||  typeInput == htmlInputReset
                                ||  typeInput == htmlInputSubmit;
    CScopeFlag  csfCalcing(this);

    Listen();

    CElement::CLock   LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);

    if (pci->_grfLayout & LAYOUT_FORCE)
    {
        _fSizeThis         = TRUE;
        _fAutoBelow        = FALSE;
        _fPositionSet      = FALSE;
        _fContainsRelative = FALSE;
    }


    if (fIsButton)
    {
        fNormalMode = fNormalMode || pci->_smMode == SIZEMODE_SET;
    }

    fWidthChanged  = (fNormalMode
                                ? psize->cx != sizeOriginal.cx
                                : FALSE);
    fHeightChanged = (fNormalMode
                                ? psize->cy != sizeOriginal.cy
                                : FALSE);

    fRecalcText = (fNormalMode && (   IsDirty()
                                ||  _fSizeThis
                                ||  fWidthChanged
                                ||  fHeightChanged))
            ||  (pci->_grfLayout & LAYOUT_FORCE)
            ||  (pci->_smMode == SIZEMODE_MMWIDTH && !_fMinMaxValid)
            ||  (pci->_smMode == SIZEMODE_MINWIDTH && (!_fMinMaxValid || _xMin < 0));

    // If this site is in need of sizing, then size it
    if (fRecalcText)
    {
        CUnitValue  uvWidth =pNode->GetCascadedwidth();
        CUnitValue  uvHeight=pNode->GetCascadedheight();
        SIZE        sizeText;

        if (GetType() == htmlInputFile)
        {
            // calculate button size of input file
            IGNORE_HR(DYNCAST(CInputFileLayout, this)->ComputeInputFileButtonSize(pci));
        }

        //
        // If dirty, ensure display tree nodes exist
        //

        if (    _fSizeThis
            &&  fNormalMode
            &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
        {
            grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        }

        // input hidden should be invisible in browse mode
        Assert(GetType() != htmlInputHidden || IsEditable());

        switch (typeInput)
        {
        case htmlInputText:
        case htmlInputPassword:
        case htmlInputFile:
        case htmlInputHidden:
            grfReturn |= CalcSizeHelper(pci, psize);
            _fContentsAffectSize = FALSE;
            sizeText = *psize;
            grfReturn |= super::CalcSize(pci, &sizeText, psizeDefault);
            if (!fNormalMode && PercentWidth())
            {
                *psize = sizeText;
            }
            break;

        case htmlInputButton:
        case htmlInputSubmit:
        case htmlInputReset:
            grfReturn |= CalcSizeHelper(pci, psize);
            break;

        default:
            Assert(0 && "CalcSize: Illegal htmlInputType");
            psize->cx = psize->cy = 0;
            goto Cleanup;
        }

        if (psizeDefault)
        {
            *psizeDefault = *psize;
        }

        grfReturn |= LAYOUT_THIS  |
                    (psize->cx != sizeOriginal.cx
                            ? LAYOUT_HRESIZE
                            : 0) |
                    (psize->cy != sizeOriginal.cy
                            ? LAYOUT_VRESIZE
                            : 0);
        if ( _pDispNode && fIsButton)
        {
            GetElementDispNode()->SetInset(DYNCAST(CInputButtonLayout, this)->GetBtnHelper()->_sizeInset);
        }

        //
        // If size changes occurred, size the display nodes
        //

        if (    fNormalMode
            &&  _pDispNode
            &&  grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
        {
            CSize sizeContent(_dp.GetMaxWidth(), _dp.GetHeight());

            SizeDispNode(pci, *psize);

            // (paulnel) Make sure the max of the content cell has the borders
            // removed. Taking the full container size will cause scrolling on
            // selection.
            CRect rcContainer;
            GetClientRect(&rcContainer);
            sizeContent.Max(rcContainer.Size());
            SizeContentDispNode(sizeContent);

            if (HasRequestQueue())
            {
                long xParentWidth;
                long yParentHeight;

                _dp.GetViewWidthAndHeightForChild(
                    pci,
                    &xParentWidth,
                    &yParentHeight,
                    pci->_smMode == SIZEMODE_MMWIDTH);

                ProcessRequests(pci, CSize(xParentWidth, yParentHeight));
            }

            Reset(FALSE);
            Assert(!HasRequestQueue() || GetView()->HasLayoutTask(this));
        }

        if (pci->_smMode == SIZEMODE_MMWIDTH)
        {
            _xMax = psize->cx;
            _xMin = psize->cx;
            psize->cy = psize->cx;

            _fMinMaxValid = TRUE;
        }
        else if (pci->_smMode == SIZEMODE_MINWIDTH)
        {
            _xMin = psize->cx;
        }

    }
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }

Cleanup:
    return grfReturn;
}

LONG
CInputLayout::GetMaxLength()
{
    switch (GetType())
    {
    case htmlInputText:
    case htmlInputPassword:
        return DYNCAST(CInput, ElementOwner())->GetAAmaxLength();
    default:
        return super::GetMaxLength();
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::MeasureInputFileCaption
//
//  Synopsis:   Measure the button's caption for Input File.
//
//  Note:       Measuring needs to be done by ourselves as there's no underlying
//              TextSite for this faked button.
//
//----------------------------------------------------------------------------
HRESULT
CInputFileLayout::MeasureInputFileCaption(SIZE * psize, CCalcInfo * pci)
{
    HRESULT hr = S_OK;
    int i;
    long lWidth, lCharWidth;
    TCHAR * pch;
    CCcs * pccs;

    Assert(psize);
    Assert(GetType() == htmlInputFile);

    CInput          *pInputFile     = DYNCAST(CInput, ElementOwner());

    if ( -1 == pInputFile->_icfButton )
    {
        CCharFormat cf = *GetFirstBranch()->GetCharFormat();
        LOGFONT lf;
        LONG icf;

        DefaultFontInfoFromCodePage( g_cpDefault, &lf );

        cf.SetFaceName(lf.lfFaceName);

        cf._fBold = lf.lfWeight >= FW_BOLD;
        cf._fItalic = lf.lfItalic;
        cf._fUnderline = lf.lfUnderline;
        cf._fStrikeOut = lf.lfStrikeOut;

        cf._wWeight = (WORD)lf.lfWeight;

        cf._lcid = GetUserDefaultLCID();
        cf._bCharSet = lf.lfCharSet;
        cf._fNarrow = IsNarrowCharSet(lf.lfCharSet);
        cf._bPitchAndFamily = lf.lfPitchAndFamily;

        cf._bCrcFont = cf.ComputeFontCrc();
        cf._fHasDirtyInnerFormats = !!cf.AreInnerFormatsDirty();

        hr = TLS( _pCharFormatCache )->CacheData( & cf, & icf );

        if (hr)
            goto Cleanup;

        pInputFile->_icfButton = SHORT(icf);
    }

    pccs = fc().GetCcs(pci->_hdc, pci, GetCharFormatEx(pInputFile->_icfButton));

    if (!pccs)
    {
        hr = E_FAIL;
        goto Cleanup;
    }

    lWidth = 0;
    for ( i = _cchButtonCaption, pch = _pchButtonCaption;
          i > 0;
          i--, pch++ )
    {
        if ( ! pccs->Include(*pch, lCharWidth) )
        {
            Assert(0 && "Char not in font!");
        }

        lWidth += lCharWidth;
    }

    psize->cx = lWidth;
    psize->cy = pccs->GetBaseCcs()->_yHeight;

    pccs->Release();

Cleanup:
    RRETURN(hr);
}
void
CInputFileLayout::ComputeInputFileBorderInfo(CDocInfo *pdci, CBorderInfo & BorderInfo)
{
    CInput *    pInput  = DYNCAST(CInput, ElementOwner());
    CDocInfo    DocInfo;
    int         i;

    if (!pdci)
    {
        pdci = &DocInfo;
        pdci->Init(pInput);
    }

    pInput->_fRealBorderSize = TRUE;
    pInput->GetBorderInfo( pdci, &BorderInfo);
    pInput->_fRealBorderSize = FALSE;

    BorderInfo.rcSpace.right = 0;

    BorderInfo.aiWidths[BORDER_RIGHT] = pdci->DocPixelsFromWindowX(BorderInfo.aiWidths[BORDER_RIGHT], FALSE);
    BorderInfo.aiWidths[BORDER_LEFT] = pdci->DocPixelsFromWindowX(BorderInfo.aiWidths[BORDER_LEFT], FALSE);

    BorderInfo.aiWidths[BORDER_TOP] = pdci->DocPixelsFromWindowY(BorderInfo.aiWidths[BORDER_TOP], FALSE);
    BorderInfo.aiWidths[BORDER_BOTTOM] = pdci->DocPixelsFromWindowY(BorderInfo.aiWidths[BORDER_BOTTOM], FALSE);

    Assert(BORDER_TOP < BORDER_RIGHT);
    Assert(BORDER_RIGHT < BORDER_LEFT);
    Assert(BORDER_TOP < BORDER_BOTTOM);
    Assert(BORDER_BOTTOM < BORDER_LEFT);

    for (i = BORDER_TOP; i <= BORDER_LEFT; i ++)
    {
        if (BorderInfo.abStyles[i] != fmBorderStyleSunken)
            continue;
        if (!BTN_PRESSED(pInput->_wBtnStatus))
        {
            BorderInfo.abStyles[i]= fmBorderStyleRaised;
        }
    }

    BorderInfo.wEdges = BF_RECT | BF_SOFT;

    GetBorderColorInfoHelper( GetFirstBranch(), pdci, &BorderInfo);    // Need to pick up the colors, etc.
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::ComputeButtonSize
//
//  Synopsis:   Measure the button's caption.
//
//  Note:       Measuring needs to be done by ourselves as there's no underlying
//              TextSite for this faked button.
//
//----------------------------------------------------------------------------
HRESULT
CInputFileLayout::ComputeInputFileButtonSize(CCalcInfo * pci)
{
    HRESULT hr = S_OK;
    SIZE        sizeText;
    CBorderInfo bInfo;
    int         uitotalBXWidth;

    int         ui1LogicalPixelX    = pci->DocPixelsFromWindowX(1);
                // default horizontal offset size is 4 logical pixels
    int         uiMinInsetH = ui1LogicalPixelX * 4;

    CInput *    pInput = DYNCAST(CInput, ElementOwner());

    hr = THR(MeasureInputFileCaption(&sizeText, pci));
    if ( hr )
        goto Error;

    pInput->_fRealBorderSize = TRUE;
    pInput->GetBorderInfo(pci, &bInfo, FALSE);
    pInput->_fRealBorderSize = FALSE;

    uitotalBXWidth = bInfo.aiWidths[BORDER_RIGHT] + bInfo.aiWidths[BORDER_LEFT];
    uitotalBXWidth = pci->DocPixelsFromWindowX(uitotalBXWidth);

    _xCaptionOffset = sizeText.cx / 2 - uitotalBXWidth;

    // we should have a min offset
    if (_xCaptionOffset < uiMinInsetH)
    {
        _xCaptionOffset = uiMinInsetH;
    }

    _sizeButton.cx = sizeText.cx + _xCaptionOffset + uitotalBXWidth;

    // only remember the text height of the button
    _sizeButton.cy = sizeText.cy;;

    _xCaptionOffset = _xCaptionOffset >> 1;

Cleanup:
    RRETURN(hr);

Error:
    hr = E_FAIL;
    goto Cleanup;
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::RenderInputFileButtonBorder
//
//  Synopsis:   Render.
//
//  Note:       We draw the fake button border here
//
//----------------------------------------------------------------------------

void
CInputFileLayout::RenderInputFileButtonBorder(CFormDrawInfo *pDI, UINT uFlag)
{
    SIZE sizeClient;
    CBorderInfo bi;
    RECT rcButton;
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);

    ComputeInputFileBorderInfo(pDI, bi);

    GetSize(&sizeClient);

    if(!fRightToLeft)
    {
        rcButton.left = sizeClient.cx - _sizeButton.cx;
        rcButton.right = sizeClient.cx;
    }
    else
    {
        // when RTL the button goes on the left the the text input
        rcButton.left = 0;
        rcButton.right = _sizeButton.cx;
    }

    rcButton.top = 0;
    rcButton.bottom = sizeClient.cy;
    //  Draw the borders
    ::DrawBorder(pDI, &rcButton, &bi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::RenderInputFileButtonContent
//
//  Synopsis:   Render.
//
//  Note:       We draw the fake button image here
//
//----------------------------------------------------------------------------

void
CInputFileLayout::RenderInputFileButtonCaption(CFormDrawInfo *pDI)
{
    CBorderInfo BorderInfo;
    RECT rcClip;
    RECT rcButton;
    SIZE sizeClient;
#ifdef WIN16
    GDIRECT rc, *pRect;
    pRect = &rc;
#else
    RECT *pRect = &rcClip;
#endif
    HDC hdc = pDI->GetDC(TRUE);
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);

    CInput          *pInputOuter = DYNCAST(CInput, ElementOwner());

    long lOffsetX, lOffsetY;
    CCcs * pccs;
    DWORD dwDCObjType       = GetObjectType(hdc);
    const CCharFormat * pcf = GetCharFormatEx( pInputOuter->_icfButton );

    pccs = fc().GetCcs( hdc, pDI, pcf );
    if (!pccs)
        return;

    ComputeInputFileBorderInfo(pDI, BorderInfo);    // Need to pick up the colors, etc.

    //  Render the caption text
    SetBkColor  (hdc, GetSysColorQuick(COLOR_BTNFACE));
    SetTextColor(hdc, GetSysColorQuick(COLOR_BTNTEXT));

    SelectFontEx(hdc, pccs->GetBaseCcs()->_hfont);

    GetSize(&sizeClient);

    if(!fRightToLeft)
    {
        rcButton.left = sizeClient.cx - _sizeButton.cx
                        + BorderInfo.aiWidths[BORDER_LEFT];
        rcButton.right = sizeClient.cx - BorderInfo.aiWidths[BORDER_RIGHT];
    }
    else
    {
        rcButton.left = BorderInfo.aiWidths[BORDER_LEFT];
        rcButton.right = rcButton.left + _sizeButton.cx + BorderInfo.aiWidths[BORDER_RIGHT];
    }

    rcButton.top = BorderInfo.aiWidths[BORDER_TOP];
    rcButton.bottom = sizeClient.cy - BorderInfo.aiWidths[BORDER_BOTTOM];

    lOffsetX = _xCaptionOffset
             + pDI->DocPixelsFromWindowX(BTN_PRESSED(pInputOuter->_wBtnStatus) ?
                                                        1 : 0, FALSE);

    lOffsetY = max (0L, (LONG)((rcButton.bottom - rcButton.top - _sizeButton.cy) / 2))
             + pDI->DocPixelsFromWindowY(BTN_PRESSED(pInputOuter->_wBtnStatus) ?
                                                        1 : 0, FALSE);
    IntersectRect(&rcClip, &rcButton, pDI->ClipRect());
#ifdef WIN16
    CopyRect(pRect, &rcClip);
#endif

    // fix for printing
    if (Doc() && Doc()->IsPrintDoc() && Doc()->PaintBackground() &&
        (dwDCObjType == OBJ_ENHMETADC || dwDCObjType == OBJ_METADC))
    {
        BitBlt(hdc,
                    rcButton.left,
                    rcButton.top,
                    rcButton.right  - rcButton.left,
                    rcButton.bottom - rcButton.top,
                    hdc, 0, 0, WHITENESS);
        PatBltBrush(hdc,
                    &rcButton, PATCOPY,
                    GetSysColorQuick(COLOR_BTNFACE));
    }

    VanillaTextOut( pccs,
                    hdc,
                    rcButton.left + lOffsetX,
                    rcButton.top + lOffsetY,
                    ETO_OPAQUE | ETO_CLIPPED,
                    pRect,
                    _pchButtonCaption ? _pchButtonCaption : g_Zero.ach,
                    _cchButtonCaption,
                    g_cpDefault,
                    NULL);

    pccs->Release();
}


void CInputFileLayout::DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
    CBorderInfo     bi;
    RECT            rc, rcButton;
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);

    DYNCAST(CInput, ElementOwner())->_fRealBorderSize = TRUE;
    Verify(ElementOwner()->GetBorderInfo(pDI, &bi, TRUE));
    DYNCAST(CInput, ElementOwner())->_fRealBorderSize = FALSE;

    bi.aiWidths[BORDER_TOP]    = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_TOP], FALSE);
    bi.aiWidths[BORDER_RIGHT]  = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_RIGHT], FALSE);
    bi.aiWidths[BORDER_BOTTOM] = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM], FALSE);
    bi.aiWidths[BORDER_LEFT]   = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT], FALSE);

    rc = rcButton = *prcBounds;
    
    if(!fRightToLeft)
    {
        rc.right = rc.right - _sizeButton.cx 
                            - pDI->DocPixelsFromWindowX(CInput::cxButtonSpacing, FALSE); 
        rcButton.left = rc.right;
    }
    else
    {
        rc.left = rc.left + _sizeButton.cx 
                            + pDI->DocPixelsFromWindowX(CInput::cxButtonSpacing, FALSE); 
        rcButton.right = rc.left;
    }

    ::DrawBorder(pDI, &rc, &bi);

    RenderInputFileButtonCaption(pDI);
    RenderInputFileButtonBorder(pDI);

    // We only want to paint selection on the button rect in this part
    // The client portion of the selection is handled in the DrawClient
    // method of this class. This way any text in the client will be
    // correctly painted with the selection.
    if (_fTextSelected)
    {
        DrawTextSelectionForRect(pDI->GetDC(), (CRect *)& rcButton ,& pDI->_rcClip , _fSwapColor);
    }


}

//+---------------------------------------------------------------------------
//
//  Member:     CInputFileLayout::DrawClient
//
//  Synopsis:   Draw client rect part of the controls
//
//  Arguments:  prcBounds       bounding rect of display leaf node
//              prcRedraw       rect to be redrawn
//              pSurface        surface to render into
//              pDispNode       pointer to display node
//              pClientData     client-dependent data for drawing pass
//              dwFlags         flags for optimization
//
//----------------------------------------------------------------------------

void
CInputFileLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;

    {
        // we set draw surface information separately for Draw() and
        // the stuff below, because the Draw method of some subclasses
        // (like CFlowLayout) puts pDI into a special device coordinate
        // mode
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);
        Draw(pDI);
    }

    {
        // see comment above
        CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

        // We only want to paint selection on the client rect in this part
        // The button portion of the selection is handled in the DrawClientBorder
        // method of this class
        if (_fTextSelected)
        {
            DrawTextSelectionForRect(pDI->GetDC(), (CRect *)prcRedraw ,& pDI->_rcClip , _fSwapColor);
        }

        if ( GetView()->IsFlagSet(CView::VF_ZEROBORDER) )
        {
            CLayout* pParentLayout = GetUpdatedParentLayout();
            if ( pParentLayout && pParentLayout->ElementOwner()->IsEditable() )
            {
                DrawZeroBorder(pDI);
            }
        }
    }
}

void
CInputFileLayout::GetButtonRect(RECT *prc)
{
    CTreeNode * pNodeLayout  = GetFirstBranch();
    const CParaFormat *  pPF = pNodeLayout->GetParaFormat();
    BOOL fRightToLeft = pPF->HasRTL(FALSE);

    Assert(prc);
    GetRect(prc, COORDSYS_CONTAINER);
    if(!fRightToLeft)
        prc->left = prc->right - _sizeButton.cx;
    else
        prc->right = prc->left + _sizeButton.cx;
}

//-----------------------------------------------------------------------------
//
//  Member:     Notify
//
//  Synopsis:   Respond to a tree notification
//
//  Arguments:  pnf - Pointer to the tree notification
//
//-----------------------------------------------------------------------------
void
CInputFileLayout::Notify(CNotification * pnf)
{
    if (pnf->IsTextChange())
    {
        if (Doc()->IsInScript())
        {
            DYNCAST(CInput, ElementOwner())->_fDirtiedByOM = TRUE;
        }
    }
    super::Notify(pnf);
}

