//+---------------------------------------------------------------------------
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1994-1996
//
//  File:       marqlyt.cxx
//
//  Contents:   Implementation of CMarqueeLayout
//
//----------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_FLOWLYT_HXX_
#define X_FLOWLYT_HXX_
#include "flowlyt.hxx"
#endif

#ifndef X_MARQLYT_HXX_
#define X_MARQLYT_HXX_
#include "marqlyt.hxx"
#endif

#ifndef X_TASKMAN_HXX_
#define X_TASKMAN_HXX_
#include "taskman.hxx"
#endif

#ifndef X_TXTSITE_HXX_
#define X_TXTSITE_HXX_
#include "txtsite.hxx"
#endif

#ifndef X_MARQUEE_HXX_
#define X_MARQUEE_HXX_
#include "marquee.hxx"
#endif

#ifndef X_TREEPOS_HXX_
#define X_TREEPOS_HXX_
#include "treepos.hxx"
#endif


MtDefine(CMarqueeLayout, Layout, "CMarqueeLayout")

const CLayout::LAYOUTDESC CMarqueeLayout::s_layoutdesc =
{
    LAYOUTDESC_NOSCROLLBARS |
    LAYOUTDESC_FLOWLAYOUT,          // _dwFlags
};

HRESULT
CMarqueeLayout::Init()
{
    CMarquee * pMarquee = DYNCAST(CMarquee, ElementOwner());
    HRESULT hr = super::Init();

    if(hr)
        goto Cleanup;

    GetDisplay()->SetWordWrap(pMarquee->_direction == htmlMarqueeDirectionup ||
                pMarquee->_direction == htmlMarqueeDirectiondown ? TRUE : FALSE);
Cleanup:
    RRETURN(hr);
}

//+-------------------------------------------------------------------------
//
//  Method:     CMarqueeLayout::CalcSize
//
//  Synopsis:   Calculate the size of the object
//
//--------------------------------------------------------------------------

DWORD
CMarqueeLayout::CalcSize( CCalcInfo * pci,
                          SIZE *      psize,
                          SIZE *      psizeDefault)
{
    Assert(ElementOwner());
    CScopeFlag   csfCalcing(this);
    CMarquee   * pMarquee = DYNCAST(CMarquee, ElementOwner());

    Assert(pci);
    Assert(psize);

    // This routine does not handle set, or page requests
    Assert(pci->_smMode != SIZEMODE_SET     &&
           pci->_smMode != SIZEMODE_PAGE     );

    CSaveCalcInfo   sci(pci, this);

    SIZE        sizeView;
    SIZE        sizeSave;
    CSize       sizeOriginal;
    CUnitValue  uvWidth      = GetFirstBranch()->GetCascadedwidth();
    CUnitValue  uvHeight     = GetFirstBranch()->GetCascadedheight();
    BOOL        fMinMax      = (   pci->_smMode == SIZEMODE_MMWIDTH
                                || pci->_smMode == SIZEMODE_MINWIDTH
                               );
    BOOL        fNormalMode  = (    pci->_smMode == SIZEMODE_NATURAL
                                ||  pci->_smMode == SIZEMODE_SET
                                ||  pci->_smMode == SIZEMODE_FULLSIZE);
    DWORD       grfReturn;

    long        lOldYMargin = pMarquee->_lYMargin;
    long        lOldXMargin = pMarquee->_lXMargin;

    long    bdrH;
    long    bdrV;

    CBorderInfo bi;

    GetSize(&sizeOriginal);

    if (_fForceLayout)
    {
        pci->_grfLayout |= LAYOUT_FORCE;
        _fForceLayout = FALSE;
    }
    
    grfReturn  = (pci->_grfLayout & LAYOUT_FORCE);
    _fSizeThis = (_fSizeThis || (pci->_grfLayout & LAYOUT_FORCE));

    if (!_fSizeThis && fNormalMode)
    {
        *psize = sizeOriginal;
        return grfReturn;
    }

    ElementOwner()->GetBorderInfo(pci, &bi, TRUE);

    bdrH = pci->DocPixelsFromWindowX(bi.aiWidths[BORDER_LEFT]
                                    + bi.aiWidths[BORDER_RIGHT]);
    bdrV = pci->DocPixelsFromWindowY(bi.aiWidths[BORDER_BOTTOM]
                                    + bi.aiWidths[BORDER_TOP]);

    //
    // If dirty, ensure display tree nodes exist
    //

    if (    _fSizeThis
        &&  (EnsureDispNode(pci, (grfReturn & LAYOUT_FORCE)) == S_FALSE))
    {
        grfReturn |= LAYOUT_HRESIZE | LAYOUT_VRESIZE;
    }

    // Set default width to 100%
    if (uvWidth.IsNullOrEnum())
    {
        uvWidth.SetPercent(100);
    }

    // Set default height for vertical marquees only to 200pix
    if (    uvHeight.IsNull()
        &&  (   pMarquee->_direction == htmlMarqueeDirectionup
            ||  pMarquee->_direction == htmlMarqueeDirectiondown))
    {
        
        uvHeight.SetValue(200, CUnitValue::UNIT_PIXELS);
    }



    // Determine the basic "view" size from the user-supplied settings
    sizeView.cx = (!PercentWidth() || !fMinMax
                        ? uvWidth.XGetPixelValue(pci,
                                                 pci->_sizeParent.cx,
                                                 GetFirstBranch()->GetFontHeightInTwips(&uvWidth))
                        : 0);
    sizeView.cy = (!uvHeight.IsNullOrEnum() && (!PercentHeight() || !fMinMax)
                        ? uvHeight.YGetPixelValue(pci,
                                                  pci->_sizeParent.cy,
                                                  GetFirstBranch()->GetFontHeightInTwips(&uvHeight))
                        : 0);


    // Layout the contained text using the "view" size
    // (This must be always be performed since our container may optimize away the
    //  normal SIZEMODE_NATURAL call. However, if at least one dimension is expressed
    //  as a percentage, then a subsequent call, with SIZEMODE_NATURAL and a correct
    //  _cxParent in the CI, will be made.)
    if (!fMinMax || !PercentSize())
    {
        CSaveCalcInfo   sci(pci);
        BOOL    fLeftOrRight =      pMarquee->_direction == htmlMarqueeDirectionright
                                ||  pMarquee->_direction == htmlMarqueeDirectionleft;

        // Set up margins (which should not include spacing)
        if (fLeftOrRight)
        {
            pMarquee->_lXMargin = max(0L, sizeView.cx - bdrH);
            pMarquee->_lYMargin = 0;
        }
        else
        {
            pMarquee->_lXMargin = 0;
            pMarquee->_lYMargin = max(0L, sizeView.cy - bdrV);
        }

        if (lOldYMargin !=  pMarquee->_lYMargin || lOldXMargin != pMarquee->_lXMargin)
        {
            // We should always force a recalc when margins changed. The display has no
            // other way to realize it.
            _dp.FlushRecalc();
        }
        // Ensure there is some height (or else layout will not occur)
        if (!sizeView.cy)
        {
            sizeView.cy = 1 + bdrV;
        }

        // Disable sizing to content during text layout
        // (This ensures that CalcSize returns the "view" size rather than the text size)
        _fContentsAffectSize = FALSE;
        pci->_smMode = SIZEMODE_NATURAL;
        sizeSave = sizeView;
        if (fLeftOrRight)
        {
            sizeView.cx = 1 + bdrH;
        }
        grfReturn |= super::CalcSize(pci, &sizeView);

        // Ensure enough height to display all contained text (for horizontal flow only)
        // BUGBUG: This is not quite correct since it does not take into account insets (brendand)
        if (fLeftOrRight)
        {
            int cyView = _dp.GetHeight() + bdrV;

            // If the necessary height differs from the current "view" height,
            // re-calc one last time to establish the correct height
            // (The returned sized is ignored since it can only change if the text contains
            //  objects whose height is a percentage of the view. To prevent infinite
            //  recursion (since each change in height would cause a height change), those
            //  objects are "clipped" to the view if necessary.)
            if (sizeView.cy < cyView || _fChildWidthPercent)
            {
                _fSizeThis = TRUE;
                if (sizeView.cy < cyView)
                {
                    sizeView.cy = cyView;
                }
                if (_fChildWidthPercent)
                {
                    sizeView.cx = sizeSave.cx;
                }

                grfReturn |= super::CalcSize(pci, &sizeView);
            }
            sizeView.cx = sizeSave.cx;
        }
    }

    // Any change to content should cause resize since we measure our 
    // _sizeScroll only at CalcSize time
    _fContentsAffectSize = TRUE;

    psize->cx = sizeView.cx;
    psize->cy = (pci->_smMode == SIZEMODE_MMWIDTH
                        ? sizeView.cx
                        : sizeView.cy);


    if (!fMinMax)
    {
        pMarquee->_sizeScroll.cx = _dp.GetWidth();
        pMarquee->_sizeScroll.cy = _dp.GetHeight();

        grfReturn |= LAYOUT_THIS  |
                     (psize->cx != sizeOriginal.cx
                             ? LAYOUT_HRESIZE
                             : 0) |
                     (psize->cy != sizeOriginal.cy
                             ? LAYOUT_VRESIZE
                             : 0);

        pMarquee->_fToBigForSwitch = (pMarquee->_sizeScroll.cx - 2 * pMarquee->_lXMargin) >= pMarquee->_lXMargin;
        pMarquee->InitScrollParams();

        //
        // If size changes occurred, size the display nodes
        //

        if (grfReturn & (LAYOUT_FORCE | LAYOUT_HRESIZE | LAYOUT_VRESIZE))
        {
            SizeDispNode(pci, *psize);
            SizeContentDispNode(CSize(_dp.GetMaxWidth(), _dp.GetHeight()));
        }
        // temp
        _dp.SetViewSize(CRect(CSize(_dp.GetMaxWidth(), _dp.GetHeight())));
    }
    else
    {
        _fSizeThis = TRUE;
    }

    return grfReturn;
}


void
CMarqueeLayout::GetMarginInfo(CParentInfo *ppri,
                                LONG * plLMargin,
                                LONG * plTMargin,
                                LONG * plRMargin,
                                LONG * plBMargin)
{
    CMarquee *      pMarquee    = DYNCAST(CMarquee, ElementOwner());
    LONG            lhMargin    = pMarquee->GetAAhspace();
    LONG            lvMargin    = pMarquee->GetAAvspace();

    if (lhMargin < 0)
        lhMargin = 0;
    if (lvMargin < 0)
        lvMargin = 0;

    super::GetMarginInfo( ppri, plLMargin, plTMargin, plRMargin, plBMargin);

    if (plLMargin)
    {
        *plLMargin += lhMargin;
    }
    if (plRMargin)
    {
        *plRMargin += lhMargin;
    }

    if (plTMargin)
    {
        *plTMargin += lvMargin;
    }
    if (plBMargin)
    {
        *plBMargin += lvMargin;
    }
}
