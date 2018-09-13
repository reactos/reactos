//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1998
//
//  File:       tarealyt.cxx
//
//  Contents:   Implementation of layout class for <RICHTEXT> <TEXTAREA> controls.
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

#ifndef X_TAREALYT_HXX_
#define X_TAREALYT_HXX_
#include "tarealyt.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
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

#ifndef X_TEXTAREA_HXX_
#define X_TEXTAREA_HXX_
#include "textarea.hxx"
#endif

#ifndef X__FONT_H_
#define X__FONT_H_
#include "_font.h"
#endif

#ifndef X_SHAPE_HXX_
#define X_SHAPE_HXX_
#include "shape.hxx"
#endif

MtDefine(CTextAreaLayout, Layout, "CTextAreaLayout")
MtDefine(CRichtextLayout, Layout, "CRichtextLayout")

extern void DrawTextSelectionForRect(HDC hdc, CRect *prc, CRect *prcClip, BOOL fSwapColor);

const CLayout::LAYOUTDESC CRichtextLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

HRESULT
CRichtextLayout::GetFontSize(CCalcInfo * pci, SIZE * psizeFontForShortStr, SIZE * psizeFontForLongStr)
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

//+-------------------------------------------------------------------------
//
//  Method:     CRichtextLayout::CalcSizeHelper
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CRichtextLayout::CalcSizeHelper(
    CCalcInfo * pci,
    int         charX,
    int         charY,
    SIZE *      psize)
{
    CSize       sizeOriginal;
    DWORD       grfReturn;
    CElement *  pInput  = ElementOwner();

    GetSize(&sizeOriginal);

    //BUGBUG (gideons) should modify Cols and Rows if user resizes
    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);

    CTreeNode *     pTreeNode = pInput->GetFirstBranch();
    CUnitValue      uvWidth   = pTreeNode->GetCascadedwidth();
    CUnitValue      uvHeight  = pTreeNode->GetCascadedheight();
    BOOL            fMinMax   = (   pci->_smMode == SIZEMODE_MMWIDTH
                                ||  pci->_smMode == SIZEMODE_MINWIDTH );

    if (uvWidth.IsNullOrEnum() || uvHeight.IsNullOrEnum())
    {
        SIZE            sizeFontForShortStr;
        SIZE            sizeFontForLongStr;
        BOOL            fIsPrinting = Doc()->IsPrintDoc();
        styleOverflow   overflow;
        extern SIZE     g_sizeScrollbar;

        GetFontSize(pci, &sizeFontForShortStr, &sizeFontForLongStr);
        Assert(sizeFontForShortStr.cx && sizeFontForShortStr.cy && sizeFontForLongStr.cx && sizeFontForLongStr.cy);

// BUGBUG: The TEXT_INSET_DEFAULT_xxxx values are really padding and should be handled as such (brendand)
        psize->cx = charX * sizeFontForLongStr.cx
                    + pci->DocPixelsFromWindowX(TEXT_INSET_DEFAULT_LEFT
                                            + TEXT_INSET_DEFAULT_RIGHT
                                            + (fIsPrinting ? 2 : GetDisplay()->GetCaret()));
        psize->cy = charY * sizeFontForLongStr.cy
                    + pci->DocPixelsFromWindowY(TEXT_INSET_DEFAULT_TOP
                                            + TEXT_INSET_DEFAULT_BOTTOM
                                            + (fIsPrinting ? 2 : 0));

// BUGBUG: Add this space in another way? (brendand)
        overflow = pTreeNode->GetCascadedoverflowY();
        if (overflow != styleOverflowHidden)
        {
            psize->cx += g_sizeScrollbar.cx;
            if (!IsWrapSet())
            {
                psize->cy += g_sizeScrollbar.cy;
            }
        }
        AdjustSizeForBorder(psize, pci, TRUE);
    }

    if (!uvWidth.IsNullOrEnum())
    {
        psize->cx = (!fMinMax || !PercentWidth() ?
                        uvWidth.XGetPixelValue(pci,
                                               pci->_sizeParent.cx,
                                               pInput->GetFirstBranch()->GetFontHeightInTwips(&uvWidth) )
                        : 0);
    }

    if (!uvHeight.IsNullOrEnum())
    {
        psize->cy = (pci->_smMode == SIZEMODE_MMWIDTH
                            ? psize->cx
                            : (!fMinMax || !PercentHeight()
                                    ? uvHeight.YGetPixelValue(pci,
                                                    pci->_sizeParent.cy,
                                                    pInput->GetFirstBranch()->GetFontHeightInTwips(&uvHeight) )
                                    : 0));
    }
    if (pci->_smMode == SIZEMODE_MMWIDTH)
    {
        _xMax = psize->cx;
        _xMin = psize->cx;

        _fMinMaxValid = TRUE;
    }
    else if (pci->_smMode == SIZEMODE_MINWIDTH)
    {
        _xMin = psize->cx;
    }

    return grfReturn;
}

HRESULT
CRichtextLayout::Init()
{
    HRESULT hr = S_OK;

    hr = super::Init();

    if(hr)
        goto Cleanup;

    _fAllowSelectionInDialog = TRUE;

Cleanup:
    RRETURN(hr);
}

void
CRichtextLayout::SetWrap()
{
    BOOL    fWrap = IsWrapSet();

    GetDisplay()->SetWordWrap(fWrap);
    GetDisplay()->SetWrapLongLines(fWrap);
}

BOOL
CRichtextLayout::IsWrapSet()
{
    return (DYNCAST(CRichtext, ElementOwner())->GetAAwrap() != htmlWrapOff);
}

DWORD
CRichtextLayout::CalcSize( CCalcInfo * pci,
                           SIZE *      psize,
                           SIZE *      psizeDefault)
{
    Assert(ElementOwner());
    Assert(pci);
    Assert(psize);

    CScopeFlag      csfCalcing(this);
    CRichtext     * pTextarea = DYNCAST(CRichtext, ElementOwner());

    Listen();

    CSaveCalcInfo   sci(pci, this);

    CSize   sizeOriginal;
    DWORD   grfReturn;

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
    BOOL fWidthChanged  = (fNormalMode
                                ? psize->cx != sizeOriginal.cx
                                : FALSE);
    BOOL fHeightChanged = (fNormalMode
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
        //BUGBUG (gideons) should modify Cols and Rows if user resizes

        CUnitValue  uvWidth  = pTextarea->GetFirstBranch()->GetCascadedwidth();
        CUnitValue  uvHeight = pTextarea->GetFirstBranch()->GetCascadedheight();
        int         charX = 1;
        int         charY = 1;

        //
        // If dirty, ensure display tree nodes exist
        //

        if (    _fSizeThis
            &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
        {
            grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
        }

        SetWrap();

        _fContentsAffectSize = FALSE;

        if ( uvWidth.IsNullOrEnum() || uvHeight.IsNullOrEnum()  )
        {
            charX = pTextarea->GetAAcols();
            Assert(charX > 0);
            charY = pTextarea->GetAArows();
            Assert(charY > 0);
        }

        grfReturn |= CalcSizeHelper(pci, charX, charY, psize);

        if (fNormalMode)
        {
            grfReturn |= super::CalcSize(pci, psize);
        }

        if (psizeDefault)
        {
            *psizeDefault = *psize;
        }
        
        //
        // If size changes occurred, size the display nodes
        //

        if (    fNormalMode
            &&  grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
        {
            SizeDispNode(pci, *psize);
            SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
        }

        if(pci->_smMode == SIZEMODE_MMWIDTH)
        {
            // for minmax mode min width is returned in cy.
            _xMin = psize->cy = psize->cx;
        }
    }
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }

    return grfReturn;
}

HRESULT
CRichtextLayout::OnTextChange(void)
{
    CRichtext *     pInput  = DYNCAST(CRichtext, ElementOwner());

    if (!pInput->IsEditable(TRUE))
        pInput->_fTextChanged = TRUE;

    if (pInput->_fFiredValuePropChange)
    {
        pInput->_fFiredValuePropChange = FALSE;
    }
    else
    {
        pInput->OnPropertyChange(DISPID_CRichtext_value, 0); // value change
    }
    return S_OK;
}

// JS OnSelection event
HRESULT
CRichtextLayout::OnSelectionChange(void)
{
    DYNCAST(CRichtext, ElementOwner())->Fire_onselect();  //JS event
    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CRichtextLayout::DrawClient
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
CRichtextLayout::DrawClient(
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
        // In RTL the scrollbar is on the left and will leave extra highlighting
        // on the right side of the control if we do not adjust it here
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

