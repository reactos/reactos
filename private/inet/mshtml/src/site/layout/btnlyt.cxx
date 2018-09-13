//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       btnlyt.cxx
//
//  Contents:   Implementation of layout class for <BUTTON> controls.
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

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif

#ifndef X__FONTLNK_H_
#define X__FONTLNK_H_
#include "_fontlnk.h"
#endif

#ifndef X_INPUTBTN_HXX_
#define X_INPUTBTN_HXX_
#include "inputbtn.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

#ifndef X_BTNLYT_HXX_
#define X_BTNLYT_HXX_
#include "btnlyt.hxx"
#endif

#ifndef X_DISPNODE_HXX_
#define X_DISPNODE_HXX_
#include "dispnode.hxx"
#endif


MtDefine(CButtonLayout, Layout, "CButtonLayout")

const CLayout::LAYOUTDESC CButtonLayout::s_layoutdesc =
{
    LAYOUTDESC_NOSCROLLBARS     |
    LAYOUTDESC_HASINSETS        |
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

HRESULT
CButtonLayout::GetFontSize(CCalcInfo * pci, SIZE * psizeFontForShortStr, SIZE * psizeFontForLongStr)
{
    HRESULT hr = S_OK;
    CCcs    * pccs;
    CBaseCcs *pBaseCcs;
    const CCharFormat *pCF;

    Assert(ElementOwner());

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

#define BUTTON_INSET_MIN 1

void
CBtnHelper::AdjustInsetForSize(
    CCalcInfo *     pci,
    SIZE *          psizeText, // size of the text
    SIZE *          psize,     // button size
    CBorderInfo *   pbInfo,
    SIZE *          psizeFontForShortStr,
    LONG            lCaret,
    BOOL            fWidthNotSet,
    BOOL            fHeightNotSet,
    BOOL            fRightToLeft
    )
{
    int ui1LogicalPixelX    = pci->DocPixelsFromWindowX(1);
    int ui1LogicalPixelY    = pci->DocPixelsFromWindowY(1);

    SIZE    sizeText;
    int     uiMinInsetH = ui1LogicalPixelX * 2; //4 pixels
    int     uiMinInsetV = ui1LogicalPixelY * 4;
    int     iTotalInset;

    // the Caret is added in Design mode
    // we only substract 3 pixels border size at each side which is Netscape
    // border size + default border (IE specific)
    int     uitotalBXWidth = pci->DocPixelsFromWindowX(6 + lCaret);

    // Cache border sizes
    LONG    lBorderX = pci->DocPixelsFromWindowX(pbInfo->aiWidths[BORDER_RIGHT]
                                               + pbInfo->aiWidths[BORDER_LEFT]);
    LONG    lBorderY = pci->DocPixelsFromWindowX(pbInfo->aiWidths[BORDER_BOTTOM]
                                               + pbInfo->aiWidths[BORDER_TOP]);

    CSize &     sizeInset     = _sizeInset;

    sizeInset = g_Zero.size;

    sizeText.cx = psize->cx - lBorderX;
    sizeText.cy = psize->cy - lBorderY;

    if (fWidthNotSet)
    {
        // Netscape total inset = text pixel length/2 - borders
        // Netscape sets inset to short font width + 1 when string is empty
        // Also, due to rounding problem, sizeText might be little bit bigger than it should be
        // we should accept some 7% mistake.
        iTotalInset = (sizeText.cx > (uitotalBXWidth >> 4))
                            ? (sizeText.cx >> 1)
                            : (psizeFontForShortStr->cx + 1);
        iTotalInset = iTotalInset - uitotalBXWidth; // only removes the 4 pixels (netscape)

        if (iTotalInset < uiMinInsetH)
        {
            // Netscape has negative inset when the text length is less than 4
            // we keep the inset to 2 to avoid hiding the chars.
            // inset to 2, because we have to think about default button
            // and focus rect
            iTotalInset = uiMinInsetH;
        }
        sizeInset.cx   = iTotalInset >> 1;

        // remove 2 thick border pixels, if this button is the default button
        psize->cx = sizeText.cx + iTotalInset + lBorderX;
    }
    else
    {
        sizeInset.cx = max((INT) BUTTON_INSET_MIN,
                            (INT)    (psize->cx - psizeText->cx)/2);
    }

    if(fRightToLeft)
        sizeInset.cx = - sizeInset.cx;


    if (fHeightNotSet)
    {
        // calc the Y - total inset = font height/2 - borders
        // we only substract 2 pixels border size which is Netscape
        // border size.

        iTotalInset = (psizeFontForShortStr->cy >> 1) - (ui1LogicalPixelY * 4);

        if (iTotalInset < uiMinInsetV)
        {
            iTotalInset = uiMinInsetV;
        }

        sizeInset.cy = (iTotalInset >> 1) - ui1LogicalPixelY * 2;

        // the text height should not be smaller than the font
        sizeText.cy = max(sizeText.cy, psizeFontForShortStr->cy);

        // remove 2 thick border pixels, if this button is the default button
        // We don't remove these pixels directly from insets, because
        // Draw is going to do this.
        psize->cy = sizeText.cy + iTotalInset + lBorderY - ui1LogicalPixelY * 2;
    }
    else
    {
        sizeInset.cy = max((INT) BUTTON_INSET_MIN,
                            (INT) (psize->cy - psizeText->cy - lBorderY)/2);
    }
}


void
CButtonLayout::DrawClient(
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
                       dwFlags );

    // (bug 49150) Has the button just appeared? Should it be the default element
    CButton * pButton = DYNCAST(CButton, ElementOwner());
    const CCharFormat *pCF = GetFirstBranch()->GetCharFormat();
    Assert(pButton && pCF);

    if (pButton->GetBtnWasHidden() && pButton->GetAAtype() == htmlInputSubmit
        && !pCF->IsDisplayNone() && !pCF->IsVisibilityHidden())
    {
        pButton->SetDefaultElem();
        pButton->SetBtnWasHidden( FALSE );
    }
}

void CButtonLayout::DrawClientBorder(
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
    BOOL            fDefaultAndCurrent =    pDoc
                                        &&  ElementOwner()->_fDefault
                                        &&  ElementOwner()->IsEnabled()
                                        &&  pDoc->HasFocus();

    Verify(ElementOwner()->GetBorderInfo(pDI, &bi, TRUE));

    // draw default if necessary
    bi.acrColors[BORDER_TOP][1]    =
    bi.acrColors[BORDER_RIGHT][1]  =
    bi.acrColors[BORDER_BOTTOM][1] =
    bi.acrColors[BORDER_LEFT][1]   = fDefaultAndCurrent
                                            ? RGB(0,0,0)
                                            : ElementOwner()->GetInheritedBackgroundColor();
    bi.xyFlat = pDI->DocPixelsFromWindowX(fDefaultAndCurrent
                                                ? -1
                                                : 1,
                                          FALSE);

    bi.aiWidths[BORDER_TOP]    = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_TOP], FALSE);
    bi.aiWidths[BORDER_RIGHT]  = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_RIGHT], FALSE);
    bi.aiWidths[BORDER_BOTTOM] = pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM], FALSE);
    bi.aiWidths[BORDER_LEFT]   = pDI->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT], FALSE);
    ::DrawBorder(pDI, (RECT *)prcBounds, &bi);
}

//+---------------------------------------------------------------------------
//
//  Member:     CButtonLayout::CalcSize
//
//
//----------------------------------------------------------------------------

DWORD
CButtonLayout::CalcSize( CCalcInfo * pci,
                         SIZE      * psize,
                         SIZE      * psizeDefault)
{
    CSaveCalcInfo   sci(pci, this);
    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSize           sizeOriginal;
    DWORD           grfReturn;

    Assert(pci);
    Assert(psize);

    Listen();

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

    BOOL fRecalcText = FALSE;
    BOOL fNormalMode = pci->_smMode == SIZEMODE_NATURAL ||
                       pci->_smMode == SIZEMODE_SET     ||
                       pci->_smMode == SIZEMODE_FULLSIZE;
    BOOL fMinMax     = pci->_smMode == SIZEMODE_MMWIDTH || 
                       pci->_smMode == SIZEMODE_MINWIDTH;
    BOOL fWidthChanged  = (fNormalMode
                                ? psize->cx != sizeOriginal.cx
                                : FALSE);
    BOOL fHeightChanged = (fNormalMode
                                ? psize->cy != sizeOriginal.cy
                                : FALSE);

#ifdef  NEVER
    if (fNormalMode && !_fSizeThis)
    {
        *psize = sizeOriginal;
        return grfReturn;
    }
#endif

    fRecalcText = (fNormalMode && (   IsDirty()
                                ||  _fSizeThis
                                ||  fWidthChanged
                                ||  fHeightChanged))
            ||  (pci->_grfLayout & LAYOUT_FORCE)
            ||  (pci->_smMode == SIZEMODE_MMWIDTH && !_fMinMaxValid)
            ||  (pci->_smMode == SIZEMODE_MINWIDTH && (!_fMinMaxValid || _xMin < 0));

    if (fRecalcText)
    {
        CUnitValue  uvWidth         = ElementOwner()->GetFirstBranch()->GetCascadedwidth();
        CUnitValue  uvHeight        = ElementOwner()->GetFirstBranch()->GetCascadedheight();
        BOOL        fWidthNotSet    = uvWidth.IsNullOrEnum();
        BOOL        fHeightNotSet   = uvHeight.IsNullOrEnum();
        SIZE        sizeStyle;
        CBorderInfo bInfo;
        SIZE        sizeFontForShortStr, sizeFontForLongStr;

        _fContentsAffectSize = TRUE; //fWidthNotSet || fHeightNotSet;

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
            // the width of the button should be minimum size of the font width
            // for IE4 compat
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
            psize->cy = 1 + bInfo.aiWidths[BORDER_TOP]
                          + bInfo.aiWidths[BORDER_BOTTOM];
            psize->cy = pci->DocPixelsFromWindowY(psize->cy);
        }

        sizeStyle = *psize;

        grfReturn |= super::CalcSize(pci, psize, psizeDefault);

        if (fWidthNotSet)
        {
            sizeStyle.cx = psize->cx;
        }

        if (fHeightNotSet)
        {
            sizeStyle.cy = psize->cy;
        }

        psize->cx = _dp.GetWidth();
        psize->cy = _dp.GetHeight();

        GetBtnHelper()->AdjustInsetForSize( pci,
                                            psize,
                                            &sizeStyle,
                                            &bInfo,
                                            &sizeFontForShortStr,
                                            GetDisplay()->GetCaret(),
                                            fWidthNotSet,
                                            fHeightNotSet,
                                            GetDisplay()->IsRTL());
        if (!fWidthNotSet)
            GetBtnHelper()->_sizeInset.cx = 0;

        // PERF

        *psize = sizeStyle;

        grfReturn |= LAYOUT_THIS  |
                    (psize->cx != sizeOriginal.cx
                            ? LAYOUT_HRESIZE
                            : 0) |
                    (psize->cy != sizeOriginal.cy
                            ? LAYOUT_VRESIZE
                            : 0);

        switch(pci->_smMode)
        {
        case    SIZEMODE_NATURAL:
        case    SIZEMODE_SET:
        case    SIZEMODE_FULLSIZE:

            Assert(_pDispNode);
            if (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
            {
                SizeDispNode(pci, *psize);
                SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
            }
            _pDispNode->SetInset(GetBtnHelper()->_sizeInset);
            if (fWidthNotSet)
            {
                CRect rc(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
                unsigned    fSizeToContentSave = _fSizeToContent;
    
                _fSizeToContent = TRUE;
                _dp.SetViewSize(rc);
                _dp.RecalcLineShift(pci, 0);
                _fSizeToContent = fSizeToContentSave;
            }
            _fSizeThis = FALSE;
            break;
        case SIZEMODE_MMWIDTH:
            _xMax = psize->cx;
            _xMin = psize->cx;

            psize->cy = psize->cx;
            _fMinMaxValid = TRUE;
            break;
        case SIZEMODE_MINWIDTH:
            _xMin = psize->cx;
            break;
        }
    }
    // Otherwise, defer to default handling
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }

    return grfReturn;
}

//+---------------------------------------------------------------------------
//
//  Member:     CButtonLayout::GetFocusShape
//
//  Synopsis:   Returns the shape of the focus outline that needs to be drawn
//              when this element has focus. This function creates a new
//              CShape-derived object. It is the caller's responsibility to
//              release it.
//
//----------------------------------------------------------------------------

HRESULT
CButtonLayout::GetFocusShape(long lSubDivision, CDocInfo * pdci, CShape ** ppShape)
{
    CBorderInfo     bi;
    CRect           rc;
    CRectShape *    pShape;
    HRESULT         hr = S_OK;

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
    RRETURN(hr);
}

void
CButtonLayout::DoLayout(
    DWORD   grfLayout)
{
    super::DoLayout(grfLayout);
    if(     !IsDisplayNone()
        &&  (grfLayout & LAYOUT_MEASURE)
        )
    {
        GetElementDispNode()->SetInset(GetBtnHelper()->_sizeInset);
    }
}

CBtnHelper * CButtonLayout::GetBtnHelper()
{
    CElement * pElement = ElementOwner();
    Assert(pElement);
    CButton * pButton = DYNCAST(CButton, pElement);
    return pButton->GetBtnHelper();
}

