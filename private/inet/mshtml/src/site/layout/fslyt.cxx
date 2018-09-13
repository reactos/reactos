//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       fslyt.cxx
//
//  Contents:   Implementation of CLegendLayout, CFieldSetLayout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_FSLYT_HXX_
#define X_FSLYT_HXX_
#include "fslyt.hxx"
#endif

#ifndef X_E1D_HXX_
#define X_E1D_HXX_
#include "e1d.hxx"
#endif

#ifndef X_DISPTREE_H_
#define X_DISPTREE_H_
#pragma INCMSG("--- Beg <disptree.h>")
#include <disptree.h>
#pragma INCMSG("--- End <disptree.h>")
#endif

#ifndef X_DISPTYPE_HXX_
#define X_DISPTYPE_HXX_
#include "disptype.hxx"
#endif

#ifndef X_DISPROOT_HXX_
#define X_DISPROOT_HXX_
#include "disproot.hxx"
#endif


MtDefine(CLegendLayout, Layout, "CLegendLayout")
MtDefine(CFieldSetLayout, Layout, "CFieldSetLayout")

IMPLEMENT_LAYOUT_FNS(CLegendElement, CLegendLayout)
IMPLEMENT_LAYOUT_FNS(CFieldSetElement, CFieldSetLayout)

const CLayout::LAYOUTDESC CLegendLayout::s_layoutdesc =
{
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

HRESULT
CLegendLayout::Init()
{
    HRESULT hr = super::Init();

    if(hr)
        goto Cleanup;

    GetDisplay()->SetWordWrap(FALSE);

Cleanup:
    RRETURN(hr);
}

DWORD
CLegendLayout::CalcSize(CCalcInfo * pci, SIZE *psize, SIZE *psizeDefault)
{
    Assert(ElementOwner());
    CScopeFlag      csfCalcing(this);
    CElement::CLock LockS(ElementOwner(), CElement::ELEMENTLOCK_SIZING);
    CSaveCalcInfo   sci(pci, this);
    CSize           sizeOriginal;


    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }

    BOOL    fNormalMode    =    pci->_smMode == SIZEMODE_NATURAL
                            ||  pci->_smMode == SIZEMODE_SET
                            ||  pci->_smMode == SIZEMODE_FULLSIZE;
    BOOL    fWidthChanged  =    (fNormalMode
                                    ? psize->cx != sizeOriginal.cx
                                    : FALSE);
    BOOL    fHeightChanged =    (fNormalMode
                                    ? psize->cy != sizeOriginal.cy
                                    : FALSE);
    BOOL    fRecalcText    = FALSE;
    DWORD   grfReturn      = pci->_grfLayout & LAYOUT_FORCE;

    _fSizeThis = (_fSizeThis || (pci->_grfLayout & LAYOUT_FORCE));

    fRecalcText =   (   fNormalMode
                    &&  _fSizeThis)
                ||  fWidthChanged
                ||  fHeightChanged
                ||  !IsCommitted()
                ||  (   pci->_smMode == SIZEMODE_MMWIDTH
                    &&  !_fMinMaxValid)
                ||  (   pci->_smMode == SIZEMODE_MINWIDTH
                    &&  (   !_fMinMaxValid
                        ||  _xMin < 0));

    if (fRecalcText)
    {
        CBorderInfo bi;
        CUnitValue  uvWidth  = GetFirstBranch()->GetCascadedwidth();
        CUnitValue  uvHeight = GetFirstBranch()->GetCascadedheight();

        ElementOwner()->GetBorderInfo(pci, &bi, TRUE);
        if ( uvWidth.IsNullOrEnum() || uvHeight.IsNullOrEnum())
        {
            _fContentsAffectSize = TRUE;

            psize->cx = 1 + bi.aiWidths[BORDER_LEFT]
                          + bi.aiWidths[BORDER_RIGHT];
            psize->cx = pci->DocPixelsFromWindowX(psize->cx);
            psize->cy = 1 + bi.aiWidths[BORDER_TOP]
                          + bi.aiWidths[BORDER_BOTTOM];
            psize->cy = pci->DocPixelsFromWindowY(psize->cy);
        }

        if (!uvWidth.IsNullOrEnum())
        {
            psize->cx = uvWidth.XGetPixelValue(pci, pci->_sizeParent.cx,
                             GetFirstBranch()->GetFontHeightInTwips(&uvWidth));
        }

        if (!uvHeight.IsNullOrEnum())
        {
            psize->cy = uvHeight.YGetPixelValue(pci, pci->_sizeParent.cy,
                             GetFirstBranch()->GetFontHeightInTwips(&uvHeight));
        }

        SIZE    sizeControl;
        sizeControl = *psize;

        grfReturn = super::CalcSize(pci, psize, &sizeControl);
    }
    else
    {
        grfReturn = super::CalcSize(pci, psize);
    }
    return grfReturn;
}

void
CLegendLayout::GetMarginInfo(CParentInfo *ppri,
                                LONG * plLMargin,
                                LONG * plTMargin,
                                LONG * plRMargin,
                                LONG * plBMargin)
{
    CParentInfo     PRI;
    long            lDefMargin;

    if (!ppri)
    {
        // BUGBUG: MohanB - Must create an overload of CParentInfo::Init
        // that takes a CLayout* as an argument
        PRI.Init(this);
        ppri = &PRI;
    }
    lDefMargin = ppri->DocPixelsFromWindowX(FIELDSET_CAPTION_OFFSET
                                            + FIELDSET_BORDER_OFFSET);

    super::GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);

    if (plLMargin)
    {
        *plLMargin += lDefMargin;
    }
    if (plRMargin)
    {
        *plRMargin += lDefMargin;
    }
}


void
CLegendLayout::GetLegendInfo(SIZE *pSizeLegend, POINT *pPosLegend)
{
    CDispNode * pDispNode = GetElementDispNode();

    if (pDispNode)
    {
        pDispNode->GetSize(pSizeLegend);
        *pPosLegend = pDispNode->GetPosition();
    }
    else
    {
        *pSizeLegend = g_Zero.size;
    }
}

void
CFieldSetLayout::DrawClient(
    const RECT *    prcBounds,
    const RECT *    prcRedraw,
    CDispSurface *  pDispSurface,
    CDispNode *     pDispNode,
    void *          cookie,
    void *          pClientData,
    DWORD           dwFlags)
{
    Assert(pClientData);

    super::DrawClient(prcBounds, prcRedraw, pDispSurface, pDispNode, cookie, pClientData, dwFlags);

    // the only thing we draw here is the border

    CFormDrawInfo * pDI = (CFormDrawInfo *)pClientData;
    CSetDrawSurface sds(pDI, prcBounds, prcRedraw, pDispSurface);

    if (DYNCAST(CFieldSetElement, ElementOwner())->GetLegendLayout())
    {
        CBorderInfo bi;

        DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = TRUE;
        if (ElementOwner()->GetBorderInfo(pDI, &bi, TRUE) != DISPNODEBORDER_NONE)
        {
            long lBdrLeft= pDI->DocPixelsFromWindowY(bi.aiWidths[BORDER_LEFT]);

            // only draws the top border
            bi.wEdges = BF_TOP;
            bi.aiWidths[BORDER_LEFT] = 0;
            bi.aiWidths[BORDER_RIGHT] = 0;
            bi.aiWidths[BORDER_BOTTOM] = 0;

            if (bi.sizeCaption.cx > lBdrLeft)
            {
                bi.sizeCaption.cx -= lBdrLeft;
                bi.sizeCaption.cy -= lBdrLeft;
            }

            ((CRect &)(pDI->_rcClip)).IntersectRect(*prcBounds);

            DrawBorder(pDI, &pDI->_rc, &bi);
        }
        DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = FALSE;
    }
}
void CFieldSetLayout::DrawClientBorder(
                const RECT *   prcBounds,
                const RECT *   prcRedraw,
                CDispSurface * pDispSurface,
                CDispNode *    pDispNode,
                void *         pClientData,
                DWORD          dwFlags)
{
    DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = TRUE;
    super::DrawClientBorder(prcBounds, prcRedraw, pDispSurface, pDispNode, pClientData, dwFlags);
    DYNCAST(CFieldSetElement, ElementOwner())->_fDrawing = FALSE;
}
